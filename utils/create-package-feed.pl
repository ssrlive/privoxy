#!/usr/local/bin/perl
#< LICENSE: WTFPL >
use warnings;
use strict;
use Digest::SHA;
my @months = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);
my @days   = qw(Sun Mon Tue Wed Thu Fri Sat Sun);

#< Config START >
my $scan_dir = shift(@ARGV)
    or die "Local package directory not specified (first argument)\n";
my $save_rss_file = shift(@ARGV)
    or die "RSS output file path not specified (second argument)\n";
my $base_dlurl = 'https://www.privoxy.org/sf-download-mirror/';
my $maxlimit = 1000;
my $max_advertised_files = 100;

#< Config END >

my @Array = ();
my $i     = 0;
my $sec;
my $min;
my $hour;
my $mday;
my $mon;
my $year;
my $wday;
my $yday;
my $isdst;
my $target;
my $target_sha256;
my $target_uri;
my $target_time;
my $target_line;

#
# 1st & 2nd directory should NOT contain ANY 'FILES'. (expecting only 'Directory')
#
opendir(my $D1, $scan_dir) or die "Can't open 1st directory! /";
while (my $fi1 = readdir($D1)) {
    next if ($fi1 =~ m/^\./);

    next if ($fi1 eq 'OldFiles' or $fi1 eq 'pkgsrc');
    opendir(my $D2, $scan_dir . $fi1 . '/')
        or die "Can't open 2nd directory! /$fi1";
    while (my $fi2 = readdir($D2)) {
        next if ($fi2 =~ m/^\./);

## start listing /OS/Version/FILE
        opendir(my $D3, $scan_dir . $fi1 . '/' . $fi2 . '/')
            or die "Can't open 3rd directory! /$fi1/$fi2";
        while (my $fi3 = readdir($D3)) {
            next if ($fi3 =~ m/^\./);
            $target = $scan_dir . $fi1 . '/' . $fi2 . '/' . $fi3;
            next if (!-e $target);    # skip if file is not exist

            # Get SHA-256 hash
            my $filedata;
            open($filedata, "<", $target)
                or die "Can't open '$target' to generate checksum $!";
            my $sha256 = Digest::SHA->new("SHA-256");
            $sha256->addfile($filedata);
            close($filedata);
            $target_sha256 = $sha256->hexdigest;

            # URI and Time
            $target_uri  = $fi1 . '/' . $fi2 . '/' . $fi3;
            my $escaped_target_uri = $target_uri;
            $escaped_target_uri =~ s@ @%20@g;
            $target_time = (stat $target)[9];

            # RSS line
            $target_line =
                '<item><title><![CDATA[' . $target_uri . ']]></title>';
            $target_line .=
                  '<description><![CDATA['
                . $target_uri
                . ' (SHA-256: '
                . $target_sha256
                . ')]]></description>';
            $target_line .=
                  '<link>'
                . $base_dlurl
                . $escaped_target_uri
                . '</link><guid>'
                . $base_dlurl
                . $escaped_target_uri
                . '</guid>';
            $target_line .= '<pubDate>';
            ($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) =
                gmtime($target_time);
            $target_line .= sprintf("%s, %s %s %d %.2d:%.2d:%.2d GMT",
                $days[$wday], $mday, $months[$mon], ($year + 1900),
                $hour, $min, $sec);
            $target_line .= '</pubDate></item>';
            $target_line .= "\n";

            # Add it to Array
            $Array[$i] = ([$target_time, $target_line]);
            $i++;
            die "maxlimit $maxlimit reached!" unless ($i < $maxlimit);
        }
        closedir($D3);
## end listing /OS/Version/FILE

    }
    closedir($D2);

}
closedir($D1);

# Result = Full XML Codes
my $result =
    '<?xml version="1.0" encoding="utf-8"?>
 <rss xmlns:content="http://purl.org/rss/1.0/modules/content/" version="2.0">
  <channel>
   <title>Privoxy Releases</title>
   <link>https://www.privoxy.org/announce.txt</link>
   <description><![CDATA[Privoxy Releases RSS feed]]></description>
   <pubDate>';
($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = gmtime();
$result .=
      "$days[$wday], $mday $months[$mon] "
    . ($year + 1900)
    . " $hour:$min:$sec GMT";
$result .= '</pubDate>';
$result .= "\n";

# Sort Array
my @resArray = sort { @$a[0] <=> @$b[0] } @Array;
$i--;
while ($max_advertised_files-- > 0 && $i >= 0) {
    $result .= $resArray[$i][1];
    $i--;
}
$result .= '  </channel>
   </rss>';

# Save it.
open(my $XMLF, ">", $save_rss_file) or die "Failed to write XML file";
print $XMLF $result;
close($XMLF);
