#ifndef _KILLPOPUP_H
#define _KILLPOPUP_H
#define KILLPOPUP_H_VERSION "$Id: killpopup.h,v 1.1 2001/05/13 21:57:06 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/killpopup.h,v $
 *
 * Purpose     :  Handles the filtering of popups.
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
 *    $Log: killpopup.h,v $
 *
 *********************************************************************/


#include "project.h"

#ifdef KILLPOPUPS

extern void filter_popups(struct client_state *csp, char *host_name, char *buff, int size);

#endif /* def KILLPOPUPS */

/* Revision control strings from this header and associated .c file */
extern const char killpopup_rcs[];
extern const char killpopup_h_rcs[];

#endif /* ndef _KILLPOPUP_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
