<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
 <!--

  File :  $Source: /cvsroot/ijbswa/current/doc/webserver/actions/index.php,v $

  Purpose  :  Submit form for actions file feedback (step 1)
              This file belongs in
              ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/

  $Id: index.php,v 1.25.2.7 2004/01/30 17:16:44 oes Exp $

  $Log: index.php,v $
  Revision 1.25.2.7  2004/01/30 17:16:44  oes
  Require AF 1.8

  Revision 1.25.2.6  2003/11/30 11:39:52  oes
  Fixed typo

  Revision 1.25.2.5  2003/09/01 15:20:45  oes
  Require AF 1.7

  Revision 1.25.2.4  2003/03/19 14:00:20  oes
  Require AF 1.6

  Revision 1.25.2.3  2002/08/27 16:33:39  oes
  Require AF 1.5 & Privoxy 3.0

  Revision 1.25.2.2  2002/08/06 08:39:09  oes
  Require AF 1.4; link to new AF download location

  Revision 1.25.2.1  2002/07/27 18:47:49  oes
  Require Privoxy 2.9.16 and AF 1.3

  Revision 1.25  2002/04/29 17:30:20  oes
  Fixed BML link text

  Revision 1.24  2002/04/28 16:56:47  swa
  bookmarklet text

  Revision 1.23  2002/04/13 14:13:19  oes
  Require exact AF version; Added hint where to go for BRs, FRs and SRs

  Revision 1.22  2002/04/11 10:11:04  oes
  Actionsfile Version 1.2

  Revision 1.21  2002/04/10 13:51:19  oes
  Updated to new Bookmarklet

  Revision 1.20  2002/04/10 00:07:35  oes
  Moved window sizing and positioning code to Bookmarklet

  Revision 1.19  2002/04/09 13:06:29  oes
  Resize and jump to the right on load

  Revision 1.18  2002/04/08 17:03:29  oes
   - Fixed problem with spaces in URLs
   - Adapt to unified stylesheet

  Revision 1.17  2002/04/08 10:32:00  oes
  cosmetics again

  Revision 1.16  2002/04/08 08:11:04  oes
  Bumped up actions file number

  Revision 1.15  2002/04/07 17:13:08  oes
  Ooops: fixing submit target url

  Revision 1.14  2002/04/07 15:10:12  oes
  Restoring CVS history

  Revision 1.13  2002/04/06 15:19:35  oes
  Clean-up, smarter handling of unreachable URLs

  Revision 1.12  2002/04/06 11:34:44  oes
  Cosmetics

  Revision 1.11  2002/04/04 19:48:11  oes
  Reactivating the scripts ,-)

  Revision 1.10  2002/04/03 19:36:04  swa
  consistent look

  Revision 1.9  2002/04/02 19:32:45  oes
  Adding temporary fix for missing curl support on SF (step 2 + 3 on oesterhelt.org)

  Revision 1.8  2002/04/02 08:45:22  oes
  Made script location indepandant

  Revision 1.7  2002/04/02 07:21:34  oes
  Using relative link for step2

  Revision 1.6  2002/04/02 06:14:22  oes
  Fixed bookmarklet

  Revision 1.5  2002/04/01 19:13:47  oes (based on 1.2)
  Extended, fixed bugs, beefed up design, made IE-safe

  Revision 1.4  2002/03/30 20:44:44  swa
  have consistent look and feel. part 2.
  use correct urls.

  Revision 1.3  2002/03/30 19:49:34  swa
  have consistent look and feel

  Revision 1.2  2002/03/30 03:35:48  oes
  Updated bookmarklet

  Revision 1.1  2002/03/30 03:20:30  oes
  Added Feedback mechanism for actions file


  Copyright (C) 2002 the SourceForge Privoxy team. 
  http://www.privoxy.org/

  Written by Andreas Oesterhelt

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
  <meta http-equiv="Content-Style-Type" content="text/css">
  <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
  <meta http-equiv="Content-Script-Type" content="text/javascript">
  <link rel="stylesheet" type="text/css" href="../privoxy.css">
  <link rel="stylesheet" type="text/css" href="../p_feedback.css">

<?php

/*
 * Config:
 */
$required_actions_file_version = "1.8";
$required_privoxy_version = "3.0";
$actions_file_download = "http://sourceforge.net/project/showfiles.php?group_id=11118&release_id=147447";
$submit_target = "http://www.oesterhelt.org/actions/step2.php";


/*
 * Debug:
 */
//phpinfo();
//error_reporting(E_ALL);
error_reporting(E_NONE);

/*
 * Function: error_abort
 * Purpose:  Return an error page with $title and $message
 */
function error_abort($title, $message)
{
   if ($title == "invalid") /* shortcut */
   {
      $title = "Invalid Feedback Submission";
   }

   echo ("  <title>Privoxy: $title</title>
           </head>
           <body>
            <div class=\"title\">
             <h1>
              <a href=\"http://www.privoxy.org/\">Privoxy</a>: $title
              </h1>
             </div>
            <center>
             <div class=\"warning\">
              $message
             </div>
            </center>
            <p>Valid <a href=\"http://validator.w3.org/\">HTML 4.01 Transitional</a></p>
           </body>
          </html>\n");
   exit; 
}


/*
 * Bookmarklet that leads here:
 */
$my_address = "http://" . $HTTP_SERVER_VARS["HTTP_HOST"] . $PHP_SELF;
$bookmarklet = "javascript:w=Math.floor(screen.width/2);h=Math.floor(screen.height*0.9);void(window.open('$my_address?url='+escape(location.href)," .
               "'Feedback','screenx='+w+',width='+w+',height='+h+',scrollbars=yes,toolbar=no,location=no,directories=no,status=no,menubar=no," .
               "copyhistory=no').focus());";

/* 
 * Provide default if URL unset
 */
if (!isset($url))
{
   $url = "http://www.example.com/";
}
else
{
   $url = strtr($url, " ", "+");
}

/* 
 * Deny feedback which is not based on our latest
 * distribution:
 */
$headers = getallheaders();

if (!isset($headers["X-Actions-File-Version"]) || $headers["X-Actions-File-Version"] != $required_actions_file_version)
{

   error_abort("invalid", "<p><b>Thank you for using <a href=\"http://www.privoxy.org/\" target=\"_blank\">Privoxy</a>'s
               feedback reporting mechanism!</b> However, in order to make optimal use of our limited development resources,
               we regret that we can at this time only accept problem reports based on:
               </p>
               <ul>
                <li><a href=\"http://www.privoxy.org/\" target=\"_blank\">Privoxy</a> version $required_privoxy_version or later</li>
                <li><a href=\"$actions_file_download\">Actionsfile</a> version version $required_actions_file_version</li>
               </ul>
               <p>We hope you will understand that we feel unable to maintain concurrent versions of the file.</p>
               <p><i>Hint: To upgrade your actions file, follow the above link to download the file, then save as
                  default.action in your Privoxy config directory</i>
               </p>");
}

?>

  <title>Privoxy Action List Feedback - Step 1 of 2</title>
 </head>

 <body>
  <div class="title">
    <h1>
      <a href="http://www.privoxy.org" target="_blank">Privoxy</a> Action List Feedback - Step 1 of 2
    </h1>
  </div>

  <div class="box">
   <p>
    <b>Thank you for reporting a missing or invalid action!</b> 
   </p>

   <p>
    The Privoxy team relies on <b>your</b> feedback to maintain an efficient actions file!
    <br>Please fill the below form and click to proceed to step 2.
   </p>

   <p>
    Please keep in mind that this is <b>not</b> the place for
    <a href="http://sourceforge.net/tracker/?group_id=11118&amp;atid=211118" target="_blank">support requests</a>,
    <br><a href="http://sourceforge.net/tracker/?group_id=11118&amp;atid=111118" target="_blank">bug reports</a> or
    <a href="http://sourceforge.net/tracker/?atid=361118&amp;group_id=11118" target="_blank">feature requests</a>.
   </p>

  </div>

  <div class="box">
   <form action="<?php echo($submit_target); ?>" method="post">

    <table border="0" cellpadding="0" cellspacing="4">

     <tr>
      <td align="right">URL:</td>
      <td>
       <input name="referrer_url" value="<?php echo($url); ?>" type="text" size="45" maxlength="255">
      </td>
     </tr>

     <tr>
      <td align="right">Nature of the problem:</td>
      <td>
       <select name="problem" size="1">
        <option selected value="INVALID">Please select...</option>
        <option value="P1">An advertisment was NOT blocked</option>
        <option value="P2">An innocent image WAS blocked</option>
        <option value="P3">The whole page was erroneously blocked</option>
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

  <center>
   <div class="info">
    <h2>Using <a href="http://www.bookmarklets.com" target="_blank">Bookmarklets</a> for Feedback</h2>
    <p>
     To make it even easier for you, we provide a bookmarklet which will not only take you here from
     any troubled page you might be surfing, but also pre-fill the form!
    </p>
    <p>
     Please right-click the following link and choose "Add to Favorites" (IE) or "Add Bookmark for Link" (Netscape): 
     <a href="<?php echo($bookmarklet); ?>">Privoxy - Submit Actions File Feedback</a>
    </p>

    <p>
     <i>You might get a warning that the bookmark "may not be safe" (IE) - just click OK.
     For even faster access, you can put it on the "Links" bar (IE) or the "Personal Toolbar" (Netscape),
     and submit feedback with a single click!</i>
    </p>
   </div>
  </center>

  <p>Valid <a href="http://validator.w3.org/">HTML 4.01 Transitional</a></p>

 </body>
</html>
