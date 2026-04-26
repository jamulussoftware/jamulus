"""Staged-distillation pipeline: schemas, adapter protocol, and shared helpers.

This module defines the type foundation and adapter interface for the staged
PR-discussion distillation pipeline. It includes Pydantic models for signal
extraction and classification, the abstract backend adapter protocol, and
shared JSON parsing helpers that all adapters use after unwrapping their
provider-specific response envelopes.

No pipeline orchestration code is included in this module; orchestration
and phase sequencing are implemented in later substeps.
"""

from __future__ import annotations

import json
import time
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, Literal, Protocol, runtime_checkable

from pydantic import BaseModel, ConfigDict, Field
from .app_logger import logger

if TYPE_CHECKING:
    from .cli_config import BackendConfig


PARSE_EXCEPTIONS = (
    RuntimeError,
    ValueError,
    TypeError,
    KeyError,
    IndexError,
    json.JSONDecodeError,
)


# ============================================================================
# Pydantic Models (Signal Extraction and Classification)
# ============================================================================


class Signal(BaseModel):
    """Extracted user-visible signal from PR discussion.

    Machine-targeted JSON schema capturing the essential user-facing
    information from one section of the PR discussion thread.
    """

    change: str = Field(
        ...,
        description="User-visible change description",
    )
    impact: str = Field(
        ...,
        description="Severity or scope of the change (e.g., 'high', 'medium', 'low')",
    )
    users_affected: str = Field(
        ...,
        description="User groups affected (e.g., 'server operators', 'musicians')",
    )
    confidence: str = Field(
        ...,
        description="How certain the extraction is (e.g., 'high', 'medium', 'low')",
    )
    final_outcome: bool = Field(
        ...,
        description="Whether this reflects a final decision (True) or early speculation (False)",
    )

    model_config = ConfigDict(extra="allow")


class ClassifiedSignal(BaseModel):
    """Classified signal assigned to a release-note category.

    Result of the classification stage: a Signal with its assigned category.
    """

    signal: Signal = Field(
        ...,
        description="The original extracted signal",
    )
    category: Literal["internal", "minor", "targeted", "major", "no_user_facing_changes"] = Field(
        ...,
        description="Release-note category assignment",
    )

    model_config = ConfigDict(extra="allow")


class ClassifiedSignals(BaseModel):
    """Complete classification result for a set of signals.

    Returned by the classification stage; can be empty if no user-facing
    changes were identified.
    """

    classified: list[ClassifiedSignal] = Field(
        default_factory=list,
        description="List of classified signals, empty if no user-facing changes",
    )
    summary: str = Field(
        default="",
        description="Optional summary of the classification result",
    )

    model_config = ConfigDict(extra="allow")


# ============================================================================
# Data Classes (Chunking and Metadata)
# ============================================================================


@dataclass
class Chunk:
    """One ordered segment of PR discussion.

    Represents a contiguous section of the PR discussion with associated
    metadata for relevance tracking and source attribution.
    """

    text: str
    """The actual content text of this chunk."""

    source: str = "unknown"
    """Source identifier (e.g., 'pr_body', 'comment_123', 'review_456')."""

    relevance_score: float = 0.0
    """Relevance score assigned by ranking (embedding, chat, or positional)."""

    chunk_index: int = 0
    """Original discussion order index (preserved through selection)."""


@dataclass
class DistilledContextMetadata:
    """Metadata about the distilled context production."""

    pr_number: int = 0
    """PR number being processed."""

    total_chunks: int = 0
    """Total chunks produced before relevance selection."""

    selected_chunks: int = 0
    """Number of chunks selected after relevance filtering."""

    extraction_phase_duration_ms: float = 0.0
    """Time spent in extraction phase (milliseconds)."""

    consolidation_phase_duration_ms: float = 0.0
    """Time spent in consolidation phase (milliseconds)."""

    classification_phase_duration_ms: float = 0.0
    """Time spent in classification phase (milliseconds)."""


@dataclass(frozen=True)
class DistillationPrompts:
    """Prompt bundles for staged distillation phases."""

    extraction: list[dict[str, str]]
    consolidation: list[dict[str, str]]
    classification: list[dict[str, str]]
    ranking: list[dict[str, str]]


@dataclass(frozen=True)
class DistillationOptions:
    """Execution tuning options for staged distillation orchestration."""

    max_direct_consolidation_chunks: int = 20
    max_ranking_chunks: int = 30
    maintainer_keywords: list[str] | None = None
    last_n_fallback_chunks: int = 3
    """Time spent in classification phase (milliseconds)."""


@dataclass
class DistilledContext:
    """Structured staged-preprocessing output passed to prompt builders in later steps."""

    summary: str
    """Summary of key changes from the distillation pipeline."""

    structured_signals: list[dict[str, Any]]
    """Signals classified and ready for release note rendering."""

    classification: dict[str, Any]
    """Classification result with category breakdowns."""

    metadata: dict[str, Any]
    """Pipeline execution metadata (timing, chunk counts, etc.)."""


def default_fallback_signal(source: str) -> Signal:
    """Return a deterministic fallback signal when parsing fails."""
    return Signal(
        change=f"Change from {source[:30]}",
        impact="low",
        users_affected="some users",
        confidence="medium",
        final_outcome=False,
    )


def metadata_to_dict(metadata: DistilledContextMetadata) -> dict[str, Any]:
    """Convert metadata dataclass to stable dict representation."""
    return {
        "pr_number": metadata.pr_number,
        "total_chunks": metadata.total_chunks,
        "selected_chunks": metadata.selected_chunks,
        "extraction_phase_duration_ms": metadata.extraction_phase_duration_ms,
        "consolidation_phase_duration_ms": metadata.consolidation_phase_duration_ms,
        "classification_phase_duration_ms": metadata.classification_phase_duration_ms,
    }


def classified_signals_to_dicts(classified: ClassifiedSignals) -> list[dict[str, Any]]:
    """Flatten classified signal objects into rendering-friendly dictionaries."""
    signals_dicts = []
    for classified_signal in classified.classified:
        signal_dict = classified_signal.signal.model_dump()
        signal_dict["category"] = classified_signal.category
        signals_dicts.append(signal_dict)
    return signals_dicts


def build_distilled_context(
    classified: ClassifiedSignals,
    metadata: DistilledContextMetadata,
    classification_payload: dict[str, Any] | None = None,
) -> DistilledContext:
    """Build final DistilledContext payload with shared metadata formatting."""
    signals_dicts = classified_signals_to_dicts(classified)
    return DistilledContext(
        summary=classified.summary,
        structured_signals=signals_dicts,
        classification=classification_payload or classified.model_dump(),
        metadata=metadata_to_dict(metadata),
    )


# ============================================================================
# Shared Parsing Helpers
# ============================================================================


def _parse_signal_list(content: str) -> list[Signal]:
    """Parse and validate a list of Signal objects from LLM response content.

    The LLM is expected to produce JSON array of signals matching the
    Signal schema. This helper extracts the JSON, validates it against
    the Pydantic model, and propagates validation errors.

    Args:
        content: Raw response content string (may include preamble/postamble)

    Returns:
        List of validated Signal objects

    Raises:
        ValueError: If JSON array cannot be found or extracted
        pydantic.ValidationError: If JSON does not match Signal schema
    """
    # Try to extract JSON array from response, handling common LLM patterns.
    # First try: look for ```json...``` code block.
    if "```json" in content:
        start = content.index("```json") + len("```json")
        end = content.find("```", start)
        if end > start:
            json_text = content[start:end].strip()
        else:
            json_text = content
    elif "```" in content:
        # Fallback: generic code block
        start = content.index("```") + 3
        end = content.find("```", start)
        if end > start:
            json_text = content[start:end].strip()
        else:
            json_text = content
    else:
        json_text = content

    # Try to parse as JSON array
    try:
        data = json.loads(json_text)
    except json.JSONDecodeError as e:
        raise ValueError(
            f"Failed to parse response as JSON: {e}\n"
            f"Response text (first 200 chars): {content[:200]}"
        ) from e

    if not isinstance(data, list):
        raise ValueError(
            f"Expected JSON array at top level, got {type(data).__name__}"
        )

    # Validate each element against Signal schema
    signals = []
    for idx, item in enumerate(data):
        try:
            signals.append(Signal(**item))
        except Exception as e:
            raise ValueError(
                f"Signal validation failed at index {idx}: {e}\n"
                f"Item: {item}"
            ) from e

    return signals


def _parse_classified_signals(content: str) -> ClassifiedSignals:
    """Parse and validate ClassifiedSignals result from LLM response content.

    The LLM is expected to produce a JSON object matching the ClassifiedSignals
    schema, which contains a list of classified signals and optional summary.

    Args:
        content: Raw response content string (may include preamble/postamble)

    Returns:
        Validated ClassifiedSignals object

    Raises:
        ValueError: If JSON object cannot be found or extracted
        pydantic.ValidationError: If JSON does not match ClassifiedSignals schema
    """
    # Try to extract JSON object from response, handling common LLM patterns.
    if "```json" in content:
        start = content.index("```json") + len("```json")
        end = content.find("```", start)
        if end > start:
            json_text = content[start:end].strip()
        else:
            json_text = content
    elif "```" in content:
        # Fallback: generic code block
        start = content.index("```") + 3
        end = content.find("```", start)
        if end > start:
            json_text = content[start:end].strip()
        else:
            json_text = content
    else:
        json_text = content

    # Try to parse as JSON object
    try:
        data = json.loads(json_text)
    except json.JSONDecodeError as e:
        raise ValueError(
            f"Failed to parse response as JSON: {e}\n"
            f"Response text (first 200 chars): {content[:200]}"
        ) from e

    if not isinstance(data, dict):
        raise ValueError(
            f"Expected JSON object at top level, got {type(data).__name__}"
        )

    # Validate against ClassifiedSignals schema
    try:
        return ClassifiedSignals(**data)
    except Exception as e:
        raise ValueError(
            f"ClassifiedSignals validation failed: {e}\n"
            f"Data: {data}"
        ) from e


# ============================================================================
# Backend Adapter Protocol
# ============================================================================


@runtime_checkable
class DistillationAdapter(Protocol):
    """Abstract backend interface for staged distillation pipeline.

    All backend implementations (Ollama, GitHub, Dummy) must implement this
    protocol. The shared pipeline orchestration in distillation.py calls
    these five methods; adapters handle only provider-specific API details
    and envelope unwrapping. All phase sequencing, fallback logic, retry
    policy, and error propagation live in the shared pipeline.

    The shared pipeline checks `isinstance(adapter, DistillationAdapter)` at
    startup to catch missing methods immediately.
    """

    def select_relevant_chunks(
        self,
        chunks: list[Chunk],
        use_embeddings: bool,
        ranking_prompts: list[dict[str, str]],
    ) -> list[Chunk]:
        """Rank and filter chunks by relevance.

        Return chunks in original discussion order with relevance metadata
        populated. This is the provider's ranking call; the shared pipeline
        owns fallback sequencing and batching for large chunk sets.

        Args:
            chunks: Ordered list of discussion chunks to rank
            use_embeddings: Whether embeddings API is available for ranking.
                When True, use backend embeddings API; when False, use a
                single chat-based ranking call. The shared pipeline owns
            ranking_prompts: Prompt list for ranking phase
                batching and positional fallback if chat-ranking fails.

        Returns:
            Ranked subset of input chunks in original discussion order,
            with relevance_score populated. May include all or a filtered
            subset depending on backend ranking logic.

        Raises:
            Exception: Any provider failure; propagates to shared pipeline
                for fallback/retry logic.
        """

    def extract_chunk_signals(
        self,
        chunk: Chunk,
        extraction_prompts: list[dict[str, str]],
    ) -> list[Signal]:
        """Extract user-visible signals from one chunk.

        Send extraction prompt messages + chunk.text user message
        to provider. Extract response content, pass to _parse_signal_list,
        and return the validated signals.

        Args:
            chunk: Single discussion chunk to extract from
            extraction_prompts: Prompt list for extraction phase

        Returns:
            List of extracted Signal objects from this chunk

        Raises:
            ValueError: If response cannot be parsed as Signal JSON
            Exception: Any provider failure
        """

    def consolidate_signals(
        self,
        signals: list[Signal],
        consolidation_prompts: list[dict[str, str]],
    ) -> list[Signal]:
        """Consolidate and deduplicate extracted signals.

        Send consolidation prompt messages + json(signals) user message
        to provider. Extract response content, pass to _parse_signal_list,
        and return the consolidated signals.

        May prefer later final outcomes over earlier speculation based on
        the consolidation_prompt guidance.

        Args:
            signals: List of extracted signals to consolidate
            consolidation_prompts: Prompt list for consolidation phase

        Returns:
            Consolidated and deduplicated list of Signal objects

        Raises:
            ValueError: If response cannot be parsed as Signal JSON
            Exception: Any provider failure
        """

    def classify_signals(
        self,
        signals: list[Signal],
        classification_prompts: list[dict[str, str]],
    ) -> ClassifiedSignals:
        """Classify signals into release-note categories.

        Send classification prompt messages + json(signals) user message
        to provider. Extract response content, pass to _parse_classified_signals,
        and return the classified result.

        Each signal is assigned one of: internal, minor, targeted, major,
        or no_user_facing_changes. Empty result (no user-facing changes)
        is valid and must not be treated as failure.

        Args:
            signals: List of consolidated signals to classify
            classification_prompts: Prompt list for classification phase

        Returns:
            ClassifiedSignals object with signals assigned to categories

        Raises:
            ValueError: If response cannot be parsed as ClassifiedSignals JSON
            Exception: Any provider failure
        """

    def render_final_context(
        self,
        classified: ClassifiedSignals,
        metadata: DistilledContextMetadata,
    ) -> Any:
        """Assemble final DistilledContext from classification results.

        Combine classified signals, metadata, and any backend-specific
        context into the DistilledContext dataclass used by downstream
        prompt builders. No provider call is required.

        Args:
            classified: Classification stage result
            metadata: Metadata about distillation pipeline execution

        Returns:
            DistilledContext object ready for use in prompt building
        """

    def get_adapter_tag(self) -> str:
        """Return a short string tag identifying this adapter for logging.

        Returns:
            String tag (e.g., "ollama", "github", "dummy") used in phase logs
        """
# ============================================================================
# Pipeline Orchestration
# ============================================================================


def _ordered_chunk(pr_data: dict[str, Any]) -> list[Chunk]:
    """Convert PR discussion data into ordered discussion chunks.

    Creates an ordered sequence of chunks from PR body and comments,
    preserving the discussion order. Each chunk includes source attribution
    for diagnostics and signal tracing.

    Args:
        pr_data: PR data dict from _fetch_pr_data (contains 'number', 'title',
                'body', 'comments', 'reviews')

    Returns:
        List of Chunk objects in discussion order, indexed consecutively
    """
    chunks = []
    chunk_index = 0

    # First chunk: PR title and body
    title = pr_data.get("title", "")
    body = pr_data.get("body", "")
    pr_number = pr_data.get("number", 0)

    if title or body:
        combined = f"Title: {title}\n\n{body}" if title else body
        chunks.append(
            Chunk(
                text=combined.strip(),
                source=f"pr_{pr_number}_body",
                chunk_index=chunk_index,
            )
        )
        chunk_index += 1

    # Remaining chunks: comments (including inline review comments)
    comments = pr_data.get("comments", [])
    for comment_idx, comment in enumerate(comments):
        if isinstance(comment, dict):
            # Comment from timeline or inline review
            comment_text = comment.get("body") or comment.get("text") or ""
            if comment_text:
                chunks.append(
                    Chunk(
                        text=comment_text,
                        source=f"pr_{pr_number}_comment_{comment_idx}",
                        chunk_index=chunk_index,
                    )
                )
                chunk_index += 1

    return chunks


def _select_relevant_chunks_with_fallback(
    chunks: list[Chunk],
    adapter: DistillationAdapter,
    use_embeddings: bool,
    ranking_prompts: list[dict[str, str]],
    options: DistillationOptions | None = None,
    **legacy_kwargs: Any,
) -> list[Chunk]:
    """Rank and filter chunks by relevance with fallback chain.

    Attempts to rank chunks using embeddings (if available) or chat-ranking.
    On failure, falls back to positional selection (first, last N, and
    maintainer-keyword chunks).

    Args:
        chunks: Ordered list of chunks to rank
        adapter: Distillation adapter implementing the ranking call
        use_embeddings: Whether to try embeddings-based ranking
        ranking_prompts: Prompt list for chat-based ranking
        max_ranking_chunks: Max chunks per chat-ranking batch (default 30)
        maintainer_keywords: Keywords indicating maintainer decisions
        last_n_fallback_chunks: Number of trailing chunks to retain in fallback

    Returns:
        Ranked subset of chunks in original discussion order with
        relevance_score populated. Preserves chunk_index for ordering.
    """
    options = options or DistillationOptions()
    max_ranking_chunks = legacy_kwargs.get(
        "max_ranking_chunks",
        options.max_ranking_chunks,
    )
    maintainer_keywords = legacy_kwargs.get(
        "maintainer_keywords",
        options.maintainer_keywords,
    )
    last_n_fallback_chunks = legacy_kwargs.get(
        "last_n_fallback_chunks",
        options.last_n_fallback_chunks,
    )

    if maintainer_keywords is None:
        maintainer_keywords = ["merged", "agreed", "closing", "decided", "approved"]

    backend_tag = adapter.get_adapter_tag()
    if use_embeddings and callable(getattr(adapter, "get_embedding_adapter_tag", None)):
        backend_tag = str(getattr(adapter, "get_embedding_adapter_tag")())

    def _positional_fallback() -> list[Chunk]:
        """Select chunks positionally when ranking is unavailable."""
        fallback_start = time.perf_counter()
        logger.debug(
            "   staged.relevance_selection.fallback_to_positional.start "
            f"chunks={len(chunks)} route=[{'embed' if use_embeddings else 'chat'}:{backend_tag}] "
            f"last_n_fallback_chunks={last_n_fallback_chunks} "
            f"keyword_count={len(maintainer_keywords)}"
        )
        logger.info(
            "   staged.relevance_selection.fallback_to_positional "
            f"chunks={len(chunks)}"
        )

        selected = []
        selected_indices = set()

        if chunks:
            selected.append(chunks[0])
            selected_indices.add(0)

        start_last = max(0, len(chunks) - last_n_fallback_chunks)
        for i in range(start_last, len(chunks)):
            if i not in selected_indices:
                selected.append(chunks[i])
                selected_indices.add(i)

        keyword_lower = [kw.lower() for kw in maintainer_keywords]
        for i, chunk in enumerate(chunks):
            if i not in selected_indices and any(kw in chunk.text.lower() for kw in keyword_lower):
                selected.append(chunk)
                selected_indices.add(i)

        selected.sort(key=lambda chunk: chunk.chunk_index)
        logger.trace(
            "   staged.relevance_selection.fallback_to_positional.selected "
            f"chunk_indices={[chunk.chunk_index for chunk in selected]}"
        )
        logger.debug(
            "   staged.relevance_selection.fallback_to_positional.end "
            f"chunks={len(selected)} "
            f"elapsed_ms={(time.perf_counter() - fallback_start) * 1000:.2f}"
        )
        return selected

    try:
        # Embeddings-based ranking can score all chunks in one provider call.
        if use_embeddings or len(chunks) <= max_ranking_chunks:
            return adapter.select_relevant_chunks(chunks, use_embeddings, ranking_prompts)

        if max_ranking_chunks <= 0:
            raise ValueError("max_ranking_chunks must be greater than zero")

        logger.info(
            f"   staged.relevance_selection.batching chunks={len(chunks)} "
            f"batch_size={max_ranking_chunks}"
        )

        ranked_batches: list[Chunk] = []
        for batch_start in range(0, len(chunks), max_ranking_chunks):
            batch = chunks[batch_start: batch_start + max_ranking_chunks]
            ranked_batches.extend(
                adapter.select_relevant_chunks(batch, use_embeddings, ranking_prompts)
            )

        ranked_batches.sort(key=lambda chunk: chunk.chunk_index)
        return ranked_batches

    except PARSE_EXCEPTIONS as e:
        # Log the failure with context
        logger.warning(
            f"   staged.relevance_selection.failed use_embeddings={use_embeddings}: {e}"
        )
        return _positional_fallback()


def _extract_chunk_signals(
    chunks: list[Chunk],
    adapter: DistillationAdapter,
    extraction_prompts: list[dict[str, str]],
) -> list[Signal]:
    """Extract signals from all selected chunks.

    Calls adapter.extract_chunk_signals for each chunk and aggregates
    the results into a single signal list.

    Args:
        chunks: Selected chunks to process
        adapter: Distillation adapter implementing the extraction call
        extraction_prompts: Prompt list for extraction stage

    Returns:
        Aggregated list of extracted signals from all chunks

    Raises:
        Exception: If extraction fails for any chunk; includes chunk index in error
    """
    all_signals = []

    for chunk_idx, chunk in enumerate(chunks):
        try:
            signals = adapter.extract_chunk_signals(chunk, extraction_prompts)
            all_signals.extend(signals)
        except Exception as e:
            logger.error(
                f"   staged.extraction.chunk_failed chunk_index={chunk_idx} "
                f"source={chunk.source}: {e}"
            )
            raise

    return all_signals


def _consolidate_signals_hierarchical(
    signals: list[Signal],
    adapter: DistillationAdapter,
    consolidation_prompts: list[dict[str, str]],
    max_direct_consolidation_chunks: int = 20,
) -> list[Signal]:
    """Consolidate extracted signals with hierarchical batching for large sets.

    When the signal count exceeds max_direct_consolidation_chunks, runs
    consolidation in sequential batches and then consolidates the batch
    results (recursively if needed) before returning.

    Args:
        signals: Extracted signals to consolidate
        adapter: Distillation adapter implementing consolidation call
        consolidation_prompts: Prompt list for consolidation stage
        max_direct_consolidation_chunks: Threshold for batching (default 20)

    Returns:
        Consolidated and deduplicated signal list

    Raises:
        Exception: If consolidation fails at any batch; includes batch number in error
    """
    if len(signals) == 0:
        return []

    # Base case: small enough to consolidate directly
    if len(signals) <= max_direct_consolidation_chunks:
        return adapter.consolidate_signals(signals, consolidation_prompts)

    # Recursive case: batch consolidation
    logger.info(
        f"   staged.consolidation.batching signals={len(signals)} "
        f"batch_size={max_direct_consolidation_chunks}"
    )

    batch_results = []
    batch_num = 0

    # Process each batch
    for i in range(0, len(signals), max_direct_consolidation_chunks):
        batch = signals[i: i + max_direct_consolidation_chunks]
        batch_num += 1

        try:
            consolidated_batch = adapter.consolidate_signals(batch, consolidation_prompts)
            batch_results.extend(consolidated_batch)
        except Exception as e:
            logger.error(
                f"   staged.consolidation.batch_failed batch={batch_num} "
                f"batch_size={len(batch)}: {e}"
            )
            raise

    # Recursively consolidate batch results if still too large
    if len(batch_results) > max_direct_consolidation_chunks:
        return _consolidate_signals_hierarchical(
            batch_results,
            adapter,
            consolidation_prompts,
            max_direct_consolidation_chunks,
        )

    return batch_results


def _trace_signals(phase: str, signals: list[Signal]) -> None:
    """Log each signal at TRACE level after a pipeline phase."""
    for i, sig in enumerate(signals):
        logger.trace(
            f"   staged.{phase}.signal[{i}] "
            f"change={sig.change!r} impact={sig.impact} "
            f"confidence={sig.confidence} final_outcome={sig.final_outcome}"
        )


def _trace_classified(classified: ClassifiedSignals) -> None:
    """Log each classified signal at TRACE level after classification."""
    for i, cs in enumerate(classified.classified):
        logger.trace(
            f"   staged.classification.signal[{i}] "
            f"category={cs.category} change={cs.signal.change!r}"
        )
    if not classified.classified:
        logger.trace("   staged.classification.result no_user_facing_changes")


def run_distillation_pipeline(
    pr_data: dict[str, Any],
    adapter: DistillationAdapter,
    backend_config: BackendConfig,
    prompts: DistillationPrompts,
    options: DistillationOptions | None = None,
) -> Any:
    """Orchestrate the full staged distillation pipeline.

    Executes ordered chunking, relevance selection (with fallback chain),
    per-chunk extraction, hierarchical consolidation, and classification
    in sequence. Returns a DistilledContext suitable for prompt building.

    All errors from adapter calls include full request/response context
    for diagnostics. Phase logging includes timing and chunk counts.

    Args:
        pr_data: PR data structure from _fetch_pr_data
        adapter: Distillation adapter (must implement DistillationAdapter protocol)
        backend_config: Backend configuration with capabilities (supports_embeddings)
        prompts: Role-keyed prompt bundles for each pipeline phase.
        options: Execution tuning for ranking/consolidation fallback behavior.

    Returns:
        DistilledContext object ready for use by downstream prompt builders.
        Note: DistilledContext is imported from main.py by the caller.

    Raises:
        Exception: If any phase fails after fallback exhaustion
    """
    metadata = DistilledContextMetadata(pr_number=pr_data.get("number", 0))

    options = options or DistillationOptions()

    if not prompts.extraction:
        raise ValueError("Extraction prompts cannot be empty")
    if not prompts.consolidation:
        raise ValueError("Consolidation prompts cannot be empty")
    if not prompts.classification:
        raise ValueError("Classification prompts cannot be empty")
    if not prompts.ranking:
        raise ValueError("Ranking prompts cannot be empty")

    # Phase 1: Ordered chunking
    logger.debug("   staged.chunking.start")
    phase_start = time.perf_counter()

    chunks = _ordered_chunk(pr_data)
    metadata.total_chunks = len(chunks)

    logger.debug(
        "   staged.chunking.end "
        f"chunks={len(chunks)} elapsed_ms={(time.perf_counter() - phase_start) * 1000:.2f}"
    )

    # Phase 2: Relevance selection
    use_embeddings = getattr(backend_config, "capabilities", None) and getattr(
        backend_config.capabilities, "supports_embeddings", False
    )
    backend_tag = adapter.get_adapter_tag()
    if use_embeddings and callable(getattr(adapter, "get_embedding_adapter_tag", None)):
        backend_tag = str(getattr(adapter, "get_embedding_adapter_tag")())
    logger.debug(
        "   staged.relevance_selection.start "
        f"chunks={len(chunks)} "
        f"route=[{'embed' if use_embeddings else 'chat'}:{backend_tag}] "
        f"use_embeddings={bool(use_embeddings)}"
    )
    phase_start = time.perf_counter()

    selected_chunks = _select_relevant_chunks_with_fallback(
        chunks,
        adapter,
        use_embeddings,
        prompts.ranking,
        options,
    )
    metadata.selected_chunks = len(selected_chunks)

    logger.debug(
        f"   staged.relevance_selection.end chunks={len(selected_chunks)} "
        f"elapsed_ms={(time.perf_counter() - phase_start) * 1000:.2f}"
    )

    # Phase 3: Per-chunk extraction
    logger.debug(
        "   staged.extraction.start "
        f"chunks={len(selected_chunks)} route=[chat:{adapter.get_adapter_tag()}]"
    )
    phase_start = time.perf_counter()

    signals = _extract_chunk_signals(
        selected_chunks,
        adapter,
        prompts.extraction,
    )

    metadata.extraction_phase_duration_ms = (time.perf_counter() - phase_start) * 1000
    logger.debug(
        f"   staged.extraction.end signals={len(signals)} "
        f"elapsed_ms={metadata.extraction_phase_duration_ms:.2f}"
    )
    _trace_signals("extraction", signals)

    # Phase 4: Hierarchical consolidation
    logger.debug(
        "   staged.consolidation.start "
        f"signals={len(signals)} route=[chat:{adapter.get_adapter_tag()}]"
    )
    phase_start = time.perf_counter()

    signals = _consolidate_signals_hierarchical(
        signals,
        adapter,
        prompts.consolidation,
        options.max_direct_consolidation_chunks,
    )

    metadata.consolidation_phase_duration_ms = (time.perf_counter() - phase_start) * 1000
    logger.debug(
        f"   staged.consolidation.end signals={len(signals)} "
        f"elapsed_ms={metadata.consolidation_phase_duration_ms:.2f}"
    )
    _trace_signals("consolidation", signals)

    # Phase 5: Classification
    logger.debug(
        "   staged.classification.start "
        f"signals={len(signals)} route=[chat:{adapter.get_adapter_tag()}]"
    )
    phase_start = time.perf_counter()

    classified = adapter.classify_signals(
        signals,
        prompts.classification,
    )

    metadata.classification_phase_duration_ms = (time.perf_counter() - phase_start) * 1000
    logger.debug(
        f"   staged.classification.end signals={len(classified.classified)} "
        f"elapsed_ms={metadata.classification_phase_duration_ms:.2f}"
    )
    _trace_classified(classified)

    # Phase 6: Render final context
    logger.debug(
        "   staged.rendering.start "
        f"route=[chat:{adapter.get_adapter_tag()}]"
    )
    phase_start = time.perf_counter()

    distilled_context = adapter.render_final_context(classified, metadata)
    logger.debug(
        "   staged.rendering.end "
        f"elapsed_ms={(time.perf_counter() - phase_start) * 1000:.2f}"
    )

    return distilled_context
