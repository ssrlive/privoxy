#!/usr/bin/perl

# $Id: changelog2doc.pl,v 1.4 2010/10/31 13:26:07 fabiankeil Exp $
# $Source: /cvsroot/ijbswa/current/utils/changelog2doc.pl,v $

# Filter to parse the ChangeLog and translate the changes for
# the most recent version into something that looks like markup
# for the documentation but still needs fine-tuning.

use strict;
use warnings;

my @entries;

sub read_entries() {
    my $section_reached = 0;
    my $i = -1;

    while (<>) {
        if (/^\*{3} /) {
            last if $section_reached;
            $section_reached = 1;
            next;
        }
        next unless $section_reached;
        next if /^\s*$/;

        if (/^-/) {
            $i++; 
            $entries[$i]{description} = '';
        }
        s@^-?\s*@@;

        $entries[$i]{description} .= $_;
    }
    print "Parsed " . @entries . " entries.\n";
}

sub create_listitem_markup($) {
    my $entry = shift;
    my $description = $entry->{description};

    chomp $description;

    $description =~ s@\n@\n    @g;
    return "  <listitem>\n" .
           "   <para>\n" .
           "    " . $description . "\n" .
           "   </para>\n" .
           "  </listitem>\n";
}

sub wrap_in_para_itemlist_markup($) {
    my $content = shift;
    my $markup = "<para>\n" .
                 " <itemizedlist>\n" .
                 "  $content" .
                 " </itemizedlist>\n" .
                 "</para>\n";
    return $markup;
}

sub generate_markup() {
    my $markup = '';

    foreach my $entry (@entries) {
        $markup .= create_listitem_markup(\%{$entry});
    }

    print wrap_in_para_itemlist_markup($markup);
}

sub main () {
    read_entries();
    generate_markup();
}

main();
