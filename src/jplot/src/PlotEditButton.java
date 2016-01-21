/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * 
 *
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;


class PlotEditButton extends JButton 
{  
   public PlotEditButton(String imgSrc) 
   {
      ImageIcon icon =  new ImageIcon(imgSrc);
      setIcon(icon);
      setMargin(new Insets(0,0,0,0));
      width = icon.getIconWidth() + 2 * BORDER_XWIDTH;
      height = icon.getIconHeight() + 2 * BORDER_YHEIGHT;
      myid = 0;
      setMargin(new Insets(0,0,0,0));
   }

  public PlotEditButton(String imgSrc, int id) 
  {
      ImageIcon icon =  new ImageIcon(imgSrc);
      setIcon(icon);
      width = icon.getIconWidth() + 2 * BORDER_XWIDTH;
      height = icon.getIconHeight() + 2 * BORDER_YHEIGHT;
      myid = id;
      setMargin(new Insets(0,0,0,0));
  }

   public void helpMessage (String s)
   {
	helpStr = " "+s;
   }

   public void setId (int  id)
   {
	myid = id;
   }

   public int getId ()
   {
	return myid;
   }

   public void setAutoRelease(boolean set)
   {
	autoReset = set;
   }

   public void setActive(boolean set)
   {
	pressed = set;
	active = set;
	repaint();
   }

   public static final int BORDER_XWIDTH = 2;
   public static final int BORDER_YHEIGHT = 2;
   private int width;
   private int height;
   private int myid;
   private String helpStr = null; 
   private boolean pressed = false; 
   private boolean autoReset = true; 
   private boolean active = false; 
}

