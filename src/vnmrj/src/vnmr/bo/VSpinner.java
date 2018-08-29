/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.beans.*;
import java.awt.dnd.*;
import java.awt.event.*;
import javax.swing.event.*;

import vnmr.ui.*;
import vnmr.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class VSpinner extends JSpinner implements VObjIF, VEditIF,
    StatusListenerIF, VObjDef, DropTargetListener, PropertyChangeListener
{

    public String type = null;
    public String color = null;
    public String vnmrVar = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String bg = null;
    public Color  fgColor = null;
    public Color  bgColor = null;
    public Font   font = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    protected double value = 0;
    protected VSpinner m_spinner;
    protected String label;
    private String statusParam = null;
    private String vnmrCmd = null;
    public boolean digital = false;
    private String incr = null;
    private String limitPar = null;
    private Double min = new Double(-0xfffffff);
    private Double max = new Double(0xfffffff);
    private String minStr = null;
    private String maxStr = null;
    private VObjIF minMax = null;
    private VObjIF minObj = null;
    private VObjIF maxObj = null;
    private boolean inModalMode = false;
    private long cmdLife = 10000; // Max time to get echo of a command (ms)
    private VObjCommandFilter filter = new VObjCommandFilter(cmdLife);
    protected SpinnerNumberModel model;
    protected JTextField m_entry;

    private int nHeight = 0;
    private int nWidth = 0;
    private int rHeight = 90;
    private int fontH = 0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private boolean panel_enabled=true;


    public VSpinner(SessionShare sshare, ButtonIF vif, String typ)
    {
        this.type = typ;
        this.vnmrIf = vif;
        this.color = "black";
        this.fontSize = "8";
        this.fontStyle = "Bold";
        m_spinner = this;
        model = new SpinnerNumberModel(value, min.doubleValue(), max.doubleValue(), 1);
        setModel(model);
        m_entry = ((DefaultEditor)getEditor()).getTextField();
        setBackground(Util.getBgColor());

        minMax = new MinMax();
        minObj = new MinMax();
        maxObj = new MinMax();
        ((MinMax)minObj).mxtype = 1;
        ((MinMax) maxObj).mxtype = 2;

        addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0 && inEditMode) {
                    if (clicks >= 2) {
                        ParamEditUtil.setEditObj(m_spinner);
                    }
                }
            }
        });

        addChangeListener(new ChangeListener()
        {
            public void stateChanged(ChangeEvent e)
            {
                dataChange(e);
            }
        });

        DisplayOptions.addChangeListener(this);
        new DropTarget(this, this);
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt) {
        if(color!=null){
            fgColor=DisplayOptions.getColor(color);
            setForeground(fgColor);
        }
        bgColor = Util.getBgColor();
        setBackground(bgColor);
        changeFont();
    }

    public void addMouseListener(MouseListener ml)
    {
        super.addMouseListener(ml);
        if (m_entry != null)
            m_entry.addMouseListener(ml);
    }

    public void removeMouseListener(MouseListener ml)
    {
        super.removeMouseListener(ml);
        if (m_entry != null)
            m_entry.removeMouseListener(ml);
    }

    public void setBackground(Color color)
    {
        super.setBackground(color);
        int nCompCount = getComponentCount();
        for (int i = 0; i < nCompCount; i++)
        {
            Component comp = getComponent(i);
            comp.setBackground(color);
        }
        if (m_entry != null)
            m_entry.setBackground(color);
    }

    public void setForeground(Color color)
    {
        super.setForeground(color);
        if (m_entry != null)
            m_entry.setForeground(color);
    }

    public void setFont(Font font)
    {
        super.setFont(font);
        if (m_entry != null)
            m_entry.setFont(font);
    }

    private void dataChange(EventObject e) {
        double i;

        if (inModalMode || inEditMode) {
            return;
        }
        i = model.getNumber().doubleValue();
        String strValue = String.valueOf(i);
        //entry.setText(strValue);
        if (value != i && vnmrCmd != null) {
            value = i;
            filter.sendVnmrCmd(vnmrIf, this, vnmrCmd);
            //vnmrIf.sendVnmrCmd(this, vnmrCmd);
            filter.setLastVal(strValue);
        }
    }

    public void setDefLabel(String s) {
        this.label = s;
    }

    public void setDefColor(String c) {
        this.color = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        //setOpaque(s);
        if (s) {
           addMouseListener(ml);
           if (font != null) {
                setFont(font);
                fontH = font.getSize();
                rHeight = fontH;
           }
           defDim = getPreferredSize();
           curLoc.x = defLoc.x;
           curLoc.y = defLoc.y;
           curDim.width = defDim.width;
           curDim.height = defDim.height;
           nWidth = 0;
        } else {
           removeMouseListener(ml);
        }
        inEditMode = s;
    }

    private void setSmallLargeStep() {
        double k;
        if (incr == null) {
            k = 1;
            model.setStepSize(new Double(k));
        }
        else
            model.setStepSize(Double.valueOf(incr));
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        fontH = font.getSize();
        rHeight = fontH;
        if (!inEditMode) {
             if ((curDim.height > 0) && (rHeight > curDim.height)) {
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
          case FGCOLOR:
                     return color;
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
          case VAR2:
                     return limitPar;
          case SETVAL:
                     return setVal;
          case CMD:
                     return vnmrCmd;
          case VALUE:
                     return String.valueOf(value);
          case DIGITAL:
                     return String.valueOf(digital);
          case MIN:
             if(minStr == null)
                     return limitPar == null ? String.valueOf(min) : null;
             else
                     return limitPar == null ? minStr : null;
          case MAX:
             if(maxStr == null)
                     return limitPar == null ? String.valueOf(max) : null;
             else
                     return limitPar == null ? maxStr : null;
          case STATPAR:
                     return statusParam;
          case INCR1:
                     return incr;
          default:
                    return null;
        }
    }

    public void setAttribute(int attr, String c) {
        if (c != null) {
            c = c.trim();
            if (c.length() == 0) c=null;
        }
        double k = 0;
        switch (attr) {
          case TYPE:
                     type = c;
                     break;
          case FGCOLOR:
                     color = c;
                         fgColor=DisplayOptions.getColor(c);
                     setForeground(fgColor);
                     repaint();
                     break;
          case BGCOLOR:
                     bg = c;
                     if (c != null) {
                        bgColor = VnmrRgb.getColorByName(c);
                     }
                     else {
                        bgColor = Util.getBgColor();
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
          case CMD:
                     vnmrCmd = c;
                     break;
          case VALUE:
                     try {
                         value = Double.parseDouble(c);
                         model.setValue(Double.valueOf(c));
                     } catch (NumberFormatException ex) { }
                     break;
          case VARIABLE:
                     vnmrVar = c;
                     break;
          case VAR2:
                     limitPar = c;
                     if (minMax != null && limitPar != null) {
                         vnmrIf.asyncQueryMinMax(minMax, limitPar);
                     }
                     break;
          case DIGITAL:
                     digital = Boolean.valueOf(c).booleanValue();
                     //setDigitalReadout(digital); /* FUTURE? */
                     break;
          case MIN:
                     if (limitPar == null) {
                         if (c!= null && c.indexOf("VALUE")>=0) {
                             minStr = c;
                             return;
                         }
                         try {
                             min = Double.valueOf(c);
                         } catch (NumberFormatException ex) { return; }

                         model.setMinimum(min);
                         setSmallLargeStep();
                     }
                     break;
          case MAX:
                     if (limitPar == null) {
                         if (c != null && c.indexOf("VALUE")>=0) {
                             maxStr = c;
                             return;
                         }
                         try {
                             max = Double.valueOf(c);
                         } catch (NumberFormatException ex) { return;}

                         model.setMaximum(max);
                         setSmallLargeStep();
                     }
                     break;
          case STATPAR:
                     statusParam = c;
                     updateStatus(ExpPanel.getStatusValue(statusParam));
                     break;
          case INCR1:
                     if (c != null) {
                         try {
                             k = Double.parseDouble(c);
                         } catch (NumberFormatException ex) { return; }
                         incr = c;
                     }
                     else
                        incr = null;
                     break;
          case ENABLED:
                     panel_enabled=c.equals("false")?false:true;
                     setEnabled(panel_enabled);
                     break;
        }
        validate();
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
    };

    public void updateValue() {
        if (setVal != null) {
            vnmrIf.asyncQueryParam(this, setVal);
        }
        if (minStr != null) {
            vnmrIf.asyncQueryParam(minObj, minStr);
        }
        if (maxStr != null) {
            vnmrIf.asyncQueryParam(maxObj, maxStr);
        }
        if (showVal != null) {
            vnmrIf.asyncQueryShow(this, showVal);
        }
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
                if (filter.isValExpected(vstring)) {
                    return;
                }
                try {
                    value = Double.parseDouble(vstring);
                    model.setValue(Double.valueOf(vstring));
                } catch (NumberFormatException ex) { }
            }
        }
    }

    public void setValue(ParamIF pf) {
        if (pf != null && !filter.isValExpected(pf.value) ) {
            try {
                value = Double.parseDouble(pf.value);
                model.setValue(Double.valueOf(pf.value));
            } catch (NumberFormatException ex) { }
        }
    }

    public void setShowValue(ParamIF pf) {}
    public void refresh() {}
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

    public Object[][] getAttributes()
    {
        return attributes;
    }

    class MinMax extends VObjAdapter {
        // Gets "setValue" messages for parameter limits
        public int  mxtype  = 0;
        public void setValue(ParamIF pf) {
            if (pf != null) {

                StringTokenizer tok = new StringTokenizer(pf.value);
                String mstr;

                if (mxtype == 1) {
                    if(tok.countTokens() > 0) try {
                        mstr = tok.nextToken();
                        min = Double.valueOf(mstr);
                        model.setMinimum(min);
                        model.setMaximum(max);
                        setSmallLargeStep();
                        validate();
                        repaint();
                    } catch (NumberFormatException ex) { }
                    return;
                } else if (mxtype == 2) {
                    if(tok.countTokens() > 0) try {
                        mstr = tok.nextToken();
                        max = Double.valueOf(mstr);
                        model.setMinimum(min);
                        model.setMaximum(max);
                        setSmallLargeStep();
                        validate();
                        repaint();
                    } catch (NumberFormatException ex) { }
                    return;
                } else if (tok.countTokens() < 4) {
                    System.out.println("VSpinner.MinMax.setValue(): "+
                                       "not enough tokens: \""+
                                       pf.value+"\"");
                    return;
                }
                tok.nextToken(); // Toss the "m"
                try {
                    mstr = tok.nextToken();
                    max = Double.valueOf(mstr);
                    mstr = tok.nextToken();
                    min = Double.valueOf(mstr);
                    model.setMinimum(min);
                    model.setMaximum(max);
                    setSmallLargeStep();
                    // Ignore the last token (increment)
                    validate();
                    repaint();
                } catch (NumberFormatException ex) { }
            }
        }
    }

    private final static String[] throttle_menu
            = {"0","50","100","200","Disable Drag"};

    private final static Object[][] attributes = {
        {new Integer(VARIABLE), "Vnmr variables:    "},
        {new Integer(SETVAL),   "Value of item:"},
        {new Integer(SHOW),     "Enable condition:"},
        {new Integer(CMD),      "Vnmr command:"},
        {new Integer(STATPAR),  "Status parameter:"},
        {new Integer(VAR2),     "Limits parameter:"},
        {new Integer(MIN),      "Min displayed value:"},
        {new Integer(MAX),      "Max displayed value:"},
        {new Integer(INCR1),    "Mouse click adjustment value:"},
    };

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() { } // Not used

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
        if (defDim.width <= 0)
            defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double)defDim.width * xRatio);
        curDim.height = (int) ((double)defDim.height * yRatio);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
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
           if ((w != nWidth) || (h != nHeight) || (h < rHeight)) {
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

    public void adjustFont(int w, int h) {
        if (h <= 0)
           return;
        if (font == null) {
            font = getFont();
            //fontH = entry.getSize();
            rHeight = fontH;
        }
        nHeight = h;
        nWidth = w;
        h -= 8;
        if (fontH > h)
            rHeight = h - 1;
        else
            rHeight = fontH;
        if ((rHeight < 9) && (fontH >= 9))
            rHeight = 9;
        int w2 = (w * 2) / 5;
        String strfont = font.getName();
        int nstyle = font.getStyle();
        //Font  curFont = font.deriveFont((float) rHeight);
        Font curFont = DisplayOptions.getFont(strfont, nstyle, rHeight);
        while (rHeight >= 9) {
            FontMetrics fm2 = getFontMetrics(curFont);
            int cw = fm2.stringWidth("x");
            if (cw < w2)
                break;
            if (rHeight <= 9)
                break;
            rHeight--;
            //curFont = font.deriveFont((float) rHeight);
            curFont = DisplayOptions.getFont(strfont, nstyle, rHeight);
        }
        if (rHeight > nHeight)
            rHeight = nHeight;
        setFont(curFont);
    }


}
