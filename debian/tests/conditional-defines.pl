#!/usr/bin/perl
#
# Check http://config.privoxy.org/show-status for Conditional #defines enabled
#
# (c) 2022 Roland Rosenfeld <roland@debian.org>

use strict;
use warnings;
use LWP::UserAgent ();
use HTML::TreeBuilder 5 -weak;

my $exitcode = 0;

my $ua = LWP::UserAgent->new(timeout => 10);
$ua->env_proxy;
my $response = $ua->get('http://config.privoxy.org/show-status');
if (!$response->is_success) {
   die $response->status_line;
}
my $tree = HTML::TreeBuilder->new;
$tree->parse($response->decoded_content);

# Search for "Conditional #defines:" table:
my $summary = 'The state of some ./configure options and what they do.';
my $table = $tree->look_down('_tag' => 'table',
                             'summary' => $summary);
unless (defined $table) {
   die "summary '$summary' not found in tables";
}

# These features are intentionaly disabled, all others should be enabled:
my %disabled_features = ('FEATURE_ACCEPT_FILTER' => 1, # BSD only
                         'FEATURE_STRPTIME_SANITY_CHECKS' =>1, # BSD libc only
                         'FEATURE_GRACEFUL_TERMINATION' =>1, # devel only
                        );

my $enabled = 0;
my $disabled_ok = 0;
my $disabled_nok = 0;
foreach my $tr ($table->look_down('_tag' => 'tr')) {
   my $td2 = ($tr->look_down('_tag' => 'td')) [1];
   next unless defined $td2;
   my $code = $tr->look_down('_tag' => 'code');
   my $feature = $code->detach_content;
   my $value = $td2->detach_content;
   if ($value !~ /Yes/) {
      # feature disabled, check whitelist
      if ($feature eq 'FEATURE_64_BIT_TIME_T') {
         # See https://en.wikipedia.org/wiki/Year_2038_problem
         # On Linux >= 5.6 time_t should be 64bit, too.
         printf "%s is disabled, which is ok on 32bit systems", $feature;
         $disabled_ok++;
      } elsif (! defined $disabled_features{$feature}) {
         printf "%s is disabled, but should be enabled\n", $feature;
         $exitcode = 1;
         $disabled_nok++;
      } else {
         $disabled_ok++;
      }
   } else {
      $enabled++;
   }
}

printf "%d features enabled\n", $enabled;
printf "%d features intentionally disabled\n", $disabled_ok;
printf "%d features unintentionally disabled\n", $disabled_nok;

if ($enabled < 10) {
   printf STDERR "Found only %d Conditional #defines, seems test ist broken\n",
                 $enabled;
   $exitcode = 1;
}

exit $exitcode;
