#ifndef CGIEDIT_H_INCLUDED
#define CGIEDIT_H_INCLUDED
#define CGIEDIT_H_VERSION "$Id: cgiedit.h,v 1.1 2001/09/16 15:47:37 jongfoster Exp $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/cgiedit.h,v $
 *
 * Purpose     :  CGI-based actionsfile editor.
 *                
 *                Functions declared include:
 * 
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
 *    $Log: cgiedit.h,v $
 *    Revision 1.1  2001/09/16 15:47:37  jongfoster
 *    First version of CGI-based edit interface.  This is very much a
 *    work-in-progress, and you can't actually use it to edit anything
 *    yet.  You must #define FEATURE_CGI_EDIT_ACTIONS for these changes
 *    to have any effect.
 *
 *
 **********************************************************************/


#include "project.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CGI functions
 */
#ifdef FEATURE_CGI_EDIT_ACTIONS
extern int cgi_edit_actions        (struct client_state *csp,
                                    struct http_response *rsp,
                                    struct map *parameters);
extern int cgi_edit_actions_for_url(struct client_state *csp,
                                    struct http_response *rsp,
                                    struct map *parameters);
extern int cgi_edit_actions_list   (struct client_state *csp,
                                    struct http_response *rsp,
                                    struct map *parameters);
extern int cgi_edit_actions_submit (struct client_state *csp,
                                    struct http_response *rsp,
                                    struct map *parameters);
#endif /* def FEATURE_CGI_EDIT_ACTIONS */


/* Revision control strings from this header and associated .c file */
extern const char cgiedit_rcs[];
extern const char cgiedit_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef CGI_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/
