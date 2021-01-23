#!/usr/bin/python3

import sys
import os
if len(sys.argv) != 4:
    print('Expecing 4 arguments, 1 script path and 3 parameters')
    print('Number of arguments:', len(sys.argv), 'arguments.')
    print('Argument List:', str(sys.argv))
    raise Exception("wrong agruments")
    
print('GITHUB_REF', os.environ['GITHUB_REF'])
fullref=os.environ['GITHUB_REF']
reflist = fullref.split("/", 2)
pushed_name = reflist[2]
print('fullref', fullref)
print('pushed_name', pushed_name)
print('reflist', reflist)


fullref=sys.argv[1]
pushed_name=sys.argv[2]
jamulus_version=sys.argv[3]
release_version_name = jamulus_version

if fullref.startswith("refs/tags/"):
    print('this reference is a Tag')
    publish_to_release = True
    release_tag = pushed_name # tag already exists
    release_title="Release {}  ({})".format(release_version_name, pushed_name)
    if pushed_name.startswith("r"):
        if "beta" in pushed_name:
            print('this reference is a Beta-Release-Tag')
            is_prerelease = True
        else:
            print('this reference is a Release-Tag')
            publish_to_release = True
            is_prerelease = False
    else:
        print('this reference is a Non-Release-Tag')
        publish_to_release = False
        is_prerelease = True # just in case
    
        
    if pushed_name == "latest":
        print('this reference is a Latest-Tag')
        publish_to_release = True
        release_version_name = "latest"
        release_title='Release "latest"'
elif fullref.startswith("refs/heads/"):
    print('this reference is a Head/Branch')
    publish_to_release = False
    is_prerelease = True
    release_title='Pre-Release of "{}"'.format(pushed_name)
    release_tag = "releasetag/"+pushed_name #better not use pure pushed name, creates a tag with the name of the branch, leads to ambiguous references => can not push to this branch easily
else:
    print('unknown git-reference type')
    publish_to_release = False
    is_prerelease = True
    release_title='Pre-Release of "{}"'.format(pushed_name)
    release_tag = "releasetag/"+pushed_name #avoid ambiguity in references in all cases

def set_github_variable(varname, varval):
    print("{}='{}'".format(varname, varval)) #console output
    print("::set-output name={}::{}".format(varname, varval))

set_github_variable("PUBLISH_TO_RELEASE", str(publish_to_release).lower())
set_github_variable("IS_PRERELEASE", str(is_prerelease).lower())
set_github_variable("RELEASE_TITLE", release_title)
set_github_variable("RELEASE_TAG", release_tag) 
set_github_variable("PUSHED_NAME", pushed_name)
set_github_variable("JAMULUS_VERSION", jamulus_version)
set_github_variable("RELEASE_VERSION_NAME", release_version_name)
