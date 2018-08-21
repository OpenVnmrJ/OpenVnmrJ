/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import javax.swing.*;
import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;

import vnmr.util.*;
import vnmr.templates.*;
import vnmr.bo.*;

public class Annotation extends JPanel implements VObjDef, SwingConstants
{
    private int gridNum = 40;
    private int panX;
    private int panY;
    private int panW;
    private int panH;
    private int fontH = 0;
    private float gridW = 0;
    private float gridH = 0;
    private float printRw = 1;
    private float printRh = 1;
    private boolean  newGeom = true;
    private boolean  bPrintMode = false;
    private Object[] annObjs;
    private int[] orientObjs;
    private int objRooms = 0;
    private int objNum = 0;
    private long timeOftemplate = 0;
    private String templateFile = null;
    
    
    public Annotation() {
         setLayout(new xyLayout());
         setOpaque(false);
        
         addComponentListener(new ComponentAdapter() {
            public void componentResized(ComponentEvent evt) {
                 resizeProc();
            }
         });

         orientObjs = new int[4];
    }

    public Rectangle getBounds(Rectangle rv) {
        if (rv == null) {
            return new Rectangle(panX, panY, panW, panH);
        }
        else {
            rv.setBounds(panX, panY, panW, panH);
            return rv;
        }
    }

    public Rectangle getBounds() {
        return new Rectangle(panX, panY, panW, panH);
    }

    public void setBounds(int x, int y, int w, int h) {
        panX = x;
        panY = y;
        panW = w;
        panH = h;
        super.setBounds(x, y, w, h);
    }

    public void setLocation(int x, int y) {
        panX = x;
        panY = y;
        super.setLocation(x, y);
    }

    public void setPrintBounds(int x, int y, int w, int h) {
        panX = x;
        panY = y;
        panW = w;
        panH = h;
        gridW = ((float) panW) / ((float) gridNum);
        gridH = ((float) panH) / ((float) gridNum);
        newGeom = true;
/*
        int count = getComponentCount();
        int i;
        Component comp;
        for (i = 0; i < count; i++) {
             comp = getComponent(i);
             if  (comp != null) {
                if (comp instanceof VAnnotateTable)
                    ((VAnnotateTable) comp).layoutTable();
             }
        }
*/
    }

    public boolean isVisible() {
        return true;
    }

    public boolean isShowing() {
        return true;
    }

    private void setOrient(int show, String s) {
        for (int i = 0; i < 4; i++) {
            int k = orientObjs[i];
            if (k > 0 && annObjs[k] != null) {
                String d;
                VAnnotateBox vObj = (VAnnotateBox) annObjs[k];
                if (show > 0) {
                    d = s.substring(i, i+1);
                    vObj.setAttribute(ORIENTATION, d);
                }
                else
                    vObj.setAttribute(ORIENTATION, null);
            }
        }
    }

    public void setValue(int id, int debug, int show, String v) {
        if (id >= 1999) {
            if (v != null)
                setOrient(show, v);
            return;
        }
        if (id >= objRooms)
            return;
        if (annObjs[id] == null)
            return;
        JComponent comp = (JComponent) annObjs[id];
        if (show > 0)
            comp.setVisible(true);
        else
            comp.setVisible(false);
        if (v != null)
            ((VObjIF) comp).setAttribute(LABEL, v);
    }

    private void addObj(Component c, int k) {
        int i;

        if (k >= objRooms) {
            Object[] newList = new Object[k + 6];
            for (i = 0; i < objRooms; i++)
                newList[i] = annObjs[i];
            for (i = objRooms; i < k+6; i++)
                newList[i] = null;
            objRooms = k + 6;
            annObjs = newList;
        }
        annObjs[k] = c;
        objNum++;
    }


    private void expandObjList(Container p) {
         int i, k;
         int count = p.getComponentCount();

         for (i = 0; i < count; i++) {
             Component comp = p.getComponent(i);
             if  (comp != null && (comp instanceof VAnnotationIF)) {
                k = ((VAnnotationIF)comp).getId();
                if (k <= 0)
                    k = objNum + 1;
                addObj(comp, k);
                if  (comp instanceof Container)
                    expandObjList((Container)comp);
             }
         }
    }

    private void setObjList() {
        int i, k;
        int count = getComponentCount();

        if (objRooms <= 0) {
            objRooms = 20;
            annObjs = new Object[objRooms];
        }
        for (k = 0; k < objRooms; k++)
            annObjs[k] = null;
        objNum = 0;
        for (i = 0; i < count; i++) {
            Component comp = getComponent(i);
            if  (comp != null && (comp instanceof VAnnotationIF)) {
                k = ((VAnnotationIF)comp).getId();
                if (k <= 0)
                    k = objNum + 1;
                addObj(comp, k);
                if  (comp instanceof Container)
                    expandObjList((Container)comp);
            }
        }
    }

    public String getTemplateFile() {
        return templateFile;
    }

    public void loadTemplate(String f) {
        int i;

        if (f == null)
            return;
        UNFile file = new UNFile(f);
        if (!file.exists())
            return;
        if (templateFile != null) {
            if (templateFile.equals(f)) {
                if (timeOftemplate == file.lastModified())
                     return;
            }
        }
        templateFile = f;
        timeOftemplate = file.lastModified();
        for (i = 0; i < 4; i++)
             orientObjs[i] = 0;

        removeAll();
        try {
               LayoutBuilder.build(this, null, f);
        }
        catch (Exception e) { e.printStackTrace(); }
        setAttribute();
        getGridNum();
        newGeom = true;
        setObjList();
        for (i = 1; i < objRooms; i++) {
            if (annObjs[i] != null) {
                if (annObjs[i] instanceof VAnnotateBox) {
                    VAnnotateBox vObj = (VAnnotateBox) annObjs[i];
                    if (vObj.isOrientObj()) {
                        if (vObj.dockAt == WEST)
                            orientObjs[0] = i;               
                        else if (vObj.dockAt == EAST)
                            orientObjs[1] = i;               
                        else if (vObj.dockAt == NORTH)
                            orientObjs[2] = i;               
                        else
                            orientObjs[3] = i;               
                    }
                }
            }
        }
        validate();
    }


    private void resizeProc() {
         panW = getWidth();
         panH = getHeight();
         gridW = ((float) panW) / ((float) gridNum);
         gridH = ((float) panH) / ((float) gridNum);
         newGeom = true;
    }


    public int getGridNum() {
        int count = getComponentCount();
        gridNum = 12;
        for (int i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof VAnnotationIF)) {
              int k = ((VAnnotationIF) comp).getGridNum();
              if (k > gridNum)
                  gridNum = k;
           }
        }
        return gridNum;
    }


    public void setAttribute() {
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof VObjIF)) {
              ((VObjIF) comp).setAttribute(ANNOTATION, "yes");
           }
        }
    }

    public void setPrintMode(boolean s) {
        // bPrintMode = s;
        int count = getComponentCount();
        int i;
        for (i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof VAnnotationIF)) {
              ((VAnnotationIF) comp).setPrintMode(s);
           }
        }
    }

    public void setPrintColor(boolean s) {
        int count = getComponentCount();
        int i;
        for (i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof VAnnotationIF)) {
              ((VAnnotationIF) comp).setPrintColor(s);
           }
        }
    }

    public void setPrintRatio(float rw, float rh) {
        printRw = rw;
        printRh = rh;
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof VAnnotationIF)) {
              ((VAnnotationIF) comp).setPrintRatio(rw, rh);
           }
        }
    }

    public void  paint(Graphics g) {
    }

    public void  draw(Graphics2D g, int x, int y) {
        Font ft = g.getFont();
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof VAnnotationIF)) {
              ((VAnnotationIF) comp).draw(g, x, y);
           }
        }
        g.setFont(ft);
    }


    public void xlayout() {
        if (bPrintMode)
            return;
        Dimension dim = getSize();
        gridW = ((float) dim.width) / ((float) gridNum);
        gridH = ((float) dim.height) / ((float) gridNum);
        int count = getComponentCount();
        int i;
        Component comp;
        for (i = 0; i < count; i++) {
             comp = getComponent(i);
             if  (comp != null) {
                if (comp instanceof VAnnotateTable)
                    ((VAnnotateTable) comp).layoutTable();
                else if (comp instanceof JPanel)
                    ((JPanel) comp).doLayout();
             }
        }
    }


    class xyLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
        }


        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(100, 100);
        }

        public void layoutContainer(Container target) {
            if (bPrintMode)
                return;
            synchronized(target.getTreeLock()) {
               Dimension dim = target.getSize();
               gridW = ((float) dim.width) / ((float) gridNum);
               gridH = ((float) dim.height) / ((float) gridNum);
               int count = target.getComponentCount();
               int i;
               for (i = 0; i < count; i++) {
                    Component comp = target.getComponent(i);
                    if  ((comp != null) && (comp instanceof VAnnotationIF)) {
                        ((VAnnotationIF)comp).adjustBounds(gridNum, gridNum, gridW, gridH);
                    }
               }
               if (newGeom) {
                    newGeom = false;
                    for (i = 0; i < count; i++) {
                        Component comp = target.getComponent(i);
                        if  (comp instanceof VAnnotationIF) {
                            ((VAnnotationIF)comp).setBoundsRatio(true);
                        }
                    }
               }
            } // synchronized
        } // layoutContainer
    }
}
