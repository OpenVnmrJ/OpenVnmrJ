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
import java.awt.dnd.*;
import java.awt.event.*;
import java.beans.*;

import javax.swing.*;

import java.util.*;

import vnmr.ui.*;
import vnmr.util.*;

/**
 * A two-line button that shows Infostat data.
 */
public class VStatusButton extends TwoLineButton
    implements DropTargetListener, StatusListenerIF, VEditIF,
               VObjDef, VObjIF, VStatusIF, ExpListenerIF,
               PropertyChangeListener, ActionListener {
    private JToggleButton  button;
    private String  fontName = "Dialog";
    private String  fontStyle = "Bold";
    private String  fontSize = "12";
    private String  statcol;
    private String  title = null;
    protected boolean show = true;
    protected String showVal = null;
    protected int isActive = 1;
    protected int     status_index = -1;
    protected String  statkey = null;
    protected String  statpar = null;
    protected String  statset = null;
    protected int     state = UNSET;
    protected String  value = " ";
    protected String  setval = null;
    protected boolean showset = false;
    private String  type = null;
    private String  fg = "black";
    private String  isEnabled = "yes";
    private Color   fgColor = Color.black;
    private Color   bgColor = null;
    protected String m_strValue = null;
    protected String m_strVar = null;
    protected String m_strDisplay = null;
    protected String m_strcmd = null;
    protected String m_strcmd2 = null;
    private boolean isEditing = false;
    protected boolean inEditMode = false;
    private boolean isFocused = false;
    protected ButtonIF vnmrIf;
    private MouseAdapter ml;
    protected VStatusChart m_statusChart=null;
    protected ChartFrame  chart_frame = null;
    protected boolean first_time=true;
    private String window_title="Chart";
    private String titleLabel="Chart";
    private final static Font bold
        = DisplayOptions.getFont("Dialog",Font.BOLD,12);
    private final static Font italics
        = DisplayOptions.getFont("Dialog",Font.ITALIC,12);
    private final static Font plain
        = DisplayOptions.getFont("Dialog",Font.PLAIN,12);
    private double xRatio = 1.0;
    private double yRatio = 1.0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);

    protected static int ONID=0;
    protected static int OFFID=1;
    protected static int INTERACTIVEID=2;
    protected static int NPID=3;
    protected static int READYID=4;
    protected static String[] names = {"On","Off","Interactive",
                                       "NotPresent","Ready"};
    protected Color  colors[]={Global.ONCOLOR,
                               Global.OFFCOLOR,
                               Global.INTERACTIVECOLOR,
                               Global.NPCOLOR,
                               Global.READYCOLOR};

    protected Font  fonts[]= {plain, plain, italics, plain, plain};
    protected String precision = "";
    protected int numdigits=1;

    /** constructor (for LayoutBuilder) */
    public  VStatusButton(SessionShare sshare, ButtonIF vif, String typ) {
        button=this;
        if(Global.STATUS_TO_FG==false)
            statcol=Util.getLabel("mlBg","bg");
        else
            statcol=Util.getLabel("mlFg","fg");
        type = typ;
        vnmrIf = vif;
        setBackground(null);
        setContentAreaFilled(false);
        setValueString(value);
        setValueFont(plain);
        setTitleFont(bold);
        //setTitleColor(fgColor);
        getFonts();
        changeFont();
        setEnabled(true);
        setMargin(new Insets(1, 1, 1, 1));
        setPreferredSize(new Dimension(65, 20));
        ExpPanel.addExpListener(this);
        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if (inEditMode) {
                    if ((modifier & InputEvent.BUTTON1_MASK) != 0 && clicks >= 2)
                        ParamEditUtil.setEditObj( (VObjIF) evt.getSource());
                } else {
                    requestFocusInWindow();
                }
                if (button instanceof VMsStatusButton) {
                    buttonAction();
                }
            }
        };
        m_statusChart = new VStatusChart(sshare,vif,"chart");
        addMouseListener(ml);
        addActionListener(this);
        DisplayOptions.addChangeListener(this);
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt) {
    	fgColor=DisplayOptions.getColor(fg);
    	setTitleColor(fgColor);
        getFonts();
        setState(state);
        repaint();
        super.propertyChange(evt);
    }

    private void getFonts() {
        for(int i=0;i<names.length;i++){
            String name=DisplayOptions.getOption(DisplayOptions.COLOR,names[i]);
            if(name !=null){
                colors[i]=DisplayOptions.getColor(name);
                fonts[i]=DisplayOptions.getFont(names[i]);
            }
        }
    }

    public void actionPerformed(ActionEvent ae) {
        buttonAction();
    }

    /** show/hide chart. */
    protected void buttonAction() {
        if (inEditMode)
            return;
        if (chart_frame == null) {
            makeChartFrame();
            initChartFrame();
        }
        if (chart_frame.isVisible()) {
            chartframeShowing(false);
        } else {
            setBorder(BorderFactory.createLoweredBevelBorder());
            chart_frame.setVisible(true);
            if (m_strcmd != null)
                vnmrIf.sendVnmrCmd(this, m_strcmd);
            chart_frame.repaint();
        }
    }

    protected void makeChartFrame() {
        chart_frame = new ChartFrame(m_statusChart);
        chart_frame.setTitle(titleLabel);
    }

    protected void initChartFrame() {
        chart_frame.addComponentListener(new ComponentAdapter() {
                public void componentResized(ComponentEvent ce) {
                    writePersistence();
                }
                public void componentMoved(ComponentEvent ce) {
                    writePersistence();
                }
            });
        String fileName = getPersistenceName();
        if (!FileUtil.setGeometryFromPersistence(fileName, chart_frame)) {
            Point pt=getLocationOnScreen();
            pt.y -= chart_frame.getHeight();
            chart_frame.setLocation(pt);
        }
        chart_frame.validate();
    }

    protected String getPersistenceName() {
        return title;
    }

    public void updateValue(Vector params) {
        String param;
        if (vnmrIf == null)
            return;
        if (showVal != null) {
            vnmrIf.asyncQueryShow(this, showVal);
        } else if (m_strVar != null) {
            StringTokenizer objTokenizer = new StringTokenizer(m_strVar, ", \n");
            while (objTokenizer.hasMoreTokens()) {
                param = objTokenizer.nextToken();
                if (params.contains(param))
                    updateValue();
            }
        }
    }

    public void updateValue() {
        ExpPanel expPanel = Util.getActiveView();
        if (expPanel != null && m_strValue != null)
           expPanel.asyncQueryParam(this,m_strValue);
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            try {
                isActive = Integer.parseInt(s);
                if (isActive > 0) {
                    setVisible(true);
                    updateValue();
                } else {
                    setVisible(false);
                }
            } catch (NumberFormatException nfe) {
            }
        }
    }

    /** Update state from Infostat. */
    public void updateStatus(String msg) {
        if (msg == null)
            return;

        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String key = tok.nextToken();
            String val = "";
            if (tok.hasMoreTokens()) {
                val = tok.nextToken("").trim(); // Get remainder of msg
            }
            if (key.equals(statkey)) {
                state = getState(val);
                setState(state);
            }
            if (key.equals(statpar)) {
                if (val != null && !val.equals("-")) {
                    value = val;
                    if(val.length()>0 && precision.length()>0){
                        tok = new StringTokenizer(val);
                        if (tok.hasMoreTokens()){
                            val=tok.nextToken();
                        }
                        try {
                            String units="";
                            double dval = Double.parseDouble(val);
                            String sval=Fmt.f(numdigits, dval);
                            if (tok.hasMoreTokens())                            
                                units = " "+tok.nextToken();
                            value = sval+units;
                                
                        } catch (Exception e) {
                        }
                    }
                    setState(state);
                }
            }
            if (key.equals(statset)) {
                if (val !=null && !val.equals("-")) {
                    setval = val;
                    setState(state);
                }
            }
        }
        m_statusChart.updateStatus(msg);
    }

    protected void chartframeShowing(boolean bShowing) {
        if (!bShowing) {
            writePersistence();
            chart_frame.setVisible(false);
            if (m_strcmd2 != null)
                vnmrIf.sendVnmrCmd(this, m_strcmd2);
            setBorder(BorderFactory.createRaisedBevelBorder());
            setSelected(false);
        }
    }

    /** Get state option. */
    protected int getState(String s) {
        for(int i=0;i<states.length;i++)
            if(s.equals(states[i]))
                return i+1;
        return UNSET;
    }

    /** Get state display string. */
    protected String getStateString(int s) {
        switch (s) {
        default:
        case OFF:
            return Util.getLabel("sOFF","Off");
        case REGULATED:
            return Util.getLabel("sREGULATED","Regulated");
        case NOTREG:
            return Util.getLabel("sNOTREG","Not Regulated");
        case NOTPRESENT:
            return Util.getLabel("sNOTPRESENT","Not present");
        case ON:
            return Util.getLabel("sON","On");
        case EXT:
            return Util.getLabel("sEXT","EXT");
        }
    }

    /** set state option. */
    protected void setState(int s) {
        setValueFont(plain);
        setValueColor(Color.black);
        setBackground(null);
        setShow(show);
        switch (s) {
        default:
        case OFF:
            setColor(colors[OFFID]);
            setValueFont(fonts[OFFID]);
            setValueString(value);
            showset=false;
            break;
        case EXT:
        case REGULATED:
            setColor(colors[ONID]);
            setValueFont(fonts[ONID]);
            setValueString(value);
            showset=true;
            break;
        case ON:
        case NOTREG:
            setColor(colors[INTERACTIVEID]);
            setValueFont(fonts[INTERACTIVEID]);
            setValueString(value);
            showset=true;
            break;
        case NOTPRESENT:
            setColor(colors[NPID]);
            setValueFont(fonts[NPID]);
            setValueString("    ");
            showset=false;
            break;
        }
        setToolTip();
    }

    protected void writePersistence() {
        String fileName = getPersistenceName();
        FileUtil.writeGeometryToPersistence(fileName, chart_frame);
    }

    /** Set status color. */
    protected void setColor(Color color) {
        if (statcol.equals(status_color[0])){
            setValueColor(color);
            setBackground(null);
        }
        else
            setBackground(color);
    }

    /** set tooltip text. */
    protected void setToolTip() {
        String s=getStateString(state);
        if(showset && setval != null && statkey != null) {
            s += " [" + setval + "]";
        }
        else
            s = title;
        setToolTipText(s);
    }

    public Point getToolTipLocation(MouseEvent event) {
        return new Point(0,0);
    }

    protected void setShow(boolean flag) {
        setVisible(flag);
    }

    protected void setTitleString(String s) {
        setLine1(s);
    }

    protected void setTitleFont(Font f) {
        setLine1Font(f);
    }

    protected void setTitleColor(Color c) {
        setLine1Color(c);
    }

    protected void setValueString(String s) {
        setLine2(s);
    }

    protected void setValueFont(Font f) {
        setLine2Font(f);
    }

    protected void setValueColor(Color c) {
        setLine2Color(c);
    }

    // VObjIF interface

    /** set an attribute. */
    public void setAttribute(int attr, String c) {
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case TITLE:
            title = c;
            setTitleString(c);
            repaint();
            break;
        case STATCOL:
            statcol = c;
            refresh();
            break;
        case FONT_NAME:
            fontName = c;
            changeFont();
            repaint();
            break;
        case FONT_STYLE:
            fontStyle = c;
            changeFont();
            repaint();
            break;
        case FONT_SIZE:
            fontSize = c;
            changeFont();
            repaint();
            break;
        case STATKEY:
            if (c != null)
            {
                for (int i = 0; i < status_key.length; i++) {
                    if (c.equals(status_key[i]) && i != status_index) {
                        statkey = c;
                        setval = null;
                        value = "";
                        statpar = status_val[i];
                        statset = status_set[i];
                        status_index = i;
                        break;
                    }
                }
                if ( /*isEditing && */status_index >= 0) {
                    updateStatus(ExpPanel.getStatusValue(statkey));
                    updateStatus(ExpPanel.getStatusValue(statpar));
                    updateStatus(ExpPanel.getStatusValue(statset));
                    repaint();
                }
            }
            break;
        case FGCOLOR:
            fg = c;
            fgColor = DisplayOptions.getColor(c);
            setTitleColor(fgColor);
            repaint();
            break;
        case VARIABLE:
            m_strVar = c;
            break;
        case CMD:
            m_strcmd = c;
            break;
        case CMD2:
            m_strcmd2 = c;
            break;
        case DISPLAY:
            m_strDisplay = c;
            break;
        case SETVAL:
            m_strValue = c;
            break;
        case ENABLED:
            isEnabled=c;
            if(c.equals("yes") || c.equals("true"))
                setEnabled(true);
            else
                setEnabled(false);
            break;
        case PANEL_NAME:
            window_title=c.trim();
            titleLabel = Util.getLabelString(window_title);
            if (chart_frame != null)
               chart_frame.setTitle(titleLabel);
            break;
        case SHOW:
            showVal = c;
            if (showVal != null) {
                setVisible(false);
            }
            break;
        case NUMDIGIT:
            try {
                numdigits=Integer.valueOf(c);
            } catch (NumberFormatException nfe) { break; }
            precision = c;
            break;       
        }
        m_statusChart.setAttribute(attr,c);
    }

    /** get an attribute. */
    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return type;
        case TITLE:
            return title;
        case FONT_NAME:
            return fontName;
        case FONT_STYLE:
            return fontStyle;
        case FONT_SIZE:
            return fontSize;
        case STATKEY:
            return statkey;
        case STATCOL:
            return statcol;
        case FGCOLOR:
            return fg;
        case VARIABLE:
            return m_strVar;
        case CMD:
            return m_strcmd;
        case CMD2:
            return m_strcmd2;
        case DISPLAY:
            return m_strDisplay;
        case SETVAL:
            return m_strValue;
        case ENABLED:
            return isEnabled;
        case PANEL_NAME:
            return window_title;
        case SHOW:
            return showVal;
        case NUMDIGIT:
            return precision;
        default:
            return m_statusChart.getAttribute(attr);
        }
    }

    public void setValue(ParamIF p) {
        if (p != null && !inEditMode && !isEditing)
        {
            String strValue = p.value;
            if (strValue != null)
            {
                m_statusChart.setValue(strValue);
                setValueColor(Color.getColor(fg));
                if (m_strDisplay != null && !m_strDisplay.equalsIgnoreCase("yes"))
                    strValue = "";
                setValueString(strValue.trim());
                repaint();
            }
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


    public void setEditStatus(boolean s) {isEditing = s;}
    public void setEditMode(boolean s) {
        inEditMode = s;
        if (s) {
            defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
        }
    }
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void setDefLabel(String s) {}
    public void setDefColor(String s) {fg=s;}
    public void changeFocus(boolean s) {isFocused = s;}
    public ButtonIF getVnmrIF() {return vnmrIf;}
    public void setVnmrIF(ButtonIF vif) {vnmrIf=vif;}
    public void refresh() {     setState(state);}

    public void setDefLoc(int x, int y) {
        defLoc.x = x;
        defLoc.y = y;
    }

    public Point getDefLoc() {
        tmpLoc.x = defLoc.x;
        tmpLoc.y = defLoc.y;
        return tmpLoc;
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
        super.setBounds(x, y, w, h);
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
        xRatio =  x;
        yRatio =  y;
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

    public void destroy() {
    	if(chart_frame !=null)
    		chart_frame.destroy();
        DisplayOptions.removeChangeListener(this);
    }
    public void changeFont() {
        Font font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setTitleFont(font);
    }

    // DropTargetListener interface

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop

    // VEditIF interface

    public Object[][] getAttributes()  { return attributes; }

    private final static String[] true_false =
    {Util.getLabel("mlFalse"), Util.getLabel("mlTrue")};
    private final static String[] yes_no = {Util.getLabel("mlYes"), Util.getLabel("mlNo")};

    private final static Object[][] attributes = {
        {new Integer(TITLE),     Util.getLabel("lcTITLE")},
        {new Integer(STATKEY),   Util.getLabel(STATKEY)},
        {new Integer(VARIABLE),  Util.getLabel(VARIABLE)},
        {new Integer(SETVAL),    Util.getLabel(SETVAL)},
        {new Integer(CMD),       Util.getLabel(CMD)},
        {new Integer(CMD2),      Util.getLabel(CMD2)},
        {new Integer(PANEL_NAME),Util.getLabel("lcPANEL_NAME")},
        {new Integer(VALUES),    Util.getLabel("lcVALUES")},
        {new Integer(INCR1),     Util.getLabel("lcUPDATE")},
        {new Integer(NUMDIGIT),  Util.getLabel("eNUMDIGIT")},
        {new Integer(DISPLAY),   Util.getLabel("lcDISPLAY"),"radio", yes_no},
        {new Integer(MIN),       Util.getLabel(MIN)},
        {new Integer(MAX),       Util.getLabel(MAX)},
        {new Integer(SHOWMAX),   Util.getLabel("lcSHOWMAX"), "menu", true_false},
        {new Integer(STATCOL),   Util.getLabel(STATCOL),"menu",status_color},
    };

    class ChartFrame extends JDialog {
        private VStatusChartIF chart;

        public ChartFrame(VStatusChartIF c) {
            super(VNMRFrame.getVNMRFrame()); // Attach dialog to main VJ frame
            if (VNMRFrame.getVNMRFrame() == null) {
                throw(new NullPointerException("Null owner for Dialog window"));
            }
            chart=c;
            getContentPane().add((Component)chart);
            pack();
            addWindowListener(new WindowAdapter() {
                public void  windowClosing(WindowEvent e) {
                    /*writePersistence();
                      button.setSelected(false);
                      button.setBorder(BorderFactory.createRaisedBevelBorder());*/
                    chartframeShowing(false);
                }
            });
        }

        public void validate() {
            Dimension win = getSize();
            chart.setSize(win);
            super.validate();
            chart.validate();
            // setTitle(window_title);
        }

        public String getAttribute(int attr) {
            return chart.getAttribute(attr);
        }

        public void setAttribute(int attr, String c) {
            chart.setAttribute(attr,c);
        }

        public void updateValue(Vector params) {
            chart.updateValue(params);
        }

        public void updateValue() {
            chart.updateValue();
        }

        public void setValue(String value) {
            chart.setValue(value);
        }

        public void updateStatus(String msg) {
            chart.updateStatus(msg);
        }

        public void destroy() { chart.destroy();}
    }   // class ChartFrame

    public void setModalMode(boolean s) {}

    public void sendVnmrCmd() {}

} // class VStatusButton
