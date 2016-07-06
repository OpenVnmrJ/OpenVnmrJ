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
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.NoSuchElementException;
import java.util.regex.Pattern;

import vnmr.util.Fmt;
import vnmr.util.QuotedStringTokenizer;

public class CalBand implements Comparable<CalBand> {
    
    public static final String SRC_NONE = "";
    public static final String SRC_CAL_EXISTS = "C";
    public static final String SRC_PROBE_BANDS = "P";
    public static final String SRC_GENERIC = "G";

    private double m_start_Hz;
    private double m_stop_Hz;
    private int m_rfchan;
    private String m_name;
    private String m_portName = "";
    private String m_source = SRC_NONE;
    private long m_date = 0;
    private boolean m_isOK = false;


    /**
     * Constructs a CalibrationBand specification from a text string.
     * The format of the string is:
     * <pre> start_MHz stop_MHz rfchan "Band Name" "Port Name"</pre>
     * The double-quotes are required if the band name or port name contains
     * white-space.
     * "Band Name" is the name the band is known by, e.g. "Carbon".
     * "Port Name" is the name of the probe RF port, e.g. "X-Band".
     * If the port name is not specified, it defaults to the same as the
     * band name.
     * @param bandSpec The string specifying the calibration band range.
     * @throws NumberFormatException The start_MHz or stop_MHz is invalid.
     * @throws NoSuchElementException At least Band Name, start, and stop
     * must be specified in the string.
     */
    public CalBand(String bandSpec)
            throws NumberFormatException, NoSuchElementException {
        QuotedStringTokenizer toker = new QuotedStringTokenizer(bandSpec);
        m_start_Hz = 1e6 * Double.valueOf(toker.nextToken());
        m_stop_Hz = 1e6 * Double.valueOf(toker.nextToken());
        m_rfchan = Integer.parseInt(toker.nextToken());
        m_name = toker.nextToken().trim();
        m_isOK = true;
        if (toker.hasMoreTokens()) {
            m_portName = toker.nextToken().trim();
        }
        if (m_portName.length() == 0) {
            m_portName = m_name;
        }
    }

    public CalBand(String spec, String sourceCalFile) {
        this(spec);
        m_source = sourceCalFile;
    }

    public String getSpec() {
        return (getStart() / 1e6) + " " + (getStop() / 1e6) + " " + getRfchan()
                + " \"" + getName() + "\" \"" + getPortName() + "\"";
    }

    public String getSource() {
        return m_source;
    }

    public void setSource(String m_source) {
        this.m_source = m_source;
    }

    /**
     * Get the start frequency for this band.
     * @return Low frequency limit of band in Hz.
     */
    public double getStart() {
        return m_start_Hz;
    }

    /**
     * Get the stop frequency for this band.
     * @return High frequency limit of band in Hz.
     */
    public double getStop() {
        return m_stop_Hz;
    }

    /**
     * Get the name this band is known by.
     * @return The tuning band name.
     */
    public String getName() {
        return m_name;
    }

    /**
     * Get the RF channel number.
     * @return The RF channel number used for this band (from 1).
     */
    public int getRfchan() {
        return m_rfchan;
    }

    /**
     * Get the name of the probe port this band uses.
     * @return The probe name.
     */
    public String getPortName() {
        return m_portName;
    }

    public long getDate() {
        return m_date;
    }

    public void setDate(long date) {
        m_date = date;
    }

    /**
     * Construct a Pattern that matches any calibration file for the
     * frequency range of this calibration.
     * @return The compiled regexp Pattern.
     */
    public Pattern getFilePattern() {
        String strPattern = MtuneControl.getCalNameTemplate(null, null,
                                                            getRfchan(),
                                                            getStart(),
                                                            getStop());
        return Pattern.compile(strPattern);
    }

    public long getFileDate() {
        long date = 0;
        double start = MtuneControl.adjustCalSweepStart(getStart());
        double stop = MtuneControl.adjustCalSweepStop(getStop());
        int rfchan = getRfchan();
        String path = MtuneControl.getExactCalFilePath(start, stop, rfchan);
        if (path != null) {
            File file = new File(path);
            if (file != null) {
                date = file.lastModified();
            }
        }
        return date;
    }

    public String getLabel() {
        long time = getFileDate();
        String strDate = "";
        if (time != 0) {
            DateFormat df = new SimpleDateFormat("yyyy-MM-dd");
            strDate = df.format(new Date(time));
        }
        String range = "";
        String rfchan = "";
        if (getStart() != 0 || getStop() != 0) {
                range = Fmt.f(1, getStart() / 1e6) + " to "
                + Fmt.f(1, getStop() / 1e6) + " MHz";
                rfchan = getRfchan() + "";
        }
        String label = "<html><table><tr>"
                + "<td width=300>" + getName() + ": " + range + "</td>"
                + "<td width=20>" + rfchan + "</td>"
                + "<td width=90>" + strDate + "</td>"
                + "<td width=15>" + getSource() + "</td>"
                + "</tr></table>";
        return label;
    }

    public String getShortLabel() {
        String label = "<html>" // TODO: need html?
                + getName() + ": "
                + Fmt.f(1, getStart() / 1e6) + " to "
                + Fmt.f(1, getStop() / 1e6) + " MHz" 
                + ", rfchan" + getRfchan()
                ;
        return label;
    }

    /**
     * 
     * @return True if this CalibrationBand was successfully initialized.
     */
    public boolean isOK() {
        return m_isOK;
    }

    public int compareTo(CalBand other) {
        double ostart = other.getStart();
        if (m_start_Hz < ostart) {
            return -1;
        } else if (m_start_Hz > ostart) {
            return 1;
        }
        double ostop = other.getStop();
        if (m_stop_Hz < ostop) {
            return -1;
        } else if (m_stop_Hz > ostop) {
            return 1;
        }
        double ochan = other.getRfchan();
        if (m_rfchan < ochan) {
            return -1;
        } else if (m_rfchan > ochan) {
            return 1;
        }
        return 0;
    }

    public boolean equals(CalBand other) {
        return (m_start_Hz == other.getStart() && m_stop_Hz == other.getStop()
                && m_rfchan == other.getRfchan());
    }
}
