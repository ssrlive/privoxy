/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/src/java/org/privoxy/activityconsole/ActivityConsoleGui.java,v $
 *
 * Purpose     :  Provide the central GUI for displaying Privoxy
 *                statistics.  It can be contacted either by the
 *                local machine or other machines in a network and
 *                display consolidated, tabular statistics.
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
 *    $Log: ActivityConsoleGui.java,v $
 *    Revision 1.1  2003/01/18 14:37:24  david__schmidt
 *    Initial checkin of directory structure and source code of the java Activity
 *    Console
 *
 *********************************************************************/

package org.privoxy.activityconsole;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.*;
import javax.swing.table.*;

/**
 * The main Activity Console GUI.
 * @author Last Modified By: $Author: david__schmidt $
 * @version $Rev$-$Date: 2003/01/18 14:37:24 $$State: Exp $
 */
public final class ActivityConsoleGui extends JFrame implements ActionListener
{
  private static final String
  COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  ActivityConsoleGui parent_;
  ServerThread _serverThread = null;
  private ListResourceBundle resStrings = (ListResourceBundle)ListResourceBundle.getBundle("org.privoxy.activityconsole.ActivityConsoleResources");

  JTable _table;

  JScrollPane _tableScroller = new JScrollPane();

  SortableTableModel _model;

  Vector _tableColumnMap = new Vector();

  JPanel _mainPanel = new JPanel(new GridBagLayout());

  JMenuItem _deleteItem, _quitItem, _configItem;
  JCheckBoxMenuItem _viewWideItem;

  private DefaultTableCellRenderer _statRenderer = null;

  int _port = 0;

  Properties _properties = null;

  /**
   * Constructor of the Activity Console GUI.
   * @param arg the port to serve connections on - as an int parsed from the String
   */
  public ActivityConsoleGui(String arg)
  {
    int i;

    addWindowListener(new WindowCloseMonitor());

    JMenuBar menuBar = new JMenuBar();

    JMenu menuFile = new JMenu(resStrings.getString("menuFile"));
    MenuAction quitAction = new MenuAction(resStrings.getString("menuFileQuit"));
    _quitItem = menuFile.add(quitAction);
    menuBar.add(menuFile);

    JMenu menuEdit = new JMenu(resStrings.getString("menuEdit"));
    _configItem = menuEdit.add(new MenuAction(resStrings.getString("menuEditConfig")));
    _deleteItem = menuEdit.add(new MenuAction(resStrings.getString("menuEditDelete")));
    menuBar.add(menuEdit);

    JMenu menuView = new JMenu(resStrings.getString("menuView"));
    _viewWideItem = new JCheckBoxMenuItem(resStrings.getString("menuViewWide"));
    _viewWideItem.addActionListener(this);
    menuView.add(_viewWideItem);
    menuBar.add(menuView);

    this.setJMenuBar(menuBar);
    _deleteItem.setEnabled(false);

    loadProperties();

    try
    {
      _port = Integer.parseInt(arg);
      if (_port < 0)
        _port = 0;
    }
    catch (Throwable t)
    {
      _port = 0;
    }

    /**
     * The cell renderer for the StatWidget Component - simply returns the component
     * itself.  Additionally, it has the extra hack of telling the StatWidget where
     * it is in the table so it can update itself again when it comes time to flash.
     */
    _statRenderer = new DefaultTableCellRenderer()
    {
      public Component getTableCellRendererComponent(JTable table,
                                                     Object value,
                                                     boolean isSelected,
                                                     boolean hasFocus,
                                                     int row,
                                                     int column)
      {
        /* Housekeeping: keep track of the row, column and table references as we go */
        ((StatWidget)value).setRowColTable(row,column,table);
        return(Component)value;
      }

      public void setValue(Object value)
      {
        Color color = null;
        try
        {
          color = (Color)value;
        }
        catch (ClassCastException e)
        {
          color = Color.white;
        }
        setBackground(color);
      }
    };

    initTable();

    ActivityConsoleGuiUtil.constrain(_mainPanel, _tableScroller,
                                     1, 1, // X, Y Coordinates
                                     1, 1, // Grid width, height
                                     GridBagConstraints.BOTH,  // Fill value
                                     GridBagConstraints.WEST,  // Anchor value
                                     1.0,1.0,  // Weight X, Y
                                     0, 0, 0, 0 ); // Top, left, bottom, right insets

    this.getContentPane().add(_mainPanel, BorderLayout.CENTER);

    parent_ = this;
    this.pack();
    _table.setPreferredScrollableViewportSize(new Dimension(_table.getWidth(),50));
    this.pack();

    if (_port > 0)
    {
      _serverThread = new ServerThread(this, _port);
      _serverThread.start();
    }
    updateTitle(_port);
    setBounds(ActivityConsoleGuiUtil.center(this.getSize()));
    this.show();
  }

  /**
   * Updates the title bar with the port currently being served.
   * @param port the port being served
   */
  public void updateTitle(int port)
  {
    String title = resStrings.getString("guiTitle");

    title = StringUtil.replaceSubstring(title,"%1",""+port);
    setTitle(title);
  }

  public void actionPerformed(ActionEvent e)
  {
    if (e.getSource() == _viewWideItem)
    {
      setProperty("AC.detailedColumnSet", _viewWideItem.isSelected());
      initTable();
      this.pack();
      _table.setPreferredScrollableViewportSize(new Dimension(_table.getWidth(),50));
      this.pack();
    }
  }

  class MenuAction extends AbstractAction
  {
    public MenuAction(String text)
    {
      super(text,null);
    }

    public MenuAction(String text, Icon icon)
    {
      super(text,icon);
    }

    public void actionPerformed(ActionEvent e)
    {
      if (e.getSource() == _quitItem)
      {
        saveProperties();
        parent_.setVisible(false);
        parent_.dispose();
        System.exit(0);
      }
      else if (e.getSource() == _deleteItem)
      {
        deleteAction();
      }
      else if (e.getSource() == _configItem)
      {
        changeServerAction();
      }
    }
  }

  /**
   * Asks the user to specify a new port to serve
   */
  public void changeServerAction()
  {
    int port = -1;
    String message = resStrings.getString("guiNewPortPrompt");
    message = StringUtil.replaceSubstring(message,"%1",""+_port);

    String inputValue = JOptionPane.showInputDialog(this,
                                                    message,
                                                    resStrings.getString("guiNewPortTitle"),
                                                    JOptionPane.QUESTION_MESSAGE);
    if (inputValue != null)
      try
      {
        port = Integer.parseInt(inputValue);
      }
      catch (Throwable t)
      {
        port = -1;
      }
    if (port < 1)
      JOptionPane.showMessageDialog(null, resStrings.getString("guiNewPortErrorPrompt"), resStrings.getString("guiNewPortErrorTitle"), JOptionPane.ERROR_MESSAGE);
    else
    {
      if (_port != port)
      {
        if (_serverThread != null)
        {
          _serverThread.doClose();
          _serverThread.interrupt();
          _serverThread = null;
        }
        _port = port;
        _serverThread = new ServerThread(parent_, port);
        _serverThread.start();
        updateTitle(_port);
      }
    }
  }

  /**
   * Deletes the "selected" row after seeking confirmation
   */
  public void deleteAction()
  {
    int numSelections = _table.getSelectedRowCount();
    int selRow = _table.getSelectedRow();
    if (numSelections > 0)
    {
      if ((selRow > -1) &&
          (selRow < _table.getRowCount()))
      {
        /* Ask for confirmation */
        String message = resStrings.getString("guiDeleteConfirmPrompt");
        message = StringUtil.replaceSubstring(message,"%1",(String)_model.getValueAt(selRow,0));
        int ret = JOptionPane.showConfirmDialog(null,
                                                message,
                                                resStrings.getString("guiDeleteConfirmTitle"),
                                                JOptionPane.YES_NO_OPTION);
        if (ret == JOptionPane.YES_OPTION)
        {
          _model.removeRow(selRow);
        }
      }
    }
  }

  /**
   * Builds a new table with the requested columns.
   */
  public void initTable()
  {
    _model = new SortableTableModel(new Vector(), getColumnNames());
    _table = new JTable(_model);
    _table.setPreferredScrollableViewportSize(new Dimension(800,50));
    _table.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);
    _table.setCellSelectionEnabled(false);
    _table.setRowSelectionAllowed(false);
    SortButtonRenderer headerRenderer = new SortButtonRenderer();
    TableColumnModel cm = _table.getColumnModel();
    /* Make the first column twice the width of the others. It shows bigger stuff. */
    cm.getColumn(0).setPreferredWidth(cm.getColumn(0).getPreferredWidth() * 2);
    cm.getColumn(0).setHeaderRenderer(headerRenderer);
    for (int i = 1;i<_model.getColumnCount();i++)
    {
      cm.getColumn(i).setPreferredWidth((int)(cm.getColumn(i).getPreferredWidth() * 1));
      cm.getColumn(i).setCellRenderer(_statRenderer);
      cm.getColumn(i).setHeaderRenderer(headerRenderer);
    }

    JTableHeader header = _table.getTableHeader();
    header.addMouseListener(new HeaderListener(header,headerRenderer));

    ListSelectionModel csm = _table.getSelectionModel();
    csm.addListSelectionListener(new SelectedListener(csm));

    _tableScroller.setViewportView(_table);

  }

  /**
   * Retrieves the names of the column headers.
   * @return Vector the set of column names.  It also has the side-effect of adding
   * entries to the global column mapping Vector where we map the staus integer identifiers
   * to the column positions and names.  Should probably fix that too.
   */
  public Vector getColumnNames()
  {
    Vector names = new Vector();
    _tableColumnMap = getUserColumnNames();

    names.addElement(resStrings.getString("guiDefaultColumn0"));
    for (int i = 0; i < _tableColumnMap.size(); i ++)
    {
      names.addElement(((ColumnRef)_tableColumnMap.elementAt(i)).getDescription());
    }

    return names;
  }

  /**
   * Builds a map of columns based on the properties file.
   * If it is somehow unsuitable, the default table will be built.
   * This should be made to read a properties file. FIXME.
   * @return Vector The vector of column name-to-stat-ID mappings
   */
  public Vector getUserColumnNames()
  {
    Vector map;

    map = getDefaultColumnNames();

    return map;
  }

  /**
   * Builds a default map of columns.
   * @return Vector The vector of column name-to-stat-ID mappings
   */
  public Vector getDefaultColumnNames()
  {
    Vector map = new Vector();
    boolean detailedList = getProperty("AC.detailedColumnSet", false);
    // In case they didn't have the preference set... set it.
    setProperty("AC.detailedColumnSet", detailedList);
    _viewWideItem.setSelected(detailedList);

    map.addElement(new ColumnRef(resStrings.getString("guiDefaultColumn1"),1));
    map.addElement(new ColumnRef(resStrings.getString("guiDefaultColumn2"),2));
    map.addElement(new ColumnRef(resStrings.getString("guiDefaultColumn3"),3));
    if (detailedList)
    {
      map.addElement(new ColumnRef(resStrings.getString("guiDefaultColumn4"),4));
      map.addElement(new ColumnRef(resStrings.getString("guiDefaultColumn5"),5));
      map.addElement(new ColumnRef(resStrings.getString("guiDefaultColumn6"),6));
      map.addElement(new ColumnRef(resStrings.getString("guiDefaultColumn7"),7));
      map.addElement(new ColumnRef(resStrings.getString("guiDefaultColumn8"),8));
      map.addElement(new ColumnRef(resStrings.getString("guiDefaultColumn9"),9));
      map.addElement(new ColumnRef(resStrings.getString("guiDefaultColumn10"),10));
    }
    return map;
  }

  /**
   * Parses a String of statistics coming from Privoxy.
   * @param line The statistics string sent from Privoxy
   * @param from the hostname that sent the statistics
   */
  public void updateStats(String line, String from)
  {
    /*
     * An example line of data:
     * 0:8118 1:0 2:0 3:0 4:0 5:0 6:0 7:0 8:0 9:0 10:0
     */
    int key, value;
    String tableKey = "", key_str, value_str, token;
    StringTokenizer colonToken;
    StringTokenizer spaceTokens = new StringTokenizer(line);
    Vector stats = new Vector();

    while (spaceTokens.hasMoreTokens())
    {
      token = spaceTokens.nextToken();
      colonToken = new StringTokenizer(token,":");
      if (colonToken.hasMoreTokens())
      {
        key_str = null; value_str = null;
        key = -1; value = 0;

        /* First token is the key */
        key_str = colonToken.nextToken();
        try
        {
          key = Integer.parseInt(key_str);
        }
        catch (NumberFormatException n)
        {
          key = -1;
        }

        if ((colonToken.hasMoreTokens()) && (key > -1))
        {
          /* Next token, if present, is the value */
          value_str = colonToken.nextToken();
          if (key == 0)
          {
            /*
             * The key to the table row is the concatenation of the serving
             * IP address string, a full colon, and the port string.
             */
            tableKey = from + ":" + value_str;
          }
          try
          {
            value = Integer.parseInt(value_str);
            stats.addElement((Object)(new Stat(key, value)));
          }
          catch (NumberFormatException n)
          {
            value = 0;
          }
        }
      }
    }
    if ((tableKey.compareTo("") != 0) && (stats.size() > 0))
    {
      updateTable(tableKey, stats);
      stats.removeAllElements();
    }
    stats = null;
  }

  /**
   * Updates (or creates) a line in the table representing the incoming packet of stats.
   * @param tableKey Our key to a unique table row: the hostname concatenated with the Privoxy port being served.
   * @param stats Vector of statistics elements
   */
  public void updateTable(String tableKey, Vector stats)
  {
    boolean found = false;
    for (int i = 0; i < _model.getRowCount(); i++)
    {
      if (((String)_model.getValueAt(i,_table.convertColumnIndexToView(0))).compareTo(tableKey) == 0)
      {
        updateTableEntry(i, stats);
        found = true;
      }
    }
    /* If we can't find one in the table already... */
    if (found == false)
      createTableEntry(tableKey, stats);
  }

  /**
   * Creates a line in the table representing the incoming packet of stats.
   * @param tableKey Our key to a unique table row: the hostname concatenated with the Privoxy port being served.
   * @param stats Vector of statistics elements
   */
  public void createTableEntry(String tableKey, Vector stats)
  {
    int i, j;
    Vector row = new Vector();
    boolean added = false;

    row.addElement(tableKey);

    /*
     * If we have a key (in stats) that maps to a key in the _tableColumnMap,
     * then we add it to the vector destined for the table.
     */
    for (i = 0; i < _tableColumnMap.size(); i ++)
    {
      for (j = 0; j < stats.size(); j++)
      {
        if (((Stat)stats.elementAt(j)).getKey() == ((ColumnRef)_tableColumnMap.elementAt(i)).getKey())
        {
          row.addElement(new StatWidget(((Stat)stats.elementAt(j)).getValue(),500));
          added = true;
        }
      }
      if (added == false)
      {
        row.addElement(new StatWidget(0,500));
      }
      else
        added = false;
    }
    _model.addRow(row);
  }

  /**
   * Updates a line in the table by tweaking the StatWidgets.
   * @param row the table row if the StatWidget
   * @param stats The Vector of Stat elements to update the table row with
   */
  public void updateTableEntry(int row, Vector stats)
  {
    int i, j;

    for (i = 0; i < _tableColumnMap.size(); i ++)
    {
      for (j = 0; j < stats.size(); j++)
      {
        if (((Stat)stats.elementAt(j)).getKey() == ((ColumnRef)_tableColumnMap.elementAt(i)).getKey())
        {
          ((StatWidget)_model.getValueAt(row,i+1)).updateValue(((Stat)stats.elementAt(j)).getValue());
          stats.removeElementAt(j);
          break;
        }
      }
    }
  }

  /**
   * Load up the properties
   *
   */
  private void loadProperties()
  {
    _properties = new Properties();
    try
    {
      _properties.load(new FileInputStream("ActivityConsole.properties"));
    }
    catch (Throwable t)
    {
      // System.out.println(t);
      // No properties file... use hardcoded defaults.
    }
  }

  /**
   * Save the properties
   *
   */
  private void saveProperties()
  {
    try
    {
      _properties.store(new FileOutputStream("ActivityConsole.properties"),resStrings.getString("guiPropertiesFileHeader"));
    }
    catch (Throwable t)
    {
      System.out.println(t);
    }
  }

  /**
   * Set a property
   */
  public void setProperty(String key, String value)
  {
    _properties.setProperty(key,value);
  }

  /**
   * Set a boolean property
   */
  public void setProperty(String key, boolean value)
  {
    Boolean bVal = new Boolean(value);
    _properties.setProperty(key,(String)bVal.toString());
  }

  /**
   * Get a property
   */
  public String getProperty(String key, String defaultValue)
  {
    return _properties.getProperty(key,defaultValue);
  }

  /**
   * Get a boolean property
   */
  public boolean getProperty(String key, boolean defaultValue)
  {
    String sDefaultValue;
    String property;
    if (defaultValue == false)
      sDefaultValue = "false";
    else
      sDefaultValue = "true";
    property = _properties.getProperty(key,sDefaultValue);
    if (property.compareToIgnoreCase("true") == 0)
      return true;
    else
      return false;
  }

  /**
   * Remove a property
   */
  public void removeProperty(String key)
  {
    _properties.remove(key);
  }

  /**
   * Worker class to offer a clickable table header for sorting.
   */
  class HeaderListener extends MouseAdapter
  {
    JTableHeader   header;
    SortButtonRenderer renderer;

    HeaderListener(JTableHeader header,SortButtonRenderer renderer)
    {
      this.header   = header;
      this.renderer = renderer;
    }

    public void mousePressed(MouseEvent e)
    {
      Point click = e.getPoint();
      int col = header.columnAtPoint(click);
      int margin1, margin2;
      int sortCol = header.getTable().convertColumnIndexToModel(col);

      /* Don't perform the sort if the user is just trying to resize the columns. */
      margin1 = header.columnAtPoint(new Point(click.x+3,click.y));
      margin2 = header.columnAtPoint(new Point(click.x-3,click.y));
      if ((col == margin1) && (col == margin2))
      {
        renderer.setPressedColumn(col);
        renderer.setSelectedColumn(col);
        header.repaint();

        if (header.getTable().isEditing())
        {
          header.getTable().getCellEditor().stopCellEditing();
        }

        boolean isAscent;
        if (SortButtonRenderer.DOWN == renderer.getState(col))
        {
          isAscent = true;
        }
        else
        {
          isAscent = false;
        }
        ((SortableTableModel)header.getTable().getModel())
        .sortByColumn(sortCol, isAscent);    
      }
    }

    public void mouseReleased(MouseEvent e)
    {
      int col = header.columnAtPoint(e.getPoint());
      renderer.setPressedColumn(-1);
      header.repaint();
    }
  }

  /**
   * Worker class to tell the menu when it's OK to delete a row (i.e. when a row gets
   * selected).  This doesn't work reliably, but it's better than nothing.
   */
  public class SelectedListener implements ListSelectionListener
  {
    ListSelectionModel model;

    public SelectedListener(ListSelectionModel lsm)
    {
      model = lsm;
    }

    public void valueChanged(ListSelectionEvent lse)
    {
      // NOTE - keep this in sync with columnSelectionChanged below...
      int numSelections = _table.getSelectedRowCount();
      int selRow = _table.getSelectedRow();
      if (numSelections > 0)
      {
        if ((selRow > -1) &&
            (selRow < _table.getRowCount()))
        {
          _deleteItem.setEnabled(true);
        }
        else
          _deleteItem.setEnabled(false);
      }
      else
        _deleteItem.setEnabled(false);
    }
    public void columnSelectionChanged(ListSelectionEvent lse)
    {
      // NOTE - keep this in sync with valueChanged above...
      int numSelections = _table.getSelectedRowCount();
      int selRow = _table.getSelectedRow();
      if (numSelections > 0)
      {
        if ((selRow > -1) &&
            (selRow < _table.getRowCount()))
        {
          _deleteItem.setEnabled(true);
        }
        else
          _deleteItem.setEnabled(false);
      }
      else
        _deleteItem.setEnabled(false);
    }
  }

  /**
   * Watch for the window closing event.  Dunno why swing doesn't handle this better natively.
   */
  public class WindowCloseMonitor extends WindowAdapter
  {
    public void windowClosing(WindowEvent e)
    {
      saveProperties();
      Window w = e.getWindow();
      w.setVisible(false);
      w.dispose();
      System.exit(0);
    }
  }
}
