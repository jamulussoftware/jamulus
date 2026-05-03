#!/bin/bash
##############################################################################
# Copyright (c) 2026
#
# Author(s):
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

for required_path in ".git" "tools/release_announcement/prompts" "tools/release_announcement"; do
	if [[ ! -e "$required_path" ]]; then
		echo "Error: expected '$required_path' in current working directory '$PWD'." >&2
		echo "Run this script from the repository root." >&2
		exit 1
	fi
done

python3 -m venv /tmp/release-annoucement.venv
# This is the python venv, no point running shellcheck on it
# shellcheck disable=SC1091
source /tmp/release-annoucement.venv/bin/activate
DELAY_SECS="${DELAY_SECS:-5}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
python3 -m pip install -q "${SCRIPT_DIR}/release_announcement"
RELEASE_ANNOUNCEMENT_PROG="$(basename "${BASH_SOURCE[0]}")" python3 -m release_announcement --delay-secs "$DELAY_SECS" "$@"
