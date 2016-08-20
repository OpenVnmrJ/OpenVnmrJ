// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*
 * 
 *
 * Config.java
 *
 * Created on April 29, 2006, 8:14 AM
/*
 * Config.java
 *
 * Created on April 29, 2006, 8:14 AM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

/**
 *
 * @author frits
 */

package accounting;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.text.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.*;
import javax.swing.table.*;
import javax.swing.text.*;
import javax.xml.parsers.*;
import org.xml.sax.*;
import org.xml.sax.helpers.DefaultHandler;

public class Config extends JPanel {
    
    private final static int ACQ_REDUCE_FILE = 0;
    private final static int LOGIN_REDUCE_FILE = 1;
    
    private Config conf;
    private AProps prop;
    private JCheckBox[] activeCB = new JCheckBox[2];
    private JLabel[] activeLabel = new JLabel[2];
    private JLabel[] activeStatus = new JLabel[2];
    private JRadioButton[] billmodeJRB = new JRadioButton[2];
    private ButtonGroup billmodeBG;
    private JRadioButton[] reduceJRB = new JRadioButton[2];
    private ButtonGroup reduceBG;
    private JButton activeGo;
//    private JSeparator js1 = new JSeparator();
    private JLabel currLbl, currEG, currExample, currHttp;
    private JTextField currJTF;
    private JLabel logoLbl, logoShow;
    private JButton logoBrowse;
    private ImageIcon logoIcon;
    private JLabel modeLbl;
//    private JComboBox modeJCB;
    private JLabel headerLbl;
    private JTextField[] headerJTF = new JTextField[4];
    private JButton saveProp;
//    private JSeparator js2 = new JSeparator();
    private DateChooser reduceToDate;
    private long reduceTimeMills;
    private JButton reduceGo;
    private JFileChooser logoChooser;
    private String[] billingModeStrings = { "login", "goes", "login"};
    private AbstractTableModel mtm;
    private int reduceMode;
    private AccountInfo2 ai;
    
    JPanel p1,p2,p3;

    
    /** Creates a new instance of Config */
    public Config(AbstractTableModel m, AccountInfo2 aiArg) {
        Color c;
        
        mtm = m;    //when headers change...
        ai  = aiArg; // to re-read records.
        
        conf = this;
        setLayout( new ConfigLayout() );
        prop = AProps.getInstance();

        Font f = new Font("Arial",Font.PLAIN,12);
	
        /*****************************/
        /*     Panel 1 components    */
        /*****************************/
        p1 = new JPanel( new P1Layout() );
//        p1.setBackground( c );
//        Border b1 = BorderFactory.createLineBorder( c, 3);
//        Border b1 = BorderFactory.createLineBorder( p1.getBackground().darker(), 3);
        Border b1 = BorderFactory.createEtchedBorder();
        Border b = BorderFactory.createTitledBorder(b1, "Record keeping");
        p1.setBorder(b);

        activeLabel[0] = new JLabel("Acquisition & Use records keeping: ");
	activeLabel[0].setFont(f);
        StringBuilder sb = new StringBuilder(prop.getRootDir());
        sb.append("/adm/accounting/acctLog.xml");
        activeCB[0] = new JCheckBox();
        activeCB[0].addItemListener(new ActiveGoEar());
        activeStatus[0] = new JLabel();
	activeStatus[0].setFont(f);
        File recordFile = new File(sb.toString());
        
        if (recordFile.exists()) {
            activeCB[0].setSelected(true);
            activeStatus[0].setText("Active");
        }
        else {
            activeCB[0].setSelected(false);
            sb = new StringBuilder(prop.getRootDir());
            sb.append("/adm/accounting/acctLog_onHold.xml");
            recordFile = new File(sb.toString());
            if (recordFile.exists()) {
                activeStatus[0].setText("On hold");
            }
            else {
                activeStatus[0].setText("Inactive");
            }
        }
        
        p1.add(activeCB[0]);
        p1.add(activeLabel[0]);
        p1.add(activeStatus[0]);

        
        add(p1);
        
//        add(js1);
        /*****************************/
        /*     Panel 2 components    */
        /*****************************/
//        c = new Color(240,240,210);
        p2 = new JPanel( new P2Layout() );
//        p2.setBackground( c );
//        b1 = BorderFactory.createLineBorder( new Color(200,180,0), 3);
        b = BorderFactory.createTitledBorder(b1, "Invoice Properties");
        p2.setBorder(b);
        
        currLbl = new JLabel("Locale:");
	currLbl.setFont(f);
        p2.add(currLbl);
        currJTF     = new JTextField(2);
        String local = (String) (prop.get("currency"));
        currJTF.setText(local);
        currJTF.addActionListener( new CurrEar() );
        p2.add(currJTF);
        
        currEG = new JLabel("  For Example: ");
	currEG.setFont(f);
        p2.add ( currEG );
        NumberFormat nf = prop.getCurrencyFormat();
        currExample = new JLabel( nf.format( 12345.67 ) );
	currExample.setFont(f);
        p2.add(currExample);
        
        currHttp = new JLabel("http://www.chemie.fu-berlin.de/diverse/doc/ISO_3166.html");
	currHttp.setFont(f);
        p2.add(currHttp);
        
        logoLbl = new JLabel("Logo: ");
	logoLbl.setFont(f);
        p2.add(logoLbl);
        logoBrowse = new JButton("Browse...");
//        logoBrowse.setBackground(c);
        logoChooser = new JFileChooser(AProps.getInstance().getRootDir()+"/adm/accounting");
        logoChooser.setDialogTitle("Select a logo GIF file");
        logoChooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
//       FileFilter filter = new FileFilter();
//       filter.addExtension("gif");
//       filter.setDescription("JPG & GIF Images");
//       chooser.setFileFilter(filter);
        logoBrowse.addActionListener( new BrowseEar() );
        p2.add(logoBrowse);
        logoIcon = prop.logo();
        logoShow = new JLabel(logoIcon);
        p2.add(logoShow);
       
        modeLbl = new JLabel("Billing mode: ");
	modeLbl.setFont(f);
        p2.add(modeLbl);
        ActionListener billmodeEar = new BillModeEar();
        // String bm = prop.getBillingMode();
        billmodeJRB[0] = new JRadioButton("VnmrJ use records", prop.isLoginBillingMode());
	billmodeJRB[0].setFont(f);
        if (prop.isLoginBillingMode())
	    billmodeJRB[0].setSelected(true);
        billmodeJRB[0].addActionListener( billmodeEar );
        billmodeJRB[1] = new JRadioButton("Acquisition records", prop.isGoesBillingMode());
	billmodeJRB[1].setFont(f);
        if (prop.isGoesBillingMode())
	    billmodeJRB[1].setSelected(true);
        billmodeJRB[1].addActionListener( billmodeEar );

        p2.add(billmodeJRB[0]);
        p2.add(billmodeJRB[1]);
        /**********
        billmodeBG = new ButtonGroup();
        billmodeBG.add(billmodeJRB[0]);
        billmodeBG.add(billmodeJRB[1]);
        **********/


//        Vector modes = new Vector(3);
//        modes.add( new String(" Login time") );
//        modes.add( new String(" Acquisition time ") );
//        modeJCB = new JComboBox(modes);
////        modeJCB.setBackground(c);
//        p2.add(modeJCB);
//        if ( prop.getBillingMode().equalsIgnoreCase("macros") )
//            modeJCB.setSelectedIndex(0);
//        else
//            modeJCB.setSelectedIndex(1);
       
        headerLbl = new JLabel("Table headers: ");
	headerLbl.setFont(f);
        p2.add(headerLbl);
        String[] headerString = prop.getTableHeaders();
        DocumentListener headerEar = new HeaderEar();
        for (int i=0; i<4; i++) {
            headerJTF[i] = new JTextField(headerString[i],8);
            headerJTF[i].getDocument().addDocumentListener( headerEar );
            p2.add(headerJTF[i]);
        }
       
//        saveProp = new JButton("Save Properties");
////        saveProp.setBackground(c);
//        saveProp.addActionListener( new SaveEar() );
//        p2.add(saveProp);
        
        add(p2);
       
//        add(js2);
       
        /*****************************/
        /*     Panel 3 components    */
        /*****************************/
//        c = new Color(210,210,240);
        p3 = new JPanel( new P3Layout() );
//        p3.setBackground( c );
        b = BorderFactory.createTitledBorder(b1, "Record sizing");
        p3.setBorder(b);
        reduceToDate = new DateChooser("Remove records before");
        p3.add(reduceToDate);
        ActionListener reduceJRBEar = new ReduceJRBEar();
        reduceJRB[0] = new JRadioButton("VnmrJ use records", true);
	reduceJRB[0].setFont(f);
        reduceJRB[0].addActionListener( reduceJRBEar );
        reduceJRB[1] = new JRadioButton("Acquisition records",false);
	reduceJRB[1].setFont(f);
        reduceJRB[1].addActionListener( reduceJRBEar );
//        reduceBG = new ButtonGroup();
//        p3.add(reduceJRB[0]);
//        p3.add(reduceJRB[1]);
//        reduceBG.add(reduceJRB[0]);
//        reduceBG.add(reduceJRB[1]);
        
        reduceGo = new JButton(" Cut Records ");
//        reduceGo.setBackground(c);
        reduceGo.addActionListener( new ReduceGoEar() );
        p3.add(reduceGo);
        
        add(p3);
    }
    
    public class ConfigLayout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {
        }

        public void layoutContainer(Container parent) {
          final int deltay = 40, h = 18, labelW = 70;
          int tmpX1, tmpX2, tmpY1, tmpY2;
          int x = parent.getWidth();
          int y = parent.getHeight();
          int yy=30;
//      System.out.println("Panel: x="+x+" y="+y);
          p1.setBounds(10,5,x-20,(int)2.5*deltay);
          yy = (int)2.5*deltay-5;
//          js1.setBounds(10, yy+15, x-20,h);
          yy += 15;
          p3.setBounds(10,yy, x-20,9*h+9);
          yy += 10*h;
//          js2.setBounds(10, yy, x-20,h);
          yy += 5;
          p2.setBounds(10,yy,x-20,8*deltay);
        }
        public Dimension minimumLayoutSize( Container parent ) {
          return ( new Dimension(0,0));
        }
        public Dimension preferredLayoutSize( Container parent ) {
          return ( new Dimension(parent.getWidth(),parent.getHeight()));
        }
        public void removeLayoutComponent( Component c  ) {
        }
    }
    
    public class P1Layout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {
        }

        public void layoutContainer(Container parent) {
          final int deltay = 20, h = 18, labelW = 70;
          int x = parent.getWidth();
          int y = parent.getHeight();
          int yy=deltay;
//      System.out.println("Panel: x="+x+" y="+y);

          
          yy += 20;
         
          activeLabel[0].setBounds(20,yy,x/2+70,h);
          activeCB[0].setBounds(x/2+60,yy,20,h);
          activeStatus[0].setBounds(x/2+80,yy,100,h);
        }
        public Dimension minimumLayoutSize( Container parent ) {
          return ( new Dimension(0,0));
        }
        public Dimension preferredLayoutSize( Container parent ) {
          return ( new Dimension(parent.getWidth(),parent.getHeight()));
        }
        public void removeLayoutComponent( Component c  ) {
        }
    }
    
     public class P2Layout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {}

        public void layoutContainer(Container parent) {
          final int deltay = 40, h = 18, labelW = 70;
          int tmpX1, tmpX2, tmpY1, tmpY2;
          int x = parent.getWidth();
          int y = parent.getHeight();
          int yy=30;
//      System.out.println("Panel: x="+x+" y="+y);
          currLbl.setBounds(10, yy, 100, h);
          currJTF.setBounds(80, yy, 60, h);
          currEG.setBounds(150, yy, 120, h);
          currExample.setBounds(240, yy, 120, h);
          currHttp.setBounds(10,yy+15, 350, h);
          yy += deltay + 15;
          logoLbl.setBounds(10, yy, 100, h);
          logoShow.setBounds(45,yy,logoIcon.getIconWidth(), logoIcon.getIconHeight());
          logoBrowse.setBounds(2*x/3, yy, 100, h+20);
          yy += logoIcon.getIconHeight()+h;
          modeLbl.setBounds(10, yy, 100, h);
          billmodeJRB[0].setBounds(110,yy,180,h);
          billmodeJRB[1].setBounds(110,yy+h,180,h);
//          modeJCB.setBounds(110, yy,150, h);
          yy += deltay+2*h;
          headerLbl.setBounds(10, yy,100,h);
          for (int i=0; i<4; i++)
              headerJTF[i].setBounds(110+i*65, yy, 65, h);
          yy += deltay;
//          saveProp.setBounds(20,yy, x-40,2*h);
        }
        public Dimension minimumLayoutSize( Container parent ) {
          return ( new Dimension(0,0));
        }
        public Dimension preferredLayoutSize( Container parent ) {
          return ( new Dimension(parent.getWidth(),parent.getHeight()));
        }
        public void removeLayoutComponent( Component c  ) {
        }
    }
     
     public class P3Layout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {}

        public void layoutContainer(Container parent) {
          final int deltay = 40, h = 18, labelW = 70;
          int x = parent.getWidth();
          int y = parent.getHeight();
          int yy=30;
//      System.out.println("Panel: x="+x+" y="+y);
          reduceToDate.setBounds(25, yy, 175, 125 );
          reduceJRB[0].setBounds(220,yy, 150,h+10);
          reduceJRB[1].setBounds(220,yy+25, 150,h+10);
          reduceGo.setBounds(220, yy+85, 120, h+20);
        }
        public Dimension minimumLayoutSize( Container parent ) {
          return ( new Dimension(0,0));
        }
        public Dimension preferredLayoutSize( Container parent ) {
          return ( new Dimension(parent.getWidth(),parent.getHeight()));
        }
        public void removeLayoutComponent( Component c  ) {
        }
    }
     
    private class CurrEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            System.out.print("Curr Action");
            String country = ((JTextField)ae.getSource()).getText();
            System.out.println(": '"+country+"'");
            Locale l = new Locale( country );
            String lang = l.getLanguage();
            l = new Locale(lang,country);
            NumberFormat printMoney = NumberFormat.getCurrencyInstance(l);
//    System.out.println("df2 has '"+nf.getCurrency()+"'");
            Currency newC = Currency.getInstance(printMoney.getCurrency().toString());
            printMoney.setCurrency(newC);
            currExample.setText( printMoney.format( 12345.67 ) );
            prop.setCurrencyFormat(country);
        }
    }
    
    private class BrowseEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
           int returnVal = logoChooser.showOpenDialog(conf);
           if (returnVal == JFileChooser.APPROVE_OPTION) {
//              System.out.println("You chose to open this file: " +  logoChooser.getSelectedFile().getName());
               File file = logoChooser.getSelectedFile();
               String fullpath = file.getAbsolutePath();
               // The logo must be in the /vnmr/adm/accounting directory
               // as per the manual
               String system = new String(AProps.getRootDir()+"/adm/accounting/");
               if(fullpath.indexOf(system) == -1) {
                   JOptionPane pane = new JOptionPane();
                   pane.showMessageDialog(Config.this, "Logo file must be in " + system
                               + " directory.");
                   return;
               }

               
              ImageIcon logo = new ImageIcon( logoChooser.getSelectedFile().getAbsolutePath() );
              logoShow.setIcon(logo);
              prop.setLogo(logoChooser.getSelectedFile().getName());
           }
        }
    }
        
//    private class SaveEar implements ActionListener {
//        public void actionPerformed(ActionEvent ae) {
      public void saveProperties() {
            String tmpStr;
            File prop = new File(AProps.getInstance().getRootDir()+"/adm/accounting/accounts/accounting.prop");
            try {
                BufferedWriter propFd = new BufferedWriter( new FileWriter(prop));
 
                propFd.write("# This file is computer generated by VnmrJ Accounting.");
                propFd.newLine();
                propFd.write("# This file is used to allow for setting of certain properties of the");
                propFd.newLine();
                propFd.write("# Accounting package.");
                propFd.newLine();
                propFd.write("# To test: java -jar account.jar Aprops. This will just");
                propFd.newLine();
                propFd.write("# display the various values. It does not guarantee it is ok.");
                propFd.newLine();
                propFd.write("# Any line that start with a '#' is considered a comment and skipped, ");
                propFd.newLine();
                propFd.write("# blank lines would be better if they have at least starting '#'.");
                propFd.newLine();
                propFd.write("# The format consists of a single keyword, and a value-string, separetes ");
                propFd.newLine();
                propFd.write("# by <space>=<space>.");
                propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("# The 'currency' keyword defines currency symbol and number format.");
                propFd.newLine();
                propFd.write("# It uses the two letter country code as (currently) defined at:");
                propFd.newLine();
                propFd.write("# http://www.chemie.fu-berlin.de/diverse/doc/ISO_3166.html");
                propFd.newLine();
                propFd.write("# As of Java 1.4.1 not all produce the expected result, but 'ie' for Ireland");
                propFd.newLine();
                propFd.write("# or 'nl for The Netherlands produce EUR or the euro character, respectively.");
                propFd.newLine();
                propFd.write("# This does also produce 12.345,67 if currency is 'nl' or 'de', etc.");
                propFd.newLine();
                propFd.write("#");            propFd.newLine();
                tmpStr = currJTF.getText();
                propFd.write("currency = "+tmpStr);
                propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("# By default it is \"#,##0.00\"");
                propFd.newLine();
                propFd.write("# This does produce 12.345,67 if currency is NL,DE, etc; \"#.##0,00\" gives");
                propFd.newLine();
                propFd.write("# an exception. This is not really needed.");
                propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("numberFormat = #,##0.00");
                propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("# defines the gif used on the bills. You can (and should) put you own logo ");
                propFd.newLine();
                propFd.write("# here ");
                propFd.newLine();
                propFd.write("#");             propFd.newLine();
                File tmpFile = logoChooser.getSelectedFile();
                if (tmpFile == null) 
                    tmpStr = new String("vnmrjNameBW.png");
                else
                    tmpStr = tmpFile.getName();
                propFd.write("logo = "+tmpStr);
                propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("#");
                propFd.newLine();
                propFd.write("# The header of the rate table can be set with these four string, they must");
                propFd.newLine();
                propFd.write("# be enclosed by double quotes, starting and ending spaces are trimmed.");
                propFd.newLine();
                propFd.write("# They can be anything, but the table is used as is. Only the first 4 are used.");
                propFd.newLine();
                propFd.write("# Less then 4 causes trouble.");
                propFd.newLine();
                propFd.write("# You may wan to change this if you change 'billingmode' to");
                propFd.newLine();
                propFd.write("# tableheaders = \"Day\" \"Time\" \"Go Rate\" \"Go/hr\"");
                propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("tableheaders = \""+headerJTF[0].getText()+"\"\t\""+headerJTF[1].getText()+
                                           "\"\t\""+headerJTF[2].getText()+"\"\t\""+headerJTF[3].getText()+"\"");
                propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("#");            propFd.newLine();
                propFd.write("# The billing can  be based on either login/logout use, or based on acquisition time");
                propFd.newLine();
                propFd.write("# activity. Changing this mode has various effects on where the program");
                propFd.newLine();
                propFd.write("# expects it input. See the manual for details");
                propFd.newLine();
                propFd.write("# Currently we support 'login time' and 'goes'");
                propFd.newLine();
                propFd.write("#");            propFd.newLine();
                if (billmodeJRB[0].isSelected())
                    tmpStr = (String) billingModeStrings[0];
                else if (billmodeJRB[1].isSelected())
                    tmpStr = (String) billingModeStrings[1];
                else
                    tmpStr = (String) billingModeStrings[2];
 //               tmpStr = (String) billingModeStrings[modeJCB.getSelectedIndex()];
                propFd.write("billingmode = "+tmpStr);
                propFd.newLine();
                propFd.write(AProps.loginBillingKey);
                if (billmodeJRB[0].isSelected())
                    propFd.write(" = true");
                else
                    propFd.write(" = false");
                propFd.newLine();
                propFd.write(AProps.goesBillingKey);
                if (billmodeJRB[1].isSelected())
                    propFd.write(" = true");
                else
                    propFd.write(" = false");
                propFd.newLine();
                propFd.write("#");           propFd.newLine();
                propFd.write("#END OF FILE");
                propFd.newLine();
            
                propFd.close();
//                AProps.reset();
                //Recheck this!
//               ReadLast.getInstance(new AccountInfo2()).reset();
//                mtm.fireTableStructureChanged();
                
            }
            catch (IOException ioe) {
                System.out.println("Cannot write accouning.prop file");
                // ioe.printStackTrace();
//            }
//            System.out.println("After Save : "+AProps.reset());
        }
    }

    private void reduceRecords(StringBuffer sb, Date reduceDate) {
        Date tmpDate = null;;
        File sourceFile, reducedFile;
        BufferedWriter fOut = null;
        boolean bError = false;
        String errMsg = null;
        SaxContentHandler xmlHandler = null;

        sb.append("accounting/acctLog");
        sourceFile = new File( sb.toString() + ".xml");
        if ( ! sourceFile.exists() ) {
             System.out.println("File: "+sourceFile.getName() +" does not exist");
             return;
        }
        reducedFile = new File( sb.toString() + ".sml");
        reduceTimeMills = reduceDate.getTime();

        try {
            fOut = new BufferedWriter (new FileWriter (reducedFile) );
            fOut.write("<accounting>\n");
            SAXParserFactory spf = SAXParserFactory.newInstance();
            SAXParser parser = spf.newSAXParser();
            xmlHandler = new SaxContentHandler(fOut);
            parser.parse(sourceFile, xmlHandler);
            fOut.write("</accounting>\n");
            fOut.close();
            fOut = null;
        }
        catch (ParserConfigurationException pce) {
            System.out.println(pce.getMessage());
            errMsg = pce.getMessage();
            bError = true;
        }
        catch (FactoryConfigurationError fce) {
            System.out.println(fce.getMessage());
            errMsg = fce.getMessage();
            bError = true;
        }
        catch (Exception ioe) {
             ioe.printStackTrace();
             errMsg = ioe.getMessage();
             bError = true;
        }
        finally {
            try {
               if (fOut != null)
                   fOut.close();
            }
            catch (Exception e2) {}
        }

        if (bError) {
            if (errMsg != null) {
               JOptionPane.showMessageDialog(null,
                   errMsg, "Error", JOptionPane.ERROR_MESSAGE);
            }
            return;
        }
        if (xmlHandler == null)
            return;
        int total = xmlHandler.getEntryNumber();;
        int remain = xmlHandler.getValidEntryNumber();
        String cutString = new String("Cutting "+Integer.toString(total-remain)+" records");
        JOptionPane pane = new JOptionPane();
        int result = pane.showConfirmDialog((JPanel)conf, cutString,
                        "Cutting Records",   JOptionPane.YES_NO_OPTION);
        if (result == JOptionPane.YES_OPTION) {
           try {
              String cmd;
              sourceFile.delete();
              reducedFile.renameTo(sourceFile);
              cmd = new String("chmod 666 "+sb.toString()+".xml");
              Runtime rt = Runtime.getRuntime();
              rt.exec(cmd);
           }
           catch (IOException ioe) {
              ioe.printStackTrace();
           }
        }
        else {
              reducedFile.delete();
        }
    }

    private class ActiveGoEar implements ItemListener {
        public void itemStateChanged(ItemEvent ie) {
            File records;
//            System.out.println("ActiveGoEar called");
            JCheckBox source = (JCheckBox) ie.getItemSelectable();
            
            if (source == activeCB[0]) {
                StringBuffer sb = new StringBuffer( AProps.getInstance().getRootDir());
                sb.append("/adm/");
//                System.out.println("for "+source.isSelected());
                
                if (source.isSelected()) {
                    sb.append("accounting/acctLog");
                    StringBuffer sb2 = new StringBuffer( sb.toString());
                    sb2.append("_onHold");
                    records = new File(sb2.toString()+".xml");
                    if (records.exists()) {
                        records.renameTo( new File(sb.toString()+".xml") );
                    }
                    else {
                        records = new File(sb.toString()+".xml");
                        if (records.exists()) {
                            return;
                        }
                        try {
                            BufferedWriter bf = new BufferedWriter( new FileWriter(records));
			    bf.write("<accounting>\n</accounting>\n",0,26);
                            bf.newLine();
                            bf.close();
                            // Be sure the log file has world write permission
                            records.setWritable(true, false);
                        }
                        catch (IOException ioe) {
                            ioe.printStackTrace();
                        }
                    }
                    activeStatus[0].setText("Active");
                }
                else {
                    sb.append("accounting/acctLog");
                    StringBuffer sb2 = new StringBuffer( sb.toString());
                    sb2.append("_onHold");
                    records = new File(sb.toString()+".xml");
                    if (records.exists()) {
                        records.renameTo( new File(sb2.toString()+".xml") );
                    }
                    activeStatus[0].setText("On hold");
                }
            }
            else {
                StringBuffer sb = new StringBuffer( AProps.getInstance().getRootDir());
                sb.append("/adm/");
//                System.out.println("for "+source.isSelected());
                
                if (source.isSelected()) {
                    sb.append("accounting/loginrecords");
                    StringBuffer sb2 = new StringBuffer( sb.toString());
                    sb2.append("_onHold");
                    records = new File(sb2.toString()+".txt");
                    if (records.exists()) {
                        records.renameTo( new File(sb.toString()+".txt") );
                    }
                    else {
                        records = new File(sb.toString()+".txt");
                        if (records.exists()) {
                            return;
                        }
                        try { // create an empty file
                            String os,cmd;
                            records.createNewFile();
                             os = System.getProperty("os.name");
                            if (os.compareToIgnoreCase("SunOS")==0 ||
                                os.compareToIgnoreCase("Linux")==0) {
                                cmd = new String("chmod 666 "+sb.toString()+".txt");
                                Runtime rt = Runtime.getRuntime();
                                rt.exec(cmd);
                            }
                        }
                        catch (IOException ioe) {
                             ioe.printStackTrace();
                        }
                    }
                    activeStatus[1].setText("Active");
                }
                else {
                    sb.append("accounting/loginrecords");
                    StringBuffer sb2 = new StringBuffer( sb.toString());
                    sb2.append("_onHold");
                    records = new File(sb.toString()+".txt");
                    if (records.exists()) {
                        records.renameTo( new File(sb2.toString()+".txt") );
                    }
                    activeStatus[1].setText("On hold");
                }
             }
        }
    }
    
     private class ReduceJRBEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            if (ae.getSource() == reduceJRB[0])
                reduceMode = Config.LOGIN_REDUCE_FILE;
            else
                reduceMode = Config.ACQ_REDUCE_FILE;
        }
     }
     
     private class BillModeEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            if (billmodeJRB[0].isSelected()) {
                prop.setBillingMode("login");
                prop.setLoginBillingMode(true);
            }
            else {
                prop.setBillingMode("goes");
                prop.setLoginBillingMode(false);
            }
            if (billmodeJRB[1].isSelected()) {
                prop.setBillingMode("goes");
                prop.setGoesBillingMode(true);
            }
            else
                prop.setGoesBillingMode(false);
            ReadLast.getInstance(ai).readRecords(5);
        }
     }
     
     private class HeaderEar implements DocumentListener {
         public void insertUpdate(DocumentEvent de) { 
             System.out.println("DocumnetEvent: insert");
             fireChange(de);
         }
         public void removeUpdate(DocumentEvent de) {
             System.out.println("DocumnetEvent: remove");
             fireChange(de);
         }
         public void changedUpdate(DocumentEvent de) {
            System.out.println("DocumnetEvent: changed");
            fireChange(de);
         }
         private void fireChange(DocumentEvent de) {
            prop.setHeaders(headerJTF);
            mtm.fireTableStructureChanged();
            System.out.println("Header "+headerJTF[1].getText());
         }
     }
     
    private class ReduceGoEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            Date d = reduceToDate.getDate();
            System.out.println("Up to date: '"+d.toString());
            StringBuffer sb = new StringBuffer( AProps.getInstance().getRootDir()+"/adm/");

             /*************
//            if (reduceMode == Config.LOGIN_REDUCE_FILE) {
//                loginReduce(sb,d);
//            }
//            else {
            goesReduce(sb,d);
//            }
             *************/

              reduceRecords(sb,d);
        }
        
        private void loginReduce(StringBuffer sb, Date d) {
            long uptoTime, tmpTime;
            Date tmpDate;
            File largeFile, smallFile;
            String oneLine,dateString;
            int i, j,k;
            int total=0, remain=0;
            uptoTime = d.getTime()/1000;
//            System.out.println("Upto: "+ uptoTime);
            sb.append("accounting/loginrecords");
            largeFile = new File( sb.toString() + ".txt");
            if ( ! largeFile.exists() ) return;
            smallFile = new File( sb.toString() + ".sml");
            try {
                BufferedReader br = new BufferedReader (new FileReader (largeFile) );
                BufferedWriter bw = new BufferedWriter (new FileWriter (smallFile) );
                oneLine = br.readLine();
                System.out.println(oneLine);
                while (oneLine != null) {
                    total++;
                    i = oneLine.indexOf("start");
                    if (i == -1)
                        i = oneLine.indexOf("done");
                    i = oneLine.indexOf((int)'\"',i);
                    j = oneLine.indexOf((int)'\"',i+1);
                    if ((i>-1) && (j>-1)) {
                        dateString = oneLine.substring(i+1,j);
                        tmpTime = Long.parseLong(dateString);
                        if (tmpTime >= uptoTime) {
                            System.out.print(" keeper ! ");
                            bw.write(oneLine); bw.newLine();
                            remain++;
                        }
                        System.out.println(dateString); 
                    }
                    oneLine = br.readLine();
                    System.out.println(oneLine);
                }
                br.close();
                bw.close();
                String cutString = new String("Cutting "+Integer.toString(total-remain)+" login records");
                JOptionPane pane = new JOptionPane();
                int result = pane.showConfirmDialog((JPanel)conf, cutString,
                        "Cutting Records",   JOptionPane.YES_NO_OPTION);
//        System.out.println("result="+result);
               if (result == JOptionPane.YES_OPTION) {
//                  System.out.println("Renaming "+smallFile.getName()+" to "+largeFile.getName());
                  largeFile.delete();
                  boolean a = smallFile.renameTo(largeFile);
//                  System.out.println("Renaming returned "+ a);
               }
               else {
                    smallFile.delete();
               }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
        
        private void goesReduce(StringBuffer sb, Date reduceDate) {
            ArrayList aList = new ArrayList();
            Date tmpDate = null;;
            File largeFile, smallFile;
            String oneLine,tmpString,dateString, str;
            StringTokenizer tok;
            boolean include;
            int i,j;
            int total=0, remain=0;
            BufferedReader br = null;
            BufferedWriter bw = null;
    
//            System.out.println("Upto: "+ reduceDate.toString());
            sb.append("accounting/acctLog");
            SimpleDateFormat sdf = new SimpleDateFormat("EEE MMM d HH:mm:ss zzz yyyy");
            largeFile = new File( sb.toString() + ".xml");
            if ( ! largeFile.exists() ) {
                System.out.println("File: "+largeFile.getName() +" does not exist");
                return;
            }
            smallFile = new File( sb.toString() + ".sml");
            try {
                br = new BufferedReader (new FileReader (largeFile) );
                bw = new BufferedWriter (new FileWriter (smallFile) );
                oneLine = br.readLine();
                while (oneLine != null) { //copy header stuff
                    if ( ! oneLine.startsWith("<entry") ) {
                        bw.write(oneLine);    bw.newLine();
                        // System.out.println(oneLine);
                        oneLine = br.readLine();
                    }
                    else {
                        break;
                    }
                }
                include = false;
                while (oneLine != null) {
                   if (!include) {
                      i = oneLine.indexOf("start");
                      if (i >= 0) {
                         str = oneLine.substring(i);
                         tok = new StringTokenizer(str, " =\n");
                         str = tok.nextToken().trim();
                         if (!str.equals("start"))
                             i = -1;
                      }
                      else {
                         i = oneLine.indexOf("end");
                         if (i >= 0) {
                             str = oneLine.substring(i);
                             tok = new StringTokenizer(str, " =\n");
                             str = tok.nextToken().trim();
                             if (!str.equals("end"))
                                 i = -1;
                         }
                      }
                      if (i >= 0) {
                         i = oneLine.indexOf((int)'\"',i);
                         j = oneLine.indexOf((int)'\"',i+1);
                         if ((i>-1) && (j>-1)) {
                            dateString = oneLine.substring(i+1,j);
                            ParsePosition pp = new ParsePosition(0);
                            tmpDate = sdf.parse(dateString,pp);
                            if (reduceDate.compareTo(tmpDate) <= 0)
                                include = true;
                         }
                      }
                    }
                    if ( oneLine.startsWith("<entry") ) {
                        total++;
                        if ( include ) {
                            for (i=0; i< aList.size(); i++) {
                                bw.write((String)aList.get(i));   bw.newLine();
                            }
                            remain++;
                            include = false;
                        }
                        aList.clear();
                    }
                    aList.add(oneLine);
                    oneLine = br.readLine();
                }
                // check the last one, includes trailing lines
                str = null;
                if ( include && aList.size() > 0) {
                    remain++;
                    for (i=0; i< aList.size(); i++) {
                        bw.write((String)aList.get(i));   bw.newLine();
                    }
                    str = (String)aList.get(aList.size() - 1);
                }
                // Terminate xml file with closing line
                if (str == null && aList.size() > 0) {
                    str = (String)aList.get(aList.size() - 1);
                }
                if (str != null) {
                     if (str.indexOf("</accounting") < 0)
                         str = null;
                }
                if (str == null)
                    bw.write("</accounting>\n");
                br.close();
                br = null;
                bw.close();
                bw = null;
                String cutString = new String("Cutting "+Integer.toString(total-remain)+" records");
                JOptionPane pane = new JOptionPane();
                int result = pane.showConfirmDialog((JPanel)conf, cutString,
                        "Cutting Records",   JOptionPane.YES_NO_OPTION);
                if (result == JOptionPane.YES_OPTION) {
                    largeFile.delete();
                    smallFile.renameTo(largeFile);
                }
                else {
                    smallFile.delete();
                }
            }
            catch (IOException ioe) {
                ioe.printStackTrace();
            }

            finally {
            try {
               if (br != null)
                   br.close();
               if (bw != null)
                   bw.close();
            }
            catch (Exception e2) {}
        }
      }
    }

    private class SaxContentHandler extends DefaultHandler  {
        private StringBuffer strBuf = null;
        BufferedWriter fOut = null;
        SimpleDateFormat sdf;
        SimpleDateFormat sdfNoZone;
        int totalNum = 0;
        int remainNum = 0;

        public SaxContentHandler(BufferedWriter writer) {
             super();
             this.fOut = writer;
             sdf = new SimpleDateFormat("EEE MMM d HH:mm:ss zzz yyyy");
             sdfNoZone = new SimpleDateFormat("EEE MMM d HH:mm:ss yyyy");
        }

        public void endDocument() {
        }

        public void endElement(String name) {
        }

        public void startDocument() {
        }

        // reduceTimeMills = reduceDate.getTime();
        // uri is Namespace
        // localName The local name (without prefix), or the
        //    empty string if Namespace processing is not being performed.
        // qName The qualified name (with prefix), or the empty string if
        //    qualified names are not available

        public void startElement(String uri, String localName,
                                 String gName, Attributes atts ) {
             if (!gName.equals("entry"))
                 return;
             String aName, aValue;
             long startMills = 0;
             long endMills = 0;
             int i;
             Date date;

             totalNum++;

             aValue = atts.getValue("startsec");
             if (aValue != null) {
                 startMills = (long) (Double.parseDouble(aValue) * 1000.0);
             }
             else {
                 aValue = atts.getValue("start");
                 if (aValue != null) {
                     try {
                         date = sdf.parse(aValue, new ParsePosition(0));
                     }
                     catch(Exception ex) {
                         date = null;
                     }
                     if (date == null) {
                         date = sdfNoZone.parse(aValue, new ParsePosition(0));
                         if (date == null)
                             return;
                      }
                      startMills = date.getTime();
                 }
             }
             if (startMills > 0) {
                if (startMills < reduceTimeMills) {
                     return;
                }
             }
             aValue = atts.getValue("endsec");
             if (aValue != null) {
                 endMills = (long) (Double.parseDouble(aValue) * 1000.0);
             }
             else {
                 aValue = atts.getValue("end");
                 if (aValue != null) {
                     try {
                         date = sdf.parse(aValue, new ParsePosition(0));
                     }
                     catch(Exception ex) {
                         date = null;
                     }
                     if (date == null) {
                         date = sdfNoZone.parse(aValue, new ParsePosition(0));
                         if (date == null)
                             return;
                      }
                      endMills = date.getTime();
                 }
             }
             if (endMills > 0) {
                if (endMills < reduceTimeMills)
                    return;
             }
             remainNum++;

             try {
                fOut.write("<");
                fOut.write(gName);
                fOut.write("\n");
                for (i = 0; i< atts.getLength(); i++) {
                    aName = atts.getQName(i);
                    aValue = atts.getValue(i);
                    if (aName.equals("start")) {
                       if (startMills > 0) {
                           date = new Date(startMills);
                           aValue = date.toString();
                       }
                    }
                    else if (aName.equals("end")) {
                       if (endMills > 0) {
                           date = new Date(endMills);
                           aValue = date.toString();
                       }
                    }
                    fOut.write("       ");
                    fOut.write(aName);
                    fOut.write("=\"");
                    fOut.write(aValue);
                    fOut.write("\"\n");
                }
                fOut.write("/>\n");
             }
             catch (IOException ioe) {
                ioe.printStackTrace();
             }
        }

        public int getEntryNumber() {
             return totalNum;
        }

        public int getValidEntryNumber() {
             return remainNum;
        }
    }
}
