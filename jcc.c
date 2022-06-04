/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/jcc.c,v $
 *
 * Purpose     :  Main file.  Contains main() method, main loop, and
 *                the main connection-handling function.
 *
 * Copyright   :  Written by and Copyright (C) 2001-2022 the
 *                Privoxy team. https://www.privoxy.org/
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
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#ifdef _WIN32
# ifndef FEATURE_PTHREAD
#  ifndef STRICT
#   define STRICT
#  endif
#  include <winsock2.h>
#  include <windows.h>
#  include <process.h>
# endif /* ndef FEATURE_PTHREAD */

# include "win32.h"
# ifndef _WIN_CONSOLE
#  include "w32log.h"
# endif /* ndef _WIN_CONSOLE */
# include "w32svrapi.h"

#else /* ifndef _WIN32 */

# include <unistd.h>
# include <sys/wait.h>
# include <sys/time.h>
# include <sys/stat.h>
# include <sys/ioctl.h>

#ifdef sun
#include <sys/termios.h>
#endif /* sun */

#ifdef unix
#include <pwd.h>
#include <grp.h>
#endif

# include <signal.h>

# ifdef __BEOS__
#  include <socket.h>  /* BeOS has select() for sockets only. */
#  include <OS.h>      /* declarations for threads and stuff. */
# endif

#ifdef HAVE_POLL
#ifdef __GLIBC__
#include <sys/poll.h>
#else
#include <poll.h>
#endif /* def __GLIBC__ */
#else
# ifndef FD_ZERO
#  include <select.h>
# endif
#warning poll() appears to be unavailable. Your platform will become unsupported in the future.
#endif /* HAVE_POLL */

#endif

#include "project.h"
#include "list.h"
#include "jcc.h"
#ifdef FEATURE_HTTPS_INSPECTION
#include "ssl.h"
#endif
#include "filters.h"
#include "loaders.h"
#include "parsers.h"
#include "miscutil.h"
#include "errlog.h"
#include "jbsockets.h"
#include "gateway.h"
#include "actions.h"
#include "cgi.h"
#include "loadcfg.h"
#include "urlmatch.h"
#ifdef FEATURE_CLIENT_TAGS
#include "client-tags.h"
#endif

int daemon_mode = 1;
struct client_states clients[1];
struct file_list     files[1];

#ifdef FEATURE_STATISTICS
int urls_read     = 0;     /* total nr of urls read inc rejected */
int urls_rejected = 0;     /* total nr of urls rejected */
#ifdef MUTEX_LOCKS_AVAILABLE
unsigned long long number_of_requests_received = 0;
unsigned long long number_of_requests_blocked = 0;
#endif
#endif /* def FEATURE_STATISTICS */

#ifdef FEATURE_GRACEFUL_TERMINATION
int g_terminate = 0;
#endif

#if !defined(_WIN32)
static void sig_handler(int the_signal);
#endif
static int client_protocol_is_unsupported(struct client_state *csp, char *req);
static jb_err get_request_destination_elsewhere(struct client_state *csp, struct list *headers);
static jb_err get_server_headers(struct client_state *csp);
static const char *crunch_reason(const struct http_response *rsp);
static void send_crunch_response(struct client_state *csp, struct http_response *rsp);
static char *get_request_line(struct client_state *csp);
static jb_err receive_client_request(struct client_state *csp);
static jb_err parse_client_request(struct client_state *csp);
static void build_request_line(struct client_state *csp, const struct forward_spec *fwd, char **request_line);
static jb_err change_request_destination(struct client_state *csp);
static void handle_established_connection(struct client_state *csp);
static void chat(struct client_state *csp);
static void serve(struct client_state *csp);
#if !defined(_WIN32) || defined(_WIN_CONSOLE)
static void usage(const char *myname);
#endif
static void initialize_mutexes(void);
static void release_mutexes(void);
static jb_socket bind_port_helper(const char *haddr, int hport, int backlog);
static void bind_ports_helper(struct configuration_spec *config, jb_socket sockets[]);
static void close_ports_helper(jb_socket sockets[]);
static void listen_loop(void);
static void serve(struct client_state *csp);

#ifdef __BEOS__
static int32 server_thread(void *data);
#endif /* def __BEOS__ */

#ifdef _WIN32
#define sleep(N)  Sleep(((N) * 1000))
#endif

#ifdef FUZZ
int process_fuzzed_input(char *fuzz_input_type, char *fuzz_input_file);
void show_fuzz_usage(const char *name);
#endif

#ifdef MUTEX_LOCKS_AVAILABLE
/*
 * XXX: Does the locking stuff really belong in this file?
 */
privoxy_mutex_t log_mutex;
privoxy_mutex_t log_init_mutex;
privoxy_mutex_t connection_reuse_mutex;

#ifdef FEATURE_HTTPS_INSPECTION
privoxy_mutex_t certificate_mutex;
privoxy_mutex_t ssl_init_mutex;
#endif

#ifdef FEATURE_EXTERNAL_FILTERS
privoxy_mutex_t external_filter_mutex;
#endif
#ifdef FEATURE_CLIENT_TAGS
privoxy_mutex_t client_tags_mutex;
#endif
#ifdef FEATURE_STATISTICS
privoxy_mutex_t block_statistics_mutex;
#endif
#ifdef FEATURE_EXTENDED_STATISTICS
privoxy_mutex_t filter_statistics_mutex;
privoxy_mutex_t block_reason_statistics_mutex;
#endif

#if !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R)
privoxy_mutex_t resolver_mutex;
#endif /* !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R) */

#ifndef HAVE_GMTIME_R
privoxy_mutex_t gmtime_mutex;
#endif /* ndef HAVE_GMTIME_R */

#ifndef HAVE_LOCALTIME_R
privoxy_mutex_t localtime_mutex;
#endif /* ndef HAVE_GMTIME_R */

#if !defined(HAVE_ARC4RANDOM) && !defined(HAVE_RANDOM)
privoxy_mutex_t rand_mutex;
#endif /* !defined(HAVE_ARC4RANDOM) && !defined(HAVE_RANDOM) */

#endif /* def MUTEX_LOCKS_AVAILABLE */

#if defined(unix)
const char *basedir = NULL;
const char *pidfile = NULL;
static int received_hup_signal = 0;
#endif /* defined unix */

/* HTTP snipplets. */
static const char CSUCCEED[] =
   "HTTP/1.1 200 Connection established\r\n\r\n";

static const char CHEADER[] =
   "HTTP/1.1 400 Invalid header received from client\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "Invalid header received from client.\n";

static const char FTP_RESPONSE[] =
   "HTTP/1.1 400 Invalid request received from client\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "Invalid request. Privoxy doesn't support FTP.\n";

static const char GOPHER_RESPONSE[] =
   "HTTP/1.1 400 Invalid request received from client\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "Invalid request. Privoxy doesn't support gopher.\n";

/* XXX: should be a template */
static const char MISSING_DESTINATION_RESPONSE[] =
   "HTTP/1.1 400 Bad request received from client\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "Bad request. Privoxy was unable to extract the destination.\n";

/* XXX: should be a template */
static const char INVALID_SERVER_HEADERS_RESPONSE[] =
   "HTTP/1.1 502 Server or forwarder response invalid\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "Bad response. The server or forwarder response doesn't look like HTTP.\n";

/* XXX: should be a template */
static const char MESSED_UP_REQUEST_RESPONSE[] =
   "HTTP/1.1 400 Malformed request after rewriting\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "Bad request. Messed up with header filters.\n";

static const char TOO_MANY_CONNECTIONS_RESPONSE[] =
   "HTTP/1.1 503 Too many open connections\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "Maximum number of open connections reached.\n";

static const char CLIENT_CONNECTION_TIMEOUT_RESPONSE[] =
   "HTTP/1.1 504 Connection timeout\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "The connection timed out because the client request didn't arrive in time.\n";

static const char CLIENT_BODY_PARSE_ERROR_RESPONSE[] =
   "HTTP/1.1 400 Failed reading client body\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "Failed parsing or buffering the chunk-encoded client body.\n";

static const char CLIENT_BODY_BUFFER_ERROR_RESPONSE[] =
   "HTTP/1.1 400 Failed reading client body\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "Failed to buffer the client body to apply content filters.\n"
   "Could be caused by a socket timeout\n";

static const char UNSUPPORTED_CLIENT_EXPECTATION_ERROR_RESPONSE[] =
   "HTTP/1.1 417 Expecting too much\r\n"
   "Content-Type: text/plain\r\n"
   "Connection: close\r\n\r\n"
   "Privoxy detected an unsupported Expect header value.\n";

/* A function to crunch a response */
typedef struct http_response *(*crunch_func_ptr)(struct client_state *);

/* Crunch function flags */
#define CF_NO_FLAGS        0
/* Cruncher applies to forced requests as well */
#define CF_IGNORE_FORCE    1
/* Crunched requests are counted for the block statistics */
#define CF_COUNT_AS_REJECT 2

/* A crunch function and its flags */
struct cruncher
{
   const crunch_func_ptr cruncher;
   const int flags;
};

static int crunch_response_triggered(struct client_state *csp, const struct cruncher crunchers[]);

/* Complete list of cruncher functions */
static const struct cruncher crunchers_all[] = {
   { direct_response, CF_COUNT_AS_REJECT|CF_IGNORE_FORCE},
   { block_url,       CF_COUNT_AS_REJECT },
#ifdef FEATURE_TRUST
   { trust_url,       CF_COUNT_AS_REJECT },
#endif /* def FEATURE_TRUST */
   { redirect_url,    CF_NO_FLAGS  },
   { dispatch_cgi,    CF_IGNORE_FORCE},
   { NULL,            0 }
};

/* Light version, used after tags are applied */
static const struct cruncher crunchers_light[] = {
   { block_url,       CF_COUNT_AS_REJECT },
   { redirect_url,    CF_NO_FLAGS },
   { NULL,            0 }
};


/*
 * XXX: Don't we really mean
 *
 * #if defined(unix)
 *
 * here?
 */
#if !defined(_WIN32)
/*********************************************************************
 *
 * Function    :  sig_handler
 *
 * Description :  Signal handler for different signals.
 *                Exit gracefully on TERM and INT
 *                or set a flag that will cause the errlog
 *                to be reopened by the main thread on HUP.
 *
 * Parameters  :
 *          1  :  the_signal = the signal cause this function to call
 *
 * Returns     :  -
 *
 *********************************************************************/
static void sig_handler(int the_signal)
{
   switch(the_signal)
   {
      case SIGTERM:
      case SIGINT:
         log_error(LOG_LEVEL_INFO, "exiting by signal %d .. bye", the_signal);
#if defined(unix)
         if (pidfile)
         {
            unlink(pidfile);
         }
#endif /* unix */
         exit(the_signal);
         break;

      case SIGHUP:
#if defined(unix)
         received_hup_signal = 1;
#endif
         break;

      default:
         /*
          * We shouldn't be here, unless we catch signals
          * in main() that we can't handle here!
          */
         log_error(LOG_LEVEL_FATAL,
            "sig_handler: exiting on unexpected signal %d", the_signal);
   }
   return;

}
#endif


/*********************************************************************
 *
 * Function    :  get_write_delay
 *
 * Description :  Parse the delay-response parameter.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Number of milliseconds to delay writes.
 *
 *********************************************************************/
static unsigned int get_write_delay(const struct client_state *csp)
{
   unsigned int delay;
   char *endptr;
   char *newval;

   if ((csp->action->flags & ACTION_DELAY_RESPONSE) == 0)
   {
      return 0;
   }
   newval = csp->action->string[ACTION_STRING_DELAY_RESPONSE];

   delay = (unsigned)strtol(newval, &endptr, 0);
   if (*endptr != '\0')
   {
      log_error(LOG_LEVEL_FATAL,
         "Invalid delay-response{} parameter: '%s'", newval);
   }

   return delay;

}


/*********************************************************************
 *
 * Function    :  client_protocol_is_unsupported
 *
 * Description :  Checks if the client used a known unsupported
 *                protocol and deals with it by sending an error
 *                response.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  req = the first request line send by the client
 *
 * Returns     :  TRUE if an error response has been generated, or
 *                FALSE if the request doesn't look invalid.
 *
 *********************************************************************/
static int client_protocol_is_unsupported(struct client_state *csp, char *req)
{
   /*
    * If it's a FTP or gopher request, we don't support it.
    *
    * These checks are better than nothing, but they might
    * not work in all configurations and some clients might
    * have problems digesting the answer.
    *
    * They should, however, never cause more problems than
    * Privoxy's old behaviour (returning the misleading HTML
    * error message:
    *
    * "Could not resolve http://(ftp|gopher)://example.org").
    */
   if (!strncmpic(req, "GET ftp://", 10) || !strncmpic(req, "GET gopher://", 13))
   {
      const char *response = NULL;
      const char *protocol = NULL;

      if (!strncmpic(req, "GET ftp://", 10))
      {
         response = FTP_RESPONSE;
         protocol = "FTP";
      }
      else
      {
         response = GOPHER_RESPONSE;
         protocol = "GOPHER";
      }
      log_error(LOG_LEVEL_ERROR,
         "%s tried to use Privoxy as %s proxy: %s",
         csp->ip_addr_str, protocol, req);
      log_error(LOG_LEVEL_CLF,
         "%s - - [%T] \"%s\" 400 0", csp->ip_addr_str, req);
      freez(req);

#ifdef FEATURE_HTTPS_INSPECTION
      if (client_use_ssl(csp))
      {
         ssl_send_data_delayed(&(csp->ssl_client_attr),
            (const unsigned char *)response, strlen(response),
            get_write_delay(csp));
      }
      else
#endif
      {
         write_socket_delayed(csp->cfd, response, strlen(response),
            get_write_delay(csp));
      }

      return TRUE;
   }

   return FALSE;
}


/*********************************************************************
 *
 * Function    :  client_has_unsupported_expectations
 *
 * Description :  Checks if the client used an unsupported expectation
 *                in which case an error message is delivered.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  TRUE if an error response has been generated, or
 *                FALSE if the request doesn't look invalid.
 *
 *********************************************************************/
static int client_has_unsupported_expectations(const struct client_state *csp)
{
   if ((csp->flags & CSP_FLAG_UNSUPPORTED_CLIENT_EXPECTATION))
   {
      log_error(LOG_LEVEL_ERROR,
         "Rejecting request from client %s with unsupported Expect header value",
         csp->ip_addr_str);
      log_error(LOG_LEVEL_CLF,
         "%s - - [%T] \"%s\" 417 0", csp->ip_addr_str, csp->http->cmd);
      write_socket_delayed(csp->cfd,
         UNSUPPORTED_CLIENT_EXPECTATION_ERROR_RESPONSE,
         strlen(UNSUPPORTED_CLIENT_EXPECTATION_ERROR_RESPONSE),
         get_write_delay(csp));

      return TRUE;
   }

   return FALSE;

}


/*********************************************************************
 *
 * Function    :  get_request_destination_elsewhere
 *
 * Description :  If the client's request was redirected into
 *                Privoxy without the client's knowledge,
 *                the request line lacks the destination host.
 *
 *                This function tries to get it elsewhere,
 *                provided accept-intercepted-requests is enabled.
 *
 *                "Elsewhere" currently only means "Host: header",
 *                but in the future we may ask the redirecting
 *                packet filter to look the destination up.
 *
 *                If the destination stays unknown, an error
 *                response is send to the client and headers
 *                are freed so that chat() can return directly.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  headers = a header list
 *
 * Returns     :  JB_ERR_OK if the destination is now known, or
 *                JB_ERR_PARSE if it isn't.
 *
 *********************************************************************/
static jb_err get_request_destination_elsewhere(struct client_state *csp, struct list *headers)
{
   if (!(csp->config->feature_flags & RUNTIME_FEATURE_ACCEPT_INTERCEPTED_REQUESTS))
   {
      log_error(LOG_LEVEL_ERROR, "%s's request: \'%s\' is invalid."
         " Privoxy isn't configured to accept intercepted requests.",
         csp->ip_addr_str, csp->http->cmd);
      /* XXX: Use correct size */
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 400 0",
         csp->ip_addr_str, csp->http->cmd);

      write_socket_delayed(csp->cfd, CHEADER, strlen(CHEADER),
         get_write_delay(csp));
      destroy_list(headers);

      return JB_ERR_PARSE;
   }
   else if (JB_ERR_OK == get_destination_from_headers(headers, csp->http))
   {
      /* Split the domain we just got for pattern matching */
      init_domain_components(csp->http);

      return JB_ERR_OK;
   }
   else
   {
      /* We can't work without destination. Go spread the news.*/

      /* XXX: Use correct size */
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 400 0",
         csp->ip_addr_str, csp->http->cmd);
      log_error(LOG_LEVEL_ERROR,
         "Privoxy was unable to get the destination for %s's request: %s",
         csp->ip_addr_str, csp->http->cmd);

      write_socket_delayed(csp->cfd, MISSING_DESTINATION_RESPONSE,
         strlen(MISSING_DESTINATION_RESPONSE), get_write_delay(csp));
      destroy_list(headers);

      return JB_ERR_PARSE;
   }
   /*
    * TODO: If available, use PF's ioctl DIOCNATLOOK as last resort
    * to get the destination IP address, use it as host directly
    * or do a reverse DNS lookup first.
    */
}


/*********************************************************************
 *
 * Function    :  get_server_headers
 *
 * Description :  Parses server headers in iob and fills them
 *                into csp->headers so that they can later be
 *                handled by sed().
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_ERR_OK if everything went fine, or
 *                JB_ERR_PARSE if the headers were incomplete.
 *
 *********************************************************************/
static jb_err get_server_headers(struct client_state *csp)
{
   int continue_hack_in_da_house = 0;
   char * header;

   while (((header = get_header(csp->iob)) != NULL) || continue_hack_in_da_house)
   {
      if (header == NULL)
      {
         /*
          * continue hack in da house. Ignore the ending of
          * this head and continue enlisting header lines.
          * The reason is described below.
          */
         enlist(csp->headers, "");
         continue_hack_in_da_house = 0;
         continue;
      }
      else if (0 == strncmpic(header, "HTTP/1.1 100", 12))
      {
         /*
          * It's a bodyless continue response, don't
          * stop header parsing after reaching its end.
          *
          * As a result Privoxy will concatenate the
          * next response's head and parse and deliver
          * the headers as if they belonged to one request.
          *
          * The client will separate them because of the
          * empty line between them.
          *
          * XXX: What we're doing here is clearly against
          * the intended purpose of the continue header,
          * and under some conditions (HTTP/1.0 client request)
          * it's a standard violation.
          *
          * Anyway, "sort of against the spec" is preferable
          * to "always getting confused by Continue responses"
          * (Privoxy's behaviour before this hack was added)
          */
         log_error(LOG_LEVEL_HEADER, "Continue hack in da house.");
         continue_hack_in_da_house = 1;
      }
      else if (*header == '\0')
      {
         /*
          * If the header is empty, but the Continue hack
          * isn't active, we can assume that we reached the
          * end of the buffer before we hit the end of the
          * head.
          *
          * Inform the caller an let it decide how to handle it.
          */
         return JB_ERR_PARSE;
      }

      if (JB_ERR_MEMORY == enlist(csp->headers, header))
      {
         /*
          * XXX: Should we quit the request and return a
          * out of memory error page instead?
          */
         log_error(LOG_LEVEL_ERROR,
            "Out of memory while enlisting server headers. %s lost.",
            header);
      }
      freez(header);
   }

   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  crunch_reason
 *
 * Description :  Translates the crunch reason code into a string.
 *
 * Parameters  :
 *          1  :  rsp = a http_response
 *
 * Returns     :  A string with the crunch reason or an error description.
 *
 *********************************************************************/
static const char *crunch_reason(const struct http_response *rsp)
{
   char * reason = NULL;

   assert(rsp != NULL);
   if (rsp == NULL)
   {
      return "Internal error while searching for crunch reason";
   }

   switch (rsp->crunch_reason)
   {
      case UNSUPPORTED:
         reason = "Unsupported HTTP feature";
         break;
      case BLOCKED:
         reason = "Blocked";
         break;
      case UNTRUSTED:
         reason = "Untrusted";
         break;
      case REDIRECTED:
         reason = "Redirected";
         break;
      case CGI_CALL:
         reason = "CGI Call";
         break;
      case NO_SUCH_DOMAIN:
         reason = "DNS failure";
         break;
      case FORWARDING_FAILED:
         reason = "Forwarding failed";
         break;
      case CONNECT_FAILED:
         reason = "Connection failure";
         break;
      case OUT_OF_MEMORY:
         reason = "Out of memory (may mask other reasons)";
         break;
      case CONNECTION_TIMEOUT:
         reason = "Connection timeout";
         break;
      case NO_SERVER_DATA:
         reason = "No server data received";
         break;
      default:
         reason = "No reason recorded";
         break;
   }

   return reason;
}


/*********************************************************************
 *
 * Function    :  log_applied_actions
 *
 * Description :  Logs the applied actions if LOG_LEVEL_ACTIONS is
 *                enabled.
 *
 * Parameters  :
 *          1  :  actions = Current action spec to log
 *
 * Returns     :  Nothing.
 *
 *********************************************************************/
static void log_applied_actions(const struct current_action_spec *actions)
{
   /*
    * The conversion to text requires lots of memory allocations so
    * we only do the conversion if the user is actually interested.
    */
   if (debug_level_is_enabled(LOG_LEVEL_ACTIONS))
   {
      char *actions_as_text = actions_to_line_of_text(actions);
      log_error(LOG_LEVEL_ACTIONS, "%s", actions_as_text);
      freez(actions_as_text);
   }
}


/*********************************************************************
 *
 * Function    :  send_crunch_response
 *
 * Description :  Delivers already prepared response for
 *                intercepted requests, logs the interception
 *                and frees the response.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = Fully prepared response. Will be freed on exit.
 *
 * Returns     :  Nothing.
 *
 *********************************************************************/
static void send_crunch_response(struct client_state *csp, struct http_response *rsp)
{
      const struct http_request *http = csp->http;
      char status_code[4];

      assert(rsp != NULL);
      assert(rsp->head != NULL);

      if (rsp == NULL)
      {
         log_error(LOG_LEVEL_FATAL, "NULL response in send_crunch_response.");
      }

      /*
       * Extract the status code from the actual head
       * that will be send to the client. It is the only
       * way to get it right for all requests, including
       * the fixed ones for out-of-memory problems.
       *
       * A head starts like this: 'HTTP/1.1 200...'
       *                           0123456789|11
       *                                     10
       */
      status_code[0] = rsp->head[9];
      status_code[1] = rsp->head[10];
      status_code[2] = rsp->head[11];
      status_code[3] = '\0';

      /* Log that the request was crunched and why. */
      log_applied_actions(csp->action);
#ifdef FEATURE_HTTPS_INSPECTION
      if (client_use_ssl(csp))
      {
         log_error(LOG_LEVEL_CRUNCH, "%s: https://%s%s", crunch_reason(rsp),
            http->hostport, http->path);
         log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s https://%s%s %s\" %s %lu",
            csp->ip_addr_str, http->gpc, http->hostport, http->path,
            http->version, status_code, rsp->content_length);
      }
      else
#endif
      {
         log_error(LOG_LEVEL_CRUNCH, "%s: %s", crunch_reason(rsp), http->url);
         log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" %s %lu",
            csp->ip_addr_str, http->ocmd, status_code, rsp->content_length);
      }
      /* Write the answer to the client */
#ifdef FEATURE_HTTPS_INSPECTION
      if (client_use_ssl(csp))
      {
         if ((ssl_send_data_delayed(&(csp->ssl_client_attr),
                (const unsigned char *)rsp->head, rsp->head_length,
                get_write_delay(csp)) < 0)
          || (ssl_send_data_delayed(&(csp->ssl_client_attr),
                (const unsigned char *)rsp->body, rsp->content_length,
                get_write_delay(csp)) < 0))
         {
            /* There is nothing we can do about it. */
            log_error(LOG_LEVEL_CONNECT, "Couldn't deliver the error message "
               "for https://%s%s through client socket %d using TLS/SSL",
               http->hostport, http->url, csp->cfd);
         }
      }
      else
#endif
      {
         if (write_socket_delayed(csp->cfd, rsp->head, rsp->head_length,
                get_write_delay(csp))
          || write_socket_delayed(csp->cfd, rsp->body, rsp->content_length,
                get_write_delay(csp)))
         {
            /* There is nothing we can do about it. */
            log_error(LOG_LEVEL_CONNECT,
               "Couldn't deliver the error message for %s through client socket %d: %E",
               http->url, csp->cfd);
         }
      }

      /* Clean up and return */
      if (cgi_error_memory() != rsp)
      {
         free_http_response(rsp);
      }
      return;
}


/*********************************************************************
 *
 * Function    :  crunch_response_triggered
 *
 * Description :  Checks if the request has to be crunched,
 *                and delivers the crunch response if necessary.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  crunchers = list of cruncher functions to run
 *
 * Returns     :  TRUE if the request was answered with a crunch response
 *                FALSE otherwise.
 *
 *********************************************************************/
static int crunch_response_triggered(struct client_state *csp, const struct cruncher crunchers[])
{
   struct http_response *rsp = NULL;
   const struct cruncher *c;

   /*
    * If CGI request crunching is disabled,
    * check the CGI dispatcher out of order to
    * prevent unintentional blocks or redirects.
    */
   if (!(csp->config->feature_flags & RUNTIME_FEATURE_CGI_CRUNCHING)
       && (NULL != (rsp = dispatch_cgi(csp))))
   {
      /* Deliver, log and free the interception response. */
      send_crunch_response(csp, rsp);
      csp->flags |= CSP_FLAG_CRUNCHED;
      return TRUE;
   }

   for (c = crunchers; c->cruncher != NULL; c++)
   {
      /*
       * Check the cruncher if either Privoxy is toggled
       * on and the request isn't forced, or if the cruncher
       * applies to forced requests as well.
       */
      if (((csp->flags & CSP_FLAG_TOGGLED_ON) &&
          !(csp->flags & CSP_FLAG_FORCED)) ||
          (c->flags & CF_IGNORE_FORCE))
      {
         rsp = c->cruncher(csp);
         if (NULL != rsp)
         {
            /* Deliver, log and free the interception response. */
            send_crunch_response(csp, rsp);
            csp->flags |= CSP_FLAG_CRUNCHED;
#ifdef FEATURE_STATISTICS
            if (c->flags & CF_COUNT_AS_REJECT)
            {
#ifdef MUTEX_LOCKS_AVAILABLE
               privoxy_mutex_lock(&block_statistics_mutex);
               number_of_requests_blocked++;
               privoxy_mutex_unlock(&block_statistics_mutex);
#endif
               csp->flags |= CSP_FLAG_REJECTED;
            }
#endif /* def FEATURE_STATISTICS */

            return TRUE;
         }
      }
   }

   return FALSE;
}


/*********************************************************************
 *
 * Function    :  build_request_line
 *
 * Description :  Builds the HTTP request line.
 *
 *                If a HTTP forwarder is used it expects the whole URL,
 *                web servers only get the path.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  fwd = The forwarding spec used for the request.
 *                      Can be NULL.
 *          3  :  request_line = The old request line which will be replaced.
 *
 * Returns     :  Nothing. Terminates in case of memory problems.
 *
 *********************************************************************/
static void build_request_line(struct client_state *csp, const struct forward_spec *fwd, char **request_line)
{
   struct http_request *http = csp->http;

   /*
    * Downgrade http version from 1.1 to 1.0
    * if +downgrade action applies.
    */
   if ((csp->action->flags & ACTION_DOWNGRADE)
     && (!strcmpic(http->version, "HTTP/1.1")))
   {
      freez(http->version);
      http->version = strdup_or_die("HTTP/1.0");
   }

   /*
    * Rebuild the request line.
    */
   freez(*request_line);
#ifdef FEATURE_HTTPS_INSPECTION
   if (fwd != NULL && fwd->forward_host &&
       fwd->type != FORWARD_WEBSERVER && client_use_ssl(csp))
   {
      *request_line = strdup("CONNECT ");
   }
   else
#endif
   {
      *request_line = strdup(http->gpc);
      string_append(request_line, " ");
   }

   if (fwd != NULL && fwd->forward_host && fwd->type != FORWARD_WEBSERVER)
   {
#ifdef FEATURE_HTTPS_INSPECTION
      if (client_use_ssl(csp))
      {
         char port_string[10];

         string_append(request_line, http->host);
         snprintf(port_string, sizeof(port_string), ":%d", http->port);
         string_append(request_line, port_string);
      }
      else
#endif
      {
         string_append(request_line, http->url);
      }
   }
   else
   {
      string_append(request_line, http->path);
   }
   string_append(request_line, " ");
   string_append(request_line, http->version);

   if (*request_line == NULL)
   {
      log_error(LOG_LEVEL_FATAL, "Out of memory writing HTTP command");
   }
   log_error(LOG_LEVEL_HEADER, "New HTTP Request-Line: %s", *request_line);
}


/*********************************************************************
 *
 * Function    :  change_request_destination
 *
 * Description :  Parse a (rewritten) request line and regenerate
 *                the http request data.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Forwards the parse_http_request() return code.
 *                Terminates in case of memory problems.
 *
 *********************************************************************/
static jb_err change_request_destination(struct client_state *csp)
{
   struct http_request *http = csp->http;
   jb_err err;

   log_error(LOG_LEVEL_REDIRECTS, "Rewrite detected: %s",
      csp->headers->first->str);
   free_http_request(http);
   err = parse_http_request(csp->headers->first->str, http);
   if (JB_ERR_OK != err)
   {
      log_error(LOG_LEVEL_ERROR, "Couldn't parse rewritten request: %s.",
         jb_err_to_string(err));
   }
   if (http->ssl && strcmpic(csp->http->gpc, "CONNECT"))
   {
      /*
       * A client header filter changed the request URL from
       * http:// to https:// which we currently don't support.
       */
      log_error(LOG_LEVEL_ERROR, "Changing the request destination from http "
         "to https behind the client's back currently isn't supported.");
      return JB_ERR_PARSE;
   }

   return err;
}


#ifdef FEATURE_CONNECTION_KEEP_ALIVE
/*********************************************************************
 *
 * Function    :  server_response_is_complete
 *
 * Description :  Determines whether we should stop reading
 *                from the server socket.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  content_length = Length of content received so far.
 *
 * Returns     :  TRUE if the response is complete,
 *                FALSE otherwise.
 *
 *********************************************************************/
static int server_response_is_complete(struct client_state *csp,
   unsigned long long content_length)
{
   int content_length_known = !!(csp->flags & CSP_FLAG_CONTENT_LENGTH_SET);

   if (!strcmpic(csp->http->gpc, "HEAD"))
   {
      /*
       * "HEAD" implies no body, we are thus expecting
       * no content. XXX: incomplete "list" of methods?
       */
      csp->expected_content_length = 0;
      content_length_known = TRUE;
      csp->flags |= CSP_FLAG_SERVER_CONTENT_LENGTH_SET;
   }

   if (csp->http->status == 204 || csp->http->status == 304)
   {
      /*
       * Expect no body. XXX: incomplete "list" of status codes?
       */
      csp->expected_content_length = 0;
      content_length_known = TRUE;
      csp->flags |= CSP_FLAG_SERVER_CONTENT_LENGTH_SET;
   }

   return (content_length_known && ((0 == csp->expected_content_length)
            || (csp->expected_content_length <= content_length)));
}


#ifdef FEATURE_CONNECTION_SHARING
/*********************************************************************
 *
 * Function    :  wait_for_alive_connections
 *
 * Description :  Waits for alive connections to timeout.
 *
 * Parameters  :  N/A
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void wait_for_alive_connections(void)
{
   int connections_alive = close_unusable_connections();

   while (0 < connections_alive)
   {
      log_error(LOG_LEVEL_CONNECT,
         "Waiting for %d connections to timeout.",
         connections_alive);
      sleep(60);
      connections_alive = close_unusable_connections();
   }

   log_error(LOG_LEVEL_CONNECT, "No connections to wait for left.");

}
#endif /* def FEATURE_CONNECTION_SHARING */


/*********************************************************************
 *
 * Function    :  save_connection_destination
 *
 * Description :  Remembers a connection for reuse later on.
 *
 * Parameters  :
 *          1  :  sfd  = Open socket to remember.
 *          2  :  http = The destination for the connection.
 *          3  :  fwd  = The forwarder settings used.
 *          4  :  server_connection  = storage.
 *
 * Returns     : void
 *
 *********************************************************************/
static void save_connection_destination(jb_socket sfd,
                                        const struct http_request *http,
                                        const struct forward_spec *fwd,
                                        struct reusable_connection *server_connection)
{
   assert(sfd != JB_INVALID_SOCKET);
   assert(NULL != http->host);

   server_connection->sfd = sfd;
   server_connection->host = strdup_or_die(http->host);
   server_connection->port = http->port;

   assert(NULL != fwd);
   assert(server_connection->gateway_host == NULL);
   assert(server_connection->gateway_port == 0);
   assert(server_connection->forwarder_type == 0);
   assert(server_connection->forward_host == NULL);
   assert(server_connection->forward_port == 0);

   server_connection->forwarder_type = fwd->type;
   if (NULL != fwd->gateway_host)
   {
      server_connection->gateway_host = strdup_or_die(fwd->gateway_host);
   }
   else
   {
      server_connection->gateway_host = NULL;
   }
   server_connection->gateway_port = fwd->gateway_port;
   if (NULL != fwd->auth_username)
   {
      server_connection->auth_username = strdup_or_die(fwd->auth_username);
   }
   else
   {
      server_connection->auth_username = NULL;
   }
   if (NULL != fwd->auth_password)
   {
      server_connection->auth_password = strdup_or_die(fwd->auth_password);
   }
   else
   {
      server_connection->auth_password = NULL;
   }

   if (NULL != fwd->forward_host)
   {
      server_connection->forward_host = strdup_or_die(fwd->forward_host);
   }
   else
   {
      server_connection->forward_host = NULL;
   }
   server_connection->forward_port = fwd->forward_port;
}
#endif /* FEATURE_CONNECTION_KEEP_ALIVE */


/*********************************************************************
 *
 * Function    : verify_request_length
 *
 * Description : Checks if we already got the whole client requests
 *               and sets CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ if
 *               we do.
 *
 *               Data that doesn't belong to the current request is
 *               either thrown away to let the client retry on a clean
 *               socket, or stashed to be dealt with after the current
 *               request is served.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  void
 *
 *********************************************************************/
static void verify_request_length(struct client_state *csp)
{
   unsigned long long buffered_request_bytes =
      (unsigned long long)(csp->client_iob->eod - csp->client_iob->cur);

   if ((csp->expected_client_content_length != 0)
      && (buffered_request_bytes != 0))
   {
      if (csp->expected_client_content_length >= buffered_request_bytes)
      {
         csp->expected_client_content_length -= buffered_request_bytes;
         log_error(LOG_LEVEL_CONNECT, "Reduced expected bytes to %llu "
            "to account for the %llu ones we already got.",
            csp->expected_client_content_length, buffered_request_bytes);
      }
      else
      {
         assert(csp->client_iob->eod > csp->client_iob->cur + csp->expected_client_content_length);
         csp->client_iob->eod = csp->client_iob->cur + csp->expected_client_content_length;
         log_error(LOG_LEVEL_CONNECT, "Reducing expected bytes to 0. "
            "Marking the server socket tainted after throwing %llu bytes away.",
            buffered_request_bytes - csp->expected_client_content_length);
         csp->expected_client_content_length = 0;
         csp->flags |= CSP_FLAG_SERVER_SOCKET_TAINTED;
      }

      if (csp->expected_client_content_length == 0)
      {
         csp->flags |= CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ;
      }
   }

   if (!(csp->flags & CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ)
      && ((csp->client_iob->cur < csp->client_iob->eod)
         || (csp->expected_client_content_length != 0)))
   {
      if (strcmpic(csp->http->gpc, "GET")
         && strcmpic(csp->http->gpc, "HEAD")
         && strcmpic(csp->http->gpc, "TRACE")
         && strcmpic(csp->http->gpc, "OPTIONS")
         && strcmpic(csp->http->gpc, "DELETE"))
      {
         /* XXX: this is an incomplete hack */
         csp->flags &= ~CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ;
         log_error(LOG_LEVEL_CONNECT, "There better be a request body.");
      }
      else
      {
         csp->flags |= CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ;

         if ((csp->config->feature_flags & RUNTIME_FEATURE_TOLERATE_PIPELINING) == 0)
         {
            csp->flags |= CSP_FLAG_SERVER_SOCKET_TAINTED;
            log_error(LOG_LEVEL_CONNECT,
               "Possible pipeline attempt detected. The connection will not "
               "be kept alive and we will only serve the first request.");
            /* Nuke the pipelined requests from orbit, just to be sure. */
            clear_iob(csp->client_iob);
         }
         else
         {
            /*
             * Keep the pipelined data around for now, we'll deal with
             * it once we're done serving the current request.
             */
            csp->flags |= CSP_FLAG_PIPELINED_REQUEST_WAITING;
            assert(csp->client_iob->eod >= csp->client_iob->cur);
            log_error(LOG_LEVEL_CONNECT, "Complete client request followed by "
               "%d bytes of pipelined data received.",
               (int)(csp->client_iob->eod - csp->client_iob->cur));
         }
      }
   }
   else
   {
      csp->flags |= CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ;
      log_error(LOG_LEVEL_CONNECT, "Complete client request received.");
   }
}


/*********************************************************************
 *
 * Function    :  mark_server_socket_tainted
 *
 * Description :  Makes sure we don't reuse a server socket
 *                (if we didn't read everything the server sent
 *                us reusing the socket would lead to garbage).
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  void.
 *
 *********************************************************************/
static void mark_server_socket_tainted(struct client_state *csp)
{
   /*
    * For consistency we always mark the server socket
    * tainted, however, to reduce the log noise we only
    * emit a log message if the server socket could have
    * actually been reused.
    */
   if ((csp->flags & CSP_FLAG_SERVER_CONNECTION_KEEP_ALIVE)
      && !(csp->flags & CSP_FLAG_SERVER_SOCKET_TAINTED))
   {
      log_error(LOG_LEVEL_CONNECT,
         "Marking the server socket %d tainted.",
         csp->server_connection.sfd);
   }
   csp->flags |= CSP_FLAG_SERVER_SOCKET_TAINTED;
}

/*********************************************************************
 *
 * Function    :  get_request_line
 *
 * Description : Read the client request line.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Pointer to request line or NULL in case of errors.
 *
 *********************************************************************/
static char *get_request_line(struct client_state *csp)
{
   char buf[BUFFER_SIZE];
   char *request_line = NULL;
   int len;

   memset(buf, 0, sizeof(buf));

   if ((csp->flags & CSP_FLAG_PIPELINED_REQUEST_WAITING) != 0)
   {
      /*
       * If there are multiple pipelined requests waiting,
       * the flag will be set again once the next request
       * has been parsed.
       */
      csp->flags &= ~CSP_FLAG_PIPELINED_REQUEST_WAITING;

      request_line = get_header(csp->client_iob);
      if ((NULL != request_line) && ('\0' != *request_line))
      {
         return request_line;
      }
      else
      {
         log_error(LOG_LEVEL_CONNECT, "No complete request line "
            "received yet. Continuing reading from %d.", csp->cfd);
      }
   }

   do
   {
      if (
#ifdef FUZZ
          0 == (csp->flags & CSP_FLAG_FUZZED_INPUT) &&
#endif
          !data_is_available(csp->cfd, csp->config->socket_timeout)
          )
      {
         if (socket_is_still_alive(csp->cfd))
         {
            log_error(LOG_LEVEL_CONNECT,
               "No request line on socket %d received in time. Timeout: %d.",
               csp->cfd, csp->config->socket_timeout);
            write_socket_delayed(csp->cfd, CLIENT_CONNECTION_TIMEOUT_RESPONSE,
               strlen(CLIENT_CONNECTION_TIMEOUT_RESPONSE),
               get_write_delay(csp));
         }
         else
         {
            log_error(LOG_LEVEL_CONNECT,
               "The client side of the connection on socket %d got "
               "closed without sending a complete request line.", csp->cfd);
         }
         return NULL;
      }

      len = read_socket(csp->cfd, buf, sizeof(buf) - 1);

      if (len <= 0) return NULL;

      /*
       * If there is no memory left for buffering the
       * request, there is nothing we can do but hang up
       */
      if (add_to_iob(csp->client_iob, csp->config->buffer_limit, buf, len))
      {
         return NULL;
      }

      request_line = get_header(csp->client_iob);

   } while ((NULL != request_line) && ('\0' == *request_line));

   return request_line;

}

enum chunk_status
{
   CHUNK_STATUS_MISSING_DATA,
   CHUNK_STATUS_BODY_COMPLETE,
   CHUNK_STATUS_PARSE_ERROR
};


/*********************************************************************
 *
 * Function    :  chunked_body_is_complete
 *
 * Description :  Figures out whether or not a chunked body is complete.
 *
 *                Currently it always starts at the beginning of the
 *                buffer which is somewhat wasteful and prevents Privoxy
 *                from starting to forward the correctly parsed chunks
 *                as soon as theoretically possible.
 *
 *                Should be modified to work with a common buffer,
 *                and allow the caller to skip already parsed chunks.
 *
 *                This would allow the function to be used for unbuffered
 *                response bodies as well.
 *
 * Parameters  :
 *          1  :  iob = Buffer with the body to check.
 *          2  :  length = Length of complete body
 *
 * Returns     :  Enum with the result of the check.
 *
 *********************************************************************/
static enum chunk_status chunked_body_is_complete(struct iob *iob, size_t *length)
{
   unsigned int chunksize;
   char *p = iob->cur;

   do
   {
      /*
       * We need at least a single digit, followed by "\r\n",
       * followed by an unknown amount of data, followed by "\r\n".
       */
      if (p + 5 > iob->eod)
      {
         return CHUNK_STATUS_MISSING_DATA;
      }
      if (sscanf(p, "%x", &chunksize) != 1)
      {
         return CHUNK_STATUS_PARSE_ERROR;
      }

      /*
       * We want at least a single digit, followed by "\r\n",
       * followed by the specified amount of data, followed by "\r\n".
       */
      if (p + chunksize + 5 > iob->eod)
      {
         return CHUNK_STATUS_MISSING_DATA;
      }

      /* Skip chunk-size. */
      p = strstr(p, "\r\n");
      if (NULL == p)
      {
         return CHUNK_STATUS_PARSE_ERROR;
      }
      /* Move beyond the chunkdata. */
      p += 2 + chunksize;

      /* Make sure we're still within the buffer and have two bytes left */
      if (p + 2 > iob->eod)
      {
         return CHUNK_STATUS_MISSING_DATA;
      }

      /* There should be another "\r\n" to skip */
      if (memcmp(p, "\r\n", 2))
      {
         return CHUNK_STATUS_PARSE_ERROR;
      }
      p += 2;
   } while (chunksize > 0U);

   *length = (size_t)(p - iob->cur);
   assert(*length <= (size_t)(iob->eod - iob->cur));
   assert(p <= iob->eod);

   return CHUNK_STATUS_BODY_COMPLETE;

}


/*********************************************************************
 *
 * Function    : receive_chunked_client_request_body
 *
 * Description : Read the chunk-encoded client request body.
 *               Failures are dealt with.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_ERR_OK or JB_ERR_PARSE
 *
 *********************************************************************/
static jb_err receive_chunked_client_request_body(struct client_state *csp)
{
   size_t body_length;
   enum chunk_status status;

   while (CHUNK_STATUS_MISSING_DATA ==
      (status = chunked_body_is_complete(csp->client_iob, &body_length)))
   {
      char buf[BUFFER_SIZE];
      int len;

      if (!data_is_available(csp->cfd, csp->config->socket_timeout))
      {
         log_error(LOG_LEVEL_ERROR,
            "Timeout while waiting for the client body.");
         break;
      }
      len = read_socket(csp->cfd, buf, sizeof(buf) - 1);
      if (len <= 0)
      {
         log_error(LOG_LEVEL_ERROR,
            "Reading the client body failed: %E");
         break;
      }
      if (add_to_iob(csp->client_iob, csp->config->buffer_limit, buf, len))
      {
         break;
      }
   }
   if (status != CHUNK_STATUS_BODY_COMPLETE)
   {
      write_socket_delayed(csp->cfd, CLIENT_BODY_PARSE_ERROR_RESPONSE,
         strlen(CLIENT_BODY_PARSE_ERROR_RESPONSE), get_write_delay(csp));
      log_error(LOG_LEVEL_CLF,
         "%s - - [%T] \"Failed reading chunked client body\" 400 0", csp->ip_addr_str);
      return JB_ERR_PARSE;
   }
   log_error(LOG_LEVEL_CONNECT,
      "Chunked client body completely read. Length: %lu", body_length);
   csp->expected_client_content_length = body_length;

   return JB_ERR_OK;

}


#ifdef FUZZ
/*********************************************************************
 *
 * Function    :  fuzz_chunked_transfer_encoding
 *
 * Description :  Treat the fuzzed input as chunked transfer encoding
 *                to check and dechunk.
 *
 * Parameters  :
 *          1  :  csp      = Used to store the data.
 *          2  :  fuzz_input_file = File to read the input from.
 *
 * Returns     : Result of dechunking
 *
 *********************************************************************/
extern int fuzz_chunked_transfer_encoding(struct client_state *csp, char *fuzz_input_file)
{
   size_t length;
   size_t size = (size_t)(csp->iob->eod - csp->iob->cur);
   enum chunk_status status;

   status = chunked_body_is_complete(csp->iob, &length);
   if (CHUNK_STATUS_BODY_COMPLETE != status)
   {
      log_error(LOG_LEVEL_INFO, "Chunked body is incomplete or invalid");
   }
   if (get_bytes_missing_from_chunked_data(csp->iob->cur, size, 0) == 0)
   {
      if (CHUNK_STATUS_BODY_COMPLETE != status)
      {
         log_error(LOG_LEVEL_ERROR,
            "There's disagreement about whether or not the chunked body is complete.");
      }
   }

   return (JB_ERR_OK == remove_chunked_transfer_coding(csp->iob->cur, &size));

}


/*********************************************************************
 *
 * Function    : fuzz_client_request
 *
 * Description : Try to get a client request from the fuzzed input.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  fuzz_input_file = File to read the input from.
 *
 * Returns     :  Result of fuzzing.
 *
 *********************************************************************/
extern int fuzz_client_request(struct client_state *csp, char *fuzz_input_file)
{
   jb_err err;

   csp->cfd = 0;
   csp->ip_addr_str = "fuzzer";

   if (strcmp(fuzz_input_file, "-") != 0)
   {
      log_error(LOG_LEVEL_FATAL,
         "Fuzzed client requests can currently only be read from stdin (-).");
   }
   err = receive_client_request(csp);
   if (err != JB_ERR_OK)
   {
      return 1;
   }
   err = parse_client_request(csp);
   if (err != JB_ERR_OK)
   {
      return 1;
   }

   return 0;

}
#endif  /* def FUZZ */


#ifdef FEATURE_FORCE_LOAD
/*********************************************************************
 *
 * Function    :  force_required
 *
 * Description : Checks a request line to see if it contains
 *               the FORCE_PREFIX. If it does, it is removed
 *               unless enforcing requests has beend disabled.
 *
 * Parameters  :
 *          1  :  request_line = HTTP request line
 *
 * Returns     :  TRUE if force is required, FALSE otherwise.
 *
 *********************************************************************/
static int force_required(const struct client_state *csp, char *request_line)
{
   char *p;

   p = strstr(request_line, "http://");
   if (p != NULL)
   {
      /* Skip protocol */
      p += strlen("http://");
   }
   else
   {
      /* Intercepted request usually don't specify the protocol. */
      p = request_line;
   }

   /* Go to the beginning of the path */
   p = strstr(p, "/");
   if (p == NULL)
   {
      /*
       * If the path is missing the request line is invalid and we
       * are done here. The client-visible rejection happens later on.
       */
      return 0;
   }

   if (0 == strncmpic(p, FORCE_PREFIX, strlen(FORCE_PREFIX) - 1))
   {
      if (!(csp->config->feature_flags & RUNTIME_FEATURE_ENFORCE_BLOCKS))
      {
         /* XXX: Should clean more carefully */
         strclean(request_line, FORCE_PREFIX);
         log_error(LOG_LEVEL_FORCE,
            "Enforcing request: \"%s\".", request_line);

         return 1;
      }
      log_error(LOG_LEVEL_FORCE,
         "Ignored force prefix in request: \"%s\".", request_line);
   }

   return 0;

}
#endif /* def FEATURE_FORCE_LOAD */


/*********************************************************************
 *
 * Function    :  receive_client_request
 *
 * Description : Read the client's request (more precisely the
 *               client headers) and answer it if necessary.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_ERR_OK, JB_ERR_PARSE or JB_ERR_MEMORY
 *
 *********************************************************************/
static jb_err receive_client_request(struct client_state *csp)
{
   char buf[BUFFER_SIZE];
   char *p;
   char *req = NULL;
   struct http_request *http;
   int len;
   jb_err err;

   /* Temporary copy of the client's headers before they get enlisted in csp->headers */
   struct list header_list;
   struct list *headers = &header_list;

   /* We don't care if the arriving data is a valid HTTP request or not. */
   csp->requests_received_total++;

   http = csp->http;

   memset(buf, 0, sizeof(buf));

   req = get_request_line(csp);
   if (req == NULL)
   {
      mark_server_socket_tainted(csp);
      return JB_ERR_PARSE;
   }
   assert(*req != '\0');

   if (client_protocol_is_unsupported(csp, req))
   {
      return JB_ERR_PARSE;
   }

#ifdef FEATURE_FORCE_LOAD
   if (force_required(csp, req))
   {
      csp->flags |= CSP_FLAG_FORCED;
   }
#endif /* def FEATURE_FORCE_LOAD */

   err = parse_http_request(req, http);
   freez(req);
   if (JB_ERR_OK != err)
   {
      write_socket_delayed(csp->cfd, CHEADER, strlen(CHEADER),
         get_write_delay(csp));
      /* XXX: Use correct size */
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"Invalid request\" 400 0", csp->ip_addr_str);
      log_error(LOG_LEVEL_ERROR,
         "Couldn't parse request line received from %s: %s",
         csp->ip_addr_str, jb_err_to_string(err));

      free_http_request(http);
      return JB_ERR_PARSE;
   }
   if (http->ssl && strcmpic(http->gpc, "CONNECT"))
   {
      write_socket_delayed(csp->cfd, CHEADER, strlen(CHEADER),
         get_write_delay(csp));
      /* XXX: Use correct size */
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"Invalid request\" 400 0",
         csp->ip_addr_str);
      log_error(LOG_LEVEL_ERROR, "Client %s tried to send a https "
         "URL without sending a CONNECT request first",
         csp->ip_addr_str);
      free_http_request(http);
      return JB_ERR_PARSE;
   }

   /* grab the rest of the client's headers */
   init_list(headers);
   for (;;)
   {
      p = get_header(csp->client_iob);

      if (p == NULL)
      {
         /* There are no additional headers to read. */
         break;
      }

      if (*p == '\0')
      {
         /*
          * We didn't receive a complete header
          * line yet, get the rest of it.
          */
         if (!data_is_available(csp->cfd, csp->config->socket_timeout))
         {
            log_error(LOG_LEVEL_ERROR,
               "Client headers did not arrive in time. Timeout: %d",
               csp->config->socket_timeout);
            destroy_list(headers);
            return JB_ERR_PARSE;
         }

         len = read_socket(csp->cfd, buf, sizeof(buf) - 1);
         if (len <= 0)
         {
            log_error(LOG_LEVEL_ERROR, "read from client failed: %E");
            destroy_list(headers);
            return JB_ERR_PARSE;
         }

         if (add_to_iob(csp->client_iob, csp->config->buffer_limit, buf, len))
         {
            /*
             * If there is no memory left for buffering the
             * request, there is nothing we can do but hang up
             */
            destroy_list(headers);
            return JB_ERR_MEMORY;
         }
      }
      else
      {
         if (!strncmpic(p, "Transfer-Encoding:", 18))
         {
            /*
             * XXX: should be called through sed()
             *      but currently can't.
             */
            client_transfer_encoding(csp, &p);
         }
         /*
          * We were able to read a complete
          * header and can finally enlist it.
          */
         enlist(headers, p);
         freez(p);
      }
   }

   if (http->host == NULL)
   {
      /*
       * If we still don't know the request destination,
       * the request is invalid or the client uses
       * Privoxy without its knowledge.
       */
      if (JB_ERR_OK != get_request_destination_elsewhere(csp, headers))
      {
         /*
          * Our attempts to get the request destination
          * elsewhere failed or Privoxy is configured
          * to only accept proxy requests.
          *
          * An error response has already been sent
          * and we're done here.
          */
         return JB_ERR_PARSE;
      }
   }

#ifdef FEATURE_CLIENT_TAGS
   /* XXX: If the headers were enlisted sooner, passing csp would do. */
   set_client_address(csp, headers);
   get_tag_list_for_client(csp->client_tags, csp->client_address);
#endif

   /*
    * Determine the actions for this URL
    */
#ifdef FEATURE_TOGGLE
   if (!(csp->flags & CSP_FLAG_TOGGLED_ON))
   {
      /* Most compatible set of actions (i.e. none) */
      init_current_action(csp->action);
   }
   else
#endif /* ndef FEATURE_TOGGLE */
   {
      get_url_actions(csp, http);
   }

   enlist(csp->headers, http->cmd);

   /* Append the previously read headers */
   err = list_append_list_unique(csp->headers, headers);
   destroy_list(headers);

   return err;

}


/*********************************************************************
 *
 * Function    : parse_client_request
 *
 * Description : Parses the client's request and decides what to do
 *               with it.
 *
 *               Note that since we're not using select() we could get
 *               blocked here if a client connected, then didn't say
 *               anything!
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_ERR_OK or JB_ERR_PARSE
 *
 *********************************************************************/
static jb_err parse_client_request(struct client_state *csp)
{
   struct http_request *http = csp->http;
   jb_err err;

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   if ((csp->config->feature_flags & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE)
    && (!strcmpic(csp->http->version, "HTTP/1.1"))
    && (csp->http->ssl == 0))
   {
      /* Assume persistence until further notice */
      csp->flags |= CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
   }
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

   if (csp->http->ssl == 0)
   {
      /*
       * This whole block belongs to chat() but currently
       * has to be executed before sed().
       */
      if (csp->flags & CSP_FLAG_CHUNKED_CLIENT_BODY)
      {
         if (receive_chunked_client_request_body(csp) != JB_ERR_OK)
         {
            return JB_ERR_PARSE;
         }
      }
      else
      {
         csp->expected_client_content_length = get_expected_content_length(csp->headers);
      }
      verify_request_length(csp);
   }
#ifndef FEATURE_HTTPS_INSPECTION
   else
   {
      csp->flags |= CSP_FLAG_SERVER_SOCKET_TAINTED;
   }
#endif

   err = sed(csp, FILTER_CLIENT_HEADERS);
   if (JB_ERR_OK != err)
   {
      log_error(LOG_LEVEL_ERROR, "Failed to parse client request from %s.",
         csp->ip_addr_str);
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 400 0",
         csp->ip_addr_str, csp->http->cmd);
      write_socket_delayed(csp->cfd, CHEADER, strlen(CHEADER), get_write_delay(csp));
      return JB_ERR_PARSE;
   }
   csp->flags |= CSP_FLAG_CLIENT_HEADER_PARSING_DONE;

   /* Check request line for rewrites. */
   if ((NULL == csp->headers->first->str)
      || (strcmp(http->cmd, csp->headers->first->str) &&
         (JB_ERR_OK != change_request_destination(csp))))
   {
      /*
       * A header filter broke the request line - bail out.
       */
      write_socket_delayed(csp->cfd, MESSED_UP_REQUEST_RESPONSE,
         strlen(MESSED_UP_REQUEST_RESPONSE), get_write_delay(csp));
      /* XXX: Use correct size */
      log_error(LOG_LEVEL_CLF,
         "%s - - [%T] \"Invalid request generated\" 400 0", csp->ip_addr_str);
      log_error(LOG_LEVEL_ERROR,
         "Invalid request line after applying header filters.");
      free_http_request(http);

      return JB_ERR_PARSE;
   }

   if (client_has_unsupported_expectations(csp))
   {
      return JB_ERR_PARSE;
   }

   return JB_ERR_OK;

}


/*********************************************************************
 *
 * Function    : read_http_request_body
 *
 * Description : Reads remaining request body from the client.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 on success, anything else is an error.
 *
 *********************************************************************/
static int read_http_request_body(struct client_state *csp)
{
   size_t to_read = csp->expected_client_content_length;
   int len;

   assert(to_read != 0);

   /* check if all data has been already read */
   if (to_read <= (csp->client_iob->eod - csp->client_iob->cur))
   {
      return 0;
   }

   for (to_read -= (size_t)(csp->client_iob->eod - csp->client_iob->cur);
        to_read > 0 && data_is_available(csp->cfd, csp->config->socket_timeout);
        to_read -= (unsigned)len)
   {
      char buf[BUFFER_SIZE];
      size_t max_bytes_to_read = to_read < sizeof(buf) ? to_read : sizeof(buf);

      log_error(LOG_LEVEL_CONNECT,
         "Waiting for up to %lu bytes of request body from the client.",
         max_bytes_to_read);
      len = read_socket(csp->cfd, buf, (int)max_bytes_to_read);
      if (len <= -1)
      {
         log_error(LOG_LEVEL_CONNECT, "Failed receiving request body from %s: %E", csp->ip_addr_str);
         return 1;
      }
      if (add_to_iob(csp->client_iob, csp->config->buffer_limit, (char *)buf, len))
      {
         return 1;
      }
      assert(to_read >= len);
   }

   if (to_read != 0)
   {
      log_error(LOG_LEVEL_CONNECT,
         "Not enough request body has been read: expected %lu more bytes.",
         to_read);
      return 1;
   }
   log_error(LOG_LEVEL_CONNECT,
      "The last %d bytes of the request body have been read.", len);
   return 0;
}


/*********************************************************************
 *
 * Function    : update_client_headers
 *
 * Description : Updates the HTTP headers from the client request.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  new_content_length = new content length value to set
 *
 * Returns     :  0 on success, anything else is an error.
 *
 *********************************************************************/
static int update_client_headers(struct client_state *csp, size_t new_content_length)
{
   static const char content_length[] = "Content-Length:";
   int updated = 0;
   struct list_entry *p;

#ifndef FEATURE_HTTPS_INSPECTION
   for (p = csp->headers->first;
#else
   for (p = csp->http->client_ssl ? csp->https_headers->first : csp->headers->first;
#endif
        !updated  && (p != NULL); p = p->next)
   {
      /* Header crunch()ed in previous run? -> ignore */
      if (p->str == NULL)
      {
         continue;
      }

      /* Does the current parser handle this header? */
      if (0 == strncmpic(p->str, content_length, sizeof(content_length) - 1))
      {
         updated = (JB_ERR_OK == header_adjust_content_length((char **)&(p->str), new_content_length));
         if (!updated)
         {
            return 1;
         }
      }
   }

   return !updated;
}


/*********************************************************************
 *
 * Function    : can_buffer_request_body
 *
 * Description : Checks if the current request body can be stored in
 *               the client_iob without hitting buffer limit.
 *
 * Parameters  :
 *          1  : csp = Current client state (buffers, headers, etc...)
 *
 * Returns     : TRUE if the current request size do not exceed buffer limit
 *               FALSE otherwise.
 *
 *********************************************************************/
static int can_buffer_request_body(const struct client_state *csp)
{
   if (!can_add_to_iob(csp->client_iob, csp->config->buffer_limit,
                       csp->expected_client_content_length))
   {
      log_error(LOG_LEVEL_INFO,
         "Not filtering request body from %s: buffer limit %lu will be exceeded "
         "(content length %llu)", csp->ip_addr_str, csp->config->buffer_limit,
         csp->expected_client_content_length);
      return FALSE;
   }
   return TRUE;
}


/*********************************************************************
 *
 * Function    : send_http_request
 *
 * Description : Sends the HTTP headers from the client request
 *               and all the body data that has already been received.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 on success, 1 on error, 2 if the request got crunched.
 *
 *********************************************************************/
static int send_http_request(struct client_state *csp)
{
   char *hdr;
   int write_failure;

   hdr = list_to_text(csp->headers);
   if (hdr == NULL)
   {
      /* FIXME Should handle error properly */
      log_error(LOG_LEVEL_FATAL, "Out of memory parsing client header");
   }
   list_remove_all(csp->headers);

   /*
    * Write the client's (modified) header to the server
    * (along with anything else that may be in the buffer)
    */
   write_failure = 0 != write_socket(csp->server_connection.sfd, hdr, strlen(hdr));
   freez(hdr);

   if (write_failure)
   {
      log_error(LOG_LEVEL_CONNECT, "Failed sending request headers to: %s: %E",
         csp->http->hostport);
      return 1;
   }

   /* XXX: Filtered data is not sent if there's a pipelined request? */
   if (((csp->flags & CSP_FLAG_PIPELINED_REQUEST_WAITING) == 0)
      && (flush_iob(csp->server_connection.sfd, csp->client_iob, 0) < 0))
   {
      log_error(LOG_LEVEL_CONNECT, "Failed sending request body to: %s: %E",
         csp->http->hostport);
      return 1;
   }
   return 0;
}


#ifdef FEATURE_HTTPS_INSPECTION
/*********************************************************************
 *
 * Function    : read_https_request_body
 *
 * Description : Reads remaining request body from the client.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 on success, anything else is an error.
 *
 *********************************************************************/
static int read_https_request_body(struct client_state *csp)
{
   size_t to_read = csp->expected_client_content_length;
   int len;

   assert(to_read != 0);

   /* check if all data has been already read */
   if (to_read <= (csp->client_iob->eod - csp->client_iob->cur))
   {
      return 0;
   }

   for (to_read -= (size_t)(csp->client_iob->eod - csp->client_iob->cur);
        to_read > 0 && (is_ssl_pending(&(csp->ssl_client_attr)) ||
          data_is_available(csp->cfd, csp->config->socket_timeout));
        to_read -= (unsigned)len)
   {
      unsigned char buf[BUFFER_SIZE];
      size_t max_bytes_to_read = to_read < sizeof(buf) ? to_read : sizeof(buf);

      log_error(LOG_LEVEL_CONNECT,
         "Buffering encrypted client body. Prepared to read up to %lu bytes.",
         max_bytes_to_read);
      len = ssl_recv_data(&(csp->ssl_client_attr), buf,
         (unsigned)max_bytes_to_read);
      if (len <= 0)
      {
         log_error(LOG_LEVEL_CONNECT,
            "Did not receive the whole encrypted request body from %s.",
            csp->ip_addr_str);
         return 1;
      }
      if (add_to_iob(csp->client_iob, csp->config->buffer_limit, (char *)buf, len))
      {
         return 1;
      }
      assert(to_read >= len);
   }

   if (to_read != 0)
   {
      log_error(LOG_LEVEL_CONNECT,
         "Not enough encrypted request body has been read: expected %lu more bytes.",
         to_read);
      return 1;
   }

   log_error(LOG_LEVEL_CONNECT,
      "The last %llu bytes of the encrypted request body have been read.",
      csp->expected_client_content_length);
   return 0;
}


/*********************************************************************
 *
 * Function    : receive_and_send_encrypted_post_data
 *
 * Description : Reads remaining request body from the client and sends
 *               it to the server.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 on success, anything else is an error.
 *
 *********************************************************************/
static int receive_and_send_encrypted_post_data(struct client_state *csp)
{
   int content_length_known = csp->expected_client_content_length != 0;

   while ((content_length_known && csp->expected_client_content_length != 0) ||
      (is_ssl_pending(&(csp->ssl_client_attr)) ||
            data_is_available(csp->cfd, csp->config->socket_timeout)))
   {
      unsigned char buf[BUFFER_SIZE];
      int len;
      int max_bytes_to_read = sizeof(buf);

      if (content_length_known && csp->expected_client_content_length < sizeof(buf))
      {
         max_bytes_to_read = (int)csp->expected_client_content_length;
      }
      log_error(LOG_LEVEL_CONNECT,
         "Prepared to read up to %d bytes of encrypted request body from the client.",
         max_bytes_to_read);
      len = ssl_recv_data(&(csp->ssl_client_attr), buf,
         (unsigned)max_bytes_to_read);
      if (len == -1)
      {
         return 1;
      }
      if (len == 0)
      {
         /* XXX: Does this actually happen? */
         break;
      }
      log_error(LOG_LEVEL_CONNECT, "Forwarding %d bytes of encrypted request body.",
         len);
      len = ssl_send_data(&(csp->ssl_server_attr), buf, (size_t)len);
      if (len == -1)
      {
         return 1;
      }
      if (csp->expected_client_content_length != 0)
      {
         if (csp->expected_client_content_length >= len)
         {
            csp->expected_client_content_length -= (unsigned)len;
         }
         if (csp->expected_client_content_length == 0)
         {
            log_error(LOG_LEVEL_CONNECT, "Forwarded the last %d bytes.", len);
            break;
         }
      }
   }

   log_error(LOG_LEVEL_CONNECT, "Done forwarding encrypted request body.");

   return 0;

}


/*********************************************************************
 *
 * Function    : send_https_request
 *
 * Description : Sends the HTTP headers from the client request
 *               and all the body data that has already been received.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 on success, anything else is an error.
 *
 *********************************************************************/
static int send_https_request(struct client_state *csp)
{
   char *hdr;
   int ret;
   long flushed = 0;

   hdr = list_to_text(csp->https_headers);
   if (hdr == NULL)
   {
      /* FIXME Should handle error properly */
      log_error(LOG_LEVEL_FATAL, "Out of memory parsing client header.");
   }
   list_remove_all(csp->https_headers);

   /*
    * Write the client's (modified) header to the server
    * (along with anything else that may be in the buffer)
    */
   ret = ssl_send_data(&(csp->ssl_server_attr),
      (const unsigned char *)hdr, strlen(hdr));
   freez(hdr);

   if (ret < 0)
   {
      log_error(LOG_LEVEL_CONNECT,
         "Failed sending encrypted request headers to: %s: %E",
         csp->http->hostport);
      mark_server_socket_tainted(csp);
      return 1;
   }

   /* XXX: Client body isn't sent if there's pipelined data? */
   if (((csp->flags & CSP_FLAG_PIPELINED_REQUEST_WAITING) == 0)
      && ((flushed = ssl_flush_socket(&(csp->ssl_server_attr),
            csp->client_iob)) < 0))
   {
      log_error(LOG_LEVEL_CONNECT, "Failed sending request body to: %s: %E",
         csp->http->hostport);
      return 1;
   }
   if (flushed != 0 || csp->expected_client_content_length != 0)
   {
      if (csp->expected_client_content_length != 0)
      {
         if (csp->expected_client_content_length < flushed)
         {
            log_error(LOG_LEVEL_ERROR,
               "Flushed %ld bytes of request body while only expecting %llu.",
               flushed, csp->expected_client_content_length);
            csp->expected_client_content_length = 0;
         }
         else
         {
            log_error(LOG_LEVEL_CONNECT,
               "Flushed %ld bytes of request body while expecting %llu.",
               flushed, csp->expected_client_content_length);
            csp->expected_client_content_length -= (unsigned)flushed;
            if (receive_and_send_encrypted_post_data(csp))
            {
               return 1;
            }
         }
      }
      else
      {
         log_error(LOG_LEVEL_CONNECT,
            "Flushed %ld bytes of request body.", flushed);
      }
   }

   log_error(LOG_LEVEL_CONNECT, "Encrypted request sent.");

   return 0;

}


/*********************************************************************
 *
 * Function    :  receive_encrypted_request_headers
 *
 * Description :  Receives the encrypted request headers when
 *                https-inspecting.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_ERR_OK on success,
 *                JB_ERR_PARSE or JB_ERR_MEMORY otherwise
 *
 *********************************************************************/
static jb_err receive_encrypted_request_headers(struct client_state *csp)
{
   char buf[BUFFER_SIZE];
   int len;
   char *p;

   do
   {
      log_error(LOG_LEVEL_HEADER, "Waiting for encrypted client headers");
      if (!is_ssl_pending(&(csp->ssl_client_attr)) &&
          !data_is_available(csp->cfd, csp->config->socket_timeout))
      {
         log_error(LOG_LEVEL_CONNECT,
            "Socket %d timed out while waiting for client headers", csp->cfd);
         return JB_ERR_PARSE;
      }
      len = ssl_recv_data(&(csp->ssl_client_attr),
         (unsigned char *)buf, sizeof(buf));
      if (len == 0)
      {
         log_error(LOG_LEVEL_CONNECT,
            "Socket %d closed while waiting for client headers", csp->cfd);
         return JB_ERR_PARSE;
      }
      if (len == -1)
      {
         return JB_ERR_PARSE;
      }
      if (add_to_iob(csp->client_iob, csp->config->buffer_limit, buf, len))
      {
         return JB_ERR_MEMORY;
      }
      p = strstr(csp->client_iob->cur, "\r\n\r\n");
   } while (p == NULL);

   log_error(LOG_LEVEL_HEADER, "Encrypted headers received completely");

   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  change_encrypted_request_destination
 *
 * Description :  Parse a (rewritten) request line from an encrypted
 *                request and regenerate the http request data.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Forwards the parse_http_request() return code.
 *                Terminates in case of memory problems.
 *
 *********************************************************************/
static jb_err change_encrypted_request_destination(struct client_state *csp)
{
   jb_err err;
   char *original_host = csp->http->host;
   int original_port = csp->http->port;

   log_error(LOG_LEVEL_REDIRECTS, "Rewrite detected: %s",
      csp->https_headers->first->str);
   csp->http->host = NULL;
   free_http_request(csp->http);
   err = parse_http_request(csp->https_headers->first->str, csp->http);
   if (JB_ERR_OK != err)
   {
      log_error(LOG_LEVEL_ERROR, "Couldn't parse rewritten request: %s.",
         jb_err_to_string(err));
      freez(original_host);
      return err;
   }

   if (csp->http->host == NULL)
   {
      char port_string[10];
      /*
       * The rewritten request line did not specify a host
       * which means we can use the original host specified
       * by the client.
       */
      csp->http->host = original_host;
      csp->http->port = original_port;
      log_error(LOG_LEVEL_REDIRECTS, "Keeping the original host: %s",
         csp->http->host);
      /*
       * If the rewritten request line didn't contain a host
       * it also didn't contain a port so we can reuse the host
       * port.
       */
      freez(csp->http->hostport);
      csp->http->hostport = strdup_or_die(csp->http->host);
      snprintf(port_string, sizeof(port_string), ":%d", original_port);
      err = string_append(&csp->http->hostport, port_string);
      if (err != JB_ERR_OK)
      {
         log_error(LOG_LEVEL_ERROR, "Failed to rebuild hostport: %s.",
            jb_err_to_string(err));
         return err;
      }

      /*
       * While the request line didn't mention it,
       * we're https-inspecting and want to speak TLS
       * with the server.
       */
      csp->http->server_ssl = 1;
      csp->http->ssl = 1;
   }
   else
   {
      /* The rewrite filter added a host so we can ditch the original */
      freez(original_host);
      csp->http->server_ssl = csp->http->ssl;
   }

   csp->http->client_ssl = 1;

   freez(csp->https_headers->first->str);
   build_request_line(csp, NULL, &csp->https_headers->first->str);

   if (!server_use_ssl(csp))
   {
      log_error(LOG_LEVEL_REDIRECTS,
         "Rewritten request line results in downgrade to http");
      /*
       * Replace the unencryptd headers received with the
       * CONNECT request with the ones we received securely.
       */
      destroy_list(csp->headers);
      csp->headers->first = csp->https_headers->first;
      csp->headers->last  = csp->https_headers->last;
      csp->https_headers->first = NULL;
      csp->https_headers->last = NULL;
   }

   return JB_ERR_OK;

}


/*********************************************************************
 *
 * Function    :  process_encrypted_request_headers
 *
 * Description :  Receives and parses the encrypted headers send
 *                by the client when https-inspecting.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_ERR_OK on success,
 *                JB_ERR_PARSE or JB_ERR_MEMORY otherwise
 *
 *********************************************************************/
static jb_err process_encrypted_request_headers(struct client_state *csp)
{
   char *p;
   char *request_line;
   jb_err err;
   /* Temporary copy of the client's headers before they get enlisted in csp->https_headers */
   struct list header_list;
   struct list *headers = &header_list;

   assert(csp->ssl_with_client_is_opened);

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   if (csp->config->feature_flags & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE)
   {
      csp->flags |= CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
   }
#endif
   err = receive_encrypted_request_headers(csp);
   if (err != JB_ERR_OK)
   {
      if (csp->client_iob->cur == NULL ||
          csp->client_iob->cur == csp->client_iob->eod)
      {
         /*
          * We did not receive any data, most likely because the
          * client is done. Don't log this as a parse failure.
          */
         return JB_ERR_PARSE;
      }
      /* XXX: Also used for JB_ERR_MEMORY */
      log_error(LOG_LEVEL_ERROR, "Failed to receive encrypted request: %s",
         jb_err_to_string(err));
      ssl_send_data_delayed(&(csp->ssl_client_attr),
         (const unsigned char *)CHEADER, strlen(CHEADER), get_write_delay(csp));
      return err;
   }

   /* We don't need get_request_line() because the whole HTTP head is buffered. */
   request_line = get_header(csp->client_iob);
   if (request_line == NULL)
   {
      log_error(LOG_LEVEL_ERROR, "Failed to get the encrypted request line");
      ssl_send_data_delayed(&(csp->ssl_client_attr),
         (const unsigned char *)CHEADER, strlen(CHEADER), get_write_delay(csp));
      return JB_ERR_PARSE;
   }
   assert(*request_line != '\0');

   if (client_protocol_is_unsupported(csp, request_line))
   {
      /*
       * If the protocol is unsupported we're done here.
       * client_protocol_is_unsupported() took care of sending
       * the error response and logging the error message.
       */
      return JB_ERR_PARSE;
   }

#ifdef FEATURE_FORCE_LOAD
   if (force_required(csp, request_line))
   {
      csp->flags |= CSP_FLAG_FORCED;
   }
#endif /* def FEATURE_FORCE_LOAD */

   free_http_request(csp->http);

   err = parse_http_request(request_line, csp->http);
   /* XXX: Restore ssl setting. This is ugly */
   csp->http->client_ssl = 1;
   csp->http->server_ssl = 1;

   freez(request_line);
   if (JB_ERR_OK != err)
   {
      ssl_send_data_delayed(&(csp->ssl_client_attr),
         (const unsigned char *)CHEADER, strlen(CHEADER), get_write_delay(csp));
      /* XXX: Use correct size */
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"Invalid request\" 400 0", csp->ip_addr_str);
      log_error(LOG_LEVEL_ERROR,
         "Couldn't parse request line received from %s: %s",
         csp->ip_addr_str, jb_err_to_string(err));

      free_http_request(csp->http);
      return JB_ERR_PARSE;
   }

   /* Parse the rest of the client's headers. */
   init_list(headers);
   for (;;)
   {
      p = get_header(csp->client_iob);

      if (p == NULL)
      {
         /* There are no additional headers to read. */
         break;
      }
      enlist(headers, p);
      freez(p);
   }

   if (JB_ERR_OK != get_destination_from_https_headers(headers, csp->http))
   {
      /*
       * Our attempts to get the request destination
       * elsewhere failed.
       */
      log_error(LOG_LEVEL_ERROR,
         "Failed to get the encrypted request destination");
      ssl_send_data_delayed(&(csp->ssl_client_attr),
         (const unsigned char *)CHEADER, strlen(CHEADER), get_write_delay(csp));
      destroy_list(headers);

      return JB_ERR_PARSE;
   }

   /* Split the domain we just got for pattern matching */
   init_domain_components(csp->http);

#ifdef FEATURE_CLIENT_TAGS
   /* XXX: If the headers were enlisted sooner, passing csp would do. */
   if (csp->client_address == NULL)
   {
      set_client_address(csp, headers);
      get_tag_list_for_client(csp->client_tags, csp->client_address);
   }
#endif

#ifdef FEATURE_TOGGLE
   if ((csp->flags & CSP_FLAG_TOGGLED_ON) != 0)
#endif
   {
      /*
       * Determine the actions for this request after
       * clearing the ones from the previous one.
       */
      free_current_action(csp->action);
      get_url_actions(csp, csp->http);
   }

   enlist(csp->https_headers, csp->http->cmd);

   /* Append the previously read headers */
   err = list_append_list_unique(csp->https_headers, headers);
   destroy_list(headers);
   if (JB_ERR_OK != err)
   {
      /* XXX: Send error message */
      return err;
   }

   /* XXX: Work around crash */
   csp->error_message = NULL;

   /* XXX: Why do this here? */
   csp->http->ssl = 1;

   err = sed_https(csp);
   if (JB_ERR_OK != err)
   {
      ssl_send_data_delayed(&(csp->ssl_client_attr),
         (const unsigned char *)CHEADER, strlen(CHEADER), get_write_delay(csp));
      log_error(LOG_LEVEL_ERROR, "Failed to parse client request from %s.",
         csp->ip_addr_str);
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 400 0",
         csp->ip_addr_str, csp->http->cmd);
      return JB_ERR_PARSE;
   }

   if ((NULL == csp->https_headers->first->str)
      || (strcmp(csp->http->cmd, csp->https_headers->first->str) &&
         (JB_ERR_OK != change_encrypted_request_destination(csp))))
   {
      ssl_send_data_delayed(&(csp->ssl_client_attr),
         (const unsigned char *)MESSED_UP_REQUEST_RESPONSE,
         strlen(MESSED_UP_REQUEST_RESPONSE), get_write_delay(csp));
      log_error(LOG_LEVEL_ERROR,
         "Invalid request line after applying header filters.");
      /* XXX: Use correct size */
      log_error(LOG_LEVEL_CLF,
         "%s - - [%T] \"Invalid request generated\" 400 0", csp->ip_addr_str);

      return JB_ERR_PARSE;
   }

   log_error(LOG_LEVEL_HEADER, "Encrypted request headers processed");
   log_error(LOG_LEVEL_REQUEST, "https://%s%s", csp->http->hostport,
      csp->http->path);

   return err;

}

/*********************************************************************
 *
 * Function    :  cgi_page_requested
 *
 * Description :  Checks if a request is for an internal CGI page.
 *
 * Parameters  :
 *          1  :  host = The host requested by the client.
 *
 * Returns     :  1 if a CGI page has been requested, 0 otherwise
 *
 *********************************************************************/
static int cgi_page_requested(const char *host)
{
   if ((0 == strcmpic(host, CGI_SITE_1_HOST))
    || (0 == strcmpic(host, CGI_SITE_1_HOST "."))
    || (0 == strcmpic(host, CGI_SITE_2_HOST))
    || (0 == strcmpic(host, CGI_SITE_2_HOST ".")))
   {
      return 1;
   }

   return 0;

}


#ifdef FEATURE_CONNECTION_KEEP_ALIVE
/*********************************************************************
 *
 * Function    :  continue_https_chat
 *
 * Description :  Behaves similar to chat() but only deals with
 *                https-inspected requests that arrive on an already
 *                established connection. The first request is always
 *                served by chat() which is a lot more complex as it
 *                has to deal with forwarding settings and connection
 *                failures etc.
 *
 *                If a connection to the server has already been
 *                opened it is reused unless the request is blocked
 *                or the forwarder changed.
 *
 *                If a connection to the server has not yet been
 *                opened (because the previous request was crunched),
 *                or the forwarder changed, the connection is dropped
 *                so that the client retries on a fresh one.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Nothing.
 *
 *********************************************************************/
static void continue_https_chat(struct client_state *csp)
{
   const struct forward_spec *fwd;

   if (JB_ERR_OK != process_encrypted_request_headers(csp))
   {
      csp->flags &= ~CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
      return;
   }

#if defined(FEATURE_STATISTICS) && defined(MUTEX_LOCKS_AVAILABLE)
   privoxy_mutex_lock(&block_statistics_mutex);
   number_of_requests_received++;
   privoxy_mutex_unlock(&block_statistics_mutex);
#endif

   csp->requests_received_total++;

   /*
    * We have an encrypted request. Check if one of the crunchers wants it.
    */
   if (crunch_response_triggered(csp, crunchers_all))
   {
      /*
       * Yes. The client got the crunch response and we're done here.
       */
      return;
   }
   if (csp->ssl_with_server_is_opened == 0)
   {
      log_error(LOG_LEVEL_CONNECT,
         "Dropping the client connection on socket %d. "
         "The server connection has not been established yet.",
         csp->cfd);
      csp->flags &= ~CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
      return;
   }
   assert(csp->server_connection.sfd != JB_INVALID_SOCKET);

   if (csp->expected_client_content_length != 0 &&
      (client_body_filters_enabled(csp->action) ||
       client_body_taggers_enabled(csp->action)) &&
      can_buffer_request_body(csp))
   {
      int content_modified;

      if (read_https_request_body(csp))
      {
         /* XXX: handle */
         return;
      }
      if (client_body_taggers_enabled(csp->action))
      {
         execute_client_body_taggers(csp, csp->expected_client_content_length);
         if (crunch_response_triggered(csp, crunchers_all))
         {
            /*
             * Yes. The client got the crunch response and we're done here.
             */
            return;
         }
      }
      if (client_body_filters_enabled(csp->action))
      {
         size_t modified_content_length = csp->expected_client_content_length;
         content_modified = execute_client_body_filters(csp,
            &modified_content_length);
         if ((content_modified == 1) &&
            (modified_content_length != csp->expected_client_content_length) &&
            update_client_headers(csp, modified_content_length))
         {
            /* XXX: Send error response */
            log_error(LOG_LEVEL_HEADER, "Error updating client headers");
            return;
         }
      }
      csp->expected_client_content_length = 0;
   }

   fwd = forward_url(csp, csp->http);
   if (!connection_destination_matches(&csp->server_connection, csp->http, fwd))
   {
      log_error(LOG_LEVEL_CONNECT,
         "Dropping the client connection on socket %d with "
         "server socket %d connected to %s. The forwarder has changed.",
         csp->cfd, csp->server_connection.sfd, csp->server_connection.host);
      csp->flags &= ~CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
      return;
   }

   log_applied_actions(csp->action);

   log_error(LOG_LEVEL_CONNECT,
      "Reusing server socket %d connected to %s. Requests already sent: %u.",
      csp->server_connection.sfd, csp->server_connection.host,
      csp->server_connection.requests_sent_total);

   if (send_https_request(csp))
   {
      /*
       * Most likely the server connection timed out. We can't easily
       * create a new one so simply drop the client connection without a
       * error response to let the client retry.
       */
      log_error(LOG_LEVEL_CONNECT,
         "Dropping client connection on socket %d. "
         "Forwarding the encrypted client request failed.",
         csp->cfd);
      return;
   }
   csp->server_connection.request_sent = time(NULL);
   csp->server_connection.requests_sent_total++;
   handle_established_connection(csp);
   freez(csp->receive_buffer);
}
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */
#endif


/*********************************************************************
 *
 * Function    :  send_server_headers
 *
 * Description :  Sends the server headers to the client.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 on succes, -1 on error.
 *
 *********************************************************************/
static int send_server_headers(struct client_state *csp)
{
   char *server_headers;
   int ret;

   server_headers = list_to_text(csp->headers);
   if (server_headers == NULL)
   {
      /*
       * Memory is too tight to even generate the header.
       * Send our static "Out-of-memory" page.
       */
      log_error(LOG_LEVEL_ERROR,
         "Out of memory while trying to send server headers.");
      send_crunch_response(csp, cgi_error_memory());
      mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
      close_client_and_server_ssl_connections(csp);
#endif
      return -1;
   }
#ifdef FEATURE_HTTPS_INSPECTION
   if (client_use_ssl(csp))
   {
      ret = ssl_send_data_delayed(&(csp->ssl_client_attr),
         (const unsigned char *)server_headers, strlen(server_headers),
         get_write_delay(csp));
   }
   else
#endif
   {
      ret = write_socket_delayed(csp->cfd, server_headers, strlen(server_headers),
         get_write_delay(csp));
   }
   freez(server_headers);
   if (ret < 0)
   {
      log_error(LOG_LEVEL_ERROR, "Failed to send server headers to the client.");
      mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
      close_client_and_server_ssl_connections(csp);
#endif
      return -1;
   }

   return 0;

}

/*********************************************************************
 *
 * Function    :  handle_established_connection
 *
 * Description :  Shuffle data between client and server once the
 *                connection has been established and the request
 *                has been sent.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Nothing.
 *
 *********************************************************************/
static void handle_established_connection(struct client_state *csp)
{
   char *p;
   int n;
#ifdef HAVE_POLL
   struct pollfd poll_fds[2];
#else
   fd_set rfds;
   jb_socket maxfd;
   struct timeval timeout;
#endif
   int server_body;
   int ms_iis5_hack = 0;
   unsigned long long byte_count = 0;
   struct http_request *http;
   long len = 0; /* for buffer sizes (and negative error codes) */
   int buffer_and_filter_content = 0;
   unsigned int write_delay;

   /* Skeleton for HTTP response, if we should intercept the request */
   struct http_response *rsp;
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   int watch_client_socket;
#endif

   size_t chunk_offset = 0;
#ifdef FEATURE_HTTPS_INSPECTION
   int ret = 0;
   int use_ssl_tunnel = 0;
   csp->dont_verify_certificate = 0;

   if (csp->http->ssl && !(csp->action->flags & ACTION_HTTPS_INSPECTION))
   {
      /* Pass encrypted content without filtering. */
      use_ssl_tunnel = 1;
   }
#endif

   csp->receive_buffer_size = csp->config->receive_buffer_size;
   csp->receive_buffer = zalloc(csp->receive_buffer_size + 1);
   if (csp->receive_buffer == NULL)
   {
      log_error(LOG_LEVEL_ERROR,
         "Out of memory. Failed to allocate the receive buffer.");
      rsp = cgi_error_memory();
      send_crunch_response(csp, rsp);
      return;
   }

   http = csp->http;

#ifndef HAVE_POLL
   maxfd = (csp->cfd > csp->server_connection.sfd) ?
      csp->cfd : csp->server_connection.sfd;
#endif

   /* pass data between the client and server
    * until one or the other shuts down the connection.
    */

   server_body = 0;

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   watch_client_socket = 0 == (csp->flags & CSP_FLAG_PIPELINED_REQUEST_WAITING);
#endif
   write_delay = get_write_delay(csp);

   for (;;)
   {
#ifdef FEATURE_HTTPS_INSPECTION
      if (server_use_ssl(csp) && is_ssl_pending(&(csp->ssl_server_attr)))
      {
         /*
          * It's possible that the TLS library already consumed all the
          * data the server intends to send. If that happens poll() and
          * select() will no longer see the data as available so we have
          * to skip the calls.
          */
         goto server_wants_to_talk;
      }
      if (watch_client_socket && client_use_ssl(csp) &&
         is_ssl_pending(&(csp->ssl_client_attr)))
      {
         /*
          * The TLS libray may also consume all of the remaining data
          * from the client when we're shuffling the data from an
          * unbuffered request body to the server.
          */
         goto client_wants_to_talk;
      }
#endif
#ifndef HAVE_POLL
      FD_ZERO(&rfds);
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
      if (!watch_client_socket)
      {
         maxfd = csp->server_connection.sfd;
      }
      else
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */
      {
         FD_SET(csp->cfd, &rfds);
      }

      FD_SET(csp->server_connection.sfd, &rfds);
#endif /* ndef HAVE_POLL */

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
      if (server_body && server_response_is_complete(csp, byte_count))
      {
         if (csp->expected_content_length == byte_count)
         {
            log_error(LOG_LEVEL_CONNECT,
               "Done reading from server. Content length: %llu as expected. "
               "Bytes most recently read: %ld.",
               byte_count, len);
         }
         else
         {
            log_error(LOG_LEVEL_CONNECT,
               "Done reading from server. Expected content length: %llu. "
               "Actual content length: %llu. Bytes most recently read: %ld.",
               csp->expected_content_length, byte_count, len);
         }
         len = 0;
         /*
          * XXX: Should not jump around, handle_established_connection()
          * is complicated enough already.
          */
         goto reading_done;
      }
#endif  /* FEATURE_CONNECTION_KEEP_ALIVE */

#ifdef HAVE_POLL
      poll_fds[0].fd = csp->cfd;
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
      if (!watch_client_socket)
      {
         /*
          * Ignore incoming data, but still watch out
          * for disconnects etc. These flags are always
          * implied anyway but explicitly setting them
          * doesn't hurt.
          */
         poll_fds[0].events = POLLERR|POLLHUP;
      }
      else
#endif
      {
         poll_fds[0].events = POLLIN;
      }
      poll_fds[1].fd = csp->server_connection.sfd;
      poll_fds[1].events = POLLIN;
      n = poll(poll_fds, 2, csp->config->socket_timeout * 1000);
#else
      timeout.tv_sec = csp->config->socket_timeout;
      timeout.tv_usec = 0;
      n = select((int)maxfd + 1, &rfds, NULL, NULL, &timeout);
#endif /* def HAVE_POLL */

      /* Server or client not responding in timeout */
      if (n == 0)
      {
         log_error(LOG_LEVEL_CONNECT, "Socket timeout %d reached: %s",
            csp->config->socket_timeout, http->url);
         if ((byte_count == 0) && (http->ssl == 0))
         {
            send_crunch_response(csp, error_response(csp, "connection-timeout"));
         }
         mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
         close_client_and_server_ssl_connections(csp);
#endif
         return;
      }
      else if (n < 0)
      {
#ifdef HAVE_POLL
         log_error(LOG_LEVEL_ERROR, "poll() failed!: %E");
#else
         log_error(LOG_LEVEL_ERROR, "select() failed!: %E");
#endif
         mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
         close_client_and_server_ssl_connections(csp);
#endif
         return;
      }

      /*
       * This is the body of the browser's request,
       * just read and write it.
       *
       * Receives data from browser and sends it to server
       *
       * XXX: Make sure the client doesn't use pipelining
       * behind Privoxy's back.
       */
#ifdef HAVE_POLL
      if ((poll_fds[0].revents & (POLLERR|POLLHUP|POLLNVAL)) != 0)
      {
         log_error(LOG_LEVEL_CONNECT,
            "The client socket %d has become unusable while "
            "the server socket %d is still open.",
            csp->cfd, csp->server_connection.sfd);
         mark_server_socket_tainted(csp);
         break;
      }

      if (poll_fds[0].revents != 0)
#else
      if (FD_ISSET(csp->cfd, &rfds))
#endif /* def HAVE_POLL*/
      {
         int max_bytes_to_read;

#ifdef FEATURE_HTTPS_INSPECTION
         client_wants_to_talk:
#endif

         max_bytes_to_read = (int)csp->receive_buffer_size;

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
         if ((csp->flags & CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ))
         {
            if (data_is_available(csp->cfd, 0))
            {
               /*
                * If the next request is already waiting, we have
                * to stop select()ing the client socket. Otherwise
                * we would always return right away and get nothing
                * else done.
                */
               watch_client_socket = 0;
               log_error(LOG_LEVEL_CONNECT,
                  "Stop watching client socket %d. "
                  "There's already another request waiting.",
                  csp->cfd);
               continue;
            }
            /*
             * If the client socket is set, but there's no data
             * available on the socket, the client went fishing
             * and continuing talking to the server makes no sense.
             */
            log_error(LOG_LEVEL_CONNECT,
               "The client closed socket %d while "
               "the server socket %d is still open.",
               csp->cfd, csp->server_connection.sfd);
            mark_server_socket_tainted(csp);
            break;
         }
         if (csp->expected_client_content_length != 0)
         {
            if (csp->expected_client_content_length < csp->receive_buffer_size)
            {
               max_bytes_to_read = (int)csp->expected_client_content_length;
            }
            log_error(LOG_LEVEL_CONNECT,
               "Waiting for up to %d bytes from the client.",
               max_bytes_to_read);
         }
         assert(max_bytes_to_read <= csp->receive_buffer_size);
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

#ifdef FEATURE_HTTPS_INSPECTION
         if (client_use_ssl(csp))
         {
            if (csp->http->status == 101)
            {
               len = ssl_recv_data(&(csp->ssl_client_attr),
                  (unsigned char *)csp->receive_buffer,
                  (size_t)max_bytes_to_read);
               if (len == -1)
               {
                  log_error(LOG_LEVEL_ERROR, "Failed to receive data "
                     "on client socket %d for an upgraded connection",
                     csp->cfd);
                  break;
               }
               if (len == 0)
               {
                  log_error(LOG_LEVEL_CONNECT, "Done receiving data "
                     "on client socket %d for an upgraded connection",
                     csp->cfd);
                  break;
               }
               byte_count += (unsigned long long)len;
               len = ssl_send_data(&(csp->ssl_server_attr),
                  (unsigned char *)csp->receive_buffer, (size_t)len);
               if (len == -1)
               {
                  log_error(LOG_LEVEL_ERROR, "Failed to send data "
                     "on server socket %d for an upgraded connection",
                     csp->server_connection.sfd);
                  break;
               }
               continue;
            }
            log_error(LOG_LEVEL_CONNECT, "Breaking with TLS/SSL.");
            break;
         }
         else
#endif /* def FEATURE_HTTPS_INSPECTION */
         {
            len = read_socket(csp->cfd, csp->receive_buffer, max_bytes_to_read);

            if (len <= 0)
            {
               /* XXX: not sure if this is necessary. */
               mark_server_socket_tainted(csp);
               break; /* "game over, man" */
            }

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
            if (csp->expected_client_content_length != 0)
            {
               assert(len <= max_bytes_to_read);
               csp->expected_client_content_length -= (unsigned)len;
               log_error(LOG_LEVEL_CONNECT,
                  "Expected client content length set to %llu "
                  "after reading %ld bytes.",
                  csp->expected_client_content_length, len);
               if (csp->expected_client_content_length == 0)
               {
                  log_error(LOG_LEVEL_CONNECT,
                     "Done reading from the client.");
                  csp->flags |= CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ;
               }
            }
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

            if (write_socket(csp->server_connection.sfd, csp->receive_buffer, (size_t)len))
            {
               log_error(LOG_LEVEL_ERROR, "write to: %s failed: %E", http->host);
               mark_server_socket_tainted(csp);
               return;
            }
         }
         continue;
      }

      /*
       * The server wants to talk. It could be the header or the body.
       */
#ifdef HAVE_POLL
      if (poll_fds[1].revents != 0)
#else
      if (FD_ISSET(csp->server_connection.sfd, &rfds))
#endif /* HAVE_POLL */
      {
#ifdef FEATURE_HTTPS_INSPECTION
         server_wants_to_talk:
#endif
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
         /*
          * If we are buffering content, we don't want to eat up to
          * buffer-limit bytes if the client no longer cares about them.
          * If we aren't buffering, however, a dead client socket will be
          * noticed pretty much right away anyway, so we can reduce the
          * overhead by skipping the check.
          */
         if (buffer_and_filter_content && !socket_is_still_alive(csp->cfd))
         {
#ifdef _WIN32
            log_error(LOG_LEVEL_CONNECT,
               "The server still wants to talk, but the client may already have hung up on us.");
#else
            log_error(LOG_LEVEL_CONNECT,
               "The server still wants to talk, but the client hung up on us.");
            mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
            close_client_and_server_ssl_connections(csp);
#endif
            return;
#endif /* def _WIN32 */
         }
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

#ifdef FEATURE_HTTPS_INSPECTION
         /*
          * Reading data from standard or secured connection (HTTP/HTTPS)
          */
         if (server_use_ssl(csp))
         {
            len = ssl_recv_data(&(csp->ssl_server_attr),
               (unsigned char *)csp->receive_buffer, csp->receive_buffer_size);
         }
         else
#endif
         {
            len = read_socket(csp->server_connection.sfd, csp->receive_buffer,
               (int)csp->receive_buffer_size);
         }

         if (len < 0)
         {
            log_error(LOG_LEVEL_ERROR, "read from: %s failed: %E", http->host);

            if ((http->ssl && (csp->fwd == NULL))
#ifdef FEATURE_HTTPS_INSPECTION
               && use_ssl_tunnel
#endif
                )
            {
               /*
                * Just hang up. We already confirmed the client's CONNECT
                * request with status code 200 and unencrypted content is
                * no longer welcome.
                */
               log_error(LOG_LEVEL_ERROR,
                  "CONNECT already confirmed. Unable to tell the client about the problem.");
               return;
            }
            else if (byte_count)
            {
               /*
                * Just hang up. We already transmitted the original headers
                * and parts of the original content and therefore missed the
                * chance to send an error message (without risking data corruption).
                *
                * XXX: we could retry with a fancy range request here.
                */
               log_error(LOG_LEVEL_ERROR, "Already forwarded the original headers. "
                  "Unable to tell the client about the problem.");
               mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
               close_client_and_server_ssl_connections(csp);
#endif
               return;
            }
            /*
             * XXX: Consider handling the cases above the same.
             */
            mark_server_socket_tainted(csp);
            len = 0;
         }

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
         reading_done:
#endif  /* FEATURE_CONNECTION_KEEP_ALIVE */

         /*
          * This is guaranteed by allocating with zalloc_or_die()
          * and never (intentionally) writing to the last byte.
          *
          * csp->receive_buffer_size is the size of the part of the
          * buffer we intentionally write to, but we actually
          * allocated csp->receive_buffer_size+1 bytes so the assertion
          * stays within the allocated range.
          */
         assert(csp->receive_buffer[csp->receive_buffer_size] == '\0');

         /*
          * Add a trailing zero to let be able to use string operations.
          * XXX: do we still need this with filter_popups gone?
          */
         assert(len <= csp->receive_buffer_size);
         csp->receive_buffer[len] = '\0';

         /*
          * Normally, this would indicate that we've read
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
         if (len == 0)
         {

            if (server_body || (http->ssl
#ifdef FEATURE_HTTPS_INSPECTION
                  && use_ssl_tunnel
#endif
               ))
            {
               /*
                * If we have been buffering up the document,
                * now is the time to apply content modification
                * and send the result to the client.
                */
               if (buffer_and_filter_content)
               {
                  p = execute_content_filters(csp);
                  /*
                   * If content filtering fails, use the original
                   * buffer and length.
                   * (see p != NULL ? p : csp->iob->cur below)
                   */
                  if (NULL == p)
                  {
                     csp->content_length = (size_t)(csp->iob->eod - csp->iob->cur);
                  }
#ifdef FEATURE_COMPRESSION
                  else if ((csp->flags & CSP_FLAG_CLIENT_SUPPORTS_DEFLATE)
                     && (csp->content_length > LOWER_LENGTH_LIMIT_FOR_COMPRESSION))
                  {
                     char *compressed_content = compress_buffer(p,
                        (size_t *)&csp->content_length, csp->config->compression_level);
                     if (compressed_content != NULL)
                     {
                        freez(p);
                        p = compressed_content;
                        csp->flags |= CSP_FLAG_BUFFERED_CONTENT_DEFLATED;
                     }
                  }
#endif

                  if (JB_ERR_OK != update_server_headers(csp))
                  {
                     log_error(LOG_LEVEL_FATAL,
                        "Failed to update server headers. after filtering.");
                  }

                  if (send_server_headers(csp))
                  {
                     return;
                  }

#ifdef FEATURE_HTTPS_INSPECTION
                  /*
                   * Sending data with standard or secured connection (HTTP/HTTPS)
                   */
                  if (client_use_ssl(csp))
                  {
                     if (ssl_send_data_delayed(&(csp->ssl_client_attr),
                              (const unsigned char *) ((p != NULL) ? p : csp->iob->cur),
                              csp->content_length, get_write_delay(csp)) < 0)
                     {
                        log_error(LOG_LEVEL_ERROR,
                           "Failed to send the modified content to the client over TLS");
                        freez(p);
                        mark_server_socket_tainted(csp);
                        close_client_and_server_ssl_connections(csp);
                        return;
                     }
                  }
                  else
#endif /* def FEATURE_HTTPS_INSPECTION */
                  {
                     if (write_socket_delayed(csp->cfd, ((p != NULL) ? p : csp->iob->cur),
                         (size_t)csp->content_length, write_delay))
                     {
                        log_error(LOG_LEVEL_ERROR, "write modified content to client failed: %E");
                        freez(p);
                        mark_server_socket_tainted(csp);
                        return;
                     }
                  }

                  freez(p);
               }

               break; /* "game over, man" */
            }

            /*
             * This is not the body, so let's pretend the server just sent
             * us a blank line.
             */
            snprintf(csp->receive_buffer, csp->receive_buffer_size, "\r\n");
            len = (int)strlen(csp->receive_buffer);

            /*
             * Now, let the normal header parsing algorithm below do its
             * job.  If it fails, we'll exit instead of continuing.
             */

            ms_iis5_hack = 1;
         }

         /*
          * If we're in the body of the server document, just write it to
          * the client, unless we need to buffer the body for later
          * content-filtering.
          */
         if (server_body || (http->ssl
#ifdef FEATURE_HTTPS_INSPECTION
               && use_ssl_tunnel
#endif
            ))
         {
            if (buffer_and_filter_content)
            {
               /*
                * If there is no memory left for buffering the content, or the buffer limit
                * has been reached, switch to non-filtering mode, i.e. make & write the
                * header, flush the iob and buf, and get out of the way.
                */
               if (add_to_iob(csp->iob, csp->config->buffer_limit, csp->receive_buffer, len))
               {
                  long flushed;

                  log_error(LOG_LEVEL_INFO,
                     "Flushing header and buffers. Stepping back from filtering.");

                  if (send_server_headers(csp))
                  {
                     return;
                  }

#ifdef FEATURE_HTTPS_INSPECTION
                  /*
                   * Sending data with standard or secured connection (HTTP/HTTPS)
                   */
                  if (client_use_ssl(csp))
                  {
                     if (((flushed = ssl_flush_socket(&(csp->ssl_client_attr),
                                csp->iob)) < 0)
                        || (ssl_send_data_delayed(&(csp->ssl_client_attr),
                              (const unsigned char *)csp->receive_buffer, (size_t)len,
                              get_write_delay(csp)) < 0))
                     {
                        log_error(LOG_LEVEL_CONNECT,
                           "Flush header and buffers to client failed");
                        mark_server_socket_tainted(csp);
                        close_client_and_server_ssl_connections(csp);
                        return;
                     }
                  }
                  else
#endif /* def FEATURE_HTTPS_INSPECTION */
                  {
                     if (((flushed = flush_iob(csp->cfd, csp->iob, write_delay)) < 0)
                      || write_socket_delayed(csp->cfd, csp->receive_buffer, (size_t)len,
                            write_delay))
                     {
                        log_error(LOG_LEVEL_CONNECT,
                           "Flush header and buffers to client failed: %E");
                        mark_server_socket_tainted(csp);
                        return;
                     }
                  }

                  /*
                   * Reset the byte_count to the amount of bytes we just
                   * flushed. len will be added a few lines below.
                   */
                  byte_count = (unsigned long long)flushed;
                  if ((csp->flags & CSP_FLAG_CHUNKED) && (chunk_offset != 0))
                  {
                     log_error(LOG_LEVEL_CONNECT,
                        "Reducing chunk offset %lu by %ld to %lu.", chunk_offset, flushed,
                        (chunk_offset - (unsigned)flushed));
                     assert(chunk_offset >= flushed); /* XXX: Reachable with malicious input? */
                     chunk_offset -= (unsigned)flushed;

                     /* Make room in the iob. */
                     csp->iob->cur = csp->iob->eod = csp->iob->buf;

                     if (add_to_iob(csp->iob, csp->config->buffer_limit,
                           csp->receive_buffer, len))
                     {
                        /* This is not supposed to happen but ... */
                        csp->flags &= ~CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
                        log_error(LOG_LEVEL_ERROR, "Failed to buffer %ld bytes of "
                           "chunk-encoded data after resetting the buffer.", len);
                        return;
                     }
                  }
                  buffer_and_filter_content = 0;
                  server_body = 1;
               }
            }
            else
            {
#ifdef FEATURE_HTTPS_INSPECTION
               /*
                * Sending data with standard or secured connection (HTTP/HTTPS)
                */
               if (client_use_ssl(csp))
               {
                  ret = ssl_send_data_delayed(&(csp->ssl_client_attr),
                     (const unsigned char *)csp->receive_buffer, (size_t)len,
                     get_write_delay(csp));
                  if (ret < 0)
                  {
                     log_error(LOG_LEVEL_ERROR,
                        "Sending data to client failed");
                     mark_server_socket_tainted(csp);
                     close_client_and_server_ssl_connections(csp);
                     return;
                  }
               }
               else
#endif /* def FEATURE_HTTPS_INSPECTION */
               {
                  if (write_socket_delayed(csp->cfd, csp->receive_buffer,
                        (size_t)len, write_delay))
                  {
                     log_error(LOG_LEVEL_ERROR, "write to client failed: %E");
                     mark_server_socket_tainted(csp);
                     return;
                  }
               }
               if (csp->flags & CSP_FLAG_CHUNKED)
               {
                  /*
                   * While we don't need the data to filter it, put it in the
                   * buffer so we can keep track of the offset to the start of
                   * the next chunk and detect when the response is finished.
                   */
                  size_t encoded_bytes = (size_t)(csp->iob->eod - csp->iob->cur);

                  if (csp->config->buffer_limit / 4 < encoded_bytes)
                  {
                     /*
                      * Reset the buffer to reduce the memory footprint.
                      */
                     log_error(LOG_LEVEL_CONNECT,
                        "Reducing the chunk offset from %lu to %lu after "
                        "discarding %lu bytes to make room in the buffer.",
                        chunk_offset, (chunk_offset - encoded_bytes),
                        encoded_bytes);
                     chunk_offset -= encoded_bytes;
                     csp->iob->cur = csp->iob->eod = csp->iob->buf;
                  }
                  if (add_to_iob(csp->iob, csp->config->buffer_limit,
                     csp->receive_buffer, len))
                  {
                     /* This is not supposed to happen but ... */
                     csp->flags &= ~CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
                     log_error(LOG_LEVEL_ERROR,
                        "Failed to buffer %ld bytes of chunk-encoded data.",
                        len);
                     return;
                  }
               }
            }
            byte_count += (unsigned long long)len;

            if (csp->flags & CSP_FLAG_CHUNKED)
            {
               int rc;
               size_t encoded_bytes = (size_t)(csp->iob->eod - csp->iob->cur);

               rc = get_bytes_missing_from_chunked_data(csp->iob->cur, encoded_bytes,
                  chunk_offset);
               if (rc >= 0)
               {
                  if (rc != 0)
                  {
                     chunk_offset = (size_t)rc;
                  }

                  if (chunked_data_is_complete(csp->iob->cur, encoded_bytes, chunk_offset))
                  {
                     log_error(LOG_LEVEL_CONNECT,
                        "We buffered the last chunk of the response.");
                     csp->expected_content_length = byte_count;
                     csp->flags |= CSP_FLAG_CONTENT_LENGTH_SET;
                  }
               }
            }

            continue;
         }
         else
         {
            /*
             * We're still looking for the end of the server's header.
             * Buffer up the data we just read.  If that fails, there's
             * little we can do but send our static out-of-memory page.
             */
            if (add_to_iob(csp->iob, csp->config->buffer_limit, csp->receive_buffer, len))
            {
               log_error(LOG_LEVEL_ERROR, "Out of memory while looking for end of server headers.");
               rsp = cgi_error_memory();
               send_crunch_response(csp, rsp);
               mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
               close_client_and_server_ssl_connections(csp);
#endif
               return;
            }

            /* Convert iob into something sed() can digest */
            if (JB_ERR_PARSE == get_server_headers(csp))
            {
               if (ms_iis5_hack)
               {
                  /*
                   * Well, we tried our MS IIS/5 hack and it didn't work.
                   * The header is incomplete and there isn't anything
                   * we can do about it.
                   */
                  log_error(LOG_LEVEL_ERROR, "Invalid server headers. "
                     "Applying the MS IIS5 hack didn't help.");
                  log_error(LOG_LEVEL_CLF,
                     "%s - - [%T] \"%s\" 502 0", csp->ip_addr_str, http->cmd);
#ifdef FEATURE_HTTPS_INSPECTION
                  /*
                   * Sending data with standard or secured connection (HTTP/HTTPS)
                   */
                  if (client_use_ssl(csp))
                  {
                     ssl_send_data_delayed(&(csp->ssl_client_attr),
                        (const unsigned char *)INVALID_SERVER_HEADERS_RESPONSE,
                        strlen(INVALID_SERVER_HEADERS_RESPONSE), get_write_delay(csp));
                  }
                  else
#endif /* def FEATURE_HTTPS_INSPECTION */
                  {
                     write_socket_delayed(csp->cfd,
                        INVALID_SERVER_HEADERS_RESPONSE,
                        strlen(INVALID_SERVER_HEADERS_RESPONSE), write_delay);
                  }
                  mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
                  close_client_and_server_ssl_connections(csp);
#endif
                  return;
               }
               else
               {
                  /*
                   * Since we have to wait for more from the server before
                   * we can parse the headers we just continue here.
                   */
                  log_error(LOG_LEVEL_CONNECT,
                     "Continuing buffering server headers from socket %d. "
                     "Bytes most recently read: %ld.", csp->cfd, len);
                  continue;
               }
            }
            else
            {
               /*
                * Account for the content bytes we
                * might have gotten with the headers.
                */
               assert(csp->iob->eod >= csp->iob->cur);
               byte_count = (unsigned long long)(csp->iob->eod - csp->iob->cur);
            }

            /* Did we actually get anything? */
            if (NULL == csp->headers->first)
            {
               if ((csp->flags & CSP_FLAG_REUSED_CLIENT_CONNECTION))
               {
                  log_error(LOG_LEVEL_ERROR,
                     "No server or forwarder response received on socket %d. "
                     "Closing client socket %d without sending data.",
                     csp->server_connection.sfd, csp->cfd);
                  log_error(LOG_LEVEL_CLF,
                     "%s - - [%T] \"%s\" 502 0", csp->ip_addr_str, http->cmd);
               }
               else
               {
                  log_error(LOG_LEVEL_ERROR,
                     "No server or forwarder response received on socket %d.",
                     csp->server_connection.sfd);
                  send_crunch_response(csp, error_response(csp, "no-server-data"));
               }
               free_http_request(http);
               mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
               close_client_and_server_ssl_connections(csp);
#endif
               return;
            }

            if (!csp->headers->first->str)
            {
               log_error(LOG_LEVEL_ERROR, "header search: csp->headers->first->str == NULL, assert will be called");
            }
            assert(csp->headers->first->str);

            if (strncmpic(csp->headers->first->str, "HTTP", 4) &&
                strncmpic(csp->headers->first->str, "ICY", 3))
            {
               /*
                * It doesn't look like a HTTP (or Shoutcast) response:
                * tell the client and log the problem.
                */
               if (strlen(csp->headers->first->str) > 30)
               {
                  csp->headers->first->str[30] = '\0';
               }
               log_error(LOG_LEVEL_ERROR,
                  "Invalid server or forwarder response. Starts with: %s",
                  csp->headers->first->str);
               log_error(LOG_LEVEL_CLF,
                  "%s - - [%T] \"%s\" 502 0", csp->ip_addr_str, http->cmd);
#ifdef FEATURE_HTTPS_INSPECTION
               /*
                * Sending data with standard or secured connection (HTTP/HTTPS)
                */
               if (client_use_ssl(csp))
               {
                  ssl_send_data_delayed(&(csp->ssl_client_attr),
                     (const unsigned char *)INVALID_SERVER_HEADERS_RESPONSE,
                     strlen(INVALID_SERVER_HEADERS_RESPONSE),
                     get_write_delay(csp));
               }
               else
#endif /* def FEATURE_HTTPS_INSPECTION */
               {
                  write_socket_delayed(csp->cfd, INVALID_SERVER_HEADERS_RESPONSE,
                     strlen(INVALID_SERVER_HEADERS_RESPONSE), write_delay);
               }
               free_http_request(http);
               mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
               close_client_and_server_ssl_connections(csp);
#endif
               return;
            }

            /*
             * Disable redirect checkers, so that they will be only run
             * again if the user also enables them through tags.
             *
             * From a performance point of view it doesn't matter,
             * but it prevents duplicated log messages.
             */
#ifdef FEATURE_FAST_REDIRECTS
            csp->action->flags &= ~ACTION_FAST_REDIRECTS;
#endif
            csp->action->flags &= ~ACTION_REDIRECT;

            /*
             * We have now received the entire server header,
             * filter it and send the result to the client
             */
            if (JB_ERR_OK != sed(csp, FILTER_SERVER_HEADERS))
            {
               log_error(LOG_LEVEL_CLF,
                  "%s - - [%T] \"%s\" 502 0", csp->ip_addr_str, http->cmd);
#ifdef FEATURE_HTTPS_INSPECTION
               /*
                * Sending data with standard or secured connection (HTTP/HTTPS)
                */
               if (client_use_ssl(csp))
               {
                  ssl_send_data_delayed(&(csp->ssl_client_attr),
                     (const unsigned char *)INVALID_SERVER_HEADERS_RESPONSE,
                     strlen(INVALID_SERVER_HEADERS_RESPONSE),
                     get_write_delay(csp));
               }
               else
#endif
               {
                  write_socket_delayed(csp->cfd, INVALID_SERVER_HEADERS_RESPONSE,
                     strlen(INVALID_SERVER_HEADERS_RESPONSE), write_delay);
               }
               free_http_request(http);
               mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
               close_client_and_server_ssl_connections(csp);
#endif
               return;
            }

            if ((csp->flags & CSP_FLAG_CHUNKED)
               && !(csp->flags & CSP_FLAG_CONTENT_LENGTH_SET))
            {
               int rc;
               size_t encoded_size = (size_t)(csp->iob->eod - csp->iob->cur);

               rc = get_bytes_missing_from_chunked_data(csp->iob->cur, encoded_size,
                  chunk_offset);
               if (rc >= 0)
               {
                  if (rc != 0)
                  {
                     chunk_offset = (size_t)rc;
                  }
                  if (chunked_data_is_complete(csp->iob->cur, encoded_size, chunk_offset))
                  {
                     log_error(LOG_LEVEL_CONNECT,
                        "Looks like we got the last chunk together with "
                        "the server headers. We better stop reading.");
                     byte_count = (unsigned long long)(csp->iob->eod - csp->iob->cur);
                     csp->expected_content_length = byte_count;
                     csp->flags |= CSP_FLAG_CONTENT_LENGTH_SET;
                  }
               }
            }
            csp->server_connection.response_received = time(NULL);

            if (crunch_response_triggered(csp, crunchers_light))
            {
               /*
                * One of the tags created by a server-header
                * tagger triggered a crunch. We already
                * delivered the crunch response to the client
                * and are done here after cleaning up.
                */
               mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
               close_client_and_server_ssl_connections(csp);
#endif
               return;
            }

            /* Buffer and pcrs filter this if appropriate. */
            buffer_and_filter_content = content_requires_filtering(csp);

            if (!buffer_and_filter_content)
            {
               /*
                * Write the server's (modified) header to
                * the client (along with anything else that
                * may be in the buffer). Use standard or secured
                * connection.
                */
               if (send_server_headers(csp))
               {
                  return;
               }
#ifdef FEATURE_HTTPS_INSPECTION
               if (client_use_ssl(csp))
               {
                  if ((len = ssl_flush_socket(&(csp->ssl_client_attr),
                           csp->iob)) < 0)
                  {
                     log_error(LOG_LEVEL_CONNECT,
                        "Sending buffered bytes to the client failed");

                     /*
                      * The write failed, so don't bother mentioning it
                      * to the client... it probably can't hear us anyway.
                      */
                     mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
                     close_client_and_server_ssl_connections(csp);
#endif
                     return;
                  }
               }
               else
#endif /* def FEATURE_HTTPS_INSPECTION */
               {
                  if ((len = flush_iob(csp->cfd, csp->iob, write_delay)) < 0)
                  {
                     log_error(LOG_LEVEL_ERROR,
                        "Sending buffered bytes to the client failed.");
                     /*
                      * The write failed, so don't bother mentioning it
                      * to the client... it probably can't hear us anyway.
                      */
                     mark_server_socket_tainted(csp);
                     return;
                  }
               }
               if (csp->flags & CSP_FLAG_CHUNKED &&
                 !(csp->flags & CSP_FLAG_CONTENT_LENGTH_SET))
               {
                  /*
                   * In case of valid data we shouldn't flush more
                   * data than chunk_offset but the data may be invalid.
                   */
                  if (chunk_offset >= len)
                  {
                     log_error(LOG_LEVEL_CONNECT,
                        "Reducing chunk offset from %lu to %lu after flushing %ld bytes",
                        chunk_offset, (chunk_offset - (unsigned)len), len);
                     chunk_offset = chunk_offset - (unsigned)len;
                  }
                  else
                  {
                     log_error(LOG_LEVEL_CONNECT,
                        "Keeping chunk offset at %lu despite flushing %ld bytes",
                        chunk_offset, len);
                     /*
                      * If we can't parse the chunk-encoded data we should
                      * not reuse the server connection.
                      */
                     mark_server_socket_tainted(csp);
                  }
               }
				}

            /* we're finished with the server's header */

            server_body = 1;

            /*
             * If this was a MS IIS/5 hack then it means the server
             * has already closed the connection. Nothing more to read.
             * Time to bail.
             */
            if (ms_iis5_hack)
            {
               log_error(LOG_LEVEL_ERROR,
                  "Closed server connection detected. "
                  "Applying the MS IIS5 hack didn't help.");
               log_error(LOG_LEVEL_CLF,
                  "%s - - [%T] \"%s\" 502 0", csp->ip_addr_str, http->cmd);
#ifdef FEATURE_HTTPS_INSPECTION
               /*
                * Sending data with standard or secured connection (HTTP/HTTPS)
                */
               if (client_use_ssl(csp))
               {
                  ssl_send_data_delayed(&(csp->ssl_client_attr),
                     (const unsigned char *)INVALID_SERVER_HEADERS_RESPONSE,
                     strlen(INVALID_SERVER_HEADERS_RESPONSE),
                     get_write_delay(csp));
               }
               else
#endif /* def FEATURE_HTTPS_INSPECTION */
               {
                  write_socket_delayed(csp->cfd, INVALID_SERVER_HEADERS_RESPONSE,
                     strlen(INVALID_SERVER_HEADERS_RESPONSE), write_delay);
               }
               mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
               close_client_and_server_ssl_connections(csp);
#endif
               return;
            }
         }
         continue;
      }
      mark_server_socket_tainted(csp);
#ifdef FEATURE_HTTPS_INSPECTION
      close_client_and_server_ssl_connections(csp);
#endif
      return; /* huh? we should never get here */
   }

   if (csp->content_length == 0)
   {
      /*
       * If Privoxy didn't recalculate the Content-Length,
       * byte_count is still correct.
       */
      csp->content_length = byte_count;
   }

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   if ((csp->flags & CSP_FLAG_CONTENT_LENGTH_SET)
      && (csp->expected_content_length != byte_count))
   {
      log_error(LOG_LEVEL_CONNECT,
         "Received %llu bytes while expecting %llu.",
         byte_count, csp->expected_content_length);
      mark_server_socket_tainted(csp);
   }
#endif

#ifdef FEATURE_HTTPS_INSPECTION
   if (client_use_ssl(csp))
   {
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s https://%s%s %s\" 200 %llu",
         csp->ip_addr_str, http->gpc, http->hostport, http->path,
         http->version, csp->content_length);
   }
   else
#endif
   {
      log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 200 %llu",
         csp->ip_addr_str, http->ocmd, csp->content_length);
   }
   csp->server_connection.timestamp = time(NULL);
}


/*********************************************************************
 *
 * Function    :  chat
 *
 * Description :  Once a connection from the client has been accepted,
 *                this function is called (via serve()) to handle the
 *                main business of the communication.  This function
 *                returns after dealing with a single request. It can
 *                be called multiple times with the same client socket
 *                if the client is keeping the connection alive.
 *
 *                The decision whether or not a client connection will
 *                be kept alive is up to the caller which also must
 *                close the client socket when done.
 *
 *                FIXME: chat is nearly thousand lines long.
 *                Ridiculous.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Nothing.
 *
 *********************************************************************/
static void chat(struct client_state *csp)
{
   const struct forward_spec *fwd;
   struct http_request *http;
   /* Skeleton for HTTP response, if we should intercept the request */
   struct http_response *rsp;
#ifdef FEATURE_HTTPS_INSPECTION
   int use_ssl_tunnel = 0;
#endif

   http = csp->http;

   if (receive_client_request(csp) != JB_ERR_OK)
   {
      return;
   }

#if defined(FEATURE_STATISTICS) && defined(MUTEX_LOCKS_AVAILABLE)
   privoxy_mutex_lock(&block_statistics_mutex);
   number_of_requests_received++;
   privoxy_mutex_unlock(&block_statistics_mutex);
#endif

   if (parse_client_request(csp) != JB_ERR_OK)
   {
      return;
   }

#ifdef FEATURE_HTTPS_INSPECTION
   /*
    * Setting flags to use old solution with SSL tunnel and to disable
    * certificate verification.
    */
   if (csp->http->ssl && !(csp->action->flags & ACTION_HTTPS_INSPECTION)
      && !cgi_page_requested(csp->http->host))
   {
      use_ssl_tunnel = 1;
   }

   if (http->ssl && (csp->action->flags & ACTION_IGNORE_CERTIFICATE_ERRORS))
   {
      csp->dont_verify_certificate = 1;
   }
#endif

   /*
    * build the http request to send to the server
    * we have to do one of the following:
    *
    * create =    use the original HTTP request to create a new
    *             HTTP request that has either the path component
    *             without the http://domainspec (w/path) or the
    *             full orininal URL (w/url)
    *             Note that the path and/or the HTTP version may
    *             have been altered by now.
    *
    * SSL proxy = Open a socket to the host:port of the server
    *             and create TLS/SSL connection with server and
    *             with client. Then behave like mediator between
    *             client and server over TLS/SSL.
    *
    * SSL proxy = Pass the request unchanged if forwarding a CONNECT
    *    with     request to a parent proxy. Note that we'll be sending
    * forwarding  the CFAIL message ourselves if connecting to the parent
    *             fails, but we won't send a CSUCCEED message if it works,
    *             since that would result in a double message (ours and the
    *             parent's). After sending the request to the parent, we
    *             must parse answer and send it to client. If connection
    *             with server is established, we do TLS/SSL proxy. Otherwise
    *             we send parent response to client and close connections.
    *
    * here's the matrix:
    *                        SSL
    *                    0        1
    *                +--------+--------+
    *                |        |        |
    *             0  | create |   SSL  |
    *                | w/path |  proxy |
    *  Forwarding    +--------+--------+
    *                |        |   SSL  |
    *             1  | create |  proxy |
    *                | w/url  |+forward|
    *                +--------+--------+
    *
    */

#ifdef FEATURE_HTTPS_INSPECTION
   /*
    * Presetting SSL client and server flags
    */
   if (http->ssl && !use_ssl_tunnel)
   {
      http->client_ssl = 1;
      http->server_ssl = 1;
   }
   else
   {
      http->client_ssl = 0;
      http->server_ssl = 0;
   }
#endif

#ifdef FEATURE_HTTPS_INSPECTION
   /*
    * Log the request unless we're https inspecting
    * in which case we don't have the path yet and
    * will log the request later.
    */
   if (!client_use_ssl(csp))
#endif
   {
      log_error(LOG_LEVEL_REQUEST, "%s%s", http->hostport, http->path);
   }

   if (http->ssl && connect_port_is_forbidden(csp))
   {
      const char *acceptable_connect_ports =
         csp->action->string[ACTION_STRING_LIMIT_CONNECT];
      assert(NULL != acceptable_connect_ports);
      log_error(LOG_LEVEL_INFO, "Request from %s marked for blocking. "
         "limit-connect{%s} doesn't allow CONNECT requests to %s",
         csp->ip_addr_str, acceptable_connect_ports, csp->http->hostport);
      csp->action->flags |= ACTION_BLOCK;
      http->ssl = 0;
#ifdef FEATURE_HTTPS_INSPECTION
      http->client_ssl = 0;
      http->server_ssl = 0;
#endif
   }

   /*
    * We have a request. Check if one of the crunchers wants it
    * unless the client wants to use TLS/SSL in which case we
    * haven't setup the TLS context yet and will send the crunch
    * response later.
    */
   if (
#ifdef FEATURE_HTTPS_INSPECTION
       !client_use_ssl(csp) &&
#endif
       crunch_response_triggered(csp, crunchers_all))
   {
      /*
       * Yes. The client got the crunch response and we're done here.
       */
      return;
   }

#ifdef FEATURE_HTTPS_INSPECTION
   if (client_use_ssl(csp) && !use_ssl_tunnel)
   {
      int ret;
      /*
       * Creating a SSL proxy.
       *
       * By sending the CSUCCEED message we're lying to the client as
       * the connection hasn't actually been established yet. We don't
       * establish the connection until we have seen and parsed the
       * encrypted client headers.
       */
      if (write_socket_delayed(csp->cfd, CSUCCEED,
            strlen(CSUCCEED), get_write_delay(csp)) != 0)
      {
         log_error(LOG_LEVEL_ERROR, "Sending SUCCEED to client failed");
         return;
      }

      ret = create_client_ssl_connection(csp);
      if (ret != 0)
      {
         log_error(LOG_LEVEL_ERROR,
            "Failed to open a secure connection with the client");
         return;
      }
      if (JB_ERR_OK != process_encrypted_request_headers(csp))
      {
         close_client_ssl_connection(csp);
         return;
      }
      /*
       * We have an encrypted request. Check if one of the crunchers now
       * wants it (for example because the previously invisible path was
       * required to match).
       */
      if (crunch_response_triggered(csp, crunchers_all))
      {
         /*
          * Yes. The client got the crunch response and we're done here.
          */
         return;
      }
   }
#endif

   /* If we need to apply client body filters, buffer the whole request now. */
   if (csp->expected_client_content_length != 0 &&
      (client_body_filters_enabled(csp->action) ||
         client_body_taggers_enabled(csp->action)) &&
      can_buffer_request_body(csp))
   {
      int content_modified;
      size_t modified_content_length;

#ifdef FEATURE_HTTPS_INSPECTION
      if (client_use_ssl(csp) && read_https_request_body(csp))
      {
         log_error(LOG_LEVEL_ERROR, "Failed to buffer the encrypted "
            "request body to apply filters or taggers.");
         log_error(LOG_LEVEL_CLF,
            "%s - - [%T] \"%s\" 400 0", csp->ip_addr_str, csp->http->cmd);

         ssl_send_data_delayed(&(csp->ssl_client_attr),
            (const unsigned char *)CLIENT_BODY_BUFFER_ERROR_RESPONSE,
            strlen(CLIENT_BODY_BUFFER_ERROR_RESPONSE),
            get_write_delay(csp));

         return;
      }
      else
#endif
      if (read_http_request_body(csp))
      {
         log_error(LOG_LEVEL_ERROR,
            "Failed to buffer the request body to apply filters or taggers,");
         log_error(LOG_LEVEL_CLF,
            "%s - - [%T] \"%s\" 400 0", csp->ip_addr_str, csp->http->cmd);

         write_socket_delayed(csp->cfd, CLIENT_BODY_BUFFER_ERROR_RESPONSE,
            strlen(CLIENT_BODY_BUFFER_ERROR_RESPONSE), get_write_delay(csp));

         return;
      }
      if (client_body_taggers_enabled(csp->action))
      {
         execute_client_body_taggers(csp, csp->expected_client_content_length);
         if (crunch_response_triggered(csp, crunchers_all))
         {
            /*
             * Yes. The client got the crunch response and we're done here.
             */
            return;
         }
      }
      if (client_body_filters_enabled(csp->action))
      {
         modified_content_length = csp->expected_client_content_length;
         content_modified = execute_client_body_filters(csp,
            &modified_content_length);
         if ((content_modified == 1) &&
            (modified_content_length != csp->expected_client_content_length) &&
            update_client_headers(csp, modified_content_length))
         {
            /* XXX: Send error response */
            log_error(LOG_LEVEL_HEADER, "Error updating client headers");
            return;
         }
      }
      csp->expected_client_content_length = 0;
   }

   log_applied_actions(csp->action);

   /* decide how to route the HTTP request */
   fwd = forward_url(csp, http);

   freez(csp->headers->first->str);
   build_request_line(csp, fwd, &csp->headers->first->str);

   if (fwd->forward_host)
   {
      log_error(LOG_LEVEL_CONNECT, "via [%s]:%d to: %s",
         fwd->forward_host, fwd->forward_port, http->hostport);
   }
   else
   {
      log_error(LOG_LEVEL_CONNECT, "to %s", http->hostport);
   }

   /* here we connect to the server, gateway, or the forwarder */

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   if ((csp->server_connection.sfd != JB_INVALID_SOCKET)
      && socket_is_still_alive(csp->server_connection.sfd)
      && connection_destination_matches(&csp->server_connection, http, fwd))
   {
      log_error(LOG_LEVEL_CONNECT,
         "Reusing server socket %d connected to %s. Total requests: %u.",
         csp->server_connection.sfd, csp->server_connection.host,
         csp->server_connection.requests_sent_total);
   }
   else
   {
      if (csp->server_connection.sfd != JB_INVALID_SOCKET)
      {
#ifdef FEATURE_CONNECTION_SHARING
         if (csp->config->feature_flags & RUNTIME_FEATURE_CONNECTION_SHARING
#ifdef FEATURE_HTTPS_INSPECTION
            && !server_use_ssl(csp)
#endif
            )
         {
            remember_connection(&csp->server_connection);
         }
         else
#endif /* def FEATURE_CONNECTION_SHARING */
         {
            log_error(LOG_LEVEL_CONNECT,
               "Closing server socket %d connected to %s. Total requests: %u.",
               csp->server_connection.sfd, csp->server_connection.host,
               csp->server_connection.requests_sent_total);
            close_socket(csp->server_connection.sfd);
         }
         mark_connection_closed(&csp->server_connection);
      }
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

      /*
       * Connecting to destination server
       */
      csp->server_connection.sfd = forwarded_connect(fwd, http, csp);

      if (csp->server_connection.sfd == JB_INVALID_SOCKET)
      {
         if (fwd->type != SOCKS_NONE)
         {
            /* Socks error. */
            rsp = error_response(csp, "forwarding-failed");
         }
         else if (errno == EINVAL)
         {
            rsp = error_response(csp, "no-such-domain");
         }
         else
         {
            rsp = error_response(csp, "connect-failed");
         }

         /* Write the answer to the client */
         if (rsp != NULL)
         {
            send_crunch_response(csp, rsp);
         }

         /*
          * Temporary workaround to prevent already-read client
          * bodies from being parsed as new requests. For now we
          * err on the safe side and throw all the following
          * requests under the bus, even if no client body has been
          * buffered. A compliant client will repeat the dropped
          * requests on an untainted connection.
          *
          * The proper fix is to discard the no longer needed
          * client body in the buffer (if there is one) and to
          * continue parsing the bytes that follow.
          */
#ifdef FEATURE_HTTPS_INSPECTION
         close_client_ssl_connection(csp);
#endif
         drain_and_close_socket(csp->cfd);
         csp->cfd = JB_INVALID_SOCKET;

         return;
      }

#ifdef FEATURE_HTTPS_INSPECTION
      /*
       * Creating TLS/SSL connections with destination server or parent
       * proxy. If forwarding is enabled, we must send client request to
       * parent proxy and receive, parse and resend parent proxy answer.
       */
      if (http->ssl && !use_ssl_tunnel)
      {
         int ret;
         if (fwd->forward_host != NULL)
         {
            char server_response[BUFFER_SIZE];
            int ret = 0;
            int len = 0;
            char *hdr = list_to_text(csp->headers);
            memset(server_response, 0, sizeof(server_response));

            if (hdr == NULL)
            {
               log_error(LOG_LEVEL_FATAL,
                  "Out of memory parsing client header");
            }
            list_remove_all(csp->headers);

            /*
             * Sending client's CONNECT request to the parent proxy
             */
            ret = write_socket(csp->server_connection.sfd, hdr, strlen(hdr));

            freez(hdr);

            if (ret != 0)
            {
               log_error(LOG_LEVEL_CONNECT,
                  "Sending request headers to: %s failed", http->hostport);
               mark_server_socket_tainted(csp);
               close_client_ssl_connection(csp);
               return;
            }

            /* Waiting for parent proxy server response */
            len = read_socket(csp->server_connection.sfd, server_response,
               sizeof(server_response)-1);

            if (len <= 0)
            {
               log_error(LOG_LEVEL_ERROR, "No response from parent proxy "
                  "server on socket %d.", csp->server_connection.sfd);

               rsp = error_response(csp, "no-server-data");
               if (rsp)
               {
                  send_crunch_response(csp, rsp);
               }
               mark_server_socket_tainted(csp);
               close_client_ssl_connection(csp);
               return;
            }

            /*
             * Test if the connection to the destination server was
             * established successfully by the parent proxy.
             */
            if (!tunnel_established_successfully(server_response, (unsigned int)len))
            {
               log_error(LOG_LEVEL_ERROR,
                  "The forwarder %s failed to establish a connection with %s",
                  fwd->forward_host, http->host);
               rsp = error_response(csp, "connect-failed");
               if (rsp)
               {
                  send_crunch_response(csp, rsp);
               }
               mark_server_socket_tainted(csp);
               close_client_ssl_connection(csp);
               return;
            }
         } /* -END- if (fwd->forward_host != NULL) */

         /*
          * We can now create the TLS/SSL connection with the destination server.
          */
         ret = create_server_ssl_connection(csp);
         if (ret != 0)
         {
            if (csp->server_cert_verification_result != SSL_CERT_VALID &&
                csp->server_cert_verification_result != SSL_CERT_NOT_VERIFIED)
            {
               /*
                * If the server certificate is invalid, we must inform
                * the client and then close connection to the client.
                */
               ssl_send_certificate_error(csp);
               close_client_and_server_ssl_connections(csp);
               return;
            }
            if (csp->server_cert_verification_result == SSL_CERT_NOT_VERIFIED
             || csp->server_cert_verification_result == SSL_CERT_VALID)
            {
               /*
                * The TLS/SSL connection wasn't created but an invalid
                * certificate wasn't detected. Report it as connection
                * failure.
                */
               rsp = error_response(csp, "connect-failed");
               if (rsp)
               {
                  send_crunch_response(csp, rsp);
               }
               close_client_and_server_ssl_connections(csp);
               return;
            }
         }
      }/* -END- if (http->ssl) */
#endif /* def FEATURE_HTTPS_INSPECTION */

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
      save_connection_destination(csp->server_connection.sfd,
         http, fwd, &csp->server_connection);
      csp->server_connection.keep_alive_timeout =
         (unsigned)csp->config->keep_alive_timeout;
   }
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

   csp->server_connection.requests_sent_total++;

   if ((fwd->type == SOCKS_5T) && (NULL == csp->headers->first))
   {
      /* Client headers have been sent optimistically */
      assert(csp->headers->last == NULL);
   }
   else if (http->ssl == 0 || (fwd->forward_host
#ifdef FEATURE_HTTPS_INSPECTION
         && use_ssl_tunnel
#endif
           ))
   {
      int status = send_http_request(csp);
      if (status == 2)
      {
         /* The request got crunched, a response has been delivered. */
         return;
      }
      if (status != 0)
      {
         rsp = error_response(csp, "connect-failed");
         if (rsp)
         {
            send_crunch_response(csp, rsp);
         }
         return;
      }
   }
   else
   {
      /*
       * Using old solution with SSL tunnel or new solution with SSL proxy
       */
      list_remove_all(csp->headers);
#ifdef FEATURE_HTTPS_INSPECTION
      if (use_ssl_tunnel)
#endif
      {
         /*
         * We're running an SSL tunnel and we're not forwarding,
         * so just ditch the client headers, send the "connect succeeded"
         * message to the client, flush the rest, and get out of the way.
         */
         if (write_socket_delayed(csp->cfd, CSUCCEED,
               strlen(CSUCCEED), get_write_delay(csp)))
         {
            return;
         }
      }
#ifdef FEATURE_HTTPS_INSPECTION
      else
      {
         /*
          * If server certificate has been verified and is invalid,
          * we must inform the client and then close the connection
          * with client and server.
          */
         if (csp->server_cert_verification_result != SSL_CERT_VALID &&
             csp->server_cert_verification_result != SSL_CERT_NOT_VERIFIED)
         {
            ssl_send_certificate_error(csp);
            close_client_and_server_ssl_connections(csp);
            return;
         }
         if (send_https_request(csp))
         {
            rsp = error_response(csp, "connect-failed");
            if (rsp)
            {
               send_crunch_response(csp, rsp);
            }
            close_client_and_server_ssl_connections(csp);
            return;
         }
      }
#endif /* def FEATURE_HTTPS_INSPECTION */
      clear_iob(csp->client_iob);
   }/* -END- else ... if (http->ssl == 1) */

   log_error(LOG_LEVEL_CONNECT, "to %s successful", http->hostport);

   /* XXX: should the time start earlier for optimistically sent data? */
   csp->server_connection.request_sent = time(NULL);

   handle_established_connection(csp);
   freez(csp->receive_buffer);
}


#ifdef FUZZ
/*********************************************************************
 *
 * Function    :  fuzz_server_response
 *
 * Description :  Treat the input as a whole server response.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  fuzz_input_file = File to read the input from.
 *
 * Returns     :  0
 *
 *********************************************************************/
extern int fuzz_server_response(struct client_state *csp, char *fuzz_input_file)
{
   static struct forward_spec fwd; /* Zero'd due to being static */
   csp->cfd = 0;

   if (strcmp(fuzz_input_file, "-") == 0)
   {
      /* XXX: Doesn't work yet. */
      csp->server_connection.sfd = 0;
   }
   else
   {
      csp->server_connection.sfd = open(fuzz_input_file, O_RDONLY);
      if (csp->server_connection.sfd == -1)
      {
         log_error(LOG_LEVEL_FATAL, "Failed to open %s: %E",
            fuzz_input_file);
      }
   }
   csp->fwd = &fwd;
   csp->content_type |= CT_GIF;
   csp->action->flags |= ACTION_DEANIMATE;
   csp->action->string[ACTION_STRING_DEANIMATE] = "last";

   csp->http->path = strdup_or_die("/");
   csp->http->host = strdup_or_die("fuzz.example.org");
   csp->http->hostport = strdup_or_die("fuzz.example.org:80");
   /* Prevent client socket monitoring */
   csp->flags |= CSP_FLAG_PIPELINED_REQUEST_WAITING;
   csp->flags |= CSP_FLAG_CHUNKED;

   csp->config->feature_flags |= RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE;
   csp->flags |= CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;

   csp->content_type |= CT_DECLARED|CT_GIF;

   csp->config->socket_timeout = 0;

   cgi_init_error_messages();

   handle_established_connection(csp);
   freez(csp->receive_buffer);

   return 0;
}
#endif


#ifdef FEATURE_CONNECTION_KEEP_ALIVE
/*********************************************************************
 *
 * Function    :  prepare_csp_for_next_request
 *
 * Description :  Put the csp in a mostly vergin state.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void prepare_csp_for_next_request(struct client_state *csp)
{
   csp->content_type = 0;
   csp->content_length = 0;
   csp->expected_content_length = 0;
   csp->expected_client_content_length = 0;
   list_remove_all(csp->headers);
   clear_iob(csp->iob);
   freez(csp->error_message);
   free_http_request(csp->http);
   destroy_list(csp->headers);
#ifdef FEATURE_HTTPS_INSPECTION
   destroy_list(csp->https_headers);
#endif
   destroy_list(csp->tags);
#ifdef FEATURE_CLIENT_TAGS
   destroy_list(csp->client_tags);
   freez(csp->client_address);
#endif
   free_current_action(csp->action);
   if (NULL != csp->fwd)
   {
      unload_forward_spec(csp->fwd);
      csp->fwd = NULL;
   }
   /* XXX: Store per-connection flags someplace else. */
   csp->flags = (CSP_FLAG_ACTIVE | CSP_FLAG_REUSED_CLIENT_CONNECTION);
#ifdef FEATURE_TOGGLE
   if (global_toggle_state)
#endif /* def FEATURE_TOGGLE */
   {
      csp->flags |= CSP_FLAG_TOGGLED_ON;
   }

   if (csp->client_iob->eod > csp->client_iob->cur)
   {
      long bytes_to_shift = csp->client_iob->cur - csp->client_iob->buf;
      size_t data_length  = (size_t)(csp->client_iob->eod - csp->client_iob->cur);

      assert(bytes_to_shift > 0);
      assert(data_length > 0);

      log_error(LOG_LEVEL_CONNECT, "Shifting %lu pipelined bytes by %ld bytes",
         data_length, bytes_to_shift);
      memmove(csp->client_iob->buf, csp->client_iob->cur, data_length);
      csp->client_iob->cur = csp->client_iob->buf;
      assert(csp->client_iob->eod == csp->client_iob->buf + bytes_to_shift + data_length);
      csp->client_iob->eod = csp->client_iob->buf + data_length;
      memset(csp->client_iob->eod, '\0', (size_t)bytes_to_shift);

      csp->flags |= CSP_FLAG_PIPELINED_REQUEST_WAITING;
   }
   else
   {
      /*
       * We mainly care about resetting client_iob->cur so we don't
       * waste buffer space at the beginning and don't mess up the
       * request restoration done by cgi_show_request().
       *
       * Freeing the buffer itself isn't technically necessary,
       * but makes debugging more convenient.
       */
      clear_iob(csp->client_iob);
   }
}
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */


/*********************************************************************
 *
 * Function    :  serve
 *
 * Description :  This is little more than chat.  We only "serve" to
 *                to close (or remember) any socket that chat may have
 *                opened.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void serve(struct client_state *csp)
{
   int config_file_change_detected = 0; /* Only used for debugging */
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
#ifdef FEATURE_CONNECTION_SHARING
   static int monitor_thread_running = 0;
#endif /* def FEATURE_CONNECTION_SHARING */
   int continue_chatting = 0;

   log_error(LOG_LEVEL_CONNECT, "Accepted connection from %s on socket %d",
      csp->ip_addr_str, csp->cfd);

   do
   {
      unsigned int latency;

#ifdef FEATURE_HTTPS_INSPECTION
      if (continue_chatting && client_use_ssl(csp))
      {
         continue_https_chat(csp);
      }
      else
#endif
      {
         chat(csp);
      }

      /*
       * If the request has been crunched,
       * the calculated latency is zero.
       */
      latency = (unsigned)(csp->server_connection.response_received -
         csp->server_connection.request_sent) / 2;

      if ((csp->flags & CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE)
         && (csp->flags & CSP_FLAG_CRUNCHED)
         && (csp->expected_client_content_length != 0))
      {
         csp->flags |= CSP_FLAG_SERVER_SOCKET_TAINTED;
         log_error(LOG_LEVEL_CONNECT,
            "Tainting client socket %d due to unread data.", csp->cfd);
      }

      continue_chatting = (csp->config->feature_flags
         & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE)
         && !(csp->flags & CSP_FLAG_SERVER_SOCKET_TAINTED)
         && (csp->cfd != JB_INVALID_SOCKET)
         && (csp->flags & CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE)
         && ((csp->flags & CSP_FLAG_SERVER_CONTENT_LENGTH_SET)
            || (csp->flags & CSP_FLAG_CHUNKED));

      if (!(csp->flags & CSP_FLAG_CRUNCHED)
         && (csp->server_connection.sfd != JB_INVALID_SOCKET))
      {
         if (!(csp->flags & CSP_FLAG_SERVER_KEEP_ALIVE_TIMEOUT_SET))
         {
            csp->server_connection.keep_alive_timeout = csp->config->default_server_timeout;
         }
         if (!(csp->flags & CSP_FLAG_SERVER_CONNECTION_KEEP_ALIVE)
            || (csp->flags & CSP_FLAG_SERVER_SOCKET_TAINTED)
            || !socket_is_still_alive(csp->server_connection.sfd)
            || !(latency < csp->server_connection.keep_alive_timeout))
         {
            log_error(LOG_LEVEL_CONNECT,
               "Closing server socket %d connected to %s. "
               "Keep-alive: %u. Tainted: %u. Socket alive: %u. Timeout: %u.",
               csp->server_connection.sfd, (csp->server_connection.host != NULL) ?
               csp->server_connection.host : csp->http->host,
               0 != (csp->flags & CSP_FLAG_SERVER_CONNECTION_KEEP_ALIVE),
               0 != (csp->flags & CSP_FLAG_SERVER_SOCKET_TAINTED),
               socket_is_still_alive(csp->server_connection.sfd),
               csp->server_connection.keep_alive_timeout);
#ifdef FEATURE_CONNECTION_SHARING
            if (csp->config->feature_flags & RUNTIME_FEATURE_CONNECTION_SHARING)
            {
               forget_connection(csp->server_connection.sfd);
            }
#endif /* def FEATURE_CONNECTION_SHARING */
#ifdef FEATURE_HTTPS_INSPECTION
            close_server_ssl_connection(csp);
#endif
            close_socket(csp->server_connection.sfd);
            mark_connection_closed(&csp->server_connection);
#ifdef FEATURE_HTTPS_INSPECTION
            if (continue_chatting && client_use_ssl(csp))
            {
               /*
                * Close the client socket as well as Privoxy currently
                * can't establish a new server connection when the client
                * socket is reused and would drop the connection in
                * continue_https_chat() anyway.
                */
               continue_chatting = 0;
               csp->flags &= ~CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
               log_error(LOG_LEVEL_CONNECT,
                  "Client socket %d is no longer usable. "
                  "The server socket has been closed.", csp->cfd);
            }
#endif
         }
      }

      if (continue_chatting && any_loaded_file_changed(csp))
      {
         continue_chatting = 0;
         config_file_change_detected = 1;
      }
#ifdef FEATURE_HTTPS_INSPECTION
      if (continue_chatting && client_use_ssl(csp) &&
         csp->ssl_with_client_is_opened == 0)
      {
         continue_chatting = 0;
         log_error(LOG_LEVEL_CONNECT, "Client socket %d is no longer usable. "
            "The TLS session has been terminated.", csp->cfd);
      }
#endif

      if (continue_chatting)
      {
         if (((csp->flags & CSP_FLAG_PIPELINED_REQUEST_WAITING) != 0)
            && socket_is_still_alive(csp->cfd))
         {
            log_error(LOG_LEVEL_CONNECT, "Client request %d has been "
               "pipelined on socket %d and the socket is still alive.",
               csp->requests_received_total+1, csp->cfd);
            prepare_csp_for_next_request(csp);
            continue;
         }

         if (0 != (csp->flags & CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE))
         {
            if (csp->server_connection.sfd != JB_INVALID_SOCKET)
            {
               log_error(LOG_LEVEL_CONNECT,
                  "Waiting for the next client request on socket %d. "
                  "Keeping the server socket %d to %s open.",
                  csp->cfd, csp->server_connection.sfd, csp->server_connection.host);
            }
            else
            {
               log_error(LOG_LEVEL_CONNECT,
                  "Waiting for the next client request on socket %d. "
                  "No server socket to keep open.", csp->cfd);
            }
         }

         if ((csp->flags & CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE)
            && data_is_available(csp->cfd, (int)csp->config->keep_alive_timeout)
            && socket_is_still_alive(csp->cfd))
         {
            log_error(LOG_LEVEL_CONNECT,
               "Data arrived in time on client socket %d. Requests so far: %u",
               csp->cfd, csp->requests_received_total);
            prepare_csp_for_next_request(csp);
         }
         else
         {
#ifdef FEATURE_CONNECTION_SHARING
            if ((csp->config->feature_flags & RUNTIME_FEATURE_CONNECTION_SHARING)
               && (csp->server_connection.sfd != JB_INVALID_SOCKET)
               && (socket_is_still_alive(csp->server_connection.sfd))
#ifdef FEATURE_HTTPS_INSPECTION
               && !server_use_ssl(csp)
#endif
                )
            {
               time_t time_open = time(NULL) - csp->server_connection.timestamp;

               if (csp->server_connection.keep_alive_timeout < time_open - (time_t)latency)
               {
                  break;
               }

               remember_connection(&csp->server_connection);
               csp->server_connection.sfd = JB_INVALID_SOCKET;
               drain_and_close_socket(csp->cfd);
               csp->cfd = JB_INVALID_SOCKET;
               privoxy_mutex_lock(&connection_reuse_mutex);
               if (!monitor_thread_running)
               {
                  monitor_thread_running = 1;
                  privoxy_mutex_unlock(&connection_reuse_mutex);
                  wait_for_alive_connections();
                  privoxy_mutex_lock(&connection_reuse_mutex);
                  monitor_thread_running = 0;
               }
               privoxy_mutex_unlock(&connection_reuse_mutex);
            }
#endif /* def FEATURE_CONNECTION_SHARING */
            break;
         }
      }
      else if (csp->server_connection.sfd != JB_INVALID_SOCKET)
      {
         log_error(LOG_LEVEL_CONNECT,
            "Closing server socket %d connected to %s. Keep-alive: %u. "
            "Tainted: %u. Socket alive: %u. Timeout: %u. "
            "Configuration file change detected: %u",
            csp->server_connection.sfd, csp->server_connection.host,
            0 != (csp->flags & CSP_FLAG_SERVER_CONNECTION_KEEP_ALIVE),
            0 != (csp->flags & CSP_FLAG_SERVER_SOCKET_TAINTED),
            socket_is_still_alive(csp->server_connection.sfd),
            csp->server_connection.keep_alive_timeout,
            config_file_change_detected);
      }
   } while (continue_chatting);

#else
   chat(csp);
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

   if (csp->cfd != JB_INVALID_SOCKET)
   {
      log_error(LOG_LEVEL_CONNECT, "Closing client socket %d. "
         "Keep-alive: %u. Socket alive: %u. Data available: %u. "
         "Configuration file change detected: %u. Requests received: %u.",
         csp->cfd, 0 != (csp->flags & CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE),
         socket_is_still_alive(csp->cfd), data_is_available(csp->cfd, 0),
         config_file_change_detected, csp->requests_received_total);
#ifdef FEATURE_HTTPS_INSPECTION
      close_client_ssl_connection(csp);
#endif
      drain_and_close_socket(csp->cfd);
   }

   if (csp->server_connection.sfd != JB_INVALID_SOCKET)
   {
#ifdef FEATURE_CONNECTION_SHARING
      if (csp->config->feature_flags & RUNTIME_FEATURE_CONNECTION_SHARING)
      {
         forget_connection(csp->server_connection.sfd);
      }
#endif /* def FEATURE_CONNECTION_SHARING */

#ifdef FEATURE_HTTPS_INSPECTION
      close_server_ssl_connection(csp);
#endif /* def FEATURE_HTTPS_INSPECTION */

      close_socket(csp->server_connection.sfd);
   }

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   mark_connection_closed(&csp->server_connection);
#endif

   free_csp_resources(csp);

   csp->flags &= ~CSP_FLAG_ACTIVE;

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


#if !defined(_WIN32) || defined(_WIN_CONSOLE)
/*********************************************************************
 *
 * Function    :  usage
 *
 * Description :  Print usage info & exit.
 *
 * Parameters  :  Pointer to argv[0] for identifying ourselves
 *
 * Returns     :  No. ,-)
 *
 *********************************************************************/
static void usage(const char *name)
{
   printf("Privoxy version " VERSION " (" HOME_PAGE_URL ")\n"
          "Usage: %s [--config-test] "
#if defined(unix)
          "[--chroot] "
#endif /* defined(unix) */
          "[--help] "
#if defined(unix)
          "[--no-daemon] [--pidfile pidfile] [--pre-chroot-nslookup hostname] [--user user[.group]] "
#endif /* defined(unix) */
         "[--version] [configfile]\n",
          name);

#ifdef FUZZ
   show_fuzz_usage(name);
#endif

   printf("Aborting\n");

   exit(2);

}
#endif /* #if !defined(_WIN32) || defined(_WIN_CONSOLE) */


#ifdef MUTEX_LOCKS_AVAILABLE
/*********************************************************************
 *
 * Function    :  privoxy_mutex_lock
 *
 * Description :  Locks a mutex.
 *
 * Parameters  :
 *          1  :  mutex = The mutex to lock.
 *
 * Returns     :  Void. May exit in case of errors.
 *
 *********************************************************************/
void privoxy_mutex_lock(privoxy_mutex_t *mutex)
{
#ifdef FEATURE_PTHREAD
   int err = pthread_mutex_lock(mutex);
   if (err)
   {
      if (mutex != &log_mutex)
      {
         log_error(LOG_LEVEL_FATAL,
            "Mutex locking failed: %s.\n", strerror(err));
      }
      exit(1);
   }
#else
   EnterCriticalSection(mutex);
#endif /* def FEATURE_PTHREAD */
}


/*********************************************************************
 *
 * Function    :  privoxy_mutex_unlock
 *
 * Description :  Unlocks a mutex.
 *
 * Parameters  :
 *          1  :  mutex = The mutex to unlock.
 *
 * Returns     :  Void. May exit in case of errors.
 *
 *********************************************************************/
void privoxy_mutex_unlock(privoxy_mutex_t *mutex)
{
#ifdef FEATURE_PTHREAD
   int err = pthread_mutex_unlock(mutex);
   if (err)
   {
      if (mutex != &log_mutex)
      {
         log_error(LOG_LEVEL_FATAL,
            "Mutex unlocking failed: %s.\n", strerror(err));
      }
      exit(1);
   }
#else
   LeaveCriticalSection(mutex);
#endif /* def FEATURE_PTHREAD */
}


/*********************************************************************
 *
 * Function    :  privoxy_mutex_init
 *
 * Description :  Prepares a mutex.
 *
 * Parameters  :
 *          1  :  mutex = The mutex to initialize.
 *
 * Returns     :  Void. May exit in case of errors.
 *
 *********************************************************************/
static void privoxy_mutex_init(privoxy_mutex_t *mutex)
{
#ifdef FEATURE_PTHREAD
   int err = pthread_mutex_init(mutex, 0);
   if (err)
   {
      printf("Fatal error. Mutex initialization failed: %s.\n",
         strerror(err));
      exit(1);
   }
#else
   InitializeCriticalSection(mutex);
#endif /* def FEATURE_PTHREAD */
}

static void privoxy_mutex_release(privoxy_mutex_t* mutex)
{
#ifdef FEATURE_PTHREAD
    pthread_mutex_destroy(mutex);
#else
    DeleteCriticalSection(mutex);
#endif /* def FEATURE_PTHREAD */
}

#endif /* def MUTEX_LOCKS_AVAILABLE */

/*********************************************************************
 *
 * Function    :  initialize_mutexes
 *
 * Description :  Prepares mutexes if mutex support is available.
 *
 * Parameters  :  None
 *
 * Returns     :  Void, exits in case of errors.
 *
 *********************************************************************/
static void initialize_mutexes(void)
{
#ifdef MUTEX_LOCKS_AVAILABLE
   /*
    * Prepare global mutex semaphores
    */

#ifdef FEATURE_HTTPS_INSPECTION
   privoxy_mutex_init(&certificate_mutex);
   privoxy_mutex_init(&ssl_init_mutex);
#endif

   privoxy_mutex_init(&log_mutex);
   privoxy_mutex_init(&log_init_mutex);
   privoxy_mutex_init(&connection_reuse_mutex);
#ifdef FEATURE_EXTERNAL_FILTERS
   privoxy_mutex_init(&external_filter_mutex);
#endif
#ifdef FEATURE_CLIENT_TAGS
   privoxy_mutex_init(&client_tags_mutex);
#endif
#ifdef FEATURE_STATISTICS
   privoxy_mutex_init(&block_statistics_mutex);
#endif
#ifdef FEATURE_EXTENDED_STATISTICS
   privoxy_mutex_init(&filter_statistics_mutex);
   privoxy_mutex_init(&block_reason_statistics_mutex);
#endif

   /*
    * XXX: The assumptions below are a bit naive
    * and can cause locks that aren't necessary.
    *
    * For example older FreeBSD versions (< 6.x?)
    * have no gethostbyname_r, but gethostbyname is
    * thread safe.
    */
#if !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R)
   privoxy_mutex_init(&resolver_mutex);
#endif /* !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R) */
   /*
    * XXX: should we use a single mutex for
    * localtime() and gmtime() as well?
    */
#ifndef HAVE_GMTIME_R
   privoxy_mutex_init(&gmtime_mutex);
#endif /* ndef HAVE_GMTIME_R */

#ifndef HAVE_LOCALTIME_R
   privoxy_mutex_init(&localtime_mutex);
#endif /* ndef HAVE_GMTIME_R */

#if !defined(HAVE_ARC4RANDOM) && !defined(HAVE_RANDOM)
   privoxy_mutex_init(&rand_mutex);
#endif /* !defined(HAVE_ARC4RANDOM) && !defined(HAVE_RANDOM) */

#endif /* def MUTEX_LOCKS_AVAILABLE */
}

static void release_mutexes(void)
{
#ifdef MUTEX_LOCKS_AVAILABLE
    /*
     * Prepare global mutex semaphores
     */

#ifdef FEATURE_HTTPS_INSPECTION
    privoxy_mutex_release(&certificate_mutex);
    privoxy_mutex_release(&ssl_init_mutex);
#endif

    privoxy_mutex_release(&log_mutex);
    privoxy_mutex_release(&log_init_mutex);
    privoxy_mutex_release(&connection_reuse_mutex);
#ifdef FEATURE_EXTERNAL_FILTERS
    privoxy_mutex_release(&external_filter_mutex);
#endif
#ifdef FEATURE_CLIENT_TAGS
    privoxy_mutex_release(&client_tags_mutex);
#endif

#if !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R)
    privoxy_mutex_release(&resolver_mutex);
#endif /* !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_GETHOSTBYNAME_R) */

#ifndef HAVE_GMTIME_R
    privoxy_mutex_release(&gmtime_mutex);
#endif /* ndef HAVE_GMTIME_R */

#ifndef HAVE_LOCALTIME_R
    privoxy_mutex_release(&localtime_mutex);
#endif /* ndef HAVE_GMTIME_R */

#if !defined(HAVE_ARC4RANDOM) && !defined(HAVE_RANDOM)
    privoxy_mutex_release(&rand_mutex);
#endif /* !defined(HAVE_ARC4RANDOM) && !defined(HAVE_RANDOM) */

#endif /* def MUTEX_LOCKS_AVAILABLE */
}

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
int real_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
   int argc_pos = 0;
   int do_config_test = 0;
#ifndef HAVE_ARC4RANDOM
   unsigned int random_seed;
#endif
#ifdef unix
   struct passwd *pw = NULL;
   struct group *grp = NULL;
   int do_chroot = 0;
   char *pre_chroot_nslookup_to_load_resolver = NULL;
#endif
#ifdef FUZZ
   char *fuzz_input_type = NULL;
   char *fuzz_input_file = NULL;
#endif

   Argc = argc;
   Argv = argv;

   configfile =
#if !defined(_WIN32)
   "config"
#else
   "config.txt"
#endif
      ;

   /* Prepare mutexes if supported and necessary. */
   initialize_mutexes();

   /* Enable logging until further notice. */
   init_log_module();

   /*
    * Parse the command line arguments
    *
    * XXX: simply printing usage information in case of
    * invalid arguments isn't particularly user friendly.
    */
   while (++argc_pos < argc)
   {
#ifdef _WIN32
      /* Check to see if the service must be installed or uninstalled */
      if (strncmp(argv[argc_pos], "--install", 9) == 0)
      {
         const char *pName = argv[argc_pos] + 9;
         if (*pName == ':')
            pName++;
         exit((install_service(pName)) ? 0 : 1);
      }
      else if (strncmp(argv[argc_pos], "--uninstall", 11) == 0)
      {
         const char *pName = argv[argc_pos] + 11;
         if (*pName == ':')
            pName++;
         exit((uninstall_service(pName)) ? 0 : 1);
      }
      else if (strcmp(argv[argc_pos], "--service") == 0)
      {
         bRunAsService = TRUE;
         w32_set_service_cwd();
         atexit(w32_service_exit_notify);
      }
      else
#endif /* defined(_WIN32) */


#if !defined(_WIN32) || defined(_WIN_CONSOLE)

      if (strcmp(argv[argc_pos], "--help") == 0)
      {
         usage(argv[0]);
      }

      else if (strcmp(argv[argc_pos], "--version") == 0)
      {
         printf("Privoxy version " VERSION " (" HOME_PAGE_URL ")\n");
         exit(0);
      }

#if defined(unix)

      else if (strcmp(argv[argc_pos], "--no-daemon") == 0)
      {
         set_debug_level(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_INFO);
         daemon_mode = 0;
      }

      else if (strcmp(argv[argc_pos], "--pidfile") == 0)
      {
         if (++argc_pos == argc) usage(argv[0]);
         pidfile = strdup_or_die(argv[argc_pos]);
      }

      else if (strcmp(argv[argc_pos], "--user") == 0)
      {
         char *user_arg;
         char *group_name;

         if (++argc_pos == argc) usage(argv[argc_pos]);

         user_arg = strdup_or_die(argv[argc_pos]);
         group_name = strchr(user_arg, '.');
         if (NULL != group_name)
         {
            /* Nul-terminate the user name */
            *group_name = '\0';

            /* Skip the former delimiter to actually reach the group name */
            group_name++;

            grp = getgrnam(group_name);
            if (NULL == grp)
            {
               log_error(LOG_LEVEL_FATAL, "Group '%s' not found.", group_name);
            }
         }
         pw = getpwnam(user_arg);
         if (NULL == pw)
         {
            log_error(LOG_LEVEL_FATAL, "User '%s' not found.", user_arg);
         }

         freez(user_arg);
      }

      else if (strcmp(argv[argc_pos], "--pre-chroot-nslookup") == 0)
      {
         if (++argc_pos == argc) usage(argv[0]);
         pre_chroot_nslookup_to_load_resolver = strdup_or_die(argv[argc_pos]);
      }

      else if (strcmp(argv[argc_pos], "--chroot") == 0)
      {
         do_chroot = 1;
      }
#endif /* defined(unix) */

      else if (strcmp(argv[argc_pos], "--config-test") == 0)
      {
         do_config_test = 1;
      }
#ifdef FUZZ
      else if (strcmp(argv[argc_pos], "--fuzz") == 0)
      {
         argc_pos++;
         if (argc < argc_pos + 2) usage(argv[0]);
         fuzz_input_type = argv[argc_pos];
         argc_pos++;
         fuzz_input_file = argv[argc_pos];
      }
      else if (strcmp(argv[argc_pos], "--stfu") == 0)
      {
         set_debug_level(LOG_LEVEL_STFU);
      }
#endif
      else if (argc_pos + 1 != argc)
      {
         /*
          * This is neither the last command line
          * option, nor was it recognized before,
          * therefore it must be invalid.
          */
         usage(argv[0]);
      }
      else

#endif /* defined(_WIN32) && !defined(_WIN_CONSOLE) */
      {
         configfile = argv[argc_pos];
      }

   } /* -END- while (more arguments) */

   show_version(Argv[0]);

#if defined(unix)
   if (*configfile != '/')
   {
      char cwd[BUFFER_SIZE];
      char *abs_file;
      size_t abs_file_size;

      /* make config-filename absolute here */
      if (NULL == getcwd(cwd, sizeof(cwd)))
      {
         perror("failed to get current working directory");
         exit(1);
      }

      basedir = strdup_or_die(cwd);
      /* XXX: why + 5? */
      abs_file_size = strlen(cwd) + strlen(configfile) + 5;
      abs_file = malloc_or_die(abs_file_size);
      strlcpy(abs_file, basedir, abs_file_size);
      strlcat(abs_file, "/", abs_file_size);
      strlcat(abs_file, configfile, abs_file_size);
      configfile = abs_file;
   }
#endif /* defined unix */


   files->next = NULL;
   clients->next = NULL;

   /* XXX: factor out initialising after the next stable release. */
#ifdef _WIN32
   InitWin32();
#endif

#ifndef HAVE_ARC4RANDOM
   random_seed = (unsigned int)time(NULL);
#ifdef HAVE_RANDOM
   srandom(random_seed);
#else
   srand(random_seed);
#endif /* ifdef HAVE_RANDOM */
#endif /* ifndef HAVE_ARC4RANDOM */

   /*
    * Unix signal handling
    *
    * Catch the abort, interrupt and terminate signals for a graceful exit
    * Catch the hangup signal so the errlog can be reopened.
    *
    * Ignore the broken pipe signal as connection failures
    * are handled when and where they occur without relying
    * on a signal.
    */
#if !defined(_WIN32)
{
   int idx;
   const int catched_signals[] = { SIGTERM, SIGINT, SIGHUP };

   for (idx = 0; idx < SZ(catched_signals); idx++)
   {
#ifdef sun /* FIXME: Is it safe to check for HAVE_SIGSET instead? */
      if (sigset(catched_signals[idx], sig_handler) == SIG_ERR)
#else
      if (signal(catched_signals[idx], sig_handler) == SIG_ERR)
#endif /* ifdef sun */
      {
         log_error(LOG_LEVEL_FATAL, "Can't set signal-handler for signal %d: %E", catched_signals[idx]);
      }
   }

   if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      log_error(LOG_LEVEL_FATAL, "Can't set ignore-handler for SIGPIPE: %E");
   }

}
#else /* ifdef _WIN32 */
# ifdef _WIN_CONSOLE
   /*
    * We *are* in a windows console app.
    * Print a verbose messages about FAQ's and such
    */
   printf("%s", win32_blurb);
# endif /* def _WIN_CONSOLE */
#endif /* def _WIN32 */

#ifdef FUZZ
   if (fuzz_input_type != NULL)
   {
      exit(process_fuzzed_input(fuzz_input_type, fuzz_input_file));
   }
   log_error(LOG_LEVEL_FATAL,
      "When compiled with fuzzing support, Privoxy should only be used for fuzzing. "
      "Various data structures are static which is unsafe when using threads.");
#endif

   if (do_config_test)
   {
      exit(NULL == load_config());
   }

   /* Initialize the CGI subsystem */
   cgi_init_error_messages();

   /*
    * If running on unix and without the --no-daemon
    * option, become a daemon. I.e. fork, detach
    * from tty and get process group leadership
    */
#if defined(unix)
{
   if (daemon_mode)
   {
      int fd;
      pid_t pid = fork();

      if (pid < 0) /* error */
      {
         perror("fork");
         exit(3);
      }
      else if (pid != 0) /* parent */
      {
         int status;
         pid_t wpid;
         /*
          * must check for errors
          * child died due to missing files aso
          */
         sleep(1);
         wpid = waitpid(pid, &status, WNOHANG);
         if (wpid != 0)
         {
            exit(1);
         }
         exit(0);
      }
      /* child */

      setsid();

      /*
       * stderr (fd 2) will be closed later on,
       * when the config file has been parsed.
       */
      close(0);
      close(1);

      /*
       * Reserve fd 0 and 1 to prevent abort() and friends
       * from sending stuff to the clients or servers.
       */
      fd = open("/dev/null", O_RDONLY);
      if (fd == -1)
      {
         log_error(LOG_LEVEL_FATAL, "Failed to open /dev/null: %E");
      }
      else if (fd != 0)
      {
         if (dup2(fd, 0) == -1)
         {
            log_error(LOG_LEVEL_FATAL, "Failed to reserve fd 0: %E");
         }
         close(fd);
      }
      fd = open("/dev/null", O_WRONLY);
      if (fd == -1)
      {
         log_error(LOG_LEVEL_FATAL, "Failed to open /dev/null: %E");
      }
      else if (fd != 1)
      {
         if (dup2(fd, 1) == -1)
         {
            log_error(LOG_LEVEL_FATAL, "Failed to reserve fd 1: %E");
         }
         close(fd);
      }

#ifdef FEATURE_EXTERNAL_FILTERS
      for (fd = 0; fd < 3; fd++)
      {
         mark_socket_for_close_on_execute(fd);
      }
#endif

      if (chdir("/") != 0)
      {
         log_error(LOG_LEVEL_FATAL, "Failed to cd into '/': %E");
      }

   } /* -END- if (daemon_mode) */

   /*
    * As soon as we have written the PID file, we can switch
    * to the user and group ID indicated by the --user option
    */
   if (pidfile != NULL)
   {
      write_pid_file(pidfile);
   }
   if (NULL != pw)
   {
      if (setgid((NULL != grp) ? grp->gr_gid : pw->pw_gid))
      {
         log_error(LOG_LEVEL_FATAL, "Cannot setgid(): Insufficient permissions.");
      }
      if (NULL != grp)
      {
         if (setgroups(1, &grp->gr_gid))
         {
            log_error(LOG_LEVEL_FATAL, "setgroups() failed: %E");
         }
      }
      else if (initgroups(pw->pw_name, pw->pw_gid))
      {
         log_error(LOG_LEVEL_FATAL, "initgroups() failed: %E");
      }
      if (do_chroot)
      {
         if (!pw->pw_dir)
         {
            log_error(LOG_LEVEL_FATAL, "Home directory for %s undefined", pw->pw_name);
         }
         /* Read the time zone file from /etc before doing chroot. */
         tzset();
         if (NULL != pre_chroot_nslookup_to_load_resolver
             && '\0' != pre_chroot_nslookup_to_load_resolver[0])
         {
            /* Initialize resolver library. */
            (void) resolve_hostname_to_ip(pre_chroot_nslookup_to_load_resolver);
         }
         if (chroot(pw->pw_dir) < 0)
         {
            log_error(LOG_LEVEL_FATAL, "Cannot chroot to %s", pw->pw_dir);
         }
         if (chdir ("/"))
         {
            log_error(LOG_LEVEL_FATAL, "Cannot chdir /");
         }
      }
      if (setuid(pw->pw_uid))
      {
         log_error(LOG_LEVEL_FATAL, "Cannot setuid(): Insufficient permissions.");
      }
      if (do_chroot)
      {
         char putenv_dummy[64];

         strlcpy(putenv_dummy, "HOME=/", sizeof(putenv_dummy));
         if (putenv(putenv_dummy) != 0)
         {
            log_error(LOG_LEVEL_FATAL, "Cannot putenv(): HOME");
         }

         snprintf(putenv_dummy, sizeof(putenv_dummy), "USER=%s", pw->pw_name);
         if (putenv(putenv_dummy) != 0)
         {
            log_error(LOG_LEVEL_FATAL, "Cannot putenv(): USER");
         }
      }
   }
   else if (do_chroot)
   {
      log_error(LOG_LEVEL_FATAL, "Cannot chroot without --user argument.");
   }
}
#endif /* defined unix */

#ifdef _WIN32
   /* This will be FALSE unless the command line specified --service
    */
   daemon_mode = 0;
   if (bRunAsService)
   {
      daemon_mode = 1;
      /* Yup, so now we must attempt to establish a connection
       * with the service dispatcher. This will only work if this
       * process was launched by the service control manager to
       * actually run as a service. If this isn't the case, i've
       * known it take around 30 seconds or so for the call to return.
       */

      /* The StartServiceCtrlDispatcher won't return until the service is stopping */
      if (w32_start_service_ctrl_dispatcher(w32ServiceDispatchTable))
      {
         /* Service has run, and at this point is now being stopped, so just return */
         return 0;
      }

#ifdef _WIN_CONSOLE
      printf("Warning: Failed to connect to Service Control Dispatcher\nwhen starting as a service!\n");
#endif
      /* An error occurred. Usually it's because --service was wrongly specified
       * and we were unable to connect to the Service Control Dispatcher because
       * it wasn't expecting us and is therefore not listening.
       *
       * For now, just continue below to call the listen_loop function.
       */
   }
#endif /* def _WIN32 */

   listen_loop();

   release_mutexes();

   /* NOTREACHED */
   return(-1);

}


/*********************************************************************
 *
 * Function    :  bind_port_helper
 *
 * Description :  Bind the listen port.  Handles logging, and aborts
 *                on failure.
 *
 * Parameters  :
 *          1  :  haddr = Host address to bind to. Use NULL to bind to
 *                        INADDR_ANY.
 *          2  :  hport = Specifies port to bind to.
 *          3  :  backlog = Listen backlog.
 *
 * Returns     :  Port that was opened.
 *
 *********************************************************************/
static jb_socket bind_port_helper(const char *haddr, int hport, int backlog)
{
   int result;
   jb_socket bfd;

   result = bind_port(haddr, hport, backlog, &bfd);

   if (result < 0)
   {
      const char *bind_address = (NULL != haddr) ? haddr : "INADDR_ANY";
      switch(result)
      {
         case -3:
            log_error(LOG_LEVEL_FATAL,
               "can't bind to %s:%d: There may be another Privoxy "
               "or some other proxy running on port %d",
               bind_address, hport, hport);
            exit(-1);

         case -2:
            log_error(LOG_LEVEL_FATAL,
               "can't bind to %s:%d: The hostname is not resolvable",
               bind_address, hport);
            exit(-1);

         default:
            log_error(LOG_LEVEL_FATAL, "can't bind to %s:%d: %E",
               bind_address, hport);
            exit(-1);
      }

      /* shouldn't get here */
      return JB_INVALID_SOCKET;
   }

#ifndef HAVE_POLL
#ifndef _WIN32
   if (bfd >= FD_SETSIZE)
   {
      log_error(LOG_LEVEL_FATAL,
         "Bind socket number too high to use select(): %d >= %d",
         bfd, FD_SETSIZE);
   }
#endif
#endif

   if (haddr == NULL)
   {
      log_error(LOG_LEVEL_INFO, "Listening on port %d on all IP addresses",
         hport);
   }
   else
   {
      log_error(LOG_LEVEL_INFO, "Listening on port %d on IP address %s",
         hport, haddr);
   }

   return bfd;
}


/*********************************************************************
 *
 * Function    :  bind_ports_helper
 *
 * Description :  Bind the listen ports.  Handles logging, and aborts
 *                on failure.
 *
 * Parameters  :
 *          1  :  config = Privoxy configuration.  Specifies ports
 *                         to bind to.
 *          2  :  sockets = Preallocated array of opened sockets
 *                          corresponding to specification in config.
 *                          All non-opened sockets will be set to
 *                          JB_INVALID_SOCKET.
 *
 * Returns     :  Nothing. Inspect sockets argument.
 *
 *********************************************************************/
static void bind_ports_helper(struct configuration_spec * config,
                              jb_socket sockets[])
{
   int i;

   for (i = 0; i < MAX_LISTENING_SOCKETS; i++)
   {
      if (config->hport[i])
      {
         sockets[i] = bind_port_helper(config->haddr[i],
            config->hport[i], config->listen_backlog);
#if defined(FEATURE_ACCEPT_FILTER) && defined(SO_ACCEPTFILTER)
         if (config->enable_accept_filter && sockets[i] != JB_INVALID_SOCKET)
         {
            struct accept_filter_arg af_options;
            bzero(&af_options, sizeof(af_options));
            strlcpy(af_options.af_name, "httpready", sizeof(af_options.af_name));
            if (setsockopt(sockets[i], SOL_SOCKET, SO_ACCEPTFILTER, &af_options,
                  sizeof(af_options)))
            {
               log_error(LOG_LEVEL_ERROR,
                  "Enabling accept filter for socket %d failed: %E", sockets[i]);
            }
         }
#endif
      }
      else
      {
         sockets[i] = JB_INVALID_SOCKET;
      }
   }
   config->need_bind = 0;
}


/*********************************************************************
 *
 * Function    :  close_ports_helper
 *
 * Description :  Close listenings ports.
 *
 * Parameters  :
 *          1  :  sockets = Array of opened and non-opened sockets to
 *                          close. All sockets will be set to
 *                          JB_INVALID_SOCKET.
 *
 * Returns     :  Nothing.
 *
 *********************************************************************/
static void close_ports_helper(jb_socket sockets[])
{
   int i;

   for (i = 0; i < MAX_LISTENING_SOCKETS; i++)
   {
      if (JB_INVALID_SOCKET != sockets[i])
      {
         close_socket(sockets[i]);
      }
      sockets[i] = JB_INVALID_SOCKET;
   }
}


#ifdef _WIN32
/* Without this simple workaround we get this compiler warning from _beginthread
 *     warning C4028: formal parameter 1 different from declaration
 */
void w32_service_listen_loop(void *p)
{
   listen_loop();
}
#endif /* def _WIN32 */


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
   struct client_states *csp_list = NULL;
   struct client_state *csp = NULL;
   jb_socket bfds[MAX_LISTENING_SOCKETS];
   struct configuration_spec *config;
   unsigned int active_threads = 0;
#if defined(FEATURE_PTHREAD)
   pthread_attr_t attrs;

   pthread_attr_init(&attrs);
   pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
#endif

   config = load_config();

#ifdef FEATURE_CONNECTION_SHARING
   /*
    * XXX: Should be relocated once it no
    * longer needs to emit log messages.
    */
   initialize_reusable_connections();
#endif /* def FEATURE_CONNECTION_SHARING */

   bind_ports_helper(config, bfds);

#ifdef FEATURE_GRACEFUL_TERMINATION
   while (!g_terminate)
#else
   for (;;)
#endif
   {
#if !defined(FEATURE_PTHREAD) && !defined(_WIN32) && !defined(__BEOS__)
      while (waitpid(-1, NULL, WNOHANG) > 0)
      {
         /* zombie children */
      }
#endif /* !defined(FEATURE_PTHREAD) && !defined(_WIN32) && !defined(__BEOS__) */

      /*
       * Free data that was used by died threads
       */
      active_threads = sweep();

#if defined(unix)
      /*
       * Re-open the errlog after HUP signal
       */
      if (received_hup_signal)
      {
         if (NULL != config->logfile)
         {
            init_error_log(Argv[0], config->logfile);
         }
         received_hup_signal = 0;
      }
#endif

      csp_list = zalloc_or_die(sizeof(*csp_list));
      csp = &csp_list->csp;

      log_error(LOG_LEVEL_CONNECT,
         "Waiting for the next client connection. Currently active threads: %u",
         active_threads);

      /*
       * This config may be outdated, but for accept_connection()
       * it's fresh enough.
       */
      csp->config = config;

      if (!accept_connection(csp, bfds))
      {
         log_error(LOG_LEVEL_CONNECT, "accept failed: %E");
         freez(csp_list);
         continue;
      }

      csp->flags |= CSP_FLAG_ACTIVE;
      csp->server_connection.sfd = JB_INVALID_SOCKET;

      csp->config = config = load_config();

      if (config->need_bind)
      {
         /*
          * Since we were listening to the "old port", we will not see
          * a "listen" param change until the next request.  So, at
          * least 1 more request must be made for us to find the new
          * setting.  I am simply closing the old socket and binding the
          * new one.
          *
          * Which-ever is correct, we will serve 1 more page via the
          * old settings.  This should probably be a "show-status"
          * request.  This should not be a so common of an operation
          * that this will hurt people's feelings.
          */

         close_ports_helper(bfds);

         bind_ports_helper(config, bfds);
      }

#ifdef FEATURE_TOGGLE
      if (global_toggle_state)
#endif /* def FEATURE_TOGGLE */
      {
         csp->flags |= CSP_FLAG_TOGGLED_ON;
      }

      if (run_loader(csp))
      {
         log_error(LOG_LEVEL_FATAL, "a loader failed - must exit");
         /* Never get here - LOG_LEVEL_FATAL causes program exit */
      }

#ifdef FEATURE_ACL
      if (block_acl(NULL,csp))
      {
         log_error(LOG_LEVEL_CONNECT,
            "Connection from %s on %s (socket %d) dropped due to ACL",
            csp->ip_addr_str, csp->listen_addr_str, csp->cfd);
         close_socket(csp->cfd);
         freez(csp->ip_addr_str);
         freez(csp->listen_addr_str);
         freez(csp_list);
         continue;
      }
#endif /* def FEATURE_ACL */

      if ((0 != config->max_client_connections)
         && (active_threads >= config->max_client_connections))
      {
         log_error(LOG_LEVEL_ERROR,
            "Rejecting connection from %s. Maximum number of connections reached.",
            csp->ip_addr_str);
         write_socket_delayed(csp->cfd, TOO_MANY_CONNECTIONS_RESPONSE,
            strlen(TOO_MANY_CONNECTIONS_RESPONSE), get_write_delay(csp));
         close_socket(csp->cfd);
         freez(csp->ip_addr_str);
         freez(csp->listen_addr_str);
         freez(csp_list);
         continue;
      }

      /* add it to the list of clients */
      csp_list->next = clients->next;
      clients->next = csp_list;

      if (config->multi_threaded)
      {
         int child_id;

/* this is a switch () statement in the C preprocessor - ugh */
#undef SELECTED_ONE_OPTION

/* Use Pthreads in preference to native code */
#if defined(FEATURE_PTHREAD) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
         {
            pthread_t the_thread;
            int ret;

            ret = pthread_create(&the_thread, &attrs,
               (void * (*)(void *))serve, csp);
            child_id = ret ? -1 : 0;
         }
#endif

#if defined(_WIN32) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
         child_id = _beginthread(
            (void (*)(void *))serve,
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

#if !defined(SELECTED_ONE_OPTION)
         child_id = fork();

         /* This block is only needed when using fork().
          * When using threads, the server thread was
          * created and run by the call to _beginthread().
          */
         if (child_id == 0)   /* child */
         {
            int rc = 0;
#ifdef FEATURE_TOGGLE
            int inherited_toggle_state = global_toggle_state;
#endif /* def FEATURE_TOGGLE */

            serve(csp);

            /*
             * If we've been toggled or we've blocked the request, tell Mom
             */

#ifdef FEATURE_TOGGLE
            if (inherited_toggle_state != global_toggle_state)
            {
               rc |= RC_FLAG_TOGGLED;
            }
#endif /* def FEATURE_TOGGLE */

#ifdef FEATURE_STATISTICS
            if (csp->flags & CSP_FLAG_REJECTED)
            {
               rc |= RC_FLAG_BLOCKED;
            }
#endif /* ndef FEATURE_STATISTICS */

            _exit(rc);
         }
         else if (child_id > 0) /* parent */
         {
            /* in a fork()'d environment, the parent's
             * copy of the client socket and the CSP
             * are not used.
             */
            int child_status;
#if !defined(_WIN32) && !defined(__CYGWIN__)

            wait(&child_status);

            /*
             * Evaluate child's return code: If the child has
             *  - been toggled, toggle ourselves
             *  - blocked its request, bump up the stats counter
             */

#ifdef FEATURE_TOGGLE
            if (WIFEXITED(child_status) && (WEXITSTATUS(child_status) & RC_FLAG_TOGGLED))
            {
               global_toggle_state = !global_toggle_state;
            }
#endif /* def FEATURE_TOGGLE */

#ifdef FEATURE_STATISTICS
            urls_read++;
            if (WIFEXITED(child_status) && (WEXITSTATUS(child_status) & RC_FLAG_BLOCKED))
            {
               urls_rejected++;
            }
#endif /* def FEATURE_STATISTICS */

#endif /* !defined(_WIN32) && defined(__CYGWIN__) */
            close_socket(csp->cfd);
            csp->flags &= ~CSP_FLAG_ACTIVE;
         }
#endif

#undef SELECTED_ONE_OPTION
/* end of cpp switch () */

         if (child_id < 0)
         {
            /*
             * Spawning the child failed, assume it's because
             * there are too many children running already.
             * XXX: If you assume ...
             */
            log_error(LOG_LEVEL_ERROR,
               "Unable to take any additional connections: %E. Active threads: %u",
               active_threads);
            write_socket_delayed(csp->cfd, TOO_MANY_CONNECTIONS_RESPONSE,
               strlen(TOO_MANY_CONNECTIONS_RESPONSE), get_write_delay(csp));
            close_socket(csp->cfd);
            csp->flags &= ~CSP_FLAG_ACTIVE;
         }
      }
      else
      {
         serve(csp);
      }
   }

#if defined(FEATURE_PTHREAD)
   pthread_attr_destroy(&attrs);
#endif

   /* NOTREACHED unless FEATURE_GRACEFUL_TERMINATION is defined */

#ifdef FEATURE_GRACEFUL_TERMINATION

   log_error(LOG_LEVEL_INFO, "Graceful termination requested.");

   close_ports_helper(bfds);

   unload_current_config_file();
   unload_current_actions_file();
   unload_current_re_filterfile();
#ifdef FEATURE_TRUST
   unload_current_trust_file();
#endif

   if (config->multi_threaded)
   {
      int i = 60;
      do
      {
         sleep(1);
         sweep();
      } while ((clients->next != NULL) && (--i > 0));

      if (i <= 0)
      {
         log_error(LOG_LEVEL_ERROR, "Graceful termination failed "
            "- still some live clients after 1 minute wait.");
      }
   }
   sweep();
   sweep();

#if defined(unix)
   freez(basedir);
#endif

#ifdef FEATURE_HTTPS_INSPECTION
   /*
    * Only release TLS backed resources if there
    * are no active connections left.
    */
   if (clients->next == NULL)
   {
      ssl_release();
   }
#endif

   log_error(LOG_LEVEL_INFO, "Exiting gracefully.");

#if defined(_WIN32) && !defined(_WIN_CONSOLE)
   /* Cleanup - remove taskbar icon etc. */
   TermLogWindow();
#endif

   exit(0);
#endif /* FEATURE_GRACEFUL_TERMINATION */

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
