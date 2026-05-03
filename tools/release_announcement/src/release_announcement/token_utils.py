"""Shared helpers for resolving GitHub tokens for CLI/API backends."""

from __future__ import annotations

import os
import subprocess
from typing import Callable


def normalize_github_token(raw_token: str) -> str:
    """Normalize token text from environment variables or gh output."""
    token = raw_token.replace("\r", "").replace("\n", "").strip()
    if token.lower().startswith("bearer "):
        token = token[7:].strip()

    if any(ch.isspace() for ch in token) or any(
        ord(ch) < 32 or ord(ch) == 127 for ch in token
    ):
        raise RuntimeError(
            "GitHub token contains whitespace/control characters after normalization. "
            "Set GH_TOKEN/GITHUB_TOKEN to the raw token value."
        )

    if not token:
        raise RuntimeError("GitHub token is empty after normalization.")

    return token


def resolve_token_from_env_or_gh(normalize: Callable[[str], str]) -> str:
    """Resolve GH token from env first, then from `gh auth token` fallback."""
    raw_token = os.getenv("GH_TOKEN") or os.getenv("GITHUB_TOKEN")
    if raw_token:
        return normalize(raw_token)

    try:
        gh_token = subprocess.check_output(
            ["gh", "auth", "token"], text=True, stderr=subprocess.STDOUT
        )
    except FileNotFoundError as exc:
        raise RuntimeError(
            "GH_TOKEN/GITHUB_TOKEN is not set and GitHub CLI ('gh') is not installed. "
            "Install gh or set GH_TOKEN/GITHUB_TOKEN."
        ) from exc
    except subprocess.CalledProcessError as err:
        details = (err.output or "").strip()
        if details:
            raise RuntimeError(
                "GH_TOKEN/GITHUB_TOKEN is not set and failed to run 'gh auth token'.\n"
                f"gh output: {details}"
            ) from err
        raise RuntimeError(
            "GH_TOKEN/GITHUB_TOKEN is not set and failed to run 'gh auth token'."
        ) from err

    return normalize(gh_token)


def resolve_github_token() -> str:
    """Resolve GH token from env first, then from `gh auth token` fallback."""
    return resolve_token_from_env_or_gh(normalize_github_token)
