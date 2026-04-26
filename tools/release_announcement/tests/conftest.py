"""Pytest guardrails for backend usage in the release_announcement test suite."""

from __future__ import annotations

import os

import pytest


@pytest.fixture(autouse=True)
def prevent_real_backend_calls(
    monkeypatch: pytest.MonkeyPatch,
    request: pytest.FixtureRequest,
) -> None:
    """Block only real network calls outside the final full integration test."""
    is_integration = request.node.get_closest_marker("integration") is not None
    is_final_full_integration = (
        is_integration
        and os.getenv("RA_RUN_STEP2_E2E") == "1"
        and "test_step2_stub_integration.py" in request.node.nodeid
    )
    if is_final_full_integration:
        return

    def _blocked(*_args, **_kwargs):
        raise RuntimeError(
            "Real backend call blocked in test. Mock backend calls in unit/regression tests; "
            "only the final full integration test may call real providers."
        )

    # Block GitHub HTTP calls at urllib boundary.
    monkeypatch.setattr(
        "src.release_announcement.backends.github_backend.urllib.request.urlopen",
        _blocked,
    )
    monkeypatch.setattr(
        "src.release_announcement.backends.distillation_adapters.github_adapter."
        "urllib.request.urlopen",
        _blocked,
    )

    # Block Ollama HTTP calls at the transport boundary used by ollama's client.
    monkeypatch.setattr("httpx.Client.request", _blocked)
