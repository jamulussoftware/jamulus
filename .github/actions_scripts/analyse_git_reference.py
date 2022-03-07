#!/usr/bin/python3
# This script is trigged from the Github Autobuild workflow.
# It analyzes Jamulus.pro and git push details (tag vs. branch, etc.) to decide
#   - whether a release should be created,
#   - whether it is a pre-release, and
#   - what its title should be.

import os
import re
import subprocess

REPO_PATH = os.path.join(os.path.dirname(__file__), '..', '..')


def get_version_from_jamulus_pro():
    with open(REPO_PATH + '/Jamulus.pro', 'r') as f:
        pro_content = f.read()
    m = re.search(r'^VERSION\s*=\s*(\S+)$', pro_content, re.MULTILINE)
    if not m:
        raise Exception("Unable to determine Jamulus.pro VERSION")
    return m.group(1)


def get_git_hash():
    return subprocess.check_output([
        'git',
        'describe',
        '--match=xxxxxxxxxxxxxxxxxxxx',
        '--always',
        '--abbrev',
        '--dirty'
    ]).decode('ascii').strip()


def write_changelog(version):
    changelog = subprocess.check_output([
        'perl',
        f'{REPO_PATH}/.github/actions_scripts/getChangelog.pl',
        f'{REPO_PATH}/ChangeLog',
        version,
    ])
    with open(f'{REPO_PATH}/autoLatestChangelog.md', 'wb') as f:
        f.write(changelog)


def get_release_version_name(jamulus_pro_version):
    if "dev" in jamulus_pro_version:
        name = "{}-{}".format(jamulus_pro_version, get_git_hash())
        print("building an intermediate version: ", name)
        return name

    name = jamulus_pro_version
    print("building a release version: ", name)
    return name


def set_github_variable(varname, varval):
    print("{}='{}'".format(varname, varval))  # console output
    print("::set-output name={}::{}".format(varname, varval))


jamulus_pro_version = get_version_from_jamulus_pro()
write_changelog(jamulus_pro_version)
release_version_name = get_release_version_name(jamulus_pro_version)

fullref = os.environ['GITHUB_REF']
reflist = fullref.split("/", 2)
pushed_name = reflist[2]

# decisions about release, prerelease, title and tag:
publish_to_release = False
is_prerelease = True
if fullref.startswith("refs/tags/"):
    print('this reference is a Tag')
    release_tag = pushed_name  # tag already exists
    release_title = "Release {}  ({})".format(release_version_name, pushed_name)

    if pushed_name.startswith("r"):
        publish_to_release = True
        if re.match(r'^r\d+_\d+_\d+$', pushed_name):
            print('this reference is a Release-Tag')
            is_prerelease = False
        else:
            print('this reference is a Non-Release-Tag')
    else:
        print('this reference is a Non-Release-Tag')

elif fullref.startswith("refs/heads/"):
    print('this reference is a Head/Branch')
    release_title = 'Pre-Release of "{}"'.format(pushed_name)
    release_tag = "releasetag/" + pushed_name  # better not use pure pushed name, creates a tag with the name of the branch, leads to ambiguous references => can not push to this branch easily

else:
    print('unknown git-reference type: ' + fullref)
    release_title = 'Pre-Release of "{}"'.format(pushed_name)
    release_tag = "releasetag/" + pushed_name  # avoid ambiguity in references in all cases

# set github-available variables
set_github_variable("PUBLISH_TO_RELEASE", str(publish_to_release).lower())
set_github_variable("IS_PRERELEASE", str(is_prerelease).lower())
set_github_variable("RELEASE_TITLE", release_title)
set_github_variable("RELEASE_TAG", release_tag)
set_github_variable("PUSHED_NAME", pushed_name)
set_github_variable("JAMULUS_VERSION", release_version_name)
set_github_variable("RELEASE_VERSION_NAME", release_version_name)
set_github_variable("X_GITHUB_WORKSPACE", os.environ['GITHUB_WORKSPACE'])
