const char errlog_rcs[] = "$Id: errlog.c,v 1.6 2001/05/25 21:55:08 jongfoster Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/errlog.c,v $
 *
 * Purpose     :  Log errors to a designated destination in an elegant,
 *                printf-like fashion.
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
 *    $Log: errlog.c,v $
 *    Revision 1.6  2001/05/25 21:55:08  jongfoster
 *    Now cleans up properly on FATAL (removes taskbar icon etc)
 *
 *    Revision 1.5  2001/05/22 18:46:04  oes
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
 *    Revision 1.4  2001/05/21 19:32:54  jongfoster
 *    Added another #ifdef _WIN_CONSOLE
 *
 *    Revision 1.3  2001/05/20 01:11:40  jongfoster
 *    Added support for LOG_LEVEL_FATAL
 *    Renamed LOG_LEVEL_FRC to LOG_LEVEL_FORCE,
 *    and LOG_LEVEL_REF to LOG_LEVEL_RE_FILTER
 *
 *    Revision 1.2  2001/05/17 22:42:01  oes
 *     - Cleaned CRLF's from the sources and related files
 *     - Repaired logging for REF and FRC
 *
 *    Revision 1.1.1.1  2001/05/15 13:58:51  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#endif /* ndef _WIN32 */

#include <errno.h>
/* #include <pthread.h> */

#ifdef _WIN32
#include <windows.h>
#ifndef _WIN_CONSOLE
#include "w32log.h"
#endif /* ndef _WIN_CONSOLE */
#endif /* def _WIN32 */

#include "errlog.h"
#include "project.h"

const char errlog_h_rcs[] = ERRLOG_H_VERSION;


/*
 * LOG_LEVEL_FATAL, LOG_LEVEL_ERROR and LOG_LEVEL_INFO
 * cannot be turned off.  (There are some exceptional situations
 * where we need to get a message to the user).
 *
 * FIXME: Do we need LOG_LEVEL_INFO here?
 */
#define LOG_LEVEL_MINIMUM  (LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO)

/* where to log (default: stderr) */
static FILE *logfp = NULL;

/* where to log (NULL == stderr) */
static char * logfilename = NULL;

/* logging detail level.  */
static int debug = LOG_LEVEL_MINIMUM;  

static void fatal_error(const char * error_message);


/*********************************************************************
 *
 * Function    :  fatal_error
 *
 * Description :  Displays a fatal error to standard error (or, on 
 *                a WIN32 GUI, to a dialog box), and exits
 *                JunkBuster with status code 1.
 *
 * Parameters  :
 *          1  :  error_message = The error message to display.
 *
 * Returns     :  Does not return.
 *
 *********************************************************************/
static void fatal_error(const char * error_message)
{
#if defined(_WIN32) && !defined(_WIN_CONSOLE)
   MessageBox(g_hwndLogFrame, error_message, "Internet JunkBuster Error", 
      MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST);  

   /* Cleanup - remove taskbar icon etc. */
   TermLogWindow();

#else /* if !defined(_WIN32) || defined(_WIN_CONSOLE) */
   fputs(error_message, stderr);
#endif /* defined(_WIN32) && !defined(_WIN_CONSOLE) */

   exit(1);
}


/*********************************************************************
 *
 * Function    :  init_errlog
 *
 * Description :  Initializes the logging module.  Must call before
 *                calling log_error.
 *
 * Parameters  :
 *          1  :  prog_name  = The program name.
 *          2  :  logfname   = The logfile name, or NULL for stderr.
 *          3  :  debuglevel = The debugging level.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void init_error_log(const char *prog_name, const char *logfname, int debuglevel)
{
   FILE *fp;

   /* FIXME RACE HAZARD: should start critical section error_log_use here */

   /* set the logging detail level */
   debug = debuglevel | LOG_LEVEL_MINIMUM;

   if ((logfp != NULL) && (logfp != stderr))
   {
      fclose(logfp);
   }
   logfp = stderr;

   /* set the designated log file */
   if( logfname )
   {
      if( !(fp = fopen(logfname, "a")) )
      {
         log_error(LOG_LEVEL_FATAL, "init_errlog(): can't open logfile: %s", logfname);
      }

      /* set logging to be completely unbuffered */
      setbuf(fp, NULL);

      logfp = fp;
   }

   log_error(LOG_LEVEL_INFO, "Internet JunkBuster version " VERSION);
   if (prog_name != NULL)
   {
      log_error(LOG_LEVEL_INFO, "Program name: %s", prog_name);
   }

   /* FIXME RACE HAZARD: should end critical section error_log_use here */

} /* init_error_log */


/*********************************************************************
 *
 * Function    :  log_error
 *
 * Description :  This is the error-reporting and logging function.
 *
 * Parameters  :
 *          1  :  loglevel  = the type of message to be logged
 *          2  :  fmt       = the main string we want logged, printf-like
 *          3  :  ...       = arguments to be inserted in fmt (printf-like).
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void log_error(int loglevel, char *fmt, ...)
{
   va_list ap;
   char outbuf[BUFSIZ];
   char * src = fmt;
   int outc = 0;
   long this_thread = 1;  /* was: pthread_t this_thread;*/

#if defined(_WIN32) && !defined(_WIN_CONSOLE)
   /*
    * Irrespective of debug setting, a GET/POST/CONNECT makes
    * the taskbar icon animate.  (There is an option to disable
    * this but checking that is handled inside LogShowActivity()).
    */
   if (loglevel == LOG_LEVEL_GPC)
   {
      LogShowActivity();
   }
#endif /* defined(_WIN32) && !defined(_WIN_CONSOLE) */

   /* verify if loglevel applies to current settings and bail out if negative */
   if(!(loglevel & debug))
   {
      return;
   }

   /* FIXME get current thread id */
   /* this_thread = (long)pthread_self(); */

   switch (loglevel)
   {
      /* FIXME: What about LOG_LEVEL_LOG ??? */
      case LOG_LEVEL_ERROR:
         outc = sprintf(outbuf, "IJB(%d) Error: ", this_thread);
         break;
      case LOG_LEVEL_FATAL:
         outc = sprintf(outbuf, "IJB(%d) Fatal error: ", this_thread);
         break;
      case LOG_LEVEL_GPC:
         outc = sprintf(outbuf, "IJB(%d) Request: ", this_thread);
         break;
      case LOG_LEVEL_CONNECT:
         outc = sprintf(outbuf, "IJB(%d) Connect: ", this_thread);
         break;
      case LOG_LEVEL_HEADER:
         outc = sprintf(outbuf, "IJB(%d) Header: ", this_thread);
         break;
      case LOG_LEVEL_INFO:
         outc = sprintf(outbuf, "IJB(%d) Info: ", this_thread);
         break;
#ifdef PCRS
      case LOG_LEVEL_RE_FILTER:
         outc = sprintf(outbuf, "IJB(%d) Re-Filter: ", this_thread);
         break;
#endif /* def PCRS */
#ifdef FORCE_LOAD
      case LOG_LEVEL_FORCE:
         outc = sprintf(outbuf, "IJB(%d) Force: ", this_thread);
         break;
#endif /* def FORCE_LOAD */
#ifdef FAST_REDIRECTS
      case LOG_LEVEL_REDIRECTS:
         outc = sprintf(outbuf, "IJB(%d) Redirect: ", this_thread);
         break;
#endif /* def FAST_REDIRECTS */
      default:
         outc = sprintf(outbuf, "IJB(%d) UNKNOWN LOG TYPE(%d): ", this_thread, loglevel);
         break;
   }
   
   /* get ready to scan var. args. */
   va_start( ap, fmt );

   /* build formatted message from fmt and var-args */
   while ((*src) && (outc < BUFSIZ-2))
   {
      char tempbuf[BUFSIZ];
      char *sval;
      int ival;
      unsigned uval;
      long lval;
      unsigned long ulval;
      int oldoutc;
      char ch;
      
      ch = *src++;
      if( ch != '%' )
      {
         outbuf[outc++] = ch;
         continue;
      }

      ch = *src++;
      switch (ch) {
         case '%':
            outbuf[outc++] = '%';
            break;
         case 'd':
            ival = va_arg( ap, int );
            oldoutc = outc;
            outc += sprintf(tempbuf, "%d", ival);
            if (outc < BUFSIZ-1) 
            {
               strcpy(outbuf + oldoutc, tempbuf);
            }
            else
            {
               outbuf[oldoutc] = '\0';
            }
            break;
         case 'u':
            uval = va_arg( ap, unsigned );
            oldoutc = outc;
            outc += sprintf(tempbuf, "%u", uval);
            if (outc < BUFSIZ-1) 
            {
               strcpy(outbuf + oldoutc, tempbuf);
            }
            else
            {
               outbuf[oldoutc] = '\0';
            }
            break;
         case 'l':
            /* this is a modifier that must be followed by u or d */
            ch = *src++;
            if (ch == 'd')
            {
               lval = va_arg( ap, long );
               oldoutc = outc;
               outc += sprintf(tempbuf, "%ld", lval);
            }
            else if (ch == 'u')
            {
               ulval = va_arg( ap, unsigned long );
               oldoutc = outc;
               outc += sprintf(tempbuf, "%lu", ulval);
            }
            else
            {
               /* Error */
               sprintf(outbuf, "IJB(%d) Error: log_error(): Bad format string:\n"
                               "Format = \"%s\"\n"
                               "Exiting.", this_thread, fmt);
               /* FIXME RACE HAZARD: should start critical section error_log_use here */
               if( !logfp )
               {
                  logfp = stderr;
               }
               fputs(outbuf, logfp);
               /* FIXME RACE HAZARD: should end critical section error_log_use here */
               fatal_error(outbuf);
               /* Never get here */
               break;
            }
            if (outc < BUFSIZ-1) 
            {
               strcpy(outbuf + oldoutc, tempbuf);
            }
            else
            {
               outbuf[oldoutc] = '\0';
            }
            break;
         case 'c':
            /*
             * Note that char paramaters are converted to int, so we need to
             * pass "int" to va_arg.  (See K&R, 2nd ed, section A7.3.2, page 202)
             */
            outbuf[outc++] = (char) va_arg( ap, int );
            break;
         case 's':
            sval = va_arg( ap, char * );
            oldoutc = outc;
            outc += strlen(sval);
            if (outc < BUFSIZ-1) 
            {
               strcpy(outbuf + oldoutc, sval);
            }
            else
            {
               outbuf[oldoutc] = '\0';
            }
            break;
         case 'E':
            /* Non-standard: Print error code from errno */
            ival = errno;
#ifndef NOSTRERROR
            sval = strerror(ival);
#else /* def NOSTRERROR */
            sval = NULL
#endif /* def NOSTRERROR */
            if (sval == NULL)
            {
               sprintf(tempbuf, "(errno = %d)", ival);
               sval = tempbuf;
            }
            oldoutc = outc;
            outc += strlen(sval);
            if (outc < BUFSIZ-1) 
            {
               strcpy(outbuf + oldoutc, sval);
            }
            else
            {
               outbuf[oldoutc] = '\0';
            }
            break;
         default:
            sprintf(outbuf, "IJB(%d) Error: log_error(): Bad format string:\n"
                            "Format = \"%s\"\n"
                            "Exiting.", this_thread, fmt);
            /* FIXME RACE HAZARD: should start critical section error_log_use here */
            if( !logfp )
            {
               logfp = stderr;
            }
            fputs(outbuf, logfp);
            /* FIXME RACE HAZARD: should end critical section error_log_use here */
            fatal_error(outbuf);
            /* Never get here */
            break;

      } /* switch( p ) */

   } /* for( p ... ) */
   
   /* done with var. args */
   va_end( ap );
   
   if (outc >= BUFSIZ-2)
   {
      /* insufficient room for newline and trailing null. */

      static const char warning[] = "... [too long, truncated]\n";

      if (outc < BUFSIZ)
      {
         /* Need to add terminating null in this case. */
         outbuf[outc] = '\0';
      }

      /* Truncate output */
      outbuf[BUFSIZ - sizeof(warning)] = '\0';

      /* Append warning */
      strcat(outbuf, warning);
   }
   else
   {
      /* Add terminating newline and null */
      outbuf[outc++] = '\n';
      outbuf[outc] = '\0';
   }

   /* FIXME RACE HAZARD: should start critical section error_log_use here */

   /* deal with glibc stupidity - it won't let you initialize logfp */
   if( !logfp )
   {
      logfp = stderr;
   }

   fputs(outbuf, logfp);

   if (loglevel == LOG_LEVEL_FATAL)
   {
      fatal_error(outbuf);
      /* Never get here */
   }

   /* FIXME RACE HAZARD: should end critical section error_log_use here */

#if defined(_WIN32) && !defined(_WIN_CONSOLE)
   /* Write to display */
   LogPutString(outbuf);
#endif /* defined(_WIN32) && !defined(_WIN_CONSOLE) */

}


/*
  Local Variables:
  tab-width: 3
  end:
*/

