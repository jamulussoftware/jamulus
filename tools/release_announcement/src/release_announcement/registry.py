"""Backend registry and protocol definitions for release_announcement."""

from __future__ import annotations

import importlib
import pkgutil
from typing import Callable, Protocol, runtime_checkable



class ModelNotFoundError(Exception):
    """Raised when the backend reports the model name is invalid."""


@runtime_checkable
class BackendProtocol(Protocol):
    """Abstract interface that all backends must implement."""

    def probe_chat(self, model: str | None) -> bool:
        """Send a minimal chat request to verify the backend is reachable with this model.

        Args:
            model: The model name to probe. If None, the backend uses its own internal default.

        Returns:
            True on success.

        Raises:
            ModelNotFoundError: When the backend reports the model name is invalid.
            Any other exception (network, auth, etc.) propagates as-is.
        """

    def probe_embeddings(self, model: str | None) -> bool:
        """Send a minimal embedding request to verify embedding support for this model.

        Args:
            model: The model name to probe. If None, the backend uses its own internal default.

        Returns:
            True on success, False when the backend does not support embeddings as a capability.

        Raises:
            ModelNotFoundError: When the model name is invalid.
            Any other exception (network, auth, etc.) propagates as-is.
        """

    def call_chat(self, prompt: dict) -> str:
        """Call the chat model with the given prompt.

        Args:
            prompt: The prompt to send to the chat model.

        Returns:
            The response from the chat model.
        """


class BackendRegistry:
    """Registry for backends implementing BackendProtocol."""

    def __init__(self) -> None:
        self._backends: dict[str, BackendProtocol] = {}
        self._factories: dict[str, Callable[[], BackendProtocol]] = {}
        self._default_backend = "ollama"
        self._bootstrapped = False

    def _bootstrap_distillation_adapters(self) -> None:
        """Import distillation adapter modules once in deterministic order.

        Adapter modules register themselves via module import side effects.
        """
        if self._bootstrapped:
            return

        package_root = __name__.rsplit(".", maxsplit=1)[0]
        package_name = f"{package_root}.backends.distillation_adapters"
        try:
            package = importlib.import_module(package_name)
        except ImportError as err:
            raise RuntimeError(
                "Failed to import distillation adapter package "
                f"'{package_name}': {err}"
            ) from err

        module_names = sorted(
            name
            for _, name, _ in pkgutil.iter_modules(package.__path__)
            if not name.startswith("_")
        )
        for name in module_names:
            importlib.import_module(f"{package_name}.{name}")

        self._bootstrapped = True

    def register(self, name: str, factory: Callable[[], BackendProtocol]) -> None:
        """Register a backend factory function for lazy initialization.

        The factory is called only when the backend is first requested, and the
        instance is cached for subsequent lookups. Re-registering a name
        replaces the existing factory and invalidates any cached instance.
        """
        if name in self._backends:
            del self._backends[name]
        self._factories[name] = factory

    def get(self, name: str) -> BackendProtocol | None:
        """Get a backend by name, lazily initializing factories on first access."""
        self._bootstrap_distillation_adapters()

        # Check if already instantiated
        if name in self._backends:
            return self._backends[name]

        # Check if there's a factory for lazy initialization
        if name in self._factories:
            factory = self._factories[name]
            backend = factory()
            # Cache the instantiated backend
            self._backends[name] = backend
            return backend

        return None

    def resolve_backend_name(self, name: str | None) -> str:
        """Resolve a backend name, using the default if name is None."""
        return name if name is not None else self._default_backend


# Global registry instance
registry = BackendRegistry()
