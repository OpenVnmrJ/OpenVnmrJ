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
import javax.swing.*;
import javax.swing.plaf.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VThermometer extends JProgressBar
    implements VObjIF, VEditIF, StatusListenerIF, VObjDef, DropTargetListener
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
    public boolean digital = false;
    public double min = 0;
    public double max = 100;
    public boolean showMin = false;
    public boolean showMax = false;
    public Color  fgColor = null;
    public Color  bgColor, orgBg;
    public Font   font = null;
    private String statusParam = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private String orient = "vertical";
    private MouseAdapter ml;
    private ButtonIF vnmrIf;

    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);


    public VThermometer(SessionShare sshare, ButtonIF vif, String typ) {
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
    }

    public void setDefLabel(String s) {
    	this.value = s;
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public boolean isRequestFocusEnabled()
    {
        return Util.isFocusTraversal();
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
	    	     return String.valueOf(digital);
	  case MIN:
	    	     return String.valueOf(min);
	  case MAX:
	    	     return String.valueOf(max);
	  case SHOWMIN:
	    	     return String.valueOf(showMin);
	  case SHOWMAX:
	    	     return String.valueOf(showMax);
	  case STATPAR:
	    	     return String.valueOf(statusParam);
	  case ORIENTATION:
	    	     return String.valueOf(orient);
          default:
                    return null;
        }
    }

    public void setAttribute(int attr, String c) {
	if (c != null)
	   c = c.trim();
        switch (attr) {
          case TYPE:
                     type = c;
                     break;
          case FGCOLOR:
                     fg = c;
                     fgColor = VnmrRgb.getColorByName(c);
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
                        bgColor = Util.getBgColor();
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
                     digital = Boolean.valueOf(c).booleanValue();
		     setStringPainted(digital);
                     break;
          case SHOWMIN:
                     showMin = Boolean.valueOf(c).booleanValue();
		     //setMinMarker(showMin);
		     break;
          case SHOWMAX:
                     showMax = Boolean.valueOf(c).booleanValue();
		     //setMaxMarker(showMax);
		     break;
          case MIN:
	             try {
			 min = Double.parseDouble(c);
		     } catch (NumberFormatException ex) { }
		     setMinimum(min);
                     break;
          case MAX:
	             try {
			 max = Double.parseDouble(c);
		     } catch (NumberFormatException ex) { }
		     setMaximum(max);
                     break;
          case STATPAR:
                     statusParam = c;
		     updateStatus(ExpPanel.getStatusValue(statusParam));
                     break;
          case ORIENTATION:
                     orient = c;
		     if (orient != null) {
		       if (orient.toLowerCase().startsWith("v")) {
			 setOrientation(JProgressBar.VERTICAL);
		       } else {
			 setOrientation(JProgressBar.HORIZONTAL);
		       }
		     }
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
		setValue(Double.parseDouble(value));
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
		    setValue(Double.parseDouble(v));
		    value = v;
		} catch (NumberFormatException ex) {
		    super.setValue(0);
		    value = "0";
		}
	    }
	}
    }

    public void setValue(double dval) {
	super.setValue((int)dval);
    }

    public void setMinimum(double dval) {
	super.setMinimum((int)dval);
    }

    public void setMaximum(double dval) {
	super.setMaximum((int)dval);
    }

    public void refresh() {
    }
    public void destroy() {
    }

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

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


    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        if (!inEditMode) {
            e.rejectDrop();
            return;
        }
        try {
            Transferable tr = e.getTransferable();
            if (tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR)) {
                Object obj =
                    tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
                if (obj instanceof VObjIF) {
                    e.acceptDrop(DnDConstants.ACTION_MOVE);
                    JComponent comp = (JComponent)obj;
                    Container cont = (Container)comp.getParent();
                    while (cont != null) {
                        if (cont instanceof ParamPanel)
                                break;
                        cont = cont.getParent();
                    }
                    e.getDropTargetContext().dropComplete(true);

                    if (cont != null) {
                        Point pt1 = cont.getLocationOnScreen();
                        Point pt2 = this.getLocationOnScreen();
                        Point pte = e.getLocation();
                        int x = pt2.x + pte.x - 10;
                        int y = pt2.y + pte.y - 10;
                        if (x < pt1.x) x = pt1.x;
                        if (y < pt1.y) y = pt1.y;
                        comp.setLocation(x, y);
                        ParamEditUtil.relocateObj((VObjIF) comp);
                    }
                    return;
                }
            }
        } catch (IOException io) {}
          catch (UnsupportedFlavorException ufe) { }
        e.rejectDrop();
    } // drop

    public Object[][] getAttributes()
    {
	return attributes;
    }

    private final static String[] orient_str = {"Vertical", "Horizontal"};
    private final static String[] true_false = {"True", "False"};
    private final static Object[][] attributes = {
	{new Integer(STATPAR), "Status parameter:"},
	{new Integer(VARIABLE), "Vnmr variables:"},
	{new Integer(SETVAL), "Vnmr value expression:"},
	{new Integer(MIN), "Min displayed value:"},
	{new Integer(MAX), "Max displayed value:"},
	{new Integer(ORIENTATION), "Orientation:", "menu", orient_str},
	{new Integer(DIGITAL), "Show digital readout:", "menu", true_false},
	{new Integer(BGCOLOR), "Background color:", "color"},
	//{new Integer(SHOWMAX), "Show max value marker:", "menu", true_false},
    };

    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}
}

