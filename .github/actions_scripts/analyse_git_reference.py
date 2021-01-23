#!/usr/bin/python3

import sys

if len(sys.argv) != 4:
    print('Expecing 4 arguments, 1 script path and 3 parameters')
    print('Number of arguments:', len(sys.argv), 'arguments.')
    print('Argument List:', str(sys.argv))
    raise Exception("wrong agruments")

fullref=sys.argv[1]
pushed_name=sys.argv[2]
jamulus_version=sys.argv[3]
release_version_name = jamulus_version

if fullref.startswith("refs/tags/"):
    release_tag = pushed_name # tag already exists
    if "beta" in pushed_name:
        print('this reference is a Beta-Release-Tag')
        is_prerelease = True
    else:
        print('this reference is a Release-Tag')
        is_prerelease = True
    if pushed_name == "latest":
        print('this reference is a Latest-Tag')
        release_version_name = "latest"
        release_title='Release "latest"'
    else:
        print('this reference is a Unknown-Type Tag')
        release_version_name = jamulus_version
        release_title="Release {}  ({})".format(release_version_name, pushed_name)
elif fullref.startswith("refs/heads/"):
    print('this reference is a Head/Branch')
    is_prerelease=True
    release_title='Pre-Release of "{}"'.format(pushed_name)
    release_tag = "releasetag/"+pushed_name #better not use pure pushed name, creates a tag with the name of the branch, leads to ambiguous references => can not push to this branch easily
else:
    print('unknown git-reference type')
    is_prerelease=True
    release_title='Pre-Release of "{}"'.format(pushed_name)
    release_tag = "releasetag/"+pushed_name #avoid ambiguity in references in all cases

def set_github_variable(varname, varval):
    print("{}='{}'".format(varname, varval)) #console output
    print("::set-output name={}::{}".format(varname, varval))

set_github_variable("IS_PRERELEASE", str(is_prerelease).lower())
set_github_variable("RELEASE_TITLE", release_title)
set_github_variable("RELEASE_TAG", "releasetag/"+pushed_name) 
set_github_variable("PUSHED_NAME", pushed_name)
set_github_variable("JAMULUS_VERSION", jamulus_version)
set_github_variable("RELEASE_VERSION_NAME", release_version_name)
