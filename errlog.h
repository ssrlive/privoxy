#ifndef _ERRLOG_H
#define _ERRLOG_H
#define ERRLOG_H_VERSION "$Id: errlog.h,v 1.1 2001/05/13 21:57:06 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/errlog.h,v $
 *
 * Purpose     :  Log errors to a designated destination in an elegant,
 *                printf-like fashion.
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
 *    $Log: errlog.h,v $
 *
 *********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

/* Debug level for errors */

#define LOG_LEVEL_GPC     0x0001
#define LOG_LEVEL_CONNECT 0x0002
#define LOG_LEVEL_IO      0x0004
#define LOG_LEVEL_HEADER  0x0008
#define LOG_LEVEL_LOG     0x0010
#ifdef PCRS
#define LOG_LEVEL_FRC     0x0020
#define LOG_LEVEL_REF     0x0040
#endif /* def PCRS */

/* Following are always on: */
#define LOG_LEVEL_ERROR   0x1000
#define LOG_LEVEL_INFO    0x2000

extern void init_error_log(const char *prog_name, const char *logfname, int debuglevel);
extern void log_error(int loglevel, char *fmt, ...);

/* Revision control strings from this header and associated .c file */
extern const char errlog_rcs[];
extern const char errlog_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _ERRLOG_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/

