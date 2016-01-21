/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;


import static org.junit.Assert.*;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.Vector;

import org.junit.*;

public class ProbeIdCacheTest {
	static String id           = "TEST-PROBEID-01234-56789";
	static String path         = "tunecal_autox400DB:autox400DB:tunecal";
	static String unique       = ((Long)Thread.currentThread().getId()).toString();
	static String cacheRootDir = "/tmp/probe_cache_" + unique;
	static String mountDir     = "/tmp/probe_mount_" + Thread.currentThread().getId();
	static String tmpDir       = "/tmp/probe_tmp_" + Thread.currentThread().getId();
	static String dbDir        = mountDir;
	static Integer testNum     = 0; // hold-over from pre-JUnit testing
	static Vector<String> tmpDirs = new Vector<String>(); 
	
	// initialize the test data to be read and written
	final static String tstData0 = "hello = secret" + '\n';
	final static String tstData1 = "hello = world" + '\n';
	final static String tstData2 = "hello = deep" + '\n';
	
	// cleanup before and after tests
	private static void cleanup(String dirName) {
		File dir = new File(dirName);
		// recursively remove temporary directories
		if (dirName.startsWith("/tmp")) {
			ProbeIdDb.rmdir(dir);
			assert(!dir.exists());
		}
	}

	@Before
	public void setUp() throws Exception {
		// set up system properties
		System.setProperty(ProbeId.KEY_PROBEID_CACHE, cacheRootDir);
		System.setProperty(ProbeId.KEY_PROBEID_MOUNTPOINT, mountDir);
		System.setProperty(ProbeId.KEY_PROBEID_DBDIR, dbDir);
		
		// populate the mount point
		cleanup(cacheRootDir); cleanup(mountDir); cleanup(tmpDir);
		new File(mountDir).mkdir();
		new File(tmpDir).mkdir();
	}

	@After
	public void tearDown() throws Exception {
		cleanup(cacheRootDir); cleanup(mountDir); cleanup(tmpDir);
	}

	/**
	 * Helper class for unit testing cache operations
	 * @author dirk
	 *
	 */
	class CacheTest {
		final String mountDir  = System.getProperty(ProbeId.KEY_PROBEID_MOUNTPOINT);
		final String cacheRoot = System.getProperty(ProbeId.KEY_PROBEID_CACHE);
		final String probeId;
		final String dirSubdir = "subdir";     // subdirectory name of sub-directory test fixture
		final String fnSubdir = "subdir_file"; // file name of sub-directory test fixture
		String keyRdWr   = ProbeIdDb.getKeyFromHostUserFile("read_write");
		String keyDontRd = ProbeIdDb.getKeyFromHostUserFile("dont_read");		
		String keySubdir = ProbeIdDb.getKeyFromHostUserFile(dirSubdir,fnSubdir);

		// initialize the cache
		final ProbeIdCache cache;

		// set up some handles
		final String cacheDir;
		final File shelfDir;
		final File cacheRdWr, probeRdWr, shelfRdWr;
		final File cacheDontRd, probeDontRd, shelfDontRd;
		final File cacheSubdir, probeSubdir, shelfSubdir;
		
		// pretend to physically connect the probe
		public CacheTest connect() {
			File mnt = new File(mountDir);
			int population = mnt.exists() ? mnt.listFiles().length : 0;
			assertFalse(population > 0);
			if (mnt.exists()) mnt.delete();
			boolean renamed = shelfDir.renameTo(mnt);
			assertTrue(renamed);
			assertFalse(shelfDir.exists());
			return this;
		}
		
		// pretend to physically disconnect the probe
		public CacheTest disconnect() {
			File mnt = new File(mountDir);
			assertFalse(shelfDir.exists());
			boolean renamed = mnt.renameTo(shelfDir);
			assertTrue(renamed);
			assertFalse(mnt.exists());
			return this;
		}

		// write some data to the cache
		public void write(String key) throws IOException {
			OutputStream dbWriter = null; // ProbeIdDb.getBufferedWriter(keyRdWr);
			OutputStream cacheWriter = cache.getBufferedWriter(key, dbWriter);
			cacheWriter.write(("hello = world" + '\n').getBytes());
			cacheWriter.close();
		}

		// read a table from the cache
		public String read(String key) throws IOException { 
			InputStream dbReader = ProbeIdDb.getBufferedReader(key, null);
			InputStream cacheReader = cache.getBufferedReader(key, null, dbReader);
			BufferedReader reader = new BufferedReader(new InputStreamReader(cacheReader));
			return reader.readLine();
		}
		
		// read probe id directly from probe, bypassing cache
		public String readProbeId() throws IOException, ProbeIdMismatchException {
			return ProbeIdDb.readProbeId();
		}
		
		public CacheTest populate() throws IOException {
			connect();
			OutputStream ostrReadWrite  = ProbeIdDb.getBufferedWriter(keyRdWr);
			OutputStream ostrDontRead   = ProbeIdDb.getBufferedWriter(keyDontRd);
			OutputStream ostrSubdirFile = ProbeIdDb.getBufferedWriter(keySubdir);
			ostrReadWrite.write((tstData1).getBytes());
			ostrDontRead.write((tstData0).getBytes());
			ostrSubdirFile.write((tstData2).getBytes());
			ostrReadWrite.close();
			ostrDontRead.close();
			ostrSubdirFile.close();
			disconnect();
			return this;
		}
		
		// constructor uses default probe ID
		CacheTest() throws IOException, ProbeIdMismatchException { 
			this("TEST-ProbeIdCacheUnitTest"); 
		}
		
		// constructor allows caller to specify probe ID
		CacheTest(String probeId) throws IOException, ProbeIdMismatchException { 
			this.probeId = probeId;
			// make a "disconnected" version of the probe filesystem in a temporary location
			shelfDir    = new File(tmpDir + File.separator + probeId);
			ProbeIdDb.init(mountDir, probeId, path);
			ProbeIdDb db = new ProbeIdDb(mountDir);
			// initialize cache
			cache       = new ProbeIdCache(cacheRoot, db, probeId);
			cacheDir    = cache.getPath();
		
			// initialize handles to various files
			cacheRdWr   = ProbeIdDb.getFile(cacheDir, null, keyRdWr);		
			probeRdWr   = ProbeIdDb.getFile(mountDir, null, keyRdWr);
			shelfRdWr   = ProbeIdDb.getFile(shelfDir.getPath(), null, keyRdWr);
			cacheDontRd = ProbeIdDb.getFile(cacheDir, null, keyDontRd);		
			probeDontRd = ProbeIdDb.getFile(mountDir, null, keyDontRd);
			shelfDontRd = ProbeIdDb.getFile(shelfDir.getPath(), null, keyDontRd);
			cacheSubdir = ProbeIdDb.getFile(cacheDir, null, keySubdir);		
			probeSubdir = ProbeIdDb.getFile(mountDir, null, keySubdir);
			shelfSubdir = ProbeIdDb.getFile(shelfDir.getPath(), null, keySubdir);

			// put the probe on the shelf for the moment
			disconnect();
		}
	}
	
	@Test
	/**
	 * Verify that a cache behaves correctly when its underlying
	 * probe has been removed and replaced by another probe.
	 */
	public void testProbeIdCacheReplacedProbe() throws IOException, ProbeIdMismatchException {
		testNum++;
		// prepare cache for testing - we might have 2 caches, but
		// no harm done
		CacheTest probe1 = new CacheTest("TEST-PROBE-1").populate();
		CacheTest probe2 = new CacheTest("TEST-PROBE-2").populate();
		assertTrue(probe1.shelfRdWr.exists());     // sanity check
		assertFalse(probe1.probeRdWr.exists());    // sanity check
		probe1.connect();
		assertFalse(probe1.shelfRdWr.exists());    // sanity check
		assertTrue(probe1.probeRdWr.exists());     // sanity check
		String id0 = probe1.readProbeId();
		assertTrue(probe1.probeId.equals(id0));    // saved probe ID should match actual
		
		// Read something from the probe into cache
		String data1 = probe1.read(probe1.keyRdWr);
		assertTrue(data1.equals(tstData1.trim())); // sanity check
		
		// Write something to the cache
		probe1.write(probe1.keyDontRd);
		
		// Delete a table from the probe database
		probe1.cache.delete(probe1.keyDontRd);
		
		// "Physically disconnect" the probe before flush
		assertTrue(probe1.probeRdWr.exists());  // sanity check - should still exist on the probe
		probe1.disconnect();
		assertFalse(probe1.probeRdWr.exists()); // sanity check - probe disconnected
	
		// shouldn't be able to get a probe ID now
		boolean caught0 = false;
		try {
			@SuppressWarnings("unused")
			String id1 = probe1.readProbeId();			
		} catch (ProbeIdMismatchException e) {
			caught0 = true;
		}
		assertTrue(caught0);
		
		// Flush the cache
		boolean caught1 = false;
		try {
			probe1.cache.flush(probe1.probeId);
		} catch (ProbeIdMismatchException e) {
			caught1 = true;
		}
		assertTrue(caught1);
		
		// Verify that the data is still in the cache
		// "Physically connect" a new probe
		probe2.connect();
		boolean caught2 = false;
		try {
			probe1.cache.flush(probe1.probeId);
		} catch (ProbeIdMismatchException e) {
			caught2 = true;
		}
		assertFalse(caught2);
		assertTrue(probe1.shelfDontRd.exists());  // nothing was actually flushed
		
		// reconnect probe1 and flush the cache
		probe2.disconnect();
		probe1.connect();
		boolean caught3 = false;
		try {
			probe1.cache.flush(probe1.probeId);
		} catch (ProbeIdMismatchException e) {
			caught3 = true;
		}
		assertFalse(caught3);
		assertFalse(probe1.probeDontRd.exists()); // sanity check - probe disconnected
	}
	
	@Test
	public void testProbeIdCache() throws Exception {
		CacheTest tst = new CacheTest().populate().connect();
		// sanity checks
		File probeRdWr  = ProbeIdDb.getFile(tst.mountDir, null, tst.keyRdWr);
		String cacheDir = tst.cache.getPath();
		File cacheRdWr  = ProbeIdDb.getFile(tst.cacheDir, null, tst.keyRdWr);
		assertTrue(probeRdWr.exists() && probeRdWr.canRead() && probeRdWr.canWrite());
		assertFalse(cacheRdWr.exists());

		// access a probe file
		testNum++;
		InputStream istrRdWr = ProbeIdDb.getBufferedReader(tst.keyRdWr, null);
		InputStream istrCacheRdWr = tst.cache.getBufferedReader(tst.keyRdWr, null, istrRdWr);

		// verify that it is now in the cache
		assertTrue(istrCacheRdWr != null);
		assertTrue(cacheRdWr.exists());
		System.out.println("test " + testNum + " PASSED: file copied to cache");

		// verify that no other files are in the cache
		testNum++;
		File cacheDontRd = ProbeIdDb.getFile(cacheDir, null, tst.keyDontRd);
		File probeDontRd = ProbeIdDb.getFile(mountDir, null, tst.keyDontRd);
		assertFalse(cacheDontRd.exists());
		assertTrue(probeDontRd.exists());
 		System.out.println("test " + testNum + " PASSED: only accessed file cached");

		// write (modify) the cached (on read) file
		testNum++;
		OutputStream ostrProbeRdWr = ProbeIdDb.getBufferedWriter(tst.keyRdWr);
		OutputStream ostrRdWr = tst.cache.getBufferedWriter(tst.keyRdWr, ostrProbeRdWr);
		long mod0 = probeRdWr.lastModified();
		Thread.sleep(1000); // create measurable file modification time difference
		ostrProbeRdWr.write((int) '\n');   // overwrite probe file 
		ostrProbeRdWr.flush();
		ostrProbeRdWr.close();
		long mod1 = probeRdWr.lastModified();
		assertTrue(mod1 > mod0);
		System.out.println("test " + testNum + " PASSED: modification time of cache is older than store");

		testNum++;
		Thread.sleep(1000); // create measurable file modification time difference
		byte[] msg = ("bigger = better" + '\n').getBytes();
		ostrRdWr.write(msg, (int) probeRdWr.length() - 1, msg.length);
		ostrRdWr.close();

		// verify that the cached version was modified 
		assertTrue(probeRdWr.lastModified() < cacheRdWr.lastModified());
		System.out.println("test " + testNum + " PASSED: only the cached file was modified");

		// verify file that was neither read nor modified isn't in cache
		assertFalse(cacheDontRd.exists());

		// write a new file
		testNum++;
		OutputStream ostrProbeDontRd = ProbeIdDb.getBufferedWriter(tst.keyDontRd);
		OutputStream ostrDontRd = tst.cache.getBufferedWriter(tst.keyDontRd, ostrProbeDontRd);
		Thread.sleep(1000);
		ostrDontRd.write(msg);
		ostrDontRd.close();

		// verify that the new file is in the cache
		assertTrue(cacheDontRd.exists());

		// verify that the new file isn't written to the probe
		long modCacheDontRd1 = cacheDontRd.lastModified();
		long modProbeDontRd1 = probeDontRd.lastModified();
		assertTrue(modCacheDontRd1 > modProbeDontRd1);
		System.out.println("test " + testNum + " PASSED: new file only written to cache");

		// flush the cache
		testNum++;
		Thread.sleep(1000);
		tst.cache.flush(tst.probeId);

		// verify that the probe file is newer than the cache file
		long modCacheDontRd = cacheDontRd.lastModified();
		long modProbeDontRd = probeDontRd.lastModified();
		assertTrue(modProbeDontRd > modCacheDontRd);

		long modCacheRdWr = cacheRdWr.lastModified();
		long modProbeRdWr = probeRdWr.lastModified();
		assertTrue(modProbeRdWr >  modCacheRdWr);
		System.out.println("test " + testNum + " PASSED: flushed files written to backing store");
	}
	
	@Test
	public void testProbeIdCacheDelete() throws IOException, ProbeIdMismatchException {
		testNum++;
		// prepare cache for testing
		CacheTest tst = new CacheTest().populate().connect();

		// load the cache
		assertFalse(tst.cacheRdWr.exists());
		tst.read(tst.keyRdWr);
		assertTrue(tst.cacheRdWr.exists());
		
		// delete a probe configuration
		assertTrue(tst.cacheRdWr.exists());
		tst.cache.delete(tst.keyRdWr);
		
		// verify that it isn't in the cache
		File cached  = ProbeIdDb.getFile(tst.cacheDir, null, tst.keyRdWr);
		assertTrue(!cached.exists());

		// verify that it is on the probe
		File mounted = ProbeIdDb.getFile(tst.mountDir, null, tst.keyRdWr);
		assertTrue(mounted.exists());
		System.out.println("test " + testNum + " PASSED: file removed from cache not probe");

		// verify that it isn't read from the probe and re-cached
		testNum++;
		InputStream istr = ProbeIdDb.getBufferedReader(tst.keyRdWr, null);
		InputStream istrCache = tst.cache.getBufferedReader(tst.keyRdWr, null, istr);
		assertTrue(istr != null); // sanity test
		assertTrue(istrCache == null);
		System.out.println("test " + testNum + " PASSED: deleted cached file isn't accessed");
		
		// verify that it isn't in the probe after flushing the cache
		testNum++;
		tst.cache.flush(tst.probeId);
		assertTrue(!mounted.exists());
		System.out.println("test " + testNum + " PASSED: file removed from probe after cache flush");

		// verify that it is accessible once added back (i.e. that any .trash entries have been removed)
		testNum++;
		InputStream istr1 = ProbeIdDb.getBufferedReader(tst.keyRdWr, null);
		InputStream istrCache1 = tst.cache.getBufferedReader(tst.keyRdWr, null, istr1);
		assertTrue(istr1 == null);
		assertTrue(istrCache1 == null);
		tst.write(tst.keyRdWr); // create a file in the cache
		InputStream istr2 = ProbeIdDb.getBufferedReader(tst.keyRdWr, null);
		InputStream istrCache2 = tst.cache.getBufferedReader(tst.keyRdWr, null, istr2);
		assertTrue(istr2 == null);
		assertTrue(istrCache2 != null);
		istrCache2.close();
		System.out.println("test " + testNum + " PASSED: deleted file can be added back after flush");

		// verify that it is accessible once added back, but before flush
		testNum++;
		tst.cache.delete(tst.keyRdWr);
		tst.write(tst.keyRdWr);
		InputStream istr3 = ProbeIdDb.getBufferedReader(tst.keyRdWr, null);
		InputStream istrCache3 = tst.cache.getBufferedReader(tst.keyRdWr, null, istr3);
		assertTrue(istr3 == null);
		assertTrue(istrCache3 != null);
		istrCache3.close();
		System.out.println("test " + testNum + " PASSED: deleted file can be added back before flush");

		// verify that file added back after delete remains in cache and on the probe
		testNum++;
		tst.cache.flush(tst.probeId);
		InputStream istr4 = ProbeIdDb.getBufferedReader(tst.keyRdWr, null);
		InputStream istrCache4 = tst.cache.getBufferedReader(tst.keyRdWr, null, istr4);
		assertTrue(istr4 != null);
		assertTrue(istrCache4 != null);
		istr4.close();
		istrCache4.close();
		System.out.println("test " + testNum + " PASSED: deleted file added back before flush is available on cache and probe after flush");
	}
	
	@Test
	public void testProbeIdUnCachedDelete() 
		throws IOException, ProbeIdMismatchException 
	{
		// delete a table that was never cached
		testNum++;

		// populate a probe and connect it to the system
		CacheTest tst = new CacheTest().populate().connect();

		// verify that the target isn't in the cache
		assertFalse(tst.cacheRdWr.exists());

		// verify that the target is on the probe
		assertTrue(tst.probeRdWr.exists());
		
		// delete the table
		tst.cache.delete(tst.keyRdWr);

		// verify that it is still in the cache
		if (tst.probeRdWr.exists()); // grey box test that can be deleted
			System.out.println("test "+testNum.toString()+" INFO: deleted object not deleted from backing store");

		// verify that it is removed from the backing store after flushing
		tst.cache.flush(tst.cache.probeId);
		assertFalse(tst.cacheRdWr.exists());
		assertFalse(tst.probeRdWr.exists());
	}

	@Test
	public void testProbeIdUncachedSubdirDelete() throws IOException, ProbeIdMismatchException
	{
		// delete an non-cached file in a subdirectory that is also not in
		// the cache, i.e. probes/autox400DB/autox400DB, where neither probes
		// nor probes/autox400DB are in the cache.  Might as well make it a
		// system file while we're at it.
		testNum++;

		// populate a probe and connect it to the system
		CacheTest tst = new CacheTest().populate().connect();

		// verify that the target isn't in the cache
		assertFalse(tst.cacheSubdir.exists());

		// verify that the target is on the probe
		assertTrue(tst.probeSubdir.exists());
		
		// delete the table from the top-level Probe ID
		ProbeIdCmd cmd = 
			new ProbeIdCmd("drop").key(tst.fnSubdir).opt(tst.dirSubdir);
		ProbeIdIO probe = new ProbeIdIO(cmd);
		probe.flush();
		
		// verify that it is removed from the backing store after flushing
		assertFalse(tst.cacheSubdir.exists());
		assertFalse(tst.probeSubdir.exists());
	}
	
	@Test
	public void testProbeIdCacheAtomicity() {
		// check atomic deletion
		testNum++;
		fail("not implemented yet");
		
		// check for atomic write flush
		testNum++;
		fail("not implemented yet");
	}
}
