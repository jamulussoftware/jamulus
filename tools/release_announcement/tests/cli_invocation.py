"""Shared CLI command builders for release_announcement integration tests."""

from __future__ import annotations

import sys


def build_release_announcement_cmd(
    backend: str,
    pipeline: str,
    start: str,
    end: str | None = None,
    dry_run: bool = False,
) -> list[str]:
    """Build a release_announcement module invocation command."""
    cmd = [
        sys.executable,
        "-m",
        "release_announcement",
        "--delay-secs",
        "0",
        "--backend",
        backend,
        "--pipeline",
        pipeline,
        "--file",
        "ReleaseAnnouncement.md",
        start,
    ]
    if end:
        cmd.append(end)
    if dry_run:
        cmd.append("--dry-run")
    return cmd
