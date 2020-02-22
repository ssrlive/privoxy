/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/ssl.c,v $
 *
 * Purpose     :  File with TLS/SSL extension. Contains methods for
 *                creating, using and closing TLS/SSL connections.
 *
 * Copyright   :  Written by and Copyright (c) 2017 Vaclav Svec. FIT CVUT.
 *                Copyright (C) 2018-2019 by Fabian Keil <fk@fabiankeil.de>
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

#include <string.h>
#include <unistd.h>

#if !defined(MBEDTLS_CONFIG_FILE)
#  include "mbedtls/config.h"
#else
#  include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/md5.h"
#include "mbedtls/pem.h"
#include "mbedtls/base64.h"
#include "mbedtls/error.h"

#include "config.h"
#include "project.h"
#include "miscutil.h"
#include "errlog.h"
#include "jcc.h"
#include "ssl.h"


/*
 * Macros for searching begin and end of certificates.
 * Necessary to convert structure mbedtls_x509_crt to crt file.
 */
#define PEM_BEGIN_CRT     "-----BEGIN CERTIFICATE-----\n"
#define PEM_END_CRT       "-----END CERTIFICATE-----\n"

/*
 * Macros for ssl.c
 */
#define ERROR_BUF_SIZE                   1024              /* Size of buffer for error messages */
#define CERTIFICATE_BUF_SIZE             16384             /* Size of buffer to save certificate. Value 4096 is mbedtls library buffer size for certificate in DER form */
#define PRIVATE_KEY_BUF_SIZE             16000             /* Size of buffer to save private key. Value 16000 is taken from mbed TLS library examples. */
#define RSA_KEY_PUBLIC_EXPONENT          65537             /* Public exponent for RSA private key generating */
#define RSA_KEYSIZE                      2048              /* Size of generated RSA keys */
#define GENERATED_CERT_VALID_FROM        "20100101000000"  /* Date and time, which will be set in generated certificates as parameter valid from */
#define GENERATED_CERT_VALID_TO          "20401231235959"  /* Date and time, which will be set in generated certificates as parameter valid to */
#define CERT_SIGNATURE_ALGORITHM         MBEDTLS_MD_SHA256 /* The MD algorithm to use for the signature */
#define CERT_SERIAL_NUM_LENGTH           4                 /* Bytes of hash to be used for creating serial number of certificate. Min=2 and max=16 */
#define INVALID_CERT_INFO_BUF_SIZE       2048              /* Size of buffer for message with information about reason of certificate invalidity. Data after the end of buffer will not be saved */
#define CERT_PARAM_COMMON_NAME           "CN="
#define CERT_PARAM_ORGANIZATION          ",O="
#define CERT_PARAM_ORG_UNIT              ",OU="
#define CERT_PARAM_COUNTRY               ",C=CZ"
#define KEY_FILE_TYPE                    ".pem"
#define CERT_FILE_TYPE                   ".crt"
#define CERT_SUBJECT_PASSWORD            ""
#define CERT_INFO_PREFIX                 ""

/*
 * Properties of cert for generating
 */
typedef struct {
   char       *issuer_crt;                         /* filename of the issuer certificate       */
   char       *subject_key;                        /* filename of the subject key file         */
   char       *issuer_key;                         /* filename of the issuer key file          */
   const char *subject_pwd;                        /* password for the subject key file        */
   const char *issuer_pwd;                         /* password for the issuer key file         */
   char       *output_file;                        /* where to store the constructed key file  */
   const char *subject_name;                       /* subject name for certificate             */
   char       issuer_name[ISSUER_NAME_BUF_SIZE];   /* issuer name for certificate              */
   const char *not_before;                         /* validity period not before               */
   const char *not_after;                          /* validity period not after                */
   const char *serial;                             /* serial number string                     */
   int        is_ca;                               /* is a CA certificate                      */
   int        max_pathlen;                         /* maximum CA path length                   */
} cert_options;

/*
 * Properties of key for generating
 */
typedef struct {
   mbedtls_pk_type_t type;   /* type of key to generate  */
   int  rsa_keysize;         /* length of key in bits    */
   char *key_file_path;      /* filename of the key file */
} key_options;

static int generate_webpage_certificate(struct client_state *csp);
static char *make_certs_path(const char *conf_dir, const char *file_name, const char *suffix);
static int file_exists(const char *path);
static int host_to_hash(struct client_state *csp);
static int ssl_verify_callback(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags);
static void free_certificate_chain(struct client_state *csp);
static unsigned int get_certificate_mutex_id(struct client_state *csp);
static unsigned long  get_certificate_serial(struct client_state *csp);
static void free_client_ssl_structures(struct client_state *csp);
static void free_server_ssl_structures(struct client_state *csp);
static int seed_rng(struct client_state *csp);

/*********************************************************************
 *
 * Function    :  client_use_ssl
 *
 * Description :  Tests if client in current client state structure
 *                should use SSL connection or standard connection.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  If client should use TLS/SSL connection, 1 is returned.
 *                Otherwise 0 is returned.
 *
 *********************************************************************/
extern int client_use_ssl(const struct client_state *csp)
{
   return csp->http->client_ssl;
}


/*********************************************************************
 *
 * Function    :  server_use_ssl
 *
 * Description :  Tests if server in current client state structure
 *                should use SSL connection or standard connection.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  If server should use TLS/SSL connection, 1 is returned.
 *                Otherwise 0 is returned.
 *
 *********************************************************************/
extern int server_use_ssl(const struct client_state *csp)
{
   return csp->http->server_ssl;
}


/*********************************************************************
 *
 * Function    :  is_ssl_pending
 *
 * Description :  Tests if there are some waiting data on ssl connection
 *
 * Parameters  :
 *          1  :  ssl = SSL context to test
 *
 * Returns     :   0 => No data are pending
 *                >0 => Pending data length
 *
 *********************************************************************/
extern size_t is_ssl_pending(mbedtls_ssl_context *ssl)
{
   if (ssl == NULL)
   {
      return 0;
   }

   return mbedtls_ssl_get_bytes_avail(ssl);
}


/*********************************************************************
 *
 * Function    :  ssl_send_data
 *
 * Description :  Sends the content of buf (for n bytes) to given SSL
 *                connection context.
 *
 * Parameters  :
 *          1  :  ssl = SSL context to send data to
 *          2  :  buf = Pointer to data to be sent
 *          3  :  len = Length of data to be sent to the SSL context
 *
 * Returns     :  Length of sent data or negative value on error.
 *
 *********************************************************************/
extern int ssl_send_data(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len)
{
   int ret = 0;
   size_t max_fragment_size = 0;  /* Maximal length of data in one SSL fragment*/
   int send_len             = 0;  /* length of one data part to send */
   int pos                  = 0;  /* Position of unsent part in buffer */

   if (len == 0)
   {
      return 0;
   }

   /* Getting maximal length of data sent in one fragment */
   max_fragment_size = mbedtls_ssl_get_max_frag_len(ssl);

   /*
    * Whole buffer must be sent in many fragments, because each fragment
    * has its maximal length.
    */
   while (pos < len)
   {
      /* Compute length of data, that can be send in next fragment */
      if ((pos + (int)max_fragment_size) > len)
      {
         send_len = (int)len - pos;
      }
      else
      {
         send_len = (int)max_fragment_size;
      }

      /*
       * Sending one part of the buffer
       */
      while ((ret = mbedtls_ssl_write(ssl,
         (const unsigned char *)(buf + pos),
         (size_t)send_len)) < 0)
      {
         if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
             ret != MBEDTLS_ERR_SSL_WANT_WRITE)
         {
            char err_buf[ERROR_BUF_SIZE];

            mbedtls_strerror(ret, err_buf, sizeof(err_buf));
            log_error(LOG_LEVEL_ERROR,
               "Sending data over TLS/SSL failed: %s", err_buf);
            return -1;
         }
      }
      /* Adding count of sent bytes to position in buffer */
      pos = pos + send_len;
   }

   return (int)len;
}


/*********************************************************************
 *
 * Function    :  ssl_recv_data
 *
 * Description :  Receives data from given SSL context and puts
 *                it into buffer.
 *
 * Parameters  :
 *          1  :  ssl = SSL context to receive data from
 *          2  :  buf = Pointer to buffer where data will be written
 *          3  :  max_length = Maximum number of bytes to read
 *
 * Returns     :  Number of bytes read, 0 for EOF, or negative
 *                value on error.
 *
 *********************************************************************/
extern int ssl_recv_data(mbedtls_ssl_context *ssl, unsigned char *buf, size_t max_length)
{
   int ret = 0;
   memset(buf, 0, max_length);

   /*
    * Receiving data from SSL context into buffer
    */
   do
   {
      ret = mbedtls_ssl_read(ssl, buf, max_length);
   } while (ret == MBEDTLS_ERR_SSL_WANT_READ
      || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

   if (ret < 0)
   {
      char err_buf[ERROR_BUF_SIZE];

      if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
      {
         log_error(LOG_LEVEL_CONNECT,
            "The peer notified us that the connection is going to be closed");
         return 0;
      }
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Receiving data over TLS/SSL failed: %s", err_buf);
   }

   return ret;
}


/*********************************************************************
 *
 * Function    :  ssl_flush_socket
 *
 * Description :  Send any pending "buffered" content with given
 *                SSL connection. Alternative to function flush_socket.
 *
 * Parameters  :
 *          1  :  ssl = SSL context to send buffer to
 *          2  :  iob = The I/O buffer to flush, usually csp->iob.
 *
 * Returns     :  On success, the number of bytes send are returned (zero
 *                indicates nothing was sent).  On error, -1 is returned.
 *
 *********************************************************************/
extern long ssl_flush_socket(mbedtls_ssl_context *ssl, struct iob *iob)
{
   /* Computing length of buffer part to send */
   long len = iob->eod - iob->cur;

   if (len <= 0)
   {
      return(0);
   }

   /* Sending data to given SSl context */
   if (ssl_send_data(ssl, (const unsigned char *)iob->cur, (size_t)len) < 0)
   {
      return -1;
   }
   iob->eod = iob->cur = iob->buf;
   return(len);
}


/*********************************************************************
 *
 * Function    :  ssl_debug_callback
 *
 * Description :  Debug callback function for mbedtls library.
 *                Prints info into log file.
 *
 * Parameters  :
 *          1  :  ctx   = File to save log in
 *          2  :  level = Debug level
 *          3  :  file  = File calling debug message
 *          4  :  line  = Line calling debug message
 *          5  :  str   = Debug message
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void ssl_debug_callback(void *ctx, int level, const char *file, int line, const char *str)
{
   /*
   ((void)level);
   fprintf((FILE *)ctx, "%s:%04d: %s", file, line, str);
   fflush((FILE *)ctx);
   log_error(LOG_LEVEL_INFO, "SSL debug message: %s:%04d: %s", file, line, str);
   */
}


/*********************************************************************
 *
 * Function    :  create_client_ssl_connection
 *
 * Description :  Creates TLS/SSL secured connection with client
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 on success, negative value if connection wasn't created
 *                successfully.
 *
 *********************************************************************/
extern int create_client_ssl_connection(struct client_state *csp)
{
   /* Paths to certificates file and key file */
   char *key_file  = NULL;
   char *ca_file   = NULL;
   char *cert_file = NULL;
   int ret = 0;
   char err_buf[ERROR_BUF_SIZE];

   /*
    * Initializing mbedtls structures for TLS/SSL connection
    */
   mbedtls_net_init(&(csp->mbedtls_client_attr.socket_fd));
   mbedtls_ssl_init(&(csp->mbedtls_client_attr.ssl));
   mbedtls_ssl_config_init(&(csp->mbedtls_client_attr.conf));
   mbedtls_x509_crt_init(&(csp->mbedtls_client_attr.server_cert));
   mbedtls_pk_init(&(csp->mbedtls_client_attr.prim_key));
#if defined(MBEDTLS_SSL_CACHE_C)
   mbedtls_ssl_cache_init(&(csp->mbedtls_client_attr.cache));
#endif

   /*
    * Preparing hash of host for creating certificates
    */
   ret = host_to_hash(csp);
   if (ret != 0)
   {
      log_error(LOG_LEVEL_ERROR, "Generating hash of host failed: %d", ret);
      ret = -1;
      goto exit;
   }

   /*
    * Preparing paths to certificates files and key file
    */
   ca_file   = csp->config->ca_cert_file;
   cert_file = make_certs_path(csp->config->certificate_directory,
      (const char *)csp->http->hash_of_host_hex, CERT_FILE_TYPE);
   key_file  = make_certs_path(csp->config->certificate_directory,
      (const char *)csp->http->hash_of_host_hex, KEY_FILE_TYPE);

   if (cert_file == NULL || key_file == NULL)
   {
      ret = -1;
      goto exit;
   }

   /*
    * Generating certificate for requested host. Mutex to prevent
    * certificate and key inconsistence must be locked.
    */
   unsigned int cert_mutex_id = get_certificate_mutex_id(csp);
   privoxy_mutex_lock(&(certificates_mutexes[cert_mutex_id]));

   ret = generate_webpage_certificate(csp);
   if (ret < 0)
   {
      log_error(LOG_LEVEL_ERROR,
         "Generate_webpage_certificate failed: %d", ret);
      privoxy_mutex_unlock(&(certificates_mutexes[cert_mutex_id]));
      ret = -1;
      goto exit;
   }
   privoxy_mutex_unlock(&(certificates_mutexes[cert_mutex_id]));

   /*
    * Seed the RNG
    */
   ret = seed_rng(csp);
   if (ret != 0)
   {
      ret = -1;
      goto exit;
   }

   /*
    * Loading CA file, webpage certificate and key files
    */
   ret = mbedtls_x509_crt_parse_file(&(csp->mbedtls_client_attr.server_cert),
      cert_file);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Loading webpage certificate %s failed: %s", cert_file, err_buf);
      ret = -1;
      goto exit;
   }

   ret = mbedtls_x509_crt_parse_file(&(csp->mbedtls_client_attr.server_cert),
      ca_file);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Loading CA certificate %s failed: %s", ca_file, err_buf);
      ret = -1;
      goto exit;
   }

   ret = mbedtls_pk_parse_keyfile(&(csp->mbedtls_client_attr.prim_key),
      key_file, NULL);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Loading and parsing webpage certificate private key %s failed: %s",
         key_file, err_buf);
      ret = -1;
      goto exit;
   }

   /*
    * Setting SSL parameters
    */
   ret = mbedtls_ssl_config_defaults(&(csp->mbedtls_client_attr.conf),
      MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM,
      MBEDTLS_SSL_PRESET_DEFAULT);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "mbedtls_ssl_config_defaults failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   mbedtls_ssl_conf_rng(&(csp->mbedtls_client_attr.conf),
      mbedtls_ctr_drbg_random, &ctr_drbg);
   mbedtls_ssl_conf_dbg(&(csp->mbedtls_client_attr.conf),
      ssl_debug_callback, stdout);

#if defined(MBEDTLS_SSL_CACHE_C)
   mbedtls_ssl_conf_session_cache(&(csp->mbedtls_client_attr.conf),
      &(csp->mbedtls_client_attr.cache), mbedtls_ssl_cache_get,
      mbedtls_ssl_cache_set);
#endif

   /*
    * Setting certificates
    */
   ret = mbedtls_ssl_conf_own_cert(&(csp->mbedtls_client_attr.conf),
      &(csp->mbedtls_client_attr.server_cert),
      &(csp->mbedtls_client_attr.prim_key));
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "mbedtls_ssl_conf_own_cert failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   ret = mbedtls_ssl_setup(&(csp->mbedtls_client_attr.ssl),
      &(csp->mbedtls_client_attr.conf));
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "mbedtls_ssl_setup failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   mbedtls_ssl_set_bio(&(csp->mbedtls_client_attr.ssl),
      &(csp->mbedtls_client_attr.socket_fd), mbedtls_net_send,
      mbedtls_net_recv, NULL);
   mbedtls_ssl_session_reset(&(csp->mbedtls_client_attr.ssl));

   /*
    * Setting socket fd in mbedtls_net_context structure. This structure
    * can't be set by mbedtls functions, because we already have created
    * a TCP connection when this function is called.
    */
   csp->mbedtls_client_attr.socket_fd.fd = csp->cfd;

   /*
    *  Handshake with client
    */
   log_error(LOG_LEVEL_CONNECT,
      "Performing the TLS/SSL handshake with client. Hash of host: %s",
      csp->http->hash_of_host_hex);
   while ((ret = mbedtls_ssl_handshake(&(csp->mbedtls_client_attr.ssl))) != 0)
   {
      if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
          ret != MBEDTLS_ERR_SSL_WANT_WRITE)
      {
         mbedtls_strerror(ret, err_buf, sizeof(err_buf));
         log_error(LOG_LEVEL_ERROR,
            "medtls_ssl_handshake with client failed: %s", err_buf);
         ret = -1;
         goto exit;
      }
   }

   log_error(LOG_LEVEL_CONNECT, "Client successfully connected over TLS/SSL");
   csp->ssl_with_client_is_opened = 1;

exit:
   /*
    * Freeing allocated paths to files
    */
   freez(cert_file);
   freez(key_file);

   /* Freeing structures if connection wasn't created successfully */
   if (ret < 0)
   {
      free_client_ssl_structures(csp);
   }
   return ret;
}


/*********************************************************************
 *
 * Function    :  close_client_ssl_connection
 *
 * Description :  Closes TLS/SSL connection with client. This function
 *                checks if this connection is already created.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
extern void close_client_ssl_connection(struct client_state *csp)
{
   int ret = 0;

   if (csp->ssl_with_client_is_opened == 0)
   {
      return;
   }

   /*
    * Notifying the peer that the connection is being closed.
    */
   do {
      ret = mbedtls_ssl_close_notify(&(csp->mbedtls_client_attr.ssl));
   } while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);

   free_client_ssl_structures(csp);
   csp->ssl_with_client_is_opened = 0;
}


/*********************************************************************
 *
 * Function    :  free_client_ssl_structures
 *
 * Description :  Frees structures used for SSL communication with
 *                client.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void free_client_ssl_structures(struct client_state *csp)
{
   /*
   * We can't use function mbedtls_net_free, because this function
   * inter alia close TCP connection on setted fd. Instead of this
   * function, we change fd to -1, which is the same what does
   * rest of mbedtls_net_free function.
   */
   csp->mbedtls_client_attr.socket_fd.fd = -1;

   /* Freeing mbedtls structures */
   mbedtls_x509_crt_free(&(csp->mbedtls_client_attr.server_cert));
   mbedtls_pk_free(&(csp->mbedtls_client_attr.prim_key));
   mbedtls_ssl_free(&(csp->mbedtls_client_attr.ssl));
   mbedtls_ssl_config_free(&(csp->mbedtls_client_attr.conf));
#if defined(MBEDTLS_SSL_CACHE_C)
   mbedtls_ssl_cache_free(&(csp->mbedtls_client_attr.cache));
#endif
}


/*********************************************************************
 *
 * Function    :  create_server_ssl_connection
 *
 * Description :  Creates TLS/SSL secured connection with server.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 on success, negative value if connection wasn't created
 *                successfully.
 *
 *********************************************************************/
extern int create_server_ssl_connection(struct client_state *csp)
{
   int ret = 0;
   char err_buf[ERROR_BUF_SIZE];
   char *trusted_cas_file = NULL;
   int auth_mode = MBEDTLS_SSL_VERIFY_REQUIRED;

   csp->server_cert_verification_result = SSL_CERT_NOT_VERIFIED;
   csp->server_certs_chain.next = NULL;

   /* Setting path to file with trusted CAs */
   trusted_cas_file = csp->config->trusted_cas_file;

   /*
    * Initializing mbedtls structures for TLS/SSL connection
    */
   mbedtls_net_init(&(csp->mbedtls_server_attr.socket_fd));
   mbedtls_ssl_init(&(csp->mbedtls_server_attr.ssl));
   mbedtls_ssl_config_init(&(csp->mbedtls_server_attr.conf));
   mbedtls_x509_crt_init(&(csp->mbedtls_server_attr.ca_cert));

   /*
   * Setting socket fd in mbedtls_net_context structure. This structure
   * can't be set by mbedtls functions, because we already have created
   * TCP connection when calling this function.
   */
   csp->mbedtls_server_attr.socket_fd.fd = csp->server_connection.sfd;

   /*
    * Seed the RNG
    */
   ret = seed_rng(csp);
   if (ret != 0)
   {
      ret = -1;
      goto exit;
   }

   /*
    * Loading file with trusted CAs
    */
   ret = mbedtls_x509_crt_parse_file(&(csp->mbedtls_server_attr.ca_cert),
      trusted_cas_file);
   if (ret < 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "Loading trusted CAs file %s failed: %s",
         trusted_cas_file, err_buf);
      ret = -1;
      goto exit;
   }

   /*
    * Set TLS/SSL options
    */
   ret = mbedtls_ssl_config_defaults(&(csp->mbedtls_server_attr.conf),
      MBEDTLS_SSL_IS_CLIENT,
      MBEDTLS_SSL_TRANSPORT_STREAM,
      MBEDTLS_SSL_PRESET_DEFAULT);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "mbedtls_ssl_config_defaults failed: %s",
         err_buf);
      ret = -1;
      goto exit;
   }

   /*
    * Setting how strict should certificate verification be and other
    * parameters for certificate verification
    */
   if (csp->dont_verify_certificate)
   {
      auth_mode = MBEDTLS_SSL_VERIFY_NONE;
   }

   mbedtls_ssl_conf_authmode(&(csp->mbedtls_server_attr.conf), auth_mode);
   mbedtls_ssl_conf_ca_chain(&(csp->mbedtls_server_attr.conf),
      &(csp->mbedtls_server_attr.ca_cert), NULL);

   /* Setting callback function for certificates verification */
   mbedtls_ssl_conf_verify(&(csp->mbedtls_server_attr.conf),
      ssl_verify_callback, (void *)csp);

   mbedtls_ssl_conf_rng(&(csp->mbedtls_server_attr.conf),
      mbedtls_ctr_drbg_random, &ctr_drbg);
   mbedtls_ssl_conf_dbg(&(csp->mbedtls_server_attr.conf),
      ssl_debug_callback, stdout);

   ret = mbedtls_ssl_setup(&(csp->mbedtls_server_attr.ssl),
      &(csp->mbedtls_server_attr.conf));
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "mbedtls_ssl_setup failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   /*
    * Set the hostname to check against the received server certificate
    */
   ret = mbedtls_ssl_set_hostname(&(csp->mbedtls_server_attr.ssl),
      csp->http->host);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "mbedtls_ssl_set_hostname failed: %s",
         err_buf);
      ret = -1;
      goto exit;
   }

   mbedtls_ssl_set_bio(&(csp->mbedtls_server_attr.ssl),
      &(csp->mbedtls_server_attr.socket_fd), mbedtls_net_send,
      mbedtls_net_recv, NULL);

   /*
    * Handshake with server
    */
   log_error(LOG_LEVEL_CONNECT,
      "Performing the TLS/SSL handshake with server");

   while ((ret = mbedtls_ssl_handshake(&(csp->mbedtls_server_attr.ssl))) != 0)
   {
      if (ret != MBEDTLS_ERR_SSL_WANT_READ
       && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
      {
         mbedtls_strerror(ret, err_buf, sizeof(err_buf));

         if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED)
         {
            log_error(LOG_LEVEL_ERROR,
               "Server certificate verification failed: %s", err_buf);
            csp->server_cert_verification_result =
               mbedtls_ssl_get_verify_result(&(csp->mbedtls_server_attr.ssl));

            ret = -1;
         }
         else
         {
            log_error(LOG_LEVEL_ERROR,
               "mbedtls_ssl_handshake with server failed: %s", err_buf);
            ret = -1;
         }
         goto exit;
      }
   }

   log_error(LOG_LEVEL_CONNECT, "Server successfully connected over TLS/SSL");

   /*
    * Server certificate chain is valid, so we can clean
    * chain, because we will not send it to client.
    */
   free_certificate_chain(csp);

   csp->ssl_with_server_is_opened = 1;
   csp->server_cert_verification_result =
      mbedtls_ssl_get_verify_result(&(csp->mbedtls_server_attr.ssl));

exit:
   /* Freeing structures if connection wasn't created successfully */
   if (ret < 0)
   {
      free_server_ssl_structures(csp);
   }

   return ret;
}


/*********************************************************************
 *
 * Function    :  close_server_ssl_connection
 *
 * Description :  Closes TLS/SSL connection with server. This function
 *                checks if this connection is already opened.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void close_server_ssl_connection(struct client_state *csp)
{
   int ret = 0;

   if (csp->ssl_with_server_is_opened == 0)
   {
      return;
   }

   /*
   * Notifying the peer that the connection is being closed.
   */
   do {
      ret = mbedtls_ssl_close_notify(&(csp->mbedtls_server_attr.ssl));
   } while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);

   free_server_ssl_structures(csp);
   csp->ssl_with_server_is_opened = 0;
}


/*********************************************************************
 *
 * Function    :  free_server_ssl_structures
 *
 * Description :  Frees structures used for SSL communication with server
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void free_server_ssl_structures(struct client_state *csp)
{
   /*
   * We can't use function mbedtls_net_free, because this function
   * inter alia close TCP connection on setted fd. Instead of this
   * function, we change fd to -1, which is the same what does
   * rest of mbedtls_net_free function.
   */
   csp->mbedtls_client_attr.socket_fd.fd = -1;

   mbedtls_x509_crt_free(&(csp->mbedtls_server_attr.ca_cert));
   mbedtls_ssl_free(&(csp->mbedtls_server_attr.ssl));
   mbedtls_ssl_config_free(&(csp->mbedtls_server_attr.conf));
}


/*********************************************************************
 *
 * Function    :  close_client_and_server_ssl_connections
 *
 * Description :  Checks if client or server should use secured
 *                connection over SSL and if so, closes all of them.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
extern void close_client_and_server_ssl_connections(struct client_state *csp)
{
   if (client_use_ssl(csp) == 1)
   {
      close_client_ssl_connection(csp);
   }
   if (server_use_ssl(csp) == 1)
   {
      close_server_ssl_connection(csp);
   }
}

/*====================== Certificates ======================*/

/*********************************************************************
 *
 * Function    :  write_certificate
 *
 * Description :  Writes certificate into file.
 *
 * Parameters  :
 *          1  :  crt = certificate to write into file
 *          2  :  output_file = path to save certificate file
 *          3  :  f_rng = mbedtls_ctr_drbg_random
 *          4  :  p_rng = mbedtls_ctr_drbg_context
 *
 * Returns     :  Length of written certificate on success or negative value
 *                on error
 *
 *********************************************************************/
static int write_certificate(mbedtls_x509write_cert *crt, const char *output_file,
   int(*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
   FILE *f = NULL;
   size_t len = 0;
   unsigned char cert_buf[CERTIFICATE_BUF_SIZE + 1]; /* Buffer for certificate in PEM format + terminating NULL */
   int ret = 0;
   char err_buf[ERROR_BUF_SIZE];

   memset(cert_buf, 0, sizeof(cert_buf));

   /*
    * Writing certificate into PEM string. If buffer is too small, function
    * returns specific error and no buffer overflow can happen.
    */
   if ((ret = mbedtls_x509write_crt_pem(crt, cert_buf,
      sizeof(cert_buf) - 1, f_rng, p_rng)) != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Writing certificate into buffer failed: %s", err_buf);
      return -1;
   }

   len = strlen((char *)cert_buf);

   /*
    * Saving certificate into file
    */
   if ((f = fopen(output_file, "w")) == NULL)
   {
      log_error(LOG_LEVEL_ERROR, "Opening file %s to save certificate failed",
         output_file);
      return -1;
   }

   if (fwrite(cert_buf, 1, len, f) != len)
   {
      log_error(LOG_LEVEL_ERROR,
         "Writing certificate into file %s failed", output_file);
      fclose(f);
      return -1;
   }

   fclose(f);

   return (int)len;
}


/*********************************************************************
 *
 * Function    :  write_private_key
 *
 * Description :  Writes private key into file and copies saved
 *                content into given pointer to string. If function
 *                returns 0 for success, this copy must be freed by
 *                caller.
 *
 * Parameters  :
 *          1  :  key = key to write into file
 *          2  :  ret_buf = pointer to string with created key file content
 *          3  :  key_file_path = path where to save key file
 *
 * Returns     :  Length of written private key on success or negative value
 *                on error
 *
 *********************************************************************/
static int write_private_key(mbedtls_pk_context *key, unsigned char **ret_buf,
   const char *key_file_path)
{
   size_t len = 0;                /* Length of created key    */
   FILE *f = NULL;                /* File to save certificate */
   int ret = 0;
   char err_buf[ERROR_BUF_SIZE];

   /* Initializing buffer for key file content */
   *ret_buf = zalloc_or_die(PRIVATE_KEY_BUF_SIZE + 1);

   /*
    * Writing private key into PEM string
    */
   if ((ret = mbedtls_pk_write_key_pem(key, *ret_buf, PRIVATE_KEY_BUF_SIZE)) != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Writing private key into PEM string failed: %s", err_buf);
      ret = -1;
      goto exit;
   }
   len = strlen((char *)*ret_buf);

   /*
    * Saving key into file
    */
   if ((f = fopen(key_file_path, "wb")) == NULL)
   {
      log_error(LOG_LEVEL_ERROR,
         "Opening file %s to save private key failed: %E",
         key_file_path);
      ret = -1;
      goto exit;
   }

   if (fwrite(*ret_buf, 1, len, f) != len)
   {
      fclose(f);
      log_error(LOG_LEVEL_ERROR,
         "Writing private key into file %s failed",
         key_file_path);
      ret = -1;
      goto exit;
   }

   fclose(f);

exit:
   if (ret < 0)
   {
      freez(*ret_buf);
      *ret_buf = NULL;
      return ret;
   }
   return (int)len;
}


/*********************************************************************
 *
 * Function    :  generate_key
 *
 * Description : Tests if private key for host saved in csp already
 *               exists.  If this file doesn't exists, a new key is
 *               generated and saved in a file. The generated key is also
 *               copied into given parameter key_buf, which must be then
 *               freed by caller. If file with key exists, key_buf
 *               contain NULL and no private key is generated.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  key_buf = buffer to save new generated key
 *
 * Returns     :  -1 => Error while generating private key
 *                 0 => Key already exists
 *                >0 => Length of generated private key
 *
 *********************************************************************/
static int generate_key(struct client_state *csp, unsigned char **key_buf)
{
   mbedtls_pk_context key;
   key_options key_opt;
   int ret = 0;
   char err_buf[ERROR_BUF_SIZE];

   key_opt.key_file_path = NULL;

   /*
    * Initializing structures for key generating
    */
   mbedtls_pk_init(&key);

   /*
    * Preparing path for key file and other properties for generating key
    */
   key_opt.type        = MBEDTLS_PK_RSA;
   key_opt.rsa_keysize = RSA_KEYSIZE;

   key_opt.key_file_path = make_certs_path(csp->config->certificate_directory,
      (char *)csp->http->hash_of_host_hex, KEY_FILE_TYPE);
   if (key_opt.key_file_path == NULL)
   {
      ret = -1;
      goto exit;
   }

   /*
    * Test if key already exists. If so, we don't have to create it again.
    */
   if (file_exists(key_opt.key_file_path) == 1)
   {
      ret = 0;
      goto exit;
   }

   /*
    * Seed the RNG
    */
   ret = seed_rng(csp);
   if (ret != 0)
   {
      ret = -1;
      goto exit;
   }

   /*
    * Setting attributes of private key and generating it
    */
   if ((ret = mbedtls_pk_setup(&key,
      mbedtls_pk_info_from_type(key_opt.type))) != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "mbedtls_pk_setup failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(key), mbedtls_ctr_drbg_random,
      &ctr_drbg, (unsigned)key_opt.rsa_keysize, RSA_KEY_PUBLIC_EXPONENT);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "Key generating failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   /*
    * Exporting private key into file
    */
   if ((ret = write_private_key(&key, key_buf, key_opt.key_file_path)) < 0)
   {
      log_error(LOG_LEVEL_ERROR,
         "Writing private key into file %s failed", key_opt.key_file_path);
      ret = -1;
      goto exit;
   }

exit:
   /*
    * Freeing used variables
    */
   freez(key_opt.key_file_path);

   mbedtls_pk_free(&key);

   return ret;
}


/*********************************************************************
 *
 * Function    :  generate_webpage_certificate
 *
 * Description :  Creates certificate file in presetted directory.
 *                If certificate already exists, no other certificate
 *                will be created. Subject of certificate is named
 *                by csp->http->host from parameter. This function also
 *                triggers generating of private key for new certificate.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  -1 => Error while creating certificate.
 *                 0 => Certificate already exists.
 *                >0 => Length of created certificate.
 *
 *********************************************************************/
static int generate_webpage_certificate(struct client_state *csp)
{
   mbedtls_x509_crt issuer_cert;
   mbedtls_pk_context loaded_issuer_key, loaded_subject_key;
   mbedtls_pk_context *issuer_key  = &loaded_issuer_key;
   mbedtls_pk_context *subject_key = &loaded_subject_key;
   mbedtls_x509write_cert cert;
   mbedtls_mpi serial;

   unsigned char *key_buf = NULL;    /* Buffer for created key */

   int ret = 0;
   char err_buf[ERROR_BUF_SIZE];
   cert_options cert_opt;

   /* Paths to keys and certificates needed to create certificate */
   cert_opt.issuer_key  = NULL;
   cert_opt.subject_key = NULL;
   cert_opt.issuer_crt  = NULL;
   cert_opt.output_file = NULL;

   /*
    * Create key for requested host
    */
   int subject_key_len = generate_key(csp, &key_buf);
   if (subject_key_len < 0)
   {
      log_error(LOG_LEVEL_ERROR, "Key generating failed");
      return -1;
   }

   /*
    * Initializing structures for certificate generating
    */
   mbedtls_x509write_crt_init(&cert);
   mbedtls_x509write_crt_set_md_alg(&cert, CERT_SIGNATURE_ALGORITHM);
   mbedtls_pk_init(&loaded_issuer_key);
   mbedtls_pk_init(&loaded_subject_key);
   mbedtls_mpi_init(&serial);
   mbedtls_x509_crt_init(&issuer_cert);

   /*
    * Presetting parameters for certificate. We must compute total length
    * of parameters.
    */
   size_t cert_params_len = strlen(CERT_PARAM_COMMON_NAME) +
      strlen(CERT_PARAM_ORGANIZATION) + strlen(CERT_PARAM_COUNTRY) +
      strlen(CERT_PARAM_ORG_UNIT) +
      3 * strlen(csp->http->host) + 1;
   char cert_params[cert_params_len];
   memset(cert_params, 0, cert_params_len);

   /*
    * Converting unsigned long serial number to char * serial number.
    * We must compute length of serial number in string + terminating null.
    */
   unsigned long certificate_serial = get_certificate_serial(csp);
   int serial_num_size = snprintf(NULL, 0, "%lu", certificate_serial) + 1;
   if (serial_num_size <= 0)
   {
      serial_num_size = 1;
   }

   char serial_num_text[serial_num_size];  /* Buffer for serial number */
   ret = snprintf(serial_num_text, (size_t)serial_num_size, "%lu", certificate_serial);
   if (ret < 0 || ret >= serial_num_size)
   {
      log_error(LOG_LEVEL_ERROR,
         "Converting certificate serial number into string failed");
      ret = -1;
      goto exit;
   }

   /*
    * Preparing parameters for certificate
    */
   strlcpy(cert_params, CERT_PARAM_COMMON_NAME,  cert_params_len);
   strlcat(cert_params, csp->http->host,         cert_params_len);
   strlcat(cert_params, CERT_PARAM_ORGANIZATION, cert_params_len);
   strlcat(cert_params, csp->http->host,         cert_params_len);
   strlcat(cert_params, CERT_PARAM_ORG_UNIT,     cert_params_len);
   strlcat(cert_params, csp->http->host,         cert_params_len);
   strlcat(cert_params, CERT_PARAM_COUNTRY,      cert_params_len);

   cert_opt.issuer_crt = csp->config->ca_cert_file;
   cert_opt.issuer_key = csp->config->ca_key_file;
   cert_opt.subject_key = make_certs_path(csp->config->certificate_directory,
      (const char *)csp->http->hash_of_host_hex, KEY_FILE_TYPE);
   cert_opt.output_file = make_certs_path(csp->config->certificate_directory,
      (const char *)csp->http->hash_of_host_hex, CERT_FILE_TYPE);

   if (cert_opt.subject_key == NULL || cert_opt.output_file == NULL)
   {
      ret = -1;
      goto exit;
   }

   cert_opt.subject_pwd   = CERT_SUBJECT_PASSWORD;
   cert_opt.issuer_pwd    = csp->config->ca_password;
   cert_opt.subject_name  = cert_params;
   cert_opt.not_before    = GENERATED_CERT_VALID_FROM;
   cert_opt.not_after     = GENERATED_CERT_VALID_TO;
   cert_opt.serial        = serial_num_text;
   cert_opt.is_ca         = 0;
   cert_opt.max_pathlen   = -1;

   /*
    * Test if certificate exists and private key was already created
    */
   if (file_exists(cert_opt.output_file) == 1 && subject_key_len == 0)
   {
      ret = 0;
      goto exit;
   }

   /*
    * Seed the PRNG
    */
   ret = seed_rng(csp);
   if (ret != 0)
   {
      ret = -1;
      goto exit;
   }

   /*
    * Parse serial to MPI
    */
   ret = mbedtls_mpi_read_string(&serial, 10, cert_opt.serial);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "mbedtls_mpi_read_string failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   /*
    * Loading certificates
    */
   ret = mbedtls_x509_crt_parse_file(&issuer_cert, cert_opt.issuer_crt);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "Loading issuer certificate %s failed: %s",
         cert_opt.issuer_crt, err_buf);
      ret = -1;
      goto exit;
   }

   ret = mbedtls_x509_dn_gets(cert_opt.issuer_name,
      sizeof(cert_opt.issuer_name), &issuer_cert.subject);
   if (ret < 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "mbedtls_x509_dn_gets failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   /*
    * Loading keys from file or from buffer
    */
   if (key_buf != NULL && subject_key_len > 0)
   {
      /* Key was created in this function and is stored in buffer */
      ret = mbedtls_pk_parse_key(&loaded_subject_key, key_buf,
         (size_t)(subject_key_len + 1), (unsigned const char *)
         cert_opt.subject_pwd, strlen(cert_opt.subject_pwd));
   }
   else
   {
      /* Key wasn't created in this function, because it already existed */
      ret = mbedtls_pk_parse_keyfile(&loaded_subject_key,
         cert_opt.subject_key, cert_opt.subject_pwd);
   }

   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "Parsing subject key %s failed: %s",
         cert_opt.subject_key, err_buf);
      ret = -1;
      goto exit;
   }

   ret = mbedtls_pk_parse_keyfile(&loaded_issuer_key, cert_opt.issuer_key,
      cert_opt.issuer_pwd);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Parsing issuer key %s failed: %s", cert_opt.issuer_key, err_buf);
      ret = -1;
      goto exit;
   }

   /*
    * Check if key and issuer certificate match
    */
   if (!mbedtls_pk_can_do(&issuer_cert.pk, MBEDTLS_PK_RSA) ||
      mbedtls_mpi_cmp_mpi(&mbedtls_pk_rsa(issuer_cert.pk)->N,
         &mbedtls_pk_rsa(*issuer_key)->N) != 0 ||
      mbedtls_mpi_cmp_mpi(&mbedtls_pk_rsa(issuer_cert.pk)->E,
         &mbedtls_pk_rsa(*issuer_key)->E) != 0)
   {
      log_error(LOG_LEVEL_ERROR,
         "Issuer key doesn't match issuer certificate");
      ret = -1;
      goto exit;
   }

   mbedtls_x509write_crt_set_subject_key(&cert, subject_key);
   mbedtls_x509write_crt_set_issuer_key(&cert, issuer_key);

   /*
    * Setting parameters of signed certificate
    */
   ret = mbedtls_x509write_crt_set_subject_name(&cert, cert_opt.subject_name);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Setting subject name in signed certificate failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   ret = mbedtls_x509write_crt_set_issuer_name(&cert, cert_opt.issuer_name);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Setting issuer name in signed certificate failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   ret = mbedtls_x509write_crt_set_serial(&cert, &serial);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Setting serial number in signed certificate failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   ret = mbedtls_x509write_crt_set_validity(&cert, cert_opt.not_before,
      cert_opt.not_after);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR,
         "Setting validity in signed certificate failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   /*
    * Setting the basicConstraints extension for certificate
    */
   ret = mbedtls_x509write_crt_set_basic_constraints(&cert, cert_opt.is_ca,
      cert_opt.max_pathlen);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "Setting the basicConstraints extension "
         "in signed certificate failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

#if defined(MBEDTLS_SHA1_C)
   /* Setting the subjectKeyIdentifier extension for certificate */
   ret = mbedtls_x509write_crt_set_subject_key_identifier(&cert);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "mbedtls_x509write_crt_set_subject_key_"
         "identifier failed: %s", err_buf);
      ret = -1;
      goto exit;
   }

   /* Setting the authorityKeyIdentifier extension for certificate */
   ret = mbedtls_x509write_crt_set_authority_key_identifier(&cert);
   if (ret != 0)
   {
      mbedtls_strerror(ret, err_buf, sizeof(err_buf));
      log_error(LOG_LEVEL_ERROR, "mbedtls_x509write_crt_set_authority_key_"
         "identifier failed: %s", err_buf);
      ret = -1;
      goto exit;
   }
#endif /* MBEDTLS_SHA1_C */

   /*
    * Writing certificate into file
    */
   ret = write_certificate(&cert, cert_opt.output_file,
      mbedtls_ctr_drbg_random, &ctr_drbg);
   if (ret < 0)
   {
      log_error(LOG_LEVEL_ERROR, "Writing certificate into file failed");
      goto exit;
   }

exit:
   /*
    * Freeing used structures
    */
   mbedtls_x509write_crt_free(&cert);
   mbedtls_pk_free(&loaded_subject_key);
   mbedtls_pk_free(&loaded_issuer_key);
   mbedtls_mpi_free(&serial);
   mbedtls_x509_crt_free(&issuer_cert);

   freez(cert_opt.subject_key);
   freez(cert_opt.output_file);
   freez(key_buf);

   return ret;
}


/*********************************************************************
 *
 * Function    :  make_certs_path
 *
 * Description : Creates path to file from three pieces. This fuction
 *               takes parameters and puts them in one new mallocated
 *               char * in correct order. Returned variable must be freed
 *               by caller. This function is mainly used for creating
 *               paths of certificates and keys files.
 *
 * Parameters  :
 *          1  :  conf_dir  = Name/path of directory where is the file.
 *                            '.' can be used for current directory.
 *          2  :  file_name = Name of file in conf_dir without suffix.
 *          3  :  suffix    = Suffix of given file_name.
 *
 * Returns     :  path => Path was built up successfully
 *                NULL => Path can't be built up
 *
 *********************************************************************/
static char *make_certs_path(const char *conf_dir, const char *file_name,
   const char *suffix)
{
   /* Test if all given parameters are valid */
   if (conf_dir == NULL || *conf_dir == '\0' || file_name == NULL ||
      *file_name == '\0' || suffix == NULL || *suffix == '\0')
   {
      log_error(LOG_LEVEL_ERROR,
         "make_certs_path failed: bad input parameters");
      return NULL;
   }

   char *path = NULL;
   size_t path_size = strlen(conf_dir)
      + strlen(file_name) + strlen(suffix) + 2;

   /* Setting delimiter and editing path length */
#if defined(_WIN32) || defined(__OS2__)
   char delim[] = "\\";
   path_size += 1;
#else /* ifndef _WIN32 || __OS2__ */
   char delim[] = "/";
#endif /* ifndef _WIN32 || __OS2__ */

   /*
    * Building up path from many parts
    */
#if defined(unix)
   if (*conf_dir != '/' && basedir && *basedir)
   {
      /*
       * Replacing conf_dir with basedir. This new variable contains
       * absolute path to cwd.
       */
      path_size += strlen(basedir) + 2;
      path = zalloc_or_die(path_size);

      strlcpy(path, basedir,   path_size);
      strlcat(path, delim,     path_size);
      strlcat(path, conf_dir,  path_size);
      strlcat(path, delim,     path_size);
      strlcat(path, file_name, path_size);
      strlcat(path, suffix,    path_size);
   }
   else
#endif /* defined unix */
   {
      path = zalloc_or_die(path_size);

      strlcpy(path, conf_dir,  path_size);
      strlcat(path, delim,     path_size);
      strlcat(path, file_name, path_size);
      strlcat(path, suffix,    path_size);
   }

   return path;
}


/*********************************************************************
 *
 * Function    :  get_certificate_mutex_id
 *
 * Description :  Computes mutex id from host name hash. This hash must
 *                be already saved in csp structure
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Mutex id for given host name
 *
 *********************************************************************/
static unsigned int get_certificate_mutex_id(struct client_state *csp) {
#ifdef LIMIT_MUTEX_NUMBER
   return (unsigned int)(csp->http->hash_of_host[0] % 32);
#else
   return (unsigned int)(csp->http->hash_of_host[1]
      + 256 * (int)csp->http->hash_of_host[0]);
#endif /* LIMIT_MUTEX_NUMBER */
}


/*********************************************************************
 *
 * Function    :  get_certificate_serial
 *
 * Description :  Computes serial number for new certificate from host
 *                name hash. This hash must be already saved in csp
 *                structure.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Serial number for new certificate
 *
 *********************************************************************/
static unsigned long  get_certificate_serial(struct client_state *csp) {
   unsigned long exp    = 1;
   unsigned long serial = 0;

   int i = CERT_SERIAL_NUM_LENGTH;
   /* Length of hash is 16 bytes, we must avoid to read next chars */
   if (i > 16)
   {
      i = 16;
   }
   if (i < 2)
   {
      i = 2;
   }

   for (; i >= 0; i--)
   {
      serial += exp * (unsigned)csp->http->hash_of_host[i];
      exp *= 256;
   }
   return serial;
}


/*********************************************************************
 *
 * Function    :  ssl_send_certificate_error
 *
 * Description :  Sends info about invalid server certificate to client.
 *                Sent message is including all trusted chain certificates,
 *                that can be downloaded in web browser.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
extern void ssl_send_certificate_error(struct client_state *csp)
{
   size_t message_len = 0;
   int ret = 0;
   struct certs_chain *cert = NULL;

   /* Header of message with certificate informations */
   const char message_begin[] =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Connection: close\r\n\r\n"
      "<html><body><h1>Invalid server certificate</h1><p>Reason: ";
   const char message_end[] = "</body></html>\r\n\r\n";
   char reason[INVALID_CERT_INFO_BUF_SIZE];
   memset(reason, 0, sizeof(reason));

   /* Get verification message from verification return code */
   mbedtls_x509_crt_verify_info(reason, sizeof(reason), " ",
      csp->server_cert_verification_result);

   /*
    * Computing total length of message with all certificates inside
    */
   message_len = strlen(message_begin) + strlen(message_end)
                 + strlen(reason) + strlen("</p>") + 1;

   cert = &(csp->server_certs_chain);
   while (cert->next != NULL)
   {
      size_t base64_len = 4 * ((strlen(cert->file_buf) + 2) / 3) + 1;

      message_len += strlen(cert->text_buf) + strlen("<pre></pre>\n")
                     +  base64_len + strlen("<a href=\"data:application"
                        "/x-x509-ca-cert;base64,\">Download certificate</a>");
      cert = cert->next;
   }

   /*
    * Joining all blocks in one long message
    */
   char message[message_len];
   memset(message, 0, message_len);

   strlcpy(message, message_begin, message_len);
   strlcat(message, reason       , message_len);
   strlcat(message, "</p>"       , message_len);

   cert = &(csp->server_certs_chain);
   while (cert->next != NULL)
   {
      size_t olen = 0;
      size_t base64_len = 4 * ((strlen(cert->file_buf) + 2) / 3) + 1; /* +1 for terminating null*/
      char base64_buf[base64_len];
      memset(base64_buf, 0, base64_len);

      /* Encoding certificate into base64 code */
      ret = mbedtls_base64_encode((unsigned char*)base64_buf,
               base64_len, &olen, (const unsigned char*)cert->file_buf,
               strlen(cert->file_buf));
      if (ret != 0)
      {
         log_error(LOG_LEVEL_ERROR,
            "Encoding to base64 failed, buffer is to small");
      }

      strlcat(message, "<pre>",        message_len);
      strlcat(message, cert->text_buf, message_len);
      strlcat(message, "</pre>\n",     message_len);

      if (ret == 0)
      {
         strlcat(message, "<a href=\"data:application/x-x509-ca-cert;base64,",
            message_len);
         strlcat(message, base64_buf, message_len);
         strlcat(message, "\">Download certificate</a>", message_len);
      }

      cert = cert->next;
   }
   strlcat(message, message_end, message_len);

   /*
    * Sending final message to client
    */
   ssl_send_data(&(csp->mbedtls_client_attr.ssl),
      (const unsigned char *)message, strlen(message));
   /*
    * Waiting before closing connection. Some browsers don't show received
    * message if there isn't this delay.
    */
   sleep(1);

   free_certificate_chain(csp);
}


/*********************************************************************
 *
 * Function    :  ssl_verify_callback
 *
 * Description :  This is a callback function for certificate verification.
 *                It's called for all certificates in server certificate
 *                trusted chain and it's preparing information about this
 *                certificates. Prepared informations can be used to inform
 *                user about invalid certificates.
 *
 * Parameters  :
 *          1  :  csp_void = Current client state (buffers, headers, etc...)
 *          2  :  crt   = certificate from trusted chain
 *          3  :  depth = depth in trusted chain
 *          4  :  flags = certificate flags
 *
 * Returns     :  0 on success and negative value on error
 *
 *********************************************************************/
static int ssl_verify_callback(void *csp_void, mbedtls_x509_crt *crt,
   int depth, uint32_t *flags)
{
   struct client_state *csp  = (struct client_state *)csp_void;
   struct certs_chain  *last = &(csp->server_certs_chain);
   size_t olen = 0;
   int ret = 0;

   /*
    * Searching for last item in certificates linked list
    */
   while (last->next != NULL)
   {
      last = last->next;
   }

   /*
    * Preparing next item in linked list for next certificate
    */
   last->next = malloc_or_die(sizeof(struct certs_chain));
   last->next->next = NULL;
   memset(last->next->text_buf, 0, sizeof(last->next->text_buf));
   memset(last->next->file_buf, 0, sizeof(last->next->file_buf));

   /*
    * Saving certificate file into buffer
    */
   if ((ret = mbedtls_pem_write_buffer(PEM_BEGIN_CRT, PEM_END_CRT,
      crt->raw.p, crt->raw.len, (unsigned char *)last->file_buf,
      sizeof(last->file_buf)-1, &olen)) != 0)
   {
      return(ret);
   }

   /*
    * Saving certificate information into buffer
    */
   mbedtls_x509_crt_info(last->text_buf, sizeof(last->text_buf) - 1,
      CERT_INFO_PREFIX, crt);

   return 0;
}


/*********************************************************************
 *
 * Function    :  free_certificate_chain
 *
 * Description :  Frees certificates linked list. This linked list is
 *                used to save informations about certificates in
 *                trusted chain.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void free_certificate_chain(struct client_state *csp)
{
   struct certs_chain *cert = csp->server_certs_chain.next;

   /* Cleaning buffers */
   memset(csp->server_certs_chain.text_buf, 0,
      sizeof(csp->server_certs_chain.text_buf));
   memset(csp->server_certs_chain.file_buf, 0,
      sizeof(csp->server_certs_chain.file_buf));
   csp->server_certs_chain.next = NULL;

   /* Freeing memory in whole linked list */
   if (cert != NULL)
   {
      do
      {
         struct certs_chain *cert_for_free = cert;
         cert = cert->next;
         freez(cert_for_free);
      } while (cert != NULL);
   }
}


/*********************************************************************
 *
 * Function    :  file_exists
 *
 * Description :  Tests if file exists and is readable.
 *
 * Parameters  :
 *          1  :  path = Path to tested file.
 *
 * Returns     :  1 => File exists and is readable.
 *                0 => File doesn't exist or is not readable.
 *
 *********************************************************************/
static int file_exists(const char *path)
{
   FILE *f;
   if ((f = fopen(path, "r")) != NULL)
   {
      fclose(f);
      return 1;
   }

   return 0;
}


/*********************************************************************
 *
 * Function    :  host_to_hash
 *
 * Description :  Creates MD5 hash from host name. Host name is loaded
 *                from structure csp and saved again into it.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  1 => Error while creating hash
 *                0 => Hash created successfully
 *
 *********************************************************************/
static int host_to_hash(struct client_state *csp)
{
   int ret = 0;

#if !defined(MBEDTLS_MD5_C)
   log_error(LOG_LEVEL_ERROR, "MBEDTLS_MD5_C is not defined. Can't create"
      "MD5 hash for certificate and key name.");
   return -1;
#else
   memset(csp->http->hash_of_host, 0, sizeof(csp->http->hash_of_host));
   mbedtls_md5((unsigned char *)csp->http->host, strlen(csp->http->host),
      csp->http->hash_of_host);

   /* Converting hash into string with hex */
   size_t i = 0;
   for (; i < 16; i++)
   {
      if ((ret = sprintf((char *)csp->http->hash_of_host_hex + 2 * i, "%02x",
         csp->http->hash_of_host[i])) < 0)
      {
         log_error(LOG_LEVEL_ERROR, "Sprintf return value: %d", ret);
         return -1;
      }
   }

   return 0;
#endif /* MBEDTLS_MD5_C */
}


/*********************************************************************
 *
 * Function    :  tunnel_established_successfully
 *
 * Description :  Check if parent proxy server response contains
 *                informations about successfully created connection with
 *                destination server. (HTTP/... 2xx ...)
 *
 * Parameters  :
 *          1  :  server_response = Buffer with parent proxy server response
 *          2  :  response_len = Length of server_response
 *
 * Returns     :  1 => Connection created successfully
 *                0 => Connection wasn't created successfully
 *
 *********************************************************************/
extern int tunnel_established_successfully(const char *server_response,
   unsigned int response_len)
{
   unsigned int pos = 0;

   if (server_response == NULL)
   {
      return 0;
   }

   /* Tests if "HTTP/" string is at the begin of received response */
   if (strncmp(server_response, "HTTP/", 5) != 0)
   {
      return 0;
   }

   for (pos = 0; pos < response_len; pos++)
   {
      if (server_response[pos] == ' ')
      {
         break;
      }
   }

   /*
    * response_len -3 because of buffer end, response structure and 200 code.
    * There must be at least 3 chars after space.
    * End of buffer: ... 2xx'\0'
    *             pos = |
    */
   if (pos >= (response_len - 3))
   {
      return 0;
   }

   /* Test HTTP status code */
   if (server_response[pos + 1] != '2')
   {
      return 0;
   }

   return 1;
}


/*********************************************************************
 *
 * Function    :  seed_rng
 *
 * Description :  Seeding the RNG for all SSL uses
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     : -1 => RNG wasn't seed successfully
 *                0 => RNG is seeded successfully
 *
 *********************************************************************/
static int seed_rng(struct client_state *csp)
{
   int ret = 0;
   char err_buf[ERROR_BUF_SIZE];

   if (rng_seeded == 0)
   {
      privoxy_mutex_lock(&rng_mutex);
      if (rng_seeded == 0)
      {
         mbedtls_ctr_drbg_init(&ctr_drbg);
         mbedtls_entropy_init(&entropy);
         ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
            &entropy, NULL, 0);
         if (ret != 0)
         {
            mbedtls_strerror(ret, err_buf, sizeof(err_buf));
            log_error(LOG_LEVEL_ERROR,
               "mbedtls_ctr_drbg_seed failed: %s", err_buf);
            privoxy_mutex_unlock(&rng_mutex);
            return -1;
         }
         rng_seeded = 1;
      }
      privoxy_mutex_unlock(&rng_mutex);
   }
   return 0;
}
