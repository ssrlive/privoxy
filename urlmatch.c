const char urlmatch_rcs[] = "$Id: urlmatch.c JGF $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/urlmatch.c,v $
 *
 * Purpose     :  Declares functions to match URLs against URL
 *                patterns.
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
 *    $Log: urlmatch.c,v $
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
#include "urlmatch.h"
#include "ssplit.h"
#include "miscutil.h"
#include "errlog.h"

const char urlmatch_h_rcs[] = URLMATCH_H_VERSION;

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
   assert(http);

   freez(http->cmd);
   freez(http->gpc);
   freez(http->host);
   freez(http->url);
   freez(http->hostport);
   freez(http->path);
   freez(http->ver);
   freez(http->host_ip_addr_str);
   freez(http->dbuffer);
   freez(http->dvec);
   http->dcount = 0;
}


/*********************************************************************
 *
 * Function    :  parse_http_url
 *
 * Description :  Parse out the host and port from the URL.  Find the
 *                hostname & path, port (if ':'), and/or password (if '@')
 *
 * Parameters  :
 *          1  :  url = URL (or is it URI?) to break down
 *          2  :  http = pointer to the http structure to hold elements.
 *                       Will be zeroed before use.  Note that this
 *                       function sets the http->gpc and http->ver
 *                       members to NULL.
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out of memory
 *                JB_ERR_CGI_PARAMS on malformed command/URL
 *                                  or >100 domains deep.
 *
 *********************************************************************/
jb_err parse_http_url(const char * url,
                      struct http_request *http,
                      struct client_state *csp)
{
   /*
    * Zero out the results structure
    */
   memset(http, '\0', sizeof(*http));


   /*
    * Save our initial URL
    */
   http->url = strdup(url);
   if (http->url == NULL)
   {
      return JB_ERR_MEMORY;
   }


   /*
    * Split URL into protocol,hostport,path.
    */
   {
      char *buf;
      char *url_noproto;
      char *url_path;

      buf = strdup(url);
      if (buf == NULL)
      {
         return JB_ERR_MEMORY;
      }

      /* Find the start of the URL in our scratch space */
      url_noproto = buf;
      if (strncmpic(url_noproto, "http://",  7) == 0)
      {
         url_noproto += 7;
         http->ssl = 0;
      }
      else if (strncmpic(url_noproto, "https://", 8) == 0)
      {
         url_noproto += 8;
         http->ssl = 1;
      }
      else
      {
         http->ssl = 0;
      }

      url_path = strchr(url_noproto, '/');
      if (url_path != NULL)
      {
         /*
          * Got a path.
          *
          * NOTE: The following line ignores the path for HTTPS URLS.
          * This means that you get consistent behaviour if you type a
          * https URL in and it's parsed by the function.  (When the
          * URL is actually retrieved, SSL hides the path part).
          */
         http->path = strdup(http->ssl ? "/" : url_path);
         *url_path = '\0';
         http->hostport = strdup(url_noproto);
      }
      else
      {
         /*
          * Repair broken HTTP requests that don't contain a path,
          * or CONNECT requests
          */
         http->path = strdup("/");
         http->hostport = strdup(url_noproto);
      }

      free(buf);

      if ( (http->path == NULL)
        || (http->hostport == NULL))
      {
         free(buf);
         free_http_request(http);
         return JB_ERR_MEMORY;
      }
   }


   /*
    * Split hostport into user/password (ignored), host, port.
    */
   {
      char *buf;
      char *host;
      char *port;

      buf = strdup(http->hostport);
      if (buf == NULL)
      {
         free_http_request(http);
         return JB_ERR_MEMORY;
      }

      /* check if url contains username and/or password */
      host = strchr(buf, '@');
      if (host != NULL)
      {
         /* Contains username/password, skip it and the @ sign. */
         host++;
      }
      else
      {
         /* No username or password. */
         host = buf;
      }

      /* check if url contains port */
      port = strchr(host, ':');
      if (port != NULL)
      {
         /* Contains port */
         /* Terminate hostname and point to start of port string */
         *port++ = '\0';
         http->port = atoi(port);
      }
      else
      {
         /* No port specified. */
         http->port = (http->ssl ? 143 : 80);
      }

      http->host = strdup(host);

      free(buf);

      if (http->host == NULL)
      {
         free_http_request(http);
         return JB_ERR_MEMORY;
      }
   }


   /*
    * Split domain name so we can compare it against wildcards
    */
   {
      char *vec[BUFFER_SIZE];
      int size;
      char *p;

      http->dbuffer = strdup(http->host);
      if (NULL == http->dbuffer)
      {
         free_http_request(http);
         return JB_ERR_MEMORY;
      }

      /* map to lower case */
      for (p = http->dbuffer; *p ; p++)
      {
         *p = tolower((int)(unsigned char)*p);
      }

      /* split the domain name into components */
      http->dcount = ssplit(http->dbuffer, ".", vec, SZ(vec), 1, 1);

      if (http->dcount <= 0)
      {
         // Error: More than SZ(vec) components in domain
         //    or: no components in domain
         free_http_request(http);
         return JB_ERR_PARSE;
      }

      /* save a copy of the pointers in dvec */
      size = http->dcount * sizeof(*http->dvec);

      http->dvec = (char **)malloc(size);
      if (NULL == http->dvec)
      {
         free_http_request(http);
         return JB_ERR_MEMORY;
      }

      memcpy(http->dvec, vec, size);
   }


   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  parse_http_request
 *
 * Description :  Parse out the host and port from the URL.  Find the
 *                hostname & path, port (if ':'), and/or password (if '@')
 *
 * Parameters  :
 *          1  :  req = HTTP request line to break down
 *          2  :  http = pointer to the http structure to hold elements
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out of memory
 *                JB_ERR_CGI_PARAMS on malformed command/URL
 *                                  or >100 domains deep.
 *
 *********************************************************************/
jb_err parse_http_request(const char *req,
                          struct http_request *http,
                          struct client_state *csp)
{
   char *buf;
   char *v[10];
   int n;
   jb_err err;
   int is_connect = 0;

   memset(http, '\0', sizeof(*http));

   buf = strdup(req);
   if (buf == NULL)
   {
      return JB_ERR_MEMORY;
   }

   n = ssplit(buf, " \r\n", v, SZ(v), 1, 1);
   if (n != 3)
   {
      free(buf);
      return JB_ERR_PARSE;
   }

   /* this could be a CONNECT request */
   if (strcmpic(v[0], "connect") == 0)
   {
      /* Secure */
      is_connect = 1;
   }
   /* or it could be any other basic HTTP request type */
   else if ((0 == strcmpic(v[0], "get"))
         || (0 == strcmpic(v[0], "head"))
         || (0 == strcmpic(v[0], "post"))
         || (0 == strcmpic(v[0], "put"))
         || (0 == strcmpic(v[0], "delete"))
 
         /* or a webDAV extension (RFC2518) */
         || (0 == strcmpic(v[0], "propfind"))
         || (0 == strcmpic(v[0], "proppatch"))
         || (0 == strcmpic(v[0], "move"))
         || (0 == strcmpic(v[0], "copy"))
         || (0 == strcmpic(v[0], "mkcol"))
         || (0 == strcmpic(v[0], "lock"))
         || (0 == strcmpic(v[0], "unlock"))
         )
   {
      /* Normal */
      is_connect = 0;
   }
   else
   {
      /* Unknown HTTP method */
      free(buf);
      return JB_ERR_PARSE;
   }

   err = parse_http_url(v[1], http, csp);
   if (err)
   {
      free(buf);
      return err;
   }

   /*
    * Copy the details into the structure
    */
   http->ssl = is_connect;
   http->cmd = strdup(req);
   http->gpc = strdup(v[0]);
   http->ver = strdup(v[2]);

   if ( (http->cmd == NULL)
     || (http->gpc == NULL)
     || (http->ver == NULL) )
   {
      free(buf);
      free_http_request(http);
      return JB_ERR_MEMORY;
   }

   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  simple_domaincmp
 *
 * Description :  Domain-wise Compare fqdn's.  The comparison is
 *                both left- and right-anchored.  The individual
 *                domain names are compared with simplematch().
 *                This is only used by domain_match.
 *
 * Parameters  :
 *          1  :  pv = array of patterns to compare
 *          2  :  fv = array of domain components to compare
 *          3  :  len = length of the arrays (both arrays are the
 *                      same length - if they weren't, it couldn't
 *                      possibly be a match).
 *
 * Returns     :  0 => domains are equivalent, else no match.
 *
 *********************************************************************/
static int simple_domaincmp(char **pv, char **fv, int len)
{
   int n;

   for (n = 0; n < len; n++)
   {
      if (simplematch(pv[n], fv[n]))
      {
         return 1;
      }
   }

   return 0;

}


/*********************************************************************
 *
 * Function    :  domain_match
 *
 * Description :  Domain-wise Compare fqdn's. Governed by the bimap in
 *                pattern->unachored, the comparison is un-, left-,
 *                right-anchored, or both.
 *                The individual domain names are compared with
 *                simplematch().
 *
 * Parameters  :
 *          1  :  pattern = a domain that may contain a '*' as a wildcard.
 *          2  :  fqdn = domain name against which the patterns are compared.
 *
 * Returns     :  0 => domains are equivalent, else no match.
 *
 *********************************************************************/
static int domain_match(const struct url_spec *pattern, const struct http_request *fqdn)
{
   char **pv, **fv;  /* vectors  */
   int    plen, flen;
   int unanchored = pattern->unanchored & (ANCHOR_RIGHT | ANCHOR_LEFT);

   plen = pattern->dcount;
   flen = fqdn->dcount;

   if (flen < plen)
   {
      /* fqdn is too short to match this pattern */
      return 1;
   }

   pv   = pattern->dvec;
   fv   = fqdn->dvec;

   if (unanchored == ANCHOR_LEFT)
   {
      /*
       * Right anchored.
       *
       * Convert this into a fully anchored pattern with
       * the fqdn and pattern the same length
       */
      fv += (flen - plen); /* flen - plen >= 0 due to check above */
      return simple_domaincmp(pv, fv, plen);
   }
   else if (unanchored == 0)
   {
      /* Fully anchored, check length */
      if (flen != plen)
      {
         return 1;
      }
      return simple_domaincmp(pv, fv, plen);
   }
   else if (unanchored == ANCHOR_RIGHT)
   {
      /* Left anchored, ignore all extra in fqdn */
      return simple_domaincmp(pv, fv, plen);
   }
   else
   {
      /* Unanchored */
      int n;
      int maxn = flen - plen;
      for (n = 0; n <= maxn; n++)
      {
         if (!simple_domaincmp(pv, fv, plen))
         {
            return 0;
         }
         /*
          * Doesn't match from start of fqdn
          * Try skipping first part of fqdn
          */
         fv++;
      }
      return 1;
   }

}


/*********************************************************************
 *
 * Function    :  create_url_spec
 *
 * Description :  Creates a "url_spec" structure from a string.
 *                When finished, free with unload_url().
 *
 * Parameters  :
 *          1  :  url = Target url_spec to be filled in.  Must be
 *                      zeroed out before the call (e.g. using zalloc).
 *          2  :  buf = Source pattern, null terminated.  NOTE: The
 *                      contents of this buffer are destroyed by this
 *                      function.  If this function succeeds, the
 *                      buffer is copied to url->spec.  If this
 *                      function fails, the contents of the buffer
 *                      are lost forever.
 *
 * Returns     :  JB_ERR_OK - Success
 *                JB_ERR_MEMORY - Out of memory
 *                JB_ERR_PARSE - Cannot parse regex (Detailed message
 *                               written to system log)
 *
 *********************************************************************/
jb_err create_url_spec(struct url_spec * url, const char * buf)
{
   char *p;

   assert(url);
   assert(buf);

   /* save a copy of the orignal specification */
   if ((url->spec = strdup(buf)) == NULL)
   {
      return JB_ERR_MEMORY;
   }

   if ((p = strchr(buf, '/')))
   {
      if (NULL == (url->path = strdup(p)))
      {
         freez(url->spec);
         return JB_ERR_MEMORY;
      }
      url->pathlen = strlen(url->path);
      *p = '\0';
   }
   else
   {
      url->path    = NULL;
      url->pathlen = 0;
   }
#ifdef REGEX
   if (url->path)
   {
      int errcode;
      char rebuf[BUFFER_SIZE];

      if (NULL == (url->preg = zalloc(sizeof(*url->preg))))
      {
         freez(url->spec);
         freez(url->path);
         return JB_ERR_MEMORY;
      }

      sprintf(rebuf, "^(%s)", url->path);

      errcode = regcomp(url->preg, rebuf,
            (REG_EXTENDED|REG_NOSUB|REG_ICASE));
      if (errcode)
      {
         size_t errlen = regerror(errcode,
            url->preg, rebuf, sizeof(rebuf));

         if (errlen > (sizeof(rebuf) - (size_t)1))
         {
            errlen = sizeof(rebuf) - (size_t)1;
         }
         rebuf[errlen] = '\0';

         log_error(LOG_LEVEL_ERROR, "error compiling %s: %s",
            url->spec, rebuf);

         freez(url->spec);
         freez(url->path);
         freez(url->preg);

         return JB_ERR_PARSE;
      }
   }
#endif
   if ((p = strchr(buf, ':')) == NULL)
   {
      url->port = 0;
   }
   else
   {
      *p++ = '\0';
      url->port = atoi(p);
   }

   if (buf[0] != '\0')
   {
      char *v[150];
      int size;

      /* Parse domain part */
      if (buf[strlen(buf) - 1] == '.')
      {
         url->unanchored |= ANCHOR_RIGHT;
      }
      if (buf[0] == '.')
      {
         url->unanchored |= ANCHOR_LEFT;
      }

      /* split domain into components */

      url->dbuffer = strdup(buf);
      if (NULL == url->dbuffer)
      {
         freez(url->spec);
         freez(url->path);
#ifdef REGEX
         freez(url->preg);
#endif /* def REGEX */
         return JB_ERR_MEMORY;
      }

      /* map to lower case */
      for (p = url->dbuffer; *p ; p++)
      {
         *p = tolower((int)(unsigned char)*p);
      }

      /* split the domain name into components */
      url->dcount = ssplit(url->dbuffer, ".", v, SZ(v), 1, 1);

      if (url->dcount < 0)
      {
         freez(url->spec);
         freez(url->path);
#ifdef REGEX
         freez(url->preg);
#endif /* def REGEX */
         freez(url->dbuffer);
         url->dcount = 0;
         return JB_ERR_MEMORY;
      }
      else if (url->dcount != 0)
      {

         /* save a copy of the pointers in dvec */
         size = url->dcount * sizeof(*url->dvec);

         url->dvec = (char **)malloc(size);
         if (NULL == url->dvec)
         {
            freez(url->spec);
            freez(url->path);
#ifdef REGEX
            freez(url->preg);
#endif /* def REGEX */
            freez(url->dbuffer);
            url->dcount = 0;
            return JB_ERR_MEMORY;
         }

         memcpy(url->dvec, v, size);
      }
   }

   return JB_ERR_OK;

}


/*********************************************************************
 *
 * Function    :  free_url_spec
 *
 * Description :  Called from the "unloaders".  Freez the url
 *                structure elements.
 *
 * Parameters  :
 *          1  :  url = pointer to a url_spec structure.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void free_url_spec(struct url_spec *url)
{
   if (url == NULL) return;

   freez(url->spec);
   freez(url->dbuffer);
   freez(url->dvec);
   freez(url->path);
#ifdef REGEX
   if (url->preg)
   {
      regfree(url->preg);
      freez(url->preg);
   }
#endif

}


/*********************************************************************
 *
 * Function    :  url_match
 *
 * Description :  Compare a URL against a URL pattern.
 *
 * Parameters  :
 *          1  :  pattern = a URL pattern
 *          2  :  url = URL to match
 *
 * Returns     :  0 iff the URL matches the pattern, else nonzero.
 *
 *********************************************************************/
int url_match(const struct url_spec *pattern,
              const struct http_request *url)
{
   return ((pattern->port == 0) || (pattern->port == url->port))
       && ((pattern->dbuffer == NULL) || (domain_match(pattern, url) == 0))
       && ((pattern->path == NULL) ||
#ifdef REGEX
            (regexec(pattern->preg, url->path, 0, NULL, 0) == 0)
#else
            (strncmp(pattern->path, url->path, pattern->pathlen) == 0)
#endif
      );
}


/*
  Local Variables:
  tab-width: 3
  end:
*/
