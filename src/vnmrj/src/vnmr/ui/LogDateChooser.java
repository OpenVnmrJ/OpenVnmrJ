/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.text.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;

public class LogDateChooser extends JPanel {

    int date=0;
    EtchedBorder border;
    Container c;
    DateFormatSymbols dfs;
    GregorianCalendar gc;
    JButton number[] = new JButton[31];
    JButton mUp, mDown, yUp, yDown;
    JPanel thisDialog;
    JLabel title;
    JLabel[] days = new JLabel[7];
    JLabel month, year;
    String[] months;
    String[] weekdays;

    public LogDateChooser(String label) {
        thisDialog = this;
        border = new EtchedBorder();
        setBorder(border);
        Font f = new Font("Helvetica",Font.PLAIN,10);
        title = new JLabel(label);
        add (title);
        setLayout(new DateLayout());
        dfs = new DateFormatSymbols();
        months = dfs.getMonths();
        weekdays = dfs.getShortWeekdays();
        for(int i=0;i<7;i++) {
            days[i] = new JLabel(weekdays[i+1]);
            days[i].setFont(f);
            add(days[i]);
        }
        gc = new GregorianCalendar();
        mDown = new JButton("<");
        mDown.setMargin(new Insets(0,0,0,0));
        mDown.addActionListener(new MDownEar());
        mDown.setBorderPainted(false);
        mDown.setFont(f);
        mDown.setForeground(Color.BLUE);
        add(mDown);
        mUp = new JButton(">");
        mUp.setMargin(new Insets(0,0,0,0));
        mUp.addActionListener(new MUpEar());
        mUp.setBorderPainted(false);
        mUp.setFont(f);
        mUp.setForeground(Color.BLUE);
        add(mUp);
        month = new JLabel(months[gc.get(Calendar.MONTH)]);
        month.setHorizontalAlignment(SwingConstants.CENTER);
        month.setFont(f);
        add(month);
        year  = new JLabel(new Integer(gc.get(Calendar.YEAR)).toString());
        year.setFont(f);
        add(year);
        yDown = new JButton("<");
        yDown.setMargin(new Insets(0,0,0,0));
        yDown.addActionListener(new YDownEar());
        yDown.setBorderPainted(false);
        yDown.setFont(f);
        yDown.setForeground(Color.BLUE);
        add(yDown);
        yUp = new JButton(">");
        yUp.setMargin(new Insets(0,0,0,0));
        yUp.addActionListener(new YUpEar());
        yUp.setBorderPainted(false);
        yUp.setFont(f);
        yUp.setForeground(Color.BLUE);
        add(yUp);
        //    System.out.println(year.getText());
        NumberEar numberEar = new NumberEar();
        for (int i=0; i<31; i++) {
            number[i] = new JButton(new Integer(i+1).toString());
            number[i].setMargin(new Insets(0,0,0,0));
            number[i].addActionListener(numberEar);
            number[i].setBorderPainted(false);
            number[i].setContentAreaFilled(false);
            number[i].setFont(f);
            number[i].setForeground(Color.BLUE);
            add(number[i]);
        }
        number[0].setForeground(Color.CYAN);
    }

    public Date getDate() {
        gc.set(Calendar.DAY_OF_MONTH,(date+1));
        gc.set(Calendar.AM_PM,Calendar.AM);
        gc.set(Calendar.HOUR,0);
        gc.set(Calendar.MINUTE,0);
        gc.set(Calendar.SECOND,0);
        return( gc.getTime() );
    }

    // For the ending date, we want the end of the selected day which is
    public Date getEndDate() {
        gc.set(Calendar.DAY_OF_MONTH,(date+1));
        gc.set(Calendar.AM_PM,Calendar.AM);
        gc.set(Calendar.HOUR,23);
        gc.set(Calendar.MINUTE,59);
        gc.set(Calendar.SECOND,59);
        return( gc.getTime() );
    }

    public class DateLayout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {
        }

        public void layoutContainer(Container parent) {
            //      System.out.println("Layout called");
            month.setText(months[gc.get(Calendar.MONTH)]);
            year.setText(new Integer(gc.get(Calendar.YEAR)).toString());
            gc.set(Calendar.DAY_OF_MONTH,1);
            int dayOffset = gc.get(Calendar.DAY_OF_WEEK)-1;
            int daysInMonth = gc.getActualMaximum(Calendar.DAY_OF_MONTH);
            //      System.out.println("Layout=>"+dayOffset);
            int xScale=10, yScale=10;
            title.setBounds(10,5,15*xScale,yScale);
            mDown.setBounds(10,25,(int)(1.5*xScale),yScale);
            month.setBounds((int)(10+1.5*xScale),25,52,yScale);
            mUp.setBounds((int)(62+1.5*xScale),25,2*xScale,yScale);
            yDown.setBounds(110,25,(int)(1.5*xScale),yScale);
            year.setBounds((int)(110+1.5*xScale),25,35,yScale);
            yUp.setBounds((int)(132+1.5*xScale),25,(int)(1.5*xScale),yScale);
            for (int i=0; i<7; i++) {
                days[i].setBounds((int)(2.2*i*xScale+10),45,(int)(2.2*xScale),yScale);
            }
            // Put number in their places for this month
            int i;
            for (i=0; i<daysInMonth; i++) {
                number[i].setBounds((int)(((i+dayOffset)%7)*2.2*xScale+10),((i+dayOffset)/7)*yScale+55,(int)(2.2*xScale),yScale);
                // Show them in case they were unshown for the prev month (see below)
                number[i].setVisible(true);
            }
            // This needs to unshow the remaining squares, else there can be 
            // numbers remaining from the previous month.  Like "31" for months
            // without 31 days and worse for Feb.
            // Continue where "i" left off in the loop above.
            for (;i < 31; i++) {
                number[i].setVisible(false);
            }
      

        }
        public Dimension minimumLayoutSize( Container parent ) {
            return ( new Dimension(200,200));
        }
        public Dimension preferredLayoutSize( Container parent ) {
            return ( new Dimension(200,200));
        }
        public void removeLayoutComponent( Component c  ) {
        }

    }
    private class NumberEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            JButton b = (JButton)ae.getSource();
            //      System.out.println("NumberEar: button="+b.getText());
            int i = new Integer(b.getText()).intValue()-1;
            number[date].setForeground(Color.BLUE);
            date=i;
            number[i].setForeground(Color.CYAN);
        }
    }
    private class MUpEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            gc.roll(Calendar.MONTH,1);
            //      System.out.println("Date now is: "+gc.getTime().toString());
            month.setText(months[gc.get(Calendar.MONTH)]);
        }
    }
    private class MDownEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            gc.roll(Calendar.MONTH,-1);
            //      System.out.println("Date now is: "+gc.getTime().toString());
            month.setText(months[gc.get(Calendar.MONTH)]);
        }
    }
    private class YUpEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            gc.roll(Calendar.YEAR,1);
            //      System.out.println("Date now is: "+gc.getTime());
            year.setText(new Integer(gc.get(Calendar.YEAR)).toString());
        }
    }
    private class YDownEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            gc.roll(Calendar.YEAR,-1);
            //      System.out.println("Date now is: "+gc.getTime().toString());
            year.setText(new Integer(gc.get(Calendar.YEAR)).toString());
        }
    }
}
