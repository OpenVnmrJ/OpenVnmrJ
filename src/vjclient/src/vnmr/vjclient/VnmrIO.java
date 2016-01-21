/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.vjclient;

import java.io.File;

public interface VnmrIO {
	public static final String KEY_VNMR_SYSDIR = "sysdir";
	public static final String KEY_VNMR_USRDIR = "userdir";
	public static final String KEY_INFO2VJ = "TalkToVnmrEnabled";
	final Boolean DEFAULT_INFO2VNMRJ = true;
	final String  DEFAULT_TALK2VNMRJ = "probes" + File.separator + ".talk";
	final String  KEY_TALK2VNMRJ     = "Talk2Vnmr";  // Send2Vnmr pid+port file
	
	static public final int TIMEOUT_VNMRJ = 1000;		   // Send2Vnmr command timeout

	static public final int DEBUG_LEVEL_MASK = 0xf; // To specify level 0-15
	static public final String NL = System.getProperty("line.separator", "\n");

	final String DEFAULT_USRDIR = "vnmrsys";
	final String DEFAULT_SYSDIR = File.separator + "vnmr";
	final String KEY_SYSDIR     = "ProbeIdSysDir";
	final String KEY_USRDIR     = "ProbeIdUsrDir";
	final String DEFAULT_PERSISTENCE_DIR = "persistence";

	/** VJ command to set message to non-acquisition error. */
    static public final String VJ_ERR_MSG = " jFunc(99,'oe') ";

    /** VJ command to set message to non-acquisition information. */
    static public final String VJ_INFO_MSG = " jFunc(99,'oi') ";

    /** VJ command to set message to non-acquisition warning. */
    static public final String VJ_WARN_MSG = " jFunc(99,'ow') ";
}