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
import java.awt.datatransfer.*;
import java.awt.event.*;
import java.io.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VTab extends JLabel
implements VObjIF, VObjDef, DropTargetListener, PropertyChangeListener
{
    public String type = null;
    public String label = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String vnmrCmd2 = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public Color  fgColor;
    public Color  bgColor;
    public Font   font = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private int isActive = 1;

    private FontMetrics fm = null;
    private float fontRatio = 1;
    private int rWidth = 0;
    private int rWidth2 = 0;
    private int nWidth = 0;
    private int rHeight = 0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);


    public VTab(SessionShare sshare, ButtonIF vif, String typ) {
	this.type = typ;
	this.vnmrIf = vif;
	this.fg = "PlainText";
        this.fontSize = "8";
        this.fontStyle = "Plain";
        this.label = "tab";
	setText(this.label);
	bgColor = Util.getBgColor();
	setBackground(bgColor);
	setOpaque(true);
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


    private ParamLayout getParamLayout(){
        Container cont = (Container)getParent();
        while (cont != null) {
           if (cont instanceof ParamLayout)
               break;
           cont = cont.getParent();
        }
        return (ParamLayout) cont;
    }

    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
	if(fg!=null){
	    fgColor=DisplayOptions.getColor(fg);
	    setForeground(fgColor);
        }
	bgColor = Util.getBgColor();
	setBackground(bgColor);
	changeFont();
     }

    public void setDefLabel(String s) {
    	this.label = s;
	    setText(s);
	rWidth = 0;
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
	/*if (bg == null)
	   setOpaque(s);*/

	if (s) {
           addMouseListener(ml);
           if (font != null)
                setFont(font);
           fontRatio = 1.0f;
	   defDim = getPreferredSize();
           curLoc.x = defLoc.x;
           curLoc.y = defLoc.y;
           curDim.width = defDim.width;
           curDim.height = defDim.height;
           rWidth2 = rWidth;
	}
        else {
           removeMouseListener(ml);
	}
	inEditMode = s;
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
	fm = getFontMetrics(font);
        calSize();
        fontRatio = 1.0f;
        if (!inEditMode) {
             if ((curDim.width > 0) && (rWidth > curDim.width)) {
                 adjustFont(curDim.width, curDim.height);
             }
        }
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
	  case LABEL:
	  case VALUE:
		     return label;
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
	  case SETVAL:
            	     return setVal;
	  case VARIABLE:
		     return vnmrVar;
	  case TAB:
	    	     return Boolean.TRUE.toString();
	  default:
		    return null;
	}
    }

    public void setAttribute(int attr, String c) {
	switch (attr) {
	  case TYPE:
		     type = c;
		     break;
	  case LABEL:
	  case VALUE:
		     setText(c);
		     if(inEditMode && c !=label){
        	         ParamLayout p = getParamLayout();
        	         if(p!=null && p.isTabLabel(label))
        	             p.rebuildTabs();
        	     }
		     label = c;
		     rWidth = 0;
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
                        //setOpaque(true);
                     }
                     else {
                        bgColor = Util.getBgColor();
                        //setOpaque(true);
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
	}
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void setValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
	     label = pf.value;
	     setText(label);
	     rWidth = 0;
        }
    }

    public void setShowValue(ParamIF pf) {
	if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            isActive = Integer.parseInt(s);
            if (isActive > 0) {
                setBackground(bgColor);
		//setOpaque(false);
            }
            else {
		setOpaque(true);
                if (isActive == 0)
                    setBackground(Global.IDLECOLOR);
                else
                    setBackground(Global.NPCOLOR);
            }
            if (isActive >= 0) {
                setEnabled(true);
                if (setVal != null) {
                    vnmrIf.asyncQueryParam(this, setVal);
                }
            }
            else {
                setEnabled(false);
            }
        }
    }

    public void updateValue() {
	if (vnmrIf == null)
	    return;
        if (showVal != null) {
	    vnmrIf.asyncQueryShow(this, showVal);
        }
        else if (setVal != null) {
	    vnmrIf.asyncQueryParam(this, setVal);
        }
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


    public void refresh() {
    }

    public void addDefChoice(String c) {}
    public void addDefValue(String c) {}

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
	    VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop
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

    public void calSize() {
        if (fm == null) {
            font = getFont();
            fm = getFontMetrics(font);
        }
        rHeight = font.getSize();
        rWidth = 4;
        if (label != null)
            rWidth += fm.stringWidth(label);
        if (getIcon() != null)
            rWidth += getIcon().getIconWidth();
        rWidth2 = rWidth;
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
        if (!inEditMode) {
           if (rWidth <= 0) {
                calSize();
           }
           if ((w != nWidth) || (w < rWidth2)) {
              adjustFont(w, h);
           }
        }
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

    public Point getLocation(Point pt) {
        return getLocation();
    }


    public void adjustFont(int w, int h) {
        Font  curFont = null;

        if (w <= 0)
           return;
        if (label == null || (label.length() <= 0))
           return;
        nWidth = w;
        if (w > rWidth2) {
           if (fontRatio >= 1.0f)
              return;
        }
        float s = (float) w / (float) rWidth;
        if (rWidth > w) {
           if (s >= 0.98f)
                s = 0.98f;
           if (s < 0.5f)
                s = 0.5f;
        }
        if (s > 1)
            s = 1;
        if (s == fontRatio)
	    return;
        fontRatio = s;
        s = (float) rHeight * fontRatio;
        if ((s < 10) && (rHeight > 10))
            s = 10;
        if (fontRatio < 1) {
            String fname = font.getName();
            if (!fname.equals("Dialog")) {
                s = rHeight;
                //curFont = new Font("Dialog", font.getStyle(), (int)s);
                curFont = DisplayOptions.getFont("Dialog", font.getStyle(), (int)s);
            }
            else
                //curFont = font.deriveFont(s);
                curFont = DisplayOptions.getFont(font.getName(),font.getStyle(),(int)s);
        }
        else
            curFont = font;
        String strfont = curFont.getName();
        int nstyle = curFont.getStyle();
        while (s > 10) {
            FontMetrics fm2 = getFontMetrics(curFont);
            rWidth2 = 0;
            int fh = curFont.getSize();
            if (label != null)
                 rWidth2 += fm2.stringWidth(label);
            if (getIcon() != null)
                 rWidth2 += getIcon().getIconWidth();
            if ((rWidth2 < nWidth) && (fh < h))
                 break;
            s = s - 0.5f;
            if (s < 10)
                break;
            //curFont = curFont.deriveFont(s);
            curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
        }
        if(rWidth2 > w)
            rWidth2 = w;
        setFont(curFont);
    }

}

