/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

import java.io.File;

import vnmr.vjclient.VnmrIO;

/**
 * Note on probe mount points:
 *   Probe was originally mounted on /vnmr/probeid/mnt.  For testing
 *   purposes this mount point can be made a symbolic link to test
 *   probe directories in /vnmr/probeid.
 *   
 *   Since then, the mount point has been moved to /mnt/probe to avolid
 *   the complexities of mounting and unmounting probes during updates
 *   to VnmrJ.  The link in /vnmr/probeid/mnt (i.e. the "logical mount point"
 *   is retained to facilitate testing, and will point to the "physical
 *   mount point', /mnt/probe, during normal operations.
 *   
 * @author dirk
 *
 */
public interface ProbeId {
	// constant declarations
	final Integer DEFAULT_VERBOSITY     = 0;
	final Integer DEFAULT_PORT          = 5004;
	final Integer DEFAULT_MONITOR_INTERVAL = 1000;
	final Integer DEFAULT_IO_TIMEOUT    = 5000;   // NAS server timeout
	final Integer MAX_BUF_SIZE          = 8*1024;
	final Integer ENCRYPTION_STRENGTH   = 256;    // 256 requires export?
	final String DEFAULT_VJ_PROBEID_PARAM = "probeid"; 		   // the VnmrJ variable to change
	final String DEFAULT_TMP_DIR        = "/tmp";
	final String DEFAULT_TMP_FILENAME   = "vj.pio.";
	final String DEFAULT_TMP_EXT        = ".tmp";
	final String DEFAULT_FIFO_IN        = DEFAULT_TMP_DIR + File.separator + "vj.pio.to";    // to server
	final String DEFAULT_FIFO_OUT       = DEFAULT_TMP_DIR + File.separator + "vj.pio.from";  // from server
	final String DEFAULT_SERVER_PIDFILE = DEFAULT_TMP_DIR + File.separator + "vj.pio.pid";   // Probe Server lock file
	final String DEFAULT_SERVER_LOCKFILE= DEFAULT_TMP_DIR + File.separator + "vj.pio.lock";  // Probe Server I/O lock
	final String DEFAULT_CIPHER         = "AES";               // DES is obsolete
	final String DEFAULT_ENCRYPT        = "true";
	final String DEFAULT_KEYFILE        = "ProbeId.key";
	final String DEFAULT_PROBEID_PERSIST_DIR = "probeid";
	final String DEFAULT_STATE_FILENAME = "probeid";
	final String DEFAULT_FLUSH_FILENAME = ".flush";            // record time stamp of last cache flush
	final String DEFAULT_PROBEID_ROOT   = VnmrIO.DEFAULT_SYSDIR + File.separator + "probeid";
	final String DEFAULT_CACHE          = DEFAULT_PROBEID_ROOT + File.separator + "cache";
	final String DEFAULT_LOGICAL_MNT    = DEFAULT_PROBEID_ROOT + File.separator + "mnt";
	final String DEFAULT_PHYSICAL_MNT	= "/mnt/probe";        // file system mount point
	final String DEFAULT_DB             = DEFAULT_LOGICAL_MNT;
	final String DEFAULT_NIC            = "eth0"; 			   // TODO: get programmatically
	final String DEFAULT_ZIP_ENTRY      = "Spec";
	final String DEFAULT_CFG_DIR        = "probes";
	final String DEFAULT_CFG_TEMPLATE   = "probe.tmplt";       // legacy probe template file name
	final String DEFAULT_TRASH          = ".trash";            // where names of deleted files are stored
	final String DEFAULT_CAL_TARGET_SPEC= "Targets";
    final String DEFAULT_TUNE_DIR       = "tune";              // TODO: add to a common protune/probeid lib
    final String DEFAULT_TUNE_PREFIX    = "tunecal_";
	final Boolean DEFAULT_CFG_CHECK     = false;               // validate probe configuration (probe attached, matched)
	final Boolean DEFAULT_DELETE_POLICY = false;               // false=delete-through, true=use trash
	final Boolean INFORM_VNMRJ          = true;                // send error and info msgs to VnmrJ
	final String KEY_FILE               = "ProbeId.key";
	final String NULL_CIPHER            = "";
	final String CMD_READ               = "read ";
	final String CMD_WRITE              = "write ";
	final String CMD_QUIT               = "quit ";
	final String CMD_START              = "start";
	
	// strings returned to user or specified by user to distinguish user from system or factory files
	final String USER_LABEL             = "user";              // user tables or records
	final String SYSTEM_LABEL           = "system";            // system tables or records
	final String FACTORY_LABEL          = "uncalibrated";      // to avoid temptation to treat factory probe files as superior

	final String FACTORY_PREFIX         = "$";                 // indicate factory file
	final String WILDCARD_PREFIX        = "*";                 // indicate user, factory, system search path
	final String FACTORY_DIR            = "Varian";            // top-level factory directory
    final String FACTORY_CALIBRATION    = "parameters";        // top-level factory calibration file
    final String FACTORY_TUNING         = "Tune";              // top-level factory probe tuning file
	final String FACTORY_SAFETY         = "safety_levels";     // top-level factory safety_level specs
	final String KEY_SEPARATOR          = "_";
	final String PARAM_KEY_PROBEID      = "ProbeID";           // Probe file key for unique Probe ID
	final String KEY_PROBEID            = "ProbeId";           // search key for the Probe ID file
	final String KEY_PROBEID_INFO2VJ    = VnmrIO.KEY_TALK2VNMRJ;  // send info and error msgs to VnmrJ
	final String KEY_PROBEID_ENCRYPT    = "ProbeIdEncrypt";    // whether to encrypt Probe ID files
	final String KEY_PROBEID_CIPHER     = "ProbeIdCipher";     // Probe ID file encryption type
	final String KEY_PROBEID_PASSWORD   = "ProbeIdPassword";   // for encrypted files
	final String KEY_PROBEID_USEPROBEID = "UseProbeId";        // whether to use Probe ID files
	final String KEY_PROBEID_USECACHE   = "ProbeIdCache";      // whether to cache probe ID files locally
	final String KEY_PROBEID_MOUNTPOINT = "ProbeIdLogicalMnt"; // i.e. /mnt/probe_mnt/
	final String KEY_PROBEID_MNT        = "ProbeIdPhysicalMnt";
	final String KEY_PROBEID_LOCAL      = "ProbeIdLocal";      // whether to use client/server mode
	final String KEY_PROBEID_ETHERPORT  = "ProbeIdEtherPort";  // Ethernet port to use for host id
	final String KEY_PROBEID_CACHE      = "ProbeIdCache";      // root directory of cache file system
	final String KEY_PROBEID_REMOTE     = "ProbeIdRemote";     // address of the ProbeIdDb Server (not used)
	final String KEY_PROBEID_PORT       = "ProbeIdPort";       // ProbeIdDb Server port (not used)
	final String KEY_PROBEID_DBDIR      = "ProbeIdDbDir";      // root directory of ProbeIdDb
	final String KEY_PROBEID_KEYFILE    = "ProbeIdKeyFile";    
	final String KEY_PROBEID_KEYLENGTH  = "ProbeIdKeyLength";
	final String KEY_PROBEID_ZIPENTRY   = "ProbeIdZipEntryId"; // Mtune's default ZIP file entry
	final String KEY_PROBEID_VALIDATE   = "ProbeIdValidate";   // Validate legacy against Probe ID
	final String KEY_PROBEID_VERBOSE    = "ProbeIdVerbose";
	final String KEY_PROBEID_CFG        = "ProbeIdProbeConfig";// Probe configuration directory name
	final String KEY_PROBEID_CHECK_CFG  = "ProbeIdCheckConfig";// Validate probe configuration (inserted, match)
	final String KEY_PROBEID_TRASH      = "ProbeIdTrash";      // Probe cache trash sub-directory name
	final String KEY_PROBEID_CACHE_POL  = "ProbeIdTrashPolicy";// Probe cache delete policy
	final String KEY_PROBEID_CAL_TARGET_SPEC = "ProbeIdCalTargetSpec"; // Marketing spec numbers
	final String KEY_PROBEID_MONITOR_INTERVAL = "ProbeIdMonInterval";// Check Probe ID interval
    final String KEY_PROBEID_TUNE_DIR   = "ProbeIdTuneDir";    // Protune tuning directory
    final String KEY_PROBEID_TUNE_PREFIX= "ProbeIdTunePrefix"; // Protune tuning sub-directory prefix
	final String KEY_VALUE_FORMAT       = "%-17s  %s";         // Probe file key/value pair format
	final String KEY_VALUE_LIST_SEPARATOR = ","; 			   // separator for groups of key/value pairs
	final String KEY_VALUE_NUCLEUS_DELIM  = ":";               // delimiter for nucleus header (Probe:, H1:, etc.)
	final String KEY_VALUE_PAIR_SEPARATOR = "="; 			   // separator for key/value
	final String OPTIONS_SEPARATOR      = "-";   			   // separator for command-line options
	final String MSG_SERVER_EXITING     = "Probe ID server is exiting...";
	static final String iam = "Probe Server: ";
}
