"""Deterministic PR skip rules used before any LLM processing."""

from __future__ import annotations

import re
from typing import Any

_CHANGELOG_SKIP_RE = re.compile(r"(?m)^CHANGELOG:\s*SKIP\s*$", re.IGNORECASE)
_TRANSLATION_UPDATE_SKIP_RE = re.compile(
    r"^Translations update from Hosted Weblate$",
    re.IGNORECASE,
)
_CI_ACTION_BUMP_SKIP_RE = re.compile(
    r"^Build:\s+Bump\s+\S+\s+from\s+\S+\s+to\s+\S+"
    r"(?:\s+\(Automated PR\))?$",
    re.IGNORECASE,
)

_SKIP_REASON_CHANGELOG = "CHANGELOG: SKIP"
_SKIP_REASON_WEBLATE = "Weblate translations"
_SKIP_REASON_CI_ACTION_BUMP = "CI action version bump"


def get_skip_reason(pr_data: dict[str, Any]) -> str | None:
    """Return skip reason when a deterministic skip rule matches PR content.

    Current skip rules:
    - PR body or comments contain ``CHANGELOG: SKIP``.
    - PR title equals ``Translations update from Hosted Weblate``.
    - PR title matches ``Build: Bump <package> from <old> to <new>``
      optionally followed by ``(Automated PR)``.
    """
    text_fields = [pr_data.get("body") or ""] + list(pr_data.get("comments", []))
    if any(_CHANGELOG_SKIP_RE.search(field) for field in text_fields if field):
        return _SKIP_REASON_CHANGELOG

    title = str(pr_data.get("title") or "").strip()
    if _TRANSLATION_UPDATE_SKIP_RE.fullmatch(title):
        return _SKIP_REASON_WEBLATE

    if _CI_ACTION_BUMP_SKIP_RE.fullmatch(title):
        return _SKIP_REASON_CI_ACTION_BUMP

    return None


def has_changelog_skip(pr_data: dict[str, Any]) -> bool:
    """Backward-compatible boolean helper for skip-rule checks."""
    return get_skip_reason(pr_data) is not None
