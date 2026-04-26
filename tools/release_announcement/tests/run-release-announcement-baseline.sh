#!/bin/bash
##############################################################################
# Release Announcement Tool Baseline Matrix Runner
#
# Purpose: Execute the Step 0 baseline matrix for the release-announcement
#          tool against representative scenarios (normal PR, SKIP PR, large PR,
#          tag range) with both Ollama and GitHub backends.
#
# Usage:   tools/release_announcement/tests/run-release-announcement-baseline.sh
#
# Output:  Timestamped directory under tools/release_announcement/tests/build/release-announcement-baseline-YYYYMMDD_HHMMSS/
#          containing per-run: metadata, command, timing (with us precision),
#          stdout-stderr, git diff, file snapshots, and exit code.
#
# Prerequisites:
#   - Working directory: repository root
#   - Git worktree: clean (run 'git status' to verify)
#   - tools/update-release-announcement.sh: present and executable
#   - ReleaseAnnouncement.md: present and unchanged
#
# Exit code: 0 on complete (all 8 scenarios attempted)
#            1 if prerequisites not met
##############################################################################

set -euo pipefail

cd "$(git rev-parse --show-toplevel)" || { echo "Error: not in a git repository"; exit 1; }

# Verify prerequisites
if ! [[ -f tools/update-release-announcement.sh ]]; then
  echo "Error: tools/update-release-announcement.sh not found"
  exit 1
fi

if ! [[ -f ReleaseAnnouncement.md ]]; then
  echo "Error: ReleaseAnnouncement.md not found"
  exit 1
fi

if ! git diff --quiet --exit-code; then
  echo "Error: git worktree has uncommitted changes; run 'git status' to review"
  exit 1
fi

# Create timestamped baseline directory
BASELINE_DIR="tools/release_announcement/tests/build/release-announcement-baseline-$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BASELINE_DIR"

# Capture starting state
BASE_COMMIT=$(git rev-parse HEAD)
printf '%s\n' "$BASE_COMMIT" > "$BASELINE_DIR/base_commit.txt"
cp ReleaseAnnouncement.md "$BASELINE_DIR/ReleaseAnnouncement.initial.md"

# Function to run one scenario and capture artifacts
run_case() {
  local backend="$1"
  local label="$2"
  local start="$3"
  local end="${4:-}"
  local dry="${5:-false}"

  local run_dir="$BASELINE_DIR/${backend}__${label}"
  mkdir -p "$run_dir"

  # Build command
  local -a cmd=(./tools/update-release-announcement.sh --delay-secs 0 --backend "$backend" --file ReleaseAnnouncement.md "$start")
  if [[ -n "$end" ]]; then
    cmd+=("$end")
  fi
  if [[ "$dry" == "true" ]]; then
    cmd+=(--dry-run)
  fi

  # Save metadata and command for later inspection
  {
    printf 'Backend: %s\n' "$backend"
    printf 'Label: %s\n' "$label"
    printf 'StartArg: %s\n' "$start"
    printf 'EndArg: %s\n' "${end:-<omitted>}"
    printf 'DryRun: %s\n' "$dry"
  } > "$run_dir/metadata.txt"

  {
    printf 'Command:'
    printf ' %q' "${cmd[@]}"
    printf '\n'
  } > "$run_dir/command.txt"

  # Capture start timestamp (microseconds)
  local start_ns
  local start_iso_us
  start_ns=$(date +%s%N)
  start_iso_us=$(date -u +"%Y-%m-%dT%H:%M:%S.%6NZ")

  # Run command and capture exit code
  set +e
  "${cmd[@]}" > "$run_dir/stdout-stderr.log" 2>&1
  local ec=$?
  set -e

  # Capture end timestamp (microseconds)
  local end_ns
  local end_iso_us
  end_ns=$(date +%s%N)
  end_iso_us=$(date -u +"%Y-%m-%dT%H:%M:%S.%6NZ")

  # Compute duration
  local dur_ns
  local dur_ms
  dur_ns=$((end_ns - start_ns))
  dur_ms=$((dur_ns / 1000000))

  # Save timing information
  {
    printf 'start_iso_utc_us=%s\n' "$start_iso_us"
    printf 'end_iso_utc_us=%s\n' "$end_iso_us"
    printf 'start_epoch_ns=%s\n' "$start_ns"
    printf 'end_epoch_ns=%s\n' "$end_ns"
    printf 'duration_ns=%s\n' "$dur_ns"
    printf 'duration_ms=%s\n' "$dur_ms"
    printf 'exit_code=%s\n' "$ec"
  } > "$run_dir/timing.txt"

  printf '%s\n' "$ec" > "$run_dir/exit_code.txt"

  # Capture file snapshots and diffs
  cp ReleaseAnnouncement.md "$run_dir/ReleaseAnnouncement.after.md"
  git --no-pager diff -- ReleaseAnnouncement.md > "$run_dir/git-diff.txt"
  git status --short > "$run_dir/git-status-short.txt"

  # Reset to baseline for next scenario
  git reset --hard "$BASE_COMMIT" > "$run_dir/reset.log" 2>&1

  # Report result
  echo "✓ $backend/$label exit=$ec duration_ms=$dur_ms"
}

echo "=== Release Announcement Baseline Matrix ==="
echo "Repository: $(git remote get-url origin || echo '<local>')"
echo "Baseline dir: $BASELINE_DIR"
echo ""

# Execute baseline matrix: 4 scenarios × 2 backends = 8 runs
run_case ollama pr3429 pr3429 "" false
run_case github pr3429 pr3429 "" false

run_case ollama pr3625 pr3625 "" false
run_case github pr3625 pr3625 "" false

run_case ollama pr3502 pr3502 "" false
run_case github pr3502 pr3502 "" false

run_case ollama tag_r3_12_0beta4_to_r3_12_0beta5 r3_12_0beta4 r3_12_0beta5 true
run_case github tag_r3_12_0beta4_to_r3_12_0beta5 r3_12_0beta4 r3_12_0beta5 true

# Finalize baseline
cp ReleaseAnnouncement.md "$BASELINE_DIR/ReleaseAnnouncement.final.md"
git reset --hard "$BASE_COMMIT" >/dev/null 2>&1
find "$BASELINE_DIR" -maxdepth 2 -type f | sort > "$BASELINE_DIR/_artifact-index.txt"

echo ""
echo "=== Baseline Complete ==="
echo "Output directory: $BASELINE_DIR"
echo "Artifact count: $(find "$BASELINE_DIR" -maxdepth 2 -type f | wc -l)"
echo ""
echo "To inspect timing:"
echo "  for d in $BASELINE_DIR/*__*; do echo \"\$(basename \$d): \$(grep duration_ms \$d/timing.txt)\"; done"
