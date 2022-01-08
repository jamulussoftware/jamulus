#!/usr/bin/env perl

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
