const char errlog_rcs[] = "$Id: errlog.c,v 1.10 2001/05/29 11:52:21 oes Exp $";
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
 *    Revision 1.10  2001/05/29 11:52:21  oes
 *    Conditional compilation of w32_socket_error
 *
 *    Revision 1.9  2001/05/28 16:15:17  jongfoster
 *    Improved reporting of errors under Win32.
 *
 *    Revision 1.8  2001/05/26 17:25:14  jongfoster
 *    Added support for CLF (Common Log Format) and fixed LOG_LEVEL_LOG
 *
 *    Revision 1.7  2001/05/26 15:21:28  jongfoster
 *    Activity animation in Win32 GUI now works even if debug==0
 *
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
 * LOG_LEVEL_FATAL cannot be turned off.  (There are
 * some exceptional situations where we need to get a
 * message to the user).
 */
#define LOG_LEVEL_MINIMUM  LOG_LEVEL_FATAL

/* where to log (default: stderr) */
static FILE *logfp = NULL;

/* where to log (NULL == stderr) */
static char * logfilename = NULL;

/* logging detail level.  */
static int debug = (LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO);  

/* static functions */
static void fatal_error(const char * error_message);
static char * w32_socket_strerr(int errcode, char * tmp_buf);


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
   if ((loglevel & debug) == 0)
   {
      return;
   }

   /* FIXME get current thread id */
   /* this_thread = (long)pthread_self(); */

   switch (loglevel)
   {
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
      case LOG_LEVEL_LOG:
         outc = sprintf(outbuf, "IJB(%d) Writing: ", this_thread);
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
      case LOG_LEVEL_CLF:
         outc = 0;
         outbuf[0] = '\0';
         break;
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
         case 'N':
            /* Non-standard: Print a counted string.  Takes 2 parameters:
             * int length, const char * string
             */
            ival = va_arg( ap, int );
            sval = va_arg( ap, char * );
            if (ival < 0)
            {
               ival = 0;
            }
            oldoutc = outc;
            outc += ival;
            if (outc < BUFSIZ-1)
            {
               memcpy(outbuf + oldoutc, sval, ival);
            }
            else
            {
               outbuf[oldoutc] = '\0';
            }
            break;
         case 'E':
            /* Non-standard: Print error code from errno */
#ifdef _WIN32
            ival = WSAGetLastError();
            sval = w32_socket_strerr(ival, tempbuf);
#else /* ifndef _WIN32 */
            ival = errno; 
#ifdef HAVE_STRERROR
            sval = strerror(ival);
#else /* ifndef HAVE_STRERROR */
            sval = NULL;
#endif /* ndef HAVE_STRERROR */
            if (sval == NULL)
            {
               sprintf(tempbuf, "(errno = %d)", ival);
               sval = tempbuf;
            }
#endif /* ndef _WIN32 */
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
         case 'T':
            /* Non-standard: Print a Common Log File timestamp */
            {
               /*
                * Write timestamp into tempbuf.
                *
                * Complex because not all OSs have tm_gmtoff or
                * the %z field in strftime()
                */
               time_t now; 
               struct tm *tm_now; 
               struct tm gmt; 
               int days, hrs, mins; 
               time (&now); 
               gmt = *gmtime (&now); 
               tm_now = localtime (&now); 
               days = tm_now->tm_yday - gmt.tm_yday; 
               hrs = ((days < -1 ? 24 : 1 < days ? -24 : days * 24) + tm_now->tm_hour - gmt.tm_hour); 
               mins = hrs * 60 + tm_now->tm_min - gmt.tm_min; 
               strftime (tempbuf, BUFSIZ-6, "%d/%b/%Y:%H:%M:%S ", tm_now); 
               sprintf (tempbuf + strlen(tempbuf), "%+03d%02d", mins / 60, abs(mins) % 60); 
            }
            oldoutc = outc;
            outc += strlen(tempbuf);
            if (outc < BUFSIZ-1) 
            {
               strcpy(outbuf + oldoutc, tempbuf);
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


#ifdef _WIN32
/*********************************************************************
 *
 * Function    :  w32_socket_strerr
 *
 * Description :  Translate the return value from WSAGetLastError()
 *                into a string.
 *
 * Parameters  :
 *          1  :  errcode = The return value from WSAGetLastError().
 *          2  :  tmp_buf = A temporary buffer that might be used to
 *                          store the string.
 *
 * Returns     :  String representing the error code.  This may be
 *                a global string constant or a string stored in
 *                tmp_buf.
 *
 *********************************************************************/
static char * w32_socket_strerr(int errcode, char * tmp_buf)
{
#define TEXT_FOR_ERROR(code,text) \
   if (errcode == code)           \
   {                              \
      return #code " - " text;    \
   }

   TEXT_FOR_ERROR(WSAEACCES, "Permission denied")
   TEXT_FOR_ERROR(WSAEADDRINUSE, "Address already in use.")
   TEXT_FOR_ERROR(WSAEADDRNOTAVAIL, "Cannot assign requested address.");
   TEXT_FOR_ERROR(WSAEAFNOSUPPORT, "Address family not supported by protocol family.");
   TEXT_FOR_ERROR(WSAEALREADY, "Operation already in progress.");
   TEXT_FOR_ERROR(WSAECONNABORTED, "Software caused connection abort.");
   TEXT_FOR_ERROR(WSAECONNREFUSED, "Connection refused.");
   TEXT_FOR_ERROR(WSAECONNRESET, "Connection reset by peer.");
   TEXT_FOR_ERROR(WSAEDESTADDRREQ, "Destination address required.");
   TEXT_FOR_ERROR(WSAEFAULT, "Bad address.");
   TEXT_FOR_ERROR(WSAEHOSTDOWN, "Host is down.");
   TEXT_FOR_ERROR(WSAEHOSTUNREACH, "No route to host.");
   TEXT_FOR_ERROR(WSAEINPROGRESS, "Operation now in progress.");
   TEXT_FOR_ERROR(WSAEINTR, "Interrupted function call.");
   TEXT_FOR_ERROR(WSAEINVAL, "Invalid argument.");
   TEXT_FOR_ERROR(WSAEISCONN, "Socket is already connected.");
   TEXT_FOR_ERROR(WSAEMFILE, "Too many open sockets.");
   TEXT_FOR_ERROR(WSAEMSGSIZE, "Message too long.");
   TEXT_FOR_ERROR(WSAENETDOWN, "Network is down.");
   TEXT_FOR_ERROR(WSAENETRESET, "Network dropped connection on reset.");
   TEXT_FOR_ERROR(WSAENETUNREACH, "Network is unreachable.");
   TEXT_FOR_ERROR(WSAENOBUFS, "No buffer space available.");
   TEXT_FOR_ERROR(WSAENOPROTOOPT, "Bad protocol option.");
   TEXT_FOR_ERROR(WSAENOTCONN, "Socket is not connected.");
   TEXT_FOR_ERROR(WSAENOTSOCK, "Socket operation on non-socket.");
   TEXT_FOR_ERROR(WSAEOPNOTSUPP, "Operation not supported.");
   TEXT_FOR_ERROR(WSAEPFNOSUPPORT, "Protocol family not supported.");
   TEXT_FOR_ERROR(WSAEPROCLIM, "Too many processes.");
   TEXT_FOR_ERROR(WSAEPROTONOSUPPORT, "Protocol not supported.");
   TEXT_FOR_ERROR(WSAEPROTOTYPE, "Protocol wrong type for socket.");
   TEXT_FOR_ERROR(WSAESHUTDOWN, "Cannot send after socket shutdown.");
   TEXT_FOR_ERROR(WSAESOCKTNOSUPPORT, "Socket type not supported.");
   TEXT_FOR_ERROR(WSAETIMEDOUT, "Connection timed out.");
   TEXT_FOR_ERROR(WSAEWOULDBLOCK, "Resource temporarily unavailable.");
   TEXT_FOR_ERROR(WSAHOST_NOT_FOUND, "Host not found.");
   TEXT_FOR_ERROR(WSANOTINITIALISED, "Successful WSAStartup not yet performed.");
   TEXT_FOR_ERROR(WSANO_DATA, "Valid name, no data record of requested type.");
   TEXT_FOR_ERROR(WSANO_RECOVERY, "This is a non-recoverable error.");
   TEXT_FOR_ERROR(WSASYSNOTREADY, "Network subsystem is unavailable.");
   TEXT_FOR_ERROR(WSATRY_AGAIN, "Non-authoritative host not found.");
   TEXT_FOR_ERROR(WSAVERNOTSUPPORTED, "WINSOCK.DLL version out of range.");
   TEXT_FOR_ERROR(WSAEDISCON, "Graceful shutdown in progress.");
   /*
    * The following error codes are documented in the Microsoft WinSock
    * reference guide, but don't actually exist.
    *
    * TEXT_FOR_ERROR(WSA_INVALID_HANDLE, "Specified event object handle is invalid.");
    * TEXT_FOR_ERROR(WSA_INVALID_PARAMETER, "One or more parameters are invalid.");
    * TEXT_FOR_ERROR(WSAINVALIDPROCTABLE, "Invalid procedure table from service provider.");
    * TEXT_FOR_ERROR(WSAINVALIDPROVIDER, "Invalid service provider version number.");
    * TEXT_FOR_ERROR(WSA_IO_PENDING, "Overlapped operations will complete later.");
    * TEXT_FOR_ERROR(WSA_IO_INCOMPLETE, "Overlapped I/O event object not in signaled state.");
    * TEXT_FOR_ERROR(WSA_NOT_ENOUGH_MEMORY, "Insufficient memory available.");
    * TEXT_FOR_ERROR(WSAPROVIDERFAILEDINIT, "Unable to initialize a service provider.");
    * TEXT_FOR_ERROR(WSASYSCALLFAILURE, "System call failure.");
    * TEXT_FOR_ERROR(WSA_OPERATION_ABORTED, "Overlapped operation aborted.");
    */

   sprintf(tmp_buf, "(error number %d)", errcode);
   return tmp_buf;
}
#endif /* def _WIN32 */


/*
  Local Variables:
  tab-width: 3
  end:
*/

