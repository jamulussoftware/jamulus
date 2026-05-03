"""CLI parsing and backend configuration resolution for release_announcement."""

from __future__ import annotations


import argparse
import os
from pathlib import Path
from dataclasses import dataclass, field

from .registry import registry


def _resolve_default_prompt_file() -> str:
    # Package-relative path works for editable (dev) installs.
    # CWD-relative path works when invoked via the shell script from the repo root.
    pkg_relative = (
        Path(__file__).resolve().parents[2]
        / "prompts"
        / "release-announcement.prompt.yml"
    )
    if pkg_relative.exists():
        return str(pkg_relative)
    return str(
        Path("tools") / "release_announcement" / "prompts" / "release-announcement.prompt.yml"
    )


DEFAULT_PROMPT_FILE = _resolve_default_prompt_file()


@dataclass
class BackendCapabilities:
    """Runtime capability probe result for the selected backend/models."""

    supports_chat: bool
    supports_embeddings: bool = False


@dataclass
class ModelSelection:
    """Resolved model state for one model role (chat or embedding)."""

    model: str | None
    backend: str
    source: str


@dataclass
class PipelineOptions:
    """CLI pipeline options that can vary independently from model selection."""

    dry_run: bool = False
    mode: str = "staged"
    staged_mode: str | None = None
    delay_secs: int = 30


@dataclass
class ModelRouting:
    """Resolved backend/source metadata for chat and embedding model selections."""

    chat_backend: str = "ollama"
    embedding_backend: str = "ollama"
    chat_source: str = "default"
    embedding_source: str = "default"


@dataclass(init=False)
class BackendConfig:
    """Resolved backend/model configuration and capability probe state."""

    backend: str = "ollama"
    chat_model: str | None = None
    embedding_model: str | None = None
    pipeline: PipelineOptions = field(default_factory=PipelineOptions)
    capabilities: BackendCapabilities = field(
        default_factory=lambda: BackendCapabilities(supports_chat=False, supports_embeddings=False)
    )
    routing: ModelRouting = field(default_factory=ModelRouting)

    def __init__(self, **kwargs: object) -> None:
        self.backend = str(kwargs.get("backend", "ollama"))
        self.chat_model = kwargs.get("chat_model") if isinstance(
            kwargs.get("chat_model"), str) or kwargs.get("chat_model") is None else None
        self.embedding_model = kwargs.get("embedding_model") if isinstance(
            kwargs.get("embedding_model"), str) or kwargs.get("embedding_model") is None else None

        dry_run = bool(kwargs.get("dry_run", False))
        pipeline_mode = str(kwargs.get("pipeline_mode", "staged"))
        staged_mode = kwargs.get("staged_mode")
        staged_mode = str(staged_mode) if isinstance(staged_mode, str) else None
        delay_secs = int(kwargs.get("delay_secs", 0))

        capabilities = kwargs.get("capabilities")
        chat_model_backend = str(kwargs.get("chat_model_backend", self.backend))
        embedding_model_backend = str(kwargs.get("embedding_model_backend", self.backend))
        chat_model_source = str(kwargs.get("chat_model_source", "default"))
        embedding_model_source = str(kwargs.get("embedding_model_source", "default"))

        pipeline = kwargs.get("pipeline")
        routing = kwargs.get("routing")
        self.pipeline = pipeline or PipelineOptions(
            dry_run=dry_run,
            mode=pipeline_mode,
            staged_mode=staged_mode,
            delay_secs=delay_secs,
        )
        self.capabilities = (
            capabilities
            if isinstance(capabilities, BackendCapabilities)
            else BackendCapabilities(
                supports_chat=False,
                supports_embeddings=False,
            )
        )
        self.routing = routing or ModelRouting(
            chat_backend=chat_model_backend,
            embedding_backend=embedding_model_backend,
            chat_source=chat_model_source,
            embedding_source=embedding_model_source,
        )

    @property
    def dry_run(self) -> bool:
        """Return whether dry-run mode is enabled."""
        return self.pipeline.dry_run

    @dry_run.setter
    def dry_run(self, value: bool) -> None:
        self.pipeline.dry_run = value

    @property
    def pipeline_mode(self) -> str:
        """Return the selected pipeline mode."""
        return self.pipeline.mode

    @pipeline_mode.setter
    def pipeline_mode(self, value: str) -> None:
        self.pipeline.mode = value

    @property
    def delay_secs(self) -> int:
        """Return the inter-PR delay in seconds."""
        return self.pipeline.delay_secs

    @delay_secs.setter
    def delay_secs(self, value: int) -> None:
        self.pipeline.delay_secs = value

    @property
    def staged_mode(self) -> str | None:
        """Return the selected staged mode override, if any."""
        return self.pipeline.staged_mode

    @staged_mode.setter
    def staged_mode(self, value: str | None) -> None:
        self.pipeline.staged_mode = value

    @property
    def chat_model_backend(self) -> str:
        """Return the backend used for chat model calls."""
        return self.routing.chat_backend

    @chat_model_backend.setter
    def chat_model_backend(self, value: str) -> None:
        self.routing.chat_backend = value

    @property
    def embedding_model_backend(self) -> str:
        """Return the backend used for embedding model calls."""
        return self.routing.embedding_backend

    @embedding_model_backend.setter
    def embedding_model_backend(self, value: str) -> None:
        self.routing.embedding_backend = value

    @property
    def chat_model_source(self) -> str:
        """Return where chat model selection came from."""
        return self.routing.chat_source

    @chat_model_source.setter
    def chat_model_source(self, value: str) -> None:
        self.routing.chat_source = value

    @property
    def embedding_model_source(self) -> str:
        """Return where embedding model selection came from."""
        return self.routing.embedding_source

    @embedding_model_source.setter
    def embedding_model_source(self, value: str) -> None:
        self.routing.embedding_source = value


def _parse_model_selector(raw: str, selected_backend: str |
                          None = None) -> tuple[str | None, str | None]:
    """Parse optional BACKEND/model syntax.

    The separator is '/'. A prefix before the first '/' is treated as the
    backend name. A trailing '/' with no model (e.g. 'github/') selects that
    backend's default model. Without a '/' prefix, a bare known backend name
    (e.g. '--embed github') is a backend-only selector. Anything else
    (including Ollama model tags like 'mistral-large-3:675b-cloud') is treated
    as a plain model name under the default backend. If selected_backend is
    None and no prefix is found, returns None for backend.
    """
    if "/" in raw:
        prefix, remainder = raw.split("/", 1)
        # 'github/' → (github, None); 'github/model' → (github, model)
        return prefix, remainder if remainder else None

    # No '/' — treat as a plain model name under the default backend.
    return selected_backend, raw


def _resolve_chat_selection(args: argparse.Namespace) -> ModelSelection:
    if args.chat_model is None:
        return ModelSelection(model=None, backend=args.backend, source="default")

    backend, model = _parse_model_selector(args.chat_model, args.backend)
    return ModelSelection(model=model, backend=backend, source="flag")


def _resolve_embedding_selection(args: argparse.Namespace) -> ModelSelection:
    if args.embedding_model is None:
        return ModelSelection(model=None, backend=args.backend, source="default")

    backend, model = _parse_model_selector(args.embedding_model, args.backend)
    return ModelSelection(model=model, backend=backend, source="flag")


def resolve_backend_config(args: argparse.Namespace) -> BackendConfig:
    """Resolve parsed CLI args into a full backend configuration object."""
    chat = _resolve_chat_selection(args)
    embedding = _resolve_embedding_selection(args)

    # Resolve backend names with defaults via registry
    resolved_chat_backend = registry.resolve_backend_name(chat.backend)
    resolved_embedding_backend = registry.resolve_backend_name(embedding.backend)
    primary_backend = registry.resolve_backend_name(args.backend)

    # If no chat model is provided, use the backend's default
    if chat.model is None:
        chat_backend = registry.get(resolved_chat_backend)
        chat_model = (
            getattr(chat_backend, "_default_chat_model", None)
            if chat_backend
            else None
        )
        chat_model_source = "backend-default"
    else:
        chat_model = chat.model
        chat_model_source = chat.source

    # If no embedding model is provided, use the backend's default
    if embedding.model is None:
        embedding_backend = registry.get(resolved_embedding_backend)
        embedding_model = (
            getattr(embedding_backend, "_default_embedding_model", None)
            if embedding_backend
            else None
        )
        embedding_model_source = "backend-default"
    else:
        embedding_model = embedding.model
        embedding_model_source = embedding.source

    return BackendConfig(
        backend=primary_backend,
        chat_model=chat_model,
        embedding_model=embedding_model,
        pipeline=PipelineOptions(
            dry_run=args.dry_run,
            mode=args.pipeline,
            staged_mode=args.staged_mode,
            delay_secs=args.delay_secs,
        ),
        routing=ModelRouting(
            chat_backend=resolved_chat_backend,
            embedding_backend=resolved_embedding_backend,
            chat_source=chat_model_source,
            embedding_source=embedding_model_source,
        ),
    )


def validate_cli_args(parser: argparse.ArgumentParser, args: argparse.Namespace) -> None:
    """Validate cross-argument invariants before startup probing or processing."""
    if args.staged_mode and args.pipeline != "staged":
        parser.error("--staged-mode is only valid when --pipeline staged is set.")

    if args.staged_mode == "chat-only" and args.embedding_model:
        parser.error("--staged-mode chat-only cannot be combined with --embedding-model.")


def build_arg_parser() -> argparse.ArgumentParser:
    """Build and return the command-line argument parser."""
    prog = os.environ.get("RELEASE_ANNOUNCEMENT_PROG")
    parser = argparse.ArgumentParser(
        description="Progressive Release Announcement Generator",
        prog=prog,
    )
    parser.add_argument(
        "start",
        help="Starting boundary, or upper bound if end is omitted"
        " (e.g. pr3409 or v3.11.0)",
    )
    parser.add_argument(
        "end",
        nargs="?",
        help="Ending boundary (e.g. pr3500 or HEAD). Defaults to start if omitted.",
    )
    parser.add_argument("--file", required=True, help="Markdown file to update")
    parser.add_argument(
        "--prompt",
        default=DEFAULT_PROMPT_FILE,
        help="YAML prompt template file",
    )
    parser.add_argument(
        "--backend",
        default="ollama",
        help=(
            "Default model provider when no provider prefix is given in --chat-model or "
            "--embedding-model. Accepts any registered provider name; defaults to 'ollama'. "
            "Affects both model selection and capability probing."
        ),
    )
    parser.add_argument(
        "--chat-model",
        "--model",
        dest="chat_model",
        default=None,
        help=(
            "Chat model (alias: --model). Supports an optional provider prefix separated "
            "by '/' (e.g. 'github/gpt-4o', 'ollama/mistral-large-3:675b-cloud'). "
            "Without a prefix the provider from --backend is used. "
            "Omit entirely to use that provider's default model. "
            "Examples: --chat-model gpt-4o, --chat-model github/gpt-4o, "
            "--model ollama/mistral-large-3:675b-cloud"
        ),
    )
    parser.add_argument(
        "--embedding-model",
        "--embed",
        dest="embedding_model",
        default=None,
        help=(
            "Embedding model (alias: --embed). Supports the same optional provider prefix "
            "syntax as --chat-model, separated by '/' "
            "(e.g. 'github/text-embedding-3-small', 'ollama/all-minilm'). "
            "Without a prefix the provider from --backend is used. "
            "A trailing '/' with no model name (e.g. '--embed github/') uses that "
            "provider's default embedding model. "
            "Omit entirely if embeddings are not required. "
            "Examples: --embed all-minilm, --embed github/, "
            "--embedding-model github/text-embedding-3-small"
        ),
    )
    parser.add_argument(
        "--delay-secs",
        type=int,
        default=int(os.getenv("DELAY_SECS", "30")),
        help="Seconds to sleep before each PR is processed",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show which PRs would be processed/skipped without calling the LLM",
    )
    parser.add_argument(
        "--pipeline",
        default="staged",
        choices=["legacy", "staged"],
        help=(
            "Preprocessing pipeline mode. "
            "'legacy': single-shot — raw PR data sent directly to the LLM in one call. "
            "'staged': progressive distillation — PRs are chunked, extracted, consolidated, "
            "and classified before the final LLM call (default)."
        ),
    )
    parser.add_argument(
        "--staged-mode",
        default=None,
        choices=["chat-only", "embedding-assisted"],
        help=(
            "Staged-mode execution preference. Valid only with --pipeline staged. "
            "If unset, mode is auto-selected from runtime capabilities."
        ),
    )
    parser.add_argument(
        "--log-level",
        default="INFO",
        choices=["CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "TRACE"],
        help="Set log verbosity: CRITICAL, ERROR, WARNING, INFO, DEBUG, TRACE (default: INFO)",
    )
    return parser
