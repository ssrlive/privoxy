#ifndef CGI_H_INCLUDED
#define CGI_H_INCLUDED
#define CGI_H_VERSION "$Id: cgi.h,v 1.7 2001/07/29 18:43:08 jongfoster Exp $"
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
extern struct http_response *finish_http_response(struct http_response *rsp);
extern void free_http_response(struct http_response *rsp);

extern struct map *default_exports(struct client_state *csp, char *caller);
extern struct map *map_block_killer(struct map *map, char *name);
extern char *fill_template(struct client_state *csp, const char *template, struct map *exports);


/*
 * Text generators
 */
extern char *make_menu(const char *self);
extern char *dump_map(struct map *map);

#ifdef FEATURE_STATISTICS
extern struct map *add_stats(struct map *exports);
#endif /* def FEATURE_STATISTICS */


/*
 * Hint: You can encode your own GIFs like that:
 * perl -e 'while (read STDIN, $c, 1) { printf("\\%.3o,", unpack("C", $c)); }'
 */

static const char JBGIF[] =
   "GIF89aD\000\013\000\360\000\000\000\000\000\377\377\377!"
   "\371\004\001\000\000\001\000,\000\000\000\000D\000\013\000"
   "\000\002a\214\217\251\313\355\277\000\200G&K\025\316hC\037"
   "\200\234\230Y\2309\235S\230\266\206\372J\253<\3131\253\271"
   "\270\215\342\254\013\203\371\202\264\334P\207\332\020o\266"
   "N\215I\332=\211\312\3513\266:\026AK)\364\370\365aobr\305"
   "\372\003S\275\274k2\354\254z\347?\335\274x\306^9\374\276"
   "\037Q\000\000;";

static const char BLANKGIF[] =
   "GIF89a\001\000\001\000\200\000\000\377\377\377\000\000"
   "\000!\371\004\001\000\000\000\000,\000\000\000\000\001"
   "\000\001\000\000\002\002D\001\000;";


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
