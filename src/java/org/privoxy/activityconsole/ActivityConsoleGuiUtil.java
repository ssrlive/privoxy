/*********************************************************************
 *
 * File        :  $Source$
 *
 * Purpose     :  Utility functions for GridBag layout and centering
 *                frames.
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

/**
 * The ActivityConsoleGuiUtil class: Helper routines for the GridBag layout.
 * @author Last Modified By: $Author$
 * @version $Rev$-$Date$$State$
 */
public class ActivityConsoleGuiUtil 
{
  private static final String
    COPYRIGHT = org.privoxy.activityconsole.Copyright.COPYRIGHT;

  /** constrain - Helper method for setting a componets constraints in a gridbag layout;
    * takes all of the possible parameters for grid constraints.
    * @param container  conatiner to add the component to
    * @param component  component that will be added
    * @param grid_x x value
    * @param grid_y y value
    * @param grid_width grid width for the component
    * @param grid_height grid height for the component
    * @param fill fill value
    * @param anchor anchor value
    * @param weight_x weight x
    * @param weight_y weight y
    * @param top top inset
    * @param left left inset
    * @param bottom bottom inset
    * @param right right inset
    */

  public static void constrain(Container container,
                               Component component, 
                               int grid_x,
                               int grid_y,
                               int grid_width,
                               int grid_height,
                               int fill,
                               int anchor,
                               double weight_x,
                               double weight_y,
                               int top,
                               int left,
                               int bottom,
                               int right)
  {
    GridBagConstraints c = new GridBagConstraints();
    c.gridx = grid_x;
    c.gridy = grid_y;
    c.gridwidth = grid_width;
    c.gridheight = grid_height;
    c.fill = fill;
    c.anchor = anchor;
    c.weightx = weight_x;
    c.weighty = weight_y;
    if (top+bottom+left+right > 0)
      c.insets = new Insets(top, left, bottom, right);

    ((GridBagLayout)container.getLayout()).setConstraints(component, c);
    container.add(component);
  }

  public static void constrain(Container container,
                               Component component, 
                               int grid_x,
                               int grid_y,
                               int grid_width,
                               int grid_height)
  {
    constrain(container,
              component,
              grid_x,
              grid_y,
              grid_width,
              grid_height,
              GridBagConstraints.NONE, 
              GridBagConstraints.NORTHWEST,
              0.0, 0.0, 0, 0, 0, 0);
  }

  public static void constrain(Container container,
                               Component component, 
                               int grid_x,
                               int grid_y,
                               int grid_width,
                               int grid_height,
                               int top,
                               int left,
                               int bottom,
                               int right)
  {
    constrain(container,
              component,
              grid_x,
              grid_y, 
              grid_width,
              grid_height,
              GridBagConstraints.NONE, 
              GridBagConstraints.NORTHWEST, 
              0.0, 0.0, top, left, bottom, right);
  }

  /** constrainLast - Helper method for setting a componets constraints in a gridbag layout;
    * takes all of the possible parameters for grid constraints.
    * @param container  conatiner to add the component to
    * @param component  component that will be added
    * @param grid_x x value
    * @param grid_y y value
    * @param top top inset
    * @param left left inset
    * @param bottom bottom inset
    * @param right right inset
    */

  public static void constrainLast(Container container,
                                   Component component,
                                   int grid_x,
                                   int grid_y,
                                   int top,
                                   int left,
                                   int bottom,
                                   int right)
  {
    constrain(container,
              component,
              grid_x,
              grid_y, 
              GridBagConstraints.REMAINDER,1,
              GridBagConstraints.HORIZONTAL, 
              GridBagConstraints.NORTHWEST, 
              0.0, 0.0, top, left, bottom, right);
  }

  /** constrain - Helper method for setting a componets constraints in a gridbag layout;
    * takes all of the possible parameters for grid constraints.
    * @param container  conatiner to add the component to
    * @param component  component that will be added
    * @param int        x value
    * @param int        y value
    * @param int        top inset
    * @param int        left inset
    * @param int        bottom inset
    * @param int        right inset
    */

  public static void constrain(Container container, Component component, 
                               int grid_x, int grid_y,int top, int left, int bottom, int right)
  {
    constrain(container,
              component,
              grid_x,
              grid_y, 
              1, 1,
              GridBagConstraints.NONE, 
              GridBagConstraints.NORTHWEST, 
              0.0, 0.0, top, left, bottom, right);
  }

  public static Rectangle center(Dimension dim)
  {
    final Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();

    final Rectangle centeredRect =
    new Rectangle( (screenSize.width  - dim.width)  /2,
                   (screenSize.height - dim.height) /2,
                   dim.width,
                   dim.height);
    return centeredRect;
  }
}
