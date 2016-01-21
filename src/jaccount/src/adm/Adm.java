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
import java.util.*;
import javax.swing.*;
import accounting.*;

public class Adm extends JFrame implements WindowListener {
    
    Container c;
    AccountingTab aTab;
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        Adm adm = new Adm();
    }
 
    /** Creates a new instance of Adm */
    public Adm(){
         setTitle("VnmrJ Accounting");
         // setSize(1000,680);
         ImageIcon icon = new ImageIcon(AProps.getInstance().getRootDir()+"/adm/accounting/accounting.gif");
         if (icon == null) System.out.println("icon is null");
         setIconImage(icon.getImage());
         
   //      this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
         this.addWindowListener(this);
         c = getContentPane();
         
         // Check if certain items exist
         int error = 0;
         StringBuilder sb = new StringBuilder();
         String s = AProps.getInstance().getRootDir();
         if ((s == null) || s.length() < 5) {
             sb.append("Environment parameter 'vnmrsystem' or 'vnmrsystem_win' not defined.\n");
             error++;
         }
         File dir = new File(s+"/adm/accounting/accounts");
         if ( ! dir.exists() ) {
             sb.append("Cannot find the '"+s+"/adm/accounts' directory, please create.\n");
             error++;
         }
         File aprop = new File(s+"/adm/accounting/accounts/accounting.prop");
         if ( ! aprop.exists() ) {
             sb.append("Cannot find the '"+s+"/adm/accounts/accounting.prop' file, reintall software.\n");
             error++;
         }
         
         if ( error > 0) {
             sb.append("\nSee manual for details");
             JOptionPane pane = new JOptionPane();
             pane.showMessageDialog(null, sb.toString(),"Error",JOptionPane.ERROR_MESSAGE);
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
         setVisible(true);
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
