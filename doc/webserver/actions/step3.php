<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
 <!--

  File :  $Source: /cvsroot/ijbswa/current/doc/webserver/actions/step3.php,v $

  Purpose  :  Submit form for actions file feedback (step 1)
              This file belongs in
              ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/

  $Id: step3.php,v 1.5 2002/04/02 07:22:43 oes Exp $

  $Log: step3.php,v $
  Revision 1.5  2002/04/02 07:22:43  oes
  Cosmetics

  Revision 1.4  2002/04/01 19:13:47  oes
  Extended, fixed bugs, beefed up design, made IE-safe

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
  <meta http-equiv="Content-Style-Type" content="text/css">
  <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
  <link rel="stylesheet" type="text/css" href="../p_feedback.css">


<?php

/* 
 * Config:
 */
$logfile = "results/actions-feedback.txt";


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
             <div class=\"errorbox\">
              $message
             </div>
            </center>
            <p>Valid <a href=\"http://validator.w3.org/\">HTML 4.01 Transitional</a></p>
           </body>
          </html>\n");
   exit; 
}


/* 
 * Cannot start with step 3:
 */
if (!isset($referrer_url))
{
   error_abort("invalid", "When submitting your feedback please start with <a href=\"index.php\">step 1</a>.");
}


/* 
 * Cannot work on unknown problem:
 */
if (!isset($problem))
{
   error_abort("invalid", "You need to select the nature of the problem in <a href=\"index.php\">step 1</a>.");
}


/* 
 * Don't accept unconfirmed URLs
 */
if (!isset($url_confirmed))
{
   error_abort("invalid", "When submitting URLs that this script can't retrieve, you need to check \"Yes, I'm sure\"
                <a href=\"javascript:history.back();\">step 2</a>.");
}


/*
 * Handle optional text fields:
 */
if (!isset($name) || ($name == ""))
{
   $name = "anonymous";
}


/* 
 * Open the logfile or fail:
 */
$fp = fopen($logfile, "a");

if(!$fp)
{
   echo ("  <title>Internal Script Error</title>
           </head>
           <body>
            <div class=\"title\">
              <h1><a href=\"http://www.privoxy.org/\">Privoxy</a>: Internal Script Error</h1>
            </div>
            <center>
             <div class=\"errorbox\">
              <p>
               This script was unable to open its logfile.
              </p>
              <p>
               Please <a href=\"mailto:info@privoxy.org?SUBJECT=Feedback-Script-Broken\">mail its owner</a>!
              </p>
             </div>
            </center>
           </body>
          </html>");
   exit; 
}


/*
 * Write Head (type, severity, user, client-ip)
 * and remarks field:
 */
fwrite($fp, "\n#FEEDBACK TYPE $problem SEVERITY $severity FROM $name ON $REMOTE_ADDR VERIFIED $url_verified TIME " . date("r") ."\n");
if (isset($remarks))
{
   $lines = explode("\n", $remarks);
   foreach ($lines as $line)
   {
      fwrite($fp, "#REMARKS: $line\n");
   }
}


/*
 * Depending on the type of problem reported,
 * we need to write additional data:
 */
switch ($problem)
{
   /*
    * Banner not blocked:
    */
   case "P1":
      fwrite($fp, "#BLOCK-REFERRER: $referrer_url\n");
      if (isset($num_images))
      {
         for($i=0; $i < $num_images; $i++)
         {
             if (isset($block_image[$i]))
             {
                fwrite($fp, "#BLOCK-URL: $image_url[$i]\n");
             }
         }
      }
      if (isset($manual_image_url) && ($manual_image_url != ""))
      {
         fwrite($fp, "#BLOCK-URL: $manual_image_url\n");
      }
      break;

   /*
    * Innocent image blocked:
    */
   case "P2":
      fwrite($fp, "#UNBLOCK-REFERRER: $referrer_url\n");
      if (isset($image_url) && ($image_url != ""))
      {
         fwrite($fp, "#UNBLOCK-URL: $image_url\n");
      }
      break;

   /*
    * All other problems:
    */
   default:
      fwrite($fp, "#PROBLEM-URL: $referrer_url\n");
      break;
}        
            
fclose($fp);

?>

  <title>Privoxy Action List Feedback - Result</title>
 </head>

 <body>
  <div class="title">
   <h1>
    <a href="http://www.privoxy.org" target="_blank">Privoxy</a> Action List Feedback - Result
   </h1>
  </div>

  <div class="box">
   <p>
    <b>Thank you very much for taking the time to submit your feedback!</b>
   </p>

   <p>
    It will be reviewed by the developers and used to improve the
    distribution actions file.
   </p>
   
   <p align=center>
    <input type="submit" value="Close this window" onclick="window.close();">
   </p>

  </div>

  <p>Valid <a href="http://validator.w3.org/">HTML 4.01 Transitional</a></p>

 </body>
</html>
