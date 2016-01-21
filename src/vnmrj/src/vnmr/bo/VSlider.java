/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.*;
import java.text.DecimalFormat;
import java.util.*;
import java.beans.*;
import java.awt.dnd.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.plaf.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VSlider extends ComboSlider implements VObjIF, VEditIF,
        VLimitListenerIF, StatusListenerIF, VObjDef, DropTargetListener,
        PropertyChangeListener {
    public String type = null;
    public String color = null;
    public String vnmrVar = null;
    public String showVal = null;
    public String setVal = null;
    public String scaleVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String tipStr = null;
    public String objName = null;
    public Color fgColor = null;
    public Font font = null;
    private String m_orient = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private VObjIF me;

    private String m_strDragThrottle = null;
    private int m_dragThrottle = -1;
    private String statusParam = null;
    private String vnmrCmd = null;
    private String label = null;
    private double value = 0;
    public boolean digital = false;
    private String incr1Str = null;
    private String incr2Str = null;
    private String digits = null;
    private String limitPar = null;
    private int slider_min = 0x8fffffff;
    private int slider_max = 0xffffffff;
    private int slider_incr1 = 1;
    private int slider_incr2 = 2;
    private VObjIF minMaxObj = null;
    private VObjIF minObj = null;
    private VObjIF maxObj = null;
    private VObjIF incr1Obj = null;
    private VObjIF incr2Obj = null;
    private VObjIF limitsObj = null;
    private String minStr = null;
    private String maxStr = null;
    private boolean inModalMode = false;
    private VSliderUI vui = null;
    private long cmdLife = 10000; // Max time to get echo of a command (ms)
    private VObjCommandFilter filter = new VObjCommandFilter(cmdLife);

    private int nHeight = 0;
    private int nWidth = 0;
    private int rHeight = 90;
    private int fontH = 0;
    private Dimension defDim = new Dimension(0, 0);
    private Dimension curDim = new Dimension(0, 0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private boolean panel_enabled = true;
    protected DecimalFormat valformat = new DecimalFormat("#");
    protected double vmin = 0;
    protected double vmax = 100;
    protected double vincr1 = 1;
    protected double vincr2 = 2;
    private boolean m_dragging = false;
    
    static private final int MIN_OBJ = 1;
    static private final int MAX_OBJ = 2;
    static private final int INCR1_OBJ = 3;
    static private final int INCR2_OBJ = 4;
    static private final int LIMITS_OBJ = 5;
    static private final int MINMAX_OBJ = 0;

    /**
     * Gets set true when value is initialized.
     * Used to prevent bogus values getting sent to VnmrBG.
     */
    private boolean m_valueValid = false;
    private boolean m_sliderValid = false;

    public VSlider(SessionShare sshare, ButtonIF vif, String typ) {
        me = this;
        this.type = typ;
        this.vnmrIf = vif;
        this.color = "black";
        this.fontSize = "8";
        this.fontStyle = "Bold";
        this.value = 0;
        minMaxObj = new MinMax();
        minObj = new MinMax();
        maxObj = new MinMax();
        incr1Obj = new MinMax();
        incr2Obj = new MinMax();
        limitsObj = new MinMax();
        ((MinMax) minObj).mxtype = MIN_OBJ;
        ((MinMax) maxObj).mxtype = MAX_OBJ;
        ((MinMax) incr1Obj).mxtype = INCR1_OBJ;
        ((MinMax) incr2Obj).mxtype = INCR2_OBJ;
        ((MinMax) minMaxObj).mxtype = MINMAX_OBJ;
        ((MinMax) limitsObj).mxtype = LIMITS_OBJ;
        setOpaque(true);
        setBackground(null);

        DisplayOptions.addChangeListener(this);

        /********
        ComponentUI jui = null;
        String uis = slider.getUI().toString();
        if (uis.indexOf("Metal") >= 0) {
            jui = VSliderMetalUI.createUI((JComponent) slider);
        } 
        if (jui != null) {
            slider.setUI((SliderUI) jui);
            vui = (VSliderUI) jui;
        }
        ********/

        addChangeListener(new ChangeListener() {
            public void stateChanged(ChangeEvent evt) {
                if(m_dragging)
                    dataChange(false);
                else
                    dataChange(true);
           }
        });

        /*
         * NB: Starting with JDK 1.2.2, the Change Events for JSliders
         * got broken.  When the mouse is dragged and released, no
         * Change Event is fired after JSlider.getValueIsAdjusting()
         * becomes false.  If it ever gets fixed, the following Mouse
         * Listener can be deleted.  See bug #4246117.  Contrary to
         * what "iah" says, this does happen under Solaris, and it
         * makes no difference if you add the change listener to the
         * model instead of to the slider.  Sun doesn't seem to think
         * this is really a bug; maybe it works with Java L&F?
         * (Still broken in JDK v 1.3 Beta.  Bummer.)
         */
        slider.addMouseListener(new MouseListener() {
            public void mouseClicked(MouseEvent evt) {
            }

            public void mouseEntered(MouseEvent evt) {
            }

            public void mouseExited(MouseEvent evt) {
            }

            public void mousePressed(MouseEvent evt) {
                m_valueValid = true; // Needed for first slider drag
                m_dragging=true;
                Messages.postDebug("VSlider", "VSlider.mousePressed()");
            }

            public void mouseReleased(MouseEvent evt) {
                m_valueValid = true; // Needed for first slider drag
                Messages.postDebug("VSlider", "VSlider.mouseReleased()");
                dataChange(true);
                m_dragging=false;
            }
        });

        addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent ev) {
                entryAction();
            }
        });
/*
        entry.addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent e) {
                entryAction();
            }
        });
*/
        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        ParamEditUtil.setEditObj(me);
                    }
                }
            }
        };
        ExpPanel.addLimitListener(this);
        new DropTarget(this, this);
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt) {
        if (color != null) {
            fgColor = DisplayOptions.getColor(color);
            setForeground(fgColor);
        }
        changeFont();
        if (DisplayOptions.isUpdateUIEvent(evt))
            setSliderSteps();
    }

    private boolean dragOK() {
        return m_dragThrottle >= 0;
    }

    /**
     * @param notDragging Set true only if we know this call was not triggered
     * by a mouse drag. If false, it still <i>may</i> not be from a mouse drag.
     */
    private void dataChange(boolean notDragging) {
        String strValue;

        if (inModalMode || inEditMode)
            return;
        if (!m_valueValid && !m_dragging)
            return;
        if (vnmrCmd != null && (dragOK() || notDragging)) {
            setValueFromSlider();
            setEntryFromValue();
            strValue = String.valueOf(value);
            filter.sendVnmrCmd(vnmrIf, this, vnmrCmd);
            filter.setLastVal(strValue);
            // Messages.postDebug("VSlider", "VSlider.dataChange("+notDragging+") command key: "
            //         + strValue);
        }
    }

    private void entryAction() {
        if (inModalMode || inEditMode) {
            return;
        }
        String svalue = getEntryValue();
        double dv = value;
        try {
            dv = Double.parseDouble(svalue);
        } catch (NumberFormatException ex) {
            setEntryFromValue();
            return;
        }
        if (dv == value)
            return;
        value = dv;
        m_valueValid = true;
        setSliderFromValue();
        dataChange(true);
    }

    private void setSliderSteps() {
        vui = null;
        SliderUI ui = slider.getUI();
        if (ui == null || !(ui instanceof VSliderUI))
            return;
        vui = (VSliderUI) ui;
        vui.setSmallStep(slider_incr1);
        vui.setLargeStep(slider_incr2);
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
            m_sliderValid=false;
            removeMouseListener(ml);
        }
        if (vui != null)
            vui.setEditMode(s);
        inEditMode = s;
    }


    public void changeFont() {
        font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
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
        case SETVAL2:
            return scaleVal;
        case CMD:
            return vnmrCmd;
        case VALUE:
            return String.valueOf(value);
        case DIGITAL:
            return String.valueOf(digital);
        case MIN:
            if (minStr == null)
                return limitPar == null ? String.valueOf(vmin) : null;
            else
                return limitPar == null ? minStr : null;
        case MAX:
            if (maxStr == null)
                return limitPar == null ? String.valueOf(vmax) : null;
            else
                return limitPar == null ? maxStr : null;
        case STATPAR:
            return statusParam;
        case INCR1:
            return incr1Str;
        case INCR2:
            return incr2Str;
        case NUMDIGIT:
            return digits;
        case DRAG:
            return m_strDragThrottle;
        case TOOL_TIP:
            return null;
        case TOOLTIP:
            return tipStr;
        case ORIENTATION:
            return m_orient;
        case PANEL_NAME:
            return objName;
        default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        if (c != null) {
            c = c.trim();
            if (c.length() == 0)
                c = null;
        }
        int k = 0;
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case FGCOLOR:
            color = c;
            fgColor = DisplayOptions.getColor(c);
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
        case CMD:
            vnmrCmd = c;
            break;
        case VALUE:
            try {
                value = Double.parseDouble(c);
            } catch (NumberFormatException ex) {
            }
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        case VAR2:
            limitPar = c;
            if (minMaxObj != null && limitPar != null) {
                vnmrIf.asyncQueryMinMax(minMaxObj, limitPar);
            }
            break;
        case DIGITAL:
            digital = Boolean.valueOf(c).booleanValue();
            //setDigitalReadout(digital); /* FUTURE? */
            break;
        case MIN:
            minStr = c;
            if (c != null && (c.indexOf("VALUE") < 0)) {
                try {
                    vmin = Double.parseDouble(c);
                } catch (NumberFormatException ex) {
                    return;
                }
                setSliderLimits();
            }            
            break;
        case MAX:
            maxStr = c;            
            if (c != null && (c.indexOf("VALUE") < 0)) {
                try {
                    vmax = Double.parseDouble(c);
                } catch (NumberFormatException ex) {
                    return;
                }
                setSliderLimits();
            }            
            break;
        case INCR1:
            incr1Str = c;
            if (c != null && (c.indexOf("VALUE") < 0)) {
                try {
                    vincr1 = Double.parseDouble(c);
                } catch (NumberFormatException ex) {
                    vincr1 = 1.0;
                }
                slider_incr1 = (int) vincr1;
                setSliderSteps();
                return;
            }
            break;
        case INCR2:
            incr2Str = c;
            if (c != null && (c.indexOf("VALUE") < 0)) {
                try {
                    vincr2 = Double.parseDouble(c);
                } catch (NumberFormatException ex) {
                    vincr2 = 2.0;
                }
                slider_incr2 = (int) vincr2;
                setSliderSteps();
                return;
            }
            break;
        case STATPAR:
            statusParam = c;
            if (statusParam != null)
               updateStatus(ExpPanel.getStatusValue(statusParam));
            break;
        case NUMDIGIT:
            if (c != null) {
                try {
                    k = Integer.parseInt(c);
                } catch (NumberFormatException ex) {
                    return;
                }
                digits = c;
                if (k > 0) {
                    entry.setVisible(true);
                    entry.setColumns(k);
                } else
                    entry.setVisible(false);
                entryLen = k;
            } else {
                digits = null;
                entryLen = -1;
            }
            break;
        case DRAG:
            m_strDragThrottle = c;
            try {
                m_dragThrottle = Integer.parseInt(m_strDragThrottle);
            } catch (NumberFormatException nfe) {
                m_dragThrottle = -1;
            }
            filter.setThrottle(m_dragThrottle);
            break;
        case ENABLED:
            panel_enabled = c.equals("false") ? false : true;
            setEnabled(panel_enabled);
            break;
        case TOOL_TIP:
        case TOOLTIP:
            tipStr = c;
            if (c != null && c.length() > 0)
               setToolTipText(Util.getLabel(c));
            else
               setToolTipText(null);
            break;
        case ORIENTATION:
            m_orient = c;
            int orient = JSlider.HORIZONTAL;
            if (m_orient != null && m_orient.toLowerCase().startsWith("v")) {
                orient = JSlider.VERTICAL;
            }
            slider.setOrientation(orient);
            break;
        case PANEL_NAME:
            objName = c;
            break;
        }
        validate();
        repaint();
    }

    public void paint(Graphics g) {
        super.paint(g);
        if (!isEditing)
            return;
        Dimension psize = getSize();
        if (isFocused)
            g.setColor(Color.yellow);
        else
            g.setColor(Color.green);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height - 1, psize.width - 1, psize.height - 1);
        g.drawLine(psize.width - 1, 0, psize.width - 1, psize.height - 1);
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    };

    /* (non-Javadoc)
     * @see vnmr.bo.VObjIF#updateValue()
     */
    public void updateValue() {
        // NB: value update requests resulting from pnew messages 
        // self-produced while dragging the slider are now ignored. The entry
        // box is updated directly when dragging using the current values for
        // min,max, sliderValue etc. This prevents the slider thumb from 
        // oscillating between old and new values when vnmrbg is busy.
        //
        if(m_dragging) 
            return;
        if(limitPar==null){
            if (minStr != null && minStr.indexOf("VALUE") >= 0){
                vnmrIf.asyncQueryParam(minObj, minStr);
                m_sliderValid=false;               
            }   
            if (maxStr != null && maxStr.indexOf("VALUE") >= 0){
                vnmrIf.asyncQueryParam(maxObj, maxStr);
                m_sliderValid=false;
            }   
        }
        if (incr1Str != null && incr1Str.indexOf("VALUE") >= 0){
            vnmrIf.asyncQueryParam(incr1Obj, incr1Str);
            m_sliderValid=false;
        }   
        if (incr2Str != null && incr2Str.indexOf("VALUE") >= 0){
            vnmrIf.asyncQueryParam(incr2Obj, incr2Str);
            m_sliderValid=false;
        }   
        if(!m_sliderValid){
            vnmrIf.asyncQueryParam(limitsObj, "$VALUE=0");               
        }
        if (setVal != null) {
            vnmrIf.asyncQueryParam(this, setVal);
        }
        if (showVal != null) {
            vnmrIf.asyncQueryShow(this, showVal);
        }
    }


    public void updateStatus(String msg) {
        if (msg == null || statusParam == null || msg.length() == 0
                || msg.equals("null") || msg.indexOf(statusParam) < 0) {
            return;
        }
        if (m_dragging)
            return;
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
                    m_valueValid = false;
                    setEntryFromValue();
                    setSliderFromValue();
                } catch (NumberFormatException ex) {
                }
            }
        }
    }

    public void updateLimit(String msg) {
        if (msg == null || limitPar == null || msg.equals("null")
                || msg.indexOf(limitPar) < 0)
            return;

        StringTokenizer strTok = new StringTokenizer(msg);
        if (strTok.hasMoreTokens()) {
            String parm = strTok.nextToken();
            if (parm.equals(limitPar))
                minMaxObj.setValue(new ParamIF(parm, "", msg.substring(parm
                        .length())));
        }
    }

    private void setSliderLimits() {
        if(incr1Str==null && vmax!=vmin)
            vincr1=Math.abs((vmax-vmin)/100);
        if(incr2Str==null)
            vincr2=5*vincr1;
        
        String fstring="#" ;
        
        if(vincr1<1){
            int i;
            fstring="";
            int fraction_digits=(int)(-Math.log10(vincr1));
            int value_digits=(int)(Math.log10(Math.floor(Math.abs(vmax-vmin))));
            value_digits=(value_digits==0)?1:value_digits;
            fstring+="0";
            fstring+=".";
            for(i=0;i<fraction_digits;i++)
                fstring+="0";
        }

        valformat = new DecimalFormat(fstring);

        slider_min=0;
        slider_max=(int)Math.abs((vmax-vmin)/vincr1);
        slider_incr1=1;
        
        slider_incr2=vincr1!=0?(int)(vincr2/vincr1):1;
        slider_incr2=slider_incr2<1?1:slider_incr2;
        setMinimum(slider_min);
        setMaximum(slider_max);
        m_sliderValid=true;
        Messages.postDebug("VSlider", "VSlider.setSliderLimits() "+
                "max: "+slider_max+" format:"+fstring);

        // Be set or reset the slider position based on these limits
        setSliderFromValue();

        setSliderSteps();

        /*****
        if (vui == null)
            return;
        vui.setLargeStep(slider_incr2);
        vui.setLargeStep(slider_incr1);
        *****/
    }

    private void setEntryFromValue() {
        entry.setText(valformat.format(value));
    }
    private void setSliderFromValue() {
        double b=0,m=1;
        if(vmax!=vmin){
            m=(slider_max-slider_min)/(vmax-vmin);
            b=slider_min-m*vmin;
        }
        double val=m*value+b;
        slider.setValue((int)(val+0.5));
        Messages.postDebug("VSlider", "VSlider.setSliderFromValue() "+
                "value:"+valformat.format(value)+" slider:"+(int)val);
    }
    private void setValueFromSlider() {
        double v=getSliderValue();
        double b=0,m=1;
        if(vmax!=vmin){
            m=(slider_max-slider_min)/(vmax-vmin);
            b=slider_min-m*vmin;
        }
        value=(v-b)/m;
        Messages.postDebug("VSlider", "VSlider.setValueFromSlider() " +
        		"value:"+valformat.format(value)+" slider:"+(int)v);
    }

    public void setValue(ParamIF pf) {
        if (pf != null && !filter.isValExpected(pf.value) && !m_dragging) {
            try {
                value = Double.parseDouble(pf.value);
                Messages.postDebug("VSlider", "VSlider.setValue: received from VnmrBG: "
                        + valformat.format(value));
                
                m_valueValid = false;
                setEntryFromValue();
                if(m_sliderValid)
                    setSliderFromValue();
            } catch (NumberFormatException ex) {
            }
        }
    }

    public void setShowValue(ParamIF pf) {
    }

    public void refresh() {
    }

    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void addDefChoice(String s) {
    }

    public void addDefValue(String s) {
    }

    public void dragEnter(DropTargetDragEvent e) {
    }

    public void dragExit(DropTargetEvent e) {
    }

    public void dragOver(DropTargetDragEvent e) {
    }

    public void dropActionChanged(DropTargetDragEvent e) {
    }

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this, inEditMode);
    }

    public Object[][] getAttributes() {
        return attributes;
    }

    class MinMax extends VObjAdapter {
        // Gets "setValue" messages for parameter limits
        public int mxtype = 0;

        public void setValue(ParamIF pf) {
            if (pf == null || slider.getValueIsAdjusting())
                return;
            StringTokenizer tok = new StringTokenizer(pf.value);
            String mstr;
            switch (mxtype) {
            case MIN_OBJ:
                if (tok.countTokens() > 0) {
                    try {
                        Messages.postDebug("VSlider",
                                "VSlider Min: received from VnmrBG: " + pf.value);
                        mstr = tok.nextToken();
                        vmin = Double.parseDouble(mstr);
                    } catch (NumberFormatException ex) { }
                    setSliderLimits();
                }
                break;
            case MAX_OBJ:
                if (tok.countTokens() > 0) {
                    try {
                        Messages.postDebug("VSlider",
                                "VSlider Max: received from VnmrBG: " + pf.value);
                        mstr = tok.nextToken();
                        vmax = Double.parseDouble(mstr);
                    } catch (NumberFormatException ex) { }
                    setSliderLimits();
                }
                break;
            case INCR1_OBJ:
                if (tok.countTokens() > 0)
                    try {
                        Messages.postDebug("VSlider",
                                "VSlider Incr1: received from VnmrBG: " + pf.value);
                        mstr = tok.nextToken();
                        vincr1 = Double.parseDouble(mstr);
                    } catch (NumberFormatException ex) {
                    }
                break;
            case INCR2_OBJ:
                if (tok.countTokens() > 0)
                    try {
                        Messages.postDebug("VSlider",
                                "VSlider Incr2: received from VnmrBG: " + pf.value);
                        mstr = tok.nextToken();
                        vincr2 = Double.parseDouble(mstr);
                    } catch (NumberFormatException ex) {
                    }
                break;
            case MINMAX_OBJ:
                try {
                    Messages.postDebug("VSlider",
                            "VSlider MinMax: received from VnmrBG: " + pf.value);
                    
                    if (tok.countTokens() < 4) {
                        System.out
                                .println("VSlider.MinMax.setValue(): "
                                        + "not enough tokens: \""
                                        + pf.value + "\"");
                        return;
                    }
                    tok.nextToken(); // Toss the "m"
                    mstr = tok.nextToken();
                    vmax = Double.parseDouble(mstr);
                    mstr = tok.nextToken();
                    vmin = Double.parseDouble(mstr);                    
                    setSliderLimits();
                    validate();
                    repaint();
                } catch (NumberFormatException ex) {
                }
                break;
            case LIMITS_OBJ:
                Messages.postDebug("VSlider",
                        "VSlider SetLimits: received from VnmrBG: ");
                setSliderLimits();
                validate();
                repaint();
                break;
            }
        }
    }

    private final static String[] throttle_menu = { "0", "50", "100", "200",
            "Disable Drag" };

    private final static Object[][] attributes = {
            { new Integer(VARIABLE), Util.getLabel(VARIABLE)},
            { new Integer(SETVAL), Util.getLabel(SETVAL) },
            { new Integer(SHOW), Util.getLabel(SHOW) },
            { new Integer(CMD), Util.getLabel(CMD) },
            { new Integer(STATPAR), Util.getLabel(STATPAR) },
            { new Integer(VAR2), "Limits parameter:" },
            { new Integer(MIN), Util.getLabel(MIN) },
            { new Integer(MAX),  Util.getLabel(MAX) },
            { new Integer(INCR2), "Coarse adjustment value:" },
            { new Integer(INCR1), "Fine adjustment value:" },
            { new Integer(NUMDIGIT), "Number of digits displayed:" },
            { new Integer(TOOLTIP), Util.getLabel(TOOLTIP) },
            { new Integer(DRAG), "Ms between updates while dragging:", "menu",
                    throttle_menu }, };

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
    } // Not used

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
        double xRatio = x;
        double yRatio = y;
        if (x > 1.0)
            xRatio = x - 1.0;
        if (y > 1.0)
            yRatio = y - 1.0;
        if (defDim.width <= 0)
            defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double) defDim.width * xRatio);
        curDim.height = (int) ((double) defDim.height * yRatio);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
    }

    public void setBounds(int x, int y, int w, int h) {
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
                //adjustFont(w, h);
            }
        }
        super.setBounds(x, y, w, h);
    }

    public Point getLocation() {
        if (inEditMode) {
            tmpLoc.x = defLoc.x;
            tmpLoc.y = defLoc.y;
        } else {
            tmpLoc.x = curLoc.x;
            tmpLoc.y = curLoc.y;
        }
        return tmpLoc;
    }

    public void adjustFont(int w, int h) {
        if (h <= 0)
            return;
        if (font == null) {
            font = entry.getFont();
            fontH = font.getSize();
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
        int cols = entry.getColumns();
        String strfont = font.getName();
        int nstyle = font.getStyle();
        //Font  curFont = font.deriveFont((float) rHeight);
        Font curFont = DisplayOptions.getFont(strfont, nstyle, rHeight);
        while (rHeight >= 9) {
            FontMetrics fm2 = getFontMetrics(curFont);
            int cw = fm2.stringWidth("x") * cols;
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
