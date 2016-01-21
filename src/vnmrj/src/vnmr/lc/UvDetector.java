/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import vnmr.ui.StatusManager;

/**
 * This is the interface for communicating with UV detectors for LC.
 */
public abstract class UvDetector {

    /**
     * Send any status messages to this guy.
     */
    protected StatusManager m_statusManager;


    public UvDetector(StatusManager statusManager) {
        m_statusManager = statusManager;
    }

    /**
     * Return the name of this detector, typically the model number, as
     * a string.
     */
    public abstract String getName();

    /**
     * Read the status from the detector periodically.  This will include
     * the absorption at selected wavelengths, if possible.  These
     * wavelengths may be changed by downloadMethod().
     * @param period The time between samples in ms. If 0, read status once.
     */
    public abstract boolean readStatus(int period);

    /** See if the detector is ready to start a run. */
    public abstract boolean isReady();

    /** Open communication with the detector. */
    public abstract boolean connect();

    /** Shut down communication with the detector. */
    public abstract void disconnect();

    /** Determine if detector communication is OK. */
    public abstract boolean isConnected();

    /** Turn on the lamp(s). */
    public abstract boolean lampOn();

    /** Turn off the lamp(s). */
    public abstract boolean lampOff();

    /** Make the current absorption the zero reference. */
    public abstract boolean autoZero();

    /** Download a (constant) run method with maximum run time. */
    public abstract boolean downloadMethod(LcMethod params);

    /** See if the last downloaded method is up-to-date. */
    public abstract boolean isDownloaded(LcMethod params);

    /**
     * Start a run, if possible.
     */
    public abstract boolean start();

    /** Stop a run and save data, continue monitoring selected wavelength(s). */
    public abstract boolean stop();

    /** Stop a run in such a way that it can be resumed. */
    public abstract boolean pause();

    /** Restart a paused run. */
    public abstract boolean restart();

    /** Reset the detector state to be ready to start a new run. */
    public abstract boolean reset();

    /**
     * Selects who deals with the data.
     * @return Returns false if this detector cannot send data,
     * otherwise returns true.
     */
    public abstract boolean setDataListener(LcDataListener listener);

    /**
     * Selects channels and wavelengths to monitor for the chromatogram.
     * @param lambdas The wavelengths to monitor.
     * @param traces The corresponding traces to send them to.
     * @param chromatogramManager Where to send the data.
     * @return Returns "false" if this detector does not return monitor
     * absorptions separately from the spectra.
     */
    public abstract boolean setChromatogramChannels(float[] lambdas,
                                                    int[] traces,
                                                    LcRun chromatogramManager);

    /**
     * @return The expected interval between data points in ms.
     */
    public abstract int getDataInterval();

    /**
     * @return The string that uniquely identifies this detector type to the
     * software.
     */
    public abstract String getCanonicalIdString();

    /**
     * @return The string to show the user to identify the detector type.
     */
    public abstract String getIdString();

    /**
     * Get the wavelength range this type of detector is good for.
     * @return Two ints, the min and max wavelengths, respectively, in nm.
     */
    public abstract int[] getMaxWavelengthRange();

    /** Determine if this detector can return spectra. */
    public abstract boolean isPda();

    /**
     * The number of "monitor" wavelengths this detector can support.
     * These may be monitored either from getting status or from
     * reading an analog-out value, but must be obtainable outside
     * of a run.
     */
    public abstract int getNumberOfMonitorWavelengths();

    /**
     * The number of "analog outputs" this detector supports, and
     * that we want the software to support.
     * If we can get all the data we need digitally, this should be
     * set to 0.
     */
    public abstract int getNumberOfAnalogChannels();

    /**
     * Send status strings indicating the UvDetector is not connected.
     */
    public void setDisconnectedStatus() {
        m_statusManager.processStatusData("uvTitle UV-Vis");
        m_statusManager.processStatusData("uvId ");
        m_statusManager.processStatusData("uvTime ");
        m_statusManager.processStatusData("uvStatus ");
        m_statusManager.processStatusData("uvLampStatus ");
        m_statusManager.processStatusData("uvLambda ");
        m_statusManager.processStatusData("uvLambda2 ");
        m_statusManager.processStatusData("uvAttn ");
        m_statusManager.processStatusData("uvAttn2 ");

        boolean ok = true;
        m_statusManager.processStatusData("uvLampOnOk " + ok);
        m_statusManager.processStatusData("uvLampOffOk " + ok);
        m_statusManager.processStatusData("uvAutoZeroOk " + ok);
        m_statusManager.processStatusData("uvResetOk " + ok);
        m_statusManager.processStatusData("uvDownloadOk " + ok);
        m_statusManager.processStatusData("uvStartOk " + ok);
        m_statusManager.processStatusData("uvStopOk " + ok);
    }
}
