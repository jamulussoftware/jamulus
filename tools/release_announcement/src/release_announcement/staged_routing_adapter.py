"""Routing adapter for staged distillation with split chat/embedding backends."""

from __future__ import annotations

from .distillation import (
    Chunk,
    ClassifiedSignals,
    DistillationAdapter,
    DistilledContext,
    DistilledContextMetadata,
    Signal,
)


class StagedRoutingAdapter(DistillationAdapter):
    """Route staged calls across chat and embedding backends.

    Per plan, ``--backend`` only provides defaults; staged execution routes
    chat-like phases via ``chat_model_backend`` and embedding ranking via
    ``embedding_model_backend`` when embeddings are enabled.
    """

    def __init__(
        self,
        *,
        chat_adapter: DistillationAdapter,
        embedding_adapter: DistillationAdapter,
    ) -> None:
        self._chat_adapter = chat_adapter
        self._embedding_adapter = embedding_adapter

    def select_relevant_chunks(
        self,
        chunks: list[Chunk],
        use_embeddings: bool,
        ranking_prompts: list[dict[str, str]],
    ) -> list[Chunk]:
        if use_embeddings:
            return self._embedding_adapter.select_relevant_chunks(
                chunks,
                use_embeddings,
                ranking_prompts,
            )
        return self._chat_adapter.select_relevant_chunks(
            chunks,
            use_embeddings,
            ranking_prompts,
        )

    def extract_chunk_signals(
        self,
        chunk: Chunk,
        extraction_prompts: list[dict[str, str]],
    ) -> list[Signal]:
        return self._chat_adapter.extract_chunk_signals(chunk, extraction_prompts)

    def consolidate_signals(
        self,
        signals: list[Signal],
        consolidation_prompts: list[dict[str, str]],
    ) -> list[Signal]:
        return self._chat_adapter.consolidate_signals(signals, consolidation_prompts)

    def classify_signals(
        self,
        signals: list[Signal],
        classification_prompts: list[dict[str, str]],
    ) -> ClassifiedSignals:
        return self._chat_adapter.classify_signals(signals, classification_prompts)

    def render_final_context(
        self,
        classified: ClassifiedSignals,
        metadata: DistilledContextMetadata,
    ) -> DistilledContext:
        return self._chat_adapter.render_final_context(classified, metadata)

    def get_chat_adapter_tag(self) -> str:
        """Return the chat adapter tag for staged route-aware logging."""
        return self._chat_adapter.get_adapter_tag()

    def get_embedding_adapter_tag(self) -> str:
        """Return the embedding adapter tag for staged route-aware logging."""
        return self._embedding_adapter.get_adapter_tag()

    def get_stage_adapter_tag(self, *, phase: str, use_embeddings: bool = False) -> str:
        """Return backend tag for a given phase and route mode."""
        if phase == "relevance_selection" and use_embeddings:
            return self.get_embedding_adapter_tag()
        return self.get_chat_adapter_tag()

    def get_adapter_tag(self) -> str:
        """Return the chat adapter's tag (staged routing is transparent)."""
        return self._chat_adapter.get_adapter_tag()
