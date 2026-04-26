"""Dummy backend adapter for testing the staged distillation pipeline.

Implements the DistillationAdapter protocol with hardcoded returns and no LLM calls.
Used for Step 4 verification and unit testing of the pipeline orchestration.
Registers under the "dummy" backend name (which has no prior registration).
"""

import sys
import time

from src.release_announcement.distillation import (
    build_distilled_context,
    default_fallback_signal,
    Chunk,
    Signal,
    ClassifiedSignal,
    ClassifiedSignals,
    DistilledContextMetadata,
    DistilledContext,
)
from src.release_announcement.registry import registry

try:
    from src.release_announcement.main import logger
except ImportError:
    class DummyLogger:
        def trace(self, msg):
            """Write trace output to stdout when the real logger is unavailable."""
            print(msg, file=sys.stdout)
    logger = DummyLogger()


def _system_prompt_preview(prompts: list[dict[str, str]], limit: int = 80) -> str:
    """Return a stable preview of the first system prompt in a prompt list."""
    for prompt in prompts:
        if isinstance(prompt, dict) and prompt.get("role") == "system":
            content = str(prompt.get("content", "")).strip().replace("\n", " ")
            return content[:limit]
    return ""


class DummyBackend:
    """Dummy adapter implementing DistillationAdapter protocol for testing."""

    def select_relevant_chunks(
        self,
        chunks: list[Chunk],
        use_embeddings: bool,
        ranking_prompts: list[dict[str, str]],
    ) -> list[Chunk]:
        """Return all chunks with deterministic descending relevance scores."""
        logger.trace("DummyBackend.select_relevant_chunks start")
        logger.trace(
            "DummyBackend.select_relevant_chunks "
            f"use_embeddings={use_embeddings}"
        )
        logger.trace(
            "DummyBackend.select_relevant_chunks "
            "ranking_prompts="
            f"{len(ranking_prompts)}"
        )
        start = time.perf_counter()
        result = []
        for i, chunk in enumerate(chunks):
            scored_chunk = Chunk(
                text=chunk.text,
                source=chunk.source,
                relevance_score=1.0 - (i * 0.05),  # Decay from 1.0
                chunk_index=chunk.chunk_index,
            )
            result.append(scored_chunk)
        elapsed = (time.perf_counter() - start) * 1000
        logger.trace(f"DummyBackend.select_relevant_chunks end elapsed_ms={elapsed:.2f}")
        return result

    def extract_chunk_signals(
        self,
        chunk: Chunk,
        extraction_prompts: list[dict[str, str]],
    ) -> list[Signal]:
        """Return a single synthetic signal derived from the input chunk."""
        logger.trace("DummyBackend.extract_chunk_signals start")
        logger.trace(
            "DummyBackend.extract_chunk_signals "
            f"extraction_prompts={len(extraction_prompts)}"
        )
        logger.info(
            "DummyBackend.extract_chunk_signals "
            f"system_prompt_preview={_system_prompt_preview(extraction_prompts)!r}"
        )
        start = time.perf_counter()
        result = [default_fallback_signal(chunk.source)]
        elapsed = (time.perf_counter() - start) * 1000
        logger.trace(f"DummyBackend.extract_chunk_signals end elapsed_ms={elapsed:.2f}")
        return result

    def consolidate_signals(
        self,
        signals: list[Signal],
        consolidation_prompts: list[dict[str, str]],
    ) -> list[Signal]:
        """Return the input signals unchanged for consolidation tests."""
        logger.trace("DummyBackend.consolidate_signals start")
        logger.trace(
            "DummyBackend.consolidate_signals "
            f"consolidation_prompts={len(consolidation_prompts)}"
        )
        logger.info(
            "DummyBackend.consolidate_signals "
            f"system_prompt_preview={_system_prompt_preview(consolidation_prompts)!r}"
        )
        logger.trace(f"DummyBackend.consolidate_signals signals_count={len(signals)}")
        start = time.perf_counter()
        result = signals
        elapsed = (time.perf_counter() - start) * 1000
        logger.trace(f"DummyBackend.consolidate_signals end elapsed_ms={elapsed:.2f}")
        return result

    def classify_signals(
        self,
        signals: list[Signal],
        classification_prompts: list[dict[str, str]],
    ) -> ClassifiedSignals:
        """Classify every signal as minor and build a synthetic summary."""
        logger.trace("DummyBackend.classify_signals start")
        logger.trace(
            "DummyBackend.classify_signals "
            f"classification_prompts={len(classification_prompts)}"
        )
        logger.info(
            "DummyBackend.classify_signals "
            f"system_prompt_preview={_system_prompt_preview(classification_prompts)!r}"
        )
        logger.trace(f"DummyBackend.classify_signals signals_count={len(signals)}")
        start = time.perf_counter()
        classified = [
            ClassifiedSignal(signal=signal, category="minor")
            for signal in signals
        ]
        result = ClassifiedSignals(
            classified=classified,
            summary=f"Dummy classification: {len(signals)} signals classified as minor",
        )
        elapsed = (time.perf_counter() - start) * 1000
        logger.trace(f"DummyBackend.classify_signals end elapsed_ms={elapsed:.2f}")
        return result

    def render_final_context(
        self,
        classified: ClassifiedSignals,
        metadata: DistilledContextMetadata,
    ) -> DistilledContext:
        """Convert classified signals into the final distilled context structure."""
        logger.trace("DummyBackend.render_final_context start")
        start = time.perf_counter()
        result = build_distilled_context(classified=classified, metadata=metadata)
        elapsed = (time.perf_counter() - start) * 1000
        logger.trace(f"DummyBackend.render_final_context end elapsed_ms={elapsed:.2f}")
        return result

    def get_adapter_tag(self) -> str:
        """Return the adapter identifier for logging."""
        return "dummy"


# Register the dummy backend at import time using the same lazy factory
# registration pattern as production backends.
def _create_dummy_adapter():
    """Factory function for creating a dummy backend."""
    return DummyBackend()


_DUMMY_REGISTRATION_STATE = {"registered": False}


def register_dummy_backend():
    """Register the dummy backend in the registry."""
    if _DUMMY_REGISTRATION_STATE["registered"]:
        return
    registry.register("dummy", _create_dummy_adapter)
    _DUMMY_REGISTRATION_STATE["registered"] = True


# Auto-register on import
register_dummy_backend()
