#ifndef IPC_H_INCLUDED
#define IPC_H_INCLUDED
#define IPC_H_VERSION "$Id: ipc.h,v 2.0 2002/06/04 14:34:21 jongfoster Exp $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/src/ipc.h,v $
 *
 * Purpose     :  Functions to provide portable interprocess
 *                communications: semaphores, sleeping, etc.
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
 *    $Log: ipc.h,v $
 *
 *********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Here, we define common ways of defining mutual exclusion variables
 * and the manipulation thereof.  We define an alias for the type that
 * particular OSes like to see, and we have a common model of locking
 * and unlocking them.
 */

#ifdef _WIN32
  #include <windows.h>
  #define IPC_MUTEX_LOCK HANDLE
  #define IPC_CREATE_MUTEX(lock) InitializeCriticalSection(&lock)
  #define IPC_LOCK_MUTEX(lock) EnterCriticalSection(lock)
  #define IPC_UNLOCK_MUTEX(lock) LeaveCriticalSection(lock)
  #define IPC_SLEEP_SECONDS(seconds) Sleep(seconds * 1000)
#elif __OS2__
  #define INCL_DOSSEMAPHORES
  #define INCL_DOSPROCESS
  #include <os2.h>
  #define IPC_MUTEX_LOCK HMTX
  #define IPC_CREATE_MUTEX(lock) DosCreateMutexSem(NULL, &lock, 0, FALSE)
  #define IPC_LOCK_MUTEX(lock) DosRequestMutexSem(lock,SEM_INDEFINITE_WAIT)
  #define IPC_UNLOCK_MUTEX(lock) DosReleaseMutexSem(lock)
  #define IPC_SLEEP_SECONDS(seconds) DosSleep(seconds * 1000)
#else
  /* Generic unix processing.  This will probably need tweaking for variants. */
  #include <sys/signal.h>
  #include <pthread.h>
  #define IPC_MUTEX_LOCK pthread_mutex_t
  #define IPC_CREATE_MUTEX(lock) pthread_mutex_init(&lock,0)
  #define IPC_LOCK_MUTEX(lock) pthread_mutex_lock(&lock)
  #define IPC_UNLOCK_MUTEX(lock) pthread_mutex_unlock(&lock)
  #define IPC_SLEEP_SECONDS(seconds) sleep(seconds)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef IPC_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/
