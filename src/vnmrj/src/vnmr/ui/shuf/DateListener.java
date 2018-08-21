/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui.shuf;

import java.util.*;

/**
 * This interface provides notification of date changes.
 *
 * @author Mark Cao
 */
public interface DateListener {
    /**
     * notify of date change
     * @param cal new date
     */
    public void dateChanged(GregorianCalendar cal);

} // interface DateListener
