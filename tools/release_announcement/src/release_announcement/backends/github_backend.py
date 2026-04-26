"""GitHub Models backend for release-announcement generation."""

import json
import os
import sys
import urllib.error
import urllib.request
from dataclasses import dataclass
from typing import Any, Callable
import math
from ..app_logger import logger

_embedding_support_cache: dict[str, bool] = {}
"""Per-process cache so we only ping the embeddings endpoint once per model."""

DEFAULT_BACKEND_CONFIG: dict[str, Any] = {
    "default_chat_model": "openai/gpt-4o",
    "default_embedding_model": "openai/text-embedding-3-small",
    "chat_endpoint": "https://models.github.ai/inference/chat/completions",
    "embeddings_endpoint": "https://models.github.ai/inference/embeddings",
    "token_limit": 7500,
}


@dataclass(frozen=True)
class GitHubCallOptions:
    """Optional overrides and backend config for chat completion requests."""

    chat_model_override: str | None = None
    embedding_model_override: str | None = None
    backend_config: dict[str, Any] | None = None


@dataclass(frozen=True)
class GitHubProbeRequest:
    """All values required to probe chat/embedding support for GitHub Models."""

    chat_model: str | None
    embedding_model: str | None
    chat_endpoint: str
    embeddings_endpoint: str
    probe_embeddings: bool


def _estimate_tokens(text: str) -> int:
    """Conservative token estimate for budget trimming before API calls."""
    # Use a stricter 3-chars-per-token approximation and round up so we
    # under-shoot model limits less often on large PR payloads.
    return max(1, math.ceil(len(text) / 3))


def _probe_embedding_support(model: str, token: str, embeddings_endpoint: str) -> bool:
    """Return True when a model supports embeddings at the configured endpoint."""
    if model in _embedding_support_cache:
        return _embedding_support_cache[model]

    payload = {"model": model, "input": ["ping"]}
    req = urllib.request.Request(
        embeddings_endpoint,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            resp.read()
        supported = True
    except (urllib.error.HTTPError, urllib.error.URLError):
        supported = False

    _embedding_support_cache[model] = supported
    support_text = "supported" if supported else "not supported"
    logger.info(f"  [budget] Embeddings probe ({model}): {support_text}")
    return supported


def _call_github_embeddings(
    texts: list[str], model: str, token: str, embeddings_endpoint: str
) -> list[list[float]]:
    """Call the GitHub Models embeddings endpoint and return one vector per input text."""
    payload = {"model": model, "input": texts}
    req = urllib.request.Request(
        embeddings_endpoint,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            body = resp.read().decode("utf-8")
    except urllib.error.HTTPError as err:
        error_body = err.read().decode("utf-8", errors="replace")
        raise RuntimeError(
            "Embeddings API call failed during semantic chunk selection. "
            f"endpoint={embeddings_endpoint} model={model} "
            f"http={err.code} body={error_body}"
        ) from err
    except urllib.error.URLError as err:
        raise RuntimeError(
            "Embeddings API connection error during semantic chunk selection. "
            f"endpoint={embeddings_endpoint} model={model} error={err}"
        ) from err

    data = json.loads(body)
    return [item["embedding"] for item in sorted(data["data"], key=lambda x: x["index"])]


def _cosine_similarity(a: list[float], b: list[float]) -> float:
    """Cosine similarity between two vectors (pure Python, no numpy required)."""
    dot = sum(x * y for x, y in zip(a, b))
    mag_a = math.sqrt(sum(x * x for x in a))
    mag_b = math.sqrt(sum(x * x for x in b))
    return dot / (mag_a * mag_b) if mag_a and mag_b else 0.0


def _extract_pr_section_from_user_text(user_text: str) -> tuple[str, str, str] | None:
    """Split user message into preamble, PR block, and suffix."""
    split_marker = "\nNewly merged pull request:\n"
    marker_idx = user_text.find(split_marker)
    if marker_idx == -1:
        return None

    preamble = user_text[: marker_idx + len(split_marker)]
    rest = user_text[marker_idx + len(split_marker):]

    json_end = rest.find("\n====\n")
    if json_end == -1:
        return preamble, rest, ""
    return preamble, rest[:json_end], rest[json_end:]


def _parse_pr_payload_from_user_text(user_text: str) -> tuple[str, dict[str, Any], str] | None:
    """Parse PR payload from a prompt user message."""
    parts = _extract_pr_section_from_user_text(user_text)
    if parts is None:
        return None

    preamble, pr_json_str, suffix = parts
    try:
        pr_data = json.loads(pr_json_str)
    except json.JSONDecodeError:
        return None
    return preamble, pr_data, suffix


def _collect_pr_text_chunks(
    pr_data: dict[str, Any],
) -> tuple[dict[str, Any], str, list[str], list[str], list[tuple[str, str]]]:
    """Extract header/body/comments/reviews and embeddable chunks from PR data."""
    pr_header = {"number": pr_data.get("number"), "title": pr_data.get("title")}
    body: str = pr_data.get("body") or ""
    comments: list[str] = pr_data.get("comments") or []
    reviews: list[str] = pr_data.get("reviews") or []

    chunks: list[tuple[str, str]] = []
    if body.strip():
        chunks.append(("body", body))
    chunks.extend((f"comment_{i}", c) for i, c in enumerate(comments) if c and c.strip())
    chunks.extend((f"review_{i}", r) for i, r in enumerate(reviews) if r and r.strip())
    return pr_header, body, comments, reviews, chunks


def _pick_relevant_chunk_labels(selection: dict[str, Any]) -> set[str]:
    """Return labels for chunks selected by embedding similarity within the token budget."""
    embed_trunc = 4000
    chunks = selection["chunks"]
    vectors = _call_github_embeddings(
        [selection["query_text"][:embed_trunc]]
        + [text[:embed_trunc] for _, text in chunks],
        selection["model"],
        selection["token"],
        selection["embeddings_endpoint"],
    )
    scored = sorted(
        enumerate(chunks),
        key=lambda ic: _cosine_similarity(vectors[0], vectors[ic[0] + 1]),
        reverse=True,
    )

    selected: set[str] = set()
    used_tokens = 0
    for _, (label, text) in scored:
        tokens = _estimate_tokens(text)
        if used_tokens + tokens <= selection["budget"]:
            selected.add(label)
            used_tokens += tokens
    return selected


def _build_trimmed_pr_payload(
    pr_header: dict[str, Any],
    body: str,
    comments: list[str],
    reviews: list[str],
    selected: set[str],
) -> dict[str, Any]:
    """Rebuild PR payload preserving original field order with selected text chunks."""
    trimmed: dict[str, Any] = dict(pr_header)
    trimmed["body"] = body if "body" in selected else ""
    trimmed["comments"] = [c for i, c in enumerate(comments) if f"comment_{i}" in selected]
    trimmed["reviews"] = [r for i, r in enumerate(reviews) if f"review_{i}" in selected]
    return trimmed


def _replace_user_message_content(
    messages: list[dict[str, Any]], new_content: str
) -> list[dict[str, Any]]:
    """Return a copy of messages with the user message content replaced."""
    return [{**m, "content": new_content} if m["role"] == "user" else m for m in messages]


def _prepare_embedding_trim_context(
    prompt: dict[str, Any], token_limit: int
) -> dict[str, Any] | None:
    """Prepare parsed prompt data needed for embedding-based trimming."""
    messages = prompt["messages"]
    system_text = next((m["content"] for m in messages if m["role"] == "system"), "")
    user_text = next((m["content"] for m in reversed(messages) if m["role"] == "user"), None)
    if user_text is None:
        return None

    parsed = _parse_pr_payload_from_user_text(user_text)
    if parsed is None:
        return None

    preamble, pr_data, suffix = parsed
    pr_header, body, comments, reviews, chunks = _collect_pr_text_chunks(pr_data)
    if not chunks:
        return None

    fixed_tokens = (
        _estimate_tokens(preamble)
        + _estimate_tokens(json.dumps(pr_header, indent=2))
        + _estimate_tokens(suffix)
        + 200
    )
    return {
        "messages": messages,
        "user_text": user_text,
        "query_text": (system_text[:300] + " " + pr_header.get("title", "")).strip(),
        "budget": token_limit - fixed_tokens,
        "preamble": preamble,
        "suffix": suffix,
        "pr_header": pr_header,
        "body": body,
        "comments": comments,
        "reviews": reviews,
        "chunks": chunks,
    }


def _chunk_text_for_summarization(text: str, chunk_chars: int) -> list[str]:
    """Split text into near-size chunks preferring newline boundaries."""
    parts: list[str] = []
    remaining = text
    while len(remaining) > chunk_chars:
        split_at = remaining.rfind("\n", 0, chunk_chars) or chunk_chars
        parts.append(remaining[:split_at])
        remaining = remaining[split_at:]
    if remaining.strip():
        parts.append(remaining)
    return parts


def _summarise_chunk_via_chat(
    chunk: str,
    model: str,
    token: str,
    endpoint: str,
    chunk_chars: int,
) -> str:
    """Summarise one chunk using chat completions, falling back to truncation on failures."""
    payload = {
        "model": model,
        "messages": [
            {
                "role": "system",
                "content": (
                    "Summarise the following portion of a pull request discussion in "
                    "2-4 sentences, focusing on user-visible changes and key technical "
                    "context. Be concise and factual."
                ),
            },
            {"role": "user", "content": chunk},
        ],
        "max_completion_tokens": 256,
    }
    req = urllib.request.Request(
        endpoint,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            decoded = json.loads(resp.read().decode())
            return decoded["choices"][0]["message"]["content"].strip()
    except urllib.error.HTTPError as err:
        error_body = err.read().decode("utf-8", errors="replace")
        logger.warning(
            "  [budget] Chunk summarisation failed during chat fallback "
            f"(endpoint={endpoint}, model={model}, http={err.code}, body={error_body}) "
            "— keeping truncated text"
        )
    except urllib.error.URLError as err:
        logger.warning(
            "  [budget] Chunk summarisation connection failure during chat fallback "
            f"(endpoint={endpoint}, model={model}, error={err}) "
            "— keeping truncated text"
        )
    except (KeyError, json.JSONDecodeError) as err:
        logger.warning(
            "  [budget] Chunk summarisation parsing failed during chat fallback "
            f"(endpoint={endpoint}, model={model}, error={err}) "
            "— keeping truncated text"
        )
    return chunk[:chunk_chars]


def _execute_github_chat_completion(
    endpoint: str,
    payload: dict[str, Any],
    token: str,
    model: str,
) -> str:
    """Execute chat-completion request and return response text or exit with rich diagnostics."""
    req = urllib.request.Request(
        endpoint,
        data=json.dumps(payload).encode("utf-8"),
        headers={
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            body = resp.read().decode("utf-8")
    except urllib.error.HTTPError as err:
        error_body = err.read().decode("utf-8", errors="replace")
        logger.error("GitHub Models chat completion failed.")
        logger.error("  Operation: final release-announcement generation call")
        logger.error(f"  Endpoint: {endpoint}")
        logger.error(f"  Model: {model}")
        logger.error(f"  HTTP: {err.code}")
        logger.error(f"  Response body: {error_body}")
        if "unsupported" in error_body.lower():
            logger.error(
                "  Hint: This model likely does not support chat completions on this endpoint. "
                "Use a chat-capable model (for example openai/gpt-4o) for --model."
            )
        sys.exit(1)
    except urllib.error.URLError as err:
        logger.error(
            "GitHub Models chat completion request failed before "
            "a response was received."
        )
        logger.error("  Operation: final release-announcement generation call")
        logger.error(f"  Endpoint: {endpoint}")
        logger.error(f"  Model: {model}")
        logger.error(f"  network_error={err}")
        sys.exit(1)

    try:
        data = json.loads(body)
        content = data.get("choices", [{}])[0].get("message", {}).get("content", "")
    except (json.JSONDecodeError, IndexError, TypeError) as err:
        logger.error(f"Invalid response from GitHub Models API: {err}")
        sys.exit(1)

    if not content:
        logger.error("GitHub Models API returned an empty response.")
        sys.exit(1)
    return content.strip()


def _trim_prompt_via_embeddings(
    prompt: dict[str, Any],
    model: str,
    token: str,
    backend_config: dict[str, Any],
) -> dict[str, Any]:
    context = _prepare_embedding_trim_context(prompt, backend_config["token_limit"])
    if context is None:
        return prompt

    try:
        selected = _pick_relevant_chunk_labels(
            {
                "chunks": context["chunks"],
                "query_text": context["query_text"],
                "model": model,
                "token": token,
                "budget": context["budget"],
                "embeddings_endpoint": backend_config["embeddings_endpoint"],
            }
        )
    except RuntimeError as err:
        logger.warning(f"  [budget] Embedding call failed ({err}) — falling back to summarisation")
        return _summarize_pr_chunks(prompt, model, token, backend_config)

    trimmed = _build_trimmed_pr_payload(
        context["pr_header"],
        context["body"],
        context["comments"],
        context["reviews"],
        selected,
    )
    new_pr_json = json.dumps(trimmed, indent=2)
    new_user_content = context["preamble"] + new_pr_json + context["suffix"]
    logger.info(
        f"  [budget] Embedding trim: ~{_estimate_tokens(context['user_text'])} → "
        f"~{_estimate_tokens(new_user_content)} tokens "
        f"(kept {len(selected)}/{len(context['chunks'])} PR text chunks)"
    )
    new_messages = _replace_user_message_content(context["messages"], new_user_content)
    return {**prompt, "messages": new_messages}


def _summarize_pr_chunks(
    prompt: dict[str, Any],
    model: str,
    token: str,
    backend_config: dict[str, Any],
) -> dict[str, Any]:
    """Trim an over-budget prompt by summarising the PR discussion in chunks."""
    messages = prompt["messages"]
    user_text = next((m["content"] for m in reversed(messages) if m["role"] == "user"), None)
    if user_text is None:
        return prompt

    parts = _extract_pr_section_from_user_text(user_text)
    if parts is None:
        return prompt

    preamble, pr_block, suffix = parts
    effective_chat_endpoint = os.getenv("MODELS_ENDPOINT", backend_config["chat_endpoint"])
    chunk_chars = 3000 * 4

    parts_to_summarize = _chunk_text_for_summarization(pr_block, chunk_chars)
    if len(parts_to_summarize) <= 1:
        return prompt

    logger.info(f"  [budget] Chat-only: summarising PR in {len(parts_to_summarize)} chunks...")
    condensed = "\n\n".join(
        _summarise_chunk_via_chat(chunk, model, token, effective_chat_endpoint, chunk_chars)
        for chunk in parts_to_summarize
    )
    if _estimate_tokens(preamble + condensed + suffix) > backend_config["token_limit"]:
        logger.info("  [budget] Still over budget - consolidating summaries...")
        condensed = _summarise_chunk_via_chat(
            condensed, model, token, effective_chat_endpoint, chunk_chars
        )
    logger.info(
        f"  [budget] Summarisation trim: ~{_estimate_tokens(user_text)} → "
        f"~{_estimate_tokens(preamble + condensed + suffix)} tokens"
    )
    new_messages = _replace_user_message_content(messages, preamble + condensed + suffix)
    return {**prompt, "messages": new_messages}


def call_github_models_api(
    prompt: dict[str, Any],
    resolve_token: Callable[[], str],
    options: GitHubCallOptions | None = None,
) -> str:
    """Call GitHub Models API with token-budget-aware prompt reduction."""
    options = options or GitHubCallOptions()
    config = DEFAULT_BACKEND_CONFIG | (options.backend_config or {})
    token = resolve_token()

    configured_chat_model = prompt.get("model", config["default_chat_model"])
    chat_model = configured_chat_model
    embedding_model = config["default_embedding_model"]

    if options.chat_model_override:
        chat_model = options.chat_model_override
    if options.embedding_model_override:
        embedding_model = options.embedding_model_override

    all_text = " ".join(m.get("content", "") for m in prompt.get("messages", []))
    estimated = _estimate_tokens(all_text)
    if estimated > config["token_limit"]:
        logger.info(
            f"  [budget] Prompt ~{estimated} tokens exceeds "
            f"{config['token_limit']} limit - reducing..."
        )
        if _probe_embedding_support(embedding_model, token, config["embeddings_endpoint"]):
            prompt = _trim_prompt_via_embeddings(
                prompt,
                embedding_model,
                token,
                config,
            )
        else:
            prompt = _summarize_pr_chunks(prompt, chat_model, token, config)

    endpoint = os.getenv("MODELS_ENDPOINT", config["chat_endpoint"])
    model_parameters = prompt.get("modelParameters", {})
    payload: dict[str, Any] = {"model": chat_model, "messages": prompt["messages"]}
    if "maxCompletionTokens" in model_parameters:
        payload["max_completion_tokens"] = model_parameters["maxCompletionTokens"]
    if "temperature" in model_parameters:
        payload["temperature"] = model_parameters["temperature"]
    return _execute_github_chat_completion(endpoint, payload, token, chat_model)


def _probe_request(
    endpoint: str,
    payload: dict[str, Any],
    token: str,
) -> dict[str, Any]:
    """Execute a probe request and return decoded JSON, raising rich RuntimeError on failures."""
    req = urllib.request.Request(
        endpoint,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            raw = resp.read().decode("utf-8")
            return json.loads(raw)
    except urllib.error.HTTPError as err:
        body = err.read().decode("utf-8", errors="replace")
        headers = dict(err.headers.items()) if err.headers else {}
        raise RuntimeError(
            "GitHub Models capability probe HTTP failure. "
            f"request_payload={json.dumps(payload, ensure_ascii=True)} "
            f"response_status={err.code} "
            f"response_headers={json.dumps(headers, ensure_ascii=True)} "
            f"response_body={body}"
        ) from err
    except urllib.error.URLError as err:
        raise RuntimeError(
            "GitHub Models capability probe network failure. "
            f"request_payload={json.dumps(payload, ensure_ascii=True)} "
            f"error={err}"
        ) from err
    except json.JSONDecodeError as err:
        raise RuntimeError(
            "GitHub Models capability probe returned non-JSON response. "
            f"request_payload={json.dumps(payload, ensure_ascii=True)} "
            f"error={err}"
        ) from err


def probe_capabilities(
    token: str,
    request: GitHubProbeRequest,
) -> dict[str, bool]:
    """Probe GitHub Models chat/embedding support for selected models."""
    supports_chat = False
    if request.chat_model is not None:
        chat_payload = {
            "model": request.chat_model,
            "messages": [{"role": "user", "content": "Reply with 'ok'."}],
            "max_completion_tokens": 8,
        }
        chat_response = _probe_request(request.chat_endpoint, chat_payload, token)
        content = (
            chat_response.get("choices", [{}])[0]
            .get("message", {})
            .get("content", "")
        )
        if not isinstance(content, str) or not content.strip():
            raise RuntimeError(
                "GitHub Models chat capability probe returned empty content. "
                f"request_payload={json.dumps(chat_payload, ensure_ascii=True)} "
                f"response_body={json.dumps(chat_response, ensure_ascii=True)}"
            )
        supports_chat = True

    supports_embeddings = False
    if request.probe_embeddings and request.embedding_model is not None:
        embed_payload = {"model": request.embedding_model, "input": ["test"]}
        try:
            embed_response = _probe_request(request.embeddings_endpoint, embed_payload, token)
            data = embed_response.get("data") or []
            vector = data[0].get("embedding") if data and isinstance(data[0], dict) else None
            supports_embeddings = isinstance(vector, list) and bool(vector)
            if not supports_embeddings:
                raise RuntimeError(
                    "GitHub Models embeddings probe returned no embedding vector. "
                    f"request_payload={json.dumps(embed_payload, ensure_ascii=True)} "
                    f"response_body={json.dumps(embed_response, ensure_ascii=True)}"
                )
        except RuntimeError as err:
            logger.warning(
                "GitHub embeddings capability probe failed; "
                "continuing with chat-only staged mode. "
                f"{err}"
            )
            supports_embeddings = False

    return {"supports_chat": supports_chat, "supports_embeddings": supports_embeddings}
