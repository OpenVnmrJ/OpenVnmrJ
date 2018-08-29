/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**
 * Title :       VMakePanelsTag.java
 * Description:  class used by by SpinCAD & JPsg for creating a "makepanel"
 *      element in the xml file displaying list of new parameters to be created. This
 *      element has an entry field which contains the name of layout directory to be
 *      used as a starting point for panel file and directory creation
 *
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
import javax.swing.text.*;
import javax.swing.plaf.*;
import javax.swing.border.*;
import javax.swing.event.*;
import vnmr.util.*;
import vnmr.ui.*;


public class VMakePanelsTag extends JTextField implements VEditIF, VObjIF, VObjDef, DropTargetListener,PropertyChangeListener
{
    public String type = null;
    public String label = null;
    public String value = null;
    public String precision = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String m_strDisAbl = null;
    public Color  fgColor = null;
    public Color  bgColor, orgBg;
    public Font   font = null;
    private int isActive = 1;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    protected boolean isEnabled = true;
    protected String keyStr = null;
    protected String valStr = null;
    private boolean inModalMode = false;
    private VMakePanelsTag m_objEntry;
    private VTextUndoMgr undo=null;

    private final static String[] m_arrStrDisAbl = {"Grayed out", "Label" };

    /** The array of the attributes that are displayed in the edit template.*/
    private final static Object[][] m_attributes = {
        {new Integer(VARIABLE), "Vnmr variables:    "},
        {new Integer(SETVAL),   "Value of item:"},
        {new Integer(SHOW),     "Enable condition:"},
        {new Integer(CMD),      "Vnmr command:"},
        {new Integer(NUMDIGIT), "Decimal Places:"},
        {new Integer(DISABLE),  "Disable Style:", "menu", m_arrStrDisAbl}
    };

    public VMakePanelsTag(SessionShare sshare, ButtonIF vif, String typ) {
        this.type = typ;
        this.vnmrIf = vif;
        this.fg = "black";
        this.fontSize = "8";
        m_objEntry = this;
        setText("");
        orgBg = VnmrRgb.getColorByName("darkGray");
        bgColor = Util.getBgColor();
        setBackground(bgColor);
        setHorizontalAlignment(JTextField.LEFT);
        setMargin(new Insets(0, 2, 0, 2));
        addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent ev) {
                entryAction();
            }
        });

        addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent evt) {
                    focusLostAction();
            }
            public void focusGained(FocusEvent evt) {
                    focusGainedAction();
            }
        });

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
        bgColor = Util.getBgColor();
        setBackground(bgColor);
        changeFont();
     }

    /*
    protected void processEvent(AWTEvent e) {
        String str = e.toString();
        String msg = str.substring(str.indexOf('['), str.indexOf(']')+1);
        System.out.println("processEvent("+msg+")");
        super.processEvent(e);
    }
    */

    public void setDefLabel(String s) {
        this.value = s;
        setText(s);
        setScrollOffset(0);
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        if (s) {
           addMouseListener(ml);
           setOpaque(s);
        }
        else {
           removeMouseListener(ml);
           if ((bg != null) || (isActive < 1))
              setOpaque(true);
           else
              setOpaque(false);
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

    /**
     *  Returns the attributes array object.
     */
    public Object[][] getAttributes()
    {
        return m_attributes;
    }

    public String getAttribute(int attr) {
        switch (attr) {
          case TYPE:
                     return type;
          case LABEL:
                     return null;
          case VALUE:
                     value = getText();
                     if (value != null)
                        value = value.trim();
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
          case VARIABLE:
                     return vnmrVar;
          case CMD:
                     return vnmrCmd;
          case SETVAL:
                     return setVal;
          case NUMDIGIT:
                     return precision;
          case KEYSTR:
                     return keyStr;
          case KEYVAL:
                     return keyStr==null ? null: getText();
          case DISABLE:
                     return m_strDisAbl;
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
                        bgColor = Util.getBgColor();
                        if (isActive < 1)
                           setOpaque(true);
                        else
                           setOpaque(inEditMode);
                     }
                     setBackground(bgColor);
                     repaint();
                     break;
          case SHOW:
                     showVal = c;
                     setBorder(isEnabled);
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
          case CMD:
                     vnmrCmd = c;
                     break;
          case NUMDIGIT:
                     precision = c;
                     break;
          case LABEL:
                     label=c;
                     break;
          case VALUE:
                     if (c != null)
                        c = c.trim();
                     value = c;
                     setText(c);
                     break;
          case KEYSTR:
                         keyStr=c;
                     break;
          case KEYVAL:
                     setText(c);
                     break;
          case DISABLE:
                     m_strDisAbl = c;
                     break;
        }
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

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void updateValue() {
        if (vnmrIf == null)
            return;
        if (showVal != null) {
            vnmrIf.asyncQueryShow(this, showVal);
        }
        else if (setVal != null) {
            vnmrIf.asyncQueryParam(this, setVal, precision);
        }
    }

    public void setValue(ParamIF pf) {
        if (pf != null) {
            value = pf.value;
        if (!getText().equals(value)) {
                if (value != null)
                    value = value.trim();
                setText(value);
                setScrollOffset(0);
                if (getCaretPosition() > value.length())
                    setCaretPosition(value.length());
            }
        }
    }

    public void setShowValue(ParamIF pf) {
        int newActive = isActive;
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            newActive = Integer.parseInt(s);
            if (newActive > 0) {
                setOpaque(true);
                setBackground(bgColor);
            }
            else {
                setOpaque(true);
                if (newActive == 0)
                    setBackground(Global.IDLECOLOR);
                else
                    setBackground(Global.NPCOLOR);
            }
            if (newActive >= 0) {
                setEnabled(true);
                isEnabled=true;
                if (setVal != null) {
                    vnmrIf.asyncQueryParam(this, setVal, precision);
                }
            }
            else {
                setEnabled(false);
                isEnabled = false;
            }
            setBorder(isEnabled);
        }
        if (newActive != isActive) {
            isActive = newActive;
            repaint();
        }
    }

    private void setBorder(boolean isEnabled)
    {
        JTextField txfTmp = new JTextField();
        if (isEnabled)
            setBorder(txfTmp.getBorder());
        else
        {
            String strStyle = getAttribute(DISABLE);
            if (strStyle != null && strStyle.equals(m_arrStrDisAbl[1]))
            {
                setBorder(null);
                setBackground(bgColor);
            }
            else
            {
                setBorder(txfTmp.getBorder());
                setBackground(Global.NPCOLOR);
            }
        }
    }

    private void focusLostAction() {
        if(undo!=null)
           Undo.removeUndoMgr(undo,this);

        if (inModalMode || inEditMode || vnmrCmd == null)
            return;
        if ((isActive < 0) || (vnmrIf == null))
            return;
        String d = getText();
        if (d != null)
            d = d.trim();

        if(value != null) {
            value = value.trim();
            if ( ! value.equals(d)) {
                value = d;
                vnmrIf.sendVnmrCmd(this, vnmrCmd);
            }
        }
        else if (d != null) {
            value = d;
            vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
    }
    private void focusGainedAction() {
        //if (inModalMode || inEditMode || vnmrCmd == null)
         //   return;
        //if ((isActive < 0) || (vnmrIf == null))
         //   return;
            if(undo==null)
                undo=new VTextUndoMgr(this);
            Undo.setUndoMgr(undo,this);
    }

    private void entryAction() {
        if (inModalMode || inEditMode || vnmrCmd == null)
            return;
        if (!hasFocus())
            return;
        if (vnmrIf == null)
            return;
        value = getText();
        if (value != null)
            value = value.trim();
        if (isActive >= 0) {
            vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
    }

    public void refresh() { }
    public void  destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void setSizeRatio(double x, double y) { }

    public Point getDefLoc() {
     return new Point(0,0);
    }

    public void setDefLoc(int x, int y) {}
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
                VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {

        value = getText();
        if (value != null)
            value = value.trim();
        (AcqPanelUtility.INSTANCE).makeAcqPanels(getAttribute(VALUE));
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }
}
