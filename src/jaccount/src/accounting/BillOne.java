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
import java.awt.RenderingHints;
import java.io.*;
import java.text.*;
import java.util.*;
import javax.swing.*;

public class BillOne extends JPanel implements Printable {

  private AccountInfo2 ai;
  private AProps aProps;
  private ArrayList[] goesLines;
  private ArrayList[] loginLines;
  private ArrayList[] goesRates;
  private ArrayList[] loginRates;
  private double[] goesTimes;
  private double[] loginTimes;
  private double[] goesCharges;
  private double[] loginCharges;
  private ImageIcon logo;
  private JLabel logoLabel;
  private JLabel name,address1, address2, cityState, country;
  private JLabel accountNumber,billDate,dueDate,totalCost;
  private JLabel account,bill,due,cost;
  JPanel p;
  private JScrollPane jsp;
  private JSeparator js1,js2;
  private String billingPeriod = null;
  private int nLines;
  private int linesY0 = 0;
  private int prtY0 = 0;
  private int fontH = 12;
  private int prtFontH = 12;
  private int nOAndOs = 0;
  private int nameWidth = 0;
  private int pageWidth = 0;
  private int pageHeight = 0;
  private int pageNo = 0;
  private int prtLineIndex = 0;
  private int prtArrayIndex = 0;
  private int prtArrayNo = 0;
  private int prevLineIndex = 0;
  private int prevArrayIndex = 0;
  private int prevArrayNo = 0;

  private int MAX_LINES = 40;
  private Font  prtFont;
  private FontMetrics fm;
  private FontMetrics prtFm;
  private Color color0 = Color.BLUE;
  private Color color1 = Color.BLACK;
  private Color prtBg = new Color(220, 220, 220);
  private boolean bSummary = false;
  private boolean bGoes = false;
  private boolean bLogin = false;
  private double totalCharge = 0.0;
  private boolean bFirstPage = true;
  private boolean bPrintingAcq = false;
  private boolean bPrintingLogin = false;
  private boolean bPrevAcq = false;
  private boolean bPrevLogin = false;
  private boolean bPrevFirstPage = true;
  private String  maxName = " ";

  private Date startDate;
  private Date endDate;
  public static String acqTimeStr = "acquisition time [d+h:m:s] ";
  public static String chargeStr = " charge";
  public static String loginTimeStr = "login time [d+h:m:s] ";
  public static String totalChargeStr = "total charge";


  public BillOne(AccountInfo2 ai, boolean bSum) {
    this.ai = ai;
    this.bSummary = bSum;
    setLayout( new BorderLayout() );
    
    p = new JPanel();
    p.setBackground(Color.WHITE);
    p.setBorder(BorderFactory.createEmptyBorder());
    setSize(425+33,584);  // page + frame +scrollbar
    p.setLayout( new BillPaneLayout() );
    p.setBackground(Color.WHITE);
    
    String accountStr = ai.account();

    aProps = AProps.getInstance();
    logo = aProps.logo();
    logoLabel = new JLabel(logo);
    p.add(logoLabel);

    Font f = new Font("Serif",Font.PLAIN,12);
    fm = getFontMetrics(f);
    setFont(f);
    fontH = f.getSize() + 4;
    name = new JLabel(getAccountName());
    name.setFont(f);
    p.add(name);
    address1 = new JLabel(ai.address1());
    address1.setFont(f);
    p.add(address1);
    address2 = new JLabel(ai.address2());
    address2.setFont(f);
    p.add(address1);
    cityState = new JLabel(ai.city()+", "+ai.zip());
    cityState.setFont(f);
    p.add(cityState);
    country = new JLabel(ai.country());
    country.setFont(f);
    p.add(country);

    Font f2 = new Font("Arial",Font.PLAIN,8);
    accountNumber = new JLabel("ACCOUNT NUMBER");
    accountNumber.setFont(f2);
    p.add(accountNumber);
    billDate = new JLabel("BILLING DATE");
    billDate.setFont(f2);
    p.add(billDate);
    dueDate = new JLabel("DUE DATE");
    dueDate.setFont(f2);
    p.add(dueDate);
    totalCost = new JLabel("AMOUNT DUE");
    totalCost.setFont(f2);
    p.add(totalCost);

    Font f3 = new Font("Arial",Font.PLAIN,12);
    account = new JLabel(accountStr);
    account.setFont(f3);                  p.add(account);
    StringBuffer date = new StringBuffer();
    GregorianCalendar gc = new GregorianCalendar();
    SimpleDateFormat sdf = new SimpleDateFormat("MMM d,yyyy");
    sdf.format(gc.getTime(),date,new FieldPosition(0));
    bill = new JLabel( date.toString() );
    bill.setFont(f3);
    p.add(bill);
    gc.add(Calendar.DATE,31);
    date = new StringBuffer();
    sdf.format(gc.getTime(),date,new FieldPosition(0));
    due = new JLabel( date.toString() );
    due.setFont(f3);
    p.add(due);

    NumberFormat nf = aProps.getCurrencyFormat();
    cost = new JLabel( nf.format( 1.23 ) );
    cost.setFont(f3);
    p.add(cost);

    js1 = new JSeparator();
    p.add(js1);
    js2 = new JSeparator();
    p.add(js2);

    buildList();

    nf = aProps.getCurrencyFormat();
    cost.setText( nf.format( totalCharge ) );
    JScrollPane jsp = new JScrollPane(p);
    super.add(jsp);
  }

  private void buildList() {
      ArrayList list;
      int i, k, n, w;
      String onDate, tmpStr,str;
      String billMode;
      StringTokenizer st = new StringTokenizer(ai.oAndOs());
      st.nextToken(); // skip "operator "
      nOAndOs = st.countTokens();
      ReadLast reader = ReadLast.getInstance(ai);
      startDate = reader.startDate();
      endDate = reader.endDate();

      totalCharge = 0.0;
      if (startDate != null && endDate != null) {
          StringBuffer date = new StringBuffer();
          SimpleDateFormat sdf = new SimpleDateFormat("MMM d,yyyy");
          sdf.format(startDate, date, new FieldPosition(0));
          tmpStr = "Billing Period:  "+ date.toString();
          date = new StringBuffer();
          sdf.format(endDate, date, new FieldPosition(0));
          billingPeriod = tmpStr + " - "+ date.toString();
      }
      if (nOAndOs < 1)
          return;
      goesTimes = new double[nOAndOs];
      goesCharges = new double[nOAndOs];
      loginTimes = new double[nOAndOs];
      loginCharges = new double[nOAndOs];
      for (i = 0; i < nOAndOs; i++) {
          goesTimes[i] = 0.0;
          goesCharges[i] = 0.0;
          loginTimes[i] = 0.0;
          loginCharges[i] = 0.0;
      }

      billMode = aProps.getBillingMode();
      nLines = 2;
      if (aProps.isGoesBillingMode()) { 
          bGoes = true;
          aProps.setBillingMode("goes");
          goesLines = new ArrayList[nOAndOs];
          goesRates = new ArrayList[nOAndOs];
          for (i = 0; i < nOAndOs; i++) {
              str = st.nextToken();
              if (bSummary) {
                 w = fm.stringWidth(str);
                 if (w > nameWidth) {
                    nameWidth = w;
                    maxName = str;
                 }
              }
              list = reader.getLines(getAccountName(), str);
              goesCharges[i] = reader.total();
              goesTimes[i] = reader.getTotalTime(); // minutes
              totalCharge += goesCharges[i];
              goesLines[i] = list;
              goesRates[i] = reader.getRateList();

              n = list.size();
              nLines += n / 4;
              for (k = 0; k < n; k += 4) {
                 // Who and when logged in
                 tmpStr  = (String)list.get(k);
                 // date
                 onDate = (String)list.get(k+1);
                 str = tmpStr + ": "+ onDate;
                 list.set(k+1, str);
                 // how long  k+2
                 // cost  k+3
              }
          }
          nLines++;
      }
      if (aProps.isLoginBillingMode()) { 
          bLogin = true;
          aProps.setBillingMode("login");
          loginLines = new ArrayList[nOAndOs];
          loginRates = new ArrayList[nOAndOs];
          st = new StringTokenizer(ai.oAndOs());
          st.nextToken(); // skip "operator "
          nLines += 2;
          reader = ReadLast.getInstance(ai);
          reader.readRecords(2);
          for (i = 0; i < nOAndOs; i++) {
              str = st.nextToken();
              if (bSummary) {
                 w = fm.stringWidth(str);
                 if (w > nameWidth) {
                    nameWidth = w;
                    maxName = str;
                 }
              }
              list = reader.getLines(getAccountName(), str);
              loginCharges[i] = reader.total();
              loginTimes[i] = reader.getTotalTime();
              totalCharge += loginCharges[i];
              loginLines[i] = list;
              loginRates[i] = reader.getRateList();

              n = list.size();
              nLines += n / 4;
              for (k = 0; k < n; k += 4) {
                 tmpStr  = (String)list.get(k);
                 onDate = (String)list.get(k+1);
                 str = tmpStr + ": "+ onDate;
                 list.set(k+1, str);
              }
          }
          nLines++;
      }
      aProps.setBillingMode(billMode);
  }

  public double getTotalCharge() {
      return totalCharge;
  }

  public double getGoesTotalTime() {
      double sum = 0.0;

      for (int i = 0; i < nOAndOs; i++)
           sum += goesTimes[i];
      return sum;
  }

  public double getGoesTotalCharge() {
      double sum = 0.0;

      for (int i = 0; i < nOAndOs; i++)
           sum += goesCharges[i];
      return sum;
  }

  public double getLoginTotalTime() {
      double sum = 0.0;

      for (int i = 0; i < nOAndOs; i++)
           sum += loginTimes[i];
      return sum;
  }

  public double getLoginTotalCharge() {
      double sum = 0.0;

      for (int i = 0; i < nOAndOs; i++)
           sum += loginCharges[i];
      return sum;
  }

  public String getMinuteTime(double value) {

      if (value < 0.0)
          value = 0.0;
      int days = (int) (value / 60.0 / 24.0);
      int hr = (int)(value / 60.0) % 24;
      int min = (int)(value % 60);
      int sec = 0;
      StringBuilder sb = new StringBuilder("(");
      if (days>0)
              sb.append(days).append("+");
      if (hr < 10)
              sb.append("0");
      sb.append(hr).append(":");
      if (min < 10)
           sb.append("0");
      sb.append(min).append(":");
      if (sec < 10)
           sb.append("0");
      sb.append(sec).append(")");
      return( sb.toString() );
  }

  public String getBillingPeriod() {
      return billingPeriod;
  }

  public String getAccountName() {
      return ai.account();
  }

  public String getBillingName() {
      String str = ai.name();

      if (str != null && (str.length() > 0))
          return str;
      return ai.account();
  }

  public int getNameWidth() {
      return nameWidth;
  }

  public void paintSummary(Graphics g) {
      if (nOAndOs < 1)
         return;

      Rectangle r = g.getClipBounds();
      int i, x, x1, x2, x3, x4, x5;
      int y, y0, y2;
      int line;
      double charge;
      StringTokenizer st = new StringTokenizer(ai.oAndOs());
      NumberFormat nf = aProps.getNumberFormat();

      y = linesY0 + fontH;
      y0 = r.y - fontH;
      if (y0 < 0)
          y0 = 0;
      y2 = r.y + r.height + fontH;

      g.setColor(color1);
      g.drawString(billingPeriod, 5, y);
      y += fontH;
      x1 = nameWidth + 30;
      x2 = x1;
      x3 = x1 + 20;
      x4 = x3 + 20;
      if (bGoes) {
          g.drawString(acqTimeStr, x1, y);
          x1 += 5;
          x2 = x1 + fm.stringWidth(acqTimeStr) + 20;
          g.drawString(chargeStr, x2, y);
          x2 = x2 + fm.stringWidth(chargeStr);
          x3 = x2 + 30;
      }
      if (bLogin) {
          g.drawString(loginTimeStr, x3, y);
          x3 += 5;
          x4 = x3 + fm.stringWidth(acqTimeStr) + 20;
          g.drawString(chargeStr, x4, y);
          x4 = x4 + fm.stringWidth(chargeStr);
      }
      x5 = getWidth() - 10;
      x = x5 - fm.stringWidth(totalChargeStr);
      g.drawString(totalChargeStr, x, y);
      y = y + fontH + 4;
      line = 0;
      st.nextToken();
      for (i = 0; i < nOAndOs; i++) {
         if (y > y0) {
            if ((line % 2) == 0) {
                g.setColor(color0);
            }
            else {
                g.setColor(color1);
            }
            String data = st.nextToken();
            g.drawString(data, 5, y);
            charge = 0.0;
            if (bGoes) {
                g.drawString(getMinuteTime(goesTimes[i]), x1, y);
                data = nf.format(goesCharges[i]);
                x = x2 - fm.stringWidth(data);
                g.drawString(data, x, y);
                charge = goesCharges[i];
            }
            if (bLogin) {
                g.drawString(getMinuteTime(loginTimes[i]), x3, y);
                data = nf.format(loginCharges[i]);
                x = x4- fm.stringWidth(data);
                g.drawString(data, x, y);
                charge += loginCharges[i];
            }
            data = nf.format(charge);
            x = x5 - fm.stringWidth(data);
            g.drawString(data, x, y);
         }
         line++;
         y += fontH;
         if (y > y2)
            break;
      }
  }

  private void drawHeader(boolean bAcq, Graphics g, int y, int w) {
     int x1, x2, x3, x4;

     w -= 10;
     x1 = 5;
     x2 = x1 + (w * 50 / 100);
     x3 = x2 + (w * 20 / 100);
     if (bAcq)
          g.drawString("acquisition at", x1, y);
     else
          g.drawString("login at", x1, y);
     g.drawString("for [d+h:m:s]", x2 - 16, y);
     g.drawString("rate/hr", x3 - 10, y);
     x4 = w - fm.stringWidth("charge"); 
     g.drawString("charge", x4, y);
  }

  private int drawList(Graphics g, ArrayList list, ArrayList rates,
                    int y0, int y, int w, int y2) {
     int n = rates.size();
     int x1, x2, x3, x4;
     int i, line;
     String str;

     w -= 10;
     x1 = 5;
     x2 = x1 + (w * 50 / 100);
     x3 = x2 + (w * 20 / 100);

     for (line = 0; line < n; line++) {
         i = line * 4;
         if (y > y0) {
            if ((line % 2) == 0) {
                g.setColor(color0);
            }
            else {
                g.setColor(color1);
            }
            str = (String)list.get(i+1);
            g.drawString(str, x1, y);
            str = (String)list.get(i+2);
            g.drawString(str, x2, y);
            str = (String)rates.get(line);
            g.drawString(str, x3, y);
            str = (String)list.get(i+3);
            x4 = w - fm.stringWidth(str); 
            g.drawString(str, x4, y);
         }
         y += fontH;
         if (y > y2)
            break;
     }
     return y;
  }

  public void paint(Graphics g) {
      super.paint(g);
      ((Graphics2D)g).setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING,
            RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
      if (bSummary) {
          paintSummary(g);
          return;
      }

      Rectangle r = g.getClipBounds();
      int a, i, n, w;
      int h = r.height;
      int y0;
      int y = linesY0 + fontH;
      int y2 = r.y + r.height + fontH;
      int arrays = 0;

      g.setColor(color1);
      g.drawString(billingPeriod, 5, y);
      y += fontH;
      n = 0;
      y0 = r.y - fontH;
      if (y0 < 0)
          y0 = 0;
      w = getWidth();
      if (bGoes) {
          arrays = goesLines.length;
          drawHeader(true, g, y, w);
          y += fontH;
          for (a = 0; a < arrays; a++) {
             y = drawList(g, goesLines[a], goesRates[a], y0, y, w, y2);
             if (y > y2)
                 return;
          }
          y += fontH;
      }

      if (bLogin) {
          arrays = loginLines.length;
          g.setColor(color1);
          drawHeader(false, g, y, w);
          y += fontH;
          for (a = 0; a < arrays; a++) {
             y = drawList(g, loginLines[a], loginRates[a], y0, y, w, y2);
             if (y > y2)
                 return;
          }
      }
  }

  public void startPrint() {
      bFirstPage = true;
      pageNo = 0;
      prtLineIndex = 0;
      prtArrayIndex = 0;
      prtArrayNo = 0;
      if (bGoes)
          bPrintingAcq = true;
      else
          bPrintingAcq = false;
      if (bLogin)
          bPrintingLogin = true;
      else
          bPrintingLogin = false;
      bPrevFirstPage = true;
      savePrtEnv();
  }

  private void printFooter(Graphics2D g2d, int pageIndex) {
      Font f = new Font("Ariel",Font.ITALIC, 10);
      String str = " page "+(pageIndex+1);
      int w = prtFm.stringWidth(str) + 4;
      g2d.setFont(f);
      g2d.drawString(str, pageWidth - w, pageHeight- 6);
  }

  private void printLogo(Graphics2D g2d) {
     int xIW, yIH;
     int yOff = 0;

     logo = aProps.logo();
     xIW = logo.getIconWidth();
     yIH = logo.getIconHeight();
     if (yIH < 70) yOff = 70 - yIH;
     g2d.drawImage((Image)logo.getImage(), pageWidth-xIW,
                               0, xIW, yIH, Color.WHITE, null);
     prtY0 = yOff +yIH;
  }

  private void printAddress(Graphics2D g2d) {
     Font f = new Font("Ariel",Font.PLAIN, 12);
     g2d.setFont(f);
     g2d.setColor(Color.BLACK);
     if (ai.address2().isEmpty()) {
        prtY0 -= 36;
     }
     else {
        prtY0 -= 48;
     }
     g2d.drawString(getBillingName(), 10, prtY0);
     prtY0 += prtFontH;
     g2d.drawString(ai.address1(), 10, prtY0);
     prtY0 += prtFontH;
     if (!ai.address2().isEmpty()) {
         g2d.drawString(ai.address2(),10, prtY0);
         prtY0 += prtFontH;
     }
     g2d.drawString(ai.city()+", "+ai.zip(),10, prtY0);
     prtY0 += prtFontH;
  }

  private void printTotal(Graphics2D g2d) {
     DecimalFormat df = new DecimalFormat((String)aProps.get("numberFormat"));
     String y, str;
     double c, m, r;

     String country = (String)aProps.get("currency");
     Locale l = new Locale( country );
     String lang = l.getLanguage();
     l = new Locale(lang,country);
     NumberFormat nf = NumberFormat.getCurrencyInstance(l);
     Currency newC = Currency.getInstance(nf.getCurrency().toString());
     nf.setCurrency(newC);
     y = nf.format( totalCharge );
     g2d.drawString(y,(int)(pageWidth*3/4+10),prtY0+30);
  }

  private void printSum(Graphics2D g2d) {
     Font f = new Font("Ariel",Font.ITALIC, 8);
     g2d.setFont(f);
     g2d.drawRect(0,prtY0+5, pageWidth, 40);
     g2d.drawString("ACCOUNT NUMBER",8, prtY0+15);
     g2d.drawRect(2,prtY0+7,pageWidth/4 - 2,36);
     g2d.drawString("BILLING DATE",pageWidth/4 + 8, prtY0+15);
     g2d.drawRect(pageWidth/4+2,prtY0+7,pageWidth/4 - 4, 36);
     g2d.drawString("DUE DATE",pageWidth/2 + 8, prtY0+15);
     g2d.drawRect(pageWidth/2, prtY0+7, pageWidth/4 - 2, 36);
     g2d.drawString("AMOUNT DUE", pageWidth*3/4 + 8,prtY0+15);
     g2d.drawRect(pageWidth*3/4, prtY0+7, pageWidth/4 - 2,36);
     f = new Font("Ariel",Font.PLAIN, 12);
     g2d.setFont(f);
     g2d.drawString(ai.account(),10,prtY0+30);
     StringBuffer date = new StringBuffer();
     GregorianCalendar gc = new GregorianCalendar();
     SimpleDateFormat sdf = new SimpleDateFormat("MMM d,yyyy");
     sdf.format(gc.getTime(),date,new FieldPosition(0));
     g2d.drawString( date.toString(), pageWidth/4 + 10, prtY0 + 30);
     gc.add(Calendar.DATE,31);
     date = new StringBuffer();
     sdf.format(gc.getTime(),date,new FieldPosition(0));
     g2d.drawString( date.toString(), pageWidth/2 + 10, prtY0 + 30);
     printTotal(g2d);
  }

  private void printSummary(Graphics2D g) {
      int i, x, x1, x2, x3, x4, x5;
      int y, y0, y2;
      int lines;
      double charge;
      StringTokenizer st = new StringTokenizer(ai.oAndOs());
      NumberFormat nf = aProps.getNumberFormat();

      y = prtY0;
      x1 = prtFm.stringWidth(maxName) + 20;
      x2 = x1;
      x3 = x1 + 20;
      x4 = x3 + 20;
      if (bPrintingAcq) {
          if (!bPrintingLogin)
             x1 += 20;
          g.drawString(acqTimeStr, x1, y);
          x1 += 5;
          x2 = x1 + prtFm.stringWidth(acqTimeStr) + 20;
          g.drawString(chargeStr, x2, y);
          x2 = x2 + prtFm.stringWidth(chargeStr);
          x3 = x2 + 30;
      }
      if (bPrintingLogin) {
          g.drawString(loginTimeStr, x3, y);
          x4 = x3 + prtFm.stringWidth(acqTimeStr) + 20;
          x3 += 5;
          g.drawString(chargeStr, x4, y);
          x4 = x4 + prtFm.stringWidth(chargeStr);
      }
      x5 = pageWidth - 10;
      x = x5 - prtFm.stringWidth(totalChargeStr);
      g.drawString(totalChargeStr, x, y);
      y = y + prtFontH;
      lines = 0;
      st.nextToken();
      i = 0;
      while (i < prtArrayIndex) {
         st.nextToken();
         i++;
      }
      for (i = prtArrayIndex; i < nOAndOs; i++) {
          String data = st.nextToken();
          g.drawString(data, 10, y);
          charge = 0.0;
          if (bGoes) {
              g.drawString(getMinuteTime(goesTimes[i]), x1, y);
              data = nf.format(goesCharges[i]);
              x = x2 - prtFm.stringWidth(data);
              g.drawString(data, x, y);
              charge = goesCharges[i];
          }
          if (bLogin) {
              g.drawString(getMinuteTime(loginTimes[i]), x3, y);
              data = nf.format(loginCharges[i]);
              x = x4- prtFm.stringWidth(data);
              g.drawString(data, x, y);
              charge += loginCharges[i];
          }
          data = nf.format(charge);
          x = x5 - prtFm.stringWidth(data);
          g.drawString(data, x, y);
          lines++;
          y += prtFontH;
          if (lines >= MAX_LINES)
             break;
      }
      prtArrayIndex = i;
      prtLineIndex = 0;
      if (i >= nOAndOs) {
          bPrintingLogin = false;
          bPrintingAcq = false;
      }
  }

  private void printList(Graphics2D g, ArrayList list, ArrayList rates) {
      int x1, x2, x3, x4;
      int i, k, n, y, w;
      String str;

      x1 = 10;
      x2 = (int)(pageWidth * 0.5);
      x3 = (int)(pageWidth * 0.75);
      x4 = pageWidth - 10;
      y = prtY0 + prtLineIndex * prtFontH;

      n = rates.size();
      if (n <= 0)
          prtLineIndex++;
      for (i = prtArrayIndex; i < n; i++) {
          k = i * 4;
          str = (String)list.get(k+1);
          g.drawString(str, x1, y);
          str = (String)list.get(k+2);
          g.drawString(str, x2, y);
          str = (String)rates.get(i);
          g.drawString(str, x3, y);
          str = (String)list.get(k+3);
          w = prtFm.stringWidth(str);
          g.drawString(str, x4 - w, y);
          y += prtFontH; 
          prtLineIndex++;
          if (prtLineIndex >= MAX_LINES)
              break;
      }
      prtArrayIndex = i + 1;
      if (prtLineIndex >= MAX_LINES)
          prtLineIndex = 0;
      if (prtArrayIndex >= n)
          prtArrayIndex = 0;
  }

  private void savePrtEnv() {
      bPrevAcq = bPrintingAcq;
      bPrevLogin = bPrintingLogin;
      prevLineIndex = prtLineIndex;
      prevArrayIndex = prtArrayIndex;
      prevArrayNo = prtArrayNo;
      bPrevFirstPage = bFirstPage;
  }

  private void resetPrtEnv() {
      bPrintingAcq = bPrevAcq;
      bPrintingLogin = bPrevLogin;
      prtLineIndex = prevLineIndex;
      prtArrayIndex = prevArrayIndex;
      prtArrayNo = prevArrayNo;
      bFirstPage = bPrevFirstPage;
  }

  public int print(Graphics g, PageFormat pf, int pageIndex) {
      Graphics2D g2d = (Graphics2D)g;
      g2d.translate(pf.getImageableX(), pf.getImageableY());
      pageWidth = (int)pf.getImageableWidth();
      pageHeight = (int)pf.getImageableHeight();

      if (bSummary &&  (pageIndex > 20))
           return Printable.NO_SUCH_PAGE;

      if (pageIndex == 0)
          startPrint();
      else {
         if (pageIndex == pageNo)
             resetPrtEnv();
         else
             savePrtEnv();
      }
      pageNo = pageIndex;
      if (!bPrintingAcq && !bPrintingLogin)
         return Printable.NO_SUCH_PAGE;
      prtY0 = 80;
      printLogo(g2d);
      printAddress(g2d);
      if (bFirstPage)
         printSum(g2d);
      else {
         g2d.drawLine(0, prtY0 +35, pageWidth, prtY0 +35);
         g2d.drawLine(0, prtY0 +37, pageWidth, prtY0 +37);
      }

      bFirstPage = false;

      int a, i, k, arrays;
      String data;
      if (prtFont == null) {
         prtFont = new Font("Ariel",Font.PLAIN, 8);
         prtFm = getFontMetrics(prtFont);
      }

      g2d.setFont(prtFont);
      g2d.setColor(Color.BLACK);

      prtY0 = prtY0 + 45 + prtFontH;
      g2d.drawString(billingPeriod, 10, prtY0);
      prtY0 += prtFontH;
      k = prtY0 + prtFontH;

      i = (pageHeight - k - 5) / prtFontH;
      MAX_LINES = i / 2;
      k -= 10;
      g2d.setColor(prtBg);
      for (i = 0; i < MAX_LINES; i++) {
         g2d.fillRect(0, k, pageWidth, prtFontH);
         k += 24;
      }
      MAX_LINES = MAX_LINES * 2;

      g2d.setColor(Color.BLACK);
      if (bSummary) {
         if (nOAndOs < 1) {
            bPrintingAcq = false;
            bPrintingLogin = false;
         }
         else
            printSummary(g2d);
         printFooter(g2d, pageIndex);
         return Printable.PAGE_EXISTS;
      }

      NumberFormat nf = aProps.getNumberFormat();

      if (bPrintingAcq)
          g2d.drawString("acquisition at", 30, prtY0);
      else
          g2d.drawString("login at", 30, prtY0);
      g2d.drawString("for [dd+hh:mm:ss]", (int)(0.5*pageWidth), prtY0);
      g2d.drawString("rate/hr", (int)(0.75*pageWidth),prtY0);
      a = prtFm.stringWidth("charge"); 
      g2d.drawString("charge",  pageWidth - a - 10, prtY0);
      prtY0 += prtFontH;

      if (bPrintingAcq) {
           arrays = goesLines.length;
           for (a = prtArrayNo; a < arrays; a++) {
               if (goesRates[a].size() > 0) {
                  printList(g2d, goesLines[a], goesRates[a]);
                  if (prtLineIndex <= 0) {
                     if (prtArrayIndex <= 0)
                        a++;
                      break;
                  }
               }
               prtArrayIndex = 0;
           }
           prtArrayNo = a;
           if (prtArrayNo < arrays) {
               printFooter(g2d, pageIndex);
               return Printable.PAGE_EXISTS;
           }
           prtArrayNo = 0;
           prtArrayIndex = 0;
           bPrintingAcq = false;
           if (!bPrintingLogin) {
               printFooter(g2d, pageIndex);
               return Printable.PAGE_EXISTS;
           }
           prtLineIndex += 2;
           if (prtLineIndex >= MAX_LINES)
               return Printable.PAGE_EXISTS;
           a = prtY0 + prtLineIndex * prtFontH;
           g2d.drawString("login at", 30, a);
           prtLineIndex ++;
      }
      if (!bPrintingLogin)
          return Printable.NO_SUCH_PAGE;

      arrays = loginLines.length;
      for (a = prtArrayNo; a < arrays; a++) {
          if (loginRates[a].size() > 0) {
             printList(g2d, loginLines[a], loginRates[a]);
             if (prtLineIndex <= 0) {
                if (prtArrayIndex <= 0)
                   a++;
                 break;
             }
          }
          prtArrayIndex = 0;
      }
      prtArrayNo = a;
      if (prtArrayNo < arrays) {
          printFooter(g2d, pageIndex);
          return Printable.PAGE_EXISTS;
      }
      bPrintingLogin = false;

      printFooter(g2d, pageIndex);
      return Printable.PAGE_EXISTS;
  }

  private void writeList(PrintWriter fd, String type, ArrayList list, ArrayList rates) {
      int i, k, n, x;
      String str;

      n = rates.size();
      for (i = 0; i < n; i++) {
          k = i * 4;
          // account name
          str = (String)list.get(k);
          fd.print("\"");
          fd.print(str);
          fd.print("\",");
          fd.print(type);
          str = (String)list.get(k+1);
          x = str.indexOf(':');
          if (x > 0)
              str = str.substring(x+1).trim();
          // start time 
          fd.print(",\"");
          fd.print(str);
          fd.print("\",");
          // duration
          str = (String)list.get(k+2);
          fd.print("\"");
          fd.print(str);
          fd.print("\",");
          // rate
          str = (String)rates.get(i);
          fd.print("\"");
          fd.print(str);
          fd.print("\",");
          // charge
          str = (String)list.get(k+3);
          fd.print("\"");
          fd.print(str);
          fd.println("\"");
       }
  }

  public void writeSummaryToCsv(PrintWriter fd, boolean bFirst) {
      StringTokenizer st = new StringTokenizer(ai.oAndOs());
      NumberFormat nf = aProps.getNumberFormat();
      double charge;

      if (bFirst) {
         fd.print("Account,");
         if (bGoes) {
            fd.print("\"");
            fd.print("Acquisition[d+h:m:s]");
            fd.print("\",");
            fd.print("Charge,");
         }
         if (bLogin) {
            fd.print("\"");
            fd.print("Login[d+h:m:s]");
            fd.print("\",");
            fd.print("Charge,");
         }
         fd.print("\"");
         fd.print("Total Charge");
         fd.println("\"");
      }
      st.nextToken();
      for (int i = 0; i < nOAndOs; i++) {
         String str = st.nextToken();
         fd.print("\"");
         fd.print(str);
         fd.print("\",");
         charge = 0.0;
         if (bGoes) {
            str = getMinuteTime(goesTimes[i]);
            fd.print("\"");
            fd.print(str);
            fd.print("\",");
            str = nf.format(goesCharges[i]);
            fd.print("\"");
            fd.print(str);
            fd.print("\",");
            charge = goesCharges[i];
          }
          if (bLogin) {
            str = getMinuteTime(loginTimes[i]);
            fd.print("\"");
            fd.print(str);
            fd.print("\",");
            str = nf.format(loginCharges[i]);
            fd.print("\"");
            fd.print(str);
            fd.print("\",");
            charge += loginCharges[i];
          }
          str = nf.format(charge);
          fd.print("\"");
          fd.print(str);
          fd.println("\"");
      }
  }

  public void writeToCsv(PrintWriter fout, boolean bFirst) {
      int i, num;
      String str;

      if (bSummary) {
         writeSummaryToCsv(fout, bFirst);
         return;
      }

      if (bFirst) {
         fout.print("Account,Type,Start,");
         fout.print("\"");
         fout.print("Duration[d+h:m:s]");
         fout.print("\",");
         fout.println("Rate,Charge");
      }
      if (bGoes) {
          num = goesLines.length;
          for (i = 0; i < num; i++) {
             if (goesRates[i].size() > 0)
                 writeList(fout, "acquisition", goesLines[i], goesRates[i]);
          }
      }
      if (bLogin) {
          num = loginLines.length;
          for (i = 0; i < num; i++) {
             if (loginRates[i].size() > 0)
                 writeList(fout, "login", loginLines[i], loginRates[i]);
          }
      }
  }

  private class BillPaneLayout implements LayoutManager {
    int deltaY = 15;

    public void addLayoutComponent(String str, Component c) {
    }

    public void layoutContainer(Container parent) {
      int x = parent.getWidth();
      int w, dx;
      int xIW = logo.getIconWidth();
      int yIH = logo.getIconHeight();
      // If no logo or logo too small, we end up cutting off name and address
      if(yIH < 40)
          yIH = 40;
      logoLabel.setBounds(x-xIW-10,10,xIW,yIH);
      int m=0;
      country.setBounds(10,yIH+10-m*deltaY,200,14);
      m++;
      cityState.setBounds(10,yIH+10-m*deltaY,200,14);
      if (address2.getText()!=null && address2.getText().length()>0) {
        m++;
        address2.setBounds(10, yIH + 10 - m * deltaY, 200, 14);
      }
      m++;
      address1.setBounds(10,yIH+10-m*deltaY,200,14);
      m++;
      name.setBounds(10,yIH+10-m*deltaY,300,14);

      js1.setBounds(5,yIH+24,x-10,2);
      accountNumber.setBounds(10,yIH+26,x*20/100,deltaY);
      billDate.setBounds(x*25/100,yIH+26,x*20/100,deltaY);
      dueDate.setBounds(x*50/100,yIH+26,x*20/100,deltaY);
      totalCost.setBounds(x*75/100,yIH+26,x*20/100,deltaY);
      account.setBounds(10,yIH+36,x*20/100,deltaY);
      bill.setBounds(x*25/100,yIH+36,x*20/100,deltaY);
      due.setBounds(x*50/100,yIH+36,x*20/100,deltaY);
      cost.setBounds(x*75/100,yIH+36,x*20/100,deltaY);
      js2.setBounds(5,yIH+50,x-10,2);
      linesY0 = yIH + 50;
    }

    public Dimension minimumLayoutSize( Container parent ) {
      return ( new Dimension(0,0));
    }

    public Dimension preferredLayoutSize( Container parent ) {
      int a, n;

      n = logo.getIconHeight() + 4;
      if (bSummary)
          a = n + (nOAndOs + 4) * fontH + 50;
      else
          a = n + nLines * fontH + 50;
      return ( new Dimension(425,a));
    }

    public void removeLayoutComponent( Component c  ) {
    }
  }
}
