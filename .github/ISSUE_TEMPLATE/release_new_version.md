---
name: 'For maintainers: Release a new version'
about: This is the Developer-only template which we use for tracking the process of future releases.
title: 'Prepare Release 3.y.z'
labels: release
assignees: ''
---

**Target timeline**
Scheduled feature freeze / Start of translation process: <!-- Add a date, usually about 6-8 weeks from the last release, e.g. 2021-04-20 -->
Targeted translation completion date: <!-- E.g. two weeks after feature freeze, e.g. 2021-05-04 -->
Approximate release date: <!-- E.g. one week after targeted translation completion date to have time for late translations, reviews, RC, e.g. 2021-05-11 -->
Current state: <!-- Planning|Translations (beta)|Code freeze (rc)|Released -->

**Checklist**
- [ ] Assign this issue to the release shepherd who is in charge of managing this checklist
- [ ] Pin this issue
- [ ] Create a milestone and add all relevant issues
- [ ] Ensure that all issues/PR targeted for this release are done by checking the Project board with an appropriate filter (e.g. [`milestone:"Release 3.8.0"`](https://github.com/orgs/jamulussoftware/projects/2?card_filter_query=milestone%3A"release+3.8.0")). Reminder main developers to review entries in *Waiting on team* state. De-tag unfinished Issues/PRs.
- [ ] Declare a freeze for code website by updating this Issue and adding a comment. PRs can still be worked on and may get reviewed, but must not be merged unless agreed explicitly.
- [ ] Check ./Jamulus -h output against the en-Command-Line-Options.md wiki page and update if necessary.
- [ ] Write the Changelog based on the list of merged PRs ([example for 3.8.0](https://github.com/jamulussoftware/jamulus/pulls?q=is%3Amerged+milestone%3A"Release+3.8.0"))
- [ ] Start Website translations
  - [ ] Check for broken links with a link checker locally
  - [ ] Create new branch `translate3_y_z` based on branch `release`
  - [ ] Squash-merge `changes` into `translate3_y_z`
  - [ ] Check if the list of translators in `tools/create-translation-issues.sh` is up-to-date
  - [ ] Create a translation Issue for each language, e.g. `../jamulus/tools/create-translation-issues.sh 3.8.0 2021-05-15 web 'Note: The term "Central server" has been replaced with "Directory server"'` (Last argument is optional and should be skipped if there is nothing to add)
- [ ] Start App translations
  - [ ] Generate `.ts` files in master via `lupdate`
  - [ ] Check if the list of translators in `tools/create-translation-issues.sh` is up-to-date
  - [ ] Create a translation Issue for each language, e.g. `./tools/create-translation-issues.sh 3.8.0 2021-05-15 'Note: The term "Central server" has been replaced with "Directory server"'` (Last argument is optional and should be skipped if there is nothing to add)
- [ ] [Tag a beta release](https://github.com/jamulussoftware/jamulus/blob/master/RELEASE-PROCESS.md#steps-for-a-specific-release)
- [ ] Announce the beta release on Github Discussions. Pin the thread.
- [ ] Get feedback on the stability and resource usage (memleaks?) of the beta release
- [ ] Finish Website translations
  - [ ] Wait for all PRs to be merged
  - [ ] Check for broken links with a link checker locally
  - [ ] Decide what to do about missing translations. Depending on the missing changes, consider keeping the old state (if it is still valid) or temporary backing out the relevant file(s) if they would become wrong.
- [ ] Finish App translations
  - [ ] Review PRs <details><summary>Checklist for the PRs</summary> <!-- FIXME: This should be moved to the admin wiki and linked here -->
     ```
     - [ ] Translator listed in the `src/util.cpp` **optionally add link to PR or code**
     - [ ] Punctuation and spacing consistent
     - [ ] Signal words consistent ("ASIO", "Buffer")
     - [ ] App translations: No untranslated strings (`grep unfinished -5 src/res/translation/translation_$TRANSLATION*.ts`)
     - [ ] App translations: Only a single `.ts` file checked in (`.qm` in addition is also OK)
     - [ ] Installer translations: Passes `tools/check-wininstaller-translations.sh`
     ```
     </details>
  - [ ] Wait for all PRs to be merged. If translations are missing after the targeted translation completion date, this is ok. They can be updated with the next release again.
  - [ ] Check for conflicting accelerator keys (see `tools/checkkeys.pl`)
  - [ ] Generate `.qm` files via `lrelease Jamulus.pro`
- [ ] [Tag a release candidate](https://github.com/jamulussoftware/jamulus/blob/master/RELEASE-PROCESS.md#steps-for-a-specific-release)
- [ ] Announce the release candidate on Github Discussions. Pin the thread. Unpin and lock the beta thread.
- [ ] Draft an announcement, include all contributors via `tools/get_release_contributors.py`
- [ ] [Update the version number in `Jamulus.pro` and add the release date to the Changelog header and commit](https://github.com/jamulussoftware/jamulus/blob/master/RELEASE-PROCESS.md#steps-for-a-specific-release)
- [ ] Tag this commit as `r3_y_z`
- [ ] Wait for the build to complete
- [ ] Do a smoke test for Windows/Mac/Linux -- Do the binaries start/connect properly? Can earlier Jamulus versions properly connect to a server based on the new release?
- [ ] [Force tag that tag as `latest` and push.](https://github.com/jamulussoftware/jamulus/blob/master/RELEASE-PROCESS.md#if-this-is-a-proper-release-move-the-latest-tag)
- [ ] Upload the artifacts to SourceForge and link latest (see #1814 for docs)
- [ ] [Prepare `Jamulus.pro` (`dev` suffix) and ChangeLog (add a header) for the next release](https://github.com/jamulussoftware/jamulus/blob/master/RELEASE-PROCESS.md#make-the-master-branch-ready-for-post-release-development)
- [ ] Trigger the update notification by updating both Update Check Servers with the new version (@pljones for update02, email corrados for update01)
- [ ] Announce the new release with a summary of changes (+ link to the changelog for details) and a link to the download page
  - [ ] On Github Discussions in the Announcements section. Lock the announcement thread. Pin the thread. Unpin and lock release candidate thread.
  - [ ] On Facebook in the group "Jamulus (official group)". Turn off replies.
- [ ] Update download links on the website by editing `config.yml`
- [ ] Publish Website release by merging `translate3_y_z` into `release`
- [ ] Delete the `translate3_y_z` branch from the Website repo
- [ ] Check that all Issues and PRs tagged for this release are in Done/Closed state.
- [ ] Close the release milestone in both jamulus and jamuluswebsite repos
- [ ] Create a milestone for the next minor release in jamulus and jamuluswebsite repos
- [ ] Update [this checklist](https://github.com/jamulussoftware/jamulus/blob/master/.github/ISSUE_TEMPLATE/release_new_version.md) with any improvements which were found
- [ ] Create a release retrospective Discussion
- [ ] Unpin and close this issue
