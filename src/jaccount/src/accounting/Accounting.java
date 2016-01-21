/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package accounting;

import java.io.File;
import java.awt.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.*;


public class Accounting extends JPanel {
    
  Billing bill;
  Accounts2 stp;
  public Config conf;
  AbstractTableModel atm;
  private static JComponent instance = null;
  private static Component  topComp = null;
  private JTabbedPane jtp;
  private boolean bAccountPanel = false;

  public Accounting(AccountingTab jsp) {

    File accountsDir = new File(AProps.getInstance().getRootDir()+"/adm/accounting/accounts");
/*    if (!accountsDir.exists()) {
      accountsDir.mkdirs();
      System.out.println("Created " + accountsDir.getAbsolutePath());
    }
    System.out.println("Accounting..."+accountsDir.getAbsolutePath());
*/
    setSize(500, 600);
    jtp = new JTabbedPane(JTabbedPane.TOP/* ,JTabbedPane.SCROLL_TAB_LAYOUT */);
    
    bill = new Billing(accountsDir, jsp);
    stp = new Accounts2(accountsDir,bill); 
    conf = new Config((AbstractTableModel)stp.getSpinTable().getModel(), bill.ai);

    jtp.addTab("Billing",null,bill,"Write One or More Bills");
    jtp.addTab("Accounts",null,stp,"Maintain Accounts");
    jtp.addTab("Properties",null,conf,"Set Configuration and Enable/Disable");

    add(jtp);
    instance = (JComponent) this;
    jtp.addChangeListener(new ChangeListener() {
         public void stateChanged(ChangeEvent e) {
             tabChanged();
         }
    });
  }
  
  Billing getBilling() {
      return (bill);
  }

  public static JComponent getInstance() {
       return instance;
  }

  public static void showErrorMsg(String str) {
       if (instance != null) {
            if (topComp == null) {
               Container c = instance.getParent();
               while (c != null) {
                   if (c.getParent() == null)
                       break;
                   c = c.getParent();
               }
               topComp = c;
            }
            JOptionPane.showMessageDialog(topComp,
            str, "Error",
            JOptionPane.ERROR_MESSAGE);
       }
       else
           System.out.println(str);
  }

  private void tabChanged() {
        Component comp = jtp.getSelectedComponent();
        if (comp != stp) {
            if (bAccountPanel)
               stp.saveAccountInfo();
            bAccountPanel = false;
        }
        else
            bAccountPanel = true;
  }

}
