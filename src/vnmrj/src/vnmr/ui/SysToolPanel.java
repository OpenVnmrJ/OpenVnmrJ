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
import javax.swing.*;

import vnmr.util.*;
import vnmr.templates.*;
import vnmr.bo.*;

public class SysToolPanel extends JPanel
{
   // private  SessionShare sshare;
   private  JComponent   rightTool = null;
   private  JComponent   leftTool = null;
   private  JComponent   middleTool = null;
   private  JComponent   popupTool = null;
   private  ExpViewIF    expIf = null;
   private  JPanel       vpPanel = null;
   private  JComponent   imgVpPanel = null;
   private  String  vpFile = "INTERFACE"+File.separator+"VpMenu.xml";
   private  JComboBox    vpMenu = null;
   private  Color        bgColor;
   private  VpBoxRender  vpRender = null;
   private  ArrayList    vpList = null;
   private  int          vpNum = 0;
   private  int[]        vpStatus;
   private  int          vpSize = 9;
   private  VOnOffButton  ctrlButton;

   public SysToolPanel(SessionShare s)  {
      // this.sshare = s;
      setLayout(new toolLayout());
      bgColor = Util.getToolBarBg();
      setBackground(bgColor);
      ctrlButton = new VOnOffButton(VOnOffButton.ICON, this);
      ctrlButton.setPreferredSize(new Dimension(8, 16));
      add(ctrlButton);
      ArrayList types = Util.getUser().getAppTypes();
      if (!Util.isImagingUser()) {
          String vpFilePath = FileUtil.openPath(vpFile);
          if (vpFilePath != null) {
              vpPanel = new JPanel();
              try {
                 LayoutBuilder.build(vpPanel, null, vpFilePath);
              }  catch(Exception e) {
                 Messages.writeStackTrace(e);
              }
          }
          if (vpPanel != null) {
              vpPanel.setLayout(new SimpleVLayout(0, 0, true));
              searchVpMenu();
              if (vpMenu != null) {
                 vpList = ((VMenu)vpMenu).getCompList();
                 ((VMenu)vpMenu).setVpMenu(true);
                 ((VMenu)vpMenu).setDefault(false);
                 if (vpList != null && vpList.size() > 1) {
                     Util.setViewportMenu((JComponent) vpMenu);
                     add(vpPanel);
                 }
                 else {
                     vpList = null;
                 }
                 vpPanel.setVisible(false);
                 setVpMenuRender();
              }
          }
      }
      if (vpList != null)
          vpSize = vpList.size();
      vpStatus = new int[vpSize+1];
   }

   public VOnOffButton getControlButton() {
        return ctrlButton;
   }

   public void setToolBar(JComponent obj) {
       if (obj == null)
           return;
       if (obj == rightTool)
           return;
       if (rightTool != null)
           remove(rightTool);
       rightTool = obj;
        add(rightTool);
        validate();
        obj.repaint();
   } 

   public void setLeftToolBar(JComponent obj) {
       if (obj == null)
           return;
       if (obj == leftTool)
           return;
       if (leftTool != null)
           remove(leftTool);
       leftTool = obj;
       add(leftTool);
       validate();
       obj.repaint();
   }

   public void setPopupToolBar(JComponent obj) {
       if (obj == null || obj == popupTool) {
           return;
       }
       popupTool = obj;
       add(popupTool);
       validate();
       obj.repaint();
   }

   public void setImageVpPanel(JComponent obj) {
       if (obj != null && (obj == imgVpPanel))
           return;
       if (imgVpPanel != null) {
          remove(imgVpPanel);
          imgVpPanel = null;
       }
       if (obj != null && vpPanel == null) {
           imgVpPanel = obj;
           add(imgVpPanel);
       }
   }

   public void setMiddleToolBar(JComponent obj) {
       if (obj != null && (obj == middleTool))
           return;
       if (middleTool != null)
          remove(middleTool);
       middleTool = obj;
       if (obj != null) {
          add(middleTool, 0);
          ctrlButton.setMoveable(false);
       }
       else
          ctrlButton.setMoveable(true);
       validate();
       if (obj != null)
          obj.repaint();
   } 

   public boolean hasMiddleToolBar() {
       if (middleTool != null)
           return true;
       return false;
   }

   public void setVpMenuVisible(boolean b) {
       if (vpList != null && vpPanel != null)
           vpPanel.setVisible(b);
   } 

   private void setVpMenuRender() {
       Font ft = vpMenu.getFont();
       if (ft == null) 
           return;
       FontMetrics fm = getFontMetrics(ft);
       if (fm == null)
           return;
       int k = vpMenu.getItemCount();
       int w = 0;
       int w2;
       int h = ft.getSize();
       for (int i = 0; i < k; i++) {
           Object obj = vpMenu.getItemAt(i);
           if ( obj instanceof String) {
              w2 = fm.stringWidth((String) obj);
              if (w2 > w)
                 w = w2;
           }
       }
       if (w < 20)
           w = 20;
       if (vpRender == null) {
           vpRender = new VpBoxRender();
           vpMenu.setRenderer(vpRender);
       }
       vpRender.setPreferredSize(new Dimension(w+10, h+4));
   }

   public void setBg(Color bg) {
       bgColor = bg;
       setBackground(bg);
       if (vpMenu != null) {
          vpMenu.setBackground(bg);
          vpPanel.setBackground(bg);
       }
   }

   public void saveUiLayout() {
/*
        if (sshare == null)
            return;
*/
   }

   private void setVpStatus() {
       if (vpList == null)
           return;
       int k = vpList.size();
       int i;
       for (i = 0; i < k; i++) {
           JComponent ml = (JComponent) vpList.get(i); 
           if (ml.isEnabled())
               vpStatus[i] = 1;
           else
               vpStatus[i] = 0;
       }
       if (expIf != null) {
           for (i = 0; i < k; i++) {
               if (vpStatus[i] > 0)
                  expIf.setCanvasAvailable(i, true);
               else
                  expIf.setCanvasAvailable(i, false);
           }
       }
   }

   private void setVpList(int n) {
       if (vpList == null || n < 1)
            return;
       int k = vpList.size();
       if (k < 1)
            return;
       vpNum = n;
       int index = vpMenu.getSelectedIndex();
       if (n > k)
            n = k;
       ((VMenu)vpMenu).setAdddMode(true);
       vpMenu.removeAllItems(); 
       for (int i = 0; i < n; i++) {
           VMenuLabel ml = (VMenuLabel) vpList.get(i); 
           String s = ml.getAttribute(VObjDef.LABEL);
           vpMenu.addItem(s);
       }
       if (index >= n)
           index = 0;
       if (vpStatus[index] < 1) {
           for (k = 0; k < vpNum; k++) {
              if (vpStatus[k] > 0) {
                 index = k;
                 break;
              }
           }
       }
       vpMenu.setSelectedIndex(index);
       setVpMenuRender();
       if (n > 1)
          setVpMenuVisible(true);
       else
          setVpMenuVisible(false);
       ((VMenu)vpMenu).setAdddMode(false);
   }

   private void setVpList() {
       setVpList(vpNum);
   }

   public void setVpNum(int n) {
       if (expIf == null)
            expIf = (ExpViewIF) Util.getViewArea();
       if (n < 1 || vpMenu == null)
            return;
       if (n == vpNum)
            return;
       setVpList(n);
   }

   public void updateVpMenuLabel() {
       setVpList();
       setVpMenuRender();
   } 

   public void updateVpMenuStatus() {
        setVpStatus();
   } 

   public void setActiveExp(ExpPanel exp) {
       if (vpMenu == null || exp == null)
            return;
       
       int k = exp.getViewId();
       int index = vpMenu.getSelectedIndex();
       if (vpStatus[k] > 0) {
           if (index != k)
               vpMenu.setSelectedIndex(k);
       }
       ((VObjIF) vpMenu).setVnmrIF((ButtonIF)exp);
   }

   public void setActiveVp(int n) {
        if (vpMenu == null)
            return;
        if (n >= vpMenu.getItemCount())
            return;
       if (vpStatus[n] < 1)
            return;
       int index = vpMenu.getSelectedIndex();
       if (index != n) 
           vpMenu.setSelectedIndex(n);
   }

   public void setVpEnable(int n, boolean b) {
   }

   public void init() {
       if (vpList != null) {
           vpMenu.setSelectedIndex(0);
           if (vpMenu instanceof VMenu) {
               ExpPanel.addExpListener((ExpListenerIF)vpMenu);
           }
       }
       for (int i = 0; i < vpSize; i++)
           vpStatus[i] = 1;
   }


   private void setVpMenu(JComboBox c) {
       if (!(c instanceof VMenu))
           return;
       if (vpMenu == null) {
           vpMenu = (JComboBox) c;
           return;
       }
       String name = ((VObjIF)c).getAttribute(VObjDef.LABEL);
       if (name == null)
           return;
       if (name.equals("Viewport")) 
           vpMenu = (JComboBox) c;
   }

   private void checkObj(Container c, int level) {
       int k = c.getComponentCount();
       for (int i = 0; i < k; i++) {
           Component m = c.getComponent(i);
           if (m != null) {
               if ( m instanceof JComboBox) {
                   setVpMenu((JComboBox) m);
               }
               else if (m instanceof Container)
                   checkObj((Container)m, level+1);
           }
       }
   }

   private void searchVpMenu() {
       if (vpPanel == null)
           return;
       int k = vpPanel.getComponentCount();

       for (int i = 0; i < k; i++) {
           Component m = vpPanel.getComponent(i);
           if (m != null) {
               if ( m instanceof JComboBox) {
                   setVpMenu((JComboBox) m);
               }
               else if (m instanceof Container)
                   checkObj((Container)m, 1);
           }
       }
   }

   private class toolLayout implements LayoutManager {

       public void addLayoutComponent(String name, Component comp) {}

       public void removeLayoutComponent(Component comp) {}

       public Dimension preferredLayoutSize(Container target) {
          int w = 500;
          int h = 0;
          int h2 = 20;
          if (rightTool != null)
              h = rightTool.getPreferredSize().height;
          if (leftTool != null)
              h2 = leftTool.getPreferredSize().height;
          if (h2 > h)
              h = h2;
          if (vpPanel != null && vpPanel.isVisible()) {
              h2 = vpPanel.getPreferredSize().height + 2;
              if (h2 > h)
                  h = h2;
          }
          if (imgVpPanel != null && imgVpPanel.isVisible()) {
              h2 = imgVpPanel.getPreferredSize().height + 2;
              if (h2 > h)
                  h = h2;
          }
          return new Dimension(w, h);
       }

       public Dimension minimumLayoutSize(Container target) {
           return new Dimension(0, 0); // unused
       }

       public void layoutContainer(Container target) {
           synchronized (target.getTreeLock()) {
              Dimension dim = target.getSize();
              Insets insets = target.getInsets();
              int dw = dim.width;
              int x = dim.width;
              int y = 0;
              int x0 = insets.left;
              Dimension d;
              if (dw < 10 || dim.height < 8)
                  return;
              y = 1;
              if (imgVpPanel != null && imgVpPanel.isVisible()) {
                  d = imgVpPanel.getPreferredSize();
                  x = dw - d.width - 4;
                  d.height = dim.height - 2;
                  imgVpPanel.setBounds(x, y, d.width, d.height);
                  dw = x - 4;
              }
              if (vpPanel != null && vpPanel.isVisible()) {
                  d = vpPanel.getPreferredSize();
                  x = dw - d.width - 4;
                  d.height = dim.height - 2;
                  vpPanel.setBounds(x, y, d.width, d.height);
                  dw = x - 4;
              }
              dw = dw - 2;
              if (rightTool != null) {
                  d = rightTool.getPreferredSize();
                  x = dw - d.width;
                  y = (dim.height - d.height) / 2;
                  if (x < 10) {
                     x = 10;
                     d.width = dw - 10;
                  }
                  if (y < 0) y = 0;
                  rightTool.setBounds(x, y, d.width, d.height);
                  dw = x - 4;
              }
              ctrlButton.setBounds(x0, y, 8, dim.height);
              x0 += 12;
              if (leftTool != null) {
                  d = leftTool.getPreferredSize();
                  y = (dim.height - d.height) / 2;
                  if (y < 0) y = 0;
                  if ((x0 + d.width) > dw)
                     d.width = dw - x0;
                  leftTool.setBounds(x0, y, d.width, d.height);
                  x0 = x0 + d.width + 4;
              }
              if (popupTool != null && popupTool.isVisible()) {
                  d = popupTool.getPreferredSize();
                  y = (dim.height - d.height) / 2;
                  if (y < 0) y = 0;
                  if ((x0 + d.width) > dw)
                     d.width = dw - x0;
                  popupTool.setBounds(x0, y, d.width, d.height);
                  x0 = x0 + d.width + 4;
              }
              if (middleTool != null) {
                  d = middleTool.getPreferredSize();
                  y = 0;
                  d.width = dw - x0 - 4;
                  if (d.width <= 30) {
                     d.width = 30;
                  }
                  middleTool.setBounds(x0, y, d.width, dim.height);
              }
           } 
       }
   }

   private class VpBoxRender extends JLabel implements ListCellRenderer {
       private int oldIndex = 0;

       public VpBoxRender() {
          setOpaque(true);
          setHorizontalAlignment(LEFT);
          setVerticalAlignment(CENTER);
       }

       public Component getListCellRendererComponent(
                   JList list, Object value, int index,
                   boolean isSelected, boolean cellHasFocus) {

           if (value == null)
               return this;
           boolean  bHilit = isSelected;
           int sItem = list.getSelectedIndex();
           if (sItem >= 0) {
               if (vpStatus[sItem] < 1) {
                  if (index == oldIndex)
                     bHilit = true;
                  list.setSelectedIndex(oldIndex);
               }
           }

           String s = value.toString();
           if (index >= 0 && isSelected) {
              if (vpStatus[index] > 0)
                  oldIndex = index;
              else {
                  bHilit = false;
              }
           }
           if (bHilit) {
              setBackground(list.getSelectionBackground());
              setForeground(list.getSelectionForeground());
           } else {
              // setBackground(list.getBackground());
              setBackground(bgColor);
              setForeground(list.getForeground());
           }
           if (index >= 0 && vpStatus[index] < 1) {
              setBackground(bgColor);
              setEnabled(false);
           }
           else
              setEnabled(true);

           setText(s);

           return this;
        }
   }
}

