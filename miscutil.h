#ifndef _MISCUTIL_H
#define _MISCUTIL_H
#define MISCUTIL_H_VERSION "$Id: miscutil.h,v 1.1.1.1 2001/05/15 13:59:00 oes Exp $"
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
 *    Revision 1.1.1.1  2001/05/15 13:59:00  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#if defined(__cplusplus)
extern "C" {
#endif

extern void *zalloc(int size);

extern unsigned long hash_string(const char* s);

extern char *safe_strerror(int err);

extern int strcmpic(const char *s1, const char *s2);
extern int strncmpic(const char *s1, const char *s2, size_t n);

#ifdef __MINGW32__
extern char *strdup(const char *s);
#endif /* def __MINGW32__ */

/* Revision control strings from this header and associated .c file */
extern const char miscutil_rcs[];
extern const char miscutil_h_rcs[];

#if defined(__cplusplus)
}
#endif

#endif /* ndef _MISCUTIL_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
