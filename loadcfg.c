const char loadcfg_rcs[] = "$Id: loadcfg.c,v 1.1 2001/05/13 21:57:06 administrator Exp $";
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/loadcfg.c,v $
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

#else /* ifndef _WIN32 */

# include <unistd.h>
# include <sys/time.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <signal.h>

#endif

#include "loadcfg.h"
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

static const char VANILLA_WAFER[] =
   "NOTICE=TO_WHOM_IT_MAY_CONCERN_"
   "Do_not_send_me_any_copyrighted_information_other_than_the_"
   "document_that_I_am_requesting_or_any_of_its_necessary_components._"
   "In_particular_do_not_send_me_any_cookies_that_"
   "are_subject_to_a_claim_of_copyright_by_anybody._"
   "Take_notice_that_I_refuse_to_be_bound_by_any_license_condition_"
   "(copyright_or_otherwise)_applying_to_any_cookie._";

#ifdef TOGGLE
/* by haroon - indicates if ijb is enabled */
int g_bToggleIJB        = 1;   /* JunkBusters is enabled by default. */
#endif

int debug               = 0;
int multi_threaded      = 1;

#if defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST)
int tinygif             = 0;
const char *tinygifurl  = NULL;
#endif /* defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST) */

const char *logfile     = NULL;

const char *configfile  = NULL;

const char *blockfile   = NULL;
const char *cookiefile  = NULL;
const char *forwardfile = NULL;

#ifdef ACL_FILES
const char *aclfile     = NULL;
#endif /* def ACL_FILES */

#ifdef USE_IMAGE_LIST
const char *imagefile   = NULL;
#endif /* def USE_IMAGE_LIST */

#ifdef KILLPOPUPS
const char *popupfile   = NULL;
int kill_all_popups     = 0;     /* Not recommended really ... */
#endif /* def KILLPOPUPS */

#ifdef PCRS
const char *re_filterfile = NULL;
int re_filter_all       = 0;
#endif /* def PCRS */

#ifdef TRUST_FILES
const char *trustfile   = NULL;
#endif /* def TRUST_FILES */

#ifdef JAR_FILES
const char *jarfile     = NULL;
FILE *jar = NULL;
#endif /* def JAR_FILES */

const char *referrer    = NULL;
const char *uagent      = NULL;
const char *from        = NULL;

#ifndef SPLIT_PROXY_ARGS
const char *suppress_message = NULL;
#endif /* ndef SPLIT_PROXY_ARGS */

int suppress_vanilla_wafer = 0;
int add_forwarded       = 0;

struct list wafer_list[1];
struct list xtra_list[1];

#ifdef TRUST_FILES
struct list trust_info[1];
struct url_spec *trust_list[64];
#endif /* def TRUST_FILES */

/*
 * Port and IP to bind to.
 * Defaults to HADDR_DEFAULT:HADDR_PORT == 127.0.0.1:8000
 */
const char *haddr = NULL;
int         hport = 0;

#ifndef SPLIT_PROXY_ARGS
int suppress_blocklists = 0;  /* suppress listing sblock and simage */
#endif /* ndef SPLIT_PROXY_ARGS */

struct proxy_args proxy_args[1];

int configret = 0;
int config_changed = 0;


/*
 * The load_config function is now going to call `init_proxy_args',
 * so it will need argc and argv.  Since load_config will also be
 * a signal handler, we need to have these globally available.
 */
int Argc = 0;
const char **Argv = NULL;


/*
 * This takes the "cryptic" hash of each keyword and aliases them to
 * something a little more readable.  This also makes changing the
 * hash values easier if they should change or the hash algorthm changes.
 * Use the included "hash" program to find out what the hash will be
 * for any string supplied on the command line.
 */

#define hash_trustfile                 56494766ul
#define hash_trust_info_url            449869467ul
#define hash_debug                     78263ul
#define hash_tinygif                   2227702ul
#define hash_add_forwarded_header      3191044770ul
#define hash_single_threaded           4250084780ul
#define hash_suppress_vanilla_wafer    3121233547ul
#define hash_wafer                     89669ul
#define hash_add_header                237434619ul
#define hash_cookiefile                247469766ul
#define hash_logfile                   2114766ul
#define hash_blockfile                 48845391ul
#define hash_imagefile                 51447891ul
#define hash_jarfile                   2046641ul
#define hash_listen_address            1255650842ul
#define hash_forwardfile               1268669141ul
#define hash_aclfile                   1908516ul
#define hash_popupfile                 54623516ul
#define hash_kill_all_popups           2311539906ul
#define hash_re_filterfile             3877522444ul
#define hash_re_filter_all             3877521376ul
#define hash_user_agent                283326691ul
#define hash_referrer                  10883969ul
#define hash_referer                   2176719ul
#define hash_from                      16264ul
#define hash_hide_console              2048809870ul
#define hash_include_stats             2174146548ul
#define hash_suppress_blocklists       1948693308ul
#define hash_toggle                    447966ul

#define hash_activity_animation        1817904738ul
#define hash_log_messages              2291744899ul
#define hash_log_highlight_messages    4032101240ul
#define hash_log_buffer_size           2918070425ul
#define hash_log_max_lines             2868344173ul
#define hash_log_font_name             2866730124ul
#define hash_log_font_size             2866731014ul
#define hash_show_on_task_bar          215410365ul
#define hash_close_button_minimizes    3651284693ul


/*********************************************************************
 *
 * Function    :  load_config
 *
 * Description :  Load the config file and all parameters.
 *
 * Parameters  :
 *          1  :  signum : this can be the signal SIGHUP or 0 (if from main).
 *                In any case, we just ignore this and reload the config file.
 *
 * Returns     :  configret : 0 => Ok, everything else is an error.
 *                Note: we use configret since a signal handler cannot
 *                return a value, and this function does double duty.
 *                Ie. Is is called from main and from signal( SIGHUP );
 *
 *********************************************************************/
void load_config( int signum )
{
   char buf[BUFSIZ];
   char *p, *q;
   FILE *configfp = NULL;

   configret = 0;
   config_changed = 1;

   log_error(LOG_LEVEL_INFO, "loading configuration file '%s':", configfile);

   init_proxy_args(Argc, Argv);


   /* (Waste of memory [not quite a "leak"] here.  The 
    * last blockfile/popupfile/... etc will not be 
    * unloaded until we load a new one.  If the 
    * block/popup/... feature has been disabled in 
    * the new config file, then we're wasting some 
    * memory we could otherwise reclaim.
    */

   /* Disable all loaders. */
   remove_all_loaders();

   /*
    * Reset to as close to startup state as we can.
    * But leave changing the logfile until after we're done loading.
    */

   #ifdef JAR_FILES
   if ( NULL != jar )
   {
      fclose( jar );
      jar = NULL;
   }
   #endif /* def JAR_FILES */

   debug             = 0;
   multi_threaded    = 1;

#if defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST)
   tinygif           = 0;
   freez((char *)tinygifurl);
#endif /* defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST) */

   suppress_vanilla_wafer = 0;
   add_forwarded     = 0;
   hport             = HADDR_PORT;

#ifdef _WIN_CONSOLE
   hideConsole       = 0;
#endif /*def _WIN_CONSOLE*/

#ifdef PCRS
   re_filter_all     = 0;
#endif /* def PCRS */

#ifdef KILLPOPUPS
   kill_all_popups   = 0;
#endif /* def KILLPOPUPS */

#ifdef TOGGLE
   g_bToggleIJB      = 1;
#endif

#ifdef STATISTICS
   urls_read         = 0;
   urls_rejected     = 0;
#endif /* def STATISTICS */

#ifndef SPLIT_PROXY_ARGS
   suppress_blocklists = 0;
#endif /* ndef SPLIT_PROXY_ARGS */

   freez((char *)from);
   freez((char *)haddr);
   freez((char *)uagent);
   freez((char *)referrer);
   freez((char *)logfile);


   freez((char *)blockfile);
   freez((char *)cookiefile);
   freez((char *)forwardfile);

#ifdef ACL_FILES
   freez((char *)aclfile);
#endif /* def ACL_FILES */

#ifdef USE_IMAGE_LIST
   freez((char *)imagefile);
#endif /* def USE_IMAGE_LIST */

#ifdef JAR_FILES
   freez((char *)jarfile);
#endif /* def JAR_FILES */

#ifdef KILLPOPUPS
   freez((char *)popupfile);
#endif /* def KILLPOPUPS */

#ifndef SPLIT_PROXY_ARGS
   freez((char *)suppress_message);
#endif /* ndef SPLIT_PROXY_ARGS */

#ifdef TRUST_FILES
   freez((char *)trustfile);
#endif /* def TRUST_FILES */

#ifdef PCRS
   freez((char *)re_filterfile);
#endif /* def PCRS */

   if (NULL != configfile)
   {
      if ((configfp = fopen(configfile, "r")) == NULL)
      {
         log_error(LOG_LEVEL_ERROR, "can't open configuration file '%s':  %E",
                 configfile);
         configret = 1;
         return;
      }
   }

   if (NULL != configfp)
   {
      memset (buf, 'j', sizeof(buf));
      while (read_config_line(buf, sizeof(buf), configfp, NULL) != NULL)
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
         savearg(cmd, arg);


         switch( hash_string( cmd ) )
         {
#ifdef TRUST_FILES
            case hash_trustfile :
               freez((char *)trustfile);
               trustfile = strdup(arg);
               continue;

            case hash_trust_info_url :
               enlist(trust_info, arg);
               continue;
#endif /* def TRUST_FILES */

            case hash_debug :
               debug |= atoi(arg);
               continue;

#if defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST)
            case hash_tinygif :
               freez((char *)tinygifurl);
               tinygif = atoi(arg);
               if(3 == tinygif)
               {
                  p = arg;
                  while((*p >= '0') && (*p <= '9'))
                  {
                     p++;
                  }
                  while((*p == ' ') || (*p == '\t'))
                  {
                     p++;
                  }
                  if (*p)
                  {
                     q = malloc(strlen(p) + 5);
                     if (q)
                     {
                        strcpy(q, p);
                        strcat(q, "\r\n\r\n");
                        tinygifurl = q;
                     }
                  }
               }
               if ((tinygif != 1) && 
                   (tinygif != 2) && 
                   ((tinygif != 3) || (tinygifurl==NULL)) )
               {
                  log_error(LOG_LEVEL_ERROR, "tinygif setting invalid.");
               }
               continue;
#endif /* defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST) */

            case hash_add_forwarded_header :
               add_forwarded = 1;
               continue;

            case hash_single_threaded :
               multi_threaded = 0;
               continue;

            case hash_suppress_vanilla_wafer :
               suppress_vanilla_wafer = 1;
               continue;

            case hash_wafer :
               enlist(wafer_list, arg);
               continue;

            case hash_add_header :
               enlist(xtra_list, arg);
               continue;

            case hash_cookiefile :
               freez((char *)cookiefile);
               cookiefile = strdup(arg);
               continue;

            case hash_logfile :
               freez((char *)logfile);
               logfile = strdup(arg);
               continue;

            case hash_blockfile :
               freez((char *)blockfile);
               blockfile = strdup(arg);
               continue;

#ifdef USE_IMAGE_LIST
            case hash_imagefile :
               freez((char *)imagefile);
               imagefile = strdup(arg);
               continue;
#endif /* def USE_IMAGE_LIST */

#ifdef JAR_FILES
            case hash_jarfile :
               freez((char *)jarfile);
               jarfile = strdup(arg);
               continue;
#endif /* def JAR_FILES */

            case hash_listen_address :
               freez((char *)haddr);
               haddr = strdup(arg);
               continue;

            case hash_forwardfile :
               freez((char *)forwardfile);
               forwardfile = strdup(arg);
               continue;

#ifdef ACL_FILES
            case hash_aclfile :
               freez((char *)aclfile);
               aclfile = strdup(arg);
               continue;
#endif /* def ACL_FILES */

#ifdef KILLPOPUPS
            case hash_popupfile :
               freez((char *)popupfile);
               popupfile = strdup(arg);
               continue;

            case hash_kill_all_popups :
               kill_all_popups = 1;
               continue;
#endif /* def KILLPOPUPS */

#ifdef PCRS
            case hash_re_filterfile :
               freez((char *)re_filterfile);
               re_filterfile = strdup(arg);
               continue;

            case hash_re_filter_all :
               re_filter_all = 1;
               log_error(LOG_LEVEL_REF, "re_filter policy is %s.",
                          re_filter_all ? "RADICAL" : "SEMI-SMART");
               continue;
#endif /* def PCRS */

            case hash_user_agent :
               freez((char *)uagent);
               uagent = strdup(arg);
               continue;

               /*
                * Offer choice of correct spelling according to dictionary,
                * or the misspelling used in the HTTP spec.
                */
            case hash_referrer :
            case hash_referer :
               freez((char *)referrer);
               referrer = strdup(arg);
               continue;

            case hash_from :
               freez((char *)from);
               from = strdup(arg);
               continue;

#ifdef _WIN_CONSOLE
            case hash_hide_console :
               hideConsole = 1;
               continue;
#endif /*def _WIN_CONSOLE*/

#ifndef SPLIT_PROXY_ARGS
            case hash_suppress_blocklists :
               if (arg[0] != '\0')
               {
                  suppress_message = strdup(arg);
               }
               else
               {
                  /* There will be NO reference in proxy-args. */
                  suppress_message = NULL;
               }

               suppress_blocklists = 1;
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

#ifndef TRUST_FILES
            case hash_trustfile :
            case hash_trust_info_url :
#endif /* ndef TRUST_FILES */
#ifndef USE_IMAGE_LIST
            case hash_imagefile :
#endif /* ndef USE_IMAGE_LIST */
#ifndef PCRS
            case hash_re_filterfile :
            case hash_re_filter_all :
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
#if !defined(DETECT_MSIE_IMAGES) && !defined(USE_IMAGE_LIST)
            case hash_tinygif :
#endif /* !defined(DETECT_MSIE_IMAGES) && !defined(USE_IMAGE_LIST) */
#ifndef KILLPOPUPS
            case hash_popupfile :
            case hash_kill_all_popups :
#endif /* ndef KILLPOPUPS */
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
               log_error(LOG_LEVEL_ERROR, "Unrecognized directive (%lulu) in "
                     "configuration file: \"%s\"", hash_string( cmd ), buf);
               p = malloc( BUFSIZ );
               if (p != NULL)
               {
                  sprintf( p, "<br>\nWARNING: unrecognized directive : %s<br><br>\n", buf );
                  proxy_args->invocation = strsav( proxy_args->invocation, p );
                  freez( p );
               }
               /*
                * I decided that I liked this better as a warning than an
                * error.
                */

               /*
                * configret = 1;
                * return;
                */
               continue;
         }
      }
      fclose(configfp);
   }

   init_error_log(Argv[0], logfile, debug);

   if (cookiefile)
   {
      add_loader(load_cookiefile);
   }

   if (blockfile)
   {
      add_loader(load_blockfile);
   }

#ifdef USE_IMAGE_LIST
   if (imagefile)
   {
      add_loader(load_imagefile);
   }
#endif /* def USE_IMAGE_LIST */

#ifdef TRUST_FILES
   if (trustfile)
   {
      add_loader(load_trustfile);
   }
#endif /* def TRUST_FILES */

   if (forwardfile)
   {
      add_loader(load_forwardfile);
   }

#ifdef ACL_FILES
   if (aclfile)
   {
      add_loader(load_aclfile);
   }
#endif /* def ACL_FILES */

#ifdef KILLPOPUPS
   if (popupfile)
   {
      add_loader(load_popupfile);
   }
#endif /* def KILLPOPUPS */

#ifdef PCRS
   if (re_filterfile)
   {
      add_loader(load_re_filterfile);
   }
#endif /* def PCRS */

#ifdef JAR_FILES
   if ( NULL != jarfile )
   {
      if ( NULL == (jar = fopen(jarfile, "a")) )
      {
         log_error(LOG_LEVEL_ERROR, "can't open jarfile '%s': %E", jarfile);
         configret = 1;
         return;
      }
      setbuf(jar, NULL);
   }
#endif /* def JAR_FILES */

   if ( NULL == haddr )
   {
      haddr = strdup( HADDR_DEFAULT );
   }

   if ( NULL != haddr )
   {
      if ((p = strchr(haddr, ':')))
      {
         *p++ = '\0';
         if (*p)
         {
            hport = atoi(p);
         }
      }

      if (hport <= 0)
      {
         *--p = ':';
         log_error(LOG_LEVEL_ERROR, "invalid bind port spec %s", haddr);
         configret = 1;
         return;
      }
      if (*haddr == '\0')
      {
         haddr = NULL;
      }
   }

   if (run_loader(NULL))
   {
      configret = 1;
      return;
   }

#ifdef JAR_FILES
   /*
    * If we're logging cookies in a cookie jar, and the user has not
    * supplied any wafers, and the user has not told us to suppress the
    * vanilla wafer, then send the vanilla wafer.
    */
   if ((jarfile != NULL)
       && (wafer_list->next == NULL)
       && (suppress_vanilla_wafer == 0))
   {
      enlist(wafer_list, VANILLA_WAFER);
   }
#endif /* def JAR_FILES */

   end_proxy_args();

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
