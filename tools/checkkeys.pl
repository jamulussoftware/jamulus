#!/usr/bin/perl
##############################################################################
# Copyright (c) 2021-2024
#
# Author(s):
#  Tony Mountifield
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

use open qw(:std :utf8);
use XML::Simple qw(:strict);
use Data::Dumper;

my %keys;

my $doneeng = 0;

while ($ts = <*.ts>) {
	my $xs = XMLin($ts, KeyAttr => {}, ForceArray => ['context', 'message']);

	# print Data::Dumper->Dump([$xs], [qw(xs)]);

	printf "Language: %s\n", $xs->{language};

	foreach $context (@{$xs->{context}}) {
		# printf "\n========================================\nContext: %s\n", $context->{name};
		my $contextname = $context->{name};
		$contextname =~ s/Base$//;	# merge base class with its child
		$contextname = 'CClientDlg+CHelpMenu' if ($contextname eq 'CClientDlg' || $contextname eq 'CHelpMenu');

		foreach $message (@{$context->{message}}) {
			# printf "  Msg: %s\n", $message->{source};
			next if $message->{translation}{type} eq 'obsolete';
			next if $message->{translation}{type} eq 'vanished';
			#next if $message->{translation}{type} eq 'unfinished';	# don't skip unfinished strings, as they may still get used

			next unless $message->{source} =~ /\&(.)/;

			push @{$keys{en}{$contextname}{uc $1}}, $message->{source} unless $doneeng;

			if (exists($message->{translation}{content})) {
				if ($message->{translation}{content} =~ /\&(.)/) {
					push @{$keys{$xs->{language}}{$contextname}{uc $1}}, $message->{translation}{content} . " (" . $message->{source} . ")";
				}
			} elsif ($message->{translation} =~ /\&(.)/) {
				push @{$keys{$xs->{language}}{$contextname}{uc $1}}, $message->{translation} . " (" . $message->{source} . ")";
			}

		}
	}

	#print Data::Dumper->Dump([\%keys], [qw(*keys)]);

	#exit;
	$doneeng = 1;
}

print "\nPossible duplicate hotkeys:\n";

foreach $lang (sort keys %keys) {
	my $lref = $keys{$lang};
	my $langname = "\n".$lang;
	foreach $context (sort keys %{$lref}) {
		my $cref = $lref->{$context};
		my $contextname = $context;
		foreach $letter (sort keys %{$cref}) {
			if (@{$cref->{$letter}} > 1) {
				if ($langname) {
					print $langname,"\n";
					$langname = '';
				}
				if ($contextname) {
					print "\t",$contextname,"\n";
					$contextname = '';
				}
				printf "\t\tKey: %s\n", $letter;

				foreach $s (@{$cref->{$letter}}) {
					printf "\t\t\t%s\n", $s;
				}
			}
		}
	}
}
