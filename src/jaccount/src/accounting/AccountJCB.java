/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package accounting;

import java.awt.event.*;
import java.io.*;
import java.text.*;
import java.util.*;
import javax.swing.*;
import java.util.Iterator;
import java.util.TreeSet;

public class AccountJCB extends JComboBox{
  private AccountInfo2 ai;
  private File        accountsDir;
  private RateEntry   re;
  private String      lastFileRead;
  private String      filePath;
  private long        lastFileDate;
  private  String[]    accounts;
  TreeSet <String>tree = new TreeSet<String>();

  public AccountJCB(File accountsDir, boolean editable) {
    int iTmp;
    lastFileRead = new String("None");
    this.accountsDir = accountsDir;
    ai = new AccountInfo2();
    re = new RateEntry();
    accounts = accountsDir.list();
    for (int n = 0; n < accounts.length; n++) {
        tree.add(accounts[n]);
    }

    Iterator iterator;
    iterator = tree.iterator();
    while (iterator.hasNext()) {
      String data = (String)iterator.next();  
      iTmp = data.length();
      if ( (iTmp > 4) && data.substring(iTmp-4,iTmp).equals(".txt") ){
        String name = data.substring(0, data.length() - 4);
        this.addItem(name);
      }
    }
    this.setEditable(editable);
  }

  public AccountInfo2 read(String str1) {
    File jfile;
    if (str1.compareTo(lastFileRead) == 0) {
        jfile = new File(filePath);
        if (lastFileDate == jfile.lastModified()) {
            return(ai);
        }
    }
    ai = new AccountInfo2();
    ai.account(str1);
    filePath = AProps.getInstance().getRootDir()+"/adm/accounting/accounts/"+str1+".txt";
    jfile = new File(filePath);
    if (!jfile.exists())
        return(ai);
    lastFileDate = jfile.lastModified();
    try {
      BufferedReader bufIn = new BufferedReader(new FileReader(filePath));
      String str = bufIn.readLine(); // seed
      while (str != null && (str.startsWith("#")) ) {
        str = bufIn.readLine();
        if (str==null) break;
      }
      if (str == null) {
          bufIn.close();
          return(ai);
      }
  
      ai.name(str);
      ai.address1(bufIn.readLine());
      ai.address2(bufIn.readLine());
      ai.city(bufIn.readLine());
      ai.zip(bufIn.readLine());
      str = bufIn.readLine().trim();
      // Allow backwards compatiblity with older profile files without country
      if(!str.startsWith("#"))
          ai.country(str);

      str = bufIn.readLine().trim();
      while (str.startsWith("#") || str.compareTo("")==0)
        str = bufIn.readLine();
      StringTokenizer st = new StringTokenizer("  ");
      while( (str != null) && ! str.startsWith("# Owners/op")) {
        re = new RateEntry();
        st = new StringTokenizer(str);
        re.day = st.nextToken();
        if (re.day.endsWith("."))
           re.day = re.day.substring(0,3);
        re.time = st.nextToken();
        re.login = st.nextToken();
        re.loginhr = st.nextToken();
//        re.go = st.nextToken();
//        re.gohr = st.nextToken();
        ai.rates(re);
        str = bufIn.readLine();
        if (str != null)
          str = str.trim();
        else
            break;
      }
      str = bufIn.readLine();
      if (str != null) {
        ai.oAndOs(str);
      }
      else {
        ai.oAndOs( new String("operators "));
      }
      bufIn.close();
    } catch (FileNotFoundException fnf) {
      // If file does not exist, and editable is true,that is ok:
      // ignore exception and continue
      // If  not editable, we should never get here, unless someone
      // deletes the file while running this program
//      if ( ! editable ) {
//        JOptionPane pane = new JOptionPane();
//        pane.showMessageDialog(stf, "File '"+str1+"' not Found");
//      }
      return(ai);
    } catch (IOException ioe) {
      System.out.println("IO Exception");
//      ioe.getStackTrace();
      return(null);
    }
    lastFileRead = str1;
    // fill in int dayOfWeek and nMinutes (how long rate is valid)
    int n = ai.nrates();
    int jday=0;
    RateEntry tmpRE = ai.rates(n-1);
    DateFormatSymbols dfs = new DateFormatSymbols();
    String[] days = dfs.getShortWeekdays();
    Double t = new Double(0.0), t0 = new Double(0.0), t1 = new Double(0.0); 
    for (int i=0; i<n; i++) {
        tmpRE = ai.rates(i);
        for (int j=1; j<days.length; j++) {
            if (tmpRE.day.compareTo(days[j])==0) {
                jday=j;
                break;
            }
        }
        tmpRE.dayOfWeek(jday);
        if (i==0) {
           t0 = AProps.getInstance().toMinutes(tmpRE.time()) + (jday-1) * 24 * 60;
           t = t0;
        }
        else {
           t1 = AProps.getInstance().toMinutes(tmpRE.time()) + (jday-1) * 24 * 60;
           t = t1 - t;
           ai.rates(i-1).nMinutes(t);
           t = t1;
        }
    }
    if (n > 1) {
        t = 7 * 24 * 60.0  - t1;
    }
    else {
        t = 7 * 24 * 60.0;
    }
    ai.rates(n-1).nMinutes(t);
   
    return(ai);
  }

  public AccountInfo2 read() {
    if (getSelectedIndex() < 0)
        return(ai);
    String str = (String)getSelectedItem().toString().trim();
    return read(str);
  }
}
