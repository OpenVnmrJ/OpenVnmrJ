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
import javax.swing.*;

import vnmr.util.*;


public class PushpinObj extends JPanel implements PushpinIF
{
    public VJButton pinBut;
    public VJButton closeBut;
    public JComponent inComp = null;
    public JComponent upComp = null;
    public JComponent lowComp = null;
    public PushpinControler controler = null;
    public JComponent  pComp = null;
    public PushpinPanel superPanel = null;
    public PushpinPanel pinContainer = null;
    public PushpinTitle titleComp = null;
    public String title = null;
    public String name = null;
    public boolean bShowPin = true;
    public boolean bClosed = false;
    public boolean bHide = false;
    public boolean bPopup = false;
    public boolean bShowTab = true;
    public boolean bTabOnTop = false;
    public boolean bPopupMaster = false;
    public boolean bAvailable = true;
    public PushpinTab  tab = null;
    private Icon   pinInIcon, pinOutIcon, closeIcon;    
    public int locY = 0;
    public int locX = 0;
    public int orgLocY = 0;
    public int lowLocY = -1;
    public int tmpLowLocY = -1;
    public float refY = 0.0f;
    public float refX = 0.0f;
    public float refH = 0.5f;
    public int ctrlOrient = SwingConstants.LEFT;
    public static Color tColor = new Color(80, 80, 250);
    public static Color tColor2 = new Color(160, 230, 250);
    private int pinX = 0;
    private int timeCount;
    private int walkAllowed = 0;
    private int walkCount = 0;
    private Rectangle tabRec;
    private Rectangle thisRec;

    public PushpinObj(JComponent p, PushpinPanel c) {
         this.inComp = p;
         this.pComp = c;
         this.superPanel = c;
         this.pinContainer = c;
         pinInIcon = Util.getImageIcon("pushin.gif");
         pinOutIcon = Util.getImageIcon("pushout.gif");
         closeIcon = Util.getImageIcon("closepanel.gif");
         pinBut = new VJButton(pinInIcon);
         closeBut = new VJButton(closeIcon);
         pinBut.setRolloverIcon(pinOutIcon);
         pinBut.setToolTipText(Util.getLabel("sHide"));
         closeBut.setToolTipText(Util.getLabel("blClose"));
         add(pinBut);
         add(closeBut);
              setOpaque(false);
         if (p != null) {
    //          p.setOpaque(false);
              add(p);
         }
         setLayout(new pushpinLayout());
         PushpinListener l = new PushpinListener();
         pinBut.setActionCommand("hide");
         pinBut.addActionListener(l);
         closeBut.setActionCommand("close");
         closeBut.addActionListener(l);
    }


    public PushpinObj(JComponent p) {
         this(p, null);
    }


    public void setTabOnTop(boolean s) {
         bTabOnTop = s;
    }

    public boolean isTabOnTop() {
         return bTabOnTop;
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

    public void removeTab() {
        if (controler != null) {
            if (tab != null)
                controler.removeTab(this);
        }
    }

    public void showTab(boolean b) {
         bShowTab = b;
    }

    public boolean isAvailable() {
         return bAvailable;
    }

    public void setPinStatus(boolean bOpen) {
        if (bOpen) {
           pinBut.setRolloverIcon(pinInIcon);
           pinBut.setIcon(pinOutIcon);
           pinBut.setToolTipText(Util.getLabel("blOpen"));
        }
        else {
           pinBut.setRolloverIcon(pinOutIcon);
           pinBut.setIcon(pinInIcon);
           pinBut.setToolTipText(Util.getLabel("sHide"));
        }
        pinBut.revalidate();
    }

    public void setPinFocused(boolean b) {
         ButtonModel model = ((AbstractButton) pinBut).getModel();
         model.setPressed(b);
         model.setRollover(b);
         model.setArmed(b);
    }

    public void setStatus(String s) {
        if (s == null)
           return;
        bClosed = false;
        bHide = false;
        bPopup = false;
        if (s.equals("close"))
           bClosed = true;
        if (s.equals("hide"))
           bHide = true;
        if (bHide) {
           if (bAvailable && tab != null)
              tab.setVisible(true);
           setPinStatus(true);
        }
        else {
           if (tab != null)
              tab.setVisible(false);
           setPinStatus(false);
        }
    }

    public String getStatus() {
         if (bClosed)
             return "close";
         if (bHide)
             return "hide";
         return "open";
    }

    public JComponent getPinObj() {
         return inComp;
    }

    public void setName(String s) {
         name = s;
         if (titleComp != null)
             titleComp.setName(s);
    }

    public String getName() {
         return name;
    }

    public String getTitle() {
         return title;
    }

    public void setTitle(String s) {
         title = s;
         if (titleComp == null) {
             titleComp = new PushpinTitle(s);
             add(titleComp);
         }
         else
             titleComp.setTitle(s);
    }

    public void showTitle(boolean s) {
         if (titleComp != null)
             titleComp.setVisible(s);
    }

    public void showPushPin(boolean s) {
         if (s != bShowPin) {
             pinBut.setVisible(s);
             closeBut.setVisible(s);
         }
         bShowPin = s;
    }

    public void setPin(boolean s) {
        if (s) {
           pinBut.setVisible(bShowPin);
           closeBut.setVisible(bShowPin);
        }
        else {
           pinBut.setVisible(s);
           closeBut.setVisible(s);
        }
    }

    public void setTab(PushpinTab comp) {
        tab = comp;
        if (tab != null) {
            tab.refX = refX;
            tab.refY = refY;
            if (bHide && bAvailable)
               tab.setVisible(true);
            else
               tab.setVisible(false);
        }
    }

    private void setTimer(boolean bOn) {
       if (superPanel == null)
          return;
       if (bOn) {
          timeCount = 0;
          walkCount = 0;
       }
       superPanel.setTimer(bOn, this);
    }

    public JComponent getTab() {
        return tab;
    }

    public boolean isHide() {
        return bHide;
    }

    public boolean isOpen() {
        if (bClosed || bHide)
            return false;
        else
            return true;
    }

    public boolean isClose() {
        return bClosed;
    }
    
    public boolean isPopup() {
        return bPopup;
    }
    
    private PushpinIF getLowObj() {
        if (lowComp == null)
            return null;
        PushpinIF comp = (PushpinIF) lowComp;
        while (comp != null) {
            if (comp.isOpen())
                break;
            comp = (PushpinIF) comp.getLowerComp();
        }
        if (comp != null && comp.isOpen())
            return comp;
        return null;
    }

    private void saveLowLoc() {
        PushpinIF comp = getLowObj();
        if (comp != null)
            lowLocY = comp.getDividerLoc();
    }

    private void saveSetLoc() {
        PushpinIF comp = getLowObj();
        if (comp == null)
           return;
        int loc2 =  comp.getDividerLoc();
        if (locY > 0 && loc2 < locY)
           locY = loc2; 
        tmpLowLocY = loc2;
    }

    private void setLowLoc() {
        PushpinIF comp = getLowObj();
        if (comp == null || lowLocY < 0)
            return;
        comp.setDividerLoc(lowLocY);
    }

    private void resetLowLoc() {
        PushpinIF comp = getLowObj();
        if (comp == null || tmpLowLocY < 0)
           return;
        comp.setDividerLoc(tmpLowLocY);
    }

    public void pinClose(boolean bAct) {
         bHide = false;
         bClosed = true;
         if (tab != null)
              tab.setVisible(false);
         if (!bAct || pinContainer == null) {
              return;
         }
         pinContainer.closeComp(this);
         pinContainer.recordCurrentLayout();
         saveLowLoc();
         pinContainer.resetPushPinLayout();
    }

    public void pinOpen(boolean bAct) {
         bClosed = false;
         bHide = false;
         setPinStatus(false);
         if (tab != null)
              tab.setVisible(false);
         if (pinContainer == null)
              return;
         if (bAct) {
             setLowLoc();
             pinContainer.resetPushPinLayout();
         }
    }

    public void pinHide(boolean bAct) {
         bClosed = false;
         bHide = true;
         setPinStatus(true);
         if (tab != null)
              tab.setVisible(true);
         if (!bAct || pinContainer == null) {
              return;
         }
         pinContainer.hideComp(this);
         pinContainer.recordCurrentLayout();
         saveLowLoc();
         pinContainer.resetPushPinLayout();
    }

    /* pinShow will be called by tab button */
    public void pinShow(boolean on) {
         if (on) {
             saveSetLoc();
             if (!tab.isShowing()) // not called by tab button
                 return;
             tabRec = tab.getBounds();
             Point pt = tab.getLocationOnScreen();
             tabRec.x = pt.x;
             tabRec.y = pt.y;
             thisRec = null;
             setTimer(true);
/*
             if (superPanel.popupTool(name))
                setTimer(true);
*/
         }
         else {
             setTimer(false);
             resetLowLoc();
             superPanel.popdnTool(name);
         }
    }

    public void openFromHide() {
         if (!bHide)
             return;
         setLowLoc();
         superPanel.openTool(name);
    }

    public void pinHide() {
         if (bPopup) {
             pinShow(false);
             // setStatus("open");
             setLowLoc();
             superPanel.openTool(name);
         }
         else {
             saveLowLoc();
             setStatus("hide");
             superPanel.popdnTool(name);
         }
    }

    public void pinClose() {
         setStatus("close");
         if (bPopup) {
             pinShow(false);
         }
         else
             saveLowLoc();
         superPanel.popdnTool(name);
    }

    public void pinPopup(boolean b) {
         if (b && isOpen()) {
             bPopup = false;
             return;
         }
         bPopup = b;
         setPinFocused(false);
         if (b)
              setPinStatus(true);
         else if (!bHide)
              setPinStatus(false);
         pinBut.hideBorder();
    }

    public void setPopup(boolean s, boolean originator) {
         bPopup = s;
         bPopupMaster = originator;
         setPinFocused(false);
         if (s)
              setPinStatus(true);
         else if (!bHide)
              setPinStatus(false);
         if (originator) {
              if (pinContainer != null) {
                  if (s)
                     pinContainer.setPin(false, this);
                  else
                     pinContainer.setPin(true, this);
              }
              setTimer(s);
         }
    }

    public void setControler(PushpinControler c) {
         controler = c;
         if (c != null) {
             c.addTab(this);
             ctrlOrient = c.getOrientation();
         }
    }

    public PushpinControler getControler() {
         return controler;
    }

    public void setContainer(JComponent c) {
         pComp = c;
         if (c instanceof PushpinPanel)
             pinContainer = (PushpinPanel) c;
    }

    public void setSuperContainer(JComponent c) {
         if (c != null && (c instanceof PushpinPanel))
             superPanel = (PushpinPanel) c;
    }

    public JComponent getPinContainer() {
         return pComp;
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

    public int getDividerOrientation() {
         return JSplitPane.VERTICAL_SPLIT;
    }

    public void setDividerLoc(int x) {
         locY = x;
    }

    public int getDividerLoc() {
         return locY;
    }

    public void setRefX(float x) {
         refX = x;
    }

    public void setRefY(float x) {
         refY = x;
    }

    public float getRefX() {
         return refX;
    }

    public float getRefY() {
         float f = refY;
         if (bPopup) {
             PushpinIF comp = (PushpinIF) lowComp;
             while (comp != null) {
                 if (comp.isOpen())
                     break;
                  comp = (PushpinIF) comp.getLowerComp();
             }
             if (comp != null)
                 f = comp.getRefY();
         }
         if (f > refY)
             f = refY;
         return f;
    }

    public void setRefH(float x) {
         refH = x;
    }

    public float getRefH() {
         return refH;
    }

    public void timerProc() {
         Point mousePt;
         boolean inTab = false;
         boolean inObj = false;

         try {
             PointerInfo ptInfo = MouseInfo.getPointerInfo();
             mousePt = ptInfo.getLocation();
         }
         catch (HeadlessException e) {
             pinShow(false);
             // setTimer(false);
             return;
         }
         inTab = tabRec.contains(mousePt.x, mousePt.y);
         timeCount++;
         if (timeCount == 1) {
             if (!inTab) {
                 // setTimer(false);
                 pinShow(false);
                 return;
             }
             if (!superPanel.popupTool(name)) {
                 setTimer(false);
             }
             return;
         }
         
         if (!bPopup) {
             if (timeCount > 4)
                 pinShow(false);
                 // setTimer(false);
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
                  int d = 0;
                  if (thisRec.y > tabRec.y)
                      d = thisRec.y - tabRec.y;
                  else if (tabRec.y > (thisRec.y + thisRec.height))
                      d = tabRec.y - thisRec.y - thisRec.height;
                  if (d > 100) {
                      walkAllowed = 1;
                      if (d > 300)
                         walkAllowed = 2;
                  }
                  else
                      walkAllowed = 0;
                  walkCount = 0;
              }
              inObj = thisRec.contains(mousePt.x, mousePt.y);
              if (!inObj) {
                 walkCount++;
                 if (walkCount > walkAllowed) 
                       pinShow(false);
                 return;
              }
              else {
                 walkAllowed = 0;
              }
         }
         else {
              walkCount = 0;
         }
    }

    public Rectangle getDividerBounds() {
         return null;
    }

    public void setBounds(int x, int y, int w, int h) {
         super.setBounds(x, y, w, h);
         if (bPopup) {
             return;
         }
         if (isShowing()) {
              locY = y;
              Container p = getParent();
              if (p != null) {
                  if (p instanceof JSplitPane) {
                        JSplitPane jp = (JSplitPane) p;
                        if (jp.getBottomComponent() == this) {
                            locY = jp.getDividerLocation(); 
                            y = y - 6;
                        }
                        else {
                            locY = jp.getY() - 6;
                            if (locY < 0)
                                locY = 0;
                        }
                        locX = 0;
                   }
                   else {
                        locY = y;
                        locX = x;
                   }
              }
              p = pComp;
              if (superPanel != null)
                   p = superPanel;
              if (p != null) {
                   Dimension d = p.getSize();
                   Point pt0 = p.getLocationOnScreen();
                   Point pt1 = getLocationOnScreen();
                   y = pt1.y - pt0.y;
                   float f = (float) y / (float) d.height;
                   if (f < 1.0f)
                      refY = f;
                   refH = (float) h / (float) d.height;
                   refX = (float) x / (float) d.width;
                   if (tab != null) {
                      tab.refX = refX;
                      tab.refY = refY;
                   }
              }
         }
    }


    class pushpinLayout implements LayoutManager {

        public void addLayoutComponent(String tname, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // preferredLayoutSize()

        public Dimension minimumLayoutSize(Container target) {
            int h = 0;
            if (titleComp != null && titleComp.isVisible())
                 h = titleComp.height;
            else if (pinBut.isVisible())
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
                int     x0, y0;
                int     cw = panSize.width - insets.left - insets.right;
                int     ch = panSize.height - insets.top - insets.bottom;
                int     h0;
                if (cw < 2 ||  ch < 2)
                   return;
                h0 = 0;
                dim = closeBut.getPreferredSize();
                if (bShowPin)
                    h0 = dim.height;
                x0 = 0;
                y0 = 0;
                if (titleComp != null && titleComp.isVisible()) {
                    if (titleComp.height > h0)
                        h0 = titleComp.height;
                    titleComp.setBounds(x0, y0, cw, h0);
                    ch = ch - h0;
                }
                if (bShowPin) {
                   y = (h0 - dim.height) / 2 + y0;
                   if (y < 0)
                       y = 0;
                   x = panSize.width - dim.width - 4 - x0;
                   if (x > 0)
                      closeBut.setBounds(x, y, dim.width, dim.height);
                   dim = pinBut.getPreferredSize();
                   x = x - dim.width - 4;
                   if (x > 0)
                      pinBut.setBounds(x, y, dim.width, dim.height);
                   pinX = x;
                }
                else
                   pinX = dim.width;

                if (ch < 1)
                    ch = 1;
                if (inComp != null)
                   inComp.setBounds(x0, h0+y0, cw, ch);
            }
        }
    }

    private class PushpinTitle extends JPanel {
        private String label;
        private String tname;
        public int width = 0;
        public int width2 = 0;
        public int height = 0;
        private int descent = 0;

        public PushpinTitle( String s) {
             label = s;
             setOpaque(false);
             setFont(getFont().deriveFont(Font.BOLD, 12.0f));
             calSize();
        }

        public void setTitle(String s) {
             label = s;
             calSize();
        }

        public void setName(String s) {
             tname = s;
             calSize();
        }

        private void calSize() {
            FontMetrics fm = getFontMetrics(getFont());
            height = fm.getAscent() + fm.getDescent();
            descent = fm.getDescent();
            if (label != null)
               width = fm.stringWidth(label) + 6;
            if (tname != null)
               width2 = fm.stringWidth(tname) + 4;
        }

        public void paint(Graphics g) {
            Dimension dim = getSize();
            int h = dim.height;
            int w = dim.width;
            Graphics2D g2 = (Graphics2D) g;
            if (bPopup) {
               g2.setColor(Util.getActiveBg());
               // g2.setColor(Color.yellow);
            }
            else {
                // GradientPaint gp = new GradientPaint(0, 0, tColor, w, h, tColor2); 
                // g2.setPaint(gp);
               g2.setColor(Util.getInactiveBg());
            }
            g2.fill(new Rectangle(0, 0, w, h));
            int x = 4;  
            int y = dim.height - descent; 
            if (bPopup)
                g.setColor(Util.getActiveFg());
            else
                g.setColor(Util.getInactiveFg());
            if (x + width < pinX)
                g.drawString(label, x, y);
            else {
                if (x + width2 < pinX)
                    g.drawString(tname, x, y);
            }
        }
    }

    private class PushpinListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            String cmd = e.getActionCommand();
            if (cmd.equals("hide")) {
                 pinHide();
            }
            if (cmd.equals("close")) {
                 pinClose();
            }
        }
   }
}
