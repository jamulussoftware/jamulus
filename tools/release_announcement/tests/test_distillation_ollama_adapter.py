"""Unit tests for the Ollama adapter (Step 6).

Tests verify that the Ollama adapter correctly implements the DistillationAdapter
protocol and handles errors as specified.
"""

import json
from unittest.mock import patch

import pytest

from tests.assertions import assert_distillation_adapter_surface
from tests.test_data import sample_metadata_123

from src.release_announcement.distillation import (
    Chunk,
    Signal,
    ClassifiedSignal,
    ClassifiedSignals,
    DistilledContext,
)
from src.release_announcement.backends.distillation_adapters.ollama_adapter import (
    OllamaDistillationAdapter,
    _create_ollama_adapter,
)

OLLAMA_CALL_PATH = (
    "src.release_announcement.backends.distillation_adapters."
    "ollama_adapter.call_ollama_model"
)
OLLAMA_EMBED_PATH = (
    "src.release_announcement.backends.distillation_adapters."
    "ollama_adapter.ollama.embed"
)


class TestOllamaAdapterInitialization:
    """Tests for Ollama adapter initialization."""

    def test_ollama_adapter_instantiation(self):
        """Verify OllamaDistillationAdapter can be instantiated."""
        adapter = OllamaDistillationAdapter()
        assert adapter is not None
        assert_distillation_adapter_surface(adapter)

    def test_ollama_adapter_model_names_from_env(self, monkeypatch):
        """Verify model names are read from environment variables."""
        monkeypatch.setenv("OLLAMA_MODEL", "test-model")
        monkeypatch.setenv("OLLAMA_EMBEDDING_MODEL", "test-embedding-model")

        adapter = OllamaDistillationAdapter()
        assert adapter.chat_model == "test-model"
        assert adapter.embedding_model == "test-embedding-model"

    def test_ollama_adapter_default_model_names(self, monkeypatch):
        """Verify default model names are used when env vars are not set."""
        monkeypatch.delenv("OLLAMA_MODEL", raising=False)
        monkeypatch.delenv("OLLAMA_EMBEDDING_MODEL", raising=False)

        adapter = OllamaDistillationAdapter()
        assert adapter.chat_model == "mistral-large-3:675b-cloud"
        assert adapter.embedding_model is None

    def test_ollama_adapter_factory(self):
        """Verify the factory function creates an Ollama adapter."""
        adapter = _create_ollama_adapter()
        assert isinstance(adapter, OllamaDistillationAdapter)


class TestOllamaSelectRelevantChunks:
    """Tests for OllamaAdapter.select_relevant_chunks()."""

    @pytest.fixture
    def sample_chunks(self):
        """Create sample chunks for testing."""
        return [
            Chunk(text=f"Chunk {i}", source=f"comment_{i}", chunk_index=i)
            for i in range(3)
        ]

    def test_select_relevant_chunks_with_embeddings(self, sample_chunks, monkeypatch):
        """Verify select_relevant_chunks uses embeddings when available."""
        monkeypatch.setenv("OLLAMA_EMBEDDING_MODEL", "test-embedding-model")

        adapter = OllamaDistillationAdapter()

        # Mock the ollama.embed call
        with patch(OLLAMA_EMBED_PATH) as mock_embed:
            mock_embed.return_value = {
                "embeddings": [[0.1, 0.2], [0.3, 0.4], [0.5, 0.6]]
            }

            result = adapter.select_relevant_chunks(
                sample_chunks,
                use_embeddings=True,
                ranking_prompts=[{"role": "user", "content": "dummy"}],
            )

            # Verify the embeddings API was called
            mock_embed.assert_called_once_with(
                model="test-embedding-model",
                input=["Chunk 0", "Chunk 1", "Chunk 2"],
            )

            # Verify all chunks are returned with relevance scores
            assert len(result) == 3
            for chunk in result:
                assert chunk.relevance_score > 0

    def test_select_relevant_chunks_with_chat(self, sample_chunks):
        """Verify select_relevant_chunks uses chat when embeddings are not available."""
        adapter = OllamaDistillationAdapter()  # embedding_model is None

        # Create ranking prompts
        prompts = [{"role": "user", "content": "Rank {chunk_count} chunks: {chunks}"}]

        # Mock the call_ollama_model function
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.return_value = json.dumps({"0": 0.9, "1": 0.8, "2": 0.7})

            result = adapter.select_relevant_chunks(
                sample_chunks,
                use_embeddings=False,
                ranking_prompts=prompts,
            )

            # Verify the chat API was called
            mock_call.assert_called_once()

            # Verify all chunks are returned with relevance scores
            assert len(result) == 3
            assert result[0].relevance_score == 0.9
            assert result[1].relevance_score == 0.8
            assert result[2].relevance_score == 0.7

    def test_select_relevant_chunks_error_handling(self, sample_chunks):
        """Verify select_relevant_chunks handles errors correctly."""
        adapter = OllamaDistillationAdapter()

        # Create ranking prompts
        prompts = [{"role": "user", "content": "Rank {chunk_count} chunks: {chunks}"}]

        # Mock the call_ollama_model function to raise an exception
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.side_effect = RuntimeError("Test error")

            with pytest.raises(RuntimeError) as exc_info:
                adapter.select_relevant_chunks(
                    sample_chunks,
                    use_embeddings=False,
                    ranking_prompts=prompts,
                )

            # Verify the error message contains the expected diagnostics
            error_msg = str(exc_info.value)
            assert "[Ollama ranking]" in error_msg
            assert "phase=select_relevant_chunks" in error_msg
            assert "error=Test error" in error_msg


class TestOllamaExtractChunkSignals:
    """Tests for OllamaAdapter.extract_chunk_signals()."""

    @pytest.fixture
    def sample_chunk(self):
        """Create a sample chunk for testing."""
        return Chunk(text="Test chunk", source="comment_1", chunk_index=0)

    def test_extract_chunk_signals_success(self, sample_chunk):
        """Verify extract_chunk_signals returns a list of signals."""
        adapter = OllamaDistillationAdapter()

        # Mock the call_ollama_model function
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.return_value = json.dumps([
                {
                    "change": "Test change",
                    "impact": "high",
                    "users_affected": "all users",
                    "confidence": "high",
                    "final_outcome": True,
                }
            ])

            result = adapter.extract_chunk_signals(
                sample_chunk,
                [{"role": "system", "content": "test prompt"}],
            )

            # Verify the chat API was called
            mock_call.assert_called_once()

            # Verify a signal is returned
            assert len(result) == 1
            assert isinstance(result[0], Signal)
            assert result[0].change == "Test change"

    def test_extract_chunk_signals_fallback(self, sample_chunk):
        """Verify extract_chunk_signals falls back when parsing fails."""
        adapter = OllamaDistillationAdapter()

        # Mock the call_ollama_model function to return invalid JSON
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.return_value = "invalid json"

            result = adapter.extract_chunk_signals(
                sample_chunk,
                [{"role": "system", "content": "test prompt"}],
            )

            # Verify a fallback signal is returned
            assert len(result) == 1
            assert isinstance(result[0], Signal)

    def test_extract_chunk_signals_error_handling(self, sample_chunk):
        """Verify extract_chunk_signals handles errors correctly."""
        adapter = OllamaDistillationAdapter()

        # Mock the call_ollama_model function to raise an exception
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.side_effect = RuntimeError("Test error")

            with pytest.raises(RuntimeError) as exc_info:
                adapter.extract_chunk_signals(
                    sample_chunk,
                    [{"role": "system", "content": "test prompt"}],
                )

            # Verify the error message contains the expected diagnostics
            error_msg = str(exc_info.value)
            assert "[Ollama extraction]" in error_msg
            assert "phase=extract_chunk_signals" in error_msg
            assert "error=Test error" in error_msg


class TestOllamaConsolidateSignals:
    """Tests for OllamaAdapter.consolidate_signals()."""

    @pytest.fixture
    def sample_signals(self):
        """Create sample signals for testing."""
        return [
            Signal(
                change=f"Change {i}",
                impact="high",
                users_affected="all users",
                confidence="high",
                final_outcome=True,
            )
            for i in range(3)
        ]

    def test_consolidate_signals_success(self, sample_signals):
        """Verify consolidate_signals returns a list of signals."""
        adapter = OllamaDistillationAdapter()

        # Mock the call_ollama_model function
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.return_value = json.dumps([
                {
                    "change": "Consolidated change",
                    "impact": "high",
                    "users_affected": "all users",
                    "confidence": "high",
                    "final_outcome": True,
                }
            ])

            result = adapter.consolidate_signals(
                sample_signals,
                [{"role": "system", "content": "test prompt"}],
            )

            # Verify the chat API was called
            mock_call.assert_called_once()

            # Verify signals are returned
            assert len(result) == 1
            assert isinstance(result[0], Signal)

    def test_consolidate_signals_fallback(self, sample_signals):
        """Verify consolidate_signals falls back when parsing fails."""
        adapter = OllamaDistillationAdapter()

        # Mock the call_ollama_model function to return invalid JSON
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.return_value = "invalid json"

            result = adapter.consolidate_signals(
                sample_signals,
                [{"role": "system", "content": "test prompt"}],
            )

            # Verify the input signals are returned unchanged
            assert len(result) == 3
            assert result == sample_signals

    def test_consolidate_signals_error_handling(self, sample_signals):
        """Verify consolidate_signals handles errors correctly."""
        adapter = OllamaDistillationAdapter()

        # Mock the call_ollama_model function to raise an exception
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.side_effect = RuntimeError("Test error")

            with pytest.raises(RuntimeError) as exc_info:
                adapter.consolidate_signals(
                    sample_signals,
                    [{"role": "system", "content": "test prompt"}],
                )

            # Verify the error message contains the expected diagnostics
            error_msg = str(exc_info.value)
            assert "[Ollama consolidation]" in error_msg
            assert "phase=consolidate_signals" in error_msg
            assert "error=Test error" in error_msg


class TestOllamaClassifySignals:
    """Tests for OllamaAdapter.classify_signals()."""

    @pytest.fixture
    def sample_signals(self):
        """Create sample signals for testing."""
        return [
            Signal(
                change=f"Change {i}",
                impact="high",
                users_affected="all users",
                confidence="high",
                final_outcome=True,
            )
            for i in range(3)
        ]

    def test_classify_signals_success(self, sample_signals):
        """Verify classify_signals returns a ClassifiedSignals object."""
        adapter = OllamaDistillationAdapter()

        # Mock the call_ollama_model function
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.return_value = json.dumps({
                "classified": [
                    {"category": "major"},
                    {"category": "minor"},
                    {"category": "internal"},
                ],
                "summary": "Test summary",
            })

            result = adapter.classify_signals(
                sample_signals,
                [{"role": "system", "content": "test prompt"}],
            )

            # Verify the chat API was called
            mock_call.assert_called_once()

            # Verify a ClassifiedSignals object is returned
            assert isinstance(result, ClassifiedSignals)
            assert len(result.classified) == 3
            assert result.summary == "Test summary"

    def test_classify_signals_fallback(self, sample_signals):
        """Verify classify_signals falls back when parsing fails."""
        adapter = OllamaDistillationAdapter()

        # Mock the call_ollama_model function to return invalid JSON
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.return_value = "invalid json"

            result = adapter.classify_signals(
                sample_signals,
                [{"role": "system", "content": "test prompt"}],
            )

            # Verify all signals are classified as "minor"
            assert len(result.classified) == 3
            for classified in result.classified:
                assert classified.category == "minor"

    def test_classify_signals_error_handling(self, sample_signals):
        """Verify classify_signals handles errors correctly."""
        adapter = OllamaDistillationAdapter()

        # Mock the call_ollama_model function to raise an exception
        with patch(OLLAMA_CALL_PATH) as mock_call:
            mock_call.side_effect = RuntimeError("Test error")

            with pytest.raises(RuntimeError) as exc_info:
                adapter.classify_signals(
                    sample_signals,
                    [{"role": "system", "content": "test prompt"}],
                )

            # Verify the error message contains the expected diagnostics
            error_msg = str(exc_info.value)
            assert "[Ollama classification]" in error_msg
            assert "phase=classify_signals" in error_msg
            assert "error=Test error" in error_msg


class TestOllamaRenderFinalContext:
    """Tests for OllamaAdapter.render_final_context()."""

    @pytest.fixture
    def sample_classified(self):
        """Create a sample ClassifiedSignals object for testing."""
        signals = [
            Signal(
                change=f"Change {i}",
                impact="high",
                users_affected="all users",
                confidence="high",
                final_outcome=True,
            )
            for i in range(3)
        ]
        classified_signals = [
            ClassifiedSignal(signal=signal, category="major")
            for signal in signals
        ]
        return ClassifiedSignals(
            classified=classified_signals,
            summary="Test summary",
        )

    @pytest.fixture
    def sample_metadata(self):
        """Create a sample DistilledContextMetadata object for testing."""
        return sample_metadata_123()

    def test_render_final_context_success(self, sample_classified, sample_metadata):
        """Verify render_final_context returns a DistilledContext object."""
        adapter = OllamaDistillationAdapter()

        result = adapter.render_final_context(sample_classified, sample_metadata)

        # Verify a DistilledContext object is returned
        assert isinstance(result, DistilledContext)
        assert result.summary == "Test summary"
        assert len(result.structured_signals) == 3
        assert result.metadata["pr_number"] == 123

    def test_render_final_context_empty_signals(self, sample_metadata):
        """Verify render_final_context handles empty signals."""
        adapter = OllamaDistillationAdapter()

        empty_classified = ClassifiedSignals(
            classified=[],
            summary="Empty summary",
        )

        result = adapter.render_final_context(empty_classified, sample_metadata)

        # Verify a DistilledContext object is returned
        assert isinstance(result, DistilledContext)
        assert result.summary == "Empty summary"
        assert len(result.structured_signals) == 0
