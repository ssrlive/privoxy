#ifndef _CGI_H
#define _CGI_H
#define CGI_H_VERSION "$Id: $"
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
 *
 *
 **********************************************************************/


#include "project.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct http_response *cgi_dispatch(struct client_state *csp);
extern int make_http_response(struct http_response *rsp);
extern void free_http_response(struct http_response *rsp);

extern struct map *parse_cgi(char *argstring);
char *dump_map(struct map *map);

extern int cgi_default(struct client_state *csp, struct http_response *rsp,
		       struct map *parameters);
int cgi_show_status(struct client_state *csp, struct http_response *rsp,
                    struct map *parameters);

extern char *ijb_show_url_info(struct http_request *http, struct client_state *csp);

extern char *redirect_url(struct http_request *http, struct client_state *csp);
extern int cgi_send_banner(struct client_state *csp, struct http_response *rsp,
			   struct map *parameters);



#ifdef TRUST_FILES
extern char *ij_untrusted_url(struct http_request *http, struct client_state *csp);
#endif /* def TRUST_FILES */



#ifdef STATISTICS
extern char *add_stats(char *s);
#endif /* def STATISTICS */

static const char CJBGIF[] =
   "GIF89aD\000\013\000\360\000\000\000\000\000\377\377\377!"
   "\371\004\001\000\000\001\000,\000\000\000\000D\000\013\000"
   "\000\002a\214\217\251\313\355\277\000\200G&K\025\316hC\037"
   "\200\234\230Y\2309\235S\230\266\206\372J\253<\3131\253\271"
   "\270\215\342\254\013\203\371\202\264\334P\207\332\020o\266"
   "N\215I\332=\211\312\3513\266:\026AK)\364\370\365aobr\305"
   "\372\003S\275\274k2\354\254z\347?\335\274x\306^9\374\276"
   "\037Q\000\000;";

static const char CBLANKGIF[] =
   "GIF89a\001\000\001\000\200\000\000\377\377\377\000\000"
   "\000!\371\004\001\000\000\000\000,\000\000\000\000\001"
   "\000\001\000\000\002\002D\001\000;";

static const char CBLOCK[] = 
#ifdef AMIGA 
       "HTTP/1.0 403 Request for blocked URL\n" 
#else /* ifndef AMIGA */
       "HTTP/1.0 202 Request for blocked URL\n"
#endif /* ndef AMIGA */
       "Pragma: no-cache\n"
       "Last-Modified: Thu Jul 31, 1997 07:42:22 pm GMT\n"
       "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT\n"
       "Content-Type: text/html\n\n"
       "<html>\n"
       "<head>\n"
       "<title>Internet Junkbuster: Request for blocked URL</title>\n"
       "</head>\n"
       WHITEBG
       "<center><h1>"
       BANNER
       "</h1></center>\n"
      "<p align=center>Your request for <b>%s%s</b>\n"
      "was blocked.<br><a href=\"http://i.j.b/show-url-info?url=%s%s\">See why</a>"
#ifdef FORCE_LOAD
       " or <a href=\"http://%s" FORCE_PREFIX "%s\">"
       "go there anyway.</a>"
#endif /* def FORCE_LOAD */
      "</p>\n"
      "</body>\n"
      "</html>\n";

#ifdef TRUST_FILES
static const char CTRUST[] =
#ifdef AMIGA 
       "HTTP/1.0 403 Request for untrusted URL\n"
#else /* ifndef AMIGA */
       "HTTP/1.0 202 Request for untrusted URL\n"
#endif /* ndef AMIGA */
       "Pragma: no-cache\n"
       "Last-Modified: Thu Jul 31, 1997 07:42:22 pm GMT\n"
       "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT\n"
       "Content-Type: text/html\n\n"
       "<html>\n"
       "<head>\n"
       "<title>Internet Junkbuster: Request for untrusted URL</title>\n"
       "</head>\n"
       WHITEBG
       "<center>"
       "<a href=http://i.j.b/ij-untrusted-url?%s+%s+%s>"
       BANNER
       "</a>"
       "</center>"
       "</body>\n"
       "</html>\n";
#endif /* def TRUST_FILES */


static const char C_HOME_PAGE[] =
   "<html>\n"
   "<head>\n"
   "<title>Internet Junkbuster: Information</title>\n"
   "</head>\n"
   BODY
   "<h1><center>"
   BANNER
   "</h1></center>\n"
   "<p><a href=\"" HOME_PAGE_URL "\">JunkBuster web site</a></p>\n"
   "<p><a href=\"http://i.j.b/show-proxy-arg\">Proxy configuration</a></p>\n"
   "<p><a href=\"http://i.j.b/show-url-info\">Look up a URL</a></p>\n"
   "</body>\n"
   "</html>\n";

static const char C_URL_INFO_HEADER[] =
   "HTTP/1.0 200 OK\n"
   "Pragma: no-cache\n"
   "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT\n"
   "Content-Type: text/html\n\n"
   "<html>\n"
   "<head>\n"
   "<title>Internet Junkbuster: URL Info</title>\n"
   "</head>\n"
   BODY
   "<h1><center>"
   BANNER
   "</h1></center>\n"
   "<p>Information for: <a href=\"http://%s\">http://%s</a></p>\n";
static const char C_URL_INFO_FOOTER[] =
   "\n</p>\n"
   "</body>\n"
   "</html>\n";

static const char C_URL_INFO_FORM[] =
   "HTTP/1.0 200 OK\n"
   "Pragma: no-cache\n"
   "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT\n"
   "Content-Type: text/html\n\n"
   "<html>\n"
   "<head>\n"
   "<title>Internet Junkbuster: URL Info</title>\n"
   "</head>\n"
   BODY
   "<h1><center>"
   BANNER
   "</h1></center>\n"
   "<form method=\"GET\" action=\"http://i.j.b/show-url-info\">\n"
   "<p>Please enter a URL, without the leading &quot;http://&quot;:</p>"
   "<p><input type=\"text\" name=\"url\" size=\"80\">"
   "<input type=\"submit\" value=\"Info\"></p>\n"
   "</form>\n"
   "</body>\n"
   "</html>\n";

static const char CFAIL[] =
   "HTTP/1.0 503 Connect failed\n"
   "Content-Type: text/html\n\n"
   "<html>\n"
   "<head>\n"
   "<title>Internet Junkbuster: Connect failed</title>\n"
   "</head>\n"
   BODY
   "<h1><center>"
   BANNER
   "</center></h1>"
   "TCP connection to '%s' failed: %s.\n<br>"
   "</body>\n"
   "</html>\n";

static const char CNXDOM[] =
   "HTTP/1.0 404 Non-existent domain\n"
   "Content-Type: text/html\n\n"
   "<html>\n"
   "<head>\n"
   "<title>Internet Junkbuster: Non-existent domain</title>\n"
   "</head>\n"
   BODY
   "<h1><center>"
   BANNER
   "</center></h1>"
   "No such domain: %s\n"
   "</body>\n"
   "</html>\n";

static const char CNOBANNER[] =
   "HTTP/1.0 200 No Banner\n"
   "Content-Type: text/html\n\n"
   "<html>\n"
   "<head>\n"
   "<title>Internet Junkbuster: No Banner</title>\n"
   "</head>\n"
   BODY
   "<h1><center>"
   BANNER
   "</h1>"
   "You asked for a banner that this proxy can't produce because either configuration does not permit.\n<br>"
   "or the URL didn't end with .gif\n"
   "</center></body>\n"
   "</html>\n";


/* Revision control strings from this header and associated .c file */
extern const char cgi_rcs[];
extern const char cgi_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _CGI_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
