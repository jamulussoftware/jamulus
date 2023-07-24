#!/bin/bash
##############################################################################
# Copyright (c) 2022-2023
#
# Author(s):
#  Christian Hoffmann
#  The Jamulus Development Team
#
##############################################################################
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
##############################################################################

set -eu -o pipefail

YEAR=$(date +%Y)
echo "Updating global copyright strings..."
sed -re 's/(Copyright.*2[0-9]{3}-)[0-9]{4}/\1'"${YEAR}"'/g' -i src/translation/*.ts src/util.cpp src/aboutdlgbase.ui

echo "Updating copyright comment headers..."
find android ios linux mac src windows tools .github -regex '.*\.\(cpp\|h\|mm\|sh\|py\|pl\)' -not -regex '\./\(\.git\|libs/\|moc_\|ui_\).*' | while read -r file; do
    sed -re 's/((\*|#).*Copyright.*[^-][0-9]{4})(\s*-\s*\b[0-9]{4})?\s*$/\1-'"${YEAR}"'/' -i "${file}"
done

sed -re 's/^( [0-9]{4}-)[0-9]{4}( The Jamulus)/\1'"${YEAR}"'\2/' -i linux/debian/copyright
