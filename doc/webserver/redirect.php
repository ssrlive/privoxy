M<?php

error_reporting(E_ALL);

// File        :  $Source: /cvsroot/ijbswa/current/project.h,v $
//
// Purpose     :  redirects requests to a specific paragraph in the online docs
//                This file belongs into
//                ijbswa.sourceforge.net:/home/groups/i/ij/ijbswa/htdocs/
//                
// $Id: Makefile.in,v 1.12 2001/06/12 17:15:56 swa Exp $
//
// Written by and Copyright (C) 2001 the SourceForge
// IJBSWA team.  http://ijbswa.sourceforge.net
//
// Based on the Internet Junkbuster originally written
// by and Copyright (C) 1997 Anonymous Coders and 
// Junkbusters Corporation.  http://www.junkbusters.com


// Parse the v= and to= paramaters
function parse_parameters()
{
   global $v, $to;
   global $version_major, $version_minor, $version_point;

   $version_major = 0;
   $version_minor = 0;
   $version_point = 0;

   if (isset($v))
   {
      // Version specified

      $v = trim($v);

      // Check if it's valid.
      // Valid versions have the form "n.n.n", where n=digit(s).
      if ( (strspn($v,"0123456789.") == strlen($v)) )
      {
         // Probably valid.  Copy into globals.
         $version_pieces = explode (".", $v, 4);
         if (isset($version_pieces[0]))
         {
            $version_major = 0 + $version_pieces[0];
         }
         if (isset($version_pieces[1]))
         {
            $version_minor = 0 + $version_pieces[1];
         }
         if (isset($version_pieces[2]))
         {
            $version_point = 0 + $version_pieces[2];
         }
      }
   }

   if (isset($to))
   {
      // Trim whitespace and convert to lowercase.
      $to = strtolower(trim($to));

      // Restrict the characters in the string by removing everything
      // from the first disallowed character onwards.
      //
      // Allowed characters are 0-9, a-z, ".", "_", "-".
      //
      $to = substr($to, 0, strspn($to, "0123456789abcdefghijklmnopqrstuvwxyz._-"));
   }
   else
   {
      $to = "";
   }
}

parse_parameters();

// For debugging:
// print "Version {$version_major}.{$version_minor}.{$version_point}<br>";
// print "Target \"{$to}\"<br>";


// Please do NOT delete any of these redirects.  Even if you take them
// out of JunkBuster, they may be in use by older releases.

// Note 2: Should *not* include #target part in these URLs.
// (It works with MS IE, but is not valid HTTP.)

switch($to)
{
   case "faq":
      // Used by 2.9.0+
      header ("Location: http://www.junkbusters.com/ht/en/ijb2faq.html");
      exit;
   case "option":
      // Used by 2.9.0+
      // Config file options
      // called as redirect.php?v=X.X.X&to=option#optionname
      header ("Location: http://www.junkbusters.com/ht/en/ijb2man.html");
      exit;
   case "win":
      // Used by 2.9.0+ on WIN32
      header ("Location: http://www.junkbusters.com/ht/en/ijbwin.html");
      exit;
//   case "home":
//      // Currently hard-wired into the code.
//      header ("Location: http://ijbswa.sourceforge.net/");
//      exit;
//   case "gpl":
//      // Currently hard-wired into the code.
//      header ("Location: http://www.fsf.org/copyleft/gpl.html");
//      exit;
   default:
      header ("Location: http://ijbswa.sourceforge.net/");
      exit;
}

exit;
?>

// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General
// Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  See the GNU General Public
// License for more details.
//
// The GNU General Public License should be included with
// this file.  If not, you can view it at
// http://www.gnu.org/copyleft/gpl.html
// or write to the Free Software Foundation, Inc., 59
// Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//
// $Log: Makefile.in,v $
//
