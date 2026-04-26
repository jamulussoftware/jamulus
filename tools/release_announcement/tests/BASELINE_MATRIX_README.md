# Release Announcement Tool: Baseline Matrix Execution Guide for AI Agents

## Overview

This document provides AI agents with detailed, actionable guidance for capturing and verifying the **Step 0 baseline matrix** for the release-announcement tool restructuring plan. The baseline captures the tool's current behavior across representative scenarios before any code changes are made.

## Purpose

The baseline matrix establishes:
1. Current behavior of both Ollama and GitHub backends against 4 representative PR scenarios
2. High-precision timing (microsecond start/end, millisecond duration) for each run
3. Exact command invocations, stdout/stderr logs, git diffs, and exit codes
4. A reference against which all post-restructure verification steps compare outputs

## Prerequisites

Before running the baseline, verify:

```bash
# Must be at repository root
cd "$(git rev-parse --show-toplevel)"

# Worktree must be clean
git status --short

# These files must exist
test -f tools/update-release-announcement.sh
test -f ReleaseAnnouncement.md
```

All three prerequisites must pass; if any fail, stop and report the failure before proceeding.

## Execution

### Quick Start (Recommended)

```bash
tools/release_announcement/tests/run-release-announcement-baseline.sh
```

This script:
- Verifies all prerequisites
- Creates a timestamped output directory under `tools/release_announcement/tests/build/release-announcement-baseline-YYYYMMDD_HHMMSS/`
- Runs all 8 scenarios (4 scenarios × 2 backends)
- Captures timing, logs, diffs, and snapshots for each
- Cleans up git worktree after each scenario
- Reports high-level summary on completion

### Manual Execution (If Script Fails)

If the automated script fails, you can run individual scenarios by referencing the script's `run_case` function. Each scenario follows this pattern:

```bash
BASELINE_DIR="tools/release_announcement/tests/build/release-announcement-baseline-$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BASELINE_DIR"
BASE_COMMIT=$(git rev-parse HEAD)

# Capture initial state
cp ReleaseAnnouncement.md "$BASELINE_DIR/ReleaseAnnouncement.initial.md"
printf '%s\n' "$BASE_COMMIT" > "$BASELINE_DIR/base_commit.txt"

# Example: ollama backend, PR #3429
./tools/update-release-announcement.sh --delay-secs 0 --backend ollama --file ReleaseAnnouncement.md pr3429

# Capture outputs
cp ReleaseAnnouncement.md "$BASELINE_DIR/ollama__pr3429/ReleaseAnnouncement.after.md"
git diff -- ReleaseAnnouncement.md > "$BASELINE_DIR/ollama__pr3429/git-diff.txt"

# Reset for next scenario
git reset --hard "$BASE_COMMIT"
```

## Baseline Matrix Scenarios

Run all 8 combinations below. Each scenario is abbreviated as `{backend}__` followed by a label:

| # | Backend | Scenario | Arguments | Type | Expected Role |
|---|---------|----------|-----------|------|----------------|
| 1 | ollama | Normal PR | `pr3429` | single PR | Medium discussion, normal flow |
| 2 | github | Normal PR | `pr3429` | single PR | Medium discussion, normal flow |
| 3 | ollama | SKIP PR | `pr3625` | single PR | CHANGELOG: SKIP directive, should skip processing |
| 4 | github | SKIP PR | `pr3625` | single PR | CHANGELOG: SKIP directive, should skip processing |
| 5 | ollama | Large PR | `pr3502` | single PR | Large thread (182 comments), stress test for token trimming |
| 6 | github | Large PR | `pr3502` | single PR | Large thread (182 comments), stress test for embeddings/trimming |
| 7 | ollama | Tag Range | `r3_12_0beta4 r3_12_0beta5 --dry-run` | backfill | Multiple PRs, dry-run mode (no commits) |
| 8 | github | Tag Range | `r3_12_0beta4 r3_12_0beta5 --dry-run` | backfill | Multiple PRs, dry-run mode (no commits) |

## Expected Output Structure

Each scenario generates a directory named `{backend}__{label}` containing:

- `metadata.txt` — backend, label, PR/tag args, dry-run flag
- `command.txt` — exact shell command executed
- `timing.txt` — start/end ISO UTC timestamps (microsecond precision), duration (ns and ms), exit code
- `stdout-stderr.log` — full stdout and stderr from the tool
- `ReleaseAnnouncement.after.md` — announcement file snapshot after run
- `git-diff.txt` — git diff output (shows changes to ReleaseAnnouncement.md)
- `git-status-short.txt` — git status after run (should be empty if cleanup worked)
- `exit_code.txt` — single integer exit code
- `reset.log` — git reset output used to clean up between scenarios
- `_artifact-index.txt` — sorted list of all files in the baseline directory (generated at end)

### Timing File Format (Example)

```
start_iso_utc_us=2026-04-04T10:56:52.835529Z
end_iso_utc_us=2026-04-04T10:57:06.029304Z
start_epoch_ns=1775300212832604746
end_epoch_ns=1775300226027601118
duration_ns=13194996372
duration_ms=13194
exit_code=0
```

## Success Criteria

A baseline run is **successful** if all 8 scenarios complete without hanging or crashing, and end with an exit code report. Specifically:

1. ✅ All 8 scenario directories created under `$BASELINE_DIR`
2. ✅ Each has `timing.txt` with valid start/end timestamps and exit_code
3. ✅ At least one scenario per backend runs with `exit_code=0`
4. ✅ `ReleaseAnnouncement.initial.md` == `ReleaseAnnouncement` before any run
5. ✅ Final git worktree is clean: `git status --short` shows only untracked files (ini-*, tools/release_announcement/, etc.)

### Known Issues & Tolerances

- **Large PR timeouts:** Ollama backend on PR #3502 may take > 4 minutes. If it times out or fails, that exit code and timing are still captured in `timing.txt` and `stdout-stderr.log`.
- **SKIP PR handling:** PR #3625 should be skipped (no changes to announcement). Check `git-diff.txt` to verify no output was written.
- **Dry-run semantics:** Tag-range runs use `--dry-run` and should not create git commits. Verify with `reset.log` and empty `git-status-short.txt`.

## Post-Run Inspection

### View timing summary

```bash
BASELINE_DIR=".vscode/release-announcement-baseline-20260404_161752"
for d in "$BASELINE_DIR"/*__*; do
  name=$(basename "$d")
  timing="$d/timing.txt"
  if [ -f "$timing" ]; then
    dur=$(grep '^duration_ms=' "$timing" | cut -d= -f2)
    code=$(grep '^exit_code=' "$timing" | cut -d= -f2)
    printf '%s\tduration_ms=%s\texit_code=%s\n' "$name" "$dur" "$code"
  fi
done
```

### Check for differences

```bash
BASELINE_DIR=".vscode/release-announcement-baseline-20260404_161752"
# Show scenarios with non-zero exit codes
for d in "$BASELINE_DIR"/*__*; do
  code=$(cat "$d/exit_code.txt" 2>/dev/null || echo "missing")
  if [ "$code" != "0" ]; then
    echo "$(basename "$d") exited with code: $code"
    head -20 "$d/stdout-stderr.log"
  fi
done
```

### Save baseline reference

```bash
# Link the latest baseline as "reference"
LATEST=$(ls -dtd .vscode/release-announcement-baseline-* | head -1)
ln -sfn "$(basename "$LATEST")" .vscode/release-announcement-baseline-reference
echo "Baseline reference updated: .vscode/release-announcement-baseline-reference"
```

## Baseline Timing Reference (Current)

If previous baseline runs succeeded, their timings are recorded here for comparison:

### Captured on 2026-04-04 at 15:17–15:21 (Complete, All 8 Scenarios)

| Scenario | Duration (ms) | Duration (sec) | Exit Code |
|----------|---:|---:|---:|
| ollama/pr3429 | 14638 | 14.6s | ✅ 0 |
| github/pr3429 | 7445 | 7.4s | ✅ 0 |
| ollama/pr3625 | 4238 | 4.2s | ✅ 0 |
| github/pr3625 | 3935 | 3.9s | ✅ 0 |
| ollama/pr3502 | 128914 | 128.9s | ✅ 0 |
| github/pr3502 | 15546 | 15.5s | ✅ 0 |
| ollama/tag-range | 6755 | 6.8s | ✅ 0 |
| github/tag-range | 6301 | 6.3s | ✅ 0 |

**Total baseline duration:** ~3 min 30 sec across all 8 scenarios

**Key observations:**
- ✅ **All scenarios now complete with exit code 0** (ollama/pr3502 now succeeds; previous run had exit code 1)
- Ollama large-thread (pr3502) consistently takes 2+ minutes (~129 sec); GitHub completes in ~15 sec
- GitHub handles all scenarios within 7–15 seconds
- Ollama normal/medium cases: 4–14 seconds
- Tag-range dry-run scenarios complete in ~6–7 seconds for both backends
- Previous run had ollama/pr3502 with exit code 1; fresh run shows all exit code 0 — baseline is clean

## For AI Agents: How to Rerun

1. **Check prerequisites:**
   ```bash
   git status --short  # Must show only untracked: ini-*, tools/release_announcement/
   test -f .vscode/run-release-announcement-baseline.sh
   ```

2. **Execute the baseline:**
   ```bash
   .vscode/run-release-announcement-baseline.sh
   ```

3. **Wait for completion** (typically 10–15 minutes for all 8 runs).

4. **Verify success:**
   ```bash
   # Check final worktree status
   git status --short

   # Count artifacts
   LATEST=$(ls -dtd .vscode/release-announcement-baseline-* | head -1)
   find "$LATEST" -maxdepth 2 -type f | wc -l
   # Should be > 70 files (8 scenarios × ~9 files + index + snapshots)
   ```

5. **Report summary:**
   ```bash
   LATEST=$(ls -dtd .vscode/release-announcement-baseline-* | head -1)
   echo "Baseline: $LATEST"
   for d in "$LATEST"/*__*; do
     name=$(basename "$d")
     code=$(cat "$d/exit_code.txt")
     dur=$(grep '^duration_ms=' "$d/timing.txt" | cut -d= -f2)
     echo "$name: exit=$code, duration_ms=$dur"
   done
   ```

## Troubleshooting

| Issue | Resolution |
|-------|-----------|
| Script exits with "Error: tools/update-release-announcement.sh not found" | Verify you are at repo root: `pwd` should be `/home/peter/git/Jamulus-wip` |
| Script exits with "git worktree has uncommitted changes" | Run `git status` to see uncommitted files; stash or discard as needed |
| A scenario hangs (no output for > 600s) | Interrupt with Ctrl+C; scenario will be marked incomplete in the summary |
| Large PR (pr3502) takes > 5 minutes | Normal for Ollama; GitHub typically completes in ~14s. Both durations are valid baseline data |
| Ollama/GitHub completes normally but tool returns exit code 1 | Baseline is still valid; exit code 1 is captured in `timing.txt`. This may be a known issue to investigate during Step 1 |

## Related Files

- **Script:** [.vscode/run-release-announcement-baseline.sh](.vscode/run-release-announcement-baseline.sh)
- **Plan:** [.vscode/plan-releaseAnnouncementDistillation.prompt.md](.vscode/plan-releaseAnnouncementDistillation.prompt.md) — see "Capture a baseline before any changes are made" section
- **Archived baselines:** All runs under `.vscode/release-announcement-baseline-*/` are timestamped and preserved for audit

## Summary

The baseline matrix establishes ground truth for the release-announcement tool's current behavior in both backends across representative scenarios. Successful execution of this baseline is the prerequisite for all subsequent Step 1–10 verification work.
