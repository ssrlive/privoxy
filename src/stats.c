const char stats_rcs[] = "$Id: stats.c,v 2.3 2002/07/18 22:06:12 jongfoster Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/src/jcc.c,v $
 *
 * Purpose     :  
 *                
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
 *********************************************************************/


#ifdef unix
#include <sys/signal.h>
#endif
#include "project.h"
#include "errlog.h"
#include "stats.h"
#include "ipc.h"

const char stats_h_rcs[] = STATS_H_VERSION;
const char ipc_h_rcs[] = IPC_H_VERSION;
static IPC_MUTEX_LOCK stats_lock;

struct configuration_spec *latest_config;
int changed = 0,
    stats_array[STATS_MAX_KEYS];

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
  int i;
#if defined (_WIN32) || defined (__OS2__)
  int child_id;
#else
  pthread_attr_t attr;
  pthread_t thread;
#endif /* def unix */

  log_error(LOG_LEVEL_INFO, "init_stats_config hit.");
  IPC_CREATE_MUTEX(stats_lock);
  for (i=0; i < STATS_MAX_KEYS; i++)
  {
    stats_array[i] = 0;
  }
  latest_config = config;

  /*
   * Start the timing/sending thread - we'll need a lot of work here
   * for each platform.  I imagine there is also a possibility of 
   * doing this via fork() instead of threads.
   */

#ifdef _WIN32
    child_id = _beginthread(
            (void (*)(void *))forward_stats,
            64 * 1024,
            NULL);
#elif __OS2__
    child_id = _beginthread(
            (void(* _Optlink)(void*))forward_stats,
            NULL,
            64 * 1024,
            NULL);
#else
    /* Generic unix processing */
    signal(SIGALRM, null_routine);  /* Ignore the SIGALRM signal */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread, &attr, forward_stats, NULL);
#endif

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
  latest_config = config;
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
    stats_array[key] += value;
    changed = 1;
    IPC_UNLOCK_MUTEX(stats_lock);
  }
  /* log_error(LOG_LEVEL_INFO, "Accumulate stats: key %d, value %d, total %d; send to: %s:%d", key, value, stats_array[key], latest_config->activity_address,latest_config->activity_port); */
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
void *forward_stats()
{
  int local_stats_array[STATS_MAX_KEYS];
  
  log_error(LOG_LEVEL_INFO, "forward_stats ready.");
  for (;;)
  {
    IPC_SLEEP_SECONDS(latest_config->activity_freq);
    if (changed == 1)
    {
      IPC_LOCK_MUTEX(stats_lock);
      memcpy(local_stats_array,stats_array,sizeof(stats_array));
      changed = 0;
      IPC_UNLOCK_MUTEX(stats_lock);
      send_stats(&local_stats_array);
    }
  }
}

/*********************************************************************
 *
 * Function    :  send_stats
 *
 * Description :  Attempt to send statistics to the listening console
 *
 * Parameters  :  N/A
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void send_stats(int *local_stats_array[])
{
  /* Here, we initiate the socket send to the console */
  /*
  log_error(LOG_LEVEL_INFO, "send_stats sending stats: %d %d %d %d %d %d %d %d %d %d",
    local_stats_array[0],local_stats_array[1],local_stats_array[2],local_stats_array[3],local_stats_array[4],local_stats_array[5],local_stats_array[6],local_stats_array[7],local_stats_array[8],local_stats_array[9]);
  */
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
