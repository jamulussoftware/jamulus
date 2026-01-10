#!/usr/bin/env python3
#
##############################################################################
# Copyright (c) 2026
#
# Author(s):
#  ChatGPT
#  ann0see
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
Qt TS translation checker.

This tool validates Qt `.ts` translation files according to Qt Linguist
semantics.
Warnings are reported with best-effort line numbers. In strict mode, the
presence of any warning results in a non-zero exit code to allow CI failure.
"""

import argparse
import re
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path

# Regex helpers
PLACEHOLDER_RE = re.compile(r"%\d+")
HTML_TAG_RE = re.compile(r"<[^>]+>")

# ANSI escape codes
BOLD = "\033[1m"
CYAN = "\033[36m"
YELLOW = "\033[33m"
RED = "\033[31m"
RESET = "\033[0m"

# Severity Enum
class Severity(IntEnum):
    WARNING = 1
    SEVERE = 2

# Data structures
@dataclass(frozen=True)
class MessageContext:
    ts_file: Path
    line: int
    lang: str
    source: str
    translation: str
    tr_type: str
    excerpt: str

@dataclass(frozen=True)
class WarningItem:
    ts_file: Path
    line: int
    message: str
    severity: Severity

# Helpers
def approximate_message_lines(text: str):
    """Yield approximate line numbers for <message> elements."""
    lines = text.splitlines()
    cursor = 0
    for _ in range(text.count("<message")):
        for i in range(cursor, len(lines)):
            if "<message" in lines[i]:
                cursor = i + 1
                yield i + 1
                break
        else:
            yield 0

# Checks
def check_language_header(ts_file: Path, root):
    file_lang = ts_file.stem.replace("translation_", "")
    header_lang = root.attrib.get("language", "")
    if header_lang != file_lang:
        return [WarningItem(ts_file, 0,
                            f"Language header mismatch '{header_lang}' != '{file_lang}'",
                            Severity.WARNING)]
    return []

def check_empty_translation(ctx: MessageContext):
    if not ctx.translation and ctx.tr_type != "unfinished":
        return [WarningItem(ctx.ts_file, ctx.line,
                            f"{ctx.lang}: empty translation for '{ctx.excerpt}...'",
                            Severity.SEVERE)]
    return []

def check_placeholders(ctx: MessageContext):
    if ctx.tr_type != "unfinished" and set(PLACEHOLDER_RE.findall(ctx.source)) != set(PLACEHOLDER_RE.findall(ctx.translation)):
        return [WarningItem(ctx.ts_file, ctx.line,
                            f"{ctx.lang}: placeholder mismatch for '{ctx.excerpt}...'\n"
                            f"Source: {ctx.source}\nTranslation: {ctx.translation}",
                            Severity.WARNING)]
    return []

def check_html(ctx: MessageContext):
    if HTML_TAG_RE.search(ctx.source) and not HTML_TAG_RE.search(ctx.translation) and ctx.tr_type != "unfinished":
        return [WarningItem(ctx.ts_file, ctx.line,
                            f"{ctx.lang}: HTML missing for '{ctx.excerpt}...'\n"
                            f"Source: {ctx.source}\nTranslation: {ctx.translation}",
                            Severity.WARNING)]
    return []

def check_whitespace(ctx: MessageContext):
    if ctx.source != ctx.source.strip() or ctx.translation != ctx.translation.strip():
        return [WarningItem(ctx.ts_file, ctx.line,
                            f"{ctx.lang}: leading/trailing whitespace difference for '{ctx.excerpt}...'",
                            Severity.WARNING)]
    return []

def check_newline_consistency(ctx: MessageContext):
    if ctx.source.endswith("\n") != ctx.translation.endswith("\n"):
        return [WarningItem(ctx.ts_file, ctx.line,
                            f"{ctx.lang}: newline mismatch for '{ctx.excerpt}...'",
                            Severity.WARNING)]
    return []

# Detect warnings
def detect_warnings(ts_file: Path):
    try:
        text = ts_file.read_text(encoding="utf-8")
        root = ET.fromstring(text)
    except (OSError, ET.ParseError) as exc:
        return [WarningItem(ts_file, 0,
                            f"Error reading or parsing XML: {exc}",
                            Severity.SEVERE)]

    warnings = []
    warnings.extend(check_language_header(ts_file, root))

    file_lang = ts_file.stem.replace("translation_", "")
    message_lines = approximate_message_lines(text)

    for context in root.findall("context"):
        for message, line in zip(context.findall("message"), message_lines):
            source = (message.findtext("source") or "").strip()
            tr_elem = message.find("translation")
            translation = ""
            tr_type = ""
            if tr_elem is not None:
                translation = (tr_elem.text or "").strip()
                tr_type = tr_elem.attrib.get("type", "")
            excerpt = source[:30].replace("\n", " ")

            ctx = MessageContext(ts_file, line, file_lang, source, translation, tr_type, excerpt)

            # All checks
            warnings.extend(check_empty_translation(ctx))
            warnings.extend(check_placeholders(ctx))
            warnings.extend(check_html(ctx))
            warnings.extend(check_whitespace(ctx))
            warnings.extend(check_newline_consistency(ctx))

    return warnings

# CLI
def main():
    parser = argparse.ArgumentParser(description="Qt TS translation checker with extended rules")
    parser.add_argument("--ts-dir", type=Path, default=Path("src/translation"),
                        help="Directory containing translation_*.ts files")
    parser.add_argument("--strict", action="store_true",
                        help="Exit non-zero if any warning is found")
    args = parser.parse_args()

    if not args.ts_dir.exists():
        print(f"Directory not found: {args.ts_dir}", file=sys.stderr)
        return 2
    ts_files = sorted(args.ts_dir.glob("translation_*.ts"))
    if not ts_files:
        print(f"No TS files found in {args.ts_dir}", file=sys.stderr)
        return 2

    all_warnings = []
    for ts_file in ts_files:
        all_warnings.extend(detect_warnings(ts_file))

    grouped = defaultdict(list)
    for w in all_warnings:
        grouped[(w.ts_file, w.line)].append(w)

    # Detailed output
    for (file, line), messages in sorted(grouped.items()):
        for w in messages:
            color = RED if w.severity == Severity.SEVERE else YELLOW
            print(f"{BOLD}{file}{RESET} {CYAN}line {line}{RESET}: {color}{w.message}{RESET}")

    # Test summary
    failures_by_language = defaultdict(lambda: {"severe":0, "warning":0})
    all_languages = set()

    for w in all_warnings:
        lang = w.ts_file.stem.replace("translation_", "")
        all_languages.add(lang)
        if w.severity == Severity.SEVERE:
            failures_by_language[lang]["severe"] += 1
        else:
            failures_by_language[lang]["warning"] += 1

    print("\n== Test Summary ==")
    for lang in sorted(all_languages):
        counts = failures_by_language[lang]
        print(f"{BOLD}[{lang}]{RESET} Severe: {counts['severe']}, Warnings: {counts['warning']}")

    total_severe = sum(f["severe"] for f in failures_by_language.values())
    total_warning = sum(f["warning"] for f in failures_by_language.values())
    print(f"\nTotal Severe: {total_severe}, Total Warnings: {total_warning}")

    if total_severe > 0 or (args.strict and total_warning > 0):
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(main())
