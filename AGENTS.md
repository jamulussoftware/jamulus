# Jamulus — Agent Instructions

Jamulus is a real-time networked music jamming application (client and server). Qt/C++, built with qmake. Entry point: `src/main.cpp`; build variants via `CONFIG` flags in `Jamulus.pro`.

This file is the starting point for coding agents. It links to the detailed docs — read the relevant one before starting:

- [CONTRIBUTING.md](CONTRIBUTING.md) — process, code style, licensing, ownership. Its rules apply to agent-assisted work without exception.
- [COMPILING.md](COMPILING.md) — building on every supported platform.
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — source map, threading model, and the invariants that protect live performances.
- [docs/DEPLOY.md](docs/DEPLOY.md) — moving a self-built server binary onto production hosts and verifying it.
- [docs/JAMULUS_PROTOCOL.md](docs/JAMULUS_PROTOCOL.md) — the wire protocol.
- [SECURITY.md](SECURITY.md) — report vulnerabilities to team@jamulus.io, never in a public issue.

## Scope

- One logical change per PR. No drive-by refactors, no reformatting of code you didn't otherwise touch, no bundled extras.
- Features must be agreed in a GitHub issue or Discussion **before** coding — see CONTRIBUTING.md. If no agreement exists, propose; don't implement.
- Stability outranks everything: Jamulus runs live performances. Prefer not adding a feature over adding risk (KISS principles in CONTRIBUTING.md).

## Architecture invariants

Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) before changing code. Its invariants, in brief:

1. Never block the real-time audio path (sound callbacks, socket thread, server mix timer).
2. All network input is untrusted — bounds-check everything; drop malformed packets silently.

## DO NOT edit (generated or third-party)

- `moc_*.cpp`, `ui_*.h`, `qrc_*.cpp` are generated at build time (moc/uic/rcc) — never hand-edit.
- `*.qm` files are compiled translations — regenerate from the `.ts` sources in `src/translation/`, never hand-edit.
- `libs/` is third-party (opus, oboe, NSIS). `libs/oboe` is a git submodule — run `git submodule update --init` if it is empty.
- Never run clang-format on any of the above.

## Build / verify

- Linux desktop: `qmake && make` (`qmake-qt5` on Fedora).
- Headless server: `qmake "CONFIG+=headless serveronly" && make`.
- macOS: `qmake QMAKE_APPLE_DEVICE_ARCHS=arm64 QT_ARCH=arm64 -spec macx-xcode Jamulus.pro` (use `x86_64` on Intel Macs; `macx-clang` to build with `make`), then `xcodebuild build` and `macdeployqt ./Release/Jamulus.app`. Full platform details: [COMPILING.md](COMPILING.md).
- There is no automated test suite. To verify a change: build a headless server and run `./Jamulus --server --nogui`, connect a client build to `127.0.0.1`, and exercise the changed behavior. State in the PR what you tested and on which platform. An untested change is an unfinished change.

## Style (C / C++ / Obj-C++)

- Run `make clang_format` before committing (the target exists once `qmake` has generated the Makefile). CI enforces a specific clang-format version — see `clangFormatVersion` in `.github/workflows/coding-style-check.yml`.
- If you add a new source directory or file extension, update `.github/workflows/coding-style-check.yml` and `.clang-format-ignore` as well as `Jamulus.pro` (see the comment above `CLANG_FORMAT_SOURCES`) — otherwise CI and local formatting silently diverge.
- Rules in brief: 4-space indent (no tabs), braces on their own line, space inside `()` and around `if`/`for`/`while` conditions, column limit 150, left pointer alignment (`int* p`).
- New files need an AGPL 3.0 (or later) license header; pre-3.12.1 files carry combined GPL/AGPL blocks — leave existing headers alone. Details in CONTRIBUTING.md.

## Translations

- Use substitution, never concatenation: `tr ( "Hello, %1!" ).arg ( name )` — concatenated fragments cannot be translated.
- Per-language `.ts` files live in `src/translation/`; see [docs/TRANSLATING.md](docs/TRANSLATING.md).

## JSON-RPC

- If you change RPC methods, regenerate `docs/JSON-RPC.md` with `tools/generate_json_rpc_docs.py` — CI (`check-json-rpcs-docs.yml`) fails otherwise.

## Other tooling

- `.sh` files: lint with `shellcheck --shell=bash` and `shfmt -d`.
- Python (under `tools/`): max line length 99, run `pylint` (< 3.0) with the repo's `.pylintrc`.
- ChangeLog: put `CHANGELOG: <one sentence>` in the PR description (`CHANGELOG: SKIP` for changes users won't notice). Do **not** edit the `ChangeLog` file — it causes conflicts.

## Version / portability constraints

- Minimum Qt is **5.12.2**. Guard newer APIs with `#if QT_VERSION >= QT_VERSION_CHECK(...)`.
- Maintain C++11 compatibility and keep Windows, macOS, Linux, Android and iOS building.

## PR expectations

- Naming your branch `autobuild/<name>` triggers CI builds of all targets on your fork — use it before opening the PR.
- Include the `CHANGELOG:` line, and for new dependencies or build changes add `AUTOBUILD: Please build all targets` to the PR description.
