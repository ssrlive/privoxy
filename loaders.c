const char loaders_rcs[] = "$Id: loaders.c,v 1.1 2001/05/13 21:57:06 administrator Exp $";
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/loaders.c,v $
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
#include "loaders.h"
#include "encode.h"
#include "filters.h"
#include "parsers.h"
#include "jcc.h"
#include "ssplit.h"
#include "miscutil.h"
#include "errlog.h"
#include "gateway.h"

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


#define NLOADERS 8
static int (*loaders[NLOADERS])(struct client_state *);


/*
 * Currently active files.
 * These are also entered in the main linked list of files.
 */
static struct file_list *current_blockfile      = NULL;
static struct file_list *current_cookiefile     = NULL;
static struct file_list *current_forwardfile    = NULL;

#ifdef ACL_FILES
static struct file_list *current_aclfile        = NULL;
#endif /* def ACL_FILES */

#ifdef USE_IMAGE_LIST
static struct file_list *current_imagefile      = NULL;
#endif /* def USE_IMAGE_LIST */

#ifdef KILLPOPUPS
static struct file_list * current_popupfile     = NULL;
#endif /* def KILLPOPUPS */

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

         if (ncsp->blist)     /* block files */
         {
            ncsp->blist->active = 1;
         }

         if (ncsp->clist)     /* cookie files */
         {
            ncsp->clist->active = 1;
         }

         /* FIXME: These were left out of the "10" release.  Should they be here? */
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

#ifdef USE_IMAGE_LIST
         if (ncsp->ilist)     /* image files */
         {
            ncsp->ilist->active = 1;
         }
#endif /* def USE_IMAGE_LIST */

#ifdef KILLPOPUPS
         if (ncsp->plist)     /* killpopup files */
         {
            ncsp->plist->active = 1;
         }
#endif /* def KILLPOPUPS */

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
         freez(ncsp->referrer);
         freez(ncsp->x_forwarded);
         freez(ncsp->ip_addr_str);
         freez(ncsp->iob->buf);

         free_http_request(ncsp->http);

         destroy_list(ncsp->headers);
         destroy_list(ncsp->cookie_list);

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
 * Function    :  unload_url
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
static void unload_url(struct url_spec *url)
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

/*********************************************************************
 *
 * Function    :  unload_blockfile
 *
 * Description :  Unloads a blockfile.
 *
 * Parameters  :
 *          1  :  f = the data structure associated with the blockfile.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void unload_blockfile(void *f)
{
   struct block_spec *b = (struct block_spec *)f;
   if (b == NULL) return;

   unload_blockfile(b->next);

   unload_url(b->url);

   freez(b);

}


#ifdef USE_IMAGE_LIST
/*********************************************************************
 *
 * Function    :  unload_imagefile
 *
 * Description :  Unloads an imagefile.
 *
 * Parameters  :
 *          1  :  f = the data structure associated with the imagefile.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void unload_imagefile(void *f)
{
   struct block_spec *b = (struct block_spec *)f;
   if (b == NULL) return;

   unload_imagefile(b->next);

   unload_url(b->url);

   freez(b);

}
#endif /* def USE_IMAGE_LIST */


/*********************************************************************
 *
 * Function    :  unload_cookiefile
 *
 * Description :  Unloads a cookiefile.
 *
 * Parameters  :
 *          1  :  f = the data structure associated with the cookiefile.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void unload_cookiefile(void *f)
{
   struct cookie_spec *b = (struct cookie_spec *)f;
   if (b == NULL) return;

   unload_cookiefile(b->next);

   unload_url(b->url);

   freez(b);

}


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

   unload_url(b->url);

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

   unload_url(b->url);

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


#ifdef KILLPOPUPS
/*********************************************************************
 *
 * Function    :  unload_popupfile
 *
 * Description :  Free the lists of blocked, and allowed popup sites.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void unload_popupfile(void * b)
{
   struct popup_settings * data = (struct popup_settings *) b;
   struct popup_blocklist * cur = NULL;
   struct popup_blocklist * temp= NULL;

   /* Free the blocked list. */
   cur = data->blocked;
   while (cur != NULL)
   {
      temp = cur->next;
      freez (cur->host_name);
      free  (cur);
      cur  = temp;
   }
   data->blocked = NULL;

   /* Free the allowed list. */
   cur = data->allowed;
   while (cur != NULL)
   {
      temp = cur->next;
      freez (cur->host_name);
      free  (cur);
      cur  = temp;
   }
   data->allowed = NULL;

}
#endif /* def KILLPOPUPS */


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
 *                           (proxy_args is only filled in if
 *                           SPLIT_PROXY_ARGS and !suppress_blocklists).
 *
 * Returns     :  If file unchanged: 0 (and sets newfl == NULL)
 *                If file changed: 1 and sets newfl != NULL
 *                On error: 1 and sets newfl == NULL
 *
 *********************************************************************/
static int check_file_changed(const struct file_list * current,
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
 *                it.  Trims comments, leading and trailing whitespace.
 *                Also wites the file to fs->proxy_args.
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

      /* Trim off newline and any comment */
      if ((p = strpbrk(linebuf, "\r\n#")) != NULL)
      {
         *p = '\0';
      }
      
      /* Trim leading whitespace */
      p = linebuf;
      while (*p && ijb_isspace(*p))
      {
         *p++;
      }

      if (*p)
      {
         /* There is something other than whitespace on the line. */

         /* Move the data to the start of buf */
         if (p != linebuf)
         {
            /* strcpy that can cope with overlap. */
            q = linebuf;
            while ((*q++ = *p++) != '\0')
            {
               /* Do nothing */
            }
         }

         /* Trim trailing whitespace */
         p = linebuf + strlen(linebuf) - 1;

         /*
          * Note: the (p >= retval) below is paranoia, it's not really needed.
          * When p == retval then ijb_isspace(*p) will be false and we'll drop
          * out of the loop.
          */
         while ((p >= linebuf) && ijb_isspace(*p))
         {
            p--;
         }
         p[1] = '\0';

         /* More paranoia.  This if statement is always true. */
         if (*linebuf)
         {
            strcpy(buf, linebuf);
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

   if (!check_file_changed(current_aclfile, aclfile, &fs))
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

   fp = fopen(aclfile, "r");

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
   log_error(LOG_LEVEL_ERROR, "can't load access control list %s: %E", aclfile);
   return(-1);

}
#endif /* def ACL_FILES */


/*********************************************************************
 *
 * Function    :  load_blockfile
 *
 * Description :  Read and parse a blockfile and add to files list.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int load_blockfile(struct client_state *csp)
{
   FILE *fp;

   struct block_spec *b, *bl;
   char  buf[BUFSIZ], *p, *q;
   int port, reject;
   struct file_list *fs;
   struct url_spec url[1];

   if (!check_file_changed(current_blockfile, blockfile, &fs))
   {
      /* No need to load */
      if (csp)
      {
         csp->blist = current_blockfile;
      }
      return(0);
   }
   if (!fs)
   {
      goto load_blockfile_error;
   }

   fs->f = bl = (struct block_spec *) zalloc(sizeof(*bl));
   if (bl == NULL)
   {
      goto load_blockfile_error;
   }

   if ((fp = fopen(blockfile, "r")) == NULL)
   {
      goto load_blockfile_error;
   }

   while (read_config_line(buf, sizeof(buf), fp, fs) != NULL)
   {
      reject = 1;

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

      /* skip lines containing only ~ */
      if (*buf == '\0')
      {
         continue;
      }

      /* allocate a new node */
      if (((b = zalloc(sizeof(*b))) == NULL)
#ifdef REGEX
          || ((b->url->preg = zalloc(sizeof(*b->url->preg))) == NULL)
#endif
      )
      {
         fclose(fp);
         goto load_blockfile_error;
      }

      /* add it to the list */
      b->next  = bl->next;
      bl->next = b;

      /* save a copy of the orignal specification */
      if ((b->url->spec = strdup(buf)) == NULL)
      {
         fclose(fp);
         goto load_blockfile_error;
      }

      b->reject = reject;

      if ((p = strchr(buf, '/')))
      {
         b->url->path    = strdup(p);
         b->url->pathlen = strlen(b->url->path);
         *p = '\0';
      }
      else
      {
         b->url->path    = NULL;
         b->url->pathlen = 0;
      }
#ifdef REGEX
      if (b->url->path)
      {
         int errcode;
         char rebuf[BUFSIZ];

         sprintf(rebuf, "^(%s)", b->url->path);

         errcode = regcomp(b->url->preg, rebuf,
               (REG_EXTENDED|REG_NOSUB|REG_ICASE));

         if (errcode)
         {
            size_t errlen =
               regerror(errcode,
                  b->url->preg, buf, sizeof(buf));

            buf[errlen] = '\0';

            log_error(LOG_LEVEL_ERROR, "error compiling %s: %s\n",
                    b->url->spec, buf);
            fclose(fp);
            goto load_blockfile_error;
         }
      }
      else
      {
         freez(b->url->preg);
      }
#endif
      if ((p = strchr(buf, ':')) == NULL)
      {
         port = 0;
      }
      else
      {
         *p++ = '\0';
         port = atoi(p);
      }

      b->url->port = port;

      if ((b->url->domain = strdup(buf)) == NULL)
      {
         fclose(fp);
         goto load_blockfile_error;
      }

      /* split domain into components */
      *url = dsplit(b->url->domain);
      b->url->dbuf = url->dbuf;
      b->url->dcnt = url->dcnt;
      b->url->dvec = url->dvec;
   }

   fclose(fp);

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   /* the old one is now obsolete */
   if (current_blockfile)
   {
      current_blockfile->unloader = unload_blockfile;
   }

   fs->next    = files->next;
   files->next = fs;
   current_blockfile = fs;

   if (csp)
   {
      csp->blist = fs;
   }

   return(0);

load_blockfile_error:
   log_error(LOG_LEVEL_ERROR, "can't load blockfile '%s': %E", blockfile);
   return(-1);

}


#ifdef USE_IMAGE_LIST
/*********************************************************************
 *
 * Function    :  load_imagefile
 *
 * Description :  Read and parse an imagefile and add to files list.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int load_imagefile(struct client_state *csp)
{
   FILE *fp;

   struct block_spec *b, *bl;
   char  buf[BUFSIZ], *p, *q;
   int port, reject;
   struct file_list *fs;
   struct url_spec url[1];

   if (!check_file_changed(current_imagefile, imagefile, &fs))
   {
      /* No need to load */
      if (csp)
      {
         csp->ilist = current_imagefile;
      }
      return(0);
   }
   if (!fs)
   {
      goto load_imagefile_error;
   }

   fs->f = bl = (struct block_spec *)zalloc(sizeof(*bl));
   if (bl == NULL)
   {
      goto load_imagefile_error;
   }

   if ((fp = fopen(imagefile, "r")) == NULL)
   {
      goto load_imagefile_error;
   }

   while (read_config_line(buf, sizeof(buf), fp, fs) != NULL)
   {
      reject = 1;

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

      /* skip lines containing only ~ */
      if (*buf == '\0')
      {
         continue;
      }

      /* allocate a new node */
      if (((b = zalloc(sizeof(*b))) == NULL)
#ifdef REGEX
      || ((b->url->preg = zalloc(sizeof(*b->url->preg))) == NULL)
#endif
      )
      {
         fclose(fp);
         goto load_imagefile_error;
      }

      /* add it to the list */
      b->next  = bl->next;
      bl->next = b;

      /* save a copy of the orignal specification */
      if ((b->url->spec = strdup(buf)) == NULL)
      {
         fclose(fp);
         goto load_imagefile_error;
      }

      b->reject = reject;

      if ((p = strchr(buf, '/')))
      {
         b->url->path    = strdup(p);
         b->url->pathlen = strlen(b->url->path);
         *p = '\0';
      }
      else
      {
         b->url->path    = NULL;
         b->url->pathlen = 0;
      }
#ifdef REGEX
      if (b->url->path)
      {
         int errcode;
         char rebuf[BUFSIZ];

         sprintf(rebuf, "^(%s)", b->url->path);

         errcode = regcomp(b->url->preg, rebuf,
               (REG_EXTENDED|REG_NOSUB|REG_ICASE));

         if (errcode)
         {
            size_t errlen =
               regerror(errcode,
                  b->url->preg, buf, sizeof(buf));

            buf[errlen] = '\0';

            log_error(LOG_LEVEL_ERROR, "error compiling %s: %s",
                    b->url->spec, buf);
            fclose(fp);
            goto load_imagefile_error;
         }
      }
      else
      {
         freez(b->url->preg);
      }
#endif
      if ((p = strchr(buf, ':')) == NULL)
      {
         port = 0;
      }
      else
      {
         *p++ = '\0';
         port = atoi(p);
      }

      b->url->port = port;

      if ((b->url->domain = strdup(buf)) == NULL)
      {
         fclose(fp);
         goto load_imagefile_error;
      }

      /* split domain into components */
      *url = dsplit(b->url->domain);
      b->url->dbuf = url->dbuf;
      b->url->dcnt = url->dcnt;
      b->url->dvec = url->dvec;
   }
#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
#endif /* ndef SPLIT_PROXY_ARGS */

   fclose(fp);

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   /* the old one is now obsolete */
   if (current_imagefile)
   {
      current_imagefile->unloader = unload_imagefile;
   }

   fs->next    = files->next;
   files->next = fs;
   current_imagefile = fs;

   if (csp)
   {
      csp->ilist = fs;
   }

   return(0);

load_imagefile_error:
   log_error(LOG_LEVEL_ERROR, "can't load imagefile '%s': %E", imagefile);
   return(-1);

}
#endif /* def USE_IMAGE_LIST */


/*********************************************************************
 *
 * Function    :  load_cookiefile
 *
 * Description :  Read and parse a cookiefile and add to files list.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int load_cookiefile(struct client_state *csp)
{
   FILE *fp;

   struct cookie_spec *b, *bl;
   char  buf[BUFSIZ], *p, *q;
   int port, user_cookie, server_cookie;
   struct file_list *fs;
   struct url_spec url[1];

   if (!check_file_changed(current_cookiefile, cookiefile, &fs))
   {
      /* No need to load */
      if (csp)
      {
         csp->clist = current_cookiefile;
      }
      return(0);
   }
   if (!fs)
   {
      goto load_cookie_error;
   }

   fs->f = bl = (struct cookie_spec   *)zalloc(sizeof(*bl));
   if (bl == NULL)
   {
      goto load_cookie_error;
   }

   if ((fp = fopen(cookiefile, "r")) == NULL)
   {
      goto load_cookie_error;
   }

   while (read_config_line(buf, sizeof(buf), fp, fs) != NULL)
   {
      p = buf;

      switch ((int)*p)
      {
         case '>':
            server_cookie = 0;
            user_cookie   = 1;
            p++;
            break;

         case '<':
            server_cookie = 1;
            user_cookie   = 0;
            p++;
            break;

         case '~':
            server_cookie = 0;
            user_cookie   = 0;
            p++;
            break;

         default:
            server_cookie = 1;
            user_cookie   = 1;
            break;
      }

      /*
       * Elide any of the "special" chars from the
       * front of the pattern
       */
      q = buf;
      if (p > q) while ((*q++ = *p++))
      {
         /* nop */
      }

      /* skip lines containing only "special" chars */
      if (*buf == '\0')
      {
         continue;
      }

      /* allocate a new node */
      if (((b = zalloc(sizeof(*b))) == NULL)
#ifdef REGEX
      || ((b->url->preg = zalloc(sizeof(*b->url->preg))) == NULL)
#endif
      )
      {
         fclose(fp);
         goto load_cookie_error;
      }

      /* add it to the list */
      b->next  = bl->next;
      bl->next = b;

      /* save a copy of the orignal specification */
      if ((b->url->spec = strdup(buf)) == NULL)
      {
         fclose(fp);
         goto load_cookie_error;
      }

      b->send_user_cookie     = user_cookie;
      b->accept_server_cookie = server_cookie;

      if ((p = strchr(buf, '/')))
      {
         b->url->path    = strdup(p);
         b->url->pathlen = strlen(b->url->path);
         *p = '\0';
      }
      else
      {
         b->url->path    = NULL;
         b->url->pathlen = 0;
      }
#ifdef REGEX
      if (b->url->path)
      {
         int errcode;
         char rebuf[BUFSIZ];

         sprintf(rebuf, "^(%s)", b->url->path);

         errcode = regcomp(b->url->preg, rebuf,
               (REG_EXTENDED|REG_NOSUB|REG_ICASE));
         if (errcode)
         {
            size_t errlen =
               regerror(errcode,
                  b->url->preg, buf, sizeof(buf));

            buf[errlen] = '\0';

            log_error(LOG_LEVEL_ERROR, "error compiling %s: %s",
                    b->url->spec, buf);
            fclose(fp);
            goto load_cookie_error;
         }
      }
      else
      {
         freez(b->url->preg);
      }
#endif
      if ((p = strchr(buf, ':')) == NULL)
      {
         port = 0;
      }
      else
      {
         *p++ = '\0';
         port = atoi(p);
      }

      b->url->port = port;

      if ((b->url->domain = strdup(buf)) == NULL)
      {
         fclose(fp);
         goto load_cookie_error;
      }

      /* split domain into components */

      *url = dsplit(b->url->domain);
      b->url->dbuf = url->dbuf;
      b->url->dcnt = url->dcnt;
      b->url->dvec = url->dvec;
   }

   fclose(fp);

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   /* the old one is now obsolete */
   if (current_cookiefile)
   {
      current_cookiefile->unloader = unload_cookiefile;
   }

   fs->next    = files->next;
   files->next = fs;
   current_cookiefile = fs;

   if (csp)
   {
      csp->clist = fs;
   }

   return(0);

load_cookie_error:
   log_error(LOG_LEVEL_ERROR, "can't load cookiefile '%s': %E", cookiefile);
   return(-1);

}


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
   int port, reject, trusted;
   struct file_list *fs;
   struct url_spec url[1];

   if (!check_file_changed(current_trustfile, trustfile, &fs))
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

   if ((fp = fopen(trustfile, "r")) == NULL)
   {
      goto load_trustfile_error;
   }

   tl = trust_list;

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
      if (((b = zalloc(sizeof(*b))) == NULL)
#ifdef REGEX
      || ((b->url->preg = zalloc(sizeof(*b->url->preg))) == NULL)
#endif
      )
      {
         fclose(fp);
         goto load_trustfile_error;
      }

      /* add it to the list */
      b->next  = bl->next;
      bl->next = b;

      /* save a copy of the orignal specification */
      if ((b->url->spec = strdup(buf)) == NULL)
      {
         fclose(fp);
         goto load_trustfile_error;
      }

      b->reject = reject;

      if ((p = strchr(buf, '/')))
      {
         b->url->path    = strdup(p);
         b->url->pathlen = strlen(b->url->path);
         *p = '\0';
      }
      else
      {
         b->url->path    = NULL;
         b->url->pathlen = 0;
      }
#ifdef REGEX
      if (b->url->path)
      {
         int errcode;
         char rebuf[BUFSIZ];

         sprintf(rebuf, "^(%s)", b->url->path);

         errcode = regcomp(b->url->preg, rebuf,
               (REG_EXTENDED|REG_NOSUB|REG_ICASE));

         if (errcode)
         {
            size_t errlen =
               regerror(errcode,
                  b->url->preg, buf, sizeof(buf));

            buf[errlen] = '\0';

            log_error(LOG_LEVEL_ERROR, "error compiling %s: %s",
                    b->url->spec, buf);
            fclose(fp);
            goto load_trustfile_error;
         }
      }
      else
      {
         freez(b->url->preg);
      }
#endif
      if ((p = strchr(buf, ':')) == NULL)
      {
         port = 0;
      }
      else
      {
         *p++ = '\0';
         port = atoi(p);
      }

      b->url->port = port;

      if ((b->url->domain = strdup(buf)) == NULL)
      {
         fclose(fp);
         goto load_trustfile_error;
      }

      /* split domain into components */
      *url = dsplit(b->url->domain);
      b->url->dbuf = url->dbuf;
      b->url->dcnt = url->dcnt;
      b->url->dvec = url->dvec;

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
   log_error(LOG_LEVEL_ERROR, "can't load trustfile '%s': %E", trustfile);
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
   char  buf[BUFSIZ], *p, *q, *tmp;
   char  *vec[4];
   int port, n, reject;
   struct file_list *fs;
   const struct gateway *gw;
   struct url_spec url[1];

   if (!check_file_changed(current_forwardfile, forwardfile, &fs))
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

   if ((fp = fopen(forwardfile, "r")) == NULL)
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

      reject = 1;

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

      /* skip lines containing only ~ */
      if (*buf == '\0')
      {
         continue;
      }

      /* allocate a new node */
      if (((b = zalloc(sizeof(*b))) == NULL)
#ifdef REGEX
      || ((b->url->preg = zalloc(sizeof(*b->url->preg))) == NULL)
#endif
      )
      {
         fclose(fp);
         goto load_forwardfile_error;
      }

      /* add it to the list */
      b->next  = bl->next;
      bl->next = b;

      /* save a copy of the orignal specification */
      if ((b->url->spec = strdup(buf)) == NULL)
      {
         fclose(fp);
         goto load_forwardfile_error;
      }

      b->reject = reject;

      if ((p = strchr(buf, '/')))
      {
         b->url->path    = strdup(p);
         b->url->pathlen = strlen(b->url->path);
         *p = '\0';
      }
      else
      {
         b->url->path    = NULL;
         b->url->pathlen = 0;
      }
#ifdef REGEX
      if (b->url->path)
      {
         int errcode;
         char rebuf[BUFSIZ];

         sprintf(rebuf, "^(%s)", b->url->path);

         errcode = regcomp(b->url->preg, rebuf,
               (REG_EXTENDED|REG_NOSUB|REG_ICASE));

         if (errcode)
         {
            size_t errlen = regerror(errcode, b->url->preg, buf, sizeof(buf));

            buf[errlen] = '\0';

            log_error(LOG_LEVEL_ERROR, "error compiling %s: %s",
                    b->url->spec, buf);
            fclose(fp);
            goto load_forwardfile_error;
         }
      }
      else
      {
         freez(b->url->preg);
      }
#endif
      if ((p = strchr(buf, ':')) == NULL)
      {
         port = 0;
      }
      else
      {
         *p++ = '\0';
         port = atoi(p);
      }

      b->url->port = port;

      if ((b->url->domain = strdup(buf)) == NULL)
      {
         fclose(fp);
         goto load_forwardfile_error;
      }

      /* split domain into components */
      *url = dsplit(b->url->domain);
      b->url->dbuf = url->dbuf;
      b->url->dcnt = url->dcnt;
      b->url->dvec = url->dvec;

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
   log_error(LOG_LEVEL_ERROR, "can't load forwardfile '%s': %E", forwardfile);
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

#ifndef SPLIT_PROXY_ARGS
   char *p;
#endif /* ndef SPLIT_PROXY_ARGS */
   if (!check_file_changed(current_re_filterfile, re_filterfile, &fs))
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
   if ((fp = fopen(re_filterfile, "r")) == NULL)
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
         log_error(LOG_LEVEL_REF, 
               "Adding re_filter job %s failed with error %d.", buf, error);
         continue;
      }
      else
      {
         dummy->next = bl->joblist;
         bl->joblist = dummy;
         log_error(LOG_LEVEL_REF, "Adding re_filter job %s succeeded.", buf);
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
   log_error(LOG_LEVEL_ERROR, "can't load re_filterfile '%s': %E", re_filterfile);
   return(-1);

}
#endif /* def PCRS */


#ifdef KILLPOPUPS
/*********************************************************************
 *
 * Function    :  load_popupfile
 *
 * Description :  Load, and parse the popup blocklist.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 => success, else there was an error.
 *
 *********************************************************************/
int load_popupfile(struct client_state *csp)
{
   FILE *fp;
   char  buf[BUFSIZ], *p, *q;
   struct popup_blocklist *entry = NULL;
   struct popup_settings * data;
   struct file_list *fs;
   p = buf;
   q = buf;

   if (!check_file_changed(current_popupfile, popupfile, &fs))
   {
      /* No need to load */
      if (csp)
      {
         csp->plist = current_popupfile;
      }
      return(0);
   }
   if (!fs)
   {
      goto load_popupfile_error;
   }

   fs->f = data = (struct popup_settings  *)zalloc(sizeof(*data));
   if (data == NULL)
   {
      goto load_popupfile_error;
   }

   if ((fp = fopen(popupfile, "r")) == NULL)
   {
      goto load_popupfile_error;
   }

   while (read_config_line(buf, sizeof(buf), fp, fs) != NULL)
   {
      entry = (struct popup_blocklist*)zalloc(sizeof(struct popup_blocklist));
      if (!entry)
      {
         fclose( fp );
         goto load_popupfile_error;
      }

      /* Handle allowed hosts. */
      if ( *buf == '~' )
      {
         /* Rememeber: skip the tilde */
         entry->host_name = strdup( buf + 1 );
         if (!entry->host_name)
         {
            fclose( fp );
            goto load_popupfile_error;
         }

         entry->next = data->allowed;
         data->allowed = entry;
      }
      else
      {
         /* Blocked host */
         entry->host_name = strdup( buf );
         if (!entry->host_name)
         {
            fclose( fp );
            goto load_popupfile_error;
         }

         entry->next = data->blocked;
         data->blocked = entry;
      }
   }

   fclose( fp );

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   /* the old one is now obsolete */
   if ( NULL != current_popupfile )
   {
      current_popupfile->unloader = unload_popupfile;
   }

   fs->next    = files->next;
   files->next = fs;
   current_popupfile = fs;

   if (csp)
   {
      csp->plist = fs;
   }

   return( 0 );

load_popupfile_error:
   log_error(LOG_LEVEL_ERROR, "can't load popupfile '%s': %E", popupfile);
   return(-1);

}
#endif /* def KILLPOPUPS */



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
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void add_loader(int (*loader)(struct client_state *))
{
   int i;

   for (i=0; i < NLOADERS; i++)
   {
      if (loaders[i] == NULL)
      {
         loaders[i] = loader;
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
      if (loaders[i] == NULL)
      {
         break;
      }
      ret |= (loaders[i])(csp);
   }
   return(ret);

}


/*********************************************************************
 *
 * Function    :  remove_all_loaders
 *
 * Description :  Remove all loaders from the list.
 *
 * Parameters  :  N/A
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void remove_all_loaders(void)
{
   memset( loaders, 0, sizeof( loaders ) );
}


/*
  Local Variables:
  tab-width: 3
  end:
*/
