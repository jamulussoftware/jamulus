#!/usr/bin/perl
##############################################################################
# Copyright (c) 2021-2025
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
use XML::LibXML;
use Data::Dumper;

my %keys;

my $doneeng = 0;
my $language;

while ($ts = <*.ts>) {
	my $dom = XML::LibXML->load_xml(location => $ts);

	foreach my $TS ($dom->findnodes('//TS/@language')) {
		$language = $TS->to_literal();
		printf "Language: %s\n", $language;
	}

	foreach my $context ($dom->findnodes('//context')) {
		my $contextname = $context->findvalue('./name');
		# printf "\n========================================\nContext: %s\n", $contextname;
		$contextname =~ s/Base$//;      # merge base class with its child
		$contextname = 'CClientDlg+CHelpMenu' if ($contextname eq 'CClientDlg' || $contextname eq 'CHelpMenu');

		MESSAGE: foreach my $message ($context->findnodes('./message')) {
			my $source = $message->findvalue('./source');
			# printf "  Msg: %s\n", $source;

			foreach my $translation ($message->findnodes('./translation')) {
				my $type = $translation->{type};
				next MESSAGE if $type eq 'obsolete';
				next MESSAGE if $type eq 'vanished';
				#next MESSAGE if $type eq 'unfinished';	# don't skip unfinished strings, as they may still get used

				# skip messages without an accelerator key
				next MESSAGE unless $source =~ /\&(.)/;

				push @{$keys{en}{$contextname}{uc $1}}, $source unless $doneeng;

				my $content = $translation->to_literal();
				if ($content =~ /\&(.)/) {
					push @{$keys{$language}{$contextname}{uc $1}}, $content . " (" . $source . ")";
				}
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
