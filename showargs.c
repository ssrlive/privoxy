const char showargs_rcs[] = "$Id: showargs.c,v 1.1 2001/05/13 21:57:07 administrator Exp $";
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/showargs.c,v $
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
 *
 *********************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

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
 * Function    :  strsav
 *
 * Description :  Reallocate "old" and append text to it.  This makes
 *                it easier to append to malloc'd strings.
 *
 * Parameters  :
 *          1  :  old = Old text that is to be extended.  Will be
 *                free()d by this routine.
 *          2  :  text_to_append = Text to be appended to old.
 *
 * Returns     :  Pointer to newly malloc'ed appended string.
 *                If there is no text to append, return old.  Caller
 *                must free().
 *
 *********************************************************************/
char *strsav(char *old, const char *text_to_append)
{
   int old_len, new_len;
   char *p;

   if (( text_to_append == NULL) || (*text_to_append == '\0'))
   {
      return(old);
   }

   if (NULL != old)
   {
      old_len = strlen(old);
   }
   else
   {
      old_len = 0;
   }

   new_len = old_len + strlen(text_to_append) + 1;

   if (old)
   {
      if ((p = realloc(old, new_len)) == NULL)
      {
         log_error(LOG_LEVEL_ERROR, "realloc(%d) bytes for proxy_args failed!", new_len);
         exit(1);
      }
   }
   else
   {
      if ((p = (char *)malloc(new_len)) == NULL)
      {
         log_error(LOG_LEVEL_ERROR, "malloc(%d) bytes for proxy_args failed!", new_len);
         exit(1);
      }
   }

   strcpy(p + old_len, text_to_append);
   return(p);

}


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
void savearg(char *c, char *o)
{
   char buf[BUFSIZ];

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

   proxy_args->invocation = strsav(proxy_args->invocation, buf);

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
void init_proxy_args(int argc, const char *argv[])
{
   const struct gateway *g;
   int i;

   freez(proxy_args->header);
   freez(proxy_args->invocation);
   freez(proxy_args->gateways);
   freez(proxy_args->trailer);
   

   proxy_args->header = strsav(proxy_args->header,
      "HTTP/1.0 200 OK\n"
      "Server: IJ/" VERSION "\n"
      "Content-type: text/html\n\n"

      "<html>"
      "<head>"
      "<title>Internet Junkbuster Proxy Status</title>"
      "</head>\n"
      "<body bgcolor=\"#f8f8f0\" link=\"#000078\" alink=\"#ff0022\" vlink=\"#787878\">\n"
      "<center>\n"
      "<h1>" BANNER "\n"
      "<a href=\"" REDIRECT_URL "faq#show\">Proxy Status</a>\n"
      "</h1></center>\n"
      "<h2>You are using the " BANNER " <sup><small><small>TM</small></small></sup></h2>\n"
      "Version: " VERSION "\n"
      "<br>Home page: <a href=\"" HOME_PAGE_URL "\">" HOME_PAGE_URL "</a>\n"
      "<p>\n"
   );

   proxy_args->header = strsav(proxy_args->header,
      "<h2>The program was invoked as follows</h2>\n");

   for (i=0; i < argc; i++)
   {
      proxy_args->header = strsav(proxy_args->header, argv[i]);
      proxy_args->header = strsav(proxy_args->header, " ");
   }
   proxy_args->header = strsav(proxy_args->header, "<br>\n");


   proxy_args->invocation = strsav(
      proxy_args->invocation,
      "<br>\n"
      "and the following options were set in the configuration file"
      "<br><br>\n"
   );


   proxy_args->gateways = strsav(proxy_args->gateways,
      "<h2>It supports the following gateway protocols:</h2>\n");

   for (g = gateways; g->name; g++)
   {
      proxy_args->gateways = strsav(proxy_args->gateways, g->name);
      proxy_args->gateways = strsav(proxy_args->gateways, " ");
   }
   proxy_args->gateways = strsav(proxy_args->gateways, "<br>\n");

}


/*********************************************************************
 *
 * Function    :  end_proxy_args
 *
 * Description :  Create the "bottom" of the show-proxy-args page.
 *
 * Parameters  :  None
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void end_proxy_args(void)
{
   char *b = NULL;
   char buf[BUFSIZ];

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
   SHOW_RCS(w32log_h_rcs)
   SHOW_RCS(w32log_rcs)
   SHOW_RCS(w32res_h_rcs)
   SHOW_RCS(w32rulesdlg_h_rcs)
   SHOW_RCS(w32rulesdlg_rcs)
   SHOW_RCS(w32taskbar_h_rcs)
   SHOW_RCS(w32taskbar_rcs)
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

#ifdef USE_IMAGE_LIST
   b = strsav(b, "  <li><code>#define <b>USE_IMAGE_LIST</b></code> - Enables using image list to detect images.</li>\n");
#else /* ifndef USE_IMAGE_LIST */
   b = strsav(b, "  <li><code>#undef <b>USE_IMAGE_LIST</b></code> - Disables using image list to detect images.</li>\n");
#endif /* ndef USE_IMAGE_LIST */

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

   b = strsav(b, "</ul>\n<br>\n");

   b = strsav(b,
      "<small><small><p>\n"
      "Code and documentation of the " BANNER " Proxy"
      "<sup><small>TM</small></sup>\n"
      "<a href=\"http://www.junkbusters.com/ht/en/legal.html#copy\">\n" "Copyright</a>&#169; 1997 Junkbusters Corporation\n"
      "<a href=\"http://www.junkbusters.com/ht/en/legal.html#marks\"><sup><small>TM</small></sup></a><br>\n"
      "Copying and distribution permitted under the"
      "<a href=\"http://www.gnu.org/copyleft/gpl.html\">\n"
      "<small>GNU</small></a> "
      "General Public License.\n"
      "</small>"
      "<address><kbd>webmaster@junkbusters.com</kbd></address>"
      "</small>"
      "</body></html>\n"
   );

   proxy_args->trailer = b;

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
