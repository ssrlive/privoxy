#ifndef _LOADCFG_H
#define _LOADCFG_H
#define LOADCFG_H_VERSION "$Id: loadcfg.h,v 1.1 2001/05/13 21:57:06 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/loadcfg.h,v $
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
 *    $Log: loadcfg.h,v $
 *
 *********************************************************************/


/* Declare struct FILE for vars and funcs. */
#include <stdio.h>

/* All of our project's data types. */
#include "project.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Global variables */

#ifdef TOGGLE
/* indicates if ijb is enabled */
extern int g_bToggleIJB;
#endif

extern int debug;
extern int multi_threaded;

#if defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST)
extern int tinygif;
extern const char *tinygifurl;
#endif /* defined(DETECT_MSIE_IMAGES) || defined(USE_IMAGE_LIST) */

extern const char *logfile;

extern const char *configfile;

#ifdef ACL_FILES
extern const char *aclfile;
#endif /* def ACL_FILES */

extern const char *blockfile;
extern const char *cookiefile;
extern const char *forwardfile;

#ifdef USE_IMAGE_LIST
extern const char *imagefile;
#endif /* def USE_IMAGE_LIST */

#ifdef KILLPOPUPS
extern const char *popupfile;
#endif /* def KILLPOPUPS */

#ifdef TRUST_FILES
extern const char *trustfile;
#endif /* def TRUST_FILES */

#ifdef PCRS
extern const char *re_filterfile;
#endif /* def PCRS */

#ifdef PCRS
extern int re_filter_all;
#endif /* def PCRS */

#ifdef KILLPOPUPS
extern int kill_all_popups;     /* Not recommended really .. */
#endif /* def KILLPOPUPS */

#ifdef JAR_FILES
extern const char *jarfile;
extern FILE *jar;
#endif /* def JAR_FILES */

extern const char *referrer;
extern const char *uagent;
extern const char *from;

#ifndef SPLIT_PROXY_ARGS
extern const char *suppress_message;
#endif /* ndef SPLIT_PROXY_ARGS */

extern int suppress_vanilla_wafer;
extern int add_forwarded;

extern struct list wafer_list[];
extern struct list xtra_list[];

#ifdef TRUST_FILES
extern struct list trust_info[];
extern struct url_spec *trust_list[];
#endif /* def TRUST_FILES */

extern const char *haddr;
extern int   hport;

#ifndef SPLIT_PROXY_ARGS
extern int suppress_blocklists;  /* suppress listing sblock and simage */
#endif /* ndef SPLIT_PROXY_ARGS */

extern struct proxy_args proxy_args[1];

extern int configret;
extern int config_changed;


/* The load_config function is now going to call:
 * init_proxy_args, so it will need argc and argv.
 * Since load_config will also be a signal handler,
 * we need to have these globally available.
 */
extern int Argc;
extern const char **Argv;


extern void load_config( int );


/* Revision control strings from this header and associated .c file */
extern const char loadcfg_rcs[];
extern const char loadcfg_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _JCC_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
