#ifndef _ENCODE_H
#define _ENCODE_H
#define ENCODE_H_VERSION "$Id: encode.h,v 1.1 2001/05/13 21:57:06 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/encode.h,v $
 *
 * Purpose     :  Functions to encode and decode URLs, and also to
 *                encode cookies and HTML text.
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
 *    $Log: encode.h,v $
 *
 *********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

extern char * html_encode(const char *s);
extern char * cookie_encode(const char *s);
extern char * url_encode(const char *s);
extern char * url_decode(const char *str);

/* Revision control strings from this header and associated .c file */
extern const char encode_rcs[];
extern const char encode_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _ENCODE_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
