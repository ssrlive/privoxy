<?php

error_reporting(E_NONE);
#error_reporting(E_ALL);

//  File :  $Source: /cvsroot/ijbswa/current/doc/webserver/actions/index.php,v $
//
//  Purpose  :  Submit form for actions file feedback (step 1)
//              This file belongs in
//              ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/
//
//  $Id: index.php,v 1.1 2002/03/30 03:20:30 oes Exp $
//
//  $Log: index.php,v $
//  Revision 1.1  2002/03/30 03:20:30  oes
//  Added Feedback mechanism for actions file
//
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

/*
 * MUST be updated in sync with actions file:
 */
$current_actions_file_version = "1.0";

/*
 * For testing: 
 */
#$base_url = "http://www.oesterhelt.org/actions";
$base_url = "http://www.privoxy.org/actions";


/* 
 * Provide default if unset
 */
if (!isset($url))
{
   $url = "http://www.example.com/";
}

/* 
 * Deny feedback which is not based on our latest
 * distribution:
 */
$headers = getallheaders();

if (!isset($headers["X-Actions-File-Version"]) || $headers["X-Actions-File-Version"] < $current_actions_file_version)
{
   echo ("<html><head><title>Invalid Privoxy Action List Feedback</title></head>
                <body><h2>Invalid Feedback Submission</h2>
                      <p>You are either not using Privoxy at all, or using an
                         actions file which is not based on the recent
                         distribution actions file (version $current_actions_file_version).</p>
                      <p>As much as we welcome your feedback, we are unable to process
                         input for the actions file if not based on our distribution. Sorry.</p>
                      <p>You can <a href=\"http://www.privoxy.org/\">download the latest version of
                         Privoxy here.</a></p>
                 </body>
          </html>");
   exit;
}

?>

<html>
  <head>
    <title>Privoxy Action List Feedback - Step 1 of 2</title>
    <link rel="stylesheet" type="text/css" href="../p_web.css">
</head>

<h2><a href="http://www.privoxy.org" target="_blank">Privoxy</a> Action List Feedback - Step 1 of 2</h2>

<p>
<b>Thank you for reporting a missing or invalid action!</b>
</p>
<p>
Please fill the below form and click to
proceed to Step 2.
</p>

<form action="<?php echo($base_url); ?>/step2.php" method="post">

<table border="0" cellpadding="0" cellspacing="4">

<tr>
<td align="right">URL:</td>
<td>
<input name="referrer_url" value="<?php echo ($url); ?>" type="text" size="45" maxlength="255">
</td>
</tr>

<tr>
<td align="right">Nature of the problem:</td>
<td>
<select name="problem" size="1">
<option selected value="INVALID">Please select...</option>
<option value="P1">An advertisment was NOT blocked</option>
<option value="P2">An innocent image WAS blocked</option>
<option value="P3">The whole page was erraneously blocked</option>
<option value="P4">The page needs popups but they don't work</option>
<option value="P5">Other problem</option>
</select>
</td>
</tr>

<tr>
<td align="right"></td>
<td>
<input type=submit value="Proceed to step 2">
</td>
<td></td>
</tr>

</table>
</form>
</p>

<p>To faciliate your feedback, you can bookmark <a href="javascript:void(window.open('http://privoxy.org/actions/?url='+escape(location.href),'Feedback', 'width=600,scrollbars=yes,toolbar=no,location=no,directories=no,status=no,menubar=no,copyhistory=no').focus());">this bookmarklet</a><br><i>(right-click and select "File Bookmark for link")</i>.
<br>Used on a page that you want to report on, it will take you here and pre-fill the URL field.
</p>

</body>
</html>
