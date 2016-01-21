/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import javax.swing.*;
import javax.swing.border.*;
import java.io.*;
import java.awt.*;
import java.awt.dnd.*;


import vnmr.ui.*;
import vnmr.bo.*;


public class AnnotateObj extends JLabel
{
     private DragSource dragSource;
     private LocalRefSelection ref;
     private Dimension psize = null;
     private int lines = 1;
     private String strs[];
     private String val;
     private String name;


     public AnnotateObj(String s) {
          setText(s);
          setBorder(new EtchedBorder(EtchedBorder.LOWERED));
          ref = new LocalRefSelection(this);
          dragSource = new DragSource();
          dragSource.createDefaultDragGestureRecognizer(this,
                           DnDConstants.ACTION_COPY,
                           new AnnGestureListener());
     }

     public AnnotateObj() {
          this("");
     }

     public Dimension getPreferredSize() {
        if (psize != null) {
            return psize;
        }
        Graphics g = getGraphics();
         
        if (g == null) {
           return new Dimension(10, 10);
        }
        FontMetrics fm = g.getFontMetrics();
        int w = 4;
        int w2;
        int h = fm.getHeight() * strs.length;
        for (int k = 0; k < strs.length; k++) {
           w2 = fm.stringWidth(strs[k]);
           if (w2 > w)
              w = w2;
        }
        psize = new Dimension(w+4, h+4);
        return psize;
     }

     public void setName(String s) {
        name = s;
     }

     public String getName() {
        return name;
     }

     public String getText() {
        return null;
     }

     public void setText(String s) {
        lines = 1;
        val = s;
        if (s == null) {
            strs = null;
            return;
        }
        strs = s.split("\n");   
        lines = strs.length;
        psize = null;
     }

     public void paint(Graphics g) {
        super.paint(g);
        Dimension  size = getSize();
        if (strs == null) {
           return;
        }
        int x = 0;
        String str; 
        FontMetrics fm = g.getFontMetrics();
        int y = fm.getAscent() + 2;
        for (int k = 0; k < strs.length; k++) {
            str = strs[k]; 
            if (str != null) {
                x = (size.width - fm.stringWidth(str)) / 2;
                g.drawString(str, x, y);
                y += fm.getHeight();
            }
        }      
     }

     class AnnDragSourceListener implements DragSourceListener {
        public void dragDropEnd (DragSourceDropEvent evt) {}
        public void dragEnter (DragSourceDragEvent evt) {
        }
        public void dragExit (DragSourceEvent evt) {
        }
        public void dragOver (DragSourceDragEvent evt) {}
        public void dropActionChanged (DragSourceDragEvent evt) {
        }
     } // class AnnDragSourceListener

     class AnnGestureListener implements DragGestureListener {
        AnnDragSourceListener nl = null;
        public void dragGestureRecognized(DragGestureEvent e) {
             if (nl == null)
                  nl = new AnnDragSourceListener();
             dragSource.startDrag(e, null, ref, nl);
        }
     } // class AnnGestureListener
}

