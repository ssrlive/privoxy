const char win32_rcs[] = "$Id: win32.c,v 1.2 2001/07/29 19:32:00 jongfoster Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/win32.c,v $
 *
 * Purpose     :  Win32 User Interface initialization and message loop
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
 *    $Log: win32.c,v $
 *    Revision 1.2  2001/07/29 19:32:00  jongfoster
 *    Renaming _main() [mingw32 only] to real_main(), for ANSI compliance.
 *
 *    Revision 1.1.1.1  2001/05/15 13:59:08  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#include "config.h"

#ifdef _WIN32

#include <stdio.h>

#include "project.h"
#include "jcc.h"

/* Uncomment this if you want to build Win32 as a console app */
/* #define _WIN_CONSOLE */

#include <windows.h>

#include <stdarg.h>
#include <process.h>

#include "win32.h"

const char win32_h_rcs[] = WIN32_H_VERSION;

const char win32_blurb[] =
"Internet Junkbuster Proxy(TM) Version " VERSION " for Windows is Copyright (C) 1997-8\n"
"by Junkbusters Corp.  This is free software; it may be used and copied under\n"
"the GNU General Public License: http://www.gnu.org/copyleft/gpl.html .\n"
"This program comes with ABSOLUTELY NO WARRANTY OF ANY KIND.\n"
"\n"
"For information about how to to configure the proxy and your browser, see\n"
"        " REDIRECT_URL "win\n"
"\n"
"The Internet Junkbuster Proxy(TM) is running and ready to serve!\n"
"";

#ifdef _WIN_CONSOLE

int hideConsole     = 0;

#else

HINSTANCE g_hInstance;
int g_nCmdShow;

static void  __cdecl UserInterfaceThread(void *);

#endif


/*********************************************************************
 *
 * Function    :  WinMain
 *
 * Description :  M$ Windows "main" routine:
 *                parse the `lpCmdLine' param into main's argc and argv variables,
 *                start the user interface thread (for the systray window), and
 *                call main (i.e. patch execution into normal IJB startup).
 *
 * Parameters  :
 *          1  :  hInstance = instance handle of this IJB execution
 *          2  :  hPrevInstance = instance handle of previous IJB execution
 *          3  :  lpCmdLine = command line string which started IJB
 *          4  :  nCmdShow = window show value (MIN, MAX, NORMAL, etc...)
 *
 * Returns     :  `main' never returns, so WinMain will also never return.
 *
 *********************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   int argc = 0;
   int i;
   int res;
   const char **argv = NULL;
   char *pszArgs = NULL;
   char *pszLastTok;
   char szModule[MAX_PATH+1];
#ifndef _WIN_CONSOLE
   HANDLE hInitCompleteEvent = NULL;
#endif

   /* Split command line into arguments */
   pszArgs = (char *)malloc(strlen(lpCmdLine) + 1);
   strcpy(pszArgs, lpCmdLine);

   GetModuleFileName(hInstance, szModule, MAX_PATH);

   /* Count number of spaces */
   argc = 1;
   if (strlen(pszArgs) > 0)
   {
      pszLastTok = pszArgs;
      do
      {
         argc++;
         pszLastTok = strchr(pszLastTok+1, ' ');
      } while (pszLastTok);
   }

   /* Allocate array of strings */
   argv = (char **)malloc(sizeof(char *) * argc);

   /* step through command line replacing spaces with zeros, initialise array */
   argv[0] = szModule;
   i = 1;
   pszLastTok = pszArgs;
   do
   {
      argv[i] = pszLastTok;
      pszLastTok = strchr(pszLastTok+1, ' ');
      if (pszLastTok)
      {
         while (*pszLastTok != '\0' && *pszLastTok == ' ')
         {
            *pszLastTok = '\0';
            pszLastTok++;
         }
      }
      i++;
   } while (pszLastTok && *pszLastTok != '\0');

#ifndef _WIN_CONSOLE
   /* Create a user-interface thread and wait for it to initialise */
   hInitCompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   g_hInstance = hInstance;
   g_nCmdShow = nCmdShow;
   _beginthread(UserInterfaceThread, 0, &hInitCompleteEvent);
   WaitForSingleObject(hInitCompleteEvent, INFINITE);
   DeleteObject(hInitCompleteEvent);
#endif

#ifdef __MINGW32__
   res = real_main( argc, argv );
#else
   res = main( argc, argv );
#endif

   /* Cleanup */
   free(argv);
   free(pszArgs);

   return res;

}

#endif

/*********************************************************************
 *
 * Function    :  InitWin32
 *
 * Description :  Initialise windows, setting up the console or windows as appropriate.
 *
 * Parameters  :  None
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void InitWin32(void)
{
   WORD wVersionRequested;
   WSADATA wsaData;

#ifdef _WIN_CONSOLE
   SetProcessShutdownParameters(0x100, SHUTDOWN_NORETRY);
   if (hideConsole)
   {
      FreeConsole();
   }
#endif
   wVersionRequested = MAKEWORD(2, 0);
   if (WSAStartup(wVersionRequested, &wsaData) != 0)
   {
#ifndef _WIN_CONSOLE
      MessageBox(NULL, "Cannot initialize WinSock library", "Internet JunkBuster Error", 
         MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST);  
#endif
      exit(1);
   }

}


#ifndef _WIN_CONSOLE
#include <signal.h>
#include <assert.h>

#include "win32.h"
#include "w32log.h"


/*********************************************************************
 *
 * Function    :  UserInterfaceThread
 *
 * Description :  User interface thread.  WinMain will wait for us to set
 *                the hInitCompleteEvent before patching over to `main'.
 *                This ensures the systray window is active before beginning
 *                IJB operations.
 *
 * Parameters  :
 *          1  :  pData = pointer to `hInitCompleteEvent'.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void __cdecl UserInterfaceThread(void *pData)
{
   MSG msg;
   HANDLE hInitCompleteEvent = *((HANDLE *) pData);

   /* Initialise */
   InitLogWindow();
   SetEvent(hInitCompleteEvent);

   /* Enter a message processing loop */
   while (GetMessage(&msg, (HWND) NULL, 0, 0))
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   /* Cleanup */
   TermLogWindow();

   /* Time to die... */
   raise(SIGINT);

}


#endif


/*
  Local Variables:
  tab-width: 3
  end:
*/
