#ifndef STATS_H_INCLUDED
#define STATS_H_INCLUDED
#define STATS_H_VERSION "$Id: stats.h,v 2.0 2002/06/04 14:34:21 jongfoster Exp $"
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
 *
 *********************************************************************/


/* Revision control strings from this header and associated .c file */
extern const char stats_rcs[];
extern const char stats_h_rcs[];

/* Global variables */

/* These are the different types of statistics we will be gathering. */
#define STATS_REQUEST 0
#define STATS_FILTER 1
#define STATS_GIF_DEANIMATE 2
#define STATS_ACL_RESTRICT 3
#define STATS_IMAGE_BLOCK 4
#define STATS_COOKIE 5
#define STATS_REFERER 6
#define STATS_CLIENT_UA 7
#define STATS_CLIENT_FROM 8
#define STATS_CLIENT_X_FORWARDED 9
/** Define the maximum number of 'keys' we'll be sending.  Always keep this
  * number one greater than the last actual key; it is used to define an 
  * array (i.e. int stats[STATS_MAX_KEYS]. */
#define STATS_MAX_KEYS 10

/* Functions */

void init_stats_config(struct configuration_spec * config);
void update_stats_config(struct configuration_spec * config);
void accumulate_stats(int key, int value);
void *forward_stats();
void send_stats(int *p_local_stats_array[]);

#endif /* ndef STATS_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/
