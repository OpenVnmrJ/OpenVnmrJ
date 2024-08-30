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
import javax.swing.*;
import java.beans.*;

import vnmr.templates.*;
import vnmr.util.*;
import vnmr.bo.*;


public class HelpOverlay extends JPanel implements ActionListener,
                          PropertyChangeListener {

    private SessionShare sshare;
    private Hashtable hs;
    private JButton closeBtn;
    private JButton navigateBtn;
    private JCheckBox showCk;
    private JPanel xmlPanel;
    private String xmlFileName;
    private String oldFilePath;
    private long  fileTime = 0;
    private Vector<Component> closeList;
    private Vector<Component> navigatorList;

    private Color bgColor;
    private Color bgColor1;

    public HelpOverlay(SessionShare sshare) {
         this.sshare = sshare;
         this.xmlFileName = "OverlayPanel.xml";
         buildGUi();
         setPanelBackground();
    }


    private void setPanelBackground() {
         Color c = UIManager.getColor("Panel.background");
         Color f = UIManager.getColor("Panel.foreground");
         bgColor = new Color(c.getRed(), c.getGreen(), c.getBlue(), 200);
         bgColor1 = new Color(f.getRed(), f.getGreen(), f.getBlue(), 240);
         showCk.setForeground(c);
         setOpaque(false);
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
         setPanelBackground();
    }

    private void updatePanel(Container c) {
         int num = c.getComponentCount();

         for (int i = 0; i < num; i++) {
            Component comp = c.getComponent(i);
            if (comp instanceof VObjIF) {
               ((VObjIF)comp).updateValue();
            }
            else if (comp instanceof Container)
               updatePanel((Container) comp);
         }
    }

    private void updatePanel() {
         if (xmlPanel == null)
             return;
         int i;
         int num = xmlPanel.getComponentCount();
         Component comp;
         for (i = 0; i < num; i++) {
            comp = xmlPanel.getComponent(i);
            if (comp instanceof VObjIF) {
               ((VObjIF)comp).updateValue();
            }
            else if (comp instanceof Container)
               updatePanel((Container) comp);
         }
         num = closeList.size();
         for (i = 0; i < num; i++) {
              comp = closeList.elementAt(i);
              if (comp != null)
                  ((VObjIF)comp).updateValue();
         }
         num = navigatorList.size();
         for (i = 0; i < num; i++) {
              comp = navigatorList.elementAt(i);
              if (comp != null)
                  ((VObjIF)comp).updateValue();
         }
    }

    private void replaceButtons(Container c) {
         int num = c.getComponentCount();

         for (int i = 0; i < num; i++) {
            Component comp = c.getComponent(i);
            if (comp instanceof VObjIF) {
                if (comp instanceof VGroup)
                    replaceButtons((Container) comp);
                else {
                    String name = ((VObjIF)comp).getAttribute(VObjDef.PANEL_NAME);
                    if (name != null) {
                        if (name.equals("close"))
                            closeList.add(comp);
                        if (name.equals("navigator"))
                            navigatorList.add(comp);
                    }
                }
            }
         }
    }

    private void replaceButtons() {
         if (xmlPanel == null)
             return;
         int i;
         int k;
         Component comp;
         int num = xmlPanel.getComponentCount();

         for (i = 0; i < num; i++) {
            comp = xmlPanel.getComponent(i);
            if (comp instanceof VObjIF) {
                if (comp instanceof VGroup)
                    replaceButtons((Container) comp);
                else {
                    String name = ((VObjIF)comp).getAttribute(VObjDef.PANEL_NAME);
                    if (name != null) {
                        if (name.equals("close"))
                            closeList.add(comp);
                        else if (name.equals("navigator"))
                            navigatorList.add(comp);
                    }
                }
            }
         }
         num = closeList.size();
         k = 0;
         for (i = 0; i < num; i++) {
             comp = closeList.elementAt(i);
             if (comp != null) {
                 add(comp, 0);
                 k++;
             }
         }
         if (k > 0)
             closeBtn.setVisible(false);
         else
             closeBtn.setVisible(true);

         num = navigatorList.size();
         for (i = 0; i < num; i++) {
             comp = navigatorList.elementAt(i);
             if (comp != null)
                 add(comp, 1);
         }
    }

    private boolean createXmlPanel() {

         if (xmlPanel == null) {
             xmlPanel = new JPanel();
             xmlPanel.setOpaque(false);
             xmlPanel.setLayout(new VRubberPanLayout());
         }

         if (closeList == null)
             closeList = new Vector<Component>();
         if (navigatorList == null)
             navigatorList = new Vector<Component>();

         String path = "INTERFACE"+File.separator+xmlFileName;
         path = FileUtil.openPath(path);
         if (path == null)
             return false;
         File fd = new File(path);
         if (oldFilePath != null) {
             if (oldFilePath.equals(path)) {
                 if (fileTime == fd.lastModified())
                     return false;
             }
         }
         xmlPanel.removeAll();

         oldFilePath = path;
         fileTime = fd.lastModified();
         try {
             LayoutBuilder.build(xmlPanel, null, path);
         }  catch(Exception e) {
             Messages.writeStackTrace(e);
         }
         int i, num;
         Component comp;

         num = closeList.size();
         for (i = 0; i < num; i++) {
             comp = closeList.elementAt(i);
             if (comp != null)
                remove(comp);
         }
         closeList.clear();

         num = navigatorList.size();
         for (i = 0; i < num; i++) {
             comp = navigatorList.elementAt(i);
             if (comp != null)
                 remove(comp);
         }
         navigatorList.clear();

         replaceButtons();

         return true;
    }

    private void buildGUi() {

         boolean bVisible = true;

         String lb = Util.getLabel("Close");
         closeBtn = new JButton(lb);
         closeBtn.setActionCommand("close");
         closeBtn.addActionListener(this);
         add(closeBtn); 

         lb = Util.getLabel("_Show_this_Help_Overlay_on_startup", "Show this Help Overlay on Startup");
         showCk = new JCheckBox(lb, true);
         showCk.setActionCommand("show");
         showCk.addActionListener(this);
         showCk.setOpaque(false);
         add(showCk); 

         createXmlPanel();

         add(xmlPanel);

         setLayout(new HelpOverlayLayout());

         if (sshare != null)
             hs = sshare.userInfo();
         if (hs != null) {
             String str = (String)hs.get("overlayHelp");
             if (str != null)
                 bVisible = str.equals("off") ? false : true;
         }
/*
         if (Util.isImagingUser())
             bVisible = false;
         else
         {
            File fd = new File(FileUtil.sysdir() +"/msg/noHelpOverlay");
            if (fd.exists())
               bVisible = false;
         }
 */
         bVisible = false;
         showCk.setSelected(bVisible);

         addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
            }

            public void mousePressed(MouseEvent evt) {
            }

         });

         addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent evt) {
            }

         });

         DisplayOptions.addChangeListener(this);

         setVisible(bVisible);
    }

    public void setVisible(boolean b) {
         super.setVisible(b);
         if (b) {
            updatePanel();
            requestFocus();
         }
    }

    private void saveShowStatus() {
         if (hs == null)
             return;
         if(showCk.isSelected())
             hs.put("overlayHelp", "on");
         else
             hs.put("overlayHelp", "off");
    }

    public void setOption(String  str) {
         if (str == null)
             return;
         String s = str.trim();
         if (s.startsWith("file:")) {
             s = s.substring(5).trim(); 
             if (s.length() > 1) {
                 xmlFileName = s;
                 if (createXmlPanel()) {
                     if (isVisible())
                         updatePanel();
                     revalidate();
                 } 
             }
             return;
         }
    }

    public void actionPerformed(ActionEvent  e) {
        String cmd=e.getActionCommand();

        if(cmd.equals("close")){
            setVisible(false);
            Util.getAppIF().setInputFocus();
            return;
        }
        if(cmd.equals("show")){
            saveShowStatus();
            return;
        }
    }

    public void paint(Graphics g) {
        Graphics2D g2d = (Graphics2D)g;
        Dimension dim = getSize();
        GradientPaint gradient = new GradientPaint(0, 0, bgColor, 0, dim.height, bgColor1);
        g2d.setPaint(gradient);
        g2d.fillRect(0, 0, dim.width, dim.height);
        super.paint(g);
    }

    private class HelpOverlayLayout implements LayoutManager {

       public void addLayoutComponent(String name, Component comp) {}

       public void removeLayoutComponent(Component comp) {}

       public Dimension preferredLayoutSize(Container target) {
           return new Dimension(0, 0);
       }

       public Dimension minimumLayoutSize(Container target) {
           return new Dimension(0, 0);
       }

       public void layoutContainer(Container target) {
           synchronized (target.getTreeLock()) {
              Dimension dim = target.getSize();
              Dimension d = closeBtn.getPreferredSize();
              int i, num;
              Component comp;
              int x = dim.width - d.width - 20; 
              int y = 12;
              closeBtn.setBounds(x, y, d.width, d.height);
              y = d.height + 12;
              xmlPanel.setBounds(2, 2, dim.width - 4, dim.height - d.height - 40);

              d = showCk.getPreferredSize();
              x = 12;
              y = dim.height - d.height - 30;
              showCk.setBounds(x, y, d.width + 20, d.height + 4);

              xmlPanel.setBounds(0, 0, dim.width, y - 6);

              num = closeList.size();
              y = 12;
              for (i = 0; i < num; i++) {
                  comp = closeList.elementAt(i);
                  if (comp != null) {
                      d = comp.getPreferredSize();
                      x = dim.width - d.width - 20;
                      comp.setBounds(x, y, d.width, d.height);
                  }
              }
              num = navigatorList.size();
              for (i = 0; i < num; i++) {
                  comp = navigatorList.elementAt(i);
                  if (comp != null) {
                      d = comp.getPreferredSize();
                      x = dim.width - d.width - 20;
                      y = dim.height - d.height - 20;
                      comp.setBounds(x, y, d.width, d.height);
                  }
              }
                      
           }
       }
    }
}

