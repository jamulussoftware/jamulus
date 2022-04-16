#!/bin/bash
set -eu

YEAR=$(date +%Y)
echo "Updating global copyright strings..."
sed -re 's/(Copyright.*2[0-9]{3}-)[0-9]{4}/\1'"${YEAR}"'/g' -i src/translation/*.ts src/util.cpp src/aboutdlgbase.ui

echo "Updating copyright comment headers..."
find android ios linux mac src windows -regex '.*\.\(cpp\|h\|mm\)' -not -regex '\./\(\.git\|libs/\|moc_\|ui_\).*' | while read -r file; do
    sed -re 's/(\*.*Copyright.*[^-][0-9]{4})(\s*-\s*\b[0-9]{4})?\s*$/\1-'"${YEAR}"'/' -i "${file}"
done


sed -re 's/^( [0-9]{4}-)[0-9]{4}( The Jamulus)/\1'"${YEAR}"'\2/' -i distributions/debian/copyright
