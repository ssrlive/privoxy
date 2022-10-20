/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/loadcfg.c,v $
 *
 * Purpose     :  Loads settings from the configuration file into
 *                global variables.  This file contains both the
 *                routine to load the configuration and the global
 *                variables it writes to.
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
#include <ctype.h>
#include <assert.h>

#ifdef _WIN32

# ifndef STRICT
#  define STRICT
# endif
# include <winsock2.h>
# include <windows.h>

# include "win32.h"
# ifndef _WIN_CONSOLE
#  include "w32log.h"
# endif /* ndef _WIN_CONSOLE */

#else /* ifndef _WIN32 */

# include <unistd.h>
# include <sys/wait.h>
# include <sys/time.h>
# include <sys/stat.h>
# include <signal.h>

#endif

#include "project.h"
#include "loadcfg.h"
#include "list.h"
#include "jcc.h"
#include "filters.h"
#include "loaders.h"
#include "miscutil.h"
#include "errlog.h"
#include "ssplit.h"
#include "encode.h"
#include "urlmatch.h"
#include "cgi.h"
#include "gateway.h"
#ifdef FEATURE_CLIENT_TAGS
#include "client-tags.h"
#endif

/*
 * Default number of seconds after which an
 * open connection will no longer be reused.
 */
#define DEFAULT_KEEP_ALIVE_TIMEOUT 180

/*
 * Default backlog passed to listen().
 */
#define DEFAULT_LISTEN_BACKLOG 128

#ifdef FEATURE_TOGGLE
/* Privoxy is enabled by default. */
int global_toggle_state = 1;
#endif /* def FEATURE_TOGGLE */

/* The filename of the configfile */
const char *configfile  = NULL;

/*
 * CGI functions will later need access to the invocation args,
 * so we will make argc and argv global.
 */
int Argc = 0;
char * const * Argv = NULL;

static struct file_list *current_configfile = NULL;


/*
 * This takes the "cryptic" hash of each keyword and aliases them to
 * something a little more readable.  This also makes changing the
 * hash values easier if they should change or the hash algorithm changes.
 * To find out the hash for a new directive put it in the config file
 * and read the number from the error message in the log).
 *
 * Please keep this list sorted alphabetically (but with the Windows
 * console and GUI specific options last).
 */

#define hash_actions_file                1196306641U /* "actionsfile" */
#define hash_accept_intercepted_requests 1513024973U /* "accept-intercepted-requests" */
#define hash_admin_address               4112573064U /* "admin-address" */
#define hash_allow_cgi_request_crunching  258915987U /* "allow-cgi-request-crunching" */
#define hash_buffer_limit                1881726070U /* "buffer-limit */
#define hash_ca_cert_file                1622923720U /* "ca-cert-file" */
#define hash_ca_directory                1623615670U /* "ca-directory" */
#define hash_ca_key_file                 1184187891U /* "ca-key-file" */
#define hash_ca_password                 1184543320U /* "ca-password" */
#define hash_certificate_directory       1367994217U /* "certificate-directory" */
#define hash_cipher_list                 1225729316U /* "cipher-list" */
#define hash_client_header_order         2701453514U /* "client-header-order" */
#define hash_client_specific_tag         3353703383U /* "client-specific-tag" */
#define hash_client_tag_lifetime         3239141416U /* "client-tag-lifetime" */
#define hash_compression_level           2464423563U /* "compression-level" */
#define hash_confdir                        1978389U /* "confdir" */
#define hash_connection_sharing          1348841265U /* "connection-sharing" */
#define hash_cors_allowed_origin         2769345637U /* "cors-allowed-origin" */
#define hash_debug                            78263U /* "debug" */
#define hash_default_server_timeout      2530089913U /* "default-server-timeout" */
#define hash_deny_access                 1227333715U /* "deny-access" */
#define hash_enable_accept_filter        2909040407U /* "enable-accept-filter" */
#define hash_enable_edit_actions         2517097536U /* "enable-edit-actions" */
#define hash_enable_compression          3943696946U /* "enable-compression" */
#define hash_enable_proxy_authentication_forwarding 4040610791U /* enable-proxy-authentication-forwarding */
#define hash_enable_remote_toggle        2979744683U /* "enable-remote-toggle" */
#define hash_enable_remote_http_toggle    110543988U /* "enable-remote-http-toggle" */
#define hash_enforce_blocks              1862427469U /* "enforce-blocks" */
#define hash_filterfile                   250887266U /* "filterfile" */
#define hash_forward                        2029845U /* "forward" */
#define hash_forward_socks4              3963965521U /* "forward-socks4" */
#define hash_forward_socks4a             2639958518U /* "forward-socks4a" */
#define hash_forward_socks5              3963965522U /* "forward-socks5" */
#define hash_forward_socks5t             2639958542U /* "forward-socks5t" */
#define hash_forwarded_connect_retries    101465292U /* "forwarded-connect-retries" */
#define hash_handle_as_empty_returns_ok  1444873247U /* "handle-as-empty-doc-returns-ok" */
#define hash_hostname                      10308071U /* "hostname" */
#define hash_keep_alive_timeout          3878599515U /* "keep-alive-timeout" */
#define hash_listen_address              1255650842U /* "listen-address" */
#define hash_listen_backlog              1255655735U /* "listen-backlog" */
#define hash_logdir                          422889U /* "logdir" */
#define hash_logfile                        2114766U /* "logfile" */
#define hash_max_client_connections      3595884446U /* "max-client-connections" */
#define hash_permit_access               3587953268U /* "permit-access" */
#define hash_proxy_info_url              3903079059U /* "proxy-info-url" */
#define hash_receive_buffer_size         2880297454U /* "receive-buffer-size */
#define hash_single_threaded             4250084780U /* "single-threaded" */
#define hash_socket_timeout              1809001761U /* "socket-timeout" */
#define hash_split_large_cgi_forms        671658948U /* "split-large-cgi-forms" */
#define hash_suppress_blocklists         1948693308U /* "suppress-blocklists" */
#define hash_templdir                      11067889U /* "templdir" */
#define hash_temporary_directory         1824125181U /* "temporary-directory" */
#define hash_tolerate_pipelining         1360286620U /* "tolerate-pipelining" */
#define hash_toggle                          447966U /* "toggle" */
#define hash_trust_info_url               430331967U /* "trust-info-url" */
#define hash_trust_x_forwarded_for       2971537414U /* "trust-x-forwarded-for" */
#define hash_trusted_cgi_referrer        4270883427U /* "trusted-cgi-referrer" */
#define hash_trusted_cas_file            2679803024U /* "trusted-cas-files" */
#define hash_trustfile                     56494766U /* "trustfile" */
#define hash_usermanual                  1416668518U /* "user-manual" */
#define hash_activity_animation          1817904738U /* "activity-animation" */
#define hash_close_button_minimizes      3651284693U /* "close-button-minimizes" */
#define hash_hide_console                2048809870U /* "hide-console" */
#define hash_log_buffer_size             2918070425U /* "log-buffer-size" */
#define hash_log_font_name               2866730124U /* "log-font-name" */
#define hash_log_font_size               2866731014U /* "log-font-size" */
#define hash_log_highlight_messages      4032101240U /* "log-highlight-messages" */
#define hash_log_max_lines               2868344173U /* "log-max-lines" */
#define hash_log_messages                2291744899U /* "log-messages" */
#define hash_show_on_task_bar             215410365U /* "show-on-task-bar" */


static void savearg(char *command, char *argument, struct configuration_spec * config);
#ifdef FEATURE_CLIENT_TAGS
static void free_client_specific_tags(struct client_tag_spec *tag_list);
#endif

/*********************************************************************
 *
 * Function    :  unload_configfile
 *
 * Description :  Free the config structure and all components.
 *
 * Parameters  :
 *          1  :  data: struct configuration_spec to unload
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void unload_configfile (void * data)
{
   struct configuration_spec * config = (struct configuration_spec *)data;
   struct forward_spec *cur_fwd = config->forward;
   int i;

#ifdef FEATURE_ACL
   struct access_control_list *cur_acl = config->acl;

   while (cur_acl != NULL)
   {
      struct access_control_list * next_acl = cur_acl->next;
      free(cur_acl);
      cur_acl = next_acl;
   }
   config->acl = NULL;
#endif /* def FEATURE_ACL */

   while (cur_fwd != NULL)
   {
      struct forward_spec * next_fwd = cur_fwd->next;

      unload_forward_spec(cur_fwd);

      cur_fwd = next_fwd;
   }
   config->forward = NULL;

   freez(config->confdir);
   freez(config->logdir);
   freez(config->templdir);
   freez(config->hostname);
#ifdef FEATURE_EXTERNAL_FILTERS
   freez(config->temporary_directory);
#endif

   for (i = 0; i < MAX_LISTENING_SOCKETS; i++)
   {
      freez(config->haddr[i]);
   }
   freez(config->logfile);

   for (i = 0; i < MAX_AF_FILES; i++)
   {
      freez(config->actions_file_short[i]);
      freez(config->actions_file[i]);
      freez(config->re_filterfile_short[i]);
      freez(config->re_filterfile[i]);
   }

   list_remove_all(config->ordered_client_headers);

   freez(config->admin_address);
   freez(config->cors_allowed_origin);
   freez(config->proxy_info_url);
   freez(config->proxy_args);
   freez(config->usermanual);
   freez(config->trusted_cgi_referrer);

#ifdef FEATURE_HTTPS_INSPECTION
   freez(config->ca_password);
   freez(config->ca_directory);
   freez(config->ca_cert_file);
   freez(config->ca_key_file);
   freez(config->certificate_directory);
   freez(config->cipher_list);
   freez(config->trusted_cas_file);
#endif

#ifdef FEATURE_TRUST
   freez(config->trustfile);
   list_remove_all(config->trust_info);
#endif /* def FEATURE_TRUST */

#ifdef FEATURE_CLIENT_TAGS
   free_client_specific_tags(config->client_tags);
#endif

   freez(config);
}


#ifdef FEATURE_GRACEFUL_TERMINATION
/*********************************************************************
 *
 * Function    :  unload_current_config_file
 *
 * Description :  Unloads current config file - reset to state at
 *                beginning of program.
 *
 * Parameters  :  None
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void unload_current_config_file(void)
{
   if (current_configfile)
   {
      current_configfile->unloader = unload_configfile;
      current_configfile = NULL;
   }
}
#endif


#ifdef FEATURE_CLIENT_TAGS
/*********************************************************************
 *
 * Function    :  register_tag
 *
 * Description :  Registers a client-specific-tag and its description
 *
 * Parameters  :
 *          1  :  config: The tag list
 *          2  :  name:  The name of the client-specific-tag
 *          3  :  description: The human-readable description for the tag
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void register_tag(struct client_tag_spec *tag_list,
   const char *name, const char *description)
{
   struct client_tag_spec *new_tag;
   struct client_tag_spec *last_tag;

   last_tag = tag_list;
   while (last_tag->next != NULL)
   {
      last_tag = last_tag->next;
   }
   if (last_tag->name == NULL)
   {
      /* First entry */
      new_tag = last_tag;
   }
   else
   {
      new_tag = zalloc_or_die(sizeof(struct client_tag_spec));
   }
   new_tag->name = strdup_or_die(name);
   new_tag->description = strdup_or_die(description);
   if (new_tag != last_tag)
   {
      last_tag->next = new_tag;
   }
}


/*********************************************************************
 *
 * Function    :  free_client_specific_tags
 *
 * Description :  Frees client-specific tags and their descriptions
 *
 * Parameters  :
 *          1  :  tag_list: The tag list to free
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void free_client_specific_tags(struct client_tag_spec *tag_list)
{
   struct client_tag_spec *this_tag;
   struct client_tag_spec *next_tag;

   next_tag = tag_list;
   do
   {
      this_tag = next_tag;
      next_tag = next_tag->next;

      freez(this_tag->name);
      freez(this_tag->description);

      if (this_tag != tag_list)
      {
         freez(this_tag);
      }
   } while (next_tag != NULL);
}
#endif /* def FEATURE_CLIENT_TAGS */


/*********************************************************************
 *
 * Function    :  parse_numeric_value
 *
 * Description :  Parse the value of a directive that can only have
 *                a single numeric value. Terminates with a fatal error
 *                if the value is NULL or not numeric.
 *
 * Parameters  :
 *          1  :  name:  The name of the directive. Used for log messages.
 *          2  :  value: The value to parse
 *
 *
 * Returns     :  The numerical value as integer
 *
 *********************************************************************/
static int parse_numeric_value(const char *name, const char *value)
{
   int number;
   char *endptr;

   assert(name != NULL);
   assert(value != NULL);

   if ((value == NULL) || (*value == '\0'))
   {
      log_error(LOG_LEVEL_FATAL, "Directive %s used without argument", name);
   }

   number = (int)strtol(value, &endptr, 0);
   if (*endptr != '\0')
   {
      log_error(LOG_LEVEL_FATAL,
         "Directive '%s' used with non-numerical value: '%s'", name, value);
   }

   return number;

}


/*********************************************************************
 *
 * Function    :  parse_toggle_value
 *
 * Description :  Parse the value of a directive that can only be
 *                enabled or disabled. Terminates with a fatal error
 *                if the value is NULL or something other than 0 or 1.
 *
 * Parameters  :
 *          1  :  name:  The name of the directive. Used for log messages.
 *          2  :  value: The value to parse
 *
 *
 * Returns     :  The numerical toggle state
 *
 *********************************************************************/
static int parse_toggle_state(const char *name, const char *value)
{
   int toggle_state;
   assert(name != NULL);
   assert(value != NULL);

   if ((value == NULL) || (*value == '\0'))
   {
      log_error(LOG_LEVEL_FATAL, "Directive %s used without argument", name);
   }

   toggle_state = atoi(value);

   /*
    * Also check the length as atoi() doesn't mind
    * garbage after a valid integer, but we do.
    */
   if (((toggle_state != 0) && (toggle_state != 1)) || (strlen(value) != 1))
   {
      log_error(LOG_LEVEL_FATAL,
         "Directive %s used with invalid argument '%s'. Use either '0' or '1'.",
         name, value);
   }

   return toggle_state;

}


/*********************************************************************
 *
 * Function    :  parse_client_header_order
 *
 * Description :  Parse the value of the header-order directive
 *
 * Parameters  :
 *          1  :  ordered_header_list:  List to insert the ordered
 *                                      headers into.
 *          2  :  ordered_headers:  The ordered header names separated
 *                                  by spaces or tabs.
 *
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void parse_client_header_order(struct list *ordered_header_list, const char *ordered_headers)
{
   char *original_headers_copy;
   char **vector;
   size_t max_segments;
   int number_of_headers;
   int i;

   assert(ordered_header_list != NULL);
   assert(ordered_headers != NULL);

   if (ordered_headers == NULL)
   {
      log_error(LOG_LEVEL_FATAL, "header-order used without argument");
   }

   /*
    * XXX: This estimate is guaranteed to be high enough as we
    *      let ssplit() ignore empty fields, but also a bit wasteful.
    *      The same hack is used in get_last_url() so it looks like
    *      a real solution is needed.
    */
   max_segments = strlen(ordered_headers) / 2;
   if (max_segments == 0)
   {
      max_segments = 1;
   }
   vector = malloc_or_die(max_segments * sizeof(char *));

   original_headers_copy = strdup_or_die(ordered_headers);

   number_of_headers = ssplit(original_headers_copy, "\t ", vector, max_segments);
   if (number_of_headers == -1)
   {
      log_error(LOG_LEVEL_FATAL, "Failed to split ordered headers");
   }

   for (i = 0; i < number_of_headers; i++)
   {
      if (JB_ERR_OK != enlist(ordered_header_list, vector[i]))
      {
         log_error(LOG_LEVEL_FATAL,
            "Failed to enlist ordered header: %s", vector[i]);
      }
   }

   freez(vector);
   freez(original_headers_copy);

   return;

}


/*********************************************************************
 *
 * Function    :  load_config
 *
 * Description :  Load the config file and all parameters.
 *
 *                XXX: more than thousand lines long
 *                and thus in serious need of refactoring.
 *
 * Parameters  :  None
 *
 * Returns     :  The configuration_spec, or NULL on error.
 *
 *********************************************************************/
struct configuration_spec * load_config(void)
{
   char *buf = NULL;
   char *p, *q;
   FILE *configfp = NULL;
   struct configuration_spec * config = NULL;
   struct client_state * fake_csp;
   struct file_list *fs;
   unsigned long linenum = 0;
   int i;
   char *logfile          = NULL;
#ifdef FEATURE_HTTPS_INSPECTION
   char *ca_cert_file     = NULL;
   char *ca_key_file      = NULL;
   char *ca_directory     = NULL;
   char *trusted_cas_file = NULL;
   char *certificate_directory = NULL;
#endif

   if (!check_file_changed(current_configfile, configfile, &fs))
   {
      /* No need to load */
      return ((struct configuration_spec *)current_configfile->f);
   }
   if (NULL == fs)
   {
      log_error(LOG_LEVEL_FATAL,
         "can't check configuration file '%s':  %E", configfile);
      return NULL;
   }

   if (NULL != current_configfile)
   {
      log_error(LOG_LEVEL_INFO, "Reloading configuration file '%s'", configfile);
   }

#ifdef FEATURE_TOGGLE
   global_toggle_state = 1;
#endif /* def FEATURE_TOGGLE */

   fs->f = config = zalloc_or_die(sizeof(*config));

   /*
    * This is backwards from how it's usually done.
    * Following the usual pattern, "fs" would be stored in a member
    * variable in "csp", and then we'd access "config" from "fs->f",
    * using a cast.  However, "config" is used so often that a
    * cast each time would be very ugly, and the extra indirection
    * would waste CPU cycles.  Therefore we store "config" in
    * "csp->config", and "fs" in "csp->config->config_file_list".
    */
   config->config_file_list = fs;

   /*
    * Set to defaults
    */
   config->multi_threaded            = 1;
   config->buffer_limit              = 4096 * 1024;
   config->receive_buffer_size       = BUFFER_SIZE;
   config->usermanual                = strdup_or_die(USER_MANUAL_URL);
   config->proxy_args                = strdup_or_die("");
   config->forwarded_connect_retries = 0;
#ifdef FEATURE_HTTPS_INSPECTION
   config->ca_password               = strdup_or_die("");
   ca_cert_file                      = strdup_or_die("cacert.crt");
   ca_key_file                       = strdup_or_die("cakey.pem");
   ca_directory                      = strdup_or_die("./CA");
   trusted_cas_file                  = strdup_or_die("trustedCAs.pem");
   certificate_directory             = strdup_or_die("./certs");
#endif

#ifdef FEATURE_CLIENT_TAGS
   config->client_tag_lifetime       = 60;
#endif
   config->trust_x_forwarded_for     = 0;
#if defined(FEATURE_ACCEPT_FILTER) && defined(SO_ACCEPTFILTER)
   config->enable_accept_filter      = 0;
#endif
   config->listen_backlog            = DEFAULT_LISTEN_BACKLOG;
   config->trusted_cgi_referrer      = NULL;
   /*
    * 128 client sockets ought to be enough for everybody who can't
    * be bothered to read the documentation to figure out how to
    * increase the limit.
    */
   config->max_client_connections    = 128;
   config->socket_timeout            = 300; /* XXX: Should be a macro. */
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   config->default_server_timeout    = 0;
   config->keep_alive_timeout        = DEFAULT_KEEP_ALIVE_TIMEOUT;
   config->feature_flags            &= ~RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE;
   config->feature_flags            &= ~RUNTIME_FEATURE_CONNECTION_SHARING;
#endif
   config->feature_flags            &= ~RUNTIME_FEATURE_CGI_TOGGLE;
   config->feature_flags            &= ~RUNTIME_FEATURE_SPLIT_LARGE_FORMS;
   config->feature_flags            &= ~RUNTIME_FEATURE_ACCEPT_INTERCEPTED_REQUESTS;
   config->feature_flags            &= ~RUNTIME_FEATURE_EMPTY_DOC_RETURNS_OK;
   config->feature_flags            &= ~RUNTIME_FEATURE_FORWARD_PROXY_AUTHENTICATION_HEADERS;
#ifdef FEATURE_COMPRESSION
   config->feature_flags            &= ~RUNTIME_FEATURE_COMPRESSION;
   /*
    * XXX: Run some benchmarks to see if there are better default values.
    */
   config->compression_level         = 1;
#endif
   config->feature_flags            &= ~RUNTIME_FEATURE_TOLERATE_PIPELINING;
   config->cors_allowed_origin       = NULL;

   configfp = fopen(configfile, "r");
   if (NULL == configfp)
   {
      log_error(LOG_LEVEL_FATAL,
         "can't open configuration file '%s':  %E", configfile);
      /* Never get here - LOG_LEVEL_FATAL causes program exit */
   }

   while (read_config_line(configfp, &linenum, &buf) != NULL)
   {
      char cmd[BUFFER_SIZE];
      char arg[BUFFER_SIZE];
      char tmp[BUFFER_SIZE];
#ifdef FEATURE_ACL
      struct access_control_list *cur_acl;
#endif /* def FEATURE_ACL */
      struct forward_spec *cur_fwd;
      int vec_count;
      char *vec[3];
      unsigned int directive_hash;

      strlcpy(tmp, buf, sizeof(tmp));

      /* Copy command (i.e. up to space or tab) into cmd */
      p = buf;
      q = cmd;
      while (*p && (*p != ' ') && (*p != '\t'))
      {
         *q++ = *p++;
      }
      *q = '\0';

      /* Skip over the whitespace in buf */
      while (*p && ((*p == ' ') || (*p == '\t')))
      {
         p++;
      }

      /* Copy the argument into arg */
      if (strlcpy(arg, p, sizeof(arg)) >= sizeof(arg))
      {
         log_error(LOG_LEVEL_FATAL, "Config line too long: %s", buf);
      }

      /* Should never happen, but check this anyway */
      if (*cmd == '\0')
      {
         freez(buf);
         continue;
      }

      /* Make sure the command field is lower case */
      for (p = cmd; *p; p++)
      {
         if (privoxy_isupper(*p))
         {
            *p = (char)privoxy_tolower(*p);
         }
      }

      directive_hash = hash_string(cmd);
      switch (directive_hash)
      {
/* *************************************************************************
 * actionsfile actions-file-name
 * In confdir by default
 * *************************************************************************/
         case hash_actions_file :
            i = 0;
            while ((i < MAX_AF_FILES) && (NULL != config->actions_file[i]))
            {
               i++;
            }

            if (i >= MAX_AF_FILES)
            {
               log_error(LOG_LEVEL_FATAL, "Too many 'actionsfile' directives in config file - limit is %d.\n"
                  "(You can increase this limit by changing MAX_AF_FILES in project.h and recompiling).",
                  MAX_AF_FILES);
            }
            config->actions_file_short[i] = strdup_or_die(arg);
            config->actions_file[i] = make_path(config->confdir, arg);

            break;
/* *************************************************************************
 * accept-intercepted-requests
 * *************************************************************************/
         case hash_accept_intercepted_requests:
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_ACCEPT_INTERCEPTED_REQUESTS;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_ACCEPT_INTERCEPTED_REQUESTS;
            }
            break;

/* *************************************************************************
 * admin-address email-address
 * *************************************************************************/
         case hash_admin_address :
            freez(config->admin_address);
            config->admin_address = strdup_or_die(arg);
            break;

/* *************************************************************************
 * allow-cgi-request-crunching
 * *************************************************************************/
         case hash_allow_cgi_request_crunching:
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_CGI_CRUNCHING;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_CGI_CRUNCHING;
            }
            break;

/* *************************************************************************
 * buffer-limit n
 * *************************************************************************/
         case hash_buffer_limit :
            config->buffer_limit = (size_t)(1024 * parse_numeric_value(cmd, arg));
            break;

/* *************************************************************************
 * client-header-order header-1 header-2 ... header-n
 * *************************************************************************/
         case hash_client_header_order:
            list_remove_all(config->ordered_client_headers);
            parse_client_header_order(config->ordered_client_headers, arg);
            break;

/* *************************************************************************
 * client-specific-tag tag-name description
 * *************************************************************************/
#ifdef FEATURE_CLIENT_TAGS
         case hash_client_specific_tag:
            {
               char *name;
               char *description;

               name = arg;
               description = strstr(arg, " ");
               if (description == NULL)
               {
                  log_error(LOG_LEVEL_FATAL,
                     "client-specific-tag '%s' lacks a description.", name);
               }
               *description = '\0';
               /*
                * The length is limited because we don't want truncated
                * HTML caused by the cgi interface using static buffer
                * sizes.
                */
               if (strlen(name) > CLIENT_TAG_LENGTH_MAX)
               {
                  log_error(LOG_LEVEL_FATAL,
                     "client-specific-tag '%s' is longer than %d characters.",
                     name, CLIENT_TAG_LENGTH_MAX);
               }
               description++;
               register_tag(config->client_tags, name, description);
            }
            break;
#endif /* def FEATURE_CLIENT_TAGS */

/* *************************************************************************
 * client-tag-lifetime ttl
 * *************************************************************************/
#ifdef FEATURE_CLIENT_TAGS
         case hash_client_tag_lifetime:
         {
            int ttl = parse_numeric_value(cmd, arg);
            if (0 <= ttl)
            {
               config->client_tag_lifetime = (unsigned)ttl;
            }
            else
            {
               log_error(LOG_LEVEL_FATAL,
                  "client-tag-lifetime can't be negative.");
            }
            break;
         }
#endif /* def FEATURE_CLIENT_TAGS */

/* *************************************************************************
 * confdir directory-name
 * *************************************************************************/
         case hash_confdir :
            freez(config->confdir);
            config->confdir = make_path(NULL, arg);
            break;

/* *************************************************************************
 * compression-level 0-9
 * *************************************************************************/
#ifdef FEATURE_COMPRESSION
         case hash_compression_level :
         {
            int compression_level = parse_numeric_value(cmd, arg);
            if (-1 <= compression_level && compression_level <= 9)
            {
               config->compression_level = compression_level;
            }
            else
            {
               log_error(LOG_LEVEL_FATAL,
                  "Invalid compression-level value: %s", arg);
            }
            break;
         }
#endif

/* *************************************************************************
 * connection-sharing (0|1)
 * *************************************************************************/
#ifdef FEATURE_CONNECTION_SHARING
         case hash_connection_sharing :
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_CONNECTION_SHARING;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_CONNECTION_SHARING;
            }
            break;
#endif

/* *************************************************************************
 * cors-allowed-origin http://www.example.org
 * *************************************************************************/
         case hash_cors_allowed_origin :
            /*
             * We don't validate the specified referrer as
             * it's only used for string comparison.
             */
            freez(config->cors_allowed_origin);
            config->cors_allowed_origin = strdup_or_die(arg);
            break;

/* *************************************************************************
 * debug n
 * Specifies debug level, multiple values are ORed together.
 * *************************************************************************/
         case hash_debug :
            config->debug |= parse_numeric_value(cmd, arg);
            break;

/* *************************************************************************
 * default-server-timeout timeout
 * *************************************************************************/
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
         case hash_default_server_timeout :
         {
            int timeout = parse_numeric_value(cmd, arg);
            if (0 <= timeout)
            {
               config->default_server_timeout = (unsigned int)timeout;
            }
            else
            {
               log_error(LOG_LEVEL_FATAL,
                  "Invalid default-server-timeout value: %s", arg);
            }
            break;
         }
#endif

/* *************************************************************************
 * deny-access source-ip[/significant-bits] [dest-ip[/significant-bits]]
 * *************************************************************************/
#ifdef FEATURE_ACL
         case hash_deny_access:
            strlcpy(tmp, arg, sizeof(tmp));
            vec_count = ssplit(tmp, " \t", vec, SZ(vec));

            if ((vec_count != 1) && (vec_count != 2))
            {
               log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for "
                     "deny-access directive in configuration file.");
               string_append(&config->proxy_args,
                  "<br>\nWARNING: Wrong number of parameters for "
                  "deny-access directive in configuration file.<br><br>\n");
               break;
            }

            /* allocate a new node */
            cur_acl = zalloc_or_die(sizeof(*cur_acl));
            cur_acl->action = ACL_DENY;

            if (acl_addr(vec[0], cur_acl->src) < 0)
            {
               log_error(LOG_LEVEL_ERROR, "Invalid source address, port or netmask "
                  "for deny-access directive in configuration file: \"%s\"", vec[0]);
               string_append(&config->proxy_args,
                  "<br>\nWARNING: Invalid source address, port or netmask "
                  "for deny-access directive in configuration file: \"");
               string_append(&config->proxy_args,
                  vec[0]);
               string_append(&config->proxy_args,
                  "\"<br><br>\n");
               freez(cur_acl);
               break;
            }
            if (vec_count == 2)
            {
               if (acl_addr(vec[1], cur_acl->dst) < 0)
               {
                  log_error(LOG_LEVEL_ERROR, "Invalid destination address, port or netmask "
                     "for deny-access directive in configuration file: \"%s\"", vec[1]);
                  string_append(&config->proxy_args,
                     "<br>\nWARNING: Invalid destination address, port or netmask "
                     "for deny-access directive in configuration file: \"");
                  string_append(&config->proxy_args,
                     vec[1]);
                  string_append(&config->proxy_args,
                     "\"<br><br>\n");
                  freez(cur_acl);
                  break;
               }
            }
#ifdef HAVE_RFC2553
            else
            {
               cur_acl->wildcard_dst = 1;
            }
#endif /* def HAVE_RFC2553 */

            /*
             * Add it to the list.  Note we reverse the list to get the
             * behaviour the user expects.  With both the ACL and
             * actions file, the last match wins.  However, the internal
             * implementations are different:  The actions file is stored
             * in the same order as the file, and scanned completely.
             * With the ACL, we reverse the order as we load it, then
             * when we scan it we stop as soon as we get a match.
             */
            cur_acl->next  = config->acl;
            config->acl = cur_acl;

            break;
#endif /* def FEATURE_ACL */

#if defined(FEATURE_ACCEPT_FILTER) && defined(SO_ACCEPTFILTER)
/* *************************************************************************
 * enable-accept-filter 0|1
 * *************************************************************************/
         case hash_enable_accept_filter :
            config->enable_accept_filter = parse_toggle_state(cmd, arg);
            break;
#endif /* defined(FEATURE_ACCEPT_FILTER) && defined(SO_ACCEPTFILTER) */

/* *************************************************************************
 * enable-edit-actions 0|1
 * *************************************************************************/
#ifdef FEATURE_CGI_EDIT_ACTIONS
         case hash_enable_edit_actions:
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_CGI_EDIT_ACTIONS;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_CGI_EDIT_ACTIONS;
            }
            break;
#endif /* def FEATURE_CGI_EDIT_ACTIONS */

/* *************************************************************************
 * enable-compression 0|1
 * *************************************************************************/
#ifdef FEATURE_COMPRESSION
         case hash_enable_compression:
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_COMPRESSION;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_COMPRESSION;
            }
            break;
#endif /* def FEATURE_COMPRESSION */

/* *************************************************************************
 * enable-proxy-authentication-forwarding 0|1
 * *************************************************************************/
         case hash_enable_proxy_authentication_forwarding:
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_FORWARD_PROXY_AUTHENTICATION_HEADERS;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_FORWARD_PROXY_AUTHENTICATION_HEADERS;
            }
            break;

/* *************************************************************************
 * enable-remote-toggle 0|1
 * *************************************************************************/
#ifdef FEATURE_TOGGLE
         case hash_enable_remote_toggle:
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_CGI_TOGGLE;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_CGI_TOGGLE;
            }
            break;
#endif /* def FEATURE_TOGGLE */

/* *************************************************************************
 * enable-remote-http-toggle 0|1
 * *************************************************************************/
         case hash_enable_remote_http_toggle:
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_HTTP_TOGGLE;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_HTTP_TOGGLE;
            }
            break;

/* *************************************************************************
 * enforce-blocks 0|1
 * *************************************************************************/
         case hash_enforce_blocks:
#ifdef FEATURE_FORCE_LOAD
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_ENFORCE_BLOCKS;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_ENFORCE_BLOCKS;
            }
#else
            log_error(LOG_LEVEL_ERROR, "Ignoring directive 'enforce-blocks'. "
               "FEATURE_FORCE_LOAD is disabled, blocks will always be enforced.");
#endif /* def FEATURE_FORCE_LOAD */
            break;

/* *************************************************************************
 * filterfile file-name
 * In confdir by default.
 * *************************************************************************/
         case hash_filterfile :
            i = 0;
            while ((i < MAX_AF_FILES) && (NULL != config->re_filterfile[i]))
            {
               i++;
            }

            if (i >= MAX_AF_FILES)
            {
               log_error(LOG_LEVEL_FATAL, "Too many 'filterfile' directives in config file - limit is %d.\n"
                  "(You can increase this limit by changing MAX_AF_FILES in project.h and recompiling).",
                  MAX_AF_FILES);
            }
            config->re_filterfile_short[i] = strdup_or_die(arg);
            config->re_filterfile[i] = make_path(config->confdir, arg);

            break;

/* *************************************************************************
 * forward url-pattern (.|http-proxy-host[:port])
 * *************************************************************************/
         case hash_forward:
            strlcpy(tmp, arg, sizeof(tmp));
            vec_count = ssplit(tmp, " \t", vec, SZ(vec));

            if (vec_count != 2)
            {
               log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for forward "
                     "directive in configuration file.");
               string_append(&config->proxy_args,
                  "<br>\nWARNING: Wrong number of parameters for "
                  "forward directive in configuration file.");
               break;
            }

            /* allocate a new node */
            cur_fwd = zalloc_or_die(sizeof(*cur_fwd));
            cur_fwd->type = SOCKS_NONE;

            /* Save the URL pattern */
            if (create_pattern_spec(cur_fwd->url, vec[0]))
            {
               log_error(LOG_LEVEL_ERROR, "Bad URL specifier for forward "
                     "directive in configuration file.");
               string_append(&config->proxy_args,
                  "<br>\nWARNING: Bad URL specifier for "
                  "forward directive in configuration file.");
               freez(cur_fwd);
               break;
            }

            /* Parse the parent HTTP proxy host:port */
            p = vec[1];

            if (strcmp(p, ".") != 0)
            {
               cur_fwd->forward_port = 8000;
               parse_forwarder_address(p,
                  &cur_fwd->forward_host, &cur_fwd->forward_port,
                  NULL, NULL);
            }

            /* Add to list. */
            cur_fwd->next = config->forward;
            config->forward = cur_fwd;

            break;

/* *************************************************************************
 * forward-socks4 url-pattern socks-proxy[:port] (.|http-proxy[:port])
 * *************************************************************************/
         case hash_forward_socks4:
            strlcpy(tmp, arg, sizeof(tmp));
            vec_count = ssplit(tmp, " \t", vec, SZ(vec));

            if (vec_count != 3)
            {
               log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for "
                     "forward-socks4 directive in configuration file.");
               string_append(&config->proxy_args,
                  "<br>\nWARNING: Wrong number of parameters for "
                  "forward-socks4 directive in configuration file.");
               break;
            }

            /* allocate a new node */
            cur_fwd = zalloc_or_die(sizeof(*cur_fwd));
            cur_fwd->type = SOCKS_4;

            /* Save the URL pattern */
            if (create_pattern_spec(cur_fwd->url, vec[0]))
            {
               log_error(LOG_LEVEL_ERROR, "Bad URL specifier for forward-socks4 "
                     "directive in configuration file.");
               string_append(&config->proxy_args,
                  "<br>\nWARNING: Bad URL specifier for "
                  "forward-socks4 directive in configuration file.");
               freez(cur_fwd);
               break;
            }

            /* Parse the SOCKS proxy host[:port] */
            p = vec[1];

            /* XXX: This check looks like a bug. */
            if (strcmp(p, ".") != 0)
            {
               cur_fwd->gateway_port = 1080;
               parse_forwarder_address(p,
                  &cur_fwd->gateway_host, &cur_fwd->gateway_port,
                  NULL, NULL);
            }

            /* Parse the parent HTTP proxy host[:port] */
            p = vec[2];

            if (strcmp(p, ".") != 0)
            {
               cur_fwd->forward_port = 8000;
               parse_forwarder_address(p,
                  &cur_fwd->forward_host, &cur_fwd->forward_port,
                  NULL, NULL);
            }

            /* Add to list. */
            cur_fwd->next = config->forward;
            config->forward = cur_fwd;

            break;

/* *************************************************************************
 * forward-socks4a url-pattern socks-proxy[:port] (.|http-proxy[:port])
 * *************************************************************************/
         case hash_forward_socks4a:
         case hash_forward_socks5:
         case hash_forward_socks5t:
            strlcpy(tmp, arg, sizeof(tmp));
            vec_count = ssplit(tmp, " \t", vec, SZ(vec));

            if (vec_count != 3)
            {
               log_error(LOG_LEVEL_ERROR,
                  "Wrong number of parameters for %s in configuration file.",
                  cmd);
               string_append(&config->proxy_args,
                  "<br>\nWARNING: Wrong number of parameters for ");
               string_append(&config->proxy_args, cmd);
               string_append(&config->proxy_args,
                  "directive in configuration file.");
               break;
            }

            /* allocate a new node */
            cur_fwd = zalloc_or_die(sizeof(*cur_fwd));

            if (directive_hash == hash_forward_socks4a)
            {
               cur_fwd->type = SOCKS_4A;
            }
            else if (directive_hash == hash_forward_socks5)
            {
               cur_fwd->type = SOCKS_5;
            }
            else
            {
               assert(directive_hash == hash_forward_socks5t);
               cur_fwd->type = SOCKS_5T;
            }

            /* Save the URL pattern */
            if (create_pattern_spec(cur_fwd->url, vec[0]))
            {
               log_error(LOG_LEVEL_ERROR,
                  "Bad URL specifier for %s in configuration file.",
                  cmd);
               string_append(&config->proxy_args,
                  "<br>\nWARNING: Bad URL specifier for ");
               string_append(&config->proxy_args, cmd);
               string_append(&config->proxy_args,
                  "directive in configuration file.");
               freez(cur_fwd);
               break;
            }

            /* Parse the SOCKS proxy [user:pass@]host[:port] */
            p = vec[1];

            cur_fwd->gateway_port = 1080;
            parse_forwarder_address(p,
               &cur_fwd->gateway_host, &cur_fwd->gateway_port,
               &cur_fwd->auth_username, &cur_fwd->auth_password);

            /* Parse the parent HTTP proxy host[:port] */
            p = vec[2];

            if (strcmp(p, ".") != 0)
            {
               cur_fwd->forward_port = 8000;
               parse_forwarder_address(p,
                  &cur_fwd->forward_host, &cur_fwd->forward_port,
                  NULL, NULL);
            }

            /* Add to list. */
            cur_fwd->next = config->forward;
            config->forward = cur_fwd;

            break;

/* *************************************************************************
 * forwarded-connect-retries n
 * *************************************************************************/
         case hash_forwarded_connect_retries :
            config->forwarded_connect_retries = parse_numeric_value(cmd, arg);
            break;

/* *************************************************************************
 * handle-as-empty-doc-returns-ok 0|1
 *
 * Workaround for firefox hanging on blocked javascript pages.
 *   Block with the "+handle-as-empty-document" flag and set the
 *   "handle-as-empty-doc-returns-ok" run-time config flag so that
 *   Privoxy returns a 200/OK status instead of a 403/Forbidden status
 *   to the browser for blocked pages.
 ***************************************************************************/
         case hash_handle_as_empty_returns_ok:
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_EMPTY_DOC_RETURNS_OK;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_EMPTY_DOC_RETURNS_OK;
            }
            break;

/* *************************************************************************
 * hostname hostname-to-show-on-cgi-pages
 * *************************************************************************/
         case hash_hostname :
            freez(config->hostname);
            config->hostname = strdup_or_die(arg);
            break;

/* *************************************************************************
 * keep-alive-timeout timeout
 * *************************************************************************/
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
         case hash_keep_alive_timeout :
         {
            int timeout = parse_numeric_value(cmd, arg);
            if (0 < timeout)
            {
               config->feature_flags |= RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE;
               config->keep_alive_timeout = (unsigned int)timeout;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE;
            }
            break;
         }
#endif

/* *************************************************************************
 * listen-address [ip][:port]
 * *************************************************************************/
         case hash_listen_address :
            i = 0;
            while ((i < MAX_LISTENING_SOCKETS) && (NULL != config->haddr[i]))
            {
               i++;
            }

            if (i >= MAX_LISTENING_SOCKETS)
            {
               log_error(LOG_LEVEL_FATAL, "Too many 'listen-address' directives in config file - limit is %d.\n"
                  "(You can increase this limit by changing MAX_LISTENING_SOCKETS in project.h and recompiling).",
                  MAX_LISTENING_SOCKETS);
            }
            config->haddr[i] = strdup_or_die(arg);
            break;

/* *************************************************************************
 * listen-backlog n
 * *************************************************************************/
         case hash_listen_backlog :
            /*
             * We don't enfore an upper or lower limit because on
             * many platforms all values are valid and negative
             * number mean "use the highest value allowed".
             */
            config->listen_backlog = parse_numeric_value(cmd, arg);
            break;

/* *************************************************************************
 * logdir directory-name
 * *************************************************************************/
         case hash_logdir :
            freez(config->logdir);
            config->logdir = make_path(NULL, arg);
            break;

/* *************************************************************************
 * logfile log-file-name
 * In logdir by default
 * *************************************************************************/
         case hash_logfile :
            if (daemon_mode)
            {
               logfile = make_path(config->logdir, arg);
               if (NULL == logfile)
               {
                  log_error(LOG_LEVEL_FATAL, "Out of memory while creating logfile path");
               }
            }
            break;

/* *************************************************************************
 * max-client-connections number
 * *************************************************************************/
         case hash_max_client_connections :
         {
            int max_client_connections = parse_numeric_value(cmd, arg);

#if !defined(_WIN32) && !defined(HAVE_POLL)
            /*
             * Reject values below 1 for obvious reasons and values above
             * FD_SETSIZE/2 because Privoxy needs two sockets to serve
             * client connections that need forwarding.
             *
             * We ignore the fact that the first three file descriptors
             * are usually set to /dev/null, one is used for logging
             * and yet another file descriptor is required to load
             * config files.
             */
            if ((max_client_connections < 1) || (FD_SETSIZE/2 < max_client_connections))
            {
               log_error(LOG_LEVEL_FATAL, "max-client-connections value %d"
                  " is invalid. Value needs to be above 1 and below %d"
                  " (FD_SETSIZE/2).", max_client_connections, FD_SETSIZE/2);
            }
#else
            /*
             * The Windows libc uses FD_SETSIZE for an array used
             * by select(), but has no problems with file descriptors
             * above the limit as long as no more than FD_SETSIZE are
             * passed to select().
             * https://msdn.microsoft.com/en-us/library/windows/desktop/ms739169%28v=vs.85%29.aspx
             *
             * On platforms were we use poll() we don't have to enforce
             * an upper connection limit either.
             */
            if (max_client_connections < 1)
            {
               log_error(LOG_LEVEL_FATAL, "max-client-connections value"
                  " has to be a number above 1. %d is invalid.",
                  max_client_connections);
            }
#endif
            config->max_client_connections = max_client_connections;
            break;
         }

/* *************************************************************************
 * permit-access source-ip[/significant-bits] [dest-ip[/significant-bits]]
 * *************************************************************************/
#ifdef FEATURE_ACL
         case hash_permit_access:
            strlcpy(tmp, arg, sizeof(tmp));
            vec_count = ssplit(tmp, " \t", vec, SZ(vec));

            if ((vec_count != 1) && (vec_count != 2))
            {
               log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for "
                     "permit-access directive in configuration file.");
               string_append(&config->proxy_args,
                  "<br>\nWARNING: Wrong number of parameters for "
                  "permit-access directive in configuration file.<br><br>\n");

               break;
            }

            /* allocate a new node */
            cur_acl = zalloc_or_die(sizeof(*cur_acl));
            cur_acl->action = ACL_PERMIT;

            if (acl_addr(vec[0], cur_acl->src) < 0)
            {
               log_error(LOG_LEVEL_ERROR, "Invalid source address, port or netmask "
                  "for permit-access directive in configuration file: \"%s\"", vec[0]);
               string_append(&config->proxy_args,
                  "<br>\nWARNING: Invalid source address, port or netmask for "
                  "permit-access directive in configuration file: \"");
               string_append(&config->proxy_args,
                  vec[0]);
               string_append(&config->proxy_args,
                  "\"<br><br>\n");
               freez(cur_acl);
               break;
            }
            if (vec_count == 2)
            {
               if (acl_addr(vec[1], cur_acl->dst) < 0)
               {
                  log_error(LOG_LEVEL_ERROR, "Invalid destination address, port or netmask "
                     "for permit-access directive in configuration file: \"%s\"", vec[1]);
                  string_append(&config->proxy_args,
                     "<br>\nWARNING: Invalid destination address, port or netmask for "
                     "permit-access directive in configuration file: \"");
                  string_append(&config->proxy_args,
                     vec[1]);
                  string_append(&config->proxy_args,
                     "\"<br><br>\n");
                  freez(cur_acl);
                  break;
               }
            }
#ifdef HAVE_RFC2553
            else
            {
               cur_acl->wildcard_dst = 1;
            }
#endif /* def HAVE_RFC2553 */

            /*
             * Add it to the list.  Note we reverse the list to get the
             * behaviour the user expects.  With both the ACL and
             * actions file, the last match wins.  However, the internal
             * implementations are different:  The actions file is stored
             * in the same order as the file, and scanned completely.
             * With the ACL, we reverse the order as we load it, then
             * when we scan it we stop as soon as we get a match.
             */
            cur_acl->next  = config->acl;
            config->acl = cur_acl;

            break;
#endif /* def FEATURE_ACL */

/* *************************************************************************
 * proxy-info-url url
 * *************************************************************************/
         case hash_proxy_info_url :
            freez(config->proxy_info_url);
            config->proxy_info_url = strdup_or_die(arg);
            break;


/* *************************************************************************
 * receive-buffer-size n
 * *************************************************************************/
         case hash_receive_buffer_size :
            config->receive_buffer_size = (size_t)parse_numeric_value(cmd, arg);
            if (config->receive_buffer_size < BUFFER_SIZE)
            {
               log_error(LOG_LEVEL_INFO,
                  "receive-buffer-size %lu seems low and may cause problems. "
                  "Consider setting it to at least %d.",
                  config->receive_buffer_size, BUFFER_SIZE);
            }
            break;

/* *************************************************************************
 * single-threaded 0|1
 * *************************************************************************/
         case hash_single_threaded :
            config->multi_threaded = 0 == parse_toggle_state(cmd, arg);
            break;

/* *************************************************************************
 * socket-timeout numer_of_seconds
 * *************************************************************************/
         case hash_socket_timeout :
         {
            int socket_timeout = parse_numeric_value(cmd, arg);
            if (0 <= socket_timeout)
            {
               config->socket_timeout = socket_timeout;
            }
            else
            {
               log_error(LOG_LEVEL_FATAL, "Invalid socket-timeout: '%s'", arg);
            }
            break;
         }

/* *************************************************************************
 * split-large-cgi-forms
 * *************************************************************************/
         case hash_split_large_cgi_forms :
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_SPLIT_LARGE_FORMS;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_SPLIT_LARGE_FORMS;
            }
            break;

/* *************************************************************************
 * templdir directory-name
 * *************************************************************************/
         case hash_templdir :
            freez(config->templdir);
            config->templdir = make_path(NULL, arg);
            break;

#ifdef FEATURE_EXTERNAL_FILTERS
/* *************************************************************************
 * temporary-directory directory-name
 * *************************************************************************/
         case hash_temporary_directory :
            freez(config->temporary_directory);
            config->temporary_directory = make_path(NULL, arg);
            break;
#endif

/* *************************************************************************
 * tolerate-pipelining (0|1)
 * *************************************************************************/
         case hash_tolerate_pipelining :
            if (parse_toggle_state(cmd, arg) == 1)
            {
               config->feature_flags |= RUNTIME_FEATURE_TOLERATE_PIPELINING;
            }
            else
            {
               config->feature_flags &= ~RUNTIME_FEATURE_TOLERATE_PIPELINING;
            }
            break;

/* *************************************************************************
 * toggle (0|1)
 * *************************************************************************/
#ifdef FEATURE_TOGGLE
         case hash_toggle :
            global_toggle_state = parse_toggle_state(cmd, arg);
            break;
#endif /* def FEATURE_TOGGLE */

/* *************************************************************************
 * trust-info-url url
 * *************************************************************************/
#ifdef FEATURE_TRUST
         case hash_trust_info_url :
            enlist(config->trust_info, arg);
            break;
#endif /* def FEATURE_TRUST */

/* *************************************************************************
 * trust-x-forwarded-for (0|1)
 * *************************************************************************/
         case hash_trust_x_forwarded_for :
            config->trust_x_forwarded_for = parse_toggle_state(cmd, arg);
            break;

/* *************************************************************************
 * trusted-cgi-referrer http://www.example.org/some/path.html
 * *************************************************************************/
         case hash_trusted_cgi_referrer :
            /*
             * We don't validate the specified referrer as
             * it's only used for string comparison.
             */
            freez(config->trusted_cgi_referrer);
            config->trusted_cgi_referrer = strdup_or_die(arg);
            break;

/* *************************************************************************
 * trustfile filename
 * (In confdir by default.)
 * *************************************************************************/
#ifdef FEATURE_TRUST
         case hash_trustfile :
            freez(config->trustfile);
            config->trustfile = make_path(config->confdir, arg);
            break;
#endif /* def FEATURE_TRUST */

/* *************************************************************************
 * usermanual url
 * *************************************************************************/
         case hash_usermanual :
            /*
             * XXX: If this isn't the first config directive, the
             * show-status page links to the website documentation
             * for the directives that were already parsed. Lame.
             */
            freez(config->usermanual);
            config->usermanual = strdup_or_die(arg);
            break;

#ifdef FEATURE_HTTPS_INSPECTION
/* *************************************************************************
 * ca private key file password
 * *************************************************************************/
         case hash_ca_password:
            freez(config->ca_password);
            config->ca_password = strdup(arg);
            break;

/* *************************************************************************
 * ca-directory directory
 * *************************************************************************/
         case hash_ca_directory:
            freez(ca_directory);
            ca_directory = make_path(NULL, arg);

            if (NULL == ca_directory)
            {
               log_error(LOG_LEVEL_FATAL, "Out of memory while creating ca dir path");
            }

            break;

/* *************************************************************************
 * ca cert file ca-cert-file
 * In ca dir by default
 * *************************************************************************/
         case hash_ca_cert_file:
            freez(ca_cert_file);
            ca_cert_file = make_path(config->ca_directory, arg);

            if (NULL == ca_cert_file)
            {
               log_error(LOG_LEVEL_FATAL, "Out of memory while creating ca certificate file path");
            }

            break;

/* *************************************************************************
 * ca key file ca-key-file
 * In ca dir by default
 * *************************************************************************/
         case hash_ca_key_file:
            freez(ca_key_file);
            ca_key_file = make_path(config->ca_directory, arg);

            if (NULL == ca_key_file)
            {
               log_error(LOG_LEVEL_FATAL, "Out of memory while creating ca key file path");
            }

            break;

/* *************************************************************************
 * certificate-directory directory
 * *************************************************************************/
         case hash_certificate_directory:
            freez(certificate_directory);
            certificate_directory = make_path(NULL, arg);

            if (NULL == certificate_directory)
            {
               log_error(LOG_LEVEL_FATAL,
                  "Out of memory while creating certificate directory path");
            }

            break;

/* *************************************************************************
 * cipher-list list-of-ciphers
 * *************************************************************************/
         case hash_cipher_list:
            freez(config->cipher_list);
            config->cipher_list = strdup_or_die(arg);

            break;

/* *************************************************************************
 * trusted CAs file name trusted-cas-file
 * *************************************************************************/
         case hash_trusted_cas_file:
            freez(trusted_cas_file);
            trusted_cas_file = make_path(config->ca_directory, arg);

            if (NULL == trusted_cas_file)
            {
               log_error(LOG_LEVEL_FATAL, "Out of memory while creating trusted CAs file path");
            }

            break;
#endif

/* *************************************************************************
 * Win32 Console options:
 * *************************************************************************/

/* *************************************************************************
 * hide-console
 * *************************************************************************/
#ifdef _WIN_CONSOLE
         case hash_hide_console :
            hideConsole = 1;
            break;
#endif /*def _WIN_CONSOLE*/


/* *************************************************************************
 * Win32 GUI options:
 * *************************************************************************/

#if defined(_WIN32) && ! defined(_WIN_CONSOLE)
/* *************************************************************************
 * activity-animation (0|1)
 * *************************************************************************/
         case hash_activity_animation :
            g_bShowActivityAnimation = parse_toggle_state(cmd, arg);
            break;

/* *************************************************************************
 *  close-button-minimizes (0|1)
 * *************************************************************************/
         case hash_close_button_minimizes :
            g_bCloseHidesWindow = parse_toggle_state(cmd, arg);
            break;

/* *************************************************************************
 * log-buffer-size (0|1)
 * *************************************************************************/
         case hash_log_buffer_size :
            g_bLimitBufferSize = parse_toggle_state(cmd, arg);
            break;

/* *************************************************************************
 * log-font-name fontname
 * *************************************************************************/
         case hash_log_font_name :
            if (strlcpy(g_szFontFaceName, arg,
                   sizeof(g_szFontFaceName)) >= sizeof(g_szFontFaceName))
            {
               log_error(LOG_LEVEL_FATAL,
                  "log-font-name argument '%s' is longer than %u characters.",
                  arg, sizeof(g_szFontFaceName)-1);
            }
            break;

/* *************************************************************************
 * log-font-size n
 * *************************************************************************/
         case hash_log_font_size :
            g_nFontSize = parse_numeric_value(cmd, arg);
            break;

/* *************************************************************************
 * log-highlight-messages (0|1)
 * *************************************************************************/
         case hash_log_highlight_messages :
            g_bHighlightMessages = parse_toggle_state(cmd, arg);
            break;

/* *************************************************************************
 * log-max-lines n
 * *************************************************************************/
         case hash_log_max_lines :
            g_nMaxBufferLines = parse_numeric_value(cmd, arg);
            break;

/* *************************************************************************
 * log-messages (0|1)
 * *************************************************************************/
         case hash_log_messages :
            g_bLogMessages = parse_toggle_state(cmd, arg);
            break;

/* *************************************************************************
 * show-on-task-bar (0|1)
 * *************************************************************************/
         case hash_show_on_task_bar :
            g_bShowOnTaskBar = parse_toggle_state(cmd, arg);
            break;

#endif /* defined(_WIN32) && ! defined(_WIN_CONSOLE) */


/* *************************************************************************
 * Warnings about unsupported features
 * *************************************************************************/
#ifndef FEATURE_ACL
         case hash_deny_access:
#endif /* ndef FEATURE_ACL */
#ifndef FEATURE_CGI_EDIT_ACTIONS
         case hash_enable_edit_actions:
#endif /* ndef FEATURE_CGI_EDIT_ACTIONS */
#ifndef FEATURE_TOGGLE
         case hash_enable_remote_toggle:
#endif /* ndef FEATURE_TOGGLE */
#ifndef FEATURE_ACL
         case hash_permit_access:
#endif /* ndef FEATURE_ACL */
#ifndef FEATURE_TOGGLE
         case hash_toggle :
#endif /* ndef FEATURE_TOGGLE */
#ifndef FEATURE_TRUST
         case hash_trustfile :
         case hash_trust_info_url :
#endif /* ndef FEATURE_TRUST */

#ifndef _WIN_CONSOLE
         case hash_hide_console :
#endif /* ndef _WIN_CONSOLE */

#if defined(_WIN_CONSOLE) || ! defined(_WIN32)
         case hash_activity_animation :
         case hash_close_button_minimizes :
         case hash_log_buffer_size :
         case hash_log_font_name :
         case hash_log_font_size :
         case hash_log_highlight_messages :
         case hash_log_max_lines :
         case hash_log_messages :
         case hash_show_on_task_bar :
#endif /* defined(_WIN_CONSOLE) || ! defined(_WIN32) */
            /* These warnings are annoying - so hide them. -- Jon */
            /* log_error(LOG_LEVEL_INFO, "Unsupported directive \"%s\" ignored.", cmd); */
            break;

/* *************************************************************************/
         default :
/* *************************************************************************/
            /*
             * I decided that I liked this better as a warning than an
             * error.  To change back to an error, just change log level
             * to LOG_LEVEL_FATAL.
             */
            log_error(LOG_LEVEL_ERROR, "Ignoring unrecognized directive "
               "'%s' (%uU) in line %lu in configuration file (%s).",
               buf, directive_hash, linenum, configfile);
            string_append(&config->proxy_args,
               " <strong class='warning'>Warning: Ignoring unrecognized directive:</strong>");
            break;

/* *************************************************************************/
      } /* end switch(hash_string(cmd)) */

      /* Save the argument for the show-status page. */
      savearg(cmd, arg, config);
      freez(buf);
   } /* end while (read_config_line(...)) */

   fclose(configfp);

   set_debug_level(config->debug);

   freez(config->logfile);

   if (daemon_mode)
   {
      if (NULL != logfile)
      {
         config->logfile = logfile;
         init_error_log(Argv[0], config->logfile);
      }
      else
      {
         disable_logging();
      }
   }

#ifdef FEATURE_HTTPS_INSPECTION
   /*
    * Setting SSL parameters from loaded values into structures
    */
   freez(config->ca_directory);
   config->ca_directory = make_path(NULL, ca_directory);
   freez(ca_directory);

   freez(config->ca_cert_file);
   config->ca_cert_file = make_path(config->ca_directory, ca_cert_file);
   freez(ca_cert_file);

   freez(config->ca_key_file);
   config->ca_key_file  = make_path(config->ca_directory, ca_key_file);
   freez(ca_key_file);

   freez(config->trusted_cas_file);
   config->trusted_cas_file = make_path(config->ca_directory, trusted_cas_file);
   freez(trusted_cas_file);

   freez(config->certificate_directory);
   config->certificate_directory = make_path(NULL, certificate_directory);
   freez(certificate_directory);
#endif
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   if (config->default_server_timeout > config->keep_alive_timeout)
   {
      log_error(LOG_LEVEL_ERROR,
         "Reducing the default-server-timeout from %d to the keep-alive-timeout %d.",
         config->default_server_timeout, config->keep_alive_timeout);
      config->default_server_timeout = config->keep_alive_timeout;
   }
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */

#ifdef FEATURE_CONNECTION_SHARING
   if (config->feature_flags & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE)
   {
      if (!config->multi_threaded)
      {
         /*
          * While we could use keep-alive without multiple threads
          * if we didn't bother with enforcing the connection timeout,
          * that might make Tor users sad, even though they shouldn't
          * enable the single-threaded option anyway.
          *
          * XXX: We could still use Proxy-Connection: keep-alive.
          */
         config->feature_flags &= ~RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE;
         log_error(LOG_LEVEL_ERROR,
            "Config option single-threaded disables connection keep-alive.");
      }
   }
   else if ((config->feature_flags & RUNTIME_FEATURE_CONNECTION_SHARING))
   {
      log_error(LOG_LEVEL_ERROR, "Config option connection-sharing "
         "has no effect if keep-alive-timeout isn't set.");
      config->feature_flags &= ~RUNTIME_FEATURE_CONNECTION_SHARING;
   }
#endif /* def FEATURE_CONNECTION_SHARING */

   if (NULL == config->proxy_args)
   {
      log_error(LOG_LEVEL_FATAL, "Out of memory loading config - insufficient memory for config->proxy_args");
   }

   if (config->re_filterfile[0])
   {
      add_loader(load_re_filterfiles, config);
   }

   if (config->actions_file[0])
   {
      add_loader(load_action_files, config);
   }

#ifdef FEATURE_TRUST
   if (config->trustfile)
   {
      add_loader(load_trustfile, config);
   }
#endif /* def FEATURE_TRUST */

   if (NULL == config->haddr[0])
   {
      config->haddr[0] = strdup_or_die(HADDR_DEFAULT);
   }

   for (i = 0; i < MAX_LISTENING_SOCKETS && NULL != config->haddr[i]; i++)
   {
      if ((*config->haddr[i] == '[')
         && (NULL != (p = strchr(config->haddr[i], ']')))
         && (p[1] == ':')
         && (0 < (config->hport[i] = atoi(p + 2))))
      {
         *p = '\0';
         memmove((void *)config->haddr[i], config->haddr[i] + 1,
            (size_t)(p - config->haddr[i]));
      }
      else if (NULL != (p = strchr(config->haddr[i], ':'))
         && (0 < (config->hport[i] = atoi(p + 1))))
      {
         *p = '\0';
      }
      else
      {
         log_error(LOG_LEVEL_FATAL, "invalid bind port spec %s", config->haddr[i]);
         /* Never get here - LOG_LEVEL_FATAL causes program exit */
      }
      if (*config->haddr[i] == '\0')
      {
         /*
          * Only the port specified. We stored it in config->hport[i]
          * and don't need its text representation anymore.
          * Use config->hport[i] == 0 to iterate listening addresses since
          * now.
          */
         freez(config->haddr[i]);
      }
   }

   /*
    * Want to run all the loaders once now.
    *
    * Need to set up a fake csp, so they can get to the config.
    */
   fake_csp = zalloc_or_die(sizeof(*fake_csp));
   fake_csp->config = config;

   if (run_loader(fake_csp))
   {
      freez(fake_csp);
      log_error(LOG_LEVEL_FATAL, "A loader failed while loading config file. Exiting.");
      /* Never get here - LOG_LEVEL_FATAL causes program exit */
   }
   freez(fake_csp);

/* FIXME: this is a kludge for win32 */
#if defined(_WIN32) && !defined (_WIN_CONSOLE)

   g_default_actions_file = config->actions_file[1]; /* FIXME Hope this is default.action */
   g_user_actions_file  = config->actions_file[2];  /* FIXME Hope this is user.action */
   g_default_filterfile = config->re_filterfile[0]; /* FIXME Hope this is default.filter */
   g_user_filterfile    = config->re_filterfile[1]; /* FIXME Hope this is user.filter */

#ifdef FEATURE_TRUST
   g_trustfile        = config->trustfile;
#endif /* def FEATURE_TRUST */


#endif /* defined(_WIN32) && !defined (_WIN_CONSOLE) */
/* FIXME: end kludge */


   if (current_configfile == NULL)
   {
      config->need_bind = 1;
   }
   else
   {
      struct configuration_spec * oldcfg = (struct configuration_spec *)
                                           current_configfile->f;
      /*
       * Check if config->haddr[i],hport[i] == oldcfg->haddr[i],hport[i]
       */
      config->need_bind = 0;

      for (i = 0; i < MAX_LISTENING_SOCKETS; i++)
      {
         if (config->hport[i] != oldcfg->hport[i])
         {
            config->need_bind = 1;
         }
         else if (config->haddr[i] == NULL)
         {
            if (oldcfg->haddr[i] != NULL)
            {
               config->need_bind = 1;
            }
         }
         else if (oldcfg->haddr[i] == NULL)
         {
            config->need_bind = 1;
         }
         else if (0 != strcmp(config->haddr[i], oldcfg->haddr[i]))
         {
            config->need_bind = 1;
         }
      }

      current_configfile->unloader = unload_configfile;
   }

   fs->next = files->next;
   files->next = fs;

   current_configfile = fs;

   return (config);
}


/*********************************************************************
 *
 * Function    :  savearg
 *
 * Description :  Called from `load_config'.  It saves each non-empty
 *                and non-comment line from config into
 *                config->proxy_args.  This is used to create the
 *                show-status page.  On error, frees
 *                config->proxy_args and sets it to NULL
 *
 * Parameters  :
 *          1  :  command = config setting that was found
 *          2  :  argument = the setting's argument (if any)
 *          3  :  config = Configuration to save into.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void savearg(char *command, char *argument, struct configuration_spec * config)
{
   char * buf;
   char * s;

   assert(command);
   assert(argument);

   /*
    * Add config option name embedded in
    * link to its section in the user-manual
    */
   buf = strdup_or_die("\n<a href=\"");
   if (!strncmpic(config->usermanual, "file://", 7) ||
       !strncmpic(config->usermanual, "http", 4))
   {
      string_append(&buf, config->usermanual);
   }
   else
   {
      string_append(&buf, "http://" CGI_SITE_2_HOST "/user-manual/");
   }
   string_append(&buf, CONFIG_HELP_PREFIX);
   string_join  (&buf, string_toupper(command));
   string_append(&buf, "\">");
   string_append(&buf, command);
   string_append(&buf, "</a> ");

   if (NULL == buf)
   {
      freez(config->proxy_args);
      return;
   }

   if ((NULL != argument) && ('\0' != *argument))
   {
      s = html_encode(argument);
      if (NULL == s)
      {
         freez(buf);
         freez(config->proxy_args);
         return;
      }

      if (strncmpic(argument, "http://", 7) == 0)
      {
         string_append(&buf, "<a href=\"");
         string_append(&buf, s);
         string_append(&buf, "\">");
         string_join  (&buf, s);
         string_append(&buf, "</a>");
      }
      else
      {
         string_join  (&buf, s);
      }
   }

   string_append(&buf, "<br>");
   string_join(&config->proxy_args, buf);
}


/*
  Local Variables:
  tab-width: 3
  end:
*/
