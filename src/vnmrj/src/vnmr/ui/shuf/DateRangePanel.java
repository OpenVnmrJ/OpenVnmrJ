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
import javax.swing.*;
import javax.swing.event.*;

import  vnmr.bo.*;
import  vnmr.ui.*;
import  vnmr.util.*;

/**
 * Allow user to specify a range of dates.
 *
 * @author Mark Cao
 */
public class DateRangePanel extends JComponent {
    // ==== static variables
    /** range type: all */
    public static final String ALL = "on any date";
    /** range type: since */
    public static final String SINCE = "since";
    /** range type: before */
    public static final String BEFORE = "before";
    /** range type: between */
    public static final String BETWEEN = "between";
    /** all range types as strings */
    public static final String[] ALLRANGES =
    { "on any date", "since", "before", "between" };

    // ==== instance variables
    /** Table of values. Key-value pairs are as follows:
     *    "DateRange" => String
     *    "date-0" => GregorianCalendar
     *    "date-1" => GregorianCalendar
     * Range types may be the constant values ALL, ON, SINCE, BEFORE,
     * and BETWEEN. Depending on the range type, date-0 and date-1
     * may or may not have to be provided.
     */
    private Hashtable statement;
    /** range button (to select "since", "between", etc.) */
    private DateRangeButton rangeButton;
    /** first date panel */
    private DatePanel datePanel1;
    /** second date panel */
    private DatePanel datePanel2;
    /** label with the word 'and' */
    private JLabel andLabel;

    /**
     * constructor
     * @param date1 GregorianCalendar
     * @param date2 GregorianCalendar
     */
    public DateRangePanel(GregorianCalendar date1, GregorianCalendar date2) {

	setLayout(new FlowLayout(FlowLayout.CENTER, 3, 0));
	
	rangeButton = new DateRangeButton();
	add(rangeButton);

	datePanel1 = new DatePanel(date1);
	datePanel2 = new DatePanel(date2);

	andLabel = new JLabel("and");

	add(datePanel1);
	add(andLabel);
	add(datePanel2);

	/* Define behavior that triggers
	 * history changes. */
	rangeButton.addPopListener(new PopListener() {
	    public void popHappened(String popStr) {
		SessionShare sshare = ResultTable.getSshare();
		StatementHistory history = sshare.statementHistory();
		history.append("DateRange", popStr);
	    }
	});
	datePanel1.addDateListener(new DateListener() {
	    public void dateChanged(GregorianCalendar cal) {
		SessionShare sshare = ResultTable.getSshare();
		StatementHistory history = sshare.statementHistory();
		history.append("date-0", cal);
	    }
	});
	datePanel2.addDateListener(new DateListener() {
	    public void dateChanged(GregorianCalendar cal) {
		SessionShare sshare = ResultTable.getSshare();
		StatementHistory history = sshare.statementHistory();
		history.append("date-1", cal);
	    }
	});

	updateRange();
    } // DateRangePanel()

    /**
     * update the panel
     */
    public void updateRange() {

	SessionShare sshare = ResultTable.getSshare();
	if(sshare == null)
	    return;
	StatementHistory history = sshare.statementHistory();
	if(history == null)
	    return;

	Hashtable statement = history.getCurrentStatement();

	String dateRange = (String)statement.get("DateRange");

	if(dateRange == null)
	    return;

	if(dateRange.equals(SINCE)) {
	    setDatePanelVal(datePanel1, "date-0");
	    rangeButton.setText(SINCE);
	    datePanel1.setVisible(true);
	    datePanel2.setVisible(false);
	    andLabel.setVisible(false);
	}
	else if(dateRange.equals(BEFORE)) {
	    setDatePanelVal(datePanel1, "date-0");
	    rangeButton.setText(BEFORE);
	    datePanel1.setVisible(true);
	    datePanel2.setVisible(false);
	    andLabel.setVisible(false);
	}
	else if(dateRange.equals(BETWEEN)) {
	    setDatePanelVal(datePanel1, "date-0");
	    setDatePanelVal(datePanel2, "date-1");
	    rangeButton.setText(BETWEEN);
	    datePanel1.setVisible(true);
	    datePanel2.setVisible(true);
	    andLabel.setVisible(true);
	}
	else { // ALL and default
	    rangeButton.setText(ALL);
	    datePanel1.setVisible(false);
	    datePanel2.setVisible(false);
	    andLabel.setVisible(false);
	}
    } // UpdateRange()

    /**
     * Based on the key, set the date of the date panel.
     * @param datePanel date panel
     * @param key key
     */
    private void setDatePanelVal(DatePanel datePanel, String key) {
	SessionShare sshare = ResultTable.getSshare();
	StatementHistory history = sshare.statementHistory();
	Hashtable statement = history.getCurrentStatement();

	datePanel.setDate((GregorianCalendar)statement.get(key));
    } // setDatePanelVal()

    /**
     * update panel values
     */
    public void updateValues(Hashtable statement) {
	updateRange();
    } // updateValues()


} // class DateRangePanel

/**
 * provides for selection of a range keyword ("since", "between", etc.)
 */
class DateRangeButton extends PopButton {
    /**
     * constructor
     */
    DateRangeButton() {
	super(DateRangePanel.ALLRANGES, (boolean) true);
	setOpaque(false);

        addMouseListener(new MouseAdapter() {
                public void mousePressed(MouseEvent e) {
                    int button = e.getButton();

                    // This item does not show its menu with a button 3 press.
                    // Since that is how it work for the JTextField items,
                    // I need this to work that way also.  Catch button 3
                    // and force an actionPerformed() to cause the popup
                    // menu to come up.
                    if (button == MouseEvent.BUTTON3) {
                        // Create an ActionEvent
                        Object source = (Object) e.getSource();
                        int id = e.getID();
                        // Set command = btn # where # is the button number
                        // We need to catch this in PopButton and only allow
                        // Btn 3, not Btn 1.  That is, we want this item to
                        // act like the Text items and open a menu on the 
                        // btn 3, and do nothing on btn 1.  That is not the
                        // normal case for a JButton.
                        ActionEvent av = new ActionEvent(source, id, "Btn " +
                                                                     button);
                        // Fire it off
                        fireActionPerformed(av);
                    }
                }
            });

    } // DateRangeButton()
} // class DateRangeButton
