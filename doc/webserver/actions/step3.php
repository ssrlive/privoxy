<?php

error_reporting(E_NONE);
#error_reporting(E_ALL);

//  File :  $Source: $
//
//  Purpose  :  Submit form for actions file feedback (step 1)
//              This file belongs in
//              ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/
//
//  $Id: $
//
//  $Log: $
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
 * Cannot start with step 3:
 */
if (!isset($referrer_url))
{
     echo ("<html><head><title>Invalid Feedback Submission</title></head>
                <body><h2>Invalid Feedback Submission</h2>
                      <p>When submitting your feedback please start with
                         <a href=\"test.php\">step 1</a>.</p>
                 </body>
          </html>");
   exit; 
}

/* 
 * Cannot work on unknown problem:
 */
if (!isset($problem))
{
     echo ("<html><head><title>Invalid Feedback Submission</title></head>
                <body><h2>Invalid Feedback Submission</h2>
                      <p>You need to select the nature of the problem in
                         <a href=\"test.php\">step 1</a>.</p>
                 </body>
          </html>");
   exit; 
}

/*
 * Handle optional text fields:
 */
if (!isset($name))
{
   $name = "anonymous";
}

/* 
 * Open the logfile:
 */
$logfile = "feedback-data.txt";
$fp = fopen($logfile, "a");

if(!$fp)
{
   echo ("<html><head><title>Internal Script Error</title></head>
                <body><h2>Internal Script Error</h2>
                      <p>This script was unable to open its logfile.</p>
                      <p>Please <a href=\"mailto:info@privoxy.org?SUBJECT=Feedback-Script-Broken\">mail its owner</a>!</p>
                 </body>
          </html>");
   exit; 
}

/*
 * Write Head and remarks field:
 */
fwrite($fp, "#FEEDBACK TYPE $problem SEVERITY $severity FROM $name ON $REMOTE_ADDR\n");
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

   case "P2":
      fwrite($fp, "#UNBLOCK-REFERRER: $referrer_url\n");
      if (isset($manual_image_url) && ($manual_image_url != ""))
      {
         fwrite($fp, "#UNBLOCK-URL: image_url\n");
      }
      break;


}        
            
fclose($fp);

?>

<html>
  <head>
    <title>Privoxy Action List Feedback - Result</title>
    <link rel="stylesheet" type="text/css" href="../p_web.css">
</head>

<h2><a href="http://www.privoxy.org" target="_blank">Privoxy</a> Action List Feedback - Result</h2>

<p><b>Thank you very much for taking the time to submit your feedback!</b></p>

<p>It will be reviewed by the developers and used to improve the
distribution actions file.</p>

<input type="submit" value="Close this window" onClick="window.close()">

</body>
</html>
