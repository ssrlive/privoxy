/*********************************************************************
 *
 * File        :  $Source$
 *
 * Purpose     :  Sorts JTable rows.
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

import java.awt.*;
import java.util.*;
import javax.swing.*;
import javax.swing.table.*;


/** 
 * Sorts JTable rows.
 * @author Last Modified By: $Author$
 * @version $Rev$-$Date$$State$
 */
public class TableSorter
{
  private static final String
  COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  SortableTableModel model;

  public TableSorter(SortableTableModel model)
  {
    this.model = model;
  }


  //n2 selection
  public void sort(int column, boolean isAscent)
  {
    int n = model.getRowCount();
    int[] indexes = model.getIndexes();   

    for (int i=0; i<n-1; i++)
    {
      int k = i;
      for (int j=i+1; j<n; j++)
      {
        if (isAscent)
        {
          if (compare(column, j, k) < 0)
          {
            k = j;
          }
        }
        else
        {
          if (compare(column, j, k) > 0)
          {
            k = j;
          }
        }
      }
      int tmp = indexes[i];
      indexes[i] = indexes[k];
      indexes[k] = tmp;
    }
  }


  // comparators
  public int compare(int column, int row1, int row2)
  {
    Object o1 = model.getValueAt(row1, column);
    Object o2 = model.getValueAt(row2, column); 
    if (o1 == null && o2 == null)
    {
      return  0; 
    }
    else if (o1 == null)
    {
      return -1; 
    }
    else if (o2 == null)
    {
      return  1; 
    }
    else
    {
      Class type = model.getColumnClass(column);
      if (type.getSuperclass() == Number.class)
      {
        return compare((Number)o1, (Number)o2);
      }
      else if (type == String.class)
      {
        return((String)o1).compareTo((String)o2);
      }
      else if (type == Date.class)
      {
        return compare((Date)o1, (Date)o2);
      }
      else if (type == Boolean.class)
      {
        return compare((Boolean)o1, (Boolean)o2);
      }
      else if (o1 instanceof StatWidget)
      {
        return((StatWidget)o1).compareTo((StatWidget)o2);
      }
      else
      {
        return((String)o1).compareTo((String)o2);
      }      
    }
  }

  public int compare(Number o1, Number o2)
  {
    double n1 = o1.doubleValue();
    double n2 = o2.doubleValue();
    if (n1 < n2)
    {
      return -1;
    }
    else if (n1 > n2)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }

  public int compare(Date o1, Date o2)
  {
    long n1 = o1.getTime();
    long n2 = o2.getTime();
    if (n1 < n2)
    {
      return -1;
    }
    else if (n1 > n2)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }

  public int compare(Boolean o1, Boolean o2)
  {
    boolean b1 = o1.booleanValue();
    boolean b2 = o2.booleanValue();
    if (b1 == b2)
    {
      return 0;
    }
    else if (b1)
    {
      return 1;
    }
    else
    {
      return -1;
    }
  }
}