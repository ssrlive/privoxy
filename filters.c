const char filters_rcs[] = "$Id: filters.c,v 1.11 2001/05/29 11:53:23 oes Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/filters.c,v $
 *
 * Purpose     :  Declares functions to parse/crunch headers and pages.
 *                Functions declared include:
 *                   `acl_addr', `add_stats', `block_acl', `block_imageurl',
 *                   `block_url', `url_permissions', `domaincmp', `dsplit',
 *                   `filter_popups', `forward_url', 'redirect_url',
 *                   `ij_untrusted_url', `intercept_url', `re_process_buffer',
 *                   `show_proxy_args', 'ijb_send_banner', and `trust_url'
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
 *    $Log: filters.c,v $
 *    Revision 1.11  2001/05/29 11:53:23  oes
 *    "See why" link added to "blocked" page
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
 *    Revision 1.9  2001/05/27 22:17:04  oes
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
 *    Revision 1.8  2001/05/26 17:13:28  jongfoster
 *    Filled in a function comment.
 *
 *    Revision 1.7  2001/05/26 15:26:15  jongfoster
 *    ACL feature now provides more security by immediately dropping
 *    connections from untrusted hosts.
 *
 *    Revision 1.6  2001/05/26 00:28:36  jongfoster
 *    Automatic reloading of config file.
 *    Removed obsolete SIGHUP support (Unix) and Reload menu option (Win32).
 *    Most of the global variables have been moved to a new
 *    struct configuration_spec, accessed through csp->config->globalname
 *    Most of the globals remaining are used by the Win32 GUI.
 *
 *    Revision 1.5  2001/05/25 22:34:30  jongfoster
 *    Hard tabs->Spaces
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
 *    Revision 1.3  2001/05/20 16:44:47  jongfoster
 *    Removing last hardcoded JunkBusters.com URLs.
 *
 *    Revision 1.2  2001/05/20 01:21:20  jongfoster
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
 *    Revision 1.1.1.1  2001/05/15 13:58:52  oes
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
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

#include "project.h"
#include "filters.h"
#include "encode.h"
#include "jcc.h"
#include "showargs.h"
#include "parsers.h"
#include "ssplit.h"
#include "gateway.h"
#include "jbsockets.h"
#include "errlog.h"
#include "jbsockets.h"
#include "miscutil.h"

#ifdef _WIN32
#include "win32.h"
#endif

const char filters_h_rcs[] = FILTERS_H_VERSION;

/* Fix a problem with Solaris.  There should be no effect on other
 * platforms.
 * Solaris's isspace() is a macro which uses it's argument directly
 * as an array index.  Therefore we need to make sure that high-bit
 * characters generate +ve values, and ideally we also want to make
 * the argument match the declared parameter type of "int".
 */
#define ijb_isdigit(__X) isdigit((int)(unsigned char)(__X))


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


#ifdef ACL_FILES
/*********************************************************************
 *
 * Function    :  block_acl
 *
 * Description :  Block this request?
 *                Decide yes or no based on ACL file.
 *
 * Parameters  :
 *          1  :  dst = The proxy or gateway address this is going to.
 *                      Or NULL to check all possible targets.
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *                      Also includes the client IP address.
 *
 * Returns     : 0 = FALSE (don't block) and 1 = TRUE (do block)
 *
 *********************************************************************/
int block_acl(struct access_control_addr *dst,
              struct client_state *csp)
{
   struct file_list *fl;
   struct access_control_list *a, *acl;

   /* if not using an access control list, then permit the connection */
   if (((fl = csp->alist) == NULL) || 
       ((acl = (struct access_control_list *) fl->f) == NULL))
   {
      return(0);
   }

   /* search the list */
   for (a = acl->next ; a ; a = a->next)
   {
      if ((csp->ip_addr_long & a->src->mask) == a->src->addr)
      {
         if (dst == NULL)
         {
            /* Just want to check if they have any access */
            if (a->action == ACL_PERMIT)
            {
               return(0);
            }
         }
         else if ( ((dst->addr & a->dst->mask) == a->dst->addr)
           && ((dst->port == a->dst->port) || (a->dst->port == 0)))
         {
            if (a->action == ACL_PERMIT)
            {
               return(0);
            }
            else
            {
               return(1);
            }
         }
      }
   }

   return(1);

}


/*********************************************************************
 *
 * Function    :  acl_addr
 *
 * Description :  Called from `load_aclfile' to parse an ACL address.
 *
 * Parameters  :
 *          1  :  aspec = String specifying ACL address.
 *          2  :  aca = struct access_control_addr to fill in.
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int acl_addr(char *aspec, struct access_control_addr *aca)
{
   int i, masklength, port;
   char *p;

   masklength = 32;
   port       =  0;

   if ((p = strchr(aspec, '/')))
   {
      *p++ = '\0';

      if (ijb_isdigit(*p) == 0)
      {
         return(-1);
      }
      masklength = atoi(p);
   }

   if ((masklength < 0) || (masklength > 32))
   {
      return(-1);
   }

   if ((p = strchr(aspec, ':')))
   {
      *p++ = '\0';

      if (ijb_isdigit(*p) == 0)
      {
         return(-1);
      }
      port = atoi(p);
   }

   aca->port = port;

   aca->addr = ntohl(resolve_hostname_to_ip(aspec));

   if (aca->addr == -1)
   {
      log_error(LOG_LEVEL_ERROR, "can't resolve address for %s", aspec);
      return(-1);
   }

   /* build the netmask */
   aca->mask = 0;
   for (i=1; i <= masklength ; i++)
   {
      aca->mask |= (1 << (32 - i));
   }

   /* now mask off the host portion of the ip address
    * (i.e. save on the network portion of the address).
    */
   aca->addr = aca->addr & aca->mask;

   return(0);

}
#endif /* def ACL_FILES */


/*********************************************************************
 *
 * Function    :  block_url
 *
 * Description :  Called from `chat'.  Check to see if we need to block this.
 *
 * Parameters  :
 *          1  :  http = http_request request to "check" for blocked
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  NULL => unblocked, else string to HTML block description.
 *
 *********************************************************************/
char *block_url(struct http_request *http, struct client_state *csp)
{
   char *p;
   int n;
   int factor = 2;

   if ((csp->permissions & PERMIT_BLOCK) == 0)
   {
      return(NULL);
   }
   else
   {
#ifdef FORCE_LOAD
      factor++;
#endif /* def FORCE_LOAD */

      n  = strlen(CBLOCK);
      n += factor * strlen(http->hostport);
      n += factor * strlen(http->path);

      p = (char *)malloc(n);

#ifdef FORCE_LOAD
      sprintf(p, CBLOCK, http->hostport, http->path, http->hostport, http->path,
              http->hostport, http->path);
#else
      sprintf(p, CBLOCK, http->hostport, http->path, http->hostport, http->path);
#endif /* def FORCE_LOAD */

      return(p);
   }
}


#ifdef IMAGE_BLOCKING
/*********************************************************************
 *
 * Function    :  block_imageurl
 *
 * Description :  Given a URL which is blocked, decide whether to 
 *                send the "blocked" image or HTML.
 *
 * Parameters  :
 *          1  :  http = URL to check.
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  True (nonzero) if URL is in image list, false (0)
 *                otherwise
 *
 *********************************************************************/
int block_imageurl(struct http_request *http, struct client_state *csp)
{
#ifdef DETECT_MSIE_IMAGES
   if ((csp->accept_types 
       & (ACCEPT_TYPE_IS_MSIE|ACCEPT_TYPE_MSIE_IMAGE|ACCEPT_TYPE_MSIE_HTML))
       == (ACCEPT_TYPE_IS_MSIE|ACCEPT_TYPE_MSIE_IMAGE))
   {
      return 1;
   }
   else if ((csp->accept_types 
       & (ACCEPT_TYPE_IS_MSIE|ACCEPT_TYPE_MSIE_IMAGE|ACCEPT_TYPE_MSIE_HTML))
       == (ACCEPT_TYPE_IS_MSIE|ACCEPT_TYPE_MSIE_HTML))
   {
      return 0;
   }
#endif

   return ((csp->permissions & PERMIT_IMAGE) != 0);
}
#endif /* def IMAGE_BLOCKING */


#ifdef PCRS
/*********************************************************************
 *
 * Function    :  re_process_buffer
 *
 * Description :  Apply all jobs from the joblist (aka. Perl regexp's) to
 *                the text buffer that's been accumulated in csp->iob->buf
 *                and set csp->content_length to the modified size.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  a pointer to the (newly allocated) modified buffer.
 *                
 *
 *********************************************************************/
char *re_process_buffer(struct client_state *csp)
{
   int hits=0;
   int size = csp->iob->eod - csp->iob->cur;
   char *old=csp->iob->cur, *new = NULL;
   pcrs_job *job, *joblist;

   struct file_list *fl;
   struct re_filterfile_spec *b;

   /* Sanity first ;-) */
   if (size <= 0)
   {
      return(strdup(""));
   }

   if ( ( NULL == (fl = csp->rlist) ) || ( NULL == (b = fl->f) ) )
   {
      log_error(LOG_LEVEL_ERROR, "Unable to get current state of regexp filtering.");
      return(strdup(""));
   }

   joblist = b->joblist;


   log_error(LOG_LEVEL_RE_FILTER, "re_filtering %s%s (size %d) ...",
              csp->http->hostport, csp->http->path, size);

   /* Apply all jobs from the joblist */
   for (job = joblist; NULL != job; job = job->next)
   {
      hits += pcrs_exec_substitution(job, old, size, &new, &size);
      if (old != csp->iob->cur) free(old);
      old=new;
   }

   log_error(LOG_LEVEL_RE_FILTER, " produced %d hits (new size %d).", hits, size);

   csp->content_length = size;

   /* fwiw, reset the iob */
   IOB_RESET(csp);
   return(new);

}
#endif /* def PCRS */


#ifdef TRUST_FILES
/*********************************************************************
 *
 * Function    :  trust_url
 *
 * Description :  Should we "trust" this URL?  See "trustfile" line in config.
 *
 * Parameters  :
 *          1  :  http = http_request request for requested URL
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  NULL => trusted, else string to HTML "untrusted" description.
 *
 *********************************************************************/
char *trust_url(struct http_request *http, struct client_state *csp)
{
   struct file_list *fl;
   struct block_spec *b;
   struct url_spec url[1], **tl, *t;
   char *p, *h;
   char *hostport, *path, *refer;
   struct http_request rhttp[1];
   int n;

   if (((fl = csp->tlist) == NULL) || ((b  = fl->f) == NULL))
   {
      return(NULL);
   }

   *url = dsplit(http->host);

   /* if splitting the domain fails, punt */
   if (url->dbuf == NULL) return(NULL);

   memset(rhttp, '\0', sizeof(*rhttp));

   for (b = b->next; b ; b = b->next)
   {
      if ((b->url->port == 0) || (b->url->port == http->port))
      {
         if ((b->url->domain[0] == '\0') || (domaincmp(b->url, url) == 0))
         {
            if ((b->url->path == NULL) ||
#ifdef REGEX
               (regexec(b->url->preg, http->path, 0, NULL, 0) == 0)
#else
               (strncmp(b->url->path, http->path, b->url->pathlen) == 0)
#endif
            )
            {
               freez(url->dbuf);
               freez(url->dvec);

               if (b->reject == 0) return(NULL);

               hostport = url_encode(http->hostport);
               path     = url_encode(http->path);

               if (csp->referrer)
               {
                  refer = url_encode(csp->referrer);
               }
               else
               {
                  refer = url_encode("undefined");
               }

               n  = strlen(CTRUST);
               n += strlen(hostport);
               n += strlen(path);
               n += strlen(refer);

               p = (char *)malloc(n);

               sprintf(p, CTRUST, hostport, path, refer);

               freez(hostport);
               freez(path);
               freez(refer);

               return(p);
            }
         }
      }
   }

   freez(url->dbuf);
   freez(url->dvec);

   if ((csp->referrer == NULL)|| (strlen(csp->referrer) <= 9))
   {
      /* no referrer was supplied */
      goto trust_url_not_trusted;
   }

   /* forge a URL from the referrer so we can use
    * convert_url() to parse it into its components.
    */

   p = NULL;
   p = strsav(p, "GET ");
   p = strsav(p, csp->referrer + 9);   /* skip over "Referer: " */
   p = strsav(p, " HTTP/1.0");

   parse_http_request(p, rhttp, csp);

   if (rhttp->cmd == NULL)
   {
      freez(p);
      goto trust_url_not_trusted;
   }

   freez(p);

   *url = dsplit(rhttp->host);

   /* if splitting the domain fails, punt */
   if (url->dbuf == NULL) goto trust_url_not_trusted;

   for (tl = csp->config->trust_list; (t = *tl) ; tl++)
   {
      if ((t->port == 0) || (t->port == rhttp->port))
      {
         if ((t->domain[0] == '\0') || domaincmp(t, url) == 0)
         {
            if ((t->path == NULL) ||
#ifdef REGEX
               (regexec(t->preg, rhttp->path, 0, NULL, 0) == 0)
#else
               (strncmp(t->path, rhttp->path, t->pathlen) == 0)
#endif
            )
            {
               /* if the URL's referrer is from a trusted referrer, then
                * add the target spec to the trustfile as an unblocked
                * domain and return NULL (which means it's OK).
                */

               FILE *fp;

               freez(url->dbuf);
               freez(url->dvec);

               if ((fp = fopen(csp->config->trustfile, "a")))
               {
                  h = NULL;

                  h = strsav(h, "~");
                  h = strsav(h, http->hostport);

                  p = http->path;
                  if ((*p++ == '/')
                  && (*p++ == '~'))
                  {
                  /* since this path points into a user's home space
                   * be sure to include this spec in the trustfile.
                   */
                     if ((p = strchr(p, '/')))
                     {
                        *p = '\0';
                        h = strsav(h, http->path);
                        h = strsav(h, "/");
                     }
                  }

                  free_http_request(rhttp);

                  fprintf(fp, "%s\n", h);
                  freez(h);
                  fclose(fp);
               }
               return(NULL);
            }
         }
      }
   }

trust_url_not_trusted:
   free_http_request(rhttp);

   hostport = url_encode(http->hostport);
   path     = url_encode(http->path);

   if (csp->referrer)
   {
      refer = url_encode(csp->referrer);
   }
   else
   {
      refer = url_encode("undefined");
   }

   n  = strlen(CTRUST);
   n += strlen(hostport);
   n += strlen(path);
   n += strlen(refer);

   p = (char *)malloc(n);
   sprintf(p, CTRUST, hostport, path, refer);

   freez(hostport);
   freez(path);
   freez(refer);

   return(p);

}
#endif /* def TRUST_FILES */


static const char C_HOME_PAGE[] =
   "HTTP/1.0 200 OK\n"
   "Pragma: no-cache\n"
   "Expires: Thu Jul 31, 1997 07:42:22 pm GMT\n"
   "Content-Type: text/html\n\n"
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


/*********************************************************************
 *
 * Function    :  intercept_url
 *
 * Description :  checks the URL `basename' against a list of URLs to
 *                snarf. If it matches, it calls the associated function
 *                which returns an HTML page to send back to the client.
 *                Right now, we snarf:
 *                      "show-proxy-args", and
 *                      "ij-untrusted-url" (optional w/TRUST_FILES)
 *
 * Parameters  :
 *          1  :  http = http_request request, check `basename's of blocklist
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  1 if it intercepts & handles the request.
 *
 *********************************************************************/
int intercept_url(struct http_request *http, struct client_state *csp)
{
   char *basename = NULL;
   const struct interceptors *v;

   if (0 == strcmpic(http->host,"i.j.b"))
   {
      /*
       * Catch http://i.j.b/...
       */
      basename = http->path;
   }
   else if ( ( (0 == strcmpic(http->host,"ijbswa.sourceforge.net"))
            || (0 == strcmpic(http->host,"ijbswa.sf.net")) )
            && (0 == strncmpic(http->path,"/config", 7))
            && ((http->path[7] == '/') || (http->path[7] == '\0')))
   {
      /*
       * Catch http://ijbswa.sourceforge.net/config/...
       * and   http://ijbswa.sf.net/config/...
       */
      basename = http->path + 7;
   }

   if (!basename)
   {
      /* Don't want to intercept */
      return(0);
   }

   /* We have intercepted it. */

   /* remove any leading slash */
   if (*basename == '/')
   {
      basename++;
   }

   log_error(LOG_LEVEL_GPC, "%s%s intercepted!", http->hostport, http->path);
   log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 200 3", 
                            csp->ip_addr_str, http->cmd); 

   for (v = intercept_patterns; v->str; v++)
   {
      if (strncmp(basename, v->str, v->len) == 0)
      {
         char * p = ((v->interceptor)(http, csp));

         if (p != NULL)
         {
            /* Send HTML redirection result */
            write_socket(csp->cfd, p, strlen(p));

            freez(p);
         }
         return(1);
      }
   }

   write_socket(csp->cfd, C_HOME_PAGE, strlen(C_HOME_PAGE));

   return(1);
}

#ifdef FAST_REDIRECTS
/*********************************************************************
 *
 * Function    :  redirect_url
 *
 * Description :  Checks for redirection URLs and returns a HTTP redirect
 *                to the destination URL.
 *
 * Parameters  :
 *          1  :  http = http_request request, check `basename's of blocklist
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  NULL if URL was clean, HTTP redirect otherwise.
 *
 *********************************************************************/
char *redirect_url(struct http_request *http, struct client_state *csp)
{
   char *p, *q;

   p = q = csp->http->path;
   log_error(LOG_LEVEL_REDIRECTS, "checking path: %s", p);

   /* find the last URL encoded in the request */
   while (p = strstr(p, "http://"))
   {
      q = p++;
   }

   /* if there was any, generate and return a HTTP redirect */
   if (q != csp->http->path)
   {
      log_error(LOG_LEVEL_REDIRECTS, "redirecting to: %s", q);

      p = (char *)malloc(strlen(HTTP_REDIRECT_TEMPLATE) + strlen(q));
      sprintf(p, HTTP_REDIRECT_TEMPLATE, q);
      return(p);
   }
   else
   {
      return(NULL);
   }

}
#endif /* def FAST_REDIRECTS */

/*********************************************************************
 *
 * Function    :  url_permissions
 *
 * Description :  Gets the permissions for this URL.
 *
 * Parameters  :
 *          1  :  http = http_request request for blocked URLs
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  permissions bitmask specifiying what this URL can do.
 *                If not on list, will be default_permissions.
 *
 *********************************************************************/
int url_permissions(struct http_request *http, struct client_state *csp)
{
   struct file_list *fl;
   struct permissions_spec *b;
   struct url_spec url[1];
   int permissions = csp->config->default_permissions;

   if (((fl = csp->permissions_list) == NULL) || ((b = fl->f) == NULL))
   {
      return(permissions);
   }

   *url = dsplit(http->host);

   /* if splitting the domain fails, punt */
   if (url->dbuf == NULL)
   {
      return(permissions);
   }

   for (b = b->next; NULL != b; b = b->next)
   {
      if ((b->url->port == 0) || (b->url->port == http->port))
      {
         if ((b->url->domain[0] == '\0') || (domaincmp(b->url, url) == 0))
         {
            if ((b->url->path == NULL) ||
#ifdef REGEX
               (regexec(b->url->preg, http->path, 0, NULL, 0) == 0)
#else
               (strncmp(b->url->path, http->path, b->url->pathlen) == 0)
#endif
            )
            {
               permissions &= b->mask;
               permissions |= b->add;
            }
         }
      }
   }

   freez(url->dbuf);
   freez(url->dvec);
   return(permissions);

}


/*********************************************************************
 *
 * Function    :  forward_url
 *
 * Description :  Should we forward this to another proxy?
 *
 * Parameters  :
 *          1  :  http = http_request request for current URL
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Return gw_default for no forward match,
 *                else a gateway pointer to a specific forwarding proxy.
 *
 *********************************************************************/
const struct gateway *forward_url(struct http_request *http, struct client_state *csp)
{
   struct file_list *fl;
   struct forward_spec *b;
   struct url_spec url[1];

   if (((fl = csp->flist) == NULL) || ((b = fl->f) == NULL))
   {
      return(gw_default);
   }

   *url = dsplit(http->host);

   /* if splitting the domain fails, punt */
   if (url->dbuf == NULL) return(gw_default);

   for (b = b->next; b ; b = b->next)
   {
      if ((b->url->port == 0) || (b->url->port == http->port))
      {
         if ((b->url->domain[0] == '\0') || (domaincmp(b->url, url) == 0))
         {
            if ((b->url->path == NULL) ||
#ifdef REGEX
               (regexec(b->url->preg, http->path, 0, NULL, 0) == 0)
#else
               (strncmp(b->url->path, http->path, b->url->pathlen) == 0)
#endif
            )
            {
               freez(url->dbuf);
               freez(url->dvec);
               return(b->gw);
            }
         }
      }
   }

   freez(url->dbuf);
   freez(url->dvec);
   return(gw_default);

}


/*********************************************************************
 *
 * Function    :  dsplit
 *
 * Description :  Takes a domain and returns a pointer to a url_spec
 *                structure populated with dbuf, dcnt and dvec.  The
 *                other fields in the structure that is returned are zero.
 *
 * Parameters  :
 *          1  :  domain = a URL address
 *
 * Returns     :  url_spec structure populated with dbuf, dcnt and dvec.
 *
 *********************************************************************/
struct url_spec dsplit(char *domain)
{
   struct url_spec ret[1];
   char *v[BUFSIZ];
   int size;
   char *p;

   memset(ret, '\0', sizeof(*ret));

   ret->unanchored = (domain[strlen(domain) - 1] == '.');

   ret->dbuf = strdup(domain);

   /* map to lower case */
   for (p = ret->dbuf; *p ; p++) *p = tolower(*p);

   /* split the domain name into components */
   ret->dcnt = ssplit(ret->dbuf, ".", v, SZ(v), 1, 1);

   if (ret->dcnt <= 0)
   {
      memset(ret, '\0', sizeof(ret));
      return(*ret);
   }

   /* save a copy of the pointers in dvec */
   size = ret->dcnt * sizeof(*ret->dvec);

   if ((ret->dvec = (char **)malloc(size)))
   {
      memcpy(ret->dvec, v, size);
   }


   return(*ret);

}


/*********************************************************************
 *
 * Function    :  domaincmp
 *
 * Description :  Compare domain names.
 *                domaincmp("a.b.c",  "a.b.c")  => 0 (MATCH)
 *                domaincmp("a*.b.c", "a.b.c")  => 0 (MATCH)
 *                domaincmp("a*.b.c", "abc.b.c")  => 0 (MATCH)
 *                domaincmp("a*c.b.c","abbc.b.c")  => 0 (MATCH)
 *                domaincmp("*a.b.c", "dabc.b.c")  => 0 (MATCH)
 *                domaincmp("b.c"   , "a.b.c")  => 0 (MATCH)
 *                domaincmp("a.b"   , "a.b.c")  => 1 (DIFF)
 *                domaincmp("a.b."  , "a.b.c")  => 0 (MATCH)
 *                domaincmp(""      , "a.b.c")  => 0 (MATCH)
 *                
 * FIXME: I need a definition!
 *
 * Parameters  :
 *          1  :  pattern = a domain that may contain a '*' as a wildcard.
 *          2  :  fqdn = domain name against which the patterns are compared.
 *
 * Returns     :  0 => domains are equivalent, else no match.
 *
 *********************************************************************/
int domaincmp(struct url_spec *pattern, struct url_spec *fqdn)
{
   char **pv, **fv;  /* vectors  */
   int    pn,   fn;  /* counters */
   char  *p,   *f;   /* chars    */

   pv = pattern->dvec;
   fv = fqdn->dvec;
   fn = pn = 0;

   while (fn < fqdn->dcnt && pn < pattern->dcnt)
   {
      p = pv[pn];
      f = fv[fn];

      if (trivimatch(p, f))
      {
         if(pn)
         {
            return 1;
         }
      }
      else
      {
         pn++;
      }
      fn++;
   }

   return ((pn < pattern->dcnt) || ((fn < fqdn->dcnt) && !pattern->unanchored));

}


/* intercept functions */

/*********************************************************************
 *
 * Function    :  show_proxy_args
 *
 * Description :  This "crunch"es "http:/any.thing/show-proxy-args" and
 *                returns a web page describing the current status of IJB.
 *
 * Parameters  :
 *          1  :  http = ignored
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A string that contains the current status of IJB.
 *
 *********************************************************************/
char *show_proxy_args(struct http_request *http, struct client_state *csp)
{
   char *s = NULL;

#ifdef SPLIT_PROXY_ARGS
   FILE * fp;
   char buf[BUFSIZ];
   char * p;
   const char * filename = NULL;
   const char * file_description = NULL;
   char * query_string = strrchr(http->path, '?');
   char which_file = '\0';


   if (query_string != NULL)
   {
      /* first char past the last '?' (maybe '\0')*/
      which_file = query_string[1];
   }
   switch (which_file)
   {
   case 'p':
      if (csp->permissions_list)
      {
         filename = csp->permissions_list->filename;
         file_description = "Permissions List";
      }
      break;
   case 'f':
      if (csp->flist)
      {
         filename = csp->flist->filename;
         file_description = "Forward List";
      }
      break;

#ifdef ACL_FILES
   case 'a':
      if (csp->alist)
      {
         filename = csp->alist->filename;
         file_description = "Access Control List";
      }
      break;
#endif /* def ACL_FILES */

#ifdef PCRS
   case 'r':
      if (csp->rlist)
      {
         filename = csp->rlist->filename;
         file_description = "RE Filter List";
      }
      break;
#endif /* def PCRS */

#ifdef TRUST_FILES
   case 't':
      if (csp->tlist)
      {
         filename = csp->tlist->filename;
         file_description = "Trust List";
      }
      break;
#endif /* def TRUST_FILES */
   }

   if (filename)
   {
      /* Display specified file */
      /* FIXME: Add HTTP headers so this isn't cached */
      s = strsav(s,
         "HTTP/1.0 200 OK\n"
         "Server: IJ/" VERSION "\n"
         "Content-type: text/html\n"
         "Pragma: no-cache\n"
         "Last-Modified: Thu Jul 31, 1997 07:42:22 pm GMT\n"
         "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT\n"
         "\n"

         "<html>"
         "<head>"
         "<title>Internet Junkbuster Proxy Status - ");
      s = strsav(s, file_description);
      s = strsav(s, 
         "</title>"
         "</head>\n"
         "<body bgcolor=\"#f8f8f0\" link=\"#000078\" alink=\"#ff0022\" vlink=\"#787878\">\n"
         "<center>\n"
         "<h1>" BANNER "\n");
      s = strsav(s, file_description);
      s = strsav(s, 
         "</h1></center>\n"
         "<p><a href=\"show-proxy-args\">Back to proxy status</a></p>\n"
         "<h2>");
      s = strsav(s, file_description);
      s = strsav(s,
         "</h2>\n"
         "Contents of file &quot;<code>");
      p = html_encode(filename);
      s = strsav(s, p);
      freez(p);
      s = strsav(s,
         "</code>&quot;:<br>\n"
         "</p>\n"
         "<pre>");
      
      if ((fp = fopen(filename, "r")) == NULL)
      {
         s = strsav(s, "</pre><h1>ERROR OPENING FILE!</h1><pre>");
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
      }

      s = strsav(s,
         "</pre>\n"
         "<br>\n"
         "<p><a href=\"show-proxy-args\">Back to proxy status</a></p>\n"
         "<br>\n"
         "<small><small><p>\n"
         "The " BANNER " Proxy - \n"
         "<a href=\"" HOME_PAGE_URL "\">" HOME_PAGE_URL "</a>\n"
         "</small></small>"
         "</body></html>\n");
      return(s);
   }
#endif /* def SPLIT_PROXY_ARGS */
   
   s = strsav(s, csp->config->proxy_args_header);
   s = strsav(s, csp->config->proxy_args_invocation);
#ifdef STATISTICS
   s = add_stats(s);
#endif /* def STATISTICS */
   s = strsav(s, csp->config->proxy_args_gateways);

#ifdef SPLIT_PROXY_ARGS
   s = strsav(s, 
      "<h2>The following files are in use:</h2>\n"
      "<p>(Click a filename to view it)</p>\n"
      "<ul>\n");

   if (csp->permissions_list)
   {
      s = strsav(s, "<li>Permissions List: <a href=\"show-proxy-args?permit\"><code>");
      s = strsav(s, csp->permissions_list->filename);
      s = strsav(s, "</code></a></li>\n");
   }

   if (csp->flist)
   {
      s = strsav(s, "<li>Forward List: <a href=\"show-proxy-args?forward\"><code>");
      s = strsav(s, csp->flist->filename);
      s = strsav(s, "</code></a></li>\n");
   }

#ifdef ACL_FILES
   if (csp->alist)
   {
      s = strsav(s, "<li>Access Control List: <a href=\"show-proxy-args?acl\"><code>");
      s = strsav(s, csp->alist->filename);
      s = strsav(s, "</code></a></li>\n");
   }
#endif /* def ACL_FILES */

#ifdef PCRS
   if (csp->rlist)
   {
      s = strsav(s, "<li>RE Filter List: <a href=\"show-proxy-args?re\"><code>");
      s = strsav(s, csp->rlist->filename);
      s = strsav(s, "</code></a></li>\n");
   }
#endif /* def PCRS */

#ifdef TRUST_FILES
   if (csp->tlist)
   {
      s = strsav(s, "<li>Trust List: <a href=\"show-proxy-args?trust\"><code>");
      s = strsav(s, csp->tlist->filename);
      s = strsav(s, "</code></a></li>\n");
   }
#endif /* def TRUST_FILES */

   s = strsav(s, "</ul>");

#else /* ifndef SPLIT_PROXY_ARGS */
   if (csp->clist)
   {
      s = strsav(s, csp->clist->proxy_args);
   }

   if (csp->flist)
   {
      s = strsav(s, csp->flist->proxy_args);
   }

#ifdef ACL_FILES
   if (csp->alist)
   {
      s = strsav(s, csp->alist->proxy_args);
   }
#endif /* def ACL_FILES */

#ifdef PCRS
   if (csp->rlist)
   {
      s = strsav(s, csp->rlist->proxy_args);
   }
#endif /* def PCRS */

#ifdef TRUST_FILES
   if (csp->tlist)
   {
      s = strsav(s, csp->tlist->proxy_args);
   }
#endif /* def TRUST_FILES */

#endif /* ndef SPLIT_PROXY_ARGS */

   s = strsav(s, csp->config->proxy_args_trailer);

   return(s);

}


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


/*********************************************************************
 *
 * Function    :  permissions_to_text
 *
 * Description :  Converts a permissionsfil entry from numeric form
 *                ("mask" and "add") to text.
 *
 * Parameters  :
 *          1  :  mask = As from struct permissions_spec
 *          2  :  add  = As from struct permissions_spec
 *
 * Returns     :  A string.  Caller must free it.
 *
 *********************************************************************/
char * permissions_to_text(unsigned mask, unsigned add)
{
   char * result = strdup("");

   /* sanity - prevents "-feature +feature" */
   mask |= add;

#define PERMISSION_TO_TEXT(__bit, __name)   \
   if (!(mask & __bit))                     \
   {                                        \
      result = strsav(result, " -" __name); \
   }                                        \
   else if (add & __bit)                    \
   {                                        \
      result = strsav(result, " +" __name); \
   }

   PERMISSION_TO_TEXT(PERMIT_COOKIE_SET, "cookies-set");
   PERMISSION_TO_TEXT(PERMIT_COOKIE_READ, "cookies-read");
   PERMISSION_TO_TEXT(PERMIT_RE_FILTER, "filter");
   PERMISSION_TO_TEXT(PERMIT_POPUPS, "popup");
   PERMISSION_TO_TEXT(PERMIT_REFERER, "referer");
   PERMISSION_TO_TEXT(PERMIT_FAST_REDIRECTS, "fast-redirects");
   PERMISSION_TO_TEXT(PERMIT_BLOCK, "block");
   PERMISSION_TO_TEXT(PERMIT_IMAGE, "image");

   return result;
}


 /*********************************************************************
 *
 * Function    :  ijb_show_url_info
 *
 * Description :  (please fill me in)
 *
 * Parameters  :
 *          1  :  http = http_request request for crunched URL
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  ???FIXME
 *
 *********************************************************************/
char *ijb_show_url_info(struct http_request *http, struct client_state *csp)
{
   char * query_string = strchr(http->path, '?');
   char * host = NULL;
   
   if (query_string != NULL)
   {
      query_string = url_decode(query_string + 1);
      if (strncmpic(query_string, "url=", 4) == 0)
      {
         host = strdup(query_string + 4);
      }
      freez(query_string);
   }
   if (host != NULL)
   {
      char * result;
      char * path;
      char * s;
      int port = 80;
      struct file_list *fl;
      struct permissions_spec *b;
      struct url_spec url[1];
      int permissions = csp->config->default_permissions;

      result = (char *)malloc(sizeof(C_URL_INFO_HEADER) + 2 * strlen(host));
      sprintf(result, C_URL_INFO_HEADER, host, host);

      s = permissions_to_text(permissions, permissions);
      result = strsav(result, "<h3>Defaults:</h3>\n<p><b>{");
      result = strsav(result, s);
      result = strsav(result, " }</b></p>\n<h3>Patterns affecting the URL:</h3>\n<p>\n");
      freez(s);

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
      }

      if (((fl = csp->permissions_list) == NULL) || ((b = fl->f) == NULL))
      {
         freez(host);
         freez(path);
         result = strsav(result, C_URL_INFO_FOOTER);
         return result;
      }

      *url = dsplit(host);

      /* if splitting the domain fails, punt */
      if (url->dbuf == NULL)
      {
         freez(host);
         freez(path);
         result = strsav(result, C_URL_INFO_FOOTER);
         return result;
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
                  s = permissions_to_text(b->mask, b->add);
                  result = strsav(result, "<b>{");
                  result = strsav(result, s);
                  result = strsav(result, " }</b><br>\n<code>");
                  result = strsav(result, b->url->spec);
                  result = strsav(result, "</code><br>\n<br>\n");
                  freez(s);
                  permissions &= b->mask;
                  permissions |= b->add;
               }
            }
         }
      }

      freez(url->dbuf);
      freez(url->dvec);

      freez(host);
      freez(path);

      s = permissions_to_text(permissions, permissions);
      result = strsav(result, "</p>\n<h2>Final Results:</h2>\n<p><b>{");
      result = strsav(result, s);
      result = strsav(result, " }</b><br>\n<br>\n");
      freez(s);

      result = strsav(result, C_URL_INFO_FOOTER);
      return result;
   }
   else
   {
      return strdup(C_URL_INFO_FORM);
   }
}


/*********************************************************************
 *
 * Function    :  ijb_send_banner
 *
 * Description :  This "crunch"es "http://i.j.b/ijb-send-banner and
 *                sends the image.
 *
 * Parameters  :
 *          1  :  http = http_request request for crunched URL
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  NULL, indicating that it has already sent the data.
 *
 *********************************************************************/
char *ijb_send_banner(struct http_request *http, struct client_state *csp)
{
   write_socket(csp->cfd, JBGIF, sizeof(JBGIF)-1);
   
   return(NULL);
}

#ifdef TRUST_FILES
/*********************************************************************
 *
 * Function    :  ij_untrusted_url
 *
 * Description :  This "crunch"es "http:/any.thing/ij-untrusted-url" and
 *                returns a web page describing why it was untrusted.
 *
 * Parameters  :
 *          1  :  http = http_request request for crunched URL
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A string that contains why this was untrusted.
 *
 *********************************************************************/
char *ij_untrusted_url(struct http_request *http, struct client_state *csp)
{
   int n;
   char *hostport, *path, *refer, *p, *v[9];
   char buf[BUFSIZ];
   struct url_spec **tl, *t;


   static const char format[] =
      "HTTP/1.0 200 OK\r\n"
      "Pragma: no-cache\n"
      "Last-Modified: Thu Jul 31, 1997 07:42:22 pm GMT\n"
      "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT\n"
      "Content-Type: text/html\n\n"
      "<html>\n"
      "<head>\n"
      "<title>Internet Junkbuster: Request for untrusted URL</title>\n"
      "</head>\n"
      BODY
      "<center><h1>"
      BANNER
      "</h1></center>"
      "The " BANNER " Proxy "
      "<A href=\"" HOME_PAGE_URL "\">"
      "(" HOME_PAGE_URL ") </A>"
      "intercepted the request for %s%s\n"
      "because the URL is not trusted.\n"
      "<br><br>\n";

   if ((n = ssplit(http->path, "?+", v, SZ(v), 0, 0)) == 4)
   {
      hostport = url_decode(v[1]);
      path     = url_decode(v[2]);
      refer    = url_decode(v[3]);
   }
   else
   {
      hostport = strdup("undefined_host");
      path     = strdup("/undefined_path");
      refer    = strdup("undefined");
   }

   n  = sizeof(format);
   n += strlen(hostport);
   n += strlen(path    );

   if ((p = (char *)malloc(n)))
   {
      sprintf(p, format, hostport, path);
   }

   strsav(p, "The referrer in this request was <strong>");
   strsav(p, refer);
   strsav(p, "</strong><br>\n");

   freez(hostport);
   freez(path    );
   freez(refer   );

   p = strsav(p, "<h3>The following referrers are trusted</h3>\n");

   for (tl = csp->config->trust_list; (t = *tl) ; tl++)
   {
      sprintf(buf, "%s<br>\n", t->spec);
      p = strsav(p, buf);
   }

   if (csp->config->trust_info->next)
   {
      struct list *l;

      strcpy(buf,
         "<p>"
         "You can learn more about what this means "
         "and what you may be able to do about it by "
         "reading the following documents:<br>\n"
         "<ol>\n"
      );

      p = strsav(p, buf);

      for (l = csp->config->trust_info->next; l ; l = l->next)
      {
         sprintf(buf,
            "<li> <a href=%s>%s</a><br>\n",
               l->str, l->str);
         p = strsav(p, buf);
      }

      p = strsav(p, "</ol>\n");
   }

   p = strsav(p, "</body>\n" "</html>\n");

   return(p);

}
#endif /* def TRUST_FILES */


#ifdef STATISTICS
/*********************************************************************
 *
 * Function    :  add_stats
 *
 * Description :  Statistics function of JB.  Called by `show_proxy_args'.
 *
 * Parameters  :
 *          1  :  s = string that holds the proxy args description page
 *
 * Returns     :  A pointer to the descriptive status web page.
 *
 *********************************************************************/
char *add_stats(char *s)
{
   /*
    * Output details of the number of requests rejected and
    * accepted. This is switchable in the junkbuster config.
    * Does nothing if this option is not enabled.
    */

   float perc_rej;   /* Percentage of http requests rejected */
   char out_str[81];
   int local_urls_read     = urls_read;
   int local_urls_rejected = urls_rejected;

   /*
    * Need to alter the stats not to include the fetch of this
    * page.
    *
    * Can't do following thread safely! doh!
    *
    * urls_read--;
    * urls_rejected--; * This will be incremented subsequently *
    */

   s = strsav(s,"<h2>Statistics for this " BANNER ":</h2>\n");

   if (local_urls_read == 0)
   {

      s = strsav(s,"No activity so far!\n");

   }
   else
   {

      perc_rej = (float)local_urls_rejected * 100.0F /
            (float)local_urls_read;

      sprintf(out_str,
         "%d requests received, %d filtered "
         "(%6.2f %%).",
         local_urls_read, 
         local_urls_rejected, perc_rej);

      s = strsav(s,out_str);
   }

   return(s);
}
#endif /* def STATISTICS */


/*
  Local Variables:
  tab-width: 3
  end:
*/
