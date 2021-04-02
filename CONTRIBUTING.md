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

### Focus on and improve the strengths of Jamulus
There are other solutions for real time online jamming available. But Jamulus stands alone with its client/server system, minimalistic GUI and simple functionality. 

### Source code consistency
- Respect the existing code style: Tab size=4, insert spaces.
- Space before and after `(` and `)`, except no space between `)` and `;`, and no space before an empty `()`.
- All bodies of `if`, `else`, `while`, `for`, etc., to be enclosed in braces `{` and `}`, usually on separate lines.

Make sure (if possible) that your code compiles on Windows/Mac/Linux.
Do not use diff/patch to send your code changes but create a Github fork of the Jamulus code and create a Pull Request when you are done.

### Documentation/Acknowledgements
Each new feature or bug fix must be documented in the ChangeLog. If a new contributor/translator adds code to the Jamulus project, he/she should be added to the contributor/translator list in the About dialog of the Jamulus software (see in `src/util.cpp` in the constructor function `CAboutDlg::CAboutDlg()`).

### Merging Pull Requests
The git master branch is protected to require at least two reviews by the main developers before the pull request can be merged. If a pull request receives at least two positive reviews, any of the main developers can initiate the merge.

---

## Want to get involved in other ways? 

We always need help with documentation, translation and anything else. Have a look at our [overview for contributors](https://jamulus.io/wiki/Contribution).

