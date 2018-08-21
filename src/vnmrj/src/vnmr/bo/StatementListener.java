/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.util.*;

/**
 * Listener for the following types of messages:
 * <ul>
 * <li> state of movability back has changed
 * <li> state of movability forward has changed
 * <li> new current statement
 * </ul>
 *
 * @author Mark Cao
 */
public interface StatementListener {
    /**
     * notification of a new statement
     * @param statement new statement
     */
    public void newStatement(Hashtable statement);

    /**
     * notification that back movability has changed
     * @param state boolean
     */
    public void backMovabilityChanged(boolean state);

    /**
     * notification that forward movability has changed
     * @param state boolean
     */
    public void forwardMovabilityChanged(boolean state);

    /**
     * notification of a change in the list of saved statements
     * @param 
     */
    public void saveListChanged();

} // interface StatementListener
