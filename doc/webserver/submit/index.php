<?php
//  File :  $Source: /cvsroot/ijbswa/current/doc/webserver/submit/index.php,v $
//
//  Purpose  :  Submit form for ads and such
//              This file belongs in
//              ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/
//
//  $Id: index.php,v 1.4 2002/03/29 09:55:21 swa Exp $
//
//  Written by and Copyright (C) 2001 the SourceForge
//  Privoxy team. http://www.privoxy.org/
//
//  Based on the Internet Junkbuster originally written
//  by and Copyright (C) 1997 Anonymous Coders and
//  Junkbusters Corporation.  http://www.junkbusters.com
//
//  This program is free software; you can redistribute it
//  and/or modify it under the terms of the GNU General
//  Public License as published by the Free Software
//  Foundation; either version 2 of the License, or (at
//  your option) any later version.
//
//  This program is distributed in the hope that it will
//  be useful, but WITHOUT ANY WARRANTY; without even the
//  implied warranty of MERCHANTABILITY or FITNESS FOR A
//  PARTICULAR PURPOSE.  See the GNU General Public
//  License for more details.
//
//  The GNU General Public License should be included with
//  this file.  If not, you can view it at
//  http://www.gnu.org/copyleft/gpl.html
//  or write to the Free Software Foundation, Inc., 59
//  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//
$headers = getallheaders();
?>
<html>
  <head>
    <title>Privoxy|Submit</title>
    <link rel="stylesheet" type="text/css" href="../p_web.css">
</head>

<h1>Privoxy Feedback</h1>

<p>Compared to <a
href="http://sourceforge.net/tracker/?group_id=11118&atid=111118">bug
reports</a> or <a
href="http://sourceforge.net/tracker/?atid=361118&group_id=11118&func=browse">feature
requests</a>, this page is intended to optimize the blocking behavior
of Privoxy. Therefor we need your feedback.</p><p> If you have
observed and advertisement, that was not blocked or an image that was
incorrectly blocked, please use the forms below to report this.</p>

<h2>New Advertisement</h2>

<!-- testing mail sending
// <?
//$ret_val=mail("stefan@waldherr.org", "Subject", "Message"); echo $ret_val;
?>
-->


<?
$cfile  = "counter-data.inc";
$localip = "127.0.0.1";
$serverip = "127.0.0.1";
# Enter information on the next line, but only after you read
# the "c-readme.txt" file.
$browser_id = "";
if (file_exists ($cfile)) {
	$fp = fopen ($cfile,"r+");
	$data = fgets ($fp,25);
	$ip = chop (substr($data,0,15));
	$count = substr($data,15);
# Unremark the second "if" statement and remark the first one if
# your site is on a hosted server and you have a dynamic IP, but
# only after you read "c-readme.txt" file.
	if ($REMOTE_ADDR == $localip or $REMOTE_ADDR == $serverip)
#       if ($REMOTE_ADDR == substr_count($HTTP_USER_AGENT,$browser_id) > 0)
        	$np = $ip;
	else
		$np = $REMOTE_ADDR;
    	if ($np != $ip)
        	$count += 1;
	rewind ($fp);
	fputs ($fp,substr($np."        ",0,15).$count);
	fclose ($fp);
	echo $count;
}
else {
	$fp = fopen($cfile,"w");
	$np = $REMOTE_ADDR;
	$count = "1";
	fputs ($fp,substr($np."        ",0,15).$count);
	fclose ($fp);
	echo $count;
}
?>


<p>
<form action="http://privox.org/submit/confirmad.php" method="post">
<table border="0" cellpadding="0" cellspacing="4">

<tr>
<td align="right">Your name:</td>
<td>
<input name="submit_name" value="anonymous" type="text" size="30" maxlength="30">
</td>
<td>optional</td>
</tr>

<tr>
<td align="right">Your email address:</td>
<td>
<input name="submit_email" value="anonymous" type="text" size="30" maxlength="30">
</td>
<td>optional</td>
</tr>

<tr>
<td align="right">Website, where I observed an ad:</td>
<td>
<input name="submit_url" value="prefilled" type="text" size="45" maxlength="255">
</td>
<td>Please change, if necessary</td>
</tr>

<tr>
<td align="right">How annoying is the ad:</td>
<td>
<select name="submit_annoy" size="1">
<option value="A0">Nice, not to have</option>
<option selected value="A1">Should be removed</option>
<option value="A2">Must be removed</option>
</select>
</td>
<td>Please select one</td>
</tr>

<tr>
<td align="right">Privoxy Version:</td>
<td>
<input name="submit_pversion" value="prefilled" readonly type="text" size="30" maxlength="30">
</td>
<td>Automatically determined</td>
</tr>

<tr>
<td align="right">Action File:</td>
<td>
<input name="submit_actionfile" value="prefilled" readonly type="text" size="30" maxlength="30">
</td>
<td>Automatically determined</td>
</tr>

<tr>
<td align="right">Action File Version:</td>
<td>
<input name="submit_actionversion" value="prefilled" readonly type="text" size="30" maxlength="30">
</td>
<td>Automatically determined</td>
</tr>

<tr>
<td align="right">Remarks:</td>
<td>
<textarea name="submit_remarks" cols="35" rows="3">
None.
</textarea>
</td>
<td>Please change, if necessary</td>
</tr>

<tr>
<td align="right"></td>
<td>
<input type="hidden" name="submit_targeturl" value="/submit/confirmad.php">
<input type=submit value="Submit">
</td>
<td></td>
</tr>

</table>
</form>
</p>

<h2>Incorrect blocking</h2>
<p>We soon present a form where you can submit websites, where the
default action file was too agressive.</p>

<h2>Misc</h2>
<p>Bla bla bla</p>

</body>
</html>

<!--
	$Log: index.php,v $
	Revision 1.4  2002/03/29 09:55:21  swa
	.
	
	Revision 1.3  2002/03/29 09:39:48  swa
	added form
	
-->