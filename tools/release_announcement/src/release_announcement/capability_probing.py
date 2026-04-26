"""Backend capability probing and staged-mode validation logic."""

from __future__ import annotations

import json
from dataclasses import dataclass
from typing import Callable

from .cli_config import BackendCapabilities, BackendConfig
from .distillation import PARSE_EXCEPTIONS
from .registry import BackendProtocol, ModelNotFoundError


PROBE_EXCEPTIONS = PARSE_EXCEPTIONS + (OSError, AttributeError)


@dataclass(frozen=True)
class CapabilityProbeDeps:
    """Injected callables and endpoint constants used by capability probing."""

    get_backend: Callable[[str], BackendProtocol | None]
    log_warning: Callable[[str], None]


def _format_probe_error(
    *,
    phase: str,
    backend_name: str,
    model: str | None,
    err: Exception,
) -> str:
    """Return a diagnostics-rich probe error message suitable for user-facing logs."""
    parts = [
        f"Capability probe failure: phase={phase}",
        f"backend={backend_name}",
        f"model={model!r}",
    ]

    endpoint = getattr(err, "endpoint", None)
    if endpoint is not None:
        parts.append(f"endpoint={endpoint}")

    request_payload = getattr(err, "request_payload", None)
    if request_payload is not None:
        parts.append(
            "request_payload="
            f"{json.dumps(request_payload, ensure_ascii=True, default=str)}"
        )

    status_code = getattr(err, "status_code", None)
    if status_code is not None:
        parts.append(f"response_status={status_code}")

    headers = getattr(err, "headers", None)
    if headers is not None:
        parts.append(
            "response_headers="
            f"{json.dumps(headers, ensure_ascii=True, default=str)}"
        )

    body = getattr(err, "body", None)
    if body is not None:
        parts.append(f"response_body={body}")

    parts.append(f"error={err}")
    return " ".join(parts)


def probe_capabilities(
    config: BackendConfig,
    deps: CapabilityProbeDeps,
) -> BackendCapabilities:
    """Probe backend capabilities for the resolved model/backends."""
    should_probe_embeddings = not (
        config.pipeline_mode == "staged" and config.staged_mode == "chat-only"
    )

    if config.chat_model is None:
        raise RuntimeError("Chat model resolution failed before capability probing.")

    chat_backend = deps.get_backend(config.chat_model_backend)
    if chat_backend is None:
        raise RuntimeError(
            f"Unsupported chat model backend for probing: {config.chat_model_backend}"
        )
    try:
        supports_chat = bool(chat_backend.probe_chat(config.chat_model))
    except ModelNotFoundError as err:
        raise RuntimeError(str(err)) from err
    except PROBE_EXCEPTIONS as err:
        raise RuntimeError(
            _format_probe_error(
                phase="chat",
                backend_name=config.chat_model_backend,
                model=config.chat_model,
                err=err,
            )
        ) from err
    except Exception as err:
        raise RuntimeError(
            _format_probe_error(
                phase="chat",
                backend_name=config.chat_model_backend,
                model=config.chat_model,
                err=err,
            )
        ) from err

    supports_embeddings = False
    if should_probe_embeddings:
        embedding_backend = deps.get_backend(config.embedding_model_backend)
        if embedding_backend is None:
            raise RuntimeError(
                "Unsupported embedding model backend for probing: "
                f"{config.embedding_model_backend}"
            )
        try:
            supports_embeddings = bool(embedding_backend.probe_embeddings(config.embedding_model))
        except ModelNotFoundError as err:
            deps.log_warning(
                "Embedding capability probe failed; "
                "continuing with chat-only fallback. "
                + _format_probe_error(
                    phase="embedding",
                    backend_name=config.embedding_model_backend,
                    model=config.embedding_model,
                    err=err,
                )
            )
            supports_embeddings = False
        except PROBE_EXCEPTIONS as err:
            deps.log_warning(
                "Embedding capability probe failed; "
                "continuing with chat-only fallback. "
                + _format_probe_error(
                    phase="embedding",
                    backend_name=config.embedding_model_backend,
                    model=config.embedding_model,
                    err=err,
                )
            )
            supports_embeddings = False

    capabilities = BackendCapabilities(
        supports_chat=supports_chat,
        supports_embeddings=supports_embeddings,
    )
    config.capabilities = capabilities
    return capabilities


def validate_mode(
    config: BackendConfig,
    log_warning: Callable[[str], None] | None = None,
) -> None:
    """Validate and reconcile pipeline/staged mode against probed capabilities."""
    if not config.capabilities.supports_chat:
        raise RuntimeError("Backend chat capability is required but unavailable.")

    if config.pipeline_mode == "legacy":
        return

    if config.pipeline_mode != "staged":
        raise RuntimeError(f"Unsupported pipeline mode: {config.pipeline_mode}")

    if config.staged_mode == "chat-only":
        config.capabilities.supports_embeddings = False
        return

    if config.staged_mode == "embedding-assisted":
        if not config.capabilities.supports_embeddings:
            warning_message = (
                "--staged-mode embedding-assisted requested but embeddings are "
                "unavailable; downgrading to chat-only."
            )
            if log_warning is not None:
                log_warning(warning_message)
            config.staged_mode = "chat-only"
        return

    config.staged_mode = (
        "embedding-assisted" if config.capabilities.supports_embeddings else "chat-only"
    )
