#ifndef _FILTERS_H
#define _FILTERS_H
#define FILTERS_H_VERSION "$Id: filters.h,v 1.1 2001/05/13 21:57:06 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/filters.h,v $
 *
 * Purpose     :  Declares functions to parse/crunch headers and pages.
 *                Functions declared include:
 *                   `acl_addr', `add_stats', `block_acl', `block_imageurl',
 *                   `block_url', `cookie_url', `domaincmp', `dsplit',
 *                   `filter_popups', `forward_url'
 *                   `ij_untrusted_url', `intercept_url', `re_process_buffer',
 *                   `show_proxy_args', and `trust_url'
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
 *    $Log: filters.h,v $
 *
 *********************************************************************/


#include "project.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ACL_FILES
extern int block_acl(struct access_control_addr *src, struct access_control_addr *dst, struct client_state *csp);
extern int acl_addr(char *aspec, struct access_control_addr *aca);
#endif /* def ACL_FILES */

extern char *block_url(struct http_request *http, struct client_state *csp);
#ifdef TRUST_FILES
extern char *trust_url(struct http_request *http, struct client_state *csp);
#endif /* def TRUST_FILES */
extern char *intercept_url(struct http_request *http, struct client_state *csp);

#if defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST)
extern int block_imageurl(struct http_request *http, struct client_state *csp);
#endif /* defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST) */

#ifdef USE_IMAGE_LIST
extern int block_imageurl_using_imagelist(struct http_request *http, struct client_state *csp);
#endif /* def USE_IMAGE_LIST */

extern struct cookie_spec *cookie_url(struct http_request *http, struct client_state *csp);
extern const struct gateway *forward_url(struct http_request *http, struct client_state *csp);

extern struct url_spec dsplit(char *domain);
extern int domaincmp(struct url_spec *pattern, struct url_spec *fqdn);

extern char *show_proxy_args(struct http_request *http, struct client_state *csp);

#ifdef TRUST_FILES
extern char *ij_untrusted_url(struct http_request *http, struct client_state *csp);
#endif /* def TRUST_FILES */

#ifdef STATISTICS
extern char *add_stats(char *s);
#endif /* def STATISTICS */

#ifdef PCRS
extern void re_process_buffer(struct client_state *csp);
#endif /* def PCRS */

/* Revision control strings from this header and associated .c file */
extern const char filters_rcs[];
extern const char filters_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _FILTERS_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
