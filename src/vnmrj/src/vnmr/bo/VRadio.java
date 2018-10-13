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

public class VRadio extends JRadioButton implements VObjIF, VObjDef, VEditIF,
     DropTargetListener,PropertyChangeListener
{
    public String type = null;
    public String label = null;
    public String fg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String vnmrCmd2 = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public Color  fgColor = null;
    public Font   font = null;
    public Font   font2 = null;
    public String tipStr = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private boolean bFocusable = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private int isActive = 1;
    private boolean inModalMode = false;
    private String m_viewport = null;

    private FontMetrics fm = null;
    private float fontRatio = 1;
    private int rWidth = 0;
    private int rWidth2 = 0;
    private int nWidth = 0;
    private int rHeight = 0;
    static int iconWidth = 0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private boolean panel_enabled=true;
    private Insets myInsets = new Insets(0,0,0,0);

    public VRadio(SessionShare sshare, ButtonIF vif, String typ) {
    this.type = typ;
    this.vnmrIf = vif;
    this.fg = "PlainText";
        this.fontSize = "8";
        this.label = "";
    setText(this.label);
    setOpaque(true);
    //setBackground(Util.getBgColor());
    setOpaque(false);
 
    addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
        boolean flag = ((JRadioButton)e.getSource()).isSelected();
                radioAction(e, flag);
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
       bFocusable = isFocusable();
       setFocusable(false);
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
        if(fg!=null){
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        //setBackground(null);
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

    public boolean isRequestFocusEnabled()
    {
        return Util.isFocusTraversal();
    }

    public Insets getInsets() {
        return myInsets;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        if(s) {
            addMouseListener(ml);
            if(font != null)
                setFont(font);
            fontRatio = 1.0f;
            defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
            rWidth2 = rWidth;
            setFocusable(bFocusable);
        } else {
            removeMouseListener(ml);
            setFocusable(false);
        }
        inEditMode = s;
    }

    public void changeFont() {
        font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
        setFont(font);
        rHeight = font.getSize();
        font2 = null;
        fontRatio = 1.0f;
        fm = getFontMetrics(font);
        calSize();
        if(!isPreferredSizeSet()) {
            setDefSize();
        }
        if(label != null) {
            if(!inEditMode) {
                if((curDim.width > 0) && (rWidth > curDim.width)) {
                    adjustFont();
                }
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
            return label;
        case FGCOLOR:
            return fg;
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
        case CMD:
            return vnmrCmd;
        case CMD2:
            return vnmrCmd2;
        case TOOL_TIP:
            return null;
        case TOOLTIP:
            return tipStr;
        case VALUE:
            if (isSelected())
                return "1";
            else
                return "0";
       case TRACKVIEWPORT:
            return m_viewport;
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
            label = c;
            if (tipStr == null && c != null) {
               if (c.length() > 0) {
                  String tp = Util.getTooltipText(c, null);
                  if (tp != null && tp.length() > 0)
                     setToolTipText(tp);
               }
            }
            setText(c);
            rWidth = 0;
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
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
        case CMD:
            vnmrCmd = c;
            break;
        case CMD2:
            vnmrCmd2 = c;
            break;
        case VALUE:
            if (c.equals("1"))
                setSelected(true);
            else
                setSelected(false);
            break;
        case TOOL_TIP:
        case TOOLTIP:
            tipStr = c;
            if (c != null && c.length() > 0)
               setToolTipText(Util.getLabel(c));
            else
               setToolTipText(null);
            break;
        case ENABLED:
            panel_enabled=c.equals("false")?false:true;
	    if(!panel_enabled) setSelected(false);
            setEnabled(c.equals("false")?false:true);
            break;
        case TRACKVIEWPORT:
            m_viewport = c;
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
       if (pf.value != null) {
                if (pf.value.equals("0"))
                    setSelected(false);
                else
                    setSelected(true);
           }
        }
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            isActive = Integer.parseInt(s);
            if (panel_enabled && isActive >= 0) {
                setEnabled(true);
            }
            else {
                // if (!panel_enabled && setVal != null && isActive >= 0)
                //     vnmrIf.asyncQueryParam(this, setVal);
	    	setSelected(false);
                setEnabled(false);
            }
            if (setVal != null)
                vnmrIf.asyncQueryParam(this, setVal);
        }
    }

    public void updateValue() {
        if (m_viewport != null && m_viewport.toLowerCase().startsWith("y")) {
            vnmrIf = Util.getActiveView();
        }
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

    private void radioAction(ActionEvent ev, boolean set) {
        if (inModalMode || inEditMode || vnmrIf == null)
        return;
        if (isActive < 0)
        return;
        if (set) {
            if (vnmrCmd != null)
               vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
        else {
            if (vnmrCmd2 != null)
               vnmrIf.sendVnmrCmd(this, vnmrCmd2);
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

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if (vnmrIf == null)
            return;
        if (this.isSelected()) {
            if (vnmrCmd != null)
               vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
        else {
            if (vnmrCmd2 != null)
               vnmrIf.sendVnmrCmd(this, vnmrCmd2);
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

    private void setDefSize() {
        if (rWidth <= 0)
            calSize();
        defDim.width = rWidth + 8;
        defDim.height = rHeight + 8;
        setPreferredSize(defDim);
    }


    public void setSizeRatio(double x, double y) {
        double xRatio =  x;
        double yRatio =  y;
        if (x > 1.0)
            xRatio = x - 1.0;
        if (y > 1.0)
            yRatio = y - 1.0;
    if (defDim.width <= 0) {
        if (!isPreferredSizeSet())
                setDefSize();
        defDim = getPreferredSize();
    }
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double)defDim.width * xRatio);
        curDim.height = (int) ((double)defDim.height * yRatio);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
    }


    public void calSize() {
        if (fm == null) {
            font = getFont();
            fm = getFontMetrics(font);
            rHeight = font.getSize();
        }
        if (iconWidth == 0) {
            Icon icon = UIManager.getIcon("RadioButton.icon");
            if (icon != null)
                iconWidth = icon.getIconWidth() + getIconTextGap() + 2;
            else
                iconWidth = 8;
            icon = null;
        }
        rWidth = iconWidth;
        if (label != null)
            rWidth += fm.stringWidth(label);
        rWidth2 = rWidth;
    }

    public void reshape(int x, int y, int w, int h) {
        if (inEditMode) {
           defLoc.x = x;
           defLoc.y = y;
           defDim.width = w;
           defDim.height = h;
        }
        curDim.width = w;
        curDim.height = h;
        curLoc.x = x;
        curLoc.y = y;
        if (!inEditMode) {
            if (rWidth <= 0) {
                calSize();
            }
            if ((w != nWidth) || (w < rWidth2)) {
                adjustFont();
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

    public void adjustFont() {
        Font  curFont = null;

        if (curDim.width <= 0)
           return;
        int w = curDim.width;
        int h = curDim.height;
        nWidth = w;
        if (label == null || (label.length() <= 0)) {
            rWidth2 = 0;
           return;
        }
        if (w > rWidth2) {
           if (fontRatio >= 1.0f)
              return;
        }
        float s = (float)(w - iconWidth) / (float)(rWidth - iconWidth);
        if (rWidth > w) {
           if (s > 0.98f)
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
            if (font2 == null) {
                String fname = font.getName();
                if (!fname.equals("Dialog"))
                    //font2 = new Font("Dialog", font.getStyle(), rHeight);
                    font2 = DisplayOptions.getFont("Dialog", font.getStyle(), rHeight);
                else
                    font2 = font;
            }
            if (s < (float) rHeight)
                s += 1;
            //curFont = font2.deriveFont(s);
            curFont = DisplayOptions.getFont(font2.getName(), font2.getStyle(), (int)s);
        }
        else
            curFont = font;
        int fh = 0;
        String strfont = curFont.getName();
        int nstyle = curFont.getStyle();
        while (s > 7) {
            FontMetrics fm2 = getFontMetrics(curFont);
            rWidth2 = iconWidth;
            if (label != null)
                rWidth2 += fm2.stringWidth(label);
            fh = curFont.getSize();
            if ((rWidth2 <= nWidth) && (fh < h))
                break;
            if (s <= 7)
                break;
            s--;
            //curFont = curFont.deriveFont(s);
            curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
        }
        if (rWidth2 > w)
            rWidth2 = w;
        setFont(curFont);
    }
    public Object[][] getAttributes() {
        return attributes;
    }

    private final static Object[][] attributes = {
            { new Integer(LABEL), Util.getLabel(LABEL) },
            { new Integer(VARIABLE), Util.getLabel(VARIABLE) },
            { new Integer(SETVAL), Util.getLabel(SETVAL) },
            { new Integer(SHOW), Util.getLabel(SHOW) },
            { new Integer(CMD), Util.getLabel(CMD) },
            { new Integer(CMD2), Util.getLabel(CMD2) },
            { new Integer(STATPAR), Util.getLabel(STATPAR) }, 
            { new Integer(TOOLTIP), Util.getLabel(TOOLTIP) }
    };

}

