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
import java.util.*;
import javax.swing.*;

import  vnmr.bo.*;
import  vnmr.ui.*;

/**
 * This component displays the current statement (query).
 *
 * @author Mark Cao
 */
public class StatementView extends JComponent implements StatementListener {
    /** session share */
    private SessionShare sshare;
    /** card layout */
    private CardLayout cardLayout;
    /** table of encountered statement types (String => Component) */
    private Hashtable statementTypes;

    /**
     * constructor
     * @param sshare session share
     */
    public StatementView(SessionShare sshare) {
	this.sshare = sshare;

	cardLayout = new CardLayout();
	setLayout(cardLayout);
	statementTypes = new Hashtable();

	LocatorHistoryList lhl = sshare.getLocatorHistoryList();
	if(lhl !=null)
		lhl.addAllStatementListeners(this);

    } // StatementView()

    /**
     * given a fixed width, get preferred height
     * @param height height
     */
    public int getPreferredHeight(int width) {
	StatementDefinition sd;

	sd = sshare.getCurrentStatementType();
	if(sd == null)
	    return 0;

	return sd.getPreferredHeight(width);
    } // getPreferredHeight()

    /**
     * Respond to a request for a new statement. If that statement type
     * is already represented in the card layout, then do not create a
     * new one.  Simply switch the card layout to that type, and call for
     * an update.
     * @param statement Hashtable
     */
    public void newStatement(Hashtable statement) {
	String statementType = (String)statement.get("Statement_type");
	if (!statementTypes.containsKey(statementType)) {
	    StatementDefinition comp =
		sshare.shufflerService().getAStatement(statementType);
	    if(comp == null)
		return;
	    statementTypes.put(statementType, comp);
	    add(statementType, comp);
	    comp.updateValues(statement, false);
	    sshare.setCurrentStatementType(comp);
	} else {
	    sshare.setCurrentStatementType(
		(StatementDefinition)statementTypes.get(statementType));
	    sshare.getCurrentStatementType().updateValues(statement, false);
	}
	// show type
	cardLayout.show(this, statementType);
    } // newStatement()

    /**
     * notification that back movability has changed
     * @param state boolean
     */
    public void backMovabilityChanged(boolean state) {}

    /**
     * notification that forward movability has changed
     * @param state boolean
     */
    public void forwardMovabilityChanged(boolean state) {}

    /**
     * notification of a change in the list of saved statements
     * @param 
     */
    public void saveListChanged() {}


} // class StatementView
