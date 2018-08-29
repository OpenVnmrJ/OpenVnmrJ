/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2005</p>
 * <p>Company: </p>
 * @author not attributable
 * @version 1.0
 */
package accounting;

import java.awt.*;
import java.awt.event.*;
import java.awt.print.*;
import java.io.*;
import javax.swing.*;

public class AccountingTab extends JSplitPane {
    
  public Accounting acc;
  private JPanel billPanel,mainPanel;
  private JScrollPane jsp;
  public JTabbedPane billTabs;
  
  private AProps aProps;
  private ImageIcon logo;
  private JLabel logoLabel;
  
  private JLabel lastBill;
  private JLabel fromToDates;
    
  public AccountingTab() {
//        System.out.println("AccountingTab");
        setDividerLocation(420);
        
        billTabs = new JTabbedPane();
        billTabs.setTabLayoutPolicy(JTabbedPane.SCROLL_TAB_LAYOUT);
        
        billPanel = new JPanel( new InitLayout() );
        billPanel.setBackground(Color.WHITE); // default to a blank white page
        
        aProps = AProps.getInstance();
        logo = aProps.logo();
        logoLabel = new JLabel(logo);    billPanel.add(logoLabel);
        lastBill = new JLabel("Last billing period was:");
                                         billPanel.add(lastBill);
                                         
        fromToDates = new JLabel();
        File lastBillDates = new File(aProps.getRootDir()+"/adm/accounting/last_bill_dates.txt");
        if ( lastBillDates.exists() ) {
//            System.out.println("Found lastBillDates file");
            try {
                BufferedReader br = new BufferedReader( new FileReader(lastBillDates));
                fromToDates = new JLabel(br.readLine());
            }
            catch (IOException ioe) {
                ioe.printStackTrace();
            }
        }
                                         
        billPanel.add(fromToDates);
        billTabs.addTab("Last",billPanel);
        
        setRightComponent(billTabs);
        
        acc = new Accounting(this);
        setLeftComponent(acc);
        
  }
  
  public JTabbedPane getBillTabs() {
      return (billTabs);
  }
  public class InitLayout implements LayoutManager {
      public void addLayoutComponent(String str, Component c) {}

      public void layoutContainer(Container parent) {
          int x = parent.getWidth();
          int y = parent.getHeight();
//      System.out.println("Page: "+x+","+y);
          int xIW = logo.getIconWidth();
          int yIH = logo.getIconHeight();
          logoLabel.setBounds(x-xIW-10,10,xIW,yIH);
          
          lastBill.setBounds(30,yIH+30,x-30,30);
          fromToDates.setBounds(30,yIH+50, x-30,30);
          
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
}
