#ifndef CGI_H_INCLUDED
#define CGI_H_INCLUDED
#define CGI_H_VERSION "$Id: cgi.h,v 1.12 2001/09/13 23:31:25 jongfoster Exp $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/cgi.h,v $
 *
 * Purpose     :  Declares functions to intercept request, generate
 *                html or gif answers, and to compose HTTP resonses.
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
 *    $Log: cgi.h,v $
 *    Revision 1.12  2001/09/13 23:31:25  jongfoster
 *    Moving image data to cgi.c rather than cgi.h.
 *
 *    Revision 1.11  2001/08/05 16:06:20  jongfoster
 *    Modifiying "struct map" so that there are now separate header and
 *    "map_entry" structures.  This means that functions which modify a
 *    map no longer need to return a pointer to the modified map.
 *    Also, it no longer reverses the order of the entries (which may be
 *    important with some advanced template substitutions).
 *
 *    Revision 1.10  2001/08/01 21:19:22  jongfoster
 *    Moving file version information to a separate CGI page.
 *
 *    Revision 1.9  2001/08/01 00:17:54  jongfoster
 *    Adding prototype for map_conditional
 *
 *    Revision 1.8  2001/07/30 22:08:36  jongfoster
 *    Tidying up #defines:
 *    - All feature #defines are now of the form FEATURE_xxx
 *    - Permanently turned off WIN_GUI_EDIT
 *    - Permanently turned on WEBDAV and SPLIT_PROXY_ARGS
 *
 *    Revision 1.7  2001/07/29 18:43:08  jongfoster
 *    Changing #ifdef _FILENAME_H to FILENAME_H_INCLUDED, to conform to
 *    ANSI C rules.
 *
 *    Revision 1.6  2001/06/29 21:45:41  oes
 *    Indentation, CRLF->LF, Tab-> Space
 *
 *    Revision 1.5  2001/06/29 13:22:44  oes
 *    - Cleaned up
 *    - Added new functions: default_exports(), make_menu(),
 *      error_response() etc, ranamed others and changed
 *      param and return types.
 *    - Removed HTTP/HTML snipplets
 *    - Removed logentry from cancelled commit
 *
 *    Revision 1.4  2001/06/09 10:50:58  jongfoster
 *    Changing "show URL info" handler to new style.
 *    Adding "extern" to some function prototypes.
 *
 *    Revision 1.3  2001/06/03 19:12:16  oes
 *    introduced new cgi handling
 *
 *    No revisions before 1.3
 *
 **********************************************************************/


#include "project.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Dispatch & parameter parsing functions
 */
extern struct http_response *dispatch_cgi(struct client_state *csp);
extern struct map *parse_cgi_parameters(char *argstring);

/*
 * CGI functions
 */
extern int cgi_show_version(struct client_state *csp, struct http_response *rsp,
                            struct map *parameters);
extern int cgi_default(struct client_state *csp, struct http_response *rsp,
                       struct map *parameters);
extern int cgi_show_status(struct client_state *csp, struct http_response *rsp,
                           struct map *parameters);
extern int cgi_show_url_info(struct client_state *csp, struct http_response *rsp,
                             struct map *parameters);
extern int cgi_send_banner(struct client_state *csp, struct http_response *rsp,
   		                  struct map *parameters);

/* Not exactly a CGI */
extern struct http_response *error_response(struct client_state *csp, const char *template, int err);

/*
 * CGI support functions
 */
extern struct http_response * alloc_http_response(void);
extern void free_http_response(struct http_response *rsp);

extern struct http_response *finish_http_response(struct http_response *rsp);

extern struct map * default_exports(const struct client_state *csp, const char *caller);
extern void map_block_killer(struct map *map, const char *name);
extern void map_conditional(struct map *exports, const char *name, int choose_first);
extern char *fill_template(struct client_state *csp, const char *templatename, struct map *exports);


/*
 * Text generators
 */
extern char *make_menu(const char *self);
extern char *dump_map(const struct map *map);

#ifdef FEATURE_STATISTICS
extern struct map *add_stats(struct map *exports);
#endif /* def FEATURE_STATISTICS */

/*
 * Some images.
 */
extern const char image_junkbuster_gif_data[];
extern const int  image_junkbuster_gif_length;
extern const char image_blank_gif_data[];
extern const int  image_blank_gif_length;


/* Revision control strings from this header and associated .c file */
extern const char cgi_rcs[];
extern const char cgi_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef CGI_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/
