const char filters_rcs[] = "$Id: filters.c,v 1.1 2001/05/13 21:57:06 administrator Exp $";
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/filters.c,v $
 *
 * Purpose     :  Declares functions to parse/crunch headers and pages.
 *                Functions declared include:
 *                   `acl_addr', `add_stats', `block_acl', `block_imageurl',
 *                   `block_url', `cookie_url', `domaincmp', `dsplit',
 *                   `filter_popups', `forward_url',
 *                   `ij_untrusted_url', `intercept_url', `re_process_buffer',
 *                   `show_proxy_args', and `trust_url'
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
      "<p align=center>Your request for <b>%s%s</b><br>\n"
      "was blocked because it matches the following pattern "
      "in the blockfile: <b>%s</b>\n</p>"
#ifdef FORCE_LOAD
       "<p align=center><a href=\"http://" FORCE_PREFIX
        "%s%s\">Go there anyway.</a></p>"
#endif /* def FORCE_LOAD */
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
       "<a href=http://internet.junkbuster.com/ij-untrusted-url?%s+%s+%s>"
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
 *          1  :  src = Address the browser/user agent is requesting.
 *          2  :  dst = The proxy or gateway address this is going to.
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     : 0 = FALSE (don't block) and 1 = TRUE (do block)
 *
 *********************************************************************/
int block_acl(struct access_control_addr *src, struct access_control_addr *dst, struct client_state *csp)
{
   struct file_list *fl;
   struct access_control_list *a, *acl;
   struct access_control_addr s[1], d[1];

   /* if not using an access control list, then permit the connection */
   if (((fl = csp->alist) == NULL) || ((acl = fl->f) == NULL))
   {
      return(0);
   }

   /* search the list */
   for (a = acl->next ; a ; a = a->next)
   {
      *s = *src;
      *d = *dst;

      s->addr &= a->src->mask;
      d->addr &= a->dst->mask;

      if ((s->addr  == a->src->addr)
      && (d->addr  == a->dst->addr)
      && ((s->port == a->src->port)
       || (s->port == 0)
       || (a->src->port == 0))
      && ((d->port == a->dst->port)
       || (d->port == 0)
       || (a->dst->port == 0)))
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

   return(1);

}


/*********************************************************************
 *
 * Function    :  acl_addr
 *
 * Description :  Called from `load_aclfile'.  FIXME: I can't say more.
 *
 * Parameters  :
 *          1  :  aspec = (what?)
 *          2  :  aca = (what?)
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
   struct file_list *fl;
   struct block_spec *b;
   struct url_spec url[1];
   char *p;
   int n;

   if (((fl = csp->blist) == NULL) || ((b = fl->f) == NULL))
   {
      return(NULL);
   }

   *url = dsplit(http->host);

   /* if splitting the domain fails, punt */
   if (url->dbuf == NULL) return(NULL);

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

               n  = strlen(CBLOCK);
               n += strlen(http->hostport);
               n += strlen(http->path);
               n += strlen(b->url->spec);
#ifdef FORCE_LOAD
               n += strlen(http->hostport);
               n += strlen(http->path);
#endif /* def FORCE_LOAD */

               p = (char *)malloc(n);

#ifdef FORCE_LOAD
               sprintf(p, CBLOCK, http->hostport, http->path, b->url->spec, http->hostport, http->path);
#else
               sprintf(p, CBLOCK, http->hostport, http->path, b->url->spec);
#endif /* def FORCE_LOAD */

               return(p);
            }
         }
      }
   }
   freez(url->dbuf);
   freez(url->dvec);
   return(NULL);

}


#if defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST)
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

#if defined(USE_IMAGE_LIST)
   return block_imageurl_using_imagelist(http, csp);
#else
   /* Don't know - assume HTML */
   return 0;
#endif
}
#endif /* defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST) */


#ifdef USE_IMAGE_LIST
/*********************************************************************
 *
 * Function    :  block_imageurl
 *
 * Description :  Test if a URL is in the imagelist.
 *
 * Parameters  :
 *          1  :  http = URL to check.
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  True (nonzero) if URL is in image list, false (0)
 *                otherwise
 *
 *********************************************************************/
int block_imageurl_using_imagelist(struct http_request *http, struct client_state *csp)
{
   struct file_list *fl;
   struct block_spec *b;
   struct url_spec url[1];

   if (((fl = csp->ilist) == NULL) || ((b  = fl->f) == NULL))
   {
      return(0);
   }

   *url = dsplit(http->host);

   /* if splitting the domain fails, punt */
   if (url->dbuf == NULL) return(0);

   for (b = b->next; b ; b = b->next)
   {

      if ((b->url->port == 0) || (b->url->port == http->port))
      {
         /* port matches, check domain */
         if ((b->url->domain[0] == '\0') || (domaincmp(b->url, url) == 0))
         {
            /* domain matches, check path */
            if ((b->url->path == NULL) ||
#ifdef REGEX
               (regexec(b->url->preg, http->path, 0, NULL, 0) == 0)
#else
               (strncmp(b->url->path, http->path, b->url->pathlen) == 0)
#endif
            )
            {
               /* Matches */
               freez(url->dbuf);
               freez(url->dvec);

               if (b->reject == 0) return(0);

               return(1);
            }
         }
      }
   }
   freez(url->dbuf);
   freez(url->dvec);
   return(0);

}
#endif /* def USE_IMAGE_LIST */


#ifdef PCRS
/*********************************************************************
 *
 * Function    :  re_process_buffer
 *
 * Description :  Apply all jobs from the joblist (aka. Perl regexp's) to
 *                the text buffer that's been accumulated in csp->iob->buf.
 *                Then, write the modified buffer out to the client
 *                (Maybe this should happen from jcc.c via flush_socket
 *                for better readability).
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void re_process_buffer(struct client_state *csp)
{
   int hits=0;
   int size = csp->iob->eod - csp->iob->cur;
   char *old=csp->iob->cur, *new = NULL;
   pcrs_job *job, *joblist;

   struct file_list *fl;
   struct re_filterfile_spec *b;

   /* Sanity first ;-) */
   if (size <= 0) return;

   if ( ( NULL == (fl = csp->rlist) ) || ( NULL == (b = fl->f) ) )
   {
      log_error(LOG_LEVEL_ERROR, "Unable to get current state of regexp filtering.");
      return;
   }

   joblist = b->joblist;


   log_error(LOG_LEVEL_REF, "re_filtering %s%s (size %d) ...",
              csp->http->hostport, csp->http->path, size);

   /* Apply all jobs from the joblist */
   for (job = joblist; NULL != job; job = job->next)
   {
      hits += pcrs_exec_substitution(job, old, size, &new, &size);
      if (old != csp->iob->cur) free(old);
      old=new;
   }

   log_error(LOG_LEVEL_REF, " produced %d hits (new size %d).", hits, size);

   if (write_socket(csp->cfd, old, size) != size)
   {
      log_error(LOG_LEVEL_ERROR, "write to client failed.");
   }

   /* fwiw, reset the iob */
   IOB_RESET(csp);
   freez(new);
   return;

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

   for (tl = trust_list; (t = *tl) ; tl++)
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

               if ((fp = fopen(trustfile, "a")))
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
 * Returns     :  NULL for no recognized URLs, or an HTML description page.
 *
 *********************************************************************/
char *intercept_url(struct http_request *http, struct client_state *csp)
{
   char *basename;
   const struct interceptors *v;

   basename = strrchr(http->path, '/');

   if (basename == NULL) return(NULL);

   basename ++; /* first char past the last slash */

   if (*basename)
   {
      for (v = intercept_patterns; v->str; v++)
      {
         if (strncmp(basename, v->str, v->len) == 0)
         {
            return((v->interceptor)(http, csp));
         }
      }
   }

   return(NULL);

}


/*********************************************************************
 *
 * Function    :  cookie_url
 *
 * Description :  Accept this cookie, or no?  See "cookiefile" setting.
 *
 * Parameters  :
 *          1  :  http = http_request request for blocked URLs
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  NULL => accept, cookie_spec pointer to crunch.
 *
 *********************************************************************/
struct cookie_spec *cookie_url(struct http_request *http, struct client_state *csp)
{
   struct file_list *fl;
   struct cookie_spec *b;
   struct url_spec url[1];

   if (((fl = csp->clist) == NULL) || ((b = fl->f) == NULL))
   {
      return(NULL);
   }

   *url = dsplit(http->host);

   /* if splitting the domain fails, punt */
   if (url->dbuf == NULL) return(NULL);

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
               freez(url->dbuf);
               freez(url->dvec);
               return(b);
            }
         }
      }
   }

   freez(url->dbuf);
   freez(url->dvec);
   return(NULL);

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

   if ((p = strrchr(domain, '.')))
   {
      if (*(++p) == '\0')
      {
         ret->toplevel = 1;
      }
   }

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
 *                domaincmp("a.b.c" , "a.b.c")  => 0 (MATCH)
 *                domaincmp("a*.b.c", "a.b.c")  => 0 (MATCH)
 *                domaincmp("b.c"   , "a.b.c")  => 0 (MATCH)
 *                domaincmp(""      , "a.b.c")  => 0 (MATCH)
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
   pn = pattern->dcnt;

   fv = fqdn->dvec;
   fn = fqdn->dcnt;

   while ((pn > 0) && (fn > 0))
   {
      p = pv[--pn];
      f = fv[--fn];

      while (*p && *f && (*p == tolower(*f)))
      {
         p++, f++;
      }

      if ((*p != tolower(*f)) && (*p != '*')) return(1);
   }

   if (pn > 0) return(1);

   return(0);

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
   case 'b':
      if (csp->blist)
      {
         filename = csp->blist->filename;
         file_description = "Block List";
      }
      break;
   case 'c':
      if (csp->clist)
      {
         filename = csp->clist->filename;
         file_description = "Cookie List";
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

#ifdef USE_IMAGE_LIST
   case 'i':
      if (csp->ilist)
      {
         filename = csp->ilist->filename;
         file_description = "Image List";
      }
      break;
#endif /* def USE_IMAGE_LIST */

#ifdef KILLPOPUPS
   case 'p':
      if (csp->plist)
      {
         filename = csp->plist->filename;
         file_description = "Popup list";
      }
      break;
#endif /* def KILLPOPUPS */

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
         "</body></html>\n");
      return(s);
   }
#endif /* def SPLIT_PROXY_ARGS */
   
   s = strsav(s, proxy_args->header);
   s = strsav(s, proxy_args->invocation);
#ifdef STATISTICS
   s = add_stats(s);
#endif /* def STATISTICS */
   s = strsav(s, proxy_args->gateways);

#ifdef SPLIT_PROXY_ARGS
   s = strsav(s, 
      "<h2>The following files are in use:</h2>\n"
      "<p>(Click a filename to view it)</p>\n"
      "<ul>\n");

   if (csp->blist)
   {
      s = strsav(s, "<li>Block List: <a href=\"show-proxy-args?block\"><code>");
      s = strsav(s, csp->blist->filename);
      s = strsav(s, "</code></a></li>\n");
   }

   if (csp->clist)
   {
      s = strsav(s, "<li>Cookie List: <a href=\"show-proxy-args?cookie\"><code>");
      s = strsav(s, csp->clist->filename);
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

#ifdef USE_IMAGE_LIST
   if (csp->ilist)
   {
      s = strsav(s, "<li>Image List: <a href=\"show-proxy-args?image\"><code>");
      s = strsav(s, csp->ilist->filename);
      s = strsav(s, "</code></a></li>\n");
   }
#endif /* def USE_IMAGE_LIST */

#ifdef KILLPOPUPS
   if (csp->plist)
   {
      s = strsav(s, "<li>Popup List: <a href=\"show-proxy-args?popup\"><code>");
      s = strsav(s, csp->plist->filename);
      s = strsav(s, "</code></a></li>\n");
   }
#endif /* def KILLPOPUPS */

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
   if (csp->blist)
   {
      s = strsav(s, csp->blist->proxy_args);
   }

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

#ifdef USE_IMAGE_LIST
   if (csp->ilist)
   {
      s = strsav(s, csp->ilist->proxy_args);
   }
#endif /* def USE_IMAGE_LIST */

#ifdef KILLPOPUPS
   if (csp->plist)
   {
      s = strsav(s, csp->plist->proxy_args);
   }
#endif /* def KILLPOPUPS */

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

   s = strsav(s, proxy_args->trailer);

   return(s);

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

   for (tl = trust_list; (t = *tl) ; tl++)
   {
      sprintf(buf, "%s<br>\n", t->spec);
      p = strsav(p, buf);
   }

   if (trust_info->next)
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

      for (l = trust_info->next; l ; l = l->next)
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
