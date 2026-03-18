import subprocess
import json
import argparse
import sys
import ollama

def sanitize_pr_data(raw_json):
    """Removes metadata to save tokens and focus on narrative content."""
    data = json.loads(raw_json)
    clean_data = {
        "title": data.get("title"),
        "body": data.get("body"),
        "comments": [c.get("body") for c in data.get("comments", []) if c.get("body")],
        "reviews": [r.get("body") for r in data.get("reviews", []) if r.get("body")]
    }
    return clean_data

def get_prs_with_discussions(tag1, tag2):
    try:
        # Get dates for the tags
        start = subprocess.check_output(f"gh release view {tag1} --json publishedAt -q .publishedAt", shell=True, text=True).strip()
        end = subprocess.check_output(f"gh release view {tag2} --json publishedAt -q .publishedAt", shell=True, text=True).strip()

        # Get list of merged PR IDs AND titles
        cmd = f'gh pr list --state merged --search "merged:{start}..{end}" --json number,title'
        pr_list = json.loads(subprocess.check_output(cmd, shell=True, text=True))
        
        enriched_prs = []
        for pr in pr_list:
            num = pr['number']
            title = pr['title']
            # Improved console output including the title
            print(f"   > Fetching context for PR #{num}: {title}")
            
            detail_cmd = f'gh pr view {num} --json title,body,comments,reviews'
            raw_details = subprocess.check_output(detail_cmd, shell=True, text=True)
            
            # Sanitize and add to our collection
            enriched_prs.append(sanitize_pr_data(raw_details))
            
        return json.dumps(enriched_prs, indent=2)
        
    except subprocess.CalledProcessError as e:
        print(f"Error fetching data from GitHub: {e}")
        sys.exit(1)

def run_ollama_logic(new_prs_json, old_summary, model):
    # Your comprehensive prompt remains the same
    prompt = f"""
    You are a technical writer maintaining the working Release Announcement draft for Jamulus...
    
    [... PROMPT TRUNCATED FOR BREVITY - USE YOUR EXISTING PROMPT HERE ...]

    EXISTING SUMMARY:
    {old_summary if old_summary else "No existing summary provided."}

    NEW PR DATA (Cleaned Discussion Context):
    {new_prs_json}

    RETURN ONLY THE COMPLETE UPDATED MARKDOWN DOCUMENT AS THE FINAL OUTPUT.
    """
    
    response = ollama.chat(model=model, messages=[{'role': 'user', 'content': prompt}])
    return response['message']['content']

def main():
    parser = argparse.ArgumentParser(description="Generate a narrative Release Announcement using Ollama.")
    parser.add_argument("start_tag", help="The older tag")
    parser.add_argument("end_tag", help="The newer tag")
    parser.add_argument("--summary", help="Path to existing Markdown draft.")
    parser.add_argument("--model", default="mistral-large-3:675b-cloud", help="Ollama model.")

    args = parser.parse_args()

    existing_content = ""
    if args.summary:
        try:
            with open(args.summary, 'r') as f:
                existing_content = f.read()
        except FileNotFoundError:
            existing_content = args.summary

    print(f"--- Gathering PRs between {args.start_tag} and {args.end_tag} ---")
    full_data = get_prs_with_discussions(args.start_tag, args.end_tag)

    print(f"--- Synthesizing Narrative with {args.model} ---")
    result = run_ollama_logic(full_data, existing_content, args.model)
    
    print("\n--- UPDATED RELEASE ANNOUNCEMENT ---")
    print(result)

if __name__ == "__main__":
    main()
