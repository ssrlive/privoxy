/*********************************************************************
 *
 * File        :  $Source$
 *
 * Purpose     :  Swing details of rendering a column header as a button.
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
 * Swing details of rendering a column header as a button.
 * @author Last Modified By: $Author$
 * @version $Rev$-$Date$$State$
 */
public class SortButtonRenderer extends JButton implements TableCellRenderer
{

  private static final String
  COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  public static final int NONE = 0;
  public static final int DOWN = 1;
  public static final int UP   = 2;

  int pushedColumn;
  Hashtable state;
  JButton downButton,upButton;

  public SortButtonRenderer()
  {
    pushedColumn   = -1;
    state = new Hashtable();

    setMargin(new Insets(0,0,0,0));
    setHorizontalTextPosition(LEFT);
    setIcon(new BlankIcon());

    downButton = new JButton();
    downButton.setMargin(new Insets(0,0,0,0));
    downButton.setHorizontalTextPosition(LEFT);
    downButton.setIcon(new BevelArrowIcon(BevelArrowIcon.DOWN, false, false));
    downButton.setPressedIcon(new BevelArrowIcon(BevelArrowIcon.DOWN, false, true));

    upButton = new JButton();
    upButton.setMargin(new Insets(0,0,0,0));
    upButton.setHorizontalTextPosition(LEFT);
    upButton.setIcon(new BevelArrowIcon(BevelArrowIcon.UP, false, false));
    upButton.setPressedIcon(new BevelArrowIcon(BevelArrowIcon.UP, false, true));

  }

  public Component getTableCellRendererComponent(JTable table, Object value,
                                                 boolean isSelected, boolean hasFocus, int row, int column)
  {
    JButton button = this;
    Object obj = state.get(new Integer(column));
    if (obj != null)
    {
      if (((Integer)obj).intValue() == DOWN)
      {
        button = downButton;
      }
      else
      {
        button = upButton;
      }
    }
    button.setText((value ==null) ? "" : value.toString());
    boolean isPressed = (column == pushedColumn);
    button.getModel().setPressed(isPressed);
    button.getModel().setArmed(isPressed);
    return button;
  }

  public void setPressedColumn(int col)
  {
    pushedColumn = col;
  }

  public void setSelectedColumn(int col)
  {
    if (col < 0) return;
    Integer value = null;
    Object obj = state.get(new Integer(col));
    if (obj == null)
    {
      value = new Integer(DOWN);
    }
    else
    {
      if (((Integer)obj).intValue() == DOWN)
      {
        value = new Integer(UP);
      }
      else
      {
        value = new Integer(DOWN);
      }
    }
    state.clear();
    state.put(new Integer(col), value);
  } 

  public int getState(int col)
  {
    int retValue;
    Object obj = state.get(new Integer(col));
    if (obj == null)
    {
      retValue = NONE;
    }
    else
    {
      if (((Integer)obj).intValue() == DOWN)
      {
        retValue = DOWN;
      }
      else
      {
        retValue = UP;
      }
    }
    return retValue;
  }
}
