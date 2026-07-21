# Jamulus — Agent Instructions

Real-time networked music jamming app. Qt/C++ qmake project. Client and server share same codebase. Entry point: `src/main.cpp`. Configure `CONFIG` flags in `Jamulus.pro`.

**Make the smallest possible change. One logical change per PR. Never mix refactoring with fixes/features.**

Priority order: Stability > Low latency / real-time safety > Backwards compatibility > Maintainability > New features. This order resolves conflicts only — new features are welcome.

---

## Build

Linux: `qmake && make` (use `qmake-qt5` on Fedora). Headless server: `qmake "CONFIG+=headless serveronly" && make`. First run: `git submodule update --init` (oboe for Android). See `COMPILING.md` for the full per-platform table.

macOS (xcode spec) use: `qmake QMAKE_APPLE_DEVICE_ARCHS=arm64 QT_ARCH=arm64 -spec macx-xcode Jamulus.pro`
(Use `x86_64` on Intel Macs; `macx-clang` to build with `make`). Then `xcodebuild build`,
and `macdeployqt ./Release/Jamulus.app` (or `./Debug/Jamulus.app`) to make it runnable.

No test suite: run a headless server, connect a client to `127.0.0.1`, exercise the change, and state in the PR what you tested.

## Never Do

**`Never Do` rules are absolute**

- Block or slow: audio callbacks, socket handling, server mixing timers (stalls = audible dropouts). Preallocate buffers; keep real-time paths lock-free.
- Allocate excessive memory, do file I/O, or log excessively in real-time paths (blocks the audio thread).
- Trust any value received from a remote client — validate size and bounds on all network input (malformed input crashes).
- Edit generated files: `moc_*.cpp`, `ui_*.h`, `qrc_*.cpp`, `*.qm` — regenerate, don't hand-edit.
- Edit or reformat third-party code in `libs/` (e.g. opus, oboe, NSIS).
- Edit `ChangeLog` directly — use a `CHANGELOG:` line in the PR instead.

## Always

- Attach test evidence (logs/output) to the PR — never just assert something works.
- Say so if you did not run or verify something.

## Ask first

- Architecture changes (e.g. networking/protocol, threading, build system) — open an issue to discuss (see `CONTRIBUTING.md`).

## Qt / portability

- Minimum Qt: **5.12.2**. Qt 6 recommended (not for iOS, see below). Guard newer APIs with `#if QT_VERSION >= QT_VERSION_CHECK(...)`.
- C++11 (C++17 on Android for Oboe).
- iOS builds require Qt 5.15 or later (Qt 6 buggy on iOS).
- Preserve platform support. Don't break Android/iOS builds.
- Supported desktop: Windows 10+, macOS 10.10+, Ubuntu 20.04+/Debian 11+.

## Style (C / C++ / Obj-C++)

```bash
make clang_format   # run before committing (target exists only after qmake generated Makefile)
```

- **CI uses clang-format** (check `.github/workflows/coding-style-check.yml` for version).
- CI runs **shellcheck + shfmt** on `.sh` files; **pylint** (config: `.pylintrc`) on `.py` files in `tools/`.
- All new contributions: AGPL 3.0+ license header. Pre-3.12.1dev code: GPL 3.0+ (see `CONTRIBUTING.md`).
- Use `tr ( "Hello %1" ).arg ( name )` for user-facing strings — never string concatenation.

## JSON-RPC

- If change RPC methods are changed, regenerate `docs/JSON-RPC.md` with `tools/generate_json_rpc_docs.py` (CI fails otherwise).
- Requires `--jsonrpcport` + `--jsonrpcsecretfile` at runtime. Binds to localhost by default.

## PR expectations

- One logical change per PR — no unrelated cleanup or reformatting of untouched code. Discuss features in an issue before implementing. See `CONTRIBUTING.md`.
- Branch `autobuild.*` triggers CI builds on your fork.
- Follow `.github/pull_request_template.md`. Include `CHANGELOG:` line with changelog description. For new deps/build changes, add `AUTOBUILD: Please build all targets`.
- Builds? Tested? Smallest change possible? Self reviewed against "Priority order" above?
- Disclose AI-generated text in PRs/issues — never in code comments.

## Read when relevant

- `CONTRIBUTING.md` — process, style, licensing
- `COMPILING.md` — full build per platform, CONFIG flags table
- `docs/JAMULUS_PROTOCOL.md` — network protocol, packet IDs, ack rules
- `SECURITY.md` — security reporting
