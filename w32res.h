#ifndef _W32RES_H
#define _W32RES_H
#define W32RES_H_VERSION "$Id: w32res.h,v 1.1 2001/05/13 21:57:07 administrator Exp $"
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/w32res.h,v $
 *
 * Purpose     :  Identifiers for Windows GUI resources.
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
 *    $Log: w32res.h,v $
 *
 *********************************************************************/

#define IDS_NEW_BLOCKER                   1

#define ID_NEW_BLOCKER                    100
#define IDR_TRAYMENU                      101
#define IDI_IDLE                          102
#define IDR_LOGVIEW                       103
#define IDR_ACCELERATOR                   104
#define IDR_POPUP_SELECTION               105
#define IDD_RULES                         106
#define IDI_DENYRULE                      107
#define IDI_ALLOWRULE                     108

#define IDI_JUNKBUSTER                    200
#define IDI_JUNKBUSTER1                   201
#define IDI_JUNKBUSTER2                   202
#define IDI_JUNKBUSTER3                   203
#define IDI_JUNKBUSTER4                   204
#define IDI_JUNKBUSTER5                   205
#define IDI_JUNKBUSTER6                   206
#define IDI_JUNKBUSTER7                   207
#define IDI_JUNKBUSTER8                   208

#define IDC_NEW                           300
#define IDC_ACTION                        301
#define IDC_RULES                         302
#define IDC_CREATE                        303
#define IDC_MOVEUP                        304
#define IDC_MOVEDOWN                      305
#define IDC_DELETE                        306
#define IDC_SAVE                          307

#define ID_SHOWWINDOW                     4000
#define ID_HELP_ABOUTJUNKBUSTER           4001
#define ID_FILE_EXIT                      4002
#define ID_VIEW_CLEARLOG                  4003
#define ID_VIEW_LOGMESSAGES               4004
#define ID_VIEW_MESSAGEHIGHLIGHTING       4005
#define ID_VIEW_LIMITBUFFERSIZE           4006
#define ID_VIEW_ACTIVITYANIMATION         4007
#define ID_HELP_FAQ                       4008
#define ID_HELP_MANUAL                    4009
#define ID_HELP_GPL                       4010
#define ID_HELP_STATUS                    4011
#ifdef TOGGLE
#define ID_TOGGLE_IJB                     4012
#endif
#define ID_RELOAD_CONFIG                  4013

/* Break these out so they are easier to extend, but keep consecutive */
#define ID_TOOLS_EDITJUNKBUSTER           5000
#define ID_TOOLS_EDITBLOCKERS             5001
#define ID_TOOLS_EDITCOOKIES              5002
#define ID_TOOLS_EDITFORWARD              5003

#ifdef ACL_FILES
#define ID_TOOLS_EDITACLS                 5005
#endif /* def ACL_FILES */

#ifdef USE_IMAGE_LIST
#define ID_TOOLS_EDITIMAGE                5006
#endif /* def USE_IMAGE_LIST */

#ifdef KILLPOPUPS
#define ID_TOOLS_EDITPOPUPS               5007
#endif /* def KILLPOPUPS */

#ifdef PCRS
#define ID_TOOLS_EDITPERLRE               5008
#endif /* def PCRS */

#ifdef TRUST_FILES
#define ID_TOOLS_EDITTRUST                5004
#endif /* def TRUST_FILES */

/*
 * The following symbols are declared in <afxres.h> in VC++.
 * However, mingw32 doesn't have that header.  Let's 
 * always declare them here, for consistency.
 * These are the VC++ values.
 */
#define IDC_STATIC      (-1)
#define ID_EDIT_COPY  30000


#endif /* ndef _W32RES_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
