"""Unit tests for Substep 4a: Pydantic schemas, dataclasses, and parsing helpers.

Tests the type foundation of the staged distillation pipeline:
- Signal, ClassifiedSignal, ClassifiedSignals Pydantic models
- Chunk and DistilledContextMetadata dataclasses
- JSON parsing helpers (_parse_signal_list, _parse_classified_signals)
"""

import json
import pytest
from pydantic import ValidationError

from src.release_announcement.distillation import (
    Signal,
    ClassifiedSignal,
    ClassifiedSignals,
    Chunk,
    DistilledContextMetadata,
    _parse_signal_list,
    _parse_classified_signals,
)


# ============================================================================
# Tests for Signal Pydantic Model
# ============================================================================


def test_signal_creation_valid():
    """Test creating a valid Signal."""
    signal = Signal(
        change="Fixed memory leak in audio buffer",
        impact="high",
        users_affected="all server operators",
        confidence="high",
        final_outcome=True,
    )

    assert signal.change == "Fixed memory leak in audio buffer"
    assert signal.impact == "high"
    assert signal.users_affected == "all server operators"
    assert signal.confidence == "high"
    assert signal.final_outcome is True


def test_signal_required_fields():
    """Test that Signal requires all fields."""
    with pytest.raises(ValidationError):
        Signal(
            change="Fix",
            impact="high",
            # Missing users_affected, confidence, final_outcome
        )


def test_signal_field_types():
    """Test that Signal validates field types."""
    # final_outcome should be bool
    signal = Signal(
        change="Fix",
        impact="high",
        users_affected="all",
        confidence="high",
        final_outcome=True,
    )
    assert isinstance(signal.final_outcome, bool)

    # Pydantic should coerce string "true" to bool
    signal2 = Signal(
        change="Fix",
        impact="high",
        users_affected="all",
        confidence="high",
        final_outcome="true",  # type: ignore
    )
    assert signal2.final_outcome is True


def test_signal_extra_fields_allowed():
    """Test that Signal allows extra fields (extra='allow' in Config)."""
    signal = Signal(
        change="Fix",
        impact="high",
        users_affected="all",
        confidence="high",
        final_outcome=True,
        extra_field="allowed",  # type: ignore
    )
    assert signal.change == "Fix"
    # Extra field is stored in __pydantic_extra__ or similar


# ============================================================================
# Tests for ClassifiedSignal Pydantic Model
# ============================================================================


def test_classified_signal_creation_valid():
    """Test creating a valid ClassifiedSignal."""
    signal = Signal(
        change="Fixed bug",
        impact="high",
        users_affected="users",
        confidence="high",
        final_outcome=True,
    )

    classified = ClassifiedSignal(signal=signal, category="major")

    assert classified.signal == signal
    assert classified.category == "major"


def test_classified_signal_valid_categories():
    """Test all valid category values."""
    signal = Signal(
        change="Fix",
        impact="low",
        users_affected="none",
        confidence="low",
        final_outcome=False,
    )

    for category in ["internal", "minor", "targeted", "major", "no_user_facing_changes"]:
        classified = ClassifiedSignal(signal=signal, category=category)  # type: ignore
        assert classified.category == category


def test_classified_signal_invalid_category():
    """Test that invalid categories are rejected."""
    signal = Signal(
        change="Fix",
        impact="low",
        users_affected="none",
        confidence="low",
        final_outcome=False,
    )

    with pytest.raises(ValidationError):
        ClassifiedSignal(signal=signal, category="invalid_category")  # type: ignore


# ============================================================================
# Tests for ClassifiedSignals Pydantic Model
# ============================================================================


def test_classified_signals_empty():
    """Test creating ClassifiedSignals with empty list (no user-facing changes)."""
    result = ClassifiedSignals(classified=[], summary="No user-facing changes")

    assert len(result.classified) == 0
    assert result.summary == "No user-facing changes"


def test_classified_signals_with_items():
    """Test ClassifiedSignals with multiple classified items."""
    signal1 = Signal(
        change="Fix A",
        impact="high",
        users_affected="users",
        confidence="high",
        final_outcome=True,
    )
    signal2 = Signal(
        change="Fix B",
        impact="low",
        users_affected="some",
        confidence="medium",
        final_outcome=False,
    )

    classified1 = ClassifiedSignal(signal=signal1, category="major")
    classified2 = ClassifiedSignal(signal=signal2, category="minor")

    result = ClassifiedSignals(
        classified=[classified1, classified2],
        summary="Two changes detected",
    )

    assert len(result.classified) == 2
    assert result.classified[0].category == "major"
    assert result.classified[1].category == "minor"


def test_classified_signals_defaults():
    """Test ClassifiedSignals with default values."""
    result = ClassifiedSignals()

    assert result.classified == []
    assert result.summary == ""


# ============================================================================
# Tests for Chunk Dataclass
# ============================================================================


def test_chunk_creation_minimal():
    """Test creating Chunk with minimal required fields."""
    chunk = Chunk(text="Some discussion text")

    assert chunk.text == "Some discussion text"
    assert chunk.source == "unknown"
    assert chunk.relevance_score == 0.0
    assert chunk.chunk_index == 0


def test_chunk_creation_full():
    """Test creating Chunk with all fields."""
    chunk = Chunk(
        text="Important change",
        source="pr_100_comment_5",
        relevance_score=0.85,
        chunk_index=3,
    )

    assert chunk.text == "Important change"
    assert chunk.source == "pr_100_comment_5"
    assert chunk.relevance_score == 0.85
    assert chunk.chunk_index == 3


def test_chunk_mutability():
    """Test that Chunk fields can be modified."""
    chunk = Chunk(text="Original")
    chunk.relevance_score = 0.95
    chunk.source = "new_source"

    assert chunk.relevance_score == 0.95
    assert chunk.source == "new_source"


# ============================================================================
# Tests for DistilledContextMetadata Dataclass
# ============================================================================


def test_distilled_context_metadata_defaults():
    """Test DistilledContextMetadata with default values."""
    metadata = DistilledContextMetadata()

    assert metadata.pr_number == 0
    assert metadata.total_chunks == 0
    assert metadata.selected_chunks == 0
    assert metadata.extraction_phase_duration_ms == 0.0
    assert metadata.consolidation_phase_duration_ms == 0.0
    assert metadata.classification_phase_duration_ms == 0.0


def test_distilled_context_metadata_with_values():
    """Test DistilledContextMetadata with explicit values."""
    metadata = DistilledContextMetadata(
        pr_number=3502,
        total_chunks=10,
        selected_chunks=7,
        extraction_phase_duration_ms=125.5,
        consolidation_phase_duration_ms=89.3,
        classification_phase_duration_ms=45.2,
    )

    assert metadata.pr_number == 3502
    assert metadata.total_chunks == 10
    assert metadata.selected_chunks == 7
    assert metadata.extraction_phase_duration_ms == 125.5


# ============================================================================
# Tests for _parse_signal_list Helper
# ============================================================================


def test_parse_signal_list_valid_json_array():
    """Test parsing valid JSON array of signals."""
    json_text = json.dumps([
        {
            "change": "Fix A",
            "impact": "high",
            "users_affected": "all",
            "confidence": "high",
            "final_outcome": True,
        },
        {
            "change": "Fix B",
            "impact": "low",
            "users_affected": "some",
            "confidence": "medium",
            "final_outcome": False,
        },
    ])

    signals = _parse_signal_list(json_text)

    assert len(signals) == 2
    assert signals[0].change == "Fix A"
    assert signals[1].change == "Fix B"
    assert isinstance(signals[0], Signal)


def test_parse_signal_list_with_code_block():
    """Test parsing signals from ```json code block."""
    response = """
    Here are the extracted signals:

    ```json
    [
        {
            "change": "Added feature",
            "impact": "medium",
            "users_affected": "users",
            "confidence": "high",
            "final_outcome": true
        }
    ]
    ```

    This is the end of the response.
    """

    signals = _parse_signal_list(response)

    assert len(signals) == 1
    assert signals[0].change == "Added feature"


def test_parse_signal_list_with_generic_code_block():
    """Test parsing signals from generic ``` code block."""
    response = """
    Results:

    ```
    [
        {
            "change": "Fix",
            "impact": "low",
            "users_affected": "none",
            "confidence": "low",
            "final_outcome": false
        }
    ]
    ```
    """

    signals = _parse_signal_list(response)

    assert len(signals) == 1
    assert signals[0].change == "Fix"


def test_parse_signal_list_empty_array():
    """Test parsing empty signal array."""
    json_text = json.dumps([])

    signals = _parse_signal_list(json_text)

    assert not signals


def test_parse_signal_list_invalid_json():
    """Test that invalid JSON raises ValueError."""
    with pytest.raises(ValueError, match="Failed to parse response as JSON"):
        _parse_signal_list("{not valid json")


def test_parse_signal_list_non_array():
    """Test that non-array JSON raises ValueError."""
    json_text = json.dumps({"signal": "not an array"})

    with pytest.raises(ValueError, match="Expected JSON array at top level"):
        _parse_signal_list(json_text)


def test_parse_signal_list_invalid_signal_schema():
    """Test that signals not matching schema raise ValueError."""
    json_text = json.dumps([
        {
            "change": "Fix",
            # Missing required fields
        }
    ])

    with pytest.raises(ValueError, match="Signal validation failed"):
        _parse_signal_list(json_text)


# ============================================================================
# Tests for _parse_classified_signals Helper
# ============================================================================


def test_parse_classified_signals_valid_json():
    """Test parsing valid ClassifiedSignals JSON."""
    json_text = json.dumps({
        "classified": [
            {
                "signal": {
                    "change": "Fix A",
                    "impact": "high",
                    "users_affected": "all",
                    "confidence": "high",
                    "final_outcome": True,
                },
                "category": "major",
            }
        ],
        "summary": "One major change",
    })

    result = _parse_classified_signals(json_text)

    assert len(result.classified) == 1
    assert result.classified[0].category == "major"
    assert result.summary == "One major change"


def test_parse_classified_signals_empty():
    """Test parsing empty ClassifiedSignals (no user-facing changes)."""
    json_text = json.dumps({
        "classified": [],
        "summary": "No user-facing changes",
    })

    result = _parse_classified_signals(json_text)

    assert len(result.classified) == 0
    assert result.summary == "No user-facing changes"


def test_parse_classified_signals_with_code_block():
    """Test parsing ClassifiedSignals from ```json code block."""
    response = """
    Classification result:

    ```json
    {
        "classified": [
            {
                "signal": {
                    "change": "Feature X",
                    "impact": "high",
                    "users_affected": "users",
                    "confidence": "high",
                    "final_outcome": true
                },
                "category": "major"
            }
        ],
        "summary": "One major feature addition"
    }
    ```
    """

    result = _parse_classified_signals(response)

    assert len(result.classified) == 1
    assert result.classified[0].signal.change == "Feature X"


def test_parse_classified_signals_invalid_json():
    """Test that invalid JSON raises ValueError."""
    with pytest.raises(ValueError, match="Failed to parse response as JSON"):
        _parse_classified_signals("{invalid json")


def test_parse_classified_signals_non_object():
    """Test that non-object JSON raises ValueError."""
    json_text = json.dumps(["not", "an", "object"])

    with pytest.raises(ValueError, match="Expected JSON object at top level"):
        _parse_classified_signals(json_text)


def test_parse_classified_signals_invalid_category():
    """Test that invalid category raises ValueError."""
    json_text = json.dumps({
        "classified": [
            {
                "signal": {
                    "change": "Fix",
                    "impact": "low",
                    "users_affected": "none",
                    "confidence": "low",
                    "final_outcome": False,
                },
                "category": "invalid_category",
            }
        ],
        "summary": "Invalid",
    })

    with pytest.raises(ValueError):
        _parse_classified_signals(json_text)


# ============================================================================
# Integration Tests
# ============================================================================


def test_signal_to_classified_to_result_flow():
    """Integration test: Signal → ClassifiedSignal → ClassifiedSignals."""
    signals = [
        Signal(
            change="Change 1",
            impact="high",
            users_affected="users",
            confidence="high",
            final_outcome=True,
        ),
        Signal(
            change="Change 2",
            impact="low",
            users_affected="some",
            confidence="low",
            final_outcome=False,
        ),
    ]

    classified_signals = [
        ClassifiedSignal(signal=signals[0], category="major"),
        ClassifiedSignal(signal=signals[1], category="minor"),
    ]

    result = ClassifiedSignals(
        classified=classified_signals,
        summary="Two changes: one major, one minor",
    )

    assert len(result.classified) == 2
    assert result.classified[0].signal.change == "Change 1"
    assert result.classified[0].category == "major"


def test_parse_and_classify_full_flow():
    """Test parsing signal list, then creating classified signals."""
    # First: extract signals from LLM response
    extraction_response = """
    ```json
    [
        {
            "change": "Improved audio codec",
            "impact": "high",
            "users_affected": "all",
            "confidence": "high",
            "final_outcome": true
        }
    ]
    ```
    """

    extracted = _parse_signal_list(extraction_response)
    assert len(extracted) == 1

    # Second: classify the signals
    classification_response = """
    ```json
    {
        "classified": [
            {
                "signal": {
                    "change": "Improved audio codec",
                    "impact": "high",
                    "users_affected": "all",
                    "confidence": "high",
                    "final_outcome": true
                },
                "category": "major"
            }
        ],
        "summary": "Major improvement to audio handling"
    }
    ```
    """

    classified = _parse_classified_signals(classification_response)
    assert len(classified.classified) == 1
    assert classified.classified[0].category == "major"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
