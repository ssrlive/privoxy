#!/usr/bin/perl

# $Id: filter2docs.pl,v 1.3 2008/05/11 12:31:29 fabiankeil Exp $
# $Source: /cvsroot/ijbswa/current/utils/filter2docs.pl,v $

# Parse the filter names and descriptions from a filter file and
# spit out copy&paste-ready markup for the various places in
# configuration and documentation where all filters are listed.

use strict;
use warnings;

die "Usage: $0 filter-file\n" unless (@ARGV == 1) ;
open(INPUT, "< $ARGV[0]") or die "Coudln't open input file $ARGV[0] because $!\n";

my ($comment_lines, $action_lines, $sgml_source_1, $sgml_source_2);

while (<INPUT>) {
  if (/^(FILTER): ([-\w]+) (.*)$/) {
    my $type_uc = $1;
    my $name = $2;
    my $description = $3;
    my $type = lc($type_uc);

    $comment_lines .= "#     $name:" . (" " x (20-length($name))) . "$description\n";
    $action_lines  .= "+$type" . "{$name} \\\n";
    $sgml_source_1 .= "   <para>\n    <anchor id=\"$type-$name\">\n    <screen>+" . $type . "{$name}" .
                      (" " x (20-length($name))) . "# $description</screen>\n   </para>\n";
    $sgml_source_2 .= " -<link linkend=\"" . $type_uc . "-" . uc($name) . "\">$type" . "{$name}</link> \\\n";
  }
}

print <<DOCMARKUP;

Comment lines for default.action:

$comment_lines

Block of filter actions for standard.action:

$action_lines

SGML Source for AF chapter in U-M:

$sgml_source_1

SGML Source for AF Tutorial chapter in U-M:

$sgml_source_2
DOCMARKUP
