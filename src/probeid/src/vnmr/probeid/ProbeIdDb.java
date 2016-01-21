/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.RandomAccessFile;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.sql.Timestamp;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Collection;
import java.util.Date;
import java.util.LinkedList;
import java.util.SortedSet;
import java.util.TreeSet;
import java.util.Vector;
import vnmr.vjclient.*;

/**
 *   ProbeIdDb - provides physical file level access and related
 *   utilities.
 *   
 *   Encryption/Decryption is handled at higher levels, as are
 *   client/server implementation details.
 *   
 *   In a client/server environment, the server and client are
 *   likely to have different mount points.
 *   
 *   Implements a simple file-based database in order to maintain
 *   a semblance of backwards compatibility with previous generations
 *   of probe files.
 *
 *   Currently the system assumes all files are written and read
 *   atomically and in their entirety.
 *   
 *   Implementation details are hidden from the caller by providing
 *   a BufferedReader and BufferedWriter, which are the legacy
 *   interfaces used by VnmrJ apt code.
 *   
 *   FIXME: currently most of the getBufferedReader/getBufferedWriter 
 *   actually return InputStream/OutputStream, which make for more
 *   ready concatenation of multiple levels of files.
 */
public class ProbeIdDb implements ProbeId, ProbeIdStore {
    // TODO: make sysdir a bona fide subdir under hostid
    private final static String SysDirTag = "."; // right above userdirs and other appdirs
    private final static String TagSeqFile = ".tagseq"; // where sequence number of lf last tag is kept    private final static Executors = Executors.newCachedThreadPool();
    

   
    static class Tokenizer {
		private String [] m_tokens = null; // components of original string
		private int m_pos = 0;             // current index into string
		
		public String get(String key) {
			while (m_pos < m_tokens.length && m_tokens[m_pos].isEmpty())
				++m_pos;
			if (m_pos < m_tokens.length && m_tokens[m_pos].equals(key)) {
				int idx = m_pos + 1;
				m_pos = idx + 1;
				return m_tokens[idx];
			}
			return "";
		}
		
		public String find(String key) {
			for (int i=0; i<m_tokens.length; i++)
				if (m_tokens[i].equals(key))
					return m_tokens[i+1];
			return null;
		}
		
		public void reset() {
			m_pos = 0;
		}
		
		public Tokenizer(String tokens) {
			m_tokens = tokens.split("\\s+");
		}

		public boolean hasMoreTokens() {
			return m_pos < m_tokens.length;
		}

		public String nextToken() {
			return hasMoreTokens() ? m_tokens[m_pos++] : "";
		}
	}

	public static final String KEY_EXTENSION = ""; // needs to be "" for backwards compatibility
	public static final String ZIP_EXTENSION = ".zip";
	private static char [] m_buf = new char[MAX_BUF_SIZE]; // utility buffer
	private static byte [] m_buffer = new byte[MAX_BUF_SIZE];
	private File m_db = null;
   	
	ProbeIdDb() {
		String dirDb = System.getProperty(KEY_PROBEID_DBDIR);
		if (dirDb == null || dirDb.equals("")) 
			dirDb = ProbeId.DEFAULT_DB;
		start(dirDb);
	}
	
	ProbeIdDb(String dirDb) {
		start(dirDb);		
	}
	
	private void start(String dirDb) {
		m_db = new File(dirDb);
		if (!m_db.exists())
			m_db.mkdirs();
		// check whether a probe is attached and writable.
		if (!m_db.canRead()) {
			ProbeIdIO.warning("No Probe is currently attached");
			return;
		}
		//if (!m_db.canWrite()) {
		//	ProbeIdIO.warning("Probe is not attached or not writeable");
		//}
	}
	
	// prepare optional path components for concatenation
	private static String optional(String dir) {
		String sep = File.separator;
		if (dir==null || dir.isEmpty())
			return "";
		else
			return dir.startsWith(sep) ? dir : sep + dir;
	}

	// ignore user if dir is an absolute pathname, i.e. leads with "/"
	private static String optional(String user, String dir) {
		if (dir!=null && dir.startsWith(File.separator))
			return optional(dir.substring(1));
		else 
			return optional(user) + optional(dir);
	}
	
    /**
     * Generate a tag to represent the specified directory.
     * Tries to identify system and user directories with
     * meaningful tag names.
     * @param dir - the appdir associated with this tag
     * @return tag - the tag representing the appdir
     * @throws IOException 
     */
    public static String createAppdirTag(ProbeIdStore db, File dir)
        throws IOException 
    {
        File appdir = dir.getCanonicalFile();
        assert(appdir.canRead());
        String tag = null;

        // check if it's VnmrJ's userdir
        if (tag == null) {
            File usrdir = new File(ProbeIdDb.getUserDir()).getCanonicalFile();
            if (appdir.equals(usrdir))
                tag = ProbeIdDb.getUserId();
        }
        // check if it's VnmrJ's sysdir
        if (tag == null) {
            File sysdir = new File(ProbeIdDb.getSystemDir()).getCanonicalFile();
            if (appdir.equals(sysdir))
                tag = ProbeIdDb.getSysTag();
        }
        // generate a tag
        if (tag == null) {
            String key  = getKeyFromHost(); // top-level beneath hostid
            File dstdir = db.getPath(key);  
            tag = createTag(dstdir);
        }
        return tag;
    }
    
    /**
     * Artificial Tag creation is via a sequence file.  Tags are numerically
     * sequential and maintained in a tag sequence file.  The sequence file
     * is accessed by all instances of the probe server running on the host,
     * so file locking is required.
     * 
     * Caller is expected to use lazy allocation, only allocating when it is
     * certain that a file is to be written to the tag directory.
     * 
     * @param dir - target directory
     * @return unique tag
     * @throws IOException 
     */
    private static String createTag(File dir) throws IOException {
        File file = new File(dir, TagSeqFile);
        RandomAccessFile seqFile = new RandomAccessFile(file,"rw");
        FileChannel channel = seqFile.getChannel();
        FileLock lock = channel.lock();
        BufferedReader reader = new BufferedReader(new FileReader(file));
        int seqNum = 0;
        if (reader != null) {
            String seq = reader.readLine();
            if (seq != null)
                seqNum = Integer.getInteger(seq) + 1;
            reader.close();
        }
        BufferedWriter writer = new BufferedWriter(new FileWriter(file));
        writer.write(Integer.toString(seqNum));
        lock.release();
        channel.close();
        String tag = String.format("%010d",seqNum);
        return tag;
    }
    
    public static String getSysTag() { return SysDirTag; }
    public static String getUsrTag() { return getUserId(); }
    
	// convert a key to a file system path
	private static String getPathNameFromTok(String mount, String dir, Tokenizer tok) {
		String hostid = tok.get("host");
		String userid = tok.get("user");   // userdir is really a special case of appdir
		String appdir = tok.get("appdir"); // actually an appdir tag
		String subdir = tok.get("subdir");
		String fileid = tok.get("file");
		assert(optional(userid).isEmpty() || optional(appdir).isEmpty()); // shouldn't see both at same time

		// mount isn't really "optional", but the effect is the same
		String path = optional(mount) + optional(hostid)
		                              + optional(appdir, dir)
									  + optional(userid, dir)
									  + optional(subdir) + optional(fileid);
		if ( tok.hasMoreTokens() ) {
			path += KEY_SEPARATOR + tok.get("ver"); //tokenValue(tok, "ver", lookahead);
		}
		path += KEY_EXTENSION;
		return path;
	}

	private static File getPathFromTok(String mount, String dir, Tokenizer tok) {
		return new File(getPathNameFromTok(mount, dir, tok));
	}
	
	public String getPathName(String key) {
		return getPathNameFromTok(m_db.getPath(), null, new Tokenizer(key));
	}
	
	public File getPath(String key) {
		return getPath(m_db.getPath(), null, new Tokenizer(key));
	}
	
	public static File getPath(String mount, String key) {
		return getPath(mount, null, new ProbeIdDb.Tokenizer(key));
	}

	/**
	 * Get the ProbeID path including everything except the mount point
	 * @param key
	 * @return path name
	 */
	public static String getSubdir(String key) {
		return getPathNameFromTok(null, null, new Tokenizer(key));
	}
	
	/**
	 * assumes that the "user" component of the key is implicit
	 * in the path variable; key should not contain a "user" component.
	 * @param mount - mount point
	 * @param path  - search path, may be null
	 * @param key   - search key associated with requested file
	 * @return an first file that exists in the specified path or null
	 */
	public static File getPath(String mount, String path, Tokenizer tok) {
		if (mount == null || mount.isEmpty())
			mount = System.getProperty(KEY_PROBEID_DBDIR, DEFAULT_DB);
		
		if (path == null || path.isEmpty()) // no search path specified
			return getPathFromTok(mount, null, tok);
		
		for (String dir : path.split(":") ) { // search path specified
			tok.reset();
			File file = getPathFromTok(mount, dir, tok);
			if (file.exists())
				return file;
		}
		return null;
	}

	public static File getFile(String mount, String path, String key) {
		return getPath(mount, path, new Tokenizer(key));
	}
	
	public String getPath() { return m_db.getPath(); }
	
	private static FileInputStream reader(Tokenizer tok, String path) {
		String mount = System.getProperty(KEY_PROBEID_DBDIR, DEFAULT_DB);
		File file = getPath(mount, path, tok);
		if (file != null) {
			try {
				return new FileInputStream(file);
			} catch (FileNotFoundException e) {
				return null;
			}
		} else return null;
	}

	protected static FileInputStream reader(Tokenizer tok) {
		return reader(tok, null);
	}
	
	public static File getPathFromKey(String mount, String path, String key) {
		return getPathFromTok(mount, path, new Tokenizer(key));
	}
	
	public static FileInputStream getBufferedReader(String key, String path) {
		return reader(new Tokenizer(key), path);
	}
	
	public FileInputStream getInputStreamFromKey(String key) {
		return reader(new Tokenizer(key), null);
	}
	
	/** ProbeIdDb instances are at the end of the line, so ignore the cache parameter */
	public FileInputStream getInputStreamFromKey(String key, boolean cache) {
		return getInputStreamFromKey(key);
	}
	
	public static FileOutputStream prepare(File file, boolean append) {
		try {
			File parent = file.getParentFile();
			if (!parent.exists()) {
				if ( !parent.mkdirs() ) {
					ProbeIdIO.error("could not create ProbeIdDb entry " + parent.getPath());
				}
			}
			assert((file.canWrite() && file.isFile()) || parent.canWrite());
    		return new FileOutputStream(file, append); 
		} catch (IOException ioe) {
			ProbeIdIO.error("ProbeIdDb IO Exception entry " + file.getPath());
			return null;
		}		
	}
	
	public static FileOutputStream writer(Tokenizer tok) {
		String mount = System.getProperty(KEY_PROBEID_DBDIR);
		return prepare(getPath(mount, null, tok), false);
	}

	public static FileOutputStream writer(Tokenizer tok, boolean append) {
		String mount = System.getProperty(KEY_PROBEID_DBDIR);
		return prepare(getPath(mount, null, tok), append);
	}

	public static FileOutputStream getBufferedWriter(String key) {
		return writer(new Tokenizer(key), false);
	}

	public FileOutputStream getOutputStreamFromKey(String key, boolean append) {
		return writer(new Tokenizer(key), append);
	}

	public boolean delete(String key) {
		return rmdir(getPath(key));
	}

	/**
	 * return 1st file that exists in a set of alternate paths under root
	 * @param path
	 * @param alternates
	 * @return
	 */
	public static String select(String root, String[] alternates) {
		for (String alternate : alternates)
			if (new File(optional(root) + optional(alternate)).exists())
				return alternate;
		return null;
	}

	/**
	 * Utility to copy a file.
	 * @param in
	 * @param out
	 */
	public static boolean copy(BufferedReader in, BufferedWriter out) throws IOException {
		int bytes_read = 0;
		try {
			while ((bytes_read = in.read(m_buf, 0, MAX_BUF_SIZE)) > 0)
				out.write(m_buf, 0, bytes_read);
			in.close();
			out.close();
			return true;
		} catch (IOException e) {
			throw new IOException("Copy failed", e);
		}
	}
	
	public static boolean copy(InputStream in, OutputStream out) throws IOException {
		int bytes_read = 0;
		try {
			while ((bytes_read = in.read(m_buffer, 0, MAX_BUF_SIZE)) > 0)
				out.write(m_buffer, 0, bytes_read);
			in.close();
			out.close();
			return true;
		} catch (IOException e) {
			throw new IOException("Copy failed", e);
		}
	}

	public static boolean isLink(File file) throws IOException {
		if (file == null) return false;
		boolean isFile = file.isFile();
		boolean isDir  = file.isDirectory();
		boolean isDead = !isFile && !isDir;           // assume a symbolic dead link
		File parent = file.getParentFile().getCanonicalFile();
		String name = file.getName();
		File child = new File(parent, name);
		String canonical = child.getCanonicalPath();
		String absolute  = child.getAbsolutePath();
		boolean isLive = !absolute.equals(canonical); // assume a live symbolic link
		return isLive || isDead;
	}
	
	
	/**
	 * Utility to recursively remove a directory
	 * @param dir
	 * @return true if and only if the specified file was deleted.
	 */
	public static boolean rmdir(File dir) {
		if (dir != null && dir.exists()) {
			if (dir.isDirectory())
				for (String name : dir.list())
					rmdir(new File(dir, name));
			ProbeIdIO.info("probeid: deleting "+dir);
			return dir.delete();
		} else
			try {
				if (isLink(dir)) {
					return dir.delete();
				} else {
					if (dir != null)
						ProbeIdIO.info("probeid: not deleting "+dir+": no such file");
					return true;
				}
			} catch (IOException e) {
				ProbeIdIO.error("probeid: could not delete "+dir+": "+e.getMessage());
				return false;
			}
	}
	
	private static String optionalKey(String key, String val) {
		return val == null || val.isEmpty() ? "" : key + val;
	}

	// The mount point is ignored when a locally mounted probe is used.
	public static String getProbeIdKey() {
		return new String(" file " + KEY_PROBEID);
	}
	
	public static String getKeyFromFile(String file) {
		return new String("file " + file);
	}
	
	public void append(String key, String value) throws IOException {
		OutputStream ostr = ProbeIdDb.getBufferedWriter(key);
		BufferedWriter owrt = new BufferedWriter(new OutputStreamWriter(ostr));
		owrt.append(value);
		owrt.flush();
		owrt.close();
	}
	
	
	/**
	 *  TODO: replace with a key generated from a less generic and,
	 *  more importantly, less implementation-specific set of sub-keys.
	 * @param host
	 * @param user
	 * @param file
	 * @return String representing the key
	 */
	public static String getKeyFromHostUserFile(String host, 
												String user, 
												String file) 
	{
		return getKeyFromHostUserFile(host, user, null, file);
	}

	public static String getKeyFromHostUserFile(String host, 
												String user,
												String subdir,
												String file) 
	{
		return new String("host " + host + " user " + user + 
						  optionalKey(" subdir ", subdir) + 
						  optionalKey(" file ", file));
	}
	
	public static String getKeyFromHostUserFile(String file) {
		String host = getHostId();
		String user = getUserId();
		return getKeyFromHostUserFile(host, user, file);
	}

	public static String getKeyFromHostUserFile(String subdir, String file) {
		String host = getHostId();
		String user = getUserId();
		return getKeyFromHostUserFile(host, user, subdir, file);
	}

	public static String getAppdirKeyFromHostFile(String appdir, String subdir, String file) {
	    String host = getHostId();
	    return new String("host " + host + " appdir " + appdir + 
	            optionalKey(" subdir ", subdir) + 
	            optionalKey(" file ", file));
	}    

	/**
	 * return a key for either a relative or an absolute path
	 * @param host
	 * @param subdir
	 * @param file
	 * @return
	 */
	public static String getKeyFromHostSubdir(String host, String subdir, String file) {
		if (subdir != null && subdir.startsWith(File.separator)) // absolute
			return new String("host " + host + optionalKey(" subdir ", subdir) + optionalKey(" file ",file));
		else // relative to user
			return getKeyFromHostUserFile(subdir, file);
	}

	public static String getKeyFromHostSubdir(String subdir, String file, Boolean sys) {
		if (sys != null && sys)
			return getKeyFromHostSubdir(subdir != null ? File.separator+subdir : null, file);
		else
			return getKeyFromHostUserFile(subdir, file);
	}
	
	public static String getKeyFromHostSubdir(String subdir, String file) {
		return getKeyFromHostSubdir(getHostId(), subdir, file);
	}
	
	public static String getKeyFromHostFile(String host, String file) {
		return new String("host " + host + " file " + file);
	}

	public static String getKeyFromHostFile(String file) {
		return getKeyFromHostFile(getHostId(), file);
	}

	/**
	 * Get a key for the system or user directory
	 * @return a key appropriate for the specified host and user
	 */
	public static String getKeyFromHostUser() {
		return new String("host " + getHostId() + " user " + getUserId());
	}
	
	public static String getKeyFromHost() {
		return new String("host " + getHostId());
	}
	
	/**
	 * Returns a key to a top-level probe file, i.e. host-, config-, 
	 * and user-independent.
	 * @param file
	 * @return key
	 */
	public static String getKeyFromProbe(String file) {
		return new String("file " + file);
	}

	// FIXME: get rid of unsightly prefix and add a sys parameter
	public static String getKeyFromProbe(String subdir, String file) {
		String subdirKey = "subdir " + subdir + optionalKey(" file ", file);
		if (subdir.startsWith(File.separator))
			return new String(subdirKey);
		else
			return new String("user " + getUserId() + " " + subdirKey);
	}
	
	
	/**
	 * Create a userid string based on system VNMR user id
	 */
	public static String getUserId() {
		return System.getProperty("user.name");
	}
	

	/**
	 * check if key specifies a compressed file name ending in ".zip"
	 * @param key
	 * @return true if the key specifies a filename of form *.zip
	 */
	public static boolean isZip(String key) {
		Tokenizer tok = new Tokenizer(key);
		String name = tok.find("file");
		return name != null && name.endsWith(ZIP_EXTENSION);
	}
	

	/**
	 * Create a hostid string based on mac address
	 */
	static String getHostId() {
		String ethername = System.getProperty(KEY_PROBEID_ETHERPORT, DEFAULT_NIC);
		String hostid = "";
		try {
			NetworkInterface ni = NetworkInterface.getByName(ethername);
			if (ni == null) {
				ProbeIdIO.error("Network Interface "+ethername+" is disabled");
				return null;
			}
			byte[] mac = ni.getHardwareAddress();
			for (byte macbyte : mac) {
				int bytevalue = macbyte & 0xFF; // no unsigned data types in Java
				if (bytevalue < 0x10)
					hostid += "0";
				hostid += Integer.toHexString(bytevalue);
			}
		} catch (SocketException e) {
			ProbeIdIO.error(iam+" error accessing network interface "
					+ethername+" "+e.getMessage());
			e.printStackTrace();
		}
		return hostid;
	}
		
	/**
	 * Return a list of keys for files in the search path matching 
	 * the specified pattern.  May return subdirectories, since
	 * the /probes subdirectory is used to create a list of probes.
	 * @param root - the mount point (cache or probe)
	 * @param path - search paths
	 * @param pattern - file name regular expression
	 * @return
	 */
	static public SortedSet<String> list(String root,
								  		 String paths,
								  		 FilenameFilter pattern)
	{
		SortedSet<String> match = new TreeSet<String>();
		String host = getHostId();
		for (String path : paths.split(File.pathSeparator)) {
			String dirname = optional(root) + optional(host) + optional(path);
			File dir = new File(dirname);
			String [] files = dir.list(pattern);
			if (files != null)
				for (String file : files) {
					String key = ProbeIdDb.getKeyFromHostSubdir(path, file);
					match.add(key);
				}
		}
		return match;
	}

	public SortedSet<String> list(String paths, FilenameFilter pattern)
	{
		return list(m_db.getPath(), paths, pattern);
	}

	/**
	 * List the contents of a subdirectory. Subdirectories are logically
	 * a secondary key to the file name.
	 * Treats system and user directories differently.
	 * @param result
	 * @param root
	 * @param subdir
	 * @param sys
	 * @return
	 */
	public static String[] list(Collection<String> result, String root, String subdir, boolean sys) {
		String host = getHostId();
		String path = null;
		if (sys) {
			String sysDir = File.separator + subdir;
			String sysKey = getKeyFromProbe(sysDir, null);
			path = getSubdir(sysKey);
		} else {
			String usrKey = getKeyFromProbe(subdir, null); // 
			path = getSubdir(usrKey);
		}
		String dirname = optional(root) + optional(host) + optional(path);
		return list(result, dirname);
	}

    public static String[] list(Collection<String> result, String root, String subdir, String key)
    {
        String path = getSubdir(key);
        String dirname = optional(root) + optional(path) + optional(subdir);
        return list(result, dirname);
    }

	public static String[] list(Collection<String> result, String dirname) {
		File dir = new File(dirname);
        // find any file not starting with a period
		FilenameFilter pattern = new RegexFileFilter("^[^.]+.*");
		String [] files = dir.list(pattern);
		if (files != null && result != null)
			for (String file : files) {
				result.add(file);
			}
		return files;		
	}

	public void list(Collection<String> result, String subdir, boolean sys) {
	    list(result, m_db.getPath(), subdir, sys);
	}

	public static boolean addParam(BufferedReader dbin, BufferedWriter dbout, 
								   String nuc, String param, String value) throws IOException 
	{
		Vector<String> val = new Vector<String>(100);
		Vector<String> attr = new Vector<String>(100);
		String key = nuc + param;
		String endOfNuc = nuc + "date"; // end of records for nucleus nuc
		String date = nuc + datestamp();
		boolean foundDate = false;
		boolean foundParam = false;
		if (dbin != null && dbout != null) {
			String line = null;
			while ((line = dbin.readLine()) != null) {
				ProbeIdIO.debug("matching line: "+line);
				Tokenizer tok = new Tokenizer(line);
				String left=tok.nextToken().trim();
				String right=tok.nextToken().trim();
				if (left==null || right==null) { 
					// unparseable or empty line
					attr.add(left); val.add(right);
					continue;
				}
				if (key.equals(left)) { // matching param found
					attr.add(left); val.add(value);
					foundParam = true;
				} else if (key.equals(endOfNuc)) {
					if (!foundParam) {
						attr.add(key); val.add(value);
					}
					attr.add(endOfNuc); val.add(date);
					foundDate = true;
				}
			}
			if (!foundDate && !foundParam) {
				attr.add(nuc+":"); val.add("Parameters"); // table header
				attr.add(key); val.add(value);			  // the sole key/value pair
				attr.add(endOfNuc); val.add(date); 		  // the datestamp
			}
			saveparams(dbout, val, attr);
		}
		return true;
	}

	public static void saveparams(BufferedWriter dbout, 
								  Vector<String> val, Vector<String> attr) 
		throws IOException 
	{
		for (int i=0; i<attr.size(); i++) {
			String left = attr.get(i), right = val.get(i);
			if (left == null)
				dbout.write('\n');
			else if (right == null)
				dbout.write(left+'\n');
			else {
				String line = String.format(KEY_VALUE_FORMAT, left, right);
				dbout.write(line+'\n');
			}
		}
		dbout.flush();
	}
	
	/**
	 * Analogous to VNMR getparam built-in.  Values are assumed to
	 * be white-space separated key-value pairs.
	 * 
	 * @param db         - a BufferedReader to a key value file
	 * @param param		 - the key to match 
	 * @returns value of matching key as a String
	 * @throws IOException
	 */
	public static String getParam(BufferedReader db, String param) throws IOException {
		if (db != null) {
			String line = null;
			while ((line = db.readLine()) != null) {
				Tokenizer tok = new Tokenizer(line);
				ProbeIdIO.debug("matching line: "+line);
				if (param.equals(tok.nextToken().trim()))
					return tok.nextToken().trim();
			}
		}
		return null;
	}

	/**
	 * Return the probeId from the perspective of a member of the
	 * cache hierarchy.  Since this is the lowest level, that means
	 * the probe ID from the hardware.
	 */
	public String getProbeId() {
	    try {
            return readProbeId();
        } catch (IOException e) {
        } catch (ProbeIdMismatchException e) {
        }
        return "";
	}
	
	/**
	 * Read the probe ID out of the database
	 * @return
	 * @throws IOException
	 * @throws ProbeIdMismatchException 
	 */
	public static String readProbeId() throws IOException, ProbeIdMismatchException {
		String key = ProbeIdDb.getProbeIdKey();
		InputStream dbReader = ProbeIdDb.getBufferedReader(key, null);
		if (dbReader == null)
			throw new ProbeIdMismatchException("Probe ID file not accessable");
		BufferedReader reader = new BufferedReader(new InputStreamReader(dbReader));
		return getParam(reader, ProbeId.PARAM_KEY_PROBEID);
	}

	/**
	 * Returns the contents of a table in a vector of rows
	 * @param db
	 * @param prefix
	 * @return
	 * @throws IOException
	 */
	public static Collection<String> readFile(BufferedReader db, String prefix) throws IOException {
		Vector<String> match = new Vector<String>();
		if (prefix == null) prefix = "";
		if (db != null) {
			String line = null;
			while ((line = db.readLine()) != null)
				if (line.startsWith(prefix))
					match.add(line);
		}
		return match;
	}
	
	/**
	 * get modification time of first record matching pattern
	 * @param root
	 * @param paths
	 * @param pattern
	 * @return modification time
	 */
	public static Long lastModified(String root, String paths, 
							 		FilenameFilter pattern) 
	{
		Long mod = null;
		SortedSet<String> keys = list(root, paths, pattern);
		if (!keys.isEmpty()) {
			String key = keys.first();
			File file = getFile(null, null, key);
			mod = file.lastModified();
		}
		return mod;
	}
	
	public Long lastModified(String paths, FilenameFilter pattern) {
		return lastModified(m_db.getPath(), paths, pattern);
	}
	
	/**
	 * Create the required file system structure.  It currently
	 * presumes a mounted probe filesystem, and would need to be changed.
	 * This is acceptable because this is part of the factory process.
	 * @param probeMntPt
	 * @param subdirs
	 * @return
	 */
	static private boolean mkdirs(String probeMntPt, String subdirs) {
		String hostDir = optional(probeMntPt) + optional(getHostId());
		String dataDir = optional(hostDir) + optional(getUserId(), subdirs);
		File dataDirs = new File(dataDir);
		return dataDirs.mkdirs();
	}
	
	public static String timestamp() {
		SimpleDateFormat ext = new SimpleDateFormat("yyMMddhhmmssSS");
		return "." + ext.format(Calendar.getInstance().getTime());
	}

	public static String datestamp() {
		SimpleDateFormat ext = new SimpleDateFormat("dd-MMM-yy");
		return ext.format(Calendar.getInstance().getTime());		
	}
	
	public boolean copy(String keySrc, String keyDst) throws IOException {
		File dirSrc = getPathFromKey(getPath(), null, keySrc);
		File dirDst = getPathFromKey(getPath(), null, keyDst);
		return copy(dirSrc, dirDst);
	}

	/**
	 * copy from one ProbeID database table to another
	 * @param src
	 * @param dst
	 * @throws IOException
	 * 
	  	// TODO: Replace with Path.copyTo(src,dst,ATOMIC)
		File dir = old.getParentFile();
		File tmp = File.createTempFile("$commit$", DEFAULT_TMP_EXT, dir);
		boolean renamed  = old.renameTo(tmp);
	 */
	public static boolean copy(File src, File dst) throws IOException {
		if (src.exists()) {
			dst.getParentFile().mkdirs();
			String cmd = "cp -r " + src.getPath() + " " + dst.getPath();
			String tmp = dst.getPath() + timestamp();
			File tmpFile = null;  // for atomicity
			ProbeIdIO.info("ProbeIdDb: copying probe: " + cmd);
			
			if (dst.exists()) 			   // copy will overwrite existing file
				dst.renameTo(tmpFile = new File(tmp));

			try {
				Process copying = Runtime.getRuntime().exec(cmd);
				copying.waitFor();		   // make this a synchronous operation
				ProbeIdDb.rmdir(tmpFile);  // destroy replaced files
				return true;
			} catch (InterruptedException e) {
				Messages.postError("ProbeIdDb: interrupted wating for probe copy");
				if (tmpFile.exists()) {    // recover from copy error
					ProbeIdDb.rmdir(dst);  // get rid of partial copy
					tmpFile.renameTo(dst); // restore original
				}
			}
			return false;
		} else // if the source doesn't exist, no need to copy
			return false;
	}

	public static String getUserDir()   { return VnmrProxy.getUserDir(); }
    public static String getSystemDir() { return VnmrProxy.getSystemDir(); }
    
	public static boolean exists(ProbeIdStore db, String key) {
	    File tbl = db.getPath(key);
	    return tbl.exists();
	}
	
	/** add an optional subdir and/or file component to existing key;
	 * @param key
	 * @param subdir
	 * @param file
	 * @return
	 */
	public static String addToKey(String key, String[] subdirs, String file) {
	    String path = ""; // partial path
	    String sep = "";
	    if (subdirs != null)
	        for (String subdir : subdirs) {
	            path += sep + subdir;
	            sep = File.separator;
	        }
	    return key + optionalKey(" subdir ", path)
	               + optionalKey(" file ", file);
	}
	
	public static boolean canWrite(ProbeIdStore db, String key) {
	    File path = db.getPath(key);
	    return path != null && path.canWrite();
	}

	/**
	 * A pair of file names
	 * @author vnmr1
	 */
	public static class SrcDst {
		public File src;
		public String dst;
		public SrcDst(File src, String dst) {
			this.src = src; 
			this.dst = dst;
		}
		
		/**
		 * Import a file in the given subdirectory to the system.  Will import
		 * both the user and system versions, if they exist.  Source may be a 
		 * directory, in which case file parameter should be set to null.
		 * The key will subsequently need to be augmented with the filename.
		 * Example: 
		 *   /vnmr/probes/autox400DB/autox400DB 
		 * 	  -> /mnt/probe/000c29181c28/probes/autox400DB/autox400DB
		 *   import("autox400DB", "probes/autox400DB", "autox400DB")
		 * @param legacy - legacy name for probe (i.e. probe configuration)
		 * @param subdir - secondary key
		 * @param file   - primary key
		 */
		public static Collection<SrcDst> getImportFiles(String legacy, 
														String subdir, 
														String file,
														Boolean sys) 
		{
			Collection<SrcDst> files = new LinkedList<SrcDst>();
			//assert(legacy == null && subdir == null);
			if (sys != null && sys) {
				String sysSrcDir = getSystemDir();
				String sysSrc = optional(sysSrcDir) + optional(subdir) + optional(file);
				File   sysSrcFile = new File(sysSrc);
				getImportDirectory(files, sysSrcFile, subdir, file, true);
			} else {
				String usrSrcDir = getUserDir();
				String usrSrc = optional(usrSrcDir) + optional(subdir) + optional(file);
				File   usrSrcFile = new File(usrSrc);
				getImportDirectory(files, usrSrcFile, subdir, file, false);
			}
			return files;
		}
		
		/**
		 * recursively import specified directory
		 * @param files  - collection of files to import
		 * @param src    - source path (may be a directory)
		 * @param subdir - destination subdirectory
		 * @param dst    - destination file
		 * @param sys    - true if a system file
		 */
		private static void getImportDirectory(Collection<SrcDst> files, 
											   File src, 
											   String subdir, String dst, 
											   boolean sys) 
		{
			if (!src.exists()) // don't process nonexistent files and directories
				return;
			if (src.isDirectory()) {
				for (String child : src.list()) {
					File srcFile = new File(src.getPath() + File.separator + child);
					if (srcFile.isDirectory()) {
						String dstPath = subdir + File.separator + child;
						getImportDirectory(files, srcFile, dstPath, null, sys);
					} else {
						getImportDirectory(files, srcFile, subdir, child, sys);
					}
				}
			} else {
				// FIXME: the leading '/' is a kludge - fix the API!
				String dstDir = (sys ? File.separator : "") + subdir;
				String dstKey = null;
				if (sys)
					dstKey = getKeyFromHostSubdir(dstDir, dst);
				else
					dstKey = getKeyFromHostUserFile(dstDir, dst);
				files.add(new SrcDst(src, dstKey));
			}
		}
	};

	/**
	 * Initialize the Probe's internal structures
	 * @param path
	 */
	public File init(String path) {
		makeProbeDirs(getPath(), path);
		return new File(getPath());
	}
	
	/**
	 * Return a userdir style path; system files are encoded
	 * with a subdir that starts with a file separator ("/" on linux).
	 * @param subdir
	 * @return
	 */
	public static String relativeForm(String subdir) {
		if (subdir != null && subdir.startsWith(File.separator))
			return subdir.substring(1);
		return subdir;
	}
	
	/**
	 * Initialize the ProbeId file system for test purposes.
	 * to the time stamp to avoid conflicts between 2 different systems.
	 * @throws IOException 
	 */
	static public Vector<String> init(String root, String id, String path) {
		Vector<String> made = null;
		if (root==null || !root.startsWith(DEFAULT_TMP_DIR)) {
			System.err.println("probeid: init expects root file in "+DEFAULT_TMP_DIR+" (root="+root+")");
			return null;
		}
		try {
			String probeIdFile = root + File.separator + KEY_PROBEID + KEY_EXTENSION;
			Date date = new Date();
			Timestamp timestamp = new Timestamp(date.getTime());
			String probeIdValue = (id == null) ? timestamp.toString() : id;
			made = makeProbeDirs(root, path);
			BufferedWriter out = new BufferedWriter(new FileWriter(new File(probeIdFile)));
			ProbeIdKeyValueIO.write(out, "ID:", "Parameter");
			ProbeIdKeyValueIO.write(out, PARAM_KEY_PROBEID,probeIdValue);
			out.flush();
		} catch (IOException e) {
			ProbeIdIO.error(iam+" init I/O error "+e.getMessage());
			e.printStackTrace();
		}
		return made;
	}

	/**
	 * @param root
	 * @param path
	 * @param made
	 */
	public static Vector<String> makeProbeDirs(String root, String path) {
		Vector<String> made = new Vector<String>();
		if (path != null) 
			for (String dir : path.split(":") ) // search path specified
				if (mkdirs(root, dir))
					made.add(root + File.separator + dir);
		return made;
	}

	/**
	 * Flush outstanding transactions to disk; since an instance of ProbeIdDb
	 * is the final destination, simply returns true.
	 */
	@Override
	public boolean flush(String id) {
		return true;
	}
	
	public void cleanup(String[] dirs) {
		for (String dir : dirs) {
			if (dir.startsWith(DEFAULT_TMP_DIR)) {
				rmdir(new File(dir));
			} else {
				ProbeIdIO.error("cleanup <"+dir+">: not rooted in "+DEFAULT_TMP_DIR);
			}
		}
	}
	
	public void init(String id, String path) {
		init(m_db.getPath(), id, path);
	}
	
	/**
	 * utility to facilitate unit testing
	 * @param key
	 * @param data
	 * @return test file string
	 * @throws IOException
	 */
	private static String testFileIO(String key, String path, String data) throws IOException
	{
		testFileWrite(key, data);
		return testFileRead(key, path);
	}

	private static String testFileIO(String key, String data) throws IOException
	{
		return testFileIO(key, null, data);
	}

	private static String testFileRead(String key, String path) throws IOException
	{
		char[] buf = new char[1024];
		InputStreamReader reader = new InputStreamReader(getBufferedReader(key, path));
		if (reader != null) {
			reader.read(buf);
			reader.close();	
			return buf.toString();
		}
		return null;
	}
	
	private static void testFileWrite(String key, String data) throws IOException
	{
		OutputStreamWriter writer = new OutputStreamWriter(ProbeIdDb.getBufferedWriter(key));
		writer.write(data);
		writer.close();		
	}
	
	public static void main(String[] args)
	{
		// set system properties for all tests
		String mountPoint = "/tmp/probe_mnt";
		System.setProperty(KEY_PROBEID_DBDIR, mountPoint);
		System.setProperty(KEY_PROBEID_ETHERPORT, DEFAULT_NIC);
		String id   = "TEST-PROBEID-01234-56789";
		String path = "tunecal_autox400DB"
					  +File.pathSeparator+"autox400DB"
					  +File.pathSeparator+"tunecal";

		// initial population of ProbeIdDb
		ProbeIdDb db = new ProbeIdDb();
		new File(mountPoint).mkdir();
		db.init(id, path);
		
		// test 1: read probe ID file
		try {
			String key1 = ProbeIdDb.getProbeIdKey();
			String contents = testFileIO(key1, id);
			assert(contents == id);
			System.out.println("test1 PASSED");
		} catch (IOException e) {
			e.printStackTrace();
			System.out.println("test1 FAILED");
		}
		
		// test 2: read host default file and confirm absolute pathnames
		try {
			String filename = "HostTest";
			String hostname = getHostId();
			String pathname = mountPoint 
							+ "/" + hostname 
							+ "/" + filename;
			String key2 = ProbeIdDb.getKeyFromHostFile(filename);
			String data = "Duck = quack\nCow = moo\n";
			String contents = testFileIO(key2, data);
			assert(contents.equals(data));
			
			// confirm absolute pathname
			FileInputStream shouldbe = new FileInputStream(pathname);
			FileInputStream actual = new FileInputStream(db.getPath(key2));
			assert(shouldbe.getFD() == actual.getFD());
			System.out.println("test2 PASSED");
		} catch (IOException e) {
			e.printStackTrace();
			System.out.println("test2.FAILED");
		}
		
		// test 3: read host+user file
		try {
			String filename = "HostUserTest";
			String hostname = getHostId();
			String username = System.getProperty("user.name");
			String pathname = mountPoint 
							+ File.separator + hostname 
							+ File.separator + username
							+ File.separator + filename;
			String key3 = ProbeIdDb.getKeyFromHostUserFile(hostname, username, filename);
			String data = "Duck = quack\nCow = moo\n";
			String contents = testFileIO(key3, data);
			assert contents == data;
			
			// confirm absolute pathname
			FileInputStream shouldbe = new FileInputStream(pathname);
			FileInputStream actual = new FileInputStream(db.getPath(key3));
			assert shouldbe.getFD() == actual.getFD();
			System.out.println("test3 PASSED");
		} catch (IOException e) {
			e.printStackTrace();
			System.out.println("test3 FAILED");
		}
		
		// test 4: read host+path file
		try {
			String usrfile   = "UserPathTest";  // find this in user area
			String sysfile   = "SysPathTest";   // find this in system area
			String nofile    = "NoPathTest";    // don't find this at all
			String username  = System.getProperty("user.name");
			String usrpath   = username + "/vnmrsys/test";
			String syspath   = "/vnmr/test";
			String boguspath = "/bogus";
			String search    = boguspath + ":" + usrpath + ":" + syspath;
			String usrkey    = ProbeIdDb.getKeyFromHostFile(usrfile);
			String syskey    = ProbeIdDb.getKeyFromHostFile(sysfile);
			String notkey    = ProbeIdDb.getKeyFromHostFile(nofile);
			String absusrkey = ProbeIdDb.getKeyFromHostFile(usrpath + "/" + usrfile);
			String abssyskey = ProbeIdDb.getKeyFromHostFile(syspath + "/" + sysfile);
			String usrdata   = "iam what iam";
			String sysdata   = "quod erat demonstrandum";
			testFileWrite(absusrkey, usrdata);
			testFileWrite(abssyskey, sysdata);
			
			// read the files 
			String usrcontent = testFileRead(usrkey, search);
			String syscontent = testFileRead(syskey, search);
			String nilcontent = testFileRead(notkey, search);
			assert usrcontent == usrdata;
			assert syscontent == sysdata;
			assert nilcontent == null;
			
			// confirm absolute pathname (white box test)
			String root = mountPoint + "/" + getHostId() + "/";
			FileInputStream usrideal  = new FileInputStream(root + usrpath + "/" + usrfile);
			FileInputStream sysideal  = new FileInputStream(root + syspath + "/" + sysfile);
			FileInputStream usractual = new FileInputStream(ProbeIdDb.getFile(null, path, usrkey));
			FileInputStream sysactual = new FileInputStream(ProbeIdDb.getFile(null, path, syskey));
			assert(usrideal.getFD() == usractual.getFD());
			assert(sysideal.getFD() == sysactual.getFD());
			System.out.println("test4 PASSED");
		} catch (IOException e) {
			e.printStackTrace();
			System.out.println("test 4 FAILED");
		}
	}

	@Override
	public boolean isCached(String key) { return false; }
}
