#ifndef _CONFIG_H
#define _CONFIG_H
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/acconfig.h,v $
 *
 * Purpose     :  This file should be the first thing included in every
 *                .c file.  (Before even system headers).  It contains 
 *                #define statements for various features.  It was
 *                introduced because the compile command line started
 *                getting ludicrously long with feature defines.
 *
 * Copyright   :  Written by and Copyright (C) 2001 the SourceForge
 *                IJBSWA team.  http://ijbswa.sourceforge.net
 *
 *                Based on the Internet Junkbuster originally written
 *                by and Copyright (C) 1997 Anonymous Coders and 
 *                Junkbusters Corporation.  http://www.junkbusters.com
 *
 *                This program is free software; you can redistribute it 
 *                and/or modify it under the terms of the GNU General
 *                Public License as published by the Free Software
 *                Foundation; either version 2 of the License, or (at
 *                your option) any later version.
 *
 *                This program is distributed in the hope that it will
 *                be useful, but WITHOUT ANY WARRANTY; without even the
 *                implied warranty of MERCHANTABILITY or FITNESS FOR A
 *                PARTICULAR PURPOSE.  See the GNU General Public
 *                License for more details.
 *
 *                The GNU General Public License should be included with
 *                this file.  If not, you can view it at
 *                http://www.gnu.org/copyleft/gpl.html
 *                or write to the Free Software Foundation, Inc., 59
 *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Revisions   :
 *    $Log: acconfig.h,v $
 *
 *********************************************************************/

@TOP@

/*
 * Version number - Major (X._._)
 */
#undef VERSION_MAJOR

/*
 * Version number - Minor (_.X._)
 */
#undef VERSION_MINOR

/*
 * Version number - Point (_._.X)
 */
#undef VERSION_POINT

/*
 * Version number, as a string
 */
#undef VERSION

/*
 * Regular expression matching for URLs.  (Highly recommended).  If this is 
 * not defined then you can ony use prefix matching.
 */
#undef REGEX

/*
 * Allow JunkBuster to be "disabled" so it is just a normal non-blocking
 * non-anonymizing proxy.  This is useful if you're trying to access a
 * blocked or broken site - just change the setting in the config file
 * and send a SIGHUP (UN*X), or use the handy "Disable" menu option (Windows
 * GUI).
 */
#undef TOGGLE

/*
 * Enables arbitrary content modification regexps
 */
#undef PCRS

/*
 * If a stream is compressed via gzip (Netscape specific I think), then
 * it cannot be modified with Perl regexps.  This forces it to be 
 * uncompressed.
 */
#undef DENY_GZIP

/*
 * Enables statistics function.
 */
#undef STATISTICS

/*
 * Bypass filtering for 1 page only
 */
#undef FORCE_LOAD

/*
 * Split the show-proxy-args page into a page for each config file.
 */
#undef SPLIT_PROXY_ARGS

/*
 * Kills JavaScript popups - window.open, onunload, etc.
 */
#undef KILLPOPUPS

/*
 * Support for webDAV - e.g. so Microsoft Outlook can access HotMail e-mail
 */
#undef WEBDAV

/*
 * Detect image requests automatically for MSIE.  Will fall back to
 * other image-detection methods (i.e. USE_IMAGE_LIST) for other
 * browsers.
 *
 * It detects the following header pair as an image request:
 *
 * User-Agent: Mozilla/4.0 (compatible; MSIE 5.5; Windows NT 5.0)
 * Accept: * / *
 *
 * And the following as a HTML request:
 *
 * User-Agent: Mozilla/4.0 (compatible; MSIE 5.5; Windows NT 5.0)
 * Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, * / *
 *
 * And no, I haven't got that backwards - IE is being wierd.
 *
 * Known limitations: 
 * 1) If you press shift-reload on a blocked HTML page, you get
 *    the image "blocked" page, not the HTML "blocked" page.
 * 2) Once an image "blocked" page has been sent, viewing it 
 *    in it's own browser window *should* bring up the HTML
 *    "blocked" page, but it doesn't.  You need to clear the 
 *    browser cache to get the HTML version again.
 *
 * These limitations are due to IE making inconsistent choices
 * about which "Accept:" header to send.
 */
#undef DETECT_MSIE_IMAGES

/*
 * Use image list to detect images.
 * If you do not define this then everything is treated as HTML.
 *
 * Whatever the setting of this value, DETECT_MSIE_IMAGES will 
 * override it for people using Internet Explorer.
 */
#undef USE_IMAGE_LIST

/*
 * Allows the use of ACL files to control access to the proxy by IP address.
 */
#undef ACL_FILES

/*
 * Allows the use of trust files.
 */
#undef TRUST_FILES

/*
 * Allows the use of jar files to capture cookies.
 */
#undef JAR_FILES

/*
 * Use PCRE rather than GNU Regex
 */
#undef PCRE

@BOTTOM@

#endif /* _CONFIG_H */
