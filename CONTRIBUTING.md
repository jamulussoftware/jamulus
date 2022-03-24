# Want to contribute some code?

## We’d really appreciate your support!

- If the [Github issue](https://github.com/jamulussoftware/jamulus/issues) for your feature/bug fix already exists, write a message in that issue saying you want to work on it.
- Otherwise, please [post on the GitHub Discussions](https://github.com/jamulussoftware/jamulus/discussions) and say that you are planning to do some coding and explain why. Then we can discuss the specification.
- Please only start coding after we have agreed to a specification to avoid putting lots of work into something which may not get accepted later.

## Jamulus project/source code general principles

### Stability

This has been, and must continue to be, the most important requirement. It is particularly important for use cases such as the Worldjam where instability during a live performance would be unacceptable. The following two principles are designed to support this principle.

### [Keep it Simple and Stupid](https://en.wikipedia.org/wiki/KISS_principle) and [Do One Thing and Do It Well](https://en.wikipedia.org/wiki/Unix_philosophy#Do_One_Thing_and_Do_It_Well)

If a feature or function can be achieved in another way by another system or method, then that is preferable to building that feature into Jamulus.

### Source code consistency

- Respect the existing code style: Tab size=4, insert spaces.
- Space before and after `(` and `)`, except no space between `)` and `;`, and no space before an empty `()`.
- All bodies of `if`, `else`, `while`, `for`, etc., to be enclosed in braces `{` and `}`, on separate lines.

You can use your editor's or IDE's clang-format support to accomplish that.
On the command line, you can run `make clang_format` to do the same before committing.

Do not use diff/patch to send your code changes but create a Github fork of the Jamulus code and create a Pull Request when you are done.

Please run a local build test. Make sure there are no errors. After opening a pull request, keep an eye on the CI checks for quality or compile issues and fix them as required.

If there are conflicts with jamulussoftware/jamulus showing when you go to raise the pull request, resolve those locally:

```shell
git checkout master ;# checkout your local master
git pull upstream master --tags ;# refresh from jamulussoftware/jamulus (i.e. use your upstream remote name)
git push --tags --force ;# update your fork's master
git checkout - ;# back to your working branch
git rebase master ;# replay your changes onto the latest code
git push --force ;# update your working branch on your fork
```

### Supported platforms

We support the following platforms and versions:

- Windows 7 or higher
- macOS <!-- Which versions? -->
- Ubuntu Linux 18.04 or later, Debian 10 or later, most Linux flavors with recent enough Qt versions
  <!-- Do we support BSD? I think a recent discussion was about FreeBSD? -->
  <!-- Should we already list Android? If so, what platforms/versions? -->

Please try to avoid breaking any of them by introducing platform-specific code. Check to see if any newly introduced Qt calls are available in the minimum supported Qt version, which is currently 5.9. Note that code _style_ in a file may be Qt 4.x, and while you should normally stick to existing style if making large-scale changes, then updating to Qt 5.9 style is encouraged.
Keep all code compatible to C++11.

### Dependencies

If your code requires new dependencies, be sure that those are available on all supported platforms and that the introduction of those dependencies has been discussed and accepted.

### User experience

Jamulus is used by people all over the world with different backgrounds and knowledge. Features and language should be user focused.
The Jamulus project has a [style and tone guide](https://jamulus.io/contribute/Style-and-Tone) which you should follow to remain consistent.
Features should be usable in the sense that they act as expected to somebody who does not have a technical background.

### Ownership

The submitter of an Issue or a PR is responsible for its care and feeding, answering all questions directed at them, and making agreed amends if needed.

So as to reduce effort for others in understanding a PR or an Issue, authors are strongly encouraged to update their initial posts/PR descriptions or title to reflect the current state of play, amends, enhancements, outstanding issues etc. Admins reserve the right to do this as they see fit.

### Documentation/Acknowledgements

Each new feature or bug fix must be documented in the ChangeLog. Please provide a single-sentence suggestion for that as part of your pull request description. Do not modify the ChangeLog file as part of your PR as it will lead to conflicts.

If you are a first-time contributor/translator, please add yourself to the contributor/translator list in the About dialog of the Jamulus software (see in `src/util.cpp` in the constructor function `CAboutDlg::CAboutDlg()`).

### Merging Pull Requests

The git master branch is protected to require at least two reviews by the main developers before the pull request can be merged. If a pull request receives at least two positive reviews, any of the main developers can initiate the merge.

---

## Want to get involved in other ways?

We always need help with documentation, translation and anything else. Have a look at our [overview for contributors](https://jamulus.io/wiki/Contribution).
