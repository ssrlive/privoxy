<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
 <!--

  File :  $Source: /cvsroot/ijbswa/current/doc/webserver/actions/index.php,v $

  Purpose  :  Submit form for actions file feedback (step 1)
              This file belongs in
              ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/

  $Id: index.php,v 1.8 2002/04/02 08:45:22 oes Exp $

  $Log: index.php,v $
  Revision 1.8  2002/04/02 08:45:22  oes
  Made script location indepandant

  Revision 1.7  2002/04/02 07:21:34  oes
  Using relative link for step2

  Revision 1.5  2002/04/01 19:13:47  oes
  Extended, fixed bugs, beefed up design, made IE-safe

  Revision 1.2  2002/03/30 03:35:48  oes
  Updated bookmarklet

  Revision 1.1  2002/03/30 03:20:30  oes
  Added Feedback mechanism for actions file


  Written by and Copyright (C) 2002 the SourceForge
  Privoxy team. http://www.privoxy.org/

  This program is free software; you can redistribute it
  and/or modify it under the terms of the GNU General
  Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at
  your option) any later version.

  This program is distributed in the hope that it will
  be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  The GNU General Public License should be included with
  this file.  If not, you can view it at
  http://www.gnu.org/copyleft/gpl.html
  or write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 -->

 <head>
  <style type="text/css">
   body, div, p, h1, h2, ul, ol, li, td, th, dl, dt, dd { font-family:helvetica,helv,arial,sans-serif; font-size:10px }
   body { background-color: #ffffff }
   div.title    { background-color:#dddddd; border:solid black 1px; margin:20px; min-width: 80%; padding:20px; font-size:15px; font-weight:bold }
   div.box      { background-color:#eeeeee; border:solid black 1px; margin:20px; min-width: 80%; padding:20px; font-size:10px }
   div.infobox  { background-color:#ccccff; border:solid black 1px; margin:20px; min-width: 60%; max-width: 60%; padding:20px; font-size:10px; }
   div.errorbox { background-color:#ffdddd; border:solid black 1px; margin:20px; min-width: 60%; max-width: 60%; padding:20px; font-size:10px; }
  </style>

<?php

/*
 * MUST be updated in sync with actions file:
 */
$required_actions_file_version = "1.0";
$required_privoxy_version = "2.9.13";
$actions_file_download = "http://www.privoxy.org/actions/testdrive.action";


/*
 * For testing: 
 */
//phpinfo();
//error_reporting(E_ALL);
error_reporting(E_NONE);


/*
 * Bookmarklet that leads here:
 */
$my_address = "http://" . $HTTP_SERVER_VARS["HTTP_HOST"] . $PHP_SELF;
$bookmarklet = "javascript:void(window.open('$my_address?url='+escape(location.href), 'Feedback', " .
               "'width=600,scrollbars=yes,toolbar=no,location=no,directories=no,status=no,menubar=no,copyhistory=no').focus());";


/* 
 * Provide default if URL unset
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

if (!isset($headers["X-Actions-File-Version"]) || $headers["X-Actions-File-Version"] < $required_actions_file_version)
{
   echo ("<title>Invalid Privoxy Action List Feedback</title></head>
          <body><div class=\"title\">Invalid Feedback Submission</div>
           <div align=\"center\">
            <div class=\"errorbox\" align=\"left\"><p>As much as we welcome your feedback, please note that
             we can only accept problem reports based on:
             <ul>
              <li><a href=\"http://www.privoxy.org/\">Privoxy</a> version $required_privoxy_version or later</li>
              <li><a href=\"$actions_file_download\">Actionsfile</a> version  version $required_actions_file_version or later</li>
             </ul>
             <p>We hope you will understand that we feel unable to maintain concurrent versions of the file.</p>
            </div>
           </div>
          </body>
         </html>");
   exit;
}

?>

  <title>Privoxy Action List Feedback - Step 1 of 2</title>
 </head>

 <body>
  <div class="title"><a href="http://www.privoxy.org/" target="_blank">Privoxy</a> Action List Feedback - Step 1 of 2</div>

  <div class="box">
   <p>
    <b>Thank you for reporting a missing or invalid action!</b> 
    <br>The Privoxy team relies on <b>your</b> feedback to maintain an efficient actions file!
   </p>

   <p>
    Please fill the below form and click to proceed to step 2.
   </p>
  </div>

  <div class="box">
   <!-- FIXME: This must become action="step2.php" as soon as SF supports curl in PHP -->
   <form action="http://www.oesterhelt.org/actions/step2.php" method="post">

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
      <td>&nbsp;</td>
      <td>
       <input type=submit value="Proceed to step 2">
      </td>
     </tr>

    </table>
   </form>
  </div>

  <div align=center>
   <div class="infobox" width="60%" align=\"left\">
    <p>
     <big>
      <b>Using <a href="http://www.bookmarklets.com" target="_blank">Bookmarklets</a> for Feedback</b>
     </big>
    </p>
    <p>
     To make it even easier for you, we provide a bookmarklet which will not only take you here from
     any troubled page you might be surfing, but also pre-fill the form!
    </p>
    <p>
     Please right-click the following link and choose "Add to Favorites" (IE) or "Add Bookmark for Link" (Netscape): 
     <a href="<?php echo($bookmarklet); ?>">Privoxy-Submit</a>
    </p>

    <p>
     <i>You might get a warning that the bookmark "may not be safe" (IE) - just click OK.
     For even faster access, you can put it on the "Links" bar (IE) or the "Personal Toolbar" (Netscape),
     and submit feedback with a single click!</i>
    </p>
   </div>
  </div>

 </body>
</html>
