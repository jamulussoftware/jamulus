"""Shared logger for release_announcement runtime output."""

from __future__ import annotations

import sys


class GitOperationError(Exception):
    """Raised when git operations fail (commits, references, etc.)."""


class BackendValidationError(Exception):
    """Raised when backend validation fails (unknown backend, etc.)."""


class AppLogger:
    """Minimal level-based logger writing to stdout/stderr."""

    LEVELS = {
        "CRITICAL": 50,
        "ERROR": 40,
        "WARNING": 30,
        "INFO": 20,
        "DEBUG": 10,
        "TRACE": 5,
    }

    def __init__(self, level: str = "INFO") -> None:
        self.set_level(level)

    def set_level(self, level: str) -> None:
        """Set the active minimum log level."""
        self.level = self.LEVELS.get(level.upper(), 20)

    def log(self, level: str, message: str) -> None:
        """Emit a log line if it meets the active level threshold."""
        lvl = self.LEVELS.get(level.upper(), 20)
        if lvl < self.level:
            return
        stream = sys.stderr if lvl >= 30 else sys.stdout
        formatted_message = f"[{level.upper()}] {message}"
        print(formatted_message, file=stream)

    def critical(self, message: str) -> None:
        """Log a critical message."""
        self.log("CRITICAL", message)

    def error(self, message: str) -> None:
        """Log an error message."""
        self.log("ERROR", message)

    def warning(self, message: str) -> None:
        """Log a warning message."""
        self.log("WARNING", message)

    def info(self, message: str) -> None:
        """Log an informational message."""
        self.log("INFO", message)

    def debug(self, message: str) -> None:
        """Log a debug message."""
        self.log("DEBUG", message)

    def trace(self, message: str) -> None:
        """Log a trace message."""
        self.log("TRACE", message)


logger = AppLogger()
