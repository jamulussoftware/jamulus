#!/usr/bin/env python3
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


def get_build_version(jamulus_pro_version):
    if "dev" in jamulus_pro_version:
        version = "{}-{}".format(jamulus_pro_version, get_git_hash())
        return 'intermediate', version

    version = jamulus_pro_version
    return 'release', version


def set_github_variable(varname, varval):
    print("{}='{}'".format(varname, varval))  # console output
    print("::set-output name={}::{}".format(varname, varval))


jamulus_pro_version = get_version_from_jamulus_pro()
set_github_variable("JAMULUS_PRO_VERSION", jamulus_pro_version)
build_type, build_version = get_build_version(jamulus_pro_version)
print(f'building a version of type "{build_type}": {build_version}')

fullref = os.environ['GITHUB_REF']
publish_to_release = bool(re.match(r'^refs/tags/r\d+_\d+_\d+\S*$', fullref))

# BUILD_VERSION is required for all builds including branch pushes
# and PRs:
set_github_variable("BUILD_VERSION", build_version)

# PUBLISH_TO_RELEASE is always required as the workflow decides about further
# steps based on this. It will only be true for tag pushes with a tag
# starting with "r".
set_github_variable("PUBLISH_TO_RELEASE", str(publish_to_release).lower())

if publish_to_release:
    reflist = fullref.split("/", 2)
    release_tag = reflist[2]
    release_title = f"Release {build_version}  ({release_tag})"
    is_prerelease = not re.match(r'^r\d+_\d+_\d+$', release_tag)
    if not is_prerelease and build_version != release_tag[1:].replace('_', '.'):
        raise Exception(f"non-pre-release tag {release_tag} doesn't match Jamulus.pro VERSION = {build_version}")

    # Those variables are only used when a release is created at all:
    set_github_variable("IS_PRERELEASE", str(is_prerelease).lower())
    set_github_variable("RELEASE_TITLE", release_title)
    set_github_variable("RELEASE_TAG", release_tag)
