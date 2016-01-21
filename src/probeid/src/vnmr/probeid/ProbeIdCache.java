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
import java.util.Collection;

/**
 * ProbeIdCache options:
 * 1) intercept calls to database
 * 
 * 2) actual database is on the cache filesystem and we just
 *    load or flush it to the flash filesystem.
 * 
 * 3) copy all files over to the cache, mark the cache as
 *    dirty.
 *    
 * FIXME: add ability to mark directories or files as "don't cache",
 *        perhaps with a ".dont_cache" subdirectory for directories
 *        or a permissions bit of "g+x" for individual files (as much
 *        as I hate overloading those bits).
 * 
 * Advantages of latter approach:
 *   only need to replicate the data to/from cache
 * 
 * Mark changed files as dirty by changing write permissions.
 * 
 * @author dirk
 *
 */
public class ProbeIdCache implements ProbeId, ProbeIdStore {
	public String probeId = "";         // probe this cache is locked to

	private static String m_trashDirName = System.getProperty(KEY_PROBEID_TRASH, ProbeId.DEFAULT_TRASH);
	private final File m_cacheDir;       
	private final File m_timestamp;
	private ProbeIdStore m_backingStore; // the cached object
	public class ProbeIdCacheReader extends FileInputStream {
		public ProbeIdCacheReader(InputStream store, File cached) 
			throws FileNotFoundException
		{
			super(cached);
		}
	}

	/**
	 * Create an output file cache writer.  For now that the fact that
	 * the stream is created implies that it will always be written to,
	 * otherwise we'll have to override the 3 super.write(...) methods.
	 * 
	 * flush() doesn't do anything because the same file will potentially
	 * be written many times, and flushing will be handled by the cache
	 * itself.
	 * 
	 * @author dirk
	 *
	 */
	public class ProbeIdCacheWriter extends FileOutputStream {
		public ProbeIdCacheWriter(OutputStream store, File cached, boolean append) 
			throws FileNotFoundException
		{
			super(cached, append);
		}
	}

	/**
	 * Constructor for an ephemeral cache, such as a directory in the
	 * native VnmrJ file system.
	 * @param cacheDir
	 * @param backingStore
	 * @param probeId
	 */
	ProbeIdCache(File cacheDir, ProbeIdStore backingStore, String probeId)
	{
		m_backingStore = backingStore;
		m_cacheDir = cacheDir;
		this.probeId = probeId;
		// initialize a timestamp to determine whether to flush the cache
		m_timestamp = new File(m_cacheDir, DEFAULT_FLUSH_FILENAME);
	}
	
	ProbeIdCache(String cacheDirName, ProbeIdStore backingStore, String probeId)
		throws IOException, ProbeIdMismatchException 
	{
		m_backingStore = backingStore;
		
		// and then check the default directory
		if (cacheDirName == null || cacheDirName.isEmpty())
			cacheDirName = ProbeId.DEFAULT_CACHE;
		
		// and then complain
		if (cacheDirName == null)
			throw new IOException();

		// create the cache directory based on cache root and Probe Id string
		m_cacheDir = new File(cacheDirName + File.separator + probeId);
		if (!m_cacheDir.exists())
			m_cacheDir.mkdirs();
		
		// initialize a timestamp to determine whether to flush the cache
		m_timestamp = ProbeIdTinyFiles.getFlushFile(m_cacheDir);
		
		// sanity checks
		assert(m_cacheDir.isDirectory());
		assert(m_cacheDir.canWrite());
		
		if (probeId == null) // ostensibly an uninitialized probe
			throw new ProbeIdMismatchException("Cache Probe ID doesn't match Physical Probe ID", this.probeId);
		// lock this cache to a particular physical probe
		this.probeId = probeId;
	}
	
	public String getPath()         { return m_cacheDir.getPath(); }
	private String getCacheRoot()   { return m_cacheDir.getParent(); }
	private File getCacheRootFile() { return m_cacheDir.getParentFile(); }
	private File getCacheRootFile(String id) {
		return new File(getCacheRootFile(), id);
	}
	 
	public static boolean checkPath(String cacheRoot, String probeId) {
		String root = cacheRoot!=null
				? cacheRoot 
				: System.getProperty(ProbeId.KEY_PROBEID_CACHE, ProbeId.DEFAULT_CACHE);
		File cacheDir = new File(root + File.separator + probeId);
		return cacheDir.exists() && cacheDir.canRead();
	}
	
	/** list each probe ID in the cache */
	public void listProbeIDs(Collection<String> results, boolean disconnected) {
		ProbeIdDb.list(results, getCacheRoot());
		if (disconnected) {
			boolean deleted = results.remove(probeId);
			assert(deleted);
		}
	}
	
	/** list contents */
	public void list(Collection<String> results, String id, String subdir, boolean sys) {
		File root = getCacheRootFile(id);
		ProbeIdDb.list(results, root.getPath(), subdir, sys);
	}
	
	/** list contents of specified appdir */
    public void list(Collection<String> results, String id, String path, String key) {
        File root = getCacheRootFile(id);
        ProbeIdDb.list(results, root.getPath(), path, key);
    }

	/**
	 * return a File to the specified path.  
	 * This is trickier than it looks, since this is a cache and the
	 * path may refer to a file that isn't in the cache but could be.
	 * 
	 * FIXME: this needs to be reworked a bit in order to work smoothly
	 * with remote probes.  It's good enough for prototyping purposes for now.
	 */
	public File getPath(String key) {
		File cached = null;
		if ( m_cacheDir != null) {
			cached = ProbeIdDb.getFile(getPath(), null, key);
			if (!cached.exists()) {
				File mounted = probeFile(cached.getPath());
				if (mounted != null && mounted.exists())
					return mounted;
			}
		}
		return cached;
	}
	
	/**
	 * Cache the specified table.  Provided to support BLOBs, which
	 * users can write to directly and should be written only to the
	 * cache.
	 * @param key
	 * @return
	 * @throws IOException 
	 */
	public File cache(String key) throws IOException {
		File cached = null;
		if (m_cacheDir != null) {
			cached = ProbeIdDb.getFile(getPath(), null, key);
			if (!cached.exists()) {
				File mounted = probeFile(cached.getPath());
				if (mounted != null && mounted.exists())
					try {
						copy(mounted, cached);
					} catch (IOException e) {
						ProbeIdIO.error("I/O error caching "+mounted.getPath()+" in "+cached.getPath());
					}
			}
		}
		return cached;
	}
	
	public String getPathName(String key) {
		File file = getPath(key);
		return file == null ? null : file.getPath();
	}
	
	public static File getCached(String mount, String key) {
		return ProbeIdDb.getFile(mount, null, key);
	}
	
	public File getCached(String key) {
		return ProbeIdDb.getFile(getPath(), null, key);
	}
	
	public static boolean isCached(String mount, String key) {
		if (mount != null) {
			File path = ProbeIdDb.getFile(mount, null, key);
			if (path != null) 
				return path.exists();
		}
		return false;
	}

	public boolean isCached(String key) {
		return isCached(getPath(), key);
	}
	
	private File getTrash(File file, boolean create) {
	    File parent   = file.getParentFile();
	    File trashDir = new File(parent + File.separator + m_trashDirName);
	    if (create && !trashDir.exists())
	    	if (!trashDir.mkdirs()) {
	    		ProbeIdIO.error("could not create trash folder "+trashDir.getPath());
	    		return null;
	    	}
	    return new File(trashDir, file.getName());
	}

	private boolean deleted(File file) {
		if (file == null || file.getName().equals(m_trashDirName))
			return false;
		File trashFile = getTrash(file, false);
		return !file.exists() && trashFile.exists();		
	}
	
	private boolean trashed(File file) {
		if (file == null || file.getName().equals(m_trashDirName))
			return false;
		String parentName = file.getParentFile().getName();
		return parentName.equals(m_trashDirName);
	}
	
	/**
	 * If a file isn't cached, we have 2 options: either mark it
	 * as deleted and then delete it when we flush, or delete the
	 * file on the probe.  For now it is marked for deletion.
	 * @throws IOException 
	 */
	public boolean delete(String key) throws IOException {
	    File file  = getCached(key);
    	File trash = getTrash(file, true);
    	if (trash == null)
    		return false;
    	if (trash.exists())
    		ProbeIdDb.rmdir(trash);
	    if (file.exists()) {
			ProbeIdIO.info("probeid: moving "+file.getPath()+" to wastebasket "+trash.getPath());
			if (!file.renameTo(trash)) {
				ProbeIdIO.error("probeid: could not move "+file+" to wastebasket");
				return false;
			}
	    } else { // it's not in the cache but could be on probe
			ProbeIdIO.info("probeid: marking "+file+" for deletion");
			trash.createNewFile();
		}
	    return true;
	}
	
	public String getProbeId() { return probeId; }
	
	public void list(Collection<String> results, String subdir, boolean sys) {
		ProbeIdDb.list(results, getPath(), subdir, sys);
		if (probeId.equals(m_backingStore.getProbeId()))
		    m_backingStore.list(results, subdir, sys);
	}
	
	/**
	 * Return an InputStream associated with a cached file.  Indicate
	 * that it is used as a read cache entry by making it readable.
	 * @param key   - key
	 * @param path  - search path (i.e. user and system)
	 * @param store - store InputStream
	 * @return an InputStream to the cached file
	 * @throws IOException
	 */
	public FileInputStream getBufferedReader(String key, String path, InputStream store)
		throws IOException 
	{
		FileInputStream istream = null;
		File cacheFile = ProbeIdDb.getFile(getPath(), path,  key);
		if (store == null)
			store = m_backingStore.getInputStreamFromKey(key);
		if (store != null) { // there's a backing store 
			if (!cacheFile.exists() && !deleted(cacheFile)) {
				cacheFile.getParentFile().mkdirs();
				BufferedWriter out = new BufferedWriter(new FileWriter(cacheFile));
				BufferedReader in  = new BufferedReader(new InputStreamReader(store));
				ProbeIdDb.copy(in, out);
			}
		}
		if (cacheFile != null && cacheFile.exists()) {
			try {
				istream = this.new ProbeIdCacheReader(istream, cacheFile);
			} catch (FileNotFoundException e) {
				ProbeIdIO.error("I/O error caching "+path);
				throw e;
			}
		}
		return istream;
	}
	
	public InputStream getInputStreamFromKey(String key)
		throws IOException 
	{
		return getBufferedReader(key, null, null);
	}
	
	public InputStream getInputStreamFromKey(String key, boolean cache)
		throws IOException 
	{
		if (cache)
			return getInputStreamFromKey(key);
		else
			return m_backingStore.getInputStreamFromKey(key, cache);
	}

	/**
	 * Return an OutputStream for the cache file corresponding to the
	 * given stream.  It is marked "dirty" by being made writable.
	 * @param cache directory
	 * @param key
	 * @param ostream is an OutputStream interface to the backing store
	 * @return OutputStream to cache file
	 * @throws FileNotFoundException
	 */
	public FileOutputStream getOutputStreamFromKey(
			String key, OutputStream ostream, boolean append) 
		throws FileNotFoundException 
	{
		File cacheFile = ProbeIdDb.getFile(getPath(), null, key);
		if (!cacheFile.exists()) {
			cacheFile.getParentFile().mkdirs();
			//cacheFile.setReadable(false);
		}
		//cacheFile.setWritable(true);
		ProbeIdIO.debug("probeid: cache "+cacheFile.getPath());
		return this.new ProbeIdCacheWriter(ostream, cacheFile, append);
	}
	
	public FileOutputStream getBufferedWriter(String key, OutputStream ostream) 
		throws FileNotFoundException 
	{
		return getOutputStreamFromKey(key, ostream, false);
	}

	public FileOutputStream getOutputStreamFromKey(String key, boolean append) 
		throws FileNotFoundException
	{
		return getOutputStreamFromKey(key, null, append);
	}
	
	public boolean checkProbeId() throws ProbeIdMismatchException {
	    assert(probeId != null && !probeId.isEmpty());
	    assert(m_backingStore != null);
		return probeId.equals(m_backingStore.getProbeId());
	}
	
	/**
	 * FIXME: this should be based on key, not on file, since it breaks the
	 * paradigm that the the ProbeIdCache shouldn't know the implementation 
	 * details of the ProbeIdDb (in this case, file system layout).
	 * 
	 * @param cache file path
	 * @return equivalent Probe ID file path
	 */
	File probeFile(String cacheFilePath) {
		File probeFile = null;
		assert(cacheFilePath.startsWith(m_cacheDir.getPath()));
		int cachePathLen = m_cacheDir.getPath().length();
		// get the part of cache file independent portion of the path
		String relPath = cacheFilePath.substring(cachePathLen);
		// get the probe mount point 
		String mountPoint = m_backingStore.getPath();
		assert(mountPoint.equals(System.getProperty(KEY_PROBEID_MOUNTPOINT, 
													DEFAULT_LOGICAL_MNT)));
		try {
			if (checkProbeId())
				probeFile = new File(mountPoint + relPath);
		} catch (ProbeIdMismatchException e) {
			// indicate mismatch or disconnect by returning null;
		}
		return probeFile;
	}
	
	/**
	 * Don't delete the existing files, but copy modified ones to the
	 * Probe and update the existing cache files if any have been modified.
	 * It is the responsibility of the caller to make sure that the
	 * current probe ID matches actual probe ID.
	 * 
	 * @throws IOException 
	 * @throws ProbeIdMismatchException if the cached probe doesn't match the requested probe
	 */
	@Override
	public boolean flush(String id) 
		throws IOException, ProbeIdMismatchException 
	{
		if (this.probeId.equals(probeId)) {
			if (checkProbeId()) {
				synchronized(m_cacheDir) {
					String hostKey = ProbeIdDb.getKeyFromHost();
					File hostDir = ProbeIdDb.getFile(getPath(), null, hostKey);
					if (flush(hostDir))
						ProbeIdTinyFiles.timestampFlush(m_cacheDir);
				}
			}
		} else
			throw new ProbeIdMismatchException(
					"Cache Probe ID '" + id 
					+ "' does not match Physical Probe ID", this.probeId);
		return false;
	}
	
	private String deletedFileName(File file) {
		return file.getParentFile().getParent() + File.separator + file.getName();
	}
	
	private boolean flush(File file) throws IOException {
	 	if (file == null || !file.exists()) return false;
		boolean flushed = true;
		if (trashed(file)) {
			File probeFile = probeFile(deletedFileName(file));
			flushed &= ProbeIdDb.rmdir(probeFile);
			flushed &= ProbeIdDb.rmdir(file);
		} else if (file.isDirectory()) {
			// process the trash directory first, in case a deleted object was added back
			File trash = new File(file, m_trashDirName);
			if (trash.exists())
				flushed &= flush(trash);
			// recursively flush the remaining files and directories
			for (String name : file.list()) // trash should be empty
				flushed &= flush(new File(file, name));
		} else {
			if (ProbeIdTinyFiles.needsFlush(m_timestamp, file))
				flushed &= copy(file, probeFile(file.getPath()));
		}
		return flushed;
	}
	
	/**
	 * Flush for native files and directories
	 * @param id
	 * @param src
	 * @param path
	 * @param file
	 * @param sys
	 * @return
	 * @throws IOException
	 */
	private boolean flush_(File src, String path, String file, boolean sys) 
		throws IOException 
	{
		boolean flushed=true;
		if (!src.exists()) return false;
		if (src.isDirectory()) {
			for (String name : src.list()) {
				File child = new File(src, name);
				boolean isDir = child.isDirectory();
				String dstPath = isDir ? path + File.separator + name : path;
				String dstFile = isDir ? null : name;
				flushed &= flush_(child, dstPath, dstFile, sys);
			}
			return flushed;
		} else {
			String key = ProbeIdDb.getKeyFromHostSubdir(path, file, sys);
			File dst = ProbeIdDb.getPathFromKey(m_backingStore.getPath(), null, key);
			if (m_timestamp == null || !src.equals(m_timestamp))
				if (ProbeIdTinyFiles.needsFlush(m_timestamp, src))
					flushed &= copy(src, dst);
		}
		return flushed;
	}
	
	public boolean flush(File src, String path, String file, boolean sys) 
		throws IOException 
	{
		boolean flushed = flush_(src,path,file,sys);
		if (flushed)
			ProbeIdTinyFiles.timestamp(m_timestamp);
		return false;
	}

	/**
	 * Copy a file
	 * @param inFile
	 * @param outFile
	 * @throws IOException
	 */
	private static boolean copy(File inFile, File outFile) throws IOException {
		if (inFile.isDirectory())
			return ProbeIdDb.copy(inFile, outFile);
		if (outFile != null) {
		    boolean ok = outFile.getParentFile().mkdirs();
		    if (!ok) {
		    	ProbeIdIO.error("Could not create probe flush directory "+outFile.getParentFile().getPath());
		    } else {
		    	BufferedWriter out = new BufferedWriter(new FileWriter(outFile));
		    	BufferedReader in  = new BufferedReader(new FileReader(inFile));
		    	return ProbeIdDb.copy(in, out);
		    }
		}
		return false;
	}

	/**
	 * ProbeId internal database table replication program
	 * @param keySrc - search key for source
	 * @param keyDst - search key for destination
	 * @return true if copy succeeded or there was nothing to copy
	 * @throws IOException
	 */
	public boolean copy(String keySrc, String keyDst) throws IOException {
		File src = cache(keySrc);
		File dst = ProbeIdDb.getPathFromKey(getPath(), null, keyDst);
		boolean exists = src != null && src.exists();
		return exists ? ProbeIdDb.copy(src, dst) : false;
	}
	
	public static void main(String [] args)
	{
		// run unit tests
		org.junit.runner.JUnitCore.main("vnmr.apt.ProbeIdCacheTest");
	}
}
