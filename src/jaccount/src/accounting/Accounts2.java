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
import java.io.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;

public class Accounts2 extends JPanel {

  Accounts2 stf;
  AccountInfo2 ai;
  AccountJCB accJCB;
  Billing bill;
  File accountsDir;
  JButton insertRow, deleteRow, save, remove;
  JLabel accL, billL, nameL, addressL, address2L, cityL, stateL, countryL;
  JLabel usersLabel;
  JPanel p1,p2,p3,p4;
  UserChooser userChooser;
  JSeparator js1, js2, js3;
  JTextField nameTF,addressTF, address2TF, cityTF, stateTF, countryTF;
  JScrollPane jsp,jsp2;
  SpinTable2 st;
  private String oldAccName = null;

  public Accounts2(File accountsDir, Billing bill) {
    this.accountsDir = accountsDir;
    this.bill = bill;
    setLayout( new AccountsLayout() );
    Font f = new Font("Ariel",Font.PLAIN,12);
    stf=this;       // confirm overwrite account file dialog
    // Create the JCombo Box with account names,
    // true to allow entering of account
    p1 = new JPanel( new P1Layout() );
//    Color c = new Color(175,230,230);
//    Border b1 = BorderFactory.createLineBorder( p1.getBackground().darker(), 3);
    Border b1 = BorderFactory.createEtchedBorder();
//    Border b1 = BorderFactory.createLineBorder( c, 3);
    Border b = BorderFactory.createTitledBorder(b1, "Account");
    p1.setBorder(b);
    accL = new JLabel("Account: ");                        p1.add(accL);
    accL.setFont(f);
    accJCB = new AccountJCB(accountsDir, true);
    accJCB.setSelectedIndex(-1);
    accJCB.addActionListener( new AccountJCBEar() );       p1.add(accJCB);
                                                           add(p1);
//    js1 = new JSeparator();                                add(js1);
    // Create the address info text entry fields
    p2 = new JPanel( new P2Layout() );
    b = BorderFactory.createTitledBorder(b1, "Billing address");
    p2.setBorder(b);
    billL = new JLabel("Billing Address:");                p2.add(billL);
    nameL = new JLabel("Name: ");
    nameL.setFont(f);
    nameL.setHorizontalAlignment(SwingConstants.RIGHT);    p2.add(nameL);
    nameTF = new JTextField(40);                           p2.add(nameTF);
    addressL = new JLabel("Address: ");
    addressL.setFont(f);
    addressL.setHorizontalAlignment(SwingConstants.RIGHT); p2.add(addressL);
    addressTF = new JTextField(40);                        p2.add(addressTF);
    address2TF = new JTextField(40);                       p2.add(address2TF);
    cityL = new JLabel("City: ");
    cityL.setFont(f);
    cityL.setHorizontalAlignment(SwingConstants.RIGHT);    p2.add(cityL);
    cityTF = new JTextField(40);                           p2.add(cityTF);
    stateL = new JLabel("State: ");
    stateL.setFont(f);
    stateL.setHorizontalAlignment(SwingConstants.RIGHT);   p2.add(stateL);
    stateTF = new JTextField(40);                          p2.add(stateTF);
    countryL = new JLabel("Country: ");
    countryL.setFont(f);
    countryL.setHorizontalAlignment(SwingConstants.RIGHT); p2.add(countryL);
    countryTF = new JTextField(40);                        p2.add(countryTF);
      
                                                           add(p2);
                                                           
//    js2 = new JSeparator();                                add(js2);
    // Create the rate table, day/time start and rate spinners
    p3 = new JPanel( new P3Layout() );
    b = BorderFactory.createTitledBorder(b1, "Rate schedule");
    p3.setBorder(b);
    st = new SpinTable2();
    jsp = new JScrollPane(st);                             p3.add(jsp);
    // Create the insert/delete row and save account button 
    insertRow = new JButton("Insert Row");
//    insertRow.setBackground(c);
    insertRow.addActionListener(new InsertRowEar());       p3.add(insertRow);
    deleteRow = new JButton("Delete Row");
//    deleteRow.setBackground(c);
    deleteRow.addActionListener(new DeleteRowEar());       p3.add(deleteRow);
                                                           add(p3);
//    js3 = new JSeparator();                                add(js3);
    
    p4 = new JPanel( new P2Layout() );
    b = BorderFactory.createTitledBorder(b1, "Owners and operators");
    p4.setBorder(b);
    userChooser = UserChooser.getInstance();
    jsp2 = new JScrollPane(userChooser);                   p4.add(jsp2);
                                                           add(p4);
                                                           
    save = new JButton("Save");
//    save.setBackground(c);
    save.addActionListener(new SaveEar());                 add(save);
    remove = new JButton("Remove");
    remove.addActionListener(new RemoveListener());               add(remove);
  }
  
  public JTable getSpinTable() {
      return st;
  }

  public class AccountsLayout implements LayoutManager {
    public void addLayoutComponent(String str, Component c) {
    }

    public void layoutContainer( Container parent ) {
      final int deltay=21, h=9, labelW=70;
      int tmpX1,tmpX2, tmpY1,tmpY2;
      int x = parent.getWidth();
      int y = parent.getHeight();
      int mult;
//       System.out.println("Account Panel: "+parent+"x="+x+" y="+y);
      p1.setBounds(10,5,x-20,5*h);
      mult = 6;
//      js1.setBounds(5,mult*h,x-10,10);
      p2.setBounds(10,mult*h+5,x-20,17*h);
      mult += 18;
//      js2.setBounds(5,mult*h,x-10,10);
      p3.setBounds(10,mult*h+5,x-20,19*h);
      mult += 20;
//      js3.setBounds(5,mult*h,x-10,10);
      p4.setBounds(10,mult*h+5,x-20,16*h);
      jsp2.setBounds(10,20,p4.getWidth()-20,p4.getHeight()-30);
      save.setBounds(10,y-2*deltay-10,x/2-20,2*deltay);
      remove.setBounds(10+x/2,y-2*deltay-10,x/2-25,2*deltay);
    }
    public Dimension minimumLayoutSize( Container parent ) {
      return ( new Dimension(0,0));
    }
    public Dimension preferredLayoutSize( Container parent ) {
        //return( new Dimension(300,400) );
        return ( new Dimension(parent.getWidth(),parent.getHeight()));
    }
    public void removeLayoutComponent( Component c  ) {
    }
  }

  public class P1Layout implements LayoutManager {
    public void addLayoutComponent(String str, Component c) {  }

    public void layoutContainer(Container parent) {
      // int  h = 18, deltay = 21;
      int x, y;
      int pw = parent.getWidth();
      int ph = parent.getHeight();
      Dimension dim0 = accL.getPreferredSize();
      Dimension dim = accJCB.getPreferredSize();
      //  accJCB.setBounds(x - 15 - 100, 15, 100, h);
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
  
  public class P2Layout implements LayoutManager {
    public void addLayoutComponent(String str, Component c) {  }

    public void layoutContainer(Container parent) {
      final int  h = 18, deltay = 21, labelW=70;
      int x = parent.getWidth();
      int y = parent.getHeight();
      int mult = 1;
//      System.out.println("Panel: x="+x+" y="+y);
      nameL.setBounds(5,mult*deltay,labelW,h);
      nameTF.setBounds(labelW+10,mult*deltay,x-115,h);
      mult++;
      addressL.setBounds(5,mult*deltay,labelW,h);
      addressTF.setBounds(labelW+10,mult*deltay,x-115,h);
      mult++;
      address2TF.setBounds(labelW+10,mult*deltay,x-115,h);
      mult++;
      cityL.setBounds(5,mult*deltay,labelW,h);
      cityTF.setBounds(labelW+10,mult*deltay,x-115,h);
      mult++;
      stateL.setBounds(5,mult*deltay,labelW,h);
      stateTF.setBounds(labelW+10,mult*deltay,x-115,h);
      mult++;
      countryL.setBounds(5,mult*deltay,labelW,h);
      countryTF.setBounds(labelW+10,mult*deltay,x-115,h);
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
      final int  h = 18, deltay = 10;
      int x = parent.getWidth();
      int y = parent.getHeight();
//      System.out.println("Panel: x="+x+" y="+y);
      jsp.setBounds(10,2*deltay,x-20,6*h);
      insertRow.setBounds(17,y-3*deltay,(int)(0.45*x),2*deltay);
      deleteRow.setBounds((int)(0.45*x+23), y-3*deltay,(int)(0.45*x),2*deltay);
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
  
  public class AccountJCBEar implements ActionListener {
    public void actionPerformed(ActionEvent ae) {
//System.out.println("Combo Return");
      String newName = (String) accJCB.getSelectedItem();

      if (oldAccName != null) {
          if (!oldAccName.equals(newName)) {
              saveUser(oldAccName, false);
          }
      }

      ai = accJCB.read();
      if (ai == null)
          return;
      oldAccName = newName;
      nameTF.setText(ai.name());
      addressTF.setText(ai.address1());
      address2TF.setText(ai.address2());
      cityTF.setText(ai.city());
      stateTF.setText(ai.zip());
      countryTF.setText(ai.country());

      st.setRates(ai);
      userChooser.setCheckedOAndOs(ai);
    }
  }


  class InsertRowEar implements ActionListener {
    public void actionPerformed(ActionEvent ae) {
      st.insertRow();
    }
  }

  class DeleteRowEar implements ActionListener {
    public void actionPerformed(ActionEvent ae) {
      st.deleteRow();
    }
  }

   private void saveUser(String accountName, boolean bCheck) {
      StringBuilder errMsg= new StringBuilder("");
      if (accountName == null)
         return;
//      System.out.println("Saving to: '"+accountName+"'");
      boolean badFileName=false;
      if (accountName==null) {
          badFileName=true;
      }
      else {
          accountName=accountName.trim();
          if (accountName.isEmpty() ) {
              badFileName = true;
          }
      }
      if (badFileName) {
          errMsg.append("Please enter or select an account first.\n");
      }
      else {
        if ( (accountName.indexOf('!') != -1)  || (accountName.indexOf('"') != -1) ||
             (accountName.indexOf('$') != -1)  || (accountName.indexOf('&') != -1) ||
             (accountName.indexOf('\'')!= -1) ||  (accountName.indexOf('(') != -1) ||
             (accountName.indexOf(')') != -1)  || (accountName.indexOf('*') != -1) ||
             (accountName.indexOf(';') != -1)  || (accountName.indexOf('<') != -1) ||
             (accountName.indexOf('>') != -1)  || (accountName.indexOf('?') != -1) ||
             (accountName.indexOf('\\') != -1) || (accountName.indexOf('[') != -1) ||
             (accountName.indexOf(']') != -1)  || (accountName.indexOf('^') != -1) ||
             (accountName.indexOf('`') != -1)  || (accountName.indexOf('{') != -1) ||
             (accountName.indexOf('}') != -1)  || (accountName.indexOf('|') != -1) ||
             (accountName.indexOf(',') != -1) ) {
            errMsg.append("The account name should not contain any of these characters:\n         !$^&*()`{}[]|\\\"\',\n");
        }
      }
            
      if ( (nameTF.getText() == null) || (nameTF.getText().trim().length() == 0) ) {
          errMsg.append("The Billing address must contain at least a name.\n");
      }
      
      if (errMsg.length() > 5) {
        JOptionPane pane = new JOptionPane();
        pane.showMessageDialog(stf, errMsg);
        return;
      }

      File acc = new File(AProps.getInstance().getRootDir()+"/adm/accounting/accounts/"+accountName+".txt");
      if (acc.exists()) {
          if (!acc.canWrite()) {
              if (bCheck) {
                 JOptionPane.showMessageDialog(p4,
                    "Could not overwrite "+acc.getAbsolutePath());
              }
              return;
          }
          if (bCheck) {
              JOptionPane pane = new JOptionPane();
              int result = pane.showConfirmDialog(p4,
                   "File exists, overwrite?","File Exists",
                   JOptionPane.YES_NO_OPTION);
              if (result != JOptionPane.YES_OPTION)
                  return;
          }
      }
      
      if (st.checkOrder(p4) == 1)
          return;
      
      if (! acc.exists() ) {
          accJCB.addItem(accountName);
          bill.accJCB.addItem(accountName);
      }

      try {
        BufferedWriter bufOut = new BufferedWriter(new FileWriter(acc));
        bufOut.write("# String to Identify this file as a computer written Account File");
        bufOut.newLine();
        bufOut.write("# Do not edit this file by hand");
        bufOut.newLine();
        bufOut.write("# Even the comments are critical");
        bufOut.newLine();
        bufOut.write("#");
        bufOut.newLine();
        save(bufOut);
        st.save(bufOut);
        userChooser.listCheckedOAndOs(bufOut);
        bufOut.close();
      } catch (IOException ioe) {
        System.out.println("IO Exception "+ioe.getStackTrace());
        ioe.getStackTrace();
      }
  }

  public void saveAccountInfo() {
      String accountName = (String) accJCB.getSelectedItem();
      saveUser(accountName, false);
  }

  class SaveEar implements ActionListener {
    public void actionPerformed(ActionEvent ae) {
         String accountName = (String) accJCB.getSelectedItem();
         saveUser(accountName, true);
    }
  }

  class RemoveListener implements ActionListener {
      private boolean badFileName;
      public void actionPerformed(ActionEvent ae) {  
        StringBuilder errMsg= new StringBuilder("");
        String accountName = (String) accJCB.getSelectedItem();//  .toString().trim();
        badFileName=false;
        if (accountName==null) {
            badFileName=true;
        }
        else {
            accountName=accountName.trim();
            if (accountName.isEmpty() ) {
                badFileName = true;
            }
        }
        if (badFileName) {
            errMsg.append("Please enter or select an account first.");
        }
        else {  
            File acc = new File(AProps.getInstance().getRootDir()+"/adm/accounting/accounts/"+accountName+".txt");
            if (acc.exists()) {
                if (!acc.canWrite()) {
                   JOptionPane.showMessageDialog(p4,
                      "Could not remove "+acc.getAbsolutePath());
                   return;
                }
                acc.delete();
                errMsg.append(accountName + " Removed.  Please close and restart "
                        + "accounting for this change to be shown.");
            }
        }
        if (errMsg.length() > 5) {
            JOptionPane pane = new JOptionPane();
            pane.showMessageDialog(stf, errMsg);
            return;
          }
      }
    }
 
  
  
  final static String InvComment = new String("# Investigator information");
  public void save(BufferedWriter bufOut) throws IOException {
    bufOut.write(InvComment);
    bufOut.newLine();
    bufOut.write(nameTF.getText());       bufOut.newLine();
    bufOut.write(addressTF.getText());    bufOut.newLine();
    bufOut.write(address2TF.getText());   bufOut.newLine();
    bufOut.write(cityTF.getText());       bufOut.newLine();
    bufOut.write(stateTF.getText());      bufOut.newLine();
    bufOut.write(countryTF.getText());    bufOut.newLine();
  }
}
