const char stats_rcs[] = "$Id: stats.c,v 2.4 2003/01/06 02:03:13 david__schmidt Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/src/stats.c,v $
 *
 * Purpose     :  Functions and definitions for accumulating and
 *                sending statistics to an "external" stats console
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
 *    $Log: stats.c,v $
 *    Revision 2.4  2003/01/06 02:03:13  david__schmidt
 *    Update stats protocol now that the console is actually running
 *
 *
 *********************************************************************/


#include <string.h>
#ifdef unix
#include <sys/signal.h>
#include <unistd.h>
#include <pthread.h>
#endif
#include "project.h"
#include "errlog.h"
#include "miscutil.h"
#include "stats.h"
#include "ipc.h"
#include "jbsockets.h"

const char stats_h_rcs[] = STATS_H_VERSION;
const char ipc_h_rcs[] = IPC_H_VERSION;
static IPC_MUTEX_LOCK stats_lock;

stats_struct *main_stats;

/*********************************************************************
 *
 * Function    :  init_stats_config
 *
 * Description :  Initializes the statistics array and spawns a thread
 *                to transmit statistics to the listening party.
 *
 * Parameters  :
 *          1  :  config = Privoxy configuration.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void init_stats_config(struct configuration_spec * config)
{
  int i, child_id;
#ifdef unix
  pthread_attr_t attr;
  pthread_t thread;
#endif /* def unix */

  main_stats = zalloc(sizeof(stats_struct));
  IPC_CREATE_MUTEX(stats_lock);
  for (i=0; i < STATS_MAX_KEYS; i++)
  {
    main_stats->stats_array[i] = 0;
  }
  main_stats->config = config;

  accumulate_stats(STATS_PRIVOXY_PORT, config->hport);

  /*
   * Start the timing/sending thread - I stole this from jcc.c. 
   * The idea is to get a mutiplatform thread started.
   * YMMV - please tweak for your platform!
   */

/* this is a switch () statment in the C preprocessor - ugh */
#undef SELECTED_ONE_OPTION

/* Use pthreads in preference to any native code */
#if defined(FEATURE_PTHREAD) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
  signal(SIGALRM, null_routine);  /* Ignore the SIGALRM signal */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  child_id = (pthread_create(&thread, &attr,
    (void*)forward_stats, main_stats) ? -1 : 0);
  pthread_attr_destroy(&attr);
#endif

#if defined(_WIN32) && !defined(_CYGWIN) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
  child_id = _beginthread(
    (void (*)(void *))forward_stats,
    64 * 1024,
    main_stats);
#endif

#if defined(__OS2__) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
  child_id = _beginthread(
    (void(* _Optlink)(void*))forward_stats,
    NULL,
    64 * 1024,
    main_stats);
#endif

#if defined(__BEOS__) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
  thread_id tid = spawn_thread
    (server_thread, "forward_stats", B_NORMAL_PRIORITY, NULL);
  if ((tid >= 0) && (resume_thread(tid) == B_OK))
  {
    child_id = (int) tid;
  }
  else
  {
    child_id = -1;
  }
#endif

#if defined(AMIGA) && !defined(SELECTED_ONE_OPTION)
#define SELECTED_ONE_OPTION
  if((child_id = (int)CreateNewProcTags(
     NP_Entry, (ULONG)server_thread,
     NP_Output, Output(),
     NP_CloseOutput, FALSE,
     NP_Name, (ULONG)"privoxy child",
     NP_StackSize, 200*1024,
     TAG_DONE)))
  {
     childs++;
     Signal((struct Task *)child_id, SIGF_SINGLE);
     Wait(SIGF_SINGLE);
  }
#endif

#if !defined(SELECTED_ONE_OPTION)
  /* I don't think the IPC will really work in a fork()'d environment,
   * so proceed with caution.  FIXME.
   */
#error FIXME - stats will not work without pthreads!
  child_id = fork();

  if (child_id == 0)   /* child */
  {
     forward_stats(main_stats);
     _exit(0);
  }
  else if (child_id > 0) /* parent */
  {
  }
#endif

#undef SELECTED_ONE_OPTION
/* end of c preprocessor switch () */

}

/*********************************************************************
 *
 * Function    :  update_stats_config
 *
 * Description :  Updates the pointer to the most recent Privoxy
 *                configuration changes.
 *
 * Parameters  :
 *          1  :  config = Privoxy configuration.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void update_stats_config(struct configuration_spec * config)
{
  main_stats->config = config;
}

/*********************************************************************
 *
 * Function    :  accumulate_stats
 *
 * Description :  Updates one element of the statistics array with a
 *                single integer value.
 *
 * Parameters  :
 *          1  :  key = the key into the stats array
 *          2  :  value = the value to add to the current stats key
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void accumulate_stats(int key, int value)
{
  if (key < STATS_MAX_KEYS)
  {
    IPC_LOCK_MUTEX(stats_lock);
    main_stats->stats_array[key] += value;
    main_stats->changed = 1;
    IPC_UNLOCK_MUTEX(stats_lock);
  }
}

/*********************************************************************
 *
 * Function    :  forward_stats
 *
 * Description :  Main routine for the statistics thread; loops and 
 *                periodically checks if there's anything to send.  If
 *                so, call send_stats() to do the work.
 *
 * Parameters  :  N/A
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void *forward_stats(stats_struct *pstats)
{
  int local_stats_array[STATS_MAX_KEYS];
 
  for (;;)
  {
    IPC_SLEEP_SECONDS(pstats->config->activity_freq);
    if (pstats->changed == 1)
    {
      IPC_LOCK_MUTEX(stats_lock);
      memcpy(local_stats_array,pstats->stats_array,sizeof(pstats->stats_array));
      pstats->changed = 0;
      IPC_UNLOCK_MUTEX(stats_lock);
      send_stats(local_stats_array);
    }
  }
}

/*********************************************************************
 *
 * Function    :  send_stats
 *
 * Description :  Attempt to send statistics to the listening console.
 *                Stats are formatted as a clear-text string for now -
 *                no need for any encoding fanciness just yet.
 *
 * Parameters  :
 *          1  :  local_stats_array, a pointer to a local copy of the
 *                statistics array.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void send_stats(int local_stats_array[])
{
  jb_socket sk;
  char *msg = NULL, tmp_msg[64];
  int i;

  /* Here, we initiate the socket send to the console */
  sk = connect_to(main_stats->config->activity_address,main_stats->config->activity_port,NULL);
  if (sk > 0)
  {
    /* max size of a key looks like this: xxxxx:xxxxxb */
    msg = zalloc(
      STATS_MAX_KEYS * 64  /* Space for keys - much bigger than necessary for safety */
      );
    if (msg)
    {
      for (i = 0; i < STATS_MAX_KEYS; i++)
      {
        sprintf(tmp_msg,"%d:%d ",i,local_stats_array[i]);
        strcat(msg,tmp_msg);
      }
      write_socket(sk,msg,strlen(msg));
      freez(msg);
    }
    close_socket(sk);
  }
}

/*********************************************************************
 *
 * Function    :  null_routine
 *
 * Description :  Called when hit by a signal in unix; do nothing.
 *
 * Parameters  :
 *          1  :  sig - the integer signal
 *
 * Returns     :  N/A
 *
 *********************************************************************/
#ifdef unix
void null_routine(int sig)
{
  /* sigignore(sig); */
}
#endif /* def unix */
