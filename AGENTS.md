# Jamulus — Agent Instructions

Real-time networked music jamming app. Qt/C++ qmake project. Client and server share one codebase; entry point: `src/main.cpp`. Configure `CONFIG` flags in `Jamulus.pro`.

**Make the smallest possible change. One logical change per PR. Never mix refactoring with fixes/features.**

Priority order: Stability > Low latency / real-time safety > Backwards compatibility > Maintainability > New features. This order resolves conflicts only — new features are welcome.

---

## Build

Linux: `qmake && make` (use `qmake-qt5` on Fedora). Headless server: `qmake "CONFIG+=headless serveronly" && make`. First run: `git submodule update --init` (oboe for Android). Run `make distclean` before re-running `qmake` with different `CONFIG` flags. Full per-platform table: `COMPILING.md`.

macOS: `qmake QMAKE_APPLE_DEVICE_ARCHS=arm64 QT_ARCH=arm64 -spec macx-xcode Jamulus.pro` (Use `x86_64` on Intel Macs; `macx-clang` if using `make`). Then `xcodebuild build`, and `macdeployqt ./{Debug,Release}/Jamulus.app`.

**Testing:** run headless server (args `-s -n`), connect a client (e.g. via: `-n -c localhost`; may need jackd running on Linux. Run dummy Jack via: `jackd -d dummy`), exercise the change; use the JSON-RPC API (`docs/JSON-RPC.md`) where possible. State what you tested in the PR with evidence. GitHub Actions builds multiple platforms — on failure read the failing step's log.

## Never Do

**`Never Do` rules are absolute**

- Introduce code that prevents processing of audio within the minimum cycle time for any frame (i.e. worst case must remain viable); DO test this and produce evidence to support the change
  - this covers sound process in `src/sound`, network processing in `src/socket.cpp` and mixing in `src/server.cpp`
  - potential problems include (but not limited to): memory allocation, file I/O, locks
  - where possible, move processing off the real-time thread with queued signals
- Trust values from remote clients — validate size/bounds on all network input (malformed input crashes).
- Edit generated files (`moc_*.cpp`, `ui_*.h`, `qrc_*.cpp`, `*.qm`) — regenerate; don't edit/reformat third-party code in `libs/`.
- Edit `ChangeLog` directly — use a `CHANGELOG:` line in the PR.

## Always

- Attach test evidence (logs/output) to the PR — never just assert something works.
- Say so if you did not run or verify something.

## Ask first

- Architecture changes (networking/protocol, threading, build system) — open an issue to discuss (see `CONTRIBUTING.md`).

## Qt / portability

- Minimum Qt: **5.12.2**. Qt 6 recommended (iOS: Qt 5.15+ required, Qt 6 iOS buggy). Guard newer APIs with `#if QT_VERSION >= QT_VERSION_CHECK(...)`.
- C++11 (C++17 on Android for Oboe).
- Preserve platform support.
- Desktop: Windows 10+, macOS 10.10+, Ubuntu 20.04+/Debian 11+.

## Style (C / C++ / Obj-C++)

- **CI uses clang-format** (version in `.github/workflows/coding-style-check.yml`).
- Run `make clang_format` before committing (works only after qmake).
- CI runs **shellcheck + shfmt** on `.sh` files; **pylint** (config: `.pylintrc`) on `.py` files in `tools/`.
- New contributions: AGPL 3.0+ license header. Pre-3.12.1dev code: GPL 3.0+ (see `CONTRIBUTING.md`).
- Use `tr ( "Hello %1" ).arg ( name )` for user-facing strings — never string concatenation.

## JSON-RPC

- Changing RPC methods (e.g. `src/clientrpc.cpp` / `src/serverrpc.cpp`) requires regenerating `docs/JSON-RPC.md` with `tools/generate_json_rpc_docs.py` (CI fails otherwise).
- Requires `--jsonrpcport` + `--jsonrpcsecretfile` at runtime. Binds to localhost by default. Secret requires ≥16 characters.

## PR expectations

- One logical change per PR — no unrelated cleanup or reformatting of untouched code. Discuss features in an issue before implementing. See `CONTRIBUTING.md`.
- Branch names starting with `autobuild` trigger CI builds on your fork.
- Follow `.github/pull_request_template.md`. Include `CHANGELOG:` line. Add `AUTOBUILD: Please build all targets` for skipped targets (iOS, Windows JACK, Linux armhf/arm64) if touched; see `.github/workflows/autobuild.yml`.
- Builds? Tested? Smallest change possible? Self reviewed against "Priority order" above?
- Disclose AI-generated text at the end of Comments/PRs. (e.g: `> 🤖 Used AI: <model>, <harness>`) — never in code comments.

## Read when relevant
- `CONTRIBUTING.md` — process, style, licensing
- `COMPILING.md` — full build per platform, CONFIG flags table
- `docs/JAMULUS_PROTOCOL.md` — network protocol, packet IDs, ack rules
- `docs/agents/COMMENTING.md` — rules when commenting on GitHub
- `SECURITY.md` — security reporting
