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
import java.awt.print.*;
import java.beans.*;
import java.io.*;
import java.util.*;
import javax.swing.*;

import java.lang.reflect.*;
import java.awt.image.BufferedImage;
import javax.swing.Timer;
import javax.imageio.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.sms.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;
import vnmr.ui.VnmrCanvas.XMap;
import vnmr.print.VjPrintDef;
import vnmr.print.VjPrintPreview;
import vnmr.print.VjPrintEvent;
import vnmr.print.VjPrintEventListener;
import vnmr.print.VjPaperMedia;
import vnmr.print.VjPrintUtil;
import vnmr.print.VjPrintService;

/**
 * The container for experiment windows.
 */

public class ExpViewArea extends  ExpViewIF implements VnmrKey,
              PropertyChangeListener, Printable, VjPrintEventListener
{
    private int maxRows = 9;
    private int maxCols = 9;
    private int maxViews = 9;
    private boolean isRunning = false;
    private SessionShare sshare;
    private AppIF appIf;
    private ExpPanel expPanels[];
    private Thread expThread[];
    private ExpPanel  activeExp = null;
    private ExpPanel  topExp = null;
    private ExpPanel  topLeftExp = null;
    private boolean expVisible[];
    private boolean expSelected[];
    private boolean jmolVisible[];
    private CanvasIF canvasObj[];
    private JComponent canvasComp[];
    private SysToolPanel sysToolbar = null;
    private Vector<JComponent>  ctrlTabs = null;
    private int rows = 1;
    private int cols = 1;
    private int newCols = 1;
    private int newRows = 1;
    private int curCols = 1;
    private int curRows = 1;
    private int prefRows = 1;
    private int prefCols = 1;
    private int panW = 0;
    private int panH = 0;
    private int imgW = 0;
    private int imgH = 0;
    private int expNum = 1;
    private int expShownNum = 1;
    private int layer0 = JLayeredPane.DEFAULT_LAYER.intValue();
    private int layer1 = layer0 + 10;
    private int layer2 = layer0 + 20;
    private int layer3 = layer0 + 30;
    private int layer4 = layer0 + 40;
    private int activeWin = 0;
    private int prtWidth = 0; // image width on paper
    private int prtHeight = 0;
    private int paperWidth = 0;  // postcripts unit
    private int paperHeight = 0;
    private int paperTopMargin = 30;
    private int paperBottomMargin = 30;
    private int paperLeftMargin = 30;
    private int paperRightMargin = 30;
    private int prtCopies = 1;
    private int prtLw = 1;
    private int aipRows = 1;
    private int aipCols = 1;
    private int aipFrameWidth = 0;
    private int aipFrameHeight = 0;
    private int aipPrtId = 0;
    protected Class<?> m_vjmolClass = null;
    protected ArrayList<JComponent> m_aListJMol = new ArrayList<JComponent>();
    private boolean viewAll = true;
    private boolean inSmallLarge = false;
    private boolean logoutCalled = false;
    private boolean bOverlayed = false;
    private boolean bResize = false;
    private boolean bSyncMode = false;
    private boolean bSmsVisible = false;
    private boolean bWalkUp = true;
    private boolean bChangeCanvas = true;
    private boolean bXor = false;
    private boolean bXwindow = false;
    private boolean bShowMaxBut = true;
    private boolean bRebuild = false;
    private boolean bImgUser = false;  // imaging user
    private boolean bRunPrtScrnPopup = false;  // imaging user
    private boolean bPrtColor = true;
    private boolean bPlot = false;
    private boolean bPrintToFile = false;
    private boolean bPrtVnmrj = false;
    private boolean bFitPaper = false;
    private boolean bPrtPreview = true;
    private boolean inPrintMode = false;
    private boolean bTransparent = false;
    private boolean inPrintFrame = false;
    private boolean bJavaPrint = false;
    private boolean bMMMargin = false; // milimeter unit 
    private boolean m_align = false;
    private boolean m_stack = false;
    private boolean m_XY1D = false;
    private boolean bReady = false;
    private boolean bRecreateVp = false;
    private boolean bPurgeVp = false;
    private String  expShowStr = "expShow_";
    private String  prtFile = null;
    private String  prtRegion = null;
    private String  prtFormat = "jpg";
    private String  pltFormat = "jpg";
    private String  imgFormat = null;
    private String  prtSize = null;
    private String  prtPaper = "letter";
    private String  prtFrames = null;
    private String  prtDpiStr = "300";
    private String  prtOrient = null;
    private String  prtPlotter = null;
    private double m_spx = 0;
    private double m_spy = 0;
    private double m_epx = 0;
    private double m_epy = 0;
    private double m_sp2 = 0;
    private double m_sp1 = 0;
    private double m_ep2 = 0;
    private double m_ep1 = 0;
    private float prtDpi;
    private float prtFontH;
    private Font  prtFont;
    private Color  prtFgColor;
    private Color  prtBgColor;
    private Graphics2D prtGc;
    private int m_alignvp = 0;
    private int m_overlayMode = 0;
    private int m_prevOverlayMode = 0;
    private static int NOOVERLAY_NOALIGN = 0;
    private static int OVERLAY_NOALIGN = 1;
    private static int NOOVERLAY_ALIGN = 2;
    private static int OVERLAY_ALIGN = 3;
    private static int STACKED = 4;
    private static int UNSTACKED = 5;
    private static String viewAllStr = "viewAll";
    private Timer printTimer = null;
    private Robot  robot = null;
    private BufferedImage  plotImg = null;
    private BufferedImage  plotTransImg = null;
    private BufferedImage  vnmrjImg = null;
    private VjPrintService vjPrtService;
    private Dimension paperDim;
    private VButton trayButton;

    public SmsPanel smsPan = null;

    /**
     * constructor
     * @param sshare session share
     */
    public ExpViewArea(SessionShare sshare, AppIF ap)
    {
        this.sshare = sshare;
        this.appIf = ap;

        Dimension dim = (Dimension) sshare.getProperty("canvasnum");
        bImgUser = Util.isImagingUser();
        if (dim == null) {
            newRows = 1;
            newCols = 1;
        } else {
            newRows = dim.height;
            newCols = dim.width;
            prefRows = dim.height;
            prefCols = dim.width;
        }

        String tmpStr;
        Util.setViewArea(this);
        User user = Util.getUser();
        if (user != null) {
            ArrayList<?> appTypes = user.getAppTypes();
            if (appTypes != null) {
               for (int k = 0; k < appTypes.size(); k++) {
                   tmpStr = (String)appTypes.get(k);
                   if (Global.WALKUPIF.equals(tmpStr)
                       || "Walkup".equals(tmpStr)
                       || Global.LCIF.equals(tmpStr))
                   {
                       bWalkUp = true;
                       break;
                   }
               }
            }
        }

        createTrayButton();
        initArrays();
        Util.getViewportMenu();
        sysToolbar = (SysToolPanel) Util.getSysToolBar();
        Integer si=(Integer)sshare.getProperty("activeWin");
        if(si != null)
            activeWin = si.intValue();
        Boolean sb=(Boolean)sshare.getProperty(viewAllStr);
        if(sb != null)
            viewAll = sb.booleanValue();

/*
        String persona = System.getProperty("persona");
        if (persona != null && persona.equals("Rudy")) {
            maxRows = 1;
            maxCols = 1;
            maxViews = 1;
            viewAll = false;
        }
*/
        if (newCols > maxCols)
            newCols = maxCols;
        if (newRows > maxRows)
            newRows = maxRows;
        if (newCols < 1)
            newCols = 1;
        if (newRows < 1)
            newRows = 1;
        cols = newCols;
        rows = newRows;
        if (viewAll) {
            curCols = cols;
            curRows = rows;
        }
        else{
            curCols = 1;
            curRows = 1;
        }

        setOpaque(false);
        // setBackground(Color.black);
        setLayout(new PanelLayout());
        int k, m;
        m = rows * cols;
        si = (Integer)sshare.getProperty("expNum");
        if (si != null) {
            k = si.intValue();
            if (k >= 1 && k <= maxViews)
               m = k;
        }
        if (m > maxViews)
            m = maxViews;
        if(activeWin >= m)
            activeWin = 0;
        if (m <= 1) {
            viewAll = false;
            m = 1;
        }
        expNum = m;

        m = 0;
        for (k = 0; k < expNum; k++) {
            createExp(k);
            if (expSelected[k]) 
                m = k;
        }
        getShownNum();
        if (!expSelected[activeWin])
            activeWin = m;
        if (expShownNum < 1) {
            setCanvasAvailable(activeWin, true);
        }
        Util.setActiveView(expPanels[activeWin]);
        m = rows * cols;
        if ( m != expShownNum) {
            adjustRowColumn();
            cols = newCols;
            rows = newRows;
            if(viewAll){
                curCols = cols;
                curRows = rows;
            }
            else{
                curCols = 1;
                curRows = 1;
            }
        }
/*
        activeExp = expPanels[activeWin];
        setCanvasAvailable(activeWin, true);
        activeExp.setShowFlag(true);
        setExpVisible(activeWin, true);
        activeExp.setActive(true);
        expShownNum = expNum;
*/
        bXwindow = Util.isNativeGraphics();
        DisplayOptions.addChangeListener(this);
        /**
            if (Util.isNativeGraphics()) {
               vpManager = new VxRepaintManager();
               RepaintManager.setCurrentManager(vpManager);
               vpManager.setAppIf(ap);
            }
        **/
        bReady = true;

    } //ExpViewArea()

    public void rebuildUI(SessionShare sshare1, AppIF ap)
    {
        boolean oldImgUser = bImgUser;
        this.sshare = sshare1;
        appIf = ap;

        bRebuild = true;
        bImgUser = Util.isImagingUser();
        User user = Util.getUser();
        if (user != null) {
            ArrayList<?> appTypes = user.getAppTypes();
            if (appTypes != null) {
               for (int k = 0; k < appTypes.size(); k++) {
                   String tmpStr = (String)appTypes.get(k);
                   if (Global.WALKUPIF.equals(tmpStr)
                       || "Walkup".equals(tmpStr)
                       || Global.LCIF.equals(tmpStr))
                   {
                       bWalkUp = true;
                       break;
                   }
               }
            }
        }
        Util.getViewportMenu();
        sysToolbar = (SysToolPanel) Util.getSysToolBar();
        ExpPanel.clearStaticListeners();
        for (int n = 0; n < maxViews; n++) {
             if (expPanels[n] != null) {
                 expPanels[n].rebuildUI(sshare1, appIf);
                 expPanels[n].showTrayButton(bWalkUp);
             } 
        }
        
        if (oldImgUser != bImgUser) {
            if (bImgUser) {
                if (expNum > 3)
                    setCanvasNum(3);
            }
        }
    }

    public void run()
    {
        int k;

        if (!isRunning) {
            for (k = 0; k < maxViews; k++) {
               if (expThread[k] != null) {
                   if (!expThread[k].isAlive()) {
                     //  expThread[k].setName("ExpViewArea" + k);
                       expThread[k].start();
                   }
                }
            }
        }
        isRunning = true;
        if (bRebuild) {
            for (k = 0; k < maxViews; k++) {
               if (expPanels[k] != null) {
                  expPanels[k].resumeUI();
               }
           }
           if (expPanels[activeWin] != null)
               expPanels[activeWin].setActive(true);
           else
               setActiveCanvas(0);
           // ExpSelector.setForceUpdate(true);
           // ExpSelTree.setForceUpdate(true);
        }
        else {
           // ExpSelector.setForceUpdate(false);
           // ExpSelTree.setForceUpdate(false);
        }
    }

    public void propertyChange(PropertyChangeEvent e)
    {
        JComponent vjmol = expPanels[activeWin].getVJMol();
        if (vjmol != null)
        {
            String strProperty = e.getPropertyName();
            if (strProperty == null)
                return;

            strProperty = strProperty.toLowerCase();
            if (strProperty.indexOf("vjbackground") >= 0 ||
                strProperty.indexOf("plaintext") >= 0)
                setvjmolPref();
        }
    }

    public VButton getTrayButton() {
        return trayButton;
    }

    private void createTrayButton() {
         
        trayButton = new VButton(sshare, null, "button");
        trayButton.setAttribute(VObjDef.ICON, "sampleTrayOpen.gif");
        // trayButton.setBackground(Util.getBgColor());
        trayButton.setToolTipText(Util.getTooltipString("Open tray panel"));
        trayButton.setOpaque(true);
        // trayButton.setBorderAlways(true);
        trayButton.setVisible(false);
        trayButton.setEnabled(false);
        trayButton.setBounds(0, 0, 16, 16);
        trayButton.setVerticalAlignment(SwingConstants.CENTER);
        add(trayButton, 0);
        setLayer(trayButton, layer3);
        trayButton.setActionCommand("Open");
        trayButton.addActionListener(new ActionListener() {
              public void actionPerformed(ActionEvent evt) {
                     if (activeExp != null)
                         activeExp.processTrayCmd("showButton");
                     else
                         showSmsPanel(true, false);
              }
        });
    }


    private void initArrays() {
        String s = System.getProperty("viewports");
        int n = 9;
        Boolean sb;
        if (s != null) {
            try
            {
                n = Integer.parseInt(s);
            }
            catch (Exception e) { n = 9; }
        }
        if (n > 0 && n < 20)
            maxViews = n;
        maxRows = maxViews;
        maxCols = maxViews;
        expPanels = new ExpPanel[maxViews];
        expThread = new Thread[maxViews];
        expVisible = new boolean[maxViews];
        expSelected = new boolean[maxViews];
        jmolVisible = new boolean[maxViews];
        canvasObj = new CanvasIF[maxViews];
        canvasComp = new JComponent[maxViews];
        for (n = 0; n < maxViews; n++) {
            expPanels[n] = null;
            expThread[n] = null;
            canvasComp[n] = null;
            canvasObj[n] = null;
            expVisible[n] = true;
            expSelected[n] = true;
            jmolVisible[n] = false;
            if (!bImgUser) {
               s = expShowStr+n;
               sb = (Boolean)sshare.getProperty(s);
               if (sb != null)
                   expSelected[n] = sb.booleanValue();
            }
        }
    }

    private void createExp(int k) {
         if (expPanels[k] != null) {
            if ((expThread[k] != null) && expThread[k].isAlive())
            {
                if (expPanels[k].isAlive())
                   return;
            }
            remove(expPanels[k]);
            expPanels[k].quit();
            expPanels[k] = null;
         }
         if ((expThread[k] != null) && expThread[k].isAlive()) {
            try {
                    expThread[k].join();
            }
            catch(InterruptedException e) { }
            expThread[k] = null;
         }

         ExpPanel exp = new ExpPanel(sshare, this, appIf, k);
         expPanels[k] = exp;
         expThread[k] = new Thread(exp);
         expThread[k].setName("ExpViewArea" + k);
         add(exp);
         exp.setActive(false);
         exp.showTrayButton(bWalkUp);
          //exp.showVJmolButton(true);
         if (curCols > 1)
              exp.setFullWidth(false);
         else
              exp.setFullWidth(true);
         if (viewAll && k < expNum) {
              setExpVisible(k, true);
         }
         else {
              exp.setShowFlag(false);
              setExpVisible(k, false);
         }
         exp.setMenuAttr(bShowMaxBut);
         if (isRunning)
              expThread[k].start();
    }

    public SmsPanel getSmsPanel() {
        return smsPan;
    }

    public boolean isCsiButtonVisible() {
        boolean bCsi = false;
        for (int n = 0; n < maxViews; n++) {
             if (expPanels[n] != null) {
                 if (expPanels[n].isCsiPanelVisible()) {
                     bCsi = true;
                     break;
                 }
             } 
        }
        return bCsi;
    }

    public void showCsiButtonPalette(boolean bShowCsi) {
        if (appIf == null)
            return;
        if (bShowCsi) {
            appIf.showCsiButtonPalette(bShowCsi);
            return;
        }
       
        boolean bCsi = isCsiButtonVisible();
        appIf.showCsiButtonPalette(bCsi);
    }

    public void showSmsPanel(boolean bShowSms, boolean bDebug) {
        int n;
        boolean bChangeSms = false;
        boolean bButtonVisible = false;

        bSmsVisible = false;
        if (smsPan != null)
            bSmsVisible = smsPan.isVisible();
        if (bSmsVisible != bShowSms)
            bChangeSms = true;
        bButtonVisible = trayButton.isVisible();
        bSmsVisible = bShowSms;
        if (bShowSms) {
            for (n = 0; n < maxViews; n++) {
                if (expPanels[n] != null) {
                    if (bXwindow)
                        expPanels[n].sendToVnmr("M@xstop\n");
                    expPanels[n].setVisible(false);
                }
            }
            if (smsPan == null) {
                smsPan = new SmsPanel(sshare, bDebug);
                add(smsPan);
                setLayer(smsPan, layer4);
            }
            else
                smsPan.setDebugMode(bDebug);
            trayButton.setVisible(false);
            smsPan.setVisible(true);
        }
        else {
            if (smsPan != null)
                smsPan.setVisible(false);
            for (n = 0; n < expNum; n++) {
                expPanels[n].setVisible(expVisible[n]);
            }
            if (trayButton.isEnabled()) {
                trayButton.setVisible(true);
            }
            else {
                trayButton.setVisible(false);
            }
            sendToVnmr("vnmrjcmd('tray repaint')");
        }
        if (bChangeSms)
            revalidate();
        else {
            if (bButtonVisible != trayButton.isVisible()) {
               if (topLeftExp != null)
                   topLeftExp.revalidate();
               else
                   revalidate();
            }
        }
    }

    public void setTraybuttonEnabled(boolean b) {
        trayButton.setEnabled(b);
        if (b) {
            showSmsPanel(bSmsVisible, false);
        }
        else {
            showSmsPanel(false, false);
        }
    }

    public void suspendUI() {
         for (int n = 0; n < maxViews; n++) {
             if (expPanels[n] != null) {
                 expPanels[n].closeOutput();
             } 
         }
    }

    public void resumeUI() {
         for (int n = 0; n < maxViews; n++) {
             if (expPanels[n] != null) {
                 expPanels[n].openOutput();
             } 
         }
         
    }

    public static final String[] imageFormats = {"bmp", "gif", "jpg", "jpeg",
                  "pcl", "pdf", "png", "ppm", "ps", "tif", "tiff"};
    public static final String[] javaFormats = {"gif", "jpg", "jpeg", "png" };

    private int image_no = 1;

    private void showImage(String strPath) {
        String fpath = FileUtil.openPath(strPath);
        if (fpath == null) {
             Messages.postError("Could not open file: "+strPath);
             return;
        }
        StringTokenizer tok = new StringTokenizer(strPath, ".");
        String format = "";
        int i;
        boolean bSupported = false;
        while (tok.hasMoreTokens())
             format = tok.nextToken().trim();
        for (i = 0; i < imageFormats.length; i++) {
            if (format.equals(imageFormats[i])) {
                bSupported = true;
                break;
            }
        }
        if (!bSupported) {
             Messages.postError("Could not display "+strPath);
             return;
        }
        bSupported = false;
        for (i = 0; i < javaFormats.length; i++) {
            if (format.equals(javaFormats[i])) {
                bSupported = true;
                break;
            }
        }
        if (!bSupported) {
           String user = System.getProperty("user.name");
           String tmpName = File.separator+"plotpreviews"+File.separator;
           if (user != null)
                tmpName = tmpName+user+"_tmpplot_"+Integer.toString(image_no)+".gif";
           else
                tmpName = tmpName+"tmpplot_"+Integer.toString(image_no)+".gif";
           image_no++;
           if (image_no > 22)
                image_no = 1;
           String tmpFile = FileUtil.savePath("PERSISTENCE" +tmpName);
           String[] cmds = {
               WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
               FileUtil.SYS_VNMR + "/bin/convert "
               +" -transparent black "
               + " \"" + fpath + "\" " + "GIF:\""
               + tmpFile + "\"" };
           WUtil.runScript(cmds);
           fpath = FileUtil.openPath(tmpFile);
           if (fpath == null) {
               Messages.postError("Could not convert "+strPath+" to GIF.");
               return;
           }
        }
        sendToVnmr("imagefile('display', '"+fpath+"')");
    }

    public void showJMolPanel(boolean bShow)
    {
        showJMolPanel(activeWin, bShow);
    }

    public void showJMolPanel(int n, boolean bShow)
    {
        if (!Util.isVJmol())
            return;

        if (expPanels[n] == null)
            return;

        JComponent vjmol = expPanels[n].getVJMol();
        int th = expPanels[n].getTitleBarHeight();
        try
        {
            String strPath = FileUtil.openPath("MOLLIB/"+getMol(n));
            jmolVisible[n] = bShow;
            if (bShow) {
                if (vjmol == null)
                {
                    vjmol = createVJMolPanel(strPath, n);
                    if (vjmol == null) {
                        jmolVisible[n] = false;
                        return;
                    }
                }

                if (!m_aListJMol.contains(vjmol)) {
                    add(vjmol);
                    setLayer(vjmol, layer3);
                    m_aListJMol.add(vjmol);
                    Rectangle r = expPanels[n].getBounds();
                    r.y = r.y + th;
                    vjmol.setBounds(r);
                }
                Class<?> argClass[] = {boolean.class, String.class};
                Object[] args = {Boolean.valueOf(bShow), strPath};
                vjmolMethod(vjmol, "showMol", argClass, args);
            }
            else {
                expPanels[n].saveVJmolImage(false, true);
                expPanels[n].saveVJmolImage(true, false);
            }
            if (vjmol != null)
                vjmol.setVisible(bShow);
            setExpVisible(n, expVisible[n]);
            expPanels[n].validate();
        }
        catch (Exception e)
        {
            jmolVisible[n] = false;
            Messages.writeStackTrace(e);
        }
    }

    protected JComponent createVJMolPanel(String strPath, int n)
    {
        JComponent vjmol = null;
        if (expPanels[n] == null)
            return vjmol;
        vjmol = expPanels[n].getVJMol();
        if (vjmol != null)
            return vjmol;
        
        try
        {
            String strfile = FileUtil.openPath(FileUtil.SYS_VNMR+"/java/jmol.jar");
            if (strfile == null)
            {
                Messages.postError("Jmol is not installed. Please install the "+
                                   "Jmol package and try again.");
                return vjmol;
            }

            m_vjmolClass = Class.forName("vjmol.VJMol");
            if (m_vjmolClass == null)
                return vjmol;
            Class<?>[] argClass = {String.class, String.class, String.class};
            Object[] args = {strPath, FileUtil.savePath("PERSISTENCE/mol"),
                             FileUtil.savePath("MOLLIB/icons")};
            Constructor<?> constructor = m_vjmolClass.getConstructor(argClass);
            vjmol = (JComponent)constructor.newInstance(args);

            if (vjmol != null)
            {
                expPanels[n].setVJMol(vjmol);
                setvjmolPref();

                JButton btnVJmol = (JButton)vjmolMethod(vjmol, "getVJmolButton",
                                                            null, null);
                btnVJmol.setVisible(true);
                btnVJmol.setName(Integer.toString(n));
                btnVJmol.setToolTipText(Util.getLabel("Close JMol Editor"));
                btnVJmol.addActionListener(new ActionListener()
                {
                    public void actionPerformed(ActionEvent e)
                    {
                        // appIf.showJMolPanel(false);
                        Component src = (Component)e.getSource();
                        String idStr = src.getName();
                        int id = -1;
	                try {
	                    id = Integer.parseInt(idStr);
	                }
	                catch (NumberFormatException er) { id = -1; }
                        if (id >= 0) 
                            showJMolPanel(id, false);
                        else
                            showJMolPanel(false);
                    }
                });
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }

        return vjmol;
    }

    /**
     * This method calls the methods in vjmol.
     * @param vjmol   the vjmol component.
     * @param strMethod  the method name to be called.
     * @param argClass   the argument class array for the method.
     * @param args       the arguments for the method.
     * @return    the object that's returned by the method.
     */
    protected Object vjmolMethod(JComponent vjmol, String strMethod,
                                 Class<?>[] argClass, Object[] args)
    {
        Object obj = null;
        try
        {
            Method method = m_vjmolClass.getMethod(strMethod, argClass);
            obj = method.invoke(vjmol, args);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
        return obj;
    }

    /**
     * Sets the preferences for vjmol.
     */
    protected void setvjmolPref()
    {
        JComponent vjmol = expPanels[activeWin].getVJMol();
        if (vjmol != null && m_vjmolClass != null)
        {
            Font font = DisplayOptions.getFont("PlainText");
            Color color = DisplayOptions.getColor("PlainText");
            Color bgColor = Util.getBgColor();
            String strColor = (String)sshare.getProperty("molbackground");
            // Color bgColor2 = DisplayOptions.getColor(strColor);
            Color bgColor2 = Color.black;
            strColor = (String)sshare.getProperty("molforeground");
            Color fgColor = null;
            if (strColor != null && !strColor.equals("default"))
                fgColor = DisplayOptions.getColor(strColor);
            Class<?>[] argClass = {JComponent.class, Font.class, Color.class,
                                Color.class, Color.class, Color.class};
            Object[] args = {vjmol, font, color, fgColor, bgColor, bgColor2};
            vjmolMethod(vjmol, "setPref", argClass, args);
        }
    }

    protected void showMol(String strPath, JComponent vjmol)
    {
        strPath = FileUtil.openPath(strPath);
        if (strPath == null || strPath.equals(""))
            return;
        if (!strPath.endsWith(".mol")) {
            showImage(strPath);
            return;
        }

        if (!Util.isVJmol() || strPath == null || strPath.equals(""))
            return;

        if (vjmol == null) {
            vjmol = createVJMolPanel(strPath, activeWin);
        }

        if (vjmol != null)
        {
            Messages.postInfo("Loading file: " + strPath);
            Class<?>[] argClass = {String.class};
            Object[] args = {strPath};
            String strMsg = (String)vjmolMethod(vjmol, "showMol", argClass, args);
            if (strMsg == null)
            {
                expPanels[activeWin].saveVJmolImage(false, true);
                expPanels[activeWin].saveVJmolImage(true, false);
            }
            else
                Messages.postError("Error loading file: " + strPath);
        }
    }

    protected String saveImage(int n, String strPath, boolean bPrint, boolean bGif, boolean bShow)
    {
        JComponent vjmol = expPanels[n].getVJMol();
        if (vjmol != null)
        {
            Class<?>[] argClass = {String.class, boolean.class, boolean.class};
            Object[] args = {strPath, Boolean.valueOf(bPrint), Boolean.valueOf(bGif)};
            strPath = (String)vjmolMethod(vjmol, "saveImage", argClass, args);
        }
        return strPath;
    }

    protected String getMol(int id)
    {
        String strMol = "";
        CanvasIF canvas = expPanels[id].getCanvas();
        if (canvas == null)
            return strMol;
        VIcon icon = canvas.getIcon();
        if (icon == null || !icon.isSelected())
            return strMol;

        String strFile = icon.getFile();
        if (strFile != null && !strFile.equals(""))
        {
            int nIndex = strFile.lastIndexOf(File.separator);
            if (nIndex >= 0 && nIndex+1 < strFile.length())
            {
                int nIndex2 = strFile.lastIndexOf(".");
                if (nIndex2 < 0 || nIndex2 > strFile.length())
                    nIndex2 = strFile.length()-1;
                String strType = strFile.substring(nIndex2+1);
                if (strType.equals("display") || strType.equals("plot"))
                    strMol = strFile.substring(nIndex+1, nIndex2);
                else
                    strMol = strFile.substring(nIndex+1);
            }
        }
        return strMol;
    }

    private boolean showExpVjMol(int n, boolean b) {
        if (expPanels[n] == null)
            return false;
        if (!expPanels[n].isAlive())
            b = false;
        JComponent vjmol = expPanels[n].getVJMol();
        if (vjmol == null)
            return false;
        vjmol.setVisible(b);
        return b;
    }

    private void setExpVisible(int n, boolean b) {
        if (n >= expNum)
            b = false;
        expVisible[n] = b;
        if (expPanels[n] == null)
           return;
        if (!expSelected[n])
            b = false;
        if (b)
        {
            if (!bSmsVisible && expSelected[n]) {
                boolean bShowJmol = false;
                VnmrCanvas canvas = expPanels[n].getCanvas();
                if (jmolVisible[n])
                    bShowJmol = showExpVjMol(n, jmolVisible[n]);
                if (canvas != null) {
                    if (!bShowJmol)
                        canvas.setVisible(b);
                    else
                        canvas.setVisible(false);
                }
                expPanels[n].setVisible(b);
            }
        }
        else {
            showExpVjMol(n, b);
            expPanels[n].setVisible(b);
        }
    }

    private int openActiveExp() {
        int k;
        activeExp = null;
        for (k = 0; k < expNum; k++) {
            if (expPanels[k] != null && expThread[k] != null) {
                if (expSelected[k] && expPanels[k].isAlive()) {
                    activeExp = expPanels[k];
                    setExpVisible(k, true);
                    break;
                }
            }
        }

/*
        if (activeExp == null) {
            for (k = 0; k < expNum; k++) {
                if (expPanels[k] != null && expThread[k] != null) {
                    activeExp = expPanels[k];
                    setCanvasAvailable(k, true);
                    setExpVisible(k, true);
                    expSelected[k] = true;
                    break;
                }
            }
        }
*/
        if (activeExp != null) {
            activeExp.sendSetActive();
            return activeExp.getViewId();
        }
        return (-1);
    }

    public int getMaxCanvasNum() {
        return maxViews;
    }

    public int getCanvasNum() {
        return expNum;
    }

    public int getShownNum() {
        int a;
        expShownNum = 0;
        for (a = 0; a < expNum; a++) {
           if (expPanels[a] != null && expSelected[a])
                expShownNum++;
        }
        bShowMaxBut = false;
        if (!bOverlayed && expShownNum > 1)
            bShowMaxBut = true;
        for (a = 0; a < maxViews; a++) {
            if (expPanels[a] != null)
                expPanels[a].setMenuAttr(bShowMaxBut);
        }
        return expShownNum;
    }

    public void expReady(int num) {
        if (expPanels[num] != null && num == activeWin) {
           if (!expPanels[num].isInActive())
              expPanels[num].requestActive();
        }
    }

    private void adjustRowColumn() {
        getShownNum();
        bChangeCanvas = false;
        if (prefRows * prefCols == expShownNum) {
            newRows = prefRows;
            newCols = prefCols;
            return;
        }
	if (prefRows == 1) {
            newRows = 1;
            newCols = expShownNum;
            return;
        }
   	if (prefCols == 1) {
            newRows = expShownNum;
            newCols = 1;
            return;
	}
	if (expShownNum < 4 || expShownNum == 5 || expShownNum == 7) {
            newRows = 1;
            newCols = expShownNum;
            return;
        }
        if (expShownNum < 9) {
            if (expShownNum % 2 == 0) {
                newRows = 2;
                newCols = expShownNum / 2;
                return;
            }
        }
        if (expShownNum % 3 == 0) {
            newRows = 3;
            newCols = expShownNum / 3;
            return;
        }
        newRows = 4;
        newCols = expShownNum / 4;
        if (expShownNum % 4 != 0)
            newCols = newCols + 1;
    }

    private boolean isOverlayed() {
        for (int k = 0; k < expNum; k++) {
           ExpPanel exp = expPanels[k];
           if (exp != null) {
               if (getLayer(exp) > layer0)
                   return true;
           }
        }
        return false;
    }

    private void resetExpLayer(boolean bViewAll) {
        for (int k = 0; k < maxViews; k++) {
           ExpPanel exp = expPanels[k];
           if (exp != null) {
               setLayer(exp, layer0);
               exp.setOpaque(true);
               exp.setMenuAttr(bShowMaxBut);
               exp.setOverlay(false, false, false, null);
              /***
               if (bViewAll)
                   setExpVisible(k, expSelected[k]);
               else {
                  if (exp != activeExp)
                      setExpVisible(k, false);
               }
              ***/
           }
        }
    }

    public void setCanvasNum(int num) {
        int n;
        boolean bNumChanged = false;
        ExpPanel exp;

        if (num <= 0)
            return;
        if (num > maxViews)
            num = maxViews;

        if (expNum != num) {
            bChangeCanvas = true;
            bNumChanged = true;
            /***
            if (expPanels[activeWin] != null) {
               String str = "jFunc("+VFLUSH+", 0)\n";
               expPanels[activeWin].sendToVnmr(str);
            }
            ****/
        }
        expNum = num;
        sshare.putProperty("expNum",new Integer(expNum));

        // if (activeWin >= num)
        //     activeWin = 0;
        if (bPurgeVp) {
            for (n = num; n < maxViews; n++) {
                exp = expPanels[n];
                expPanels[n] = null;
                expVisible[n] = false;
                // expSelected[n] = false;
                canvasComp[n] = null;
                canvasObj[n] = null;

                if (exp != null) {
                    remove(exp);
                    exp.quit();
                }
            }
            for (n = num; n < maxViews; n++) {
                if (expThread[n] != null && expThread[n].isAlive()) {
                    try {
                        expThread[n].join();
                    }
                    catch(InterruptedException e) { }
                    expThread[n] = null;
                }
            }
        }
        for (n = 0; n < num; n++) {
            if (expPanels[n] == null) {
                bChangeCanvas = true;
                createExp(n);
            }
            if (!expPanels[n].isAlive()) {
                if (bRecreateVp) {
                    bChangeCanvas = true;
                    createExp(n);
                }
            }
        }
        for (n = num; n < maxViews; n++)
            setExpVisible(n, false);

        if (sysToolbar != null)
            sysToolbar.setVpNum(num);

	if(m_overlayMode < 1 || bNumChanged) {
           if (bOverlayed || isOverlayed()) {
               bChangeCanvas = true;
               resetExpLayer(true);
               viewAll = true;
           }
           if (bNumChanged)
               viewAll = true;
           bOverlayed = false;
        }
        if (viewAll) {
            for (n = 0; n < num; n++)
                setExpVisible(n, expSelected[n]);
        }
        getShownNum();
        if (expShownNum < 1)
            return;
/**
        if (bOverlayed) {
            return;
        }
        if (expPanels[activeId] == null || (!expPanels[activeId].isAlive())) {
            if (bChangeCanvas)
                activeId = openActiveExp();
        }
        if (activeId < 0) {  // no ExpPanel, something wrong
            createExp(0);
            expPanels[0].setTabPanels(ctrlTabs);
            activeId = 0;
        }
        if (activeId >= 0)
            activeWin = activeId;
        for (n = 0; n < expNum; n++) {
            if (expPanels[n] == null) {
                bChangeCanvas = true;
                expPanels[activeId].flushVnmr(); // vnmrbg save global
                return;
            }
            if (!expPanels[n].isAlive()) {
                if (bRecreateVp) {
                    bChangeCanvas = true;
                    expPanels[activeId].flushVnmr(); // vnmrbg save global
                    return;
                }
            }
        }
*/
        if (bChangeCanvas) {
            adjustRowColumn(); 
            initCanvasArray();
        } 
/*
        if (bNumChanged) {
	    expPanels[activeId].sendToVnmr("vpLayout('init', 'auto',"+num+")");
	    expPanels[activeId].sendToVnmr("vpLayout('auto')");
        }
        else if (bChangeCanvas)
            initCanvasArray();
*/
    }

    public void setCanvasNum(int num, String opt, String opt2) {
        bPurgeVp = false;
        bRecreateVp = false;
        if (opt != null) {
            if (opt.equalsIgnoreCase("clean") || opt.equalsIgnoreCase("purge"))
               bPurgeVp = true;
            if (opt.equalsIgnoreCase("recover"))
               bRecreateVp = true;
        }
        if (opt2 != null) {
            if (opt2.equalsIgnoreCase("clean") || opt2.equalsIgnoreCase("purge"))
               bPurgeVp = true;
            if (opt2.equalsIgnoreCase("recover"))
               bRecreateVp = true;
        }
        setCanvasNum(num);
    }

    public void setCanvasNum(int num, String opt) {
        setCanvasNum(num, opt, null);
    }

    public void windowClosing() {
        if ( (expPanels[activeWin] != null) && expPanels[activeWin].outPortReady()) {
           expPanels[activeWin].sendToVnmr("exit");
           return;
        }
        VNMRFrame vframe = VNMRFrame.getVNMRFrame();
        if (vframe != null)
           vframe.exitAll();
        else
           System.exit(0);
    }

    public void canvasExit(int id)
    {
        if (logoutCalled) {
            return;
	}
        if (expPanels[id] != null) {
           /*+prefRows+"  "+prefCols***********
            canvasComp[id] = null;
            canvasObj[id] = null;
            remove(expPanels[id]);
            setExpVisible(id, false);
            expPanels[id] = null;
            if ((expThread[id] != null) && expThread[id].isAlive()) {
                try {
                    expThread[id].join();
                }
                catch(InterruptedException e) {
                }
            }
            expThread[id] = null;
           **************/
        }
        int num = 0;
        for (int n = 0; n < maxViews; n++) {
            if (expPanels[n] != null && expPanels[n].isAlive())
                num++;
        }
        if (num <= 0) {
/*
            VNMRFrame vframe = VNMRFrame.getVNMRFrame();
            if (vframe != null) {
                vframe.exitAll();
                return;
            }
*/
        }
        if(DebugOutput.isSetFor("traceXML"))  // debug mode
        {
            if (!bRecreateVp)
                return;
        }

        if (bRecreateVp)
            setCanvasNum(expNum);

        if (activeWin == id) {
            bChangeCanvas = true;
            openActiveExp();
        }
    }

    public void setCanvasArray(int r, int c)
    {
        int k;
        if (c < 1 || r < 1)
            return;
        prefRows = r;
        prefCols = c;
        if ((r == curRows) && (c == curCols)) {
            if (!bChangeCanvas)
               return;
        }
        bChangeCanvas = false;
        getShownNum();
        if (expShownNum < 1)
            return;
 
        newCols = c;
        if (newCols > maxCols)
            newCols = maxCols;
        newRows = r;
        if (newRows > maxRows)
            newRows = maxRows;
        if (bOverlayed) {
            resetExpLayer(true);
            bOverlayed = false;
        }
        viewAll = true;
        if (newRows * newCols != expShownNum) {
            adjustRowColumn(); 
        }
/*
        sshare.putProperty("canvasnum",new Dimension(newCols, newRows));
	VTabbedToolPanel tp = Util.getVTabbedToolPanel();
        if(tp != null) tp.updateVpInfo(newCols*newRows);
        int n = newRows * newCols;
*/
        for (k = 0; k < expNum; k++) {
            if (expPanels[k] == null) {
                // send FLUSH command to every Vnmrbg and 
                // call initCanvasArray later
                if (expPanels[activeWin] != null) {
                    expPanels[activeWin].flushVnmr();
                    return;
                }
            }
        }
        initCanvasArray();
    }

    public void initCanvasArray()
    {
        int r, k, w;
        int canvasNum = 0;
        // int totalNum = 0;
        int requestNum = 0;
        boolean bNewCanvas = false;
        boolean bOldView = viewAll;

        for (r = 0; r < expNum; r++) {
            if (expPanels[r] == null) {
                createExp(r);
                expPanels[r].setTabPanels(ctrlTabs);
                bNewCanvas = true;
            }
        }
        if (bNewCanvas) {
            bChangeCanvas = true;
	    // sendToVnmr("vpLayout('auto')");
            adjustRowColumn(); 
            // return;
        }
        w = -1;
        for (r = 0; r < expNum; r++) {
            if (expPanels[r] != null) {
                // totalNum++;
                if (expSelected[r]) {
                   if (expPanels[r].isInActive())  // active canvas
                      w = r;
                   else if (w < 0)
                      w = r;
                   canvasNum++;
                }
            }
        }
        if (canvasNum < 1)
            return;

        sshare.putProperty("canvasnum",new Dimension(newCols, newRows));
        cols = newCols;
        rows = newRows;
        requestNum = newRows * newCols;
        curCols = cols;
        curRows = rows;
        if (canvasNum == 1 || requestNum > 1) {
            if (bOverlayed)
                overlayCanvas(false, false);
        }
        if (!viewAll) {
            curCols = 1;
            curRows = 1;
        }
/*
        viewAll= false;
        if (requestNum > 1 && expShownNum > 1)
            viewAll= true;
*/
        
        canvasNum = 0;
        for (k = 0; k < maxViews; k++) {
            if (expPanels[k] == null)
                continue;
            if (!expSelected[k] || canvasNum >= requestNum) {
                setExpVisible(k, false);
                continue;
            }
            // expPanels[k].setMenuAttr(viewAll);
            if (viewAll) {
                expPanels[k].setFullWidth(false);
                setExpVisible(k, true);
            }
            else {
                expPanels[k].setFullWidth(true);
                setExpVisible(k, false);
            }
            if (activeWin == k)
                 w = activeWin;
            else if (w < 0)
                 w = k;
            canvasNum++;
        }
        if (w < 0)
            return;

/*
        if (totalNum > expNum) {
            VTabbedToolPanel tp = Util.getVTabbedToolPanel();
            if(tp != null) tp.updateVpInfo(totalNum);
        }
*/

        setExpVisible(w, true);
        expPanels[w].requestActive();
        revalidate();
        if (bOldView != viewAll)
            expPanels[w].repaint();
    }

    public void setActiveCanvas(int id)
    {
        int k;
        int oldActiveWin = activeWin;

        if (id < 0 || id >= expNum)
            return; 
        if (appIf.inVpAction()) {
            if (appIf.getVpId() != id)
                return;
        }

        if (!expSelected[id])
            return;
        if (expPanels[id] == null || expThread[id] == null)
        {
            // createExp(id);
            return;
        }
        if (!expPanels[id].isAlive())
            return;

	// VTabbedToolPanel tp = Util.getVTabbedToolPanel();
        if (oldActiveWin == id) {
            if (expPanels[id].isInActive()) {
               // VTabbedToolPanel will be handled by VnmrjUI
               // if(tp != null) tp.switchLayout(id);
                return;
            }
        }

        activeWin = id;

        getShownNum();
        if (bChangeCanvas) {
            if (expShownNum < 1)
                return;
/*
            k = curCols * curRows;
            if (!bOverlayed && (expShownNum != k)) {
                if (expShownNum < 4) {
                   newCols = expShownNum;
                   newRows = 1;
                }
                else {
                   if (expShownNum % 2 == 0) {
                       newRows = 2;
                       newCols = expShownNum / 2;
                   }
                   else if (expShownNum % 3 == 0) {
                       newRows = 3;
                       newCols = expShownNum / 3;
                   }
                }
                initCanvasArray();
            }
*/
        }

        if(!viewAll && bXwindow) {
            if (expPanels[oldActiveWin] != null) {
                   expPanels[oldActiveWin].sendToVnmr("M@xstop\n");
            }
        }

        for (k = 0; k < maxViews; k++) {
            if (k != id) {
                if (expPanels[k] != null)
                    expPanels[k].setActive(false);
            }
        }
        activeExp = expPanels[activeWin];
        setExpVisible(activeWin, true);
        if (!viewAll || bOverlayed) {
            if (activeWin != oldActiveWin)
            {
                 if (!bOverlayed)
                     setExpVisible(oldActiveWin, false);
            }
            Dimension d = expPanels[activeWin].getSize();
            if (d.width != panW || d.height != panH)
                 bChangeCanvas = true;
/*
            if (!appIf.inVpAction())
                resizeCanvas();
            else {
                Dimension d = expPanels[activeWin].getSize();
                if (d.width < 10 || d.height < 10)
                    resizeCanvas();
            }
*/
        }
        activeExp.setActive(true);
        sshare.putProperty("activeWin",new Integer(activeWin));
        // if(tp != null) tp.switchLayout(id);
        if (bOverlayed) {
            bChangeCanvas = false;
            overlayCanvas(true, true);
        }
        if (bChangeCanvas) {
            bChangeCanvas = false;
            resizeCanvas();
        }
    }

    public void setActiveVp(int id) {
        setActiveCanvas(id);
    }

    public VpLayoutInfo getLayoutInfo(int id) {
        if (id < 0 || id >= maxViews)
            return null;
        if (expPanels[id] != null)
            return  expPanels[id].getVpLayoutInfo();
        else
            return null;
    }

    public VpLayoutInfo getLayoutInfo() {
        if (expPanels[activeWin] != null && expPanels[activeWin].isShowing())
            return getLayoutInfo(activeWin);
        else
            return null;
    }

    public ExpPanel getActiveVp() {
        return  expPanels[activeWin];
    }

    public int getActiveWindow() {
        return activeWin;
    }

    public void setCanvasComp(int n, JComponent comp) {
        if (n < maxViews) {
            canvasComp[n] = comp;
            canvasObj[n] = (CanvasIF) comp;
        }
    }

    public CanvasIF getActiveCanvas() {
        return canvasObj[activeWin];
    }

    public void setSuspend(boolean s) {
        for (int k = 0; k < maxViews; k++) {
                if (canvasObj[k] != null) {
                    canvasObj[k].setSuspend(s);
                }
        }
    }

    public void setResizeStatus(boolean s) {
        int k;

        bResize = s;
        if (!bXwindow)
            return;
        if (s) {
            String mess = "M@xresize";
            sendToAllVnmr(mess);
            for (k = 0; k < maxViews; k++) {
                if (expPanels[k] != null && canvasObj[k] != null) {
                    canvasObj[k].setSuspend(true);
                }
            }
        }
        else {
            EventQueue q = Toolkit.getDefaultToolkit().getSystemEventQueue();
            if (q == null)
                return;
            for (k = 0; k < maxViews; k++) {
                if (expPanels[k] != null && canvasComp[k] != null) {
                     ComponentEvent e = new ComponentEvent((JComponent) canvasComp[k],
                                ComponentEvent.COMPONENT_MOVED);
                     q.postEvent(e);
                }
            }
        }
    }

    public boolean isResizing() {
        return bResize;
    }

    public void setResizeStatus(int n, boolean s) {
        bResize = s;
        if (!bXwindow)
            return;
        if (expPanels[n] != null) {
            if (s) {
                String mess = "M@xresize\n";
                expPanels[n].sendToVnmr(mess);
            }
            else {
                EventQueue q = Toolkit.getDefaultToolkit().getSystemEventQueue();
                if (q != null) {
                    if (canvasComp[n] != null) {
                        ComponentEvent e = new ComponentEvent(canvasComp[n],
                                ComponentEvent.COMPONENT_MOVED);
                        q.postEvent(e);
                    }
                }
            }
        }
    }

    public void sendToVnmr(String str) {
        if (expPanels[activeWin] != null)
           expPanels[activeWin].sendToVnmr(str);
    }

    public void sendToVnmr(int vp, String str)
    {
        int id = vp - 1;
        if ((id >= 0) && (id < maxViews)) {
            if (expPanels[id] != null)
                expPanels[id].sendToVnmr(str);
        }
    }

    public void sendToAllVnmr(String str)
    {
        for (int k = 0; k < maxViews; k++) {
            if (expPanels[k] != null)
                expPanels[k].sendToVnmr(str);
        }
    }

    public void sendToAllVnmr(String str, int originator)
    {
        for (int k = 0; k < maxViews; k++) {
            if ((k != originator) && (expPanels[k] != null))
                expPanels[k].sendToVnmr(str);
        }
    }

    public void dbAction(String action, String type)
    {
        for (int k = 0; k < maxViews; k++) {
            if (expPanels[k] != null)
                expPanels[k].dbAction(action, type);
        }
    }

    public void setGraphicRegion()
    {
        for (int k = 0; k < maxViews; k++) {
            if (expPanels[k] != null)
                expPanels[k].setGraphicRegion();
        }
    }


    public void refresh()
    {
        for (int k = 0; k < maxViews; k++) {
            if (expPanels[k] != null)
                expPanels[k].refresh();
        }
    }

    public void setTabPanels(Vector<JComponent> v)
    {
        ctrlTabs = v;
        for (int k = 0; k < maxViews; k++) {
            if (expPanels[k] != null)
                expPanels[k].setTabPanels(v);
        }
    }

    public ParamIF syncQueryParam(String p)
    {
        return expPanels[activeWin].syncQueryParam(p);
    }

    public ParamIF syncQueryVnmr(int type, String p)
    {
        return expPanels[activeWin].syncQueryVnmr(type, p);
    }

    public ParamIF syncQueryExpr(String p)
    {
        return expPanels[activeWin].syncQueryExpr(p);
    }

    public ExpPanel getActiveExp()
    {
        return activeExp;
    }

    public ExpPanel getDefaultExp()
    {
        for (int n = 0; n < maxViews; n++) {
            if (expPanels[n] != null)
                return expPanels[n];
        }
        return null;
    }

    public ExpPanel getExp(int id)
    {
        if (id >= maxViews)
            return null;
        return expPanels[id];
    }

    public boolean isExpAvailable(int id)
    {
        // if (id >= expNum || expPanels[id] == null)
        if (id >= expNum)
            return false;
        return expSelected[id];
    }

    /**
     * Returns the names (e.g., "exp1") of all the experiments currently
     * loaded in some workspace.
     * @return A sorted set containing the names.
     */
    public SortedSet<String> getExperimentNames() {
        SortedSet<String> names = new TreeSet<String>();
        for (int i = 0; i < maxViews; i++) {
            if (expPanels[i] != null) {
                names.add(expPanels[i].getExpName());
            }
        }
        return names;
    }


    public void setParamPanel(String name)
    {
        expPanels[activeWin].setParamPanel(name);
    }

    public ParameterPanel getParamPanel(String name)
    {
        return expPanels[activeWin].getParamPanel(name);
    }

    private void validateExps() {
        for (int k = 0; k < expNum; k++) {
            if (expPanels[k] != null && expVisible[k]) {
                if (expSelected[k] && expPanels[k] != activeExp) {
                    expPanels[k].validate();
                }
            }
        }
        if (activeExp != null)
            activeExp.validate();
    }

    protected void resizeCanvas()
    {
        int w, h, r, c, k;
        int th;
        // Dimension pSize = getSize();
        // int rw = pSize.width - curCols * 2;
        // int rh = pSize.height - curRows * 2;
        ExpPanel exp;
        Rectangle rc = getBounds();
        int xRows = curRows;
        int xCols = curCols;

        if (bOverlayed) {
            xRows = 1;
            xCols = 1;
        }
        int rw = rc.width - (xCols -1) * 2;
        int rh = rc.height - (xRows -1) * 2;
        w = rw / xCols;
        h = rh / xRows;
        if (w < 20)
            w = 20;
        if (h < 20)
            h = 20;
        int dw = rw - w * xCols;
        int dh = rh - h * xRows;

        panW = w;
        panH = h;
        if (bSmsVisible) {
            if (smsPan != null)
               smsPan.setBounds(0, 0, rc.width, rc.height);
        }
        JComponent vjmol = null;
        VnmrCanvas canvas;
        if (activeExp != null) {
            vjmol = activeExp.getVJMol();
            th = activeExp.getTitleBarHeight();
            trayButton.setSize(16, th);
        }
        topLeftExp = activeExp;
        if (bOverlayed || (curCols * curRows == 1)) {
            for (k = 0; k < expNum; k++) {
                if (expPanels[k] != null && expVisible[k]) {
                    if (expSelected[k] && expPanels[k] != activeExp) {
                        expPanels[k].setBounds(0, 0, w, h);
                        // expPanels[k].validate();
                        canvas = expPanels[k].getCanvas();
                        if (canvas != null)
                            canvas.createShowEvent();
                    }
                }
            }
            if (activeExp != null) {
                activeExp.setBounds(0, 0, w, h);
                th = activeExp.getTitleBarHeight();
                canvas = activeExp.getCanvas();
                if (canvas != null)
                    canvas.createShowEvent();
                if (vjmol != null && vjmol.isVisible() && m_vjmolClass != null)
                {
                    vjmol.setBounds(0, th, w, h);
                    vjmolMethod(vjmol, "updateSize", null, null);
                }
                // activeExp.validate();
            }
            validateExps();
            
            return;
        }
        k = 0;
        int x;
        int y = 0;
        exp = null;
        for (r = 0; r < curRows; r++) {
            x = 0;
            dw = rw - w * xCols;
            for (c = 0; c < curCols; c++) {
                exp = null;
                while (k < maxViews) {
                    if (expPanels[k] != null && expVisible[k]) {
                        if (expSelected[k]) {
                            exp = expPanels[k];
                            break;
                        }
                    }
                    k++;
                }
                if (exp == null)
                    break;
                exp.setBounds(x, y, w, h);
                if (x <= 0  && y <= 0)
                    topLeftExp = exp;
                th = exp.getTitleBarHeight();
                if (k == activeWin && vjmol != null && vjmol.isVisible() &&
                        m_vjmolClass != null)
                {
                        vjmol.setBounds(x, y+th, w, h);
                        vjmolMethod(vjmol, "updateSize", null, null);
                }
                // exp.validate();
                k++;
                if (k >= maxViews)
                    break;
                x = x + w + 2;
                if (dw > 0) {
                    x++;
                    dw--;
                }
            }
            if (k >= maxViews)
                break;
            y = y + h + 2;
            if (dh > 0) {
                y++;
                dh--;
            }
        }
        validateExps();
    }

    public synchronized void smallLarge(int id)
    {
        int i;

        if (expPanels[id] == null)
             return;
        getShownNum();
        if (expShownNum <= 1) { // something wrong
            viewAll = false;
            return;
        }
        if (!expSelected[id]) {
            id = expNum;
            for (i = 0; i < expNum; i++) {
               if (expPanels[i] != null && expSelected[i]) {
                      id = i;
                      break;
                }
            }
            if (id >= expNum)
               return; 
        }
        if (!expPanels[id].isVisible())
            return;
        inSmallLarge = true;
        setResizeStatus(true);
        /**
         for (i = 0; i < maxViews; i++) {
            if (expPanels[i] != null) {
                expPanels[i].sendToVnmr("M@xresize\n");
            }
         }
        **/
        if (viewAll) { // split screen -> full screen
            viewAll = false;
            // activeWin = id;
            curCols = 1;
            curRows = 1;
            for (i = 0; i < maxViews; i++) {
                if (i != id && expPanels[i] != null) {
                    //  expPanels[i].sendToVnmr("M@xstop\n");
                    setExpVisible(i, false);
                }
            }
            // expPanels[id].sendToVnmr("M@xstart\n");
            expPanels[id].setFullWidth(true);
            setExpVisible(id, true);
        }
        else { // full screen -> split screen
            viewAll = true;
            curCols = cols;
            curRows = rows;
            for (i = 0; i < expNum; i++) {
               if (expPanels[i] != null) {
                   // expPanels[i].sendToVnmr("M@xstop\n");
                   if (cols > 1)
                       expPanels[i].setFullWidth(false);
                   else
                       expPanels[i].setFullWidth(true);
/*
                   if ((i != activeWin)) {
                       if (expPanels[i].isVisible())
                           setExpVisible(i, false);
                           // expPanels[i].setVisible(false);  // make it to generate event
                   }
                   expPanels[i].setMenuAttr(true);
*/
                   if (expSelected[i])
                       setExpVisible(i, true);
                   else
                       setExpVisible(i, false);
                }
            }
            for (i = expNum; i < maxViews; i++)
                 setExpVisible(i, false);
        }

        sshare.putProperty(viewAllStr,new Boolean(viewAll));

        for (int k = 0; k < maxViews; k++) {
            if (k != id && expPanels[k] != null)
                expPanels[k].setActive(false);
        }
        activeExp = expPanels[id];
        // activeExp.sendSetActive();
        // activeExp.setActive(true);

        resizeCanvas();
        //if panel is visible already, setVisible will not generate
        // Shown event, so the XSHOW needs to be send explicitly.
        if (viewAll) {
            // expPanels[activeWin].sendToVnmr("jFunc(" + XSHOW + ", 1)\n");
            // for (i = 0; i < maxViews; i++) {
            //      if (expPanels[i] != null)
            //        expPanels[i].sendToVnmr("X@stop2\n");
            // }
            setResizeStatus(false);
        }
        else {
            // expPanels[activeWin].sendToVnmr("X@stop2\n");
            setResizeStatus(activeWin, false);
        }
        /*
         * if (viewAll) { sendToAllVnmr("jFunc("+PAINT+")\n"); }
         */
        inSmallLarge = false;
        revalidate();
    }

    public synchronized void toggleVpSize(int id) {
        if (expPanels[id] == null || (!expPanels[id].isAlive()))
            return;
        if (!expPanels[id].isVisible())
            return;
        Dimension d0 = getSize();
        Dimension d1 = expPanels[id].getSize();
        if (d1.width <= (d0.width - 8) || d1.height <= (d0.height - 8))
            viewAll = true;
        else
            viewAll = false;
        smallLarge(id);
    }

    public synchronized void maximizeViewport(int id, boolean max)
    {
        if (id >= expNum)
            return;
	if(m_overlayMode == OVERLAY_ALIGN) {
	   setOverlayMode(0);		
	   return;
	}

        if (activeWin == id) {
	    if(max && !viewAll) return;  // already maximized
	    if(!max && viewAll) return;  // already splitted
        }
        if (expPanels[id] == null || (!expSelected[id]))
             return;
 	if(max) viewAll = true;
        else viewAll = false;
        smallLarge(id);
    }

    private void execOverlayCmd() {
	if (m_overlayMode == NOOVERLAY_ALIGN || m_overlayMode == OVERLAY_ALIGN) 
	     processOverlayCmd("align"); 
	else if(m_overlayMode == STACKED) processOverlayCmd("stack"); 
	else if(m_overlayMode == UNSTACKED) processOverlayCmd("unstack"); 
        else processOverlayCmd("overlayMode");

	m_prevOverlayMode = m_overlayMode;
    }

    public void setOverlayMode(int  n) {
        boolean  bSync = false;

	setOverlayDispType(n);

	m_overlayMode = n;

	if(n == NOOVERLAY_NOALIGN ||
	   n == NOOVERLAY_ALIGN ) {
		overlaySync(false); 
		overlayCanvas(false);
	} else if(n == OVERLAY_NOALIGN) {
		overlaySync(false);
		overlayCanvas(true);
        } else if(n == OVERLAY_ALIGN || n == STACKED || n == UNSTACKED) {
                bSync = true;
		overlaySync(true);
		overlayCanvas(true);
	}

        if (!bSync)
            execOverlayCmd();
    }

    public void setOverlayDispType(int  n) {
        for (int k = 0; k < maxViews; k++) {
           ExpPanel exp = expPanels[k];
           if (exp != null) {
               VnmrCanvas canvas = exp.getCanvas(); 
               if (canvas != null)
                   canvas.overlayDispType(n, false);
           }
        }
    }

    public void overlaySync(boolean b) {
        bSyncMode = b;
        for (int k = 0; k < maxViews; k++) {
           ExpPanel exp = expPanels[k];
           if (exp != null) {
               VnmrCanvas canvas = exp.getCanvas(); 
               if (canvas != null)
                   canvas.overlaySync(b);
           }
        }
    }

    public void canvasSyncReady(int id) {
        for (int k = 0; k < maxViews; k++) {
           if (k != id) {
               ExpPanel exp = expPanels[k];
               if (exp != null) {
                   VnmrCanvas canvas = exp.getCanvas(); 
                   if (canvas != null) {
                       if (canvas.isOverlaySync()) // canvas not ready
                           return;
                   }
               }
           }
        }
        execOverlayCmd();
    }

    public void overlayCanvas(boolean bOvly, boolean bResize1) {
        if (bXwindow) // do not support xwindow
            return;
        int k;
        ExpPanel exp = null;
        boolean oldOvly = bOverlayed;
        bOverlayed = bOvly;
        if (bOvly) {
            if (activeExp == null || !expSelected[activeExp.getViewId()]) {
                for (k = 0; k < expNum; k++) {
                     if (expPanels[k] != null && expSelected[k]) {
                         if (expPanels[k].isAlive()) {
                             exp = expPanels[k];
                             break;
                         }
                     }
                }
                if (exp != null) {
                    getShownNum();
                    exp.sendSetActive();
                }
                return;
            }
        }
        getShownNum();
        if (oldOvly == bOvly) {
            if (bSyncMode) {
                overlaySync(false);
                execOverlayCmd();
            }
            if (!bOvly)  // no need to change
               return;
            if (activeExp != null && activeExp == topExp) {
               if (!bChangeCanvas)
                  return;
            }
            bChangeCanvas = false;
        }
        if (activeExp == null)
            return;
        if (bOvly) {
            for (k = 0; k < maxViews; k++) {
                exp = expPanels[k];
                if (exp != null) {
                    setExpVisible(k, expSelected[k]);
                }
            }
        }
        exp = null;

        VnmrCanvas canvas;
        VnmrCanvas topCanvas;
        boolean isFirst = true;

        if (topExp == null)
            topExp = activeExp;
        topCanvas = activeExp.getCanvas(); 
        if (topCanvas == null)
            return;
        if (!bOvly) {
            // topExp.setOverlay(false, false, false, null);
            if (isOverlayed()) {
                resetExpLayer(viewAll);
                for (k = 0; k < maxViews; k++) {
                    exp = expPanels[k];
                    if (exp != null) {
                        if (!viewAll) {
                            if (exp != activeExp) 
                                exp.setVisible(false);
                        }
                        else
                            setExpVisible(k, expSelected[k]);
                    }
                }
            }
            setExpVisible(activeExp.getViewId(), true);
            topExp = null;
            exp = null;
            if (!bResize1)
                return;
        }
        else {
            topExp = activeExp;
            setLayer(topExp, layer2);
            topCanvas.startOverlay(true);
            for (k = 0; k < expNum; k++) {
                exp = expPanels[k];
                if (exp != null && exp != activeExp) {
                    // exp.setMenuAttr(false);
                    if (expSelected[k]) {
                        canvas = exp.getCanvas(); 
                        canvas.startOverlay(false);
                        if (isFirst) {
                            setLayer(exp, layer0);
                            exp.setOpaque(true);
                            isFirst = false;
                            /*  overlay_flag, is_bottom, is_top, top_comp */
                            exp.setOverlay(bOvly, true, false, topCanvas);
                            // canvas.setOverlay(bOvly, true, false, topCanvas);
                        }
                        else {
                            setLayer(exp, layer1);
                            exp.setOpaque(false);
                            exp.setOverlay(bOvly, false, false, topCanvas);
                            // canvas.setOverlay(bOvly, false, false, topCanvas);
                        }
                    }
                }
            }
            topExp.setOverlay(bOvly, false, true, null);
            if (isFirst)
                topExp.setOpaque(true);
            else
                topExp.setOpaque(false);
            // topExp.setMenuAttr(false);
        }
        if (!bResize1)
             return;
        resizeCanvas();
        revalidate();
    }

    public void paint_frame_backregion(Graphics2D g) {
        for (int k = 0; k < expNum; k++) {
           ExpPanel exp = expPanels[k];
           if (exp != null) {
              if (expSelected[k]) {
                  VnmrCanvas canvas = exp.getCanvas();
                  if (canvas != null)
                      canvas.paint_frame_backregion(g, true);
              }
           }
        }
    }

    // don't know why this is needed. It sends overlayMode and repaint. 
    // It is called by setCanvasVisible (hide and show the canvas). 
    public void fixScales(boolean b) {
	if(expPanels[m_alignvp] == null) return;
        XMap activeMap = expPanels[m_alignvp].getCanvas().getMap(1);
	if(activeMap == null) return;

        int k = 0;
        for (k = 0; k < expNum; k++) {
           if (k != m_alignvp && expPanels[k] != null && expSelected[k]) { 
           XMap map = expPanels[k].getCanvas().getMap(1);
	   if(map == null) continue;
	   if((activeMap.dcmd.equals("dconi") && map.dcmd.equals("dconi") &&
	   map.axis2.equals(activeMap.axis2) && map.axis1.equals(activeMap.axis1)) ||
		(activeMap.dcmd.equals("ds") && map.dcmd.equals("ds") &&
           map.axis2.equals(activeMap.axis2))) {
	
	     if(b) {
		expPanels[k].getCanvas().overlayDispType(m_overlayMode, false);
		expPanels[k].sendToVnmr("repaint('all')\n");
		return;
	     } else {
//System.out.println("fixScales false "+k+" "+m_alignvp);
		expPanels[k].getCanvas().overlayDispType(m_overlayMode, true);
		expPanels[k].sendToVnmr("repaint('all')\n");
		return;
	     }
	   }
	   }
	}
    }

    public void setCanvasVisible(int id, boolean b) {
        if (id >= maxViews)
           return;

        ExpPanel exp = expPanels[id];
        if (exp == null)
           return;

        VnmrCanvas canvas = exp.getCanvas();
        if (canvas == null)
           return;

	if(m_overlayMode >= OVERLAY_ALIGN && id == m_alignvp) {
	   fixScales(b);
	}
        if (b)
           canvas.setVisible(b);

        int k = 0;
        for (k = 0; k < expNum; k++) {
           if (id != k && expPanels[k] != null) {
              if (expPanels[k].isVisible())
                 k++;
           }
        }
        if (k <= 1)
           return;
        if (!isOverlayed() && !bOverlayed)
           return;
        canvas.setVisible(b);

        boolean isFirst = true;
        boolean bRelayout = false;
        int  x = 0;

        if (exp != activeExp) {
           if (!b) {
              if (getLayer(exp) == layer0 || canvas.isBottomLayer())
                  bRelayout = true;
           }
           else {
              for (k = 0; k < expNum; k++) {
                  exp = expPanels[k];
                  if (exp != null) {
                     if (expSelected[k]) {
                         if (getLayer(exp) == layer0)
                             x++;
                         else {
                             canvas = exp.getCanvas();
                             if (canvas.isBottomLayer())
                                 x++;
                         }
                     }
                  }
              }
              if (x > 1)
                  bRelayout = true;
           }
        }
        if (bRelayout) {
           for (k = 0; k < expNum; k++) {
               exp = expPanels[k];
               if (exp != null && exp != activeExp) {
                   if (expSelected[k]) {
                      canvas = exp.getCanvas();
                      if (canvas != null && canvas.isVisible()) {
                         if (isFirst) {
                             isFirst = false;
                             canvas.setBottomLayer(true);
                             setLayer(exp, layer0);
                             exp.setOpaque(true);
                          }
                          else {
                              canvas.setBottomLayer(false);
                              setLayer(exp, layer1);
                              exp.setOpaque(false);
                          }
                      }
                      else
                          exp.setOpaque(false);
                   }
               }
           }
        }

        Runnable repaintActiveCanvas = new Runnable() {
                public void run() {
                    /***
                    if (expPanels[activeWin] != null) {
                       VnmrCanvas xcanvas = expPanels[activeWin].getCanvas();
                       if (xcanvas != null)
                           xcanvas.repaint();
                    }
                    ***/
                    repaint();
                }
        };
        SwingUtilities.invokeLater(repaintActiveCanvas);
    }

    public void overlayCanvas(boolean b) {
        overlayCanvas(b, true);
    }

    public void setCanvasAvailable(int n, boolean b) {
        if (n >= maxViews)
           return;
        boolean bActive = false;
        if (expPanels[n] != null && expPanels[n].isAlive()) {
           if (expSelected[n] != b)
               bChangeCanvas = true;
           bActive = expPanels[n].isInActive();
           expPanels[n].setAvailable(b);
        }
        expSelected[n] = b;
        setExpVisible(n, b);
        getShownNum();
        if (!bReady)
           return;
        if (!b && bActive) {
            openActiveExp();
        }
        viewAll = true;
        sshare.putProperty(viewAllStr,new Boolean(viewAll));
        if (bChangeCanvas) {
            adjustRowColumn();
            initCanvasArray();
        }
    }

    public void setCanvasOverlayMode(String cmd) {
        if (bXwindow) // do not support xwindow
            return;
        if (cmd == null)
            return;
        VnmrCanvas canvas;
        ExpPanel exp;
        if (cmd.equals("xor"))
            bXor = true;
        else
            bXor = false;

        for (int k = 0; k < maxViews; k++) {
            exp = expPanels[k];
            if (exp != null) {
                canvas = exp.getCanvas(); 
                if (canvas != null)
                    canvas.setOverlayMode(bXor);
            }
        }
    }

    public void setCanvasGraphicsMode(String cmd) {
        if (bXwindow) // do not support xwindow
            return;
        if (cmd == null)
            return;
        VnmrCanvas canvas;
        ExpPanel exp;
        for (int k = 0; k < maxViews; k++) {
            exp = expPanels[k];
            if (exp != null) {
                canvas = exp.getCanvas(); 
                if (canvas != null)
                    canvas.setGraphicsMode(cmd);
            }
        }
    }

    public void logout()
    {
        logoutCalled = true;
        int a;
        for (a = 0; a < maxViews; a++) {
            if (expPanels[a] != null) {
                expPanels[a].quit();
            }
            expPanels[a] = null;
            canvasComp[a] = null;
            canvasObj[a] = null;
            String s = expShowStr+a;
            if (expSelected[a])
                sshare.putProperty(s, new Boolean(true));
            else
                sshare.putProperty(s, new Boolean(false));
        }
        for (a = 0; a < maxViews; a++) {
            if (expThread[a] != null && expThread[a].isAlive()) {
                try {
                    expThread[a].join();
                }
                catch(InterruptedException e) {
                }
            }
            expThread[a] = null;
        }
        sshare.putProperty(viewAllStr,new Boolean(viewAll));
    }

    public void setViewMargin(int m)
    {
/*
        if (vpManager != null)
            vpManager.setViewMargin(m);
*/
    }

    public void setCanvasCursor(String c) {
        for (int i = 0; i < expNum; i++) {
             if (expPanels[i] != null) {
                expPanels[i].setCanvasCursor(c);
             }
        }
    }

    public int getTitleBarHeight()
    {
        int h = 0;

        for (int k = 0; k < maxViews; k++) {
            if (expPanels[k] != null) {
                int h2 = expPanels[k].getTitleBarHeight();
                if (h2 > h)
                    h = h2;
            }
        }
        return h;
    }

    public void processDataInfo(String key, String cmd) 
    {
	StringTokenizer tok = new StringTokenizer(cmd, " ,\n");
        if(!tok.hasMoreTokens()) return;
	
	String vpStr = tok.nextToken().trim();
        int vp;
	try {
	   vp = Integer.parseInt(vpStr);
	}
	catch (NumberFormatException er) { return; }

	vp = vp -1;
	if(vp < 0 || vp >= maxViews || expPanels[vp] == null) return;
	
 	int ind = cmd.indexOf(" ");
	if( ind < 0) return;

	cmd = cmd.substring(ind);
	expPanels[vp].getCanvas().processCommand(key+" "+cmd.trim());
		
    }
    
    public void updateTrace(int t, String axis2, String axis1) 
    {
	int k;
        VnmrCanvas canvas = null;
        for (k = 0; k < maxViews; k++) {
	    if(k == activeWin) continue;
            if (expPanels[k] != null && expSelected[k]) {
		canvas = expPanels[k].getCanvas();
		if(canvas == null) continue;
		canvas.updateTrace(k, t, axis2, axis1);
	    }
	}
    }

    public void processOverlayCmd(String cmd) 
    {
	if(cmd.equals("stack") || 
	   cmd.equals("unstack") || 
	   cmd.equals("align") || 
	   cmd.equals("init") || 
	   cmd.equals("overlayMode") ) processOverlayCmd_init(cmd);
	else if(cmd.equals("update")) processOverlayCmd_update(cmd);
	else  return;
    }

    public void processOverlayCmd_update(String cmd)
    {
	   if(!cmd.equals("update") ) return;
    }
  
    public void processOverlayCmd_init(String cmd)
    {
  
	if(!cmd.equals("stack") && 
	   !cmd.equals("unstack") &&
	   !cmd.equals("align") &&
	   !cmd.equals("init") &&
	   !cmd.equals("overlayMode") ) return;

	boolean bStack = false;
	boolean bAlign = false;
	if(cmd.equals("stack")) bStack = true;
	if(cmd.equals("align")) bAlign = true;

        XMap activeMap = null;
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;

        VnmrCanvas canvas = null;
	if(expPanels[activeWin] != null) {
	   canvas = expPanels[activeWin].getCanvas();
	}
	if(canvas != null) {
          activeMap = canvas.getMap(1);
	  if(activeMap != null) {
            x = activeMap.x;
            y = activeMap.y;
            w = activeMap.width;
            h = activeMap.height;
	  }
	}

	//get data info from canvas, count 1D and 2D and fill the arrays.
	XMap map[] = new XMap[expNum]; 
    	m_spx = 0;
    	m_spy = 0;
        m_epx = 0;
        m_epy = 0;
	m_align = false;
	m_stack = false;
	m_alignvp = -1;
	String axis2=null;
	String axis1=null;
	int i1 = 0;
	int i2 = 0;
	int i1D = -1;
	int i2D = -1;
	m_XY1D=false; // true if the same 1D will be displayed as side spectrum for both F1,F2.
        boolean has1D_X=false; // true if a X side spectrum exist.
 	int k;
        for (k = 0; k < expNum; k++) {
            if (expPanels[k] != null && expSelected[k]) {
		canvas = expPanels[k].getCanvas();
		if(canvas == null) continue;

		if(bStack || bAlign)
		   canvas.selectFrameWithG(1, x, y, w, h);
		else if(m_prevOverlayMode == NOOVERLAY_ALIGN || 
			m_prevOverlayMode == OVERLAY_ALIGN || 
			m_prevOverlayMode == STACKED) { 
		   canvas.selectFrameWithG(1, 0, 0, 0, 0);
		} else canvas.selectFrame(1);

                map[k] = canvas.getMap(1); 
		if( map[k] == null) continue;

		map[k].set1D_Y(0, 0);
		if( map[k].dcmd == null) continue;


		if(map[k].dcmd.equals("ds")) {
		   if(map[k].axis2.length() > 0) i1++;
		   else {
			sendOverlayInfo(cmd,null,null);
			return;
		   }
		} else if(map[k].dcmd.equals("dconi")) {
		   if(map[k].axis2.length() > 0 && map[k].axis1.length() > 0) i2++;
		   else {
			sendOverlayInfo(cmd,null,null);
			return;
		   }
		} else {
		   sendOverlayInfo(cmd,null,null);
		   return;
		} 
	    }
        }

	if(i1 > 0 || i2 > 0) {
	   m_align = true;
	   m_stack = true;
	}
	// find vp with minimum sw (and sw1)
	// try in the order: heteronumclear 2D, 2D then 1D.
        if(i2 > 0) {
          for (k = 0; k < expNum; k++) {
		if(map[k] == null || map[k].dcmd == null) continue;
		if(map[k].dcmd.equals("dconi") 
			&& !map[k].axis2.equals(map[k].axis1)) {
		   i2D = k;
		   break;
		} else if(map[k].dcmd.equals("dconi") && i2D < 0) {
		   i2D = k;
		}
	  }
	  //if 2D
	  if(i2D >=0 && i2D < expNum) {
             if(map[i2D].axis2.equals(map[i2D].axis1)) m_XY1D=true;
	     expPanels[i2D].getCanvas().overlayDispType(m_overlayMode, true);
	     m_alignvp = i2D; 
	     m_spx = map[i2D].spx;
	     m_spy = map[i2D].spy;
	     m_epx = map[i2D].epx;
	     m_epy = map[i2D].epy;
	     for (k = 0; k < expNum; k++) {
		if(map[k] == null || k == i2D || map[k].dcmd == null ||
		!map[k].dcmd.equals("dconi")) continue;

                if( !map[k].axis1.equals(map[i2D].axis1) || 
		  !map[k].axis2.equals(map[i2D].axis2)) {
		    m_align = false;
		    m_stack = false;
		    sendOverlayInfo(cmd,null,null);
		    return;
                } else {
		    if(map[k].spx > m_spx) m_spx = map[k].spx;
		    if(map[k].spy > m_spy) m_spy = map[k].spy;
		    if(map[k].epx <  m_epx) m_epx = map[k].epx;
		    if(map[k].epy <  m_epy) m_epy = map[k].epy;
		}
	     }
	  } 
	}

	int trace = 0;
	if(i1 > 0) {
          for (k = 0; k < expNum; k++) {
		if(map[k] == null || map[k].dcmd == null) continue;
		if(map[k].dcmd.equals("ds")) {
		   i1D = k;
		   break;
		}
	  }
	  //if 1D
	  if(i1D >=0 && i1D < expNum) {
	     if(i2D >= 0 && i2D < expNum) {
		m_stack = false;
		if(map[i2D].axis2.equals(map[i2D].axis1)) trace = 0;
                else trace = map[i2D].trace;
		if(!map[i1D].axis2.equals(map[i2D].axis2) &&
		  !map[i1D].axis2.equals(map[i2D].axis1) ) {
		    m_align = false;
		} else if(map[i1D].axis2.equals(map[i2D].axis2)) {
                    if(map[i1D].spx > m_spx) m_spx = map[i1D].spx;
                    if(map[i1D].epx < m_epx) m_epx = map[i1D].epx;
                    map[i1D].set1D_Y(2, trace);
                    if(m_XY1D) {
                       has1D_X=true;
                    }
		} else if(map[i1D].axis2.equals(map[i2D].axis1)) {
		    if(map[i1D].spx > m_spy) m_spy = map[i1D].spx;
		    if(map[i1D].epx < m_epy) m_epy = map[i1D].epx;
		    map[i1D].set1D_Y(1, trace);
		}
	     } else {
	        expPanels[i1D].getCanvas().overlayDispType(m_overlayMode, true);
	        m_alignvp = i1D; 
	        m_spx = map[i1D].spx;
		m_epx = map[i1D].epx;
	     }
	     for (k = 0; k < expNum; k++) {
	        if(map[k] == null || k == i1D || map[k].dcmd == null ||  
		!map[k].dcmd.equals("ds")) continue;

                if(i2D >= 0) {
		  if( !map[k].axis2.equals(map[i2D].axis2) &&
		  	!map[k].axis2.equals(map[i2D].axis1) ) {
		    m_align = false;
                  } else if(map[k].axis2.equals(map[i2D].axis2)) {
                    if(map[k].spx > m_spx) m_spx = map[k].spx;
                    if(map[k].epx < m_epx) m_epx = map[k].epx;
                    if(m_XY1D && has1D_X) {
                            m_XY1D=false;
                            map[k].set1D_Y(1, trace);
                        } else {
                            map[k].set1D_Y(2, trace);
                        }
                  } else if(map[k].axis2.equals(map[i2D].axis1)) {
                    if(map[k].spx > m_spy) m_spy = map[k].spx;
                    if(map[k].epx < m_epy) m_epy = map[k].epx;
                    map[k].set1D_Y(1, trace);
                  }
		} else if( !map[k].axis2.equals(map[i1D].axis2)) {
		    m_stack = false;
		    m_align = false;
                } else {
		    if(map[k].spx > m_spx) m_spx = map[k].spx;
		    if(map[k].epx < m_epx) m_epx = map[k].epx;
		    map[i1D].set1D_Y(2, 0);
		}
	     }
	  } 
	}

	if(m_spx >= m_epx || (i2 > 0 && m_spy >= m_epy)) {
	  m_align = false;
	  m_stack = false;
	}  
	if((i1+i2) < 2) {
	  m_align = false;
	  m_stack = false;
	} 

        if(i2D >=0 && i2D < expNum) {
            axis2 = map[i2D].axis2;
            axis1 = map[i2D].axis1;
	    m_sp2 = map[i2D].sp2;
	    m_sp1 = map[i2D].sp1;
	    m_ep2 = map[i2D].ep2;
	    m_ep1 = map[i2D].ep1;
	    if(map[i2D].sp2 > m_spx) m_sp2 = map[i2D].sp2;
	    if(map[i2D].sp1 > m_spy) m_sp1 = map[i2D].sp1;
	    if(map[i2D].ep2 < m_epx) m_ep2 = map[i2D].ep2;
	    if(map[i2D].ep1 < m_epy) m_ep1 = map[i2D].ep1;
	} else if (activeWin >= 0 && activeWin < expNum) {
	  k = activeWin;
	  if(map[k] != null) {
            axis2 = map[k].axis2;
            axis1 = map[k].axis1;
	    m_sp2 = map[k].sp2;
	    m_sp1 = map[k].sp1;
	    m_ep2 = map[k].ep2;
	    m_ep1 = map[k].ep1;
	    if(map[k].sp2 > m_spx) m_sp2 = map[k].sp2;
	    if(map[k].sp1 > m_spy) m_sp1 = map[k].sp1;
	    if(map[k].ep2 < m_epx) m_ep2 = map[k].ep2;
	    if(map[k].ep1 < m_epy) m_ep1 = map[k].ep1;
	  }
	}

	sendOverlayInfo(cmd,axis2,axis1);
    }

    private void sendOverlayInfo(String cmd,String axis2,String axis1)
    {
 	int k;
        boolean stack=m_stack;
        for (k = 0; k < expNum; k++) {
            if (expPanels[k] != null && expSelected[k]) {
		VnmrCanvas canvas = expPanels[k].getCanvas();
		if(canvas == null) continue;
                XMap map = canvas.getMap(1); 
		if( map == null || map.dcmd == null) continue;
                if(map.dcmd.indexOf("dss") != -1) stack=false;
            }
        }

        for (k = 0; k < expNum; k++) {
            if (expPanels[k] != null && expSelected[k]) {
		VnmrCanvas canvas = expPanels[k].getCanvas();
		if(canvas == null) continue;

		canvas.sendOverlaySpecInfo(cmd, m_overlayMode,
			m_align, stack, activeWin, 
			m_alignvp, m_spx, m_epx-m_spx, m_spy, m_epy-m_spy,
			m_sp2, m_ep2-m_sp2, m_sp1, m_ep1-m_sp1,m_XY1D,axis2,axis1);

	    }
	}
    }

    private String getPrintFileName() {
        if (prtFile == null || prtFile.length() < 1)
	    return null;
        String fname = prtFile;
        String strExt = pltFormat.toLowerCase();
        if (!prtFile.endsWith(".")) {
            if (!strExt.startsWith("."))
		strExt = "." + strExt;
	}
	if (!prtFile.endsWith(strExt))
	    fname = prtFile + strExt;
        return fname;
    }

    /**
     * Saves the image to the different format
     */
	private String convert(String sFile, boolean bConvert) {
		String destFile = getPrintFileName();
		if (destFile == null)
			return null;

		File srcfile = new File(sFile);
		if (!srcfile.exists() || !srcfile.canRead())
			return null;
		destFile = FileUtil.savePath(destFile, false);
		if (destFile == null)
			return destFile;

		if (bPlot || pltFormat.equals("ps") || pltFormat.equals("pcl")
				|| pltFormat.equals("pdf")) {
			StringBuffer sb = new StringBuffer().append("jFunc(")
					.append(JPRINT).append(",'-jpg'");
			if (bPlot)
				sb.append(",'-print'");
			sb.append(",'-file', '").append(UtilB.windowsPathToUnix(sFile))
					.append("','-outfile','")
					.append(UtilB.windowsPathToUnix(destFile)).append("'");
			sb.append(",'-clear'");
			if (!bPrtColor)
				sb.append(",'-mono'");
			if (prtSize != null)
				sb.append(",'-size','").append(prtSize).append("'");
			if (prtOrient != null)
				sb.append(",'-orient','").append(prtOrient).append("'");
			if (prtPlotter != null && (prtPlotter.length() > 0))
				sb.append(",'-plotter',`").append(prtPlotter).append("`");
			if (prtPaper != null)
				sb.append(",'-paper','").append(prtPaper).append("'");
			sb.append(",'-iformat','").append(pltFormat).append("'");
			sb.append(",'-dpi','").append(prtDpiStr).append("'");
			sb.append(",'-iwidth','").append(imgW).append("'");
			sb.append(",'-iheight','").append(imgH).append("'");
			sb.append(",'-pwidth','").append(paperWidth).append("'");
			sb.append(",'-pheight','").append(paperHeight).append("'");
			sb.append(",'-dwidth','").append(prtWidth).append("'");
			sb.append(",'-dheight','").append(prtHeight).append("'");
			sb.append(",'-dx','").append(paperLeftMargin).append("'");
			sb.append(",'-dy','").append(paperBottomMargin).append("'");
			if (prtCopies > 1 && bPlot)
				sb.append(",'-copy','").append(prtCopies).append("'");
			sb.append(")");
			expPanels[activeWin].sendToVnmr(sb.toString());

			return destFile;
		}
		if (bConvert) {
			// Notes
			// 1. On Windows, "convert" is a unix shell script that calls ImageMagic convert.exe
			//   - It is assumed that ImageMagic has been separately installed and added to the SFU path
			//   - file arguments need to be in windows path format 
			//   - file arguments passed into this call are in the correct format for the target host os
			// 2. If a simple rename is carried out (bConvert=false) paths need to be in unix format
			int rotate = 0;
			if (prtOrient != null)
				rotate = -90;
			if (pltFormat.equals("pdf")) {
				String[] cmds1 = {
						WGlobal.SHTOOLCMD,
						WGlobal.SHTOOLOPTION,
						FileUtil.SYS_VNMR + "/bin/convert -page " + prtPaper + " -rotate " + rotate
								+ " \"" + sFile + "\" " + pltFormat + ":\""
								+ destFile + "\"" };
				WUtil.runScript(cmds1);
			} else {
				String[] cmds2 = {
						WGlobal.SHTOOLCMD,
						WGlobal.SHTOOLOPTION,
						FileUtil.SYS_VNMR + "/bin/convert " + "\"" + sFile + "\" " + pltFormat + ":\""
								+ destFile + "\"" };
				WUtil.runScript(cmds2);
			}
			WUtil.deleteFile(sFile);
		} else {
			if (Util.iswindows()) {
				String[] cmds3 = {
						WGlobal.SHTOOLCMD,
						WGlobal.SHTOOLOPTION,
						"mv " + "\"" + UtilB.windowsPathToUnix(sFile)
								+ "\" \"" + UtilB.windowsPathToUnix(destFile)
								+ "\"" };
				WUtil.runScript(cmds3);
			} else {
				String[] cmds3 = { WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
						"mv " + "\"" + sFile + "\" \"" + destFile + "\"" };
				WUtil.runScript(cmds3);
			}
		}
		return destFile;
	}

    public void convertJpeg(String sfile, String dfile, Dimension dim) {
        imgW = dim.width;
        imgH = dim.height;
        Dimension pdim = VjPaperMedia.getPaperPS(prtPaper, false);
        if (pdim == null)
            pdim = PrinterPaper.getPaperSize(prtPaper);
        paperLeftMargin = PrinterPaper.getPaperMargin();
        paperTopMargin = paperLeftMargin;
        paperWidth = pdim.width;
        paperHeight = pdim.height;
        boolean bPs = false;
        prtFile = dfile;
        if (!bPlot) {
           if (pltFormat.equalsIgnoreCase("ps"))
              bPs = true;
           else if (pltFormat.equalsIgnoreCase("pcl"))
              bPs = true;
           else if (pltFormat.equalsIgnoreCase("pdf"))
              bPs = true;
        }
        if (bPlot || bPs) {
           if (prtOrient != null) {
               if (prtOrient.equalsIgnoreCase("landscape") &&
                                  (pdim.height > pdim.width)) {
                   int dw = pdim.height;
                   pdim.height = pdim.width;
                   pdim.width = dw;
               }
               else
                   prtOrient = null;
           }
           if (prtSize.equalsIgnoreCase("halfpage")) {
               pdim.width = (int) ((float)pdim.width * 0.55f);
               pdim.height = (int) ((float)pdim.height * 0.55f);
               paperLeftMargin = 0;
               paperTopMargin = 0;
           }
           else if (prtSize.equalsIgnoreCase("quarterpage")) {
               pdim.width = (int) ((float)pdim.width * 0.4f);
               pdim.height = (int) ((float)pdim.height * 0.4f);
               paperLeftMargin = 0;
               paperTopMargin = 0;
           }
           prtWidth = pdim.width - paperLeftMargin - paperRightMargin;
           prtHeight = pdim.height - paperTopMargin - paperBottomMargin;
           float f1 = (float) dim.height / (float) dim.width;
           float fw = (float) prtWidth;
           float fh = (float) prtHeight;
           float fh2;
           float f2 = 1.0f;
           while (true) {
               fh2 = fw * f2 * f1; 
               if (fh2 <= fh)
                  break;
               f2 = f2 - (fh2 - fh) / fh2;
           }
           fw = fw * f2;
           fh = fw * f1;
           prtWidth = (int) fw;
           prtHeight = (int) fh;
        }
        String imgFile = convert(sfile, true);
        if (!bPlot && imgFile != null)
             Messages.postInfo("Saving file: " + imgFile);
    }

    public void printImage() {
        String dataFile;
        String iformat = "png";
        boolean bNeedConvert = true;
 
        if (plotImg == null) {
            return;
        }
        // current ImageIO supports png, jpeg, x-png, bmp, gif
        if (pltFormat.equals("jpg") || pltFormat.equals("png") || pltFormat.equals("gif")) {
             bNeedConvert = false;
             iformat = pltFormat;
        }
        String tmpFile = new StringBuffer().append("plot").append(File.separator).append("tmp").
               append(System.currentTimeMillis()).append(".").append(pltFormat).toString();
        dataFile = FileUtil.savePath(tmpFile);
        if (dataFile == null) {
            plotImg = null;
            return;
        }

        try {
            File file = new File(dataFile);
            ImageIO.write(plotImg, iformat, file); 
        }
        catch (Exception ex) {
            Messages.postInfo("Error: could not write image.");
            plotImg = null;
            return;
        }
        catch (OutOfMemoryError e2) {
            Messages.postInfo("Error: out of memory for image.");
            plotImg = null;
            return;
        }

        String imgFile = convert(dataFile, bNeedConvert);
        if (imgFile != null) {
            if (!bPlot)
                Messages.postInfo("Saving file: " + imgFile);
        }
        else
            Messages.postInfo("Error: could not create image.");

        plotImg = null;
    }

    private void printFrames() {
        if (prtFile == null)
             return;
        if (robot == null) {
             try
             {
                robot = new Robot();
             }
             catch (Exception e)
             {
                Messages.writeStackTrace(e);
                Messages.postDebug(e.toString());
                return;
             }
        }

        int x, y, w, h;
        paperDim = VjPaperMedia.getPaperPS(prtPaper, false);
        if (paperDim == null)
            paperDim = PrinterPaper.getPaperSize(prtPaper);
        paperLeftMargin = PrinterPaper.getPaperMargin();
        paperRightMargin = paperLeftMargin;
        paperTopMargin = paperLeftMargin;
        paperBottomMargin = paperTopMargin;
        paperWidth = paperDim.width;
        paperHeight = paperDim.height;
        boolean bPs = false;
        if (!bPlot) {
           if (pltFormat.equalsIgnoreCase("ps"))
              bPs = true;
           else if (pltFormat.equalsIgnoreCase("pcl"))
              bPs = true;
           else if (pltFormat.equalsIgnoreCase("pdf"))
              bPs = true;
        }
        if (bPlot || bPs) {
           if (prtOrient != null) {
               if (prtOrient.equalsIgnoreCase("landscape") &&
                                  (paperDim.height > paperDim.width)) {
                   w = paperDim.height;
                   paperDim.height = paperDim.width;
                   paperDim.width = w;
               }
               else
                   prtOrient = null;
           }
           if (prtSize.equalsIgnoreCase("halfpage")) {
               paperDim.width = (int) ((float)paperDim.width * 0.55f);
               paperDim.height = (int) ((float)paperDim.height * 0.55f);
               paperLeftMargin = 0;
               paperTopMargin = 0;
           }
           else if (prtSize.equalsIgnoreCase("quarterpage")) {
               paperDim.width = (int) ((float)paperDim.width * 0.4f);
               paperDim.height = (int) ((float)paperDim.height * 0.4f);
               paperLeftMargin = 0;
               paperTopMargin = 0;
           }
           w = paperDim.width - paperLeftMargin * 2;
           h = paperDim.height - paperTopMargin - paperBottomMargin;
           float rw = (float)aipFrameWidth / (float)aipFrameHeight;
           float rs = 1.0f;
           while (rs > 0.5f) {
                prtWidth = (int) ((float) w * rs);
                prtHeight = (int) ((float) prtWidth * rw);
                if (prtWidth <= w && prtHeight <= h)
                    break;
                rs = rs - 0.05f;
           }
        }

        VnmrCanvas canvas = expPanels[aipPrtId].getCanvas();
        Dimension dim0 = canvas.getSize();
        BufferedImage frameImg = plotImg;
        BufferedImage tmpImg = null;
        String tmpName = prtFile;
        int nIndex = tmpName.indexOf(".");
        Graphics2D tmpGc = null;
        int nFrames = 0;
        int col, row, num;
        int pw = 0;
        int ph = 0;
        int r = 100;
        int c = 100;
        QuotedStringTokenizer tok = new QuotedStringTokenizer(prtFrames);
        if (tok.hasMoreTokens())
            nFrames = Integer.parseInt(tok.nextToken());
        if (nFrames <= 0) {
            if ((aipCols * aipRows) > 1) {
                Messages.postError("No frames selected.");
                return;
            }
            nFrames = 1;
        }
        if (aipCols > 1 || aipRows > 1) {
             c = dim0.width / aipCols;
             r = dim0.height / aipRows;
        }
        num = 0;
        while (num < nFrames) {
             x = 2;
             y = 2;
             w = aipFrameWidth;
             h = aipFrameHeight;
             try {
                 if (tok.hasMoreTokens())
                    x = Integer.parseInt(tok.nextToken());
                 if (tok.hasMoreTokens())
                    y = Integer.parseInt(tok.nextToken());
                 if (tok.hasMoreTokens())
                    tok.nextToken();  // skip w
                 if (tok.hasMoreTokens())
                    tok.nextToken();  // skip h
             }
             catch (NumberFormatException er) {
                 Messages.postError("Error argument: "+prtFrames);
                 return;
             }
             if (w < 1 || h < 1) {
                 Messages.postError("Error image size: "+w+" x "+h);
                 return;
             }
             if (aipCols > 1 || aipRows > 1) {
                 col = x / c;
                 row = y / r;
                 x = (w + 4) * col + 2;
                 y = (h + 4) * row + 2;
             }
             num++;
             if (tmpImg == null || pw != w || ph != h) {
                 try {
                    tmpImg = new BufferedImage(w, h, BufferedImage.TYPE_INT_RGB);
                 }
                 catch (OutOfMemoryError e) {
                    tmpImg = null;
                 }
                 catch (Exception ex) {
                    tmpImg = null;
                 }
                 if (tmpImg == null) {
                    Messages.postError("Could not allocate memory("+w+" x "+h+") for print.");
                    return;
                 }

                 pw = w;
                 ph = h;
                 tmpGc = tmpImg.createGraphics();
             }
             tmpGc.drawImage(frameImg, 0, 0, w, h, x, y, x+w, y+h, null);
             if (nFrames > 1) {
                 if (nIndex >= 0)
                     prtFile = new StringBuffer().append(tmpName.substring(0, nIndex)).
                             append(num).append(tmpName.substring(nIndex)).toString();
                 else
                     prtFile = tmpName+num;
             }
             else
                 prtFile = tmpName;
             plotImg = tmpImg;
             
             printImage();
        }

        frameImg = null;
        tmpImg = null;
        if (tmpGc != null) {
           tmpGc.dispose();
           tmpGc = null;
        }
    }

    private void previewImage() {
        if (plotImg == null) {
           return;
        }
        VjPrintPreview.setPreviewImage(plotImg);
        int w = (int) ((float) paperDim.width * prtDpi / 72.0f);
        int h = (int) ((float) paperDim.height * prtDpi / 72.0f);
        VjPrintPreview.setPreviewSize(new Dimension(w, h),
                        new Dimension(imgW, imgH));
        int l = (int) ((float) paperLeftMargin * prtDpi / 72.0f);
        int r = (int) ((float) paperRightMargin * prtDpi / 72.0f);
        int t = (int) ((float) paperTopMargin * prtDpi / 72.0f);
        int b = (int) ((float) paperBottomMargin * prtDpi / 72.0f);
        VjPrintPreview.setMargin(l, r, t, b);
        if (bPrintToFile)
            VjPrintPreview.showDialog(VjPrintDef.SAVE);
        else
            VjPrintPreview.showDialog(VjPrintDef.PRINT);
        VjPrintPreview.setPrintEventListener(this);
    }

    private boolean printNextCanvas() {
        int n, k;
        VnmrCanvas canvas;

        n = 0;
        for (k = 0; k < maxViews; k++) {
            if (expPanels[k] != null) {
                canvas = expPanels[k].getCanvas();
                if (canvas != null && canvas.getPrintMode()) {
                   if (canvas.isPrinting())
                      return true;
                   n++;
                }
            }
        }
        if (n < 1)
            return false;
        canvas = null;
        for (k = 0; k < maxViews; k++) {
            // the top (active) one will be the last to print
            if (k != activeWin && expPanels[k] != null) {
                 canvas = expPanels[k].getCanvas();
                 if (canvas != null && canvas.getPrintMode()) {
                      if (!canvas.isPrinting())
                          break;
                 }
            }
            canvas = null;
        }
        if (canvas == null)
            canvas = expPanels[activeWin].getCanvas();
        if (canvas != null && canvas.getPrintMode()) {
            if (!canvas.isPrinting())
                canvas.printCanvas();
            return true;
        }
        return false;
    }

    // canvas finished print
    public void finishPrintCanvas(int id) {
        if (expPanels[id] != null) {
            VnmrCanvas canvas = expPanels[id].getCanvas();
            // clear canvas print flag
            if (canvas != null) {
                if (bTransparent && canvas.getPrintMode()) {
                   BufferedImage img = canvas.getPrintImage();
                   if (img != null) {
                      prtGc.drawImage(img, 0, 0, imgW, imgH,
                             0, 0, imgW, imgH, null);
                   }
                }
                canvas.setPrintMode(false);
            }
        }
        if (printNextCanvas())
            return;

        if (plotImg != null) {
            if (bPrtPreview) {
                previewImage();
                return;
            }
            if (inPrintFrame)
                printFrames();
            else
                printImage();
        }
        inPrintMode = false;
        aipCols = 0;
        aipRows = 0;
    }

    // print screen
    public void printWindow(int id, String f, Dimension dim, Point pt) {
        if (robot == null) {
            try
            {
                robot = new Robot();
            }
            catch (Exception e)
            {
                Messages.writeStackTrace(e);
                Messages.postDebug(e.toString());
                return;
            }
        }
        prtFile = f;
        plotImg = robot.createScreenCapture(new Rectangle(pt, dim));
        if (plotImg == null)
            return;
        if (bPrtPreview) {
            vnmrjImg = plotImg;
            imgW = plotImg.getWidth();
            imgH = plotImg.getHeight();
            previewImage();
            return;
        }
        printImage();
        vnmrjImg = null;
    }

    private void printCanvas() {
        int x, y, dw, dh, k;
        int c, th, cnum;
        Graphics2D  gc;
        VnmrCanvas  canvas;
        int xRows = curRows;
        int xCols = curCols;

        if (plotImg == null)
            return;
        if (printTimer == null) {
             ActionListener printAction = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                     stopPrintcmd();
                }
             };
             printTimer = new Timer(180000, printAction);
             printTimer.setRepeats(false);
        }
        printTimer.restart();

        if (!bPrtPreview )
            Messages.postInfo("Creating image.");
        if (bOverlayed) {
            xRows = 1;
            xCols = 1;
        }
        int w = imgW - (xCols -1) * 2;
        int h = imgH - (xRows -1) * 2;
        w = w / xCols;
        h = h / xRows;
        dw = imgW - w * xCols;
        dh = imgH - h * xRows;

        JComponent vjmol = null;
        th = 0;
        if (activeExp != null) {
            vjmol = activeExp.getVJMol();
            th = activeExp.getTitleBarHeight();
        }

        bTransparent = false;
        cnum = 0;
        for (k = 0; k < maxViews; k++) {
             if (expPanels[k] != null) {
                 canvas = expPanels[k].getCanvas();
                 // if (expSelected[k] && expPanels[k].isVisible()) {
                 if (canvas != null) {
                     if (canvas.isShowing()) {
                         canvas.setPrintMode(true);
                         cnum++;
                     }
                     else
                         canvas.setPrintMode(false);
                 }
             }
        }
        if (cnum <= 0)
             return;

        if (bOverlayed || (curCols * curRows == 1)) {
            if (vjmol != null && vjmol.isVisible() && m_vjmolClass != null)
            {
                vjmol.setBounds(0, th, w, h);
                vjmolMethod(vjmol, "updateSize", null, null);
            }
            if (cnum > 1)
                bTransparent = true;
            for (k = 0; k < maxViews; k++) {
                if (expPanels[k] != null) {
                    canvas = expPanels[k].getCanvas();
                    if (canvas != null && canvas.getPrintMode()) {
                        gc = plotImg.createGraphics();
                        gc.setBackground(prtBgColor);
                        gc.setFont(prtFont);
                        canvas.setPrintEnv(0, 0, w, h, prtLw, gc, bPrtColor,
                             bTransparent, bJavaPrint, plotTransImg);
                    }
                }
            }
            printNextCanvas();
            return;
        }

        k = 0;
        y = 0;
        c = 0;
        y = 0;
        x = 0;
        dw = imgW - w * xCols;
        for (k = 0; k < maxViews; k++) {
            if (expPanels[k] != null) {
                canvas = expPanels[k].getCanvas();
                if (canvas != null && canvas.getPrintMode()) {
                    gc = plotImg.createGraphics();
                    gc.setBackground(prtBgColor);
                    gc.setFont(prtFont);
                    canvas.setPrintEnv(x, y, w, h, prtLw, gc, bPrtColor,
                           bTransparent, bJavaPrint, plotTransImg);
                    if (k == activeWin && vjmol != null && vjmol.isVisible() &&
                        m_vjmolClass != null)
                    {
                        vjmol.setBounds(x, y+th, w, h);
                        vjmolMethod(vjmol, "updateSize", null, null);
                    } 
                    x = x + w + 2;
                    if (dw > 0) {
                        x++;
                        dw--;
                    }
                    c++;
                    if (c >= curCols) {
                        x = 0;
                        c = 0;
                        dw = imgW - w * xCols;
                        y = y + h + 2;
                        if (dh > 0) {
                            y++;
                            dh--;
                        }
                    }
                }
            }
        }
        printNextCanvas();
    }

    private void stopPrintcmd() {
        if ( bPrtPreview )
            printTimer.restart();
        inPrintMode = false;
        for (int k = 0; k < maxViews; k++) {
            if (expPanels[k] != null) {
                VnmrCanvas canvas = expPanels[k].getCanvas();
                if (canvas != null)
                    canvas.setPrintMode(false);
            }
        }
    }

    private void setupPrintSize() {
        int n;
        float scrDpi = (float)Toolkit.getDefaultToolkit().getScreenResolution();
        prtDpi = 72.0f;
        prtFontH = 12.0f;

        try {
            prtDpi = Float.parseFloat(prtDpiStr);
        }
        catch (NumberFormatException er) {
            prtDpi = scrDpi;
        }
        if (prtDpi < 10.0f)
            prtDpi = scrDpi;
        if (prtDpi > 1400.0f)
            prtDpi = 1400.0f;
        /***
        if (bOverlayed) {
            if (prtDpi > 300.0f)
                prtDpi = 300.0f;
        }
        ***/
        if (pltFormat.equals("pcl")) {
            n = (int) VjPrintUtil.getPclDpi((double) prtDpi);
            prtDpi = (float) n;
        }
        prtDpiStr = Integer.toString((int) prtDpi);
        paperDim = VjPaperMedia.getPaperPS(prtPaper, false);
        if (paperDim == null)
            paperDim = PrinterPaper.getPaperSize(prtPaper);
        if (bJavaPrint) {
            if (paperWidth > 0)
                paperDim.width = paperWidth;
            if (paperHeight > 0)
                paperDim.height = paperHeight;
        }
        else {
            paperLeftMargin = PrinterPaper.getPaperMargin();
            paperRightMargin = paperLeftMargin;
            paperTopMargin = paperLeftMargin;
            paperBottomMargin = paperLeftMargin;
        }
        paperWidth = paperDim.width;
        paperHeight = paperDim.height;
        if (prtOrient != null) {
            if (prtOrient.equalsIgnoreCase(VjPrintDef.LANDSCPAE)) {
                if (paperDim.height > paperDim.width) {
                    paperDim.height = paperWidth;
                    paperDim.width = paperHeight;
                }
            }
            else
                prtOrient = null;
        }
        if (prtSize.equalsIgnoreCase("halfpage") ||
                        prtSize.equalsIgnoreCase("quarterpage")) {
            if (prtSize.equalsIgnoreCase("halfpage")) {
                paperDim.width = (int) ((float)paperDim.width * 0.55f);
                paperDim.height = (int) ((float)paperDim.height * 0.55f);
                prtFontH = prtFontH * 0.8f;
            }
            else {
                paperDim.width = (int) ((float)paperDim.width * 0.4f);
                paperDim.height = (int) ((float)paperDim.height * 0.4f);
                prtFontH = prtFontH * 0.6f;
            }
            paperLeftMargin = 0;
            paperRightMargin = 0;
            paperTopMargin = 0;
            paperBottomMargin = 0;
            prtFontH = prtFontH * 0.8f;
        }
        n = paperDim.width / 3;
        if (paperLeftMargin > n)
            paperLeftMargin = n;
        if (paperRightMargin > n)
            paperRightMargin = n;
        n = paperDim.height / 3;
        if (paperTopMargin > n)
            paperTopMargin = n;
        if (paperBottomMargin > n)
            paperBottomMargin = n;

        prtWidth = paperDim.width - paperLeftMargin - paperRightMargin;
        prtHeight = paperDim.height - paperTopMargin - paperBottomMargin;
    }

    // start canvas print
    private void startPrintCanvas() {
        int   n;
        float f1, f2, fh, fw;
        float scrDpi = (float)Toolkit.getDefaultToolkit().getScreenResolution();
        Dimension dim = getSize();

        setupPrintSize();

        dim.height = dim.height - expPanels[activeWin].getTitleBarHeight();

        float inchW = (float)(prtWidth) / 72.0f;
        float inchH = (float)(prtHeight) / 72.0f;
        if (!bFitPaper) {
            inchH = (float) prtHeight;
            inchW = (float) prtWidth;
            f1 = (float) dim.height / (float) dim.width;
            fw =  inchW * f1;
            f2 = 1.0f;
            while (true) {
               fh = fw * f2;
               if (fh <= inchH)
                  break;
               f2 = f2 - (fh - inchH) / fh;
            }
            prtWidth = (int) (inchW * f2);
            prtHeight = (int) fh;
            inchW = (float)(prtWidth) / 72.0f;
            inchH = (float)(prtHeight) / 72.0f;
        }
        if (bPrtPreview) {
            Dimension dimScr = Toolkit.getDefaultToolkit().getScreenSize();
            float r1, r2;
            float dw, dh;
            float pw, ph;
            float sh;
            float sw = (float) dimScr.width * 0.4f;
            if (dimScr.height > 1000)
                sh = (float) (dimScr.height - 400);
            else
                sh = (float) (dimScr.height - 280);

            if (sh < 500)
                sh = (float) (dimScr.height - 120);
            f1 = scrDpi;
            pw = (float) paperDim.width / 72.0f;
            ph = (float) paperDim.height / 72.0f;
            while (true) {
                 dw = pw * f1;
                 dh = ph * f1;
                 if ((dh <= sh) && (dw <= sw))
                    break;
                 if (dh > sh)
                    r1 = (dh - sh) / dh;
                 else
                    r1 = 0f;
                 if (dw > sw)
                    r2 = (dw - sw) / dw;
                 else
                    r2 = 0f;
                 if (r1 > r2)
                    f1 = f1 - f1 * r1;
                 else
                    f1 = f1 - f1 * r2;
                 if (f1 < 20)
                    break;
            }
            prtDpi = f1;
        }
        bTransparent = false;
        plotTransImg = null;
        int xRows = curRows;
        int xCols = curCols;
        if (bOverlayed) {
            xRows = 1;
            xCols = 1;
        }
        float orgDpi = prtDpi;
        while (true) {
            imgW = (int) (inchW * prtDpi);
            imgH = (int) (inchH * prtDpi);
            int tw = imgW / xCols;
            int th = imgH / xRows;
            try {
               plotImg = new BufferedImage(imgW, imgH, BufferedImage.TYPE_INT_RGB);
               plotTransImg = new BufferedImage(tw, th, BufferedImage.TYPE_INT_ARGB);
            }
            catch (OutOfMemoryError e) {
               plotImg = null;
               plotTransImg = null;
            }
            catch (Exception ex) {
               plotImg = null;
               plotTransImg = null;
            }
            if (plotImg != null || prtDpi < 10)
                break;
            n = (int) prtDpi;
            n = n * 4 / 5;
            prtDpi = (float) n;
        }
        if (plotImg == null) {
            inPrintMode = false;
            Messages.postError("Could not allocate enough memory for image.");
            return;
        }
        if (prtDpi < orgDpi)
            Messages.postInfo("Insufficient memory, image resolution was reduced to "+prtDpi+".");

        inPrintMode = true;
        prtDpiStr = Integer.toString((int) prtDpi);
        if (!bJavaPrint) {
            if (bPrtColor) {
                for (n = 0; n < expNum; n++) {
                   if (expPanels[n] != null && expPanels[n].isVisible()) {
                      prtBgColor = expPanels[n].getBackground();
                      break;
                   }
                }
            }
            else
                 prtBgColor = Color.white;
            prtFgColor = Color.black;
        }
        prtGc = plotImg.createGraphics();
        prtGc.setBackground(prtBgColor);
        prtGc.clearRect(0, 0, imgW, imgH);
        prtFont = null;
        VnmrCanvas canvas = expPanels[activeWin].getCanvas();
        if (canvas != null)
            prtFont = canvas.getDefaultFont();
        if (prtFont == null)
            prtFont = prtGc.getFont();
        prtFontH = prtFont.getSize2D();
        f1 = prtDpi / scrDpi;
        prtFontH = prtFontH * f1;
        if (prtFontH < 6.0f)
            prtFontH = 6.0f;
        prtFont = prtFont.deriveFont(prtFontH);
        prtGc.setFont(prtFont);

        printCanvas();
    }

    private void startPrintFrames(int expId) {
        if (prtFrames == null)
              return;
        int k, th;
        int w = 20;
        int h = 20;
        int nFrames = 0;
        QuotedStringTokenizer tok = new QuotedStringTokenizer(prtFrames);
        if (aipCols < 1)
            aipCols = 1;
        if (aipRows < 1)
            aipRows = 1;
        if (tok.hasMoreTokens()) {
            try {
                nFrames = Integer.parseInt(tok.nextToken());
            }
            catch (NumberFormatException er) {
                Messages.postError("Error argument: "+prtFrames);
                return;
            }
        }
        if (nFrames <= 0) {
            if ((aipRows * aipCols) > 1) {
                Messages.postError("No frames selected.");
                return;
            }
            nFrames = 1;
        }
        if (nFrames > (aipRows * aipCols)) {
            Messages.postError("Error argument: "+prtFrames);
            return;
        }
        VnmrCanvas canvas = expPanels[expId].getCanvas();
        if (canvas == null)
            return;

        aipPrtId = expId;
        inPrintMode = true;
        bTransparent = false;
        Dimension dim0 = canvas.getSize();
        boolean bError = false;
        while (tok.hasMoreTokens())
        {
             tok.nextToken();  // skip x
             if (!tok.hasMoreTokens()) {
                 bError = true;
                 break;
             }
             tok.nextToken();  // skip y
             if (!tok.hasMoreTokens()) {
                 bError = true;
                 break;
             }
             try {
                 k = Integer.parseInt(tok.nextToken());
                 if ( k > w)
                     w = k;
                 if (!tok.hasMoreTokens()) {
                     bError = true;
                     break;
                 }
                 k = Integer.parseInt(tok.nextToken());
                 if (k > h)
                     h = k;
             }
             catch (NumberFormatException er) {
                 bError = true;
                 break;
             }
        }
        if (bError) {
             inPrintMode = false;
             Messages.postError("Error argument: "+prtFrames);
             return;
        }
        float r = (float) w / (float) h;
        if (r < 0.01f)
            r = 0.1f;
        k = 160; // minimum size
        if (aipRows < 2 && aipCols < 2) {
             if (dim0.width > dim0.height) {
                if (dim0.height > k)
                    k = dim0.height - 2;
             }
             else {
                if (dim0.width > k)
                    k = dim0.width - 2;
             }
        }
        if (w < k || h < k) {
             if (w < h) {
                 w = k;
                 h = (int) ((float) w / r);
             }
             else {
                 h = k;
                 w = (int) ((float) h * r);
             }
        }
        imgW = aipCols * w + aipCols * 4;
        imgH = aipRows * h + aipRows * 4;
        plotImg = null;
        while (true) {
            try {
                plotImg = new BufferedImage(imgW, imgH, BufferedImage.TYPE_INT_RGB);
                plotTransImg = new BufferedImage(imgW, imgH, BufferedImage.TYPE_INT_RGB);
            }
            catch (OutOfMemoryError e) {
                plotImg = null;
            }
            catch (Exception ex) {
                plotImg = null;
            }
            if (plotImg != null)
                break;
            if (w < 40)
                break;
            w = w * 3 / 4;
            h = (int) ((float) w / r);
            imgW = aipCols * w + aipCols * 4;
            imgH = aipRows * h + aipRows * 4;
        }
        if (plotImg == null) {
            aipCols = 1;
            plotTransImg = null;
            inPrintMode = false;
            Messages.postError("Could not allocate memory for image.");
            return;
        }
        aipFrameWidth = w;
        aipFrameHeight = h;
        inPrintFrame = true;
        prtGc = plotImg.createGraphics();
        if (bPrtColor) {
            if (expPanels[expId].isVisible())
               prtGc.setBackground(expPanels[expId].getBackground());
            else {
               for (k = expId; k < expNum; k++) {
                  if (expPanels[k] != null && expPanels[k].isVisible()) {
                     prtGc.setBackground(expPanels[k].getBackground());
                     break;
                  }
               }
            }
        }
        else
             prtGc.setBackground(Color.white);
        prtGc.clearRect(0, 0, imgW, imgH);
        prtGc.setFont(canvas.getFont());
        JComponent vjmol = null;
        th = 0;
        if (activeExp != null) {
            vjmol = activeExp.getVJMol();
            th = activeExp.getTitleBarHeight();
        }
        for (k = 0; k < maxViews; k++) {
            if (expPanels[k] != null) {
                canvas = expPanels[k].getCanvas();
                canvas.setPrintMode(false);
            }
        }

        if (printTimer == null) {
             ActionListener printAction = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                     stopPrintcmd();
                }
             };
             printTimer = new Timer(480000, printAction); // 8 minutes
             printTimer.setRepeats(false);
        }
        printTimer.restart();
        canvas = expPanels[expId].getCanvas();
        canvas.setPrintMode(true);
        canvas.setPrintEnv(0, 0, imgW, imgH, prtLw, prtGc, bPrtColor, false,
                            bJavaPrint, plotTransImg);
        if (vjmol != null && vjmol.isVisible() && m_vjmolClass != null)
        {
              vjmol.setBounds(0, th, imgW, imgH);
              vjmolMethod(vjmol, "updateSize", null, null);
        }
        canvas.printCanvas();
    }

    private void parseVbgPrintcmd(QuotedStringTokenizer tok)
    {
        String data;
        String copies = null;
        String fileName = null;
        String printer = null;
        while (tok.hasMoreTokens())
        {
            data = tok.nextToken();
            if (data.equals("copies:")) {
                if (tok.hasMoreTokens())
                    copies = tok.nextToken();
                continue;
            }
            if (data.equals("file:")) {
                if (tok.hasMoreTokens())
                    fileName = tok.nextToken();
                continue;
            }
            if (data.equals("printer:")) {
                if (tok.hasMoreTokens())
                    printer = tok.nextToken();
                continue;
            }
        }
        if (fileName == null)
            return;
        if (copies == null)
            copies = "1";
        if (printer == null) {
            printer = prtPlotter;
        }

        String cmd = "lpr "; 
        if (System.getProperty("os.name").equals("Linux")) {
           if (printer != null && (printer.length() > 0))
              cmd = "lpr -P "+printer;
           String[] cmds = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, 
                     cmd+ " -# "+copies+" \""+fileName+"\""};
           WUtil.runScript(cmds);
        }
        else {
           if (printer != null && (printer.length() > 0))
              cmd = "lp -d "+printer;
           else
              cmd = "lp ";
           String[] cmds2 = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, 
                     cmd + " -n"+copies+" \""+fileName+"\""};
           WUtil.runScript(cmds2);
        }

        appIf.appendMessage("Plotting started.");
        Messages.postMessage("ow Plotting started.");
        WUtil.deleteFile(fileName);
    }

    private boolean parsePrintcmd(QuotedStringTokenizer tok)
    {
        boolean doPrint = false;
        String data;
        String xdata = null;
        while (tok.hasMoreTokens() || xdata != null)
        {
            if (xdata != null) {
                data = xdata;
                xdata = null;
            }
            else
                data = tok.nextToken();
            if (data.equals("exec")) {
                doPrint = true;
                continue;
            }
            if (data.equals("graphics")) {
                bRunPrtScrnPopup = true;
                continue;
            }
            if (data.equals("toprinter")) {
                parseVbgPrintcmd(tok);
                bRunPrtScrnPopup = false;
                return false;
            }
            if (data.startsWith("region:")) {
                prtRegion = data.substring(7);
                continue;
            }
            if (data.startsWith("layout:")) {
                prtOrient = data.substring(7);
                continue;
            }
            if (data.startsWith("file:")) {
                prtFile = data.substring(5);
                continue;
            }
            if (data.startsWith("format:")) {
                if (data.length() > 7)
                   prtFormat = data.substring(7);
                continue;
            }
            if (data.startsWith("pformat:")) {
                if (data.length() > 8)
                   prtFormat = data.substring(8);
                continue;
            }
            if (data.startsWith("iformat:")) {
                if (data.length() > 8)
                   imgFormat = data.substring(8);
                continue;
            }
            if (data.startsWith("dpi:")) {
                prtDpiStr = data.substring(4);
                continue;
            }
            if (data.startsWith("send:")) {
                data = data.substring(5);
                bPlot = (data.equalsIgnoreCase("print")) ? true : false;
                continue;
            }
            if (data.startsWith("paper:")) {
                prtPaper = data.substring(6);
                continue;
            }
            if (data.startsWith("size:")) {
                prtSize = data.substring(5);
                continue;
            }
            if (data.startsWith("plotter:")) {
                prtPlotter = data.substring(8);
                prtPlotter=prtPlotter.replace("\\", "\\\\");
                continue;
            }
            if (data.startsWith("color:")) {
                data = data.substring(6);
                bPrtColor = (data.equalsIgnoreCase("color")) ? true : false;
                continue;
            }
            if (data.startsWith("lw:")) {
                if (data.length() > 3) {
                   data = data.substring(3);
                   try {
                      prtLw = Integer.parseInt(data);
                   }
                   catch (NumberFormatException er) { prtLw = 1; }
                   if (prtLw > 60)
                      prtLw = 60;
                }
                continue;
            }
            if (data.startsWith("aiprows:")) {
                if (data.length() > 8) {
                   data = data.substring(8);
                   try {
                      aipRows = Integer.parseInt(data);
                   }
                   catch (NumberFormatException er) { aipRows = 1; }
                }
                if (aipRows < 1)
                   aipRows = 1;
                continue;
            }
            if (data.startsWith("aipcols:")) {
                if (data.length() > 8) {
                   data = data.substring(8);
                   try {
                      aipCols = Integer.parseInt(data);
                   }
                   catch (NumberFormatException er) { aipCols = 1; }
                   if (aipCols < 1)
                      aipCols = 1;
                }
                continue;
            }
            if (data.startsWith("device:")) {
                data = data.substring(7);
                bPlot = (data.equalsIgnoreCase("print")) ? true : false;
                continue;
            }
            if (data.startsWith("frames:"))
            {
                prtFrames = data.substring(7);
                StringBuffer sbData = new StringBuffer().append(prtFrames);
                while (tok.hasMoreTokens())
                {
                    String d = tok.nextToken();
                    char   c = d.charAt(0);
                    if (d.indexOf(':') > 0 || (c < '0' || c > '9')) {
                        xdata = d;
                        break;
                    }
                    sbData.append(" ").append(d);
                }
                prtFrames = sbData.toString();
            }
        }

        if (!bPlot && imgFormat != null)
             pltFormat = imgFormat;
        else
             pltFormat = prtFormat;
        if (pltFormat == null) {
             if (bPlot)
                  pltFormat = "ps";
             else
                  pltFormat = "jpg";
        }
        pltFormat = pltFormat.toLowerCase();
        if (pltFormat.equalsIgnoreCase("jpeg"))
             pltFormat = "jpg";
        return doPrint;
    }

    public boolean processPrintcmd(String cmd)
    {
        if (cmd == null)
            return true;
        QuotedStringTokenizer strTok = new QuotedStringTokenizer(cmd, " ,\n");
        bPlot = false;
        imgFormat = null;
        prtRegion = "graphics";
        prtFrames = null;
        prtFile = null;
        bFitPaper = false;
        bPrtPreview = false;
        parsePrintcmd(strTok);

        if (prtFile == null || prtFile.length() < 1)
            prtFile = new StringBuffer().append("plot").append(File.separator).append("img").append(System.currentTimeMillis()).toString();
        else if (prtFile.indexOf(File.separator) < 0)
            prtFile = new StringBuffer().append("plot").append(File.separator).append(prtFile).toString();

        if (prtRegion.indexOf("frames") >= 0 && prtFrames != null) {
            return false;
        }
        else if (prtRegion.equalsIgnoreCase("vnmrj")) {
            return false;
        }

        startPrintCanvas();
        return true;
    }

    public void execPrintcmd(QuotedStringTokenizer tok, int expId) {
        String data;

        if (inPrintMode && !bRunPrtScrnPopup)
            return;
        bRunPrtScrnPopup = false;
        boolean doPrint = parsePrintcmd(tok);
        if (bRunPrtScrnPopup)
        {
            openPrintScreenPopup();
            return;
        }
        if (inPrintMode)
            return;

        if (!doPrint) {
            return;
        }
        inPrintFrame = false;
        bJavaPrint = false;
        bFitPaper = false;
        bPrtPreview = false;

        if (bPlot || prtFile == null || prtFile.length() < 1)
            prtFile = new StringBuffer().append("plot").append(File.separator).append("tmp").append(System.currentTimeMillis()).toString();
        else if (prtFile.indexOf(File.separator) < 0)
            prtFile = new StringBuffer().append("plot").append(File.separator).append(prtFile).toString();
        if (prtRegion.indexOf("frames") >= 0 && prtFrames != null) {
           // data = new StringBuffer().append("region:").append(prtRegion).
           //      append(" frames:").append(prtFrames).append(" file:").append(prtFile).toString();
           // if (expPanels[expId] != null)
           //    expPanels[expId].execPrintCmd(data);
           data = new StringBuffer().append("region:").append(prtRegion).
                append(" file:").append(prtFile).append(" frames:").append(prtFrames).toString();

           startPrintFrames(expId);
           return;
        }
        if (prtRegion.equalsIgnoreCase("vnmrj")) {
           data = new StringBuffer().append("region:").append(prtRegion).
                   append(" file:").append(prtFile).toString();
           if (expPanels[expId] != null)
               expPanels[expId].execPrintCmd(data);
           return;
        }
        startPrintCanvas();
    }

    class PanelLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
            //unused
        } //preferredLayoutSize()
        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
            //unused
        } //minimumLayoutSize()
        public void layoutContainer(Container target) {
/*
            if (vpManager != null && target.isShowing()) {
                Point myLoc = getLocationOnScreen();
                vpManager.setViewLoc(myLoc);
            }
*/
            if (appIf.isResizing() || inSmallLarge) {
                return;
            }
            synchronized(target.getTreeLock()) {
                resizeCanvas();
            }
        }
    }

    class VxRepaintManager extends RepaintManager {
        private int viewH = 0;
        private int viewH2 = 0;
        private AppIF appIf1;
        private Point viewPt = new Point(0, 0);
        private int  dh;
        private int  sy;

        public void setViewMargin(int h) {
            if (h > viewH) {
               viewH = h;
               viewH2 = h + 2;
            }
        }

        public void setAppIf(AppIF f) {
            appIf1 = f;
        }

        public void setViewLoc(Point pt) {
            viewPt.x = pt.x;
            viewPt.y = pt.y;
            sy = pt.y;
        }

        public synchronized void addDirtyRegion(JComponent c,
             int x, int y, int w, int h) {
             if (c instanceof ExpViewArea) {
                if (!c.isShowing())
                    return;
                Point pt = c.getLocationOnScreen();
                if (appIf1.isResizing()) {
                    if (pt.y > sy) {
                           dh = viewH;
                           sy = pt.y;
                    }
                    else {
                           dh = sy - pt.y + viewH2;
                           sy = pt.y;
                    }
                    if (dh < h)
                        h = dh;
                }
                else
                    sy = viewPt.y;
              }
              super.addDirtyRegion(c,x,y,w,h);
        }
    } // VxRepaintManager

    public SessionShare getSessionShare() {
        return sshare;
    }

    private Float getFloat(String key)
    {
        Object obj = sshare.getProperty(key);
        if (obj == null || !(obj instanceof Float))
             return null;
        return (Float) obj;
    }

    private String getString(String key)
    {
        Object obj = sshare.getProperty(key);
        if (obj == null || !(obj instanceof String))
             return null;
        return (String) obj;
    }

    private void printVnmrj() {
        if (activeExp == null)
           return;
        if (prtFile == null)
            prtFile = new StringBuffer().append("plot").append(File.separator).append("print").append(System.currentTimeMillis()).toString();
        if (bPrtPreview || vnmrjImg == null) {
            String data = new StringBuffer().append("region:").append("vnmrj").
                   append(" format:").append(prtFormat).
                   append(" file:").append(prtFile).toString();
            VjPrintPreview.closeDialog();
            if (vjPrtService != null)
                vjPrtService.closeDialog();
            activeExp.plotCmd = data;
            activeExp.printVnmrj();
            return;
        }
        plotImg = vnmrjImg;
        printImage();
        vnmrjImg = null;
    }


    private boolean configPrintScreen(boolean allowPreview) {
        bPrtPreview = false;
        Float fv;
        bFitPaper = false;
        bPrtColor = true;
        prtFgColor = Color.black;
        prtBgColor = Color.white;
        prtOrient = null;
        prtDpiStr = "300";
        prtPaper = "letter";
        prtFormat = "ps";
        prtSize = "full";
        paperLeftMargin = 40;
        paperTopMargin = -1;
        paperWidth = 0;
        paperHeight = 0;
        prtCopies = 1;
        bPrintToFile = false;
        prtFile = null;
        bPrtVnmrj = false;

        String mess = getString(VjPrintDef.DESTIONATION);
        if (mess != null && (mess.equals(VjPrintDef.FILE)))
            bPrintToFile = true;
        mess = getString(VjPrintDef.PRINT_AREA);
        if (mess != null && (mess.equals(VjPrintDef.VNMRJ)))
            bPrtVnmrj = true;
        bMMMargin = false;
        mess = getString(VjPrintDef.PRINT_MARGIN_UNIT);
        if (mess != null && (mess.equals(VjPrintDef.MM)))
            bMMMargin = true;

        if (bPrintToFile) {
            bPlot = false;
            // prtTmpFile = (String) sshare.getProperty(VjPrintDef.FILE_TMPNAME);
            prtFile = getString(VjPrintDef.FILE_NAME);
            mess = getString(VjPrintDef.FILE_RESHAPE);
            if (mess != null &&  (mess.equals(VjPrintDef.YES)))
                 bFitPaper = true;
            mess = getString(VjPrintDef.FILE_COLOR);
            if (mess != null &&  (mess.equals(VjPrintDef.MONO)))
                 bPrtColor = false;
            if (allowPreview) {
                mess = getString(VjPrintDef.FILE_PREVIEW);
                if (mess != null && (mess.equals(VjPrintDef.YES)))
                     bPrtPreview = true;
            }
            mess = getString(VjPrintDef.FILE_FG);
            if (mess != null)
                 prtFgColor = DisplayOptions.getColor(mess);
            mess = getString(VjPrintDef.FILE_BG);
            if (mess != null)
                 prtBgColor = DisplayOptions.getColor(mess);
            mess = getString(VjPrintDef.FILE_ORIENTATION);
            if (mess != null)
                 prtOrient = mess;
            mess = getString(VjPrintDef.FILE_RESOLUTION);
            if (mess != null)
                 prtDpiStr = mess;
            mess = getString(VjPrintDef.FILE_PAPER);
            if (mess != null)
                 prtPaper = mess;
            mess = getString(VjPrintDef.FILE_FORMAT);
            if (mess != null)
                 prtFormat = mess;
            fv = getFloat(VjPrintDef.FILE_WIDTH);
            if (fv != null)
                 paperWidth = fv.intValue();
            fv = getFloat(VjPrintDef.FILE_HEIGHT);
            if (fv != null)
                 paperHeight = fv.intValue();
            if (prtFile == null && !bPrtPreview)
                return false;
        }
        else {
            bPlot = true;
            mess = getString(VjPrintDef.PRINT_RESHAPE);
            if (mess != null &&  (mess.equals(VjPrintDef.YES)))
                 bFitPaper = true;
            mess = getString(VjPrintDef.PRINT_COLOR);
            if (mess != null &&  (mess.equals(VjPrintDef.MONO)))
                 bPrtColor = false;
            if (allowPreview) {
                 mess = getString(VjPrintDef.PRINT_PREVIEW);
                 if (mess != null &&  (mess.equals(VjPrintDef.YES)))
                     bPrtPreview = true;
            }
            mess = getString(VjPrintDef.PRINT_FG);
            if (mess != null)
                 prtFgColor = DisplayOptions.getColor(mess);
            mess = getString(VjPrintDef.PRINT_BG);
            if (mess != null)
                 prtBgColor = DisplayOptions.getColor(mess);
            mess = getString(VjPrintDef.PRINT_RESOLUTION);
            if (mess != null)
                 prtDpiStr = mess;
            mess = getString(VjPrintDef.PRINT_PAPER);
            if (mess != null)
                 prtPaper = mess;
            mess = getString(VjPrintDef.PRINT_FORMAT);
            if (mess != null)
                 prtFormat = mess;
            fv = getFloat(VjPrintDef.PRINT_WIDTH);
            if (fv != null)
                 paperWidth = fv.intValue();
            fv = getFloat(VjPrintDef.PRINT_HEIGHT);
            if (fv != null)
                 paperHeight = fv.intValue();
            fv = getFloat(VjPrintDef.PRINT_COPIES);
            if (fv != null)
                 prtCopies = fv.intValue();
            if (!bPrtPreview)
                 prtFile = new StringBuffer().append("plot").append(File.separator).append("print").append(System.currentTimeMillis()).toString();
        }
        prtPlotter = (String) sshare.getProperty(VjPrintDef.PRINTER_NAME);
        // windows plotter names can have \ chars which need to be "escaped" in order to work with c-strings
	if (prtPlotter != null && (prtPlotter.length() > 0))
           prtPlotter=prtPlotter.replace("\\", "\\\\");

        if (prtOrient == null) { 
            mess = (String) sshare.getProperty(VjPrintDef.PRINT_ORIENTATION);
            if (mess != null)
                 prtOrient = mess;
        }

        if (paperTopMargin < 0) {
            paperLeftMargin = 40;
            paperRightMargin = 40;
            paperTopMargin = 40;
            paperBottomMargin = 40;
            float funit = 72.0f;
            if (bMMMargin)
               funit = 72.0f/ 25.4f;
            fv = getFloat(VjPrintDef.PRINT_TOP_MARGIN);
            if (fv != null) {
               paperTopMargin = (int) (fv.floatValue() * funit);
            }
            fv = getFloat(VjPrintDef.PRINT_BOTTOM_MARGIN);
            if (fv != null)
               paperBottomMargin = (int) (fv.floatValue() * funit);
            fv = getFloat(VjPrintDef.PRINT_LEFT_MARGIN);
            if (fv != null)
               paperLeftMargin = (int) (fv.floatValue() * funit);
            fv = getFloat(VjPrintDef.PRINT_RIGHT_MARGIN);
            if (fv != null)
               paperRightMargin = (int) (fv.floatValue() * funit);
        }

        pltFormat = prtFormat.toLowerCase();
        if (!bPrtColor)
            prtBgColor = Color.white;

        return true;
    }
    
    // exec cmd from VjPrintService (print screen)
    //  print vnmrj window: setupPrintSize, printVnmrj
    //  print canvas: startPrintCanvas, setupPrintSize, printCanvas
    private void execPrintScreen(boolean allowPreview) {
         if (!configPrintScreen(allowPreview))
             return;
         if (bPrtVnmrj) {
             prtDpiStr = "72";
             setupPrintSize();
             printVnmrj();
             return;
         }
         vnmrjImg = null;
         inPrintMode = true;
         bJavaPrint = true;
         plotImg = null;
         plotTransImg = null;
         inPrintFrame = false;
         startPrintCanvas();
    }

    public int print(Graphics g, PageFormat pf, int pageIndex)
    {
         if (pageIndex != 0) {
             return Printable.NO_SUCH_PAGE;
         }
         prtGc = (Graphics2D) g;
         imgW = (int)pf.getImageableWidth();
         imgH = (int)pf.getImageableHeight();
         int px = (int)pf.getImageableX();
         int py = (int)pf.getImageableY();

         execPrintScreen(true);

         prtGc.translate(px, py);
         prtGc.setBackground(prtBgColor);
         prtGc.clearRect(0, 0, imgW, imgH);
              
         prtGc.setColor(Color.red);
         prtGc.drawRect(0, 0, imgW, imgH);
         prtGc.setColor(Color.cyan);
         prtGc.drawRect(2, 2, imgW - 4, imgH - 4);
             
         prtGc.setColor(prtFgColor);
         printCanvas();
         try {
             while (true) {
                 if (inPrintMode) {
                    Thread.yield();
                 }
                 else
                 {
                    break;
                 }
             }
         }
         catch (Exception e) { }
         return Printable.PAGE_EXISTS;
    }

    private void openPrintScreenPopup()
    {
         if (vjPrtService == null)
         {
             vjPrtService = new VjPrintService(this);
             vjPrtService.setPrintEventListener(this);
         }
         if (vjPrtService != null)
             vjPrtService.showDialog();
    }

    public void printEventPerformed(VjPrintEvent e) {
         inPrintMode = false;
         aipCols = 1;
         aipRows = 1;
         VjPrintPreview.setPreviewImage(null);
         int status = e.getStatus();
         if (status == VjPrintDef.PRINT) {
             execPrintScreen(true);
             if (!bPrtPreview)
                 VjPrintPreview.closeDialog();
             return;
         }
         if (status == VjPrintDef.PREVIEW_CANCEL) {
             bPrtPreview = false;
             return;
         }
         if (status == VjPrintDef.PREVIEW_APPROVE) { // from preview dialog
             String fname = null;
             if (bPrintToFile) {
                 fname = getPrintFileName();
                 Component comp = VjPrintService.getTopDialog();
                 if (comp == null)
                     return;

                 if (fname == null || fname.length() < 1) {
                      JOptionPane.showMessageDialog(comp,
                         "File name entry is empty. Please enter file name",
                         "Print To File",
                         JOptionPane.WARNING_MESSAGE);
                      return;
                  }

                 fname = FileUtil.openPath(fname);
                 if (fname != null) {
                      int ret = JOptionPane.showConfirmDialog(comp,
                         "File: "+fname+".\n"+
                         "This file already exists. Would you like to overwrite the existing file?",
                         "Print To File",
                         JOptionPane.YES_NO_OPTION);
                      if (ret != JOptionPane.YES_OPTION)
                         return;
                 }
             }
             execPrintScreen(false);
             VjPrintPreview.closeDialog();
             vjPrtService.closeDialog();
             return;
         }
     }

} // class ExpViewArea

