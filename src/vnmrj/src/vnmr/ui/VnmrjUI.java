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
import java.io.*;
import java.util.*;
import java.beans.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.text.*;
import javax.swing.SwingConstants;
import javax.swing.Timer;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.shuf.FillDBManager;
import vnmr.ui.shuf.SpotterButton;
import vnmr.ui.shuf.ProtocolBrowser;
import vnmr.print.VjPrintUtil;

/**
 * This "experiment interface" is for users like Sylvia.
 *
 */
public class VnmrjUI extends AppIF implements VnmrjIF, VnmrKey, DockConstants, VnmrjUiNames, PropertyChangeListener {

    private boolean     bResizable = true;
    private boolean     isDraging = false;
    private boolean     bVpAction = false;
    private boolean     bLogout = false;
    private boolean     bSwitching = false;
    private boolean     isFirstClick = true;
    private boolean     bLayout = false;
    private boolean     bGToolMoving = false;
    private boolean     bDividerMoving = false;
    // flag used to set the height of controlPanel
    private boolean     resizeActive = true;
    private boolean     bCsiVisible = false;
    private int         commandH = 24;
    private int         commandRH;
    private int         commandY;
    private int         controlX = 0;
    private int         controlY = 0;
    private int         expY2;
    private int         handleH = 7; // the height of divider under command line
    private int         handleY = 0;
    private int         panelW = 0;
    private int         panelH = 0;
    private int         topMenuH = 20;
    private int         usrToolH = 24;
    private int         sysToolH = 24;
    private int         waitVp = 0; // the vp waiting for input
    private static int  LAYER0 = JLayeredPane.DEFAULT_LAYER.intValue();
    private static int  LAYER1 = LAYER0 + 10;
    private static int  LAYER2 = LAYER0 + 20;
    private static int  LAYER3 = LAYER0 + 30;
    private static int  LAYER4 = LAYER0 + 40;
    private Integer     defaultLayer = new Integer(LAYER0+5);
    private Integer     layer1 = new Integer(LAYER1);
    private Integer     layer2 = new Integer(LAYER2);
    private Integer     layer3 = new Integer(LAYER3);
    private Integer     layer4 = new Integer(LAYER4);
    private int         messageRH;
    private int         messageY;
    private int         prePos = 0;
    private int         py;
    private int         pyd;
    private int         statusY = 0;
    private int         titleBarH = 0;
    private int         topBarY2 = 0;
    private int         viewAreaH;
    private int         viewAreaW = 0;
    private int         viewAreaX = 0;
    private int         viewAreaY = 0;
    private int         vpId = 0;
    private int         dockImgWidth = 0;
    private int         dockImgHeight = 0;

    private AppInstaller    appInstaller;
    private AppIF        oldAappIF = null;
    private VnmrjUI      oldVjUI = null;
    private Point           ps;
    private Rectangle       frameRec;
    private Rectangle       viewRec;
    private Rectangle       viewRec2;
    private VOnOffButton    topToolBut, statusBut;
    private VOnOffButton    sysToolBut;
    protected boolean       m_bShowCmdArea = true;
    protected boolean       geoChanged = false;
    protected boolean       bChangeLayout = false;
    protected boolean       bNative = false; // Xwindow graphics
    protected boolean       bRebuild = false;
    protected boolean       bWaitInput = false;
    protected VjToolBar     usrToolBar;
    // If this is not static, then the macro command ends up overruled
    // when the UI is rebuilt.  static keeps the info intact
    static protected boolean       showCmdAreaViaMacro = true;

   /** layout info **/
    private int xmainMenu = 1;
    private int xtoolBar = 1;
    private int xsysToolBar = 1;
    private int xgToolBar = 1; // graphicsToolBar
    private int xinfoBar = 1;
    private int xlocatorTop = 1; // locatorOnTop
    private int xparamTop = 1; // panelOnTop
    private int sysToolAtTop = 1; // sysToolPan position above usrToolBar 
    private int usrToolInSysTool = 1;
    private int gToolDock = DOCK_EXP_RIGHT; // graphicsTool dock position
    private int csiToolDock = DOCK_EXP_LEFT; // csiTool dock position
    private int dockButtonNum;
    private float paramRx = 0;
    private float paramRy = 0;
    private RepaintManager orgRM = null;
    private DummyRepaintManager dummyRM = null;
   /** end of layout info **/

    private PushpinPanel  paramPinPan;
    private PushpinPanel  protocolPinPan;
    private SysToolPanel  sysToolPan;
    private DisplayPalette csiButtonPalette;
    protected static Hashtable<String, String>   m_htNameKey = new Hashtable<String, String>();
    private Hashtable<String, Object> hs = null;
    private GraphicsWinTool displayWinBar;
    private GraphicsToolIF graphicsToolBar = null;
    private SubButtonPanel buttonTool = null;
    private Vector<VFrameListener> frameEventListeners = new Vector<VFrameListener>();
    private Vector<Component> toolbarListeners = new Vector<Component>();
    private Vector<JSeparator> separatorList = new Vector<JSeparator>();

    private Action m_action;
    private Vector<String> m_vecStrNames = new Vector<String>();
    private Vector<String> m_vecStrKeys  = new Vector<String>();
    private Vector<String> m_vecStrCmd   = new Vector<String>();
    private DockButton[] dockButtonList;
    private DockButton dockedButton;
    private JPanel cmdResizeBar;
    private JPanel statusButPane;
    private JSplitPane cmdSplitBar;
    private PushpinControler  toolTabBar; // tab bar of vertical panel
    private PushpinControler  paramTabBar; // tab bar of parameter panel
    private RightsList  rightsLst = null;
    // private static DisplayOptions displayOptions = null;
    private ProtocolBrowser protocolBrowser = null;
    private HelpOverlay helpOverlayPanel = null;
    private ImageViewPanel imgPane;
    private Color dividerColor = Color.black;
    private PushpinIF dividerObj;

    AcceleratorKeyTable keyTable = null;

    /**
     * constructor
     * @param appInstaller application installer
     * @param user user
     */
    public VnmrjUI(AppInstaller appInstaller, User user, boolean bReBuild) {
        // super(appInstaller, user);
        super();
        this.appInstaller = appInstaller;
        this.bRebuild = bReBuild;
        this.oldVjUI = null; 
        if (bReBuild) {
            oldAappIF = Util.getAppIF();
            suspendUI();
            if (oldAappIF != null && (oldAappIF instanceof VnmrjUI)) {
                sshare = oldAappIF.sshare;
                oldVjUI = (VnmrjUI) oldAappIF;
            }
            else {
                oldAappIF = null;
                this.bRebuild = false;
                bReBuild = false;
            }
        }
        if (sshare == null) {
            sshare = new SessionShare(appInstaller, user);
        }
        if (FillDBManager.locatorOff())
            VNMRFrame.setNoDatabase();
        if (!bReBuild) {
            sshare.setAppInstaller(appInstaller);
            sshare.setUser(user);
            if (!FillDBManager.locatorOff())
               sshare.FillLocatorHistoryList();
        }
        Util.setAppIF(this);
        Util.setUser(user);
        if (!bReBuild)
            Util.setFocus();
        if (bReBuild)
            DisplayOptions.removeChangeListener(oldVjUI);
        DisplayOptions.addChangeListener(this);

        setLayout(new VnmrjUILayout());
        bNative = Util.isNativeGraphics();

        rightsLst = new RightsList(false);
        
        m_bShowCmdArea = true;
        if(rightsLst != null)
            m_bShowCmdArea = !rightsLst.approveIsFalse("enablecmdline");

        Util.shufflerInToolPanel(false);

        createTopTools();
        if (!bNative) {
           displayPalette = new DisplayPalette();
           add(displayPalette, layer3);
           graphicsToolBar = (GraphicsToolIF) displayPalette;
           displayPalette.setLocation(60, 160);
           Util.setGraphicsToolBar(graphicsToolBar);
           graphicsToolBar.setShow(true);
           displayPalette.setName(graphtoolname);
        }

        csiButtonPalette = new DisplayPalette();
        csiButtonPalette.setShowAble(false);
        csiButtonPalette.setShow(false);
        csiButtonPalette.setDockPosition(DOCK_EXP_LEFT);
        csiButtonPalette.setLocation(260, 260);
        csiButtonPalette.setName(csitoolname);
        csiButtonPalette.setDefaultOrientation(DockConstants.VERTICAL);
        add(csiButtonPalette, layer3);

        createMainPanels();

        statusBar = new ExpStatusBar(sshare,expViewArea.getDefaultExp());
        statusButPane = new JPanel(new BorderLayout());
        statusBut = new VOnOffButton(VOnOffButton.ICON, statusBar);
        statusBut.setName("HardwareTool");
        statusBut.setMoveable(false);
        statusButPane.add(statusBut, BorderLayout.CENTER);
        add(statusBar, defaultLayer);
        add(statusButPane, defaultLayer);
        Util.setExpStatusBar(statusBar);

        // to check if there is layout info from the last session
        Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
        if (Util.islinux())
           frameRec = new Rectangle(10, 10, dim.width, dim.height - 50);
        else if (Util.iswindows())
           frameRec = new Rectangle(10, 10, dim.width, dim.height - 40);
        else
           frameRec = new Rectangle(10, 10, dim.width, dim.height);
        initUiLayout();

        ExpPanel.addExpListener(statusBar); // register as a vnmrbg listener
        if (!Util.isMultiSq()) {
            QueuePanel qp = Util.getStudyQueue();
            if (qp != null)
               ExpPanel.addExpListener(qp);
        }
        sysToolPan.init();

        keyTable = AcceleratorKeyTable.getAcceleratorKeyTable();

        addKeyListener(new KeyAdapter() {
            public void keyPressed(KeyEvent e) {
                processKeyPressed(e);
            }
            public void keyReleased(KeyEvent e) {
                processKeyReleased(e);
            }
            public void keyTyped(KeyEvent e) {}
        });


        VMouseAdapter mouseadapter = new VMouseAdapter();
        Toolkit.getDefaultToolkit().addAWTEventListener(mouseadapter,
                                              AWTEvent.MOUSE_EVENT_MASK);

        setBgColor();
        if (bRebuild ) {
            addTopTools();
        }
        if (bReBuild && !FillDBManager.locatorOff()) {
            SpotterButton spotterButton = sshare.getSpotterButton();
            if (spotterButton != null) {
                spotterButton.updateMenu();
            }
        }

        KeyboardFocusManager.getCurrentKeyboardFocusManager().addKeyEventDispatcher(
            new KeyEventDispatcher() {
                public boolean dispatchKeyEvent(KeyEvent e) {
                    if (e.getID() == KeyEvent.KEY_RELEASED) {
                        int kcode = e.getKeyCode();
                        if (kcode == KeyEvent.VK_CONTROL || kcode == KeyEvent.VK_ESCAPE || 
                        	kcode == KeyEvent.VK_ALT) {
                            int mod = e.getModifiers();
                            if (mod == 0 && commandArea != null) {
                                if (commandArea.isVisible()) {
                                    setCommandLineFocus();
                                }
                            }
                        }
                    }

                    // If the key should not be dispatched to the
                    // focused component, return true
                    return false;
                }
            });

        if (usrToolInSysTool != 0)
            setUsrToolBarPosition(0);
        if (!Util.isAdminIf()) {
           if (ExpSelTree.getInstance() != null) {
               // ExpSelector is the master of ExpSelTree
               if (ExpSelector.getInstance() == null)
                   new ExpSelector(sshare);
           }
        }
        expViewArea.run();
        setPrinterQueryTimer(); 
        VPopupManager.init();
        sysToolPan.setPopupToolBar(VPopupManager.getPopupContainer());

    } // VnmrjUI()

    public VnmrjUI(AppInstaller appInstaller, User user) {
        this(appInstaller, user, false);
    }

    public void initLayout() {
        Container pp = getParent();
        while (pp != null) {
           if (pp instanceof JFrame) {
               JFrame frame = (JFrame) pp;
               initLayout(frame);
               break;
           }
           pp = pp.getParent();
        }
    }

    public void initLayout(JFrame f) {
        if (f == null)
            return;
        Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
        if (frameRec.width > dim.width)
            frameRec.width = dim.width - 10;
        if (frameRec.height > dim.height)
            frameRec.height = dim.height - 10;
        Point pt = new Point (frameRec.x, frameRec.y);
        if (frameRec.x + frameRec.width >  dim.width)
            pt.x = dim.width - frameRec.width;
        if (frameRec.y + frameRec.height >  dim.height)
            pt.y = dim.height - frameRec.height;
        f.setLocation(pt);
        f.setSize(new Dimension(frameRec.width, frameRec.height));
    }

    public void sendCmdToVnmr(String str) {
        messageArea.append(str);
        if (expViewArea != null)
           expViewArea.sendToVnmr(str);
    }

    public void sendToVnmr(String str) {
        if (expViewArea != null) {
           if (bWaitInput)
               expViewArea.sendToVnmr(waitVp+1, str);
           else
               expViewArea.sendToVnmr(str);
        }
        bWaitInput = false;
    }

    public void sendToAllVnmr(String str) {
        if (expViewArea != null)
           expViewArea.sendToAllVnmr(str);
    }

    public void exitVnmr() {
	sendToAllVnmr("exit('saveNotSharedGlobal')");
        appInstaller.exitAll();
    }

    public void windowClosing() {
        if (expViewArea != null)
            expViewArea.windowClosing();
        else
            appInstaller.exitAll();
    }

    public void setInputPrompt(int vpId, String m) {
        if (m == null || (commandArea == null))
            return;
        bWaitInput = true;
        waitVp = vpId;
        commandArea.setPrompt(m);
    }

    private void setCommandLineFocus() {
        VNMRFrame frame = VNMRFrame.getVNMRFrame();

        Container c = KeyboardFocusManager.getCurrentKeyboardFocusManager().getCurrentFocusCycleRoot();
        if (c != null) {
           if (c != frame) 
              return;
        }

        commandArea.setFocus();
    }

    public VjToolBar getToolBar()
    {
        return usrToolBar;
    }

    @Override
    public void setButtonBar(SubButtonPanel b) {
        buttonTool = b;
        if (graphicsToolBar != null)
             graphicsToolBar.setToolBar(b);
    }

    @Override
    public DisplayPalette getCsiButtonPalette() {
        if (csiButtonPalette.isVisible())
            return csiButtonPalette;
        return displayPalette;
    }

    @Override
    public void setCsiButtonBar(SubButtonPanel b) {
        if (csiButtonPalette.isVisible())
            csiButtonPalette.setToolBar(b);
        else
            displayPalette.setToolBar(b);
    }

    public void showCsiButtonPalette(boolean b) {
        boolean bVis = csiButtonPalette.isShow();
        bCsiVisible = b;
        if (bVis == b)
            return;
        bGToolMoving = true;
        if (!b) {
            saveToolBarDefaults((GraphicsToolIF)csiButtonPalette, csiToolKey);
            saveToolBarDefaults(graphicsToolBar, graph2ToolKey);
            setToolBarDefaults(graphicsToolBar, graphToolKey);
        }
        else {
            saveToolBarDefaults(graphicsToolBar, graphToolKey);
            setToolBarDefaults((GraphicsToolIF)csiButtonPalette,csiToolKey);
            setToolBarDefaults(graphicsToolBar, graph2ToolKey);
        }
        csiButtonPalette.setShow(b);
        csiButtonPalette.setShowAble(b);
        // updateToolbarListeners();
        bGToolMoving = false;
    }

    public void createGraphicsToolBar() {
        if (bNative) {
            displayWinBar = new GraphicsWinTool(VNMRFrame.getVNMRFrame());
            graphicsToolBar = (GraphicsToolIF) displayWinBar;
            Util.setGraphicsToolBar(graphicsToolBar);
            initGraphicsToolBar();
            graphicsToolBar.setToolBar(buttonTool);
            graphicsToolBar.resetSize();
            addFrameEventListener((Component) displayWinBar);
            //((Component)graphicsToolBar).setBackground(bgColor);
         }
    }

    public RightsList getRightsList() {
        return rightsLst;
    }

    public Rectangle getCommandLineLoc()
    {
        return commandArea.getBounds();
    }

    public boolean isFirstClick()
    {
        return isFirstClick;
    }

    public boolean isCmdLineOpen()
    {
        return  commandArea.isShowing();
    }

    // This method is now used to bring the cmdline to the proper state.
    // That is, it will show or unshow as per the right, enablecmdline
    // AND the showCmdAreaViaMacro param.
    public void sendCmdLineOpen()
    {
        
        if (isFirstClick()) {
            if(rightsLst != null)
                m_bShowCmdArea = !rightsLst.approveIsFalse("enablecmdline");
            if(m_bShowCmdArea && showCmdAreaViaMacro) {
               sendToAllVnmr("jFunc(29, 1)\n");
               commandArea.setVisible(true);
            }
            else {
                commandArea.setVisible(false);
            }
        }
    }

    public ParamIF syncQueryParam(String p) {
        if (expViewArea == null)
           return null;
        return expViewArea.syncQueryParam(p);
    }

    public ParamIF syncQueryVnmr(int type, String p) {
        if (expViewArea == null)
           return null;
        return expViewArea.syncQueryVnmr(type, p);
    }

    public ParamIF syncQueryExpr(String p) {
        if (expViewArea == null)
           return null;
        return expViewArea.syncQueryExpr(p);
    }

    public ParameterPanel getParamPanel(String name) {
        if (expViewArea == null)
           return null;
        return expViewArea.getParamPanel(name);
    }

    // public synchronized void notifyLogout() {
    public void notifyLogout() {
        if (bLogout)
            return;
        bLogout = true;
        saveUiLayout();
        vToolPanel.saveUiLayout();
        if (sysToolPan != null)
           sysToolPan.saveUiLayout();
        if (expViewArea != null)
           expViewArea.logout();
        expViewArea = null;

        commandArea.writePersistence();
    }

    public void closeUI() {
        saveUiLayout();
        vToolPanel.saveUiLayout();
        if (sysToolPan != null)
           sysToolPan.saveUiLayout();
        commandArea.writePersistence();
        DisplayOptions.removeChangeListener(this);
        ExpPanel.removeExpListener(statusBar);

    }

    public void suspendUI() {
         if (expViewArea != null)
            expViewArea.suspendUI();
    }

    public void resumeUI() {
         if (expViewArea != null)
            expViewArea.resumeUI();
    }

    private void getDefaultPrinterName() {
        new Thread(new Runnable()
        {
            public void run()
            {
                VjPrintUtil.getSaveDefaultPrinterName();
            }
        }).start();
    }

    private void setPrinterQueryTimer() {
         ActionListener queryPrinterAction = new ActionListener() {
              public void actionPerformed(ActionEvent ae) {
                     getDefaultPrinterName();
                }
          };
          Timer printerTimer = new Timer(10000, queryPrinterAction);
          printerTimer.setRepeats(false);
          printerTimer.restart();
    }

    private void saveToolBarDefaults(GraphicsToolIF bar, String name) {
        if (bar == null)
            return;
        if (bar.isShow())
            hs.put(name, "on");
        else
            hs.put(name, "off");
        Point pt = bar.getPosition();
        if (bar instanceof GraphicsWinTool) {
            if (isShowing()) {
                 Point pt2 = getLocationOnScreen();
                 pt.x = pt.x - pt2.x;
                 pt.y = pt.y - pt2.y;
              }
           }
        String key = name+"X";
        hs.put(key, new Integer(pt.x));
        key = name+"Y";
        hs.put(key, new Integer(pt.y));
        key = name+"Orient";
        hs.put(key, new Integer(bar.getDefaultOrientation()));
        key = name+"Dock";
        hs.put(key, new Integer(bar.getDockPosition()));
    }

    private void setToolBarDefaults(GraphicsToolIF bar, String name) {
        Integer ix;
        int x = 100;
        int y = 100;
        String key = name+"X";

        if (bar == null)
            return;
        ix = (Integer) hs.get(key);
        if (ix != null)
            x = ix.intValue();
        key = name+"Y";
        ix = (Integer) hs.get(key);
        if (ix != null)
            y = ix.intValue();
        if (bar instanceof GraphicsWinTool) {
            if (isShowing()) {
              Point pt = getLocationOnScreen();
              x = x + pt.x;
              y = y + pt.y;
            }
        }
        bar.setPosition(x, y);
        key = name+"Orient";
        ix = (Integer)hs.get(key);
        if (ix != null) {
            x = ix.intValue();
            bar.setDefaultOrientation(x);
        }
        key = name+"Dock";
        ix = (Integer)hs.get(key);
        if (ix != null) {
            x = ix.intValue();
            if (x >= DOCK_NONE && x < DOCK_NORTH_WEST)
                bar.setDockPosition(x);
        }

        String st = (String)hs.get(name);
        if (st != null)
        {
            boolean bVisible = st.equals("off") ? false : true;
            bar.setShow(bVisible);
        }
    }

    private void initGraphicsToolBar() {
        hs = sshare.userInfo();
        if (hs == null)
            return;
        if (graphicsToolBar == null)
           return;
        graphicsToolBar.setShow(true);
        setToolBarDefaults(graphicsToolBar, graphToolKey);
        gToolDock = graphicsToolBar.getDockPosition();
        if (graphicsToolBar.isShow())
           xgToolBar = 1;
        else
           xgToolBar = 0;

        csiButtonPalette.setShowAble(false);
        csiButtonPalette.setShow(true);
        setToolBarDefaults((GraphicsToolIF)csiButtonPalette, csiToolKey);
        csiToolDock = csiButtonPalette.getDockPosition();
    }

    public void initUiLayout() {
        if (sshare == null)
            return;
        hs = sshare.userInfo();
        if (hs == null)
            return;
        Integer ix;
        Float fx, fy;
        String st;
        boolean bVisible = true;
        ix = (Integer) hs.get("frameX");
        if (ix != null)
            frameRec.x = ix.intValue();
        ix = (Integer) hs.get("frameY");
        if (ix != null)
            frameRec.y = ix.intValue();
        ix = (Integer) hs.get("frameWidth");
        if (ix != null)
            frameRec.width = ix.intValue();
        ix = (Integer) hs.get("frameHeight");
        if (ix != null)
            frameRec.height = ix.intValue();
        initGraphicsToolBar();

        createHelpOverlayPanel();
/*
        st = (String) hs.get("mainMenu");
        if (st != null) {
            if (st.equals("off"))
                topMenuBar.setVisible(false);
            else
                topMenuBar.setVisible(true);
        }
*/
        st = (String)hs.get("StatusMessages");
        if (st != null)
        {
            try
            {
                int n = Integer.parseInt(st);
                Util.setStatusMessagesNumber(n);
            }
            catch (Exception e) {}
        }
        // Set the command area height
        commandRH = 18;
        ix = (Integer) hs.get("commandAreaHeight");
        if (ix != null)
        {
            commandH = ix.intValue();
            commandRH = commandH;
        }
        // Set the command area visibility
        bVisible = true;
        st = (String)hs.get("commandArea");
        if (st != null)
        {
            bVisible = st.equals("off") ? false : true;
        }
        if (!m_bShowCmdArea)
            bVisible = false;
        if (!bVisible) {
            commandArea.setVisible(false);
        }
        else {
            commandArea.setVisible(true);
        }
        //Set the message area height
        ix = (Integer) hs.get("messageAreaHeight");
        if (ix != null) {
            messageRH = ix.intValue();
        }
        else {
            messageRH = 0;
        }
        // Set the visibility of the message area and command area.
        bVisible = true;
        st = (String) hs.get("messageArea");
        if (st != null) {
            bVisible = st.equals("off") ? false : true;
        }
        if (!m_bShowCmdArea)
            bVisible = false;
        messageArea.setVisible(bVisible);
        st = (String) hs.get("systoolBar");
        if (st != null) {
            if (st.equals("off")) {
                sysToolPan.setVisible(false);
                xsysToolBar = 0;
            }
            else {
                sysToolPan.setVisible(true);
                xsysToolBar = 1;
            }
        }
        ix = (Integer) hs.get("systoolTop");
        if (ix != null)
            sysToolAtTop = ix.intValue();
        ix = (Integer) hs.get("usrInSys");
        if (ix != null)
            usrToolInSysTool = ix.intValue();

        ix = (Integer) hs.get("topMenuH");
        if (ix != null)
            topMenuH = ix.intValue();
        ix = (Integer) hs.get("sysToolH");
        if (ix != null)
            sysToolH = ix.intValue();
        ix = (Integer) hs.get("usrToolH");
        if (ix != null)
            usrToolH = ix.intValue();

        st = (String) hs.get("toolBar");
        if (st != null) {
            if (st.equals("off")) {
                xtoolBar = 0;
                usrToolBar.setVisible(false);
            }
            else {
                xtoolBar = 1;
                usrToolBar.setVisible(true);
            }
        }
        st = (String) hs.get("infoBar");
        if (st != null) {
            if (st.equals("off")) {
                statusBar.setVisible(false);
                statusButPane.setVisible(false);
                xinfoBar = 0;
            }
            else {
                statusBar.setVisible(true);
                statusButPane.setVisible(true);
                xinfoBar = 1;
            }
        }

        fx = (Float) hs.get("paramRx");
        fy = (Float) hs.get("paramRy");
        if (fx != null && fy != null) {
            paramRx = fx.floatValue();
            paramRy = fy.floatValue();
            if (paramRx < 0) paramRx = 0;
            if (paramRy < 0) paramRy = 0;
        }
        else {
            paramRx = 0.3f;
            paramRy = 0.6f;
        }
        if (frameRec.x < 0)  frameRec.x = 10;
        if (frameRec.y < 0)  frameRec.y = 10;
        if (frameRec.width <= 0)  frameRec.width = 600;
        if (frameRec.height <= 0)  frameRec.width = 600;
        st = (String) hs.get("paramPin.status");
        if (st != null)
            paramPinPan.setStatus(st);
        st = (String) hs.get("vpLayout");
        if (st != null) {
            if (st.equals("off"))
                bChangeLayout = false;
            else
                bChangeLayout = true;
        }
        if (!messageArea.isVisible()) {
            messageRH = 0;
        }
        else {
            commandArea.setVisible(true);
        }
       
        vToolPanel.initUiLayout();
    }

    public void setVpLayout(boolean b) {
        bChangeLayout = b;
    }

    public void setMenuBar(JComponent b) {
        if (b != null && topMenuBar != null) {
            SwingUtilities.updateComponentTreeUI(b); 
            topMenuBar.setMenuBar(b);
         }
    }

    public void setSysToolBar(JComponent b) {
        if (b != null && sysToolPan != null) {
             SwingUtilities.updateComponentTreeUI(b); 
            sysToolPan.setToolBar(b);
        }
    }

    public void setSysLeftToolBar(JComponent b) {
        if (b != null && sysToolPan != null) {
            SwingUtilities.updateComponentTreeUI(b); 
            sysToolPan.setLeftToolBar(b);
        }
    }

    public void showImagePanel(boolean bShow, String imageFile) {
        if (imgPane == null) {
            imgPane = new ImageViewPanel();
            paramPinPan.addImagePanel((JComponent) imgPane);
        }
        imgPane.setVisible(bShow);
        if (bShow)
            imgPane.showImage(imageFile);
    }

    public boolean isResizing() {
        if (isDraging || bVpAction)
             return true;
        return false;
    }

    public boolean inVpAction() {
        return bVpAction;
    }

    public int getVpId() {
        return vpId;
    }

    public boolean setViewPort(int vpNum, int status) {
        if (vpNum < 0 || bWaitInput)
            return false;
        if (status > 0) {
              if (expViewArea != null) {
                if (!expViewArea.isExpAvailable(vpNum))
                   return false;    
              }
//            if (vpId != vpNum) {
//                // Notify everybody of viewport change.
//                // New viewport will have a different jviewport value.
//                sendToVnmr("vnmrjcmd('pnew','jviewport')");
//            }
            vpId = vpNum;
        }
        else
            vpId = -1;
        return true;

    }

    public boolean setViewPort(int expNum) {
        return setViewPort(expNum, 1);
    }

    // set single or multiple viewports visible
    public boolean setSmallLarge(int vpNum) {
        if (vpNum < 0 || bWaitInput)
            return false;
        if (expViewArea != null)
            expViewArea.toggleVpSize(vpNum);
        return true; 
    }

    public void saveUiLayout() {
        if (sshare == null)
            return;
        hs = sshare.userInfo();
        if (hs == null)
            return;
        String str;
        Dimension  dim = Toolkit.getDefaultToolkit().getScreenSize();
        hs.put("screenWidth", new Integer(dim.width));
        hs.put("screenHeight", new Integer(dim.height));
        VNMRFrame frame = VNMRFrame.getVNMRFrame();
        Point pt = frame.getLocation();
        dim = frame.getSize();
        hs.put("frameX", new Integer(pt.x));
        hs.put("frameY", new Integer(pt.y));
        hs.put("frameWidth", new Integer(dim.width));
        hs.put("frameHeight", new Integer(dim.height));
        if ((topMenuBar != null) && (topMenuBar.isVisible()))
            hs.put("mainMenu", "on");
        else
            hs.put("mainMenu", "off");
        hs.put("topMenuH", new Integer(topMenuH));
        hs.put("sysToolH", new Integer(sysToolH));
        hs.put("usrToolH", new Integer(usrToolH));
        hs.put("systoolTop", new Integer(sysToolAtTop));
        if ((sysToolPan != null) && (sysToolPan.isVisible()))
            hs.put("systoolBar", "on");
        else
            hs.put("systoolBar", "off");
        if ((usrToolBar != null) && (usrToolBar.isVisible()))
            hs.put("toolBar", "on");
        else
            hs.put("toolBar", "off");
        if ((statusBar != null) && (statusBar.isVisible()))
            hs.put("infoBar", "on");
        else
            hs.put("infoBar", "off");
        hs.put("usrInSys", new Integer(usrToolInSysTool));

        Rectangle r = getBounds();
        float f1 = (float) ((float) controlX / (float) r.width);
        float f2 = (float) ((float) controlY / (float) r.height);
        hs.put("paramLocX", new Float(f1));
        hs.put("paramLocY", new Float(f2));
        hs.put("paramRx", new Float(paramRx));
        hs.put("paramRy", new Float(paramRy));
        str = paramPinPan.getStatus(); 
        if (str != null)
            hs.put("paramPin.status", str);
        str = vToolPanel.getStatus(); 
        if (str != null)
            hs.put("toolPin.status", str);

        if (commandArea != null)
        {
            str = commandArea.isVisible() ? "on" : "off";
            hs.put("commandArea", str);
            hs.put("commandAreaHeight", new Integer(commandRH));
        }
        if (messageArea != null)
        {
            str = (messageArea.isVisible() && commandArea.isVisible()) ? "on" : "off";
            hs.put("messageArea", str);
            hs.put("messageAreaHeight", new Integer(messageRH));
        }

        if (bChangeLayout)
            hs.put("vpLayout", "on");
        else
            hs.put("vpLayout", "off");

        if (graphicsToolBar != null) {
            r = graphicsToolBar.getBounds();
            hs.put("buttonPanHeight", new Integer(r.height));
            if (csiButtonPalette.isShow())
                saveToolBarDefaults(graphicsToolBar, graph2ToolKey);
            else
                saveToolBarDefaults(graphicsToolBar, graphToolKey);
        }
        if (csiButtonPalette.isShow()) {
            saveToolBarDefaults((GraphicsToolIF)csiButtonPalette, csiToolKey);
        }
    }


    public void saveUiLayout(String f) {
        if (f == null)
            return;
        Rectangle r;
        float     f1, f2;
        Point     pt;
        Dimension dim;
        PrintWriter os = null;
        try {
           os = new PrintWriter(new FileWriter(f));
           dim = Toolkit.getDefaultToolkit().getScreenSize();
           os.println("screenWidth  "+Integer.toString(dim.width));
           os.println("screenHeight  "+Integer.toString(dim.height));

           VNMRFrame frame = VNMRFrame.getVNMRFrame();
           pt = frame.getLocation();
           dim = frame.getSize();
           os.println("frameX  "+Integer.toString(pt.x));
           os.println("frameY  "+Integer.toString(pt.y));
           os.println("frameWidth  "+Integer.toString(dim.width));
           os.println("frameHeight  "+Integer.toString(dim.height));
           if ((topMenuBar != null) && (topMenuBar.isVisible()))
                os.println("mainMenu  on");
           else
                os.println("mainMenu  off");
           if ((usrToolBar != null) && (usrToolBar.isVisible()))
                os.println("toolBar  on");
           else
                os.println("toolBar  off");
           if ((statusBar != null) && (statusBar.isVisible()))
                os.println("infoBar  on");
           else
                os.println("infoBar  off");
           r = getBounds();
           f1 = (float) ((float) controlX / (float) r.width);
           f2 = (float) ((float) controlY / (float) r.height);
           os.println("paramLocX  "+Float.toString(f1));
           os.println("paramLocY  "+Float.toString(f2));
           os.println("paramRx "+Float.toString(paramRx));
           os.println("paramRy "+ Float.toString(paramRy));
           String str = paramPinPan.getStatus();
           if (str != null)
               os.println("paramPin.status "+str);
           str = vToolPanel.getStatus();
           if (str != null)
               os.println("toolPin.status "+str);

           if (graphicsToolBar != null) {
                r = graphicsToolBar.getBounds();
                os.println("buttonPanHeight "+ Integer.toString(r.height));
                if (graphicsToolBar.isShow())
                   os.println(graphToolKey+" on");
                else
                   os.println(graphToolKey+" off");
           }
           if (csiButtonPalette != null) {
                if (csiButtonPalette.isShow())
                   os.println(csiToolKey+" on");
                else
                   os.println(csiToolKey+" off");
           }

           if (bChangeLayout)
               os.println("vpLayout on");
           else
               os.println("vpLayout off");
           // os.close();
        }
        catch(IOException er)
        {
            //System.err.println(er.toString());
            Messages.postError(er.toString());
        }
        finally {
            try {
                if (os != null)
                    os.close();
            } catch (Exception e) {}
        }

    }

    public void enableResize() {
        resizeActive = true;
    }

    public void disableResize() {
        bResizable = false;
        resizeActive = false;
    }

    public void enableCmdInput() {
        if (commandArea.isVisible()) {
             commandArea.setFocus();
             sendToAllVnmr("jFunc("+XINPUT+", 1)\n");
        }
    }

    public void recordCurrentLayout() {
        if ((topMenuBar != null) && (topMenuBar.isVisible()))
              xmainMenu = 1;
        else
              xmainMenu = 0;
        if ((usrToolBar != null) && (usrToolBar.isVisible()))
              xtoolBar = 1;
        else
              xtoolBar = 0;
        if ((sysToolPan != null) && (sysToolPan.isVisible()))
              xsysToolBar = 1;
        else
              xsysToolBar = 0;
        if ((statusBar != null) && (statusBar.isVisible()))
              xinfoBar = 1;
        else
              xinfoBar = 0;
        xparamTop = paramPinPan.getStatusVal();
        xlocatorTop = vToolPanel.getStatusVal();
    }

    public void copyCurrentLayout(VpLayoutInfo info) {
        info.bAvailable = true;
        info.bNeedSave = true;
        info.xmainMenu = xmainMenu;
        info.xtoolBar = xtoolBar;
        info.xsysToolBar = xsysToolBar;
        info.xinfoBar = xinfoBar;
        info.xlocatorTop = xlocatorTop;
        info.xparamTop = xparamTop;
        info.xgraphicsToolBar = xgToolBar;
        info.xgrMaxH = 0;
        info.xgrMaxW = 0;
        info.xcontrolX = paramRx;
        info.xcontrolY = paramRy;
        info.vpX = viewAreaX;
        info.vpY = handleY;
    }

    public void compareCurrentLayout(VpLayoutInfo info) {
        geoChanged = false;
        if (info.xinfoBar != xinfoBar) {
            geoChanged = true;
            return;
        }
        if (info.xlocatorTop != xlocatorTop) {
            geoChanged = true;
            return;
        }
        if (info.xparamTop != xparamTop) {
            geoChanged = true;
            return;
        }
        if (info.xsysToolBar != xsysToolBar) {
            geoChanged = true;
            return;
        }
        if (info.xtoolBar != xtoolBar) {
            geoChanged = true;
            return;
        }
        if (info.xcontrolX != paramRx) {
            geoChanged = true;
            return;
        }
        if (info.xcontrolY != paramRy) {
            geoChanged = true;
            return;
        }
        if (info.vpY != handleY) {
            geoChanged = true;
            return;
        }
        if (info.vpX != viewAreaX) {
            geoChanged = true;
            return;
        }
    }

    public CommandInput getCmdLine() {
        return commandArea;
    }

    // There are two different controls over the showing of the command line.
    // One is the "right" enablecmdline controlled by the users profile such as
    // BasicLiquids.xml.  The other control is via macro command "cmdLine".
    // It is the cmdLine call that comes here.  
    public void setCmdLine(boolean bShow)
    {
       
        showCmdAreaViaMacro = bShow;
        
        // Cause this to take effect
        sendCmdLineOpen();
        
    }

    public void setCurrentLayout(VpLayoutInfo info) {

        // sendToAllVnmr("M@xresize\n");
        if (expViewArea == null)
            return;
        expViewArea.setResizeStatus(true);

        xmainMenu = info.xmainMenu;
        xtoolBar = info.xtoolBar;
        xsysToolBar = info.xsysToolBar;
        paramRy = info.xcontrolY;
        paramRx = info.xcontrolX;
        handleY = info.vpY;
        viewAreaX = info.vpX;
        xinfoBar = info.xinfoBar;

        if (usrToolBar != null) {
           if (xtoolBar > 0) {
                usrToolBar.setVisible(true);
           }
           else {
                sysToolPan.setVisible(false);
           }
        }
        if (sysToolPan != null ) {
           if (xsysToolBar > 0) {
                sysToolPan.setVisible(true);
           }
           else {
                sysToolPan.setVisible(false);
           }
        }
        if (statusBar != null) {
            if (xinfoBar > 0) {
                statusBar.setVisible(true);
                statusButPane.setVisible(true);
            }
            else {
                statusBar.setVisible(false);
                statusButPane.setVisible(false);
            }
        }
        xparamTop = info.xparamTop;
        paramPinPan.setStatus(xparamTop);
       //    vToolPanel.setStatus(xlocatorTop);
    }

    // public synchronized void switchLayout(int vpNum) {
    public void switchLayout(int vpNum) {

         if (vpNum != vpId) { // set vpId before switch to avoid quick switch
                              // back and forth
              return;
         }
         if (bSwitching) {
              return;
         }

         if (expViewArea == null)
            return;
         bSwitching = true;
         int oldId = -1;
         try
         {
             if (bNative) {
                if (orgRM == null)
                   orgRM = RepaintManager.currentManager(this);
                if (dummyRM == null) {
                   dummyRM = new DummyRepaintManager();
                   dummyRM.setDoubleBufferingEnabled(false);
                }
             }

             recordCurrentLayout();
             bVpAction = true;
             resizeActive = false;
             geoChanged = false;
             VpLayoutInfo vInfo = expViewArea.getLayoutInfo();
             if (bChangeLayout) {
                 if (vInfo != null) {  // save current layout info
                     copyCurrentLayout(vInfo);
                     oldId = vInfo.vpId;
                 }

                 if (oldId != vpId) {
                     vInfo = expViewArea.getLayoutInfo(vpId);
                     if ((vInfo != null) && vInfo.bAvailable) {
                         compareCurrentLayout(vInfo);
                     }
                 }
             }
             if (bNative) {
                if (geoChanged)
                   RepaintManager.setCurrentManager(dummyRM);
                else
                   expViewArea.setSuspend(true);
             }
             if(vToolPanel != null)
                vToolPanel.setViewPort(vpId);
             expViewArea.setActiveCanvas(vpId);
             if (geoChanged) {
                 setCurrentLayout(vInfo);
                 updateToolbarListeners();
             }
             Util.getDisplayOptions().setViewPort(vpId);
             bVpAction = false;
             if (geoChanged || expViewArea.getShownNum() <= 1) {
                 // doLayout();
                revalidate();
            }
/*
            if (isDraging)
               sendToAllVnmr("X@stop\n");
            else
               sendToAllVnmr("X@stop2\n");
*/

           enableCmdInput();
           resizeActive = true;
           if (bNative) {
              if (geoChanged) {
                 RepaintManager.setCurrentManager(orgRM);
                 ComponentEvent e = new ComponentEvent(this,
                                     ComponentEvent.COMPONENT_MOVED);
                 Toolkit.getDefaultToolkit().getSystemEventQueue().postEvent(e);
              }
              else {
                 expViewArea.setResizeStatus(false);
              }
           }
           else {
              expViewArea.setResizeStatus(false);
           }

           if(vToolPanel != null)
                vToolPanel.switchLayout(vpId, bChangeLayout);

           bSwitching = false;
       }
       catch (Exception e)
       {
           //e.printStackTrace();
           Messages.writeStackTrace(e);
           bSwitching = false;
       }
    }

    public void openUiLayout(String f, int vpNum) {
         if (vpNum != vpId) {
              return;
         }
         if (expViewArea == null)
            return;
         bVpAction = true;
         resizeActive = false;
         geoChanged = false;
         if (f != null) {
             openUiLayout(f);
         }
         bVpAction = false;
         if (geoChanged) {
            paramPinPan.validate();
            vToolPanel.validate();
            expViewArea.validate();
            if (bNative)
                expViewArea.setGraphicRegion();
            revalidate();
         }
/*
         if (isDraging)
               sendToAllVnmr("X@stop\n");
         else
               sendToAllVnmr("X@stop2\n");
*/
         expViewArea.setActiveCanvas(vpId);
         expViewArea.setResizeStatus(false);
         enableCmdInput();
         resizeActive = true;
    }

    public void openUiLayout(String f) {
        BufferedReader  in = null;
        String  data, atr;
        StringTokenizer tok;
        Rectangle r;
        int iv;
        float fv;
        boolean bChanged = false;

         if (expViewArea == null)
            return;
        if (f == null) {
           return;
        }

        try {
           in = new BufferedReader(new FileReader(f));
           bChanged = false;
           // sendToAllVnmr("M@xresize\n");
           expViewArea.setResizeStatus(true);
           r = getBounds();
           while ((data = in.readLine()) != null)
           {
                if (data.length() < 2)
                    continue;
                tok = new StringTokenizer(data, " ,\n\t");
                if (!tok.hasMoreTokens())
                    continue;
                atr = tok.nextToken();
                if (!tok.hasMoreTokens())
                    continue;
                data = tok.nextToken();
                if (atr.equals("frameX")) {
                    frameRec.x = Integer.parseInt(data);
                    continue;
                }
                if (atr.equals("frameY")) {
                    frameRec.y = Integer.parseInt(data);
                    continue;
                }
                if (atr.equals("frameWidth")) {
                    iv = Integer.parseInt(data);
                    if (iv != frameRec.width) {
                        frameRec.width = iv;
                        bChanged = true;
                    }
                    continue;
                }
                if (atr.equals("frameHeight")) {
                    iv = Integer.parseInt(data);
                    if (iv != frameRec.height) {
                        frameRec.height = iv;
                        bChanged = true;
                    }
                    continue;
                }
                if (atr.equals("mainMenu")) {
                    continue;
                }
                if (atr.equals("paramRx")) {
                    fv = Float.parseFloat(data);
                    if (paramRx != fv) {
                        paramRx = fv;
                        bChanged = true;
                    }
                    controlX = (int) ((float) r.width * paramRx);
                    continue;
                }
                if (atr.equals("paramRy")) {
                    fv = Float.parseFloat(data);
                    if (paramRy != fv) {
                        paramRy = fv;
                        bChanged = true;
                    }
                    controlY = (int) ((float) r.height * paramRy);
                    continue;
                }
                if (atr.equals("toolBar")) {
                    if (data.equals("off")) {
                        if (usrToolBar.isVisible()) {
                             bChanged = true;
                             usrToolBar.setVisible(false);
                        }
                    }
                    else {
                        if (!usrToolBar.isVisible()) {
                             bChanged = true;
                             usrToolBar.setVisible(true);
                        }
                    }
                    continue;
                }
                if (atr.equals("infoBar")) {
                    if (data.equals("off")) {
                        if (statusBar.isVisible() ) {
                             bChanged = true;
                             statusBar.setVisible(false);
                             statusButPane.setVisible(false);
                        }
                    }
                    else {
                        if (!statusBar.isVisible() ) {
                             bChanged = true;
                             statusBar.setVisible(true);
                             statusButPane.setVisible(true);
                        }
                    }
                    continue;
                }
                if (atr.equals("graphMaxHeight")) {
                    continue;
                }
                if (atr.equals("graphMaxWidth")) {
                    continue;
                }
                if (atr.equals("paramPin.status")) {
                    paramPinPan.setStatus(data);
                    iv = paramPinPan.getStatusVal();
                    if (xparamTop != iv)
                        bChanged = true;
                    continue;
                }
                if (atr.equals("panelOnTop")) {
                    continue;
                }
                if (atr.equals("toolPin.status")) {
                    vToolPanel.setStatus(data);
                    iv = vToolPanel.getStatusVal();
                    if (xlocatorTop != iv)
                        bChanged = true;
                    continue;
                }
                if (atr.equals("locatorOnTop")) {
                    continue;
                }
           }
        }
        catch(IOException e)
        {
            String strError = "Error reading file: "+ f;
            Messages.writeStackTrace(e, strError);
            Messages.postError(strError);
        }
        finally {
            try {
                if (in != null)
                    in.close();
            } catch (Exception ee) {}
        }

        if (!bChanged) {
           if (!bVpAction) {
               sendToAllVnmr("X@stop\n");
               expViewArea.setResizeStatus(false);
           }
           geoChanged = false;
           return;
        }
        if (!bVpAction) {
           bLayout = true;
           revalidate();
           // expViewArea.setGraphicRegion();
           // sendToAllVnmr("X@stop\n");
           // expViewArea.setResizeStatus(false);
        }
        geoChanged = true;
    }

    public void showSmsPanel(boolean b, boolean mode) {
        if (expViewArea != null)
           expViewArea.showSmsPanel(b, mode);
    }

    public void showJMolPanel(boolean bShow)
    {
        if (expViewArea != null)
            expViewArea.showJMolPanel(bShow);
    }

    public void showJMolPanel(int id, boolean bShow)
    {
        if (expViewArea != null)
            expViewArea.showJMolPanel(id, bShow);
    }

    /**
     *  Checks if there is a key accelerator for the given name.
     *  @param strName  the name of the menu item, the button or any other name.
     */
    public boolean isKeyBinded(String strName)
    {
        if (m_htNameKey != null)
             if (m_htNameKey.get(strName) != null)
                return true;
        return false;
    }

    /**
     *  Gets the name of the key accelerators for the given name.
     *  @param strName  the name of the menu item, the button or any other name,
     */
    public String getKeyBinded(String strName)
    {
        String strKey = (String)m_htNameKey.get(strName);
        if (strKey == null)
             strKey = "";
        return strKey;
    }

    /**
     *  @param strName  the name of the menu item, the button or any other name,
     *  strName and strKey are defined in KeyStrings.txt, where
     *  strKey is accelerator keys separated by +, i.e.,
     *  control+L or control+shift+Z. But VSubMenuItem uses
     *  KeyStroke.getKeyStroke(str) to set accelerator keys, where str
     *  has the syntax of "control L", "control shift Z".
     */
    public String getKeyBindedForMenu(String strName)
    {
        String strKey = (String)m_htNameKey.get(strName);
        if (strKey == null)
             strKey = "";
        return strKey.replace('+', ' ');
    }

    private void setUsrToolBarPosition(int pos) {
        if (pos == 0) {
            sysToolAtTop = 1;
            usrToolInSysTool = 1;
            remove(usrToolBar);
            sysToolPan.setMiddleToolBar(usrToolBar);
            revalidate();
            return;
        }
        usrToolInSysTool = 0;
        if (pos == 1)
            sysToolAtTop = 0;
        else
            sysToolAtTop = 1;
        sysToolPan.setMiddleToolBar(null);
        add(usrToolBar);
        revalidate();
    }

    public void setVerticalDividerLoc(int location) {
        Dimension d = getSize();
        VpLayoutInfo info = null;
        if (expViewArea != null)
            info = expViewArea.getLayoutInfo();
        if (location < 0)
            location = 0;
        paramRy = (float) location / (float) d.height;
        if (paramRy >= 1.0f)
            paramRy = 0.95f;
        if (bChangeLayout && info != null) {
            info.xcontrolY = paramRy;
            info.bNeedSave = true;
        }
        revalidate();
    }

    public void setHorizontalDividerLoc(int location) {
        Dimension d = getSize();
        VpLayoutInfo info = null;
        if (expViewArea != null)
            info = expViewArea.getLayoutInfo();
        if (location < 5)
            location = 5;
        paramRx = (float) location / (float) d.width;
        if (paramRx > 1.0f)
            paramRx = 1.0f;
        if (bChangeLayout && info != null) {
            info.xcontrolX = paramRx;
            info.bNeedSave = true;
        }
        revalidate();
    }

    public void raiseToolPanel(boolean bFront)
    {
        if (expViewArea == null)
            return;
        if (bFront) {
            if (vToolPanel.isVisible()) {
               if (getLayer(vToolPanel) > LAYER2)
                  return;
            }
            expViewArea.setResizeStatus(true);
            int n = vToolPanel.getOpenCount();
            if (n > 0)
               setLayer(vToolPanel, LAYER3);
            else
               setLayer(vToolPanel, LAYER4);
            vToolPanel.setVisible(true);
        }
        else {
            if (getLayer(vToolPanel) < LAYER2)
               return;
            if (!vToolPanel.isVisible()) {
               setLayer(vToolPanel, LAYER1);
               return;
            }
            expViewArea.setResizeStatus(true);
            setLayer(vToolPanel, LAYER1);
        }
        expViewArea.setGraphicRegion();
        expViewArea.setResizeStatus(false);
    }

    public void raiseComp(Component c, boolean bFront)
    {
        if (expViewArea == null)
            return;
        expViewArea.setResizeStatus(true);
        if (bFront) {
           setLayer(c, LAYER3);
           c.setVisible(true);
        }
        else {
           setLayer(c, LAYER1);
           c.setVisible(false);
        }
        expViewArea.setGraphicRegion();
        expViewArea.setResizeStatus(false);
    }

    public void openComp(String name, String cmd) {
        if (name == null)
            return;
        if (cmd.equalsIgnoreCase("open")) {
            openTool(name, true, false);
            updateToolbarListener(name);
            return;
        }
        if (cmd.equalsIgnoreCase("show")) {
            // make tool available but unnecessarily visible 
            openTool(name, true, true);
            return;
        }
        if (cmd.equalsIgnoreCase("close")) {
            closeTool(name);
            updateToolbarListener(name);
            return;
        }
        if (cmd.equalsIgnoreCase("moveup")) {
            moveTool(name, true);
            return;
        }
        if (cmd.equalsIgnoreCase("movedown")) {
            moveTool(name, false);
            return;
        }
        if (cmd.equalsIgnoreCase("hide")) {
            hideTool(name);
            updateToolbarListener(name);
            return;
        }

        if (cmd.equalsIgnoreCase("switch") || cmd.equalsIgnoreCase("toggle")) {
            toggleTool(name);
            updateToolbarListener(name);
            return;
        }
    }

    // x and y is the point on screen
    public void moveComp(String name, int x, int y) {
        if (!name.equalsIgnoreCase(usrtoolname))
             return;
        if (usrToolBar == null || sysToolPan == null)
             return;
        if (!sysToolPan.isVisible())
             return;
        Point loc = sysToolPan.getLocationOnScreen();
        Dimension dim = sysToolPan.getSize();
        if (x < loc.x)
             return;
        int y1 = loc.y - 5; // tolerance
        int y2 = loc.y + dim.height + 2;
        int dir = 0;

        if (y < y1)
            dir = 1; 
        if (y > y2)
            dir = 2; 
        setUsrToolBarPosition(dir);
    }

    public void createTopTools() {
           if (bRebuild) {
               topMenuBar = oldVjUI.topMenuBar;
               sysToolPan = oldVjUI.sysToolPan;
               usrToolBar = oldVjUI.usrToolBar;
               topToolBar = oldVjUI.usrToolBar;
               sysToolBut = oldVjUI.sysToolBut;
               topToolBut = oldVjUI.topToolBut;
               protocolPinPan = oldVjUI.protocolPinPan;
               protocolBrowser = oldVjUI.protocolBrowser;
               return;
           }
           topMenuBar = new MenuPanel(0, 10, 0, 0);
           add(topMenuBar, defaultLayer);

           usrToolBar = new VjToolBar(sshare, null);
           topToolBar = usrToolBar;
           add(usrToolBar, defaultLayer);
           topToolBut = usrToolBar.getControlButton();
           topToolBut.setName(usrToolName);
           topToolBut.setDragable(true);
           Util.setUsrToolBar(usrToolBar);

           sysToolPan = new SysToolPanel(sshare);
           add(sysToolPan, defaultLayer);
           sysToolBut = sysToolPan.getControlButton();
           sysToolBut.setName(sysToolName);
           sysToolBut.setMoveable(true);
           Util.setSysToolBar(sysToolPan);

           topMenuBar.setBorder(null);
           sysToolPan.setBorder(null);
           usrToolBar.setBorder(null);

           sysToolPan.setImageVpPanel(usrToolBar.getVPToolBar());
    }

    private void addTopTools() {
           add(topMenuBar, defaultLayer);
           add(sysToolPan, defaultLayer);
           if (usrToolInSysTool != 0)
               setUsrToolBarPosition(0);
           else
               add(usrToolBar, defaultLayer);
    }

    private void createProtocolPinPanel() {
       if (protocolPinPan != null)
           return;
       protocolBrowser = ExpPanel.getProtocolBrowser();
       if (protocolBrowser == null)
           return;
       protocolPinPan = new PushpinPanel(protocolBrowser);
       protocolPinPan.setTitle("Protocol Browser");
       protocolPinPan.setName("Protocol Browser");
       // protocolPinPan.setOrientation(JSplitPane.VERTICAL_SPLIT);
       protocolPinPan.showPushPin(true);
       protocolPinPan.setTabOnTop(true);
       protocolPinPan.setControler(paramTabBar);
       protocolPinPan.showTab(true);
       protocolPinPan.controlVisible(true);
       protocolPinPan.setOpaque(true);
       protocolPinPan.setTabOnTop(false);
       protocolPinPan.setStatus("close");
       Dimension dim = expViewArea.getSize();
       expViewArea.add(protocolPinPan, JLayeredPane.MODAL_LAYER);
       protocolPinPan.setSize(dim.width, dim.height);
       protocolPinPan.setLocation(0,0);
    }

    private void createHelpOverlayPanel() {
       if (helpOverlayPanel != null)
           return;

       helpOverlayPanel = new HelpOverlay(sshare);
       add(helpOverlayPanel, layer4); 
    }

    public HelpOverlay getHelpOverlay() {
       if (helpOverlayPanel == null)
           createHelpOverlayPanel();
       return helpOverlayPanel;
    }

    JComponent tool_1, tool_2;

    private boolean getToolByName(String name) {
        String toolName = name.toLowerCase();
        tool_1 = null;
        tool_2 = null;

        if (toolName.equals(systoolname)) {
           tool_1 = sysToolPan;
           return true;
        }
        if (toolName.equals(usrtoolname)) {
           tool_1 = usrToolBar;
           return true;
        }
        if (toolName.equals(graphtoolname)) {
           tool_1 = displayPalette;
           if (bCsiVisible)
              tool_2 = csiButtonPalette; 
           return true;
        }
        if (toolName.equals(hardwaretoolname)) {
           tool_1 = statusBar;
           tool_2 = statusButPane;
           return true;
        }
        if (toolName.equals(parameterpanelname)) {
           tool_1 = paramPinPan;
           return true;
        }
        if (toolName.equals(csitoolname)) {
           tool_1 = csiButtonPalette;
           return true;
        }
        if (toolName.equals(protocolbrowsername)) {
           if (protocolPinPan == null)
               createProtocolPinPanel();
           tool_1 = protocolPinPan;
           return true;
        }
        if (toolName.equals(helpoverlayname)) {
           if (helpOverlayPanel == null)
               createHelpOverlayPanel();
           tool_1 = helpOverlayPanel;
           return true;
        }

        return false;
    }

    public void openTool(String name, boolean bOpen, boolean bShowOnly) {
        if (getToolByName(name)) {
            if (tool_1 != null) {
               if (tool_1 instanceof PushpinIF)
                   ((PushpinIF)tool_1).setStatus("open");
               else if (tool_1 instanceof GraphicsToolIF)
                   ((GraphicsToolIF)tool_1).setShow(true);
               else
                   tool_1.setVisible(true);
            }
            if (tool_2 != null) {
               if (tool_2 instanceof PushpinIF)
                   ((PushpinIF)tool_2).setStatus("open");
               else if (tool_2 instanceof GraphicsToolIF)
                   ((GraphicsToolIF)tool_2).setShow(true);
               else
                   tool_2.setVisible(true);
            }
            validate();
            return;
        }
        if (name.equalsIgnoreCase(commandlinename)) {
            // Update rightsLst and m_bShowCmdArea
            // rightsLst = new RightsList(false);
            // m_bShowCmdArea = !rightsLst.approveIsFalse("enablecmdline");

            if (!m_bShowCmdArea)
                return;
            if (commandArea.isVisible()) {
                if (commandRH >= commandH) {
                   commandArea.setFocus();
                   return;
                }
            }
            if (bShowOnly)
                return;
            if (commandRH < commandH)
                commandRH = commandH + 1;
            messageRH = commandH * 2;
            commandArea.setVisible(true);
            messageArea.setVisible(true);
            revalidate();
            commandArea.setFocus();
            return;
        }

        vToolPanel.openTool(name, bOpen, bShowOnly);
    }

    public void openTool(String name, boolean bOpen) {
        openTool(name, bOpen, false);
    }

    public void closeTool(String name) {
        if (getToolByName(name)) {
            if (tool_1 != null) {
               if (tool_1 instanceof PushpinIF)
                   ((PushpinIF)tool_1).setStatus("close");
               else if (tool_1 instanceof GraphicsToolIF)
                   ((GraphicsToolIF)tool_1).setShow(false);
               else
                   tool_1.setVisible(false);
            }
            if (tool_2 != null) {
               if (tool_2 instanceof PushpinIF)
                   ((PushpinIF)tool_2).setStatus("close");
               else if (tool_2 instanceof GraphicsToolIF)
                   ((GraphicsToolIF)tool_2).setShow(false);
               else
                   tool_2.setVisible(false);
            }
            validate();
            return;
        }
        if (name.equalsIgnoreCase(commandlinename)) {
            commandArea.setVisible(false);
            // Unshowing the resize bar will make it so they cannot drag
            // the cmd line back open.  If it is un-shown here, it needs
            // to be re-shown where ever the commandArea is set visible.
            //cmdResizeBar.setVisible(false);
            return;
        }

        vToolPanel.closeTool(name);
    }

    public void hideTool(String name) {
        if (getToolByName(name)) {
            if (tool_1 != null) {
               if (tool_1 instanceof PushpinIF)
                   ((PushpinIF)tool_1).setStatus("hide");
            }
            if (tool_2 != null) {
               if (tool_2 instanceof PushpinIF)
                   ((PushpinIF)tool_2).setStatus("hide");
            }
            validate();
            return;
        }
        if (name.equalsIgnoreCase(commandlinename)) {
            commandArea.setVisible(false);
            return;
        }
        vToolPanel.hideTool(name);
    }

    public void moveTool(String name, boolean bUp) {
        int  oldVal = sysToolAtTop;
        if (name.equalsIgnoreCase(systoolname)) {
            if (sysToolPan != null) {
               if (bUp)
                  sysToolAtTop = 1;
               else
                  sysToolAtTop = 0;
            }
        }
        else if (name.equalsIgnoreCase(usrtoolname)) {
            if (usrToolBar != null) {
               if (bUp)
                  sysToolAtTop = 0;
               else
                  sysToolAtTop = 1;
            }
        }
        if (sysToolAtTop != oldVal) {
           if (sysToolPan.isVisible() && usrToolBar.isVisible())
               revalidate();
        }
    }

    public void toggleTool(String name) {
        if (getToolByName(name)) {
            if (tool_1 != null) {
                if (tool_1.isVisible()) {
                   if (tool_1 instanceof PushpinIF)
                      ((PushpinIF)tool_1).setStatus("close");
                   else
                      tool_1.setVisible(false); 
                   if (tool_2 != null)
                      tool_2.setVisible(false); 
                }
                else {
                   if (tool_1 instanceof PushpinIF)
                      ((PushpinIF)tool_1).setStatus("open");
                   else
                      tool_1.setVisible(true); 
                   if (tool_2 != null)
                      tool_2.setVisible(true); 
                }
            }
            validate();
            return;
        }
    }


    public boolean checkObject(String name, int id) {
        if (getToolByName(name)) {
            if (tool_1 != null) {
                if (tool_1.isVisible())
                    return true;
            }
        }
        if (name.equalsIgnoreCase(commandlinename)) {
            if (commandArea.isVisible())
               return true;
        }

        return false;
    }

    public boolean checkObject(String name) {
        return checkObject(name, vpId);
    }

    public boolean checkObject(String name, VObjIF obj) {
        int   id = vpId;
        ButtonIF bif = obj.getVnmrIF();
        if (bif != null) {
            id = ((ExpPanel) bif).getViewId();
        }
        return checkObject(name, id);
    }

    public boolean checkObjectExist(String name, int id) {
        if (getToolByName(name)) {
            if (tool_1 != null)
                return true;
        }

        if (rightsLst == null)
            rightsLst = new RightsList(false);
        String toolName = name.toLowerCase();
        if (toolName.equals(commandlinename)) {
            m_bShowCmdArea = !rightsLst.approveIsFalse("enablecmdline");
            return m_bShowCmdArea;
        }
        if (toolName.equals(locatorname)) {
            if (rightsLst.approveIsFalse("locatorok"))
               return false;
        }
        if (toolName.equals(browsername)) {
            if (rightsLst.approveIsFalse("broswerok"))
               return false;
        }
        if (toolName.equals(sqname)) {
            if (rightsLst.approveIsFalse("sqok"))
               return false;
        }
        JComponent comp = vToolPanel.searchTool(name, id);
        if (comp != null) {
            if (comp instanceof VToolPanel) {
               comp = ((VToolPanel) comp).searchTool(name, id);
            }
        }
        if (comp != null) {
            return true;
        }

        return false;
    }

    public boolean checkObjectExist(String name) {
        return checkObjectExist(name, vpId);
    }

    public boolean checkObjectExist(String name, VObjIF obj) {
        int   id = vpId;
        ButtonIF bif = obj.getVnmrIF();
        if (bif != null) {
            id = ((ExpPanel) bif).getViewId();
        }
        return checkObjectExist(name, id);
    }

    public void canvasSizeChanged( int id ) {
        if (gToolDock > DOCK_NONE) {
            graphicsToolBar.adjustDockLocation();
        }
        if (csiToolDock > DOCK_NONE) {
            if (csiButtonPalette.isVisible())
                csiButtonPalette.adjustDockLocation();
        }
    }

    private void setFrameListener(boolean bIconified) {
        int len = frameEventListeners.size();
        for (int k = 0; k < len; k++) {
            VFrameListener l = (VFrameListener)frameEventListeners.elementAt(k);
            if (l != null) {
                if (bIconified) {
                   l.saveStatus();
                   l.comp.setVisible(false);
                }
                else
                   l.recoverStatus();
            }
        }
    }

    public void frameEventProcess(WindowEvent e) {
        int id = e.getID();
        switch(id) {
          case WindowEvent.WINDOW_OPENED:
              if (graphicsToolBar == null)
                   createGraphicsToolBar();
              break;
          case WindowEvent.WINDOW_ICONIFIED:
              setFrameListener(true);
              break;
          case WindowEvent.WINDOW_DEICONIFIED:
              setFrameListener(false);
              break;
        }
    }

    public void addFrameEventListener(Component o) {
        if (o != null)
           frameEventListeners.add(new VFrameListener(o));
    }

    public void removeFrameEventListener(Component o) {
        int len = frameEventListeners.size();
        for (int k = 0; k < len; k++) {
            VFrameListener l = (VFrameListener)frameEventListeners.elementAt(k);
            if (l != null && l.comp == o)
                frameEventListeners.remove(l);
        }
    }

    public void addToolbarListener(Component o) {
         toolbarListeners.add(o);
    }

    public void removeToolbarListener(Component o) {
        int len = toolbarListeners.size();
        for (int k = 0; k < len; k++) {
            Component l = (Component)toolbarListeners.elementAt(k);
            if (l != null && l == o)
                toolbarListeners.remove(o);
        }
    }

    public void updateToolbarListeners() {
        int len = toolbarListeners.size();
        for (int k = 0; k < len; k++) {
            Component obj = (Component)toolbarListeners.elementAt(k);
            if (obj != null && (obj instanceof VObjIF))
                ((VObjIF) obj).updateValue();
        }
    }

    public void updateToolbarListener(String name) {
        String toolName = name.toLowerCase();
        int len = toolbarListeners.size();
        for (int k = 0; k < len; k++) {
            Component obj = (Component)toolbarListeners.elementAt(k);
            if (obj != null && (obj instanceof VObjIF)) {
                VObjIF vObj = (VObjIF) obj;
                String objName = vObj.getAttribute(VObjDef.CHECKOBJ);
                if (objName != null) {
                    String checkName = objName.toLowerCase();
                    if (toolName.equals(checkName))
                        vObj.updateValue();
                }
            }
        }
    }

    private void createDockButtonList() {
        Image img = null;
        ImageIcon icon = Util.getGeneralIcon("anchor.gif");
        if (icon != null)
            img = icon.getImage();

        dockImgWidth = 0;
        dockImgHeight = 0;
        if (img != null) {
            dockImgWidth = img.getWidth(null);
            dockImgHeight = img.getHeight(null);
        }

        if (img == null || dockImgWidth > 36) {
            icon = Util.getImageIcon("anchor.gif");
            if (icon != null)
                img = icon.getImage();
            if (img == null) {
               dockImgWidth = 24;
               dockImgHeight = 24;
            }
            else {
               dockImgWidth = img.getWidth(null);
               dockImgHeight = img.getHeight(null);
            }
        }

        dockButtonNum = 5;
        dockButtonList = new DockButton[dockButtonNum];
        for (int i = 0; i < dockButtonNum; i++) {
            dockButtonList[i] = new DockButton(i, img, dockImgWidth, dockImgHeight);
            add(dockButtonList[i], layer2);
        }
        dockButtonList[0].setDock(DOCK_FRAME_LEFT); // dock at frame
        dockButtonList[1].setDock(DOCK_EXP_RIGHT);
        dockButtonList[2].setDock(DOCK_ABOVE_EXP);
        dockButtonList[3].setDock(DOCK_BELLOW_EXP);
        dockButtonList[4].setDock(DOCK_EXP_LEFT);

        /*******
        dockButtonList[2].setDock(DOCK_NORTH_WEST); // viewport 
        dockButtonList[3].setDock(DOCK_SOUTH_WEST); // viewport 
        dockButtonList[4].setDock(DOCK_NORTH_EAST); // viewport 
        dockButtonList[5].setDock(DOCK_SOUTH_EAST); // viewport 
        dockButtonList[6].setDock(DOCK_EXP_RIGHT);
        ******/
    }

    public void setGraphToolMoveStatus(boolean bStart) {
        if (dockButtonList == null)
            createDockButtonList();
        int i;
        
        bGToolMoving = false;
        if (bStart) {
            ExpPanel exp = Util.getActiveView();
            if (exp == null)
                return;
            if (dockImgWidth < 1) {
                dockImgWidth = dockButtonList[0].getImageWidth();
                if (dockImgWidth < 1)
                   return;
            }

            dockedButton = null;
            bGToolMoving = true;
            Dimension dim = getSize();
            int h = dim.height - topBarY2 - 10;
            int w = dockImgWidth + 12;
            int y = topBarY2;

              // DOCK_FRAME_LEFT  the most left of frame
            dockButtonList[0].setRange(0, y, w, h);

              // DOCK_EXP_LEFT to the left side of viewArea
            dockButtonList[4].setRange(viewAreaX - w, y, w, h);
            w = dockImgWidth + 24;

              // DOCK_EXP_RIGHT the most right  of frame
            dockButtonList[1].setRange(dim.width - w, y, w, h);

              // DOCK_ABOVE_EXP 
            y = viewAreaY - dockImgHeight;
            if (y < 1)
               y = 1;
            w = dim.width - viewAreaX - dockImgWidth - 12;
            if (w < 20)
               w = 20;
            dockButtonList[2].setRange(viewAreaX, y, w, dockImgHeight);
              // DOCK_BELLOW_EXP 
            y = viewAreaY + viewAreaH;
            if ((y + dockImgHeight) > h)
               y = h - dockImgHeight;
            dockButtonList[3].setRange(viewAreaX, y, w, dockImgHeight);
        }
        // gToolDock = DOCK_NONE;
        for (i = 0; i < dockButtonNum; i++) {
            // if (dockButtonList[i].isActive())
            //     gToolDock = dockButtonList[i].getDock();
            dockButtonList[i].setActive(false);
            dockButtonList[i].setVisible(bStart);
        }
    }

    public void graphToolMoveTo(int x, int y) {
        if (dockedButton != null) {
            if (dockedButton.contain(x, y)) {
               if (dockedButton.getDock() != DOCK_EXP_RIGHT)
                  return;
            }
            else {
               dockedButton.setActive(false);
               dockedButton = null;
            }
        }
        DockButton newDockedButton = null;
        for (int i = 0; i < dockButtonNum; i++) {
            if (dockButtonList[i].contain(x, y)) {
                newDockedButton = dockButtonList[i];
                newDockedButton.setActive(true);
                break;
            }
        }
        if (dockedButton != null) {
            if (dockedButton != newDockedButton) {
                dockedButton.setActive(false);
            }
        }
        dockedButton = newDockedButton;
    }

    public int getGraphToolDockPosition() {
        if (dockedButton == null)
           return DOCK_NONE;
        return dockedButton.getDock();
    }

    public int getGraphToolDockPosition(JComponent comp) {
        if (comp == displayPalette)
           return gToolDock;
        return csiToolDock;
    }


    private void createMainPanels() {

       // Do the key bindings if there are any in the file.
       doKeyBindings();

       toolTabBar = new PushpinControler(SwingConstants.LEFT);
       paramTabBar = new PushpinControler(SwingConstants.RIGHT);
       toolTabBar.putClientProperty(PanelTexture, "no");
       paramTabBar.putClientProperty(PanelTexture, "no");
       toolTabBar.setVisible(false);
       paramTabBar.setVisible(false);
       add(toolTabBar, defaultLayer);
       add(paramTabBar, defaultLayer);

       if (Util.isImagingUser())
           Util.setMultiSq(false);
       else
           Util.setMultiSq(true);  // each viewport has its own StudyQueue
       
       if (bRebuild)
           expViewArea = oldVjUI.expViewArea;
       if (bRebuild && expViewArea != null) {
           expViewArea.rebuildUI(sshare, this);
           add(expViewArea, defaultLayer);
       }
       else {
           expViewArea = new ExpViewArea(sshare, this);
           add(expViewArea, defaultLayer);
       }
       Util.setViewArea(expViewArea);

       usrToolBar.setExpIf(expViewArea.getDefaultExp());

       // Build the DisplayOptions panel so that colors get defined.
       // displayOptions = Util.getDisplayOptions();

       if (bRebuild)
           vToolPanel = oldVjUI.vToolPanel;
       if (vToolPanel != null) {
           vToolPanel.setResource(sshare,this);
       }
       else {
           vToolPanel = new VTabbedToolPanel(sshare,this);
           vToolPanel.setOrientation(JSplitPane.HORIZONTAL_SPLIT);
           vToolPanel.setOpaque(true);
       }
       vToolPanel.setRightComponent(this);
       add(vToolPanel, defaultLayer);
       vToolPanel.fill();

       vToolPanel.setControler(toolTabBar);

       controlPanel = new ControlPanel(sshare);
       paramPinPan = new PushpinPanel(controlPanel);
       paramPinPan.setTitle("Parameter Panel");
       paramPinPan.setName("Parameter");
       paramPinPan.showTab(true);
       paramPinPan.setOrientation(JSplitPane.VERTICAL_SPLIT);
       paramPinPan.showPushPin(true);
       paramPinPan.setTopComponent(expViewArea);
       paramPinPan.setControler(paramTabBar);
       paramPinPan.controlVisible(true);
       paramPinPan.setOpaque(true);
       add(paramPinPan, defaultLayer);
       paramPanel = (JComponent) paramPinPan;
       // add(controlPanel, new Integer(LAYER1));

       if (bRebuild)
           commandArea = oldVjUI.commandArea;
       if (commandArea == null)
           commandArea = new CommandInput(sshare, this, appInstaller);
       add(commandArea, defaultLayer);

       if (bRebuild)
           messageArea = oldVjUI.messageArea;
       if (messageArea == null)
           messageArea = new MessageTool(sshare, this);
       add(messageArea, defaultLayer);

       Util.setControlPanel(controlPanel);
/*
       Util.setDisplayPalette(displayPalette);
*/

       vToolPanel.updateVpInfo(expViewArea.getMaxCanvasNum());

        cmdResizeBar = new JPanel();
        cmdResizeBar.setOpaque(false);
        cmdResizeBar.setCursor(Cursor.getPredefinedCursor(Cursor.N_RESIZE_CURSOR));
        cmdSplitBar = new JSplitPane();
        cmdSplitBar.setOneTouchExpandable(false);
        cmdSplitBar.setOrientation(JSplitPane.VERTICAL_SPLIT);

        // Update rightsLst and m_bShowCmdArea
        rightsLst = new RightsList(false);
        m_bShowCmdArea = !rightsLst.approveIsFalse("enablecmdline");

        if (!m_bShowCmdArea)
        {
            // commandArea.setEnabled(false);
            // messageArea.setEnabled(false);
            disableResize();
            cmdResizeBar.setVisible(false);
            cmdSplitBar.setVisible(false);
        }
        else {
            add(cmdResizeBar, layer1);
            add(cmdSplitBar, defaultLayer);
            setResizeBarProc();
        }

        addComponentListener(new ComponentAdapter() {
            public void componentMoved(ComponentEvent evt) {
                 repaint();
                 expViewArea.setResizeStatus(false);
            }
        });

        if (bRebuild)
           controlPanel.buildTopButtons();

        expViewArea.resumeUI();
       //Util.resetVjColors();

      //  expViewArea.run();

    }

    public void doLayout() {
        getLayout().layoutContainer(this);
    }

    public void setBgColor() {
        Color color = Util.getToolBarBg();
        sysToolPan.setBg(color);
        usrToolBar.setBackground(color);
        color = new Color(color.getRGB());
        sysToolBut.setBackground(color);
        topToolBut.setBackground(color);
        color = Util.getBgColor();
        statusBut.setBackground(color);
        color = Util.getSeparatorBg();
        cmdResizeBar.setBackground(color);
       
       /****************
        Color sep=Util.getSeparatorBg();
        cmdResizeBar.setBackground(Util.getSeparatorBg());

        int len = separatorList.size();
        for (int k = 0; k < len; k++) {
             JSeparator sp = (JSeparator) separatorList.elementAt(k);
             if (sp != null)
                 sp.setBackground(sep);
        }

        Color bg = Util.getBgColor();
        setBackground(bg);
        if(statusBar!=null)
            statusBar.setBackground(bg);
        if (statusBut != null) 
           statusBut.setBg(bg);
 
        if (graphicsToolBar != null)
            ((Component)graphicsToolBar).setBackground(bg);
        if (csiButtonPalette != null)
            csiButtonPalette.setBackground(bg);

        vToolPanel.setBackground(bg);
         controlPanel.setBackground(bg);
         
        Color color = Util.getMenuBarBg();
        topMenuBar.setBackground(color);

        color = Util.getToolBarBg();
        usrToolBar.setBackground(color);
        topToolBut.setBg(color);
        sysToolPan.setBg(color);
        sysToolBut.setBg(color);
       ****************/
     }

    public void propertyChange(PropertyChangeEvent evt) {
        if(DisplayOptions.isUpdateUIEvent(evt)){
            // VNMRFrame.getVNMRFrame().updateComponentTreeUI();
            setBgColor();
        }
    }
    
    private void setResizeBarProc() {
        cmdResizeBar.addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent evt) {
                if (!resizeActive)
                    return;
                bResizable = false;
                py = evt.getY();
                ps = cmdResizeBar.getLocation();
                pyd = py;
                viewRec = expViewArea.getBounds();
                viewRec2 = expViewArea.getBounds();
                if (titleBarH <= 0) {
                    titleBarH = expViewArea.getTitleBarHeight();
                    if (titleBarH < handleH)
                       titleBarH = handleH;
                }
                isDraging = false;
                bResizable = true;
                prePos = messageRH;
            }

            public void mouseReleased(MouseEvent evt) {
                if (expViewArea == null)
                    return;
                if (isDraging) {
                    isDraging = false;
                    expViewArea.revalidate();
                    if (bNative)
                        expViewArea.setGraphicRegion();
                    expViewArea.setResizeStatus(false);
                }
                if (!resizeActive)
                    return;
                if (commandArea.isVisible()) {
                    enableCmdInput();
                }
                else
                    sendToAllVnmr("jFunc("+XINPUT+", 0)\n");
                if (messageArea.isVisible())
                    messageArea.showBottomLine();
            }
        });  // MouseListener

        cmdResizeBar.addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent evt) {
                int     ny;
                if (viewRec == null)
                    return;
                if (!resizeActive || !bResizable || !showCmdAreaViaMacro)
                    return;
                
                // Update rightsLst and m_bShowCmdArea.  If cmdline is
                // not enabled, then don't let them resize it, else they
                // can just open it by resizing it.
                // rightsLst = new RightsList(false);
                // m_bShowCmdArea = !rightsLst.approveIsFalse("enablecmdline");
                if(!m_bShowCmdArea)
                    return;

                if( isFirstClick )
                {
                   isFirstClick = false;
                }
                py = evt.getY();
                ny = ps.y + (py - pyd);
                if (ny < topBarY2)
                   ny = topBarY2;
                if (ny != ps.y) {
                    ps.y = ny;
                    viewRec.y = ny + handleH;
                    if (viewRec.y >= controlY)
                        viewRec.y = controlY - 1;
                    viewRec.height = expY2 - viewRec.y + 1;
                    if( viewRec.height < 1 )
                        viewRec.height = 1;
                    if (!isDraging) {
                        expViewArea.setResizeStatus(true);
                    }
                    isDraging = true;
                    cmdResizeBar.setLocation(ps.x, ny);
                    cmdSplitBar.setLocation(ps.x, ny);
                    expViewArea.setBounds(viewRec);
                    if (protocolPinPan != null && protocolPinPan.isVisible()) {
                         protocolPinPan.setSize(viewRec.width, viewRec.height);
                         protocolBrowser.updateSize();
                    }
                    // ny--;
                    commandY = ny - commandH;
                    if (commandY < topBarY2)
                        commandY = topBarY2;
                    commandRH = ny - commandY;

                    if (commandRH <= 2) {
                        commandRH = 0;
                        messageRH = 0;
                        commandArea.setVisible(false);
                        messageArea.setVisible(false);
                        commandArea.validate();
                        return;
                    }
                    commandArea.setBounds(viewAreaX, commandY, viewAreaW, commandRH);
                    if (!commandArea.isVisible()) {
                        commandArea.setVisible(true);
                    }
                    commandArea.validate();
                    messageRH = commandY - messageY;
                    if (messageRH <= 2) {
                        messageRH = 0;
                        messageArea.setVisible(false);
                        messageArea.validate();
                        return;
                    }
                    messageArea.setBounds(viewAreaX, messageY, viewAreaW, messageRH);
                    if (!messageArea.isVisible())
                        messageArea.setVisible(true);
                    messageArea.validate();

                    boolean  isPullup = false;
                    if( prePos > (messageRH+10) ) {
                       isPullup = true;
                       prePos = messageRH;
                    }

                    if (isPullup)
                    {
                        if (messageArea.getAccessibleContext().getAccessibleDescription() != null)
                           messageArea.getAccessibleContext().getAccessibleDescription().trim();
                        messageArea.showBottomLine();
                    }
                }  // if (ny != ps.y){
           } // mouseDragged
        }); // cmdResizeBar MouseMotionListener
    }

    public void paintImmediately(int x,int y,int w, int h) {
        if (isDraging && (viewRec2 != null) ) {
            if (x >= viewRec.x) {
                int k = y + h;
                if (k > viewRec2.y) {
                     h = viewRec2.y - y + titleBarH;
                if (h < 1)
                    h = 1;
                }
            }
            viewRec2.y = viewRec.y;
        }
        super.paintImmediately(x, y, w, h);
    }


    /**
     *  Opens the file that has the accelerator keys and the commands,
     *  initiliazes the name, keys and the command vectors.
     *  It then register the specific keys to specific commands read from
     *  the file.
     */
    protected void doKeyBindings()
    {
        m_action = new AbstractAction("keys")
        {
            public void actionPerformed(ActionEvent e)
            {
                String strActCmd = e.getActionCommand();
                if (strActCmd != null)
                    sendToVnmr(strActCmd);
            }
        };

        getInputMap().clear();
        getActionMap().clear();
        String strFilePath = FileUtil.openPath("INTERFACE"+File.separator+
                                                "KeyStrings.txt");
        if (strFilePath != null)
        {
            initVectors(strFilePath);
            registerKeys();
        }
    }

    /**
     *  Registers the specific keys to the specific commands.
     *  It uses the input map and the action map to create the bindings.
     */
    protected void registerKeys()
    {
        // ComponentInputMap im = new ComponentInputMap(this);
        // ActionMap am = new ActionMap();
        for(int i = 0; i < m_vecStrNames.size(); i++)
        {
            KeyStroke key  = KeyStroke.getKeyStroke(m_vecStrKeys.elementAt(i));
            if (key == null)
                continue;
            String strName = m_vecStrNames.elementAt(i);
            String strCmd  = m_vecStrCmd.elementAt(i);

        // Remove previous bindings.
            getInputMap().remove(key);
            getActionMap().remove(strName);
            unregisterKeyboardAction(key);

        // Create new bindings.
            getInputMap().put(key, strName);
            getActionMap().put(strName , m_action);
            registerKeyboardAction(m_action, strCmd, key, JComponent.WHEN_IN_FOCUSED_WINDOW);
        }
    }


    /**
     *  Initializes the name, keys and the commands vector.
     *  The format of the file is as follows:
     *  Name    Keys    Command
     *  Some examples:
     *  "Proton"       F2         "AuH"
     *  "CD2Cl2"       F4         "solvent = 'CD2Cl2'"
     *  "1X1"          shift+F1   "vnmrjcmd('window','1 1')"
     *  "1X2"          shift+F2   "vnmrjcmd('window','1 2')"
     */
    protected void initVectors(String strFilePath)
    {
        String strLine;
        Hashtable<String, String> htKeyName = new Hashtable<String, String>();
        BufferedReader in = openReadFile(strFilePath);
        try
        {
            while((strLine = in.readLine()) != null)
            {
                // Set the name of the key accelerator,
            // which could be name of a menu item or a button.
                int nStartIndex = (strLine.length() > 1 ) ? 1 : 0;
                nStartIndex = strLine.indexOf('"', nStartIndex);
                if (nStartIndex <= 0) continue;
                String strName = strLine.substring(1, nStartIndex);
                m_vecStrNames.add(strName);

                // skip the blank spaces.
                nStartIndex++;
                if (isMoreSpaces(strLine, nStartIndex))
                {
                    while(isMoreSpaces(strLine, nStartIndex) && (nStartIndex < strLine.length()))
                        nStartIndex++;
                }

                // Set the key accelerator.
                int nEndIndex = getIndex(strLine, nStartIndex);
                if (nEndIndex < 0)
                {
                    m_vecStrNames.removeElement(strName);
                    continue;
                }
                String strKey = strLine.substring(nStartIndex, nEndIndex);
                m_htNameKey.put(strName, strKey);
        // if the key accelerator is of the form cntrl+F4,
        // then change the key to "cntrl F4" by replacing the '+' with a ' '.
        strKey = strKey.replace('+', ' ');
            m_vecStrKeys.add(strKey);

        // Set the command for the key accelerator.
        nEndIndex = strLine.indexOf('"', nEndIndex) + 1;
        int nCmdIndex = strLine.indexOf('"', nEndIndex);
        if (nEndIndex > strLine.length() || nCmdIndex <= 0)
        {
            m_vecStrNames.removeElement(strName);
            m_vecStrKeys.removeElement(strKey);
            m_htNameKey.remove(strName);
            htKeyName.remove(strKey);
            continue;
        }
        m_vecStrCmd.add(strLine.substring(nEndIndex, nCmdIndex).trim());

        // Check to see if the current key was binded to another name before,
        // if so then remove the name-key relationship from m_htNameKey
        // because the current key is being binded to something else.
           String strPrevName = (String)htKeyName.get(strKey);
           if (strPrevName != null && !strPrevName.equals(strName))
              m_htNameKey.remove(strPrevName);
           htKeyName.put(strKey, strName);
        }
        // in.close();
      }
      catch (Exception e)
      {
        String strError = "Error reading file: "+ strFilePath;
        Messages.writeStackTrace(e, strError);
        Messages.postError(strError);
      }
      finally {
           try {
                if (in!= null)
                   in.close();
           } catch (Exception ex) {}
       }
    }

    /**
     *  Checks if there is a space at the given index in the string.
     *  @param strLine  the string that could contain blank spaces.
     *  @param  nIndex  the index where a blank space might exist.
     *  @return         true if there is a space at nIndex in strLine,
     *                  false otherwise.
     */
    private boolean isMoreSpaces(String strLine, int nIndex)
    {
       return (strLine.charAt(nIndex) == ' ' || strLine.charAt(nIndex) == '\t');
    }

    /**
     *  Returns the index for a given string.
     *  @param strLine      the string.
     *  @param nStartIndex  the index from where the search should begin.
     *  @return             the index of either ' ' or '\t' in the string.
     */
    private int getIndex(String strLine, int nStartIndex)
    {
    int nTmp1 = Math.max(strLine.indexOf(' ', nStartIndex), 0);
    int nTmp2 = Math.max(strLine.indexOf('\t', nStartIndex), 0);
    int nIndex = 0;
    if (nTmp1 == 0)         // there are no tabs in the string.
        nIndex = nTmp2;
    else if (nTmp2 == 0)    // there are no blank spaces in the string.
        nIndex = nTmp1;
    else                    // there are both tabs and blank spaces.
        nIndex =  Math.min(nTmp1, nTmp2);
    return nIndex;
    }


    /**
     * Opens the file for reading.
     * @param strPath   the path where the file is located.
     * @return BufferedReader the BufferedReader that would read the file.
     */
    public BufferedReader openReadFile(String strPath)
    {
       BufferedReader in = null;
       try
       {
        if (strPath != null)
            in = new BufferedReader(new FileReader(strPath));
       }
       catch (FileNotFoundException e)
       {
        //e.printStackTrace(System.err);
        Messages.writeStackTrace(e);
       }
       return in;
    }

   private void layoutMainPanels(int x, int y, int w, int h) {
       int x0 = x;
       int x1 = x0;
       int x2;
       int x3 = x + w;

      // vertical partitions
       int v2, w1;
       Dimension dim;

       topBarY2 = y;
       if (expViewArea == null)
           return;

       v2 = y + h;

       dim = commandArea.getPreferredSize();
       commandH = dim.height + 4;
       if (commandH > 40)
          commandH = 40;

       // calculate position of pulse handle
       messageY = topBarY2;
       if (viewAreaW == 0) { // the very beginning
           handleY = topBarY2;
       }
       if (!commandArea.isVisible()) {
           // commandY = topBarY2 - 2;
           commandY = topBarY2;
           commandRH = 0;
       }
       else {
           commandY = messageY + messageRH;
           if (commandY > 1)
               commandRH = commandH;
       }
       handleY = commandY + commandRH;
       if (handleY > (statusY - handleH)) {
             handleY = statusY - handleH;
       }

       w1 = paramTabBar.getPreferredSize().width;
       paramTabBar.setBounds(x3 - w1, y, w1, h);
       if (paramTabBar.isVisible())
           x3 = x3 - w1;
       w1 = toolTabBar.getPreferredSize().width;
       toolTabBar.setBounds(x0, y, w1, h);
       if (toolTabBar.isVisible())
           x1 = x0 + w1;

       controlX = (int) ((float)panelW * paramRx);
       controlY = (int) ((float)panelH * paramRy);
       if (controlY < (handleY + handleH))
           controlY = handleY + handleH;
       if (controlX < x1)
           controlX = x1;

       if (!vToolPanel.isVisible() || getLayer(vToolPanel) > LAYER3)
              x2 = x1;
       else
       {
           if (controlX < 6)
              controlX = 6;
           x2 = controlX;
       }

       vToolPanel.setBounds(x1, y, controlX - x1, h);

       int dock1 = 0;
       int dock2 = 0;

       if (displayPalette != null && displayPalette.isVisible())
           dock1 = gToolDock;
       if (csiButtonPalette.isVisible())
           dock2 = csiToolDock;
       if (dock1 == DOCK_EXP_LEFT) {
           dim = displayPalette.getPreferredSize();
           displayPalette.setBounds(x2, topBarY2, dim.width, h);
           x2 = x2 + dim.width;
       }
       if (dock2 == DOCK_EXP_LEFT) {
           dim = csiButtonPalette.getPreferredSize();
           csiButtonPalette.setBounds(x2, topBarY2, dim.width, h);
           x2 = x2 + dim.width;
       }
       viewAreaX = x2;
       viewAreaW = x3 - x2;

       paramPinPan.setBounds( x2, controlY, viewAreaW, v2 - controlY);
       expY2 = controlY;
       if (!paramPinPan.isVisible() || getLayer(paramPinPan) > LAYER1)
              expY2 = v2;
       if (dock1 == DOCK_BELLOW_EXP) {
           dim = displayPalette.getPreferredSize();
           expY2 = expY2 - dim.height;
           displayPalette.setBounds(x2, expY2, viewAreaW, dim.height);
           expY2 = expY2 - 1;
       }
       if (dock2 == DOCK_BELLOW_EXP) {
           dim = csiButtonPalette.getPreferredSize();
           expY2 = expY2 - dim.height;
           csiButtonPalette.setBounds(x2, expY2, viewAreaW, dim.height);
           expY2 = expY2 - 1;
       }

       viewAreaY = handleY;
       if (commandRH > 1) {
           commandArea.setBounds(viewAreaX, commandY, viewAreaW, commandRH);
           if (messageRH > 1)
               messageArea.setBounds(viewAreaX, messageY,viewAreaW, messageRH);
       }
       else
           messageArea.setBounds(viewAreaX, messageY,viewAreaW, 0);
       if (cmdResizeBar.isVisible()) {
          cmdResizeBar.setBounds(viewAreaX, handleY, viewAreaW, handleH);
          cmdSplitBar.setBounds(viewAreaX, handleY, viewAreaW, handleH);
          viewAreaY = handleY + handleH;
       }

       if (dock1 == DOCK_ABOVE_EXP) {
           dim = displayPalette.getPreferredSize();
           displayPalette.setBounds(x2, viewAreaY, viewAreaW, dim.height);
           viewAreaY = viewAreaY + dim.height;
       }
       if (dock2 == DOCK_ABOVE_EXP) {
           dim = csiButtonPalette.getPreferredSize();
           csiButtonPalette.setBounds(x2, viewAreaY, viewAreaW, dim.height);
           viewAreaY = viewAreaY + dim.height;
       }

       viewAreaH = expY2 - viewAreaY;
       expViewArea.setBounds(viewAreaX, viewAreaY, viewAreaW, viewAreaH);
              /* Record current positons of commandY and messageY */
       if (protocolPinPan != null && protocolPinPan.isVisible()) {
           protocolPinPan.setSize(viewAreaW, viewAreaH);
           protocolBrowser.updateSize();
       }
       if ( bLayout ) {
             bLayout = false;
             expViewArea.setResizeStatus(false);
       }
   }

   private int layoutSysToolBar(int x, int y, int w, int spH, int spIndex) {
        int toolH = sysToolPan.getPreferredSize().height;
        if (toolH < 24)    // 24 is the average of icon height
            toolH = sysToolH;
        else
            sysToolH = toolH;
        sysToolPan.setBounds(x, y, w, toolH);
        JSeparator sp = getSeparator(spIndex);
        y += toolH;
        sp.setBounds(x, y, w, spH);
        sp.setVisible(true);
        y += spH;  
        return y;
   }

   public JSeparator getSeparator(int index) {
           int len = separatorList.size();
           if (len <= index)
                separatorList.setSize(index + 4);
           JSeparator sp = (JSeparator) separatorList.elementAt(index);
           if (sp == null) {
                sp = new JSeparator();
                // sp.setBackground(Util.getSeparatorBg());
                separatorList.setElementAt(sp, index);
                add(sp);
           }
           return sp;
   }

   public void clearSeparator(int index) {
           int len = separatorList.size();
           for (int k = index; k < len; k++) {
               JSeparator sp = (JSeparator) separatorList.elementAt(k);
               if (sp != null)
                   sp.setVisible(false);
           }
   }

   public void setDividerMoving(boolean b, PushpinIF obj) {
        bDividerMoving = b;
        dividerObj = obj;
   }

   public void setDividerColor(Color c) {
        dividerColor = c;
   }

   public void paint(Graphics g) {
       super.paint(g);
       if (!bDividerMoving || dividerObj == null)
          return;
       Rectangle r = dividerObj.getDividerBounds();
       if (r == null || r.width < 1)
          return;
       g.setColor(dividerColor);
       g.fillRect(r.x, r.y, r.width, r.height);
   }

    private class VnmrjUILayout implements LayoutManager {

       public void addLayoutComponent(String name, Component comp) {}

       public void removeLayoutComponent(Component comp) {}

       public Dimension preferredLayoutSize(Container target) {
          int h = 0;
          if (topMenuBar.isVisible())
              h = topMenuBar.getPreferredSize().height+1;
          if (sysToolPan.isVisible())
              h = h + sysToolPan.getPreferredSize().height+1;
          if (usrToolBar.isVisible()) {
             int h1 = usrToolBar.getPreferredSize().height + 1;
             if (h1 < 10)
                h1 = 28;
             h += h1;
          }
          return new Dimension(500, h); // unused
       } // preferredLayoutSize()

       public Dimension minimumLayoutSize(Container target) {
           return new Dimension(0, 0); // unused
       } // minimumLayoutSize()


       public void layoutContainer(Container target) {
         if (isDraging || bVpAction) {
              return;
         }
         if (bLogout)
              return;
         if (bGToolMoving)
              return;
         synchronized (target.getTreeLock()) {
              Dimension tdim = target.getSize();
              Insets insets = target.getInsets();
              Dimension dim;
              int x0 = insets.left;
              int y0 = insets.top;
              int vh, dw;
              int bw = 8;
              int spIndex = 0;
              int spH = 2;
              int tWidth = tdim.width - x0 - insets.right;
              int tHeight = tdim.height - insets.top - insets.bottom;
              JSeparator sp;
              Component obj;
              Point pt;

              if (tWidth < 20 || tHeight < 20)
                   return;
              panelW = tWidth;
              panelH = tHeight;
              if (helpOverlayPanel != null) {
                   helpOverlayPanel.setBounds(0, 0, tWidth, tHeight);
                   if (helpOverlayPanel.isVisible())
                       helpOverlayPanel.requestFocus();
              }
              sysToolH = sysToolPan.getPreferredSize().height;
              if (topMenuBar.isVisible()) {
                   vh = topMenuBar.getPreferredSize().height;
                   if (vh < 12) // not a reasonable number
                      vh = topMenuH;
                   else
                      topMenuH = vh;
                   topMenuBar.setBounds(x0, y0, tWidth, vh);
                   y0 = y0 + vh;
                   sp = getSeparator(spIndex);
                   spIndex++;
                   sp.setBounds(x0, y0, tWidth, spH);
                   sp.setVisible(true);
                   y0 = y0 + spH;
              }
              if (sysToolAtTop > 0 && sysToolPan.isVisible()) {
                   y0 = layoutSysToolBar(x0, y0, tWidth, spH, spIndex);
                   spIndex++;
              }
              if (usrToolBar.isVisible() && !sysToolPan.hasMiddleToolBar()) {
                   dim = usrToolBar.getPreferredSize();
                   vh = dim.height;
                   if (vh < 16) { // it is empty
                       vh = 24;
                   }
                   if (vh > 60)
                       vh = 60;
                   usrToolBar.setBounds(x0, y0, tWidth, vh);
                   usrToolH = vh;
                   y0 = y0 + vh;
                   sp = getSeparator(spIndex);
                   spIndex++;
                   sp.setBounds(x0, y0, tWidth, spH);
                   sp.setVisible(true);
                   y0 = y0 + spH;
              }
              if (sysToolAtTop <= 0  && sysToolPan.isVisible()) {
                   y0 = layoutSysToolBar(x0, y0, tWidth, spH, spIndex);
                   spIndex++;
              }

              if (statusBar != null && statusBar.isVisible()) {
                   vh = statusBar.getPreferredSize().height;
                   tHeight = tHeight - vh;
                   statusBar.setBounds(bw+x0, tHeight, tWidth - bw, vh);
                   statusButPane.setBounds(x0, tHeight, bw, vh);
                   statusBut.setPreferredSize(new Dimension(bw, vh));
              }
              statusY = tHeight;

              clearSeparator(spIndex);

              gToolDock = DOCK_EXP_RIGHT;
              csiToolDock = DOCK_NONE;
              if (graphicsToolBar != null) {
                 gToolDock = graphicsToolBar.getDockPosition();
                 if (gToolDock > DOCK_BELLOW_EXP) {
                    graphicsToolBar.setDockPosition(DOCK_NONE);
                    gToolDock = DOCK_NONE;
                 }
              }
              if (csiButtonPalette.isVisible()) {
                 csiToolDock = csiButtonPalette.getDockPosition();
                 if (csiToolDock > DOCK_BELLOW_EXP) {
                    // csiButtonPalette.setDockPosition(DOCK_NONE);
                    csiToolDock = DOCK_NONE;
                 }
                 if (gToolDock == csiToolDock) {
                    if (csiToolDock == DOCK_EXP_LEFT) {
                        graphicsToolBar.setDockPosition(DOCK_EXP_RIGHT);
                        gToolDock = DOCK_EXP_RIGHT;
                    }
                    else if (gToolDock == DOCK_EXP_RIGHT) {
                        csiButtonPalette.setDockPosition(DOCK_EXP_LEFT);
                        csiToolDock = DOCK_EXP_LEFT;
                    }
                 }
              }
 
              if (graphicsToolBar != null && graphicsToolBar.isShow()) {
                 graphicsToolBar.setDockTopPosition(y0);
                 graphicsToolBar.setDockBottomPosition(tHeight);
                 obj = (Component)graphicsToolBar;
                 dim = obj.getPreferredSize();
                 dw = dim.width;
                 pt = new Point(0, 0);
                 if (gToolDock > DOCK_NONE) {
                    if (graphicsToolBar instanceof JWindow) {
                       if (target.isShowing())
                          pt = target.getLocationOnScreen();
                    }
                    if (gToolDock == DOCK_FRAME_LEFT) { // left side
                       graphicsToolBar.setBounds(x0+pt.x, y0+pt.y-1, dw, tHeight - y0 + 1);
                       x0 = x0 + dw;
                       tWidth = tWidth - dw;
                    }
                    if (gToolDock == DOCK_EXP_RIGHT) { // right side
                       graphicsToolBar.setBounds(tWidth-dw+pt.x, y0+pt.y-1, dw, tHeight - y0 + 1);
                       tWidth = tWidth - dw;
                    }
                 }
                 else if (!(graphicsToolBar instanceof JWindow)) {
                    pt = obj.getLocation();
                    if ((pt.y+dim.height) > statusY) {
                        pt.y = statusY - dim.height;
                    }
                    if (pt.y < 0) {
                        pt.y = 0;
                    }
                    if ((pt.x+ dw) > tWidth) {
                        pt.x = tWidth - dw;
                        if (pt.x < 0)
                           pt.x = 0;
                    }
                    graphicsToolBar.setBounds(pt.x, pt.y, dw, dim.height);
                 }
              }
              if (csiButtonPalette.isVisible()) {
                 csiButtonPalette.setDockTopPosition(y0);
                 csiButtonPalette.setDockBottomPosition(tHeight);
                 dim = csiButtonPalette.getPreferredSize();
                 dw = dim.width;
                 if (csiToolDock > DOCK_NONE) {
                    if (csiToolDock == DOCK_FRAME_LEFT) { // left side
                       csiButtonPalette.setBounds(x0, y0-1, dw, tHeight - y0 + 1);
                       x0 = x0 + dw;
                       tWidth = tWidth - dw;
                    }
                    if (csiToolDock == DOCK_EXP_RIGHT) { // right side
                       csiButtonPalette.setBounds(tWidth-dw, y0-1, dw, tHeight - y0 + 1);
                       tWidth = tWidth - dw;
                    }
                 }
                 else {
                    pt = csiButtonPalette.getLocation();
                    if ((pt.y+dim.height) > statusY) {
                        pt.y = statusY - dim.height;
                    }
                    if (pt.y < 0) {
                        pt.y = 0;
                    }
                    if ((pt.x+dw) > tWidth) {
                        pt.x = tWidth - dw;
                        if (pt.x < 0)
                           pt.x = 0;
                    }
                    csiButtonPalette.setBounds(pt.x, pt.y, dw, dim.height);
                 }
              }

              layoutMainPanels(x0, y0, tWidth, tHeight - y0);
         } // synchronized
       } // layoutContainer
    }

    public void setInputFocus()
    {
        sendToVnmr("isimagebrowser");
    }

    public void setInputFocus(String cmd) {
        if (cmd == null)
           cmd = "";
       cmd = cmd.trim();
       if (helpOverlayPanel != null) {
           if (helpOverlayPanel.isVisible()) {
               helpOverlayPanel.requestFocus();
               return;
           }
       }
       if (cmd.equals("1"))
            requestFocus();
        else
            super.setInputFocus();
    }

    public void processXKeyEvent(KeyEvent e) {
	// overwrite to do nothing (not forward evt to command line).
    }

    public void processKeyPressed(KeyEvent e) {
	if(keyTable == null) return;
	//System.out.println("keyPressed " + e.getKeyText(e.getKeyCode()));
	String key = KeyEvent.getKeyText(e.getKeyCode());
	keyTable.processKey(key,"keyPressed");
    }

    public void processKeyReleased(KeyEvent e) {
	if(keyTable == null) return;
	//System.out.println("keyReleased " + e.getKeyText(e.getKeyCode()));
	String key = KeyEvent.getKeyText(e.getKeyCode());
	keyTable.processKey(key,"keyReleased");
    }

    public void processMousePressed(MouseEvent evt){
	if(keyTable == null) return;
	if(evt.getButton() != MouseEvent.BUTTON3) return;
	//System.out.println("showAcceleratorKeys ");
	keyTable.showMenu();
    }

    public void processMouseReleased(MouseEvent evt){
	if(keyTable == null) return;
	if(evt.getButton() != MouseEvent.BUTTON3) return;
	//System.out.println("doAcceleratorKeys ");
	keyTable.dismissMenu();
    }


  class VMouseAdapter extends MouseAdapter implements AWTEventListener
  {

    protected KeyboardFocusManager keyboardfocusmanager = KeyboardFocusManager.getCurrentKeyboardFocusManager();

    public void eventDispatched(AWTEvent e)
    {
        if (e instanceof MouseEvent && !Util.isFocusTraversal())
        {
            if (((MouseEvent)e).getID() != MouseEvent.MOUSE_PRESSED)
                return;

            Object comp = e.getSource();
            Component comp2 = keyboardfocusmanager.getFocusOwner();
            // if the current focus is on textcomponent, and the component being
            // clicked on is not a text component, then the focus would remain
            // in the text component, so to execute vnmrcmd, do focuslost and
            // focusgained on text component
            if (comp2 instanceof JTextComponent && (comp2 instanceof VEntry ||
                comp2 instanceof VTextPane || comp2 instanceof VTextWin ||
                comp2 instanceof VTextMsg) && !comp2.equals(comp) &&
                !(comp instanceof JTextComponent))
            {
                JTextComponent txt = (JTextComponent)comp2;
                FocusEvent fe = new FocusEvent(txt, FocusEvent.FOCUS_LOST);
                txt.dispatchEvent(fe);
                fe = new FocusEvent(txt, FocusEvent.FOCUS_GAINED);
                txt.dispatchEvent(fe);
                if (comp2 instanceof VEntry)
                {
                    int n = txt.getCaretPosition();
                    if (n >= 0)
                        txt.select(n,n);
                }
            }
        }
    }
  }

  private class VFrameListener {
      public Component  comp;
      public boolean    bOpened;

      public VFrameListener(Component o) {
          this.comp = o;
          saveStatus();
      }

      public void saveStatus() {
          bOpened = comp.isVisible();
      }

      public void recoverStatus() {
          comp.setVisible(bOpened);
      }
  }

} // class VnmrjUI
