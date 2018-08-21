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
import java.beans.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import javax.swing.plaf.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VClockDial extends ClockDial implements VObjIF, VEditIF,
    StatusListenerIF, VObjDef, DropTargetListener, PropertyChangeListener
{
    public String type = null;
    public String value = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String digital = "false";
    public String isElastic = "false";
    public String isPieSlice = "true";
    public String isColorBars = "false";
    public String[] barColors = {
        "black",
        "0x999999",
        "0xbb00ff",
        "blue",
        "cyan",
        "0x00bb00",
        "0x99ff99",
        "yellow",
        "orange",
        "red",
    };
    public String min = "0";
    public String max = "100";
    public String showMin = "false";
    public String showMax = "false";
    public Color  fgColor = null;
    public Color  bgColor, orgBg;
    public Font   font = null;
    private String nHands = "1";
    private String maxColor = null;
    private String statusParam = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;

    private int rWidth = 0;
    private int rWidth2 = 0;
    private int nWidth = 0;
    private int rHeight = 0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);


    public VClockDial(SessionShare sshare, ButtonIF vif, String typ) {
	this.type = typ;
	this.vnmrIf = vif;
	this.fg = "black";
        this.fontSize = "8";
	this.fontStyle = "Bold";
        this.value = "0";
	this.statusParam="";
	setOpaque(false);
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

    // PropertyChangeListener interface

	public void propertyChange(PropertyChangeEvent evt){
		if(fg!=null){
            fgColor=DisplayOptions.getColor(fg);
        	setForeground(fgColor);
        }
		if(maxColor!=null) 	   
		 	setMaxColor(DisplayOptions.getColor(maxColor));
 		changeFont();
    }

    // private methods

    private boolean booleanValue(String s){
        if(s.equals("True") || s.equals("true") || s.equals("yes"))
            return true;
        return false;
    }

    private String booleanString(String s){
        if(s.equals("True") || s.equals("true") || s.equals("yes"))
            return "yes";
        return "no";
    }
	
    // VObjIF interface

    public void setDefLabel(String s) {
    	this.value = s;
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
	if (bg == null)
	   setOpaque(s);
	if (s) {
           addMouseListener(ml);
           if (font != null)
                setFont(font);
	   defDim = getPreferredSize();
	   curLoc.x = defLoc.x;
           curLoc.y = defLoc.y;
           curDim.width = defDim.width;
           curDim.height = defDim.height;
	}
        else {
           removeMouseListener(ml);
	}
	inEditMode = s;
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        repaint();
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
	    return type;
        case FGCOLOR:
	    return fg;
        case BGCOLOR:
	    return bg;
        case SHOW:
	    return showVal;
        case FONT_NAME:
	    return fontName;
        case FONT_STYLE:
	    return fontStyle;
        case FONT_SIZE:
	    return fontSize;
        case VARIABLE:
	    return vnmrVar;
        case SETVAL:
	    return setVal;
        case DIGITAL:
	    return digital;
        case MIN:
	    return min;
        case MAX:
	    return max;
        case SHOWMIN:
	    return showMin;
        case SHOWMAX:
	    return showMax;
        case STATPAR:
	    return statusParam;
        case COUNT:
	    return nHands;
        case COLOR1:
	    return maxColor;
        case ELASTIC:
	    return isElastic;
        case DECOR1:
	    return isPieSlice;
        case DECOR2:
	    return isColorBars;
        case COLOR3:
	    return barColors[0];
        case COLOR4:
	    return barColors[1];
        case COLOR5:
	    return barColors[2];
        case COLOR6:
	    return barColors[3];
        case COLOR7:
	    return barColors[4];
        case COLOR8:
	    return barColors[5];
        case COLOR9:
	    return barColors[6];
        case COLOR10:
	    return barColors[7];
        case COLOR11:
	    return barColors[8];
        case COLOR12:
	    return barColors[9];
        default:
	    return null;
        }
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
        case TYPE:
	    type = c;
	    break;
        case FGCOLOR:
	    fg = c;
 	    fgColor=DisplayOptions.getColor(fg);
	    setForeground(fgColor);
	    repaint();
	    break;
        case BGCOLOR:
	    bg = c;
	    if (c != null) {
		bgColor = VnmrRgb.getColorByName(c);
		setOpaque(true);
	    }
	    else {
		bgColor = orgBg;
		setOpaque(inEditMode);
	    }
	    setBackground(bgColor);
	    repaint();
	    break;
        case SHOW:
	    showVal = c;
	    break;
        case FONT_NAME:
	    fontName = c;
	    break;
        case FONT_STYLE:
	    fontStyle = c;
	    break;
        case FONT_SIZE:
	    fontSize = c;
	    break;
        case SETVAL:
	    setVal = c;
	    break;
        case VARIABLE:
	    vnmrVar = c;
	    break;
        case DIGITAL:
	    setDigitalReadout(booleanValue(c));
	    digital=booleanString(c);
	    break;
        case ELASTIC:
	    setBoundsAreElastic(booleanValue(c));
	    isElastic = booleanString(c);
	    break;
        case SHOWMIN:
	    setMinMarker(booleanValue(c));
	    showMin=booleanString(c);
	    break;
        case SHOWMAX:
	    setMaxMarker(booleanValue(c));
	    showMax = booleanString(c);
	    break;
        case MIN:
	    try {
		setMinimum(Double.parseDouble(c));
		min = c;
	    } catch (NumberFormatException ex) { }
	    break;
        case MAX:
	    try {
		setMaximum(Double.parseDouble(c));
		max = c;
	    } catch (NumberFormatException ex) { }
	    break;
        case STATPAR:
	    if (c != null)
	        statusParam = c.trim();
	    else
	        statusParam = null;
	    if (statusParam != null)
	        updateStatus(ExpPanel.getStatusValue(statusParam));
	    break;
        case COUNT:
	    nHands = c;
	    setLongHand(nHands.equals("2"));
	    break;
        case COLOR1:
            maxColor = c;
 	    setMaxColor(DisplayOptions.getColor(c));
	    break;
        case DECOR1:
            isPieSlice = booleanString(c);
            setPieSlice(booleanValue(c));
            break;
        case DECOR2:
            isColorBars = booleanString(c);
            setColorBars(booleanValue(c));
            break;
        case COLOR3:
            barColors[0] = c;
 	    setBarColor(0, DisplayOptions.getColor(c));
	    break;
        case COLOR4:
            barColors[1] = c;
 	    setBarColor(1, DisplayOptions.getColor(c));
	    break;
        case COLOR5:
            barColors[2] = c;
 	    setBarColor(2, DisplayOptions.getColor(c));
	    break;
        case COLOR6:
            barColors[3] = c;
 	    setBarColor(3, DisplayOptions.getColor(c));
	    break;
        case COLOR7:
            barColors[4] = c;
 	    setBarColor(4, DisplayOptions.getColor(c));
	    break;
        case COLOR8:
            barColors[5] = c;
 	    setBarColor(5, DisplayOptions.getColor(c));
	    break;
        case COLOR9:
            barColors[6] = c;
 	    setBarColor(6, DisplayOptions.getColor(c));
	    break;
        case COLOR10:
            barColors[7] = c;
 	    setBarColor(7, DisplayOptions.getColor(c));
	    break;
        case COLOR11:
            barColors[8] = c;
 	    setBarColor(8, DisplayOptions.getColor(c));
	    break;
        case COLOR12:
            barColors[9] = c;
 	    setBarColor(9, DisplayOptions.getColor(c));
	    break;
        }
	repaint();
    }


    public void paint(Graphics g) {
	super.paint(g);
	if (!isEditing)
	    return;
	Dimension  psize = getSize();
	if (isFocused)
            g.setColor(Color.yellow);
        else
            g.setColor(Color.green);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }

    public ButtonIF getVnmrIF() {
	return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
	vnmrIf = vif;
    }

    public void updateValue() {
	if (vnmrIf == null)
	    return;
        if (setVal != null) {
	    vnmrIf.asyncQueryParam(this, setVal);
        }
        if (showVal != null) {
	    vnmrIf.asyncQueryShow(this, showVal);
        }
    }

    public void setValue(ParamIF pf) {
	if (pf != null) {
    	    value = pf.value;
	    try {
		super.setValue(Double.parseDouble(value));
	    } catch (NumberFormatException ex) { }
	}
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            if (s.equals("0")) {
                setEnabled(false);
            }
            else
                setEnabled(true);
        }
    }

    public void updateStatus(String msg) {
	if (msg == null || msg.length() == 0) {
	    return;
	}
	StringTokenizer tok = new StringTokenizer(msg);
	if (tok.hasMoreTokens()) {
	    String parm = tok.nextToken();
	    if (parm.equals(statusParam)) {
		String v = tok.nextToken();
		try {
		    super.setValue(Double.parseDouble(v));
		    value = v;
		} catch (NumberFormatException ex) {
		    super.setValue(0);
		    value = "0";
		}
	    }
	}
    }

    public void refresh() {
    }
    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
		VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop

    public Object[][] getAttributes()
    {
	return attributes;
    }

    private final static String[] yes_no = {"yes", "no"};
    private final static String[] one_two = {"1", "2"};
    private final static Object[][] attributes = {
	{new Integer(VARIABLE), "Vnmr variables:"},
	{new Integer(SETVAL), "Value of item:"},
	{new Integer(SHOW), "Enable condition:"},
	{new Integer(STATPAR), "Status variables:"},
	{new Integer(MIN), "Min value:"},
	{new Integer(MAX), "Max value:"},
	{new Integer(ELASTIC), "Max value elastic:", "radio", yes_no},
	{new Integer(COUNT), "Number of hands:", "radio", one_two},
	{new Integer(DIGITAL), "Digital readout:", "radio", yes_no},
	{new Integer(SHOWMAX), "Show max value:", "radio", yes_no},
	{new Integer(COLOR1), "Max marker color:", "color"},
	{new Integer(DECOR1), "Show pie slice:", "radio", yes_no},
	{new Integer(DECOR2), "Show color bars:", "radio", yes_no},
    };

    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

    public void setSizeRatio(double x, double y) {
        double xRatio =  x;
        double yRatio =  y;
        if (x > 1.0)
            xRatio = x - 1.0;
        if (y > 1.0)
            yRatio = y - 1.0;
	if (defDim.width <= 0)
	    defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double)defDim.width * xRatio);
        curDim.height = (int) ((double)defDim.height * yRatio);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
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

