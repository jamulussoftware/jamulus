#!/usr/bin/python3

#
# on a triggered github workflow, this file does the decisions and variagles like
#   - shall the build be released (otherwise just run builds to check if there are errors)
#   - is it a prerelease
#   - title, tag etc of release_tag
#
# see the last lines of the file to see what variables are set
#


import sys
import os
import subprocess

# get the jamulus version from the .pro file
def get_jamulus_version(repo_path_on_disk):
    jamulus_version = ""
    with open (repo_path_on_disk + '/Jamulus.pro','r') as f:
        pro_content = f.read()
    pro_content = pro_content.replace('\r','')
    pro_lines = pro_content.split('\n')
    for line in pro_lines:
        line = line.strip()
        VERSION_LINE_STARTSWITH = 'VERSION = '
        if line.startswith(VERSION_LINE_STARTSWITH):
            jamulus_version = line[len(VERSION_LINE_STARTSWITH):]
            return jamulus_version
    return "UNKNOWN_VERSION"

def get_git_hash():
    return subprocess.check_output(['git', 'describe', '--match=xxxxxxxxxxxxxxxxxxxx', '--always', '--abbrev', '--dirty']).decode('ascii').strip()
    #return subprocess.check_output(['git', 'rev-parse', 'HEAD'])
    #return subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'])

if len(sys.argv) == 1:
   pass
else:
    print('wrong number of arguments')
    print('Number of arguments:', len(sys.argv), 'arguments.')
    print('Argument List:', str(sys.argv))
    raise Exception("wrong agruments")
    
# derive workspace-path
repo_path_on_disk = os.environ['GITHUB_WORKSPACE'] 

# derive git related variables
version_from_changelog = get_jamulus_version(repo_path_on_disk)
if "dev" in version_from_changelog:
    release_version_name = "{}-{}".format(version_from_changelog, get_git_hash())
    print("building an intermediate version: ", release_version_name)
else:
    release_version_name = version_from_changelog
    print("building a release version: ", release_version_name)


fullref=os.environ['GITHUB_REF']
reflist = fullref.split("/", 2)
pushed_name = reflist[2]


# run Changelog-script
os.system('perl "{}"/.github/actions_scripts/getChangelog.pl "{}"/ChangeLog "{}" > "{}"/autoLatestChangelog.md'.format(
    os.environ['GITHUB_WORKSPACE'],
    os.environ['GITHUB_WORKSPACE'],
    version_from_changelog,
    os.environ['GITHUB_WORKSPACE']
))

# decisions about release, prerelease, title and tag
if fullref.startswith("refs/tags/"):
    print('this reference is a Tag')
    release_tag = pushed_name # tag already exists
    release_title="Release {}  ({})".format(release_version_name, pushed_name)
    
    if pushed_name.startswith("r"):
        if "beta" in pushed_name:
            print('this reference is a Beta-Release-Tag')
            publish_to_release = True
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
        is_prerelease = False
        release_version_name = "latest"
        release_title='Release "latest"'
elif fullref.startswith("refs/heads/"):
    print('this reference is a Head/Branch')
    publish_to_release = False
    is_prerelease = True
    release_title='Pre-Release of "{}"'.format(pushed_name)
    release_tag = "releasetag/"+pushed_name #better not use pure pushed name, creates a tag with the name of the branch, leads to ambiguous references => can not push to this branch easily
else:
    print('unknown git-reference type: ' + fullref)
    publish_to_release = False
    is_prerelease = True
    release_title='Pre-Release of "{}"'.format(pushed_name)
    release_tag = "releasetag/"+pushed_name #avoid ambiguity in references in all cases

#helper function: set github variable and print it to console
def set_github_variable(varname, varval):
    print("{}='{}'".format(varname, varval)) #console output
    print("::set-output name={}::{}".format(varname, varval))

#set github-available variables
set_github_variable("PUBLISH_TO_RELEASE", str(publish_to_release).lower())
set_github_variable("IS_PRERELEASE", str(is_prerelease).lower())
set_github_variable("RELEASE_TITLE", release_title)
set_github_variable("RELEASE_TAG", release_tag) 
set_github_variable("PUSHED_NAME", pushed_name)
set_github_variable("JAMULUS_VERSION", release_version_name)
set_github_variable("RELEASE_VERSION_NAME", release_version_name)
set_github_variable("X_GITHUB_WORKSPACE", os.environ['GITHUB_WORKSPACE'])
set_github_variable("BUILD_FLATPACK", str(True).lower())
