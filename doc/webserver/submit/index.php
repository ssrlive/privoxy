<?php
//  File :  $Source: /cvsroot/ijbswa/current/doc/webserver/submit/index.php,v $
//
//  Purpose  :  Submit form for ads and such
//              This file belongs in
//              ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/
//
//  $Id: index.php,v 1.2 2002/03/29 07:57:00 swa Exp $
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

<h1>Feedback</h1>

<p>Bla bla bla</p>

<h2>Advertisements</h2>

<p>You have seen an ad and want to support us making it disappear.
Great. Please fill out the form below and hit "Submit".</p>

<p>
<form action="http://privoxy.org/submit/confirmad.php" method="post">
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

<h2>Misc</h2>

<p>Bla bla bla</p>


</body>
</html>

<!--
	$Log$
-->