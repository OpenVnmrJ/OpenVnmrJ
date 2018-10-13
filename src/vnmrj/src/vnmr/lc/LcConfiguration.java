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

import vnmr.bo.VContainer;
import vnmr.lc.VLcParamTracker;
import vnmr.ui.ExpPanel;
import vnmr.util.ButtonIF;
import vnmr.util.Messages;

import static vnmr.lc.LcMethodParameter.*;

/**
 * This class is used to hold configuration information that applies
 * to all runs.
 * Run specific values (which depend on the method) should be kept
 * in LcMethodEditor.
 */
public class LcConfiguration extends VContainer {

    // Containers for config values
    // These all have set and get methods
    private double[] m_transferTimes = new double[0];
    private double m_referenceTime = 0;
    // TODO: Do the following belong here, or in LcMethodEditor?
    private boolean m_bMsAutoPumpControl = true;
    private String m_nmrDoneAction = null;
    private String m_peakAction = null;
    private boolean m_bManualFlowControl = false;

    // Containers for parameter listeners
    private VLcParamTracker m_myIpAddress;
    private VLcParamTracker m_msSyncDelay;
    private VLcParamTracker m_numberOfLoops;
    private VLcParamTracker m_traceNameTags;
    private VLcParamTracker m_lampSelection;
    private VLcParamTracker m_msPumpFlow;
    private VLcParamTracker m_nmrProbeVolume;
    private VLcParamTracker m_uvFlowCellId;
    private VLcParamTracker m_uvFlowCellRatio;
    private VLcParamTracker m_loopVolume;
    private VLcParamTracker m_pumpToLoopTime;
    private VLcParamTracker m_lcTraceNametags;
    private VLcParamTracker[] m_lcConfigSolv;

    /**
     * Normal constructor.
     */
    public LcConfiguration(ButtonIF vif) {
        super(null, vif, "LcConfig");

        // Create parameter listeners
        m_myIpAddress = addParam("lcMyIpAddress", STRING_TYPE);
        m_msSyncDelay = addParam("msSyncDelay", DOUBLE_TYPE);
        m_numberOfLoops = addParam("lcAnalyteCollector", INT_TYPE);
        m_traceNameTags = addParam("lcTrace", STRING_TYPE);
        m_lampSelection = addParam("lcLampSelection", INT_TYPE);
        m_msPumpFlow = addParam("msPumpFlow", DOUBLE_TYPE);
        m_nmrProbeVolume = addParam("lcNmrProbeVolume", DOUBLE_TYPE);
        m_uvFlowCellId = addParam("lcFlowCell", INT_TYPE);
        m_uvFlowCellRatio = addParam("lcFlowCellRatio", DOUBLE_TYPE);
        m_loopVolume = addParam("lcLoopVolume", DOUBLE_TYPE);
        m_pumpToLoopTime = addParam("lcXferDelayPumpToLoop", DOUBLE_TYPE);
        m_lcConfigSolv = new VLcParamTracker[3];
        for (int i = 0; i < 3; i++) {
            String name = "lcConfigSolv" + (char)('A' + i);
            m_lcConfigSolv[i] = addParam(name, STRING_TYPE);
        }

        ExpPanel.addExpListener(this);
        updateValue();
    }

    /**
     * Add a VLcParamTracker to the panel.
     * Creates the item and adds it to the panel, so that changes in
     * the parameter value will be tracked.
     * The "type" should be one of the strings defined in LcMethodParameter:
     * INT_TYPE, DOUBLE_TYPE, STRING_TYPE, or BOOLEAN_TYPE.
     * @param param The name of the parameter to track.
     * @param type The parameter type.
     * @return The handle for the VLcParamTracker created.
     */
    private VLcParamTracker addParam(String param, String type) {
        VLcParamTracker item = new VLcParamTracker(null, getVnmrIF(), param);
        item.setAttribute(VARIABLE, param);
        item.setAttribute(SUBTYPE, type);
        add(item);
        return item;
    }


    /*
     * "Get" methods used to acces Vnmr parameter values:
     */

    /**
     * The IP address of this host.
     * (NB: No longer needed.)
     */
    public String getMyIpAddress() {
        String rtn = (String)m_myIpAddress.getValue();
        Messages.postDebug("LcConfiguration",
                           "getMyIpAddress() returns " + rtn);
        return rtn;
    }

    /**
     * Startup delay for the Mass Spectrometer.
     * The delay between the start command and the start of the MS run
     * (in seconds).
     */
    public double getMsSyncDelay() {
        double rtn = ((Double)m_msSyncDelay.getValue()).doubleValue();
        Messages.postDebug("LcConfiguration",
                           "getMsSyncDelay() returns " + rtn);
        return rtn;
    }

    /**
     * The number of loops in the analyte collector.
     * This number includes the bypass valve position.
     * Returns 0 if there is no analyte collector.
     */
    public int getNumberOfLoops() {
        int rtn = ((Integer)m_numberOfLoops.getValue()).intValue();
        Messages.postDebug("LcConfiguration",
                           "getNumberOfLoops() returns " + rtn);
        return rtn;
    }

    /**
     * Returns true if there is an analyte collector.
     */
    public boolean isAcHere() {
        return getNumberOfLoops() > 0;
    }

    /**
     * Split a trace's tag-name string into separate string for
     * the tag and the name.
     * @return Array of 2 strings, the first is the name, the second
     * is the tag.
     * @see #getChannelNameTag(int)
     */
    private static String[] splitNameTag(String nametag) {
        String[] rtn = new String[2];
        rtn[0] = nametag;       // Fallback name is the whole string
        rtn[1] = "";            // Fallback tag is empty
        if (nametag != null && nametag.startsWith(":")) {
            StringTokenizer toker = new StringTokenizer(nametag);
            if (toker.countTokens() > 1) {
                rtn[1] = toker.nextToken();  // First token is tag
                rtn[0] = toker.nextToken("").trim(); // Remainder is name
            }
        }
        return rtn;
    }

    /**
     * Returns the nametag of the specified trace.
     * An example nametag is ":uv1: PDA 335 Out-1". The tag is the
     * initial token (up to the space) and the rest is the name.
     * Returns a 0 length string if the trace number is undefined.
     * @param itrace Which trace to get, starting from 0.
     * @return The tag and name of the trace.
     */
    private String getChannelNameTag(int itrace) {
        String nametag = (String)m_traceNameTags.getValue(itrace);
        String rtn = (nametag == null) ? "" : nametag;
        return rtn;
    }

    /**
     * Returns the name of the specified trace.
     * The name is suitable for use as a label.
     * Returns a 0 length string if the trace number is undefined.
     * @param itrace Which trace to get, starting from 0.
     * @return The name of the trace.
     */
    public String getChannelName(int itrace) {
        String nametag = getChannelNameTag(itrace);
        String[] rtn = splitNameTag(nametag);
        Messages.postDebug("LcConfiguration",
                           "getChannelName(" + itrace + ") returns " + rtn[0]);
        return rtn[0];
    }

    /**
     * Returns the tag of the specified trace.
     * The tag is read by the software to figure out how to get the
     * data for the trace.
     * Returns a 0 length string if the trace number is undefined.
     * @param itrace Which trace to get, starting from 0.
     * @return The tag of the trace.
     */
    public String getChannelTag(int itrace) {
        String nametag = getChannelNameTag(itrace);
        String[] rtn = splitNameTag(nametag);
        Messages.postDebug("LcConfiguration",
                           "getChannelTag(" + itrace + ") returns " + rtn[1]);
        return rtn[1];
    }

    /**
     * Returns the trace number connected to the primary UV monitor
     * wavelength through the analog output (SLIM ADC).
     * @return The trace number (from 0), or -1 if there is no such trace.
     */
    public int getUvChannel() {
        int n = m_traceNameTags.getArraySize();
        for (int i = 0; i < n; i++) {
            String name = getChannelTag(i);
            if (name != null && name.startsWith(":uv1:adc")) {
                return i;
            }
        }
        return -1;            
    }

    /**
     * Get whether a given trace number is enabled.
     * This is not allowed to change during the run.
     * @param trace The trace number, starting from 0.
     * @return True if the trace is enabled.
     */
    public boolean isTraceEnabled(int trace) {
        int n = m_traceNameTags.getArraySize();
        return trace < n && !getChannelNameTag(trace).equals("none");
    }

    public int getLampSelection() {
        int rtn = ((Integer)m_lampSelection.getValue()).intValue();
        Messages.postDebug("LcConfiguration",
                           "getLampSelection() returns " + rtn);
        return rtn;
    }

    public double getMsPumpFlow() {
        double rtn = ((Double)m_msPumpFlow.getValue()).doubleValue();
        Messages.postDebug("LcConfiguration",
                           "getMsPumpFlow() returns " + rtn);
        return rtn;
    }

    /**
     * Get probe volume in mL (parameter value is in uL).
     */
    public double getNmrProbeVolume() {
        double rtn = ((Double)m_nmrProbeVolume.getValue()).doubleValue();
        rtn /= 1000;
        Messages.postDebug("LcConfiguration",
                           "getNmrProbeVolume() returns " + rtn);
        return rtn;
    }

    /**
     * Get the flow cell type.
     */
    public int getFlowCell() {
        int rtn = ((Integer)m_uvFlowCellId.getValue()).intValue();
        Messages.postDebug("LcConfiguration",
                           "getFlowCell() returns " + rtn);
        return rtn;
    }

    /**
     * Get the flow cell ratio.
     */
    public double getFlowCellRatio() {
        double rtn = ((Double)m_uvFlowCellRatio.getValue()).doubleValue();
        Messages.postDebug("LcConfiguration",
                           "getFlowCellRatio() returns " + rtn);
        return rtn;
    }

    /**
     * Get collector loop volume in mL (parameter value is in uL).
     */
    public double getLoopVolume() {
        double rtn = ((Double)m_loopVolume.getValue()).doubleValue();
        rtn /= 1000;
        Messages.postDebug("LcConfiguration",
                           "getLoopVolume() returns " + rtn);
        return rtn;
    }

    /**
     * Get the pump to loop transfer time (seconds).
     */
    public double getPumpToLoopTransferTime() {
        double rtn = ((Double)m_pumpToLoopTime.getValue()).doubleValue();
        Messages.postDebug("LcConfiguration",
                           "getPumpToLoopTransferTime() returns " + rtn);
        return rtn;
    }

    /**
     * Get the name of the solvent in the specified bottle.
     */
    public String getSolvent(int iBottle) {
        String rtn = ((String)m_lcConfigSolv[iBottle].getValue());
        Messages.postDebug("LcConfiguration",
                           "getSolvent(" + iBottle + ") returns " + rtn);
        return rtn;
    }

    /*
     * Set / Get methods used for values stored in this class:
     */

    public void setMsPumpControl(boolean b) {
        m_bMsAutoPumpControl = b;
    }

    public boolean getMsPumpControl() {
        return m_bMsAutoPumpControl;
    }

    public void setManualFlowControl(boolean b) {
        m_bManualFlowControl = b;
    }

    public boolean isManualFlowControl() {
        return m_bManualFlowControl;
    }

    public void setPeakAction(String cmd) {
        m_peakAction = cmd;
    }

    public String getPeakAction() {
        return m_peakAction;
    }

    public void setNmrDoneAction(String cmd) {
        m_nmrDoneAction = cmd;
    }

    public String getNmrDoneAction() {
        return m_nmrDoneAction;
    }

    public void setTransferTimes(double[] valArray) {
        m_transferTimes = valArray;
    }

    public double[] getTransferTimes() {
        return m_transferTimes;
    }

    public void setReferenceTime(double value) {
        m_referenceTime = value;
    }

    public double getReferenceTime() {
        return m_referenceTime;
    }

}
