import subprocess
import json
import argparse
import sys
import os
import ollama
import re

def get_universal_timestamp(identifier):
    """
    Resolves an identifier to an ISO8601 timestamp.
    Supports: pr123, Tags, SHAs, and Branches.
    """
    target = identifier

    # 1. Resolve PR to its merge commit SHA
    if identifier.lower().startswith("pr"):
        pr_id = identifier[2:]
        try:
            print(f"   > Resolving PR #{pr_id} to its merge commit...")
            cmd = f"gh pr view {pr_id} --json mergeCommit -q .mergeCommit.oid"
            target = subprocess.check_output(cmd, shell=True, text=True).strip()

            if not target or target == "null":
                print(f"Error: PR #{pr_id} has no merge commit (is it merged?)")
                sys.exit(1)
        except subprocess.CalledProcessError:
            print(f"Error: Failed to fetch PR #{pr_id} from GitHub.")
            sys.exit(1)

    # 2. Get authoritative Git timestamp for the target
    try:
        return subprocess.check_output(
            ["git", "show", "-s", "--format=%cI", target],
            text=True
        ).strip()
    except subprocess.CalledProcessError:
        print(f"Error: Could not resolve '{target}' as a Git object.")
        sys.exit(1)

def get_ordered_pr_list(start_iso, end_iso):
    """Fetches PRs merged in range, ordered oldest to newest."""
    cmd = f'gh pr list --state merged --search "merged:{start_iso}..{end_iso}" --json number,title,mergedAt'
    prs = json.loads(subprocess.check_output(cmd, shell=True, text=True))
    return sorted(prs, key=lambda x: x['mergedAt'])

def sanitize_pr_data(raw_json):
    """Strips metadata and GitHub noise to save tokens."""
    data = json.loads(raw_json)
    # Remove "(Fixes #123)" and "(Closes #123)"
    clean_body = re.sub(r'\(?(Fixes|Closes) #\d+\)?', '', data.get("body", ""), flags=re.IGNORECASE)

    return {
        "number": data.get("number"),
        "title": data.get("title"),
        "body": clean_body,
        "comments": [c.get("body") for c in data.get("comments", []) if c.get("body")],
        "reviews": [r.get("body") for r in data.get("reviews", []) if r.get("body")]
    }

def run_ollama_logic(new_pr_data, old_summary, model):
    # Determine if this looks like a minor fix
    is_fix = any(word in new_pr_data['title'].lower() for word in ['fix', 'correct', 'typo'])

    """Enforces narrative style using Few-Shot examples."""
    style_guide = """
    STYLE GUIDELINES:
    - NO BULLET POINTS. Use friendly, editorial narrative prose.
    - GROUP BY AUDIENCE: ## For everyone, ## For Windows users, ## For macOS users, ## For mobile users (iOS & Android), ## For server operators, ## Translations.
    - FOCUS ON BENEFITS: Describe what users can DO now, not code changes.
    - DEDUPLICATE: If a feature is already mentioned, update the existing paragraph.
    - CLARITY: If you are unsure of the user benefit, do not write a whole new heading.
      Add a single, simple sentence to the most relevant 'For users' section.
      Example: "The iOS 'About' dialog now correctly displays the operating system version."
    SPECIAL INSTRUCTION FOR THIS PR:
    {'This is a minor fix. DO NOT create a new level-2 (##) heading. Only add a single sentence to an existing section.' if is_fix else 'Integrate this normally.'}
    """
    high_bar_rule = """
    - THE HIGH BAR RULE: Only mention changes that significantly improve the
      musician's experience. If a PR is purely "housekeeping" (like copyright
      updates, minor internal refactoring, or CI fixes), return the document
      UNCHANGED.
    - CRITICAL RULES FOR TRUTH:
      1. NO GUESSING: Do not invent benefits. Only describe benefits explicitly mentioned
         in the PR Body or Developer Comments.
      2. THE "ABOUT BOX" RULE: If a fix only affects internal metadata or the 'About'
         dialog, describe it simply as "UI polish" or "Information accuracy."
      3. CHECK THE CODE: If the diff shows changes to `src/util.h` or version strings,
         it usually means the 'About' box or internal identification is being corrected.
    - AGGREGATE SMALL FIXES: Do not give a dedicated paragraph to every bug fix.
      For example, small iOS fixes should be woven into one "Mobile Improvements" paragraph.
    - DELETE THE FILLER: If the existing text contains fluff (e.g., "demonstrates
      our ongoing commitment"), remove it. Keep the announcement punchy.
    - CHANGELOG SKIP: Unless counter-indicated in the discussion, such PRs should be skipped.
    """

    prompt = f"""
    You are a technical writer for Jamulus. You must follow the following rules and guidance.
    {high_bar_rule}
    {style_guide}

    TASK:
    Integrate this ONE new Pull Request into the EXISTING Release Announcement.
    If the change is significant, give it a dedicated level-2 heading before the audience sections.

    EXISTING ANNOUNCEMENT:
    {old_summary if old_summary else "# Jamulus Release Announcement"}

    NEW PR TO INTEGRATE:
    {json.dumps(new_pr_data, indent=2)}

    RETURN THE COMPLETE UPDATED MARKDOWN DOCUMENT ONLY.
    """

    response = ollama.chat(model=model, messages=[{'role': 'user', 'content': prompt}])
    return response['message']['content'].strip()

def strip_markdown_fences(text):
    """
    Removes ```markdown ... ``` or ``` ... ``` wrappers
    that LLMs often add to their responses.
    """
    # Remove the opening fence (with or without 'markdown' tag)
    text = re.sub(r'^```(markdown)?\n', '', text, flags=re.IGNORECASE)
    # Remove the closing fence
    text = re.sub(r'\n```$', '', text)
    return text.strip()

def main():
    parser = argparse.ArgumentParser(description="Progressive Release Announcement Generator")
    parser.add_argument("start", help="Starting boundary (e.g. pr3409 or v3.11.0)")
    parser.add_argument("end", help="Ending boundary (e.g. pr3500 or HEAD)")
    parser.add_argument("--file", required=True, help="Markdown file to update")
    parser.add_argument("--model", default="mistral-large-3:675b-cloud")
    args = parser.parse_args()

    # 1. Resolve Timeframes
    print(f"--- Resolving boundaries: {args.start} -> {args.end} ---")
    start_ts = get_universal_timestamp(args.start)
    end_ts = get_universal_timestamp(args.end)

    # 2. Fetch and Filter
    all_prs = get_ordered_pr_list(start_ts, end_ts)
    start_pr_num = args.start[2:] if args.start.lower().startswith("pr") else None
    todo_prs = [p for p in all_prs if str(p['number']) != start_pr_num]

    if not todo_prs:
        print("No new PRs found to process.")
        return

    print(f"--- Found {len(todo_prs)} PRs to merge oldest-to-newest ---")

    # 3. Progressive Merge Loop
    for pr in todo_prs:
        pr_num = pr['number']
        pr_title = pr['title']

        with open(args.file, 'r') as f:
            current_content = f.read()

        print(f"--- Processing PR #{pr_num}: {pr_title} ---")

        raw_details = subprocess.check_output(f'gh pr view {pr_num} --json number,title,body,comments,reviews', shell=True, text=True)
        sanitized_pr = sanitize_pr_data(raw_details)

        # AI processing
        updated_ra = run_ollama_logic(sanitized_pr, current_content, args.model)

        # CLEAN THE OUTPUT
        updated_ra = strip_markdown_fences(updated_ra)

        with open(args.file, 'w') as f:
            f.write(updated_ra)

        # 4. Git Commit
        commit_msg = f"[bot] RA: Merge #{pr_num}: {pr_title}"
        subprocess.run(["git", "add", args.file], check=True)
        subprocess.run(["git", "commit", "-m", commit_msg], check=True)
        print(f"   Successfully committed.")

    print(f"\n--- Done! Processed {len(todo_prs)} PRs. ---")

if __name__ == "__main__":
    main()
