"""Unit tests for staged-distillation pipeline orchestration (Substep 4b).

Tests the helper functions: _ordered_chunk, _select_relevant_chunks_with_fallback,
and _consolidate_signals_hierarchical without requiring a full adapter integration.
"""

from unittest.mock import MagicMock, call

import pytest

from src.release_announcement.distillation import (
    _ordered_chunk,
    _select_relevant_chunks_with_fallback,
    _consolidate_signals_hierarchical,
    run_distillation_pipeline,
    Chunk,
    Signal,
    ClassifiedSignal,
    ClassifiedSignals,
    DistillationPrompts,
    DistillationAdapter,
)


def _mk_signal(
    change: str,
    impact: str,
    users_affected: str,
    confidence: str,
    final_outcome: bool,
) -> Signal:
    return Signal(
        change=change,
        impact=impact,
        users_affected=users_affected,
        confidence=confidence,
        final_outcome=final_outcome,
    )


# ============================================================================
# Tests for _ordered_chunk
# ============================================================================


def test_ordered_chunk_with_pr_body_and_comments():
    """Test chunking converts PR body and comments to ordered chunks."""
    pr_data = {
        "number": 100,
        "title": "Fix bug in audio handling",
        "body": "This PR fixes issue #42.",
        "comments": [
            {"body": "First comment", "text": None},
            {"body": "Second comment", "text": None},
        ],
        "reviews": [],
    }

    chunks = _ordered_chunk(pr_data)

    assert len(chunks) == 3
    assert chunks[0].chunk_index == 0
    assert chunks[1].chunk_index == 1
    assert chunks[2].chunk_index == 2

    # First chunk should be title + body
    assert "Fix bug in audio handling" in chunks[0].text
    assert "This PR fixes issue #42." in chunks[0].text
    assert chunks[0].source == "pr_100_body"

    # Remaining chunks should be comments
    assert chunks[1].text == "First comment"
    assert chunks[1].source == "pr_100_comment_0"
    assert chunks[2].text == "Second comment"
    assert chunks[2].source == "pr_100_comment_1"


def test_ordered_chunk_preserves_order():
    """Test that chunks preserve PR discussion order."""
    pr_data = {
        "number": 42,
        "title": "Title",
        "body": "Body",
        "comments": [
            {"body": f"Comment {i}", "text": None} for i in range(10)
        ],
        "reviews": [],
    }

    chunks = _ordered_chunk(pr_data)

    # Verify chunk_index is sequential
    for i, chunk in enumerate(chunks):
        assert chunk.chunk_index == i


def test_ordered_chunk_skips_empty_comments():
    """Test that empty/None comments are skipped."""
    pr_data = {
        "number": 50,
        "title": "Title",
        "body": "Body",
        "comments": [
            {"body": "Real comment"},
            {"body": None, "text": None},
            {"body": ""},
            {"body": "Another real comment"},
        ],
        "reviews": [],
    }

    chunks = _ordered_chunk(pr_data)

    # Should have: body + 2 real comments
    assert len(chunks) == 3
    assert chunks[1].text == "Real comment"
    assert chunks[2].text == "Another real comment"


def test_ordered_chunk_missing_title():
    """Test chunking when PR has no title."""
    pr_data = {
        "number": 77,
        "title": "",
        "body": "Just a body",
        "comments": [{"body": "A comment"}],
        "reviews": [],
    }

    chunks = _ordered_chunk(pr_data)

    assert len(chunks) == 2
    assert "Just a body" in chunks[0].text


# ============================================================================
# Tests for _select_relevant_chunks_with_fallback
# ============================================================================


def test_select_relevant_chunks_success_with_embeddings():
    """Test successful selection when adapter returns ranked chunks."""
    chunks = [
        Chunk(text="Chunk 0", chunk_index=0),
        Chunk(text="Chunk 1", chunk_index=1),
        Chunk(text="Chunk 2", chunk_index=2),
    ]

    adapter = MagicMock(spec=DistillationAdapter)
    # Adapter returns chunks reordered by relevance but preserving indices
    adapter.select_relevant_chunks.return_value = [
        Chunk(text="Chunk 2", chunk_index=2, relevance_score=0.9),
        Chunk(text="Chunk 0", chunk_index=0, relevance_score=0.7),
        Chunk(text="Chunk 1", chunk_index=1, relevance_score=0.5),
    ]

    result = _select_relevant_chunks_with_fallback(
        chunks,
        adapter,
        use_embeddings=True,
        ranking_prompts=[{"role": "user", "content": "dummy"}],
    )

    assert len(result) == 3
    # Adapter was called with chunks, use_embeddings flag, and ranking_prompt
    adapter.select_relevant_chunks.assert_called_once_with(
        chunks,
        True,
        [{"role": "user", "content": "dummy"}],
    )


def test_select_relevant_chunks_fallback_on_adapter_failure():
    """Test positional fallback when adapter raises an exception."""
    chunks = [
        Chunk(text="Chunk 0", chunk_index=0),
        Chunk(text="Chunk 1", chunk_index=1),
        Chunk(text="Chunk 2", chunk_index=2),
        Chunk(text="Chunk 3", chunk_index=3),
        Chunk(text="Chunk 4", chunk_index=4),
    ]

    adapter = MagicMock(spec=DistillationAdapter)
    adapter.select_relevant_chunks.side_effect = RuntimeError("Ranking failed")

    result = _select_relevant_chunks_with_fallback(
        chunks,
        adapter,
        use_embeddings=False,
        ranking_prompts=[{"role": "user", "content": "dummy"}],
        last_n_fallback_chunks=2,
    )

    # Fallback should keep: first chunk (0) + last N (3, 4)
    assert len(result) == 3
    assert result[0].chunk_index == 0
    assert result[1].chunk_index == 3
    assert result[2].chunk_index == 4


def test_select_relevant_chunks_fallback_with_keywords():
    """Test positional fallback includes chunks with maintainer keywords."""
    chunks = [
        Chunk(text="Chunk 0", chunk_index=0),
        Chunk(text="Discussed but no decision", chunk_index=1),
        Chunk(text="We agreed on this approach", chunk_index=2),
        Chunk(text="Some implementation detail", chunk_index=3),
        Chunk(text="This was merged and deployed", chunk_index=4),
    ]

    adapter = MagicMock(spec=DistillationAdapter)
    adapter.select_relevant_chunks.side_effect = RuntimeError("Ranking failed")

    result = _select_relevant_chunks_with_fallback(
        chunks,
        adapter,
        use_embeddings=False,
        ranking_prompts=[{"role": "user", "content": "dummy"}],
        last_n_fallback_chunks=1,
        maintainer_keywords=["agreed", "merged"],
    )

    # Should keep: 0 (first) + 2 (agreed) + 4 (merged) + 4 (last N=1)
    indices = {c.chunk_index for c in result}
    assert 0 in indices  # First chunk
    assert 2 in indices  # Contains "agreed"
    assert 4 in indices  # Contains "merged" and is last
    # Result should be in discussion order
    result_indices = [c.chunk_index for c in result]
    assert result_indices == sorted(result_indices)


def test_select_relevant_chunks_preserves_discussion_order():
    """Test that fallback preserves original discussion order."""
    chunks = [
        Chunk(text="A", chunk_index=0),
        Chunk(text="B", chunk_index=1),
        Chunk(text="C", chunk_index=2),
        Chunk(text="D", chunk_index=3),
    ]

    adapter = MagicMock(spec=DistillationAdapter)
    adapter.select_relevant_chunks.side_effect = RuntimeError("Fail")

    result = _select_relevant_chunks_with_fallback(
        chunks,
        adapter,
        use_embeddings=False,
        ranking_prompts=[{"role": "user", "content": "dummy"}],
        last_n_fallback_chunks=1,
    )

    # Verify order is preserved
    result_indices = [c.chunk_index for c in result]
    assert result_indices == sorted(result_indices)


def test_select_relevant_chunks_batches_large_chat_ranking_sets():
    """Test that chat-based ranking batches large chunk sets."""
    chunks = [
        Chunk(text=f"Chunk {index}", chunk_index=index)
        for index in range(5)
    ]

    adapter = MagicMock(spec=DistillationAdapter)
    adapter.select_relevant_chunks.side_effect = [
        [
            Chunk(text="Chunk 1", chunk_index=1, relevance_score=0.8),
            Chunk(text="Chunk 0", chunk_index=0, relevance_score=0.9),
        ],
        [
            Chunk(text="Chunk 3", chunk_index=3, relevance_score=0.7),
            Chunk(text="Chunk 2", chunk_index=2, relevance_score=0.85),
        ],
        [
            Chunk(text="Chunk 4", chunk_index=4, relevance_score=0.95),
        ],
    ]

    result = _select_relevant_chunks_with_fallback(
        chunks,
        adapter,
        use_embeddings=False,
        ranking_prompts=[{"role": "user", "content": "dummy"}],
        max_ranking_chunks=2,
    )

    assert adapter.select_relevant_chunks.call_count == 3
    assert adapter.select_relevant_chunks.call_args_list == [
        call(chunks[0:2], False, [{"role": "user", "content": "dummy"}]),
        call(chunks[2:4], False, [{"role": "user", "content": "dummy"}]),
        call(chunks[4:5], False, [{"role": "user", "content": "dummy"}]),
    ]
    assert [chunk.chunk_index for chunk in result] == [0, 1, 2, 3, 4]


def test_select_relevant_chunks_does_not_batch_embeddings():
    """Test that embeddings-based ranking still uses a single adapter call."""
    chunks = [
        Chunk(text=f"Chunk {index}", chunk_index=index)
        for index in range(4)
    ]

    adapter = MagicMock(spec=DistillationAdapter)
    adapter.select_relevant_chunks.return_value = chunks

    result = _select_relevant_chunks_with_fallback(
        chunks,
        adapter,
        use_embeddings=True,
        ranking_prompts=[{"role": "user", "content": "dummy"}],
        max_ranking_chunks=2,
    )

    adapter.select_relevant_chunks.assert_called_once_with(
        chunks,
        True,
        [{"role": "user", "content": "dummy"}],
    )
    assert result == chunks


# ============================================================================
# Tests for _consolidate_signals_hierarchical
# ============================================================================


def test_consolidate_signals_direct_small_batch():
    """Test that small signal sets (<=threshold) use direct consolidation."""
    signals = [
        Signal(
            change="Fix A",
            impact="high",
            users_affected="all",
            confidence="high",
            final_outcome=True,
        ),
        _mk_signal("Fix B", "low", "some", "medium", False),
    ]

    adapter = MagicMock(spec=DistillationAdapter)
    adapter.consolidate_signals.return_value = signals

    result = _consolidate_signals_hierarchical(
        signals,
        adapter,
        [{"role": "system", "content": "prompt"}],
        max_direct_consolidation_chunks=20,
    )

    # Should be called exactly once (direct consolidation)
    adapter.consolidate_signals.assert_called_once()
    assert result == signals


def test_consolidate_signals_batched_large_set():
    """Test that large signal sets (>threshold) use batch consolidation."""
    # Create 25 signals (exceeds default threshold of 20)
    signals = [
        Signal(
            change=f"Change {i}",
            impact="low",
            users_affected="none",
            confidence="low",
            final_outcome=False,
        )
        for i in range(25)
    ]

    adapter = MagicMock(spec=DistillationAdapter)
    # First call: consolidate first batch of 20
    batch1_result = signals[:15]  # Consolidation reduces 20 to 15
    # Second call: consolidate second batch of 5
    batch2_result = signals[20:23]  # Consolidation reduces 5 to 3
    # Combined: 15 + 3 = 18 signals (below threshold, no recursion)

    adapter.consolidate_signals.side_effect = [
        batch1_result,  # First batch (20 → 15)
        batch2_result,  # Second batch (5 → 3)
    ]

    result = _consolidate_signals_hierarchical(
        signals,
        adapter,
        [{"role": "system", "content": "prompt"}],
        max_direct_consolidation_chunks=20,
    )

    # Should be called 2 times: 2 batches (combined result is 18 < 20, no recursion)
    assert adapter.consolidate_signals.call_count == 2
    # Result should be combined batch results
    assert len(result) == 18


def test_consolidate_signals_recursive_batching():
    """Test that recursive batching occurs when batch results exceed threshold."""
    # Create 60 signals
    signals = [
        Signal(
            change=f"Change {i}",
            impact="low",
            users_affected="none",
            confidence="low",
            final_outcome=False,
        )
        for i in range(60)
    ]

    adapter = MagicMock(spec=DistillationAdapter)

    # Simulate a scenario where batch consolidation results still exceed threshold:
    # 60 signals → 3 batches of 20 → each reduces by half → 30 total → exceeds threshold → recurse
    # 30 signals → 2 batches of 15 (only 2 batches needed) → each reduces to 8 → 16 total → fits
    batch_1_result = signals[0:10]   # 20 → 10
    batch_2_result = signals[20:30]  # 20 → 10
    batch_3_result = signals[40:50]  # 20 → 10
    # Batch results combined: 30 signals (exceeds 20) → triggers recursion
    recursive_batch_1 = signals[0:8]   # 15 → 8
    recursive_batch_2 = signals[25:33]  # 15 → 8

    adapter.consolidate_signals.side_effect = [
        batch_1_result,
        batch_2_result,
        batch_3_result,
        recursive_batch_1,
        recursive_batch_2,
    ]

    result = _consolidate_signals_hierarchical(
        signals,
        adapter,
        [{"role": "system", "content": "prompt"}],
        max_direct_consolidation_chunks=20,
    )

    # Should be called at least 4 times (3 initial batches + 2 recursive batches)
    # The exact count depends on internal recursion depth
    assert adapter.consolidate_signals.call_count >= 4
    # Verify it returns a valid result
    assert isinstance(result, list)


def test_consolidate_signals_empty_list():
    """Test that empty signal list returns immediately."""
    adapter = MagicMock(spec=DistillationAdapter)

    result = _consolidate_signals_hierarchical(
        [],
        adapter,
        [{"role": "system", "content": "prompt"}],
    )

    assert result == []
    adapter.consolidate_signals.assert_not_called()


def test_consolidate_signals_error_propagation():
    """Test that consolidation errors propagate with context."""
    signals = [
        _mk_signal("Fix", "low", "none", "low", False)
        for _ in range(25)
    ]

    adapter = MagicMock(spec=DistillationAdapter)
    adapter.consolidate_signals.side_effect = ValueError("Invalid signal format")

    with pytest.raises(ValueError, match="Invalid signal format"):
        _consolidate_signals_hierarchical(
            signals,
            adapter,
            [{"role": "system", "content": "prompt"}],
            max_direct_consolidation_chunks=20,
        )


# ============================================================================
# Integration Tests
# ============================================================================


def test_ordered_chunk_to_fallback_selection_integration():
    """Integration test: chunking → fallback selection on failure."""
    pr_data = {
        "number": 200,
        "title": "Feature X",
        "body": "Implement feature X",
        "comments": [
            {"body": "Review comment 1"},
            {"body": "We agreed to proceed"},
            {"body": "Implementation detail"},
            {"body": "This was merged successfully"},
        ],
        "reviews": [],
    }

    chunks = _ordered_chunk(pr_data)
    assert len(chunks) == 5  # body + 4 comments

    adapter = MagicMock(spec=DistillationAdapter)
    adapter.select_relevant_chunks.side_effect = RuntimeError("Adapter down")

    selected = _select_relevant_chunks_with_fallback(
        chunks,
        adapter,
        use_embeddings=False,
        ranking_prompts=[{"role": "user", "content": "dummy"}],
        max_ranking_chunks=30,
        last_n_fallback_chunks=2,
        maintainer_keywords=["agreed", "merged"],
    )

    # Should include: body (0) + "agreed" (2) + "merged" (4) + last 2 (3, 4)
    indices = {c.chunk_index for c in selected}
    assert 0 in indices
    assert 2 in indices  # agreed
    assert 4 in indices  # merged


def test_run_distillation_pipeline_routes_phase_prompts_to_matching_adapter_calls():
    """Verify each phase receives the correct prompt bundle in pipeline order."""
    pr_data = {
        "number": 314,
        "title": "Prompt routing",
        "body": "Body content",
        "comments": [{"body": "Timeline comment"}],
        "reviews": [],
    }

    ranking_prompts = [{"role": "user", "content": "RANKING_PROMPT_MARKER {chunk_count} {chunks}"}]
    extraction_prompts = [{"role": "system", "content": "EXTRACTION_PROMPT_MARKER"}]
    consolidation_prompts = [{"role": "system", "content": "CONSOLIDATION_PROMPT_MARKER"}]
    classification_prompts = [{"role": "system", "content": "CLASSIFICATION_PROMPT_MARKER"}]

    prompts = DistillationPrompts(
        extraction=extraction_prompts,
        consolidation=consolidation_prompts,
        classification=classification_prompts,
        ranking=ranking_prompts,
    )

    selected_chunk = Chunk(text="selected", source="pr_314_comment_0", chunk_index=1)
    extracted_signal = _mk_signal(
        "User-visible change",
        "medium",
        "operators",
        "high",
        True,
    )
    consolidated_signal = _mk_signal(
        "Consolidated change",
        "medium",
        "operators",
        "high",
        True,
    )

    adapter = MagicMock(spec=DistillationAdapter)
    adapter.select_relevant_chunks.return_value = [selected_chunk]
    adapter.extract_chunk_signals.return_value = [extracted_signal]
    adapter.consolidate_signals.return_value = [consolidated_signal]
    adapter.classify_signals.return_value = ClassifiedSignals(
        classified=[
            ClassifiedSignal(
                signal=consolidated_signal,
                category="minor",
            )
        ],
        summary="classified",
    )
    adapter.render_final_context.return_value = {"summary": "ok"}

    class _Capabilities:
        supports_embeddings = False

    class _BackendConfig:
        capabilities = _Capabilities()

    result = run_distillation_pipeline(
        pr_data=pr_data,
        adapter=adapter,
        backend_config=_BackendConfig(),
        prompts=prompts,
    )

    assert result == {"summary": "ok"}

    adapter.select_relevant_chunks.assert_called_once()
    select_args = adapter.select_relevant_chunks.call_args.args
    assert select_args[2] is ranking_prompts

    adapter.extract_chunk_signals.assert_called_once_with(selected_chunk, extraction_prompts)
    adapter.consolidate_signals.assert_called_once_with([extracted_signal], consolidation_prompts)
    adapter.classify_signals.assert_called_once_with([consolidated_signal], classification_prompts)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
