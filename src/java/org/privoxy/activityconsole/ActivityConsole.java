/*********************************************************************
 *
 * File        :  $Source$
 *
 * Purpose     :  Launch the Activity Console GUI with either the 
 *                specified listen port or the default if none is 
 *                specified on the command line.
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

/**
 * Launch the Activity Console GUI with either the specified listen port
 * or the default if none is specified on the command line.
 * @author Last Modified By: $Author$
 * @version $Rev$-$Date$$State$
 */
public class ActivityConsole
{
  private static final String
    COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  /**
   * main method; initializes the GUI.
   *
   * @String[] args - command line parameters.
   */
  public static void main(java.lang.String[] args)
  {
    try
    {
      ActivityConsoleGui gui = new ActivityConsoleGui(args[0]);
    }
    catch (Throwable t)
    {
      ActivityConsoleGui gui = new ActivityConsoleGui("8119");
    }
  }
}
