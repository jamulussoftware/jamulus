"""Unit tests for the GitHub distillation adapter."""

import json
from unittest.mock import patch

import pytest

from src.release_announcement.backends.distillation_adapters.github_adapter import (
    GitHubDistillationAdapter,
)
from src.release_announcement.distillation import Chunk, Signal

GH_CHAT_PATH = (
    "src.release_announcement.backends.distillation_adapters.github_adapter."
    "GitHubDistillationAdapter._github_chat_completion"
)
GH_POST_PATH = (
    "src.release_announcement.backends.distillation_adapters.github_adapter."
    "GitHubDistillationAdapter._github_post_json"
)


@pytest.fixture
def github_adapter() -> GitHubDistillationAdapter:
    return GitHubDistillationAdapter(token_resolver=lambda: "test-token")


@pytest.fixture
def sample_chunks() -> list[Chunk]:
    return [Chunk(text=f"Chunk {i}", source=f"comment_{i}", chunk_index=i) for i in range(3)]


@pytest.fixture
def sample_signal() -> Signal:
    return Signal(
        change="Test change",
        impact="low",
        users_affected="users",
        confidence="high",
        final_outcome=True,
    )


def test_github_adapter_protocol_surface_present(
    github_adapter: GitHubDistillationAdapter,
) -> None:
    assert hasattr(github_adapter, "select_relevant_chunks")
    assert hasattr(github_adapter, "extract_chunk_signals")
    assert hasattr(github_adapter, "consolidate_signals")
    assert hasattr(github_adapter, "classify_signals")
    assert hasattr(github_adapter, "render_final_context")


def test_github_select_relevant_chunks_chat_single_provider_call(
    github_adapter: GitHubDistillationAdapter,
    sample_chunks: list[Chunk],
) -> None:
    ranking_prompts = [
        {"role": "system", "content": "Rank chunks"},
        {"role": "user", "content": "Rank {chunk_count} chunks: {chunks}"},
    ]

    with patch(GH_CHAT_PATH) as mock_chat:
        mock_chat.return_value = json.dumps({"0": 0.9, "1": 0.8, "2": 0.7})

        out = github_adapter.select_relevant_chunks(
            sample_chunks,
            use_embeddings=False,
            ranking_prompts=ranking_prompts,
        )

        assert mock_chat.call_count == 1
        payload = mock_chat.call_args.args[0]
        assert payload["model"] == github_adapter.chat_model
        assert len(payload["messages"]) == 2
        assert "Chunk 0" in payload["messages"][1]["content"]
        assert len(out) == 3


def test_github_select_relevant_chunks_embeddings_single_provider_call(
    github_adapter: GitHubDistillationAdapter,
    sample_chunks: list[Chunk],
) -> None:
    with patch(GH_POST_PATH) as mock_post:
        mock_post.return_value = {
            "data": [
                {"index": 0, "embedding": [0.1, 0.2]},
                {"index": 1, "embedding": [0.3, 0.4]},
                {"index": 2, "embedding": [0.5, 0.6]},
            ]
        }

        out = github_adapter.select_relevant_chunks(
            sample_chunks,
            use_embeddings=True,
            ranking_prompts=[{"role": "user", "content": "unused"}],
        )

        assert mock_post.call_count == 1
        assert mock_post.call_args.args[0] == github_adapter.embedding_endpoint
        payload = mock_post.call_args.args[1]
        assert payload["model"] == github_adapter.embedding_model
        assert payload["input"] == ["Chunk 0", "Chunk 1", "Chunk 2"]
        assert len(out) == 3


def test_github_extract_chunk_signals_parses_typed_signals(
    github_adapter: GitHubDistillationAdapter,
) -> None:
    chunk = Chunk(text="Body", source="body", chunk_index=0)
    with patch(GH_CHAT_PATH) as mock_chat:
        mock_chat.return_value = json.dumps(
            [
                {
                    "change": "Extracted",
                    "impact": "high",
                    "users_affected": "all",
                    "confidence": "high",
                    "final_outcome": True,
                }
            ]
        )

        signals = github_adapter.extract_chunk_signals(
            chunk,
            [{"role": "system", "content": "extract"}],
        )

        assert mock_chat.call_count == 1
        assert len(signals) == 1
        assert signals[0].change == "Extracted"


def test_github_consolidate_signals_single_provider_call(
    github_adapter: GitHubDistillationAdapter,
    sample_signal: Signal,
) -> None:
    with patch(GH_CHAT_PATH) as mock_chat:
        mock_chat.return_value = json.dumps(
            [
                {
                    "change": "Consolidated",
                    "impact": "low",
                    "users_affected": "users",
                    "confidence": "high",
                    "final_outcome": True,
                }
            ]
        )

        _ = github_adapter.consolidate_signals(
            [sample_signal],
            [{"role": "system", "content": "consolidate"}],
        )

        assert mock_chat.call_count == 1
        payload = mock_chat.call_args.args[0]
        assert payload["messages"][-1]["role"] == "user"


def test_github_classify_signals_single_provider_call(
    github_adapter: GitHubDistillationAdapter,
    sample_signal: Signal,
) -> None:
    with patch(GH_CHAT_PATH) as mock_chat:
        mock_chat.return_value = json.dumps(
            {
                "classified": [
                    {
                        "signal": {
                            "change": sample_signal.change,
                            "impact": sample_signal.impact,
                            "users_affected": sample_signal.users_affected,
                            "confidence": sample_signal.confidence,
                            "final_outcome": sample_signal.final_outcome,
                        },
                        "category": "minor",
                    }
                ],
                "summary": "ok",
            }
        )

        _ = github_adapter.classify_signals(
            [sample_signal],
            [{"role": "system", "content": "classify"}],
        )

        assert mock_chat.call_count == 1


def test_github_malformed_response_raises_diagnostic_runtime_error(
    github_adapter: GitHubDistillationAdapter,
) -> None:
    chunk = Chunk(text="Body", source="body", chunk_index=1)

    with patch(GH_CHAT_PATH) as mock_chat:
        mock_chat.return_value = "not-json"

        with pytest.raises(RuntimeError) as exc_info:
            github_adapter.extract_chunk_signals(
                chunk,
                [{"role": "system", "content": "extract"}],
            )

        msg = str(exc_info.value)
        assert "[GitHub extraction]" in msg
        assert "request_payload=" in msg


def test_github_invalid_model_surfaces_fatal_diagnostics(
    github_adapter: GitHubDistillationAdapter,
    sample_chunks: list[Chunk],
) -> None:
    with patch(GH_CHAT_PATH) as mock_chat:
        mock_chat.side_effect = RuntimeError("status=404 response=model not found")

        with pytest.raises(RuntimeError) as exc_info:
            github_adapter.select_relevant_chunks(
                sample_chunks,
                use_embeddings=False,
                ranking_prompts=[
                    {"role": "user", "content": "Rank {chunk_count} {chunks}"}
                ],
            )

        msg = str(exc_info.value)
        assert "[GitHub ranking]" in msg
        assert "request_payload=" in msg
        assert "status=404" in msg


def test_github_mid_pipeline_failure_logs_and_reraises(
    github_adapter: GitHubDistillationAdapter,
    capsys: pytest.CaptureFixture[str],
) -> None:
    chunk = Chunk(text="Body", source="body", chunk_index=5)
    with patch(GH_CHAT_PATH) as mock_chat:
        mock_chat.side_effect = RuntimeError("http=500 body=boom")

        with pytest.raises(RuntimeError) as exc_info:
            github_adapter.extract_chunk_signals(
                chunk,
                [{"role": "system", "content": "extract"}],
            )

        msg = str(exc_info.value)
        assert "[GitHub extraction]" in msg
        assert "chunk_index=5" in msg
        assert "request_payload=" in msg
        assert "http=500 body=boom" in msg

        captured = capsys.readouterr()
        assert "[GitHub extraction]" in captured.err
