/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

/**
 * Command interface to ProbeIDIO based on standalone command-line
 * options.
 *  
 * @author Dirk
 *
 * TODO: refactor command processing to use a more consistent
 * interface, i.e. -<cmd> -key <key1> -key <key2> -key <key3>
 * rather than     -<cmd> <key> -opt <key2> -sys
 */
public interface ProbeIdIOCommands {
	final String CMD_EXPORT     = "-export"; // export a table
	final String CMD_IMPORT     = "-import"; // import a legacy file
	final String CMD_ECHO       = "-echo";   // ping the server
	final String CMD_INIT       = "-init";   // initialize the server
	final String CMD_BLOB       = "-blob:";  // a blob command header
	final String CMD_OPTION     = "-opt";    // optional secondary key
	final String CMD_PROBE_NAME = "-cfg";    // probe configuration file name
	final String CMD_SYSTEM     = "-sys";    // search only system files
	final String CMD_USER       = "-usr";    // search only user files
	final String CMD_PROBE_ID   = "-id";     // return ID of attached probe
}
