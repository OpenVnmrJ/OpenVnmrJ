/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.*;
import javax.swing.*;
import java.util.*;
import vnmr.util.*;
import vnmr.ui.*;


public class VGrid extends JComponent implements VEditIF, VObjIF, VObjDef
{
    public String type = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private ButtonIF vnmrIf;
    private int gridWidth = 1;
    private int gridHeight = 1;
    private int gridX = 1;
    private int gridY = 1;

    private final static Object[][] m_attributes = {
        {new Integer(GRID_COLUMN), "grid column:"},
        {new Integer(GRID_ROW), "grid row:"},
        {new Integer(GRID_XSPAN), "grid column span:"},
        {new Integer(GRID_YSPAN), "grid row span:"},
    };


    public VGrid(SessionShare sshare, ButtonIF vif, String typ) {
        super();
        this.type = typ;
        this.vnmrIf = vif;
        setOpaque(false);
        setLayout(new XGridLayout());
    }

    private int parseInt(String str) {
        int value = 1;

        if (str == null)
           return value;
        try {
              value = Integer.parseInt(str);
        } catch (Exception e) { }
        if (value <= 0)
            value = 1;
        return value;
    }


    public void setAttribute(int attr, String c) {
        String strValue = c;
        if (c != null)
            c = c.trim();
        switch (attr) {
           case TYPE:
               type = c;
               break;
           case GRID_COLUMN:  // grid-column
               if (c != null)
                  gridX = parseInt(c);
               if (gridX < 1)
                  gridX = 1;
               break;
           case GRID_ROW:  // grid-row
               if (c != null)
                  gridY = parseInt(c);
               if (gridY < 1)
                  gridY = 1;
               break;
           case GRID_XSPAN:  // grid-column-span
               if (c != null)
                  gridWidth = parseInt(c);
               break;
           case GRID_YSPAN:  // grid-row-span
               if (c != null)
                  gridHeight = parseInt(c);
               break;
        }
    }

    public String getAttribute(int attr) {
        switch (attr) {
          case TYPE:
            return type;
          case GRID_XSPAN:  // grid-column-span
            return Double.toString(gridWidth);
          case GRID_YSPAN:  // grid-row-span
            return Double.toString(gridHeight);
          case GRID_ROW:  // grid-row
            return Double.toString(gridY);
          case GRID_COLUMN:  // grid-column
            return Double.toString(gridX);
        }
        return null;
    }

    public void setGridColumn(int n) {
         gridX = n;
    }

    public void setGridRow(int n) {
         gridY = n;
    }

    public void setGridColumnSpan(int n) {
         gridWidth = n;
    }

    public void setGridRowSpan(int n) {
         gridHeight = n;
    }

    public int getGridColumn() {
         return gridX;
    }

    public int getGridRow() {
         return gridY;
    }

    public int getGridColumnSpan() {
         return gridWidth;
    }

    public int getGridRowSpan() {
         return gridHeight;
    }

    public void setEditStatus(boolean s) {}
    public void setEditMode(boolean s) {}
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void setDefLabel(String s) {}
    public void setDefColor(String s) {}
    public void setDefLoc(int x, int y) {}
    public void refresh() {}
    public void changeFont() {}
    public void updateValue() {}
    public void setValue(ParamIF p) {}
    public void setShowValue(ParamIF p) {}
    public void changeFocus(boolean s) {}
    public ButtonIF getVnmrIF() { return null; }
    public void setVnmrIF(ButtonIF vif) {}
    public void destroy() {}
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}
    public void setSizeRatio(double w, double h) {}
    public Point getDefLoc() { return (new Point(0,0)); }


    // VEditIF
    public Object[][] getAttributes() {
        return m_attributes;
    }

    public Dimension getRealSize() {
            int count = getComponentCount();
            int w = 6;
            int h = 6;
            int w2 = 6;
            int h2 = 6;
            int k;
            Component comp;
            Dimension d;
            Point pt;

            for (k = 0; k < count; k++) {
                comp = getComponent(k);
                if (comp != null) {
                    if (comp instanceof VObjIF)
                       pt = ((VObjIF)comp).getDefLoc();
                    else
                       pt = comp.getLocation();
                    d = comp.getPreferredSize();
                    if ((d.width + pt.x) > w)
                        w = d.width + pt.x;
                    if ((d.height + pt.y) > h)
                        h = d.height + pt.y;

                    if (comp instanceof VGroup)
                        d = ((VGroup) comp).getActualSize();

                    if ((d.width + pt.x) > w2)
                        w2 = d.width + pt.x;
                    if ((d.height + pt.y) > h2)
                        h2 = d.height + pt.y;
                }
            }
            if (w < w2)
                w = w2;
            if (h < h2)
                h = h2;
            return new Dimension(w, h);
    }


    private class XGridLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        }
     
        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                int width = target.getWidth();
                int height = target.getHeight();
                int count = target.getComponentCount();
                for (int i = 0; i < count; i++) {
                    Component comp = target.getComponent(i);
                    comp.setBounds(0, 0, width, height);
                }
            }
        }
    }
}

