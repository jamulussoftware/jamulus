#!/bin/bash

# Please run this script with the first parameter being the root of the repo
if [ -z "${1}" ]; then
    echo "Please give the path to the repository root as second parameter to this script!"
    exit 1
fi

cd ${1}
		
		
echo "Building... qmake"
if [-x /Users/runner/work/jamulus/jamulus/Qt/5.15.2/clang_64/bin/qmake]
then
	/Users/runner/work/jamulus/jamulus/Qt/5.15.2/clang_64/bin/qmake
else
	qmake
fi

echo "Building... make"
make


echo "Done"