#ifndef _SHOWARGS_H
#define _SHOWARGS_H
#define SHOWARGS_H_VERSION "$Id: showargs.h,v 1.1 2001/05/13 21:57:07 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/showargs.h,v $
 *
 * Purpose     :  Contains various utility routines needed to 
 *                generate the show-proxy-args page.
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
 *    $Log: showargs.h,v $
 *
 *********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

extern char *strsav(char *old, const char *text_to_append);
extern void savearg(char *c, char *o);

extern void init_proxy_args(int argc, const char *argv[]);
extern void end_proxy_args(void);

/* Revision control strings from this header and associated .c file */
extern const char showargs_rcs[];
extern const char showargs_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _SHOWARGS_H */
