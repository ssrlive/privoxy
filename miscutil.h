#ifndef MISCUTIL_H_INCLUDED
#define MISCUTIL_H_INCLUDED
#define MISCUTIL_H_VERSION "$Id: miscutil.h,v 1.10 2001/09/20 13:34:09 steudten Exp $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/miscutil.h,v $
 *
 * Purpose     :  zalloc, hash_string, safe_strerror, strcmpic,
 *                strncmpic, and MinGW32 strdup functions.  These are
 *                each too small to deserve their own file but don't 
 *                really fit in any other file.
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
 *    $Log: miscutil.h,v $
 *    Revision 1.10  2001/09/20 13:34:09  steudten
 *
 *    change long to int for prototype hash_string()
 *
 *    Revision 1.9  2001/07/29 18:43:08  jongfoster
 *    Changing #ifdef _FILENAME_H to FILENAME_H_INCLUDED, to conform to
 *    ANSI C rules.
 *
 *    Revision 1.8  2001/06/29 13:32:14  oes
 *    Removed logentry from cancelled commit
 *
 *    Revision 1.7  2001/06/05 22:32:01  jongfoster
 *    New function make_path() to splice directory and file names together.
 *
 *    Revision 1.6  2001/06/03 19:12:30  oes
 *    introduced bindup()
 *
 *    Revision 1.5  2001/06/01 10:31:51  oes
 *    Added character class matching to trivimatch; renamed to simplematch
 *
 *    Revision 1.4  2001/05/31 17:32:31  oes
 *
 *     - Enhanced domain part globbing with infix and prefix asterisk
 *       matching and optional unanchored operation
 *
 *    Revision 1.3  2001/05/29 23:10:09  oes
 *
 *
 *     - Introduced chomp()
 *     - Moved strsav() from showargs to miscutil
 *
 *    Revision 1.2  2001/05/29 09:50:24  jongfoster
 *    Unified blocklist/imagelist/permissionslist.
 *    File format is still under discussion, but the internal changes
 *    are (mostly) done.
 *
 *    Also modified interceptor behaviour:
 *    - We now intercept all URLs beginning with one of the following
 *      prefixes (and *only* these prefixes):
 *        * http://i.j.b/
 *        * http://ijbswa.sf.net/config/
 *        * http://ijbswa.sourceforge.net/config/
 *    - New interceptors "home page" - go to http://i.j.b/ to see it.
 *    - Internal changes so that intercepted and fast redirect pages
 *      are not replaced with an image.
 *    - Interceptors now have the option to send a binary page direct
 *      to the client. (i.e. ijb-send-banner uses this)
 *    - Implemented show-url-info interceptor.  (Which is why I needed
 *      the above interceptors changes - a typical URL is
 *      "http://i.j.b/show-url-info?url=www.somesite.com/banner.gif".
 *      The previous mechanism would not have intercepted that, and
 *      if it had been intercepted then it then it would have replaced
 *      it with an image.)
 *
 *    Revision 1.1.1.1  2001/05/15 13:59:00  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#if defined(__cplusplus)
extern "C" {
#endif

extern void *zalloc(int size);

extern unsigned int hash_string(const char* s);

extern char *safe_strerror(int err);

extern int strcmpic(const char *s1, const char *s2);
extern int strncmpic(const char *s1, const char *s2, size_t n);

extern char *strsav(char *old, const char *text_to_append);
extern int string_append(char **target_string, const char *text_to_append);

extern char *chomp(char *string);
extern int simplematch(char *pattern, char *text);

extern char *bindup(const char *string, int n);

extern char *make_path(const char * dir, const char * file);

#ifdef __MINGW32__
extern char *strdup(const char *s);
#endif /* def __MINGW32__ */

/* Revision control strings from this header and associated .c file */
extern const char miscutil_rcs[];
extern const char miscutil_h_rcs[];

#if defined(__cplusplus)
}
#endif

#endif /* ndef MISCUTIL_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/
