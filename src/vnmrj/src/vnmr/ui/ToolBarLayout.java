/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.util.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.bo.*;

public class ToolBarLayout implements LayoutManager
{
   private int preferredWidth = 0;
   private int actualWidth = 0;
   private int rowHeight = 12;
   private Vector vecComp = null;
   private int[] compX = null;
   private int[] compY = null;
   private int[] curX;
   private int[] curY;
   private int   marginY = 2;
   private int   ptLen = 0;
   private int   lastWidth = 0;
   private boolean bEditMode = false;
   private Container layoutTarget;

   protected VjToolBar m_toolbar;

   public ToolBarLayout(VjToolBar toolbar)
   {
        super();
        m_toolbar = toolbar;
   }

   public void addLayoutComponent( String name, Component comp ) {
//	super.addLayoutComponent(name, comp);
   }
   public void removeLayoutComponent( Component comp ) {
//	super.removeLayoutComponent(comp);
   }

  public Dimension preferredLayoutSize( Container target )
  {
      Dimension dim = new Dimension( 0, 0 );
      Insets insets = target.getInsets();
      int count = target.getComponentCount();

      rowHeight = 10;
      actualWidth = 0;
      if (count <= 0)
         return dim;
      Component comp;
      Dimension d;
      Point pt;

      preferredWidth = 0;

      for( int i = 0; i < count; i++ )
      {
         comp = target.getComponent( i );
         if (comp == null || !comp.isVisible())
             continue;
         d = comp.getPreferredSize();
         actualWidth += d.width;
         if (d.height > rowHeight)
             rowHeight = d.height;
       
         if (comp instanceof VObjIF)
            pt = ((VObjIF)comp).getDefLoc();
         else
            pt = comp.getLocation();
	 if ((pt.x + d.width) > preferredWidth)
             preferredWidth = pt.x + d.width;
         lastWidth = d.width;
      }
      if (preferredWidth < actualWidth)
          preferredWidth = actualWidth;
      preferredWidth = preferredWidth + insets.left + insets.right;
      actualWidth = actualWidth + insets.left + insets.right;
      if (preferredWidth > actualWidth)
          dim.width = preferredWidth;
      else
          dim.width = actualWidth;
     
      dim.height = rowHeight + marginY;
      return dim;
  }

  public Dimension minimumLayoutSize( Container parent )
  {
      return new Dimension(10, 10);
  }


  public void layoutContainer( Container target )
  {
      Point loc;
      Dimension dim;
      Insets insets = target.getInsets();
      int cw = target.getSize().width - insets.left - insets.right;
      int ch = target.getSize().height - insets.top - insets.bottom;
      int count = target.getComponentCount();
      int x, y, w, h, ptX;
      if (count <= 0)
	   return;
      double xRatio = 1.0;
      String str;

      x = 0;
      h = 20;
      w = 20;
      ptX = 0;
      if (cw < preferredWidth) {
          xRatio = (double) (cw - lastWidth) / (double) preferredWidth;
          if (xRatio < 0.1)
              xRatio = 0.1;
      }
      for( int i = 0; i < count; i++ )
      {
         Component comp = target.getComponent( i );
         if (comp == null || !comp.isVisible())
             continue;
         dim = comp.getPreferredSize();
         h = dim.height;
         w = dim.width;
         if (h > ch)
             h = ch;
         loc = comp.getLocation();
         if (!bEditMode) {
            if (comp instanceof VObjIF)
                loc = ((VObjIF)comp).getDefLoc();
            if (xRatio < 1.0)
                loc.x = (int) (xRatio * loc.x);
            if (loc.x < ptX)
                loc.x = ptX;
         }

         y = (ch - h) / 2;
         if (y < 0)
             y = 0;
         comp.setBounds( loc.x, y, w, h );

         ptX = loc.x + w + 1;
         comp.validate();
      }
   }


   public int getRowHeight() {
	return (rowHeight + marginY);
   }

   public int getActualWidth() {
	return actualWidth;
   }

   public int getGap() {
	return marginY;
   }

   public void resetPreferedSize( Container target )
   {
      int count = target.getComponentCount();
      Dimension dim;

      for( int i = 0; i < count; i++ )
      {
         Component comp = target.getComponent( i );
         if (comp == null || !comp.isVisible())
             continue;
         if( comp instanceof VButton ) {
            Icon icon = ((JButton)comp).getIcon();
            dim = comp.getPreferredSize();
            if (icon == null) {
               comp.setPreferredSize(null);
               Dimension d2 = ((VButton)comp).getMinSize();
               dim = comp.getPreferredSize();
               if (dim != null && dim.width > 6) {
                   if (d2.width > dim.width)
                       dim.width = d2.width;
                   comp.setPreferredSize(dim);
               }
            }
            else {
               dim.width = icon.getIconWidth() + 4;
               dim.height = icon.getIconHeight() + 4;
               if (dim.width > 6 && dim.height > 6)
                   comp.setPreferredSize(dim);
            }
         }
      }
   }

   private void sortOrder(Container target) {
      int count = target.getComponentCount();
      int x = 0;
      int y = 0;
      int k;
      Dimension dim;
      Component c1, c2;
      if (count <= 0)
	   return;
      preferredLayoutSize(target);
      boolean bChangeOrder = false;

      if ((vecComp == null) || (vecComp.size() < count))
          vecComp = new Vector(count);
      if (ptLen < count) {
          ptLen = count + 2;
          compX = new int[ptLen];
          compY = new int[ptLen];
      }

      vecComp.clear();
      for( int i = 0; i < count; i++ ) {
          c1 = target.getComponent( i );
          x = c1.getX();
          y = c1.getY();
           if (i == 0) {
               vecComp.add(c1);
           }
           else {
               k = i / 2;
               while (k > 0) {
                   c2 = (Component) vecComp.elementAt(k);
                   if (c2.getX() > x)
                       k = k / 2;
                   else
                       break;
               }
               while (k < i) {
                   c2 = (Component) vecComp.elementAt(k);
                   if (c2.getX() > x) {
                       bChangeOrder = true;
                       vecComp.insertElementAt(c1, k);
                       break;
                   }
                   k++;
               }
               if (k == i) { // room not found
                   vecComp.add(c1);
               }
           }
       }
       if (bChangeOrder)
       {
           target.removeAll();
       }
       for(k = 0; k < vecComp.size(); k++ ) {
           c1 = (Component) vecComp.elementAt(k);
           if (c1 != null) {
               compX[k] = c1.getX();
               compY[k] = c1.getY();
               if (bChangeOrder)
                   target.add(c1);
           }
       }
   }

    public void saveLocations(Container target) {
         layoutTarget = target;
         resetPreferedSize(target);
	 sortOrder(target);
    }

    public void setEditMode(boolean s) {
         boolean oldMode = bEditMode;
         bEditMode = s;
         if (oldMode != s && layoutTarget != null) {
             if (oldMode)
                 layoutContainer(layoutTarget);
         }
    }
}
