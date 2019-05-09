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
import java.io.*;
import java.util.*;
import java.net.*;

import javax.imageio.ImageIO;
import javax.swing.*;

// import javax.swing.plaf.*;
// import java.awt.peer.*;
// import com.sun.java.swing.plaf.motif.*;
// import sun.awt.*;
// import java.lang.reflect.*;
import java.awt.image.*;
import java.beans.*;
import java.awt.datatransfer.Clipboard;

import javax.print.DocFlavor;
import javax.swing.Timer;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.shuf.*;
import vnmr.templates.*;
import vnmr.lc.*;
import vnmr.cryo.*;
import vnmr.jgl.*;
import vnmr.sms.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;
import vnmr.print.VjPlotterConfig;
import vnmr.sms.SmsInfoObj;

/**
 * The container for experiment windows.
 */

public class ExpPanel extends JPanel
                implements Runnable, SocketIF, VGaphDef, VnmrKey, ButtonIF,
                           VObjDef, VSplitListener, PropertyChangeListener,
                           VjLogListener, StatusManager, ComponentListener
{
//### G3D DEV ##############
    private GLDemo gldemo=null;
    private JGLDialog gldialog=null;
    private JGLComMgr glcom=null;
    private JPanel glPanel = null;
    private JPanel activeSensor = null;
    private int glSize = 50; // the percentage of the size of lcPanel
//################################
    public static String JFUNC = "jFunc(";
    public static String LINE = ")\n";
    /** session share */
    private SessionShare sshare;
    ExpViewIF expIf;
    private AppIF appIf;
    private SubButtonPanel  subButPanel;
    private SubButtonPanel  csiButPanel;
    private VMenuBar  vmenuBar;
    private SysToolBar  rToolBar; // right side toolbar
    private SysToolBar  lToolBar; // left side toolbar
    private String   expName = "exp0";
    private String   panelName = null;
    private boolean  sendWinId = true;
    private boolean  needPanelInfo;
    private boolean  panelBusy = false;
    private boolean  vbgBusy = false;
    private boolean  panelReady = false;
    private boolean  readyToRun = false;
    private boolean  bExiting = false;
    private boolean  bAvailable = true;
    private boolean  bSuspend = false;
    private boolean  bSuspendPnew = false;
    private boolean  bAlive = true;
    private boolean  bNative = true; // native graphics
    private boolean  bShowFlag = true; // the flag of visible
    private boolean  useBusyBar = true;
    private boolean  useBusyIcon = false;
    private boolean  bPlot = false;
    private boolean  bReopenTool = false;
    private boolean  bVerbose = false;
    private boolean  sqBusy = false;
    private boolean  bCsiActive = false;
    private boolean  bCsiOpened = false;
    private static   boolean  bDebugCmd = false;
    private static   boolean  bDebugPnew = false;
    private static   boolean  bDebugUiCmd = false;
    private static   boolean  bDebugQuery = false;
    private static   boolean  bSendInfo = false;
    private static   boolean  bSmsDebug;
    private static   boolean  debug = false;
    private static   boolean  m_sqIgnoreInactiveVp = true;
    private static   String   rfMonStr;
    private static java.util.List <StatusListenerIF> statusListenerList = null;
    private static java.util.List <ExpListenerIF> expListenerList = null;
    private static java.util.Map <String,String> statusMessages = null;
    private static java.util.List <VLimitListenerIF> m_aListLimitListener = null;
    private static MasStatusListener masStatusListener=null;
    private static Hashtable <String,ModelessPopup> modelessPopups = null;
    private static ParamArrayPanel paramArrayPanel=null;
    private static JDialog sm_previewPopup = null;
    private static VjPlotterConfig plotConfig;
    private static VnmrPlot vnmrPlot = null;

    // Keep m_appDirLabels for all instantiations
    private static ArrayList<String> m_appDirLabels = new ArrayList<String>();
    
    private static UserRightsPanel profilePanel = null;
    private static ExpSelTabEditor expSelTabEditor = null;
    private static ExpSelTreeEditor expSelTreeEditor = null;
    private static String m_showTrayCmd = null;
    private static String m_hideTrayCmd = null;
    private static LocatorPopup locatorPanel=null;
    private static ProtocolBrowser protocolBrowser=null;
    private static VnmrJLogWindow logWindow = null;
    private static DicomConf dicomConfigPanel = null;
    private static LogToCSVPanel logToCSVPanel = null;

    private java.util.List <ExpPanInfo> paramPanelList = null;
    private java.util.List <VTextFileWin> alphatextList = null;
    private ShufflerParam  shufParam = null;
    private ControlPanel  controlPanel = null;
    private ParameterTabControl paramTabCtrl;
    private Vector<ExpPanInfo> panVector;
    private String  sarrayVal[];
    // private String  arrayVal[];
    private Vector <JButton>  buttonList;
    private ModalPopup modalPopup=null;
    private String m_strVpxmlfile;
    //private UserDirectoryDialog usrdir = null;
    protected ParamPanelId m_paramPanelId;
    protected LoginBox m_loginbox;
    protected VPrint m_objPrint;
    protected JComponent vjmol;
    protected String m_strVJmolImagePath;
    protected String m_strMolColor = "black";
    protected VJButton m_btnVJmol = null;
    protected QueuePanel qPanel = null;
    private LcControl m_lcControl = null;
    private VContainer lcPanel = null;
    private MsCorbaClient pmlClient = null;
    private CryoControl m_cryoControl = null;
    private VSeparator vpBorder = null;
    private MouseAdapter ml;
    private VpLayoutInfo vpLayoutInfo = null;
    // private static String vpmacro="vpaction";
    private VButton trayButton = null;
    private TrayObj trayInfo = null;
    private String p11Tip = null;
    private String plotFile = null;
    // private String plotDpi = "120";
    private String plotFormat = "jpeg";
    private String plotSize = "page";  // page, halfpage, quarterpage
    private String plotOrient = "portrait"; // portrait or landscape
    private String plotRegion = "graphics"; // graphics, frames, allframes, vnmrj
    private String plotter = null;
    public  String plotCmd = null;
    private char charValue[];
    private ImageIcon p11Icon = null;
    private Timer vnmrInitTimer = null;
    private Timer vnmrbgTimer = null;
    private Timer busyTimer = null;
    private Timer printTimer = null;
    private Timer sqTimer = null;
    private VPrintDialog printDialog;
    private HelpBar  helpBar;
    private ArrayList<String> m_queueSendToVnmrCmds = new ArrayList<String>();
    private ArrayList<String> m_appDirs = new ArrayList<String>();

    private int    charCapacity = 0;
    private int    iLcPos = VSeparator.SOUTH; // the position of lcPanel
    private int    iLcSize = 50; // the percentage of the size of lcPanel
    private int    buttonNum = -1;
    private int    paramPanelListLimit = 12;
    private int    queryCount = 0;
    private int    sarraySize = 0;
    private int    sqCount = 0;
    private int    timerCount = 0;
    private int    csiDividerOrient = SwingConstants.VERTICAL;
    private float  csiDividerLoc = 0.5f;
    private float  glDividerLoc = 0.4f;
    private ExpInfoObj apptypeObj = null;

    private static final int APPTYPE = 1;
    // private VDivider glDivider = null;
    private VDivider csiDivider = null;
    private ImageViewPanel imgPane;

    public final static String GLPANE = "glWindow";
    public final static String CSIPANE = "csiWindow";
    public final static String CSIDIVIDER = "csiDivider";
    public final static String GLDIVIDER = "glDivider";

    /**
     * constructor
     * @param sshare session share
     */
    public ExpPanel(SessionShare sshare, ExpViewIF ep, AppIF ap, int num) {
        this.sshare = sshare;
        this.expIf = ep;
        this.appIf = ap;

        winId = num;
        // vpmacro = Util.getLabel("mVPaction","vpaction");

        // setBackground(DisplayOptions.getColor("graphics20"));
        if (statusListenerList == null) {
            statusListenerList = Collections.synchronizedList(new LinkedList<StatusListenerIF>());
        }
        if (expListenerList == null) {
            expListenerList = Collections.synchronizedList(new LinkedList<ExpListenerIF>());
        }
        if (statusMessages == null) {
            statusMessages = Collections.synchronizedMap(new HashMap<String,String>());
        }
        if (m_aListLimitListener == null)
            m_aListLimitListener = Collections.synchronizedList(new ArrayList<VLimitListenerIF>());
        alphatextList = Collections.synchronizedList(new LinkedList<VTextFileWin>());
        if (paramPanelList == null) {
            try {
                paramPanelListLimit =
                    Integer.parseInt(System.getProperty("savepanels"));
            } catch (NumberFormatException nfe) {
                paramPanelListLimit = 12;
            }
            if (paramPanelListLimit < 5) {
                paramPanelListLimit = 5;
            } else if (paramPanelListLimit > 30) {
                paramPanelListLimit = 30;
            }
            paramPanelList = new ArrayList<ExpPanInfo>(paramPanelListLimit);
        }

        String busy = System.getProperty("useBusyIcon");
        if (busy != null) {
            if (busy.startsWith("y")) {
                useBusyIcon=true;
            } else {
                useBusyIcon=false;
            }
        }
        busy = System.getProperty("useBusyBar");
        if (busy != null) {
            if (busy.startsWith("y")) {
                useBusyBar=true;
            } else {
                useBusyBar=false;
            }
        }
        if (useBusyBar)
            useBusyIcon = false;
        // Get the host name
        try {
            InetAddress inetAddress = InetAddress.getLocalHost();
            localHostName = inetAddress.getHostName();
        }
        catch(Exception e) {
            Messages.postWarning("Error getting HostName");
            localHostName = new String("");
        }

        // setLayout(new canvasLayout());

        bNative = Util.isNativeGraphics();

        vsocket = new CanvasSocket(this, winId);
        vsocketThread = new Thread(vsocket);
        socketPort = vsocket.getPort();

        vsocketThread.start();

/*
        border = (Border) new CanvasBorder(Color.white, 1);
        setBorder(border);
*/
        vcanvas = new VnmrCanvas(this.sshare, winId, this, bNative);
        buttonPan = new JPanel();
        buttonPan.setOpaque(false);
        buttonPan.setLayout(new buttonPanLayout());
        // makeTitleBar();
        buttonList = new Vector<JButton>();
        resizeBut = new VJButton(Util.getImageIcon("resizeWin.gif"));
        tearBut = new VJButton(Util.getImageIcon("twinWin.gif"));

        busyBut = new JButton(Util.getImageIcon("busy.gif"));
        busyBut.setOpaque(false);
        busyBut.setVisible(false);
        busyBut.setMargin(new Insets(0,0,0,0));
        busyBut.setContentAreaFilled(false);
        busyBut.setBorder(BorderFactory.createEmptyBorder());

        buttonPan.add(resizeBut);
        if (useBusyIcon)
            buttonPan.add(busyBut);
        activeSensor = new JPanel();
        activeSensor.setOpaque(false);
        helpBar = new HelpBar();
        // helpBar.setVisible(false);

        makeTitleBar();
/*
        add(tearBut);
*/
        if (showTitleBar)
            add(titleComp);
        else
            add(buttonPan);
        buttonList.add(resizeBut);
/*
        buttonList.add(tearBut);
*/
        add(vcanvas);
        queryQsize = 100;
        queryQ = new Vector<QueryData>(queryQsize);
        subButPanel = new SubButtonPanel(this);
        vmenuBar = new VMenuBar(this.sshare, this, winId);
        rToolBar = new SysToolBar(this.sshare, this, winId);
        lToolBar = new SysToolBar(this.sshare, this, winId, "SysToolMenu.xml");
        //initialize();
        
//### G3D DEV ##############      
        glPanel = new JPanel();
        add(glPanel);
        glPanel.setVisible(false);

        //glcom=new JGLComMgr(this);
        //gldialog = new JGLDialog(glcom,this,sshare);
//########################

        lcPanel = new VContainer(this.sshare, this, "lcPanel");
        add(lcPanel);
        lcPanel.setVisible(false);

        vpBorder = new VSeparator(this);
        vpBorder.setListener(this);
        add(vpBorder, 0);
        initLcInfo();

        // NB: This no longer works, somebody seems to clear the listener list.
        // Instead, call addExpListener() in the layout manager to make sure
        // it's there whenever the lcPanel is visible.
        //if (lcPanel instanceof ExpListenerIF)
        //    addExpListener(lcPanel);

        resizeBut.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                // expIf.smallLarge(winId);
                if (appIf.setSmallLarge(winId))
                    sendSetActive();
            }
        });

        tearBut.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                createTearFrame();
            }
        });

        /*dpsBut.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                showDpsFrame();
            }
        });*/

        // Add listeners to start VnmrBG when panels appear.
        // (Window must be shown for VBG to get the XWindows ID.)
/*
        vcanvas.addComponentListener(this);
        addComponentListener(this);
*/

        // install dynamic behaviors
        addComponentListener(new ComponentAdapter() {
            public void componentResized(ComponentEvent evt) {
                Dimension dim = getSize();
                Insets insets = getInsets();
                int     y = insets.top;
                if (titleComp.isVisible()) {
                    Rectangle rc = titleComp.getBounds();
                    y = rc.y + rc.height;
                }
                yGap = y;
                if (!panelReady) {
                   if (dim.width > 0) {
                      panelReady = true;
                      mayRunVnmr();
                   }
                }
            }

            public void componentShown(ComponentEvent evt) {
                /*StringBuffer sbcmd = new StringBuffer().append("M@event jFunc(").
                                          append(XSHOW).append(", 1)\n");*/
                String str = "M@event jFunc("+XSHOW+", 1)\n";
                sendToVnmr(str);
            }

            public void componentHidden(ComponentEvent evt) {
                /*StringBuffer sbcmd = new StringBuffer().append("M@event jFunc(").
                                          append(XSHOW).append(", 0)\n");*/
                String str = "M@event jFunc("+XSHOW+", 0)\n";
                sendToVnmr(str);
            }
        });

        ml = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                  requestActive();
             }
        };
        titleComp.addMouseListener(ml);
        activeSensor.addMouseListener(ml);
        vpLayoutInfo = new VpLayoutInfo(winId);
        panelName = vpLayoutInfo.getPanelName();

        setLayout(new canvasLayout());
        DisplayOptions.addChangeListener(this);

        // This needs to be instantiated someplace and it must be after
        // addStatusListener() is available, so put it here.
        masStatusListener = new MasStatusListener(this);

        // See MasStatusListener, but now for Gradient Coil ID
        new GradCoilIdListener(this);

        vcanvas.setVjColors();
/*
        if (!bNative) {
            panelReady = true;
            mayRunVnmr();
        }
*/
        trayButton = ((ExpViewArea)expIf).getTrayButton();
        createBusyTimer();

        if (vcanvas.bImgType && winId > 0) {
            // apptypeObj = new ExpInfoObj("$VALUE=apptype", APPTYPE);
            // apptypeObj.setExpPanel(this);
        }
        initCsiPanel();
        actCanvas = vcanvas;
        aipCanvas = vcanvas;
        paramTabCtrl = new ParameterTabControl(winId, sshare, this);
        if (panelName != null)
           paramTabCtrl.setPanelName(panelName);
        else
           paramTabCtrl.getPanelName();
        // if (Util.isMultiSq())
        //     m_sqIgnoreInactiveVp = false;
        setSize(400, 400);
    } // ExpPanel()
    
    public int getWinId(){
    	return winId;
    }


    public void propertyChange(PropertyChangeEvent evt)
    {
/*
        titleBg = Util.getBgColor();
        if (!titleActive)
            titleComp.setBackground(titleBg);

        Color color = Util.getMenuBarBg();
        if (vmenuBar != null)
            vmenuBar.setBackground(color);
*/

        setTitleColor();
        // setBackground(DisplayOptions.getColor("graphics20"));
        if (vcanvas !=null)
            vcanvas.setVjColors();
        if (panVector == null)
            return;
        if(DisplayOptions.isUpdateUIEvent(evt)){
            int nLength = panVector.size();
            for (int k = 0; k < nLength; k++) {
                  ExpPanInfo info = (ExpPanInfo)panVector.elementAt(k);
                  if (info != null) {
                      if (info.paramPanel != null)
                         SwingUtilities.updateComponentTreeUI(info.paramPanel);
                      if (info.actionPanel != null)
                         SwingUtilities.updateComponentTreeUI(info.actionPanel);
                  }
            }
            if (vmenuBar != null)
                  SwingUtilities.updateComponentTreeUI(vmenuBar);
            if (rToolBar != null)
                  SwingUtilities.updateComponentTreeUI(rToolBar);
            if (lToolBar != null)
                  SwingUtilities.updateComponentTreeUI(lToolBar);
            if (trayButton != null)
                  SwingUtilities.updateComponentTreeUI(trayButton);
            if (fpPanel != null)
                  SwingUtilities.updateComponentTreeUI(fpPanel);
            if (buttonPan != null) {
                  int n = buttonPan.getComponentCount();
                  for (int i = 0; i < n; i++) {
                       JComponent comp = (JComponent) buttonPan.getComponent(i);
                       if (comp != null)
                            comp.setOpaque(false);
                  }
            }
        }
    }

    public static void clearStaticListeners() {
        if (statusListenerList != null)
           statusListenerList.clear();
        if (expListenerList != null)
           expListenerList.clear();
        // if (m_aListLimitListener != null)
        //    m_aListLimitListener.clear();

    }

    public void rebuildUI(SessionShare sessionsshare, AppIF ap) {
        this.sshare = sessionsshare;
        appIf = ap;
        vmenuBar = new VMenuBar(this.sshare, this, winId);
        rToolBar = new SysToolBar(this.sshare, this, winId);
        lToolBar = new SysToolBar(this.sshare, this, winId, "SysToolMenu.xml");
        // panVector = null;
        controlPanel = null;
        // locatorPanel = null;
        masStatusListener = new MasStatusListener(this);
        new GradCoilIdListener(this);
        if (alphatextList != null)
           alphatextList.clear();
        vcanvas.rebuildUI(sshare, ap);
        // ExpSelector.setForceUpdate(true);
        paramTabCtrl.buildTopButtons();
        setTabPanels(paramTabCtrl.getPanelTabs());
    }

    public static void setShowRFmon(int a) {
        bSendInfo = true;
	if (a>0) {
            rfMonStr = new String("exists('showrfmon','parameter','global'):$e if ($e>0) then showrfmon=1 endif");
        }
        else
            rfMonStr = new String("exists('showrfmon','parameter','global'):$e if ($e>0) then showrfmon=-1 endif");
    }

    public JButton getDpsButton()
    {
        if (dpsBut == null)
        {
            dpsBut = new VJButton(Util.getImageIcon("dps.gif"));
            dpsBut.setVisible(false);
            add(dpsBut, 1);
            buttonList.add(dpsBut);
            buttonPan.add(dpsBut);
            dpsBut.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    showDpsFrame();
                }
            });
        }
        return dpsBut;
    }

    public JButton getFdaButton()
    {
        if (fdaBut == null)
        {
            fdaBut = new JButton(Util.getImageIcon("blueFDA.gif"));
            fdaBut.setOpaque(false);
            fdaBut.setMargin(new Insets(0, 0, 0, 0));
            fdaBut.setContentAreaFilled(false);
            fdaBut.setBorder(BorderFactory.createEmptyBorder());
            fdaBut.setVisible(false);
            add(fdaBut, 1);
            buttonList.add(fdaBut);
            buttonPan.add(fdaBut);
        }
        return fdaBut;
    }

    public JButton getDirButton()
    {
        if (dirBut == null)
        {
            dirBut = new JButton(Util.getImageIcon("blank.gif"));
            dirBut.setOpaque(false);
            dirBut.setMargin(new Insets(0, 0, 0, 0));
            dirBut.setContentAreaFilled(false);
            dirBut.setBorder(BorderFactory.createEmptyBorder());
            dirBut.setVisible(false);
            add(dirBut, 1);
            buttonList.add(dirBut);
            buttonPan.add(dirBut);
        }
        return dirBut;
    }

    public JButton getLinkButton()
    {
        if (linkBut == null)
        {
            linkBut = new JButton(Util.getImageIcon("blank.gif"));
            linkBut.setOpaque(false);
            linkBut.setMargin(new Insets(0, 0, 0, 0));
            linkBut.setContentAreaFilled(false);
            linkBut.setBorder(BorderFactory.createEmptyBorder());
            linkBut.setVisible(false);
            add(linkBut, 1);
            buttonList.add(linkBut);
            buttonPan.add(linkBut);
        }
        return linkBut;
    }

    public JButton getCheckButton()
    {
        if (checkBut == null)
        {
            checkBut = new JButton(Util.getImageIcon("blank.gif"));
            checkBut.setOpaque(false);
            checkBut.setMargin(new Insets(0, 0, 0, 0));
            checkBut.setContentAreaFilled(false);
            checkBut.setBorder(BorderFactory.createEmptyBorder());
            checkBut.setVisible(false);
            add(checkBut, 1);
            buttonList.add(checkBut);
            buttonPan.add(checkBut);
        }
        return checkBut;
    }

    public VpLayoutInfo getVpLayoutInfo() {
         return vpLayoutInfo;
    }

    public JComponent getVJMol()
    {
        return vjmol;
    }

    public void setVJMol(JComponent mol)
    {
        vjmol = mol;
    }

    public String getVJMolImagePath()
    {
        return m_strVJmolImagePath;
    }

    public void setVJMolImagePath(String strPath)
    {
        m_strVJmolImagePath = strPath;
    }

    public VnmrCanvas getCanvas()
    {
        return vcanvas;
    }

    public String getExpName() {
        return expName;
    }
    
    public ExpViewIF getExpView() {
    	return expIf;
    }

    public int getTitleBarHeight() {
        int h = 0;
        if (titleComp != null)
            h = titleComp.getBounds().height;
        return h;
    }

   /*********
    public QueuePanel getQueuePanel() {
        if (qPanel == null) {
            qPanel = SqContainer.createStudyQueue(this);
            if (qPanel != null)
                qPanel.updateValue();
        }
        return qPanel;
    }
   *********/


    public void sendSetActive() {
        // String cmd=vpmacro+"('click',"+(winId+1)+")";
        if (!bAlive)
           return;
        int prevVp = appIf.getVpId() + 1;
        if (!appIf.setViewPort(winId, 1))
           return;
        Util.setActiveView(this);
        int newVp = winId + 1;
        setVpLayout(newVp, prevVp);
    }

    public void setShowFlag(boolean s) {
        bShowFlag = s;
        if (!s) {
            sendToVnmr (new StringBuffer().append(JFUNC).append(XSHOW).append(", 0) ").toString());
        }
    }

    public void setVisible(boolean s) {
        bShowFlag = s;
        if (s) {
            if (isShowing())
               sendToVnmr (new StringBuffer().append(JFUNC).append(XSHOW).append(", 1) ").toString());
        }
        super.setVisible(s);
    }

    public void requestActive() {
        if (!active)
            sendSetActive();
    }

    public void setInputFocus() {
        appIf.setInputFocus();
    }

    public void processMousePressed(MouseEvent e) {
        appIf.processMousePressed(e);
    }

    public void processMouseReleased(MouseEvent e) {
        appIf.processMouseReleased(e);
    }

    public void mouseProc(MouseEvent ev, int release, int drag) {
        int  modify, but, bx, by;

        modify = ev.getModifiers();

        but = 0;
        gok = false;
        if ((modify & (1 << 1)) != 0) // Control key pressed
        {    return; }
        else {
             if ((modify & (1 << 2)) != 0) // button 3
                but = 2;
             else if ((modify & (1 << 3)) != 0)
                but = 1;
        }
        bx = ev.getX() - xGap;
        by = ev.getY() - yGap;
        if (by <= 0)
                return;
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(VMOUSE).
                                  append(",").append(but).append(", ").append(drag).
                                  append(", ").append(release).append(", ").
                                  append(bx).append(", ").append(by).append(LINE);
        //String mess = "jFunc("+VMOUSE+","+but+", "+drag+", "+release+", "+bx+", "+by+")\n";
        if (outPort != null) {
                outPort.outPut(sbcmd.toString());
                sbcmd = new StringBuffer().append(JFUNC).append(VSYNC).append(LINE);
                outPort.outPut(sbcmd.toString());
        }
    }

    public boolean  isInActive() {
        return active;
    }

    private void  setTitleColor() {
        titleBg = Util.getInactiveBg();
        titleFg = Util.getInactiveFg();
        // titleHiBg = Util.getActiveBg();
        // titleHiFg = Util.getActiveFg();
        // titleHiBg = new Color(0, 133, 213); // Agilent blue

        titleHiBg = new Color(6, 104, 163);
        titleHiFg = Color.white;

        if (titleComp == null)
            return;
        if (titleActive) {
           titleComp.setBackground(titleHiBg);
           titleComp.setForeground(titleHiFg);
        }
        else {
           titleComp.setBackground(titleBg);
           titleComp.setForeground(titleFg);
        }
        titleComp.setBusyColor();
        int k = titleComp.getComponentCount();
        for (int i = 0; i < k; i++) {
            Component comp = titleComp.getComponent(i);
            if (comp instanceof VObjIF) {
                // VObjIF obj = (VObjIF) comp;
                if (titleActive)
                   comp.setForeground(titleHiFg);
                else {
                   comp.setForeground(titleFg);
/*
                   String c = obj.getAttribute(VObjDef.FGCOLOR);
                   if (c != null)
                       obj.setAttribute(VObjDef.FGCOLOR, c);
*/
                }
            }
        }
        if (helpBar != null)
            helpBar.setForeground(new Color(188, 188, 188));
            // helpBar.setForeground(Color.black);
    }

    private void  updateTools(boolean bAll) {
        if (outPort == null)
            return;
        if (vmenuBar != null)
            vmenuBar.updateValue(bAll);
        if (rToolBar != null)
            rToolBar.updateValue();
        if (lToolBar != null)
            lToolBar.updateValue();
        if (active) {
            VjToolBar vt = Util.getToolBar();
            if (vt != null)
                 vt.updateValue();
            if (qPanel != null)
                 qPanel.updateValue();
        }
    }

    // set graphics interactive buttons
    private void setButtonBar() {
        if (!active)
           return;
        if (appIf == null)
           return;
        boolean bCsiButton = ((ExpViewArea)expIf).isCsiButtonVisible();
        if (bCsiButton) {
            if (bCsiOpened) {
                appIf.setCsiButtonBar(csiButPanel);
                appIf.setButtonBar(subButPanel);
            }
            else { // swap button panels
                appIf.setCsiButtonBar(subButPanel);
                appIf.setButtonBar(null);
            }
        }
        else {
            appIf.setButtonBar(subButPanel);
        }
    }

    public void  setActive(boolean f) {
        boolean  oldActive = active;

        // if (vcanvas == null)
        //     return;
        queryCount = 0;
        if (bVerbose)
            System.out.println("Exp "+winId+": set active "+f);

        controlPanel = Util.getControlPanel();
        /******
        if (!f && active) {
             if (controlPanel != null) {
                  panelName = controlPanel.getPanelName();
             }
        }
        ******/

        active = f;
        titleActive = false;
        vcanvas.setActive(active);
        paramTabCtrl.setExpActive(active);  

        if (f) {
            panelName = paramTabCtrl.getPanelName();
            vpLayoutInfo.setPanelName(panelName);
            Util.setActiveView(this);
            Util.setExpDir(expDir);
            ActionHandler vpAH = Util.getVpActionHandler();
            if (vpAH != null) {
                vpAH.fireAction(new ActionEvent(this, 0, String.valueOf(winId+1)));
            }

            // Initially when the viewports come up, only the active viewport panels
            // are built, and the id of the actionbar, and the panel and the filenames
            // of the other viewports are stored, and when the viewport is switched,
            // then build the current panel if not already built.
            /**
            if (m_paramPanelId != null) {
                int[] nArrPanelId = m_paramPanelId.m_nArrPanelId;
                String[] strArrPanelFile = m_paramPanelId.m_strArrPanelFile;
                createParamPanel(nArrPanelId[0], strArrPanelFile[0]);
                createParamPanel(nArrPanelId[1], strArrPanelFile[1]);
            }
            **/
            if (bVerbose)
               System.out.println("Exp "+winId+": query count_0 "+queryCount);
            createParamPanel(-1);
            if (controlPanel != null)
                controlPanel.setParameterTabControl(paramTabCtrl);
            if (bVerbose)
               System.out.println("Exp "+winId+": param panel query count "+queryCount);

            if (!oldActive)
                updateTools(false);
            if (showTitleBar)
                 titleActive = true;

            if (bVerbose)
               System.out.println("Exp "+winId+": tools query count "+queryCount);

            setParamPanels();

            if (appIf != null) {
                // appIf.setButtonBar(subButPanel);
                // if (csiButPanel != null)
                //    appIf.setCsiButtonBar(csiButPanel);
                setButtonBar();
                appIf.setMenuBar(vmenuBar);
                if (appIf instanceof VnmrjIF) {
                   ((VnmrjIF)appIf).setSysToolBar(rToolBar);
                   ((VnmrjIF)appIf).setSysLeftToolBar(lToolBar);
                }
                appIf.validate();
                if(modelessPopups != null) {
                    Enumeration<String> e=modelessPopups.keys();
                    while (e.hasMoreElements()) {
                        ModelessPopup popup = (ModelessPopup) (modelessPopups.get(  e.nextElement()));
                        if (popup != null) {
                            popup.setAppIF(appIf);
                            popup.setVnmrIF(this);
                            if (popup.isVisible()) {
                                popup.buildPanel();
                                popup.updateAllValue();
                            }
                        }
                    }
                }
                if(paramArrayPanel != null) {
                    paramArrayPanel.setVnmrIF(this);
                }
            }
            showTrayButton(true);
        }
        if (showTitleBar) {
            setTitleColor();
        }
        if (bVerbose)
            System.out.println("Exp "+winId+": total query count "+queryCount);

    }

    public void  resumeUI() {
         bSuspend = false;
         bSuspendPnew = false;
         updateExpListeners();
         updateTools(true);
         queryPanelInfo();
         if (active) {
             Messages.postDebug("start UI  ");
             setParamPanels();
             VTabbedToolPanel vToolPanel = Util.getVTabbedToolPanel();
             if(vToolPanel != null) vToolPanel.updateValue();
             ExpSelector.update();
         }
    }

    public void waitLogin(boolean b) {
         bSuspendPnew = b;
         ExpSelector.waitLogin(b);
    }

    public void startLogin() {
         bSuspendPnew = false;
         resetBrowsers();
         ExpSelector.startLogin();
    }

    public void  setBorderAttr(Color c, int thick) {
/*
        if (!this.isVisible())
            return;
        CanvasBorder p = (CanvasBorder) border;
        p.setLineColor(c);
*/
    }

    public void  setGraphicRegion() {
        // to give vnmrbg the region of canvas
        if (vcanvas != null)
            vcanvas.setGraphicRegion();
    }

    public void  setMenuAttr(boolean f) {
        boolean bVis = false;
        if (f) {
            resizeBut.setVisible(true);
            if (isFullWidth)
               bVis = true;
        }
        else {
            resizeBut.setVisible(false);
            bVis = true;
        }
        if (helpBar != null)
             helpBar.setVisible(bVis);
    }

    public boolean isAvailable() {
       return bAvailable;
    }

    public void setAvailable(boolean b) {
       bAvailable = b;
    }

    public void setOverlay(boolean overlay, boolean bottom, boolean top,
        VnmrCanvas topObj) {
        if (overlay && !top) {
             if (titleComp != null)
                titleComp.setVisible(false);
        }
        else {
             if (showTitleBar)
                titleComp.setVisible(true);
        }
        vcanvas.setOverlay(overlay, bottom, top, topObj);
    }

    private void  createTearFrame() {
        if (tearCanvas == null) {
            tearCanvas = new VnmrTearCanvas(vcanvas, this);
        }
        tearCanvas.setVisible(true);
        tearCanvas.setState(Frame.NORMAL);
        tearCanvas.validate();
        if (bNative)
            sendToVnmr(new StringBuffer().append(JFUNC).append(PAINT2).
                       append(LINE).toString());
        else
            tearBut.setVisible(false);
    }

    private void  showDpsFrame() {
        if (dpsPanel == null) {
            dpsPanel = new DpsInfoPan(this, winId);
        }
        dpsPanel.setVisible(true);
        dpsPanel.validate();
    }

    public synchronized void  processGraphData(DataInputStream ins, int type) {
        int     code;
        long    n;
        boolean res;
        VnmrCanvas canvas;

        try {
            code = ins.readInt();
        }
        catch (Exception e) {
            return;
        }

       if (type >= AIPCODE) {
           canvas = aipCanvas;
           type -= AIPCODE;
       }
       else
           canvas = actCanvas;
       res = true;
       try {
          switch (type) {
             case  1:
                res = canvas.gFunc(ins, code);
                break;
             case  2:
                if (code == CLEAR && sendWinId)
                    sendCanvasId();
                res = canvas.g2Func(code);
                break;
             case  3:
                res = canvas.g3Func(ins, code);
                break;
             case  4:
                res = canvas.g4Func(ins, code);
                break;
             case  5:
                res = canvas.g5Func(ins, code);
                break;
             case  6:
                res = canvas.g6Func(ins, code);
                break;
             case  7:
                res = canvas.g7Func(ins, code);
                break;
             case  9:
                res = canvas.g9Func(ins, code);
                break;
//### G3D DEV ##############
            case JGLDef.G3D:
                 if (glcom != null)
                     glcom.graphicsCmd(ins,code);
                break;
//################################
             default:
                res = false;
                break;
          }
       }
       catch (Exception e) {
           Messages.writeStackTrace(e);
       }
       if (!res) {
           try {
                n = ins.available();
                if (n > 0)
                    n = ins.skip(n);
           }
           catch (Exception e) { }
       }
    }

    public synchronized void  processPrintData(DataInputStream ins, int type) {
        int     code;
        long    n;
        boolean res;

        try {
            code = ins.readInt();
        }
        catch (Exception e) {
            return;
        }
        if (type >= AIPCODE)
            type -= AIPCODE;

        if (printCanvas == null) {
            printCanvas = vcanvas.getPrintCanvas();
            if (printCanvas == null) {
                try {
                    n = ins.available();
                    if (n > 0)
                        n = ins.skip(n);
                }
                catch (Exception e) { }
                return;
            }
       }

       res = true;
       try {
          switch (type) {
             case  1:
                res = printCanvas.gFunc(ins, code);
                break;
             case  2:
                res = printCanvas.g2Func(code);
                break;
             case  3:
                res = printCanvas.g3Func(ins, code);
                break;
             case  4:
                res = printCanvas.g4Func(ins, code);
                break;
             case  5:
                res = printCanvas.g5Func(ins, code);
                break;
             case  6:
                res = printCanvas.g6Func(ins, code);
                break;
             case  9:
                res = printCanvas.g9Func(ins, code);
                break;
             default:
                res = false;
                break;
          }
       }
       catch (Exception e) {
           Messages.writeStackTrace(e);
       }
       if (!res) {
           try {
                n = ins.available();
                if (n > 0)
                    n = ins.skip(n);
           }
           catch (Exception e) { }
       }
    }


    public void  processPnew(StringTokenizer  tok, String str) {
        int a, b, c;
        String data;

        if ( bSuspendPnew ) {
            return;
        }

        // Messages.postDebug("pnew", "ExpPanel.processPnew(): " + str);
        queryCount = 0;

        if (!tok.hasMoreTokens())
            return;
        if (paramVector == null) {
            paramVector = new Vector<String>(50);
        }
        else {
            paramVector.clear();
        }
        a = 0;
        try {
            a = Integer.parseInt(tok.nextToken()); // number of params
        }
            catch (NumberFormatException er) { return; }
        b = 0;
        while (tok.hasMoreTokens()) {
            data = tok.nextToken().trim();
            if (data.length() > 0)
                paramVector.add(data);
            b++;
            if (b >= a)
                break;
        }
        if (bDebugPnew) {
            StringBuffer sb = new StringBuffer();
            for (c = 0; c < paramVector.size(); c++)
                sb.append(paramVector.elementAt(c)).append(" ");
            logWindow.append(VnmrJLogWindow.PARAM, winId, null, sb.toString());
        }

        // Construct a vector that contains both param names and value
        // strings.  Assumes values follow the param names and are
        // delimited by two <ESC> characters:
        // (\033\033value1\033\033value2\033\033).
        Vector<String> parValVector= new Vector<String>(paramVector.size() * 2);
        parValVector.addAll(paramVector);
        String delim = "\033\033";
        int delimLen = delim.length();
        int idx = str.indexOf(delim);
        for (c=0; c<a && idx >= 0; ++c) {
            idx += delimLen;    // idx --> Beginning of value
            int idx2 = str.indexOf(delim, idx); // End of value
            if (idx2 < 0) {
                break;          // End of value list
            }
            parValVector.add(str.substring(idx, idx2));
            idx = idx2;
        }

        // Make sure there are exactly "a" value strings
        for ( ; c<a; c++) {
            parValVector.add("\033"); // Indicates missing value
        }
        if (paramVector.size() <= 0)
            return;

        updateExpListeners(paramVector);
        DisplayOptions.updatePnewValues(parValVector);
        // We need to catch expselu and expselnu
        ExpSelector.updatePnewValues(parValVector); 
/*
        if (active) {
            ParamListenerIF actPanel = (ParamListenerIF)Util.getActionPanel();
            if (actPanel != null) {
                actPanel.updateValue(paramVector);
            }
        }
*/

        if (VNMRFrame.usesDatabase()){
            if (shufParam == null) {
                shufParam = new ShufflerParam(this);
            }
            shufParam.updateValue(expName, parValVector);
        }

        // We need to catch when mas thinwall changes states
        masStatusListener.updateValue(parValVector);

        if (panVector != null) {
            updateParamPanels(parValVector); // Names and values to ParamLayout
            if (active) {
                // controlPanel = Util.getControlPanel();
                // if (controlPanel != null)
                //     controlPanel.updateTabValue(paramVector);
                paramTabCtrl.updateTabValue(paramVector);
            }
        }
        if(paramArrayPanel != null)
                paramArrayPanel.updatePnewParams(paramVector);
        /***********
        if(modelessPopups != null)
           for(Enumeration e=modelessPopups.keys(); e.hasMoreElements();) {
                ModelessPopup popup =
                (ModelessPopup)(modelessPopups.get((String)e.nextElement()));
                if(popup.isVisible()) popup.updatePnewParams(parValVector);
           }
        **********/
        ModalPopup.updateModalPopups(parValVector);
        // if (vmenuBar != null)
        //     vmenuBar.updateValue(paramVector);
        if (rToolBar != null)
            rToolBar.updateValue(paramVector);
        if (lToolBar != null)
            lToolBar.updateValue(paramVector);
        // VjToolBar vt = ((ExperimentIF) appIf).getToolBar();
        if (active) {
            if (vmenuBar != null)
                vmenuBar.updateValue(paramVector);
            VjToolBar vt = Util.getToolBar();
            if (vt != null)
                vt.updateValue(paramVector);

            VTabbedToolPanel vToolPanel = Util.getVTabbedToolPanel();
            if(vToolPanel != null)
                vToolPanel.updateValue(parValVector);
            if (qPanel != null)
                qPanel.updateValue(parValVector);

            if (modelessPopups != null) {
               Enumeration<String> e = modelessPopups.keys();
               while (e.hasMoreElements()) {
                   ModelessPopup popup =
                      (ModelessPopup)(modelessPopups.get(e.nextElement()));
                   if (popup != null && popup.isVisible()) {
                        popup.updatePnewParams(parValVector);
                   }
               }
            }
        }
        b = paramVector.size();
        if (trayInfo != null) {
            for (a = 0; a < b; a++ ) {
               if (paramVector.elementAt(a).equals("traymax")) {
                   trayInfo.updateValue();
                   break;
               }
            }
        }
        if (apptypeObj != null) {
            for (a = 0; a < b; a++ ) {
               data = paramVector.elementAt(a);
               if (data.equals("apptype") || data.equals("seqfil")) {
                   apptypeObj.updateValue();
                   break;
               }
            }
        }
        if (bVerbose)
            System.out.println("Exp "+winId+":  total pnew query count "+queryCount);
    }

    private void processButtonData(SubButtonPanel pane,StringTokenizer tok) {
        int a, b, i;

        if (pane == null)
            return;
        if (!tok.hasMoreTokens())
            return;
        b = -9;
        a = -1;
        try {
            b = Integer.parseInt(tok.nextToken());
            if (tok.hasMoreTokens())
               a = Integer.parseInt(tok.nextToken());
        }
        catch (NumberFormatException er) { }

        if (b < 0) {
            if (b == -2) {
                pane.showAllButtons();
            }
            else if (b == -1) {
                pane.removeAllButtons();
                buttonNum = b;
            }
            return;
        }
        if (a < 0)
            return;
        if (!tok.hasMoreTokens())
            return;
        String iconName = " ";
        String tip = "";
        String str = tok.nextToken("\n").trim();
        i = str.indexOf(';');
        if (i >= 0) {
            if (i > 0)
               iconName = str.substring(0, i).trim();
            tip = str.substring(i + 1);
        }
        else {
            tok = new StringTokenizer(str, " ,\n");
            iconName = tok.nextToken().trim();
            if (tok.hasMoreTokens())
                tip = tok.nextToken("\n").trim();
        }
        pane.addButton(iconName, b, a);
        if (b > buttonNum)
            buttonNum = b;
        pane.setButtonTip(tip, b);
    }

    private void setButtonCmd(SubButtonPanel pane,StringTokenizer tok) {
        int  id;

        if (!tok.hasMoreTokens())
            return;
        id = -9;
        try {
            id = Integer.parseInt(tok.nextToken());
        }
        catch (NumberFormatException er) { return; }
        if (id < 0) {
            pane.clearButtonCmds();
            return;
        }
        if (!tok.hasMoreTokens())
            return;
        String cmd = tok.nextToken("\n").trim();
        if (cmd.length() > 0)
            pane.setButtonCmd(cmd, id);
    }

    /** Process socket communications coming back from Vnmrbg
     */
    public void  processMasterData(String  str) {
        StringTokenizer tok = new StringTokenizer(str, " ,\n");
        if (!tok.hasMoreTokens())
            return;
        int a, b, c;
        // Messages.postDebug("master", "master " + str);

      try {
        String  type = tok.nextToken();
        if (type.equals("error")) {
            appIf.setMessageColor(Color.red);
            appIf.appendMessage(str.substring(6)+"\n");
            Messages.postMessage("oe "+str.substring(6));
            return;
        }
        if (type.equals("title")) {
            if (!tok.hasMoreTokens())
              return;
            type = tok.nextToken();
            String data = tok.nextToken("\n").trim();
            updateTitleBar(type, data);
            return;
        }
        if (type.equals("line3")) {
                String s=str.substring(6);
            appIf.appendMessage(s.substring(2) + "\n");

            Messages.postMessage(s);
            return;
        }
        if (type.equals("clear")) {
            appIf.clearMessage();
            return;
        }
        if (type.equals("file")) {
            if (!tok.hasMoreTokens())
              return;
            type = tok.nextToken();
            appIf.clearMessage();
            return;
        }
        if (type.equals("button")) {
            processButtonData(subButPanel, tok);
            return;
        }
        if (type.equals("input")) {
            appIf.setInputPrompt(winId, str.substring(6));
            return;
        }
        if (type.equals("hist")) {
            appIf.setInputData(str.substring(5));
            return;
        }
        if (type.equals("pnew")) {
            if (pVector == null)
                pVector = new Vector<String>();
            else
                pVector.clear();
            if (!tok.hasMoreTokens())
                return;
            a = 0;
            try {
                a = Integer.parseInt(tok.nextToken()); // number of params
            }
            catch (NumberFormatException er) { return; }
            b = 0;
            while (tok.hasMoreTokens()) {
                type = tok.nextToken().trim();
                if (type.length() > 0)
                    pVector.add(type);
                b++;
                    if (b >= a)
                        break;
            }
            if (bDebugPnew) {
               StringBuffer sb = new StringBuffer();
               for (c = 0; c < pVector.size(); c++)
                  sb.append(pVector.elementAt(c)).append(" ");
               logWindow.append(VnmrJLogWindow.PARAM, winId, null, sb.toString());
            }
            updateExpListeners(pVector);
/*
            ParamListenerIF actPanel = (ParamListenerIF)Util.getActionPanel();
            if (actPanel != null) {
                actPanel.updateValue(pVector);
            }
*/
            if (VNMRFrame.usesDatabase()) {
                if (shufParam == null) {
                   shufParam = new ShufflerParam(this);
                }
                shufParam.updateValue(expName, pVector);
            }
            if (panVector != null) {
                controlPanel = Util.getControlPanel();
                updateParamPanels(pVector); // pass names and values to ParamLayout
                if (active) {
                   // if (controlPanel != null)
                   //    controlPanel.updateTabValue(pVector);
                   paramTabCtrl.updateTabValue(pVector);
                }
                setPanelBusy(false);
                // sendFree();
            }
            if(paramArrayPanel != null)
                paramArrayPanel.updatePnewParams(pVector);
            if(active && modelessPopups != null) {
               Enumeration<String> e = modelessPopups.keys();
               while (e.hasMoreElements()) {
                ModelessPopup popup =
                     (ModelessPopup)(modelessPopups.get(e.nextElement()));
                if(popup.isVisible()) popup.updatePnewParams(pVector);
              }
            }
            ModalPopup.updateModalPopups(pVector);
            // VjToolBar vt = ((ExperimentIF) appIf).getToolBar();
/*
            VjToolBar vt = Util.getToolBar();
            if (vt != null)
                vt.updateValue(pVector);
*/

            if(active) {
                VTabbedToolPanel vToolPanel = Util.getVTabbedToolPanel();
                if(vToolPanel != null) vToolPanel.updateValue(pVector);
            }

            return; // pnew
        }
        if (type.equals("pglo")) {
            if (outPort != null) {
                StringBuffer sbcmd = new StringBuffer().append(JFUNC).
                                           append(PGLOBAL).append(", '").
                                           append(str.substring(5)).append("')\n");
                //String cmd = "jFunc("+PGLOBAL+", '"+str.substring(5)+"')\n";
                expIf.sendToAllVnmr(sbcmd.toString(), winId);
                // Util.sendToAllVnmr(cmd);
            }
            return;
        }
        if (type.equals("shcond")) {
            processShowQuery(str.substring(7).trim());
            return;
        }
        if (type.equals("SVF") || type.equals("SVP") || type.equals("SVS")
           || type.equals("svr") || type.startsWith("svr_")) {
            addToShuffler(str.substring(4).trim());
            return;
        }
        if (type.equals("RmFile")) {
            removeFromShuffler(str.substring(7).trim());
            return;
        }
        if (type.equals("flush")) {
            expIf.initCanvasArray();
            return;
        }
        if (type.equals("vnmrjcmd")) {
            if (tok.hasMoreTokens())
                processVjCmd(str);
            return;
        }
        if (type.equals("exit")) {
            Util.exitVnmr();
            return;
        }
        if (type.equals("aipbutton")) {
            if (bCsiOpened)
                processButtonData(csiButPanel, tok);
            else
                processButtonData(subButPanel, tok);
            return;
        }
        if (type.equals("buttoncmd")) {
            setButtonCmd(subButPanel, tok);
            return;
        }
        if (type.equals("aipbuttoncmd")) {
            if (bCsiOpened)
                setButtonCmd(csiButPanel, tok);
            else
                setButtonCmd(subButPanel, tok);
            return;
        }
      }
      catch (Exception e) {
          Messages.writeStackTrace(e, "Exp "+winId+" error on '"+str+"' ");
      }
    }

    // eval shcond pnew gtxt gtxtc error line3 clear file button
    // ARRAY aval para input hist pglo SVF SVP SVS RmFile
    // flush expn layout2 exit vnmrjcmd pars expr port addr

    public void  processComData(String  str) {
        StringTokenizer tok = new StringTokenizer(str, " ,\n");
        if (!tok.hasMoreTokens())
            return;
        int a, b, c;
        String  type = tok.nextToken();
        Messages.postDebug("com", "com " + str);

       try {
        if (type.equals("drag")) {
            if (vcanvas != null)  // sync-drag only supported for native windows
                        vcanvas.enableDrag();
                return;
        }
        if (type.equals("eval")) {
            processEvalQuery(str.substring(5).trim());
            return;
        }
        if (type.equals("shcond")) {
            processShowQuery(str.substring(7).trim());
            return;
        }
        if (type.equals("pnew")) {
            processPnew(tok, str);
            return;
        }
        if (type.equals("title")) {
            if(!tok.hasMoreTokens())
              return;
            type = tok.nextToken();
            String data = tok.nextToken("\n").trim();
            updateTitleBar(type, data);
            return;
        }

        if (type.equals("gtxt")) {
            a = 0; b = 0; c = 0;
            try {
                a = Integer.parseInt(tok.nextToken());  //  point x
                b = Integer.parseInt(tok.nextToken());  // point y
                c = Integer.parseInt(tok.nextToken());
                // d = Integer.parseInt(tok.nextToken());  // length
            }
            catch (NumberFormatException er) { return; }
            type = tok.nextToken("\n").trim();
            vcanvas.drawText(type, a, b, c);
            return;
        }
        if (type.equals("gtxtc")) {
            a = 0;
            try {
                a = Integer.parseInt(tok.nextToken());
            }
            catch (NumberFormatException er) { return; }
            vcanvas.setTextColor(a);
            return;
        }
        if (type.equals("error")) {
            appIf.setMessageColor(Color.red);
            appIf.appendMessage(str.substring(6)+"\n");
            Messages.postMessage("oe "+str.substring(6));
            return;
        }
        if (type.equals("line3")) {
            String s=str.substring(6);
            if (s.indexOf("Warning: ") >= 0) {
                s = s.replaceFirst("o[ie] Warning: ", "ow ");
            }
            appIf.appendMessage(s.substring(2) + "\n");
            Messages.postMessage(s);
            return;
        }
        if (type.equals("clear")) {
            appIf.clearMessage();
            return;
        }
        if (type.equals("file")) {
            if (!tok.hasMoreTokens())
                return;
            type = tok.nextToken();
            appIf.clearMessage();
            return;
        }
        if (type.equals("button")) {
            processButtonData(subButPanel, tok);
            return;
        }
        if (type.equals("aipbutton")) {
            if (bCsiOpened)
                processButtonData(csiButPanel, tok);
            else
                processButtonData(subButPanel, tok);
            return;
        }
        if (type.equals("ARRAY")) {
            processARRAYQuery(str.substring(6).trim());
            return;
        }
        if (type.equals("aval")) {
            processAvalQuery(str.substring(5).trim());
            return;
        }
        if (type.equals("para")) {
            processEvalQuery(str.substring(5));
            return;
        }
        if (type.equals("plim"))
        {
            updateLimitListeners(str.substring(5));
            return;
        }
        if (type.equals("input")) {
            appIf.setInputPrompt(winId, str.substring(6));
            return;
        }
        if (type.equals("hist")) {
            appIf.setInputData(str.substring(5));
            return;
        }
        if (type.equals("pglo")) {
            if (outPort != null) {
                StringBuffer sbcmd = new StringBuffer().append(JFUNC).
                                          append(PGLOBAL).append(", '").
                                          append(str.substring(5)).append("')\n");
                //String cmd = "jFunc("+PGLOBAL+", '"+str.substring(5)+"')\n";
                expIf.sendToAllVnmr(sbcmd.toString(), winId);
                // Util.sendToAllVnmr(cmd);
            }
            return;
        }
        if (type.equals("SVF") || type.equals("SVP") || type.equals("SVS")
        || type.equals("svr") || type.startsWith("svr_")) {
            addToShuffler(str.substring(4).trim());
            return;
        }
        if (type.equals("RmFile")) {
            removeFromShuffler(str.substring(7).trim());
            return;
        }
        if (type.equals("flush")) {
            expIf.initCanvasArray();
            return;
        }
        if (type.equals("bootup"))
        {
            if (active)
            {
                // need to send graphics colors to vnmrbgs after they
                // have finished booting. (probably could put this call in
                // a better place though)
                // DisplayOptions.initColors(); 
                // updateTools(true);
            }
            // initColors only set dps colors, it is unnecessary.
            // DisplayOptions.initColors(); 
            // DisplayOptions.initSpecColors(); 
            return;
        }
        if (type.equals("expn")) {
            sendButtonMask();
            updateExpListeners();
            if (sendWinId)
                sendCanvasId();

            // sendBusy();
            setPanelBusy(true);
            updateTools(true);

	    // Vnmrbg was not running yet when Infostat sent its values
            // wait until now to send the string command to Vnmrbg
            if (bSendInfo) {
                sendToVnmr(rfMonStr);
                bSendInfo=false;
            }

            if (active) {
                controlPanel = Util.getControlPanel();
                // if (controlPanel != null)
                //     controlPanel.updateTabValue();
                paramTabCtrl.updateTabValue();
                VTabbedToolPanel vToolPanel = Util.getVTabbedToolPanel();
                if(vToolPanel != null)
                     vToolPanel.updateValue();
            }
            if (tok.hasMoreTokens()) {
                expName = tok.nextToken();
                //if (titleBar != null)
                //    titleBar.setText(" "+expName);
                if (needPanelInfo) {
                    queryPanelInfo();
                }
            }
            if (tok.hasMoreTokens()) {
                expDir = tok.nextToken("\n").trim();
                if (active)
                    Util.setExpDir(expDir);
            }
/*
            if (appIf instanceof ExperimentIF)
                ((ExperimentIF)appIf).sendCmdLineOpen();
*/
            if (appIf instanceof VnmrjIF)
                ((VnmrjIF)appIf).sendCmdLineOpen();
            showTrayButton(true);
            expIf.expReady(winId);
            //if (glcom != null)
            //   glcom.setShowing(true);
            // sendFree();
            setPanelBusy(false);
            if (apptypeObj != null)
                apptypeObj.updateValue();
            return;
        }
        if (type.equals("layout2")) {
            Messages.postError("layout2 replaced by layoutpar");
            return;
        }
        if (type.equals("layoutpar")) {

            int id = 0;
            String fin = null;
            if (tok.hasMoreTokens()) {
                try {
                    id = Integer.parseInt(tok.nextToken());
                }
                catch (NumberFormatException er) { return; }
            }
            if (tok.hasMoreTokens())
                fin = tok.nextToken().trim();
            if (fin != null)
            {
                // sendBusy();
                setPanelBusy(true);
                // store the panel id, and filename, if the current viewport
                // is the active viewport, then create panels, otherwise the
                // initial panels would be created when that viewport is clicked.
                // getParamPanelId().addPanelInfo(id, fin);

                setParamPanelLayout(id, fin);
                createParamPanel(id);
                if (this.equals(Util.getActiveView())) {
        	    if (vmenuBar != null) vmenuBar.updateValue(true);
		}
                    // createParamPanel(id, fin);
                // sendFree();
                setPanelBusy(false);
            }
            return;
        }
        if (type.equals("exit")) {
            Util.exitVnmr();
            return;
        }
        if (type.equals("vnmrjcmd")) {
            if (tok.hasMoreTokens())
                processVjCmd(str);
            return;
        }
        if (type.equals("lccmd")) {
            if (tok.hasMoreTokens())
                processLcCmd(str);
            return;
        }
        if (type.equals("pmlcmd")) {
            if (tok.hasMoreTokens())
                processPmlCmd(str);
            return;
        }
        if (type.equals("pars")) {
            processSyncQuery(str.substring(5).trim());
            return;
        }
        if (type.equals("expr")) {
            processSyncExpr(str.substring(5).trim());
            return;
        }
        if (type.equals("mouse")) {
            if (!tok.hasMoreTokens())
                return;
            type = tok.nextToken().trim();
            if (vcanvas != null) {
              if (type.equals("on"))
                vcanvas.sendMoreEvent(true);
              else
                vcanvas.sendMoreEvent(false);
            }
            return;
        }

        if (type.equals("vbgcmd"))
        {
            if (bDebugCmd) {
                if (tok.hasMoreTokens())
                   logWindow.appendVbgCmd(winId, str.substring(7));
            }
            return;
        }

        if (type.equals("buttoncmd")) {
            setButtonCmd(subButPanel, tok);
            return;
        }
        if (type.equals("aipbuttoncmd")) {
            if (bCsiOpened)
                setButtonCmd(csiButPanel, tok);
            else
                setButtonCmd(subButPanel, tok);
            return;
        }

        if (type.equals("dps")) {
            if (!tok.hasMoreTokens())
                return;
            type = tok.nextToken();
            if (type.equals("window")) {
                type = tok.nextToken();
                if (type == null)
                    return;
                if (type.equals("on")) {
                    getDpsButton().setVisible(true);
                    if (dpsPanel == null) {
                        dpsPanel = new DpsInfoPan(this, winId);
                    }
                }
                if (type.equals("off")) {
                    getDpsButton().setVisible(false);
                    if (dpsPanel != null) {
                        dpsPanel.setVisible(false);
                    }
                }
                return;
            }
            if (dpsPanel != null)
                dpsPanel.showInfo(str.substring(4));
            return;
        }

        if (type.equals("port") || type.equals("addr")) {
                if (!tok.hasMoreTokens())
                   return;
                vnmrHostName = tok.nextToken();
                a = 0;
                b = 0;
                try {
                  a = Integer.parseInt(tok.nextToken());
                  b = Integer.parseInt(tok.nextToken());
                }
                catch (NumberFormatException er) { return; }

                if (a > 0 && vnmrHostPort != a) {
                    vnmrHostPort = a;
                    int  vnmrId = b;
                    outPort = new CanvasOutSocket(this, vnmrHostName, vnmrHostPort);
                    String vpath = FileUtil.sysdir()+File.separator+"tmp"+File.separator+"vnmr"+vnmrId;
                    try {
                        PrintWriter os = new PrintWriter(new FileWriter(vpath));
                        if (os != null) {
                            os.println("vnmr "+vnmrId);
                            os.close();
                        }
                    }
                    catch(IOException er) { 
                       // return;
                    }
                    if (vnmrbgTimer != null) {
                        vnmrbgTimer.stop();
                        vnmrbgTimer = null;
                    }
                    // if any sendToVnmr commands were queue, send them now
                    //System.out.println("sending: " + m_queueSendToVnmrCmds.size() + " queued cmds");
                    for(int i=0; i < m_queueSendToVnmrCmds.size(); i++)
                    {
                       String cmd = m_queueSendToVnmrCmds.get(i);
                       // System.out.println("sending ququed cmd("+i+"): '"+cmd+"'");
                       sendToVnmr(cmd);
                    }
                    // all sent, clear the queue
                    m_queueSendToVnmrCmds.clear();

                }
                return;
        }
        // vloc is expected to contain 3 args, expname, attr and val
        // This is a result of the command jFunc(41, 'expname attr', 'attr')
        if (type.equals("vloc")) {
            String attr=null;
            String val=null;
            String expname=null;
            if(tok.hasMoreTokens())
                expname = tok.nextToken().trim();
            if(tok.hasMoreTokens())
                attr = tok.nextToken().trim();
            if(tok.hasMoreTokens())
                val = tok.nextToken().trim();

            if(expname != null && attr != null && val != null) {

                Vector<String> parval = new Vector<String>();
                parval.add(attr);
                parval.add(val);
                shufParam = new ShufflerParam(this);
                shufParam.updateValue(expname, parval);
            }
            return;
        }
      }
      catch (Exception e) {
          Messages.writeStackTrace(e, "error on '"+str+"' ");
      }
    }

    private void processPmlCmd(String str) {
        Messages.postDebug("PML", "ExpPanel.processPmlCmd(" + str + ")");
        StringTokenizer tok = new StringTokenizer(str, " ,\n");
        tok.nextToken();        // Skip "pmlcmd" part
        String cmd = tok.nextToken("").trim(); // Rest of string

        if (pmlClient == null) {
            pmlClient = new MsCorbaClient(this);
            Messages.postDebug("PML", "ExpPanel.processPmlCmd(): "
                               + "Created pmlClient=" + pmlClient);
        }

        if (pmlClient != null) {
            // boolean ok = false;
            // ok = pmlClient.sendCommand(cmd);
            //if (!ok) {
            //    pmlClient = null; // Make new client next time
            //}
        } else {
            Messages.postError("ExpPanel.processPmlCmd \"" + cmd + "\""
                               + ":  trouble starting PML interface.");
        }
    }

    private void  processLcCmd(String  str) {
        Messages.postDebug("lccmd", "processLcCmd(): " + str);

        if (m_lcControl == null) {
            StringTokenizer tok = new StringTokenizer(str, " ,\n");
            tok.nextToken();        // strip "lccmd"
            String cmd = "";
            if (tok.hasMoreTokens()) {
                cmd = tok.nextToken();
            }
            if (cmd.equals("init")) {
                boolean haveLock = false;
                if (tok.hasMoreTokens()) {
                    String sarg = tok.nextToken();
                    try {
                        int arg = Integer.parseInt(sarg);
                        haveLock = (arg != 0);
                    } catch (NumberFormatException nfe) {}
                }
                Messages.postDebug("lccmd",
                                   "processLcCmd: init: haveLock=" + haveLock);
                Messages.postDebug("Creating LC interface");
                m_lcControl = new LcControl(this, haveLock);
                if (m_lcControl == null) {
                    Messages.postDebug("ExpPanel.processLcCmd \"" + str + "\""
                                       + ":  trouble starting LC interface.");
                }
            } else {
                // We got an LC command without an "init" first
                // We'll have VnmrBg sort it out
                // args is either nothing or all the args to lcInit,
                // including parentheses
                String args = "";
                if (cmd.length() > 0) {
                    // We want to resend this command after initialization
                    StringTokenizer stripper = new StringTokenizer(str, " ,\n");
                    stripper.nextToken();         // strip "lccmd"
                    args = "('cmd', '"
                            + stripper.nextToken("").trim() // stripped string
                            + "')";
                }
                sendToVnmr("lcInit" + args);
            }
        }

        if (m_lcControl != null) {
            m_lcControl.execCommand(str);
        }
    }

    public String getMyLcIpAddress() {
        if (m_lcControl == null) {
            return "";
        } else {
            return m_lcControl.getMyIpAddress();
        }
    }
    
    private void  processCryoCmd(String  str) {

        if (m_cryoControl == null) {
            StringTokenizer tok = new StringTokenizer(str, " ,\n");
            String cmd = "";
            if (tok.hasMoreTokens())
                cmd = tok.nextToken();
            if (cmd.equals("init")) {
                m_cryoControl = new CryoControl(this);
            } else {
                // We got an CRYO command without an "init" first
                sendToVnmr("vnmrjcmd('CRYO init')"
                           + "vnmrjcmd('CRYO " + str + "')");
            }
        }

        if (m_cryoControl != null) {
            m_cryoControl.execCommand(str);
        } else {
        }
    }

    public void submitEmptyTray(String s) {
        StringTokenizer tok = new StringTokenizer(s, " ,\n");
        String macro = "xmsubmit";
        String s1 = "day";
        if (tok.hasMoreTokens())
            macro = tok.nextToken().trim();
        if (tok.hasMoreTokens())
            s1 = tok.nextToken().trim();
        StringBuffer sb = new StringBuffer().append(macro).append("('");
        sb.append(s1).append("', '0', '0', '')\n");
        sendToVnmr(sb.toString());
    }


    private void printDialogCmd(QuotedStringTokenizer tok)
    {
        // isPrintscreen and ghostscriptAvailable are not mutually
        // exclusive, but if we are performing a (so-called)
        // "printscreen" we do not need to perform a conversion,
        // and if we are not performing a printscreen then we
        // *may* need a conversion
        boolean isPrintscreen = false;
        boolean ghostscriptAvailable = true;
        String arg;
        String localSysdir = System.getProperty("sysdir");

        if (tok.hasMoreTokens()) {
                arg = tok.nextToken().trim();
                isPrintscreen = arg.equals("printscreen");
                ghostscriptAvailable = arg.equals("gsavailable");
        }

        if (isPrintscreen) {
            String printDialogCommand;
            QuotedStringTokenizer dialogTok;

            printDialogCommand = "region:graphics";
            printDialogCommand += " layout:landscape";
            printDialogCommand += " file:" + localSysdir
                        + "/tmp/printtemp.jpg";

            printDialogCommand += " pformat:jpeg";
            printDialogCommand += " iformat:jpg";
            printDialogCommand += " printsize:page";
            printDialogCommand += " color:color";
            Messages.postInfo("DEBUG jgw: printDialogCommand 1 = "
                                  + printDialogCommand);
            dialogTok = new QuotedStringTokenizer(printDialogCommand);
            ((ExpViewArea)expIf).execPrintcmd(dialogTok , winId);

            printDialogCommand = "exec device:file";
            Messages.postInfo("DEBUG jgw: printDialogCommand 2 = "
                                  + printDialogCommand);
            dialogTok = new QuotedStringTokenizer(printDialogCommand);
            ((ExpViewArea)expIf).execPrintcmd(dialogTok , winId);

            new VPrintDialog(localSysdir + "/tmp/printtemp.jpg",
                                 javax.print.DocFlavor.INPUT_STREAM.JPEG);
            // new VPrintDialog(this, localSysdir + "\\tmp\\printtemp.jpg", javax.print.DocFlavor.INPUT_STREAM.JPEG);

            return;
        }
        if (ghostscriptAvailable) {
            String inputPath = localSysdir + "/tmp/printpreview.ps";
            String outputPath = localSysdir + "/tmp/printpreview.png";

            boolean jgwTemp = VPrintDialog.generatePngFile(inputPath,
                                           outputPath, false);
            if (!jgwTemp) {
                    System.err.println("DEBUG jgw: generatePngFile() failed");
                    return;
            }

            new VPrintDialog(outputPath,
                       javax.print.DocFlavor.INPUT_STREAM.PNG, inputPath);
               // new VPrintDialog(this, outputPath,
               //        javax.print.DocFlavor.INPUT_STREAM.PNG, inputPath);

            return;
        } else {
            String path = localSysdir + "/tmp/printpreview.ps";

            new VPrintDialog(path,
                               javax.print.DocFlavor.INPUT_STREAM.POSTSCRIPT);
           // new VPrintDialog(this, path,
           //         javax.print.DocFlavor.INPUT_STREAM.POSTSCRIPT);

        }
    }

    private void  screenShotVnmrj() {
        Dimension dim = appIf.getSize();
        Point pt = appIf.getLocationOnScreen();

        if (plotFile == null)
            return;
        if (!CaptureImage.doCapture(plotFile, dim, pt))
            return;
        if (plotFormat != null && plotFormat.length() > 0) {
            boolean bOk = false;
            if (!plotFormat.equals(CaptureImage.getOutputFormat()))
               bOk = CaptureImage.doConvert(plotFile, CaptureImage.JPEG, plotFormat);
            if (bOk) {
               if (!plotFile.equals(CaptureImage.getOutputFileName())) {
                  // delete the original file after converting
                  File objFile = new File(plotFile);
                  objFile.delete();
               }
            }
         }
         plotFormat = "jpeg";
    }

    private void  processScreenShot(QuotedStringTokenizer tok) {
        boolean bGraphics = true;
        String  data;

        if (!tok.hasMoreTokens())
            return;
        plotFile = null;
        plotFormat = null;
        data = tok.nextToken();
        if (data.equals("vnmrj") || data.equals("frame"))
            bGraphics = false;
        if (tok.hasMoreTokens()) {
            plotFile = tok.nextToken();
            if (tok.hasMoreTokens())
                plotFormat = tok.nextToken();
        }
        if (bGraphics) {
            if (plotFile == null)
                return;
            if (plotFormat != null)
                data = plotFile+","+plotFormat;
            else
                data = plotFile;
            processCDump(data);
            plotFile = null;
            return;
        }
        if (!isShowing())
            return;
        JFrame vframe = (JFrame) VNMRFrame.getVNMRFrame();
        if (vframe == null)
            return;
        Runnable scrnDump = new Runnable() {
                public void run() {
                    screenShotVnmrj();
                }
        };
        vframe.toFront();
        SwingUtilities.invokeLater(scrnDump);
    }

    // command from 'a' to 'l'
    private void  vjCmd(String  str, String cmd, QuotedStringTokenizer tok) {
        String data;
        boolean aShow;

//### G3D DEV ##############       
        if (cmd.equals("gldemo")) {
            String demo="gears";
            if (tok.hasMoreTokens())
                demo=tok.nextToken();
            if(gldemo==null)
                gldemo=new GLDemo();
            gldemo.setDemo(demo);
            gldemo.setVisible(true);
            return;
        }
        if(cmd.contains("g3dinit")) {
           if (Util.isMacOs())
           {
               gldialog = null;
               glcom = null;
               return;
           }
           else
           {
        	if(gldialog == null) {
                    try {
        		glcom=new JGLComMgr(this);
        		gldialog = new JGLDialog(glcom,this,sshare);
                     }
                     catch (Exception e) {
                        gldialog = null;
                        glcom = null;
                        Messages.writeStackTrace(e);
                     }
                     catch (ExceptionInInitializerError e2) {
                        gldialog = null;
                        glcom = null;
                        Messages.postError(str);
                        e2.printStackTrace();
                     }
                     catch (NoClassDefFoundError e3) {
                        gldialog = null;
                        glcom = null;
                        Messages.postError(str);
                        e3.printStackTrace();
                     }
                     if (gldialog == null)
                         return;
        	}
           }
        }
        if (gldialog!=null && cmd.equals("g3d")) {
            if (tok.hasMoreTokens()){
                String token=tok.nextToken();
                String value="";
                if (tok.hasMoreTokens())
                	value=tok.nextToken();
                if(token.equals("window")){
                	gldialog.setShowWindow(value);
                	return;
                }
                else if(token.equals("viewport")){
                	gldialog.setJoinViewport(value);
                	getLayout().layoutContainer(this);
                 	return;
                }
                else if(token.equals("repaint")){
                	repaint();
                 	return;
                }
             }      	
        }
        if(glcom != null && glcom.comCmd(str)) // check for glcom command
            return;
// ################################

        if (cmd.equals("array")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            if(!data.equals("popup"))
                return;

              // Create and show ParamArrayPanel
            paramArrayPanel = ParamArrayPanel.showParamArrayPanel(this);
            return;
        }
        if (cmd.equals("batch")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken();
            if (data.equals("pnew")) {
                if (!tok.hasMoreTokens())
                    return;
                data = tok.nextToken();
                if (data.equals("on")) {
                    sendToVnmr("vnmrjcmd('batch param on')");
                } else if (data.equals("off")) {
                    sendToVnmr("vnmrjcmd('batch param off')");
                }
            } else if (data.equals("param")) {
                if (!tok.hasMoreTokens())
                    return;
                data = tok.nextToken();
                if (data.equals("on")) {
                } else if (data.equals("off")) {
                }
            }
            return;
        }
        if (cmd.equals("busy")) {
            setPanelBusy(true);
            // panelBusy = true;
            // setBusy(true);
            return;
        }
        if (cmd.equals("canvasCursor")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            ((ExpViewArea)expIf).setCanvasCursor(data);
            return;
        }
        if (cmd.equals("canvas")) {
            CanvasIF canvas = Util.getViewArea().getActiveCanvas();
            if(canvas != null && tok.hasMoreTokens()){
                String s=str.substring(15);
                canvas.processCommand(s.trim());
            }
            return;
        }
        if (cmd.equals("closepopup")) {
            if (!tok.hasMoreTokens())
                return;
            processClosePopupCmd(str.substring(9));
            return;
        }
        if (cmd.equals("cmdLine"))
        {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            if (data.equals("operator"))
            {
                String strOperator = tok.nextToken().trim();
                data = WUserUtil.getOperatordata(strOperator, WOperators.OPERATORS[5]);
                if (data == null || data.trim().equals("") || data.equals("null"))
                    data = tok.nextToken().trim();
            }
            aShow = data.equals("yes") ? true : false;
            // We need to save this state for use by the menu system
            // Once the jFunc 101 cmd is given, then the command
            // cmdlineOK(-1) can be used to get the status and used
            // for things like a show condition.
            String state = "1";
            if(aShow)
                state = "0";
            // The 101 is defined for CMDLINEOK in smagic.c .  It must
            // remain in sync between here and there.
            String vcmd = "jFunc(101," + state + ")";
            sendToVnmr(vcmd);
            
            // ((ExperimentIF)Util.getAppIF()).setCmdLine(aShow);
            // ((ExperimentIF)Util.getAppIF()).setCmdLine(aShow);
            VnmrjIF vjIf = Util.getVjIF();
            if (vjIf != null)
               vjIf.setCmdLine(aShow);
            return;
        }
        if (cmd.equals("colormap")) {
            aShow = true;
            boolean bDebug = false;
            ImageColorMapEditor ed = ImageColorMapEditor.getInstance();
            while (tok.hasMoreTokens()) {
                data = tok.nextToken().trim();
                if (data.equalsIgnoreCase("close"))
                   aShow = false;
                if (data.equalsIgnoreCase("debug"))
                   bDebug = true;
            } 
            ed.showDialog(aShow);
            ed.setDebugMode(bDebug);
            return;
        }
        if (cmd.equals("cursor"))
        {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            actCanvas.setCursor(VCursor.getCursor(data));
            return;
        }
        if (cmd.equals("csiwindow")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            aShow = false;
            if (data.equalsIgnoreCase("open") || data.equalsIgnoreCase("show"))
                aShow = true;
            setCsiPanelVisible(aShow);
            return;
        }
        if (cmd.equalsIgnoreCase("customuicolor")) {
            if (!tok.hasMoreTokens())
                return;
            DisplayOptions.enableCustomUIColor(str);
            return;
        }
        if (cmd.equals("lcwindow")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            aShow = false;
            if (data.equalsIgnoreCase("open") || data.equalsIgnoreCase("show"))
                aShow = true;
            if (lcPanel != null)
                lcPanel.setVisible(aShow);
            return;
        }
        if (cmd.equals("debug")) {
            if (logWindow == null) {
                logWindow = VnmrJLogWindow.getInstance();
                logWindow.addEventListener(this);
            }
            if (!tok.hasMoreTokens()) {
                logWindow.addEventListener(this);
                logWindow.setLogMode(VnmrJLogWindow.SHOWLOG, true);
                return;
            }
            boolean bMode = true;
            boolean bAll = false;
            int     logType = 0;
            data = tok.nextToken().trim();
            String mode = data;
            if (tok.hasMoreTokens())
               mode = tok.nextToken().trim();
            if (data.equalsIgnoreCase("off")) {
               bAll = true;
               bMode = false;
            }
            if (data.equalsIgnoreCase("all"))
               bAll = true;
            if (mode.equalsIgnoreCase("off"))
               bMode = false;
            if (bAll && !bMode) {
                logWindow.setLogMode(VnmrJLogWindow.SHOWLOG, bMode);
                return;
            }
            if (data.equals("cmd") || data.equals("command")) {
               logType = VnmrJLogWindow.COMMAND;
            }
            else if (data.equals("ui") || data.equals("vnmrjcmd")) {
               logType = VnmrJLogWindow.UICMD;
            }
            if (data.equals("pnew") || data.equals("parameter")) {
               logType = VnmrJLogWindow.PARAM;
            }
            if (data.equals("query") || data.equals("value")) {
               logType = VnmrJLogWindow.QUERY;
            }
            logWindow.setLogMode(logType, bMode);

            return;
        }
        if (cmd.equals("display")) {
            if (!tok.hasMoreTokens())
                return;
//            data = tok.nextToken().trim();
//            if(!data.equals("options") || !tok.hasMoreTokens())
//                return;
//            DisplayOptions.setOption(str.substring(24));
            data = tok.nextToken().trim();
            if(data.equals("options"))
                DisplayOptions.setOption(str.substring(24));
            if(data.equals("theme") && tok.hasMoreTokens()){
                String type=tok.nextToken().trim();
                if(tok.hasMoreTokens())
                    DisplayOptions.setTheme(type,tok.nextToken().trim());
            }
            return;
        }
        if (cmd.equals("edit")) {
            if (!tok.hasMoreTokens())
                return;
            processEditCmd(str.substring(9));
            return;
        }
        if (cmd.equals("enablepanel")) {
            if (panVector == null)
                return;
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            if (!tok.hasMoreTokens())
                return;
            String name = tok.nextToken("\n").trim();
            ExpPanInfo info = null;
            ParameterPanel p = null;
            int nLength = panVector.size();
            for (int k = 0; k < nLength; k++) {
                info = (ExpPanInfo)panVector.elementAt(k);
                if ((info != null) && info.name.equals(name)) {
                     info.bActive = data.equals("false")?false:true;
                     p = (ParameterPanel)info.paramPanel;
                     break;
                }
            }
            if (p != null) {
                 ParamLayout p2 = p.getParamLayout();
                 if (p2 != null)
                      p2.enablePanel(info.bActive);
            }
            return;
        }

        if (cmd.equals("exit")) {
            Util.exitVnmr();
            return;
        }
        if (cmd.equals("feval")) {
            if (!tok.hasMoreTokens())
                return;
            processFileEvalQuery(tok.nextToken());
            return;
        }
        if (cmd.equals("frame")) {
            if (tok.hasMoreTokens()) {
               vcanvas.frameCommand(tok.nextToken("\n"));
            }
            return;
        }
        if (cmd.equalsIgnoreCase("appdir")) {
	    if (tok.hasMoreTokens()) {
                processAppDir(str.substring(9));
	    }
            return;
	}
        if (cmd.equalsIgnoreCase("language")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
	    String country = null;
	    if(tok.hasMoreTokens()) country = tok.nextToken().trim();
            Util.switchLanguage(data, country);
            return;
	}
        if (cmd.equals("dataInfo")) {
	    //if (!active) return;
            String s=str.substring(18);
            ((ExpViewArea)expIf).processDataInfo("dataInfo", s.trim());
/*
            CanvasIF canvas = Util.getViewArea().getActiveCanvas();
            if(canvas != null) {
               String s=str.substring(9);
               canvas.processCommand(s.trim());
	    }
*/
            return;
        }
        if (cmd.equals("free")) {
            setPanelBusy(false);
            // panelBusy = false;
            // setBusy(false);
            return;
        }
        if (cmd.equals("graphics")) {
            if (!tok.hasMoreTokens())
                return;
            vcanvas.setGraphicsMode(tok.nextToken("\n"));
/*
            expIf.setCanvasGraphicsMode(tok.nextToken("\n"));
*/
            return;
        }
        if (cmd.equals("icon"))
        {
            actCanvas.drawIcon(str.substring(13));
            return;
        }
        if (cmd.equals("dragtest")) {
            vcanvas.dragtest(tok.nextToken("\n"));
            return;
        }
        if (cmd.equals("helpoverlay")) {
            if (!tok.hasMoreTokens())
                return;
            VnmrjIF vjIf = Util.getVjIF();
            HelpOverlay ov = vjIf.getHelpOverlay();
            if (ov == null)
                return;
            data = tok.nextToken("\n");
            ov.setOption(data);
            return;
        }
        if (cmd.equals("input")) {
            if (!tok.hasMoreTokens())
                return;
            appIf.setInputPrompt(winId, tok.nextToken());
            return;
        }
        if (cmd.equals("isimagebrowser"))
        {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            // ((ExperimentIF)appIf).setInputFocus(data);
            appIf.setInputFocus(data);
            return;
        }
        if (cmd.equals("layout")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            if (!tok.hasMoreTokens())
                return;
            if (data.equals("use")) {
                if (active)
                    Util.openUiLayout(tok.nextToken().trim());
            }
            if (data.equals("save"))
                Util.saveUiLayout(tok.nextToken().trim());
            return;
        }

        if (cmd.equals("help")) {
/*
            if (!tok.hasMoreTokens())
                return;

            if (tok.nextToken().equals("jhelp")) {
                VnmrJHelp vjh = new VnmrJHelp();
                vjh.setJHelpVisible();
            }
 */
            if (!tok.hasMoreTokens())
                return;
            VBrowser.displayURL(tok.nextToken());
            return;
        }
        if (cmd.equals("iplot")) {
            if (!tok.hasMoreTokens())
                return;
            if (vnmrPlot == null)
                vnmrPlot = new VnmrPlot();
            vnmrPlot.plot(tok.nextToken("\n").trim());
            return;
        }
    }

    private void  setParamPanelLayout(int id, String layout) {
        if (panVector == null)
             return;
        ExpPanInfo info = null;
        int  fid = id;
        int  nLength = panVector.size();
        if (fid >= 90)
             fid = id - 90;
        
        for (int k = 0; k < nLength; k++) {
             info = (ExpPanInfo) panVector.elementAt(k);
             if ((info != null) && (info.id == fid))
                     break;
             info = null;
        }
        if (info != null)
             info.layoutDir = layout;
    }

    private void  vjCmd2(String  str, String cmd, QuotedStringTokenizer tok) {
        String data;
        int num = 0;
        if (glcom != null && str.length()>9) {
            String gstr=str.substring(9);
            if(glcom.comCmd(gstr)){
                //if(gldialog != null)
               //    gldialog.updateTools(gstr);
                return;
            }
        }

        if (cmd.equals("mol"))
        {
            if (!tok.hasMoreTokens())
               return;
            data = tok.nextToken().trim();
            if (data.equals("on"))
                appIf.showJMolPanel(true);
            else if (data.equals("off"))
                appIf.showJMolPanel(false);
            else if (data.equals("icon"))
                vcanvas.drawIcon(m_strVJmolImagePath);
            else if (data.equals("display")) {
                if (tok.hasMoreTokens())
                    ((ExpViewArea)expIf).showMol(tok.nextToken(), vjmol);
            }
            else if (data.equals("print"))
                saveVJmolImage(true, false);
            else if (data.equals("background") || data.equals("foreground"))
            {
                if (tok.hasMoreTokens())
                    sshare.putProperty("mol"+data, tok.nextToken());
                ((ExpViewArea)expIf).setvjmolPref();
                m_strMolColor = (String)sshare.getProperty("molbackground");
            }
            else if (data.equals("open"))
                openMol(str.substring(13));
            else
                vcanvas.drawIcon(str.substring(12));
            return;
        }
        if (cmd.equals("mouse")) {
            if (!tok.hasMoreTokens())
               return;
            data = tok.nextToken().trim();
            if (data.equals("on"))
                vcanvas.sendMoreEvent(true);
            else
                vcanvas.sendMoreEvent(false);
            return;
        }
        if (cmd.equals("movie")) {
            String action = tok.nextToken().trim();
            String path = "/tmp/myMovie.mov";
	    int width = 500; 
	    int height = 500; 
	    int nimages = 100; 
	    int rate = 5; 
            if (tok.hasMoreTokens()) 
               path = tok.nextToken().trim();
            if (tok.hasMoreTokens()) {
               data = tok.nextToken().trim();
               try {
                  width = Integer.parseInt(data);
               }
               catch (NumberFormatException er) {}
            }
            if (tok.hasMoreTokens()) {
               data = tok.nextToken().trim();
               try {
                 height = Integer.parseInt(data);
               }
               catch (NumberFormatException er) {}
            }
            if (tok.hasMoreTokens()) {
               data = tok.nextToken().trim();
               try {
                 nimages = Integer.parseInt(data);
               }
               catch (NumberFormatException er) {}
            }
            if (tok.hasMoreTokens()) {
               data = tok.nextToken().trim();
               try {
                 rate = Integer.parseInt(data);
               }
               catch (NumberFormatException er) {}
            }
            if (action.equals("start")) {
                ImagesToJpeg movieMaker = ImagesToJpeg.get();
                if(movieMaker != null) {
		  movieMaker.start(path, width, height, nimages, rate);
                }
            } else if (action.equals("next")) {
                actCanvas.sendMoreEvent(false);
                ImagesToJpeg movieMaker = ImagesToJpeg.get();
                if(movieMaker != null) {
		  movieMaker.next();
                }
            } else if (action.equals("done")) {
                actCanvas.sendMoreEvent(false);
                ImagesToJpeg movieMaker = ImagesToJpeg.get();
                if(movieMaker != null) {
		  movieMaker.done();
                }
            } else if (action.equals("view")) {
		Util.sendToVnmr("simpleMovie('play','"+path+"',"+nimages+","+width+","+height+","+rate+")");
	    }
            return;
        }
        if (cmd.equals("p11check")) {
            JButton button = getCheckButton();
            if (tok.hasMoreTokens()) {
                data = tok.nextToken().trim();
                if (data.equals("blue")) {
                   button.setVisible(true);
                   button.setIcon(Util.getImageIcon("blueFid.gif"));
                   button.setToolTipText("fid checksum matches");
                   return;
                } else if (data.equals("yellow")) {
                   button.setVisible(true);
                   button.setIcon(Util.getImageIcon("yellowFid.gif"));
                   button.setToolTipText("old data: checksum not defined");
                   return;
                } else if (data.equals("red")) {
                   button.setVisible(true);
                   button.setIcon(Util.getImageIcon("redFid.gif"));
                   button.setToolTipText("fid or checksum corrupted");
                   return;
                } else if (data.equals("none")) {
                   button.setVisible(true);
                   button.setIcon(Util.getImageIcon("blank.gif"));
                   button.setToolTipText("no fid");
                   return;
                }
            }
            button.setVisible(false);
            return;
        }
        if (cmd.equals("p11link")) {
            JButton button = getLinkButton();
            if (tok.hasMoreTokens()) {
                data = tok.nextToken().trim();
                String linkpath = "";
                if (tok.hasMoreTokens()) linkpath = tok.nextToken().trim();
                if (data.equals("blueLink")) {
                   button.setVisible(true);
                   button.setIcon(Util.getImageIcon("blueLink.gif"));
                   button.setToolTipText("fid linked to FDA dir " + linkpath);
                   return;
                } else if (data.equals("yellowLink")) {
                   button.setVisible(true);
                   button.setIcon(Util.getImageIcon("yellowLink.gif"));
                   button.setToolTipText("fid linked to non-FDA dir " + linkpath);
                   return;
                } else if (data.equals("redLink")) {
                   button.setVisible(true);
                   button.setIcon(Util.getImageIcon("redLink.gif"));
                   button.setToolTipText("fid linked to corrupted FDA dir " + linkpath);
                   return;
                } else if (data.equals("blueNoLink")) {
                   button.setVisible(true);
                   button.setIcon(Util.getImageIcon("blueNoLink.gif"));
                   button.setToolTipText("fid is not linked");
                   return;
                } else if (data.equals("yellowNoLink")) {
                   button.setVisible(true);
                   button.setIcon(Util.getImageIcon("yellowNoLink.gif"));
                   button.setToolTipText("fid is not linked");
                   return;
                } else if (data.equals("redNoLink")) {
                   button.setVisible(true);
                   button.setIcon(Util.getImageIcon("redNoLink.gif"));
                   button.setToolTipText("fid is not linked");
                   return;
                } else if (data.equals("none")) {
                   button.setVisible(false);
                   return;
                }
            }
            button.setVisible(false);
            return;
        }
        if (cmd.equals("p11svfdir")) {
            String pcmd = null;
            String svfdir = null;
            JButton button = getDirButton();
            SmsPanel sPan = ((ExpViewArea)expIf).getSmsPanel();

            p11Tip = null;
            p11Icon = null;
            if (tok.hasMoreTokens()) {
                pcmd = tok.nextToken().trim();
                if (tok.hasMoreTokens())
                    svfdir = tok.nextToken().trim();
                else
                    svfdir = "";
            }
            if (pcmd != null) {
                if (pcmd.equals("blue")) {
                   p11Icon = Util.getImageIcon("blueDir.gif");
                   p11Tip = "svfdir is FDA dir: " + svfdir;
                }
                else if (pcmd.equals("yellow")) {
                   p11Icon = Util.getImageIcon("yellowDir.gif");
                   p11Tip = "svfdir is non-FDA dir: " + svfdir;
                }
                else if (pcmd.equals("none")) {
                   p11Icon = Util.getImageIcon("blank.gif");
                   p11Tip = "svfdir is not defined";
                }
            }
            if (p11Icon != null) {
                button.setIcon(p11Icon);
                button.setToolTipText(p11Tip);
                button.setVisible(true);
            }
            else
                button.setVisible(false);
            if (sPan != null)
                sPan.setP11svfdir(p11Icon, p11Tip);
            return;
        }
        if (cmd.equals("pageDialog")) {
            VPrintDialog.displayPageDialog(this);
            return;
        }
        if (cmd.equals("popup")) {
            if (!tok.hasMoreTokens())
                return;
            processPopupCmd(str.substring(9));
            return;
        }
        if (cmd.equals("print")) {
            if (!tok.hasMoreTokens())
                return;
            ((ExpViewArea)expIf).execPrintcmd(tok, winId);
            return;
        }
        if (cmd.equals("printDialog")) {
            Messages.postDebug("Calling printDialogCmd");
            printDialogCmd(tok);
            Messages.postDebug("printDialogCmd done");
            return;
        }
        if (cmd.equalsIgnoreCase("previewPlotfile")) {
            String orient = "portrait";
            boolean noRefresh = false;
            if (tok.hasMoreTokens())
                 orient = tok.nextToken();
            if (tok.hasMoreTokens())
                 noRefresh = tok.nextToken().equals("norefresh");
            previewPlotfile(orient, noRefresh);
            // previewPlotfile();
            return;
        }
        if (cmd.equals("printscreen")) {            
            if (printDialog == null)
                printDialog = new VPrintDialog();
            else
                printDialog.showDialog(true);
            // printDialog.setVisible(true);
            return;
        }
        if (cmd.equals("reqpar")) {
            String arg = "";
            String paramName = "";
            if (!tok.hasMoreTokens()) {
                return;
            }
            arg = tok.nextToken().trim();
            if (arg.equals("showgui")) {
                // TODO: implement -- the intention is that "showgui" should
            	// display all required parameters, whether they have been
            	// set to a value or not, and "warngui" only displays missing
            	// required parameters -- in both cases, the list is populated
            	// (in ReqParWarnDialog) prior to displaying
            } else if (arg.equals("warngui")) {
                if (!tok.hasMoreTokens()) {
                    return;
                } else {
                    arg = tok.nextToken().trim();
                    if (arg.equals("set")) {
                        if (!tok.hasMoreTokens()) {
                            return;
                        } else {
                            if (!tok.hasMoreTokens()) {
                                return;
                            } else {
                                arg =tok.nextToken().trim();
                                if (!tok.hasMoreTokens()) {
                                    return;
                                } else {
                                    paramName = tok.nextToken().trim();
                                }
                                if (arg.equals("string")) {
                                    ReqparWarnDialog.getReqparWarnDialog(this).addParameter(paramName, true);
                                } else if (arg.equals("real")) {
                                    ReqparWarnDialog.getReqparWarnDialog(this).addParameter(paramName, false);
                                } else {
                                    return;
                                }
                            }
                        }
                    } else if (arg.equals("show")) {
                        centerOnScreen(ReqparWarnDialog.getReqparWarnDialog(this));
                        if (tok.hasMoreTokens()) {
                            // the last arg is a callback string -- see comments
                            // in ReqparWarnDialog
                            arg =tok.nextToken("\n").trim();
                            ReqparWarnDialog.getReqparWarnDialog(this).showDialog(arg);
                        } else {
                            ReqparWarnDialog.getReqparWarnDialog(this).showDialog();
                        }
                    }            			
                }
            }            	
            return;
        }
        if (cmd.equals("mouseMenu")) {
	    //vnmrjcmd('mouseMenu',xmlfile,x,y)
	    // called by right mouse click in certain mode
            if (!tok.hasMoreTokens())
               return;
            data = tok.nextToken().trim();
            String xmlfile=null;
            if (data.startsWith("/")) {
               xmlfile = FileUtil.openPath(data);
            }
            else {
               xmlfile = FileUtil.openPath("INTERFACE/"+data);
            }
            if(xmlfile == null) {
		return;
            }
	    int x=0,y=0; // pixel position of clicking on the canvas
            if (tok.hasMoreTokens()) {
              data = tok.nextToken().trim();
              try {
                 x = Integer.parseInt(data);
              } catch (Exception e) { return; }
	    }
            if (tok.hasMoreTokens()) {
              data = tok.nextToken().trim();
              try {
                 y = Integer.parseInt(data);
              } catch (Exception e) { return; }
	    }
	
	    JPopupMenu menu = new JPopupMenu();
            try {
            	LayoutBuilder.build(menu, this, xmlfile);
            }
            catch(Exception e) { 
                return;
            }
	    //menu.setBackground(Util.getBgColor());
            //menu.setForeground(DisplayOptions.getColor("PlainText"));
            SwingUtilities.updateComponentTreeUI(menu);
	    menu.show(vcanvas,x,y);
            return;
        }

        if (cmd.equals("screenshot") || cmd.equals("screendump")) {
            processScreenShot(tok);
            return;
        }
        if (cmd.equals("seqhelp")) {
            if (!tok.hasMoreTokens())
               return;
            data = tok.nextToken().trim();
            
            CSH_Util.displayCSHelp(data);
            
            return;
        }
        if (cmd.equals("setMatchingOnly")) {
            if (!tok.hasMoreTokens())
               return;
            data = tok.nextToken().trim();

            if (data.equals("on") || data.equals("true")) {
                MatchDialog.setshowMatchingOnly(true);
            }
            else {
                MatchDialog.setshowMatchingOnly(false);
            }
            return;
        }
        if (cmd.equals("setpage")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            String page="";
            if (tok.hasMoreTokens()){
                while(tok.hasMoreTokens()){
                    page+=tok.nextToken().trim();
                    if(tok.hasMoreTokens())
                        page+=" ";
                }
            }
	    data = Util.getLabelString(data);	
	    page = Util.getLabelString(page);	
            // controlPanel = Util.getControlPanel();
            // if(controlPanel !=null){
            //     controlPanel.setParamPage(data,page);
            // }
            paramTabCtrl.setParamPage(data,page);
            return;
        }
        if(cmd.equals("setStatusVariable")) {
            if (!tok.hasMoreTokens()) {
                Messages.postError(
                    "usage: vnmrjcmd(\'setStatusVariable name value\')");
                return;
            }
            processStatusData(tok.nextToken(""));
            return;
        }
        if (cmd.equals("showmshelp")) {
            String whichHelp = "cpr";
            String dir;
            
            dir = System.getProperty("sysdir");
            if (tok.hasMoreTokens()) {
                whichHelp = tok.nextToken().trim();
            }
            
            String[] helpCommand = {"hh.exe",
                                    dir + "/help/" + whichHelp + ".chm"};
            try {
                Runtime.getRuntime().exec(helpCommand);
            } catch (Exception e) {
            }
            return;
        }
        if (cmd.equals("submitstudy")) {
            SmsPanel sPan = ((ExpViewArea)expIf).getSmsPanel();
            data = "day";
            if (tok.hasMoreTokens())
               data = tok.nextToken("\n").trim();
            if (sPan == null || (!sPan.isVisible())) {
               submitEmptyTray(data);
               return;
            }
            sPan.submit(this, data);
            sPan = null;
            return;
        }
        if (cmd.equals("toolbar")) {
            // VjToolBar objToolbar = ((ExperimentIF)appIf).getToolBar();
            if (tok.hasMoreTokens()) {
               data = tok.nextToken("\n").trim();
               if (data.equals("test")) {
                 // if (toolBarTester == null)
                  //    toolBarTester = new ToolBarTester();
                  // toolBarTester.setVisible(true);
                  // toolBarTester.updateValue();
                  return;
               }
            }
            VjToolBar objToolbar = Util.getToolBar();
            if (objToolbar != null)
               objToolbar.processVnmrCmd(str.substring(9));
            return;
        }
        if (cmd.equals("toolpanel")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            if (!tok.hasMoreTokens())
                return;
            String p = tok.nextToken().trim();
            VnmrjIF vjIf = Util.getVjIF();
            if (vjIf != null) 
                vjIf.openComp(data, p);
            // data = Util.getLabelString(data);
            if (data.equals("Locator") || data.equals(Util.getLabelString("Locator"))) {
               if (Util.isShufflerInToolPanel())
                  return;
               if (p.equalsIgnoreCase("open")) {
                  if (locatorPanel == null)
                      locatorPanel = new LocatorPopup();
                  if (locatorPanel != null)  {
                      Util.setLocatorPopup(locatorPanel);
		      locatorPanel.showPanel();
                  }
               }
            }
            return;
        }
        if (cmd.equals("tooltip")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            if (data.equals("off")) {
                ToolTipManager.sharedInstance().setEnabled(false);
                VTipManager.tipManager().setActive(false);
            }
            else if (data.equals("on")) {
                ToolTipManager.sharedInstance().setEnabled(true);
                VTipManager.tipManager().setActive(true);
            }
            return;
        }
        if (cmd.equals("tray")) {
            if (tok.hasMoreTokens())
                processTrayCmd(tok.nextToken("\n"));
            return;
        }
        if (cmd.equals("uiobj")) {
            // VobjUtil.processUiCmd(tok.nextToken("\n"));
            return;
        }
        if (cmd.equals("util")) { // process Utilites menu
            if (!tok.hasMoreTokens())
                    return;
            processUtilCmd(str.substring(9));
            return;
        }
        if (cmd.equalsIgnoreCase("rebuild")) {
            if (DebugOutput.isSetFor("appdirs"))
                Messages.postDebug("vjCmd2: rebuild cmd received");

            processRebuild();
            return;
	}
        if (cmd.equals("viewport")) {
            int a = 1;
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            if (data.equalsIgnoreCase("max") || data.equalsIgnoreCase("maximize")) {
                ((ExpViewArea)expIf).maximizeViewport(winId,true);
                return;
            }
            if (data.equalsIgnoreCase("min") || data.equalsIgnoreCase("minimize")) {
                ((ExpViewArea)expIf).maximizeViewport(winId,false);
                return;
            }
            if (data.equalsIgnoreCase("select")) {
                if (!tok.hasMoreTokens())
                    return;
                data = tok.nextToken().trim();
                try {
                    a = Integer.parseInt(data);
                }
                catch (NumberFormatException er) { return; }
                a--;
                if (!tok.hasMoreTokens())
                    return;
                data = tok.nextToken().trim();
                if (data.equalsIgnoreCase("off"))
                    expIf.setCanvasAvailable(a, false);
                else
                    expIf.setCanvasAvailable(a, true);

		((ExpViewArea)expIf).processOverlayCmd("overlayMode");
                return;
            }
            if (data.equalsIgnoreCase("default"))
                setDefaultVp();
            else
            {
                setVp(data);
            }
            return;
        }
	if (cmd.equals("mergeResourceFiles")) {
          if (!tok.hasMoreTokens()) return;
          String file1 = tok.nextToken().trim();
          if (!tok.hasMoreTokens()) return;
          String file2 = tok.nextToken().trim();

	  String option="all";
          if (tok.hasMoreTokens()) option = tok.nextToken().trim();

          Util.mergeResourceFiles(file1, file2, option);
          return;
        }
	if (cmd.equals("writeResourceFiles")) {
          if (!tok.hasMoreTokens()) return;
          String path = tok.nextToken().trim();
          Util.writeResourceFiles(path);
          return;
        }
	if (cmd.equals("writeToFrameTitle")) {
            // VNMRFrame.appendStringToTitle(str.substring(27).trim());
            if (helpBar != null) {
                helpBar.setMessage(str.substring(27).trim());
                buttonPan.repaint();
            }
            return;
        }
        if (cmd.equals("overlayMode")) {
	    // this is to set viewport overlay mode.
            if (!active)
                return;
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            num = 0;
            try {
                    num = Integer.parseInt(data);
            }
            catch (NumberFormatException er) { return; }
	    ((ExpViewArea)expIf).setOverlayMode(num);
            return;
        }
        if (cmd.equals("overlaySpec")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            ((ExpViewArea)expIf).processOverlayCmd(data);
            return;
	}
        if (cmd.equals("setcolor")) {
/*
            if (!active)
                return;
*/
	    if(tok.countTokens() < 3) 
                return;
            data = tok.nextToken().trim();
            int win = 0;
            try {
                    win = Integer.parseInt(data);
            }
            catch (NumberFormatException er) { return; }

            data = tok.nextToken().trim();
            try {
                    num = Integer.parseInt(data);
            }
            catch (NumberFormatException er) { return; }
            if (win > 0 ) {
                if (!tok.hasMoreTokens())
                    return;
                data = tok.nextToken().trim();
		Color c=DisplayOptions.getColor(data);
		int red = c.getRed();
            	int grn = c.getGreen();
            	int blu = c.getBlue();
		String vcmd = "setcolor('graphics',"+num+","+red+","+grn+","+blu+")"; 
                expIf.sendToVnmr(win, vcmd);
            }
            return;
        }
        if (cmd.equals("visible")) {
            if (!tok.hasMoreTokens())
               return;
            data = tok.nextToken().trim();
            try {
                    num = Integer.parseInt(data);
            }
            catch (NumberFormatException er) { return; }
            if (!tok.hasMoreTokens())
               return;
            data = tok.nextToken().trim();
            boolean bShow = true;
            if (data.equals("off") || data.equals("false"))
                bShow = false;
            if (num <= 0)
                return;
	    ((ExpViewArea)expIf).setCanvasVisible(num-1, bShow);
            return;
        }
		if (cmd.equals("vnmr_jplot")) {
			if (!tok.hasMoreTokens())
				return;
			final String jargs = tok.nextToken().trim();
			String sysDir = System.getProperty("sysdir");
			String userDir = System.getProperty("userdir");
			String user = System.getProperty("user");

			String args = "-Dsysdir=\"" + UtilB.unixPathToWindows(sysDir)+"\"";
			args += " -Duserdir=\""+UtilB.unixPathToWindows(userDir)+"\"";
			args += " -Duser=" + user;
			String java = FileUtil.SYS_VNMR+"/jre/bin/java";
			String jplot = UtilB.unixPathToWindows(sysDir+ "/java/jplot.jar PlotConfig");
			if (UtilB.iswindows())
				java += ".exe";
			String[] cmds = { WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
					java + " " + args + " -cp " + jplot + " "+jargs };

		    WUtil.runScriptInThread(cmds);
			return;
		}
        if (cmd.equals("vpactive")) {
/*
            if (!active)
                return;
*/

            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            setVp(data);
            return;
        }
        if (cmd.equals("vpgraphics")) {
/*
            if (!active)
                return;
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            if (data.equals("on")) {
                appIf.setViewPort(winId, 0);
            }
*/
            return;
        }
        if (cmd.equals("vplayout")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            if (!tok.hasMoreTokens())
                return;
            if (data.equals("use")) {
                int k = appIf.getVpId();
                data = tok.nextToken().trim();
                try {
                    num = Integer.parseInt(data);
                }
                catch (NumberFormatException er) { return; }
                num--;
                if (k == num) {
                    // Util.openUiLayout(data, num);
                    appIf.switchLayout(num);
                }
                return;
            }
            if (data.equals("save"))
                Util.saveUiLayout(tok.nextToken().trim());
            return;
        }
        if (cmd.equals("vpnum")) {
            if (tok.hasMoreTokens()) {
                try {
                   num = Integer.parseInt(tok.nextToken());
                }
                catch (NumberFormatException er) { num = 0; }
            }
            if (num < 1)
                return;
            String data2 = null;
            data = null;
            if (tok.hasMoreTokens())
                data = tok.nextToken().trim();
            if (tok.hasMoreTokens())
                data2 = tok.nextToken().trim();
            expIf.setCanvasNum(num, data, data2);
            return;
        }
        if (cmd.equals("vppopup"))
        {
            if (!tok.hasMoreTokens())
                return;
            processVpPopupCmd(str.substring(9));
            return;
        }
        if (cmd.equals("window")) {
            if (!tok.hasMoreTokens())
               return;
            int a = 1;
            int b = 1;
            try {
                a = Integer.parseInt(tok.nextToken());
                b = Integer.parseInt(tok.nextToken());
            }
            catch (NumberFormatException er) { return; }
            expIf.setCanvasArray(a, b);
            return;
        }
        if (cmd.equals("native")) {
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            int xwin = 0;
            if (data.equals("on") || data.equals("yes"))
                xwin = Util.getWindowId();
            if (xwin > 0) {
                if (!bNative) {
                    bNative = true;
                    sendCanvasId();
                    vcanvas.setNativeGraphics(true);
                }
            }
            else {
                if (bNative) {
                    bNative = false;
                    sendCanvasId();
                    vcanvas.setNativeGraphics(false);
                }
            }
            return;
        }
        if (cmd.equals("repaint")) {
            vcanvas.repaint();
            return;
        }
        if (cmd.equals("verbose")) {
            bVerbose = true;
            if (!tok.hasMoreTokens())
                return;
            data = tok.nextToken().trim();
            if (data.equals("on") || data.equals("yes"))
                vcanvas.setVerbose(true);
            else {
                bVerbose = false;
                vcanvas.setVerbose(false);
            }
            return;
        }
    }
    
    /**
     * Show a preview window for the default plotFile.
     */
    private void previewPlotfile(String orient, boolean noRefresh) {
        XJButton refreshButton;
        XJButton printButton;
        XJButton closeButton;
        XJButton copyButton;
        XJButton pdfButton;
        Insets buttonInsets = new Insets(5, 5, 5, 5);
        String localSysdir = System.getProperty("sysdir");      
        String inputPath = localSysdir + "/tmp/printpreview.ps";
        String outputPath = localSysdir + "/tmp/printpreview.png";
        
        
        if (!VPrintDialog.generatePngFile(inputPath, outputPath, true)) {
            // TODO: send/display error message?
            return;
        }
               
        Image image = null;
        Icon icon = null;
        try {
            image = ImageIO.read(new File(outputPath));
            icon = new ImageIcon(image);
            if (orient.equals("landscape")
                && icon.getIconWidth() < icon.getIconHeight())
            {
                image = ImageUtil.rotateImage(image);
                icon = new ImageIcon(image);
            }
        } catch (IOException e) {
            Messages.postError("Cannot open " + outputPath);
        }
        new ImageIcon(outputPath);
        JLabel imagePane = new JLabel(icon);
        String title = Util.getLabelString("Print Preview");
        if (sm_previewPopup == null) {
            sm_previewPopup = new JDialog(VNMRFrame.getVNMRFrame(), title);
        } else {
            sm_previewPopup.getContentPane().removeAll();
        }
        JPanel buttonBar = new JPanel();

        String pgWidth = Fmt.fg(2, VPrintDialog.getPageWidth());
        String pgHeight = Fmt.fg(2, VPrintDialog.getPageHeight());
        title = title + "   (" + pgWidth + " x " + pgHeight + ")";
        sm_previewPopup.setTitle(title);

        String label;
        Icon recycleIcon = Util.getImageIcon("recycleXbg");
        label = Util.getTooltipString("Refresh Preview");
        refreshButton = new XJButton(recycleIcon);
        refreshButton.setBorderInsets(buttonInsets);
        refreshButton.setToolTipText(label);
        if (noRefresh) {
                refreshButton.setEnabled(false);
        } else {
            refreshButton.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        sendToActiveVnmr("print('preview')");
                    }
                }
            );
        }
        buttonBar.add(refreshButton);

        Icon printIcon = Util.getImageIcon("print2");
        label = Util.getLabelString("Print...");
        printButton = new XJButton(printIcon);
        printButton.setBorderInsets(buttonInsets);
        printButton.setToolTipText(label);
        buttonBar.add(printButton);
        printButton.addActionListener(
                new ActionListener() {
                    // need to get these again here because inner class can't
                    // see them????
                    String mm_sysdir = System.getProperty("sysdir");
                    String mm_inputPath = mm_sysdir + "/tmp/printpreview.ps";
                    String mm_outputPath = mm_sysdir + "/tmp/printpreview.png";
                    public void actionPerformed(ActionEvent e) {
                        VPrintDialog.generatePngFile(mm_inputPath,
                                                     mm_outputPath,
                                                     false);
                        new VPrintDialog(mm_outputPath,
                                         DocFlavor.INPUT_STREAM.PNG,
                                         mm_inputPath);
                    }
                }
            );

        Icon copyIcon = Util.getImageIcon("Copy_24");
        copyButton = new XJButton(copyIcon);
        copyButton.setBorderInsets(buttonInsets);
        label = Util.getTooltipString("Copy plot to clipboard");
        copyButton.setToolTipText(label);
        buttonBar.add(copyButton);
        copyButton.addActionListener(
                new ActionListener() {
                    String mm_sysdir = System.getProperty("sysdir");
                    String mm_inputPath = mm_sysdir + "/tmp/printpreview.ps";
                    String mm_outputPath = mm_sysdir + "/tmp/printpreview.png";
                    public void actionPerformed(ActionEvent e) {
                        VPrintDialog.generatePngFile(mm_inputPath,
                                mm_outputPath,
                                false);
                        copyImageFile(mm_outputPath);
                    }
                }
            );

        Icon pdfIcon = Util.getImageIcon("ExportPdf");
        pdfButton = new XJButton(pdfIcon);
        pdfButton.setBorderInsets(buttonInsets);
        label = Util.getTooltipString("Export to PDF file");
        pdfButton.setToolTipText(label);
        buttonBar.add(pdfButton);
        pdfButton.addActionListener(
                new ActionListener() {
                    String mm_pdfDir = System.getProperty("userdir");
                    String mm_sysdir = System.getProperty("sysdir");
                    String mm_inputPath = mm_sysdir + "/tmp/printpreview.ps";
                    public void actionPerformed(ActionEvent e) {
                        mm_pdfDir = VPrintDialog.generatePdfFile(mm_inputPath,
                                mm_pdfDir, null);
                    }
                }
            );

       Icon closeIcon = Util.getImageIcon("exit_24");
        label = Util.getLabelString("Close Preview");
        closeButton = new XJButton(closeIcon);
        closeButton.setBorderInsets(buttonInsets);
        closeButton.setToolTipText(label);
        buttonBar.add(closeButton);
        closeButton.addActionListener(
                new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        sm_previewPopup.dispose();
                    }
                }
            );

        Container content = sm_previewPopup.getContentPane();
        //content.setLayout(new BoxLayout(content, BoxLayout.Y_AXIS));
        content.add(buttonBar, BorderLayout.NORTH);
        content.add(imagePane, BorderLayout.CENTER);
        sm_previewPopup.pack();
        sm_previewPopup.setVisible(true);
    }

    /**
     * Copy the specified image file to the clipboard.
     * @param path
     */
    public void copyImageFile(String path) {
        try {
            Clipboard clipboard = Toolkit.getDefaultToolkit()
                .getSystemClipboard();
            ImageFileTransferable imageTransferable
                = new ImageFileTransferable(path);
            imageTransferable.exportToClipboard(this, clipboard,
                                                TransferHandler.COPY);
        } catch (Exception e) {
            //e.printStackTrace();
            Messages.postError("Error copying file: " + path);
            Messages.writeStackTrace(e);
        }
    }


    // help popup util array display edit toolbar batch feval exit
    // free busy window layout tooltip
    // ***********************************************************
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // there are too many vjcmds, for convenience and effiency,
    // they were divided into several groups.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // ***********************************************************
    private static VJOpenPopup openPanel=null;
    private static VJSavePopup savePanel=null;
    private static VJFileBrowser browserPanel=null;
    private static JDialog fpDialog = null;
    private static JFileChooser fpPanel = null;
    
    // static method to reset browsers and profile panel to null.  This
    // needs to take place when the operator is changed so that new ones
    // will be created.
    static public void resetBrowsers() {
        if(openPanel != null) {
            openPanel.destroyPanel();
            openPanel = null;
        }
        if(savePanel != null) {
            savePanel.destroyPanel();
            savePanel = null;
        }
        if(browserPanel != null) {
            browserPanel.destroyPanel("browser");
            browserPanel = null;
        }
        if(profilePanel != null) {
            profilePanel.destroyPanel();
            profilePanel = null;
        }
        if(expSelTabEditor != null) {
            expSelTabEditor.destroyPanel();
            expSelTabEditor = null;
        }
        if(expSelTreeEditor != null) {
            expSelTreeEditor.destroyPanel();
            expSelTreeEditor = null;
        }

    }
    
    static public VJFileBrowser getBrowserPanel() {
         return browserPanel;
    }

    static public void setBrowserPanel(VJFileBrowser panel) {
        browserPanel = panel;
    }

    static public ProtocolBrowser getProtocolBrowser() {
        if (protocolBrowser == null)
            protocolBrowser = new ProtocolBrowser();
        return protocolBrowser;
    }
    
    public void  processRebuild() {
        ExpSelector.setForceUpdate(true);
	Util.setAppDirs(m_appDirs, m_appDirLabels, true);
    }

    private void openFpBrowser(String cmd) {
        File dir;

        if (fpDialog == null) {
             fpDialog = new JDialog();
             fpPanel = new JFileChooser();
             fpDialog.add(fpPanel);
             fpPanel.setApproveButtonText("OK");
             fpPanel.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
             fpPanel.setMultiSelectionEnabled(true);
             fpDialog.setAlwaysOnTop(true);
             fpDialog.toFront();
             String f = (String)sshare.getProperty("fpDir");
             if (f == null) {
                 f = FileUtil.openPath("USER"+File.separator+"fingerprintlib");
                 if (f == null)
                     f = FileUtil.openPath("USER");
             }
             if (f != null) {
                 dir = new File(f);
                 if (dir.exists())
                     fpPanel.setCurrentDirectory(dir);
             }
             SwingUtilities.updateComponentTreeUI(fpPanel);
        }
        int ret = fpPanel.showOpenDialog(ModelessPopup.getContainer() );
        if (ret != JFileChooser.APPROVE_OPTION)
             return;
        File[] files = fpPanel.getSelectedFiles();
        if (files.length < 1)
             return;
        StringBuffer sb = new StringBuffer(cmd).append("(");
        for (int n = 0; n < files.length; n++) {
             if (files[n] != null) {
                 if (n > 0)
                     sb.append(",");
                 if (files[n].isDirectory())
                     sb.append("'directory','");
                 else
                     sb.append("'file','");
                 sb.append(files[n].getAbsolutePath()).append("'");
             }
        }
        sb.append(")\n");
        sendToVnmr(sb.toString());
        dir = fpPanel.getCurrentDirectory();
        sshare.putProperty("fpDir", dir.getAbsolutePath());
    }

    public void  processAppDir(String  str) {
        StringTokenizer tok = new StringTokenizer(str, " ,\n");
        tok.nextToken();  // skip keyword.

        int ind = 0;
        if (!tok.hasMoreTokens())
            return;
        try {
            ind = Integer.parseInt(tok.nextToken());
        }
        catch (NumberFormatException er) { return; }

		if (ind == 0) {
			m_appDirs.clear();
			m_appDirLabels.clear();
			Util.suspendUI();
		} else if (tok.hasMoreTokens()) {
			String dir = tok.nextToken(";\n").trim();
			m_appDirs.add(FileUtil.getAppdirPath(dir));
			if (tok.hasMoreTokens()) {
				m_appDirLabels.add(tok.nextToken().trim());
			} else {
				m_appDirLabels.add("appdir " + ind);
			}
		}
	}
    
    public ArrayList<String> getAppDirLabels() {
        return m_appDirLabels;
    }

    public void  processVjCmd(String  str) {
        QuotedStringTokenizer tok = new QuotedStringTokenizer(str, " ,\n");
        tok.nextToken();
        String cmd = tok.nextToken().trim();
        char  ch1 = cmd.charAt(0);

        if (bDebugUiCmd) {
            logWindow.append(VnmrJLogWindow.UICMD, winId, null, str);
        }
        if (ch1 >= 'a') {
            if (ch1 >= 'm')
                vjCmd2(str, cmd, tok);
            else
                vjCmd(str, cmd, tok);
            return;
        }
        if (cmd.equals("WINDOW"))
        {
            if (!tok.hasMoreTokens())
                return;
            cmd = tok.nextToken().trim();
            if (cmd.equalsIgnoreCase("hide")) {
               VNMRFrame.getVNMRFrame().setVisible(false);
            }
            else if (cmd.equalsIgnoreCase("icon")) {
               VNMRFrame.getVNMRFrame().setState(Frame.ICONIFIED);
            }
            else if (cmd.equalsIgnoreCase("normal")) {
               VNMRFrame.getVNMRFrame().setState(Frame.NORMAL);
            }
            else if (cmd.equalsIgnoreCase("show")) {
               VNMRFrame.getVNMRFrame().setVisible(true);
            }
            return;
        }
        if (cmd.equals("RQ")) {
            RQPanel rq=Util.getRQPanel();
            if(rq!=null && tok.hasMoreTokens()){
                String s=str.substring(11);
                rq.processCommand(s.trim());
            }
            return;
        }
        if (cmd.startsWith("SQ")) {
            Messages.postDebug("SQcmd", "processVjCmd: cmd=\"" + cmd + "\""
                    + ", msg=\"" + str + "\"");
            if (str.matches(".*\\WSQ +ignoreInactiveVp\\W.*")) {
                // NB: turn it off if there is a "0" argument
                m_sqIgnoreInactiveVp = !str.matches(".*\\W0+\\W.*");
            } else {
                sqBusy = true;
                QueuePanel sq = qPanel;
                if (sq == null)
                    sq = Util.getStudyQueue(cmd);
                boolean ok = sq != null && tok.hasMoreTokens();
                if (m_sqIgnoreInactiveVp) {
                    int thisVp = getViewId();
                    int actVp = ((ExpViewArea)getExpView()).getActiveWindow();
                    ok &= (thisVp == actVp);
                }
                if (ok) {
                    String s=str.substring(9+cmd.length());
                    sq.processCommand(s.trim());
                }
                sqBusy = false;
            }
            return;
        }
        if (cmd.equals("PRINT"))
        {
            if (!tok.hasMoreTokens())
                return;
            String strcmd = str.substring(14);
            processPrintcmd(strcmd);
/*
            final String strcmd = str.substring(14);
            new Thread(new Runnable()
            {
                public void run()
                {
                    processPrintcmd(strcmd);
                }
            }).start();
*/
            return;
        }
        if (cmd.equals("GRAPHICS"))
        {
            if (!tok.hasMoreTokens())
                return;
            processGraphicsCmd(0, tok);
            return;
        }
        if (cmd.equals("GRAPHICS1"))
        {
            if (!tok.hasMoreTokens())
                return;
            processGraphicsCmd(1, tok);
            return;
        }
        if (cmd.equals("GRAPHICS2"))
        {
            if (!tok.hasMoreTokens())
                return;
            processGraphicsCmd(2, tok);
            return;
        }
        if (cmd.equals("StatusMessages"))
        {
            if (!tok.hasMoreTokens())
                return;
            cmd = tok.nextToken().trim();
            try
            {
                int number = Integer.parseInt(cmd);
                Util.setStatusMessagesNumber(number);
                sshare.putProperty("StatusMessages", cmd);
            }
            catch (Exception e) {}
            return;
        }
        if (cmd.startsWith("CRYO")) {
            // vnmrjcmd('CRYO', ...)
            Messages.postDebug("cryocmd", "CRYO command: " + cmd);
            if (!tok.hasMoreTokens())
                return;
            cmd = tok.nextToken().trim();
            try
            {
                processCryoCmd(cmd);
            }
            catch (Exception e) {
                Messages.postDebug("CRYOBAY exception: " + e);
                Messages.writeStackTrace(e);
            }
            return;
        }
        if (cmd.startsWith("LogToCSVPanel")) {
            // vnmrjcmd('LogToCSV')  No Arguments
            if(logToCSVPanel == null) {
                String sysDir = System.getProperty("sysdir");
                String logFilepath = sysDir + File.separator + "adm" + File.separator + 
                                     "accounting"+ File.separator + "acctLog.xml";
                logToCSVPanel = new LogToCSVPanel(logFilepath); 
            }
               
            logToCSVPanel.showPanel();

            return;
        }
        /* Locator vnmrjcmd's.
         * The general command syntax is:
         *    vnmrjcmd('LOC add/addlist/remove objtype filename/fullpath
         *              [-attr attr value] [-nodis] [-tray]
         * For example: "vnmrjcmd('LOC add protocol protocol2.xml')"
         *        would add the file protocol2.xml in the user default protocol
         *        directory to the locator.  Giving a fullpath in place of the
         *        filenamewill use that fullpath.
         * The command: "vnmrjcmd('LOC remove protocol protocol2.xml')"
         *    would remove that same file from the locator.
         *
         * The optional [attr value] args are to set an attribute by the name
         *    of 'attr', to a value of 'value' when adding.  This is general,
         *    but is specifically created so that one can save a study and
         *    specify that its 'automation' attribute be set to the fullpath
         *    for its parent automation.
         *  The command to add a study to an automation would be:
         *    "vnmrjcmd('LOC add study study1 automation /home/.../auto1')"
         *
         * Spotter_types available are: vnmr_data, panels_n_components, shims,
         *                              study, automation, protocol and images.
         */
        if (cmd.equals("LOC")) {
            if (!tok.hasMoreTokens()) {
                Messages.postError(
                    "usage: vnmrjcmd(\'LOC add/addlist/remove/addstatement/" +
                    "savestatement/removestatement/show/trashfile\')");
                return;
            }
            cmd = tok.nextToken().trim();
            if(cmd.equals("add") || cmd.equals("remove")) {
                // It is add or remove, get the objType and filename
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC add locator_type filename\')"
                        + "\n    No args were found after \'add\'.");
                    return;
                }
                String type = tok.nextToken().trim();
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC add locator_type filename\')"
                        + "\n    Only one arg was found after \'add\'.");
                    return;
                }
                String filename = tok.nextToken().trim();
                if (Util.iswindows())
                    filename = UtilB.unixPathToWindows(filename);

                if(cmd.equals("add")) {

                    String attr=null;
                    String value=null;
                    // Check to see if we have the two optional
                    // args, attr & value
                    if (tok.hasMoreTokens()) {
                        attr = tok.nextToken().trim();
                    }
                    if (tok.hasMoreTokens()) {
                        value = tok.nextToken().trim();
                    }

                    if(attr != null && value != null) {
                        // We had the two optional args and now have attr and
                        // value. If attr is the name of an objtype, then it
                        // is probably being used to set study or automation
                        // attributes to host:fullpath. In that case, we
                        // only need to be given fullpath.  We then need to
                        // convert it to the actual path and host on the
                        // actual computer where the study or automation is
                        // located.
                        boolean foundit = false;
                        for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
                            if(attr.equals(Shuf.OBJTYPE_LIST[i])) {
                                foundit = true;
                                break;
                            }
                        }
                        if(foundit) {
                            // attr is an objtype, do the conversion
                            Vector mp = MountPaths.getPathAndHost(value);
                            String dhost = (String) mp.get(Shuf.HOST);
                            String dpath = (String) mp.get(Shuf.PATH);
                            value = dhost + ":" + dpath;
                        }
                        ArrayList<String> fn = new ArrayList<String>();
                        fn.add(filename);
                        addToShuffler(type, fn, attr, value, false, false);
                    }
                    else {
                        ArrayList<String> fn = new ArrayList<String>();
                        fn.add(filename);
                        // If only attr was set and it equals 'updatelocdis',
                        // then add to shuffler and update the display
                        if(attr != null && attr.equals("updatelocdis")) 
                            addToShuffler(type, fn, null, null, true, false);
                        else
                            addToShuffler(type, fn);
                        
                        // If this is a protocol, update the ProtocolBrowser
                        if(type.equals(Shuf.DB_PROTOCOL)) {
                            if(protocolBrowser != null) {
                                UpdatePBResultsViaThread td;
                                td = new UpdatePBResultsViaThread(protocolBrowser);
                                td.setPriority(Thread.MIN_PRIORITY);
                                td.start();  
                            }
                                
                        }
                    }

                }
                else if(cmd.equals("remove")) {
                    removeFromShuffler(type, filename);
                }
                return;
            }
            // Add a space or ';' separated list of files in the form eg.,
            // LOC addlist study /home/user/auto1/study1 /home/user/auto1/study2
            //       /home/user/auto1/study3 [-attr attr value] [-nodir] [-tray]
            // I added '-attr' to that part so that all options start with '-'
            // so that I can take all items from 'study' to '-' or end of line
            // to be the list of files.
            // -nodis means don't update the locator display
            // -tray means update the tray display
            if(cmd.equals("addlist")) {
                // Get the objType
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC addlist locator_type "
                        + "filename_list\')"
                        + "\n    No args were found after \'add\'.");
                    return;
                }
                String type = tok.nextToken().trim();
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC addlist locator_type "
                        + "filename_list\')"
                        + "\n    Only one arg was found after \'add\'.");
                    return;
                }

                // Get all items up to "-" or end of line
                ArrayList<String> filenames = new ArrayList<String>();
                String item;
                boolean locDisplayUpdate = true;
                boolean trayUpdate = false;
                String attr=null;
                String value=null;


                while(tok.hasMoreTokens()) {
                    item = tok.nextToken().trim();
                    // Do we have an option?
                    if(item.startsWith("-")) {
                        if(item.equals("-nodis"))
                            locDisplayUpdate = false;
                        else if(item.equals("-tray"))
                            trayUpdate = true;
                        else if(item.equals("-attr")) {
                            if(tok.hasMoreTokens()) {
                                attr = tok.nextToken().trim();
                            }
                            if (tok.hasMoreTokens()) {
                                value = tok.nextToken().trim();
                            }
                        }
                    }
                    else {
                        // Not an option, must be a filename, add it
                        if(UtilB.iswindows())
                                item=UtilB.unixPathToWindows(item);
                        filenames.add(item);
                    }
                }
                if(attr != null && value != null) {
                    // We had the two optional args and now have attr and
                    // value. If attr is the name of an objtype, then it
                    // is probably being used to set study or automation
                    // attributes to host:fullpath. In that case, we
                    // only need to be given fullpath.  We then need to
                    // convert it to the actual path and host on the
                    // actual computer where the study or automation is
                    // located.
                    boolean foundit = false;
                    for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
                        if(attr.equals(Shuf.OBJTYPE_LIST[i])) {
                            foundit = true;
                            break;
                        }
                    }
                    if(foundit) {
                        // attr is an objtype, do the conversion
                        Vector mp = MountPaths.getPathAndHost(value);
                        String dhost = (String) mp.get(Shuf.HOST);
                        String dpath = (String) mp.get(Shuf.PATH);
                        value = dhost + ":" + dpath;
                    }
                }
                addToShuffler(type, filenames, attr, value,
                              locDisplayUpdate, trayUpdate );
                return;
            }
            if(cmd.equals("addstatement") || cmd.equals("savestatement")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC addstatement name label\')");
                    return;
                }
                String name = tok.nextToken().trim();
                String label;
                if (tok.hasMoreTokens()) {
                    label = tok.nextToken().trim();
                }
                else {
                    label = "**skip**";
                }
                SessionShare session = ResultTable.getSshare();
                StatementHistory history = session.statementHistory();
                history.writeCurStatement(name, label);
                // Get number from name and append to 'S' making something
                // like S1, etc and tell user it was saved.  It should always
                // be the last character.  Unfortunately, it starts at )
                // and the S numbers start at 1.  We need to turn it into
                // an int and add one.
                String num = name.substring(name.length() -1, name.length());
                int inum = Integer.parseInt(num) + 1;
                
                sendToVnmr("write('line3', 'Save Statement S" + inum + "')");
                return;
            }
            if(cmd.equals("removestatement")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC removestatement name\')");
                    return;
                }
                String name = tok.nextToken().trim();
                SessionShare session = ResultTable.getSshare();
                StatementHistory history = session.statementHistory();

                // Remove the file
                history.removeSavedStatement(name);
                return;
            }
            if(cmd.equals("loadstatement")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC loadstatement name\')");
                    return;
                }
                String name = tok.nextToken().trim();
                SessionShare session = ResultTable.getSshare();
                StatementHistory history = session.statementHistory();
                history.readNamedStatement(name);
                return;
            }
            if(cmd.equals("show")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC show category\')");
                    return;
                }
                String objType = tok.nextToken().trim();

                LocatorHistory lh = sshare.getLocatorHistory();
                // Update locator to the most recent statement for this type
                lh.setHistoryToThisType(objType);
                return;

            }
            if(cmd.equals("trashfile")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC trashfile fullpath category\')");
                    return;
                }

                String objType=null;
                boolean bProtocolBrowser = false;
                String fullpath = tok.nextToken().trim();
                
                // Don't allow deleting of system protocols
                String sysdir = System.getProperty("sysdir");
                if(sysdir != null) {
                    UNFile sysfile = new UNFile(sysdir);
                    if(sysfile != null) {
                        // Need to compare the canonical path
                        String csysdir = sysfile.getCanonicalPath();
                        if(fullpath.startsWith(csysdir + "/")) {
                            Messages.postError("Cannot Delete System Files");
                            return;
                        }
                    }
                }


                
                if (tok.hasMoreTokens()){
                    // The last arg is optional.  It is sometimes the objType
                    // and sometimes used to desginate that this call originated
                    // from the ProtocolBrowser.
                    String arg = tok.nextToken().trim();
                    if(arg.equals("protocolBrowser"))
                        bProtocolBrowser = true;
                    else
                        objType = tok.nextToken().trim();
                }
                if (objType == null) {
                    // If objtype is not given, look for it.
                    objType = FillDBManager.getType(fullpath);
                    if(objType.equals("?")) {
                        Messages.postError(
                                "Could not determine objType of " + fullpath);
                        return;
                    }
                }

                // Get the current user name and host for creating a TrashItem
                String user = System.getProperty("user.name");

                /***********
                String hostName;
                try {
                    InetAddress inetAddress = InetAddress.getLocalHost();
                    hostName = inetAddress.getHostName();
                }
                catch(Exception e) {
                    Messages.postError(
                             "Error getting HostName for trashfile cmd");
                    return;
                }
                ***********/

                // Now we need to extract the filename from the fullpath given
                int index = fullpath.lastIndexOf(File.separator);
                String filename = fullpath.substring(index + 1);

                // fullpath and hostFullPath here need to be direct,
                // not mounted paths
                String dpath, dhost;
                try {
                    Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
                    dhost = (String) mp.get(Shuf.HOST);
                    dpath = (String) mp.get(Shuf.PATH);
                }
                catch (Exception e) {
                    Messages.postError("Problem getting cononical path for\n"
                                       + fullpath);
                    Messages.writeStackTrace(e);
                    return;
                }

                String hostFullpath = dhost + ":" + dpath;

                TrashItem trash = new TrashItem(filename, dpath, objType,
                                                dhost, user, hostFullpath, bProtocolBrowser);
                trash.trashIt();

                return;
            }
            if(cmd.equals("set")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC set variable value\')");
                    return;
                }
                String variable = tok.nextToken().trim();
                String value;
                if (tok.hasMoreTokens()) {
                    value = tok.nextToken().trim();
                }
                else {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC set variable value\')");
                    return;
                }
                if(variable.equals("maxLocItemsToShow")) {
                    ShufDBManager.setMaxLocItemsToShow(value);
                }
                return;
            }
            if(cmd.equals("saveBrowserBntDir")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC saveBrowserBntDir 1or2\')");
                    return;
                }
                String value = tok.nextToken().trim();
                int whichOne = Integer.parseInt(value);
                LocatorBrowser locatorBrowser;
                locatorBrowser = LocatorBrowser.getLocatorBrowser();
                if(locatorBrowser != null)
                    locatorBrowser.setDirBtnPath(whichOne);
                return;
            }
            if(cmd.equals("setBrowserDir")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC setBrowserDir 1or2\')");
                    return;
                }
                String value = tok.nextToken().trim();
                int whichOne = Integer.parseInt(value);
                LocatorBrowser locatorBrowser;
                locatorBrowser = LocatorBrowser.getLocatorBrowser();
                if(locatorBrowser != null)
                    locatorBrowser.setBrowserToDirBtnPath(whichOne);
                return;
            }
            if(cmd.equals("saveVJBrowserBtnDir")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC saveVJBrowserBtnDir Btn#\')");
                    return;
                }
                String value = tok.nextToken().trim();
                int whichOne = Integer.parseInt(value);
                if(browserPanel != null)
                    browserPanel.setDirBtnPath(whichOne);
                return;
            }
            if(cmd.equals("setVJBrowserBtnDir")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC setVJBrowserBtnDir Btn#\')");
                    return;
                }
                String value = tok.nextToken().trim();
                int whichOne = Integer.parseInt(value);
                if(browserPanel != null)
                    browserPanel.setBrowserToDirBtnPath(whichOne);
                return;
            }
            
            if(cmd.equals("saveOpenBtnDir")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC saveOpenBtnDir Btn#\')");
                    return;
                }
                String value = tok.nextToken().trim();
                int whichOne = Integer.parseInt(value);
                if(openPanel != null)
                    openPanel.setDirBtnPath(whichOne);
                return;
            }
            if(cmd.equals("saveSaveBtnDir")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC saveSaveBtnDir Btn#\')");
                    return;
                }
                String value = tok.nextToken().trim();
                int whichOne = Integer.parseInt(value);
                if(savePanel != null)
                    savePanel.setDirBtnPath(whichOne);
                return;
            }
            if(cmd.equals("setOpenBtnDir")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC setOpenBtnDir Btn#\')");
                    return;
                }
                String value = tok.nextToken().trim();
                int whichOne = Integer.parseInt(value);
                if(openPanel != null)
                    openPanel.setBrowserToDirBtnPath(whichOne);
               return;
            }
            if(cmd.equals("setSaveBtnDir")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC setSaveBtnDir Btn#\')");
                    return;
                }
                String value = tok.nextToken().trim();
                int whichOne = Integer.parseInt(value);
                if(savePanel != null)
                    savePanel.setBrowserToDirBtnPath(whichOne);
               return;
            }
            // saveAsPanel
            if(cmd.endsWith("saveAsPanel")) {
                if(savePanel == null)
                    savePanel = new VJSavePopup();
                savePanel.showPanel("Save");
                return;
            }
            // vnmrj file Browser use vc = "vnmrjcmd('LOC openDataPanel')" 
            if(cmd.endsWith("openDataPanel")) {
            	if(openPanel == null) 
                   openPanel = new VJOpenPopup();                
                openPanel.showPanel("Open");
                return;
            }
            // vj file Browser use vc = "vnmrjcmd('LOC openVJBrowser')"
            if(cmd.endsWith("openVJBrowser")) {
            	if(openPanel == null) 
                   openPanel = new VJOpenPopup();                
                openPanel.showPanel("Open");
                return;
            }
            // system file Browser use vc = "vnmrjcmd('LOC openFileBrowser')" 
            if(cmd.endsWith("openFileBrowser")) {               
                VFileDialog vfileDialog; // modal dialog: rebuild each call
                vfileDialog = new VFileDialog(VNMRFrame.getVNMRFrame(),
                                              FileDialog.LOAD);
                vfileDialog.setVisible(true);
                return;
            }
            // browserPanel
            if(cmd.endsWith("browserPanel")) {
                if(browserPanel == null)
                    // This will set browserPanel
                    new VJBrowserPopup("Browser");
                if (tok.hasMoreTokens()) {
                   browserPanel.updateTree(tok.nextToken().trim());
                }
                browserPanel.showPanel("Browser");
                return;
            }
            // locatorPanel
            if(cmd.endsWith("locatorPanel")) {
                // Only make one of these.  The locator is not set up
                // for two to run at the same time.
                if(FillDBManager.locatorOff()) {
                    Messages.postWarning("The Locator has been turned off in \'vnmrj adm\'. Aborting!");
                    return;
                }
                if (Util.isShufflerInToolPanel()) {
                    VnmrjIF vjIf = Util.getVjIF();
                    if (vjIf != null) {
                        vjIf.openComp("Locator", "open");
                        return;
                    }
                }
                if (locatorPanel == null)
                    locatorPanel = new LocatorPopup();
                if (locatorPanel != null) {
                    Util.setLocatorPopup(locatorPanel);
                    locatorPanel.showPanel();
                }
                return;
            }
            // protocolBrowser
            if(cmd.endsWith("protocolBrowser")) {
                // The locator/DB MUST be turned on for this feature to work
                if(FillDBManager.locatorOff()) {
                    Messages.postWarning("The DB(locator) has been turned off "
                                         + "in \'vnmrj adm\'. Aborting!");
                }
                if (protocolBrowser == null)
                    protocolBrowser = new ProtocolBrowser();
                if (protocolBrowser != null) {
                    // protocolBrowser.showPanel();
                    VnmrjIF vjIf = Util.getVjIF();
                    if (vjIf != null) 
                       vjIf.openComp(VnmrjUiNames.protocolbrowsername, "open");
                }
 
                return;
            }
            if(cmd.endsWith("protocolBrowserData")) {
                String filename = tok.nextToken().trim();
                // The locator/DB MUST be turned on for this feature to work
                if(FillDBManager.locatorOff()) {
                    Messages.postWarning("The DB(locator) has been turned off "
                                         + "in \'vnmrj adm\'. Aborting!");
                }
                if (protocolBrowser == null) {
                    Messages.postError("The Protocol Browser Panel has not been "
                                       + "created.  Aborting!");
                }
                String dataType = tok.nextToken().trim();
                VnmrjIF vjIf = Util.getVjIF();
                if (vjIf != null) 
                    vjIf.openComp(VnmrjUiNames.protocolbrowsername, "open");
                
                String fullpath = null;
                String scName = null;
                if(tok.hasMoreTokens())
                    fullpath = tok.nextToken().trim();
                // protocolBrowser.showPanel();
                // Update/Create the Data Tab and do the DB search
                // Determine and pass the SCName if a fullpath was given.
                // the SCName will be the directory where the protocol resides.
                // Thus, take off the protocol name and all other proceeding 
                // directories.  eg., the fullpath should look like
                //    /home/me/vnmrsys/studycardlib/MySC/epi_01.xml
                // We want the "MySC" part of this path.
                if(fullpath != null) {
                    int ind = fullpath.lastIndexOf(File.separator);
                    scName = fullpath.substring(0, ind);
                    ind = scName.lastIndexOf(File.separator);
                    scName= scName.substring(ind +1, scName.length());
                }
                protocolBrowser.updateDataPanel(filename, dataType, scName);

                return;
            }
            if(cmd.endsWith("protocolBrowserSC")) {
                // Create a StudyCard Tab with the contents of this fullpath
                String fullpath = tok.nextToken().trim();
                protocolBrowser.updateStudyCardPanel(fullpath);
                return;
            }
            if(cmd.endsWith("protocolBrowserSCupdate")) {
                // Update the StudyCard Tab if and only if the Tab already
                // contains the contents of this SC.  Else, ignore.
                String filename = tok.nextToken().trim();
                String curSCFullpath = protocolBrowser.getStudyCardTabFullpath();
                // See if curSCFullpath has this file in its path
                if(curSCFullpath.indexOf(filename + ".xml") > -1) 
                    protocolBrowser.updateStudyCardPanel(curSCFullpath);
                return;
            }
            if(cmd.equals("protocolBrowserClose")) {
                // if(protocolBrowser != null)
                //     protocolBrowser.unshow();
                VnmrjIF vjIf = Util.getVjIF();
                if (vjIf != null && protocolBrowser != null) {
                    vjIf.openComp(VnmrjUiNames.protocolbrowsername, "hide");
                }
                return;
            }

            if(cmd.equals("locatoroff")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC locatoroff true/false\')");
                    return;
                }
                ShufDBManager dbManager = ShufDBManager.getdbManager();
                String value = tok.nextToken().trim();
                if(value.startsWith("t"))
                    dbManager.setLocatorOff(true);
                else
                    dbManager.setLocatorOff(false);
                return;
            }
            if(cmd.equals("datelimit")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOC datelimit dayLimit\')");
                    return;
                }
                String value = tok.nextToken().trim();
                int dayLimit = Integer.parseInt(value);
                // ShufDBManager dbManager = ShufDBManager.getdbManager();
                // dbManager.setDateLimitMsFromDays(dayLimit);
                FillDBManager.setDateLimitMsFromDays(dayLimit);
                return;
            }
            if(cmd.endsWith("fpBrowser")) {
                if (tok.hasMoreTokens())
                    cmd = tok.nextToken().trim();
                else
                    cmd = "fpAction";
                if (tok.hasMoreTokens())
                {
                    String dirName = tok.nextToken().trim();
                    sshare.putProperty("fpDir", dirName );
                    if (fpPanel != null) {
                       File dir = new File(dirName);
                       if (dir.exists())
                          fpPanel.setCurrentDirectory(dir);
                    }
                }
                openFpBrowser(cmd);
                return;
            }
            return;
        }
        /* Locator Trash vnmrjcmd's
         * The current available commands are
         *     vnmrjcmd('LOCTRASH autodir active fullpath')
         *     vnmrjcmd('LOCTRASH autodir sqloaded fullpath')
         *
         * These are used to set within the vnmrj environment, the active
         * autodir and the autodir currently in the sq.
         */
        if (cmd.equals("LOCTRASH")) {
            if (!tok.hasMoreTokens()) {
                Messages.postError(
                    "usage: vnmrjcmd(\'LOCTRASH autodir param fullpath\')");
                return;
            }
            String type = tok.nextToken().trim();
            if(type.equals("autodir")) {
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOCTRASH autodir param fullpath\')"
                        + "\n    No args were found after \'autodir\'.");
                    return;
                }
                String param = tok.nextToken().trim();
                if (!tok.hasMoreTokens()) {
                    Messages.postError(
                        "usage: vnmrjcmd(\'LOCTRASH autodir param fullpath\')"
                        + "\n    Only one arg was found after \'autodir\'.");
                    return;
                }
                String mpath = tok.nextToken().trim();
                if(param.equals("active")) {
                    TrashItem.setActiveAutodir(mpath);
                }
                else if(param.equals("sqloaded")) {
                    TrashItem.setSqLoaded(mpath);
                }
                else {
                    Messages.postError(
                         "usage: vnmrjcmd(\'LOCTRASH autodir param fullpath\')"
                         + "\n    param must be active or sqloaded.");
                    return;
                }
            }
            return;
        }
        if (cmd.equals("DEBUG")) {
            if (!tok.hasMoreTokens()) {
                Messages.postError(
                    "usage: vnmrjcmd(\'DEBUG add category\')");
                return;
            }
            cmd = tok.nextToken().trim();
            if (!tok.hasMoreTokens()) {
                Messages.postError(
                        "usage: vnmrjcmd(\'DEBUG add category\')");
                return;
            }
            String category = tok.nextToken().trim();

            if(cmd.equals("add")) {
                    DebugOutput.addCategory(category);
            }
            else if(cmd.equals("remove")) {
                    DebugOutput.removeCategory(category);
            } else if (cmd.equals("precision")) {
                    DebugOutput.setTimePrecision(category);
            }
            return;
        }
        if (cmd.equals("FDA")) {
            JButton button = getFdaButton();
            if (tok.hasMoreTokens()) {
                cmd = tok.nextToken().trim();
                if (cmd.equals("on")) {
                    button.setVisible(true);
                    button.setToolTipText("FDA mode");
                    return;
                }
            }
            button.setVisible(false);
            return;
        }
        if ((cmd.equals("CR") && active) || cmd.equals("CH")) {
	// send CR (cursor) if vp active
	// send CH (crosshair) regardless active
            int win = 0;
            if (!tok.hasMoreTokens())
                return;
            cmd = tok.nextToken().trim();

            try {
                 win = Integer.parseInt(cmd);
            }
            catch (NumberFormatException er) { return; }
            if (!tok.hasMoreTokens())
                return;
            cmd = tok.nextToken("\n").trim();
            if (cmd.length() > 0) {
                expIf.sendToVnmr(win, cmd);
            }
            return;
        }
        if (cmd.equals("VP")) {
            int win = 0;
            if (!tok.hasMoreTokens())
                return;
            cmd = tok.nextToken().trim();
            if (cmd.equals("layout")) {
                if (!active)
                    return;
                if (!tok.hasMoreTokens())
                    return;
                cmd = tok.nextToken().trim();
                if (cmd.equals("off"))
                    appIf.setVpLayout(false);
                else if (cmd.equals("on"))
                    appIf.setVpLayout(true);
                return;
            }
            if (cmd.equals("overlay")) {
                if (!tok.hasMoreTokens())
                     return;
                cmd = tok.nextToken().trim();
                boolean syncMode = false;
                boolean ovlyMode = false;
                if (cmd.equals("on"))
                    ovlyMode = true;
                if (tok.hasMoreTokens()) {
                    cmd = tok.nextToken().trim();
                    if (cmd.equals("sync"))
                        syncMode = true;
                }
                if (!ovlyMode)
                    syncMode = false;
                expIf.overlaySync(syncMode);
                expIf.overlayCanvas(ovlyMode);
	
		if(syncMode && tok.hasMoreTokens())
		((ExpViewArea)expIf).processOverlayCmd(tok.nextToken().trim());
		else if(syncMode)
		((ExpViewArea)expIf).processOverlayCmd("align");
                return;
            }
            if (cmd.equals("overlayMode")) {
	        // this is to set overlay XOR mode.
                if (!tok.hasMoreTokens())
                     return;
                cmd = tok.nextToken().trim();
                expIf.setCanvasOverlayMode(cmd);
                return;
            }
            if (cmd.equals("current")) {
	        // this is to set overlay XOR mode.
                if (!tok.hasMoreTokens())
                     return;
                cmd = tok.nextToken("\n").trim();
                if (cmd.length() > 0) {
                    expIf.sendToVnmr(cmd);
                }
                return;
            }

            try {
                 win = Integer.parseInt(cmd);
            }
            catch (NumberFormatException er) { return; }
            if (!tok.hasMoreTokens())
                return;
            cmd = tok.nextToken("\n").trim();
            if (cmd.length() > 0) {
                expIf.sendToVnmr(win, cmd);
            }
            return;
        }
    }

    // We need a way for other routines to force any browser to be updated.
    // ExpPanel has a pointer to the current browser so I am putting a
    // static here for external use.
    static public void updateBrowser() {
        if(browserPanel != null)
            browserPanel.rescanCurrentDirectory();
    }
    
    static public void openBrowser(String path) {
        if(browserPanel == null)
                new VJBrowserPopup("Browser");
        browserPanel.showBrowser(path);
    }

    // Likewise, since the pointer for the current browser is in here, 
    // add static here for writing the persistence file
    static public void writeBrowserPersistence() {
        if(browserPanel != null)
            browserPanel.writePersistence("Browser");
    }
    
    static public String getStatusValue(String parm) {
        if (statusMessages == null)
           return null;
        return (String)statusMessages.get(parm);
    }

        /** update on pnew */
    public void updateExpListeners(Vector<String> v) {
        // update once-only on start of vnmrj

        synchronized(expListenerList){
            Iterator<ExpListenerIF> itr = expListenerList.iterator();
            while (itr.hasNext()) {
                ExpListenerIF item = itr.next();
                item.updateValue(v);
            }
        }
    }
    /** first time only initialization */
    public void updateExpListeners() {
        synchronized(expListenerList){
            Iterator<ExpListenerIF> itr = expListenerList.iterator();
            while (itr.hasNext()) {
                ExpListenerIF item = itr.next();
                item.updateValue();
            }
        }
    }

    public void updateLimitListeners(String strValue)
    {
        synchronized(m_aListLimitListener)
        {
            Iterator<VLimitListenerIF> iter = m_aListLimitListener.iterator();
            while (iter.hasNext())
            {
                VLimitListenerIF limitIF = iter.next();
                limitIF.updateLimit(strValue);
            }
        }
    }

    public ParamPanelId getParamPanelId()
    {
        if (m_paramPanelId == null)
            m_paramPanelId = new ParamPanelId();
        return m_paramPanelId;
    }

    private void popupPrinterConfig() {
        if (plotConfig == null) {
            plotConfig = new VjPlotterConfig(false, true);
        }
        else {
            if (!plotConfig.isVisible()) {
                plotConfig.rebuild();
            }
        }
        plotConfig.setVisible(true);
        plotConfig.toFront();
    }


    public void processClosePopupCmd(String ds) {
        if(modelessPopups == null) return;

        QuotedStringTokenizer tok = new QuotedStringTokenizer(ds, " \n");
        String par1 = tok.nextToken();  // skip 'closepopup'
        String xmlfile="";
        if (tok.hasMoreTokens()) {
            par1 = tok.nextToken().trim();
            if(par1.startsWith("file:")) {
                String filename = par1.substring(5);
                if (filename.startsWith("/")) {
                   xmlfile = FileUtil.openPath(filename);
                }
                else {
                   xmlfile = FileUtil.openPath("INTERFACE/"+filename);
                }
                if(xmlfile == null) {
                    xmlfile = FileUtil.getLayoutPath("default",filename);
                }
            }
	}
        if (xmlfile != null && modelessPopups.containsKey(xmlfile)) {
            ModelessPopup popup = (ModelessPopup)modelessPopups.get(xmlfile);
            if (popup != null) {
                popup.setVisible(false);
                modelessPopups.remove(xmlfile);
                popup.destroy();
	    }
	}
	return;
    }

    public void processPopupCmd(String ds) {

        String par1;
        String mode = "";
        String xmlfile = "";
        String location = "";
        String strHelpFile = "";
        String strPnew = "";
        String strCancel = "";
        String strClose = "";
        String strOK = "";
        String title = "";
        String filename = "";
        int width = 0;
        int height = 0;
        String strRebuild = "rebuild:";
        boolean bFrameType = false;
        boolean bOnTop = true;

        QuotedStringTokenizer tok = new QuotedStringTokenizer(ds, " \n");
        par1 = tok.nextToken();  // skip 'popup'
        while (tok.hasMoreTokens()) {
            par1 = tok.nextToken().trim();
            if(par1.startsWith("mode:")) {
                mode = par1.substring(5);
                continue;
            }
            if(par1.startsWith("file:")) {
                filename = par1.substring(5);
                if (filename.startsWith("/")) {
                   xmlfile = FileUtil.openPath(filename);
                }
                else {
                   xmlfile = FileUtil.openPath("INTERFACE/"+filename);
                }
                if(xmlfile == null) {
                    xmlfile = FileUtil.getLayoutPath("default",filename);
                }
                continue;
            }
            if(par1.startsWith("location:")) {
                location = par1.substring(9);
                continue;
            }
            if (par1.startsWith(strRebuild)) {
                strRebuild = par1.substring(strRebuild.length());
                continue;
            }
            if(par1.startsWith("width:")) {
                width = (new Integer(par1.substring(6))).intValue();
                continue;
            }
            if(par1.startsWith("height:")) {
                height = (new Integer(par1.substring(7))).intValue();
                continue;
            }
            if (par1.startsWith("help:")) {
                strHelpFile = par1.substring(5);
                continue;
            }
            if (par1.startsWith("pnewupdate:")) {
                strPnew = par1.substring(11);
                continue;
            }
            if (par1.startsWith("ok:")) {
                strOK = par1.substring(3);
                continue;
            }
            if (par1.startsWith("cancel:")) {
                strCancel = par1.substring(7);
                continue;
            }
            if (par1.startsWith("close:")) {
                strClose = par1.substring(6);
                continue;
            }
            if(par1.startsWith("type:")) {
                par1 = par1.substring(5).trim();
                if (par1.equalsIgnoreCase("frame") ||
                              par1.equalsIgnoreCase("window"))
                    bFrameType = true;
                continue;
            }
            if (par1.startsWith("ontop:") || par1.startsWith("onTop:")) {
                par1 = par1.substring(6).trim();
                if (par1.equalsIgnoreCase("no") || par1.equalsIgnoreCase("false"))
                    bOnTop = false;
                continue;
            }
            if(par1.startsWith("title:"))
                title = par1.substring(6);
	    if (par1.indexOf(":") < 0)
                title = title + " " + par1;
        }
	title = Util.getLabelString(title);
        if (filename.equals("Plotters.xml")) {
            popupPrinterConfig();
            return;
        }

        if(mode.length() == 0) mode = "modal";
        boolean bRebuild = strRebuild.equalsIgnoreCase("yes") ? true : false;

        boolean foundJOption = false;
        int     typeJOption = 0;
        String stringJOption = null;
        if (mode.startsWith("message")) {
           typeJOption = JOptionPane.PLAIN_MESSAGE;
           stringJOption = new String("");
           foundJOption = true;
        }
        else if (mode.startsWith("warning")) {
           typeJOption = JOptionPane.WARNING_MESSAGE;
           stringJOption = new String("Warning");
           foundJOption = true;
        }
        else if (mode.startsWith("error")) {
           typeJOption = JOptionPane.ERROR_MESSAGE;
           stringJOption = new String("Error");
           foundJOption = true;
        }
        else if (mode.startsWith("option")) {
           typeJOption = JOptionPane.YES_NO_OPTION;
           stringJOption = new String("Option");
           foundJOption = true;
        }
        if (foundJOption) {
        	if (stringJOption.compareTo("Option") != 0)	{
        		JOptionPane.showMessageDialog(VNMRFrame.getVNMRFrame(),
                    title,stringJOption,typeJOption);
        		return;
        	}
        	else {
        		// int jop = JOptionPane.showConfirmDialog(null,
                     int jop = JOptionPane.showConfirmDialog(VNMRFrame.getVNMRFrame(),
        	            title,"Option",JOptionPane.YES_NO_OPTION);
        	     try {
        		    FileWriter fw = new FileWriter("/tmp/vnmrj_confirm");
        		    fw.write(jop+0x30);
        		    fw.flush();
        		    fw.close();
        	      } catch (IOException ioe) {
        			ioe.printStackTrace();
        		}
        	      return;
        	}
        	
        }

        ModelessPopup popup = null;
        boolean bUpdate = false;

        if (mode.startsWith("modal") && xmlfile != null) {

            if(title.length() == 0) title = "Modal Dialog";

            modalPopup = ModalPopup.getModalPopup(sshare, this, appIf,
				title, xmlfile, width, height,
				bRebuild, strHelpFile,
				strCancel, strOK, strPnew);
            modalPopup.setOnTop(bOnTop);
            modalPopup.showDialogAndSetParms(location);
        } else if (mode.startsWith("modeless") && xmlfile != null) {

            /* stop modal popups from blocking communication with Vnmrbg/Vnmrj */
            if (modalPopup != null) modalPopup.setVisible(false);

            if(title.length() == 0) title = "Modeless Dialog";

            if(modelessPopups == null)
                modelessPopups = new Hashtable<String,ModelessPopup>();

            if(modelessPopups.containsKey(xmlfile) && !bRebuild) {
                popup = (ModelessPopup)modelessPopups.get(xmlfile);
                if (popup != null) {
                   popup.setTitle(title);
                   if(!popup.isVisible()) {
                       // popup.updateAllValue();
                       // bUpdate = true;
                       popup.setVisible(true);
                   }
                   else if(popup.getState() == Frame.ICONIFIED) {
                       // popup.updateAllValue();
                       bUpdate = true;
                       popup.setState(Frame.NORMAL);
                   }
                   else {
                       // bUpdate = true;
                       // popup.setVisible(false);
                       // popup.updateAllValue();
                       // popup.setVisible(true);
                       popup.toFront();
                   }
                }
            } else if(modelessPopups.containsKey(xmlfile)) {
                popup = (ModelessPopup)modelessPopups.get(xmlfile);
                if (popup != null) {
                    modelessPopups.remove(xmlfile);
                    popup.setVisible(false);
                    popup.destroy();
                }
                if (bFrameType)
                   popup = new ModelessPopup(null, sshare, this, appIf, title,
                              xmlfile, width, height, bRebuild, strHelpFile, strClose);
                else
                   popup = new ModelessPopup(sshare, this, appIf, title,
                              xmlfile, width, height, bRebuild, strHelpFile, strClose);
                popup.setOnTop(bOnTop);
                popup.showDialogAndSetParms(location);
                modelessPopups.put(xmlfile, popup);
            } else {
                if (bFrameType)
                   popup = new ModelessPopup(null, sshare, this, appIf, title,
                              xmlfile, width, height, bRebuild, strHelpFile, strClose);
                else
                   popup = new ModelessPopup(sshare, this, appIf, title,
                              xmlfile, width, height, bRebuild, strHelpFile, strClose);
                popup.setOnTop(bOnTop);
                popup.showDialogAndSetParms(location);
                modelessPopups.put(xmlfile, popup);
            }
        }
        if (popup != null) {
            ButtonIF vif = (ButtonIF) Util.getActiveView();
            if (vif != null)
                popup.setVnmrIF(vif);
            if (bUpdate) {
                popup.updateAllValue();
            }
        }
    }

    private void processVpPopupCmd(String ds) {

        String par1;
        String mode = "";
        String xmlfile = "";
        String location = "";
        String strHelpFile = "";
        String strClose = "";
        String title = "";
        int width = 0;
        int height = 0;
        String strRebuild = "rebuild:";

        QuotedStringTokenizer tok = new QuotedStringTokenizer(ds, " ,\n");
        par1 = tok.nextToken();  // skip 'popup'
        while (tok.hasMoreTokens()) {
            par1 = tok.nextToken().trim();
            if(par1.startsWith("mode:")) mode = par1.substring(5);
            if(par1.startsWith("file:"))
                xmlfile = FileUtil.openPath("INTERFACE/"+par1.substring(5));
            if(par1.startsWith("location:")) location = par1.substring(9);
            if (par1.startsWith(strRebuild))
                strRebuild = par1.substring(strRebuild.length());
            if(par1.startsWith("width:")) width = (new Integer(par1.substring(6))).intValue();
            if(par1.startsWith("height:"))  height = (new Integer(par1.substring(7))).intValue();
            if (par1.startsWith("help:"))
                strHelpFile = par1.substring(5);
            if (par1.startsWith("close:"))
                strClose = par1.substring(6);
            if(par1.startsWith("title:"))
                title = par1.substring(6);
	    if (par1.indexOf(":") < 0) title = title + " " + par1;
        }
	title = Util.getLabelString(title);

        if(mode.length() == 0) mode = "modal";
        boolean bRebuild = strRebuild.equalsIgnoreCase("yes") ? true : false;
        m_strVpxmlfile = xmlfile;

        if (mode.startsWith("modal") && xmlfile.endsWith(".xml")) {

            if(title.length() == 0) title = "Modal Dialog";

            VPDialog vpPopup = VPDialog.getModalVpPopup(sshare, this, appIf, title,
                           xmlfile, width, height, bRebuild, strHelpFile, strClose);
            vpPopup.showDialogAndSetParms(location);

        }
        else if (mode.startsWith("modeless") && xmlfile.endsWith(".xml"))
        {
            if(title.length() == 0) title = "Modeless Dialog";

            if(modelessPopups == null)
                modelessPopups = new Hashtable<String,ModelessPopup>();

            if(modelessPopups.containsKey(xmlfile)) {
                ModelessPopup popup = (ModelessPopup)modelessPopups.get(xmlfile);
                ButtonIF vif = (ButtonIF) Util.getActiveView();
                if (vif != null)
                     popup.setVnmrIF(vif);
                if(popup != null && !popup.isVisible()) {
                    popup.updateAllValue();
                    popup.setVisible(true);
                }
                else if(popup != null && popup.getState() == Frame.ICONIFIED) {
                    popup.updateAllValue();
                    popup.setState(Frame.NORMAL);
                }
                else if(popup != null) {
                        popup.setVisible(false);
                        popup.updateAllValue();
                        popup.setVisible(true);
                }
            } else {
                VPDialog popup = new VPDialog(sshare, this, appIf, title, xmlfile,
                                 width, height, bRebuild, strHelpFile, strClose);
                popup.showDialogAndSetParms(location);
                modelessPopups.put(xmlfile, popup);
            }
        }

    }

    public Hashtable<String,ModelessPopup> getModelessPopups() {
        return modelessPopups;
    }

    public ModelessPopup getModelessPopup(String xmlfile) {

    //xmlfile is full path

        Object obj = null;

        if(modelessPopups != null) {
            obj = modelessPopups.get(xmlfile);
        }

        if(obj != null) return (ModelessPopup)obj;
        return null;
    }

    /**
     * Prints the images to the printer or the file.
     * @param cmd  the command for print
     */
    protected void doPrintcmd()
    {
        String strPar = "";               // the parameter
        String strSend = "";              // send it to printer or file
        String strFrames = "";            // the frames to be printed

        if (printTimer != null)
            printTimer.stop();
        if (plotCmd == null)
            return;
        plotFile = null;
        QuotedStringTokenizer strTok = new QuotedStringTokenizer(plotCmd, " ,\n");
        while (strTok.hasMoreTokens())
        {
            strPar = strTok.nextToken();
            if (strPar.startsWith("layout:")) {
                plotOrient = strPar.substring(7);
                continue;
            }
            if (strPar.startsWith("file:")) {
                plotFile = strPar.substring(5);
                continue;
            }
            if (strPar.startsWith("region:")) {
                plotRegion = strPar.substring(7);
                continue;
            }
            if (strPar.startsWith("format:")) {
                if (strPar.length() > 7)
                   plotFormat = strPar.substring(7);
                continue;
            }
            if (strPar.startsWith("pformat:")) {
                if (strPar.length() > 8)
                   plotFormat = strPar.substring(8);
                continue;
            }
            if (strPar.startsWith("send:")) {
                strSend = strPar.substring(5);
                continue;
            }
            if (strPar.startsWith("size:")) {
                plotSize = strPar.substring(5);
                continue;
            }
            if (strPar.startsWith("plotter:")) {
                plotter = strPar.substring(8);
                continue;
            }
            if (strPar.startsWith("dpi:")) {
                // plotDpi = strPar.substring(4);
                continue;
            }
            if (strPar.startsWith("frames:"))
            {
                strFrames = strPar.substring(7);
                StringBuffer sbData = new StringBuffer().append(strFrames);
                while (strTok.hasMoreTokens())
                {
                    sbData.append(" ").append(strTok.nextToken());
                }
                strFrames = sbData.toString();
            }
        }

        bPlot = (strSend.equalsIgnoreCase("print")) ? true : false;

        // Get the size and the location for the image to be captured
        Dimension dimCanvas;
        Point pointCanvas;
        boolean bImgUser = Util.isImagingUser();
        if (plotFile != null && (plotFile.indexOf(File.separator) < 0))
            plotFile = new StringBuffer().append("plot").append(File.separator).append(plotFile).toString();

        if (bImgUser && plotRegion.indexOf("frames") >= 0 && strFrames.length() > 0)
        {
            processPrintframes(strFrames, plotFile, plotFormat, plotOrient, plotSize,
                               plotter);
        }
        else if (plotRegion.equalsIgnoreCase("vnmrj"))
        {
            dimCanvas = appIf.getSize();
            pointCanvas = appIf.getLocationOnScreen();
            printWindow(plotFile, dimCanvas, pointCanvas);
        }
        if (bReopenTool) {
            VnmrjIF vjIf = Util.getVjIF();
            vjIf.openComp("GraphicsTool", "open");
        }
    } 

    public void printVnmrj() {

        VnmrjIF vjIf = Util.getVjIF();
        Component graphicsToolbar = (Component) Util.getGraphicsToolBar();
            
            // Also unshow the graphics toolbar if showing, but bring it back 
            // after capture if needed.
        bReopenTool = false;
        if(graphicsToolbar != null && graphicsToolbar.isVisible()) {
            int dock =  ((GraphicsToolIF) graphicsToolbar).getDockPosition();
            if (dock < 1 || dock > 2) { // not on left or right side
                 bReopenTool = true;
                 vjIf.openComp("GraphicsTool", "close");
            }
        }
        
        JFrame vframe = (JFrame) VNMRFrame.getVNMRFrame();
        if (vframe != null)
            vframe.toFront();

        if (printTimer == null) {
             ActionListener printAction = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                     doPrintcmd();
                }
             };
             printTimer = new Timer(400, printAction);
             printTimer.setRepeats(false);
        }
        printTimer.start();
    }

    protected void raiseUpWindow() {
        // We need to get the modless popup out of the way.  Get the
        // popup from the modelessPopups hashtable using the path of the
        // xml file as the key.  
        String xmlfile = FileUtil.openPath("INTERFACE/Print.xml");
        ModelessPopup popup = null;
        if (modelessPopups != null)
            popup = modelessPopups.get(xmlfile);
        if(popup != null) {
            // Make the Print Screen panel go away.  Else it shows up in the
            // screen capture.  If people decide they want it to stay up,
            // then bring it back up after the capture.
            popup.setVisible(false);
        }

        printVnmrj();

    }
        
    protected void printWindow(String strFile, Dimension dim, Point point)
    {
        ((ExpViewArea)expIf).printWindow(winId, strFile, dim, point);
    }

    protected void printWindow(String strFile, String strFormat, String strLayout,
         String strSize, String strPlotter, Dimension dim, Point point)
    {
         printWindow(strFile, dim, point);
/**
        {
            // write to the file
            if (!bPlot)
            {
                strFile = convert(strFile, strWriteFile, strFormat);
                if (strFile != null)
                    Messages.postInfo("Saving file: " + strFile);
            }
            // send to the printer
            else
            {
                if (strPlotter == null || strPlotter.equals("") ||
                    strPlotter.equalsIgnoreCase("none"))
                    Messages.postError("Plotter not found. Please specify plotter " +
                                      "in the 'Printers' dialog.");
                else
                    strWriteFile = convert(strWriteFile, strSize, strLayout, strPlotter, dim);
                // if (m_objPrint == null)
                //     m_objPrint = new VPrint();
                // m_objPrint.print(strWriteFile, strLayout);
            }
            WUtil.deleteFile(strWriteFile);
        }
****/
    }

    /**
     * Prints each selected frame
     * @param strFrames   the number of frames selected, and their dimensions
     *                    x0 y0 width height e.g. 2 4 4 10 10 6 6 10 10
     * @param strFile     the name of the file
     * @param strFormat   the format of the image
     * @param strLayout   the layout of the image to be printed
     * @param strSize     the size of the image to be printed
     * @param bPrint      true if send to printer
     */
    protected void processPrintframes(String strFrames, String strFile, String strFormat,
                      String strLayout, String strSize, String strPlotter)
    {
        QuotedStringTokenizer strTokenizer = new QuotedStringTokenizer(strFrames);
        int nFrames = 0;
        if (strTokenizer.hasMoreTokens())
            nFrames = Integer.parseInt(strTokenizer.nextToken());
        if (nFrames <= 0) {
            Messages.postError("No frames selected.");
            return;
        }
        int i = 0;
        int j = 1;
        // the size is x0, y0, width, height
        int[] nArrSize = {0, 0, 0, 0};
        Dimension dim = new Dimension();
        Point point = new Point();
        Point pointVp = vcanvas.getLocationOnScreen();
        int nIndex = strFile.indexOf(".");
        String strValue;
        String strWritefile = strFile;
        // strframes is of the form: numFrames x0 y0 width height ....
        while (strTokenizer.hasMoreTokens())
        {
            strValue = strTokenizer.nextToken();
            if (i%4 == 0)
                nArrSize[0] = Integer.parseInt(strValue);
            else if (i%4 == 1)
                nArrSize[1] = Integer.parseInt(strValue);
            else if (i%4 == 2)
                nArrSize[2] = Integer.parseInt(strValue);
            else if (i%4 == 3)
            {
                nArrSize[3] = Integer.parseInt(strValue);
                dim.setSize(nArrSize[2], nArrSize[3]);
                point.setLocation(pointVp.x+nArrSize[0], pointVp.y+nArrSize[1]);
                // if images are written to a file, then append number to each file
                if (nFrames > 1 && !bPlot)
                {
                    if (nIndex >= 0)
                        strWritefile = new StringBuffer().append(strFile.substring(0, nIndex)).
                             append(j).append(strFile.substring(nIndex)).toString();
                    else
                        strWritefile = strFile + j;
                    j++;
                }
                printWindow(strWritefile, strFormat, strLayout, strSize, strPlotter,
                                dim, point);
            }
            i++;
        }
    }

    /**
     * Saves the image
     * @param strWriteFile the file to be written
     * @param strSize      the size the image to be saved
     * @return the file written
     */
    protected String convert(String strWriteFile, String strSize, String strLayout,
                             String strPlotter, Dimension dim)
    {
        String strFile = new StringBuffer().append("USER/PERSISTENCE/imageFile").
                             append(System.currentTimeMillis()).append(".ps").toString();
        strFile = FileUtil.savePath(strFile);
        int nSize = 525;
        if (strSize.indexOf("half") >= 0)
            nSize = 375;
        else if (strSize.indexOf("quarter") >= 0)
            nSize = 250;

        int nLayout = 0;
        if (strLayout.equalsIgnoreCase("landscape"))
            nLayout = 90;

        // The specified nSize is for the width of a piece of letter paper.
        // If the length to be printed is greater than 1.4 * width, then it
        // will go off the page.  We need to catch that can resize to fit
        // Everything here assumes that letter paper is 612x792 dots and
        // A4 is 595x842 and are to fit the smaller in each dimension
        if(nLayout == 90) {
            if(dim.width > dim.height * 1.4) 
                nSize = (int)(nSize * (dim.height * 1.4) / dim.width);      
        }
        else {
            if(dim.height > dim.width * 1.4)
                nSize = (int)(nSize * (dim.width * 1.4) / dim.height);
        }
        
        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.SYS_VNMR + "/bin/convertgeom " +
                         nSize + " " + nLayout + " " + strPlotter + " \"" +
                         strWriteFile + "\" \"" + strFile + "\""};
        WUtil.runScript(cmd, true);
        return strFile;
    }

    /**
     * Saves the image to the different format
     * @param strFile       the name of the file
     * @param strWriteFile  the name of the file to be written
     * @param strFormat     the format the images should be saved
     */
    protected String convert(String strFile, String strWriteFile, String strFormat)
    {
        strFormat = strFormat.toLowerCase();
        int nIndex = strFile.lastIndexOf('.');
        strFile = FileUtil.savePath(strFile);

        if (strFile == null)
            return strFile;

        String strExt = strFormat;

        // file name is of the form 'file.'
        if (nIndex >= 0 && nIndex+1 >= strFile.length())
            strFile = new StringBuffer().append(strFile).append(strExt).toString();
        else if (strExt != null && !strExt.equals(""))
        {
            strExt = new StringBuffer().append(".").append(strExt).toString();
            int nLength = strFile.length();
            nIndex = nLength-4;
            if (nIndex <= 0 || !strExt.equalsIgnoreCase(strFile.substring(nIndex)))
                strFile = new StringBuffer().append(strFile).append(strExt).toString();
        }

        if (strFormat.equalsIgnoreCase("jpg") || strFormat.equalsIgnoreCase("jpeg"))
        {
            File objfile = new File(strWriteFile);
            objfile.renameTo(new File(strFile));
            objfile = new File(strFile);
            if (objfile.exists())
                return strFile;
        }
        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, 
        		FileUtil.SYS_VNMR + "/bin/convert \"" + strWriteFile + "\" " + strFormat + ":\"" + strFile + "\""};
        WUtil.runScript(cmd);
        return strFile;
    }

    protected void processPrintcmd(String cmd) {
        if (((ExpViewArea)expIf).processPrintcmd(cmd)) {
            // ExpViewArea will take care of this job
            return;
        }
        plotCmd = cmd;
        raiseUpWindow();

        // doPrintcmd(cmd);
    } 

    public void execPrintCmd(String cmd) {
        plotCmd = cmd;
        raiseUpWindow();
    } 

    protected void saveVJmolImage(final boolean bPrint, final boolean bShow)
    {
        m_strVJmolImagePath = FileUtil.openPath("MOLLIB/icons");
        if (m_strVJmolImagePath == null)
            m_strVJmolImagePath = FileUtil.savePath("MOLLIB/icons");

        if (vjmol == null || m_strVJmolImagePath == null)
            return;

        // save the image
        final String strPath = ((ExpViewArea)expIf).saveImage(winId, m_strVJmolImagePath,
                                                              bPrint, false, bShow);
        if (strPath == null || strPath.equals(""))
            return;


        new Thread(new Runnable()
        {
            public void run()
            {
                // String strColor = m_strMolColor;
                String strColor = "black";
                if (bPrint)
                    strColor = "white";
           
                // remove whitespace
                String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.sysdir() + "/bin/convert -trim " +
                                    " -transparent " + strColor + " \"" +
                                    strPath + "\" gif:\"" + strPath + "\""};
                WUtil.runScript(cmd);
                // save the image as transparent gif
                //((ExpViewArea)expIf).saveImage(strPath, bPrint, true, bShow);

                // display the image
                if (!bPrint)
                {
                    String strIcon = "";
                    VIcon icon = vcanvas.getIcon();
                    String strPath2 = strPath;
                    if (Util.iswindows())
                        strPath2 = UtilB.windowsPathToUnix(strPath);
                    if (icon != null && icon.isSelected() &&
                        strPath.equals(icon.getFile()))
                    {
                        strIcon = icon.getIconId();

                       //  sendToVnmr("imagefile('delete', '"+strIcon+"')");
                       if (icon.isMol())
                           sendToVnmr("imagefile('load', '"+strIcon+"','"+strPath2+"', 'mol')");
                       else
                           sendToVnmr("imagefile('load', '"+strIcon+"','"+strPath2+"')");
                        
                    }
                    else if (bShow)
                        sendToVnmr("imagefile('display', '"+strPath2+"', 'mol')");
                }

            }
        }).start();
    }

    protected void openMol(String cmd)
    {
        QuotedStringTokenizer strTok = new QuotedStringTokenizer(cmd);
        String strMode = "";
        String strUserdir = "";
        String strPath = "";

        while (strTok.hasMoreTokens())
        {
            String strValue = strTok.nextToken();
            if (strValue.startsWith("mode:"))
                strMode = strValue.substring(5);
            else if (strValue.startsWith("userdir:"))
                strUserdir = strValue.substring(8);
            else if (strValue.startsWith("file:"))
                strPath = strValue.substring(5);
        }

        strPath = FileUtil.openPath(strPath);
        if (strPath == null)
        {
            if (strMode != null && !strMode.equals(""))
                Messages.postError(strMode + " not found.");
            return;
        }

        strUserdir = FileUtil.savePath(strUserdir);
        if (strUserdir == null)
            strUserdir = FileUtil.usrdir();

        if (!Util.iswindows())
        {
            String[] cmd2 = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.SYS_VNMR + 
                                "/jre/bin/java -Duser.dir=" + strUserdir +  " -jar " + strPath};
            WUtil.runScriptInThread(cmd2);
        }
        else
        {
            String cmd2 = FileUtil.sysdir() + "/jre/bin/java.exe -Duser.dir=\"" + 
                          strUserdir +  "\" -jar \"" + strPath + "\"";
            WUtil.runScriptInThread(cmd2);
        }
        if (strMode != null && !strMode.equals(""))
            Messages.postInfo("Opening " + strMode + "...");
    }

    public void setVp(String vp)
    {
        int newVp = 0;
        try {
            newVp = Integer.parseInt(vp);
        }
        catch (NumberFormatException er) {
            return;
        }
        if (newVp > 0) {
            int prevVp = appIf.getVpId() + 1;
            if (appIf.setViewPort(newVp - 1)) {
               setVpLayout(newVp, prevVp);
            }
        }
    }

    /**
     * Inform VnmrBGs of a change in active viewport.
     * @param newVp The new viewport number (from 1).
     * @param prevVp The old viewport number (from 1).
     */
    private void setVpLayout(int newVp, int prevVp) {
        sendToVnmr("M@vplayout"+newVp);
        if (prevVp != newVp) {
            expIf.sendToAllVnmr("activeviewport=" + newVp);
	    expIf.sendToVnmr(newVp, "activeviewport(" + newVp + ","+prevVp+")");
        }
    }

    protected void setDefaultVp()
    {
        if (modelessPopups == null)
            return;
        Object objDialog = modelessPopups.get(m_strVpxmlfile);
        if (objDialog instanceof VPDialog)
        {
            ((VPDialog)objDialog).setDefaultVp();
        }
    }

    private void processCDump(String cmd)
    {
        String strFile = "";
        String strSuffix = "";
        String strCmd = "";

        if (vcanvas == null)
            return;
        QuotedStringTokenizer tok = new QuotedStringTokenizer(cmd, " ,\n");
        if (tok.hasMoreTokens())
        {
            strFile = tok.nextToken().trim();
        }
        if (tok.hasMoreTokens())
        {
            strSuffix = tok.nextToken().trim();
        }
        if (tok.hasMoreTokens())
        {
            strCmd = tok.nextToken("\n").trim();
        }

        if (strFile == null || strFile.trim().length() <= 0)
            return;

        // Get the size and the location for the image to be captured
        // Dimension dimCanvas = vcanvas.getWinSize();
        // Point pointCanvas = vcanvas.getLocationOnScreen();

        // capture the image
        // boolean bOk = CaptureImage.doCapture(strFile, dimCanvas, pointCanvas);

        BufferedImage img = vcanvas.getCanvasImage();
        if (img == null)
             return;

        boolean bOk = CaptureImage.write(strFile, img);

        // Convert the file to the given suffix format
        if (bOk && strSuffix != null && strSuffix.length() > 0)
        {
            if (!strSuffix.equals(CaptureImage.getOutputFormat()))
               bOk = CaptureImage.doConvert(strFile, CaptureImage.JPEG, strSuffix);
            if (bOk) {
               if (!strFile.equals(CaptureImage.getOutputFileName())) {
                  // delete the original file after converting
                  File objFile = new File(strFile);
                  objFile.delete();
               }
            }
        }

        if (bOk && strCmd != null && strCmd.length() > 0)
        {
            String[] shCmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, strCmd};
            WUtil.runScriptInThread(shCmd);
        }
    }

    private void processGraphicsCmd(int type, QuotedStringTokenizer tok) {
        String cmd = tok.nextToken().trim();
        if (vcanvas == null)
            return;
        if (cmd.equals("cdump") || cmd.equals("save")) {
             if (!tok.hasMoreTokens())
                return;
             if (type == 0) {
                cmd = tok.nextToken("\n");
                processCDump(cmd);
             }
             return;
        }
        if (cmd.equals("show")) {
             if (appIf == null)
                 return;
             if (tok.hasMoreTokens())
                 cmd = tok.nextToken().trim();
             else
                 cmd = "";
             if (type == 0) {
                 vcanvas.showImage(cmd);
             }
             if (type == 1) {
                 if (imgPane == null) {
                     imgPane = new ImageViewPanel();
                     add(imgPane, 0);
                     validate();
                 }
                 imgPane.setVisible(true);
                 imgPane.showImage(cmd);
             }
             if (type == 2) {
                 appIf.showImagePanel(true, cmd);
             }
             return;
        }
        if (cmd.equals("close")) {
             if (appIf == null)
                 return;
             if (type == 1) {
                 imgPane.setVisible(false);
             }
             if (type == 2) {
                 appIf.showImagePanel(false, null);
             }
             return;
        }
    }

    // process inter-java class "vc" commands (vnmrbg not used)
    private void processJavaCmd(String ds) {
        QuotedStringTokenizer tok = new QuotedStringTokenizer(ds, " \')(,\n");
        String par1 = tok.nextToken(); // skip calling function
        par1 = tok.nextToken().trim();
        String strHelpFile = "";

        Messages.postDebug("java", "processJavaCmd: " + ds);
        // DisplayOptions.java commands
        // syntax: vc="java(display options [help file])" open dialog
        // vc="java(display style StdParStyle=$VALUE)" set style
        // 
        if(par1.equals("display")) {
            par1 = tok.nextToken().trim();
            if(par1.equals("options")) {
                if(tok.hasMoreTokens())
                    strHelpFile = getHelpFile(tok.nextToken().trim());
                Util.showDisplayOptions(strHelpFile);
            } else if(par1.equals("style")) {
                String data = tok.nextToken(")").trim();
                data=data.replaceAll("\'","");
                data=data.replaceFirst(",","");
                DisplayOptions.setOption(data);
            } else if(par1.equals("style2")) {
                String data = tok.nextToken(")").trim();
                data=data.replaceAll("\'","");
                data=data.replaceFirst(",","");
                DisplayOptions.setOption("graphics"+data);
                DisplayOptions.setOption("ps"+data);
            }
            return;
        } else if(par1.equals("console")) {
            par1 = tok.nextToken().trim();
            // TODO: pass commands to console via remote socket class
        }
        else if(glcom != null){
            String str=ds.substring(5);
            str=str.replace('\'', ' ');
            str=str.replace(',', ' ');
            str=str.replace(')', '\n');
            if(glcom.comCmd(str)){
               // if(gldialog != null)
                //    gldialog.updateTools(str);
                return;
            }
        }
    }
    private void processUtilCmd(String ds) {
        if (DebugOutput.isSetFor("utilcmd")) {
            Messages.postDebug("processUtilCmd string: " + ds);
        }
        QuotedStringTokenizer tok = new QuotedStringTokenizer(ds, " ,\n");
        String par1 = tok.nextToken();  // skip 'util'
        par1 = tok.nextToken().trim();
        String strHelpFile = "";
        if (par1.equals("savedata")) {
            if (tok.hasMoreTokens())
                strHelpFile = getHelpFile(tok.nextToken().trim());
            return;
        }
        if (par1.equals("savestudydata")) {
            if (tok.hasMoreTokens())
                strHelpFile = getHelpFile(tok.nextToken().trim());
            return;
        }
        if(par1.equals("tooltabs")){
            Util.getToolPanelUtil();           
            return;
        }
        if(par1.equals("display")){
            par1 = tok.nextToken().trim();
            if(par1.equals("options"))
            {
                String tab=null;
                while (tok.hasMoreTokens()){
                    par1=tok.nextToken(";\n").trim();
                    if(par1.startsWith("help:", 0))
                        strHelpFile = getHelpFile(par1);
                    else if(par1.startsWith("tab:", 0)){
                        tab=par1.substring(4);
                    }
                }
                Util.showDisplayOptions(strHelpFile);
                if(tab!=null)
                    Util.getDisplayOptions().showTab(tab);
            }
            return;
        }
        if(par1.equals("printers")){
            par1 = tok.nextToken().trim();
            String title = "";
            while (tok.hasMoreTokens()){
                String par = tok.nextToken().trim();
                if (par.startsWith("help:"))
                    strHelpFile = getHelpFile(par);
                else
                    title += " " + par;
            }
            Plotters dialog = Plotters.getPlotters(sshare,this,appIf,par1,title,
                                                   strHelpFile);
            dialog.setVisible(true);
            dialog.transferFocus();
            return;
        }
        if(par1.equals("importPanel")){
            ImportDialog importDialog;
            if (tok.hasMoreTokens())
                strHelpFile = getHelpFile(tok.nextToken());
            importDialog = ImportDialog.getImportDialog(null, strHelpFile);
            importDialog.showDialogAndGetPaths();
            return;
        }
        if(par1.equals("updatetable")){
            if(tok.hasMoreTokens()){
                String table = tok.nextToken().trim();
                // ShufDBManager dbManager = ShufDBManager.getdbManager();
                // Do the updat in a thread so vnmrj can continue.
                UpdateDatabase updateThread = new UpdateDatabase(table, true);
                updateThread.setPriority(Thread.MIN_PRIORITY);
                updateThread.setName("Updatetable");
                updateThread.start();
            }
            return;
        }
        if(par1.equals("updateLocator")){
            // Cause another shuffle.
            StatementHistory history = sshare.statementHistory();
            history.updateWithoutNewHistory();
            return;
        }
        if(par1.equals("saveCustomLocStatement")){
            if (tok.hasMoreTokens())
                strHelpFile = getHelpFile(tok.nextToken());
            SaveCustomLocatorStatement.showPopup(strHelpFile);
            return;
        }
        if(par1.equals("deleteCustomLocStatement")){
            CustomStatementRemoval csr;
            csr = CustomStatementRemoval.getCustomStatementRemoval();
            if (tok.hasMoreTokens())
                strHelpFile = getHelpFile(tok.nextToken());
            csr.showDialog(strHelpFile);
            return;
        }
        if (par1.equals("logout"))
        {
            if (m_loginbox == null)
                m_loginbox = new LoginBox();
            String titleargs = "";
            if (tok.hasMoreTokens()) {
                titleargs = tok.nextToken("");
            }
            m_loginbox.setVisible(true, titleargs);
            return;
        }
        if (par1.equals("loginpassword"))
        {
            if (tok.hasMoreTokens())
                strHelpFile = getHelpFile(tok.nextToken());
            VLoginPassword.showDialog(strHelpFile);
            return;
        }
        if(par1.equals("edit_profile")) {
            if(profilePanel == null) {
                profilePanel = new UserRightsPanel("operator");
            }
            profilePanel.setVisible(true);
            return;
        }
        if(par1.equals("dicomConfig")) {
            if(dicomConfigPanel == null) {
                dicomConfigPanel = new DicomConf(null);
            }
            dicomConfigPanel.initLayout();
            dicomConfigPanel.setVisible(true);
            return;
        }
        if(par1.equals("expSelTabEditor")) {
            // In case the appdirs have been edited, we need to just recreate
            // this so it track the current appdirs.
            expSelTabEditor  = new ExpSelTabEditor();
            expSelTabEditor.setVisible(true);
            return;
        }
        if(par1.equals("expSelTreeEditor")) {
            // In case the appdirs have been edited, we need to just recreate
            // this so it track the current appdirs.
            expSelTreeEditor  = new ExpSelTreeEditor();
            expSelTreeEditor.setVisible(true);
            return;
        }
        // Add protocol to user level ExperimentSelector_user.xml file
        if(par1.equals("updateexpsel")) {
            // Redo tokenizer and include a colon for a delimineter
            tok = new QuotedStringTokenizer(ds, ",:\n", false);
            // skip the first tokens
            if (tok.hasMoreTokens())
                 tok.nextToken();  // util
            if (tok.hasMoreTokens())
                 tok.nextToken();         // updateexpsel
            
            String protocol=null;
            String label=null;
            String tab=null;
            String menu1=null;
            String menu2=null;
            String expSelNoShow="false";
            String opOnly="false";
 
            if (tok.hasMoreTokens())
                protocol = tok.nextToken().trim();
            if (tok.hasMoreTokens())
                label = tok.nextToken().trim();
            if (tok.hasMoreTokens())
                tab = tok.nextToken().trim();
            if (tok.hasMoreTokens())
                menu1 = tok.nextToken().trim();
            if (tok.hasMoreTokens())
                menu2 = tok.nextToken().trim();
            if (tok.hasMoreTokens()) {
                expSelNoShow = tok.nextToken().trim();
                if(expSelNoShow.length() == 0)
                    expSelNoShow = "false";
                // Put this one inside the previous hasMoreTokens because
                // calling hasMoreTokens twice after the end causes an Exception
                if (tok.hasMoreTokens())
                    opOnly = tok.nextToken().trim();
                if(opOnly.equals("opOnly"))
                    opOnly = "true";
            }
            if (DebugOutput.isSetFor("expselector")) {
                Messages.postDebug("updateexpsel:" + protocol + ":" + label + ":" 
                        + tab + ":" + menu1 + ":" + menu2 + ":" +  expSelNoShow + ":" +  opOnly);
            }
            // Add a line to the users definition file and update the Exp Sel
            ExpSelector.updateExpSelFile(protocol, label, tab, menu1, menu2, expSelNoShow, opOnly);
            return;
        }
        // Remove protocol from user level ExperimentSelector_user.xml file
        if(par1.equals("removeexpsel")) {
            tok = new QuotedStringTokenizer(ds);
            // skip the first tokens
            if (tok.hasMoreTokens())
                 tok.nextToken();  // util
            if (tok.hasMoreTokens())
                 tok.nextToken();     // removeexpsel
            
            String protocol = null;

            if (tok.hasMoreTokens())
                protocol = tok.nextToken().trim();
            if (DebugOutput.isSetFor("expselector")) {
                Messages.postDebug("removeexpsel:" + protocol);
            }

            // Remove all entries for this protocol from the users definition file 
            // and update the Exp Sel
            ExpSelector.removeProtocolFromExpSelFile(protocol);
            return;
        }
        if (par1.equals("bgReady")) {
            waitLogin(false);
            return;
        }
        if (par1.equals("setexpsel")) {
            if (tok.hasMoreTokens())
                par1 = tok.nextToken().trim();
            if (!tok.hasMoreTokens())
                return;
            String src = tok.nextToken().trim();
            ExpSelector.buildProtocolFile(par1, src);
            return;
        }
    }

    private void processEditCmd(String ds) {
        QuotedStringTokenizer tok = new QuotedStringTokenizer(ds, " ,\n");
        String par1 = tok.nextToken();  // skip 'edit'
        par1 = tok.nextToken().trim();
        String strHelpFile = "";
        if (par1.equals("template")) {
            if (tok.hasMoreTokens())
                strHelpFile = getHelpFile(tok.nextToken());
            controlPanel = Util.getControlPanel();
            ParamInfo.setInfoPanel(strHelpFile);
            ParamInfo.setEditMode(true);
            return;
        }
        else if(par1.equals("protocols")){
            if (tok.hasMoreTokens())
                strHelpFile = getHelpFile(tok.nextToken());
            Util.showProtocolEditor(strHelpFile);
            return;
        }
        else if(par1.equals("acceleratorKeys")){
            if (tok.hasMoreTokens())
                strHelpFile = getHelpFile(tok.nextToken());
            AcceleratorKeyEditor.showAcceleratorKeyEditor(strHelpFile);
            return;
        }
        if (par1.equals("annotation")) {
            if (annotation == null) {
                if (tok.hasMoreTokens())
                    strHelpFile = getHelpFile(tok.nextToken());
                annotation = new AnnotationEditor();
            }
            if (annotation != null) {
                annotation.setVisible(true);
                annotation.setState(Frame.NORMAL);
            }
            return;
        }
        if (par1.equals("tray")) {
            boolean on = true;
            if (bSmsDebug)
               System.out.println("tray edit Cmd: "+ds);
            while (tok.hasMoreTokens()) {
               par1 = tok.nextToken().trim();
               if (par1.equalsIgnoreCase("off"))
                   on = false;
               else if (par1.equalsIgnoreCase("on"))
                   on = true;
               else if (par1.equals("-d") || par1.equals("-debug"))
                   bSmsDebug = false;
               else if (par1.equals("debug"))
                   bSmsDebug = true;
            }
            appIf.showSmsPanel(on, bSmsDebug);
            SmsPanel sPan = ((ExpViewArea)expIf).getSmsPanel();
            if (sPan != null)
                sPan.setP11svfdir(p11Icon, p11Tip);
            return;
        }

    }

    /**
     * Handle tray commands from VnmrJ.
     * <br>Usage: <code>vnmrjcmd('tray', args)</code>
     * @param ds The "args" to the tray command, possibly with spaces.
     */
    public void processTrayCmd(String ds) {
        VStrTokenizer tok = new VStrTokenizer(ds, " ,\n");
        boolean on = false;
        boolean smsVisible = false;
        // boolean update = false;
        // boolean delta = false;
        String d1 = null;
        if (bSmsDebug)
           System.out.println("tray vjCmd: "+ds);
        SmsPanel sPan = ((ExpViewArea)expIf).getSmsPanel();
        if (sPan != null) {
           smsVisible = sPan.isVisible();
           on = smsVisible;
        }
        while (tok.hasMoreTokens()) {
               d1 = tok.nextToken().trim().toLowerCase();
               if (d1.equalsIgnoreCase("repaint")) {
                   repaintTitleBar();
                   return;
               }
               if (d1.equalsIgnoreCase(SmsCmds.UPDATE)) {  // update
                   if (sPan != null)
                       sPan.execCmd(d1, null);
                   return;
               }
               if (d1.equalsIgnoreCase(SmsCmds.CLEAR)) { // clearselect
                   if (sPan != null)
                       sPan.execCmd(d1, null);
                   return;
               }
               if (d1.equalsIgnoreCase(SmsCmds.DELTA) ||
                          d1.equalsIgnoreCase(SmsCmds.REMOVE)) { // delta, remove
                   if (sPan != null)
                       sPan.execCmd(d1, ds.substring(tok.getPosition()).trim());
                   return;
               }
               if (d1.equalsIgnoreCase(SmsCmds.COLOR_EDITOR)) {  // coloreditor
                   if (tok.hasMoreTokens())
                      d1 = tok.nextToken().trim();
                   else
                      d1 = "open";
                   SmsPanel.showColorEditor(this, d1);
                   return;
               }
               if (d1.equalsIgnoreCase(SmsCmds.TOOLTIP)) { // tooltip
                   if (tok.hasMoreTokens())
                      d1 = tok.nextToken().trim();
                   else
                      d1 = "open";
                   SmsPanel.showTrayTooltip(d1);
                   return;
               }
               if (d1.equalsIgnoreCase(SmsCmds.SELECT)) {
                   if (sPan != null)
                       sPan.execCmd(d1, ds.substring(tok.getPosition()).trim());
                   return;
               }
               if (d1.equalsIgnoreCase(SmsCmds.SELECTABLE)) {
                   SmsPanel.setSelectable(ds.substring(tok.getPosition()).trim());
                   return;
               }

               if (d1.equalsIgnoreCase(SmsCmds.HIDE_CMD)) { // sethidetraycmd
                   m_hideTrayCmd = null;
                   if (tok.hasMoreTokens()) {
                       m_hideTrayCmd = tok.nextToken("").trim();
                   }
                   SmsPanel.setHideTrayCmd(m_hideTrayCmd);
                   return;
               }
               if (d1.equalsIgnoreCase(SmsCmds.SHOW_CMD)) { // setshowtraycmd
                   m_showTrayCmd = null;
                   if (tok.hasMoreTokens()) {
                       m_showTrayCmd = tok.nextToken("").trim();
                   }
                   return;
               }

               if (d1.equalsIgnoreCase(SmsCmds.SHOW_BUTTON)) { // showbutton
                   if (m_showTrayCmd != null) {
                       sendToVnmr(m_showTrayCmd);
                       return;
                   }
                   on = true;
               }
               else if (d1.equalsIgnoreCase(SmsCmds.OFF)) // off
                   on = false;
               else if (d1.equalsIgnoreCase(SmsCmds.SHOW))  // show
                   on = true;
               else if (d1.equalsIgnoreCase(SmsCmds.CLOSE))  // close
                   on = false;
               else if (d1.equals("-d") || d1.equals("-debug")) {
                   bSmsDebug = false;
                   if (sPan != null)
                       sPan.setDebugMode(bSmsDebug); 
               }
               else if (d1.equals("debug")) {
                   bSmsDebug = true;
                   if (sPan != null)
                       sPan.setDebugMode(bSmsDebug); 
               }
        }
/*
        if (update) {
            if (sPan != null && sPan.isVisible())
               sPan.setVisible(true);
            return;
        }
        if (delta) {
            if (sPan != null && sPan.isVisible())
                sPan.deltaSmaple(d1);
            return;
        }
*/
        if (on != smsVisible)
            appIf.showSmsPanel(on, bSmsDebug);
        if (sPan != null)
            sPan.setP11svfdir(p11Icon, p11Tip);
    }

    protected String getHelpFile(String strFile)
    {
        if (strFile.startsWith("help:"))
            return strFile.substring(5);
        else
            return "";
    }

    public void  processStatusData(String  str) {
        /* Put this message in hash table */
        Messages.postDebug("StatusMsgs","processStatusData: " + str);
        StringTokenizer tok = new StringTokenizer(str);
        if ( ! tok.hasMoreTokens()) {
            return;
        }
        String parm = tok.nextToken();
        String v = (String) statusMessages.get(parm);
        if (str.equals(v)) {
            return;
        }
        statusMessages.put(parm, str);

        /* Send message to all listeners */
        synchronized(statusListenerList) {
            Iterator<StatusListenerIF> itr = statusListenerList.iterator();
            while (itr.hasNext()) {
                StatusListenerIF pan = itr.next();
                pan.updateStatus(str);
            }
        }
    }

    /** Add a file to the Shuffler.
     *
     *  The first arg is objType, and the second is filename.
     *  If filename is a fullpath, use it.
     *  If filename is only a name or is a relative path, try to determine
     *  the default directory where it might be.
     *  Use the current user as the owner.
     */
    private void addToShuffler(String objType, ArrayList<String> fnames) {
        // Without forcing the loc update, new protocols don't show up
        // in the locator.  If we always update the loc, it makes things
        // too slow for some repeative operations.
        if(objType.equals(Shuf.DB_PROTOCOL))
            addToShuffler(objType, fnames, null, null, true, false);
        else
            addToShuffler(objType, fnames, null, null, false, false);
 
    }
    /** Add a file to the Shuffler.
     *
     *  The first arg is objType, and the second is filename.
     *  If filename is a fullpath, use it.
     *  If filename is only a name or is a relative path, try to determine
     *  the default directory where it might be.
     *  Use the current user as the owner.
     *  The 3rd arg is an attribute name
     *  The 4th arg is the attr value.
     *  Set this attr to this value for the file being added
     */
    private void addToShuffler(String objType, ArrayList<String> fnames, String attr,
                               String value, boolean locDisplayUpdate,
                               boolean trayUpdate) {
        AddToLocatorViaThread addThread;
        String fpath;
        String owner;
        String fname;
        ArrayList<String> fpaths = new ArrayList<String>();
        ArrayList<String> filenames = new ArrayList<String>();

        // Get the current user name
        owner = System.getProperty("user.name");

        ShufDBManager dbManager = ShufDBManager.getdbManager();

        for(int i=0; i < fnames.size(); i++) {
            fname = (String) fnames.get(i);
            fpath = dbManager.getDefaultPath(objType, fname);

            if(fpath == null) {
                Messages.postError("addToShuffler: " + fname + " Not Found.");
                return;
            }

            if (fname.lastIndexOf(File.separator) != -1) {
                // arg contains a relative path, get the fname off the end.
                fname = fname.substring(fname.lastIndexOf(File.separator) +1);
            }
            fpaths.add(fpath);
            filenames.add(fname);
        }


        addThread = new AddToLocatorViaThread(objType, filenames, fpaths,
                                          owner, true, attr, value,
                                          locDisplayUpdate, trayUpdate, expIf);
        addThread.setPriority(Thread.MIN_PRIORITY);
        addThread.setName("AddToLocatorViaThread");
        addThread.start();

    }



    /** Add a file to the Shuffler.
     *
     *    The arg is a single string with 4 separated
     *    strings, (user, objType, filename, fullpath).
     */
    private void addToShuffler(String str) {
        String owner;
        String objType;
        String filename;
        String fullPath;

        StringReader sr = new  StringReader(str);
        // Use StreamTokenizer so that the quoted filename and fullpath
        // are taken as an intact token.
        StreamTokenizer tok = new StreamTokenizer(sr);
        // Allow underscore characters in the name, it defaults to a delimiter
        tok.wordChars('_', '_');
        // Allow slashes in fullpath
        tok.wordChars('/', '/');

        try {
            tok.nextToken();
            owner = new String(tok.sval);
            tok.nextToken();
            objType = new String(tok.sval);
            tok.nextToken();
            filename = new String(tok.sval);
            tok.nextToken();
            fullPath = new String(tok.sval);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return;
        }
        addToShuffler(objType, filename, fullPath, owner, true);

    }


    /** Add a file to the Shuffler.
     *
     */
    public void addToShuffler(String objType, String filename,
                              String fullPath, String owner,
                              boolean highlight) {

        AddToLocatorViaThread addThread;
        if (Util.iswindows())
            filename = UtilB.unixPathToWindows(filename);
        addThread = new AddToLocatorViaThread(objType, filename, fullPath,
                                          owner, highlight);
        addThread.setPriority(Thread.MIN_PRIORITY);
        addThread.setName("AddToLocatorViaThread");
        addThread.start();
    }

    /************************************************** <pre>
     * Remove this file from the shuffler.  fname can be either a
     *  full path or a filename.  If it is a filename, the user's default
     *  directory for this type of file will be used.
     *
     *
     </pre> **************************************************/

    private void removeFromShuffler(String objType, String fname) {
        String fpath;
        String host;

        // Get the current user name
        // String owner = System.getProperty("user.name");

        // Get the fullpath for this fname.
        ShufDBManager dbManager = ShufDBManager.getdbManager();
        fpath = dbManager.getDefaultPath(objType, fname);

        if(fpath == null) {
            Messages.postError("addToShuffler: " + fname + " Not Found.");
            return;
        }
        // Get the local host name
        try {
            InetAddress inetAddress = InetAddress.getLocalHost();
            host = inetAddress.getHostName();
        }
        catch(Exception e) {
            Messages.postError("Error getting HostName");
            host = new String("");
        }

        removeFromShuffler(objType, fpath, host, true);

    }

    private void removeFromShuffler(String str) {
    	if(FillDBManager.locatorOff())
    		return;
        QuotedStringTokenizer tok = new QuotedStringTokenizer(str, " ,\n");
        if (!tok.hasMoreTokens())
            return;
        String host = tok.nextToken().trim();
        if (!tok.hasMoreTokens())
            return;
        String type = tok.nextToken().trim();
        if (!tok.hasMoreTokens())
            return;
        String file = tok.nextToken().trim();
        ShufDBManager dbManager = ShufDBManager.getdbManager();
        dbManager.removeEntryFromDB(type, file, host);

        // Cause another shuffle so that this item disappears.
        SessionShare session = ResultTable.getSshare();
        session.statementHistory().updateWithoutNewHistory();

        // Also remove from the Holding area if it is there.
        HoldingDataModel holdingDataModel = sshare.holdingDataModel();
        holdingDataModel.removeFileEntry(file, host);
    }


    public void removeFromShuffler(String objType, String fullpath,
                                    String host, boolean reshuffle) {

        ShufDBManager dbManager = ShufDBManager.getdbManager();
        dbManager.removeEntryFromDB(objType, fullpath, host);

        if(reshuffle) {
            // Cause another shuffle so that this item disappears.
            SessionShare session = ResultTable.getSshare();
            session.statementHistory().updateWithoutNewHistory();
        }

        // Also remove from the Holding area if it is there.
        HoldingDataModel holdingDataModel = sshare.holdingDataModel();
        holdingDataModel.removeFileEntry(fullpath, host);
    }

    public void  flushVnmr() {
        String str = "jFunc("+VFLUSH+")\n";
        sendToVnmr(str);
        if (outPort == null)
            expIf.initCanvasArray();
    }

    public void  refresh() {
        if (vcanvas != null)
            vcanvas.refresh();
    }

    public void  processAlphaData(String  str) {
        appIf.appendMessage(str+"\n");
    }


    public int  getVnmrPort() {
        return vnmrHostPort;
    }

    public String  getVnmrHost() {
        return vnmrHostName;
    }

    private void  createSqTimer() {
         ActionListener sqAction = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                     childProcessExit();
                }
         };
         sqTimer = new Timer(1000, sqAction);
         sqTimer.setRepeats(false);
    }

    public boolean isAlive() {
         return bAlive;
    }

    public void  childProcessExit() {
            vnmrProcess = null;
            outPort = null;
            if (sqBusy) {
                if (sqTimer == null)
                    createSqTimer();
                sqCount++;
                if (sqCount < 5) {
                    sqTimer.restart();
                    return;
                }
            }
            if (!bExiting) {
                bAlive = false;
                expIf.canvasExit(winId);
                // quit();
                if (vcanvas != null)
                   vcanvas.setVisible(false);
                setBackground(Color.gray);
            }
/*
            VNMRFrame vframe = VNMRFrame.getVNMRFrame();
            if (vframe != null)
                vframe.exitAll();
*/
    }

    public void  statusProcessExit() {
            statusProcess = null;
    }

    public void  statusProcessError() {
    }

    public void  graphSocketError(Exception e) {
/*
        String str = "jFunc(2)\n";
        sendToVnmr(str);
*/
        if (bExiting)
            return;
        if (vnmrThread == null || !vnmrThread.isAlive())
            return;
/*
    Messages.postError("Vnmr socket was broken ");
        Messages.writeStackTrace(e, "Error caught in Graphics socket.");
*/
    }

    public void  comSocketError(Exception e) {
        if (bExiting)
            return;
        if (vnmrThread == null || !vnmrThread.isAlive())
            return;
        String str = "jFunc(2)\n";
        sendToVnmr(str);
/*
    Messages.postError("Vnmr socket was broken ");
        Messages.writeStackTrace(e, "Error caught in Master socket.");
*/
    }

    public void  alphaSocketError(Exception e) {
        if (bExiting)
            return;
        if (vnmrThread == null || !vnmrThread.isAlive())
            return;
        String str = "jFunc(2)\n";
        sendToVnmr(str);
/*
    Messages.postError("Vnmr socket was broken ");
        Messages.writeStackTrace(e, "Error caught in Aplha socket.");
*/
    }

    public void  graphSocketReady() {
        vnmrReady = true;
    }


    public int getViewId() {
        return winId;
    }

    private void vnmrBusyAction() {
        if (vbgBusy || panelBusy)
            setBusy(true);
    }

    private void popupVnmrbgError() {
        vnmrbgTimer.stop();
        vnmrbgTimer = null;
        if (outPort != null)
            return;
        String message = "";
        String filePath = FileUtil.openPath("SYSTEM"+File.separator+"msg"+
                    File.separator+"vnmrjStartupError.txt");
        if (filePath != null) {
            String str;
            try
            {
                 BufferedReader in = new BufferedReader(new FileReader(filePath));
                 while((str = in.readLine()) != null) {
                     message = message +"\n" + str;
                 }
                 in.close();
            }
            catch (Exception e)
            { }
        }

        if (message.length() < 10)
            message = "Unable to start Vnmrbg program. \n Please Check that the desktop hostname is present in the /etc/hosts file.";

        JOptionPane.showMessageDialog(null,
                message,
                "Vnmr Error",
                JOptionPane.ERROR_MESSAGE);

        VNMRFrame vframe = VNMRFrame.getVNMRFrame();
        if (vframe != null)
           vframe.exitAll();
        else
           System.exit(0);
    }

    private void createBusyTimer() {
        ActionListener vnmrBusy = new ActionListener() {
             public void actionPerformed(ActionEvent ae) {
                  vnmrBusyAction();
             }
        };
        busyTimer = new Timer(300, vnmrBusy);
        busyTimer.setRepeats(false);
    }

    private void startVnmrbgTimer() {
        if (winId != 0 || vnmrbgTimer != null)
           return;
        ActionListener vnmrbgAction = new ActionListener() {
             public void actionPerformed(ActionEvent ae) {
                  popupVnmrbgError();
             }
        };
        vnmrbgTimer = new Timer(12000, vnmrbgAction);
        vnmrbgTimer.start();
    }

    private void initVnmrTimer() {
        if (vnmrInitTimer != null)
           return;
        ActionListener vnmrAction = new ActionListener() {
             public void actionPerformed(ActionEvent ae) {
                  timerCount++;
                  mayRunVnmr();
             }
        };
        timerCount = 0;
        vnmrInitTimer = new Timer(1000, vnmrAction);
        vnmrInitTimer.start();
    }

    private synchronized void mayRunVnmr() {
        if (Util.isAdminIf()) {  // don't run Vnmrbg
            if (vnmrInitTimer != null)
               vnmrInitTimer.stop();
            return;
        }
        if (vnmrRunning) {
            if (vnmrInitTimer != null) {
               vnmrInitTimer.stop();
               vnmrInitTimer = null;
            }
            return;
        }
        if (!readyToRun || !panelReady) {
            if (vnmrInitTimer == null) {
               initVnmrTimer(); 
               return;
            }
            int k = winId + 1;
            if (k > 5) k = 5;
            if (timerCount < k)
               return;
        }
        vnmrRunning = true;
        if (vnmrInitTimer != null) {
           vnmrInitTimer.stop();
           vnmrInitTimer = null;
        }
        startVnmrbgTimer();
        String xinfo = "-jxid 0";
        if (bNative) {
            int xid = Util.getWindowId(); // X window id
            String s = vcanvas.getCanvasSize();
            if (s == null)
                s = "0x0+0+0";
            String g = vcanvas.getCanvasRegion();
            if (g == null)
                g = "0x0+0+0";
            xinfo = "-jxid "+xid+" -jxgeom "+s+" -jxregion "+g;
        }
        vnmrProcess = new VnmrProcess(this, localHostName, socketPort, winId+1,
            xinfo);
        vnmrThread = new Thread(vnmrProcess);
        vnmrThread.setName("Run Vnmr");
        vnmrThread.start();
    }

    public void run() {
        String test = System.getProperty("test");

        if ((test != null) && (test.equals("true"))) {
           return;
        }
        /*******
        if (Util.isMultiSq()) {
           if (qPanel == null)
              qPanel = SqContainer.createStudyQueue(this);
        }
        ********/
        readyToRun = true;
        mayRunVnmr();
        if (winId == 0 && statusProcess == null) {
            StringBuffer tempSb;
            String fstr;
            tempSb = new StringBuffer().append(System.getProperty("sysdir"));
            tempSb.append(File.separator).append("bin").append(File.separator);
            if (Util.iswindows()) {
                tempSb.append("winInfostat.exe");
            } else {
                tempSb.append("Infostat");
            }
            fstr = tempSb.toString();
            File f = new File(fstr);
            if (!f.exists()) {
                  return;
            }
            String host = System.getProperty("console");
            if (host == null)
                host = localHostName;
            if (host.equals("none") || host.equals("null"))
                return;
            stSocket = new StatusSocket(this, winId);
            stSocketThread = new Thread(stSocket);
            statusPort = stSocket.getPort();
            stSocketThread.setName("SocketThread");
            stSocketThread.start();

           // int trys=0;
           // int maxtrys=20;
           // while (outPort == null && trys < maxtrys)
           // {
             // try { Thread.sleep(200); trys++;} catch(InterruptedException ie) { };
           // }
           
           // if (trys > 0 ) System.out.println("waited "+(trys*0.2)+" secs"); 
           // System.out.println("waited "+(trys*0.2)+" secs"); 
           

            statusProcess = new StatusProcess(this, host, statusPort);
            statusThread = new Thread(statusProcess);
            statusThread.setName("Run Infostat");
            statusThread.start();
        }
        if (active) {
            ActionHandler vpAH = Util.getVpActionHandler();
            if (vpAH != null) {
                vpAH.fireAction(new ActionEvent(this, 0, String.valueOf(winId+1)));
            }
        }

    }

    public void setFullWidth(boolean b) {
        isFullWidth = b;
        if (trayButton == null) {
            trayButton = ((ExpViewArea)expIf).getTrayButton();
        }
        if (helpBar != null)
            helpBar.setVisible(b);
    }

    public void sendToActiveVnmr(String str) {
        if (expIf != null)
            expIf.sendToVnmr(str);
    }

    public void sendToVnmr(String str) {
        if (bSuspend) {
           // System.out.println("Suspended not sent: '" + str + "'" );
            return;
        }
        if (outPort != null) {
            queryCount++;
            outPort.outPut(str);
        }
        else   // if Vnmrbg commlink not ready then queue the commands
        {
           m_queueSendToVnmrCmds.add(str);
           // System.out.println("outPort not ready, not sent: '" + str + "'" );
        }
    }

    public void closeOutput() {
        // bSuspend = true;
    }

    public void openOutput() {
        bSuspend = false;
        if (active) {
            if (ExpSelector.needUpdate())
                ExpSelector.update();
        }
    }

    public void sendCmdToVnmr(String str) {
        if (sendWinId)
                return;
        sendToVnmr(str);
    }

        /** patch up attribute expression for vnmr */
    private String attrString(VObjIF obj,int attr){
        String d = obj.getAttribute(attr);
        if (d == null)
            return "";
        int first = 0;
        int last = d.length() - 1;

        if (last > 0) {
            if ((d.charAt(0) == '\'') && (d.charAt(last) == '\'')) {
                first = 1;
                last--;
            }
        }
        if (last < first)
            return "";
        if (charCapacity < (last + 12)) {
            charCapacity = last + 32;
            charValue = new char[charCapacity + 4];
        }
        int index = 0;
        int sq = 0;
        char c;
        for (int i = first; i <= last; i++) {
            c = d.charAt(i);
            if (c == '\\') {
                sq++;
                charValue[index++] = c;
            }
            else {
                if (c == '\n') {
                    charValue[index++] = '\\';
                    charValue[index++] = 'n';
                }
                else {
                    if (c == '\'') {
                       sq = sq % 2;
                       if (sq == 0)
                          charValue[index++] = '\\';
                    }
                    charValue[index++] = c;
                }
                sq = 0;
            }
            if (index >= charCapacity) {
                charCapacity = charCapacity + 12;
                char newValue[] = new char[charCapacity + 4];
                System.arraycopy(charValue, 0, newValue, 0, index);
                charValue = newValue;
            }
        }
        return new String(charValue, 0, index);
    }


    /** generic string token substitution utility */
//     public String subString(String s, String m, String d){
//         String r="";
//         StringTokenizer tok= new StringTokenizer(s," \t\':+-*/\"/()[]=,\0",true);
//         while(tok.hasMoreTokens()){
//             String t=tok.nextToken();
//             if(t.equals(m))
//                 r+=d;
//             else
//                 r+=t;
//         }
//         return r;
//     }

    public String subString(String s, String m, String d) {
        StringBuffer b = new StringBuffer();
        int offset = 0;
        int start = 0;
        int len = s.length();
        if (len < 1)
             return s;
        offset = s.indexOf(m);
        while (offset >= 0) {
             if (offset > start) {
                b.append(s.substring(start, offset));
             }
             b.append(d);
             start = offset + m.length();
             offset = s.indexOf(m, start);
        }
        if (start < len)
             b.append(s.substring(start));

        return b.toString();
    }

    public void sendVnmrCmd(VObjIF obj, String cmd)
    {
        sendVnmrCmd(obj, cmd, VALUE);
    }

    public void sendVnmrCmd(VObjIF obj, String cmd, int nKey) {
        int len = cmd.length();
        if(len <= 0)
            return;
        if (obj == null) {
            if (bDebugCmd)
                logWindow.append(VnmrJLogWindow.COMMAND, winId, obj, cmd);
            sendToVnmr(cmd);
            return;
        }
        if(obj instanceof VSubMenuItem) {
            // VjToolBar vt = ((ExperimentIF) appIf).getToolBar();
            VjToolBar vt = Util.getToolBar();
            if(vt != null) {
                if(AddToolsDialog.getToolEditStatus()) {
                    if(vt.getDnDNewToolStatus()) {
                        String vc = obj.getAttribute(CMD);
                        String label = obj.getAttribute(LABEL);
                        vt.setNewToolInfo(label, vc);
                    } else
                        vt.processVnmrCmd("toolbarEditor");
                    return;
                }
            }
        }

        if(cmd.indexOf("$VALUES") >= 0)
            cmd = subString(cmd, "$VALUES", attrString(obj, VALUES));
        if(cmd.indexOf("$VALUE") >= 0)
            cmd = subString(cmd, "$VALUE", attrString(obj, nKey));

        // for inter-java class communication using "vc" VObjIF attribute
        // xml syntax: vc="java(keyword option args ..)"
        // delimiters: " \',()\t\n"
        // e.g. java(display style StdParFont=$VALUE)
        // or java('display style','StdParFont=$VALUE') etc.
        // 

        if(cmd.startsWith("java")) {
            StringTokenizer tok = new StringTokenizer(cmd, " (");
            String par1 = tok.nextToken();
            if(par1.equals("java")) {
		//vnmrbg cmd may follow java cmd
                tok = new StringTokenizer(cmd, ")");
                par1 = tok.nextToken();
                processJavaCmd(par1+")");
/*
		if(tok.hasMoreTokens() && outPort != null) {
		   sendToVnmr(tok.nextToken().trim());
*/
		if(tok.hasMoreTokens()) {
    		   sendVnmrCmd(obj, tok.nextToken().trim(), nKey);
		}
                return;
            }
        }

        if (bDebugCmd) {
            logWindow.append(VnmrJLogWindow.COMMAND, winId, obj, cmd);
        }

        //if(outPort != null)
        //	System.out.println("lost:"+cmd);

        sendToVnmr(cmd);
    }

    public void setCanvasCursor(String c) {
        vcanvas.processCommand("cursor "+c);
    }

    private void setBusyTimer() {
         if (vbgBusy || panelBusy) {
             if (!busyTimer.isRunning())
                 busyTimer.restart();
             // busyTimer.start();
         }
         else {
             busyTimer.stop();
             setBusy(false);
         }
    }

    public void setBusyTimer(boolean bz) { // from VnmrCanvas
         vbgBusy = bz;
         setBusyTimer();
    }

    public void setPanelBusy(boolean b) {
         panelBusy = b;
         setBusyTimer();
    }

    public void sendBusy() {
        // panelBusy = true;
	// setBusy(true);
	// sendToVnmr("vnmrjcmd('busy')\n");
    }

    public void sendFree() {
        // panelBusy = false;
	// setBusy(false);
        // sendToVnmr("vnmrjcmd('free')\n");
    }

    public boolean isBusy()
    {
        return panelBusy;
    }

    public ParamIF syncQueryParam(String param) {
        if (!vnmrReady || outPort == null)
            return null;
        syncParam = param;
        waitParam = true;
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(PVAL).
                            append(", '$VALUE=").append(param).append("')\n");
        //String cmd = "jFunc("+PVAL+", '$VALUE="+param+"')\n";
        sendToVnmr(sbcmd.toString());
        while (waitParam) { }
        return sParamIf;
    }

    public ParamIF syncQueryDGroup(String param) {
        if (!vnmrReady || outPort == null)
            return null;
        syncParam = param;
        waitParam = true;
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(PGRP).
                                  append(", '").append(param).append("')\n");
        //String cmd = "jFunc("+PGRP+", '"+param+"')\n";
        sendToVnmr(sbcmd.toString());
        while (waitParam) { }
        return sParamIf;
    }

    public ParamIF syncQueryPStatus(String param) {
        if (!vnmrReady || outPort == null)
            return null;
        syncParam = param;
        waitParam = true;
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(PSTATUS).
                                  append(", '").append(param).append("')\n");
        //String cmd = "jFunc("+PSTATUS+", '"+param+"')\n";
        sendToVnmr(sbcmd.toString());
        while (waitParam) { }
        return sParamIf;
    }

    public ParamIF syncQueryMinMax(String param) {
        if (!vnmrReady || outPort == null)
            return null;
        syncParam = param;
        waitParam = true;
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(PMINMAX).
                                  append(", 0, '").append(param).append("')\n");
        //String cmd = "jFunc("+PMINMAX+", 0, '"+param+"')\n";
        sendToVnmr(sbcmd.toString());
        while (waitParam) { }
        return sParamIf;
    }

    // SyncQuerys are unsynchronized, should only be called from main thread
    public ParamIF syncQueryVnmr(int type, String param) {
        if (!vnmrReady || outPort == null)
            return null;
        syncParam = param;
        syncWaitParam = true;
        StringBuffer sb = new StringBuffer(param);
        // Insert "\" in front of any "'"
        // Change line feeds to "\n"
        for (int i=0; i<sb.length(); i++) {
            if (sb.charAt(i) == '\'') {
                if (i == 0 || sb.charAt(i-1) != '\\')
                    sb.insert(i++, '\\');
            }
            if (sb.charAt(i) == '\n') {
                sb.deleteCharAt(i);
                sb.insert(i, "\\n");
                i++;
            }
        }

        if (sb.length() <= 0) {
            syncWaitParam = false;
            return null;
        }
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(type).
                                  append(", '").append(sb).append("')\n");
        //String cmd = "jFunc("+type+", '"+sb+"')\n";
        sendToVnmr(sbcmd.toString());
        while (syncWaitParam) { }
        return sParamIf;
    }

    public ParamIF syncQueryExpr(String str) {
        if (!vnmrReady || outPort == null)
            return null;
        syncParam = str;
        waitParam = true;
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(PEXPR).
                                  append(",").append(winId).append(", '").
                                  append(str).append("')\n");
        //String cmd = "jFunc("+PEXPR+","+winId+", '"+str+"')\n";
        sendToVnmr(sbcmd.toString());
        while (waitParam) { }
        return sParamIf;
    }

    public void asyncQueryVnmr(VObjIF obj, int type, String param, String unit){
        asyncQueryVnmrNamed("null",obj,type,param,unit);
    }

    public void asyncQueryVnmr(VObjIF obj, int type, String param) {
        asyncQueryVnmrNamed("null",obj,type,param,null);
    }

    public void asyncQueryVnmrNamed(String name, VObjIF obj, int type, String param, String unit) {
        if (!vnmrReady || outPort == null || param == null)
            return ;
        if (bSuspend)
            return ;

        StringBuffer sb = new StringBuffer(param.trim());
        // Insert "\" in front of any "'"
        // Change line feeds to "\n"
        for (int i = 1; i < sb.length(); i++) {
            if (sb.charAt(i) == '\'') {
                if (sb.charAt(i-1) != '\\')
                   sb.insert(i++, '\\');
            }
            if (sb.charAt(i) == '\n') {
                sb.deleteCharAt(i);
                sb.insert(i, "\\n");
                i++;
            }
        }
        if (sb.length() <= 0) {
            return;
        }

        queryQsize = queryQ.size();
        int   num = 0;
        while (num < queryQsize) {
            if (queryQ.elementAt(num) == null)
                break;
            num++;
        }
        QueryData qd =new QueryData(obj,name);
        if (num >= queryQsize)
            queryQ.add(qd);
        else
            queryQ.setElementAt(qd, num);

        num++;
        //String cmd;
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(type).
                                  append(", ").append(num).append(", '").append(sb);
        if (unit != null) {
           sbcmd.append("', '").append(unit).append("')");
           //cmd = "jFunc("+type+", "+num+", '"+sb+"', '"+unit+"')";
        }
        else
        {
            sbcmd.append("')");
            //cmd = "jFunc(" + type + ", " + num + ", '" + sb + "')";
        }
        if (bDebugQuery) {
            logWindow.append(VnmrJLogWindow.QUERY, winId, obj, sb.toString());
        }
        sendToVnmr(sbcmd.toString());
    }

    public void asyncQueryVnmrArray(VObjIF obj, int type, String param) {
        asyncQueryVnmrArrayNamed("null",obj,type,param);
    }

    public void asyncQueryVnmrArrayNamed(String name, VObjIF obj, int type, String param) {
        if (!vnmrReady || outPort == null)
            return ;
        if (bSuspend)
            return ;
        queryQsize = queryQ.size();
        int   num = 0;
        while (num < queryQsize) {
            if (queryQ.elementAt(num) == null)
                break;
            num++;
        }

        QueryData qd =new QueryData(obj,name);
        if (num >= queryQsize)
            queryQ.add(qd);
        else
            queryQ.setElementAt(qd, num);

        num++;
        //String cmd;
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(type).
                                  append(", ").append(num);
        if (param != null) {
           sbcmd.append(", '").append(param).append("')\n");
           //cmd = "jFunc("+type+", "+num+", '"+param+"')\n";
        }
        else
        {
            sbcmd.append(")\n");
            //cmd = "jFunc(" + type + ", " + num + ")\n";
        }
        if (bDebugQuery && param != null) {
            logWindow.append(VnmrJLogWindow.QUERY, winId, obj, param);
        }
        sendToVnmr(sbcmd.toString());
    }

    public void asyncQueryARRAY(VObjIF obj, int cmdType, String param) {
        asyncQueryVnmrArray(obj, cmdType, param);
    }

    public void asyncQueryParamNamed(String name, VObjIF obj, String param) {
        asyncQueryVnmrNamed(name, obj, PVAL, param, null);
    }
    public void asyncQueryParam(VObjIF obj, String param) {
        asyncQueryVnmr(obj, PVAL, param, null);
    }

    public void asyncQueryParamNamed(String name, VObjIF obj, String param, String unit) {
        asyncQueryVnmrNamed(name, obj, PVAL, param, unit);
    }
    public void asyncQueryParam(VObjIF obj, String param, String unit) {
        asyncQueryVnmr(obj, PVAL, param, unit);
    }

    public void asyncQueryShowNamed(String name, VObjIF obj, String param) {
        asyncQueryVnmrNamed(name, obj, SHOWCOND, param, null);
    }
    public void asyncQueryShow(VObjIF obj, String param) {
        asyncQueryVnmr(obj, SHOWCOND, param, null);
    }

    public void asyncQueryMinMax(VObjIF obj, String param) {
        asyncQueryVnmr(obj, PMINMAX, param, null);
    }

    private void processSyncQuery(String ds) {
        String  type, d1, d2;
        int     m;
        StringTokenizer tok = new StringTokenizer(ds, " ,\n");
        type = null;
        sParamIf = null;
        if (tok.hasMoreTokens()) {
            type = tok.nextToken();
        }
        if (type == null || !tok.hasMoreTokens()) {
            waitParam = false;
            return;
        }
        if (type.equals("r")) { // real number
            sParamIf = new ParamIF(syncParam, "real", tok.nextToken("\n").trim());
        }
        else if (type.equals("s")) { // string
            sParamIf = new ParamIF(syncParam, "string", tok.nextToken("\n").trim());
        }
        else if (type.equals("t")) { // real number array
            sarraySize = 0;
            try {
                 sarraySize = Integer.parseInt(tok.nextToken());
            }
            catch (NumberFormatException er) { return; }
            if (sarraySize <= 0)
                return;
            sarrayVal = new String[sarraySize];
            for (m = 0; m < sarraySize; m++) {
                if (tok.hasMoreTokens())
                   sarrayVal[m] = tok.nextToken().trim();
                else {
                   waitParam = false;
                   return;
                }
            }
            sParamIf = new ParamIF(syncParam, "realArray", sarrayVal);
        }
        else if (type.equals("u")) { // string array
            m = -1;
            sarraySize = -1;
            try {
                 sarraySize = Integer.parseInt(tok.nextToken());
                 if (tok.hasMoreTokens())
                        m = Integer.parseInt(tok.nextToken());
            }
            catch (NumberFormatException err) { return; }
            if (sarraySize <= 0 || m < 0 || !tok.hasMoreTokens()) {
                // something wrong
                waitParam = false;
                return;
            }
            if (m == 0 || sarrayVal == null)
                sarrayVal = new String[sarraySize];
            if (m < sarraySize)
                sarrayVal[m] = tok.nextToken("\n").trim();
            else
                waitParam = false;
            if (m == sarraySize -1 ) {
                waitParam = false;
                sParamIf = new ParamIF(syncParam, "stringArray", sarrayVal);
            }
            return;
        }
        else if (type.equals("m")) { // min, max
            d1 = tok.nextToken().trim();
            if (tok.hasMoreTokens()) {
                d2 = tok.nextToken("\n").trim();
                sParamIf = new ParamIF(syncParam, "real", d1, d2);
            }
        }
        else if (type.equals("d")) { // dgroup
            sParamIf = new ParamIF(syncParam, "dGroup", tok.nextToken("\n").trim());
        }
        else if (type.equals("a")) { // active status
            sParamIf = new ParamIF(syncParam, "pStatus", tok.nextToken("\n").trim());
        }
        else { // another real
            sParamIf = new ParamIF(syncParam, "real", tok.nextToken("\n").trim());
        }
        waitParam = false;
    }

    private void processARRAYQuery(String ds) {
        processEvalQuery(ds);
    }

    private void processAvalQuery(String ds) {
        processEvalQuery(ds);
    }

    private void processFileEvalQuery(String path) {
        if (path == null) {
            return;
        }
        path = path.trim();
    if (path.length() <= 0)
            return;
        BufferedReader in;
        try {
            in = new BufferedReader(new FileReader(path));
        } catch(FileNotFoundException e1) {
            Messages.postError("Cannot open file \""+path+"\"");
            Messages.writeStackTrace(e1);
            return;
        }
        String line;
        try {
            while ((line = in.readLine()) != null) {
        if (line.length() > 1)
                    processEvalQuery(line);
            }
        } catch(IOException e2) {}
        try {
            in.close();
            (new File(path)).delete();
        } catch(Exception e3) {}
    }

    private void centerOnScreen(Component c) {
    	int x;
    	int y;
    	Dimension screenSize;		// make this an instance variable?

        screenSize = Toolkit.getDefaultToolkit().getScreenSize();

    	x = (screenSize.width - c.getWidth()) / 2;
    	y = (screenSize.height - c.getHeight()) / 2;

    	c.setLocation(x, y);
    }

    private void processEvalQuery(String ds) {
        int     id;
        String   d;
        if (debug) {
            String ms = Long.toString(new Date().getTime());
            int msc = ms.length() - 3;
            Messages.postDebug(ms.substring(0,msc)+"."+ms.substring(msc)
                               +": "+ds);
        }
        StringTokenizer tok = new StringTokenizer(ds, " ,\n");
        id = -1;
        if (tok.hasMoreTokens()) {
            try {
                id = Integer.parseInt(tok.nextToken());
            }
            catch (NumberFormatException er) { return; }
        }
        d = " ";
        if (tok.hasMoreTokens()) {
            d = tok.nextToken("\n").trim();
            if (d.startsWith("E@@"))
                d = null;
        }
        if (id < 0 || d == null) {
            // Error - clean up
            if (waitParam)
                sParamIf = null;
            waitParam = false;
            if (id > 0) {
                queryQsize = queryQ.size();
                if (id <= queryQsize) {
                    queryQ.setElementAt(null, id-1);
                }
            }
            return;
        }

        // Turn '\n' into line feed
        if (d.indexOf("\\n") >= 0) {
            StringBuffer sb = new StringBuffer(d);
            for (int j=0, k=0; (j=d.indexOf("\\n", j)) >= 0; j++) {
                sb.delete(j+k, j+k+2);
                sb.insert(j+k, '\n');
                k--;
            }
            d = sb.toString();
        }

        if (id > 0) {
            queryQsize = queryQ.size();
            if (id <= queryQsize) {
                QueryData qd = (QueryData)queryQ.elementAt(id - 1);
                if (qd != null) {
                   paramIf = new ParamIF(qd.name, "string", d);
                   VObjIF vobj = qd.object;
                   if (vobj != null) {
                       vobj.setValue(paramIf);
                   }
                }
                queryQ.setElementAt(null, id-1);
            }
            return;
        }
        // Answers to sync requests have ID of 0. Handle them here.
        sParamIf = new ParamIF(syncParam, "string", d);
        syncWaitParam = false;
        waitParam = false;      // Needed?
    }

    private void processShowQuery(String ds) {
        int     id;
        String  d;
        StringTokenizer tok = new StringTokenizer(ds, " ,\n");
        id = 0;
        if (!tok.hasMoreTokens())
            return;
        try {
            id = Integer.parseInt(tok.nextToken());
        }
        catch (NumberFormatException er) { return; }
        if (!tok.hasMoreTokens())
            return;
        d = tok.nextToken("\n").trim();
        if (d.startsWith("E@@"))
            d = null;
        queryQsize = queryQ.size();
        if (id < 1 || id > queryQsize) {
            return;
        }

        QueryData qd = (QueryData)queryQ.elementAt(id - 1);

        queryQ.setElementAt(null, id-1);
        if (d == null || qd == null) {
            return;
        }
        VObjIF vobj = qd.object;
        paramIf = new ParamIF(qd.name, "string", d);
        if (vobj != null) {
            vobj.setShowValue(paramIf);
        }
    }

    /**********
    private void processAsyncQuery(String ds) {
        String  name, type;
        int     m;
        int     arraySize;
        StringTokenizer tok = new StringTokenizer(ds, " ,\n");
        name = null;
        type = null;
        paramIf = null;
        if (tok.hasMoreTokens()) {
            name = tok.nextToken();
        }
        if (tok.hasMoreTokens()) {
            type = tok.nextToken();
        }
        if (type == null || !tok.hasMoreTokens()) {
            return;
        }
        if (type.equals("r")) { // real number
            paramIf = new ParamIF(name, "real", tok.nextToken("\n").trim());
        }
        else if (type.equals("s")) { // string
            paramIf = new ParamIF(name, "string", tok.nextToken("\n").trim());
        }
        else if (type.equals("t")) { // real number array
            arraySize = 0;
            try {
                arraySize = Integer.parseInt(tok.nextToken());
            }
            catch (NumberFormatException er) { return; }
            if (arraySize <= 0)
                return;
            arrayVal = new String[arraySize];
            for (m = 0; m < arraySize; m++) {
                if (tok.hasMoreTokens())
                   arrayVal[m] = tok.nextToken().trim();
                else {
                   waitParam = false;
                   return;
                }
            }
            paramIf = new ParamIF(name, "realArray", arrayVal);
        }
        else if (type.equals("u")) { // string array
            arraySize = 0;
            m = -1;
            try {
                arraySize = Integer.parseInt(tok.nextToken());
                if (tok.hasMoreTokens())
                    m = Integer.parseInt(tok.nextToken());
            }
            catch (NumberFormatException err) { return; }
            if (arraySize <= 0 || m < 0 || !tok.hasMoreTokens()) {
                // something wrong
                waitParam = false;
                return;
            }
            if (m == 0)
                arrayVal = new String[arraySize];
            if (m < arraySize)
                arrayVal[m] = tok.nextToken("\n").trim();
            else
                waitParam = false;
            if (m == arraySize -1 ) {
                waitParam = false;
                paramIf = new ParamIF(name, "stringArray", arrayVal);
            }
            return;
        }
        else if (type.equals("d")) { // dgroup
            paramIf = new ParamIF(name, "dGroup", tok.nextToken("\n").trim());
        }
        else if (type.equals("a")) { // active status
            paramIf = new ParamIF(name, "pStatus", tok.nextToken("\n").trim());
        }
    }
    **********/

    private void processSyncExpr(String ds) {
        StringTokenizer tok = new StringTokenizer(ds, " ,\n");
        sParamIf = null;
        if (tok.hasMoreTokens()) {
            try {
                 Integer.parseInt(tok.nextToken());
            }
            catch (NumberFormatException er) { return; }
        }
        if (tok.hasMoreTokens()) {
            sParamIf = new ParamIF(syncParam, "Expr", tok.nextToken("\n").trim());
        }
        waitParam = false;
    }

    public void setParamPanels() {
        controlPanel = Util.getControlPanel();
        if (panVector == null)
            setTabPanels(paramTabCtrl.getPanelTabs());
        if (!active || panVector == null)
            return;
        
        paramTabCtrl.updateTabValue();

        ExpPanInfo info;
        int nLength = panVector.size();
        for (int k = 0; k < nLength; k++) {
            info = (ExpPanInfo)panVector.elementAt(k);
            // controlPanel.setParamPanel(winId, info.name, info.paramPanel);
            // controlPanel.setActionPanel(winId,info.name,info.actionPanel);
            if (info.paramPanel != null)
                paramTabCtrl.setParamPanel(info.name, info.paramPanel, info.fpathIn);
            if (info.actionPanel != null)
               paramTabCtrl.setActionPanel(info.name,info.actionPanel, info.afpathIn);
        }
    }

    public void setParamPanel(String name) {
    }

    public ParameterPanel getParamPanel(String name) {
         if (panVector == null)
            return null;
         ExpPanInfo info;
         int nLength = panVector.size();
         for (int k = 0; k < nLength; k++) {
            info = (ExpPanInfo)panVector.elementAt(k);
            if (info.name.equals(name))
                return (ParameterPanel)info.paramPanel;
         }
         return null;
    }

    /***********
    public ArrayList getParamPanels()
    {
        ArrayList<JComponent> aListParamPanels = new ArrayList<JComponent>();
        int nLength = paramPanelList.size();
        for (int i = 0; i < nLength; i++)
        {
            ExpPanInfo expPanInfo = (ExpPanInfo)paramPanelList.get(i);
            aListParamPanels.add(expPanInfo.paramPanel);
        }
        return aListParamPanels;
    }
    ***********/


    public void quit() {
        bExiting = true;
        if (vpLayoutInfo != null)
           vpLayoutInfo.saveLayoutInfo();
        if (tearCanvas != null) {
           tearCanvas.setVisible(false);
           tearCanvas.dispose();
           tearCanvas = null;
        }
        sendToVnmr("jFunc("+VCLOSE+","+CLOSEALL+")\n");
        sendToVnmr("vnmrexit\n");
        if (vnmrProcess != null) {
            vnmrProcess.killProcess();
        }
        if (statusProcess != null) {
            statusProcess.killProcess();
        }
        if (vnmrThread != null) {
            if (vnmrThread.isAlive()) {
                try {
                     vnmrThread.join(500);
                }
                catch (InterruptedException e) { }
            }
            vnmrThread = null;
        }
        if (statusThread != null) {
            if (statusThread.isAlive()) {
                try {
                     statusThread.join(500);
                }
                catch (InterruptedException e) { }
            }
        }
        if (vsocketThread != null) {
            if (vsocketThread.isAlive()) {
                vsocket.quit();
                try {
                     vsocketThread.join();
                }
                catch (InterruptedException e) { }
            }
        }
        if (stSocketThread != null) {
            if (stSocketThread.isAlive()) {
                stSocket.quit();
                try {
                     stSocketThread.join(500);
                }
                catch (InterruptedException e) { }
            }
        }
        if (vcanvas != null)
            vcanvas.freeObj();
        if(gldialog!=null)
            gldialog.destroy();
        gldialog=null;
        glcom=null;
        vnmrProcess = null;
        statusProcess = null;
        statusThread = null;
        stSocket = null;
        vsocket = null;
        vsocketThread = null;
        vcanvas = null;
        csiCanvas = null;
        subButPanel = null;
        csiButPanel = null;
        titleBar = null;
        resizeBut = null;
        tearBut = null;
        busyBut = null;
        dpsBut = null;
        dirBut = null;
        linkBut = null;
        checkBut = null;
        fdaBut = null;
        buttonList.clear();
        if (buttonPan != null)
           buttonPan.removeAll();
        buttonPan = null;
        activeSensor = null;
        helpBar = null;
        qPanel = null;
    }

    /*********
    private void moveMenu() {
        Dimension dim = getSize();
        Point pt = getLocation();
        Dimension butDim;
        int x;
        int y = 0;
        int h = 14;
        if (bExiting)
        return;
        x = dim.width - 2;
        if (winId == 0 && !isFullWidth) {
           if (useBusyIcon)
              x = dim.width - 12;
        }
        if (titleComp != null) {
           h = titleComp.getPreferredSize().height;
           if (h < 0)
             h = 14;
        }
        int nLength = buttonList.size();
        for (int k = 0; k < nLength; k++) {
           JButton but = (JButton) buttonList.elementAt(k);
           if ((but != null) && but.isVisible()) {
                butDim = but.getPreferredSize();
                x = x - butDim.width;
                if (x < 10)
                    return;
                if (butDim.height > h)
                    butDim.height = h;
                but.setBounds(x, y, butDim.width, butDim.height);
                x = x - 4;
           }
        }

        butDim = busyBut.getPreferredSize();
        x = x - butDim.width - 4;
        if ( x < 2)
           x = 2;
        busyBut.setBounds(x, 2, butDim.width, butDim.height);
    }
    *********/

    public void setBusy(boolean bz) {
        if (useBusyIcon) {
            busyBut.setVisible(bz);
        }
        if (useBusyBar) {
            SmsPanel sPan = ((ExpViewArea)expIf).getSmsPanel();
            if (bz) {
                if (titleComp.isShowing())
                   titleComp.setBusy(bz);
                else if (sPan != null && sPan.isShowing())
                   sPan.setBusy(bz);
            }
            else {
                titleComp.setBusy(bz);
                if (sPan != null)
                   sPan.setBusy(bz);
	    }
        }
        if (vcanvas != null)
           vcanvas.setBusy(vbgBusy);
/*
        if (bz)
            setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
        else
            setCursor(null);
*/
    }

    public void buttonActiveCall(int num) {
        String mess;
        num++;
        if (outPort != null) {
           mess = "jFunc("+VMENU+","+num+")\n";
           sendToVnmr(mess);
        }
    }

    public void funcKeyCall(int num) {
        if (num <= buttonNum)
           buttonActiveCall(num);
    }

    public void tearOffOpen(boolean  open) {
        if (open) {
           if (bNative) {
                sendToVnmr("jFunc("+PAINT2+")\n");
                return;
           }
           // tearBut.setVisible(false);
        }
/*
        else
           tearBut.setVisible(true);
*/
    }

    public void childMouseProc(MouseEvent ev, boolean release, boolean drag) {
        if (drag) {
            if (gok) {
                mouseProc (ev, 0, 1);
            }
            else {
            }
        }
        else {
            if (release)
                mouseProc (ev, 1, 0);
            else
                mouseProc (ev, 0, 0);
        }
    }

    public VContainer getLcPanel() {
        return lcPanel;
    }

  //### G3D DEV ##############
    public JPanel getOglPanel() {
        return glPanel;
    }
  //###########################

    public void setPanelGeometry(VContainer p, int size, char pos) {
        int iPos = VSeparator.SOUTH;

        switch (pos) {
           case 'N':
           case 'T':
                    iPos = VSeparator.NORTH;
                    break;
           case 'S':
           case 'B':
                    iPos = VSeparator.SOUTH;
                    break;
           case 'E':
           case 'R':
                    iPos = VSeparator.EAST;
                    break;
           case 'W':
           case 'L':
                    iPos = VSeparator.WEST;
                    break;
           default:
                    iPos = VSeparator.SOUTH;
                    break;
        }
        if (iPos > 100)
           iPos = 100;
        if (iPos < 0)
           iPos = 0;
            vpBorder.setAttachement(iPos);
            vpBorder.setRatio(size);
        if ( p == lcPanel) {
            if (iLcSize != size || iLcPos != iPos) {
                iLcSize = size;
                iLcPos = iPos;
                saveLcInfo();
                revalidate();
            }
        }
    }

    public int getSplitPosition() {
        return iLcPos;
    }

    public void setSplitPos(VSeparator comp, int pos) {
        if (comp == vpBorder) {
            if (iLcPos != pos) {
                iLcPos = pos;
                saveLcInfo();
                revalidate();
            }
        }
    }

    public void setSplitSize(VSeparator comp, int size) {
        if (comp == vpBorder) {
        	if (glPanel.isVisible() && glSize!=size){
        		glSize=size;
                if(gldialog!=null)
                	gldialog.setSplitRatio(glSize);
        		revalidate();
        	}
        	if (lcPanel.isVisible() && iLcSize != size) {
                iLcSize = size;
                saveLcInfo();
                revalidate();
            }
        }
    }

    public void  dbAction(String act, String type) {
        // if (type.equals(Shuf.DB_AUTODIR) || type.equals(Shuf.DB_STUDY) ) {
        if (type.equals(Shuf.DB_AUTODIR)) {
            SmsPanel sPan = ((ExpViewArea)expIf).getSmsPanel();
            if (sPan != null && (sPan.isVisible()))
                sPan.dbAction(type);
        }
    }

    public void  previewAnnotation(String f) {
        vcanvas.previewAnnotation(f);
    }

    public void setCsiActive(boolean s) {
         bCsiActive = s;
         if (s) {
            if (csiCanvas == null || !csiCanvas.isVisible())
               bCsiActive = false;
         }
         if (bCsiActive)
            actCanvas = csiCanvas;
         else
            actCanvas = vcanvas;
         vcanvas.setCsiActive(bCsiActive);
         if (csiCanvas != null)
            csiCanvas.setCsiActive(bCsiActive);
    }

    public boolean isCsiPanelVisible() {
         return bCsiOpened;
    }

    public void setCsiPanelOrient(boolean bVertical) {
        csiDividerOrient = SwingConstants.VERTICAL;
        if (bVertical)
            csiDividerOrient = SwingConstants.HORIZONTAL;
        if (csiDivider != null) {
            if (csiDividerOrient != csiDivider.getOrientation()) {
                csiDivider.setOrientation(csiDividerOrient);
            }
        }
    }

    public void setCsiPanelVisible(boolean s) {
        boolean bClearCanvas = false;
        // appIf.showCsiButtonPalette(s);
        if (bCsiOpened == s)
           return;
        bCsiOpened = s;
        if (!s) {
           sendToVnmr(new StringBuffer().append(JFUNC).append(CSIOPENED).append(", 0)").toString());
           ((ExpViewArea)expIf).showCsiButtonPalette(s);
           if (csiDivider == null || !csiDivider.isVisible())
               return;
           csiDividerLoc = csiDivider.getLocRatio();
           if (csiCanvas != null) {
               csiCanvas.setCsiActive(false);
               csiCanvas.setVnmrCanvas(null);
               csiCanvas.setVisible(false);
               // csiCanvas.setGeom(0, 0, 0, 0);
           }
           csiDivider.setVisible(false);
           vcanvas.setCsiCanvas(null);
           actCanvas = vcanvas;
           aipCanvas = vcanvas;
           setButtonBar();
           revalidate();
           return;
        }
        if (csiButPanel == null) {
           csiButPanel = new SubButtonPanel(this);
           csiButPanel.setButtonCode(AIPMENU);
        }
        // set csiCanvas visible
        if (csiCanvas == null) {
           csiCanvas = new VnmrCanvas(this.sshare, winId, this, bNative);
           csiCanvas.setCsiMode(true);
           csiCanvas.setVnmrCanvas(vcanvas);
           add(csiCanvas);
           bClearCanvas = true;
        }
        else if (!csiCanvas.isVisible())
           bClearCanvas = true;
        if (csiDivider == null) {
           csiDivider = new VDivider(this);
           csiDivider.setOrientation(csiDividerOrient);
           csiDivider.setLocRatio(csiDividerLoc);
           add(csiDivider);
        }
        sendToVnmr(new StringBuffer().append(JFUNC).append(CSIOPENED).append(", 1)").toString());
        csiCanvas.setVnmrCanvas(vcanvas);
        vcanvas.setCsiCanvas(csiCanvas);
        csiDivider.setVisible(true);
        csiCanvas.setVisible(true);
        aipCanvas = csiCanvas;
        if (bClearCanvas) {
            vcanvas.clearCanvas();
            subButPanel.removeAllButtons();
            subButPanel.showAllButtons(); // repaint 
        }
        ((ExpViewArea)expIf).showCsiButtonPalette(s);
        // if (active)
        //    appIf.setCsiButtonBar(csiButPanel);
        setButtonBar();
        
        revalidate();
    }

    private void initCsiPanel() {

        if (vcanvas == null)
            return;
        String name = CSIPANE+winId;
        Boolean cData = (Boolean) sshare.getProperty(name);
        boolean csiOpened = false;
        if (cData != null)
            csiOpened = cData.booleanValue();
        name = CSIDIVIDER+winId;
        Float iData = (Float) sshare.getProperty(name);
        if (iData != null)
        {
            csiDividerLoc = iData.floatValue();
             if (csiDividerLoc > 0.8f)
                csiDividerLoc = 0.8f;
             if (csiDividerLoc < 0.1f)
                csiDividerLoc = 0.1f;
        }
        name = GLDIVIDER+winId;
        iData = (Float) sshare.getProperty(name);
        if (iData != null) {
            glDividerLoc = iData.floatValue();
            if (glDividerLoc > 0.8f)
                glDividerLoc = 0.8f;
            if (glDividerLoc < 0.1f)
                glDividerLoc = 0.1f;
        }
        if (vcanvas.bImgType && csiOpened)
            setCsiPanelVisible(csiOpened);
    }

    /********
    private void saveCsiInfo() {
        if (vcanvas == null || sshare == null)
           return;
        if (!vcanvas.bImgType)
           return;
        if (csiDivider != null)
            csiDividerLoc = csiDivider.getLocRatio();
        boolean bOpened = false;
        String name = CSIPANE+winId;
        if (csiCanvas != null && csiCanvas.isVisible())
            bOpened = true;
        sshare.putProperty(name, new Boolean(bOpened));
        name = CSIDIVIDER+winId;
        sshare.putProperty(name, new Float(csiDividerLoc));

        bOpened = false;
        if (glDivider != null)
            glDividerLoc = glDivider.getLocRatio();
        if (glPanel !=null && glPanel.isVisible())
            bOpened = true;
        name = GLPANE+winId;
        sshare.putProperty(name, new Boolean(bOpened));
        name = GLDIVIDER+winId;
        sshare.putProperty(name, new Float(glDividerLoc));
    }
    ********/

    private void layoutComponents(int w, int h, boolean bDoLayout) {
        Insets insets = getInsets();
        Point pt = getLocation();
        int     x;
        int     cw = w - insets.left - insets.right;
        int     ch = h - insets.bottom;

        if (bExiting)
            return;
        xGap = insets.left;
        yGap = insets.top;
        Dimension td;
        int     x2 = w - insets.right;
        int     y2 = ch;
        int     dividerSize = 4;
        int     w2 = 0;
        float   fv;
        if (x2 < 8 || ch < 8) // too small
             return;
        td = buttonPan.getPreferredSize();
        w2 = td.width + 2;
        if (bDoLayout)
            buttonPan.setLocation(cw - td.width, 0);
        // if (titleComp != null && titleComp.isVisible()) {
        if (showTitleBar) {
            td = titleComp.getPreferredSize();
            if (td.height <= 2)
                td.height = titleHeight;
            else if (td.height != titleHeight) {
                titleHeight = td.height;
                sshare.putProperty("titleHeight", new Integer(titleHeight));
            }
            x = xGap;
            if (pt.x < 10 && pt.y < 10) { //  on top left corner
                if (trayButton != null) {
                    trayButton.setBounds(xGap, 0, titleHeight, titleHeight);
                    if (trayButton.isVisible())
                        x = xGap + titleHeight;
                 }
            }
            if (m_btnVJmol != null && m_btnVJmol.isVisible()) {
                 if (bDoLayout)
                   m_btnVJmol.setBounds(new Rectangle(x, yGap, 16, titleHeight));
                 x = x + 16;
            }
            td.width = cw;
            if (bDoLayout) {
                titleComp.setPreferredSize(td);
                titleComp.setBounds(new Rectangle(x, yGap, cw-x, td.height));
                w2 = cw - x - w2;
                if (w2 < 0)
                   w2 = 0;
                activeSensor.setPreferredSize(new Dimension(w2, td.height));
                activeSensor.setBounds(new Rectangle(0, 0, w2, td.height));
            }
            yGap = yGap + td.height;
            ch = ch - yGap;
            expIf.setViewMargin(td.height);
            td = buttonPan.getPreferredSize();
            x = cw - x - td.width;
            if (x < 0)
                x = 0;
            if (bDoLayout)
                buttonPan.setLocation(x, 0);
        } // showTitleBar
        if (ch <= 1)
            ch = 1;
        if (cw <= 1)
            cw = 1;

        //### G3D DEV ##############

        if (gldialog != null && glPanel.isVisible()) {
            Rectangle r=titleComp.getBounds();
            int iBY, iBX, iBW;
            yGap=r.height;
            //yGap=td.height;
            iBY = yGap;
            iBX = xGap;
            iBW = 6;

            glSize=gldialog.getSplitRatio();

            if (bDoLayout)
                vpBorder.setRegion(xGap, yGap, xGap + cw, yGap + ch);

            int glY = yGap;
            int glX = xGap;
            int glW = cw * glSize / 100;
            cw = cw - glW;
            xGap = xGap + glW;
            if (glW < iBW)
                iBX = xGap;
            else
                iBX = xGap - iBW;
            if (bDoLayout) {
                glPanel.setBounds(glX, glY, glW, ch);
                vpBorder.setBounds(iBX, iBY, iBW, ch);
                vpBorder.setRatio(glSize);
                vpBorder.setAttachement(VSeparator.WEST);
                vpBorder.setVisible(true);
                //sendToVnmr("vnmrjcmd('g3d','repaint')");
                gldialog.setChangedPanel();
            }
        } // glPanel isVisible
        //##########################
        else if (lcPanel.isVisible()) { // only one or the other for now
            int iLcX, iLcY, iLcW, iLcH;
            int iBY, iBX, iBW;
            boolean isHorizontal = true;

            iLcX = xGap;
            iLcY = yGap;
            iLcW = cw;
            iLcH = ch;
            iBY = yGap;
            iBX = xGap;
            iBW = 6;
            if (bDoLayout) {
                vpBorder.setRegion(xGap, yGap, xGap + cw, yGap + ch);
                vpBorder.setRatio(iLcSize);
            }
            switch (iLcPos) {
                case VSeparator.SOUTH:
                            iLcH = ch * iLcSize / 100;
                            ch = ch - iLcH;
                            iLcY = yGap + ch;
                            iLcX = xGap;
                            iLcW = cw;
                            if (iLcH < iBW)
                                iBY = iLcY + iLcH - iBW;
                            else
                                iBY = iLcY;
                            break;
                case VSeparator.NORTH:
                            iLcH = ch * iLcSize / 100;
                            ch = ch - iLcH;
                            iLcX = xGap;
                            iLcY = yGap;
                            iLcW = cw;
                            yGap = iLcY + iLcH;
                            if (iLcH < iBW)
                               iBY = iLcY;
                            else
                               iBY = yGap - iBW;
                            break;
                case VSeparator.WEST:
                            iLcH = ch;
                            iLcY = yGap;
                            iLcX = xGap;
                            iLcW = cw * iLcSize / 100;
                            cw = cw - iLcW;
                            xGap = xGap + iLcW;
                            if (iLcW < iBW)
                                iBX = xGap;
                            else
                                iBX = xGap - iBW;
                            isHorizontal = false;
                            break;
                case VSeparator.EAST:
                            iLcH = ch;
                            iLcY = yGap;
                            iLcW = cw * iLcSize / 100;
                            iLcX = cw - iLcW;
                            cw = cw - iLcW;
                            if (iLcW < iBW)
                                iBX = iLcX + iLcW - iBW;
                            else
                                iBX = iLcX;
                            isHorizontal = false;
                            break;
            }
            if (iLcW <= 0)
                iLcW = 0;
            if (iLcH <= 0)
                iLcH = 0;
            if (bDoLayout) {
                vpBorder.setAttachement(iLcPos);
                lcPanel.setBounds(iLcX, iLcY, iLcW, iLcH);
                if (isHorizontal)
                    vpBorder.setBounds(iBX, iBY, iLcW, iBW);
                else
                    vpBorder.setBounds(iBX, iBY, iBW, iLcH);
                vpBorder.setVisible(true);
            }
        } // lcPanel isVisible
        else {
            if (bDoLayout)
                vpBorder.setVisible(false);
        }
        if (ch <= 0)
            ch = 0;
        if (cw <= 0)
            cw = 0;
        if (bDoLayout) {
            Rectangle r = vcanvas.getBounds();
            if ((r.width != cw) || (r.height != ch))
                vcanvas.underConstruct(true);
            else
                vcanvas.underConstruct(false);
        }
        int csiW = 0;
        int csiH = ch;
        int csiX = xGap;
        int csiY = yGap;
        int canvasX = xGap;
        int canvasY = yGap;
        int canvasW = cw;
        int canvasH = ch;
        if (csiCanvas != null && csiCanvas.isVisible()) {
           csiW = cw / 2;
           if (csiDivider != null && csiDivider.isVisible()) {
               int orient = csiDivider.getOrientation();
               csiDivider.setCtrlRange(xGap, yGap, x2, y2);
               fv = csiDivider.getLocRatio();
               if (orient == SwingConstants.VERTICAL) {
                   csiW = (int) ((float) cw * fv);
                   if (csiW > 0) {
                      if ((csiX + csiW) >= (x2 - dividerSize))
                         csiW = x2 - dividerSize - csiX;
                   }
                   if (csiW < 0)
                      csiW = 0;
                   xGap = csiX + csiW;
                   csiDivider.setBounds(xGap, yGap, dividerSize, ch);
                   canvasX = xGap + dividerSize;
                   canvasW = x2 - canvasX;
                   if (canvasW < 0)
                      canvasW = 0;
               }
               else {
                   csiW = cw;
                   csiH = (int) ((float) ch * fv);
                   if (csiH > 0) {
                      if ((csiY + csiH) >= (y2 - dividerSize))
                         csiH = y2 - dividerSize - yGap;
                   }
                   if (csiH < 0)
                      csiH = 0;
                   yGap = csiY + csiH;
                   csiDivider.setBounds(xGap, yGap, cw, dividerSize);
                   canvasY = yGap + dividerSize;
                   canvasH = y2 - canvasY;
                   if (canvasH < 0)
                       canvasH = 0;
               }
           }
           if (bDoLayout)
                csiCanvas.setBounds(csiX, csiY, csiW, csiH);
           else
                csiCanvas.setGeom(csiX, csiY, csiW, csiH);
        }
        if (bDoLayout) {
            vcanvas.setBounds(canvasX, canvasY, canvasW, canvasH);
            if (imgPane != null && imgPane.isVisible())
               imgPane.setBounds(canvasX, canvasY, canvasW, canvasH);
        }
        else
            vcanvas.setGeom(canvasX, canvasY, canvasW, canvasH);
        if (bDoLayout && lcPanel.isVisible()) {
                /*
                 * NB: When the lcPanel is set visible, Vnmr tends to
                 * draw on its canvas after the lcPanel is drawn, but
                 * before Vnmr knows about its new canvas geometry.
                 * (So the lcPanel is erased by Vnmr.)
                 *
                 * Workaround:  At this point, Vnmr has been sent a
                 * message about its new size.  Send this message to
                 * ourself through Vnmr, so that the lcPanel is
                 * repainted *after* Vnmr has gotten the message about
                 * the new xcanvas size.
                 *
                 * TODO: Don't tell Vnmr to repaint until after it's
                 * been sent the new size.  The problem seems to be
                 * that VnmrXCanvas.componentShown() is getting
                 * called.
                 */
             sendToVnmr("lccmd('repaintLcGraph')");

                // NB: the listenerList is sometimes cleared, so make sure here
                // that lcPanel is always in the list if it's visible.
             if (lcPanel instanceof ExpListenerIF) {
                 addExpListener(lcPanel);
             }
        }
    }

    public Object getAipMap(int n) {
        if (csiCanvas != null && csiCanvas.isVisible())
           return csiCanvas.getAipMap(n);
        return vcanvas.getAipMap(n);
    }

    public Object getAipMap() {
        if (csiCanvas != null && csiCanvas.isVisible())
           return csiCanvas.getAipMap();
        return vcanvas.getAipMap();
    }

    @Override
    public void setBounds(int x, int y, int w, int h) {
        layoutComponents(w, h, false);
        super.setBounds(x, y, w, h);
    }

    private class titleLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()

        public Dimension preferredLayoutSize(Container target) {
            synchronized (target.getTreeLock()) {
                int w = 20;
                int h = 10;
                Dimension d;
                int nmembers = target.getComponentCount();
                for (int i = 0; i < nmembers; i++) {
                    Component comp = target.getComponent(i);
                    d = comp.getPreferredSize();
                    if (d.height > h)
                        h = d.height;
                    w += d.width;
                }
                if (h > 26)
                    h = 26;
                return new Dimension(w, h);
            }
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension size;
                Point loc;
                int x = 0;
                int x2 = 0;
                int w;
                Dimension dim = target.getSize();
                int nmembers = target.getComponentCount();
                for (int i = 0; i < nmembers; i++) {
                    Component comp = target.getComponent(i);
                    if (comp == helpBar) {
                         // helpBar.setSize(dim);
                         continue;
                    }
                    size = comp.getPreferredSize();
                    if (comp instanceof VObjIF) {
                       loc = ((VObjIF) comp).getDefLoc();
                       if (loc.x < x)
                           loc.x = x;
                       if (comp instanceof VLabel) {
                          w = ((VLabel) comp).getLabelWidth();
                          if (w > size.width)
                              size.width = w;
                          if (w > 1)
                              x2 = loc.x + w;
                       }
                       x = loc.x + size.width;
                    }
                    else {
                       loc = comp.getLocation();
                    }
                    comp.setBounds(new Rectangle(loc.x, loc.y, size.width, size.height));
                    comp.validate();
                }
                w = dim.width - x2;
                if (w < 0)
                    w = 0;
                helpBar.setBounds(new Rectangle(x2, 0, w, dim.height));
            }
        } // layoutContainer()
    }

    private class buttonPanLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()

        public Dimension preferredLayoutSize(Container target) {
            synchronized (target.getTreeLock()) {
                int w = 4;
                int h = 10;
                int num = target.getComponentCount();
                Dimension dim;
                for (int i = 0; i < num; i++) {
                    Component comp = target.getComponent(i);
                    if (comp != null) {
                       dim = comp.getPreferredSize();
                       if (dim.height > h)
                           h = dim.height;
                       w = w + dim.width + 4;
                    }
                }
                return new Dimension(w, h);
            }
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension d = target.getSize();
                int x = d.width - 2;
                int num = target.getComponentCount();
                if (x < 6)
                    return;
                for (int i = 0; i < num; i++) {
                    Component comp = target.getComponent(i);
                    if (comp.isVisible()) {
                       d = comp.getPreferredSize();
                       x -= d.width;
                       comp.setBounds(new Rectangle(x, 0, d.width, d.height));
                       comp.validate();
                       x -= 4;
                    }
                }
            }
        } // layoutContainer()
    }

    class canvasLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // preferredLayoutSize()

       public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension winSize = target.getSize();
                layoutComponents(winSize.width, winSize.height, true);
            }
        }
    }

    /*
     * ComponentListener Interface:
     */
    public void componentHidden(ComponentEvent e) {}
    public void componentMoved(ComponentEvent e) { }
    public void componentResized(ComponentEvent e) {}
    public void componentShown(ComponentEvent e) {
        Messages.postDebug("ExpPanelShown",
                           "ExpPanel.componentShown: panelReady="
                           + panelReady
                           + ", isShowing=" + isShowing()
                           + ", vcanvas.isShowing=" + vcanvas.isShowing());
        if (!panelReady) {
            if (isShowing() && vcanvas.isShowing()) {
                panelReady = true;
                mayRunVnmr();
            }
            if (vnmrRunning) {
                removeComponentListener(this);
                vcanvas.removeComponentListener(this);
            }
        }
    }

    private void sendButtonMask() {
/**
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(XMASK).
            append(",").append(Event.SHIFT_MASK).append(",").append(Event.CTRL_MASK).
            append(", ").append(Event.META_MASK).append(", ").append(Event.ALT_MASK).
            append(", ").append(InputEvent.BUTTON1_MASK).append(", ").
            append(InputEvent.BUTTON2_MASK).append(", ").
            append(InputEvent.BUTTON3_MASK).append(LINE);
**/
        StringBuffer sbcmd = new StringBuffer().append(JFUNC).append(XMASK).
            append(",").append(InputEvent.SHIFT_DOWN_MASK).append(",").append(InputEvent.CTRL_DOWN_MASK).
            append(", ").append(InputEvent.META_DOWN_MASK).append(", ").append(InputEvent.ALT_DOWN_MASK).
            append(", ").append(InputEvent.BUTTON1_MASK).append(", ").
            append(InputEvent.BUTTON2_MASK).append(", ").
            append(InputEvent.BUTTON3_MASK).append(LINE);

        sendToVnmr(sbcmd.toString());

        sbcmd = new StringBuffer().append(JFUNC).append(XMASK2).append(",").
            append(MouseEvent.MOUSE_PRESSED).append(",").
            append(MouseEvent.MOUSE_RELEASED).append(", ").
            append(MouseEvent.MOUSE_MOVED).append(", ").
            append(MouseEvent.MOUSE_DRAGGED).append(", ").
            append(MouseEvent.MOUSE_CLICKED).append(", ").
            append(MouseEvent.MOUSE_EXITED).append(", ").
            append(MouseEvent.MOUSE_ENTERED).append(LINE);

        /*d = "jFunc("+XMASK2+","+MouseEvent.MOUSE_PRESSED+","+
            MouseEvent.MOUSE_RELEASED+", "+MouseEvent.MOUSE_MOVED+", "+
            MouseEvent.MOUSE_DRAGGED+", "+MouseEvent.MOUSE_CLICKED+", "+
            MouseEvent.MOUSE_EXITED+", "+ MouseEvent.MOUSE_ENTERED+")\n";*/
        sendToVnmr(sbcmd.toString());
    }

    private void sendCanvasId() {
        int id, xNative;
        StringBuffer sbcmd = new StringBuffer();

        if (outPort == null)
            return;

	xNative = 0;
        id = Util.getWindowId();
        if (id > 0) {
            if (!bShowFlag) {
                sendToVnmr ("M@xstop\n");
            }
	    if (bNative)
		xNative = 1;
        }
        sbcmd.append(JFUNC).append(WINID).append(", '").append(id).
                append("', '").append(xNative).append("')\n");
        sendToVnmr(sbcmd.toString());
        int scrnDpi = Toolkit.getDefaultToolkit().getScreenResolution();
        sbcmd = new StringBuffer().append(JFUNC).append(SCRNDPI).
                append(", '").append(scrnDpi).append("')\n");
        sendToVnmr(sbcmd.toString());

	if (vcanvas != null) {
            vcanvas.setGraphicRegion();
            sendWinId = false;
        }
    }

    public void  saveLcInfo() {
        String sData = "lcSize"+this.winId;
        sshare.putProperty(sData, new Integer(iLcSize));
        sData = "lcPos"+this.winId;
        sshare.putProperty(sData, new Integer(iLcPos));
    }

    public void  initLcInfo() {
        String sData = "lcSize"+this.winId;
        Integer iData = (Integer) sshare.getProperty(sData);
        if (iData != null) {
            iLcSize = iData.hashCode();
        }
        sData = "lcPos"+this.winId;
        iData = (Integer) sshare.getProperty(sData);
        if (iData != null) {
            iLcPos = iData.hashCode();
        }
        vpBorder.setRatio(iLcSize);
        vpBorder.setAttachement(iLcPos);
    }

    public void  makeTitleBar() {
        String cstr = (String) sshare.getProperty("titleBar");
        showTitleBar = true;
        if (cstr != null && (cstr.equals("no")))
             showTitleBar = false;
        if (showTitleBar)
            makeXmlTitle();
        if (titleComp == null) {
            if (titleBar == null)
                titleBar = new BusyPanel();
             if (showTitleBar) {
                titleBar.add(buttonPan);
                titleBar.add(activeSensor);
                titleBar.add(helpBar);
            }
            titleComp = titleBar;
            titleBar.setLayout(new titleLayout());
        }
/*
        cstr = (String) sshare.userInfo().get("titleBg");
        if (cstr != null)
           titleBg = VnmrRgb.getColorByName(cstr);
        else
           titleBg = Util.getBgColor();

        cstr = (String) sshare.userInfo().get("titleHiBg");
        if (cstr == null)
            cstr = "brown";
        titleHiBg = VnmrRgb.getColorByName(cstr);

        cstr = (String) sshare.userInfo().get("titleFg");
        if (cstr == null)
            cstr = "blue";
        titleFg = VnmrRgb.getColorByName(cstr);
        cstr = (String) sshare.userInfo().get("titleHiFg");
        if (cstr == null)
            cstr = "white";
        titleHiFg = VnmrRgb.getColorByName(cstr);
*/
        setTitleColor();
        titleComp.setOpaque(true);

        helpBar.setLocation(100, 0);
        if (showTitleBar)
             titleComp.setVisible(true);
        else
             titleComp.setVisible(false);
        validate();
    }

    static public void addStatusListener(StatusListenerIF pan) {
        if (statusListenerList == null)
            return;
        if (!statusListenerList.contains(pan)) {
            statusListenerList.add(pan);

            /* Send all the current status data to the new listener */
            synchronized(statusMessages) {
                Iterator<String> itr = statusMessages.values().iterator();
                while (itr.hasNext()) {
                    pan.updateStatus((String)itr.next());
                }
            }
        }
    }

    static public void updateStatusListener(StatusListenerIF pan) {
            /* Send all the current status data to the specified listener */
            synchronized(statusMessages) {
                    Iterator<String> itr = statusMessages.values().iterator();
                    while (itr.hasNext())
                        pan.updateStatus(itr.next());
            }
    }

    static public void removeStatusListener(StatusListenerIF pan) {
        statusListenerList.remove(pan);
    }

    static public void addExpListener(ExpListenerIF item) {
        if (expListenerList == null) {
            expListenerList = Collections.synchronizedList(new LinkedList<ExpListenerIF>());
        }
        if (!expListenerList.contains(item))
            expListenerList.add(item);
    }

    static public void removeExpListener(ExpListenerIF pan) {
        if (expListenerList != null)
            expListenerList.remove(pan);
    }

    static public void addLimitListener(VLimitListenerIF limitIF) {
        if (!m_aListLimitListener.contains(limitIF))
            m_aListLimitListener.add(limitIF);
    }

    static public void removeLimitListener(VLimitListenerIF pan) {
            m_aListLimitListener.remove(pan);
    }

    public void addAlphatextListener(VTextFileWin obj) {
        if (!alphatextList.contains(obj))
            alphatextList.add(obj);
    }

    public void removeAlphatextListener(VTextFileWin obj) {
        alphatextList.remove(obj);
    }

    public void setAlphatext(String f) {
        synchronized(alphatextList) {
            Iterator<VTextFileWin> itr = alphatextList.iterator();
            while (itr.hasNext()) {
                VTextFileWin obj = itr.next();
                obj.setValue(f);
            }
        }
    }

    private boolean sendPanelInfo(ExpPanInfo info, String param) {
        if (outPort == null)
            return false;
        String str;
        StringBuffer sbcmd;
       /***
        if (info.afname != null && info.afname.length() > 0) {
            int k = info.id + 90;
            sbcmd = new StringBuffer().append(JFUNC).append(VLAYOUT).
                         append(", ").append(k).append(", '").append(param).append("')\n");
            str = sbcmd.toString();
            sendToVnmr(str);
            if (debug) {
                String ms = Long.toString(new Date().getTime());
                int msc = ms.length() - 3;
                Messages.postDebug(ms.substring(0,msc)+"."+ms.substring(msc)
                                       +": sendPanelInfo:"+str);
            }
        }
        ****/
        if (info.fname != null && info.fname.length() > 0) {
            if (debug)
                    Messages.postDebug("query file for panel "+info.name);
            sbcmd = new StringBuffer().append(JFUNC).append(VLAYOUT).append(", ").
                         append(info.id).append(", '").append(param).append("')\n");
            str = sbcmd.toString();
            sendToVnmr(str);
            if (debug) {
                String ms = Long.toString(new Date().getTime());
                int msc = ms.length() - 3;
                Messages.postDebug(ms.substring(0,msc)+"."+ms.substring(msc)
                                       +": sendPanelInfo:"+str);
            }
        }
        info.isWaiting = true;
        info.needNewFile = false;
        return true;
    }

    public void queryPanelInfo() {
        if (panVector == null) {
            setTabPanels(paramTabCtrl.getPanelTabs());
            if (panVector == null)
                return;
        }
        // controlPanel = Util.getControlPanel();
        // if (active)
        //     controlPanel.setVnmrIf(this);
        //    panelName = controlPanel.getPanelName();
        panelName = paramTabCtrl.getPanelName();
        vpLayoutInfo.setPanelName(panelName);
        int nLength = panVector.size();
        for (int k = 0; k < nLength; k++) {
            ExpPanInfo info = (ExpPanInfo)panVector.elementAt(k);
            if (info.name.equals(panelName)) {
                // Load only the current panel
                StringTokenizer tok = new StringTokenizer(info.param," ,\n");
                String p = "";
                if (tok.hasMoreTokens())
                    p = tok.nextToken().trim();
                if (debug) {
                    String ms = Long.toString(new Date().getTime());
                    int msc = ms.length() - 3;
                    Messages.postDebug(ms.substring(0,msc)
                                          +"."+ms.substring(msc)
                                          +": Load panel #"+k+": p="+p
                                          +", info="+info);
                }
                info.isCurrent = true;
                if (sendPanelInfo (info, p))
                    needPanelInfo = false;
            }
            else {
                info.isCurrent = false;
                info.needNewFile = true;
            }
        }
    }

    public void queryPanelInfo(String name) {
        if (panVector == null || name == null)
            return;
        int nLength = panVector.size();
        for (int k = 0; k < nLength; k++) {
            ExpPanInfo info = (ExpPanInfo)panVector.elementAt(k);
            if (name.equals(info.name)) {
                StringTokenizer tok = new StringTokenizer(info.param," ,\n");
                String p = "";
                if (tok.hasMoreTokens())
                       p = tok.nextToken().trim();
                if (debug) {
                       String ms = Long.toString(new Date().getTime());
                       int msc = ms.length() - 3;
                       Messages.postDebug(ms.substring(0,msc)
                                          +"."+ms.substring(msc)
                                          +": Load panel #"+k+": info="+info);
                }
                info.isCurrent = true;
                info.toUpdate = false;
                vpLayoutInfo.setPanelName(name);
                //  controlPanel = Util.getControlPanel();
                if (info.needNewFile || info.paramPanel == null) {
                    // if (controlPanel != null)
                    //         controlPanel.setOutOfDateFlag(name, true);
                    paramTabCtrl.setOutOfDateFlag(name, true);
                    sendPanelInfo (info, p);
                }
                else {
                    // if (controlPanel != null)
                    //     controlPanel.setOutOfDateFlag(name, false);
                    paramTabCtrl.setOutOfDateFlag(name, false);
                }
            }
            else
                info.isCurrent = false;
        }
    }

    public void setTabPanels(Vector<JComponent> v) {
        if (v == null)
            return;
        if (panVector != null) {
            panVector.clear();
        } else {
            panVector = new Vector<ExpPanInfo>();
        }
        int nLength = v.size();
        for (int k = 0; k < nLength; k++) {
            Component comp = (Component)v.elementAt(k);
            if (comp instanceof VPanelTab) {
                VPanelTab vp = (VPanelTab) comp;
                ExpPanInfo info = new ExpPanInfo(k, vp.pname, vp.pfile, vp.actionFile, vp.param,vp.ptype);
                panVector.add(info);
            }
        }
        /***
        if (active) {
            controlPanel = Util.getControlPanel();
            // if (controlPanel != null)
            //     controlPanel.setVnmrIf(this);
        }
        ***/
/*
        if (expName != null) {
            queryPanelInfo();
        }
        else
*/
            needPanelInfo = true;
    }


    public void createBadPanel(ExpPanInfo info, int id, String fout) {
        if (id < 90) {
           info.isWaiting = false;
           info.fpathIn = "NOTFOUND";
           info.fpathOut = fout;
           info.dateOfFile = 0;
               ParameterPanel pan = new ParameterPanel(sshare, this,info);
           if (info.paramPanel != null
                    && !havePanel((ParameterPanel)info.paramPanel))
           {
                removeStatusListener((StatusListenerIF)info.paramPanel);
                ((ParameterPanel)info.paramPanel).freeObj();
           }
           info.paramPanel = pan;
           if (pan != null) {
                addStatusListener(pan);
           }
           /*****
           if (active && controlPanel != null) {
                 controlPanel.setParamPanel(info.name, pan);
           }
           *****/
           paramTabCtrl.setParamPanel(info.name, pan, null);
           pan = null;
           return;
        }
        info.afpathIn = "NOTFOUND";
        info.afpathOut = fout;
        info.dateOfAfile = 0;
        ActionBar apan = new ActionBar();
        info.actionPanel = apan;
        /***
        if (active && controlPanel != null) {
              controlPanel.setActionPanel(winId, info.name, apan);
        }
        ***/
        paramTabCtrl.setActionPanel(info.name, apan);
    }

    public void getPanelDirs(ExpPanInfo info, String fname, String ds) {
        info.layoutDir=ds;
        String dir=FileUtil.savePath("USER/LAYOUT");
        info.fpathOut=dir+File.separator+info.layoutDir;
        String file=FileUtil.getLayoutPath(info.layoutDir,fname);
        String defdir=FileUtil.getDefaultName(info.layoutDir);
        if(file==null)
            file="NOTFOUND";
        info.defaultDir=dir+"/"+defdir;
        info.fpathIn=file;
    }

    public void createParamPanel(int id, String expdir) {
        if (panVector == null)
            return;
        int pid;
        int k, m;
        boolean doGc = false;
        boolean goodFile = true;
        if (id >= 90)
            pid = id - 90;
        else
            pid = id;
        ExpPanInfo info = null;
        m_paramPanelId = null;
        int nLength = panVector.size();
        for (k = 0; k < nLength; k++) {
            info = (ExpPanInfo)panVector.elementAt(k);
            if (info.id == pid)
                break;
            info = null;
        }
        if (info == null)
            return;

        info.isWaiting = false;

        String fname = id<90? info.fname:info.afname;
        if (fname == null)
            return;
        getPanelDirs(info,fname,expdir);
        String fin=info.fpathIn;
        String fout=info.fpathOut;

        if (debug && id < 90)
            Messages.postDebug("file for panel "+info.name+" is "+fin);

        File fd = null;
        if (fin.equals("NOPARAM") || fin.equals("NOTFOUND")) {
            fin = "NOTFOUND";
            if (fout == null || fin.equals("NOPARAM") )
                fout = "NOTFOUND";
            goodFile = false;
        }
        else {
            fd = new File(fin);
            if (fd == null || !fd.isFile())
                goodFile = false;
        }

        controlPanel = Util.getControlPanel();

        if (!goodFile) {
            createBadPanel(info, id, fout);
            if (id < 90)
                info.needNewFile = false;
            return;
        }

        if (id < 90) { // parameter panel
            ParameterPanel pan;
            info.needNewFile = false;
            if (info.isCurrent)
                setPanelBusy(true);
            //    sendBusy();
            if (info.paramPanel != null) {
                if (info.fpathIn != null && info.fpathIn.equals(fin)
                   && info.dateOfFile == fd.lastModified()
                   && info.viewport == winId)
                {
                    // Already have that panel; just update values
                    pan = (ParameterPanel)info.paramPanel;
                    pan.setFileInfo(info);
                    // if (active && controlPanel != null)
                    //     controlPanel.setParamPanel(info.name, pan);
                    paramTabCtrl.setParamPanel(info.name, pan, info.fpathIn);
                    info.toUpdate = false;
                    pan.updateAllValue(true);
                    pan = null;
                    if (info.isCurrent)
                      setPanelBusy(false);
                    //  sendFree();
                    return;
                }
            }
            info.dateOfFile = fd.lastModified();
            info.toUpdate = true;

            // Look for required panel in storage
            pan = null;
            for (k = paramPanelList.size()-1; k>=0; --k) {
                ExpPanInfo epi = (ExpPanInfo)paramPanelList.get(k);
                if (epi.fpathIn != null && epi.fpathIn.equals(fin)
                    && epi.dateOfFile == fd.lastModified()
                    && epi.viewport == winId)
                {
                    // This is the panel we want; move it to the end
                    // of the list (most recently used position).
                    paramPanelList.remove(k);
                    paramPanelList.add(epi);
                    pan = (ParameterPanel)epi.paramPanel;
                    if (pan != null) {
                        pan.setFileInfo(info);
                        pan.updateAllValue(true);
                    }
                    break;
                }
            }
            if (pan == null) {
                ExpPanInfo epi;
                if (paramPanelList.size() >= paramPanelListLimit) {
                    // Delete least recently used panel
                    // epi = (ExpPanInfo)paramPanelList.remove(0);
                    boolean toRemove = false;
                    for (k = 0; k < paramPanelList.size(); k++) {
                        epi = (ExpPanInfo)paramPanelList.get(k);
                        toRemove = true;
                        for (m = 0; m < panVector.size(); m++) {
                           ExpPanInfo info2 = (ExpPanInfo)panVector.elementAt(m);

                           if (epi.paramPanel == info2.paramPanel) {
                                toRemove = false;
                                break;
                            }
                        }
                        if (toRemove) {
                            pan = (ParameterPanel)epi.paramPanel;
                            paramPanelList.remove(k);
                            break;
                        }
                    }
                    // pan = (ParameterPanel)epi.paramPanel;
                    // if (pan != null && (!controlPanel.havePanel(pan))) {
                    if (pan != null && (!paramTabCtrl.havePanel(pan))) {
                        // This is the last reference to this panel.
                        // Time to free memory.
                        removeStatusListener(pan);
                        pan.freeObj();
                        // This is too often for garbage collection.
                        // TODO: need to add it somewhere else?
                        // System.gc();
                        // System.runFinalization();
                        doGc = true;
                    }
                    if (debug) {
                        Messages.postDebug("Panel removed: "+
                           paramPanelList.size()+" remaining");
                    }
                }
                // Parse XML file "fin" to create new panel

                pan = new ParameterPanel(sshare,this,info);
                if (info.paramPanel != null
                    && !havePanel((ParameterPanel)info.paramPanel))
                {
                    // This is the last reference to this panel.
                    // Time to free memory.
                    removeStatusListener((StatusListenerIF)info.paramPanel);
                    ((ParameterPanel)info.paramPanel).freeObj();
                    // This is too often for garbage collection.
                    // TODO: need to add it somewhere else?
                    // System.gc();
                    // System.runFinalization();
                    doGc = true;
                }
                info.paramPanel = pan;
                epi = new ExpPanInfo(info.id,
                         winId,
                         info.name,
                         info.fname,
                         info.afname,
                         info.param,
                         info.ptype);
                epi.fpathIn = info.fpathIn;
                epi.dateOfFile = info.dateOfFile;
                epi.paramPanel = info.paramPanel;
                paramPanelList.add(epi);
                if (debug) {
                    Messages.postDebug("Panels Saved: "+paramPanelList.size()
                           +", Limit="+paramPanelListLimit +
                           "\npanVector: len="+panVector.size());
                }
            }
            if (pan != null)
                addStatusListener(pan);
            info.paramPanel = pan;

            // if (active && controlPanel != null)
            //    controlPanel.setParamPanel(info.name, pan);
            paramTabCtrl.setParamPanel(info.name, pan, info.fpathIn);
            pan = null;
            if (doGc) {
                System.gc();
                System.runFinalization();
            }
            // if (info.isCurrent)
                setPanelBusy(false);
            //    sendFree();
            return;
        }
        else  {  // action button panel
            if (info.afpathIn != null && info.afpathIn.equals(fin)
                && info.dateOfAfile == fd.lastModified())
            {
                // Already have that panel; just update values
                if (info.actionPanel != null) {
                    ((ActionBar)info.actionPanel).updateAllValue();
                    // if (active && controlPanel != null)
                    //   controlPanel.setActionPanel(winId, info.name, info.actionPanel);
                    paramTabCtrl.setActionPanel(info.name, info.actionPanel, info.afpathIn);
                    return;
                }
            }
            info.afpathIn = fin;
            info.afpathOut = fout;
            info.dateOfAfile = fd.lastModified();
            ActionBar apan = new ActionBar();
            apan.setOpaque(false);
            // apan.setFloatable(false);
            // apan.setMargin(new Insets(0, 0, 0, 0));

            try {
                apan = (ActionBar)LayoutBuilder.build(apan, this, fin);
            }
            catch(Exception e) { 
                return;
            }
            info.actionPanel = apan;
            apan.setFileInfo(info);
            apan.adjustLayout();
            // if (active && controlPanel != null)
            //     controlPanel.setActionPanel(winId, info.name, apan);
            paramTabCtrl.setActionPanel(info.name, apan, info.afpathIn);
            apan.updateAllValue();
        }
    }

    private void createParamPanel(int id) {
        if (panVector == null)
            return;
        int nLength = panVector.size();
        ExpPanInfo info = null;  
        for (int k = 0; k < nLength; k++) {
            info = (ExpPanInfo)panVector.elementAt(k);
            if ((info != null) && (info.id == id))
                break;
            if (id < 0) {
                if (info.isCurrent)
                   break;
            }
        }
        if (info == null)
            info = (ExpPanInfo)panVector.elementAt(0);
        if ((info == null) || (info.layoutDir == null))
            return;
        createParamPanel(info.id, info.layoutDir);
        createParamPanel(info.id + 90, info.layoutDir);
    }

    private boolean havePanel(ParameterPanel p) {
        for (int k=paramPanelList.size()-1; k>=0; --k) {
            ExpPanInfo epi = (ExpPanInfo)paramPanelList.get(k);
            if (epi.paramPanel == p) {
                return true;
            }
        }
        return false;
    }

    private boolean newPanel(ExpPanInfo pinfo, Vector<String> v) {
        Vector<String> pv = pinfo.paramVector;
        if (pv == null || pv.size() < 1)
            return false;
        int nLength = pv.size();
        int nLength2 = v.size();
        for (int k = 0; k < nLength; k++) {
            String p = (String) pv.elementAt(k);
            for (int i = 0; i < nLength2; i++) {
                if (p.equals(v.elementAt(i))) {
                    if (pinfo.isCurrent) {
                        sendPanelInfo(pinfo, p);
                    }
                    else
                        pinfo.needNewFile = true;
                    return true;
                }
            }
        }
        return false;
    }

    public void updateParamPanels(Vector<String> v) {
        if (panVector == null)
            return;
        ExpPanInfo info;
        int nLength = panVector.size();
        for (int k = 0; k < nLength; k++) {
            info = (ExpPanInfo)panVector.elementAt(k);
            if (!newPanel(info, v) && !info.isWaiting) {
                if (info.paramPanel != null) {
                   ParameterPanel pan = (ParameterPanel) info.paramPanel;
                   pan.updateValue(v);
                }
                if (info.actionPanel != null) {
                   ((ActionBar)info.actionPanel).updateValue(v);
                }
            }
        }
    }

    private void  updateTitleBar(String  param, String data) {
         if (xmlTitle == null)
             return;
         ParamIF pf = new ParamIF("title", "string", data);
         int nums = xmlTitle.getComponentCount();
         StringTokenizer tok;
         String          vars, v;
         for (int i = 0; i < nums; i++) {
            Component comp = xmlTitle.getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF obj = (VObjIF) comp;
                vars = obj.getAttribute(VARIABLE);
                if (vars == null)
                    continue;
                tok = new StringTokenizer(vars, " ,\n");
                while (tok.hasMoreTokens()) {
                    v = tok.nextToken();
                    if (v.equals(param)) {
                        obj.setValue(pf);
                        break;
                    }
                }
            }
        }
    }

    public void  makeXmlTitle() {
        String fpath = FileUtil.openPath("INTERFACE/TitleBar.xml");
        if(fpath==null)
                return;
        xmlTitle = new BusyPanel();
        // xmlTitle.setBorder(BorderFactory.createEmptyBorder());
        xmlTitle.setBorder(null);
        xmlTitle.setLayout(new titleLayout());
        try {
                LayoutBuilder.build(xmlTitle, this, fpath);
        }
        catch(Exception e) { xmlTitle = null; }
        if (xmlTitle == null)
            return;

        titleComp = xmlTitle;
        int i, k, mx, n;
        Component comp;
        int nums = xmlTitle.getComponentCount();
        Component compList[] = new Component[nums];
        int[] lx = new int[nums];
        for (i = 0; i < nums; i++) {
            comp = xmlTitle.getComponent(i);
            if (comp instanceof VLabel)
                 ((VLabel) comp).setTitlebarObj(true);
            compList[i] = comp;
            Point pt = comp.getLocation();
            if (pt.x > 2999)
                pt.x = 900;
            lx[i] = pt.x;
        }

        xmlTitle.removeAll();
        xmlTitle.add(buttonPan);
        xmlTitle.add(activeSensor);
        xmlTitle.add(helpBar);

        for (i = 0; i < nums; i++) {
             n = nums;
             mx = 9999;
             for (k = 0; k < nums; k++) {
                 if (mx > lx[k]) {
                      n = k;
                      mx = lx[k];
                 }
             }
             if (n < nums) {
                 lx[n] = 9999;
                 xmlTitle.add(compList[n]);
             }
        }
    }

    public void showTrayButton(boolean b) {
         if (b) {
             if (trayInfo == null) {
                trayInfo = new TrayObj(sshare, this, "button");
                trayInfo.setEnabled(false);
                trayInfo.destroy(); // DisplayOptions.removeChangeListener
                trayInfo.setAttribute(SHOW, "if (traymax > 0.5) then $SHOW=1 else $SHOW=-1 endif");
             }
             if (trayButton == null)
                trayButton = ((ExpViewArea)expIf).getTrayButton();
             trayInfo.updateValue();
         }
    }

    protected void showVJmolButton(boolean bShow)
    {
        if (bShow) {
             m_btnVJmol = new VJButton("m");
             m_btnVJmol.setBackground(Util.getBgColor());
             m_btnVJmol.setVisible(Util.isVJmol());
             m_btnVJmol.setToolTipText(Util.getTooltipString("Open molecule panel"));
             m_btnVJmol.setOpaque(true);
             m_btnVJmol.setBorderAlways(true);
             add(m_btnVJmol, 0);
             m_btnVJmol.addActionListener(new ActionListener() {
                 public void actionPerformed(ActionEvent evt) {
                     // ((ExperimentIF)appIf).showJMolPanel(true);
                     // ((ExperimentIF)appIf).showJMolPanel(true);
                     VnmrjIF vjIf = Util.getVjIF();
                     if (vjIf != null)
                         vjIf.showJMolPanel(true);
                 }
             });
         }
         else {
             m_btnVJmol  = null;
         }

    }


    public void repaintTitleBar() {
        if (titleBar != null) {
            titleBar.repaint();
            //Messages.postDebug("REPAINTED titleBar");
        }
        if (titleComp != null) {
            titleComp.repaint();
            //Messages.postDebug("REPAINTED titleComp");
        }
    }

    private class QueryData {
        public VObjIF object;
        public String name;
        public QueryData(VObjIF obj, String s){
            this.object=obj;
            this.name=s;
        }
    } // class QueryData

    public void doSomething() {
        if (panelBusy) {
            return;
        }
        if (panVector == null)
            return;
        ExpPanInfo info;
        int nLength = panVector.size();
        for (int k = 0; k < nLength; k++) {
            info = (ExpPanInfo)panVector.elementAt(k);
            if (info.isWaiting)
                continue;
            if (info.needNewFile) {
                Vector<?> pv = info.paramVector;
                String p = "";
                if (pv != null && pv.size() > 0)
                    p = (String) pv.elementAt(0);
                sendPanelInfo(info, p);
                return;
            }
        }
        nLength = panVector.size();
        for (int k = 0; k < nLength; k++) {
            info = (ExpPanInfo)panVector.elementAt(k);
            if (info.isWaiting)
                continue;
            if (!info.needNewFile) {
                ParameterPanel pan = (ParameterPanel)info.paramPanel;
                if (pan != null && info.toUpdate) {
                    info.toUpdate = false;
                    pan.updateAllValue();
                    if (info.actionPanel != null)
                        ((ActionBar)info.actionPanel).updateAllValue();
                    return;
                }
            }
        }
    }

    public boolean outPortReady() {
        if(outPort == null)
            return false;
        else
            return true;
    }

    public void setLogMode(int type, boolean value) {
        switch (type) {
            case VnmrJLogWindow.COMMAND:
                  bDebugCmd = value;
                  break;
            case VnmrJLogWindow.PARAM:
                  bDebugPnew = value;
                  break;
            case VnmrJLogWindow.QUERY:
                  bDebugQuery = value;
                  break;
            case VnmrJLogWindow.UICMD:
                  bDebugUiCmd = value;
                  break;
            case VnmrJLogWindow.SHOWLOG:
                  if (!value) {
                      bDebugCmd = value;
                      bDebugPnew = value;
                      bDebugQuery = value;
                      bDebugUiCmd = value;
                  }
                  break;
        }
    }

    public void setVnmrInfo(int type, String value) {
        if (type == APPTYPE)
            vcanvas.setApptype(value);
    }

    /**
     *  This class stores the actionbar id and filename, and the panel id and filename
     *  initially for the current panel.
     */
    class ParamPanelId
    {
        static final int SIZE = 3;
        protected int[] m_nArrPanelId = new int[SIZE];
        protected String[] m_strArrPanelFile = new String[SIZE];
        protected boolean m_bBuild = false;
        private int m_nIndex = 0;

        public void addPanelInfo(int id, String file)
        {
            if (m_nIndex < SIZE)
            {
                m_nArrPanelId[m_nIndex] = id;
                m_strArrPanelFile[m_nIndex] = file;
                m_nIndex = m_nIndex + 1;
            }
        }

        public boolean isPanelBuilt()
        {
            return m_bBuild;
        }

        public void setPanelBuilt(boolean bBuild)
        {
            m_bBuild = bBuild;
        }

        public void clear()
        {
            m_bBuild = false;
            m_nArrPanelId = new int[SIZE];
            m_strArrPanelFile = new String[SIZE];
        }
    }


    /**
     * This is a JPanel that can display a moving, wavy background to
     * indicate a "busy" condition.
     */
    class BusyPanel extends JPanel {
        private final int xCycle = 25; // Not zero!
        private final int yCycle = 5;
        private final int cycle = 2 * xCycle + (2 * yCycle * yCycle) / xCycle;
        private final int delay = 100; // milliseconds between updates
        private int phase = 0;
        private Timer busyBarTimer = null;
        private boolean busy = false;
        private Color bg1;
        private Color grad1;
        private Color grad2;


        /**
         * Construct a BusyPanel with the default wave pattern.
         */
        public BusyPanel() {
            ActionListener updatePattern = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                    repaint();
                }
            };
            busyBarTimer = new Timer(delay, updatePattern);
            busyBarTimer.setRepeats(true);
            grad1 = new Color(0, 133, 213); // Agilent blue
            grad2 = new Color(8, 187, 249);
            setBusyColor();
            setBackground(getBackground());
        }

        /**
         * Override the JPanel method in order to keep two background colors.
         * One color is the normal background.  The other is derived from
         * the first, and is used to make the "wavy" pattern.
         * The second color will be made a shade brighter than the first,
         * or, if that is not possible, a shade darker.
         * @param c The desired normal background color.
         */
        public void setBackground(Color c) {
            bg1 = c;
            Util.changeBrightness(c, 110);
            super.setBackground(c);
        }

        public void setBusyColor() {
            // grad1 = DisplayOptions.getVJColor("VJCtrlHighlightColor");
            // grad2 = DisplayOptions.getVJColor("VJShadowColor");
        }

        /**
         * Paint the wavy pattern if busy, otherwise solid color.
         * The "phase" of the pattern is incremented on each call,
         * so that repeated calls make the pattern move.
         */
        protected void paintComponent(Graphics g) {
            if (busy) {
                Graphics2D g2 = (Graphics2D)g;
                g2.setPaint(new GradientPaint(phase, 0, grad1,
                                              phase + xCycle, yCycle, grad2,
                                              true));
                g2.fill(new Rectangle(0, 0, getWidth(), getHeight()));
                phase = ++phase % cycle;
            } else {
                g.setColor(bg1);
                g.fillRect(0, 0, getWidth(), getHeight());
            }
         //    super.paintComponent(g);
        }

        /**
         * Set whether the panel is busy or not.  Calling this results
         * in a repaint.
         * @param bz Specify "true" to make it busy.
         */
        public void setBusy(boolean bz) {
            if (busy == bz)
                return;
            busy = bz;
            if (bz) {
                if (!busyBarTimer.isRunning())
                    busyBarTimer.restart();
                // busyBarTimer.start();
            } else {
                busyBarTimer.stop();
            }
            // titleComp.repaint();
            repaint();
/*
            if (!bz) {
                paintResizeIcon();
            }
*/
        }

    }

    private class TrayObj extends VTab {
        public TrayObj(SessionShare sshare, ButtonIF vif, String typ) {
           super(sshare, vif, typ);
        }

        public void setShowValue(ParamIF pf) {
            int  val = 0;
            if (pf != null && pf.value != null) {
                String  s = pf.value.trim();
                val = Integer.parseInt(s);
            }
            if (val > 0) {
                if (active)
                  ((ExpViewArea)expIf).setTraybuttonEnabled(true);
            }
            else {
                if (active)
                  ((ExpViewArea)expIf).setTraybuttonEnabled(false);
            }
        }
    }

    private class ExpInfoObj extends SmsInfoObj {
        private ExpPanel exp;
        private int id;
         
        public ExpInfoObj(String val, int id) {
            super(val, id);
            this.id = id;
        }

        public void setValue(ParamIF pf) {
            if (exp == null)
                return;
            if (pf != null && pf.value != null) {
                 exp.setVnmrInfo(id, pf.value);
            }
        }

    }

    private int winId = 0;
    private boolean gok = true;
    private boolean vnmrReady = false;
    private boolean active = false;
    private boolean vnmrRunning = false;
    private boolean isFullWidth = false;
    private boolean showTitleBar = true;
    private int socketPort = 0;
    private int statusPort = 0;
    private int yGap = 0;
    private int xGap = 0;
    private int titleHeight = 20;
    private int queryQsize;
    private String vnmrHostName;
    private String localHostName;
    private String syncParam;
    private String expDir;
    private ParamIF paramIf;
    private ParamIF sParamIf;
    private boolean waitParam = false;
    private boolean syncWaitParam = false;
    private boolean titleActive = false;
    private int vnmrHostPort = 0;
    private CanvasOutSocket outPort = null;
    private JButton resizeBut;
    private JButton tearBut;
    private JButton busyBut;
    private JButton dpsBut;
    private JButton dirBut;
    private JButton linkBut;
    private JButton checkBut;
    private JButton fdaBut;
    private JPanel  buttonPan;
    private BusyPanel titleBar = null;
    private BusyPanel titleComp = null;
    private Vector<String> paramVector = null;
    private Vector<String> pVector;
    private Vector <QueryData> queryQ;
    private VnmrCanvas vcanvas;
    private VnmrCanvas printCanvas = null;
    private VnmrCanvas csiCanvas = null;
    private VnmrCanvas actCanvas = null;
    private VnmrCanvas aipCanvas = null;
    private VnmrTearCanvas tearCanvas;
    private VnmrProcess vnmrProcess;
    private StatusProcess statusProcess = null;
    private CanvasSocket vsocket;
    private StatusSocket stSocket;
    private Thread vsocketThread;
    private Thread vnmrThread;
    private Thread statusThread = null;
    private Thread stSocketThread = null;
    private Color  titleBg;
    private Color  titleHiBg;
    private Color  titleFg;
    private Color  titleHiFg;
    private BusyPanel xmlTitle;
    private DpsInfoPan dpsPanel;
    private AnnotationEditor annotation = null;
} // class ExpPanel

