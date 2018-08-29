/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.dnd.*;

import vnmr.bo.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class VPseudo2 extends JComponent implements VObjIF, DropTargetListener
{

    private boolean bFocus = true;
    private boolean inEditMode = false;
    private ButtonIF vnmrIf;
    private Point tmpLoc = new Point(0, 0);


    public VPseudo2()
    {
        setOpaque(false);
        //setMargin(new Insets(0,0,0,0));
        setBorder(new VButtonBorder());
        new DropTarget(this, this);
    }

    public void changeFocus(boolean b) {
        if (bFocus != b) {
            bFocus = b;
            repaint();
        }
    }

    public void paint(Graphics g) {
        Dimension  psize = getPreferredSize();
        if (bFocus)
            g.setColor(Color.yellow);
        else
            g.setColor(Color.green);
        g.drawRect(0, 0, psize.width -1, psize.height -1);
        g.drawRect(1, 1, psize.width -3, psize.height -3);
    }

    public void setAttribute(int t, String s) {}
    public String getAttribute(int t) { return null; }
    public void setEditStatus(boolean s) { }
    public void setEditMode(boolean s) {
        inEditMode = s;
    }

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
    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void destroy() {}
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

    public Point getDefLoc() {
        return  tmpLoc;
    }

    public void setSizeRatio(double x, double y) {}

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        if (inEditMode)
            VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop
}
