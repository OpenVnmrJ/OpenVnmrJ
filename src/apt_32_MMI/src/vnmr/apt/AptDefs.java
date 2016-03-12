/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

package vnmr.apt;

import java.awt.Color;
import java.io.FilenameFilter;

public interface AptDefs {

    public static final String UPDATING_OFF = "off";
    // NB: if it's not "on" or "off", we assume "manual".
    //private static final String UPDATING_MANUAL = "manual";
    public static final String UPDATING_ON = "on";

    /** The usual port used to connect to the motor module. */
    public final static int TELNET_PORT = 23;

    /** The port used to connect to the fake, debugging motor module. */
    public final static int DEBUG_MOTOR_PORT = 5000;

    // Debug messages
    static public final int DEBUG_LEVEL_MASK = 0xf; // To specify level 0-15
    static public final int DEBUG_TUNE = 0x10;  // Debug the tune algorithm
    static public final int DEBUG_PERSISTENCE = 0x20;   // Persistence files
    static public final int DEBUG_MTUNE = 0x40;   // Mtune timing

    // Color coded status display definitions
    static public final int STATUS_PREVIOUS = -1;

    static public final int STATUS_READY = 1;
    static public final Color STATUS_READY_COLOR = new Color(180, 255, 180);
    static public final String STATUS_READY_MSG = "Ready";

    static public final int STATUS_RUNNING = 2;
    static public final Color STATUS_RUNNING_COLOR = Color.cyan;
    static public final String STATUS_RUNNING_MSG = "Running";

    static public final int STATUS_NOTREADY = 3;
    static public final Color STATUS_NOTREADY_COLOR = new Color(255, 180, 180);
    static public final String STATUS_NOTREADY_MSG = "Not Ready";

    static public final int STATUS_MLNOTREADY = 4;
    static public final Color STATUS_MLNOTREADY_COLOR = Color.yellow;
    static public final String STATUS_MLNOTREADY_MSG = "No magnet leg I/O";

    static public final int STATUS_INITIALIZE = 5;
    static public final Color STATUS_INITIALIZE_COLOR = Color.yellow;
    static public final String STATUS_INITIALIZE_MSG = "Initializing";

    static public final int STATUS_INDEXING = 6;
    static public final Color STATUS_INDEXING_COLOR = Color.orange;
    static public final String STATUS_INDEXING_MSG = "Indexing...";

    static public final int MAX_STEP_SIZE = 500;

    static public final String NL_PROP = System.getProperty("line.separator");
    static public final String NL = (NL_PROP == null) ? "\n" : NL_PROP;

    static public final String LOAD_CAL_NAME = "CalLoad";
    static public final String OPEN_CAL_NAME = "CalOpen";
    static public final String SHORT_CAL_NAME = "CalShort";
    static public final String PROBE_CAL_NAME = "CalProbe";
    static public final String SIGMA_CAL_NAME = "CalSigma";
    static public final String SPEC_CAL_NAME = "Spec";
    static public final String PHASE_CAL_NAME = "Phase";
    static public final String DELAY_CAL_NAME = "ProbeDelay";
    static public final String DIP_REGION_CAL_NAME = "DipRegion";
    static public final String VERSION_CAL_NAME = "Version";

    /** VJ command to set message to non-acquisition error. */
    static public final String VJ_ERR_MSG = " jFunc(99,'oe') ";

    /** VJ command to set message to non-acquisition information. */
    static public final String VJ_INFO_MSG = " jFunc(99,'oi') ";

    /** VJ command to set message to non-acquisition warning. */
    static public final String VJ_WARN_MSG = " jFunc(99,'ow') ";

    /** Unicode space same width as digit. */
    static public final char DSPC = (char)0x2007;

    /** Degrees symbol */
    static public final String DEG = "\u00b0";

    static public FilenameFilter CHAN_FILE_FILTER
    = new RegexFileFilter("^chan#[0-9]+$");

    static public FilenameFilter MOTOR_FILE_FILTER
    = new RegexFileFilter("^motor#[0-9]+$");
}
