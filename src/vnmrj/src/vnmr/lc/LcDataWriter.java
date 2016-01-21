/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.io.*;

import vnmr.util.DebugOutput;
import vnmr.util.Fmt;
import vnmr.util.Messages;


/**
 * This class writes the LC chromatogram data to file.
 * It's basic function is to merge chromatogram data from disparate
 * sources, interpolating values to give all data streams the same
 * time base.
 * All it needs to know to do this is (1) the desired sampling frequency
 * (ms between output points) and (2) which traces to expect data for.
 * As soon as it gets data for all traces up through time T, it writes
 * out the data for all traces up through that time.
 *<p>
 * Note: All the data times here are in ms -- until we write
 * out the data.  Then the time is converted to minutes.
 */
public class LcDataWriter implements LcDef {

    private LcData m_data;
    private long m_dataInterval_ms;
    private String m_filepath;

    private int m_maxTraces;      // Max number of traces and data columns
    private int[] m_trace2Column; // length = m_maxTraces
    private int m_nTraces;      // Number of active traces (Ie, they have data)
    private long[] m_lastTime_ms; // length = m_nTraces
    private long m_nextTime_ms; // The next data time to write out


    /**
     * Normal constructor.
     * @param data The complete input data set, up to the present time.
     * @param dt The desired sampling interval in ms.
     * @param filepath The full path to the data file to write.
     */
    public LcDataWriter(LcData data, int dt, String filepath) {
        m_data = data;
        m_dataInterval_ms = dt;
        m_filepath = filepath;
        m_nextTime_ms = 0;  // First data to write is at t=0;
        setActiveTraces();
    }

    /**
     * Get the interval between data points for these data.
     * @return The interval in ms.
     */
    public long getDataInterval() {
        return m_dataInterval_ms;
    }

    /**
     */
    private void setActiveTraces() {
        m_trace2Column = m_data.getTraceToColumnTable();
        m_nTraces = LcData.countActiveTraces(m_trace2Column);
        m_maxTraces = m_trace2Column.length; // Max number of traces

        m_lastTime_ms = new long[m_nTraces];
        for (int i = 0, j = 0; i < m_nTraces; i++) {
            m_lastTime_ms[i] = -1; // Don't even have time 0 yet
        }
    }

    /**
     * Call this to notify this object that new data is available.
     * Examines the data to see if it is now possible to write out
     * one or more lines.  If some data has already been written,
     * appends the new data, otherwise overwrites any old file.
     * @param newData An array containing the new data points, so
     * we can quickly decide if this new stuff makes it possible
     * to write additional output.  If null, we can do without it.
     */
    public void appendData(LcDatum[] newData) {
        // Append iff m_nextTime_ms > 0
        appendData(newData, m_nextTime_ms > 0);
    }

    /**
     * Call this to notify this object that new data is available.
     * Examines the data to see if it is now possible to write out
     * one or more lines.
     * @param newData An array containing the new data points, so
     * we can quickly decide if this new stuff makes it possible
     * to write additional output.  If null, we can do without it.
     * @param append If true, append to existing data, otherwise
     * start a new data file - wiping out any previous data.
     */
    synchronized public void appendData(LcDatum[] newData, boolean append) {
        // Update data trackers; see if we can write anything.
        int len = newData.length;
        for (int i = 0; i < len; i++) {
            int iColumn = m_trace2Column[newData[i].getChannel()];
            m_lastTime_ms[iColumn] = newData[i].getRunTime();
        }
        long lastTime = m_lastTime_ms[0];
        for (int i = 1; i < m_nTraces; i++) {
            if (lastTime > m_lastTime_ms[i]) {
                lastTime = m_lastTime_ms[i];
            }
        }

        if (lastTime > m_nextTime_ms) { // TODO: Should this be ">=" ?
            // Need to write some data

            // For each trace, interpolate all the points from m_nextTime_ms
            // to <= lastTime
            int npts = 1 + (int)((lastTime - m_nextTime_ms)
                                 / m_dataInterval_ms);
            if (DebugOutput.isSetFor("lcDataWriter")) {
                if (npts > 1) {
                    System.err.println("npts=" + npts + ", m_nextTime_ms="
                                       + m_nextTime_ms);
                }
            }
            long[] time = new long[npts];
            for (int i = 0; i < npts; i++) {
                time[i] = m_nextTime_ms + i * m_dataInterval_ms;
            }
            double[][] data = new double[m_nTraces][npts]; // Zeros all data
            for (int i = 0; i < m_nTraces; i++) {
                data[i] = interpolateData(i, npts);
            }

            writeData(time, data, append);
            m_nextTime_ms += npts * m_dataInterval_ms;
        }
    }

    /**
     * 
     */
    private double[] interpolateData(int iColumn, int npts) {
        double[] data = new double[npts];

        int size = m_data.getColumnSize(iColumn);
        if (size == 1) {
            // Everything at beginning just gets initial value
            double v = m_data.getColumnValue(iColumn, 0);
            for (int i = 0; i < npts; i++) {
                data[i] = v;
            }
        } else if (size > 1) {
            // Linear interpolation
            double[] y = {m_data.getColumnValue(iColumn, size - 2),
                          m_data.getColumnValue(iColumn, size - 1)};
            long[] t = {m_data.getColumnFlowTime(iColumn, size - 2),
                        m_data.getColumnFlowTime(iColumn, size - 1)};
            // Back up through the data array
            int j = size - 2;   // Index in data of t[0], y[0]
            // Init ti to last time we want and step backwards:
            long ti = m_nextTime_ms + (npts - 1) * m_dataInterval_ms;
            for (int i = npts - 1; i >= 0; --i, ti -= m_dataInterval_ms) {
                while (t[0] > ti) {
                    if (j == 0) {
                        // It's before first data point; use first value
                        data[i] = y[0];
                        break;
                    } else {
                        --j;
                        t[1] = t[0];
                        t[0] = m_data.getColumnFlowTime(iColumn, j);
                        y[1] = y[0];
                        y[0] = m_data.getColumnValue(iColumn, j);
                    }
                }
                Messages.postDebug("lcDataInterp", "t0=" + t[0]
                                   + ", ti=" + ti + ", t1=" + t[1]);
                data[i] = y[0] + (ti - t[0]) * (y[1] - y[0]) / (t[1] - t[0]);
                if (Double.isInfinite(data[i]) || Double.isNaN(data[i])) {
                    Messages.postError("Trouble interpolating data point: "
                                       + "t[0]=" + t[0] + ", t[1]=" + t[1]);
                    data[i] = (y[1] + y[0]) / 2;
                }// TODO: Need to fix this if it can happen
            }
        }
                
        return data;
    }

    synchronized public void writeData(long[] time,
                                       double[][] data, boolean append) {
        // Open file
        BufferedWriter fw = null;
        try {
            fw = new BufferedWriter(new FileWriter(m_filepath, append));
        } catch (IOException ioe) {
            Messages.postError("Cannot write LC Data file: " + m_filepath);
            return;
        }
        PrintWriter pw = new PrintWriter(fw);

        // Write the data
        int npts = time.length;
        int ncols = data.length;
        for (int i = 0; i < npts; i++) {
            pw.print(Fmt.f(5, time[i] / 60000.0, false));
            for (int j = 0; j < ncols; j++) {
                pw.print("," + Fmt.f(5, data[j][i], false));
            }
            pw.println();
        }

        // Close file
        pw.close();
    }
}
