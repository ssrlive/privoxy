/* vim:ts=3: */
const char loadcfg_rcs[] = "$Id: loadcfg.c,v 1.13 2001/06/05 22:33:54 jongfoster Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/loadcfg.c,v $
 *
 * Purpose     :  Loads settings from the configuration file into
 *                global variables.  This file contains both the 
 *                routine to load the configuration and the global
 *                variables it writes to.
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
 *    $Log: loadcfg.c,v $
 *    Revision 1.13  2001/06/05 22:33:54  jongfoster
 *
 *    Fixed minor memory leak.
 *    Also now uses make_path to prepend the pathnames.
 *
 *    Revision 1.12  2001/06/05 20:04:09  jongfoster
 *    Now uses _snprintf() in place of snprintf() under Win32.
 *
 *    Revision 1.11  2001/06/04 18:31:58  swa
 *    files are now prefixed with either `confdir' or `logdir'.
 *    `make redhat-dist' replaces both entries confdir and logdir
 *    with redhat values
 *
 *    Revision 1.10  2001/06/03 19:11:54  oes
 *    introduced confdir option
 *
 *    Revision 1.10  2001/06/03 11:03:48  oes
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
 *    Revision 1.9  2001/06/01 20:06:24  jongfoster
 *    Removed support for "tinygif" option - moved to actions file.
 *
 *    Revision 1.8  2001/05/31 21:27:13  jongfoster
 *    Removed many options from the config file and into the
 *    "actions" file: add_forwarded, suppress_vanilla_wafer,
 *    wafer, add_header, user_agent, referer, from
 *    Also globally replaced "permission" with "action".
 *
 *    Revision 1.7  2001/05/29 09:50:24  jongfoster
 *    Unified blocklist/imagelist/permissionslist.
 *    File format is still under discussion, but the internal changes
 *    are (mostly) done.
 *
 *    Also modified interceptor behaviour:
 *    - We now intercept all URLs beginning with one of the following
 *      prefixes (and *only* these prefixes):
 *        * http://i.j.b/
 *        * http://ijbswa.sf.net/config/
 *        * http://ijbswa.sourceforge.net/config/
 *    - New interceptors "home page" - go to http://i.j.b/ to see it.
 *    - Internal changes so that intercepted and fast redirect pages
 *      are not replaced with an image.
 *    - Interceptors now have the option to send a binary page direct
 *      to the client. (i.e. ijb-send-banner uses this)
 *    - Implemented show-url-info interceptor.  (Which is why I needed
 *      the above interceptors changes - a typical URL is
 *      "http://i.j.b/show-url-info?url=www.somesite.com/banner.gif".
 *      The previous mechanism would not have intercepted that, and
 *      if it had been intercepted then it then it would have replaced
 *      it with an image.)
 *
 *    Revision 1.6  2001/05/26 00:28:36  jongfoster
 *    Automatic reloading of config file.
 *    Removed obsolete SIGHUP support (Unix) and Reload menu option (Win32).
 *    Most of the global variables have been moved to a new
 *    struct configuration_spec, accessed through csp->config->globalname
 *    Most of the globals remaining are used by the Win32 GUI.
 *
 *    Revision 1.5  2001/05/25 22:34:30  jongfoster
 *    Hard tabs->Spaces
 *
 *    Revision 1.4  2001/05/22 18:46:04  oes
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
 *    Revision 1.2  2001/05/17 23:01:01  oes
 *     - Cleaned CRLF's from the sources and related files
 *
 *    Revision 1.1.1.1  2001/05/15 13:58:58  oes
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
#include <ctype.h>

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

/* VC++ has "_snprintf", not "snprintf" */
#define snprintf _snprintf

#else /* ifndef _WIN32 */

# include <unistd.h>
# include <sys/time.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <signal.h>

#endif

#include "loadcfg.h"
#include "list.h"
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

const char loadcfg_h_rcs[] = LOADCFG_H_VERSION;

/*
 * Fix a problem with Solaris.  There should be no effect on other
 * platforms.
 * Solaris's isspace() is a macro which uses it's argument directly
 * as an array index.  Therefore we need to make sure that high-bit
 * characters generate +ve values, and ideally we also want to make
 * the argument match the declared parameter type of "int".
 */
#define ijb_isupper(__X) isupper((int)(unsigned char)(__X))
#define ijb_tolower(__X) tolower((int)(unsigned char)(__X))

#ifdef TOGGLE
/* by haroon - indicates if ijb is enabled */
int g_bToggleIJB        = 1;   /* JunkBusters is enabled by default. */
#endif

/* The filename of the configfile */
const char *configfile  = NULL;

/*
 * The load_config function is now going to call `init_proxy_args',
 * so it will need argc and argv.  So we need to have these
 * globally available.
 */
int Argc = 0;
const char **Argv = NULL;

static struct file_list *current_configfile = NULL;


/*
 * This takes the "cryptic" hash of each keyword and aliases them to
 * something a little more readable.  This also makes changing the
 * hash values easier if they should change or the hash algorthm changes.
 * Use the included "hash" program to find out what the hash will be
 * for any string supplied on the command line.  (Or just put it in the
 * config file and read the number from the error message in the log).
 */


#define hash_aclfile                      1908516ul
#define hash_actions_file              3825730796ul /* FIXME "permissionsfile" */
#define hash_debug                          78263ul
#define hash_confdir                      1978389lu
#define hash_logdir                        422889lu
#define hash_forwardfile               1268669141ul
#define hash_jarfile                      2046641ul
#define hash_listen_address            1255650842ul
#define hash_logfile                      2114766ul
#define hash_re_filterfile             3877522444ul
#define hash_single_threaded           4250084780ul
#define hash_suppress_blocklists       1948693308ul
#define hash_toggle                        447966ul
#define hash_trust_info_url             449869467ul
#define hash_trustfile                   56494766ul

#define hash_hide_console              2048809870ul

#define hash_activity_animation        1817904738ul
#define hash_close_button_minimizes    3651284693ul
#define hash_log_buffer_size           2918070425ul
#define hash_log_font_name             2866730124ul
#define hash_log_font_size             2866731014ul
#define hash_log_highlight_messages    4032101240ul
#define hash_log_max_lines             2868344173ul
#define hash_log_messages              2291744899ul
#define hash_show_on_task_bar           215410365ul


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
void unload_configfile (void * data)
{
   struct configuration_spec * config = (struct configuration_spec *)data;

#ifdef JAR_FILES
   if ( NULL != config->jar )
   {
      fclose( config->jar );
      config->jar = NULL;
   }
#endif /* def JAR_FILES */

   freez((char *)config->confdir);
   freez((char *)config->logdir);

   freez((char *)config->haddr);
   freez((char *)config->logfile);

   freez((char *)config->actions_file);
   freez((char *)config->forwardfile);

#ifdef ACL_FILES
   freez((char *)config->aclfile);
#endif /* def ACL_FILES */

#ifdef JAR_FILES
   freez((char *)config->jarfile);
#endif /* def JAR_FILES */

#ifndef SPLIT_PROXY_ARGS
   freez((char *)config->suppress_message);
#endif /* ndef SPLIT_PROXY_ARGS */

#ifdef PCRS
   freez((char *)config->re_filterfile);
#endif /* def PCRS */

}


/*********************************************************************
 *
 * Function    :  load_config
 *
 * Description :  Load the config file and all parameters.
 *
 * Parameters  :
 *          1  :  csp = Client state (the config member will be 
 *                filled in by this function).
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
struct configuration_spec * load_config(void)
{
   char buf[BUFSIZ];
   char *p, *q;
   FILE *configfp = NULL;
   struct configuration_spec * config = NULL;
   struct client_state * fake_csp;
   struct file_list *fs;

   if (!check_file_changed(current_configfile, configfile, &fs))
   {
      /* No need to load */
      return ((struct configuration_spec *)current_configfile->f);
   }
   if (!fs)
   {
      log_error(LOG_LEVEL_FATAL, "can't check configuration file '%s':  %E",
                configfile);
   }

   log_error(LOG_LEVEL_INFO, "loading configuration file '%s':", configfile);

#ifdef TOGGLE
   g_bToggleIJB      = 1;
#endif

   fs->f = config = (struct configuration_spec *)zalloc(sizeof(*config));

   if (config==NULL)
   {
      freez(fs->filename);
      freez(fs);
      log_error(LOG_LEVEL_FATAL, "can't allocate memory for configuration");
      /* Never get here - LOG_LEVEL_FATAL causes program exit */
   }

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

   init_proxy_args(Argc, Argv, config);

   /*
    * Set to defaults
    */

   config->multi_threaded    = 1;
   config->hport             = HADDR_PORT;

   if ((configfp = fopen(configfile, "r")) == NULL)
   {
      log_error(LOG_LEVEL_FATAL, "can't open configuration file '%s':  %E",
              configfile);
      /* Never get here - LOG_LEVEL_FATAL causes program exit */
   }

   while (read_config_line(buf, sizeof(buf), configfp, fs) != NULL)
   {
      char cmd[BUFSIZ];
      char arg[BUFSIZ];
      char tmp[BUFSIZ];

      strcpy(tmp, buf);

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
      strcpy(arg, p);

      /* Should never happen, but check this anyway */
      if (*cmd == '\0')
      {
         continue;
      }

      /* Make sure the command field is lower case */
      for (p=cmd; *p; p++)
      {
         if (ijb_isupper(*p))
         {
            *p = ijb_tolower(*p);
         }
      }

      /* Save the argument for show-proxy-args */
      savearg(cmd, arg, config);


      switch( hash_string( cmd ) )
      {
#ifdef TRUST_FILES
         case hash_trustfile :
            freez((char *)config->trustfile);
            config->trustfile = make_path(config->confdir, arg);
            continue;

         case hash_trust_info_url :
            enlist(config->trust_info, arg);
            continue;
#endif /* def TRUST_FILES */

         case hash_debug :
            config->debug |= atoi(arg);
            continue;

         case hash_confdir :
            freez((char *)config->confdir);
            config->confdir = strdup(arg);
            continue;            

         case hash_logdir :
            freez((char *)config->logdir);
            config->logdir = strdup(arg);
            continue;            

         case hash_single_threaded :
            config->multi_threaded = 0;
            continue;

         case hash_actions_file :
            freez((char *)config->actions_file);
            config->actions_file = make_path(config->confdir, arg);
            continue;

         case hash_logfile :
            freez((char *)config->logfile);
            config->logfile = make_path(config->logdir, arg);
            continue;

#ifdef JAR_FILES
         case hash_jarfile :
            freez((char *)config->jarfile);
            config->jarfile = make_path(config->logdir, arg);
            continue;
#endif /* def JAR_FILES */

         case hash_listen_address :
            freez((char *)config->haddr);
            config->haddr = strdup(arg);
            continue;

         case hash_forwardfile :
            freez((char *)config->forwardfile);
            config->forwardfile = make_path(config->confdir, arg);
            continue;

#ifdef ACL_FILES
         case hash_aclfile :
            freez((char *)config->aclfile);
            config->aclfile = make_path(config->confdir, arg);
            continue;
#endif /* def ACL_FILES */

#ifdef PCRS
         case hash_re_filterfile :
            freez((char *)config->re_filterfile);
            config->re_filterfile = make_path(config->confdir, arg);
            continue;
#endif /* def PCRS */

#ifdef _WIN_CONSOLE
         case hash_hide_console :
            hideConsole = 1;
            continue;
#endif /*def _WIN_CONSOLE*/

#ifndef SPLIT_PROXY_ARGS
         case hash_suppress_blocklists :
            if (arg[0] != '\0')
            {
               config->suppress_message = strdup(arg);
            }
            else
            {
               /* There will be NO reference in proxy-args. */
               config->suppress_message = NULL;
            }

            config->suppress_blocklists = 1;
            continue;
#endif /* ndef SPLIT_PROXY_ARGS */

#ifdef TOGGLE
         case hash_toggle :
            g_bToggleIJB = atoi(arg);
            continue;
#endif /* def TOGGLE */

#if defined(_WIN32) && ! defined(_WIN_CONSOLE)
         case hash_activity_animation :
            g_bShowActivityAnimation = atoi(arg);
            continue;

         case hash_log_messages :
            g_bLogMessages = atoi(arg);
            continue;

         case hash_log_highlight_messages :
            g_bHighlightMessages = atoi(arg);
            continue;

         case hash_log_buffer_size :
            g_bLimitBufferSize = atoi(arg);
            continue;

         case hash_log_max_lines :
            g_nMaxBufferLines = atoi(arg);
            continue;

         case hash_log_font_name :
            strcpy( g_szFontFaceName, arg );
            continue;

         case hash_log_font_size :
            g_nFontSize = atoi(arg);
            continue;

         case hash_show_on_task_bar :
            g_bShowOnTaskBar = atoi(arg);
            continue;

         case hash_close_button_minimizes :
            g_bCloseHidesWindow = atoi(arg);
            continue;
#endif /* defined(_WIN32) && ! defined(_WIN_CONSOLE) */

         /* Warnings about unsupported features */

#ifndef PCRS
         case hash_re_filterfile :
#endif /* ndef PCRS */
#ifndef TOGGLE
         case hash_toggle :
#endif /* ndef TOGGLE */
#if defined(_WIN_CONSOLE) || ! defined(_WIN32)
         case hash_activity_animation :
         case hash_log_messages :
         case hash_log_highlight_messages :
         case hash_log_buffer_size :
         case hash_log_max_lines :
         case hash_log_font_name :
         case hash_log_font_size :
         case hash_show_on_task_bar :
         case hash_close_button_minimizes :
#endif /* defined(_WIN_CONSOLE) || ! defined(_WIN32) */
#ifndef _WIN_CONSOLE
         case hash_hide_console :
#endif /* ndef _WIN_CONSOLE */
#ifndef JAR_FILES
         case hash_jarfile :
#endif /* ndef JAR_FILES */
#ifndef ACL_FILES
         case hash_aclfile :
#endif /* ndef ACL_FILES */
#ifdef SPLIT_PROXY_ARGS
         case hash_suppress_blocklists :
#endif /* def SPLIT_PROXY_ARGS */
            log_error(LOG_LEVEL_INFO, "Unsupported directive \"%s\" ignored.", cmd);
            continue;

         default :
            /*
             * I decided that I liked this better as a warning than an
             * error.  To change back to an error, just change log level
             * to LOG_LEVEL_FATAL.
             */
            log_error(LOG_LEVEL_ERROR, "Unrecognized directive (%lulu) in "
                  "configuration file: \"%s\"", hash_string( cmd ), buf);
            p = malloc( BUFSIZ );
            if (p != NULL)
            {
               sprintf( p, "<br>\nWARNING: unrecognized directive : %s<br><br>\n", buf );
               config->proxy_args_invocation = strsav( config->proxy_args_invocation, p );
               freez( p );
            }
            continue;
      } /* end switch( hash_string(cmd) ) */
   } /* end while ( read_config_line(...) ) */

   fclose(configfp);

   init_error_log(Argv[0], config->logfile, config->debug);

   if (config->actions_file)
   {
      add_loader(load_actions_file, config);
   }

   if (config->forwardfile)
   {
      add_loader(load_forwardfile, config);
   }

#ifdef ACL_FILES
   if (config->aclfile)
   {
      add_loader(load_aclfile, config);
   }
#endif /* def ACL_FILES */

#ifdef PCRS
   if (config->re_filterfile)
   {
      add_loader(load_re_filterfile, config);
   }
#endif /* def PCRS */

#ifdef TRUST_FILES
   if (config->trustfile)
   {
      add_loader(load_trustfile, config);
   }
#endif

#ifdef JAR_FILES
   if ( NULL != config->jarfile )
   {
      if ( NULL == (config->jar = fopen(config->jarfile, "a")) )
      {
         log_error(LOG_LEVEL_FATAL, "can't open jarfile '%s': %E", config->jarfile);
         /* Never get here - LOG_LEVEL_FATAL causes program exit */
      }
      setbuf(config->jar, NULL);
   }
#endif /* def JAR_FILES */

   if ( NULL == config->haddr )
   {
      config->haddr = strdup( HADDR_DEFAULT );
   }

   if ( NULL != config->haddr )
   {
      if ((p = strchr(config->haddr, ':')))
      {
         *p++ = '\0';
         if (*p)
         {
            config->hport = atoi(p);
         }
      }

      if (config->hport <= 0)
      {
         *--p = ':';
         log_error(LOG_LEVEL_FATAL, "invalid bind port spec %s", config->haddr);
         /* Never get here - LOG_LEVEL_FATAL causes program exit */
      }
      if (*config->haddr == '\0')
      {
         config->haddr = NULL;
      }
   }

   /*
    * Want to run all the loaders once now.
    *
    * Need to set up a fake csp, so they can get to the config.
    */
   fake_csp = (struct client_state *) zalloc (sizeof(*fake_csp));
   fake_csp->config = config;

   if (run_loader(fake_csp))
   {
      freez(fake_csp);
      log_error(LOG_LEVEL_FATAL, "A loader failed while loading config file. Exiting.");
      /* Never get here - LOG_LEVEL_FATAL causes program exit */
   }
   freez(fake_csp);

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

/* FIXME: this is a kludge for win32 */
#if defined(_WIN32) && !defined (_WIN_CONSOLE)

   g_actions_file     = config->actions_file;
   g_forwardfile      = config->forwardfile;
#ifdef ACL_FILES
   g_aclfile          = config->aclfile;
#endif /* def ACL_FILES */
#ifdef PCRS
   g_re_filterfile    = config->re_filterfile;
#endif
#ifdef TRUST_FILES
   g_trustfile        = config->trustfile;
#endif
   

#endif /* defined(_WIN32) && !defined (_WIN_CONSOLE) */
/* FIXME: end kludge */


   config->need_bind = 1;

   if (current_configfile)
   {
      struct configuration_spec * oldcfg = (struct configuration_spec *)
                                           current_configfile->f;
      /*
       * Check if config->haddr,hport == oldcfg->haddr,hport
       *
       * The following could be written more compactly as a single,
       * (unreadably long) if statement.
       */
      config->need_bind = 0;
      if (config->hport != oldcfg->hport)
      {
         config->need_bind = 1;
      }
      else if (config->haddr == NULL)
      {
         if (oldcfg->haddr != NULL)
         {
            config->need_bind = 1;
         }
      }
      else if (oldcfg->haddr == NULL)
      {
         config->need_bind = 1;
      }
      else if (0 != strcmp(config->haddr, oldcfg->haddr))
      {
         config->need_bind = 1;
      }

      current_configfile->unloader = unload_configfile;
   }

   fs->next = files->next;
   files->next = fs;

   current_configfile = fs;

   return (config);
}


/*
  Local Variables:
  tab-width: 3
  end:
*/
