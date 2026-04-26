"""Ollama backend for release-announcement generation."""

import json
import os
from typing import Any

import ollama
from ..app_logger import logger
from ..distillation import PARSE_EXCEPTIONS


def call_ollama_model(
    prompt: dict[str, Any],
    chat_model_override: str | None = None,
    embedding_model_override: str | None = None,
    model_override: str | None = None,
) -> str:
    """Call Ollama API for local model inference."""
    del embedding_model_override  # Ollama backend currently uses chat model only.
    model = (
        chat_model_override
        or model_override
        or os.getenv("OLLAMA_MODEL", "mistral-large-3:675b-cloud")
    )
    response = ollama.chat(model=model, messages=prompt["messages"])
    return response["message"]["content"].strip()


def probe_capabilities(
    chat_model: str | None,
    embedding_model: str | None,
    probe_embeddings: bool,
) -> dict[str, bool]:
    """Probe Ollama chat/embedding support for the selected models."""
    supports_chat = False
    if not probe_embeddings and chat_model is not None:
        chat_payload = {
            "model": chat_model,
            "messages": [{"role": "user", "content": "Reply with 'ok'."}],
        }
        try:
            response = ollama.chat(model=chat_model, messages=chat_payload["messages"])
            content = response.get("message", {}).get("content", "")
            if not isinstance(content, str) or not content.strip():
                raise RuntimeError(
                    "Ollama chat probe returned an empty or invalid message content. "
                    f"request_payload={json.dumps(chat_payload, ensure_ascii=True)} "
                    f"response={json.dumps(response, ensure_ascii=True)}"
                )
            supports_chat = True
        except Exception as err:
            raise RuntimeError(
                "Ollama chat capability probe failed. "
                f"request_payload={json.dumps(chat_payload, ensure_ascii=True)} "
                f"error={err}"
            ) from err

    supports_embeddings = False
    if probe_embeddings:
        model = embedding_model or os.getenv("OLLAMA_EMBEDDING_MODEL")
        if not model:
            return {"supports_chat": supports_chat, "supports_embeddings": False}
        embed_payload = {"model": model, "input": "test"}
        try:
            response = ollama.embeddings(model=model, prompt="test")
            vector = response.get("embedding")
            supports_embeddings = isinstance(vector, list) and bool(vector)
            if not supports_embeddings:
                raise RuntimeError(
                    "Ollama embeddings probe returned no embedding vector. "
                    f"request_payload={json.dumps(embed_payload, ensure_ascii=True)} "
                    f"response={json.dumps(response, ensure_ascii=True)}"
                )
        except PARSE_EXCEPTIONS as err:
            logger.warning(
                "Ollama embedding capability probe failed; "
                "continuing with chat-only staged mode. "
                f"request_payload={json.dumps(embed_payload, ensure_ascii=True)} "
                f"error={err}"
            )
            supports_embeddings = False

    return {"supports_chat": supports_chat, "supports_embeddings": supports_embeddings}
