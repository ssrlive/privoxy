#ifndef _SSPLIT_H
#define _SSPLIT_H
#define SSPLIT_H_VERSION "$Id: ssplit.h,v 1.1 2001/05/13 21:57:07 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/ssplit.h,v $
 *
 * Purpose     :  A function to split a string at specified deliminters.
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
 *    $Log: ssplit.h,v $
 *
 *********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

extern int ssplit(char *s, char *c, char *v[], int n, int m, int l);

/* Revision control strings from this header and associated .c file */
extern const char ssplit_rcs[];
extern const char ssplit_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _SSPLIT_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
