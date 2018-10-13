/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.cryo;


/**
 * This is the interface for communicating with the CryoBay.
 */
public interface CryoBay {

    /**
     * Open communication with the cryobay.
     * @param host The host name or IP of the cryobay.
     * @param port The port number to use.
     */
    public void connect(String host, int port);

    /**
     * Shut down communication with the cryobay.
     */
    public void disconnect();

    /**
     * Determine if cryobay communication is OK.
     * @return True if OK.
     */
    public boolean isConnected();

    /**
     * Start the cryobay.
     * @return True if OK.
     */
    public boolean start();

    /**
     * Stop the cryobay.
     * @return True if OK.
     */
    public boolean stop();

    /**
     * Detach the probe from the cryobay.
     * @return True if OK.
     */
    public boolean detach();

    /**
     * Send a command to the cryobay.
     * @param command The string to send.
     * @param priority If 1, message is sent before regular priority messages.
     *        If 2, normal message. If 3, send only if not already queued.
     * @return True if OK.
     */
    public boolean sendToCryoBay(String command, Priority priority);

}
