#ifndef STATS_H_INCLUDED
#define STATS_H_INCLUDED
#define STATS_H_VERSION "$Id: stats.h,v 2.3 2002/12/30 19:56:16 david__schmidt Exp $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/src/stats.h,v $
 *
 * Purpose     :  Functions and definitions for accumulating and
 *                sending statistics to an "external" stats console
 *
 * Copyright   :  Written by and Copyright (C) 2002, 2003 the SourceForge
 *                Privoxy team. http://www.privoxy.org/
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
 *    $Log: stats.h,v $
 *    Revision 2.3  2002/12/30 19:56:16  david__schmidt
 *    End of initial drop of statistics console infrastructure.  Data stream
 *    is transmitted on the stats port every interval, provided the data has
 *    changed since the last transmission.  More work probably needs to be
 *    done with regard to multiplatform threading; I stole the thread spawning
 *    code from jcc.c, but haven't been able to test it everywhere.
 *
 *    Revision 2.2  2002/12/28 04:17:58  david__schmidt
 *    Fix null_routine on unix
 *
 *    Revision 2.1  2002/12/28 03:58:19  david__schmidt
 *    Initial drop of dashboard instrumentation - enabled with
 *    --enable-activity-console
 *
 *
 *********************************************************************/


/* Revision control strings from this header and associated .c file */
extern const char stats_rcs[];
extern const char stats_h_rcs[];

/* Global variables */

/* These are the different types of statistics we will be gathering. */
#define STATS_PRIVOXY_PORT 0
#define STATS_REQUEST 1
#define STATS_FILTER 2
#define STATS_IMAGE_BLOCK 3
#define STATS_GIF_DEANIMATE 4
#define STATS_COOKIE 5
#define STATS_REFERER 6
#define STATS_ACL_RESTRICT 7
#define STATS_CLIENT_UA 8
#define STATS_CLIENT_FROM 9
#define STATS_CLIENT_X_FORWARDED 10
/** Define the maximum number of 'keys' we'll be sending.  Always keep this
  * number one greater than the last actual key; it is used to define an 
  * array (i.e. int stats[STATS_MAX_KEYS]. */
#define STATS_MAX_KEYS 11

/* Functions */

void init_stats_config(struct configuration_spec * config);
void update_stats_config(struct configuration_spec * config);
void accumulate_stats(int key, int value);
void *forward_stats();
void send_stats(int p_local_stats_array[]);
#ifdef unix
void null_routine(int sig);
#endif /* def unix */

/* Typedefs */

typedef struct
{
  int changed;
  int stats_array[STATS_MAX_KEYS];
  struct configuration_spec *config;
} stats_struct;

#endif /* ndef STATS_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/
