#ifndef _LOADERS_H
#define _LOADERS_H
#define LOADERS_H_VERSION "$Id: loaders.h,v 1.1 2001/05/13 21:57:06 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/loaders.h,v $
 *
 * Purpose     :  Functions to load and unload the various
 *                configuration files.  Also contains code to manage
 *                the list of active loaders, and to automatically 
 *                unload files that are no longer in use.
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
 *    $Log: loaders.h,v $
 *
 *********************************************************************/


#include "project.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void sweep(void);
extern char *read_config_line(char *buf, int buflen, FILE *fp, struct file_list *fs);

extern int load_blockfile(struct client_state *csp);
extern int load_cookiefile(struct client_state *csp);
extern int load_forwardfile(struct client_state *csp);
  
#ifdef ACL_FILES
extern int load_aclfile(struct client_state *csp);
#endif /* def ACL_FILES */

#ifdef USE_IMAGE_LIST
extern int load_imagefile(struct client_state *csp);
#endif /* def USE_IMAGE_LIST */
 
#ifdef KILLPOPUPS
extern int load_popupfile(struct client_state *csp);
#endif /* def KILLPOPUPS */

#ifdef TRUST_FILES
extern int load_trustfile(struct client_state *csp);
#endif /* def TRUST_FILES */

#ifdef PCRS
extern int load_re_filterfile(struct client_state *csp);
#endif /* def PCRS */

extern void add_loader(int (*loader)(struct client_state *));
extern int run_loader(struct client_state *csp);
extern void remove_all_loaders(void);

#ifdef PCRS
extern int load_re_filterfile(struct client_state *csp);
#endif /* def PCRS */

#ifdef KILLPOPUPS
extern int load_popupfile(struct client_state *csp);
#endif /* def KILLPOPUPS */

/* Revision control strings from this header and associated .c file */
extern const char loaders_rcs[];
extern const char loaders_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _LOADERS_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
