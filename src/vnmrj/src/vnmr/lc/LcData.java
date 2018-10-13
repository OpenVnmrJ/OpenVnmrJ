/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.util.*;

import vnmr.util.Messages;


/**
 * Holds data from an LC run.  Data are kept in lists of "LcDatum"s; one
 * list for each active trace.
 */
public class LcData {

    /** An array of lists of data. One list for each active trace. */
    private ArrayList<LcDatum>[] m_dataLists;

    /** Length of "m_dataLists". */
    private int n_columns;

    /** Map trace index to "m_dataLists" index. */
    private int[] m_trace2column;

    /** Map "m_dataLists" index to trace index. */
    private int[] m_column2trace;

    /** The maximum value in each data list. */
    private double[] m_maxvals;

    /** The minimum value in each data list. */
    private double[] m_minvals;


    /**
     * General purpose constructor.  Some, but not all, of the arguments
     * may be null.  The simplest call just specifies the number of data lists:
     * <pre>
     * lcdata = new LcData(null, null, new int[ntraces]);
     * </pre>
     * @param col2trace The i'th element gives the trace index of the
     * i'th data list. Defaults to col2trace[i] = i.
     * @param trace2col The i'th element gives the list index of trace
     * with index "i".  Traces that have no data are assigned to list -1.
     * Defaults to trace2col[i] = i.
     * @param capacities The initial capacity of each data list. Defaults
     * to 0.
     */
    public LcData(int[]col2trace, int[] trace2col, int[] capacities) {
        init(col2trace, trace2col, capacities);
    }

    /**
     * Convenience constructor, make ntraces channels of length ndats.
     * @param ndats The expected number of data points in every trace.
     * @param ntraces The number of traces.
     */
    public LcData(int ndats, int ntraces) {
        m_dataLists = getArrayListArray(ntraces);
        for (int i = 0; i < ntraces; i++) {
            m_dataLists[i] = new ArrayList<LcDatum>(ndats);
        }
        // TODO: Use active_traces to make effecient table mappings
        m_column2trace = m_trace2column = getDefaultTraceTable(ntraces);
    }

    /**
     * Initializes all the class member variables.  Arguments are the
     * same as for {@link #LcData(int[], int[], int[])}.
     */
    private void init(int[]col2trace, int[] trace2col, int[] capacities) {
        int ncols;

        // Calculate or default any unspecified arguments
        if (col2trace == null && trace2col == null) {
            ncols = capacities.length;
            col2trace = getDefaultTraceTable(ncols);
            trace2col = getDefaultTraceTable(ncols);
        }
        if (col2trace == null) {
            col2trace = invertTrace2Col(trace2col);
        }
        if (trace2col == null) {
            trace2col = invertCol2Trace(col2trace);
        }
        ncols = col2trace.length;
        if (capacities == null) {
            capacities = new int[ncols];
            for (int i = 0; i < ncols; i++) {
                capacities[i] = 0;
            }
        }

        // Initialize the member variables
        m_dataLists = getArrayListArray(ncols);
        m_minvals = new double[ncols];
        m_maxvals = new double[ncols];
        for (int i = 0; i < ncols; i++) {
            m_dataLists[i] = new ArrayList<LcDatum>(capacities[i]);
            m_minvals[i] = m_maxvals[i] = 0;
        }
        m_trace2column = trace2col;
        m_column2trace = col2trace;
    }

    /**
     * Used to initialize an array of ArrayList<LcDatum>'s.
     * The "ArrayLists" are actually null.
     * @param n The size of the array to initialize.
     * @return An Array of n "nulls".
     */
    @SuppressWarnings("unchecked")
    private static ArrayList<LcDatum>[] getArrayListArray(int n) {
        return new ArrayList[n];
    }

    /**
     * @param trace2col The i'th element gives the list index of trace
     * with index "i".  Traces that have no data have value "-1". If
     * there are "n" non-negative elements, exactly one must have each
     * of the values (0, 1, ... n-1).  I.e., no "holes" in the column
     * assignments; no "unused" columns.
     * @return The column-to-trace mapping.  The i'th element gives
     * the trace index of the i'th data list.
     */
    private static int[] invertTrace2Col(int[] trace2col) {
        if (trace2col == null) {
            return null;
        }
        int ntraces = trace2col.length;
        int maxcol = -1;
        for (int i = 0; i < ntraces; i++) {
            if (trace2col[i] >= maxcol) {
                maxcol = trace2col[i];
            }
        }
        int ncols = maxcol + 1;
        int[] col2trace = new int[ncols];
        // Initialize all elements to -1 in case some columns are not
        // used.  (This would not be normal.)
        for (int i = 0; i < ncols; i++) {
            col2trace[i] = -1;
        }
        for (int i = 0; i < ntraces; i++) {
            int j = trace2col[i];
            if (j >= 0) {
                col2trace[j] = i;
            }
        }
        // Now check that we used all the columns
        for (int i = 0; i < ncols; i++) {
            if (col2trace[i] < 0) {
                col2trace = null;
                Messages.postError("Illegal LcData trace to column mapping: "
                                   + arrayToString(trace2col));
                break;
            }
        }

        return col2trace;
    }

    /**
     * @param col2trace The column-to-trace mapping.  The i'th element gives
     * the trace index of the i'th data list. All elements must be >= 0;
     * @return The trace-to-column mapping. The i'th element gives the
     * list index of trace with index "i".  Traces that have no data
     * have value "-1".  Returns null if col2trace has any negative
     * elements.
     */
    private static int[] invertCol2Trace(int[] col2trace) {
        if (col2trace == null) {
            return null;
        }
        int ncols = col2trace.length;
        int maxtrace = -1;
        for (int i = 0; i < ncols; i++) {
            if (maxtrace < col2trace[i]) {
                maxtrace = col2trace[i];
            } else if (col2trace[i] < 0) {
                Messages.postError("Negative trace in "
                                   + "LcData column to trace mapping: "
                                   + arrayToString(col2trace));
                return null;
            }
        }
        int ntraces = maxtrace + 1;
        int[] trace2col = new int[ntraces];
        for (int i = 0; i < ntraces; i++) {
            trace2col[i] = -1;
        }
        for (int i = 0; i < ncols; i++) {
            trace2col[col2trace[i]] = i;
        }
        return trace2col;
    }

    private static String arrayToString(int[] iarray) {
        StringBuffer sb = new StringBuffer("[");
        for (int i = 0; i < iarray.length; i++) {
            if (i == 0) {
                sb.append("" + iarray[i]);
            } else {
                sb.append(", " + iarray[i]);
            }
        }
        sb.append("]");
        return sb.toString();
    }

    private static int[] getDefaultTraceTable(int n) {
        int[] traceToColumn = new int[n];
        for (int i = 0; i < n; i++) {
            traceToColumn[i] = i; // Default trace-to-column mapping
        }
        return traceToColumn;
    }

    /**
     * Get an array of data lengths.
     * Array is of length "ntraces", and each element has value "ndats".
     * Used to pass to the LcData constructor to prepare for "ntraces"
     * data streams of equal length.
     * @param ndats The expected number of data points in each trace.
     * @param ntraces The number of traces.
     * @return Array of length "ntraces" with each element equal to "ndats".
     */
    private static int[] getArray(int ndats, int ntraces) {
        int[] rtn = new int[ntraces];
        for (int i = 0; i < ntraces; i++) {
            rtn[i] = ndats;
        }
        return rtn;
    }

    /**
     * Sets up the mapping of trace number to dataset number.
     * We pass it two parameters.  The first gives the traces that
     * come from various standard detectors.  (The data for some
     * of these traces may actually come from the ADC.)
     * The second says which traces come from ADC's.  (This could
     * include non-standard detectors not in the first list.)
     *
     * @param detTraces An array of arrays of trace numbers. Each
     * non-negative element gives the trace# for a detector
     * channel. Trace numbers start from 0. The reason for the array
     * of arrays is just caller convenience; there can be a separate
     * array for each detector.
     * @param traceAdcs An array of length equal to the maximum number
     * of traces whose non-negative entries give the ADC numbers for
     * those traces. The i'th array element is the ADC number for the
     * i'th trace. ADC numbers and trace numbers start from 0. A value
     * <0 means the i'th trace does not come from an ADC. E.g., for 5
     * traces, the last from the third ADC, the rest from other
     * sources: traceAdcs={-1,-1,-1,-1,2}.
     */
    static public int[] getTraceToColumnTable(int[][] detTraces,
                                              int[] traceAdcs) {
        int n = traceAdcs.length; // Max number of traces
        boolean[] allTraces = new boolean[n]; // Mark traces in use
        for (int i = 0; i < n; i++) {
            if (traceAdcs[i] >= 0) {
                allTraces[i] = true;
            } else {
                allTraces[i] = false;
            }
        }
        // We've marked all the traces from the ADCs - any additional?
        for (int i = 0; i < detTraces.length; i++) {
            for (int j = 0; j < detTraces[i].length; j++) {
                int k = detTraces[i][j];
                if (k >= 0 && k < n) {
                    allTraces[k] = true; // This trace may not be from ADC
                }
            }
        }

        // Now make a list of the active ones
        int[] rtn = new int[n];
        for (int i = 0, j = 0; i < n; i++) {
            if (allTraces[i]) {
                rtn[i] = j++;
            } else {
                rtn[i] = -1;
            }
        }
        return rtn;
    }

    static public int countActiveTraces(int[] traceToColumn) {
        int n = traceToColumn.length;
        int rtn = 0;
        for (int i = 0; i < n; i++) {
            if (traceToColumn[i] >= 0) {
                rtn++;
            }
        }
        return rtn;
    }

    /**
     * @return The internal table converting trace index to column index.
     */
    public int[] getTraceToColumnTable() {
        return m_trace2column;
    }

    /**
     * Get the number of data channels held in this data set.
     */
    public int getNumberOfChannels() {
        return m_dataLists.length;
    }

    /**
     * Calculate the mean time between data points in a given trace.
     * This is just (tLast - tFirst) / (n - 1).
     * @return The mean time between data points in milliseconds.
     */
    public int getInterval(int trace) {
        trace = m_trace2column[trace];
        if (trace < 0 || m_dataLists == null || trace >= m_dataLists.length) {
            return 0;
        }
        ArrayList list = m_dataLists[trace];
        int n1 = list.size() - 1;
        if (n1 <= 0) {
            return 0;
        }
        long t0 = getColumnFlowTime(trace, 0);
        long t1 = getColumnFlowTime(trace, n1);
        return (int)(t1 - t0) / n1;
    }

    /**
     * Get the number of data points in a given column.
     */
    public int getColumnSize(int column) {
        if (column < 0 || m_dataLists == null || column >= m_dataLists.length) {
            return 0;
        } else {
            return m_dataLists[column].size();
        }
    }

    /**
     * Get the number of data points in a given trace.
     */
    public int getSize(int trace) {
        trace = m_trace2column[trace];
        if (trace < 0 || m_dataLists == null || trace >= m_dataLists.length) {
            return 0;
        } else {
            return m_dataLists[trace].size();
        }
    }

    /**
     * Insert each of the datums in an array.
     */
    public void add(LcDatum[] datums) {
        for (int i = 0; i < datums.length; i++) {
            add(datums[i]);
        }
    }

    /**
     * Insert an LcDatum into the LcData object.
     */
    public void add(LcDatum datum) {
        int iColumn = m_trace2column[datum.getChannel()];
        m_dataLists[iColumn].add(datum);

        double value = datum.getValue();
        if (m_dataLists[iColumn].size() == 1) {
            m_maxvals[iColumn] = m_minvals[iColumn] = value;
        } else if (m_maxvals[iColumn] < value) {
            m_maxvals[iColumn] = value;
        } else if (m_minvals[iColumn] > value) {
            m_minvals[iColumn] = value;
        }
    }

    /**
     * Add data for all columns at a specified flow time.
     * The trace numbers are taken from the "trace table"
     * specified when this LcData object was created.
     * @param timeAndData An array containing both the flow time and the
     * data values. First element is the flow time of the data points in
     * minutes from start of run.  The rest are the values for that time.
     */
    public void add(double[] timeAndData) {
        int ndats = timeAndData.length - 1;
        double[] data = new double[ndats];
        for (int i = 0; i < ndats; i++) {
            data[i] = timeAndData[i + 1];
        }
        add((long)(timeAndData[0] * 60000), 0, data);
    }

    /**
     * Add data for all columns at a specified flow time.
     * The trace numbers are taken from the "trace table"
     * specified when this LcData object was created.
     * @param flowTime The flow time of the data points; ms from start of run.
     * @param clockTime The system time of the data, in ms since 1970.
     * @param data The array of data values, one value for each column.
     */
    public void add(long flowTime, long clockTime, double[] data) {
        int len = data.length;
        for (int i = 0; i < len; i++) {
            int trace = m_column2trace[i];
            add(new LcDatum(m_column2trace[i], flowTime, 0, data[i]));
        }
    }

    /**
     * Add data for three channels to the end of the first three arrays.
     * This is a legacy method specifically for SLIM data.
     * @param flowTime The flow time of the data points; ms from start of run.
     * @param clockTime The system time of the data, in ms since 1970.
     * @param a value of first channel (mv)
     * @param b value of second channel
     * @param c value of third channel
     */
    public void add(long flowTime, long clockTime,
                       double a, double b, double c) {
        if (m_dataLists.length < 3) {
            return;
        }
        double[] d = {a, b, c};
        for (int i = 0; i < 3; i++) {
            LcDatum datum = new LcDatum(i, flowTime, clockTime, d[i]);
            add(datum);
        }
        return;
    }

    /**
     * Returns data from a given column and index.
     * @param column Which column to get data from.
     * @param i The index of the data to get.
     * @return The data Pt.
     */
    public LcDatum getPt(int column, int i) {
        return (LcDatum)m_dataLists[column].get(i);
    }

    public int getTraceNumber(int chan) {
        if (chan > m_dataLists.length || m_dataLists[chan].size() <= 0) {
            return -1;
        } else {
            return ((LcDatum)m_dataLists[chan].get(0)).getChannel();
        }
    }

    /**
     * Returns the data value for a given trace and index.
     * @param trace Which trace to get data from.
     * @param i The index of the data to get.
     * @return The data value.
     */
    public double getValue(int trace, int i) {
        trace = m_trace2column[trace];
        return getColumnValue(trace, i);
    }

    /**
     * Returns the data value for a given column and index.
     * @param column Which column to get data from.
     * @param i The index of the data to get.
     * @return The data value.
     */
    public double getColumnValue(int column, int i) {
        return ((LcDatum)m_dataLists[column].get(i)).getValue();
    }

    public double getValueAt(int trace, long time_ms) {
        trace = m_trace2column[trace];
        int lo = getIndex(trace, time_ms, false);
        if (lo == m_dataLists[trace].size()) {
            return getColumnValue(trace, lo);
        } else {
            double x1 = getColumnFlowTime(trace, lo);
            double y1 = getColumnValue(trace, lo);
            double x2 = getColumnFlowTime(trace, lo + 1);
            double y2 = getColumnValue(trace, lo + 1);
            if (x1 == x2) {
                return (y1 + y2) / 2;
            } else {
                return y1 + (y2 - y1) * (time_ms - x1) / (x2 - x1);
            }
        }
    }

    /**
     * Gets the flow time in ms for a given trace.
     */
    public long getFlowTime(int trace, int i) {
        trace = m_trace2column[trace];
        return getColumnFlowTime(trace, i);
    }
        
    /**
     * Gets the flow time in ms for a given column.
     */
    public long getColumnFlowTime(int column, int i) {
        //ArrayList list = m_dataLists[column];
        return getPt(column, i).getRunTime();
    }

    /**
     * Gets index closest to a given flow time.
     * @param trace The trace to look at (from 0).
     * @param flow_ms The given flow time, measured from 0 at the
     * start of the run.
     * @return The index closest to the given time.
     */
    public int getIndex(int trace, long flow_ms) {
        trace = m_trace2column[trace];
        return getIndex(trace, flow_ms, true);
    }

    /**
     * Gets index corresponding to a given flow time.
     * If "closestFlag" is true, the point closest to the given time
     * is returned, otherwise the point just below (or at) the given
     * time is returned.
     * @param trace The trace to look at (from 0).
     * @param flow_ms The given flow time, measured from 0 at the
     * start of the run.
     * @param closestFlag True to find closest point, false to take
     * first point before given time.
     * @return The index corresponding to the given time.
     */
    public int getIndex(int trace, long flow_ms, boolean closestFlag) {
        trace = m_trace2column[trace];
        int size = m_dataLists[trace].size();
        int lo = -1;
        int hi = size;
        int mid = 0;
        while ((hi - lo) > 1) {
            mid = (hi + lo) / 2;
            if (flow_ms > getColumnFlowTime(trace, mid)) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        if (lo < 0) {
            return 0;
        } else if (hi >= size) {
            return size - 1;
        } else {
            if (closestFlag) {
                long tlo = getColumnFlowTime(trace, lo);
                long thi = getColumnFlowTime(trace, lo + 1);
                if (thi - flow_ms < flow_ms - tlo) {
                    ++lo;
                }
            }
            return lo;
        }
    }

    /**
     * Gets the clock time on channel 0 for a given point.
     * @param i The index of the point.
     * @return The clock time in ms since 1970.
     */
    public long getClockTime(int i) {
        return getClockTime(0, i);
    }

    /**
     * Gets the clock time for a given point on a given trace.
     * @param trace The trace to look at.
     * @param i The index of the point.
     * @return The clock time in ms since 1970.
     */
    public long getClockTime(int trace, int i) {
        trace = m_trace2column[trace];
        return getPt(trace, i).getClockTime();
    }

    /**
     * Get the minimum value in a trace.
     */
    public double getMin(int trace) {
        int idx = m_trace2column[trace];
        return (idx < 0 || idx >= m_minvals.length) ? 0 : m_minvals[idx];
    }


    /**
     * Get the maximum value in a trace.
     */
    public double getMax(int trace) {
        int idx = m_trace2column[trace];
        return (idx < 0 || idx >= m_maxvals.length) ? 0 : m_maxvals[idx];
    }

    /**
     * Returns a String containing all the data, one triple per line.
     */
    public String toStringln() {
        StringBuffer sb = new StringBuffer("LcData:\n");
        int n = m_dataLists[0].size();
        for (int i=0; i<n; i++) {
            LcDatum lcd = getPt(0, i);
            sb.append(lcd.getClockTime() + ", "
                      + lcd.getRunTime() + ", ");
            for (int j = 0; j < 3; j++) {
                sb.append(m_dataLists[j].get(i).toString() + "; ");
            }
            sb.append("\n");
        }
        sb.append("\n");
        return sb.toString();
    }
}
