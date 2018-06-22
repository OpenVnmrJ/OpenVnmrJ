/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package accounting;

import java.awt.*;
import java.awt.event.*;
import java.awt.print.*;
import java.text.*;
import java.util.*;
import java.io.*;
import javax.swing.*;
import javax.swing.border.*;

//public class Billing extends JSplitPane{
public class Billing extends JPanel{
    
    AccountInfo2 ai;
    Billing billing;
    Date sDate = new Date();
    Date eDate = new Date();
    JButton displayOne, printOne;
    JButton displayOneSum;
    static JButton displayAll, displayAllSum;
    JButton printAll;
    AccountJCB accJCB;
    JLabel accL, nameL, addressL, address2L, cityL, stateL, countryL;
    JLabel nameTF,addressTF, address2TF, cityTF, stateTF, countryTF;
    // JLabel startOfBilling, endOfBilling;
    DateChooser startOfBilling, endOfBilling;
    static JPanel billPanel;
    JPanel hostPanel, selectPanel;
    JPanel p1,p2,p3,p4;
    JSeparator js1,js2,js3;
    JSeparator js_p4;
    DateChooser dateC;
    String [] accounts;
    AccountingTab  billJsp;
    PrinterJob pj = null;
    boolean doS = false;
    boolean doSCsv = false;
    boolean doUCsv = false;
    String saveDir = null;
    Date iStartDate = null;
    Date iEndDate = null;
    
    public Billing(File accountsDir, AccountingTab jsp) {
        billing=this;
        billJsp = jsp;
        
        setLayout(new BorderLayout());
//    setOrientation(JSplitPane.VERTICAL_SPLIT);
        setSize(400,800);
        Font f = new Font("Ariel",Font.PLAIN,12);
        
        
//    hostPanel = new HostSelect();
//    this.setTopComponent(hostPanel);
        
        selectPanel = new JPanel( new BillingLayout() );
        selectPanel.setSize(400,600);
        
        p1 = new JPanel( new P1Layout() );
//        Color c = new Color(200,230,175);
//        Border b1 = BorderFactory.createLineBorder( p1.getBackground().darker(), 3);
        Border b1 = BorderFactory.createEtchedBorder();
//        Border b1 = BorderFactory.createLineBorder(c, 3);
        Border b = BorderFactory.createTitledBorder(b1, "Account");
        p1.setBorder(b);
        accL = new JLabel("Account: ");
        p1.add(accL);
        accL.setFont(f);
        accJCB = new AccountJCB(accountsDir, false);
        accJCB.setSelectedIndex(-1);
        accJCB.addActionListener( new JCBEar() );
        p1.add(accJCB);
        selectPanel.add(p1);
        
//        js1 = new JSeparator();                                selectPanel.add(js1);
        
        p2 = new JPanel( new P2Layout() );
        b = BorderFactory.createTitledBorder(b1, "Invoice address");
        p2.setBorder(b);
        nameL = new JLabel("Name: ");
        nameL.setFont(f);
        nameL.setHorizontalAlignment(SwingConstants.RIGHT);    p2.add(nameL);
        nameTF = new JLabel();                                 p2.add(nameTF);
        nameTF.setFont(f);
        addressL = new JLabel("Address: ");
        addressL.setFont(f);
        addressL.setHorizontalAlignment(SwingConstants.RIGHT); p2.add(addressL);
        addressTF = new JLabel();                              p2.add(addressTF);
        addressTF.setFont(f);
        address2TF = new JLabel();                             p2.add(address2TF);
        address2TF.setFont(f);
        cityL = new JLabel("City: ");
        cityL.setFont(f);
        cityL.setHorizontalAlignment(SwingConstants.RIGHT);    p2.add(cityL);
        cityTF = new JLabel();                                 p2.add(cityTF);
        cityTF.setFont(f);
        stateL = new JLabel("State: ");
        stateL.setFont(f);
        stateL.setHorizontalAlignment(SwingConstants.RIGHT);   p2.add(stateL);
        stateTF = new JLabel();                                p2.add(stateTF);
        stateTF.setFont(f);
        countryL = new JLabel("Country: ");
        countryL.setFont(f);
        countryL.setHorizontalAlignment(SwingConstants.RIGHT);    p2.add(countryL);
        countryTF = new JLabel();                                 p2.add(countryTF);
        countryTF.setFont(f);

        selectPanel.add(p2);
        
//        js2 = new JSeparator();                                selectPanel.add(js2);
        
        p3 = new JPanel( new P3Layout() );
        b = BorderFactory.createTitledBorder(b1, "Invoice dates");
        p3.setBorder(b);
        startOfBilling = new DateChooser("Start Date:");       p3.add(startOfBilling);
        endOfBilling   = new DateChooser("End Date:");         p3.add(endOfBilling);
        selectPanel.add(p3);
        
//        js3 = new JSeparator();                                selectPanel.add(js3);
        
        p4 = new JPanel( new P4Layout() );
        // b = BorderFactory.createTitledBorder(b1, "Display or Print");
        b = BorderFactory.createTitledBorder(b1, "Display");
        p4.setBorder(b);
        BillEar be = new BillEar();
        displayOne = new JButton("Display this invoice");
        displayOne.setActionCommand("DisplayOne");
        p4.add(displayOne);
        displayOne.addActionListener(be);

        displayOneSum = new JButton("Display this summary");
        displayOneSum.setActionCommand("DisplayOneSum");
        p4.add(displayOneSum);
        displayOneSum.addActionListener(be);

        js_p4 = new JSeparator();
        p4.add(js_p4);

        displayAll = new JButton("Display all invoice");
        displayAll.setActionCommand("DisplayAll");
        p4.add(displayAll);
        displayAll.addActionListener(be);

        displayAllSum = new JButton("Display all summary");
        displayAllSum.setActionCommand("DisplayAllSum");
        p4.add(displayAllSum);
        displayAllSum.addActionListener(be);

        printOne = new JButton("Print this invoice");
        // p4.add(printOne);
        printOne.addActionListener( new PrintBill() );
        printAll = new JButton("Print all invoices");
        // p4.add(printAll);
        printAll.addActionListener( be );

        selectPanel.add(p4);
        
//    this.setBottomComponent(selectPanel);
        add(selectPanel);
    }
    
    AccountInfo2 getAccountInfo() {
        return (ai);
    }

    public void acctOutput(Date startDate, Date endDate, String svDir,
                         boolean summary, boolean sumCsv, boolean userCsv) {
        saveDir = svDir;
        doS = summary;
        doSCsv = sumCsv;
        doUCsv = userCsv;
        iStartDate = startDate;
        iEndDate = endDate;
        if (userCsv)
           displayOne.doClick();
        if (sumCsv || summary)
           displayAllSum.doClick();
        
    }

    private void updateAccountInfo() {
            ai = accJCB.read();
            nameTF.setText(ai.name());
            addressTF.setText(ai.address1());
            address2TF.setText(ai.address2());
            cityTF.setText(ai.city());
            stateTF.setText(ai.zip());
            countryTF.setText(ai.country());
    }

    private void displayBill(String type) {
         boolean bOneUser = true;
         boolean bSum = false;

         if (type.startsWith("DisplayOne")) {
             String name = (String)accJCB.getSelectedItem();
             if (name == null || (name.length() == 0)) {
                 if ( ! doUCsv ) {
                 JOptionPane pane = new JOptionPane();
                 pane.showMessageDialog(billing, "Please choose an account first");
                 return;
                 }
             }
             if (type.equals("DisplayOneSum"))
                 bSum = true;
         }
         else {
             bOneUser = false;
             if (type.equals("DisplayAllSum"))
                 bSum = true;
         }
         if (iStartDate != null)
            sDate = iStartDate;
         else
            sDate = startOfBilling.getDate();
         if (iEndDate != null)
            eDate = iEndDate;
         else
            eDate = endOfBilling.getDate();
         ReadLast readLast = ReadLast.getAndCheckInstance(ai);
         if (!readLast.isGoodRecords())
               return;
         readLast.startDate(sDate);
         readLast.endDate(eDate);

         // billPanel = new Bill(ai);

         String tabStr = "All";
         if ( !doUCsv) {
         if (bOneUser) {
             tabStr = (String)accJCB.getSelectedItem().toString().trim();
             if (bSum)
                 tabStr = "Summary of " + tabStr;
         }
         else {
             if (bSum)
                 tabStr = "Summary of All";
         }
         }

         billPanel = new BillPanel(accJCB, bOneUser, bSum, sDate, eDate,
                           doS, doSCsv, doUCsv, saveDir);

         JTabbedPane pane = (JTabbedPane)billJsp.getBillTabs();
         pane.addTab(tabStr, billPanel);
         pane.setTabComponentAt(pane.getTabCount()-1, new ButtonTabComponent(pane));
         billJsp.setDividerLocation(420);
         pane.setSelectedIndex(pane.getTabCount()-1);

    }

    private void printAllBill(String type) {
         System.out.println("Error printAllBill not dead code. type = "+type);
    }
    
    public class BillingLayout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {
        }
        
        public void layoutContainer(Container parent) {
            final int deltay = 21, h = 9, labelW = 70;
            int tmpX1, tmpX2, tmpY1, tmpY2;
            int x = parent.getWidth();
            int y = parent.getHeight();
            int mult;
            p1.setBounds(10,5,x-20,5*h);
            mult = 6;
//            js1.setBounds(5,mult*h, x-10, 10);
            p2.setBounds(10,mult*h+5,x-20,15*h);
            mult += 16;
//            js2.setBounds(5, mult*h, x-10, 10);
            p3.setBounds(10,mult*h+5,x-20,19*h);
            mult += 20;
//            js3.setBounds(5, mult*h, x-10, 10);
            p4.setBounds(10,mult*h+5, x-20,18*h);
        }
        public Dimension minimumLayoutSize( Container parent ) {
            return ( new Dimension(0,0));
        }
        public Dimension preferredLayoutSize( Container parent ) {
            //return ( new Dimension(300,400) );
            return ( new Dimension(parent.getWidth(),parent.getHeight()));
        }
        public void removeLayoutComponent( Component c  ) {
        }
    }
    
    private class P1Layout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {  }
        
        public void layoutContainer(Container parent) {
            // final int  h = 18, deltay = 21;
            int x, y;
            int pw = parent.getWidth();
            int ph = parent.getHeight();
            Dimension dim0 = accL.getPreferredSize();
            Dimension dim = accJCB.getPreferredSize();
            if (dim.width < 100)
               dim.width = 100;
            if (dim.height > 22)
               dim.height = dim.height - 4;
            x = pw - 15 - dim.width;
            if (x <= dim0.width) {
               x = dim0.width + 6;
               dim.width = pw - x - 4;
            }
            y = (ph - dim.height) / 2 + 2;
            accJCB.setBounds(x, y, dim.width, dim.height);
            x = x - dim0.width - 6;
            y = (ph - dim0.height) / 2;
            accL.setBounds(x, y, dim0.width, dim0.height);

            // accJCB.setBounds(x - 15 - 100, 15, 100, h);
            // accL.setBounds(x - 15 - 100 - 60, 15, 60, h);
        }
        public Dimension minimumLayoutSize( Container parent ) {
            return ( new Dimension(0,0));
        }
        public Dimension preferredLayoutSize( Container parent ) {
            //return ( new Dimension(300,400) );
            return ( new Dimension(parent.getWidth(),parent.getHeight()));
        }
        public void removeLayoutComponent( Component c  ) { }
    }
    
    private class P2Layout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {  }
        
        public void layoutContainer(Container parent) {
            final int  h = 18, deltay = 21, labelW=70;
            int x = parent.getWidth();
            int y = parent.getHeight();
            int mult = 1;
            nameL.setBounds(5, mult * deltay, labelW, h);
            nameTF.setBounds(labelW + 10, mult * deltay, x - 115, h);
            mult++;
            addressL.setBounds(5, mult * deltay, labelW, h);
            addressTF.setBounds(labelW + 10, mult * deltay, x - 115, h);
            mult++;
            if (address2TF.getText() != null) {
               if (address2TF.getText().length()!=0) {
                   address2TF.setBounds(labelW + 10, mult * deltay, x - 115, h);
                   mult++;
               }
            }
            cityL.setBounds(5, mult * deltay, labelW, h);
            cityTF.setBounds(labelW + 10, mult * deltay, x - 115, h);
            mult++;
            stateL.setBounds(5, mult * deltay, labelW, h);
            stateTF.setBounds(labelW + 10, mult * deltay, x - 115, h);
            mult++;
            countryL.setBounds(5, mult * deltay, labelW, h);
            countryTF.setBounds(labelW + 10, mult * deltay, x - 115, h);
        }
        public Dimension minimumLayoutSize( Container parent ) {
            return ( new Dimension(0,0));
        }
        public Dimension preferredLayoutSize( Container parent ) {
            //return ( new Dimension(300,400) );
            return ( new Dimension(parent.getWidth(),parent.getHeight()));
        }
        public void removeLayoutComponent( Component c  ) { }
    }
    
    private class P3Layout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {  }
        
        public void layoutContainer(Container parent) {
            final int  h = 18, deltay = 21, labelW = 70;
            int x = parent.getWidth();
            int y = parent.getHeight();
            int mult = 1;
            startOfBilling.setBounds(17,mult * deltay,(int)(2.5*labelW-2), h*7);
            endOfBilling.setBounds((int)(0.5*x+4),mult * deltay, (int)(2.5*labelW-2), h*7);
        }
        public Dimension minimumLayoutSize( Container parent ) {
            return ( new Dimension(0,0));
        }
        public Dimension preferredLayoutSize( Container parent ) {
            //return ( new Dimension(300,400) );
            return ( new Dimension(parent.getWidth(),parent.getHeight()));
        }
        public void removeLayoutComponent( Component c  ) { }
    }
    
    private class P4Layout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {  }
        
        public void layoutContainer(Container parent) {
            int  h = 9, deltay = 21;
            int x = parent.getWidth();
            int y = parent.getHeight();
            int mult = 4;
            int w;

            y = mult * h;
            w = (int)(0.45*x);
            deltay = 28;
            displayOne.setBounds(17, y, w, deltay);
            displayOneSum.setBounds((int)(0.45*x+23), y, w, deltay);
            // printOne.setBounds((int)(0.45*x+23), mult*h,(int)(0.45*x),deltay);
            mult +=5;
            js_p4.setBounds(17, mult*h,x-34,9);
            mult +=2;
            y = mult * h;
            displayAll.setBounds(17, y, w, deltay);
            displayAllSum.setBounds((int)(0.45*x+23), y, w, deltay);

            // printAll.setBounds(17, mult*h,x-34,2*deltay);
        }
        public Dimension minimumLayoutSize( Container parent ) {
            return ( new Dimension(0,0));
        }
        public Dimension preferredLayoutSize( Container parent ) {
            //return ( new Dimension(300,400) );
            return ( new Dimension(parent.getWidth(),parent.getHeight()));
        }
        public void removeLayoutComponent( Component c  ) { }
    }
    
    private class JCBEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            updateAccountInfo();
        }
    }
    
    private class BillEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            // String s = ((JButton)ae.getSource()).getText();
            String s = ((JButton)ae.getSource()).getActionCommand();

            updateAccountInfo();
            if (s.startsWith("Display"))
                displayBill(s);
            else
                printAllBill(s);
        }
    }


    class PrintBill implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
System.out.println("Error PrintBill ActionListened called");
        }
    }
}
