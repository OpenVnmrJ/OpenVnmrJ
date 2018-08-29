/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.beans.*;
import java.util.*;

import javax.swing.*;
import javax.swing.Timer;

import vnmr.util.*;


public class DpsInfoPan extends JDialog
    implements ActionListener, PropertyChangeListener {
    /** session share */
    static final int SCAN_PLUSS = 1;
    static final int SCAN_MINUS = 2;
    static final int SHIFT_LEFT = 3;
    static final int SHIFT_RIGHT = 4;

    private SessionShare sshare;
    private ButtonIF expIf;
    private Container container;
    private int infoId;
    private int sysNt = 1;
    private int sysCt = 1;
    private int newCt = 1;
    private int repeatMode = 0;
    private JPanel mainPan;
    private JPanel classPan;
    private JPanel modePan;
    private JPanel rfPan;
    private JPanel ctrlPan;
    private JPanel ctrlPan2;
    private JPanel timePan;
    private JPanel objPan;
    private JTextArea infoPan;
    private JLabel cLabel;
    private JLabel timeLabel;
    private JLabel scanLabel;
    private JLabel objLabel;
    private JScrollPane textScroll;
    private JRadioButton timeBut, powerBut, phaseBut;
    private JRadioButton realShapeRadio, absShapeRadio;
    private JCheckBox labelBut, valBut, moreBut;
    private JCheckBox decoupleBut;
    private JButton scopeBut;
    private boolean bChanging = false;
    private Font bFont;
    private Timer dpsTimer = null;
    private StringBuffer strBuf;
    private static MouseAdapter ml = null;
    private DpsArrayPan arrayPan;
    private DpsWindow dpsWindow;
    private SimpleH2Layout xLayout
        = new SimpleH2Layout(SimpleH2Layout.LEFT, 4, 0, false, false);
    
    private static String[] buttomLabels = {"null", "+Scan", "-Scan", 
              " <- ", " -> " };
    private static String[] buttomCmds = {"null", "+Scan", "-Scan",
              "<-", "->" };
    // private static int[] buttomMode = {0, SCAN_PLUSS, SCAN_MINUS,
    //           SHIFT_LEFT, SHIFT_RIGHT };


    public DpsInfoPan(ButtonIF sp, int num) {
        super(VNMRFrame.getVNMRFrame()); // Attach dialog to main VJ frame
        if (VNMRFrame.getVNMRFrame() == null) {
            throw(new NullPointerException("Owner of Dialog window is null"));
        }
        this.expIf = sp;
        this.infoId = num;

        setTitle("dps: ");
        DisplayOptions.addChangeListener(this);

        container = getContentPane();
        mainPan = new JPanel();
        mainPan.setLayout(new SimpleVLayout(0, 2, true));
        ml = new MouseAdapter() {
            public void mousePressed(MouseEvent evt) {
                repeatButtonPress(evt);
            }

            public void mouseReleased(MouseEvent evt) {
                repeatButtonRelease(evt);
            }
        };

        buildClassPan();
        buildModePan();
        buildRFPan();
        buildCtrlPan();
        buildCtrlPan2();
        mainPan.add(new JSeparator());
        buildArrayPan();
        mainPan.add(new JSeparator());
        buildTimePan();
        mainPan.add(new JSeparator());
        buildInfoPan();
        container.add(mainPan);
        pack();
        setGeom();
    } // DpsInfoPan()

    public void propertyChange(PropertyChangeEvent evt) {
        if(DisplayOptions.isUpdateUIEvent(evt))
            SwingUtilities.updateComponentTreeUI(this);
        else{
            String sName = evt.getOldValue().toString();
            if(sName.indexOf("Dps") < 0)
                return;
            if(sName.indexOf("Color") < 0)
                return;
            String sVal = "" + evt.getNewValue();
            if(sVal == null || sVal.length() < 2)
                return;
            sVal = "dps('-vj', 'color', '" + sName + "', '" + sVal + "')\n";
            expIf.sendToVnmr(sVal);
        }
    }

    public void showInfo(String str) {
        VStrTokenizer tok = new VStrTokenizer(str, " ,\n");
        if (!tok.hasMoreTokens())
            return;
        String type = tok.nextToken();
        String data;

        if (type.equals("array")) {
            data = str.substring(6);
            arrayPan.processData(data);
            return;
        }
        if (type.equals("class")) {
            bChanging = true;
            data = str.substring(tok.getPosition()).trim();
            if (data.equals("time"))
                timeBut.setSelected(true);
            else if (data.equals("power"))
                powerBut.setSelected(true);
            else if (data.equals("phase"))
                phaseBut.setSelected(true);
            bChanging = false;
            return;
        }
        if (type.equals("clear")) {
            arrayPan.clearArray();
            objLabel.setText(" ");
            infoPan.setText(" ");
            return;
        }
        if (type.equals("element")) {
            data = str.substring(tok.getPosition());
            objLabel.setText(data);
            return;
        }
        if (type.equals("exptime")) {
            data = str.substring(tok.getPosition());
            timeLabel.setText(data);
            return;
        }
        if (type.equals("info")) {
            if (!tok.hasMoreTokens())
                return;
            type = tok.nextToken();
            data = str.substring(tok.getPosition());
            if (type.equals("new")) {
                objLabel.setText(" ");
                // infoPan.setText(" ");
                if (strBuf == null)
                    strBuf = new StringBuffer(100);
                strBuf.delete(0, strBuf.length());
            } else if (type.equals("end")) {
                infoPan.setText(strBuf.toString());
                infoPan.setCaretPosition(0);
            } else {
                strBuf.append(data);
                strBuf.append("\n");
            }
            return;
        }
        if (type.equals("menu")) {
            if (!tok.hasMoreTokens())
                return;
            type = tok.nextToken();
            if (type.equals("down"))
                setVisible(false);
            else if (type.equals("up"))
                setVisible(true);
            return;
        }
        if (type.equals("mode")) {
            bChanging = true;
            data = str.substring(tok.getPosition()).trim();
            if (data.equals("label"))
                labelBut.setSelected(true);
            else if (data.equals("value"))
                valBut.setSelected(true);
            else if (data.equals("more"))
                moreBut.setSelected(true);
            else if (data.equals("decoupling"))
                decoupleBut.setSelected(true);
            bChanging = false;
            return;
        }
        if (type.equals("nt")) {
            if (!tok.hasMoreTokens())
                return;
            try {
                sysNt = Integer.parseInt(tok.nextToken());
            }
            catch(NumberFormatException ne) {
                sysNt = 1;
            }
            return;
        }
        if (type.equals("property")) {
            DisplayOptions dp = Util.getDisplayOptions();
            if (dp != null) {
                Util.showDisplayOptions();
                dp.showTab("DpsColors");
            }
            return;
        }
        if (type.equals("scan")) {
            data = str.substring(tok.getPosition());
            scanLabel.setText(data);
            return;
        }
        if (type.equals("title")) {
            data = str.substring(6);
            setTitle(data);
            return;
        }
        if (type.equals("transient")) {
            if (!tok.hasMoreTokens())
                return;
            try {
                sysCt = Integer.parseInt(tok.nextToken());
            }
            catch(NumberFormatException ne) {
                sysCt = 1;
            }
            return;
        }
        if (type.equals("view")) {
            if (!tok.hasMoreTokens())
                return;
            boolean bShow = true;
            data = tok.nextToken();
            if (tok.hasMoreTokens()) {
                String view = tok.nextToken();
                if (view.equals("off") || view.equals("no"))
                    bShow = false;
            }
            if (data.equals("decoupling")) {
                decoupleBut.setVisible(bShow);
                // scopeBut.setVisible(bShow);
            }
            return;
        }
        if (type.equals("rfshape")) {
            if (!tok.hasMoreTokens())
                return;
            bChanging = true;
            data = tok.nextToken();
            if (data.equalsIgnoreCase("absolute"))
                absShapeRadio.setSelected(true);
            else if (data.equalsIgnoreCase("real"))
                realShapeRadio.setSelected(true);
            bChanging = false;
            return;
        }
        if (type.equals("scopewindow")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken();
            if (dpsWindow == null)
                dpsWindow = new DpsWindow();
            dpsWindow.setVisible(true);
            dpsWindow.openDpsInfo(data); 
            return;
        }
    }

    public void buildClassPan() {
        classPan = new JPanel();
        classPan.setLayout(xLayout);
        cLabel = new JLabel("Class: ");
        classPan.add(cLabel);
        bFont = cLabel.getFont();
        ButtonGroup butGrp = new ButtonGroup();
        timeBut = new JRadioButton("Timing", true);
        timeBut.setActionCommand("'class','time'");
        timeBut.addActionListener(this);
        timeBut.setOpaque(false);
        butGrp.add(timeBut);
        classPan.add(timeBut);
        powerBut = new JRadioButton("Power", false);
        powerBut.setActionCommand("'class', 'power'");
        powerBut.addActionListener(this);
        powerBut.setOpaque(false);
        butGrp.add(powerBut);
        classPan.add(powerBut);
        phaseBut = new JRadioButton("Phase", false);
        phaseBut.setActionCommand("'class', 'phase'");
        phaseBut.addActionListener(this);
        phaseBut.setOpaque(false);
        butGrp.add(phaseBut);
        classPan.add(phaseBut);
        mainPan.add(classPan);
    }

    public void buildModePan() {
        modePan = new JPanel();
        modePan.setLayout(xLayout);
        JLabel lb = new JLabel("Mode: ");
        modePan.add(lb);
        bFont = lb.getFont();

        labelBut = new JCheckBox("Label", false);
        labelBut.setOpaque(false);
        labelBut.setActionCommand("'mode', 'label'");
        labelBut.setFont(bFont);
        labelBut.addActionListener(this);
        modePan.add(labelBut);

        valBut = new JCheckBox("Value", false);
        valBut.setOpaque(false);
        valBut.setActionCommand("'mode', 'value'");
        valBut.setFont(bFont);
        valBut.addActionListener(this);
        modePan.add(valBut);

        decoupleBut = new JCheckBox("Decoupling", false);
        decoupleBut.setOpaque(false);
        decoupleBut.setActionCommand("'mode', 'decoupling'");
        decoupleBut.setFont(bFont);
        decoupleBut.addActionListener(this);
        modePan.add(decoupleBut);

        moreBut = new JCheckBox("More", false);
        moreBut.setOpaque(false);
        moreBut.setActionCommand("'mode', 'more'");
        moreBut.setFont(bFont);
        moreBut.addActionListener(this);
        modePan.add(moreBut);

        mainPan.add(modePan);
        Dimension dm = lb.getPreferredSize();
        Dimension dm2 = cLabel.getPreferredSize();
        if(dm.width > dm2.width)
            cLabel.setPreferredSize(dm);
        else
            lb.setPreferredSize(dm2);
        mainPan.add(modePan);
    }

    private JButton createButton(String label, String cmd) {
        JButton but = new JButton(label);
        but.setBorder(new VButtonBorder());
        but.setActionCommand(cmd);
        but.addActionListener(this);
        return but;
    }

    private JButton createRepeatButton(int num) {
        JButton but = new JButton(buttomLabels[num]);
        but.setBorder(new VButtonBorder());
        but.setActionCommand(buttomCmds[num]);
        but.addMouseListener(ml);
        return but;
    }

    public void buildCtrlPan() {
        ctrlPan = new JPanel();
        ctrlPan.setLayout(new dpsColLayout());

        JButton but = createButton("Full", "'button', 'Full'");
        ctrlPan.add(but);
        but = createButton("+20%", "'button', '+20%'");
        ctrlPan.add(but);
        but = createButton("-20%", "'button', '-20%'");
        ctrlPan.add(but);
        but = createButton("Expand", "'button', 'Expand'");
        ctrlPan.add(but);
        but = createButton("Plot", "'button', 'Plot'");
        ctrlPan.add(but);
        scopeBut = createButton("Advanced", "'button', 'scopeview'");
        ctrlPan.add(scopeBut);
        but = createButton("Close", "'button', 'Close'");
        ctrlPan.add(but);
        mainPan.add(ctrlPan);
    }

    public void buildCtrlPan2() {
        ctrlPan2 = new JPanel();
        ctrlPan2.setLayout(new dpsColLayout());
        JButton but = createRepeatButton(SHIFT_RIGHT);
        ctrlPan2.add(but);
        but = createRepeatButton(SHIFT_LEFT);
        ctrlPan2.add(but);
        but = createRepeatButton(SCAN_MINUS);
        ctrlPan2.add(but);
        but = createRepeatButton(SCAN_PLUSS);
        ctrlPan2.add(but);
        but = createButton("Redo", "'button', 'Redo'");
        ctrlPan2.add(but);
        but = createButton("Property", "'button', 'Property'");
        ctrlPan2.add(but);
        mainPan.add(ctrlPan2);
    }

    public void buildArrayPan() {
        arrayPan = new DpsArrayPan(expIf);
        mainPan.add(arrayPan);
    }

    public void buildTimePan() {
        timePan = new JPanel();
        timePan.setLayout(new SimpleVLayout());
        timeLabel = new JLabel("Exp Time: ");
        timePan.add(timeLabel);
        scanLabel = new JLabel("Scan: ");
        timePan.add(scanLabel);
        mainPan.add(timePan);
    }

    public void buildInfoPan() {
        objPan = new JPanel();
        objPan.setLayout(xLayout);
        objLabel = new JLabel(" Element: ");
        objPan.add(objLabel);
        objLabel = new JLabel(" ");
        objLabel.setForeground(Color.blue);
        Font font = new Font(Font.SANS_SERIF, Font.BOLD, 14);
        objLabel.setFont(font);
        objPan.add(objLabel);
        infoPan = new JTextArea(" ", 12,0);
        infoPan.setOpaque(true);

        infoPan.setEditable(false);
        
        font = new Font(Font.MONOSPACED, Font.PLAIN, 14);
        infoPan.setFont(font);
        textScroll = new JScrollPane();
        textScroll.setViewportView(infoPan);


        mainPan.add(objPan);
        mainPan.add(textScroll);
    }

    public void buildRFPan() {
        rfPan = new JPanel();
        rfPan.setLayout(xLayout);
        JLabel lb = new JLabel("RF Shape: ");
        rfPan.add(lb);
        ButtonGroup butGrp = new ButtonGroup();
        absShapeRadio = new JRadioButton("Absolute", true);
        absShapeRadio.setActionCommand("'rfshape','absolute'");
        absShapeRadio.addActionListener(this);
        absShapeRadio.setOpaque(false);
        butGrp.add(absShapeRadio);
        rfPan.add(absShapeRadio);

        realShapeRadio = new JRadioButton("Real", false);
        realShapeRadio.setActionCommand("'rfshape','real'");
        realShapeRadio.addActionListener(this);
        realShapeRadio.setOpaque(false);
        butGrp.add(realShapeRadio);
        rfPan.add(realShapeRadio);
        mainPan.add(rfPan);
    }

    public void actionPerformed(ActionEvent evt) {
        if (bChanging)
            return; 
        String cmd = "dps('-vj', " + evt.getActionCommand();
        if (evt.getSource() instanceof JCheckBox) {
            if (((JCheckBox)evt.getSource()).isSelected())
                cmd = cmd + ", '1'";
            else
                cmd = cmd + ", '0'";
        }
        cmd = cmd + ")\n";
        expIf.sendToVnmr(cmd);
        repeatMode = 0;
    }


    private void paintScan() {
        String cmd = "dps('-vj', 'transient',"+newCt+")\n";
        expIf.sendToVnmr(cmd);
    }


    private void updateScanLabel() {
        if (repeatMode == SCAN_PLUSS)
           newCt++;
        if (repeatMode == SCAN_MINUS)
           newCt--;
        if (newCt < 1) {
           if (dpsTimer != null)
               dpsTimer.stop();
           newCt = 1;
        }
        else if (newCt > sysNt) {
           if (dpsTimer != null)
               dpsTimer.stop();
           newCt = sysNt;
        }
        String data = " Scan:  "+newCt+"      nt: "+sysNt; 
        scanLabel.setText(data);
    }

    private void repeatAction() {
        if (repeatMode < 1)
            return;
        if (repeatMode == SCAN_PLUSS || repeatMode == SCAN_MINUS) {
            updateScanLabel();
            return;
        }
        String cmd = "dps('-vj','button', '" + buttomCmds[repeatMode]  + "')\n";
        expIf.sendToVnmr(cmd);
    }

    private void startRepeatTimer() {
        if (dpsTimer == null) {
            ActionListener updateAction = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                     repeatAction();
                }
             };
             dpsTimer = new Timer(200, updateAction);
             dpsTimer.setInitialDelay(500);
             dpsTimer.setRepeats(true);
        }
        if (!dpsTimer.isRunning())
             dpsTimer.restart();
    }

    private void repeatButtonPress(MouseEvent e) {
        JButton but = (JButton)e.getSource();
        String cmd = but.getActionCommand();
        repeatMode = 0;
        for (int n = 1; n < buttomCmds.length; n++) {
            if (cmd.equals(buttomCmds[n])) {
               repeatMode = n;
               break;
            }
        }
        if (repeatMode < 1)
            return;
        if (repeatMode == SCAN_PLUSS || repeatMode == SCAN_MINUS) {
            newCt = sysCt;
            updateScanLabel();
        }
        if (repeatMode == SHIFT_LEFT || repeatMode == SHIFT_RIGHT) {
            cmd = "dps('-vj','button', '" + buttomCmds[repeatMode]  + "')\n";
            expIf.sendToVnmr(cmd);
        }

        startRepeatTimer();
    }

    private void repeatButtonRelease(MouseEvent e) {
        if (dpsTimer != null && dpsTimer.isRunning())
            dpsTimer.stop();
        if (repeatMode < 1)
            return;
        if (repeatMode == SCAN_PLUSS || repeatMode == SCAN_MINUS) {
            repeatMode = 0;
            sysCt = newCt;
            paintScan();
            return;
        }
        repeatMode = 0;
        String cmd = "dps('-vj', 'button', 'Stop')\n";
        expIf.sendToVnmr(cmd);
    }

    public void clearWindow() {
        repaint();
    } 

    public void setGeom() {
        sshare = Util.getSessionShare();
        if (sshare == null)
            return;
        int x, y, w, h;
        String dname = "dps" + infoId + "Geom";
        String geomStr = (String)sshare.getProperty(dname);

        if (geomStr == null)
            return;
        StringTokenizer tok = new StringTokenizer(geomStr, " ,\n");
        try {
            if (!tok.hasMoreTokens())
                return;
            x = Integer.parseInt(tok.nextToken());
            if (!tok.hasMoreTokens())
                return;
            y = Integer.parseInt(tok.nextToken());
            if (!tok.hasMoreTokens())
                return;
            w = Integer.parseInt(tok.nextToken());
            if (!tok.hasMoreTokens())
                return;
            h = Integer.parseInt(tok.nextToken());
        } catch(NumberFormatException ne) {
            return;
        }
        setLocation(new Point(x, y));
        setSize(new Dimension(w, h));
    } 

    public void  saveGeom() {
        if (sshare == null)
            return;
        Point pt = getLocation();
        Dimension dim = getSize();
        String dname = "dps" + infoId + "Geom";
        String dval = pt.x + " " + pt.y + " " + dim.width + " " + dim.height;
        sshare.putProperty(dname, dval);
    }

    public void setVisible(boolean b) {
        if (!b && isShowing())
            saveGeom();
        super.setVisible(b);
    }

    private class dpsColLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()

        public Dimension preferredLayoutSize(Container target) {
            synchronized (target.getTreeLock()) {
                int w = 0;
                int h = 0;
                Dimension r;
                int nmembers = target.getComponentCount();
                for (int i = 0; i < nmembers; i++) {
                    Component comp = target.getComponent(i);
                    r = comp.getPreferredSize();
                    if (h < r.height)
                        h = r.height;
                    w = w + r.width;
                }
                h += 4;
                return new Dimension(w, h);
            }
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension pSize = target.getSize();
                Dimension size;
                int y = 0;
                int x = 0;
                int w = 0;
                int k = 0;
                int gap = 0;
                int nmembers = target.getComponentCount();
                for (int i = 0; i < nmembers; i++) {
                    Component comp = target.getComponent(i);
                    if (comp.isVisible()) {
                        size = comp.getPreferredSize();
                        w += size.width;
                        k++;
                    }
                }
                if (k <= 0)
                    k = 2;
                x = 4;
                gap = (pSize.width - w - 4) / k;
                if (gap > 20)
                    gap = 20;
                if (gap < 0) {
                    gap = 0;
                    x = 0;
                }
                y = 0;
                for (int i = 0; i < nmembers; i++) {
                    Component comp = target.getComponent(i);
                    if (comp.isVisible()) {
                        size = comp.getPreferredSize();
                        comp.setBounds(new Rectangle(x, y, size.width,
                                                     size.height));
                        x = x + size.width + gap;
                    }
                }
            }
        } // layoutContainer()
    } // dpsColLayout

} // class DpsInfoPan

