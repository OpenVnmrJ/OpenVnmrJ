/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

/**
 * Interface for someone who receives UV or PDA data, usually in real
 * time.  Note that for a one or two channel UV detector, we just get
 * passed one or two wavelengths in setWavelengths(), and one or two
 * data points per "spectrum".
 */
public interface LcDataListener {

    /**
     * Called before any data is sent to define the wavelenths that
     * apply to all the subsequent data.  This is essentially also
     * the signal that a new data set is coming.
     * @param wavelengths An array equal in length to the number of
     * points in each spectrum, giving the wavelength in nm of each point.
     */
    public void setUvWavelengths(float[] wavelengths);

    /**
     * Called whenever a new spectrum is available.
     * @param time The run time in minutes of the spectrum.
     * @param spectrum An array equal in length to the number of
     * wavelengths sent earlier, giving the absorption in AU at
     * each wavelength.
     */
    public void processUvData(double time, float[] spectrum);

    /**
     * Called after the last spectrum has been sent.
     * At this point, any cleanup action should be taken, such as
     * flushing any remaining data or closing files.
     */
    public void endUvData();
}
