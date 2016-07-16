#!/usr/local/bin/perl
#< LICENSE: WTFPL >
use warnings;
use strict;
use Digest::SHA1;
my @months=qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);
my @days=qw(Sun Mon Tue Wed Thu Fri Sat Sun);

#< Config START >
my $scan_dir='/xxxxxxxxxxxxxxxxxxxxxx/sf-download/';
my $base_dlurl='https://www.privoxy.org/sf-download-mirror/';
my $save_rss_file='/xxxxxxxxxxxxxxxxxxxxxx/release.xml';# e.g., release.rss
my $maxlimit=10;
#< Config END >

my @Array=();
my $i=0;
my $sec;my $min;my $hour;my $mday;my $mon;my $year;my $wday;my $yday;my $isdst;
my $target;my $target_sha1;my $target_uri;my $target_time;my $target_line;

# 
# 1st & 2nd directory should NOT contain ANY 'FILES'. (expecting only 'Directory')
# 
opendir (D1,$scan_dir) or die "Can't open 1st directory! /";
MOUT: while (my $fi1=readdir(D1)){
next if ($fi1 =~ m/^\./);

opendir (D2,$scan_dir.$fi1.'/') or die "Can't open 2nd directory! /$fi1";
while (my $fi2=readdir(D2)){
next if ($fi2 =~ m/^\./);

## start listing /OS/Version/FILE
opendir (D3,$scan_dir.$fi1.'/'.$fi2.'/') or die "Can't open 3rd directory! /$fi1/$fi2";
while (my $fi3=readdir(D3)){
next if ($fi3 =~ m/^\./);
$target=$scan_dir.$fi1.'/'.$fi2.'/'.$fi3;next if (! -e $target);# skip if file is not exist

# Get SHA-1 hash
my $filedata;
unless (open $filedata,$target){next;}
my $sha1 = Digest::SHA1->new;$sha1->addfile($filedata);close $filedata;
$target_sha1=$sha1->hexdigest;

# URI and Time
$target_uri=$fi1.'/'.$fi2.'/'.$fi3;
$target_time=(stat $target)[9];

# RSS line
$target_line='<item><title><![CDATA['.$target_uri.']]></title>';
$target_line.='<description><![CDATA['.$target_uri.' (SHA-1: '.$target_sha1.')]]></description>';
$target_line.='<link>'.$base_dlurl.$target_uri.'</link><guid>'.$base_dlurl.$target_uri.'</guid>';
$target_line.='<pubDate>';
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)=gmtime($target_time);$target_line.="$days[$wday], $mday $months[$mon] ".($year+1900)." $hour:$min:$sec GMT";
$target_line.='</pubDate></item>';

# Add it to Array
$Array[$i]=([$target_time,$target_line]);$i++;
if ($i>=$maxlimit){last MOUT;}
}
closedir D3;
## end listing /OS/Version/FILE

}
closedir D2;

}
closedir D1;

# Result = Full XML Codes
my $result='<?xml version="1.0" encoding="utf-8"?><rss xmlns:content="http://purl.org/rss/1.0/modules/content/" version="2.0"><channel><title>Privoxy Releases</title><link>https://www.privoxy.org/announce.txt</link><description><![CDATA[Privoxy Releases RSS feed]]></description><pubDate>';
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)=gmtime();$result.="$days[$wday], $mday $months[$mon] ".($year+1900)." $hour:$min:$sec GMT";
$result.='</pubDate>';
# Sort Array
my @resArray=sort {@$a[0]<=>@$b[0]} @Array;$i--;
while($i>=0){$result.=$resArray[$i][1];$i--;}  
$result.='</channel></rss>';
# Save it.
open(XMLF,"> $save_rss_file") or die "Failed to write XML file";  
print XMLF $result;
close(XMLF);
