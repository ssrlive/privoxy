/*********************************************************************
 *
 * File        :  $Source$
 *
 * Purpose     :  Utility string functions.
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
 * Utility string functions.
 * @author Last Modified By: $Author$
 * @version $Rev$-$Date$$State$
 */
class StringUtil
{
  private static final String
    COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  /****************************************************************************
  * This method replaces the preValue with the postValue in the originalString.
  *
  * @param originalString the string to have replacements
  * @param preValue The substring value that the string currently contains
  * @param postValue The substring value to replace the preValue
  * @param recursive true if replacements should occur recursively
  *
  * @return String representing the substituted value(s)
  ****************************************************************************/
  public static final String replaceSubstring( String originalString
                                               , String preValue
                                               , String postValue
                                               , boolean recursive
                                             )
  {
    String finalString = originalString;
    int previousFind = originalString.length();

    int index = originalString.lastIndexOf(preValue);
    while (index > -1        &&
           index < previousFind
          )
    {
      previousFind = index;

      finalString = originalString.substring(0,index)
                    + postValue
                    + finalString.substring(index+preValue.length());

      index = finalString.lastIndexOf(preValue,previousFind);
      if (!recursive)
      {
        if (index == previousFind)
          previousFind--;
        index = finalString.lastIndexOf(preValue,previousFind);
      }
    }

    return finalString;
  }

  /****************************************************************************
  * This method replaces the preValue with the postValue in the originalString.
  *
  * @param originalString the string to have replacements
  * @param preValue The substring value that the string currently contains
  * @param postValue The substring value to replace the preValue
  *
  * @return String representing the substituted value(s)
  ****************************************************************************/
  public static final String replaceSubstring( String originalString
                                               , String preValue
                                               , String postValue
                                             )
  {
    return(replaceSubstring(originalString, preValue, postValue, false));
  }
}
