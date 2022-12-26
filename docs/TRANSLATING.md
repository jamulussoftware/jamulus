# Guide for Translators

This guide is written to provide instructions from scratch for contributing to the translation of the Jamulus application to other languages.

Jamulus is mostly developed on GitHub. However, translators are not required to use GitHub, as Jamulus is translated on [Hosted Weblate](https://hosted.weblate.org/projects/jamulus/).

You have two options to translate Jamulus:
1. Using [Weblate](https://hosted.weblate.org/projects/jamulus/jamulus-app/) (online translation, easier and more feature rich. However, as of 10/2022, it will show your e-mail address on GitHub.) (recommended)
2. Using Git directly (more advanced, but you can test your changes on your machine locally)

---

## Introduction (using Weblate)

Translators should (if possible) have a GitHub account to communicate, as described in the **1. Setting up** paragraph in the Git section below. This account should be used to [create an account on Hosted Weblate](https://hosted.weblate.org/accounts/register/). Ensure that the Jamulus main developers are aware of your intentions to translate to your chosen language beforehand (e.g. by [creating or commenting on a respective issue during the release process for your language on GitHub](https://github.com/jamulussoftware/jamulus/issues)). This ensures you will have proper access and that support is provided. Have a look at the [adding a new language (checklist)](#adding-a-new-language-checklist) for what to expect.

## Using Weblate for translation

After logging into Hosted Weblate, visit the [Jamulus project page on Hosted Weblate](https://hosted.weblate.org/projects/jamulus/). There are several components dedicated to the Jamulus project. If you want to translate e.g. the main Jamulus program or the Windows installer, click on "[Jamulus app](https://hosted.weblate.org/projects/jamulus/jamulus-app/)" or "[Windows Installer](https://hosted.weblate.org/projects/jamulus/windows-installer/)" and choose your language. After that, translate the strings displayed on the Website. Weblate will open or update a pull request on the Jamulus repo within the next few hours, where your translation will be reviewed.

More info is provided in the [Weblate documentation](https://docs.weblate.org/).

---

## Introduction (using Git)

For completeness, this document describes the use of both GitHub (using Git), and of _Qt Linguist_.

The translator must be able to carry out the following steps, each of which are explained below:

- Create their own linked copy ("repository" or "repo") of Jamulus in GitHub. This is called Forking.
- Copy ("clone") their own repository to their computer, using either:
  - Command line `git`, or
  - [GitHub Desktop](https://docs.github.com/en/desktop)
- Update their own local repo from the upstream main branch.
- Create a local branch to contain the update.
- Use the Qt Linguist tool to edit the appropriate translation (`.ts`) file.
- Commit the updated `.ts` file to the branch their own local git repo.
- Push the branch from their local repo to their own repo on GitHub.
- Open a pull request (PR) the developers can use to merge the updated file into the upstream repo.

This guide contains two main parts:

1. Instructions for getting set up, which only needs to be done once.

2. The workflow for updating and contributing translations as a part of the preparation for a release of Jamulus.

---

## 1. Setting up

### Visit github.com

First of all, visit the [GitHub website](https://github.com)

If you don't yet have a GitHub account, click on **Sign up** to go to the Create Your Account page. Enter:

- Your chosen username. This is a simple word containing letters and, optionally, numbers, and will identify you in the GitHub world. It is not an email address. This name is represented in the examples below as `yourusername`.
- Your email address. This will be used by GitHub to send you notifications by email, and to identify commits made by you.
- Your chosen password. Do not use the same password as for any other website.
- Solve the puzzle to prove you are human and click **Create account**.

If you do have a GitHub account, and are not yet logged in, click on **Sign in**, and enter your username and password, then **Sign in**.

Go to the [Jamulus repository](https://github.com/jamulussoftware/jamulus).

Create your own copy ("fork") of the Jamulus repository by clicking the **Fork** button at the top right of the page.

If your GitHub account is also part of an organisation, GitHub will ask you where to create the fork. Choose your personal GitHub account.

It will then display the message "Forking jamulussoftware/jamulus", and when finished will display the home page of your own Jamulus repo (**yourusername/jamulus**).

### Command line git tools

Linux and Mac machines come with command-line Git tools ready-installed or easily available.

- On Mac, the `git` command is available in `/usr/bin/git`, as part of the Xcode package, or can be installed separately (see below).
- On Linux, it may be necessary to install the `git` package using one of these commands:
  - On RedHat or CentOS, `yum install git` or `dnf install git`
  - On Debian or Ubuntu: `apt install git`
- On Windows, Git can be obtained from [Git for Windows](https://git-for-windows.github.io/)

More information about installing Git on various systems can be found [here](https://www.atlassian.com/git/tutorials/install-git)

Make a local copy of your Jamulus repo by using `git clone`:

- At the shell command-line, navigate to the directory that will be the parent of your `jamulus` development directory.
- Give one of the following commands:
  - For ssh access: `git clone git@github.com:yourusername/jamulus.git`
  - For https access: `git clone https://github.com/yourusername/jamulus.git`

This will create a `jamulus` directory. Change to that directory.

- Add the upstream repository: `git remote add upstream https://github.com/jamulussoftware/jamulus.git`
- Check the remotes using `git remote -v`. The output should look like this:
  ```
  origin  git@github.com:yourusername/jamulus.git (fetch)
  origin  git@github.com:yourusername/jamulus.git (push)
  upstream        https://github.com/jamulussoftware/jamulus.git (fetch)
  upstream        https://github.com/jamulussoftware/jamulus.git (push)
  ```
  or this:
  ```
  origin  https://github.com/yourusername/jamulus.git (fetch)
  origin  https://github.com/yourusername/jamulus.git (push)
  upstream        https://github.com/jamulussoftware/jamulus.git (fetch)
  upstream        https://github.com/jamulussoftware/jamulus.git (push)
  ```

### GitHub Desktop

[GitHub Desktop](https://docs.github.com/en/desktop) is available for macOS 10.10 or later, and Windows 7 64-bit or later. It is not available for 32-bit Windows.

To install GitHub Desktop, visit the [download page](https://desktop.github.com) and follow the link for the appropriate Operating System.

When downloading for Mac, the instructions suggest opening `GitHubDesktop.zip`, but in fact, what was downloaded was `GitHubDesktop.app`. This should just be moved from `Downloads` to `Applications`.

Run GitHub Desktop, and do the following steps:

- On the Welcome Screen, click on **Sign in to github.com**
- Sign in by following the instructions. These may vary depending on whether you have logged into GitHub via a web browser already. If necessary, click on **Authorize Desktop**.
- Confirm access by entering your GitHub password.
- for Mac, if the browser requests to open "GitHub Desktop.app", click **Allow**.
- In Configure Git, enter your name and email address. These will be used to identify commits you make to Git. Click **Continue**.
- Agree or decline to submit periodic usage stats, and click **Finish**.
- Either:
  - Select **Clone a Repository from the Internet** and enter `yourusername/jamulus`, or
  - Select your own fork of Jamulus from the list (`yourusername/jamulus`) and click the **Clone** button.
- Select the Local Path where the project should be stored, and click **Clone**.
- It will display the page **Cloning jamulus** with progress indication.
- On completion of cloning, it will ask "How are you planning to use this fork?". Answer "To contribute to the parent project".

### Qt Linguist

Qt Linguist is a part of the Qt development suite, and may either be installed via your Operating System's packaging manager,
or by downloading from the [Qt Open Source download page](https://www.qt.io/download-open-source). In the latter case you
will need to create an account on the Qt website.

Under Linux

- On RedHat or CentOS: `yum install qt5-linguist` or `dnf install qt5-linguist`
- On Debian or Ubuntu: `apt install qttools5-dev-tools`

Instructions for use are in the [Qt Linguist Manual](https://doc.qt.io/qt-5/qtlinguist-index.html)

---

## 2. Doing a translation

### Get the most up-to-date files and set up a new branch

#### With git command line tools

The first step is to get the local repo up to date with the upstream main:

```
cd projectdir/jamulus
git fetch upstream
```

This fetches information about the current state of the upstream repo. It is now necessary to apply any upstream changes to the local repo.
As the user will not be updating the `main` branch themselves, `git rebase` can be used to fast-forward to the current state:

```
git checkout main
git rebase upstream/main
git push
```

Finally, create a new branch for the changes that will be done (do not just do them on `main`).
The actual name of the branch is not critical, since the branch will be deleted after being merged,
but it's worth choosing a meaningful name:

```
git checkout -b translate-r3_7_0-german
```

(The branch name is chosen by the user; the above name translate-r3_7_0-german is an example for the German translation of V3.7.0)

The branch will be used later as the source of a pull request.

#### With GitHub Desktop

Select the current repository as `jamulus`, and the current branch as `main`.

Click on "Fetch origin" to get fully up to date with upstream.

Drop down the "Current Branch" menu and click on the **New Branch** button. Give the branch a meaningful name such as `translate-r3_7_0-german` and click **Create Branch**.

Do not click on **Publish branch** just yet.

### Work on the translation file

Open Qt Linguist, and navigate to the directory `src/translation` within your project directory.

In this directory are translation source files for each language, each with a `.ts` suffix. Don't worry about the `.qm` files, as they are compiled when building the release code.

Open the `.ts` file for the language being worked on.

Each context in the left-hand column represents a source file or graphical form containing translatable words and phrases ("strings"). The icon beside each shows a green tick if there is nothing further to be done,
a yellow tick if a translation needs attention, and a question mark if there are untranslated strings.

After clicking on a Context, the list of strings needing translation is shown, with a green tick beside the ones that have been done. The right-hand pane will show either the source code or the form,
so that the usage can be checked, in case this aids the translation.

Strings with a grey tick can be ignored. They are old strings that used to be used but are no longer. They are retained for reference in case they should be used again in the future.

Again, strings with a question mark require translation.

After some or all of the required strings have been translated, the file can be saved.

### Submit the updated translation as a pull request (PR)

Once all translations have been done, the branch containing the changes must be commited to your own repo, pushed to GitHub (`origin`) and then a pull request raised to the upstream repo.

#### With git command line tools

First, check the current branch with `git status`, to make sure it is correct, and then do a commit,
with a commit message describing the change, for example:

```
git commit -am 'Update German translations for v3.7.0'
```

Then do a `git push`.

It will probably tell you to set the upstream repository for tracking, and conveniently gives the command for copying and pasting:

```
git push --set-upstream origin translate-r3_7_0-german
```

Finally, go to the GitHub website where it will most likely offer a banner saying there is a recent commit and offering to open a pull request. Do so.

#### With GitHub Desktop

Select the current repository as `jamulus`, and the branch that was created above, such as `translate-r3_7_0-german`.

The changed file(s) should be listed in the left-hand column as `src/translation/translation_xx_YY.ts`. When the file is selected, the differences will be displayed in the main panel.

Add a simple commit message in the first box below the file list, (e.g. change "Update filename"
to something like "Update German translations for v3.7.0"), and add any extra description in the Description box (optional, probably not required).

Commit the changes to the local git repo by clicking on **Commit to <branch>**.

Click on **Publish branch** or **Push origin**. This will push the branch to your own repo on GitHub. (It will say **Publish branch** for a new branch, or **Push origin** if the branch has already been published).

There will now be a section offering **Create pull request**. Click on that to create the PR to the upstream repository.

#### Updating a PR/fixing conflicts

Other contributors may have edited your files after you began your work, which can cause conflicts. If GitHub or git tells you about conflicts, or if you are requested to [rebase](https://teamairship.com/update-outdated-branch-without-creating-merge-commit/) your work, do so as follows:

```shell
git remote add upstream git@github.com:jamulussoftware/jamulus ;# add the main repo as upstream remote if you didn’t already as mentioned above
git fetch upstream ; # Get the latest changes from the main upstream repo you added before (if needed)
git checkout <theBranchYourWorkIsOn> ;# switch to (= checkout) the branch you want to rebase
git rebase upstream/main ;# replay (=rebase) your changes onto the latest code (and fix conflicts if needed)
git rebase --continue # to continue the rebasing progress after having fixed the conflicts
git push --force # Push the changes to your repo. Be aware that this will overwrite your remote branch!
```

---

## Adding a new language (checklist)

The following section describes how to add a new language and what to do or wait for.
Please open an issue on GitHub and introduce yourself first, even if you intend to add the translation files yourself.

After having opened an issue on GitHub:
1. Fork the Jamulus repository as described above, then install Qt on your machine (See [COMPILING.md](../COMPILING.md)). (You will need `lrelease` and `lupdate` later on.)
2. Create a branch based on the `main` branch: Locally navigate to the folder you cloned the repo to (with `cd /path/to/jamulus/repo`) and type `git checkout -b "translate-add-xx_YY"` (where xx_YY is the language code of the new language).
3. Open Jamulus.pro in an editor such as `nano`: `nano Jamulus.pro` and add your language file (which will be created later) in alphabetical order to the `TRANSLATIONS` variable. The format should match the other present languages.
4. Also add `src/translation/translation_xx_YY.qm` to the DISTFILES variable similar to the other files
5. Save and close Jamulus.pro.
6. Add the `.qm` file in `src/resources.qrc` similar to other languages.
7. Now move back to the repository root and run `lupdate Jamulus.pro`. Afterwards, run `lrelease Jamulus.pro` which should create all of the necessary files.
8. Finally, add, commit and push the changes to your repository with the following commands: `git add Jamulus.pro src/translation/translation_xx_YY.* src/resources.qrc`, `git commit -m "Translation: Add empty xx_YY translation"`, `git push`
9. Open a pull request from your repo to the main repo with a brief description that you plan to translate Jamulus to your language
10. Once the pull request is merged, Weblate should pick up the new language.

### Adding a translation to the Windows installer

To add a new translation to the Installer (while undertaking the above steps with Git) [as described above](#adding-a-new-language-checklist):

1. Copy `src/translation/wininstaller/en_UK.nsi` to `src/translation/wininstaller/xx_YY.nsi`
2. Add your language with [the correct language code](https://nsis.sourceforge.io/Examples/Modern%20UI/MultiLanguage.nsi) to `src/translation/installerlng.nsi` by following the file format
3. Rename `${LANG_ENGLISH}` to the respective value you set in installerlng.nsi in xx_YY.nsi

### After a language is added

Start translating the new language using Hosted Weblate or Git (as described in the first section of this guide). If you plan to work on the translation for a longer period, please contact the administrators to be invited to the Jamulus organisation as well as translators channel on Discord or Matrix.

Since every contributor can and should be added to the in-app contributor list, please remind us to add you to the `src/utils.h` file as translator or open a pull request yourself.

---

## That's all

Thank you for contributing!
