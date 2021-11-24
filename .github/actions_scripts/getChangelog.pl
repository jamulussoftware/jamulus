#!/usr/bin/env perl

use strict;
use warnings;
# get the file path
my $ChangeLogFile = $ARGV[0];
# get the version
my $Version = $ARGV[1];
# open the file
open my $fh, '<', $ChangeLogFile or die;

my $logversion = "";
my $entry;
while (my $line = <$fh>) {
   if ($line =~ /^### (\S+)[^#]*###$/m) {
       # The version for this section of the ChangeLog
       $logversion = $1;
       next;
   }
   if ($logversion eq $Version) {
       if ($line =~ /^- (.*)$/) { # beginning of entry
           $entry = $1;
       } elsif ($line =~ /^ ( \S.*)$/) { # continuation of entry
           $entry .= $1;
       } else { # otherwise, separator between entries
           print "${entry}\n" if $entry;
           $entry = undef;
       }
   }
}
close $fh;
