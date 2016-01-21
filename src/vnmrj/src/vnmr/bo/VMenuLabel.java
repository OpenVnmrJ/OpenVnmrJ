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
import vnmr.util.*;
import vnmr.ui.*;

public class VMenuLabel extends JLabel implements VObjIF, VObjDef, DropTargetListener,
					    PropertyChangeListener {
    public String type = null;
    public String chval = null;
    public String value = null;
    public String vnmrVar = null;
    public String showVal = null;
    public String setVal = null;
    public String fg = null;
    public String bg = null;
    public Color  fgColor;
    public Color  bgColor, orgBg;

    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private boolean bVpMenu = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private Point tmpLoc = new Point(0, 0);
    

    public VMenuLabel(SessionShare sshare, ButtonIF vif, String typ) {
	this.type = typ;
	this.vnmrIf = vif;
	setOpaque(false);
	orgBg = getBackground();
	bgColor = Util.getBgColor();
	setBackground(bgColor);
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
	if (fg != null)
	{
	    fgColor=DisplayOptions.getColor(fg);
	    setForeground(fgColor);
	}
	changeFont();
	bgColor = Util.getBgColor();
	setBackground(bgColor);
    }

    public String getType() {
        return type;
    }

    public String getLabel() {
        return getText();
    }

    public void setDefLabel(String s) {
    	this.value = s;
	setText(s);
    }

    public void setDefColor(String c) {
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
	}
        else {
           removeMouseListener(ml);
	}
	inEditMode = s;
    }

    public void changeFont() {
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
                     return value;
          case CHVAL:
                     return chval;
          case SHOW:
                     return showVal;
	  case VARIABLE:
                     return vnmrVar;
          case SETVAL:
                     return setVal;
	  case FGCOLOR:
                     return fg;
          case BGCOLOR:
                     return bg;
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
                     value = c;
                     setText(c);
                     break;
          case CHVAL:
                     chval=c;
                     break;
          case SHOW:
                     showVal = c;
                     break;
	  case VARIABLE:
                     vnmrVar = c;
                     break;
          case SETVAL:
                     setVal = c;
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

        }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void setVpMenu(boolean b) {
        bVpMenu = b;
    }

    public void setValue(ParamIF pf) {
        if (pf != null) {
            if (!pf.value.equals(value)) {
               value = pf.value;
               setText(value);
               if (bVpMenu)
                 Util.updateVpMenuLabel();
            }
        }
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            int isActive = Integer.parseInt(s);
            if (isActive > 0) {
                 if (!isEnabled()) {
                    setEnabled(true);
                    if (bVpMenu)
                       Util.updateVpMenuStatus();
                }
            }
            else {
                 if (isEnabled()) {
                    setEnabled(false);
                    if (bVpMenu)
                       Util.updateVpMenuStatus();
                }
            }
        }
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

    public void refresh() {
    }

    public void destroy() { }
    public void setDefLoc(int x, int y) {}
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}
    public void drop(DropTargetDropEvent e) {
         e.rejectDrop();
         return;
    }
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

    public Point getDefLoc() {
        return tmpLoc;
    }

    public void setSizeRatio(double x, double y) { }
}

