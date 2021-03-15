/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.File;
import java.util.*;
import java.util.EventObject.*;
import java.beans.*;
import java.awt.*;
import java.lang.reflect.Constructor;
import javax.swing.JLayeredPane;
import javax.swing.border.Border;
import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSplitPane;
import javax.swing.JTextArea;
import javax.swing.plaf.basic.BasicSplitPaneDivider;
import javax.swing.plaf.basic.BasicSplitPaneUI;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import org.xml.sax.Attributes;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.AttributesImpl;
import org.xml.sax.helpers.DefaultHandler;

import vnmr.ui.shuf.*;
import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.ExpSelTree;


public class VToolPanel extends PushpinPanel implements ExpListenerIF, PropertyChangeListener {
    static private boolean browserCheckInUse=false;
    static private boolean browserStatus=false;
    static private boolean locatorStatus=false;
    static private boolean bExpTreeFirst = false;
    static private boolean bCreateExpParam = true;
    static public JComponent locator = null;  // Shuffler
    static public JComponent holding = null; // HoldingArea
    static public JComponent expCard = null; // ExpCards
    static public JComponent expSelector = null; // ExpSelector
    static public JComponent expSelTree = null; // ExpSelTree
    static public JComponent reviewQueue = null; // RQPanel
    static public JComponent studyQueue = null; // QueuePanel
    static public JComponent browser = null; // Browser
    static public JComponent vjBrowser = null; // VJBrowser

    private final static String expParamName = "expSelectorType";
    
    private final static String[] toolNames = {"Locator", "Holding", "Sq",
                                               "Rq", "XMLToolPanel", "ExperimentPanel", "ExperimentSelector", "ExperimentSelTree", "NotePad", "Instructions", 
        "Browser", "RobotView", "ParamPanel", "SQ" };
    private final static String[] toolTitles = {"Locator", "Holding  Panel", "Study Queue",
                                                "Review Queue", "Imaging Tools", "Experiment Panel", "Experiment Selector", "Experiment Selector Tree", "Note  Pad", "Instructions", 
        "Browser", "Robot View", "Paramameter Panel", "Study Queue" };


    ArrayList           spanes = new ArrayList();
    ArrayList           spNames = new ArrayList();
    ArrayList           objList = new ArrayList();
    Hashtable           panes = new Hashtable();
    Dimension           zeroDim = new Dimension(0,0);
    HashMap             vobjs = new HashMap();
    HoldingArea         holdingArea;
    QueuePanel          queueArea;
    SessionShare        sshare;
    Hashtable           hs = null;
    PushpinPanel        pinPanel;
    JPanel              toolPanel;
    Component           popupComp = null;
    RightsList          rights = null;
 
    int vpId = 0;
    int layoutId = 99;
    int maxViews = 9;
    int nviews = 1;
    int openComps = 0;
    Hashtable tp_paneInfo[] = new Hashtable[maxViews+1];
    public Hashtable tp_dividers = new Hashtable();
    boolean bSwitching = false;
    boolean RQinit = false;
    String tp_rqtype = "";
    String buildFile = null; // the source of xml file
    long   dateOfbuildFile = 0;


    ArrayList keys = new ArrayList();
    ArrayList vpInfo = new ArrayList();
   
    public VToolPanel(SessionShare sshare) {
        // super( new BorderLayout() );
        this.sshare = sshare;
        this.pinPanel = this;
        this.toolPanel = new JPanel();
        setPinObj(this.toolPanel);
        this.toolPanel.setLayout(new BorderLayout());
        setTitle("Tool Panel");
        setName("LocatorTool");

        hs = sshare.userInfo();
        if (hs != null) {
            Object obj = hs.get("canvasnum");
            if(obj != null) {
                Dimension dim = (Dimension)obj;
                nviews = (dim.height)*(dim.width);
            }
        }

        for(int i=0; i<nviews; i++) {
            tp_paneInfo[i] = new Hashtable();
        }

/*
        obj = sshare.userInfo().get("activeWin");
        if(obj != null) {
            vpId = ((Integer)obj).intValue();
        } else vpId = 0;
*/

        fillHashMap();

        holdingArea = new HoldingArea(sshare);
        Util.setHoldingArea(holdingArea);

    }

    private void fillHashMap() {
        addTool("Locator",              vnmr.ui.shuf.Shuffler.class);
        addTool("Holding",              vnmr.ui.HoldingArea.class);
        addTool("Sq",                   vnmr.ui.QueuePanel.class);
        addTool("Rq",                   vnmr.ui.RQPanel.class);
        addTool("XMLToolPanel",         vnmr.ui.XMLToolPanel.class);
        addTool("ExperimentPanel",      vnmr.ui.ExpSelector.class);
        addTool("ExperimentSelector",   vnmr.ui.ExpSelector.class);
        addTool("ExperimentSelTree",    vnmr.ui.ExpSelTree.class);
//        addTool("RobotView",          vnmr.ui.RobotViewComp.class);
        addTool("NotePad",              vnmr.ui.NoteEntryComp.class);
        addTool("Instructions",         vnmr.ui.PublisherNotesComp.class);
//        addTool("ParamPanel",         vnmr.ui.ParamComp.class);
        addTool("Browser",              vnmr.ui.shuf.VJFileBrowser.class);
        addTool("VJBrowser",            vnmr.ui.shuf.VJFileBrowser.class);
    }

    private void addTool(String key, Class tool) {
        Class[] constructArgs;
        if(key.equals("XMLToolPanel") || key.equals("Sq")) {
            constructArgs = new  Class[3];
            constructArgs[0] = vnmr.ui.SessionShare.class;
            constructArgs[1] = String.class;
            constructArgs[2] = String.class;
        } else {
            constructArgs = new  Class[1];
            constructArgs[0] = vnmr.ui.SessionShare.class;
        }
        try {
            Constructor c = tool.getConstructor(constructArgs);
            vobjs.put(key,c);
        }
        catch (NoSuchMethodException nse) {
            Messages.postError("Problem initiating " + key + " area.");
            Messages.writeStackTrace(nse, tool +  " not found.");
        }
        catch (SecurityException se) {
            Messages.postError("Problem initiating " + key + " area.");
            Messages.writeStackTrace(se);
        }
    }
       
    private Constructor getTool(String key) {
        return ( (Constructor) vobjs.get(key) );
    }

    private JComponent getToolComp(String key) {
        if (key.equals("Locator")) {
            if (locator == null)
               locator = (JComponent) Util.getShuffler();
            return locator;
        }
        if (key.equals("Holding"))
            return holding;
        if (key.equals("Rq"))
            return reviewQueue;
        if (key.equals("Sq") || key.equals("SQ"))
            return studyQueue;
        if (key.equals("Browser"))
            return browser;
        if (key.equals("ExperimentSelector"))
            return expSelector;
        if (key.equals("ExperimentSelTree"))
            return expSelTree;
        if (key.equals("VJBrowser"))
            return vjBrowser;
        return null;
    }

    private void setToolComp(String key, JComponent comp) {
        if (comp instanceof Shuffler) {
            locator = comp;
            return;
        }
        if (comp instanceof HoldingArea) {
            holding = comp;
            return;
        }
        if (comp instanceof QueuePanel) {
            studyQueue = comp;
            return;
        }
        if (comp instanceof RQPanel) {
            reviewQueue = comp;
            return;
        }
        if (comp instanceof ExpCards) {
            expCard = comp;
            return;
        }
        if (comp instanceof ExpSelector) {
            expSelector = comp;
            return;
        }
        if (comp instanceof ExpSelTree) {
            expSelTree = comp;
            return;
        }
        if (key.equals("Browser")) {
            browser = comp;
            return;
        }
        if (key.equals("VJBrowser")) {
            vjBrowser = comp;
            return;
        }
    }

    protected class WBasicSplitPaneUI extends BasicSplitPaneUI {
    /**
     * Creates the default divider.
     */
        public BasicSplitPaneDivider createDefaultDivider() {
            return new WBasicSplitPaneDivider(this);
        }
    }

    protected class WBasicSplitPaneDivider extends BasicSplitPaneDivider {
        public WBasicSplitPaneDivider(WBasicSplitPaneUI ui) {
            super(ui);
        }
    /**
     * Creates and return an instance of JButton that can be used to
     * collapse the left component in the split pane.
     */
        protected JButton createLeftOneTouchButton() {
           JButton b = new JButton() {
              public void setBorder(Border b) { }

              public void paint(Graphics g) {
                if (splitPane != null) {
                    int[]   xs = new int[3];
                    int[]   ys = new int[3];
                    int     blockSize;

                    // Fill the background first ...
                    g.setColor(toolPanel.getBackground());
                    g.fillRect(0, 0, toolPanel.getWidth(),
                               toolPanel.getHeight());

                    // ... then draw the arrow.
                    g.setColor(Color.blue);
                    if (orientation == JSplitPane.VERTICAL_SPLIT) {
                        blockSize = Math.min(getHeight(), ONE_TOUCH_SIZE);
                        xs[0] = blockSize;
                        xs[1] = 0;
                        xs[2] = blockSize << 1;
                        ys[0] = 0;
                        ys[1] = ys[2] = blockSize;
                        g.drawPolygon(xs, ys, 3); // Little trick to make the
                                                  // arrows of equal size
                    }
                    else {
                        blockSize = Math.min(getWidth(), ONE_TOUCH_SIZE);
                        xs[0] = xs[2] = blockSize;
                        xs[1] = 0;
                        ys[0] = 0;
                        ys[1] = blockSize;
                        ys[2] = blockSize << 1;
                    }
                    g.fillPolygon(xs, ys, 3);
                }
              } // paint
              // Don't want the button to participate in focus traversable.
              public boolean isFocusable() {
                return false;
              }
           };
           b.setMinimumSize(new Dimension(ONE_TOUCH_SIZE, ONE_TOUCH_SIZE));
           b.setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
           b.setFocusPainted(false);
           b.setBorderPainted(false);
           b.setRequestFocusEnabled(false);
           return b;
        } // createLeftOneTouchButton

        /**
        * Creates and return an instance of JButton that can be used to
        * collapse the right component in the split pane.
        */
        protected JButton createRightOneTouchButton() {
           JButton b = new JButton() {
               public void setBorder(Border border) {
               }
               public void paint(Graphics g) {
                  if (splitPane != null) {
                    int[]          xs = new int[3];
                    int[]          ys = new int[3];
                    int            blockSize;

                    // Fill the background first ...
                    g.setColor(toolPanel.getBackground());
                    g.fillRect(0, 0, toolPanel.getWidth(),
                               toolPanel.getHeight());

                    // ... then draw the arrow.
                    if (orientation == JSplitPane.VERTICAL_SPLIT) {
                        blockSize = Math.min(getHeight(), ONE_TOUCH_SIZE);
                        xs[0] = blockSize;
                        xs[1] = blockSize << 1;
                        xs[2] = 0;
                        ys[0] = blockSize;
                        ys[1] = ys[2] = 0;
                    }
                    else {
                        blockSize = Math.min(getWidth(), ONE_TOUCH_SIZE);
                        xs[0] = xs[2] = 0;
                        xs[1] = blockSize;
                        ys[0] = 0;
                        ys[1] = blockSize;
                        ys[2] = blockSize << 1;
                    }
                    g.setColor(Color.blue);
                    g.fillPolygon(xs, ys, 3);
                  }
               } // paint
               // Don't want the button to participate in focus traversable.
               public boolean isFocusable() {
                  return false;
               }
           };
           b.setMinimumSize(new Dimension(ONE_TOUCH_SIZE, ONE_TOUCH_SIZE));
           b.setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
           b.setFocusPainted(false);
           b.setBorderPainted(false);
           b.setRequestFocusEnabled(false);
           return b;
        }  // createRightOneTouchButton
    }


    public void recordCurrentLayout() {
        tp_dividers.clear();
        for (int j=0; j<spanes.size(); j++) {
            JSplitPane jsp = (JSplitPane) spanes.get(j);
            int sv = jsp.getDividerLocation();
            tp_dividers.put("pane"+j, new Integer(sv));
            JComponent obj = (JComponent) jsp.getBottomComponent();
            if (obj instanceof PushpinIF) {
               ((PushpinIF) obj).setDividerLoc(sv);
            }
        }
    }

    public void resetDivider() {
        if (spanes == null)
            return;
        Integer hashValue;
        int splitSize;
        int h, h2;
        JSplitPane jsp;
        boolean bPopup;
        boolean bTopPopup;
        Dimension dim = toolPanel.getSize();
        int num = spanes.size();
        if (num < 1)
            return;
        if (dim.height < 10) {
            Integer ix;
            ix = (Integer) hs.get("frameHeight");
            if (ix != null) {
                dim.height = ix.intValue() - 200;
            }
            else {
                dim.height =  Toolkit.getDefaultToolkit().getScreenSize().height;
                dim.height -= 300;
            }
        }
        h = (dim.height - num * 6) / (num +1);
        if (h < 6)
            h = 6;
        for (int i=0; i< num; i++) {
            jsp = (JSplitPane)spanes.get(i);
            Component obj = jsp.getBottomComponent();
            Component topObj = jsp.getTopComponent();
            if (obj != null && (obj instanceof JSplitPane))
                obj = ((JSplitPane)obj).getTopComponent();
            bPopup = false;
            bTopPopup = false;
            splitSize = h;
            if (topObj != null && (topObj instanceof PushpinIF))
               bTopPopup = ((PushpinIF)topObj).isPopup();
            if (obj != null) {
               if (obj instanceof PushpinIF) {
                  splitSize = ((PushpinIF)obj).getDividerLoc();
                  bPopup = ((PushpinIF)obj).isPopup();
               }
               else {
                  hashValue = null;
                  if (tp_dividers != null)
                     hashValue = (Integer) tp_dividers.get("pane"+i);
                  if (hashValue == null)
                     splitSize = h;
                  else
                     splitSize = (int) hashValue.intValue();
               }
               if (splitSize < 10) {
                  h2 = h;
                  if (topObj != null && (topObj instanceof PushpinObj)) {
                     if (((PushpinObj)topObj).lowLocY > h)
                        h2 = ((PushpinObj)topObj).lowLocY;
                     if (splitSize < h2)
                        splitSize = h2;
                  }
                  else if (!bPopup)
                      splitSize = h;
               }
            }
            jsp.setDividerLocation(splitSize);
        }
    }

    public void copyCurrentLayout(VpLayoutInfo info) {
        info.tp_dividers.clear();

        for(Enumeration e=tp_dividers.keys(); e.hasMoreElements();) {
           String key = (String)e.nextElement();
           int value = ((Integer)tp_dividers.get(key)).intValue();
           info.tp_dividers.put(key, new Integer(value));
        }
    }

    public boolean compareCurrentLayout(VpLayoutInfo info) {
        boolean b = false;

        String key;
        Object obj;
        int value1;
        int value2;
        for (int j=0; j<spanes.size(); j++) {
           key = "pane"+j;

           obj = tp_dividers.get(key);
           if(obj != null) value1 = ((Integer)obj).intValue();
           else value1 = 0;

           obj = info.tp_dividers.get(key);
           if(obj != null) value2 = ((Integer)obj).intValue();
           else value2 = 0;

           if(value1 != value2) {
                tp_dividers.remove(key);
                tp_dividers.put(key, new Integer(value2));
                b = true;
           } 
        }

        return b;
    }

    public boolean comparePanelLayout(int currId, int newId) {
        if(currId >= nviews || newId >= nviews) return false;

        if(tp_paneInfo[currId].size() != tp_paneInfo[newId].size()) return true;

        String key;
        Object value1;
        Object value2;
        for(Enumeration e=tp_paneInfo[currId].keys(); e.hasMoreElements();) {
           key = (String)e.nextElement();
           value1 = tp_paneInfo[currId].get(key);
           value2 = tp_paneInfo[newId].get(key);
           if(!value1.equals(value2)) return true;
        }

        return false;
    }

    public void putHsLayout(int id) {
        if (hs == null || id >= maxViews)
            return;
        int   j;
        String key, name;
        JComponent  obj;
        PushpinIF   pobj;

        recordCurrentLayout();
        for (j=0; j<keys.size(); j++) {
            key = (String)keys.get(j);
            obj = (JComponent)panes.get(key);
            if (obj instanceof PushpinIF) {
               pobj = (PushpinIF) obj;
               name = "tool."+id+"."+pobj.getName()+".";
               hs.put(name+"dividerLoc", new Integer(pobj.getDividerLoc()));
               hs.put(name+"refY", new Float(pobj.getRefY()));
               hs.put(name+"refH", new Float(pobj.getRefH()));
               key = pobj.getStatus();
               if (key != null)
                  hs.put(name+"status", key);
            } 
        }
    }

    public void getHsLayout(int id) {
        if (hs == null || id >= maxViews)
            return;
        String  key, name, status;
        Float   fstr;
        Integer istr;
        PushpinIF expTreeObj = null;
        PushpinIF expObj = null;

        for (int j=0; j<keys.size(); j++) {
            key = (String)keys.get(j);
            JComponent  obj = (JComponent)panes.get(key);
            if (obj instanceof PushpinIF) {
               PushpinIF pobj = (PushpinIF) obj;
               String objName = pobj.getName();
               name = "tool."+id+"."+objName+".";
               istr = (Integer) hs.get(name+"dividerLoc");
               if (istr != null) {
                   pobj.setDividerLoc(istr.intValue());
               }
               fstr = (Float) hs.get(name+"refY");
               if (fstr != null) {
                   pobj.setRefY(fstr.floatValue());
               }
               fstr = (Float) hs.get(name+"refH");
               if (fstr != null) {
                   pobj.setRefH(fstr.floatValue());
               }
               status = (String) hs.get(name+"status");
               if (objName.equals("ExperimentSelTree"))
                   expTreeObj = pobj;
               if (objName.equals("ExperimentSelector"))
                   expObj = pobj;
               if (status != null) {
                  pobj.setStatus(status);
               }
            }
        }
        if (expTreeObj != null && expObj != null) {
            if (expTreeObj.isOpen() && expObj.isOpen()) {
                if (bExpTreeFirst)
                    expObj.setStatus("close");
                else
                    expTreeObj.setStatus("close");
            }
        }
    }

    public void getSplitLayout(int id) {
        if (hs == null || id >= maxViews)
            return;
        String  key, name, status;
        Integer istr;
        for (int j=0; j<keys.size(); j++) {
            key = (String)keys.get(j);
            JComponent  obj = (JComponent)panes.get(key);
            if (obj instanceof PushpinIF) {
               PushpinIF pobj = (PushpinIF) obj;
               name = "tool."+id+"."+pobj.getName()+".";
               istr = (Integer) hs.get(name+"dividerLoc");
               if (istr != null) {
                   pobj.setDividerLoc(istr.intValue());
               }
            }
        }
    }


    public void switchLayout(int newId, boolean bLayout) {
         if(layoutId == newId) return;
         if(newId >= nviews)
             updateVpInfo(newId+1);

         if (bSwitching) {
              return;
         }

         bSwitching = true;
         int oldId = layoutId;
         vpId = newId;
         layoutId = newId;
         // recordCurrentLayout();

         VpLayoutInfo vInfo = Util.getViewArea().getLayoutInfo(oldId);
         if (vInfo != null) {  // save current layout info
              copyCurrentLayout(vInfo);
         }

         vInfo = Util.getViewArea().getLayoutInfo(newId);

         putHsLayout(oldId);
         if (bLayout)
            getHsLayout(vpId);
         else
            getSplitLayout(vpId);
         setCurrentLayout();
/*
        if(comparePanelLayout(oldId, newId)) {
              setCurrentLayout();
        }
        if ((vInfo != null) && vInfo.bAvailable &&
              compareCurrentLayout(vInfo)) {
              setCurrentLayout(vInfo);
        }
*/
        updatePinTabs();
        validate();
        // repaint();

        bSwitching = false;
    }

    public void switchLayout(int newId) {
        switchLayout(newId, true);
    }

    public void setCurrentLayout(VpLayoutInfo info) {

        resetDivider();
/*
        if(spanes == null || tp_dividers == null) return;
        Integer hashValue;
        int splitSize;
        for (int i=0; i<spanes.size(); i++) {
            hashValue = (Integer) tp_dividers.get("pane"+i);
            if (hashValue == null)
                splitSize = 300;
	    else
                splitSize = (int) hashValue.intValue();
            ((JSplitPane)spanes.get(i)).setDividerLocation(splitSize);
        }
*/
    }

    private void clearPanel() {
        for (int i=0; i<spanes.size(); i++) {
            ((JSplitPane)spanes.get(i)).removeAll();
        }
	spanes.clear();
	toolPanel.removeAll();
    }

    public void setCurrentLayout(int newId) {
        int i, j;
        int  count, lastJspIndex;

        if(newId >= nviews || keys.size() == 0) return;

        clearPanel();
        clearPushpinComp(); /* invalidate  all pushpin objs */

        JComponent lastComp=null;
        JComponent newComp;
        PushpinIF  pinObj;
	JSplitPane  jsp;

        String key;
        String currValue;
        count = 0;
        openComps = 0;
        i = 0;
        pinObj = null;
	for(j=0; j<keys.size(); j++) {
	   key = (String)keys.get(j);
           currValue = (String)tp_paneInfo[newId].get(key);
           if(!currValue.equals("no")) {
		newComp = (JComponent)panes.get(key);
                if (newComp != null && (newComp instanceof PushpinIF)) {
                    if (pinObj != null)
                       pinObj.setLowerComp(newComp);
                    ((PushpinIF)newComp).setUpperComp((JComponent)pinObj);
                    pinObj = (PushpinIF)newComp;
                    pinObj.setLowerComp(null);
                    pinObj.setAvailable(true);
                    if (pinObj.isOpen())
                       openComps++;
                    else if (!pinObj.isPopup()) {
                             newComp = null;
                    }
                }
                else if (newComp != null)
                    openComps++;
                if (newComp != null) {
                    count++;
                    lastComp = newComp;
                }
	   }
        }
        if (count == 0)
           pinPanel.setStatus("hide");
        else
           pinPanel.setStatus("open");
        if (count == 1) {
           toolPanel.add(lastComp);
           toolPanel.revalidate();
        }
        if (count <= 1)
           return;

        lastComp = null;
        count = 0;
	for (j=0; j<keys.size(); j++) {
	   key = (String)keys.get(j);
           currValue = (String)tp_paneInfo[vpId].get(key);
	   newComp = (JComponent)panes.get(key);
           if (newComp != null && (newComp instanceof PushpinIF)) {
               pinObj = (PushpinIF)newComp;
               if (!pinObj.isOpen()) {
                   if (!pinObj.isPopup())
                         newComp = null;
               }
           }
           if(!currValue.equals("no") && newComp != null) {
               if (count > 0) { 
	          jsp = new JSplitPane(JSplitPane.VERTICAL_SPLIT,false);
                  jsp.setOneTouchExpandable(false);
                  // jsp.setUI(new WBasicSplitPaneUI());
                  // jsp.setDividerSize(6);
                  // jsp.setBorder(null);
                  jsp.setTopComponent(lastComp);
		  jsp.addPropertyChangeListener(JSplitPane.DIVIDER_LOCATION_PROPERTY, this);
                  spanes.add(jsp);
                  lastJspIndex = spanes.size() - 1;
                  if (lastJspIndex == 0)
                     toolPanel.add(jsp);
                  else
                    ((JSplitPane)spanes.get(lastJspIndex-1)).setBottomComponent(jsp);
               }
               count++;
	       lastComp = newComp;
           }
        }
        if(lastComp != null) {
            lastJspIndex = spanes.size() - 1;
            ((JSplitPane)spanes.get(lastJspIndex)).setBottomComponent(lastComp);
	}
        resetDivider();
        updatePinTabs();
        // toolPanel.revalidate();
    }

    public void setCurrentLayout() {
        if(vpId >= nviews || keys.size() == 0) return;

        setCurrentLayout(vpId);
    }

    public int getOpenCount() {
        return openComps;
    }

    public void initPanel() {
        if(Util.getRQPanel() != null) {
           Messages.postDebug("buildRQPanel");
           Util.getRQPanel().buildRQPanel(tp_rqtype);
        }
    }

    public void setViewPort(int id) {
        if(id >= nviews || id < 0) return;
        Object obj = tp_paneInfo[id].get("Rq");
        vpId = id;
        if(obj != null && ((String)obj).equals("yes") && 
            Util.getRQPanel() != null) {
                Util.getRQPanel().setViewPort(id);
        }
    }

    public void updateValue(Vector v) {
        if(vpId >= nviews) return;

        Object obj = tp_paneInfo[vpId].get("Rq");
        if(obj != null && ((String)obj).equals("yes") && 
            Util.getRQPanel() != null) {

             if(!RQinit) {
                Messages.postDebug("initRQ"); 
                RQinit = true;
                ReviewQueue rq = Util.getRQPanel().getReviewQueue();
                rq.doInit();
             }

             Util.getRQPanel().updateValue(v);
        }
	// update SQ
	QueuePanel queuePanel = Util.getStudyQueue();
	if(queuePanel != null) queuePanel.updateValue(v);
    }

    private void sendExpSelectorInfo() {
        String s;

        if (bCreateExpParam) {
            bCreateExpParam = false;
            s = "create('"+expParamName+"','string','global','')\n";
            Util.sendToVnmr(s);
        }

        boolean bNotSet = true;
        PushpinIF obj;
        JComponent comp = searchTool("ExperimentSelTree");
        s = expParamName + "='";
        if (comp != null && (comp instanceof PushpinIF)) {
            obj = (PushpinIF) comp;
            if (obj.isOpen()) {
                s = s + "tree'";
                bNotSet = false;
            }
        }
        if (bNotSet) {
            comp = searchTool("ExperimentSelector");
            if (comp != null && (comp instanceof PushpinIF)) {
                obj = (PushpinIF) comp;
                if (obj.isOpen()) {
                    s = s + "default'";
                    bNotSet = false;
                }
            }
        }
        if (bNotSet)
            s = s + "off'";
        Util.sendToVnmr(s);
    }

    public void updateValue() {

        if(vpId >= nviews) return;
        Object obj = tp_paneInfo[vpId].get("Rq");
        if(obj != null && ((String)obj).equals("yes") && 
            Util.getRQPanel() != null) {

             if(!RQinit) {
                Messages.postDebug("initRQ"); 
                RQinit = true;
                ReviewQueue rq = Util.getRQPanel().getReviewQueue();
                rq.doInit();
             }

            Util.getRQPanel().updateValue();
        }
	// update SQ
	QueuePanel queuePanel = Util.getStudyQueue();
	if(queuePanel != null) queuePanel.updateValue();
        if (bCreateExpParam)
            sendExpSelectorInfo();
    }

/***
    public void setCurrentLayout(int newId) {
        if(newId >= nviews || vpId >= nviews) return;

        String key;
        String newValue;
        String currValue;
        for(Enumeration e=tp_paneInfo[newId].keys(); e.hasMoreElements();) {
           key = (String)e.nextElement();
           newValue = (String)tp_paneInfo[newId].get(key);
           currValue = (String)tp_paneInfo[vpId].get(key);
           JComponent comp = (JComponent) panes.get(key);
           // if(newValue.equals("yes") && currValue.equals("no")) {
           // else if(newValue.equals("no") && currValue.equals("yes")) {
           if (comp != null) {
              if(newValue.equals("no"))
                  comp.setVisible(false);
              else
                  comp.setVisible(true);
           }
        }
        setCurrentLayout();
        resetDivider();
        updatePinTabs();
    }
***/
   

    public void initUiLayout() {
        getHsLayout(vpId);
        setCurrentLayout();
        if (hs == null)
            return;
/*
        int splitSize;
        Integer hashValue;
        if (hs == null)
            return;
        for (int i=0; i<spanes.size(); i++) {
            hashValue = (Integer) hs.get("pane"+i);
            if (hashValue == null)
                splitSize = 300;
            else
                splitSize = (int) hashValue.intValue();
            ((JSplitPane)spanes.get(i)).setDividerLocation(splitSize);
        }
*/

        Object obj = hs.get("rqtype");
        if(obj!=null) tp_rqtype = (String)obj;
    }

    public void resetPushPinLayout() {
        // putHsLayout(vpId);
        setCurrentLayout();
        toolPanel.revalidate();
    }

    private void switchExpSelector(PushpinIF newObj, String name) {
        JComponent comp = searchTool(name);
        if (comp == null || !(comp instanceof PushpinIF))
            return;
        PushpinIF oldObj = (PushpinIF) comp;
        if (oldObj.isOpen()) {
            int splitSize = oldObj.getDividerLoc();
            newObj.setDividerLoc(splitSize);
        }
        oldObj.setStatus("close");
        sendExpSelectorInfo();
    }

    public JComponent searchTool(String name, int id) {
        if (name == null)
            return null;
        if(id >= nviews || keys.size() <= 0)
            return null;
	for (int j=0; j<keys.size(); j++) {
	   String key = (String)keys.get(j);
           if (name.equals(key)) {
	      JComponent comp = (JComponent)panes.get(key);
              String Value = (String)tp_paneInfo[id].get(key);
              if(!Value.equals("no") && comp != null) {
                   return comp;
              }
           }
        }
        return null;
    }

    public JComponent searchTool(String name) {
        return searchTool(name, vpId);
    }

    public boolean openTool(String name, boolean bOpen, boolean bShowOnly) {
        JComponent comp = searchTool(name);
        if (comp == null)
           return false;
        if (comp instanceof PushpinIF) {
           PushpinIF pobj = (PushpinIF) comp;
           if (bShowOnly) {
               if (!pobj.isClose())
                   return false;
           }
           if (pobj.isOpen()) {
               if (comp.isShowing())
                  return false;  // no change
               else
                  return true;
           }
           if (!bOpen) {
               if (!pobj.isClose())
                  return false;
           }
           pobj.setStatus("open");
           if (name.equals("ExperimentSelector"))
               switchExpSelector(pobj, "ExperimentSelTree");
           else if (name.equals("ExperimentSelTree"))
               switchExpSelector(pobj, "ExperimentSelector");
        }
        setCurrentLayout();
        pinPanel.setVisible(true);
        return true;
    }

    public boolean openTool(String name, boolean bOpen) {
        return openTool(name, bOpen, false);
    }

    public boolean closeTool(String name) {
        JComponent comp = searchTool(name);
        if (comp == null)
           return false;
        if (comp instanceof PushpinIF) {
           PushpinIF pobj = (PushpinIF) comp;
           if (pobj.isClose())
               return false;
           pobj.setStatus("close");
        }
        setCurrentLayout();

        return true;
    }

    public boolean hideTool(String name) {
        JComponent comp = searchTool(name);
        if (comp == null)
           return false;
        if (comp instanceof PushpinIF) {
           PushpinIF pobj = (PushpinIF) comp;
           pobj.setStatus("hide");
        }
        setCurrentLayout();

        return true;
    }

    public boolean popupTool(String name) {
        JComponent comp = searchTool(name);
        if (comp == null)
           return false;
        if (comp.isShowing())
           return true;
        PushpinIF pobj = (PushpinIF) comp;
        pobj.pinPopup(true);
        setCurrentLayout();
        return true;
    }

    public boolean popdnTool(String name) {
        JComponent comp = searchTool(name);
        if (comp == null)
           return false;
        PushpinIF pobj = (PushpinIF) comp;
        pobj.pinPopup(false);
        setCurrentLayout();
        return true;
    }

    public void saveuiLayout(String f) {
    }
 
    public void saveUiLayout() {
        putHsLayout(vpId);

        if (hs == null)
            return;
/*
        for (int i=0; i<spanes.size(); i++) {
            int splitValue = ((JSplitPane)spanes.get(i)).getDividerLocation();
            hs.put("pane"+i, new Integer(splitValue));
        }
*/
        RQPanel rq = Util.getRQPanel();
        if(rq != null) {
           tp_rqtype = rq.getRQtype(); 
           hs.put("rqtype", tp_rqtype);
        }
    }

    public void openUiLayout(String f) {
    }

    public void clearAll() {
        clearPanel();
        panes.clear();
        keys.clear();
        vpInfo.clear();
        objList.clear();
        removeAllPushpinComp();
        for(int i=0; i < nviews; i++) 
            tp_paneInfo[i].clear();
        if (expCard != null) {
            ((ExpCards) expCard).finalize();
            expCard = null;
        }
    }

    public void fill() {
        String newPath = FileUtil.openPath("INTERFACE/ToolPanel.xml");
        File fd = null;
        boolean bSameFile = true;
        if (newPath != null)
            fd = new File(newPath);
        if (fd != null && fd.exists()) {
            bSameFile = false;
            if (buildFile != null && buildFile.equals(newPath)) {
                if (fd.lastModified() == dateOfbuildFile)
                     bSameFile = true;
            }
        }
        else
            fd = null;
/*
        if (bSameFile) {
            JComponent obj;
            for (int i=0; i < objList.size(); i++) {
                obj = (JComponent)objList.get(i);
                if (obj != null) {
                    if (obj instanceof Shuffler)
                         Util.shufflerInToolPanel(true);
                    else {
                       if (obj instanceof StatusListenerIF)
                           ExpPanel.addStatusListener((StatusListenerIF) obj);
                       else if (obj instanceof ExpListenerIF)
                           ExpPanel.addExpListener((ExpListenerIF) obj);
                    }
                }
            }
            return;
        }
*/

        buildFile = newPath;
        if (fd != null)
            dateOfbuildFile = fd.lastModified();

        clearAll();
        if (rights == null) {
            VnmrjIF vif = Util.getVjIF();
            if (vif != null)
                rights = vif.getRightsList();
        }
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            spf.setValidating(false);  // set to true if we get DOCTYPE
            spf.setNamespaceAware(false); // set to true with referencing
            SAXParser parser = spf.newSAXParser();
            if(fd == null) {
                /* get locator only */
                String key = "Locator";
                Constructor c = (Constructor)getTool(key);
                Object[] vargs = new Object[1];
                vargs[0] = sshare;
                if (c != null) {
                   JComponent comp = (JComponent)c.newInstance(vargs);
                   PushpinObj pObj = new PushpinObj(comp, pinPanel);
                   objList.add(comp);
                   pObj.setTitle(key);
                   pObj.setName(key);
                   pObj.showPushPin(true);
                   pObj.showTitle(true);
                   addTabComp(pObj);
                   // panes.put(key,comp);
                   panes.put(key, pObj);
                   keys.add(key);
                   vpInfo.add("all");
                   Util.shufflerInToolPanel(true);
                   for(int i=0; i<nviews; i++) tp_paneInfo[i].put(key,"yes");
                }
            } else {
                parser.parse( fd, new MySaxHandler() );
            }
        } catch (ParserConfigurationException pce) {
            System.out.println("The underlying parser does not support the " +
                               "requested feature(s).");
        } catch (FactoryConfigurationError fce) {
            System.out.println("Error occurred obtaining SAX Parser Factory.");
        } catch (Exception e) {
                e.printStackTrace();
        }
    }


    public class MySaxHandler extends DefaultHandler {

        JComponent  curComp = null;
        boolean  bExpSelector = false;

        public void endDocument() {
            // System.out.println("End of Document");
            updateVpInfo();
        }

        public void endElement(String uri, String localName, String qName) {
            // System.out.println("End of Element '"+qName+"'");
        }
        public void error(SAXParseException spe) {
            System.out.println("Error at line "+spe.getLineNumber()+
                                   ", column "+spe.getColumnNumber());
        }
        public void fatalError(SAXParseException spe) {
            System.out.println("Fatal error at line "+spe.getLineNumber()+
                                         ", column "+spe.getColumnNumber());
        }
        public void startDocument() {
            // System.out.println("Start of Document");
        }
        public void startElement(String uri,   String localName,
                                 String qName, Attributes attr) {
            if ( ! qName.equals("tool")) return;

            String lastName = attr.getValue("name");
            if (rights != null) {
               if (lastName.equals("Locator")) {
                  if (rights.approveIsFalse("locatorok")) {
                      return;
                  }
               }
               else if (lastName.equals("Browser")) {
                  if (rights.approveIsFalse("broswerok")) {
                      return;
                  }
               }
               else if (lastName.equals("Sq") || lastName.equals("SQ")) {
/*
                  if (rights.approveIsFalse("sqok")) {
                      return;
                  }
*/
               }
            }

            curComp = getToolComp(lastName);
            if (curComp == null) {
               Constructor c = (Constructor)getTool(lastName);
               Object[] vargs;
               if(lastName.equals("XMLToolPanel")) {
                   lastName = attr.getValue("label");
                   vargs = new Object[3];
                   vargs[0] = sshare;
                   vargs[1] = lastName;
                   vargs[2] = attr.getValue("file");
               } else if(lastName.equals("Sq")) {
                   String idStr = attr.getValue("id");
                   if(idStr == null || idStr.length() <= 0) idStr = "SQ";
                   vargs = new Object[3];
                   vargs[0] = sshare;
                   vargs[1] = idStr;
                   vargs[2] = attr.getValue("macro");
               } else {
                   vargs = new Object[1];
                   vargs[0] = sshare;
               }
               if (c != null) {
                   try {
                       if (DebugOutput.isSetFor("vtoolpanel")) {
                           Messages.postDebug("VToolPanel: Creating " + lastName);
                       }
                       curComp = (JComponent)c.newInstance(vargs);
                   }
                   catch (Exception e) {
                       //System.out.println(e.toString());
                       curComp = null;
                   }
               }
            }
            else {
               if (curComp instanceof StatusListenerIF)
                    ExpPanel.addStatusListener((StatusListenerIF) curComp);
               else if (curComp instanceof ExpListenerIF)
                    ExpPanel.addExpListener((ExpListenerIF) curComp);
            }

            if (curComp == null)
                curComp = new JLabel(lastName);
            else {
                setToolComp(lastName, curComp);
                objList.add(curComp);

                if (!bExpSelector) {
                   if (lastName.equals("ExperimentSelTree")) {
                       bExpSelector = true;
                       bExpTreeFirst = true;
                   }
                   else if (lastName.equals("ExperimentSelector")) {
                       bExpSelector = true;
                       bExpTreeFirst = false;
                   }
               }
            }
            if (curComp instanceof Shuffler)
                Util.shufflerInToolPanel(true);

            String vps = attr.getValue("viewport");

            int  index = -1;
            PushpinObj pObj = new PushpinObj(curComp, pinPanel);
            for (int n = 0; n < toolNames.length; n++) {
                if (lastName.equals(toolNames[n])) {
                    index = n;
                    break;
                }
            }

            if (index >= 0) {
		String name = Util.getLabelString(toolTitles[index]);
                pObj.setTitle(name);
	    } else { 
		String name = Util.getLabelString(lastName);
                pObj.setTitle(name);
	    }

            pObj.setName(lastName);
            pObj.showPushPin(true);
            pObj.showTitle(true);
            addTabComp(pObj);
            panes.put(lastName, pObj);
            keys.add(lastName);
            vpInfo.add(vps);
        }

        public void warning(SAXParseException spe) {
            System.out.println("Warning at line "+spe.getLineNumber()+
                                     ", column "+spe.getColumnNumber());
        }

    }

    public void updateVpInfo(int n) {
        
        if(n == nviews) return;

        n = Math.min(n,maxViews);

        if(n > nviews) 
        for(int i=nviews; i<n; i++)
            tp_paneInfo[i] = new Hashtable();

        nviews = n;

        updateVpInfo();
    }

    private void updateVpInfo() {
        String key;
        String vps;
        for(int j=0; j<keys.size(); j++) {
            key = (String)keys.get(j);
            vps = (String)vpInfo.get(j);
            if(vps == null || vps.length() <= 0 || vps.equals("all")) {
                for(int i=0; i<nviews; i++) {
                    tp_paneInfo[i].put(key,"yes");
                }
            } else {
              for(int i=0; i<nviews; i++) tp_paneInfo[i].put(key,"no");
              StringTokenizer tok = new StringTokenizer(vps, " ,\n");
              while(tok.hasMoreTokens()) {
                int vp = Integer.valueOf(tok.nextToken()).intValue();
                vp--;
                if(vp >= 0 && vp < nviews) {
                    tp_paneInfo[vp].remove(key);
                    tp_paneInfo[vp].put(key,"yes");
                }
              }
            }
        }
    }


    /******************************************************************
     * Summary: Return true if the requested pane is being used and
     *  has a non zero size.
     *
     *  I am looking at both the size of jsp and the divider location.
     *  Under some cases, only one of these will be zero.  I have had
     *  to monitor both so that if either is zero, this pane is not
     *  actually showing when it is the topComponent.  When it is the
     *  bottomComponent, I have to compare the divider location with the max.
     *****************************************************************/
    public boolean isPaneOpen(String paneClassName) {
        String key;
        JSplitPane jsp;

        // Go through the spanes looking for one containing the paneClassName
        
        for(int i=0; i < spanes.size(); i++) {
            jsp =  (JSplitPane)spanes.get(i);
            Component tc = jsp.getTopComponent();
            Component bc = jsp.getBottomComponent();

            if (tc != null) {
               if (tc instanceof PushpinIF)
                  tc = (Component) ((PushpinIF) tc).getPinObj();
            }
            if (bc != null) {
               if (bc instanceof PushpinIF)
                  bc = (Component) ((PushpinIF) bc).getPinObj();
            }

            // Get the size of the splitpane
            Dimension dim = jsp.getSize();
            double heightOfjsp = dim.getHeight();
            int divLoc = jsp.getDividerLocation();

            if(tc != null && tc.getClass().getName().endsWith(paneClassName)) {
                // If the size is non zero, return true
                if(heightOfjsp > 1.0  && divLoc > 20) {
                    if(DebugOutput.isSetFor("isPaneOpen"))
                        Messages.postDebug("isPaneOpen for " + 
                                           paneClassName + " = true");
                    return true;
                }
                else {
                    if(DebugOutput.isSetFor("isPaneOpen"))
                        Messages.postDebug("isPaneOpen for " + 
                                           paneClassName + " = false");
                    return false;
                }
            }
            if(bc != null && bc.getClass().getName().endsWith(paneClassName)) {
                int maxLoc = jsp.getMaximumDividerLocation();

                // If the size is non zero, and divider location is not
                // as max return true.  Use max Location because this is the
                // bottom compontent of the jsp and is divLoc == maxLoc,
                // it means that the split pane is pulled all of the way down 
                // giving no space to the bottom component.
                if(heightOfjsp > 1.0  && divLoc < maxLoc) {
                    if(DebugOutput.isSetFor("isPaneOpen"))
                        Messages.postDebug("isPaneOpen for " + 
                                           paneClassName + " = true"); 
                    return true;
                }
                else {
                    if(DebugOutput.isSetFor("isPaneOpen"))
                        Messages.postDebug("isPaneOpen for " + 
                                           paneClassName + " = false"); 
                    return false;
                }
            }

        }

        if(DebugOutput.isSetFor("isPaneOpen"))
            Messages.postDebug("isPaneOpen did not find " + 
                                           paneClassName + " therefore = false"); 

        return false;
    }

    // We need to have the locator update itself if the browser is
    // opened or closed.  Else, the locator will not stay up to date.
    // Unfortunately, when we get called here, all of the changes are
    // not yet made, so that a call to isPaneOpen() from here
    // will give the result from before the current change happens.
    // I cannot seem to find any way to get called after the change
    // nor to use the evt info to know the real result.
    // So, I will put in a call to another thread which will
    // sleep for a few msec and then check isPaneOpen() and check
    // to see if a change in browser status has occured.
    public void propertyChange(PropertyChangeEvent evt) {

        CheckBrowserStatus checkBrowserStatus;
        checkBrowserStatus = new CheckBrowserStatus();
        checkBrowserStatus.setPriority(Thread.MIN_PRIORITY);
        checkBrowserStatus.setName("Check Browser Status");
        checkBrowserStatus.start();
    }

    // We need to have the locator update itself if the browser is
    // opened or closed.  Else, the locator will not stay up to date.
    // Unfortunately, when we get called in propertyChange(), all of the 
    // changes are not yet made, so that a call to isPaneOpen() from there
    // will give the result from before the current change happens.
    // I cannot seem to find any way to get called after the change
    // nor to use the evt info in propertyChange() to know the real result.
    // So, in propertyChange(), I put in a call to this thread which will
    // sleep for a few msec and then check isPaneOpen() and check
    // to see if a change in browser status has occured and update
    // the locator if necessary.
    class CheckBrowserStatus extends Thread {

        public void run() {

            // Only allow one instance of this to run at a time.
            if(browserCheckInUse)
                return;
            
            // Do not allow multiple instances
            browserCheckInUse = true;

            try {
                // Wait a bit for the split pane resize to be completed
                sleep(2);
            
                boolean newStatus = isPaneOpen("LocatorBrowser");
                if(newStatus != browserStatus) {
                    // There was a change in status, update the locator if it
                    // is open
                    boolean locOpen = isPaneOpen("Shuffler");
                    if(locOpen) {
                        // Cause the locator to update
                        SessionShare sshare = ResultTable.getSshare();
                        StatementHistory history = sshare.statementHistory();
                        history.updateWithoutNewHistory();
                    }

                    browserStatus = newStatus;
                }
            }
            catch (Exception e) {}

            try {
                // Now we also need to check on the locator
                // If the locator is closed and then the browser directory
                // is changed, and the locator is reopened, the locator will
                // need to update to the new browser directory.
                boolean newStatus = isPaneOpen("Shuffler");
                if(!FillDBManager.locatorOff() && newStatus != locatorStatus) {
                    // There was a change in status, update the locator if it
                    // is open
                    if(newStatus) {
                        // Cause the locator to update
                        SessionShare sshare = ResultTable.getSshare();
                        StatementHistory history = sshare.statementHistory();
                        history.updateWithoutNewHistory();
                    }

                    locatorStatus = newStatus;
                }
            }
            catch (Exception e) {}

            // Clear the flag which stops multiple calls at one time
            browserCheckInUse = false;
        }
    }
}


