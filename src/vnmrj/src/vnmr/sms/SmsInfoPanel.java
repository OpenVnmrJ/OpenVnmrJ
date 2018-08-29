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
import javax.swing.border.*;
import java.io.*;
import java.awt.*;
import java.util.*;
import java.awt.event.*;

import vnmr.util.*;

public  class SmsInfoPanel extends JPanel implements SmsDef
{
   private JPanel menuPan;
   private JTextArea textPan;
   private JScrollPane textScroll;
   private JSeparator separator;
   private JComboBox rackMenu;
   private JComboBox zoneMenu;
   private JComboBox zone2Menu;
   private JComboBox orientMenu;
   private JComboBox orderMenu;
   private JComboBox order2Menu;
   private JLabel  rackLabel;
   private JLabel  rackName;
   private JLabel  zoneLabel;
   private JLabel  orderLabel;
   private JLabel  infoLabel;
   private JLabel  infoLabel2;
   private JLabel  sampleLabel;
   private JRadioButton  gilsonBut;
   private JRadioButton  rotateBut;
   private JButton configBut;
   private JButton submitBut;
   private ZoneListener zoneL;
   private RackListener rackL;
   private OrientListener orientL;
   private OrderListener orderL;
   private Order2Listener order2L;
   private ButtonListener butttonL;
   private RackInfo rackInfo;
   // private Vector rackList = null;
   private RackObj rackList[];
   private RackObj curRack;
   private SmsSample infoObj = null;
   private boolean debug = false;
   private boolean bTest = false;
   private int  orient = BYROW;
   private int  curType = 0;
   private int  stydyCount = 0;
   private Thread configThread;

   public SmsInfoPanel() {
        menuPan = new JPanel();
        textScroll = new JScrollPane();
        textPan = new JTextArea();
        textScroll.setViewportView(textPan);
        separator = new JSeparator(); 
        debug = SmsUtil.getDebugMode();
        bTest = SmsUtil.getTestMode();
        textPan.setEditable(false);
        textPan.setLineWrap(false);
        textPan.setMargin(new Insets(2, 2, 1, 2));

        rackMenu = new JComboBox();
        zoneMenu = new JComboBox();
        zone2Menu = new JComboBox();

        zoneMenu.setOpaque(false);
        rackMenu.setOpaque(false);
        textPan.setOpaque(false);
        menuPan.setOpaque(false);

        butttonL = new ButtonListener();
        configBut = new JButton("Configure Racks");
        configBut.setActionCommand("Configure");
        configBut.addActionListener(butttonL);
        gilsonBut = new JRadioButton("GilsonNumber");
        gilsonBut.setActionCommand("Gilson");
        gilsonBut.setEnabled(false);
        gilsonBut.addActionListener(butttonL);
        submitBut = new JButton("Submit");
        submitBut.setActionCommand("Submit");
        submitBut.addActionListener(butttonL);

        infoLabel = new JLabel("   ");
        infoLabel2 = new JLabel("   ");
        sampleLabel = new JLabel("   ");
        rotateBut = new JRadioButton("Rotate Tray");
        rotateBut.setActionCommand("Rotate");
        rotateBut.addActionListener(butttonL);

        rackL = new RackListener();
        zoneL = new ZoneListener();
        rackMenu.addActionListener(rackL);
        zoneMenu.addActionListener(zoneL);
        zone2Menu.addActionListener(zoneL);
        zone2Menu.addItem("1");
        zone2Menu.addItem("2");
        zone2Menu.addItem("All");

        orientL = new OrientListener();
        orientMenu = new JComboBox();
        orientMenu.addActionListener(orientL);
        orientMenu.addItem("Horizontal");
        orientMenu.addItem("Vertical");

        orderL = new OrderListener();
        orderMenu = new JComboBox();
        orderMenu.addActionListener(orderL);
        orderMenu.addItem("Left & Front");
        orderMenu.addItem("Right & Front");
        orderMenu.addItem("Left & Back");
        orderMenu.addItem("Right & Back");

        order2L = new Order2Listener();
        order2Menu = new JComboBox();
        order2Menu.addActionListener(order2L);
        order2Menu.addItem("Downward only");
        order2Menu.addItem("Upward only");
        order2Menu.addItem("Down & Up");
        order2Menu.addItem("Up & Down");

        rackLabel = new JLabel("Rack:");
        rackName = new JLabel("   ");
        zoneLabel = new JLabel("Zone:");
        orderLabel = new JLabel("Sample Order:");
        rackLabel.setOpaque(false);
        rackName.setOpaque(false);
        sampleLabel.setOpaque(false);

        menuPan.add(configBut);
        menuPan.add(rotateBut);
        menuPan.add(rackLabel);
        menuPan.add(rackName);
        menuPan.add(rackMenu);
        menuPan.add(zoneLabel);
        menuPan.add(zoneMenu);
        menuPan.add(zone2Menu);
        menuPan.add(orderLabel);
        menuPan.add(orientMenu);
        menuPan.add(orderMenu);
        menuPan.add(order2Menu);
        menuPan.add(gilsonBut);
        menuPan.add(infoLabel);
        menuPan.add(infoLabel2);
        setDebugMode(debug);

        orderLabel.setVisible(false); 
        orientMenu.setVisible(false); 
        orderMenu.setVisible(false); 
        order2Menu.setVisible(false); 

        add(menuPan);
        menuPan.setLayout(null);
        if (bTest || debug)
            add(submitBut);

        add(separator);
        add(sampleLabel);
        add(textScroll);
        setLayout(new SmsInfoLayout());
        menuPan.setVisible(false);
        separator.setVisible(false);
        SmsUtil.setInfoPanel(this);

        putClientProperty("backgroundTexture", "none");
        setBorder(new EtchedBorder(EtchedBorder.LOWERED));
   }

   public void setBg(Color c) {
        setBackground(c);
        zoneMenu.setBackground(c);
        zone2Menu.setBackground(c);
        rackMenu.setBackground(c);
        configBut.setBackground(c);
        submitBut.setBackground(c);
        rotateBut.setBackground(c);
        gilsonBut.setBackground(c);
        orderMenu.setBackground(c);
        order2Menu.setBackground(c);
        JViewport vp = textScroll.getViewport();
        vp.setBackground(c);
        textPan.setFont(DisplayOptions.getFont(null, "StdPar", null));
        // sampleLabel.setFont(textPan.getFont());
        SwingUtilities.updateComponentTreeUI(rackMenu);
        SwingUtilities.updateComponentTreeUI(zoneMenu);
        SwingUtilities.updateComponentTreeUI(zone2Menu);
        SwingUtilities.updateComponentTreeUI(orientMenu);
        SwingUtilities.updateComponentTreeUI(orderMenu);
        SwingUtilities.updateComponentTreeUI(order2Menu);
        SwingUtilities.updateComponentTreeUI(separator);
   }

   public void setDebugMode(boolean b) {
        debug = b;
        if (debug) { 
           if (curType == VAST) {
               gilsonBut.setVisible(true); 
               infoLabel.setVisible(true); 
               infoLabel2.setVisible(true); 
           }
        }
        else {
           gilsonBut.setVisible(false); 
           infoLabel.setVisible(false); 
           infoLabel2.setVisible(false); 
           orderLabel.setVisible(false); 
           orientMenu.setVisible(false); 
           orderMenu.setVisible(false); 
           order2Menu.setVisible(false); 
        }
   }

   public void updateZoneMenu(String s) {
        zoneMenu.removeAllItems();
        gilsonBut.setEnabled(false);
        if (s == null || rackList == null)
            return;
/*
        GrkPlate plate = null;
        for (int k = 0; k < rackList.size(); k++) {
            plate = (GrkPlate)rackList.elementAt(k);
            if (s.equals(plate.name))
                break;
            plate = null;
        }
        if (plate == null)
            return;
*/
        curRack = null;
        for (int k = 0; k < rackList.length; k++) {
            curRack = (RackObj)rackList[k];
            if (curRack != null && s.equals(curRack.idStr))
                break;
            curRack = null;
        }
        if (curRack == null)
            return;

        if (curRack.zones > 1) {
            if (curRack.trayType == GRK97)
                zoneMenu.addItem("All");
        }
        for (int n = 1; n <= curRack.zones; n++) {
            zoneMenu.addItem(Integer.toString(n));
        }
        if (curRack.zones > 1) {
            if (curRack.trayType != GRK97)
                zoneMenu.addItem("All");
        }
        String str = "Rack: "+curRack.name+"   Pattern: "+curRack.pattern+"   Start: "+curRack.start;
        infoLabel.setText(str);
        rotateBut.setSelected(curRack.rotated);
        rackName.setText(curRack.name);
        gilsonBut.setSelected(curRack.gilsonNumber);
        if (curRack.name.equals("205") || curRack.name.equals("505"))
            gilsonBut.setEnabled(true);
        if (curRack.name.equals("205h") || curRack.name.equals("505h"))
            gilsonBut.setEnabled(true);
   }

   public void setRackMenu(String s) {
        if (s == null)
            return;
        rackMenu.setSelectedItem(s);
   }

   public void setZoneMenu(String s) {
        if (s == null)
            return;
        zoneMenu.setSelectedItem(s);
   }

   public void updateRackMenu() {
        rackInfo.setNewInfo(false);
        rackList = rackInfo.getRackList();
        rackMenu.removeAllItems();
        for (int k = 0; k < rackList.length; k++) {
            curRack = (RackObj)rackList[k];
            if (curRack != null && curRack.name != null)
            if (curRack != null && curRack.name != null && curRack.zones > 0)
                rackMenu.addItem(curRack.idStr);
        }
        String str = (String) rackMenu.getSelectedItem();
        updateZoneMenu(str);
   }

   public void setZoneInfo(String s) {
        if (curRack == null)
           return;
        int a = 0;
        try {
          a = Integer.parseInt(s);
        }
        catch (NumberFormatException er) { return; }
        if (a < 1 || curRack.zones < a)
           return;
        String str = "Zone: "+s+"    Rows: "+curRack.rows[a]+"   Cols: "+curRack.cols[a]+"   Wells: "+curRack.numbers[a];
        infoLabel2.setText(str);
   }


   public void updateRackInfo() {
        if (configThread != null) {
            if (configThread.isAlive()) {
                try {
                   configThread.join(100);
                }
                catch (InterruptedException e) {}
            }
        }
        rackInfo.update();
        updateRackMenu();
   }

   public void setType(int n) {
        curType = n;
        if (n == CAROSOL || n == SMS || n == GRK12) {
             menuPan.setVisible(false); 
             separator.setVisible(false); 
             return;
        }
        if (n == SMS100) {
             configBut.setVisible(false); 
             rotateBut.setVisible(false); 
             rackLabel.setVisible(false); 
             rackName.setVisible(false); 
             rackMenu.setVisible(false); 
             orderLabel.setVisible(false); 
             gilsonBut.setVisible(false); 
             infoLabel.setVisible(false); 
             infoLabel2.setVisible(false); 
             orientMenu.setVisible(false); 
             orderMenu.setVisible(false); 
             order2Menu.setVisible(false); 
             zoneMenu.setVisible(false); 
             zone2Menu.setVisible(true); 
             menuPan.setVisible(true);
             separator.setVisible(true); 
             return;
        }
        if (n == VAST) {
             zone2Menu.setVisible(false); 
             rackLabel.setVisible(true); 
             rackName.setVisible(true); 
             rackMenu.setVisible(true); 
             configBut.setVisible(true); 
             rotateBut.setVisible(true); 
             setDebugMode(debug);
             zoneMenu.setVisible(true); 
             menuPan.setVisible(true);
             separator.setVisible(true); 
             if (rackInfo == null) 
                rackInfo = SmsUtil.getRackInfo();
             if (rackInfo != null) {
                if (rackInfo.isNewInfo())
                    updateRackMenu();
             }
             return;
        }
        if (n == GRK49 || n == GRK97) {
             zone2Menu.setVisible(false); 
             rackLabel.setVisible(false); 
             rackName.setVisible(false); 
             rackMenu.setVisible(false); 
             configBut.setVisible(false); 
             rotateBut.setVisible(false); 
             setDebugMode(debug);
             zoneMenu.setVisible(true); 
             menuPan.setVisible(true);
             separator.setVisible(true); 
             if (rackInfo == null) 
                rackInfo = SmsUtil.getRackInfo();
             if (rackInfo != null) {
                if (rackInfo.isNewInfo())
                    updateRackMenu();
             }
             return;
        }

   }

   public void showInfo(SampleInfo info, boolean multi) {
        if (info == null || !info.isValidated())
            return;
        if(multi) {
            textPan.append("< Study  "+stydyCount+" >");
        }
        stydyCount++;
        textPan.append("\nStudyid:    ");
        textPan.append(info.studyId);
        textPan.append("\nOperator:   ");
        textPan.append(info.user);
        textPan.append("\nSamplename: ");
        textPan.append(info.name);
        textPan.append("\nSolvent:    ");
        textPan.append(info.solvent);
        textPan.append("\nNotebook:   ");
        textPan.append(info.notebook);
        if (info.barcode != null && info.barcode.length() > 0) {
           textPan.append("\nBarcode:    ");
           textPan.append(info.barcode);
        }
        textPan.append("\nPage:       ");
        textPan.append(info.page);

        textPan.append("\nDate:       ");
        textPan.append(info.date);
        textPan.append("\nStatus:     ");
        textPan.append(info.statusStr);
        textPan.append("\n\n");
   }

   public void showSampleInfo(SmsSample obj) {
        textPan.setText("");
        infoObj = obj;
        if (obj == null) {
            sampleLabel.setText("  ");
            return;
        }
        boolean multiStudy = false;
        String str = " Sample#: "+obj.idStr+"   ( "+obj.studyNum;
        if (obj.studyNum > 1) {
           multiStudy = true;
           str = str + " studies )";
        }
        else
           str = str + " study )";
        sampleLabel.setText(str);
/*
        textPan.append(" Sample#: ");
        textPan.append(obj.idStr);
        if (obj.rackId > 0) {
            textPan.append("Rack:   ");
            textPan.append(Integer.toString(obj.rackId));
            textPan.append("\nZone:   ");
            textPan.append(Integer.toString(obj.zoneId));
        }
*/
        if (curType != GRK12 && (obj.row > 0)) {
            textPan.append("Row:    ");
            if (obj.rowStr != null)
                textPan.append(obj.rowStr);
            else
                textPan.append(Integer.toString(obj.row));
            textPan.append("\nCol:    ");
            if (obj.colStr != null)
                textPan.append(obj.colStr);
            else
                textPan.append(Integer.toString(obj.col));
            textPan.append("\n");
        }
        if (obj.status == OPEN) {
            textPan.append("\nStatus:  empty");
            return;
        }
        Vector<SampleInfo> v = obj.getInfoList();
        if (v == null)
            return;
        int m = v.size();
        SampleInfo fobj;
        stydyCount = 1;
        for (int n = 0; n < m; n++) {
            fobj = v.elementAt(n);
            showInfo(fobj, multiStudy);
        }
   }

   public void showSampleInfo() {
        showSampleInfo(infoObj);
   }

   public void runConfigure() {
        if (configThread != null && configThread.isAlive()) {
            return;
        }
        ConfigProcess process = new ConfigProcess(this);
        configThread = new Thread(process);
        configThread.start(); 
   }

   public void setGilsonNum(boolean b) {
        if (curRack != null) {
           curRack.setGilsonNum(b);
           showSampleInfo(infoObj);
        }
   }

   public void setRotate(boolean b) {
        if (curRack != null) {
           curRack.setRotate(b);
           showSampleInfo(infoObj);
        }
   }

   public void testSubmit() {
        SmsUtil.submitTest();
   }

   private class RackListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JComboBox cb = (JComboBox) e.getSource();
            if (cb == null)
               return;
            String str = (String) cb.getSelectedItem();
            if (str == null)
               return;
            Util.getActiveView().sendToVnmr("vrack="+str);
            SmsUtil.showGrkPlate(str);
            showSampleInfo(null);
        }
   }

   private class ZoneListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JComboBox cb = (JComboBox) e.getSource();
            if (cb == null)
               return;
            String str = (String) cb.getSelectedItem();
            if (str == null)
               return;
            SmsUtil.setZone(str);
            // textPan.setText("");
            if (str.compareToIgnoreCase("All")!=0)
                Util.getActiveView().sendToVnmr("vzone="+str);
            showSampleInfo(null);
            setZoneInfo(str);
        }
   }

   private class OrientListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JComboBox cb = (JComboBox) e.getSource();
            int n = cb.getSelectedIndex() + 1;
            SmsUtil.setOrient(n);
            if (n != orient) {
                 orient = n;
                 if (orient == BYROW) {
                     order2Menu.setVisible(false);
                     orderMenu.setVisible(true);
                 }
                 else {
                     orderMenu.setVisible(false);
                     order2Menu.setVisible(true);
                 }
            }
        }
   }

   private class OrderListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JComboBox cb = (JComboBox) e.getSource();
            int n = cb.getSelectedIndex() + 1;
            SmsUtil.setOrder(n);
        }
   }

   private class Order2Listener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            JComboBox cb = (JComboBox) e.getSource();
            int n = cb.getSelectedIndex() + 1;
            SmsUtil.setOrder2(n);
        }
   }

   private class ButtonListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            boolean flag;
            String cmd = e.getActionCommand();
            if (cmd.equals("Configure"))
                runConfigure();
            if (cmd.equals("Rotate")) {
                flag = ((JRadioButton)e.getSource()).isSelected();
                setRotate(flag);
            }
            if (cmd.equals("Gilson")) {
                flag = ((JRadioButton)e.getSource()).isSelected();
                setGilsonNum(flag);
            }
            if (cmd.equals("Submit")) {
                testSubmit();
            }
        }
   }


   private class SmsInfoLayout implements LayoutManager {
        private int width;
        private int height;
        private int px;
        private int py;

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        private void layoutMenuPan() {
            Dimension d1, d2;
            int h = 0;
            int xw = 0;
            int y = 0;
            int x = px + 2;
             if (zone2Menu.isVisible()) {
                 py = py + 4;
                 d1 = zoneLabel.getPreferredSize();
                 d2 = zone2Menu.getPreferredSize();
                 if (d1.height > h)
                      h = d1.height;
                 if (d2.height > h)
                      h = d2.height;
                 if (d1.height < h)
                      y = py + (h - d1.height) / 2;
                 zoneLabel.setBounds(x, y, d1.width, d1.height);
                 zone2Menu.setBounds(d1.width + 8, py, d2.width, d2.height);
                 py = py + h + 4;
                 menuPan.setBounds(0, 0, width, py);
                 return;
             }
             if (configBut.isVisible()) {
                 py = py + 4;
                 d1 = configBut.getPreferredSize();
                 d2 = rotateBut.getPreferredSize();
                 h = d1.height;
                 if (d2.height > h)
                     h = d2.height;
                 y = py;
                 if (d1.height < h)
                     y = py + (h - d1.height) / 2;
                 configBut.setBounds(x, y, d1.width, d1.height);
                 x = x + d1.width + 8;
                 y = py;
                 if (d2.height < h)
                     y = py + (h - d2.height) / 2;
                 rotateBut.setBounds(x, y, d2.width, d2.height);
                 py = py + h + 2;
             }
             if (rackMenu.isVisible()) {
                 d1 = rackLabel.getPreferredSize();
                 xw = d1.width;
             }
             if (zoneMenu.isVisible()) {
                 d1 = zoneLabel.getPreferredSize();
                 if (d1.width > xw)
                    xw = d1.width;
             }
             if (orderLabel.isVisible()) {
                 d1 = orderLabel.getPreferredSize();
                 if (d1.width > xw)
                    xw = d1.width;
             }
             xw += 8;
             if (rackMenu.isVisible()) {
                 d1 = rackLabel.getPreferredSize();
                 d2 = rackMenu.getPreferredSize();
                 // w = d1.width + d2.width;
                 x = px + 4;
                 h = d1.height;
                 if (d2.height > h)
                     h = d2.height;
                 y = py;
                 if (d1.height < h)
                     y = py + (h - d1.height) / 2;
                 rackLabel.setBounds(x, y, d1.width, d1.height);
                 x = x + d1.width + 4;
                 d1 = rackName.getPreferredSize();
                 rackName.setBounds(x, y, d1.width, d1.height);
                 x = x + d1.width + 6;
                 y = py;
                 if (d2.height < h)
                     y = py + (h - d2.height) / 2;
                 rackMenu.setBounds(x, y, d2.width, d2.height);
                 x = x + d2.width + 10;
                 // py = py + h;
                 // px = px + w + 4;
                 d1 = zoneLabel.getPreferredSize();
                 d2 = zoneMenu.getPreferredSize();
                 // w = d1.width + d2.width + px;
                 h = d1.height;
                 if (d2.height > h)
                      h = d2.height;
                 y = py;
                 if (d1.height < h)
                      y = py + (h - d1.height) / 2;
                 zoneLabel.setBounds(x, y, d1.width, d1.height);
                 x = x + d1.width + 6;
                 y = py;
                 if (d2.height < h)
                      y = py + (h - d2.height) / 2;
                 zoneMenu.setBounds(x, y, d2.width, d2.height);
                 // px = px + w + 2;
                 py = py + h;
             }
             else if (zoneMenu.isVisible()) {
                 py = py + 4;
                 x = px + 4;
                 d1 = zoneLabel.getPreferredSize();
                 d2 = zoneMenu.getPreferredSize();
                 h = d1.height;
                 if (d2.height > h)
                      h = d2.height;
                 y = py;
                 if (d1.height < h)
                      y = py + (h - d1.height) / 2;
                 zoneLabel.setBounds(x, y, d1.width, d1.height);
                 x = x + d1.width + 6;
                 y = py;
                 if (d2.height < h)
                      y = py + (h - d2.height) / 2;
                 zoneMenu.setBounds(x, y, d2.width, d2.height);
                 py = py + h;
             }
             if (orderLabel.isVisible()) {
                 d1 = orderLabel.getPreferredSize();
                 d2 = orientMenu.getPreferredSize();
                 x = px + 2;
                 h = d1.height;
                 if (d2.height > h)
                        h = d2.height;
                 y = py;
                 if (d1.height < h)
                      y = py + (h - d1.height) / 2;
                 orderLabel.setBounds(x, y, d1.width, d1.height);
                 orientMenu.setBounds(px + xw, py, d2.width, d2.height);
                 px = px + d2.width + 2;
                 if (orderMenu.isVisible()) {
                    d2 = orderMenu.getPreferredSize();
                    orderMenu.setBounds(px + xw, py, d2.width, d2.height);
                    px = px + d2.width + 2;
                 }
                 if (order2Menu.isVisible()) {
                    d2 = order2Menu.getPreferredSize();
                    order2Menu.setBounds(px + xw, py, d2.width, d2.height);
                 }
                 py = py + h;
             }
             if (infoLabel.isVisible()) {
                 d1 = gilsonBut.getPreferredSize();
                 d2 = infoLabel.getPreferredSize();
                 x = px + 2;
                 gilsonBut.setBounds(x, py, d1.width, d1.height);
                 x = px + 4;
                 py = py + d1.height;
                 infoLabel.setBounds(x, py, d2.width, d2.height);
                 py = py + d2.height;
                 d1 = infoLabel2.getPreferredSize();
                 infoLabel2.setBounds(x, py, d1.width, d1.height);
                 py = py + d1.height;
             }
             py = py + 4;
             menuPan.setBounds(0, 0, width, py);
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim = target.getSize();
                width = dim.width;
                height = dim.height;
                px = 0;
                py = 2;
                if (menuPan.isVisible()) {
                    py = 0;
                    layoutMenuPan();
                    separator.setBounds(1, py, width-3, 2);
                    py += 2;
                }
                if (bTest || debug) {
                    Dimension d1 = submitBut.getPreferredSize();
                    submitBut.setBounds(4, py, d1.width, d1.height);
                    py = py + d1.height;
                    separator.setBounds(1, py, width-3, 2);
                    py += 2;
                }
                dim = sampleLabel.getPreferredSize();
                sampleLabel.setBounds(0, py, width, dim.height+4);
                py = py + dim.height + 6;
                textScroll.setBounds(0, py, width, height - py);
            }
        }
   } //  SmsInfoLayout

   private class ConfigProcess implements Runnable {
       SmsInfoPanel listener;

       public ConfigProcess(SmsInfoPanel p) {
           this.listener = p;
       }

       public void run() {
          Process proc = null;
          try {
              String addr = SmsUtil.getVnmrAddr();
/*
              if (addr == null)
                   return;
*/
              String cmdarray[] = new String[4];
              cmdarray[0] = "vnmr_gilson";
              cmdarray[1] = "panel2";
              cmdarray[2] = "\""+addr+"\"";
              cmdarray[3] = "fg";
              Runtime rt = Runtime.getRuntime();
              proc = rt.exec(cmdarray);
              proc.waitFor();
              listener.updateRackInfo();
          }
          catch (IOException e) {
              System.out.println(e);
          }
          catch (InterruptedException ex)
          {
              System.out.println(ex);
          }
          finally {
             try {
                 if (proc != null) {
                    OutputStream os = proc.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = proc.getInputStream();
                    if(is != null)
                        is.close();
                    is = proc.getErrorStream();
                    if(is != null)
                        is.close();
                 }
              }
              catch (Exception ex) {
                  Messages.writeStackTrace(ex);
              }
           }
       }
   }
}

