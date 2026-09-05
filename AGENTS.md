# Jamulus — Agent Instructions

Real-time networked music jamming app. Qt/C++ qmake project. Client and server share one codebase; entry point: `src/main.cpp`. Configure `CONFIG` flags in `Jamulus.pro`.

**[`CONTRIBUTING.md`](CONTRIBUTING.md) is the source of truth for everything this project requires of a contribution; this file does not restate those requirements, and defers to it if the two ever disagree.** Read it before changing code, and before opening or commenting on an issue, Pull Request or discussion here.

What is below is orientation only: where things are, and how to build and run them.

---

## Build

Linux: `qmake && make` (use `qmake-qt5` on Fedora). Headless server: `qmake "CONFIG+=headless serveronly" && make`. First run: `git submodule update --init` (oboe for Android). Run `make distclean` before re-running `qmake` with different `CONFIG` flags. Full per-platform table: `COMPILING.md`.

macOS: `qmake QMAKE_APPLE_DEVICE_ARCHS=arm64 QT_ARCH=arm64 -spec macx-xcode Jamulus.pro` (Use `x86_64` on Intel Macs; `macx-clang` if using `make`). Then `xcodebuild build`, and `macdeployqt ./{Debug,Release}/Jamulus.app`.

## Run it

A plain build gives one binary that is both client and server. Run the server headless with `-s -n`; connect a client with `-n -c localhost` (on Linux this may need jackd — `jackd -d dummy`). A `CONFIG+=serveronly` binary rejects `-c`. Drive it through the JSON-RPC API where that is possible: it needs `--jsonrpcport` and `--jsonrpcsecretfile`; see `docs/JSON-RPC.md`. GitHub Actions builds several platforms; on failure, read the failing step's log.

## Where the rules are

| Before you… | Read |
|---|---|
| start writing anything at all | [the opening bullets](CONTRIBUTING.md#contributing-to-jamulus) |
| resolve a design tradeoff | [general principles](CONTRIBUTING.md#jamulus-projectsource-code-general-principles) |
| touch `src/sound`, `src/socket.cpp` or `src/server.cpp` | [Real-time safety](CONTRIBUTING.md#real-time-safety) |
| parse anything that arrived over the network | [Input arriving over the network](CONTRIBUTING.md#input-arriving-over-the-network) |
| change an existing protocol message | [Wire compatibility](CONTRIBUTING.md#wire-compatibility) |
| format code | [Source code consistency](CONTRIBUTING.md#source-code-consistency) |
| edit a generated file or `libs/` | [Files not to edit by hand](CONTRIBUTING.md#files-not-to-edit-by-hand) |
| use AI for any part of the work | [Using AI](CONTRIBUTING.md#using-ai) |
| add a file, or copy code in | [Licensing](CONTRIBUTING.md#licensing) |
| use a Qt or C++ feature that may be too new | [Supported platforms](CONTRIBUTING.md#supported-platforms) |
| add a dependency | [Dependencies](CONTRIBUTING.md#dependencies) |
| write user-facing text | [User experience](CONTRIBUTING.md#user-experience) |
| open a Pull Request | [Submitting code](CONTRIBUTING.md#submitting-code-and-getting-started), [Testing](CONTRIBUTING.md#testing), [Ownership](CONTRIBUTING.md#ownership) |
| post a comment or a review | [Commenting and reviewing](CONTRIBUTING.md#commenting-and-reviewing), and `docs/agents/COMMENTING.md` |
| write a `CHANGELOG:` line | [Documentation/Acknowledgements](CONTRIBUTING.md#documentationacknowledgements) |

## Read when relevant

- `COMPILING.md` — full build per platform, CONFIG flags table
- `docs/JAMULUS_PROTOCOL.md` — network protocol, packet IDs, ack rules
- `docs/agents/COMMENTING.md` — commenting on GitHub
- `SECURITY.md` — security reporting
