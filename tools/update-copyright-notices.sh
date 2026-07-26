#!/bin/bash
##############################################################################
# Copyright (c) 2022-2026
#
# Author(s):
#  Christian Hoffmann
#  The Jamulus Development Team
#
# As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
# under AGPL 3.0 or any later version.
#
# Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
# This code will be licensed under GPL 3.0 (or any later version) from
# 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
# the combined work, including network use provisions.
#
##############################################################################
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# ---------------------------------------------------------------------------
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
