const char gateway_rcs[] = "$Id: gateway.c,v 1.1 2001/05/13 21:57:06 administrator Exp $";
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/gateway.c,v $
 *
 * Purpose     :  Contains functions to connect to a server, possibly
 *                using a "gateway" (i.e. HTTP proxy and/or SOCKS4
 *                proxy).  Also contains the list of gateway types.
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
 *    $Log: gateway.c,v $
 *
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#endif /* def _WIN32 */

#include "jcc.h"
#include "errlog.h"
#include "jbsockets.h"
#include "gateway.h"

const char gateway_h_rcs[] = GATEWAY_H_VERSION;

#define SOCKS_4      40    /* original SOCKS 4 protocol */
#define SOCKS_4A     41    /* as modified for hosts w/o external DNS */

const struct gateway gateways[] = {
   /* type        function          gw type/host/port, fw host/port */
   { "direct",    direct_connect,   0,          NULL, 0,    NULL, 0 },
   { ".",         direct_connect,   0,          NULL, 0,    NULL, 0 },
   { "socks",     socks4_connect,   SOCKS_4,    NULL, 1080, NULL, 0 },
   { "socks4",    socks4_connect,   SOCKS_4,    NULL, 1080, NULL, 0 },
   { "socks4a",   socks4_connect,   SOCKS_4A,   NULL, 1080, NULL, 0 },
   { NULL,        NULL,             0,          NULL, 0,    NULL, 0 }
};

const struct gateway *gw_default = gateways; /* direct */


#define SOCKS_REQUEST_GRANTED          90
#define SOCKS_REQUEST_REJECT           91
#define SOCKS_REQUEST_IDENT_FAILED     92
#define SOCKS_REQUEST_IDENT_CONFLICT   93

/* structure of a socks client operation */
struct socks_op {
   unsigned char vn;          /* socks version number */
   unsigned char cd;          /* command code */
   unsigned char dstport[2];  /* destination port */
   unsigned char dstip[4];    /* destination address */
   unsigned char userid;      /* first byte of userid */
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


/*********************************************************************
 *
 * Function    :  direct_connect
 *
 * Description :  Direct how we connect to the web.  This can be:
 *                directly    : no forwarding, or
 *                indirectly  : through another proxy such as squid.
 *
 * Parameters  :
 *          1  :  gw = pointer to a gateway structure (such as gw_default)
 *          2  :  http = the http request and apropos headers
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  -1 => failure, else it is the socket file descriptor.
 *
 *********************************************************************/
int direct_connect(const struct gateway *gw, struct http_request *http, struct client_state *csp)
{
   if (gw->forward_host)
   {
      return(connect_to(gw->forward_host, gw->forward_port, csp));
   }
   else
   {
      return(connect_to(http->host, http->port, csp));
   }

}


/*********************************************************************
 *
 * Function    :  socks4_connect
 *
 * Description :  Connect to the SOCKS server, and connect through
 *                it to the web server or web proxy.   This handles
 *                all the SOCKS negotiation, and returns a file
 *                descriptor for a socket which can be treated as a
 *                normal (non-SOCKS) socket.
 *
 * Parameters  :
 *          1  :  gw = pointer to a gateway structure (such as gw_default)
 *          2  :  http = the http request and apropos headers
 *          3  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  -1 => failure, else a socket file descriptor.
 *
 *********************************************************************/
int socks4_connect(const struct gateway *gw, struct http_request *http, struct client_state *csp)
{
   int web_server_addr;
   unsigned char cbuf[BUFSIZ];
   unsigned char sbuf[BUFSIZ];
   struct socks_op    *c = (struct socks_op    *)cbuf;
   struct socks_reply *s = (struct socks_reply *)sbuf;
   int n, csiz, sfd, target_port;
   int err = 0;
   char *errstr, *target_host;

   if ((gw->gateway_host == NULL) || (*gw->gateway_host == '\0'))
   {
      log_error(LOG_LEVEL_CONNECT, "socks4_connect: NULL gateway host specified");
      err = 1;
   }

   if (gw->gateway_port <= 0)
   {
      log_error(LOG_LEVEL_CONNECT, "socks4_connect: invalid gateway port specified");
      err = 1;
   }

   if (err)
   {
      errno = EINVAL;
      return(-1);
   }

   if (gw->forward_host)
   {
      target_host = gw->forward_host;
      target_port = gw->forward_port;
   }
   else
   {
      target_host = http->host;
      target_port = http->port;
   }

   /* build a socks request for connection to the web server */

   strcpy((char *)&(c->userid), socks_userid);

   csiz = sizeof(*c) + sizeof(socks_userid) - 1;

   switch (gw->type)
   {
      case SOCKS_4:
         web_server_addr = htonl(resolve_hostname_to_ip(target_host));
         break;
      case SOCKS_4A:
         web_server_addr = 0x00000001;
         n = csiz + strlen(target_host) + 1;
         if (n > sizeof(cbuf))
         {
            errno = EINVAL;
            return(-1);
         }
         strcpy(((char *)cbuf) + csiz, http->host);
         csiz = n;
         break;
      default:
         /* Should never get here */
         log_error(LOG_LEVEL_ERROR, "SOCKS4 impossible internal error - bad SOCKS type.");
         errno = EINVAL;
         return(-1);
   }

   c->vn          = 4;
   c->cd          = 1;
   c->dstport[0]  = (target_port       >> 8  ) & 0xff;
   c->dstport[1]  = (target_port             ) & 0xff;
   c->dstip[0]    = (web_server_addr   >> 24 ) & 0xff;
   c->dstip[1]    = (web_server_addr   >> 16 ) & 0xff;
   c->dstip[2]    = (web_server_addr   >>  8 ) & 0xff;
   c->dstip[3]    = (web_server_addr         ) & 0xff;

   /* pass the request to the socks server */
   sfd = connect_to(gw->gateway_host, gw->gateway_port, csp);

   if (sfd < 0)
   {
      return(-1);
   }

   if ((n = write_socket(sfd, (char *)c, csiz)) != csiz)
   {
      log_error(LOG_LEVEL_CONNECT, "SOCKS4 negotiation write failed...");
      close_socket(sfd);
      return(-1);
   }

   if ((n = read_socket(sfd, sbuf, sizeof(sbuf))) != sizeof(*s))
   {
      log_error(LOG_LEVEL_CONNECT, "SOCKS4 negotiation read failed...");
      close_socket(sfd);
      return(-1);
   }

   switch (s->cd)
   {
      case SOCKS_REQUEST_GRANTED:
         return(sfd);
         break;
      case SOCKS_REQUEST_REJECT:
         errstr = "SOCKS request rejected or failed";
         errno = EINVAL;
         break;
      case SOCKS_REQUEST_IDENT_FAILED:
         errstr = "SOCKS request rejected because "
            "SOCKS server cannot connect to identd on the client";
         errno = EACCES;
         break;
      case SOCKS_REQUEST_IDENT_CONFLICT:
         errstr = "SOCKS request rejected because "
            "the client program and identd report "
            "different user-ids";
         errno = EACCES;
         break;
      default:
         errstr = (char *) cbuf;
         errno = ENOENT;
         sprintf(errstr,
                 "SOCKS request rejected for reason code %d\n", s->cd);
   }

   log_error(LOG_LEVEL_CONNECT, "socks4_connect: %s ...", errstr);

   close_socket(sfd);
   return(-1);

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
