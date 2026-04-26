"""Step 8 output parity tests using CLI entrypoint invocation."""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from release_announcement import main as ra_main
from tests import dummy_backend
from tests.assertions import patch_cli_startup_happy_path


def _invoke_main_with_args(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    pipeline: str,
) -> bytes:
    """Import modules, then execute CLI main with argv and return output bytes."""
    dummy_backend.register_dummy_backend()

    announcement_path = tmp_path / f"ReleaseAnnouncement.{pipeline}.md"
    announcement_path.write_text("seed\n", encoding="utf-8")

    prompt_path = tmp_path / "release-announcement.prompt.yml"
    prompt_path.write_text(
        "messages:\n"
        "  - role: system\n"
        "    content: test-system-prompt\n",
        encoding="utf-8",
    )

    sample_pr_data = {
        "number": 3429,
        "title": "Test PR",
        "body": "Test body",
        "comments": ["Comment 1", "Comment 2"],
        "reviews": [],
    }

    patch_cli_startup_happy_path(
        monkeypatch,
        ra_main,
        [{"number": 3429, "title": "Test PR"}],
    )
    monkeypatch.setattr(ra_main, "_fetch_pr_data", lambda _pr_num, _config: sample_pr_data)
    monkeypatch.setattr(
        ra_main,
        "_process_with_llm",
        lambda _ai_prompt, _config: "Generated release notes\n",
    )
    monkeypatch.setattr(ra_main, "_check_for_changes", lambda _path: "no_changes")

    argv = [
        "release_announcement",
        "--backend",
        "dummy",
        "--pipeline",
        pipeline,
        "--delay-secs",
        "0",
        "--file",
        str(announcement_path),
        "--prompt",
        str(prompt_path),
        "pr3429",
    ]
    monkeypatch.setattr(sys, "argv", argv)

    ra_main.main()
    return announcement_path.read_bytes()


def test_entrypoint_legacy_vs_staged_are_byte_identical(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    """Legacy and staged entrypoint runs should emit identical announcement bytes."""
    legacy_bytes = _invoke_main_with_args(monkeypatch, tmp_path, pipeline="legacy")
    staged_bytes = _invoke_main_with_args(monkeypatch, tmp_path, pipeline="staged")

    assert legacy_bytes == staged_bytes
