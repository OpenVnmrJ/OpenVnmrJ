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
import java.io.*;
import java.text.*;
import java.util.*;
import javax.swing.*;

public class Bill  extends JPanel{

  private AccountInfo2 ai;
  private AProps aProps;
  private ArrayList lines, labels;
  private ImageIcon logo;
  private JLabel logoLabel;
  private JLabel name,address1, address2, cityState, country;
  private JLabel accountNumber,billDate,dueDate,totalCost;
  private JLabel account,bill,due,cost;
  JPanel p;
  private JScrollPane jsp;
  private JSeparator js1,js2;
  private int nLines;


  public Bill(AccountInfo2 ai) {
    double sumTotal = 0.0;
    this.ai = ai;
    setLayout( new BorderLayout() );
    
    p = new JPanel();
    p.setBackground(Color.WHITE);
    setSize(425+33,584);  // page + frame +scrollbar
    p.setLayout( new BillLayout() );
    p.setBackground(Color.WHITE);
    
    String accountStr = ai.account();

    aProps = AProps.getInstance();
    logo = aProps.logo();
    logoLabel = new JLabel(logo);    p.add(logoLabel);

    Font f = new Font("Serif",Font.PLAIN,12);
    name = new JLabel(ai.name());
    name.setFont(f);                      p.add(name);
    address1 = new JLabel(ai.address1());
    address1.setFont(f);                  p.add(address1);
    address2 = new JLabel(ai.address2());
    address2.setFont(f);                  p.add(address1);
    cityState = new JLabel(ai.city()+", "+ai.zip());
    cityState.setFont(f);                 p.add(cityState);
    country = new JLabel(ai.country());
    country.setFont(f);                 p.add(country);

    Font f2 = new Font("Arial",Font.PLAIN,8);
    accountNumber = new JLabel("ACCOUNT NUMBER");
    accountNumber.setFont(f2);            p.add(accountNumber);
    billDate = new JLabel("BILLING DATE");
    billDate.setFont(f2);                 p.add(billDate);
    dueDate = new JLabel("DUE DATE");
    dueDate.setFont(f2);                  p.add(dueDate);
    totalCost = new JLabel("AMOUNT DUE");
    totalCost.setFont(f2);                p.add(totalCost);

    Font f3 = new Font("Arial",Font.PLAIN,12);
    account = new JLabel(accountStr);
    account.setFont(f3);                  p.add(account);
    StringBuffer date = new StringBuffer();
    GregorianCalendar gc = new GregorianCalendar();
    SimpleDateFormat sdf = new SimpleDateFormat("MMM d,yyyy");
    sdf.format(gc.getTime(),date,new FieldPosition(0));
    bill = new JLabel( date.toString() );
    bill.setFont(f3);                     p.add(bill);
    gc.add(Calendar.DATE,31);
    date = new StringBuffer();
    sdf.format(gc.getTime(),date,new FieldPosition(0));
    due = new JLabel( date.toString() );
    due.setFont(f3);                      p.add(due);

    NumberFormat nf = aProps.getCurrencyFormat();
    cost = new JLabel( nf.format( 1.23 ) );
    cost.setFont(f3);                      p.add(cost);

    js1 = new JSeparator();                p.add(js1);
    js2 = new JSeparator();                p.add(js2);

    int j=0, evenLine=0;
    Color bg;
    JLabel dateLabel,tmpLabel;
    Double tmpD;
    String onDate, tmpStr,str;
    Integer tmpInt;
    labels = new ArrayList();
    double doubleTmp;

    StringTokenizer st = new StringTokenizer(ai.oAndOs());
    st.nextToken(); // skip "operator "
    int nOAndOs = st.countTokens();
    nLines = 0;
    for (int i=0; i<nOAndOs; i++) {
        str = st.nextToken();
        lines = ReadLast.getInstance(ai).getLines(str);
        sumTotal += ReadLast.getInstance(ai).total() ;
// System.out.println("sumTotal="+sumTotal);
// System.out.println("done ReadLast.getInstance().getLines()");
        int n = lines.size();
        nLines += n;
// System.out.println("no of Lines "+n/4);
        for (j=0; j<n; j += 4) {
            evenLine ^= 1;
            if (evenLine == 0) { bg = Color.BLUE;  }
            else               { bg = Color.BLACK; }
// System.out.println("... "+j);

            // Who and when logged in
            tmpStr  = (String)lines.get(j);
            // date
            onDate = (String)lines.get(j+1);
            tmpLabel = new JLabel(tmpStr+": "+onDate);
            tmpLabel.setFont(f);      tmpLabel.setForeground(bg);
            tmpLabel.setHorizontalAlignment(SwingConstants.LEFT);
            labels.add(tmpLabel);     p.add(tmpLabel);
// System.out.println(tmpLabel.getText());
            // how long
            tmpLabel = new JLabel((String)lines.get(j+2));
            tmpLabel.setFont(f);      tmpLabel.setForeground(bg);
            tmpLabel.setHorizontalAlignment(SwingConstants.RIGHT);
            labels.add(tmpLabel);     p.add(tmpLabel);
// System.out.println(tmpLabel.getText());
            // rate
            tmpD = new Double((ai.getRate(onDate)).loginhr);
            nf = aProps.getNumberFormat();
//  Because the rate may vary, we leave this blank
//            tmpLabel = new JLabel(nf.format(tmpD));
            tmpLabel = new JLabel(" ");
            tmpLabel.setFont(f);      tmpLabel.setForeground(bg);
            tmpLabel.setHorizontalAlignment(SwingConstants.RIGHT);
            labels.add(tmpLabel);     p.add(tmpLabel);
// System.out.println(tmpLabel.getText());     
            tmpLabel = new JLabel((String)(lines.get(j+3)));
            tmpLabel.setFont(f);      tmpLabel.setForeground(bg);
            tmpLabel.setHorizontalAlignment(SwingConstants.RIGHT);
            labels.add(tmpLabel);     p.add(tmpLabel);
// System.out.println(tmpLabel.getText());
        }
    }
    nf = aProps.getCurrencyFormat();
    cost.setText( nf.format( sumTotal ) );
    
    JScrollPane jsp = new JScrollPane(p);
    super.add(jsp);
  }

  private class BillLayout implements LayoutManager {
    int deltaY = 15;
    public void addLayoutComponent(String str, Component c) {
    }
    public void layoutContainer(Container parent) {
      int x = parent.getWidth();
      int y = parent.getHeight();
      int w, dx;
//      System.out.println("Page: "+x+","+y);
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
      JLabel tmpLabel;
      int n = labels.size();
      x = x - 10;
      for (int i=0; i<n; i+=4) {
        tmpLabel = (JLabel)labels.get(i);
        w = x*40/100;
        dx = 5;
        tmpLabel.setBounds(dx, yIH+50+((i/4)*deltaY), w, deltaY);
        tmpLabel = (JLabel)labels.get(i + 1);
        dx = dx + w;
        w = x* 20/100;
        tmpLabel.setBounds(dx, yIH+50+((i/4)*deltaY), w, deltaY);
        dx = dx + w;
        tmpLabel = (JLabel)labels.get(i + 2);
        tmpLabel.setBounds(dx, yIH+50+((i/4)*deltaY), w,  deltaY);
        dx = dx + w;
        tmpLabel = (JLabel)labels.get(i + 3);
        tmpLabel.setBounds(dx, yIH+50+((i/4)*deltaY), w, deltaY);
      }
    }
    public Dimension minimumLayoutSize( Container parent ) {
      return ( new Dimension(0,0));
    }
    public Dimension preferredLayoutSize( Container parent ) {
      int a;
      a = logo.getIconHeight() + 4 + nLines * deltaY / 4 + 50;
      return ( new Dimension(425,a));
    }
    public void removeLayoutComponent( Component c  ) {
    }
  }
}
