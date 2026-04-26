"""Integration test for the Step 2 stub pipeline against baseline output."""

from __future__ import annotations

import os
import shutil
import subprocess
from datetime import UTC, datetime
from pathlib import Path

import pytest

from tests.cli_invocation import build_release_announcement_cmd


REPO_ROOT = Path(__file__).resolve().parents[3]
BASELINE_DIR = REPO_ROOT / ".vscode" / "release-announcement-baseline"


class Step2RunContext:
    """Runtime context for Step 2 integration command execution."""

    def __init__(
        self,
        backend: str,
        pr_ref: str,
        py_env: dict[str, str],
        run_root: Path,
    ):
        self.backend = backend
        self.pr_ref = pr_ref
        self.py_env = py_env
        self.run_root = run_root


def _git_output(*args: str) -> str:
    return subprocess.check_output(["git", *args], cwd=REPO_ROOT, text=True).strip()


def _run_command(command: list[str], log_file: Path, env: dict[str, str]) -> int:
    with log_file.open("w", encoding="utf-8") as handle:
        proc = subprocess.run(
            command,
            cwd=REPO_ROOT,
            stdout=handle,
            stderr=subprocess.STDOUT,
            check=False,
            env=env,
            text=True,
        )
    return proc.returncode


def _resolve_github_token_from_gh() -> str | None:
    """Return token from gh CLI auth context when available."""
    try:
        raw = subprocess.check_output(
            ["gh", "auth", "token"],
            cwd=REPO_ROOT,
            text=True,
            stderr=subprocess.STDOUT,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None

    token = raw.strip()
    return token or None


def _prepare_step2_environment() -> dict[str, object]:
    backend = os.getenv("RA_STEP2_BACKEND", "github")
    pr_ref = os.getenv("RA_STEP2_PR", "pr3429")
    strict_output = os.getenv("RA_STRICT_OUTPUT", "0") == "1"

    resolved_token = os.getenv("GH_TOKEN") or os.getenv("GITHUB_TOKEN")
    if not resolved_token:
        resolved_token = _resolve_github_token_from_gh()

    baseline_case_dir = BASELINE_DIR / f"{backend}__{pr_ref}"
    baseline_after = baseline_case_dir / "ReleaseAnnouncement.after.md"
    base_commit = _git_output("rev-parse", "HEAD")
    run_root = (
        REPO_ROOT
        / "build"
        / "release_announcement"
        / "step2_stub"
        / datetime.now(UTC).strftime("%Y%m%d_%H%M%S")
    )
    run_root.mkdir(parents=True, exist_ok=True)

    py_env = os.environ.copy()
    src_path = REPO_ROOT / "tools" / "release_announcement" / "src"
    py_env["PYTHONPATH"] = f"{src_path}:{py_env.get('PYTHONPATH', '')}"
    if resolved_token:
        py_env["GH_TOKEN"] = resolved_token
        py_env["GITHUB_TOKEN"] = resolved_token

    return {
        "backend": backend,
        "pr_ref": pr_ref,
        "strict_output": strict_output,
        "resolved_token": resolved_token,
        "baseline_case_dir": baseline_case_dir,
        "baseline_after": baseline_after,
        "base_commit": base_commit,
        "run_root": run_root,
        "py_env": py_env,
    }


def _assert_step2_prereqs(env: dict[str, object]) -> None:
    backend = str(env["backend"])
    resolved_token = env["resolved_token"]
    baseline_after = env["baseline_after"]

    if backend in {"github", "actions"} and not resolved_token:
        pytest.skip(
            "GitHub backend requires GH_TOKEN/GITHUB_TOKEN or an authenticated "
            "`gh auth token` context"
        )

    tracked_dirty = subprocess.run(
        ["git", "diff", "--quiet", "--exit-code"], cwd=REPO_ROOT, check=False
    )
    if tracked_dirty.returncode != 0:
        pytest.skip("Tracked files are dirty; integration test requires clean tracked worktree")

    if isinstance(baseline_after, Path) and not baseline_after.exists():
        pytest.skip(f"Missing baseline artifact: {baseline_after}")


def _run_pipeline_case(
    context: Step2RunContext,
    pipeline: str,
    base_commit: str,
) -> tuple[int, str, str, bool]:
    label = f"{context.backend}_{context.pr_ref}_{pipeline}"
    case_dir = context.run_root / label
    case_dir.mkdir(parents=True, exist_ok=True)

    cmd = build_release_announcement_cmd(
        backend=context.backend,
        pipeline=pipeline,
        start=context.pr_ref,
    )
    code = _run_command(cmd, case_dir / "stdout-stderr.log", context.py_env)

    after_text = (REPO_ROOT / "ReleaseAnnouncement.md").read_text(encoding="utf-8")
    (case_dir / "ReleaseAnnouncement.after.md").write_text(after_text, encoding="utf-8")
    diff_text = subprocess.check_output(
        ["git", "--no-pager", "diff", "--", "ReleaseAnnouncement.md"],
        cwd=REPO_ROOT,
        text=True,
    )
    (case_dir / "git-diff.txt").write_text(diff_text, encoding="utf-8")

    subprocess.run(["git", "reset", "--hard", base_commit], cwd=REPO_ROOT, check=True)
    changed = bool(diff_text.strip())
    return (
        code,
        after_text,
        (case_dir / "stdout-stderr.log").read_text(encoding="utf-8"),
        changed,
    )


@pytest.mark.integration
def test_step2_stub_legacy_and_staged_match_baseline_for_one_real_pr() -> None:
    """Compare staged and legacy Step 2 output against one saved real-PR baseline."""
    if os.getenv("RA_RUN_STEP2_E2E") != "1":
        pytest.skip("Set RA_RUN_STEP2_E2E=1 to run networked Step 2 integration checks")

    env = _prepare_step2_environment()
    _assert_step2_prereqs(env)

    try:
        context = Step2RunContext(
            backend=str(env["backend"]),
            pr_ref=str(env["pr_ref"]),
            py_env=env["py_env"],
            run_root=env["run_root"],
        )
        legacy_code, legacy_after, legacy_log, legacy_changed = _run_pipeline_case(
            context=context,
            pipeline="legacy",
            base_commit=str(env["base_commit"]),
        )
        staged_code, staged_after, staged_log, staged_changed = _run_pipeline_case(
            context=context,
            pipeline="staged",
            base_commit=str(env["base_commit"]),
        )

        assert legacy_code == 0
        assert staged_code == 0

        baseline_diff_text = (env["baseline_case_dir"] / "git-diff.txt").read_text(
            encoding="utf-8"
        )
        baseline_changed = bool(baseline_diff_text.strip())

        # Default verification is behavioral parity rather than byte-identity.
        assert legacy_changed == baseline_changed
        assert staged_changed == baseline_changed
        assert staged_changed == legacy_changed

        if env["strict_output"]:
            baseline_text = env["baseline_after"].read_text(encoding="utf-8")
            assert legacy_after == baseline_text
            assert staged_after == legacy_after

        assert "staged.chunking.start" in staged_log
        assert "staged.extraction.start" in staged_log
        assert "staged.consolidation.start" in staged_log
        assert "staged.classification.start" in staged_log
        assert (
            "staged preprocessing returned no context, falling back to legacy mode"
            in staged_log
        )
        assert "staged.chunking.start" not in legacy_log
    finally:
        subprocess.run(
            ["git", "reset", "--hard", str(env["base_commit"])],
            cwd=REPO_ROOT,
            check=False,
        )
        if (REPO_ROOT / "ReleaseAnnouncement.md").exists() and not env["baseline_after"].exists():
            shutil.copy2(REPO_ROOT / "ReleaseAnnouncement.md",
                         env["run_root"] / "ReleaseAnnouncement.last.md")
