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


public class PushpinPanel extends JLayeredPane implements PushpinIF, ActionListener, PropertyChangeListener 
{
    private VJButton pinBut;
    private VJButton closeBut;
    private JComponent inComp = null;
    private Component topComp = null;
    private JComponent upComp = null;
    private JComponent lowComp = null;
    private JComponent topParent = null;
    private JComponent imgComp = null;
    // private Component bottomComp = null;
    private PushpinControler controler = null;
    private PushpinPanel superComp = null;
    private PushpinPanel pinContainer = null;
    private PushpinTitle titleComp = null;
    private PushpinIF    lastComp = null;
    private JPanel resizeBar;
    private JSplitPane splitBar;
    private String title = null;
    private String name = null;
    private String lastName = null;
    private Icon   BpinInIcon, BpinOutIcon, BcloseIcon;    
    private Icon   pinInIcon = null;
    private Icon   pinOutIcon, closeIcon;    
    private boolean bShowPin = false;
    private boolean bClosed = false;
    private boolean bHide = false;
    private boolean bPopup = false;
    private boolean bPopupMaster = false;
    private boolean bShowTab = false;
    private boolean bTabOnTop = false;
    private boolean bAvailable = true;
    private boolean bRbVisible = false;
    private boolean bEntered = false;
    private boolean bCtrlVisible = false;
    private VnmrjIF vjIf = null;
    private PushpinTab  tab = null;
    private ArrayList<PushpinIF> tabCompList;
    private int divSize = 7;
    private int orientation = 9;
    private int locY = 0;
    private int mvX = 0;
    private int mvY;
    private int mvW = 0;
    private int mvH;
    private int difX;
    private int difY;
    private int maxX;
    private int maxY;
    private int minX;
    private int minY;
    private int startY;
    private int startX;
    private int timeCount;
    private Rectangle tabRec;
    private Rectangle thisRec;
    private float refY = 0.0f;
    private float refX = 0.0f;
    private float refH = 0.5f;
    private float refW = 0.1f;
    private Timer  timer = null;
    private PushpinIF timerListener = null;
    private Color activeColor;

    public PushpinPanel(JComponent p) {
        this.inComp = p;
        DisplayOptions.addChangeListener(this);
        BpinInIcon = Util.getImageIcon("pushinB.gif");
        BpinOutIcon = Util.getImageIcon("pushoutB.gif");
        BcloseIcon = Util.getImageIcon("closepanelB.gif");
        pinBut = new VJButton(BpinInIcon);
        closeBut = new VJButton(BcloseIcon);
        pinBut.setRolloverIcon(BpinOutIcon);
        pinBut.setToolTipText(Util.getLabel("sHide"));
        closeBut.setToolTipText(Util.getLabel("blClose"));
        resizeBar = new JPanel();
        splitBar = new JSplitPane();
        splitBar.setOneTouchExpandable(false);
        resizeBar.setVisible(false);
        splitBar.setVisible(false);
        resizeBar.setOpaque(false);
        pinBut.setVisible(false);
        closeBut.setVisible(false);
        activeColor = Util.getActiveBg();
        // activeColor = SystemColor.activeCaption;
        pinBut.setBackground(activeColor);
        closeBut.setBackground(activeColor);
        add(resizeBar, JLayeredPane.PALETTE_LAYER);
        add(splitBar, JLayeredPane.DEFAULT_LAYER);
        add(pinBut, JLayeredPane.MODAL_LAYER);
        add(closeBut, JLayeredPane.MODAL_LAYER);
        tabCompList = new ArrayList<PushpinIF>();
        if(p != null) {
            p.setOpaque(false);
            add(p, JLayeredPane.DEFAULT_LAYER);
        }
        setLayout(new pushpinLayout());

        resizeBar.addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent ev) {
                startResize(ev);
                ev.consume();
            }

            public void mouseReleased(MouseEvent e) {
                e.consume();
                doResize(e);
            }

            public void mouseEntered(MouseEvent evt) {
            }

            public void mouseExited(MouseEvent evt) {
            }
        });

        resizeBar.addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseMoved(MouseEvent e) {
            }

            public void mouseDragged(MouseEvent e) {
                e.consume();
                dragResize(e);
            }
        });

        PinListener l = new PinListener();
        pinBut.setActionCommand("hide");
        pinBut.addActionListener(l);
        closeBut.setActionCommand("close");
        closeBut.addActionListener(l);
    }

    public void propertyChange(PropertyChangeEvent evt) {
        setBgColor();           
    }
    public PushpinPanel() {
         this(null);
    }

    public JComponent getPinObj() {
         return inComp;
    }

    public void setBgColor() {
        Color color = Util.getBgColor();
        setBackground(color);
        if(inComp!=null)
            inComp.setBackground(color);
        if(topComp!=null)
            topComp.setBackground(color);
        if(upComp!=null)
            upComp.setBackground(color);
        if(lowComp!=null)
            lowComp.setBackground(color);
        color = Util.getSeparatorBg();
        resizeBar.setBackground(color);
        activeColor = Util.getActiveBg();
    }

    /*  add main component  */
    public void setPinObj(JComponent p) {
         if (inComp != null)
              remove(inComp);
         inComp = p;
         if (p != null) {
              p.setOpaque(false);
              add(p, JLayeredPane.DEFAULT_LAYER);
         }
    }

    public void setOrientation(int dir) {
        orientation = dir;
        bRbVisible = true;
        splitBar.setOrientation(dir);
        if (dir == JSplitPane.VERTICAL_SPLIT) {
            resizeBar.setCursor(Cursor.getPredefinedCursor(Cursor.N_RESIZE_CURSOR));
        }
        else if (dir == JSplitPane.HORIZONTAL_SPLIT) {
            resizeBar.setCursor(Cursor.getPredefinedCursor(Cursor.E_RESIZE_CURSOR));
            // resizeBar.setOrientation(SwingConstants.VERTICAL);
        }
        else {
            bRbVisible = false;
        }
        resizeBar.setVisible(bRbVisible);
        splitBar.setVisible(bRbVisible);
    }

    public void setName(String s) {
        name = s;
        if (controler == null) {
            name = s;
            return;
        }
        if (s!=null && !s.equals(name)) {
            name = s;
            controler.removeTab(this);
            tab = null;
        }
        if (tab == null && name != null) {
            controler.addTab(this);
        }
    }

    public String getName() {
        return name;
    }

    public String getLastName() {
        if (lastComp != null)
            return lastComp.getName();
        return lastName;
    }

    public void setLastName(String s) {
        lastName = s;
        if (lastComp != null)
            return;
        for (int k = 0; k < tabCompList.size(); k++) {
            PushpinIF comp = (PushpinIF) tabCompList.get(k);
            if (comp != null && s.equals(comp.getName())) {
               lastComp = comp;
               if (!comp.isOpen())
                   comp.pinOpen(false);
               break;
            }
         }
    }

    private int getOpenCounts() {
        int  openCount = 0;
        for (int k = 0; k < tabCompList.size(); k++) {
            PushpinIF comp = (PushpinIF) tabCompList.get(k);
            if (comp != null) {
                if (comp.isOpen())
                   openCount++;
            }
        }
        return openCount;
    }


    public void setTabOnTop(boolean s) {
         bTabOnTop = s;
    }

    public boolean isTabOnTop() {
         return bTabOnTop;
    }

    public void controlVisible(boolean b) {
         bCtrlVisible = b;
    }

    public void removeTab() {
        if (controler != null) {
            if (tab != null)
                controler.removeTab(this);
        }
    }


    public void showTab(boolean b) {
         bShowTab = b;
         if (b && tab == null && name != null) {
             if (controler != null)
               controler.addTab(this);
         }
    }

    public void setAvailable(boolean s) {
        bAvailable = s;
        if (tab != null) {
            if (bHide && bAvailable)
               tab.setVisible(true);
            else
               tab.setVisible(false);
        }
    }

    public boolean isAvailable() {
         return bAvailable;
    }

    public void setVisible(boolean b) {
         if (!b)
            doResize(null); // erase drag line 
         super.setVisible(b);
    }

    public void setStatus(String s) {
        if (s == null)
           return;
        bClosed = false;
        bPopup = false;
        bHide = false;
        if (s.equals("close"))
           bClosed = true;
        if (s.equals("hide"))
           bHide = true;
        if (bCtrlVisible) {
            if (!bClosed && !bHide)
               setVisible(true);
            else
               setVisible(false);
        }
        if (tab != null) {
           if (bHide && bAvailable)
              tab.setVisible(true);
           else
              tab.setVisible(false);
        }
    }

    public String getStatus() {
         if (bClosed)
             return "close";
         if (bHide)
             return "hide";
         return "open";
    }

    public int getStatusVal() {
         if (bHide)
             return 2;
         if (bClosed)
             return 3;
         return 1;
    }

    public void setStatus(int n) {
         if (n == 1)
             setStatus("open");
         if (n == 2)
             setStatus("hide");
         if (n == 3)
             setStatus("close");
    }

    public void hideAllTabs() {
         for (int k = 0; k < tabCompList.size(); k++) {
            PushpinIF comp = (PushpinIF) tabCompList.get(k);
            if (comp != null) {
                if (comp.getTab() != null)
                    comp.getTab().setVisible(false);
                if (comp instanceof PushpinPanel)
                    ((PushpinPanel) comp).hideAllTabs();
            }
         }
    }

    public void updatePinTabs() {
         if (tab != null) {
             if (bHide && bAvailable)
                tab.setVisible(true);
             else
                tab.setVisible(false);
             controler.revalidate();
             controler.repaint();
         }
    }

    public boolean isHide() {
         return bHide;
    }

    public boolean isOpen() {
        if (!bClosed && !bHide)
            return true;
        else
            return false;
    }

    public boolean isClose() {
        return bClosed;
    }

    public boolean isPopup() {
        return bPopup;
    }

    public void pinClose(boolean bAct) {
         if (!bAct) {
             bClosed = true;
             bHide = false;
             updatePinTabs();
             return;
         }
         if (bPopup) {
             if (!bPopupMaster && pinContainer != null) {
                  pinContainer.pinClose(true);
                  return;
             }
             pinShow(false);
         }
         bClosed = true;
         bHide = false;
         if (pinContainer != null) {
             bClosed = pinContainer.closeComp(this);
         }
         if (bClosed) {
             if (pinContainer == null)
                 setVisible(false);
         }
         updatePinTabs();
    }

    public void pinOpen(boolean bAct) {
         if (!bAct) {
             bClosed = false;
             bHide = false;
             return;
         }
         if (bPopup) {
             if (!bPopupMaster && pinContainer != null) {
                  pinContainer.pinOpen(true);
                  return;
             }
             pinShow(false);
         }
         bClosed = false;
         bHide = false;
         if (pinContainer == null) {
             setVisible(true);
             updatePinTabs();
             return;
         }
         else
             pinContainer.resetPushPinLayout();
    }

    public void pinOpen(PushpinIF c) { // from child component
         if (bPopup) {  
             pinShow(false);
         }
         bClosed = false;
         bHide = false;
         pinContainer.resetPushPinLayout();
    }

    public void pinHide(boolean bAct) {
         bClosed = false;
         bHide = true;
         if (bAct) {
             if (pinContainer != null) {
                 bHide = pinContainer.hideComp(this);
             }
             if (bHide) {
                 if (pinContainer == null)
                    setVisible(false);
             }
         }
         updatePinTabs();
    }

    public void pinShow(boolean on) {
         if (superComp == null) { // the highest level of pushpinpanel
            if (on) {
               if (!tab.isShowing()) // not called by tab button
                   return;
               if (isShowing())
                   return;
               tabRec = tab.getBounds();
               Point pt = tab.getLocationOnScreen();
               tabRec.x = pt.x;
               tabRec.y = pt.y;
               thisRec = null;
               setTimer(true, this);
            }
            else {
               setTimer(false, this);
               vjIf = Util.getVjIF();
               if (vjIf != null)
                  vjIf.raiseComp(this, on);
            }
            setPopup(on, true);
            return;
         }
          
         if (on) {
             superComp.popupTool(name);
             setTimer(true, this);
         }
         else {
             setTimer(false, this);
             superComp.popdnTool(name);
         }
    }

    public void pinPopup(boolean b) {
         bPopup = b;
    }

    public void setPopup(boolean s, boolean originator) {
         bPopup = s;
         bPopupMaster = originator;
         if (bShowPin) {
              AbstractButton b = (AbstractButton) pinBut;
              ButtonModel model = b.getModel();
              model.setArmed(false);
         }
         if (s) {
              if (originator) {
                 if (bShowPin) {
                    hilitPushPin(s);
                    pinBut.setIcon(pinOutIcon);
                    pinBut.setRolloverIcon(pinInIcon);
                    closeBut.setIcon(closeIcon);
                    pinBut.setToolTipText(Util.getLabel("blOpen"));
                    setPin(false);
                  }
              }
              if (resizeBar.isVisible()) {
                  resizeBar.setVisible(false);
                  splitBar.setVisible(false);
              }
         }
         else {
              if (originator) {
                  if (bShowPin) {
                     hilitPushPin(s);
                     pinBut.setIcon(BpinInIcon);
                     pinBut.setRolloverIcon(BpinOutIcon);
                     pinBut.setToolTipText(Util.getLabel("sHide"));
                     closeBut.setIcon(BcloseIcon);
                     setPin(true);
                  }
              }
              if (bRbVisible) {
                 resizeBar.setVisible(true);
                 splitBar.setVisible(true);
              }
         }
    }

    public void openComp(PushpinIF c) {
         if (c == null)
            return;
         for (int k = 0; k < tabCompList.size(); k++) {
            PushpinIF comp = (PushpinIF) tabCompList.get(k);
            if (comp == c) {
                c.pinOpen(true);
                return;
            }
         }
    }

    public boolean closeComp(PushpinIF c) {
         int  count = getOpenCounts();
         if (count >= 1)
             return true;
         bHide = true;
         lastComp = c;
         if (pinContainer != null) {
                pinContainer.closeComp(this); 
                pinContainer.recordCurrentLayout();
                pinContainer.resetPushPinLayout();
         }
         else {
                setVisible(false);
                updatePinTabs();
         }
         return true;
    }

    public boolean hideComp(PushpinIF c) {
         int  count = getOpenCounts();
         if (count >= 1)
            return true;
         bHide = true;
         lastComp = c;
         if (pinContainer != null) {
              pinContainer.hideComp(this);
              pinContainer.recordCurrentLayout();
              pinContainer.resetPushPinLayout();
         } 
         else {
              setVisible(false);
              updatePinTabs();
         }
         return true;
    }

    public void recordCurrentLayout() {
    }

    public void resetPushPinLayout() {
    }

    public void showPinObj(PushpinIF c, boolean on) {
    }

    public void addTabComp(PushpinIF c) {
         boolean bExist = false;
         for (int k = 0; k < tabCompList.size(); k++) {
             PushpinIF pin = (PushpinIF) tabCompList.get(k);
             if (c.equals(pin)) {
                 bExist = true;
                 break;
             }
         }
         if (!bExist)
             tabCompList.add(c);
         if (controler != null)
              c.setControler(controler);
         c.setSuperContainer(superComp);
    }

    public void clearPushpinComp() {
         for (int k = 0; k < tabCompList.size(); k++) {
             PushpinIF c = (PushpinIF) tabCompList.get(k);
             if (c != null) {
                c.setAvailable(false);
/*
                if (c instanceof PushpinPanel)
                   ((PushpinPanel)c).clearPushpinComp();
*/
             }
         }
    }

    public void removeAllPushpinComp() {
         if (controler != null) {
            removeTab();
            for (int k = 0; k < tabCompList.size(); k++) {
               PushpinIF c = (PushpinIF) tabCompList.get(k);
               if (c != null) {
                    c.removeTab();
                    if (c instanceof PushpinPanel)
                       ((PushpinPanel)c).removeAllPushpinComp();
               }
            }
            controler.revalidate();
         }
         tabCompList.clear();
    }

    public void setControler(PushpinControler c) {
         controler = c;
         for (int k = 0; k < tabCompList.size(); k++) {
            PushpinIF obj = (PushpinIF) tabCompList.get(k);
            if (obj != null)
                obj.setControler(controler);
         }
         if (tab == null && name != null) {
            if (bShowTab)
               c.addTab(this);
         }
    }

    public PushpinControler getControler() {
         return controler;
    }

    public void setSuperContainer(JComponent c) {
         if (c instanceof PushpinPanel) {
             superComp = (PushpinPanel) c;
             for (int k = 0; k < tabCompList.size(); k++) {
                PushpinIF obj = (PushpinIF) tabCompList.get(k);
                if (obj != null)
                    obj.setSuperContainer(c);
             }
         }
    }

    public void timerProc() {
         Point   mousePt;
         boolean inTab = false;
         boolean inObj = false;

         try {
             PointerInfo ptInfo = MouseInfo.getPointerInfo();
             mousePt = ptInfo.getLocation();
         }
         catch (HeadlessException e) {
             pinShow(false);
             return;
         }
         inTab = tabRec.contains(mousePt.x, mousePt.y);
         timeCount++;
         if (timeCount == 1) {
             if (!inTab) {
                 pinShow(false);
                 return;
             }
             if (!isShowing()) {
                 vjIf = Util.getVjIF();
                 if (vjIf != null)
                    vjIf.raiseComp(this, true);
             }
             return;
         }

         if (!bPopup) {
             if (timeCount > 4)
                 pinShow(false);
             return;
         }

         if (!inTab) {
             if (thisRec == null) {
                 if (!isShowing()) {
                     if (timeCount > 4)
                         pinShow(false);
                     return;
                 }
                 Point pt = getLocationOnScreen();
                 thisRec = getBounds();
                 thisRec.x = pt.x;
                 thisRec.y = pt.y;
             }
             inObj = thisRec.contains(mousePt.x, mousePt.y);
             if (!inObj) {
                 pinShow(false);
                 return;
             }
         }

         if (!bEntered && timeCount > 30) {
             timer.stop();
             pinShow(false);
             return;
         }
    }

    public void actionPerformed(ActionEvent e) {
       Object obj = e.getSource();
       if (obj instanceof Timer) {
            timerListener.timerProc();
       }
    }

    public void setTimer(boolean on, PushpinIF obj) {
       if (obj == null)
           return;
       if (!on) {
           if (obj.equals(timerListener)) {
               if (timer != null && (timer.isRunning()))
                   timer.stop();
               timerListener = null;
           }
           return;
       }
       if (timerListener != null) {
           if (!obj.equals(timerListener)) {
              if (timerListener.isPopup())
                   timerListener.pinShow(false);
           }
       }
       timerListener = obj;
       bEntered = false;
       timeCount = 0;
       if (timer == null) {
            timer = new Timer(800, this);
            timer.setInitialDelay(300);
       }
       if (timer != null && (!timer.isRunning()))
            timer.restart();
    }

    

    public void setContainer(JComponent c) {
         if (c instanceof PushpinPanel) {
             pinContainer = (PushpinPanel) c;
             if (superComp == null)
                 superComp = pinContainer;
         }
    }


    public JComponent getPinContainer() {
         return pinContainer;
    }

    public void setDividerLoc(int y) {
         locY = y;
    }

    public int getDividerLoc() {
         return locY;
    }

    public void setRefX(float x) {
         refX = x;
    }

    public float getRefX() {
         return refX;
    }

    public void setRefY(float x) {
         refY = x;
    }

    public float getRefY() {
         return refY;
    }

    public void setRefH(float x) {
         refH = x;
    }

    public float getRefH() {
         return refH;
    }

    public float getRefW() {
         return refW;
    }

    public int getOrientation() {
        return orientation;
    }

    public int getDividerOrientation() {
        return orientation;
    }

    public String getTitle() {
         return title;
    }

    public void setTitle(String s) {
         title = s;
         if (name == null)
             name = title;
         if (titleComp == null) {
             titleComp = new PushpinTitle(s);
             add(titleComp);
             titleComp.setVisible(false);
         }
         else
             titleComp.setTitle(s);
    }

    public void showTitle(boolean s) {
         if (titleComp != null)
             titleComp.setVisible(s);
    }

    public void setTopComponent(Component c) {
         topComp = c;
    }

    public void setRightComponent(Component c) {
         topComp = c;
    }

    public void setBottomComponent(Component c) {
         // bottomComp = c;
    }

    public void setLeftComponent(Component c) {
         // bottomComp = c;
    }

    public void showPushPin(boolean s) {
         pinBut.setVisible(s);
         closeBut.setVisible(s);
         bShowPin = s;
         if (s && pinInIcon == null) {
             pinInIcon = Util.getImageIcon("pushin.gif");
             pinOutIcon = Util.getImageIcon("pushout.gif");
             closeIcon = Util.getImageIcon("closepanel.gif");
        }
    }


    public void setPanelPin(boolean s) {
        boolean b = true;
        if (s) {
           b = false;
           pinBut.setToolTipText(Util.getLabel("blOpen"));
           pinBut.setIcon(pinOutIcon);
           pinBut.setRolloverIcon(pinInIcon);
           pinBut.setOpaque(s);
           closeBut.setOpaque(s);
           pinBut.setVisible(s);
           closeBut.setVisible(s);
        }
        else {
           pinBut.setToolTipText(Util.getLabel("blClose"));
           pinBut.setIcon(pinInIcon);
           pinBut.setRolloverIcon(pinOutIcon);
           pinBut.setOpaque(s);
           closeBut.setOpaque(s);
           pinBut.setVisible(bShowPin);
           closeBut.setVisible(bShowPin);
        }
        for (int k = 0; k < tabCompList.size(); k++) {
           PushpinIF c = (PushpinIF) tabCompList.get(k);
           if (c != null) {
                c.setPin(b);
            }
         }
    }

    public void setPin(boolean s) {
         for (int k = 0; k < tabCompList.size(); k++) {
           PushpinIF c = (PushpinIF) tabCompList.get(k);
           if (c != null) {
                c.setPin(s);
            }
         }
    }

    public void setPin(boolean s, PushpinIF obj) {
         for (int k = 0; k < tabCompList.size(); k++) {
           PushpinIF c = (PushpinIF) tabCompList.get(k);
           if (c != null && c != obj) {
                c.setPin(s);
            }
         }
    }

    public void hilitPushPin(boolean s) {
         pinBut.setBackground(activeColor);
         closeBut.setBackground(activeColor);
         pinBut.setOpaque(s);
         closeBut.setOpaque(s);
    }

    public void setUpperComp(JComponent c) {
         upComp = c;
    }

    public void setLowerComp(JComponent c) {
         lowComp = c;
    }

    public JComponent getUpperComp() {
         return upComp;
    }

    public JComponent getLowerComp() {
         return lowComp;
    }

    public boolean openTool(String tname)
    { return false; }

    public boolean popupTool(String tname)
    { return false; }

    public boolean popdnTool(String tname)
    { return false;  }

    public void setDividerSize(int newSize) {
         divSize = newSize;
    } 
    
    public void setTab(PushpinTab comp) {
        tab = comp;
        if (tab != null) {
           tab.refX = refX;
           tab.refY = refY;
           if (bHide && bAvailable && bShowTab)
              tab.setVisible(true);
           else
              tab.setVisible(false);
        }
    }

    public JComponent getTab() {
        return tab;
    }

    public void startResize(MouseEvent ev) {
        if (topComp == null)
           return;
        Container tp = (Container) topComp;
        while (!tp.isVisible()) {
           tp = tp.getParent();
           if (tp == null)
              break;
        }
        if (tp == null || (!tp.isShowing()))
           return;

        vjIf = Util.getVjIF();
        if (vjIf == null)
           return;
 
        topParent = (JComponent) vjIf;
        Point pt0 = topParent.getLocationOnScreen();
        Point pt1 = getLocationOnScreen();
        Point pt2 = tp.getLocationOnScreen();
        Dimension d = getSize();
        Dimension d2 = tp.getSize();
        startX = ev.getX();
        startY = ev.getY();
        Color c = UIManager.getColor("SplitPaneDivider.draggingColor");
        vjIf.setDividerMoving(true, this);
        vjIf.setDividerColor(c);
        if (orientation == JSplitPane.VERTICAL_SPLIT) {
            difY = pt1.y - pt0.y;
            difX = pt1.x - pt0.x;
            mvX = difX;
            mvY = difY;
            maxY = pt1.y - pt0.y + d.height - divSize;
            minY = pt2.y - pt0.y + divSize;
            mvW = d.width;
            mvH = divSize;
            maxX = d2.width;
            minX = divSize;
        }
        else {
            difY = pt1.y - pt0.y;
            difX = pt1.x - pt0.x + d.width - divSize;
            mvX = difX;
            mvY = difY;
            maxX = pt2.x - pt0.x + d2.width - divSize;
            minX = pt1.x - pt0.x + divSize;

            mvW = divSize;
            mvH = d.height;
        }
        topParent.paintImmediately(mvX, mvY, mvW, mvH);
    }

    public void doResize(MouseEvent ev) {
        if (topComp == null || vjIf == null)
           return;
        vjIf.setDividerMoving(false, null);
        if (ev == null)
           return;
        if (orientation == JSplitPane.VERTICAL_SPLIT)
           vjIf.setVerticalDividerLoc(mvY); 
        else 
           vjIf.setHorizontalDividerLoc(mvX + divSize); 
        if (mvW > 1)
           topParent.paintImmediately(mvX, mvY, mvW, mvH);
    }

    public void dragResize(MouseEvent ev) {
        if (topComp == null || topParent == null)
           return;

        int x = ev.getX();
        int y = ev.getY();
        int x0 = mvX;
        int y0 = mvY;
        int x2 = mvX + mvW;
        int y2 = mvY + mvH;

        if (orientation == JSplitPane.VERTICAL_SPLIT) {
            mvY = y + difY - startY;
            if (mvY < minY)
               mvY = minY;
            else if (mvY > maxY)
               mvY = maxY;
            if (mvY < y0)
               y0 = mvY;
            if ((mvY + mvH) > y2)
               y2 = mvY + mvH;
        }
        else {
            mvX = x + difX - startX;
            if (mvX < minX)
                mvX = minX;
            else if (mvX > maxX)
                mvX = maxX;
            if (mvX < x0)
               x0 = mvX;
            if ((mvX + mvW) > x2)
               x2 = mvX + mvW;
        }

        topParent.paintImmediately(x0, y0, x2 - x0, y2 - y0);
    }

    public Rectangle getDividerBounds() {
        return new Rectangle(mvX, mvY, mvW, mvH);
    }

    public void setBounds(int x, int y, int w, int h) {
         super.setBounds(x, y, w, h);
         if (bPopup) {
             return;
         }
         Container p = getParent();
         if (pinContainer != null)
             p = (Container) pinContainer;
         if (p == null)
             return;
 
         Dimension d = p.getSize();
         locY = y;
         refY = (float) y / (float) d.height;
         refX = (float) x / (float) d.width;
         refH = (float) h / (float) d.height;
         refW = (float) w / (float) d.width;
         if (tab != null) {
             tab.refX = refX;
             tab.refY = refY;
         }
    }

    public void openFromHide() {
         if (!bHide)
             return;
         bPopup = true;
         hideProc(); 
    }

    private void hideProc() {
         if (bPopup) {
             pinShow(false);
             if (superComp != null) {
                  superComp.openTool(name);
                  return;
             }
             pinOpen(true);
         }
         else {
             pinHide(true);
         }
    }

    private void closeProc() {
         if (bPopup) {
             pinShow(false);
         }
         pinClose(true);
    }

    public void addImagePanel(JComponent comp) {
         if (comp == null)
             return;
         imgComp = comp;
         add(imgComp, JLayeredPane.POPUP_LAYER);
         validate();
    }

    class pushpinLayout implements LayoutManager {

        public void addLayoutComponent(String cname, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // preferredLayoutSize()

        public Dimension minimumLayoutSize(Container target) {
            int h = 0;
            if (titleComp != null && titleComp.isVisible())
                h = titleComp.height;
            else if (bShowPin)
                h = 12;
            return new Dimension(0, h);
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension panSize = target.getSize();
                Insets insets = getInsets();
                Dimension  dim;
                int     x;
                int     y;
                int     h;
                int     x0 = insets.left;
                int     y0 = insets.top;
                int     y2 = panSize.height - insets.bottom;
                int     x2 = panSize.width - insets.right;
                int     cw = panSize.width - insets.left - insets.right;
                int     ch = panSize.height - insets.top - insets.bottom;
                y = 0;
                x = 0;
                h = 0;

                if (resizeBar.isVisible()) {
                   if (orientation == JSplitPane.HORIZONTAL_SPLIT) {
                      x2 -= divSize;
                      resizeBar.setBounds(x2, y0, divSize, ch);
                      splitBar.setBounds(x2, y0, divSize, ch);
                      cw -= divSize;
                   }
                   else {
                      resizeBar.setBounds(x0, y0, cw, divSize);
                      splitBar.setBounds(x0, y0, cw, divSize);
                      y0 = y0 + divSize;
                   }
                }

                if (orientation == JSplitPane.VERTICAL_SPLIT)
                    h = 2;
                dim = closeBut.getPreferredSize();
                   
                if (pinBut.isVisible()) {
                   h = dim.height + 2;
                }
                if (titleComp != null && titleComp.isVisible()) {
                    if (titleComp.height > h)
                       h = titleComp.height;
                    titleComp.setBounds(x0, y0, cw, h);
                    y0 += h;
                }

                if (pinBut.isVisible()) {
                    y = (h - dim.height) / 2;
                    x = x2 - dim.width - 4;
                    closeBut.setBounds(x, y, dim.width, dim.height);
                    dim = pinBut.getPreferredSize();
                    x = x - dim.width - 4;
                    pinBut.setBounds(x, y, dim.width, dim.height);
                }

                if (inComp != null) {
                    inComp.setBounds(x0, y0, cw, y2 - y0);
                    inComp.revalidate();
                }
                if (imgComp != null && imgComp.isVisible()) {
                    imgComp.setBounds(x0, y0, cw, y2 - y0);
                    imgComp.revalidate();
                }
            }
        }
    }

    private class PushpinTitle extends JPanel {
        private String label;
        public int width = 0;
        public int height = 0;
        private int descent = 0;

        public PushpinTitle( String s) {
             label = s;
             calSize();
             setBgColor();
        }

        public void setTitle(String s) {
             label = s;
             calSize();
        }

        private void calSize() {
            FontMetrics fm = getFontMetrics(getFont());
            height = fm.getAscent() + fm.getDescent() + 4;
            descent = fm.getDescent();
            width = fm.stringWidth(label) + 6;
        }

        public void paint(Graphics g) {
            Dimension dim = getSize();
            int x = (dim.width - width) / 2;  
            int y = dim.height - descent - 2; 
            g.drawString(label, x, y);
        }
    }

    private class PinListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            String cmd = e.getActionCommand();
            if (cmd.equals("hide")) {
                 hideProc();
                 return;
            }
            if (cmd.equals("close")) {
                 closeProc();
                 return;
            }
        }
   }
}
