#ifndef JBSOCKETS_H_INCLUDED
#define JBSOCKETS_H_INCLUDED
#define JBSOCKETS_H_VERSION "$Id: jbsockets.h,v 1.2 2001/06/07 23:06:09 jongfoster Exp $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/jbsockets.h,v $
 *
 * Purpose     :  Contains wrappers for system-specific sockets code,
 *                so that the rest of JunkBuster can be more
 *                OS-independent.  Contains #ifdefs to make this work
 *                on many platforms.
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
 *    $Log: jbsockets.h,v $
 *    Revision 1.2  2001/06/07 23:06:09  jongfoster
 *    The host parameter to connect_to() is now const.
 *
 *    Revision 1.1.1.1  2001/05/15 13:58:54  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

struct client_state;

extern int connect_to(const char *host, int portnum, struct client_state *csp);
extern int write_socket(int fd, const char *buf, int n);
extern int read_socket(int fd, char *buf, int n);
extern void close_socket(int fd);

extern int bind_port(const char *hostnam, int portnum);
extern int accept_connection(struct client_state * csp, int fd);

extern int resolve_hostname_to_ip(const char *host);

/* Revision control strings from this header and associated .c file */
extern const char jbsockets_rcs[];
extern const char jbsockets_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef JBSOCKETS_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/
