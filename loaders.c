const char loaders_rcs[] = "$Id: loaders.c,v 1.35 2002/01/17 21:03:08 jongfoster Exp $";
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
 *    Revision 1.35  2002/01/17 21:03:08  jongfoster
 *    Moving all our URL and URL pattern parsing code to urlmatch.c.
 *
 *    Renaming free_url to free_url_spec, since it frees a struct url_spec.
 *
 *    Revision 1.34  2001/12/30 14:07:32  steudten
 *    - Add signal handling (unix)
 *    - Add SIGHUP handler (unix)
 *    - Add creation of pidfile (unix)
 *    - Add action 'top' in rc file (RH)
 *    - Add entry 'SIGNALS' to manpage
 *    - Add exit message to logfile (unix)
 *
 *    Revision 1.33  2001/11/13 00:16:38  jongfoster
 *    Replacing references to malloc.h with the standard stdlib.h
 *    (See ANSI or K&R 2nd Ed)
 *
 *    Revision 1.32  2001/11/07 00:02:13  steudten
 *    Add line number in error output for lineparsing for
 *    actionsfile and configfile.
 *    Special handling for CLF added.
 *
 *    Revision 1.31  2001/10/26 17:39:01  oes
 *    Removed csp->referrer
 *    Moved ijb_isspace and ijb_tolower to project.h
 *
 *    Revision 1.30  2001/10/25 03:40:48  david__schmidt
 *    Change in porting tactics: OS/2's EMX porting layer doesn't allow multiple
 *    threads to call select() simultaneously.  So, it's time to do a real, live,
 *    native OS/2 port.  See defines for __EMX__ (the porting layer) vs. __OS2__
 *    (native). Both versions will work, but using __OS2__ offers multi-threading.
 *
 *    Revision 1.29  2001/10/23 21:38:53  jongfoster
 *    Adding error-checking to create_url_spec()
 *
 *    Revision 1.28  2001/10/07 15:40:39  oes
 *    Replaced 6 boolean members of csp with one bitmap (csp->flags)
 *
 *    Revision 1.27  2001/09/22 16:36:59  jongfoster
 *    Removing unused parameter fs from read_config_line()
 *
 *    Revision 1.26  2001/09/22 14:05:22  jongfoster
 *    Bugfix: Multiple escaped "#" characters in a configuration
 *    file are now permitted.
 *    Also removing 3 unused headers.
 *
 *    Revision 1.25  2001/09/13 22:44:03  jongfoster
 *    Adding {} to an if statement
 *
 *    Revision 1.24  2001/07/30 22:08:36  jongfoster
 *    Tidying up #defines:
 *    - All feature #defines are now of the form FEATURE_xxx
 *    - Permanently turned off WIN_GUI_EDIT
 *    - Permanently turned on WEBDAV and SPLIT_PROXY_ARGS
 *
 *    Revision 1.23  2001/07/20 15:51:54  oes
 *    Fixed indentation of prepocessor commands
 *
 *    Revision 1.22  2001/07/20 15:16:17  haroon
 *    - per Guy's suggestion, added a while loop in sweep() to catch not just
 *      the last inactive CSP but all other consecutive inactive CSPs after that
 *      as well
 *
 *    Revision 1.21  2001/07/18 17:26:24  oes
 *    Changed to conform to new pcrs interface
 *
 *    Revision 1.20  2001/07/17 13:07:01  oes
 *    Fixed segv when last line in config files
 *     lacked a terminating (\r)\n
 *
 *    Revision 1.19  2001/07/13 14:01:54  oes
 *    Removed all #ifdef PCRS
 *
 *    Revision 1.18  2001/06/29 21:45:41  oes
 *    Indentation, CRLF->LF, Tab-> Space
 *
 *    Revision 1.17  2001/06/29 13:31:51  oes
 *    Various adaptions
 *
 *    Revision 1.16  2001/06/09 10:55:28  jongfoster
 *    Changing BUFSIZ ==> BUFFER_SIZE
 *
 *    Revision 1.15  2001/06/07 23:14:14  jongfoster
 *    Removing ACL and forward file loaders - these
 *    files have been merged into the config file.
 *    Cosmetic: Moving unloader funcs next to their
 *    respective loader funcs
 *
 *    Revision 1.14  2001/06/01 03:27:04  oes
 *    Fixed line continuation problem
 *
 *    Revision 1.13  2001/05/31 21:28:49  jongfoster
 *    Removed all permissionsfile code - it's now called the actions
 *    file, and (almost) all the code is in actions.c
 *
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
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <assert.h>

#if !defined(_WIN32) && !defined(__OS2__)
#include <unistd.h>
#endif

#include "project.h"
#include "list.h"
#include "loaders.h"
#include "filters.h"
#include "parsers.h"
#include "jcc.h"
#include "miscutil.h"
#include "errlog.h"
#include "actions.h"
#include "urlmatch.h"

const char loaders_h_rcs[] = LOADERS_H_VERSION;

/*
 * Currently active files.
 * These are also entered in the main linked list of files.
 */

#ifdef FEATURE_TRUST
static struct file_list *current_trustfile      = NULL;
#endif /* def FEATURE_TRUST */

static struct file_list *current_re_filterfile  = NULL;



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
      if (ncsp->flags & CSP_FLAG_ACTIVE)
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

         if (ncsp->rlist)     /* pcrsjob files */
         {
            ncsp->rlist->active = 1;
         }

#ifdef FEATURE_TRUST
         if (ncsp->tlist)     /* trust files */
         {
            ncsp->tlist->active = 1;
         }
#endif /* def FEATURE_TRUST */

      }
      else
      /*
       * this client is not active, release its resources
       * and the ones of all inactive clients that might
       * follow it
       */
      {
         while (!(ncsp->flags & CSP_FLAG_ACTIVE))
         {
            csp->next = ncsp->next;

            freez(ncsp->ip_addr_str);
            freez(ncsp->my_ip_addr_str);
            freez(ncsp->my_hostname);
            freez(ncsp->x_forwarded);
            freez(ncsp->iob->buf);

            free_http_request(ncsp->http);

            destroy_list(ncsp->headers);
            destroy_list(ncsp->cookie_list);

            free_current_action(ncsp->action);

#ifdef FEATURE_STATISTICS
            urls_read++;
            if (ncsp->flags & CSP_FLAG_REJECTED)
            {
               urls_rejected++;
            }
#endif /* def FEATURE_STATISTICS */

            freez(ncsp);

            /* are there any more in sequence after it? */
            if( !(ncsp = csp->next) )
               break;
         }
      }
   }

   for (fl = files; fl && (nfl = fl->next) ; fl = fl->next)
   {
      if ( ( 0 == nfl->active ) && ( NULL != nfl->unloader ) )
      {
         fl->next = nfl->next;

         (nfl->unloader)(nfl->f);

         freez(nfl->filename);

         freez(nfl);
      }
   }

}


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
 *                           fields filled, and all others zeroed.
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
       /* force reload of configfile and all the logs */
       if ( !MustReload ) return 0;
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
   *newfl = fs;
   return 1;
}


/*********************************************************************
 *
 * Function    :  simple_read_line
 *
 * Description :  Read a single line from a file and return it.
 *                This is basically a version of fgets() that malloc()s
 *                it's own line buffer.  Note that the buffer will
 *                always be a multiple of BUFFER_SIZE bytes long.
 *                Therefore if you are going to keep the string for
 *                an extended period of time, you should probably
 *                strdup() it and free() the original, to save memory.
 *
 *
 * Parameters  :
 *          1  :  dest = destination for newly malloc'd pointer to
 *                line data.  Will be set to NULL on error.
 *          2  :  fp = File to read from
 *          3  :  newline = Standard for newlines in the file.
 *                Will be unchanged if it's value on input is not
 *                NEWLINE_UNKNOWN.
 *                On output, may be changed from NEWLINE_UNKNOWN to
 *                actual convention in file.
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *                JB_ERR_FILE   on EOF.
 *
 *********************************************************************/
jb_err simple_read_line(FILE *fp, char **dest, int *newline)
{
   int len = 0;
   int buflen = BUFFER_SIZE;
   char * buf;
   char * p;
   int ch;
   int realnewline = NEWLINE_UNKNOWN;

   if (NULL == (buf = malloc(buflen)))
   {
      return JB_ERR_MEMORY;
   }

   p = buf;

/*
 * Character codes.  If you have a wierd compiler and the following are
 * incorrect, you also need to fix NEWLINE() in loaders.h
 */
#define CHAR_CR '\r' /* ASCII 13 */
#define CHAR_LF '\n' /* ASCII 10 */

   while (FOREVER)
   {
      ch = fgetc(fp);
      if (ch == EOF)
      {
         if (len > 0)
         {
            *p = '\0';
            *dest = buf;
            return JB_ERR_OK;
         }
         else
         {
            free(buf);
            *dest = NULL;
            return JB_ERR_FILE;
         }
      }
      else if (ch == CHAR_CR)
      {
         ch = getc(fp);
         if (ch == CHAR_LF)
         {
            if (*newline == NEWLINE_UNKNOWN)
            {
               *newline = NEWLINE_DOS;
            }
         }
         else
         {
            if (ch != EOF)
            {
               ungetc(ch, fp);
            }
            if (*newline == NEWLINE_UNKNOWN)
            {
               *newline = NEWLINE_MAC;
            }
         }
         *p = '\0';
         *dest = buf;
         if (*newline == NEWLINE_UNKNOWN)
         {
            *newline = realnewline;
         }
         return JB_ERR_OK;
      }
      else if (ch == CHAR_LF)
      {
         *p = '\0';
         *dest = buf;
         if (*newline == NEWLINE_UNKNOWN)
         {
            *newline = NEWLINE_UNIX;
         }
         return JB_ERR_OK;
      }
      else if (ch == 0)
      {
         *p = '\0';
         *dest = buf;
         return JB_ERR_OK;
      }

      *p++ = ch;

      if (++len >= buflen)
      {
         buflen += BUFFER_SIZE;
         if (NULL == (p = realloc(buf, buflen)));
         {
            free(buf);
            return JB_ERR_MEMORY;
         }
         buf = p;
         p = buf + len;
      }
   }
}


/*********************************************************************
 *
 * Function    :  edit_read_line
 *
 * Description :  Read a single non-empty line from a file and return
 *                it.  Trims comments, leading and trailing whitespace
 *                and respects escaping of newline and comment char.
 *                Provides the line in 2 alternative forms: raw and
 *                preprocessed.
 *                - raw is the raw data read from the file.  If the
 *                  line is not modified, then this should be written
 *                  to the new file.
 *                - prefix is any comments and blank lines that were
 *                  read from the file.  If the line is modified, then
 *                  this should be written out to the file followed
 *                  by the modified data.  (If this string is non-empty
 *                  then it will have a newline at the end).
 *                - data is the actual data that will be parsed
 *                  further by appropriate routines.
 *                On EOF, the 3 strings will all be set to NULL and
 *                0 will be returned.
 *
 * Parameters  :
 *          1  :  fp = File to read from
 *          2  :  raw_out = destination for newly malloc'd pointer to
 *                raw line data.  May be NULL if you don't want it.
 *          3  :  prefix_out = destination for newly malloc'd pointer to
 *                comments.  May be NULL if you don't want it.
 *          4  :  data_out = destination for newly malloc'd pointer to
 *                line data with comments and leading/trailing spaces
 *                removed, and line continuation performed.  May be
 *                NULL if you don't want it.
 *          5  :  newline = Standard for newlines in the file.
 *                On input, set to value to use or NEWLINE_UNKNOWN.
 *                On output, may be changed from NEWLINE_UNKNOWN to
 *                actual convention in file.  May be NULL if you
 *                don't want it.
 *          6  :  line_number = Line number in file.  In "lines" as
 *                reported by a text editor, not lines containing data.
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *                JB_ERR_FILE   on EOF.
 *
 *********************************************************************/
jb_err edit_read_line(FILE *fp,
                      char **raw_out,
                      char **prefix_out,
                      char **data_out,
                      int *newline,
                      unsigned long *line_number)
{
   char *p;          /* Temporary pointer   */
   char *linebuf;    /* Line read from file */
   char *linestart;  /* Start of linebuf, usually first non-whitespace char */
   int contflag = 0; /* Nonzero for line continuation - i.e. line ends '\' */
   int is_empty = 1; /* Flag if not got any data yet */
   char *raw    = NULL; /* String to be stored in raw_out    */
   char *prefix = NULL; /* String to be stored in prefix_out */
   char *data   = NULL; /* String to be stored in data_out   */
   int scrapnewline;    /* Used for (*newline) if newline==NULL */
   jb_err rval = JB_ERR_OK;

   assert(fp);
   assert(raw_out || data_out);
   assert(newline == NULL
       || *newline == NEWLINE_UNKNOWN
       || *newline == NEWLINE_UNIX
       || *newline == NEWLINE_DOS
       || *newline == NEWLINE_MAC);

   if (newline == NULL)
   {
      scrapnewline = NEWLINE_UNKNOWN;
      newline = &scrapnewline;
   }

   /* Set output parameters to NULL */
   if (raw_out)
   {
      *raw_out    = NULL;
   }
   if (prefix_out)
   {
      *prefix_out = NULL;
   }
   if (data_out)
   {
      *data_out   = NULL;
   }

   /* Set string variables to new, empty strings. */

   if (raw_out)
   {
      if ((raw = malloc(1)) == NULL)
      {
         return JB_ERR_MEMORY;
      }
      *raw = '\0';
   }
   if (prefix_out)
   {
      if ((prefix = malloc(1)) == NULL)
      {
         freez(raw);
         return JB_ERR_MEMORY;
      }
      *prefix = '\0';
   }
   if (data_out)
   {
      if ((data = malloc(1)) == NULL)
      {
         freez(raw);
         freez(prefix);
         return JB_ERR_MEMORY;
      }
      *data = '\0';
   }

   /* Main loop.  Loop while we need more data & it's not EOF. */

   while ( (contflag || is_empty)
        && (JB_ERR_OK == (rval = simple_read_line(fp, &linebuf, newline))))
   {
      if (line_number)
      {
         (*line_number)++;
      }
      if (raw)
      {
         string_append(&raw,linebuf);
         if (string_append(&raw,NEWLINE(*newline)))
         {
            freez(prefix);
            freez(data);
            free(linebuf);
            return JB_ERR_MEMORY;
         }
      }

      /* Line continuation? Trim escape and set flag. */
      p = linebuf + strlen(linebuf) - 1;
      contflag = ((*linebuf != '\0') && (*p == '\\'));
      if (contflag)
      {
         *p = '\0';
      }

      /* Trim leading spaces if we're at the start of the line */
      linestart = linebuf;
      if (*data == '\0')
      {
         /* Trim leading spaces */
         while (*linestart && isspace((int)(unsigned char)*linestart))
         {
            linestart++;
         }
      }

      /* Handle comment characters. */
      p = linestart;
      while ((p = strchr(p, '#')) != NULL)
      {
         /* Found a comment char.. */
         if ((p != linebuf) && (*(p-1) == '\\'))
         {
            /* ..and it's escaped, left-shift the line over the escape. */
            char *q = p - 1;
            while ((*q = *(q + 1)) != '\0')
            {
               q++;
            }
            /* Now scan from just after the "#". */
         }
         else
         {
            /* Real comment.  Save it... */
            if (p == linestart)
            {
               /* Special case:  Line only contains a comment, so all the
                * previous whitespace is considered part of the comment.
                * Undo the whitespace skipping, if any.
                */
               linestart = linebuf;
               p = linestart;
            }
            if (prefix)
            {
               string_append(&prefix,p);
               if (string_append(&prefix, NEWLINE(*newline)))
               {
                  freez(raw);
                  freez(data);
                  free(linebuf);
                  return JB_ERR_MEMORY;
               }
            }

            /* ... and chop off the rest of the line */
            *p = '\0';
         }
      } /* END while (there's a # character) */

      /* Write to the buffer */
      if (*linestart)
      {
         is_empty = 0;
         if (data)
         {
            if (string_append(&data, linestart))
            {
               freez(raw);
               freez(prefix);
               free(linebuf);
               return JB_ERR_MEMORY;
            }
         }
      }

      free(linebuf);
   } /* END while(we need more data) */

   /* Handle simple_read_line() errors - ignore EOF */
   if ((rval != JB_ERR_OK) && (rval != JB_ERR_FILE))
   {
      freez(raw);
      freez(prefix);
      freez(data);
      return rval;
   }

   if (raw ? (*raw == '\0') : is_empty)
   {
      /* EOF and no data there.  (Definition of "data" depends on whether
       * the caller cares about "raw" or just "data").
       */

      free(raw);
      free(prefix);
      free(data);

      return JB_ERR_FILE;
   }
   else
   {
      /* Got at least some data */

      /* Remove trailing whitespace */
      chomp(data);

      if (raw_out)
      {
         *raw_out    = raw;
      }
      else
      {
         free(raw);
      }
      if (prefix_out)
      {
         *prefix_out = prefix;
      }
      else
      {
         free(prefix);
      }
      if (data_out)
      {
         *data_out   = data;
      }
      else
      {
         free(data);
      }
      return JB_ERR_OK;
   }
}


/*********************************************************************
 *
 * Function    :  read_config_line
 *
 * Description :  Read a single non-empty line from a file and return
 *                it.  Trims comments, leading and trailing whitespace
 *                and respects escaping of newline and comment char.
 *
 * Parameters  :
 *          1  :  buf = Buffer to use.
 *          2  :  buflen = Size of buffer in bytes.
 *          3  :  fp = File to read from
 *	    4  :  linenum = linenumber in file
 *
 * Returns     :  NULL on EOF or error
 *                Otherwise, returns buf.
 *
 *********************************************************************/
char *read_config_line(char *buf, int buflen, FILE *fp, unsigned long *linenum)
{
   jb_err err;
   char *buf2 = NULL;
   err = edit_read_line(fp, NULL, NULL, &buf2, NULL, linenum);
   if (err)
   {
      if (err == JB_ERR_MEMORY)
      {
         log_error(LOG_LEVEL_FATAL, "Out of memory loading a config file");
      }
      return NULL;
   }
   else
   {
      assert(buf2);
      assert(strlen(buf2) + 1U < (unsigned)buflen);
      strncpy(buf, buf2, buflen - 1);
      free(buf2);
      buf[buflen - 1] = '\0';
      return buf;
   }
}


#ifdef FEATURE_TRUST
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

   unload_trustfile(b->next); /* Stack is cheap, isn't it? */

   free_url_spec(b->url);

   freez(b);

}


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

   char  buf[BUFFER_SIZE], *p, *q;
   int reject, trusted;
   struct file_list *fs;
   unsigned long linenum = 0;

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

   while (read_config_line(buf, sizeof(buf), fp, &linenum) != NULL)
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
         /* FIXME BUFFER OVERFLOW if >=64 entries */
      }
   }

   *tl = NULL;

   fclose(fp);

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
#endif /* def FEATURE_TRUST */


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
   struct re_filterfile_spec *b = (struct re_filterfile_spec *)f;

   if (b == NULL)
   {
      return;
   }

   destroy_list(b->patterns);
   pcrs_free_joblist(b->joblist);
   freez(b);

   return;
}

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

   char  buf[BUFFER_SIZE];
   int error;
   unsigned long linenum = 0;
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
   while (read_config_line(buf, sizeof(buf), fp, &linenum) != NULL)
   {
      enlist( bl->patterns, buf );

      /* We have a meaningful line -> make it a job */
      if ((dummy = pcrs_compile_command(buf, &error)) == NULL)
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
