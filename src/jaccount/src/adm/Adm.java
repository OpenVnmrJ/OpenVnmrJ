/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Adm.java
 *
 * Created on April 9, 2005, 8:53 AM
 */

/**
 * @author Frits Vosman
 */

package adm;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.text.*;
import java.util.*;
import javax.swing.*;
import accounting.*;

public class Adm extends JFrame implements WindowListener {
    
    Container c;
    AccountingTab aTab;
    private static String saveDir = null;
    private static String inFile = null;
    private static Date startDate = null;
    private static Date endDate = null;
    private static boolean cutCmd = false;
    private static boolean summaryCmd = false;
    private static boolean summaryCsvCmd = false;
    private static boolean acctCsvCmd = false;


    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        boolean showPanels = true;

        if (args.length > 0)
        {
           int i = 0;
           while (i < args.length)
           {
              if(args[i].equalsIgnoreCase("-saveDir") && args.length > i+1) {
                 i++;
                 saveDir = args[i];
                 showPanels = false;
              } else if(args[i].equalsIgnoreCase("-in") && args.length > i+1) {
                 i++;
                 inFile = args[i];
                 AProps.getInstance().setInFile( args[i] );
              } else if(args[i].equalsIgnoreCase("-startDate") && args.length > i+1) {
                 i++;
                 SimpleDateFormat sdf = new SimpleDateFormat("MMM d yyyy");
                 try {
                    if (args[i].equalsIgnoreCase("today")) {
                       String sdate = sdf.format(new Date());
                       startDate = sdf.parse(sdate);
                    } else {
                       startDate = sdf.parse(args[i]);
                    }
                    if ( startDate == null )
                    {
                       System.out.println("-startdate syntax error: " + args[i]);
                       System.exit(0);
                    }
                 }
                 catch(Exception ex) {
                    System.out.println("-startdate syntax error: " + args[i]);
                    System.exit(0);
                 }
                 showPanels = false;
              } else if(args[i].equalsIgnoreCase("-endDate") && args.length > i+1) {
                 i++;
                 SimpleDateFormat sdf = new SimpleDateFormat("MMM d yyyy");
                 try {
                    if (args[i].equalsIgnoreCase("today")) {
                       String edate = sdf.format(new Date());
                       endDate = sdf.parse(edate);
                    } else {
                       endDate = sdf.parse(args[i]);
                    }
                    if ( endDate == null )
                    {
                       System.out.println("-enddate syntax error: " + args[i]);
                       System.exit(0);
                    }
                 }
                 catch(Exception ex) {
                    System.out.println("-enddate syntax error: " + args[i]);
                    System.exit(0);
                 }
                 showPanels = false;
              } else if(args[i].equalsIgnoreCase("-cut")) {
                 cutCmd = true;
                 showPanels = false;
              } else if(args[i].equalsIgnoreCase("-summary")) {
                 summaryCmd = true;
                 showPanels = false;
              } else if(args[i].equalsIgnoreCase("-summaryCsv")) {
                 summaryCsvCmd = true;
                 showPanels = false;
              } else if(args[i].equalsIgnoreCase("-acctCsv")) {
                 acctCsvCmd = true;
                 showPanels = false;
              }
              i++;
           }
           if (saveDir != null )
           {
                 File toDir = new File(saveDir);
                 if (!toDir.exists())
                   toDir.mkdirs();
           }
        }
        Adm adm = new Adm(showPanels);
    }
 
    /** Creates a new instance of Adm */
    public Adm(boolean showPanels){
         if (inFile == null)
            setTitle("VnmrJ Accounting");
         else
            setTitle("VnmrJ Accounting using "+inFile);
         // setSize(1000,680);
         ImageIcon icon = new ImageIcon(AProps.getInstance().getRootDir()+"/adm/accounting/accounting.gif");
         if (icon != null)
            setIconImage(icon.getImage());
         
   //      this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
         this.addWindowListener(this);
         c = getContentPane();
         
         // Check if certain items exist
         int error = 0;
         StringBuilder sb = new StringBuilder();
         String s = AProps.getInstance().getRootDir();
         if ((s == null) || s.length() < 5) {
             sb.append("Environment parameter 'vnmrsystem' not defined.\n");
             error++;
         }
         File dir = new File(s+"/adm/accounting/accounts");
         if ( ! dir.exists() ) {
             sb.append("Cannot find the '"+s+"/adm/accounts' directory, please create.\n");
             error++;
         } else {
        
            File aprop = new File(s+"/adm/accounting/accounts/accounting.prop");
            if ( ! aprop.exists() ) {
                sb.append("Cannot find the '"+s+"/adm/accounts/accounting.prop' file.\n");
                error++;
            }
         }
         
         if ( error > 0) {
             sb.append("\nSee manual for details");
             if (showPanels) {
                JOptionPane pane = new JOptionPane();
                pane.showMessageDialog(null, sb.toString(),"Error",JOptionPane.ERROR_MESSAGE);
             } else {
                System.out.println(sb);
             }
             return;
         }
         JTabbedPane jtp = new JTabbedPane();
         
//         UserTab uTab = new UserTab();
//         jtp.addTab("Users", null, uTab, "Maintain Users");
         
//         WalkupTab wTab = new WalkupTab();
//         jtp.addTab("Walkup", null, wTab, "Walk Up Configuration");
         
         aTab = new AccountingTab();
//         jtp.addTab("Accounting", null, aTab, "Billing Software");
         
         c.add(aTab);
         
         Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
         if (dim.width > 1200)
             setSize(1200, 680);
         else
             setSize(dim.width - 100, 680);
         if (showPanels)
         {
            setVisible(true);
         }
         else
         {
            if (cutCmd) {
               StringBuffer sb3 = null;
               StringBuffer sb2 = new StringBuffer( AProps.getInstance().getRootDir()+"/adm/accounting/");
               if (endDate == null)  // use today's date
               {
                  SimpleDateFormat sdf2 = new SimpleDateFormat("MMM d yyyy");
                  String edate = sdf2.format(new Date());
                  try {
                  endDate = sdf2.parse(edate);
                  }
                  catch(Exception ex) { }
               }
               Config.getInstance().reduceRecords(sb2, endDate, saveDir, false);
            } else  {
               if (startDate == null)  // use today's date
               {
                  SimpleDateFormat sdf2 = new SimpleDateFormat("MMM d yyyy");
                  try {
                     startDate = sdf2.parse("Jan 1 2001");
                  }
                  catch(Exception ex) { }
               }
               if (endDate == null)  // use today's date
               {
                  SimpleDateFormat sdf2 = new SimpleDateFormat("MMM d yyyy");
                  String edate = sdf2.format(new Date());
                  try {
                     endDate = sdf2.parse(edate);
                  }
                  catch(Exception ex) { }
               }
               if (saveDir == null)
               {
                 saveDir = AProps.getInstance().getRootDir()+"/adm/accounting";
               }
               Accounting.getBilling().acctOutput(startDate, endDate, saveDir,
                            summaryCmd, summaryCsvCmd, acctCsvCmd );
            }
            System.exit(0);
         }
    }
    
    public void windowActivated(WindowEvent we) {
//        System.out.println("Window Activated");
    }
    public void windowClosed(WindowEvent we) {
//        System.out.println("Window Closed");
    }
    
    // This one gets the Sve() ane Exit()?
    public void windowClosing(WindowEvent we) {
//        System.out.println("Window Closing");
        aTab.acc.conf.saveProperties();
        System.exit(0);
    }
    public void windowDeactivated(WindowEvent we) {
//        System.out.println("Window Deactivated");
    }
    public void windowDeiconified(WindowEvent we) {
//        System.out.println("Window Deiconified");
    }
    public void windowIconified(WindowEvent we) {
//        System.out.println("Window Iconified");
    }
    public void windowOpened(WindowEvent we) {
//        System.out.println("Window Opened");
    }
    
}
