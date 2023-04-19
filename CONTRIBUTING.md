# Contributing to Jamulus

We’d really appreciate your support! Please ensure that you understand the following in order to keep things organized:

- If a [Github issue](https://github.com/jamulussoftware/jamulus/issues) for your feature/bug fix already exists, write a message in that issue indicating that you want to work on it.

- Otherwise, please [post on the GitHub Discussions](https://github.com/jamulussoftware/jamulus/discussions) and say that you are planning to do some coding and explain why. Then we can discuss the specification.
- Please begin coding only after we have agreed on a specification to avoid putting a lot of effort into something that may not be accepted later.


## Jamulus project/source code general principles

### 1. Stability

Instabilities during live performances such as WorldJam are not acceptable. As a result, stability has been, and must continue to be the most important requirement. The following principles are designed to support this.

### 2. [Keep it Simple and Stupid](https://en.wikipedia.org/wiki/KISS_principle) and 3. [Do One Thing and Do It Well](https://en.wikipedia.org/wiki/Unix_philosophy#Do_One_Thing_and_Do_It_Well)

If a feature or function can be accomplished in another way by another system or method, it is preferable not to build that feature into Jamulus. Rather than implementing each and every feature as part of Jamulus, we concentrate on a stable core and implement interfaces for interaction with third-party components as needed. The [JSON-RPC](https://github.com/jamulussoftware/jamulus/blob/main/docs/JSON-RPC.md) API for example, allows you to communicate with the client and server from outside the application.

### Source code consistency

#### C-like languages
Please install and run `clang-format` on your PC **before committing** to maintain a consistent coding style. You should use the version we use to validate the style in our CI [(see our coding-style-check file and look for the `clangFormatVersion`)](https://github.com/jamulussoftware/jamulus/blob/main/.github/workflows/coding-style-check.yml#L20). Our CI will fail and tell you about styling violations.

There are several ways to run clang-format:

- Use your editor's or IDE's clang-format support.

- Via make: after running `qmake Jamulus.pro`, run `make clang_format`

- By hand: run `clang-format -i <path/to/changed/files>`

##### Style definition

Please see the [.clang_format file](https://github.com/jamulussoftware/jamulus/blob/main/.clang-format) in the root folder. In summary:

- Respect the existing code style: Tab size=4, insert spaces.

- Insert a space before and after `(` and `)`. There should be no space between `)` and `;` or before an empty `()`.
- Enclose all bodies of `if`, `else`, `while`, `for`, etc. in braces `{` and `}` on separate lines.
- Do not use concatinations in strings with parameters. Instead use substitutions. **Do:** `QString ( tr ( "Hello, %1. Have a nice day!" ) ).arg( getName() )` **Don't:** `tr ( "Hello " ) + getName() + tr ( ". Have a nice day!" )` ...to make translation easier.

#### Python
Please install and use [pylint](https://pylint.org/) to scan any Python code.
There is a configuration file that defines some overrides,
and note the [Editorconfig file](.editorconfig) in the project too.


### Supported platforms

We support the following platforms and versions:

- **Windows 10** or later
- **macOS 10.10** or later
- **Ubuntu 18.04** or later, **Debian 10** or later, most Linux flavors with recent enough Qt versions

_While Android and iOS aren't officially supported, please don't break their builds._

Please try to avoid breaking any build by introducing platform-specific code. Check to see if any newly introduced Qt calls, parameters, properties or constants are available in the minimum supported Qt version, which is currently **5.9**. Note that code _style_ in a file may be Qt 4.x. While you should normally stick to existing style, if you make large-scale modifications, updating to Qt 5.9 style is recommended.
Maintain C++11 compatibility throughout the code.

### Dependencies

If your code requires new dependencies, ensure that they are available on all supported platforms and that their inclusion has been discussed and approved.

### User experience

Jamulus is used by people from all over the world with different backgrounds and levels of knowledge.
Features should be usable in the sense that they behave as expected by someone without a technical background.

To maintain consistency in language, follow the [style and tone guide](https://jamulus.io/contribute/Style-and-Tone). In terms of the UI, prefer standard approaches to exotic ones.

### Submitting code and getting started

We're using git to develop Jamulus. To contribute, you should get familiar to git and GitHub.

Have a look at our [guide for translators](docs/TRANSLATING.md) - especially read the git related part. If you need more in depth information, the [git-scm book](https://git-scm.com/book/en/v2) might also help you getting started. If you have any questions, don't hesitate to ask, as git can be very confusing.

### Testing

To check that there are no errors, please run a local (build/feature) test. Keep an eye on the CI checks for quality or compile issues after opening a pull request and fix them as needed. You can also test the build on your repository by naming your branch `autobuild/<branchName>` which will start the building process on your repo.

### Ownership

The submitter of an Issue or a PR is responsible for its care and feeding, answering all questions directed at them, and making agreed changes if necessary.

Authors are strongly encouraged to update their initial posts/PR descriptions or title to reflect the current state of play, amends, enhancements, outstanding issues, etc., to reduce effort for others in understanding a PR or an Issue.
Admins reserve the right to do this as they see fit.

### Documentation/Acknowledgements

The ChangeLog must be updated for each new feature or bug fix. Please include a single-sentence suggestion for that as part of your pull request description after the `CHANGELOG: ` keyword. Do not modify the ChangeLog file as part of your PR as it will lead to conflicts.

If you are a first-time contributor/translator, please add your name to the contributors/translators list in the About dialog of Jamulus (see in `src/util.cpp` in the constructor function `CAboutDlg::CAboutDlg()`).

### Merging pull requests

The git main branch is protected and requires at least two reviews by the main developers before the pull request can be merged. Any of the main developers can initiate the merge if a pull request receives at least two positive reviews.

---

## Want to get involved in other ways?

We always need help with documentation, [translation](docs/TRANSLATING.md) and anything else. Feel free to look at the [Website repository](https://github.com/jamulussoftware/jamuluswebsite) or get involved in fixing issues from the [issue tracker](https://github.com/jamulussoftware/jamulus/issues).
