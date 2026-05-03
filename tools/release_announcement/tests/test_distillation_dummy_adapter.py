"""Unit tests for the dummy backend adapter (Substep 4c).

Tests verify that the dummy backend correctly implements the DistillationAdapter protocol
with hardcoded returns and no LLM calls, suitable for pipeline orchestration testing
and Step 4 verification.
"""
from tests.dummy_backend import DummyBackend, register_dummy_backend
from tests.test_data import sample_metadata_123

from src.release_announcement.distillation import (
    ClassifiedSignal,
    ClassifiedSignals,
    Chunk,
    DistilledContext,
    DistilledContextMetadata,
    Signal,
)


class TestDummyBackendInitialization:
    """Tests for dummy backend registration and instantiation."""

    def test_dummy_backend_instantiation(self):
        """Verify DummyBackend can be instantiated."""
        backend = DummyBackend()
        assert backend is not None
        assert hasattr(backend, "select_relevant_chunks")
        assert hasattr(backend, "extract_chunk_signals")
        assert hasattr(backend, "consolidate_signals")
        assert hasattr(backend, "classify_signals")
        assert hasattr(backend, "render_final_context")

    def test_dummy_backend_registration(self):
        """Verify register_dummy_backend() can be called without errors."""
        # Should not raise, even if called multiple times
        register_dummy_backend()
        register_dummy_backend()


class TestSelectRelevantChunks:
    """Tests for DummyBackend.select_relevant_chunks()."""

    def test_select_relevant_chunks_returns_all_inputs(self):
        """Verify all input chunks are returned."""
        backend = DummyBackend()
        chunks = [
            Chunk(
                text=f"Chunk {i}",
                source=f"comment_{i}",
                chunk_index=i,
            )
            for i in range(3)
        ]
        result = backend.select_relevant_chunks(
            chunks,
            use_embeddings=False,
            ranking_prompts=[{"role": "user", "content": "dummy ranking prompt"}],
        )
        assert len(result) == 3

    def test_select_relevant_chunks_assigns_decreasing_scores(self):
        """Verify chunks get decreasing relevance scores based on position."""
        backend = DummyBackend()
        chunks = [
            Chunk(text=f"Chunk {i}", source=f"comment_{i}", chunk_index=i)
            for i in range(3)
        ]
        result = backend.select_relevant_chunks(
            chunks,
            use_embeddings=False,
            ranking_prompts=[{"role": "user", "content": "dummy ranking prompt"}],
        )

        # Scores should decrease: 1.0, 0.95, 0.90
        assert result[0].relevance_score == 1.0
        assert result[1].relevance_score == 0.95
        assert result[2].relevance_score == 0.90

    def test_select_relevant_chunks_empty_list(self):
        """Verify empty chunk list returns empty result."""
        backend = DummyBackend()
        result = backend.select_relevant_chunks(
            [],
            use_embeddings=True,
            ranking_prompts=[{"role": "user", "content": "dummy ranking prompt"}],
        )
        assert not result

    def test_select_relevant_chunks_ignores_embeddings_flag(self):
        """Verify use_embeddings parameter is accepted but unused."""
        backend = DummyBackend()
        chunks = [Chunk(text="Test", source="test", chunk_index=0)]
        result_no_emb = backend.select_relevant_chunks(
            chunks,
            use_embeddings=False,
            ranking_prompts=[{"role": "user", "content": "dummy ranking prompt"}],
        )
        result_with_emb = backend.select_relevant_chunks(
            chunks,
            use_embeddings=True,
            ranking_prompts=[{"role": "user", "content": "dummy ranking prompt"}],
        )

        # Results should be identical regardless of embeddings flag
        assert result_no_emb[0].relevance_score == result_with_emb[0].relevance_score

    def test_select_relevant_chunks_preserves_chunk_content(self):
        """Verify chunk text and source are preserved."""
        backend = DummyBackend()
        original_chunks = [
            Chunk(text="Important discussion", source="title_section", chunk_index=0),
            Chunk(text="Detailed feedback", source="comment_5", chunk_index=1),
        ]
        result = backend.select_relevant_chunks(
            original_chunks,
            use_embeddings=False,
            ranking_prompts=[{"role": "user", "content": "dummy ranking prompt"}],
        )

        assert result[0].text == "Important discussion"
        assert result[0].source == "title_section"
        assert result[1].text == "Detailed feedback"
        assert result[1].source == "comment_5"


class TestExtractChunkSignals:
    """Tests for DummyBackend.extract_chunk_signals()."""

    def test_extract_chunk_signals_returns_single_signal(self):
        """Verify extraction returns exactly one signal per chunk."""
        backend = DummyBackend()
        chunk = Chunk(text="PR discussion", source="comment_1", chunk_index=0)
        result = backend.extract_chunk_signals(
            chunk,
            [{"role": "system", "content": "placeholder prompt"}],
        )

        assert len(result) == 1
        assert isinstance(result[0], Signal)

    def test_extract_chunk_signals_signal_has_valid_fields(self):
        """Verify extracted signal has all required fields with valid values."""
        backend = DummyBackend()
        chunk = Chunk(text="Performance improvement", source="comment_3", chunk_index=0)
        signals = backend.extract_chunk_signals(
            chunk,
            [{"role": "system", "content": "placeholder"}],
        )

        signal = signals[0]
        assert signal.change is not None
        assert signal.impact in ["low", "medium", "high"]
        assert signal.users_affected is not None
        assert signal.confidence in ["low", "medium", "high"]
        assert isinstance(signal.final_outcome, bool)

    def test_extract_chunk_signals_includes_source_in_change(self):
        """Verify extracted signal references the chunk source."""
        backend = DummyBackend()
        chunk = Chunk(text="Some discussion", source="title_part", chunk_index=0)
        signals = backend.extract_chunk_signals(
            chunk,
            [{"role": "system", "content": "ignored"}],
        )

        # Signal change should mention the source
        assert "title_part" in signals[0].change

    def test_extract_chunk_signals_ignores_prompt(self):
        """Verify extraction_prompt parameter is accepted but unused."""
        backend = DummyBackend()
        chunk = Chunk(text="Test", source="src", chunk_index=0)

        result1 = backend.extract_chunk_signals(
            chunk,
            [{"role": "system", "content": "prompt A"}],
        )
        result2 = backend.extract_chunk_signals(
            chunk,
            [{"role": "system", "content": "prompt B"}],
        )

        # Results should be identical regardless of prompt
        assert result1[0].change == result2[0].change

    def test_extract_chunk_signals_multiple_chunks_different_signals(self):
        """Verify multiple calls produce signals referencing their respective chunks."""
        backend = DummyBackend()
        chunk1 = Chunk(text="Fix", source="comment_1", chunk_index=0)
        chunk2 = Chunk(text="Enhancement", source="comment_2", chunk_index=1)

        signals1 = backend.extract_chunk_signals(
            chunk1,
            [{"role": "system", "content": "prompt"}],
        )
        signals2 = backend.extract_chunk_signals(
            chunk2,
            [{"role": "system", "content": "prompt"}],
        )

        # Each should have a different source reference
        assert "comment_1" in signals1[0].change
        assert "comment_2" in signals2[0].change


class TestConsolidateSignals:
    """Tests for DummyBackend.consolidate_signals()."""

    def test_consolidate_signals_returns_input_unchanged(self):
        """Verify consolidation returns exact same signals."""
        backend = DummyBackend()
        signals = [
            Signal(
                change="Feature added",
                impact="medium",
                users_affected="most users",
                confidence="high",
                final_outcome=True,
            ),
            Signal(
                change="Bug fixed",
                impact="low",
                users_affected="some users",
                confidence="medium",
                final_outcome=False,
            ),
        ]
        result = backend.consolidate_signals(
            signals,
            [{"role": "system", "content": "placeholder"}],
        )

        assert len(result) == 2
        assert result[0] == signals[0]
        assert result[1] == signals[1]

    def test_consolidate_signals_empty_list(self):
        """Verify consolidation of empty list returns empty list."""
        backend = DummyBackend()
        result = backend.consolidate_signals(
            [],
            [{"role": "system", "content": "prompt"}],
        )
        assert not result

    def test_consolidate_signals_ignores_prompt(self):
        """Verify consolidation_prompt parameter is accepted but unused."""
        backend = DummyBackend()
        signal = Signal(
            change="Change",
            impact="low",
            users_affected="users",
            confidence="medium",
            final_outcome=False,
        )
        signals = [signal]

        result1 = backend.consolidate_signals(
            signals,
            [{"role": "system", "content": "prompt A"}],
        )
        result2 = backend.consolidate_signals(
            signals,
            [{"role": "system", "content": "prompt B"}],
        )

        # Results should be identical
        assert result1 == result2

    def test_consolidate_signals_preserves_order(self):
        """Verify signal order is preserved."""
        backend = DummyBackend()
        signals = [
            Signal(change=f"Signal {i}", impact="low", users_affected="users",
                   confidence="low", final_outcome=False)
            for i in range(5)
        ]
        result = backend.consolidate_signals(
            signals,
            [{"role": "system", "content": "prompt"}],
        )

        for i, signal in enumerate(result):
            assert signal.change == f"Signal {i}"


class TestClassifySignals:
    """Tests for DummyBackend.classify_signals()."""

    def test_classify_signals_returns_classified_signals_object(self):
        """Verify classification returns ClassifiedSignals object."""
        backend = DummyBackend()
        signals = [
            Signal(
                change="Change",
                impact="low",
                users_affected="users",
                confidence="medium",
                final_outcome=False,
            )
        ]
        result = backend.classify_signals(
            signals,
            [{"role": "system", "content": "placeholder"}],
        )

        # Should have classified list and summary
        assert hasattr(result, "classified")
        assert hasattr(result, "summary")
        assert len(result.classified) == 1

    def test_classify_signals_all_assigned_to_minor(self):
        """Verify all signals are classified as 'minor'."""
        backend = DummyBackend()
        signals = [
            Signal(
                change=f"Signal {i}",
                impact="high" if i % 2 == 0 else "low",
                users_affected="users",
                confidence="high" if i % 2 == 0 else "low",
                final_outcome=i % 2 == 0,
            )
            for i in range(5)
        ]
        result = backend.classify_signals(
            signals,
            [{"role": "system", "content": "prompt"}],
        )

        # All should be classified as "minor" regardless of impact
        for classified in result.classified:
            assert classified.category == "minor"

    def test_classify_signals_preserves_signal_data(self):
        """Verify signal data is preserved in classification."""
        backend = DummyBackend()
        original_signal = Signal(
            change="Specific change",
            impact="medium",
            users_affected="many users",
            confidence="high",
            final_outcome=True,
        )
        result = backend.classify_signals(
            [original_signal],
            [{"role": "system", "content": "prompt"}],
        )

        classified = result.classified[0]
        assert classified.signal == original_signal

    def test_classify_signals_includes_summary(self):
        """Verify summary mentions signal count."""
        backend = DummyBackend()
        signals = [
            Signal(change="Sig 1", impact="low", users_affected="users",
                   confidence="low", final_outcome=False),
            Signal(change="Sig 2", impact="low", users_affected="users",
                   confidence="low", final_outcome=False),
            Signal(change="Sig 3", impact="low", users_affected="users",
                   confidence="low", final_outcome=False),
        ]
        result = backend.classify_signals(
            signals,
            [{"role": "system", "content": "prompt"}],
        )

        summary = str(result.summary)
        assert "3" in summary or "three" in summary.lower()

    def test_classify_signals_empty_list(self):
        """Verify empty signal list produces empty classified list."""
        backend = DummyBackend()
        result = backend.classify_signals(
            [],
            [{"role": "system", "content": "prompt"}],
        )
        assert len(result.classified) == 0

    def test_classify_signals_ignores_prompt(self):
        """Verify classification_prompt parameter is accepted but unused."""
        backend = DummyBackend()
        signals = [
            Signal(change="Change", impact="low", users_affected="users",
                   confidence="low", final_outcome=False),
        ]

        result1 = backend.classify_signals(
            signals,
            [{"role": "system", "content": "prompt A"}],
        )
        result2 = backend.classify_signals(
            signals,
            [{"role": "system", "content": "prompt B"}],
        )

        # Results should be identical
        assert len(result1.classified) == len(result2.classified)
        assert result1.classified[0].category == result2.classified[0].category


class TestRenderFinalContext:
    """Tests for DummyBackend.render_final_context()."""

    def test_render_final_context_returns_distilled_context(self):
        """Verify rendering returns DistilledContext object."""
        backend = DummyBackend()

        signal = Signal(
            change="Change",
            impact="low",
            users_affected="users",
            confidence="low",
            final_outcome=False,
        )
        classified = ClassifiedSignals(
            classified=[ClassifiedSignal(signal=signal, category="minor")],
            summary="Test summary",
        )
        metadata = sample_metadata_123()

        result = backend.render_final_context(classified, metadata)

        assert isinstance(result, DistilledContext)
        assert hasattr(result, "summary")
        assert hasattr(result, "structured_signals")
        assert hasattr(result, "classification")
        assert hasattr(result, "metadata")

    def test_render_final_context_includes_classified_data(self):
        """Verify rendered context includes classified signals."""
        backend = DummyBackend()

        signal = Signal(
            change="Critical bug fix",
            impact="high",
            users_affected="all users",
            confidence="high",
            final_outcome=True,
        )
        classified = ClassifiedSignals(
            classified=[ClassifiedSignal(signal=signal, category="major")],
            summary="Important fix",
        )
        metadata = DistilledContextMetadata(
            pr_number=456,
            total_chunks=8,
            selected_chunks=4,
            extraction_phase_duration_ms=200,
            consolidation_phase_duration_ms=100,
            classification_phase_duration_ms=50,
        )

        result = backend.render_final_context(classified, metadata)

        # Structured signals should contain the signal data
        assert len(result.structured_signals) == 1
        assert result.structured_signals[0]["change"] == "Critical bug fix"
        assert result.structured_signals[0]["impact"] == "high"

    def test_render_final_context_includes_metadata(self):
        """Verify rendered context preserves metadata."""
        backend = DummyBackend()

        metadata = DistilledContextMetadata(
            pr_number=789,
            total_chunks=20,
            selected_chunks=10,
            extraction_phase_duration_ms=1000,
            consolidation_phase_duration_ms=500,
            classification_phase_duration_ms=300,
        )

        result = backend.render_final_context(ClassifiedSignals(), metadata)

        assert result.metadata["pr_number"] == 789
        assert result.metadata["total_chunks"] == 20
        assert result.metadata["selected_chunks"] == 10

    def test_render_final_context_empty_signals(self):
        """Verify rendering works with no classified signals."""
        backend = DummyBackend()

        classified = ClassifiedSignals(classified=[], summary="No changes")
        metadata = DistilledContextMetadata(
            pr_number=999,
            total_chunks=5,
            selected_chunks=0,
            extraction_phase_duration_ms=50,
            consolidation_phase_duration_ms=0,
            classification_phase_duration_ms=0,
        )

        result = backend.render_final_context(classified, metadata)

        assert len(result.structured_signals) == 0
        assert result.summary == "No changes"


class TestDummyBackendIntegration:
    """Integration tests for dummy backend as a complete adapter."""

    def test_full_pipeline_flow(self):
        """Verify dummy backend works through complete pipeline flow."""
        backend = DummyBackend()

        # Stage 1: Select chunks
        chunks = [
            Chunk(text=f"Chunk {i}", source=f"source_{i}", chunk_index=i)
            for i in range(3)
        ]
        selected = backend.select_relevant_chunks(
            chunks,
            use_embeddings=False,
            ranking_prompts=[{"role": "user", "content": "dummy ranking prompt"}],
        )
        assert len(selected) == 3

        # Stage 2: Extract signals
        all_signals = []
        for chunk in selected:
            signals = backend.extract_chunk_signals(
                chunk,
                [{"role": "system", "content": "extraction prompt"}],
            )
            all_signals.extend(signals)
        assert len(all_signals) == 3

        # Stage 3: Consolidate signals
        consolidated = backend.consolidate_signals(
            all_signals,
            [{"role": "system", "content": "consolidation prompt"}],
        )
        assert len(consolidated) == 3

        # Stage 4: Classify signals
        classified_objs = [
            ClassifiedSignal(signal=sig, category="minor")
            for sig in consolidated
        ]
        classified = ClassifiedSignals(classified=classified_objs, summary="Test")

        # Stage 5: Render context
        metadata = DistilledContextMetadata(
            pr_number=111,
            total_chunks=3,
            selected_chunks=3,
            extraction_phase_duration_ms=100,
            consolidation_phase_duration_ms=50,
            classification_phase_duration_ms=30,
        )
        final_context = backend.render_final_context(classified, metadata)

        assert isinstance(final_context, DistilledContext)
        assert len(final_context.structured_signals) == 3
