/*********************************************************************
 *
 * File        :  $Source$
 *
 * Purpose     :  Part of the StatWidget, the thread that manages the
 *                timing of the swatch of color.
 *
 * Copyright   :  Written by and Copyright (C) 2003 the SourceForge
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
 *    $Log$
 *********************************************************************/

package org.privoxy.activityconsole;

import  java.io.*;
import  java.util.*;

/** 
 * The thread that manages the timing of the swatch of color of the StatWidget.
 * @author Last Modified By: $Author$
 * @version $Rev$-$Date$$State$
 */
public class StatWidgetThread extends Thread
{
  private static final String
    COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  int _statusDurationMillis = 0;
  Date _nextUpdate = null;
  StatWidget _parent = null;

  public StatWidgetThread(StatWidget parent, int statusDurationMillis)
  {
    _parent = parent;
    _statusDurationMillis = statusDurationMillis;
  }

  public final void run()
  {
    boolean snooze = false;
    int sleepFor;

    while (1==1)
    {
      /* Check once before we wait at all, just in case... */
      if (_nextUpdate != null)
        snooze = checkTimer();
      if (snooze == true)
        sleepFor = _statusDurationMillis;
      else
        sleepFor = 540000 + (int)(Math.random() * 60000); /* Sit around for 10 minutes plus or minus a minute */
      try
      {
        sleep(sleepFor);
        if (_nextUpdate != null)
          snooze = checkTimer();
      }
      catch (Throwable t2)
      {
        pulse();
        snooze = true;
      }
    }
  }

  public void pulse()
  {
    _nextUpdate = new Date(new Date().getTime() + _statusDurationMillis);
  }

  public boolean checkTimer()
  {
    boolean returnVal = true;
    Date now = new Date();
    if (now.after(_nextUpdate))
    {
      _parent.timerPop();
      _nextUpdate = null;
      returnVal = false;
    }
    return returnVal;
  }
}
