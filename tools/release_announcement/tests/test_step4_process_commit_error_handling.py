"""Step 4 regression tests for commit-stage error handling and messaging."""

from __future__ import annotations

import argparse
import subprocess
import sys

import pytest

from release_announcement import main as ra_main
from release_announcement.cli_config import BackendConfig
from tests.assertions import patch_cli_startup_happy_path


def test_process_prs_reports_result_before_git_add_failure(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    """Print process outcome before staging and raise user-facing error on git failure."""
    args = argparse.Namespace(
        delay_secs=0,
        dry_run=False,
        file="/tmp/ra.md",
        prompt="unused.prompt.yml",
    )
    config = BackendConfig(backend="ollama")

    monkeypatch.setattr(ra_main, "process_single_pr", lambda *_args: "committed")

    def _fake_run(cmd, check, capture_output, text):
        del check, capture_output, text
        if cmd[:2] == ["git", "add"]:
            raise subprocess.CalledProcessError(
                128,
                cmd,
                stderr="fatal: /tmp/ra.md: '/tmp/ra.md' is outside repository",
            )
        return subprocess.CompletedProcess(cmd, 0, "", "")

    monkeypatch.setattr(subprocess, "run", _fake_run)

    process_prs = getattr(ra_main, "_process_prs")
    with pytest.raises(RuntimeError) as exc_info:
        process_prs(
            args,
            config,
            [{"number": 3502, "title": "Add MIDI GUI tab and learn function"}],
        )

    out = capsys.readouterr().out
    assert "Updated release announcement for #3502. Preparing git commit." in out
    msg = str(exc_info.value)
    assert "Could not stage or commit updates for PR #3502." in msg
    assert "Command failed: git add /tmp/ra.md." in msg
    assert "outside repository" in msg


def test_main_exits_cleanly_on_process_prs_runtime_error(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    """Main should surface processing errors as a single critical line and exit 1."""
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "release_announcement",
            "--delay-secs",
            "0",
            "--file",
            "ReleaseAnnouncement.md",
            "pr3502",
        ],
    )

    patch_cli_startup_happy_path(
        monkeypatch,
        ra_main,
        [{"number": 3502, "title": "Test PR"}],
    )
    monkeypatch.setattr(
        ra_main,
        "_process_prs",
        lambda _args, _config, _prs: (_ for _ in ()).throw(
            RuntimeError("Could not stage or commit updates for PR #3502.")
        ),
    )

    with pytest.raises(SystemExit) as exc_info:
        ra_main.main()

    assert exc_info.value.code == 1
    err = capsys.readouterr().err
    assert "Could not stage or commit updates for PR #3502." in err


def test_process_prs_wraps_process_single_pr_exceptions(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Convert per-PR exceptions into user-facing RuntimeError context."""
    args = argparse.Namespace(
        delay_secs=0,
        dry_run=False,
        file="ReleaseAnnouncement.md",
        prompt="unused.prompt.yml",
    )
    config = BackendConfig(backend="ollama")

    monkeypatch.setattr(
        ra_main,
        "process_single_pr",
        lambda *_args: (_ for _ in ()).throw(RuntimeError("boom")),
    )

    process_prs = getattr(ra_main, "_process_prs")
    with pytest.raises(RuntimeError) as exc_info:
        process_prs(args, config, [{"number": 3502, "title": "Test PR"}])

    assert "Failed while processing PR #3502 (Test PR): boom" in str(exc_info.value)


def test_main_exits_cleanly_on_unexpected_startup_error(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    """Unexpected startup exceptions should be reported as a single critical line."""
    monkeypatch.setattr(
        sys,
        "argv",
        [
            "release_announcement",
            "--delay-secs",
            "0",
            "--file",
            "ReleaseAnnouncement.md",
            "pr3502",
        ],
    )
    monkeypatch.setattr(
        ra_main,
        "_setup_backend_token",
        lambda _backend: (_ for _ in ()).throw(ValueError("token setup failed")),
    )

    with pytest.raises(SystemExit) as exc_info:
        ra_main.main()

    assert exc_info.value.code == 1
    err = capsys.readouterr().err
    assert "Unexpected startup error: token setup failed" in err
