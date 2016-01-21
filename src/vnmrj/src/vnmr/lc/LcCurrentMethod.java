/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.SortedSet;

import vnmr.lc.LcEvent.EventTrig;
import vnmr.lc.LcEvent.MarkType;
import vnmr.util.ButtonIF;
import vnmr.util.Fmt;
import vnmr.util.Messages;


public class LcCurrentMethod extends LcMethod {

    /** Used for communication with VnmrBG. */
    private ButtonIF vnmrIf;

    /** For real-time Time-slice changes. */
    private double m_sliceStartTime = 0;

    /** For real-time Time-slice changes. */
    private boolean m_interactTimeSlice = false;

    /**
     * Specifies event to be executed for changes of given variables.
     * Queue up a clone of the event from the table -- otherwise events
     * cannot be customized (e.g., set to different times).
     */
    private static Map<String, LcEvent> sm_eventMap = null;


    /**
     * Initialize static variable sm_eventMap.
     */
    static {
        sm_eventMap = new HashMap<String, LcEvent>(32);

        sm_eventMap.put("lcThresholdA",
                        new LcEvent("lcThreshold[1]=$VALUE", MarkType.NONE));
        sm_eventMap.put("lcThresholdB",
                        new LcEvent("lcThreshold[2]=$VALUE", MarkType.NONE));
        sm_eventMap.put("lcThresholdC",
                        new LcEvent("lcThreshold[3]=$VALUE", MarkType.NONE));
        sm_eventMap.put("lcThresholdD",
                        new LcEvent("lcThreshold[4]=$VALUE", MarkType.NONE));
        sm_eventMap.put("lcThresholdE",
                        new LcEvent("lcThreshold[5]=$VALUE", MarkType.NONE));

        sm_eventMap.put("lcPeakDetectA",
                        new LcEvent("lcPeakDetect[1]=$VALUE", MarkType.NONE));
        sm_eventMap.put("lcPeakDetectB",
                        new LcEvent("lcPeakDetect[2]=$VALUE", MarkType.NONE));
        sm_eventMap.put("lcPeakDetectC",
                        new LcEvent("lcPeakDetect[3]=$VALUE", MarkType.NONE));
        sm_eventMap.put("lcPeakDetectD",
                        new LcEvent("lcPeakDetect[4]=$VALUE", MarkType.NONE));
        sm_eventMap.put("lcPeakDetectE",
                        new LcEvent("lcPeakDetect[5]=$VALUE", MarkType.NONE));

        sm_eventMap.put("lcNmrBypass",
                        new LcEvent("lccmd('setNmrBypass $VALUE')",
                                    MarkType.COMMAND));

        sm_eventMap.put("msScans",
                        new LcEvent("msSetMethod('save','$VALUE')",
                                    MarkType.COMMAND));

        sm_eventMap.put("msPump",
                        new LcEvent("msPump('$VALUE')", MarkType.COMMAND));

        sm_eventMap.put("msValve",
                        new LcEvent("msValve('$VALUE')", MarkType.COMMAND));

        sm_eventMap.put("lcTimeSlice",
                        new LcEvent("timeSliceNmr('$VALUE', $TIME, $PEAK)",
                                    MarkType.SLICE));

        // This is put in only so it shows up on the chromatogram;
        // The mark may be changed, depending on $VALUE
        sm_eventMap.put("lcRelay1",
                        new LcEvent("\"Inject $VALUE\"", MarkType.INJECT_ON));
    }

    /**
     * Construct a method from a given parameter file.
     * @param vif The interface to VnmrBG.
     * @param methodFile The name (or path?) of the method parameter file.
     */
    public LcCurrentMethod(ButtonIF vif,
                           LcConfiguration config, String methodFile) {
        super(config, methodFile);
        vnmrIf = vif;
        Messages.postDebug("lccmdLoadMethod", "LcCurrentMethod.<init>: "
                + "\"LcMethod\" constructor finished");    

        // Load method into Vnmr parameters
        String name = getName();
        if (name == null) {
            Messages.postDebug("LcCurrentMethod.<init>: Method failed to load");
        } else {
            String cmd = "lcMethod('read', '" + name + "')";
            Messages.postDebug("LcCurrentMethod", "LcCurrentMethod.<init>: "
                               + "sendToVnmr: \"" + cmd + "\"");
            vnmrIf.sendToVnmr(cmd);
        }
    }

    /**
     * Sets the current value of the lcTrace parameter in the current method
     * in Vnmrbg.
     * These are, in general, different from the values in the method as
     * loaded, because of adjustments for current detector connections.
     */
    public void sendTraceStringsToVnmr() {
        LcMethodParameter traceParam = getParameter("lcTrace");
        String[] traceStrings = traceParam.getAllValueStrings();
        StringBuffer sb = new StringBuffer("lcTrace=");
        for (int i = 0; i < traceStrings.length; i++) {
            if (i > 0) {
                sb.append(",");
            }
            sb.append("'").append(traceStrings[i]).append("'");
        }
        vnmrIf.sendToVnmr(sb.toString());
    }

    /**
     * Set the lcPeakDetect[1:n] parameter to follow change in peak
     * detect method variable. The lcPeakDetect[i] value is set to 'y'
     * or 'n', depending on whether peak detection is enabled at the
     * current time.  If no run is in progress, checks for peak
     * detection at the start of the run.  The trace number is given
     * by the name of the method parameter that controls peak detection
     * for the trace: lcPeakDetectX.
     * @deprecated
     * @param parm The parameter name for the trace to set.
     * @param run The LcRun this refers to.
     */
    // TODO: This always sets lcPeakDetectX to a single, fixed value.
    // Should not be used.
    public void setPeakEnabled(String parm, LcRun run) {
        // The last letter of the parameter name gives the trace number
        Messages.postDebug("Deprecated method LcCurrentMethod.setPeakEnabled("
                           + parm + ", LcRun) has been called");
        int len = parm.length();
        char chrChan = parm.charAt(len - 1);
        int chan = chrChan - 'A' + 1; // Index runs from 1
        if (run == null || !run.isActive()) {
            // Set to value at t=0
            String yn = isPeakEnabled(chan, 0) ? "'y'" : "'n'";
            vnmrIf.sendToVnmr(parm + "=" + yn);
        } else {
            String yn = isPeakEnabled(chan, run.getFlowTime()) ? "'y'" : "'n'";
            vnmrIf.sendToVnmr(parm + "=" + yn);
        }
    }

    /**
     * Change value of peak detect method variable to follow change
     * in the peak detection, and set the parameter
     * lcPeakDetect[i] (which tracks the instantaneous value of
     * peak detection on channel i).  If called during a run, adds a new row
     * to the run method to reflect the change.
     * @param chan The trace number, starting from 0.
     * @param value Should be 'y' or 'n'.
     * @param run The LcRun this refers to.
     */
    public void setPeakEnabled(int chan, String value, LcRun run) {
        char chanLetter = (char)('A' + chan);
        String parm = "lcPeakDetect" + chanLetter;
        if (run != null && run.isActive()) {
            setValue(parm, value, run.getFlowTime());
        }
        Messages.postDebug("LcMethod",
                           "To VBG: lcPeakDetect[" + (chan + 1) + "]='"
                           + value + "'");
        vnmrIf.sendToVnmr("lcPeakDetect[" + (chan + 1) + "]='" + value + "'");
    }

    public void setThreshold(int chan, double value, LcRun run) {
        String sValue = Fmt.f(2, value, false);
        char chanLetter = (char)('A' + chan);
        String threshLabel = "lcThreshold" + chanLetter;
        if (run != null && run.isActive()) {
            setValue(threshLabel, sValue, run.getFlowTime());
        }
        Messages.postDebug("LcMethod",
                           "To VBG: lcThreshold[" + (chan + 1) + "]=" + value);
        vnmrIf.sendToVnmr("lcThreshold[" + (chan + 1) + "]=" + value);
    }

    /**
     * Sets the name of the MS Method in the current LC Method.
     * Does not download the new MS Method to the MS.
     */
    public void setMsMethod(String methodName, LcRun run) {
        if (run != null && run.isActive()) {
            setValue("msScans", methodName, run.getFlowTime());
        }
    }

    /**
     * Interactive start of time slice during a run.
     * Time-slice will continue until interactively stopped.
     */
    public void initTimeSlice(LcRun run) {
        m_sliceStartTime = run.getNmrFlowTime();
        if (run == null || isTimeSlice(m_sliceStartTime)) {
            return;
        }
        m_interactTimeSlice = true;
        String parm = "lcTimeSlice";
        run.insertTimeSlice(m_sliceStartTime, EventTrig.USER_ACTION);
        run.checkEvents((int)(m_sliceStartTime * 60000));
        setValue(parm, "y", m_sliceStartTime); // Insert slice-on line in method
    }

    /**
     * Interactively stop time-slice during a run.
     */
    public void stopTimeSlice(LcRun run) {
        double flowTime = run.getNmrFlowTime();
        if (run == null || !isTimeSlice(flowTime)) {
            return;
        }
        m_interactTimeSlice = false;
        String parm = "lcTimeSlice";
        if (run == null) {
            setValue(parm, "n");
        } else {
            if (m_sliceStartTime > 0) {
                undisableTimeSlice(m_sliceStartTime, flowTime);
            }
            m_sliceStartTime = 0;
            // Don't let time fall on point we're currently (maybe) stopped on
            setValue(parm, "n", flowTime + 0.001); // Put turn-off in method
            run.removeNextTimeSlice();
        }
    }

    /**
     * Make sure all method rows between when slicing started and the
     * given time have time-slice enabled.  Updates the slice start time.
     */
    public void undisableTimeSlice(double t) {
        if (m_interactTimeSlice && m_sliceStartTime > 0) {
            undisableTimeSlice(m_sliceStartTime, t);
            m_sliceStartTime = t;
        }
    }

    /**
     * Make sure all method rows between given times have time-slice
     * enabled.
     */
    public void undisableTimeSlice(double t0, double t1) {
        int nRows = getRowCount();
        for (int i = 0; i < nRows; i++) {
            double time = getTime(i);
            if (time > t1) {
                break;
            } else if (time >= t0) {
                LcMethodParameter par = getParameter("lcTimeSlice");
                par.setValue(i, Boolean.TRUE);
                //setValue("lcTimeSlice", i, Boolean.TRUE);
            }
        }
    }

    public void preloadEventQueue(SortedSet<LcEvent> queue) {
        m_sliceStartTime = 0;
        preloadEventQueue(queue, 0);
    }

    /**
     * Put events defined in the run into the given queue.
     * Provision is made to offset the times assigned in the queue;
     * this is used to glue separate elution methods into a single run.
     * @param queue The queue to be loaded.
     * @param offset This time is added to the method time to get the
     * event time (minutes).
     */
    public void preloadEventQueue(SortedSet<LcEvent> queue, double offset) {
        double[] rowTimes = getRowTimes(offset);
        loadExplicitCommands(queue, rowTimes);
        loadMethodEvents(queue, rowTimes);
    }

    /**
     * Get an array of retention times representing the time for each row
     * in the method.
     * @param offset This time is added to the method time to get the
     * event time (minutes).
     * @return An array of times (minutes).
     */
    private double[] getRowTimes(double offset) {
        int nRows = getRowCount();
        double[] times = new double[nRows];
        for (int i = 0; i < nRows; i++) {
            times[i] = getTime(i) + offset;
        }
        return times;
    }

    /**
     * Put commands explicitly specified in the run into the given queue.
     * These are the commands that appear in the "Timed Commands" column
     * of the method table.
     * @param queue The queue to be loaded.
     * @param time The (offset) retention time of each row (minutes).
     */
    private void loadExplicitCommands(SortedSet<LcEvent> queue, double[] time) {
        LcMethodParameter par = getParameter("lcCommand");

        // Insert an event for each lcCommand in the method
        int nRows = getRowCount();
        for (int i = 0; i < nRows; i++) {
            String cmd = par.getStringValue(i);
            if (cmd != null && cmd.length() > 0 && !cmd.equals("null")) {
                if (time[i] == 0) {
                    vnmrIf.sendToVnmr(cmd);
                } else {
                    LcEvent lce = new LcEvent((long)(time[i] * 60000)
                                              , 0, 0, 0,
                                              EventTrig.METHOD_TABLE,
                                              MarkType.COMMAND,
                                              cmd);
                    if (cmd.startsWith("timedElution")) {
                        lce.markType = MarkType.ELUTION;
                    }
                    queue.add(lce);
                }
            }
        }
    }

    /**
     * Put implicit events in the given row of the method into the
     * given queue.
     * The "lcCommand" events are ignored by this method.
     * @param queue The queue to be loaded.
     * @param time An array giving the (offset) retention time of each row
     * (minutes).
     */
    private void loadMethodEvents(SortedSet<LcEvent> queue, double[] time) {
        // Add events for method table columns that have entries in sm_eventMap
        Collection<LcMethodParameter> pars = getTabledParameters();
        pars.add(getParameter("lcTimeSlice")); // Always put in time slice
        for (LcMethodParameter par : pars) {
            LcEvent lce = sm_eventMap.get(par.getName());
            if (lce != null) {
                loadMethodEvents(queue, time, par, lce);
            }
        }
    }

    /**
     * Put implicit events for a given parameter into the given queue.
     * The "lcCommand" events are ignored by this method.
     * @param queue The queue to be loaded.
     * @param time An array giving the (offset) retention time of each row
     * (minutes).
     * @param par The parameter (table column) we are dealing with.
     * @param event A prototype of the LcEvent to put in for this parameter.
     */
    private void loadMethodEvents(SortedSet<LcEvent> queue, double[] time,
                                  LcMethodParameter par, LcEvent event) {

        // An event for each change in a method value
        String prevVal = par.getStringValue(0);
        
        // Special case: check for TimeSlice on in first row
        if ("lcTimeSlice".equals(par.getName()) && "y".equals(prevVal)) {
            insertEvent(queue, 0, par, event, "y");
        }
        int nRows = getRowCount();
        // Start at second row; row 0 values are set before run starts
        for (int i = 1; i < nRows; i++) {
            String val = par.getStringValue(i);
            if (val != null && !"null".equals(val) && !val.equals(prevVal)) {
                insertEvent(queue, time[i], par, event, val);
                prevVal = val;
            }
        }
    }

    private void insertEvent(SortedSet<LcEvent> queue, double time,
                             LcMethodParameter par, LcEvent event, String val) {
        // Put in a _copy_ of the prototype event, as we need
        // to adjust it's parameters.
        LcEvent e = (LcEvent)event.clone();

        String cmd = e.action;
        e.time_ms = (long)(time * 60000);
        cmd = cmd.replaceAll("\\$VALUE", val);
        String strTime = Double.toString(time);
        cmd = cmd.replaceAll("\\$TIME", strTime);
        e.action = cmd;

        if (cmd.startsWith("timeSlice") && !val.equals("y")) {
            e.markType = MarkType.SLICEOFF;
        } else if (par.getName().startsWith("lcRelay")
                   && !val.equals("y"))
        {
            e.markType = MarkType.INJECT_OFF;
        }
        e.trigger = EventTrig.METHOD_TABLE;
        queue.add(e);
    }
}
