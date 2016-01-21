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
 */
package vnmr.cryomon;

import java.util.StringTokenizer;


/**
 * This is the interface for communicating with the CryoBay.
 */
public interface CryoMonitor {
    /**
     * Return the name of this detector, typically the model number, as
     * a string.
     */
    public String getName();

    /**
     * Open communication with the cryobay.
     */
    public void connect(String host, int port);

    /**
     * Shut down communication with the cryobay.
     */
    public void disconnect();

    /**
     * Determine if cryobay communication is OK.
     */
    public boolean isConnected();

    /**
     * Send a command to the cryobay.
     * @param args The string to send.
     */
    public boolean sendToCryoBay(String command);

}
