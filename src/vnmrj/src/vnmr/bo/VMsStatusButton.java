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
import java.awt.event.*;
import javax.swing.*;
import java.util.*;
import vnmr.ui.*;
import vnmr.util.*;
import vnmr.lc.*;

/**
 * A two-line button that shows RF Monitor data passed by Infostat.
 */
public class VMsStatusButton extends VStatusButton {

    //private ChartFrame m_statusFrame = new ChartFrame();
    private MSPanel m_msStatusPanel = null;
    //private Map statusTable = new HashMap();

    private static final double DEFAULT_SCALING = 0.65;
    private static final String PERSISTENCE_FILE = "MsStatus";

    // Multitudinous status values
    private boolean m_q1ms = false;
    private boolean m_q3ms = false;
    private double[] m_qOffset = {-30, -7, -8, 3};
    private double m_colPress = 0.001;
    private double m_manPress = 1.23456789e-6;
    private double m_manHtr = 42;
    private double[] m_lens = {0, 0, 0, 0, -28};
    private boolean m_nebulizingGasOn = false;
    private boolean m_nebulizingGasIsAir = false;
    private boolean m_nebulizingGasIsN2 = false;
    private Rectangle[] refRectangles = {
        new Rectangle(558, 117, 125, 84),
        new Rectangle(558, 426, 125, 84),
    };
    private Rectangle[] rectangles = null;


    /**
     * Constructor (for LayoutBuilder)
     */
    public  VMsStatusButton(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
        m_msStatusPanel = new MSPanel();

        // Build up the MsStatus table
        /*
         * To add a new Kodiak parameter to monitor, you only need to
         * add a line to this file.  Each line specifies:
         * 1) The status name - analogous to those seen in Infostat,
         *    but these do not go through Infostat, but are sent
         *    directly to ExpPanel.processStatusData, which treats
         *    them the same as the ones that come from the Console
         *    through Infostat.
         * 2) The PML command to send to get the status.
         * 3) The color of the displayed value.  This can be either
         *    the RBG values in hex (e.g., "0xFF0000" for red) or a
         *    color name as found in the list in the DisplayOptions
         *    editor.
         * 4,5) The x,y position of the displayed value on the status
         *    picture.  This is relative to the top left corner and
         *    picture width of 1000 and hight 600.
         * 6) The format for displaying the status value.  This format
         *    is similar to that used by the DecimalFormat class.  It
         *    used to format the value string sent to
         *    processStatusData.  The main difference is that if the
         *    format string starts with a semicolon (";"), it is
         *    treated as a semicolon separated list of alternate
         *    labels.  The value of the status (truncated to an
         *    integer) is used to index into the list; "1" corresponds
         *    to the first entry. If the value is less than 1 or
         *    greater than the list size, it is formatted as "***".
         *
         * If a status value should not be displayed on the status picture
         *    put only items 1, 2, and 6 in the entry.
         */
        // TODO: read this from a file
        MsStatus.put("MSvalveStatus", "?lc_valve + 1", ";load;inject;");

        MsStatus.put("MSdetOn", "?on + 1",
                     "0xB3FFB3", 250, 450, ";Off;On;");

        MsStatus.put("MSnebGasPress", "?readback(96)",
                     "0xB3B3FF", 50, 150, "0 psi");
        MsStatus.put("MSmode", "?apci + esi * 2",
                     "0xB3B3FF", 10, 200, ";APCI;ESI;");
        MsStatus.put("MSnebGas",
                     "?nebulizing_gas_is_air + nebulizing_gas_is_N2 * 2",
                     "0xB3B3FF", 10, 150, ";Air,;N\u2082,;"); // N-sub-2

        MsStatus.put("MSdetVolts", "?detector",
                     "0xB3FFB3", 170, 450, "0 V");
        MsStatus.put("MSpolarity", "?pos + 1",
                     "0xB3FFB3", 170, 475, ";Negative;Positive;");

        MsStatus.put("MSQ1on", "?q1ms", "-0");
        MsStatus.put("MSQ3on", "?q3ms", "-0");
        MsStatus.put("MSQ0", "?qoffset(0)",
                     "pink", 380, 106, "'Q0:' -0.0 V");
        MsStatus.put("MSQ1", "?qoffset(1)",
                     "pink", 560, 106, "Q1: -0.0 V");
        MsStatus.put("MSQ2", "?qoffset(2)",
                     "pink", 722, 106, "Q2: -0.0 V");
        MsStatus.put("MSQ3", "?qoffset(3)",
                     "pink", 560, 530, "Q3: -0.0 V");

        MsStatus.put("MSL4", "?lens(4)",
                     "0xB3FFB3", 480, 75, "L4: -0 V");

        MsStatus.put("MSmanpress", "?manpress",
                     "0xB3B3FF", 630, 310, "0.#E0 Torr");
        //MsStatus.put("MSmanTemp", "?mantemp",
        MsStatus.put("MSmanTemp", "?manhtr",
                     "0xB3B3FF", 630, 340, "0 \u2103");
        //MsStatus.put("MScollisionPress", "?colpress",
        MsStatus.put("MScollisionPress", "?readback(56)",
                     "0xB3B3FF", 722, 530, "0.000 mTorr");

        //MsStatus.put("MSdryTemp", "?drying_gas_temp",
        MsStatus.put("MSdryTemp", "?readback(89)",
                     "pink", 200, 50, "-0 '\u2103,'"); // degrees-C
        MsStatus.put("MSdryPress", "?readback(95)",
                     "pink", 260, 50, "0 psi");
        //MsStatus.put("MScapVolts", "?capillary",
        MsStatus.put("MScapVolts", "?readback(83)",
                     "pink", 175, 75, "Capillary: -0 V");
        //MsStatus.put("MSshieldVolts", "?shield",
        MsStatus.put("MSshieldVolts", "?readback(84)",
                     "pink", 175, 250, "Shield: -0 V");


        MsStatus.put("MSneedleVolts", "?readback(81)",
                     "white", 100, 100, "0 V");
        MsStatus.put("MSneedleAmps", "?readback(82)",
                     "white", 100, 125, "0.0 \u00b5A"); // micro-A

        MsStatus.put("MSauxGasPress", "?readback(97)",
                     "0xB3B3FF", 10, 275, "Aux gas: 0 psi");
        MsStatus.put("MStorchTemp", "?readback(91)",
                     "0xB3B3FF", 10, 300, "Torch: -0 '\u2103'"); // degrees-C
        //MsStatus.put("MShousingTemp", "?api_housing_htr",
        MsStatus.put("MShousingTemp", "?readback(90)",
                     "0xB3B3FF", 10, 350, "Housing: -0 '\u2103'"); // degrees-C

        MsStatus.put("MSconnected", "?connected + 1",
                     "red", 250, 550, ";DISCONNECTED; ;");

        MsStatus.put("MSneedleVoltsSetting", "?needle", "-0");
        MsStatus.put("MSneedleAmpsSetting", "?corona", "-0");
        MsStatus.put("MSshieldVoltsSetting", "?shield", "-0");

        /*
         * Start periodic update
         */
        if (DebugOutput.isSetFor("MSMonData")) {
            Messages.postDebug("Generating fake MS status data.");
            ActionListener updateTimeout = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                    updateStatusData();
                }
            };
            updateTimer = new javax.swing.Timer(UPDATE_DELAY, updateTimeout);
            updateTimer.start();
        }
    }

    /**************************************
     * SEND FAKE STATUS DATA TO ExpPanel
     **************************************/
    javax.swing.Timer updateTimer = null;
    private final int UPDATE_DELAY = 1000; // ms
    private int m_ValveStatus = 1;

    private void updateStatusData() {
        double dx = Math.random() - 0.5;
        double val = 3 + dx / 10;
        m_ValveStatus = (m_ValveStatus == 1) ? 2 : 1;
        ((ExpPanel)vnmrIf).processStatusData("MSvalveStatus " + m_ValveStatus);
        ((ExpPanel)vnmrIf).processStatusData("MScollisionPress "
                                             + Fmt.f(3, val));
        ((ExpPanel)vnmrIf).processStatusData("MSmode  1"); // APCI = 1
        dx = Math.random() - 0.5;
        val = 3 + dx;
        ((ExpPanel)vnmrIf).processStatusData("MSneedleAmps "
                                             + Fmt.f(3, val));    }
    /******************************************************/

    /** show/hide chart. */
    protected void buttonAction() {
        if (inEditMode)
            return;
        if(chart_frame == null){
            chart_frame = new ChartFrame(m_msStatusPanel);
            chart_frame.pack();
            Dimension defaultSize = chart_frame.getSize();
            defaultSize.width = (int)(DEFAULT_SCALING * defaultSize.width);
            defaultSize.height = (int)(DEFAULT_SCALING * defaultSize.height);
            chart_frame.setSize(defaultSize);
            if (!FileUtil.setGeometryFromPersistence(PERSISTENCE_FILE,
                                                     chart_frame))
            {
                Point pt = getLocationOnScreen();
                pt.y -= chart_frame.getHeight();
                chart_frame.setLocation(pt);
            }
            chart_frame.validate();
        }
        if(chart_frame.isVisible()){
            chartframeShowing(false);
        }
        else{
            setBorder(BorderFactory.createLoweredBevelBorder());
            chart_frame.setVisible(true);
            if (m_strcmd != null)
                vnmrIf.sendVnmrCmd(this, m_strcmd);
            chart_frame.repaint();
        }
    }

    protected void writePersistence() {
        FileUtil.writeGeometryToPersistence(PERSISTENCE_FILE, chart_frame);
    }


    public void updateStatus(String msg) {
        /*Messages.postDebug("VMsStatusButton.updateStatus("
                           + msg + ")");/*CMP*/
        if (msg == null) {
            return;
        }

        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String key = tok.nextToken();
            if (key.startsWith("MS")) {
                MsStatus.setStatusValueInTable(msg);
                // Update status picture, if visible
                m_msStatusPanel.repaint();
                if (key.equals("MSmode")
                    || key.equals("MSneedleVolts")
                    || key.equals("MSneedleAmps") )
                {
                    // Update text on the button
                    setValueString();
                }
            }
        }
    }

    protected void setTitleString(String s){
        setLine1(s);
    }

    /**
     * Set the appearance of the Button for the current state.
     */
    protected void setState(int s) {
        setValueColor(colors[s]);
        setValueFont(fonts[s]);
        setBackground(null);
        showset=true;
    }

    protected void setValueString() {
        int curState = READYID;

        if (state != curState) {
            state = curState;
            setState(state);
        }

        double mode = MsStatus.getValue("MSmode");
        double val;
        if (mode == 1) {
            // APCI mode
            setLine2("APCI:  " + MsStatus.getText("MSneedleAmps"));
        } else if (mode == 2) {
            // ESI mode
            setLine2("ESI:  " + MsStatus.getText("MSneedleVolts"));
        } else {
            // EI or CI
            setLine2("****");       // Bad mode to be in
        }
        //m_msStatusPanel.repaint();
    }

    /**
     * Get state display string.
     */
    protected String getStateString(int s) {
        return Util.getLabel("sON","On");
    }

    //protected void setToolTip() { }

    public Point getToolTipLocation(MouseEvent event) {
        return new Point(0,0);
    }

    public void mouseClicked(MouseEvent me) {}
    public void mouseEntered(MouseEvent me) {}
    public void mouseExited(MouseEvent me) {}
    public void mousePressed(MouseEvent me) {  }
    public void mouseReleased(MouseEvent me) {  }


    class MSPanel extends JPanel
        implements VStatusChartIF/*, MouseListener, MouseMotionListener*/ {
        private Image image = null;
        private Image imgQ1 = null;
        private Image imgQ3 = null;
        private int width;
        private int height;

        public MSPanel() {
            image = Util.getImage("msStatus.gif");
            if (image != null) {
                ImageIcon icon = new ImageIcon(image);
                this.width = icon.getIconWidth();
                this.height = icon.getIconHeight();
            }
            imgQ1 = Util.getImage("msStatusQ1.gif");
            imgQ3 = Util.getImage("msStatusQ3.gif");
            setBackground(Color.BLACK);

            //addMouseMotionListener(this);
            //Messages.postDebug("Mouse Motion Listener added.");/*CMP*/
        }

        /*
         * MouseMotionListener methods
         */
        /*
        public void mouseDragged(MouseEvent me) {
            Messages.postDebug("Drag: " + me.getX() + ", " + me.getY());
        }

        public void mouseMoved(MouseEvent me) {
            Messages.postDebug("Move: " + me.getX() + ", " + me.getY());
        }
        */

        public Dimension getPreferredSize() {
            return new Dimension(width, height);
        }

        public void paintComponent(Graphics g) {
            super.paintComponent(g); //paint background

            Dimension dim = chart_frame.getContentPane().getSize();

            //Draw image
            if (image != null) {
                g.drawImage(image, 0, 0, dim.width, dim.height, this);
            }
            if (MsStatus.getValue("MSQ1on") > 0 && imgQ1 != null) {
                g.drawImage(imgQ1, 0, 0, dim.width, dim.height, this);
            }
            if (MsStatus.getValue("MSQ3on") > 0 &&  imgQ3 != null) {
                g.drawImage(imgQ3, 0, 0, dim.width, dim.height, this);
            }
            /*Messages.postDebug("MSQ1on=" + MsStatus.getValue("MSQ1on")
                               + ", MSQ3on=" + MsStatus.getValue("MSQ3on")
                               + ", imgQ1=" + imgQ1
                               + ", imgQ3=" + imgQ3
                               );/*CMP*/

            // Fill in values at various x,y positions
            int x;
            int y;

            //int ptSize = 16 + (int)(8 * (dim.width - width) / (double)width);
            int ptSize = 20 * dim.width / width;
            Font font = new Font("Default", Font.BOLD, ptSize);
            g.setFont(font);

            Collection<MsStatus.Record> values = MsStatus.values();
            for (MsStatus.Record rec: values) {
                if (rec.text != null) {
                    g.setColor(rec.color);
                    x = (int)((rec.x * dim.width) / width);
                    y = (int)((rec.y * dim.height) / height);
                    g.drawString(rec.text, x, y);
                }
            }

            /*
            g.setColor(new Color(0x6cc268));

            x = (int)((480.0 * dim.width) / width);
            y = (int)((75.0 * dim.height) / height);
            g.drawString("L4  " + Fmt.f(1, m_lens[4]) + " V", x, y);
            
            g.setColor(Color.CYAN);

            x = (int)((630.0 * dim.width) / width);
            y = (int)((310.0 * dim.height) / height);
            g.drawString(Fmt.e(1, m_manPress) + " Torr", x, y);
            
            x = (int)((630.0 * dim.width) / width);
            y = (int)((340.0 * dim.height) / height);
            g.drawString(Fmt.f(0, m_manHtr) + " \u2103", x, y); // 2103 = deg C
            
            x = (int)((722.0 * dim.width) / width);
            y = (int)((530.0 * dim.height) / height);
            g.drawString(Fmt.f(3, m_colPress) + " mTorr", x, y);
            
            g.setColor(new Color(0xfd7f6a));

            x = (int)((380.0 * dim.width) / width);
            y = (int)((106.0 * dim.height) / height);
            g.drawString("Q0  " + m_qOffset[0] + " V", x, y);
            
            x = (int)((560.0 * dim.width) / width);
            y = (int)((106.0 * dim.height) / height);
            g.drawString("Q1  " + Fmt.f(1, m_qOffset[1]) + " V", x, y);
            
            x = (int)((722.0 * dim.width) / width);
            y = (int)((106.0 * dim.height) / height);
            g.drawString("Q2  " + Fmt.f(1, m_qOffset[2]) + " V", x, y);
            
            x = (int)((560.0 * dim.width) / width);
            y = (int)((530.0 * dim.height) / height);
            g.drawString("Q3  " + Fmt.f(3, m_qOffset[3]) + " V", x, y);
            */
        }
        
        //public void validate() {
        //    super.validate();
        //}

        /**
         * Dummy method required by VStatusChartIF
         */
        public void setValue(String s){
            Messages.postError("This dummy method should never be called.");
            Messages.writeStackTrace(new Exception("Stack Trace"));
        }

        /**
         * Dummy method required by VStatusChartIF
         */
        public void updateValue(Vector v){
            Messages.postError("This dummy method should never be called.");
            Messages.writeStackTrace(new Exception("Stack Trace"));
        }

        /**
         * Dummy method required by VStatusChartIF
         */
        public void updateValue(){
            Messages.postError("This dummy method should never be called.");
            Messages.writeStackTrace(new Exception("Stack Trace"));
        }

        public String getAttribute(int attr) { return null;}
        public void setAttribute(int attr, String c) {}
        public void updateStatus(String msg) {}
        public void destroy() {}
        //public void setSize(Dimension win) {}
    }
}
