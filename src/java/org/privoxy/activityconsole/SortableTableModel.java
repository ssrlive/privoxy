/*********************************************************************
 *
 * File        :  $Source$
 *
 * Purpose     :  Sorting JTable model.
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

import java.util.*;
import java.awt.*;
import javax.swing.*;
import javax.swing.table.*;

/** 
 * Sorting JTable table model.
 * @author Last Modified By: $Author$
 * @version $Rev$-$Date$$State$
 */
public class SortableTableModel extends DefaultTableModel
{

  private static final String
  COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  int[] indexes;
  TableSorter sorter;

  public SortableTableModel()
  {
  }

  public SortableTableModel(Vector data, Vector columnNames)
  {
    super(data, columnNames);
  }
  public Object getValueAt(int row, int col)
  {
    int rowIndex = row;
    if (indexes != null)
    {
      rowIndex = indexes[row];
    }
    return super.getValueAt(rowIndex, col);
  }

  public void setValueAt(Object value, int row, int col)
  {
    int rowIndex = row;
    if (indexes != null)
    {
      rowIndex = indexes[row];
    }
    super.setValueAt(value, rowIndex, col);
  }

  public void sortByColumn(int column, boolean isAscent)
  {
    if (sorter == null)
    {
      sorter = new TableSorter(this);
    }
    sorter.sort(column, isAscent);   
    fireTableDataChanged();
  }

  public int[] getIndexes()
  {
    int n = getRowCount();
    if (indexes != null)
    {
      if (indexes.length == n)
      {
        return indexes;
      }
    }
    indexes = new int[n];
    for (int i=0; i<n; i++)
    {
      indexes[i] = i;
    }
    return indexes;
  }
}
