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
    <link rel="stylesheet" type="text/css" href="../p_feedback.css">

<?php

/* 
 * For testing:
 */
//phpinfo();
//error_reporting(E_ALL);
error_reporting(E_NONE);



/* 
 * Cannot start with step 3:
 */
if (!isset($referrer_url))
{
   echo ("  <title>Invalid Feedback Submission</title>
	    <link rel="stylesheet" type="text/css" href="../p_feedback.css">
           </head>
           <body>
            <div class=\"title\">Invalid Feedback Submission</div>
            <div align=\"center\">
             <div class=\"errorbox\" align=\"left\">
              When submitting your feedback please start with <a href=\"index.php\">step 1</a>.
             </div>
            </div>
           </body>
          </html>");
   exit; 
}


/* 
 * Cannot work on unknown problem:
 */
if (!isset($problem))
{
   echo ("  <title>Invalid Feedback Submission</title>
	    <link rel="stylesheet" type="text/css" href="../p_feedback.css">
           </head>
           <body>
            <div class=\"title\">Invalid Feedback Submission</div>
            <div align=\"center\">
             <div class=\"errorbox\" align=\"left\">
              You need to select the nature of the problem in <a href=\"index.php\">step 1</a>.
             </div>
            </div>
           </body>
          </html>");
   exit; 
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
$logfile = "feedback-data.txt";
$fp = fopen($logfile, "a");

if(!$fp)
{
   echo ("  <title>Internal Script Error</title>
	    <link rel="stylesheet" type="text/css" href="../p_feedback.css">
           </head>
           <body>
            <div class=\"title\">Internal Script Error</div>
            <div align=\"center\">
             <div class=\"errorbox\" align=\"left\">
              <p>
               This script was unable to open its logfile.
              </p>
              <p>
               Please <a href=\"mailto:info@privoxy.org?SUBJECT=Feedback-Script-Broken\">mail its owner</a>!
              </p>
             </div>
            </div>
           </body>
          </html>");
   exit; 
}


/*
 * Write Head (type, severity, user, client-ip)
 * and remarks field:
 */
fwrite($fp, "\n#FEEDBACK TYPE $problem SEVERITY $severity FROM $name ON $REMOTE_ADDR\n");
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
  <link rel="stylesheet" type="text/css" href="../p_feedback.css">
 </head>

 <body>
  <div class="title">
   <a href="http://www.privoxy.org" target="_blank">Privoxy</a> Action List Feedback - Result
  </div>

  <div class="box">
   <p>
    <b>Thank you very much for taking the time to submit your feedback!</b>
   </p>

   <p>
    The developers will review and use your submission to improve the
    distribution actions file.
   </p>
   
   <p align=center>
    <input type="submit" value="Close this window" onclick="window.close();">
   </p>

  </div>
 </body>
</html>
