/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.lc;

import java.util.StringTokenizer;


/**
 * This is the interface for communicating with auto-samplers for LC.
 */
public interface AutoSampler {
    /**
     * Return the name of this detector, typically the model number, as
     * a string.
     */
    public String getName();

    /**
     * See if the autosampler is ready to start a run.
     */
    public boolean isReady();

    /**
     * Open communication with the autosampler.
     */
    public void connect(String host, int port);

    /**
     * Shut down communication with the autosampler.
     */
    public void disconnect();

    /**
     * Determine if autosampler communication is OK.
     */
    public boolean isConnected();

    /**
     * Download an injection method to the autosampler.
     *
     * @param injVol Injection volume in units of 10 nL.
     * @param tray Tray number to use, starting from 1.
     * @param row Row number to use, starting from 1.
     * @param col Column number to use, starting from 1.
     * @param flushVol Flush volume in units of 10 nL.
     * @param mode The type of injection -- a device-specific code.
     * @param command If this is a non-blank string, the method is started
     * and the "command" is sent to VnmrBG when the injection occurs.
     * @return true if downloaded, false if download failed.
     */
    public boolean downloadMethod(int injVol, int tray, int row, int col,
                                  int flushVol, int mode, String command);

    /**
     * Start the current injection method.
     */
    public boolean startMethod();

    /**
     * Send a command to the auto-sampler.
     * @param args The series of tokens specifying the command.
     */
    public boolean command(StringTokenizer args);

}
