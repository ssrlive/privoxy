/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/jbsockets.c,v $
 *
 * Purpose     :  Contains wrappers for system-specific sockets code,
 *                so that the rest of Privoxy can be more
 *                OS-independent.  Contains #ifdefs to make this work
 *                on many platforms.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#ifdef _WIN32

#ifndef STRICT
#define STRICT
#endif
#include <winsock2.h>
#include <windows.h>
#include <sys/timeb.h>
#include <io.h>

#else

#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/socket.h>

#ifndef __BEOS__
#include <netinet/tcp.h>
#include <arpa/inet.h>
#else
#include <socket.h>
#endif

#endif

#ifdef HAVE_POLL
#ifdef __GLIBC__
#include <sys/poll.h>
#else
#include <poll.h>
#endif /* def __GLIBC__ */
#endif /* HAVE_POLL */

#include "project.h"

/* For mutex semaphores only */
#include "jcc.h"

#include "jbsockets.h"
#include "filters.h"
#include "errlog.h"
#include "miscutil.h"

/* Mac OSX doesn't define AI_NUMERICSESRV */
#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV 0
#endif

/*
 * Maximum number of gethostbyname(_r) retries in case of
 * soft errors (TRY_AGAIN).
 * XXX: Does it make sense to make this a config option?
 */
#define MAX_DNS_RETRIES 10

#ifdef HAVE_RFC2553
static jb_socket rfc2553_connect_to(const char *host, int portnum, struct client_state *csp);
#else
static jb_socket no_rfc2553_connect_to(const char *host, int portnum, struct client_state *csp);
#endif

/*********************************************************************
 *
 * Function    :  set_no_delay_flag
 *
 * Description :  Disables the Nagle algorithm (TCP send coalescence)
 *                for the given socket.
 *
 * Parameters  :
 *          1  :  fd = The file descriptor to operate on
 *
 * Returns     :  void
 *
 *********************************************************************/
static void set_no_delay_flag(int fd)
{
#ifdef TCP_NODELAY
   int mi = 1;

   if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &mi, sizeof(int)))
   {
      log_error(LOG_LEVEL_ERROR,
         "Failed to disable TCP coalescence for socket %d", fd);
   }
#else
#warning set_no_delay_flag() is a nop due to lack of TCP_NODELAY
#endif /* def TCP_NODELAY */
}

/*********************************************************************
 *
 * Function    :  connect_to
 *
 * Description :  Open a socket and connect to it.  Will check
 *                that this is allowed according to ACL.
 *
 * Parameters  :
 *          1  :  host = hostname to connect to
 *          2  :  portnum = port to connect to (XXX: should be unsigned)
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  JB_INVALID_SOCKET => failure, else it is the socket
 *                file descriptor.
 *
 *********************************************************************/
jb_socket connect_to(const char *host, int portnum, struct client_state *csp)
{
   jb_socket fd;
   int forwarded_connect_retries = 0;

   do
   {
      /*
       * XXX: The whole errno overloading is ridiculous and should
       *      be replaced with something sane and thread safe
       */
      /* errno = 0;*/
#ifdef HAVE_RFC2553
      fd = rfc2553_connect_to(host, portnum, csp);
#else
      fd = no_rfc2553_connect_to(host, portnum, csp);
#endif
      if ((fd != JB_INVALID_SOCKET) || (errno == EINVAL)
         || (csp->fwd == NULL)
         || ((csp->fwd->forward_host == NULL) && (csp->fwd->type == SOCKS_NONE)))
      {
         break;
      }
      forwarded_connect_retries++;
      if (csp->config->forwarded_connect_retries != 0)
      {
         log_error(LOG_LEVEL_ERROR,
            "Attempt %d of %d to connect to %s failed. Trying again.",
            forwarded_connect_retries, csp->config->forwarded_connect_retries + 1, host);
      }

   } while (forwarded_connect_retries < csp->config->forwarded_connect_retries);

   return fd;
}

#ifdef HAVE_RFC2553
/* Getaddrinfo implementation */
static jb_socket rfc2553_connect_to(const char *host, int portnum, struct client_state *csp)
{
   struct addrinfo hints, *result, *rp;
   char service[6];
   int retval;
   jb_socket fd = JB_INVALID_SOCKET;
#ifdef HAVE_POLL
   struct pollfd poll_fd[1];
#else
   fd_set wfds;
   struct timeval timeout;
#endif
#if !defined(_WIN32) && !defined(__BEOS__)
   int   flags;
#endif
   int connect_failed;
   /*
    * XXX: Initializing it here is only necessary
    *      because not all situations are properly
    *      covered yet.
    */
   int socket_error = 0;

#ifdef FEATURE_ACL
   struct access_control_addr dst[1];
#endif /* def FEATURE_ACL */

   /* Don't leak memory when retrying. */
   freez(csp->error_message);
   freez(csp->http->host_ip_addr_str);

   retval = snprintf(service, sizeof(service), "%d", portnum);
   if ((-1 == retval) || (sizeof(service) <= retval))
   {
      log_error(LOG_LEVEL_ERROR,
         "Port number (%d) ASCII decimal representation doesn't fit into 6 bytes",
         portnum);
      csp->error_message = strdup("Invalid port number");
      csp->http->host_ip_addr_str = strdup("unknown");
      return(JB_INVALID_SOCKET);
   }

   memset((char *)&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_NUMERICSERV; /* avoid service look-up */
#ifdef AI_ADDRCONFIG
   hints.ai_flags |= AI_ADDRCONFIG;
#endif
   if ((retval = getaddrinfo(host, service, &hints, &result)))
   {
      log_error(LOG_LEVEL_INFO,
         "Can not resolve %s: %s", host, gai_strerror(retval));
      csp->error_message = strdup(gai_strerror(retval));
      csp->http->host_ip_addr_str = strdup("unknown");
      /* XXX: Should find a better way to propagate this error. */
      errno = EINVAL;
      return(JB_INVALID_SOCKET);
   }

   csp->http->host_ip_addr_str = malloc_or_die(NI_MAXHOST);

   for (rp = result; rp != NULL; rp = rp->ai_next)
   {

#ifdef FEATURE_ACL
      memcpy(&dst->addr, rp->ai_addr, rp->ai_addrlen);

      if (block_acl(dst, csp))
      {
         socket_error = errno = EPERM;
         continue;
      }
#endif /* def FEATURE_ACL */

      retval = getnameinfo(rp->ai_addr, rp->ai_addrlen,
         csp->http->host_ip_addr_str, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if (retval)
      {
         log_error(LOG_LEVEL_ERROR,
            "Failed to get the host name from the socket structure: %s",
            gai_strerror(retval));
         continue;
      }

      fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
#ifdef _WIN32
      if (fd == JB_INVALID_SOCKET)
#else
      if (fd < 0)
#endif
      {
         continue;
      }

#ifndef HAVE_POLL
#ifndef _WIN32
      if (fd >= FD_SETSIZE)
      {
         log_error(LOG_LEVEL_ERROR,
            "Server socket number too high to use select(): %d >= %d",
            fd, FD_SETSIZE);
         close_socket(fd);
         freeaddrinfo(result);
         return JB_INVALID_SOCKET;
      }
#endif
#endif

#ifdef FEATURE_EXTERNAL_FILTERS
      mark_socket_for_close_on_execute(fd);
#endif

      set_no_delay_flag(fd);

#if !defined(_WIN32) && !defined(__BEOS__)
      if ((flags = fcntl(fd, F_GETFL, 0)) != -1)
      {
         flags |= O_NDELAY;
         fcntl(fd, F_SETFL, flags);
      }
#endif /* !defined(_WIN32) && !defined(__BEOS__) */

      connect_failed = 0;
      while (connect(fd, rp->ai_addr, rp->ai_addrlen) == JB_INVALID_SOCKET)
      {
#ifdef _WIN32
         if (errno == WSAEINPROGRESS)
#else /* ifndef _WIN32 */
         if (errno == EINPROGRESS)
#endif /* ndef _WIN32 */
         {
            break;
         }

         if (errno != EINTR)
         {
            socket_error = errno;
            close_socket(fd);
            connect_failed = 1;
            break;
         }
      }
      if (connect_failed)
      {
         continue;
      }

#if !defined(_WIN32) && !defined(__BEOS__)
      if (flags != -1)
      {
         flags &= ~O_NDELAY;
         fcntl(fd, F_SETFL, flags);
      }
#endif /* !defined(_WIN32) && !defined(__BEOS__) */

#ifdef HAVE_POLL
      poll_fd[0].fd = fd;
      poll_fd[0].events = POLLOUT;

      retval = poll(poll_fd, 1, 30000);
      if (retval == 0)
      {
         if (rp->ai_next != NULL)
         {
            /* Log this now as we'll try another address next */
            log_error(LOG_LEVEL_CONNECT,
               "Could not connect to [%s]:%s: Operation timed out.",
               csp->http->host_ip_addr_str, service);
         }
         else
         {
            /*
             * This is the last address, don't log this now
             * as it would result in a duplicated log message.
             */
            socket_error = ETIMEDOUT;
         }
      }
      else if (retval > 0)
#else
      /* wait for connection to complete */
      FD_ZERO(&wfds);
      FD_SET(fd, &wfds);

      memset(&timeout, 0, sizeof(timeout));
      timeout.tv_sec  = 30;

      /* MS Windows uses int, not SOCKET, for the 1st arg of select(). Weird! */
      if ((select((int)fd + 1, NULL, &wfds, NULL, &timeout) > 0)
         && FD_ISSET(fd, &wfds))
#endif
      {
         socklen_t optlen = sizeof(socket_error);
         if (!getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &optlen))
         {
            if (!socket_error)
            {
               /* Connection established, no need to try other addresses. */
               break;
            }
            if (rp->ai_next != NULL)
            {
               /*
                * There's another address we can try, so log that this
                * one didn't work out. If the last one fails, too,
                * it will get logged outside the loop body so we don't
                * have to mention it here.
                */
               log_error(LOG_LEVEL_CONNECT, "Could not connect to [%s]:%s: %s.",
                  csp->http->host_ip_addr_str, service, strerror(socket_error));
            }
         }
         else
         {
            socket_error = errno;
            log_error(LOG_LEVEL_ERROR, "Could not get the state of "
               "the connection to [%s]:%s: %s; dropping connection.",
               csp->http->host_ip_addr_str, service, strerror(errno));
         }
      }

      /* Connection failed, try next address */
      close_socket(fd);
   }

   freeaddrinfo(result);
   if (!rp)
   {
      log_error(LOG_LEVEL_CONNECT, "Could not connect to [%s]:%s: %s.",
         host, service, strerror(socket_error));
      csp->error_message = strdup(strerror(socket_error));
      return(JB_INVALID_SOCKET);
   }
   log_error(LOG_LEVEL_CONNECT, "Connected to %s[%s]:%s.",
      host, csp->http->host_ip_addr_str, service);

   return(fd);

}

#else /* ndef HAVE_RFC2553 */
/* Pre-getaddrinfo implementation */

static jb_socket no_rfc2553_connect_to(const char *host, int portnum, struct client_state *csp)
{
   struct sockaddr_in inaddr;
   jb_socket fd;
   unsigned int addr;
#ifdef HAVE_POLL
   struct pollfd poll_fd[1];
#else
   fd_set wfds;
   struct timeval tv[1];
#endif
#if !defined(_WIN32) && !defined(__BEOS__)
   int   flags;
#endif

#ifdef FEATURE_ACL
   struct access_control_addr dst[1];
#endif /* def FEATURE_ACL */

   /* Don't leak memory when retrying. */
   freez(csp->http->host_ip_addr_str);

   memset((char *)&inaddr, 0, sizeof inaddr);

   if ((addr = resolve_hostname_to_ip(host)) == INADDR_NONE)
   {
      csp->http->host_ip_addr_str = strdup("unknown");
      return(JB_INVALID_SOCKET);
   }

#ifdef FEATURE_ACL
   dst->addr = ntohl(addr);
   dst->port = portnum;

   if (block_acl(dst, csp))
   {
      errno = EPERM;
      return(JB_INVALID_SOCKET);
   }
#endif /* def FEATURE_ACL */

   inaddr.sin_addr.s_addr = addr;
   inaddr.sin_family      = AF_INET;
   csp->http->host_ip_addr_str = strdup(inet_ntoa(inaddr.sin_addr));

#ifndef _WIN32
   if (sizeof(inaddr.sin_port) == sizeof(short))
#endif /* ndef _WIN32 */
   {
      inaddr.sin_port = htons((unsigned short) portnum);
   }
#ifndef _WIN32
   else
   {
      inaddr.sin_port = htonl((unsigned long)portnum);
   }
#endif /* ndef _WIN32 */

   fd = socket(inaddr.sin_family, SOCK_STREAM, 0);
#ifdef _WIN32
   if (fd == JB_INVALID_SOCKET)
#else
   if (fd < 0)
#endif
   {
      return(JB_INVALID_SOCKET);
   }

#ifndef HAVE_POLL
#ifndef _WIN32
   if (fd >= FD_SETSIZE)
   {
      log_error(LOG_LEVEL_ERROR,
         "Server socket number too high to use select(): %d >= %d",
         fd, FD_SETSIZE);
      close_socket(fd);
      return JB_INVALID_SOCKET;
   }
#endif
#endif

   set_no_delay_flag(fd);

#if !defined(_WIN32) && !defined(__BEOS__)
   if ((flags = fcntl(fd, F_GETFL, 0)) != -1)
   {
      flags |= O_NDELAY;
      fcntl(fd, F_SETFL, flags);
#ifdef FEATURE_EXTERNAL_FILTERS
      mark_socket_for_close_on_execute(fd);
#endif
   }
#endif /* !defined(_WIN32) && !defined(__BEOS__) */

   while (connect(fd, (struct sockaddr *) & inaddr, sizeof inaddr) == JB_INVALID_SOCKET)
   {
#ifdef _WIN32
      if (errno == WSAEINPROGRESS)
#else /* ifndef _WIN32 */
      if (errno == EINPROGRESS)
#endif /* ndef _WIN32 */
      {
         break;
      }

      if (errno != EINTR)
      {
         close_socket(fd);
         return(JB_INVALID_SOCKET);
      }
   }

#if !defined(_WIN32) && !defined(__BEOS__)
   if (flags != -1)
   {
      flags &= ~O_NDELAY;
      fcntl(fd, F_SETFL, flags);
   }
#endif /* !defined(_WIN32) && !defined(__BEOS__) */

#ifdef HAVE_POLL
   poll_fd[0].fd = fd;
   poll_fd[0].events = POLLOUT;

   if (poll(poll_fd, 1, 30000) <= 0)
#else
   /* wait for connection to complete */
   FD_ZERO(&wfds);
   FD_SET(fd, &wfds);

   tv->tv_sec  = 30;
   tv->tv_usec = 0;

   /* MS Windows uses int, not SOCKET, for the 1st arg of select(). Weird! */
   if (select((int)fd + 1, NULL, &wfds, NULL, tv) <= 0)
#endif
   {
      close_socket(fd);
      return(JB_INVALID_SOCKET);
   }
   return(fd);

}
#endif /* ndef HAVE_RFC2553 */


/*********************************************************************
 *
 * Function    :  write_socket
 *
 * Description :  Write the contents of buf (for n bytes) to socket fd.
 *
 * Parameters  :
 *          1  :  fd = file descriptor (aka. handle) of socket to write to.
 *          2  :  buf = pointer to data to be written.
 *          3  :  len = length of data to be written to the socket "fd".
 *
 * Returns     :  0 on success (entire buffer sent).
 *                nonzero on error.
 *
 *********************************************************************/
int write_socket(jb_socket fd, const char *buf, size_t len)
{
   if (len == 0)
   {
      return 0;
   }

#ifdef FUZZ
   if (!daemon_mode && fd <= 3)
   {
      log_error(LOG_LEVEL_WRITING, "Pretending to write to socket %d: %N", fd, len, buf);
      return 0;
   }
#endif

   log_error(LOG_LEVEL_WRITING, "to socket %d: %N", fd, len, buf);

#if defined(_WIN32)
   return (send(fd, buf, (int)len, 0) != (int)len);
#elif defined(__BEOS__)
   return (send(fd, buf, len, 0) != len);
#else
   return (write(fd, buf, len) != len);
#endif

}


/*********************************************************************
 *
 * Function    :  write_socket_delayed
 *
 * Description :  Write the contents of buf (for n bytes) to
 *                socket fd, optionally delaying the operation.
 *
 * Parameters  :
 *          1  :  fd = File descriptor (aka. handle) of socket to write to.
 *          2  :  buf = Pointer to data to be written.
 *          3  :  len = Length of data to be written to the socket "fd".
 *          4  :  delay = Delay in milliseconds.
 *
 * Returns     :  0 on success (entire buffer sent).
 *                nonzero on error.
 *
 *********************************************************************/
int write_socket_delayed(jb_socket fd, const char *buf, size_t len, unsigned int delay)
{
   size_t i = 0;

   if (delay == 0)
   {
      return write_socket(fd, buf, len);
   }

   while (i < len)
   {
      size_t write_length;
      enum {MAX_WRITE_LENGTH = 10};

      if ((i + MAX_WRITE_LENGTH) > len)
      {
         write_length = len - i;
      }
      else
      {
         write_length = MAX_WRITE_LENGTH;
      }

      privoxy_millisleep(delay);

      if (write_socket(fd, buf + i, write_length) != 0)
      {
         return 1;
      }
      i += write_length;
   }

   return 0;

}


/*********************************************************************
 *
 * Function    :  read_socket
 *
 * Description :  Read from a TCP/IP socket in a platform independent way.
 *
 * Parameters  :
 *          1  :  fd = file descriptor of the socket to read
 *          2  :  buf = pointer to buffer where data will be written
 *                Must be >= len bytes long.
 *          3  :  len = maximum number of bytes to read
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
int read_socket(jb_socket fd, char *buf, int len)
{
   int ret;

   if (len <= 0)
   {
      return(0);
   }

#if defined(_WIN32)
   ret = recv(fd, buf, len, 0);
#elif defined(__BEOS__)
   ret = recv(fd, buf, (size_t)len, 0);
#else
   ret = (int)read(fd, buf, (size_t)len);
#endif

   if (ret > 0)
   {
      log_error(LOG_LEVEL_RECEIVED, "from socket %d: %N", fd, ret, buf);
   }

   return ret;
}


/*********************************************************************
 *
 * Function    :  data_is_available
 *
 * Description :  Waits for data to arrive on a socket.
 *
 * Parameters  :
 *          1  :  fd = file descriptor of the socket to read
 *          2  :  seconds_to_wait = number of seconds after which we give up.
 *
 * Returns     :  TRUE if data arrived in time,
 *                FALSE otherwise.
 *
 *********************************************************************/
int data_is_available(jb_socket fd, int seconds_to_wait)
{
   int n;
   char buf[10];
#ifdef HAVE_POLL
   struct pollfd poll_fd[1];

   poll_fd[0].fd = fd;
   poll_fd[0].events = POLLIN;

   n = poll(poll_fd, 1, seconds_to_wait * 1000);
#else
   fd_set rfds;
   struct timeval timeout;

   memset(&timeout, 0, sizeof(timeout));
   timeout.tv_sec = seconds_to_wait;

   FD_ZERO(&rfds);
   FD_SET(fd, &rfds);

   n = select(fd+1, &rfds, NULL, NULL, &timeout);
#endif

   /*
    * XXX: Do we care about the different error conditions?
    */
   return ((n == 1) && (1 == recv(fd, buf, 1, MSG_PEEK)));
}


/*********************************************************************
 *
 * Function    :  close_socket
 *
 * Description :  Closes a TCP/IP socket
 *
 * Parameters  :
 *          1  :  fd = file descriptor of socket to be closed
 *
 * Returns     :  void
 *
 *********************************************************************/
void close_socket(jb_socket fd)
{
#if defined(_WIN32) || defined(__BEOS__)
   closesocket(fd);
#else
   close(fd);
#endif
}


/*********************************************************************
 *
 * Function    :  drain_and_close_socket
 *
 * Description :  Closes a TCP/IP socket after draining unread data
 *
 * Parameters  :
 *          1  :  fd = file descriptor of the socket to be closed
 *
 * Returns     :  void
 *
 *********************************************************************/
void drain_and_close_socket(jb_socket fd)
{
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   if (socket_is_still_alive(fd))
#endif
   {
      int bytes_drained_total = 0;
      int bytes_drained;

#ifdef HAVE_SHUTDOWN
/* Apparently Windows has shutdown() but not SHUT_WR. */
#ifndef SHUT_WR
#define SHUT_WR 1
#endif
      if (0 != shutdown(fd, SHUT_WR))
      {
         log_error(LOG_LEVEL_CONNECT, "Failed to shutdown socket %d: %E", fd);
      }
#endif
#define ARBITRARY_DRAIN_LIMIT 10000
      do
      {
         char drainage[500];

         if (!data_is_available(fd, 0))
         {
            /*
             * If there is no data available right now, don't try
             * to drain the socket as read_socket() could block.
             */
            break;
         }

         bytes_drained = read_socket(fd, drainage, sizeof(drainage));
         if (bytes_drained < 0)
         {
            log_error(LOG_LEVEL_CONNECT, "Failed to drain socket %d: %E", fd);
         }
         else if (bytes_drained > 0)
         {
            bytes_drained_total += bytes_drained;
            if (bytes_drained_total > ARBITRARY_DRAIN_LIMIT)
            {
               log_error(LOG_LEVEL_CONNECT, "Giving up draining socket %d.", fd);
               break;
            }
         }
      } while (bytes_drained > 0);
      if (bytes_drained_total != 0)
      {
         log_error(LOG_LEVEL_CONNECT,
            "Drained %d bytes before closing socket %d.", bytes_drained_total, fd);
      }
   }

   close_socket(fd);

}


/*********************************************************************
 *
 * Function    :  bind_port
 *
 * Description :  Call socket, set socket options, and listen.
 *                Called by listen_loop to "boot up" our proxy address.
 *
 * Parameters  :
 *          1  :  hostnam = TCP/IP address to bind/listen to
 *          2  :  portnum = port to listen on
 *          3  :  backlog = Listen backlog
 *          4  :  pfd = pointer used to return file descriptor.
 *
 * Returns     :  if success, returns 0 and sets *pfd.
 *                if failure, returns -3 if address is in use,
 *                                    -2 if address unresolvable,
 *                                    -1 otherwise
 *********************************************************************/
int bind_port(const char *hostnam, int portnum, int backlog, jb_socket *pfd)
{
#ifdef HAVE_RFC2553
   struct addrinfo hints;
   struct addrinfo *result, *rp;
   /*
    * XXX: portnum should be a string to allow symbolic service
    * names in the configuration file and to avoid the following
    * int2string.
    */
   char servnam[6];
   int retval;
#else
   struct sockaddr_in inaddr;
#endif /* def HAVE_RFC2553 */
   jb_socket fd = JB_INVALID_SOCKET;
#ifndef _WIN32
   int one = 1;
#endif /* ndef _WIN32 */

   *pfd = JB_INVALID_SOCKET;

#ifdef HAVE_RFC2553
   retval = snprintf(servnam, sizeof(servnam), "%d", portnum);
   if ((-1 == retval) || (sizeof(servnam) <= retval))
   {
      log_error(LOG_LEVEL_ERROR,
         "Port number (%d) ASCII decimal representation doesn't fit into 6 bytes.",
         portnum);
      return -1;
   }

   memset(&hints, 0, sizeof(struct addrinfo));
   if (hostnam == NULL)
   {
      /*
       * XXX: This is a hack. The right thing to do
       * would be to bind to both AF_INET and AF_INET6.
       * This will also fail if there is no AF_INET
       * version available.
       */
      hints.ai_family = AF_INET;
   }
   else
   {
      hints.ai_family = AF_UNSPEC;
   }
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;
   hints.ai_protocol = 0; /* Really any stream protocol or TCP only */
   hints.ai_canonname = NULL;
   hints.ai_addr = NULL;
   hints.ai_next = NULL;

   if ((retval = getaddrinfo(hostnam, servnam, &hints, &result)))
   {
      log_error(LOG_LEVEL_ERROR,
         "Can not resolve %s: %s", hostnam, gai_strerror(retval));
      return -2;
   }
#else
   memset((char *)&inaddr, '\0', sizeof inaddr);

   inaddr.sin_family      = AF_INET;
   inaddr.sin_addr.s_addr = resolve_hostname_to_ip(hostnam);

   if (inaddr.sin_addr.s_addr == INADDR_NONE)
   {
      return(-2);
   }

#ifndef _WIN32
   if (sizeof(inaddr.sin_port) == sizeof(short))
#endif /* ndef _WIN32 */
   {
      inaddr.sin_port = htons((unsigned short) portnum);
   }
#ifndef _WIN32
   else
   {
      inaddr.sin_port = htonl((unsigned long) portnum);
   }
#endif /* ndef _WIN32 */
#endif /* def HAVE_RFC2553 */

#ifdef HAVE_RFC2553
   for (rp = result; rp != NULL; rp = rp->ai_next)
   {
      fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
#else
   fd = socket(AF_INET, SOCK_STREAM, 0);
#endif /* def HAVE_RFC2553 */

#ifdef _WIN32
   if (fd == JB_INVALID_SOCKET)
#else
   if (fd < 0)
#endif
   {
#ifdef HAVE_RFC2553
      continue;
#else
      return(-1);
#endif
   }

#ifdef FEATURE_EXTERNAL_FILTERS
   mark_socket_for_close_on_execute(fd);
#endif

#ifndef _WIN32
   /*
    * This is not needed for Win32 - in fact, it stops
    * duplicate instances of Privoxy from being caught.
    *
    * On UNIX, we assume the user is sensible enough not
    * to start Privoxy multiple times on the same IP.
    * Without this, stopping and restarting Privoxy
    * from a script fails.
    * Note: SO_REUSEADDR is meant to only take over
    * sockets which are *not* in listen state in Linux,
    * e.g. sockets in TIME_WAIT. YMMV.
    */
   setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
#endif /* ndef _WIN32 */

#ifdef IP_FREEBIND
   setsockopt(fd, IPPROTO_IP, IP_FREEBIND, (char *)&one, sizeof(one));
#endif

#ifdef HAVE_RFC2553
   if (bind(fd, rp->ai_addr, rp->ai_addrlen) < 0)
#else
   if (bind(fd, (struct sockaddr *)&inaddr, sizeof(inaddr)) < 0)
#endif
   {
#ifdef _WIN32
      errno = WSAGetLastError();
      if (errno == WSAEADDRINUSE)
#else
      if (errno == EADDRINUSE)
#endif
      {
#ifdef HAVE_RFC2553
         freeaddrinfo(result);
#endif
         close_socket(fd);
         return(-3);
      }
      else
      {
         close_socket(fd);
#ifndef HAVE_RFC2553
         return(-1);
      }
   }
#else
      }
   }
   else
   {
      /* bind() succeeded, escape from for-loop */
      /*
       * XXX: Support multiple listening sockets (e.g. localhost
       * resolves to AF_INET and AF_INET6, but only the first address
       * is used
       */
      break;
   }
   }

   freeaddrinfo(result);
   if (rp == NULL)
   {
      /* All bind()s failed */
      return(-1);
   }
#endif /* ndef HAVE_RFC2553 */

   while (listen(fd, backlog) == -1)
   {
      if (errno != EINTR)
      {
         close_socket(fd);
         return(-1);
      }
   }

   *pfd = fd;
   return 0;

}


/*********************************************************************
 *
 * Function    :  get_host_information
 *
 * Description :  Determines the IP address the client used to
 *                reach us and the hostname associated with it.
 *
 *                XXX: Most of the code has been copy and pasted
 *                from accept_connection() and not all of the
 *                ifdefs paths have been tested afterwards.
 *
 * Parameters  :
 *          1  :  afd = File descriptor returned from accept().
 *          2  :  ip_address = Pointer to return the pointer to
 *                             the ip address string.
 *          3  :  port =       Pointer to return the pointer to
 *                             the TCP port string.
 *          4  :  hostname =   Pointer to return the pointer to
 *                             the hostname or NULL if the caller
 *                             isn't interested in it.
 *
 * Returns     :  void.
 *
 *********************************************************************/
void get_host_information(jb_socket afd, char **ip_address, char **port,
                          char **hostname)
{
#ifdef HAVE_RFC2553
   struct sockaddr_storage server;
   int retval;
#else
   struct sockaddr_in server;
   struct hostent *host = NULL;
#endif /* HAVE_RFC2553 */
#if defined(_WIN32)
   /* according to accept_connection() this fixes a warning. */
   int s_length, s_length_provided;
#else
   socklen_t s_length, s_length_provided;
#endif
#ifndef HAVE_RFC2553
#if defined(HAVE_GETHOSTBYADDR_R_8_ARGS) ||  defined(HAVE_GETHOSTBYADDR_R_7_ARGS) || defined(HAVE_GETHOSTBYADDR_R_5_ARGS)
   struct hostent result;
#if defined(HAVE_GETHOSTBYADDR_R_5_ARGS)
   struct hostent_data hdata;
#else
   char hbuf[HOSTENT_BUFFER_SIZE];
   int thd_err;
#endif /* def HAVE_GETHOSTBYADDR_R_5_ARGS */
#endif /* def HAVE_GETHOSTBYADDR_R_(8|7|5)_ARGS */
#endif /* ifndef HAVE_RFC2553 */
   s_length = s_length_provided = sizeof(server);

   if (NULL != hostname)
   {
      *hostname = NULL;
   }
   *ip_address = NULL;
   *port = NULL;

   if (!getsockname(afd, (struct sockaddr *) &server, &s_length))
   {
      if (s_length > s_length_provided)
      {
         log_error(LOG_LEVEL_ERROR, "getsockname() truncated server address");
         return;
      }
/*
 * XXX: Workaround for missing header on Windows when
 *      configured with --disable-ipv6-support.
 *      The proper fix is to not use NI_MAXSERV in
 *      that case. It works by accident on other platforms
 *      as <netdb.h> is included unconditionally there.
 */
#ifndef NI_MAXSERV
#define NI_MAXSERV 32
#endif
      *port = malloc_or_die(NI_MAXSERV);

#ifdef HAVE_RFC2553
      *ip_address = malloc_or_die(NI_MAXHOST);
      retval = getnameinfo((struct sockaddr *) &server, s_length,
         *ip_address, NI_MAXHOST, *port, NI_MAXSERV,
         NI_NUMERICHOST|NI_NUMERICSERV);
      if (retval)
      {
         log_error(LOG_LEVEL_ERROR,
            "Unable to print my own IP address: %s", gai_strerror(retval));
         freez(*ip_address);
         freez(*port);
         return;
      }
#else
      *ip_address = strdup(inet_ntoa(server.sin_addr));
      snprintf(*port, NI_MAXSERV, "%hu", ntohs(server.sin_port));
#endif /* HAVE_RFC2553 */
      if (NULL == hostname)
      {
         /*
          * We're done here, the caller isn't
          * interested in knowing the hostname.
          */
         return;
      }

#ifdef HAVE_RFC2553
      *hostname = malloc_or_die(NI_MAXHOST);
      retval = getnameinfo((struct sockaddr *) &server, s_length,
         *hostname, NI_MAXHOST, NULL, 0, NI_NAMEREQD);
      if (retval)
      {
         log_error(LOG_LEVEL_ERROR,
            "Unable to resolve my own IP address: %s", gai_strerror(retval));
         freez(*hostname);
      }
#else
#if defined(HAVE_GETHOSTBYADDR_R_8_ARGS)
      gethostbyaddr_r((const char *)&server.sin_addr,
                      sizeof(server.sin_addr), AF_INET,
                      &result, hbuf, HOSTENT_BUFFER_SIZE,
                      &host, &thd_err);
#elif defined(HAVE_GETHOSTBYADDR_R_7_ARGS)
      host = gethostbyaddr_r((const char *)&server.sin_addr,
                      sizeof(server.sin_addr), AF_INET,
                      &result, hbuf, HOSTENT_BUFFER_SIZE, &thd_err);
#elif defined(HAVE_GETHOSTBYADDR_R_5_ARGS)
      if (0 == gethostbyaddr_r((const char *)&server.sin_addr,
                               sizeof(server.sin_addr), AF_INET,
                               &result, &hdata))
      {
         host = &result;
      }
      else
      {
         host = NULL;
      }
#elif defined(MUTEX_LOCKS_AVAILABLE)
      privoxy_mutex_lock(&resolver_mutex);
      host = gethostbyaddr((const char *)&server.sin_addr,
                           sizeof(server.sin_addr), AF_INET);
      privoxy_mutex_unlock(&resolver_mutex);
#else
      host = gethostbyaddr((const char *)&server.sin_addr,
                           sizeof(server.sin_addr), AF_INET);
#endif
      if (host == NULL)
      {
         log_error(LOG_LEVEL_ERROR, "Unable to get my own hostname: %E\n");
      }
      else
      {
         *hostname = strdup(host->h_name);
      }
#endif /* else def HAVE_RFC2553 */
   }

   return;
}


/*********************************************************************
 *
 * Function    :  accept_connection
 *
 * Description :  Accepts a connection on one of possibly multiple
 *                sockets. The socket(s) to check must have been
 *                created using bind_port().
 *
 * Parameters  :
 *          1  :  csp = Client state, cfd, ip_addr_str, and
 *                      ip_addr_long will be set by this routine.
 *          2  :  fds = File descriptors returned from bind_port
 *
 * Returns     :  when a connection is accepted, it returns 1 (TRUE).
 *                On an error it returns 0 (FALSE).
 *
 *********************************************************************/
int accept_connection(struct client_state * csp, jb_socket fds[])
{
#ifdef HAVE_RFC2553
   /* XXX: client is stored directly into csp->tcp_addr */
#define client (csp->tcp_addr)
#else
   struct sockaddr_in client;
#endif
   jb_socket afd;
#if defined(_WIN32)
   /* Weirdness - fix a warning. */
   int c_length;
#else
   socklen_t c_length;
#endif
   int retval;
   int i;
   int max_selected_socket;
#ifdef HAVE_POLL
   struct pollfd poll_fds[MAX_LISTENING_SOCKETS];
   nfds_t polled_sockets;
#else
   fd_set selected_fds;
#endif
   jb_socket fd;
   const char *host_addr;
   size_t listen_addr_size;

   c_length = sizeof(client);

#ifdef HAVE_POLL
   memset(poll_fds, 0, sizeof(poll_fds));
   polled_sockets = 0;
#else
   /*
    * Wait for a connection on any socket.
    * Return immediately if no socket is listening.
    * XXX: Why not treat this as fatal error?
    */
   FD_ZERO(&selected_fds);
#endif
   max_selected_socket = 0;
   for (i = 0; i < MAX_LISTENING_SOCKETS; i++)
   {
      if (JB_INVALID_SOCKET != fds[i])
      {
#ifdef HAVE_POLL
         poll_fds[i].fd = fds[i];
         poll_fds[i].events = POLLIN;
         polled_sockets++;
#else
         FD_SET(fds[i], &selected_fds);
#endif
         if (max_selected_socket < fds[i] + 1)
         {
            max_selected_socket = fds[i] + 1;
         }
      }
   }
   if (0 == max_selected_socket)
   {
      return 0;
   }
   do
   {
#ifdef HAVE_POLL
      retval = poll(poll_fds, polled_sockets, -1);
#else
      retval = select(max_selected_socket, &selected_fds, NULL, NULL, NULL);
#endif
   } while (retval < 0 && errno == EINTR);
   if (retval <= 0)
   {
      if (0 == retval)
      {
         log_error(LOG_LEVEL_ERROR,
            "Waiting on new client failed because select(2) returned 0."
            " This should not happen.");
      }
      else
      {
         log_error(LOG_LEVEL_ERROR,
            "Waiting on new client failed because of problems in select(2): "
            "%s.", strerror(errno));
      }
      return 0;
   }
#ifdef HAVE_POLL
   for (i = 0; i < MAX_LISTENING_SOCKETS && (poll_fds[i].revents == 0); i++);
#else
   for (i = 0; i < MAX_LISTENING_SOCKETS && !FD_ISSET(fds[i], &selected_fds);
         i++);
#endif
   if (i >= MAX_LISTENING_SOCKETS)
   {
      log_error(LOG_LEVEL_ERROR,
         "select(2) reported connected clients (number = %u, "
         "descriptor boundary = %u), but none found.",
         retval, max_selected_socket);
      return 0;
   }
   fd = fds[i];

   /* Accept selected connection */
#ifdef _WIN32
   afd = accept (fd, (struct sockaddr *) &client, &c_length);
   if (afd == JB_INVALID_SOCKET)
   {
      return 0;
   }
#else
   do
   {
      afd = accept (fd, (struct sockaddr *) &client, &c_length);
   } while (afd < 0 && errno == EINTR);
   if (afd < 0)
   {
      return 0;
   }
#endif

#ifdef SO_LINGER
   {
      struct linger linger_options;
      linger_options.l_onoff  = 1;
      linger_options.l_linger = 5;
      if (0 != setsockopt(afd, SOL_SOCKET, SO_LINGER, &linger_options, sizeof(linger_options)))
      {
         log_error(LOG_LEVEL_ERROR, "Setting SO_LINGER on socket %d failed.", afd);
      }
   }
#endif

#ifndef HAVE_POLL
#ifndef _WIN32
   if (afd >= FD_SETSIZE)
   {
      log_error(LOG_LEVEL_ERROR,
         "Client socket number too high to use select(): %d >= %d",
         afd, FD_SETSIZE);
      close_socket(afd);
      return 0;
   }
#endif
#endif

#ifdef FEATURE_EXTERNAL_FILTERS
   mark_socket_for_close_on_execute(afd);
#endif

   set_no_delay_flag(afd);

   csp->cfd = afd;
#ifdef HAVE_RFC2553
   csp->ip_addr_str = malloc_or_die(NI_MAXHOST);
   retval = getnameinfo((struct sockaddr *) &client, c_length,
         csp->ip_addr_str, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
   if (!csp->ip_addr_str || retval)
   {
      log_error(LOG_LEVEL_ERROR, "Can not save csp->ip_addr_str: %s",
         (csp->ip_addr_str) ? gai_strerror(retval) : "Insuffcient memory");
      freez(csp->ip_addr_str);
   }
#undef client
#else
   csp->ip_addr_str  = strdup(inet_ntoa(client.sin_addr));
   csp->ip_addr_long = ntohl(client.sin_addr.s_addr);
#endif /* def HAVE_RFC2553 */

   /*
    * Save the name and port of the accepting socket for later lookup.
    *
    * The string needs space for strlen(...) + 7 characters:
    * strlen(haddr[i]) + 1 (':') + 5 (port digits) + 1 ('\0')
    */
   host_addr = (csp->config->haddr[i] != NULL) ? csp->config->haddr[i] : "";
   listen_addr_size = strlen(host_addr) + 7;
   csp->listen_addr_str = malloc_or_die(listen_addr_size);
   retval = snprintf(csp->listen_addr_str, listen_addr_size,
      "%s:%d", host_addr, csp->config->hport[i]);
   if ((-1 == retval) || listen_addr_size <= retval)
   {
      log_error(LOG_LEVEL_ERROR,
         "Server name (%s) and port number (%d) ASCII decimal representation "
         "don't fit into %lu bytes",
         host_addr, csp->config->hport[i], listen_addr_size);
      return 0;
   }

   return 1;

}


/*********************************************************************
 *
 * Function    :  resolve_hostname_to_ip
 *
 * Description :  Resolve a hostname to an internet tcp/ip address.
 *                NULL or an empty string resolve to INADDR_ANY.
 *
 * Parameters  :
 *          1  :  host = hostname to resolve
 *
 * Returns     :  INADDR_NONE => failure, INADDR_ANY or tcp/ip address if successful.
 *
 *********************************************************************/
unsigned long resolve_hostname_to_ip(const char *host)
{
   struct sockaddr_in inaddr;
   struct hostent *hostp;
#if defined(HAVE_GETHOSTBYNAME_R_6_ARGS) || defined(HAVE_GETHOSTBYNAME_R_5_ARGS) || defined(HAVE_GETHOSTBYNAME_R_3_ARGS)
   struct hostent result;
#if defined(HAVE_GETHOSTBYNAME_R_6_ARGS) || defined(HAVE_GETHOSTBYNAME_R_5_ARGS)
   char hbuf[HOSTENT_BUFFER_SIZE];
   int thd_err;
#else /* defined(HAVE_GETHOSTBYNAME_R_3_ARGS) */
   struct hostent_data hdata;
#endif /* def HAVE_GETHOSTBYNAME_R_(6|5)_ARGS */
#endif /* def HAVE_GETHOSTBYNAME_R_(6|5|3)_ARGS */

   if ((host == NULL) || (*host == '\0'))
   {
      return(INADDR_ANY);
   }

   memset((char *) &inaddr, 0, sizeof inaddr);

   if ((inaddr.sin_addr.s_addr = inet_addr(host)) == -1)
   {
      unsigned int dns_retries = 0;
#if defined(HAVE_GETHOSTBYNAME_R_6_ARGS)
      while (gethostbyname_r(host, &result, hbuf,
                HOSTENT_BUFFER_SIZE, &hostp, &thd_err)
             && (thd_err == TRY_AGAIN) && (dns_retries++ < MAX_DNS_RETRIES))
      {
         log_error(LOG_LEVEL_ERROR,
            "Timeout #%u while trying to resolve %s. Trying again.",
            dns_retries, host);
      }
#elif defined(HAVE_GETHOSTBYNAME_R_5_ARGS)
      while (NULL == (hostp = gethostbyname_r(host, &result,
                                 hbuf, HOSTENT_BUFFER_SIZE, &thd_err))
             && (thd_err == TRY_AGAIN) && (dns_retries++ < MAX_DNS_RETRIES))
      {
         log_error(LOG_LEVEL_ERROR,
            "Timeout #%u while trying to resolve %s. Trying again.",
            dns_retries, host);
      }
#elif defined(HAVE_GETHOSTBYNAME_R_3_ARGS)
      /*
       * XXX: Doesn't retry in case of soft errors.
       * Does this gethostbyname_r version set h_errno?
       */
      if (0 == gethostbyname_r(host, &result, &hdata))
      {
         hostp = &result;
      }
      else
      {
         hostp = NULL;
      }
#elif defined(MUTEX_LOCKS_AVAILABLE)
      privoxy_mutex_lock(&resolver_mutex);
      while (NULL == (hostp = gethostbyname(host))
             && (h_errno == TRY_AGAIN) && (dns_retries++ < MAX_DNS_RETRIES))
      {
         log_error(LOG_LEVEL_ERROR,
            "Timeout #%u while trying to resolve %s. Trying again.",
            dns_retries, host);
      }
      privoxy_mutex_unlock(&resolver_mutex);
#else
      while (NULL == (hostp = gethostbyname(host))
             && (h_errno == TRY_AGAIN) && (dns_retries++ < MAX_DNS_RETRIES))
      {
         log_error(LOG_LEVEL_ERROR,
            "Timeout #%u while trying to resolve %s. Trying again.",
            dns_retries, host);
      }
#endif /* def HAVE_GETHOSTBYNAME_R_(6|5|3)_ARGS */
      /*
       * On Mac OSX, if a domain exists but doesn't have a type A
       * record associated with it, the h_addr member of the struct
       * hostent returned by gethostbyname is NULL, even if h_length
       * is 4. Therefore the second test below.
       */
      if (hostp == NULL || hostp->h_addr == NULL)
      {
         errno = EINVAL;
         log_error(LOG_LEVEL_ERROR, "could not resolve hostname %s", host);
         return(INADDR_NONE);
      }
      if (hostp->h_addrtype != AF_INET)
      {
#ifdef _WIN32
         errno = WSAEPROTOTYPE;
#else
         errno = EPROTOTYPE;
#endif
         log_error(LOG_LEVEL_ERROR, "hostname %s resolves to unknown address type.", host);
         return(INADDR_NONE);
      }
      memcpy((char *)&inaddr.sin_addr, (char *)hostp->h_addr, sizeof(inaddr.sin_addr));
   }
   return(inaddr.sin_addr.s_addr);

}


/*********************************************************************
 *
 * Function    :  socket_is_still_alive
 *
 * Description :  Figures out whether or not a socket is still alive.
 *
 * Parameters  :
 *          1  :  sfd = The socket to check.
 *
 * Returns     :  TRUE for yes, otherwise FALSE.
 *
 *********************************************************************/
int socket_is_still_alive(jb_socket sfd)
{
   char buf[10];
   int no_data_waiting;
#ifdef HAVE_POLL
   int poll_result;
   struct pollfd poll_fd[1];

   memset(poll_fd, 0, sizeof(poll_fd));
   poll_fd[0].fd = sfd;
   poll_fd[0].events = POLLIN;

   poll_result = poll(poll_fd, 1, 0);

   if (-1 == poll_result)
   {
      log_error(LOG_LEVEL_CONNECT, "Polling socket %d failed.", sfd);
      return FALSE;
   }
   no_data_waiting = !(poll_fd[0].revents & POLLIN);
#else
   fd_set readable_fds;
   struct timeval timeout;
   int ret;

   memset(&timeout, '\0', sizeof(timeout));
   FD_ZERO(&readable_fds);
   FD_SET(sfd, &readable_fds);

   ret = select((int)sfd+1, &readable_fds, NULL, NULL, &timeout);
   if (ret < 0)
   {
      log_error(LOG_LEVEL_CONNECT, "select() on socket %d failed: %E", sfd);
      return FALSE;
   }
   no_data_waiting = !FD_ISSET(sfd, &readable_fds);
#endif /* def HAVE_POLL */

   return (no_data_waiting || (1 == recv(sfd, buf, 1, MSG_PEEK)));
}


#ifdef FEATURE_EXTERNAL_FILTERS
/*********************************************************************
 *
 * Function    :  mark_socket_for_close_on_execute
 *
 * Description :  Marks a socket for close on execute.
 *
 *                Used so that external filters have no direct
 *                access to sockets they shouldn't care about.
 *
 *                Not implemented for all platforms.
 *
 * Parameters  :
 *          1  :  fd = The socket to mark
 *
 * Returns     :  void.
 *
 *********************************************************************/
void mark_socket_for_close_on_execute(jb_socket fd)
{
#ifdef FEATURE_PTHREAD
   int ret;

   ret = fcntl(fd, F_SETFD, FD_CLOEXEC);

   if (ret == -1)
   {
      log_error(LOG_LEVEL_ERROR,
         "fcntl(%d, F_SETFD, FD_CLOEXEC) failed", fd);
   }
#else
#warning "Sockets will be visible to external filters"
#endif
}
#endif /* def FEATURE_EXTERNAL_FILTERS */

/*
  Local Variables:
  tab-width: 3
  end:
*/
