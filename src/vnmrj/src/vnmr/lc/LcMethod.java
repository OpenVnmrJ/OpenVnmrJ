/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;


import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.StringTokenizer;
import java.util.TreeSet;
import java.util.regex.Pattern;

import vnmr.lc.jumbuck.JbControl;
import vnmr.util.DebugOutput;
import vnmr.util.FileUtil;
import vnmr.util.Fmt;
import vnmr.util.Messages;
import vnmr.util.QuotedStringTokenizer;
import vnmr.util.Util;


/**
 * This class holds an LC Run Method, which defines all the actions
 * taken during an LC Run.
 * LcMethod provides:
 * <ul>
 * <li> Storage of all parameter values used in the LC method
 * <li> Methods to "get" values of parameters.
 * <li> Methods to "set" values of all parameters that can be altered
 * during a run
 * <li> Methods to read (write) LC method from (to) parameter files
 * <li> Method to write LC method to an HTML file
 * </ul>
 * The LcMethod class does <i>not</i> include a GUI editor and is not
 * coupled with Vnmr method parameters.
 */
public class LcMethod {

    /** Unicode character used for dumping method in HTML. */
    private static final String LAMBDA = "\u03BB"; // Unicode for Greek Lambda

    /** Unicode character used for dumping method in HTML. */
    private static final String MU = "\u03BC"; // Unicode for Greek Mu

    /** Regexp for decimal digits. */
    private static final String Digits = "(\\p{Digit}+)";

    /** Regexp for decimal exponent. */
    private static final String Exp = "[eE][+-]?"+Digits;

    /**
     * Matches a solvent name in the LC Method parameter file.
     */
    private static final Pattern SOLVENT_PARAMETER_PATTERN
        = Pattern.compile("^solv[A-C]$");

    private static final Pattern NUMBER_PATTERN
        = Pattern.compile("[+-]?(("+Digits+"(\\.)?("+Digits+"?)("+Exp+")?)"
                          + "|(\\.("+Digits+")("+Exp+")?))");


    /** The method parameter that holds the number of rows in the table. */
    private LcMethodParameter m_rowCountParameter = null;

    /** The method parameter that holds the list of retention times. */
    private LcMethodParameter m_timeParameter = null;

    /**
     * The list of solvent name parameters; like {"solvA", "solvB", "solvC"}.
     */
    private String[] m_solventNames = null;

    /**
     * The list of solvent concentration parameters; like {"lcPercentA",
     * "lcPercentB", "lcPercentC"}.
     */
    private String[] m_percentNames = null;

    /** The list of solvent bottles available; like {"A", "B", "C"}. */
    private String[] m_bottles = null;

    /**
     * Hold the method parameters keyed by name.
     */
    private Map<String, LcMethodParameter> m_methodParameters = null;

    /**
     * A list of all the parameters that can go in the LC Method Table,
     * in the order the columns should appear in the table.
     */
    private ArrayList<String> m_eventOrderList = null;

    /**
     * The method name.
     */
    private String m_methodName = null;

    /**
     * The method filepath.
     */
    private String m_methodFilepath = null;

    /**
     * The class for getting the instrument configuration.
     */
    private LcConfiguration m_configuration = null;


    /**
     * Construct a method from a given parameter file.
     * @param methodFile The name (or path) of the method parameter file.
     */
    public LcMethod(LcConfiguration config, String methodFile) {
        m_configuration = config;

        // Build list of empty method parameter holders
        // Sets m_methodParameters and m_eventOrderList:
        Messages.postDebug("LcMethod", "LcMethod.<init>: calling "
                           + "buildParameterTable(\"lc/lcMethodVariables\")");
        buildParameterTable("lc/lcMethodVariables");

        // Initialize method from methodFile
        Messages.postDebug("LcMethod", "LcMethod.<init>: calling "
                           + "resolveMethodReadPath(\"" + methodFile + "\")");
        String filepath = resolveMethodReadPath(methodFile);
        if (filepath == null) {
            Messages.postError("LcMethod.<init>: method \"" + methodFile
                               + "\" not found");
        } else {
            readMethodFromFile(filepath);

            // Debug print of all method variables
            if (DebugOutput.isSetFor("LcMethod")) {
                dumpParameters("Parameter values read from " + filepath + ":");
            }
        }
        Messages.postDebug("LcMethod", "LcMethod.<init>: done");
    }

    public LcConfiguration getConfiguration() {
        return m_configuration;
    }

    /**
     * Dump all the parameter values to the debug log.
     */
    private void dumpParameters(String heading) {
        StringBuffer text = new StringBuffer(heading).append("\n");
        Collection<LcMethodParameter> params
                = m_methodParameters.values();
        String parnames[] = new String[params.size()];
        int idx = 0;
        for (LcMethodParameter param : params) {
            if (param == null) {
                parnames[idx] = "null";
            } else {
                parnames[idx] = param.getName();
            }
            idx++;
        }
        Arrays.sort(parnames);
        for (String parname : parnames) {
            if (parname == "null") {
                text.append("  NULL parameter\n");
            } else {
                text.append("  " + parname + ":\n");
                LcMethodParameter param = getParameter(parname);
                String[] vals = param.getAllValueStrings();
                for (String val : vals) {
                    text.append("    " + val + "\n");
                }
            }
        }
        Messages.postDebug(text.toString());
    }

    /**
     * Get the name of the method.
     * This is the name of the file it came from.
     */
    public String getName() {
        return m_methodName;
    }

    /**
     * Set the name of the method.
     * This is the name of the file it came from.
     * @param name The name to set.
     * @return The same String that was passed in as "name".
     */
    public String setName(String name) {
        m_methodName = name;
        return name;
    }

    /**
     * Get the absolute pathname of the method file.
     */
    public String getPath() {
        return m_methodFilepath;
    }

    /**
     * Set the absolute pathname of the method.
     * @param name The filepath to set.
     */
    public void setPath(String path) {
        m_methodFilepath = path;
        return;
    }

    /**
     * Get the parameter that holds the number of rows in the method.
     */
    protected LcMethodParameter getRowCountParameter() {
        if (m_rowCountParameter == null) {
            m_rowCountParameter = getParameter("lcMethodRows");
        }
        return m_rowCountParameter;
    }

    /**
     * Get the parameter that holds the retention time.
     */
    protected LcMethodParameter getTimeParameter() {
        if (m_timeParameter == null) {
            m_timeParameter = getParameter("lcTime");
        }
        return m_timeParameter;
    }

    /**
     * Get a parameter by name.
     */
    protected LcMethodParameter getParameter(String name) {
        int idx;
        if (name != null && (idx = name.indexOf("[")) >= 0) {
            name = name.substring(0, idx);
        }
        return m_methodParameters.get(name);
    }

    /**
     * Get the retention time for a given table row.
     * @param iRow The row index (from 0).
     * @return The retention time for the row.
     */
    public double getTime(int iRow) {
        LcMethodParameter par = getTimeParameter();
        Double rtn = null;
        for (int i = iRow; rtn == null && i >= 0; --i) {
            rtn = (Double)par.getValue(i);
        }
        return rtn == null ? 0 : rtn;
    }

    /**
     * Construct a table of method parameters from a parameter file.
     * All the parameters will be uninitialized; they only know their
     * properties: name, type, arrayability.
     * This fills the map m_methodParameters and the list m_eventOrderList.
     * @param filename The text file specifying the parameters.
     */
    private void buildParameterTable(String filename) {
        m_methodParameters = null;
        m_eventOrderList = null;
        
        String filepath = FileUtil.openPath(filename);
        if (filepath == null) {
            Messages.postError("LcMethod.buildParameterTable: file not found: "
                               + filename);
        }
        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(filepath));
            m_methodParameters = new HashMap<String, LcMethodParameter>(100);
            m_eventOrderList = new ArrayList<String>();

            String line;
            while ((line = in.readLine()) != null) {
                line = line.trim();
                if (line.length() > 0 && !line.startsWith("#")) {
                    // Process non-blank, non-comment line
                    String name = null;
                    String type = LcMethodParameter.BOOLEAN_TYPE;
                    boolean isArrayable = true;
                    boolean isEvent = false; // Is it in table when arrayed
                    String label = null;
                    try {
                        StringTokenizer toker = new StringTokenizer(line);
                        name = toker.nextToken();
                        label = name; // Default label
                        type = toker.nextToken();
                        isArrayable = toker.nextToken().equals("y");
                        isEvent = toker.nextToken().equals("y");
                        label = Util.getLabel(toker.nextToken("").trim());
                    } catch (NoSuchElementException nsee) {
                    }
                    if (name != null) {
                        LcMethodParameter par;
                        par = new LcMethodParameter(name, type,
                                                    isEvent, isArrayable,
                                                    label);
                        m_methodParameters.put(name, par);
                        if (isEvent) {
                            m_eventOrderList.add(name);
                            par.setColumnPosition(m_eventOrderList.size());
                        }
                    }
                }
            }
        } catch (IOException ioe) {
            Messages.postError("LcMethod.buildParameterTable: "
                               + "Error reading file: " + filepath);
        }
        try { in.close(); } catch (Exception e) {}
    }

    /**
     * Returns the names of parameters currently in the Method Table,
     * in the order that they should appear in the table.
     */
    public List<String> getTabledParameterNames() {
        if (m_eventOrderList == null) {
            return null;
        }

        List<String> list = new ArrayList<String>();
        for (String name : m_eventOrderList) {
            if (getParameter(name).isTabled()) {
                list.add(name);
            }
        }
        return list;
    }

    /**
     * Get the parameters that are currently in the table.
     */
    protected Collection<LcMethodParameter> getTabledParameters() {
        Collection<LcMethodParameter> rtn = new ArrayList<LcMethodParameter>();
        for (LcMethodParameter par: m_methodParameters.values()) {
            if (par.isTabled()) {
                rtn.add(par);
            }
        }
        return rtn;
    }

    /**
     * Set the number of rows in the method table.
     */
    public void setRowCount(int nRows) {
        LcMethodParameter par = getRowCountParameter();
        par.setValue(0, (Integer)nRows);
    }

    /**
     * Set the value of a given method variable to a given fixed value.
     * This means that if the variable is currently in the table, it
     * is removed from the table.
     * @param name The name of the method variable.
     * @param value The value to set it to.  For boolean values, use
     * "y" or "n".
     */
    public void setValue(String name, String value) {
        if (value != null) {
            LcMethodParameter parm = getParameter(name);
            if (parm != null) {
                parm.clear();
                parm.setValue(0, (Object)value);
            } else {
                Messages.postDebug("LcMethod.setValue: "
                                   + "Parameter not found: \"" + name + "\"");
            }
        }
    }

    /**
     * Set the value of a given method variable to a given value at
     * a given time in the method table. A convenience interface to
     * setValue(String, String, double).
     # @see #setValue(String, String, double)
     * @param name The name of the method variable.
     * @param value The value to set it to, as a string.  For boolean values,
     * use "y" or "n".
     * @param time The retention time in minutes as a (floating point) string.
     */
    public void setValue(String name, String value, String time) {
        if (name == null || time == null) {
            return;
        }
        double t = 0;
        try {
            t = Double.parseDouble(time);
        } catch (NumberFormatException nfe) {}
        setValue(name, value, t);
    }

    /**
     * Set the value of a given method variable to a given value at
     * a given time in the method table.
     * If the variable is not
     * currently in the table, it will be added to the table, with
     * an initial value (at t=0) of whatever it had as a fixed value. TODO: Does this work?
     * If the time <i>exactly</i> matches a time already in the table,
     * the first row with that time will get the specified value,
     * and any additional rows with the same time will not change it. TODO: Does this work?
     * Otherwise, a row is added in the appropriate place in the table
     * with the requested time.
     * @param name The name of the method variable.
     * @param value The value to set it to, as a string.
     * For boolean values, use "y" or "n".
     * @param time The retention time in minutes.
     * @return True if a value is set.
     */
    public boolean setValue(String name, String value, double time) {
        boolean rtn = false;
        LcMethodParameter par = getParameter(name);
        if (par != null && time >= 0) {
            int iRow = getRowIndex(time);
            if (getTime(iRow) != time) {
                // This exact time is not in the table yet
                iRow = addRow(time);
            }
            /*Messages.postDebug("schedule.setValue(): setValueAt(" + value
              + ", " + iRow + ", " + iCol + ")");/*CMP*/
            par.setValue(iRow, value);
            rtn = true;
            /*
            if (value instanceof Boolean) {
                // For Boolean values fill remainder of column with same value
                int n = getRowCount();
                for (++iRow; iRow < n; ++iRow) {
                    setValueAt(value, iRow, iCol);
                }
            }
            */
        }
        return rtn;
    }

    /**
     * Add a row at a given time position in the table.
     * Sets the given time into the "time" column.
     * If there is already a row in the table with the given time,
     * does nothing.
     * @return The row index of the row with exactly the given time
     * (starting from 0).
     */
    protected int addRow(double time) {
        int row = getRowIndex(time);
        if (getTime(row) != time) {
            // Add a new row for this time to each tabled parameter
            row++;
            /*Messages.postDebug("schedule.addRow(): nrows=" + getRowCount()
                               + ", new row=" + row);/*CMP*/
            for (LcMethodParameter par: m_methodParameters.values()) {
                Object val = (par == getTimeParameter()) ? (Double)time : null;
                if (par.isTabled()) {
                    par.insertValue(row, val);
                }
            }
            setRowCount(getRowCount() + 1);
        }
        return row;
    }

    /**
     * Get the number of rows in the method table.
     */
    public int getRowCount() {
        LcMethodParameter par = getRowCountParameter();
        int rtn = 0;
        if (par != null) {
            Object value = par.getValue();
            if (value != null) {
                rtn = (Integer)value;
            }
        }
        return rtn;
    }

    /**
     * Returns the row number that applies to a given time.
     * This is the last row that has a time less than or equal to
     * the given time.
     * If given time is outside the range of the method, returns
     * the first or last row.
     * @param time The retention time in minutes.
     */
    synchronized public int getRowIndex(double time) {
        // This trivial implementation should be fast enough
        int nRows = getRowCount();
        for (int i = 1; i < nRows; i++) {
            if (getTime(i) > time) {
                return i - 1;
            }
        }
        return nRows - 1;
    }

    /**
     * Returns true if "name" is the retention-time parameter.
     */
    public boolean isTimeParameter(String name) {
        return name != null && name.equals("lcTime");
    }

    /**
     * Get whether auto peak detection is enabled for a given trace
     * at a given retention time.
     * @param chan The trace number, starting from 0.
     * @param time The retention time in minutes.
     * @return True is peak detection is enabled.
     */
    public boolean isPeakEnabled(int chan, double time) {
        char chanLetter = (char)('A' + chan);
        String parm = "lcPeakDetect" + chanLetter;
        return ((Boolean)getValue(parm, time)).booleanValue();
    }

    /**
     * Return the value of the peak detect threshold for a given trace
     * number at a given run time.
     * @param chan Trace number, starting from 0.
     * @param time Run time in minutes.
     * @return The threshold level.
     */
    public double getThreshold(int chan, double time) {
        if (chan < 0) {
            return -1;
        }
        char chanLetter = (char)('A' + chan);
        String threshLabel = "lcThreshold" + chanLetter;
        double thresh = 0;
        try {
            thresh = ((Double)getValue(threshLabel, time)).doubleValue();
        } catch (NullPointerException npe) {
            Messages.postDebug("Failed to get threshold for " + threshLabel);
            Messages.writeStackTrace(new Exception("DEBUG"));
        }
        return thresh;
    }

    public double getAdcPeriod() {
        return ((Double)getValue("lcAdcPeriod", 0)).doubleValue();
    }

    public double getHoldTime(double time) {
        return ((Double)getValue("lcHoldTime", time)).doubleValue();
    }

    /**
     * Get the stop "hold type" for the given retention time.
     * @param time The time in minutes.
     * @return The lcHoldType value.
     */
    public String getHoldType(double time) {
        return (String)getValue("lcHoldType", time);
    }

    /**
     * Gets the period between time slices at the given run time.
     * @param time The run time (minutes).
     * @return The period between time-slice stops (seconds).
     */
    public double getTimeSlicePeriod(double time) {
        double period;
        Object oPeriod = getValue("lcTimeSlicePeriod", time);
        if (oPeriod instanceof Integer) {
            period = (double)((Integer)oPeriod).intValue();
        } else {
            period = ((Double)oPeriod).doubleValue();
        }
        return period;
    }

    /**
     * How long to do time slices (minutes)
     */
    public double getTimeSliceDuration(double time) {
        //return ((Double)getValue("lcTimeSliceDuration", time)).doubleValue();
        return 1;
    }

    /**
     * Is time slicing enabled?
     */
    public boolean isTimeSlice(double time) {
        boolean rtn = ((Boolean)getValue("lcTimeSlice", time)).booleanValue();
        return rtn;
    }

    /**
     * Use linear interpolation to get a value of a parameter at a given time.
     * If extrapolation is required, uses the closest defined value.
     */
    public Double getInterpolatedValue(String name, double time) {
        LcMethodParameter par = getParameter(name);
        if (par == null) {
            Messages.postDebug("LcMethod.getInterpolatedValue: "
                               + "Parameter is not defined: " + name);
            return null;
        }

        int row = getRowIndex(time);
        Double value = (Double)par.getValue(row);
        if (value == null) {
            // Need to extrapolate or interpolate.

            // Find first previous specified value
            int row0 = row;
            Double value0 = null;
            while (value0 == null && row0 > 0) {
                value0 = (Double)par.getValue(--row0);
            }

            // Find first successor specified value
            int nRows = getRowCount();
            int row1 = row;
            Double value1 = null;
            while (value1 == null && row1 < nRows - 1) {
                value1 = (Double)par.getValue(++row1);
            }

            if (value0 != null && value1 != null) {
                // Interpolate
                double t0 = getTime(row0);
                double t1 = getTime(row1);
                value = value0 + (time - t0) * (value1 - value0) / (t1 - t0);
            } else if (value0 != null) {
                // Use previous value
                value = value0;
            } else if (value1 != null) {
                // Use later value
                value = value1;
            } else {
                Messages.postDebug("LcMethod.getInterpolatedValue: "
                                   + "No value defined for " + name);
            }
        }
        return value;
    }

    /**
     * Return the percents for a given row, or null if no percents
     * are specified in that row.
     */
    public double[] getPercents(int iRow) {
        String[] params = getPercentNames();
        boolean ok = false;
        for (int i = 0; i < params.length; i++) {
            ok |= (getExplicitValueAt(params[i], iRow) != null);
        }
        return ok ? getPercents(getTime(iRow)) : null;
    }

    /**
     * Return the initial percent concentrations.
     */
    public double[] getInitialPercents() {
        return getPercents(0.0); // Percents at t=0
    }

    /**
     * Return the percents for a given time. Linearly interpolates
     * between defined percents in table.
     */
    public double[] getPercents(double time) {
        String[] params = getPercentNames();
        int len = params.length;
        double[] pct = new double[len];
        for (int i = 0; i < len; i++) {
            pct[i] = getInterpolatedValue(params[i], time);
        }
        return pct;
    }

    public double getInitialFlow() {
        return getFlow(0);
    }

    /**
     * Returns the Flow Rate in mL / minute.
     */
    public double getFlow(int iRow) {
        double time = getTime(iRow);
        return ((Double)getValue("lcFlowRate", time)).doubleValue();
    }

    public String getInitialMsMethod() {
        return getMsMethod(0);
    }

    public String getMsMethod(int iRow) {
        String method = null;
        double time = getTime(iRow);
        return (String)getValue("msScans", time);
    }

    public int getInitialRelays() {
        return getRelays(0);
    }

    /**
     * Gets the values of all the relays at a given time as a bitmapped int.
     * The specified row is a proxy for the time on that row.
     * The low 6 bits of the return value contain the states of the 6 relays.
     * E.g., the LSB is 0 or 1 depending on whether relay 1 is off or on,
     * respectively.
     * @param iRow The row to get the values for.
     * @return A bitmapped int containing the relay values.
     */
    public int getRelays(int iRow) {
        int relays = 0;
        double time = getTime(iRow);
        if (((Boolean)getValue("lcRelay1", time)).booleanValue()) {
            relays |= 0x1;
        } else {
            // NB: Relay 2 is always the opposite of Relay 1, so it doesn't
            // have it's own parameter.
            // They are used to control the injector valve.
            relays |= 0x2;
        }
        if (((Boolean)getValue("lcRelay3", time)).booleanValue()) {
            relays |= 0x4;
        }
        if (((Boolean)getValue("lcRelay4", time)).booleanValue()) {
            relays |= 0x8;
        }
        if (((Boolean)getValue("lcRelay5", time)).booleanValue()) {
            relays |= 0x10;
        }
        if (((Boolean)getValue("lcRelay6", time)).booleanValue()) {
            relays |= 0x20;
        }
        return relays;
    }

    public int getUvDataPeriod_ms() {
        int period = 800;
        try{
            period = Integer.parseInt(getValueString("lcPdaPeriod_ms"));
        } catch (NumberFormatException nfe) {
             Messages.postError("Number Format Error: "
                                + "Cannot get UV data rate (lcPdaPeriod_ms=\""
                                + getValueString("lcPdaPeriod_ms") + "\")"
                                + " -- using " + period + " ms");
        }
        return period;
    }

    /**
     * Get the first monitor wavelength.
     */
    public int getLambda() {
        return getLambda(1);
    }

    /**
     * Get the second monitor wavelength.
     */
    public int getLambda2() {
        return getLambda(2);
    }

    /**
     * Get the n'th monitor wavelength (first wavelength is numbered 1).
     * @param n Which wavelength.
     * @return The wavelength to monitor.
     */
    public int getLambda(int n) {
        // Look for an lcTrace[i] specifying ":uvN:"
        // Wavelength is in the corresponding lcTraceLambda[i]
        String[] traces = getAllValueStrings("lcTrace");
        String[] lambdas = getAllValueStrings("lcTraceLambda");
        String key = ":uv" + n + ":";
        int rtn = getLambdaMin();
        for (int i = 0; i < traces.length; i++) {
            if (traces[i].indexOf(key) >= 0) {
                try {
                    rtn = Integer.parseInt(lambdas[i]);
                    break;
                } catch (NumberFormatException nfe) {
                    Messages.postError("Number Format Error: "
                                       + "Wavelength # " + n + " string is \""
                                       + lambdas[i] + "\"");
                }
            }
        }
        return rtn;
    }

     /**
     * Get the minimum wavelength for the PDA.
     */
    public int getLambdaMin() {
        int lambdaMin = 200;
        try{
            lambdaMin = Integer.parseInt(getValueString("lcLambdaMin"));
        } catch (NumberFormatException nfe) {
               Messages.postError("Number Format Error: "
                               + "Cannot get LambdaMin.");
        }

        return lambdaMin;
    }

     /**
     * Get the maximum wavelength for the PDA.
     */
    public int getLambdaMax() {
        int lambdaMax = 800;
      
        try{
            lambdaMax = Integer.parseInt(getValueString("lcLambdaMax"));
        } catch (NumberFormatException nfe) {
        }
        return lambdaMax;
    }

     /**
     * Get the bandwidth for the PDA.
     */
    public int getBandwidth() {
        int bandwidth = 2;
       
        try{
            bandwidth = Integer.parseInt(getValueString("lcBandwidth"));
        } catch (NumberFormatException nfe) {
        }
        return bandwidth;
    }




    
    /*/*
     * Get the (initial) enable status for the given detector.
     * @param chan The index of the detector (from 0).
     */
    /*
    public boolean isChannelActive(int chan) {
        // NB: Obsoleted by isChanEnabled()
        char chanLetter = (char)('A' + chan);
        String param = "lcEnable" + chanLetter;
        double time = 0;
        return ((Boolean)getValue(param, time)).booleanValue();
    }
    */

    /**
     * Get the flow time at the scheduled end of the run.
     * @return The retention time in minutes.
     */
    public double getEndTime() {
        // Return time of last row.
        int nrows = (Integer)getValue("lcMethodRows");
        return (Double)getValue("lcTime", nrows - 1);
    }

    public double getEquilTime() {
        return (Double)getValue("lcEquilTime");
    }

    public int getMinPressure() {
        return (int)(double)(Double)getValue("lcMinPressure");
    }

    public int getMaxPressure() {
        return (int)(double)(Double)getValue("lcMaxPressure");
    }

    public String getEndAction() {
        if (((Integer)getValue("lcEndAction")).intValue() == 0) {
            return "pumpOff";
        } else {
            return "pumpOn";
        }
    }

    public String getPostRunCommand() {
        return getValueString("lcPostRunCommand");
    }

    /**
     * Get the action to perform at a peak.
     * For now, does not depend on the time, and keyed off the type of run.
     * @param time Time in minutes -- ignored.
     * @param type The type of run (lcexp='stop', etc.).
     * @return The command.
     */
    public String getPeakAction(double time, String type) {
        String action = "";
        Messages.postDebug("LcEvent", "LcMethod.getPeakAction: type=" + type);
        if (type == null) {
            action = "";
        } else if (type.equals("scout")
                   || type.equals("iso")
                   || type.equals("stop")
                   || type.equals("enter")
                   || type.equals("enterA"))
        {
            // NB: Peak number and flow time are filled in when event happens
            action = "stopFlowNmr($PEAK, $TIME)";
        } else if (type.equals("analyte")) {
            action = "collectLoop";
        } else if (type.equals("lcalone")) {
            action = "";
        } else {
            action = "";
        }
        return action;
    }

    /**
     * Get the values of any Method Parameter as an array of Strings.
     * @param name The name of the parameter.
     * @return The values of the parameter as an array of Strings.
     */
    private String[] getAllValueStrings(String name) {
        String[] rtn = null;
        LcMethodParameter par = getParameter(name);
        if (par != null) {
            rtn = par.getAllValueStrings();
        }
        return rtn;
    }

    /**
     * Get the value of any Method Parameter as a String.
     * If the parameter is arrayed, returns the initial value.
     * @param name The name of the parameter.
     * @return The value of the parameter as a String. Could be null.
     */
    private String getValueString(String name) {
        String rtn = "";
        LcMethodParameter par = getParameter(name);
        if (par != null) {
            rtn = par.getStringValue();
        }
        return rtn;
    }

    /**
     * Get the value that a given Method Parameter has at a given time.
     * @return The value.  The run-time object type depends on the
     * variable, so the caller may need to test for different possibilities.
     */
    public Object getValue(String name, double time) {
        if (name == null) {
            return null;
        }
        if (isTimeParameter(name)) {
            return (Double)time;
        }

        // Find the row that has the value we can use
        Object value;
        int nRows = getRowCount();
        int row = getRowIndex(time); // Row at or before the given time
        int irow = row;
        // Set row to the last row before the given time that has a value:
        while ((value = getExplicitValueAt(name, row)) == null && row > 0) {
            --row;
        }
        // If no value found, find first value after given time
        // (This lets you be sloppy about where you define the initial value.)
        if (value == null) {
            row = irow;
            while ((value = getExplicitValueAt(name, row)) == null
                   && row < nRows - 1)
            {
                ++row;
            }
        }

        if (value == null) {
            // Parameter is not in table!
            Messages.postDebug("LcMethod.getValue: parameter " + name
                               + " is not defined.");
            return null;
        }

        // For concentration percents, interpolate between two values
        if (name.startsWith("lcPercent")) {
            int rowNow = row;
            double t0 = getTime(row);
            double p0 = ((Double)value).doubleValue();
            // Is it set to a different value later?
            row = rowNow + 1;
            Object value1 = null;
            while (row < nRows
                   && (value1 = getExplicitValueAt(name, row)) == null)
            {
                row++;
            }
            if (value1 != null) {
                // Linearly interpolate to find value at given time
                double t1 = getTime(row);
                if (t0 != t1) {
                    double p1 = ((Integer)value1).intValue();
                    double p = p0 + (time - t0) * (p1 - p0) / (t1 - t0);
                    Messages.postDebug("loopCollect","getValue(" + name + ", "
                                       + Fmt.f(2, time) + ")=" + Fmt.f(2, p));
                    value = new Integer((int)(p + 0.5));
                }
            }
        }
        return value;
    }

    /**
     * Get the value that a given method variable has at the first row.
     * Equivalent to "getValue(name, 0)".
     * @param name The name of the parameter.
     * @return An Object containing the value.
     */
    public Object getValue(String name) {
        return getValue(name, 0);
    }

    /**
     * Get the value that a given method variable has at a given row.
     * If there is no value at that row, gets the value for the time
     * of that row.
     * (We do this because percent concentrations are linearly
     * interpolated between times where it is defined.)
     * The run-time type of the returned Object will be Integer, Double,
     * Boolean, or String.
     * @param name The name of the parameter.
     * @param row The row number, starting from 0.
     * @return An Object containing the value.
     */
    public Object getValue(String name, int row) {
        Object rtn = getExplicitValueAt(name, row);
        if (rtn == null) {
            // No value at that row; get value for that time
            rtn = getValue(name, getTime(row));
        }
        return rtn;
    }

    /**
     * Get the explicit value that a given method variable has at a given row.
     * If there is no explicit value for that row, returns null.
     * @param name The name of the parameter.
     * @param row The row number, starting from 0.
     * @return An Object containing the value.
     */
    public Object getExplicitValueAt(String name, int row) {
        Object rtn = null;
        LcMethodParameter par = getParameter(name);
        if (par != null) {
            rtn = par.getValue(row);
        }
        return rtn;
    }

    /**
     * Get a comma separated list of all the values of a variable.
     * The String output is MAGICAL compatible.
     * Each variable is represented as a string in single quotes.
     * Any embedded single quotes are escaped with "\".
     * @param name The name of the parameter to get.
     * @return A String containing the values.
     */
    public String getAllValuesAsString(String name) {
        String rtn = null;
        LcMethodParameter par = getParameter(name);
        if (par != null) {
            String[] strs = par.getAllValueStrings();
            int len = strs.length;
            if (len == 1) {
                rtn = strs[0];
            } else if (len > 1) {
                StringBuffer sbRtn = new StringBuffer();
                for (int i = 0; i < len; i++) {
                    if (i > 0) {
                        sbRtn.append(",");
                    }
                    String val = LcMethodParameter.toString(par.getValue(i));
                    val = Util.escape(val, "\'");
                    sbRtn.append("'").append(val).append("'");
                }
                rtn = sbRtn.toString();
            }
        }
        return rtn;
    }

    /**
     * Returns the list of solvent parameter names in the method as an
     * array, like {"solvA", "solvB", "solvC"}.
     */
    private String[] getSolventBottles() {
        if (m_solventNames == null || m_solventNames.length == 0) {
            ArrayList<String> names = new ArrayList<String>();
            Set<String> keys = m_methodParameters.keySet();
            for (String name : keys) {
                if (SOLVENT_PARAMETER_PATTERN.matcher(name).matches()) {
                    names.add(name);
                }
            }
            m_solventNames = (String[])names.toArray(new String[0]);
        }
        return m_solventNames;
    }

    /**
     * Returns the list of bottle names as an array, like {"A", "B", "C"}.
     */
    public String[] getBottles() {
        if (m_bottles == null || m_bottles.length == 0) {
            String[] pars = getPercentNames();
            int len = pars.length;
            m_bottles = new String[len];
            for (int i = 0; i < len; i++) {
                m_bottles[i] = pars[i].substring(9); // Strip off "lcPercent"
            }
        }
        return m_bottles;
    }

    /**
     * Returns the list of bottle parameter names as an array, like
     * {"lcPercentA", "lcPercentB", "lcPercentC"}.
     */
    private String[] getPercentNames() {
        if (m_percentNames == null || m_percentNames.length == 0) {
            ArrayList<String> names = new ArrayList<String>();
            Set<String> keys = m_methodParameters.keySet();
            for (String name : keys) {
                if (name != null && name.startsWith("lcPercent")) {
                    names.add(name);
                }
            }
            m_percentNames = (String[])names.toArray(new String[0]);
        }
        return m_percentNames;
    }

    /**
     * Checks if a given parameter is a percent concentration in this method.
     * @param parname The name of the parameter to check.
     * @return True if it is the name of a percent parameter.
     */
    public boolean isPercentParameter(String parname) {
        String[] parnames = getPercentNames();
        for (String name : parnames) {
            if (name.equals(parname)) {
                return true;
            }
        }
        return false;
    }

    /**
     * See if a given parameter is a trace selector, "lcTrace".
     * @param parname The parameter name.
     * @return True if this is a trace selector parameter.
     */
    public boolean isTraceParameter(String parname) {
        String pattern = "lcTrace";
        return parname.equals(pattern);
    }

    /**
     * Make sure the percent concentrations in the method add to 100%
     * at all times.
     * @param parname The name of a percent that will not be changed, or null.
     */
    public void reconcilePercents(String parname) {
        // Make fixed percents add up to <= 100%
        double fixedPct = reconcileFixedPercents(parname);

        // Reconcile each row of percents in method table
        // totalPct = (fixed + tabled percents)
        double totalPct = reconcileTabledPercents(parname, fixedPct);

        // If totalPct != 100, add correction to non-ref fixed percent
        finalizeFixedPercents(totalPct, parname);
    }

    /**
     * Make the fixed "percent"s add up to <= 100%, given a "master percent"
     * whose value is not changed, except to keep it between 0 and 100%.
     * @param refName The name of the "master percent", e.g., "lcPercentA",
     * or null if there is no master percent.
     * @return The sum of all the fixed percents.
     */
    public double reconcileFixedPercents(String refName) {
        double totalFixedPct = 0;
        if (refName != null) {
            // Validate the master percent
            LcMethodParameter param = getParameter(refName);
            if (!param.isTabled()) {
                Double value = (Double)param.getValue();
                if (value == null) {
                    value = 0.0;
                }
                value = Math.min(100, Math.max(0, value));
                totalFixedPct = value;
                param.setValue(0, value);
            }
        }

        // Make all non-master fixed percents compatible with total % so far
        String[] parnames = getPercentNames();
        for (String parName : parnames) {
            if (!parName.equals(refName)) {
                LcMethodParameter param = getParameter(parName);
                if (!param.isTabled()) {
                    Double value = (Double)param.getValue();
                    if (value == null) {
                        value = 0.0;
                    }
                    value = Math.min(100 - totalFixedPct, Math.max(0, value));
                    totalFixedPct += value;
                    param.setValue(0, value);
                    param.setTabled(false);
                }
            }
        }
        return totalFixedPct;
    }

    /**
     * Make the fixed "percent"s add up to exactly 100%,
     * given a "master percent" whose value is not changed,
     * except to keep it between 0 and 100%.
     * @param totalPct The initial total percent; <= 100;
     * @param refName The name of the "master percent", e.g., "lcPercentA",
     * or null if there is no master percent.
     */
    public void finalizeFixedPercents(double totalPct, String refName) {
        if (totalPct != 100) {
            String[] parnames = getPercentNames();
            for (String parName : parnames) {
                if (!parName.equals(refName)) {
                    LcMethodParameter param = getParameter(parName);
                    if (!param.isTabled()) {
                        Double value = (Double)param.getValue();
                        if (value != null) {
                            double correction = 100 - totalPct;
                            double delta;
                            if (correction > 0) {
                                delta = Math.min(100 - value, correction);
                            } else {
                                delta = Math.max(-value, correction);
                            }
                            totalPct += delta;
                            value += delta;
                            param.setValue(0, value);
                        }
                    }
                }
            }
        }
    }


    /**
     * Try to make the "percent"s add up to 100% at all times, given a
     * "master column" whose percents are not changed, if possible,
     * except to keep them between 0% and (100% - fixedPct).
     * @param refName The name of the master column,
     * or null, if there is no master column.
     * @param fixedPct The total of the fixed concentrations.
     * @return The total of fixedPct plus the sums of the percents in each row.
     * Less than 100% if fixedPct < 100 and there are no tabled percents.
     */
    public double reconcileTabledPercents(String refName, double fixedPct) {
        boolean isRefParamInTable = false;
        LcMethodParameter refParam = null;
        if (refName != null) {
            refParam = getParameter(refName);
            isRefParamInTable = refParam.isTabled();
        }
        // Make a handy list of tabled percents
        List<LcMethodParameter> parlist = new LinkedList<LcMethodParameter>();
        String[] parnames = getPercentNames();
        for (String parName : parnames) {
            LcMethodParameter param = getParameter(parName);
            if (param.isTabled()) {
                parlist.add(param);
            }
        }
        
        Double tabledPct = null;
        int nRows = getRowCount();
        for (int row = 0; row < nRows; row++) {
            if (isRefParamInTable && getExplicitValueAt(refName, row) == null) {
                nullifyPctsInRow(parlist, row);
            } else {
                reconcilePctsInRow(refName, fixedPct, parlist, row);
                Double pct = sumPctsInRow(parlist, row);
                if (pct != null
                    && tabledPct != null && !tabledPct.equals(pct))
                {
                    // This would be an algorithm error:
                    Messages.postDebug("LcMethod.reconcileTabledPercents: "
                                       + "percents inconsistent between rows");
                }
                if (pct != null) {
                    tabledPct = pct;
                }
            }
        }
        return tabledPct == null ? fixedPct : tabledPct + fixedPct;
    }

    /**
     * Null out all the percents in a given row of the table.
     * The percents on this row are then implicitly defined by linear
     * interpolation.
     * @param parlist List of percent parameters that are in the table.
     * @param row The row number to nullify.
     */
    private void nullifyPctsInRow(List<LcMethodParameter> parlist, int row) {
        for (LcMethodParameter param : parlist) {
            param.setValue(row, null);
        }
    }

    /**
     * @param refName The name of the reference percent parameter, or null.
     * @param fixedPct The sum of the percents not in the table.
     * @param parlist List of percent parameters that are in the table.
     * @param row The index of the row to reconcile (from 0).
     */
    private void reconcilePctsInRow(String refName, double fixedPct,
                                    List<LcMethodParameter> parlist, int row) {
        // See if all the pcts in this row are null
        boolean pctsAllNull = true;
        for (LcMethodParameter param : parlist) {
            if (param.getValue(row) != null) {
                pctsAllNull = false;
                break;
            }
        }

        // If have non-null pcts, have stuff to do
        if (!pctsAllNull) {
            // Make all pcts legal
            for (LcMethodParameter param : parlist) {
                Double value = (Double)param.getValue(row);
                if (value == null) {
                    value = 0.0;
                    System.err.println("r=" + row + ", par=" + param.getName()+ ", value=" + value);
                }

                // Make each pct individually compatible with fixedPct
                value = Math.max(0, Math.min(100 - fixedPct, value));
                param.setValue(row, value);
                param.setTabled(true);
            }

            // Now try to make them add to 100% (They are already non-null.)
            double totalPct = fixedPct + sumPctsInRow(parlist, row);
            LcMethodParameter refParam = getParameter(refName);// Maybe null
            for (LcMethodParameter param : parlist) {
                if (param != refParam) { // refParam must be done last
                    totalPct = correctPct(totalPct, param, row);
                }
            }
            if (refParam != null && refParam.isTabled()) {
                correctPct(totalPct, refParam, row); // Do the ref param
            }
        }
    }

    /**
     * Adjust the percent value of a given percent parameter in the table
     * to make the percents total to 100.
     * Does nothing if totalPct=100 or param is null.
     * @param totalPct The initial sum of all fixed percents and row percents.
     * @param param The percent parameter to adjust.
     * @param row The index of the row to adjust (from 0).
     * @return The new totalPct.
     */
    private double correctPct(double totalPct,
                              LcMethodParameter param, int row) {
        if (param != null) {
            Double value = (Double)param.getValue(row);
            if (value != null) {
                double correction = 100 - totalPct;
                double delta;
                if (correction > 0) {
                    delta = Math.min(100 - value, correction);
                } else {
                    delta = Math.max(-value, correction);
                }
                totalPct += delta;
                value += delta;
                param.setValue(row, value);
                param.setTabled(true);
            }
        }
        return totalPct;
    }

    /**
     * @param parlist List of percent parameters that are in the table.
     * @param row The index of the row to check (from 0).
     * @return The sum of the percents, or -1 if all percents are null
     * in this row.
     */
    private Double sumPctsInRow(List<LcMethodParameter> parlist, int row) {
        Double rowPct = null;
        for (LcMethodParameter param : parlist) {
            Double value = (Double)param.getValue(row);
            if (value != null) {
                rowPct = rowPct == null ? value : rowPct + value;
            }
        }
        return rowPct;
    }








    /* *********************************************************************
     * Read / write methods from / to parameter files.
     */

    /**
     * Write the method to USER/lc/lcmethods under the current method name.
     * @return True on success.
     */
    public boolean writeMethodToFile() {
        String name = getPath();
        int idx = name.indexOf("lc/lcmethods");
        if (idx > 0) {
            name = name.substring(idx);
        }
        return writeMethodToFile(name);
    }

    /**
     * Writes the method into a parameter file.
     * The types and attributes of the parameters are made the same
     * as those in the template (in /vnmr/lc/lcmethods/.TemplateMethod).
     * @param outputFilepath The name of the file to write. Full path or
     * relative to USER/lc/lcmethods.
     * @return True on success.
     */
    public boolean writeMethodToFile(String outputFilepath) {
        validateParameters();
        if (DebugOutput.isSetFor("LcMethod")) {
            dumpParameters("Writing parameter values to "
                           + outputFilepath + ":");
        }
        // Get template buffer to fill in with our values.
        String templateName = "SYSTEM/lc/lcmethods/.TemplateMethod";
        String templateFilepath = FileUtil.openPath(templateName);
        if (templateFilepath == null) {
            Messages.postError("LcMethod.writeMethodToTextFile: "
                               + "Cannot open template file: "
                               + templateName);
            return false;
        }
        StringBuffer parBuf = readFileIntoBuffer(templateFilepath);
        if (parBuf.length() == 0) {
            return false;
        }

        // For each parameter in our buffer, fill in the correct values
        int bufIdx = -1;         // Running offset into the buffer
        while ((bufIdx = findNextParameter(parBuf, bufIdx)) >= 0) {
            String parName = getParameterName(parBuf, bufIdx);
            boolean isString = getParameterType(parBuf, bufIdx);
            bufIdx = deleteNextValueLines(parBuf, bufIdx);
            // The "values" line(s) has/have been removed
            // and bufIdx points to the location of the surgery

            // Build the "values" line(s)
            String sValues = getAllValuesAsString(parName);
            if (sValues == null) {
                sValues = isString ? "''" : "0";
                Messages.postDebug("LcMethod.writeMethodToFile: "
                                   + "parameter missing from method; setting "
                                   + parName + "=" + sValues);
            }
            QuotedStringTokenizer qtoker
                    = new QuotedStringTokenizer(sValues, ",", "'");
            ArrayList<String> aValues = new ArrayList<String>();
            while (qtoker.hasMoreTokens()) {
                aValues.add(qtoker.nextToken());
            }

            // Put in the number of values
            int nValues = aValues.size();
            String strTmp = nValues + " ";
            parBuf.insert(bufIdx, strTmp);
            bufIdx += strTmp.length();

            // Put in the values
            for (int i = 0; i < nValues; i++) {
                Object val = aValues.get(i);
                if (isString) {
                    strTmp = "\"" + val + "\"\n";
                } else {
                    strTmp = String.valueOf(val);
                    // Make sure strTmp is a valid decimal number string
                    if (!NUMBER_PATTERN.matcher(strTmp).matches()) {
                        strTmp = "0";
                    }
                    strTmp += " ";
                }
                parBuf.insert(bufIdx, strTmp);
                bufIdx += strTmp.length();
            }
            if (!isString) {
                parBuf.insert(bufIdx, "\n");
                bufIdx++;
            }
        }

        // Write out the buffer (with native form of newlines)
        BufferedWriter fw = null;
        String path = null;
        try {
            path = FileUtil.savePath(outputFilepath);
            fw = new BufferedWriter(new FileWriter(path));
        } catch (IOException ioe) {
            Messages.postError("LcMethod.writeMethodToTextFile: "
                               + "Cannot open output file: "
                               + path);
        } catch (Exception e) {
            Messages.postError("LcMethod.writeMethodToTextFile: "
                               + "Cannot open output file: "
                               + outputFilepath);
            return false;
        }
        PrintWriter pw = new PrintWriter(fw);
        StringTokenizer toker = new StringTokenizer(parBuf.toString(), "\n\r");
        while (toker.hasMoreTokens()) {
            pw.println(toker.nextToken());
        }
        try { pw.close(); } catch (Exception e) {}
        return true;
    }

    /**
     * Initializes the method from a given parameter file.
     * Any method currently being held is discarded.
     * @param filepath The full path to the file to read.
     * @return True on success.
     */
    public boolean readMethodFromFile(String filepath) {
        Messages.postDebug("LcMethod",
                           "LcMethod.readMethodFromFile(\"" + filepath + "\")");
        boolean ok = false;

        // Read the parameter file into a buffer.
        // NB: This returns a 0 length buffer on failure:
        StringBuffer parBuf = readFileIntoBuffer(filepath);
        //String parBuf = parBuf.toString();

        // Parse the file for method parameter values, storing the
        // values in the proper LcMethodParameters as we go along.
        int bufIdx = -1;         // Running offset into the buffer
        bufIdx = findNextParameter(parBuf, bufIdx);
        while (bufIdx >= 0) {
            String parName = getParameterName(parBuf, bufIdx);
            boolean isString = getParameterType(parBuf, bufIdx);
            
            int parEnd = findNextParameter(parBuf, bufIdx);
            QuotedStringTokenizer toker;
            toker  = getValueTokenizer(parBuf, bufIdx, parEnd, isString);
            if (toker == null) {
                return false;
            }

            int nVals = Integer.parseInt(toker.nextToken());
            ArrayList<String> parValues = new ArrayList<String>(nVals);
            int iVal;
            for (iVal = 0; iVal < nVals && toker.hasMoreTokens(); iVal++) {
                parValues.add(toker.nextToken());
            }

            ok &= fixParValues(parName, parValues);
            ok &= setAllValuesInItem(parName, parValues);
            bufIdx = parEnd;    // bufIdx at next parameter name, or -1
        }
        validateParameters();
        fixSolventAssignments();
        reconcilePercents(null);
        // TODO: Check for missing parameters in method?
        setPath(filepath);
        return ok;
    }

    /**
     * Given a method name, find the path to the method file.
     * <br><b>Side effect:</b><br>
     * Also may modify the class member variable m_methodName by
     * prepending enough of the tail of the method pathname to make it
     * unique.
     * @param name The name or absolute or abstract path of the method
     * to be read.
     * @return The full path of the file containing the method, or null
     * if the method is not found.
     */
    public String resolveMethodReadPath(String name) {
        // TODO: Look for methods under data dirs?
        // TODO: Use symbols in methodName for usrMethodDir, sysMethodDir, etc.?
        // TODO: Validate that files found are really MethodFiles?
        Messages.postDebug("LcMethod", "LcMethod.resolveMethodReadPath(\""
                           + name + "\")");
        if (name == null) {
            return null;
        }
        String filepath = null;
        String methodName = "";
        String userdir = FileUtil.openPath("USER/lc/lcmethods/");
        String systemdir = FileUtil.openPath("SYSTEM/lc/lcmethods/");
        if (userdir != null && name.startsWith(userdir)) {
            // Absolute path under user lcmethods directory
            filepath = name;
            methodName = name.substring(userdir.length());
            Messages.postDebug("LcMethod", "LcMethod.resolveMethodReadPath: "
                               + "method is in userdir, methodName=\""
                               + methodName + "\"");
        } else if (systemdir != null && name.startsWith(systemdir)) {
            // Absolute path under system lcmethods directory
            filepath = name;
            methodName = name.substring(systemdir.length());
            Messages.postDebug("LcMethod", "LcMethod.resolveMethodReadPath: "
                               + "method is in systemdir, methodName=\""
                               + methodName + "\"");
            // But if this methodName is also under userdir, that's a problem
            if (FileUtil.openPath(userdir + name) != null) {
                // Need to specify the absolute path
                methodName = filepath;
                Messages.postDebug("LcMethod", " ... "
                                   + "but also in userdir, so methodName=\""
                                   + methodName + "\"");
            }
        } else if ((filepath = FileUtil.openPath(userdir + name)) != null) {
            methodName = name;
            Messages.postDebug("LcMethod", "LcMethod.resolveMethodReadPath: "
                               + "it's in userdir, methodName=\""
                               + methodName + "\"");
        } else if ((filepath = FileUtil.openPath(systemdir + name)) != null) {
            methodName = name;
            Messages.postDebug("LcMethod", "LcMethod.resolveMethodReadPath: "
                               + "it's in systemdir, methodName=\""
                               + methodName + "\"");
        } else if (new File(name).exists()) {
            // General relative or absolute path
            filepath = methodName = name;
            File oFilepath = new File(filepath);
            if (!oFilepath.isAbsolute()) {
                Messages.postDebug("LcMethod",
                                   "LcMethod.resolveMethodReadPath: "
                                   + "converting path=\"" + filepath
                                   + "\" to absolute path");
                filepath = methodName = oFilepath.getAbsolutePath();
            }
            Messages.postDebug("LcMethod", "LcMethod.resolveMethodReadPath: "
                               + "use absolute path, filepath=\""
                               + filepath + "\"");
        } else {
            // Couldn't find the method file
            filepath = null;
            Messages.postDebug("LcMethod", "LcMethod.resolveMethodReadPath: "
                               + "method not found, methodName=\""
                               + methodName + "\"");
        }
        Messages.postDebug("LcMethod", "LcMethod.resolveMethodReadPath: "
                           + "returning \"" + filepath + "\"");
        setName(methodName);
        return filepath;
    }

    /**
     * Possibly modify the values for a given parameter to match the
     * current instrument configuration or software revision.
     * @param parName The name of the parameter that the values apply to.
     * @param parValues A list of the values, as strings.
     * @return Always returns true, meaning we hope the conversion is OK.
     */
    private boolean fixParValues(String parName, List<String> parValues) {
        if (parName.equals("lcTrace")) {
            return fixTraceValues(parValues);
        }
        return true;            // No changes
    }


    /**
     * Possibly modify the values for the lcTrace parameter to match the
     * current instrument configuration or software revision.
     * @param parValues A list of the values, as strings.
     * @return Always returns true, meaning we hope the fix is OK.
     */
    private boolean fixTraceValues(List<String> parValues) {
        final int NTRACES = 5;
        int oldSize = parValues.size();
        // Find the best match for any traces assigned in the method
        List<String> parChoices = getMenuChoices("USER/lc/lcTraceMenu");
        for (int i = 0; i < oldSize; i++) {
            String oldValue = parValues.get(i);
            String newValue = null;
            if (!"none".equals(oldValue)) { // Never change "none"
                if (oldValue == null) {
                    newValue = "none";
                } else {
                    // "codes" is the colon delimited stuff prefixing the string
                    // This prefix is terminated by a space
                    String codes = oldValue;
                    int cut = oldValue.indexOf(' ');
                    if (cut > 0) {
                        codes = oldValue.substring(0, cut);
                    }
                    // Split "codes" into primary and secondary code words
                    StringTokenizer toker = new StringTokenizer(codes, ":");
                    String code1 = ":other:";
                    String code2 = "::";
                    if (toker.hasMoreTokens()) {
                        code1 = ":" + toker.nextToken() + ":";
                    }
                    if (toker.hasMoreTokens()) {
                        code2 = ":" + toker.nextToken() + ":";
                    }

                    // Look for a choice that matches the first code
                    for (String choice : parChoices) {
                        if (choice.startsWith(code1)) {
                            newValue = choice;
                            break;
                        }
                    }
                    // If no match or uncertain match; try second code
                    if (newValue == null || code1.equals(":other:")) {
                        for (String choice : parChoices) {
                            cut = choice.indexOf(':', 1); // Find second colon
                            cut = Math.max(0, cut);
                            codes = choice.substring(cut);
                            if (codes.startsWith(code2)) {
                                newValue = choice;
                                break;
                            }
                        }
                    }
                    parChoices.remove(newValue); // Can't use this choice again
                }
            }
            if (newValue == null) {
                newValue = "none";
            }
            parValues.set(i, newValue);
        }

        // Any additional traces are not used
        for (int i = oldSize; i < NTRACES; i++) {
            parValues.add("none");
        }
        return true;
    }

    /**
     * Assign solvent concentrations to the correct bottles to match the
     * current LC configuration.
     * Called after all the method parameters have been loaded.
     */
    private boolean fixSolventAssignments() {
        // TODO: All these solvent names should just be 2 arrayed parameters?
        String[] methodSolventBottles = getSolventBottles();
        Arrays.sort(methodSolventBottles); // Make sure they're alphabetical
        String[] configBottleNames = {"lcConfigSolvA",
                                      "lcConfigSolvB",
                                      "lcConfigSolvC", };
        String[] percentNames = getPercentNames();
        Arrays.sort(percentNames); // Make sure they're alphabetical

        int nRows = getRowCount();
        int nConfigSolvents = configBottleNames.length;
        int nMethodSolvents = Math.min(percentNames.length,
                                       methodSolventBottles.length);

        // Collect info on solvents from configuration
        LcConfiguration config = getConfiguration();
        String[] configSolvents =  new String[nConfigSolvents];
        Set<Integer> availableSolvents = new TreeSet<Integer>();
        for (int i = 0; i < nConfigSolvents; i++) {
            configSolvents[i] = config.getSolvent(i);
            availableSolvents.add(i);
        }

        // Collect info on solvents / percents from method
        Double[][] methodPercents = new Double[nMethodSolvents][nRows];
        boolean[] isSolventUsed = new boolean[nMethodSolvents];
        String[] methodSolvents = new String[nMethodSolvents];
        for (int i = 0; i < nMethodSolvents; i++) {
            LcMethodParameter methodSolventParam
                    = getParameter(methodSolventBottles[i]);
            methodSolvents[i] = (String)methodSolventParam.getValue();
            LcMethodParameter percentParam = getParameter(percentNames[i]);
            isSolventUsed[i] = false;
            for (int j = 0; j <nRows; j++) {
                methodPercents[i][j] = (Double)percentParam.getValue(j);
                if (!isSolventUsed[i] // Don't need to check if already known
                    && methodPercents[i][j] != null
                    && methodPercents[i][j].doubleValue() != 0)
                {
                    isSolventUsed[i] = true;
                }
            }
        }

        // Match method solvent requirements to configured solvents
        int[] methodToConfigMap = new int[nMethodSolvents];
        for (int i = 0; i < nMethodSolvents; i++) {
            methodToConfigMap[i] = -1;
        }
        // First, assign solvents that match configured solvents
        Set<Integer> orphanSolvents = new TreeSet<Integer>();
        for (int i = 0; i < nMethodSolvents; i++) {
            String solvent = getValueString(methodSolventBottles[i]);
            boolean foundIt = false;
            Set<Integer> usedIndices = new TreeSet<Integer>();
            for (int j = 0; solvent != null && j < nConfigSolvents; j++) {
                if (solvent.equals(configSolvents[j])
                    && !usedIndices.contains(j))
                {
                    methodToConfigMap[i] = j;
                    usedIndices.add(j);
                    availableSolvents.remove(j);
                    foundIt = true;
                    break;
                }
            }
            if (!foundIt) {
                orphanSolvents.add(i);
                if (isSolventUsed[i]) {
                    Messages.postError("Solvent not available: " + solvent);
                }
            }
        }
        // Arbitrarily assign non-matching solvents
        Iterator<Integer> availableSolventsItr = availableSolvents.iterator();
        Iterator<Integer> orphanSolventsItr = orphanSolvents.iterator();
        while (orphanSolventsItr.hasNext()) {
            int i = orphanSolventsItr.next();
            if (isSolventUsed[i]) {
                String solvent = getValueString(methodSolventBottles[i]);
                if (availableSolventsItr.hasNext()) {
                    methodToConfigMap[i] = availableSolventsItr.next();
                    availableSolventsItr.remove();
                    orphanSolventsItr.remove();
                } else {
                    Messages.postError("Method uses more solvents than are "
                                       + "configured");
                    break;
                }
            }
        }
        // Finally, assign non-matching, unused solvents
        orphanSolventsItr = orphanSolvents.iterator();
        while (orphanSolventsItr.hasNext()) {
            int i = orphanSolventsItr.next();
            String solvent = getValueString(methodSolventBottles[i]);
            if (availableSolventsItr.hasNext()) {
                methodToConfigMap[i] = availableSolventsItr.next();
                availableSolventsItr.remove();
                orphanSolventsItr.remove();
            } else {
                break;
            }
        }

        // Put method solvent values and percent values in correct parameters
        for (int i = 0; i < nMethodSolvents; i++) {
            setValue(methodSolventBottles[i], "none");
        }
        for (int i = 0; i < nMethodSolvents; i++) {
            if (methodToConfigMap[i] >= 0) {
                String newBottle = methodSolventBottles[methodToConfigMap[i]];
                setValue(newBottle, configSolvents[methodToConfigMap[i]]);
                String newPctName = percentNames[methodToConfigMap[i]];
                LcMethodParameter param = getParameter(newPctName);
                param.setNonNullValues(methodPercents[i]);
            }
        }

        return true;
    }

    /**
     * Get the valid choices for the lcTrace values.
     * @param filepath The absolute or abstract path to the file
     * containing the choices.
     * @return The list of choice strings.
     */
    public static List<String> getMenuChoices(String filepath) {
        List<String> choices = new ArrayList<String>();
        String fullpath = FileUtil.openPath(filepath);
        if (fullpath == null) {
            Messages.postError("LcMethod: Cannot read lcTrace choices");
            Messages.postDebug("LcMethod.getMenuChoices: Cannot open file \""
                               + filepath + "\"");
        } else {
            BufferedReader in = null;
            try {
                String line;
                in = new BufferedReader(new FileReader(fullpath));
                while ((line = in.readLine()) != null) {
                    line = line.trim();
                    if (!line.startsWith("#") && line.length() > 0) {
                        QuotedStringTokenizer toker
                                = new QuotedStringTokenizer(line);
                        String choice = null;
                        if (toker.hasMoreTokens()) {
                            // First token is fallback choice
                            choice = toker.nextToken();
                        }
                        if (toker.hasMoreTokens()) {
                            // Second token should always be there
                            choice = toker.nextToken();
                        }
                        if (choice != null) {
                            choices.add(choice);
                        }
                    }
                }
            } catch (IOException ioe) {
                Messages.postError("LcMethod.getMenuChoices: "
                                   + "Error reading file: " + fullpath);
            }
            try { in.close(); } catch (Exception e) {}
        }
        return choices;
    }

    /**
     * Validate all the method parameters, and corrects them if necessary.
     * Parameters with all null entries are given a default initial value
     * (that depends only on the Object type of the parameter).
     * @return Returns true if any parameter was changed.
     */
    private boolean validateParameters() {
        boolean changed = false;
        Collection<LcMethodParameter> params
                = m_methodParameters.values();
        for (LcMethodParameter param : params) {
            if (param.getInitialValue() == null) {
                param.setValue(0, param.getDefaultValue());
                changed = true;
            }
        }
        return changed;
    }

    /**
     * Sets all the values of a parameter.  Also sets whether the parameter
     * is in the table, based on whether there is more than one value.
     * (But the time parameter is always in the table.)
     * @param name The name of the parameter in which to set the values.
     * @param values A list of strings containing the values to set.
     */
    private boolean setAllValuesInItem(String name, Collection<String> values) {
        if (values == null) {
            return false;
        } else {
            return setAllValuesInItem(name, values.toArray(new String[0]));
        }
    }

    /**
     * Sets all the values of a parameter.  Also sets whether the parameter
     * is in the table, based on whether there is more than one value.
     * (But the time parameter is always in the table.)
     * @param name The name of the parameter in which to set the values.
     * @param values A list of strings containing the values to set.
     */
    private boolean setAllValuesInItem(String name, String[] values) {
        if (DebugOutput.isSetFor("LcMethod")) {
            Messages.postDebug("setAllValuesInItem: " + name);
            for (String value: values) {
                Messages.postDebug( "\"" + value + "\"");
            }
        }

        boolean rtn = false;
        if (values != null) {
            LcMethodParameter par = getParameter(name);
            if (par != null) {
                par.setAllValues(values);
                // NB: Time parameter is always in the table:
                par.setTabled(values.length > 1 || par == getTimeParameter());
                rtn = true;
            } else {
                Messages.postDebug("LcMethod.setAllValuesInItem: "
                                   + "Parameter not found: \"" + name + "\"");
            }
        }
        return rtn;
    }

    /**
     * Concatenate an array of Strings, separating elements in the array
     * with newlines in the resultant String.
     * @param lines An array of lines without newline characters.
     * @return A single String combining the input array.
     */
    private String strcatStrings(List<String> lines, String separator) {
        int len = lines.size();
        if (len == 1) {
            return lines.get(0);
        } else {
            StringBuffer sb = new StringBuffer("");
            for (String line: lines) {
                sb.append(line);
                sb.append(separator);
            }
            sb.deleteCharAt(sb.length() - 1); // Remove trailing linefeed
            return sb.toString();
        }
    }

    /**
     * @param idx Points to somewhere in the first line of the parameter
     * entry.
     */
    private QuotedStringTokenizer getValueTokenizer(StringBuffer parBuf,
                                              int idx,
                                              int parEnd,
                                              boolean isString) {
        int len = parBuf.length();
        QuotedStringTokenizer toker = null;
        // Advance to values line
        for ( ; idx < len && parBuf.charAt(idx) != '\n'; idx++);
        if (idx == len) {
            return null;        // Premature EOF
        }
        idx++;

        String parEntry;
        if (parEnd < 0) {
            parEntry = parBuf.substring(idx);
        } else {
            parEntry = parBuf.substring(idx, parEnd);
        }
        //if (isString) {
            toker = new QuotedStringTokenizer(parEntry, "\n ", "\"");
        //} else {
        //    toker = new StringTokenizer(parEntry, "\n ");
        //}
        return toker;
    }

    /**
     * Read the given text file into a StringBuffer.
     * Lines in the buffer are separated by "\n" regardless of the
     * convention used by the input file.
     * @param filepath The full path and name of the file to read.
     * @return The buffer. The buffer is empty on failure, never null.
     */
    private StringBuffer readFileIntoBuffer(String filepath) {
        StringBuffer sb = new StringBuffer();
        BufferedReader in = null;
        try {
            String line;
            in = new BufferedReader(new FileReader(filepath));
            while ((line = in.readLine()) != null) {
                sb.append(line + "\n");
            }
        } catch (IOException ioe) {
            Messages.postError("LcMethod.readFileIntoBuffer: "
                               + "Error reading file: " + filepath);
        }
        try { in.close(); } catch (Exception e) {}
        return sb;
    }

    /**
     * Given a buffer containing a parameter file and an index into
     * that buffer, finds the beginning of the next parameter name.
     * Starts looking at the given index, for a line-feed followed
     * by a letter (the first letter of the parameter name).
     * Exception, if the index is <0, returns 0 if it's a letter.
     * @param sbBuf The buffer containing the parameter file.
     * @param idx Where to start looking in the buffer. Set idx=-1 to
     * search from the beginning.
     * @return The index of the beginning of the parameter name. 
     */
    private int findNextParameter(StringBuffer sbBuf, int idx) {
        int len = sbBuf.length();
        if (idx >= len) {
            return -1;
        }
        if (idx < 0 && Character.isLetter(sbBuf.charAt(0))) {
            // Special case: first line of file starts with parameter name
            return 0;
        }
        char chr = sbBuf.charAt(idx);
        // Find first newline followed by a leter.
        for ( ; idx < len; idx++) {
            chr = sbBuf.charAt(idx);
            if (sbBuf.charAt(idx) == '\n') {
                // Check next character
                if (++idx >= len) {
                    return -1;  // EOF
                } else {
                    chr = sbBuf.charAt(idx);
                    if (Character.isLetter(chr)) {
                        return idx;
                    }
                }
            }
        }
        return -1;
    }

    /**
     * Get the parameter name that starts at the given index.
     * 
     * @param sbBuf The buffer containing the parameter file.
     * @param idx Where the name starts in the buffer.
     * @return The parameter name. 
     */
    private String getParameterName(StringBuffer sbBuf, int idx) {
        int len = sbBuf.length();
        int end;
        // Find the space at the end of the name
        for (end = idx + 1 ; end < len && sbBuf.charAt(end) != ' '; end++);
        return sbBuf.substring(idx, end);
    }

    /**
     * Given a buffer containing a parameter file
     * and an index into that buffer pointing to a parameter name,
     * finds the type of that parameter.
     * 
     * @param sbBuf The buffer containing the parameter file.
     * @param idx Where the name starts in the buffer.
     * @return True if the parameter is a string type.
     */
    private boolean getParameterType(StringBuffer sbBuf, int idx) {
        boolean isString = false;
        int len = sbBuf.length();
        int end = Math.min(idx + 80, len - 1);
        String buf = sbBuf.substring(idx, end);
        StringTokenizer toker = new StringTokenizer(buf, " ");
        if (toker.hasMoreTokens()) {
            toker.nextToken();  // Skip over the name
        }
        if (toker.hasMoreTokens()) {
            toker.nextToken();  // Skip over the subtype
        }
        if (toker.hasMoreTokens()) {
            if (toker.nextToken().equals("2")) { // String is basictype 2
                isString = true;
            }
        }
        return isString;
    }

    /**
     * Given a buffer containing a parameter file and an index into
     * that buffer pointing somewhere in a line beginning a parameter
     * definition, deletes the following line, and any succeeding lines
     * that are part of the "values" definition.
     * <p>Bug: Does not do well with string parameter values that
     * contain newlines.
     * 
     * @param sbBuf The buffer containing the parameter file.
     * @param idx Where the name starts in the buffer.
     * @return The index of the buffer location where the deletion occurred.
     */
    private int deleteNextValueLines(StringBuffer sbBuf, int idx) {
        int len = sbBuf.length();
        // Advance to the first newline
        for ( ; idx < len && sbBuf.charAt(idx) != '\n'; idx++);
        if (idx == len) {
            return idx;
        }
        idx++;
        int end;
        do {
            for (end = idx ; end < len && sbBuf.charAt(end) != '\n'; end++);
            sbBuf.delete(idx, ++end);
            len = sbBuf.length();
            // idx now points to where end pointed before the deletion
        } while (idx < len && sbBuf.charAt(idx) == '"');

        return idx;
    }

    /**
     * Write out the method in HTML format.
     */
    public void printMethod(String filepath, String title) {
        // Open file
        PrintWriter pw = null;
        try {
            pw = new PrintWriter
                    (new OutputStreamWriter(new FileOutputStream(filepath),
                                            "UTF-8"));
        } catch (IOException ioe) {
            Messages.postError("Cannot open LcMethod log file: " + filepath);
            return;
        }

        // Use nested tables (2 levels) to arrange the data.
        // Outer level if for positioning only, inner level has visible,
        // ruled tables.

        pw.println("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\""
                   + "\"http://www.w3.org/TR/REC-html40/strict.dtd\">");
        pw.println("<html>");
        pw.println("<head>");
        pw.println("<meta http-equiv=\"Content-Type\""
                   + " content=\"text/html; charset=utf-8\"/>");
        pw.println("</head>");
        pw.println("<body bgcolor=\"#FFFFFF\">");
        if (title.trim().length() > 0) {
            pw.println("<title>" + title + "</title>");
            pw.println();
            pw.println("<h3 align=center>" + title + "</h3>");
        }
        pw.println("Method Name: <b>" + getValueString("lcMethodFile") + "</b> ("
                   + LcRun.getRunTypeString(getValueString("lcexp")) + ")");

        // Start outer table
        pw.println("<table border=0 cellpadding=10>"); // Outer table

        pw.println("<tr><td colspan=2>"); // Outer table
        // Print method table
        pw.println("<table border=1 cellpadding=3 cellspacing=0>");
        pw.println("  <caption align=top>");
        pw.println("    <h4>Method Table</h4>");
        pw.println("  </caption>");
        pw.println("  <tr>");

        Collection<LcMethodParameter> tablePars = getTabledParameters();
        for (LcMethodParameter par: m_methodParameters.values()) {
            if (par.isTabled()) {
                tablePars.add(par);
            }
        }
        for (LcMethodParameter par: tablePars) {
            pw.println("<td align=center nowrap>" + par.getName() + "</td>");
        }
        pw.println("  </tr>");
        int nRows = getRowCount();
        for (int i = 0; i < nRows; i++) {
            pw.println("  <tr>");
            for (LcMethodParameter par: tablePars) {
                String value = getHtmlValueAt(par, i);
                pw.println("<td align=center nowrap>" + value + "</td>");
            }
            pw.println("  </tr>");
        }
        pw.println("  </table>");
        pw.println("</td></tr>"); // Outer table

        pw.println("<tr><td rowspan=4 valign=top>"); // Outer table
        // Pump and Relay Info
        pw.println("<table border=1 cellpadding=3 cellspacing=0>");
        pw.println("  <caption align=top>");
        pw.println("    <h4>Pump and Relay Information</h4>");
        pw.println("  </caption>");
        // Solvent Info
        pw.println("<tr><th colspan=3>Solvent Mixture</th></tr>");
        String pct = getHtmlValue("lcPercentA");
        String solv = getValueString("lcConfigSolvA");
        solv = LcControl.Solvents.getDisplayName(solv);
        pw.println("<tr><td>%A</td>"
                   + "<td>" + pct + "</td>"
                   + "<td>" + solv + "</td></tr>");

        pct = getHtmlValue("lcPercentB");
        solv = getValueString("lcConfigSolvB");
        solv = LcControl.Solvents.getDisplayName(solv);
        pw.println("<tr><td>%B</td>"
                   + "<td>" + pct + "</td>"
                   + "<td>" + solv + "</td></tr>");

        pct = getHtmlValue("lcPercentC");
        solv = getValueString("lcConfigSolvC");
        solv = LcControl.Solvents.getDisplayName(solv);
        pw.println("<tr><td>%C</td>"
                   + "<td>" + pct + "</td>"
                   + "<td>" + solv + "</td></tr>");

        solv = getValueString("solvent");
        solv = LcControl.Solvents.getDisplayName(solv);
        pw.println("<tr><td colspan=2>Lock Solvent</td>"
                   + "<td>" + solv + "</td></tr>");

        pw.println("<tr><th colspan=3>Pump</th></tr>");
        pw.println("<tr><td colspan=2>Flow Rate</td>"
                   + "<td>" + getValueString("lcFlowRate") + " mL/min</td></tr>");
        pw.println("<tr><td colspan=2>Equilibration Time</td>"
                   + "<td>" + getValueString("lcEquilTime") + " min</td></tr>");
        pw.println("<tr><td colspan=2>Minimum Pressure</td>"
                   + "<td>" + getValueString("lcMinPressure") + " atm</td></tr>");
        pw.println("<tr><td colspan=2>Maximum Pressure</td>"
                   + "<td>" + getValueString("lcMaxPressure") + " atm</td></tr>");
        String endAction = ynToOnoff(getValueString("lcEndAction"));
        pw.println("<tr><td colspan=2>Pump after Run</td>"
                   + "<td>" + endAction + "</td></tr>");
        
        pw.println("<tr><th colspan=3>Valves and Relays</th></tr>");
        pw.println("<tr><td colspan=2>Inject Valve</td>"
                   + "<td>" + getHtmlOnOffValue("lcRelay1") + "</td></tr>");
        pw.println("<tr><td colspan=2>Bypass NMR</td>"
                   + "<td>" + getHtmlOnOffValue("lcNmrBypass") + "</td></tr>");
        pw.println("<tr><td colspan=2>Relay 3</td>"
                   + "<td>" + getHtmlOnOffValue("lcRelay3") + "</td></tr>");
        pw.println("<tr><td colspan=2>Relay 4</td>"
                   + "<td>" + getHtmlOnOffValue("lcRelay4") + "</td></tr>");
        pw.println("<tr><td colspan=2>Relay 5</td>"
                   + "<td>" + getHtmlOnOffValue("lcRelay5") + "</td></tr>");
        pw.println("<tr><td colspan=2>Relay 6</td>"
                   + "<td>" + getHtmlOnOffValue("lcRelay6") + "</td></tr>");

        pw.println("  </table>");
        pw.println("</td>");    // Outer table

        pw.println("<td>");    // Outer table
        // Column Info
        pw.println("<table border=1 cellpadding=3 cellspacing=0>");
        pw.println("  <caption align=top>");
        pw.println("    <h4>Column Information</h4>");
        pw.println("  </caption>");
        pw.println("<tr><td>Column Type</td> <td>"
                   + getValueString("lcColumnType") + "</td></tr>");
        pw.println("<tr><td>Column ID</td> <td>"
                   + getValueString("lcColumnId") + "</td></tr>");
        pw.println("<tr><td>Column Length</td> <td>"
                   + getValueString("lcColumnLength") + " cm</td></tr>");
        pw.println("<tr><td>Column Width</td> <td>"
                   + getValueString("lcColumnWidth") + " cm</td></tr>");
        pw.println("<tr><td>Dead Volume</td> <td>"
                   + getValueString("lcColumnDeadVolume") + " mL</td></tr>");
        pw.println("<tr><td>Particle Size</td> <td>"
                   + getValueString("lcColumnParticleSize")
                   + " " + MU + "m</td></tr>");
        pw.println("  </table>");
        pw.println("</td></tr>");    // Outer table

        pw.println("<tr><td>");    // Outer table
        // LC-NMR Run Mode
        pw.println("<table border=1 cellpadding=3 cellspacing=0>");
        pw.println("  <caption align=top>");
        pw.println("    <h4>LC-NMR Run Mode</h4>");
        pw.println("  </caption>");
        pw.println("<tr><td>Time Slice</td>"
                   + "<td>" + getHtmlOnOffValue("lcTimeSlice") + "</td></tr>");
        String slicePeriod = getHtmlValue("lcTimeSlicePeriod", " s");
        pw.println("<tr><td>Time Slice Interval</td>" + "<td>"
                   + slicePeriod + "</td></tr>");
        pw.println("<tr><td>ADC Interval</td>"
                   + "<td>" + getValueString("lcAdcPeriod") + " s</td></tr>");
        double holdTime = Double.parseDouble(getValueString("lcHoldTime"));
        String strHoldTime = holdTime < 0 ? "Indefinitely"
                : holdTime == 0 ? "Until NMR Finishes"
                : Fmt.f(2, holdTime) + " min";
        pw.println("<tr><td>NMR Hold Time</td>"
                   + "<td>" + strHoldTime + "</td></tr>");
        pw.println("  </table>");
        pw.println("</td></tr>");    // Outer table

        pw.println("<tr><td>");    // Outer table
        // UV Detector
        String uvDetector = getValueString("lcDetector");
        if (uvDetector != null && uvDetector.equals("335")) {
            pw.println("<table border=1 cellpadding=3 cellspacing=0>");
            pw.println("  <caption align=top>");
            pw.println("    <h4>" + uvDetector + " Detector</h4>");
            pw.println("  </caption>");
            pw.println("<tr><td>Min " + LAMBDA + "</td>"
                       + "<td>" + getValueString("lcLambdaMin") + " nm</td></tr>");
            pw.println("<tr><td>Max " + LAMBDA + "</td>"
                       + "<td>" + getValueString("lcLambdaMax") + " nm</td></tr>");
            String slitWidth = JbControl.getSlitLabel(getValueString("lcBandwidth"));
            pw.println("<tr><td>Slit Width" + "</td>"
                       + "<td>" + slitWidth + "</td></tr>");
            String dataRate
                    = JbControl.getDataRateLabel(getValueString("lcPdaPeriod_ms"));
            pw.println("<tr><td>Data Interval" + "</td>"
                       + "<td>" + dataRate + "</td></tr>");
            
            pw.println("  </table>");
        }
        pw.println("</td></tr>");    // Outer table

        pw.println("<tr><td>");    // Outer table
        // Mass Spectrometer
        String massSpec = getValueString("msSelection");
        if (massSpec != null && !massSpec.equals("0")) {
            pw.println("<table border=1 cellpadding=3 cellspacing=0>");
            pw.println("  <caption align=top>");
            pw.println("    <h4>" + massSpec + " Mass Spectrometer</h4>");
            pw.println("  </caption>");
            String msScan = getHtmlValue("msScans");
            pw.println("<tr><td>MS Scan" + "</td>"
                       + "<td>" + msScan + "</td></tr>");
            String msValve = getHtmlOnOffValue("msValve");
            pw.println("<tr><td>MS Valve" + "</td>"
                       + "<td>" + msValve + "</td></tr>");
            String msPump = getHtmlOnOffValue("msPump");
            pw.println("<tr><td>MS Pump" + "</td>"
                       + "<td>" + msPump + "</td></tr>");
            pw.println("  </table>");
        }
        pw.println("</td></tr>");    // Outer table

        pw.println("<tr><td colspan=2>");    // Outer table
        // Chromatogram Traces
        pw.println("<table border=1 cellpadding=3 cellspacing=0>");
        pw.println("  <caption align=top>");
        pw.println("    <h4>" + "Chromatogram Traces</h4>");
        pw.println("  </caption>");
        // Heading
        pw.println("  <tr><td>Trace</td>"
                   + "  <td align=center nowrap>Trace Name</td>"
                   + "  <td align=center nowrap>Note</td>"
                   + "  <td align=center nowrap>Peak Detection</td>"
                   + "  <td align=center nowrap>Threshold</td>"
                   + "  </tr>"
                   );
        pw.println(getHtmlTraceString(1));
        pw.println(getHtmlTraceString(2));
        pw.println(getHtmlTraceString(3));
        pw.println(getHtmlTraceString(4));
        pw.println(getHtmlTraceString(5));

        pw.println("  </table>");
        pw.println("</td></tr>"); // Outer table
        pw.println("</table>"); // Outer table

        // End of file
        pw.println("</body>");
        pw.println("</html>");
        pw.close();
    }

    private String getHtmlTraceString(int iTrace) {
        String rtn = "";
        String traceName = "lcTrace[" + iTrace + "]";
        String traceLambdaName = "lcTraceLambda[" + iTrace + "]";
        String pkDetectName = "lcPeakDetect" + (char)('A' + iTrace - 1);
        String pkThreshName = "lcThreshold" + (char)('A' + iTrace - 1);
        /*System.out.println("pkDetectName=" + pkDetectName
                           + ", pkThreshName=" + pkThreshName);/*CMP*/
        String trace = getValueString(traceName);
        if (trace != null) {
            String note;
            note = (trace.indexOf(":uv") >= 0)
                    ? LAMBDA + "=" + getValueString(traceLambdaName) + " nm"
                    : " ";
            trace = stripTraceCodes(trace);
            String peak = " ";
            String thresh = " ";
            if (!trace.equalsIgnoreCase("none")) {
                peak = getHtmlOnOffValue(pkDetectName);
                thresh = getHtmlValue(pkThreshName);
                rtn = "<tr><td>" + iTrace + "</td>"
                        + "<td>" + trace + "</td>"
                        + "<td>" + note + "</td>"
                        + "<td>" + peak + "</td>"
                        + "<td>" + thresh + "</td>"
                        + "</tr>";
            }
        }
        return rtn;
    }

    private String getHtmlValue(String name) {
        return getHtmlValue(name, "");
    }

    private String getHtmlValue(String name, String suffix) {
        String rtn = "";
        LcMethodParameter par = getParameter(name);
        if (par != null) {
            String[] aVals = par.getAllValueStrings();
            if (aVals.length == 1) {
                rtn = aVals[0] + suffix;
            } else if (aVals.length > 1) {
                rtn = "<i>Variable</i>";
            }
        }
        return rtn;
    }

    /**
     * Returns the initial value of a boolean parameter ("y" or "n") as
     * "on" or "off".
     * @param name The name of the parameter.
     * @return Returns "on" if parameter value is "y",
     * "off" if it is "n", and the value itself if it is anything else.
     */
    private String getHtmlOnOffValue(String name) {
        String value = getHtmlValue(name);
        return ynToOnoff(value);
    }

    /**
     * Convert a "y" or "n" value or "0"/"1" value to an "on" or "off" value.
     * Other values are returned unchanged.
     * @param yn A string to convert.
     * @return Returns "on" if input is "y" or "1",
     * "off" if input is "n" or "0",
     * and the input value itself if it is anything else.
     */
    public static String ynToOnoff(String yn) {
        return (yn.equals("n") || yn.equals("0"))
                ? "Off"
                : ((yn.equals("y") || yn.equals("1"))? "On" : yn);
    }

    public String getHtmlValueAt(LcMethodParameter par, int row) {
        Object oValue = par.getValue(row);
        String value = oValue == null ? "&nbsp;" : oValue.toString();
        if (value.equals("true")) {
            if (row == 0 || !par.getValue(row - 1).toString().equals(value)) {
                value = "<b>On</b>";
            } else {
                value = "&nbsp;";
            }
        } else if (value.equals("false")) {
            if (row == 0 || !par.getValue(row - 1).toString().equals(value)) {
                value = "Off";
            } else {
                value = "&nbsp;";
            }
        }
        return value;
    }

    /**
     * Strip the initial ":" delimited codes from the front of an
     * LcTrace value. The resulting string is the value to be
     * presented to the user.
     */
    public static String stripTraceCodes(String value) {
        int idx;
        if (value != null
            && value.startsWith(":")
            && (idx = value.indexOf(' ')) > 0)
        {
            value = value.substring(idx + 1).trim();
        }
        return value;
    }
}
