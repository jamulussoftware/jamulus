# Jamulus — Agent Instructions

Real-time networked music jamming app. Qt/C++ qmake project. Client and server share one codebase; entry point: `src/main.cpp`. Qt project configuration in `Jamulus.pro`.

**[`CONTRIBUTING.md`](CONTRIBUTING.md) is the source of truth for everything this project requires of a contribution; this file does not restate those requirements, and defers to it if the two ever disagree.** Read it before changing code, and before opening or commenting on an issue, Pull Request or discussion here.

**Make the smallest possible change. One logical change per PR. Never mix refactoring with fixes/features.**

Priority order: Stability > Low latency / real-time safety > Backwards compatibility > Maintainability > New features. This order resolves conflicts only — new features are welcome.

What is below is orientation only: where things are, and how to build and run them.

---

## Build and Test

**Before running a build**, read `COMPILING.md` for your compile target. It includes build commands, platform-specific dependencies and `CONFIG` flags. `.github/autobuild` contains the build scripts for the GitHub Actions workflow. Read these files if you are stuck and need an example.

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
| build for a platform other than the two above | [`COMPILING.md`](COMPILING.md) |
| change how clients, servers and directories talk to each other | [`docs/JAMULUS_PROTOCOL.md`](docs/JAMULUS_PROTOCOL.md) |
| report a security vulnerability — never as an issue | [`SECURITY.md`](SECURITY.md) |
