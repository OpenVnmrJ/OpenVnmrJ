/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;
import java.io.*;
import java.text.Format;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Date;
import java.util.List;
import java.util.TreeSet;
import java.util.Vector;
import java.util.concurrent.Semaphore;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import vnmr.vjclient.*;
import vnmr.probeid.ProbeIdAppdirs.Appdir;

/**
 * ProbeIdIO is an abstract factory that creates stream 
 * interfaces to the probe database.  That database currently
 * returns "records" that correspond to files in the legacy
 * ProTune system.
 * 
 * Encryption is done in the Probe ID client (i.e. the host)
 * on the premise that everything up to the client is untrusted, 
 * but the client application itself is trusted.
 * 
 * Online vs. Offline processing is currently implicit when a new
 * probe is "attached" in online or offline mode.
 * 
 * FIXME: refactor m_db, m_cache, and m_crypto access to use 
 * a more generic ProbeId m_probeId interface, thereby eliminating 
 * all of the special casing required for these separate objects.
 *
 * FIXME: make ProbeIdCommand a generic control structure that is
 * passed to, at the very minimum, the methods of ProbeIdIO.
 * This will eliminate a lot of the special casing among the
 * call signatures and unify the related logic for online/offline
 * processing.
 * 
 * @author dirk
 */
public class ProbeIdIO extends ProbeIdPipeServer 
	implements ProbeId, ProbeIdIOCommands
{
	private String         m_id       = null; // cached id attached probe
	private File           m_cacheDir = null; // root of probe cache directory
	private ProbeIdClient  m_client   = null; // direct client interface
	private ProbeIdDb      m_db       = null; // direct probe DB interface
	private ProbeIdCache   m_cache    = null; // direct probe cache interface
	private ProbeIdStore   m_probeId  = null; // probe id IO interface
	private ProbeIdMonitor m_monitor  = null; // monitor probe connect/disconnect
	private ProbeIdCrypto  m_crypto   = null; // direct probe cryptographic interface
	private boolean        m_cryptoEnabled = true;
	private boolean        m_cacheEnabled  = true;
	private boolean        m_online   = true; // assume "online" mode by default
	private ProbeIdAppdirs m_appdirs  = null; // appdir search path
	private static boolean m_import   = false;
	private Integer        m_timeout  = null; // I/O timer
	private Status         m_state = Status.RUNNING;
	private enum Status { RUNNING, ERROR };
	private enum Hierarchy { HIER_PROBE, HIER_SYS, HIER_USR };
	
	// for unit testing get around synchronization issues on I/O
	// channels by locking a semaphore
	public static boolean   io_synch = false; // for unit test support
	public static Semaphore io_lock  = null;
	
	ProbeIdIO() {
		super();
		init(System.getProperty(KEY_PROBEID_CIPHER), null);
	}
	
	ProbeIdIO(ProbeIdCmd cmd) {
		super();
		init(System.getProperty(KEY_PROBEID_CIPHER), cmd);
		try {
			String name = this.getClass().getSimpleName();
			process(name, cmd);
		} catch (ProbeIdMismatchException e) {
			error(iam+" actual probe doesn't match expected probe "+e.getMessage());
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			error(iam+" illegal argument");
			e.printStackTrace();
		} catch (IOException e) {
			error(iam+" I/O error "+e.getMessage());
			e.printStackTrace();
		}
	}
	
	public Integer timeout() { return m_timeout; }
	
	public void exit() {
		if (m_monitor != null)
			m_monitor.stop();
	}
	
	public void monitor(String cmd) {
		try { // java version of isnumeric()
			Integer interval = Integer.parseInt(cmd);
			if (interval < 10)    // assume caller meant seconds
				interval *= 1000; // convert to milliseconds
			System.setProperty(KEY_PROBEID_MONITOR_INTERVAL, interval.toString());
			cmd = CMD_START;
		} catch (NumberFormatException ne) {}
		
		if (cmd.equalsIgnoreCase(CMD_START)) {
			if (m_monitor == null) 
				m_monitor = new ProbeIdMonitor(this);
			m_monitor.start();
		} else if (cmd.equalsIgnoreCase("stop")) {
			if (m_monitor != null)
				m_monitor.stop();
		} else
			error("invalid monitor command: "+cmd);
	}
	
	public Thread start(String in, String out, boolean server, Integer pid) {
		// link logical mount point to file system mount point
		String probePath = System.getProperty(KEY_PROBEID_MNT, DEFAULT_PHYSICAL_MNT);
		ProbeIdSimulatedProbe.connect(probePath, false);

		if (server) {
			//assert(m_monitor == null);
			m_monitor = new ProbeIdMonitor(this);
			m_monitor.start();
		}
		Thread thread = super.start(in, out, server);
		if (pid != null) try { // signal indicated process that we're ready
		    Process p = Runtime.getRuntime().exec("kill -USR2 "+pid);
		    p.waitFor();
		    System.err.println("notified "+pid);
		} catch (IOException e) {
		    // TODO Auto-generated catch block
		    e.printStackTrace();
		} catch (InterruptedException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
		
		if (server && thread != null) try {
		    thread.join();                   // blocking call
		} catch (InterruptedException e) {
		    ProbeIdIO.error(iam+"unexpected interrupt "+e.getMessage());
		    e.printStackTrace();
		}

		return thread;
	}
	
	public String getPath()         { return m_probeId.getPath(); }
	public File getPath(String key) { return m_probeId.getPath(key); }
	public File getSystemFile(String sub, String file) {
		String key = ProbeIdDb.getKeyFromHostSubdir(sub, file);
		return getPath(key);
	}
	public File getUserFile(String sub, String file) {
		String key = ProbeIdDb.getKeyFromHostUserFile(sub, file);
		return getPath(key);
	}
	
	/**
	 * Provide a path description based on Probe ID.  Primarily 
	 * intended for informational purposes.
	 * @param key
	 * @return path string
	 */
	public String getPathDescription(String key) {
		return ProbeIdDb.getPathFromKey(null, null, key).getPath();
	}
	
	public ProbeIdDb getDb() { return m_db; }
	
	/**
	 * getBufferedReaderFromKey returns a buffered reader for the
	 * file corresponding to the key.
	 * @param key
	 * @param doCrypto
	 * @param doCache
	 * @param doZip 
	 * @return BufferedReader for the file corresponding to key.
	 */
	private BufferedReader getBufferedReaderFromKey(
			ProbeIdStore db,
			String key, String path,
			boolean crypto, boolean cache)
	{
		InputStream in = getInputStreamFromKey(db, key, path, crypto, cache);
		if (in != null)
			return new BufferedReader(new InputStreamReader(in));
		return null;
	}
	
	private InputStream getInputStreamFromKey(
			ProbeIdStore db,			
			String key, String path,
			boolean crypto, boolean cache)
	{
		InputStream in = null; 
		if (key != null && db != null) {
			try {
				in = db.getInputStreamFromKey(key, cache);
				
				if (in != null && crypto) {
					in = m_crypto.getBufferedReader(in);
					info("crypto <"+key+">");
				}
			} catch (IOException e) {
				error(iam+"I/O error "+e.getMessage());
				e.printStackTrace();
			}
		}
		return in;
	}
	
	/**
	 * return a ZIP stream, searching path parameter for a matching file.
	 * @param file - the name of the file 
	 * @param path - the search path; leading "/" implies absolute path
	 *               rooted at <hostid>.  
	 * @return a ZIP archive stream
	 */
	public ZipInputStream getZipInput(ProbeIdStore db, String file, String path) {
		String key = ProbeIdDb.getKeyFromHostUserFile(file);
		boolean cache  = doCache();
		boolean crypto = doCrypto();
		InputStream in = getInputStreamFromKey(db, key, path, crypto, cache);
		return new ZipInputStream(in); 
	}
	
	private BufferedReader getBufferedReaderFromKey(ProbeIdStore db, String key, String path) {
		boolean cache  = doCache();
		boolean crypto = doCrypto(); 
		return getBufferedReaderFromKey(db, key, path, crypto, cache);
	}

	public BufferedReader getBufferedReaderFromKey(ProbeIdStore db, String key) {
		return getBufferedReaderFromKey(db, key, null);
	}
	
	private BufferedReader getUnencryptedReaderFromKey(ProbeIdStore db, String key, boolean dontBypass) {
		boolean cache  = doCache() && dontBypass;
		return getBufferedReaderFromKey(db, key, null, false, cache);
	}

	private BufferedWriter getBufferedWriterFromKey(ProbeIdStore db, String key, boolean append)
	{               
		OutputStream out = getOutputStreamFromKey(db, key, append);
		if (out != null)
			return new BufferedWriter(new OutputStreamWriter(out));
		return null;
	}
	
	public BufferedWriter getBufferedWriterFromKey(ProbeIdStore db, String key) {
		return getBufferedWriterFromKey(db, key, false);
	}
	
	private OutputStream getOutputStreamFromKey(ProbeIdStore db, 
												String key, boolean append)
	{
		OutputStream out = null;
		try {
			out = db.getOutputStreamFromKey(key, append);
			if (out != null && doCrypto())
				out = m_crypto.getBufferedWriter(out);
		} catch (IOException e) {
			error(iam+"I/O write error "+e.getMessage());
			e.printStackTrace();
		}
		return out;
	}

	private OutputStream getOutputStreamFromKey(ProbeIdStore db, String key) {
		return getOutputStreamFromKey(db, key, false);
	}
	
	public ZipOutputStream getZipOutput(ProbeIdStore db, String name, String subdir) 
		throws IOException 
	{
		String key = ProbeIdDb.getKeyFromHostSubdir(subdir, name);
		return new ZipOutputStream(getOutputStreamFromKey(db, key));
	}
	
	// for equivalence testing between probe id and legacy files
	public boolean compareZip(String name, String dir, File legacy) {
		boolean match = true;
		byte lbuf [] = new byte [1000];
		byte pbuf [] = new byte [1000];
		try {
			ZipInputStream lin = new ZipInputStream(new FileInputStream(legacy));
			ZipInputStream pin = getZipInput(m_probeId, dir, name);
			ZipEntry pe, le;
			while ((le = lin.getNextEntry()) != null) {
				if ((pe = pin.getNextEntry()) == null)
					return false;
				if (!pe.equals(le))
					return false;
				while (lin.read(lbuf) == pin.read(pbuf) && match)
					match &= (lbuf == pbuf);
			}
			match &= (pin.getNextEntry() == null);
		} catch (FileNotFoundException e) {
			error(iam+"I/O error reading zip file "+e.getMessage());
			e.printStackTrace();
			match = false;
		} catch (IOException e) {
			error(iam+"I/O error on zip file "+e.getMessage());
			e.printStackTrace();
			match = false;
		}
		return match;
	}
	
	public boolean getImportProbe()     { return m_import; }	
	public static void setImportProbe() { m_import = true; }

	// getters, setters, and checkers to facilitate unit testing
	public boolean doCache()    { return m_cache != null && m_cacheEnabled; }
	public void enableCache(boolean en) { 
		m_cacheEnabled = en;
		m_probeId = en ? m_cache : m_db;
	}
	public void enableCrypto(boolean en) { m_cryptoEnabled = en; }

	public boolean doCrypto() { return m_crypto != null && m_cryptoEnabled; }
	
	public boolean doRemote() { return m_client != null; }
	
	private void importProbeFile(ProbeIdStore db, File src, String dstKey) throws IOException {
		if (src.exists()) try {
			info("importing " + src.getPath() + " to " + db.getPath(dstKey).getPath());
			assert(src.isFile() && src.canRead());
			BufferedWriter writer = getBufferedWriterFromKey(db, dstKey, false);
			BufferedReader reader = new BufferedReader(new FileReader(src));
			ProbeIdDb.copy(reader, writer);
			info("imported " + src.getPath() + " to " + db.getPath(dstKey).getPath());
		} catch (FileNotFoundException e) {
			// nothing to do if there is no source file
		}
	}

	/**
	 * Import system and user files from typical directories.
	 * @param cfg - legacy probe configuration name, for example "autox400DB"
	 */
	private void importLegacyProbe(ProbeIdStore db, String cfg) {
		String[] paths = getTypicalLegacySubdirectories(cfg);
		boolean[] chooseSystemFile = { true, false };

		for (boolean sys : chooseSystemFile)
			for (String path : paths)
				importProbeFile(db, "*", cfg, path, sys);
	}

	/**
	 * @param cfg
	 * @return
	 */
	public String[] getTypicalLegacySubdirectories(String cfg) {
		String probeDir    = "probes" + File.separator + cfg;
		String tuneCalDir  = "tune" + File.separator + "tunecal_" + cfg;
		String tuneDir     = "tune" + File.separator + cfg;
		// path from "probeid('probedir_opts')" VnmrJ Magical script
		String[] paths = { probeDir, tuneCalDir, tuneDir };
		return paths;
	}
	
	/**
	 * Import probe files for configuration into the probe database.
	 * @param legacy - legacy file to import
	 * @param src    - legacy probe name (leave as null for entire subdir)
	 * @param subdir - subdirectory to import
	 */
	public void importProbeFile(ProbeIdStore db, String legacy, 
				 			    String dst, String subdir,
								Boolean sys) 
	{
		try {
			if (legacy.equals("*")) {
				// get the entire collection of probe configuration files
				Collection<ProbeIdDb.SrcDst> files
					= ProbeIdDb.SrcDst.getImportFiles(legacy, subdir, dst, sys);
				for (ProbeIdDb.SrcDst file : files)
					importProbeFile(db, file.src, file.dst);
			} else {
				File src = new File(legacy);
				String dstKey = ProbeIdDb.getKeyFromHostSubdir(subdir, dst, sys);
				importProbeFile(db, src, dstKey);
			}
		} catch (IOException e) {
			error("ProbeIdIO: File I/O error reading legacy file "
					+legacy+" "+e.getMessage());
			e.printStackTrace();
		}
	}

	/**
	 * Export a probe file to stdout (for redirection to a file 
	 * from command-line).  For development and maintenance only.
	 * TODO: require a password!
	 * 
	 * Example: to print the contents of the file corresponding to legacy
	 * probe calibration file file /vnmr/probes/autox400DB/autox400DB:
	 * 	 probeid -export autox400DB -opt probes/autox400DB
	 * 
	 * @param legacy - legacy probe file name
	 * @param subdir - subdirectory
	 * @param file   - the primary key
	 * @param sys    - force export of system directory
	 */
	public void exportProbeFile(ProbeIdStore db, String legacy, 
							    String subdir, String file, 
								Boolean sys)
	{
		String searched = "";
		subdir = ProbeIdDb.relativeForm(subdir);
		boolean sysdir = sys != null && sys;
		String cfgFactory = FACTORY_PREFIX + FACTORY_CALIBRATION;
		Appdir[] search = getTemplateKeys(subdir, file, null, sysdir, cfgFactory);
		for (Appdir src : search) {
			if (src != null && src.key != null) {
				BufferedReader reader = getBufferedReaderFromKey(db, src.key);
				if (reader != null) {
					info("path="+db.getPath(src.key));
					print(reader);
					return;
				}
				if (src.key != null) {
					searched += "\t<" + src.owner + ">\n";
					info("path="+db.getPath(src.key));
				}
			}
		}
		System.err.println("no probe file matching:\n" + searched);
	}
	
	private File getCached(ProbeIdStore db, String key) throws IOException {
		if (db instanceof ProbeIdCache)
			return ((ProbeIdCache) db).cache(key);
		//if (doCache()) return m_cache.cache(key);
		return getPath(key);
	}
	
	public File findProbeBlob(ProbeIdStore db, String subdir, String file, Boolean usr, Boolean sys)
	{
		subdir = ProbeIdDb.relativeForm(subdir);
		Appdir[] search = getTemplateKeys(subdir, file, usr, sys, null);
		for (Appdir src : search) {
			if (src != null && src.key != null) {
				File blob = null;
				try {
					blob = getCached(db, src.key);
				} catch (IOException e) {
					error("I/O error accessing BLOB "+subdir+File.separator+file);
					e.printStackTrace();
				}
				if (blob != null && blob.canRead()) 
					return blob;
			}
		}
		return null;
	}
	
	/**
	 * Verify that the BLOB exists is in the supported BLOB areas.
	 * @param subdir
	 * @return
	 */
	public boolean checkBlob(String subdir, String file) {
		File partial = subdir == null ? null : new File(ProbeIdDb.relativeForm(subdir));
		final String blobPath = "tune";
		if (partial == null 
				|| !partial.getPath().startsWith(blobPath)
				&& !(partial.getPath().startsWith("probes") && file.endsWith(".RF"))
				&& !(partial.getPath().startsWith("probes") && file.endsWith(".DEC"))
				)
		{
			error(iam+subdir+" is an invalid BLOB specification");
			return false;
		}
		return true;
	}
	
	/**
	 * Tuning files are treated as BLOBs (Binary Large OBjects).
	 * Export for writing ("a+" mode) returns either a system or
	 * a user BLOB, but there is not search order implied.
	 * @param legacy
	 * @param subdir
	 * @param file
	 * @param sysdir
	 */
	public void exportProbeBlob(ProbeIdStore db, String legacy,
			String subdir, String file, Boolean usr, Boolean sys, String mode)
	{
		String handle = "";
		/* FIXME: "move" the BLOB to a directory external to the Probe ID
		 * 		  file system and then move it back when the BLOB stream
		 *        is closed in order to prevent modification to a BLOB from
		 *        corrupting the rest of the Probe ID database.
		 *        OK to export a read-only hard link to the files (supported
		 *        under both Linux and Windows).
		 */
		if (checkBlob(subdir,file)) { // enforce blobbiness
			File blob = findProbeBlob(db, subdir, file, usr, sys);
			boolean sysdir = (sys != null) && sys.booleanValue();
			if (mode.equals("a+")) {
				blob =sysdir ? getSystemFile(subdir, file) 
						     : getUserFile(subdir, file);
				if (!blob.getParentFile().exists() && !blob.getParentFile().mkdirs()) {
					error(iam+"couldn't create BLOB path "+blob.getParent());
					blob = null;
				}
			}
			if (blob != null) {
				info("exporting handle to "+blob.getPath());
				handle = blob.getPath();
			}
		}
		print(handle); // send the handle back to the client
	}
	
	/**
	 * commit changes to a Binary Large OBject
	 */
	public void commitBlob(ProbeIdStore db, String srcPath,
						   String subdir, String file, Boolean sys) 
	{
		// check for blobbiness
		boolean usrdir = !(sys != null && sys);   // can't commit to both
		if (!checkBlob(subdir,file)) {
			print("0");
			return;
		}
		File oldBlob = findProbeBlob(db, subdir, file, usrdir, sys);
		if (oldBlob.canWrite()) {
			error(iam+subdir+File.separator+file
					+" can't be commited because is hasn't been exported");
			print("0");
			return;
		}
		File dst = new File(oldBlob.getPath());
		try {
			File src = new File(srcPath);
			if (!ProbeIdDb.copy(src, dst))
				error(iam+"commit failed ");
		} catch (IOException e) {
			error(iam+"could not rename "+subdir+File.separator+file
					+ " " + e.getMessage());
			e.printStackTrace();
			print("0");
		}
		// make the handle writable again
		assert(dst.setWritable(true)); // FIXME: make this recursive when blob is a directory
		
		// notify caller that all is well
		print("1");
	}
	
	/**
	 * Import a BLOB to the probe database.
	 * @param src
	 * @param cfg
	 * @param subdir
	 * @param sysdir
	 */
	public void importProbeBlob(ProbeIdStore db, String src,
								String dst, String subdir, Boolean sys) 
	{
		// check for blobbiness
		if (!checkBlob(subdir, dst))
			return;
		importProbeFile(db, src, dst, subdir, sys);
	}
	
	public void commitProbeFile(ProbeIdStore db, String src, 
		 				  	    String cfg, String subdir, Boolean sys)
	{
		File srcFile = new File(src);
		String searched = null;
		if (srcFile.canRead()) {
			String[] keys = getKeys(cfg, subdir, sys);
			for (String key : keys) {
				if (key != null) {
					File dstFile = db.getPath(key); 
					if (dstFile.canWrite()) {
						try {
							importProbeFile(db, srcFile, key);
						} catch (IOException e) {
							error(iam+"couldn't import probe file "+srcFile.getPath());
							e.printStackTrace();
						}
						return;
					}
				}
				searched += "\t<" + key + ">\n";
				info("path="+db.getPath(key));
			}
		}
		System.err.println("no probe file matching:\n" + searched);
	}

	/**
	 * Select a set of keys based on user parameters - either the
	 * usual user + system or the factory file in ${mountpoint}/Varian, 
	 * with the leading '$' that identifies it as a factory file removed,
	 * i.e. $parameters' becomes 'parameters'.  If appdirs is not null
	 * or empty, then search appdirs.
	 * 
	 * Nulls are pushed for keys that aren't used so that the caller can readily
	 * identify which place it was found.
	 * 
	 * @param subdir
	 * @param file
	 * @param sysdir
	 * @return
	 */
	private Appdir[] getTemplateKeys(
			String subdir, String file, Boolean usr, Boolean sys, 
			String factory) 
	{
		Collection<Appdir> keys = new Vector<Appdir>();
		Appdir usrSearch = null;
		Appdir sysSearch = null;
		boolean sysOnly = sys != null && sys;     // search only system directory
		boolean usrdir  = usr != null && usr;     // search only user directory

		// special case where we're only extracting a factory key
		if (file != null && file.startsWith(FACTORY_PREFIX)) {
			String key = ProbeIdDb.getKeyFromProbe(File.separator+subdir, file.substring(1));
			return new Appdir[] { new Appdir(key, "", FACTORY_LABEL) };
		}
		
		// don't search user directory if it isn't specified and system directory is requested
		if (m_appdirs == null && usr == null && !sysOnly || usrdir)
			usrSearch = new Appdir(ProbeIdDb.getKeyFromHostUserFile(subdir, file), 
									ProbeIdDb.getUserDir(), USER_LABEL);		
		keys.add(usrSearch);

		// search for system file by default, or if it is explicitly asked for
		if (m_appdirs== null && sys == null && !usrdir || sysOnly) 
			sysSearch = new Appdir(ProbeIdDb.getKeyFromHostSubdir(File.separator+subdir, file), 
									ProbeIdDb.getSystemDir(), SYSTEM_LABEL);	
		keys.add(sysSearch);
		
		if (m_appdirs!=null)
		    m_appdirs.getAppdirKeys(keys, subdir, file);
		    
		if (factory != null) {
			String key = ProbeIdDb.getKeyFromProbe(File.separator+FACTORY_DIR, factory.substring(1));
			keys.add(new Appdir(key,"",FACTORY_LABEL));
		}
		return (Appdir[]) keys.toArray(new Appdir[keys.size()]);
	}

	public void listProbeFiles(ProbeIdStore db, Collection<String> list, String key, String opt, Boolean sys)
	{
		String cfgdir = System.getProperty(KEY_PROBEID_CFG, DEFAULT_CFG_DIR);

		if (key.matches("connected")) {
			ProbeIdDb.list(list, db.getPath(), cfgdir, sys);
		} else if (key.matches("disconnected") || key.matches("probes")) {
			if (m_cache == null) {
				error("list disconnected probes: cache is disabled");
				return;
			}
			// get a list of disconnected probe IDs (not for user to avoid replication)
			if (sys) m_cache.listProbeIDs(list, key.matches("disconnected"));
		} else if (key.matches("legacy")) {
			// return a list of legacy probe file names (user supplies path in "opt")
		    // TODO: adjust this for appdir mode
			File dir = sys
					? new File(new File(VnmrProxy.getSystemDir()), cfgdir)
					: new File(new File(VnmrProxy.getUserDir()), cfgdir);
			ProbeIdDb.list(list, dir.getPath());
		} else if (key.matches("appdirs")) {
		    Collection<Appdir> appdirs = new Vector<Appdir>();
		    for (Appdir appdir : appdirs)
		        list.add(appdir.key);
		} else {
			// assume the key is the probe ID
			m_cache.list(list, key, cfgdir, sys);
		}
	}

    private void listAppdirProbeFiles(ProbeIdStore db, Collection<Appdir> list, String opt)
    {
        final String cfgdir = System.getProperty(KEY_PROBEID_CFG, DEFAULT_CFG_DIR);
        Collection<Appdir> appdirs = new Vector<Appdir>();
        ProbeIdAppdirs.getAppdirKeys(appdirs, m_appdirs, (String) null, null);
        // put into a temporary list until we filter out records w/o an actual probe file
        for (Appdir appdir : appdirs) {
            Collection<String> probedirs = new Vector<String>();
            m_cache.list(probedirs, db.getProbeId(), cfgdir, appdir.key);
            for (String probecfg : probedirs) {
                String[] path = { cfgdir, probecfg }; // partial path, i.e. 'probes/'+$probefilename
                String probeFileKey = ProbeIdDb.addToKey(appdir.key, path, probecfg);
                // filter for directories w. existing probe files
                if (!ProbeIdDb.exists(db, probeFileKey))
                    continue;
                // filter for "opt", i.e. "w", "unique"
                if (opt != null)
                    if (opt.equals("w") || opt.equals("rw") || opt.equals("rwx")) 
                        if (!ProbeIdDb.canWrite(db, probeFileKey))
                            continue;
                // filter for uniqueness
                if (opt != null && opt.equals("unique"))
                    for (Appdir listed : list)
                        if (listed.key.equals(probecfg))
                            continue;
                list.add(new Appdir(probecfg, appdir.owner, appdir.label));
            }
        }
    }

	/**
	 * list probe files of either the connected probe, legacy probe files,
	 * or a probe with matching ID.  (TODO: make this a file pattern?)
	 * Alternatively, provide a list of all disconnected probes that this 
	 * server knows about.
	 * 
	 * @param key - a probe id, "connected", "disconnected"
	 * @param opt - an sprintf-type format string
	 * @param usr - return only user probe files
	 * @param sys - return only system probe files
	 */
	public void listProbeFiles(ProbeIdStore db, String key, String opt, Boolean usr, Boolean sys) {
		Collection<String> results = new Vector<String>();
        final String fmt = "\"%s\" \"%s\"";
		if (key==null) key = "appdirs";
		
		if (key.equals("disconnected") || key.equals("probes")) {
		    Collection<String> probeList = new TreeSet<String>();
		    listProbeFiles(db, probeList, key, opt, true);
		    for (String probe : probeList)
		        results.add(String.format(fmt, probe, probe));
		    
		} else if (key.equals("connected")) {
			String probe = readProbeId(true);
			if (probe != null)
				results.add(String.format(fmt, probe, probe));
		
		} else if (m_appdirs == null) {
			boolean sysdir = sys != null && sys || usr == null;
			boolean usrdir = usr != null && usr || usr == null && sys == null;
			int comma = opt != null ? ((comma = opt.indexOf(',')) < 0 ? opt.length() :comma) : -1;
			String fmtUsr = opt != null && comma!=-1 
				? opt.substring(0, comma) : "\"%-27s (User)\" \"%s\"";
			String fmtSys = opt != null 
				? (comma < opt.length() ? opt.substring(comma+1) : fmtUsr)
				: "\"%-25s (System)\" \"%s\"";
			Collection<String> usrList = new TreeSet<String>();
			Collection<String> sysList = new TreeSet<String>();
			if (sysdir) 
				listProbeFiles(db, sysList, key, opt, true);
			if (usrdir)
				listProbeFiles(db, usrList, key, opt, false);
			for (String s : sysList)
				results.add(String.format(fmtSys, s, s));
			for (String u : usrList)
				results.add(String.format(fmtUsr, u, u));
		
		} else {
		    // list appdirs probe files
		    Collection<Appdir> list = new Vector<Appdir>();
		    listAppdirProbeFiles(db, list, opt);
		    printAppdirProbes(list);
		    return;
		}
		print(results);
	}
	
	/**
	 * Return selected information from a list of things the probe knows about.
	 * 
	 * @param db  - probe database handle
	 * @param cmd - query command to process
	 * @param connected - whether the probe is currently connected
	 * @throws ProbeIdMismatchException 
	 */
	public Collection<String> query(ProbeIdStore db, ProbeIdCmd cmd, boolean connected) 
		throws ProbeIdMismatchException 
	{
		Collection<String> results = null;
		String query               = cmd.key();
		String opt                 = cmd.nextOpt();

		if (query.equalsIgnoreCase("probecfg")) {
			// get a list of probe configurations ("probes" in legacy terms)
			results = new TreeSet<String>();
			if (db != null) {
                Collection<Appdir> tmp = new Vector<Appdir>();
                ProbeIdAppdirs save = m_appdirs; // TODO: boof this once appdirs are completely in place
                m_appdirs = new ProbeIdAppdirs(ProbeIdDb.getSystemDir(), SYSTEM_LABEL);
                if (opt == null || opt.equals("sys") || opt.equals("system"))
                    listAppdirProbeFiles(db, tmp, null);
                m_appdirs = new ProbeIdAppdirs(ProbeIdDb.getUserDir(), USER_LABEL);
                if (opt == null || opt.equals("usr") || opt.equals("user"))
                    listAppdirProbeFiles(db, tmp, null);
                m_appdirs = save;
                for (Appdir appdir : tmp)
                    results.add(appdir.key);
			}
			info(query + (opt == null ? "" : "," + opt));

		} else if (query.equals("list")) {
			// variation on above - get a formatted list
			results = new TreeSet<String>();
			if (db != null) {
				String subdir = System.getProperty(KEY_PROBEID_CFG, DEFAULT_CFG_DIR);
				Collection<String> usr = new TreeSet<String>();
				Collection<String> sys = new TreeSet<String>();
				ProbeIdDb.list(usr, db.getPath(), subdir, false);
				ProbeIdDb.list(sys, db.getPath(), subdir, true);
				int comma = opt != null ? opt.indexOf(',') : -1;
				String fmtUsr = opt != null ? opt.substring(0, comma) : "%-27s (User)";
				String fmtSys = opt != null ? opt.substring(comma) : "%-25s (System)";
				for (String s : sys)
					results.add(String.format(fmtSys, s));
				for (String u : usr)
					results.add(String.format(fmtUsr, u));
				results.add("\"None\" \"none\"");
			}
			
		} else if (query.equalsIgnoreCase("id")) {
			if (connected) {
				results = new Vector<String>();
				results.add(readProbeId(true));
				info(query + (opt == null ? "" : "," + opt) + 
						"=" + results.iterator().next());
			} else throw new ProbeIdMismatchException("");
		} else if (query.equalsIgnoreCase("info")) {
			if (connected) {
				results = new Vector<String>();
				getProbeIdInfo(results);
		    } else 
		    	throw new ProbeIdMismatchException("");
		} else if (query.equals("appdir")) {
		    results = new Vector<String>();
		    results.add(m_appdirs.appdirs());
		} else { // invalid key - do nothing
			info("unknown query <"+query+(opt == null ? "" : "," + opt)+">");
			return null;
		}
		print(results);
		return results;
	}

	public Collection<String> status(ProbeIdCmd cmd, boolean connected, String probeId) 
		throws ProbeIdMismatchException
	{
		Collection<String> results = null;
		String query = cmd.key();
		String opt   = cmd.nextOpt();
		if (query.equalsIgnoreCase("attached")) {
			results = new Vector<String>();
			results.add(connected ? "1" : "0");
			info(query + "=" + results.iterator().next());
		} else if (query.equalsIgnoreCase("match")) {
			results = new Vector<String>();
			results.add(connected && probeId.equals(m_id) ? "1" : "0");
			info(query + "=" + results.iterator().next());
		} else if (query.equalsIgnoreCase("expected")) {
			results = new Vector<String>();
			results.add(m_id==null ? "" : m_id);
			info(query);
		} else if (query.equalsIgnoreCase("connected")) {
			results = new Vector<String>();
			if (m_cache != null)
				results.add(readProbeId(false));
			else
				results.add("");
			info(query + "=" + results.iterator().next());
		} else if (query.equalsIgnoreCase("cache")) {
			results = new Vector<String>();
			results.add(m_cache.getPath());
			info(query + "=" + results.iterator().next());
		} else if (query.equalsIgnoreCase("cachedir")) {
			results = new Vector<String>();
			results.add(System.getProperty(KEY_PROBEID_CACHE, DEFAULT_CACHE));
			info(query + "=" + results.iterator().next());
		} else if (query.equalsIgnoreCase("mount")) {
			results = new Vector<String>();
			results.add(m_db.getPathName(ProbeIdDb.getKeyFromHost()));
			info(query + "=" + results.iterator().next());
		} else if (query.equalsIgnoreCase("mountdir")) {
			results = new Vector<String>();
			results.add(System.getProperty(KEY_PROBEID_DBDIR, DEFAULT_LOGICAL_MNT));
			info(query + "=" + results.iterator().next());
		} else if (query.equalsIgnoreCase("monitor")) {
			results = new Vector<String>();
			results.add(m_monitor == null 
					? "0" : Integer.toString(m_monitor.getInterval()));
			info(query + "=" + results.iterator().next());
		} else { // invalid key - do nothing
			info("unknown query <"+query+(opt == null ? "" : "," + opt)+">");
			return null;
		}
		print(results);
		return results;
	}

	// print routines allow VNMR magical macros to grab the output from stdout
    private void print(String result) {
        PrintStream out = getOutputStream();
        if (result != null)// && !result.isEmpty())
            out.println(result);
        out.flush();
        if (io_synch)
            io_lock.release();
    }
    
    private void print(String[] results) {
        PrintStream out = getOutputStream();
        for (String result : results)
            out.println(result != null ? result : "");
        out.flush();
        if (io_synch)
            io_lock.release();
    }
    
	private void print(boolean val) { print(val ? "1" : "0"); }
	
	private void print(Collection<String> results) {
		PrintStream out = getOutputStream();
		if (results != null && !results.isEmpty())
			for (String result : results) 
				out.println(result!=null ? result : "");
		out.flush();
		//out.close();
		if (io_synch)
			io_lock.release();
	}
	
	private void printAppdirProbes(Collection<Appdir> results) {
	    PrintStream out = getOutputStream();
	    if (results != null && !results.isEmpty()) {
	        for (Appdir result : results) {
	            String line = result != null 
	                    ? String.format("%-17s %s:%s", result.key, result.owner, result.label)
	                    : "";
	            out.println(line);
	        }
	    }
        //out.close();
        if (io_synch)
            io_lock.release();
	}
	
	private void print(BufferedReader reader) {
		PrintStream out = getOutputStream();
		String line = null;
		int lines = 0;
		try {
			if (reader != null) {
				while ((line = reader.readLine()) != null) {
					out.println(line);
					lines++;
				}
				out.flush();
				info("exported "+Integer.toString(lines)+" records");
			}
		} catch (IOException e) {
			error(iam+"I/O write error on output channel ");
			e.printStackTrace();
		} finally { 
			out.close(); 
		}
	}
	
	/**
	 * get keys for specified probe configuration
	 * @param cfg    - primary key
	 * @param subdir - secondary key
	 * @param sysdir - set to true for system-only files
	 * @return array of keys
	 */
	public static String[] getKeys(String cfg, String subdir, Boolean sysdir) {
		String syskey;
		if (subdir == null)
			syskey = ProbeIdDb.getKeyFromHostFile(cfg);
		else
			syskey = ProbeIdDb.getKeyFromHostSubdir(File.separator+subdir, cfg);
		
		String[] syskeyOnly = { syskey };
		String[] bothkeys = { ProbeIdDb.getKeyFromHostUserFile(subdir, cfg), syskey };
		return sysdir != null && sysdir ? syskeyOnly : bothkeys;
	}
	
	public static String[] getKeysFromPath(String cfg, String[] pathlist) {
		Collection<String> paths = new Vector<String>();
		for (String path : pathlist) {
			String key = ProbeIdDb.getKeyFromHostSubdir(path, cfg);
			paths.add(key);
		}
		return (String[]) paths.toArray(new String[paths.size()]);
	}
	
	public String addparam(ProbeIdStore db, String param, String value, String cfg, String subdir) {
		ProbeIdIO.notYetImplemented("addparam waiting for Database integration");
		return null;
	}
	
	public String getparam(ProbeIdStore db, String param, String cfg, String subdir) {
		String[] keys = getKeys(cfg, subdir, false);
		String result = null;
		for (String key : keys) {
			info("searching key=<"+key+"> for "+param);
			info("path="+db.getPath(key));
			BufferedReader reader = getBufferedReaderFromKey(db, key);
			if (reader != null) {
				// lookup('mfile',$file,'seek',$param,'read'):$ret,$num
				try {
					result = ProbeIdDb.getParam(reader, param);
					info(result);
					break; // only read to the 1st match (for now)
				} catch (IOException e) {
					error(iam+" getparam I/O error "+e.getMessage());
					e.printStackTrace();
				}
			}
		}
		print(result);
		return result;
	}
	
	public void setparams(ProbeIdStore db, String params, String cfg, String subdir) {
		String[] keys = getKeys(cfg, subdir, false);
		Collection<String> result = null;
		for (String key : keys) {
			info("searching key=<"+key+">");
			info("path=" + db.getPath(key));
			BufferedReader reader = getBufferedReaderFromKey(db, key);
			if (reader != null) {
				// lookup('mfile',$file,'seek',$param,'read'):$ret,$num
				try {
					// TODO: writer should be initialized from within setParams, otherwise
					// there is the potential to introduce a subtle ordering bug - read and
					// writer will refer to the same file and writer will truncate file,
					// so writer needs to be initialized after the probeCfg constructor call.
					ProbeIdKeyValueIO probeCfg = new ProbeIdKeyValueIO(reader);
					result = probeCfg.setParams(params);
					if (result != null) {
						BufferedWriter writer = getBufferedWriterFromKey(db, key);
						probeCfg.commit(writer);
						print(result);
					}
					return; // only read to the 1st match
				} catch (IOException e) {
					// TODO print an info message or other optional diag and continue
					e.printStackTrace();
				}
			}
		}
	}
	
	public Boolean checkHost(ProbeIdStore db) {
		String key = ProbeIdDb.getKeyFromHost();
		File file = db.getPath(key);
		return (file.exists() && file.isDirectory());
	}
	
	/**
	 * Check permissions of a database table/record
	 * FIXME: move this to ProbeIdDb
	 * @param db
	 * @param subdir
	 * @param sys
	 * @param initialized
	 * @return
	 */
	private String getperm(ProbeIdStore db, String subdir, Boolean sys, boolean initialized) {
		String rwx = "";
		subdir = ProbeIdDb.relativeForm(subdir);
		String key = subdir != null 
			? ProbeIdDb.getKeyFromHostSubdir(subdir, null, sys)
			: ProbeIdDb.getKeyFromHost();
		File file = db.getPath(key);
		if (file.exists()) {
			if (file.canRead()) rwx += "r";
			if (file.canWrite()) rwx += "w";
			if (file.canExecute()) rwx += "x";
		} else { // subdir doesn't exist yet - return permissions of top
			if (checkHost(db)) { // a valid initialized host
				rwx = getperm(db, null, sys, initialized);
			} else if (initialized) { // host not initialized for this probe
				rwx="rwx"; 	   // TODO: delete this and deal with probe init properly
				error("probe has not been initialized on this host");
			} else
				rwx = "rwx";   // an uninitialized host is free game
		}
		return rwx;
	}
	
	/**
	 * check if requested permissions of rwx argument are satisfied
	 * for the given path.
	 * @param rwx  - r=read/w=write/x=execute permission (in that order)
	 * @param path - path for which we need permissions (i.e. subdirectory)
	 * @param sys  - set to true for system directory
	 * @return 1 if requested permissions are a subset of actual permissions,
	 *         0 otherwise
	 */
	private String[] checkperm(ProbeIdStore db, String rwx, String path, Boolean sys, boolean initialized) {
		String perm = getperm(db, path, sys, initialized);
		Boolean match = perm.contains(rwx);
		String[] results = { match ? "1" : "0", rwx }; 
		print(results);
		return results;
	}

	/**
	 * Read a probe configuration file
	 * @param db - database instance
	 * @param prefix - optional prefix to prepend to each output line
	 * @param cfg - probe configuration file name
	 * @param opt - search path (may be null to specify default search path)
	 * @return a collection of records matching the specified probe file
	 */
	public Collection<String> readfile(ProbeIdStore db, String prefix, String cfg, String opt) {
		String[] keys = null;
		if (opt != null && opt.contains(File.pathSeparator)) //search path
			keys = getKeysFromPath(cfg, opt.split(File.pathSeparator));
		else // the usual user/system dichotomy
			keys = getKeys(cfg, opt, false);

		return readfile(db, keys, prefix);
	}
	
	public Collection<String> readfile(ProbeIdStore db, String[] keys, String prefix) {
		Collection<String> results = null;
		for (String key : keys) {
			BufferedReader reader = getBufferedReaderFromKey(db, key);
			info("searching prefix="+prefix+" key=<"+key+"> crypto="+(doCrypto() ? "true":"no"));
			if (reader != null) {
				try {
					results = ProbeIdDb.readFile(reader, prefix);
					break;  // use the 1st file we find and no others
				} catch (IOException e) {
					error("I/O error accessing key="+key+" "+e.getMessage());
					e.printStackTrace();
				}
			}
		}
		print(results);
		return results;
	}
	
	public void readCalTargets(ProbeIdStore db, String target) {
		if (target==null) 
			target = System.getProperty(KEY_PROBEID_CAL_TARGET_SPEC,
									    DEFAULT_CAL_TARGET_SPEC);
		String subdir = File.separator + FACTORY_DIR;
		String fileId = target;
		String keys[] = { ProbeIdDb.getKeyFromProbe(subdir, fileId) };
		readfile(db, keys, "");
	}
	
	/**
	 * copy a user probe subdirectory to a system probe subdirectory,
	 * providing the functionality of the probelist('copy') macro call.
	 * 
	 * Keeps the user probe to maintain VnmrJ compatibility.
	 * @param userSrc
	 * @param systemTarget
	 */
	public boolean copy(ProbeIdStore db, String src, String dst, String subdir) {
		if (dst == null) dst = src; // copy to system sub-directory of the same name
		// the probe configuration name is buried in the sub-directory path
		// to match the current directory structure.  
		// FIXME: consolidate under the probe configuration key.
		String keySrc = ProbeIdDb.getKeyFromHostUserFile(subdir, null);
		String keyDst = ProbeIdDb.getKeyFromHostSubdir(File.separator+subdir, null);
		try {
			return db.copy(keySrc, keyDst);
		} catch (IOException e) {
			error("Probe cache flush failed");
			e.printStackTrace();
			return false;
		}
	}

	/**
	 * Make a backup copy of a probe configuration.
	 * TODO: Currently saves it in the same directory as the original.
	 *       This will cause the backups to show up in the probe menu.
	 *       Move it to a separate backup directory?  If so, how
	 *       will the user interface with the backups?
	 */
	public boolean backup(ProbeIdStore db, String file, String subdir, Boolean sys)
	{
		// Autobackup magical macro uses time stamp "%Y-%m-%d-%H%M%S"
		Format formatter = new SimpleDateFormat(".yyyy-MM-dd-HHmmss");
		Date date = new Date();
		boolean success = false;
		String timestamp = formatter.format(date);
		String dst = file+timestamp;
		String dstKey = ProbeIdDb.getKeyFromHostSubdir(subdir, dst, sys);
		String srcKey = ProbeIdDb.getKeyFromHostSubdir(subdir, file, sys);
		try {
			success = db.copy(srcKey, dstKey);
		} catch (IOException e) {
			error("Backup of "+file+File.separator+subdir+" failed "+e);
			e.printStackTrace();
		}
		print(success ? dst : "");
		return success;
	}
	
	/**
	 * check whether legacy style probe file contains a nucleus
	 * @param key
	 * @param nucleus
	 * @return
	 * @throws Exception 
	 * @throws IOException 
	 */
	public boolean checkNucleus(ProbeIdStore db, String key, String cfg, String nucleus) throws IOException, Exception {
		BufferedReader reader = getBufferedReaderFromKey(db, key);
		if (ProbeIdDb.getParam(reader, nucleus + ":") != null) 
			throw new Exception("Nucleus "+ nucleus 
								+ " already exists in probe configuration "+cfg);
		reader.close();
		return true;
	}
	
	/**
	 * Add a new table to a probe configuration, traditionally corresponding
	 * to a "nucleus", but can also be ECC (Eddy Current Compensation) values.
	 * @param cfg
	 * @param subdir
	 * @param sys
	 */
	public void addnucleus(ProbeIdStore db, String cfg, String subdir, Boolean sys, String nuc, String[] keyvalues) {
		String key = ProbeIdDb.getKeyFromHostSubdir(subdir, cfg, sys);
		BufferedWriter writer = getBufferedWriterFromKey(db, key, true);
		// check whether nucleus already exists
		try {
			checkNucleus(db, key, cfg, nuc);
			String[] pair = keyvalues[0].split("\\s+", 2); // split around 1st white space
			// check if 
			for (String keyvalue : keyvalues) {
				pair = keyvalue.split("\\s+", 2); // split around 1st white space
				if (pair.length != 2)
					throw new Exception("Malformed key value specification '"
							+ keyvalue + "'");
				key = nuc + pair[0];
				String line = String.format(KEY_VALUE_FORMAT, key, pair[1]);
				writer.append(line+'\n');
			}
		} catch (IOException e) {
			error("addnucleus: I/O error "+cfg+" "+e.getMessage());
			e.printStackTrace();
		} catch (Exception e) {
			error("addnucleus: "+e.getMessage());
			e.printStackTrace();
		}
	}
	
	/**
	 * Create a new configuration from a user, system, or factory
	 * template.  This is always created in the user area and can
	 * then be promoted to the system area.
	 * @param cfg - target probe configuration
	 * @param dst - target probe configuration subdirectory
	 * @param sys - set to true if the system template is to be used
	 */
	public void add(ProbeIdStore db, String cfg, String subdir, Boolean sys) {
		String dstKey = ProbeIdDb.getKeyFromHostSubdir(subdir, cfg, sys);
		String cfgFactory = FACTORY_PREFIX + FACTORY_CALIBRATION;
		Appdir[] hostSearch = getTemplateKeys(DEFAULT_CFG_DIR, DEFAULT_CFG_TEMPLATE, null, sys, cfgFactory);
		boolean probeFilesSuccess = copyCfg(db, dstKey, hostSearch);
		if (!probeFilesSuccess) {
			// there should always be a factory template!
			error("Probe configuration from template failed");
		}
		
        String srcTuneKey = ProbeIdDb.getKeyFromProbe(File.separator+FACTORY_DIR, FACTORY_TUNING);
        String dstTuneTop = System.getProperty(KEY_PROBEID_TUNE_DIR, DEFAULT_TUNE_DIR);
        String dstTuneDir = dstTuneTop + File.separator + cfg;
        String dstTuneKey = ProbeIdDb.getKeyFromHostSubdir(dstTuneDir, null, true); // follows protune and always goes to sysdir
        
        boolean tuneFilesSuccess = false;
        try {
            tuneFilesSuccess = db.copy(srcTuneKey, dstTuneKey);
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
		print(probeFilesSuccess && tuneFilesSuccess);
	}
	
	/**
	 * Append rows to the specified table.  Used by addrow on a legacy
	 * file.  Shouldn't really be used for real database work - it's here
	 * to deal with the apparently unused "safety_levels" files in the
	 * probes subdirectory for backwards compatibility.
	 * @param cfg
	 * @param tbl
	 * @param rows
	 * @param sys
	 */
	public void add(ProbeIdStore db, String cfg, String tbl, String[] rows, Boolean sys) {
		String key = sys != null && sys 
			? ProbeIdDb.getKeyFromHostFile(tbl)
			: ProbeIdDb.getKeyFromHostUserFile(tbl);
		BufferedWriter writer = getBufferedWriterFromKey(db, key, true);
		if (writer != null) {
			try {
				for (String row : rows) {
					info("adding row \""+row+"\"");
					writer.append(row + '\n');
				}
				writer.flush();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}
	
	private boolean copyCfg(ProbeIdStore db, String dstKey, Appdir[] srcSearch) {
		String searched = "";
		for (Appdir src : srcSearch) {
			if (src != null) {
				searched += "\t<" + src.owner + ">\n";
				info("path="+db.getPath(src.key));
				if (src != null && src.key != null) {
					try {
						if (db.copy(src.key, dstKey))
							return true;
					} catch (IOException e) {
						error("Probe configuration initialization failed");
						e.printStackTrace();
					}
				}
			}
		}
		return false;
	}
	
	/**
	 * Delete a probe configuration
	 * @param cfg - probe configuration id
	 * @param sys - true if a system probe
	 * FIXME: check permissions!
	 */
	public void drop(ProbeIdStore db, String cfg, String subdir, Boolean sys) {
		String key = ProbeIdDb.getKeyFromHostSubdir(subdir, null, sys);
		boolean dropped = false;
		try {
			dropped = drop(db, key);
		} catch (IOException e) {
			error("I/O error deleting "+cfg);
		} catch (ProbeIdMismatchException e) {
			error("error deleting "+cfg+": "+e.getMessage());
		}
		print(dropped);
	}
	
	public boolean drop(ProbeIdStore db, String key) throws IOException, ProbeIdMismatchException {
		return db.delete(key);
	}

	/**
	 * Set the indicated parameter in VnmrJ with the current Probe ID.
	 * @param param VnmrJ variable to set
	 * @param tree  parameter tree for specified variable
	 */
	public void setVnmrProbeId(String param, String tree) {
		try {
			String probeId = readProbeId(true);
			String send = "false";//System.getProperty(KEY_PROBEID_INFO2VJ, INFORM_VNMRJ.toString());
			if (param == null || param.isEmpty())
				param = DEFAULT_VJ_PROBEID_PARAM;
			if (probeId == null)
				probeId = "";
			if (send.equals("true")) {
				VnmrProxy.create(param, tree, probeId);      // create probe id param
				VnmrProxy.sendNewProbeNotification(probeId); // update probe id param in case it already exists
			}
			print(probeId);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	/**
	 * Return a key describing a file at a specified level of
	 * hierarchy (user, system, or probe).
	 * @param file
	 * @param level
	 * @return key
	 */
	public String getProbeIdKey(String file, Hierarchy level) {
		String path = null;
		switch (level) {
		case HIER_USR:
			path = ProbeIdDb.getKeyFromHostUserFile(file);
			break;
		case HIER_SYS:
			path = ProbeIdDb.getKeyFromHostFile(file);
			break;
		case HIER_PROBE:
			path = ProbeIdDb.getKeyFromProbe(file);
			break;
		default:
		}
		return path;
	}
	
	private OutputStream getOutputStream(String key) throws IOException {
		OutputStream writer = null;
		Boolean remote = m_client != null;

		if (remote)
			notYetImplemented("remote probe I/O not implemented yet"); // TODO: writer = m_client.getBufferedWriter(key);
		else
			writer = ProbeIdDb.getBufferedWriter(key);

		if (doCache())
			writer = m_cache.getBufferedWriter(key, writer);
		
		if (doCrypto())
			writer = m_crypto.getBufferedWriter(writer);
		return writer;		
	}
	
	public OutputStream getOutputStream(String file, Hierarchy level)
	throws IOException 
	{		
		String key = getProbeIdKey(file, level);
		return getOutputStream(key);
	}
	
	public BufferedWriter getBufferedWriter(String file, Hierarchy level) 
		throws IOException
	{
		return new BufferedWriter(new OutputStreamWriter(getOutputStream(file, level)));
	}
	
	public BufferedWriter getBufferedWriter(String file) 
		throws IOException 
	{
		return new BufferedWriter(new OutputStreamWriter(getOutputStream(file, Hierarchy.HIER_USR)));
	}
	
	/**
	 * Get  unique Probe Identifier.  Probe ID files should not be 
	 * encrypted or cached; it is the responsibility of the caller
	 * to ensure that the buffered reader is for a raw (probe-resident)
	 * file.
	 * 
	 * @param reader - a BufferedReader for the Probe ID file.
	 * @return a String representing the unique probe identifier
	 * @throws IOException
	 */
	public static String getProbeId(BufferedReader reader) throws IOException {
		return ProbeIdDb.getParam(reader, PARAM_KEY_PROBEID);
	}
	
	public String getProbeId() {
		return m_id == null ? readProbeId(true) : m_id;
	}

	/**
	 * Read the probe ID off of the probe.
	 * @param postError - post an error to VnmrJ (can cause long timeout delay)
	 */
	String readProbeId(boolean postError) {
		if (m_state == Status.ERROR) {
			return null;
		}
		String key = ProbeIdDb.getProbeIdKey();
		BufferedReader reader = getUnencryptedReaderFromKey(m_probeId, key, false);
		String id = null;
		if (reader == null) {
			if (postError) 
				error("probe not attached or uninitialized");
		} else try {
			id = getProbeId(reader);
			reader.close();
		} catch (IOException e) {
			if (postError) error("I/O error while reading probe ID");
		}
		return id;
	}
	
	ProbeIdStore getDbHandle(ProbeIdCmd cmd) {
		String probeId = (cmd.id() != null) ? cmd.id() : m_id;
		try {
			if (cmd != null && cmd.offline()) {
				String cacheDir = System.getProperty(KEY_PROBEID_CACHE, DEFAULT_CACHE);
				 
				if (cmd.cache() != null) {
					cacheDir = cmd.cache();
					if (cacheDir.equals(m_cacheDir) && m_cache != null)
						return m_cache;
					else
						return new ProbeIdCache(cacheDir, m_db, probeId);
				}
			} else {
				return m_probeId;
			}
		} catch (IOException e) {
			error("I/O error accessing probe cache: "+e.getMessage());
			e.printStackTrace();
		} catch (ProbeIdMismatchException e) {
			// swallow this exception - it's not a problem here because
			// there may be no probe attached yet (i.e. during server startup)
		}
		return null;
	}
	
	/**
	 * check if probe matches expected value.
	 * There are several modalities.  In normal operating mode, the user
	 * just sends a command to the currently attached probe, which is expected
	 * to match the last probe that was attached.
	 * In offline mode, the user supplies the probe id and operations run on
	 * the cached data.  This works because the cache is always expected to
	 * be more current than the physical probe, at least on the current host.
	 * @return true if the probe matches the expected value or is a valid probe.
	 */
	boolean checkProbeId(ProbeIdStore db, ProbeIdCmd cmd) {
		String key = ProbeIdDb.getProbeIdKey();
		if (cmd != null && cmd.offline()) {    // offline mode
			return db != null && db.isCached(key);
		}
		BufferedReader reader = getUnencryptedReaderFromKey(m_probeId, key, false);
		if (m_id == null) {
			return readProbeId(false) != null; // cold start - read probe ID
		} else if (reader == null) {
			return false;                      // not attached
		} else try {
			try {                              // check if actual ID matches assumed
				if (m_online) {
					String id = getProbeId(reader);
					return m_id.equals(id);
				} else return true;            // don't care in offline mode
			} finally {
				reader.close();
			}
		} catch (IOException e) {
			error("I/O error while reading probe ID value");
			return false;
		}
	}
	
	/**
	 * return the contents of a the ProbeID record (i.e. file)
	 * @param name
	 * @return string array of lines
	 */
	public Collection<String> getProbeIdInfo(Collection<String> data) {
		String key  = ProbeIdDb.getProbeIdKey();
		String line = null;
		try {
			BufferedReader reader = getUnencryptedReaderFromKey(m_db, key, false);
			while ((line = reader.readLine()) != null)
				data.add(line);
		} catch (IOException e) {
			error(iam+"I/O exception for key="+key);
			e.printStackTrace();
		}
		return data;
	}
	
	// utility function to allow annotation of unimplemented features
	static public void notYetImplemented(String msg) {
		try {
			throw new Exception(msg);
		} catch (Exception e) {
			error(iam+" not implemented yet!");
			e.printStackTrace();
		}
	}

	public static int getPortNum() {
		String portNum  = System.getProperty(KEY_PROBEID_PORT);
		if (portNum != null && !portNum.isEmpty()) 
			return ProbeId.DEFAULT_PORT;
		else
			return Integer.parseInt(portNum);
	}
	
	ProbeIdIO init(String cryptoAlgorithm, ProbeIdCmd cmd) {
		String remote = System.getProperty(KEY_PROBEID_REMOTE);
		String crypto = System.getProperty(KEY_PROBEID_ENCRYPT);
		String cache  = System.getProperty(KEY_PROBEID_CACHE);
		String zentry = System.getProperty(KEY_PROBEID_ZIPENTRY);
		if (cmd != null) {
			if (cmd.crypto != null) crypto = cmd.crypto.toString();
			if (cmd.cache != null) cache = cmd.cache;
			m_id = cmd.id();
		}
		// initialize crypto system
		if (crypto != null && crypto.equalsIgnoreCase("true")) 
			m_crypto = new ProbeIdCrypto(cryptoAlgorithm);
		
        // probe DB handle has been determined, so try to read the probe ID
        if (m_id == null) 
        	m_id = readProbeId(false);
        
		// initialize either the client interface or a direct database interface
		if (remote != null && !remote.isEmpty()) 
			m_client = new ProbeIdClient(getPortNum());
		
		// initialize the cache directory
		if (cache != null && !cache.isEmpty()) {
			m_cacheDir = new File(cache);
			m_cacheDir.mkdirs();
			enableCache(true);
		}
		// hierarchy starts at cache if there is one, otherwise the raw DB
		m_probeId = m_cache != null ? m_cache : m_db;
		
		// initialize the zip entry
		if (zentry == null) 
			System.setProperty(KEY_PROBEID_ZIPENTRY, DEFAULT_ZIP_ENTRY);
		
		return this;
	}
	
	/**
	 * Add or change the cache.
	 * @param dir
	 * @throws IOException 
	 */
	private String cache(String dir) {
		if (dir != null && !dir.isEmpty()) {
			if (m_cache != null && !m_cache.getPath().equals(dir)) {
				try { // flush the previous cache
					m_cache.flush(m_id);
				} catch (IOException e) {
					// File I/O error
					error("cache flush I/O error");
				} catch (ProbeIdMismatchException e) { 
					// a new probe was already inserted
					error(e.getMessage());
				}				
				return m_cache.probeId; // do nothing if it's not a new cache directory 
			}
			try {
				m_cache = new ProbeIdCache(dir, m_db, readProbeId(false));
				return m_cache == null ? null : m_cache.probeId;
			} catch (IOException e) {
				enableCache(false);
				error(e.getMessage());
			} catch (ProbeIdMismatchException e) {
				ProbeIdIO.error(e.getMessage());
			}
		} else {
			enableCache(false);
		}
		return null; // caching disabled
	}
	
	/**
	 * Flush the specified cache directory; primarily to support unit
	 * testing and native file system probes with probe ID.
	 * @param cacheDir
	 * @throws IOException
	 * @throws ProbeIdMismatchException
	 */
	public static String flush(String cacheDir) 
		throws IOException, ProbeIdMismatchException 
	{
		ProbeIdIO probe = new ProbeIdIO();
		String probeId = probe.cache(cacheDir);
		if ( probeId != null)
			probe.m_cache.flush(probeId);
		return probeId;
	}
	
	public void flush(String src, String sub, Boolean sysdir, Boolean usrdir) {
		if (src==null) return;
		if (!src.startsWith(File.separator)) {
			error("Probe flush requires absolute path to source");
			return;
		}
		boolean sys = sysdir!=null && sysdir;
		boolean usr = usrdir!=null && usrdir;
		if ((sys && usr) || (!sys && !usr)) {
			error("Probe flush invalid combination of system and user flags");
			return;
		}
		File path = new File(src);
		ProbeIdCache cache = new ProbeIdCache(path, m_db, m_id);
		try {
			cache.flush(path, sub, null, sys);
		} catch (IOException e) {
			error("Probe flush error "+e.getMessage());
			e.printStackTrace();
		}
	}
	
	public void flush() throws IOException, ProbeIdMismatchException
	{
		m_probeId.flush(m_id);
	}
	
	/**
	 * Attach a probe to the ProbeIdIO system if the system is in
	 * "online" mode, i.e. looking at the physical probe instead of
	 * a virtual probe.
	 */
	public ProbeIdIO attach() {
		if (m_online) {
			m_id = readProbeId(false);
			return attach(true);
		}
		return this;
	}
	
	/**
	 * Attach a virtual probe.  As a side effect it puts the probe
	 * server into "offline" mode, i.e. works with a virtual probe
	 * instead of the physical probe.  This prevents the probe monitor
	 * from attaching a new physical probe if one becomes attached.
	 * A potential conflict between virtual and physical probe must
	 * be handled by the client (VnmrJ).
	 * @param id
	 * @return
	 */
	ProbeIdIO attach(String id) {
		if (id == null) { 		// go to online mode
			m_online = true;
			return attach();
		}

		if (!ProbeIdCache.checkPath(m_cacheDir.getPath(), id)) {
			error(id + " is an unknown probe id - attach probe to console at least once");
			return null;
		}
		m_online = false;
		m_id = id;
		return attach(false);
	}
	
	synchronized ProbeIdIO attach(boolean online) {
		m_db = new ProbeIdDb();
		m_probeId = m_db;
		if (m_id != null && m_cacheDir != null) {
			try {
				m_cache = null; // FIXME: this disables caching, but it ain't elegant - add a dontCache() method that sets m_enableCache to false?
				m_cache = new ProbeIdCache(m_cacheDir.getPath(), m_db, m_id);
				enableCache(true);
				m_probeId = m_cache;
			} catch (IOException e) {
				error("Could not create ProbeID cache \"" + m_cacheDir.getPath() + "\"");
				e.printStackTrace(); 
				m_cache = null;
			} catch (ProbeIdMismatchException e) {
				Messages.postWarning(e.getMessage());
			}
		}
		ProbeIdTinyFiles.setSavedId(online ? "" : (m_id != null ? m_id : ""));
		return this;
	}
	
	/**
	 * Detach the probe.  Cache remains active.
	 */
	void detach() {	
		m_db = null;    // database is probe-resident
		m_id = null;    // id is probe-resident 
	}
	
	/**
	 * Set up system defaults
	 */
	public static void setupProbeId() {
	   	System.setProperty(KEY_PROBEID_USEPROBEID, "true");
    	// enable encryption
		System.setProperty(KEY_PROBEID_ENCRYPT, "true");
		// set encryption to default
		System.setProperty(KEY_PROBEID_CIPHER, DEFAULT_CIPHER);
		// set caching to default (DEFAULT_CACHE="" disables it)
		System.setProperty(KEY_PROBEID_CACHE, DEFAULT_CACHE);
		// disable remote port
		System.setProperty(KEY_PROBEID_REMOTE, "");
		// set probe port to parameter (in case remote port is enabled by user)
        System.setProperty(KEY_PROBEID_PORT, DEFAULT_PORT.toString());
        // use the Ethernet NIC as the system ID (TODO: get console ID instead)
		System.setProperty(KEY_PROBEID_ETHERPORT, DEFAULT_NIC);	
	}

	public static void setProbeIdSystemDefaults() {
		// first set up defaults for this directory
		ProbeIdIO.setupProbeId();
	}
	
	public String getCacheDir() { return m_cache.getPath(); }
	
	/**
	 * print informational messages to stderr so that the output
	 * to stdout can still be redirected to a file or pipe.
	 * @param msg
	 */
	public static void info(String msg) {
		String verbosity = System.getProperty(ProbeId.KEY_PROBEID_VERBOSE);
		if (verbosity != null && Integer.decode(verbosity) > 0)
			System.err.println(iam+msg);
	}
	
	public static void debug(String msg) {
		String verbosity = System.getProperty(ProbeId.KEY_PROBEID_VERBOSE);
		if (verbosity != null && Integer.decode(verbosity) > 1) {
			System.err.println(msg);
			String send = System.getProperty(KEY_PROBEID_INFO2VJ, INFORM_VNMRJ.toString());
			if (send.equals("true"))
				Messages.postDebug(iam+msg);
		}
	}
	
	public static void error(String msg) {
		if (msg != null) {
			System.err.println(iam+msg);
			String send = System.getProperty(KEY_PROBEID_INFO2VJ, INFORM_VNMRJ.toString());
			if (send.equals("true"))
				Messages.postError(iam+msg);
		}
	}
	
	public static void warning(String msg) {
	    if (msg != null) {
            System.err.println(msg);
            String send = System.getProperty(KEY_PROBEID_INFO2VJ, INFORM_VNMRJ.toString());
            if (send.equals("true"))
                Messages.postWarning(iam+msg);
	    }
	}
	public static void usage(String prog) {
		String options = " -<cmd> [-opt <option>]+ [-param <param>] [-cfg <cfg>] [-sys]";
		error("usage: " + prog + options);
	}
	
	/**
	 * Initialize a raw probe filesystem 
	 * @param dst
	 * @param path
	 * @return
	 */
	private boolean initProbe(String dst, String path) {
		try {
			enableCache(false);
			String mnt = m_db == null 
							? System.getProperty(KEY_PROBEID_DBDIR) 
							: m_db.getPath();
			String prb = (new File(mnt)).getCanonicalPath();
			if (dst == null)
				error("probeid init: no destination specified");
			File target = new File(mnt, dst);
			ProbeIdDb.copy(new File(path), target);
			info("initializing "+prb);
			attach();
			return true;
		} catch (IOException ioe) {
			error("I/O Error encountered while creating "+KEY_PROBEID);
		}
		return false;
	}

	private String[] split(String line) {
		List<String> args = new ArrayList<String>();
		final Pattern regex = Pattern.compile("\"([^\"]*)\"|(?<= |^)([^ ]*)(?: |$)");
		Matcher match = regex.matcher(line);
		while (match.find()) {
			if (match.group(1) != null) 
				args.add(match.group(1));
			else if (match.group(2) != null) 
				args.add(match.group(2));
			else if (match.group() != null) 
				args.add(match.group());
		}
		return args.toArray(new String[args.size()]);
	}
	
	public boolean process(String line) {
		String[] args = split(line);
		ProbeIdCmd cmd = parse(args);
		try {
			return process(args[0], cmd, cmd.timeout());
		} catch (Exception e) {
			error(e.getMessage());
			if (!(e.getCause() instanceof ProbeIdMismatchException))
				e.printStackTrace(); // parser created invalid command!
		}
		return false;
	}
	
	public boolean process(String name, ProbeIdCmd cmd, Integer timeout) {
		Task task = new Task(name, cmd);
		Thread thread = new Thread(task);
		thread.setName("CmdProcessor");
		try {
			thread.start();
			if (timeout != null) 
				thread.join(timeout);
			else
				thread.join(DEFAULT_IO_TIMEOUT);
		} catch (InterruptedException e) {
			error("Probe command was interrupted "+e.getMessage());
			thread.interrupt();
			return false;
		}
		if (thread.isAlive()) {
			error("Network Attached Probe Unit is not responding");
			thread.interrupt();
			if (thread.isInterrupted()) {
				try {
					thread.join(DEFAULT_IO_TIMEOUT);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				if (thread.isAlive()) {
					error("Could not terminate probe command");
					// Tell VJ we've experienced an unrecoverable timeout
					VnmrProxy.sendNotification("timeout");
					m_state = Status.ERROR;
				}
			} else {
				error("Disable Probe ID to continue");
			}
		}
		// if the task result is null then the command failed
		return task.result != null ? task.result : false;
	}
	
	class Task implements Runnable {
		public Exception   exception = null;
		public Boolean     result    = null;
		private String     m_name    = null;
		private ProbeIdCmd m_cmd     = null;
		@Override
		public void run() {
			try {
				result = process(m_name, m_cmd);
			} catch (Exception e) {
				exception = e;
			}
		}
		Task(String name, ProbeIdCmd cmd) { 
			m_name = name; 
			m_cmd  = cmd; 
		}
	}

	// TODO: implement this using a Command pattern a la http://www.javaworld.com/javaworld/javatips/jw-javatip68.html?page=4
	protected static ProbeIdCmd parse(String[] args) {
		if (args == null) return null;          // no arguments = no work
		Integer verbosity = 0;
		ProbeIdCmd cmd = new ProbeIdCmd("id");  // default probe ID command
		
		try {
			for (int i=0; i<args.length; i++) {
				if (args[i] == null || args[i].isEmpty()) {
					// this is here to simplify testing with either/or flags such as -sys
				} else if (args[i].equals("-add") && i+1 < args.length) {
					cmd.set("add", args[++i]);
				} else if (args[i].equals("-addrow") && i+1 < args.length) {
					cmd.set("addrow", args[++i]);
				} else if (args[i].equals("-attach")) {
                    if (i+1 < args.length && !args[i+1].startsWith("-"))
    					cmd.set("attach", args[++i]);
                    else
                    	cmd.set("attach");
                    cmd.reattach = true;
				} else if (args[i].equals("-appdirs")) {
                    if (i+1 < args.length && !args[i+1].startsWith("-"))
                        cmd.set("appdirs", args[++i]);
                    else
                        cmd.set("query","appdirs");
				} else if (args[i].equals("+appdirs")) {
				    cmd.set("appdirs", null); // disable appdir mode
				} else if (args[i].equals("-backup") && i+1 < args.length) {
					cmd.set("backup", args[++i]);
				} else if (args[i].equals("-blank")) {
					// blank a simulated probe
					if (i+1 < args.length && args[i+1].startsWith("-"))
						cmd.set("blank");
					else 
						cmd.set("blank", args[++i]);
				} else if (args[i].startsWith("-blob:") && i+1 < args.length) {
					// get a read-only handle to a blob
					cmd.set(args[i].substring(1), args[++i]);
				} else if (args[i].equals("-cache") ) {
					System.setProperty(ProbeId.KEY_PROBEID_CACHE, args[++i]);
					info("probe cache "+args[i]);
					cmd.cache = args[i];
					cmd.reattach = true;
				} else if (args[i].equals("-cfg") && i+1 < args.length) {
					cmd.cfg(args[++i]);
				} else if (args[i].equals("-check") && i+1 < args.length) {
					System.setProperty(KEY_PROBEID_CHECK_CFG, args[++i]);
				} else if (args[i].equals("-commit") && i+1 < args.length) {
					cmd.set("commit");
					cmd.path(args[++i]);
				} else if (args[i].equals("-connect") && i+1 < args.length) {
					// simulated connecting probe to console
					cmd.set("connect", args[++i]);
				} else if (args[i].equals("-copy") && i+1 < args.length) {
					cmd.set("copy", args[++i]);
				} else if (args[i].equals("-crypto")) {
					System.setProperty(ProbeId.KEY_PROBEID_ENCRYPT, "true");
					info("encryption enabled");
					cmd.crypto = true;
				} else if (args[i].equals("-disconnect")) {
					// simulate disconnecting probe from console
					cmd.set("disconnect");
				} else if (args[i].equals("-drop")) {
					cmd.set("drop", args[++i]);
				} else if (args[i].equals("-exists")) {
					if (i+1 < args.length && !args[i+1].startsWith("-"))
						cmd.set("exists", args[++i]);
					else
						cmd.set("exists");
				} else if (args[i].equals("-export") && i+1 < args.length) {
					cmd.set("export", args[++i]);
				} else if (args[i].equals("-echo") && i+1 < args.length) {
					cmd.set("echo", args[++i]);
				} else if (args[i].equals("-flush")) {
					if (i+1 < args.length && !args[i+1].startsWith("-"))
						cmd.set("flush", args[++i]);
					else
						cmd.set("flush");
				} else if (args[i].equals("-getparam") && i+1 < args.length) {
					cmd.set("getparam", args[++i]);
				} else if (args[i].equals("-help")) {
					cmd.set("help");
					break;
				} else if (args[i].equals(CMD_PROBE_ID)) { // -id
					if (i+1 < args.length && !args[i+1].startsWith(OPTIONS_SEPARATOR))
						cmd.id(args[++i]); // set for use with offline processing
					else
						cmd.set("id");     // command to get the id
				} else if (args[i].equals("-import") && i+1 < args.length) {
					cmd.set("import");
					cmd.path(args[++i]);
				} else if (args[i].equals("-list") && i+1 < args.length) {
                    if (i+1 < args.length && !args[i+1].startsWith("-"))
                        cmd.set("list", args[++i]);
                    else
                        cmd.set("list");
				} else if (args[i].equals("-info2vnmr") && i+1 < args.length) {
					System.setProperty(KEY_PROBEID_INFO2VJ, args[++i].equals("0") ? "false" : "true");
					return null;
				} else if (args[i].equals("-init") && i+1 < args.length) {
					cmd.set("init");
					cmd.path(args[++i]);
				} else if (args[i].equals("-mnt") && i+1 < args.length) {
					String dbLocation = args[++i];
					System.setProperty(ProbeId.KEY_PROBEID_DBDIR, dbLocation);
					System.setProperty(ProbeId.KEY_PROBEID_MOUNTPOINT, dbLocation);
					info("probe mount point "+dbLocation);
					cmd.reattach = true;
				} else if (args[i].equals("-monitor") && i+1 < args.length) {
					cmd.set("monitor");
					String action = args[++i];
						cmd.key(action);
				} else if (args[i].equals("-nocrypto")) {
					System.setProperty(ProbeId.KEY_PROBEID_ENCRYPT, "false");
					info("encryption disabled");
					cmd.crypto = false;
				} else if (args[i].equals("-notify") && i+1 < args.length) {
				    cmd.notif(args[++i]);
				} else if (args[i].equals("-opt") && i+1 < args.length) {
					cmd.opt(args[++i]);
				} else if (args[i].equals("-param") && i+1 < args.length) {
					cmd.param(args[++i]);
				} else if (args[i].equals("-perm") && i+1 < args.length) {
					cmd.set("permissions", args[++i]);
				} else if (args[i].equals("-query") && i+1 < args.length) {
					cmd.set("query", args[++i]);
				} else if (args[i].equals("-readfile") && i+1 < args.length) {
					cmd.set("readfile", args[++i]);
				} else if (args[i].equals("-reset")) {
					cmd.set("reset");
				} else if (args[i].equals("-setparams") && i+1 < args.length) {
					cmd.set("setparams", args[++i]);
				} else if (args[i].equals(CMD_SYSTEM)) { // -sys
					cmd.sys(true);
				} else if (args[i].equals("-start")) {
					cmd.set(CMD_START);
				} else if (args[i].equals("-status") && i+1 < args.length) {
					cmd.set("status", args[++i]);
				} else if (args[i].equals("-tree") && i+1 < args.length) {
					cmd.tree(args[++i]); // VNMR parameter tree
				} else if (args[i].equals(CMD_USER)) { // -usr
					cmd.usr(true); // FIXME: rework this with a new interface: -search <start>[:[<end>]]
				} else if (args[i].equals("-quit")) {
					cmd.set("quit");
					break;
				} else if (args[i].equals("-v")) {
					System.setProperty(KEY_PROBEID_VERBOSE, (++verbosity).toString());
				} else if (args[i].equals("-vnmrj") && i+1 < args.length) {
					cmd.set("vnmrj", args[++i]);
				} else if (args[i].equals("-vnmrsystem") && i+1 < args.length) {
					System.setProperty(VnmrIO.KEY_SYSDIR, args[++i]);
				} else if (args[i].equals("-vnmruser") && i+1 < args.length) {
					System.setProperty(VnmrIO.KEY_USRDIR, args[++i]);
				} else if (args[i].equals("-target")) {
					if (i+1 < args.length && !args[i+1].startsWith("-"))
						cmd.set("target", args[++i]);
					else
						cmd.set("target");
				} else if (cmd.is("-property", args, i) || cmd.is("-p", args, i)) {
					cmd.property(args[++i]);
				} else if (args[i].equalsIgnoreCase("-test")) {
					cmd.set("test");
				} else if (args[i].equalsIgnoreCase("-timeout")) {
					if (i+1 < args.length && !args[i+1].startsWith("-"))
						cmd.timeout(args[++i]);
					else
						cmd.timeout(DEFAULT_IO_TIMEOUT);
					if (cmd.timeout() < 100) // if it's less than .1 secs assume secs
						cmd.timeout(cmd.timeout()*1000);
				} else {
					error("invalid option: " + args[i]);
					usage("probeid");
					return null; // don't quit on a parse error
				}
			}
		} catch (Exception e) {
			// command-line parsing error
			error(iam+"only one command is permitted at a time!");
			usage("probeid");
		}

		info("cmd=" + cmd.cmd());
		
		return cmd;
	}

	public boolean process(String name, ProbeIdCmd cmd) 
		throws ProbeIdMismatchException, IllegalArgumentException, IOException
	{
		if (cmd == null) 
			return false;

		ProbeIdStore db = getDbHandle(cmd);

		try {
			// try commands that work for disconnected or uninitialized probe
			return processDisconnected(db, cmd);
		} catch (IllegalArgumentException e) {
			// try commands for a connected probe last
			boolean online      = !cmd.offline(); // caller did't specify an ID, so use the physically attached one
			String probeId      = "";
			boolean connected   = true;
			boolean initialized = connected && probeId != "";

			if (online) {
				probeId     = readProbeId(false);
				connected   = probeId != null;
				initialized = connected && probeId != "";
				boolean mismatched  = connected && !checkProbeId(db, cmd);
				boolean reattach    = connected && m_cacheEnabled && m_cache==null;
				if (initialized) {
					if (cmd.cache != null || mismatched)  
						cache(cmd.cache);   // new cache directory
					if (cmd.reattach || reattach)
						attach();           // new mount or cache directory
					if (cmd.crypto != null) 
						enableCrypto(cmd.crypto);
				}
				if (mismatched && !ProbeIdMonitor.isRunning(m_monitor)) {
					String send = System.getProperty(KEY_PROBEID_INFO2VJ, INFORM_VNMRJ.toString());
					if (send.equals("true"))
						VnmrProxy.sendNewProbeNotification(probeId);
				}
			}
			return processConnected(db, cmd, connected, probeId, initialized);
		}
	}
	
	public boolean processDisconnected(ProbeIdStore db, ProbeIdCmd cmd) 
		throws IllegalArgumentException, ProbeIdMismatchException 
	{
		// process commands that work without an initialized probe
		if (cmd.is(CMD_START)) {
			if (cmd.getOpts().length == 0)
				return start(null, null, true, cmd.notif()) == null;
			else if (cmd.getOpts().length == 1)
				return start(null, null, false, cmd.notif()) == null;
			else if (cmd.getOpts().length == 2)
				return start(cmd.nextOpt(), cmd.nextOpt(), true, cmd.notif()) == null; // kick off the server
			else if (cmd.getOpts().length == 3)
				return start(cmd.nextOpt(), cmd.nextOpt(), false, cmd.notif()) == null;
			else
				usage(cmd.cmd());
		} else if (cmd.is("attach")) {     // attach cache and ID
			attach(cmd.id());
			print(m_id);
		} else if (cmd.is("blank")) {      // erase simulated physical probe
			ProbeIdSimulatedProbe.delete(cmd.key());
		} else if (cmd.is("connect")) {    // simulate physical probe connection
			ProbeIdSimulatedProbe.connect(cmd.key(), true);
		} else if (cmd.is("disconnect")) { // simulate physical probe disconnect
			ProbeIdSimulatedProbe.disconnect();
		} else if (cmd.is("echo")) {       // respond to a diagnostic ping
			print(cmd.key());
		} else if (cmd.is("init")) {       // initialize the probe
			initProbe(cmd.nextOpt(), cmd.path());
		} else if (cmd.is("list")) {
			listProbeFiles(db, cmd.key(), cmd.nextOpt(), cmd.usr(), cmd.sys());
		} else if (cmd.is("monitor")) {    // start the probe mount monitor
			monitor(cmd.key());
		} else if (cmd.is("reset")) {      // reset the server
			detach();
		} else if (cmd.is("quit")) {       // shut down the probe server
			return true;
		} else if (cmd.is("vnmrj")) {      // send a command to VnmrJ
			VnmrProxy.sendToVnmr(cmd.key());
 		} else if (cmd.is("test")) {       // run unit tests
			org.junit.runner.JUnitCore.main("vnmr.apt.ProbeIdIOTest");
		} else {
			throw new IllegalArgumentException("invalid argument: "+cmd.cmd());
		}
		return false;
	}
	
	private boolean doCheckProbeId() {
		String property = System.getProperty(KEY_PROBEID_CHECK_CFG, DEFAULT_CFG_CHECK.toString());
		return Boolean.getBoolean(property);
	}
	
	public boolean processConnected(ProbeIdStore db, ProbeIdCmd cmd,
									boolean connected, String probeId, boolean initialized) 
		throws ProbeIdMismatchException, IllegalArgumentException, IOException
	{
		if (m_state == Status.ERROR) {
			throw new ProbeIdMismatchException(cmd.cmd()
						+": Probe ID unit hardware error");
		}
		if (doCheckProbeId() && !checkProbeId(db, cmd))
			throw new ProbeIdMismatchException(cmd.cmd()
						+": unexpected probe "+readProbeId(false)
						+", expected "+m_id);
		if (cmd.is("id")) {
			setVnmrProbeId(cmd.param(), cmd.tree());
		} else if (cmd.is("exists")) {
			exists(db, cmd.cfg(), cmd.key(), cmd.nextOpt(), cmd.usr(), cmd.sys());
		} else if (cmd.is("flush")) {
			if (cmd.moreOpts())
				flush(cmd.key(), cmd.nextOpt(), cmd.sys(), cmd.usr());
			else
				flush();
		} else if (cmd.is("import")) {
			String key = null;
			if (!cmd.path().equals("*"))
				key = (cmd.cfg() != null) 
					? cmd.cfg() : cmd.moreOpts() 
					? cmd.nextOpt() : null;
			while (cmd.moreOpts())
				importProbeFile(db, cmd.path(), key, cmd.nextOpt(), cmd.sys());
		} else if (cmd.is("commit")) {
			String key = cmd.nextOpt();
			commitProbeFile(db, cmd.path(), key, cmd.nextOpt(), cmd.sys());
		} else if (cmd.is("export")) {
			exportProbeFile(db, cmd.cfg(), cmd.nextOpt(), cmd.key(), cmd.sys());
		} else if (cmd.cmd().startsWith("blob:")) {
			processBlob(db, cmd);
		} else if (cmd.is("getparam")) {
			getparam(db, cmd.key(), cmd.cfg(), cmd.nextOpt());  // ok if cfg is null
		} else if (cmd.is("setparams")) {
			setparams(db, cmd.key(), cmd.cfg(), cmd.nextOpt()); // ok if cfg is null
		} else if (cmd.is("readfile")) {
			readfile(db, cmd.param(), cmd.key(), cmd.nextOpt());
		} else if (cmd.is("copy")) {
			boolean copied = true;
			while (cmd.moreOpts())
				copied = copy(db, cmd.key(), cmd.key(), cmd.nextOpt()) && copied;
			print(copied);
		} else if (cmd.is("backup")) {
			backup(db, cmd.key(), cmd.nextOpt(), cmd.sys());
		} else if (cmd.is("add")) {
			add(db, cmd.key(), cmd.nextOpt(), cmd.sys());
		} else if (cmd.is("drop")) {
			while (cmd.moreOpts())
				drop(db, cmd.key(), cmd.nextOpt(), cmd.sys());
			if (connected)
				db.flush(probeId);
		} else if (cmd.is("target")) {     // calibration target
			readCalTargets(db, cmd.key());
		} else if (cmd.is("permissions")) {
			checkperm(db, cmd.key(), cmd.nextOpt(), cmd.sys(), initialized);
		} else if (cmd.is("addrow")) {
			add(db, cmd.cfg(), cmd.key(), cmd.getOpts(), cmd.sys());
		} else if (cmd.is("query")) {      
			query(db, cmd, connected);
		} else if (cmd.is("status")) {     // query state of probe
			status(cmd, connected, probeId);
		} else if (cmd.is("appdirs")) {
		    appdirs(cmd.key(), cmd.nextOpt());
		} else 
			throw new IllegalArgumentException(cmd.cmd()); 
		return false;
	}
	
	private void appdirs(String path, String labels) {
	    m_appdirs = null;
	    if (path != null)
	        m_appdirs = new ProbeIdAppdirs(path, labels);
	}
	
	/**
	 * A BLOB can a binary file, a regular file, or a directory.
	 * It is the responsibility of the application code using the BLOB to
	 * ensure atomicity of operations, etc.
	 * @param name of the command
	 * @param cmd is the Command data structure that defines the BLOB operation
	 */
	private void processBlob(ProbeIdStore db, ProbeIdCmd cmd) {
		if (cmd.is("blob:link:r+") || cmd.is("blob:link:r")) { 
			// a read-write handle to an existing BLOB
			// todo: break out the read-only version, which isn't read-only right now
			String key = cmd.key();
			String path = cmd.nextOpt();
			exportProbeBlob(db, path, path, key, cmd.usr(), cmd.sys(), "r+");
		} else if (cmd.is("blob:link:a+")) { 
			// a write handle to an existing BLOB or a directory
			// to which one can be added by the caller
			String key = cmd.key();
			String path = cmd.nextOpt();
			exportProbeBlob(db, path, path, key, cmd.usr(), cmd.sys(), "a+");
		} else if (cmd.is("blob:commit")) {
			String path = cmd.key();
			String key = cmd.nextOpt();
			commitProbeFile(db, path, key, cmd.nextOpt(), cmd.sys());
		} else if (cmd.is("blob:import")) {
			String path = cmd.key();
			String key = cmd.nextOpt();
			importProbeBlob(db, path, key, cmd.nextOpt(), cmd.sysOnly());
		} else throw new IllegalArgumentException(cmd.cmd());
	}

	public void exists(ProbeIdStore db, String cfg, String file, String subdir,
					   Boolean usrdir, Boolean sysdir)
	{
		int searched = 0;
		subdir = ProbeIdDb.relativeForm(subdir);
		Appdir[] search = getTemplateKeys(subdir, file, usrdir, sysdir, null);
		String[] results = {"0",""};
		if (file != null && !file.trim().isEmpty()) {
			for (Appdir src : search) {
				if (src != null && src.key != null) {
	                searched++;
					File tbl = db.getPath(src.key);
					if (tbl.exists()) {
					    if (m_appdirs != null) { // return appdir as the owner
					        String[] r = { Integer.toString(searched), src.owner }; 
					        results = r;
					    } else { // return pre-appdir style "user","system" owner
					        String[] r = {Integer.toString(searched), src.label};
					        results = r;
					    }
					    break;
					}
				}
			}
		}
		print(results);
	}

	/**
	 * ProbeIdIO main can be used to load a ProbeId database from an existing
	 * system directory or to run unit tests.
	 * 
	 * @param -import <directory>  to load <directory> into probe
	 * @param -test
	 * 
	 * Example: java -ea -jar ProbeIdIO -import autox400DB 
	 * @throws Exception 
	 */
	public static void main(String[] args) {
		ProbeIdIO.setProbeIdSystemDefaults(); // set Probe ID system defaults
		ProbeIdIO probe = new ProbeIdIO();
		probe.attach(ProbeIdTinyFiles.getSavedId());

		try {
			if (probe.process(CMD_START, parse(args)))
				probe.exit();
		} catch (ProbeIdMismatchException e) {
			ProbeIdIO.error(iam+"probe mismatch error "+e.getMessage());
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		info("done");
	}
}
