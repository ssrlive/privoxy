const char loaders_rcs[] = "$Id: loaders.c,v 1.12 2001/05/31 17:32:31 oes Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/loaders.c,v $
 *
 * Purpose     :  Functions to load and unload the various
 *                configuration files.  Also contains code to manage
 *                the list of active loaders, and to automatically 
 *                unload files that are no longer in use.
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
 *    $Log: loaders.c,v $
 *    Revision 1.12  2001/05/31 17:32:31  oes
 *
 *     - Enhanced domain part globbing with infix and prefix asterisk
 *       matching and optional unanchored operation
 *
 *    Revision 1.11  2001/05/29 23:25:24  oes
 *
 *     - load_config_line() and load_permissions_file() now use chomp()
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
 *    Revision 1.9  2001/05/26 17:12:07  jongfoster
 *    Fatal errors loading configuration files now give better error messages.
 *
 *    Revision 1.8  2001/05/26 00:55:20  jongfoster
 *    Removing duplicated code.  load_forwardfile() now uses create_url_spec()
 *
 *    Revision 1.7  2001/05/26 00:28:36  jongfoster
 *    Automatic reloading of config file.
 *    Removed obsolete SIGHUP support (Unix) and Reload menu option (Win32).
 *    Most of the global variables have been moved to a new
 *    struct configuration_spec, accessed through csp->config->globalname
 *    Most of the globals remaining are used by the Win32 GUI.
 *
 *    Revision 1.6  2001/05/23 12:27:33  oes
 *
 *    Fixed ugly indentation of my last changes
 *
 *    Revision 1.5  2001/05/23 10:39:05  oes
 *    - Added support for escaping the comment character
 *      in config files by a backslash
 *    - Added support for line continuation in config
 *      files
 *    - Fixed a buffer overflow bug with long config lines
 *
 *    Revision 1.4  2001/05/22 18:56:28  oes
 *    CRLF -> LF
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
 *    Revision 1.1.1.1  2001/05/15 13:58:59  oes
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
#include <sys/stat.h>
#include <ctype.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "project.h"
#include "list.h"
#include "loaders.h"
#include "encode.h"
#include "filters.h"
#include "parsers.h"
#include "jcc.h"
#include "ssplit.h"
#include "miscutil.h"
#include "errlog.h"
#include "gateway.h"
#include "actions.h"

#ifndef SPLIT_PROXY_ARGS
/* For strsav */
#include "showargs.h"
#endif /* ndef SPLIT_PROXY_ARGS */

const char loaders_h_rcs[] = LOADERS_H_VERSION;

/* Fix a problem with Solaris.  There should be no effect on other
 * platforms.
 * Solaris's isspace() is a macro which uses it's argument directly
 * as an array index.  Therefore we need to make sure that high-bit
 * characters generate +ve values, and ideally we also want to make
 * the argument match the declared parameter type of "int".
 */
#define ijb_isspace(__X) isspace((int)(unsigned char)(__X))


/*
 * Currently active files.
 * These are also entered in the main linked list of files.
 */
static struct file_list *current_forwardfile    = NULL;

#ifdef ACL_FILES
static struct file_list *current_aclfile        = NULL;
#endif /* def ACL_FILES */

#ifdef TRUST_FILES
static struct file_list *current_trustfile      = NULL;
#endif /* def TRUST_FILES */

#ifdef PCRS
static struct file_list *current_re_filterfile  = NULL;
#endif /* def PCRS */


/*********************************************************************
 *
 * Function    :  sweep
 *
 * Description :  Basically a mark and sweep garbage collector, it is run
 *                (by the parent thread) every once in a while to reclaim memory.
 *
 * It uses a mark and sweep strategy:
 *   1) mark all files as inactive
 *
 *   2) check with each client:
 *       if it is active,   mark its files as active
 *       if it is inactive, free its resources
 *
 *   3) free the resources of all of the files that
 *      are still marked as inactive (and are obsolete).
 *
 *   N.B. files that are not obsolete don't have an unloader defined.
 *
 * Parameters  :  None
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void sweep(void)
{
   struct file_list *fl, *nfl;
   struct client_state *csp, *ncsp;

   /* clear all of the file's active flags */
   for ( fl = files->next; NULL != fl; fl = fl->next )
   {
      fl->active = 0;
   }

   for (csp = clients; csp && (ncsp = csp->next) ; csp = csp->next)
   {
      if (ncsp->active)
      {
         /* mark this client's files as active */

         /*
          * Always have a configuration file.
          * (Also note the slightly non-standard extra
          * indirection here.)
          */
         ncsp->config->config_file_list->active = 1;

         if (ncsp->actions_list)     /* actions files */
         {
            ncsp->actions_list->active = 1;
         }

         if (ncsp->flist)     /* forward files */
         {
            ncsp->flist->active = 1;
         }

#ifdef ACL_FILES
         if (ncsp->alist)     /* acl files */
         {
            ncsp->alist->active = 1;
         }
#endif /* def ACL_FILES */

#ifdef PCRS
         if (ncsp->rlist)     /* perl re files */
         {
            ncsp->rlist->active = 1;
         }
#endif /* def PCRS */

#ifdef TRUST_FILES
         if (ncsp->tlist)     /* trust files */
         {
            ncsp->tlist->active = 1;
         }
#endif /* def TRUST_FILES */

      }
      else
      {
         /* this client one is not active, release its resources */
         csp->next = ncsp->next;

         freez(ncsp->ip_addr_str);
#ifdef TRUST_FILES
         freez(ncsp->referrer);
#endif /* def TRUST_FILES */
         freez(ncsp->x_forwarded);
         freez(ncsp->iob->buf);

         free_http_request(ncsp->http);

         destroy_list(ncsp->headers);
         destroy_list(ncsp->cookie_list);

         free_current_action(ncsp->action);

#ifdef STATISTICS
         urls_read++;
         if (ncsp->rejected)
         {
            urls_rejected++;
         }
#endif /* def STATISTICS */

         freez(ncsp);
      }
   }

   for (fl = files; fl && (nfl = fl->next) ; fl = fl->next)
   {
      if ( ( 0 == nfl->active ) && ( NULL != nfl->unloader ) )
      {
         fl->next = nfl->next;

         (nfl->unloader)(nfl->f);

#ifndef SPLIT_PROXY_ARGS
         freez(nfl->proxy_args);
#endif /* ndef SPLIT_PROXY_ARGS */

         freez(nfl->filename);

         freez(nfl);
      }
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
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int create_url_spec(struct url_spec * url, char * buf)
{
   char *p;
   struct url_spec tmp_url[1];

   /* paranoia - should never happen. */
   if ((url == NULL) || (buf == NULL))
   {
      return 1;
   }

   /* save a copy of the orignal specification */
   if ((url->spec = strdup(buf)) == NULL)
   {
      return 1;
   }

   if ((p = strchr(buf, '/')))
   {
      if (NULL == (url->path = strdup(p)))
      {
         freez(url->spec);
         return 1;
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
      char rebuf[BUFSIZ];

      if (NULL == (url->preg = zalloc(sizeof(*url->preg))))
      {
         freez(url->spec);
         freez(url->path);
         return 1;
      }

      sprintf(rebuf, "^(%s)", url->path);

      errcode = regcomp(url->preg, rebuf,
            (REG_EXTENDED|REG_NOSUB|REG_ICASE));
      if (errcode)
      {
         size_t errlen =
            regerror(errcode,
               url->preg, buf, sizeof(buf));

         buf[errlen] = '\0';

         log_error(LOG_LEVEL_ERROR, "error compiling %s: %s",
                 url->spec, buf);

         freez(url->spec);
         freez(url->path);
         freez(url->preg);

         return 1;
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

   if ((url->domain = strdup(buf)) == NULL)
   {
      freez(url->spec);
      freez(url->path);
#ifdef REGEX
      freez(url->preg);
#endif /* def REGEX */
      return 1;
   }

   /* split domain into components */

   *tmp_url = dsplit(url->domain);
   url->dbuf = tmp_url->dbuf;
   url->dcnt = tmp_url->dcnt;
   url->dvec = tmp_url->dvec;
   url->unanchored = tmp_url->unanchored;

   return 0; /* OK */
}


/*********************************************************************
 *
 * Function    :  free_url
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
void free_url(struct url_spec *url)
{
   if (url == NULL) return;

   freez(url->spec);
   freez(url->domain);
   freez(url->dbuf);
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


#ifdef ACL_FILES
/*********************************************************************
 *
 * Function    :  unload_aclfile
 *
 * Description :  Unloads an aclfile.
 *
 * Parameters  :
 *          1  :  f = the data structure associated with the aclfile.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void unload_aclfile(void *f)
{
   struct access_control_list *b = (struct access_control_list *)f;
   if (b == NULL) return;

   unload_aclfile(b->next);

   freez(b);

}
#endif /* def ACL_FILES */


#ifdef TRUST_FILES
/*********************************************************************
 *
 * Function    :  unload_trustfile
 *
 * Description :  Unloads a trustfile.
 *
 * Parameters  :
 *          1  :  f = the data structure associated with the trustfile.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void unload_trustfile(void *f)
{
   struct block_spec *b = (struct block_spec *)f;
   if (b == NULL) return;

   unload_trustfile(b->next);

   free_url(b->url);

   freez(b);

}
#endif /* def TRUST_FILES */


/*********************************************************************
 *
 * Function    :  unload_forwardfile
 *
 * Description :  Unloads a forwardfile.
 *
 * Parameters  :
 *          1  :  f = the data structure associated with the forwardfile.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void unload_forwardfile(void *f)
{
   struct forward_spec *b = (struct forward_spec *)f;
   if (b == NULL) return;

   unload_forwardfile(b->next);

   free_url(b->url);

   freez(b->gw->gateway_host);
   freez(b->gw->forward_host);

   freez(b);

}


#ifdef PCRS
/*********************************************************************
 *
 * Function    :  unload_re_filterfile
 *
 * Description :  Unload the re_filter list.
 *
 * Parameters  :
 *          1  :  f = the data structure associated with the filterfile.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void unload_re_filterfile(void *f)
{
   pcrs_job *joblist;
   struct re_filterfile_spec *b = (struct re_filterfile_spec *)f;

   if (b == NULL) return;

   destroy_list(b->patterns);

   joblist = b->joblist;
   while ( NULL != (joblist = pcrs_free_job(joblist)) ) {}

   freez(b);

}
#endif /* def PCRS */


/*********************************************************************
 *
 * Function    :  check_file_changed
 *
 * Description :  Helper function to check if a file needs reloading.
 *                If "current" is still current, return it.  Otherwise
 *                allocates a new (zeroed) "struct file_list", fills 
 *                in the disk file name and timestamp, and returns it.
 *
 * Parameters  :
 *          1  :  current = The file_list currently being used - will
 *                          be checked to see if it is out of date. 
 *                          May be NULL (which is treated as out of
 *                          date).
 *          2  :  filename = Name of file to check.
 *          3  :  newfl    = New file list. [Output only]
 *                           This will be set to NULL, OR a struct
 *                           file_list newly allocated on the
 *                           heap, with the filename and lastmodified
 *                           fields filled, standard header giving file
 *                           name in proxy_args, and all others zeroed.
 *                           (proxy_args is only filled in if !defined
 *                           SPLIT_PROXY_ARGS and !suppress_blocklists).
 *
 * Returns     :  If file unchanged: 0 (and sets newfl == NULL)
 *                If file changed: 1 and sets newfl != NULL
 *                On error: 1 and sets newfl == NULL
 *
 *********************************************************************/
int check_file_changed(const struct file_list * current,
                       const char * filename,
                       struct file_list ** newfl)
{
   struct file_list *fs;
   struct stat statbuf[1];

   *newfl = NULL;

   if (stat(filename, statbuf) < 0)
   {
      /* Error, probably file not found. */
      return 1;
   }

   if (current
       && (current->lastmodified == statbuf->st_mtime)
       && (0 == strcmp(current->filename, filename)))
   {
      return 0;
   }

   fs = (struct file_list *)zalloc(sizeof(struct file_list));

   if (fs == NULL)
   {
      /* Out of memory error */
      return 1;
   }

   fs->filename = strdup(filename);
   fs->lastmodified = statbuf->st_mtime;

   if (fs->filename == NULL)
   {
      /* Out of memory error */
      freez (fs);
      return 1;
   }

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      char * p = html_encode(filename);
      if (p)
      {
         fs->proxy_args = strsav(fs->proxy_args, "<h2>The file `");
         fs->proxy_args = strsav(fs->proxy_args, p);
         fs->proxy_args = strsav(fs->proxy_args, 
            "' contains the following patterns</h2>\n");
         freez(p);
      }
      fs->proxy_args = strsav(fs->proxy_args, "<pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   *newfl = fs;
   return 1;
}


/*********************************************************************
 *
 * Function    :  read_config_line
 *
 * Description :  Read a single non-empty line from a file and return
 *                it.  Trims comments, leading and trailing whitespace
 *                and respects escaping of newline and comment char.
 *                Also writes the file to fs->proxy_args.
 *
 * Parameters  :
 *          1  :  buf = Buffer to use.
 *          2  :  buflen = Size of buffer in bytes.
 *          3  :  fp = File to read from
 *          4  :  fs = File will be written to fs->proxy_args.  May
 *                be NULL to disable this feature.
 *
 * Returns     :  NULL on EOF or error
 *                Otherwise, returns buf.
 *
 *********************************************************************/
char *read_config_line(char *buf, int buflen, FILE *fp, struct file_list *fs)
{
   char *p, *q;
   char linebuf[BUFSIZ];
   int contflag = 0;

   *buf = '\0';

   while (fgets(linebuf, sizeof(linebuf), fp))
   {
#ifndef SPLIT_PROXY_ARGS
      if (fs && !suppress_blocklists)
      {
         char *html_line = html_encode(linebuf);
         if (html_line != NULL)
         {
            fs->proxy_args = strsav(fs->proxy_args, html_line);
            freez(html_line);
         }
         fs->proxy_args = strsav(fs->proxy_args, "<br>");
      }
#endif /* ndef SPLIT_PROXY_ARGS */

      /* Trim off newline */
      if ((p = strpbrk(linebuf, "\r\n")) != NULL)
      {
         *p = '\0';
      }

      /* Line continuation? Trim escape and set flag. */
      if ((p != linebuf) && (*--p == '\\'))
      {
         contflag = 1;
         *p = '\0';
      }

      /* If there's a comment char.. */
      if ((p = strpbrk(linebuf, "#")) != NULL)
      {
         /* ..and it's escaped, left-shift the line over the escape. */
         if ((p != linebuf) && (*(p-1) == '\\'))
         {
            q = p-1;
            while ((*q++ = *p++) != '\0') /* nop */;
         }
         /* Else, chop off the rest of the line */
         else
         {
            *p = '\0';
         }
      }
      
      /* Remove leading and trailing whitespace */
      chomp(linebuf);

      if (*linebuf)
      {
         strncat(buf, linebuf, buflen - strlen(buf));
         if (contflag)
         {
            contflag = 0;
            continue;
         }
         else
         {
            return buf;
         }
      }
   }
   /* EOF */
   return NULL;

}


#ifdef ACL_FILES
/*********************************************************************
 *
 * Function    :  load_aclfile
 *
 * Description :  Read and parse an aclfile and add to files list.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int load_aclfile(struct client_state *csp)
{
   FILE *fp;
   char buf[BUFSIZ], *v[3], *p;
   int i;
   struct access_control_list *a, *bl;
   struct file_list *fs;

   if (!check_file_changed(current_aclfile, csp->config->aclfile, &fs))
   {
      /* No need to load */
      if (csp)
      {
         csp->alist = current_aclfile;
      }
      return(0);
   }
   if (!fs)
   {
      goto load_aclfile_error;
   }

   fs->f = bl = (struct access_control_list *)zalloc(sizeof(*bl));
   if (bl == NULL)
   {
      freez(fs->filename);
      freez(fs);
      goto load_aclfile_error;
   }

   fp = fopen(csp->config->aclfile, "r");

   if (fp == NULL)
   {
      goto load_aclfile_error;
   }

   while (read_config_line(buf, sizeof(buf), fp, fs) != NULL)
   {
      i = ssplit(buf, " \t", v, SZ(v), 1, 1);

      /* allocate a new node */
      a = (struct access_control_list *) zalloc(sizeof(*a));

      if (a == NULL)
      {
         fclose(fp);
         freez(fs->f);
         freez(fs->filename);
         freez(fs);
         goto load_aclfile_error;
      }

      /* add it to the list */
      a->next  = bl->next;
      bl->next = a;

      switch (i)
      {
         case 3:
            if (acl_addr(v[2], a->dst) < 0)
            {
               goto load_aclfile_error;
            }
            /* no break */

         case 2:
            if (acl_addr(v[1], a->src) < 0)
            {
               goto load_aclfile_error;
            }

            p = v[0];
            if (strcmpic(p, "permit") == 0)
            {
               a->action = ACL_PERMIT;
               break;
            }

            if (strcmpic(p, "deny") == 0)
            {
               a->action = ACL_DENY;
               break;
            }
            /* no break */

         default:
            goto load_aclfile_error;
      }
   }

   fclose(fp);

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   if (current_aclfile)
   {
      current_aclfile->unloader = unload_aclfile;
   }

   fs->next = files->next;
   files->next = fs;
   current_aclfile = fs;

   if (csp)
   {
      csp->alist = fs;
   }

   return(0);

load_aclfile_error:
   log_error(LOG_LEVEL_FATAL, "can't load access control list %s: %E",
             csp->config->aclfile);
   return(-1);

}
#endif /* def ACL_FILES */


#ifdef TRUST_FILES
/*********************************************************************
 *
 * Function    :  load_trustfile
 *
 * Description :  Read and parse a trustfile and add to files list.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int load_trustfile(struct client_state *csp)
{
   FILE *fp;

   struct block_spec *b, *bl;
   struct url_spec **tl;

   char  buf[BUFSIZ], *p, *q;
   int reject, trusted;
   struct file_list *fs;

   if (!check_file_changed(current_trustfile, csp->config->trustfile, &fs))
   {
      /* No need to load */
      if (csp)
      {
         csp->tlist = current_trustfile;
      }
      return(0);
   }
   if (!fs)
   {
      goto load_trustfile_error;
   }

   fs->f = bl = (struct block_spec *)zalloc(sizeof(*bl));
   if (bl == NULL)
   {
      goto load_trustfile_error;
   }

   if ((fp = fopen(csp->config->trustfile, "r")) == NULL)
   {
      goto load_trustfile_error;
   }

   tl = csp->config->trust_list;

   while (read_config_line(buf, sizeof(buf), fp, fs) != NULL)
   {
      trusted = 0;
      reject  = 1;

      if (*buf == '+')
      {
         trusted = 1;
         *buf = '~';
      }

      if (*buf == '~')
      {
         reject = 0;
         p = buf;
         q = p+1;
         while ((*p++ = *q++))
         {
            /* nop */
         }
      }

      /* skip blank lines */
      if (*buf == '\0')
      {
         continue;
      }

      /* allocate a new node */
      if ((b = zalloc(sizeof(*b))) == NULL)
      {
         fclose(fp);
         goto load_trustfile_error;
      }

      /* add it to the list */
      b->next  = bl->next;
      bl->next = b;

      b->reject = reject;

      /* Save the URL pattern */
      if (create_url_spec(b->url, buf))
      {
         fclose(fp);
         goto load_trustfile_error;
      }

      /*
       * save a pointer to URL's spec in the list of trusted URL's, too
       */
      if (trusted)
      {
         *tl++ = b->url;
      }
   }

   *tl = NULL;

   fclose(fp);

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   /* the old one is now obsolete */
   if (current_trustfile)
   {
      current_trustfile->unloader = unload_trustfile;
   }

   fs->next    = files->next;
   files->next = fs;
   current_trustfile = fs;

   if (csp)
   {
      csp->tlist = fs;
   }

   return(0);

load_trustfile_error:
   log_error(LOG_LEVEL_FATAL, "can't load trustfile '%s': %E",
             csp->config->trustfile);
   return(-1);

}
#endif /* def TRUST_FILES */


/*********************************************************************
 *
 * Function    :  load_forwardfile
 *
 * Description :  Read and parse a forwardfile and add to files list.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int load_forwardfile(struct client_state *csp)
{
   FILE *fp;

   struct forward_spec *b, *bl;
   char buf[BUFSIZ];
   char *p, *tmp;
   char *vec[4];
   int n;
   struct file_list *fs;
   const struct gateway *gw;

   if (!check_file_changed(current_forwardfile, csp->config->forwardfile, &fs))
   {
      /* No need to load */
      if (csp)
      {
         csp->flist = current_forwardfile;
      }
      return(0);
   }
   if (!fs)
   {
      goto load_forwardfile_error;
   }

   fs->f = bl = (struct forward_spec  *)zalloc(sizeof(*bl));

   if ((fs == NULL) || (bl == NULL))
   {
      goto load_forwardfile_error;
   }

   if ((fp = fopen(csp->config->forwardfile, "r")) == NULL)
   {
      goto load_forwardfile_error;
   }

   tmp = NULL;

   while (read_config_line(buf, sizeof(buf), fp, fs) != NULL)
   {
      freez(tmp);

      tmp = strdup(buf);

      n = ssplit(tmp, " \t", vec, SZ(vec), 1, 1);

      if (n != 4)
      {
         log_error(LOG_LEVEL_ERROR, "error in forwardfile: %s", buf);
         continue;
      }

      strcpy(buf, vec[0]);

      /* skip lines containing only ~ */
      if (*buf == '\0')
      {
         continue;
      }

      /* allocate a new node */
      if (((b = zalloc(sizeof(*b))) == NULL)
      )
      {
         fclose(fp);
         goto load_forwardfile_error;
      }

      /* add it to the list */
      b->next  = bl->next;
      bl->next = b;

      /* Save the URL pattern */
      if (create_url_spec(b->url, buf))
      {
         fclose(fp);
         goto load_forwardfile_error;
      }

      /* now parse the gateway specs */

      p = vec[2];

      for (gw = gateways; gw->name; gw++)
      {
         if (strcmp(gw->name, p) == 0)
         {
            break;
         }
      }

      if (gw->name == NULL)
      {
         goto load_forwardfile_error;
      }

      /* save this as the gateway type */
      *b->gw = *gw;

      /* now parse the gateway host[:port] spec */
      p = vec[3];

      if (strcmp(p, ".") != 0)
      {
         b->gw->gateway_host = strdup(p);

         if ((p = strchr(b->gw->gateway_host, ':')))
         {
            *p++ = '\0';
            b->gw->gateway_port = atoi(p);
         }

         if (b->gw->gateway_port <= 0)
         {
            goto load_forwardfile_error;
         }
      }

      /* now parse the forwarding spec */
      p = vec[1];

      if (strcmp(p, ".") != 0)
      {
         b->gw->forward_host = strdup(p);

         if ((p = strchr(b->gw->forward_host, ':')))
         {
            *p++ = '\0';
            b->gw->forward_port = atoi(p);
         }

         if (b->gw->forward_port <= 0)
         {
            b->gw->forward_port = 8000;
         }
      }
   }

   freez(tmp);

   fclose(fp);

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   /* the old one is now obsolete */
   if (current_forwardfile)
   {
      current_forwardfile->unloader = unload_forwardfile;
   }

   fs->next    = files->next;
   files->next = fs;
   current_forwardfile = fs;

   if (csp)
   {
      csp->flist = fs;
   }

   return(0);

load_forwardfile_error:
   log_error(LOG_LEVEL_FATAL, "can't load forwardfile '%s': %E",
             csp->config->forwardfile);
   return(-1);

}


#ifdef PCRS
/*********************************************************************
 *
 * Function    :  load_re_filterfile
 *
 * Description :  Load the re_filterfile. Each non-comment, non-empty
 *                line is instantly added to the joblist, which is
 *                a chained list of pcrs_job structs.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int load_re_filterfile(struct client_state *csp)
{
   FILE *fp;

   struct re_filterfile_spec *bl;
   struct file_list *fs;

   char  buf[BUFSIZ];
   int error;
   pcrs_job *dummy;

   if (!check_file_changed(current_re_filterfile, csp->config->re_filterfile, &fs))
   {
      /* No need to load */
      if (csp)
      {
         csp->rlist = current_re_filterfile;
      }
      return(0);
   }
   if (!fs)
   {
      goto load_re_filterfile_error;
   }

   fs->f = bl = (struct re_filterfile_spec  *)zalloc(sizeof(*bl));
   if (bl == NULL)
   {
      goto load_re_filterfile_error;
   }

   /* Open the file or fail */
   if ((fp = fopen(csp->config->re_filterfile, "r")) == NULL)
   {
      goto load_re_filterfile_error;
   }

   /* Read line by line */
   while (read_config_line(buf, sizeof(buf), fp, fs) != NULL)
   {
      enlist( bl->patterns, buf );

      /* We have a meaningful line -> make it a job */
      if ((dummy = pcrs_make_job(buf, &error)) == NULL)
      {
         log_error(LOG_LEVEL_RE_FILTER, 
               "Adding re_filter job %s failed with error %d.", buf, error);
         continue;
      }
      else
      {
         dummy->next = bl->joblist;
         bl->joblist = dummy;
         log_error(LOG_LEVEL_RE_FILTER, "Adding re_filter job %s succeeded.", buf);
      }
   }

   fclose(fp);

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   /* the old one is now obsolete */
   if ( NULL != current_re_filterfile )
   {
      current_re_filterfile->unloader = unload_re_filterfile;
   }

   fs->next    = files->next;
   files->next = fs;
   current_re_filterfile = fs;

   if (csp)
   {
      csp->rlist = fs;
   }

   return( 0 );

load_re_filterfile_error:
   log_error(LOG_LEVEL_FATAL, "can't load re_filterfile '%s': %E", 
             csp->config->re_filterfile);
   return(-1);

}
#endif /* def PCRS */


/*********************************************************************
 *
 * Function    :  add_loader
 *
 * Description :  Called from `load_config'.  Called once for each input
 *                file found in config.
 *
 * Parameters  :
 *          1  :  loader = pointer to a function that can parse and load
 *                the appropriate config file.
 *          2  :  config = The configuration_spec to add the loader to.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void add_loader(int (*loader)(struct client_state *), 
                struct configuration_spec * config)
{
   int i;

   for (i=0; i < NLOADERS; i++)
   {
      if (config->loaders[i] == NULL)
      {
         config->loaders[i] = loader;
         break;
      }
   }

}


/*********************************************************************
 *
 * Function    :  run_loader
 *
 * Description :  Called from `load_config' and `listen_loop'.  This
 *                function keeps the "csp" current with any file mods
 *                since the last loop.  If a file is unchanged, the
 *                loader functions do NOT reload the file.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *                      Must be non-null.  Reads: "csp->config"
 *                      Writes: various data members.
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int run_loader(struct client_state *csp)
{
   int ret = 0;
   int i;

   for (i=0; i < NLOADERS; i++)
   {
      if (csp->config->loaders[i] == NULL)
      {
         break;
      }
      ret |= (csp->config->loaders[i])(csp);
   }
   return(ret);

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
