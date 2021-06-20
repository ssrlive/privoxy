#include "config.h"
#include "privoxyexports.h"

#include "jcc.c"

#ifndef FEATURE_GRACEFUL_TERMINATION
#error "You must define FEATURE_GRACEFUL_TERMINATION macro!"
#endif /* FEATURE_GRACEFUL_TERMINATION */

static void listen_loop_with_cb(privoxy_cb cb, void *data);

int privoxy_main_entry(const char *conf_path, privoxy_cb cb, void *data) {
#ifndef HAVE_ARC4RANDOM
    unsigned int random_seed;
#endif

    configfile = conf_path;

    /* Prepare mutexes if supported and necessary. */
    initialize_mutexes();

    /* Enable logging until further notice. */
    init_log_module();

    files->next = NULL;
    clients->next = NULL;

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

    /* Initialize the CGI subsystem */
    cgi_init_error_messages();

    listen_loop_with_cb(cb, data);

    release_mutexes();

    return 0;
}

jb_socket bfds[MAX_LISTENING_SOCKETS] = { 0 };

static void listen_loop_with_cb(privoxy_cb cb, void *data)
{
   jb_socket *listen_fd = NULL;

   struct client_states *csp_list = NULL;
   struct client_state *csp = NULL;
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

   g_terminate = 0;

   listen_fd = &bfds[0];
   if (cb) {
      cb((int) *listen_fd, data);
   }

#ifdef FEATURE_GRACEFUL_TERMINATION
   while (!g_terminate)
#else
   for (;;)
#endif
   {
#if !defined(FEATURE_PTHREAD) && !defined(_WIN32) && !defined(__BEOS__) && !defined(__OS2__)
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

      csp_list = (struct client_states *) zalloc_or_die(sizeof(*csp_list));
      csp = &csp_list->csp;

      log_error(LOG_LEVEL_CONNECT,
         "Waiting for the next client connection. Currently active threads: %u",
         active_threads);

      /*
       * This config may be outdated, but for accept_connection()
       * it's fresh enough.
       */
      csp->config = config;

      if ((*listen_fd == JB_INVALID_SOCKET) || !accept_connection(csp, bfds))
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

   if (*listen_fd != JB_INVALID_SOCKET) {
      close_socket(*listen_fd);
   }

#if defined(FEATURE_PTHREAD)
   pthread_attr_destroy(&attrs);
#endif

   /* NOTREACHED unless FEATURE_GRACEFUL_TERMINATION is defined */

#ifdef FEATURE_GRACEFUL_TERMINATION

   log_error(LOG_LEVEL_INFO, "Graceful termination requested.");

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

#endif /* FEATURE_GRACEFUL_TERMINATION */
}

extern void privoxy_shutdown(void) {
    jb_socket bfds_cache[MAX_LISTENING_SOCKETS];
#ifdef FEATURE_GRACEFUL_TERMINATION
    g_terminate = 1;
#endif
    memcpy(bfds_cache, bfds, sizeof(bfds_cache));
    bfds[0] = JB_INVALID_SOCKET;
    close_ports_helper(bfds_cache);
}
