"""Tests CLI backend capability resolution and default model selection behavior."""

from __future__ import annotations

import argparse
from dataclasses import dataclass

import pytest

from release_announcement import main as ra_main
from release_announcement.cli_config import build_arg_parser
from release_announcement.registry import ModelNotFoundError


def _parse(cli_args: list[str]) -> argparse.Namespace:
    parser = build_arg_parser()
    return parser.parse_args(cli_args + ["--file", "ReleaseAnnouncement.md", "pr3429"])


def _resolve(cli_args: list[str]) -> ra_main.BackendConfig:
    args = _parse(cli_args)
    return ra_main.resolve_backend_config(args)


def test_defaults_to_ollama_chat_and_no_embedding(monkeypatch: pytest.MonkeyPatch) -> None:
    """Use Ollama defaults when no backend or model overrides are provided."""
    monkeypatch.delenv("OLLAMA_MODEL", raising=False)
    config = _resolve([])

    assert config.backend == "ollama"
    assert config.chat_model == "mistral-large-3:675b-cloud"
    assert config.embedding_model is None
    assert config.chat_model_source == "backend-default"
    assert config.embedding_model_source == "backend-default"


def test_backend_ollama_defaults(monkeypatch: pytest.MonkeyPatch) -> None:
    """Resolve Ollama backend defaults when the backend is selected explicitly."""
    monkeypatch.delenv("OLLAMA_MODEL", raising=False)
    config = _resolve(["--backend", "ollama"])

    assert config.chat_model == "mistral-large-3:675b-cloud"
    assert config.embedding_model is None


def test_backend_github_defaults() -> None:
    """Resolve GitHub backend defaults for chat and embedding models."""
    config = _resolve(["--backend", "github"])

    assert config.chat_model == "openai/gpt-4o"
    assert config.embedding_model == "openai/text-embedding-3-small"


def test_ollama_chat_model_override(monkeypatch: pytest.MonkeyPatch) -> None:
    """Honor an explicit Ollama chat model override."""
    monkeypatch.delenv("OLLAMA_MODEL", raising=False)
    config = _resolve(["--backend", "ollama", "--chat-model", "mistral-large"])

    assert config.chat_model == "mistral-large"
    assert config.embedding_model is None


def test_ollama_embedding_model_override_with_prefix(monkeypatch: pytest.MonkeyPatch) -> None:
    """Strip the Ollama prefix when an embedding override is provided."""
    monkeypatch.delenv("OLLAMA_MODEL", raising=False)
    config = _resolve(["--backend", "ollama", "--embedding-model", "ollama/nomic-embed-text"])

    assert config.embedding_model == "nomic-embed-text"
    assert config.chat_model == "mistral-large-3:675b-cloud"


def test_github_embedding_model_override_from_ollama_backend(
        monkeypatch: pytest.MonkeyPatch) -> None:
    """Allow an embedding model override to switch to the GitHub backend."""
    monkeypatch.delenv("OLLAMA_MODEL", raising=False)
    config = _resolve(
        ["--backend", "ollama", "--embedding-model", "github/text-embedding-3-small"]
    )

    assert config.embedding_model == "text-embedding-3-small"
    assert config.chat_model == "mistral-large-3:675b-cloud"


def test_github_backend_with_ollama_chat_override() -> None:
    """Allow a GitHub backend selection with an Ollama chat model override."""
    config = _resolve(["--backend", "github", "--chat-model", "ollama/mistral-large"])

    assert config.chat_model == "mistral-large"
    assert config.embedding_model == "openai/text-embedding-3-small"
    assert config.chat_model_backend == "ollama"


def test_no_backend_with_chat_and_embedding_flags() -> None:
    """Default the backend to Ollama when only unprefixed model flags are provided."""
    config = _resolve(["--chat-model", "mistral-large", "--embedding-model", "nomic-embed-text"])

    assert config.backend == "ollama"
    assert config.chat_model == "mistral-large"
    assert config.embedding_model == "nomic-embed-text"


def test_no_backend_with_prefixed_models() -> None:
    """Resolve backend ownership from prefixed chat and embedding model values."""
    config = _resolve(
        [
            "--chat-model",
            "ollama/mistral-large",
            "--embedding-model",
            "github/text-embedding-3-small",
        ]
    )

    assert config.backend == "ollama"
    assert config.chat_model == "mistral-large"
    assert config.embedding_model == "text-embedding-3-small"
    assert config.chat_model_backend == "ollama"
    assert config.embedding_model_backend == "github"


def test_chat_prefix_only_uses_backend_default() -> None:
    """Use the backend default chat model when only a backend prefix is given."""
    config = _resolve(["--chat-model", "github/"])

    assert config.chat_model == "openai/gpt-4o"
    assert config.chat_model_backend == "github"


def test_embedding_prefix_only_uses_backend_default() -> None:
    """Use the backend default embedding model when only a backend prefix is given."""
    config = _resolve(["--embedding-model", "ollama/"])

    assert config.embedding_model is None
    assert config.embedding_model_backend == "ollama"


def test_pipeline_staged_ollama_embedding_model_is_resolved() -> None:
    """Resolve the embedding model override in staged Ollama mode."""
    config = _resolve(
        [
            "--pipeline",
            "staged",
            "--backend",
            "ollama",
            "--embedding-model",
            "ollama/nomic-embed-text",
        ]
    )

    assert config.embedding_model == "nomic-embed-text"
    assert config.embedding_model_backend == "ollama"


def test_pipeline_legacy_stores_embedding_model() -> None:
    """Preserve the embedding model override in legacy pipeline mode."""
    config = _resolve(["--pipeline", "legacy", "--embedding-model", "nomic-embed-text"])

    assert config.embedding_model == "nomic-embed-text"


def test_staged_mode_valid_with_default_staged_pipeline() -> None:
    """Allow --staged-mode when staged is the default pipeline mode."""
    parser = build_arg_parser()
    args = parser.parse_args([
        "--staged-mode",
        "chat-only",
        "--file",
        "ReleaseAnnouncement.md",
        "pr3429",
    ])

    ra_main.validate_cli_args(parser, args)


def test_chat_only_cannot_be_combined_with_embedding_model() -> None:
    """Reject embedding models when staged chat-only mode is requested."""
    parser = build_arg_parser()
    args = parser.parse_args([
        "--pipeline",
        "staged",
        "--staged-mode",
        "chat-only",
        "--embedding-model",
        "nomic-embed-text",
        "--file",
        "ReleaseAnnouncement.md",
        "pr3429",
    ])

    with pytest.raises(SystemExit):
        ra_main.validate_cli_args(parser, args)


def test_chat_only_skips_embedding_probe(monkeypatch: pytest.MonkeyPatch) -> None:
    """Skip embedding capability probes when chat-only staged mode is selected."""
    config = _resolve(
        ["--pipeline", "staged", "--staged-mode", "chat-only", "--backend", "ollama"]
    )
    calls: list[str] = []

    class _FakeBackend:
        def probe_chat(self, _model: str | None) -> bool:
            calls.append("chat")
            return True

        def probe_embeddings(self, _model: str | None) -> bool:
            calls.append("embed")
            return False

    monkeypatch.setattr(ra_main.registry, "get", lambda _name: _FakeBackend())

    caps = ra_main.probe_capabilities(config)

    assert calls == ["chat"]
    assert caps.supports_embeddings is False
    assert config.capabilities.supports_embeddings is False


def test_probe_capabilities_ollama_same_backend_probes_embedding_with_expected_args(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Probe Ollama embeddings in startup probe when chat and embeddings share backend."""
    config = _resolve(["--backend", "ollama", "--embed", "github"])
    calls: list[tuple[str, str | None]] = []

    class _FakeBackend:
        def probe_chat(self, model: str | None) -> bool:
            calls.append(("chat", model))
            return True

        def probe_embeddings(self, model: str | None) -> bool:
            calls.append(("embed", model))
            return True

    monkeypatch.setattr(ra_main.registry, "get", lambda _name: _FakeBackend())

    caps = ra_main.probe_capabilities(config)

    assert calls == [
        ("chat", "mistral-large-3:675b-cloud"),
        ("embed", "github"),
    ]
    assert caps.supports_chat is True
    assert caps.supports_embeddings is True


def test_probe_capabilities_cross_backend_makes_two_calls_with_expected_args(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Probe chat and embeddings separately when model backends differ."""
    config = _resolve(
        [
            "--backend",
            "ollama",
            "--embed",
            "github/text-embedding-3-small",
        ]
    )
    calls: list[tuple[str, str, str | None]] = []

    class _ChatBackend:
        def probe_chat(self, model: str | None) -> bool:
            calls.append(("chat", "ollama", model))
            return True

        def probe_embeddings(self, model: str | None) -> bool:
            calls.append(("embed", "ollama", model))
            return False

    class _EmbedBackend:
        def probe_chat(self, model: str | None) -> bool:
            calls.append(("chat", "github", model))
            return False

        def probe_embeddings(self, model: str | None) -> bool:
            calls.append(("embed", "github", model))
            return True

    def _get_backend(name: str):
        if name == "ollama":
            return _ChatBackend()
        if name == "github":
            return _EmbedBackend()
        return None

    monkeypatch.setattr(ra_main.registry, "get", _get_backend)

    caps = ra_main.probe_capabilities(config)

    assert calls == [
        ("chat", "ollama", "mistral-large-3:675b-cloud"),
        ("embed", "github", "text-embedding-3-small"),
    ]

    assert caps.supports_chat is True
    assert caps.supports_embeddings is True


@dataclass
class _ProbeError(RuntimeError):
    message: str
    endpoint: str | None = None
    request_payload: dict | None = None
    status_code: int | None = None
    headers: dict | None = None
    body: str | None = None

    def __str__(self) -> str:
        return self.message


@dataclass
class _GenericProbeError(Exception):
    message: str
    endpoint: str | None = None
    request_payload: dict | None = None
    status_code: int | None = None
    headers: dict | None = None
    body: str | None = None

    def __str__(self) -> str:
        return self.message


def test_probe_capabilities_chat_failure_raises_runtimeerror_with_full_diagnostics(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Chat probe failures must surface full request/response diagnostics."""
    config = _resolve(["--backend", "github"])

    class _FailingBackend:
        def probe_chat(self, _model: str | None) -> bool:
            raise _ProbeError(
                "GitHub Models HTTP error",
                endpoint="https://models.github.ai/inference/chat/completions",
                request_payload={
                    "model": "openai/gpt-4o",
                    "messages": [{"role": "user", "content": "Reply with 'ok'."}],
                },
                status_code=403,
                headers={"Content-Type": "application/json"},
                body='{"error":"forbidden"}',
            )

        def probe_embeddings(self, _model: str | None) -> bool:
            return True

    monkeypatch.setattr(ra_main.registry, "get", lambda _name: _FailingBackend())

    with pytest.raises(RuntimeError) as exc_info:
        ra_main.probe_capabilities(config)

    msg = str(exc_info.value)
    assert "phase=chat" in msg
    assert "backend=github" in msg
    assert "request_payload=" in msg
    assert "response_status=403" in msg
    assert "response_headers=" in msg
    assert "response_body={\"error\":\"forbidden\"}" in msg


def test_probe_capabilities_chat_non_runtime_exception_is_wrapped(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Non-RuntimeError backend probe exceptions should still be wrapped with diagnostics."""
    config = _resolve(["--backend", "github"])

    class _FailingBackend:
        def probe_chat(self, _model: str | None) -> bool:
            raise _GenericProbeError(
                "GitHub Models HTTP error",
                endpoint="https://models.github.ai/inference/chat/completions",
                request_payload={
                    "model": "openai/gpt-4o",
                    "messages": [{"role": "user", "content": "Reply with 'ok'."}],
                },
                status_code=403,
                headers={"Content-Type": "application/json"},
                body=(
                    '{"error":{"message":"Unable to proceed with model usage. '
                    'This account has reached its budget limit."}}'
                ),
            )

        def probe_embeddings(self, _model: str | None) -> bool:
            return True

    monkeypatch.setattr(ra_main.registry, "get", lambda _name: _FailingBackend())

    with pytest.raises(RuntimeError) as exc_info:
        ra_main.probe_capabilities(config)

    msg = str(exc_info.value)
    assert "phase=chat" in msg
    assert "backend=github" in msg
    assert "request_payload=" in msg
    assert "response_status=403" in msg
    assert "budget limit" in msg


def test_probe_capabilities_embedding_failure_warns_and_falls_back(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    """Embedding probe failures should warn and continue with embeddings disabled."""
    config = _resolve(["--backend", "github", "--embed", "ollama/github"])

    class _ChatBackend:
        def probe_chat(self, _model: str | None) -> bool:
            return True

        def probe_embeddings(self, _model: str | None) -> bool:
            return False

    class _FailingEmbedBackend:
        def probe_chat(self, _model: str | None) -> bool:
            return False

        def probe_embeddings(self, _model: str | None) -> bool:
            raise _ProbeError(
                "embed failed",
                endpoint="http://localhost:11434/api/embed",
                request_payload={"model": "github", "input": ["test"]},
                status_code=404,
                headers={"Content-Type": "application/json"},
                body='{"error":"model not found"}',
            )

    def _get_backend(name: str):
        if name == "github":
            return _ChatBackend()
        if name == "ollama":
            return _FailingEmbedBackend()
        return None

    monkeypatch.setattr(ra_main.registry, "get", _get_backend)

    caps = ra_main.probe_capabilities(config)
    captured = capsys.readouterr()
    out = captured.out + captured.err

    assert caps.supports_chat is True
    assert caps.supports_embeddings is False
    assert "Embedding capability probe failed" in out
    assert "phase=embedding" in out
    assert "request_payload=" in out
    assert "response_status=404" in out


def test_probe_capabilities_embedding_model_not_found_warns_and_falls_back(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    """Missing embedding models should warn and continue in chat-only mode."""
    config = _resolve(["--backend", "github", "--embed", "github/foo"])

    class _BackendWithMissingEmbeddingModel:
        def probe_chat(self, _model: str | None) -> bool:
            return True

        def probe_embeddings(self, _model: str | None) -> bool:
            raise ModelNotFoundError("model 'foo' not found on backend 'github'")

    monkeypatch.setattr(
        ra_main.registry,
        "get",
        lambda _name: _BackendWithMissingEmbeddingModel(),
    )

    caps = ra_main.probe_capabilities(config)
    captured = capsys.readouterr()
    out = captured.out + captured.err

    assert caps.supports_chat is True
    assert caps.supports_embeddings is False
    assert "Embedding capability probe failed" in out
    assert "phase=embedding" in out
    assert "model 'foo' not found on backend 'github'" in out
