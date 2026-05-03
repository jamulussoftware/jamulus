"""Unit tests for staged distillation prompt files.

Tests verify that stage prompt files exist, have valid YAML structure,
and can be loaded by the prompt loader without schema errors.
"""

import os
import pytest
import yaml

from src.release_announcement.main import _load_prompt_file, _load_prompts


class TestPlaceholderPromptFilesCore:
    """Tests for placeholder prompt file existence and validity."""

    def get_prompt_path(self, filename: str) -> str:
        """Get the absolute path to a prompt file in tools/release_announcement/prompts/."""
        base_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
            os.path.abspath(__file__)
        ))))
        return os.path.join(base_dir, "tools", "release_announcement", "prompts", filename)

    def test_extraction_prompt_exists(self):
        """Verify extraction.prompt.yml exists."""
        path = self.get_prompt_path("extraction.prompt.yml")
        assert os.path.exists(path), f"extraction.prompt.yml not found at {path}"

    def test_consolidation_prompt_exists(self):
        """Verify consolidation.prompt.yml exists."""
        path = self.get_prompt_path("consolidation.prompt.yml")
        assert os.path.exists(path), f"consolidation.prompt.yml not found at {path}"

    def test_classification_prompt_exists(self):
        """Verify classification.prompt.yml exists."""
        path = self.get_prompt_path("classification.prompt.yml")
        assert os.path.exists(path), f"classification.prompt.yml not found at {path}"

    def test_extraction_prompt_valid_yaml(self):
        """Verify extraction.prompt.yml is valid YAML."""
        path = self.get_prompt_path("extraction.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert data is not None

    def test_consolidation_prompt_valid_yaml(self):
        """Verify consolidation.prompt.yml is valid YAML."""
        path = self.get_prompt_path("consolidation.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert data is not None

    def test_classification_prompt_valid_yaml(self):
        """Verify classification.prompt.yml is valid YAML."""
        path = self.get_prompt_path("classification.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert data is not None

    def test_extraction_prompt_has_messages_key(self):
        """Verify extraction.prompt.yml has 'messages' key."""
        path = self.get_prompt_path("extraction.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert "messages" in data, "extraction.prompt.yml missing 'messages' key"

    def test_consolidation_prompt_has_messages_key(self):
        """Verify consolidation.prompt.yml has 'messages' key."""
        path = self.get_prompt_path("consolidation.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert "messages" in data, "consolidation.prompt.yml missing 'messages' key"

    def test_classification_prompt_has_messages_key(self):
        """Verify classification.prompt.yml has 'messages' key."""
        path = self.get_prompt_path("classification.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert "messages" in data, "classification.prompt.yml missing 'messages' key"

    def test_extraction_prompt_messages_is_list(self):
        """Verify extraction.prompt.yml messages is a list."""
        path = self.get_prompt_path("extraction.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert isinstance(data["messages"], list), "extraction.prompt.yml messages is not a list"
        assert len(data["messages"]) > 0, "extraction.prompt.yml messages is empty"

    def test_consolidation_prompt_messages_is_list(self):
        """Verify consolidation.prompt.yml messages is a list."""
        path = self.get_prompt_path("consolidation.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert isinstance(
            data["messages"],
            list,
        ), "consolidation.prompt.yml messages is not a list"
        assert len(data["messages"]) > 0, "consolidation.prompt.yml messages is empty"

    def test_classification_prompt_messages_is_list(self):
        """Verify classification.prompt.yml messages is a list."""
        path = self.get_prompt_path("classification.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert isinstance(
            data["messages"],
            list,
        ), "classification.prompt.yml messages is not a list"
        assert len(data["messages"]) > 0, "classification.prompt.yml messages is empty"

    def test_extraction_prompt_has_system_message(self):
        """Verify extraction.prompt.yml has at least one system message."""
        path = self.get_prompt_path("extraction.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        messages = data["messages"]
        system_msgs = [m for m in messages if isinstance(m, dict) and m.get("role") == "system"]
        assert len(system_msgs) > 0, "extraction.prompt.yml has no system message"

    def test_consolidation_prompt_has_system_message(self):
        """Verify consolidation.prompt.yml has at least one system message."""
        path = self.get_prompt_path("consolidation.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        messages = data["messages"]
        system_msgs = [m for m in messages if isinstance(m, dict) and m.get("role") == "system"]
        assert len(system_msgs) > 0, "consolidation.prompt.yml has no system message"

    def test_classification_prompt_has_system_message(self):
        """Verify classification.prompt.yml has at least one system message."""
        path = self.get_prompt_path("classification.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        messages = data["messages"]
        system_msgs = [m for m in messages if isinstance(m, dict) and m.get("role") == "system"]
        assert len(system_msgs) > 0, "classification.prompt.yml has no system message"

    def test_extraction_prompt_system_message_has_content(self):
        """Verify extraction.prompt.yml system message has content."""
        path = self.get_prompt_path("extraction.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        messages = data["messages"]
        system_msgs = [m for m in messages if isinstance(m, dict) and m.get("role") == "system"]
        assert len(system_msgs) > 0
        assert "content" in system_msgs[0]
        assert isinstance(system_msgs[0]["content"], str)
        assert len(system_msgs[0]["content"]) > 0

    def test_consolidation_prompt_system_message_has_content(self):
        """Verify consolidation.prompt.yml system message has content."""
        path = self.get_prompt_path("consolidation.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        messages = data["messages"]
        system_msgs = [m for m in messages if isinstance(m, dict) and m.get("role") == "system"]
        assert len(system_msgs) > 0
        assert "content" in system_msgs[0]
        assert isinstance(system_msgs[0]["content"], str)
        assert len(system_msgs[0]["content"]) > 0

    def test_classification_prompt_system_message_has_content(self):
        """Verify classification.prompt.yml system message has content."""
        path = self.get_prompt_path("classification.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        messages = data["messages"]
        system_msgs = [m for m in messages if isinstance(m, dict) and m.get("role") == "system"]
        assert len(system_msgs) > 0
        assert "content" in system_msgs[0]
        assert isinstance(system_msgs[0]["content"], str)
        assert len(system_msgs[0]["content"]) > 0



class TestPlaceholderPromptFilesRanking:
    """Tests focused on ranking prompt and cross-file comparisons."""

    def get_prompt_path(self, filename: str) -> str:
        """Get the absolute path to a prompt file in tools/release_announcement/prompts/."""
        base_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
            os.path.abspath(__file__)
        ))))
        return os.path.join(base_dir, "tools", "release_announcement", "prompts", filename)

    def test_ranking_prompt_exists(self):
        """Verify ranking.prompt.yml exists."""
        path = self.get_prompt_path("ranking.prompt.yml")
        assert os.path.exists(path), f"ranking.prompt.yml not found at {path}"

    def test_ranking_prompt_valid_yaml(self):
        """Verify ranking.prompt.yml is valid YAML."""
        path = self.get_prompt_path("ranking.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert data is not None

    def test_ranking_prompt_has_messages_key(self):
        """Verify ranking.prompt.yml has 'messages' key."""
        path = self.get_prompt_path("ranking.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert "messages" in data, "ranking.prompt.yml missing 'messages' key"

    def test_ranking_prompt_messages_is_list(self):
        """Verify ranking.prompt.yml messages is a list."""
        path = self.get_prompt_path("ranking.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        assert isinstance(data["messages"], list), "ranking.prompt.yml messages is not a list"
        assert len(data["messages"]) > 0, "ranking.prompt.yml messages is empty"

    def test_ranking_prompt_has_user_message(self):
        """Verify ranking.prompt.yml has at least one user message."""
        path = self.get_prompt_path("ranking.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        messages = data["messages"]
        user_msgs = [m for m in messages if isinstance(m, dict) and m.get("role") == "user"]
        assert len(user_msgs) > 0, "ranking.prompt.yml has no user message"

    def test_ranking_prompt_user_message_has_content(self):
        """Verify ranking.prompt.yml user message has content."""
        path = self.get_prompt_path("ranking.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        messages = data["messages"]
        user_msgs = [m for m in messages if isinstance(m, dict) and m.get("role") == "user"]
        assert len(user_msgs) > 0
        assert "content" in user_msgs[0]
        assert isinstance(user_msgs[0]["content"], str)
        assert len(user_msgs[0]["content"]) > 0

    def test_ranking_prompt_has_template_variables(self):
        """Verify ranking.prompt.yml contains template variables."""
        path = self.get_prompt_path("ranking.prompt.yml")
        with open(path, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
        messages = data["messages"]
        user_msgs = [m for m in messages if isinstance(m, dict) and m.get("role") == "user"]
        content = user_msgs[0]["content"]
        # Check for expected template variables
        assert "{chunk_count}" in content, "ranking.prompt.yml missing {chunk_count} variable"
        assert "{chunks}" in content, "ranking.prompt.yml missing {chunks} variable"

    def test_all_four_prompts_different_content(self):
        """Verify the four prompts have different content (not identical copies)."""
        with open(self.get_prompt_path("extraction.prompt.yml"), "r", encoding="utf-8") as f:
            extraction = yaml.safe_load(f)
        with open(self.get_prompt_path("consolidation.prompt.yml"), "r", encoding="utf-8") as f:
            consolidation = yaml.safe_load(f)
        with open(self.get_prompt_path("classification.prompt.yml"), "r", encoding="utf-8") as f:
            classification = yaml.safe_load(f)
        with open(self.get_prompt_path("ranking.prompt.yml"), "r", encoding="utf-8") as f:
            ranking = yaml.safe_load(f)

        extraction_content = extraction["messages"][0]["content"]
        consolidation_content = consolidation["messages"][0]["content"]
        classification_content = classification["messages"][0]["content"]
        ranking_content = ranking["messages"][0]["content"]

        # At least one should be different (they shouldn't all be identical)
        unique_prompts = {extraction_content, consolidation_content,
                          classification_content, ranking_content}
        assert len(unique_prompts) >= 2, "Prompts should have different content"


class TestLoadPromptFileFunction:
    """Tests for the _load_prompt_file helper function."""

    SIGNAL_FIELDS = [
        "change",
        "impact",
        "users_affected",
        "confidence",
        "final_outcome",
    ]

    def _assert_io_contract(
        self,
        content: str,
        expect_classification_object: bool = False,
    ) -> None:
        """Verify prompt text includes stable Input/Output contract sections."""
        assert "Input:" in content or "Input scope:" in content
        assert "Output contract:" in content
        assert "Return ONLY valid JSON." in content
        for field in self.SIGNAL_FIELDS:
            assert field in content

        if expect_classification_object:
            # Must align with ClassifiedSignals top-level structure used by parsing code.
            assert "classified" in content
            assert "summary" in content

    def get_prompt_path(self, filename: str) -> str:
        """Get the absolute path to a prompt file in tools/release_announcement/prompts/."""
        base_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
            os.path.abspath(__file__)
        ))))
        return os.path.join(base_dir, "tools", "release_announcement", "prompts", filename)

    def test_load_extraction_prompt_with_system_role(self):
        """Verify _load_prompt_file correctly loads extraction prompt with system role."""
        path = self.get_prompt_path("extraction.prompt.yml")
        content = _load_prompt_file(path, role="system")

        assert isinstance(content, str)
        assert len(content) > 0
        assert "placeholder" not in content.lower()
        self._assert_io_contract(content)

    def test_load_consolidation_prompt_with_system_role(self):
        """Verify _load_prompt_file correctly loads consolidation prompt with system role."""
        path = self.get_prompt_path("consolidation.prompt.yml")
        content = _load_prompt_file(path, role="system")

        assert isinstance(content, str)
        assert len(content) > 0
        assert "placeholder" not in content.lower()
        self._assert_io_contract(content)

    def test_load_classification_prompt_with_system_role(self):
        """Verify _load_prompt_file correctly loads classification prompt with system role."""
        path = self.get_prompt_path("classification.prompt.yml")
        content = _load_prompt_file(path, role="system")

        assert isinstance(content, str)
        assert len(content) > 0
        assert "placeholder" not in content.lower()
        self._assert_io_contract(content, expect_classification_object=True)
        assert "internal" in content
        assert "minor" in content
        assert "targeted" in content
        assert "major" in content
        assert "no_user_facing_changes" in content

    def test_stage_prompts_do_not_contain_placeholder_marker(self):
        """Verify stage prompts no longer contain placeholder content."""
        for filename in [
            "extraction.prompt.yml",
            "consolidation.prompt.yml",
            "classification.prompt.yml",
        ]:
            path = self.get_prompt_path(filename)
            content = _load_prompt_file(path, role="system")
            assert "placeholder" not in content.lower()

    def test_load_ranking_prompt_with_user_role(self):
        """Verify _load_prompt_file correctly loads ranking prompt with user role."""
        path = self.get_prompt_path("ranking.prompt.yml")
        content = _load_prompt_file(path, role="user")

        assert isinstance(content, str)
        assert len(content) > 0
        assert "{chunk_count}" in content
        assert "{chunks}" in content

    def test_load_system_prompt_fails_with_user_role(self):
        """Verify _load_prompt_file fails when requesting user role from system-only prompt."""
        path = self.get_prompt_path("extraction.prompt.yml")

        with pytest.raises(ValueError) as exc_info:
            _load_prompt_file(path, role="user")

        assert "user" in str(exc_info.value)
        assert "extraction.prompt.yml" in str(exc_info.value)

    def test_load_ranking_prompt_with_both_roles(self):
        """Verify _load_prompt_file can load both system and user roles from ranking prompt."""
        path = self.get_prompt_path("ranking.prompt.yml")

        # Ranking prompt now has both system and user messages
        system_content = _load_prompt_file(path, role="system")
        user_content = _load_prompt_file(path, role="user")

        assert isinstance(system_content, str)
        assert len(system_content) > 0
        assert "release announcement" in system_content.lower()

        assert isinstance(user_content, str)
        assert len(user_content) > 0
        assert "{chunk_count}" in user_content or "{chunks}" in user_content


class TestLoadPromptMessagesFunction:
    """Tests for the _load_prompts helper function."""

    def get_prompt_path(self, filename: str) -> str:
        """Get the absolute path to a prompt file in tools/release_announcement/prompts/."""
        base_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
            os.path.abspath(__file__)
        ))))
        return os.path.join(base_dir, "tools", "release_announcement", "prompts", filename)

    def test_load_ranking_prompt_messages_structure(self):
        """Verify _load_prompts returns full messages with both roles."""
        path = self.get_prompt_path("ranking.prompt.yml")
        messages = _load_prompts(path)

        assert isinstance(messages, list)
        assert len(messages) >= 2

        roles = [msg.get("role") for msg in messages]
        assert "system" in roles
        assert "user" in roles

    def test_ranking_prompt_messages_have_content(self):
        """Verify ranking.prompt.yml messages all have content."""
        path = self.get_prompt_path("ranking.prompt.yml")
        messages = _load_prompts(path)

        for msg in messages:
            assert "content" in msg
            assert isinstance(msg["content"], str)
            assert len(msg["content"]) > 0

    def test_ranking_prompt_user_message_has_template_variables(self):
        """Verify ranking.prompt.yml user message contains template variables."""
        path = self.get_prompt_path("ranking.prompt.yml")
        messages = _load_prompts(path)

        user_msg = next((m for m in messages if m.get("role") == "user"), None)
        assert user_msg is not None

        content = user_msg["content"]
        assert "{chunk_count}" in content
        assert "{chunks}" in content

    def test_load_prompts_invalid_file(self):
        """Verify _load_prompts fails gracefully for missing file."""
        with pytest.raises(FileNotFoundError):
            _load_prompts("/nonexistent/prompts/missing.prompt.yml")

    def test_load_extraction_prompt_messages(self):
        """Verify extraction.prompt.yml loads as messages list with system role."""
        path = self.get_prompt_path("extraction.prompt.yml")
        messages = _load_prompts(path)

        assert isinstance(messages, list)
        assert len(messages) >= 1

        system_msg = next((m for m in messages if m.get("role") == "system"), None)
        assert system_msg is not None
        assert isinstance(system_msg["content"], str)
        assert len(system_msg["content"]) > 0

    def test_load_consolidation_prompt_messages(self):
        """Verify consolidation.prompt.yml loads as messages list with system role."""
        path = self.get_prompt_path("consolidation.prompt.yml")
        messages = _load_prompts(path)

        assert isinstance(messages, list)
        assert len(messages) >= 1

        system_msg = next((m for m in messages if m.get("role") == "system"), None)
        assert system_msg is not None
        assert isinstance(system_msg["content"], str)
        assert len(system_msg["content"]) > 0

    def test_load_classification_prompt_messages(self):
        """Verify classification.prompt.yml loads as messages list with system role."""
        path = self.get_prompt_path("classification.prompt.yml")
        messages = _load_prompts(path)

        assert isinstance(messages, list)
        assert len(messages) >= 1

        system_msg = next((m for m in messages if m.get("role") == "system"), None)
        assert system_msg is not None
        assert isinstance(system_msg["content"], str)
        assert len(system_msg["content"]) > 0
