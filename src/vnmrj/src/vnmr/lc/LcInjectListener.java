/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.lc;

public interface  LcInjectListener {
    /**
     * This is called to set the command to run when an injection
     * occurs, i.e., when handleInjection() is called. Calling
     * handleInjection also clears the command, so this must be
     * called before every injection. The command string is sent
     * to VnmrBG.
     */
    public void setInjectionCommand(String command);

    /**
     * This is called when the injection loop is filled with sample and
     * the valve switches to "inject".  The listener does whatever is
     * appropriate for the current system state.
     * If the injectionCommand is set to a non-blank string, it is sent
     * to VnmrBG.  Otherwise, if the run
     * has paused waiting for an injection, the run is restarted.
     */
    public void handleInjection();
}

