const char parsers_rcs[] = "$Id: parsers.c,v 1.45 2002/01/09 14:33:03 oes Exp $";
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
 *                   `flush_socket', ``get_header', `sed',
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
 *    Revision 1.45  2002/01/09 14:33:03  oes
 *    Added support for localtime_r.
 *
 *    Revision 1.44  2001/12/14 01:22:54  steudten
 *    Remove 'user:pass@' from 'proto://user:pass@host' for the
 *    new added header 'Host: ..'. (See Req ID 491818)
 *
 *    Revision 1.43  2001/11/23 00:26:38  jongfoster
 *    Fixing two really stupid errors in my previous commit
 *
 *    Revision 1.42  2001/11/22 21:59:30  jongfoster
 *    Adding code to handle +no-cookies-keep
 *
 *    Revision 1.41  2001/11/05 23:43:05  steudten
 *    Add time+date to log files.
 *
 *    Revision 1.40  2001/10/26 20:13:09  jongfoster
 *    ctype.h is needed in Windows, too.
 *
 *    Revision 1.39  2001/10/26 17:40:04  oes
 *    Introduced get_header_value()
 *    Removed http->user_agent, csp->referrer and csp->accept_types
 *    Removed client_accept()
 *
 *    Revision 1.38  2001/10/25 03:40:48  david__schmidt
 *    Change in porting tactics: OS/2's EMX porting layer doesn't allow multiple
 *    threads to call select() simultaneously.  So, it's time to do a real, live,
 *    native OS/2 port.  See defines for __EMX__ (the porting layer) vs. __OS2__
 *    (native). Both versions will work, but using __OS2__ offers multi-threading.
 *
 *    Revision 1.37  2001/10/23 21:36:02  jongfoster
 *    Documenting sed()'s error behaviou (doc change only)
 *
 *    Revision 1.36  2001/10/13 12:51:51  joergs
 *    Removed client_host, (was only required for the old 2.0.2-11 http://noijb.
 *    force-load), instead crumble Host: and add it (again) in client_host_adder
 *    (in case we get a HTTP/1.0 request without Host: header and forward it to
 *    a HTTP/1.1 server/proxy).
 *
 *    Revision 1.35  2001/10/09 22:39:21  jongfoster
 *    assert.h is also required under Win32, so moving out of #ifndef _WIN32
 *    block.
 *
 *    Revision 1.34  2001/10/07 18:50:55  oes
 *    Added server_content_encoding, renamed server_transfer_encoding
 *
 *    Revision 1.33  2001/10/07 18:04:49  oes
 *    Changed server_http11 to server_http and its pattern to "HTTP".
 *      Additional functionality: it now saves the HTTP status into
 *      csp->http->status and sets CT_TABOO for Status 206 (partial range)
 *
 *    Revision 1.32  2001/10/07 15:43:28  oes
 *    Removed FEATURE_DENY_GZIP and replaced it with client_accept_encoding,
 *       client_te and client_accept_encoding_adder, triggered by the new
 *       +no-compression action. For HTTP/1.1 the Accept-Encoding header is
 *       changed to allow only identity and chunked, and the TE header is
 *       crunched. For HTTP/1.0, Accept-Encoding is crunched.
 *
 *    parse_http_request no longer does anything than parsing. The rewriting
 *      of http->cmd and version mangling are gone. It now also recognizes
 *      the put and delete methods and saves the url in http->url. Removed
 *      unused variable.
 *
 *    renamed content_type and content_length to have the server_ prefix
 *
 *    server_content_type now only works if csp->content_type != CT_TABOO
 *
 *    added server_transfer_encoding, which
 *      - Sets CT_TABOO to prohibit filtering if encoding compresses
 *      - Raises the CSP_FLAG_CHUNKED flag if Encoding is "chunked"
 *      - Change from "chunked" to "identity" if body was chunked
 *        but has been de-chunked for filtering.
 *
 *    added server_content_md5 which crunches any Content-MD5 headers
 *      if the body was modified.
 *
 *    made server_http11 conditional on +downgrade action
 *
 *    Replaced 6 boolean members of csp with one bitmap (csp->flags)
 *
 *    Revision 1.31  2001/10/05 14:25:02  oes
 *    Crumble Keep-Alive from Server
 *
 *    Revision 1.30  2001/09/29 12:56:03  joergs
 *    IJB now changes HTTP/1.1 to HTTP/1.0 in requests and answers.
 *
 *    Revision 1.29  2001/09/24 21:09:24  jongfoster
 *    Fixing 2 memory leaks that Guy spotted, where the paramater to
 *    enlist() was not being free()d.
 *
 *    Revision 1.28  2001/09/22 16:32:28  jongfoster
 *    Removing unused #includes.
 *
 *    Revision 1.27  2001/09/20 15:45:25  steudten
 *
 *    add casting from size_t to int for printf()
 *    remove local variable shadow s2
 *
 *    Revision 1.26  2001/09/16 17:05:14  jongfoster
 *    Removing unused #include showarg.h
 *
 *    Revision 1.25  2001/09/16 13:21:27  jongfoster
 *    Changes to use new list functions.
 *
 *    Revision 1.24  2001/09/13 23:05:50  jongfoster
 *    Changing the string paramater to the header parsers a "const".
 *
 *    Revision 1.23  2001/09/12 18:08:19  steudten
 *
 *    In parse_http_request() header rewriting miss the host value, so
 *    from http://www.mydomain.com the result was just " / " not
 *    http://www.mydomain.com/ in case we forward.
 *
 *    Revision 1.22  2001/09/10 10:58:53  oes
 *    Silenced compiler warnings
 *
 *    Revision 1.21  2001/07/31 14:46:00  oes
 *     - Persistant connections now suppressed
 *     - sed() no longer appends empty header to csp->headers
 *
 *    Revision 1.20  2001/07/30 22:08:36  jongfoster
 *    Tidying up #defines:
 *    - All feature #defines are now of the form FEATURE_xxx
 *    - Permanently turned off WIN_GUI_EDIT
 *    - Permanently turned on WEBDAV and SPLIT_PROXY_ARGS
 *
 *    Revision 1.19  2001/07/25 17:21:54  oes
 *    client_uagent now saves copy of User-Agent: header value
 *
 *    Revision 1.18  2001/07/13 14:02:46  oes
 *     - Included fix to repair broken HTTP requests that
 *       don't contain a path, not even '/'.
 *     - Removed all #ifdef PCRS
 *     - content_type now always inspected and classified as
 *       text, gif or other.
 *     - formatting / comments
 *
 *    Revision 1.17  2001/06/29 21:45:41  oes
 *    Indentation, CRLF->LF, Tab-> Space
 *
 *    Revision 1.16  2001/06/29 13:32:42  oes
 *    - Fixed a comment
 *    - Adapted free_http_request
 *    - Removed logentry from cancelled commit
 *
 *    Revision 1.15  2001/06/03 19:12:38  oes
 *    deleted const struct interceptors
 *
 *    Revision 1.14  2001/06/01 18:49:17  jongfoster
 *    Replaced "list_share" with "list" - the tiny memory gain was not
 *    worth the extra complexity.
 *
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
 *    '/ * inside comment' warning removed.
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

#ifndef _WIN32
#include <stdio.h>
#include <sys/types.h>
#endif

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#if !defined(_WIN32) && !defined(__OS2__)
#include <unistd.h>
#endif

#include "project.h"
#include "list.h"
#include "parsers.h"
#include "encode.h"
#include "ssplit.h"
#include "errlog.h"
#include "jbsockets.h"
#include "miscutil.h"
#include "list.h"

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
   { "Accept-Encoding:",         16,   client_accept_encoding },
   { "TE:",                      3,    client_te },
   { "Host:",                     5,   crumble },
/* { "if-modified-since:",       18,   crumble }, */
   { "Keep-Alive:",              11,   crumble },
   { "connection:",              11,   crumble },
   { "proxy-connection:",        17,   crumble },
   { NULL,                       0,    NULL }
};


const struct parsers server_patterns[] = {
   { "HTTP",                4, server_http },
   { "set-cookie:",        11, server_set_cookie },
   { "connection:",        11, crumble },
   { "Content-Type:",      13, server_content_type },
   { "Content-Length:",    15, server_content_length },
   { "Content-MD5:",       12, server_content_md5 },
   { "Content-Encoding:",  17, server_content_encoding },
   { "Transfer-Encoding:", 18, server_transfer_coding },
   { "Keep-Alive:",        11, crumble },
   { NULL, 0, NULL }
};


void (* const add_client_headers[])(struct client_state *) = {
   client_host_adder,
   client_cookie_adder,
   client_x_forwarded_adder,
   client_xtra_adder,
   client_accept_encoding_adder,
   connection_close_adder,
   NULL
};


void (* const add_server_headers[])(struct client_state *) = {
   connection_close_adder,
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
 * Function    :  get_header_value
 *
 * Description :  Get the value of a given header from a chained list
 *                of header lines or return NULL if no such header is
 *                present in the list.
 *
 * Parameters  :
 *          1  :  header_list = pointer to list
 *          2  :  header_name = string with name of header to look for.
 *                              Trailing colon required, capitalization
 *                              doesn't matter.
 *
 * Returns     :  NULL if not found, else value of header
 *
 *********************************************************************/
char *get_header_value(const struct list *header_list, const char *header_name)
{
   struct list_entry *cur_entry;
   char *ret = NULL;
   size_t length = 0;

   assert(header_list);
   assert(header_name);
   length = strlen(header_name);

   for (cur_entry = header_list->first; cur_entry ; cur_entry = cur_entry->next)
   {
      if (cur_entry->str)
      {
         if (!strncmpic(cur_entry->str, header_name, length))
         {
            /*
             * Found: return pointer to start of value
             */
            ret = (char *) (cur_entry->str + length);
            while (*ret && ijb_isspace(*ret)) ret++;
            return(ret);
         }
      }
   }

   /* 
    * Not found
    */
   return NULL;

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
 * Returns     :  Single pointer to a fully formed header, or NULL
 *                on out-of-memory error.
 *
 *********************************************************************/
char *sed(const struct parsers pats[], void (* const more_headers[])(struct client_state *), struct client_state *csp)
{
   struct list_entry *p;
   const struct parsers *v;
   char *hdr;
   void (* const *f)();

   for (v = pats; v->str ; v++)
   {
      for (p = csp->headers->first; p ; p = p->next)
      {
         /* Header crunch()ed in previous run? -> ignore */
         if (p->str == NULL) continue;

         if (v == pats) log_error(LOG_LEVEL_HEADER, "scan: %s", p->str);

         if (strncmpic(p->str, v->str, v->len) == 0)
         {
            hdr = v->parser(v, p->str, csp);
            freez(p->str); /* FIXME: Yuck! patching a list...*/
            p->str = hdr;
         }
      }
   }

   /* place any additional headers on the csp->headers list */
   for (f = more_headers; *f ; f++)
   {
      (*f)(csp);
   }

   hdr = list_to_text(csp->headers);

   return hdr;

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
char *crumble(const struct parsers *v, const char *s, struct client_state *csp)
{
   log_error(LOG_LEVEL_HEADER, "crunch!");
   return(NULL);

}


/*********************************************************************
 *
 * Function    :  server_content_type
 *
 * Description :  Set the content-type for filterable types (text/.*,
 *                javascript and image/gif) unless filtering has been
 *                forbidden (CT_TABOO) while parsing earlier headers.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header string we are "considering"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A duplicate string pointer to this header (ie. pass thru)
 *
 *********************************************************************/
char *server_content_type(const struct parsers *v, const char *s, struct client_state *csp)
{
   if (csp->content_type != CT_TABOO)
   {
      if (strstr(s, " text/") || strstr(s, "application/x-javascript"))
         csp->content_type = CT_TEXT;
      else if (strstr(s, " image/gif"))
         csp->content_type = CT_GIF;
      else
         csp->content_type = 0;
   }

   return(strdup(s));

}


/*********************************************************************
 *
 * Function    :  server_transfer_coding
 *
 * Description :  - Prohibit filtering (CT_TABOO) if transfer coding compresses
 *                - Raise the CSP_FLAG_CHUNKED flag if coding is "chunked"
 *                - Change from "chunked" to "identity" if body was chunked
 *                  but has been de-chunked for filtering.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header string we are "considering"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A duplicate string pointer to this header (ie. pass thru)
 *
 *********************************************************************/
char *server_transfer_coding(const struct parsers *v, const char *s, struct client_state *csp)
{
   /*
    * Turn off pcrs and gif filtering if body compressed
    */
   if (strstr(s, "gzip") || strstr(s, "compress") || strstr(s, "deflate"))
   {
      csp->content_type = CT_TABOO;
   }

   /*
    * Raise flag if body chunked
    */
   if (strstr(s, "chunked"))
   {
      csp->flags |= CSP_FLAG_CHUNKED;

      /*
       * If the body was modified, it has been
       * de-chunked first, so adjust the header:
       */
      if (csp->flags & CSP_FLAG_MODIFIED)
      {
         return(strdup("Transfer-Encoding: identity"));
      }
   }

   return(strdup(s));

}


/*********************************************************************
 *
 * Function    :  server_content_encoding
 *
 * Description :  Prohibit filtering (CT_TABOO) if content encoding compresses
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header string we are "considering"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A duplicate string pointer to this header (ie. pass thru)
 *
 *********************************************************************/
char *server_content_encoding(const struct parsers *v, const char *s, struct client_state *csp)
{
   /*
    * Turn off pcrs and gif filtering if body compressed
    */
   if (strstr(s, "gzip") || strstr(s, "compress") || strstr(s, "deflate"))
   {
      csp->content_type = CT_TABOO;
   }

   return(strdup(s));

}


/*********************************************************************
 *
 * Function    :  server_content_length
 *
 * Description :  Adjust Content-Length header if we modified
 *                the body.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header string we are "considering"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A duplicate string pointer to this header (ie. pass thru)
 *
 *********************************************************************/
char *server_content_length(const struct parsers *v, const char *s, struct client_state *csp)
{
   if (csp->content_length != 0) /* Content length has been modified */
   {
      char * s2 = (char *) zalloc(100);
      sprintf(s2, "Content-Length: %d", (int) csp->content_length);

      log_error(LOG_LEVEL_HEADER, "Adjust Content-Length to %d", (int) csp->content_length);
      return(s2);
   }
   else
   {
      return(strdup(s));
   }

}


/*********************************************************************
 *
 * Function    :  server_content_md5
 *
 * Description :  Crumble any Content-MD5 headers if the document was
 *                modified. FIXME: Should we re-compute instead?
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header string we are "considering"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A duplicate string pointer to this header (ie. pass thru)
 *
 *********************************************************************/
char *server_content_md5(const struct parsers *v, const char *s, struct client_state *csp)
{
   if (csp->flags & CSP_FLAG_MODIFIED)
   {
      log_error(LOG_LEVEL_HEADER, "Crunching Content-MD5");
      return(NULL);
   }
   else
   {
      return(strdup(s));
   }

}


/*********************************************************************
 *
 * Function    :  client_accept_encoding
 *
 * Description :  Rewrite the client's Accept-Encoding header so that
 *                if doesn't allow compression, if the action applies.
 *                Note: For HTTP/1.0 the absence of the header is enough.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header string we are "considering"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A copy of the client's original or the modified header.
 *
 *********************************************************************/
char *client_accept_encoding(const struct parsers *v, const char *s, struct client_state *csp)
{
   if ((csp->action->flags & ACTION_NO_COMPRESSION) == 0)
   {
      return(strdup(s));
   }
   else
   {
      log_error(LOG_LEVEL_HEADER, "Supressed offer to compress content");

      if (!strcmpic(csp->http->ver, "HTTP/1.1"))
      {
         return(strdup("Accept-Encoding: identity;q=1.0, *;q=0"));
      }
      else
      {
         return(NULL);
      }
   }

}


/*********************************************************************
 *
 * Function    :  client_te
 *
 * Description :  Rewrite the client's TE header so that
 *                if doesn't allow compression, if the action applies.
 *
 * Parameters  :
 *          1  :  v = ignored
 *          2  :  s = header string we are "considering"
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A copy of the client's original or the modified header.
 *
 *********************************************************************/
char *client_te(const struct parsers *v, const char *s, struct client_state *csp)
{
   if ((csp->action->flags & ACTION_NO_COMPRESSION) == 0)
   {
      return(strdup(s));
   }
   else
   {
      log_error(LOG_LEVEL_HEADER, "Supressed offer to compress transfer");
      return(NULL);
   }

}

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
char *client_referrer(const struct parsers *v, const char *s, struct client_state *csp)
{
   const char * newval;
   char * s2;
#ifdef FEATURE_FORCE_LOAD
   /* Since the referrer can include the prefix even
    * even if the request itself is non-forced, we must
    * clean it unconditionally
    */
   strclean(s, FORCE_PREFIX);
#endif /* def FEATURE_FORCE_LOAD */

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
      s2 = strsav(NULL, "Referer: ");
      s2 = strsav(s2, "http://");
      s2 = strsav(s2, csp->http->hostport);
      s2 = strsav(s2, "/");
      return(s2);
   }

   /*
    * Have we got a fixed referer?
    */
   if (0 == strncmpic(newval, "http://", 7))
   {
      /*
       * We have a specific (fixed) referer we want to send.
       */
      char * s3;

      log_error(LOG_LEVEL_HEADER, "modified");

      s3 = strsav( NULL, "Referer: " );
      s3 = strsav( s3, newval );
      return(s3);
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
   s2 = strsav(NULL, "Referer: ");
   s2 = strsav(s2, "http://");
   s2 = strsav(s2, csp->http->hostport);
   s2 = strsav(s2, "/");
   return(s2);
}


/*********************************************************************
 *
 * Function    :  client_uagent
 *
 * Description :  Handle the "user-agent" config setting properly
 *                and remember its original value to enable browser
 *                bug workarounds. Called from `sed'.
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
char *client_uagent(const struct parsers *v, const char *s, struct client_state *csp)
{
   const char * newval;
   char * s2;

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

   s2 = strsav( NULL, "User-Agent: " );
   s2 = strsav( s2, newval );
   return(s2);

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
char *client_ua(const struct parsers *v, const char *s, struct client_state *csp)
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
char *client_from(const struct parsers *v, const char *s, struct client_state *csp)
{
   const char * newval;
   char * s2;

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

   s2 = strsav( NULL, "From: " );
   s2 = strsav( s2, newval );
   return(s2);

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
char *client_send_cookie(const struct parsers *v, const char *s, struct client_state *csp)
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
char *client_x_forwarded(const struct parsers *v, const char *s, struct client_state *csp)
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

/* the following functions add headers directly to the header list */

/*********************************************************************
 *
 * Function    :  client_host_adder
 *
 * Description :  (re)adds the host header. Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void client_host_adder(struct client_state *csp)
{
   char *p = NULL,
        *pos = NULL;

   if ( !csp->http->hostport || !*(csp->http->hostport)) return;
   p = strsav(p, "Host: ");
   /*
   ** remove 'user:pass@' from 'proto://user:pass@host'
   */
   if ( (pos = strchr( csp->http->hostport, '@')) != NULL )
   {
       p = strsav(p, pos+1);
   }
   else
   {
      p = strsav(p, csp->http->hostport);
   }
   log_error(LOG_LEVEL_HEADER, "addh: %s", p);
   enlist(csp->headers, p);

   freez(p);
}


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
   struct list_entry *lst;
   char *tmp = NULL;
   char *e;

   for (lst = csp->cookie_list->first; lst ; lst = lst->next)
   {
      if (tmp)
      {
         tmp = strsav(tmp, "; ");
      }
      tmp = strsav(tmp, lst->str);
   }

   for (lst = csp->action->multi[ACTION_MULTI_WAFER]->first;  lst ; lst = lst->next)
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
 * Function    :  client_accept_encoding_adder
 *
 * Description :  Add an Accept-Encoding header to the client's request
 *                that disables compression if the action applies, and
 *                the header is not already there. Called from `sed'.
 *                Note: For HTTP/1.0, the absence of the header is enough.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void client_accept_encoding_adder(struct client_state *csp)
{
   if (   ((csp->action->flags & ACTION_NO_COMPRESSION) != 0)
       && (!strcmpic(csp->http->ver, "HTTP/1.1")) )
   {
      enlist_unique(csp->headers, "Accept-Encoding: identity;q=1.0, *;q=0", 16);
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
   struct list_entry *lst;

   for (lst = csp->action->multi[ACTION_MULTI_ADD_HEADER]->first;
        lst ; lst = lst->next)
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

   freez(p);
}


/*********************************************************************
 *
 * Function    :  connection_close_adder
 *
 * Description :  Adds a "Connection: close" header to csp->headers
 *                as a temporary fix for the needed but missing HTTP/1.1
 *                support. Called from `sed'.
 *                FIXME: This whole function shouldn't be neccessary!
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void connection_close_adder(struct client_state *csp)
{
   enlist(csp->headers, "Connection: close");
}


/*********************************************************************
 *
 * Function    :  server_http
 *
 * Description :  - Save the HTTP Status into csp->http->status
 *                - Set CT_TABOO to prevent filtering if the answer
 *                  is a partial range (HTTP status 206)
 *                - Rewrite HTTP/1.1 answers to HTTP/1.0 if +downgrade
 *                  action applies.
 *
 * Parameters  :
 *          1  :  v = parser pattern that matched this header
 *          2  :  s = header that matched this pattern
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Copy of changed  or original answer.
 *
 *********************************************************************/
char *server_http(const struct parsers *v, const char *s, struct client_state *csp)
{
   char *ret = strdup(s);

   sscanf(ret, "HTTP/%*d.%*d %d", &(csp->http->status));
   if (csp->http->status == 206)
   {
      csp->content_type = CT_TABOO;
   }

   if ((csp->action->flags & ACTION_DOWNGRADE) != 0)
   {
      ret[7] = '0';
      log_error(LOG_LEVEL_HEADER, "Downgraded answer to HTTP/1.0");
   }
   return(ret);

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
char *server_set_cookie(const struct parsers *v, const char *s, struct client_state *csp)
{
#ifdef FEATURE_COOKIE_JAR
   if (csp->config->jar)
   {
      /*
       * Write timestamp into outbuf.
       *
       * Complex because not all OSs have tm_gmtoff or
       * the %z field in strftime()
       */
      char tempbuf[ BUFFER_SIZE ];
      time_t now; 
      struct tm tm_now; 
      time (&now); 
#ifdef HAVE_LOCALTIME_R
       tm_now = *localtime_r(&now, &tm_now);
#else
       tm_now = *localtime (&now); 
#endif
      strftime(tempbuf, BUFFER_SIZE-6, "%b %d %H:%M:%S ", &tm_now); 

      fprintf(csp->config->jar, "%s %s\t%s\n", tempbuf, csp->http->host, (s + v->len + 1));
   }
#endif /* def FEATURE_COOKIE_JAR */

   if ((csp->action->flags & ACTION_NO_COOKIE_SET) != 0)
   {
      return(crumble(v, s, csp));
   }
   else if ((csp->action->flags & ACTION_NO_COOKIE_KEEP) != 0)
   {
      /* Flag whether or not to log a message */
      int changed = 0;

      /* A variable to store the tag we're working on */
      char * cur_tag;

      /* Make a copy of the header we can write to */
      char * result = strdup(s);
      if (result == NULL)
      {
         /* FIXME: This error handling is incorrect */
         return NULL;
      }

      /* Skip "Set-Cookie:" (11 characters) in header */
      cur_tag = result + 11;

      /* skip whitespace between "Set-Cookie:" and value */
      while (*cur_tag && ijb_isspace(*cur_tag))
      {
         cur_tag++;
      }

      /* Loop through each tag in the cookie */
      while (*cur_tag)
      {
         /* Find next tag */
         char * next_tag = strchr(cur_tag, ';');
         if (next_tag != NULL)
         {
            /* Skip the ';' character itself */
            next_tag++;

            /* skip whitespace ";" and start of tag */
            while (*next_tag && ijb_isspace(*next_tag))
            {
               next_tag++;
            }
         }
         else
         {
            /* "Next tag" is the end of the string */
            next_tag = cur_tag + strlen(cur_tag);
         }

         /* Is this the "Expires" tag? */
         if (strncmpic(cur_tag, "expires=", 8) == 0)
         {
            /* Delete the tag by copying the rest of the string over it.
             * (Note that we cannot just use "strcpy(cur_tag, next_tag)",
             * since the behaviour of strcpy is undefined for overlapping
             * strings.)
             */
            memmove(cur_tag, next_tag, strlen(next_tag) + 1);

            /* That changed the header, need to issue a log message */
            changed = 1;

            /* Note that the next tag has now been moved to *cur_tag,
             * so we do not need to update the cur_tag pointer.
             */
         }
         else
         {
            /* Move on to next cookie tag */
            cur_tag = next_tag;
         }
      }

      if (changed)
      {
         log_error(LOG_LEVEL_HEADER, "Changed cookie to a temporary one.");
      }

      return result;
   }

   return(strdup(s));
}


#ifdef FEATURE_FORCE_LOAD
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
#endif /* def FEATURE_FORCE_LOAD */


/*
  Local Variables:
  tab-width: 3
  end:
*/
