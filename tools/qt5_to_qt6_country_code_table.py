#!/usr/bin/env python3
##############################################################################
# Copyright (c) 2022-2024
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

"""
Jamulus uses Qt5 QLocale::Country codes in its protocol.
Qt6 changed those codes in an incompatible way.
To make >=Qt6 clients compatible with the Jamulus protocol in the wild,
we need to translate between those codes.
This script generates two translation tables from Qt5 + Qt6 header files:
A table to translate from Qt5 to Qt6 and an inverse table.
"""
import re

# verify that the following actually points to Qt5 on your system!
QT5_INCLUDE = '/usr/include/qt/QtCore/qlocale.h'
QT6_INCLUDE = '/usr/include/qt6/QtCore/qlocale.h'


def parse_enum_from_header(header):
    name_to_val = {}
    val_to_name = {}
    with open(header) as f:
        s = f.read()
    matches = re.search(r'enum Country[^\n]+{([^}]+)}', s)
    assignments = matches.group(1)

    assign_pattern = re.compile(r'^\s*(\S+)(?:\s+.*)?\s+=\s+([^\s,]+),?$')
    for assignment in assignments.split('\n'):
        if assignment.startswith('#') or not assignment.strip():
            continue
        matches = assign_pattern.match(assignment)
        if not matches:
            raise ValueError(f"unable to match line: {assignment!r}")
        name = matches.group(1)
        val = matches.group(2)
        if val.isnumeric():
            val = int(val)
            if val not in val_to_name:
                val_to_name[val] = name
            name_to_val[name] = val
        elif val in name_to_val:
            # resolve aliases:
            name_to_val[name] = name_to_val[val]
        else:
            raise ValueError("Unhandled case")
    return name_to_val, val_to_name


def make_struct(name, table):
    ret = ''
    highest_key = max(int(x) for x in table.keys())
    # Need to escape the literal '{' by using '{{'
    ret += f'constexpr int const static {name}[] = {{'
    for key in range(highest_key + 1):
        ret += str(table.get(key, -1))  # fill gaps with -1
        ret += ', '
    ret += '};'
    return ret


qt5_name_to_val, qt5_val_to_name = parse_enum_from_header(QT5_INCLUDE)
qt6_name_to_val, qt6_val_to_name = parse_enum_from_header(QT6_INCLUDE)

qt5_to_qt6 = {}
qt6_to_qt5 = {}
# We need to support all Qt5 codes, so we work by value:
for qt5_val, name in qt5_val_to_name.items():
    qt6_val = qt6_name_to_val[name]
    qt5_to_qt6[qt5_val] = qt6_val
    qt6_to_qt5[qt6_val] = qt5_val

print(make_struct('wireFormatToQt6Table', qt5_to_qt6))
print(make_struct('qt6CountryToWireFormat', qt6_to_qt5))
