#!/usr/bin/env python3
##############################################################################
# Copyright (c) 2026
#
# Author(s):
#  The Jamulus Development Team
#
##############################################################################
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
##############################################################################

"""
Given a range of git history, iterate over PR merges and
update the specified release announcement file.

This script supports multiple LLM backends:
- Ollama (local models, --backend ollama)
- GitHub Models API for CLI use (--backend github, resolves token via env or 'gh auth token')
- GitHub Models API for workflow use (--backend actions, requires GITHUB_TOKEN in step env)
"""

import argparse
import json
import os
import re
import subprocess
import sys
import time
from datetime import datetime
from typing import Any, cast

import yaml

from .cli_config import (
    BackendCapabilities,
    BackendConfig,
    build_arg_parser,
    resolve_backend_config as resolve_backend_config_impl,
    validate_cli_args as validate_cli_args_impl,
)
from .capability_probing import (
    CapabilityProbeDeps,
    probe_capabilities as probe_capabilities_impl,
    validate_mode as validate_mode_impl,
)
from .app_logger import logger, GitOperationError, BackendValidationError
from .registry import registry
from .token_utils import normalize_github_token, resolve_github_token
from .distillation import DistilledContext, DistillationPrompts, run_distillation_pipeline
from .distillation import PARSE_EXCEPTIONS
from .skip_rules import get_skip_reason
from .staged_routing_adapter import StagedRoutingAdapter
from .backends.ollama_backend import (
    call_ollama_model,
)
from .backends.github_backend import (
    GitHubCallOptions,
    call_github_models_api as call_github_models_api_backend,
)

GITHUB_MODELS_DEFAULT_CHAT_MODEL = "openai/gpt-4o"
GITHUB_MODELS_DEFAULT_EMBEDDING_MODEL = "openai/text-embedding-3-small"
GITHUB_MODELS_CHAT_ENDPOINT = "https://models.github.ai/inference/chat/completions"
GITHUB_MODELS_EMBEDDINGS_ENDPOINT = "https://models.github.ai/inference/embeddings"
GITHUB_MODELS_CHAT_TOKEN_LIMIT = 7500
"""Conservative token budget for a single GitHub Models chat-completions call."""

_PR_MERGE_RE = re.compile(r"^Merge pull request #(\d+) from ")
_upstream_repo_cache: dict = {}
_current_repo_name_cache: dict = {}
STAGED_PREPROCESS_EXCEPTIONS = PARSE_EXCEPTIONS + (OSError, AttributeError)
HANDLED_MAIN_EXCEPTIONS = (
    GitOperationError,
    BackendValidationError,
    FileNotFoundError,
    yaml.YAMLError,
    RuntimeError,
)
UNEXPECTED_MAIN_EXCEPTIONS = (
    ValueError,
    TypeError,
    KeyError,
    IndexError,
    OSError,
    AttributeError,
)

def get_gh_auth_env() -> dict[str, str]:
    """Get subprocess env with normalized token for gh CLI calls if provided."""
    env = os.environ.copy()
    raw_token = os.getenv("GH_TOKEN") or os.getenv("GITHUB_TOKEN")
    if not raw_token:
        return env

    token = normalize_github_token(raw_token)
    # Set both vars so gh uses a clean value consistently.
    env["GH_TOKEN"] = token
    env["GITHUB_TOKEN"] = token
    return env


def get_universal_timestamp(identifier: str) -> str:
    """
    Resolves an identifier to an ISO8601 timestamp.
    Supports: pr123, Tags, SHAs, and Branches.
    """
    target = resolve_identifier_to_git_target(identifier)

    # 2. Get authoritative Git timestamp for the target
    try:
        return subprocess.check_output(
            ["git", "show", "-s", "--format=%cI", target], text=True
        ).strip()
    except subprocess.CalledProcessError as err:
        raise GitOperationError(
            f"Could not resolve '{target}' as a Git object: {err}"
        ) from err


def _get_upstream_repo() -> str | None:
    """
    Return the parent repository's 'owner/name' string for the current repo,
    or None if the current repo has no parent (i.e. it IS the canonical upstream).
    Result is cached after the first call.
    """
    if "value" not in _upstream_repo_cache:
        try:
            raw = subprocess.check_output(
                ["gh", "repo", "view", "--json", "parent"],
                text=True,
                env=get_gh_auth_env(),
            )
            parent = json.loads(raw).get("parent")
            if parent:
                owner = parent["owner"]["login"]
                name = parent["name"]
                _upstream_repo_cache["value"] = f"{owner}/{name}"
            else:
                _upstream_repo_cache["value"] = None
        except (subprocess.CalledProcessError, json.JSONDecodeError, KeyError):
            _upstream_repo_cache["value"] = None
    return _upstream_repo_cache["value"]


def _get_current_repo_name() -> str | None:
    """
    Return the current repo's 'owner/name' string (e.g. 'jamulussoftware/jamulus').
    Used when the current repo has no parent (i.e. it is the canonical upstream).
    Result is cached after the first call.
    """
    if "value" not in _current_repo_name_cache:
        try:
            result = subprocess.check_output(
                ["gh", "repo", "view", "--json", "nameWithOwner", "--jq", ".nameWithOwner"],
                text=True,
                env=get_gh_auth_env(),
            )
            _current_repo_name_cache["value"] = result.strip() or None
        except (subprocess.CalledProcessError, OSError):
            _current_repo_name_cache["value"] = None
    return _current_repo_name_cache["value"]


def _fetch_inline_review_comments(pr_num: int, repo: str) -> list[str]:
    """
    Fetch inline code-review thread comments (pull request review comments) via
    the GitHub REST API.  These are tied to specific lines of code and are NOT
    returned by 'gh pr view --json reviews', which only carries the top-level
    review submission body (usually empty for code-only reviews).
    """
    env = get_gh_auth_env()
    bodies: list[str] = []
    page = 1
    while True:
        try:
            raw = subprocess.check_output(
                [
                    "gh", "api",
                    f"/repos/{repo}/pulls/{pr_num}/comments?per_page=100&page={page}",
                ],
                text=True,
                env=env,
            )
        except subprocess.CalledProcessError:
            break
        items = json.loads(raw)
        if not items:
            break
        bodies.extend(item["body"] for item in items if item.get("body"))
        if len(items) < 100:
            break
        page += 1
    return bodies


def resolve_identifier_to_git_target(identifier: str) -> str:
    """
    Resolve an identifier to a git object SHA or ref.

    Non-pr identifiers (tags, SHAs, branches) are passed straight through to
    git — no network call needed.

    pr<N> identifiers refer to a merged PR in the upstream repository.  The
    upstream is discovered automatically via 'gh repo view --json parent'; if
    the current repository has no parent it is itself the upstream.
    """
    if identifier.lower().startswith("pr"):
        pr_id = identifier[2:]
        upstream = _get_upstream_repo()
        repo_desc = f"upstream {upstream}" if upstream else "current repo"
        logger.info(f"   > Resolving PR #{pr_id} via {repo_desc} ...")
        try:
            cmd = ["gh", "pr", "view", pr_id, "--json", "mergeCommit", "--jq", ".mergeCommit.oid"]
            if upstream:
                cmd += ["--repo", upstream]
            target = subprocess.check_output(cmd, text=True, env=get_gh_auth_env()).strip()
            if not target or target == "null":
                raise GitOperationError(f"PR #{pr_id} has no merge commit (is it merged?)")
            return target
        except subprocess.CalledProcessError as err:
            raise GitOperationError(
                f"Failed to fetch PR #{pr_id} from {repo_desc}: {err}"
            ) from err

    return identifier


def get_previous_commit_timestamp(identifier: str) -> str:
    """Get the timestamp of the commit immediately before the resolved target."""
    target = resolve_identifier_to_git_target(identifier)
    previous_target = f"{target}~"

    try:
        return subprocess.check_output(
            ["git", "show", "-s", "--format=%cI", previous_target], text=True
        ).strip()
    except subprocess.CalledProcessError as err:
        raise GitOperationError(
            f"Could not resolve previous commit for '{identifier}': {err}"
        ) from err


def parse_iso_datetime(value: str) -> datetime:
    """Parse ISO8601 timestamps from Git/GitHub into timezone-aware datetime."""
    return datetime.fromisoformat(value.replace("Z", "+00:00"))


def get_ordered_pr_list(start_iso: str, end_iso: str) -> list:
    """
    Find PRs merged in (start_iso, end_iso] from the local git log.
    Parses 'Merge pull request #N' merge commits so it works correctly
    regardless of which repository the workflow runs in (no GitHub API used).
    Returns list of dicts with 'number', 'title', 'mergedAt', oldest-first.
    """
    sep = "\x1e"  # ASCII record-separator; safe as git log field delimiter
    log_output = subprocess.check_output(
        ["git", "log", "--merges", f"--format=tformat:{sep}%cI%n%s%n%b"], text=True
    )

    start_dt = parse_iso_datetime(start_iso)
    end_dt = parse_iso_datetime(end_iso)

    prs = []
    for record in log_output.split(sep):
        record = record.strip()
        if not record:
            continue
        lines = record.splitlines()
        if len(lines) < 2:
            continue

        committed_at = lines[0].strip()
        subject = lines[1].strip()
        match = _PR_MERGE_RE.match(subject)
        if not match:
            continue

        try:
            committed_dt = parse_iso_datetime(committed_at)
        except ValueError:
            continue

        if committed_dt <= start_dt or committed_dt > end_dt:
            continue

        pr_num = int(match.group(1))
        # GitHub merge commits: body line 1 is the PR title; fall back to subject.
        title = next((line.strip() for line in lines[2:] if line.strip()), subject)
        prs.append({"number": pr_num, "title": title, "mergedAt": committed_at})

    return sorted(prs, key=lambda x: x["mergedAt"])


def sanitize_pr_data(raw_json: str) -> dict[str, Any]:
    """Strips metadata and GitHub noise to save tokens."""
    data = json.loads(raw_json)
    # Remove "(Fixes #123)" and "(Closes #123)"
    clean_body = re.sub(
        r"\(?(Fixes|Closes) #\d+\)?", "", data.get("body", ""), flags=re.IGNORECASE
    )

    return {
        "number": data.get("number"),
        "title": data.get("title"),
        "body": clean_body,
        "comments": [c.get("body") for c in data.get("comments", []) if c.get("body")],
        "reviews": [r.get("body") for r in data.get("reviews", []) if r.get("body")],
    }


def load_prompt_template(prompt_file: str) -> dict[str, Any]:
    """Load the prompt template from YAML file."""
    try:
        with open(prompt_file, "r", encoding="utf-8") as f:
            return yaml.safe_load(f)
    except FileNotFoundError as exc:
        raise FileNotFoundError(f"Prompt file '{prompt_file}' not found") from exc
    except yaml.YAMLError as err:
        raise yaml.YAMLError(f"Failed to parse prompt file: {err}")


def build_ai_prompt(
    current_announcement: str, pr_data: dict[str, Any], prompt_template: dict[str, Any]
) -> dict[str, Any]:
    """Build the complete AI prompt from template and data."""
    system_prompt = next(
        m["content"] for m in prompt_template["messages"] if m["role"] == "system"
    )

    user_content = (
        "Current working announcement:\n====\n"
        + current_announcement
        + "\n====\n\nNewly merged pull request:\n"
        + json.dumps(pr_data, indent=2)
        + "\n====\n\nUpdate the Release Announcement to include any user-relevant "
        + "changes from this PR.\nReturn the complete updated Markdown document only."
    )
    return {
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_content},
        ],
        "model": prompt_template.get("model", GITHUB_MODELS_DEFAULT_CHAT_MODEL),
        "modelParameters": prompt_template.get("modelParameters", {}),
    }


def call_github_models_api(
    prompt: dict[str, Any],
    chat_model_override: str | None = None,
    embedding_model_override: str | None = None,
) -> str:
    """Call the extracted GitHub backend with common token/config resolution from main."""
    return call_github_models_api_backend(
        prompt=prompt,
        resolve_token=resolve_github_token,
        options=GitHubCallOptions(
            chat_model_override=chat_model_override,
            embedding_model_override=embedding_model_override,
            backend_config={
                "default_chat_model": GITHUB_MODELS_DEFAULT_CHAT_MODEL,
                "default_embedding_model": GITHUB_MODELS_DEFAULT_EMBEDDING_MODEL,
                "chat_endpoint": GITHUB_MODELS_CHAT_ENDPOINT,
                "embeddings_endpoint": GITHUB_MODELS_EMBEDDINGS_ENDPOINT,
                "token_limit": GITHUB_MODELS_CHAT_TOKEN_LIMIT,
            },
        ),
    )


def strip_markdown_fences(text: str) -> str:
    """
    Removes ```markdown ... ``` or ``` ... ``` wrappers
    that LLMs often add to their responses.
    """
    # Remove the opening fence (with or without 'markdown' tag)
    text = re.sub(r"^```(markdown)?\n", "", text, flags=re.IGNORECASE)
    # Remove the closing fence
    text = re.sub(r"\n```$", "", text)
    return text.strip()


def _resolve_backend_config(args: argparse.Namespace) -> BackendConfig:
    return resolve_backend_config_impl(args)


def resolve_backend_config(args: argparse.Namespace) -> BackendConfig:
    """Public wrapper around backend config resolution."""
    return _resolve_backend_config(args)


def validate_cli_args(parser: argparse.ArgumentParser, args: argparse.Namespace) -> None:
    """Validate cross-argument invariants before startup probing or processing."""
    validate_cli_args_impl(parser, args)


def probe_capabilities(config: BackendConfig) -> BackendCapabilities:
    """Probe backend capabilities for the resolved model/backends."""
    deps = CapabilityProbeDeps(
        get_backend=registry.get,
        log_warning=logger.warning,
    )
    return probe_capabilities_impl(
        config=config,
        deps=deps,
    )


def validate_mode(config: BackendConfig) -> None:
    """Validate and reconcile pipeline/staged mode against probed capabilities."""
    validate_mode_impl(config, logger.warning)


def _load_prompt_file(path: str, role: str = "system") -> str:
    """Load a prompt YAML file and return the message content for the specified role.

    For single-role extraction (system only), specify role="system".
    For prompts with multiple roles (e.g., ranking with both system and user),
    use _load_prompts() to get the full message list instead.

    Args:
        path: Path to the prompt YAML file.
        role: The message role to extract ("system" or "user").

    Returns:
        The content of the first message matching the specified role.

    Raises:
        ValueError: If no message with the specified role is found.
    """
    with open(path, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f)
    messages = data.get("messages", [])
    for msg in messages:
        if isinstance(msg, dict) and msg.get("role") == role:
            content = msg.get("content", "")
            logger.trace(f"loaded {role} prompt from: {path}")
            return content
    raise ValueError(f"No {role} message found in {path}")


def _load_prompts(path: str) -> list[dict[str, str]]:
    """Load a prompt YAML file and return the full messages list.

    Used for multi-role prompts (e.g., ranking with both system and user messages).

    Args:
        path: Path to the prompt YAML file.

    Returns:
        List of message dicts with 'role' and 'content' keys.

    Raises:
        ValueError: If the file has no 'messages' key or it's not a list.
    """
    with open(path, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f)
    messages = data.get("messages", [])
    if not isinstance(messages, list) or len(messages) == 0:
        raise ValueError(f"Prompt file {path} has no valid 'messages' list")
    logger.trace(f"loaded {len(messages)} message(s) from: {path}")
    return messages


def _resolve_prompt_dir() -> str:
    """Resolve the prompts directory for stage prompt YAML files.

    Tries the package-relative path first (correct for editable/dev installs),
    then falls back to a CWD-relative path (correct when run via the shell
    script from the repository root, where the package is pip-installed into a
    temporary venv outside the repo tree).
    """
    pkg_relative = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "prompts"))
    if os.path.isdir(pkg_relative):
        return pkg_relative
    cwd_relative = os.path.join(os.getcwd(), "tools", "release_announcement", "prompts")
    if os.path.isdir(cwd_relative):
        return cwd_relative
    raise FileNotFoundError(
        "Could not locate tools/release_announcement/prompts. "
        f"Tried package-relative: {pkg_relative} and CWD-relative: {cwd_relative}. "
        "Run the script from the repository root."
    )


def prepare_pr_context(
    pr_data: dict[str, Any],
    backend_config: BackendConfig,
    pipeline_mode: str,
) -> DistilledContext | None:
    """Optional staged preprocessing insertion point for distillation."""
    if pipeline_mode == "legacy":
        return None
    if pipeline_mode != "staged":
        raise ValueError(f"Unsupported pipeline mode: {pipeline_mode}")

    # Load stage prompts from tools/release_announcement/prompts.
    prompt_dir = _resolve_prompt_dir()
    extraction_prompts = _load_prompts(os.path.join(prompt_dir, "extraction.prompt.yml"))
    consolidation_prompts = _load_prompts(os.path.join(prompt_dir, "consolidation.prompt.yml"))
    classification_prompts = _load_prompts(os.path.join(prompt_dir, "classification.prompt.yml"))
    ranking_prompts = _load_prompts(os.path.join(prompt_dir, "ranking.prompt.yml"))

    # Resolve adapters by role. --backend only provides defaults for these.
    chat_backend_name = backend_config.chat_model_backend
    embedding_backend_name = backend_config.embedding_model_backend

    chat_adapter = registry.get(chat_backend_name)
    if chat_adapter is None:
        raise RuntimeError(f"Backend '{chat_backend_name}' not found in registry.")

    embedding_adapter = registry.get(embedding_backend_name)
    if embedding_adapter is None:
        raise RuntimeError(f"Backend '{embedding_backend_name}' not found in registry.")

    adapter = StagedRoutingAdapter(
        chat_adapter=chat_adapter,
        embedding_adapter=embedding_adapter,
    )

    logger.info(
        "staged.preprocessing.start "
        f"chat_backend={chat_backend_name} "
        "embedding_mode="
        f"{'enabled' if backend_config.capabilities.supports_embeddings else 'disabled'}"
        f":{embedding_backend_name}"
    )
    _preprocess_start = time.monotonic()

    # Call the real staged pipeline
    try:
        context = run_distillation_pipeline(
            pr_data=pr_data,
            adapter=adapter,
            backend_config=backend_config,
            prompts=DistillationPrompts(
                extraction=extraction_prompts,
                consolidation=consolidation_prompts,
                classification=classification_prompts,
                ranking=ranking_prompts,
            ),
        )
        _elapsed = time.monotonic() - _preprocess_start
        logger.info(f"staged.preprocessing.end context=distilled elapsed={_elapsed:.1f}s")
        return context
    except STAGED_PREPROCESS_EXCEPTIONS as err:
        _elapsed = time.monotonic() - _preprocess_start
        logger.error(f"staged pipeline failed: {err}")
        logger.info(f"staged.preprocessing.end context=none elapsed={_elapsed:.1f}s")
        return None


def process_single_pr(
    pr_num: int,
    pr_title: str,
    announcement_file: str,
    prompt_file: str,
    config: BackendConfig | None = None,
) -> str:
    """
    Process a single PR and optionally update the announcement file.
    Returns one of:
            "committed"        – LLM updated the file with user-facing changes.
            "no_changes"       – LLM ran but produced no diff.
            "skipped:<reason>" – PR matched skip rules; not sent to LLM.
            "dry_run"          – dry-run mode; PR would have been processed (no LLM called).
    """
    logger.info(f"--- Processing PR #{pr_num}: {pr_title} ---")
    if config is None:
        config = BackendConfig()

    # Get PR data
    pr_data = _fetch_pr_data(pr_num, config)
    if pr_data is None:
        return "dry_run"

    # Check deterministic skip rules before any LLM processing.
    skip_reason = get_skip_reason(pr_data)
    if skip_reason is not None:
        return f"skipped:{skip_reason}"

    if config.dry_run:
        return "dry_run"

    if config.delay_secs > 0:
        logger.info(f"--- Sleeping {config.delay_secs}s before PR #{pr_num} ---")
        time.sleep(config.delay_secs)

    distilled_context: DistilledContext | None = None
    if config.pipeline_mode == "staged":
        try:
            distilled_context = cast(
                DistilledContext | None,
                prepare_pr_context(pr_data, config, config.pipeline_mode),
            )
        except STAGED_PREPROCESS_EXCEPTIONS as err:
            logger.warning(
                f"staged preprocessing failed ({err}); falling back to legacy mode"
            )
            distilled_context = None

        if distilled_context is None:
            logger.warning(
                "staged preprocessing returned no context, falling back to legacy mode"
            )

    # Load and process announcement
    current_content = _load_announcement_content(announcement_file)
    prompt_template = _load_prompt_template(prompt_file)
    ai_prompt = _build_ai_prompt(
        current_content,
        pr_data,
        prompt_template,
        pipeline_mode=config.pipeline_mode,
        distilled_context=distilled_context,
    )

    # Process with LLM backend
    updated_ra = _process_with_llm(ai_prompt, config)

    # Write and check changes
    _write_and_check_announcement(updated_ra, announcement_file)

    return _check_for_changes(announcement_file)


# Get PR data, routing to upstream when running from a fork so that upstream
# PR numbers resolve correctly regardless of where the script runs.
def _fetch_pr_data(pr_num: int, config: BackendConfig) -> dict[str, Any] | None:
    """Fetch PR data from GitHub API."""
    upstream = _get_upstream_repo()
    repo_flag = ["--repo", upstream] if upstream else []
    try:
        update_text = subprocess.check_output(
            ["gh", "pr", "view", str(pr_num), "--json", "number,title,body,comments,reviews"]
            + repo_flag,
            text=True,
            env=get_gh_auth_env(),
        )
        pr_data = sanitize_pr_data(update_text)

        # gh pr view omits inline code-review thread comments (tied to specific
        # lines of code); fetch them separately via the REST API.
        repo = upstream or _get_current_repo_name()
        if repo:
            pr_data["comments"].extend(_fetch_inline_review_comments(pr_num, repo))

        return pr_data
    except subprocess.CalledProcessError:
        if config.dry_run:
            return None  # PR not accessible; skip metadata fetch in dry-run.
        raise


def _load_announcement_content(announcement_file: str) -> str:
    """Load current announcement content."""
    with open(announcement_file, "r", encoding="utf-8") as f:
        return f.read()


def _load_prompt_template(prompt_file: str) -> dict[str, Any]:
    """Load prompt template."""
    return load_prompt_template(prompt_file)


def _build_ai_prompt(
    current_content: str,
    pr_data: dict[str, Any],
    prompt_template: dict[str, Any],
    pipeline_mode: str = "legacy",
    distilled_context: DistilledContext | None = None,
) -> dict[str, Any]:
    """Build AI prompt from template and data, selecting legacy or staged path.

    Legacy mode always uses raw PR payload prompt construction.
    Staged mode uses distilled context when available, otherwise falls back to
    the legacy raw-PR path.
    """
    if pipeline_mode == "staged" and distilled_context is not None:
        return _build_staged_ai_prompt(
            current_content=current_content,
            pr_data=pr_data,
            distilled_context=distilled_context,
            prompt_template=prompt_template,
        )
    return build_ai_prompt(current_content, pr_data, prompt_template)


def _build_staged_ai_prompt(
    current_content: str,
    pr_data: dict[str, Any],
    distilled_context: DistilledContext,
    prompt_template: dict[str, Any],
) -> dict[str, Any]:
    """Build staged prompt using distilled preprocessing context.

    The final edit step remains the same, but the staged path supplies the
    distilled summary/signals instead of the full raw PR discussion payload.
    """
    system_prompt = next(
        m["content"] for m in prompt_template["messages"] if m["role"] == "system"
    )

    distilled_payload = {
        "pr_number": pr_data.get("number"),
        "pr_title": pr_data.get("title"),
        "summary": distilled_context.summary,
        "structured_signals": distilled_context.structured_signals,
        "classification": distilled_context.classification,
        "metadata": distilled_context.metadata,
    }

    user_content = (
        "Current working announcement:\n====\n"
        + current_content
        + "\n====\n\nDistilled pull request context:\n"
        + json.dumps(distilled_payload, indent=2)
        + "\n====\n\nUpdate the Release Announcement to include any user-relevant "
        + "changes from this PR.\nReturn the complete updated Markdown document only."
    )

    return {
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_content},
        ],
        "model": prompt_template.get("model", GITHUB_MODELS_DEFAULT_CHAT_MODEL),
        "modelParameters": prompt_template.get("modelParameters", {}),
    }


def _process_with_llm(ai_prompt: dict[str, Any], config: BackendConfig) -> str:
    """Process with appropriate LLM backend."""
    target_backend = config.chat_model_backend or config.backend
    if target_backend == "ollama":
        updated_ra = call_ollama_model(ai_prompt, config.chat_model, config.embedding_model)
    elif target_backend in {"github", "actions"}:
        updated_ra = call_github_models_api(
            ai_prompt,
            chat_model_override=config.chat_model,
            embedding_model_override=config.embedding_model,
        )
    else:
        raise BackendValidationError(f"Unknown backend '{target_backend}'")
    return strip_markdown_fences(updated_ra)


def _write_and_check_announcement(updated_ra: str, announcement_file: str) -> None:
    """Write updated announcement and check for changes."""
    with open(announcement_file, "w", encoding="utf-8") as f:
        f.write(updated_ra)


def _check_for_changes(announcement_file: str) -> str:
    """Check if announcement file has changes."""
    diff_check = subprocess.run(
        ["git", "diff", "-w", "--exit-code", announcement_file],
        capture_output=True,
        check=False,
    )
    return "no_changes" if diff_check.returncode == 0 else "committed"


def _setup_backend_token(backend: str) -> None:
    """Resolve and pin the GitHub token for the chosen backend."""
    if backend == "github":
        token = resolve_github_token()
        os.environ["GH_TOKEN"] = token
        os.environ["GITHUB_TOKEN"] = token
    elif backend == "actions":
        raw = os.getenv("GITHUB_TOKEN") or os.getenv("GH_TOKEN")
        if not raw:
            logger.critical(
                "--backend actions requires GITHUB_TOKEN to be set.\n"
                "Add this to your workflow step:\n"
                "  env:\n"
                "    GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}"
            )
            sys.exit(1)
        token = normalize_github_token(raw)
        os.environ["GH_TOKEN"] = token
        os.environ["GITHUB_TOKEN"] = token


def _resolve_timeframe(args: argparse.Namespace) -> tuple[str, str, str, str]:
    """Resolve revision labels and timestamps for PR discovery."""
    if args.end is None:
        lower_bound_label = f"{args.start}~"
        upper_bound_label = args.start
        start_ts = get_previous_commit_timestamp(args.start)
        end_ts = get_universal_timestamp(args.start)
    else:
        lower_bound_label = args.start
        upper_bound_label = args.end
        start_ts = get_universal_timestamp(args.start)
        end_ts = get_universal_timestamp(args.end)
    return lower_bound_label, upper_bound_label, start_ts, end_ts


def _process_prs(
    args: argparse.Namespace,
    config: BackendConfig,
    todo_prs: list[dict[str, Any]],
) -> None:
    """Process discovered PRs and print summary."""
    processed = skipped = no_changes = 0
    for pr in todo_prs:
        pr_num = pr["number"]
        pr_title = pr["title"]

        try:
            result = process_single_pr(
                pr_num,
                pr_title,
                args.file,
                args.prompt,
                config,
            )
        except Exception as err:
            raise RuntimeError(
                f"Failed while processing PR #{pr_num} ({pr_title}): {err}"
            ) from err

        if result == "committed":
            logger.info(f"   > Updated release announcement for #{pr_num}. Preparing git commit.")
            _commit_pr_update(args.file, pr_num, pr_title)
            logger.info("   Successfully committed.")
            processed += 1
        elif result == "no_changes":
            logger.info(f"   > No user-facing changes for #{pr_num}. Skipping commit.")
            no_changes += 1
        elif result.startswith("skipped"):
            skip_reason = result.split(":", 1)[1] if ":" in result else "skip rule"
            logger.info(f"   > Skipping PR #{pr_num} ({skip_reason}).")
            skipped += 1
        elif result == "dry_run":
            logger.info(f"   [DRY RUN] Would process PR #{pr_num}.")
            processed += 1

    if args.dry_run:
        logger.info(
            f"\n--- [DRY RUN] Done. {len(todo_prs)} PRs found: "
            f"{processed} would process, {skipped} would skip. ---"
        )
    else:
        logger.info(
            f"\n--- Done! {len(todo_prs)} PRs: "
            f"{processed} committed, {no_changes} no changes, {skipped} skipped. ---"
        )


def _commit_pr_update(announcement_file: str, pr_num: int, pr_title: str) -> None:
    """Stage and commit announcement updates for a processed PR."""
    try:
        subprocess.run(
            ["git", "add", announcement_file],
            check=True,
            capture_output=True,
            text=True,
        )
        subprocess.run(
            ["git", "commit", "-m", f"[bot] RA: Merge #{pr_num}: {pr_title}"],
            check=True,
            capture_output=True,
            text=True,
        )
    except subprocess.CalledProcessError as err:
        command = " ".join(str(part) for part in err.cmd)
        stderr = (err.stderr or "").strip()
        stdout = (err.stdout or "").strip()
        details = stderr or stdout or str(err)
        raise RuntimeError(
            f"Could not stage or commit updates for PR #{pr_num}. "
            f"Command failed: {command}. {details}"
        ) from err


def _initialize_runtime(
    parser: argparse.ArgumentParser,
    args: argparse.Namespace,
) -> BackendConfig:
    """Validate args, prepare token env, and reconcile capabilities."""
    validate_cli_args(parser, args)

    # Resolve and pin the token before any subprocess calls that need it.
    _setup_backend_token(args.backend)

    config = _resolve_backend_config(args)
    logger.debug(
        "Capability resolution input: "
        f"backend={config.backend} "
        f"chat_backend={config.chat_model_backend} chat_model={config.chat_model!r} "
        f"embedding_backend={config.embedding_model_backend} "
        f"embedding_model={config.embedding_model!r} "
        f"pipeline={config.pipeline_mode} staged_mode={config.staged_mode}"
    )

    if config.dry_run:
        logger.debug("Skipping backend capability probe in dry-run mode...")
        # In dry-run mode, assume all capabilities are available since we do not call backends.
        config.capabilities = BackendCapabilities(
            supports_chat=True,
            supports_embeddings=True,
        )
    else:
        logger.debug("Probing backend capabilities...")
        probe_capabilities(config)

    logger.debug(
        "Capability probe result: "
        f"supports_chat={config.capabilities.supports_chat} "
        f"supports_embeddings={config.capabilities.supports_embeddings}"
    )

    if config.pipeline_mode == "staged":
        logger.debug("Reconciling requested pipeline mode with probed capabilities...")
        validate_mode(config)
        logger.debug(f"Effective staged mode after validation: {config.staged_mode}")
    else:
        validate_mode(config)
        logger.debug("Legacy pipeline selected; staged mode reconciliation skipped.")

    return config


def _log_and_exit_on_handled_error(err: Exception) -> None:
    """Log a handled startup/processing failure and terminate with code 1."""
    logger.critical(str(err))
    sys.exit(1)


def _log_and_exit_on_unexpected_error(context: str, err: Exception) -> None:
    """Handle unexpected startup/processing failures with a single critical line."""
    logger.critical(f"Unexpected {context} error: {err}")
    sys.exit(1)


def _run_processing(args: argparse.Namespace, config: BackendConfig) -> None:
    """Resolve target PR range and process announcements in sequence."""
    lower_bound_label, upper_bound_label, start_ts, end_ts = _resolve_timeframe(args)
    logger.info(f"--- Resolving boundaries: {lower_bound_label} -> {upper_bound_label} ---")

    todo_prs = get_ordered_pr_list(start_ts, end_ts)
    if not todo_prs:
        logger.info("No new PRs found to process.")
        return

    logger.info(f"--- Found {len(todo_prs)} PRs to merge oldest-to-newest ---")
    _process_prs(args, config, todo_prs)


def main() -> None:
    """Run the release announcement generation CLI."""
    parser = build_arg_parser()
    args = parser.parse_args()

    # Set up logging level
    logger.set_level(getattr(args, "log_level", "INFO"))

    if args.delay_secs < 0:
        logger.critical("--delay-secs must be >= 0")
        sys.exit(1)

    try:
        config = _initialize_runtime(parser, args)
    except HANDLED_MAIN_EXCEPTIONS as err:
        _log_and_exit_on_handled_error(err)
    except UNEXPECTED_MAIN_EXCEPTIONS as err:
        _log_and_exit_on_unexpected_error("startup", err)

    try:
        _run_processing(args, config)
    except HANDLED_MAIN_EXCEPTIONS as err:
        _log_and_exit_on_handled_error(err)
    except UNEXPECTED_MAIN_EXCEPTIONS as err:
        _log_and_exit_on_unexpected_error("processing", err)


def run_cli() -> None:
    """Run CLI with clean interrupt handling for all module entry points."""
    try:
        main()
    except KeyboardInterrupt:
        logger.critical("User interrupt -- stopped")
        sys.exit(130)


if __name__ == "__main__":
    run_cli()
