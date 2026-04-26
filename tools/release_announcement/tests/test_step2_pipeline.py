"""Tests Step 2 staged preprocessing integration and legacy fallback behavior."""

from __future__ import annotations

import os
from pathlib import Path

import pytest

from release_announcement import main as ra_main
from release_announcement.distillation import ClassifiedSignals
from release_announcement.skip_rules import has_changelog_skip


def _sample_pr_data() -> dict:
    return {
        "number": 123,
        "title": "Sample PR",
        "body": "User visible change",
        "comments": ["Looks good", "Merged"],
        "reviews": ["Please rename field"],
    }


def test_prepare_pr_context_legacy_returns_none(capsys: pytest.CaptureFixture[str]) -> None:
    """Return no staged context when the legacy pipeline mode is requested."""
    config = ra_main.BackendConfig(pipeline_mode="legacy")

    context = ra_main.prepare_pr_context(_sample_pr_data(), config, "legacy")

    assert context is None
    assert "staged.preprocessing" not in capsys.readouterr().out


def test_process_single_pr_staged_falls_back_to_legacy_builder(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    """Fall back to the legacy builder when staged preprocessing yields no context."""
    ann_file = tmp_path / "ReleaseAnnouncement.md"
    ann_file.write_text("existing announcement", encoding="utf-8")

    pr_data = _sample_pr_data()
    build_calls: list[dict] = []

    monkeypatch.setattr(ra_main, "_fetch_pr_data", lambda *_args, **_kwargs: pr_data)

    def _capture_build(
        current_content: str,
        in_pr_data: dict,
        _template: dict,
        pipeline_mode: str = "legacy",
        distilled_context=None,
    ) -> dict:
        build_calls.append(
            {
                "content": current_content,
                "pr_data": in_pr_data,
                "pipeline_mode": pipeline_mode,
                "distilled_context": distilled_context,
            }
        )
        return {
            "messages": [{"role": "user", "content": "prompt"}],
            "model": "m",
            "modelParameters": {},
        }

    monkeypatch.setattr(ra_main, "_load_prompt_template",
                        lambda *_args, **_kwargs: {"messages": []})
    monkeypatch.setattr(ra_main, "_build_ai_prompt", _capture_build)
    monkeypatch.setattr(ra_main, "_process_with_llm", lambda *_args, **_kwargs: "updated")
    monkeypatch.setattr(ra_main, "_write_and_check_announcement", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(ra_main, "_check_for_changes", lambda *_args, **_kwargs: "no_changes")

    result = ra_main.process_single_pr(
        pr_num=123,
        pr_title="Sample PR",
        announcement_file=str(ann_file),
        prompt_file="unused.yml",
        config=ra_main.BackendConfig(backend="ollama", pipeline_mode="staged"),
    )

    assert result == "no_changes"
    assert build_calls
    assert build_calls[0]["pr_data"] == pr_data
    assert build_calls[0]["pipeline_mode"] == "staged"
    assert build_calls[0]["distilled_context"] is None

    captured = capsys.readouterr()
    output = captured.out + captured.err
    assert "staged preprocessing returned no context, falling back to legacy mode" in output


def test_prepare_pr_context_passes_loaded_phase_prompts_to_pipeline(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Load real stage prompt files and pass them through to the staged pipeline call."""
    captured: dict[str, object] = {}

    class _StubBackend:
        pass

    def _stub_get(_name: str):
        return _StubBackend()

    def _stub_run_distillation_pipeline(*, pr_data, adapter, backend_config, prompts):
        captured["pr_data"] = pr_data
        captured["adapter"] = adapter
        captured["backend_config"] = backend_config
        captured["prompts"] = prompts
        return ra_main.DistilledContext(
            summary="ok",
            structured_signals=[],
            classification={"classified": []},
            metadata={},
        )

    monkeypatch.setattr(ra_main.registry, "get", _stub_get)
    monkeypatch.setattr(ra_main, "run_distillation_pipeline", _stub_run_distillation_pipeline)

    config = ra_main.BackendConfig(backend="dummy", pipeline_mode="staged")
    context = ra_main.prepare_pr_context(_sample_pr_data(), config, "staged")

    assert context is not None
    assert context.summary == "ok"

    resolve_prompt_dir = getattr(ra_main, "_resolve_prompt_dir")
    load_prompts = getattr(ra_main, "_load_prompts")
    prompt_dir = resolve_prompt_dir()
    expected_extraction = load_prompts(
        str(os.path.join(prompt_dir, "extraction.prompt.yml"))
    )
    expected_consolidation = load_prompts(
        str(os.path.join(prompt_dir, "consolidation.prompt.yml"))
    )
    expected_classification = load_prompts(
        str(os.path.join(prompt_dir, "classification.prompt.yml"))
    )
    expected_ranking = load_prompts(str(os.path.join(prompt_dir, "ranking.prompt.yml")))

    assert "prompts" in captured
    prompts = captured["prompts"]
    assert prompts.extraction == expected_extraction
    assert prompts.consolidation == expected_consolidation
    assert prompts.classification == expected_classification
    assert prompts.ranking == expected_ranking


def test_prepare_pr_context_logs_elapsed_time_when_staged_pipeline_fails(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    """Log elapsed time on the handled no-context exit path too."""

    class _StubBackend:
        pass

    def _stub_get(_name: str):
        return _StubBackend()

    monotonic_values = iter([100.0, 101.25])

    def _stub_monotonic() -> float:
        return next(monotonic_values)

    def _stub_run_distillation_pipeline(**_kwargs):
        raise AttributeError("forced preprocessing failure")

    monkeypatch.setattr(ra_main.registry, "get", _stub_get)
    monkeypatch.setattr(ra_main, "run_distillation_pipeline", _stub_run_distillation_pipeline)
    monkeypatch.setattr(ra_main.time, "monotonic", _stub_monotonic)

    config = ra_main.BackendConfig(backend="dummy", pipeline_mode="staged")
    context = ra_main.prepare_pr_context(_sample_pr_data(), config, "staged")

    assert context is None

    output = capsys.readouterr().out
    assert "staged.preprocessing.end context=none elapsed=1.2s" in output


def test_prepare_pr_context_routes_chat_and_embedding_backends_independently(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Use embedding backend for embeddings ranking and chat backend otherwise."""

    class _ChatAdapter:
        def select_relevant_chunks(self, _chunks, use_embeddings, _ranking_prompts):
            if use_embeddings:
                raise AssertionError("chat adapter should not handle embeddings ranking")
            return []

        def extract_chunk_signals(self, _chunk, _extraction_prompts):
            return []

        def consolidate_signals(self, _signals, _consolidation_prompts):
            return []

        def classify_signals(self, _signals, _classification_prompts):
            return ClassifiedSignals(classified=[], summary="")

        def render_final_context(self, _classified, _metadata):
            return ra_main.DistilledContext(
                summary="chat",
                structured_signals=[],
                classification={"classified": []},
                metadata={},
            )

        def get_adapter_tag(self):
            return "ollama"

    class _EmbeddingAdapter(_ChatAdapter):
        def select_relevant_chunks(self, _chunks, use_embeddings, _ranking_prompts):
            if not use_embeddings:
                raise AssertionError("embedding adapter should only handle embeddings ranking")
            return []

        def get_adapter_tag(self):
            return "github"

    def _stub_get(name: str):
        if name == "ollama":
            return _ChatAdapter()
        if name == "github":
            return _EmbeddingAdapter()
        return None

    observed = {"embedding_selected": False, "chat_selected": False}

    def _stub_run_distillation_pipeline(*, adapter, **_kwargs):
        adapter.select_relevant_chunks([], True, [])
        observed["embedding_selected"] = True
        adapter.select_relevant_chunks([], False, [])
        observed["chat_selected"] = True
        return ra_main.DistilledContext(
            summary="ok",
            structured_signals=[],
            classification={"classified": []},
            metadata={},
        )

    monkeypatch.setattr(ra_main.registry, "get", _stub_get)
    monkeypatch.setattr(ra_main, "run_distillation_pipeline", _stub_run_distillation_pipeline)

    config = ra_main.BackendConfig(
        backend="ollama",
        chat_model_backend="ollama",
        embedding_model_backend="github",
        pipeline_mode="staged",
    )
    context = ra_main.prepare_pr_context(_sample_pr_data(), config, "staged")

    assert context is not None
    assert observed["embedding_selected"] is True
    assert observed["chat_selected"] is True


@pytest.mark.parametrize("backend", ["ollama", "github", "actions"])
def test_process_single_pr_staged_calls_prepare_for_all_backends(
    backend: str,
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Call staged preprocessing once for every supported staged backend."""
    ann_file = tmp_path / "ReleaseAnnouncement.md"
    ann_file.write_text("existing announcement", encoding="utf-8")

    monkeypatch.setattr(ra_main, "_fetch_pr_data", lambda *_args, **_kwargs: _sample_pr_data())
    monkeypatch.setattr(ra_main, "_load_prompt_template",
                        lambda *_args, **_kwargs: {"messages": []})
    monkeypatch.setattr(
        ra_main,
        "_build_ai_prompt",
        lambda *_args, **_kwargs: {
            "messages": [{"role": "user", "content": "prompt"}],
            "model": "m",
            "modelParameters": {},
        },
    )
    monkeypatch.setattr(ra_main, "_process_with_llm", lambda *_args, **_kwargs: "updated")
    monkeypatch.setattr(ra_main, "_write_and_check_announcement", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(ra_main, "_check_for_changes", lambda *_args, **_kwargs: "no_changes")

    calls = {"count": 0}

    def _stub_prepare(*_args, **_kwargs):
        calls["count"] += 1

    monkeypatch.setattr(ra_main, "prepare_pr_context", _stub_prepare)

    result = ra_main.process_single_pr(
        pr_num=123,
        pr_title="Sample PR",
        announcement_file=str(ann_file),
        prompt_file="unused.yml",
        config=ra_main.BackendConfig(backend=backend, pipeline_mode="staged"),
    )

    assert result == "no_changes"
    assert calls["count"] == 1


def test_build_ai_prompt_uses_staged_builder_when_context_present() -> None:
    """Route staged mode with context through the staged builder path."""
    prompt = {
        "messages": [{"role": "system", "content": "sys"}],
        "model": "m",
        "modelParameters": {},
    }
    pr_data = _sample_pr_data()
    distilled = ra_main.DistilledContext(
        summary="distilled summary",
        structured_signals=[{"signal": "a"}],
        classification={"classified": []},
        metadata={"stage": "ok"},
    )

    build_ai_prompt = getattr(ra_main, "_build_ai_prompt")
    result = build_ai_prompt(
        current_content="seed",
        pr_data=pr_data,
        prompt_template=prompt,
        pipeline_mode="staged",
        distilled_context=distilled,
    )

    assert result["messages"][0]["content"] == "sys"
    user_content = result["messages"][1]["content"]
    assert "Distilled pull request context" in user_content
    assert "distilled summary" in user_content
    assert "Newly merged pull request" not in user_content


def test_build_ai_prompt_uses_legacy_builder_when_context_absent() -> None:
    """Route to legacy prompt shape when staged mode has no distilled context."""
    prompt = {
        "messages": [{"role": "system", "content": "sys"}],
        "model": "m",
        "modelParameters": {},
    }
    pr_data = _sample_pr_data()

    build_ai_prompt = getattr(ra_main, "_build_ai_prompt")
    result = build_ai_prompt(
        current_content="seed",
        pr_data=pr_data,
        prompt_template=prompt,
        pipeline_mode="staged",
        distilled_context=None,
    )

    user_content = result["messages"][1]["content"]
    assert "Newly merged pull request" in user_content


def test_has_changelog_skip_matches_hosted_weblate_title() -> None:
    """Match the deterministic skip rule for Hosted Weblate translation updates."""
    pr_data = {
        "number": 999,
        "title": "Translations update from Hosted Weblate",
        "body": "",
        "comments": [],
        "reviews": [],
    }

    assert has_changelog_skip(pr_data) is True


def test_process_single_pr_skips_hosted_weblate_translation_updates(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Skip Hosted Weblate translation updates without invoking LLM processing."""
    ann_file = tmp_path / "ReleaseAnnouncement.md"
    ann_file.write_text("existing announcement", encoding="utf-8")

    pr_data = {
        "number": 1000,
        "title": "Translations update from Hosted Weblate",
        "body": "",
        "comments": [],
        "reviews": [],
    }

    monkeypatch.setattr(ra_main, "_fetch_pr_data", lambda *_args, **_kwargs: pr_data)

    def _fail_if_called(*_args, **_kwargs):
        raise AssertionError("LLM path should not run for Hosted Weblate translation updates")

    monkeypatch.setattr(ra_main, "_process_with_llm", _fail_if_called)

    result = ra_main.process_single_pr(
        pr_num=1000,
        pr_title=pr_data["title"],
        announcement_file=str(ann_file),
        prompt_file="unused.yml",
        config=ra_main.BackendConfig(backend="ollama", pipeline_mode="legacy"),
    )

    assert result == "skipped:Weblate translations"


def test_has_changelog_skip_matches_ci_action_version_bump_title() -> None:
    """Match deterministic skip rule for GitHub Action version bump PR titles."""
    pr_data = {
        "number": 1001,
        "title": "Build: Bump actions/upload-artifact from 6 to 7",
        "body": "",
        "comments": [],
        "reviews": [],
    }

    assert has_changelog_skip(pr_data) is True


def test_has_changelog_skip_matches_ci_action_version_bump_automated_pr_title() -> None:
    """Match deterministic skip rule for CI bump titles with automated suffix."""
    pr_data = {
        "number": 1003,
        "title": "Build: Bump actions/cache from v3 to v4 (Automated PR)",
        "body": "",
        "comments": [],
        "reviews": [],
    }

    assert has_changelog_skip(pr_data) is True


def test_process_single_pr_skips_ci_action_version_bumps(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Skip CI action version bump PRs without invoking LLM processing."""
    ann_file = tmp_path / "ReleaseAnnouncement.md"
    ann_file.write_text("existing announcement", encoding="utf-8")

    pr_data = {
        "number": 1002,
        "title": "Build: Bump owner/action from old to new",
        "body": "",
        "comments": [],
        "reviews": [],
    }

    monkeypatch.setattr(ra_main, "_fetch_pr_data", lambda *_args, **_kwargs: pr_data)

    def _fail_if_called(*_args, **_kwargs):
        raise AssertionError("LLM path should not run for CI action version bump PRs")

    monkeypatch.setattr(ra_main, "_process_with_llm", _fail_if_called)

    result = ra_main.process_single_pr(
        pr_num=1002,
        pr_title=pr_data["title"],
        announcement_file=str(ann_file),
        prompt_file="unused.yml",
        config=ra_main.BackendConfig(backend="ollama", pipeline_mode="legacy"),
    )

    assert result == "skipped:CI action version bump"


def test_process_single_pr_skips_ci_action_version_bumps_with_automated_suffix(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """Skip CI action bump PRs with '(Automated PR)' suffix."""
    ann_file = tmp_path / "ReleaseAnnouncement.md"
    ann_file.write_text("existing announcement", encoding="utf-8")

    pr_data = {
        "number": 1004,
        "title": "Build: Bump owner/action from old to new (Automated PR)",
        "body": "",
        "comments": [],
        "reviews": [],
    }

    monkeypatch.setattr(ra_main, "_fetch_pr_data", lambda *_args, **_kwargs: pr_data)

    def _fail_if_called(*_args, **_kwargs):
        raise AssertionError("LLM path should not run for CI action version bump PRs")

    monkeypatch.setattr(ra_main, "_process_with_llm", _fail_if_called)

    result = ra_main.process_single_pr(
        pr_num=1004,
        pr_title=pr_data["title"],
        announcement_file=str(ann_file),
        prompt_file="unused.yml",
        config=ra_main.BackendConfig(backend="ollama", pipeline_mode="legacy"),
    )

    assert result == "skipped:CI action version bump"


def test_has_changelog_skip_matches_non_slash_package_bump_title() -> None:
    """Match bump titles where the package name contains no owner/ prefix."""
    pr_data = {
        "number": 3617,
        "title": "Build: Bump ASIO-SDK from asiosdk_2.3.3_2019-06-14 to ASIO-SDK_2.3.4_2025-10-15 (Automated PR)",
        "body": "",
        "comments": [],
        "reviews": [],
    }

    assert has_changelog_skip(pr_data) is True
