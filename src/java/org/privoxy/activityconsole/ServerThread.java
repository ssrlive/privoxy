/*********************************************************************
 *
 * File        :  $Source$
 *
 * Purpose     :  Listen on a specified port for status updates from
 *                Privoxy.  If we get a suitable update, pass it along
 *                to the GUI for processing.  We need to handle getting
 *                shut down and restarting on another port gracefully.
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

import java.net.*;
import java.io.*;
import java.text.*;
import java.util.*;
import javax.swing.*;

/** 
 * Listens on a specified port for status updates from Privoxy. 
 * @author Last Modified By: $Author$
 * @version $Rev$-$Date$$State$
 */
public class ServerThread extends Thread
{
  private static final String
    COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  static ActivityConsoleGui _parent;
  static int _port;
  static ServerSocket _serverSocket;

  public ServerThread(ActivityConsoleGui parent, int thePort)
  {
    _parent = parent;
    _port = thePort;
  }

  public void run()
  {
    try
    {
      _serverSocket = new ServerSocket(_port);        
      try
      {
        // System.out.println( "ServerThread serving port "+_port+"." );
        boolean shouldRun = true;
        while (shouldRun)
        {
          Socket theSocket = _serverSocket.accept();
          if (!Thread.currentThread().interrupted())
          {
            BufferedReader in = 
            new BufferedReader(new InputStreamReader(theSocket.getInputStream()));
            String line = in.readLine();
            /* Ensure the line isn't null and it's not way, way too long... */
            if ((line != null) && (line.length() < 65536))
            {
              _parent.updateStats(line,theSocket.getInetAddress().getHostName());
            }
            in.close();
            theSocket.close();
          }
          else
          {
            shouldRun = false;
          }
        }
        _serverSocket.close();
        _serverSocket = null;
      }
      catch (IOException io)
      {
        try
        {
          _serverSocket.close();
          _serverSocket = null;
        }
        catch (IOException fred)
        {
          _serverSocket = null;
        }
      }
    }
    catch (IOException io)
    {
      System.err.println(io);
      JOptionPane.showMessageDialog(null, io, "Alert: port "+_port, JOptionPane.ERROR_MESSAGE);
    }
  }

  public void doClose()
  {
    try
    {
      _serverSocket.close();
    }
    catch (IOException fred)
    {
    }
  }
}
