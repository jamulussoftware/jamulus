"""Shared test data builders for distillation-related unit tests."""

from __future__ import annotations

from src.release_announcement.distillation import DistilledContextMetadata


def sample_metadata_123() -> DistilledContextMetadata:
    """Return a canonical metadata object used across adapter tests."""
    return DistilledContextMetadata(
        pr_number=123,
        total_chunks=10,
        selected_chunks=5,
        extraction_phase_duration_ms=100,
        consolidation_phase_duration_ms=50,
        classification_phase_duration_ms=30,
    )
