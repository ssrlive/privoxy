#ifndef W32RULESDLG_H_INCLUDED
#define W32RULESDLG_H_INCLUDED
#define W32RULESDLG_H_VERSION "$Id: w32rulesdlg.h,v 1.2 2001/05/26 01:26:34 jongfoster Exp $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/w32rulesdlg.h,v $
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
 *    Revision 1.2  2001/05/26 01:26:34  jongfoster
 *    New #define, WIN_GUI_EDIT, enables the (embryonic) Win32 GUI editor.
 *    This #define cannot be set from ./configure - there's no point, it
 *    doesn't work yet.  See feature request # 425722
 *
 *    Revision 1.1.1.1  2001/05/15 13:59:08  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN_CONSOLE /* entire file */
#ifdef WIN_GUI_EDIT /* entire file */

extern int ShowRulesDialog(HWND hwndParent);
extern void SetDefaultRule(const char *pszRule);

#endif /* def WIN_GUI_EDIT - entire file */
#endif /* ndef _WIN_CONSOLE - entire file */

/* Revision control strings from this header and associated .c file */
extern const char w32rulesdlg_rcs[];
extern const char w32rulesdlg_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef W32RULESDLG_H_INCLUDED */


/*
  Local Variables:
  tab-width: 3
  end:
*/
