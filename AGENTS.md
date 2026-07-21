# Jamulus — Agent Instructions

Real-time networked music jamming app. Qt/C++. Entry point: `src/main.cpp`. Configure `CONFIG` flags in `Jamulus.pro`.

## DO NOT edit (generated or third-party)
- `moc_*.cpp`, `ui_*.h`, `qrc_*.cpp` are generated at build time (moc/uic/rcc) — never hand-edit.
- `*.qm` under `.qm/` are committed compiled translations — never hand-edit (regenerate from the `.ts` sources).
- `libs/` is third-party (opus, oboe, NSIS). `libs/oboe` is a git submodule — run
  `git submodule update --init` if it is empty. Do not apply clang format on any of those files.

## Build / verify
- Linux desktop: `qmake && make` (use `qmake-qt5` on Fedora).
- Headless server: `qmake "CONFIG+=headless serveronly" && make`.
- macOS: `qmake QMAKE_APPLE_DEVICE_ARCHS=arm64 QT_ARCH=arm64 -spec macx-xcode Jamulus.pro`
  (use `x86_64` on Intel Macs; `macx-clang` to build with `make`). Then `xcodebuild build`,
  and `macdeployqt ./Release/Jamulus.app` to make it runnable. More infos: COMPILING.md.

## Style (C / C++ / Obj-C++)
- Before committing `.cpp/.h/.mm` or after `qmake Jamulus.pro`, run `make clang_format`.
- **CI uses clang-format** (see `.github/workflows/coding-style-check.yml`
  `clangFormatVersion` for version).
- Rules: tabs→4 spaces, braces on their own line, space inside `()` and around
  `if/for/while` conditions, column limit 150, left pointer alignment (`int* p`).
- New files must carry an AGPL 3.0 (or later) license header.
- Existing pre-3.12.1 code being amended should already carry combined AGPL 3.0 and GPL 2.0 (or later) license blocks.
  code is GPL 2.0+.

## Tests
- No automated test suite.

## Translations
- Use `tr ( "Hello %1" ).arg ( name )` — never concatenate translatable strings with `+`.
- Per-language `.ts` files live in `src/translation/`.

## JSON-RPC
- If you change RPC methods, regenerate `docs/JSON-RPC.md` with
  `tools/generate_json_rpc_docs.py` (CI fails otherwise — `check-json-rpcs-docs.yml`).

## Other tooling
- `.sh` files: lint with `shellcheck --shell=bash` and `shfmt -d`.
- Python (under `tools/`): max line length 99, run `pylint` (< 3.0) with `.pylintrc`.
- ChangeLog: put `CHANGELOG: <one sentence>` in the PR description. Do **not** edit
  the `ChangeLog` file (causes conflicts).

## Version / portability constraints

- Minimum Qt: **5.12**. Guard newer APIs with `#if QT_VERSION >= QT_VERSION_CHECK(...)`.
- Preserve C++11/17 and Android/iOS compatibility.

## PR expectations
- Branch named `autobuild/<name>` triggers CI builds on your fork.
- Include `CHANGELOG:` line with a brief changelog discription and, for new deps/build changes, the
  `AUTOBUILD: Please build all targets` tag.

## Before finishing

- Do not modify generated or third-party files.
- Format code.
- Ensure changes build for the affected target(s).
