/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

/**
 * Listener of pop selections.
 *
 * @author Mark Cao
 */
public interface PopListener {
    /**
     * notify of a popup selection string
     * @param popStr popup selection
     */
    public void popHappened(String popStr);

} // interface PopListener
