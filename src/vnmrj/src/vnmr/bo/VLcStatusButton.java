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
public class VLcStatusButton extends TwoLineButton 
        implements DropTargetListener, StatusListenerIF, VEditIF,
            VObjDef, VObjIF, VStatusIF, PropertyChangeListener {
    private JToggleButton  button;
    private String  fontName = "Dialog";
    private String  fontStyle = "Bold";
    private String  fontSize = "12";
    private String  statcol;
    private String  title = null;
    protected boolean show = true;
    protected int     status_index = -1;
    protected String  statkey = null;
    protected String  statpar = null;
    protected String  statset = null;
    protected String  statval = null;
    protected String  m_keyPfx = "xxx";
    protected int     state = UNSET;
    protected String  value = " ";
    protected String  tip = null;
    protected String  setval = null;
    protected String  statusValue = null;
    protected boolean showset = false;
    private String  type = null;
    private String  fg = "black";
    private String  isEnabled = "yes";
    private Color   fgColor = Color.black;
    private Color   bgColor = null;
    private String m_sReadyColor = null;
    private Color m_readyColor = null;
    private String m_sRunColor = null;
    private Color m_runColor = null;
    private boolean isEditing = false;
    protected boolean inEditMode = false;
    private boolean isFocused = false;
    protected ButtonIF vnmrIf;
    private MouseAdapter ml;
    protected ChartFrame  chart_frame = null;
    protected VLcStatusChart m_statusChart;
    private String window_title="Chart";
    private final static Font bold = new Font("Dialog", Font.BOLD, 12);
    private final static Font italics = new Font("Dialog", Font.ITALIC, 12);
    private final static Font plain = new Font("Dialog", Font.PLAIN, 12);
    protected String m_strDigits = null;
    protected int m_digits = 0;
    protected String m_chartIcon = null;

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

    /** constructor (for LayoutBuilder) */
    public  VLcStatusButton(SessionShare sshare, ButtonIF vif, String typ) {
        button=this;
        if(Global.STATUS_TO_FG==false)
            statcol=Util.getLabel("mlBg","bg");
        else
            statcol=Util.getLabel("mlFg","fg");
        type = typ;
        vnmrIf = vif;
        setBackground(null);
        setValueString(value);
        setValueFont(plain);
        setTitleFont(bold);
        setTitleColor(fgColor);
        getFonts();
        changeFont();
        setEnabled(true);
        setMargin(new Insets(1, 1, 1, 1));
        setPreferredSize(new Dimension(65, 20));
        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & InputEvent.BUTTON1_MASK) != 0 && clicks >= 2) 
                    ParamEditUtil.setEditObj((VObjIF) evt.getSource());
            }
        };
        m_statusChart = new VLcStatusChart(sshare,vif,"chart");
        addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                buttonAction(evt);
            }
        });
        DisplayOptions.addChangeListener(this);
    }

    // PropertyChangeListener interface
    
    public void propertyChange(PropertyChangeEvent evt){
        getFonts();
        setState(state);
        repaint();
    }
 
    private void getFonts(){
        for(int i=0;i<names.length;i++){
            String name=DisplayOptions.getOption(DisplayOptions.COLOR,names[i]);
            if(name !=null){
                colors[i]=DisplayOptions.getColor(name);
                fonts[i]=DisplayOptions.getFont(names[i]);
            }
        }
    }

    /**
     * Gets the preferred size for the chart for this particular status button.
     * The size is a default based on the type of status.
     */
    private Dimension getPreferredChartSize() {
        Dimension d = new Dimension(160, 150); // Plausible default size
        if (statset != null) {
            if (statset.startsWith("pump")) {
                d.width = 400;
            } else if (statset.startsWith("uv")) {
                d.width = 200;
            }
        }
        return d;
    }

    /**
     * Gets the name of the persistence file for this particular status button.
     */
    private String getPersistenceName() {
        String name = "GenericStatusChart";
        if (statset != null) {
            if (statset.startsWith("pump")) {
                name = "LcPumpStatus";
            } else if (statset.startsWith("uv")) {
                name = "LcUvStatus";
            }
        }
        return name;
    }

    protected void writePersistence() {
        FileUtil.writeGeometryToPersistence(getPersistenceName(), chart_frame);
    }

    /** show/hide chart. */
    protected void buttonAction(ActionEvent  e) {
        if (inEditMode)
            return;
        if (chart_frame == null) {
            chart_frame = new ChartFrame(m_statusChart);
            chart_frame.addComponentListener(new ComponentAdapter() {
                    public void componentResized(ComponentEvent ce) {
                        writePersistence();
                    }
                    public void componentMoved(ComponentEvent ce) {
                        writePersistence();
                    }
                });
            if (!FileUtil.setGeometryFromPersistence(getPersistenceName(),
                                                     chart_frame))
            {
                chart_frame.setSize(getPreferredChartSize());
                Point pt=getLocationOnScreen();
                pt.y -= chart_frame.getHeight();
                chart_frame.setLocation(pt);
            }
            chart_frame.validate();
        }
        if (chart_frame.isVisible()) {
            chart_frame.setVisible(false);
            setBorder(BorderFactory.createRaisedBevelBorder());
        } else {
            setBorder(BorderFactory.createLoweredBevelBorder());
            chart_frame.setVisible(true);
            chart_frame.repaint();
        }
    }
      
    /** Update state from Infostat. */
    public void updateStatus(String msg)
    {
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
                tip = val;
                state = getState(val);
                setState(state);
            } 
            if (key.equals(statpar)) {
                if (val !=null && !val.equals("-")) {
                    value = val;
                    setState(state);
                }
            }
            if (key.equals(statset)) {
                if (val !=null && !val.equals("-")) {
                    setval = val;
                    setState(state);
                }
                setTitleString();
            }
            if (key.equals(statval)) {
                if (val !=null && !val.equals("-")) {
                    statusValue = val;
                } else {
                    statusValue = null;
                }
                setState(state);
            }
            if (key.equals(m_keyPfx + "Title")) {
                if (val !=null && !val.equals("-")) {
                    title = val;
                    setTitleString();
                }
            }
        }
        m_statusChart.updateStatus(msg);
    }
    
    /** Get state option. */
    protected int getState(String s)
    {
        for(int i=0;i<states.length;i++)
                if(s.equals(states[i]))
                        return i+1;
         return UNSET;
    }

    /** Get state display string. */
    protected String getStateString(int s) {
        switch (s) {
        default:
            return "";
        case OFF:
            return Util.getLabel("sOFF","Off");
        case REGULATED:
            return Util.getLabel("sREGULATED","Regulated");
        case NOTREG:
            return Util.getLabel("sNOTREG","Not Regulated");
        case NOTPRESENT:
            return Util.getLabel("sNOTPRESENT","Not present");
        }
    }

    /** set state option. */
    protected void setState(int s) {
        setValueFont(plain);
        setValueColor(Color.black);
        //if (value.equalsIgnoreCase("ready")) {
        //    setBackground(m_readyColor);
        //} else {
        //    setBackground(null);
        //}
        //setBackground(null);
        setShow(show);
        switch (s) {
        case OFF:
            setColor(colors[OFFID]);
            setValueFont(fonts[OFFID]);
            setValueString(value);
            showset=false;
            break;
        case REGULATED:
            setColor(colors[ONID]);
            setValueFont(fonts[ONID]);
            setValueString(value);
            showset=true;
            break;
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
        default:
            setColor(Color.BLACK);
            setValueFont(fonts[ONID]);
            setValueString(value);
            showset=true;
            break;
        }
        setToolTip();
    }

    /** Set status color. */
    protected void setColor(Color color) {
        if (statcol.equals(status_color[0])){
            setValueColor(color);
        }
        String v = value.toLowerCase();
        if (v.startsWith("ready")) {
            setBackground(m_readyColor);
        } else if (v.startsWith("run")
                   || v.startsWith("stop") || v.startsWith("pause"))
        {
            setBackground(m_runColor);
        } else {
            setBackground(null);
        }
        setContentAreaFilled(false);
    }

    public void paintComponent(Graphics g) {
        Dimension  ps = getSize();
        g.setColor(getBackground());
        g.fillRect(0, 0, ps.width, ps.height);
        super.paintComponent(g);
    }    
    
    /** set tooltip text. */
    protected void setToolTip() {
        setToolTipText(tip);
    }

    public Point getToolTipLocation(MouseEvent event) {
        return new Point(0,0);
    }

    protected void setShow(boolean flag){
        setVisible(flag);
    }

    protected void setTitleString() {
        // TODO: Clean up ad hoc stuff in setTitleString()
        String s = title;
        if (statset != null && statset.startsWith("pump") && setval != null) {
            s += ": " + setval;
        } else if (statset != null && statset.startsWith("uv")
                   && setval != null)
        {
            s += ": " + setval;
        }
        setLine1(s);
        m_statusChart.setAttribute(TITLE, title);
    }

    protected void setTitleFont(Font f){
        setLine1Font(f);
    }

    protected void setTitleColor(Color c){
        setLine1Color(c);
    }

    private String appendStatusValue(String status) {
        if (status != null) {
            if (statusValue != null) {
                StringTokenizer toker = new StringTokenizer(statusValue);
                if (toker.hasMoreTokens()) {
                    String val = toker.nextToken(); // Leave off the units
                    val = trimStatusValue(val);
                    status += ": " + val;
                }
            }
        }
        return status;
    }

    private String trimStatusValue(String val) {
        if (val != null && m_strDigits != null) {
            // Clip to desired number of digits
            try {
                double dVal = Double.parseDouble(val);
                val = Fmt.f(m_digits, dVal, false);
            } catch (NumberFormatException nfe) {}
        }
        return val;
    }

    protected void setValueString(String s) {
        if (s != null && statval != null) {
            if (statval.startsWith("pump") && s.startsWith("Run")) {
                s = appendStatusValue(s);
            } else if (statval.startsWith("uv")) {
                s = appendStatusValue(s);
            }
            setLine2(s);
        }
    }

    protected void setValueFont(Font f){
        setLine2Font(f);
    }

    protected void setValueColor(Color c){
        setLine2Color(c);
    }

    // VObjIF interface

    /** set an attribute. */
    public void setAttribute(int attr, String c){
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case TITLE:
            title = c;
            setTitleString();
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
            statkey = c;
            m_statusChart.setAttribute(attr,c);
            updateStatus(ExpPanel.getStatusValue(statkey));
            int pfxlen;
            for (pfxlen = 0; pfxlen < c.length(); pfxlen++) {
                if (!Character.isLowerCase(c.charAt(pfxlen))) {
                    break;
                }
            }
            m_keyPfx = c.substring(0, pfxlen);
            break;
        case STATPAR:
            statpar = c;
            m_statusChart.setAttribute(attr,c);
            updateStatus(ExpPanel.getStatusValue(statpar));
            break;
        case STATSET:
            statset = c;
            m_statusChart.setAttribute(attr,c);
            updateStatus(ExpPanel.getStatusValue(statset));
            break;
        case STATVAL:
            statval = c;
            m_statusChart.setAttribute(attr,c);
            updateStatus(ExpPanel.getStatusValue(statval));
            break;
        case FGCOLOR:
            fg = c;
            fgColor = VnmrRgb.getColorByName(c);
            setTitleColor(fgColor);
            repaint();
            break;
        case COLOR3:
            m_sReadyColor = c;
            m_readyColor = VnmrRgb.getColorByName(c);
            repaint();
            break;
        case COLOR4:
            m_sRunColor = c;
            m_runColor = VnmrRgb.getColorByName(c);
            repaint();
            break;
        case ENABLED:
            isEnabled=c;
            if(c.equals("yes"))
                setEnabled(true);
            else
                setEnabled(false);
            break;
        case PANEL_NAME:
            window_title=c;
            break;
        case NUMDIGIT:
            m_strDigits = c;
            if (m_strDigits != null) {
                try {
                    m_digits = Integer.parseInt(m_strDigits);
                } catch (NumberFormatException nfe) {
                    m_strDigits = null;
                }
            }
            /*Messages.postDebug("m_strDigits=" + m_strDigits
                               + ", m_digits=" + m_digits);/*CMP*/
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
        case STATVAL:
            return statval;
        case STATCOL:
            return statcol;
        case FGCOLOR:
            return fg;
        case COLOR3:
            return m_sReadyColor;
        case COLOR4:
            return m_sRunColor;
        case ENABLED:
            return isEnabled;
        case PANEL_NAME:
            return window_title;
        case NUMDIGIT:
            return m_strDigits;
        case ICON:
            return m_chartIcon;
        default:
            return m_statusChart.getAttribute(attr);
        }
    }

    public void setEditStatus(boolean s) {isEditing = s;}
    public void setEditMode(boolean s) {
        if (s) {
            addMouseListener(ml);
            defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
        }
       else
            removeMouseListener(ml);
        inEditMode = s;
    }
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void setDefLabel(String s) {}
    public void setDefColor(String s) {fg=s;}
    public void updateValue() {}
    public void setValue(ParamIF p) {}
    public void setShowValue(ParamIF p) {}
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

    private final static Object[][] attributes = {
        {new Integer(TITLE),     Util.getLabel("lcTITLE"),},
        {new Integer(STATCOL),   Util.getLabel(STATCOL),"menu",status_color},
        {new Integer(STATKEY),   Util.getLabel(STATKEY),"menu",status_key},
        {new Integer(PANEL_NAME),Util.getLabel("lcPANEL_NAME")},
        {new Integer(VALUES),    Util.getLabel("lcVALUES")},
        {new Integer(SHOWMAX),   Util.getLabel("lcSHOWMAX"),"menu", true_false},
        {new Integer(COLOR1),    Util.getLabel("lcCOLOR1"),"color"},
        {new Integer(COLOR2),    Util.getLabel("lcCOLOR2"),"color"},
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
                    button.setSelected(false);
                    button.setBorder(BorderFactory.createRaisedBevelBorder());
                }
            });
        }

        public void validate() {
            Dimension win = getSize();
            chart.setSize(win);
            super.validate();
            chart.validate();
            setTitle(window_title);
        }

        public String getAttribute(int attr){ 
            return chart.getAttribute(attr);
        }

        public void setAttribute(int attr, String c){ 
            chart.setAttribute(attr,c);
        }

        public void updateStatus(String msg){
            chart.updateStatus(msg);
        }

        public void destroy() { chart.destroy();}
    }   // class ChartFrame

    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}
} // class VStatusButton
