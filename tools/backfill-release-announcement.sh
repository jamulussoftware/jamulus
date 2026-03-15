#!/bin/bash
##############################################################################
# Copyright (c) 2024-2026
#
# Author(s):
#  The Jamulus Development Team
#
##############################################################################
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
##############################################################################

# Backfills ReleaseAnnouncement.md with every PR merged since a given release
# tag, processing them one by one so the announcement builds up naturally.
#
# Requirements: git, GitHub CLI (gh), jq, curl
#
# Usage:
#   ./tools/backfill-release-announcement.sh [SINCE_TAG [UNTIL_TAG]]
#
#   SINCE_TAG   git tag of the release to start from (default: r3_11_0).
#               PRs merged *after* this tag's date will be included.
#   UNTIL_TAG   git tag or release to stop at (optional).
#               PRs merged *on or before* this tag's date will be included.
#               Useful for processing in batches to avoid API rate limits.
#
# Environment variables:
#   GITHUB_TOKEN   GitHub token with 'models: read' scope (required for AI).
#   SOURCE_REPO    Repository to read PRs from (default: jamulussoftware/jamulus).
#   ANNOUNCEMENT_FILE  Path to the announcement file to update
#                      (default: ReleaseAnnouncement.md).
#   DRY_RUN        Set to 'true' to print what would be done without making
#                  any changes (useful for testing).
#   UNTIL_DATE     ISO 8601 date/timestamp upper bound for merged PRs.
#                  Alternative to UNTIL_TAG when you don't have a tag handy
#                  (e.g. UNTIL_DATE=2025-06-01T00:00:00Z).
#   FROM_PR        Only process PRs with number >= this value (inclusive).
#                  Useful for resuming a partial run or processing in batches
#                  by PR number (e.g. FROM_PR=3560).
#   TO_PR          Only process PRs with number <= this value (inclusive).
#                  Combine with FROM_PR to process a specific PR number range
#                  (e.g. TO_PR=3580).
#   DELAY_SECS     Seconds to sleep between AI API calls (default: 0).
#                  Set to 4 or 5 when running long backfills to stay within
#                  the GitHub Models API rate limit (≈15 requests/minute on
#                  the free tier).  Example: DELAY_SECS=5
#
# The script calls the GitHub Models API (openai/gpt-4o-mini) to produce a
# user-friendly summary for each PR and adds it to the announcement document.
# Changes that are not user-facing (CI, build tooling, code style, routine
# dependency bumps) are omitted by the AI automatically.
#
# PRs whose body contains a line matching "CHANGELOG: SKIP" are skipped
# unconditionally, consistent with the per-PR workflow behaviour.
#
# Rate limiting: the GitHub Models API free tier allows ~15 requests/minute
# and ~150 requests/day.  When backfilling many PRs in one run the script will
# hit the per-minute limit.  Remedies:
#   1. Set DELAY_SECS=5 to pace calls (processes ~12 PRs/minute).
#   2. Use UNTIL_TAG / FROM_PR + TO_PR to split the work into smaller batches
#      and run each batch separately.

set -eu -o pipefail

SINCE_TAG="${1:-r3_11_0}"
UNTIL_TAG="${2:-}"
SOURCE_REPO="${SOURCE_REPO:-jamulussoftware/jamulus}"
ANNOUNCEMENT_FILE="${ANNOUNCEMENT_FILE:-ReleaseAnnouncement.md}"
DRY_RUN="${DRY_RUN:-false}"
MODELS_ENDPOINT="${MODELS_ENDPOINT:-https://models.github.ai/inference/chat/completions}"
MODEL="${MODEL:-openai/gpt-4o-mini}"
UNTIL_DATE="${UNTIL_DATE:-}"
FROM_PR="${FROM_PR:-}"
TO_PR="${TO_PR:-}"
DELAY_SECS="${DELAY_SECS:-0}"
PR_LIST_LIMIT=500
MAX_PR_NUMBER=2147483647  # upper bound when TO_PR is not specified

# Colours for interactive output (suppressed when not on a terminal)
if [[ -t 1 ]]; then
    C_BOLD='\033[1m'
    C_GREEN='\033[0;32m'
    C_YELLOW='\033[0;33m'
    C_CYAN='\033[0;36m'
    C_RESET='\033[0m'
else
    C_BOLD='' C_GREEN='' C_YELLOW='' C_CYAN='' C_RESET=''
fi

# ── helpers ────────────────────────────────────────────────────────────────

info()    { printf "${C_CYAN}→${C_RESET} %s\n"           "$*"; }
success() { printf "${C_GREEN}✓${C_RESET} %s\n"          "$*"; }
warn()    { printf "${C_YELLOW}⚠${C_RESET}  %s\n"        "$*" >&2; }
heading() { printf "\n${C_BOLD}%s${C_RESET}\n"            "$*"; }

require_cmd() {
    local cmd=$1
    if ! command -v "$cmd" &> /dev/null; then
        echo "ERROR: '$cmd' is required but was not found in PATH." >&2
        exit 1
    fi
}

# ── pre-flight checks ──────────────────────────────────────────────────────

require_cmd git
require_cmd gh
require_cmd jq
require_cmd curl

if [[ -z "${GITHUB_TOKEN:-}" ]]; then
    # Try to pick up the token from the gh CLI credential store
    GITHUB_TOKEN=$(gh auth token 2>/dev/null || true)
    if [[ -z "$GITHUB_TOKEN" ]]; then
        echo "ERROR: GITHUB_TOKEN is not set and 'gh auth token' returned nothing." >&2
        echo "       Run 'gh auth login' or export GITHUB_TOKEN before running this script." >&2
        exit 1
    fi
fi
export GITHUB_TOKEN

if [[ ! -f "$ANNOUNCEMENT_FILE" ]]; then
    echo "ERROR: Announcement file not found: $ANNOUNCEMENT_FILE" >&2
    echo "       Run this script from the repository root." >&2
    exit 1
fi

# ── look up the tag date ───────────────────────────────────────────────────

heading "Resolving release tag ${SINCE_TAG} in ${SOURCE_REPO} …"

# Try the Releases API first (gives the published date), then fall back to the
# git tag object (annotated tags), then to the tagged commit (lightweight tags).
TAG_DATE=$(
    GH_REPO="$SOURCE_REPO" gh release view "$SINCE_TAG" \
        --json publishedAt --jq '.publishedAt' 2>/dev/null \
    || GH_REPO="$SOURCE_REPO" gh api \
        "/repos/${SOURCE_REPO}/git/refs/tags/${SINCE_TAG}" \
        --jq '.object.sha' 2>/dev/null \
        | xargs -I{} GH_REPO="$SOURCE_REPO" gh api \
            "/repos/${SOURCE_REPO}/git/tags/{}" \
            --jq '.tagger.date' 2>/dev/null \
    || true
)

if [[ -z "$TAG_DATE" ]]; then
    # Last resort: look up the commit that the tag points to and use its date
    TAG_DATE=$(
        GH_REPO="$SOURCE_REPO" gh api \
            "/repos/${SOURCE_REPO}/commits/tags/${SINCE_TAG}" \
            --jq '.commit.committer.date' 2>/dev/null || true
    )
fi

if [[ -z "$TAG_DATE" ]]; then
    echo "ERROR: Could not resolve date for tag '${SINCE_TAG}' in ${SOURCE_REPO}." >&2
    echo "       Check that the tag exists and that your token has read access." >&2
    exit 1
fi

info "Tag ${SINCE_TAG} resolves to date: ${TAG_DATE}"

# ── look up the until-tag date (if specified) ──────────────────────────────

if [[ -n "$UNTIL_TAG" && -z "$UNTIL_DATE" ]]; then
    heading "Resolving upper-bound tag ${UNTIL_TAG} in ${SOURCE_REPO} …"

    UNTIL_DATE=$(
        GH_REPO="$SOURCE_REPO" gh release view "$UNTIL_TAG" \
            --json publishedAt --jq '.publishedAt' 2>/dev/null \
        || GH_REPO="$SOURCE_REPO" gh api \
            "/repos/${SOURCE_REPO}/git/refs/tags/${UNTIL_TAG}" \
            --jq '.object.sha' 2>/dev/null \
            | xargs -I{} GH_REPO="$SOURCE_REPO" gh api \
                "/repos/${SOURCE_REPO}/git/tags/{}" \
                --jq '.tagger.date' 2>/dev/null \
        || true
    )

    if [[ -z "$UNTIL_DATE" ]]; then
        UNTIL_DATE=$(
            GH_REPO="$SOURCE_REPO" gh api \
                "/repos/${SOURCE_REPO}/commits/tags/${UNTIL_TAG}" \
                --jq '.commit.committer.date' 2>/dev/null || true
        )
    fi

    if [[ -z "$UNTIL_DATE" ]]; then
        echo "ERROR: Could not resolve date for tag '${UNTIL_TAG}' in ${SOURCE_REPO}." >&2
        echo "       Check that the tag exists and that your token has read access." >&2
        exit 1
    fi

    info "Tag ${UNTIL_TAG} resolves to date: ${UNTIL_DATE}"
fi

# ── fetch the PR list ──────────────────────────────────────────────────────

if [[ -n "$UNTIL_DATE" ]]; then
    heading "Fetching merged PRs in ${SOURCE_REPO} after ${TAG_DATE} up to ${UNTIL_DATE} …"
else
    heading "Fetching merged PRs in ${SOURCE_REPO} after ${TAG_DATE} …"
fi

# gh pr list returns JSON; we sort by mergedAt ascending so the announcement
# builds up in the same order that changes actually landed.
PR_SEARCH="merged:>${TAG_DATE} base:main"
if [[ -n "$UNTIL_DATE" ]]; then
    PR_SEARCH="${PR_SEARCH} merged:<=${UNTIL_DATE}"
fi
PR_JSON=$(
    GH_REPO="$SOURCE_REPO" gh pr list \
        --state merged \
        --base main \
        --search "$PR_SEARCH" \
        --limit "$PR_LIST_LIMIT" \
        --json number,title,author,body,mergedAt,labels \
        --jq 'sort_by(.mergedAt)'
)

# Apply optional PR-number range filter (FROM_PR / TO_PR)
if [[ -n "$FROM_PR" || -n "$TO_PR" ]]; then
    _from="${FROM_PR:-0}"
    _to="${TO_PR:-$MAX_PR_NUMBER}"
    PR_JSON=$(jq --argjson from "$_from" --argjson to "$_to" \
        '[.[] | select(.number >= $from and .number <= $to)]' <<< "$PR_JSON")
fi

PR_COUNT=$(jq 'length' <<< "$PR_JSON")
info "Found ${PR_COUNT} merged PRs to process."

if [[ "$PR_COUNT" -eq 0 ]]; then
    warn "No PRs found — nothing to do."
    exit 0
fi

# ── read the AI system prompt ──────────────────────────────────────────────

PROMPT_FILE="${PROMPT_FILE:-.github/prompts/release-announcement.prompt.yml}"

if [[ ! -f "$PROMPT_FILE" ]]; then
    echo "ERROR: Prompt file not found: $PROMPT_FILE" >&2
    exit 1
fi

# Extract the system message content from the YAML prompt file.
# The file uses the actions/ai-inference format; the system content is the
# first 'content' block under 'messages'.
SYSTEM_PROMPT=$(
    python3 - "$PROMPT_FILE" <<'PYEOF'
import sys, yaml
with open(sys.argv[1]) as f:
    data = yaml.safe_load(f)
for msg in data.get("messages", []):
    if msg.get("role") == "system":
        print(msg["content"], end="")
        break
PYEOF
)

if [[ -z "$SYSTEM_PROMPT" ]]; then
    echo "ERROR: Could not extract system prompt from ${PROMPT_FILE}." >&2
    exit 1
fi

# ── configure git identity (required for commits) ─────────────────────────
# Use existing config when available; fall back to neutral defaults so the
# script works in CI environments that have no pre-configured identity.

if [[ "$DRY_RUN" != "true" ]]; then
    if ! git config user.email > /dev/null 2>&1; then
        git config user.email "actions@github.com"
    fi
    if ! git config user.name > /dev/null 2>&1; then
        git config user.name "github-actions[bot]"
    fi
fi

# ── process each PR ────────────────────────────────────────────────────────

# strip_code_fences TEXT
# If TEXT is wrapped in a markdown code fence (```lang...```) strip the
# opening and closing fence lines and any leading/trailing blank lines.
strip_code_fences() {
    local text="$1"
    # Remove the opening fence line (``` or ```markdown, etc.)
    text=$(printf '%s' "$text" | sed '1s/^```[a-zA-Z]*$//')
    # Remove the closing fence line (exactly ```)
    text=$(printf '%s' "$text" | sed '$s/^```$//')
    # Drop leading blank lines
    text=$(printf '%s' "$text" | sed '/./,$!d')
    # Drop trailing blank lines
    text=$(printf '%s' "$text" | sed -e :a -e '/^\n*$/{$d;N;ba}')
    printf '%s' "$text"
}

call_model_api() {
    # Arguments:
    #   $1  current announcement text
    #   $2  PR info text
    # Prints the updated announcement text to stdout.
    local current_announcement="$1"
    local pr_info="$2"

    local user_content
    user_content=$(printf \
        'Current working announcement:\n====\n%s\n====\n\nNewly merged pull request:\n%s\n====\n\nUpdate the Release Announcement to include any user-relevant changes from this PR.\nReturn the complete updated Markdown document only.' \
        "$current_announcement" "$pr_info")

    # Build the request payload using jq so that strings are properly escaped.
    local payload
    payload=$(jq -n \
        --arg model  "$MODEL" \
        --arg system "$SYSTEM_PROMPT" \
        --arg user   "$user_content" \
        '{
            model: $model,
            messages: [
                { role: "system", content: $system },
                { role: "user",   content: $user   }
            ],
            max_completion_tokens: 16384,
            temperature: 0.1
        }')

    # Use a temp file so we can capture both the body and the HTTP status code.
    # curl -s -f would hide the status; -w '%{http_code}' lets us detect 429.
    local tmpfile
    tmpfile=$(mktemp)
    local http_code
    http_code=$(curl -s \
        -o "$tmpfile" \
        -w '%{http_code}' \
        -X POST "$MODELS_ENDPOINT" \
        -H "Authorization: Bearer ${GITHUB_TOKEN}" \
        -H "Content-Type: application/json" \
        -d "$payload") 2>/dev/null || true
    local response
    response=$(cat "$tmpfile" 2>/dev/null || true)
    rm -f "$tmpfile"

    if [[ "$http_code" != "200" ]]; then
        if [[ "$http_code" == "429" ]]; then
            warn "Models API rate limit hit (HTTP 429) — keeping current announcement."
            warn "  Tip: set DELAY_SECS=5 to pace calls, or use UNTIL_TAG/FROM_PR/TO_PR"
            warn "  to split the backfill into smaller batches."
        else
            warn "HTTP ${http_code} from Models API — keeping current announcement."
        fi
        printf '%s' "$current_announcement"
        return
    fi

    local updated
    updated=$(jq -r '.choices[0].message.content // empty' <<< "$response")

    if [[ -z "$updated" ]]; then
        warn "Model returned an empty response — keeping current announcement."
        printf '%s' "$current_announcement"
        return
    fi

    # Strip markdown code fences if the model wrapped the output in them
    # (some models output ```markdown ... ``` or ``` ... ```).
    if [[ "$updated" =~ ^'```' ]]; then
        updated=$(strip_code_fences "$updated")
    fi

    # Guard: if the response doesn't start with a Markdown heading it is not a
    # valid document — keep the current announcement to avoid corrupting the file.
    if [[ ! "$updated" =~ ^'#' ]]; then
        warn "AI response does not look like a Markdown document — keeping current announcement."
        printf '%s' "$current_announcement"
        return
    fi

    printf '%s' "$updated"
}

PROCESSED=0
SKIPPED=0
UNCHANGED=0

for row in $(jq -r '.[] | @base64' <<< "$PR_JSON"); do
    # Decode this PR's JSON object
    pr=$(echo "$row" | base64 --decode)
    pr_number=$(jq -r '.number'        <<< "$pr")
    pr_title=$(jq  -r '.title'         <<< "$pr")
    pr_author=$(jq -r '.author.login'  <<< "$pr")
    pr_body=$(jq   -r '.body // ""'    <<< "$pr")
    merged_at=$(jq -r '.mergedAt'      <<< "$pr")

    heading "PR #${pr_number}: ${pr_title} (@${pr_author}, ${merged_at})"

    # ── skip check ────────────────────────────────────────────────────────
    if printf '%s\n' "$pr_body" | grep -qE '^CHANGELOG:[[:space:]]*SKIP[[:space:]]*$'; then
        info "Skipping (CHANGELOG: SKIP)"
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    if [[ "$DRY_RUN" == "true" ]]; then
        info "[DRY RUN] Would call AI for PR #${pr_number}"
        PROCESSED=$((PROCESSED + 1))
        continue
    fi

    # ── build the PR info block ────────────────────────────────────────────
    pr_info=$(printf 'PR #%s — %s\nby @%s\n\n%s\n' \
        "$pr_number" "$pr_title" "$pr_author" "$pr_body")

    # ── call the AI ────────────────────────────────────────────────────────
    current_announcement=$(cat "$ANNOUNCEMENT_FILE")
    updated_announcement=$(call_model_api "$current_announcement" "$pr_info")

    # ── detect whether the AI made any changes ─────────────────────────────
    if [[ "$updated_announcement" == "$current_announcement" ]]; then
        info "No user-relevant changes — announcement unchanged."
        UNCHANGED=$((UNCHANGED + 1))
        continue
    fi

    # ── write the updated file ─────────────────────────────────────────────
    printf '%s\n' "$updated_announcement" > "$ANNOUNCEMENT_FILE"
    success "Updated announcement with changes from PR #${pr_number}."

    # ── commit this PR's change as its own commit ──────────────────────────
    git add "$ANNOUNCEMENT_FILE"
    # Guard: the content comparison above caught most unchanged cases, but
    # whitespace normalisation (e.g. trailing newlines) can produce byte-identical
    # files that still compare as different strings.  Check git's view before
    # committing so we don't fail under set -eu with "nothing to commit".
    if git diff --staged --quiet; then
        info "No git diff after write — announcement effectively unchanged."
        UNCHANGED=$((UNCHANGED + 1))
        git restore --staged "$ANNOUNCEMENT_FILE" 2>/dev/null \
            || git reset HEAD "$ANNOUNCEMENT_FILE" 2>/dev/null \
            || true
        continue
    fi
    git commit -m "docs: Release Announcement for PR #${pr_number} — ${pr_title}"
    PROCESSED=$((PROCESSED + 1))

    # ── optional delay between API calls ──────────────────────────────────
    if [[ "${DELAY_SECS:-0}" -gt 0 ]]; then
        info "Sleeping ${DELAY_SECS}s before next API call (DELAY_SECS)…"
        sleep "$DELAY_SECS"
    fi
done

# ── summary ────────────────────────────────────────────────────────────────

heading "Done."
info "PRs processed (with AI): ${PROCESSED}"
info "PRs skipped (CHANGELOG: SKIP): ${SKIPPED}"
info "PRs with no user-relevant changes: ${UNCHANGED}"

if [[ "$DRY_RUN" == "true" ]]; then
    warn "DRY_RUN=true — no files were modified."
else
    success "ReleaseAnnouncement.md has been updated and each change committed."
    echo
    echo "Push all commits with:"
    echo "  git push"
fi
