"""Shared assertion helpers for backend adapter protocol tests."""

from __future__ import annotations

from collections.abc import Sequence
from typing import Any

from release_announcement.cli_config import BackendCapabilities


def assert_distillation_adapter_surface(adapter: object) -> None:
    """Assert that an adapter exposes the required distillation methods."""
    required_methods = [
        "select_relevant_chunks",
        "extract_chunk_signals",
        "consolidate_signals",
        "classify_signals",
        "render_final_context",
    ]
    for method_name in required_methods:
        assert hasattr(adapter, method_name)


def patch_cli_startup_happy_path(
    monkeypatch: Any,
    ra_main: Any,
    prs: Sequence[dict[str, Any]],
) -> None:
    """Patch startup/probing/discovery steps to deterministic no-network behavior."""
    monkeypatch.setattr(ra_main, "_setup_backend_token", lambda _backend: None)
    monkeypatch.setattr(
        ra_main,
        "probe_capabilities",
        lambda _config: BackendCapabilities(
            supports_chat=True,
            supports_embeddings=False,
        ),
    )
    monkeypatch.setattr(ra_main, "validate_mode", lambda _config: None)
    monkeypatch.setattr(
        ra_main,
        "_resolve_timeframe",
        lambda _args: (
            "pr3502~",
            "pr3502",
            "2026-01-01T00:00:00+00:00",
            "2026-01-02T00:00:00+00:00",
        ),
    )
    monkeypatch.setattr(ra_main, "get_ordered_pr_list", lambda _start, _end: list(prs))
