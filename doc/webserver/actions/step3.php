<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
 <!--

  File :  $Source: /cvsroot/ijbswa/current/doc/webserver/actions/step3.php,v $

  Purpose  :  Submit form for actions file feedback (step 1)
              This file belongs in
              ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/

  $Id: step3.php,v 1.11 2002/04/06 15:54:08 swa Exp $

  $Log: step3.php,v $
  Revision 1.11  2002/04/06 15:54:08  swa
  prework: list of what needs to
  be submitted to the tracker.

  Revision 1.10  2002/04/06 15:19:35  oes
  Clean-up, smarter handling of unreachable URLs

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

// // Debug short cut
// $referrer_url="http://informer2.comdirect.de/de/my/homepage/index.html?Show=main.html&callerPage=%2fde%2fmy%2fhomepage%2findex.html";
// $problem="P1";
// $url_confirmed="yes";
// $severity="1";
// $remarks="this is the first line of text\nthe 2nd line\nthird line.";
// //$block_image="";
// $num_images=0;
// $manual_image_url="http://www.someimage.de/de/my/homepage/index.html?Show=main.html&callerPage=%2fde%2fmy%2fhomepage%2findex.html";
// $image_url="http://www.someimage.de/de/my/homepage/index.html?Show=main.html&callerPage=%2fde%2fmy%2fhomepage%2findex.html";
// // end debug short cut

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

// now fill the tracker on sf with all the necessary values:
//
// <FORM ACTION="/tracker/index.php?group_id=11118&atid=460288" METHOD="POST" enctype="multipart/form-data">
// <INPUT TYPE="HIDDEN" NAME="func" VALUE="postadd">
//
// <SELECT NAME="category_id">
// <OPTION VALUE="100">None</OPTION>
// <OPTION VALUE="412811">filter: ad-incorrectly-blocked</OPTION>
// <OPTION VALUE="412810">filter: ad-not-blocked</OPTION>
// <OPTION VALUE="412814">filter: other problem</OPTION>
// <OPTION VALUE="412812">filter: page-incorrectly-block</OPTION>
// <OPTION VALUE="412813">filter: popups-incorrectly blo</OPTION>
//
//  <SELECT NAME="artifact_group_id">
//  <OPTION VALUE="100">None</OPTION>
//  <OPTION VALUE="195870">action file version 1.0</OPTION>
//  <OPTION VALUE="195871">action file version 1.1</OPTION>
//  <OPTION VALUE="195872">action file version 1.2</OPTION>
//  <OPTION VALUE="195873">action file version 1.3</OPTION>
//  <OPTION VALUE="195874">action file version 1.4</OPTION>
//  <OPTION VALUE="195875">action file version 1.5</OPTION>
//  <OPTION VALUE="195876">action file version 1.6</OPTION>
//  <OPTION VALUE="195877">action file version 1.7</OPTION>
//  <OPTION VALUE="195878">action file version 1.8</OPTION>
//  <OPTION VALUE="195879">action file version 1.9</OPTION>
//  <OPTION VALUE="195880">action file version 2.0</OPTION> 
//
// <SELECT NAME="assigned_to">
// <OPTION VALUE="100">None</OPTION>
// <OPTION VALUE="128588">bart1803</OPTION>
// <OPTION VALUE="249980">david__schmidt</OPTION>
// <OPTION VALUE="79346">gliptak</OPTION>
// <OPTION VALUE="322640">hal9</OPTION>
// <OPTION VALUE="108873">joergs</OPTION>
// <OPTION VALUE="199289">jongfoster</OPTION>
// <OPTION VALUE="10857">morcego</OPTION>
// <OPTION VALUE="78811">oes</OPTION>
// <OPTION VALUE="43129">roro</OPTION>
// <OPTION VALUE="249832">steudten</OPTION>
// <OPTION VALUE="74317">swa</OPTION>
//
// <SELECT NAME="priority">
// <OPTION VALUE="1">1 - Lowest</OPTION>
// <OPTION VALUE="2">2</OPTION>
// <OPTION VALUE="3">3</OPTION>
// <OPTION VALUE="4">4</OPTION>
// <OPTION VALUE="5" SELECTED>5 - Medium</OPTION>
// <OPTION VALUE="6">6</OPTION>
// <OPTION VALUE="7">7</OPTION>
// <OPTION VALUE="8">8</OPTION>
// <OPTION VALUE="9">9 - Highest</OPTION>
//
// <INPUT TYPE="TEXT" NAME="summary" SIZE="35" MAXLENGTH="40">
// 
// we can either put everything in here (nicely formatted) ..
// <TEXTAREA NAME="details" ROWS="30" COLS="55" WRAP="HARD"></TEXTAREA>
//
// ... or attach a file (with the html header so it gets displayed
// as if it is a html file
// <input type="checkbox" name="add_file" VALUE="1">
// <input type="file" name="input_file" size="30">
// <input type="text" name="file_description" size="40" maxlength="255">
//
// <INPUT TYPE="SUBMIT" NAME="SUBMIT" VALUE="SUBMIT">

//
// TODO: - set all variables above this point
//       - evaluate alternative: file attachment
//

// INFORMATION:
// below this point, all variables _must_ be set (no checking is performed)
//

//  need to switch the following statement for type of problem
switch($problem)
{
   case "P1": $sf_cat_id="412811"; $sf_summary="an advertisment was not blocked"; break;
   case "P2": $sf_cat_id="412810"; $sf_summary="an innocent image was blocked"; break;
   case "P3": $sf_cat_id="412812"; $sf_summary="the whole page was erraneously blocked"; break;
   case "P4": $sf_cat_id="412813"; $sf_summary="the page needs popups but they don't work"; break;
   case "P5": $sf_cat_id="412814"; $sf_summary="a problem occured"; break;
   default: $sf_cat_id="412814"; $sf_summary="AN UNPROCESSABLE PROBLEM OCCURED"; break;
}
// need to switch the following statement for action file version
$sf_agroup_id="195870"; // assume 1.0 for the time being
// this is always set to nobody
$sf_assigned_to="100";
// need to set values from [1...9]
switch($severity)
{
   case "1": $sf_prio="1"; break;  // low
   case "2": $sf_prio="5"; break;  // medium
   case "3": $sf_prio="9"; break;  // high
   default: $sf_cat_id="1"; break;
}
// here comes the beef
$sf_trackertext="\n\n\nIt would be awesome if the links below were clickable :-)\n\n";
$sf_trackertext=$sf_trackertext . "This item was submitted by $name\n\n";
$sf_trackertext=$sf_trackertext . "Where did it happen?\n$referrer_url\n\n";
$sf_trackertext=$sf_trackertext . "What happened?\n$sf_summary (" . date("r") . ")\n\n";
switch($problem)
{
   case "P1":
     $sf_trackertext=$sf_trackertext."These URLs are likely advertisements\n";
     for($i=0; $i < $num_images; $i++)
       {
	 if (isset($block_image[$i]))
	   {
	     $sf_trackertext = $sf_trackertext . "$image_url[$i]\n\n";
	   }
       }
     if ($manual_image_url != "")
       {
	 $sf_trackertext=$sf_trackertext . "$manual_image_url";
       }
     $sf_trackertext= $sf_trackertext . "\n\n";
     break;
   case "P2": $sf_trackertext= $sf_trackertext . "The URL of the image that was incorrectly blocked:\n$image_url\n\n"; break;
   case "P3": $sf_trackertext= $sf_trackertext . "\n\n"; break;
   case "P4": $sf_trackertext= $sf_trackertext . "\n\n"; break;
   case "P5": $sf_trackertext= $sf_trackertext . "\n\n"; break;
   default: $sf_trackertext= $sf_trackertext . "\n\n"; break; break;
}
$sf_trackertext= $sf_trackertext . "Remarks:\n--------\n$remarks\n\n\n";

?>

<div class="title">
<h1>
<a href="http://www.privoxy.org" target="_blank">Privoxy</a> Action List Feedback - Step 3 of 4.
</h1>
</div>
</head>
<body>
<div class="box">
<p>
Thank you. We have preliminary validated you input. Please click on "Submit"
to transfer you feedback to our tracker.
</p>
<p align=center>
<form action="http://sourceforge.net/tracker/index.php?group_id=11118&atid=460288" method="post" enctype="multipart/form-data">
<input type="hidden" name="func" value="postadd">
<input type="hidden" name="category_id" value="<?php echo ($sf_cat_id) ?>">
<input type="hidden" name="artifact_group_id" value="<?php echo ($sf_agroup_id) ?>">
<input type="hidden" name="assigned_to" value="<?php echo ($sf_assigned_to) ?>">
<input type="hidden" name="priority" value="<?php echo ($sf_prio) ?>">
<input type="hidden" name="summary" value="<?php echo ($sf_summary) ?>">
<input type="hidden" name="details" value="<?php echo ($sf_trackertext) ?>">
<input type="submit" name="submit" value="Submit">
</form>
The script will take you to Sourceforge where all submissions are collected.
</p>
</div>
<p>Valid <a href="http://validator.w3.org/">HTML 4.01 Transitional</a></p>
</body>
</html>

<!--

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

 -->