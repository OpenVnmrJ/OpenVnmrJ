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
import java.beans.*;
import java.util.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;

import vnmr.util.*;
import vnmr.ui.*;

public class VMagicButton extends JToggleButton implements VObjIF, VEditIF,
   VObjDef, DropTargetListener, StatusListenerIF,PropertyChangeListener
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
    public Color  fgColor = null;
    public String iconName=null;
    public String tipStr = null;
    public Font   font = null;
    public Font   font2 = null;
    private String isRadioButton = null;
    private String statusParam = null;
    private String statusEnable = null;
    private String statusValue = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private MouseAdapter mAction;
    private ButtonIF vnmrIf;
    private int isActive = 1;
    private boolean inModalMode = false;
    private boolean m_radio = false;

    private FontMetrics fm = null;
    private float fontRatio = 1;
    private int rWidth = 0;
    private int rWidth2 = 0;
    private int nWidth = 0;
    private int rHeight = 0;
    private int iconWidth = 0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private VButtonIcon icon = null;
    private ImageIcon jicon = null;

    public VMagicButton(SessionShare sshare, ButtonIF vif, String typ) {
        this.type = typ;
        this.vnmrIf = vif;
        this.fg = "black";
        this.fontSize = "8";
        this.isRadioButton = "no";
        setText(this.label);
        setOpaque(false);
        setMargin(new Insets(0, 0, 0, 0));
/*
        addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                boolean flag = ((JToggleButton)e.getSource()).isSelected();
                toggleAction(e, flag);
            }
        });
*/
        mAction = new MouseAdapter() {
              public void mousePressed(MouseEvent evt) {
                toggleAction(evt, true);
              }
              public void mouseReleased(MouseEvent evt) {
                toggleAction(evt, false);
              }
	};
	
        addMouseListener(mAction);

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

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        if (bg == null)
           setOpaque(s);
        if (s) {
           removeMouseListener(mAction);
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
           addMouseListener(mAction);
        }
        inEditMode = s;
    }

    private void setDefSize() {
        if (rWidth <= 0)
            calSize();
        defDim.width = rWidth + 8;
        defDim.height = rHeight + 8;
        setPreferredSize(defDim);
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        fontRatio = 1.0f;
        font2 = null;
        fm = getFontMetrics(font);
        calSize();
        if (!isPreferredSizeSet()) {
            setDefSize();
        }
        if (label != null) {
            if (!inEditMode) {
                if ((curDim.width > 0) && (rWidth > curDim.width)) {
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
        switch(attr) {
        case TYPE:
            return type;
        case LABEL:
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
        case CMD:
            return vnmrCmd;
        case CMD2:
            return vnmrCmd2;
        case VARIABLE:
            return vnmrVar;
        case SETVAL:
            return setVal;
        case ENABLED:
            if(isSelected())
                return "true";
            else
                return "false";
        case VALUE:
            if(isSelected())
                return "1";
            else
                return "0";
        case STATPAR:
            return statusParam;
        case STATVAL:
            return statusValue;
        case STATSHOW:
            return statusEnable;
        case RADIOBUTTON:
            return isRadioButton;
        case ICON:
            return iconName;
        case TOOL_TIP:
            return tipStr;
        default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        // Trim white space from string.
        // If string has only white space, make it null.
        if (c != null) {
            c = c.trim();
            if (c.length() == 0) c = null;
        }
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case LABEL:
            label = c;
            setText(c);
            rWidth = 0;
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            //setFgColor();
            setForeground(fgColor);
            //repaint();
            break;
        case SHOW:
            showVal = c;
            updateStatus(ExpPanel.getStatusValue(statusParam));
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
        case ENABLED:
            if (c.equals("1")||c.equals("true")||c.equals("yes"))
                setSelected(true);
            else
                setSelected(false);
            break;
        case VALUE:
            if (c.equals("1")||c.equals("true")||c.equals("yes"))
                setSelected(true);
            else
                setSelected(false);
            break;
        case STATPAR:
            statusParam = c;
            updateStatus(ExpPanel.getStatusValue(statusParam));
            break;
        case STATVAL:
            statusValue = c;
            updateStatus(ExpPanel.getStatusValue(statusParam));
            break;
        case STATSHOW:
            statusEnable = c;
            updateStatus(ExpPanel.getStatusValue(statusParam));
            break;
        case RADIOBUTTON:
            // Update obsolete values of keywords
            // (true/false keywords do not work with
            // ParamInfoPanel.showVariableFields, where labels displayed on
            // the panel must match the keywords in order to get initialized.)
            if (c.equals("false")) {
                isRadioButton = "no";
            } else if (c.equals("true")) {
                isRadioButton = "yes";
            } else {
                isRadioButton = c;
            }
            if(isRadioButton.equals("yes"))
                m_radio=true;
            else
                m_radio=false;               
            break;
        case ICON:
            iconName = c;
            icon = null;
            if(c != null) {
                String iname;
                if(c.indexOf('.') > 0)
                    iname=c;
                else
                    iname=c + ".gif";
                jicon = Util.getVnmrImageIcon(iname);
                if(jicon != null) {
                    icon = new VButtonIcon(jicon.getImage());
                    setIcon(jicon);
                }
                else{
                    jicon = Util.getImageIcon(iname);
                    if(jicon!=null){
                        icon = new VButtonIcon(jicon.getImage());
                        setIcon(jicon);
                    }
                }
            }     
            if(icon == null)
                setIcon(icon);
            break;
        case TOOL_TIP:
        case TOOLTIP:
            tipStr = c;
            if (c != null && c.length() > 0)
               setToolTipText(Util.getLabel(c));
            else
               setToolTipText(null);
            break;
        }
    }

    public boolean isPressed(){
        return getModel().isPressed();
    }

    public void setPressed(boolean b){
        getModel().setPressed(true);
        System.out.println(getAttribute(LABEL)+".setPressed("+b+")");
    }
    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void updateStatus(String msg) {
        if (msg == null || statusParam == null || msg.length() == 0 ||
            msg.equals("null") || msg.indexOf(statusParam) < 0) {
            return;
        }
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if (parm.equals(statusParam)) {
                String vstring = tok.nextToken();
                if (statusEnable != null) {
                    if (hasToken(statusEnable, vstring)) {
                        setShowValue(new ParamIF("", "", "1"));
                    } else {
                        setShowValue(new ParamIF("", "", "-1"));
                    }
                }
                if(isEnabled()){
                    if (statusValue != null) {
                        setSelected(hasToken(statusValue, vstring));
                    }
                }
                else
                    setSelected(false);
            }
        }
    }

    public void setValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            if (pf.value.equals("0"))
                setSelected(false);
            else
                setSelected(true);
        }
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            isActive = Integer.parseInt(s);
            if (isActive >= 0) {
                setEnabled(true);
                if (setVal != null) {
                    vnmrIf.asyncQueryParam(this, setVal);
                }
            }
            else {
                setSelected(false);
                setEnabled(false);
            }
        }
        repaint();
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

    private void toggleAction(AWTEvent ev, boolean set)
    {
        if (inModalMode || inEditMode || vnmrIf == null)
            return;
        if (isActive < 0)
            return;
        if (set)
        {
            if (vnmrCmd != null)
                vnmrIf.sendVnmrCmd(this, vnmrCmd);
            updateValue();
        }
        else
        {
            if (vnmrCmd2 != null)
                vnmrIf.sendVnmrCmd(this, vnmrCmd2);
            updateValue();
        }
    }

    public void refresh() { }

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

    public Object[][] getAttributes() {
        return attributes;
    }

    private boolean hasToken(String str, String tok) {
        if (str == null) return false;
        if (tok == null) return true;
        StringTokenizer strtok = new StringTokenizer(str);
        while (strtok.hasMoreTokens()) {
            if (tok.equals(strtok.nextToken())) return true;
        }
        return false;
    }

    private final static String[] m_arrStrYesNo = {"yes", "no"};

    private final static Object[][] attributes = {
        {LABEL,        Util.getLabel(LABEL)},
        {VARIABLE,     Util.getLabel(VARIABLE)},
        {ICON,         Util.getLabel(ICON) },
        {SETVAL,       Util.getLabel(SETVAL)},
        {SHOW,         Util.getLabel(SHOW)},
        {CMD,          Util.getLabel(CMD)},
        {CMD2,         Util.getLabel(CMD2)},
        {STATPAR,      Util.getLabel(STATPAR)},
        {STATVAL,      Util.getLabel(STATVAL)},
        {STATSHOW,     Util.getLabel(STATSHOW)},
        {RADIOBUTTON,  Util.getLabel(RADIOBUTTON), "radio", m_arrStrYesNo},
    };

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
    public void setBounds(int x, int y, int w, int h) {
        if(inEditMode) {
            defLoc.x = x;
            defLoc.y = y;
            defDim.width = w;
            defDim.height = h;
        }
        curDim.width = w;
        curDim.height = h;
        curLoc.x = x;
        curLoc.y = y;
        if(icon != null)
            icon.setSize(w - 4, h - 4);
        if(!inEditMode) {
            if(rWidth <= 0) {
                calSize();
            }
        }
        super.setBounds(x, y, w, h);
    }


    public void calSize() {
        if(icon !=null && label==null){
            setHorizontalAlignment(SwingConstants.CENTER);
        }
        else{
            if (fm == null) {
                font = getFont();
                fm = getFontMetrics(font);
            }
            rHeight = font.getSize();
            rWidth = 6;
            if (label != null)
              rWidth += fm.stringWidth(label);
            if (getIcon() != null)
                rWidth += getIcon().getIconWidth();
            rWidth2 = rWidth;
        }
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
                    font2 = DisplayOptions.getFont("Dialog",font.getStyle(),rHeight);
                else
                    font2 = font;
            }
            s += 1;
            //curFont = font2.deriveFont(s);
            curFont = DisplayOptions.getFont(font2.getName(),font2.getStyle(),(int)s);
        }
        else
            curFont = font;
        int fh;
        String strfont = curFont.getName();
        int nstyle = curFont.getStyle();
        while (s > 10) {
            FontMetrics fm2 = getFontMetrics(curFont);
            rWidth2 = 6;
            if (label != null)
                rWidth2 += fm2.stringWidth(label);
            if (getIcon() != null)
                 rWidth2 += getIcon().getIconWidth();
            fh = curFont.getSize();
            if ((rWidth2 < nWidth) && (fh < h))
                break;
            s = s - 0.5f;
            if (s < 10)
                break;
            //curFont = curFont.deriveFont(s);
            curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
        }
        if (rWidth2 > w)
            rWidth2 = w;
        setFont(curFont);
    }

}

