/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.cryo;

import javax.swing.*;

import java.awt.*;
import java.awt.event.*;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.*;

import vnmr.ui.ExpPanel;
import vnmr.ui.VNMRFrame;
import vnmr.util.*;
import vnmr.bo.*;

import static javax.swing.ScrollPaneConstants.*;

import static java.awt.Color.WHITE;

import static vnmr.cryo.Priority.*;
import static vnmr.cryo.CryoPort.*;
import static vnmr.cryo.CryoConstants.*;


/**
 * This class holds the pop-up CryoBay controls panel.
 */
public class CryoDiagnostics extends ModelessDialog implements ActionListener {
    
    private static final String UPLOAD_FILE_NAME = "UploadPath";
    private static final String UPLOAD_FILE_NAME_AUTO = "UploadAutoName";

    /** The action command for the Close button. */
    private static final String CLOSE_COMMAND = "Close";

    /** The action command for the Start button. */
    private static final String START = "start";

    /** The action command for the Stop button. */
    private static final String STOP = "stop";

    /** The action command for the Detach button. */
    private static final String DETACH = "detach";

    /** The action command for the Vacuum Purge button. */
    private static final String VACPURGE = "vacpurge";

    /** The action command for the Pumping Probe button. */
    private static final String PROBEPUMP = "probepump";

    /** The action command for the Thermal Cycle button. */
    private static final String THERMCYCLE = "thermcycle";

    /** The action command for the send to Cryo button. */
    private static final String CRYO = "sendToCryobay";

    /** The action command for the send to Temp button. */
    private static final String TEMP = "sendToTemp";

    /** The action command for the firmware config menu. */
    private static final String FW_CONFIG = "firmwareConfig";

    /** The action command for the firmware config menu. */
    private static final String CRYO_CONFIG = "cryobay";

    /** The action command for the firmware config menu. */
    private static final String TC_CONFIG = "temperaturecontroller";

    /** The action command for the probe calibration menu item. */
    private static final String PROBE_CONFIG = "probeConfig";

    /** The action command for the Advanced menu item */
    private static final String ADVANCED = "advanced";

    /** The action command for the about cryo menu item*/
    private static final String ABOUT_CRYO = "aboutCryo";

    /** The action command for the about comp menu item */
    private static final String ABOUT_COMP = "aboutComp";

    /** The action command for the online help menu item. */
    private static final String HELP = "onlineHelp";

    private static String m_cryotext;
    private static String m_temptext;

    /** The background color of this popup. */
    private Color m_background;

    /** Handle for communication with VnmrBG. */
    private ButtonIF m_vif;

    /** The panel holding the components for the diagnostics. */
    private CryoPanel m_varsPanel;

    /** The socket to communicate with the cryobay. */
    private CryoSocketControl m_cryobay;

    /** The socket to communicate with the cryocon temperature controller. */
    private CryoSocketControl m_tempcontroller;

    /** The socket to communicate with the cryocon data port. */
    private CryoSocketControl m_dataport;

    private JScrollPane msgScrollPane;
    private JCheckBoxMenuItem advanced;
    private JTextArea textArea;

    private String m_dateFormat;
    private JRadioButtonMenuItem m_autoNameMenuItem;
    private JFileChooser m_uploadFileChooser;
    private String m_uploadFilePath = null;
    private MessageIF m_cryoMsg;

    private JMenu probecalMenu;
    private JMenu configMenu;

    JRadioButtonMenuItem excelDates;
    JRadioButtonMenuItem stringDates;
    JRadioButtonMenuItem excelDatesLocal;
    JRadioButtonMenuItem stringDatesLocal;
    JRadioButtonMenuItem unixDates;


    public CryoDiagnostics(ButtonIF vif,
                           CryoSocketControl cryobay,
                           CryoSocketControl tempcontroller,
                           CryoSocketControl datacontroller,
                           MessageIF cryoMsg,
                           String dataDir) {
        super("");
        m_vif = vif;
        m_cryobay = cryobay;
        m_tempcontroller = tempcontroller;
        m_dataport = datacontroller;
        m_cryoMsg = cryoMsg;

        m_uploadFileChooser = new JFileChooser(dataDir);

        // Hook up ModelessDialog buttons
        setHistoryEnabled(false);
        getHistoryBtn().setVisible(false);
        setHelpEnabled(false);
        getHelpBtn().setVisible(false);
        setUndoEnabled(false);
        getUndoBtn().setVisible(false);
        setAbandonEnabled(false);
        getAbndnBtn().setVisible(false);
        setCloseAction(CLOSE_COMMAND, this);

        layoutPanel();

        // Finally, pack (but don't show) and set window geometry
        pack();
        //setPanelSize();
        setLocationRelativeTo(VNMRFrame.getVNMRFrame());
        setResizable(true);

        setTitle();

        setDetach(false);
        setStart(false);
        setStop(false);
        setVacPurge(false);
        setProbePump(false);
        setThermCycle(false);
        setCryoSend(false);
        setThermSend(false);

        m_background = Util.getBgColor();
        setBackground(m_background);
        buttonPane.setBackground(null);
        abandonButton.setBackground(null);
        closeButton.setBackground(null);
        helpButton.setBackground(null);
        historyButton.setBackground(null);
        undoButton.setBackground(null);
        getContentPane().setBackground(null);
    }

    public void setDataport(CryoSocketControl dataport) {
        m_dataport = dataport;
    }

    public CryoSocketControl getDataport() {
        return m_dataport;
    }

    public void setCryobay(CryoSocketControl cryobay) {
        m_cryobay = cryobay;
    }

    public CryoSocketControl getCryobay() {
        return m_cryobay;
    }

    public void setTempcontroller(CryoSocketControl tempcontroller) {
        m_tempcontroller = tempcontroller;
    }

    public Frame getFrame() {
        return JOptionPane.getFrameForComponent(this);
    }

    public void setTitle() {
        super.setTitle("CryoBay Controls");
    }

    @Override
    public void setVisible(boolean v) {
       if (v) {
           addFileItems(probecalMenu, "", "cryo/probecal");
           addFileItems(configMenu, "", "cryo/config");
       }
       super.setVisible(v);
    }

    private void layoutPanel() {

        buildMenuBar();

        // Base pane
        JPanel basePane = new JPanel();
        basePane.setBackground(null);
        basePane.setLayout(new BorderLayout());
        getContentPane().add(basePane, BorderLayout.CENTER);

        // Base panel for method controls
        JPanel westPanel = new JPanel();
        westPanel.setBackground(null);
        westPanel.setLayout(new BorderLayout());
        basePane.add(westPanel, BorderLayout.WEST);

        // Method controls
        m_varsPanel = new CryoPanel(null, m_vif, "cryoDiag",
                                    "INTERFACE/CryoControls.xml", m_cryoMsg);
        m_varsPanel.setActionListener(this);
        m_varsPanel.setStatusListener();
        m_varsPanel.setBackground(null);
        m_varsPanel.updateValue();

        JScrollPane varsScrollPane
                = new JScrollPane(m_varsPanel,
                                  VERTICAL_SCROLLBAR_AS_NEEDED,
                                  HORIZONTAL_SCROLLBAR_NEVER);
        varsScrollPane.setBackground(null);
        varsScrollPane.getViewport().setBackground(null);
        westPanel.add(varsScrollPane, BorderLayout.CENTER);

        textArea = new JTextArea(11, 30);
        textArea.setEditable(false);
        textArea.setBackground(WHITE);
        textArea.setFont(Font.decode("SanSerif-Plain-12"));
        msgScrollPane = new JScrollPane(textArea,
                VERTICAL_SCROLLBAR_AS_NEEDED,
                HORIZONTAL_SCROLLBAR_AS_NEEDED);
        westPanel.add(msgScrollPane, BorderLayout.SOUTH);
        msgScrollPane.setVisible(true);
    }

    public void setAdvanced(boolean state) {
        if(m_varsPanel!=null) {
                m_varsPanel.setAdvanced(state);
                m_varsPanel.refresh();
        }
//        if(msgScrollPane!=null) {
//            msgScrollPane.setVisible(state);
//        }
//        this.repaint();
//        m_varsPanel.repaint();
//        this.pack();
    }


    public void writeAdvanced(String msg) {
        if (msg.lastIndexOf("\n") != msg.length() - 1) {
            msg = msg + "\n";
        }
        textArea.append(msg);
        scrollToBottom(msgScrollPane);
    }

    /**
     * Scroll the given JScrollPane to the bottom.
     * @param scrollPane The JScrollPane to adjust.
     */
    private void scrollToBottom(JScrollPane scrollPane) {
        if (scrollPane != null){
            JScrollBar bar = scrollPane.getVerticalScrollBar();
            bar.setValue(bar.getMaximum());
        }
    }

    public void setDateFormat(String fmt) {
        Messages.postDebug("CryoDiagnostics.setDateFormat(" + fmt + ")");
        m_dateFormat = fmt;
        if (m_cryobay != null) {
            m_cryobay.setDateFormat(fmt);
        }
        if (m_tempcontroller != null) {
            m_tempcontroller.setDateFormat(fmt);
        }
        if (m_dataport != null) {
            m_dataport.setDateFormat(fmt);
        }
        if (fmt.equals(STRING_DATES)) {
            stringDates.setSelected(true);
        } else if (fmt.equals(STRING_DATES_LOCAL)) {
            stringDatesLocal.setSelected(true);
        } else if (fmt.equals(EXCEL_DATES)) {
            excelDates.setSelected(true);
        } else if (fmt.equals(EXCEL_DATES_LOCAL)) {
            excelDatesLocal.setSelected(true);
        } else {
            unixDates.setSelected(true);
        }
   }

    public void setDetach(boolean state) {
        if(m_varsPanel!=null) m_varsPanel.setDetach(state);
    }

    public void setStart(boolean state) {
        if(m_varsPanel!=null) m_varsPanel.setStart(state);
    }

    public void setStop(boolean state) {
        if(m_varsPanel!=null) m_varsPanel.setStop(state);
    }

    public void stopNMR() {
        m_vif.sendToVnmr("aa");
        m_cryoMsg.postError("Cryobay Error.  Stopping Acquisition");
    }

    public void setVacPurge(boolean state) {
        if(m_varsPanel!=null) m_varsPanel.setVacPurge(state);
    }

    public void setProbePump(boolean state) {
        if(m_varsPanel!=null) m_varsPanel.setProbePump(state);
    }

    public void setThermCycle(boolean state) {
        if(m_varsPanel!=null) m_varsPanel.setThermCycle(state);
    }

    public void setCryoSend(boolean state) {
        if(m_varsPanel!=null) m_varsPanel.setCryoSend(state);
    }

    public void setThermSend(boolean state) {
        if(m_varsPanel!=null) m_varsPanel.setThermSend(state);
    }

    /**
     * Open Default Browser for Online Help
     */
    public void onlineHelp(){
        String filepath;
        String url = null;
        String browser = null;
        String helpFile;
        String line;
        BufferedReader in;

        filepath= FileUtil.openPath("SYSTEM" + "/cryo/");
        helpFile= filepath+"help.url";

        try { in = new BufferedReader(new FileReader(helpFile)); }
        catch (FileNotFoundException e) {
                m_cryoMsg.postError("Cryobay Help address file not found: " + helpFile);
            return;
        }
        try{
                // Read in from file
                while ((line = in.readLine()) != null) {
                        if(line.startsWith("http"))
                                        url = new String(line);
                        else
                                        browser = new String(line);
                }
        } catch(Exception e){
                m_cryoMsg.postError("Cryo Cannot read help file.");
        }

        try {
                Runtime.getRuntime().exec(new String[] {browser, url});
        } catch (Exception e){
                m_cryoMsg.postError("Cryo Online Help: Browser not found " + e);
        }
    }

    private void buildMenuBar() {
        JMenuBar menuBar = new JMenuBar();
        menuBar.setOpaque(true);
        menuBar.setBackground(Util.getMenuBarBg());
        menuBar.setBorder(BorderFactory.createEmptyBorder());
        setJMenuBar(menuBar);

        // File menu
        JMenu fileMenu = new JMenu("File");
        menuBar.add(fileMenu);
        fileMenu.setMnemonic(KeyEvent.VK_F);

        // For submenus
        JMenuItem menuItem;
        // JMenu menu;             // For submenus

        probecalMenu = new JMenu("Load to Probe...");
        probecalMenu.setMnemonic(KeyEvent.VK_L);
        probecalMenu.setActionCommand(PROBE_CONFIG);
        fileMenu.add(probecalMenu);
        probecalMenu.setBackground(Util.getBgColor());
        addFileItems(probecalMenu, "", "cryo/probecal");

        configMenu = new JMenu("Firmware Configuration...");
        configMenu.setActionCommand(FW_CONFIG);
        configMenu.setMnemonic(KeyEvent.VK_F);
        fileMenu.add(configMenu);
        configMenu.setBackground(Util.getBgColor());
        addFileItems(configMenu, "", "cryo/config");

        fileMenu.addSeparator();

        menuItem = new JMenuItem("Close");
        menuItem.setMnemonic(KeyEvent.VK_C);
        menuItem.setActionCommand(CLOSE_COMMAND);
        menuItem.addActionListener(this);
        menuItem.setBackground(Util.getBgColor());
        fileMenu.add(menuItem);

        //  Options menu
        JMenu optionsMenu = new JMenu("Options");
        menuBar.add(optionsMenu);
        optionsMenu.setMnemonic(KeyEvent.VK_O);

        advanced = new JCheckBoxMenuItem("Advanced");
        advanced.setMnemonic(KeyEvent.VK_A);  //FIXME:  SDM
        advanced.setActionCommand(ADVANCED);
        advanced.addActionListener(this);
        advanced.setBackground(Util.getBgColor());
        optionsMenu.add(advanced);


        optionsMenu.addSeparator();

        ButtonGroup group = new ButtonGroup();
        //String dateFormat = getDefaultDateFormat();

        stringDates = new JRadioButtonMenuItem("Upload String Dates (GMT)");
        stringDates.setMnemonic(KeyEvent.VK_S);  //FIXME:  SDM
        stringDates.setActionCommand(STRING_DATES);
        //stringDates.setSelected(dateFormat.equals(STRING_DATES));
        group.add(stringDates);
        stringDates.addActionListener(this);
//        optionsMenu.add(stringDates);

        stringDatesLocal = new JRadioButtonMenuItem("Upload String Dates");
        //stringDatesLocal.setMnemonic(KeyEvent.VK_S);  //FIXME:  SDM
        stringDatesLocal.setActionCommand(STRING_DATES_LOCAL);
        //stringDatesLocal.setSelected(dateFormat.equals(STRING_DATES_LOCAL));
        group.add(stringDatesLocal);
        stringDatesLocal.addActionListener(this);
        optionsMenu.add(stringDatesLocal);

        excelDates = new JRadioButtonMenuItem("Upload Excel Dates (GMT)");
        excelDates.setMnemonic(KeyEvent.VK_E);  //FIXME:  SDM
        excelDates.setActionCommand(EXCEL_DATES);
        //excelDates.setSelected(dateFormat.equals(EXCEL_DATES));
        group.add(excelDates);
        excelDates.addActionListener(this);
//        optionsMenu.add(excelDates);

        excelDatesLocal = new JRadioButtonMenuItem("Upload Excel Dates");
        //excelDatesLocal.setMnemonic(KeyEvent.VK_E);  //FIXME:  SDM
        excelDatesLocal.setActionCommand(EXCEL_DATES_LOCAL);
        //excelDatesLocal.setSelected(dateFormat.equals(EXCEL_DATES_LOCAL));
        group.add(excelDatesLocal);
        excelDatesLocal.addActionListener(this);
        optionsMenu.add(excelDatesLocal);

        unixDates = new JRadioButtonMenuItem("Upload Unix Time Stamps (GMT)");
        unixDates.setMnemonic(KeyEvent.VK_U);  //FIXME:  SDM
        unixDates.setActionCommand(UNIX_DATES);
        //unixDates.setSelected(dateFormat.equals(UNIX_DATES));
        group.add(unixDates);
        unixDates.addActionListener(this);
        optionsMenu.add(unixDates);

        optionsMenu.addSeparator();

        ButtonGroup nameGroup = new ButtonGroup();

        menuItem = new JRadioButtonMenuItem("Set File Name for Uploads...");
        menuItem.setMnemonic(KeyEvent.VK_N);
        menuItem.setActionCommand(UPLOAD_FILE_NAME);
        nameGroup.add(menuItem);
        menuItem.addActionListener(this);
        //menuItem.setBackground(Util.getBgColor());
        optionsMenu.add(menuItem);

        m_autoNameMenuItem = new JRadioButtonMenuItem("Use Auto File Name for Uploads");
        m_autoNameMenuItem.setMnemonic(KeyEvent.VK_F);
        m_autoNameMenuItem.setActionCommand(UPLOAD_FILE_NAME_AUTO);
        nameGroup.add(m_autoNameMenuItem);
        m_autoNameMenuItem.addActionListener(this);
        //m_autoNameMenuItem.setBackground(Util.getBgColor());
        optionsMenu.add(m_autoNameMenuItem);
        m_autoNameMenuItem.setSelected(true);

        //  Help menu
        JMenu helpMenu = new JMenu("Help");
        menuBar.add(helpMenu);
        helpMenu.setMnemonic(KeyEvent.VK_H);

        JMenuItem aboutCryo = new JMenuItem("About CryoBay...");
        aboutCryo.setMnemonic(KeyEvent.VK_C);
        aboutCryo.setActionCommand(ABOUT_CRYO);
        aboutCryo.addActionListener(this);
        aboutCryo.setBackground(Util.getBgColor());
        helpMenu.add(aboutCryo);

        JMenuItem aboutComp = new JMenuItem("Service Counter...");
        aboutComp.setMnemonic(KeyEvent.VK_S);
        aboutComp.setActionCommand(ABOUT_COMP);
        aboutComp.addActionListener(this);
        aboutComp.setBackground(Util.getBgColor());
        helpMenu.add(aboutComp);

        helpMenu.addSeparator();

        JMenuItem onlineHelp = new JMenuItem("Online Help...");
        onlineHelp.setMnemonic(KeyEvent.VK_H);
        onlineHelp.setActionCommand(HELP);
        onlineHelp.addActionListener(this);
        onlineHelp.setBackground(Util.getBgColor());
        helpMenu.add(onlineHelp);

        //Util.setAllBackgrounds(menuItem, Util.getBgColor());
    }

    /**
     * Add menu items to a given menu for all the files in a given directory.
     * @param baseMenu The menu to which the items are added.
     * @param baseName The directory path prepended to file names.
     * @param baseDir The directory searched for files. If not absolute, looks
     * relative to both the sytem and user directories.
     */
    private void addFileItems(JMenu baseMenu, String baseName, String baseDir) {
        // baseName is like "" or "pfizer/examples"
        // baseDir is like "cryo/tempcontroller" or "cryo/cryobay"
        if (baseName.length() > 0) {
            baseName = baseName + File.separator;
        }
        baseMenu.removeAll();

        String[] dirs = FileUtil.getAllVnmrDirs(baseDir);
        if (baseDir.length() > 0) {
            baseDir = baseDir + File.separator;
        }

        // First, add subdirectories to baseMenu
        SortedSet<String> subdirs = new TreeSet<String>(); // List of dir names
        for (String dir : dirs) {
            File[] dirfiles = new File(dir).listFiles(new DirectoryFilter());
            for (File subdir : dirfiles) {
                subdirs.add(subdir.getName());
            }
        }
        for (String subdir : subdirs) {
            JMenu submenu = new JMenu(subdir);
            submenu.setBackground(Util.getBgColor());
            baseMenu.add(submenu);
            addFileItems(submenu, baseName + subdir, baseDir + subdir);
        }

        // Next, add regular files to baseMenu (with actions)
        SortedSet<String> fnames = new TreeSet<String>(); // List of file names
        for (String dir : dirs) {
            File[] files = new File(dir).listFiles(new PlainFileFilter());
            for (File file : files) {
                fnames.add(file.getName());
            }
        }
        for (String file : fnames) {
            JMenuItem item = new JMenuItem(file);
            //m_cryoMsg.postDebug("cryocmd", "CryoDiag resolving path: " + baseDir + file);
            item.setActionCommand(baseMenu.getActionCommand() + " "
                        + resolveConfigReadPath(baseDir, file));

            //for FW_CONFIG and PROBE_CONFIG
            item.addActionListener(this);
            item.setBackground(Util.getBgColor());
            baseMenu.add(item);
        }
    }

    /**
     * ActionListener interface. 
     * @param ae The action to perform.
     */
    public void actionPerformed(ActionEvent ae) {

        String cmd = ae.getActionCommand();
        m_cryoMsg.postDebug("cryocmd", "CryoDiagnostics.actionPerformed: " + cmd);

        if (cmd.equals(CLOSE_COMMAND)) {
            closeAction();
        } else if (cmd.equals(START)) {
            m_cryoMsg.postDebug("cryocmd", "Starting Cold Probe");
            m_cryobay.sendToCryoBay("<READY>", PRIORITY1);
            writeAdvanced("Sending: <READY>\n");
        } else if (cmd.equals(STOP)) {
            m_cryoMsg.postDebug("cryocmd", "Stopping Cold Probe");
            //m_cryobay.sendToCryoBay("<STOP>");
            m_cryobay.stopSystem();
        } else if (cmd.equals(DETACH)) {
            m_cryoMsg.postDebug("cryocmd", "Detaching Probe");
            m_cryobay.sendToCryoBay("<REMOVE>", PRIORITY1);
            writeAdvanced("Sending: <REMOVE>\n");
        } else if (cmd.equals(VACPURGE)) {
            m_cryoMsg.postDebug("cryocmd", "Vacuum Purging");
            m_cryobay.sendToCryoBay("<V_PURGE>", PRIORITY1);
            writeAdvanced("Sending: <V_PURGE>\n");
        } else if (cmd.equals(PROBEPUMP)) {
            m_cryoMsg.postDebug("cryocmd", "Pumping Probe");
            m_cryobay.sendToCryoBay("<V_PROBE>", PRIORITY1);
            writeAdvanced("Sending: <V_PROBE>\n");
        } else if (cmd.equals(THERMCYCLE)) {
            m_cryoMsg.postDebug("cryocmd", "Thermal Cycle");
            m_cryobay.checkThermCycle();
        } else if (cmd.equals(CRYO)) {
            m_cryoMsg.postDebug("cryocmd", "Send to Cryobay " + m_cryotext);
            m_cryotext = CryoUtils.canonicalizeCommand(m_cryotext);
            if (m_cryobay.isMessageAllowed(m_cryotext)) {
                m_cryobay.sendToCryoBay(m_cryotext, PRIORITY1);
                writeAdvanced("Sending: " + m_cryotext + "\n");
            }
        } else if (cmd.equals(TEMP)) {
                m_cryoMsg.postDebug("cryocmd", "Send to Temp " + m_temptext);
                 m_tempcontroller.sendToCryoBay(m_temptext, PRIORITY1);
                 m_tempcontroller.sendToCryoBay(";", PRIORITY1);
             writeAdvanced("Sent Temp: " + m_temptext + "\n");
        } else if (cmd.startsWith(FW_CONFIG)) {
                if(cmd.endsWith(".setup")){
                m_cryoMsg.postDebug("cryocmd", cmd);
                LoadCryocon(cmd, CRYOBAY);
                } else if(cmd.endsWith(".txt")){
                        if(m_cryobay.getState().startsWith("IDLE")){
                            m_cryoMsg.postDebug("cryocmd", cmd);
                    LoadCryocon(cmd, TEMPCTRL);
                        } else {
                        m_cryobay.popupMsg("Temp Controller", "Cannot load calibration while active", false);
                }
                }
        } else if (cmd.startsWith(CRYO_CONFIG)) {
                m_cryoMsg.postDebug("cryocmd", cmd);
                LoadCryocon(cmd, CRYOBAY);
        } else if (cmd.startsWith(TC_CONFIG)) {
            m_cryoMsg.postDebug("cryocmd", cmd);
            if(m_cryobay.getState().startsWith("IDLE")){
                LoadCryocon(cmd, TEMPCTRL);
            } else {
                m_cryobay.popupMsg("Temp Controller", "Cannot load calibration while active", false);
            }
        } else if (cmd.startsWith(PROBE_CONFIG)) {
            m_cryoMsg.postDebug("cryocmd", cmd);
            if(m_cryobay.getState().startsWith("IDLE")){
                LoadCryocon(cmd, TEMPCTRL);
            } else {
                m_cryobay.popupMsg("Temp Controller", "Cannot load calibration while active", false);
            }
        } else if (cmd.startsWith("cryoText")) {
            VObjIF vobj = (VObjIF)ae.getSource();
            String attr= vobj.getAttribute(VObjDef.PANEL_PARAM);
            m_cryoMsg.postDebug("cryocmd", "Panel Param: "+attr);
            attr= vobj.getAttribute(VObjDef.VALUE);
            m_cryoMsg.postDebug("cryocmd", "Value: "+attr);
            m_cryotext= attr;
        }  else if (cmd.startsWith("tempText")) {
            VObjIF vobj = (VObjIF)ae.getSource();
            String attr= vobj.getAttribute(VObjDef.PANEL_PARAM);
            m_cryoMsg.postDebug("cryocmd", "Panel Param: "+attr);
            attr= vobj.getAttribute(VObjDef.VALUE);
            m_cryoMsg.postDebug("cryocmd", "Value: "+attr);
            m_temptext= attr+"\n";
        } else if (cmd.startsWith(ABOUT_CRYO)) {
            m_cryobay.popupFirmwareVersion();
        } else if (cmd.startsWith(ABOUT_COMP)) {
                m_cryobay.sendToCryoBay("<GM_HOURS>", PRIORITY1);
            writeAdvanced("Sending: <GM_HOURS>\n");
        } else if (cmd.startsWith(HELP)) {
                onlineHelp();
        } else if (cmd.startsWith(ADVANCED)) {
                setAdvanced(advanced.getState());
        } else if (cmd.equals(EXCEL_DATES)
                   || cmd.equals(EXCEL_DATES_LOCAL)
                   || cmd.equals(UNIX_DATES)
                   || cmd.equals(STRING_DATES_LOCAL)
                   || cmd.equals(STRING_DATES))
        {
            setDateFormat(cmd);
        } else if (cmd.equals(UPLOAD_FILE_NAME)) {
            showUploadPanel();
        } else if (cmd.equals(UPLOAD_FILE_NAME_AUTO)) {
            m_uploadFilePath  = null;
        }
    }

    private void showUploadPanel() {
        //In response to a button click:
        int rtn = m_uploadFileChooser.showOpenDialog(getFrame());
        if (rtn == JFileChooser.APPROVE_OPTION) {
            m_uploadFilePath = m_uploadFileChooser.getSelectedFile().getPath();
            if (m_dataport != null) {
                m_dataport.setUploadFilePath(m_uploadFilePath);
            }
        }
        if (m_uploadFilePath == null) {
            // Don't have a chosen name -- make auto-name button selected
            m_autoNameMenuItem.setSelected(true);
        }
        m_cryoMsg.postDebug(//"cryodata",
                            "Upload File Path = " + m_uploadFilePath);

    }

    /**
     * Given a file name, find the path to the file.
     * @param pathTail The path to the file, rooted in USER or SYSTEM directory.
     * @param name The name of the method to be read.
     * @return The full path of the file, or null if not found.
     */
    static public String resolveConfigReadPath(String pathTail, String name) {
        String filepath;

        filepath = FileUtil.openPath("USER" + pathTail + name);
        if (filepath == null) {
            filepath = FileUtil.openPath("SYSTEM" + pathTail + name);
        }
        return filepath;
    }

//    /**
//     * Send a status string to ExpPanel - as if from Infostat.
//     * General format is:
//     * <br>"Tag Value <units>"
//     * <br>E.g.: "lklvl 82.3"
//     * <br>or "spinval 0 Hz"
//     * @param msg Status string to send.
//     */
//    public void sendStatusMessage(String msg) {
//        ((ExpPanel)m_vif).processStatusData(msg);
//    }

    public void updateStatusMessage(String msg){
        ExpPanel.getStatusValue(msg);
        //m_cryoMsg.postDebug("cryocmd", "CryoDiagnostics.updateStatusMessage " + msg);
        m_varsPanel.updateStatusMessage(msg);
    }

    private void LoadCryocon(String cryoFile, CryoPort cryoPort)
    {
        String line = null;
        String token = null;
        StringTokenizer toker;
        BufferedReader in = null;

        toker = new java.util.StringTokenizer(cryoFile);
        toker.nextToken();              //throw out command
        if (toker.hasMoreTokens()) cryoFile= toker.nextToken();
        m_cryoMsg.postDebug("cryocmd", "Cryodiag, file: " + cryoFile);

        if (cryoPort == TEMPCTRL) {
            //m_cryoMsg.postDebug("cryocmd", "Starting temp thread");
                LoadCryoconT thread= new LoadCryoconT(500, cryoFile, this);
                thread.start();
        } else {

        try {
            try { in = new BufferedReader(new FileReader(cryoFile)); }
            catch (FileNotFoundException e) {
                m_cryoMsg.postError("Cryobay Config file not found: " + cryoFile);
                return;
            }

            // Read in from file
            while ((line = in.readLine()) != null) {
                StringTokenizer st = new java.util.StringTokenizer(line);
                if (!st.hasMoreTokens()) continue;
                else {
                        token = st.nextToken("\n");
                    //m_cryoMsg.postDebug("cryocmd", "Token: "+ token);
                }
                if (cryoPort == CRYOBAY){
                        m_cryobay.sendToCryoBay(token+"\n", Priority.PRIORITY2);
                    writeAdvanced("Sending: " + token + "\n");
                }
            }

        } catch (IOException ioExc) {
        }

        }
    }

    /**
     * This thread just sends status requests every so often.
     */
    class LoadCryoconT extends Thread {
        //private boolean m_quit = false;
        //private int m_rate_ms;
        String line=null, token=null, lineIn=null;
        String m_cryoFile=null;
        java.util.StringTokenizer toker, st = null;
        BufferedReader in = null;
        CryoDiagnostics m_cryoDiag= null;

        public LoadCryoconT(int rate_ms, String cryoFile, CryoDiagnostics cryoDiag) {
            setName("CryoSocketControl.StatusPoller");
            //m_rate_ms = rate_ms;
            m_cryoFile= cryoFile;
            m_cryoDiag= cryoDiag;
        }

        public synchronized void run() {

            try { in = new BufferedReader(new FileReader(m_cryoFile)); }
            catch (FileNotFoundException e) {
                m_cryoMsg.postError("Cryo Config file not found: " + m_cryoFile);
                return;
            }
            try{

            if(m_cryoFile.endsWith(".crv")){
                m_cryoDiag.writeAdvanced("Sent Temp: CALCUR 1\n");
                m_cryoDiag.m_tempcontroller.send("CALCUR 1\n");
            }

            // Read in from file
            while ((line = in.readLine()) != null) {
                st = new java.util.StringTokenizer(line);
                if (!st.hasMoreTokens()) continue;
                else {
                        token = st.nextToken("\n");
                    //m_cryoMsg.postDebug("cryocmd", "TempCont Token: "+ token);
                }

                //use direct send, don't want to wait for replies
                m_cryoDiag.m_tempcontroller.send(token+"\n");
                m_cryoDiag.writeAdvanced("Sent Temp: " + token + "\n");
                try{
                        //m_cryoMsg.postDebug("cryocmd", "Sleeping 500 msec");
                        Thread.sleep(500);
                } catch (Exception e){
                        m_cryoMsg.postError("Error while sending to Temp controller");
                }
            }
            m_cryoDiag.m_tempcontroller.send(";\n");

            } catch (IOException ioExc) {
            }
        }

    }


    /**
     * The Close button was clicked. Save changes and exit.
     */
    private void closeAction() {
        m_cryoMsg.postDebug("cryocmd", "closeAction");
        setVisible(false);
    }

    class DirectoryFilter implements FilenameFilter {
        public boolean accept(File dir, String name) {
            boolean bShow = true;
            File file = new File(dir, name);
            if (file.isHidden() || !file.isDirectory()) {
                bShow = false;
            }
            return bShow;
        }
    }

    class PlainFileFilter implements FilenameFilter {
        public boolean accept(File dir, String name) {
            boolean bShow = true;
            File file = new File(dir, name);
            if (file.isHidden() || file.isDirectory()) {
                bShow = false;
            }
            return bShow;
        }
    }

    public String getDateFormat() {
        return m_dateFormat;
    }

    public String getUploadFilePath() {
        return m_uploadFilePath;
    }

    public void setUploadCount(int i) {
        if (m_varsPanel != null) {
            m_varsPanel.setUploadCount(i);
        }
    }

}
