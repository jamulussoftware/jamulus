#!/usr/bin/env perl
##############################################################################
# Copyright (c) 2021-2025
#
# Author(s):
#  ann0see
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

use strict;
use warnings;
# get arguments
my ($ChangeLogFile, $Version, $FormatOption) = @ARGV;
# open the file
open my $fh, '<', $ChangeLogFile or die;

my $SingleLineEntries = ($FormatOption or '') eq '--line-per-entry';

my $logversion = "";
my $entry;
my $matchedLogversion = 0;
while (my $line = <$fh>) {
   if ($line =~ /^### (\S+)[^#]*###$/m) {
       # The version for this section of the ChangeLog
       if ($matchedLogversion) {
           # We've processed the lines corresponding to $Version, and
           # we're now on the next section.  There's no more to do.
           last;
       }
       $logversion = $1;
       next;
   }
   if ($logversion eq $Version) {
       $matchedLogversion = 1;
       if ($SingleLineEntries) {
           if ($line =~ /^- (.*)$/) { # beginning of entry
               $entry = $1;
           } elsif ($line =~ /^ ( \S.*)$/) { # continuation of entry
               $entry .= $1;
           } else { # otherwise, separator between entries
               print "${entry}\n" if $entry;
               $entry = undef;
           }
       } else {
           print $line;
       }
   }
}

close $fh;

if (!$matchedLogversion) {
    die "Failed to find entry for version '${Version}'";
}
