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
import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;

import vnmr.util.*;
import vnmr.templates.LayoutBuilder;
import vnmr.bo.VTrayColor;
import vnmr.bo.VColorChooser;
import vnmr.bo.VObjDef;

public class SmsColorEditor extends ModelessDialog
   implements SmsDef, ActionListener
{
   static int   maxStatusNo = 7;

   private static String srcName ="TrayLocColor.xml";
   private static String propertyName ="TrayLocColors";
   private JScrollPane scrollPane;
   private JPanel  colorPanel;
   private JPanel  tmpPanel;
   private String  srcFilePath;
   private ButtonIF vnmrIf;
   private JPanel headerPanel;
   private JLabel nameHeader;
   private JLabel colorHeader;
   private JLabel tipHeader;
   private Vector <VTrayColor> colorList;
   private long srcFileDate;

   public static String[] statusDefs;
   public static String[] tooltips;
   public static Color[]  statusColors;

   public SmsColorEditor(ButtonIF vif) {
      super("Tray Color Editor");
      String title = Util.getLabel("_Tray_Color_Editor","Tray Color Editor");
      this.setTitle(title);
      this.vnmrIf = vif;
      this.tmpPanel = new JPanel();
      this.colorPanel = new JPanel();
      this.colorPanel.setLayout(new SimpleVLayout(4, 4,2, 4,false));
      scrollPane = new JScrollPane(colorPanel,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);

      Container c = getContentPane();
      c.add(scrollPane, BorderLayout.CENTER);
      historyButton.setVisible(false);
      undoButton.setVisible(false);
      closeButton.setActionCommand("close");
      closeButton.addActionListener(this);
      abandonButton.setActionCommand("abandon");
      abandonButton.addActionListener(this);

      buildUi();
      updateColors();

      setLocation(200, 100);
   }


   private boolean buildUi() {
       boolean bNewFile = false;
       String filePath = FileUtil.openPath("INTERFACE"+File.separator+srcName);
       if (filePath == null)
           return bNewFile;
       File newFd = new File(filePath);
       if (newFd.lastModified() != srcFileDate)
           bNewFile = true;
       if (!filePath.equals(srcFilePath))
           bNewFile = true;
       if (!bNewFile)
           return bNewFile;
 
       srcFileDate = newFd.lastModified();
       srcFilePath = filePath;
       createHeader();
       colorPanel.removeAll();
       tmpPanel.removeAll();
       try {
            LayoutBuilder.build(tmpPanel, vnmrIf, filePath);
       }  catch(Exception e) {
            Messages.writeStackTrace(e);
       }
       if (colorList == null)
           colorList = new Vector<VTrayColor>();
       else
           colorList.clear();
       addColorObjs(true);
       filePath = "USER"+File.separator+"PERSISTENCE"+File.separator+propertyName;
       filePath = FileUtil.openPath(filePath);
       if (filePath != null) {
           try {
                LayoutBuilder.build(tmpPanel, vnmrIf, filePath);
           }  catch(Exception e) {
                Messages.writeStackTrace(e);
           }
           addColorObjs(false);
       }
       layoutColorObjs();
       return true;
   }
   
   public void setVisible(boolean s) {
        if (s) {
            if (buildUi())
               updateColors();
        }
        super.setVisible(s);
   }

   public void execCmd(String cmd) {
       if (cmd == null)
           return;
       boolean bOpen = true;
       String cmdStr = cmd.toLowerCase();
       if (cmdStr.equalsIgnoreCase("close"))
           bOpen = false;
       else if (cmdStr.equalsIgnoreCase("off"))
           bOpen = false;
       setVisible(bOpen);
   }

   public static int getStatusNo(String s) {
       if (s == null)
           return OPEN;
       String[] status = STATUS_STR;
       if (statusDefs != null)
           status = statusDefs;
           
       for (int i = 0; i < maxStatusNo; i++) {
           if (s.equalsIgnoreCase(status[i]))
              return i;
       }
       return OPEN;
   }

   public static String getTooltip(int n) {
       if (n >= 0 && n < maxStatusNo)
          return tooltips[n];
       return null;
   }

   public static Color getColor(int n) {
       if (n >= 0 && n < maxStatusNo)
          return statusColors[n];
       return Color.gray;
   }

   private void saveColors() {
       int nmembers = colorPanel.getComponentCount();
       if (nmembers < 2)
          return;
       String filePath = "USER"+File.separator+"PERSISTENCE"+File.separator+propertyName;
       filePath = FileUtil.savePath(filePath, false);
       if (filePath == null)
           return;

       LayoutBuilder.writeToFile(colorPanel, filePath);
   }

   private void updateColors() {
       int i, num, k;
       VTrayColor vc;

       if (colorList == null)
          num = STATUS_NUM;
       else
          num = colorList.size() + STATUS_NUM;
       maxStatusNo = STATUS_NUM;
       statusDefs = new String[num];
       tooltips = new String[num];
       statusColors = new Color[num];

       for (i = 0; i < STATUS_NUM; i++) {
           statusDefs[i] = STATUS_STR[i];
           statusColors[i] = STATUS_COLOR[i];
           tooltips[i] = STATUS_STR[i];
       }
       for (i = STATUS_NUM; i < num; i++) {
           tooltips[i] = null;
           statusColors[i] = Color.gray;
       }
       if (colorList == null)
           return;
       for (i = 0; i < colorList.size(); i++) {
           vc = colorList.elementAt(i);
           String name = vc.getAttribute(VObjDef.PANEL_NAME);
           int newId = -1;
           for (k = 0; k < maxStatusNo; k++) {
                if (name.equalsIgnoreCase(statusDefs[k])) {
                    newId = k;
                    break;
                }
           }
           String tip = vc.getAttribute(VObjDef.TOOLTIP);
           if (tip == null)
               tip = vc.getAttribute(VObjDef.TOOL_TIP);
           if (newId < 0) {
               newId = maxStatusNo;
               maxStatusNo++;
               statusDefs[newId] = name;
           }
           if (tip != null && tip.length() > 0)
               tooltips[newId] = tip;
           else
               tooltips[newId] = null;
           VColorChooser chooser = vc.getColorChooser();
           if (chooser != null) {
               name = chooser.getAttribute(VObjDef.VALUE);
               if (name != null)
                  statusColors[newId] = DisplayOptions.getColor(name);
           }
       }
   }

   public void actionPerformed(ActionEvent e) {
       String cmd = e.getActionCommand();
       Object obj = e.getSource();

       if (obj instanceof JTextField) { // tooltip
           updateColors();
           SmsUtil.repaintTray();
           return;
       }

       if (cmd == null)
           return;
       if (cmd.equals("menu") || cmd.equals("button")) { // from VColorChooser
           updateColors();
           SmsUtil.repaintTray();
           return;
       }
       if (cmd.equals("close")) {
           saveColors();
           updateColors();
           setVisible(false);
           SmsUtil.repaintTray();
           return;
       }
       if (cmd.equals("abandon")) {
           srcFileDate = 0;
           buildUi();
           updateColors();
           setVisible(false);
           SmsUtil.repaintTray();
           return;
       }
   }

   private void layoutColorObjs() {
       VTrayColor vc;
       int w1, w3, h0, h1, i;

       colorPanel.removeAll();
       colorPanel.add(headerPanel);
       for (i = 0; i < colorList.size(); i++)
            colorPanel.add(colorList.elementAt(i));
       Dimension dim = nameHeader.getPreferredSize();
       w1 = dim.width;
       h1 = dim.height;
       h0 = h1;
       w3 = 0;
       for (i = 0; i < colorList.size(); i++) {
           vc = colorList.elementAt(i);
           dim = vc.getItemPreferredSize(1);
           if (dim.width > w1)
               w1 = dim.width;
           if (dim.height > h1)
               h1 = dim.height;
           dim = vc.getItemPreferredSize(3);
           if (dim.width > w3)
               w3 = dim.width;
           if (dim.height > h1)
               h1 = dim.height;
           VColorChooser chooser = vc.getColorChooser();
           if (chooser != null)
               chooser.addActionListener(this); 
           if (vc.tipText != null)
               vc.tipText.addActionListener(this); 
       }
       w1 += 6;
       nameHeader.setPreferredSize(new Dimension(w1, h0));
       tipHeader.setPreferredSize(new Dimension(40, h0));
       headerPanel.setPreferredSize(new Dimension(w1+40, h0));
       for (i = 0; i < colorList.size(); i++) {
           vc = colorList.elementAt(i);
           vc.setItemPreferredSize(1, new Dimension(w1, h1));
       }
       if (w3 > 1)
           colorHeader.setPreferredSize(new Dimension(w3, h0));
       dim = colorPanel.getPreferredSize();
       w1 = w1 + w3 + 20;
       colorPanel.setPreferredSize(new Dimension(w1, dim.height));
       h1 = dim.height + 90;
       if (h1 > 800)
           h1 = 800;
       w1 = w1 + 200;
       setSize(w1, h1);
   }

   private void replaceObj(VTrayColor obj) {
       for (int i = 0; i < colorList.size(); i++) {
            VTrayColor vobj = colorList.elementAt(i);
            String name0 = vobj.getAttribute(VObjDef.PANEL_NAME);
            String name1 = obj.getAttribute(VObjDef.PANEL_NAME);
            if (name0 != null && (name0.equals(name1))) {
                colorList.setElementAt(obj, i);
                break;
            }
       }
   }

   private void deepSearch(Container pObj, boolean bSystem) {
       if (pObj == null)
           return;
       int nmembers = pObj.getComponentCount();
       for (int i = 0 ; i < nmembers; i++) {
           Component comp = pObj.getComponent(i);
           if (comp instanceof VTrayColor) {
               VTrayColor vc = (VTrayColor) comp;
               String name = vc.getAttribute(VObjDef.PANEL_NAME);
               if (name != null) {
                   if (bSystem)
                       colorList.add(vc);
                   else
                       replaceObj(vc);
               }
           }
           else if (comp instanceof Container)
               deepSearch((Container)comp, bSystem);
       }
   }

   private void addColorObjs(boolean bSystem) {
        int nmembers = tmpPanel.getComponentCount();
        Component comp;
        int i;

        for (i = 0 ; i < nmembers; i++) {
            comp = tmpPanel.getComponent(i);
            if (comp instanceof VTrayColor) {
                VTrayColor vc = (VTrayColor) comp;
                String name = vc.getAttribute(VObjDef.PANEL_NAME);
                if (name != null) {
                    if (bSystem)
                        colorList.add(vc);
                    else
                        replaceObj(vc);
                }
            }
            else if (comp instanceof Container)
                deepSearch((Container)comp, bSystem);
        }
        tmpPanel.removeAll();
   }

   private void createHeader() {
        if (headerPanel != null)
            return;
        Font ft = new Font(Font.SANS_SERIF, Font.BOLD | Font.ITALIC, 12);
        headerPanel = new JPanel();
        headerPanel.setLayout(new BorderLayout());
        nameHeader = new JLabel( Util.getLabel("Name")); 
        nameHeader.setFont(ft);
        tipHeader = new JLabel( Util.getLabel("Tooltip")); 
        tipHeader.setFont(ft);
        colorHeader = new JLabel( Util.getLabel("Color")); 
        colorHeader.setFont(ft);
        headerPanel.add(colorHeader, BorderLayout.EAST);
        headerPanel.add(nameHeader, BorderLayout.WEST);
        headerPanel.add(tipHeader, BorderLayout.CENTER);
        nameHeader.setHorizontalAlignment(SwingConstants.CENTER);
        tipHeader.setHorizontalAlignment(SwingConstants.CENTER);
        colorHeader.setHorizontalAlignment(SwingConstants.CENTER);
   }
}

