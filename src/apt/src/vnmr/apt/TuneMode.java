/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.io.BufferedReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;

import vnmr.apt.ChannelInfo.SwitchPosition;


/**
 * This class holds information about a tuning mode,
 * for support of multiple personality probes.
 * Contains a list of switch positions to activate this mode
 * and a list of tune channels available with this mode.
 */
public class TuneMode {
    /** Switch position(s) to select this mode. */
    private List<SwitchPosition> m_switchList;

    /** Tune channels available with this mode. */
    private List<ChannelSpec> m_chanList;


    /**
     * Construct a TuneMode given the tune directory and the mode's name.
     * @param tunedir The system tune directory for the current probe.
     * @param name The name of the mode.
     */
    public TuneMode(String tunedir, String name) {
        String path = tunedir + "/mode#" + name;
        readConfigFile(path);
    }

    private boolean readConfigFile(String path) {
        boolean ok = true;
        BufferedReader input = TuneUtilities.getReader(path);
        if (input == null) {
            ok = false;
        }

        m_switchList = new ArrayList<SwitchPosition>();
        m_chanList = new ArrayList<ChannelSpec>();
        String line = "";
        try {
            while (ok && (line = input.readLine()) != null) {
                StringTokenizer toker = new StringTokenizer(line);
                String lowerLine = line.toLowerCase();
                if (lowerLine.startsWith("modeswitch ")) {
                    toker.nextToken(); // Discard key
                    int motor = Integer.parseInt(toker.nextToken());
                    double[] posn = new double[1];
                    posn[0] = Integer.parseInt(toker.nextToken());
                    m_switchList.add(new SwitchPosition(motor, posn));
                } else if (lowerLine.startsWith("chan ")) {
                    toker.nextToken(); // Discard key
                    int ichan = Integer.parseInt(toker.nextToken());
                    double minfreq = Double.NaN;
                    double maxfreq = Double.NaN;
                    if (toker.countTokens() >= 2) {
                        minfreq = 1e6 * Double.parseDouble(toker.nextToken());
                        maxfreq = 1e6 * Double.parseDouble(toker.nextToken());
                    }
                    m_chanList.add(new ChannelSpec(ichan, minfreq, maxfreq));
                }
            }
        } catch (NumberFormatException nfe) {
            Messages.postError("Bad number in config file: \""
                               + path + "\""
                               + ", line: \""
                               + line
                               + "\"");
            ok = false;
        } catch (NoSuchElementException nsee) {
            Messages.postError("Too few fields in line in config file: \""
                               + path + "\""
                               + ", line: \""
                               + line
                               + "\"");
            ok = false;
        } catch (IOException ioe) {
            Messages.postError("Cannot read config file: \""
                               + path + "\"");
            ok = false;
        } finally {
            try { input.close(); } catch (Exception e) {}
        }
        return ok;
    }

    /**
     * @return The list of tune channel specs for this mode.
     */
    public List<ChannelSpec> getChanList() {
        return m_chanList;
    }

}
