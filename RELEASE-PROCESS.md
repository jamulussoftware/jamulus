# Release Process for Jamulus

This document lists the exact steps required for a developer with write access to generate either a pre-release (beta, rc) or full release of Jamulus.

## Once to set up

A direct clone of the Jamulus repo is required, not a fork. This should be set up in a separate directory just for the release process:

```
$ git clone git@github.com:jamulussoftware/jamulus.git jamulus-upstream
$ cd jamulus-upstream
$ git remote -v
origin  git@github.com:jamulussoftware/jamulus.git (fetch)
origin  git@github.com:jamulussoftware/jamulus.git (push)
$ git status
On branch master
Your branch is up to date with 'origin/master'.

nothing to commit, working tree clean
```

## Steps for a specific release

First make sure all Pull Requests that should be included in the release have been merged in Github.

Next, change to the above directory `jamulus-upstream`, checkout `master` and ensure it is up to date:

```
$ cd jamulus-upstream
$ git checkout master
$ git pull
$ git status
```

Make sure there are no pending changes shown by `git status`.

Now ensure the compiled translation files are up to date:

```
$ lrelease Jamulus.pro
$ git status
```

If any of the `.qm` files have been updated by `lrelease`, they will be shown as changed files. If there are any, they should be committed and pushed:

```
$ git commit -am'Update compiled translations'
$ git push
```

Next edit both `Jamulus.pro` and `ChangeLog` with the new version number, such as **3.7.0rc2** or **3.7.0**:

```
$ vim -o Jamulus.pro ChangeLog
```

In `Jamulus.pro`, edit the `VERSION` line to give the release, e.g.

```
VERSION = 3.7.0rc2
```

or

```
VERSION = 3.7.0
```

In `ChangeLog`, for a pre-release, change the first version heading as follows:

```
### 3.7.0rc2 <- NOTE: the release version number will be 3.7.0 ###
```

for a proper release, change it as follows, removing the NOTE and including the release date:

```
### 3.7.0 (2021-03-17) ###
```

Check only those two files have changed, then commit and push the changes:

```
$ git status
$ git commit -am'Update version to 3.7.0 for release'
$ git push
```

Now add the required tag locally and then push the tag. This will start the automated release process on Github:

```
$ git tag r3_7_0
$ git push origin tag r3_7_0
```

## If this is a proper release, move the `latest` tag

This needs the `--force` option to overwrite the existing `latest` tag and move it to the current commit:

```
$ git tag --force latest
$ git push --force origin tag latest
```

For a pre-release, the `latest` tag should not be updated, but continue to point to the last proper release.

## Make the master branch ready for post-release development

This can be done immediately after pushing the above tag, there is no need to wait.

Edit both `Jamulus.pro` and `ChangeLog` to add `dev` to the version number and create a new section in the change log:

```
$ vim -o Jamulus.pro ChangeLog
```

In `Jamulus.pro`, edit the `VERSION` line to add `dev` to the current version number, e.g.

```
VERSION = 3.7.0dev
```

In `ChangeLog`, add a new section with `dev` and restore the NOTE indicating the next version number:

```
### 3.7.0dev <- NOTE: the release version number will be 3.7.1 ###


### 3.7.0 (2021-03-17) ###
```

Check only those two files have changed, then commit and push the changes:

```
$ git status
$ git commit -am'Set version to 3.7.0dev'
$ git push
```

Close the shell or change out of the upstream directory to one's usual working directory, e.g.:

```
$ cd ../jamulus
```

