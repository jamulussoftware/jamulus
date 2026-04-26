"""Ollama adapter for the staged distillation pipeline.

Implements the DistillationAdapter protocol for the Ollama backend.
"""

import json
import os
from ...app_logger import logger

import ollama

from ...distillation import (
    build_distilled_context,
    DistillationAdapter,
    Chunk,
    Signal,
    ClassifiedSignal,
    ClassifiedSignals,
    DistilledContext,
    DistilledContextMetadata,
    default_fallback_signal,
    PARSE_EXCEPTIONS,
)
from .common_adapter_utils import build_ranking_messages, build_signal_payload

# Import the call_ollama_model function for making actual Ollama API calls
from ..ollama_backend import call_ollama_model
from ...registry import registry


class OllamaDistillationAdapter(DistillationAdapter):
    """Ollama adapter implementing DistillationAdapter protocol."""

    def __init__(self) -> None:
        """Initialize the Ollama adapter with model names from environment variables."""
        self.chat_model = os.getenv("OLLAMA_MODEL", "mistral-large-3:675b-cloud")
        self.embedding_model = os.getenv("OLLAMA_EMBEDDING_MODEL")  # No default
        # Preserve compatibility with callers that read legacy default-model attributes.
        self._default_chat_model = self.chat_model
        self._default_embedding_model = self.embedding_model

    def probe_chat(self, model: str | None) -> bool:
        """Probe chat capability directly via Ollama chat API."""
        chat_model = model if model is not None else self.chat_model
        payload = {
            "model": chat_model,
            "messages": [{"role": "user", "content": "Reply with 'ok'."}],
        }
        try:
            response = ollama.chat(model=chat_model, messages=payload["messages"])
            content = response.get("message", {}).get("content", "")
            return isinstance(content, str) and bool(content.strip())
        except Exception as err:
            status_code = getattr(err, "status_code", None)
            response_body = getattr(err, "response", None)
            raise RuntimeError(
                "Ollama chat capability probe failed. "
                f"request_payload={json.dumps(payload, ensure_ascii=True)} "
                f"response_status={status_code} "
                f"response_body={response_body} "
                f"error={err}"
            ) from err

    def probe_embeddings(self, model: str | None) -> bool:
        """Probe embedding capability directly via Ollama batch embed API."""
        embedding_model = model if model is not None else self.embedding_model
        if embedding_model is None:
            return False
        payload = {"model": embedding_model, "input": ["test"]}
        try:
            response = ollama.embed(model=embedding_model, input=["test"])
            embeddings = response.get("embeddings")
            return isinstance(embeddings, list) and bool(embeddings)
        except Exception as err:
            status_code = getattr(err, "status_code", None)
            response_body = getattr(err, "response", None)
            raise RuntimeError(
                "Ollama embedding capability probe failed. "
                f"request_payload={json.dumps(payload, ensure_ascii=True)} "
                f"response_status={status_code} "
                f"response_body={response_body} "
                f"error={err}"
            ) from err

    def call_chat(self, prompt: dict) -> str:
        """Call the chat model."""
        return call_ollama_model(prompt, self.chat_model, self.embedding_model)

    def select_relevant_chunks(
        self,
        chunks: list[Chunk],
        use_embeddings: bool,
        ranking_prompts: list[dict[str, str]],
    ) -> list[Chunk]:
        """Select relevant chunks using embeddings or chat-based ranking."""
        if use_embeddings and self.embedding_model:
            embed_payload = {"model": self.embedding_model, "input": [c.text for c in chunks]}
            try:
                embeddings_response = ollama.embed(
                    model=self.embedding_model,
                    input=embed_payload["input"],
                )
                return _score_chunks_from_embeddings(chunks, embeddings_response)
            except Exception as err:
                diagnostic = (
                    f"[Ollama ranking] phase=select_relevant_chunks "
                    f"mode=embeddings model={self.embedding_model} "
                    f"chunk_count={len(chunks)} "
                    f"request_payload={json.dumps(embed_payload, ensure_ascii=True)} "
                    f"error={err}"
                )
                logger.error(diagnostic)
                raise RuntimeError(diagnostic) from err

        messages = build_ranking_messages(ranking_prompts, chunks)
        ranking_payload = {"model": self.chat_model, "messages": messages}
        try:
            response = call_ollama_model(
                {"messages": messages},
                chat_model_override=self.chat_model,
            )
            scores = _parse_chat_scores(response, len(chunks))
            return _score_chunks_from_scores(chunks, scores)
        except Exception as err:
            diagnostic = (
                f"[Ollama ranking] phase=select_relevant_chunks "
                f"mode=chat model={self.chat_model} "
                f"chunk_count={len(chunks)} "
                f"request_payload={json.dumps(ranking_payload, ensure_ascii=True)} "
                f"error={err}"
            )
            logger.error(diagnostic)
            raise RuntimeError(diagnostic) from err

    def extract_chunk_signals(
        self,
        chunk: Chunk,
        extraction_prompts: list[dict[str, str]],
    ) -> list[Signal]:
        """Extract signals from a chunk.

        Args:
            chunk: The chunk to extract signals from.
            extraction_prompts: Prompt list for extraction.

        Returns:
            List of extracted signals.

        Raises:
            RuntimeError: If the Ollama API call fails.
        """
        messages = _messages_with_user_content(extraction_prompts, chunk.text)
        request_payload = {"model": self.chat_model, "messages": messages}
        try:
            response = call_ollama_model(
                {"messages": messages},
                chat_model_override=self.chat_model,
            )

            try:
                signals_data = json.loads(response)
                return [
                    Signal(
                        change=signal_data.get("change", ""),
                        impact=signal_data.get("impact", "low"),
                        users_affected=signal_data.get("users_affected", ""),
                        confidence=signal_data.get("confidence", "low"),
                        final_outcome=signal_data.get("final_outcome", False),
                    )
                    for signal_data in signals_data
                ]
            except (json.JSONDecodeError, KeyError, TypeError):
                return [default_fallback_signal(chunk.source)]
        except Exception as err:
            diagnostic = (
                f"[Ollama extraction] phase=extract_chunk_signals "
                f"chunk_index={chunk.chunk_index} model={self.chat_model} "
                f"request_payload={json.dumps(request_payload, ensure_ascii=True)} "
                f"error={err}"
            )
            logger.error(diagnostic)
            raise RuntimeError(diagnostic) from err

    def consolidate_signals(
        self,
        signals: list[Signal],
        consolidation_prompts: list[dict[str, str]],
    ) -> list[Signal]:
        """Consolidate signals."""
        messages = _messages_with_user_content(consolidation_prompts, build_signal_payload(signals))
        request_payload = {"model": self.chat_model, "messages": messages}
        try:
            response = call_ollama_model(
                {"messages": messages},
                chat_model_override=self.chat_model,
            )
            try:
                consolidated_signals_data = json.loads(response)
                return [
                    Signal(
                        change=signal_data.get("change", ""),
                        impact=signal_data.get("impact", "low"),
                        users_affected=signal_data.get("users_affected", ""),
                        confidence=signal_data.get("confidence", "low"),
                        final_outcome=signal_data.get("final_outcome", False),
                    )
                    for signal_data in consolidated_signals_data
                ]
            except (json.JSONDecodeError, KeyError, TypeError):
                return signals
        except Exception as err:
            diagnostic = (
                f"[Ollama consolidation] phase=consolidate_signals "
                f"signal_count={len(signals)} model={self.chat_model} "
                f"request_payload={json.dumps(request_payload, ensure_ascii=True)} "
                f"error={err}"
            )
            logger.error(diagnostic)
            raise RuntimeError(diagnostic) from err

    def classify_signals(
        self,
        signals: list[Signal],
        classification_prompts: list[dict[str, str]],
    ) -> ClassifiedSignals:
        """Classify signals."""
        messages = _messages_with_user_content(classification_prompts, build_signal_payload(signals))
        request_payload = {"model": self.chat_model, "messages": messages}
        try:
            response = call_ollama_model(
                {"messages": messages},
                chat_model_override=self.chat_model,
            )
            try:
                classified_data = json.loads(response)
                classified_signals = [
                    ClassifiedSignal(
                        signal=signal,
                        category=classified_data.get("classified", [{}])[
                            i].get("category", "minor"),
                    )
                    for i, signal in enumerate(signals)
                ]
                summary = classified_data.get("summary", "Classification summary")
            except (json.JSONDecodeError, KeyError, TypeError, IndexError):
                classified_signals = [
                    ClassifiedSignal(signal=signal, category="minor")
                    for signal in signals
                ]
                summary = f"Fallback classification: {len(signals)} signals classified as minor"
            return ClassifiedSignals(
                classified=classified_signals,
                summary=summary,
            )
        except Exception as err:
            diagnostic = (
                f"[Ollama classification] phase=classify_signals "
                f"signal_count={len(signals)} model={self.chat_model} "
                f"request_payload={json.dumps(request_payload, ensure_ascii=True)} "
                f"error={err}"
            )
            logger.error(diagnostic)
            raise RuntimeError(diagnostic) from err

    def render_final_context(
        self,
        classified: ClassifiedSignals,
        metadata: DistilledContextMetadata,
    ) -> DistilledContext:
        """Render the final distilled context.

        Args:
            classified: ClassifiedSignals object.
            metadata: DistilledContextMetadata object.

        Returns:
            DistilledContext object.
        """
        context = build_distilled_context(classified=classified, metadata=metadata)
        return context

    def get_adapter_tag(self) -> str:
        """Return the adapter identifier for logging."""
        return "ollama"


def _create_ollama_adapter() -> OllamaDistillationAdapter:
    """Factory function for creating an Ollama adapter."""
    return OllamaDistillationAdapter()


def _messages_with_user_content(
    base_messages: list[dict[str, str]],
    user_content: str,
) -> list[dict[str, str]]:
    """Clone prompt messages and append a user message payload for stage input."""
    messages = [
        {"role": str(msg.get("role", "")), "content": str(msg.get("content", ""))}
        for msg in base_messages
        if isinstance(msg, dict)
    ]
    messages.append({"role": "user", "content": user_content})
    return messages


def _score_chunks_from_embeddings(
    chunks: list[Chunk],
    embeddings_response: dict,
) -> list[Chunk]:
    embeddings_list = embeddings_response.get("embeddings")
    if not isinstance(embeddings_list, list) or len(embeddings_list) < len(chunks):
        raise RuntimeError(
            "Ollama embeddings response missing vectors for ranked chunks. "
            f"response={json.dumps(embeddings_response, ensure_ascii=True)}"
        )

    scored_chunks: list[Chunk] = []
    for i, (chunk, embedding) in enumerate(zip(chunks, embeddings_list)):
        if not isinstance(embedding, list) or not embedding:
            raise RuntimeError(
                "Ollama embeddings response missing embedding vector. "
                f"chunk_index={i} response={json.dumps(embeddings_response, ensure_ascii=True)}"
            )
        relevance_score = sum(x * x for x in embedding) ** 0.5
        scored_chunks.append(
            Chunk(
                text=chunk.text,
                source=chunk.source,
                relevance_score=relevance_score,
                chunk_index=chunk.chunk_index,
            )
        )
    return scored_chunks


def _parse_chat_scores(response: str, chunk_count: int) -> list[float]:
    try:
        scores_dict = json.loads(response)
        if not isinstance(scores_dict, dict):
            raise ValueError(
                "Expected JSON object with chunk indices as keys, "
                f"got {type(scores_dict).__name__}"
            )

        scores: list[float] = []
        for i in range(chunk_count):
            chunk_key = str(i)
            if chunk_key not in scores_dict:
                raise ValueError(
                    f"Missing chunk index '{chunk_key}' in response. "
                    f"Expected indices 0-{chunk_count - 1}"
                )
            score_value = scores_dict[chunk_key]
            try:
                score = float(score_value)
            except (TypeError, ValueError) as score_err:
                raise ValueError(
                    f"Invalid score for chunk {i}: {score_value} - {score_err}"
                ) from score_err
            if not 0.0 <= score <= 1.0:
                raise ValueError(f"Score {score} for chunk {i} is outside 0.0-1.0 range")
            scores.append(score)
        return scores
    except (json.JSONDecodeError, ValueError, TypeError) as parse_err:
        diagnostic = (
            f"[Ollama ranking] phase=select_relevant_chunks parse_error=True "
            f"response_length={len(response)} "
            "expected_format='JSON object with indices as keys, "
            "e.g. {\"0\": 0.92, \"1\": 0.45}' "
            f"parse_error_detail={str(parse_err)}"
        )
        logger.error(diagnostic)
        raise RuntimeError(diagnostic) from parse_err


def _score_chunks_from_scores(chunks: list[Chunk], scores: list[float]) -> list[Chunk]:
    scored_chunks: list[Chunk] = []
    for i, chunk in enumerate(chunks):
        relevance_score = scores[i] if i < len(scores) else 0.0
        scored_chunks.append(
            Chunk(
                text=chunk.text,
                source=chunk.source,
                relevance_score=relevance_score,
                chunk_index=chunk.chunk_index,
            )
        )
    return scored_chunks


# Register the Ollama adapter at import time.
registry.register("ollama", _create_ollama_adapter)
