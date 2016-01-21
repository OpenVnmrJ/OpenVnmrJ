/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui.shuf;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import javax.swing.*;
import javax.swing.event.*;

import  vnmr.bo.*;
import  vnmr.ui.*;
import  vnmr.util.*;

/**
 * A DatePanel consists of a calendar date, plus controls to modify the
 * date.
 */
class DatePanel extends JComponent {
    // ==== instance variables
    /** calendar date */
    private GregorianCalendar cal;
    /** date label */
    private UnderlineButton dateButton;
    /** date listeners */
    private Vector listeners;

    /**
     * constructor
     * @param cal date
     */
    public DatePanel(GregorianCalendar cal2) {
	listeners = new Vector();
	setLayout(new FlowLayout());

	dateButton = new UnderlineButton();
	dateButton.setBorder(BorderFactory.createEmptyBorder());
//	dateButton.setForeground(Color.blue);

//	JButton buttonLeft = new JButton(Util.getImageIcon("left.gif"));
	JButton buttonLeft = new JButton(Util.getImageIcon("open_arrow_left.png"));
	buttonLeft.setOpaque(false);
	buttonLeft.setBorder(BorderFactory.createEmptyBorder());
	add(buttonLeft);

	add(dateButton);

        //	JButton buttonRight = new JButton(Util.getImageIcon("right.gif"));
	JButton buttonRight = new JButton(Util.getImageIcon("open_arrow_right.png"));
	buttonRight.setOpaque(false);
	buttonRight.setBorder(BorderFactory.createEmptyBorder());
	add(buttonRight);

	if (cal2 != null)
	    setDate(cal2);


	buttonLeft.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
		//DatePanel.this.cal.add(Calendar.DATE, -1);
		//updateDateLabel();
		GregorianCalendar newCal =
		    (GregorianCalendar)DatePanel.this.cal.clone();
		newCal.add(Calendar.DATE, -1);
		notifyDateListeners(newCal);
	    }
	});

	buttonRight.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
		//DatePanel.this.cal.add(Calendar.DATE, 1);
		//updateDateLabel();
		GregorianCalendar newCal =
		    (GregorianCalendar)DatePanel.this.cal.clone();
		newCal.add(Calendar.DATE, 1);
		notifyDateListeners(newCal);
	    }
	});

        // Click on the date button/string and it brings up a tck calendar
        // which return the date selected.
 	dateButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                // create a Runnable so that I can start the calendar in a
                // thread and wait for the response in the thread without
                // handing up vnmrj.
                Runnable runCal = new Runnable() {
                    public void run() {
                        try {
                            Runtime rt = Runtime.getRuntime();
                            // Get the system directory where the 
                            // executable is
                            String dir = FileUtil.sysdir();

                            Process prcs = null;
                            try {
                                // Start up the calendar
                                prcs = rt.exec(dir + "/tcl/bin/xcal2");
                                // attach to its stdout
                                InputStream istrm = prcs.getInputStream();
                                BufferedReader bfr;
                                bfr = new BufferedReader(new InputStreamReader(istrm));

                                // wait until the calendar exits
                                String dateStr =  bfr.readLine();
                                // Was there any output?  If quit is 
                                // clicked, there will be none
                                if(dateStr != null && dateStr.length() > 0) {
                                    int date[] = {-1,-1,-1};
                                    StringTokenizer st = 
                                        new StringTokenizer(dateStr);
                                    for(int i=0; i < 3; i++) {
                                        if(st.hasMoreTokens()) {
                                            String digit = st.nextToken();
                                            date[i] = 
                                                Integer.parseInt(digit);
                                        }
                                    }
                                    if(date[0] > -1 && date[1] > -1 && 
                                       date[2] > -1) {
                                        GregorianCalendar newCal;
                                        newCal = new GregorianCalendar(
                                                                       date[0], date[1] -1, 
                                                                       date[2]);

                                        // Tell the locator to update the 
                                        // item in the sentence.
                                        notifyDateListeners(newCal);
                                    }
                                }
                            }
                            finally {
                                // It is my understanding that these streams are left
                                // open sometimes depending on the garbage collector.
                                // So, close them.
                                if(prcs != null) {
                                    OutputStream os = prcs.getOutputStream();
                                    if(os != null)
                                        os.close();
                                    InputStream is = prcs.getInputStream();
                                    if(is != null)
                                        is.close();
                                    is = prcs.getErrorStream();
                                    if(is != null)
                                        is.close();
                                }
                            }
                        }
                        catch (Exception e) {
                            Messages.postError("Problem getting date "
                                               + " from calendar");
                            Messages.writeStackTrace(e);
                            return;
                        }
                    }
                };

                // Start the thread, then just continue and let java run.
                Thread th = new Thread(runCal);
                th.start();
            }
        });

    } // DatePanel()

    /**
     * add a date listener
     * @param listener
     */
    public void addDateListener(DateListener listener) {
	listeners.addElement(listener);
    } // addDateListener()

    /**
     * notify date listeners of new date
     * @param newCal new date
     */
    public void notifyDateListeners(GregorianCalendar newCal) {
	Enumeration en = listeners.elements();
	for ( ; en.hasMoreElements(); ) {
	    ((DateListener)en.nextElement()).dateChanged(newCal);
	}
    } // notifyDateListeners()

    /**
     * set date
     * @param cal calendar date
     */
    public void setDate(GregorianCalendar cal) {
	this.cal = cal;
	updateDateLabel();
    } // setDate()

    /**
     * update dateButton according to calendar
     */
    private void updateDateLabel() {
	dateButton.setText(getDateStr());
	dateButton.repaint();
    } // updateDateLabel()

    /**
     * get date as a string
     * @return date string
     */
    private String getDateStr() {
	return cal.get(Calendar.YEAR) + "-" + (cal.get(Calendar.MONTH) + 1) +
	    			  "-" + cal.get(Calendar.DAY_OF_MONTH);
    } // getDateStr()


} // class DatePanel
