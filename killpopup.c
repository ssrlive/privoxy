const char killpopup_rcs[] = "$Id: killpopup.c,v 1.3 2001/05/22 18:56:28 oes Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/killpopup.c,v $
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
 *    $Log: killpopup.c,v $
 *    Revision 1.3  2001/05/22 18:56:28  oes
 *    CRLF -> LF
 *
 *    Revision 1.2  2001/05/20 01:21:20  jongfoster
 *    Version 2.9.4 checkin.
 *    - Merged popupfile and cookiefile, and added control over PCRS
 *      filtering, in new "permissionsfile".
 *    - Implemented LOG_LEVEL_FATAL, so that if there is a configuration
 *      file error you now get a message box (in the Win32 GUI) rather
 *      than the program exiting with no explanation.
 *    - Made killpopup use the PCRS MIME-type checking and HTTP-header
 *      skipping.
 *    - Removed tabs from "config"
 *    - Moved duplicated url parsing code in "loaders.c" to a new funcition.
 *    - Bumped up version number.
 *
 *    Revision 1.1.1.1  2001/05/15 13:58:58  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "project.h"
#include "killpopup.h"
#include "jcc.h"

const char killpopup_h_rcs[] = KILLPOPUP_H_VERSION;

#ifdef KILLPOPUPS

/* Change these for debug output.  *lots*. */
/*#define POPUP_VERBOSE 1*/
#undef POPUP_VERBOSE


/*********************************************************************
 *
 * Function    :  filter_popups
 *
 * Description :  Filter the block of data that's been read from the server.
 *                Caller is responsible for checking permissons list
 *                to determine if this function should be called.
 *
 * Parameters  :
 *          1  :  buff = Buffer to scan and modify.  Null terminated.
 *          2  :  size = Buffer size, excluding null terminator.
 *
 * Returns     :  void
 *
 *********************************************************************/
void filter_popups(char *buff, int size)
{
   char *popup = NULL;
   char *close = NULL;
   char *p     = NULL;

   while ((popup = strstr( buff, "window.open(" )) != NULL)
   {
#ifdef POPUP_VERBOSE
      fprintf(logfp, "Found start of window open" );
#endif
      close = strstr( popup+1, ");" );
      if ( close )
      {
#ifdef POPUP_VERBOSE
         fprintf(logfp, "Found end of window open" );
#endif
         p = popup;
         *p++ = '1';
         for ( ; p != (close+1); p++ )
         {
            *p = ' ';
         }
#ifdef POPUP_VERBOSE
         fprintf(logfp, "Blocked %s\n", host_name );
#endif
      }
      else
      {
#ifdef POPUP_VERBOSE
         fprintf(logfp, "Couldn't find end, turned into comment.  Read boundary?\n" );
#endif
         *popup++ = '1';
         *popup++ = ';';
         *popup++ = '/';
         *popup = '/';
         /*
          * result of popup is assigned to variable and the rest commented out
          * window.open(blah
          *		will be translated to
          * 1;//ow.open(blah
          *		and
          * myWindow = window.open(blah
          *		will be translated to
          * myWindow = 1;//ow.open(blah
          */
      }
   }

   /* Filter all other crap like onUnload onExit etc.  (by BREITENB) NEW!*/
   popup=strstr( buff, "<body");
   if (!popup) popup=strstr( buff, "<BODY");
   if (!popup) popup=strstr( buff, "<Body");
   if (!popup) popup=strstr( buff, "<BOdy");
   if (popup)
   {
      close=strchr(popup,'>');
      if (close)
      {
         /* we are now between <body and the ending > FIXME: No, we're anywhere! --oes*/
         p=strstr(popup, "onUnload");
         if (p)
         {
            strncpy(p,"_nU_",4);
         }
         p=strstr(popup, "onExit");
         if (p)
         {
            strncpy(p,"_nE_",4);
         }
      }
   }

}

#endif /* def KILLPOPUPS */

/*
  Local Variables:
  tab-width: 3
  end:
*/
