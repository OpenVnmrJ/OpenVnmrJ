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
import java.awt.datatransfer.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import java.awt.image.BufferedImage;
import java.awt.image.IndexColorModel;
import java.awt.geom.AffineTransform;
import javax.imageio.*;
import java.awt.RenderingHints;


import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;
import vnmr.ui.shuf.*;
import vnmr.print.VjPrintDef;

/**
 * The graphics windows. A key responsibility is to
 * receive drop events.
 */

public class VnmrCanvas extends JLayeredPane implements CanvasIF, VGaphDef,
        VnmrKey, DragGestureListener, DragSourceListener {
    /** session share */
    private SessionShare sshare;
    private BufferedImage bimg = null;
    private BufferedImage b2img = null;
    private boolean bBacking = false;
    private boolean isBusy = false;
    private boolean bActive = true;
    private boolean sendMore = false;
    private boolean bBatch = false;
    private boolean bMacOs = false;
    private boolean bXor = false;
    private boolean bDraging = false;
    private boolean isResizing = false;
    private boolean repaintDisplatte = false;
    private boolean bResized = false;
    private boolean bPrintMode = false;
    private boolean bPrinting = false;
    private boolean bNative = false; // java or native( X ) graphics
    private boolean bSuspend = false;
    private boolean bReshow = false;
    private boolean bWaitOnPaint = false;
    private boolean bDrgTest = false;
    private boolean bDrgUp = false;
    private boolean bOverlayed = false;
    private boolean bBottomLayer = false;
    private boolean bTopLayer = true;
    private boolean bXorOvlay = false;
    private boolean bSyncOvlay = false;
    private boolean bDispCursor = true;
    private boolean bColorPaint = true;
    private boolean bMoveFrame = false;
    private boolean bPrtCanvas = false;
    private boolean bPrintService = false;
    private boolean bShowFrameBox = false;
    private boolean bFrameAct = false;
    private boolean bShowMoveBorder = false;
    private boolean bLinux = false;
    private boolean b3planes = false;
    private boolean bRexec = false;
    private boolean m_bRepaintOrient = true;
    private boolean m_bUpdateOrient = true;
    private boolean bVerbose = false;
    private boolean bInTrashCan = false;
    private boolean bMousePressed = false;
    private boolean bObjDrag = false;
    private boolean bObjMove = false;
    private boolean bObjResize = false;
    private boolean bDragInCanvas = false;
    private boolean bXorEnabled = true;
    private boolean bClickInFrame = false;
    private boolean bPaintModeOnly = true;
    private boolean bSaveXorObj = false;
    private boolean bTopFrameOn = false;
    private boolean bGinWait = false;
    private boolean bGinFunc = false;
    private boolean bArgbImg = false; // rgb with alpha
    private boolean bImgBg = true; // use imaging background color
    private boolean bFreeFontColor = true;
    private boolean bSendFontInfo = true;
    private boolean bTopFrameOnTop = true;
    public boolean bImgType = false; // imaging user
    // private boolean bWindows = false;
    private boolean bPainting = false;
    protected boolean m_bSelect = false;
    protected boolean bRubberMode = false;
    protected boolean bRubberArea = false;
    protected boolean bRubberBox = false;
    protected boolean bStartOverlay = false;
    protected boolean bTranslucent = true;
    protected boolean bSeriesLocked = false;
    protected boolean bSeriesChanged = false;
    protected boolean bLwChangeable = true;
    public boolean bCsiWindow = false;
    public boolean bCsiActive = false;
    public boolean bRepaintCsi = false;
    public boolean bThreshold = false;
    private VGraphics graphTool;
    private ButtonIF vnmrIf;
    private byte[] buf;
    private int bufLen = 0;
    private int drag_dcnt = 0;
    private int drag_ecnt = 0;
    private int drag_cnt = 0;
    private int iconNum = 0;
    private int objNum = 0;
    private int slideCount= 0;
    private int[] dconi_xCursor; // vertical line
    private int[] dconi_yCursor; // horizontal line
    private int[] thresholdCursor; // threshold line
    private int[] dconi_xLen;
    private int[] dconi_yLen;
    private int[] dconi_xStart;
    private int[] dconi_yStart;
    private int[] dconi_xColor;
    private int[] dconi_yColor;
    private int[] pv = null;
    private int[] xindex;
    protected XColor[] xcolors;
    protected int dragMouseBut = 0;
    protected int pressedMouseBut = 0;
    protected VIcon m_vIcon = null;
    // private int m_nMousex = 0;
    // private int m_nMousey = 0;
    private int oldX = 0;
    private int oldY = 0;
    private int winX = -1;
    private int winY = -1;
    private int scrX = 0; // position x relative to frame
    private int scrY = 0;
    private int scrX2 = 0;
    private int scrY2 = 0;
    private int dragX = 0; // the drag starting x
    private int dragY = 0;
    private int regionX = 0;
    private int regionX2 = 0;
    private int regionY = 0; // the drag starting x
    private int regionY2 = 0;
    private int mvx, mvy;
    private int icolorId = -3;
    private int colorId = -3;
    private int aipId = 0;
    private int lastAipId = 4;
    private int aipCmpId = -1; // colormap id
    private int butInfo = 0; // button modifier
    private int drgCount = 0;
    private int drgNum = 0;
    private int drgY = 0;
    private int dispType = 0;
    private int m_overlayMode = 0;
    private int printW = 0;
    private int printH = 0;
    private int maxW = 9999;
    private int maxH = 9999;
    private int printTrX = 0;
    private int printTrY = 0;
    private int spLw = 1;
    private int lineLw = 1;
    private int baseLw = 0;
    private int baseSp = 0;
    private int crosshairLw = 1;
    private int cursorLw = 1;
    private int thresholdLw = 1;
    private double drgStart;
    private long wheelStart = 0;
    private Dimension winSize;
    private static boolean syncDrag = true;
    private static int syncval = 2;
    private DropTarget dropTarget;
    private DragSource dragSource;
    private ExpViewArea viewIf = null;
    private Point mousePt = null;
    private Point mousePt2 = null;
    private Point molDropPt = null;
    private Point m_pPrevStart = null;
    private Point m_pPrevEnd;
    private Point loc = new Point(0, 0);
    private Rectangle m_rectSelect = new Rectangle();
    private Rectangle clipRect = null;
    private Rectangle paintClip = null;
    private AppIF appIf = null;
    private VnmrjIF vjIf = null;
    private Color copyColor;
    private Color iColor;
    private Color shadeColor;
    private Color xhairColor;
    private Color borderColor;
    private Color hilitColor;
    private Color hotColor;
    private Color bkRegionColor;
    private Color aColor;
    protected String imgFgName = "null";
    protected String imgBgName = "null";
    private String dragCmd = null;
    private String bk1 = null;
    private String bk2 = null;
    private JFrame jFrame = null;
    private VNMRFrame vjFrame = null;
    private CanvasDropTargetListener dropListener;
    private int annX = 0;
    private int annY = 0;
    private int annW = 0;
    private int annW1 = 0;
    private int annH = 0;
    private int annH1 = 0;
    private int hairX1 = 0;
    private int hairY1 = 0;
    private int hairL1 = 0;
    private int hairX2 = 0;
    private int hairY2 = 0;
    private int hairL2 = 0;
    private int frameX = 0;
    private int frameX2 = 0;
    private int frameY = 0;
    private int frameY2 = 0;
    private int frameW = 0;
    private int frameH = 0;
    private int moveX = 0;
    private int moveX2 = 0;
    private int moveY = 0;
    private int moveY2 = 0;
    private int moveW = 0;
    private int moveH = 0;
    private int orgFrameId = -1;
    private int orgFrameNum = 0;
    private int ginButton = 0;
    private int ginEvent = 0;
    private int cursorIndex = Cursor.DEFAULT_CURSOR;
    private int bkRegionX = 0;
    private int bkRegionY = 0;
    private int bkRegionW = 0;
    private int bkRegionH = 0;
    private long clickWhen = 0;
    private Point framePoint = null;
    private XMap frameMap = null;
    private XMap defaultMap = null;
    private XMap aipMap = null;
    private XMap frame_1 = null;
    private XMap frame_2 = null;
    private XMap printMap = null;
    private JPopupMenu orientPopup = null;
    private JPopupMenu iconPopup = null;
    private JPopupMenu molIconPopup = null;
    private JPopupMenu textBoxPopup = null;
    private static double deg90 = Math.toRadians(90.0);
    private static Cursor trashCursor = DragSource.DefaultMoveDrop;
    private static Cursor noDropCursor = DragSource.DefaultCopyNoDrop;
    private Cursor moveCursor = Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR);
    private static int CURSOR_NUM = 3;
    private static int xnum = 16; // the number of XColor

    private String hairS1 = null;
    private String hairS2 = null;
    private String JFUNC = "jFunc(";
    private String JEVENT = "jEvent(";
    private String JMOVE = "jMove(";
    private String JREGION = "jRegion(";
    private String specMinStr = null;
    private String specMaxStr = null;
    private String specRatioStr = "1.0";
    private String lineThickStr ="1.0";
    private Cursor objCursor = Cursor.getDefaultCursor();
    private VnmrCanvas topCanvas = null;
    private VnmrCanvas prtCanvas = null;
    private VnmrCanvas csiCanvas = null;
    private VnmrCanvas vbgCanvas = null;
    private User user = null;
    private Hashtable<String, Object> hs = null;
    private BasicStroke specStroke = null;
    private BasicStroke lineStroke = null;
    private BasicStroke[] strokeList = null;
    private Stroke defaultStroke = null;
    private static int DEFAULT_CHART = 0;
    private static int DEFAULT_1D = 1;
    private static int DEFAULT_2D = 2;
    private static int ALIGN_1D_X = 3;
    private static int ALIGN_1D_Y = 4;
    private static int ALIGN_2D = 5;
    private static int ALIGN_1D_XY = 6;
    private static int OVERLAY_ALIGN = 3;
    private static int MINWH = 10;
    private static int STROKE_NUM = 9;
    private DragSourceContext ds = null;
    private RQPanel rqPanel = null;
    private CanvasObjIF activeObj = null;
    private CanvasObjIF preActiveObj = null;
    private ReviewQueue rq;
    private Crosshair crosshairs3D[][]=new Crosshair[3][2];

    public int winW = MINWH;
    public int winH = MINWH;
    public int imgGrayNums = 0;
    public int imgGrayStart = 556;
    public int frameId = -1;
    public int frameNum = 0;
    public int bgIndex = 20;
    public int fgIndex = 0;
    public Color imgColor = null;
    public Color bgColor;
    public Color fgColor;
    public Color bgAlpha;
    public Color bgOpaque;
    public BufferedImage printImgBuffer = null;
    public Vector<XMap> xmapVector = null; // for imaging frames
    public Vector<XMap> frameVector = null; // for liquid frames
    public Vector<CanvasObjIF> imgVector = null; // for imaging pictures
    public Vector<CanvasObjIF> tbVector = null; // for textbox
    public Annotation annPan = null;
    private ImagesToJpeg movieMaker;
    protected VColorModel cmpModel;
    protected VColorModel monoModel;
    protected IndexColorModel cmIndex;
    protected IndexColorModel aipCmIndex;
    protected IndexColorModel monoCmIndex;
    protected GraphicsFont graphFont;
    protected GraphicsFont defaultFont;
    private java.util.List<GraphicsFont> graphFontList;
    private java.util.List<GraphSeries> gSeriesList;
    private BufferedImage vtxtImg = null;

    private VJArrow jArrow;

    /**
     * constructor
     * @param sshare session share
     */
    public VnmrCanvas(SessionShare sshare, int id, ButtonIF ap, boolean Xgraph) {
        int i,j,k;

        this.sshare = sshare;
        this.vnmrIf = ap;
        winId = id;

        for(i=0;i<3;i++){
            for(j=0;j<2;j++)
                crosshairs3D[i][j]=new Crosshair();
        }
        bNative = Xgraph; // java or native graphics
        bMacOs = Util.isMacOs();

        String attr = System.getProperty("syncdrag");
        if (attr != null) {
            if (attr.equals("no") || attr.equals("false"))
                syncDrag = false;
            else {
                try {
                    Integer iv = Integer.valueOf(attr);
                    syncval = iv.intValue();
                } catch (NumberFormatException e) {
                }
            }
        }
        bLinux = Util.islinux();
        user = Util.getUser();
        if (user != null) {
            ArrayList<String> a = user.getAppTypeExts();
            for (k = 0; k < a.size(); k++) {
                String s = a.get(k);
                if (s != null && (s.equals(Global.IMAGING))) {
                    bImgType = true;
                    bBacking = true;
                    bImgBg = true;
                }
            }
        }
       /****
        bWindows = Util.iswindows();
        if (bMacOs || bWindows) {
            // setXORMode does not work correctly with transparent background
            //  on MacOs,Windows
            bArgbImg = true;
            this.vnmrIf.sendToVnmr("xoroff(-1)\n");
        }
       ********/
        bPaintModeOnly = true;
        bArgbImg = true;
        this.vnmrIf.sendToVnmr("xoroff(-1)\n");

        xmapVector = new Vector<XMap>(32, 32);
        frameVector = new Vector<XMap>(8, 4);
        frameVector.setSize(8);
        xmapVector.setSize(32);

        hs = sshare.userInfo();
        if (hs != null) {
            Integer ix = (Integer) hs.get("imgGrayNums");
            if (ix != null)
                imgGrayNums = ix.intValue();
            ix = (Integer) hs.get("imgGrayStart");
            if (ix != null)
                imgGrayStart = ix.intValue();
        }

        initParameters();

        setVjColors();

        graphTool = new VGraphics();
        appIf = Util.getAppIF();
        vjIf = Util.getVjIF();

        initEventListeners();
        dragSource = new DragSource();
        dragSource.createDefaultDragGestureRecognizer(VnmrCanvas.this,
                DnDConstants.ACTION_MOVE, VnmrCanvas.this);

        dropListener = new CanvasDropTargetListener();
        dropTarget = new DropTarget(this, dropListener);

        /********
         addKeyListener(new KeyAdapter() {
             public void keyTyped(KeyEvent e) {
                  if (!bActive)
                       return;
                  if (appIf != null)
                      appIf.processXKeyEvent(e);
             }
             public void keyPressed(KeyEvent e) {
                 if (!bActive)
                     return;
                 if (appIf != null)
                     appIf.processXKeyEvent(e);
             }
             public void keyReleased(KeyEvent e) {
             }
        });
         *******/

        setSize(200, 200);
    } // VnmrCanvas()

    public VnmrCanvas(SessionShare sshare, int id, ButtonIF ap) {
        this(sshare, id, ap, true);
    }

    // this constructor is used to create class for print screen
    public VnmrCanvas(int id, ButtonIF ap, boolean imgType, boolean prtType) {
        this.vnmrIf = ap;
        this.winId = id;
        this.bImgType = imgType;
        this.bPrtCanvas = prtType;
         
        bMacOs = Util.isMacOs();
        graphTool = new VGraphics();
        appIf = Util.getAppIF();
        vjIf = Util.getVjIF();
        initParameters();
    }

    private void initStrokeList() {
        if (strokeList != null)
            return;
        strokeList = new BasicStroke[STROKE_NUM + 2];
        strokeList[0] = new BasicStroke(1.0f, BasicStroke.CAP_SQUARE,
                BasicStroke.JOIN_ROUND);
        strokeList[1] = strokeList[0];
        for (int k = 2; k < STROKE_NUM + 2; k++) {
             strokeList[k] = new BasicStroke((float) k,
                        BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND);
        }
        specStroke = strokeList[0];
        lineStroke = strokeList[0];
    }

    private void initParameters() {
        int k;

        redByte = new byte[colorSize];
        grnByte = new byte[colorSize];
        bluByte = new byte[colorSize];
        alphaByte = new byte[colorSize];
        alphaVal = (byte) 255;
        dconi_xCursor = new int[CURSOR_NUM];
        dconi_yCursor = new int[CURSOR_NUM];
        thresholdCursor = new int[CURSOR_NUM];
        dconi_xStart = new int[CURSOR_NUM];
        dconi_yStart = new int[CURSOR_NUM];
        dconi_xLen = new int[CURSOR_NUM];
        dconi_yLen = new int[CURSOR_NUM];
        dconi_xColor = new int[CURSOR_NUM];
        dconi_yColor = new int[CURSOR_NUM];
        xindex = new int[xnum];
        xcolors = new XColor[xnum];
        for (k = 0; k < xnum; k++) {
            xindex[k] = 0;
            xcolors[k] = null;
        }
        for (k = 0; k < colorSize; k++) {
            redByte[k] = 100;
            grnByte[k] = 100;
            bluByte[k] = 0;
            alphaByte[k] = alphaVal;
        }
        alphaByte[bgIndex] = 0;

        pv = new int[12];

        clear_dconi_cursor();

        wingc = (Graphics2D) getGraphics();
        mygc = wingc;
 
        if (mygc == null) {
             vtxtImg = new BufferedImage(10, 10, BufferedImage.TYPE_INT_ARGB);
             vtxtImgHeight = 0;
             if (vtxtImg != null)
                  mygc = vtxtImg.createGraphics();
        }
        bgc = null;
        frameWinGc = mygc;

        initFont();
        initStrokeList();
    }

    private void initEventListeners() {

        addComponentListener(new ComponentAdapter() {
            public void componentResized(ComponentEvent evt) {
                if (bPrtCanvas)
                    return;
                if (bNative) {
                    if (isShowing())
                      nativeResize(false);
                }
                else
                   javaResize();
                if (bActive && vjIf != null)
                    vjIf.canvasSizeChanged(winId);
                processSyncEvent();
            }

            public void componentMoved(ComponentEvent evt) {
                if (bPrtCanvas)
                    return;
                if (bNative) {
                    setGraphicRegion();
                    createShowEvent();
                }
            }

            public void componentShown(ComponentEvent evt) {
                if (isShowing()) {
                    if (bNative)
                        processShowEvent();
                    else {
                        if (wingc == null)
                           javaResize();
                    }
                }
                processSyncEvent();
            }
        });  // ComponentListener

        addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                if (bPrintMode)
                    return;
                int nClick = evt.getClickCount();
                // copy mode
                if (m_bSelect) {
                    setCopyMode(false);
                    return;
                }
                if (bLinux) {
                    long when = evt.getWhen();
                    if ((when - clickWhen) < 400)
                        nClick = 2;
                    clickWhen = when;
                }
                if (nClick >= 2) {
                    if (!bActive) {
                        ((ExpPanel)vnmrIf).requestActive();
                        return;
                    }
                    if (frameNum > 0 && select_frame(evt))
                        return;
                }
                if (objNum > 0) {
                    VIcon vIcon = null;
                    CanvasObjIF obj = null;
                    boolean bNewIcon = false;
                    if (nClick >= 2) {
                        obj = getActiveObj(evt.getX(), evt.getY());
                        if (obj != null) {
                            bNewIcon = true;
                            get_screen_loc();
                            if (obj instanceof VIcon) {
                                vIcon = (VIcon) obj;
                                // change the order of icon in vector
                                imgVector.removeElement(vIcon);
                                imgVector.addElement(vIcon);
                            } else {
                                tbVector.removeElement(obj);
                                tbVector.addElement(obj);
                            }
                        }
                    } else {
                        if (m_vIcon != null || activeObj != null)
                            bNewIcon = true;
                    }
                    if (bNewIcon) {
                        if (m_vIcon != null && !m_vIcon.equals(vIcon)) {
                            if (bNative)
                                showBox(m_vIcon, false);
                        }
                        if (vIcon != null) {
                            if (bNative)
                                showBox(vIcon, true);
                        }
                        m_vIcon = vIcon;
                        set_active_obj(obj);
                        if (activeObj != null) {
                            preActiveObj = obj;
                            obj_moveProc(evt);
                            setCursor(Cursor
                                    .getPredefinedCursor(Cursor.MOVE_CURSOR));
                        }
                        if (!bNative)
                            repaint_canvas();
                        return;
                    }
                } // iconNum > 0
                if (nClick >= 1)
                    mouseProc(evt, 0, 0, nClick);
                else if (sendMore)
                    mouseProc(evt, 0, 0, nClick);
                return;
            }

            public void mousePressed(MouseEvent evt) {
                // copy mode
                if (bPrintMode)
                    return;
                int modifier = evt.getModifiersEx();
                boolean button3 = false;
                boolean button2 = false;
                bMousePressed = true;
                pressedMouseBut = 1;
                drag_cnt = 0;
                mpx = evt.getX();
                mpy = evt.getY();
                if ((modifier & InputEvent.BUTTON3_DOWN_MASK) != 0) {
                    button3 = true;
                    pressedMouseBut = 3;
                }
                else if ((modifier & InputEvent.BUTTON2_DOWN_MASK) != 0) {
                    button2 = true;
                    pressedMouseBut = 2;
                }
                if (bGinWait) {
                    processGinEvent();
                    return;
                }
                if (pressedMouseBut == 1) {
                    if ((modifier & InputEvent.CTRL_DOWN_MASK) != 0) {
                        if (frameNum > 0) {
                             if (select_frame(evt)) {
                                bMousePressed = false;
                                return;
                             }
                        }
                    }
                    if (m_bSelect || bObjMove) {
                        if (bObjMove) {
                            if (bShowFrameBox)
                                clearFrameBorder();
                            dragX = evt.getX();
                            dragY = evt.getY();
                            drawFrameBox(false, evt.getPoint());
                        }
                        if (m_bSelect) {
                            if (m_pPrevStart == null) {
                                // First mouse press after entering select mode
                                // Initialize starting position
                                mousePt = mousePt2 = new Point(mpx, mpy);
                            } else if (m_pPrevEnd != null) {
                                // Already have a defined region;
                                // may redefine begin / end corners
                                double xStart = m_pPrevStart.getX();
                                double xEnd = m_pPrevEnd.getX();
                                double yStart = m_pPrevStart.getY();
                                double yEnd = m_pPrevEnd.getY();
                                boolean nearXStart = (Math.abs(mpx - xStart)
                                                      <= Math.abs(mpx - xEnd));
                                boolean nearYStart = (Math.abs(mpy - yStart)
                                                      <= Math.abs(mpy - yEnd));
                                if (nearXStart) {
                                    if (nearYStart) {
                                        // Flip both points
                                        m_pPrevStart.setLocation(xEnd, yEnd);
                                        m_pPrevEnd.setLocation(xStart, yStart);
                                    } else {
                                        // Flip x-values only
                                        m_pPrevStart.setLocation(xEnd, yStart);
                                        m_pPrevEnd.setLocation(xStart, yEnd);
                                    }
                                } else {
                                    if (nearYStart) {
                                        // Flip y-values only
                                        m_pPrevStart.setLocation(xStart, yEnd);
                                        m_pPrevEnd.setLocation(xEnd, yStart);
                                    }
                                }
                            }
                        }
                        return;
                    }
                }
                if (button3 || button2) {
                    bMoveFrame = false;
                    bObjMove = false;
                    bObjDrag = false;
                }
/* no longer in use. commented out March 2014
                if (frameMap != null) {
                    if (button3 && (modifier & InputEvent.CTRL_DOWN_MASK) != 0) {
                        if (activeObj == null)
                            showOrientPopup(true, mpx, mpy);
                        return;
                    }
                }
*/
                if (activeObj != null) {
                    // if (m_vIcon != null) {
                    if (activeObj instanceof VIcon) {
                        if (button3) {
                            showIconPopup(true, mpx, mpy);
                            return;
                        }
                    } else if (activeObj instanceof VTextBox) {
                        if (button3) {
                            showTextBoxPopup(true, mpx, mpy);
                            return;
                        }
                    }
                }

                mousePt = mousePt2 = new Point(mpx, mpy);
                // m_nMousex = mpx;
                // m_nMousey = mpy;
                mouseProc(evt, 0, 1, 0);
                vnmrIf.processMousePressed(evt);
            }

            public void mouseReleased(MouseEvent evt) {
                if (!bMousePressed)
                    return;
                bMousePressed = false;
                mpx = evt.getX();
                mpy = evt.getY();
                if (bGinWait) {
                    processGinEvent();
                    return;
                }
                if (bRubberMode) {
                    setCopyMode(false);
                    bDraging = false;
                    return;
                }
                if (bObjMove) {
                    bDraging = false;
                    resizeFrame();
                    return;
                }
                if (bObjResize) {
                    eraseBox();
                    bObjResize = false;
                }
                bDraging = false;
                mouseProc(evt, 1, 0, 0);
                vnmrIf.setInputFocus();
                vnmrIf.processMouseReleased(evt);
            }

            public void mouseEntered(MouseEvent evt) {
                if (bPrintMode)
                    return;
                mpx = evt.getX();
                mpy = evt.getY();
                // Sometimes gets called with cursorIndex=-1 which causes other mouse functions to fail
                if (bObjDrag && cursorIndex>=0) { 
                    setCursor(Cursor.getPredefinedCursor(cursorIndex));
                }
            }

            public void mouseExited(MouseEvent evt) {
                mpx = -1;
                mpy = -1;
                if (bPrintMode)
                    return;
                if (!bDraging) {
                    bObjMove = false;
                    bObjDrag = false;
                    if (bObjResize) {
                        eraseBox();
                        bObjResize = false;
                    }
                    else if (bShowMoveBorder)
                        repaint_canvas();
                    if (cursorIndex != Cursor.DEFAULT_CURSOR) {
                        cursorIndex = Cursor.DEFAULT_CURSOR;
                        setCursor(Cursor.getPredefinedCursor(cursorIndex));
                    }
                    if (sendMore)
                        moveProc(evt, 0, true);
                }
            }
        });  // MouseListener

        addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent evt) {
                mpx = evt.getX();
                mpy = evt.getY();
                if (bGinWait)
                    return;
                butInfo = evt.getModifiersEx();
                if ((butInfo & InputEvent.BUTTON2_DOWN_MASK) != 0)
                    dragMouseBut = 1;
                else if ((butInfo & InputEvent.BUTTON3_DOWN_MASK) != 0)
                    dragMouseBut = 2;
                else
                    dragMouseBut = 0;
                if (!bDraging)
                    drag_cnt = 0;
                bDraging = true;
                if (m_bSelect) {
                    mousePt2 = new Point(mpx, mpy);
                    if (bRubberArea)
                        repaint_canvas();
                    else
                        drawBox(mousePt, mousePt2);
                    return;
                }
                if (bObjMove && dragMouseBut == 0) {
                    drawFrameBox(false, evt.getPoint());
                    return;
                }
                moveProc(evt, 1, false);
            }

            public void mouseMoved(MouseEvent evt) {
                if (bPrintMode)
                    return;
                /*
                 if (!bNative) {
                 if (bActive && cursorIndex == Cursor.DEFAULT_CURSOR &&
                 objCursor != Cursor.getDefaultCursor())
                 setCursor(objCursor);
                 }
                 */
                mpx = evt.getX();
                mpy = evt.getY();
                if (activeObj != null) {
                    set_move_cursor(mpx, mpy, true);
                    return;
                }
                if (bFrameAct) {
                    if (frame_moveProc(evt))
                        return;
                }
                if (bActive && !bNative) {
                    if (cursorIndex == Cursor.DEFAULT_CURSOR
                            && objCursor != Cursor.getDefaultCursor()) {
                        setCursor(objCursor);
                        cursorIndex = -1;
                    }
                }
                if (sendMore) {
                    butInfo = evt.getModifiersEx();
                    moveProc(evt, 0, false);
                }
            }
        });  // MouseMotionListener

        addMouseWheelListener(new MouseWheelListener() {
             public void mouseWheelMoved(MouseWheelEvent e) {
                if (bPrintMode || bBatch)
                    return;
                long t = e.getWhen();
                long d = t - wheelStart;
                int scrolls = e.getWheelRotation();

                if (d < 100)
                    scrolls = scrolls * 2;
                wheelStart = t;
                String mess = new StringBuffer().append(JFUNC).append(WHEEL)
                    .append(", ").append(scrolls).append(")\n").toString();
                vnmrIf.sendToVnmr(mess);
             }
        });
    }

    @Override
    public void setVisible(boolean b) {
        if (bCsiWindow && b) {
            if (!isVisible())
                bRepaintCsi = true;
        }
        super.setVisible(b);
    }

    public void setCsiMode(boolean b) {
        bCsiWindow = b;
        bRepaintCsi = b;
        if (b) {
            JFUNC = "jFunc2(";
            JEVENT = "jEvent2(";
            JMOVE = "jMove2(";
            JREGION = "jRegion2(";
        }
        else {
            JFUNC = "jFunc(";
            JEVENT = "jEvent(";
            JMOVE = "jMove(";
            JREGION = "jRegion(";
        }
    }

    // Vnmrbg needs mouse data as follows:
    // button_id(0, 1, 2) drag release x y press click_number modifier

    public void mouseProc(MouseEvent ev, int release, int press, int clicks) {
        int  bid;

        if (!bActive)
            return;
        if (dragCmd != null) {
            vnmrIf.sendToVnmr(dragCmd);
            dragCmd = null;
        }
        bDraging = false;

        bid = ev.getID();
        mvx = ev.getX();
        mvy = ev.getY();

        // int bf = ev.getModifiers();
        if (press > 0) {
            butInfo = ev.getModifiersEx();
            dragMouseBut = 0;
            if ((butInfo & InputEvent.BUTTON2_DOWN_MASK) != 0)
                dragMouseBut = 1;
            else if ((butInfo & InputEvent.BUTTON3_DOWN_MASK) != 0)
                dragMouseBut = 2;
            if (activeObj != null) {
                Rectangle rect = activeObj.getBounds();
                if (rect.contains(mvx, mvy))
                    return;
                if (m_vIcon != null) {
                    showBox(m_vIcon, false);
                    m_vIcon = null;
                }
                set_active_obj(null);
                if (!bNative)
                    repaint_canvas();
            }
        } else if (activeObj != null)
            return;
        int bx = mvx;
        int by = mvy;
        if (bFrameAct) {
            if (mvx > frameX2 || mvy > frameY2)
                return;
            mvx = mvx - frameX;
            mvy = mvy - frameY;
            if (mvx < 0 || mvy < 0)
                return;
            if (frameMap.bVertical) {
                int x2 = mvx;
                if (frameMap.bRight) {
                    bx = mvy;
                    by = frameW - x2;
                } else {
                    bx = frameH - mvy;
                    by = x2;
                }
            } else {
                bx = mvx;
                by = mvy;
            }
        }

        String mess = new StringBuffer().append(JEVENT).append(VMOUSE)
                .append(", ").append(dragMouseBut).append(", 0, ").append(
                        release).append(", ").append(bx).append(", ")
                .append(by).append(", ").append(bid).append(", ")
                .append(clicks).append(", ").append(butInfo).append(", 0) ")
                .toString();
        vnmrIf.sendToVnmr(mess);
    }

    public void moveProc(MouseEvent ev, int drag, boolean exited) {
        int but, bx, by, bid, bmove;

        if (!bActive)
            return;
        if (exited) {
            bx = -1000;
            by = -1000;
        } else {
            bx = ev.getX();
            by = ev.getY();
            if (mvx == bx && mvy == by)
                return;
        }
        mvx = bx;
        mvy = by;

        if (drag > 0 && activeObj != null)
            return;

        bid = ev.getID();
        but = 0;
        bmove = 1;
        if (bFrameAct) {
            bx = bx - frameX;
            by = by - frameY;
            if (drag > 0) {
                if (bx < 0)
                    bx = 0;
                if (by < 0)
                    by = 0;
                if (bx > frameW)
                    bx = frameW;
                if (by > frameH)
                    bx = frameH;
            } else {
                if (bx < 0 || by < 0) {
                    bx = -1000;
                    by = -1000;
                }
                if (bx > frameW || by > frameH) {
                    bx = -1000;
                    by = -1000;
                }
            }
        }
        if (drag > 0) {
            but = dragMouseBut;
            bmove = 0;
            // drag_cnt++;
        }
            drag_cnt++;

        if (bFrameAct && frameMap.bVertical) {
            int x2 = bx;
            if (frameMap.bRight) {
                bx = by;
                by = frameW - x2;
            } else {
                bx = frameH - by;
                by = x2;
            }
        }

        String mess = new StringBuffer().append(JMOVE).append(VMOUSE)
                .append(",").append(but).append(", ").append(drag).append(
                        ", 0, ").append(bx).append(", ").append(by)
                .append(", ").append(bid).append(", 0, ").append(butInfo)
                .append(", ").append(bmove).append(") ").toString();
        if (bNative) {
            vnmrIf.sendToVnmr(mess);
        } else {
            if (drag_cnt > 1) {
                dragCmd = mess;
            } else {
                dragCmd = null;
                vnmrIf.sendToVnmr(mess);
            }
        }
    }

    public void reloadColormap(String mapName) {
        if (aipCmpId > 0 || csiCanvas != null) {
            cmpModel = null;
            aipCmpId = 0;
            if (csiCanvas != null) {
                csiCanvas.cmpModel = null;
                csiCanvas.aipCmpId = 0;
            }
            String mess0 = new StringBuffer().append(JFUNC).append(VnmrKey.APPLYCMP)
                 .append(", ").append("0, 0, '").append(mapName).append("')\n").toString();
            vnmrIf.sendToVnmr(mess0);

            String mess = new StringBuffer().append("aipSetColormap('")
               .append(mapName).append("','refresh')\n").toString();
            vnmrIf.sendToVnmr(mess);
        }
    }

    public void repaint_canvas() {
        if (bPrtCanvas)
            return;
        if (bOverlayed && !bTopLayer) {
            if (topCanvas != null && topCanvas.isVisible()) {
                topCanvas.repaint_canvas();
                return;
            }
        }
        if (bStartOverlay)
            return;
        if (!bWaitOnPaint) {
            bWaitOnPaint = true;
            repaint();
        }
    }

    public void setVerbose(boolean b) {
        bVerbose = b;
    }

    public Font getDefaultFont() {
        if (org_font != null)
           return org_font;
        return myFont;
    }

    public java.util.List<GraphSeries> getGraphSeries() {
        return gSeriesList;
    }

    private void clear_dconi_cursor(boolean bAll) {
        for (int k = 0; k < CURSOR_NUM; k++) {
            dconi_xCursor[k] = 0;
            dconi_yCursor[k] = 0;
            thresholdCursor[k] = 0;
            dconi_xStart[k] = 2;
            dconi_yStart[k] = 2;
            dconi_xLen[k] = 0;
            dconi_yLen[k] = 0;
            dconi_xColor[k] = 30;
            dconi_yColor[k] = 30;
            if (bAll && frameMap != null)
                frameMap.setThLoc(k, 0, 0, 0);
        }
        hairX1 = 0;
        hairX2 = 0;
    }

    private void clear_dconi_cursor() {
        clear_dconi_cursor(true);
    }

    private void clearAllSeries() {
        bSeriesLocked = true;
        bSeriesChanged = true;
        try {
           if (defaultMap == null)
              defaultMap = new XMap(0, MINWH, MINWH);
           if (bPrtCanvas) {
              defaultMap.clearSeries(false);
           }
           else {
               for (int k = 0; k <= frameNum; k++) {
                   XMap map = frameVector.elementAt(k);
                   if (map != null)
                       map.clearSeries(false);
               }
           }
           // remove_table_series(0, true);
        }
        catch (Exception e) {}

        bSeriesLocked = false;
    }

    private void clearSeries(boolean bTopLayerOnly) {
        bSeriesLocked = true;
        bSeriesChanged = true;
        try {
           if (bPrtCanvas) {
               if (defaultMap != null)
                  defaultMap.clearSeries(bTopLayerOnly);
           }
           else {
               if (frameMap != null)
                  frameMap.clearSeries(bTopLayerOnly);
           }
        }
        catch (Exception e) {}

        bSeriesLocked = false;
    }

    private void clearSeries(int x, int y, int w, int h) {
        bSeriesLocked = true;
        bSeriesChanged = true;
        try {
           if (bPrtCanvas) {
               if (defaultMap != null) {
                  if (h != fontHeight) // from vbg ParameterLine
                      defaultMap.clearSeries(x, y, w, h);
               }
           }
           else {
               if (frameMap != null)
                  frameMap.clearSeries(x, y, w, h);
           }
        }
        catch (Exception e) {}

        bSeriesLocked = false;
    }

    private void addLineSeries(int x, int y, int x2, int y2) {
         java.util.List<VJLine> list = null;
         
         bSeriesLocked = true;
         if (defaultMap != null)
             list = defaultMap.getLineList();
         if (!bPrtCanvas) {
             if (frameMap != null)
                list = frameMap.getLineList();
         }
         if (list == null) {
             bSeriesLocked = false;
             return;
         }

         bSeriesChanged = true;

         Color c = mygc.getColor();
         VJLine freeLine = null;
         try {
            for (VJLine line : list) {
               if (!line.isValid()) {
                   if (freeLine == null)
                       freeLine = line;
                   if (!bXor)
                       break;
               }
               else if (bXor) {
                   if (line.isXorMode()) {
                       if (line.equals(c, x, y, x2, y2)) {
                           line.setValid(false);
                           bSeriesLocked = false;
                           return;
                       }
                   }
               }
            }
         }
         catch (Exception e) { }
         if (freeLine == null) {
             freeLine = new VJLine();
             list.add(freeLine);
         }
         freeLine.setValue(c, x, y, x2, y2);
         freeLine.setLineWidth(lineLw);
         freeLine.setAlpha(alphaSet);
         freeLine.setValid(true);
         freeLine.setTopLayer(bTopFrameOn);
         freeLine.setXorMode(bXor);
         freeLine.clipRect = clipRect;
         bSeriesLocked = false;
    }

    private void drawSeries(Graphics2D gc) {
       if (bSeriesLocked || bSeriesChanged)
           return;
       if (defaultMap != null)
           defaultMap.drawSeries(gc);
    }

    private void setTopframeActive(int n) {
        if (frameMap == null)
           return;
        XMap map = frameMap.turnOnTopMap(n);
        if (map == null || map.gc == null) {
           if (n > 0)
               bTopFrameOn = true;
           return;
        }
        frameMap = map;
        if (bBatch) {
            mygc = frameMap.gc;
            // setDrawLineWidth(1);
        }
    }

    private void clearTopframe() {
        if (frameMap == null)
           return;
        frameMap.clearTopframe();
    }

    private void setGcColor(Graphics gc, int c) {

        if (!bColorPaint)
           return;
        /*********
        int  r, g, b;
        r = (int) redByte[c] & 0xff;
        g = (int) grnByte[c] & 0xff;
        b = (int) bluByte[c] & 0xff;
        gc.setColor(new Color(r, g, b));
        *********/

        gc.setColor(DisplayOptions.getColor(c));
    }

    private void draw_threshold_cursor(Graphics2D gc) {
        int lw = thresholdLw;
        int x, y, k, c;

        if (thresholdLw != lineLw)
            gc.setStroke(strokeList[thresholdLw]);
        c = 0;
        for (k = 0; k < CURSOR_NUM; k++) {
            if (thresholdCursor[k] > 0) {
                if (c != dconi_yColor[k]) {
                    c = dconi_yColor[k];
                    setGcColor(gc, c);
                }
                y = thresholdCursor[k] + frameY;
                x = frameX + dconi_yStart[k];
                gc.drawLine(x, y, x + dconi_yLen[k], y);
            }
        }
        if (thresholdLw != lineLw)
            gc.setStroke(lineStroke);
    }

    private void draw_dconi_cursor(Graphics2D gc) {
        int k, c;
        int x, y;

        // if (bBatch || bWaitOnPaint)
        //    return;
        if (bObjDrag)
            return;
        if (cursorLw != lineLw)
            gc.setStroke(strokeList[cursorLw]);

        c = 0;
        for (k = 0; k < CURSOR_NUM; k++) {
            if (dconi_xCursor[k] > 0) {
                if (c != dconi_xColor[k]) {
                    c = dconi_xColor[k];
                    setGcColor(gc, c);
                }
                x = dconi_xCursor[k] + frameX;
                y = frameY + dconi_xStart[k];
                gc.drawLine(x, y, x, y + dconi_xLen[k]);
            }
        }
        for (k = 0; k < CURSOR_NUM; k++) {
            if (dconi_yCursor[k] > 0) {
                if (c != dconi_yColor[k]) {
                    c = dconi_yColor[k];
                    setGcColor(gc, c);
                }
                y = dconi_yCursor[k] + frameY;
                x = frameX + dconi_yStart[k];
                gc.drawLine(x, y, x + dconi_yLen[k], y);
            }
        }
        if (cursorLw != lineLw)
            gc.setStroke(lineStroke);
    }

    private void draw_vdconi_cursor(Graphics2D gc, boolean bRight) {
        int k, c, x, y;
        if (bObjDrag)
            return;
        if (cursorLw != lineLw)
            gc.setStroke(strokeList[cursorLw]);
        c = 0;
        for (k = 0; k < CURSOR_NUM; k++) {
            if (dconi_xCursor[k] > 0) {
                if (c != dconi_xColor[k]) {
                    c = dconi_xColor[k];
                    setGcColor(gc, c);
                }
                if (bRight)
                    y = dconi_xCursor[k] + frameY;
                else
                    y = frameY2 - dconi_xCursor[k];
                // x = frameX + 2;
                x = frameX + dconi_xStart[k];
                gc.drawLine(x, y, x + dconi_xLen[k] - 4, y);
            }
        }
        for (k = 0; k < CURSOR_NUM; k++) {
            if (dconi_yCursor[k] > 0) {
                if (c != dconi_yColor[k]) {
                    c = dconi_yColor[k];
                    setGcColor(gc, c);
                }
                if (bRight) {
                    x = frameX2 - dconi_yCursor[k];
                    // y = frameY + 4;
                    y = frameY + dconi_yStart[k];;
                    gc.drawLine(x, y, x, y + dconi_yLen[k] - 4);
                } else {
                    x = frameX + dconi_yCursor[k];
                    // y = frameY2 - 4;
                    y = frameY2 - dconi_yStart[k];;
                    gc.drawLine(x, y - dconi_yLen[k], x, y);
                }
            }
        }
        if (cursorLw != lineLw)
            gc.setStroke(lineStroke);
    }

    private void draw_arrow(int x1, int y1,int x2,int y2,int thick,int c) {
        if (jArrow == null)
           jArrow = new VJArrow();
        int lw = lineLw;
        Color color = getColor(c);
        if (thick > 0)
            setLineWidth(thick);
        // mygc.setColor(color);
        // jArrow.draw(mygc, x1, y1, x2, y2, thick);
        jArrow.setEndPoints(x1, y1, x2, y2);
        jArrow.setLineWidth(lineLw);
        jArrow.setColor(color);
        jArrow.draw(mygc, false, false);

        if (thick > 0)
            setLineWidth(lw);
        mygc.setColor(graphColor);
    }

    private void draw_rect(int x1,int y1,int x2,int y2,int thick,int c) {
        int lw = lineLw;

        if (thick > 0)
            setLineWidth(thick);

        int w = x2 - x1;
        int h = y2 - y1;
        if (w < 1 || h < 1)
           return;
        mygc.setColor(getColor(c));
        if (thick < 0)
           mygc.fillRect(x1, y1, w, h);
        else
           mygc.drawRect(x1, y1, w, h);

        if (thick > 0)
            setLineWidth(lw);
        mygc.setColor(graphColor);
    }

    private void draw_roundRect(int x1,int y1,int x2,int y2,int thick,int c) {
        int lw = lineLw;

        if (thick > 0)
            setLineWidth(thick);

        int w = x2 - x1;
        int h = y2 - y1;
        if (w < 1 || h < 1)
           return;
        int arcW = w / 3;
        if (arcW > 20)
            arcW = 20;
        mygc.setColor(getColor(c));
        if (thick < 0)
           mygc.fillRoundRect(x1, y1, w, h, arcW, arcW);
        else
           mygc.drawRoundRect(x1, y1, w, h, arcW, arcW);

        if (thick > 0)
            setLineWidth(lw);
        mygc.setColor(graphColor);
    }

    private void draw_oval(int x1,int y1,int x2,int y2,int thick,int c) {
        int lw = lineLw;

        if (thick > 0)
            setLineWidth(thick);

        int w = x2 - x1;
        int h = y2 - y1;
        if (w < 1 || h < 1)
           return;
        mygc.setColor(getColor(c));
        if (thick < 0)
           mygc.fillOval(x1, y1, w, h);
        else
           mygc.drawOval(x1, y1, w, h);

        if (thick > 0)
            setLineWidth(lw);
        mygc.setColor(graphColor);
    }

    private void sendFontInfo() {
        if (bPrtCanvas)
            return;
        for (GraphicsFont gf: graphFontList) {
            gf.sendToVnmr(vnmrIf);
        }
        String mess = new StringBuffer().append("jFunc(")
               .append(VnmrKey.FONTARRAY)
               .append(",'RESET', 0,0,0,0,0,0,0)\n").toString();
        vnmrIf.sendToVnmr(mess);
    }

    private void set_3planes_mode(boolean b) {
        b3planes=b;
    }

    private void set_3planes_cursor(int indx, int c, int x1, int y1, int x2, int y2) {
        /***********
        int r, g, b;
        r = (int) redByte[c] & 0xff;
        g = (int) grnByte[c] & 0xff;
        b = (int) bluByte[c] & 0xff;
        crosshairs3D[i][j].set(new Color(r,g,b));
        ***********/

        Color color = DisplayOptions.getColor(c);
        
        int i=indx/2;
        int j=indx%2;
        crosshairs3D[i][j].set(color);
        crosshairs3D[i][j].set(x1,y1,x2,y2);
    }

    private void draw_3planes_cursors(Graphics g) {
        if (bObjDrag)
            return;
        for(int i=0;i<3;i++){
            for(int j=0;j<2;j++){
                crosshairs3D[i][j].draw(g);
            }
        }
    }

    private void draw_hair_cursor(Graphics2D g) {
        // g.setColor(DisplayOptions.getColor("crosshair"));
        if (bObjDrag)
            return;
        g.setColor(xhairColor);
        g.setFont(myFont);
        if (crosshairLw != lineLw)
            g.setStroke(strokeList[crosshairLw]);
        if (hairX1 > 0) {
            g.drawLine(hairX1, hairY1, hairX1, hairY1 + hairL1);
            g.drawString(hairS1, hairX1 + 4, hairY1 + fontHeight);
        }
        if (hairX2 > 0) {
            int x = hairX2 + hairL2;
            g.drawLine(hairX2, hairY2, x, hairY2);
            //g.drawString(hairS2, hairX2 + hairL2+4, hairY2 + fontHeight);
            x = x - fontMetric.stringWidth(hairS2);
            g.drawString(hairS2, x , hairY2 + fontHeight);
        }
        if (crosshairLw != lineLw)
            g.setStroke(lineStroke);
        /*
         if (!bOverlayed ) {
         hairX1 = 0;
         hairX2 = 0;
         }
         */
    }

    private void draw_vhair_cursor(Graphics2D g, boolean bRight) {
        if (bObjDrag)
            return;
        g.setColor(xhairColor);
        g.setFont(myFont);
        int x = frameX2 - hairY1 + frameY - hairL1;
        int y = hairX1 - frameX;
        if (crosshairLw != lineLw)
            g.setStroke(strokeList[crosshairLw]);
        if (hairX1 > 0) {
            if (bRight) {
                y = y + frameY;
                g.drawLine(x, y, x + hairL1, y);
                x = frameX2 - fontHeight;
                y = y + 3;
                g.translate(x, y);
                g.rotate(deg90);
                g.drawString(hairS1, 0, 0);
                g.rotate(-deg90);
                g.translate(-x, -y);
            } else {
                y = frameY2 - y;
                g.drawLine(frameX, y, frameX + hairL1, y);
                x = frameX + fontHeight;
                y = y - 3;
                g.translate(x, y);
                g.rotate(-deg90);
                g.drawString(hairS1, 0, 0);
                g.rotate(deg90);
                g.translate(-x, -y);
            }
        }
        if (hairX2 > 0) {
            x = hairY2 - frameY;
            y = hairX2 - frameX;
            if (bRight) {
                x = frameX2 - x;
                y = y + frameY;
                g.drawLine(x, y, x, y + hairL2);
                y = y + hairL2 + fontHeight;
                g.drawString(hairS2, x + 3, y);
            } else {
                x = frameX + x;
                y = frameY2 - y - hairL2;
                g.drawLine(x, y, x, y + hairL2);
                y = y - 2;
                g.drawString(hairS2, x + 3, y);
            }
        }
        if (crosshairLw != lineLw)
            g.setStroke(lineStroke);
        /*
         if (!bOverlayed ) {
         hairX1 = 0;
         hairX2 = 0;
         }
         */
    }

    private boolean set_move_cursor(int mx, int my, Boolean bCanvasObj) {
        if (m_bSelect)
            return false;

        int x = mx - moveX;
        int y = my - moveY;
        int gap = 8;
        int dx = moveX;
        int dy = moveY;
        int cursor = Cursor.DEFAULT_CURSOR;
        boolean bMove = false;

        if (y >= -2 && y <= moveH + 1) {
            if (x >= -2 && x <= moveW + 1) {
                bMove = true;
                if (x < 4) {
                    if (y < gap) {
                        cursor = Cursor.NW_RESIZE_CURSOR;
                        dx = moveX2;
                        dy = moveY2;
                        bMove = false;
                    } else if (y >= moveH - gap) {
                        cursor = Cursor.SW_RESIZE_CURSOR;
                        dx = moveX2;
                        dy = moveY;
                        bMove = false;
                    } else
                        cursor = Cursor.MOVE_CURSOR;
                } else if (x >= moveW - 4) {
                    if (y < gap) {
                        cursor = Cursor.NE_RESIZE_CURSOR;
                        dx = moveX;
                        dy = moveY2;
                        bMove = false;
                    } else if (y >= moveH - gap) {
                        cursor = Cursor.SE_RESIZE_CURSOR;
                        bMove = false;
                    } else
                        cursor = Cursor.MOVE_CURSOR;
                } else if (y < 4) {
                    if (x < gap) {
                        cursor = Cursor.NW_RESIZE_CURSOR;
                        dx = moveX2;
                        dy = moveY2;
                        bMove = false;
                    } else if (x >= moveW - gap) {
                        cursor = Cursor.NE_RESIZE_CURSOR;
                        dx = moveX;
                        dy = moveY2;
                        bMove = false;
                    } else
                        cursor = Cursor.MOVE_CURSOR;
                } else if (y > moveH - 4) {
                    if (x < gap) {
                        cursor = Cursor.SW_RESIZE_CURSOR;
                        dx = moveX2;
                        dy = moveY;
                        bMove = false;
                    } else if (x >= moveW - gap) {
                        cursor = Cursor.SE_RESIZE_CURSOR;
                        bMove = false;
                    } else
                        cursor = Cursor.MOVE_CURSOR;
                }
            }
        }

        if (bCanvasObj && bMove)
            cursor = Cursor.MOVE_CURSOR;
        //use objCursor when cursorIndex is delfault
        if (cursorIndex != cursor) {
            /*
             if (bActive && cursor == Cursor.DEFAULT_CURSOR)
             setCursor(objCursor);
             else if (cursor == Cursor.DEFAULT_CURSOR)
             setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
             else
             */
            setCursor(Cursor.getPredefinedCursor(cursor));
            cursorIndex = cursor;
        }

        // cursorIndex = cursor;
        if (cursor == Cursor.DEFAULT_CURSOR) {
            bObjMove = false;
            bObjDrag = false;
            return false;
        }
        bObjMove = true;
        if (cursor == Cursor.MOVE_CURSOR) {
            bObjDrag = true;
            bObjResize = false;
            framePoint = new Point(mx, my);
        } else {
            bObjDrag = false;
            bObjResize = true;
            framePoint = new Point(dx, dy);
        }
        // copyColor = Color.yellow;
        copyColor = hilitColor;
        return true;
    }

    private boolean frame_moveProc(MouseEvent ev) {
        if (bCsiWindow)
           return false;
        moveX = frameX;
        moveY = frameY;
        moveW = frameW;
        moveH = frameH;
        moveX2 = moveX + frameW;
        moveY2 = moveY + frameH;
        bMoveFrame = set_move_cursor(ev.getX(), ev.getY(), false);
        if (bShowMoveBorder && !bMoveFrame)
            repaint_canvas();
        return bMoveFrame;
    }

    private boolean obj_moveProc(MouseEvent ev) {
        bMoveFrame = false;
        Rectangle r = activeObj.getBounds();
        moveX = r.x;
        moveY = r.y;
        moveW = r.width;
        moveH = r.height;
        moveX2 = moveX + moveW;
        moveY2 = moveY + moveH;
        set_move_cursor(ev.getX(), ev.getY(), true);
        return true;
    }

    private void set_active_obj(CanvasObjIF obj) {
        if (activeObj != null) {
            if (!activeObj.equals(obj))
               activeObj.setSelected(false);
        }
        activeObj = obj;
        bMoveFrame = false;
        if (obj == null) {
            m_vIcon = null;
            return;
        }
        obj.setSelected(true);
        Rectangle r = obj.getBounds();
        moveX = r.x;
        moveY = r.y;
        moveW = r.width;
        moveH = r.height;
        moveX2 = moveX + moveW;
        moveY2 = moveY + moveH;
    }

    private void get_screen_loc() {
        if (!isShowing())
            return;
        Point xloc = getLocationOnScreen();
        scrX = xloc.x;
        scrY = xloc.y;
        scrX2 = xloc.x + winW;
        scrY2 = xloc.y + winH;
    }

    private void set_antiAlias() {
         if (bPaintModeOnly && DisplayOptions.isAAEnabled()) {
             mygc.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
         }
         else {
             mygc.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_OFF);
             mygc.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING,RenderingHints.VALUE_TEXT_ANTIALIAS_LCD_HRGB);
         }
    }

    private void adjust_print_font(Graphics2D g, int w, int h) {
         Font ft;
         FontMetrics fm;
         int   fw;
         float fs;

         if (org_font == null)
             org_font = orgPrintgc.getFont();
         ft = org_font;
         fm = getFontMetrics(ft);
         fw = fm.stringWidth("ppmm9") * 70 / 5;
         if (fw > w) {
             fs = (float) org_font.getSize();
             while (fs > 10.0f) {
                 fs = fs - 1.0f;
                 ft = org_font.deriveFont(fs);
                 fm = getFontMetrics(ft);
                 fw = fm.stringWidth("ppmm9") * 30 / 5;
                 if (fw <= w)
                     break;
             }
         }
         g.setFont(ft); 
         myFont = ft;
         fontMetric = fm;
         fontAscent = fm.getAscent();
         fontDescent = fm.getDescent();
         fontWidth = fm.stringWidth("ppmm9") / 5;
         fontHeight = fontAscent + fontDescent;
    }

    private void reset_frame_info() {
        if (frameMap == null)
            return;
        if (bPrtCanvas) {
            if (printgc == null)
                return;
            if (prtframegc != null)
                prtframegc.dispose();
            prtframegc = (Graphics2D) printgc.create();
            mygc = prtframegc;
            printTrX = frameMap.prX;
            printTrY = frameMap.prY;
            if (printTrX != 0 || printTrY != 0)
                mygc.translate(printTrX, printTrY);
            frameMap.rotateGc(mygc, frameMap.prW, frameMap.prH, false);
            bgc = null;
            frameX = 0;
            frameY = 0;
            adjust_print_font(mygc, frameMap.prW, frameMap.prH);
            return;
        }
        get_screen_loc();
        frameX = frameMap.x;
        frameY = frameMap.y;
        frameX2 = frameMap.x2;
        frameY2 = frameMap.y2;
        frameW = frameMap.width;
        frameH = frameMap.height;
        if (frameMap.fm != null) {
            fontMetric = frameMap.fm;
            fontWidth = frameMap.fontW;
            fontHeight = frameMap.fontH;
            fontAscent = frameMap.ftAscent;
            fontDescent = frameMap.ftDescent;
            myFont = frameMap.font;
        }
        if (transX != 0 || transY != 0)
            frameWinGc.translate(-transX, -transY);
        transX = frameX;
        transY = frameY;
        if (frameWinGc == null)
            frameWinGc = (Graphics2D) getGraphics();
        frameWinGc.translate(frameX, frameY);
        frameWinGc.setClip(0, 0, frameW, frameH);
        frameWinGc.setFont(myFont);
        frameWinGc.setBackground(bgColor);
        if (frameMap.gc == null)
            frameMap.gc = frameWinGc;
        if (frameMap.id == 1) {
            bimg = frameMap.img;
            bimgc = frameMap.gc;
        }
        // bgc = frameMap.gc;
        if (!isVisible() || bBatch) {
            mygc = frameMap.gc;
            bgc = null;
        }
        else {
            mygc = frameWinGc;
            bgc = frameMap.gc;
        }
    }

    public XMap getAipMap(int n) {
        XMap map = null;
     
        if (n < xmapVector.size())
            map = xmapVector.elementAt(n);
        return map;
    }

    public XMap getAipMap() {
        return getAipMap(lastAipId);
    }

    public XMap getAipMap(int id, int w, int h) {
        XMap map = null;
        if (id < xmapVector.size())
            map = xmapVector.elementAt(id);
        else
            xmapVector.setSize(id + 12);
        if (map == null) {
            map = new XMap(id, w, h);
            xmapVector.setElementAt(map, id);
            map.bImg = true;
            map.setImageId(id);
            // if (cmpModel != null)
            //     map.setColorMap(cmpModel);
        }
        else {
            if (w > MINWH && h > MINWH)
                map.setMapSize(w, h);
        }
        lastAipId = id;
        return map;
    }


    // switch active frame whithout telling vbg 
    public boolean set_frame(int id) {
        if (id <= 0 || frameVector.size() <= id)
            return false;
        XMap map = frameVector.elementAt(id);
        if (map == null || !map.bOpen) {
            return false;
        }
        frameMap = map;
        bFrameAct = true;
        frameId = id;
        if (id > frameNum)
            frameNum = id;
        reset_frame_info();
        return true;
    }

    private boolean select_frame_cycle(MouseEvent ev) {
        int x = ev.getX();
        int y = ev.getY();
        int newId = -1;
        int k, s1;
        XMap map;

        if (frameId <= 0)
            s1 = 1;
        else
            s1 = frameId + 1;
        for (k = s1; k <= frameNum; k++) {
            map = frameVector.elementAt(k);
            if (map != null && map.bEnable) {
                if (x >= map.x && x <= map.x2) {
                    if (y >= map.y && y <= map.y2) {
                        newId = k;
                        break;
                    }
                }
            }
        }
        if (newId < 0) {
            for (k = 1; k < s1; k++) {
                map = frameVector.elementAt(k);
                if (map != null && map.bEnable) {
                    if (x >= map.x && x <= map.x2) {
                        if (y >= map.y && y <= map.y2) {
                            newId = k;
                            break;
                        }
                    }
                }
            }
        }
        if (newId == frameId)
            return false;
        return select_frame(newId);
    }

    private boolean select_frame(MouseEvent ev) {
        int x = ev.getX();
        int y = ev.getY();
        int newId = -1;
        int k, s1, s2;
        int smin = 2 * winW * winH;

        s2 = smin;
        XMap map;
        bClickInFrame = false;
        for (k = 1; k <= frameNum; k++) {
            map = frameVector.elementAt(k);
            if (map != null && map.bEnable) {
                if (x >= map.x && x <= map.x2) {
                    if (y >= map.y && y <= map.y2) {
                        bClickInFrame = true;
                        s1 = map.width * map.height;
                        if (Math.abs(s1 - smin) < 0.5 * smin) {
                            if (s1 < smin)
                                s2 = s1;
                            else
                                s2 = smin;
                        }
                        if (s1 < smin) {
                            smin = s1;
                            newId = k;
                        }
                    }
                }
            }
        }

        // call select_frame_cycle if two frames are overlapped.
        //if(Math.abs(s2 - smin) < 0.5*smin) {
        if (s2 == smin) {
            return select_frame_cycle(ev);
        }

        if (newId < 0 || newId == frameId)
            return false;
        return select_frame(newId);
    }

    private boolean select_frame(int newId) {
        if (newId <= 0 || frameNum < 1)
            return false;
        /*
         if (newId <= 0 && bFrameAct) {
         set_frame(0);
         return true;
         }
         if (newId == frameId)
         return false;
         */
        clear_dconi_cursor(false);
        activateFrame(newId);
        // sendFrameSize();
        return true;
    }

    public Color getColor(int c) {
        if (c < 0 || c >= colorSize)
            return graphColor;
        if (!bColorPaint)
            return fgColor;
        if (colorId == c)
           return aColor;
        colorId = c;
        aColor = DisplayOptions.getColor(c);
        if (rgbAlpha > 250)
           return aColor;
        int r = aColor.getRed();
        int g = aColor.getGreen();
        int b = aColor.getBlue();

        aColor = new Color(r, g, b, rgbAlpha);
        return aColor;
    }

    private void setIcolor(int c) {
        if (c < 0) {
            iColor = graphColor;
            return;
        }
        if (icolorId == c)
            return;
        icolorId = c;
        iColor = getColor(c);
    }

    private void createMonoMap(IndexColorModel cm) {
        if (monoModel == null) {
             monoModel = new VColorModel("blackScale");
             monoModel.setGradientMap(255.0f, 0.0f);
        }
        if (cm != null) {
             if (cm.getMapSize() > 128)
                 monoModel.setMapSize(66);
             else
                 monoModel.setMapSize(cm.getMapSize()); 
        }
        monoCmIndex = monoModel.getModel();
    }

    private void setGrayMap(Color c1, Color c2) {
        float fr, fg, fb;
        float dr, dg, db, fk;
        int k, k2;

        if (imgColor == null)
            return;
        if (imgGrayStart < 12) // new style of colormap 
            return;
        if (imgGrayNums < 2)
            return;
        if (imgGrayNums + imgGrayStart > colorSize)
            return;
        fr = (float) c1.getRed();
        fg = (float) c1.getGreen();
        fb = (float) c1.getBlue();
        fk = imgGrayNums - 1;
        dr = ((float) c2.getRed() - fr) / fk;
        dg = ((float) c2.getGreen() - fg) / fk;
        db = ((float) c2.getBlue() - fb) / fk;
        if (Math.abs(dr) < 0.5f) {
            if (Math.abs(dg) < 0.5f) {
                if (Math.abs(db) < 0.5f)
                    return;
            }
        }

        k2 = imgGrayStart + imgGrayNums;
        byte b = (byte) rgbAlpha;
        for (k = imgGrayStart; k < k2; k++) {
            redByte[k] = (byte) fr;
            grnByte[k] = (byte) fg;
            bluByte[k] = (byte) fb;
            alphaByte[k] = b;
            fr += dr;
            fg += dg;
            fb += db;
        }
         alphaByte[imgGrayStart] = 0;
        alphaByte[bgIndex] = 0;
        setColorModel();
    }

    private void setImageGrayMap() {
        setGrayMap(bgColor, imgColor);
    }

    private XMap createFrameMap(int id, int x, int y, int w, int h) {
        if (id < 0)
            return null;
        if (frameVector.size() <= id)
            frameVector.setSize(id + 4);
        if (winW > MINWH && winH > MINWH) {
            if (w < 5 || w > winW)
                w = winW;
            if (h < 5 || h > winH)
                h = winH;
            if (x + w > winW)
                x = winW - w;
            if (y + h > winH)
                y = winH - h;
        } else { // unreasonable size
            if (w > 800)
                w = 800;
            else if (w < MINWH)
                w = MINWH;
            if (h > 800)
                h = 800;
            else if (h < MINWH)
                h = MINWH;
        }
        XMap map = frameVector.elementAt(id);
        if (map == null) {
            map = new XMap(id, x, y, w, h);
            map.setAlphaComposite(alphaSet, transComposite);
            frameVector.setElementAt(map, id);
        } else {
            map.x = x;
            map.y = y;
            map.setSize(w, h);
            map.bNewSize = true;
        }
        map.bOpen = true;
        map.bOrgOpen = true;
        if (id == 0) {
            defaultMap = map;
        }
        if (id == 1) {
            frame_1 = map;
            bimg = frame_1.img;
            bimgc = frame_1.gc;
        }
        else if (id == 2) {
            frame_2 = map;
            b2img = frame_2.img;
            b2imgc = frame_2.gc;
        }
        if (frameNum < id)
            frameNum = id;
        return map;
    }

    private void create_backup2() {
        if (bPrtCanvas)
            return;
        bBacking = true;
        XMap map = createFrameMap(2, 0, 0, winW, winH);
        if (map != null) {
            map.bOpen = false;
            map.bEnable = false;
        }
    }

    private void create_backup() {
        if (bPrtCanvas)
            return;
        if (frame_1 == null)
            createFrameMap(1, 0, 0, winW, winH);
        if (defaultMap == null)
            createFrameMap(0, 0, 0, MINWH, MINWH);
        defaultMap.width = winW;
        defaultMap.height = winH;
        defaultMap.gc = wingc;
    }

    private void setImageEnv() {
        if (bPrtCanvas)
            return;
       
        if (bimgc == null)
            create_backup();
        if (b2imgc == null)
            create_backup2();
        orgFrameNum = frameNum;
        orgFrameId = frameId;
    }

    private void setColorModel() {
        if (bTranslucent)
            cmIndex = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, alphaByte);
        else
            cmIndex = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, bgIndex);
        if (aipCmIndex == null)
            aipCmIndex = cmIndex;
    }

    private Color getTransparentColor(Color c) {
        if (rgbAlpha >= 250)
            return c;
        int r = c.getRed();
        int g = c.getGreen();
        int b = c.getBlue();
        Color color = new Color(r, g, b, rgbAlpha);
        return color;
    }

    private void setTransparentColor() {
        Color color = mygc.getColor();
        int r = color.getRed();
        int g = color.getGreen();
        int b = color.getBlue();
        color = new Color(r, g, b, rgbAlpha);
        mygc.setColor(color);
        if (bgc != null)
            bgc.setColor(color);
        graphColor = color;
        fontColor = getTransparentColor(fontColor);
    }

    private void setAipTransparentLevel(int v, int id)  {
       if (!bTranslucent) {
           return;
       }
       if (cmpModel == null)
           return;
       float fv = (float) v;
       if (fv > 99.0f)
           fv = 100.0f;
       else if (fv < 0.0f)
           fv = 0.0f;
       float newSet = 1.0f - fv / 100.0f;
       int alpha = (int) (255.0f * newSet);
       if (Math.abs(alpha - cmAlpha) < 2)
           return;

       cmAlpha = alpha;
       aipCmIndex = cmpModel.getModel(alpha);
    }

    private void setTransparentLevel(int v)  {
       float fv = (float) v;
       if (fv > 99.0f)
           fv = 100.0f;
       else if (fv < 0.0f)
           fv = 0.0f;
       float newSet = 1.0f - fv / 100.0f;
       rgbAlpha = (int) (255.0f * newSet);
       if (newSet != alphaSet) {
          alphaSet = newSet;
          colorId = -1;
          icolorId = -1;
          byte b = (byte) rgbAlpha;
          for (int k = 0; k < colorSize; k++)
             alphaByte[k] = b;
          alphaByte[bgIndex] = 0;
          // setColorModel();
          transComposite = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, newSet);
          setTransparentColor();
       }
       if (frameMap != null)
           frameMap.setAlphaComposite(newSet, transComposite);
    }

    private void setRgbAlpha(int v)  {
       float fv = (float) v;
       if (fv > 99.0f)
           fv = 100.0f;
       else if (fv < 0.0f)
           fv = 0.0f;
       float newSet = fv / 100.0f;
       rgbAlpha = (int) (255.0f * newSet);
       if (newSet != alphaSet) {
          alphaSet = newSet;
          colorId = -1;
          icolorId = -1;
          setTransparentColor();
       }
    }

    private void endPrint() {
         bPrinting = false;
         if (viewIf == null)
             viewIf = Util.getViewArea();
         if (viewIf != null)
             viewIf.finishPrintCanvas(winId);
         else
             setPrintMode(false);
    }

    private void setXorMode(Graphics2D g, Color c) {
         if (bArgbImg) {
            g.setComposite(AlphaComposite.getInstance(AlphaComposite.XOR, 1.0f));
            return;
         }
         g.setXORMode(c);
    }

    private void openColormap(int id, int firstNum, int num, String data) {
        StringTokenizer tok = new StringTokenizer(data, ",\n");
        if (!tok.hasMoreTokens())
            return;
        String name = tok.nextToken();
        String path = null;
        if (tok.hasMoreTokens())
            path = tok.nextToken();
        VColorModelPool vp = VColorModelPool.getInstance();
        vp.setTranslucent(bTranslucent);
        VColorModel cm = vp.openColorModel(id, name, path);
        if (cm == null)
            return;
        if (cm.getMapSize() < num)
            cm.setMapSize(num);
        if (cmpModel == null)
            cmpModel = cm;
        if (aipCmIndex == null || aipCmpId < 1) {
            aipCmIndex = cm.getModel();
            cmAlpha = cmpModel.getAlphaValue();
            aipCmpId = cm.getId();
        }
        if (cmIndex == null)
            cmIndex = cm.getModel();
    }

    private void switchColormap(int mapId, int whichAip) {
        if (bPrtCanvas) {
            if (!bColorPaint) {
                if (monoModel == null)
                   createMonoMap(null);
                cmpModel = monoModel;
                aipCmIndex = monoCmIndex;
                aipCmpId = mapId;
                return;
            }
        }
        if (cmpModel == null || cmpModel.getId() != mapId) {
            VColorModelPool vp = VColorModelPool.getInstance();
            cmpModel = vp.getColorModel(mapId);
            if (cmpModel == null)
                return;
            aipCmIndex = cmpModel.getModel();
            cmAlpha = cmpModel.getAlphaValue();
        }
        aipCmpId = mapId;
    }

    private void setColorInfo(int id, int order, int mapId,
                              int transparency, String imgName) {
        ImageColorMapEditor.setColorInfo(id, order, mapId, transparency, imgName);
        /***
        VColorMapPanel vp = Util.getColormapPanel();
        if (vp == null)
            return;
        vp.setColorInfo(id, order, mapId, transparency, imgName);
        ***/
    }

    private void selectColorInfo(int id) {
        ImageColorMapEditor.selectColorInfo(id);
        /***
        VColorMapPanel vp = Util.getColormapPanel();
        if (vp == null)
            return;
        vp.selectColorInfo(id);
        ***/
    }

    private void setImageId(int mapid, int id) {
        if (xmapVector == null || mapid < 0)
           return;
        if (xmapVector.size() <= mapid)
           return;
        XMap map = xmapVector.elementAt(mapid);
        if (map != null)
           map.setImageId(id);
    }

    private void setAipFrame(int id, int x, int y, int w, int h) {
        XMap map;

        aipId = id;
        // mygc = null; 
        bgc = null;
        map = null;
        bImgType = true;

        if (!bImgBg)
            setImagingBg(true);
        if (b2imgc == null)
            setImageEnv();
        if (aipId >= 4) {
            if (bPrtCanvas) {
                if (printMap == null)
                {
                    printMap = new XMap(9999, 100, 100);
                }
                if (w > MINWH && h > MINWH)
                    printMap.setSize(w, h);
                map = printMap;
                map.bImg = true;
            }
            else
                map = getAipMap(id, w, h);
            if (map == null || map.gc == null)
                aipId = 0;
            aipMap = map;
        }
        if (aipId < 4) {
            if (bPrtCanvas) {
                if (printTrX != 0 || printTrY != 0) {
                    printgc.translate(-printTrX, -printTrY);
                    printTrX = 0;
                    printTrY = 0;
                }
                mygc = printgc;
                aipMap = null;
                return;
            }
            if (aipId <= 1) {
                map = frame_1;
            } else if (aipId == 2) {
                map = frame_2;
            }
            if (map == null) {
                aipId = 0;
                map = frame_1;
            }
            if (map != null)
                mygc = map.gc;
            aipMap = map;
            return;
        }
        if (map == null)
            return;
        map.setColormapId(aipCmpId);
        mygc = map.gc;
        if (w > 0 && h > 0) {
            map.x = x;
            map.y = y;
            if (bPrtCanvas) {
                map.prW = w;
                map.prH = h;
            }
        }
    }

    public void clearCanvas() {
        if (bPrtCanvas) {
            if (printgc != null)
                printgc.clearRect(0, 0, printW, printH);
            clear_dconi_cursor(false);
            return;
        }
        if (bNative) {
            showBox(m_vIcon, false);
            return;
        }
        if (bFrameAct) {
            if (!frameMap.bText)
                frameMap.clear();
            for (int i = 0; i < xnum; i++) {
                xindex[i] = 0;
            }
        } else if (wingc != null) {
            if (!bImgType) {
                mygc = wingc;
                bgc = bimgc;
                if (!bBatch)
                    mygc.clearRect(0, 0, winW, winH);
            }
            else {
               if (bImgBg)
                   setImagingBg(false);
            }
            if (bimgc != null) {
               bimgc.clearRect(0, 0, winW, winH);
            }
        }
        clear_dconi_cursor();
        m_pPrevStart = null;
        clearSeries(false);
        remove_table_series(0, true);
        bkRegionW = 0;
        bkRegionH = 0;
        bkRegionColor = bgColor;
        bTopFrameOn = false;
        graphTool.setTopLayerOn(bTopFrameOn);
    }

    // public synchronized void  graphFunc(DataInputStream ins, int func)
    //  deprecated.
    public void graphFunc(DataInputStream ins, int func) {
    }

    public boolean gFunc(DataInputStream ins, int func) {
        int c, l, k;

        switch (func) {
        case RASTER:
            graphTool.drawRasterImage(ins, cmIndex, mygc, bgc);
            return (true);
        case IRASTER:
            graphTool.drawRasterImage(ins, aipCmIndex, mygc, null);
            return (true);
        case COLORTABLE:
            try {
                int size = ins.readInt();
                c = ins.readInt(); // offset
                l = c + size;
                for (k = c; k < l; k++) {
                    redByte[k] = (byte) ins.readInt();
                    grnByte[k] = (byte) ins.readInt();
                    bluByte[k] = (byte) ins.readInt();
                }
            } catch (Exception e) {
                Messages.writeStackTrace(e);
                return (false);
            }
            setColorModel();
            return (true);
        case ICON:
            drawIcon(ins);
            return (true);
        default:
            return (false);
        }
    }

    public boolean g6Func(DataInputStream ins, int func) {
        int c, k, x, y, id, len;
        Color  color;
        String str = null;

        try {
            id = ins.readInt();
            x = ins.readInt();
            y = ins.readInt();
            c = ins.readInt();
            len = ins.readInt();
            if (bufLen <= len) {
                bufLen = len + 6;
                buf = new byte[bufLen];
            }
            for (k = 0; k < len; k++)
                buf[k] = ins.readByte();
            buf[len] = 0;
            str = new String(buf, 0, len);
        } catch (Exception e) {
            return (false);
        }
        if (len < 1)
            return (true);

        /******** too many repeats, not the right place to set antialias
        if (bPaintModeOnly) {
            if(DisplayOptions.isAAEnabled())
                mygc.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
            else
                mygc.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_OFF);
        }
        ********/

        switch (func) {
        case SPECTRUM_MIN:
            specMinStr = str;
            return (true);
        case SPECTRUM_MAX:
            specMaxStr = str;
            return (true);
        case SPECTRUM_RATIO:
            specRatioStr = str;
            setSpectrumThickness();
            return (true);
        case LINE_THICK:
            lineThickStr = str;
            setLineThickness();
            return (true);
        case GRAPH_FONT:
            setGraphicsFont(str);
            return (true);
        case GRAPH_COLOR:
            if (!bColorPaint)
                return (true);
            color = DisplayOptions.getColor(str);
            graphColor = getTransparentColor(color);
            if (bXor && frameMap != null) {
                 for (k = 0; k < xnum; k++) {
                        if (xindex[k] == c || xindex[k] == 0)
                              break;
                  }
                  if (k >= xnum)
                        k = xnum - 1;
                  xindex[k] = c;
                  if (xcolors[k] == null)
                  xcolors[k] = new XColor(bgColor, k + 1);
                  setXorMode(mygc, xcolors[k]);
            }
            mygc.setColor(graphColor);
            if (bgc != null)
                  bgc.setColor(graphColor);
            if (bFreeFontColor)
                fontColor = graphColor;
            return (true);
        case TEXT:
            drawText(str, x, y, c);
            return (true);
        case TICTEXT:
            drawTicText(str, x, y, c);
            return (true);
        case VTEXT: // dvstring
            drawVText(str, x, y, c);
            return (true);
        case IVTEXT: // dvstring
            drawIVText(str, x, y, c);
            return (true);
        case RTEXT: // Inversepw_text
            drawRText(str, x, y);
            return (true);
        case ITEXT: // aip_drawString
            drawIText(str, x, y, c);
            return (true);
        case IPTEXT: // iplan draw3DString
            drawPText(str, x, y, id);
            return (true);
        case STEXT: // scaledText
            drawSText(str, x, y, c);
            return (true);
        case RBOX: // Inversepw_box
            drawRBox(str, x, y);
            return (true);
        case ACURSOR: // aip_setCursor
            setCursor(VCursor.getCursor(str));
            objCursor = Cursor.getDefaultCursor();
            return (true);
        case ANNFONT:
            setAnnFont(str);
            return (true);
        case ANNCOLOR:
            setAnnColor(str);
            return (true);
        case BANNER:
            drawBanner(str, c);
            return (true);
        case COLORNAME:
            color = VnmrRgb.getColorByName(str);
            redByte[id] = (byte) color.getRed();
            grnByte[id] = (byte) color.getGreen();
            bluByte[id] = (byte) color.getBlue();
            return (true);
        case HCURSOR:
            if (id == 1) {
                if (x > 0)
                    hairX1 = x + frameX;
                else
                    hairX1 = 0;
                hairY1 = y + frameY;
                hairL1 = c;
                hairS1 = str;
            } else if (id == 2) {
                if (x > 0)
                    hairX2 = x + frameX;
                else
                    hairX2 = 0;
                hairY2 = y + frameY;
                hairL2 = c;
                hairS2 = str;
            }
            if (id == 3) {
                if (c != 0) {
                    if (x <= 0)
                        hairX1 = 0;
                    if (y <= 0)
                        hairX2 = 0;
                    repaint_canvas();
                }
                /*
                 else {
                 if (x > 0 || y > 0) {
                 if (bFrameAct)
                 draw_hair_cursor(mygc);
                 else
                 draw_hair_cursor(wingc);
                 }
                 }
                 */
            }
            return (true);
        case HTEXT: // dstring
            drawHText(str, x, y, c);
            return (true);
        case NOCOLOR_TEXT: // dstring, use previuos color
            drawJText(str, x, y);
            return (true);
        case MOVIESTART: //  movie start
            movieMaker = ImagesToJpeg.get();
            if( movieMaker != null) {
                 sendMoreEvent(false);
                  // movieMaker.start(path, width, height, nimages, rate)
                  movieMaker.start(str, id, x, y, c);
            }
            return (true);
        case OPENCOLORMAP:
            openColormap(id, x, y, str);
            return (true);
        case SETCOLORINFO: // imgId,order,mapId,transparency,imgName
            setColorInfo(id, x, y, c, str);
            return (true);
        default:
            return (true);
        }
    }

    private void addYbarSeries(DataInputStream ins) {
        java.util.List<VJYbar> list = null;

        bSeriesLocked = true;

        if (defaultMap == null)
            defaultMap = new XMap(0, MINWH, MINWH);
        list = defaultMap.getYbarList();
        if (!bPrtCanvas) {
            if (frameMap != null)
                list = frameMap.getYbarList();
        }
        if (list == null) {
            bSeriesLocked = false;
            return;
        }

        bSeriesChanged = true;
        VJYbar ybar = graphTool.saveYbar(ins, mygc, list, spLw, alphaSet, bXor);
        if (ybar != null)
            ybar.clipRect = clipRect;
        bSeriesLocked = false;
    }

    private void addPolylineSeries(DataInputStream ins) {
        java.util.List<VJYbar> list = null;

        bSeriesLocked = true;

        if (defaultMap == null)
            defaultMap = new XMap(0, MINWH, MINWH);
        list = defaultMap.getYbarList();
        if (!bPrtCanvas) {
            if (frameMap != null)
                list = frameMap.getYbarList();
        }
        if (list == null) {
            bSeriesLocked = false;
            return;
        }

        bSeriesChanged = true;
        VJYbar ybar = graphTool.savePolyline(ins, mygc, list, spLw, alphaSet, bXor);
        if (ybar != null)
            ybar.clipRect = clipRect;
        bSeriesLocked = false;
    }

    public boolean g5Func(DataInputStream ins, int func) {
        // int  id;

        try {
            ins.readInt(); // skip the first int (id)
        } catch (Exception e) {
            return (false);
        }

        switch (func) {
        case YBAR:
            b3planes = false;
            while (bPainting) {
                Thread.yield();
            }
            if (bSaveXorObj || bTopFrameOn) {
                addYbarSeries(ins);
                if (bGinFunc)
                    repaint_canvas();
                return (true);
            }
            if (spLw != lineLw)
                mygc.setStroke(specStroke);
            graphTool.drawYbar(ins, mygc, bgc);
            if (spLw != lineLw)
                 mygc.setStroke(lineStroke);
            if (bGinFunc)
                repaint_canvas();
            return (true);
        case XYBAR:
            b3planes = false;
            while (bPainting) {
                Thread.yield();
            }
            addYbarSeries(ins);
            if (bGinFunc)
                repaint_canvas();
            return (true);
        case XBAR:
            graphTool.drawBar(ins, mygc, bgc);
            return (true);
        case BAR: // draw cursor on window only
            if (!bBatch)
                graphTool.drawBar(ins, mygc, null);
            return (true);
        case IPOLYLINE:
            b3planes = false;
            while (bPainting) {
                Thread.yield();
            }
            if (bSaveXorObj || bTopFrameOn) {
                addPolylineSeries(ins);
                return (true);
            }
            if (spLw != lineLw)
                mygc.setStroke(specStroke);
            graphTool.drawPolyline(ins, mygc, bgc);
            if (spLw != lineLw)
                 mygc.setStroke(lineStroke);
            if (bGinFunc)
                repaint_canvas();
            return (true);
        case IPOLYGON: // from image browser
            graphTool.fillPolygon(ins, mygc, bgc);
            return (true);
        case POLYGON: // iplan fillPolygon
            graphTool.fillPolygon(ins, mygc, bgc);
            return (true);
        case DPCON: // dpcon
            b3planes = false;
            if (spLw != lineLw)
                 mygc.setStroke(specStroke);
            graphTool.dpcon(this, ins, mygc, bgc);
            if (spLw != lineLw)
                 mygc.setStroke(lineStroke);
            return (true);
        default:
            return (false);
        }
    }

    // g4Func supports image browser
    public boolean g4Func(DataInputStream ins, int func) {
        int c, k, i, i2, s, d;
        int saveAlpha;
        // int sw, sh;
        XMap map;
        Graphics2D gc;

        try {
            k = ins.readInt();
            for (c = 0; c < k; c++)
                pv[c] = ins.readInt();
        } catch (Exception e) {
            return (false);
        }
        switch (func) {
        case SET3PMODE:
            set_3planes_mode(pv[0]>0?true:false);
            return (true);
        case SET3PCURSOR:
            set_3planes_cursor(pv[0],pv[1],pv[2],pv[3],pv[4],pv[5]);
            return (true);
        case ICLEAR:
            if (pv[3] < 2)
                return (true);
            if (bPrtCanvas) {
                // mygc.clearRect(0, 0, pv[3], pv[4]);
                mygc.clearRect(pv[1], pv[2], pv[3], pv[4]);
                return(true);
            }
            while (bPainting) {
                Thread.yield();
            }
            if (!bImgBg)
               setImagingBg(true);
            if (aipMap != null)
               aipMap.clear(pv[1], pv[2], pv[3], pv[4]);
            else
               mygc.clearRect(pv[1], pv[2], pv[3], pv[4]);
            if (bgc != null) {
                bgc.clearRect(pv[1], pv[2], pv[3], pv[4]);
            }
            //   clearSeries(bTopFrameOn);
            clearSeries(pv[1], pv[2], pv[3], pv[4]);
            m_pPrevStart = null;
            return (true);
        case IRECT:
            saveAlpha = rgbAlpha;
            if (pv[1] < 0)
            {
               rgbAlpha = 255;
               setIcolor(-pv[1]);
            }
            else
            {
               setIcolor(pv[1]);
            }
            mygc.setColor(iColor);
            mygc.drawRect(pv[2], pv[3], pv[4], pv[5]);
            if (bgc != null) {
                bgc.setColor(iColor);
                bgc.drawRect(pv[2], pv[3], pv[4], pv[5]);
            }
            rgbAlpha = saveAlpha;
            return (true);
        case ILINE:
            saveAlpha = rgbAlpha;
            if (pv[1] < 0)
            {
               rgbAlpha = 255;
               setIcolor(pv[1]);
            }
            else
            {
               setIcolor(pv[1]);
            }
            mygc.setColor(iColor);
            mygc.drawLine(pv[2], pv[3], pv[4], pv[5]);
            if (bgc != null) {
                bgc.setColor(iColor);
                bgc.drawLine(pv[2], pv[3], pv[4], pv[5]);
            }
            rgbAlpha = saveAlpha;
            return (true);
        case IOVAL:
            setIcolor(pv[1]);
            mygc.setColor(iColor);
            mygc.drawOval(pv[2], pv[3], pv[4], pv[5]);
            if (bgc != null) {
                bgc.setColor(iColor);
                bgc.drawOval(pv[2], pv[3], pv[4], pv[5]);
            }
            return (true);
        case IBACKUP:  // create image map
            bImgType = true;
            if (!bImgBg)
               setImagingBg(true);
            if (b2imgc == null)
                setImageEnv();
            i = pv[0];
            if (i < 4)
                return (true);
            if (bPrtCanvas) {
                if (printMap == null)
                    printMap = new XMap(9999, pv[1], pv[2]);
                else
                    printMap.setSize(pv[1], pv[2]);
                map = printMap;
                map.bImg = true;
            }
            else
                map = getAipMap(i, pv[1], pv[2]);
            if (map == null)
                return (true);
            if (bPrtCanvas) {
                map.prW = pv[1];
                map.prH = pv[2];
            }
            // map.setAlphaComposite(alphaSet, transComposite);
            return (true);
        case IFREEBK:
            if (bPrtCanvas)
                return (true);
            map = getAipMap(pv[0]);
            if (map != null)
                map.delete();
            return (true);
        case IWINDOW:
            setAipFrame(pv[0], pv[1], pv[2], pv[3], pv[4]);
            return (true);
        case ICOPY:
            s = pv[0]; // source id
            d = pv[1]; // destination id
            BufferedImage img = null;
            if (s < 4) {
                if (bPrtCanvas)
                    return (true);
                if (s == 0) // wrong, window can not be as source
                    return (true);
                if (bPrtCanvas) {
                    img = printImgBuffer;
                }
                else {
                    if (s <= 1) {
                        img = bimg;
                    }
                    if (s == 2) {
                        img = b2img;
                    }
                }
                // sw = winW;
                // sh = winH;
            } else {
                if (bPrtCanvas)
                    map = printMap;
                else
                    map = getAipMap(s);
                if (map == null || map.img == null)
                    return (true);
                img = map.img;
                // sw = map.width;
                // sh = map.height;
            }
            if (img == null)
                return (true);
            gc = null;
            if (d < 4) {
                if (d <= 1) {
                    if (s <= 1) {
                        return (true); // wrong, copy itself
                    }
                    if (bPrtCanvas)
                        gc = printgc;
                    else {
                        if (bimgc == null)
                            create_backup();
                        gc = bimgc;
                    }
                } else if (d == 2) {
                    if (b2imgc == null)
                        create_backup2();
                    gc = b2imgc;
                }
            } else {
                if (bPrtCanvas)
                    map = printMap;
                else
                    map = getAipMap(d);
                if (map == null || map.gc == null)
                    return (true);
                gc = map.gc;
            }
            if (gc == null)
                return (true);
            i = pv[4]; // width
            i2 = pv[5]; // height
            /*
             if (i+pv[6] > sw)
             i = sw - pv[6];
             if (i2+pv[7] > sh)
             i2 = sh;
             */

            // pv[6]: the x coordinate of rectangle of destination.
            // pv[7]: the y coordinate of rectangle of destination.
            // pv[2]: the x coordinate of rectangle of source
            // pv[3]: the y coordinate of rectangle of source

            gc.drawImage(img, pv[6], pv[7], pv[6] + i, pv[7] + i2, pv[2],
                    pv[3], pv[2] + i, pv[3] + i2, null);
            if (bPrtCanvas)
                 return (true);

            if (d == 0 && s != 1 && bimgc != null) { // from image browser
                if (gc != bimgc) {
                    bimgc.drawImage(img, pv[6], pv[7], pv[6] + i, pv[7] + i2,
                            pv[2], pv[3], pv[2] + i, pv[3] + i2, null);
                }
            }
            return (true);
        case IGRAYMAP:
            if (imgColor == null)
                imgColor = Color.white;
            if (pv[0] < 12)
                return (true);
            if (imgGrayNums != pv[1] || imgGrayStart != pv[0]) {
                imgGrayNums = pv[1];
                imgGrayStart = pv[0];
                if (hs != null) {
                    hs.put("imgGrayNums", new Integer(imgGrayNums));
                    hs.put("imgGrayStart", new Integer(imgGrayStart));
                }
            }
            setImageGrayMap();
            return (true);
        case ICOLOR:
            setIcolor(pv[0]);
            mygc.setColor(iColor);
            if (bgc != null)
                bgc.setColor(iColor);
            return (true);
        case IPLINE:
            // pv[1] is line_width
            if (bPrtCanvas) {
                mygc.drawLine(pv[2], pv[3], pv[4], pv[5]);
                return (true);
            }
            gc = null;
            if (pv[0] == 0 || aipId > 0) // canvas
                gc = mygc;
            else {
                if (pv[0] == 1) { // buffer
                    gc = bimgc;
                } else if (pv[0] == 2) // buffer 2
                    gc = b2imgc;
            }
            if (gc != null)
                gc.drawLine(pv[2], pv[3], pv[4], pv[5]);
            return (true);
        case PRECT: // iplan drawRectangle
            if (bPrtCanvas) {
                mygc.drawRect(pv[2], pv[3], pv[4], pv[5]);
                return (true);
            }
            gc = null;
            if (pv[0] == 0 || aipId > 0) // canvas
                gc = mygc;
            else {
                if (pv[0] == 1) { // buffer
                    gc = bimgc;
                } else if (pv[0] == 2) // buffer 2
                    gc = b2imgc;
            }
            if (gc != null)
                gc.drawRect(pv[2], pv[3], pv[4], pv[5]);
            if (bGinFunc)
                repaint_canvas();
            return (true);
        case PARC: // iplan drawCircle
            if (bPrtCanvas) {
                mygc.drawArc(pv[2], pv[3], pv[4], pv[5], pv[6], pv[7]);
                return (true);
            }
            gc = null;
            if (pv[0] == 0 || aipId > 0) // canvas
                gc = mygc;
            else {
                if (pv[0] == 1) { // buffer
                    gc = bimgc;
                } else if (pv[0] == 2) // buffer 2
                    gc = b2imgc;
            }
            if (gc != null)
                gc.drawArc(pv[2], pv[3], pv[4], pv[5], pv[6], pv[7]);
            return (true);
        case CROSS: // iplan drawCross
            i = pv[2] + pv[4]; // x2
            i2 = pv[3] + pv[5]; // y2
            if (bPrtCanvas) {
                mygc.drawLine(pv[2], pv[3], i, i2);
                mygc.drawLine(pv[2], i2, i, pv[3]);
                return (true);
            }
            gc = null;
            if (pv[0] == 0 || aipId > 0) // canvas
                gc = mygc;
            else {
                if (pv[0] == 1) { // buffer
                    gc = bimgc;
                } else if (pv[0] == 2) // buffer 2
                    gc = b2imgc;
            }
            if (gc != null) {
                gc.drawLine(pv[2], pv[3], i, i2);
                gc.drawLine(pv[2], i2, i, pv[3]);
            }
            return (true);
        case REGION: // iplan region
            if (bPrtCanvas) {
                return (true);
            }
            i = pv[3];
            i2 = pv[4];
            if (i <= 1 || i2 <= 1) {
                clipRect = null;
                if (mygc != null)
                    mygc.setClip(null);
                if (bgc != null)
                    bgc.setClip(null);
                /*
                 if (mygc != null)
                 mygc.setClip(0, 0, winW, winH);
                 if (bgc != null)
                 bgc.setClip(0, 0, winW, winH);
                 */
            } else {
                clipRect = new Rectangle(pv[1], pv[2], i, i2);
                if (mygc != null) {
                    mygc.setClip(clipRect);
                }
                if (bgc != null) {
                    bgc.setClip(clipRect);
                }
            }
            return (true);
        case CLEAR2: // clear pixmap
            if (bPrtCanvas || pv[3] < 2)
                return (true);
            if (pv[3] > 20) {
                while (bPainting)
                    Thread.yield();
                clearSeries(pv[1], pv[2], pv[3], pv[4]);
            }
            gc = null;
            if (pv[0] == 0)
                gc = mygc;
            else {
                if (pv[0] == 1) { // buffer
                    if (bFrameAct)
                        gc = frameMap.gc;
                    else
                        gc = bimgc;
                } else if (pv[0] == 2) // buffer 2
                    gc = b2imgc;
            }
            if (gc != null && pv[3] > 0)
                gc.clearRect(pv[1], pv[2], pv[3], pv[4]);
            return (true);
        case JFRAME:
            switch (pv[0]) {
            case JFRAME_OPEN:
                openFrame(pv[1], pv[2], pv[3], pv[4], pv[5]);
                // sendFrameSize();
                break;
            case JFRAME_CLOSE:
                closeFrame(pv[1]);
                break;
            case JFRAME_CLOSEALL:
                closeAllFrames();
                break;
            case JFRAME_CLOSEALLText:
                closeAllTextFrames();
                break;
            case JFRAME_ACTIVE:
                if (bPrtCanvas) {
                    if (frameId > 0 && (frameId != pv[1])) {
                         if (printImgBuffer != null) {
                            bSeriesChanged = false;
                            drawSeries(printgc);
                            clearSeries(false);
                            orgPrintgc.drawImage(printImgBuffer, 0, 0, printW, printH,
                                 0, 0, printW, printH, null);
                            printgc.clearRect(0, 0, printW, printH);
                         } 
                    }
                }
                if (pv[1] > 0) {
                    if (frameNum < 1) {
                        if (orgFrameNum > 0) {
                            frameNum = orgFrameNum;
                            frameId = orgFrameId;
                        }
                    }
                    if (frameNum > 0)
                        set_frame(pv[1]);
                }
                break;
            case JFRAME_IDLE:
                enableFrame(false, pv[1]);
                break;
            case JFRAME_IDLEALL:
                enableAllFrames(false);
                break;
            case JFRAME_ENABLE:
                enableFrame(true, pv[1]);
                break;
            case JFRAME_ENABLEALL:
                enableAllFrames(true);
                break;
            case ENABLE_TOP_FRAME:
                bTopFrameOn = false;
                if (bPrtCanvas || frameNum < 1) {
                    // setDrawLineWidth(1);
                    return (true);
                }
                setTopframeActive(pv[1]);
                graphTool.setTopLayerOn(bTopFrameOn);
                break;
            case CLEAR_TOP_FRAME:
                if (bPrtCanvas)
                    return (true);
                while (bPainting) {
                    Thread.yield();
                }
                clearSeries(true);
                clearTopframe();
                break;
            case RAISE_TOP_FRAME:
                if (bPrtCanvas)
                    return (true);
                if (pv[1] != 0)
                    bTopFrameOnTop = true;
                else
                    bTopFrameOnTop = false;
                repaint_canvas();
                break;
            }
            return (true);
       case BACK_REGION: // background region
            if (bPrtCanvas)
                return (true);
            if (bFrameAct) {
                frameMap.setBackRegion(pv[0], pv[1], pv[2], pv[3], pv[4], pv[5]);
            }
            else if (pv[2] > bkRegionW && pv[3] > bkRegionH) {
                bkRegionX = pv[0];
                bkRegionY = pv[1];
                bkRegionW = pv[2];
                bkRegionH = pv[3];
                bkRegionColor = DisplayOptions.getColor(pv[4]);
            }
            return (true);
        case ICURSOR2:
            c = pv[0];
            k = pv[1];
            if (k > 2)
                return (true);
            if (c == 1) { // dconi x cursor
                dconi_xCursor[k] = pv[2];
                dconi_xStart[k] = pv[3];
                dconi_xLen[k] = pv[4];
            } else if (c == 2) { // dconi y cursor
                if (bThreshold) {
                    if (!bPrtCanvas && frameMap != null) {
                       frameMap.setThLoc(k, pv[2], pv[3], pv[4]);
                       return (true);
                    }
                    thresholdCursor[k] = pv[2];
                }
                else
                    dconi_yCursor[k] = pv[2];
                dconi_yStart[k] = pv[3];
                dconi_yLen[k] = pv[4];
            }
            return (true);
        case JARROW:
            draw_arrow(pv[0], pv[1], pv[2], pv[3], pv[4], pv[5]);
            return (true);
        case JRECT:
            draw_rect(pv[0], pv[1], pv[2], pv[3], pv[4], pv[5]);
            return (true);
        case JROUNDRECT:
            draw_roundRect(pv[0], pv[1], pv[2], pv[3], pv[4], pv[5]);
            return (true);
        case JOVAL:
            draw_oval(pv[0], pv[1], pv[2], pv[3], pv[4], pv[5]);
            return (true);
        default:
            return (false);
        }
    }

    public boolean g3Func(DataInputStream ins, int func) {
        int c, r, g, b, k, l, x2, y2;
        int pc;

        try {
            pc = ins.readInt();
            for (c = 0; c < pc; c++)
                pv[c] = ins.readInt();
        } catch (Exception e) {
            Messages.writeStackTrace(e);
            return (false);
        }

        switch (func) {

        case RGB:
            c = pv[0];
            if (c < 0 || c >= colorSize)
               return (true);
            if (c >= dpsColor0 && c < 256) {
               setDpsColors();
               return (true);
            }
            redByte[c] = (byte) pv[1];
            grnByte[c] = (byte) pv[2];
            bluByte[c] = (byte) pv[3];
            if (c != bgIndex)
                alphaByte[c] = alphaVal;
            return (true);
        case LINE:
            if (pv[0] > maxW || pv[2] > maxW)
                return (true);
            if (pv[1] > maxH || pv[3] > maxH)
                return (true);
            if (bSaveXorObj || bTopFrameOn) {
                addLineSeries(pv[0], pv[1], pv[2], pv[3]);
                return (true);
            }
            if (mygc != null)
                mygc.drawLine(pv[0], pv[1], pv[2], pv[3]);
            if (bgc != null)
                bgc.drawLine(pv[0], pv[1], pv[2], pv[3]);
            if (bGinFunc)
                repaint_canvas();
            return (true);
        case BGCOLOR:
        case COLORI:
            if (bNative)
                return (true);
            // if (wingc == null)
            //     return (true);
            c = pv[0];
            if (c >= colorSize) { // set fgcolor contrast to bgcolor
                setContrastColor();
                return (true);
            }
            if (c < 0)
                return (true);

            if (func == BGCOLOR) {
                alphaByte[bgIndex] = alphaVal;
                bgIndex = c;
                if (!bColorPaint)
                    return (true);
                // the bg should be set by Displayoption only, not by  Vnmrbg.
                alphaByte[bgIndex] = 0;
                if (mygc != null)
                    mygc.setBackground(bgColor);
                if (bgc != null)
                    bgc.setBackground(bgColor);
                for (k = 1; k <= frameNum; k++) {
                    XMap map = frameVector.elementAt(k);
                    if (map != null)
                        map.setBg();
                }
                return (true);
            }

            fgIndex = c;
            if (bColorPaint) {
                graphColor = getColor(c);
                if (bXor && frameMap != null) {
                        for (k = 0; k < xnum; k++) {
                            if (xindex[k] == c || xindex[k] == 0)
                                break;
                        }
                        if (k >= xnum)
                            k = xnum - 1;
                        xindex[k] = c;
                        if (xcolors[k] == null)
                            xcolors[k] = new XColor(bgColor, k + 1);
                        setXorMode(mygc, xcolors[k]);
                }
            } else {
                if (fgIndex == bgIndex)
                        graphColor = Color.white;
                else
                        graphColor = fgColor;
            }
            mygc.setColor(graphColor);
            if (bgc != null)
                    bgc.setColor(graphColor);
            if (bFreeFontColor)
                fontColor = graphColor;
            return (true);
        case BOX:
            if (fgIndex == bgIndex) {
                while (bPainting) {
                    Thread.yield();
                }
                clearSeries(pv[0], pv[1], pv[2], pv[3]);
                mygc.clearRect(pv[0], pv[1], pv[2], pv[3]);
                if (bgc != null)
                    bgc.clearRect(pv[0], pv[1], pv[2], pv[3]);
            } else {
                mygc.fillRect(pv[0], pv[1], pv[2], pv[3]);
                if (bgc != null)
                    bgc.fillRect(pv[0], pv[1], pv[2], pv[3]);
            }
            if (bGinFunc)
                repaint_canvas();
            return (true);
        case COPY2: // copy buffer1 to window
            if (bPrtCanvas)
                return (true);
            if (bimg != null) {
                x2 = pv[0] + pv[2];
                y2 = pv[1] + pv[3];
                if (bFrameAct) {
                    frameWinGc.drawImage(frameMap.img, pv[0], pv[1], x2, y2,
                            pv[0], pv[1], x2, y2, null);
                } else {
                    if (!bImgType && wingc != null)
                        wingc.drawImage(bimg, pv[0], pv[1], x2, y2, pv[0],
                                pv[1], x2, y2, null);
                }
            }
            return (true);
        case VCURSOR:
            c = pv[0];
            g = Cursor.DEFAULT_CURSOR;
            if (c == HANDCURSOR)
                g = Cursor.HAND_CURSOR;
            cursorIndex = -1;
            if (!bNative && g == Cursor.DEFAULT_CURSOR)
                if (bActive)
                    setCursor(objCursor);
                else
                    setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
            else if (!bNative)
                setCursor(Cursor.getPredefinedCursor(g));
            return (true);
        case SFONT:
            setBannerFont(pv[0]);
            return (true);
        case GIN:
            if (bPrtCanvas)
                return (true);
            c = pv[0];
            bGinWait = false;
            if (!isShowing())
                return (true);
            bGinFunc = true;
            ginEvent = 0;
            ginButton = 0;
            if (pc > 2) {
                ginEvent = pv[1];
                ginButton = pv[2];
            }
            else {
                ginEvent = 0;
                ginButton = 0;
            }
            if (c == 1) { // no wait
                processGinEvent();
                if (gp != null)
                    gp.setVisible(false);
                // sendGinKeyEvent();
            } else if (c == 2) {
                bGinWait = true;
                processGinEvent();
                // startGinEvent();
            } else if (c == 3) {
                if (gp != null)
                    gp.setVisible(false);
            }
            if (c == 4)
                bGinFunc = false;
            if (bGinFunc)
                repaint_canvas();
            return (true);
        case ICURSOR:
            c = pv[0];
            k = pv[1];
            if (k > 2)
                return (true);
            r = pv[2]; // cursor position
            l = pv[3]; // cursor length
            if (c == 1) { // dconi x cursor
                dconi_xCursor[k] = r;
                dconi_xLen[k] = l;
                dconi_xStart[k] = 2;
            } else if (c == 2) { // dconi y cursor
                if (bThreshold) {
                    if (!bPrtCanvas && frameMap != null) {
                       frameMap.setThLoc(k, r, 4, l);
                       return (true);
                    }
                    thresholdCursor[k] = r;
                }
                else
                    dconi_yCursor[k] = r;
                dconi_yLen[k] = l;
                dconi_yStart[k] = 2;
            }
            return (true);
        case CSCOLOR: // cursor color
            bThreshold = false;
            c = pv[0];
            k = pv[1];
            if (k > 2)
                return (true);
            r = pv[2]; // cursor color
            if (c == 1) { // dconi x cursor
                dconi_xColor[k] = r;
            } else {
                dconi_yColor[k] = r;
            }
            return (true);
        case THSCOLOR: // threshold color
            c = pv[0];
            k = pv[1];
            if (k > 2)
                return (true);
            r = pv[2]; // threshold color
            if (c == 2) { // dconi y cursor
                bThreshold = true;
                dconi_yColor[k] = r;
                if (!bPrtCanvas) {
                    if (frameMap != null)
                       frameMap.setThColor(k, r);
                }
            }
            return (true);
        case FGCOLOR: // set rgb directly
            if (bColorPaint)
                graphColor = new Color(pv[0], pv[1], pv[2], rgbAlpha);
            else
                graphColor = fgColor;
            mygc.setColor(graphColor);
            if (bgc != null)
                bgc.setColor(graphColor);
            if (bFreeFontColor)
                fontColor = graphColor;
            return (true);
        case GCOLOR: // set_color_intensity
            c = pv[0]; // color index
            k = pv[1]; // number of levels
            l = pv[2]; // intensity
            if (c < 0 || c >= colorSize)
                return (true);
            if (l <= 0)
                l = 1;
            if (k < l)
                k = l;
            /********
            r = redByte[c] & 0xff;
            g = grnByte[c] & 0xff;
            b = bluByte[c] & 0xff;
            ********/

            // double s = (double) l / (double) k;
            Color color = DisplayOptions.getColor(c);
            r = color.getRed();
            g = color.getGreen();
            b = color.getBlue();
            /*
            r = (int)(s * r);
            g = (int)(s * g);
            b = (int)(s * b);
            */
            l = k;
            r = r * l / k;
            g = g * l / k;
            b = b * l / k;
            graphColor = new Color(r, g, b, rgbAlpha);
            mygc.setColor(graphColor);
            if (bgc != null)
                bgc.setColor(graphColor);
            if (b2imgc != null)
                b2imgc.setColor(graphColor);
            return (true);
        case REEXEC: // reexec vbg cmd
            if (bPrtCanvas)
                return (true);
            if (pv[0] > 0)
                bRexec = true;
            else
                bRexec = false;
            if (frameMap != null)
                frameMap.xbRexec = bRexec;
            return (true);
        case JALPHA: // alpha mode
            if (!bImgType && frameMap != null) {
                /*
                 if (pv[0] > 0) {
                 frameMap.bOpaque = false;
                 // bgColor = bgAlpha;
                 }
                 else {
                 frameMap.bOpaque = true;
                 // bgColor = bgOpaque;
                 }
                 */
            }
            return (true);
        case XORON: // enable xor
            if (bPrtCanvas)
               return (true);
            if (pv[0] < 1) {
               bPaintModeOnly = false;
               setDrawLineWidth(1);
            }
            else {
               bXorEnabled = true;
               setDrawLineWidth(pv[0]);
            }
            mygc.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_OFF);
            return (true);
        case XOROFF: // disable xor
            if (bPrtCanvas)
               return (true);
            if (pv[0] < 1) {
               bPaintModeOnly = true;
            }
            else {
               bXorEnabled = false;
               setDrawLineWidth(pv[0]);
            }
            set_antiAlias();
            return (true);
        case ICSIWINDOW: // open/close csi window
            if (pv[0] > 0)
	       ((ExpPanel)vnmrIf).setCsiPanelVisible(true);
            else
	       ((ExpPanel)vnmrIf).setCsiPanelVisible(false);
            return (true);
        case ICSIORIENT: // csi window orientation
            if (pv[0] > 0)
	       ((ExpPanel)vnmrIf).setCsiPanelOrient(true);
            else
	       ((ExpPanel)vnmrIf).setCsiPanelOrient(false);
            return (true);
        case ICSIDISP:   // switch to csi window
            if (pv[0] > 0)
	       ((ExpPanel)vnmrIf).setCsiActive(true);
            else
	       ((ExpPanel)vnmrIf).setCsiActive(false);
            return (true);
        case PENTHICK:
            // if (bPrtCanvas)
            //    return (true);
            bLwChangeable = true;
            setDrawLineWidth(pv[0]);
            if (pv[0] > 1)
                bLwChangeable = false;
            return (true);
        case AIP_TRANSPARENT:
            setAipTransparentLevel(pv[0], pv[1]);
            return (true);
        case TRANSPARENT:
            setTransparentLevel(pv[0]);
            return (true);
        case SPECTRUMWIDTH:
            // if (bPrtCanvas)
            //    return (true);
            setSpectrumWidth(pv[0]);
            return (true);
        case LINEWIDTH:
            // if (bPrtCanvas)
            //    return (true);
            setLineWidth(pv[0]);
            return (true);
        case SETCOLORMAP:
            if (aipCmpId != pv[0])
               switchColormap(pv[0], pv[1]);
            return (true);
        case AIPID:
            setImageId(pv[0], pv[1]);
            return (true);
        case SELECTCOLORINFO:
            selectColorInfo(pv[0]);
            return (true);
        case RGB_ALPHA:
            setRgbAlpha(pv[0]);
            return (true);
        default:
            return (false);
        }
    }

    public boolean g2Func(int func) {
        switch (func) {
        case CLEAR:
            while (bPainting) {
                Thread.yield();
            }
            clearCanvas();
            if (bPrtCanvas) {
                return (true);
            }
            if (bSendFontInfo) {
                bSendFontInfo = false;
                sendFontInfo();
            }
            if (!bBatch)
                repaint_canvas();
            return (true);
        case GOK:
            if (dragCmd != null) {
                vnmrIf.sendToVnmr(dragCmd);
                dragCmd = null;
            } else
                drag_cnt = 0;
            if (bDrgTest) {
                drgCount++;
                if (drgCount >= drgNum) {
                    double drgEnd = System.currentTimeMillis();
                    drgStart = drgEnd - drgStart;
                    if (drgCount == drgNum)
                        System.out.println(" drag tested " + drgNum
                                + " times, " + drgStart + "  milliseconds");
                    else {
                        bDrgTest = false;
                        System.out.println(" drag relase took " + drgStart
                                + "  milliseconds");
                    }
                    drgStart = drgEnd;
                }
                sendDragTestStr();
            }
            return (true);
        case XORMODE:
            bSaveXorObj = false;
            if (bXorEnabled) {
                bXor = true;
                if (bPaintModeOnly)
                    bSaveXorObj = true;
                setXorMode(mygc, bgColor);
                if (bgc != null) {
                    setXorMode(bgc, bgColor);
                }
            }
            else
                bXor = false;
            return (true);
        case COPYMODE:
            bXor = false;
            bSaveXorObj = false;
            mygc.setPaintMode();
            if (wingc != null)
                wingc.setPaintMode();
            if (lineLw > 1 && lineStroke != null)
                mygc.setStroke(lineStroke);
            if (bgc != null) {
                bgc.setPaintMode();
                if (lineLw > 1 && lineStroke != null)
                    bgc.setStroke(lineStroke);
            }
            return (true);
        case REFRESH:
            setColorModel();
            return (true);
        case WINBUSY:
            if (bPrtCanvas)
                return (true);
            // isBusy = true;
            vnmrIf.setBusyTimer(true);
            return (true);
        case WINFREE:
            if (bPrtCanvas)
                return (true);
            // isBusy = false;
            vnmrIf.setBusyTimer(false);
            // sometimes displayPalette will be covered by this canvas
            // repaint displayPalette will fix the problem
            if (repaintDisplatte) {
                if (appIf != null && (appIf.displayPalette != null))
                    appIf.displayPalette.repaint();
                repaintDisplatte = false;
            }
            /**
             if (!bNative)
               setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));

             ***/
            return (true);
        case BATCHON:
            if (bPrtCanvas)
                return (true);
            if (!bBatch) {
                bLwChangeable = true;
                setDrawLineWidth(1);
                graphFont = defaultFont;
                bFreeFontColor = true;
            }
            bBatch = true;
            if (aipId >= 4)
                return (true);
            if (bFrameAct) {
                if (frameMap.gc != null)
                    mygc = frameMap.gc;
                bgc = null;
                // return (true);
            }
            else {
                if (bimgc != null && aipId <= 1) {
                    mygc = bimgc;
                    bgc = null;
                }
            }
            set_antiAlias();
 
            /***
            if (bImgType && bimgc != null) {
                bBatch = true;
            } else if (bimgc != null && aipId <= 0) {
                bBatch = true;
                mygc = bimgc;
                bgc = null;
            }
            ******/
            return (true);
        case BATCHOFF:
            if (bPrtCanvas || aipId >= 4)
                return (true);
            if (bFrameAct) {
                mygc = frameWinGc;
                bgc = frameMap.gc;
                if (bBatch) {
                    bBatch = false;
                    repaint_canvas();
                    // if (!bCsiWindow && csiCanvas != null)
                    //      csiCanvas.repaint_canvas();
                }
                return (true);
            }
            if (aipId <= 1) {
                if (bImgType && bimgc != null) {
                    mygc = bimgc;
                    bgc = null;
                    bBatch = false;
                    repaint_canvas();
                    // if (!bCsiWindow && csiCanvas != null)
                    //     csiCanvas.repaint_canvas();
                } else {
                    mygc = frameWinGc;
                    bgc = bimgc;
                    // if there is icon, drawIcon will be called later.
                    // if (bimg != null && iconNum <= 0) {
                    if (bBatch) {
                        bBatch = false;
                        repaint_canvas();
                        // if (!bCsiWindow && csiCanvas != null)
                        //      csiCanvas.repaint_canvas();
                    }
                }
            }
            return (true);
        case WINPAINT:  // vnmrbg finished command execution
            if (bPrtCanvas || aipId >= 4)
                return (true);
            vnmrPaint(true);
            return (true);
        case COPY:
            if (bPrtCanvas)
                return (true);
            repaint_canvas();
            return (true);
        case VIMAGE: // draw imagefiles
            if (aipId == 0)
                paintIcon(mygc);
            return (true);
        case DCURSOR:
            if (!bDispCursor)
                return (true);
            if (iconNum <= 0) {
                if (!bFrameAct && wingc != null)
                    draw_dconi_cursor(wingc);
            }
            return (true);
        case CLEAR2:
            if (bPrtCanvas)
                return (true);
            while (bPainting) {
                Thread.yield();
            }
            clearSeries(bTopFrameOn);
            if (bFrameAct) {
                if (!frameMap.bText)
                    frameMap.clear();
            } else if (bimgc != null)
                bimgc.clearRect(0, 0, winW, winH);
            return (true);
        case PRTSYNC:
            // inform vbg that it is the end of print
            String mess = new StringBuffer().append(JFUNC).append(JSYNC)
                    .append(", 5)\n").toString(); // turn on vbg graphics port

            vnmrIf.sendToVnmr(mess);
            bPrinting = false;
            if (bPrtCanvas) {
                if (bDispCursor) {
                   if (bFrameAct && frameMap.bVertical) {
                       draw_vdconi_cursor(mygc, frameMap.bRight);
                       draw_vhair_cursor(mygc, frameMap.bRight);
                   } else {
                       draw_dconi_cursor(mygc);
                       // draw_hair_cursor(printgc);
                   }
                }
                draw_threshold_cursor(mygc);
                if (orgPrintgc != null) {
                   if (printImgBuffer != null) {
                       bSeriesChanged = false;
                       drawSeries(printgc);
                       clearSeries(false);
                       orgPrintgc.drawImage(printImgBuffer,0,0,printW,printH,
                                 0,0,printW,printH, null);
                   }
                   printTextObjects(orgPrintgc, bColorPaint, printW, printH);
                }
                endPrint();
            }
            return (true);
        case MOVIENEXT:
            movieMaker = ImagesToJpeg.get();
            if (movieMaker != null) {
                if (bimg != null)
                    movieMaker.next(bimg, bgOpaque);
                else
                    movieMaker.next();
            }
            return (true);
        case MOVIEEND:
            sendMoreEvent(false);
            movieMaker = ImagesToJpeg.get();
            if(movieMaker != null) {
                  movieMaker.done();
            }
            return (true);
        case FRMPRTSYNC:
            if (frameMap != null && frameMap.bOpen) {
                printThreshold(mygc, frameMap);
            }
            return (true);
        case IMG_SLIDES_START:
            slideCount++;
            return (true);
        case IMG_SLIDES_END:
            slideCount = 0;
            return (true);
        default:
            return (false);
        }
    }

    public boolean g9Func(DataInputStream ins, int func) {
        int k, id, len;
        int p1, p2, p3, p4;
        String str = null;

        if (!bCsiWindow) {
           if (csiCanvas != null)
               return csiCanvas.g9Func(ins, func);
        }

        try {
            id = ins.readInt();
            p1 = ins.readInt();
            p2 = ins.readInt();
            p3 = ins.readInt();
            p4 = ins.readInt();
            len = ins.readInt();
            if (bufLen <= len) {
                bufLen = len + 6;
                buf = new byte[bufLen];
            }
            for (k = 0; k < len; k++)
                buf[k] = ins.readByte();
            buf[len] = 0;
            if (len > 0)
                str = new String(buf, 0, len);
        } catch (Exception e) {
            return (false);
        }
        switch (func) {
        case 1: // annotation file name
            if (len < 1)
                return (true);
            if (annPan == null) {
                annPan = new Annotation();
                annPan.setPrintMode(bPrtCanvas);
            }
            annX = p1;
            annY = p2;
            annW = 0;
            annH = 0;
            annW1 = p3;
            annH1 = p4;
            annPan.loadTemplate(str);
            annPan.setBounds(0, 0, p3, p4);
            annPan.doLayout();
            return (true);
        case 2: //  annotation  size
            if (annPan == null)
                return (true);
            if (annW1 != p3 || annH1 != p4) {
                    annPan.setBounds(0, 0, p3, p4);
                    annPan.doLayout();
                    annW1 = p3;
                    annH1 = p4;
                }
            return (true);
        case 3: //  value 
            if (annPan == null)
                return (true);
            annPan.setValue(id, p1, p2, str);
            return (true);
        case 4: // draw annotation 
            if (annPan == null)
                return (true);
            annX = p1;
            annY = p2;
            /**
            if (bPrtCanvas) {
                annPan.draw(mygc, annX, annY);
                return (true);
            }
            **/
            if (annW != p3 || annH != p4) {
                annW = p3;
                annH = p4;
                annPan.xlayout();
            }
            if (aipId > 0) {
                annPan.draw(mygc, annX, annY);
                mygc.setFont(myFont);
                return (true);
            }
            if (anngc == null) {
                if (bimg != null)
                    anngc = bimg.createGraphics();
                else
                    anngc = (Graphics2D) getGraphics();
            }
            if (anngc != null) {
                anngc.setBackground(bgColor);
                annPan.draw(anngc, annX, annY);
                if (bPrtCanvas)
                    return (true);
                if (bimg != null) {
                    p1 = annX + annW;
                    p2 = annY + annH;
                    mygc.drawImage(bimg, annX, annY, p1, p2, annX, annY, p1,
                            p2, null);
                }
            }
            return (true);
        case 5:
            return (true);
        case 6:
            return (true);
        default:
            return (true);
        }
    }

    private VJTable create_table_series(int id, int x, int y, int w, int h) {
        VJTable table = null;

        if (gSeriesList == null)
            gSeriesList = Collections.synchronizedList(new LinkedList<GraphSeries>());
        for (GraphSeries gs: gSeriesList) {
            if (gs instanceof VJTable) {
                table = (VJTable) gs;
                if (table.getId() == id)
                    break;
            }
            table = null;
        }
        if (table == null) {
            table = new VJTable(id);
            gSeriesList.add(table);
            add(table, JLayeredPane.PALETTE_LAYER);
        }
        if ((x + w) > winW)
            x = winW - w;
        if (x < 0)
            x = 0;
        if ((y + h) > winH)
            y = winH - h;
        if (y < 0)
            y = 0;
        table.setValid(true);
        table.setVisible(true);
        table.setBounds(x, y, w, h);
        table.doLayout();
        return table;
    }

    private void remove_table_series(int id, boolean bAll) {
        if (gSeriesList == null)
            return;
        int num = 0;
        VJTable table = null;

        for (GraphSeries gs: gSeriesList) {
            table = null;
            if (gs instanceof VJTable) {
                num++;
                table = (VJTable) gs;
                if (!bAll) {
                    if (table.getId() != id)
                        table = null;
                }
            }
            if (table != null) {
                table.setValid(false);
                table.setVisible(false);
                if (!bAll)
                    break;
            }
        }
        if (!bAll || num < 6)
            return;

        // clean up VJTable
        while (true) {
            table = null;
            for (GraphSeries gs: gSeriesList) {
                if (gs instanceof VJTable) {
                    table = (VJTable) gs;
                    break;
                }
            }
            if (table == null)
                break;
            gSeriesList.remove(table);
            table.clear();
        }
    }

    public boolean g7Func(DataInputStream ins, int func) {
        int c, k, x, y, w, h, id, len;
        String str = null;

        try {
            id = ins.readInt();
            x = ins.readInt();
            y = ins.readInt();
            w = ins.readInt();
            h = ins.readInt();
            c = ins.readInt();
            len = ins.readInt();
            if (bufLen <= len) {
                bufLen = len + 6;
                buf = new byte[bufLen];
            }
            for (k = 0; k < len; k++)
                buf[k] = ins.readByte();
            buf[len] = 0;
            str = new String(buf, 0, len);
        } catch (Exception e) {
            return (false);
        }
        if (len < 1)
            return (true);

        switch (func) {
           case JTABLE:
                 VJTable t = create_table_series(id, x, y, w, h);
                 if (c > 0)
                    t.setColor(DisplayOptions.getColor(c));
                 t.load(str);
                 return (true);
           default:
                 return (true);
        }
    }


    public void vnmrPaint(boolean bFromVbg) {
        if (bPrtCanvas || aipId >= 4)
            return;
        if (bFrameAct) {
             mygc = frameWinGc;
             bgc = frameMap.gc;
             if (bBatch) {
                 bBatch = false;
                 repaint_canvas();
             }
        }
        else if (aipId <= 0) {
             if (bImgType && bimgc != null) {
                 mygc = bimgc;
                 bgc = null;
                 bBatch = false;
                 repaint_canvas();
             } else {
                 bgc = bimgc;
                 if (wingc != null)
                     mygc = wingc;
                 if (bBatch) {
                     bBatch = false;
                     repaint_canvas();
                 }
             }
        }
        /***
        if (bFromVbg) {
            if (csiCanvas != null)
                 csiCanvas.vnmrPaint(false);
            else if (vbgCanvas != null)
                 vbgCanvas.vnmrPaint(false);
        }
        ***/
    }

    private void setSpectrumWidth(int r) {
        // if (bPrtCanvas) // print screen will set its line width
        //     return;
        if (!bLwChangeable)
            return;
        if (r < 1)
            r = 1;
        r = r + baseSp;
        if (r > 60)
            r = 60;
        if (spLw == r)
            return;
        spLw = r;
        if (r < STROKE_NUM) {
            specStroke = strokeList[r];
            return;
        }
        if (strokeList[STROKE_NUM].getLineWidth() != (float) r)
            strokeList[STROKE_NUM] = new BasicStroke((float) r,
                        BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND);
        specStroke = strokeList[STROKE_NUM];
    }

    private void setSpectrumThickness() {
         if (specMaxStr == null || specMinStr == null)
             return;
         double r = 1.0;
         try {
             r = Double.parseDouble(specRatioStr);
         } catch (NumberFormatException er) {
             r = 1.0;
         }
         int thick = DisplayOptions.getLineThicknessPix(specMinStr, specMaxStr, r);
         setSpectrumWidth(thick);
    }

    private void setLineWidth(int r) {
        // if (bPrtCanvas)
        //     return;
        if (!bLwChangeable)
            return;
        if (r < 1)
            r = 1;
        r = r + baseLw;
        if (r > 60)
            r = 60;
        if (lineLw != r) {
            lineLw = r;
            if (r < STROKE_NUM) {
                lineStroke = strokeList[r];
            }
            else {
                if (strokeList[STROKE_NUM+1].getLineWidth() != (float) r)
                    strokeList[STROKE_NUM+1] = new BasicStroke((float) r,
                        BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND);
                lineStroke = strokeList[STROKE_NUM + 1];
            }
        }
        if (mygc != null)
            mygc.setStroke(lineStroke);
        if (bgc != null)
            bgc.setStroke(lineStroke);
    }

    private void setLineThickness() {
         int thick = DisplayOptions.getLineThicknessPix(lineThickStr);
         setLineWidth(thick);
    }

    private void setDrawLineWidth(int r) {
        if (strokeList == null)
            initStrokeList();
        setSpectrumWidth(r);
        setLineWidth(r);
    }

    private void setGraphicsFont(String name) {
        if (graphFontList == null)
            initFont();
        if (bPrtCanvas)
           return;
        if (name == null)
           return;
        GraphicsFont newGf = graphFont;

        if (!name.equals(newGf.fontName)) {
            newGf = null;
            for (GraphicsFont gf: graphFontList) {
                if (name.equals(gf.fontName)) {
                    newGf = gf;
                    break;
                }
            }
            if (newGf == null) {
                newGf = new GraphicsFont(name);
                graphFontList.add(newGf);
            }
        }
        graphFont = newGf;
        if (frameMap != null)
            myFont = newGf.getFont(frameW, frameH);
        else
            myFont = newGf.getFont(winW, winH);
        
        mygc.setFont(myFont);

        fontMetric = graphFont.fontMetric;
        fontAscent = graphFont.getFontAscent();
        fontDescent = graphFont.getFontDescent();
        fontWidth = graphFont.getFontWdith();
        fontHeight = graphFont.getFontHeight();
        
        fontColor = graphFont.fontColor;
        if (rgbAlpha < 250)
            fontColor = getTransparentColor(fontColor);
        bFreeFontColor = graphFont.isDefault;
        if (bgc != null) {
            bgc.setFont(myFont);
        }
    }

    private void setContrastColor() {
        int r = bgColor.getRed();
        int g = bgColor.getGreen();
        int b = bgColor.getBlue();
        graphColor = Color.yellow;

        if (r > 150) {
            if (g > 120 && b > 120)
                graphColor = VnmrRgb.getColorByName("darkGreen");
        }
        mygc.setColor(graphColor);
        if (bgc != null)
            bgc.setColor(graphColor);
    }

    private void printThreshold(Graphics gc, XMap map) {
        if (frameMap.bText)
            return;
        if (bOverlayed) {
           if (!bTopLayer)
              return;
        }
         int k, c;
         int x1;
         int y0;

         c = 99;
         for (k = 0; k < CURSOR_NUM; k++) {
             y0 = dconi_yCursor[k];
             if (y0 > 0 && dconi_yColor[k] < 18) {
                if (c != dconi_yColor[k]) {
                    c = dconi_yColor[k];
                    setGcColor(gc, c);
                }
                x1 = dconi_yLen[k] + 2;
                gc.drawLine(2, y0, x1, y0);
             }
         }
    }

    public void paint_rubberArea_default(Graphics gc) {
        if (mousePt == null || mousePt2 == null)
            return;
        int y, y2, tmp;
        if (bRubberBox) {
            y = mousePt.y;
            y2 = mousePt2.y;
            if (y < 0 || y > winH)
                return;
            if (y2 > winH)
                y2 = winH;
            if (y2 < 0)
                y2 = 0;
            if (y2 < y) {
                tmp = y;
                y = y2;
                y2 = tmp;
            }
        } else {
            y = 0;
            y2 = winH;
        }
        int x = mousePt.x;
        int x2 = mousePt2.x;
        if (x < 0 || x > winW)
            return;
        if (x2 > winW)
            x2 = winW;
        if (x2 < 0)
            x2 = 0;
        if (x2 < x) {
            tmp = x;
            x = x2;
            x2 = tmp;
        }
        gc.setColor(copyColor);
        if (shadeColor.toString().equals(bgColor.toString())) {
            gc.drawRect(x, y, x2 - x, y2 - y);
        } else {
            gc.fillRect(x, y, x2 - x, y2 - y);
        }
    }

    public void paint_rubberArea(Graphics gc) {
        if (mousePt == null || mousePt2 == null)
            return;
        if (frameY == 0 && frameY2 == 0) {
            paint_rubberArea_default(gc);
            return;
        }
        int y, y2, tmp;
        if (bRubberBox) {
            y = mousePt.y;
            y2 = mousePt2.y;
            if (y < frameY || y > frameY2)
                return;
            if (y2 > frameY2)
                y2 = frameY2;
            if (y2 < frameY)
                y2 = frameY;
            if (y2 < y) {
                tmp = y;
                y = y2;
                y2 = tmp;
            }
        } else {
            y = frameY;
            y2 = frameY + frameH;
        }
        int x = mousePt.x;
        int x2 = mousePt2.x;
        if (x < frameX || x > frameX2)
            return;
        if (x2 > frameX2)
            x2 = frameX2;
        if (x2 < frameX)
            x2 = frameX;
        if (x2 < x) {
            tmp = x;
            x = x2;
            x2 = tmp;
        }

	// in case of vertical side spectrum
	if (bRubberArea) {
          XMap map = frameVector.elementAt(1);
          if (map != null && m_overlayMode == OVERLAY_ALIGN && map.b1D_Y) {
	     x = frameX;
	     x2 = frameX + frameW;
             y = mousePt.y;
             y2 = mousePt2.y;
	     if (y < frameY || y > frameY2) return;
             if (y2 > frameY2) y2 = frameY2;
             if (y2 < frameY) y2 = frameY;
             if (y2 < y) {
                tmp = y;
                y = y2;
                y2 = tmp;
	     }
	  }
	} 
        gc.setColor(copyColor);
        if (shadeColor.toString().equals(bgColor.toString())) {
            gc.drawRect(x, y, x2 - x, y2 - y);
        } else {
            gc.fillRect(x, y, x2 - x, y2 - y);
        }
    }

    public void paint_frame(Graphics2D gc) {

        if (frameNum < 1)
            return;
        boolean xorMode = false;
        boolean bBorder = true;
        if (bOverlayed) {
            if (bXorOvlay && bTopLayer) {
                xorMode = true;
                // gc.setXORMode(bgColor);
                setXorMode(gc, bgColor);
            }
            if (!bActive)
                bBorder = false;
        }

        int x, y;
        XMap map;
        // float alpha;
        Stroke defStroke = gc.getStroke();
        for (int k = 1; k <= frameNum; k++) {
            if (k != frameId) {
                map = frameVector.elementAt(k);
                if (map != null && map.bOpen) {
                   /********
                    alpha = map.getAlpha();
                    if (alpha < 1.0f)
                       gc.setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, alpha));
                    ***********/
                   /********
                    gc.drawImage(map.img, map.x, map.y, map.x + map.width,
                            map.y + map.height, 0, 0, map.width, map.height,
                            null);
                   ********/

                    map.draw(gc);
                    map.drawThreshold(gc);
                    gc.setStroke(defStroke); 
                    if (bShowFrameBox && bBorder) {
                        // gc.setColor(DisplayOptions.getColor("border"));
                        gc.setColor(borderColor);
                        gc.drawRect(map.x, map.y, map.width, map.height);
                    }
                    /****
                    if (alpha < 1.0f)
                       gc.setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.0f));
                    ***/
                }
            }
        }
        if (frameId <= 0)
            return;
        /*
         if (bBatch)
         return;
         */
        map = frameVector.elementAt(frameId);
        if (map == null || !map.bOpen)
            return;
        /***
        gc.drawImage(map.img, map.x, map.y, map.x + map.width, map.y
                + map.height, 0, 0, map.width, map.height, null);
        ****/
        map.draw(gc);
        map.drawThreshold(gc);
        if (xorMode)
            gc.setPaintMode();
        gc.setStroke(defStroke); 
        bShowMoveBorder = false;
        if (bObjDrag && bMoveFrame && bMousePressed) {
            // gc.setColor(DisplayOptions.getColor("highlight"));
            bShowMoveBorder = true;
            gc.setColor(hilitColor);
            gc.drawRect(map.x, map.y, map.width, map.height);
        } else {
            if (!bBorder || bCsiWindow)
                return;
            if (bShowFrameBox) {
                gc.setColor(hilitColor);
                //gc.drawRect( map.x, map.y, map.width-1, map.height-1);
                x = map.x + map.width - 1;
                y = map.y + map.height - 1;
                gc.drawLine(map.x, map.y, x, map.y);
                gc.drawLine(x, map.y, x, y);
                gc.drawLine(map.x, map.y, map.x, y);
                gc.drawLine(map.x, y, x, y);
                return;
            }
            if (activeObj != null)
                return;
            if (map.width >= winW) {
                if (map.height >= winH)
                    return;
            }

            // gc.setColor(DisplayOptions.getColor("hotspot"));
            gc.setColor(hotColor);
            y = map.y;
            x = map.x;
            int size = 8;
            if (frameId == 1)
                size = 10;
            // draw 4 corners
            gc.drawLine(x, y, x + size, y);
            gc.drawLine(x, y, x, y + size);
            x = map.x + map.width - 1;
            gc.drawLine(x - size, y, x, y);
            gc.drawLine(x, y, x, y + size);
            y = map.y + map.height - 1;
            x = map.x;
            gc.drawLine(x, y, x, y - size);
            gc.drawLine(x, y, x + size, y);
            x = map.x + map.width - 1;
            gc.drawLine(x - size, y, x, y);
            gc.drawLine(x, y, x, y - size);
        }
    }

    public void paint_frame_backregion(Graphics2D gc, boolean bForce) {
        if (!bForce && bOverlayed)
            return; 
        for (int k = 1; k <= frameNum; k++) {
             XMap map = frameVector.elementAt(k);
             if (map != null && map.bOpen) {
                 map.drawBackRegion(gc);
             }
        }
    }

    public void previewAnnotation(String f) {
        if (!bCsiWindow) {
           if (csiCanvas != null) {
               csiCanvas.previewAnnotation(f);
               return;
           }
        }

        if (annPan == null)
            annPan = new Annotation();
        annW = winW;
        annH = winH;
        annPan.loadTemplate(f);
        annPan.setBounds(0, 0, winW, winH);
        annPan.doLayout();
        annPan.xlayout();
        if (anngc == null) {
            if (bimg != null)
                anngc = bimg.createGraphics();
            else
                anngc = (Graphics2D) getGraphics();
            anngc.setBackground(bgColor);
        }
        if (anngc == null)
            return;
        if (bgc != null)
            bgc.clearRect(0, 0, winW, winH);
        else
            wingc.clearRect(0, 0, winW, winH);
        annPan.draw(anngc, 0, 0);
        if (bimg != null) {
            wingc.drawImage(bimg, 0, 0, winW, winH, 0, 0, winW, winH, null);
        }
    }

    /********
    private void sendGinEvent(MouseEvent ev, int type) {
        String mess;
        int but, modify;
        modify = ev.getModifiers();
        but = 1;
        if ((modify & (1 << 2)) != 0) // button 3
            but = 3;
        else if ((modify & (1 << 3)) != 0)
            but = 2;
        mpx = ev.getX() - difx;
        mpy = ev.getY() - dify;
        mess = new StringBuffer().append("M@event ").append(JFUNC).append(XEVENT)
                .append(",").append(but).append(",").append(type).append(",")
                .append(mpx).append(",").append(mpy).append(" )\n").toString();
        vnmrIf.sendToVnmr(mess);
    }

    private void sendGinKeyEvent() {
        // gin args: button, event_type, x, y
        //  event_type 1: mouse relessed, 2: mouse pressed, 3: key pressed.
        String mess = new StringBuffer().append("M@event ").append(JFUNC).append(XEVENT)
                .append(", 0, 3, ").append(mpx).append(",").append(mpy).append(")\n").toString();
        vnmrIf.sendToVnmr(mess);
    }

    private void setGinXY(MouseEvent ev) {
        mpx = ev.getX() - difx;
        mpy = ev.getY() - dify;
    }
    ********/

    private void getFrame() {
        Container p = getParent();
        while (p != null) {
            if (p instanceof JFrame) {
                jFrame = (JFrame) p;
                break;
            }
            p = p.getParent();
        }
    }

    private void processGinEvent() {
        int retButton = pressedMouseBut;
        int x = 0;
        int y = 0;

        if (ginEvent == 1) {  // look for release
            if (bMousePressed)
               return;
        }
        else if (ginEvent == 2) { // look for press
            if (!bMousePressed)
               return;
        }
        else {
            if (!bMousePressed)
               retButton = 0;
        }
        if (ginButton > 0 && ginButton <= 3) {
            if (ginButton != retButton)
               return;
        }
        if (frameMap != null) {
            x = mpx - frameMap.x;
            y = mpy - frameMap.y;
            if (x < 0) x = 0;
            if (y < 0) y = 0;
        }
        else {
            x = mpx;
            y = mpy;
        }
        bGinWait = false;
        String mess = new StringBuffer().append("M@event ").append(JFUNC).append(XEVENT)
           .append(",").append(retButton).append(",").append(ginEvent).append(",")
           .append(x).append(",").append(y).append(" )\n").toString();
        vnmrIf.sendToVnmr(mess);
    }

    /***********
    private void startGinEvent() {
        if (!isShowing())
            return;
        if (gp != null) {
            gp.setVisible(true);
            return;
        }
        jFrame = (JFrame) VNMRFrame.getVNMRFrame();
        if (jFrame == null)
            getFrame();
        gp = jFrame.getGlassPane();
        if (gp == null) {
            sendGinKeyEvent();
            return;
        }
        gp.setVisible(true);
        Point p1 = getLocationOnScreen();
        Point p2 = gp.getLocationOnScreen();
        difx = p1.x - p2.x;
        dify = p1.y - p2.y;
        if (gpMouse == null) {
            gpMouse = new MouseAdapter() {
                public void mousePressed(MouseEvent evt) {
                    sendGinEvent(evt, 2);
                }

                public void mouseReleased(MouseEvent evt) {
                    sendGinEvent(evt, 1);
                }

                public void mouseExited(MouseEvent evt) {
                    mpx = -1;
                    mpy = -1;
                }
            };
        }
        if (gpMove == null) {
            gpMove = new MouseMotionAdapter() {
                public void mouseDragged(MouseEvent evt) {
                    setGinXY(evt);
                }

                public void mouseMoved(MouseEvent evt) {
                    setGinXY(evt);
                }
            };
        }

        gp.addMouseListener(gpMouse);
        gp.addMouseMotionListener(gpMove);
    }
    ***********/

    public void drawIcon(DataInputStream in) {
        int x, y, w, h, l, m;
        int hPos, vPos;
        String iconPath;

        x = 0;
        iconPath = null;
        try {
            x = in.readInt();
            y = in.readInt();
            w = in.readInt();
            h = in.readInt();
            hPos = in.readInt();
            vPos = in.readInt();
            l = in.readInt();
            if (l >= bufLen) {
                bufLen = l + 1;
                buf = new byte[bufLen];
            }
            for (m = 0; m < l; m++)
                buf[m] = in.readByte();
            buf[l] = 0;
            iconPath = new String(buf, 0, l);
        } catch (Exception e) {
            return;
        }
        if (!isShowing() || iconPath == null)
            return;
        ImageIcon icon = Util.getGeneralIcon(iconPath);
        if (icon == null)
            return;
        Image img = icon.getImage();
        if (img == null)
            return;
        l = img.getWidth(null);
        if (l > w)
            l = w;
        m = img.getHeight(null);
        if (m > h)
            m = h;
        if (hPos == 0) { // center
            x = x + (w - l) / 2;
        } else if (hPos == 1) { // right
            x = x + w - l;
        }
        if (vPos == 0) { // center
            y = y + (h - m) / 2;
        } else if (vPos == 2) { // bottom
            y = y + h - m;
        }
        if (!bNative) {
            if (!bBatch || bImgType)
                mygc.drawImage(img, x, y, l, m, null);
            if (bgc != null)
                bgc.drawImage(img, x, y, l, m, null);
        }
    }

    public VIcon getIcon() {
        return m_vIcon;
    }

    /*******
    private VIcon getIcon(int x, int y) {
        if (imgVector == null)
            return null;
        for (int i = 0; i < imgVector.size(); i++) {
            VIcon icon = (VIcon) imgVector.elementAt(i);
            if (icon != null) {
                Rectangle r = icon.getBounds();
                if (r.contains(x, y)) {
                    return icon;
                }
            }
        }
        return null;
    }
    *******/

    private VIcon getIcon(String idStr) {
        if (idStr == null || imgVector == null)
            return null;
        for (int i = 0; i < imgVector.size(); i++) {
            VIcon icon = (VIcon) imgVector.elementAt(i);
            if (icon != null && idStr.equals(icon.iconId))
                return icon;
        }
        return null;
    }

    private CanvasObjIF getActiveObj(int x, int y) {
        int i, k;
        CanvasObjIF obj = null;
        CanvasObjIF tmpobj = null;
        Rectangle r;

        if (bImgType && bClickInFrame)
            return null; 
        if (tbVector != null) {
            k = tbVector.size();
            for (i = 0; i < k; i++) {
                tmpobj = tbVector.elementAt(i);
                if (tmpobj != null && tmpobj.isVisible()) {
                    r = tmpobj.getBounds();
                    if (r.contains(x, y)) {
                        if (!tmpobj.equals(preActiveObj)) {
                            if (bActive)
                                TextboxEditor.setEditObj((VTextBox) tmpobj);
                            return tmpobj;
                        }
                        obj = tmpobj;
                        break;
                    }
                }
            }
        }
        if (imgVector != null) {
            k = imgVector.size();
            for (i = 0; i < k; i++) {
                tmpobj = imgVector.elementAt(i);
                if (tmpobj != null && tmpobj.isVisible()) {
                    r = tmpobj.getBounds();
                    if (r.contains(x, y)) {
                        if (!tmpobj.equals(preActiveObj))
                            return tmpobj;
                        if (obj == null)
                            obj = tmpobj;
                        break;
                    }
                }
            }
        }
        if (bActive && (obj != null)) {
            if (obj instanceof VTextBox)
                TextboxEditor.setEditObj((VTextBox) obj);
        }
        return obj;
    }

    /**
     * Draws the icon.
     * @param strValue  the action for the icon (delete, display, redraw, clear),
     *                  and name and bounds for the icon.
     */
    public void drawIcon(String strValue) {
        if (strValue == null)
            return;

        if (bPrtCanvas)
            return;
        strValue = strValue.trim();
        StringTokenizer strTok = new StringTokenizer(strValue, " ,\n");
        String strMode = "";
        if (strTok.hasMoreTokens())
            strMode = strTok.nextToken().toLowerCase();
        if (strMode.startsWith("d")) {
            if (strMode.equals("dimension")) {
                resizeIcon(strValue.substring(9));
                return;
            }
            if (strMode.equals("delete")) {
                deleteIcon(strValue.substring(6));
                return;
            }
            if (strMode.equals("deleteall")) {
                deleteAllIcon();
                return;
            }
            if (strMode.equals("displayall")) {
                showAllIcon();
                return;
            }
            if (strMode.equals("display"))
                displayIcon(strValue.substring(7));
            return;
        }
        String idStr = null;
        if (strTok.hasMoreTokens())
            idStr = strTok.nextToken();
        if (strMode.equals("hide") || strMode.equals("clear")) {
            hideIcon(idStr);
            return;
        }
        if (strMode.equals("hideall") || strMode.equals("clearall")) {
            hideAllIcon();
            return;
        }
        if (strMode.equals("show")) {
            showIcon(idStr);
            return;
        }
        if (strMode.equals("showall")) {
            showAllIcon();
            return;
        }
        if (strMode.equals("redraw")) {
            paintIcon(mygc);
            return;
        }
        if (strMode.equals("on") || strMode.equals("off")) {
            showBox(strValue.substring(2), strMode);
            return;
        }
    }

    public void displayIcon(String strValue) {
        StringTokenizer strTok = new StringTokenizer(strValue, " ,\n");
        String strFile = "";
        String strId = "";
        boolean bNewIcon = false;
        VIcon vIcon = null;
        int x = -1;
        int y = -1;
        int w = 0;
        int h = 0;

        if (strTok.hasMoreTokens())
            strId = strTok.nextToken();
        vIcon = getIcon(strId);
        if (vIcon == null) {
            vIcon = new VIcon();
            vIcon.setIconId(strId);
            bNewIcon = true;
            addIcon(vIcon);
        }

        vIcon.setVisible(false);
        if (strTok.hasMoreTokens())
            strFile = strTok.nextToken();
        try {
            if (strTok.hasMoreTokens()) {
                x = Integer.parseInt(strTok.nextToken());
                if (x >= 0) 
                   vIcon.setX(x);
            }
            if (strTok.hasMoreTokens()) {
                y = Integer.parseInt(strTok.nextToken());
                if (y >= 0) 
                   vIcon.setY(y);
            }
            if (strTok.hasMoreTokens()) {
                w = Integer.parseInt(strTok.nextToken());
                if (w > 1)
                   vIcon.setWidth(w);
            }
            if (strTok.hasMoreTokens()) {
                h = Integer.parseInt(strTok.nextToken());
                if (h > 1)
                   vIcon.setHeight(h);
            }
        } catch (NumberFormatException er) {
            removeIcon(vIcon, false);
            return;
        }
        if (strTok.hasMoreTokens()) {
            strValue = strTok.nextToken();
            if (strValue.equals("mol"))
                 vIcon.setMol(true);
        }
        vIcon.setFile(strFile);
        if (bNative) { // let Vnmrbg draw image
            return;
        }
        // Image image = Toolkit.getDefaultToolkit().createImage(strFile);
        Image image = null;
        try {
            UNFile file = new UNFile(strFile);
            image = ImageIO.read(file);
        } catch (Exception ex) {
            Messages.postInfo("Error: could not read image file " + strFile);
            return;
        }

        if (image == null) {
            removeIcon(vIcon, false);
            return;
        }
        vIcon.setImage(image);
        if (m_vIcon != null) {
            if (bNative)
                showBox(m_vIcon, false);
            m_vIcon.setSelected(false);
        }
        m_vIcon = vIcon;
        get_screen_loc();
        if (molDropPt != null) {
            vIcon.setDefaultSize();
            w = m_vIcon.getWidth();
            h = m_vIcon.getHeight();
            x = molDropPt.x - w / 2;
            y = molDropPt.y - h / 2;
            if (x < 0)
                x = 0;
            if (y < 0)
                y = 0;
            if ((x + w) > winW)
                x = winW - w;
            if ((y + h) > winH)
                y = winH - h;
            vIcon.setX(x);
            vIcon.setY(y);
            sendIconInfo(vIcon);
        }
        else {
            if (bNewIcon) {
                if (w < 2 || h < 2)
                    vIcon.setDefaultSize();
            }
            else {
                if (vIcon.isMol()) {
                    vIcon.adjustSize();
                }
            }
        }
        vIcon.setRatio(winW, winH, false);
        vIcon.setVisible(true);
        bDraging = false;
        iconNum = 1;
        molDropPt = null;
        set_active_obj((CanvasObjIF) vIcon);
        if (bNative)
            showBox(m_vIcon, true);
        else {
            paintIcon(mygc);
        }
    }

    public void paintIcon(Graphics g) {
        if (bNative)
            return;
        repaint_canvas();
    }

    public void addIcon(VIcon vIcon) {
        if (imgVector == null)
            imgVector = new Vector<CanvasObjIF>();
        if (!imgVector.contains(vIcon))
            imgVector.add(vIcon);
        iconNum++;
    }

    private void removeIcon(VIcon icon, boolean bRepaint) {
        if (icon == null || imgVector == null)
            return;
        if (icon.equals(m_vIcon))
            set_active_obj(null);
        imgVector.removeElement(icon);
        if (imgVector.isEmpty())
            iconNum = 0;
        String cmd = new StringBuffer().append("imagefile('delete','").append(
                icon.iconId).append("', '-vj')").toString();
        appIf.sendToVnmr(cmd);
        if (bRepaint)
            paintIcon(mygc);
    }

    private void removeIcon(VIcon icon) {
         removeIcon(icon, true);
    }

    public void deleteIcon(String strIcon) {
        if (strIcon == null || imgVector == null)
            return;
        strIcon = strIcon.trim();
        VIcon icon = getIcon(strIcon);
        if (icon != null)
            removeIcon(icon);
    }

    public void deleteIcon(int x, int y) {
        if (imgVector == null)
            return;
        for (int i = 0; i < imgVector.size(); i++) {
            VIcon icon = (VIcon) imgVector.elementAt(i);
            if (icon != null) {
                Rectangle r = icon.getBounds();
                if (r.contains(x, y)) {
                    removeIcon(icon);
                    return;
                }
            }
        }
    }

    public void deleteIcon() {
        removeIcon(m_vIcon);
    }

    public void deleteAllIcon() {
        if (imgVector == null)
            return;
        while (!imgVector.isEmpty()) {
            int i = imgVector.size();
            if (i <= 0)
               break;
            VIcon icon = (VIcon) imgVector.elementAt(i - 1);
            if (icon != null)
                removeIcon(icon, false);
        }
    }

    private void hideIcon(VIcon icon) {
        if (icon == null || imgVector == null)
            return;
        icon.setVisible(false);
        // if (icon.equals(m_vIcon))
        if (icon == m_vIcon)
            set_active_obj(null);
        String cmd = new StringBuffer().append("imagefile('hide','")
                .append(icon.iconId).append("','-vj')").toString();
        appIf.sendToVnmr(cmd);
        if (!bNative)
            paintIcon(mygc);
    }

    public void hideAllIcon() {
        if (imgVector == null)
            return;
        iconNum = imgVector.size();
        for (int i = 0; i < iconNum; i++) {
            VIcon icon = (VIcon) imgVector.elementAt(i);
            if (icon != null)
                icon.setVisible(false);
        }
        iconNum = 0;
        m_vIcon = null;
        set_active_obj(null);
        setObjNum();
        repaint_canvas();
    }

    public void hideIcon(String idStr) {
        if (idStr == null) {
            hideAllIcon();
            return;
        }
        VIcon vIcon = getIcon(idStr);
        if (vIcon == null)
            return;
        vIcon.setVisible(false);
        if (vIcon == m_vIcon)
            set_active_obj(null);
        repaint_canvas();
    }

    public void showAllIcon() {
        iconNum = 0;
        if (imgVector == null)
            return;
        iconNum = imgVector.size();
        for (int i = 0; i < iconNum; i++) {
            VIcon icon = (VIcon) imgVector.elementAt(i);
            if (icon != null) {
                icon.setVisible(true);
                icon.setSelected(false);
            }
        }
        set_active_obj(null);
        m_vIcon = null;
        setObjNum();
        repaint_canvas();
    }

    public void showIcon(String idStr) {
        if (idStr == null) {
            showAllIcon();
            return;
        }
        VIcon vIcon = getIcon(idStr);
        if (vIcon == null)
            return;
        iconNum = 1;
        vIcon.setVisible(true);
        setObjNum();
        repaint_canvas();
    }

    public void resizeIcon(String strValue) {
        if (strValue == null)
            return;

        StringTokenizer strTok = new StringTokenizer(strValue);
        String strId = "";
        if (strTok.hasMoreTokens())
            strId = strTok.nextToken();

        VIcon vIcon = getIcon(strId);
        if (vIcon == null)
            return;
        Rectangle r = vIcon.getBounds();
        try {
            if (strTok.hasMoreTokens())
                r.x = Integer.parseInt(strTok.nextToken());
            if (strTok.hasMoreTokens())
                r.y = Integer.parseInt(strTok.nextToken());
            if (strTok.hasMoreTokens())
                r.width = Integer.parseInt(strTok.nextToken());
            if (strTok.hasMoreTokens())
                r.height = Integer.parseInt(strTok.nextToken());
        } catch (NumberFormatException er) {
            return;
        }
        vIcon.setBounds(r);
    }

    protected void sendShowCmd(String iconId, String iconMode) {
        String cmd = new StringBuffer().append("imagefile('").append(iconMode)
                .append("', '").append(iconId).append("')").toString();
        vnmrIf.sendToVnmr(cmd);
    }

    protected void showBox(VIcon icon, boolean bSelect) {
        if (icon == null || icon.isSelected() == bSelect)
            return;

        String strId = getIconId(icon);
        icon.setSelected(bSelect);
        if (!icon.equals(m_vIcon))
            addIcon(icon);
        if (bNative) {
            String iconMode;
            if (bSelect)
                iconMode = "on";
            else
                iconMode = "off";
            sendShowCmd(strId, iconMode);
        } else
            paintIcon(mygc);
    }

    protected void showBox(String strIcon, String strMode) {
        if (strIcon == null || strMode == null)
            return;

        if (bNative) {
            sendShowCmd(strIcon, strMode);
            return;
        }

        VIcon icon = getIcon(strIcon);
        if (icon == null)
            return;

        boolean bSelect = false;
        if (strMode.equals("on"))
            bSelect = true;
        icon.setSelected(bSelect);
        if (!icon.equals(m_vIcon))
            addIcon(icon);
        paintIcon(mygc);
    }

    protected String getIconId(VIcon icon) {
        String strIcon = "";
        if (icon == null)
            return strIcon;
        return icon.iconId;
    }

    public void drawTextFile(XMap map, String file, int x, int y, String c,
            int f, String s) {

        if (Util.iswindows())
            file = UtilB.unixPathToWindows(file);

        String path = FileUtil.openPath(file);

        if (path == null)
            return;

        BufferedReader in = null;
        String line;
        Vector<String> lines = new Vector<String>();
        int maxlen = 0;

        try {
            in = new BufferedReader(new FileReader(path));
            while ((line = in.readLine()) != null) {
                if (line.startsWith("#"))
                    continue;

                line.trim();
                lines.add(line);
                if (line.length() > maxlen)
                    maxlen = line.length();
            }
            in.close();
        } catch (Exception e) {
            System.out.println("problem reading file " + path);
            Messages.writeStackTrace(e);
        }

        if (lines.size() <= 0)
            return;
        if (map != null) {
            map.bText = true;
            map.drawTextObj(lines, c, s, f);
            repaint_canvas();
            //vnmrIf.sendToVnmr("frameAction('endDrawText')");
            return;
        }

        Color clr = VnmrRgb.getColorByName(c);
        if (!bColorPaint)
            clr = fgColor;

        int h = winH;
        if (frameId > 0) {
            h = frameH;
        }
        float fsize = (h - y) / lines.size();
        int yinc = (int) fsize;

        int style = Font.PLAIN;
        if (s.equalsIgnoreCase("bold"))
            style = Font.BOLD;

        Font ft;

        if (f > 1) {
            y += f;
            if (s.equalsIgnoreCase("italic"))
                ft = new Font("Dialog", style | Font.ITALIC, f);
            else
                ft = new Font("Dialog", style, f);
        } else {
            /* font size changes with frame size. */

            fsize *= 0.8;
            if (fsize > 60)
                fsize = 60;
            if (fsize < 10)
                fsize = 10;

            f = (int) fsize;

            y += f;

            if (f < 14) {
                if (s.equalsIgnoreCase("italic"))
                    ft = new Font("Dialog", Font.PLAIN | Font.ITALIC, f);
                else
                    ft = new Font("Dialog", Font.PLAIN, f);
            } else {
                if (s.equalsIgnoreCase("italic"))
                    ft = new Font("SansSerif", Font.BOLD | Font.ITALIC, f);
                else
                    ft = new Font("SansSerif", Font.BOLD, f);
            }
        }
        mygc.setColor(clr);
        mygc.setFont(ft);
        if (bgc != null) {
            bgc.setColor(clr);
            bgc.setFont(ft);
        }
        for (int i = 0; i < lines.size(); i++) {
            line = lines.elementAt(i);

            if (y > h)
                y = winH - yinc;
            if (line.length() > 0) {
                mygc.drawString(line, x, y);
                if (bgc != null) {
                    bgc.drawString(line, x, y);
                }
            }

            y += yinc;
        }
        repaint_canvas();
        vnmrIf.sendToVnmr("frameAction('endDrawText')");
    }

    private void addTextSeries(String str, int x, int y, int c, boolean vertical) {
        java.util.List<VJText> list = null;

        bSeriesLocked = true;
        if (defaultMap != null)
             list = defaultMap.getTextList();
        if (!bPrtCanvas) {
             if (frameMap != null)
                list = frameMap.getTextList();
         }
         if (list == null) {
             bSeriesLocked = false;
             return;
         }

        bSeriesChanged = true;
        VJText freeText = null;
        try {
          for (VJText text : list) {
             if (!text.isValid()) {
                 if (freeText == null)
                     freeText = text;
                 if (!bXor)
                     break;
             }
             else if (bXor) {
                 if (text.isXorMode()) {
                     if (text.equals(fontColor, x, y, str, vertical)) {
                         text.setValid(false);
                         bSeriesLocked = false;
                         return;
                     }
                 }
             }
          }
        }
        catch (Exception e) { }

        if (freeText == null) {
             freeText = new VJText();
             list.add(freeText);
        }
        freeText.setValue(fontColor, x, y, str, vertical);
        freeText.setValid(true);
        freeText.setLineWidth(lineLw);
        freeText.setAlpha(alphaSet);
        freeText.setFontAscent(fontAscent);
        freeText.graphFont = graphFont;
        freeText.fontIndex = graphFont.index;
        freeText.setXorMode(bXor);
        freeText.setTopLayer(bTopFrameOn);
        freeText.clipRect = clipRect;
        bSeriesLocked = false;
    }

    public void drawString(String str, int x, int y) {
        Color color = mygc.getColor();
        mygc.setColor(fontColor);
        mygc.drawString(str, x, y);
        if (bgc != null) {
            bgc.setColor(fontColor);
            bgc.drawString(str, x, y);
            bgc.setColor(color);
        }
        mygc.setColor(color);
    }

    public void drawText(String str, int x, int y, int c) {
        y -= fontDescent;
        if (bSaveXorObj || bTopFrameOn) {
            addTextSeries(str, x, y, c, false);
            return;
        }
        drawString(str, x, y);
        /******
        Color color = mygc.getColor();
        mygc.setColor(fontColor);
        mygc.drawString(str, x, y);
        if (bgc != null) {
            bgc.setColor(fontColor);
            bgc.drawString(str, x, y);
            bgc.setColor(color);
        }
        mygc.setColor(color);
        ******/
    }

    public void drawTicText(String str, int x, int y, int c) {
        y -= fontDescent;
        x = x - fontMetric.stringWidth(str) / 2;
        if (bSaveXorObj || bTopFrameOn) {
            addTextSeries(str, x, y, c, false);
            return;
        }
        drawString(str, x, y);
    }

    public void drawPText(String str, int x, int y, int n) {
        y -= fontDescent;
        Color color = mygc.getColor();
        if (n <= 1 || aipId > 0) { // canvas
            mygc.setColor(fontColor);
            mygc.drawString(str, x, y);
            mygc.setColor(color);
        } else if (n == 2) { // buffer 2
            if (b2imgc != null) {
                b2imgc.setColor(fontColor);
                b2imgc.drawString(str, x, y);
                b2imgc.setColor(color);
            }
        }
    }

    public void drawIText(String str, int x, int y, int c) {
        y -= fontDescent;
        Color fcolor = fontColor;
        if (c >= 0)
           fontColor = getTransparentColor(DisplayOptions.getColor(c));
//        else
//           fontColor = DisplayOptions.getColor(c);
        drawString(str, x, y);
        fontColor = fcolor;
    }

    public void drawHText(String str, int x, int y, int c) {
        if (str == null)
            return;
        int w = fontMetric.stringWidth(str) / 2;
        y -= fontDescent;
        x = x - w;
        drawString(str, x, y);
    }

    public void drawJText(String str, int x, int y) {
        if (str == null)
            return;
        drawString(str, x, y);
    }

    private void drawMacVText(String str, int x, int y, int c) {
         if (vtxtImg == null || vtxtImgHeight < fontHeight) {
              vtxtImgWidth = fontWidth * 20;
              vtxtImg = new BufferedImage(vtxtImgWidth, fontHeight, BufferedImage.TYPE_INT_ARGB);
              if (vtxtImg == null)
                 return;
              vtxtImgHeight = fontHeight;
              vtxtgc = vtxtImg.createGraphics();
              vtxtgc.setBackground(bgAlpha);
         }
         vtxtgc.clearRect(0, 0, vtxtImgWidth, vtxtImgHeight);
         vtxtgc.setFont(mygc.getFont());
         vtxtgc.setColor(fontColor);
         // vtxtgc.setColor(mygc.getColor());
         vtxtgc.drawString(str, 0, fontAscent);
         afOrg = mygc.getTransform();
         if (af90 == null)
            af90 = new AffineTransform();
         af90.setToIdentity();
         if (bPrtCanvas)
            af90.translate(x + printTrX, y + printTrY);
         else
            af90.translate(x, y);
         af90.quadrantRotate(3);
         mygc.setTransform(af90);
         mygc.drawImage(vtxtImg, 0, 0, null);
         mygc.setTransform(afOrg);
         if (bgc != null) {
            bgc.setTransform(af90);
            bgc.drawImage(vtxtImg, 0, 0, null);
            bgc.setTransform(afOrg);
         }
    }

    public void drawVText(String str, int x, int y, int c) {
        // setTextColor(c);
        if (bSaveXorObj || bTopFrameOn) {
            addTextSeries(str, x, y, c, true);
            return;
        }
        if (bMacOs) {
            drawMacVText(str, x, y, c);
            return;
        }
        Color color = mygc.getColor();
        mygc.setColor(fontColor);
        afOrg = mygc.getTransform();
        if (af90 == null)
            af90 = new AffineTransform();
        af90.setToIdentity();
        if (bPrtCanvas)
            af90.translate(x + printTrX, y + printTrY);
        else
            af90.translate(x, y);
        af90.quadrantRotate(3);
        // af90.rotate(Math.toRadians(-90));
        mygc.setTransform(af90);
        mygc.drawString(str, 0, fontAscent);
        mygc.setTransform(afOrg);
        if (bgc != null) {
            bgc.setTransform(af90);
            bgc.drawString(str, 0, fontAscent);
            bgc.setTransform(afOrg);
            bgc.setColor(color);
        }
        mygc.setColor(color);
    }

    public void drawIVText(String str, int x, int y, int c) {
        if (c >= 0)
             fontColor = getTransparentColor(DisplayOptions.getColor(c));
        drawVText(str, x, y, c);
    }

    public void drawBox(Point pStart, Point pEnd) {
        if (wingc == null)
            return;
        if (bArgbImg) {
            boolean bRepaint = false;
            if (m_pPrevStart != null && m_pPrevEnd != null)
                bRepaint = true;
            m_pPrevStart = pStart;
            m_pPrevEnd = pEnd;
            if (pStart != null && pEnd != null) {
                m_rectSelect.setFrameFromDiagonal(pStart, pEnd);
                bRepaint = true;
            }
            if (bRepaint)
                repaint_canvas();
            return;
        }
        if (m_pPrevStart != null || pEnd != null) {
            // wingc.setXORMode(bgColor);
            setXorMode(wingc, bgColor);
            wingc.setColor(copyColor);
            if (bgc != null) {
                // bgc.setXORMode(bgColor);
                setXorMode(bgc, bgColor);
                bgc.setColor(copyColor);
            }
            if (m_pPrevStart != null && m_pPrevEnd != null) {
                m_rectSelect.setFrameFromDiagonal(m_pPrevStart, m_pPrevEnd);
                wingc.draw(m_rectSelect);
                if (bgc != null)
                    bgc.draw(m_rectSelect);
            }
            if (pStart != null && pEnd != null) {
                m_rectSelect.setFrameFromDiagonal(pStart, pEnd);
                wingc.draw(m_rectSelect);
                if (bgc != null)
                    bgc.draw(m_rectSelect);
            }
            wingc.setPaintMode();
            if (bgc != null)
                bgc.setPaintMode();
        }
        m_pPrevStart = pStart;
        m_pPrevEnd = pEnd;
    }

    public void eraseBox() {
        drawBox(null, null);
    }

    public void clearFrameBorder() {
        if (frameId <= 0 || wingc == null)
            return;
        // wingc.setXORMode(bgColor);
        setXorMode(wingc, bgColor);
        wingc.setColor(copyColor);
        wingc.drawRect(frameMap.x, frameMap.y, frameMap.width - 1,
                frameMap.height - 1);
    }

    public void drawFrameBox(boolean bClear, Point pt) {
        if (bClear) {
            if (!bObjDrag)
                drawBox(null, null);
            return;
        }
        if (bObjDrag) {
            int x = moveX + pt.x - framePoint.x;
            int y = moveY + pt.y - framePoint.y;
            if (x + moveW >= winW)
                x = winW - moveW - 1;
            if (y + moveH >= winH)
                y = winH - moveH - 1;
            if (x < 0)
                x = 0;
            if (y < 0)
                y = 0;
            if (bMoveFrame) {
                frameMap.x = x;
                frameMap.y = y;
            } else if (activeObj != null) {
                activeObj.setX(x);
                activeObj.setY(y);
            }
            repaint_canvas();
        } else {
            int w = pt.x - framePoint.x;
            int h = pt.y - framePoint.y;
            if (bMoveFrame) {
                if (w < 30 && w > -30)
                    return;
                if (h < 30 && h > -30)
                    return;
            } else {
                if (w < 10 && w > -10)
                    return;
                if (h < 10 && h > -10)
                    return;
            }
            if (pt.x < 0)
                pt.x = 0;
            if (pt.x > winW)
                pt.x = winW;
            if (pt.y < 0)
                pt.y = 0;
            if (pt.y > winH)
                pt.y = winH;
            drawBox(framePoint, pt);
        }
    }

    public void drawRBox(String str, int x, int y) {
        int tw = fontMetric.stringWidth(str) + 2;
        // mygc.setXORMode(bgColor);
        setXorMode(mygc, bgColor);
        mygc.setColor(Color.white);
        mygc.fillRect(x, y, tw, fontHeight);
        mygc.setPaintMode();
        if (bgc != null) {
            //  bgc.setXORMode(bgColor);
            setXorMode(bgc, bgColor);
            bgc.setColor(Color.white);
            bgc.fillRect(x, y, tw, fontHeight);
            bgc.setPaintMode();
        }
    }

    private void dragProc(int mx, int my) {
        int x = moveX + mx - dragX;
        int y = moveY + my - dragY;
        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        if (x + moveW >= winW)
            x = winW - moveW - 1;
        if (y + moveH >= winH)
            y = winH - moveH - 1;
        if (bMoveFrame) {
            frameMap.x = x;
            frameMap.y = y;
        } else if (activeObj != null) {
            activeObj.setX(x);
            activeObj.setY(y);
        }
        repaint_canvas();
    }

    private void dropProc(int x, int y) {
        bObjMove = false;
        bDraging = false;
        bObjDrag = false;
        if (bMoveFrame) {
            bMoveFrame = false;
            frameMap.setSize(moveW, moveH);
            reset_frame_info();
            sendFrameLoc();
            cursorIndex = Cursor.DEFAULT_CURSOR;
            setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
        } else {
            if (activeObj == null)
                return;
            activeObj.setRatio(winW, winH, false);
            if (m_vIcon != null)
                sendIconInfo(m_vIcon);
            cursorIndex = -1;
            set_active_obj(activeObj);
            set_move_cursor(x - scrX, y - scrY, true);
        }
        repaint_canvas();
    }

    public void drawRText(String str, int x, int y) {
        int ty = y + fontAscent;
        int tw = fontMetric.stringWidth(str) + 2;
        mygc.setColor(Color.blue);
        mygc.drawString(str, x, ty);
        // mygc.setXORMode(bgColor);
        setXorMode(mygc, bgColor);
        mygc.setColor(Color.white);
        mygc.fillRect(x - 1, y, tw, fontHeight);
        mygc.setPaintMode();
        if (bgc != null) {
            bgc.setColor(Color.blue);
            bgc.drawString(str, x, ty);
            //  bgc.setXORMode(bgColor);
            setXorMode(bgc, bgColor);
            bgc.setColor(Color.white);
            bgc.fillRect(x - 1, y, tw, fontHeight);
            bgc.setPaintMode();
        }
    }

    public void drawSText(String str, int x, int y, int c) {
        // setTextColor(c);
        if (bannerFont != null) {
            mygc.setFont(bannerFont);
            if (bgc != null) {
                bgc.setFont(bannerFont);
            }
        }
        mygc.drawString(str, x, y + fontAscent * bfontSize);
        mygc.setFont(myFont);
        if (bgc != null) {
            bgc.drawString(str, x, y + fontAscent * bfontSize);
            bgc.setFont(myFont);
        }
    }

    public void drawBanner(String str, int c) {
        // setTextColor(c);
        float fsize;
        FontMetrics fm;
        String d;
        int s = frameW - 16;
        int w = 0;
        int k, h, ygap;
        int bw, bh;
        VStrTokenizer tok = new VStrTokenizer(str, "\\");
        int lines = tok.countTokens();
        if (lines < 1)
            return;
        if (bPrtCanvas) {
            if (frameMap != null) {
               bw = frameMap.prW;
               bh = frameMap.prH;
            }
            else {
               bw = printW;
               bh = printH;
            }
            if (bw < 10) {
               bw = frameW;
               bh = frameH;
            }
            s = bw - 12;
        }
        else {
            bw = frameW;
            bh = frameH;
        }

        fsize = (float) (bh / (lines + 1));
        if (fsize > 160)
            fsize = 160;
        if (fsize < 10)
            fsize = 10;
        myFont = mygc.getFont();
        if (bannerFont == null) {
            // bannerFont = new Font("SansSerif", Font.BOLD | Font.ITALIC, (int)fsize);
            bannerFont = new Font("Dialog", Font.BOLD | Font.ITALIC,
                    (int) fsize);
        } else
            bannerFont = bannerFont.deriveFont(fsize);
        fm = getFontMetrics(bannerFont);
        d = tok.nextToken();
        while (tok.hasMoreTokens()) {
            String d2 = tok.nextToken();
            if (d2.length() > d.length())
                d = d2;
        }

        while (fsize > 10) {
            k = fm.stringWidth(d);
            k = k - s;
            h = (fm.getAscent() + fm.getDescent()) * lines - bh;
            if (k > 0 || h > 0) {
                if (k > 0) {
                    k = k / d.length() + 1;
                    fsize = fsize - (float) k;
                } else if (h > 0) {
                    h = h / lines + 1;
                    fsize = fsize - (float) h;
                }
                if (fsize < 10)
                    fsize = 10;
                bannerFont = bannerFont.deriveFont(fsize);
                fm = getFontMetrics(bannerFont);
            } else
                break;
        }
        // s = bannerFont.getSize();
        w = fm.stringWidth(d);
        s = fm.getAscent() + fm.getDescent();
        ygap = s / 2;
        tok.rewind();
        int x = (bw - w) / 2;
        int y;
        if (x < 0)
            x = 0;
        h = bh - s * lines;
        if (h < 0)
            h = 0;
        y = (h - ygap * (lines - 1)) / 2;
        while (y < s && lines > 1) {
            if (ygap < 1)
                break;
            ygap = ygap / 2;
            y = (h - ygap * (lines - 1)) / 2;
        }
        if (bgc != null)
            bgc.setFont(bannerFont);
        mygc.setFont(bannerFont);
        if (y < 0)
            y = 0;
        y += fm.getAscent();

        while (tok.hasMoreTokens()) {
            d = Util.getMessageString(tok.nextToken());
            if (bgc != null)
                bgc.drawString(d, x, y);
            mygc.drawString(d, x, y);
            y = y + s + ygap;
        }

        if (bgc != null)
            bgc.setFont(myFont);
        mygc.setFont(myFont);
        if (!bPrtCanvas)
            repaint_canvas();
    }

    public void setAnnFont(String data) {
        if (data == null)
            return;
        StringTokenizer tok = new StringTokenizer(data, ",\n");
        String value;
        String name = null;
        String typeStr = null;
        String colorStr = null;
        String sizeStr = null;
        int  size = 8;
        int  type = Font.PLAIN;

        while (tok.hasMoreTokens()) {
             value = tok.nextToken().trim();
             if (name == null)
                 name = value;
             else if (typeStr == null)
                 typeStr = value;
             else if (colorStr == null)
                 colorStr = value;
             else if (sizeStr == null)
                 sizeStr = value;
        }
        if (sizeStr == null)
             return;
        try {
            size = Integer.parseInt(sizeStr);
        }
        catch (NumberFormatException er) { return; }
        if (size < 8)
            size = 8;
        if (size > 160)
            size = 160;
        if (typeStr.equalsIgnoreCase("bold"))
            type = Font.BOLD;
        else if (typeStr.equalsIgnoreCase("italic"))
            type = Font.ITALIC;
        else if (typeStr.equalsIgnoreCase("bold+italic"))
            type = Font.BOLD + Font.ITALIC;
        myFont = new Font(name, type, size);
        mygc.setFont(myFont);


        if (!colorStr.equalsIgnoreCase("null"))
        {
            setAnnColor(colorStr);
            fontColor = getTransparentColor(fontColor);
//            fontColor = getTransparentColor(DisplayOptions.getColor(colorStr));
        }
        bFreeFontColor = true;
    }

    public void setAnnColor(String data) {
        if (data == null)
            return;
        colorId = -3;
        icolorId = -3;
        try {
           int id = Integer.parseInt(data);
           graphColor = getColor(id);
        } catch (NumberFormatException er) {
           graphColor = DisplayOptions.getColor(data.trim());
        }
//        graphColor = DisplayOptions.getColor(data.trim());
        if (rgbAlpha < 250) {
            int r = graphColor.getRed();
            int g = graphColor.getGreen();
            int b = graphColor.getBlue();
            graphColor = new Color(r, g, b, rgbAlpha);
        }

        fontColor = graphColor;
        bFreeFontColor = true;
        mygc.setColor(graphColor);
        if (bgc != null)
            bgc.setColor(graphColor);
    }

    public void setBannerFont(int size) {
        if (bfontSize == size)
            return;
        // float k = (float) (size * fontHeight);
        bannerFont = new Font("SansSerif", Font.BOLD | Font.ITALIC, size
                * fontHeight);
        // bannerFont = myFont.deriveFont(k);

        bfontSize = size;
    }

    public Dimension getWinSize() {
        // return new Dimension(winW, winH);
        return getSize();
    }

    public void setTearGC(Graphics gt1, Graphics gt2) {
        // tgc = gt1;
        // tgc2 = gt2;
    }

/*
    public void registerTearOff(VnmrTearCanvas b) {
    }
 */

    public void setTextColor(int c) {
        /***********
        int r, g, b;

        Color color = DisplayOptions.getColor(c);
        r = color.getRed();
        g = color.getGreen();
        b = color.getBlue();
        textColor = new Color(r, g, b, rgbAlpha);

        ***********/
    }

    public void refresh() {
        if (!isResizing && bNative) {
            vnmrIf.sendToVnmr(new StringBuffer().append("X@copy ").append(JEVENT)
                    .append(PAINT).append(") ").toString());
        }
    }

    private void nativePaint(Graphics g) {
        if (!isShowing())
            return;
        bk1 = null;
        bk2 = null;
        if (Util.isMenuUp()) {
            JComponent comp = Util.getPopupMenu();
            if (comp != null && comp.isShowing()) {
                Rectangle r = comp.getBounds();
                Point pt = comp.getLocationOnScreen();
                Point pt1 = getLocationOnScreen();
                int w = pt.x - pt1.x;
                int s = 0;
                if (w > 2) {
                    bk1 = new StringBuffer().append("X@copy ").append(JEVENT).append(
                            XCOPY).append(", 0, 0, ").append(w).append(", ")
                            .append(winSize.height).append(") ").toString();
                    s = 1;
                }
                int x = pt.x + r.width - pt1.x;
                if (x < 0)
                    x = 0;
                w = winSize.width - x;
                if (w > 2) {
                    bk2 = new StringBuffer().append("X@copy ").append(JEVENT).append(
                            XCOPY).append(", ").append(x).append(", 0, ")
                            .append(w).append(", ").append(winSize.height)
                            .append(") ").toString();
                    s = 1;
                }
                if (s > 0) {
                    ComponentEvent e1 = new ComponentEvent(this,
                            ComponentEvent.COMPONENT_SHOWN);
                    Toolkit.getDefaultToolkit().getSystemEventQueue()
                            .postEvent(e1);
                }
            }
            return;
        }
        if (appIf == null)
            return;

        if (isResizing)
            return;
        if (appIf.isResizing()) {
            Point pt = getLocationOnScreen();
            Rectangle r = g.getClipBounds();
            int y = pt.y + r.height;
            if (y < loc.y)
                return;
            if (y > oldY) {
                oldY = y;
                return;
            }
            oldY = y;
            y = pt.y - loc.y + 1;
            if (y < 0)
                y = 0;
            int h = r.height + 6;
            if (y + h > winH)
                h = winH - y;
            bk1 = new StringBuffer().append("X@copy ").append(JEVENT).append(XCOPY)
                    .append(", 0, ").append(y).append(", ").append(
                            winSize.width).append(", ").append(h).append(") ")
                    .toString();
            ComponentEvent e1 = new ComponentEvent(this,
                    ComponentEvent.COMPONENT_SHOWN);
            Toolkit.getDefaultToolkit().getSystemEventQueue().postEvent(e1);
            return;
        }
        if (bSuspend)
            return;
        // append false event to fix the timing issue.
        ComponentEvent e = new ComponentEvent(this,
                ComponentEvent.COMPONENT_SHOWN);
        Toolkit.getDefaultToolkit().getSystemEventQueue().postEvent(e);
    }

    private void adjustObjects(boolean bAdjust) {
        int i, n;
        CanvasObjIF obj;
        if (imgVector != null) {
            n = imgVector.size();
            for (i = 0; i < n; i++) {
                obj = imgVector.elementAt(i);
                if (obj != null)
                    obj.setRatio(winW, winH, bAdjust);
            }
        }
        if (tbVector != null) {
            n = tbVector.size();
            for (i = 0; i < n; i++) {
                obj = tbVector.elementAt(i);
                if (obj != null)
                    obj.setRatio(winW, winH, bAdjust);
            }
        }
        if (activeObj != null) // readjust active obj info
            set_active_obj(activeObj);
    }

    private void dispIcons(Graphics2D g) {
        if (imgVector == null)
            return;
        int n = imgVector.size();
        iconNum = 0;
        if (n < 1)
            return;
        Graphics2D gc = (Graphics2D) g.create();
        Object orgHint = gc.getRenderingHint(RenderingHints.KEY_INTERPOLATION);
        gc.setRenderingHint(RenderingHints.KEY_INTERPOLATION,
                RenderingHints.VALUE_INTERPOLATION_BICUBIC);
        for (int i = 0; i < n; i++) {
            CanvasObjIF b = imgVector.elementAt(i);
            if (b != null && b.isVisible()) {
                iconNum++;
                if (bDraging)
                    b.paint(gc, hilitColor, bDraging);
                else
                    b.paint(gc, hotColor, bDraging);
            }
        }
        if (orgHint != null)
            gc.setRenderingHint(RenderingHints.KEY_INTERPOLATION, orgHint);
    }

    private void dispTextBox(Graphics2D gc) {
        objNum = iconNum;
        if (tbVector == null)
            return;
        for (int i = 0; i < tbVector.size(); i++) {
            CanvasObjIF b = tbVector.elementAt(i);
            if (b != null && b.isVisible()) {
                objNum++;
                if (bDraging)
                    b.paint(gc, hilitColor, bDraging);
                else
                    b.paint(gc, hotColor, bDraging);
            }
        }
    }

    private void dispSeries(Graphics g) {
        if (gSeriesList == null)
            return;
        boolean bPaint = false;
        for (GraphSeries gs: gSeriesList) {
            if (gs.isValid()) {
                 bPaint = true;
                 break;
            }
        }
        if (bPaint)
            super.paint(g);
    }

    private void VnmrRepaint() {
        String mess = new StringBuffer().append(JFUNC).append(PAINTALL)
                       .append(")\n").toString();
        vnmrIf.sendToVnmr(mess);
    }

    private void printImageObjects(Graphics2D g, boolean bColor, int w, int h) {
        int i, n;
        if (imgVector == null)
            return;
        n = imgVector.size();
        if (n < 1)
            return;
        Graphics2D gc;
        /*******
        if (bPrintImg) {
            gc = (Graphics2D) g.create();
            gc.setRenderingHint(RenderingHints.KEY_INTERPOLATION,
                RenderingHints.VALUE_INTERPOLATION_BICUBIC);
        }
        else
        ******/
            gc = g;
        for (i = 0; i < n; i++) {
            CanvasObjIF obj = imgVector.elementAt(i);
            if (obj != null && obj.isVisible()) {
                obj.setRatio(winW, winH, false);
                obj.print(gc, bColor, w, h);
            }
        }
    }

    private void printTextObjects(Graphics2D g, boolean bColor, int w, int h) {
        int i, n;
        CanvasObjIF obj;
        if (g == null)
            return;
        if (tbVector != null) {
            n = tbVector.size();
            for (i = 0; i < n; i++) {
                obj = tbVector.elementAt(i);
                if (obj != null && obj.isVisible())
                    obj.print(g, bColor, w, h);
            }
        }
    }

    public void paintImmediately(int x,int y,int w, int h) {
       bPainting = true;
       bWaitOnPaint = false;
       if (bBatch && !bGinFunc) {
           bPainting = false;
           return;
       }
       super.paintImmediately(x, y, w, h);
       bPainting = false;
    }


    public void paint(Graphics g) {
        bWaitOnPaint = false;
        if (bNative) {
            nativePaint(g);
            return;
        }
        bSeriesChanged = false;
        if (bPrtCanvas) {
            bPainting = false;
            return;
        }
        Graphics2D g2d = (Graphics2D) g;
        if (bPaintModeOnly && DisplayOptions.isAAEnabled())
            g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
            
        paintClip = g.getClipBounds();
        defaultStroke = g2d.getStroke();
        if (bimg == null) {
            super.paint(g);
        } else {
            if (frameNum < 1)
                g.drawImage(bimg, 0, 0, winW, winH, 0, 0, winW, winH, null);
        }
        if (bOverlayed) {
            if (bBottomLayer && viewIf != null)
                viewIf.paint_frame_backregion(g2d);
        }
        else
            paint_frame_backregion(g2d, false);
        if (bkRegionW > 10 && bkRegionH > 10) {
            if (!bOverlayed || bBottomLayer) {
               g2d.setColor(bkRegionColor);
               g2d.fillRect(bkRegionX, bkRegionY, bkRegionW, bkRegionH);
            }
        } 
        if (iconNum > 0)
            dispIcons(g2d);
        if (myFont != null) 
            g2d.setFont(myFont);
        drawSeries(g2d);
        draw_threshold_cursor(g2d);
        g2d.setStroke(defaultStroke);
        if (frameNum > 0)
            paint_frame(g2d);
        bPainting = false;
        dispTextBox(g2d);
        dispSeries(g);
        if (bRubberArea)
            paint_rubberArea(g);
        if (bDispCursor) {
            if (bFrameAct && frameMap.bVertical) {
                draw_vdconi_cursor(g2d, frameMap.bRight);
                draw_vhair_cursor(g2d, frameMap.bRight);
            } else {
                draw_dconi_cursor(g2d);
                draw_hair_cursor(g2d);
            }
        }
        if (bArgbImg) {
            if (m_pPrevStart != null && m_pPrevEnd != null) {
                g2d.setColor(copyColor);
                g2d.draw(m_rectSelect);
            }
        }
        if(b3planes)
            draw_3planes_cursors(g);
        if (slideCount > 0) {
            slideCount--;
            vnmrIf.sendToVnmr("M@xstart\n");
        }
    }

    private BufferedImage canvasImg = null;
    private int canvasImgW = 0;
    private int canvasImgH = 0;
    private Graphics canvasImgGc = null;

    public BufferedImage getCanvasImage() {
        if (canvasImg == null || canvasImgW != winW || canvasImgH != winH) {
           try {
               canvasImg = new BufferedImage(winW, winH, BufferedImage.TYPE_INT_RGB);
           }
           catch (OutOfMemoryError e) {
                canvasImg = null;
                canvasImgW = 0;
                return null;
           }
           canvasImgW = winW;
           canvasImgH = winH;
           canvasImgGc = canvasImg.createGraphics();
        }
        if (canvasImg == null)
            return null;
        canvasImgGc.setColor(bgOpaque);
        canvasImgGc.fillRect(0, 0, winW, winH);
        paint(canvasImgGc);
        return canvasImg;
    }

    public void showImage(String name) {
        if (name == null)
             return;
        String path = FileUtil.openPath(name);
        if (path == null)
             return;
        ImageIcon icon = new ImageIcon(path);
        Image img = icon.getImage();
        Graphics gc = null;

        int w = img.getWidth(null);
        int h = img.getHeight(null);

        if (frameId > 0) {
            XMap map =  frameVector.elementAt(1);
            if (!map.bOpen)
                map =  frameVector.elementAt(frameId);
            if (map.width < winW || map.height < winH)
                map.setSize(winW, winH);
            map.clear();
            gc = map.gc;
        }
        else
            gc = bimgc;

        if (gc == null)
            return;
        gc.drawImage(img, 0, 0, w, h, 0, 0, w, h, null);
        repaint();
    }

    private void adjustFont(int w, int h) {

        if (graphFontList == null)
            initFont();
        myFont = graphFont.getFont(w, h);
        // fontMetric = getFontMetrics(myFont);
        fontMetric = graphFont.fontMetric;
        fontAscent = fontMetric.getAscent();
        fontDescent = fontMetric.getDescent();
        fontWidth = fontMetric.stringWidth("ppmm9") / 5;
        fontHeight = fontAscent + fontDescent;
    }

    public void setGeom(int x, int y, int w, int h, boolean bFromExp) {
        if (graphFontList == null)
            initFont();
        if (w <= MINWH && h <= MINWH) {
            if (!bCsiWindow)
               return;
            if (w < MINWH)
               w = MINWH;
            if (h < MINWH)
               h = MINWH;
        }
        if (bFromExp)
            adjustObjects(false);
        else
            adjustObjects(true);
        winW = w;
        winH = h;
        String mess = new StringBuffer().append(JFUNC).append(JSYNC)
                        .append(", 3)\n").toString();
        // set vbg window resizeMode on
        vnmrIf.sendToVnmr(mess);
        if (frame_1 != null) {
            if (frame_1.width <= MINWH || frame_1.height <= MINWH) {
               frame_1.x = 0;
               frame_1.y = 0;
               frame_1.setSize(w, h);
            }
        }

        mess = new StringBuffer().append(JFUNC).append(VSIZE).append(
                ",").append(w).append(",").append(h).append(", ").append(x)
                .append(", ").append(y).append(") ").toString();
        vnmrIf.sendToVnmr(mess);
        // if (frameId > 0 && (w > 0 && h > 0)) {
            XMap map;
            if (bFromExp) {
                mess = new StringBuffer().append(JFUNC).append(FRMSIZE).append(
                    ",0,").append(x).append(",").append(y).append(", ").append(w)
                    .append(", ").append(h).append(") ").toString();
                vnmrIf.sendToVnmr(mess);
            }
            for (int k = 1; k <= frameNum; k++) {
                map = frameVector.elementAt(k);
                if (map != null && map.bEnable) {
                    map.adjustSize(w, h);
                    if (bFromExp)
                        map.bNewSize = true; // send size to Vbg
                    if (!map.bText)
                        sendFrameSize(map);
               }
            }
        // }
        if (frame_1 != null) {
            bimg = frame_1.img;
            bimgc = frame_1.gc;
        }
        mess = new StringBuffer().append(JFUNC).append(JSYNC)
                .append(", 4)\n").toString();
        vnmrIf.sendToVnmr(mess);
    }

    public void setGeom(int x, int y, int w, int h) {
        Dimension dim = getSize();
        if (dim.width == w && dim.height == h) {
            if (bCsiWindow && bRepaintCsi && (wingc != null)) {
                bRepaintCsi = false;
                VnmrRepaint();
            }
            return;
        }
        setGeom(x, y, w, h, true);
    }

    private void sendFontSize(int w, int h, int ascent, int descent) {
        String mess = new StringBuffer().append(JFUNC).append(FONTSIZE).append(
                  ",").append(w).append(",").append(h).append(",").append(
                  ascent).append(",").append(descent).append(")\n").toString();
        vnmrIf.sendToVnmr(mess);
    }

    private void calGeom(boolean toVnmr) {
        winSize = getSize();
        if (!appIf.isShowing())
            return;
        if (!bNative) {
            if (winSize.width < MINWH)
                winSize.width = MINWH;
            if (winSize.height < MINWH)
                winSize.height = MINWH;
        }
        int w = winSize.width;
        int h = winSize.height;
        if (!isShowing()) {
            Rectangle r = getBounds();
            winX = r.x;
            winY = r.y;
            Container pcomp = getParent();
            while (pcomp != null) {
                if (pcomp == jFrame)
                    break;
                r = pcomp.getBounds();
                winX = winX + r.x;
                winY = winY + r.y;
                pcomp = pcomp.getParent();
            }
            loc = jFrame.getLocationOnScreen();
            loc.x += winX;
            loc.y += winY;
        } else {
            loc = getLocationOnScreen();
            Point pt2 = jFrame.getLocationOnScreen();
            winX = loc.x - pt2.x + 1;
            winY = loc.y - pt2.y;
            scrX = loc.x;
            scrY = loc.y;
            scrX2 = scrX + w;
            scrY2 = scrY + h;
        }
        if (!bNative) {
            winX = 0;
            winY = 0;
        }
        if (w < 5)
            w = 200;
        if (h < 5)
            h = 200;

        if (w != winW || h != winH) {
            bResized = true;
            winH = w;
            winW = h;
        }
        if (!toVnmr)
            return;
       /**
        String mess = new StringBuffer().append(JFUNC).append(VSIZE).append(
                ",").append(w).append(",").append(h).append(", ").append(winX)
                .append(", ").append(winY).append(") ").toString();
        vnmrIf.sendToVnmr(mess);
       ***/

        if (!bNative) {
            // sendFontSize(fontWidth, fontHeight, fontAscent, fontDescent);
        }
    }

    public void setGraphicRegion() {
        if (appIf == null) {
            appIf = Util.getAppIF();
            if (appIf == null)
                return;
        }
        if (jFrame == null) {
            jFrame = (JFrame) VNMRFrame.getVNMRFrame();
            if (jFrame == null)
                getFrame();
        }
        if (viewIf == null) {
            viewIf = Util.getViewArea();
            if (!bCsiWindow && viewIf != null) {
                viewIf.setCanvasComp(winId, this);
            }
        }
        calGeom(true);
        if (!isShowing())
            return;
        if (appIf.vToolPanel == null)
            return;

        int k, cs, x, x2, y;

        // regionX = winX;
        // regionY2 = winY + winH;
        regionX2 = winX + winW;
        regionY = winY;
        x = winX;
        y = winY + winH;

        JComponent tabPan = (JComponent) appIf.vToolPanel;
        Container pp = tabPan.getParent();
        while (pp != null) {
            if (pp instanceof JLayeredPane)
                break;
            pp = pp.getParent();
        }
        if (pp == null)
            return;
        int layer = JLayeredPane.getLayer(appIf.expViewArea);
        int tabLayer = 0;
        int ctlLayer = 0;
        if (tabPan.isShowing())
            tabLayer = JLayeredPane.getLayer(tabPan);
        if (appIf.paramPanel != null && appIf.paramPanel.isShowing())
            ctlLayer = JLayeredPane.getLayer(appIf.paramPanel);
        Point pt;
        cs = 0;
        x2 = winX;
        if (tabLayer > layer) {
            pt = tabPan.getLocationOnScreen();
            k = pt.x + tabPan.getSize().width;
            if (k > loc.x) {
                x = winX + k - loc.x;
                cs = 1;
            }
        }
        if (ctlLayer > layer) {
            pt = appIf.paramPanel.getLocationOnScreen();
            x2 = winX + pt.x - loc.x - 2;
            k = loc.y + winH;
            if (pt.y < k) {
                k = pt.y - loc.y;
                if (k < 0)
                    k = 0;
                y = winY + k;
                cs = 1;
            }
        }
        if (x != regionX || y != regionY2) {
            regionY2 = y;
            regionX = x;
            cs = 1;
        }
        if (bNative) {
            String mess = new StringBuffer().append("X@region ").append(JREGION)
                    .append(XREGION).append(",").append(regionX).append(",")
                    .append(regionY).append(", ").append(regionX2).append(", ")
                    .append(regionY2).append(")\n").toString();
            vnmrIf.sendToVnmr(mess);
            if (ctlLayer > layer && tabLayer < layer) {
                y = loc.y + winH;
                if (winW > 2 && winH > 2) {
                    if (x2 > regionX2)
                        x2 = regionX2;
                    mess = new StringBuffer().append("X@region ").append(JREGION)
                            .append(XREGION2).append(",").append(winX).append(
                                    ",").append(regionY2).append(", ").append(
                                    x2).append(", ").append(y).append(")\n")
                            .toString();
                    vnmrIf.sendToVnmr(mess);
                }
            }
        } else {
            if (wingc != null && cs > 0)
                wingc.setClip(regionX, 0, winW, regionY2 - regionY);
        }
        /*
         if (!bNative && frameId > 0) {
         reset_frame_info();
         sendFrameSize();
         }
         */
    }

    private void initFont() {
         if (graphFontList == null) {
              graphFontList = VFontFactory.getList();
              for (GraphicsFont gf: graphFontList) {
                  if (gf.fontName.equals(GraphicsFont.defaultName)) {
                      defaultFont = gf;
                      break;
                  }
              }
              if (defaultFont == null) {
                  defaultFont = new GraphicsFont();
                  graphFontList.add(defaultFont);
              }
              graphFont = defaultFont;
         }

         if (bPrtCanvas) {
            if (printgc != null)
               myFont = printgc.getFont();
         }
         else {
            myFont = defaultFont.font;
         }
         if (myFont == null) {
            if (wingc != null)
               myFont = wingc.getFont();
            else
               myFont = new Font("Dialog", Font.PLAIN, 12);
         }
         // fontMetric = wingc.getFontMetrics();
         fontMetric = getFontMetrics(myFont);
         fontAscent = fontMetric.getAscent();
         fontDescent = fontMetric.getDescent();
         fontWidth = fontMetric.stringWidth("ppmm9") / 5; // average width
         fontHeight = fontAscent + fontDescent;
         org_font = myFont;
         org_fontWidth = fontWidth;
         org_fontHeight = fontHeight;
    }

    private void nativeResize(boolean bRedraw) {
        setGraphicRegion();
        isResizing = false;
        if (!bSuspend && !bReshow) {
            String mess;
            if (bRedraw)
                mess = new StringBuffer().append(JFUNC).append(REDRAW)
                        .append(")\n").toString();
            else
                mess = new StringBuffer().append("X@copy ").append(JEVENT)
                        .append(PAINT).append(") ").toString();
            bResized = false;
            vnmrIf.sendToVnmr(mess);
        }
        if (oldX != winX)
            repaintDisplatte = true;
        oldX = winX;
        oldY = loc.y + 4;
    }

    private void javaResize() {
        if (bPrtCanvas)
            return;
        if (wingc != null)
            wingc.dispose();
        wingc = (Graphics2D) getGraphics();
        if (wingc == null)
            return;
        wingc.setBackground(bgColor);
        frameWinGc = (Graphics2D) getGraphics();
        frameWinGc.setBackground(bgColor);
        transX = 0;
        transY = 0;
        if (!bFrameAct && aipId == 0 && !bBatch) {
            mygc = wingc;
        }
        if (defaultStroke == null)
            defaultStroke = wingc.getStroke();

        String mess;
        winSize = getSize();
        if (winSize.width <= MINWH || winSize.height <= MINWH)
            return;
        winW = winSize.width;
        winH = winSize.height;
        maxW = winW + 100;
        maxH = winH + 100;
        create_backup();
        clearAllSeries();
        if (anngc != null) {
            anngc.dispose();
            anngc = null;
        }
        if (bImgType) {
            adjustFont(winW, winH);
            wingc.setFont(myFont);
            setFont(myFont);
        }
        if (frameId < 1) {
            frameW = winW;
            frameH = winH;
        }
        if (bBacking)
            create_backup2();
        if (bImgType && aipId <= 1) {
            if (bimgc != null)
                mygc = bimgc;
        }
        setGraphicRegion();
        setGeom(winX, winY, winW, winH, false);
        if (appIf != null && (!appIf.isResizing())) {
            if (!bSyncOvlay || bRepaintCsi) {
                bRepaintCsi = false;
                VnmrRepaint();
            }
        }
        if (!bCsiWindow) {
            if (frameId < 1)
                activateFrame(1);
            else
                reset_frame_info();
        }
        else
            reset_frame_info();
        bLwChangeable = true;
        setDrawLineWidth(1);
        bTopFrameOn = false;
        graphTool.setTopLayerOn(bTopFrameOn);
    }

    public boolean getPrintMode() {
        if (bPrtCanvas)
             return bPrintMode;
        if (prtCanvas != null)
             return (prtCanvas.getPrintMode());
        return false;
    }

    public void setPrintMode(boolean s) {
        if (!bPrtCanvas) {
            if (s) {
                if (prtCanvas == null)
                     createPrintCanvas();
            }
            if (prtCanvas != null)
                prtCanvas.setPrintMode(s);
            return;
        }
        bPrintMode = s;
        if (!s) {
            bPrinting = false;
            if (printImgBuffer != null) {
               if (printgc != null)
                   printgc.dispose();
               printgc = null;
               printImgBuffer = null;
            }
            if (prtframegc != null) {
               prtframegc.dispose();
               prtframegc = null;
            }
            if (orgPrintgc != null)
               orgPrintgc.dispose();
            orgPrintgc = null;
            wingc = null;
            anngc = null;
        }
        if (annPan != null)
            annPan.setPrintMode(s);
    }

    public BufferedImage getPrintImage() {
        if (!bPrtCanvas) {
            if (prtCanvas != null)
                return prtCanvas.getPrintImage();
            return null;
        }
        return null;
        // return printImgBuffer;
    }

    public VnmrCanvas getPrintCanvas() {
        return prtCanvas;
    }

    private void createPrintCanvas()  {
        if (prtCanvas == null) {
              prtCanvas = new VnmrCanvas(winId, vnmrIf, bImgType, true);
              prtCanvas.setVisible(false);
              add(prtCanvas);
        }

    }

    public boolean isPrinting() {
        if (bPrtCanvas)
             return bPrinting;
        if (prtCanvas != null)
             return (prtCanvas.isPrinting());
        return false;
    }

    public void printCanvas() {
        if (!bPrtCanvas) {
            if (prtCanvas != null)
                prtCanvas.printCanvas();
            else
                endPrint();
            return;
        }

        if (printgc == null || printImgBuffer == null) {
            bPrintMode = false;
            endPrint();
            return;
        }
        if (bPrinting)
            return;
        bPrinting = true;
        printgc.clearRect(0, 0, printW, printH);
        // ask vbg to start print
        String mess = new StringBuffer().append(JFUNC).append(JSYNC).append(
                ", 2)\n").toString();
        vnmrIf.sendToVnmr(mess);
    }


    public void copyColors(byte[] r, byte[] g, byte[] b)
    {
         for (int i = 0; i < colorSize; i++) {
              redByte[i] = r[i];
              grnByte[i] = g[i];
              bluByte[i] = b[i];
         }
    }

    public void copyXCursors(int[] xs, int[] lens, int[] colors)
    {
         for (int i = 0; i < xs.length; i++) {
              dconi_xCursor[i] = xs[i];
              dconi_xLen[i] = lens[i];
              dconi_xColor[i] = colors[i];
              dconi_xStart[i] = 2;
         }
    }

    public void copyYCursors(int[] ys, int[] lens, int[] colors)
    {
         for (int i = 0; i < ys.length; i++) {
              dconi_yCursor[i] = ys[i];
              dconi_yLen[i] = lens[i];
              dconi_yColor[i] = colors[i];
              dconi_yStart[i] = 2;
         }
    }

    public void copyParameters(VnmrCanvas dst) {
            if (dst == null)
                return;
            dst.winW = winW;
            dst.winH = winH;
            dst.bgIndex = bgIndex;
            dst.fgIndex = fgIndex;
            dst.bImgType = bImgType;
            dst.bImgBg = bImgBg;
            dst.imgColor = imgColor;
            dst.imgGrayNums = imgGrayNums;
            dst.imgGrayStart = imgGrayStart;
            dst.frameNum = frameNum;
            dst.frameId = frameId;
            dst.aipId = aipId;
            dst.bgColor = bgAlpha;
            dst.bgAlpha = bgAlpha;
            dst.bgOpaque = bgOpaque;
            dst.frameVector = frameVector;
            dst.xmapVector = xmapVector;
            dst.imgVector = imgVector;
            dst.tbVector = tbVector;
            dst.bTranslucent = bTranslucent;
            dst.bDispCursor = bDispCursor;
            dst.bMacOs = bMacOs;
            dst.bOverlayed = bOverlayed;
            dst.bTopLayer = bTopLayer;
            dst.specMaxStr = specMaxStr;
            dst.specMinStr = specMinStr;
            dst.specRatioStr = specRatioStr;
            dst.lineThickStr = lineThickStr;
            if (annPan != null) {
                if (dst.annPan == null)
                    dst.annPan = new Annotation();
                dst.annPan.loadTemplate(annPan.getTemplateFile());
            }
    }

    private Float getFloat(String key)
    {
        Object obj = hs.get(key);
        if (obj == null || !(obj instanceof Float))
             return null;
        return (Float) obj;
    }

    private String getString(String key)
    {
        Object obj = hs.get(key);
        if (obj == null || !(obj instanceof String))
             return null;
        return (String) obj;
    }


    private void configPrintScreen() {
        sshare = Util.getSessionShare();
        boolean  old_bDispCursor = bDispCursor;
        bDispCursor = false;
        if (sshare == null)
            return;
        hs = sshare.userInfo();
        if (hs == null)
            return;
        Float fv;
        boolean bPrintToFile = false;
        bDispCursor = false;
        String s = getString(VjPrintDef.DESTIONATION);
        if (s != null && (s.equals(VjPrintDef.FILE)))
            bPrintToFile = true;
        if (bPrintToFile) {
            s = getString(VjPrintDef.FILE_FG);
            if (s != null)
                 fgColor = DisplayOptions.getColor(s);
            s = getString(VjPrintDef.FILE_BG);
            if (s != null) {
                 bgColor = DisplayOptions.getColor(s);
                 bgColor = new Color(bgColor.getRed(), bgColor.getGreen(),
                               bgColor.getBlue(), 0);
            }
                 
            s = getString(VjPrintDef.FILE_CURSOR_LINE);
            if (s != null && s.equals(VjPrintDef.YES)) {
                 if (old_bDispCursor)
                     bDispCursor = true;
            }
            fv = getFloat(VjPrintDef.FILE_LINEWIDTH);
            if (fv != null)
                 lineLw = fv.intValue();
            fv = getFloat(VjPrintDef.FILE_SPWIDTH);
            if (fv != null)
                 spLw = fv.intValue();
        }
        else {
            s = getString(VjPrintDef.PRINT_FG);
            if (s != null)
                 fgColor = DisplayOptions.getColor(s);
            s = getString(VjPrintDef.PRINT_BG);
            if (s != null) {
                 bgColor = DisplayOptions.getColor(s);
                 bgColor = new Color(bgColor.getRed(), bgColor.getGreen(),
                               bgColor.getBlue(), 0);
            }
            s = getString(VjPrintDef.PRINT_CURSOR_LINE);
            if (s != null && s.equals(VjPrintDef.YES)) {
                 if (old_bDispCursor)
                     bDispCursor = true;
            }
            fv = getFloat(VjPrintDef.PRINT_LINEWIDTH);
            if (fv != null)
                 lineLw = fv.intValue();
            fv = getFloat(VjPrintDef.PRINT_SPWIDTH);
            if (fv != null)
                 spLw = fv.intValue();
        }
        if (spLw < 1)
            spLw = 1;
        if (lineLw < 1)
            lineLw = 1;
    }


    public void setPrintEnv(int x, int y, int w, int h, int lw, Graphics2D g,
            boolean bColor, boolean trans, boolean prtSrv, BufferedImage img)
    {
        printW = w;
        printH = h;
        printTrX = 0;
        printTrY = 0;
        bPrintService = prtSrv;
        bColorPaint = bColor;

        if (!bPrtCanvas) {
            createPrintCanvas();
            if (prtCanvas == null)
                return;
            if (x != 0 || y != 0)
                g.translate(x, y);
            printImageObjects(g, bColor, w, h);

            String mess;
            StringBuffer sb;
            FontMetrics fm = g.getFontMetrics();
            int fa = fm.getAscent();
            int fd = fm.getDescent();
            int fw = fm.stringWidth("ppmm9") / 5;
            int fh = fa + fd;
            // the begin of print info
            mess = new StringBuffer().append(JFUNC).append(JSYNC).append(
                ", 1)\n").toString();
            vnmrIf.sendToVnmr(mess);
            sendFontSize(fw, fh, fa, fd);
            mess = new StringBuffer().append(JFUNC).append(VSIZE).append(",")
                .append(w).append(",").append(h).append(",0,0)").toString();
            vnmrIf.sendToVnmr(mess);
            if (frameId > 0) {
                sb = new StringBuffer().append(JFUNC).append(PSIZE).append(
                    ",0,0,0,").append(w).append(",").append(h).append(")");
                vnmrIf.sendToVnmr(sb.toString());

                XMap map;
                for (int k = 1; k <= frameNum; k++) {
                    map = frameVector.elementAt(k);
                    if (map != null && map.bEnable) {
                        map.prX = (int) (map.ratioX * (float) w);
                        map.prY = (int) (map.ratioY * (float) h);
                        map.prW = (int) (map.ratioW * (float) w);
                        map.prH = (int) (map.ratioH * (float) h);
                        if (!map.bText) {
                           sb = new StringBuffer().append(JFUNC).append(PSIZE)
                             .append(",").append(map.id).append(",").append(
                              map.prX).append(",").append(map.prY).append(",");
                           if (map.bVertical)
                              sb.append(map.prH).append(",").append(map.prW)
                                    .append(")");
                           else
                              sb.append(map.prW).append(",").append(map.prH)
                                    .append(")");
                           vnmrIf.sendToVnmr(sb.toString());
                        } else {
                            map.printText(g);
                        }
                    }
                }
            }
            // the end of print info
            mess = new StringBuffer().append(JFUNC).append(JSYNC).append(
                ", 5)\n").toString();
            vnmrIf.sendToVnmr(mess);

            bColorPaint = true;
            copyParameters(prtCanvas);
            prtCanvas.copyColors(redByte, grnByte, bluByte);
            prtCanvas.copyXCursors(dconi_xCursor, dconi_xLen, dconi_xColor);
            prtCanvas.copyYCursors(dconi_yCursor, dconi_yLen, dconi_yColor);
            prtCanvas.setPrintEnv(x, y, w, h, lw,g,bColor,trans,prtSrv,img);
            if (aipCmpId > 0)
                createMonoMap(aipCmIndex);
            return;
        }

        if (!bPrintMode)
            setPrintMode(true);
        orgPrintgc = g;
        printImgBuffer = img;
        spLw = 1;
        lineLw = 1;
        maxW = x + w + 100;
        maxH = y + h + 100;
        org_font = g.getFont();
        myFont = org_font;
        icolorId = -3;
        if (bPrintService) // from vnmr.print.VjPrintService
            configPrintScreen();
        else
            bDispCursor = false;
        if (!bColor)
            bgColor = new Color(255,255,255,0);
        if (printImgBuffer != null) {
            // the printImgBuffer is the actual area for painting
            printgc = printImgBuffer.createGraphics();
            printgc.setFont(org_font);
            printgc.setBackground(bgColor);
        }
        else
            printgc = g;

        if (!bColor)
            setGrayMap(Color.white, Color.black);
        else
            setImageGrayMap();
        setBackground(bgColor);

        initFont();

        if (annPan != null) {
            annPan.setPrintColor(bColor);
            annPan.setPrintMode(true);
        }

        for (int i = 0; i < xnum; i++)
            xindex[i] = 0;
        if (printMap != null)
            printMap.clear();
        baseSp = 0;
        if (spLw > 1) {
            if (spLw > 60)
                spLw = 60;
            baseSp = spLw - 1;
            specStroke = new BasicStroke((float) spLw,
                        BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND);
        }
        else
            specStroke = strokeList[0];
        baseLw = 0;
        if (lineLw > 1) {
            if (lineLw > 60)
                lineLw = 60;
            baseLw = lineLw - 1;
            lineStroke = new BasicStroke((float) lineLw, BasicStroke.CAP_SQUARE,
                        BasicStroke.JOIN_ROUND);
        }
        else
            lineStroke = strokeList[0];
        printgc.setStroke(lineStroke);
        printgc.setColor(fgColor);
        mygc = printgc;
        bgc = null;
        wingc = printgc;
        anngc = printgc;
        fontMetric = g.getFontMetrics();
        fontAscent = fontMetric.getAscent();
        fontDescent = fontMetric.getDescent();
        fontWidth = fontMetric.stringWidth("ppmm9") / 5;
        fontHeight = fontAscent + fontDescent;
        winW = w;
        winH = h;
        annW = 0;
        annH = 0;
        annW1 = 0;
        annH1 = 0;
        aipCmpId = 0;
        if (defaultMap == null)
            defaultMap = new XMap(0, MINWH, MINWH);
        defaultMap.width = winW;
        defaultMap.height = winH;
        defaultMap.gc = printgc;
        defaultMap.fontH = fontHeight;

        clearSeries(false);
        clear_dconi_cursor(true);
    }

    public String getCanvasSize() {
        // if (!isShowing())
        //     return null;
        if (jFrame == null) {
            jFrame = (JFrame) VNMRFrame.getVNMRFrame();
            if (jFrame == null)
                getFrame();
        }
        calGeom(false);
        String str = new StringBuffer().append("").append(winSize.width)
                .append("x").append(winSize.height).append("+").append(winX)
                .append("+").append(winY).toString();
        return str;
    }

    public String getCanvasRegion() {
        // if (!isShowing())
        //     return null;
        String str = new StringBuffer().append("").append(regionX).append("x")
                .append(regionY).append("+").append(regionX2).append("+")
                .append(regionY2).toString();
        return str;
    }

    public void underConstruct(boolean s) {
        isResizing = s;
    }

    public void freeObj() {
        redByte = null;
        grnByte = null;
        bluByte = null;
        alphaByte = null;
        graphTool = null;
        bimg = null;
        dropTarget = null;
        cmIndex = null;
        aipCmIndex = null;
        if (bimgc != null) {
            bimgc.dispose();
            bimgc = null;
        }
        if (wingc != null) {
            wingc.dispose();
            wingc = null;
        }
        XMap map;
        int i, k;
        try {
          if (xmapVector != null) {
            k = xmapVector.size();
            for (i = 0; i < k; i++) {
                map = xmapVector.elementAt(i);
                if (map != null)
                    map.delete();
            }
            xmapVector.clear();
            xmapVector = null;
         }
         if (frameVector != null) {
            k = frameVector.size();
            for (i = 0; i < k; i++) {
                map = frameVector.elementAt(i);
                if (map != null)
                    map.delete();
            }
            frameVector.clear();
            frameVector = null;
         }
        }
        catch (Exception err) { }
    }

    public void showImgParams(String file, String title) {
        if (Util.iswindows())
            file = UtilB.unixPathToWindows(file);

        String path = FileUtil.openPath(file);
        if (path == null)
            return;

        JPopupMenu parmpop;

        if (title.length() > 0)
            parmpop = new ParamPopup(path, title);
        else
            parmpop = new ParamPopup(path);
        parmpop.show(this, mousePt.x, mousePt.y);
        parmpop.repaint();
    }

    private void setObjNum() {
        int i, k;
        CanvasObjIF b;

        objNum = 0;
        if (tbVector != null) {
            k = tbVector.size();
            for (i = 0; i < k; i++) {
                b = tbVector.elementAt(i);
                if (b != null && b.isVisible()) {
                    objNum++;
                    return;
                }
            }
        }
        if (imgVector != null) {
            k = imgVector.size();
            for (i = 0; i < k; i++) {
                b = imgVector.elementAt(i);
                if (b != null && b.isVisible()) {
                    objNum++;
                    return;
                }
            }
        }
    }

    private void rebuildTextBoxList() {
        if (tbVector == null)
            return;
        int id = 1;
        Vector<CanvasObjIF> newVector = new Vector<CanvasObjIF>();
        for (int i = 0; i < tbVector.size(); i++) {
            VTextBox box = (VTextBox) tbVector.elementAt(i);
            if (box != null) {
                if (box.isAvailable()) {  // not removed
                    box.setId(id);
                    newVector.addElement(box);
                    id++;
                }
            }
        }

        tbVector.clear();
        tbVector = newVector;
        if (activeObj != null && !activeObj.isVisible()) {
            if (activeObj instanceof VTextBox)
                TextboxEditor.setEditObj(null);
            set_active_obj(null);
        }
    }

    private VTextBox getTextBox(int n) {
        if (tbVector == null)
            return null;
        VTextBox box = null;
        for (int i = 0; i < tbVector.size(); i++) {
            box = (VTextBox) tbVector.elementAt(i);
            if (box != null) {
                if (box.getId() == n)
                    break;
            }
            box = null;
        }
        return box;
    }

    private VTextBox getTextFrame(int n) {
        if (tbVector == null)
            return null;
        VTextBox box = null;
        for (int i = 0; i < tbVector.size(); i++) {
            box = (VTextBox) tbVector.elementAt(i);
            if (box != null) {
                if (box.getFrameId() == n)
                    break;
            }
            box = null;
        }
        return box;
    }

    private void setTextBoxVisible(boolean bVisible, boolean bAll) {
        if (tbVector == null)
            return;
        for (int i = 0; i < tbVector.size(); i++) {
            VTextBox box = (VTextBox) tbVector.elementAt(i);
            if (box != null) {
                if (bAll)
                    box.setVisible(bVisible);
                else if (box.isSelected())
                    box.setVisible(bVisible);
            }
        }
        if (!bVisible) {
            if (activeObj != null && !activeObj.isVisible()) {
                if (activeObj instanceof VTextBox)
                    TextboxEditor.setEditObj(null);
                set_active_obj(null);
            }
        }
        setObjNum();
        repaint_canvas();
    }

    private void deleteTextFrame(VTextBox box) {
        if (tbVector == null || box == null)
            return;
        box.clear();
        rebuildTextBoxList();
        setObjNum();
        repaint_canvas();
    }
       
    private void deleteTextBox(boolean bAll, boolean includeTextFrame) {
        if (tbVector == null)
            return;
        VTextBox box;
        boolean  bChanged = false;
        int i;
        for (i = 0; i < tbVector.size(); i++) {
            box = (VTextBox) tbVector.elementAt(i);
            if (box != null) {
                if (bAll) {
                    if (!includeTextFrame) {
                       if (box.isTextFrameType())
                           box = null;
                    }
                }
                else {
                    if (!box.isSelected())
                       box = null;
                }
                if (box != null) {
                    box.clear(); 
                    bChanged = true;
                }
            }
        }
        if (!bChanged)
            return;
        rebuildTextBoxList();
        setObjNum();
        repaint_canvas();
    }

    public void addTextBox(VTextBox box) {
        if (tbVector == null)
            tbVector = new Vector<CanvasObjIF>();
        int i = 0;
        int n = 1;
        boolean bNew = true;
        for (i = 0; i < tbVector.size(); i++) {
            VTextBox b = (VTextBox) tbVector.elementAt(i);
            if (b != null) {
                b.setSelected(false);
                if (b.getId() >= n)
                    n = b.getId() + 1;
                if (b.equals(box))
                   bNew = false;
            }
        }
        if (bNew) {
           tbVector.addElement(box);
           box.id = n;
        }
        box.vnmrIf = vnmrIf;
        set_active_obj((CanvasObjIF) box);
        objNum = 1;
        box.setRatio(winW, winH, false);
        box.update();
        get_screen_loc();
    }

    private void processTextBoxCommand(StringTokenizer tok) {
        String cmd = tok.nextToken().trim();
        if (cmd.equalsIgnoreCase("edit")) {
            if (tok.hasMoreTokens())
                cmd = tok.nextToken();
            else
                cmd = "";
            TextboxEditor.execCmd(cmd);
            if (activeObj != null && (activeObj instanceof VTextBox))
                TextboxEditor.setEditObj((VTextBox) activeObj);
            return;
        }
        if (cmd.equalsIgnoreCase("delete")) {
            deleteTextBox(false, true);
            return;
        }
        if (cmd.equalsIgnoreCase("deleteall")) {
            deleteTextBox(true, true);
            return;
        }
        if (cmd.equalsIgnoreCase("hideall")) {
            setTextBoxVisible(false, true);
            return;
        }
        if (cmd.equalsIgnoreCase("showall")) {
            setTextBoxVisible(true, true);
            return;
        }
        if (cmd.equalsIgnoreCase("opentemplate")) {
            // opentemplate, sourcefile
            if (!tok.hasMoreTokens())
                return;
            if (tbVector == null)
                tbVector = new Vector<CanvasObjIF>();
            deleteTextBox(true, false);
            TextboxEditor.loadTemplate(vnmrIf, tok.nextToken(), tbVector);
            adjustObjects(false);
            return;
        }
        if (cmd.equalsIgnoreCase("savetemplate")) {
            // savetemplate, sourcefile
            if (!tok.hasMoreTokens())
                return;
            if (tbVector == null)
                return;
            TextboxEditor.saveTemplate(tok.nextToken(), tbVector);
            return;
        }
        if (cmd.equalsIgnoreCase("display")) {
            // display, id, datafile, sourcefile
            if (!tok.hasMoreTokens())
                return;
            int id = 1;

            try {
                id = Integer.parseInt(tok.nextToken());
            } catch (NumberFormatException er) {
                return;
            }
            if (!tok.hasMoreTokens())
                return;
            String d1 = tok.nextToken();
            String d2 = null;
            if (tok.hasMoreTokens())
                d2 = tok.nextToken();
            VTextBox box = getTextBox(id);
            if (box == null)
                return;
            box.setValue(d1, d2);
            if (box.isSelected())
                set_active_obj((CanvasObjIF) box);
            repaint_canvas();
            return;
        }
        if (cmd.equalsIgnoreCase("deletetemplate")) {
            // deletetemplate, sourcefile
            if (!tok.hasMoreTokens())
                return;
            TextboxEditor.deleteTemplate(tok.nextToken());
            return;
        }
    }

    private void processTextFrameCommand(StringTokenizer tok) {
        int id = 1;
        int k;
        float[] rv;
        VTextBox box;
        String cmd = tok.nextToken().trim();
        if (cmd.equalsIgnoreCase("display")) {
           // display id x y w h file color font ...
            if (!tok.hasMoreTokens())
                return;
            try {
                id = Integer.parseInt(tok.nextToken());
            } catch (NumberFormatException er) {
                return;
            }
            rv = new float[4];
            try {
                for (k = 0; k < 4; k++) {
                    if (!tok.hasMoreTokens())
                        return;
                    rv[k] = Float.parseFloat(tok.nextToken());
                }
            } catch (NumberFormatException er) {
                return;
            }
            if (!tok.hasMoreTokens())
                return;
           
            box = getTextFrame(id);
            if (box == null)
                box = new VTextBox();
            box.setBoundsRatio((float)winW, (float)winH, rv[0], rv[1], rv[2], rv[3]);
            box.setFrameId(id);
            box.setTextFrameType(true);
            String file = tok.nextToken();
            if (file.startsWith("'")) {
               file = file.substring(1, file.length() - 1);
            }
            if (tok.hasMoreTokens())
                box.setColorName(tok.nextToken());
            if (tok.hasMoreTokens())
                box.setFontName(tok.nextToken());
            if (tok.hasMoreTokens())
                box.setFontStyle(tok.nextToken());
            if (tok.hasMoreTokens())
                box.setFontSize(tok.nextToken());
            box.setupFont(); 
            box.setValue(file, null, false);
            box.setSrcFileName("tmp"+id);
            addTextBox(box);
            repaint_canvas();
            return;
        }
        if (cmd.equalsIgnoreCase("hideall")) {
            setTextBoxVisible(false, true);
            return;
        }
        if (cmd.equalsIgnoreCase("showall")) {
            setTextBoxVisible(true, true);
            return;
        }
        if (cmd.equalsIgnoreCase("deleteall")) {
            deleteTextBox(true, true);
            return;
        }
        if (!tok.hasMoreTokens())
            return;
        try {
            id = Integer.parseInt(tok.nextToken());
        } catch (NumberFormatException er) {
            return;
        }
        box = getTextFrame(id);
        if (box == null)
            return;
        if (cmd.equalsIgnoreCase("delete")) {
            deleteTextFrame(box);
            return;
        }
        if (cmd.equalsIgnoreCase("hide")) {
            box.setVisible(false);
            repaint_canvas();
            return;
        }
        if (cmd.equalsIgnoreCase("show")) {
            box.setVisible(true);
            repaint_canvas();
            return;
        }
    }

    private void processTableCmd(StringTokenizer tok) {
        boolean bOpen = true;
        boolean bAll = false;
        String fileName = null;
        String data, value;
        int  id = -1;
        int  x = 0;
        int  y = 0;
        int  w = 0;
        int  h = 0;
        int  color = 0;
        VJTable table;

        try {
            while (true) {
                if (!tok.hasMoreTokens())
                   break;
                data = tok.nextToken().trim();
                int n = data.indexOf(":");
                if (n > 0) {
                   value = data.substring(n+1).trim();
                   data = data.substring(0, n);
                   if (value.length() < 1)
                       continue;
                   if (data.equals("file"))
                       fileName = value;
                   else {
                       n = Integer.parseInt(value);
                       if (data.equals("id"))
                           id = n;
                       else if (data.equals("x"))
                           x = n;
                       else if (data.equals("y"))
                           y = n;
                       else if (data.equals("w") || data.equals("width"))
                           w = n;
                       else if (data.equals("h") || data.equals("height"))
                           h = n;
                       else if (data.equals("color") || data.equals("c"))
                           color = n;
                   }
                }
                else {
                    if (data.equals("open"))
                        bOpen = true;
                    if (data.equals("close") || data.equals("delete"))
                        bOpen = false;
                    else if (data.equalsIgnoreCase("closeall") ||
                             data.equalsIgnoreCase("deleteall")) {
                        bOpen = false;
                        bAll = true;
                    }
                }
            }
        }
        catch (NumberFormatException er) {
        }

        if (id < 0)
            return;
        if (!bOpen) {
            remove_table_series(id, bAll);
            return;
        }
        if (fileName == null)
            return;
        table = create_table_series(id, x, y, w, h);
        if (color > 0)
            table.setColor(DisplayOptions.getColor(color));
        table.load(fileName);
    }

    public void processCommand(String str) {
        StringTokenizer tok = new StringTokenizer(str);
        String cmd = tok.nextToken().trim();
        if (cmd.equals("showImgParams") && tok.countTokens() > 0) {
            String path = tok.nextToken().trim();
            String title = "";
            while (tok.hasMoreTokens())
                title += tok.nextToken().trim() + " ";
            showImgParams(path, title);
            return;
        }
        if (cmd.equals("rubberband")) {
            setRubberMode(true);
            return;
        }
        if (cmd.equals("endrubberband")) {
            setRubberMode(false);
            return;
        }
        if (cmd.equals("rubberarea")) {
            bRubberBox = false;
            setRubberArea(true);
            return;
        }
        if (cmd.equals("endrubberarea")) {
            bRubberBox = false;
            setRubberArea(false);
            return;
        }
        if (cmd.equals("endoverlay")) {
            if (bStartOverlay) {
                bStartOverlay = false;
                repaint_canvas();
            }
            return;
        }
        if (cmd.equals("rubberbox")) {
            bRubberBox = true;
            setRubberArea(true);
            return;
        }
        if (cmd.equals("endrubberbox")) {
            bRubberBox = true;
            setRubberArea(false);
            return;
        }
        if (cmd.equals("select") && Util.iswindows()) {
            setCopyMode(true);
            return;
        }
        if (cmd.equals("selectall") && Util.iswindows()) {
            setCopyMode(true);
            Point point = new Point(regionX, regionY);
            Point point2 = new Point(regionX2, regionY2);
            drawBox(point, point2);
            return;
        }
        if (cmd.equals("copy") && m_bSelect && Util.iswindows()) {
            copyArea();
            setCopyMode(false);
            return;
        }
        if (cmd.equals("frame")) {
            frameCommand(str);
            return;
        }
        if (cmd.equals("scale")) {
            int id = 1;
            int s = 1;

            try {
                if (tok.hasMoreTokens())
                    id = Integer.parseInt(tok.nextToken());
                if (tok.hasMoreTokens())
                    s = Integer.parseInt(tok.nextToken());
            } catch (NumberFormatException er) {
                return;
            }
            XMap map = getAipMap(id + 4);
            if (map == null) {
                return;
            }
            if (bimgc != null) {
                // bimgc.setRenderingHint(RenderingHints.KEY_INTERPOLATION,
                //         RenderingHints.VALUE_INTERPOLATION_BICUBIC);
                bimgc.drawImage(map.img, 0, 0, map.width * s, map.height * s,
                        0, 0, map.width, map.height, null);
                repaint();
            }
            return;
        }
        if (cmd.equals("cursor") && tok.countTokens() > 0) {
            String cr = (String) tok.nextToken().trim();
            objCursor = VCursor.getCursor(cr);
            setCursor(objCursor);
            cursorIndex = -1;
            return;
        }
        if (cmd.equals("frameBox") && tok.countTokens() > 0) {
            boolean oldVal = bShowFrameBox;
            if (((String) tok.nextToken().trim()).equals("0"))
                bShowFrameBox = false;
            else
                bShowFrameBox = true;
            if (oldVal != bShowFrameBox)
                repaint_canvas();
            return;
        }
        if (cmd.equals("textBox") && tok.countTokens() > 0) {
            processTextBoxCommand(tok);
            return;
        }
        if (cmd.equals("textfile") && tok.countTokens() > 0) {
            String path = null;
            int x = 0;
            int y = 0;
            String c = "yellow";
            int f = 14;
            String s = "plain";

            try {
                if (tok.hasMoreTokens())
                    path = tok.nextToken().trim();
                if (tok.hasMoreTokens())
                    x = Integer.parseInt(tok.nextToken());
                if (tok.hasMoreTokens())
                    y = Integer.parseInt(tok.nextToken());
                if (tok.hasMoreTokens())
                    c = tok.nextToken().trim();
                if (tok.hasMoreTokens())
                    f = Integer.parseInt(tok.nextToken());
                if (tok.hasMoreTokens())
                    s = tok.nextToken().trim();
            } catch (NumberFormatException er) { }

            if (path == null)
                return;
            if (x < 0)
                x = 0;
            if (y < 0)
                y = 0;
            XMap map = frameVector.elementAt(frameId);
            drawTextFile(map, path, x, y, c, f, s);
            return;
        }
        if (cmd.equals("table")) {
            processTableCmd(tok);
            return;
        }
        if (cmd.equals("dataInfo") && tok.countTokens() > 1) {

            int id = 0;
            try {
                id = Integer.parseInt(tok.nextToken());
            } catch (NumberFormatException er) {
                return;
            }

            if (id != 1)
                return;
            XMap map = frameVector.elementAt(id);
            if (map == null || !map.bOpen) {
                return;
            }

            String dcmd = tok.nextToken().trim();
            boolean bInit = false;
            boolean bUpdate = false;
            if (dcmd.equals("dconi") && tok.countTokens() > 9) {
                if (!dcmd.equals(map.dcmd))
                    bInit = true;
                map.dcmd = dcmd;
                String tmpstr = tok.nextToken().trim();
                if (!tmpstr.equals(map.axis2)) {
                    map.axis2 = tmpstr;
                    bInit = true;
                }
                tmpstr = tok.nextToken().trim();
                if (!tmpstr.equals(map.axis1)) {
                    map.axis1 = tmpstr;
                    bInit = true;
                }
                double d2;
                try {
                    d2 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d2 = 0.0;
                }
                double d1;
                try {
                    d1 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d1 = 0.0;
                }
                if (d2 != map.spx || d1 != map.spy)
                    bInit = true;
                map.spx = d2;
                map.spy = d1;

                try {
                    d2 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d2 = 0.0;
                }
                try {
                    d1 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d1 = 0.0;
                }
                if ((map.spx + d2) != map.epx || (map.spy + d1) != map.epy)
                    bInit = true;
                map.epx = map.spx + d2;
                map.epy = map.spy + d1;

                try {
                    d2 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d2 = 0.0;
                }
                try {
                    d1 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d1 = 0.0;
                }
                if (d2 != map.sp2 || d1 != map.sp1)
                    bUpdate = true;
                map.sp2 = d2;
                map.sp1 = d1;

                try {
                    d2 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d2 = 0.0;
                }
                try {
                    d1 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d1 = 0.0;
                }
                if ((map.sp2 + d2) != map.ep2 || (map.sp1 + d1) != map.ep1)
                    bUpdate = true;
                map.ep2 = map.sp2 + d2;
                map.ep1 = map.sp1 + d1;

                int t;
                try {
                    t = Integer.parseInt(tok.nextToken());
                } catch (NumberFormatException er) {
                    t = 1;
                }
                if (map.trace != t) {
                    map.trace = t;
                    if (bActive && m_overlayMode == OVERLAY_ALIGN) {
                        Util.getViewArea().updateTrace(map.trace, map.axis2,
                                map.axis1);
                    }
                    bInit = true;
                }

                if (bInit)
                    Util.getViewArea().processOverlayCmd("init");
                if (bUpdate)
                    Util.getViewArea().processOverlayCmd("update");

            } else if (dcmd.equals("dconi") && tok.countTokens() > 1) {
                if (!dcmd.equals(map.dcmd))
                    bInit = true;
                map.dcmd = dcmd;
                String name = tok.nextToken().trim();
                double d;
                try {
                    d = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d = 0.0;
                }
                if (name.equals("sp") && d != map.sp2) {
                    map.sp2 = d;
                    bUpdate = true;
                } else if (name.equals("sp1") && d != map.sp1) {
                    map.sp1 = d;
                    bUpdate = true;
                } else if (name.equals("wp") && (map.sp2 + d) != map.ep2) {
                    map.ep2 = map.sp2 + d;
                    bUpdate = true;
                } else if (name.equals("wp1") && (map.sp1 + d) != map.ep1) {
                    map.ep1 = map.sp1 + d;
                    bUpdate = true;
                }

                if (bInit)
                    Util.getViewArea().processOverlayCmd("init");
                if (bUpdate)
                    Util.getViewArea().processOverlayCmd("update");

            } else if (dcmd.equals("ds") && tok.countTokens() > 4) {
                if (!dcmd.equals(map.dcmd))
                    bInit = true;
                map.dcmd = dcmd;
                String tmpstr = tok.nextToken().trim();
                if (!tmpstr.equals(map.axis2))
                    bInit = true;
                map.axis2 = tmpstr;
                double d2;
                try {
                    d2 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d2 = 0.0;
                }
                if (d2 != map.spx)
                    bInit = true;
                map.spx = d2;

                try {
                    d2 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d2 = 0.0;
                }
                if ((map.spx + d2) != map.epx)
                    bInit = true;
                map.epx = map.spx + d2;

                try {
                    d2 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d2 = 0.0;
                }
                if (d2 != map.sp2)
                    bUpdate = true;
                map.sp2 = d2;

                try {
                    d2 = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d2 = 0.0;
                }
                if ((map.sp2 + d2) != map.ep2)
                    bUpdate = true;
                map.ep2 = map.sp2 + d2;

                if (bInit)
                    Util.getViewArea().processOverlayCmd("init");
                if (bUpdate)
                    Util.getViewArea().processOverlayCmd("update");

            } else if (dcmd.equals("ds") && tok.countTokens() > 1) {
                if (!dcmd.equals(map.dcmd))
                    bInit = true;
                map.dcmd = dcmd;
                String name = tok.nextToken().trim();
                double d;
                try {
                    d = Double.parseDouble(tok.nextToken());
                } catch (NumberFormatException er) {
                    d = 0.0;
                }
                if (name.equals("sp") && d != map.sp2) {
                    map.sp2 = d;
                    bUpdate = true;
                } else if (name.equals("wp") && (map.sp2 + d) != map.ep2) {
                    map.ep2 = map.sp2 + d;
                    bUpdate = true;
                }

                if (bInit)
                    Util.getViewArea().processOverlayCmd("init");
                if (bUpdate)
                    Util.getViewArea().processOverlayCmd("update");
	    } else {
                map.dcmd = dcmd;
            }
            return;
        }
        if (cmd.equals("textFrame") && tok.countTokens() > 0) {
            processTextFrameCommand(tok);
            return;
        }
    }

    public XMap getMap(int id) {
        return frameVector.elementAt(id);
    }

    public void selectFrameWithG(int num, int x, int y, int w, int h) {
        XMap map = frameVector.elementAt(num);
        if (map == null)
            return;
        if (w == 0 || h == 0)
            setDispOrient(map, "");
        if (num == frameId && map.x == x && map.y == y && map.width == w
                && map.height == h)
            return;
        openFrame(num, x, y, w, h);
    }

    public void selectFrame(int num) {
        if (num == frameId)
            return;
        if (frameNum < 1)
            return;
        activateFrame(num);
    }

    public void activateFrame(int num) {
        int oldId = frameId;

        if (!set_frame(num))
            return;
        if (frameMap == null)
            return;
        set_active_obj(null);
        frameMap.bEnable = true;
        frameMap.bOrgEnable = true;
        frameMap.xbActive = true;
        if (frameId != oldId) {
            repaint_canvas();
        }
        sendFrameId();
    }

    public void openTextFrame(int id, String path, int x, int y, int w, int h,
            String c, int f, String s) {
        if (bNative)
            return;
        if (id <= 0) {
            // set_frame(0);
            return;
        }
        if (frameVector.size() <= id)
            frameVector.setSize(id + 4);
        if (winW > 50 && winH > 50) {
            if (w < 5 || w > winW)
                w = winW / 2;
            if (h < 5 || h > winH)
                h = winH / 2;
            if (x + w > winW)
                x = winW - w;
            if (y + h > winH)
                h = winH - h;
        } else {
            if (w > 500)
                w = 500;
            if (h > 200)
                h = 200;
        }
        XMap map = frameVector.elementAt(id);
        if (map == null) {
            map = new XMap(id, x, y, w, h);
            frameVector.setElementAt(map, id);
        } else {
            map.x = x;
            map.y = y;
            map.setSize(w, h);
            map.bNewSize = true;
        }
        map.bOpen = true;
        map.bOrgOpen = true;
        map.bText = true;
        map.bEnable = true;
        map.bOrgEnable = true;
        map.xbActive = true;
        bFrameAct = true;
        if (id > frameNum)
            frameNum = id;
        if (path.length() > 0) {
            drawTextFile(map, path, x, y, c, f, s);
            repaint_canvas();
        }
    }

    public void openFrame(int id, int x, int y, int w, int h) {
        if (bNative)
            return;
        XMap map = createFrameMap(id, x, y, w, h);
        if (map == null)
            return;
        sendFrameSize(map);
        sendFrameStatus();
        activateFrame(id);
    }

    public void enableFrame(boolean b, int num) {
        if (frameVector.size() <= num)
            return;
        XMap map = frameVector.elementAt(num);
        if (map != null && map.bOpen) {
            map.bEnable = b;
            map.bOrgEnable = b;
        }
    }

    public void enableAllFrames(boolean b) {
        int s = frameVector.size();
        for (int k = 0; k < s; k++) {
            XMap map = frameVector.elementAt(k);
            if (map != null && map.bOpen) {
                map.bEnable = b;
                map.bOrgEnable = b;
            }
        }
    }

    private void openDefaultFrame() {
        int count = 0;
        int last = 0;
        int newId = 0;
        int s = frameVector.size();
        for (int k = 1; k < s; k++) {
            XMap map = frameVector.elementAt(k);
            if (map != null) {
                if (map.bOpen) {
                    count++;
                    last = k;
                    if (!map.bText)
                        newId = k;
                }
            }
        }
        frameId = 0;
        if (count == 0) {
            frameNum = 0;
            openFrame(1, 0, 0, winW, winH);
            return;
        }
        frameNum = last;
        if (newId == 0)
            newId = last;
        activateFrame(newId);
    }

    public void closeFrame(int num) {
        if (frameVector.size() <= num)
            return;
        if (num <= 0) {
            return;
        }
        XMap map = frameVector.elementAt(num);
        if (map == null)
            return;
        map.bOpen = false;
        map.bOrgOpen = false;
        map.bEnable = false;
        map.bOrgEnable = false;
        map.xbActive = false;
        map.bText = false;
        map.clear();
        if (frameId == num)
            openDefaultFrame();
        sendFrameStatus();
        repaint_canvas();
    }

    public void closeAllFrames() {
        int s = frameVector.size();
        for (int k = 0; k < s; k++) {
            XMap map = frameVector.elementAt(k);
            if (map != null && map.id != 1) {
                if (map.bOpen)
                    map.clear();
                map.bOpen = false;
                map.bOrgOpen = false;
                map.xbActive = false;
                map.bEnable = false;
                map.bOrgEnable = false;
                map.bText = false;
            }
        }
        frameNum = 0;
        frameId = 0;
        openFrame(1, 0, 0, winW, winH);
        repaint_canvas();
    }

    public void closeAllTextFrames() {
        int s = frameVector.size();
        boolean bOpenFrame = false;
        int count = 0;
        int lastId = 0;
        for (int k = 0; k < s; k++) {
            XMap map = frameVector.elementAt(k);
            if (map != null) {
                if (map.bText) {
                    map.bOpen = false;
                    map.bOrgOpen = false;
                    map.bEnable = false;
                    map.bOrgEnable = false;
                    map.xbActive = false;
                    count++;
                    if (frameId == k)
                        bOpenFrame = true;
                } else
                    lastId = k;
            }
        }
        if (bOpenFrame)
            openDefaultFrame();
        else
            frameNum = lastId;
        if (count > 0)
            repaint_canvas();
    }

    public void openAllTextFrames() {
        int s = frameVector.size();
        int count = 0;
        int lastId = 0;
        for (int k = 0; k < s; k++) {
            XMap map = frameVector.elementAt(k);
            if (map != null && map.bText) {
                map.bOpen = true;
                map.bOrgOpen = true;
                map.bEnable = true;
                map.bOrgEnable = true;
                map.xbActive = false;
                count++;
                lastId = k;
            }
        }
        if (frameNum < lastId)
            frameNum = lastId;
        if (count > 0)
            repaint_canvas();
    }

    public void deleteFrame(int num) {
        if (num <= 0)
            return;
        if (frameVector.size() <= num)
            return;
        XMap map = frameVector.elementAt(num);
        if (map == null)
            return;
        map.delete();
        sendFrameStatus();
        frameVector.setElementAt(null, num);
        if (frameId == num)
            openDefaultFrame();
        repaint_canvas();
    }

    public void deleteAllTextFrames() {
        int s = frameVector.size();
        boolean bOpenFrame = false;
        for (int k = 0; k < s; k++) {
            XMap map = frameVector.elementAt(k);
            if (map != null && map.bText) {
                map.delete();
                frameVector.setElementAt(null, k);
                if (frameId == k)
                    bOpenFrame = true;
            }
        }
        if (bOpenFrame)
            openDefaultFrame();
        repaint_canvas();
    }

    public void frameCommand(String str) {
        StringTokenizer tok = new StringTokenizer(str);
        String cmd = tok.nextToken().trim();
        int id = 0;
        if (tok.hasMoreTokens()) {
            try {
                id = Integer.parseInt(tok.nextToken());
            } catch (NumberFormatException er) {
                return;
            }
        }
        if (cmd.equals("close")) {
            closeFrame(id);
            return;
        }
        if (cmd.equals("closeall")) {
            closeAllFrames();
            return;
        }
        if (cmd.equals("closealltext")) {
            closeAllTextFrames();
            return;
        }
        if (cmd.equals("active")) {
            activateFrame(id);
            return;
        }
        if (cmd.equals("open")) {
            openFrame(id, 10, 10, 80, 80);
            // sendFrameSize();
            return;
        }
        if (cmd.equalsIgnoreCase("orientation")
                || cmd.equalsIgnoreCase("orient")) {
            if (!tok.hasMoreTokens())
                return;
            if (id == 0)
                id = frameId;
            if (id <= 0)
                return;
            if (frameVector.size() <= id)
                return;
            XMap map = frameVector.elementAt(id);
            setDispOrient(map, tok.nextToken("\n").trim());
            return;
        }
    }

    public void sendMoreEvent(boolean s) {
        sendMore = s;
        if (csiCanvas != null)
           csiCanvas.sendMoreEvent(s);
    }

    public void setActive(boolean s) {
        boolean oldVal = bActive;
        bActive = s;

        if (csiCanvas != null)
            csiCanvas.setActive(s);
        if (s && activeObj != null && (activeObj instanceof VTextBox))
            TextboxEditor.setEditObj((VTextBox) activeObj);
        if (bCsiWindow)
            return;
        if (s && oldVal != s && vjIf != null)
            vjIf.canvasSizeChanged(winId);
    }

    public void resetDrag() {
        drag_ecnt = drag_dcnt = 0;
    }

    public void enableDrag() {
        drag_ecnt++;
        if (dragCmd != null) {
            vnmrIf.sendToVnmr(dragCmd);
            dragCmd = null;
        }
    }

    public void disableDrag() {
        drag_dcnt++;
    }

    public boolean dragEnabled() {
        if (syncDrag)
            return drag_ecnt >= drag_dcnt - syncval;
        return true;
    }

    public void rebuildUI(SessionShare s, AppIF ap) {
        sshare = s;
        vjIf = Util.getVjIF();
        hs = sshare.userInfo();
        appIf = ap;
    }

    public void nativeSync() {
        if (vjFrame == null)
            vjFrame = VNMRFrame.getVNMRFrame();
        if (vjFrame != null)
            vjFrame.syncX();
    }

    public void setSuspend(boolean s) {
        bSuspend = s;
    }

    public void processSyncEvent() {
        if (bSyncOvlay) {
            bSyncOvlay = false;
            if (viewIf != null)
                viewIf.canvasSyncReady(winId);
        }
        if (bStartOverlay)
            vnmrIf.sendToVnmr("vnmrjcmd('canvas', 'endoverlay')");
    }

    public void processShowEvent() {
        if (isResizing)
            return;
        if (bActive)
            nativeSync();
        if (bk1 != null || bk2 != null) {
            if (bk1 != null)
                vnmrIf.sendToVnmr(bk1);
            if (bk2 != null)
                vnmrIf.sendToVnmr(bk2);
            bk1 = null;
            bk2 = null;
        } else {
            if (bSuspend)
                return;
            bResized = false;
            vnmrIf.sendToVnmr(new StringBuffer().append("X@copy ").append(JEVENT)
                    .append(PAINT).append(") ").toString());
            bReshow = false;
        }
    }

    public void setNativeGraphics(boolean b) {
        if (bNative != b) {
            bNative = b;
            if (!b)
                javaResize();
            else
                nativeResize(true);
        }
    }

    public void createShowEvent() {
        if (viewIf == null)
            return;
        if (viewIf.isResizing())
            return;
        if (bNative) {
            bSuspend = false;
            bReshow = true;
            String str = "X@stop2\n";
            if (bResized)
                str = "X@stop3\n";
            vnmrIf.sendToVnmr(str);
        }
        ComponentEvent es = new ComponentEvent(this,
                ComponentEvent.COMPONENT_SHOWN);
        Toolkit.getDefaultToolkit().getSystemEventQueue().postEvent(es);
    }

    public void setRubberMode(boolean bCopy) {
        if (frameId > 1) {
            XMap map = frameVector.elementAt(frameId);
            if (map != null && map.bText)
                return;
        }

        m_bSelect = bCopy;
        bRubberMode = bCopy;
        // copyColor = Color.yellow;
        copyColor = hilitColor;
        if (!m_bSelect) {
            if (!bNative)
                if (bActive)
                    setCursor(objCursor);
                else
                    setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
            cursorIndex = -1;
            if (mousePt == null)
                return;
            if (!bRubberArea)
                eraseBox();
            int x = mousePt.x - frameX;
            int y = mousePt.y - frameY;
            int x2 = mpx;
            int y2 = mpy;
            if (bFrameAct) {
                if (x < 0)
                    x = 0;
                if (y < 0)
                    y = 0;
                x2 = x2 - frameX;
                if (x2 > frameW)
                    x2 = frameW;
                y2 = y2 - frameY;
                if (y2 > frameH)
                    y2 = frameH;
            }
            /*
              vnmrIf.sendToVnmr("frameAction('endrubberband',"+ frameId+
                  ","+mousePt.x+","+mousePt.y+
                  ","+mpx+","+mpy+")");
             */
            vnmrIf.sendToVnmr("frameAction('endrubberband'," + frameId + ","
                    + x + "," + y + "," + x2 + "," + y2 + ")");
        } else {
            if (!bNative)
                setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        }
    }

    public void setRubberArea(boolean s) {

        if (frameId > 1) {
            XMap map = frameVector.elementAt(frameId);
            if (map != null && map.bText)
                return;
        }

        if (s)
            bRubberArea = s;
        setRubberMode(s);
        bRubberArea = s;
        if (!s)
            repaint_canvas();
        else {
            // shadeColor = DisplayOptions.getColor("shade");
            if (shadeColor.toString().equals(bgColor.toString())) {
                copyColor = hilitColor;
            } else {
                int r = shadeColor.getRed();
                int g = shadeColor.getGreen();
                int b = shadeColor.getBlue();
                copyColor = new Color(r, g, b, 160);
            }
        }
    }

    public void setCopyMode(boolean bCopy) {
        if (bRubberMode) {
            if (bRubberArea)
                setRubberArea(bCopy);
            else
                setRubberMode(bCopy);
            return;
        }
        m_bSelect = bCopy;
        copyColor = Util.getContrastingColor(bgColor);
        if (!bCopy) {
            eraseBox();
            vnmrIf.sendToVnmr("copymode='false'");
        }
    }

    public void copyArea() {
        try {
            Clipboard clipboard = Toolkit.getDefaultToolkit()
                    .getSystemClipboard();
            CanvasTransferable canvasTransferable = new CanvasTransferable(
                    this, m_rectSelect);
            canvasTransferable.exportToClipboard(this, clipboard,
                    TransferHandler.COPY);
        } catch (Exception e) {
            //e.printStackTrace();
            Messages.postError("Error copying selected area");
            Messages.writeStackTrace(e);
        }
    }

    private void sendFrameId() {
        if (frameMap == null)
            return;
        String mess = new StringBuffer().append(JFUNC).append(FRMID).append(
                ",").append(frameMap.id).append(") ").toString();
        vnmrIf.sendToVnmr(mess);
    }

    private void sendFrameStatus() {
        XMap map;
        int n = frameVector.size();
        String mess;

        for (int k = 1; k < n; k++) {
            map = frameVector.elementAt(k);
            if (map != null) {
                if (map.bOpen) {
                    mess = new StringBuffer().append(JFUNC).append(FRMSTAUS)
                            .append(",").append(map.id).append(", 1)")
                            .toString();
                } else {
                    mess = new StringBuffer().append(JFUNC).append(FRMSTAUS)
                            .append(",").append(map.id).append(", 0)")
                            .toString();
                }
                vnmrIf.sendToVnmr(mess);
            }
        }
    }

    private void sendFrameSize(XMap map) {
        if (map == null)
            return;
        if (!map.bNewSize)
            return;
        map.bNewSize = false;
        fontWidth = map.fontW;
        fontHeight = map.fontH;
        fontAscent = map.ftAscent;
        fontDescent = map.ftDescent;
        sendFontSize(fontWidth, fontHeight, fontAscent, fontDescent);
        StringBuffer sb0 = new StringBuffer().append(JFUNC).append(FVERTICAL)
                .append(",").append(map.id).append(",");
        if (map.bVertical)
             sb0.append(1);
        else
             sb0.append(0);
        sb0.append(")");
        vnmrIf.sendToVnmr(sb0.toString());

        StringBuffer sb = new StringBuffer().append(JFUNC).append(FRMSIZE)
                .append(",").append(map.id).append(",").append(map.x).append(
                        ",").append(map.y).append(",");
        if (map.bVertical) {
            sb.append(map.height).append(",").append(map.width).append(")");
        } else {
            sb.append(map.width).append(",").append(map.height).append(")");
        }
        vnmrIf.sendToVnmr(sb.toString());
    }

    private void sendFrameSize() {
        sendFrameSize(frameMap);
    }

    private void sendFrameLoc() {
        if (frameMap == null)
            return;
        StringBuffer sb = new StringBuffer().append(JFUNC).append(FRMLOC)
                .append(",").append(frameMap.id).append(",").append(frameMap.x)
                .append(",").append(frameMap.y).append(",");
        if (frameMap.bVertical) {
            sb.append(frameMap.height).append(",").append(frameMap.width)
                    .append(")");
        } else {
            sb.append(frameMap.width).append(",").append(frameMap.height)
                    .append(")");
        }
        vnmrIf.sendToVnmr(sb.toString());
    }

    private void sendIconInfo(VIcon icon) {
        String cmd = new StringBuffer().append("imagefile('resize','").append(
                icon.iconId).append("',").append(icon.getX()).append(",")
                .append(icon.getY()).append(",").append(icon.getWidth())
                .append(",").append(icon.getHeight()).append(")").toString();
        appIf.sendToVnmr(cmd);
    }

    private void resizeFrame() {
        int x, y, w, h;
        setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
        cursorIndex = Cursor.DEFAULT_CURSOR;
        bObjMove = false;
        eraseBox();
        if (!bObjDrag) {
            x = m_rectSelect.x;
            y = m_rectSelect.y;
            w = m_rectSelect.width;
            h = m_rectSelect.height;
            if (x < 0)
                x = 0;
            if (y < 0)
                y = 0;
            if (y + h > winH)
                y = winH - h;
            if (x + w > winW)
                x = winW - w;
            if (bMoveFrame) {
                frameMap.x = x;
                frameMap.y = y;
                frameMap.setSize(w, h);
            } else if (activeObj != null) {
                activeObj.setX(x);
                activeObj.setY(y);
                activeObj.setSize(w, h);
            }
        } else {
            if (bMoveFrame)
                frameMap.setSize(moveW, moveH);
        }
        if (bMoveFrame)
            reset_frame_info();
        else if (activeObj != null)
            activeObj.setRatio(winW, winH, false);
        if (bObjDrag) {
            bObjDrag = false;
            if (bMoveFrame)
                sendFrameLoc();
            repaint_canvas();
        } else {
            bObjResize = false;
            if (bMoveFrame) {
                if (frameMap.bText)
                    repaint_canvas();
                else
                    sendFrameSize();
            } else
                repaint_canvas();
        }
        if (!bMoveFrame) {
            if (m_vIcon != null)
                sendIconInfo(m_vIcon);
            set_active_obj(activeObj);
        }
    }

    public synchronized void setBusy(boolean s) {
        if (isBusy == s)
            return;
        isBusy = s;
        if (s) {
            if (dropTarget.isActive())
                dropTarget.setActive(false);
            try {
                dropTarget.removeDropTargetListener(dropListener);
            } catch (IllegalArgumentException er) {
            }
            setDropTarget(null);
        } else {
            if (!dropTarget.isActive())
                dropTarget.setActive(true);
            try {
                dropTarget.addDropTargetListener(dropListener);
            } catch (TooManyListenersException er) {
            }
            setDropTarget(dropTarget);
        }
    }

    private void sendOverlayMode(int n) {
        StringBuffer sb = new StringBuffer().append(JFUNC).append(OVLYTYPE).append(",")
                .append(n).append(")");
        vnmrIf.sendToVnmr(sb.toString());
    }

    public void setBottomLayer(boolean b) {
        bBottomLayer = b;
    }

    public boolean isBottomLayer() {
        return bBottomLayer;
    }


    public void startOverlay(boolean b) {
        bStartOverlay = b;
    }

    public void setOverlay(boolean overlay, boolean bottom, boolean top,
            VnmrCanvas canvas) {
        // boolean oldOv = bOverlayed;
        // boolean oldTop = bTopLayer;
        bOverlayed = overlay;
        bBottomLayer = bottom;
        bTopLayer = top;
        if (!top || !overlay) {
            bStartOverlay = false;
        }
        if (canvas != null && canvas.equals(this)) {
            topCanvas = null;
            bTopLayer = true;
        } else
            topCanvas = canvas;
        bDispCursor = true;
        if (overlay && !top) {
            bDispCursor = false;
        }
        sendOverlayMode(dispType);
        if (overlay && top) {
            /*
             if (oldOv && !oldTop && m_overlayMode > 1) {
             vnmrIf.sendToVnmr("repaint");
             }
             */
            /*
             if (frameMap != null) {
             frameMap.bNewSize = true;
             sendFrameSize(); 
             }
             */
        }
    }

    public void setOverlayMode(boolean xorMode) {
        bXorOvlay = xorMode;
        if (bOverlayed && bTopLayer)
            repaint_canvas();
    }

    public void updateTrace(int id, int t, String axis2, String axis1) {
        if (m_overlayMode != OVERLAY_ALIGN)
            return;
        XMap map = frameVector.elementAt(1);
        StringBuffer sb;
        if (map != null) {
            if (m_overlayMode == OVERLAY_ALIGN && map.axis2 != null
                    && map.axis1 == null && map.axis2.equals(axis1)) {

                int activeWin = Util.getViewArea().getActiveWindow();
                map.b1D_Y = true;
                m_bRepaintOrient = false;
                m_bUpdateOrient = false;
                map.setVertical(true, false);
                m_bRepaintOrient = true;

                sb = new StringBuffer().append(JFUNC).append(
                        OVLYTYPE).append(",'").append("align").append("',")
                        .append(m_overlayMode).append(",'").append("align")
                        .append("',").append(activeWin).append(",").append(
                                activeWin).append(",").append(ALIGN_1D_Y)
                        .append(",0,0,0,0,0,0,0,0)");
                Util.getViewArea().sendToVnmr(id + 1, sb.toString());
            } else if (m_overlayMode == OVERLAY_ALIGN && map.axis2 != null
                    && map.axis1 == null && map.axis2.equals(axis2)) {

                int activeWin = Util.getViewArea().getActiveWindow();
                map.b1D_Y = false;
                m_bRepaintOrient = false;
                m_bUpdateOrient = false;
                map.setVertical(false, false);
                m_bRepaintOrient = true;

                sb = new StringBuffer().append(JFUNC).append(
                        OVLYTYPE).append(",'").append("align").append("',")
                        .append(m_overlayMode).append(",'").append("align")
                        .append("',").append(activeWin).append(",").append(
                                activeWin).append(",").append(ALIGN_1D_X)
                        .append(",0,0,0,0,0,0,0,0)");
                Util.getViewArea().sendToVnmr(id + 1, sb.toString());
            }
        }
    }

    public void sendOverlaySpecInfo(String cmd, int overlayMode, boolean align,
            boolean stack, int activeWin, int alignvp, double spx, double wpx,
            double spy, double wpy, double sp2, double wp2, double sp1,
            double wp1, boolean homo, String axis2, String axis1) {

        String mode = "none";
        if (stack)
            mode = "stack";
        else if (align)
            mode = "align";

        int chartMode = DEFAULT_CHART;
        XMap map = frameVector.elementAt(1);
        if (map != null
                && (cmd.equals("align") || m_overlayMode == OVERLAY_ALIGN)) {
            if (map.b1D_Y) {
                chartMode = ALIGN_1D_Y;
                m_bRepaintOrient = false;
                map.setVertical(true, false);
                m_bRepaintOrient = true;
            } else if (map.dcmd != null && map.dcmd.equals("dconi")) {
                chartMode = ALIGN_2D;
	    } else if(homo) {
                chartMode = ALIGN_1D_XY;
            } else {
                chartMode = ALIGN_1D_X;
            }
        } else if (map != null && map.dcmd != null) {
            if (map.dcmd.equals("dconi")) {
                chartMode = DEFAULT_2D;
            } else {
                chartMode = DEFAULT_1D;
            }
        }

        StringBuffer sb = new StringBuffer().append(JFUNC).append(OVLYTYPE).append(",'")
                .append(cmd).append("',").append(overlayMode).append(",'")
                .append(mode).append("',").append(activeWin).append(",")
                .append(alignvp).append(",").append(chartMode).append(",")
                .append(spx).append(",").append(wpx).append(",").append(spy)
                .append(",").append(wpy).append(",").append(sp2).append(",")
                .append(wp2).append(",").append(sp1).append(",").append(wp1)
		.append(",'").append(axis2).append("','").append(axis1)
		.append("')");


        vnmrIf.sendToVnmr(sb.toString());
    }

    public void overlayDispType(int n, boolean bTop) {
        m_overlayMode = n;
        if (bTop)
            dispType = 0;
        else
            dispType = n;
        sendOverlayMode(dispType);
    }

    public void overlayDispType(int n) {
        int k;
        XMap map;
        dispType = n;
        if (bOverlayed) {
            if (!bTopLayer)
                sendOverlayMode(dispType);
            else
                sendOverlayMode(0);
            if (n >= 3) {
                for (k = 2; k <= frameNum; k++) {
                    map = frameVector.elementAt(k);
                    if (map != null) {
                        map.bOpen = false;
                        map.bEnable = false;
                    }
                }
            }
        } else {
            for (k = 1; k <= frameNum; k++) {
                map = frameVector.elementAt(k);
                if (map != null) {
                    map.bOpen = map.bOrgOpen;
                    map.bEnable = map.bOrgEnable;
                }
            }
        }
        sendFrameStatus();
    }

    public void overlaySync(boolean b) {
        bSyncOvlay = b;
    }

    public boolean isOverlaySync() {
        return bSyncOvlay;
    }

    public void setGraphicsMode(String cmd) {
        // QuotedStringTokenizer tok = new QuotedStringTokenizer(cmd, " ,\n");
        StringTokenizer tok = new StringTokenizer(cmd, " ,\n");
        if (!tok.hasMoreTokens())
            return;
        String type = tok.nextToken();
        if (!tok.hasMoreTokens())
            return;
        String val;
        if (type.equals("overlayMode")) {
            val = tok.nextToken();
            if (val.equals("xor"))
                bXorOvlay = true;
            else
                bXorOvlay = false;
            if (viewIf != null)
                viewIf.setCanvasOverlayMode(val);
            return;
        }
        if (type.equals("transpancy")) {
            val = tok.nextToken();
            float fv = 1.0f;
            int v = 255;
            try {
                fv = Float.parseFloat(val);
            } catch (Exception e) {
                return;
            }

            if (fv > 1.0f)
                fv = 1.0f;
            if (fv < 0.0f)
                fv = 0.0f;
            v = (int) (255.0f * (1.0f - fv));
            alphaVal = (byte) v;
            alphaByte[bgIndex] = 0;
            return;
        }
        if (type.equalsIgnoreCase("orientation")
                || type.equalsIgnoreCase("orient")) {
            setDispOrient(frameMap, tok.nextToken("\n").trim());
            return;
        }
    }

    public void enterTrashCan(boolean b) {
        bInTrashCan = b;
        if (ds != null) {
            if (b)
                ds.setCursor(trashCursor);
            else
                ds.setCursor(noDropCursor);
        }
    }

    public void dropTrashCan() {
        if (bObjDrag) {
            if (bMoveFrame) {
            } else if (activeObj != null) {
                if (activeObj instanceof VIcon)
                    removeIcon((VIcon) activeObj);
                else if (activeObj instanceof VTextBox)
                    deleteTextBox(false, true);
            }
            bObjDrag = false;
            bObjMove = false;
            return;
        }
    }

    public void dragDropEnd(DragSourceDropEvent evt) {
        if (bActive)
            setCursor(objCursor);
        else
            setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
        cursorIndex = -1;
        if (bObjDrag)
            dropProc(evt.getX(), evt.getY());
    }

    public void dragEnter(DragSourceDragEvent e) {
        int y = e.getY();
        int x = e.getX();
        ds = e.getDragSourceContext();
        if (bInTrashCan)
            ds.setCursor(trashCursor);
        else {
            if (x >= scrX && x <= scrX2 && y >= scrY && y <= scrY2) {
                bDragInCanvas = true;
                ds.setCursor(moveCursor);
            } else if (bDragInCanvas) {
                bDragInCanvas = false;
                ds.setCursor(noDropCursor);
            }
        }
    }

    public void dragExit(DragSourceEvent evt) {
        ds = evt.getDragSourceContext();
        if (bDragInCanvas)
            ds.setCursor(noDropCursor);
        bDragInCanvas = false;
    }

    public void dragOver(DragSourceDragEvent evt) {
        if (bObjDrag) {
            int y = evt.getY();
            int x = evt.getX();
            if (x >= scrX && x <= scrX2) {
                if (y >= scrY && y <= scrY2)
                    dragProc(x - scrX, y - scrY);
            }
        }
    }

    public void dropActionChanged(DragSourceDragEvent evt) {
    }

    public void dragGestureRecognized(DragGestureEvent evt) {
        // if (bObjMove || bRubberMode)
        if (bRubberMode)
            return;
        if (!bObjDrag) {
            return;
        } else {
            if (bMoveFrame) {
                return;
            }
            moveCursor = Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR);
        }
        mousePt = evt.getDragOrigin();
        bDragInCanvas = true;
        bInTrashCan = false;
        LocalRefSelection ref = new LocalRefSelection(VnmrCanvas.this);
        dragSource.startDrag(evt, moveCursor, ref, VnmrCanvas.this);
    }

    public Point getDragStartPoint() {
        return mousePt;
    }

    public int getDragMouseBut() {
        return dragMouseBut;
    }

    private void sendDragTestStr() {
        String mess;
        if (drgCount == 0) {
            mess = new StringBuffer().append(JEVENT).append(VMOUSE).append(
                    ", ").append(1).append(", 0, ").append(0).append(", ")
                    .append(20).append(", ").append(drgY).append(", ").append(
                            MouseEvent.MOUSE_PRESSED).append(", ").append(0)
                    .append(", ").append(butInfo).append(", 0) ").toString();
        } else if (drgCount <= drgNum) {
            mess = new StringBuffer().append(JMOVE).append(VMOUSE).append(
                    ",").append(1).append(", ").append(1).append(", 0, ")
                    .append(20).append(", ").append(drgY).append(", ").append(
                            MouseEvent.MOUSE_DRAGGED).append(", 0, ").append(
                            butInfo).append(", ").append(0).append(") ")
                    .toString();
        } else {
            mess = new StringBuffer().append(JEVENT).append(VMOUSE).append(
                    ", ").append(1).append(", 0, ").append(1).append(", ")
                    .append(20).append(", ").append(drgY).append(", ").append(
                            MouseEvent.MOUSE_RELEASED).append(", ").append(0)
                    .append(", ").append(butInfo).append(", 0) ").toString();
        }
        vnmrIf.sendToVnmr(mess);
        if (bDrgUp)
            drgY -= 20;
        else
            drgY += 20;
    }

    public void dragtest(String s) {
        StringTokenizer tok = new StringTokenizer(s, " ,\n");

        if (bDrgTest)
            return;
        bDrgTest = true;
        if (bDrgUp)
            bDrgUp = false;
        else
            bDrgUp = true;
        drgNum = 10;
        drgCount = 0;
        while (tok.hasMoreTokens()) {
            String d = tok.nextToken().trim();
            if (d.equals("up"))
                bDrgUp = true;
            else if (d.equals("down"))
                bDrgUp = false;
            else {
                try {
                    drgNum = Integer.parseInt(d);
                } catch (NumberFormatException er) {
                }
            }
        }
        System.out.println("dragtest start...");
        if (bDrgUp)
            drgY = winH - 10;
        else
            drgY = 10;
        drgStart = System.currentTimeMillis();
        sendDragTestStr();
    }


    public void setImagingBg(boolean bSet) {
        bImgBg = bSet;
      /*****
        if (!bImgType)
            return;
        if (bImgBg == bSet)
            return;

        bImgBg = bSet;
        if (bImgBg)
            bgOpaque = DisplayOptions.getColor("graphics8");
        else
            bgOpaque = DisplayOptions.getColor("graphics20");
        bgColor = bgOpaque;
        setBackground(bgColor);
        if (bimgc != null) {
            bimgc.setBackground(bgColor);
            bimgc.clearRect(0, 0, winW, winH);
        }
        if (b2imgc != null) {
            b2imgc.setBackground(bgColor);
            b2imgc.clearRect(0, 0, winW, winH);
        }
        Container p = getParent();
        if (p != null)
            p.setBackground(bgOpaque);
       ****/
    }

    public void setApptype(String s) {
        if (!bImgType)
            return;
        boolean orgBg = bImgBg;
        bImgBg = false;
        if (s.startsWith("im")) {
           if (!s.startsWith("im1"))
               bImgBg = true;
        }
        if (bImgBg == orgBg)
            return;
        // set imaging background
        if (bImgBg)
            bgOpaque = DisplayOptions.getColor("graphics8");
        else
            bgOpaque = DisplayOptions.getColor("graphics20");
        int r = bgOpaque.getRed();
        int g = bgOpaque.getGreen();
        int b = bgOpaque.getBlue();
        bgAlpha = new Color(r, g, b, 0);
        bgColor = bgAlpha;
        setBackground(bgColor);
        if (bimgc != null) {
            bimgc.setBackground(bgColor);
            bimgc.clearRect(0, 0, winW, winH);
        }
        if (b2imgc != null) {
            b2imgc.setBackground(bgColor);
            b2imgc.clearRect(0, 0, winW, winH);
        }
        Container p = getParent();
        if (p != null)
            p.setBackground(bgOpaque);
        if (vnmrIf != null) {
            if (vnmrIf instanceof JComponent)
                ((JComponent) vnmrIf).setBackground(bgOpaque);
        }
        clearAllSeries();

        vnmrIf.sendToVnmr("repaint");
    }

   
    static String[] dpsColorNames =
    { "DpsBase", "DpsPulse", "DpsS",
      "DpsMS", "DpsUS", "DpsDelay", 
      "DpsLabel", "DpsPwr", "DpsPhs",
      "DpsChannel", "DpsStatus", "DpsAcquire",
      "DpsOn", "DpsOff", "DpsMark",
      "DpsHWLoop", "DpsPELoop", "DpsSelected"
    };
      // "DpsInfo", "DpsFunc" };

    public void setDpsColors() {
         int n;
         
         for (int k = 0; k < dpsColorNames.length; k++) {
             Color c = DisplayOptions.getColor(dpsColorNames[k]);
             if (c != null) {
                n = k + dpsColor0;   
                redByte[n] = (byte) c.getRed();
                grnByte[n] = (byte) c.getGreen();
                bluByte[n] = (byte) c.getBlue();
             }
         }
    }

    public void setVjColors() {
        xhairColor = DisplayOptions.getColor("crosshair");
        borderColor = DisplayOptions.getColor("border");
        hilitColor = DisplayOptions.getColor("highlight");
        hotColor = DisplayOptions.getColor("hotspot");
        shadeColor = DisplayOptions.getColor("shade");
        bgOpaque = Color.black;

        boolean bNewColor = false;
        String name;

        if (bImgType && bImgBg) {
            name = DisplayOptions.getOption("graphics8Color");
            if (bImgBg && name != null) {
                if (!name.equals(imgBgName)) {
                    bNewColor = true;
                    imgBgName = name;
                }
                bgOpaque = DisplayOptions.getColor("graphics8");
            }
        } else {
            if (DisplayOptions.getOption("graphics20Color") != null)
                bgOpaque = DisplayOptions.getColor("graphics20");
        }
        int r = bgOpaque.getRed();
        int g = bgOpaque.getGreen();
        int b = bgOpaque.getBlue();
        bgAlpha = new Color(r, g, b, 0);
        bgColor = bgAlpha;
        fgColor = Color.black;
        Container p = getParent();
        if (p != null) {
            p.setBackground(bgOpaque);
        } else if (vnmrIf != null) {
            if (vnmrIf instanceof JComponent) {
                ((JComponent) vnmrIf).setBackground(bgOpaque);
            }
        }
        if (vnmrIf != null) {
            if (vnmrIf instanceof JComponent)
                ((JComponent) vnmrIf).setBackground(bgOpaque);
        }
        setBackground(bgColor);
        if (bimgc != null)
            bimgc.setBackground(bgColor);
        if (b2imgc != null)
            b2imgc.setBackground(bgColor);
        if (bImgType) {
            name = DisplayOptions.getOption("graphics19Color");
            if (name != null && (!name.equals(imgFgName))) {
                bNewColor = true;
                imgFgName = name;
                imgColor = DisplayOptions.getColor("graphics19");
            }
            if (bNewColor)
                setImageGrayMap();
        }
        int k;
        crosshairLw = DisplayOptions.getLineThicknessPix("Crosshair");
        cursorLw = DisplayOptions.getLineThicknessPix("Cursor");
        thresholdLw = DisplayOptions.getLineThicknessPix("Threshold");
        if (crosshairLw >= STROKE_NUM)
            crosshairLw = STROKE_NUM - 1;
        if (crosshairLw < 1)
            crosshairLw = 1;
        if (cursorLw >= STROKE_NUM)
            cursorLw = STROKE_NUM - 1;
        if (cursorLw < 1)
            cursorLw = 1;
        if (thresholdLw >= STROKE_NUM)
            thresholdLw = STROKE_NUM - 1;
        if (thresholdLw < 1)
            thresholdLw = 1;

        redByte[256] = (byte) borderColor.getRed();
        grnByte[256] = (byte) borderColor.getGreen();
        bluByte[256] = (byte) borderColor.getBlue();
        for (k = 0; k < xnum; k++)
            xcolors[k] = null;
        setDpsColors();
        if (graphFontList == null)
            initFont();
        boolean bFontChanged = false;
        for (GraphicsFont gf: graphFontList) {
             if (gf.update())
                 bFontChanged = true;
        }
        if (bFontChanged) {
             DisplayOptions.writeFonts();
             // sendFontSize(1, 1, 1, 1);  // let Vnmrbg reload fonts
             sendFontInfo();
        }
        for (k = 1; k <= frameNum; k++) {
             XMap map = frameVector.elementAt(k);
             if (map != null && map.bEnable) {
                  map.updateFontSize();
                  map.bNewSize = true;
                  if (!map.bText)
                        sendFrameSize(map);
             }
        }
        if (bFontChanged)
             VnmrRepaint();
    }

    public void setCsiCanvas(VnmrCanvas pan) {
        if (bCsiWindow)
           return;
        csiCanvas = pan;
        if (csiCanvas == null)
           return;
        csiCanvas.sendMoreEvent(sendMore);
        csiCanvas.setActive(bActive);
        if (cmpModel != null)
            csiCanvas.cmpModel = cmpModel;
        if (aipCmIndex != null) {
            csiCanvas.aipCmIndex = aipCmIndex;
            csiCanvas.aipCmpId = aipCmpId;
        }
        else
            csiCanvas.aipCmpId = 0;
        if (cmIndex != null)
            csiCanvas.cmIndex = cmIndex;
        if (annPan != null)
            csiCanvas.annPan = annPan;
        if (xmapVector != null)
            csiCanvas.xmapVector = xmapVector;
        annW = 0;
        annW1 = 0;
    }

    public void setVnmrCanvas(VnmrCanvas pan) {
        vbgCanvas = pan;
        if (pan == null)
           return;
        redByte = pan.redByte;
        grnByte = pan.grnByte;
        bluByte = pan.bluByte;
        alphaByte = pan.alphaByte;
        alphaVal = pan.alphaVal;
        bgIndex = pan.bgIndex;
        fgIndex = pan.fgIndex;
        bImgType = pan.bImgType;
        bgColor = pan.bgColor;
        bgAlpha = pan.bgAlpha;
        bgOpaque = pan.bgOpaque;
        imgColor = pan.imgColor;
        annW = 0;
        annW1 = 0;
    }

    public void setCsiActive(boolean b) {
        if (!b && bCsiActive) {
           if (vbgCanvas != null) {
               if (annPan != null)
                   vbgCanvas.annPan = annPan;
               if (cmpModel != null)
                   vbgCanvas.cmpModel = cmpModel;
               if (aipCmIndex != null) {
                   vbgCanvas.aipCmIndex = aipCmIndex;
                   vbgCanvas.aipCmpId = aipCmpId;
               }
           }
           annW = 0;
           annW1 = 0;
        }
        bCsiActive = b;
    }

    private class CanvasDropTargetListener implements DropTargetListener {
        public void dragEnter(DropTargetDragEvent evt) {
            //if (!bActive || isBusy)
            if (!canDrop(evt) || isBusy)
                evt.rejectDrag();
        }

        public void dragExit(DropTargetEvent evt) {
        }

	private boolean canDrop(DropTargetDragEvent evt) {
	    if(bActive) return true;

	    // the following items (string, SQ, RQ, Locator, or Browser items) 
	    // can be dropped to non-active viewports.
            try { 
                Transferable tr = evt.getTransferable();
                if (tr.isDataFlavorSupported(DataFlavor.stringFlavor)) return true;
                Object obj = tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
                if (obj instanceof VTreeNodeElement || obj instanceof VFileElement 
			|| obj instanceof ShufflerItem) return true; 
            } catch (IOException io) {
	        return false;
            } catch (UnsupportedFlavorException ufe) {
	        return false;
            }
	    return false;
	}

        public void dragOver(DropTargetDragEvent evt) {
            //if (!bActive || isBusy)
            if (!canDrop(evt) || isBusy)
                evt.rejectDrag();
        }

        public void dropActionChanged(DropTargetDragEvent evt) {
        }

        public void drop(DropTargetDropEvent evt) {
            //if (!bActive || isBusy) {
            if (isBusy) {
                evt.rejectDrop();
                return;
            }

            Point dropPt = evt.getLocation();
            try {
                Transferable tr = evt.getTransferable();

                // Catch drag of String (probably from JFileChooser)
                if (tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                    Object obj = tr.getTransferData(DataFlavor.stringFlavor);
                    evt.acceptDrop(DnDConstants.ACTION_COPY);
                    evt.getDropTargetContext().dropComplete(true);

                    // The object, being a String, is the fullpath of the
                    // dragged item.
                    String fullpath = (String) obj;
                    File file = new File(fullpath);
                    if (!file.exists()) {
                        Messages.postError("File not found " + fullpath);
                        return;
                    }

                    // If this is not a locator recognized objType, then
                    // disallow the drag.
                    // Allow unknown to get to locaction, the user may want to do something there.
                    //                    String objType = FillDBManager.getType(fullpath);
                    //                   if(objType.equals("?")) {
                    //                        Messages.postError("Unrecognized drop item " + 
                    //                                           fullpath);
                    //                        return;
                    //                    }

                    // Create a ShufflerItem
                    // Since the code is accustom to dealing with ShufflerItem's
                    // being D&D, I will just use that type and fill it in.
                    // This allows ShufflerItem.actOnThisItem() and the macro
                    // locaction to operate normally and take care of this item.
                    ShufflerItem item = new ShufflerItem(fullpath, "BROWSER");

                    // Finish as we would if a ShufflerItem had been dropped
	            Util.setDropView((ExpPanel)vnmrIf);
                    handleShufflerItem(item, dropPt);
	            Util.removeDropView();

                    return;
                }
                // If not a String, then should be in this set of if's
                if (!tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR)) {
                    evt.rejectDrop();
                    return;
                }
                RQBuilder rqmgr = null;
                Object obj = tr
                        .getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
                if (obj == null) {
                    evt.rejectDrop();
                    return;
                }
                if (obj instanceof VFileElement) {
	            Util.setDropView((ExpPanel)vnmrIf);
                    evt.acceptDrop(DnDConstants.ACTION_MOVE);
                    VFileElement elem = (VFileElement) obj;
                    rqmgr = (RQBuilder) elem.getTemplate();
                    if (rqmgr != null) {
                        rqmgr.dropToVnmrXCanvas(elem, dropPt,rqmgr.getMod());
                    }
                    evt.getDropTargetContext().dropComplete(true);
	            Util.removeDropView();
                    return;
                }

                rqPanel = Util.getRQPanel();
                rq = null;
                if (rqPanel != null)
                    rq = rqPanel.getReviewQueue();
                if (obj instanceof VTreeNodeElement) {
	            Util.setDropView((ExpPanel)vnmrIf);
                    VElement elem = (VTreeNodeElement) obj;
                    evt.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
                    String data = elem.getAttribute(ProtocolBuilder.ATTR_DATA);
                    ProtocolBuilder pb = (ProtocolBuilder) elem.getTemplate();
                    if (data.length() > 0 && pb != null) {
                        String path = pb.getStudyPath();
                        if (path.startsWith(File.separator)) {
                            path = path + File.separator + data;
                        } else {
                            path = data;
                        }

                        String imgPath = path;
                        // load images if exist 
                        if (path.endsWith(".fid")) {
                            imgPath = path.substring(0, path.length() - 4)
                                    + ".img";
                        } else if (!path.endsWith(".img")
                                && !path.endsWith(".fdf")) {
                            imgPath = path + ".img";
                        }

                        File f = new File(imgPath);
                        if (f.exists()) { // load images

                            if (rq != null
                                    && Util.getViewPortType().equals(
                                            Global.REVIEW))
                                rqmgr = (RQBuilder) rq.getMgr();

			    String mod = pb.getMod();
                            if (rqmgr != null) { // review vp
                                rqmgr.dropToVnmrXCanvas(imgPath, dropPt, mod);
			    } else if(mod.length() > 0) {
                                String cmd = "RQaction('moddnd', '" + imgPath
                                        + "'," + dropPt.x + ", " + dropPt.y
                                        + ")";
                                Util.sendToVnmr(cmd);
                            } else {
                                String cmd = "RQaction('dnd', '" + imgPath
                                        + "'," + dropPt.x + ", " + dropPt.y
                                        + ")";
                                Util.sendToVnmr(cmd);
                            }
                        } else {
                            pb.setDragAndDrop();
                        }
                    }
                    evt.getDropTargetContext().dropComplete(true);
	            Util.removeDropView();
                    return;
                }
                if (obj instanceof ShufflerItem) {
	            Util.setDropView((ExpPanel)vnmrIf);
                    evt.acceptDrop(DnDConstants.ACTION_COPY);
                    evt.getDropTargetContext().dropComplete(true);
                    ShufflerItem item = (ShufflerItem) obj;
                    handleShufflerItem(item, dropPt);
	            Util.removeDropView();
                    return;
                }
                if (obj instanceof VnmrCanvas) {
                    if (bObjDrag) {
                        evt.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
                        evt.getDropTargetContext().dropComplete(true);
                        return;
                    }

                    if (activeObj != null && activeObj.isSelected()) {
                        /**
                         m_nMousex = mousePt.x;
                         m_nMousey = mousePt.y;
                         Point mousePos = evt.getLocation();
                         drawIcon(mousePos.x, mousePos.y, activeObj.getWidth(), activeObj.getHeight());
                         **/
                    } else if (rq != null) {
                        int action = evt.getDropAction();
                        if (action == DnDConstants.ACTION_MOVE)
                            rq.doMoveFrame(mousePt, dropPt, dragMouseBut);
                        else if (action == DnDConstants.ACTION_COPY)
                            rq.doCopyFrame(mousePt, dropPt, dragMouseBut);
                    }

                    evt.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
                    evt.getDropTargetContext().dropComplete(true);
                    return;
                }
            } catch (IOException io) {
            } catch (UnsupportedFlavorException ufe) {
            }
            evt.rejectDrop();

        } // drop()
    }

    private void handleShufflerItem(ShufflerItem item, Point point) {
        RQBuilder rqmgr = null;

        String path = item.getFullpath();
        // load images if exist 
        String imgPath = path;
        molDropPt = null;
        if (path.endsWith(Shuf.DB_FID_SUFFIX)) {
            imgPath = path.substring(0, path.length() - 4)
                    + Shuf.DB_IMG_DIR_SUFFIX;
        } else if (!path.endsWith(Shuf.DB_IMG_DIR_SUFFIX)
                && !path.endsWith(Shuf.DB_IMG_FILE_SUFFIX)
                && !path.endsWith(Shuf.DB_CMP_DIR_SUFFIX)) {
            imgPath = path + Shuf.DB_IMG_DIR_SUFFIX;
        }

        File f = new File(imgPath);
        if (f.exists()) { // load images

            if (rq != null && Util.getViewPortType().equals(Global.REVIEW))
                rqmgr = rq.getMgr();
            if (rqmgr != null) {
                rqmgr.dropToVnmrXCanvas(path, point,"");
	    } else {
                String cmd = "RQaction('dnd', '" + path + "'," + point.x + ", "
                        + point.y + ")";
                Util.sendToVnmr(cmd);
            }
        } else {
            // Decide what is to be done with this item.
            // target is canvas and action is dnd
            molDropPt = point;
            item.actOnThisItem("Canvas", "DragNDrop", "");
        }
        //vnmrIf.requestActive();

    }

    private void showOrientPopup(boolean bShow, int x, int y) {
        if (orientPopup == null) {
            FramePopupAction ear = new FramePopupAction();
            orientPopup = new JPopupMenu();
            orientPopup.setBackground(Util.getBgColor());
            JMenuItem mi = new JMenuItem("Horizontal Display");
            mi.addActionListener(ear);
            orientPopup.add(mi);
            mi = new JMenuItem("Vertical Rightward");
            mi.addActionListener(ear);
            orientPopup.add(mi);
            mi = new JMenuItem("Vertical Leftward");
            mi.addActionListener(ear);
            orientPopup.add(mi);
        }
        if (bShow)
            orientPopup.show(this, x + 4, y);
    }

    public void setDispOrient(XMap map, String s) {
        if (map == null || !m_bUpdateOrient) {
            m_bUpdateOrient = true;
            return;
        }
        if (s.startsWith("vertical") || s.startsWith("Vertical")) {
            if (s.indexOf("Right") > 0 || s.indexOf("right") > 0)
                map.setVertical(true, true);
            else
                map.setVertical(true, false);
            return;
        }
        map.setVertical(false, false);
    }

    private void doIconAction(String s) {
        if (m_vIcon == null)
            return;
        bDraging = false;
        if (s.startsWith("Delete")) {
            removeIcon(m_vIcon);
            return;
        }
        m_vIcon.setRatio(winW, winH, false);
        if (s.startsWith("Hide")) {
            if (s.startsWith("Hide Image"))
                hideIcon(m_vIcon);
            return;
        }
        if (s.startsWith("Edit")) {
            // appIf.showJMolPanel(true);
            appIf.showJMolPanel(winId, true);
            return;
        }
        if (s.startsWith("Default")) {
            m_vIcon.setDefaultSize();
            sendIconInfo(m_vIcon);
            set_active_obj(activeObj);
            return;
        }
        if (s.startsWith("Half")) {
            m_vIcon.setSizeRatio(0.5f);
            sendIconInfo(m_vIcon);
            set_active_obj(activeObj);
            return;
        }
        if (s.startsWith("Double")) {
            m_vIcon.setSizeRatio(2.0f);
            sendIconInfo(m_vIcon);
            set_active_obj(activeObj);
            return;
        }
    }

    private class IconPopupAction implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JMenuItem obj = (JMenuItem) e.getSource();
            if (obj != null) {
                doIconAction(obj.getText());
                repaint_canvas();
            }
        }
    }

    private void showIconPopup(boolean bShow, int x, int y) {
        if (m_vIcon.isMol()) {
            if (molIconPopup == null) {
                IconPopupAction e = new IconPopupAction();
                molIconPopup = new JPopupMenu();
                molIconPopup.setBackground(Util.getBgColor());
                JMenuItem mi = new JMenuItem("Edit Jmol");
                mi.addActionListener(e);
                molIconPopup.add(mi);
                mi = new JMenuItem("Default Size");
                mi.addActionListener(e);
                molIconPopup.add(mi);
                mi = new JMenuItem("Half Size");
                mi.addActionListener(e);
                molIconPopup.add(mi);
                mi = new JMenuItem("Double Size");
                mi.addActionListener(e);
                molIconPopup.add(mi);
                mi = new JMenuItem("Hide Image");
                mi.addActionListener(e);
                molIconPopup.add(mi);
                mi = new JMenuItem("Delete Image");
                mi.addActionListener(e);
                molIconPopup.add(mi);
            }
            if (bShow)
                molIconPopup.show(this, x + 8, y);
            return;
        }
        if (iconPopup == null) {
            IconPopupAction ear = new IconPopupAction();
            iconPopup = new JPopupMenu();
            iconPopup.setBackground(Util.getBgColor());
            JMenuItem mi = new JMenuItem("Default Size");
            mi.addActionListener(ear);
            iconPopup.add(mi);
            mi = new JMenuItem("Half Size");
            mi.addActionListener(ear);
            iconPopup.add(mi);
            mi = new JMenuItem("Double Size");
            mi.addActionListener(ear);
            iconPopup.add(mi);
            mi = new JMenuItem("Hide Image");
            mi.addActionListener(ear);
            iconPopup.add(mi);
            mi = new JMenuItem("Delete Image");
            mi.addActionListener(ear);
            iconPopup.add(mi);
        }
        if (bShow)
            iconPopup.show(this, x + 8, y);
    }

    private void doTextBoxAction(String s) {
        if (activeObj == null || !(activeObj instanceof VTextBox))
            return;
        if (s.startsWith("Edit")) {
            TextboxEditor.execCmd("open");
            TextboxEditor.setEditObj((VTextBox) activeObj);
            return;
        }
        if (s.startsWith("Delete")) {
            if (s.startsWith("Delete All"))
                deleteTextBox(true, true);
            else
                deleteTextBox(false, false);
            return;
        }
        if (s.startsWith("Hide")) {
            if (s.startsWith("Hide All"))
                setTextBoxVisible(false, true);
            else
                setTextBoxVisible(false, false);
            return;
        }
    }

    private class TextBoxPopupAction implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JMenuItem obj = (JMenuItem) e.getSource();
            if (obj != null) {
                doTextBoxAction(obj.getText());
            }
        }
    }

    private void showTextBoxPopup(boolean bShow, int x, int y) {
        if (textBoxPopup == null) {
            TextBoxPopupAction ear = new TextBoxPopupAction();
            textBoxPopup = new JPopupMenu();
            textBoxPopup.setBackground(Util.getBgColor());
            JMenuItem mi = new JMenuItem("Edit Text");
            mi.addActionListener(ear);
            textBoxPopup.add(mi);
            mi = new JMenuItem("Hide Text");
            mi.addActionListener(ear);
            textBoxPopup.add(mi);
            mi = new JMenuItem("Delete Text");
            mi.addActionListener(ear);
            textBoxPopup.add(mi);
            /*
             mi = new JMenuItem("Hide All Text");
             mi.addActionListener(ear);
             textBoxPopup.add(mi);
             mi = new JMenuItem("Delete All Text");
             mi.addActionListener(ear);
             textBoxPopup.add(mi);
             */
        }
        if (bShow)
            textBoxPopup.show(this, x + 4, y);
    }

    private class FramePopupAction implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JMenuItem obj = (JMenuItem) e.getSource();
            if (obj != null) {
                setDispOrient(frameMap, obj.getText());
            }
        }
    }

    private class ParamPopup extends JPopupMenu {
        public ParamPopup(String path) {
            super();
            readParams(path);
        }

        public ParamPopup(String path, String title) {
            super(title);
            readParams(path);
        }

        private void readParams(String path) {

            setLightWeightPopupEnabled(false);
            BufferedReader in = null;
            String line;
            String symbol;
            String value;
            StringTokenizer tok;

            try {
                in = new BufferedReader(new FileReader(path));
                while ((line = in.readLine()) != null) {
                    tok = new StringTokenizer(line, " ");
                    if (!tok.hasMoreTokens())
                        continue;
                    symbol = tok.nextToken().trim();
                    value = "";
                    while (tok.hasMoreTokens())
                        value += tok.nextToken().trim() + " ";
                    JLabel col1 = new JLabel(symbol, JLabel.LEFT);
                    col1.setForeground(Color.blue);
                    JLabel col2 = new JLabel(value, JLabel.LEFT);
                    JPanel row = new JPanel();
                    row.setLayout(new GridLayout(1, 2));
                    row.add(col1);
                    row.add(col2);
                    add(row);
                }
                in.close();
            } catch (Exception e) {
                System.out.println("problem reading persistence file");
                Messages.writeStackTrace(e);
            }
        }
    }

    private class XColor extends Color {
        // private int index = 0;
        private int v = 0;
        private int a = 0;

        public XColor(int r, int g, int b, int i) {
            super(r, g, b, 0);
            // this.index = i;
            v = super.getRGB();
            a = (i & 0xff) << 24;
            v = v | a;
        }

        public XColor(Color c, int i) {
            this(c.getRed(), c.getGreen(), c.getBlue(), i);
        }

        public int getAlpha() {
            return (a >> 24) & 0xff;
        }

        public int getRGB() {
            return v;
        }

        public int hashCode() {
            return super.hashCode();
        }
    }

    public class Crosshair{
        public int xmin,xmax,ymin,ymax;
        public Color color;
        Crosshair(){
            xmin=xmax=ymin=ymax=0;
        }
        void set(int x1, int y1,int x2, int y2){
            xmin=x1;xmax=x2;ymin=y1;ymax=y2;
        }
        void set(Color c){
            color=c;
        }
        void draw(Graphics g){
            g.setColor(color);
            g.drawLine(xmin, ymin, xmax, ymax);           
        }
    }
    public class XMap {
        public int id;
        public int imgId;
        public int x = 0;
        public int y = 0;
        public int x2 = 0;
        public int y2 = 0;
        public int width = 0;
        public int height = 0;
        public int rw = 0; // actual width of image
        public int rh = 0; // actual height of image
        public int fontH = 0;
        public int fontW = 0;
        private int ftAscent = 0;
        private int ftDescent = 0;
        public int fontSize = 999;
        public int rotateX = 0;
        public int rotateY = 0;
        public int prX = 0;
        public int prY = 0;
        public int prW = 0;
        public int prH = 0;
        public int mapAlpha = 255;
        public int cmpId = 0;
        public int[] th_locs; // threshold position
        public int[] th_starts; // threshold start point
        public int[] th_lengths; // threshold length
        public int[] ds_th_colors;
        public double rotateDeg = 0;
        public float ratioX = 0;
        public float ratioY = 0;
        public float ratioW = 0.2f;
        public float ratioH = 0.2f;
        public float textFs = 0;
        public float xalphaSet = 1.0f;
        public Graphics2D gc = null;
        public Graphics2D orgGc = null;
        public BufferedImage img = null;
        public boolean xbActive = false;
        public boolean bOpen = false;
        public boolean bOrgOpen = false;
        public boolean bEnable = true;
        public boolean bOrgEnable = true;
        public boolean bText = false;  // is text object
        public boolean bImg = false;   // is imaging object
        public boolean bNewSize = true;
        public boolean xbRexec = false;
        public boolean bFixedFont = false;
        public boolean bVertical = false;
        public boolean bRight = false; // right side or left side
        public boolean bOpaque = false;
        public boolean bTopMap = false;
        public boolean bTopMapOpened = false;
        public Font font;
        public FontMetrics fm;
        public String fontName = null;
        public String colorName = null;
        public String textStr = null; // the longest text string
        public Color txtColor = Color.yellow;
        public Vector<String> textVec = null;
        public String dcmd = null;
        public String axis2 = null;
        public String axis1 = null;
        public double spx = 0.0;
        public double spy = 0.0;
        public double epx = 0.0;
        public double epy = 0.0;
        public double sp2 = 0.0;
        public double sp1 = 0.0;
        public double ep2 = 0.0;
        public double ep1 = 0.0;
        public boolean b1D_Y = false;
        public int trace = 1;
        public AlphaComposite alphaComp = null;
        protected java.util.List<VJYbar> ybarList;
        protected java.util.List<VJLine> lineList;
        protected java.util.List<VJText> textList;
        protected java.util.List<VJBackRegion> backRegionList;
        protected AlphaComposite clearComposite = null;
        protected VColorModel xcmpModel = null;
        protected IndexColorModel xcmIndex;
        protected XMap baseMap = null;
        protected XMap topMap = null;

        public XMap(int id, int x, int y, int w, int h) {
            this.id = id;
            this.x = x;
            this.y = y;
            this.width = 0;
            this.height = 0;
            font = org_font;
            fontW = org_fontWidth;
            fontH = org_fontHeight;
            th_locs = new int[CURSOR_NUM];
            th_starts = new int[CURSOR_NUM];
            th_lengths = new int[CURSOR_NUM];
            ds_th_colors = new int[CURSOR_NUM];
            clearThreshold();
            setSize(w, h);
        }

        public XMap(int id, int w, int h) {
            this(id, 0, 0, w, h);
        }

        public void setImageId(int i) {
            imgId = i;
        }

        public int getImageId() {
            return imgId;
        }

        public void setColorMap(VColorModel cm) {
            if (cm == null)
               return;
            if (cm == xcmpModel)
               return;
            xcmpModel = cm;
            mapAlpha = cm.getAlphaValue();
            xcmIndex = xcmpModel.getModel();
        }

        public VColorModel getColorMap() {
            return xcmpModel;
        }

        public void setColormapId(int id) {
            cmpId = id;
        }

        public int getColormapId() {
            return cmpId;
        }

        public IndexColorModel getColorModel() {
            return xcmIndex;
        }

        public void setTransparent(int alpha) {
            if (Math.abs(mapAlpha - alpha) < 3)
                return;
            mapAlpha = alpha;
            /***
            if (xcmpModel != null) {
                xcmIndex = xcmpModel.cloneModel(alpha);
            }
            ***/
        }

        public BufferedImage getImage() {
            return img;
        }

        public void set1D_Y(int i, int tr) {
            // i = 2 is x, i = 1 is y. 
            if (dcmd == null)
                b1D_Y = false;
            else if (dcmd.equals("dconi"))
                b1D_Y = false;
            else if (i == 1)
                b1D_Y = true;
            else if (i == 2)
                b1D_Y = false;
            else
                b1D_Y = false;
        }

        public void updateFontSize() {
             // Font newFont = defaultFont.getFont(width, height);
             // fm = getFontMetrics(newFont);
             fm = defaultFont.fontMetric;
             fontW = defaultFont.getFontWdith();
             ftAscent = defaultFont.getFontAscent();
             ftDescent = defaultFont.getFontDescent();
             fontH = defaultFont.getFontHeight();
        }

        public void setMapSize(int w, int h) {
            boolean bNewGc = false;
            boolean bNewMap = false;

            if (w < 2 || h < 2)
                return;
            if (width == w && height == h) {
                if (img != null)
                    return;
                bNewMap = true;
            }
            else
                bNewSize = true;

            if (w > rw || h > rh)
                bNewMap = true;
            else {
                if (bImg) {
                    if (rw > 400 && (rw / 3 > w))
                        bNewMap = true;
                    else if (rh > 400 && (rh / 3 > h))
                        bNewMap = true;
                }
                else if (bTopMap) {
                    if (((rw - w) > 20) || ((rh - h) > 20))
                        bNewMap = true;
                }
            }
            if (bNewMap || gc == null) {
                int h2 = h + 4;
                int w2 = w + 4;
                boolean bClear = true;
                bNewMap = true;
                BufferedImage newImg = null;
                GraphicsConfiguration gconfig = getGraphicsConfiguration();
                if (gconfig == null)
                    return;
                if (bTranslucent) {
                   try {
                      newImg = gconfig.createCompatibleImage(w2, h2, Transparency.TRANSLUCENT);
                   }
                   catch (OutOfMemoryError e0) {
                       newImg = null;
                   }
                   catch (Exception e1) {
                       newImg = null;
                       Messages.writeStackTrace(e1);
                   }
                }
                if (newImg == null) {
                   bTranslucent = false;
                   try {
                       newImg = gconfig.createCompatibleImage(w2, h2, Transparency.BITMASK);
                   }
                   catch (OutOfMemoryError e0) {
                       newImg = null;
                   }
                   catch (Exception e1) {
                       newImg = null;
                       Messages.writeStackTrace(e1);
                   }
                   if (newImg == null) {
                      if (bVerbose)
                        System.out.println(" Error:  new map is null ");
                      return;
                   }
                }
                if (gc != null)
                    gc.dispose();
                rw = w2;
                rh = h2;
                gc = newImg.createGraphics();
                gc.setBackground(bgColor);
                gc.setFont(font);
                orgGc = newImg.createGraphics();
                if (clearComposite != null)
                     orgGc.setComposite(clearComposite);
                if (DisplayOptions.isAAEnabled())
                     gc.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
                bNewGc = true;
                if (!bImg) {
                    if (!xbRexec && !bText && img != null) {
                        if (w2 > width)
                            w2 = width;
                        if (h2 > height)
                            h2 = height;
                        bClear = false;
                        gc.drawImage(img, 0, 0, w2, h2, 0, 0, w2, h2, null);
                    }
                }
                img = newImg;
                if (bClear)
                    clear();
            }
            /********
            if (bImg && !bNewMap) {
                if (width > w || height > h || width <= 10)
                     clear();
            }
            ********/
            height = h;
            width = w;
            if (bNewGc || bVertical) {
                if (!bNewGc)
                    resetGc(gc);
                rotateGc(gc);
            }
            if (graphFontList == null)
                initFont();
            if (gc != null && !bText) {
                updateFontSize();
                font = graphFont.getFont(w, h);
                gc.setFont(font);
            }
            if (bText) {
                // clear();
                drawTextObj();
            }
        }

        public void setSize(int w, int h) {
            int  cw = winW;
            int  ch = winH;
            if (x < 0)
                x = 0;
            if (y < 0)
                y = 0;
            if (w < MINWH)
                w = MINWH;
            if (h < MINWH)
                h = MINWH;
            if (cw < MINWH)
                cw = MINWH;
            if (ch < MINWH)
                ch = MINWH;
            if (!bPrintMode) {
                if (w > cw)
                    w = cw;
                if (h > ch)
                    h = ch;
            }
            if (x + w > cw)
                x = cw - w;
            if (y + h > ch)
                y = ch - y;
            ratioX = (float) x / (float) cw;
            ratioY = (float) y / (float) ch;
            ratioW = (float) w / (float) cw;
            ratioH = (float) h / (float) ch;
            x2 = x + w - 1;
            y2 = y + h - 1;
            setMapSize(w, h);
        }

        public void adjustSize() {
            int w = (int) (ratioW * (float) winW);
            int h = (int) (ratioH * (float) winH);
            x = (int) (ratioX * (float) winW);
            y = (int) (ratioY * (float) winH);
            x2 = x + w - 1;
            y2 = y + h - 1;
            setMapSize(w, h);
        }

        public void adjustSize(int pw, int ph) {
            int w = (int) (ratioW * (float) pw);
            int h = (int) (ratioH * (float) ph);
            x = (int) (ratioX * (float) pw);
            y = (int) (ratioY * (float) ph);
            x2 = x + w - 1;
            y2 = y + h - 1;
            setMapSize(w, h);
        }


        public void setActive(boolean b) {
            xbActive = b;
        }

        public void clear(int dx, int dy, int w, int h) {
            if (img == null || orgGc == null)
                return;
            if (bArgbImg) {
                if (clearComposite == null) {
                   clearComposite = AlphaComposite.getInstance(AlphaComposite.CLEAR, 0.0f);
                   orgGc.setComposite(clearComposite);
                }
                orgGc.fillRect(dx, dy, w, h);
            }
            else {
                orgGc.setBackground(bgColor);
                orgGc.clearRect(dx, dy, w, h);
            }
        }

        public void clear() {
            clear(0, 0, rw, rh);
            clearThreshold();
            clearSeries(false);
            clearBackRegion();
            bTopMapOpened = false;
            clipRect = null;
            if (gc != null)
                gc.setClip(null);
            if (topMap != null)
                topMap.clear();
        }

        public void clearThreshold() {
            if (baseMap != null) {
                baseMap.clearThreshold();
                return;
            }
            for (int k = 0; k < CURSOR_NUM; k++) {
                th_locs[k] = 0;
                th_starts[k] = 0;
                th_lengths[k] = 0;
                ds_th_colors[k] = 50;
            }
        }

        public void setThLoc(int n, int loc, int pt0, int len) {
            if (baseMap != null) {
                baseMap.setThLoc(n, loc, pt0, len);
                return;
            }
            if (n >= CURSOR_NUM)
                return;
            th_locs[n] = loc;
            th_starts[n] = pt0;
            th_lengths[n] = len;
        }

        public void setThColor(int n, int v) {
            if (baseMap != null) {
                baseMap.setThColor(n, v);
                return;
            }
            if (n >= CURSOR_NUM)
                return;
            ds_th_colors[n] = v;
        }

        public void drawThreshold(Graphics2D g2d) {
            if (baseMap != null) {
                baseMap.drawThreshold(g2d);
                return;
            }
            if (bText)
                return;
            int k, c;
            int x0, x1;
            int y0;
            if (thresholdLw > 1)
                g2d.setStroke(strokeList[thresholdLw]);

            x0 = x + 2;
            x1 = x + width - 4;
            y0 = y;
            c = 0;
            for (k = 0; k < CURSOR_NUM; k++) {
                if (th_locs[k] > 0 && ds_th_colors[k] < 18) {
                    if (c != ds_th_colors[k]) {
                        c = ds_th_colors[k];
                        g2d.setColor(DisplayOptions.getColor(c));
                    }
                    y0 = y + th_locs[k];
                    x0 = x + th_starts[k];
                    x1 = x0 + th_lengths[k];
                    g2d.drawLine(x0, y0, x1, y0);
                }
            }
            if (thresholdLw > 1)
                g2d.setStroke(strokeList[1]);
        }

        public void setBg() {
            if (gc != null)
                gc.setBackground(bgColor);
        }

        public Color getBg() {
            return bgOpaque;
        }

        public void delete() {
            if (gc != null)
                gc.dispose();
            gc = null;
            if (orgGc != null)
                orgGc.dispose();
            orgGc = null;
            img = null;
            font = null;
            fm = null;
            fontName = null;
            colorName = null;
            textStr = null;
            textVec = null;
            dcmd = null;
            axis1 = null;
            axis2 = null;
            bOpen = false;
            rw = 0;
            rh = 0;
            width = 0;
            height = 0;
            if (ybarList != null) {
               ybarList.clear();
               ybarList = null;
            }
            if (lineList != null) {
               lineList.clear();
               lineList = null;
            }
            if (textList != null) {
               textList.clear();
               textList = null;
            }
            if (topMap != null)
                topMap.delete();
            topMap = null;
            baseMap = null;
        }

        public void drawTextObj() {
            if (gc == null || textVec == null)
                return;
            if (!bPrintMode)
                clear();
            if (bColorPaint)
                gc.setColor(txtColor);
            int lineNum = textVec.size();
            String str;
            int dh, i;
            int pw = width;
            int ph = height;
            if (bVertical) {
                pw = height;
                ph = width;
            }
            if (!bFixedFont) {
                int lh = 0;
                int lw = 0;
                int sw = 0;
                int dw = 0;
                int sLen;
                float fh1, fh2;
                float fw, fh;
                lw = fm.stringWidth(textStr);
                dw = pw - lw;
                dh = ph - fontH * lineNum;
                sLen = textStr.length();
                if (textFs <= 0)
                    textFs = (float) fontH;
                if (dw > 20 && dh > lineNum) {
                    sw = lw;
                    fw = pw;
                    while (dw > 10 && dh >= lineNum) {
                        fh1 = textFs * (fw / (float) sw - 1.0f) - 2.0f;
                        fh2 = (float) (dh / lineNum);
                        if (fh1 > fh2)
                            fh1 = fh2;
                        if (fh1 > 1.0f)
                            textFs += fh1;
                        else {
                            textFs += 1.0f;
                            if (dw >= sLen)
                                textFs += 1.0f;
                            if (dw >= sLen * 2)
                                textFs += 2.0f;
                        }
                        font = font.deriveFont(textFs);
                        fm = getFontMetrics(font);
                        sw = fm.stringWidth(textStr);
                        dw = pw - sw;
                        fontH = fm.getAscent() + fm.getDescent();
                        dh = ph - fontH * lineNum;
                    }
                }

                if (dw < 0 || dh < 0) {
                    dw = 0 - dw;
                    dh = 0 - dh;
                    fh = 1;
                    fh2 = 1;
                    if (dh > lineNum) {
                        lh = dh / lineNum;
                        fh = (float) lh;
                    }
                    if (dw > 1) {
                        fh2 = (float) dw / (float) sLen;
                    }
                    if (fh > fh2)
                        textFs = textFs - fh;
                    else
                        textFs = textFs - fh2;
                    dw = 1;
                    dh = 1;
                    while (dw > 0 || dh > 0) {
                        if (textFs < 6.0f)
                            break;
                        textFs -= 1.0f;
                        if (dw > sLen)
                            textFs -= 1.0f;
                        if (dw >= sLen * 2)
                            textFs -= 2.0f;
                        font = font.deriveFont(textFs);
                        fm = getFontMetrics(font);
                        sw = fm.stringWidth(textStr);
                        dw = sw - pw;
                        fontH = fm.getAscent() + fm.getDescent();
                        dh = fontH * lineNum - ph;
                    }
                }
                ftAscent = fm.getAscent();
                ftDescent = fm.getDescent();
                fontH = ftAscent + ftDescent;
            }
            gc.setFont(font);
            dh = ftAscent + 2;
            for (i = 0; i < lineNum; i++) {
                str = textVec.elementAt(i);
                gc.drawString(str, 2, dh);
                dh += fontH;
            }
        }

        public void drawTextObj(Vector<String> lines, String cs, String fs, int f) {
            textVec = lines;
            colorName = cs;
            int style = Font.PLAIN;
            int h, l1, l2, ll;
            if (fs.indexOf("Bold") != -1)
                style = Font.BOLD;

            if (f > 1) {
                bFixedFont = true;
                textFs = 0;
            } else
                bFixedFont = false;
            if (fontSize != f || !fs.equals(fontName)) {
                h = f;
                if (h < 6) // flexible or not reasonable size
                    h = winH;
                if (fs.indexOf("Italic") != -1) {
                    font = new Font("Dialog", style | Font.ITALIC, h);
                } else
                    font = new Font("Dialog", style, h);
                fm = getFontMetrics(font);
                fontW = fm.stringWidth("ppmm9") / 5;
                ftAscent = fm.getAscent();
                ftDescent = fm.getDescent();
                fontH = ftAscent + ftDescent;
            }
            fontSize = f;
            fontName = fs;
            txtColor = VnmrRgb.getColorByName(cs);
            ll = textVec.size();
            l2 = 0;
            for (int i = 0; i < ll; i++) {
                String str = textVec.elementAt(i);
                l1 = fm.stringWidth(str);
                if (l1 > l2) {
                    textStr = str;
                    l2 = l1;
                }
            }
            drawTextObj();
        }

        public void rotateGc(Graphics2D g, int w, int h, boolean bRecord) {
            if (!bVertical)
                return;
            int dx = w;
            int dy = h;
            double deg = deg90;
            if (bRight) {
                dy = 0;
            } else {
                deg = -deg90;
                dx = 0;
            }
            g.setClip(0, 0, w, h);
            g.translate(dx, dy);
            g.rotate(deg);
            if (bRecord) {
                rotateDeg = deg;
                rotateX = dx;
                rotateY = dy;
            }
        }

        public void rotateGc(Graphics2D g) {
            rotateGc(g, width, height, true);
        }

        public void resetGc(Graphics2D g) {
            if (rotateDeg == 0)
                return;
            if (rotateDeg != 0)
                g.rotate(-rotateDeg);
            if (rotateX != 0 || rotateY != 0)
                gc.translate(-rotateX, -rotateY);
            g.setClip(0, 0, rw, rh);
        }

        public void setVertical(boolean v, boolean d) {
            if (bVertical != v || bRight != d) {
                boolean ov = bVertical;
                resetGc(gc);
                if (bText || xbActive)
                    clear();
                bNewSize = true;
                bVertical = v;
                bRight = d;
                if (bVertical)
                    rotateGc(gc);
                else {
                    rotateDeg = 0;
                    rotateX = 0;
                    rotateY = 0;
                }
                if (bText) {
                    drawTextObj();
                    repaint_canvas();
                } else if (xbActive) {
                    bgc = gc;
                    sendFrameSize(this);
                    if (ov == v && bVertical) {
                        if (m_bRepaintOrient)
                            vnmrIf.sendToVnmr("repaint");
                    }
                }
            }
        }

        public void printText(Graphics2D pgc) {
            if (prW <= 0 || prH <= 0)
                return;
            if (!bText)
                return;
            Graphics2D oldGc = gc;
            gc = (Graphics2D) pgc.create(prX, prY, prW, prH);
            if (gc == null) {
                gc = oldGc;
                return;
            }
            int oldW = width;
            int oldH = height;
            Font oldFont = font;
            float fw = (float) prW / (float) width;
            float fh = (float) prH / (float) height;
            float fx = textFs;
            width = prW;
            height = prH;
            if (bFixedFont) {
                if (fx < 1.0f)
                    fx = 12.0f;
                if (fw < fh)
                    textFs = fx * fh;
                else
                    textFs = fx * fw;
                font = font.deriveFont(textFs);
                fm = getFontMetrics(font);
                ftAscent = fm.getAscent();
                ftDescent = fm.getDescent();
                fontH = ftAscent + ftDescent;
            }

            if (bVertical) {
                if (bRight) {
                    gc.translate(width, 0);
                    gc.rotate(deg90);
                } else {
                    gc.translate(0, height);
                    gc.rotate(-deg90);
                }
            }
            drawTextObj();
            gc.dispose();
            gc = oldGc;
            if (bFixedFont) {
                font = oldFont;
                fm = getFontMetrics(font);
                ftAscent = fm.getAscent();
                ftDescent = fm.getDescent();
                fontH = ftAscent + ftDescent;
                textFs = fx;
            }
            width = oldW;
            height = oldH;
        }

        public java.util.List<VJYbar> getYbarList() {
            if (baseMap != null)
               return baseMap.getYbarList();
            if (ybarList == null)
               ybarList = Collections.synchronizedList(new LinkedList<VJYbar>());
            return ybarList;
        }

        protected void clearSeries(int dx, int dy, int w, int h) {
            if (baseMap != null) {
                baseMap.clearSeries(dx, dy, w, h);
                return;
            }
            if (h == fontH) // from vbg ParameterLine
                return;
            int dx2 = dx + w;
            int dy2 = dy + h;
            if (ybarList != null) {
                for (VJYbar bar : ybarList) {
                    if (bar.intersects(dx, dy, dx2, dy2)) {
                        if (bar.isTopLayer() == bTopFrameOn)
                            bar.setValid(false);
                    }
                }
            }
            if (lineList != null) {
                for (VJLine line : lineList) {
                    if (line.intersects(dx, dy, dx2, dy2)) {
                        if (line.isTopLayer() == bTopFrameOn)
                            line.setValid(false);
                    }
                }
            }
            if (textList != null) {
                for (VJText txt : textList) {
                    if (txt.intersects(dx, dy, dx2, dy2)) {
                        if (txt.isTopLayer() == bTopFrameOn)
                            txt.setValid(false);
                    }
                }
            }
        }

        protected void clearSeries(boolean bTopLayerOnly) {
            if (baseMap != null) {
                baseMap.clearSeries(bTopLayerOnly);
                return;
            }
            if (ybarList != null) {
                if (!bTopLayerOnly && ybarList.size() > 4)
                    ybarList.clear();
                else {
                    for (VJYbar bar : ybarList) {
                        if (bTopLayerOnly) {
                            if (bar.isTopLayer())
                                bar.setValid(false);
                        }
                        else
                            bar.setValid(false);
                    }
                }
            }
            if (lineList != null) {
                if (!bTopLayerOnly && lineList.size() > 6)
                    lineList.clear();
                else {
                    for (VJLine line : lineList) {
                        if (bTopLayerOnly) {
                            if (line.isTopLayer())
                                line.setValid(false);
                        }
                        else
                            line.setValid(false);
                    }
                }
            }
            if (textList != null) {
                if (!bTopLayerOnly && textList.size() > 14)
                    textList.clear();
                else {
                    for (VJText txt : textList) {
                        if (bTopLayerOnly) {
                            if (txt.isTopLayer())
                                txt.setValid(false);
                        }
                        else
                            txt.setValid(false);
                    }
                }
            }
        }

        public void drawSeries(Graphics2D g) {
            if (baseMap != null) {
                baseMap.drawSeries(g);
                return;
            }
            if (ybarList == null && lineList == null && textList == null)
                return;
            // if (x > 0 || y > 0)
            //    g.translate(x, y);
            g.setFont(font);
            int lw = 1;
            int i, nums;
            BasicStroke stroke;

            if (bSeriesChanged || bSeriesLocked)
                return;
          try {
            if (ybarList != null) {
                // for (VJYbar bar : ybarList) { // this access will cause race condition
                nums = ybarList.size();
                for (i = 0; i < nums; i++) {
                    if (bSeriesChanged)
                        break;
                    VJYbar bar = ybarList.get(i);
                    if (bar != null && bar.isValid()) {
                        bar.setConatinerGeom(x, y, width, height);
                        if (bar.getLineWidth() != lw) {
                            lw = bar.getLineWidth();
                            if (lw >= STROKE_NUM) {
                                stroke = new BasicStroke((float) lw,
                                   BasicStroke.CAP_SQUARE,BasicStroke.JOIN_ROUND);
                            }
                            else
                                stroke = strokeList[lw];
                            g.setStroke(stroke);
                           // g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                        }
                        if (bar.clipRect != null)
                            g.setClip(bar.clipRect);
                        bar.draw(g, bVertical, bRight);
                        if (bar.clipRect != null)
                            g.setClip(paintClip);
                    }
                }
            }
            if (lineList != null) {
                nums = lineList.size();
                for (i = 0; i < nums; i++) {
                    if (bSeriesChanged)
                        break;
                    VJLine line = lineList.get(i);
                    if (line != null && line.isValid()) {
                        line.setConatinerGeom(x, y, width, height);
                        if (line.getLineWidth() != lw) {
                            lw = line.getLineWidth();
                            if (lw >= STROKE_NUM) {
                                stroke = new BasicStroke((float) lw,
                                   BasicStroke.CAP_SQUARE,BasicStroke.JOIN_ROUND);
                            }
                            else
                                stroke = strokeList[lw];
                            g.setStroke(stroke);
                        }
                        if (line.clipRect != null)
                            g.setClip(line.clipRect);
                        line.draw(g, bVertical, bRight);
                        if (line.clipRect != null)
                            g.setClip(paintClip);
                    }
                }
            }

            if (textList != null) {
                // for (VJText txt : textList) {
                nums = textList.size();
                for (i = 0; i < nums; i++) {
                    if (bSeriesChanged)
                       break;
                    VJText txt = textList.get(i);
                    if (txt != null && txt.isValid()) {
                        txt.setConatinerGeom(x, y, width, height);
                        if (txt.clipRect != null)
                            g.setClip(txt.clipRect);
                        txt.draw(g, bVertical, bRight);
                        if (txt.clipRect != null)
                            g.setClip(paintClip);
                    }
                }
            }
          }
          catch (Exception ex) { }
            // if (x > 0 || y > 0)
            //    g.translate(-x, -y);
            if (lw != 1)
                g.setStroke(defaultStroke);
        }

        public void setAlpha(float n) {
            xalphaSet = n;
        }

        public void setAlphaComposite(float n, AlphaComposite c) {
            xalphaSet = n;
            alphaComp = c;
        }

        public float getAlpha() {
            return xalphaSet;
        }

        public void clearBackRegion() {
             if (backRegionList == null)
                 return;
             if (backRegionList.size() > 3)
                 backRegionList.clear();
             else {
                for (VJBackRegion r : backRegionList)
                    r.setValid(false);
             }
        }

        public void setBackRegion(int x, int y, int w, int h, int c, int alpha) {
             if (backRegionList == null)
                backRegionList = Collections.synchronizedList(new LinkedList<VJBackRegion>());
             VJBackRegion freeObj = null;
             try {
                for (VJBackRegion r : backRegionList) {
                   if (!r.isValid()) {
                      if (freeObj == null)
                          freeObj = r;
                   }
                   else {
                       if (r.equals(x, y, w, h)) {
                           r.setColor(DisplayOptions.getColor(c));
                           return;
                       }
                   }
                }
             }
             catch (Exception e) { return; }
             if (freeObj == null) {
                 freeObj = new VJBackRegion();
                 backRegionList.add(freeObj);
             }
             freeObj.setRegion(x, y, w, h);
             freeObj.setValid(true);
             freeObj.clipRect = clipRect;
             Color color = DisplayOptions.getColor(c);
             if (alpha < 98) {
                 int r = color.getRed();
                 int g = color.getGreen();
                 int b = color.getBlue();
                 float fv = (float) alpha;
                 if (fv < 0.0f)
                     fv = 0.0f;
                 int a = (int) (255.0f * fv / 100.0f);
                 color = new Color(r, g, b, a);
             }
             freeObj.setColor(color);
        }

        public void drawBackRegion(Graphics2D g) {
             if (backRegionList != null) {
                 for (VJBackRegion r : backRegionList) {
                     if (r.isValid()) {
                         if (r.clipRect != null)
                             g.setClip(r.clipRect);
                         r.draw(g, x, y);
                         if (r.clipRect != null)
                             g.setClip(paintClip);
                     }
                  }
             }
             if (bTopMapOpened) {
                  if (topMap != null) {
                      topMap.x = x;
                      topMap.y = y;
                      topMap.drawBackRegion(g);
                  }
              }
        }

        public java.util.List<VJLine> getLineList() {
            if (baseMap != null)
               return baseMap.getLineList();
            if (lineList == null)
               lineList = Collections.synchronizedList(new LinkedList<VJLine>());
            return lineList;
        }

        public java.util.List<VJText> getTextList() {
            if (baseMap != null)
                return baseMap.getTextList();
            if (textList == null)
               textList = Collections.synchronizedList(new LinkedList<VJText>());
            return textList;
        }

        private void openTopMap() {
            if (bTopMap)
                return;
            if (topMap == null) {
                topMap = new XMap(id, x, y, width, height);
                topMap.bTopMap = true;
                topMap.baseMap = this;
            }
            else {
                topMap.setMapSize(width, height);
            }
            topMap.x = x;
            topMap.y = y;
            topMap.x2 = x2;
            topMap.y2 = y2;
            topMap.xcmpModel = xcmpModel;
            topMap.mapAlpha = mapAlpha;
            topMap.xcmIndex = xcmIndex;
            topMap.cmpId = cmpId;
        }

        public XMap turnOnTopMap(int on) {
            if (on > 0) {
                bTopMapOpened = true;
                if (bTopMap) {
                    baseMap.bTopMapOpened = true;
                    return this;
                }
                openTopMap();
                topMap.bTopMapOpened = true;
                return topMap;
            }
            return baseMap;
        }

        public void clearTopframe() {
             if (bTopMap) {
                 clear(0, 0, rw, rh);
                 clearBackRegion();
                 return;
             }
             if (topMap != null)
                 topMap.clearTopframe();
        }

        public void draw(Graphics2D g) {
             if (bTopFrameOnTop) {
                 if (img != null)
                     g.drawImage(img, x, y, x + width, y + height,
                            0, 0, width, height, null);
                 if (!bTopMap)
                     drawSeries(g);
                 if (bTopMapOpened) {
                     if (topMap != null) {
                         if (topMap.img != null)
                             g.drawImage(topMap.img, x, y, x + width, y + height,
                                  0, 0, width, height, null);
                     }
                 }
             }
             else {
                 if (bTopMapOpened) {
                     if (topMap != null) {
                         if (topMap.img != null)
                             g.drawImage(topMap.img, x, y, x + width, y + height,
                                  0, 0, width, height, null);
                     }
                 }
                 if (img != null)
                     g.drawImage(img, x, y, x + width, y + height,
                            0, 0, width, height, null);
                 if (!bTopMap)
                     drawSeries(g);
             }
        }

    }

    private Graphics2D wingc = null;
    private Graphics2D mygc = null;
    private Graphics2D frameWinGc = null;
    private Graphics2D bgc = null;
    // private Graphics tgc;
    // private Graphics tgc2;
    private Graphics2D bimgc = null;
    private Graphics2D b2imgc = null;
    private Graphics2D anngc = null;
    private Graphics2D printgc = null;
    private Graphics2D orgPrintgc = null;
    private Graphics2D prtframegc = null;
    private Graphics2D vtxtgc = null;
    private int winId = 0;
    private int colorSize = 512;
    private int dpsColor0 = 256 - 20;

    // private int difx = 0;
    // private int dify = 0;
    private int transX = 0;
    private int transY = 0;
    private int fontWidth = 8;
    private int fontHeight = 12;
    private int fontAscent = 10;
    private int fontDescent = 2;
    private int org_fontWidth = 8;
    private int org_fontHeight = 12;
    private int vtxtImgHeight = 0;
    private int vtxtImgWidth = 0;
    private int bfontSize = 0;
    private Font bannerFont = null;
    private Font myFont = null;
    private Font org_font = null;
    private FontMetrics fontMetric;
    private Color graphColor = Color.red;
    private Color fontColor = Color.black;
    public byte[] redByte;
    public byte[] grnByte;
    public byte[] bluByte;
    public byte[] alphaByte;
    public byte alphaVal;
    private AffineTransform af90 = null;
    private AffineTransform afOrg = null;
    // private MouseAdapter gpMouse = null;
    // private MouseMotionAdapter gpMove = null;
    // private KeyAdapter gpKey = null;
    private Component gp;
    protected int mpx = 0; // x of pointer
    protected int mpy = 0; // y of pointer
    private float alphaSet = 1.0f;
    private int rgbAlpha = 255;
    private int cmAlpha = 255;
    private AlphaComposite transComposite; // transparent AlphaComposite

} // class VnmrCanvas

