// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
package accounting;

import java.io.*;
import java.text.*;
import java.util.*;
import javax.swing.*;
import java.awt.Image;

public class AProps extends Properties {

  private static AProps aProps;
  private NumberFormat printMoney = null;
  private NumberFormat printNumber = null;
  private ImageIcon logo;
  private static String rootDir = null;
  private String[] tableHeaders = null;
  private String billingMode = null;
  private String goesBillingMode = null;
  private String loginBillingMode = null;
  public static String loginBillingKey = "loginbillingmode";
  public static String goesBillingKey = "goesbillingmode";

  public static void main(String[] args) {
    AProps aProps1 = new AProps();
    aProps1.test();
  }

  private AProps() {
    try {
        load(new FileInputStream(getRootDir()+"/adm/accounting/accounts/accounting.prop"));
    } catch (IOException ioe) {
      ioe.getStackTrace();
    }
  }

  public static AProps getInstance() {
    if (aProps == null) {
      aProps = new AProps();
    }
    return aProps;
  }
  
  public static AProps reset() {
     aProps = null;
     getInstance();
     return aProps;
  }
  

  private void test() {
    String str1 = getProperty(new String("currency"));
    String str2 = getProperty("numberFormat");
    System.out.println("currency = '"+str1+"' numberFormat ='"+str2+"'");
    double value = 123456.78;
    DecimalFormat df = new DecimalFormat(str2);
    str2 = df.format(value);
    System.out.println(str1+" "+str2);

    String country = str1;
    Locale l = new Locale(country);
    String lang = l.getLanguage();
    l = new Locale(lang,country);
    NumberFormat df2 = NumberFormat.getCurrencyInstance(l);
    System.out.println("df2 has '"+df2.getCurrency()+"'");
    Currency newC = Currency.getInstance(df2.getCurrency().toString());
    df2.setCurrency(newC);
    String v2 = df2.format(value);
    System.out.println("second mode: '"+v2+"'");
    
    String[] t = getTableHeaders();
    for (int i=0; i<4; i++) {
          System.out.println("tableheaders["+i+"] = '"+t[i]+"'");
    }
    
    String b = getBillingMode();
    System.out.println("billingmode = '"+b+"'");
  }
  
  NumberFormat getCurrencyFormat()
  {
    if (printMoney != null) return printMoney;

    String country = (String)get("currency");
    Locale l = new Locale( country );
    String lang = l.getLanguage();
    l = new Locale(lang,country);
    NumberFormat printMoney = NumberFormat.getCurrencyInstance(l);
//    System.out.println("df2 has '"+nf.getCurrency()+"'");
    Currency newC = Currency.getInstance(printMoney.getCurrency().toString());
    printMoney.setCurrency(newC);
    return printMoney;
  }
  
  void setCurrencyFormat(String country) {
      if (country.compareTo("")== 0) return;
      setProperty("currency",country);
  }
  

  NumberFormat getNumberFormat() {
    if (printNumber != null) return printNumber;

    String country = (String)get("currency");
    Locale l = new Locale( country );
    String lang = l.getLanguage();
    l = new Locale(lang,country);
    NumberFormat printNumber = NumberFormat.getNumberInstance(l);
//    System.out.println("df2 has '"+nf.getCurrency()+"'");
//    Currency newC = Currency.getInstance(printMoney.getCurrency().toString());
//    printMoney.setCurrency(newC);
    printNumber.setMaximumFractionDigits(2);
    printNumber.setMinimumFractionDigits(2);
    return printNumber;
  }
  
  public ImageIcon logo() {
    if (logo == null) {
      String fln = new String(getRootDir()+"/adm/accounting/"+get("logo"));
      // See if the logo file exists, if not, use a default
      File file = new File(fln);
      if (!file.exists()) {
          fln = new String(getRootDir()+"/adm/accounting/vnmrjNameBW.png");
      }
      logo = new ImageIcon( fln );

    }
    return logo;
  }
  
  void setLogo(String newLogo) {
      if (newLogo.compareTo("")== 0) return;
      setProperty("logo",newLogo);
      String fln = new String(getRootDir()+"/adm/accounting/"+get("logo"));
      logo = new ImageIcon( fln );
      System.out.println("New logo is "+newLogo);
  }

  static public String getRootDir()  {
      if (rootDir == null) {
          rootDir = new String("./");
          String os = System.getProperty("os.name");
          if ( ( os.compareToIgnoreCase("SunOS") == 0) ||
               ( os.compareToIgnoreCase("Linux") == 0) )
              rootDir = System.getenv("vnmrsystem");
          if (os.compareToIgnoreCase("Windows XP") == 0)
              rootDir = System.getenv("vnmrsystem_win");
      }
      return(rootDir);
  }
 
  public String[] getTableHeaders() {
      String oneLine;
      if (tableHeaders == null) {
          tableHeaders = new String[4];
          oneLine = (String) get("tableheaders");
          StringTokenizer st = new StringTokenizer(oneLine,"\"");
          if(st.hasMoreTokens()) {
              tableHeaders[0] = st.nextToken().trim();
              if(st.hasMoreTokens())
                  st.nextToken();
          }
          if(tableHeaders[0] == null || tableHeaders[0].equals("") || tableHeaders[0].equals("\t")) {
              tableHeaders[0] = "Day";
          }
          if(st.hasMoreTokens()) {
              tableHeaders[1] = st.nextToken().trim();    
              if(st.hasMoreTokens())
                  st.nextToken();
          }
          if(tableHeaders[1] == null || tableHeaders[1].equals("") || tableHeaders[1].equals("\t")) {
              tableHeaders[1] = "Time";
          }
          if(st.hasMoreTokens()) {
              tableHeaders[2] = st.nextToken().trim();    
              if(st.hasMoreTokens())
                  st.nextToken();
          }
          if(tableHeaders[2] == null || tableHeaders[2].equals("") || tableHeaders[2].equals("\t")) {
              tableHeaders[2] = "Login Rate";
          }
          if(st.hasMoreTokens()) {
              tableHeaders[3] = st.nextToken().trim();
          }
          if(tableHeaders[3] == null || tableHeaders[3].equals("") || tableHeaders[3].equals("\t")) {
              tableHeaders[3] = "Login/hr";
          }
      }
      return (tableHeaders);
  }
  
  public void setHeaders(JTextField[] headers) {
      tableHeaders[0] = headers[0].getText();
      tableHeaders[1] = headers[1].getText();
      tableHeaders[2] = headers[2].getText();
      tableHeaders[3] = headers[3].getText();
  }
  
  public String getBillingMode() {
      if (billingMode == null) {
          billingMode = ((String) get("billingmode")).trim();
      }
      return (billingMode);
  }

  void setBillingMode(String newBillmode) {
      setProperty("billingmode",newBillmode);
      billingMode = newBillmode;
  }

  void setGoesBillingMode(boolean b) {
      if (b)
          goesBillingMode = "true";
      else
          goesBillingMode = "false";
      setProperty(goesBillingKey, goesBillingMode);
  }

  public boolean isGoesBillingMode() {
      getBillingMode();
      if (goesBillingMode == null) {
          goesBillingMode = ((String) get(goesBillingKey));
          if (goesBillingMode == null)
              goesBillingMode = "false";
      }
      if (goesBillingMode.equals("true") || billingMode.equals("goes"))
         return true;
      return false;
  }

  void setLoginBillingMode(boolean b) {
      if (b)
          loginBillingMode = "true";
      else
          loginBillingMode = "false";
      setProperty(loginBillingKey, loginBillingMode);
  }

  public boolean isLoginBillingMode() {
      getBillingMode();
      if (loginBillingMode == null) {
          loginBillingMode = ((String) get(loginBillingKey));
          if (loginBillingMode == null)
              loginBillingMode = "false";
      }
      if (loginBillingMode.equals("true") || billingMode.equals("login"))
         return true;
      return false;
  }
  
  Double toMinutes(String time) {
      int plusPos,lastPos;
      double t;
      String days, hours, minutes,seconds;
      plusPos = time.indexOf('+');
      lastPos = time.length();
      if (plusPos > 0) {
        days = time.substring(1,plusPos);
        t = (long)Integer.parseInt(days)*24*60;
      }
      else {
        days=null;
        plusPos = 0;
        t=0;
      }
//      System.out.print(time+":"+plusPos+","+colonPos+","+lastPos+" ");
      minutes = time.substring(plusPos,lastPos);
//      System.out.println("toMinutes: "+minutes);
      StringTokenizer st = new StringTokenizer(minutes,":()");
      t += Double.parseDouble( st.nextToken() )*60.0;  // hours
//      System.out.println("toMinutes: "+t);
      t += Double.parseDouble( st.nextToken() );       // minutes
//      System.out.println("toMinutes: "+t);
      if (st.hasMoreTokens()) {
          t += Double.parseDouble( st.nextToken() ) / 60.0;
      }
//      System.out.println("toMinutes: "+t);
      return( new Double(t) );
  }
}
