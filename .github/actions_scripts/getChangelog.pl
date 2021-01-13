#!/usr/bin/env perl

use strict;
use warnings;
# get the file path
my $ChangeLogFile = $ARGV[0];
# get the version
my $Version = quotemeta $ARGV[1];
# open the file (slurp mode): https://perlmaven.com/slurp
open my $fh, '<', $ChangeLogFile or die;
$/ = undef;
my $ChangeLog = <$fh>;
close $fh;

if ( $ChangeLog =~ m/^### $Version (.*?)$(.*?)^###/sm ) {
    print $2;
} else {
    print "No changelog found for this version.";
}
