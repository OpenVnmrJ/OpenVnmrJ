// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

package vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.beans.*;
import java.io.*;
import java.net.*;
import java.util.*;

import javax.swing.*;
import javax.swing.plaf.basic.*;
import javax.swing.plaf.metal.*;

import sun.misc.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.shuf.*;


/**
 * The application Frame. Primary responsibilities are application
 * containment and layout management.
 *
 */
public class VNMRFrame extends JFrame implements AppInstaller {

    private static String m_persona;
    private static Dimension screenDim;
    private static JFrame m_splashFrame = null;
    private static SplashScreen splash = null;
    protected static int m_nclick = 0;

    /** current user-interface application */
    private AppIF appIF;
    /** content pane */
    private Container content;
    private MemoryMonitor memorymonitor;
    private boolean needWinId = true;
    private int windowId = 0;
    static UpdateAttributeList updateAttrThread=null;
    static UpdateDatabase updateThread=null;
    static FillMacrosTables fillMacrosThread=null;
    static FillWorkspaceTable fillWorkspaceThread=null;
    static NotifyListener notifyListener=null;
    static DBStatus dbStatus=null;
    static String hostAddr;
    static VNMRFrame vnmrFrame=null;
    static String m_title;
    static boolean exitingFlag=false;
    static boolean nativeGraphics = true;
    static Process    updateDBProc;
    static String host=null;    // TODO: not used?
    static String localhost=null; // TODO: not used?
    static MetalTheme theme = null;
    private static boolean useDatabase = true;
    private static boolean bAdmin = false;

    public static final String OCEAN_THEME_CLASS = "javax.swing.plaf.metal.OceanTheme";

    /**
     * entry point to VNMR application
     */
    public static void main(String[] args) {
        if (Util.OSNAME != null) {
           if (Util.OSNAME.startsWith("Sun") || Util.OSNAME.startsWith("Mac")) {
               if (!FileUtil.checkFileSystem()) {
                      System.exit(0);
               }
           }
        }

        String persona = getPersona();
        if ( ! m_persona.equalsIgnoreCase("hideNoSplash")) {
           splash = SplashScreen.getSplashScreen();
           if (splash == null) {  // command line does not define splash
              ImageIcon background = Util.getImageIcon("Splash_VnmrJ.png");
              Font msgFont = new Font( "SansSerif", Font.PLAIN, 18);
              Color msgColor = Color.WHITE;
              m_splashFrame = Splash.getFrame(background, persona,
                                        msgFont, msgColor, false);
           }
        }

        SwingUtilities.invokeLater(new Runnable() {
           public void run() {
               buildWindows();
           }
        });
    }

    private static String  getPersona() {
        String persona = "";
        m_title = "OpenVnmrJ";
        m_persona = System.getProperty("persona");
        if (m_persona == null) {
            m_persona = "";
        }
        if (m_persona.equalsIgnoreCase("adm")) {
            m_title += " ";
            m_title += Util.getLabel("_ui_Admin","Admin");
            persona = Util.getLabel("_ui_Administrator","Administrator");
            bAdmin = true;
        }
        if (Util.isPart11Sys()) {
            m_title += " SE";
            persona = "SE";
        }
        return persona;
    }

    private static void  initDb() {
        // Not for adm
    	boolean bAdm = (m_persona.equalsIgnoreCase("adm"))
                                ? true : false;
    	if (bAdm)
            return;
        if (!FillDBManager.locatorOff()) {
            if (notifyListener == null) {
                notifyListener = new NotifyListener();
                notifyListener.setPriority(Thread.MIN_PRIORITY);
                notifyListener.setName("Notify/Listen for Database updates");
                notifyListener.start();
            }
        }
            // for admin interface, don't need to start these threads
        String noUpdateForDebug = System.getProperty("noUpdateForDebug");
        if (noUpdateForDebug == null) {
             if (updateAttrThread == null) {
                updateAttrThread = new UpdateAttributeList();
                updateAttrThread.setPriority(Thread.MIN_PRIORITY);
                updateAttrThread.setName("Update DB Attr List");
                updateAttrThread.start();
             }
             if (fillMacrosThread == null) {
                fillMacrosThread = new FillMacrosTables();
                fillMacrosThread.setPriority(Thread.MIN_PRIORITY);
                fillMacrosThread.setName("Update Macros Tables");
                fillMacrosThread.start();
             }
             if (fillWorkspaceThread == null) {
                fillWorkspaceThread = new FillWorkspaceTable();
                fillWorkspaceThread.setPriority(Thread.MIN_PRIORITY);
                fillWorkspaceThread.setName("Update Workspace Table");
                fillWorkspaceThread.start();
             }
             if (updateThread == null) {
                updateThread = new UpdateDatabase();
                updateThread.setPriority(Thread.MIN_PRIORITY);
                updateThread.setName("Update DB");
                updateThread.start();
             }
             if (dbStatus == null) {
                dbStatus = new DBStatus();
                dbStatus.setPriority(Thread.MIN_PRIORITY);
                dbStatus.setName("DBStatus");
                dbStatus.start();
             }

              // Read the tag persistence file.  This needs to be done
              // before the statement definition menus are created.
             TagList.readPersistence();
        }
    }

    private static void showLoginBox() {
        AppIF app = Util.getAppIF();
        if (app == null)
            return;
        if (app instanceof VAdminIF)
            ((VAdminIF)app).showLoginBox();
    }

    private static void buildWindows() {
        // If we are not the the EventDispatchThread,
        // we just queue up a call to this method and return.
        
        if (!SwingUtilities.isEventDispatchThread()) {
            Runnable buildWindowsEvent = new Runnable() {
                public void run() {
                    buildWindows();
                }
            };
            SwingUtilities.invokeLater(buildWindowsEvent);
            return;
        }
        // We are in the EventDisplatch Thread

        // Catch all otherwise uncaught exceptions
        try {
            String user = System.getProperty("user.name");

            Messages.postMessage(Messages.OTHER|Messages.INFO|Messages.LOG,
                              "**********************************************");
            Messages.postMessage(Messages.OTHER|Messages.INFO|Messages.LOG,
                                 "********** Starting "+m_title+ " as " + user +
                                 " **********");
            try {
                File file = new File(FileUtil.sysdir() + "/vnmrrev");
                FileReader fr = new FileReader(file);
                BufferedReader in = new BufferedReader(fr);
                String line;

                while((line = in.readLine()) != null) {
                    Messages.postMessage(Messages.OTHER|Messages.INFO|
                                         Messages.LOG, "** " + line + " **");
                }
                in.close();
            }
            catch (Exception e) {}
            Messages.postMessage(Messages.OTHER|Messages.INFO|Messages.LOG,
                              "**********************************************");
            try
            {
                String strfile = System.getProperty("vjerrfile");
                if (strfile != null && strfile.equals("custom"))
                {
                	String strpath;
                	if (!UtilB.iswindows()) {
                        strpath = FileUtil.openPath(FileUtil.usrdir()+"/"+
                                                           Messages.getLogFileName());
                	} else {
                        strpath = FileUtil.openPath(FileUtil.usrdir()+"\\"+
                                                           Messages.getLogFileName());
                	}
                    if (strpath != null)
                        System.setErr(new PrintStream(new FileOutputStream(strpath, true)));
                }
            }
            catch (Exception e) {}


            // If accessing a remote DB, do not start the DB update.  This is
            // for setting up a remote machine to access a spectrometer
            // and its DB.  The files will be on the spectrometer and not
            // the remote machine.  When we go to a full fledged networked
            // DB, we will need to have this update active for that.

            host = System.getProperty("dbhost");
            // Get the local host name
            try{
                InetAddress inetAddress = InetAddress.getLocalHost();
                localhost = inetAddress.getHostName();
            }
            catch(Exception e) {
                Messages.writeStackTrace(e);
            }

            vnmrFrame = new VNMRFrame();
            LinuxFocusBug.workAroundFocusBugInWindow(vnmrFrame);
            // vnmrFrame.installApp();

            if (VNMRFrame.usesDatabase()) {
                initDb();
            }
            
            // Call the macro to update this operator's 
            // ExperimentSelector_operatorName.xml file
            // from the protocols themselves.  This macro will
            // cause an update of the ES when it is finished
            Util.getAppIF().sendToVnmr("updateExpSelector");

            if (m_persona.equalsIgnoreCase("hide") ||
                m_persona.equalsIgnoreCase("hideNoSplash")) {
               vnmrFrame.setVisible(false);
            } else {
               vnmrFrame.setVisible(true);
            }
            if (m_persona.equalsIgnoreCase("icon")) {
                  vnmrFrame.setState(Frame.ICONIFIED);
            }
            if (splash != null)
                splash.close();
            if (m_splashFrame != null)
                m_splashFrame.dispose();

            if (m_persona.equalsIgnoreCase("adm")) {
               SwingUtilities.invokeLater(new Runnable() {
                    public void run() {
                        showLoginBox();
                    }
                });
            } else {
               String cmd = System.getProperty("cmd");
               if (cmd != null) {
	          if(cmd.length()>0)
                     Util.getAppIF().sendToVnmr(cmd);
               }
            }
        }
        catch (Exception e) {
            Messages.postError(e + " occured in VNMRFrame.buildWindows");
            Messages.writeStackTrace(e);
        }
    }

    /**************************************************
     * Refresh all visual componants following a global UI change.
     **************************************************/
    public void updateComponentTreeUI(){
        // setBackground(Util.getBgColor());
        // content.setBackground(Util.getSeparatorBg());
        SwingUtilities.updateComponentTreeUI(content);
   }
    
    /**************************************************
     * Return the Main VNMR frame for this whole application.
     *
     **************************************************/
    public static VNMRFrame getVNMRFrame() {
        return vnmrFrame;
    }


    /**
     * constructor
     */
    public VNMRFrame() {
        // Set vnmrFrame here so that it is available to build stuff
        // within this constructor.
        vnmrFrame = this;

        content = getContentPane();
        setTitle(m_title);
        Image img = null;
        if (m_title.indexOf("Admin") < 0)
           img = Util.getImage("vnmrj.gif");
        else
           img = Util.getImage("vnmrjadmin.gif");
        if (img != null)
            this.setIconImage(img);
        Util.setMainFrame(this);
        content.setLayout(new BorderLayout());
        Toolkit tk = Toolkit.getDefaultToolkit();
        screenDim = tk.getScreenSize();
        setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
        
        addWindowListener (new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                if (exitingFlag)
                    return;
                RightsList rl = new RightsList(false);
                if(rl.approveIsFalse("canexitvnmrj")) {
                    Messages.postWarning("This operator does not have"
                            + " the right to exit VnmrJ. Aborting Exit.");
                    return;
                }
                if (appIF != null && !(appIF instanceof VAdminIF))
                    appIF.windowClosing();
                else
                    exitAll();

                // This operator is allowed to exit, so proceed
                /***
                if (appIF != null && m_nclick < 1 &&
                    !(appIF instanceof VAdminIF))
                    appIF.sendCmdToVnmr("exit");
                else
                    exitAll();
                m_nclick = m_nclick+1;
                ***/
            }
            public void windowOpened(WindowEvent e) {
                getWindowId();
                if (appIF != null && (appIF instanceof VnmrjIF)) {
                    ((VnmrjIF)appIF).frameEventProcess(e);
                }
            }

            public void windowDeiconified(WindowEvent e) {
                if (appIF != null && appIF.topToolBar != null) {
                    JComponent vpComp = appIF.topToolBar.getVPToolBar();
                    //System.out.println("vnmrj showing");
                    if (vpComp != null && (vpComp instanceof JToolBar)) {
                        JToolBar vpToolbar = (JToolBar)vpComp;
                        if (((BasicToolBarUI)vpToolbar.getUI()).isFloating()) {
                            Container container = vpToolbar.getParent();
                            showVpToolbar(container);
                            container.setVisible(true);
                        }
                    }
                    if (appIF instanceof VnmrjIF) {
                        ((VnmrjIF)appIF).frameEventProcess(e);
                    }
                }
            }

            public void windowIconified(WindowEvent e) {
                if (appIF != null && (appIF instanceof VnmrjIF)) {
                    ((VnmrjIF)appIF).frameEventProcess(e);
                }
            }
        });

        String usedb = System.getProperty("usedb");
        if (usedb != null) {
        	if (usedb.equalsIgnoreCase("false") || usedb.equalsIgnoreCase("no")) {
        		useDatabase = false;
        	} else {
        		useDatabase = true;
        	}
        }

        LoginService loginService = LoginService.getDefault();
        String user = System.getProperty("user.name");
        User usr=loginService.getUser(user);
        if (usr == null) {
            String mess = "Cannot proceed until the user "+user+" is defined in \n" +
                          " the /vnmr/adm/users/userlist file.  \n" +
                          " Do this by running 'vnmrj adm' as the VnmrJ administrator.\n" +
                          " Exiting!";
            JOptionPane.showMessageDialog(null,
                   mess,
                   "VnmrJ",
                   JOptionPane.ERROR_MESSAGE);
            System.exit(0);
        }
        initAppDirs(usr);
        FileUtil.setAppTypes(usr.getAppTypeExts());
      
        setUpDefaultAttr();
        DisplayOptions.initTheme(bAdmin);
        // content.setBackground(Util.getSeparatorBg());

        installApp(usr);
 
        String showMem = System.getProperty("showMem");
        if(DebugOutput.isSetFor("showmem") ||
                       (showMem != null && showMem.equals("yes"))) {
            memorymonitor = new MemoryMonitor();
            memorymonitor.setVisible(true);
        }
        
        Signal.handle(new Signal("TERM"), new SignalHandler() {
             public void handle(Signal sig) {
                exitAll();
             }
        });

        DisplayOptions.updateUIColor();
        
     }
    public void installApp() {
        LoginService loginService = LoginService.getDefault();
        String user = System.getProperty("user.name");
        User usr=loginService.getUser(user);
        initAppDirs(usr);
        FileUtil.setAppTypes(usr.getAppTypeExts());
        installApp(usr);
    }

    /**
     * Install application for the given user.
     * @param user user
     */
    public void installApp(User user) {
        // cleanup first -- save settings

        if (user == null) {
            Messages.postError("Cannot proceed until the user is defined in " +
                             "\n   the /vnmr/adm/users/userlist file.  " +
                             "Do this by running \n    'vnmrj adm' as the " +
                             "VnmrJ administrator." +
                             "\nExiting!");
            exitAll();
        }
        if (appIF != null) {
            SessionShare sshare = appIF.getSessionShare();
            if (sshare.user().getAccountName().equals (user.getAccountName()))
            {
                return;
            }
        }
        doLogout();
        System.gc();
        if (memorymonitor != null)
            memorymonitor.repaint();

        // now setup application

        //Plotters.getDeviceLists();

        //String appType = user.getAppType();
        ArrayList appTypes = user.getAppTypes();
        if(m_persona.equalsIgnoreCase("adm")) {
            //if (appTypes.contains(Global.ADMINIF)) {
                Util.setAdminIf(true);
                appIF = new VAdminIF(this, user);
                Util.setAppIF(appIF);
        }
        else {
            // appIF = new ExperimentIF(this, user);
            Util.setAdminIf(false);
            appIF = new VnmrjUI(this, user);
            Util.setAppIF(appIF);
        }

        // set frame size
        // appIF.initLayout(this);
        content.removeAll();
        content.add(appIF, BorderLayout.CENTER);
        appIF.initLayout(this);
        validate();
    } // installApp()

    public void rebuildApp() {
        AppIF oldAppIF = appIF;
        AppIF newAppIF;

        if(m_persona.equalsIgnoreCase("adm")) {
             return; // not support yet
        }
        LoginService loginService = LoginService.getDefault();
        String userName = System.getProperty("user.name");
        User usr = loginService.getUser(userName);
        // FileUtil.setAppTypes(usr.getAppTypeExts());
        if (appIF != null) {
            appIF.closeUI();
            newAppIF = new VnmrjUI(this, usr, true);
        }
        else
            newAppIF = new VnmrjUI(this, usr, false);
        appIF = newAppIF;
        content.removeAll();
        content.add(newAppIF, BorderLayout.CENTER);
        validate();
    }
    
    public void showVpToolbar(Container container) {
        if (container instanceof Frame)
            ((Frame)container).toFront();
        else if (container instanceof Window)
            ((Window)container).toFront();
        else if (container instanceof Dialog)
            ((Dialog)container).toFront();
        else if (container instanceof JComponent)
            showVpToolbar(container.getParent());
    }

    public void exitAll() {
        // Flag for anyone wanting to know if we are in the process of
        // shutting down.
        exitingFlag = true;

        if (updateAttrThread != null)
            updateAttrThread.interrupt();
        if (updateThread != null)
            updateThread.interrupt();
        if(fillWorkspaceThread != null)
            fillWorkspaceThread.interrupt();
        if(fillMacrosThread != null)
            fillMacrosThread.interrupt();
        if(dbStatus != null)
            dbStatus.interrupt();
        if(notifyListener != null)
            notifyListener.interrupt();

        // Give the threads some time to get themselves stopped.
        try {
            Thread.sleep(100);
        }
        catch (InterruptedException ie) {}

        doLogout();
        System.exit(0);
    }

    static public boolean exiting() {
        return exitingFlag;
    }


    protected void abort(User user, String strIType)
    {
        Util.error(user.getAccountName() + " is not authorized to run the " +
                    strIType + " interface.");
        System.err.println("Exiting!");
        System.exit(1);
    }

    /**
     * logout
     */
    // private synchronized void doLogout() {
    private void doLogout() {
        if (appIF != null) {
	    Util.saveLabelLists();
            SessionShare sshare = appIF.getSessionShare();
            if (sshare != null) {
            sshare.userInfo().put("vnmrFrame.size", getSize());
            sshare.userInfo().put("vnmrFrame.location", getLocation());
            }
            boolean adm = (m_persona.equalsIgnoreCase("adm"))
                    ? true : false;

            // Skip some things if adm
            if(!adm) {
                // Save Holding table info.
                HoldingDataModel holdingDataModel = sshare.holdingDataModel();
                if (holdingDataModel != null)
                    holdingDataModel.writePersistence();
                LocatorHistoryList lhl = sshare.getLocatorHistoryList();
                if (lhl != null)
                    lhl.writePersistence();

                // Save current tags table.
                TagList.writePersistence();

                // Save the LocAttrList for faster startup
                if (VNMRFrame.usesDatabase()) {
                    ShufDBManager dbManager = ShufDBManager.getdbManager();
                    FillDBManager.attrList.writePersistence();

                    // Close the DB daemon connection
                    dbManager.closeDBConnection();
                }

                // Write persistence for browser directory
                LocatorBrowser.writePersistence();

                // Write persistence for Open panel
                VJOpenPopup.writePersistence();

                // Write persistence for Save panel
                VJSavePopup.writePersistence();

                // Write persistence for Browser panel
                ExpPanel.writeBrowserPersistence();
                
                // Write persistence for Login panel
                LoginBox.writePersistence();
                
                ExpSelTree.writePersistence();
            }

            // Delete the tooltip persistence file.  This file is used
            // during execution, but is not really for persistence.
            VTipManager.deletePersistence();

            // Put notifylogout and removeAppIF at bottom.  If any code
            // after these tries to output an error to the error box,
            // it will hang/crash.
            appIF.notifyLogout();
            if (sshare != null)
               sshare.notifyLogout();
            Util.removeAppIF();
            appIF = null;

        }
    } // doLogout()

    static {
        nativeGraphics = Util.isNativeGraphics();
        if (nativeGraphics)
              System.loadLibrary("vnmrj");
    }

   public native int getXwinId(JFrame f);

   public native int syncXwin(JFrame f);

   public void getWindowId() {
        if (!needWinId)
           return;
        if (!nativeGraphics)
           return;
        needWinId = false;
/*
        ComponentPeer peer = getPeer();
        if (peer == null)
            return;
        DrawingSurface surf = (DrawingSurface) peer;
        DrawingSurfaceInfo info = surf.getDrawingSurfaceInfo();
        Class d_class = info.getClass();
        Method getHandle = null;
        try {
                getHandle = d_class.getMethod("getDrawable", null);
        }
        catch (NoSuchMethodException e) { }
        catch (SecurityException e) { }
        if (getHandle == null)
            return;
        info.lock();

        Integer handle = null;
        try {
                handle = (Integer) getHandle.invoke(info, null);
        }
        catch (InvocationTargetException e) {}
        catch (IllegalAccessException e) {}
        info.unlock();
        if (handle == null) {
                return;
        }
        windowId = handle.intValue();
*/
        windowId = getXwinId(this);
        Util.setWindowId(windowId);
    }

    public int syncX() {
        if (!nativeGraphics)
            return 0;
        return syncXwin(this);
    }

    public static Font getDefinedFont(String pname) {
        if (pname == null)
            return null;
        String def = System.getProperty(pname);
        if (def == null)
            return null;
        StringTokenizer tok = new StringTokenizer(def, " ,\n");
        if (!tok.hasMoreTokens())
            return null;
        String  name = tok.nextToken();
        int  style = Font.PLAIN;
        int  s = 12;
        if (tok.hasMoreTokens()) {
            def = tok.nextToken().toLowerCase();
            if (def.equals("bold"))
                style = Font.BOLD;
            else if (def.equals("italic"))
                style = Font.ITALIC;
        }
        try {
            if (tok.hasMoreTokens()) {
                s = Integer.parseInt(tok.nextToken());
            }
        }
        catch (Exception e) {}
        
        return (new Font(name, style, s));
    }

    public static void setUpDefaultAttr() {
        UIManager.put("ClockDialUI", "vnmr.ui.BasicClockDialUI");
        UIManager.put("UpDownButtonUI", "vnmr.ui.MotifUpDownButtonUI");

        // UIManager.put("TabbedPane.background", Color.lightGray);
        if (Util.isMacOs()) {
            Util.setActiveBg(new Color(0,0,128));
            Util.setActiveFg(Color.white);
            Util.setInactiveBg(Color.gray);
            Util.setInactiveFg(Color.black);
        }
        Font ft = getDefinedFont("canvasFont");
        if (ft != null)
            UIManager.put("Canvas.font", ft);

        ft = getDefinedFont("font");
        if (ft == null)
            return;
        UIManager.put("VJ.font", ft);
        /*************************************
        UIManager.put("Text.font", ft);
        UIManager.put("TextField.font", ft);
        UIManager.put("TextPane.font", ft);
        UIManager.put("TextArea.font", ft);
        UIManager.put("ToolTip.font", ft);

        UIManager.put("Table.font", ft);
        UIManager.put("ToggleButton.font", ft);
        UIManager.put("ToolTip.font", ft);
        UIManager.put("Button.font", ft);
        UIManager.put("ComboBox.font", ft);
        UIManager.put("Label.font", ft);
        UIManager.put("List.font", ft);
        UIManager.put("Menu.font", ft);
        UIManager.put("MenuItem.font", ft);
        UIManager.put("PopupMenu.font", ft);
        UIManager.put("RadioButton.font", ft);
        UIManager.put("JMenuItem.font", ft);
        UIManager.put("JMenu.font", ft);
        UIManager.put("JButton.font", ft);
        UIManager.put("JComboBox.font", ft);
        UIManager.put("JLabel.font", ft);
        UIManager.put("JList.font", ft);
        UIManager.put("JToggleButton.font", ft);
        UIManager.put("Panel.font", ft);
        UIManager.put("OptionPane.font", ft);
        *************************************/
    }


    public void setWaitCursor() {
        setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
    }

    public void setDefaultCursor() {
        setCursor(Cursor.getDefaultCursor());
    }
    
    // not sure if vnmrFrame (instantiated object) is always available to
    // anyone who wants to know the result of this function, so making it
    // static -- should be ok -- note: if this is not right, the fault
    // lies in the pathological architecture of VNMRFrame
    public static boolean usesDatabase() {
    	return useDatabase;
    }

    private void initAppDirs(String path, ArrayList appDirectories, ArrayList appDirLabels) {
        // read persistence file
        // do nothing if does not exist.
        boolean sysdir = false;
        if (path == null)
            return;

        try {
            BufferedReader reader = new BufferedReader(new FileReader(path));

            String line;
            while ((line = reader.readLine()) != null) {
                if (line.startsWith("#") || line.startsWith("0"))
                    continue;

                StringTokenizer tok = new StringTokenizer(line, ";");
                if (tok.hasMoreTokens()
                        && tok.nextToken().trim().startsWith("0"))
                    continue;
                if (tok.hasMoreTokens()) {
                    String str = tok.nextToken().trim();
                    if (str.equalsIgnoreCase("USERDIR"))
                        str = FileUtil.usrdir();
                    if (str.equalsIgnoreCase( FileUtil.sysdir() ))
                        sysdir = true;
                    str=UtilB.addWindowsPathIfNeeded(str);
                    appDirectories.add(str);
                    if (tok.hasMoreTokens()) {
                        // Save the Labels also
                        String lstr = tok.nextToken().trim();
                        appDirLabels.add(lstr);
                    }
                    else {
                        // If no label for some reason, just use the path
                        appDirLabels.add(str);
                    }
                }
            }
            if (!sysdir)
            {
                appDirectories.add( FileUtil.sysdir() );
                appDirLabels.add( FileUtil.sysdir() );
            }
            reader.close();

        } catch (Exception e) {
        }
    }

    public static void appendStringToTitle(String str) {
	if(str.length()>0)
	  vnmrFrame.setTitle(m_title+"  ("+str+")");	
	else
	  vnmrFrame.setTitle(m_title);	
    }

    public void initAppDirs(User user) {

        if (user == null)
            return;
	ArrayList appDirectories = user.getAppDirectories();
	ArrayList appDirLabels = user.getAppLabels();

        if (appDirectories == null) {
            appDirectories = new ArrayList();
        } else
            appDirectories.clear();

        if (appDirLabels == null) {
            appDirLabels = new ArrayList();
        } else
            appDirLabels.clear();

        String userName = user.getAccountName();
        if (userName == null)
            return;

        // try persistence file first.
        String path = null;
        boolean bCurrUser = userName.equals(System.getProperty("user"));
        if (!bCurrUser) return;

	String operatorName = user.getCurrOperatorName();

        if (operatorName != null) {
//            RightsList rightsList =  new RightsList(operatorName, false);
//            if(!rightsList.approveIsFalse("caneditappdir"))
// Changed so that we are not trying to get the rights before the appdir
// is even set.  Just always look for the appdir_ file.  If the right is
// turned off, the user will not have a menu for editing the application
// directories.
                path = FileUtil.openPath("USER/PERSISTENCE/appdir_"
                    + operatorName);
        }
        if (path == null) {
//            RightsList rightsList =  new RightsList(userName, false);
//            if(!rightsList.approveIsFalse("caneditappdir"))
               path = FileUtil.openPath("USER/PERSISTENCE/appdir_" + userName);
        }
        
        if(path != null) initAppDirs(path, appDirectories, appDirLabels);

        if (appDirectories.size() < 1) {
            // persistence does not exist, use default appdirs
            String itype = user.getIType();
            if (itype != null && itype.equals(Global.IMGIF)) {
                path = FileUtil
                        .openPath("SYSTEM/USRS/userProfiles/appdirImaging.txt");
            } else if (itype != null && itype.equals(Global.WALKUPIF)) {
                path = FileUtil
                        .openPath("SYSTEM/USRS/userProfiles/appdirWalkup.txt");
            } else if (itype != null && itype.equals(Global.LCIF)) {
                path = FileUtil
                        .openPath("SYSTEM/USRS/userProfiles/appdirLcNmrMs.txt");
            } else {
                path = FileUtil
                        .openPath("SYSTEM/USRS/userProfiles/appdirWalkup.txt");
            }
            initAppDirs(path, appDirectories, appDirLabels);
        }
        FileUtil.setAppDirs(appDirectories, appDirLabels);
    }

} // class VNMRFrame

