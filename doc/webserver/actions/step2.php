<?php

error_reporting(E_NONE);
#error_reporting(E_ALL);


//  File :  $Source: /cvsroot/ijbswa/current/doc/webserver/actions/step2.php,v $
//
//  Purpose  :  Submit form for actions file feedback (step 2)
//              This file belongs in
//              ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/
//
//  $Id: step2.php,v 1.2 2002/03/30 19:49:34 swa Exp $
//
//  $Log: step2.php,v $
//  Revision 1.2  2002/03/30 19:49:34  swa
//  have consistent look and feel
//
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
 * For testing: 
 */
#$base_url = "http://www.oesterhelt.org/actions";
$base_url = "http://www.privoxy.org/actions";
#$base_url = "http://localhost/actions";

/* 
 * Cannot start with step 2:
 */
if (!isset($referrer_url))
{
     echo ("<html><head><title>Invalid Feedback Submission</title>
	    <link rel=\"stylesheet\" type=\"text/css\" href=\"../p_web.css\">
		</head>
                <body><h2>Invalid Feedback Submission</h2>
                      <p>When submitting your feedback please start with
                         <a href=\"index.php\">step 1</a>.</p>
                 </body>
          </html>");
   exit; 
}

/* 
 * Cannot work on unknown problem:
 */
if (!isset($problem) || $problem == "INVALID")
{
     echo ("<html><head><title>Invalid Feedback Submission</title>
	    <link rel=\"stylesheet\" type=\"text/css\" href=\"../p_web.css\">
		</head>
                <body><h2>Invalid Feedback Submission</h2>
                      <p>You need to select the nature of the problem in
                         <a href=\"javascript:back()\">step 1</a>.</p>
                 </body>
          </html>");
   exit; 
}


/*
 * Check if URL really exists and buffer its contents:
 */

$ch = curl_init ($referrer_url);
curl_setopt ($ch, CURLOPT_HEADER, 0);
curl_setopt ($ch, CURLOPT_FAILONERROR, 1);

ob_start();
$success = curl_exec ($ch);
$page = ob_get_contents();
ob_end_clean();

curl_close ($ch);

if (!$success)
{
   echo ("<html><head><title>Invalid Feedback Submission</title>
	    <link rel=\"stylesheet\" type=\"text/css\" href=\"../p_web.css\">
		</head>
                <body><h2>Invalid Feedback Submission</h2>
                      <p>The URL that you entered (<a href=\"$referrer_url\">$referrer_url</a>)
                         <br>could not be retrieved.</p>
                         <p>Make sure the URL is correct and publicly accessible.</p>
                         <p><a href=\"javascript:back()\">Back to step 1</a></p>
                         
                 </body>
          </html>");
   exit; 
}

/* 
 * Create description from problem code:
 */
switch($problem)
{
   case "P1": $problem_description="an advertisment was not blocked"; break;
   case "P2": $problem_description="an innocent image was blocked"; break;
   case "P3": $problem_description="the whole page was erraneously blocked"; break;
   case "P4": $problem_description="the page needs popups but they don't work"; break;
   case "P5": $problem_description="a problem occured"; break;
   default: $problem_description="AN UNPROCESSABLE PROBLEM OCCURED";
}

?>

<html>
  <head>
    <title>Privoxy Action List Feedback - Step 2 of 2</title>
    <link rel="stylesheet" type="text/css" href="../p_web.css">
</head>

<h2><a href="http://www.privoxy.org" target="_blank">Privoxy</a> Action List Feedback - Step 2 of 2</h2>
<p>
You are about to report that <?php echo ($problem_description) ?>
<br>on <a href="<?php echo ($referrer_url) ?>"><?php echo ($referrer_url) ?></a>.
</p>


<p>
<form action="<?php echo($base_url); ?>/step3.php" method="post">

<input type="hidden" name="problem" value="<?php echo ($problem) ?>">
<input type="hidden" name="referrer_url" value="<?php echo ($referrer_url) ?>">

<dl>

<?php

if ($problem != "P1")
{
   echo ("<!--");
}
else
{
   preg_match_all('|<img\s+[^>]*?src=[\'"]?(.*?)[\'" >]|i', $page, $matches);
   $count = count($matches[0]);
   if ($count > 0)
   {
      $referrer_base = substr($referrer_url, 0, -strpos(strrev($referrer_url), '/'));
      $referrer_host = "http://".strrev(strrchr(strrev(substr($referrer_url, 7)), "/"));

      echo ("<dt><b>Choose the images to be blocked from the following list:</b></dt><dd><p>\n");
      echo ("<input type=\"hidden\" name=\"num_images\" value=\"$count\">\n");
      echo ("<table border=\"0\" cellpadding=\"0\" cellspacing=\"4\">\n");
      for ($i=0; $i< $count; $i++) {
         $image_url = $matches[1][$i];
         if (strncmp("http://", $image_url, 7))
         {
            if ($image_url{0} == "/")
            {
               $image_url = $referrer_host.$image_url;
            }
            else
            {
               $image_url = $referrer_base.$image_url;
            }
         }
         echo ("<tr><td rowspan=2><input type=\"checkbox\" name=\"block_image[$i]\" value=\"off\"></td>\n");
         echo ("<td><a href=\"$image_url\">$image_url</a>:</td>\n");
         echo ("<td><input type=\"hidden\" name=\"image_url[$i]\" value=\"$image_url\"></td></tr>\n");
         echo ("<tr><td><img style=\"max-width: 300; max-height: 50; min-width: 20; min-height: 20\" src=\"$image_url\"></td></tr>\n");
      }
      echo ("</table></p></dd><dt><b>If the banner that you saw is not listed above, enter the URL here</b>\n");
   }
   else
   {
      echo ("<dt><b>URL of the advertisment image:</b>\n");
   }
}

?>

<br><i>Hint: right-click the image, select "Copy image location" and paste the URL here.</i></dt>
<dd>
<p><input name="manual_image_url" type="text" size="45" maxlength="255"></p>
</dd>
<?php if($problem != "P1") echo ("-->") ?>

<?php if($problem != "P2") echo ("<!--") ?>
<dt><b>URL of the innocent image:</b>
<br><i>Hint: right-click the image, select "Copy image location" and paste the URL here.
<br>This may not work if the image was blocked by size or if +image-blocker is set to redirect.</i></dt>
<dd>
<p><input name="image_url" value="unknown" type="text" size="45" maxlength="255"></p>
</dd>
<?php if($problem != "P2") echo ("-->") ?>


<dt><b>Severity:</b></dt>
<dd>
<p>
<select name="severity">
<option value="3">drives me crazy</option>
<option selected value="2">vanilla banner</option>
<option value="1">cosmetic</option>
</select>
</p>
</dd>

<dt><b>Remarks:</b> <i>(optional)</i></dt>
<dd>
<p><textarea name="remarks" cols="35" rows="3">None.</textarea></p>
</dd>

<dt><b>Your Name:</b> <i>(optional)</i></dt>
<dd>
<p><input name="name" size="45"></p>
</dd>

<dt>&nbsp;</dt>
<dd>
<input type=submit value="Submit">
</dd>

</dl>
</form>
</p>

</body>
</html>
