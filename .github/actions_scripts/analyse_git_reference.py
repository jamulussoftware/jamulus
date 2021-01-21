#!/usr/bin/python3

import sys

print('Number of arguments:', len(sys.argv), 'arguments.')
print('Argument List:', str(sys.argv))

fullref=sys.argv[1]
pushed_name=sys.argv[2]
jamulus_version=sys.argv[3]
release_version_name = jamulus_version

if fullref.startswith("refs/tags/"):
    if "beta" in pushed_name:
        print('this is a Beta-Release-Tag')
        is_prerelease = True
    else:
        print('this is a Release-Tag')
        is_prerelease = True
    if pushed_name == "latest":
        print('this is a Latest-Tag')
        release_version_name = "latest"
        release_title='Release "latest"'
    else:
        release_version_name = jamulus_version
        release_title="Release {}  ({})".format(release_version_name, pushed_name)
elif fullref.startswith("refs/heads/"):
    print('python is Head')
    is_prerelease=True
    release_title='Pre-Release of "{}"'.format(pushed_name)
else:
    print('unknown git-reference type')
    release_title='Pre-Release of "{}"'.format(pushed_name)
    is_prerelease=True

print("IS_PRERELEASE::{}".format(is_prerelease)) #debug output
print("RELEASE_TITLE::{}".format(release_title)) #debug output
print("::set-output name=IS_PRERELEASE::{}".format(str(is_prerelease).lower()))
print("::set-output name=RELEASE_TITLE::{}".format(release_title))
print("::set-output name=RELEASE_TAG::{}".format(pushed_name))
print("::set-output name=PUSHED_NAME::{}".format(pushed_name))
print("::set-output name=JAMULUS_VERSION::{}".format(jamulus_version))
print("::set-output name=RELEASE_VERSION_NAME::{}".format(release_version_name))
