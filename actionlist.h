/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/actionlist.h,v $
 *
 * Purpose     :  Master list of supported actions.
 *                Not really a header, since it generates code.
 *                This is included (3 times!) from actions.c
 *                Each time, the following macros are defined to
 *                suitable values beforehand:
 *                    DEFINE_ACTION_MULTI()
 *                    DEFINE_ACTION_STRING()
 *                    DEFINE_ACTION_BOOL()
 *                    DEFINE_ACTION_ALIAS
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
 *    $Log: actionlist.h,v $
 *    Revision 1.4  2001/07/13 13:52:12  oes
 *     - Formatting
 *     - Introduced new action ACTION_DEANIMATE
 *
 *    Revision 1.3  2001/06/07 23:03:56  jongfoster
 *    Added standard comment at top of file.
 *
 *
 *********************************************************************/


DEFINE_ACTION_MULTI ("add-header",      ACTION_MULTI_ADD_HEADER)
DEFINE_ACTION_BOOL  ("block",           ACTION_BLOCK)
DEFINE_ACTION_STRING("deanimate-gifs",  ACTION_DEANIMATE,       ACTION_STRING_DEANIMATE)
DEFINE_ACTION_BOOL  ("fast-redirects",  ACTION_FAST_REDIRECTS)
DEFINE_ACTION_BOOL  ("filter",          ACTION_FILTER)
DEFINE_ACTION_BOOL  ("hide-forwarded",  ACTION_HIDE_FORWARDED)
DEFINE_ACTION_STRING("hide-from",       ACTION_HIDE_FROM,       ACTION_STRING_FROM)
DEFINE_ACTION_STRING("hide-referer",    ACTION_HIDE_REFERER,    ACTION_STRING_REFERER)
DEFINE_ACTION_STRING("hide-user-agent", ACTION_HIDE_USER_AGENT, ACTION_STRING_USER_AGENT)
DEFINE_ACTION_BOOL  ("image",           ACTION_IMAGE)
DEFINE_ACTION_STRING("image-blocker",   ACTION_IMAGE_BLOCKER,   ACTION_STRING_IMAGE_BLOCKER)
DEFINE_ACTION_BOOL  ("no-cookies-read", ACTION_NO_COOKIE_READ)
DEFINE_ACTION_BOOL  ("no-cookies-set",  ACTION_NO_COOKIE_SET)
DEFINE_ACTION_BOOL  ("no-popups",       ACTION_NO_POPUPS)
DEFINE_ACTION_BOOL  ("vanilla-wafer",   ACTION_VANILLA_WAFER)
DEFINE_ACTION_MULTI ("wafer",           ACTION_MULTI_WAFER)
#if DEFINE_ACTION_ALIAS
DEFINE_ACTION_BOOL  ("no-popup",        ACTION_NO_POPUPS)
DEFINE_ACTION_STRING("hide-referrer",   ACTION_HIDE_REFERER,    ACTION_STRING_REFERER)
#endif /* if DEFINE_ACTION_ALIAS */
