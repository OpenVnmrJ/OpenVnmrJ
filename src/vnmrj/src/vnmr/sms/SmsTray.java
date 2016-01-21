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
import javax.swing.event.*;
import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.Timer;
import java.beans.*;

import vnmr.util.*;
import vnmr.bo.VObjIF;
import vnmr.bo.VObjDef;
import vnmr.templates.LayoutBuilder;

public class SmsTray extends JScrollPane
   implements SmsDef, ActionListener, PropertyChangeListener
{
   public int type = 0;
   private GenPlateIF plate = null; 
   private SmsPlate smsPlate = null; 
   private SmsPlate sms100Plate = null; 
   private CaroPlate caroPlate = null; 
   private GrkPlate grkPlate = null; 
   private SmsLayoutIF smsLayout = null; 
   private Timer  timer = null;
   private SamplePanel  viewPan;
   private SmsSample  enteredSample;
   private SmsSample  locPopupSample;
   private JViewport  viewPort;
   private boolean bShowPlate = true;
   private boolean bShowTooltip = true;
   private boolean bSelectable = true;
   private TrayPopupAction ear;
   private JPopupMenu locPopup;
   private JPopupMenu mPopup;
   private String  tipStr = null;
   private String  locPopupFilePath;
   private String  popupFilePath;
   private long    locPopupDate;
   private long    popupDate;
   private int     tipIndex = 0;
   private Dimension screenDim = new Dimension(1000, 1000);
   private static String locPopupName ="TrayLocPopup.xml";
   private static String popupName ="TrayPopup.xml";

   public SmsTray() {
       this.type = 0;
       setBorder(BorderFactory.createEtchedBorder());
       viewPort = getViewport();
       viewPan = new SamplePanel();
       setViewportView(viewPan);
       viewPan.addMouseListener(new MouseAdapter() {
           public void mouseClicked(MouseEvent evt) {
               sampleClicked(evt); 
           }

           public void mousePressed(MouseEvent evt) {
               samplePressed(evt); 
           }
       });

       viewPan.addMouseMotionListener(new MouseMotionAdapter() {
           public void mouseMoved(MouseEvent evt) {
               cursorMoved(evt); 
           }
       });

       // buildPopup();
       updateValue();
       ToolTipManager.sharedInstance().registerComponent(viewPan);
       Toolkit tk = Toolkit.getDefaultToolkit();
       screenDim = tk.getScreenSize();
       DisplayOptions.addChangeListener(this);
   }

   public void clearSelect() {
       if (plate != null)
           plate.clearSelect();
       SmsUtil.showSelected("");
   }

   public void setSelectable(boolean b) {
       bSelectable = b;
       if (plate != null)
           plate.setSelectable(bSelectable);
   }

   public void setSelection(Vector<String> v) {
       if (plate == null)
           return;
       boolean bGot = plate.setSelection(v);
       if (bGot) {
           getParent().repaint();
           startTimer();
       }
   }

   public void setSelection(boolean b) {
       if (!b) {
          clearSelect();
          return;
       }
   }

   public void setBg(Color c) {
       setBackground(c);
       viewPort.setBackground(c);
       viewPan.setBackground(c);
   }

   public void setType(int newType) {
       boolean changed = false;
       if (type != newType)
           changed = true;
       enteredSample = null;
       type = newType;
       if (changed) {
           clearSelect();
           SmsUtil.showSampleInfo(null);
           viewPan.setPreferredSize(new Dimension(10, 10));
           switch (type) {
             case SMS:
                     if (smsPlate == null) {
                         smsPlate = new SmsPlate(viewPan, 50);
                     }
                     plate = (GenPlateIF) smsPlate;
                     viewPan.setLayout(smsPlate.getLayoutMgr());
                     SmsUtil.setTrayPlate(plate);
                     break;
             case SMS100:
                     if (sms100Plate == null) {
                         sms100Plate = new SmsPlate(viewPan, 100);
                     }
                     plate = (GenPlateIF) sms100Plate;
                     viewPan.setLayout(sms100Plate.getLayoutMgr());
                     SmsUtil.setTrayPlate(plate);
                     break;
             case CAROSOL:
                     if (caroPlate == null) {
                         caroPlate = new CaroPlate(viewPan, 9);
                     }
                     plate = (GenPlateIF) caroPlate;
                     viewPan.setLayout(caroPlate.getLayoutMgr());
                     SmsUtil.setTrayPlate(plate);
                     break;
             case VAST:
                     showGrkPlate(grkPlate);
                     break;
             case GRK49:
                     showGrkPlate(grkPlate);
                     break;
             case GRK97:
                     showGrkPlate(grkPlate);
                     break;
             case GRK12:
                     showGrkPlate(grkPlate);
                     break;
           }
           // if (type != VAST)
           //    SmsUtil.setTrayPlate(plate);

           if (plate != null) {
              plate.setSelectable(bSelectable);
              smsLayout = (SmsLayoutIF) plate.getLayoutMgr();
           }
           revalidate();
           viewPan.validate();
           //getParent().repaint();
       }
   }

   public void showGrkPlate(GrkPlate gplate) {
       if (gplate == null)
          return;
       clearSelect();
       enteredSample = null;
       grkPlate = gplate;
       gplate.showPlate(viewPan);
       plate = (GenPlateIF) gplate;
       plate.setSelectable(bSelectable);
       viewPan.setLayout(plate.getLayoutMgr());
       smsLayout = (SmsLayoutIF) plate.getLayoutMgr();
       SmsUtil.setTrayPlate(plate);
       viewPan.setPreferredSize(new Dimension(10, 10));
       viewPan.validate();
       revalidate();
       getParent().repaint();
   }

   private void sendToVnmr(ButtonIF vif, String s) {
       if (SmsUtil.getTestMode()) {
           System.out.println("Submit: "+s);
           return;
       }
       if (SmsUtil.getDebugMode())
           System.out.println("Submit: "+s);

       if (vif != null)
           vif.sendToVnmr(s);
   }

   public void submit(ButtonIF bif, String s) {

       if (plate == null)
          return;
       Vector<SmsSample> list = plate.getSelectList();
/*
       if (list == null)
          return;
       int m = list.size();
       if (m < 1)
          return;
*/
       StringTokenizer tok = new StringTokenizer(s, " ,\n");
       String macro = "xmsubmit";
       String s1 = "day";
       String s2 = "Queued";
       if (tok.hasMoreTokens())
            macro = tok.nextToken().trim();
       if (tok.hasMoreTokens()) {
            s1 = tok.nextToken().trim();
       }
       if (tok.hasMoreTokens()) {
            s2 = tok.nextToken().trim();
       }
       int rackId = 0;
       int zoneId = 0;
       int objNum = 0;
       SmsSample obj;
       if (list != null && list.size() > 0) {
           obj = list.elementAt(0);
           rackId = obj.rackId;
           zoneId = obj.zoneId;
       }
       StringBuffer sb = new StringBuffer().append(macro).append("('");
       sb.append(s1).append("', '").append(rackId).append("', '");
       sb.append(zoneId).append("', '");

       if (list != null && list.size() > 0) {
          for (int k = 0; k < list.size(); k++) {
             obj = list.elementAt(k);
             if (obj.selected) {
                if (zoneId != obj.zoneId || rackId != obj.rackId) {
                    if (objNum > 0) {
                        sb.append("')\n");
                        sendToVnmr(bif, sb.toString());
                        objNum = 0;
                    }
                    rackId = obj.rackId;
                    zoneId = obj.zoneId;
                    sb = new StringBuffer().append(macro).append("('");
                    sb.append(s1).append("', '").append(rackId).append("', '");
                    sb.append(zoneId).append("', '");
                }
                objNum++;
                sb.append(obj.idStr).append(" ");
             }
          }
          if (objNum > 0) {
              sb.append("')\n");
              sendToVnmr(bif, sb.toString());
          }
       }
       else {
           sb.append("')\n");
           sendToVnmr(bif, sb.toString());
       }
//       if (list != null)
//          SmsUtil.preQueueSamples(list, s2);
       clearSelect();
       list = null;
       getParent().repaint();
   }

   public void setZone(int n) {
       if (plate == null)
          return;
       enteredSample = null;
       plate.setZone(n);
       viewPan.setPreferredSize(new Dimension(10, 10));
       viewPan.revalidate();
       revalidate();
       getParent().repaint();
   }

   public void setRotate(boolean b) {
       int x = 0;
       int y = 0;
       Rectangle  r = viewPort.getBounds();
       Dimension dim = viewPort.getViewSize();

       if (b) {
            y = (dim.height - r.height) / 2;
            if (y < 10)
                y = 0;
       }
       else {
            x = (dim.width - r.width) / 2;
            if (x < 10)
               x = 0;
       }
       viewPort.setViewPosition(new Point(x, y));
       repaint();
   }

   public void setZoom(int n) {
       if (smsLayout == null)
           return;
       Dimension dim = viewPan.getSize();
       int dir = smsLayout.zoomDir();
       if (n > 0) {
            if (plate.setScale(n)) {
               if (dir == 0 || dir == 1) { 
                  dim.height = (int) ((float) dim.height * 1.2f);
               }
               if (dir == 0 || dir == 2) { 
                  dim.width = (int) ((float) dim.width * 1.2f);
               }
            }
            else
               plate.getLayoutMgr().layoutContainer(viewPan);
       }
       if (n ==  0) {
            plate.setScale(n);
            dim.width = 10;
            dim.height = 10;
       }
       else if (n < 0) {
            boolean toSet = plate.setScale(n);
            if (toSet) {
               JScrollBar jb = getVerticalScrollBar();
               if (jb != null && jb.isVisible()) {
                   dim.height = (int) ((float) dim.height * 0.8f);
               }
               jb = getHorizontalScrollBar();
               if (jb != null && jb.isVisible()) {
                   dim.width = (int) ((float) dim.width * 0.8f);
               }
            }
            else
               plate.getLayoutMgr().layoutContainer(viewPan);
       }
       viewPort.setViewSize(dim);
       viewPan.setPreferredSize(dim);
       revalidate();
       getParent().repaint();
   }

   private void hilitSample() {
       boolean gotSample = false;

       if (plate != null) {
           Vector<SmsSample> list = plate.getSelectList();
           if (list.size() > 0)
               gotSample = true;
           viewPan.repaint();
       }
       if (!gotSample)
           timer.stop();
   }

   public void actionPerformed(ActionEvent e) {
       Object obj = e.getSource();
       if (obj instanceof Timer) {
            hilitSample();
       }
   }

   public void cleanUp() {
       if (timer != null)
           timer.stop();
       //clearSelect();
       //if (plate != null) {
       //    plate.clearAllSample();
       //}
   }

   public void setShowPlate(boolean b) {
       bShowPlate = b;
   }
   
        
   public void startTimer() {
       getParent().repaint();
       if (timer == null)
            timer = new Timer(200, this);
       if (timer != null && (!timer.isRunning()))
            timer.restart();
   }

   private void sampleClicked(MouseEvent ev) {
       if (ev.getButton() == 3)
           return;

       if (plate == null)
           return;
       if (!bSelectable)
           return;
       int buttonId = 1;
       int modif = ev.getModifiers();
       if ((modif & InputEvent.BUTTON1_MASK) == 0) {
           buttonId = 2;
       }
       buttonId = ev.getButton();
       int x = ev.getX();
       int y = ev.getY();
       boolean got = plate.setSelect(x, y, modif, buttonId);
       if (got) {
           getParent().repaint();
           startTimer();
       }
   }

   private void updatePopupValue(JPopupMenu p) {
       ButtonIF vif = Util.getActiveView();
       if (p == null || vif == null)
           return;

       int nmembers = p.getComponentCount();
       for (int i = 0 ; i < nmembers; i++) {
           Component comp = p.getComponent(i);
           if (comp instanceof VObjIF) {
              VObjIF obj = (VObjIF) comp;
              obj.setVnmrIF(vif);
              obj.updateValue();
           }
       }
   }

   public void updateValue() {
       buildPopup();
       updatePopupValue(locPopup);
       updatePopupValue(mPopup);
   }

   public void showTooltip(boolean b) {
        bShowTooltip = b;
        tipIndex = 0;
   }

   private void setPopupColor() {
        if (locPopup != null)
           SwingUtilities.updateComponentTreeUI(locPopup);
        if (mPopup != null)
           SwingUtilities.updateComponentTreeUI(mPopup);
   }

   public void propertyChange(PropertyChangeEvent evt) {
        if (DisplayOptions.isUpdateUIEvent(evt)) {
            setPopupColor();
        }
   }

   private void removeObjListener(AbstractButton c) {
       ActionListener[] ls = c.getActionListeners();
       if (ls == null || ls.length < 1)
           return;
       for (int i = 0; i < ls.length; i++)
           c.removeActionListener(ls[i]);
   }

   private void addTrayListener(Container pObj) {
       int nmembers = pObj.getComponentCount();
       if (nmembers < 1)
           return;
       for (int i = 0 ; i < nmembers; i++) {
            Component comp = pObj.getComponent(i);
            if (comp instanceof AbstractButton) {
                AbstractButton ab = (AbstractButton) comp;
                removeObjListener(ab); 
                ab.addActionListener(ear);
            }
            if (comp instanceof Container)
                addTrayListener((Container)comp);
       }
   }

   private void buildXmlPopup(JPopupMenu p, String f) {
       ButtonIF vif = Util.getActiveView();

       try {
            LayoutBuilder.build(p, vif, f);
       }  catch(Exception e) {
            Messages.writeStackTrace(e);
       }
       if (ear == null)
          ear = new TrayPopupAction();

       int nmembers = p.getComponentCount();
       for (int i = 0 ; i < nmembers; i++) {
            Component comp = p.getComponent(i);
            if (comp instanceof AbstractButton) {
                AbstractButton ab = (AbstractButton) comp;
                removeObjListener(ab); 
                ab.addActionListener(ear);
            }
            if (comp instanceof Container)
                addTrayListener((Container)comp);
       }
   }

   private void buildPopup() {
       boolean bNewFile;
       File newFd;
       String filePath = FileUtil.openPath("INTERFACE"+File.separator+locPopupName);
       if (filePath != null) {
           bNewFile = false;
           newFd = new File(filePath);
           if (newFd.lastModified() != locPopupDate)
               bNewFile = true;
           if (!filePath.equals(locPopupFilePath))
               bNewFile = true;
           if (bNewFile) {
               locPopupFilePath = filePath;
               locPopupDate = newFd.lastModified();
               locPopup = new JPopupMenu();
               buildXmlPopup(locPopup, filePath);
               locPopup.addPopupMenuListener(new PopupMenuListener() {
                   public void popupMenuWillBecomeVisible(PopupMenuEvent e) {
                   }
                   public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
                        locPopupInvisible();
                   }
                   public void popupMenuCanceled(PopupMenuEvent e) {
                   }
               });
           }
       }
       filePath = FileUtil.openPath("INTERFACE"+File.separator+popupName);
       if (filePath != null) {
           bNewFile = false;
           newFd = new File(filePath);
           if (newFd.lastModified() != popupDate)
               bNewFile = true;
           if (!filePath.equals(popupFilePath))
               bNewFile = true;
           if (bNewFile) {
               popupFilePath = filePath;
               popupDate = newFd.lastModified();
               mPopup = new JPopupMenu();
               buildXmlPopup(mPopup, filePath);
           }
       }
       setPopupColor();
   }

   private void locPopupInvisible() {
       if (locPopupSample == null)
           return;
       locPopupSample.bPopupItem = false;
       repaint();

   }

   private void samplePressed(MouseEvent ev) {
       if (plate == null || locPopup == null)
           return;
       if (ev.getButton() != 3)
           return;
       int modif = ev.getModifiers();
       if ((modif & Event.SHIFT_MASK) != 0)
           return;
       if ((modif & Event.CTRL_MASK) != 0)
           return;
       int x = ev.getX();
       int y = ev.getY();
       locPopupSample = plate.locateSample(x, y);
       JPopupMenu popup = null;
       
       if (locPopupSample == null) {
           popup = mPopup;
       }
       else {
           popup = locPopup;
           locPopupSample.bPopupItem = true;
       }
       if (popup == null)
           return;
       Point pt = getLocationOnScreen();
       Dimension dim = popup.getPreferredSize();
       x = x + 14;
       int x2 = x + pt.x + dim.width + 18 - screenDim.width;
       if (x2 >= 0) {
           x = x - x2;
           y = y + 14;
       }
           
       popup.show(this, x, y);
       repaint();
   }

   private void cursorMoved(MouseEvent ev) {
       if (plate == null)
           return;
       if (!bShowTooltip)
           return;

       int x = ev.getX();
       int y = ev.getY();
       int s = -1;
       SmsSample sample = plate.locateSample(x, y);
       if (enteredSample != null) {
           if (enteredSample == sample) {
               s = enteredSample.cursorAt(x, y);
               sample = null;
           }
           else
               s = enteredSample.cursorAt(-1, -1);
       }
       if (sample != null) {
           enteredSample = sample;
           s = enteredSample.cursorAt(x, y);
       }
       if (tipIndex != s) {
           tipStr = SmsColorEditor.getTooltip(s);           
           tipIndex = s;
       }
   }

   private class SamplePanel extends JPanel {

       public void paint(Graphics g) {
           super.paint(g);
           if (!bShowPlate)
               return;
           if (plate != null) {
               Graphics2D g2d = (Graphics2D) g;
               g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
               g2d.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
              plate.paintSample(g, false);
              /***
               if (SmsSample.isImageSample())
                   plate.paintSample(g, false);
               else
                   plate.paintSample(g, true);
              ***/
           }
       }

       public String getToolTipText(MouseEvent ev) {
           if (!bShowTooltip)
               return null;
           return tipStr;
       }
   }

   private class TrayPopupAction implements ActionListener {
       public void actionPerformed(ActionEvent e) {
            Object obj = e.getSource();
            if (obj == null)
               return;
            String cmd = null;
            if (obj instanceof VObjIF) {
               cmd = ((VObjIF)obj).getAttribute(VObjDef.CMD);
            }
            if (cmd != null && (cmd.length() > 0))
              processCmd(cmd);
       }

       private void processCmd(String cmd) {
            ButtonIF vif = Util.getActiveView();
            if (vif == null || plate == null)
               return;
            int len = cmd.length();
            int sx = 0;
            int d1, d2, d3, dx;
            int item;
            StringBuffer sb = new StringBuffer();
            while (true) {
                d1 = cmd.indexOf('#', sx);
                if (d1 < sx)
                    break;
                item = 0;
                dx = len+1;
                d1 = cmd.indexOf("loc#", sx);
                d2 = cmd.indexOf("rack#", sx);
                d3 = cmd.indexOf("zone#", sx);
                if (d1 >= sx) {
                    item = 1;
                    dx = d1;
                }
                if (d2 >= sx && d2 < dx) {
                    item = 2;
                    dx = d2;
                }
                if (d3 >= sx && d3 < dx) {
                    item = 3;
                    dx = d3;
                }
                if (item == 0)
                    break;
                if (sx < dx)
                   sb.append(cmd.substring(sx, dx));
                if (item == 1) {
                   if (locPopupSample == null)
                       sb.append("0");
                   else
                       sb.append(locPopupSample.idStr);
                   sx = d1 + 4;
                }
                if (item == 2) {
                   if (locPopupSample == null)
                       sb.append("0");
                   else
                       sb.append(Integer.toString(locPopupSample.rackId));
                   sx = d2 + 5;
                }
                if (item == 3) {
                   if (locPopupSample == null)
                       sb.append("0");
                   else
                       sb.append(Integer.toString(locPopupSample.zoneId));
                   sx = d3 + 5;
                }
            }
            if (sx < len)
                sb.append(cmd.substring(sx, len));
            vif.sendToVnmr(sb.toString());
       }
   }
}

