const char showargs_rcs[] = "$Id: showargs.c,v 1.14 2001/06/07 23:15:40 jongfoster Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/showargs.c,v $
 *
 * Purpose     :  Contains various utility routines needed to 
 *                generate the show-proxy-args page.
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
 *    Makefile/in
 *
 *    introduced cgi.c
 *
 *    actions.c:
 *
 *    adapted to new enlist_unique arg format
 *
 *    conf loadcfg.c
 *
 *    introduced confdir option
 *
 *    filters.c filtrers.h
 *
 *     extracted-CGI relevant stuff
 *
 *    jbsockets.c
 *
 *     filled comment
 *
 *    jcc.c
 *
 *     support for new cgi mechansim
 *
 *    list.c list.h
 *
 *    functions for new list type: "map"
 *    extended enlist_unique
 *
 *    miscutil.c .h
 *    introduced bindup()
 *
 *    parsers.c parsers.h
 *
 *    deleted const struct interceptors
 *
 *    pcrs.c
 *    added FIXME
 *
 *    project.h
 *
 *    added struct map
 *    added struct http_response
 *    changes struct interceptors to struct cgi_dispatcher
 *    moved HTML stuff to cgi.h
 *
 *    re_filterfile:
 *
 *    changed
 *
 *    showargs.c
 *    NO TIME LEFT
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

   config->proxy_args_invocation = strsav(config->proxy_args_invocation, buf);

}


/*********************************************************************
 *
 * Function    :  init_proxy_args
 *
 * Description :  Create the "top" of the show-proxy-args page.
 *
 * Parameters  :
 *          1  :  argc = argument count (same as in main)
 *          2  :  argv[] = program arguments (same as in main)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void init_proxy_args(int argc, const char *argv[], struct configuration_spec * config)
{
   char * b = NULL;
   int i;

   freez(config->proxy_args_header);
 
   
   for (i=0; i < argc; i++)
   {
      b = strsav(b, argv[i]);
      b = strsav(b, " ");
   }
   config->proxy_args_header = b;
}


/*********************************************************************
 *
 * Function    :  end_proxy_args
 *
 * Description :  Create the "bottom" of the show-proxy-args page.
 *
 * Parameters  :  None
 *
 * Returns     :  string with that bottom
 *
 *********************************************************************/
char *end_proxy_args(struct configuration_spec * config)
{
   char *b = NULL;
   char buf[BUFFER_SIZE];

   /* Instead of including *all* dot h's in the project (thus creating a
    * tremendous amount of dependencies), I will concede to declaring them
    * as extern's.  This forces the developer to add to this list, but oh well.
    */

#ifndef SPLIT_PROXY_ARGS
   if (suppress_blocklists && suppress_message!=NULL)
   {
      b = strsav(b, "<h2>File contents</h2>\n");
      b = strsav(b, suppress_message);
      b = strsav(b, "\n");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   b = strsav(b, "<h2>Source versions:</h2>\n");
   b = strsav(b, "<pre>");

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
#ifdef KILLPOPUPS
   SHOW_RCS(killpopup_h_rcs)
   SHOW_RCS(killpopup_rcs)
#endif /* def KILLPOPUPS */
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
#ifdef PCRS
   SHOW_RCS(pcrs_rcs)
   SHOW_RCS(pcrs_h_rcs)
#endif /* def PCRS */
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
   SHOW_RCS(w32rulesdlg_h_rcs)
   SHOW_RCS(w32rulesdlg_rcs)
   SHOW_RCS(w32taskbar_h_rcs)
   SHOW_RCS(w32taskbar_rcs)
#endif /* ndef _WIN_CONSOLE */
   SHOW_RCS(win32_h_rcs)
   SHOW_RCS(win32_rcs)
#endif /* def _WIN32 */

#undef SHOW_RCS

   b = strsav(b, "</pre>\n");

   b = strsav(b, "<h2>Conditional defines:</h2>\n<ul>");

#ifdef REGEX
   b = strsav(b, "  <li><code>#define <b>REGEX</b></code> - Support for regular expressions in the path specs.</li>\n");
#else /* ifndef REGEX */
   b = strsav(b, "  <li><code>#undef <b>REGEX</b></code> - No support for regular expressions in the path specs.</li>\n");
#endif /* ndef REGEX */

#ifdef PCRE
   b = strsav(b, "  <li><code>#define <b>PCRE</b></code> - Use PCRE rather than old GNU regex library.</li>\n");
#else /* ifndef PCRE */
   b = strsav(b, "  <li><code>#undef <b>PCRE</b></code> - Use old GNU regex library rather than PCRE.</li>\n");
#endif /* ndef PCRE */

#ifdef PCRS
   b = strsav(b, "  <li><code>#define <b>PCRS</b></code> - Enables arbitrary content modification regexps.</li>\n");
#else /* ifndef PCRS */
   b = strsav(b, "  <li><code>#undef <b>PCRS</b></code> - Disables arbitrary content modification regexps.</li>\n");
#endif /* ndef PCRS */

#ifdef TOGGLE
   b = strsav(b, "  <li><code>#define <b>TOGGLE</b></code> - Allow JunkBuster to be \"disabled\" so it is just a normal non-blocking non-anonymizing proxy.</li>\n");
#else /* ifndef TOGGLE */
   b = strsav(b, "  <li><code>#undef <b>TOGGLE</b></code> - Do not allow JunkBuster to be \"disabled\" so it is just a normal non-blocking non-anonymizing proxy.</li>\n");
#endif /* ndef TOGGLE */

#ifdef FORCE_LOAD
   b = strsav(b, "  <li><code>#define <b>FORCE_LOAD</b></code> - Enables bypassing filtering for a single page using the prefix \"" FORCE_PREFIX "\".</li>\n");
#else /* ifndef FORCE_LOAD */
   b = strsav(b, "  <li><code>#undef <b>FORCE_LOAD</b></code> - Disables bypassing filtering for a single page.</li>\n");
#endif /* ndef FORCE_LOAD */

#ifdef DENY_GZIP
   b = strsav(b, "  <li><code>#define <b>DENY_GZIP</b></code> - Prevents requests from being compressed - required for PCRS.</li>\n");
#else /* ifndef DENY_GZIP */
   b = strsav(b, "  <li><code>#undef <b>DENY_GZIP</b></code> - Allows requests to be compressed if the browser and server support it.</li>\n");
#endif /* ndef DENY_GZIP */

#ifdef STATISTICS
   b = strsav(b, "  <li><code>#define <b>STATISTICS</b></code> - Enables statistics function.</li>\n");
#else /* ifndef STATISTICS */
   b = strsav(b, "  <li><code>#undef <b>STATISTICS</b></code> - Disables statistics function.</li>\n");
#endif /* ndef STATISTICS */

#ifdef SPLIT_PROXY_ARGS
   b = strsav(b, "  <li><code>#define <b>SPLIT_PROXY_ARGS</b></code> - Split this page up by placing the configuration files on separate pages.</li>\n");
#else /* ifndef SPLIT_PROXY_ARGS */
   b = strsav(b, "  <li><code>#undef <b>SPLIT_PROXY_ARGS</b></code> - This page contains the text of the configuration files, they are not split onto separate pages.</li>\n");
#endif /* ndef SPLIT_PROXY_ARGS */

#ifdef KILLPOPUPS
   b = strsav(b, "  <li><code>#define <b>KILLPOPUPS</b></code> - Enables killing JavaScript popups.</li>\n");
#else /* ifndef KILLPOPUPS */
   b = strsav(b, "  <li><code>#undef <b>KILLPOPUPS</b></code> - Disables killing JavaScript popups.</li>\n");
#endif /* ndef KILLPOPUPS */

#ifdef WEBDAV
   b = strsav(b, "  <li><code>#define <b>WEBDAV</b></code> - Enables support for webDAV - e.g. stops Microsoft Outlook from accessing HotMail e-mail.</li>\n");
#else /* ifndef WEBDAV */
   b = strsav(b, "  <li><code>#undef <b>WEBDAV</b></code> - Disables support for webDAV - e.g. so Microsoft Outlook can access HotMail e-mail.</li>\n");
#endif /* ndef WEBDAV */

#ifdef DETECT_MSIE_IMAGES
   b = strsav(b, "  <li><code>#define <b>DETECT_MSIE_IMAGES</b></code> - Enables detecting image requests automatically for MSIE.</li>\n");
#else /* ifndef DETECT_MSIE_IMAGES */
   b = strsav(b, "  <li><code>#undef <b>DETECT_MSIE_IMAGES</b></code> - Disables detecting image requests automatically for MSIE.</li>\n");
#endif /* ndef DETECT_MSIE_IMAGES */

#ifdef IMAGE_BLOCKING
   b = strsav(b, "  <li><code>#define <b>IMAGE_BLOCKING</b></code> - Enables sending \"blocked\" images instead of HTML.</li>\n");
#else /* ifndef IMAGE_BLOCKING */
   b = strsav(b, "  <li><code>#undef <b>IMAGE_BLOCKING</b></code> - Disables sending \"blocked\" images instead of HTML.</li>\n");
#endif /* ndef IMAGE_BLOCKING */

#ifdef ACL_FILES
   b = strsav(b, "  <li><code>#define <b>ACL_FILES</b></code> - Enables the use of ACL files to control access to the proxy by IP address.</li>\n");
#else /* ifndef ACL_FILES */
   b = strsav(b, "  <li><code>#undef <b>ACL_FILES</b></code> - Disables the use of ACL files to control access to the proxy by IP address.</li>\n");
#endif /* ndef ACL_FILES */

#ifdef TRUST_FILES
   b = strsav(b, "  <li><code>#define <b>TRUST_FILES</b></code> - Enables the use of trust files.</li>\n");
#else /* ifndef TRUST_FILES */
   b = strsav(b, "  <li><code>#undef <b>TRUST_FILES</b></code> - Disables the use of trust files.</li>\n");
#endif /* ndef TRUST_FILES */

#ifdef JAR_FILES
   b = strsav(b, "  <li><code>#define <b>JAR_FILES</b></code> - Enables the use of jar files to capture cookies.</li>\n");
#else /* ifndef JAR_FILES */
   b = strsav(b, "  <li><code>#undef <b>JAR_FILES</b></code> - Disables the use of jar files to capture cookies.</li>\n");
#endif /* ndef JAR_FILES */

#ifdef FAST_REDIRECTS
   b = strsav(b, "  <li><code>#define <b>FAST_REDIRECTS</b></code> - Enables intercepting remote script redirects.</li>\n");
#else /* ifndef FAST_REDIRECTS */
   b = strsav(b, "  <li><code>#undef <b>FAST_REDIRECTS</b></code> - Disables intercepting remote script redirects.</li>\n");
#endif /* ndef FAST_REDIRECTS */

   b = strsav(b, "</ul>\n<br>\n");

   config->proxy_args_trailer = b;

   return b;
}


/*
  Local Variables:
  tab-width: 3
  end:
*/
