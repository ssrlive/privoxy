const char cgisimple_rcs[] = "$Id: cgisimple.c,v 1.5 2001/10/07 15:30:41 oes Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/cgisimple.c,v $
 *
 * Purpose     :  Simple CGIs to get information about JunkBuster's
 *                status.
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
 *    $Log: cgisimple.c,v $
 *    Revision 1.5  2001/10/07 15:30:41  oes
 *    Removed FEATURE_DENY_GZIP
 *
 *    Revision 1.4  2001/10/02 15:31:12  oes
 *    Introduced show-request cgi
 *
 *    Revision 1.3  2001/09/22 16:34:44  jongfoster
 *    Removing unneeded #includes
 *
 *    Revision 1.2  2001/09/19 18:01:11  oes
 *    Fixed comments; cosmetics
 *
 *    Revision 1.1  2001/09/16 17:08:54  jongfoster
 *    Moving simple CGI functions from cgi.c to new file cgisimple.c
 *
 *
 **********************************************************************/


#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#define snprintf _snprintf
#endif /* def _WIN32 */

#include "project.h"
#include "cgi.h"
#include "cgisimple.h"
#include "list.h"
#include "encode.h"
#include "jcc.h"
#include "filters.h"
#include "actions.h"
#include "miscutil.h"
#include "loadcfg.h"
#include "parsers.h"

const char cgisimple_h_rcs[] = CGISIMPLE_H_VERSION;


static char *show_rcs(void);
static void show_defines(struct map *exports);


/*********************************************************************
 *
 * Function    :  cgi_default
 *
 * Description :  CGI function that is called if no action was given.
 *                Lists menu of available unhidden CGIs.
 *               
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_default(struct client_state *csp, struct http_response *rsp,
                struct map *parameters)
{
   char *p;
   char *tmp = NULL;
   struct map *exports = default_exports(csp, "");

   /* If there were other parameters, export a dump as "cgi-parameters" */
   if(parameters)
   {
      p = dump_map(parameters);
      tmp = strsav(tmp, "<p>What made you think this cgi takes parameters?\n"
                        "Anyway, here they are, in case you're interested:</p>\n");
      tmp = strsav(tmp, p);
      map(exports, "cgi-parameters", 1, tmp, 0);
      free(p);
   }
   else
   {
      map(exports, "cgi-parameters", 1, "", 1);
   }

   rsp->body = template_load(csp, "default");
   template_fill(&rsp->body, exports);
   free_map(exports);
   return(0);

}




/*********************************************************************
 *
 * Function    :  cgi_error_404
 *
 * Description :  CGI function that is called if an unknow action was
 *                given.
 *               
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_error_404(struct client_state *csp,
                  struct http_response *rsp,
                  struct map *parameters)
{
   struct map *exports = default_exports(csp, NULL);

   rsp->status = strdup("404 JunkBuster configuration page not found");
   rsp->body = template_load(csp, "cgi-error-404");
   template_fill(&rsp->body, exports);
   free_map(exports);
   return 0;
}


/*********************************************************************
 *
 * Function    :  cgi_show_request
 *
 * Description :  Show the client's request and what sed() would have
 *                made of it.
 *               
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_show_request(struct client_state *csp, struct http_response *rsp,
                struct map *parameters)
{
   char *p;
   struct map *exports = default_exports(csp, "show-request");
   
   /*
    * Repair the damage done to the IOB by get_header()
    */
   for (p = csp->iob->buf; p < csp->iob->eod; p++)
   {
      if (*p == '\0') *p = '\n';
   }

   /*
    * Export the original client's request and the one we would
    * be sending to the server if this wasn't a CGI call
    */
   map(exports, "client-request", 1, csp->iob->buf, 1);
   map(exports, "processed-request", 1, sed(client_patterns, add_client_headers, csp), 0);
   
   rsp->body = template_load(csp, "show-request");
   template_fill(&rsp->body, exports);
   free_map(exports);
   return(0);
 
}


/*********************************************************************
 *
 * Function    :  cgi_send_banner
 *
 * Description :  CGI function that returns a banner. 
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *           type : Selects the type of banner between "trans" and "jb".
 *                  Defaults to "jb" if absent or != "trans".
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_send_banner(struct client_state *csp, struct http_response *rsp,
                    struct map *parameters)
{
   if(strcmp(lookup(parameters, "type"), "trans"))
   {
      rsp->body = bindup(image_junkbuster_gif_data, image_junkbuster_gif_length);
      rsp->content_length = image_junkbuster_gif_length;
   }
   else
   {
      rsp->body = bindup(image_blank_gif_data, image_blank_gif_length);
      rsp->content_length = image_blank_gif_length;
   }   

   enlist(rsp->headers, "Content-Type: image/gif");
   rsp->is_static = 1;

   return(0);

}


/*********************************************************************
 *
 * Function    :  cgi_show_version
 *
 * Description :  CGI function that returns a a web page describing the
 *                file versions of IJB.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_show_version(struct client_state *csp, struct http_response *rsp,
                     struct map *parameters)
{
   struct map * exports = default_exports(csp, "show-version");

   map(exports, "sourceversions", 1, show_rcs(), 0);  

   rsp->body = template_load(csp, "show-version");
   template_fill(&rsp->body, exports);
   free_map(exports);

   return(0);

}

 
/*********************************************************************
 *
 * Function    :  cgi_show_status
 *
 * Description :  CGI function that returns a a web page describing the
 *                current status of IJB.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_show_status(struct client_state *csp, struct http_response *rsp,
                    struct map *parameters)
{
   char *s = NULL;
   int i;

   FILE * fp;
   char buf[BUFFER_SIZE];
   char * p;
   const char * filename = NULL;
   char * file_description = NULL;
#ifdef FEATURE_STATISTICS
   float perc_rej;   /* Percentage of http requests rejected */
   int local_urls_read;
   int local_urls_rejected;
#endif /* ndef FEATURE_STATISTICS */

   struct map * exports = default_exports(csp, "show-status");

   switch (*(lookup(parameters, "file")))
   {
   case 'p':
      if (csp->actions_list)
      {
         filename = csp->actions_list->filename;
         file_description = "Actions List";
      }
      break;

   case 'r':
      if (csp->rlist)
      {
         filename = csp->rlist->filename;
         file_description = "Regex Filter List";
      }
      break;

#ifdef FEATURE_TRUST
   case 't':
      if (csp->tlist)
      {
         filename = csp->tlist->filename;
         file_description = "Trust List";
      }
      break;
#endif /* def FEATURE_TRUST */
   }

   if (NULL != filename)
   {
      map(exports, "file-description", 1, file_description, 1);
      map(exports, "filepath", 1, html_encode(filename), 0);

      if ((fp = fopen(filename, "r")) == NULL)
      {
         map(exports, "content", 1, "<h1>ERROR OPENING FILE!</h1>", 1);
      }
      else
      {
         while (fgets(buf, sizeof(buf), fp))
         {
            p = html_encode(buf);
            if (p)
            {
               s = strsav(s, p);
               freez(p);
               s = strsav(s, "<br>");
            }
         }
         fclose(fp);
         map(exports, "contents", 1, s, 0);
      }
      rsp->body = template_load(csp, "show-status-file");
      template_fill(&rsp->body, exports);
      free_map(exports);
      return(0);

   }

   map(exports, "redirect-url", 1, REDIRECT_URL, 1);
   
   s = NULL;
   for (i=0; i < Argc; i++)
   {
      s = strsav(s, Argv[i]);
      s = strsav(s, " ");
   }
   map(exports, "invocation", 1, s, 0);

   map(exports, "options", 1, csp->config->proxy_args, 1);
   show_defines(exports);

#ifdef FEATURE_STATISTICS
   local_urls_read     = urls_read;
   local_urls_rejected = urls_rejected;

   /*
    * Need to alter the stats not to include the fetch of this
    * page.
    *
    * Can't do following thread safely! doh!
    *
    * urls_read--;
    * urls_rejected--; * This will be incremented subsequently *
    */

   if (local_urls_read == 0)
   {
      map_block_killer(exports, "have-stats");
   }
   else
   {
      map_block_killer(exports, "have-no-stats");

      perc_rej = (float)local_urls_rejected * 100.0F /
            (float)local_urls_read;

      sprintf(buf, "%d", local_urls_read);
      map(exports, "requests-received", 1, buf, 1);

      sprintf(buf, "%d", local_urls_rejected);
      map(exports, "requests-blocked", 1, buf, 1);

      sprintf(buf, "%6.2f", perc_rej);
      map(exports, "percent-blocked", 1, buf, 1);
   }

#else /* ndef FEATURE_STATISTICS */
   map_block_killer(exports, "statistics");
#endif /* ndef FEATURE_STATISTICS */

   if (csp->actions_list)
   {
      map(exports, "actions-filename", 1,  csp->actions_list->filename, 1);
   }
   else
   {
      map(exports, "actions-filename", 1, "None specified", 1);
   }

   if (csp->rlist)
   {
      map(exports, "re-filter-filename", 1,  csp->rlist->filename, 1);
   }
   else
   {
      map(exports, "re-filter-filename", 1, "None specified", 1);
   }

#ifdef FEATURE_TRUST
   if (csp->tlist)
   {
      map(exports, "trust-filename", 1,  csp->tlist->filename, 1);
   }
   else
   {
       map(exports, "trust-filename", 1, "None specified", 1);
   }
#else
   map_block_killer(exports, "trust-support");
#endif /* ndef FEATURE_TRUST */

   rsp->body = template_load(csp, "show-status");
   template_fill(&rsp->body, exports);
   free_map(exports);
   return(0);

}

 
/*********************************************************************
 *
 * Function    :  cgi_show_url_info
 *
 * Description :  CGI function that determines and shows which actions
 *                junkbuster will perform for a given url, and which
 *                matches starting from the defaults have lead to that.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *            url : The url whose actions are to be determined.
 *                  If url is unset, the url-given conditional will be
 *                  set, so that all but the form can be suppressed in
 *                  the template.
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_show_url_info(struct client_state *csp, struct http_response *rsp,
                      struct map *parameters)
{
   char *url_param;
   char *host = NULL;
   struct map * exports = default_exports(csp, "show-url-info");

   if (NULL == (url_param = strdup(lookup(parameters, "url"))) || *url_param == '\0')
   {
      map_block_killer(exports, "url-given");
      map(exports, "url", 1, "", 1);
   }
   else
   {
      char *matches = NULL;
      char *path;
      char *s;
      int port = 80;
      int hits = 0;
      struct file_list *fl;
      struct url_actions *b;
      struct url_spec url[1];
      struct current_action_spec action[1];
      
      host = url_param;
      host += (strncmp(url_param, "http://", 7)) ? 0 : 7;

      map(exports, "url", 1, host, 1);
      map(exports, "url-html", 1, html_encode(host), 0);

      init_current_action(action);

      s = current_action_to_text(action);
      map(exports, "default", 1, s , 0);

      if (((fl = csp->actions_list) == NULL) || ((b = fl->f) == NULL))
      {
         map(exports, "matches", 1, "none" , 1);
         map(exports, "final", 1, lookup(exports, "default"), 1);

         freez(url_param);
         free_current_action(action);

         rsp->body = template_load(csp, "show-url-info");
         template_fill(&rsp->body, exports);
         free_map(exports);

         return 0;
      }

      s = strchr(host, '/');
      if (s != NULL)
      {
         path = strdup(s);
         *s = '\0';
      }
      else
      {
         path = strdup("");
      }
      s = strchr(host, ':');
      if (s != NULL)
      {
         *s++ = '\0';
         port = atoi(s);
         s = NULL;
      }

      *url = dsplit(host);

      /* if splitting the domain fails, punt */
      if (url->dbuf == NULL)
      {
         map(exports, "matches", 1, "none" , 1);
         map(exports, "final", 1, lookup(exports, "default"), 1);

         freez(url_param);
         freez(path);
         free_current_action(action);

         rsp->body = template_load(csp, "show-url-info");
         template_fill(&rsp->body, exports);
         free_map(exports);

         return 0;
      }

      for (b = b->next; NULL != b; b = b->next)
      {
         if ((b->url->port == 0) || (b->url->port == port))
         {
            if ((b->url->domain[0] == '\0') || (domaincmp(b->url, url) == 0))
            {
               if ((b->url->path == NULL) ||
#ifdef REGEX
                  (regexec(b->url->preg, path, 0, NULL, 0) == 0)
#else
                  (strncmp(b->url->path, path, b->url->pathlen) == 0)
#endif
               )
               {
                  s = actions_to_text(b->action);
                  matches = strsav(matches, "<b>{");
                  matches = strsav(matches, s);
                  matches = strsav(matches, " }</b><br>\n<code>");
                  matches = strsav(matches, b->url->spec);
                  matches = strsav(matches, "</code><br>\n<br>\n");
                  freez(s);

                  merge_current_action(action, b->action);
                  hits++;
               }
            }
         }
      }

      if (hits)
      {
         map(exports, "matches", 1, matches , 0);
      }
      else
      {
         map(exports, "matches", 1, "none", 1);
      }
      matches = NULL;

      freez(url->dbuf);
      freez(url->dvec);

      freez(url_param);
      freez(path);

      s = current_action_to_text(action);
      map(exports, "final", 1, s, 0);
      s = NULL;

      free_current_action(action);
   }

   rsp->body = template_load(csp, "show-url-info");
   template_fill(&rsp->body, exports);
   free_map(exports);
   return 0;

}


/*********************************************************************
 *
 * Function    :  cgi_robots_txt
 *
 * Description :  CGI function to return "/robots.txt".
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_robots_txt(struct client_state *csp, struct http_response *rsp,
                   struct map *parameters)
{
   char buf[100];

   rsp->body = strdup(
      "# This is the Internet Junkbuster control interface.\n"
      "# It isn't very useful to index it, and you're likely to break stuff.\n"
      "# So go away!\n"
      "\n"
      "User-agent: *\n"
      "Disallow: /\n"
      "\n");

   enlist_unique(rsp->headers, "Content-Type: text/plain", 13);

   rsp->is_static = 1;

   get_http_time(7 * 24 * 60 * 60, buf); /* 7 days into future */
   enlist_unique_header(rsp->headers, "Expires", buf);

   return 0;

}


/*********************************************************************
 *
 * Function    :  show_defines
 *
 * Description :  Create a string with all conditional #defines used
 *                when building
 *
 * Parameters  :  None
 *
 * Returns     :  string 
 *
 *********************************************************************/
static void show_defines(struct map *exports)
{

#ifdef FEATURE_ACL
   map_conditional(exports, "FEATURE_ACL", 1);
#else /* ifndef FEATURE_ACL */
   map_conditional(exports, "FEATURE_ACL", 0);
#endif /* ndef FEATURE_ACL */

#ifdef FEATURE_COOKIE_JAR
   map_conditional(exports, "FEATURE_COOKIE_JAR", 1);
#else /* ifndef FEATURE_COOKIE_JAR */
   map_conditional(exports, "FEATURE_COOKIE_JAR", 0);
#endif /* ndef FEATURE_COOKIE_JAR */

#ifdef FEATURE_FAST_REDIRECTS
   map_conditional(exports, "FEATURE_FAST_REDIRECTS", 1);
#else /* ifndef FEATURE_FAST_REDIRECTS */
   map_conditional(exports, "FEATURE_FAST_REDIRECTS", 0);
#endif /* ndef FEATURE_FAST_REDIRECTS */

#ifdef FEATURE_FORCE_LOAD
   map_conditional(exports, "FEATURE_FORCE_LOAD", 1);
#else /* ifndef FEATURE_FORCE_LOAD */
   map_conditional(exports, "FEATURE_FORCE_LOAD", 0);
#endif /* ndef FEATURE_FORCE_LOAD */

#ifdef FEATURE_IMAGE_BLOCKING
   map_conditional(exports, "FEATURE_IMAGE_BLOCKING", 1);
#else /* ifndef FEATURE_IMAGE_BLOCKING */
   map_conditional(exports, "FEATURE_IMAGE_BLOCKING", 0);
#endif /* ndef FEATURE_IMAGE_BLOCKING */

#ifdef FEATURE_IMAGE_DETECT_MSIE
   map_conditional(exports, "FEATURE_IMAGE_DETECT_MSIE", 1);
#else /* ifndef FEATURE_IMAGE_DETECT_MSIE */
   map_conditional(exports, "FEATURE_IMAGE_DETECT_MSIE", 0);
#endif /* ndef FEATURE_IMAGE_DETECT_MSIE */

#ifdef FEATURE_KILL_POPUPS
   map_conditional(exports, "FEATURE_KILL_POPUPS", 1);
#else /* ifndef FEATURE_KILL_POPUPS */
   map_conditional(exports, "FEATURE_KILL_POPUPS", 0);
#endif /* ndef FEATURE_KILL_POPUPS */

#ifdef FEATURE_PTHREAD
   map_conditional(exports, "FEATURE_PTHREAD", 1);
#else /* ifndef FEATURE_PTHREAD */
   map_conditional(exports, "FEATURE_PTHREAD", 0);
#endif /* ndef FEATURE_PTHREAD */

#ifdef FEATURE_STATISTICS
   map_conditional(exports, "FEATURE_STATISTICS", 1);
#else /* ifndef FEATURE_STATISTICS */
   map_conditional(exports, "FEATURE_STATISTICS", 0);
#endif /* ndef FEATURE_STATISTICS */

#ifdef FEATURE_TOGGLE
   map_conditional(exports, "FEATURE_TOGGLE", 1);
#else /* ifndef FEATURE_TOGGLE */
   map_conditional(exports, "FEATURE_TOGGLE", 0);
#endif /* ndef FEATURE_TOGGLE */

#ifdef FEATURE_TRUST
   map_conditional(exports, "FEATURE_TRUST", 1);
#else /* ifndef FEATURE_TRUST */
   map_conditional(exports, "FEATURE_TRUST", 0);
#endif /* ndef FEATURE_TRUST */

#ifdef REGEX_GNU
   map_conditional(exports, "REGEX_GNU", 1);
#else /* ifndef REGEX_GNU */
   map_conditional(exports, "REGEX_GNU", 0);
#endif /* def REGEX_GNU */

#ifdef REGEX_PCRE
   map_conditional(exports, "REGEX_PCRE", 1);
#else /* ifndef REGEX_PCRE */
   map_conditional(exports, "REGEX_PCRE", 0);
#endif /* def REGEX_PCRE */

#ifdef STATIC_PCRE
   map_conditional(exports, "STATIC_PCRE", 1);
#else /* ifndef STATIC_PCRE */
   map_conditional(exports, "STATIC_PCRE", 0);
#endif /* ndef STATIC_PCRE */

#ifdef STATIC_PCRS
   map_conditional(exports, "STATIC_PCRS", 1);
#else /* ifndef STATIC_PCRS */
   map_conditional(exports, "STATIC_PCRS", 0);
#endif /* ndef STATIC_PCRS */

   map(exports, "FORCE_PREFIX", 1, FORCE_PREFIX, 1);

}


/*********************************************************************
 *
 * Function    :  show_rcs
 *
 * Description :  Create a string with the rcs info for all sourcefiles
 *
 * Parameters  :  None
 *
 * Returns     :  string 
 *
 *********************************************************************/
static char *show_rcs(void)
{
   char *b = NULL;
   char buf[BUFFER_SIZE];

   /* Instead of including *all* dot h's in the project (thus creating a
    * tremendous amount of dependencies), I will concede to declaring them
    * as extern's.  This forces the developer to add to this list, but oh well.
    */

#define SHOW_RCS(__x)            \
   {                             \
      extern const char __x[];   \
      sprintf(buf, "%s\n", __x); \
      b = strsav(b, buf);        \
   }

   /* In alphabetical order */
   SHOW_RCS(actions_h_rcs)
   SHOW_RCS(actions_rcs)
   SHOW_RCS(cgi_h_rcs)
   SHOW_RCS(cgi_rcs)
#ifdef FEATURE_CGI_EDIT_ACTIONS
   SHOW_RCS(cgiedit_h_rcs)
   SHOW_RCS(cgiedit_rcs)
#endif /* def FEATURE_CGI_EDIT_ACTIONS */
   SHOW_RCS(cgisimple_h_rcs)
   SHOW_RCS(cgisimple_rcs)
#ifdef __MINGW32__
   SHOW_RCS(cygwin_h_rcs)
#endif
   SHOW_RCS(deanimate_h_rcs)
   SHOW_RCS(deanimate_rcs)
   SHOW_RCS(encode_h_rcs)
   SHOW_RCS(encode_rcs)
   SHOW_RCS(errlog_h_rcs)
   SHOW_RCS(errlog_rcs)
   SHOW_RCS(filters_h_rcs)
   SHOW_RCS(filters_rcs)
   SHOW_RCS(gateway_h_rcs)
   SHOW_RCS(gateway_rcs)
#ifdef GNU_REGEX
   SHOW_RCS(gnu_regex_h_rcs)
   SHOW_RCS(gnu_regex_rcs)
#endif /* def GNU_REGEX */
   SHOW_RCS(jbsockets_h_rcs)
   SHOW_RCS(jbsockets_rcs)
   SHOW_RCS(jcc_h_rcs)
   SHOW_RCS(jcc_rcs)
#ifdef FEATURE_KILL_POPUPS
   SHOW_RCS(killpopup_h_rcs)
   SHOW_RCS(killpopup_rcs)
#endif /* def FEATURE_KILL_POPUPS */
   SHOW_RCS(list_h_rcs)
   SHOW_RCS(list_rcs)
   SHOW_RCS(loadcfg_h_rcs)
   SHOW_RCS(loadcfg_rcs)
   SHOW_RCS(loaders_h_rcs)
   SHOW_RCS(loaders_rcs)
   SHOW_RCS(miscutil_h_rcs)
   SHOW_RCS(miscutil_rcs)
   SHOW_RCS(parsers_h_rcs)
   SHOW_RCS(parsers_rcs)
   SHOW_RCS(pcrs_rcs)
   SHOW_RCS(pcrs_h_rcs)
   SHOW_RCS(project_h_rcs)
   SHOW_RCS(ssplit_h_rcs)
   SHOW_RCS(ssplit_rcs)
#ifdef _WIN32
#ifndef _WIN_CONSOLE
   SHOW_RCS(w32log_h_rcs)
   SHOW_RCS(w32log_rcs)
   SHOW_RCS(w32res_h_rcs)
   SHOW_RCS(w32taskbar_h_rcs)
   SHOW_RCS(w32taskbar_rcs)
#endif /* ndef _WIN_CONSOLE */
   SHOW_RCS(win32_h_rcs)
   SHOW_RCS(win32_rcs)
#endif /* def _WIN32 */

#undef SHOW_RCS

   return(b);

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
