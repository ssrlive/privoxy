const char cgi_rcs[] = "$Id: cgi.c,v 1.43 2002/03/05 21:33:45 david__schmidt Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/cgi.c,v $
 *
 * Purpose     :  Declares functions to intercept request, generate
 *                html or gif answers, and to compose HTTP resonses.
 *                This only contains the framework functions, the
 *                actual handler functions are declared elsewhere.
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
 *    $Log: cgi.c,v $
 *    Revision 1.43  2002/03/05 21:33:45  david__schmidt
 *    - Re-enable OS/2 building after new parms were added
 *    - Fix false out of memory report when resolving CGI templates when no IP
 *      address is available of failed attempt (a la no such domain)
 *
 *    Revision 1.42  2002/01/21 00:33:20  jongfoster
 *    Replacing strsav() with the safer string_append() or string_join().
 *    Adding map_block_keep() to save a few bytes in the edit-actions-list HTML.
 *    Adding missing html_encode() to error message generators.
 *    Adding edit-actions-section-swap and many "shortcuts" to the list of CGIs.
 *
 *    Revision 1.41  2002/01/17 20:56:22  jongfoster
 *    Replacing hard references to the URL of the config interface
 *    with #defines from project.h
 *
 *    Revision 1.40  2002/01/09 14:26:46  oes
 *    Added support for thread-safe gmtime_r call.
 *
 *    Revision 1.39  2001/11/16 00:48:13  jongfoster
 *    Fixing a compiler warning
 *
 *    Revision 1.38  2001/11/13 00:31:21  jongfoster
 *    - Adding new CGIs for use by non-JavaScript browsers:
 *        edit-actions-url-form
 *        edit-actions-add-url-form
 *        edit-actions-remove-url-form
 *    - Fixing make_menu()'s HTML generation - it now quotes the href parameter.
 *    - Fixing || bug.
 *
 *    Revision 1.37  2001/11/01 14:28:47  david__schmidt
 *    Show enablement/disablement status in almost all templates.
 *    There is a little trickiness here: apparent recursive resolution of
 *    @if-enabled-then@ caused the toggle template to show status out-of-phase with
 *    the actual enablement status.  So a similar construct,
 *    @if-enabled-display-then@, is used to resolve the status display on non-'toggle'
 *    templates.
 *
 *    Revision 1.36  2001/10/26 17:33:27  oes
 *    marginal bugfix
 *
 *    Revision 1.35  2001/10/23 21:48:19  jongfoster
 *    Cleaning up error handling in CGI functions - they now send back
 *    a HTML error page and should never cause a FATAL error.  (Fixes one
 *    potential source of "denial of service" attacks).
 *
 *    CGI actions file editor that works and is actually useful.
 *
 *    Ability to toggle JunkBuster remotely using a CGI call.
 *
 *    You can turn off both the above features in the main configuration
 *    file, e.g. if you are running a multi-user proxy.
 *
 *    Revision 1.34  2001/10/18 22:22:09  david__schmidt
 *    Only show "Local support" on templates conditionally:
 *      - if either 'admin-address' or 'proxy-info-url' are uncommented in config
 *      - if not, no Local support section appears
 *
 *    Revision 1.33  2001/10/14 22:28:41  jongfoster
 *    Fixing stupid typo.
 *
 *    Revision 1.32  2001/10/14 22:20:18  jongfoster
 *    - Changes to CGI dispatching method to match CGI names exactly,
 *      rather than doing a prefix match.
 *    - No longer need to count the length of the CGI handler names by hand.
 *    - Adding new handler for 404 error when disptching a CGI, if none of
 *      the handlers match.
 *    - Adding new handlers for CGI actionsfile editor.
 *
 *    Revision 1.31  2001/10/10 10:56:39  oes
 *    Failiure to load template now fatal. Before, the user got a hard-to-understand assertion failure from cgi.c
 *
 *    Revision 1.30  2001/10/02 15:30:57  oes
 *    Introduced show-request cgi
 *
 *    Revision 1.29  2001/09/20 15:47:44  steudten
 *
 *    Fix BUG: Modify int size to size_t size in fill_template()
 *     - removes big trouble on machines where sizeof(int) != sizeof(size_t).
 *
 *    Revision 1.28  2001/09/19 18:00:37  oes
 *     - Deletef time() FIXME (Can't fail under Linux either, if
 *       the argument is guaranteed to be in out address space,
 *       which it is.)
 *     - Fixed comments
 *     - Pointer notation cosmetics
 *     - Fixed a minor bug in template_fill(): Failiure of
 *       pcrs_execute() now secure.
 *
 *    Revision 1.27  2001/09/16 17:08:54  jongfoster
 *    Moving simple CGI functions from cgi.c to new file cgisimple.c
 *
 *    Revision 1.26  2001/09/16 15:47:37  jongfoster
 *    First version of CGI-based edit interface.  This is very much a
 *    work-in-progress, and you can't actually use it to edit anything
 *    yet.  You must #define FEATURE_CGI_EDIT_ACTIONS for these changes
 *    to have any effect.
 *
 *    Revision 1.25  2001/09/16 15:02:35  jongfoster
 *    Adding i.j.b/robots.txt.
 *    Inlining add_stats() since it's only ever called from one place.
 *
 *    Revision 1.24  2001/09/16 11:38:01  jongfoster
 *    Splitting fill_template() into 2 functions:
 *    template_load() loads the file
 *    template_fill() performs the PCRS regexps.
 *    This is because the CGI edit interface has a "table row"
 *    template which is used many times in the page - this
 *    change means it's only loaded from disk once.
 *
 *    Revision 1.23  2001/09/16 11:16:05  jongfoster
 *    Better error handling in dispatch_cgi() and parse_cgi_parameters()
 *
 *    Revision 1.22  2001/09/16 11:00:10  jongfoster
 *    New function alloc_http_response, for symmetry with free_http_response
 *
 *    Revision 1.21  2001/09/13 23:53:03  jongfoster
 *    Support for both static and dynamically generated CGI pages.
 *    Correctly setting Last-Modified: and Expires: HTTP headers.
 *
 *    Revision 1.20  2001/09/13 23:40:36  jongfoster
 *    (Cosmetic only) Indentation correction
 *
 *    Revision 1.19  2001/09/13 23:31:25  jongfoster
 *    Moving image data to cgi.c rather than cgi.h.
 *
 *    Revision 1.18  2001/08/05 16:06:20  jongfoster
 *    Modifiying "struct map" so that there are now separate header and
 *    "map_entry" structures.  This means that functions which modify a
 *    map no longer need to return a pointer to the modified map.
 *    Also, it no longer reverses the order of the entries (which may be
 *    important with some advanced template substitutions).
 *
 *    Revision 1.17  2001/08/05 15:57:38  oes
 *    Adapted finish_http_response to new list_to_text
 *
 *    Revision 1.16  2001/08/01 21:33:18  jongfoster
 *    Changes to fill_template() that reduce memory usage without having
 *    an impact on performance.  I also renamed some variables so as not
 *    to clash with the C++ keywords "new" and "template".
 *
 *    Revision 1.15  2001/08/01 21:19:22  jongfoster
 *    Moving file version information to a separate CGI page.
 *
 *    Revision 1.14  2001/08/01 00:19:03  jongfoster
 *    New function: map_conditional() for an if-then-else syntax.
 *    Changing to use new version of show_defines()
 *
 *    Revision 1.13  2001/07/30 22:08:36  jongfoster
 *    Tidying up #defines:
 *    - All feature #defines are now of the form FEATURE_xxx
 *    - Permanently turned off WIN_GUI_EDIT
 *    - Permanently turned on WEBDAV and SPLIT_PROXY_ARGS
 *
 *    Revision 1.12  2001/07/29 18:47:05  jongfoster
 *    Adding missing #include "loadcfg.h"
 *
 *    Revision 1.11  2001/07/18 17:24:37  oes
 *    Changed to conform to new pcrs interface
 *
 *    Revision 1.10  2001/07/13 13:53:13  oes
 *    Removed all #ifdef PCRS and related code
 *
 *    Revision 1.9  2001/06/29 21:45:41  oes
 *    Indentation, CRLF->LF, Tab-> Space
 *
 *    Revision 1.8  2001/06/29 13:21:46  oes
 *    - Cosmetics: renamed and reordered functions, variables,
 *      texts, improved comments  etc
 *
 *    - Removed ij_untrusted_url() The relevant
 *      info is now part of the "untrusted" page,
 *      which is generated by filters.c:trust_url()
 *
 *    - Generators of content now call finish_http_response()
 *      themselves, making jcc.c:chat() a little less
 *      cluttered
 *
 *    - Removed obsolete "Pragma: no-cache" from our headers
 *
 *    - http_responses now know their head length
 *
 *    - fill_template now uses the new interface to pcrs, so that
 *       - long jobs (like whole files) no longer have to be assembled
 *         in a fixed size buffer
 *       - the new T (trivial) option is used, and the replacement may
 *         contain Perl syntax backrefs without confusing pcrs
 *
 *    - Introduced default_exports() which generates a set of exports
 *      common to all CGIs and other content generators
 *
 *    - Introduced convenience function map_block_killer()
 *
 *    - Introduced convenience function make_menu()
 *
 *    - Introduced CGI-like function error_response() which generates
 *      the "No such domain" and "Connect failed" messages using the
 *      CGI platform
 *
 *    - cgi_show_url_info:
 *      - adapted to new CGI features
 *      - form and answers now generated from same template
 *      - http:// prefix in URL now OK
 *
 *    - cgi_show_status:
 *      - adapted to new CGI features
 *      - no longer uses csp->init_proxy_args
 *
 *    - cgi_default:
 *      - moved menu generation to make_menu()
 *
 *    - add_stats now writes single export map entries instead
 *      of a fixed string
 *
 *    - Moved redirect_url() to filters.c
 *
 *    - Fixed mem leak in free_http_response(), map_block_killer(),
 *
 *    - Removed logentry from cancelled commit
 *
 *    Revision 1.7  2001/06/09 10:51:58  jongfoster
 *    Changing "show URL info" handler to new style.
 *    Changing BUFSIZ ==> BUFFER_SIZE
 *
 *    Revision 1.6  2001/06/07 23:05:19  jongfoster
 *    Removing code related to old forward and ACL files.
 *
 *    Revision 1.5  2001/06/05 19:59:16  jongfoster
 *    Fixing multiline character string (a GCC-only "feature"), and snprintf (it's _snprintf under VC++).
 *
 *    Revision 1.4  2001/06/04 10:41:52  swa
 *    show version string of cgi.h and cgi.c
 *
 *    Revision 1.3  2001/06/03 19:12:16  oes
 *    introduced new cgi handling
 *
 *    No revisions before 1.3
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
#include "list.h"
#include "encode.h"
#include "ssplit.h"
#include "errlog.h"
#include "miscutil.h"
#include "cgisimple.h"
#ifdef FEATURE_CGI_EDIT_ACTIONS
#include "cgiedit.h"
#endif /* def FEATURE_CGI_EDIT_ACTIONS */
#include "loadcfg.h"
/* loadcfg.h is for g_bToggleIJB only */

const char cgi_h_rcs[] = CGI_H_VERSION;

static const struct cgi_dispatcher cgi_dispatchers[] = {
   { "",
         cgi_default,
         "Junkbuster main page" },
   { "show-status", 
         cgi_show_status,  
         "Show information about the current configuration" }, 
   { "show-version", 
         cgi_show_version,  
         "Show the source code version numbers" }, 
   { "show-request", 
         cgi_show_request,  
         "Show the client's request headers." }, 
   { "show-url-info",
         cgi_show_url_info, 
         "Show which actions apply to a URL and why"  },
   { "toggle",
         cgi_toggle, 
         "Toggle JunkBuster on or off" },
#ifdef FEATURE_CGI_EDIT_ACTIONS
   { "edit-actions",
         cgi_edit_actions, 
         "Edit the actions list" },

   
   { "eaa", /* Shortcut for edit-actions-add-url-form */
         cgi_edit_actions_add_url_form, 
         NULL },
   { "eau", /* Shortcut for edit-actions-url-form */
         cgi_edit_actions_url_form, 
         NULL },
   { "ear", /* Shortcut for edit-actions-remove-url-form */
         cgi_edit_actions_remove_url_form, 
         NULL },
   { "eas", /* Shortcut for edit-actions-for-url */
         cgi_edit_actions_for_url, 
         NULL },
   { "easa", /* Shortcut for edit-actions-section-add */
         cgi_edit_actions_section_add, 
         NULL },
   { "easr", /* Shortcut for edit-actions-section-remove */
         cgi_edit_actions_section_remove, 
         NULL },
   { "eass", /* Shortcut for edit-actions-section-swap */
         cgi_edit_actions_section_swap, 
         NULL },
   { "edit-actions-for-url",
         cgi_edit_actions_for_url, 
         NULL /* Edit the actions for (a) specified URL(s) */ },
   { "edit-actions-list",
         cgi_edit_actions_list, 
         NULL /* Edit the actions list */ },
   { "edit-actions-submit",
         cgi_edit_actions_submit, 
         NULL /* Change the actions for (a) specified URL(s) */ },
   { "edit-actions-url",
         cgi_edit_actions_url, 
         NULL /* Change a URL pattern in the actionsfile */ },
   { "edit-actions-url-form",
         cgi_edit_actions_url_form, 
         NULL /* Form to change a URL pattern in the actionsfile */ },
   { "edit-actions-add-url",
         cgi_edit_actions_add_url, 
         NULL /* Add a URL pattern to the actionsfile */ },
   { "edit-actions-add-url-form",
         cgi_edit_actions_add_url_form, 
         NULL /* Form to add a URL pattern to the actionsfile */ },
   { "edit-actions-remove-url",
         cgi_edit_actions_remove_url, 
         NULL /* Remove a URL pattern from the actionsfile */ },
   { "edit-actions-remove-url-form",
         cgi_edit_actions_remove_url_form, 
         NULL /* Form to remove a URL pattern from the actionsfile */ },
   { "edit-actions-section-add",
         cgi_edit_actions_section_add, 
         NULL /* Remove a section from the actionsfile */ },
   { "edit-actions-section-remove",
         cgi_edit_actions_section_remove, 
         NULL /* Remove a section from the actionsfile */ },
   { "edit-actions-section-swap",
         cgi_edit_actions_section_swap, 
         NULL /* Swap two sections in the actionsfile */ },
#endif /* def FEATURE_CGI_EDIT_ACTIONS */
   { "robots.txt", 
         cgi_robots_txt,  
         NULL /* Sends a robots.txt file to tell robots to go away. */ }, 
   { "send-banner",
         cgi_send_banner, 
         NULL /* Send the transparent or \"Junkbuster\" gif */ },
   { "t",
         cgi_transparent_gif, 
         NULL /* Send a transparent gif (short name) */ },
   { NULL, /* NULL Indicates end of list and default page */
         cgi_error_404,
         NULL /* Unknown CGI page */ }
};


/*
 * Some images
 *
 * Hint: You can encode your own GIFs like this:
 * perl -e 'while (read STDIN, $c, 1) { printf("\\%.3o,", unpack("C", $c)); }'
 */

const char image_junkbuster_gif_data[] =
   "GIF89aD\000\013\000\360\000\000\000\000\000\377\377\377!"
   "\371\004\001\000\000\001\000,\000\000\000\000D\000\013\000"
   "\000\002a\214\217\251\313\355\277\000\200G&K\025\316hC\037"
   "\200\234\230Y\2309\235S\230\266\206\372J\253<\3131\253\271"
   "\270\215\342\254\013\203\371\202\264\334P\207\332\020o\266"
   "N\215I\332=\211\312\3513\266:\026AK)\364\370\365aobr\305"
   "\372\003S\275\274k2\354\254z\347?\335\274x\306^9\374\276"
   "\037Q\000\000;";

const int image_junkbuster_gif_length = sizeof(image_junkbuster_gif_data) - 1;


const char image_blank_gif_data[] =
   "GIF89a\001\000\001\000\200\000\000\377\377\377\000\000"
   "\000!\371\004\001\000\000\000\000,\000\000\000\000\001"
   "\000\001\000\000\002\002D\001\000;";

const int image_blank_gif_length = sizeof(image_blank_gif_data) - 1;


static struct http_response cgi_error_memory_response[1];


static struct http_response *dispatch_known_cgi(struct client_state * csp,
                                                const char * path);
static struct map *parse_cgi_parameters(char *argstring);


/*********************************************************************
 * 
 * Function    :  dispatch_cgi
 *
 * Description :  Checks if a request URL has either the magical
 *                hostname CGI_SITE_1_HOST (usully http://i.j.b/) or
 *                matches CGI_SITE_2_HOST CGI_SITE_2_PATH (usually
 *                http://ijbswa.sourceforge.net/config). If so, it passes
 *                the (rest of the) path onto dispatch_known_cgi, which
 *                calls the relevant CGI handler function.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  http_response if match, NULL if nonmatch or handler fail
 *
 *********************************************************************/
struct http_response *dispatch_cgi(struct client_state *csp)
{
   const char *host = csp->http->host;
   const char *path = csp->http->path;

   /*
    * Should we intercept ?
    */

   /* Note: "example.com" and "example.com." are equivalent hostnames. */

   /* Either the host matches CGI_SITE_1_HOST ..*/
   if (   ( (0 == strcmpic(host, CGI_SITE_1_HOST))
         || (0 == strcmpic(host, CGI_SITE_1_HOST ".")))
       && (path[0] == '/') )
   {
      /* ..then the path will all be for us.  Remove leading '/' */
      path++;
   }
   /* Or it's the host part CGI_SITE_2_HOST, and the path CGI_SITE_2_PATH */
   else if ( ( (0 == strcmpic(host, CGI_SITE_2_HOST ))
            || (0 == strcmpic(host, CGI_SITE_2_HOST ".")) )
          && (0 == strncmpic(path, CGI_SITE_2_PATH, strlen(CGI_SITE_2_PATH))) )
   {
      /* take everything following CGI_SITE_2_PATH */
      path += strlen(CGI_SITE_2_PATH);
      if (*path == '/')
      {
         /* skip the forward slash after CGI_SITE_2_PATH */
         path++;
      }
      else if (*path != '\0')
      {
         /*
          * wierdness: URL is /configXXX, where XXX is some string
          * Do *NOT* intercept.
          */
         return NULL;
      }
   }
   else
   {
      /* Not a CGI */
      return NULL;
   }

   /* 
    * This is a CGI call.
    */

   return dispatch_known_cgi(csp, path);
}


/*********************************************************************
 * 
 * Function    :  dispatch_known_cgi
 *
 * Description :  Processes a CGI once dispatch_cgi has determined that
 *                it matches one of the magic prefixes. Parses the path
 *                as a cgi name plus query string, prepares a map that
 *                maps CGI parameter names to their values, initializes
 *                the http_response struct, and calls the relevant CGI
 *                handler function.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  path = Path of CGI, with the CGI prefix removed.
 *                       Should not have a leading "/".
 *
 * Returns     :  http_response, or NULL on handler failure or out of
 *                memory.
 *
 *********************************************************************/
static struct http_response *dispatch_known_cgi(struct client_state * csp,
                                                const char * path)
{
   const struct cgi_dispatcher *d;
   struct map *param_list;
   struct http_response *rsp;
   char *query_args_start;
   char *path_copy;
   jb_err err;

   if (NULL == (path_copy = strdup(path)))
   {
      return cgi_error_memory();
   }

   query_args_start = path_copy;
   while (*query_args_start && *query_args_start != '?')
   {
      query_args_start++;
   }
   if (*query_args_start == '?')
   {
      *query_args_start++ = '\0';
   }

   if (NULL == (param_list = parse_cgi_parameters(query_args_start)))
   {
      free(path_copy);
      return cgi_error_memory();
   }


   /*
    * At this point:
    * path_copy        = CGI call name
    * param_list       = CGI params, as map
    */

   /* Get mem for response or fail*/
   if (NULL == (rsp = alloc_http_response()))
   {
      free(path_copy);
      free_map(param_list);
      return cgi_error_memory();
   }

   log_error(LOG_LEVEL_GPC, "%s%s cgi call", csp->http->hostport, csp->http->path);
   log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 200 3", 
                            csp->ip_addr_str, csp->http->cmd); 

   /* Find and start the right CGI function*/
   for (d = cgi_dispatchers; FOREVER; d++)
   {
      if ((d->name == NULL) || (strcmp(path_copy, d->name) == 0))
      {
         err = (d->handler)(csp, rsp, param_list);
         free(path_copy);
         free_map(param_list);
         if (err == JB_ERR_CGI_PARAMS)
         {
            err = cgi_error_bad_param(csp, rsp);
         }
         if (!err)
         {
            /* It worked */
            return finish_http_response(rsp);
         }
         else
         {
            /* Error in handler, probably out-of-memory */
            free_http_response(rsp);
            return cgi_error_memory();
         }
      }
   }
}


/*********************************************************************
 *
 * Function    :  parse_cgi_parameters
 *
 * Description :  Parse a URL-encoded argument string into name/value
 *                pairs and store them in a struct map list.
 *
 * Parameters  :
 *          1  :  string = string to be parsed.  Will be trashed.
 *
 * Returns     :  pointer to param list, or NULL if out of memory.
 *
 *********************************************************************/
static struct map *parse_cgi_parameters(char *argstring)
{
   char *p;
   char *vector[BUFFER_SIZE];
   int pairs, i;
   struct map *cgi_params;

   if (NULL == (cgi_params = new_map()))
   {
      return NULL;
   }

   pairs = ssplit(argstring, "&", vector, SZ(vector), 1, 1);

   for (i = 0; i < pairs; i++)
   {
      if ((NULL != (p = strchr(vector[i], '='))) && (*(p+1) != '\0'))
      {
         *p = '\0';
         if (map(cgi_params, url_decode(vector[i]), 0, url_decode(++p), 0))
         {
            free_map(cgi_params);
            return NULL;
         }
      }
   }

   return cgi_params;

}


/*********************************************************************
 *
 * Function    :  error_response
 *
 * Description :  returns an http_response that explains the reason
 *                why a request failed.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  templatename = Which template should be used for the answer
 *          3  :  sys_err = system error number
 *
 * Returns     :  A http_response.  If we run out of memory, this
 *                will be cgi_error_memory().
 *
 *********************************************************************/
struct http_response *error_response(struct client_state *csp,
                                     const char *templatename,
                                     int sys_err)
{
   jb_err err;
   struct http_response *rsp;
   struct map * exports = default_exports(csp, NULL);
   if (exports == NULL)
   {
      return cgi_error_memory();
   }

   if (NULL == (rsp = alloc_http_response()))
   {
      free_map(exports);
      return cgi_error_memory();
   }

   err = map(exports, "host", 1, html_encode(csp->http->host), 0);
   if (!err) err = map(exports, "hostport", 1, html_encode(csp->http->hostport), 0);
   if (!err) err = map(exports, "path", 1, html_encode(csp->http->path), 0);
   if (!err) err = map(exports, "error", 1, html_encode_and_free_original(safe_strerror(sys_err)), 0);
   if (!err)
   {
     err = map(exports, "host-ip", 1, html_encode(csp->http->host_ip_addr_str), 0);
     if (err)
     {
       /* Some failures, like "404 no such domain", don't have an IP address. */
       err = map(exports, "host-ip", 1, html_encode(csp->http->host), 0);
     }
   }


   if (err)
   {
      free_map(exports);
      free_http_response(rsp);
      return cgi_error_memory();
   }

   if (!strcmp(templatename, "no-such-domain"))
   {
      rsp->status = strdup("404 No such domain");
      if (rsp->status == NULL)
      {
         free_map(exports);
         free_http_response(rsp);
         return cgi_error_memory();
      }
   }
   else if (!strcmp(templatename, "connect-failed"))
   {
      rsp->status = strdup("503 Connect failed");
      if (rsp->status == NULL)
      {
         free_map(exports);
         free_http_response(rsp);
         return cgi_error_memory();
      }
   }

   err = template_fill_for_cgi(csp, templatename, exports, rsp);
   if (err)
   {
      free_http_response(rsp);
      return cgi_error_memory();
   }

   return finish_http_response(rsp);
}


/*********************************************************************
 *
 * Function    :  cgi_init_error_messages
 *
 * Description :  Call at the start of the program to initialize
 *                the error message used by cgi_error_memory().
 *
 * Parameters  :  N/A
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void cgi_init_error_messages(void)
{
   memset(cgi_error_memory_response, '\0', sizeof(*cgi_error_memory_response));
   cgi_error_memory_response->head =
      "HTTP/1.0 500 Internal JunkBuster Proxy Error\r\n"
      "Content-Type: text/html\r\n"
      "\r\n";
   cgi_error_memory_response->body =
      "<html>\r\n"
      "<head><title>500 Internal JunkBuster Proxy Error</title></head>\r\n"
      "<body>\r\n"
      "<h1>500 Internal JunkBuster Proxy Error</h1>\r\n"
      "<p>JunkBuster <b>ran out of memory</b> while processing your request.</p>\r\n"
      "<p>Please contact your proxy administrator, or try again later</p>\r\n"
      "</body>\r\n"
      "</html>\r\n";

   cgi_error_memory_response->head_length =
      strlen(cgi_error_memory_response->head);
   cgi_error_memory_response->content_length =
      strlen(cgi_error_memory_response->body);
}


/*********************************************************************
 *
 * Function    :  cgi_error_memory
 *
 * Description :  Called if a CGI function runs out of memory.
 *                Returns a statically-allocated error response.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  template_name = Name of template that could not
 *                                be loaded.
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
struct http_response *cgi_error_memory(void)
{
   /* assert that it's been initialized. */
   assert(cgi_error_memory_response->head);

   return cgi_error_memory_response;
}


/*********************************************************************
 *
 * Function    :  cgi_error_no_template
 *
 * Description :  Almost-CGI function that is called if a templae
 *                cannot be loaded.  Note this is not a true CGI,
 *                it takes a template name rather than a map of 
 *                parameters.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  template_name = Name of template that could not
 *                                be loaded.
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
jb_err cgi_error_no_template(struct client_state *csp,
                             struct http_response *rsp,
                             const char *template_name)
{
   static const char status[] =
      "500 Internal JunkBuster Proxy Error";
   static const char body_prefix[] =
      "<html>\r\n"
      "<head><title>500 Internal JunkBuster Proxy Error</title></head>\r\n"
      "<body>\r\n"
      "<h1>500 Internal JunkBuster Proxy Error</h1>\r\n"
      "<p>JunkBuster encountered an error while processing your request:</p>\r\n"
      "<p><b>Could not load template file <code>";
   static const char body_suffix[] =
      "</code></b></p>\r\n"
      "<p>Please contact your proxy administrator.</p>\r\n"
      "<p>If you are the proxy administrator, please put the required file "
      "in the <code><i>(confdir)</i>/templates</code> directory.  The "
      "location of the <code><i>(confdir)</i></code> directory "
      "is specified in the main JunkBuster <code>config</code> "
      "file.  (It's typically the JunkBuster install directory"
#ifndef _WIN32
      ", or <code>/etc/junkbuster/</code>"
#endif /* ndef _WIN32 */
      ").</p>\r\n"
      "</body>\r\n"
      "</html>\r\n";

   assert(csp);
   assert(rsp);
   assert(template_name);

   /* Reset rsp, if needed */
   freez(rsp->status);
   freez(rsp->head);
   freez(rsp->body);
   rsp->content_length = 0;
   rsp->head_length = 0;
   rsp->is_static = 0;

   rsp->body = malloc(strlen(body_prefix) + strlen(template_name) + strlen(body_suffix) + 1);
   if (rsp->body == NULL)
   {
      return JB_ERR_MEMORY;
   }
   strcpy(rsp->body, body_prefix);
   strcat(rsp->body, template_name);
   strcat(rsp->body, body_suffix);

   rsp->status = strdup(status);
   if (rsp->body == NULL)
   {
      return JB_ERR_MEMORY;
   }

   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  cgi_error_bad_param
 *
 * Description :  CGI function that is called if the parameters
 *                (query string) for a CGI were wrong.
 *               
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *
 * CGI Parameters : none
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
jb_err cgi_error_bad_param(struct client_state *csp,
                           struct http_response *rsp)
{
   struct map *exports;

   assert(csp);
   assert(rsp);

   if (NULL == (exports = default_exports(csp, NULL)))
   {
      return JB_ERR_MEMORY;
   }

   return template_fill_for_cgi(csp, "cgi-error-bad-param", exports, rsp);
}


/*********************************************************************
 *
 * Function    :  get_http_time
 *
 * Description :  Get the time in a format suitable for use in a
 *                HTTP header - e.g.:
 *                "Sun, 06 Nov 1994 08:49:37 GMT"
 *
 * Parameters  :  
 *          1  :  time_offset = Time returned will be current time
 *                              plus this number of seconds.
 *          2  :  buf = Destination for result.  Must be long enough
 *                      to hold 29 characters plus a trailing zero.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void get_http_time(int time_offset, char *buf)
{
   static const char day_names[7][4] =
      { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
   static const char month_names[12][4] =
      { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

   struct tm *t;
   time_t current_time;

   assert(buf);

   time(&current_time); /* get current time */

   current_time += time_offset;

   /* get and save the gmt */
   {
#ifdef HAVE_GMTIME_R
      struct tm dummy;
      t = gmtime_r(&current_time, &dummy);
#else
      t = gmtime(&current_time);
#endif
   }

   /* Format: "Sun, 06 Nov 1994 08:49:37 GMT" */
   snprintf(buf, 30,
      "%s, %02d %s %4d %02d:%02d:%02d GMT",
      day_names[t->tm_wday],
      t->tm_mday,
      month_names[t->tm_mon],
      t->tm_year + 1900,
      t->tm_hour,
      t->tm_min,
      t->tm_sec
      );

}


/*********************************************************************
 *
 * Function    :  finish_http_response
 *
 * Description :  Fill in the missing headers in an http response,
 *                and flatten the headers to an http head.
 *
 * Parameters  :
 *          1  :  rsp = pointer to http_response to be processed
 *
 * Returns     :  A http_response, usually the rsp parameter.
 *                On error, free()s rsp and returns cgi_error_memory()
 *
 *********************************************************************/
struct http_response *finish_http_response(struct http_response *rsp)
{
   char buf[BUFFER_SIZE];
   jb_err err;

   /* Special case - do NOT change this statically allocated response,
    * which is ready for output anyway.
    */
   if (rsp == cgi_error_memory_response)
   {
      return rsp;
   }

   /* 
    * Fill in the HTTP Status
    */
   sprintf(buf, "HTTP/1.0 %s", rsp->status ? rsp->status : "200 OK");
   err = enlist_first(rsp->headers, buf);

   /* 
    * Set the Content-Length
    */
   if (rsp->content_length == 0)
   {
      rsp->content_length = rsp->body ? strlen(rsp->body) : 0;
   }
   if (!err)
   {
      sprintf(buf, "Content-Length: %d", rsp->content_length);
      err = enlist(rsp->headers, buf);
   }

   /* 
    * Fill in the default headers:
    *
    * Content-Type: default to text/html if not already specified.
    * Date: set to current date/time.
    * Last-Modified: set to date/time the page was last changed.
    * Expires: set to date/time page next needs reloading.
    * Cache-Control: set to "no-cache" if applicable.
    * 
    * See http://www.w3.org/Protocols/rfc2068/rfc2068
    */
   if (!err) err = enlist_unique(rsp->headers, "Content-Type: text/html", 13);

   if (rsp->is_static)
   {
      /*
       * Set Expires to about 10 min into the future so it'll get reloaded
       * occasionally, e.g. if IJB gets upgraded.
       */

      if (!err)
      {
         get_http_time(0, buf);
         err = enlist_unique_header(rsp->headers, "Date", buf);
      }

      /* Some date in the past. */
      if (!err) err = enlist_unique_header(rsp->headers, "Last-Modified", "Sat, 17 Jun 2000 12:00:00 GMT");

      if (!err)
      {
         get_http_time(10 * 60, buf); /* 10 * 60sec = 10 minutes */
         err = enlist_unique_header(rsp->headers, "Expires", buf);
      }
   }
   else
   {
      /*
       * Compliant browsers should not cache this due to the "Cache-Control"
       * setting.  However, to be certain, we also set both "Last-Modified"
       * and "Expires" to the current time.
       */
      if (!err) err = enlist_unique_header(rsp->headers, "Cache-Control", "no-cache");

      get_http_time(0, buf);
      if (!err) err = enlist_unique_header(rsp->headers, "Date", buf);
      if (!err) err = enlist_unique_header(rsp->headers, "Last-Modified", buf);
      if (!err) err = enlist_unique_header(rsp->headers, "Expires", buf);
   }


   /* 
    * Write the head
    */
   if (err || (NULL == (rsp->head = list_to_text(rsp->headers))))
   {
      free_http_response(rsp);
      return cgi_error_memory();
   }
   rsp->head_length = strlen(rsp->head);

   return rsp;

}


/*********************************************************************
 *
 * Function    :  alloc_http_response
 *
 * Description :  Allocates a new http_response structure.
 *
 * Parameters  :  N/A
 *
 * Returns     :  pointer to a new http_response, or NULL.
 *
 *********************************************************************/
struct http_response *alloc_http_response(void)
{
   return (struct http_response *) zalloc(sizeof(struct http_response));

}


/*********************************************************************
 *
 * Function    :  free_http_response
 *
 * Description :  Free the memory occupied by an http_response
 *                and its depandant structures.
 *
 * Parameters  :
 *          1  :  rsp = pointer to http_response to be freed
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void free_http_response(struct http_response *rsp)
{
   /*
    * Must special case cgi_error_memory_response, which is never freed.
    */
   if (rsp && (rsp != cgi_error_memory_response))
   {
      freez(rsp->status);
      freez(rsp->head);
      freez(rsp->body);
      destroy_list(rsp->headers);
      free(rsp);
   }

}


/*********************************************************************
 *
 * Function    :  template_load
 *
 * Description :  CGI support function that loads a given HTML
 *                template from the confdir, ignoring comment
 *                lines. 
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  template_ptr = Destination for pointer to loaded
 *                               template text.
 *           3 :  template = name of the HTML template to be used
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.  
 *                JB_ERR_FILE if the template file cannot be read
 *
 *********************************************************************/
jb_err template_load(struct client_state *csp, char ** template_ptr, 
                     const char *templatename)
{
   char *templates_dir_path;
   char *full_path;
   char *file_buffer;
   FILE *fp;
   char buf[BUFFER_SIZE];

   assert(csp);
   assert(template_ptr);
   assert(templatename);

   *template_ptr = NULL;

   /*
    * Open template file or fail
    */

   templates_dir_path = make_path(csp->config->confdir, "templates");
   if (templates_dir_path == NULL)
   {
      return JB_ERR_MEMORY;
   }

   full_path = make_path(templates_dir_path, templatename);
   free(templates_dir_path);
   if (full_path == NULL)
   {
      return JB_ERR_MEMORY;
   }

   file_buffer = strdup("");
   if (file_buffer == NULL)
   {
      free(full_path);
      return JB_ERR_MEMORY;
   }

   if (NULL == (fp = fopen(full_path, "r")))
   {
      log_error(LOG_LEVEL_ERROR, "Cannot open template file %s: %E", full_path);
      free(full_path);
      free(file_buffer);
      return JB_ERR_FILE;
   }
   free(full_path);

   /* 
    * Read the file, ignoring comments.
    *
    * FIXME: The comment handling could break with lines >BUFFER_SIZE long.
    *        This is unlikely in practise.
    */
   while (fgets(buf, BUFFER_SIZE, fp))
   {
      /* skip lines starting with '#' */
      if(*buf == '#')
      {
         continue;
      }

      if (string_append(&file_buffer, buf))
      {
         fclose(fp);
         return JB_ERR_MEMORY;
      }
   }
   fclose(fp);

   *template_ptr = file_buffer;

   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  template_fill
 *
 * Description :  CGI support function that fills in a pre-loaded
 *                HTML template by replacing @name@ with value using
 *                pcrs, for each item in the output map.
 *
 *                Note that a leading '$' charachter in the export map's
 *                values will be stripped and toggle on backreference
 *                interpretation.
 *
 * Parameters  :
 *           1 :  template_ptr = IN: Template to be filled out.
 *                                   Will be free()d.
 *                               OUT: Filled out template.
 *                                    Caller must free().
 *           2 :  exports = map with fill in symbol -> name pairs
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error
 *
 *********************************************************************/
jb_err template_fill(char **template_ptr, const struct map *exports)
{
   struct map_entry *m;
   pcrs_job *job;
   char buf[BUFFER_SIZE];
   char *tmp_out_buffer;
   char *file_buffer;
   size_t  size;
   int error;
   const char *flags;

   assert(template_ptr);
   assert(*template_ptr);
   assert(exports);

   file_buffer = *template_ptr;
   size = strlen(file_buffer) + 1;

   /* 
    * Assemble pcrs joblist from exports map
    */
   for (m = exports->first; m != NULL; m = m->next)
   {
      if (*m->name == '$')
      {
         /*
          * First character of name is '$', so remove this flag
          * character and allow backreferences ($1 etc) in the
          * "replace with" text.
          */
         snprintf(buf, BUFFER_SIZE, "%s", m->name + 1);
         flags = "sigU";
      }
      else
      {
         /*
          * Treat the "replace with" text as a literal string - 
          * no quoting needed, no backreferences allowed.
          * ("Trivial" ['T'] flag).
          */
         flags = "sigTU";

         /* Enclose name in @@ */
         snprintf(buf, BUFFER_SIZE, "@%s@", m->name);
      }


      log_error(LOG_LEVEL_CGI, "Substituting: s/%s/%s/%s", buf, m->value, flags);

      /* Make and run job. */
      job = pcrs_compile(buf, m->value, flags,  &error);
      if (job == NULL) 
      {
         if (error == PCRS_ERR_NOMEM)
         {
            free(file_buffer);
            *template_ptr = NULL;
            return JB_ERR_MEMORY;
         }
         else
         {
            log_error(LOG_LEVEL_ERROR, "Error compiling template fill job %s: %d", m->name, error);
            /* Hope it wasn't important and silently ignore the invalid job */
         }
      }
      else
      {
         pcrs_execute(job, file_buffer, size, &tmp_out_buffer, &size);
         free(file_buffer);
         pcrs_free_job(job);
         if (NULL == tmp_out_buffer)
         {
            *template_ptr = NULL;
            return JB_ERR_MEMORY;
         }
         file_buffer = tmp_out_buffer;
      }
   }

   /*
    * Return
    */
   *template_ptr = file_buffer;
   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  template_fill_for_cgi
 *
 * Description :  CGI support function that loads a HTML template
 *                and fills it in.  Handles file-not-found errors
 *                by sending a HTML error message.  For convenience,
 *                this function also frees the passed "exports" map.
 *
 * Parameters  :
 *           1 :  csp = Client state
 *           2 :  templatename = name of the HTML template to be used
 *           3 :  exports = map with fill in symbol -> name pairs.
 *                          Will be freed by this function.
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error
 *
 *********************************************************************/
jb_err template_fill_for_cgi(struct client_state *csp,
                             const char *templatename,
                             struct map *exports,
                             struct http_response *rsp)
{
   jb_err err;
   
   assert(csp);
   assert(templatename);
   assert(exports);
   assert(rsp);

   err = template_load(csp, &rsp->body, templatename);
   if (err == JB_ERR_FILE)
   {
      free_map(exports);
      return cgi_error_no_template(csp, rsp, templatename);
   }
   else if (err)
   {
      free_map(exports);
      return err; /* JB_ERR_MEMORY */
   }
   err = template_fill(&rsp->body, exports);
   free_map(exports);
   return err;
}


/*********************************************************************
 *
 * Function    :  default_exports
 *
 * Description :  returns a struct map list that contains exports
 *                which are common to all CGI functions.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  caller = name of CGI who calls us and which should
 *                         be excluded from the generated menu. May be
 *                         NULL.
 * Returns     :  NULL if no memory, else a new map.  Caller frees.
 *
 *********************************************************************/
struct map *default_exports(const struct client_state *csp, const char *caller)
{
   char buf[20];
   jb_err err;
   struct map * exports;
   int local_help_exists = 0;

   assert(csp);

   exports = new_map();
   if (exports == NULL)
   {
      return NULL;
   }

   err = map(exports, "version", 1, html_encode(VERSION), 0);
   if (!err) err = map(exports, "my-ip-address", 1, html_encode(csp->my_ip_addr_str ? csp->my_ip_addr_str : "unknown"), 0);
   if (!err) err = map(exports, "my-hostname",   1, html_encode(csp->my_hostname ? csp->my_hostname : "unknown"), 0);
   if (!err) err = map(exports, "homepage",      1, html_encode(HOME_PAGE_URL), 0);
   if (!err) err = map(exports, "default-cgi",   1, html_encode(CGI_PREFIX), 0);
   if (!err) err = map(exports, "menu",          1, make_menu(caller), 0);
   if (!err) err = map(exports, "code-status",   1, CODE_STATUS, 1);
   if (!err) err = map_conditional(exports, "enabled-display", g_bToggleIJB);

   snprintf(buf, 20, "%d", csp->config->hport);
   if (!err) err = map(exports, "my-port", 1, buf, 1);

   if(!strcmp(CODE_STATUS, "stable"))
   {
      if (!err) err = map_block_killer(exports, "unstable");
   }

   if (csp->config->admin_address != NULL)
   {
      if (!err) err = map(exports, "admin-address", 1, html_encode(csp->config->admin_address), 0);
      local_help_exists = 1;
   }
   else
   {
      if (!err) err = map_block_killer(exports, "have-adminaddr-info");
   }

   if (csp->config->proxy_info_url != NULL)
   {
      if (!err) err = map(exports, "proxy-info-url", 1, html_encode(csp->config->proxy_info_url), 0);
      local_help_exists = 1;
   }
   else
   {
      if (!err) err = map_block_killer(exports, "have-proxy-info");
   }

   if (local_help_exists == 0)
   {
      if (!err) err = map_block_killer(exports, "have-help-info");
   }

   if (err)
   {
      free_map(exports);
      return NULL;
   }

   return exports;
}


/*********************************************************************
 *
 * Function    :  map_block_killer
 *
 * Description :  Convenience function.
 *                Adds a "killer" for the conditional HTML-template
 *                block <name>, i.e. a substitution of the regex
 *                "if-<name>-start.*if-<name>-end" to the given
 *                export list.
 *
 * Parameters  :  
 *          1  :  exports = map to extend
 *          2  :  name = name of conditional block
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
jb_err map_block_killer(struct map *exports, const char *name)
{
   char buf[1000]; /* Will do, since the names are hardwired */

   assert(exports);
   assert(name);
   assert(strlen(name) < 490);

   snprintf(buf, 1000, "if-%s-start.*if-%s-end", name, name);
   return map(exports, buf, 1, "", 1);
}


/*********************************************************************
 *
 * Function    :  map_block_keep
 *
 * Description :  Convenience function.  Removes the markers used
 *                by map-block-killer, to save a few bytes.
 *                i.e. removes "@if-<name>-start@" and "@if-<name>-end@"
 *
 * Parameters  :  
 *          1  :  exports = map to extend
 *          2  :  name = name of conditional block
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
jb_err map_block_keep(struct map *exports, const char *name)
{
   jb_err err;
   char buf[500]; /* Will do, since the names are hardwired */

   assert(exports);
   assert(name);
   assert(strlen(name) < 490);

   snprintf(buf, 500, "if-%s-start", name);
   err = map(exports, buf, 1, "", 1);

   if (err)
   {
      return err;
   }

   snprintf(buf, 500, "if-%s-end", name);
   return map(exports, buf, 1, "", 1);
}


/*********************************************************************
 *
 * Function    :  map_conditional
 *
 * Description :  Convenience function.
 *                Adds an "if-then-else" for the conditional HTML-template
 *                block <name>, i.e. a substitution of the form:
 *                @if-<name>-then@
 *                   True text
 *                @else-not-<name>@
 *                   False text
 *                @endif-<name>@
 *
 *                The control structure and one of the alternatives
 *                will be hidden.
 *
 * Parameters  :  
 *          1  :  exports = map to extend
 *          2  :  name = name of conditional block
 *          3  :  choose_first = nonzero for first, zero for second.
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.  
 *
 *********************************************************************/
jb_err map_conditional(struct map *exports, const char *name, int choose_first)
{
   char buf[1000]; /* Will do, since the names are hardwired */
   jb_err err;

   assert(exports);
   assert(name);
   assert(strlen(name) < 480);

   snprintf(buf, 1000, (choose_first
      ? "else-not-%s@.*@endif-%s"
      : "if-%s-then@.*@else-not-%s"),
      name, name);

   err = map(exports, buf, 1, "", 1);
   if (err)
   {
      return err;
   }

   snprintf(buf, 1000, (choose_first ? "if-%s-then" : "endif-%s"), name);
   return map(exports, buf, 1, "", 1);
}


/*********************************************************************
 *
 * Function    :  make_menu
 *
 * Description :  Returns an HTML-formatted menu of the available 
 *                unhidden CGIs, excluding the one given in <self>.
 *
 * Parameters  :  self = name of CGI to leave out, can be NULL
 *
 * Returns     :  menu string, or NULL on out-of-memory error.
 *
 *********************************************************************/
char *make_menu(const char *self)
{
   const struct cgi_dispatcher *d;
   char *result = strdup("");

   if (self == NULL)
   {
      self = "NO-SUCH-CGI!";
   }

   /* List available unhidden CGI's and export as "other-cgis" */
   for (d = cgi_dispatchers; d->name; d++)
   {
      if (d->description && strcmp(d->name, self))
      {
         string_append(&result, "<li><a href=\"" CGI_PREFIX);
         string_append(&result, d->name);
         string_append(&result, "\">");
         string_append(&result, d->description);
         string_append(&result, "</a></li>\n");
      }
   }

   return result;
}


/*********************************************************************
 *
 * Function    :  dump_map
 *
 * Description :  HTML-dump a map for debugging
 *
 * Parameters  :
 *          1  :  the_map = map to dump
 *
 * Returns     :  string with HTML
 *
 *********************************************************************/
char *dump_map(const struct map *the_map)
{
   struct map_entry *cur_entry;
   char *ret = strdup("");

   string_append(&ret, "<table>\n");

   for (cur_entry = the_map->first;
        (cur_entry != NULL) && (ret != NULL);
        cur_entry = cur_entry->next)
   {
      string_append(&ret, "<tr><td><b>");
      string_join  (&ret, html_encode(cur_entry->name));
      string_append(&ret, "</b></td><td>");
      string_join  (&ret, html_encode(cur_entry->value));
      string_append(&ret, "</td></tr>\n");
   }

   string_append(&ret, "</table>\n");
   return ret;
}


/*
  Local Variables:
  tab-width: 3
  end:
*/
