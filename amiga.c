const char amiga_rcs[] = "$Id: amiga.c,v 1.1 2001/05/13 21:57:06 administrator Exp $";
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/jcc.c,v $
 *
 * Purpose     :  Amiga-specific declarations.
 *
 * Copyright   :  Written by and Copyright (C) 2001 the SourceForge
 *                IJBSWA team.  http://ijbswa.sourceforge.net
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
 *    $Log: jcc.c,v $
 *
 *********************************************************************/


#include "config.h"

#ifdef AMIGA

#include <stdio.h>
#include <signal.h>

#include "amiga.h"

chonst char amiga_h_rcs[] = AMIGA_H_VERSION;

unsigned long __stack = 20*1024;
/* static char ver[] = "$VER: junkbuster " __AMIGAVERSION__ " (" __AMIGADATE__ ")"; */
struct Task *main_task = NULL;
int childs = 0;

void serve(struct client_state *csp);

__saveds ULONG server_thread(void)
{
   struct client_state *local_csp;
   struct UserData UserData;
   struct Task *me=FindTask(NULL);

   Wait(SIGF_SINGLE);
   local_csp=(struct client_state *)(me->tc_UserData);
   me->tc_UserData=&UserData;
   SocketBase=(APTR)OpenLibrary("bsdsocket.library",3);
   if(SocketBase)
   {
      SetErrnoPtr(&(UserData.eno),sizeof(int));
      local_csp->cfd=ObtainSocket(local_csp->cfd, AF_INET, SOCK_STREAM, 0);
      if(-1!=local_csp->cfd)
      {
         Signal(main_task,SIGF_SINGLE);
         serve((struct client_state *) local_csp);
      } else {
         local_csp->active = 0;
         Signal(main_task,SIGF_SINGLE);
      }
      CloseLibrary(SocketBase);
   } else {
      local_csp->active = 0;
      Signal(main_task,SIGF_SINGLE);
   }
   childs--;
   return 0;
}

void amiga_exit(void)
{
   if(SocketBase)
   {
      CloseLibrary(SocketBase);
   }
}

static struct SignalSemaphore memsem;
static struct SignalSemaphore *memsemptr = NULL;
static struct UserData GlobalUserData;

void InitAmiga(void)
{
   main_task = FindTask(NULL);
   main_task->tc_UserData = &GlobalUserData;

   if (((struct Library *)SysBase)->lib_Version < 39)
   {
      exit(RETURN_FAIL);
   }

   signal(SIGINT,SIG_IGN);
   SocketBase = (APTR)OpenLibrary("bsdsocket.library",3);
   if (!SocketBase)
   {
      fprintf(stderr, "Can't open bsdsocket.library V3+\n");
      exit(RETURN_ERROR);
   }
   SetErrnoPtr(&(GlobalUserData.eno),sizeof(int));
   InitSemaphore(&memsem);
   memsemptr = &memsem;

   atexit(amiga_exit);
}

#ifdef __GNUC__
#ifdef libnix
/* multitaskingsafe libnix replacements */
static void *memPool=NULL;

void *malloc (size_t s)
{
   ULONG *mem;
   LONG size = s;

   if (size<=0)
   {
      return NULL;
   }
   if (!memPool)
   {
      if (!(memPool=CreatePool(MEMF_ANY,32*1024,8*1024)))
      {
         return NULL;
      }
   }
   size += sizeof(ULONG) + MEM_BLOCKMASK;
   size &= ~MEM_BLOCKMASK;
   if (memsemptr)
   {
      ObtainSemaphore(memsemptr);
   }
   if ((mem=AllocPooled(memPool,size)))
   {
      *mem++=size;
   }
   if (memsemptr)
   {
      ReleaseSemaphore(memsemptr);
   }
   return mem;
}

void free (void *m)
{
   ULONG *mem = m;

   if(mem && memPool)
   {
      ULONG size=*--mem;

      if (memsemptr)
      {
         ObtainSemaphore(memsemptr);
      }
      FreePooled(memPool,mem,size);
      if (memsemptr)
      {
         ReleaseSemaphore(memsemptr);
      }
   }
}

void *realloc (void *old, size_t ns)
{
   void *new;
   LONG osize, *o = old;
   LONG nsize = ns;

   if (!old)
   {
      return malloc(nsize);
   }
   osize = (*(o-1)) - sizeof(ULONG);
   if (nsize <= osize)
   {
      return old;
   }
   if ((new = malloc(nsize)))
   {
      ULONG *n = new;

      osize >>= 2;
      while(osize--)
      {
         *n++ = *o++;
      }
      free(old);
   }
   return new;
}

void __memCleanUp (void)
{
   if (memsemptr)
   {
      ObtainSemaphore(memsemptr);
   }
   if (memPool)
   {
      DeletePool(memPool);
   }
   if (memsemptr)
   {
      ReleaseSemaphore(memsemptr);
   }
}

#define ADD2LIST(a,b,c) asm(".stabs \"_" #b "\"," #c ",0,0,_" #a )
#define ADD2EXIT(a,pri) ADD2LIST(a,__EXIT_LIST__,22); \
                        asm(".stabs \"___EXIT_LIST__\",20,0,0," #pri "+128")
ADD2EXIT(__memCleanUp,-50);
#elif !defined(ixemul)
#error No libnix and no ixemul!?
#endif /* libnix */
#else
#error Only GCC is supported, multitasking safe malloc/free required.
#endif /* __GNUC__ */

#endif /* def AMIGA */
