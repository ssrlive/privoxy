#ifndef _JCC_H
#define _JCC_H
#define JCC_H_VERSION "$Id: jcc.h,v 1.1 2001/05/13 21:57:06 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/jcc.h,v $
 *
 * Purpose     :  Main file.  Contains main() method, main loop, and 
 *                the main connection-handling function.
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
 *    $Log: jcc.h,v $
 *
 *********************************************************************/


/* Declare struct FILE for vars and funcs. */
#include <stdio.h>

/* All of our project's data types. */
#include "project.h"

#include "loadcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define freez(X)  if(X) free(X); X = NULL


/* Global variables */


#ifdef STATISTICS
extern int urls_read;
extern int urls_rejected;
#endif /*def STATISTICS*/

extern struct client_state clients[];

extern struct file_list    files[];

/* Global constants */

extern const char DEFAULT_USER_AGENT[];


/* Functions */

#ifdef __MINGW32__
int _main(int argc, const char *argv[]);
#else
int main(int argc, const char *argv[]);
#endif

/* Revision control strings from this header and associated .c file */
extern const char jcc_rcs[];
extern const char jcc_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _JCC_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
