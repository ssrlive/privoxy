const char jbsockets_rcs[] = "$Id: jbsockets.c,v 1.8 2001/06/03 19:12:07 oes Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/jbsockets.c,v $
 *
 * Purpose     :  Contains wrappers for system-specific sockets code,
 *                so that the rest of JunkBuster can be more
 *                OS-independent.  Contains #ifdefs to make this work
 *                on many platforms.
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
 *    $Log: jbsockets.c,v $
 *    Revision 1.8  2001/06/03 19:12:07  oes
 *    filled comment
 *
 *    Revision 1.8  2001/06/03 11:03:48  oes
 *    Makefile/in
 *
 *    introduced cgi.c
 *
 *    actions.c:
 *
 *    adapted to new enlist_unique arg format
 *
 *    conf loadcfg.c
 *
 *    introduced confdir option
 *
 *    filters.c filtrers.h
 *
 *     extracted-CGI relevant stuff
 *
 *    jbsockets.c
 *
 *     filled comment
 *
 *    jcc.c
 *
 *     support for new cgi mechansim
 *
 *    list.c list.h
 *
 *    functions for new list type: "map"
 *    extended enlist_unique
 *
 *    miscutil.c .h
 *    introduced bindup()
 *
 *    parsers.c parsers.h
 *
 *    deleted const struct interceptors
 *
 *    pcrs.c
 *    added FIXME
 *
 *    project.h
 *
 *    added struct map
 *    added struct http_response
 *    changes struct interceptors to struct cgi_dispatcher
 *    moved HTML stuff to cgi.h
 *
 *    re_filterfile:
 *
 *    changed
 *
 *    showargs.c
 *    NO TIME LEFT
 *
 *    Revision 1.7  2001/05/28 16:14:00  jongfoster
 *    Fixing bug in LOG_LEVEL_LOG
 *
 *    Revision 1.6  2001/05/26 17:28:32  jongfoster
 *    Fixed LOG_LEVEL_LOG
 *
 *    Revision 1.5  2001/05/26 15:26:15  jongfoster
 *    ACL feature now provides more security by immediately dropping
 *    connections from untrusted hosts.
 *
 *    Revision 1.4  2001/05/26 00:37:42  jongfoster
 *    Cosmetic indentation correction.
 *
 *    Revision 1.3  2001/05/25 21:57:54  jongfoster
 *    Now gives a warning under Windows if you try to bind
 *    it to a port that's already in use.
 *
 *    Revision 1.2  2001/05/17 23:01:01  oes
 *     - Cleaned CRLF's from the sources and related files
 *
 *    Revision 1.1.1.1  2001/05/15 13:58:54  oes
 *    Initial import of version 2.9.3 source tree
 *
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

#include "project.h"
#include "jbsockets.h"
#include "filters.h"
#include "errlog.h"

const char jbsockets_h_rcs[] = JBSOCKETS_H_VERSION;


/*********************************************************************
 *
 * Function    :  connect_to
 *
 * Description :  Open a socket and connect to it.  Will check
 *                that this is allowed according to ACL.
 *
 * Parameters  :
 *          1  :  host = hostname to connect to
 *          2  :  portnum = port to connent on
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *                      Not modified, only used for source IP and ACL.
 *
 * Returns     :  -1 => failure, else it is the socket file descriptor.
 *
 *********************************************************************/
int connect_to(const char *host, int portnum, struct client_state *csp)
{
   struct sockaddr_in inaddr;
   int   fd, addr;
   fd_set wfds;
   struct timeval tv[1];
#if !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA)
   int   flags;
#endif /* !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA) */

#ifdef ACL_FILES
   struct access_control_addr dst[1];
#endif /* def ACL_FILES */

   memset((char *)&inaddr, 0, sizeof inaddr);

   if ((addr = resolve_hostname_to_ip(host)) == -1)
   {
      return(-1);
   }

#ifdef ACL_FILES
   dst->addr = ntohl(addr);
   dst->port = portnum;

   if (block_acl(dst, csp))
   {
      errno = EPERM;
      return(-1);
   }
#endif /* def ACL_FILES */

   inaddr.sin_addr.s_addr = addr;
   inaddr.sin_family      = AF_INET;

   if (sizeof(inaddr.sin_port) == sizeof(short))
   {
      inaddr.sin_port = htons((short)portnum);
   }
   else
   {
      inaddr.sin_port = htonl(portnum);
   }

   if ((fd = socket(inaddr.sin_family, SOCK_STREAM, 0)) < 0)
   {
      return(-1);
   }

#ifdef TCP_NODELAY
   {  /* turn off TCP coalescence */
      int mi = 1;
      setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, (char *) &mi, sizeof (int));
   }
#endif /* def TCP_NODELAY */

#if !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA)
   if ((flags = fcntl(fd, F_GETFL, 0)) != -1)
   {
      flags |= O_NDELAY;
      fcntl(fd, F_SETFL, flags);
   }
#endif /* !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA) */

   while (connect(fd, (struct sockaddr *) & inaddr, sizeof inaddr) == -1)
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
         return(-1);
      }
   }

#if !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA)
   if (flags != -1)
   {
      flags &= ~O_NDELAY;
      fcntl(fd, F_SETFL, flags);
   }
#endif /* !defined(_WIN32) && !defined(__BEOS__) && !defined(AMIGA) */

   /* wait for connection to complete */
   FD_ZERO(&wfds);
   FD_SET(fd, &wfds);

   tv->tv_sec  = 30;
   tv->tv_usec = 0;

   if (select(fd + 1, NULL, &wfds, NULL, tv) <= 0)
   {
      close_socket(fd);
      return(-1);
   }
   return(fd);

}


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
 * Returns     :  Win32 & Unix: If no error occurs, returns the total number of
 *                bytes sent, which can be less than the number
 *                indicated by len. Otherwise, returns (-1).
 *
 *********************************************************************/
int write_socket(int fd, const char *buf, int len)
{
   if (len <= 0)
   {
      return(0);
   }

   log_error(LOG_LEVEL_LOG, "%N", len, buf);

#if defined(_WIN32) || defined(__BEOS__) || defined(AMIGA)
   return( send(fd, buf, len, 0));
#else
   return( write(fd, buf, len));
#endif

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
 *                because we are reading from a pipe, or from a terminal),
 *                or because read() was interrupted by a signal.  On error,
 *                -1 is returned, and errno is set appropriately.  In this
 *                case it is left unspecified whether the file position (if
 *                any) changes.
 *
 *********************************************************************/
int read_socket(int fd, char *buf, int len)
{
   if (len <= 0)
   {
      return(0);
   }

#if defined(_WIN32) || defined(__BEOS__) || defined(AMIGA)
   return( recv(fd, buf, len, 0));
#else
   return( read(fd, buf, len));
#endif
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
void close_socket(int fd)
{
#if defined(_WIN32) || defined(__BEOS__)
   closesocket(fd);
#elif defined(AMIGA)
   CloseSocket(fd); 
#else
   close(fd);
#endif

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
 *
 * Returns     :  if success, return file descriptor
 *                if failure, returns -2 if address is in use, otherwise -1
 *
 *********************************************************************/
int bind_port(const char *hostnam, int portnum)
{
   struct sockaddr_in inaddr;
   int fd;
   int one = 1;

   memset((char *)&inaddr, '\0', sizeof inaddr);

   inaddr.sin_family      = AF_INET;
   inaddr.sin_addr.s_addr = resolve_hostname_to_ip(hostnam);

   if (sizeof(inaddr.sin_port) == sizeof(short))
   {
      inaddr.sin_port = htons((short)portnum);
   }
   else
   {
      inaddr.sin_port = htonl(portnum);
   }

   fd = socket(AF_INET, SOCK_STREAM, 0);

   if (fd < 0)
   {
      return(-1);
   }

#ifndef _WIN32
   /*
    * FIXME: This is not needed for Win32 - in fact, it stops
    * duplicate instances of JunkBuster from being caught.
    * Is this really needed under UNIX, or should it be taked out?
    * -- Jon
    */
   setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
#endif /* ndef _WIN32 */

   if (bind (fd, (struct sockaddr *)&inaddr, sizeof(inaddr)) < 0)
   {
      close_socket (fd);
#ifdef _WIN32
      if (errno == WSAEADDRINUSE)
#else
      if (errno == EADDRINUSE)
#endif
      {
         return(-2);
      }
      else
      {
         return(-1);
      }
   }

   while (listen(fd, 5) == -1)
   {
      if (errno != EINTR)
      {
         return(-1);
      }
   }

   return fd;

}


/*********************************************************************
 *
 * Function    :  accept_connection
 *
 * Description :  Accepts a connection on a socket.  Socket must have
 *                been created using bind_port().
 *
 * Parameters  :
 *          1  :  csp = Client state, cfd, ip_addr_str, and 
 *                ip_addr_long will be set by this routine.
 *          2  :  fd  = file descriptor returned from bind_port
 *
 * Returns     :  when a connection is accepted, it returns 1 (TRUE).
 *                On an error it returns 0 (FALSE).
 *
 *********************************************************************/
int accept_connection(struct client_state * csp, int fd)
{
   struct sockaddr raddr;
   struct sockaddr_in *rap = (struct sockaddr_in *) &raddr;
   int   afd, raddrlen;

   raddrlen = sizeof raddr;
   do
   {
      afd = accept (fd, &raddr, &raddrlen);
   } while (afd < 1 && errno == EINTR);

   if (afd < 0)
   {
      return 0;
   }

   csp->cfd    = afd;
   csp->ip_addr_str  = strdup(inet_ntoa(rap->sin_addr));
   csp->ip_addr_long = ntohl(rap->sin_addr.s_addr);

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
 * Returns     :  -1 => failure, INADDR_ANY or tcp/ip address if succesful.
 *
 *********************************************************************/
int resolve_hostname_to_ip(const char *host)
{
   struct sockaddr_in inaddr;
   struct hostent *hostp;

   if ((host == NULL) || (*host == '\0'))
   {
      return(INADDR_ANY);
   }

   memset((char *) &inaddr, 0, sizeof inaddr);

   if ((inaddr.sin_addr.s_addr = inet_addr(host)) == -1)
   {
      if ((hostp = gethostbyname(host)) == NULL)
      {
         errno = EINVAL;
         return(-1);
      }
      if (hostp->h_addrtype != AF_INET)
      {
#ifdef _WIN32
         errno = WSAEPROTOTYPE;
#else
         errno = EPROTOTYPE;
#endif
         return(-1);
      }
      memcpy(
         (char *) &inaddr.sin_addr,
         (char *) hostp->h_addr,
         sizeof(inaddr.sin_addr)
      );
   }
   return(inaddr.sin_addr.s_addr);

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
