#ifndef _W32RULESDLG_H
#define _W32RULESDLG_H
#define W32RULESDLG_H_VERSION "$Id: w32rulesdlg.h,v 1.1 2001/05/13 21:57:07 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/w32rulesdlg.h,v $
 *
 * Purpose     :  A dialog to allow GUI editing of the rules.
 *                Unfinished.
 *
 * Copyright   :  Written by and Copyright (C) 2001 the SourceForge
 *                IJBSWA team.  http://ijbswa.sourceforge.net
 *
 *                Written by and Copyright (C) 1999 Adam Lock
 *                <locka@iol.ie>
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
 *    $Log: w32rulesdlg.h,v $
 *
 *********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

extern int ShowRulesDialog(HWND hwndParent);
extern void SetDefaultRule(const char *pszRule);

/* Revision control strings from this header and associated .c file */
extern const char w32rulesdlg_rcs[];
extern const char w32rulesdlg_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _W32RULESDLG_H */


/*
  Local Variables:
  tab-width: 3
  end:
*/
