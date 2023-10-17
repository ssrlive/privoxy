/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/gateway.c,v $
 *
 * Purpose     :  Contains functions to connect to a server, possibly
 *                using a "forwarder" (i.e. HTTP proxy and/or a SOCKS4
 *                or SOCKS5 proxy).
 *
 * Copyright   :  Written by and Copyright (C) 2001-2021 the
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

#ifndef _WIN32
#include <netinet/in.h>
#endif

#include <errno.h>
#include <string.h>
#include "assert.h"

#ifdef _WIN32
#include <winsock2.h>
#endif /* def _WIN32 */

#ifdef __BEOS__
#include <netdb.h>
#endif /* def __BEOS__ */

#include "project.h"
#include "jcc.h"
#include "errlog.h"
#include "jbsockets.h"
#include "gateway.h"
#include "miscutil.h"
#include "list.h"
#include "parsers.h"

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
#ifdef HAVE_POLL
#ifdef __GLIBC__
#include <sys/poll.h>
#else
#include <poll.h>
#endif /* def __GLIBC__ */
#endif /* HAVE_POLL */
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

static jb_socket socks4_connect(const struct forward_spec *fwd,
                                const char *target_host,
                                int target_port,
                                struct client_state *csp);

static jb_socket socks5_connect(const struct forward_spec *fwd,
                                const char *target_host,
                                int target_port,
                                struct client_state *csp);

enum {
   SOCKS4_REQUEST_GRANTED        =  90,
   SOCKS4_REQUEST_REJECT         =  91,
   SOCKS4_REQUEST_IDENT_FAILED   =  92,
   SOCKS4_REQUEST_IDENT_CONFLICT =  93
};

enum {
   SOCKS5_REQUEST_GRANTED             = 0,
   SOCKS5_REQUEST_FAILED              = 1,
   SOCKS5_REQUEST_DENIED              = 2,
   SOCKS5_REQUEST_NETWORK_UNREACHABLE = 3,
   SOCKS5_REQUEST_HOST_UNREACHABLE    = 4,
   SOCKS5_REQUEST_CONNECTION_REFUSED  = 5,
   SOCKS5_REQUEST_TTL_EXPIRED         = 6,
   SOCKS5_REQUEST_PROTOCOL_ERROR      = 7,
   SOCKS5_REQUEST_BAD_ADDRESS_TYPE    = 8
};

/* structure of a socks client operation */
struct socks_op {
   unsigned char vn;          /* socks version number */
   unsigned char cd;          /* command code */
   unsigned char dstport[2];  /* destination port */
   unsigned char dstip[4];    /* destination address */
   char userid;               /* first byte of userid */
   char padding[3];           /* make sure sizeof(struct socks_op) is endian-independent. */
   /* more bytes of the userid follow, terminated by a NULL */
};

/* structure of a socks server reply */
struct socks_reply {
   unsigned char vn;          /* socks version number */
   unsigned char cd;          /* command code */
   unsigned char dstport[2];  /* destination port */
   unsigned char dstip[4];    /* destination address */
};

static const char socks_userid[] = "anonymous";

#ifdef FEATURE_CONNECTION_SHARING
#ifndef FEATURE_CONNECTION_KEEP_ALIVE
#error Using FEATURE_CONNECTION_SHARING without FEATURE_CONNECTION_KEEP_ALIVE is impossible
#endif

#define MAX_REUSABLE_CONNECTIONS 100

static struct reusable_connection reusable_connection[MAX_REUSABLE_CONNECTIONS];
static int mark_connection_unused(const struct reusable_connection *connection);

/*********************************************************************
 *
 * Function    :  initialize_reusable_connections
 *
 * Description :  Initializes the reusable_connection structures.
 *                Must be called with connection_reuse_mutex locked.
 *
 * Parameters  : N/A
 *
 * Returns     : void
 *
 *********************************************************************/
extern void initialize_reusable_connections(void)
{
   unsigned int slot = 0;

#if !defined(HAVE_POLL) && !defined(_WIN32)
   log_error(LOG_LEVEL_INFO,
      "Detecting already dead connections might not work "
      "correctly on your platform. In case of problems, "
      "unset the keep-alive-timeout option.");
#endif

   for (slot = 0; slot < SZ(reusable_connection); slot++)
   {
      mark_connection_closed(&reusable_connection[slot]);
   }

   log_error(LOG_LEVEL_CONNECT, "Initialized %d socket slots.", slot);
}


/*********************************************************************
 *
 * Function    :  remember_connection
 *
 * Description :  Remembers a server connection for reuse later on.
 *
 * Parameters  :
 *          1  :  connection = The server connection to remember.
 *
 * Returns     : void
 *
 *********************************************************************/
void remember_connection(const struct reusable_connection *connection)
{
   unsigned int slot = 0;
   int free_slot_found = FALSE;

   assert(NULL != connection);
   assert(connection->sfd != JB_INVALID_SOCKET);

   if (mark_connection_unused(connection))
   {
      return;
   }

   privoxy_mutex_lock(&connection_reuse_mutex);

   /* Find free socket slot. */
   for (slot = 0; slot < SZ(reusable_connection); slot++)
   {
      if (reusable_connection[slot].sfd == JB_INVALID_SOCKET)
      {
         assert(reusable_connection[slot].in_use == 0);
         log_error(LOG_LEVEL_CONNECT,
            "Remembering socket %d for %s:%d in slot %d.",
            connection->sfd, connection->host, connection->port, slot);
         free_slot_found = TRUE;
         break;
      }
   }

   if (!free_slot_found)
   {
      log_error(LOG_LEVEL_CONNECT,
        "No free slots found to remember socket for %s:%d. Last slot %d.",
        connection->host, connection->port, slot);
      privoxy_mutex_unlock(&connection_reuse_mutex);
      close_socket(connection->sfd);
      return;
   }

   assert(slot < SZ(reusable_connection));
   assert(NULL != connection->host);
   reusable_connection[slot].host = strdup_or_die(connection->host);
   reusable_connection[slot].sfd = connection->sfd;
   reusable_connection[slot].port = connection->port;
   reusable_connection[slot].in_use = 0;
   reusable_connection[slot].timestamp = connection->timestamp;
   reusable_connection[slot].request_sent = connection->request_sent;
   reusable_connection[slot].response_received = connection->response_received;
   reusable_connection[slot].keep_alive_timeout = connection->keep_alive_timeout;
   reusable_connection[slot].requests_sent_total = connection->requests_sent_total;

   assert(reusable_connection[slot].gateway_host == NULL);
   assert(reusable_connection[slot].gateway_port == 0);
   assert(reusable_connection[slot].auth_username == NULL);
   assert(reusable_connection[slot].auth_password == NULL);
   assert(reusable_connection[slot].forwarder_type == SOCKS_NONE);
   assert(reusable_connection[slot].forward_host == NULL);
   assert(reusable_connection[slot].forward_port == 0);

   reusable_connection[slot].forwarder_type = connection->forwarder_type;
   if (NULL != connection->gateway_host)
   {
      reusable_connection[slot].gateway_host = strdup_or_die(connection->gateway_host);
   }
   else
   {
      reusable_connection[slot].gateway_host = NULL;
   }
   reusable_connection[slot].gateway_port = connection->gateway_port;
   if (NULL != connection->auth_username)
   {
      reusable_connection[slot].auth_username = strdup_or_die(connection->auth_username);
   }
   else
   {
      reusable_connection[slot].auth_username = NULL;
   }
   if (NULL != connection->auth_password)
   {
      reusable_connection[slot].auth_password = strdup_or_die(connection->auth_password);
   }
   else
   {
      reusable_connection[slot].auth_password = NULL;
   }

   if (NULL != connection->forward_host)
   {
      reusable_connection[slot].forward_host = strdup_or_die(connection->forward_host);
   }
   else
   {
      reusable_connection[slot].forward_host = NULL;
   }
   reusable_connection[slot].forward_port = connection->forward_port;

   privoxy_mutex_unlock(&connection_reuse_mutex);
}
#endif /* def FEATURE_CONNECTION_SHARING */


/*********************************************************************
 *
 * Function    :  mark_connection_closed
 *
 * Description : Marks a reused connection closed.
 *
 * Parameters  :
 *          1  :  closed_connection = The connection to mark as closed.
 *
 * Returns     : void
 *
 *********************************************************************/
void mark_connection_closed(struct reusable_connection *closed_connection)
{
   closed_connection->in_use = FALSE;
   closed_connection->sfd = JB_INVALID_SOCKET;
   freez(closed_connection->host);
   closed_connection->port = 0;
   closed_connection->timestamp = 0;
   closed_connection->request_sent = 0;
   closed_connection->response_received = 0;
   closed_connection->keep_alive_timeout = 0;
   closed_connection->requests_sent_total = 0;
   closed_connection->forwarder_type = SOCKS_NONE;
   freez(closed_connection->gateway_host);
   closed_connection->gateway_port = 0;
   freez(closed_connection->auth_username);
   freez(closed_connection->auth_password);
   freez(closed_connection->forward_host);
   closed_connection->forward_port = 0;
}


#ifdef FEATURE_CONNECTION_SHARING
/*********************************************************************
 *
 * Function    :  forget_connection
 *
 * Description :  Removes a previously remembered connection from
 *                the list of reusable connections.
 *
 * Parameters  :
 *          1  :  sfd = The socket belonging to the connection in question.
 *
 * Returns     : void
 *
 *********************************************************************/
void forget_connection(jb_socket sfd)
{
   unsigned int slot = 0;

   assert(sfd != JB_INVALID_SOCKET);

   privoxy_mutex_lock(&connection_reuse_mutex);

   for (slot = 0; slot < SZ(reusable_connection); slot++)
   {
      if (reusable_connection[slot].sfd == sfd)
      {
         assert(reusable_connection[slot].in_use);

         log_error(LOG_LEVEL_CONNECT,
            "Forgetting socket %d for %s:%d in slot %d.",
            sfd, reusable_connection[slot].host,
            reusable_connection[slot].port, slot);
         mark_connection_closed(&reusable_connection[slot]);
         break;
      }
   }

   privoxy_mutex_unlock(&connection_reuse_mutex);

}
#endif /* def FEATURE_CONNECTION_SHARING */


#ifdef FEATURE_CONNECTION_KEEP_ALIVE
/*********************************************************************
 *
 * Function    :  string_or_none
 *
 * Description :  Returns a given string or "none" if a NULL pointer
 *                is given.
 *                Helper function for connection_destination_matches().
 *
 * Parameters  :
 *          1  :  string = The string to check.
 *
 * Returns     :  The string if non-NULL, "none" otherwise.
 *
 *********************************************************************/
static const char *string_or_none(const char *string)
{
   return(string != NULL ? string : "none");
}


/*********************************************************************
 *
 * Function    :  connection_detail_matches
 *
 * Description :  Helper function for connection_destination_matches().
 *                Compares strings which can be NULL.
 *
 * Parameters  :
 *          1  :  connection_detail = The connection detail to compare.
 *          2  :  fowarder_detail = The forwarder detail to compare.
 *
 * Returns     :  TRUE for yes, FALSE otherwise.
 *
 *********************************************************************/
static int connection_detail_matches(const char *connection_detail,
                                     const char *forwarder_detail)
{
   if (connection_detail == NULL && forwarder_detail == NULL)
   {
      /* Both details are unset. */
      return TRUE;
   }

   if ((connection_detail == NULL && forwarder_detail != NULL)
    || (connection_detail != NULL && forwarder_detail == NULL))
   {
      /* Only one detail isn't set. */
      return FALSE;
   }

   /* Both details are set, but do they match? */
   return(!strcmpic(connection_detail, forwarder_detail));

}


/*********************************************************************
 *
 * Function    :  connection_destination_matches
 *
 * Description :  Determines whether a remembered connection can
 *                be reused. That is, whether the destination and
 *                the forwarding settings match.
 *
 * Parameters  :
 *          1  :  connection = The connection to check.
 *          2  :  http = The destination for the connection.
 *          3  :  fwd  = The forwarder settings.
 *
 * Returns     :  TRUE for yes, FALSE otherwise.
 *
 *********************************************************************/
int connection_destination_matches(const struct reusable_connection *connection,
                                   const struct http_request *http,
                                   const struct forward_spec *fwd)
{
   if ((connection->forwarder_type != fwd->type)
    || (connection->gateway_port   != fwd->gateway_port)
    || (connection->forward_port   != fwd->forward_port)
    || (connection->port           != http->port))
   {
      return FALSE;
   }

   if (!connection_detail_matches(connection->gateway_host, fwd->gateway_host))
   {
      log_error(LOG_LEVEL_CONNECT,
         "Gateway mismatch. Previous gateway: %s. Current gateway: %s",
         string_or_none(connection->gateway_host),
         string_or_none(fwd->gateway_host));
      return FALSE;
   }

   if (!connection_detail_matches(connection->auth_username, fwd->auth_username))
   {
      log_error(LOG_LEVEL_CONNECT, "Socks user name mismatch. "
         "Previous user name: %s. Current user name: %s",
         string_or_none(connection->auth_username),
         string_or_none(fwd->auth_username));
      return FALSE;
   }

   if (!connection_detail_matches(connection->auth_password, fwd->auth_password))
   {
      log_error(LOG_LEVEL_CONNECT, "Socks user name mismatch. "
         "Previous password: %s. Current password: %s",
         string_or_none(connection->auth_password),
         string_or_none(fwd->auth_password));
      return FALSE;
   }

   if (!connection_detail_matches(connection->forward_host, fwd->forward_host))
   {
      log_error(LOG_LEVEL_CONNECT,
         "Forwarding proxy mismatch. Previous proxy: %s. Current proxy: %s",
         string_or_none(connection->forward_host),
         string_or_none(fwd->forward_host));
      return FALSE;
   }

   return (!strcmpic(connection->host, http->host));

}
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */


#ifdef FEATURE_CONNECTION_SHARING
/*********************************************************************
 *
 * Function    :  close_unusable_connections
 *
 * Description :  Closes remembered connections that have timed
 *                out or have been closed on the other side.
 *
 * Parameters  :  none
 *
 * Returns     :  Number of connections that are still alive.
 *
 *********************************************************************/
int close_unusable_connections(void)
{
   unsigned int slot = 0;
   int connections_alive = 0;

   privoxy_mutex_lock(&connection_reuse_mutex);

   for (slot = 0; slot < SZ(reusable_connection); slot++)
   {
      if (!reusable_connection[slot].in_use
         && (JB_INVALID_SOCKET != reusable_connection[slot].sfd))
      {
         time_t time_open = time(NULL) - reusable_connection[slot].timestamp;
         time_t latency = (reusable_connection[slot].response_received -
            reusable_connection[slot].request_sent) / 2;

         if (reusable_connection[slot].keep_alive_timeout < time_open + latency)
         {
            log_error(LOG_LEVEL_CONNECT,
               "The connection to %s:%d in slot %d timed out. "
               "Closing socket %d. Timeout is: %d. Assumed latency: %ld.",
               reusable_connection[slot].host,
               reusable_connection[slot].port, slot,
               reusable_connection[slot].sfd,
               reusable_connection[slot].keep_alive_timeout,
               latency);
            close_socket(reusable_connection[slot].sfd);
            mark_connection_closed(&reusable_connection[slot]);
         }
         else if (!socket_is_still_alive(reusable_connection[slot].sfd))
         {
            log_error(LOG_LEVEL_CONNECT,
               "The connection to %s:%d in slot %d is no longer usable. "
               "Closing socket %d.", reusable_connection[slot].host,
               reusable_connection[slot].port, slot,
               reusable_connection[slot].sfd);
            close_socket(reusable_connection[slot].sfd);
            mark_connection_closed(&reusable_connection[slot]);
         }
         else
         {
            connections_alive++;
         }
      }
   }

   privoxy_mutex_unlock(&connection_reuse_mutex);

   return connections_alive;

}


/*********************************************************************
 *
 * Function    :  get_reusable_connection
 *
 * Description :  Returns an open socket to a previously remembered
 *                open connection (if there is one).
 *
 * Parameters  :
 *          1  :  http = The destination for the connection.
 *          2  :  fwd  = The forwarder settings.
 *
 * Returns     :  JB_INVALID_SOCKET => No reusable connection found,
 *                otherwise a usable socket.
 *
 *********************************************************************/
static jb_socket get_reusable_connection(const struct http_request *http,
                                         const struct forward_spec *fwd)
{
   jb_socket sfd = JB_INVALID_SOCKET;
   unsigned int slot = 0;

   close_unusable_connections();

   privoxy_mutex_lock(&connection_reuse_mutex);

   for (slot = 0; slot < SZ(reusable_connection); slot++)
   {
      if (!reusable_connection[slot].in_use
         && (JB_INVALID_SOCKET != reusable_connection[slot].sfd))
      {
         if (connection_destination_matches(&reusable_connection[slot], http, fwd))
         {
            reusable_connection[slot].in_use = TRUE;
            sfd = reusable_connection[slot].sfd;
            log_error(LOG_LEVEL_CONNECT,
               "Found reusable socket %d for %s:%d in slot %d. Timestamp made %ld "
               "seconds ago. Timeout: %d. Latency: %d. Requests served: %d",
               sfd, reusable_connection[slot].host, reusable_connection[slot].port,
               slot, time(NULL) - reusable_connection[slot].timestamp,
               reusable_connection[slot].keep_alive_timeout,
               (int)(reusable_connection[slot].response_received -
               reusable_connection[slot].request_sent),
               reusable_connection[slot].requests_sent_total);
            break;
         }
      }
   }

   privoxy_mutex_unlock(&connection_reuse_mutex);

   return sfd;

}


/*********************************************************************
 *
 * Function    :  mark_connection_unused
 *
 * Description :  Gives a remembered connection free for reuse.
 *
 * Parameters  :
 *          1  :  connection = The connection in question.
 *
 * Returns     :  TRUE => Socket found and marked as unused.
 *                FALSE => Socket not found.
 *
 *********************************************************************/
static int mark_connection_unused(const struct reusable_connection *connection)
{
   unsigned int slot = 0;
   int socket_found = FALSE;

   assert(connection->sfd != JB_INVALID_SOCKET);

   privoxy_mutex_lock(&connection_reuse_mutex);

   for (slot = 0; slot < SZ(reusable_connection); slot++)
   {
      if (reusable_connection[slot].sfd == connection->sfd)
      {
         assert(reusable_connection[slot].in_use);
         socket_found = TRUE;
         log_error(LOG_LEVEL_CONNECT,
            "Marking open socket %d for %s:%d in slot %d as unused.",
            connection->sfd, reusable_connection[slot].host,
            reusable_connection[slot].port, slot);
         reusable_connection[slot].in_use = 0;
         reusable_connection[slot].timestamp = connection->timestamp;
         break;
      }
   }

   privoxy_mutex_unlock(&connection_reuse_mutex);

   return socket_found;

}
#endif /* def FEATURE_CONNECTION_SHARING */


/*********************************************************************
 *
 * Function    :  forwarded_connect
 *
 * Description :  Connect to a specified web server, possibly via
 *                a HTTP proxy and/or a SOCKS proxy.
 *
 * Parameters  :
 *          1  :  fwd = the proxies to use when connecting.
 *          2  :  http = the http request and apropos headers
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_INVALID_SOCKET => failure, else it is the socket file descriptor.
 *
 *********************************************************************/
jb_socket forwarded_connect(const struct forward_spec *fwd,
                            struct http_request *http,
                            struct client_state *csp)
{
   const char *dest_host;
   int dest_port;
   jb_socket sfd = JB_INVALID_SOCKET;

#ifdef FEATURE_CONNECTION_SHARING
   if ((csp->config->feature_flags & RUNTIME_FEATURE_CONNECTION_SHARING)
      && !(csp->flags & CSP_FLAG_SERVER_SOCKET_TAINTED))
   {
      sfd = get_reusable_connection(http, fwd);
      if (JB_INVALID_SOCKET != sfd)
      {
         return sfd;
      }
   }
#endif /* def FEATURE_CONNECTION_SHARING */

   /* Figure out if we need to connect to the web server or a HTTP proxy. */
   if (fwd->forward_host)
   {
      /* HTTP proxy */
      dest_host = fwd->forward_host;
      dest_port = fwd->forward_port;
   }
   else
   {
      /* Web server */
      dest_host = http->host;
      dest_port = http->port;
   }

   /* Connect, maybe using a SOCKS proxy */
   switch (fwd->type)
   {
      case SOCKS_NONE:
      case FORWARD_WEBSERVER:
         sfd = connect_to(dest_host, dest_port, csp);
         break;
      case SOCKS_4:
      case SOCKS_4A:
         sfd = socks4_connect(fwd, dest_host, dest_port, csp);
         break;
      case SOCKS_5:
      case SOCKS_5T:
         sfd = socks5_connect(fwd, dest_host, dest_port, csp);
         break;
      default:
         /* Should never get here */
         log_error(LOG_LEVEL_FATAL,
            "Internal error in forwarded_connect(). Bad proxy type: %d", fwd->type);
   }

   if (JB_INVALID_SOCKET != sfd)
   {
      log_error(LOG_LEVEL_CONNECT,
         "Created new connection to %s:%d on socket %d.",
         http->host, http->port, sfd);
   }

   return sfd;

}


#ifdef FUZZ
/*********************************************************************
 *
 * Function    :  socks_fuzz
 *
 * Description :  Wrapper around socks[45]_connect() used for fuzzing.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_ERR_OK or JB_ERR_PARSE
 *
 *********************************************************************/
extern jb_err socks_fuzz(struct client_state *csp)
{
   jb_socket socket;
   static struct forward_spec fwd;
   char target_host[] = "fuzz.example.org";
   int target_port = 12345;

   fwd.gateway_host = strdup_or_die("fuzz.example.org");
   fwd.gateway_port = 12345;

   fwd.type = SOCKS_4A;
   socket = socks4_connect(&fwd, target_host, target_port, csp);

   if (JB_INVALID_SOCKET != socket)
   {
      fwd.type = SOCKS_5;
      socket = socks5_connect(&fwd, target_host, target_port, csp);
   }

   if (JB_INVALID_SOCKET == socket)
   {
      log_error(LOG_LEVEL_ERROR, "%s", csp->error_message);
      return JB_ERR_PARSE;
   }

   log_error(LOG_LEVEL_INFO, "Input looks like an acceptable socks response");

   return JB_ERR_OK;

}
#endif

/*********************************************************************
 *
 * Function    :  socks4_connect
 *
 * Description :  Connect to the SOCKS server, and connect through
 *                it to the specified server.   This handles
 *                all the SOCKS negotiation, and returns a file
 *                descriptor for a socket which can be treated as a
 *                normal (non-SOCKS) socket.
 *
 *                Logged error messages are saved to csp->error_message
 *                and later reused by error_response() for the CGI
 *                message. strdup allocation failures are handled there.
 *
 * Parameters  :
 *          1  :  fwd = Specifies the SOCKS proxy to use.
 *          2  :  target_host = The final server to connect to.
 *          3  :  target_port = The final port to connect to.
 *          4  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_INVALID_SOCKET => failure, else a socket file descriptor.
 *
 *********************************************************************/
static jb_socket socks4_connect(const struct forward_spec *fwd,
                                const char *target_host,
                                int target_port,
                                struct client_state *csp)
{
   unsigned long web_server_addr;
   char buf[BUFFER_SIZE];
   struct socks_op    *c = (struct socks_op    *)buf;
   struct socks_reply *s = (struct socks_reply *)buf;
   size_t n;
   size_t csiz;
   jb_socket sfd;
   int err = 0;
   char *errstr = NULL;

   if ((fwd->gateway_host == NULL) || (*fwd->gateway_host == '\0'))
   {
      /* XXX: Shouldn't the config file parser prevent this? */
      errstr = "NULL gateway host specified.";
      err = 1;
   }

   if (fwd->gateway_port <= 0)
   {
      errstr = "invalid gateway port specified.";
      err = 1;
   }

   if (err)
   {
      log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
      csp->error_message = strdup(errstr);
      errno = EINVAL;
      return(JB_INVALID_SOCKET);
   }

   /* build a socks request for connection to the web server */

   strlcpy(&(c->userid), socks_userid, sizeof(buf) - sizeof(struct socks_op));

   csiz = sizeof(*c) + sizeof(socks_userid) - sizeof(c->userid) - sizeof(c->padding);

   switch (fwd->type)
   {
      case SOCKS_4:
         web_server_addr = resolve_hostname_to_ip(target_host);
         if (web_server_addr == INADDR_NONE)
         {
            errstr = "could not resolve target host";
            log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s %s", errstr, target_host);
            err = 1;
         }
         else
         {
            web_server_addr = htonl(web_server_addr);
         }
         break;
      case SOCKS_4A:
         web_server_addr = 0x00000001;
         n = csiz + strlen(target_host) + 1;
         if (n > sizeof(buf))
         {
            errno = EINVAL;
            errstr = "buffer cbuf too small.";
            log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
            err = 1;
         }
         else
         {
            strlcpy(buf + csiz, target_host, sizeof(buf) - sizeof(struct socks_op) - csiz);
            /*
             * What we forward to the socks4a server should have the
             * size of socks_op, plus the length of the userid plus
             * its \0 byte (which we don't have to add because the
             * first byte of the userid is counted twice as it's also
             * part of sock_op) minus the padding bytes (which are part
             * of the userid as well), plus the length of the target_host
             * (which is stored csiz bytes after the beginning of the buffer),
             * plus another \0 byte.
             */
            assert(n == sizeof(struct socks_op) + strlen(&(c->userid)) - sizeof(c->padding) + strlen(buf + csiz) + 1);
            csiz = n;
         }
         break;
      default:
         /* Should never get here */
         log_error(LOG_LEVEL_FATAL,
            "socks4_connect: SOCKS4 impossible internal error - bad SOCKS type.");
         /* Not reached */
         return(JB_INVALID_SOCKET);
   }

   if (err)
   {
      csp->error_message = strdup(errstr);
      return(JB_INVALID_SOCKET);
   }

   c->vn          = 4;
   c->cd          = 1;
   c->dstport[0]  = (unsigned char)((target_port       >> 8 ) & 0xff);
   c->dstport[1]  = (unsigned char)((target_port            ) & 0xff);
   c->dstip[0]    = (unsigned char)((web_server_addr   >> 24) & 0xff);
   c->dstip[1]    = (unsigned char)((web_server_addr   >> 16) & 0xff);
   c->dstip[2]    = (unsigned char)((web_server_addr   >>  8) & 0xff);
   c->dstip[3]    = (unsigned char)((web_server_addr        ) & 0xff);

#ifdef FUZZ
   sfd = 0;
#else
   /* pass the request to the socks server */
   sfd = connect_to(fwd->gateway_host, fwd->gateway_port, csp);

   if (sfd == JB_INVALID_SOCKET)
   {
      /* The error an its reason have already been logged by connect_to()  */
      return(JB_INVALID_SOCKET);
   }
   else if (write_socket(sfd, (char *)c, csiz))
   {
      errstr = "SOCKS4 negotiation write failed.";
      log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
      err = 1;
      close_socket(sfd);
   }
   else if (!data_is_available(sfd, csp->config->socket_timeout))
   {
      if (socket_is_still_alive(sfd))
      {
         errstr = "SOCKS4 negotiation timed out";
      }
      else
      {
         errstr = "SOCKS4 negotiation got aborted by the server";
      }
      log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
      err = 1;
      close_socket(sfd);
   }
   else
#endif
       if (read_socket(sfd, buf, sizeof(buf)) != sizeof(*s))
   {
      errstr = "SOCKS4 negotiation read failed.";
      log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
      err = 1;
      close_socket(sfd);
   }

   if (err)
   {
      csp->error_message = strdup(errstr);
      return(JB_INVALID_SOCKET);
   }

   switch (s->cd)
   {
      case SOCKS4_REQUEST_GRANTED:
         return(sfd);
      case SOCKS4_REQUEST_REJECT:
         errstr = "SOCKS request rejected or failed.";
         errno = EINVAL;
         break;
      case SOCKS4_REQUEST_IDENT_FAILED:
         errstr = "SOCKS request rejected because "
            "SOCKS server cannot connect to identd on the client.";
         errno = EACCES;
         break;
      case SOCKS4_REQUEST_IDENT_CONFLICT:
         errstr = "SOCKS request rejected because "
            "the client program and identd report "
            "different user-ids.";
         errno = EACCES;
         break;
      default:
         errno = ENOENT;
         snprintf(buf, sizeof(buf),
            "SOCKS request rejected for reason code %d.", s->cd);
         errstr = buf;
   }

   log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s", errstr);
   csp->error_message = strdup(errstr);
   close_socket(sfd);

   return(JB_INVALID_SOCKET);

}

/*********************************************************************
 *
 * Function    :  translate_socks5_error
 *
 * Description :  Translates a SOCKS errors to a string.
 *
 * Parameters  :
 *          1  :  socks_error = The error code to translate.
 *
 * Returns     :  The string translation.
 *
 *********************************************************************/
static const char *translate_socks5_error(int socks_error)
{
   switch (socks_error)
   {
      /* XXX: these should be more descriptive */
      case SOCKS5_REQUEST_FAILED:
         return "SOCKS5 request failed";
      case SOCKS5_REQUEST_DENIED:
         return "SOCKS5 request denied";
      case SOCKS5_REQUEST_NETWORK_UNREACHABLE:
         return "SOCKS5 network unreachable";
      case SOCKS5_REQUEST_HOST_UNREACHABLE:
         return "SOCKS5 destination host unreachable";
      case SOCKS5_REQUEST_CONNECTION_REFUSED:
         return "SOCKS5 connection refused";
      case SOCKS5_REQUEST_TTL_EXPIRED:
         return "SOCKS5 TTL expired";
      case SOCKS5_REQUEST_PROTOCOL_ERROR:
         return "SOCKS5 client protocol error";
      case SOCKS5_REQUEST_BAD_ADDRESS_TYPE:
         return "SOCKS5 domain names unsupported";
      case SOCKS5_REQUEST_GRANTED:
         return "everything's peachy";
      default:
         return "SOCKS5 negotiation protocol error";
   }
}


/*********************************************************************
 *
 * Function    :  convert_ipv4_address_to_bytes
 *
 * Description :  Converts an IPv4 address from string to bytes.
 *
 * Parameters  :
 *          1  :  address = The IPv4 address string to convert.
 *          2  :  buf = The buffer to write the bytes to.
 *                      Must be at least four bytes long.
 *
 * Returns     :  JB_ERR_OK on success, JB_ERR_PARSE otherwise.
 *
 *********************************************************************/
static jb_err convert_ipv4_address_to_bytes(const char *address, char *buf)
{
   int i;
   const char *p = address;

   for (i = 0; i < 4; i++)
   {
      unsigned byte;
      if (1 != sscanf(p, "%u", &byte))
      {
         return JB_ERR_PARSE;
      }
      if (byte > 255)
      {
         return JB_ERR_PARSE;
      }
      buf[i] = (char)byte;
      if (i < 3)
      {
         p = strstr(p, ".");
         if (p == NULL)
         {
            return JB_ERR_PARSE;
         }
         p++;
      }
   }

   return JB_ERR_OK;

}


/*********************************************************************
 *
 * Function    :  read_socks_reply
 *
 * Description :  Read from a socket connected to a socks server.
 *
 * Parameters  :
 *          1  :  sfd = file descriptor of the socket to read
 *          2  :  buf = pointer to buffer where data will be written
 *                Must be >= len bytes long.
 *          3  :  len = maximum number of bytes to read
 *          4  :  timeout = Number of seconds to wait.
 *
 * Returns     :  On success, the number of bytes read is returned (zero
 *                indicates end of file), and the file position is advanced
 *                by this number.  It is not an error if this number is
 *                smaller than the number of bytes requested; this may hap-
 *                pen for example because fewer bytes are actually available
 *                right now (maybe because we were close to end-of-file, or
 *                because we are reading from a pipe, or from a terminal,
 *                or because read() was interrupted by a signal).  On error,
 *                -1 is returned, and errno is set appropriately.  In this
 *                case it is left unspecified whether the file position (if
 *                any) changes.
 *
 *********************************************************************/
static int read_socks_reply(jb_socket sfd, char *buf, int len, int timeout)
{
   if (!data_is_available(sfd, timeout))
   {
      if (socket_is_still_alive(sfd))
      {
         log_error(LOG_LEVEL_ERROR,
            "The socks connection timed out after %d seconds.", timeout);
      }
      else
      {
         log_error(LOG_LEVEL_ERROR, "The socks server hung "
            "up the connection without sending a response.");
      }
      return -1;
   }

   return read_socket(sfd, buf, len);

}


/*********************************************************************
 *
 * Function    :  socks5_connect
 *
 * Description :  Connect to the SOCKS server, and connect through
 *                it to the specified server.   This handles
 *                all the SOCKS negotiation, and returns a file
 *                descriptor for a socket which can be treated as a
 *                normal (non-SOCKS) socket.
 *
 * Parameters  :
 *          1  :  fwd = Specifies the SOCKS proxy to use.
 *          2  :  target_host = The final server to connect to.
 *          3  :  target_port = The final port to connect to.
 *          4  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_INVALID_SOCKET => failure, else a socket file descriptor.
 *
 *********************************************************************/
static jb_socket socks5_connect(const struct forward_spec *fwd,
                                const char *target_host,
                                int target_port,
                                struct client_state *csp)
{
#define SIZE_SOCKS5_REPLY_IPV4 10
#define SIZE_SOCKS5_REPLY_IPV6 22
#define SIZE_SOCKS5_REPLY_DOMAIN 300
#define SOCKS5_REPLY_DIFFERENCE (SIZE_SOCKS5_REPLY_IPV6 - SIZE_SOCKS5_REPLY_IPV4)
   int err = 0;
   char cbuf[300];
   char sbuf[SIZE_SOCKS5_REPLY_DOMAIN];
   size_t client_pos = 0;
   int server_size = 0;
   size_t hostlen = 0;
   jb_socket sfd;
   const char *errstr = NULL;

   assert(fwd->gateway_host);
   if ((fwd->gateway_host == NULL) || (*fwd->gateway_host == '\0'))
   {
      errstr = "NULL gateway host specified";
      err = 1;
   }

   if (fwd->gateway_port <= 0)
   {
      /*
       * XXX: currently this can't happen because in
       * case of invalid gateway ports we use the defaults.
       * Of course we really shouldn't do that.
       */
      errstr = "invalid gateway port specified";
      err = 1;
   }

   hostlen = strlen(target_host);
   if (hostlen > (size_t)255)
   {
      errstr = "target host name is longer than 255 characters";
      err = 1;
   }

   if ((fwd->type != SOCKS_5) && (fwd->type != SOCKS_5T))
   {
      /* Should never get here */
      log_error(LOG_LEVEL_FATAL,
         "SOCKS5 impossible internal error - bad SOCKS type");
      err = 1;
   }

   if (err)
   {
      errno = EINVAL;
      assert(errstr != NULL);
      log_error(LOG_LEVEL_CONNECT, "socks5_connect: %s", errstr);
      csp->error_message = strdup(errstr);
      return(JB_INVALID_SOCKET);
   }

#ifdef FUZZ
   sfd = 0;
   if (!err && read_socket(sfd, sbuf, 2) != 2)
#else
   /* pass the request to the socks server */
   sfd = connect_to(fwd->gateway_host, fwd->gateway_port, csp);

   if (sfd == JB_INVALID_SOCKET)
   {
      errstr = "socks5 server unreachable";
      log_error(LOG_LEVEL_CONNECT, "socks5_connect: %s", errstr);
      /* Free the generic error message provided by connect_to() */
      freez(csp->error_message);
      csp->error_message = strdup(errstr);
      return(JB_INVALID_SOCKET);
   }

   client_pos = 0;
   cbuf[client_pos++] = '\x05'; /* Version */

   if (fwd->auth_username && fwd->auth_password)
   {
      cbuf[client_pos++] = '\x02'; /* Two authentication methods supported */
      cbuf[client_pos++] = '\x02'; /* Username/password */
   }
   else
   {
      cbuf[client_pos++] = '\x01'; /* One authentication method supported */
   }
   cbuf[client_pos++] = '\x00'; /* The no authentication authentication method */

   if (write_socket(sfd, cbuf, client_pos))
   {
      errstr = "SOCKS5 negotiation write failed";
      csp->error_message = strdup(errstr);
      log_error(LOG_LEVEL_CONNECT, "%s", errstr);
      close_socket(sfd);
      return(JB_INVALID_SOCKET);
   }
   if (read_socks_reply(sfd, sbuf, sizeof(sbuf),
         csp->config->socket_timeout) != 2)
#endif
   {
      errstr = "SOCKS5 negotiation read failed";
      err = 1;
   }

   if (!err && (sbuf[0] != '\x05'))
   {
      errstr = "SOCKS5 negotiation protocol version error";
      err = 1;
   }

   if (!err && (sbuf[1] == '\xff'))
   {
      errstr = "SOCKS5 authentication required";
      err = 1;
   }

   if (!err && (sbuf[1] == '\x02'))
   {
      if (fwd->auth_username && fwd->auth_password)
      {
         /* check cbuf overflow */
         size_t auth_len = strlen(fwd->auth_username) + strlen(fwd->auth_password) + 3;
         if (auth_len > sizeof(cbuf))
         {
            errstr = "SOCKS5 username and/or password too long";
            err = 1;
         }
      }
      else
      {
         errstr = "SOCKS5 server requested authentication while "
            "no credentials are configured";
         err = 1;
      }

      if (!err)
      {
         client_pos = 0;
         cbuf[client_pos++] = '\x01'; /* Version */
         cbuf[client_pos++] = (char)strlen(fwd->auth_username);

         memcpy(cbuf + client_pos, fwd->auth_username, strlen(fwd->auth_username));
         client_pos += strlen(fwd->auth_username);
         cbuf[client_pos++] = (char)strlen(fwd->auth_password);
         memcpy(cbuf + client_pos, fwd->auth_password, strlen(fwd->auth_password));
         client_pos += strlen(fwd->auth_password);

         if (write_socket(sfd, cbuf, client_pos))
         {
            errstr = "SOCKS5 negotiation auth write failed";
            csp->error_message = strdup(errstr);
            log_error(LOG_LEVEL_CONNECT, "%s", errstr);
            close_socket(sfd);
            return(JB_INVALID_SOCKET);
         }

         if (read_socks_reply(sfd, sbuf, sizeof(sbuf),
               csp->config->socket_timeout) != 2)
         {
            errstr = "SOCKS5 negotiation auth read failed";
            err = 1;
         }
      }

      if (!err && (sbuf[1] != '\x00'))
      {
         errstr = "SOCKS5 authentication failed";
         err = 1;
      }
   }
   else if (!err && (sbuf[1] != '\x00'))
   {
      errstr = "SOCKS5 negotiation protocol error";
      err = 1;
   }

   if (err)
   {
      assert(errstr != NULL);
      log_error(LOG_LEVEL_CONNECT, "socks5_connect: %s", errstr);
      csp->error_message = strdup(errstr);
      close_socket(sfd);
      errno = EINVAL;
      return(JB_INVALID_SOCKET);
   }

   client_pos = 0;
   cbuf[client_pos++] = '\x05'; /* Version */
   cbuf[client_pos++] = '\x01'; /* TCP connect */
   cbuf[client_pos++] = '\x00'; /* Reserved, must be 0x00 */
   if (host_is_ip_address(target_host) && NULL == strstr(target_host, ":"))
   {
      cbuf[client_pos++] = '\x01'; /* Address is IPv4 address. */
      if (JB_ERR_OK != convert_ipv4_address_to_bytes(target_host, &cbuf[client_pos]))
      {
         errstr = "SOCKS5 error. Failed to convert target address to IP address";
         csp->error_message = strdup(errstr);
         log_error(LOG_LEVEL_CONNECT, "%s", errstr);
         close_socket(sfd);
         errno = EINVAL;
         return(JB_INVALID_SOCKET);
      }
      client_pos += 4;
   }
   else
   {
      /*
       * XXX: This branch is currently also used for IPv6 addresses
       */
      cbuf[client_pos++] = '\x03'; /* Address is domain name. */
      cbuf[client_pos++] = (char)(hostlen & 0xffu);
      assert(sizeof(cbuf) - client_pos > (size_t)255);
      /* Using strncpy because we really want the nul byte padding. */
      strncpy(cbuf + client_pos, target_host, sizeof(cbuf) - client_pos - 1);
      client_pos += (hostlen & 0xffu);
   }
   cbuf[client_pos++] = (char)((target_port >> 8) & 0xff);
   cbuf[client_pos++] = (char)((target_port     ) & 0xff);

#ifndef FUZZ
   if (write_socket(sfd, cbuf, client_pos))
   {
      errstr = "SOCKS5 negotiation write failed";
      csp->error_message = strdup(errstr);
      log_error(LOG_LEVEL_CONNECT, "%s", errstr);
      close_socket(sfd);
      errno = EINVAL;
      return(JB_INVALID_SOCKET);
   }

   /*
    * Optimistically send the HTTP request with the initial
    * SOCKS request if the user enabled the use of Tor extensions,
    * the CONNECT method isn't being used (in which case the client
    * doesn't send data until it gets our 200 response) and the
    * client request has actually been completely read already.
    */
   if ((fwd->type == SOCKS_5T) && (csp->http->ssl == 0)
      && (csp->flags & CSP_FLAG_CLIENT_REQUEST_COMPLETELY_READ))
   {
      char *client_headers = list_to_text(csp->headers);
      size_t header_length;

      if (client_headers == NULL)
      {
         log_error(LOG_LEVEL_FATAL, "Out of memory rebuilding client headers.");
      }
      list_remove_all(csp->headers);
      header_length= strlen(client_headers);

      log_error(LOG_LEVEL_CONNECT,
         "Optimistically sending %lu bytes of client headers intended for %s.",
         header_length, csp->http->hostport);

      if (write_socket(sfd, client_headers, header_length))
      {
         log_error(LOG_LEVEL_CONNECT,
            "optimistically writing header to: %s failed: %E", csp->http->hostport);
         freez(client_headers);
         return(JB_INVALID_SOCKET);
      }
      freez(client_headers);
      if (csp->expected_client_content_length != 0)
      {
         unsigned long long buffered_request_bytes =
            (unsigned long long)(csp->client_iob->eod - csp->client_iob->cur);
         log_error(LOG_LEVEL_CONNECT,
            "Optimistically sending %llu bytes of client body. Expected %llu.",
            csp->expected_client_content_length, buffered_request_bytes);
         assert(csp->expected_client_content_length == buffered_request_bytes);
         if (write_socket(sfd, csp->client_iob->cur, buffered_request_bytes))
         {
            log_error(LOG_LEVEL_CONNECT,
               "optimistically writing %llu bytes of client body to: %s failed: %E",
               buffered_request_bytes, csp->http->hostport);
            return(JB_INVALID_SOCKET);
         }
         clear_iob(csp->client_iob);
      }
   }
#endif

   server_size = read_socks_reply(sfd, sbuf, SIZE_SOCKS5_REPLY_IPV4,
      csp->config->socket_timeout);
   if (server_size != SIZE_SOCKS5_REPLY_IPV4)
   {
      errstr = "SOCKS5 negotiation read failed";
   }
   else
   {
      if (sbuf[0] != '\x05')
      {
         errstr = "SOCKS5 negotiation protocol version error";
      }
      else if (sbuf[2] != '\x00')
      {
         errstr = "SOCKS5 negotiation protocol error";
      }
      else if (sbuf[1] != SOCKS5_REQUEST_GRANTED)
      {
         errstr = translate_socks5_error(sbuf[1]);
      }
      else
      {
         if (sbuf[3] == '\x04')
         {
            /*
             * The address field contains an IPv6 address
             * which means we didn't get the whole reply
             * yet. Read and discard the rest of it to make
             * sure it isn't treated as HTTP data later on.
             */
            server_size = read_socks_reply(sfd, sbuf, SOCKS5_REPLY_DIFFERENCE,
               csp->config->socket_timeout);
            if (server_size != SOCKS5_REPLY_DIFFERENCE)
            {
               errstr = "SOCKS5 negotiation read failed (IPv6 address)";
            }
         }
         else if (sbuf[3] == '\x03')
         {
            /*
             * The address field contains a domain name
             * which means we didn't get the whole reply
             * yet. Read and discard the rest of it to make
             * sure it isn't treated as HTTP data later on.
             */
            unsigned domain_length = (unsigned)sbuf[4];
            int bytes_left_to_read = 5 + (int)domain_length + 2 - SIZE_SOCKS5_REPLY_IPV4;
            if (bytes_left_to_read <= 0 || sizeof(sbuf) < bytes_left_to_read)
            {
               errstr = "SOCKS5 negotiation read failed (Invalid domain length)";
            }
            else
            {
               server_size = read_socks_reply(sfd, sbuf, bytes_left_to_read,
                  csp->config->socket_timeout);
               if (server_size != bytes_left_to_read)
               {
                  errstr = "SOCKS5 negotiation read failed (Domain name)";
               }
            }
         }
         else if (sbuf[3] != '\x01')
         {
            errstr = "SOCKS5 reply contains unsupported address type";
         }
         if (errstr == NULL)
         {
            return(sfd);
         }
      }
   }

   assert(errstr != NULL);
   csp->error_message = strdup(errstr);
   log_error(LOG_LEVEL_CONNECT, "socks5_connect: %s", errstr);
   close_socket(sfd);
   errno = EINVAL;

   return(JB_INVALID_SOCKET);

}

/*
  Local Variables:
  tab-width: 3
  end:
*/
