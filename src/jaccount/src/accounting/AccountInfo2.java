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
import java.text.*;
import java.util.*;
import javax.swing.JLabel;

public class AccountInfo2 {

  private ArrayList  rates;
  
  private String     account;  // name of adm account
  private String     name;  // billing name
  private String     address1;
  private String     address2;
  private String     city = "";
  private String     zip;
  private String     country;
  private String     oAndOs;
  private double     total=0.0;
  private int        thisYear=0;

  public AccountInfo2() {
    rates  = new ArrayList();
    oAndOs = new String("operators ");
    name = "";
    account = "";
    address1 = "";
    country = "";
  }

  public void account(String account)   {this.account = account;}
  public String account()               {return this.account;}

  public void name(String name)         {this.name = name;}
  public String name()                  {return this.name;}

  public void address1(String address1) {this.address1 = address1;}
  public String address1()              {return this.address1;}

  public void address2(String address2) {this.address2 = address2;}
  public String address2()              {return this.address2;}

  public void city(String city)         {this.city = city;}
  public String city()                  {return this.city;}

  public void zip(String zip)           {this.zip = zip;}
  public String zip()                   {return this.zip;}

  public void country(String country)   {this.country = country;}
  public String country()               {return this.country;}

  public void rates(RateEntry r)        {this.rates.add(r);}
  public RateEntry rates(int i)         {return (RateEntry)rates.get(i);}
  public int nrates()                   {return rates.size();}

  public int rateCount() {return rates.size();}
  
  public double total()                 {return (this.total);}
  
  public void oAndOs( String os)         {this.oAndOs = os;}
  public String oAndOs()                {return (oAndOs); }

/* look for the rate in te account file for the time in the login string */
  public RateEntry getRate(String login) {
      int logday=0, rateday=0;
      //    System.out.println("getRate: login='"+login+"'");
      String day, time, rate=null;
      StringTokenizer st = new StringTokenizer(login);
      //frits     pts/21       flash            Fri Mar 18 13:46 - 14:30 (6+00:44)
      //                                        ^              ^
      day = st.nextToken();
      time = st.nextToken();
      time = st.nextToken();
      time = st.nextToken().substring(0,5);
      // System.out.println(login+" "+day+" "+time);
      // Get the international names of the days and which day is (1,2,3...
      // Then find day of each rate entry, (1,2,3,4
      // Now we can compare, independent of the language used for the days
      // Finally use the time to compare as well
      DateFormatSymbols dfs = new DateFormatSymbols();
      String[] days = dfs.getShortWeekdays();
      for (int j=1; j<days.length; j++) {
          if (day.compareTo(days[j])==0) {
              logday=j;
              break;
          }
      }
      // logday -> the day given in the gorecords.xml file (the log)

      int n = rates.size();
      // the last rate applies if before the first entry
      // Init to last rate given in the account
      RateEntry tmpRE,saveRE=rates(n-1);
      //    System.out.println("n="+n);
      for (int i=0; i<n; i++) {
          tmpRE = rates(i);
          for (int j=1; j<days.length; j++) {
              if (tmpRE.day.compareTo(days[j])==0) {
                  rateday=j;
                  break;
              }
          }
          //    System.out.println("j="+jday+" k="+kday+" "+tmpRE.time+" "+tmpRE.loginhr);
          // the last rate applies if before the first entry
          if (logday > rateday) {
              saveRE = tmpRE;
          }
          if ( (logday == rateday) && (time.compareTo(tmpRE.time) >= 0) ) {
              saveRE = tmpRE;
          }
      }
      return saveRE;
  }
  
  int iRE = 0, tRE=0;
  Double getCharge( String date, double howLong) {
      int jday=0;
      Double nMins;
      String day, time, rate=null;
      double remain = howLong;
      StringTokenizer st = new StringTokenizer(date);
      day = st.nextToken();
      time = st.nextToken();
      time = st.nextToken();
      time = st.nextToken().substring(0,5);
      nMins = AProps.getInstance().toMinutes(time);
      DateFormatSymbols dfs = new DateFormatSymbols();
      String[] days = dfs.getShortWeekdays();
      for (int j=1; j<days.length; j++) {
          if (day.compareTo(days[j])==0) {
              jday=j;
              break;
          }
      }
      int n = rates.size(),tableNMin;
      // One Rate for the entire Login session
      if (n==1) {  // only one flat rate
          // Get time in hours
          Double howLongHr = howLong / 60.0;
          // Calc cost of logon time
          Double cost = new Double(rates(0).loginhr).doubleValue()*howLongHr;
          // Cost of each login
          Double login = new Double(rates(0).login).doubleValue();
          cost = cost + login;
          return (cost);
      }
      // Multiple Rates for this login session
      // Default to last rate in list so that if last rate is eg., Fri and
      // the current day is Sun, it will still use Friday.
      iRE = n-1;
      RateEntry tmpRE,saveRE=rates(n-1);
      for (int i=0; i<n; i++) {
          tmpRE = rates(i);
          if (tmpRE.dayOfWeek() < jday) {
              saveRE = tmpRE;
              iRE = i;
          }
          tableNMin = (AProps.getInstance().toMinutes(tmpRE.time())).intValue();
          if ( (tmpRE.dayOfWeek() == jday) && ( nMins.intValue() > tableNMin) ) {
              saveRE = tmpRE;
              iRE = i;
          }
      }
      tRE = iRE+1; // iRE set rate, tRE set time until
      if (tRE >= n) tRE=0;
      Double charge;
      int ndays;
      double diffMins ;
      if (tRE==0) {
          ndays = rates(0).dayOfWeek() + (7-jday);
      }
      else {
          ndays = rates(tRE).dayOfWeek() - jday;
      }
      if(ndays > 7)
          ndays = ndays - 7;
      int nmin = AProps.getInstance().toMinutes(rates(tRE).time()).intValue() - nMins.intValue();
      diffMins = ndays*24*60 + nmin;
      if (remain < diffMins) {
          diffMins = remain;
      }
      // Calc charge for the first period of time up to the next rate change
      Double hours = new Double(diffMins/60.0);
      charge = new Double( hours * new Double(rates(iRE).loginhr()));
      // Add in the per session login rate
      charge +=  new Double(rates(iRE).login());
      remain -= diffMins;
      
      
      // now we have the charge for the first increment including the login,
      // calculate changes for remaining periods using their rates
      while ( remain > 0.0 ) {
          
          if ( (remain - rates(tRE).nMinutes()) >=  0) {
              
              charge += rates(tRE).nMinutes() * new Double(rates(tRE).loginhr()) / 60.0;
          }
          else {
              
              charge += remain * new Double(rates(tRE).loginhr()) / 60.0;
          }
          remain -= rates(tRE).nMinutes();
          iRE++;   if (iRE>=n) iRE=0;
          tRE++;   if (tRE>=n) tRE=0;
      }
      
      return (charge);
  }
}
