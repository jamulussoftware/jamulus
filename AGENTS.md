# Jamulus — Agent Instructions

Real-time networked music jamming app. Qt/C++ qmake project. Client and server share same codebase. Entry point: `src/main.cpp`. Configure `CONFIG` flags in `Jamulus.pro`.

**Make the smallest possible change. One logical change per PR. Never mix refactoring with fixes/features.**

Priority order: Stability > Low latency / real-time safety > Backwards compatibility > Maintainability > New features.

---

## Never Do

- Block or slow: audio callbacks, socket handling, server mixing timers.
- Allocate excessive memory, do file I/O, or log excessively in real-time paths.
- Trust any value received from a remote client — validate size and bounds on all network input.
- Edit generated files: `moc_*.cpp`, `ui_*.h`, `qrc_*.cpp`, `*.qm`.
- Edit or reformat third-party code: `libs/` (opus, oboe, NSIS).
- Edit `ChangeLog` directly.
- Change architecture without discussion.

## Build

See `COMPILING.md`.

Generally use:
```bash
git submodule update --init   # required: oboe (Android)
qmake && make                                    # Linux (use qmake-qt5 on Fedora)
qmake "CONFIG+=headless serveronly" && make      # headless server
```

macOS (xcode spec) use: `qmake QMAKE_APPLE_DEVICE_ARCHS=arm64 QT_ARCH=arm64 -spec macx-xcode Jamulus.pro`
(Use `x86_64` on Intel Macs; `macx-clang` to build with `make`). Then `xcodebuild build`,
and `macdeployqt ./Release/Jamulus.app` (or `./Debug/Jamulus.app`) to make it runnable.

No automated test suite. State in the PR what and how you tested.

## Qt / portability

- Minimum Qt: **5.12.2**. Qt 6.x.y recommended. Guard newer APIs with `#if QT_VERSION >= QT_VERSION_CHECK(...)`.
- C++11 (C++17 on Android for Oboe's `std::timed_mutex`).
- iOS builds require Qt 5.15 or later (Qt 6 is buggy for iOS).
- Preserve platform support. Don't break Android/iOS builds even if unofficial.
- Supported desktop: Windows 10+, macOS 10.10+, Ubuntu 20.04+/Debian 11+.

## Style (C / C++ / Obj-C++)

```bash
make clang_format   # run before commit (after qmake Jamulus.pro or before .cpp/.h/.mm commits)
```

- **CI uses clang-format** (check `.github/workflows/coding-style-check.yml` for version).
- CI runs **shellcheck + shfmt** on `.sh` files; **pylint** (config: `.pylintrc`) on `.py` files in `tools/`.
- Rules: tabs→4 spaces, braces on own line, space inside `()` and around `if/for/while`, column limit 150, left pointer alignment (`int* p`).
- New files: AGPL 3.0+ license header. Pre-3.12.1dev code: GPL 2.0+ (dual-licensed as part of Jamulus).
- Use `tr ( "Hello %1" ).arg ( name )` for user-facing strings — never string concatenation.

## JSON-RPC

- If you change RPC methods, regenerate `docs/JSON-RPC.md` with `tools/generate_json_rpc_docs.py` (CI fails otherwise).
- Requires `--jsonrpcport` + `--jsonrpcsecretfile` at runtime. Binds to localhost by default.

## PR expectations

- Small changes preferred, every change tested
- Branch `autobuild/<name>` triggers CI builds on your fork.
- Follow `.github/pull_request_template.md`. Include `CHANGELOG:` line with changelog description. For new deps/build changes, add `AUTOBUILD: Please build all targets`.

## Before opening a PR

One logical change per PR — no unrelated cleanup or reformatting of untouched code. Discuss features in an issue before implementing. See `CONTRIBUTING.md`.
Builds? Tested? **Smallest change possible?**

## Read when relevant

- `CONTRIBUTING.md` — process, style, licensing
- `COMPILING.md` — full build per platform, CONFIG flags table
- `docs/JAMULUS_PROTOCOL.md` — network protocol, packet IDs, ack rules
- `SECURITY.md` — security reporting
