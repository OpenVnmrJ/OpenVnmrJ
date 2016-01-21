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

public class VTextPane extends JTextPane implements VObjIF, VObjDef,
     DropTargetListener, PropertyChangeListener
{
    public String type = null;
    public String value = null;
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
    public String editable = null;
    public Color  fgColor = Color.black;
    public Color  bgColor, orgBg;
    public Font   font = null;
    protected String wrap = "yes";

    private boolean isEditing = false;
    private boolean inEditMode = false;
    private FocusAdapter fl;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private VObjIF pobj;
    private Component pcomp;
    private DragSource dragSource;
    private DragGestureRecognizer dragRecognizer;
    private DragGestureListener dragListener;
    private boolean inModalMode = false;
    private VTextUndoMgr undo=null;

    private float fontRatio = 1;
    private Point defLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private double curXRatio = 1.0;
    //private Object lock = null;
    private boolean updating = false;

    public VTextPane(VObjIF p, SessionShare sshare, ButtonIF vif, String typ) {
        this.type = typ;
        this.vnmrIf = vif;
        this.pobj = p;
        this.pcomp = (Component) p;
        this.fg = "black";
        this.fontSize = "8";
        setText(" ");
        setEditable(false);
        setOpaque(false);
        orgBg = getBackground();
        setMargin(new Insets(2, 2, 2, 2));
        //setLineWrap(true);

        ml = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        ParamEditUtil.setEditObj(pobj);
                        ParamEditUtil.setEditObj2((VObjIF) evt.getSource());
                    }
                }
             }
        };
        fl = new FocusAdapter() {
        public void focusLost(FocusEvent evt) {
            focusLostAction();
        }
            public void focusGained(FocusEvent evt) {
                    focusGainedAction();
            }
        };
        addFocusListener(fl);
        DisplayOptions.addChangeListener(this);
        new DropTarget(this, this);
/*
        dragSource = new DragSource();
        dragListener = new VObjDragListener(dragSource);
        dragRecognizer = dragSource.createDefaultDragGestureRecognizer(null,
                          DnDConstants.ACTION_COPY, dragListener);
*/
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

    public FocusAdapter getFocusListener() { return fl; }

    public Component getContainer() {
        return (Component) pobj;
    }

    public void setDefLabel(String s) {
        this.value = s;
        setText(s);
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
/*
        if (s) {
           dragRecognizer.setComponent(this);
        }
        else {
           dragRecognizer.setComponent(null);
        }
*/
        repaint();
    }

    public void setEditMode(boolean s) {
        if (bg == null)
           setOpaque(s);
        if (s) {
           addMouseListener(ml);
           setFont(font);
	   fontRatio = 1;
	}
        else
           removeMouseListener(ml);
        inEditMode = s;
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
	fontRatio = 1;
	if (!inEditMode) {
	   if (curXRatio < 1.0)
	     adjustFont((float) curXRatio);
	}
        repaint();
    }

    public void changeFocus(boolean s) {
    }

    public String getAttribute(int attr) {
        switch (attr) {
          case TYPE:
                     return type;
          case LABEL:
                     return null;
          case VALUE:
                     return value;
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
          case EDITABLE:
                     return editable;
          case CMD:
                     return vnmrCmd;
          case WRAP:
                     return wrap;
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
                     break;
          case VALUE:
                     value = c;
                     setText(c);
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
          case EDITABLE:
                     if (c.equals("yes") || c.equals("true")){
                        setEditable(true);
                        editable = "yes";
                     }
                     else {
                        setEditable(false);
                        editable = "no";
                     }
                     break;
          case CMD:
                     vnmrCmd = c;
                     break;
          case WRAP:
                     if (c.equals("yes") || c.equals("true")) {
                         //setLineWrap(true);
                         wrap = "yes";
                     } else {
                         //setLineWrap(false);
                         wrap = "no";
                     }
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
        if (pf != null) {
            value = pf.value;
            setText(value);
        }
    }

    public void setShowValue(ParamIF pf) {
    }

    public void updateValue() {
        if (vnmrIf == null)
            return;
        if (setVal != null) {
            vnmrIf.asyncQueryParam(this, setVal);
        }
        if (showVal != null) {
        }
    }

    private void focusLostAction() {
        if (inModalMode || inEditMode || vnmrCmd == null || vnmrIf == null)
            return;
        String strvalue = getText();
        if (strvalue == null)
            strvalue = "";
        if (!strvalue.equals(value))
        {
            value = strvalue;
            vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
        Undo.removeUndoMgr(undo,this);
    }

    private void focusGainedAction() {
        if (inModalMode || inEditMode || vnmrCmd == null || vnmrIf == null)
            return;
            if(undo==null)
                undo=new VTextUndoMgr(this);
            Undo.setUndoMgr(undo,this);
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
    }

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if (vnmrCmd == null || vnmrIf == null)
            return;
        value = getText();
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    public void setSizeRatio(double x, double y) {
        double x1 =  x;
        double y1 =  y;
        if (x1 > 1.0)
            x1 = x1 - 1.0;
        if (y1 > 1.0)
            y1 = y1 - 1.0;
	if (x1 != curXRatio) {
	    curXRatio = x1;
	    if (!inEditMode)
	    	adjustFont((float)x1);
	}
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

    public void adjustFont(float r) {
        if (font == null) {
            font = getFont();
        }
        int rHeight = font.getSize();
        float newH = (float)rHeight * r;
        if (newH < 10)
            newH = 10;
        if (newH == fontRatio)
            return;
        fontRatio = newH;
        //Font  curFont = font.deriveFont(newH);
        Font curFont = DisplayOptions.getFont(font.getName(),font.getStyle(),(int)newH);
        setFont(curFont);
    }
}

