/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/src/java/org/privoxy/activityconsole/ActivityConsoleResources.java,v $
 *
 * Purpose     :  Default English text for all translatable strings.
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
 *    $Log: ActivityConsoleResources.java,v $
 *    Revision 1.1  2003/01/18 14:37:24  david__schmidt
 *    Initial checkin of directory structure and source code of the java Activity
 *    Console
 *
 *********************************************************************/

package org.privoxy.activityconsole;

import java.io.Serializable;

/**
 * The default (English) language resources file for the Activity Console.
 * @author Last Modified By: $Author: david__schmidt $
 * @version $Rev$-$Date: 2003/01/18 14:37:24 $$State: Exp $
 */
public class ActivityConsoleResources extends java.util.ListResourceBundle implements Serializable
{
  private static final String
    COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  static final Object[][] contents =
  {
    {"guiTitle", "Privoxy Activity Console - serving port %1"},
    {"menuFile", "File"}, 
    {"menuFileQuit", "Quit"}, 
    {"menuEdit", "Edit"},
    {"menuEditDelete", "Delete selected row"},
    {"menuEditConfig", "Set port"},
    {"menuView", "View"},
    {"menuViewWide", "Detailed statistics"},
    {"guiNewPortTitle", "New port"},
    {"guiNewPortPrompt", "Currently serving port %1.\n\nPlease enter the new port to serve:"},
    {"guiNewPortErrorTitle", "Alert"},
    {"guiNewPortErrorPrompt", "New port must be positive."},
    {"guiDeleteConfirmTitle","Are you sure?"},
    {"guiDeleteConfirmPrompt","Are you sure you want to delete stats from host %1?"},
    {"guiPropertiesFileHeader","Privoxy Activity Console properties file - edit only while the Activity Console is not running."},

    /* Headers and descriptions for statistics columns */
    {"guiDefaultColumn0","Host:Port"},
    {"guiDefaultColumn0Description","The host and port that statistics are coming from"},
    {"guiDefaultColumn1","Request"},
    {"guiDefaultColumn1Description","The number of requests that flow through the proxy"},
    {"guiDefaultColumn2","Filter"},
    {"guiDefaultColumn2Description","The number of filters that have been applied"},
    {"guiDefaultColumn3","Image"},
    {"guiDefaultColumn3Description","The number of images that have been blocked"},
    {"guiDefaultColumn4","De-anim"},
    {"guiDefaultColumn4Description","The number of GIF images that have been de-animated"},
    {"guiDefaultColumn5","Cookie"},
    {"guiDefaultColumn5Description","The number of cookies that have been blocked"},
    {"guiDefaultColumn6","Referer"},
    {"guiDefaultColumn6Description","The number of referers blocked"},
    {"guiDefaultColumn7","ACL"},
    {"guiDefaultColumn7Description","The number of blocks due to ACL restrictions"},
    {"guiDefaultColumn8","UA"},
    {"guiDefaultColumn8Description","The number of times user-agent header was removed"},
    {"guiDefaultColumn9","FROM"},
    {"guiDefaultColumn9Description","The number of times the from: header was removed"},
    {"guiDefaultColumn10","FORWARD"},
    {"guiDefaultColumn10Description","The number of times the forward header was removed"},
  };

  /** Returns the contents of this ListBundleResources class.
   * @see java.util.ListBundleResource
   * @return Object an Object array containing this classes resources.
   */
  protected Object[][] getContents()
  {
    return contents;
  }

  /** Returns the name of the language this class represents.
   * @return String the name of the language this class represents.
   */

  public String toString()
  {
    return "English";
  }
}
