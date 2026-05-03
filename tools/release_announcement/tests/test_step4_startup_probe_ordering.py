"""Step 4 regression tests for startup capability probe ordering.

These tests ensure runtime capability probing happens once at startup, before
any PR boundary/timeframe resolution work begins.
"""

from __future__ import annotations

import sys

import pytest

from release_announcement import main as ra_main
from release_announcement.cli_config import BackendCapabilities


def _set_argv(monkeypatch: pytest.MonkeyPatch, extra: list[str] | None = None) -> None:
    argv = [
        "release_announcement",
        "--file",
        "ReleaseAnnouncement.md",
        "--delay-secs",
        "0",
        "pr3502",
    ]
    if extra:
        argv[1:1] = extra
    monkeypatch.setattr(sys, "argv", argv)


def test_startup_probes_before_timeframe_resolution(monkeypatch: pytest.MonkeyPatch) -> None:
    """Run probe/validation before resolving PR boundaries."""
    call_order: list[str] = []

    _set_argv(monkeypatch)

    monkeypatch.setattr(ra_main, "_setup_backend_token", lambda _backend: None)

    def _probe(_config):
        call_order.append("probe")
        return BackendCapabilities(supports_chat=True, supports_embeddings=False)

    def _validate(_config):
        call_order.append("validate")

    def _resolve_timeframe(_args):
        call_order.append("timeframe")
        return (
            "pr3502~",
            "pr3502",
            "2026-01-01T00:00:00+00:00",
            "2026-01-02T00:00:00+00:00",
        )

    monkeypatch.setattr(ra_main, "probe_capabilities", _probe)
    monkeypatch.setattr(ra_main, "validate_mode", _validate)
    monkeypatch.setattr(ra_main, "_resolve_timeframe", _resolve_timeframe)
    monkeypatch.setattr(ra_main, "get_ordered_pr_list", lambda _start, _end: [])

    ra_main.main()

    assert call_order == ["probe", "validate", "timeframe"]


def test_probe_failure_exits_before_timeframe_resolution(monkeypatch: pytest.MonkeyPatch) -> None:
    """Exit immediately on probe failure, before PR resolution starts."""
    _set_argv(monkeypatch, ["--backend", "ollama", "--embed", "github"])

    monkeypatch.setattr(ra_main, "_setup_backend_token", lambda _backend: None)

    def _raise_probe(_config):
        raise RuntimeError("probe failed")

    timeframe_called = False

    def _resolve_timeframe(_args):
        nonlocal timeframe_called
        timeframe_called = True
        return (
            "pr3502~",
            "pr3502",
            "2026-01-01T00:00:00+00:00",
            "2026-01-02T00:00:00+00:00",
        )

    monkeypatch.setattr(ra_main, "probe_capabilities", _raise_probe)
    monkeypatch.setattr(ra_main, "_resolve_timeframe", _resolve_timeframe)

    with pytest.raises(SystemExit) as exc:
        ra_main.main()

    assert exc.value.code == 1
    assert timeframe_called is False
