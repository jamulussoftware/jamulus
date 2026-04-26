"""GitHub Models adapters for staged distillation pipeline phases.

This module provides a single adapter class with two factory functions:
- github backend: env token first, then gh auth token fallback
- actions backend: GITHUB_TOKEN env only
"""

from __future__ import annotations

import json
import os
import subprocess
import urllib.error
import urllib.request
from dataclasses import dataclass
from typing import Any, Callable

from ...distillation import (
    build_distilled_context,
    Chunk,
    ClassifiedSignals,
    DistillationAdapter,
    DistilledContext,
    DistilledContextMetadata,
    Signal,
    _parse_classified_signals,
    _parse_signal_list,
)
from .common_adapter_utils import build_ranking_messages, build_signal_payload
from ...registry import ModelNotFoundError, registry
from ...app_logger import logger


@dataclass
class GitHubAPIError(Exception):
    """Structured API error for GitHub Models calls."""

    message: str
    status_code: int | None = None
    headers: dict[str, str] | None = None
    body: str | None = None
    request_payload: dict[str, Any] | None = None
    endpoint: str | None = None

    def __str__(self) -> str:
        parts = [self.message]
        if self.status_code is not None:
            parts.append(f"(status code: {self.status_code})")
        if self.body:
            parts.append(f"response_body={self.body}")
        return " ".join(parts)


class GitHubDistillationAdapter(DistillationAdapter):
    """GitHub Models adapter implementing the staged distillation protocol."""

    def __init__(self, token_resolver: Callable[[], str]) -> None:
        self.token_resolver = token_resolver
        self.chat_model = "openai/gpt-4o"
        self.embedding_model = "openai/text-embedding-3-small"
        # Preserve compatibility with callers that read legacy default-model attributes.
        self._default_chat_model = self.chat_model
        self._default_embedding_model = self.embedding_model
        self.chat_endpoint = "https://models.github.ai/inference/chat/completions"
        self.embedding_endpoint = "https://models.github.ai/inference/embeddings"
        self._token_cache: str | None = None

    @property
    def token(self) -> str:
        if self._token_cache is None:
            self._token_cache = self.token_resolver().strip()
        return self._token_cache

    def _github_post_json(
        self,
        endpoint: str,
        request_payload: dict[str, Any],
    ) -> dict[str, Any]:
        req = urllib.request.Request(
            endpoint,
            data=json.dumps(request_payload).encode("utf-8"),
            headers={
                "Authorization": f"Bearer {self.token}",
                "Content-Type": "application/json",
            },
            method="POST",
        )
        try:
            with urllib.request.urlopen(req, timeout=60) as resp:
                raw_body = resp.read().decode("utf-8")
                return json.loads(raw_body)
        except urllib.error.HTTPError as err:
            body = err.read().decode("utf-8", errors="replace")
            raise GitHubAPIError(
                message="GitHub Models HTTP error",
                status_code=err.code,
                headers=dict(err.headers.items()) if err.headers else None,
                body=body,
                request_payload=request_payload,
                endpoint=endpoint,
            ) from err
        except urllib.error.URLError as err:
            raise GitHubAPIError(
                message="GitHub Models network error",
                body=str(err),
                request_payload=request_payload,
                endpoint=endpoint,
            ) from err
        except json.JSONDecodeError as err:
            raise GitHubAPIError(
                message="GitHub Models returned non-JSON response",
                body=str(err),
                request_payload=request_payload,
                endpoint=endpoint,
            ) from err

    def _github_chat_completion(self, request_payload: dict[str, Any]) -> str:
        data = self._github_post_json(self.chat_endpoint, request_payload)
        try:
            return str(data["choices"][0]["message"]["content"])
        except (KeyError, IndexError, TypeError) as err:
            raise GitHubAPIError(
                message="GitHub chat response missing choices[0].message.content",
                body=json.dumps(data, ensure_ascii=True),
                request_payload=request_payload,
                endpoint=self.chat_endpoint,
            ) from err

    def _maybe_raise_model_not_found(
        self,
        model: str | None,
        backend_name: str,
        err: Exception,
    ) -> None:
        text = str(err).lower()
        if model and "model" in text and (
            "not found" in text or "does not exist" in text or "unknown" in text
        ):
            raise ModelNotFoundError(
                f"model '{model}' not found on backend '{backend_name}'"
            ) from err

    def _rank_chunks_with_chat(
        self,
        chunks: list[Chunk],
        ranking_prompts: list[dict[str, str]],
    ) -> list[Chunk]:
        messages = build_ranking_messages(ranking_prompts, chunks)
        request_payload = {
            "model": self.chat_model,
            "messages": messages,
        }
        content = self._github_chat_completion(request_payload)
        scores = json.loads(content)
        if not isinstance(scores, dict):
            raise ValueError("Ranking response is not a JSON object")
        ranked_chunks = []
        for index, chunk in enumerate(chunks):
            score_raw = scores.get(str(index), 0.0)
            ranked_chunks.append(
                Chunk(
                    text=chunk.text,
                    source=chunk.source,
                    relevance_score=float(score_raw),
                    chunk_index=chunk.chunk_index,
                )
            )
        return ranked_chunks

    def _rank_chunks_with_embeddings(self, chunks: list[Chunk]) -> list[Chunk]:
        request_payload = {
            "model": self.embedding_model,
            "input": [chunk.text for chunk in chunks],
        }
        data = self._github_post_json(self.embedding_endpoint, request_payload)
        embeddings_data = data.get("data")
        if not isinstance(embeddings_data, list) or len(embeddings_data) != len(chunks):
            raise ValueError("Invalid embedding response length for ranked chunk batch")

        vectors = [
            item.get("embedding") if isinstance(item, dict) else None
            for item in embeddings_data
        ]

        scored_chunks: list[Chunk] = []
        for index, chunk in enumerate(chunks):
            vector = vectors[index]
            if not isinstance(vector, list) or not vector:
                raise ValueError(f"Missing embedding vector for chunk index {index}")
            score = sum(float(v) * float(v) for v in vector) ** 0.5
            scored_chunks.append(
                Chunk(
                    text=chunk.text,
                    source=chunk.source,
                    relevance_score=score,
                    chunk_index=chunk.chunk_index,
                )
            )

        return scored_chunks

    def probe_chat(self, model: str | None) -> bool:
        probe_model = model or self.chat_model
        request_payload = {
            "model": probe_model,
            "messages": [{"role": "user", "content": "Reply with 'ok'."}],
        }
        try:
            content = self._github_chat_completion(request_payload)
            return bool(content.strip())
        except GitHubAPIError as err:
            self._maybe_raise_model_not_found(probe_model, "github", err)
            raise

    def probe_embeddings(self, model: str | None) -> bool:
        probe_model = model or self.embedding_model
        request_payload = {"model": probe_model, "input": ["test"]}
        try:
            data = self._github_post_json(self.embedding_endpoint, request_payload)
            vectors = data.get("data")
            return isinstance(vectors, list) and len(vectors) > 0
        except GitHubAPIError as err:
            self._maybe_raise_model_not_found(probe_model, "github", err)
            return False

    def call_chat(self, prompt: dict) -> str:
        request_payload = {
            "model": str(prompt.get("model") or self.chat_model),
            "messages": prompt.get("messages", []),
        }
        model_parameters = prompt.get("modelParameters")
        if isinstance(model_parameters, dict):
            request_payload.update(model_parameters)
        return self._github_chat_completion(request_payload)

    def select_relevant_chunks(
        self,
        chunks: list[Chunk],
        use_embeddings: bool,
        ranking_prompts: list[dict[str, str]],
    ) -> list[Chunk]:
        if use_embeddings:
            try:
                return self._rank_chunks_with_embeddings(chunks)
            except Exception as err:
                request_payload = {
                    "model": self.embedding_model,
                    "input": [chunk.text for chunk in chunks],
                }
                diagnostic = (
                    "[GitHub ranking] phase=select_relevant_chunks "
                    f"mode=embeddings endpoint={self.embedding_endpoint} "
                    f"request_payload={json.dumps(request_payload, ensure_ascii=True)} "
                    f"error={str(err)}"
                )
                logger.error(diagnostic)
                raise RuntimeError(diagnostic) from err
        try:
            return self._rank_chunks_with_chat(chunks, ranking_prompts)
        except Exception as err:
            messages = build_ranking_messages(ranking_prompts, chunks)
            request_payload = {
                "model": self.chat_model,
                "messages": messages,
            }
            diagnostic = (
                "[GitHub ranking] phase=select_relevant_chunks "
                f"mode=chat endpoint={self.chat_endpoint} "
                f"request_payload={json.dumps(request_payload, ensure_ascii=True)} "
                f"error={str(err)}"
            )
            logger.error(diagnostic)
            raise RuntimeError(diagnostic) from err

    def extract_chunk_signals(
        self,
        chunk: Chunk,
        extraction_prompts: list[dict[str, str]],
    ) -> list[Signal]:
        request_payload = {
            "model": self.chat_model,
            "messages": [
                *[
                    {
                        "role": str(prompt_msg.get("role", "")),
                        "content": str(prompt_msg.get("content", "")),
                    }
                    for prompt_msg in extraction_prompts
                ],
                {"role": "user", "content": chunk.text},
            ],
        }
        try:
            content = self._github_chat_completion(request_payload)
            return _parse_signal_list(content)
        except Exception as err:
            diagnostic = (
                "[GitHub extraction] phase=extract_chunk_signals "
                f"chunk_index={chunk.chunk_index} endpoint={self.chat_endpoint} "
                f"request_payload={json.dumps(request_payload, ensure_ascii=True)} "
                f"error={str(err)}"
            )
            logger.error(diagnostic)
            raise RuntimeError(diagnostic) from err

    def consolidate_signals(
        self,
        signals: list[Signal],
        consolidation_prompts: list[dict[str, str]],
    ) -> list[Signal]:
        request_payload = {
            "model": self.chat_model,
            "messages": [
                *[
                    {
                        "role": str(prompt_msg.get("role", "")),
                        "content": str(prompt_msg.get("content", "")),
                    }
                    for prompt_msg in consolidation_prompts
                ],
                {
                    "role": "user",
                    "content": build_signal_payload(signals),
                },
            ],
        }
        try:
            content = self._github_chat_completion(request_payload)
            return _parse_signal_list(content)
        except Exception as err:
            diagnostic = (
                "[GitHub consolidation] phase=consolidate_signals "
                f"signal_count={len(signals)} endpoint={self.chat_endpoint} "
                f"request_payload={json.dumps(request_payload, ensure_ascii=True)} "
                f"error={str(err)}"
            )
            logger.error(diagnostic)
            raise RuntimeError(diagnostic) from err

    def classify_signals(
        self,
        signals: list[Signal],
        classification_prompts: list[dict[str, str]],
    ) -> ClassifiedSignals:
        request_payload = {
            "model": self.chat_model,
            "messages": [
                *[
                    {
                        "role": str(prompt_msg.get("role", "")),
                        "content": str(prompt_msg.get("content", "")),
                    }
                    for prompt_msg in classification_prompts
                ],
                {
                    "role": "user",
                    "content": build_signal_payload(signals),
                },
            ],
        }
        try:
            content = self._github_chat_completion(request_payload)
            return _parse_classified_signals(content)
        except Exception as err:
            diagnostic = (
                "[GitHub classification] phase=classify_signals "
                f"signal_count={len(signals)} endpoint={self.chat_endpoint} "
                f"request_payload={json.dumps(request_payload, ensure_ascii=True)} "
                f"error={str(err)}"
            )
            logger.error(diagnostic)
            raise RuntimeError(diagnostic) from err

    def render_final_context(
        self,
        classified: ClassifiedSignals,
        metadata: DistilledContextMetadata,
    ) -> DistilledContext:
        return build_distilled_context(classified=classified, metadata=metadata)

    def get_adapter_tag(self) -> str:
        """Return the adapter identifier for logging."""
        return "github"


def _normalize_token(raw_token: str) -> str:
    return raw_token.replace("\r", "").replace("\n", "").strip()


def _create_github_adapter() -> GitHubDistillationAdapter:
    """Factory for `--backend github` using env token or gh fallback."""

    def _resolver() -> str:
        raw_token = os.getenv("GH_TOKEN") or os.getenv("GITHUB_TOKEN")
        if raw_token:
            return _normalize_token(raw_token)

        try:
            gh_token = subprocess.check_output(
                ["gh", "auth", "token"],
                text=True,
                stderr=subprocess.STDOUT,
            )
            return _normalize_token(gh_token)
        except (FileNotFoundError, subprocess.CalledProcessError) as err:
            raise RuntimeError(
                "Unable to resolve GitHub token for backend 'github'. "
                "Set GH_TOKEN/GITHUB_TOKEN or ensure `gh auth token` works."
            ) from err

    return GitHubDistillationAdapter(token_resolver=_resolver)


def _create_actions_adapter() -> GitHubDistillationAdapter:
    """Factory for `--backend actions` requiring GITHUB_TOKEN only."""

    def _resolver() -> str:
        raw_token = os.getenv("GITHUB_TOKEN")
        if not raw_token:
            raise RuntimeError(
                "--backend actions requires GITHUB_TOKEN to be set."
            )
        return _normalize_token(raw_token)

    return GitHubDistillationAdapter(token_resolver=_resolver)

registry.register("github", _create_github_adapter)
registry.register("actions", _create_actions_adapter)
