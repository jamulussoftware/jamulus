# Contributing to Jamulus

We’d really appreciate your support! Please ensure that you understand the following in order to keep things organized:

- If a [Github issue](https://github.com/jamulussoftware/jamulus/issues) for your feature/bug fix already exists, write a message in that issue indicating that you want to work on it.

- Otherwise, please [post on the GitHub Discussions](https://github.com/jamulussoftware/jamulus/discussions) and say that you are planning to do some coding and explain why. Then we can discuss the specification.
- Please begin coding only after we have agreed on a specification to avoid putting a lot of effort into something that may not be accepted later. Changes to networking or the wire protocol, to threading, or to the build system always need this discussion first.

- Keep commits and Pull Requests focussed on one logical change only, and make the smallest change that does it. Do not mix refactoring with a fix or a feature, and do not reformat untouched code: both hide the change under review. In case you implement multiple features, open multiple smaller PRs instead of large one. Large PRs may become stale since they are not reviewable and be closed after a long time of inactivity.

## Jamulus project/source code general principles

Where these principles pull against each other, resolve the conflict in this order: **stability > low latency and real-time safety > backwards compatibility > maintainability > new features.** This order settles conflicts only - new features are welcome (but read item 2 below first).

### 1. Stability

Instabilities during live performances such as WorldJam are not acceptable. As a result, stability has been, and must continue to be the most important requirement. The following principles are designed to support this.

#### Real-time safety

Do not introduce code that prevents processing of audio within the _minimum_ cycle time for _any_ frame (i.e. worst case must remain viable); DO test this and produce evidence to support the change.

- This covers sound processing in `src/sound`, network processing in `src/socket.cpp` and mixing in `src/server.cpp`.
- Potential problems include (but are not limited to): memory allocation, file I/O, locks.
- Where possible, move processing off the real-time thread with queued signals.

#### Input arriving over the network

Do not trust values sent by remote clients, servers or directories. Validate the size and bounds of everything read from the network before it reaches an array index, a length calculation or an allocation. Malformed input is how a crash gets into a release; see [SECURITY.md](SECURITY.md) for reporting one you find in a released version.

#### Wire compatibility

Clients and servers of different versions have to keep understanding each other, so do not renumber `PROTMESSID_*` and do not change the layout of a protocol message that already exists. Retired message IDs stay reserved - see the `OLD` entries in `src/protocol.h`. Extend the protocol by adding a new message ID.

### 2. [Keep it Simple and Stupid](https://en.wikipedia.org/wiki/KISS_principle) and 3. [Do One Thing and Do It Well](https://en.wikipedia.org/wiki/Unix_philosophy#Do_One_Thing_and_Do_It_Well)

If a feature or function can be accomplished in another way by another system or method, it is preferable not to build that feature into Jamulus. Rather than implementing each and every feature as part of Jamulus, we concentrate on a stable core and implement interfaces for interaction with third-party components as needed. The [JSON-RPC](https://github.com/jamulussoftware/jamulus/blob/main/docs/JSON-RPC.md) API for example, allows you to communicate with the client and server from outside the application.

### Source code consistency

#### C-like languages
Please install and run `clang-format` on your PC **before committing** to maintain a consistent coding style. You should use the version we use to validate the style in our CI [(see our coding-style-check file and look for the `clangFormatVersion`)](https://github.com/jamulussoftware/jamulus/blob/main/.github/workflows/coding-style-check.yml#L20). Our CI will fail and tell you about styling violations.

There are several ways to run clang-format:

- Use your editor's or IDE's clang-format support.

- Via make: after running `qmake Jamulus.pro`, run `make clang_format`

- By hand: run `clang-format -i <path/to/changed/files>`

Adding a source directory or a new file extension? `make clang_format` and the CI check read separate lists, and the [workflow's own comment](.github/workflows/coding-style-check.yml) says to update all three together: its `extensions:` list, its `paths:` filter, and `CLANG_FORMAT_SOURCES` in `Jamulus.pro`.

##### Style definition

Please see the [.clang_format file](https://github.com/jamulussoftware/jamulus/blob/main/.clang-format) in the root folder. In summary:

- Respect the existing code style: Tab size=4, insert spaces.

- Insert a space before and after `(` and `)`. There should be no space between `)` and `;` or before an empty `()`.
- Enclose all bodies of `if`, `else`, `while`, `for`, etc. in braces `{` and `}` on separate lines.
- Do not use concatenations in strings with parameters. Instead use substitutions. **Do:** `QString ( tr ( "Hello, %1. Have a nice day!" ) ).arg( getName() )` **Don't:** `tr ( "Hello " ) + getName() + tr ( ". Have a nice day!" )` ...to make translation easier.

#### Python
Please install and use [pylint](https://pylint.org/) to scan any Python code.
There is a configuration file that defines some overrides,
and note the [Editorconfig file](.editorconfig) in the project too.

#### Shell scripts
Our CI runs [shellcheck](https://www.shellcheck.net/) and `shfmt` on `.sh` files. Please run both before committing.

#### Files not to edit by hand

- Generated sources - `moc_*.cpp`, `ui_*.h`, `qrc_*.cpp` and `*.qm` - are build products. Change what they are generated from and regenerate.
- Third-party code under `libs/` keeps its upstream formatting. Do not reformat it, and keep any change to it to the minimum needed.
- `docs/JSON-RPC.md` is generated. If you change a JSON-RPC method (for example in `src/clientrpc.cpp` or `src/serverrpc.cpp`), regenerate the document with `tools/generate_json_rpc_docs.py` in the same Pull Request - our CI fails otherwise.
- The `ChangeLog` file - see [Documentation/Acknowledgements](#documentationacknowledgements) below.

### Using AI

- When you use AI as part of your work, remember that it is a tool that you choose to use and your code will be judged in the same way as any other submission.  However, we encourage you to share your AI experiences, as it is an emerging technology, by highlighting how you used AI and give your own review of its performance.

Please disclose AI-generated text at the end of the comment, issue or Pull Request description that carries it, for example `> 🤖 Used AI: <model>, <harness>`. Do not put such notes in code comments.

AI-assisted contributions **must** follow the same standards as every other contribution. The submitter remains the author and is expected to understand and stand behind every submitted line. [AGENTS.md](AGENTS.md) is the entry point into this repository for AI Agents. Low-effort contributions might be closed without comment.

### Licensing

**As of Jamulus 3.12.1dev commit eb172d47:** All new source code contributions must be licensed under AGPL 3.0 or any later version.

**Existing code:** Code contributed before commit eb172d47 was licensed under GPL 2.0+.
This code will be licensed under GPL 3.0 (or any later version) from commit eb172d47.
When distributed as part of Jamulus, the AGPL 3.0 terms govern the combined work, including network use provisions.

**New files**: Any completely new file should include a header with:
* Copyright line(s)
* Author(s)/copyright holder(s)
* License declaration: "Licensed under AGPL 3.0 or any later version. See COPYING for details."

(The warranty disclaimer and equivalent standard text from the existing files is not required, but may be included if desired.)

**Note:** We adopted AGPL 3.0 to ensure that modifications and derivative works developed for network services
remain available to users.


### Supported platforms

We support the following platforms and versions:

- **Windows 10** or later
- **macOS 10.10** or later
- **Ubuntu 20.04** or later, **Debian 11** or later, most Linux flavors with recent enough Qt versions. (Currently, it may still build on Ubuntu 18.04 and Debian 10, but the binaries built by Github will not run on those versions.)

_While Android and iOS aren't officially supported, please don't break their builds._

Please try to avoid breaking any build by introducing platform-specific code. Check the Github builds all worked before raising a pull request.
Check to see if any newly introduced Qt calls, parameters, properties or constants are available in the minimum supported Qt version, which is currently **5.12.2**. Note that code _style_ in a file may be Qt 4.x. While you should normally stick to existing style, if you make large-scale modifications, updating to Qt 5.12.2 style is recommended.
Guard any call that needs a newer Qt with `#if QT_VERSION >= QT_VERSION_CHECK(...)`.
Maintain C++11 compatibility throughout the code (the Android build uses C++17 for Oboe).

### Dependencies

If your code requires new dependencies, ensure that they are available on all supported platforms and that their inclusion has been discussed and approved. Include the `AUTOBUILD: Please build all targets` tag in your pull request to confirm Github builds for all supported platforms are successful.

### User experience

Jamulus is used by people from all over the world with different backgrounds and levels of knowledge.
Features should be usable in the sense that they behave as expected by someone without a technical background.

To maintain consistency in language, follow the [style and tone guide](https://jamulus.io/contribute/Style-and-Tone). In terms of the UI, prefer standard approaches to exotic ones.

### Submitting code and getting started

We're using git to develop Jamulus. To contribute, you should get familiar to git and GitHub.

Have a look at our [guide for translators](docs/TRANSLATING.md) - especially read the git related part. If you need more in depth information, the [git-scm book](https://git-scm.com/book/en/v2) might also help you getting started. If you have any questions, don't hesitate to ask, as git can be very confusing.

Please fill in the [Pull Request template](.github/pull_request_template.md) - it is the checklist reviewers work from.

Some build targets (iOS, Windows JACK, Linux armhf/arm64) are skipped by default. If your change touches one of them, add the `AUTOBUILD` tag described under [Dependencies](#dependencies) to the Pull Request description.

### Testing

To check that there are no errors, please perform an appropriate local (build/feature/typo) test depending on what you did. Briefly explain in the PR what and how you tested your work and attach evidence - logs, screenshots, small testing scripts or similar depending on your change. If you did not run or verify part of your change, say so rather than leaving it implied.
Keep an eye on the CI checks for quality or compile issues after opening a pull request and fix them as needed. You can also test the build on your repository by naming your branch `autobuild/<branchName>` which will start the building process on your repo.

### Ownership

The submitter of an Issue or a Pull Request is responsible for its care and feeding (this also holds for contributions that were assisted by AI), answering all questions directed at them, and making agreed changes if necessary. In case you use AI and do not understand some outputs, clarify before submission if possible (for example, by asking the project team or an AI agent) or else clearly state this.

Authors are strongly encouraged to update their initial posts/PR descriptions or title to reflect the current state of play, amends, enhancements, outstanding issues, etc., to reduce effort for others in understanding a PR or an Issue.
Admins reserve the right to do this as they see fit.

### Commenting and reviewing

- Test what you can test before you claim it - a build, a log, a run - and cut what you cannot. Words like *presumably*, *should* and *likely* usually mark a sentence that needs a measurement, or needs deleting.
- Comment when you add evidence or an answer the thread does not have yet, in the shortest form that carries it. Let an exchange between others finish, and re-read the thread just before posting - it may have moved while you were writing.
- If a comment turns out to be wrong or incomplete, edit it so that the error leaves the page. Further evidence about the same finding belongs in that comment rather than in a new one.
- Open an issue for a defect you can reproduce, and put the reproduction in the body.
- If someone states how they want to be engaged on a thread - for example, no AI-written replies - follow it while it stands. Disagreeing is welcome: say so once, with your reason; a preference can rest on a misunderstanding on either side.

### Documentation/Acknowledgements

The ChangeLog must be updated for each new feature or bug fix. Please include a single-sentence suggestion for that as part of your pull request description after the `CHANGELOG: ` keyword. Do not modify the ChangeLog file as part of your PR as it will lead to conflicts.

If you are a first-time contributor/translator, please add your name to the contributors/translators list in the About dialog of Jamulus (see in `src/util.cpp` in the constructor function `CAboutDlg::CAboutDlg()`).

### Merging pull requests

The git main branch is protected and requires at least two reviews by the main developers before the pull request can be merged. Any of the main developers can initiate the merge if a pull request receives at least two positive reviews.

---

## Want to get involved in other ways?

We always need help with documentation, [translation](docs/TRANSLATING.md) and anything else. Feel free to look at the [Website repository](https://github.com/jamulussoftware/jamuluswebsite) or get involved in fixing issues from the [issue tracker](https://github.com/jamulussoftware/jamulus/issues).
