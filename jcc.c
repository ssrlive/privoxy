const char jcc_rcs[] = "$Id: jcc.c,v 1.6 2001/05/23 00:13:58 joergs Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/jcc.c,v $
 *
 * Purpose     :  Main file.  Contains main() method, main loop, and 
 *                the main connection-handling function.
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
 *    $Log: jcc.c,v $
 *    Revision 1.6  2001/05/23 00:13:58  joergs
 *    AmigaOS support fixed.
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
 *    Revision 1.4  2001/05/21 19:34:01  jongfoster
 *    Made failure to bind() a fatal error.
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
 *    Revision 1.2  2001/05/17 22:34:44  oes
 *     - Added hint on GIF char array generation to jcc.c
 *     - Cleaned CRLF's from the sources and related files
 *     - Repaired logging for REF and FRC
 *
 *    Revision 1.1.1.1  2001/05/15 13:58:56  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32

# include <sys/timeb.h>
# include <windows.h>
# include <io.h>
# include <process.h>
# ifdef TOGGLE
#  include <time.h>
# endif /* def TOGGLE */

# include "win32.h"
# ifndef _WIN_CONSOLE
#  include "w32log.h"
# endif /* ndef _WIN_CONSOLE */

#else /* ifndef _WIN32 */

# include <unistd.h>
# include <sys/time.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <signal.h>

# ifdef __BEOS__
#  include <socket.h>  /* BeOS has select() for sockets only. */
#  include <OS.h>      /* declarations for threads and stuff. */
# endif

# ifndef FD_ZERO
#  include <select.h>
# endif

#endif

#include "project.h"
#include "jcc.h"
#include "filters.h"
#include "loaders.h"
#include "showargs.h"
#include "parsers.h"
#include "killpopup.h"
#include "miscutil.h"
#include "errlog.h"
#include "jbsockets.h"
#include "gateway.h"

const char jcc_h_rcs[] = JCC_H_VERSION;
const char project_h_rcs[] = PROJECT_H_VERSION;

const char DEFAULT_USER_AGENT[] ="User-Agent: Mozilla (X11; I; Linux 2.0.32 i586)";

struct client_state  clients[1];
struct file_list     files[1];

#ifdef STATISTICS
int urls_read     = 0;     /* total nr of urls read inc rejected */
int urls_rejected = 0;     /* total nr of urls rejected */
#endif /* def STATISTICS */


static void listen_loop(void);
static void chat(struct client_state *csp);
#ifdef AMIGA
void serve(struct client_state *csp);
#else /* ifndef AMIGA */
static void serve(struct client_state *csp);
#endif /* def AMIGA */

#ifdef __BEOS__
static int32 server_thread(void *data);
#endif /* def __BEOS__ */

#ifdef _WIN32
#define sleep(N)  Sleep(((N) * 1000))
#endif


/*********************************************************************
 *
 * Function    :  chat
 *
 * Description :  Once a connection to the client has been accepted,
 *                this function is called (via serve()) to handle the
 *                main business of the communication.  When this 
 *                function returns, the caller must close the client
 *                socket handle.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  On success, the number of bytes written are returned (zero
 *                indicates nothing was written).  On error, -1 is returned,
 *                and errno is set appropriately.  If count is zero and the
 *                file descriptor refers to a regular file, 0 will be
 *                returned without causing any other effect.  For a special
 *                file, the results are not portable.
 *
 *********************************************************************/
static void chat(struct client_state *csp)
{
/* This next line is a little ugly, but it simplifies the if statement below. */
/* Basically if TOGGLE, then we want the if to test "csp->toggled_on", else we don't */
#ifdef TOGGLE
#   define IS_TOGGLED_ON csp->toggled_on &&
#else /* ifndef TOGGLE */
#   define IS_TOGGLED_ON
#endif /* ndef TOGGLE */

/* This next line is a little ugly, but it simplifies the if statement below. */
/* Basically if TRUST_FILES, then we want the if to call "trust_url", else we don't */
#ifdef TRUST_FILES
#   define IS_TRUSTED_URL (p = trust_url(http, csp)) ||
#else /* ifndef TRUST_FILES */
#   define IS_TRUSTED_URL
#endif /* ndef TRUST_FILES */

   char buf[BUFSIZ], *hdr, *p, *req;
   char *err = NULL;
   char *eno;
   fd_set rfds;
   int n, maxfd, server_body, ms_iis5_hack = 0;
   const struct gateway *gw;
   struct http_request *http;
#ifdef KILLPOPUPS
   int block_popups;         /* bool, 1==will block popups */
   int block_popups_now = 0; /* bool, 1==currently blocking popups */
#endif /* def KILLPOPUPS */
#ifdef PCRS
   int pcrs_filter;   /* bool, 1==will filter through pcrs */
   int filtering = 0; /* bool, 1==currently filtering through pcrs */
#endif /* def PCRS */

   http = csp->http;

   /*
    * Read the client's request.  Note that since we're not using select() we
    * could get blocked here if a client connected, then didn't say anything!
    */

   while (FOREVER)
   {
      n = read_socket(csp->cfd, buf, sizeof(buf));

      if (n <= 0) break;      /* error! */

      add_to_iob(csp, buf, n);

      req = get_header(csp);

      if (req == NULL)
      {
         break;    /* no HTTP request! */
      }

      if (*req == '\0')
      {
         continue;   /* more to come! */
      }
 
#ifdef FORCE_LOAD
      /* If this request contains the FORCE_PREFIX,
       * better get rid of it now and set the force flag --oes
       */

      if (strstr(req, FORCE_PREFIX))
      {
         strclean(req, FORCE_PREFIX);
         log_error(LOG_LEVEL_FORCE, "Enforcing request \"%s\".\n", req);
         csp->force = 1;
      } 
      else
      {
         csp->force = 0;
      }
#endif /* def FORCE_LOAD */
  
      parse_http_request(req, http, csp);
      freez(req);
      break;
   }

   if (http->cmd == NULL)
   {
      strcpy(buf, CHEADER);
      write_socket(csp->cfd, buf, strlen(buf));
      return;
   }

   /* decide how to route the HTTP request */

   if ((gw = forward_url(http, csp)) == NULL)
   {
      log_error(LOG_LEVEL_FATAL, "gateway spec is NULL!?!?  This can't happen!");
      /* Never get here - LOG_LEVEL_FATAL causes program exit */
   }

   /* build the http request to send to the server
    * we have to do one of the following:
    *
    * create = use the original HTTP request to create a new
    *          HTTP request that has only the path component
    *          without the http://domainspec
    * pass   = pass the original HTTP request unchanged
    *
    * drop   = drop the HTTP request
    *
    * here's the matrix:
    *                        SSL
    *                    0        1
    *                +--------+--------+
    *                |        |        |
    *             0  | create | drop   |
    *                |        |        |
    *  Forwarding    +--------+--------+
    *                |        |        |
    *             1  | pass   | pass   |
    *                |        |        |
    *                +--------+--------+
    *
    */

   if (gw->forward_host)
   {
      /* if forwarding, just pass the request as is */
      enlist(csp->headers, http->cmd);
   }
   else
   {
      if (http->ssl == 0)
      {
         /* otherwise elide the host information from the url */
         p = NULL;
         p = strsav(p, http->gpc);
         p = strsav(p, " ");
         p = strsav(p, http->path);
         p = strsav(p, " ");
         p = strsav(p, http->ver);
         enlist(csp->headers, p);
         freez(p);
      }
   }

   /* decide what we're to do with cookies */

#ifdef TOGGLE
   if (!csp->toggled_on)
   {
      /* Most compatible set of permissions */
      csp->permissions = PERMIT_COOKIE_SET | PERMIT_COOKIE_READ | PERMIT_POPUPS;
   }
   else
   {
      csp->permissions = url_permissions(http, csp);
   }
#else /* ifndef TOGGLE */
   csp->permissions = url_permissions(http, csp);
#endif /* ndef TOGGLE */

#ifdef KILLPOPUPS
   block_popups               = ((csp->permissions & PERMIT_POPUPS) == 0);
#endif /* def KILLPOPUPS */
#ifdef PCRS
   pcrs_filter                = (csp->rlist != NULL) &&  /* There are expressions to be used */
                                ((csp->permissions & PERMIT_RE_FILTER) != 0);
#endif /* def PCRS */


   /* grab the rest of the client's headers */

   while (FOREVER)
   {
      if ( ( p = get_header(csp) ) && ( *p == '\0' ) )
      {
         n = read_socket(csp->cfd, buf, sizeof(buf));
         if (n <= 0)
         {
            log_error(LOG_LEVEL_ERROR, "read from client failed: %E");
            return;
         }
         add_to_iob(csp, buf, n);
         continue;
      }

      if (p == NULL) break;

      enlist(csp->headers, p);
      freez(p);
   }

   /* filter it as required */

   hdr = sed(client_patterns, add_client_headers, csp);

   destroy_list(csp->headers);

   /* Check the request against all rules, unless
    * we're toggled off or in force mode. 
    */
 
   if (IS_TOGGLED_ON
#ifdef FORCE_LOAD
       (!csp->force) && 
#endif /* def FORCE_LOAD */
       ( (p = intercept_url(http, csp)) ||
         IS_TRUSTED_URL
         (p = block_url(http, csp))
#ifdef FAST_REDIRECTS
         || (fast_redirects && (p = redirect_url(http, csp))) 
#endif /* def FAST_REDIRECTS */
      ))
   {
#ifdef STATISTICS
      csp->rejected = 1;
#endif /* def STATISTICS */

      log_error(LOG_LEVEL_GPC, "%s%s crunch!", http->hostport, http->path);

#if defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST)
      /* Block as image?  */
      if ( (tinygif > 0) && block_imageurl(http, csp) )
      {
         /* Send "blocked" image */
         log_error(LOG_LEVEL_GPC, "%s%s image crunch!",
                   http->hostport, http->path);

         if ((tinygif == 2) || strstr(http->path, "ijb-send-banner"))
         {
            write_socket(csp->cfd, JBGIF, sizeof(JBGIF)-1);
         }
         if (tinygif == 1)
         {
            write_socket(csp->cfd, BLANKGIF, sizeof(BLANKGIF)-1);
         }
         else if ((tinygif == 3) && (tinygifurl))
         {
            p = (char *)malloc(strlen(HTTP_REDIRECT_TEMPLATE) + strlen(tinygifurl));
            sprintf(p, HTTP_REDIRECT_TEMPLATE, tinygifurl);
            write_socket(csp->cfd, p, strlen(p));
         }
         else
         {
            write_socket(csp->cfd, JBGIF, sizeof(JBGIF)-1);
         }
      }
      else
#endif /* defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST) */
      /* Block as HTML */
      {
         /* Send HTML "blocked" message, interception, or redirection result */
         write_socket(csp->cfd, p, strlen(p));
      }

      log_error(LOG_LEVEL_LOG, "%s", p);

      freez(p);
      freez(hdr);
      return;
   }

   log_error(LOG_LEVEL_GPC, "%s%s", http->hostport, http->path);

   if (gw->forward_host)
   {
      log_error(LOG_LEVEL_CONNECT, "via %s:%d to: %s",
               gw->forward_host, gw->forward_port, http->hostport);
   }
   else
   {
      log_error(LOG_LEVEL_CONNECT, "to %s", http->hostport);
   }

   /* here we connect to the server, gateway, or the forwarder */

   csp->sfd = (gw->conn)(gw, http, csp);

   if (csp->sfd < 0)
   {
      log_error(LOG_LEVEL_CONNECT, "connect to: %s failed: %E",
                http->hostport);

      if (errno == EINVAL)
      {
         err = zalloc(strlen(CNXDOM) + strlen(http->host));
         sprintf(err, CNXDOM, http->host);
      }
      else
      {
         eno = safe_strerror(errno);
         err = zalloc(strlen(CFAIL) + strlen(http->hostport) + strlen(eno));
         sprintf(err, CFAIL, http->hostport, eno);
      }

      write_socket(csp->cfd, err, strlen(err));

      log_error(LOG_LEVEL_LOG, err);

      freez(err);
      freez(hdr);
      return;
   }

   log_error(LOG_LEVEL_CONNECT, "OK");

   if (gw->forward_host || (http->ssl == 0))
   {
      /* write the client's (modified) header to the server
       * (along with anything else that may be in the buffer)
       */

      n = strlen(hdr);

      if ((write_socket(csp->sfd, hdr, n) != n)
          || (flush_socket(csp->sfd, csp   ) <  0))
      {
         log_error(LOG_LEVEL_CONNECT, "write header to: %s failed: %E",
                    http->hostport);

         eno = safe_strerror(errno);
         err = zalloc(strlen(CFAIL) + strlen(http->hostport) + strlen(eno));
         sprintf(err, CFAIL, http->hostport, eno);
         write_socket(csp->cfd, err, strlen(err));

         freez(err);
         freez(hdr);
         return;
      }
   }
   else
   {
      /*
       * We're running an SSL tunnel and we're not forwarding,
       * so just send the "connect succeeded" message to the
       * client, flush the rest, and get out of the way.
       */
      if (write_socket(csp->cfd, CSUCCEED, sizeof(CSUCCEED)-1) < 0)
      {
         freez(hdr);
         return;
      }
      IOB_RESET(csp);
   }

   /* we're finished with the client's header */
   freez(hdr);

   maxfd = ( csp->cfd > csp->sfd ) ? csp->cfd : csp->sfd;

   /* pass data between the client and server
    * until one or the other shuts down the connection.
    */

   server_body = 0;

   while (FOREVER)
   {
      FD_ZERO(&rfds);

      FD_SET(csp->cfd, &rfds);
      FD_SET(csp->sfd, &rfds);

      n = select(maxfd+1, &rfds, NULL, NULL, NULL);

      if (n < 0)
      {
         log_error(LOG_LEVEL_ERROR, "select() failed!: %E");
         return;
      }

      /* this is the body of the browser's request
       * just read it and write it.
       */

      if (FD_ISSET(csp->cfd, &rfds))
      {
         n = read_socket(csp->cfd, buf, sizeof(buf));

         if (n <= 0) break; /* "game over, man" */

         if (write_socket(csp->sfd, buf, n) != n)
         {
            log_error(LOG_LEVEL_ERROR, "write to: %s failed: %E", http->host);
            return;
         }
         continue;
      }

      /*
       * The server wants to talk.  It could be the header or the body.
       * If `hdr' is null, then it's the header otherwise it's the body.
       * FIXME: Does `hdr' really mean `host'?
       */


      if (FD_ISSET(csp->sfd, &rfds))
      {
         fflush( 0 );
         n = read_socket(csp->sfd, buf, sizeof(buf) - 1);

         if (n < 0)
         {
            log_error(LOG_LEVEL_ERROR, "read from: %s failed: %E", http->host);

            eno = safe_strerror(errno);
            sprintf(buf, CFAIL, http->hostport, eno);
            freez(eno);
            write_socket(csp->cfd, buf, strlen(buf));
            return;
         }

         /* Add a trailing zero.  This lets filter_popups
          * use string operations.
          */
         buf[n] = '\0';

#ifdef KILLPOPUPS
         /* Filter the popups on this read. */
         if (block_popups_now)
         {
            filter_popups(buf, n);
         }
#endif /* def KILLPOPUPS */

         /* Normally, this would indicate that we've read
          * as much as the server has sent us and we can
          * close the client connection.  However, Microsoft
          * in its wisdom has released IIS/5 with a bug that
          * prevents it from sending the trailing \r\n in
          * a 302 redirect header (and possibly other headers).
          * To work around this if we've haven't parsed
          * a full header we'll append a trailing \r\n
          * and see if this now generates a valid one.
          *
          * This hack shouldn't have any impacts.  If we've
          * already transmitted the header or if this is a
          * SSL connection, then we won't bother with this
          * hack.  So we only work on partially received
          * headers.  If we append a \r\n and this still
          * doesn't generate a valid header, then we won't
          * transmit anything to the client.
          */
         if (n == 0)
         {
            /* This hack must only be enforced for headers. */
            if (server_body || http->ssl)
            {
#ifdef PCRS
               if (filtering)
               {
                  re_process_buffer(csp);
               }
#endif /* def PCRS */
               break; /* "game over, man" */
            }

            /* Let's pretend the server just sent us a blank line. */
            n = sprintf(buf, "\r\n");

            /*
             * Now, let the normal header parsing algorithm below do its
             * job.  If it fails, we'll exit instead of continuing.
             */

            ms_iis5_hack = 1;
         }

         /*
          * If this is an SSL connection or we're in the body
          * of the server document, just write it to the client.
          */

         if (server_body || http->ssl)
         {
#ifdef PCRS
            if (filtering)
            {
               add_to_iob(csp, buf, n); /* Buffer the body for filtering */
            }
            else
#endif /* def PCRS */
            {
               /* just write */
               if (write_socket(csp->cfd, buf, n) != n)
               {
                  log_error(LOG_LEVEL_ERROR, "write to client failed: %E");
                  return;
               }
            }
            continue;
         }
         else
         {
            /* we're still looking for the end of the
             * server's header ... (does that make header
             * parsing an "out of body experience" ?
             */

            /* buffer up the data we just read */
            add_to_iob(csp, buf, n);

            /* get header lines from the iob */

            while ((p = get_header(csp)))
            {
               if (*p == '\0')
               {
                  /* see following note */
                  break;
               }
               enlist(csp->headers, p);
               freez(p);
            }

            /* NOTE: there are no "empty" headers so
             * if the pointer `p' is not NULL we must
             * assume that we reached the end of the
             * buffer before we hit the end of the header.
             */

            if (p)
            {
               if (ms_iis5_hack)
               {
                  /* Well, we tried our MS IIS/5
                   * hack and it didn't work.
                   * The header is incomplete
                   * and there isn't anything
                   * we can do about it.
                   */
                  break;
               }
               else
               {
                  /* Since we have to wait for
                   * more from the server before
                   * we can parse the headers
                   * we just continue here.
                   */
                  continue;
               }
            }

            /* we have now received the entire header.
             * filter it and send the result to the client
             */

            hdr = sed(server_patterns, add_server_headers, csp);
            n   = strlen(hdr);

            /* write the server's (modified) header to
             * the client (along with anything else that
             * may be in the buffer)
             */

#ifdef KILLPOPUPS
            /* Start blocking popups if appropriate. */

            if (csp->is_text  &&  /* It's a text / * MIME-Type */
                !http->ssl    &&  /* We talk plaintext */
                block_popups)
            {
               block_popups_now = 1;
            }

#endif /* def KILLPOPUPS */

#ifdef PCRS
            /* Start re_filtering this if appropriate. */

            if (csp->is_text  &&  /* It's a text / * MIME-Type */
                !http->ssl    &&  /* We talk plaintext */
                pcrs_filter)      /* Policy allows */
            {
               filtering = 1;
            }

/* This next line is a little ugly, but it simplifies the if statement below. */
/* Basically if using PCRS, we want the OR condition to require "!filtering"  */
#define NOT_FILTERING_AND !filtering &&

#else /* not def PCRS */

#define NOT_FILTERING_AND

#endif /* def PCRS */


            if ((write_socket(csp->cfd, hdr, n) != n)
                || (NOT_FILTERING_AND (flush_socket(csp->cfd, csp) < 0)))
            {
               log_error(LOG_LEVEL_CONNECT, "write header to client failed: %E");

               /* the write failed, so don't bother
                * mentioning it to the client...
                * it probably can't hear us anyway.
                */
               freez(hdr);
               return;
            }

            /* we're finished with the server's header */

            freez(hdr);
            server_body = 1;

            /* If this was a MS IIS/5 hack then it means
             * the server has already closed the
             * connection.  Nothing more to read.  Time
             * to bail.
             */
            if (ms_iis5_hack)
            {
               break;
            }
         }
         continue;
      }

      return; /* huh? we should never get here */
   }


}


/*********************************************************************
 *
 * Function    :  serve
 *
 * Description :  This is little more than chat.  We only "serve" to
 *                to close any socket that chat may have opened.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
#ifdef AMIGA
void serve(struct client_state *csp)
#else /* ifndef AMIGA */
static void serve(struct client_state *csp)
#endif /* def AMIGA */
{
   chat(csp);
   close_socket(csp->cfd);

   if (csp->sfd >= 0)
   {
      close_socket(csp->sfd);
   }

   csp->active = 0;

}


#ifdef __BEOS__

/*********************************************************************
 *
 * Function    :  server_thread
 *
 * Description :  We only exist to call `serve' in a threaded environment.
 *
 * Parameters  :
 *          1  :  data = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Always 0.
 *
 *********************************************************************/
static int32 server_thread(void *data)
{
   serve((struct client_state *) data);
   return 0;

}

#endif


/*********************************************************************
 *
 * Function    :  main
 *
 * Description :  Load the config file and start the listen loop.
 *                This function is a lot more *sane* with the `load_config'
 *                and `listen_loop' functions; although it stills does
 *                a *little* too much for my taste.
 *
 * Parameters  :
 *          1  :  argc = Number of parameters (including $0).
 *          2  :  argv = Array of (char *)'s to the parameters.
 *
 * Returns     :  1 if : can't open config file, unrecognized directive,
 *                stats requested in multi-thread mode, can't open the
 *                log file, can't open the jar file, listen port is invalid,
 *                any load fails, and can't bind port.
 *
 *                Else main never returns, the process must be signaled
 *                to terminate execution.  Or, on Windows, use the 
 *                "File", "Exit" menu option.
 *
 *********************************************************************/
#ifdef __MINGW32__
int _main(int argc, const char *argv[])
#else
int main(int argc, const char *argv[])
#endif
{
   configfile =
#ifdef AMIGA
   "AmiTCP:db/junkbuster.config"
#elif !defined(_WIN32)
   "config"
#else
   "junkbstr.txt"
#endif
      ;

#if !defined(_WIN32) || defined(_WIN_CONSOLE)
   if ((argc >= 2) && (strcmp(argv[1], "--help")==0))
   {
      printf("JunkBuster proxy version " VERSION ".\n\n"
         "Usage: %s [configfile]\n\n"
         "See " HOME_PAGE_URL " for details.\n"
         "This program is distributed under the GNU GPL, version 2 or later.\n",
         argv[0]);
      exit(2);
   }
   if ((argc >= 2) && (strcmp(argv[1], "--version")==0))
   {
      printf(VERSION "\n");
      exit(2);
   }
#endif /* !defined(_WIN32) || defined(_WIN_CONSOLE) */

#ifdef AMIGA
   InitAmiga();
#endif

   Argc = argc;
   Argv = argv;

   if (argc > 1)
   {
      configfile = argv[1];
   }

   remove_all_loaders();
   memset( proxy_args, 0, sizeof( proxy_args ) );
   files->next = NULL;

   load_config( 0 );

   /*
    * Since load_config acts as a signal handler too, it returns
    * its status in configret.  Check it for an error in loading.
    */
   if ( 0 != configret )
   {
      /* load config failed!  Exit with error. */
      return( 1 );
   }

#ifdef _WIN32
   InitWin32();
#endif


#ifndef _WIN32
   signal(SIGPIPE, SIG_IGN);
   signal(SIGCHLD, SIG_IGN);
   signal(SIGHUP, load_config);

#else /* ifdef _WIN32 */
# ifdef _WIN_CONSOLE
   /*
    * We *are* in a windows console app.
    * Print a verbose messages about FAQ's and such
    */
   printf(win32_blurb);
# endif /* def _WIN_CONSOLE */
#endif /* def _WIN32 */


   listen_loop();

   /* NOTREACHED */
   return(-1);

}


/*********************************************************************
 *
 * Function    :  listen_loop
 *
 * Description :  bind the listen port and enter a "FOREVER" listening loop.
 *
 * Parameters  :  N/A
 *
 * Returns     :  Never.
 *
 *********************************************************************/
static void listen_loop(void)
{
   struct client_state *csp = NULL;
   int bfd;

   log_error(LOG_LEVEL_CONNECT, "bind (%s, %d)",
             haddr ? haddr : "INADDR_ANY", hport);

   bfd = bind_port(haddr, hport);
   config_changed = 0;

   if (bfd < 0)
   {
      log_error(LOG_LEVEL_FATAL, "can't bind %s:%d: %E "
         "- There may be another junkbuster or some other "
         "proxy running on port %d", 
         (NULL != haddr) ? haddr : "INADDR_ANY", hport, hport
      );
      /* shouldn't get here */
      return;
   }


   while (FOREVER)
   {
#if !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA)
      while (waitpid(-1, NULL, WNOHANG) > 0)
      {
         /* zombie children */
      }
#endif /* !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA) */
      sweep();

      if ( NULL == (csp = (struct client_state *) malloc(sizeof(*csp))) )
      {
         log_error(LOG_LEVEL_ERROR, "malloc(%d) for csp failed: %E", sizeof(*csp));
         continue;
      }

      memset(csp, '\0', sizeof(*csp));

      csp->active = 1;
      csp->sfd    = -1;

      if ( config_changed )
      {
         /*
          * Since we were listening to the "old port", we will not see
          * a "listen" param change until the next IJB request.  So, at
          * least 1 more request must be made for us to find the new
          * setting.  I am simply closing the old socket and binding the
          * new one.
          *
          * Which-ever is correct, we will serve 1 more page via the
          * old settings.  This should probably be a "show-proxy-args"
          * request.  This should not be a so common of an operation
          * that this will hurt people's feelings.
          */
         close_socket(bfd);

         log_error(LOG_LEVEL_CONNECT, "bind (%s, %d)",
                   haddr ? haddr : "INADDR_ANY", hport);
         bfd = bind_port(haddr, hport);

         config_changed = 0;
      }

      log_error(LOG_LEVEL_CONNECT, "accept connection ... ");

      if (!accept_connection(csp, bfd))
      {
         log_error(LOG_LEVEL_CONNECT, "accept failed: %E");

#ifdef AMIGA
         if(!childs)
         {
            exit(1); 
         }
#endif
         continue;
      }
      else
      {
         log_error(LOG_LEVEL_CONNECT, "OK");
      }

#if defined(TOGGLE)
      /* by haroon - most of credit to srt19170 */
      csp->toggled_on = g_bToggleIJB;
#endif

      /* add it to the list of clients */
      csp->next = clients->next;
      clients->next = csp;

      if (run_loader(csp))
      {
         log_error(LOG_LEVEL_FATAL, "a loader failed - must exit");
         /* Never get here - LOG_LEVEL_FATAL causes program exit */
      }

      if (multi_threaded)
      {
         int child_id;

/* this is a switch () statment in the C preprocessor - ugh */
#undef SELECTED_ONE_OPTION

#if defined(_WIN32) && !defined(_CYGWIN) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
         child_id = _beginthread(
            (void*)serve,
            64 * 1024,
            csp);
#endif

#if defined(__BEOS__) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
         {
            thread_id tid = spawn_thread
               (server_thread, "server", B_NORMAL_PRIORITY, csp);

            if ((tid >= 0) && (resume_thread(tid) == B_OK))
            {
               child_id = (int) tid;
            }
            else
            {
               child_id = -1;
            }
         }
#endif

#if defined(AMIGA) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
         csp->cfd = ReleaseSocket(csp->cfd, -1);
         if((child_id = (int)CreateNewProcTags(
            NP_Entry, (ULONG)server_thread,
            NP_Output, Output(),
            NP_CloseOutput, FALSE,
            NP_Name, (ULONG)"junkbuster child",
            NP_StackSize, 20*1024,
            TAG_DONE)))
         {
            childs++;
            ((struct Task *)child_id)->tc_UserData = csp;
            Signal((struct Task *)child_id, SIGF_SINGLE);
            Wait(SIGF_SINGLE);
         }
#endif

#if !defined(SELECTED_ONE_OPTION)
         child_id = fork();
#endif

#undef SELECTED_ONE_OPTION
/* end of cpp switch () */

         if (child_id < 0) /* failed */
         {
            char buf[BUFSIZ];

            log_error(LOG_LEVEL_ERROR, "can't fork: %E");

            sprintf(buf , "JunkBuster: can't fork: errno = %d", errno);

            write_socket(csp->cfd, buf, strlen(buf));
            close_socket(csp->cfd);
            csp->active = 0;
            sleep(5);
            continue;
         }

#if !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA)
         /* This block is only needed when using fork().
          * When using threads, the server thread was
          * created and run by the call to _beginthread().
          */
         if (child_id == 0)   /* child */
         {
            serve(csp);
            _exit(0);

         }
         else  /* parent */
         {
            /* in a fork()'d environment, the parent's
             * copy of the client socket and the CSP
             * are not used.
             */

#if !defined(_WIN32) && defined(__CYGWIN__)
            wait( NULL );
#endif /* !defined(_WIN32) && defined(__CYGWIN__) */
            close_socket(csp->cfd);
            csp->active = 0;
         }
#endif /* !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA) */
      }
      else
      {
         serve(csp);
      }
   }
   /* NOTREACHED */

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
