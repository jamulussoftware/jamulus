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
while (my $line = <$fh>) {
   if ($line =~ /^### (\S+)[^#]*###$/m) {
       # The version for this section of the ChangeLog
       $logversion = $1;
       next;
   }
   if ($logversion eq $Version) {
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
