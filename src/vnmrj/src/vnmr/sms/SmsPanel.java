/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

import javax.swing.*;
import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.beans.*;
import javax.swing.plaf.basic.*;
import javax.swing.Timer;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.bo.*;

public class SmsPanel extends JPanel
   implements SmsDef, VObjDef, PropertyChangeListener, ActionListener
{
   static final int TRAYMAX = 1;
   static final int AUTODIR = 2;
   static final int VADDR = 3;
   static final int OPERATOR = 4;
   static final int VRACK = 5;
   static final int VZONE = 6;

   private SessionShare sshare = null;
   private int ymargin = 0;
   private int curType = 0;
   private int trayNum = 0;
   private int trayType = 0;
   private int busyX = 0;
   private int busyW = 0;
   private String grkName = null;
   // private String defGrkName = null;
   private boolean setDivider = true;
   private boolean testMode = false;
   private boolean bDebug = false;
   private SmsInfoPanel infoPan;
   private JPanel trayPan;
   private SmsTray tray;
   private RackInfo rackInfo;
   private static SmsPanel smsPan;
   // private AppIF appIf = null;
   private VButton exitBut;
   private VButton zoomInBut;
   private VButton zoomOutBut;
   private VButton fullViewBut;
   private JLabel p11Label = null;
   private SmsInfoObj trayMax;
   private SmsInfoObj autoDir;
   private SmsInfoObj operator;
   private SmsInfoObj vAddr;
   private SmsInfoObj vRack;
   private JTextField selectedInfo;
   private Timer busyTimer = null;
   private boolean busy = false;
   private Color bg1;
   private Color bg2;
   private int   phase = 0;
   private int   xCycle = 10;
   private int   yCycle = 5;
   private int   cycle = 2 * xCycle + (2 * yCycle * yCycle) / xCycle;
   private static boolean bTrayTooltip = true;
   private static boolean bSelectable = true;
   private static double dratio = 0.7;
   private static JSplitPane splitPan;
   private static String m_hideTrayCmd = null;
   private static SmsColorEditor colorEditor;
   private static SmsPanel thisPanel;


   public SmsPanel(SessionShare sh, boolean debug, boolean bTest) {
        this.sshare = sh;
        this.testMode = bTest;
        splitPan = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true);
        this.bDebug = debug;
        SmsUtil.setDebugMode(debug);
        SmsUtil.setPanel(this);
        SmsUtil.setTestMode(bTest);
        trayPan = new JPanel();
        tray = new SmsTray();
        selectedInfo = new JTextField("  ");
        selectedInfo.setEditable(false);
        selectedInfo.setVisible(false);
        trayPan.setLayout(new BorderLayout());
        trayPan.add(tray, BorderLayout.CENTER);
        trayPan.add(selectedInfo, BorderLayout.SOUTH);
        SmsUtil.setTray(tray);
        infoPan = new SmsInfoPanel();
        splitPan.setLeftComponent(trayPan);
        splitPan.setRightComponent(infoPan);
        splitPan.setOneTouchExpandable(false);
        // splitPan.setUI(new BasicSplitPaneUI());
        // splitPan.setDividerSize(8);
        init();
        thisPanel = this;
        //  rackInfo = new RackInfo();
        DisplayOptions.addChangeListener(this);
        ymargin = 16;
        int h = Util.getActiveView().getTitleBarHeight();
        if (h > 12)
            ymargin = h;

/*
        appIf = Util.getAppIF();
        if (appIf != null) {
           if (appIf.pulseHandle != null) {
              if (appIf.pulseHandle.isVisible()) {
                 Rectangle r = appIf.pulseHandle.getBounds();
                 ymargin = r.height + 2;
              }
           }
        }
*/
        setLayout(new smsPanLayout());
        ButtonEar bEar = new ButtonEar();
        exitBut = new VButton(sshare, null, "button");
        // exitBut.setText("X");
        exitBut.setAttribute(VObjDef.ICON, "sampleTrayClose.gif");
        exitBut.setToolTipText(Util.getTooltipString("Close tray panel"));
        exitBut.addActionListener(bEar);
        exitBut.setActionCommand("Close");
/*
        exitBut.setVnmrIF(Util.getActiveView());
        exitBut.setAttribute(CMD, "vnmrjcmd('edit', 'tray', 'off')");
*/
        add(exitBut);

        zoomOutBut = new VButton(sshare, null, "button");
        zoomOutBut.setText("-");
        // zoomOutBut.setToolTipText("Zoom Out");
        zoomOutBut.setToolTipText(Util.getTooltipString("Zoom out"));
        zoomOutBut.addActionListener(bEar);
        zoomOutBut.setActionCommand("Decr");
        zoomOutBut.setVisible(false);
        add(zoomOutBut);
        fullViewBut = new VButton(sshare, null, "button");
        fullViewBut.setText("o");
        // fullViewBut.setToolTipText("Full");
        fullViewBut.setToolTipText(Util.getTooltipString("Full view"));
        fullViewBut.addActionListener(bEar);
        fullViewBut.setActionCommand("Full");
        fullViewBut.setVisible(false);
        add(fullViewBut);
        zoomInBut = new VButton(sshare, null, "button");
        zoomInBut.setText("+");
        // zoomInBut.setToolTipText("Zoom In");
        zoomInBut.setToolTipText(Util.getTooltipString("Zoom in"));
        zoomInBut.addActionListener(bEar);
        zoomInBut.setActionCommand("Incr");
        zoomInBut.setVisible(false);
        add(zoomInBut);

        add(splitPan);
        trayMax = new SmsInfoObj("$VALUE=traymax", TRAYMAX);
        autoDir = new SmsInfoObj("$VALUE=autodir", AUTODIR);
        operator = new SmsInfoObj("$VALUE=operator", OPERATOR);
        vAddr = new SmsInfoObj("$VALUE=vnmraddr", VADDR);
        vRack = new SmsInfoObj("$VALUE=vrack", VRACK);
        setBg();
        setReUsable();
        showColorEditor(null, null);
        tray.showTooltip(bTrayTooltip);
        tray.setSelectable(bSelectable);
   }

   public SmsPanel(SessionShare sh, boolean debug) {
        this(sh, debug, false);
   }

   public SmsPanel(SessionShare sh) {
        this(sh, false, false);
   }

   private void setBg() {
        Color color = Util.getBgColor();
        setBackground(color);
        splitPan.setBackground(color);
        tray.setBg(Color.black);
        infoPan.setBg(color);
        selectedInfo.setBackground(color);
	bg1 = color;
        bg2 = bg1.brighter();
        if (bg2.equals(bg1)) {
            bg2 = bg1.darker();
        }
    }

   public void propertyChange(PropertyChangeEvent evt) {
        setBg();
        if (DisplayOptions.isUpdateUIEvent(evt))
            SwingUtilities.updateComponentTreeUI(exitBut);
   }

   public void setDebugMode(boolean b) {
        bDebug = b;
        SmsUtil.setDebugMode(b);
        infoPan.setDebugMode(b);
   }

   private void init() {
        sshare = Util.getSessionShare();
        if (sshare == null)
           return;

        Double fy = (Double) sshare.getProperty("asmLocX");
        if (fy != null && !fy.isNaN())
            dratio = fy.doubleValue();
        if (dratio > 1.0)
            dratio = 0.9;
        // defGrkName = (String) hs.get("grkName");
        Boolean b = (Boolean) sshare.getProperty("trayTooltip");
        if (b != null)
            bTrayTooltip = b.booleanValue();
   }

   public void  showSampleInfo(SmsSample s) {
        infoPan.showSampleInfo(s);
   }

   public void  showSelected(String s) {
        selectedInfo.setText(s);
   }

   public int getTrayType() {
        return trayType;
   }

   public void setTrayType(String s) {
        int k = 0;
        int n = 0;
        if (bDebug)
           System.out.println("set new tray type: "+s+",  old type: "+trayNum);
        if (s == null)
            return;
        try {
            n = Integer.parseInt(s);
        }
        catch (NumberFormatException er) { n = 0; }
        if (n == trayNum)
            return;

        switch (n) {
            case 1:
                k = SMS;
                break;
            case 9:
                   k = CAROSOL;
                   break;
            case 50:
                   k = SMS;
                   break;
            case 100:
                   k = SMS100;
                   break;
            case 768:
                   k = VAST;
                   break;
            case 96:
                   k = VAST;
                   break;
            case 49:
                   k = GRK49;
                   break;
            case 97:
                   k = GRK97;
                   break;
            case 12:
                   k = GRK12;
                   break;
        }
        if (k == 0) {
            Messages.postError("traymax '"+s+"' not supported. ");
            return;
        }
        trayNum = n;
        trayType = k;
        if ((k == VAST) || (k == GRK49) || (k == GRK97) || (k == GRK12)) {
            if (!testMode)
               tray.setShowPlate(false);
            if (rackInfo == null)
               rackInfo = new RackInfo(trayType);
            else {
               rackInfo.setTrayType(k);
            }
            if ((k == GRK49) || (k == GRK97) || (k == GRK12)) {
               tray.setShowPlate(true);
	       tray.setZoom(0);
               showGrkPlate("1");
            }
        }
        else {
            tray.setShowPlate(true);
        }
        setType(k);
   }

   public synchronized void setVnmrInfo(int type, String message) {
        if (type == TRAYMAX) {
            if (bDebug)
              System.out.println("SMS got traymax: "+message);
            if (message == null) {
                Messages.postError("traymax is null ");
                return;
            }
            setTrayType(message);
/*
            if (curType == VAST) {
               if (defGrkName != null && (!defGrkName.equals(grkName)))
                  infoPan.setRackMenu(defGrkName);
            }
*/
            if (curType == VAST) {
               vRack.setVal = "$VALUE=vrack";
               vRack.retType = VRACK;
               vRack.updateValue();
            }
            return;
        }
        if (type == AUTODIR) {
            if (bDebug)
              System.out.println("SMS got autodir: "+message);
            SmsUtil.setAutoDir(message);
            return;
        }
        if (type == VADDR) {
            SmsUtil.setVnmrAddr(message);
            return;
        }
        if (type == OPERATOR) {
            if (bDebug)
              System.out.println("SMS got operator: "+message);
            SmsUtil.setOperator(message);
            return;
        }
        if (type == VRACK) {
           if (message != null && message.length() > 0) {
                // infoPan.setRackMenu(message);
           }
           if (bDebug)
              System.out.println("SMS got vrack: "+message);
           vRack.setVal = "$VALUE=vzone";
           vRack.retType = VZONE;
           vRack.updateValue();
          //  showGrkPlate(message);
            return;
        }
        if (type == VZONE) {
           if (message != null && message.length() > 0) {
                // infoPan.setZoneMenu(message);
           }
           if (bDebug)
              System.out.println("SMS got vzone: "+message);
           tray.setShowPlate(true);
	   tray.setZoom(0);
        }

   }

   public void deltaSmaple(String f) {
        if (f == null)
           return;
        SmsUtil.deltaSmaple(f);
   }

   public void removeSmaple(String f) {
        if (f == null)
           return;
        SmsUtil.removeSmaple(f);
   }

   public void setType(int n) {
        curType = n;
        tray.setType(n);
        infoPan.setType(n);
        if ((n == VAST) || (n == GRK49) || (n == GRK97) || (n == GRK12)) {
            zoomOutBut.setVisible(true);
            fullViewBut.setVisible(true);
            zoomInBut.setVisible(true);
            selectedInfo.setVisible(true);
        }
        else {
            zoomOutBut.setVisible(false);
            fullViewBut.setVisible(false);
            zoomInBut.setVisible(false);
            selectedInfo.setVisible(false);
        }
        repaint();
   }

   public void showGrkPlate(String numStr) {
        GrkPlate plate = rackInfo.getPlate(numStr);
        if (plate != null) {
              tray.showGrkPlate(plate);
              infoPan.updateZoneMenu(numStr);  
              grkName = numStr;
              // defGrkName = numStr;
        }
   }

   public void submit(ButtonIF bif, String s) {
        tray.submit(bif, s);
   }

   public void dbAction(String s) {
        // do not depend on DB any more.
        // SmsUtil.updatePlateInfo();
   }

   public void setZone(String s) {
        int n = 0;
        if (s == null)
           return;
        if ( s.equalsIgnoreCase("All") ||
             s.equalsIgnoreCase("1,2") )
           n = 999;
        else if ( s.equalsIgnoreCase("2,1"))
           n = 998;
        else {
           try {
             n = Integer.parseInt(s);
           }
           catch (NumberFormatException e) {}
        }
        tray.setZone(n);
        repaint();
   }

   public void setP11svfdir(ImageIcon icon, String tip) {
        if (icon == null) {
	   if (p11Label != null) 
	      p11Label.setVisible(false);
           validate();
           return;
        }
	if (p11Label == null) {
           p11Label = new JLabel(icon);
           add(p11Label);
        }
	else
           p11Label.setIcon(icon);
        // p11Label.setToolTipText(tip);
        p11Label.setToolTipText(Util.getTooltipString(tip));
	p11Label.setVisible(true);
        validate();
   }

   private static void addMenuBar(JFrame f) {
        JMenuBar mb = new JMenuBar();
        f.setJMenuBar(mb);
        JMenu fileMenu = new JMenu("File");
        FileMenuEar fme = new FileMenuEar();
        JMenuItem exit = new JMenuItem("Exit");
        exit.addActionListener(fme);
        fileMenu.add(exit);
        mb.add(fileMenu);

        JMenu changer = new JMenu("Changer");
        ChangerEar ce = new ChangerEar();
        JMenuItem smsItem = new JMenuItem("SMS");
        smsItem.addActionListener( ce );
        changer.add(smsItem);
        smsItem = new JMenuItem("SMS100");
        smsItem.addActionListener( ce );
        changer.add(smsItem);
        JMenuItem caroItem = new JMenuItem("Carousel");
        caroItem.addActionListener( ce );
        changer.add(caroItem);
        JMenuItem gilItem = new JMenuItem("Gilson");
        gilItem.addActionListener( ce );
        changer.add(gilItem);
        mb.add(changer);
    }

    private void setReUsable() {
        String d = "SYSTEM/USRS/operators/automation.conf";
        String f = FileUtil.openPath(d);
        if (f == null)
           return;
        boolean reUsable = false;
        try {
           BufferedReader  fin = new BufferedReader(new FileReader(f));
           String data;
           while ((data = fin.readLine()) != null) {
              d = data.trim();
              if (!d.startsWith("Sample"))
                 continue;
              StringTokenizer tok = new StringTokenizer(d, " \t\n");
              d = tok.nextToken().trim();
              if (!d.equalsIgnoreCase("SampleReuse"))
                 continue;
              if (tok.hasMoreTokens()) {
                 d = tok.nextToken().trim();
                 if (d.equalsIgnoreCase("yes"))
                     reUsable = true; 
                 break;
              }
           }
           fin.close();
        }
        catch(IOException e) {}
        SmsUtil.setReusable(reUsable);
    }

    public void updatePlateInfo() {
        if (isVisible()) {
           operator.updateValue();
           autoDir.updateValue();
        }
    }

    public void updatePlate() {
        if (curType == VAST) {
            tray.setShowPlate(false);
        } else {
            tray.startTimer();
        }
        setReUsable();
        operator.updateValue();
        //autoDir.updateValue();
        trayMax.updateValue();
        vAddr.updateValue();
        autoDir.updateValue();
    }

    public void setVisible(boolean b) {
        if (b) {
            updatePlate();
            tray.updateValue();
/*
            if (curType == VAST)
                 tray.setShowPlate(false);
            setReUsable();
            operator.updateValue();
            autoDir.updateValue();
            trayMax.updateValue();
            vAddr.updateValue();
*/
        }
        else {
            tray.cleanUp();
            if (sshare != null) {
               int k = splitPan.getDividerLocation();
               int w = getWidth();
               dratio = (double) k / w;
               sshare.putProperty("asmLocX", new Double(dratio));
               if (curType == VAST && grkName != null)
                  sshare.putProperty("grkName", grkName);
            }
            // trayNum = 0;
        }
        super.setVisible(b);
    }

    private void execSelectable(String str) {
        StringTokenizer tok = new StringTokenizer(str, " ,\n");
        if (!tok.hasMoreTokens())
            return;
        setSelectable(tok.nextToken().trim());
    }

    private void execSelect(String str) {
        StringTokenizer tok = new StringTokenizer(str, " ,\n");
        String data;

        if (tray == null)
             return;
        Vector<String> v = new Vector<String>();
        while (tok.hasMoreTokens()) {
             data = tok.nextToken().trim().toLowerCase();
             if (data.equalsIgnoreCase("all")) {
                 tray.setSelection(true);
                 return;
             }
             if (data.equalsIgnoreCase("none")) {
                 tray.setSelection(false);
             }
             else if (data.length() > 0) {
                 try {
                     int i = Integer.parseInt(data);
                     if (i >= 0)
                        v.add(data);
                 }
                 catch (NumberFormatException er) {}
             }
        }
        tray.setSelection(v);
    }

    public void execCmd(String cmd, String attr) {
        if (cmd.equalsIgnoreCase(SmsCmds.UPDATE)) { // update
            if (isVisible())
                updatePlate();
            return;
        }
        if (cmd.equalsIgnoreCase(SmsCmds.SELECT)) {  // select
            execSelect(attr);
            repaint();
            infoPan.showSampleInfo(); 
            return;
        }
        if (cmd.equalsIgnoreCase(SmsCmds.SELECTABLE)) {  // selectable
            execSelectable(attr);
            return;
        }
        if (cmd.equalsIgnoreCase(SmsCmds.DELTA)) {  // delta
            if (attr != null)
               SmsUtil.deltaSmaple(attr);
        } else if (cmd.equalsIgnoreCase(SmsCmds.REMOVE)) {  // remove
            if (attr != null)
               SmsUtil.removeSmaple(attr);
        } else if (cmd.equalsIgnoreCase(SmsCmds.CLOSE_BUTTON)) { // closeButton
            if (m_hideTrayCmd  != null) {
                Util.sendToVnmr(m_hideTrayCmd);
            } else {
                ButtonIF vif = Util.getActiveView();
                if (vif != null)
                   vif.sendVnmrCmd(exitBut, "vnmrjcmd('edit', 'tray', 'off')");
            }
        } else if (cmd.equalsIgnoreCase(SmsCmds.CLEAR)) { // clearselect
            if (tray != null) {
                tray.clearSelect();
            }
        }

        repaint();
        infoPan.showSampleInfo(); 
    }

    public void setTrayTooltip() {
        if (sshare != null)
            sshare.putProperty("trayTooltip", new Boolean(bTrayTooltip));
        if (tray != null)
            tray.showTooltip(bTrayTooltip);
    }

    public void setSelectable(boolean b) {
        bSelectable = b;
        if (tray != null)
            tray.setSelectable(bSelectable);
    }

    public static void setSelectable(String cmd) {
         if (cmd == null || cmd.length() < 1)
             return;
         bSelectable = true;
         if (cmd.equalsIgnoreCase("no") || cmd.equalsIgnoreCase("false"))
             bSelectable = false;
         if (thisPanel != null)
            thisPanel.setSelectable(bSelectable);
    }

    public static void setHideTrayCmd(String cmd) {
        m_hideTrayCmd = cmd;
    }

    public static void showColorEditor(ButtonIF vif, String cmd) {
        if (colorEditor == null) {
           if (vif == null)
              vif = Util.getActiveView();
           colorEditor = new SmsColorEditor(vif);
        }
        colorEditor.execCmd(cmd);
    }

    public static void showTrayTooltip(String cmd) {
        bTrayTooltip = true;
        if (cmd != null) {
            if (cmd.equalsIgnoreCase("close"))
                bTrayTooltip = false;
            else if (cmd.equalsIgnoreCase("off"))
                bTrayTooltip = false;
        }
        if (thisPanel != null)
            thisPanel.setTrayTooltip();
    }


    public void drawBusyBar() {
	Graphics g = getGraphics();
	Graphics2D g2 = (Graphics2D) g;

	if (busy) {
            g2.setPaint(new GradientPaint(phase, 0, bg1,
                                       phase + xCycle, yCycle, bg2, true));
            g2.fill(new Rectangle(busyX, 0, busyW, ymargin));
	    phase = ++phase % cycle;
/*
	    if (zoomOutBut.isVisible()) {
		zoomOutBut.repaint();
		fullViewBut.repaint();
		zoomInBut.repaint();
	    }
*/
        } else {
            repaint();
        }
    }

    public void actionPerformed(ActionEvent e) {
       Object obj = e.getSource();
       if (obj instanceof Timer) {
            drawBusyBar();
       }
    }


    private void startTimer() {
       if (busyTimer == null)
            busyTimer = new Timer(200, this);
       if (busyTimer != null && (!busyTimer.isRunning()))
            busyTimer.restart();
   }

    public void setBusy(boolean bz) {
            busy = bz;
            if (busy) {
                startTimer();
	        drawBusyBar();
            } else {
		if (busyTimer != null)
                    busyTimer.stop();
                repaint();
            }
            // repaint();
    }


    
    public static void main(String[] args) {
        JFrame frame = new JFrame("Sample Tray");
        addMenuBar(frame); 
        smsPan = new SmsPanel(null, true, true);
        Container cont = frame.getContentPane();
        cont.add(smsPan, BorderLayout.CENTER);
        SmsUtil.setAutoDir(System.getProperty("autodir"));
        smsPan.setType(SMS);
        frame.pack();
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        Toolkit tk = Toolkit.getDefaultToolkit();
        Dimension screenDim = tk.getScreenSize();
        int h = screenDim.height - 200;
        int w = h * 9 / 10;
        frame.setSize(w, h);
        frame.setLocation(60, 0);
         w = w * 7 / 10;
        splitPan.setDividerLocation(w);
        frame.setVisible(true);
    }

    protected class WBasicSplitPaneUI extends BasicSplitPaneUI {
            /**
             * Creates the default divider.
             */
            public BasicSplitPaneDivider createDefaultDivider()
            {
                return new WBasicSplitPaneDivider(this);
            }

    }

    protected class WBasicSplitPaneDivider extends BasicSplitPaneDivider {
            public WBasicSplitPaneDivider(WBasicSplitPaneUI ui)
            {
                super(ui);
            }

    }

    protected static class FileMenuEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            System.out.println(" Exit...");
            System.exit(0);
        }
    }

    protected static class ChangerEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            String str = (String) ((JMenuItem)ae.getSource()).getText();
            if (str.equalsIgnoreCase("SMS")) {
               smsPan.setType(SMS);
               return;
            }
            if (str.equalsIgnoreCase("SMS100")) {
               smsPan.setType(SMS100);
               return;
            }
            if (str.equalsIgnoreCase("Carousel")) {
               smsPan.setType(CAROSOL);
               return;
            }
            if (str.equalsIgnoreCase("Gilson")) {
               smsPan.setTrayType("96");
            }
        }
    }

    protected class ButtonEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            String str = ae.getActionCommand();
            if (str.equalsIgnoreCase("Close")) {
                execCmd(SmsCmds.CLOSE_BUTTON, null);
                return;
            }
            if (str.equalsIgnoreCase("Incr")) {
                tray.setZoom(1);
                return;
            }
            if (str.equalsIgnoreCase("Decr")) {
                tray.setZoom(-1);
                return;
            }
            if (str.equalsIgnoreCase("Full")) {
                tray.setZoom(0);
                return;
            }
        }
    }

    private class smsPanLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
        } 

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim = target.getSize();
                if (dim.width < 20 || dim.height < 20)
                    return;
                int x = 0;
                exitBut.setBounds(0, 0, ymargin, ymargin);
		busyX = ymargin + 2;
		busyW = dim.width - busyX;
                x = ymargin + 20; 
                zoomOutBut.setBounds(x, 0, ymargin, ymargin);
                x += ymargin;
                fullViewBut.setBounds(x, 0, ymargin, ymargin);
                x += ymargin;
                zoomInBut.setBounds(x, 0, ymargin, ymargin);
                if (p11Label != null && p11Label.isVisible()) {
                    Dimension dim2 = p11Label.getPreferredSize();
                    x = dim.width - dim2.width - 4;
                    p11Label.setBounds(x, 0, dim2.width, dim2.height);
                }
 
                if (setDivider) {
                    setDivider = false;
                    x = (int) ((double) dim.width * dratio);
                    splitPan.setDividerLocation(x);
                }
                splitPan.setBounds(0, ymargin, dim.width, dim.height - ymargin);
            }
        }
    }
}

