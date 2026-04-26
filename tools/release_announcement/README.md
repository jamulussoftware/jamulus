# release_announcement

Python package for generating and updating `ReleaseAnnouncement.md` from merged PRs.

## Entry Point

Run from the repository root after installing the package:

```bash
python -m release_announcement START [END] --file ReleaseAnnouncement.md [options]
```

Examples:

```bash
python -m release_announcement HEAD~20 HEAD --file ReleaseAnnouncement.md --backend ollama --dry-run
python -m release_announcement pr3500 --file ReleaseAnnouncement.md --backend actions
```

## CLI Flags

- Positional: `start` (required), `end` (optional)
- `--file` (required): Markdown file to update.
- `--prompt`: Prompt template file path. Default: `tools/release_announcement/prompts/release-announcement.prompt.yml`.
- `--chat-model`, `--model`: Chat model override.
- `--embedding-model`, `--embed`: Embedding model override (GitHub/actions backends).
- `--backend`: One of `ollama`, `github`, `actions`.
- `--delay-secs`: Delay before each PR is processed.
- `--dry-run`: Discover and report work without calling the LLM.
- `--pipeline`: One of `legacy` or `staged` (default).
  - `legacy`: Existing raw-PR prompt path.
  - `staged`: Step 2 stubbed preprocessing path; currently logs stub stages and falls back to legacy.

## Install

From repository root:

```bash
python3 -m pip install ./tools/release_announcement
```

For development/test dependencies:

```bash
python3 -m pip install -e ./tools/release_announcement[dev]
```

## Test Suite

Use a virtual environment under `/tmp`:

```bash
cd tools/release_announcement
rm -rf /tmp/release_announcement-test-venv
python3 -m venv /tmp/release_announcement-test-venv
. /tmp/release_announcement-test-venv/bin/activate
python -m pip install -e .[dev]
```

Run the default test suite (fast, no network):

```bash
python -m pytest
```

Expected report for the default run:

- Unit tests should pass.
- Integration/regression tests are expected to be skipped unless explicitly enabled.
- Typical output shape is similar to `6 passed, 2 skipped`.

Run only unit tests:

```bash
python -m pytest -m "not integration"
```

Optional integration checks:

```bash
# Step 2 real-PR legacy vs staged-stub parity check (writes artifacts under build/)
RA_RUN_STEP2_E2E=1 python -m pytest tests/test_step2_stub_integration.py -m integration

# Full regression matrix against baseline artifacts (writes artifacts under build/)
RA_RUN_MATRIX=1 python -m pytest tests/test_regression_matrix.py -m "integration and regression"
```

Expected report for integration/regression runs:

- Tests run only when the required environment variable is set.
- If prerequisites are missing (tokens, clean worktree, baseline artifacts), pytest reports these tests as skipped with a reason.
- If enabled and prerequisites are satisfied, these tests compare output against baseline artifacts and fail on mismatches.

### Troubleshooting Test Runs

- `No module named pytest`:
  - Ensure the `/tmp` venv is activated.
  - Reinstall dev dependencies: `python -m pip install -e .[dev]`.

- Integration tests are skipped unexpectedly:
  - Confirm required env vars are set:
    - `RA_RUN_STEP2_E2E=1` for Step 2 real-PR parity check.
    - `RA_RUN_MATRIX=1` for full matrix regression.
  - For GitHub backend checks, either set `GH_TOKEN`/`GITHUB_TOKEN` or ensure `gh auth token` works.

- Regression test skipped for missing baseline:
  - Ensure baseline artifacts exist under `tests/build/release-announcement-baseline/`.
  - Or point to a different baseline with `RA_BASELINE_DIR=/path/to/baseline`.

- Integration/regression tests skip because worktree is not clean:
  - Commit/stash tracked changes first.
  - Untracked files are allowed by the current checks.

- Matrix regression fails with output mismatches:
  - Compare generated artifacts under `tests/build/`.
  - Strict markdown byte-identity checks are opt-in. Set `RA_STRICT_OUTPUT=1` to enforce exact output matching.

## Baseline Matrix

The release-announcement tool includes a baseline matrix runner for establishing reference outputs before restructuring:

```bash
tests/run-release-announcement-baseline.sh
```

This captures the tool's current behavior against 4 representative PR scenarios with both Ollama and GitHub backends. Output is stored in `tests/build/release-announcement-baseline-YYYYMMDD_HHMMSS/`. See [tests/BASELINE_MATRIX_README.md](tests/BASELINE_MATRIX_README.md) for detailed guidance.

## Integration Test Artifacts

Integration and regression tests write artifacts to `tests/build/`, which is git-ignored. This includes:

- Baseline matrix run outputs
- Step 2 real-PR parity check snapshots
- Regression matrix comparison results
- Temporary test run logs


