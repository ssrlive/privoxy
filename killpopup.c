const char killpopup_rcs[] = "$Id: killpopup.c,v 1.1 2001/05/13 21:57:06 administrator Exp $";
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/killpopup.c,v $
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
/* CHANGED - added the below and shifted the more spammy stuff into it ;-) */
#undef POPUP_VERY_VERBOSE
#undef POPUP_VERBOSE


/*********************************************************************
 *
 * Function    :  filter_popups
 *
 * Description :  Filter the block of data that's been read from the server.
 *                IF NECESSARY.
 *
 * Parameters  :
 *          1  :  csp = Client state
 *          2  :  host_name = hostname of originating web page to
 *                look up on blocklist
 *          3  :  buff = Buffer to scan and modify.  Null terminated.
 *          4  :  size = Buffer size, excluding null terminator.
 *
 * Returns     :  void
 *
 *********************************************************************/
void filter_popups(struct client_state *csp, char *host_name, char *buff, int size)
{
   struct popup_settings * data;
   struct popup_blocklist * cur;
   int i;
   int found = 0;
   char *popup = NULL;
   char *close = NULL;
   char *p     = NULL;
   char *q     = NULL; /* by BREITENB NEW! */

   if ( (!csp->plist) || ((data = csp->plist->f) == NULL) )
   {
      /* Disabled. */
      return;
   }

   /* If the hostname is on our list for blocking then mark it
    * as a host to   block from.  (This may be later changed if the
    * host is also on the list-to-allow list).
    */

   for (i=0; (i < 50) && (i < size); i++)   /* avoid scanning binary data! */
   {
      if ((unsigned int)(buff[i])>127)
      {
#ifdef  POPUP_VERBOSE
         fprintf(logfp, "I'm not scanning binary stuff! (%i)\n",buff[i]);
#endif
         return;
      }
   }


   for (cur = data->blocked ; cur ; cur = cur->next)
   {
      if ( host_name != 0 )
      {
         if ( strcmp( cur->host_name, host_name ) == 0 )
         {
#ifdef  POPUP_VERBOSE
            fprintf(logfp, "Blocking %s\n", host_name );
#endif
            found = 1;
         }
      }
   }

   /* Force match if we're supposed to nuke _all_ popups, globally. */
   if ( kill_all_popups != 0 )
   {
#ifdef POPUP_VERBOSE
      fprintf(logfp, "Indescriminatly nuking popups..\n" );
#endif
      found = 1;
   }
   /* an exception-from blocking should still be an exception! by BREITENB NEW! */


   /*    Now, if its allowed adjust the filtering, so it _doesn't_ happen. */
   for (cur = data->allowed ; cur ; cur = cur->next)
   {
      if ( host_name != 0 )
      {
         if ( strcmp( cur->host_name, host_name ) == 0 )
         {
#ifdef POPUP_VERBOSE
            fprintf(logfp, "Reversing block decision for %s\n", host_name );
#endif
            found = 0;
         }
      }
   }

   if ( found == 0)
   {
#ifdef POPUP_VERBOSE
      fprintf(logfp, "Allowing %s\n", host_name );
#endif
      return;
   }

   while ((popup = strstr( buff, "window.open(" )) != NULL)
      /* if ( popup  )  by BREITENB filter ALL popups! NEW! */
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
         for ( p = popup; p != (close+1); p++ )
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
         *popup = '/';
         popup++;
         *popup = '/';
      }


      q=popup; /* by BREITENB NEW! */
      while (q>=buff)
      {
         if (*q==' ' || *q=='\t')
            q--;
         else break;
      }
      if (q>=buff)
      {
         if (*q=='=') *++q='1';
         /* result of popup is assigned to a variable! ensure success. hehehe. */
      }
   }

   /* Filter all other crap like onUnload onExit etc.  (by BREITENB) NEW!*/
   popup=strstr( buff, "<body");
   if (!popup) popup=strstr( buff, "<BODY");
   if (!popup) popup=strstr( buff, "<Body");
   if (!popup) popup=strstr( buff, "<BOdy");
   if (popup)
   {
      q=strchr(popup,'>');
      if (q)
      {
         /* we are now between <body and the ending > */
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
