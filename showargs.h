#ifndef SHOWARGS_H_INCLUDED
#define SHOWARGS_H_INCLUDED
#define SHOWARGS_H_VERSION "$Id: showargs.h,v 1.5 2001/06/29 13:35:41 oes Exp $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/showargs.h,v $
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
 *    Revision 1.5  2001/06/29 13:35:41  oes
 *    adapted to reflect the changes in showargs.c
 *
 *    Revision 1.4  2001/06/03 19:13:05  oes
 *    moved stuff to cgi.c
 *
 *    Revision 1.3  2001/05/29 23:11:38  oes
 *
 *     - Moved strsav() from showargs to miscutil
 *
 *    Revision 1.2  2001/05/26 00:28:36  jongfoster
 *    Automatic reloading of config file.
 *    Removed obsolete SIGHUP support (Unix) and Reload menu option (Win32).
 *    Most of the global variables have been moved to a new
 *    struct configuration_spec, accessed through csp->config->globalname
 *    Most of the globals remaining are used by the Win32 GUI.
 *
 *    Revision 1.1.1.1  2001/05/15 13:59:03  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

extern void savearg(char *c, char *o, struct configuration_spec * config);
extern char *show_rcs(void);
extern char *show_defines(void);

/* Revision control strings from this header and associated .c file */
extern const char showargs_rcs[];
extern const char showargs_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef SHOWARGS_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/

