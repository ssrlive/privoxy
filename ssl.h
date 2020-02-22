#ifndef SSL_H_INCLUDED
#define SSL_H_INCLUDED
/*********************************************************************
*
* File        :  $Source: /cvsroot/ijbswa/current/ssl.h,v $
*
* Purpose     :  File with TLS/SSL extension. Contains methods for
*                creating, using and closing TLS/SSL connections.
*
* Copyright   :  Written by and Copyright (c) 2017 Vaclav Svec. FIT CVUT.
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

#include "project.h"

/*
 * Values for flag determining certificate validity.
 * These values are compatible with return value of function
 * mbedtls_ssl_get_verify_result(). There is no value for
 * "invalid certificate", this value is set by the function
 * mbedtls_ssl_get_verify_result().
 */
#define SSL_CERT_VALID          0
#define SSL_CERT_NOT_VERIFIED   0xFFFFFFFF

/* Variables for one common RNG for all SSL use */
static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_entropy_context  entropy;
static int rng_seeded;

/* Boolean functions to get informations about TLS/SSL connections */
extern int    client_use_ssl(const struct client_state *csp);
extern int    server_use_ssl(const struct client_state *csp);
extern size_t is_ssl_pending(mbedtls_ssl_context *ssl);
extern int tunnel_established_successfully(const char * response, unsigned int response_len);

/* Functions for sending and receiving data over TLS/SSL connections */
extern int  ssl_send_data(mbedtls_ssl_context * ssl, const unsigned char * buf, size_t len);
extern int  ssl_recv_data(mbedtls_ssl_context * ssl, unsigned char * buf, size_t maxLen);
extern long ssl_flush_socket(mbedtls_ssl_context * ssl, struct iob *iob);
extern void ssl_send_certificate_error(struct client_state *csp);

/* Functions for opening and closing TLS/SSL connections */
extern int  create_client_ssl_connection(struct client_state *csp);
extern int  create_server_ssl_connection(struct client_state *csp);
extern void close_client_and_server_ssl_connections(struct client_state *csp);
extern void close_client_ssl_connection(struct client_state *csp);

#endif /* ndef SSL_H_INCLUDED */
