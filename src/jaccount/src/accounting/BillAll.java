/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 *
 */

package accounting;

import java.awt.*;
import java.awt.event.*;
import java.awt.RenderingHints;
import java.io.*;
import java.text.*;
import java.awt.print.*;
import java.net.*;
import java.util.*;
import javax.swing.*;

public class BillAll extends JPanel implements Printable {
    
    private ArrayList summaries;
    private AProps aProps;
    private ImageIcon logo;
    private JLabel    logoLabel;
    private JLabel[]  headerLabel = new JLabel[3];
    private boolean   bSummary = false;
    private boolean   bGoes = false;
    private boolean   bLogin = false;
    private boolean   bFirstPage = true;
    private String   billingPeriod = null;
    private String   maxName = " ";
    private Font      font;
    private Font      prtFont;
    private FontMetrics fm;
    private FontMetrics prtFm;
    private int fontH;
    private int linesY0 = 0;
    private int xOffset = 10;
    private int nameWidth = 0;
    private int pageWidth = 0;
    private int pageHeight = 0;
    private int pageNo = 0;
    private int prtLineIndex = 0;
    private int prevLineIndex = 0;
    private int MAX_LINES = 40;
    private int prtBillIndex = 0;
    private int prtY0 = 0;
    private int prtFontH = 12;
    private BillOne prtBillOne = null;
    private Color prtBg = new Color(230, 230, 230);
    private Vector<BillOne> billList = null;


    public BillAll(boolean bSum) {
        this.bSummary = bSum;

        init();
        setBackground(Color.WHITE);
        setSize(400,600);
    }

    private void init() {
        aProps = AProps.getInstance();
        if (aProps.isGoesBillingMode())
            bGoes = true; 
        if (aProps.isLoginBillingMode())
            bLogin = true; 
        font = new Font("Ariel",Font.PLAIN,12);
        fm = getFontMetrics(font); 
        fontH = font.getSize() + 4;
        setFont(font);
        if (!bSummary) {
            setLayout( new SimpleVLayout(0, 20));
            return;
        }

        setLayout( new SummaryAllLayout());
        logo = AProps.getInstance().logo();
        logoLabel = new JLabel(logo);
        add(logoLabel);
        try {
           headerLabel[0] = new JLabel("Summary for Spectrometer: "+
                java.net.InetAddress.getLocalHost().getHostName() );
        } catch (Exception e) {
            e.printStackTrace();
        }
        Font f = new Font("Ariel",Font.BOLD, 16);
        headerLabel[0].setFont(f);
        add(headerLabel[0]);
        headerLabel[1] = new JLabel(new Date().toLocaleString());
        headerLabel[1].setFont(font);
        add(headerLabel[1]);

    }
    
    public void addBill(BillOne bill) {
        if (bill == null)
           return;
        if (billList == null)
           billList = new Vector<BillOne>();
        if (bill.getTotalCharge() <= 0.0)
           return;
        billList.add(bill);
        if (!bSummary)
           add(bill);
        if (billingPeriod == null)
           billingPeriod = bill.getBillingPeriod();
        String name = bill.getAccountName();
        if (name != null) {
           int w = fm.stringWidth(name);
           if (w > nameWidth) {
               nameWidth = w;
               maxName = name;
           }
        } 
    }

    public void paint(Graphics g) {
        super.paint(g);
        ((Graphics2D)g).setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING,
            RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
        if (!bSummary)
            return;
        Rectangle r = g.getClipBounds();
        int i, x, x0, x1, x2, x3, x4, x5;
        int line, y, y0, y2, w;
        NumberFormat nf = aProps.getNumberFormat();

        x0 = 10;
        y = linesY0 + fontH;
        y0 = r.y - fontH;
        if (y0 < 0)
            y0 = 0;
        y2 = r.y + r.height + fontH;

        g.setColor(Color.black);
        if ( billingPeriod == null)
        {
           g.drawString("Please select Invoice dates", x0, y);
           return;
        }
        g.drawString(billingPeriod, x0, y);
        y += fontH;
        x1 = x0 + nameWidth + 30;
        x2 = x1;
        x3 = x1 + 20;
        x4 = x3 + 20;
        if (bGoes) {
           g.drawString(BillOne.acqTimeStr, x1, y);
           x1 += 5;
           x2 = x1 + fm.stringWidth(BillOne.acqTimeStr) + 20;
           g.drawString(BillOne.chargeStr, x2, y);
           x2 = x2 + fm.stringWidth(BillOne.chargeStr);
           x3 = x2 + 30;
        }
        if (bLogin) {
            g.drawString(BillOne.loginTimeStr, x3, y);
            x3 += 5;
            x4 = x3 + fm.stringWidth(BillOne.acqTimeStr) + 20;
            g.drawString(BillOne.chargeStr, x4, y);
            x4 = x4 + fm.stringWidth(BillOne.chargeStr);
        }
        x5 = getWidth() - 10;
        x = x5 - fm.stringWidth(BillOne.totalChargeStr);
        g.drawString(BillOne.totalChargeStr, x, y);
        y = y + fontH + 4;
        line = 0;
        x = 10;
        if (billList == null)
            return;
        for (i = 0; i < billList.size(); i++) {
            if (y > y0) {
                BillOne bill = billList.elementAt(i);
                if (bill != null) {
                    if ((line % 2) == 0)
                        g.setColor(Color.blue);
                    else
                        g.setColor(Color.black);

                    String str = bill.getAccountName();
                    if (str != null)
                        g.drawString(str, x0, y);
                    if (bGoes) {
                        str = bill.getMinuteTime(bill.getGoesTotalTime());
                        g.drawString(str, x1, y);
                        str = nf.format(bill.getGoesTotalCharge());
                        x = x2 - fm.stringWidth(str);
                        g.drawString(str, x, y);
                    }
                    if (bLogin) {
                        str = bill.getMinuteTime(bill.getLoginTotalTime());
                        g.drawString(str, x3, y);
                        str = nf.format(bill.getLoginTotalCharge());
                        x = x4 - fm.stringWidth(str);
                        g.drawString(str, x, y);
                    }
                    str = nf.format(bill.getTotalCharge());
                    x = x5 - fm.stringWidth(str);
                    g.drawString(str, x, y);
                }
            }
            line++;
            y += fontH;
            if (y > y2)
               break;
        }
    }

    public void startPrint() {
       bFirstPage = true;
       pageNo = 0;
       prtLineIndex = 0;
       prtBillIndex = 0;
       prevLineIndex = prtLineIndex;
       prtBillOne = billList.elementAt(0);
       prtBillOne.startPrint();
    }

    private void printLogo(Graphics2D g) {
        int xIW, yIH;
        int yOff = 0;

        logo = AProps.getInstance().logo();
        xIW = logo.getIconWidth();
        yIH = logo.getIconHeight();
        if (yIH < 70) yOff = 70 - yIH;
        g.drawImage((Image)logo.getImage(), (int)(pageWidth-xIW),
                              0,xIW,yIH,Color.WHITE, null);
        prtY0 = yOff +yIH;
    }

    private void savePrtEnv() {
       prevLineIndex = prtLineIndex;
    }

    private void resetPrtEnv() {
       prtLineIndex = prevLineIndex;
    }

    public int print(Graphics g, PageFormat pf, int pageIndex) {
      if (billList == null)
          return Printable.NO_SUCH_PAGE;
      if (pageIndex > 120)
          return Printable.NO_SUCH_PAGE;

      int i, k, x, x0, x1, x2, x3, x4, x5;
      int lines, y, y0, y2, w;

      if (pageIndex == 0) {
          startPrint();
      }
      else {
         if (pageIndex == pageNo)
             resetPrtEnv();
         else
             savePrtEnv();
      }
      pageNo = pageIndex;

      if (!bSummary) {
          if (prtBillOne == null) 
              return Printable.NO_SUCH_PAGE;
          k = prtBillOne.print(g, pf, pageIndex); 
          if (k == Printable.NO_SUCH_PAGE) {
              prtBillIndex++;
              if (prtBillIndex >= billList.size())
                  prtBillOne = null;
              else
                  prtBillOne = billList.elementAt(prtBillIndex);
              if (prtBillOne == null)
                  return Printable.NO_SUCH_PAGE;
              prtBillOne.startPrint();
              prtBillOne.print(g, pf, pageIndex); 
          }
          return Printable.PAGE_EXISTS;
      }

      if (prtLineIndex >= billList.size())
          return Printable.NO_SUCH_PAGE;

      NumberFormat nf = aProps.getNumberFormat();

      Graphics2D g2d = (Graphics2D)g;
      pageWidth = (int)pf.getImageableWidth();
      pageHeight = (int)pf.getImageableHeight();
      g2d.translate(pf.getImageableX(), pf.getImageableY());
      printLogo(g2d);
      Font f = new Font("Ariel",Font.BOLD,12);
      g2d.setFont(f);
      g2d.setColor(Color.BLACK);
      prtFontH = 12;
      g2d.drawString(headerLabel[0].getText(), 10, prtY0 - prtFontH * 2);
      f = new Font("Ariel",Font.PLAIN, 10);
      g2d.setFont(f);
      g2d.drawString(headerLabel[1].getText(), 10, prtY0 - prtFontH);

      if (prtFont == null) {
         prtFont = new Font("Ariel",Font.PLAIN, 8);
         prtFm = getFontMetrics(prtFont);
      }
      g2d.setFont(prtFont);

      prtY0 += prtFontH;
      g2d.drawString(billingPeriod, 10, prtY0);
      prtY0 += prtFontH;

      k = prtY0 + prtFontH;
      i = (pageHeight - k - 5) / prtFontH;
      MAX_LINES = i / 2;
      k -= 10;
      g2d.setColor(prtBg);
      for (i = 0; i < MAX_LINES; i++) {
         g2d.fillRect(0, k, pageWidth, prtFontH);
         k += prtFontH * 2;
      }
      MAX_LINES = MAX_LINES * 2;
      g2d.setColor(Color.BLACK);
      x0 = 10;
      x1 = prtFm.stringWidth(maxName) + 20;
      x2 = x1;
      x3 = x1 + 20;
      x4 = x3 + 20;
      y = prtY0;
      if (bGoes) {
           if (!bLogin)
              x1 += 20;
           g2d.drawString(BillOne.acqTimeStr, x1, y);
           x1 += 5;
           x2 = x1 + prtFm.stringWidth(BillOne.acqTimeStr) + 20;
           g2d.drawString(BillOne.chargeStr, x2, y);
           x2 = x2 + prtFm.stringWidth(BillOne.chargeStr);
           x3 = x2 + 30;
      }
      if (bLogin) {
           g2d.drawString(BillOne.loginTimeStr, x3, y);
           x4 = x3 + prtFm.stringWidth(BillOne.acqTimeStr) + 20;
           x3 += 5;
           g2d.drawString(BillOne.chargeStr, x4, y);
           x4 = x4 + prtFm.stringWidth(BillOne.chargeStr);
      }
      x5 = pageWidth - 10;
      x = x5 - prtFm.stringWidth(BillOne.totalChargeStr);
      g2d.drawString(BillOne.totalChargeStr, x, y);
      y = y + prtFontH;

      lines = 0;
      for (i = prtLineIndex; i < billList.size(); i++) {
          BillOne bill = billList.elementAt(i);
          if (bill != null) {
              String str = bill.getAccountName();
              if (str != null)
                  g2d.drawString(str, x0, y);
              if (bGoes) {
                  str = bill.getMinuteTime(bill.getGoesTotalTime());
                  g2d.drawString(str, x1, y);
                  str = nf.format(bill.getGoesTotalCharge());
                  x = x2 - prtFm.stringWidth(str);
                  g2d.drawString(str, x, y);
              }
              if (bLogin) {
                  str = bill.getMinuteTime(bill.getLoginTotalTime());
                  g2d.drawString(str, x3, y);
                  str = nf.format(bill.getLoginTotalCharge());
                  x = x4 - prtFm.stringWidth(str);
                  g2d.drawString(str, x, y);
              }
              str = nf.format(bill.getTotalCharge());
              x = x5 - prtFm.stringWidth(str);
              g2d.drawString(str, x, y);
              lines++;
              y += prtFontH;
              if (lines >= MAX_LINES) {
                 i++;
                 break;
              }
          }   
      } 
      prtLineIndex = i;
      String str = " page "+(pageIndex+1);
      w = prtFm.stringWidth(str) + 4;
      g2d.drawString(str, pageWidth - w, pageHeight- 6);
      return Printable.PAGE_EXISTS;
    } 

    public int printTxt(PrintWriter fd) {
      if (billList == null)
          return -1;

      int i;
      int lines;

      NumberFormat nf = aProps.getNumberFormat();
      lines = 0;
      int col1 = 0;
      int col2 = "acquisition time".length();
      int col3 = "total charge".length();
      for (i = 0; i < billList.size(); i++) {
          BillOne bill = billList.elementAt(i);
          if (bill != null) {
              String str = bill.getAccountName();
              if (str.length() > col1)
                 col1 = str.length();
          }
      } 
      String str;
      for (i = 0; i < billList.size(); i++) {
          BillOne bill = billList.elementAt(i);
          if (bill != null) {
              str = String.format("%"+col1+"s", bill.getAccountName() );
              if (str != null)
                  fd.print(str +"   ");
              if (bGoes) {
                  str = String.format("%"+col2+"s",bill.getMinuteTime(bill.getGoesTotalTime()) );
                  fd.print(str);
                  str = String.format("%"+col3+"s",nf.format(bill.getGoesTotalCharge()) );
                  fd.print(str);
              }
              if (bLogin) {
                  str = String.format("%"+col2+"s",bill.getMinuteTime(bill.getLoginTotalTime()) );
                  fd.print(str +"   ");
                  str = String.format("%"+col3+"s",nf.format(bill.getLoginTotalCharge()) );
                  fd.print(str +"   ");
              }
              str = String.format("%"+col3+"s",nf.format(bill.getTotalCharge()) );
              fd.println(str);
              lines++;
              if (lines >= MAX_LINES) {
                 i++;
                 break;
              }
          }   
      } 
      return Printable.PAGE_EXISTS;
    } 

    public void writeToCsv(PrintWriter fd) {
        if (billList == null)
            return;
        NumberFormat nf = aProps.getNumberFormat();

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

        for (int i = 0; i < billList.size(); i++) {
            BillOne bill = billList.elementAt(i);
            if (bill != null) {
                String str = bill.getAccountName();
                fd.print("\"");
                fd.print(str);
                fd.print("\",");
                if (bGoes) {
                   str = bill.getMinuteTime(bill.getGoesTotalTime());
                   fd.print("\"");
                   fd.print(str);
                   fd.print("\",");
                   str = nf.format(bill.getGoesTotalCharge());
                   fd.print("\"");
                   fd.print(str);
                   fd.print("\",");
               }
               if (bLogin) {
                   str = bill.getMinuteTime(bill.getLoginTotalTime());
                   fd.print("\"");
                   fd.print(str);
                   fd.print("\",");
                   str = nf.format(bill.getLoginTotalCharge());
                   fd.print("\"");
                   fd.print(str);
                   fd.print("\",");
               }
               str = nf.format(bill.getTotalCharge());
               fd.print("\"");
               fd.print(str);
               fd.println("\"");
            }
        }
    }

    private class SummaryAllLayout implements LayoutManager {
        int deltaY = 15;

        public void addLayoutComponent(String str, Component c) {
        }

        public void layoutContainer(Container parent) {
            JLabel tmpLabel;
            int width = parent.getWidth();

            int xIW = logo.getIconWidth();
            int yIH = logo.getIconHeight();
            int x = width-xIW-20;

            logoLabel.setBounds(x, 20, xIW, yIH);
            int nLines = yIH/deltaY+2;  // 1 for roundoff, 1 for y-offset o flogo

            headerLabel[0].setBounds(xOffset, yIH-2*deltaY+10, width-x, 2*deltaY);
            headerLabel[1].setBounds(xOffset, yIH-deltaY+20, width-x, deltaY);
            linesY0 = yIH + 50;
        }

        public Dimension minimumLayoutSize( Container parent ) {
             return ( new Dimension(0,0));
        }

        public Dimension preferredLayoutSize( Container parent ) {
            int h = logo.getIconHeight() + 50 + fontH * 3;
            if (billList != null)
                h = h + fontH * billList.size();
            return ( new Dimension(425, h));
        }
        public void removeLayoutComponent( Component c  ) {
        }
    }
}
