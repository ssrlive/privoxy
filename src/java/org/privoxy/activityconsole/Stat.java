/*********************************************************************
 *
 * File        :  $Source$
 *
 * Purpose     :  A structification of a statistic element: a key
 *                associated with a value.
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
 * A structification of a statistic element: a key associated with a value.
 * @author Last Modified By: $Author$
 * @version $Rev$-$Date$$State$
 */
public class Stat
{
  private static final String
    COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  int _key = -1, _value = 0;

  public Stat(int key, int value)
  {
    _key = key;
    _value = value;
  }

  public int getKey()
  {
    return _key;
  }

  public int getValue()
  {
    return _value;
  }
}
