#!/usr/bin/env python3
#
##############################################################################
# Copyright (c) 2026
#
# Author(s):
#  ChatGPT
#  ann0see
#  JaminShanti
#  Gemini
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
"""

import argparse
import re
import sys
import xml.etree.ElementTree as ET
import xml.sax
from collections import defaultdict, Counter
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path

# Regex helpers
PLACEHOLDER_RE = re.compile(r"%\d+")
HTML_TAG_RE = re.compile(r"<[^>]+>")

# ANSI escape codes
BOLD, CYAN, YELLOW, RED, RESET = "\033[1m", "\033[36m", "\033[33m", "\033[31m", "\033[0m"


class Severity(IntEnum):
    WARNING = 1
    SEVERE = 2


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
    lang: str
    message: str
    severity: Severity


class MessageLocator(xml.sax.ContentHandler):
    """SAX handler to find exact line numbers of <message> elements."""
    def __init__(self):
        super().__init__()
        self.lines = []
        self.locator = None

    def setDocumentLocator(self, locator):
        self.locator = locator

    def startElement(self, name, attrs):
        if name == "message" and self.locator:
            self.lines.append(self.locator.getLineNumber())


def get_exact_message_lines(text: str):
    """Yield exact line numbers for <message> elements using a SAX parser."""
    handler = MessageLocator()
    try:
        xml.sax.parseString(text.encode("utf-8"), handler)
    except xml.sax.SAXException:
        pass
    yield from handler.lines


def check_language_header(ts_file: Path, root, file_lang: str):
    header_lang = root.attrib.get("language", "")
    if header_lang != file_lang:
        msg = f"Language header mismatch '{header_lang}' != '{file_lang}'"
        return [WarningItem(ts_file, 0, file_lang, msg, Severity.WARNING)]
    return []


def check_empty_translation(ctx: MessageContext):
    if not ctx.translation.strip() and ctx.tr_type != "unfinished":
        msg = f"Empty translation for '{ctx.excerpt}'"
        return [WarningItem(ctx.ts_file, ctx.line, ctx.lang, msg, Severity.SEVERE)]
    return []


def check_placeholders(ctx: MessageContext):
    if ctx.tr_type == "unfinished":
        return []
    src_cnt = Counter(PLACEHOLDER_RE.findall(ctx.source))
    tr_cnt = Counter(PLACEHOLDER_RE.findall(ctx.translation))
    if src_cnt != tr_cnt:
        msg = (f"Placeholder mismatch for '{ctx.excerpt}'\n"
               f"Source: {ctx.source}\nTrans:  {ctx.translation}")
        return [WarningItem(ctx.ts_file, ctx.line, ctx.lang, msg, Severity.WARNING)]
    return []


def check_html(ctx: MessageContext):
    if (HTML_TAG_RE.search(ctx.source) and not HTML_TAG_RE.search(ctx.translation)
            and ctx.tr_type != "unfinished"):
        msg = (f"HTML missing for '{ctx.excerpt}'\n"
               f"Source: {ctx.source}\nTrans:  {ctx.translation}")
        return [WarningItem(ctx.ts_file, ctx.line, ctx.lang, msg, Severity.WARNING)]
    return []


def check_whitespace(ctx: MessageContext):
    if not ctx.translation or ctx.tr_type == "unfinished":
        return []
    src_lead = ctx.source != ctx.source.lstrip()
    src_trail = ctx.source != ctx.source.rstrip()
    tr_lead = ctx.translation != ctx.translation.lstrip()
    tr_trail = ctx.translation != ctx.translation.rstrip()
    if src_lead != tr_lead or src_trail != tr_trail:
        msg = f"Leading/trailing whitespace mismatch for '{ctx.excerpt}'"
        return [WarningItem(ctx.ts_file, ctx.line, ctx.lang, msg, Severity.WARNING)]
    return []


def check_newline_consistency(ctx: MessageContext):
    if ctx.source.endswith("\n") != ctx.translation.endswith("\n"):
        msg = f"Newline mismatch for '{ctx.excerpt}'"
        return [WarningItem(ctx.ts_file, ctx.line, ctx.lang, msg, Severity.WARNING)]
    return []


def _extract_message_data(message):
    src_node = message.find("source")
    source = "".join(src_node.itertext()) if src_node is not None else ""
    tr_elem = message.find("translation")
    tr_type, translation = "", ""
    if tr_elem is not None:
        tr_type = tr_elem.attrib.get("type", "")
        forms = tr_elem.findall("numerusform")
        if forms:
            translation = " ".join("".join(n.itertext()) for n in forms)
        else:
            translation = "".join(tr_elem.itertext())
    return source, translation, tr_type


def _process_context(ts_file, file_lang, context, line_gen):
    warnings = []
    for message in context.findall("message"):
        line = next(line_gen, 0)
        src, trans, tr_type = _extract_message_data(message)
        clean = src.strip().replace("\n", " ")
        excerpt = clean[:30] + ("..." if len(clean) > 30 else "")
        ctx = MessageContext(ts_file, line, file_lang, src, trans, tr_type, excerpt)
        for check in [check_empty_translation, check_placeholders, check_html,
                      check_whitespace, check_newline_consistency]:
            warnings.extend(check(ctx))
    return warnings


def detect_warnings(ts_file: Path, file_lang: str):
    try:
        text = ts_file.read_text(encoding="utf-8")
        root = ET.fromstring(text)
    except (OSError, ET.ParseError) as exc:
        return [WarningItem(ts_file, 0, file_lang, f"Error parsing XML: {exc}", Severity.SEVERE)]

    warnings = check_language_header(ts_file, root, file_lang)
    line_gen = get_exact_message_lines(text)
    for context in root.findall("context"):
        warnings.extend(_process_context(ts_file, file_lang, context, line_gen))
    return warnings


def _print_results(grouped):
    for file in sorted(grouped.keys()):
        print(f"\n{BOLD}File: {file.name}{RESET}")
        for w in sorted(grouped[file], key=lambda x: x.line):
            color, sev = (RED, "SEVERE ") if w.severity == Severity.SEVERE else (YELLOW, "WARNING")
            lines = w.message.split("\n")
            print(f"  {CYAN}Line {w.line:<4}{RESET} | {color}{sev}{RESET} | {lines[0]}")
            for extra in lines[1:]:
                print(f"            |         | {extra}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ts-dir", type=Path, default=Path("../src/translation"))
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()

    ts_files = sorted(args.ts_dir.glob("translation_*.ts"))
    if not ts_files:
        return 2

    all_warnings, stats = [], defaultdict(lambda: {"severe": 0, "warning": 0})
    for f in ts_files:
        all_warnings.extend(detect_warnings(f, f.stem.replace("translation_", "")))

    grouped = defaultdict(list)
    for w in all_warnings:
        grouped[w.ts_file].append(w)
        stats[w.lang]["severe" if w.severity == Severity.SEVERE else "warning"] += 1

    _print_results(grouped)

    print("\n== Test Summary ==")
    for lang in sorted(stats.keys()):
        print(f"{BOLD}[{lang}]{RESET} Severe: {stats[lang]['severe']}, "
              f"Warnings: {stats[lang]['warning']}")

    if sum(s["severe"] for s in stats.values()) > 0 or (
            args.strict and sum(s["warning"] for s in stats.values()) > 0):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
