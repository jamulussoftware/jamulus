"""Tests for the backend registry."""

from src.release_announcement.registry import registry


def test_registry_register_and_get():
    """Test that the registry can register and retrieve a backend."""
    # Create a minimal backend that implements the protocol
    class MinimalBackend:
        def probe_chat(self, model: str | None) -> bool:
            """Report chat support for the minimal backend stub."""
            # At least acknowledge the parameter exists
            if model is not None:
                _ = model  # Acknowledge parameter
            return True

        def probe_embeddings(self, model: str | None) -> bool:
            """Report that the minimal backend stub has no embedding support."""
            # At least acknowledge the parameter exists
            if model is not None:
                _ = model  # Acknowledge parameter
            return False

        def call_chat(self, prompt: dict) -> str:
            """Return a placeholder chat response for registry tests."""
            # At least acknowledge the parameter exists
            _ = len(prompt)  # Acknowledge parameter
            return "response"

    def create_minimal_backend():
        """Create a minimal backend instance for registry registration tests."""
        return MinimalBackend()

    registry.register("testBackend", create_minimal_backend)
    backend = registry.get("testBackend")
    assert isinstance(backend, MinimalBackend)


def test_registry_get_unknown():
    """Test that the registry returns None for unknown backends."""
    assert registry.get("unknownBackend") is None


def test_registry_duplicate_registration_replaces_backend():
    """Test that duplicate backend registration replaces the existing backend."""

    class FirstBackend:
        def probe_chat(self, _model: str | None) -> bool:
            """Report chat support for the first replacement backend."""
            return True

        def probe_embeddings(self, _model: str | None) -> bool:
            """Report no embedding support for the first replacement backend."""
            return False

        def call_chat(self, _prompt: dict) -> str:
            """Return the first backend marker response."""
            return "first"

    class SecondBackend:
        def probe_chat(self, _model: str | None) -> bool:
            """Report chat support for the second replacement backend."""
            return True

        def probe_embeddings(self, _model: str | None) -> bool:
            """Report no embedding support for the second replacement backend."""
            return False

        def call_chat(self, _prompt: dict) -> str:
            """Return the second backend marker response."""
            return "second"

    registry.register("replaceableBackend", FirstBackend)
    first = registry.get("replaceableBackend")
    assert first is not None
    assert first.call_chat({}) == "first"

    registry.register("replaceableBackend", SecondBackend)
    second = registry.get("replaceableBackend")
    assert second is not None
    assert second.call_chat({}) == "second"
    assert first is not second
