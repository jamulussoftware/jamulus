"""Test delay functionality: sleep fires only when the LLM path is taken."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import patch

import pytest

from release_announcement import main as ra_main
from release_announcement.cli_config import BackendConfig


def _stub_pr_data() -> dict:
    return {
        "number": 3502,
        "title": "Test PR",
        "body": "Some change",
        "comments": [],
        "reviews": [],
    }


def _wire_llm_stubs(monkeypatch: pytest.MonkeyPatch) -> None:
    """Patch the minimum set of collaborators so process_single_pr reaches the sleep point."""
    monkeypatch.setattr(ra_main, "_fetch_pr_data", lambda *_a, **_k: _stub_pr_data())
    monkeypatch.setattr(ra_main, "prepare_pr_context", lambda *_a, **_k: None)
    monkeypatch.setattr(ra_main, "_load_announcement_content", lambda *_a: "existing")
    monkeypatch.setattr(ra_main, "_load_prompt_template", lambda *_a: {})
    monkeypatch.setattr(ra_main, "_build_ai_prompt", lambda *_a, **_k: {"messages": []})
    monkeypatch.setattr(ra_main, "_process_with_llm", lambda *_a, **_k: "updated")
    monkeypatch.setattr(ra_main, "_write_and_check_announcement", lambda *_a, **_k: None)
    monkeypatch.setattr(ra_main, "_check_for_changes", lambda *_a, **_k: "no_changes")


def test_delay_fires_before_llm_when_nonzero(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Sleep is called with the configured delay when the LLM path is taken."""
    ann_file = tmp_path / "ReleaseAnnouncement.md"
    ann_file.write_text("existing", encoding="utf-8")
    _wire_llm_stubs(monkeypatch)

    config = BackendConfig(backend="ollama", pipeline_mode="legacy", delay_secs=5)

    with patch("release_announcement.main.time.sleep") as mock_sleep:
        ra_main.process_single_pr(
            pr_num=3502,
            pr_title="Test PR",
            announcement_file=str(ann_file),
            prompt_file="unused.yml",
            config=config,
        )
        mock_sleep.assert_called_once_with(5)


def test_delay_skipped_when_zero(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """No sleep when delay_secs is zero."""
    ann_file = tmp_path / "ReleaseAnnouncement.md"
    ann_file.write_text("existing", encoding="utf-8")
    _wire_llm_stubs(monkeypatch)

    config = BackendConfig(backend="ollama", pipeline_mode="legacy", delay_secs=0)

    with patch("release_announcement.main.time.sleep") as mock_sleep:
        ra_main.process_single_pr(
            pr_num=3502,
            pr_title="Test PR",
            announcement_file=str(ann_file),
            prompt_file="unused.yml",
            config=config,
        )
        mock_sleep.assert_not_called()


def test_delay_skipped_for_dry_run(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """No sleep when dry-run is set, even with a non-zero delay."""
    ann_file = tmp_path / "ReleaseAnnouncement.md"
    ann_file.write_text("existing", encoding="utf-8")
    monkeypatch.setattr(ra_main, "_fetch_pr_data", lambda *_a, **_k: _stub_pr_data())

    config = BackendConfig(backend="ollama", pipeline_mode="legacy", dry_run=True, delay_secs=5)

    with patch("release_announcement.main.time.sleep") as mock_sleep:
        result = ra_main.process_single_pr(
            pr_num=3502,
            pr_title="Test PR",
            announcement_file=str(ann_file),
            prompt_file="unused.yml",
            config=config,
        )
        assert result == "dry_run"
        mock_sleep.assert_not_called()


def test_delay_skipped_for_skipped_pr(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """No sleep when the PR matches a skip rule."""
    ann_file = tmp_path / "ReleaseAnnouncement.md"
    ann_file.write_text("existing", encoding="utf-8")
    weblate_pr = {**_stub_pr_data(), "title": "Translations update from Hosted Weblate"}
    monkeypatch.setattr(ra_main, "_fetch_pr_data", lambda *_a, **_k: weblate_pr)

    config = BackendConfig(backend="ollama", pipeline_mode="legacy", delay_secs=5)

    with patch("release_announcement.main.time.sleep") as mock_sleep:
        result = ra_main.process_single_pr(
            pr_num=3502,
            pr_title=weblate_pr["title"],
            announcement_file=str(ann_file),
            prompt_file="unused.yml",
            config=config,
        )
        assert result.startswith("skipped:")
        mock_sleep.assert_not_called()
