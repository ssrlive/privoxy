const char jcc_rcs[] = "$Id: jcc.c,v 1.38 2001/09/16 13:01:46 jongfoster Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/jcc.c,v $
 *
 * Purpose     :  Main file.  Contains main() method, main loop, and 
 *                the main connection-handling function.
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
 *    $Log: jcc.c,v $
 *    Revision 1.38  2001/09/16 13:01:46  jongfoster
 *    Removing redundant function call that zeroed zalloc()'d memory.
 *
 *    Revision 1.37  2001/09/10 11:12:24  oes
 *    Deleted unused variable
 *
 *    Revision 1.36  2001/09/10 10:56:15  oes
 *    Silenced compiler warnings
 *
 *    Revision 1.35  2001/07/31 14:44:22  oes
 *    Deleted unused size parameter from filter_popups()
 *
 *    Revision 1.34  2001/07/30 22:08:36  jongfoster
 *    Tidying up #defines:
 *    - All feature #defines are now of the form FEATURE_xxx
 *    - Permanently turned off WIN_GUI_EDIT
 *    - Permanently turned on WEBDAV and SPLIT_PROXY_ARGS
 *
 *    Revision 1.33  2001/07/29 19:32:00  jongfoster
 *    Renaming _main() [mingw32 only] to real_main(), for ANSI compliance.
 *
 *    Revision 1.32  2001/07/29 18:47:05  jongfoster
 *    Adding missing #include "loadcfg.h"
 *
 *    Revision 1.31  2001/07/29 12:17:48  oes
 *    Applied pthread fix by Paul Lieverse
 *
 *    Revision 1.30  2001/07/25 22:57:13  jongfoster
 *    __BEOS__ no longer overrides FEATURE_PTHREAD.
 *    This is because FEATURE_PTHREAD will soon be widely used, so I
 *    want to keep it simple.
 *
 *    Revision 1.29  2001/07/24 12:47:06  oes
 *    Applied BeOS support update by Eugenia
 *
 *    Revision 1.28  2001/07/23 13:26:12  oes
 *    Fixed bug in popup-killing for the first read that caused binary garbage to be sent between headers and body
 *
 *    Revision 1.27  2001/07/19 19:09:47  haroon
 *    - Added code to take care of the situation where while processing the first
 *      server response (which includes the server header), after finding the end
 *      of the headers we were not looking past the end of the headers for
 *      content modification. I enabled it for filter_popups.
 *      Someone else should look to see if other similar operations should be
 *      done to the discarded portion of the buffer.
 *
 *      Note 2001/07/20: No, the other content modification mechanisms will process
 *                       the whole iob later anyway. --oes
 *
 *    Revision 1.26  2001/07/18 12:31:36  oes
 *    cosmetics
 *
 *    Revision 1.25  2001/07/15 19:43:49  jongfoster
 *    Supports POSIX threads.
 *    Also removed some unused #includes.
 *
 *    Revision 1.24  2001/07/13 14:00:40  oes
 *     - Generic content modification scheme:
 *       Each feature has its own applicability flag that is set
 *       from csp->action->flags.
 *       Replaced the "filtering" int flag , by a function pointer
 *       "content_filter" to the function that will do the content
 *       modification. If it is != NULL, the document will be buffered
 *       and processed through *content_filter, which must set
 *       csp->content_length and return a modified copy of the body
 *       or return NULL (on failiure).
 *     - Changed csp->is_text to the more generic bitmap csp->content_type
 *       which can currently take the valued CT_TEXT or CT_GIF
 *     - Reformatting etc
 *     - Removed all #ifdef PCRS
 *
 *    Revision 1.23  2001/07/02 02:28:25  iwanttokeepanon
 *    Added "#ifdef ACL_FILES" conditional compilation to line 1291 to exclude
 *    the `block_acl' call.  This prevents a compilation error when the user
 *    does not wish to use the "ACL" feature.
 *
 *    Revision 1.22  2001/06/29 21:45:41  oes
 *    Indentation, CRLF->LF, Tab-> Space
 *
 *    Revision 1.21  2001/06/29 13:29:36  oes
 *    - Cleaned up, improved comments
 *    - Unified all possible interceptors (CGI,
 *      block, trust, fast_redirect) in one
 *      place, with one (CGI) answer generation
 *      mechansim. Much clearer now.
 *    - Removed the GIF image generation, which
 *      is now done in filters.c:block_url()
 *    - Made error conditions like domain lookup
 *      failiure or (various) problems while talking
 *      to the server use cgi.c:error_response()
 *      instead of generating HTML/HTTP in chat() (yuck!)
 *    - Removed logentry from cancelled commit
 *
 *    Revision 1.20  2001/06/09 10:55:28  jongfoster
 *    Changing BUFSIZ ==> BUFFER_SIZE
 *
 *    Revision 1.19  2001/06/07 23:12:52  jongfoster
 *    Replacing function pointer in struct gateway with a directly
 *    called function forwarded_connect().
 *    Replacing struct gateway with struct forward_spec
 *
 *    Revision 1.18  2001/06/03 19:12:16  oes
 *    introduced new cgi handling
 *
 *    Revision 1.17  2001/06/01 20:07:23  jongfoster
 *    Now uses action +image-blocker{} rather than config->tinygif
 *
 *    Revision 1.16  2001/06/01 18:49:17  jongfoster
 *    Replaced "list_share" with "list" - the tiny memory gain was not
 *    worth the extra complexity.
 *
 *    Revision 1.15  2001/05/31 21:24:47  jongfoster
 *    Changed "permission" to "action" throughout.
 *    Removed DEFAULT_USER_AGENT - it must now be specified manually.
 *    Moved vanilla wafer check into chat(), since we must now
 *    decide whether or not to add it based on the URL.
 *
 *    Revision 1.14  2001/05/29 20:14:01  joergs
 *    AmigaOS bugfix: PCRS needs a lot of stack, stacksize for child threads
 *    increased.
 *
 *    Revision 1.13  2001/05/29 09:50:24  jongfoster
 *    Unified blocklist/imagelist/permissionslist.
 *    File format is still under discussion, but the internal changes
 *    are (mostly) done.
 *
 *    Also modified interceptor behaviour:
 *    - We now intercept all URLs beginning with one of the following
 *      prefixes (and *only* these prefixes):
 *        * http://i.j.b/
 *        * http://ijbswa.sf.net/config/
 *        * http://ijbswa.sourceforge.net/config/
 *    - New interceptors "home page" - go to http://i.j.b/ to see it.
 *    - Internal changes so that intercepted and fast redirect pages
 *      are not replaced with an image.
 *    - Interceptors now have the option to send a binary page direct
 *      to the client. (i.e. ijb-send-banner uses this)
 *    - Implemented show-url-info interceptor.  (Which is why I needed
 *      the above interceptors changes - a typical URL is
 *      "http://i.j.b/show-url-info?url=www.somesite.com/banner.gif".
 *      The previous mechanism would not have intercepted that, and
 *      if it had been intercepted then it then it would have replaced
 *      it with an image.)
 *
 *    Revision 1.12  2001/05/27 22:17:04  oes
 *
 *    - re_process_buffer no longer writes the modified buffer
 *      to the client, which was very ugly. It now returns the
 *      buffer, which it is then written by chat.
 *
 *    - content_length now adjusts the Content-Length: header
 *      for modified documents rather than crunch()ing it.
 *      (Length info in csp->content_length, which is 0 for
 *      unmodified documents)
 *
 *    - For this to work, sed() is called twice when filtering.
 *
 *    Revision 1.11  2001/05/26 17:27:53  jongfoster
 *    Added support for CLF and fixed LOG_LEVEL_LOG.
 *    Also did CRLF->LF fix of my previous patch.
 *
 *    Revision 1.10  2001/05/26 15:26:15  jongfoster
 *    ACL feature now provides more security by immediately dropping
 *    connections from untrusted hosts.
 *
 *    Revision 1.9  2001/05/26 00:28:36  jongfoster
 *    Automatic reloading of config file.
 *    Removed obsolete SIGHUP support (Unix) and Reload menu option (Win32).
 *    Most of the global variables have been moved to a new
 *    struct configuration_spec, accessed through csp->config->globalname
 *    Most of the globals remaining are used by the Win32 GUI.
 *
 *    Revision 1.8  2001/05/25 22:43:18  jongfoster
 *    Fixing minor memory leak and buffer overflow.
 *
 *    Revision 1.7  2001/05/25 22:34:30  jongfoster
 *    Hard tabs->Spaces
 *
 *    Revision 1.6  2001/05/23 00:13:58  joergs
 *    AmigaOS support fixed.
 *
 *    Revision 1.5  2001/05/22 18:46:04  oes
 *
 *    - Enabled filtering banners by size rather than URL
 *      by adding patterns that replace all standard banner
 *      sizes with the "Junkbuster" gif to the re_filterfile
 *
 *    - Enabled filtering WebBugs by providing a pattern
 *      which kills all 1x1 images
 *
 *    - Added support for PCRE_UNGREEDY behaviour to pcrs,
 *      which is selected by the (nonstandard and therefore
 *      capital) letter 'U' in the option string.
 *      It causes the quantifiers to be ungreedy by default.
 *      Appending a ? turns back to greedy (!).
 *
 *    - Added a new interceptor ijb-send-banner, which
 *      sends back the "Junkbuster" gif. Without imagelist or
 *      MSIE detection support, or if tinygif = 1, or the
 *      URL isn't recognized as an imageurl, a lame HTML
 *      explanation is sent instead.
 *
 *    - Added new feature, which permits blocking remote
 *      script redirects and firing back a local redirect
 *      to the browser.
 *      The feature is conditionally compiled, i.e. it
 *      can be disabled with --disable-fast-redirects,
 *      plus it must be activated by a "fast-redirects"
 *      line in the config file, has its own log level
 *      and of course wants to be displayed by show-proxy-args
 *      Note: Boy, all the #ifdefs in 1001 locations and
 *      all the fumbling with configure.in and acconfig.h
 *      were *way* more work than the feature itself :-(
 *
 *    - Because a generic redirect template was needed for
 *      this, tinygif = 3 now uses the same.
 *
 *    - Moved GIFs, and other static HTTP response templates
 *      to project.h
 *
 *    - Some minor fixes
 *
 *    - Removed some >400 CRs again (Jon, you really worked
 *      a lot! ;-)
 *
 *    Revision 1.4  2001/05/21 19:34:01  jongfoster
 *    Made failure to bind() a fatal error.
 *
 *    Revision 1.3  2001/05/20 01:21:20  jongfoster
 *    Version 2.9.4 checkin.
 *    - Merged popupfile and cookiefile, and added control over PCRS
 *      filtering, in new "permissionsfile".
 *    - Implemented LOG_LEVEL_FATAL, so that if there is a configuration
 *      file error you now get a message box (in the Win32 GUI) rather
 *      than the program exiting with no explanation.
 *    - Made killpopup use the PCRS MIME-type checking and HTTP-header
 *      skipping.
 *    - Removed tabs from "config"
 *    - Moved duplicated url parsing code in "loaders.c" to a new funcition.
 *    - Bumped up version number.
 *
 *    Revision 1.2  2001/05/17 22:34:44  oes
 *     - Added hint on GIF char array generation to jcc.c
 *     - Cleaned CRLF's from the sources and related files
 *     - Repaired logging for REF and FRC
 *
 *    Revision 1.1.1.1  2001/05/15 13:58:56  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#ifdef FEATURE_PTHREAD
#include <pthread.h>
#endif /* def FEATURE_PTHREAD */

#ifdef _WIN32
# ifndef FEATURE_PTHREAD
#  include <windows.h>
#  include <process.h>
# endif /* ndef FEATURE_PTHREAD */

# include "win32.h"
# ifndef _WIN_CONSOLE
#  include "w32log.h"
# endif /* ndef _WIN_CONSOLE */

#else /* ifndef _WIN32 */

# include <unistd.h>
# include <sys/time.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <signal.h>

# ifdef __BEOS__
#  include <socket.h>  /* BeOS has select() for sockets only. */
#  include <OS.h>      /* declarations for threads and stuff. */
# endif

# ifndef FD_ZERO
#  include <select.h>
# endif

#endif

#include "project.h"
#include "list.h"
#include "jcc.h"
#include "filters.h"
#include "loaders.h"
#include "showargs.h"
#include "parsers.h"
#include "killpopup.h"
#include "miscutil.h"
#include "errlog.h"
#include "jbsockets.h"
#include "gateway.h"
#include "actions.h"
#include "cgi.h"
#include "loadcfg.h"

const char jcc_h_rcs[] = JCC_H_VERSION;
const char project_h_rcs[] = PROJECT_H_VERSION;

struct client_state  clients[1];
struct file_list     files[1];

#ifdef FEATURE_STATISTICS
int urls_read     = 0;     /* total nr of urls read inc rejected */
int urls_rejected = 0;     /* total nr of urls rejected */
#endif /* def FEATURE_STATISTICS */


static void listen_loop(void);
static void chat(struct client_state *csp);
#ifdef AMIGA
void serve(struct client_state *csp);
#else /* ifndef AMIGA */
static void serve(struct client_state *csp);
#endif /* def AMIGA */

#ifdef __BEOS__
static int32 server_thread(void *data);
#endif /* def __BEOS__ */

#ifdef _WIN32
#define sleep(N)  Sleep(((N) * 1000))
#endif


/* The vanilla wafer. */
static const char VANILLA_WAFER[] =
   "NOTICE=TO_WHOM_IT_MAY_CONCERN_"
   "Do_not_send_me_any_copyrighted_information_other_than_the_"
   "document_that_I_am_requesting_or_any_of_its_necessary_components._"
   "In_particular_do_not_send_me_any_cookies_that_"
   "are_subject_to_a_claim_of_copyright_by_anybody._"
   "Take_notice_that_I_refuse_to_be_bound_by_any_license_condition_"
   "(copyright_or_otherwise)_applying_to_any_cookie._";


/*********************************************************************
 *
 * Function    :  chat
 *
 * Description :  Once a connection to the client has been accepted,
 *                this function is called (via serve()) to handle the
 *                main business of the communication.  When this 
 *                function returns, the caller must close the client
 *                socket handle.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  On success, the number of bytes written are returned (zero
 *                indicates nothing was written).  On error, -1 is returned,
 *                and errno is set appropriately.  If count is zero and the
 *                file descriptor refers to a regular file, 0 will be
 *                returned without causing any other effect.  For a special
 *                file, the results are not portable.
 *
 *********************************************************************/
static void chat(struct client_state *csp)
{
/*
 * This next lines are a little ugly, but they simplifies the if statements
 * below.  Basically if TOGGLE, then we want the if to test "csp->toggled_on",
 * else we don't.  And if FEATURE_FORCE_LOAD, then we want the if to test
 * "csp->toggled_on", else we don't
 */
#ifdef FEATURE_TOGGLE
#   define IS_TOGGLED_ON_AND (csp->toggled_on) &&
#else /* ifndef FEATURE_TOGGLE */
#   define IS_TOGGLED_ON_AND
#endif /* ndef FEATURE_TOGGLE */
#ifdef FEATURE_FORCE_LOAD
#   define IS_NOT_FORCED_AND (!csp->force) && 
#else /* ifndef FEATURE_FORCE_LOAD */
#   define IS_NOT_FORCED_AND
#endif /* def FEATURE_FORCE_LOAD */

#define IS_ENABLED_AND   IS_TOGGLED_ON_AND IS_NOT_FORCED_AND

   char buf[BUFFER_SIZE];
   char *hdr, *p, *req;
   fd_set rfds;
   int n, maxfd, server_body;
   int ms_iis5_hack = 0;
   int byte_count = 0;
   const struct forward_spec * fwd;
   struct http_request *http;
#ifdef FEATURE_KILL_POPUPS
   int block_popups;         /* bool, 1==will block popups */
   int block_popups_now = 0; /* bool, 1==currently blocking popups */
#endif /* def FEATURE_KILL_POPUPS */

   int pcrs_filter;        /* bool, 1==will filter through pcrs */
   int gif_deanimate;      /* bool, 1==will deanimate gifs */

   /* Function that does the content filtering for the current request */
   char *(*content_filter)() = NULL; 

   /* Skeleton for HTTP response, if we should intercept the request */
   struct http_response *rsp;

   http = csp->http;

   /*
    * Read the client's request.  Note that since we're not using select() we
    * could get blocked here if a client connected, then didn't say anything!
    */

   while (FOREVER)
   {
      n = read_socket(csp->cfd, buf, sizeof(buf));

      if (n <= 0) break;      /* error! */

      add_to_iob(csp, buf, n);

      req = get_header(csp);

      if (req == NULL)
      {
         break;    /* no HTTP request! */
      }

      if (*req == '\0')
      {
         continue;   /* more to come! */
      }
 
#ifdef FEATURE_FORCE_LOAD
      /* If this request contains the FORCE_PREFIX,
       * better get rid of it now and set the force flag --oes
       */

      if (strstr(req, FORCE_PREFIX))
      {
         strclean(req, FORCE_PREFIX);
         log_error(LOG_LEVEL_FORCE, "Enforcing request \"%s\".\n", req);
         csp->force = 1;
      } 
      else
      {
         csp->force = 0;
      }
#endif /* def FEATURE_FORCE_LOAD */
  
      parse_http_request(req, http, csp);
      freez(req);
      break;
   }

   if (http->cmd == NULL)
   {
      strcpy(buf, CHEADER);
      write_socket(csp->cfd, buf, strlen(buf));

      log_error(LOG_LEVEL_CLF, "%s - - [%T] \" \" 400 0", csp->ip_addr_str);

      return;
   }

   /* decide how to route the HTTP request */

   if ((fwd = forward_url(http, csp)) == NULL)
   {
      log_error(LOG_LEVEL_FATAL, "gateway spec is NULL!?!?  This can't happen!");
      /* Never get here - LOG_LEVEL_FATAL causes program exit */
   }

   /* build the http request to send to the server
    * we have to do one of the following:
    *
    * create = use the original HTTP request to create a new
    *          HTTP request that has only the path component
    *          without the http://domainspec
    * pass   = pass the original HTTP request unchanged
    *
    * drop   = drop the HTTP request
    *
    * here's the matrix:
    *                        SSL
    *                    0        1
    *                +--------+--------+
    *                |        |        |
    *             0  | create | drop   |
    *                |        |        |
    *  Forwarding    +--------+--------+
    *                |        |        |
    *             1  | pass   | pass   |
    *                |        |        |
    *                +--------+--------+
    *
    */

   if (fwd->forward_host)
   {
      /* if forwarding, just pass the request as is */
      enlist(csp->headers, http->cmd);
   }
   else
   {
      if (http->ssl == 0)
      {
         /* otherwise elide the host information from the url */
         p = NULL;
         p = strsav(p, http->gpc);
         p = strsav(p, " ");
         p = strsav(p, http->path);
         p = strsav(p, " ");
         p = strsav(p, http->ver);
         enlist(csp->headers, p);
         freez(p);
      }
   }

   /* decide what we're to do with cookies */

#ifdef FEATURE_TOGGLE
   if (!csp->toggled_on)
   {
      /* Most compatible set of actions (i.e. none) */
      init_current_action(csp->action);
   }
   else
#endif /* ndef FEATURE_TOGGLE */
   {
      url_actions(http, csp);
   }

#ifdef FEATURE_COOKIE_JAR
   /*
    * If we're logging cookies in a cookie jar, and the user has not
    * supplied any wafers, and the user has not told us to suppress the
    * vanilla wafer, then send the vanilla wafer.
    */
   if ((csp->config->jarfile != NULL)
       && list_is_empty(csp->action->multi[ACTION_MULTI_WAFER])
       && ((csp->action->flags & ACTION_VANILLA_WAFER) != 0))
   {
      enlist(csp->action->multi[ACTION_MULTI_WAFER], VANILLA_WAFER);
   }
#endif /* def FEATURE_COOKIE_JAR */

#ifdef FEATURE_KILL_POPUPS
   block_popups               = ((csp->action->flags & ACTION_NO_POPUPS) != 0);
#endif /* def FEATURE_KILL_POPUPS */

   pcrs_filter                = (csp->rlist != NULL) &&  /* There are expressions to be used */
                                ((csp->action->flags & ACTION_FILTER) != 0);

   gif_deanimate              = ((csp->action->flags & ACTION_DEANIMATE) != 0);

   /* grab the rest of the client's headers */

   while (FOREVER)
   {
      if ( ( p = get_header(csp) ) && ( *p == '\0' ) )
      {
         n = read_socket(csp->cfd, buf, sizeof(buf));
         if (n <= 0)
         {
            log_error(LOG_LEVEL_ERROR, "read from client failed: %E");
            return;
         }
         add_to_iob(csp, buf, n);
         continue;
      }

      if (p == NULL) break;

      enlist(csp->headers, p);
      freez(p);
   }

   /* We have a request. */

   hdr = sed(client_patterns, add_client_headers, csp);
   list_remove_all(csp->headers);

   /* 
    * Now, check to see if we need to intercept it, i.e.
    * If
    */
 
   if (
       /* a CGI call was detected and answered */
   	 (NULL != (rsp = dispatch_cgi(csp))) 

       /* or we are enabled and... */
       || (IS_ENABLED_AND (

            /* ..the request was blocked */
   	    ( NULL != (rsp = block_url(csp)))

          /* ..or untrusted */
#ifdef FEATURE_TRUST
          || ( NULL != (rsp = trust_url(csp)))
#endif /* def FEATURE_TRUST */

          /* ..or a fast redirect kicked in */
#ifdef FEATURE_FAST_REDIRECTS
          || (((csp->action->flags & ACTION_FAST_REDIRECTS) != 0) && 
   		     (NULL != (rsp = redirect_url(csp))))
#endif /* def FEATURE_FAST_REDIRECTS */
   		 ))
   	)
   {
      /* Write the answer to the client */
      if ((write_socket(csp->cfd, rsp->head, rsp->head_length) != rsp->head_length)
   	     || (write_socket(csp->cfd, rsp->body, rsp->content_length) != rsp->content_length))
      { 
         log_error(LOG_LEVEL_ERROR, "write to: %s failed: %E", http->host);
      }

#ifdef FEATURE_STATISTICS
      /* Count as a rejected request */
      csp->rejected = 1;
#endif /* def FEATURE_STATISTICS */

      /* Log (FIXME: All intercept reasons apprear as "crunch" with Status 200) */
      log_error(LOG_LEVEL_GPC, "%s%s crunch!", http->hostport, http->path);
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 200 3", csp->ip_addr_str, http->cmd); 

      /* Clean up and return */
      free_http_response(rsp);
      freez(hdr);
      return;
   }

   log_error(LOG_LEVEL_GPC, "%s%s", http->hostport, http->path);

   if (fwd->forward_host)
   {
      log_error(LOG_LEVEL_CONNECT, "via %s:%d to: %s",
               fwd->forward_host, fwd->forward_port, http->hostport);
   }
   else
   {
      log_error(LOG_LEVEL_CONNECT, "to %s", http->hostport);
   }

   /* here we connect to the server, gateway, or the forwarder */

   csp->sfd = forwarded_connect(fwd, http, csp);

   if (csp->sfd < 0)
   {
      log_error(LOG_LEVEL_CONNECT, "connect to: %s failed: %E",
                http->hostport);

      if (errno == EINVAL)
      {
   	   rsp = error_response(csp, "no-such-domain", errno);

         log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 404 0", 
                   csp->ip_addr_str, http->cmd);
      }
      else
      {
   	   rsp = error_response(csp, "connect-failed", errno);

         log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 503 0", 
                   csp->ip_addr_str, http->cmd);
      }

      /* Write the answer to the client */
      if(rsp)
   	{
         if ((write_socket(csp->cfd, rsp->head, rsp->head_length) != rsp->head_length)
   	        || (write_socket(csp->cfd, rsp->body, rsp->content_length) != rsp->content_length))
         { 
            log_error(LOG_LEVEL_ERROR, "write to: %s failed: %E", http->host);
         }
      }

      free_http_response(rsp);
      freez(hdr);
      return;
   }

   log_error(LOG_LEVEL_CONNECT, "OK");

   if (fwd->forward_host || (http->ssl == 0))
   {
      /* write the client's (modified) header to the server
       * (along with anything else that may be in the buffer)
       */

      n = strlen(hdr);

      if ((write_socket(csp->sfd, hdr, n) != n)
          || (flush_socket(csp->sfd, csp   ) <  0))
      {
         log_error(LOG_LEVEL_CONNECT, "write header to: %s failed: %E",
                    http->hostport);

         log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 503 0", 
                   csp->ip_addr_str, http->cmd); 

         rsp = error_response(csp, "connect-failed", errno);

         if(rsp)
         {
            if ((write_socket(csp->cfd, rsp->head, n) != n)
   	        || (write_socket(csp->cfd, rsp->body, rsp->content_length) != rsp->content_length))
            { 
               log_error(LOG_LEVEL_ERROR, "write to: %s failed: %E", http->host);
            }
         }

         free_http_response(rsp);
         freez(hdr);
         return;
      }
   }
   else
   {
      /*
       * We're running an SSL tunnel and we're not forwarding,
       * so just send the "connect succeeded" message to the
       * client, flush the rest, and get out of the way.
       */
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 200 2\n", 
                csp->ip_addr_str, http->cmd); 

      if (write_socket(csp->cfd, CSUCCEED, sizeof(CSUCCEED)-1) < 0)
      {
         freez(hdr);
         return;
      }
      IOB_RESET(csp);
   }

   /* we're finished with the client's header */
   freez(hdr);

   maxfd = ( csp->cfd > csp->sfd ) ? csp->cfd : csp->sfd;

   /* pass data between the client and server
    * until one or the other shuts down the connection.
    */

   server_body = 0;

   while (FOREVER)
   {
      FD_ZERO(&rfds);

      FD_SET(csp->cfd, &rfds);
      FD_SET(csp->sfd, &rfds);

      n = select(maxfd+1, &rfds, NULL, NULL, NULL);

      if (n < 0)
      {
         log_error(LOG_LEVEL_ERROR, "select() failed!: %E");
         return;
      }

      /* this is the body of the browser's request
       * just read it and write it.
       */

      if (FD_ISSET(csp->cfd, &rfds))
      {
         n = read_socket(csp->cfd, buf, sizeof(buf));

         if (n <= 0)
         {
            break; /* "game over, man" */
         }

         if (write_socket(csp->sfd, buf, n) != n)
         {
            log_error(LOG_LEVEL_ERROR, "write to: %s failed: %E", http->host);
            return;
         }
         continue;
      }

      /*
       * The server wants to talk.  It could be the header or the body.
       * If `hdr' is null, then it's the header otherwise it's the body.
       * FIXME: Does `hdr' really mean `host'? No.
       */


      if (FD_ISSET(csp->sfd, &rfds))
      {
         fflush( 0 );
         n = read_socket(csp->sfd, buf, sizeof(buf) - 1);

         if (n < 0)
         {
            log_error(LOG_LEVEL_ERROR, "read from: %s failed: %E", http->host);

            log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 503 0", 
                      csp->ip_addr_str, http->cmd); 

            rsp = error_response(csp, "connect-failed", errno);

            if(rsp)
            {
               if ((write_socket(csp->cfd, rsp->head, rsp->head_length) != rsp->head_length)
   	            || (write_socket(csp->cfd, rsp->body, rsp->content_length) != rsp->content_length))
               { 
                  log_error(LOG_LEVEL_ERROR, "write to: %s failed: %E", http->host);
   			   }
   			}

            free_http_response(rsp);
            return;
         }

         /* Add a trailing zero.  This lets filter_popups
          * use string operations.
          */
         buf[n] = '\0';

#ifdef FEATURE_KILL_POPUPS
         /* Filter the popups on this read. */
         if (block_popups_now)
         {
            filter_popups(buf);
         }
#endif /* def FEATURE_KILL_POPUPS */

         /* Normally, this would indicate that we've read
          * as much as the server has sent us and we can
          * close the client connection.  However, Microsoft
          * in its wisdom has released IIS/5 with a bug that
          * prevents it from sending the trailing \r\n in
          * a 302 redirect header (and possibly other headers).
          * To work around this if we've haven't parsed
          * a full header we'll append a trailing \r\n
          * and see if this now generates a valid one.
          *
          * This hack shouldn't have any impacts.  If we've
          * already transmitted the header or if this is a
          * SSL connection, then we won't bother with this
          * hack.  So we only work on partially received
          * headers.  If we append a \r\n and this still
          * doesn't generate a valid header, then we won't
          * transmit anything to the client.
          */
         if (n == 0)
         {
            
            if (server_body || http->ssl)
            {
               /*
                * If we have been buffering up the document,
                * now is the time to apply content modification
                * and send the result to the client.
                */
               if (content_filter)
               {
                  /*
                   * If the content filter fails, use the original
                   * buffer and length.
                   * (see p != NULL ? p : csp->iob->cur below)
                   */
                  if (NULL == (p = (*content_filter)(csp)))
                  {
                     csp->content_length = csp->iob->eod - csp->iob->cur;
                  }

                  hdr = sed(server_patterns, add_server_headers, csp);
                  n = strlen(hdr);

                  if ((write_socket(csp->cfd, hdr, n) != n)
                      || (write_socket(csp->cfd, p != NULL ? p : csp->iob->cur, csp->content_length) != csp->content_length))
                  {
                     log_error(LOG_LEVEL_CONNECT, "write modified content to client failed: %E");
                     return;
                  }

                  freez(hdr);
                  freez(p);
               }

               break; /* "game over, man" */
            }

            /*
             * This is NOT the body, so 
             * Let's pretend the server just sent us a blank line.
             */
            n = sprintf(buf, "\r\n");

            /*
             * Now, let the normal header parsing algorithm below do its
             * job.  If it fails, we'll exit instead of continuing.
             */

            ms_iis5_hack = 1;
         }

         /*
          * If this is an SSL connection or we're in the body
          * of the server document, just write it to the client,
          * unless we need to buffer the body for later content-filtering
          */

         if (server_body || http->ssl)
         {
            if (content_filter)
            {
               add_to_iob(csp, buf, n); 
            }
            else
            {
               if (write_socket(csp->cfd, buf, n) != n)
               {
                  log_error(LOG_LEVEL_ERROR, "write to client failed: %E");
                  return;
               }
            }
            byte_count += n;
            continue;
         }
         else
         {
            /* we're still looking for the end of the
             * server's header ... (does that make header
             * parsing an "out of body experience" ?
             */

            /* buffer up the data we just read */
            add_to_iob(csp, buf, n);

            /* get header lines from the iob */

            while ((p = get_header(csp)))
            {
               if (*p == '\0')
               {
                  /* see following note */
                  break;
               }
               enlist(csp->headers, p);
               freez(p);
            }

            /* NOTE: there are no "empty" headers so
             * if the pointer `p' is not NULL we must
             * assume that we reached the end of the
             * buffer before we hit the end of the header.
             */

            if (p)
            {
               if (ms_iis5_hack)
               {
                  /* Well, we tried our MS IIS/5
                   * hack and it didn't work.
                   * The header is incomplete
                   * and there isn't anything
                   * we can do about it.
                   */
                  break;
               }
               else
               {
                  /* Since we have to wait for
                   * more from the server before
                   * we can parse the headers
                   * we just continue here.
                   */
                  continue;
               }
            }

            /* we have now received the entire header.
             * filter it and send the result to the client
             */

            hdr = sed(server_patterns, add_server_headers, csp);
            n   = strlen(hdr);

            /* write the server's (modified) header to
             * the client (along with anything else that
             * may be in the buffer)
             */

#ifdef FEATURE_KILL_POPUPS
            /* Start blocking popups if appropriate. */

            if ((csp->content_type & CT_TEXT) &&  /* It's a text / * MIME-Type */
                !http->ssl    &&                  /* We talk plaintext */
                block_popups)                     /* Policy allows */
            {
               block_popups_now = 1;
               /*
                * Filter the part of the body that came in the same read
                * as the last headers:
                */
               filter_popups(csp->iob->cur);
            }

#endif /* def FEATURE_KILL_POPUPS */

            /* Buffer and pcrs filter this if appropriate. */

            if ((csp->content_type & CT_TEXT) &&  /* It's a text / * MIME-Type */
                !http->ssl    &&                  /* We talk plaintext */
                pcrs_filter)                      /* Policy allows */
            {
               content_filter = pcrs_filter_response;
            }

            /* Buffer and gif_deanimate this if appropriate. */

            if ((csp->content_type & CT_GIF)  &&  /* It's a image/gif MIME-Type */
                !http->ssl    &&                  /* We talk plaintext */
                gif_deanimate)                    /* Policy allows */
            {
               content_filter = gif_deanimate_response;
            }


            /*
             * Only write if we're not buffering for content modification
             */
            if (!content_filter && ((write_socket(csp->cfd, hdr, n) != n)
                || (n = flush_socket(csp->cfd, csp) < 0)))
            {
               log_error(LOG_LEVEL_CONNECT, "write header to client failed: %E");

               /* the write failed, so don't bother
                * mentioning it to the client...
                * it probably can't hear us anyway.
                */
               freez(hdr);
               return;
            }

            if(!content_filter) byte_count += n;

            /* we're finished with the server's header */

            freez(hdr);
            server_body = 1;

            /* If this was a MS IIS/5 hack then it means
             * the server has already closed the
             * connection.  Nothing more to read.  Time
             * to bail.
             */
            if (ms_iis5_hack)
            {
               break;
            }
         }
         continue;
      }

      return; /* huh? we should never get here */
   }

   log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 200 %d", 
             csp->ip_addr_str, http->cmd, byte_count); 
}


/*********************************************************************
 *
 * Function    :  serve
 *
 * Description :  This is little more than chat.  We only "serve" to
 *                to close any socket that chat may have opened.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
#ifdef AMIGA
void serve(struct client_state *csp)
#else /* ifndef AMIGA */
static void serve(struct client_state *csp)
#endif /* def AMIGA */
{
   chat(csp);
   close_socket(csp->cfd);

   if (csp->sfd >= 0)
   {
      close_socket(csp->sfd);
   }

   csp->active = 0;

}


#ifdef __BEOS__
/*********************************************************************
 *
 * Function    :  server_thread
 *
 * Description :  We only exist to call `serve' in a threaded environment.
 *
 * Parameters  :
 *          1  :  data = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Always 0.
 *
 *********************************************************************/
static int32 server_thread(void *data)
{
   serve((struct client_state *) data);
   return 0;

}
#endif


/*********************************************************************
 *
 * Function    :  main
 *
 * Description :  Load the config file and start the listen loop.
 *                This function is a lot more *sane* with the `load_config'
 *                and `listen_loop' functions; although it stills does
 *                a *little* too much for my taste.
 *
 * Parameters  :
 *          1  :  argc = Number of parameters (including $0).
 *          2  :  argv = Array of (char *)'s to the parameters.
 *
 * Returns     :  1 if : can't open config file, unrecognized directive,
 *                stats requested in multi-thread mode, can't open the
 *                log file, can't open the jar file, listen port is invalid,
 *                any load fails, and can't bind port.
 *
 *                Else main never returns, the process must be signaled
 *                to terminate execution.  Or, on Windows, use the 
 *                "File", "Exit" menu option.
 *
 *********************************************************************/
#ifdef __MINGW32__
int real_main(int argc, const char *argv[])
#else
int main(int argc, const char *argv[])
#endif
{
   configfile =
#ifdef AMIGA
   "AmiTCP:db/junkbuster/config"
#elif !defined(_WIN32)
   "config"
#else
   "junkbstr.txt"
#endif
      ;

#if !defined(_WIN32) || defined(_WIN_CONSOLE)
   if ((argc >= 2) && (strcmp(argv[1], "--help")==0))
   {
      printf("JunkBuster proxy version " VERSION ".\n\n"
         "Usage: %s [configfile]\n\n"
         "See " HOME_PAGE_URL " for details.\n"
         "This program is distributed under the GNU GPL, version 2 or later.\n",
         argv[0]);
      exit(2);
   }
   if ((argc >= 2) && (strcmp(argv[1], "--version")==0))
   {
      printf(VERSION "\n");
      exit(2);
   }
#endif /* !defined(_WIN32) || defined(_WIN_CONSOLE) */

   Argc = argc;
   Argv = argv;

   if (argc > 1)
   {
      configfile = argv[1];
   }

   files->next = NULL;

#ifdef AMIGA
   InitAmiga();
#elif defined(_WIN32)
   InitWin32();
#endif


#ifndef _WIN32
   signal(SIGPIPE, SIG_IGN);
   signal(SIGCHLD, SIG_IGN);

#else /* ifdef _WIN32 */
# ifdef _WIN_CONSOLE
   /*
    * We *are* in a windows console app.
    * Print a verbose messages about FAQ's and such
    */
   printf(win32_blurb);
# endif /* def _WIN_CONSOLE */
#endif /* def _WIN32 */


   listen_loop();

   /* NOTREACHED */
   return(-1);

}


/*********************************************************************
 *
 * Function    :  listen_loop
 *
 * Description :  bind the listen port and enter a "FOREVER" listening loop.
 *
 * Parameters  :  N/A
 *
 * Returns     :  Never.
 *
 *********************************************************************/
static void listen_loop(void)
{
   struct client_state *csp = NULL;
   int bfd;
   struct configuration_spec * config;

   config = load_config();

   log_error(LOG_LEVEL_CONNECT, "bind (%s, %d)",
             config->haddr ? config->haddr : "INADDR_ANY", config->hport);

   bfd = bind_port(config->haddr, config->hport);

   if (bfd < 0)
   {
      log_error(LOG_LEVEL_FATAL, "can't bind %s:%d: %E "
         "- There may be another junkbuster or some other "
         "proxy running on port %d", 
         (NULL != config->haddr) ? config->haddr : "INADDR_ANY", 
         config->hport, config->hport
      );
      /* shouldn't get here */
      return;
   }

   config->need_bind = 0;


   while (FOREVER)
   {
#if !defined(FEATURE_PTHREAD) && !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA)
      while (waitpid(-1, NULL, WNOHANG) > 0)
      {
         /* zombie children */
      }
#endif /* !defined(FEATURE_PTHREAD) && !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA) */
      sweep();

      if ( NULL == (csp = (struct client_state *) zalloc(sizeof(*csp))) )
      {
         log_error(LOG_LEVEL_FATAL, "malloc(%d) for csp failed: %E", sizeof(*csp));
         continue;
      }

      csp->active = 1;
      csp->sfd    = -1;

      csp->config = config = load_config();

      if ( config->need_bind )
      {
         /*
          * Since we were listening to the "old port", we will not see
          * a "listen" param change until the next IJB request.  So, at
          * least 1 more request must be made for us to find the new
          * setting.  I am simply closing the old socket and binding the
          * new one.
          *
          * Which-ever is correct, we will serve 1 more page via the
          * old settings.  This should probably be a "show-proxy-args"
          * request.  This should not be a so common of an operation
          * that this will hurt people's feelings.
          */

         close_socket(bfd);

         log_error(LOG_LEVEL_CONNECT, "bind (%s, %d)",
                   config->haddr ? config->haddr : "INADDR_ANY", config->hport);
         bfd = bind_port(config->haddr, config->hport);

         if (bfd < 0)
         {
            log_error(LOG_LEVEL_FATAL, "can't bind %s:%d: %E "
               "- There may be another junkbuster or some other "
               "proxy running on port %d", 
               (NULL != config->haddr) ? config->haddr : "INADDR_ANY", 
               config->hport, config->hport
            );
            /* shouldn't get here */
            return;
         }

         config->need_bind = 0;
      }

      log_error(LOG_LEVEL_CONNECT, "accept connection ... ");

      if (!accept_connection(csp, bfd))
      {
         log_error(LOG_LEVEL_CONNECT, "accept failed: %E");

#ifdef AMIGA
         if(!childs)
         {
            exit(1); 
         }
#endif
         freez(csp);
         continue;
      }
      else
      {
         log_error(LOG_LEVEL_CONNECT, "OK");
      }

#ifdef FEATURE_TOGGLE
      /* by haroon - most of credit to srt19170 */
      csp->toggled_on = g_bToggleIJB;
#endif /* def FEATURE_TOGGLE */

      if (run_loader(csp))
      {
         log_error(LOG_LEVEL_FATAL, "a loader failed - must exit");
         /* Never get here - LOG_LEVEL_FATAL causes program exit */
      }

#ifdef FEATURE_ACL
      if (block_acl(NULL,csp))
      {
         log_error(LOG_LEVEL_CONNECT, "Connection dropped due to ACL");
         close_socket(csp->cfd);
         freez(csp);
         continue;
      }
#endif /* def FEATURE_ACL */

      /* add it to the list of clients */
      csp->next = clients->next;
      clients->next = csp;

      if (config->multi_threaded)
      {
         int child_id;

/* this is a switch () statment in the C preprocessor - ugh */
#undef SELECTED_ONE_OPTION

/* Use Pthreads in preference to native code */
#if defined(FEATURE_PTHREAD) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
         {
            pthread_t the_thread;
            pthread_attr_t attrs;

            pthread_attr_init(&attrs);
            pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
            child_id = (pthread_create(&the_thread, &attrs,
               (void*)serve, csp) ? -1 : 0);
            pthread_attr_destroy(&attrs);
         }
#endif

#if defined(_WIN32) && !defined(_CYGWIN) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
         child_id = _beginthread(
            (void*)serve,
            64 * 1024,
            csp);
#endif

#if defined(__BEOS__) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
         {
            thread_id tid = spawn_thread
               (server_thread, "server", B_NORMAL_PRIORITY, csp);

            if ((tid >= 0) && (resume_thread(tid) == B_OK))
            {
               child_id = (int) tid;
            }
            else
            {
               child_id = -1;
            }
         }
#endif

#if defined(AMIGA) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
         csp->cfd = ReleaseSocket(csp->cfd, -1);
         if((child_id = (int)CreateNewProcTags(
            NP_Entry, (ULONG)server_thread,
            NP_Output, Output(),
            NP_CloseOutput, FALSE,
            NP_Name, (ULONG)"junkbuster child",
            NP_StackSize, 200*1024,
            TAG_DONE)))
         {
            childs++;
            ((struct Task *)child_id)->tc_UserData = csp;
            Signal((struct Task *)child_id, SIGF_SINGLE);
            Wait(SIGF_SINGLE);
         }
#endif

#if !defined(SELECTED_ONE_OPTION)
         child_id = fork();

         /* This block is only needed when using fork().
          * When using threads, the server thread was
          * created and run by the call to _beginthread().
          */
         if (child_id == 0)   /* child */
         {
            serve(csp);
            _exit(0);

         }
         else if (child_id > 0) /* parent */
         {
            /* in a fork()'d environment, the parent's
             * copy of the client socket and the CSP
             * are not used.
             */

#if !defined(_WIN32) && defined(__CYGWIN__)
            wait( NULL );
#endif /* !defined(_WIN32) && defined(__CYGWIN__) */
            close_socket(csp->cfd);
            csp->active = 0;
         }
#endif

#undef SELECTED_ONE_OPTION
/* end of cpp switch () */

         if (child_id < 0) /* failed */
         {
            char buf[BUFFER_SIZE];

            log_error(LOG_LEVEL_ERROR, "can't fork: %E");

            sprintf(buf , "JunkBuster: can't fork: errno = %d", errno);

            write_socket(csp->cfd, buf, strlen(buf));
            close_socket(csp->cfd);
            csp->active = 0;
            sleep(5);
            continue;
         }
      }
      else
      {
         serve(csp);
      }
   }
   /* NOTREACHED */

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
