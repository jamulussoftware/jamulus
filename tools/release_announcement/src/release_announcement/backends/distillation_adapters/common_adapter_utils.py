"""Common utilities for distillation adapters (Ollama, GitHub, etc)."""

from typing import Any
import json

def build_ranking_messages(ranking_prompts: list[dict[str, str]], chunks: list[Any]) -> list[dict[str, str]]:
    """Format ranking prompts and chunk text for LLM ranking."""
    chunks_text = "\n\n".join(f"[{i}] {chunk.text}" for i, chunk in enumerate(chunks))
    messages = []
    for prompt_msg in ranking_prompts:
        role = str(prompt_msg.get("role", ""))
        content = str(prompt_msg.get("content", ""))
        if role == "user":
            content = content.format(chunk_count=len(chunks), chunks=chunks_text)
        messages.append({"role": role, "content": content})
    return messages

def build_signal_payload(signals: list[Any]) -> str:
    """Serialize signals for LLM input."""
    return json.dumps([s.model_dump() for s in signals])
