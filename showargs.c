const char showargs_rcs[] = "$Id: showargs.c,v 1.21 2001/07/30 22:08:36 jongfoster Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/showargs.c,v $
 *
 * Purpose     :  Contains various utility routines needed to 
 *                generate the show-proxy-args page.
 *                FIXME: Is this really stuff for a separate file?
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
 *    $Log: showargs.c,v $
 *    Revision 1.21  2001/07/30 22:08:36  jongfoster
 *    Tidying up #defines:
 *    - All feature #defines are now of the form FEATURE_xxx
 *    - Permanently turned off WIN_GUI_EDIT
 *    - Permanently turned on WEBDAV and SPLIT_PROXY_ARGS
 *
 *    Revision 1.20  2001/07/18 17:27:22  oes
 *    Adapted to new #defines
 *
 *    Revision 1.19  2001/07/13 14:11:36  oes
 *     - Included SHOW_RCS for deanimate.*
 *     - Removed all #ifdef PCRS
 *
 *
 *    Revision 1.18  2001/07/02 02:55:16  iwanttokeepanon
 *    Apended " on some sites" to the HTML generating function `show_defines' (@ line
 *    392); since "DENY_GZIP" is not *really* necessary for all PCRS functionallity.
 *
 *    Revision 1.17  2001/06/29 21:45:41  oes
 *    Indentation, CRLF->LF, Tab-> Space
 *
 *    Revision 1.16  2001/06/29 13:35:07  oes
 *    - Adapted
 *    - Improved comments
 *    - Removed init_proxy_args
 *    - Renamed end_proxy_args(csp) to show_rcs(void)
 *    - Removed logentry from cancelled commit
 *    - Destroyed support for ndef SPLIT_PROXY_ARGS (Ooops)
 *    - Separated the #define list into show_defines()
 *
 *    Revision 1.15  2001/06/09 10:55:28  jongfoster
 *    Changing BUFSIZ ==> BUFFER_SIZE
 *
 *    Revision 1.14  2001/06/07 23:15:40  jongfoster
 *    Removing config->proxy_args_gateways
 *    Missing return statement added to end_proxy_args().
 *
 *    Revision 1.13  2001/06/06 09:37:59  sarantis
 *    Fix misplaced comment start.
 *
 *    Revision 1.12  2001/06/04 10:41:52  swa
 *    show version string of cgi.h and cgi.c
 *
 *    Revision 1.11  2001/06/03 11:03:48  oes
 *    moved stuff to cgi.c
 *
 *    Revision 1.10  2001/05/31 21:36:07  jongfoster
 *    Added RCS for actions.[ch] and list.[ch]
 *
 *    Revision 1.9  2001/05/29 23:11:38  oes
 *
 *     - Moved strsav() from showargs to miscutil
 *
 *    Revision 1.8  2001/05/29 09:50:24  jongfoster
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
 *    Revision 1.7  2001/05/26 00:28:36  jongfoster
 *    Automatic reloading of config file.
 *    Removed obsolete SIGHUP support (Unix) and Reload menu option (Win32).
 *    Most of the global variables have been moved to a new
 *    struct configuration_spec, accessed through csp->config->globalname
 *    Most of the globals remaining are used by the Win32 GUI.
 *
 *    Revision 1.6  2001/05/25 22:32:56  jongfoster
 *    CRLF->LF
 *
 *    Revision 1.5  2001/05/22 18:54:49  oes
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
 *    Revision 1.4  2001/05/20 16:44:47  jongfoster
 *    Removing last hardcoded JunkBusters.com URLs.
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
 *    Revision 1.2  2001/05/17 23:01:01  oes
 *     - Cleaned CRLF's from the sources and related files
 *
 *    Revision 1.1.1.1  2001/05/15 13:59:03  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include "project.h"
#include "showargs.h"
#include "jcc.h"
#include "encode.h"
#include "parsers.h"
#include "errlog.h"
#include "miscutil.h"
#include "gateway.h"
#include "cgi.h"
#include "list.h"

const char showargs_h_rcs[] = SHOWARGS_H_VERSION;


/*********************************************************************
 *
 * Function    :  savearg
 *
 * Description :  Called from `load_config'.  It saves each non-empty
 *                and non-comment line from config into a list.  This
 *                list is used to create the show-proxy-args page.
 *
 * Parameters  :
 *          1  :  c = config setting that was found
 *          2  :  o = the setting's argument (if any)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void savearg(char *c, char *o, struct configuration_spec * config)
{
   char buf[BUFFER_SIZE];

   *buf = '\0';

   if ( ( NULL != c ) && ( '\0' != *c ) )
   {
      if ((c = html_encode(c)))
      {
         sprintf(buf, "<a href=\"" REDIRECT_URL "option#%s\">%s</a> ", c, c);
      }
      freez(c);
   }
   if ( ( NULL != o ) && ( '\0' != *o ) )
   {
      if ((o = html_encode(o)))
      {
         if (strncmpic(o, "http://", 7) == 0)
         {
            strcat(buf, "<a href=\"");
            strcat(buf, o);
            strcat(buf, "\">");
            strcat(buf, o);
            strcat(buf, "</a>");
         }
         else
         {
            strcat(buf, o);
         }
      }
      freez(o);
   }

   strcat(buf, "<br>\n");

   config->proxy_args = strsav(config->proxy_args, buf);

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
char *show_rcs(void)
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
   SHOW_RCS(showargs_h_rcs)
   SHOW_RCS(showargs_rcs)
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
struct map * show_defines(struct map *exports)
{

#ifdef FEATURE_ACL
   exports = map_conditional(exports, "FEATURE_ACL", 1);
#else /* ifndef FEATURE_ACL */
   exports = map_conditional(exports, "FEATURE_ACL", 0);
#endif /* ndef FEATURE_ACL */

#ifdef FEATURE_COOKIE_JAR
   exports = map_conditional(exports, "FEATURE_COOKIE_JAR", 1);
#else /* ifndef FEATURE_COOKIE_JAR */
   exports = map_conditional(exports, "FEATURE_COOKIE_JAR", 0);
#endif /* ndef FEATURE_COOKIE_JAR */

#ifdef FEATURE_DENY_GZIP
   exports = map_conditional(exports, "FEATURE_DENY_GZIP", 1);
#else /* ifndef FEATURE_DENY_GZIP */
   exports = map_conditional(exports, "FEATURE_DENY_GZIP", 0);
#endif /* ndef FEATURE_DENY_GZIP */

#ifdef FEATURE_FAST_REDIRECTS
   exports = map_conditional(exports, "FEATURE_FAST_REDIRECTS", 1);
#else /* ifndef FEATURE_FAST_REDIRECTS */
   exports = map_conditional(exports, "FEATURE_FAST_REDIRECTS", 0);
#endif /* ndef FEATURE_FAST_REDIRECTS */

#ifdef FEATURE_FORCE_LOAD
   exports = map_conditional(exports, "FEATURE_FORCE_LOAD", 1);
#else /* ifndef FEATURE_FORCE_LOAD */
   exports = map_conditional(exports, "FEATURE_FORCE_LOAD", 0);
#endif /* ndef FEATURE_FORCE_LOAD */

#ifdef FEATURE_IMAGE_BLOCKING
   exports = map_conditional(exports, "FEATURE_IMAGE_BLOCKING", 1);
#else /* ifndef FEATURE_IMAGE_BLOCKING */
   exports = map_conditional(exports, "FEATURE_IMAGE_BLOCKING, 0);
#endif /* ndef FEATURE_IMAGE_BLOCKING */

#ifdef FEATURE_IMAGE_DETECT_MSIE
   exports = map_conditional(exports, "FEATURE_IMAGE_DETECT_MSIE", 1);
#else /* ifndef FEATURE_IMAGE_DETECT_MSIE */
   exports = map_conditional(exports, "FEATURE_IMAGE_DETECT_MSIE", 0);
#endif /* ndef FEATURE_IMAGE_DETECT_MSIE */

#ifdef FEATURE_KILL_POPUPS
   exports = map_conditional(exports, "FEATURE_KILL_POPUPS", 1);
#else /* ifndef FEATURE_KILL_POPUPS */
   exports = map_conditional(exports, "FEATURE_KILL_POPUPS", 0);
#endif /* ndef FEATURE_KILL_POPUPS */

#ifdef FEATURE_PTHREAD
   exports = map_conditional(exports, "FEATURE_PTHREAD", 1);
#else /* ifndef FEATURE_PTHREAD */
   exports = map_conditional(exports, "FEATURE_PTHREAD", 0);
#endif /* ndef FEATURE_PTHREAD */

#ifdef FEATURE_STATISTICS
   exports = map_conditional(exports, "FEATURE_STATISTICS", 1);
#else /* ifndef FEATURE_STATISTICS */
   exports = map_conditional(exports, "FEATURE_STATISTICS", 0);
#endif /* ndef FEATURE_STATISTICS */

#ifdef FEATURE_TOGGLE
   exports = map_conditional(exports, "FEATURE_TOGGLE", 1);
#else /* ifndef FEATURE_TOGGLE */
   exports = map_conditional(exports, "FEATURE_TOGGLE", 0);
#endif /* ndef FEATURE_TOGGLE */

#ifdef FEATURE_TRUST
   exports = map_conditional(exports, "FEATURE_TRUST", 1);
#else /* ifndef FEATURE_TRUST */
   exports = map_conditional(exports, "FEATURE_TRUST", 0);
#endif /* ndef FEATURE_TRUST */

#ifdef REGEX_GNU
   exports = map_conditional(exports, "REGEX_GNU", 1);
#else /* ifndef REGEX_GNU */
   exports = map_conditional(exports, "REGEX_GNU", 0);
#endif /* def REGEX_GNU */

#ifdef REGEX_PCRE
   exports = map_conditional(exports, "REGEX_PCRE", 1);
#else /* ifndef REGEX_PCRE */
   exports = map_conditional(exports, "REGEX_PCRE", 0);
#endif /* def REGEX_PCRE */

#ifdef STATIC_PCRE
   exports = map_conditional(exports, "STATIC_PCRE", 1);
#else /* ifndef STATIC_PCRE */
   exports = map_conditional(exports, "STATIC_PCRE", 0);
#endif /* ndef STATIC_PCRE */

#ifdef STATIC_PCRS
   exports = map_conditional(exports, "STATIC_PCRS", 1);
#else /* ifndef STATIC_PCRS */
   exports = map_conditional(exports, "STATIC_PCRS", 0);
#endif /* ndef STATIC_PCRS */

   exports = map(exports, "FORCE_PREFIX", 1, FORCE_PREFIX, 1);

   return exports;
}


/*
  Local Variables:
  tab-width: 3
  end:
*/
