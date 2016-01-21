/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.io.File;
import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Map;
import java.util.Map.Entry;
import java.util.TreeMap;

import static vnmr.apt.AptDefs.NL;

public class MotorData {
    private static final String monthTag =
            new SimpleDateFormat("yyyy-MM").format(System.currentTimeMillis());

    private int m_motor = -1;
    private String m_motorDataPath = null;
    private double m_temperature = -999;
    private ArrayList<Datum> m_data = new ArrayList<Datum>();
    private Map<String,String> m_motorFilePaths = new TreeMap<String,String>();
    private Map<String,Histogram> m_stepErrorHistograms =
            new TreeMap<String,Histogram>();
    private Map<String,Histogram> m_reboundHistograms =
            new TreeMap<String,Histogram>();


    public MotorData(String motorDataPath) {
        // File selected by probe name and module S/N
        m_motorDataPath = motorDataPath;
    }

    public void startDataCollection(int motor, int temperature) {
        m_motor = motor;
        m_temperature = temperature;
        m_data.clear();
    }

    private String getFilePath(String fname) {
        String path = m_motorFilePaths .get(fname);
        if (path == null) {
            File ffile = new File(m_motorDataPath, fname);
            try {
                ffile.createNewFile();
                ffile.setReadable(true, false);
                ffile.setWritable(true, false);
                if (ffile.canWrite()) {
                    path = ffile.getAbsolutePath();
                    m_motorFilePaths.put(fname, path);
                }
            } catch (IOException e) {
                Messages.postWarning("Can not write to log file: "
                        + ffile.getAbsolutePath());
                m_motorFilePaths.put(fname, "-");
            }
        }
        return ("-".equals(path)) ? null : path;
    }

    public void recordStepAccuracy(int motor, int step, int err) {
        // TODO: Add TEMPERATURE tag to file name?
        String fname = "StepError" + motor + "_" + monthTag + ".csv";
        String path = getFilePath(fname);
        if (motor != m_motor || path == null) {
            // Do nothing
        } else {
            Histogram hist = getStepErrorHistogram(path);
            int[] counts = hist.getCounts(err);
            int col = hist.getColumnNumber(step);
            counts[col]++;
            TuneUtilities.writeFileSafely(path, hist.toString());
        }
    }

    public void recordRebound(int motor, int step, int rebound) {
        // TODO: Add TEMPERATURE tag to file name?
        String fname = "MotorRebound" + motor + "_" + monthTag + ".csv";
        String path = getFilePath(fname);
        if (motor != m_motor || path == null) {
            // Do nothing
        } else {
            // Get data and list of step distances into memory
            //  (Data for each motor is kept in memory)
            //  Data for motor is map of arrays of counts keyed by "rebound"
            //  Step distance list is array of integers
            Histogram hist = getReboundHistogram(path);
            // Find array starting with given "rebound"
            //  If none, insert array initialized with all 0 counts
            int[] counts = hist.getCounts(rebound);
            // Edit array to increment the appropriate count
            //  Compare "step" to distance list to get array index
            //  Increment that array element
            int col = hist.getColumnNumber(step);
            counts[col]++;
            // Write data back to file (and keep data in memory)
            TuneUtilities.writeFileSafely(path, hist.toString());
        }
    }
    private Histogram getStepErrorHistogram(String path) {
        Histogram hist = m_stepErrorHistograms.get(path);
        if (hist == null) {
            hist = Histogram.readHistogramFromFile(path);
        }
        if (hist == null) {
            int[] bins = {-11, 0, 10};
            hist = new Histogram("Error\t  Requested distance (counts)", bins);
            m_stepErrorHistograms.put(path, hist);
        }
        return hist;
    }


    private Histogram getReboundHistogram(String path) {
        Histogram hist = m_reboundHistograms.get(path);
        if (hist == null) {
            hist = Histogram.readHistogramFromFile(path);
        }
        if (hist == null) {
            int[] bins = {-11, 0, 10};
            hist = new Histogram("Rebound\t  Requested distance (counts)",
                                 bins);
            m_reboundHistograms.put(path, hist);
        }
        return hist;
    }

    public void stopDataCollection(int motor) {
        String fname = "MotorCurrent" + motor + ".csv";
        String path = getFilePath(fname);
        if (motor != m_motor || path == null) {
            // Do nothing
        } else {
            ArrayList<Datum> goodData = trimEndData(m_data);
            if (goodData != null && goodData.size() > 0) {
                Datum result = mean(goodData);
                //result.position = posn;
                //result.distance = nsteps;
                result.date = System.currentTimeMillis();
                result.motor = m_motor;
                result.temperature = (int)Math.round(m_temperature);
                result.direction = getDirection(goodData);
                TuneUtilities.appendLog(path, result.toString(), Datum.HEADER);
            }
        }
    }

    private String getDirection(ArrayList<Datum> data) {
        int delta = data.get(data.size() - 1).position - data.get(0).position;
        return delta > 0 ? "CW" : "CCW";
    }

    private ArrayList<Datum> trimEndData(ArrayList<Datum> data) {
        ArrayList<Datum> noData = new ArrayList<Datum>();
        int ndata = data.size();
        if (ndata < 10) {
            return noData;
        }
        int idx1 = ndata - 1;
        //int p0 = data.get(0).position;
        int plast = data.get(idx1).position;
        int p1 = plast;
        for (--idx1; idx1 >= 0 && Math.abs(p1 - plast) < 200; --idx1) {
            p1 = data.get(idx1).position;
        }
        if (idx1 < 10) {
            return noData;
        }
        ArrayList<Datum> goodData = new ArrayList<Datum>(data.subList(3, idx1));
        return goodData;
    }

    private Datum mean(ArrayList<Datum> data) {
        double current_ma = 0;
        double dutyCycle = 0;
        double rpm = 0;
        int npts = data.size();
        for (Datum datum : data) {
            current_ma += datum.current_ma;
            dutyCycle += datum.dutyCycle;
            rpm += datum.rpm;
        }
        int iposition = data.get(0).position;
        int distance = data.get(npts - 1).position - iposition;
        int icurrent_ma = (int)Math.round(current_ma / npts);
        int idutyCycle = (int)Math.round(dutyCycle / npts);
        int irpm = (int)Math.round(rpm / npts);
        return new Datum(iposition, distance, npts,
                         icurrent_ma, idutyCycle, irpm);
    }
    /**
     * 
     */
    public void addDatum(int position, int current_ma,
                                int duty, int rpm) {
        int distance = 0;
        int npts = 1;
        Datum datum = new Datum(position, distance, npts,
                                current_ma, duty, rpm);
        m_data.add(datum);
    }


    static class Datum {
        protected static final String HEADER =
                "Date               "
                + "\tPosn"
                + "\tDist"
                + "\tNpts"
                + "\tTemp"
                + "\tCurrent"
                + "\tDuty"
                + "\tRPM"
                ;
        private static final DateFormat dateFmt =
                new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        long date;
        int motor;
        int npoints;
        String direction;
        int temperature;
        int position;
        int distance;
        int current_ma;
        int dutyCycle;
        int rpm;

        Datum(int position, int distance, int npts,
              int current_ma, int duty, int rpm) {
            this.position = position;
            this.distance = distance;
            this.npoints = npts;
            this.current_ma = current_ma;
            this.dutyCycle = duty;
            this.rpm = rpm;
        }

        public String toString() {
            StringBuilder sb = new StringBuilder();
            //sb.append(motor);
            //sb.append("\t").append(direction);
            sb.append(dateFmt.format(date));
            sb.append("\t").append(position);
            sb.append("\t").append(distance);
            sb.append("\t").append(npoints);
            sb.append("\t").append(temperature);
            sb.append("\t").append(current_ma);
            sb.append("\t").append(dutyCycle);
            sb.append("\t").append(rpm);
            return sb.toString();
        }
    }


    static class Histogram {
        String header;
        int[] bins;
        TreeMap<Integer,int[]> data = new TreeMap<Integer,int[]>();

        public Histogram(String header, int[] bins) {
            this.header = header;
            this.bins = bins;
        }

        public int[] getCounts(int key) {
            int[] counts = data.get(key);
            if (counts == null) {
                counts = new int[bins.length + 1];
                data.put(key, counts);
            }
            return counts;
        }

        public void putCounts(int key, int[] counts) {
            data.put(key, counts);
        }

        private int getColumnNumber(int dist) {
            int n = bins.length;
            int col;
            for (col = 0; col < n; col++) {
                int bin = bins[col];
                if (dist <= bin) {
                    break;
                }
            }
            return col;
        }

        public String toString() {
            // Write the header
            StringBuilder sb = new StringBuilder(header).append(NL);

            // Write column labels
            int n = bins.length;
            int prevMax = 0;
            for (int i = 0; i <= n; i++) {
                sb.append("\t");
                if (i == 0) {
                    int bin = bins[i];
                    sb.append("<").append(bin + 1); // <-10
                    prevMax = bin;
                } else if (i == n) {
                    sb.append(">").append(prevMax); // >10
                } else {
                    int bin = bins[i];
                    sb.append(prevMax + 1).append(":").append(bin); // 10:0
                    prevMax = bin;
                }
            }
            sb.append(NL);

            // Write all the data
            Entry<Integer,int[]> entry = data.firstEntry();
            while (entry != null) {
                Integer key = entry.getKey();
                sb.append(key);
                int[] counts = entry.getValue();
                for (int count : counts) {
                    sb.append("\t").append(count);
                }
                sb.append(NL);
                entry = data.higherEntry(key);
            }
            return sb.toString();
        }

        private static Histogram readHistogramFromFile(String path) {
            Histogram hist = null;
            String buf = TuneUtilities.readFile(path, false);
            if (buf.length() > 0) {
                String[] lines = buf.split(NL);
                int len = lines.length;
                if (len > 2) {
                    String header = lines[0];
                    int[] bins = getBinsFromLabels(lines[1]);
                    hist = new Histogram(header, bins);
                    for (int i = 2; i < len; i++) {
                        Integer key = getKeyFromLine(lines[i]);
                        int[] counts = getCountsFromLine(lines[i]);
                        if (counts != null) {
                            hist.putCounts(key, counts);
                        }
                    }
                }
            }
            return hist;
        }

        private static int[] getBinsFromLabels(String string) {
            int[] bins = null;
            String[] toks = string.split("\t");
            // NB: Require at least one bin!
            int len = toks.length - 2;
            bins = new int[len];
            for (int i = 1; i <= len; i++) { // Start at second token
                String[] subtoks = toks[i].split("[:<>]");
                String subtok = subtoks[subtoks.length - 1];
                int bin = Integer.parseInt(subtok);
                if (i == 1) {
                    bin--;
                }
                bins[i - 1] = bin;
            }
            return bins;
        }

        private static Integer getKeyFromLine(String string) {
            String[] tokens = string.split("\t");
            return Integer.valueOf(tokens[0]);
        }

        private static int[] getCountsFromLine(String string) {
            int[] counts = null;
            String[] tokens = string.split("\t");
            int len = tokens.length - 1;
            if (len > 0) {
                counts = new int[len];
                for (int i = 0; i < len; i++) {
                    counts[i] = Integer.parseInt(tokens[i + 1]);
                }
            }
            return counts;
        }
    }
}
