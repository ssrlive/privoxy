/*********************************************************************
 *
 * File        :  $Source$
 *
 * Purpose     :  A graphical element (specifically, a JPanel) that 
 *                displays a little box of color and a number.  When
 *                tweaked with a new number, the color changes for a
 *                set period of time, then resets to the previous 
 *                color.
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
import javax.swing.*;
import javax.swing.table.*;

/** 
 * A graphical element that displays a little box of color and a number.
 * @author Last Modified By: $Author$
 * @version $Rev$-$Date$$State$
 */
public class StatWidget extends JPanel implements Comparable
{
  private static final String
    COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  private JPanel _statusIcon;
  private JLabel _valueLabel;
  private int _status;
  private int _value;
  private int _row, _col;
  private JTable _table = null;
  private StatWidgetThread _thread;
  Color activityColor = Color.yellow;
  Color inactivityColor = getBackground().darker();

  public StatWidget(int initialValue, int statusDurationMillis)
  {
    _value = initialValue;
    _statusIcon = new JPanel();
    _statusIcon.setBackground(inactivityColor);
    _valueLabel = new JLabel(new Integer(initialValue).toString());
    this.setLayout(new GridBagLayout());
    ActivityConsoleGuiUtil.constrain(this, _statusIcon,
                              1, 1, // X, Y Coordinates
                              1, 1, // Grid width, height
                              GridBagConstraints.NONE,  // Fill value
                              GridBagConstraints.WEST,  // Anchor value
                              0.0,0.0,  // Weight X, Y
                              0, 1, 0, 1 ); // Top, left, bottom, right insets
    ActivityConsoleGuiUtil.constrain(this, _valueLabel,
                              2, 1, // X, Y Coordinates
                              1, 1, // Grid width, height
                              GridBagConstraints.HORIZONTAL,  // Fill value
                              GridBagConstraints.WEST,  // Anchor value
                              1.0,0.0,  // Weight X, Y
                              0, 1, 0, 1 ); // Top, left, bottom, right insets
    _thread = new StatWidgetThread(this, statusDurationMillis);
    _thread.start();
  }

  public void updateValue(int newValue)
  {
    if (_value != newValue)
    {
      _value = newValue;
      _valueLabel.setText(new Integer(newValue).toString());
      _statusIcon.setBackground(activityColor);
      if (_table != null)
        ((AbstractTableModel)_table.getModel()).fireTableCellUpdated(_row,_table.convertColumnIndexToModel(_col));
      _thread.interrupt();
    }
  }

  public void setRowColTable(int row, int col, JTable table)
  {
    _row = row;
    _col = col;
    _table = table;
  }

  public int getRow()
  {
    return _row;
  }

  public int getCol()
  {
    return _col;
  }

  public int getValue()
  {
    return _value;
  }

  public void timerPop()
  {
    _statusIcon.setBackground(inactivityColor);
    if (_table != null)
      ((AbstractTableModel)_table.getModel()).fireTableCellUpdated(_row,_table.convertColumnIndexToModel(_col));
  }

  public int compareTo(Object compare)
  {
    int compareValue = ((StatWidget)compare).getValue();
    if (_value < compareValue)
      return 1;
    else if (_value == compareValue)
      return 0;
    else
      return -1;
  }
}
