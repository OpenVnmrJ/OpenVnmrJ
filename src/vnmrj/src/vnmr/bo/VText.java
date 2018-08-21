/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.util.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import java.beans.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VText extends JScrollPane implements VObjIF, VObjDef,
	  DropTargetListener, PropertyChangeListener
{
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    protected VTextWin twin;
    public String fg = null;
    public String bg = null;
    public Color  fgColor = null;
    public Color  bgColor, orgBg;

    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Dimension tmpDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);


    public VText(SessionShare sshare, ButtonIF vif, String typ) {
	this.vnmrIf = vif;
	setOpaque(false);
	setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
        setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
	twin = new VTextWin(this, sshare, vif, typ);
	setViewportView(twin);
	JViewport vp = getViewport();
	vp.setBackground(Util.getBgColor());

	orgBg = getBackground();

	ml = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                    }
                }
             }
	};
	new DropTarget(this, this);
	DisplayOptions.addChangeListener(this);
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
	JViewport vp = getViewport();
	if (vp != null)
	    vp.setBackground(Util.getBgColor());
    }

    public void destroy() {
    }

    public void setDefLabel(String s) {
	twin.setAttribute(VALUE, s);
    }

    public void setDefColor(String c) {
	twin.setAttribute(FGCOLOR, c);
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
	twin.setEditStatus(s);
        repaint();
    }

    public void setEditMode(boolean s) {
	twin.setEditMode(s);
	setOpaque(s);
	if (s) {
           addMouseListener(ml);
           curLoc.x = defLoc.x;
           curLoc.y = defLoc.y;
	   defDim = getPreferredSize();
           curDim.width = defDim.width;
           curDim.height = defDim.height;
        }
        else
           removeMouseListener(ml);
	inEditMode = s;
    }

    public void changeFont() {
	twin.changeFont();
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    public String getAttribute(int attr) {
	return twin.getAttribute(attr);
    }

    public void setAttribute(int attr, String c) {
	twin.setAttribute(attr, c);
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void setValue(ParamIF pf) {
        if (pf != null) {
	    twin.setAttribute(VALUE, pf.value);
        }
    }

    public void setShowValue(ParamIF pf) {
	twin.setShowValue(pf);
    }

    public void updateValue() {
	twin.updateValue();
    }


    public void paint(Graphics g) {
	super.paint(g);
	if (!isEditing)
	    return;
	Dimension  psize = getPreferredSize();
	if (isFocused)
            g.setColor(Color.yellow);
	else
            g.setColor(Color.green);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }

    public void refresh() {
    }

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
		VObjDropHandler.processDrop(e, this, inEditMode);
    }
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

    public void setSizeRatio(double x, double y) {
	double rx = x;
	double ry = y;
        if (rx > 1.0)
            rx = x - 1.0;
        if (ry > 1.0)
            ry = y - 1.0;
	if (defDim.width <= 0)
	    defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * rx);
        curLoc.y = (int) ((double) defLoc.y * ry);
        curDim.width = (int) ((double)defDim.width * rx);
        curDim.height = (int) ((double)defDim.height * ry);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
	twin.setSizeRatio(x, y);
    }

    public void setDefLoc(int x, int y) {
         defLoc.x = x;
         defLoc.y = y;
    }

    public Point getDefLoc() {
         tmpLoc.x = defLoc.x;
         tmpLoc.y = defLoc.y;
         return tmpLoc;
    }

    public void reshape(int x, int y, int w, int h) {
        if (inEditMode) {
           defLoc.x = x;
           defLoc.y = y;
           defDim.width = w;
           defDim.height = h;
        }
        curLoc.x = x;
        curLoc.y = y;
        curDim.width = w;
        curDim.height = h;
        super.reshape(x, y, w, h);
    }

    public Point getLocation() {
        if (inEditMode) {
           tmpLoc.x = defLoc.x;
           tmpLoc.y = defLoc.y;
        }
        else {
           tmpLoc.x = curLoc.x;
           tmpLoc.y = curLoc.y;
        }
        return tmpLoc;
    }
}

