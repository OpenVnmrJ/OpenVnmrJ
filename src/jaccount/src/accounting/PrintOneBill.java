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
import java.awt.print.*;
import java.text.*;
import java.util.*;
import javax.swing.*;

class PrintOneBill extends JComponent implements Printable {

  private int         MAX_LINES=40;
  private int         ySofar = 0;
  private double      pageWidth;
  private double      pageHeight;
  private double      total;
  private AccountInfo2 ai;
  private AProps      aProps;
  private ArrayList   lines;
  private ImageIcon   logo;
  private String      xGlobal = null, xGlobal2 = null;
  private int         k=0, nOAndOs=0;
  private String[]    _summaryLine = new String[3];
  public  String[]    summaryLine() { return _summaryLine; }
  public  StringTokenizer st = null;

  public PrintOneBill(AccountInfo2 ai) {
    this.ai = ai;
    aProps = AProps.getInstance();
    lines = null;
   
  }
  
  public int print(Graphics g, PageFormat pf, int pageIndex) {
      if (lines == null) {
        ArrayList tmpList;
        String str;
        st = new StringTokenizer(ai.oAndOs());
        st.nextToken(); // skip "operator "
        nOAndOs = st.countTokens();
        total = 0.0;
        if (nOAndOs != 0) {
          lines = new ArrayList();
          for (int i=0; i<nOAndOs; i++) {
            str = st.nextToken();
            tmpList = ReadLast.getInstance(ai).getLines(str);
            total += ReadLast.getInstance(ai).total();
//            System.out.println("Adding "+tmpList.size()+" entries to lines");
            lines.addAll(tmpList);
          }
        }
      }
      
    pageWidth = pf.getImageableWidth();
    pageHeight = pf.getImageableHeight();

    Graphics2D g2d = (Graphics2D)g;
    g2d.translate(pf.getImageableX(), pf.getImageableY());
//------------------  header ------------------------------------------------
   
    
    placeLogo(g2d); // inits ySoFar
    placeAddress(g2d);
    
    if (pageIndex == 0) {
      _summaryLine[0] = new String(ai.account());
      placeSummary(g2d);
    }
    else {
      g2d.drawLine(0,ySofar+35,(int)pageWidth,ySofar+35);
      g2d.drawLine(0,ySofar+37,(int)pageWidth,ySofar+37);
    }
    
    k = MAX_LINES * pageIndex * 4;
    if ( (lines==null) && (pageIndex==0) )
            return(Printable.PAGE_EXISTS);
    else if ( (lines==null) || (k >  lines.size()) )  {
      return Printable.NO_SUCH_PAGE;
    }

    ySofar += 75;
    g2d.setColor(Color.LIGHT_GRAY);
    for (int i=0; i<MAX_LINES/2; i++) {
      g2d.fillRect(0,ySofar-10,(int)pageWidth,12);
      ySofar += 24;
    }

    Font f = new Font("Ariel",Font.PLAIN, 8);
    g2d.setFont(f);
    g2d.setColor(Color.BLACK);

    ySofar = ySofar - MAX_LINES*12 - 12;

    g2d.drawString("logged in at",       10, ySofar);
    g2d.drawString("for [dd+hh:mm:ss]",  (int)(0.5*pageWidth), ySofar);
    g2d.drawString("rate/hr",            (int)(0.75*pageWidth),ySofar);
    g2d.drawString("charge",             (int)(0.9*pageWidth), ySofar);
    ySofar += 12;
//----------------------------------------------------------------------------
    String y,onDate;
    Double tmpD;
    int j=0;
    NumberFormat nf = aProps.getNumberFormat();
//    System.out.println("index starts next at "+k);
    while ( (j<MAX_LINES) && (k < lines.size()) ) {
      xGlobal = (String)lines.get(k); k++;    //name
      onDate = (String)lines.get(k);      k++;   // date one
      g2d.drawString(xGlobal+": "+onDate,10, ySofar);
      y = (String)lines.get(k);     k++;    // how long
      g2d.drawString(y,(int)(pageWidth*0.5),ySofar);
      tmpD = new Double((ai.getRate(onDate)).loginhr);
//      y = nf.format(tmpD);
//      g2d.drawString(y,(int)(pageWidth*0.75),ySofar);
      y = (String)lines.get(k);     k++;
      g2d.drawString(y,(int)(pageWidth*0.9),ySofar);
//      System.out.println(xGlobal+" x "+y + " " + k + "  " +lines.size()+ " "+nOAndOs);
      ySofar += 12;
      j++;  // one more line;
    }
//    System.out.println("End of Page: k="+k);
    placeFooter(g2d, pageIndex);
    return Printable.PAGE_EXISTS;
  }

  private String lengthen(String s, int len) {
    StringBuffer st = new StringBuffer();
    int l = s.length();
    if ( l < len) {
      for (int i=0; i<len-l; i++) {
        st.append("  ");
      }
      st.append(s);
    }
    return(st.toString());
  }

  private void placeLogo(Graphics2D g2d) {
    int xIW, yIH;
    int yOff = 0;
//    System.out.println("logo is '"+aProps.get("logo"));
    logo = aProps.logo();
    xIW = logo.getIconWidth();
    yIH = logo.getIconHeight();
    if (yIH < 70) yOff = 70 - yIH;
    g2d.drawImage((Image)logo.getImage(), (int)(pageWidth-xIW),
                               0,xIW,yIH,Color.WHITE, null);
    ySofar = yOff +yIH;
  }

  private void placeAddress(Graphics2D g2d) {
    Font f = new Font("Serif",Font.PLAIN,12);
    setFont(f);
    g2d.setColor(Color.BLACK);
    if (ai.address2().compareTo("") == 0) {
      ySofar -= 30;
    }
    else {
      ySofar -= 40;
    }
    g2d.drawString(ai.name(),10,ySofar);
    ySofar += 10;
    g2d.drawString(ai.address1(),10,ySofar);
    ySofar += 10;
    if (ai.address2().compareTo("") != 0) {
      g2d.drawString(ai.address2(),10,ySofar);
      ySofar += 10;
    }
    g2d.drawString(ai.city()+", "+ai.zip(),10,ySofar);
    ySofar += 10;
  }

  private void placeSummary(Graphics2D g2d) {
    Font f = new Font("Ariel",Font.ITALIC, 8);
    g2d.setFont(f);
    g2d.drawRect(0,ySofar+5,(int)pageWidth,40);
    g2d.drawString("ACCOUNT NUMBER",8,ySofar+15);
    g2d.drawRect(2,ySofar+7,(int)(pageWidth/4-2),36);
    g2d.drawString("BILLING DATE",(int)(pageWidth/4+8),ySofar+15);
    g2d.drawRect((int)(pageWidth/4+2),ySofar+7,(int)(pageWidth/4-4),36);
    g2d.drawString("DUE DATE",(int)(pageWidth/2+8),ySofar+15);
    g2d.drawRect((int)(pageWidth/2),ySofar+7,(int)(pageWidth/4-2),36);
    g2d.drawString("AMOUNT DUE", (int)(pageWidth*3/4+8),ySofar+15);
    g2d.drawRect((int)(pageWidth*3/4), ySofar+7, (int)(pageWidth/4-2),36);
    f = new Font("Ariel",Font.PLAIN, 12);
    g2d.setFont(f);
    g2d.drawString(ai.account(),10,ySofar+30);
    StringBuffer date = new StringBuffer();
    GregorianCalendar gc = new GregorianCalendar();
    SimpleDateFormat sdf = new SimpleDateFormat("MMM d,yyyy");
    sdf.format(gc.getTime(),date,new FieldPosition(0));
    g2d.drawString( date.toString(), (int)(pageWidth/4+10),ySofar+30);
    _summaryLine[1] = date.toString();
    gc.add(Calendar.DATE,31);
    date = new StringBuffer();
    sdf.format(gc.getTime(),date,new FieldPosition(0));
    g2d.drawString( date.toString(), (int)(pageWidth/2+10),ySofar+30);
    placeTotal(g2d);
  }

  private void placeTotal(Graphics2D g2d) {
    DecimalFormat df = new DecimalFormat((String)aProps.get("numberFormat"));
//    MakeWtmpx wtmpx = MakeWtmpx.getInstance();
    String y, str;
    double c, m, r;

    String country = (String)aProps.get("currency");
    Locale l = new Locale( country );
    String lang = l.getLanguage();
    l = new Locale(lang,country);
    NumberFormat nf = NumberFormat.getCurrencyInstance(l);
//    System.out.println("df2 has '"+nf.getCurrency()+"'");
    Currency newC = Currency.getInstance(nf.getCurrency().toString());
    nf.setCurrency(newC);
    y = nf.format( total );
    _summaryLine[2] = y;
    g2d.drawString(y,(int)(pageWidth*3/4+10),ySofar+30);
  }
  
  private void placeFooter(Graphics2D g2d, int pageIndex) {
      Font f = new Font("Ariel",Font.ITALIC, 12);
      g2d.setFont(f);
      g2d.drawString("page "+(pageIndex+1),(int)(pageWidth*0.9),(int)(pageHeight-20));
      String fln = new String(AProps.getInstance().getRootDir()+"/adm/accounting/varianbw.GIF");
      ImageIcon inspire = new ImageIcon( fln );
      int xIW = inspire.getIconWidth();
      int yIH = inspire.getIconHeight();
//      System.out.println("placeFooter: fln="+fln);
//      System.out.println("inspire="+inspire);
      if (inspire != null) {
         g2d.drawImage((Image)inspire.getImage(), 0,(int)(pageHeight-32),
                 (int)(xIW*12.0/yIH),12,Color.WHITE, null);
      }
  }
}
