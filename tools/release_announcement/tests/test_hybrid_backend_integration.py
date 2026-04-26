"""Integration test for hybrid backend routing in staged preprocessing."""

from release_announcement import main as ra_main


class _TagOnlyAdapter:
    """Minimal adapter stub that exposes only backend identity for routing tests."""

    def __init__(self, tag: str, observed: dict[str, str]) -> None:
        self._tag = tag
        self._observed = observed

    def select_relevant_chunks(self, _chunks, use_embeddings, _ranking_prompts):
        bucket = "embedding" if use_embeddings else "chat"
        self._observed[bucket] = self._tag
        return []

    def get_adapter_tag(self) -> str:
        return self._tag


def test_hybrid_backend_pipeline(monkeypatch) -> None:
    """Wire different chat/embedding backends and assert staged routing preserves both."""
    observed = {"chat": "", "embedding": ""}

    def _stub_registry_get(name: str):
        if name in {"ollama", "github"}:
            return _TagOnlyAdapter(name, observed)
        return None

    def _stub_run_distillation_pipeline(*, adapter, **_kwargs):
        adapter.select_relevant_chunks([], True, [])
        adapter.select_relevant_chunks([], False, [])
        return ra_main.DistilledContext(
            summary="ok", structured_signals=[], classification={"classified": []}, metadata={}
        )

    monkeypatch.setattr(ra_main.registry, "get", _stub_registry_get)
    monkeypatch.setattr(ra_main, "run_distillation_pipeline", _stub_run_distillation_pipeline)

    context = ra_main.prepare_pr_context(
        pr_data={"number": 123, "title": "Hybrid backend integration"},
        backend_config=ra_main.BackendConfig(
            backend="ollama",
            chat_model_backend="ollama",
            embedding_model_backend="github",
            pipeline_mode="staged",
        ),
        pipeline_mode="staged",
    )

    assert context is not None
    assert observed == {"chat": "ollama", "embedding": "github"}
