/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**
 * experiment status meter
 */

package  vnmr.bo;

import java.awt.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.io.*;
import javax.swing.*;
import java.util.*;
import java.beans.*;
import vnmr.util.*;
import vnmr.ui.*;
import javax.swing.border.*;

import vnmr.admin.ui.WFontColors;
import vnmr.bo.VExperimentMeter.AcqWindow.AcqItem;

public class VExperimentMeter extends JPanel
    implements DropTargetListener, VObjDef, VObjIF, StatusListenerIF,
    PropertyChangeListener,ActionListener
{
    private int time_left=0;
    private int time_start=0;
    private String timeleftString="0:0:0";
    private boolean acquiring=false;
    private AcqButton button=null;

    private AcqMeter meter=null;
    private AcqWindow m_window=null;
    private boolean rightButtons=false;
    private Dimension psize;

    private String type = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private ButtonIF vnmrIf;
    private MouseAdapter ml;

    private String stateStr=Util.getLabel("_VExperimentMeter_Initializing");
    private static int STATE_OFF=0;
    private static int STATE_ON=1;
    private static int STATE_INTERACTIVE=2;
    private static int STATE_READY=3;
    private int state=STATE_OFF;
    private static String[] disp_names = {"Off","On","Interactive","Ready"};

    private String displayVal = null;
    private String ct="1";
    private String fid="1";
    private String user="-";
    private double bar_value=1.0;
    private boolean first_show=true;
    private Dimension init_dim=new Dimension(300, 100);
    private SessionShare sshare;
    private SMenu   menu;
    private boolean show_hostgain=false;
    private boolean show_hostshim=false;
    private boolean show_preacq=false;
    private boolean show_findz0=false;
    private boolean show_rtime=true;
    private boolean show_fids=true;
    private boolean show_cts=true;
    private String acqstat=Util.getLabel("_VExperimentMeter_Acquisition_status");
    private String show_rtime_str;
    private String show_fids_str;
    private String show_cts_str;

    public static String persistence_file = "ExperimentMeter";
    private static String WINHT = "windowHeight";
    private static String WINWIDTH = "windowWidth";
    private static String WINLOCX = "windowLocX";
    private static String WINLOCY = "windowLocY";
    private int m_nWinHt = 100;
    private int m_nWinWidth = 300;
    private int m_nWinLocX = 0;
    private int m_nWinLocY = 0;
    protected Hashtable<String, AcqItem> m_acqItemTable
        = new Hashtable<String, AcqItem>();

    /** constructor (for LayoutBuilder) */
    public VExperimentMeter(SessionShare ss, ButtonIF vif, String typ) {
        this.type = typ;
        this.vnmrIf = vif;
        this.sshare = ss;

        show_rtime_str=Util.getLabel("_VExperimentMeter_Show_Remaining_Experiment_Time","Show Remaining Experiment Time");
        show_fids_str=Util.getLabel("_VExperimentMeter_Show_FID_Count","Show FID Count");
        show_cts_str=Util.getLabel("_VExperimentMeter_Show_CT_Count","Show CT Count");
        setPreferredSize(new Dimension(200, 20));
        ml = new MouseAdapter() {
        public void mouseClicked(MouseEvent evt) {
            int clicks = evt.getClickCount();
            int modifier = evt.getModifiers();
            if((modifier & InputEvent.BUTTON3_MASK) !=0)
            menuAction(evt);
            else if(inEditMode && ((modifier & (1 << 4)) != 0 && clicks >= 2))
            ParamEditUtil.setEditObj((VObjIF) evt.getSource());
            }
        };
        addMouseListener(ml);
        setLayout(new WidgitLayout());

        meter=new AcqMeter();
        meter.setString(Util.getLabel("_VExperimentMeter_Initializing"));
        meter.setBackground(null);
        add(meter);

        button=new AcqButton();

        button.addActionListener(this);
        button.setToolTipText(acqstat);
        add(button);
        DisplayOptions.addChangeListener(this);

        menu=new SMenu();
    }

    /** show popup menu. */
    private void menuAction(MouseEvent e) {
        menu.show(this, 50, 20);
    }

    public void setPreferredSize(Dimension d) {
        super.setPreferredSize(d);
        psize=d;
    }

    public Dimension getPreferredSize() {
        return psize;
    }

    /** PropertyChangeListener interface */

    public void propertyChange(PropertyChangeEvent evt) {
        setState();
    }

    // ActionListener interface

    public void actionPerformed(ActionEvent e) {
        if (inEditMode)
            return;
        showHideWindow();
    }

    /** show/hide message window. */
    private void showHideWindow() {
        if (inEditMode)
            return;
        if (first_show) {
            if (m_window == null) {
                m_window = new AcqWindow(acqstat);
            }
            Point pt = getLocationOnScreen();
            Point pc;
            if (rightButtons)
                pc = new Point((int) pt.getX() + getWidth(), (int) pt.getY());
            else
                pc = new Point((int) pt.getX() + 40, (int) pt.getY() + 30);

            if (m_nWinLocX > 0)
                pc.x = m_nWinLocX;
            if (m_nWinLocY > 0)
                pc.y = m_nWinLocY;
            m_window.setLocation(pc);
            // m_window.setSize(m_nWinWidth, m_nWinHt);
            m_window.validate();
            first_show = false;
        }
        if (m_window.isVisible()) {
            doCloseAction();
            m_window.setVisible(false);
        } else
            m_window.setVisible(true);
        button.getModel().setPressed(m_window.isVisible());
    }

    protected void doCloseAction() {
        Point ptLoc = new Point(m_nWinLocX, m_nWinLocY);
        if (m_window.getHeight() != m_nWinHt || m_window.getWidth() != m_nWinWidth
                || m_window.getLocation() != ptLoc)
            writePersistence();
    }

    /** Update state from Infostat. */
    public void updateStatus(String msg) {
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String par = tok.nextToken().trim();
            AcqItem item = m_acqItemTable.get(par);
            if (item != null && tok.hasMoreTokens()) {
                String s = "";
                while (tok.hasMoreTokens())
                    s += tok.nextToken();
                item.setItemValue(s);
            }
        }

        tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if (parm.equals("timeleft")) {
                timeleftString=tok.nextToken();
                if(acquiring){
                    time_left=value(timeleftString);
                    if(time_start== 0 || time_start < time_left)
                        time_start= time_left;
                    if(time_start <= 0)
                        bar_value= 0.0;
                    else
                        bar_value=((double)time_left)/time_start;
                    meter.setValue(bar_value);
                }
            }
            else if (m_window !=null && parm.equals("user")){
                user=tok.nextToken().trim();
                if(!user.equals("-")){
                	String operator=Util.getLabel("_Operator", "User:");
                	m_window.setTitle(acqstat+" ("+operator+user+")");
                }
                //else
                //	m_window.setTitle(acqstat);
                //System.out.println("user="+user);
            }
            else if (parm.equals("fid"))
                fid=tok.nextToken().trim();
            else if (parm.equals("ct"))
                ct=tok.nextToken().trim();
            else if (parm.equals("status")) {
                acquiring=false;
                show_hostgain=false;
                show_hostshim=false;
                show_preacq=false;
                show_findz0=false;
                parm=tok.nextToken();
                stateStr=parm;
                if(parm.equals("Inactive")) {
                    state=STATE_OFF;
		    stateStr=Util.getLabel("sINACTIVE");
                } else if(parm.equals("SpinReg")) {
                    state=STATE_ON;
		    stateStr=Util.getLabel("sSPIN_REGULATING");
                } else if(parm.equals("VTReg")) {
                    state=STATE_ON;
		    stateStr=Util.getLabel("sVT_REGULATING");
                } else if(parm.equals("Interactive")) {
                    state=STATE_INTERACTIVE;
		    stateStr=Util.getLabel("sINTERACTIVE");
                } else if(parm.equals("Acquiring")) {
                    acquiring=true;
                    meter.setValue(1.0);
                    state=STATE_ON;
		    stateStr=Util.getLabel("sACQUIRING");
                } else if(parm.equals("HostGain")) {
                    acquiring=true;
                    show_hostgain=true;
                    meter.setValue(1.0);
                    state=STATE_ON;
		    stateStr=Util.getLabel("sHOSTGAIN");
                } else if(parm.equals("HostShim")) {
                    acquiring=true;
                    show_hostshim=true;
                    meter.setValue(1.0);
                    state=STATE_ON;
		    stateStr=Util.getLabel("sHOSTSHIM");
                } else if(parm.equals("PreAcq")) {
                    acquiring=true;
                    show_preacq=true;
                    meter.setValue(1.0);
                    state=STATE_ON;
		    stateStr=Util.getLabel("sPREACQ");
                } else if(parm.equals("FindZ0")) {
                    acquiring=true;
                    show_findz0=true;
                    meter.setValue(1.0);
                    state=STATE_ON;
		    stateStr=Util.getLabel("sFINDZ0");
                } else if(parm.equals("ProbeTune")) {
                    state=STATE_ON;
		    stateStr=Util.getLabel("sPROBETUNE");
                }
                else {
                    state=STATE_READY;
		   //  stateStr=Util.getLabel("sREADY");
		}

                if(!acquiring) {
                    time_left=time_start=0;
                    ct=fid="1";
                }
            }
            else
                return;
            setState();
        }
    }

    private String acqString() {
        if (ct.equals("-"))
            ct = "1";
        if (fid.equals("-"))
            fid = "1";
        String s = " ";
        if (show_hostgain || show_hostshim || show_preacq || show_findz0)
            s += stateStr;
        else
        {
            if (show_fids)
                s += "FID: " + fid;
            if (show_cts)
                s += " CT: " + ct;
        }
        if (show_rtime)
            s += "  " + timeleftString;

        return s;
    }

    private void setState() {
        String var = disp_names[state];
        Color c = getColor(var);
        Font f = DisplayOptions.getFont(var, var, var);
        meter.setFont(f);
        // if (acquiring)
        //     c = VnmrRgb.getColorByName("0X339900");
        meter.setForeground(c);
        button.setForeground(c);
        if (acquiring) {
            c = VnmrRgb.getColorByName("0X808080");
            meter.setBackground(c);
            meter.setString(acqString());
        }
        else {
            meter.setBackground(null);
            meter.setString(stateStr);
        }
        repaint();
    }

    protected Color getColor(String strColor) {
        Color c = null;
        if (Util.getAppIF() instanceof VAdminIF)
            c = WFontColors.getColor(strColor);
        else
            c = DisplayOptions.getColor(strColor);
        return c;
    }

    private int value(String s) {
        int val;
        if (s.equals("-"))
            return 0;
        StringTokenizer tok = new StringTokenizer(s, ":");
        String digits = tok.nextToken();
        val = 3600 * Integer.parseInt(digits);
        digits = tok.nextToken();
        val += 60 * Integer.parseInt(digits);
        val += Integer.parseInt(tok.nextToken());
        return val;
    }

    /** read persistence file ~/vnmrsys/persistence/ExperimentMeter */
    private void readPersistence() {
        BufferedReader in;
        String line;
        String symbol;
        String value;
        StringTokenizer tok;
        String filepath = FileUtil.openPath("USER/PERSISTENCE/"+persistence_file);

        if(filepath==null)
            return;
        try {
            in = new BufferedReader(new FileReader(filepath));
            while ((line = in.readLine()) != null) {
                tok = new StringTokenizer(line, " ");
                if (!tok.hasMoreTokens())
                    continue;
                symbol = tok.nextToken().trim();
                if (tok.hasMoreTokens()){
                    value = tok.nextToken().trim();
                    symbol=symbol.replace('_',' ');
                    boolean state=value.equals("true")?true:false;
                    if(symbol.equals(show_rtime_str))
                        show_rtime=state;
                    else if(symbol.equals(show_fids_str))
                        show_fids=state;
                    else if(symbol.equals(show_cts_str))
                        show_cts=state;
                    else if(symbol.equals(WINHT))
                        m_nWinHt = Integer.parseInt(value);
                    else if(symbol.equals(WINWIDTH))
                        m_nWinWidth = Integer.parseInt(value);
                    else if(symbol.equals(WINLOCX))
                        m_nWinLocX = Integer.parseInt(value);
                    else if(symbol.equals(WINLOCY))
                        m_nWinLocY = Integer.parseInt(value);
                }
            }
            in.close();
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
    }

    /** write persistence file ~/vnmrsys/persistence/ExperimentMeter */
    private void writePersistence() {
        String filepath = FileUtil.savePath("USER/PERSISTENCE/"
                + persistence_file);

        PrintWriter os;
        try {
            os = new PrintWriter(new FileWriter(filepath));
            String value;
            value = show_rtime ? "true" : "false";
            os.println(new StringBuffer().append(
                    show_rtime_str.replace(' ', '_')).append(" ").append(value)
                    .toString());

            value = show_fids ? "true" : "false";
            os.println(new StringBuffer().append(
                    show_fids_str.replace(' ', '_')).append(" ").append(value)
                    .toString());

            value = show_cts ? "true" : "false";
            os.println(new StringBuffer()
                    .append(show_cts_str.replace(' ', '_')).append(" ").append(
                            value).toString());

            // Window Height
            os.println(new StringBuffer().append(WINHT).append(" ").append(
                    m_window.getHeight()).toString());

            // WindowWidth
            os.println(new StringBuffer().append(WINWIDTH).append(" ").append(
                    m_window.getWidth()).toString());

            // Window Location
            Point ptLoc = m_window.getLocation();
            os.println(new StringBuffer().append(WINLOCX).append(" ").append(
                    ptLoc.x).toString());

            os.println(new StringBuffer().append(WINLOCY).append(" ").append(
                    ptLoc.y).toString());
            os.close();
        } catch (IOException er) {
            System.out.println(new StringBuffer().append(
                    "Error: could not create ").append(filepath).toString());
        }
    }

    // VObjIF interface

    /** set an attribute. */
    public void setAttribute(int attr, String c) {
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case DISPLAY:
            displayVal = c;
            StringTokenizer tok = new StringTokenizer(c);
            while (tok.hasMoreTokens()) {
                String s = tok.nextToken();
                if (s.equals("rightbuttons"))
                    rightButtons = true;
            }
            break;
        }
    }

    /** get an attribute. */
    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return type;
        case DISPLAY:
            return displayVal;
        default:
            return null;
        }
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
    }

    public void setEditMode(boolean s) {
        inEditMode = s;
    }

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void setDefLabel(String s) {}
    public void setDefColor(String s) {}
    public void setDefLoc(int x, int y) {}
    public void refresh() {setState();}
    public void changeFont() { }
    public void updateValue() {
        refresh();
    }
    public void setValue(ParamIF p) {}
    public void setShowValue(ParamIF p) {}
    public void changeFocus(boolean s) {isFocused = s;}
    public ButtonIF getVnmrIF() {return vnmrIf;}
    public void setVnmrIF(ButtonIF vif) {vnmrIf=vif;}
    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

   // DropTargetListener interface

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}
    public void drop(DropTargetDropEvent e) {
    VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop

    public void setSizeRatio(double w, double h) {}
    public Point getDefLoc() { return getLocation(); }

    //============== Local classes ===============================

    class WidgitLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            return psize; // unused
        } // preferredLayoutSize()

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(80, 20); // unused
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                int x;
                x = psize.width - 20;
                if (x < 10)
                    x = 10;
                int y = psize.height;
                if (rightButtons) {
                    meter.setBounds(0, 0, x, y);
                    button.setBounds(x, 1, 20, y - 2);
                } else {
                    button.setBounds(0, 1, 20, y - 2);
                    meter.setBounds(20, 0, x, y);
                }
            }
        }
    } // class WidgitLayout

    class AcqButton extends JToggleButton {
        VArrowIcon icon;
        Color bgc = Util.getBgColor();
        Color fgc = Global.BGCOLOR;

        public AcqButton() {
            setOpaque(false);
            setBorder(new VButtonBorder());
            icon = new VArrowIcon(10, 10);
            setIcon(icon);
        }

        public void setForeground(Color c) {
            fgc = c;
        }

        public void setBackground(Color c) {
            bgc = c;
            super.setBackground(c);
        }

        public void paint(Graphics g) {
            super.setBackground(bgc);
            if (acquiring)
                icon.setColor(fgc);
            else
                icon.setColor(Color.black);
            super.paint(g);
        }

        public Point getToolTipLocation(MouseEvent event) {
            return new Point(0, 0);
        }
    } // class AcqButton

    class SMenu extends JPopupMenu {
        ActionListener ShowListener;

        public SMenu() {
            String string;

            readPersistence();

            // Show selected items

            JCheckBoxMenuItem item;

            ShowListener = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    JCheckBoxMenuItem item = (JCheckBoxMenuItem) evt
                            .getSource();
                    String s = evt.getActionCommand();
                    if (s.equals(show_rtime_str))
                        show_rtime = item.isSelected() ? true : false;
                    else if (s.equals(show_fids_str))
                        show_fids = item.isSelected() ? true : false;
                    else if (s.equals(show_cts_str))
                        show_cts = item.isSelected() ? true : false;
                    setState();
                    writePersistence();
                }
            };

            item = new JCheckBoxMenuItem(show_rtime_str, show_rtime);
            item.addActionListener(ShowListener);
            add(item);

            item = new JCheckBoxMenuItem(show_fids_str, show_fids);
            item.addActionListener(ShowListener);
            add(item);

            item = new JCheckBoxMenuItem(show_cts_str, show_cts);
            item.addActionListener(ShowListener);
            add(item);
        }
    } // class SMenu

    class AcqMeter extends JComponent {
        double value = 0;
        String text = null;
        Color fgcolor = Color.black;
        Color bgcolor = Util.getBgColor();
        Font font = DisplayOptions.getFont("Dialog", Font.PLAIN, 12);

        public AcqMeter() {
            setOpaque(false);
            setBorder(BorderFactory.createLoweredBevelBorder());
        }

        public void paint(Graphics g) {
            super.paint(g);
            // Color bg=getBackground();
            Rectangle r = getBounds();
            r.grow(-2, -2);
            if (acquiring) {
                g.setColor(bgcolor);
                g.fillRect(2, 2, r.width, r.height);
                g.setColor(fgcolor);
                g.fillRect(2, 2, (int) (value * r.width), r.height);
                g.setColor(Color.black);
            } else {
                g.setColor(fgcolor);
                g.fillRect(2, 2, r.width, r.height);
                g.setColor(fgColor(fgcolor));
            }
            if (text != null) {
                g.setFont(font);
                FontMetrics metrics = g.getFontMetrics(font);
                int rw = r.width - 2;
                int sw = metrics.stringWidth(text);
                int w = (rw - sw) / 2;
                if (w < 0)
                    w = 0;
                int fh = metrics.getAscent();
                int rh = r.height - 2;
                int h = (rh - fh) / 2;
                if (h < 0)
                    h = 0;
                g.drawString(text, w, 2 + h + fh);

            }
        }

        public void setString(String t) {
            text = t;
        }

        public void setForeground(Color c) {
            fgcolor = c;
        }

        public void setBackground(Color c) {
            bgcolor = c;
        }

        public void setValue(double f) {
            value = f;
        }

        public void setFont(Font f) {
            font = f;
        }

        /** return black or white for max contrast on background color. */
        protected Color fgColor(Color c) {
            float f[] = new float[3];
            c.getColorComponents(f);
            double r = f[0];
            double g = 1.1 * f[1];
            double b = f[2];
            double mag = (r * r + g * g + b * b);
            if (mag <= 1)
                return Color.white;
            return Color.black;
        }

    } // class AcqMeter

     class AcqWindow extends JDialog implements StatusListenerIF,
            PropertyChangeListener {

        JPanel panel1 = new JPanel();
        JPanel panel2 = new JPanel();
        JPanel panel3 = new JPanel();

        public AcqWindow(String s) {
            super(VNMRFrame.getVNMRFrame()); // Attach dialog to main VJ frame
            if (VNMRFrame.getVNMRFrame() == null) {
                throw(new NullPointerException("Null owner for Dialog window"));
            }
            setTitle(s);

            addWindowListener(new WindowAdapter() {
                public void windowClosing(WindowEvent e) {
                    button.setSelected(false);
                    button.getModel().setPressed(false);
                    doCloseAction();
                }
            });
            JPanel screen = new JPanel();
            screen.setLayout(new BorderLayout());

            panel1.setLayout(new GridLayout(2, 3, 2, 5));
            
            panel1.setBorder(BorderFactory.createTitledBorder(Util.getLabel("_VExperimentMeter_status")));
            
            panel1.add(new AcqItem(Util.getLabel("_VExperimentMeter_Status"),"status"));
            panel1.add(new AcqItem(Util.getLabel("_VExperimentMeter_Exp"),"ExpId"));
            panel1.add(new AcqItem(Util.getLabel("_VExperimentMeter_Queued"),"queue"));
            panel1.add(new AcqItem(Util.getLabel("_VExperimentMeter_Sample"),"sample"));
            
            panel1.add(new AcqItem(Util.getLabel("_VExperimentMeter_fid"), "fid"));
            panel1.add(new AcqItem(Util.getLabel("_VExperimentMeter_ct"), "ct"));

            screen.add(panel1, BorderLayout.NORTH);
            
            panel2.setBorder(BorderFactory.createTitledBorder(Util.getLabel("_VExperimentMeter_times")));
            panel2.setLayout(new GridLayout(1, 3, 2, 5));

            panel2.add(new AcqItem(Util.getLabel("_VExperimentMeter_Remaining"), "timeleft"));
            panel2.add(new AcqItem(Util.getLabel("_VExperimentMeter_Complete"), "timedone"));
            panel2.add(new AcqItem(Util.getLabel("_VExperimentMeter_Stored"), "timestore"));

            screen.add(panel2, BorderLayout.CENTER);

            panel3.setBorder(BorderFactory.createTitledBorder(Util.getLabel("_VExperimentMeter_hardware")));
            panel3.setLayout(new GridLayout(1, 3, 2, 5));

            panel3.add(new AcqItem(Util.getLabel("_VExperimentMeter_Lock"), "lock"));
            //panel3.add(new AcqItem(Util.getLabel("_VExperimentMeter_Decoupler"), "dec"));
            panel3.add(new AcqItem(Util.getLabel("_VExperimentMeter_Spin"), "spin"));
            panel3.add(new AcqItem(Util.getLabel("_VExperimentMeter_Vt"), "vt"));

            screen.add(panel3, BorderLayout.SOUTH);

            getContentPane().add(screen);
            pack();
            setResizable(false);
            DisplayOptions.addChangeListener(this);
        }

        /** PropertyChangeListener interface */

        public void propertyChange(PropertyChangeEvent evt) {
            if(DisplayOptions.isUpdateUIEvent(evt)) {
                SwingUtilities.updateComponentTreeUI(this);
                SwingUtilities.updateComponentTreeUI(menu);
            }
        }

        // StatusListenerIF

        /** Message Update from Infostat. */
        public void updateStatus(String msg) {
            if (msg != null) {
                StringTokenizer tok = new StringTokenizer(msg);
                if (tok.hasMoreTokens()) {
                    String par = tok.nextToken().trim();
                    AcqItem item = m_acqItemTable.get(par);
                    if (item != null && tok.hasMoreTokens()) {
                        String s = "";
                        while (tok.hasMoreTokens())
                            s += tok.nextToken();
                        item.setItemValue(s);
                    }
                }
            }
        }

        class AcqItem extends JComponent {
            JLabel value;
            JLabel label;

            String astring = "";

 
            AcqItem(String title, String param) {
                setLayout(new GridLayout(1, 2, 10, 10));
                label = new JLabel(title);
                label.setPreferredSize(new Dimension(110, 22));
                label.setHorizontalAlignment(SwingConstants.RIGHT);
                add(label);

                value = new JLabel("");
                value.setPreferredSize(new Dimension(50, 22));
                value.setHorizontalAlignment(SwingConstants.RIGHT);
                add(value);
                m_acqItemTable.put(param, this);
                if ( ! title.equals(""))
                {
                  value.setBorder(BorderFactory.createEtchedBorder());
                  // NB: getStatusValue() can return null
                  updateStatus(ExpPanel.getStatusValue(param));
                }
                pack();
            }

            public String toString() {
                return (new StringBuffer().append(label.getText()).append(" ")
                        .append(value.getText()).toString());

            }

            void setItemValue(String s) {
                if (s.equals("-")) {
                    if (acquiring)
                        value.setText(astring);
                    else
                        value.setText("");
                } else
                    value.setText(s);
            }
        }
    }
} // class VExperimentMeter
