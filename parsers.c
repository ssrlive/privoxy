const char parsers_rcs[] = "$Id: parsers.c,v 1.13 2001/05/31 21:30:33 jongfoster Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/parsers.c,v $
 *
 * Purpose     :  Declares functions to parse/crunch headers and pages.
 *                Functions declared include:
 *                   `add_to_iob', `client_cookie_adder', `client_from',
 *                   `client_referrer', `client_send_cookie', `client_ua',
 *                   `client_uagent', `client_x_forwarded',
 *                   `client_x_forwarded_adder', `client_xtra_adder',
 *                   `content_type', `crumble', `destroy_list', `enlist',
 *                   `flush_socket', `free_http_request', `get_header',
 *                   `list_to_text', `parse_http_request', `sed',
 *                   and `server_set_cookie'.
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
 *    $Log: parsers.c,v $
 *    Revision 1.13  2001/05/31 21:30:33  jongfoster
 *    Removed list code - it's now in list.[ch]
 *    Renamed "permission" to "action", and changed many features
 *    to use the actions file rather than the global config.
 *
 *    Revision 1.12  2001/05/31 17:33:13  oes
 *
 *    CRLF -> LF
 *
 *    Revision 1.11  2001/05/29 20:11:19  joergs
 *    '/* inside comment' warning removed.
 *
 *    Revision 1.10  2001/05/29 09:50:24  jongfoster
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
 *    Revision 1.9  2001/05/28 17:26:33  jongfoster
 *    Fixing segfault if last header was crunched.
 *    Fixing Windows build (snprintf() is _snprintf() under Win32, but we
 *    can use the cross-platform sprintf() instead.)
 *
 *    Revision 1.8  2001/05/27 22:17:04  oes
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
 *    Revision 1.7  2001/05/27 13:19:06  oes
 *    Patched Joergs solution for the content-length in.
 *
 *    Revision 1.6  2001/05/26 13:39:32  jongfoster
 *    Only crunches Content-Length header if applying RE filtering.
 *    Without this fix, Microsoft Windows Update wouldn't work.
 *
 *    Revision 1.5  2001/05/26 00:28:36  jongfoster
 *    Automatic reloading of config file.
 *    Removed obsolete SIGHUP support (Unix) and Reload menu option (Win32).
 *    Most of the global variables have been moved to a new
 *    struct configuration_spec, accessed through csp->config->globalname
 *    Most of the globals remaining are used by the Win32 GUI.
 *
 *    Revision 1.4  2001/05/22 18:46:04  oes
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
 *    Revision 1.2  2001/05/17 23:02:36  oes
 *     - Made referrer option accept 'L' as a substitute for '§'
 *
 *    Revision 1.1.1.1  2001/05/15 13:59:01  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "project.h"
#include "list.h"
#include "parsers.h"
#include "encode.h"
#include "filters.h"
#include "loaders.h"
#include "showargs.h"
#include "jcc.h"
#include "ssplit.h"
#include "errlog.h"
#include "jbsockets.h"
#include "miscutil.h"

const char parsers_h_rcs[] = PARSERS_H_VERSION;

/* Fix a problem with Solaris.  There should be no effect on other
 * platforms.
 * Solaris's isspace() is a macro which uses it's argument directly
 * as an array index.  Therefore we need to make sure that high-bit
 * characters generate +ve values, and ideally we also want to make
 * the argument match the declared parameter type of "int".
 * 
 * Why did they write a character function that can't take a simple 
 * "char" argument?  Doh!
 */
#define ijb_isupper(__X) isupper((int)(unsigned char)(__X))
#define ijb_tolower(__X) tolower((int)(unsigned char)(__X))


const struct parsers client_patterns[] = {
   { "referer:",                 8,    client_referrer },
   { "user-agent:",              11,   client_uagent },
   { "ua-",                      3,    client_ua },
   { "from:",                    5,    client_from },
   { "cookie:",                  7,    client_send_cookie },
   { "x-forwarded-for:",         16,   client_x_forwarded },
   { "proxy-connection:",        17,   crumble },
#ifdef DENY_GZIP
   { "Accept-Encoding: gzip",    21,   crumble },
#endif /* def DENY_GZIP */
#if defined(DETECT_MSIE_IMAGES)
   { "Accept:",                   7,   client_accept },
#endif /* defined(DETECT_MSIE_IMAGES) */
#ifdef FORCE_LOAD
   { "Host:",                     5,   client_host },
#endif /* def FORCE_LOAD */
/* { "if-modified-since:",       18,   crumble }, */
   { NULL,                       0,    NULL }
};

const struct interceptors intercept_patterns[] = {
   { "show-proxy-args",    14, show_proxy_args },
   { "ijb-send-banner",    14, ijb_send_banner },
#ifdef TRUST_FILES
   { "ij-untrusted-url",   15, ij_untrusted_url },
#endif /* def TRUST_FILES */
   { "show-url-info",      13, ijb_show_url_info },
   { NULL, 0, NULL }
};

const struct parsers server_patterns[] = {
   { "set-cookie:",        11, server_set_cookie },
   { "connection:",        11, crumble },
#if defined(PCRS) || defined(KILLPOPUPS)
   { "Content-Type:",      13, content_type },
#endif /* defined(PCRS) || defined(KILLPOPUPS) */
#ifdef PCRS
   { "Content-Length:",    15, content_length },
#endif /* def PCRS */
   { NULL, 0, NULL }
};


void (* const add_client_headers[])(struct client_state *) = {
   client_cookie_adder,
   client_x_forwarded_adder,
   client_xtra_adder,
   NULL
};


void (* const add_server_headers[])(struct client_state *) = {
   NULL
};


/*********************************************************************
 *
 * Function    :  flush_socket
 *
 * Description :  Write any pending "buffered" content.
 *
 * Parameters  :
 *          1  :  fd = file descriptor of the socket to read
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  On success, the number of bytes written are returned (zero
 *                indicates nothing was written).  On error, -1 is returned,
 *                and errno is set appropriately.  If count is zero and the
 *                file descriptor refers to a regular file, 0 will be
 *                returned without causing any other effect.  For a special
 *                file, the results are not portable.
 *
 *********************************************************************/
int flush_socket(int fd, struct client_state *csp)
{
   struct iob *iob = csp->iob;
   int n = iob->eod - iob->cur;

   if (n <= 0)
   {
      return(0);
   }

   n = write_socket(fd, iob->cur, n);
   iob->eod = iob->cur = iob->buf;
   return(n);

}


/*********************************************************************
 *
 * Function    :  add_to_iob
 *
 * Description :  Add content to the buffered page.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  buf = holds the content to be added to the page
 *          3  :  n = number of bytes to be added
 *
 * Returns     :  Number of bytes in the content buffer.
 *
 *********************************************************************/
int add_to_iob(struct client_state *csp, char *buf, int n)
{
   struct iob *iob = csp->iob;
   int have, need;
   char *p;

   have = iob->eod - iob->cur;

   if (n <= 0)
   {
      return(have);
   }

   need = have + n;

   if ((p = (char *)malloc(need + 1)) == NULL)
   {
      log_error(LOG_LEVEL_ERROR, "malloc() iob failed: %E");
      return(-1);
   }

   if (have)
   {
      /* there is something in the buffer - save it */
      memcpy(p, iob->cur, have);

      /* replace the buffer with the new space */
      freez(iob->buf);
      iob->buf = p;

      /* point to the end of the data */
      p += have;
   }
   else
   {
      /* the buffer is empty, free it and reinitialize */
      freez(iob->buf);
      iob->buf = p;
   }

   /* copy the new data into the iob buffer */
   memcpy(p, buf, n);

   /* point to the end of the data */
   p += n;

   /* null terminate == cheap insurance */
   *p = '\0';

   /* set the pointers to the new values */
   iob->cur = iob->buf;
   iob->eod = p;

   return(need);

}


/*********************************************************************
 *
 * Function    :  get_header
 *
 * Description :  This (odd) routine will parse the csp->iob
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Any one of the following:
 *
 * 1) a pointer to a dynamically allocated string that contains a header line
 * 2) NULL  indicating that the end of the header was reached
 * 3) ""    indicating that the end of the iob was reached before finding
 *          a complete header line.
 *
 *********************************************************************/
char *get_header(struct client_state *csp)
{
   struct iob *iob;
   char *p, *q, *ret;
   iob = csp->iob;

   if ((iob->cur == NULL)
      || ((p = strchr(iob->cur, '\n')) == NULL))
   {
      return(""); /* couldn't find a complete header */
   }

   *p = '\0';

   ret = strdup(iob->cur);

   iob->cur = p+1;

   if ((q = strchr(ret, '\r'))) *q = '\0';

   /* is this a blank linke (i.e. the end of the header) ? */
   if (*ret == '\0')
   {
      freez(ret);
      return(NULL);
   }

   return(ret);

}


/*********************************************************************
 *
 * Function    :  sed
 *
 * Description :  add, delete or modify lines in the HTTP header streams.
 *                On entry, it receives a linked list of headers space
 *                that was allocated dynamically (both the list nodes
 *                and the header contents).
 *
 *                As a side effect it frees the space used by the original
 *                header lines.
 *
 * Parameters  :
 *          1  :  pats = list of patterns to match against headers
 *          2  :  more_headers = list of functions to add more
 *                headers (client or server)
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Single pointer to a fully formed header.
 *
 *********************************************************************/
char *sed(const struct parsers pats[], void (* const more_headers[])(struct client_state *), struct client_state *csp)
{
   struct list *p;
   const struct parsers *v;
   char *hdr;
   void (* const *f)();

   for (v = pats; v->str ; v++)
   {
      for (p = csp->headers->next; p ; p = p->next)
      {
         /* Header crunch()ed in previous run? -> ignore */
         if (p->str == NULL) continue;

         if (v == pats) log_error(LOG_LEVEL_HEADER, "scan: %s", p->str);

         if (strncmpic(p->str, v->str, v->len) == 0)
         {
            hdr = v->parser(v, p->str, csp);
            freez(p->str);
            p->str = hdr;
         }
      }
   }

   /* place any additional headers on the csp->headers list */
   for (f = more_headers; *f ; f++)
   {
      (*f)(csp);
   }

   /* add the blank line at the end of the header, if necessary */
   if ( (csp->headers->last == NULL)
     || (csp->headers->last->str == NULL)
     || (*csp->headers->last->str != '\0') )
   {
      enlist(csp->headers, "");
   }

   hdr = list_to_text(csp->headers);

   return(hdr);

}


/*********************************************************************
 *
 * Function    :  free_http_request
 *
 * Description :  Freez a http_request structure
 *
 * Parameters  :
 *          1  :  http = points to a http_request structure to free
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void free_http_request(struct http_request *http)
{
   freez(http->cmd);
   freez(http->gpc);
   freez(http->host);
   freez(http->hostport);
   freez(http->path);
   freez(http->ver);

}


/*********************************************************************
 *
 * Function    :  parse_http_request
 *
 * Description :  Parse out the host and port from the URL.  Find the
 *                hostname & path, port (if ':'), and/or password (if '@')
 *
 * Parameters  :
 *          1  :  req = URL (or is it URI?) to break down
 *          2  :  http = pointer to the http structure to hold elements
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void parse_http_request(char *req, struct http_request *http, struct client_state *csp)
{
   char *buf, *v[10], *url, *p;
   int n;

   memset(http, '\0', sizeof(*http));

   http->cmd = strdup(req);

   buf = strdup(req);

   n = ssplit(buf, " \r\n", v, SZ(v), 1, 1);

   if (n == 3)
   {
      /* this could be a CONNECT request */
      if (strcmpic(v[0], "connect") == 0)
      {
         http->ssl      = 1;
         http->gpc      = strdup(v[0]);
         http->hostport = strdup(v[1]);
         http->ver      = strdup(v[2]);
      }

#ifdef WEBDAV

/* This next line is a little ugly, but it simplifies the if statement below. */
/* Basically if using webDAV, we want the OR condition to use these too.      */

/*
 * by haroon
 * These are the headers as defined in RFC2518 to add webDAV support
 */

#define OR_WEBDAV || \
         (0 == strcmpic(v[0], "propfind")) || \
         (0 == strcmpic(v[0], "proppatch")) || \
         (0 == strcmpic(v[0], "move")) || \
         (0 == strcmpic(v[0], "copy")) || \
         (0 == strcmpic(v[0], "mkcol")) || \
         (0 == strcmpic(v[0], "lock")) || \
         (0 == strcmpic(v[0], "unlock"))

#else /* No webDAV support is enabled.  Provide an empty OR_WEBDAV macro. */

#define OR_WEBDAV

#endif

      /* or it could be a GET or a POST (possibly webDAV too) */
      if ((strcmpic(v[0], "get")  == 0) ||
          (strcmpic(v[0], "head") == 0) OR_WEBDAV ||
          (strcmpic(v[0], "post") == 0))
      {
         http->ssl      = 0;
         http->gpc      = strdup(v[0]);
         url            = v[1];
         http->ver      = strdup(v[2]);

         if (strncmpic(url, "http://",  7) == 0)
         {
            url += 7;
         }
         else if (strncmpic(url, "https://", 8) == 0)
         {
            url += 8;
         }
         else
         {
            url = NULL;
         }

         if (url && (p = strchr(url, '/')))
         {
            http->path = strdup(p);
            *p = '\0';
            http->hostport = strdup(url);
         }
      }
   }

   freez(buf);


   if (http->hostport == NULL)
   {
      free_http_request(http);
      return;
   }

   buf = strdup(http->hostport);


   /* check if url contains password */
   n = ssplit(buf, "@", v, SZ(v), 1, 1);
   if (n == 2)
   {
      char * newbuf = NULL;
      newbuf = strdup(v[1]);
      freez(buf);
      buf = newbuf;
   }

   n = ssplit(buf, ":", v, SZ(v), 1, 1);

   if (n == 1)
   {
      http->host = strdup(v[0]);
      http->port = 80;
   }

   if (n == 2)
   {
      http->host = strdup(v[0]);
      http->port = atoi(v[1]);
   }

   freez(buf);

   if (http->host == NULL)
   {
      free_http_request(http);
   }

   if (http->path == NULL)
   {
      http->path = strdup("");
   }

}


/* here begins the family of parser functions that reformat header lines */


/*********************************************************************
 *
 * Function    :  crumble
 *
 * Description :  This is called if a header matches a pattern to "crunch"
 *
 * Parameters  :
 *          1  :  v = Pointer to parsers structure, which basically holds
 *                headers (client or server) that we want to "crunch"
 *          2  :  s = header (from sed) to "crunch"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Always NULL.
 *
 *********************************************************************/
char *crumble(const struct parsers *v, char *s, struct client_state *csp)
{
   log_error(LOG_LEVEL_HEADER, "crunch!");
   return(NULL);

}


#if defined(PCRS) || defined(KILLPOPUPS)

/*********************************************************************
 *
 * Function    :  content_type
 *
 * Description :  Is this a text/.* or javascript MIME Type?
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header string we are "considering"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A duplicate string pointer to this header (ie. pass thru)
 *
 *********************************************************************/
char *content_type(const struct parsers *v, char *s, struct client_state *csp)
{
   if (strstr (s, " text/") || strstr (s, "application/x-javascript"))
      csp->is_text = 1;
   else
      csp->is_text = 0;

   return(strdup(s));

}
#endif /* defined(PCRS) || defined(KILLPOPUPS) */


#ifdef PCRS
/*********************************************************************
 *
 * Function    :  content_length
 *
 * Description :  Crunch Content-Length header if & only if we are
 *                filtering this page through PCRS.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header string we are "considering"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A duplicate string pointer to this header (ie. pass thru)
 *
 *********************************************************************/
char *content_length(const struct parsers *v, char *s, struct client_state *csp)
{
   if (csp->content_length != 0) /* Content has been modified */
	{
	   s = (char *) zalloc(100);
	   sprintf(s, "Content-Length: %d", csp->content_length);
		log_error(LOG_LEVEL_HEADER, "Adjust Content-Length to %d", csp->content_length);
	   return(s);
	}
   else
   {
      return(strdup(s));
   }
}

#endif /* def PCRS */


/*********************************************************************
 *
 * Function    :  client_referrer
 *
 * Description :  Handle the "referer" config setting properly.
 *                Called from `sed'.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header (from sed) to "crunch"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  NULL if crunched, or a malloc'ed string with the original
 *                or modified header
 *
 *********************************************************************/
char *client_referrer(const struct parsers *v, char *s, struct client_state *csp)
{
   const char * newval;
#ifdef FORCE_LOAD
   /* Since the referrer can include the prefix even
    * even if the request itself is non-forced, we must
    * clean it unconditionally 
    */
   strclean(s, FORCE_PREFIX);
#endif /* def FORCE_LOAD */

#ifdef TRUST_FILES
   csp->referrer = strdup(s);
#endif /* def TRUST_FILES */

   /*
    * Are we sending referer?
    */
   if ((csp->action->flags & ACTION_HIDE_REFERER) == 0)
   {
      return(strdup(s));
   }

   newval = csp->action->string[ACTION_STRING_REFERER];

   /*
    * Are we blocking referer?
    */
   if ((newval == NULL) || (0 == strcmpic(newval, "block")) )
   {
      log_error(LOG_LEVEL_HEADER, "crunch!");
      return(NULL);
   }

   /*
    * Are we forging referer?
    */
   if (0 == strcmpic(newval, "forge"))
   {
      /*
       * Forge a referer as http://[hostname:port of REQUEST]/
       * to fool stupid checks for in-site links
       */
      log_error(LOG_LEVEL_HEADER, "crunch+forge!");
      s = strsav(NULL, "Referer: ");
      s = strsav(s, "http://");
      s = strsav(s, csp->http->hostport);
      s = strsav(s, "/");
      return(s);
   }

   /*
    * Have we got a fixed referer?
    */
   if (0 == strncmpic(newval, "http://", 7))
   {
      /*
       * We have a specific (fixed) referer we want to send.
       */

      log_error(LOG_LEVEL_HEADER, "modified");

      s = strsav( NULL, "Referer: " );
      s = strsav( s, newval );
      return(s);
   }

   /* Should never get here! */
   log_error(LOG_LEVEL_ERROR, "Bad parameter: +referer{%s}", newval);

   /*
    * Forge is probably the best default.
    *
    * Forge a referer as http://[hostname:port of REQUEST]/
    * to fool stupid checks for in-site links
    */
   log_error(LOG_LEVEL_HEADER, "crunch+forge!");
   s = strsav(NULL, "Referer: ");
   s = strsav(s, "http://");
   s = strsav(s, csp->http->hostport);
   s = strsav(s, "/");
   return(s);
}


/*********************************************************************
 *
 * Function    :  client_uagent
 *
 * Description :  Handle the "user-agent" config setting properly.
 *                Called from `sed'.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header (from sed) to "crunch"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A malloc'ed pointer to the default agent, or
 *                a malloc'ed string pointer to this header (ie. pass thru).
 *
 *********************************************************************/
char *client_uagent(const struct parsers *v, char *s, struct client_state *csp)
{
   const char * newval;

#ifdef DETECT_MSIE_IMAGES
   if (strstr (s, "MSIE "))
   {
      /* This is Microsoft Internet Explorer.
       * Enable auto-detect.
       */
      csp->accept_types |= ACCEPT_TYPE_IS_MSIE;
   }
#endif /* def DETECT_MSIE_IMAGES */

   if ((csp->action->flags & ACTION_HIDE_USER_AGENT) == 0)
   {
      return(strdup(s));
   }

   newval = csp->action->string[ACTION_STRING_USER_AGENT];
   if (newval == NULL)
   {
      return(strdup(s));
   }

   log_error(LOG_LEVEL_HEADER, "modified");

   s = strsav( NULL, "User-Agent: " );
   s = strsav( s, newval );
   return(s);

}


/*********************************************************************
 *
 * Function    :  client_ua
 *
 * Description :  Handle "ua-" headers properly.  Called from `sed'.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header (from sed) to "crunch"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  NULL if crunched, or a malloc'ed string to original header
 *
 *********************************************************************/
char *client_ua(const struct parsers *v, char *s, struct client_state *csp)
{
   if ((csp->action->flags & ACTION_HIDE_USER_AGENT) == 0)
   {
      return(strdup(s));
   }
   else
   {
      log_error(LOG_LEVEL_HEADER, "crunch!");
      return(NULL);
   }
}


/*********************************************************************
 *
 * Function    :  client_from
 *
 * Description :  Handle the "from" config setting properly.
 *                Called from `sed'.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header (from sed) to "crunch"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  NULL if crunched, or a malloc'ed string to
 *                modified/original header.
 *
 *********************************************************************/
char *client_from(const struct parsers *v, char *s, struct client_state *csp)
{
   const char * newval;

   if ((csp->action->flags & ACTION_HIDE_FROM) == 0)
   {
      return(strdup(s));
   }

   newval = csp->action->string[ACTION_STRING_FROM];

   /*
    * Are we blocking referer?
    */
   if ((newval == NULL) || (0 == strcmpic(newval, "block")) )
   {
      log_error(LOG_LEVEL_HEADER, "crunch!");
      return(NULL);
   }

   log_error(LOG_LEVEL_HEADER, " modified");

   s = strsav( NULL, "From: " );
   s = strsav( s, newval );
   return(s);

}


/*********************************************************************
 *
 * Function    :  client_send_cookie
 *
 * Description :  Handle the "cookie" header properly.  Called from `sed'.
 *                If cookie is accepted, add it to the cookie_list,
 *                else we crunch it.  Mmmmmmmmmmm ... cookie ......
 *
 * Parameters  :
 *          1  :  v = pattern of cookie `sed' found matching
 *          2  :  s = header (from sed) to "crunch"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Always NULL.
 *
 *********************************************************************/
char *client_send_cookie(const struct parsers *v, char *s, struct client_state *csp)
{
   if ((csp->action->flags & ACTION_NO_COOKIE_READ) == 0)
   {
      enlist(csp->cookie_list, s + v->len + 1);
   }
   else
   {
      log_error(LOG_LEVEL_HEADER, " crunch!");
   }

   /*
    * Always return NULL here.  The cookie header
    * will be sent at the end of the header.
    */
   return(NULL);

}


/*********************************************************************
 *
 * Function    :  client_x_forwarded
 *
 * Description :  Handle the "x-forwarded-for" config setting properly,
 *                also used in the add_client_headers list.  Called from `sed'.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header (from sed) to "crunch"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Always NULL.
 *
 *********************************************************************/
char *client_x_forwarded(const struct parsers *v, char *s, struct client_state *csp)
{
   if ((csp->action->flags & ACTION_HIDE_FORWARDED) == 0)
   {
      /* Save it so we can re-add it later */
      csp->x_forwarded = strdup(s);
   }

   /*
    * Always return NULL, since this information
    * will be sent at the end of the header.
    */

   return(NULL);

}

#if defined(DETECT_MSIE_IMAGES)
/*********************************************************************
 *
 * Function    :  client_accept
 *
 * Description :  Detect whether the client wants HTML or an image.
 *                Clients do not always make this information available
 *                in a sane way.  Always passes the header through
 *                the proxy unchanged.
 *
 * Parameters  :
 *          1  :  v = Ignored.
 *          2  :  s = Header string.  Null terminated.
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Duplicate of argument s.
 *
 *********************************************************************/
char *client_accept(const struct parsers *v, char *s, struct client_state *csp)
{
#ifdef DETECT_MSIE_IMAGES
   if (strstr (s, "image/gif"))
   {
      /* Client will accept HTML.  If this seems counterintuitive,
       * blame Microsoft. 
       */
      csp->accept_types |= ACCEPT_TYPE_MSIE_HTML;
   }
   else
   {
      csp->accept_types |= ACCEPT_TYPE_MSIE_IMAGE;
   }
#endif /* def DETECT_MSIE_IMAGES */

   return(strdup(s));

}
#endif /* defined(DETECT_MSIE_IMAGES) */



/* the following functions add headers directly to the header list */


/*********************************************************************
 *
 * Function    :  client_cookie_adder
 *
 * Description :  Used in the add_client_headers list.  Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void client_cookie_adder(struct client_state *csp)
{
   struct list *lst;
   char *tmp = NULL;
   char *e;

   for (lst = csp->cookie_list->next; lst ; lst = lst->next)
   {
      if (tmp)
      {
         tmp = strsav(tmp, "; ");
      }
      tmp = strsav(tmp, lst->str);
   }

   for (lst = csp->action->multi[ACTION_MULTI_WAFER]->next;  lst ; lst = lst->next)
   {
      if (tmp)
      {
         tmp = strsav(tmp, "; ");
      }

      if ((e = cookie_encode(lst->str)))
      {
         tmp = strsav(tmp, e);
         freez(e);
      }
   }

   if (tmp)
   {
      char *ret;

      ret = strdup("Cookie: ");
      ret = strsav(ret, tmp);
      log_error(LOG_LEVEL_HEADER, "addh: %s", ret);
      enlist(csp->headers, ret);
      freez(tmp);
      freez(ret);
   }

}


/*********************************************************************
 *
 * Function    :  client_xtra_adder
 *
 * Description :  Used in the add_client_headers list.  Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void client_xtra_adder(struct client_state *csp)
{
   struct list *lst = csp->action->multi[ACTION_MULTI_ADD_HEADER];

   for (lst = lst->next; lst ; lst = lst->next)
   {
      log_error(LOG_LEVEL_HEADER, "addh: %s", lst->str);
      enlist(csp->headers, lst->str);
   }

}


/*********************************************************************
 *
 * Function    :  client_x_forwarded_adder
 *
 * Description :  Used in the add_client_headers list.  Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void client_x_forwarded_adder(struct client_state *csp)
{
   char *p = NULL;

   if ((csp->action->flags & ACTION_HIDE_FORWARDED) != 0)
   {
      return;
   }

   if (csp->x_forwarded)
   {
      p = strsav(p, csp->x_forwarded);
      p = strsav(p, ", ");
      p = strsav(p, csp->ip_addr_str);
   }
   else
   {
      p = strsav(p, "X-Forwarded-For: ");
      p = strsav(p, csp->ip_addr_str);
   }

   log_error(LOG_LEVEL_HEADER, "addh: %s", p);
   enlist(csp->headers, p);

}


/*********************************************************************
 *
 * Function    :  server_set_cookie
 *
 * Description :  Handle the server "cookie" header properly.
 *                Log cookie to the jar file.  Then "crunch" it,
 *                or accept it.  Called from `sed'.
 *
 * Parameters  :
 *          1  :  v = parser pattern that matched this header
 *          2  :  s = header that matched this pattern
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  `crumble' or a newly malloc'ed string.
 *
 *********************************************************************/
char *server_set_cookie(const struct parsers *v, char *s, struct client_state *csp)
{
#ifdef JAR_FILES
   if (csp->config->jar)
   {
      fprintf(csp->config->jar, "%s\t%s\n", csp->http->host, (s + v->len + 1));
   }
#endif /* def JAR_FILES */

   if ((csp->action->flags & ACTION_NO_COOKIE_SET) != 0)
   {
      return(crumble(v, s, csp));
   }

   return(strdup(s));

}


#ifdef FORCE_LOAD
/*********************************************************************
 *
 * Function    :  client_host
 *
 * Description :  Clean the FORCE_PREFIX out of the 'host' http
 *                header, if we use force
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header (from sed) to clean
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A malloc'ed pointer to the cleaned host header 
 *
 *********************************************************************/
char *client_host(const struct parsers *v, char *s, struct client_state *csp)
{
   char *cleanhost = strdup(s);
 
   if(csp->force)
      strclean(cleanhost, FORCE_PREFIX);
 
   return(cleanhost);
}
#endif /* def FORCE_LOAD */
 
 
#ifdef FORCE_LOAD 
/*********************************************************************
 *
 * Function    :  strclean
 *
 * Description :  In-Situ-Eliminate all occurances of substring in 
 *                string
 *
 * Parameters  :
 *          1  :  string = string to clean
 *          2  :  substring = substring to eliminate
 *
 * Returns     :  Number of eliminations
 *
 *********************************************************************/
int strclean(const char *string, const char *substring)
{
   int hits = 0, len = strlen(substring);
   char *pos, *p;

   while((pos = strstr(string, substring)))
   {
      p = pos + len;
      do
      {
         *(p - len) = *p; 
      }
      while (*p++ != '\0');

      hits++;
   }

   return(hits);
}
#endif /* def FORCE_LOAD */


/*
  Local Variables:
  tab-width: 3
  end:
*/
