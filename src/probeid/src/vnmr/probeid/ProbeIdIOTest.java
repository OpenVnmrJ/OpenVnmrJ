/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/**
 * 
 */
package vnmr.probeid;

import static org.junit.Assert.*;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.util.Collection;
import java.util.Map;
import java.util.TreeMap;
import java.util.Vector;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import vnmr.vjclient.VnmrIO;
import vnmr.vjclient.VnmrProxy;

/**
 * @author dirk
 *
 */
public class ProbeIdIOTest {
	private static String m_id          = null;
	private static String m_path        = null;
	private static String m_tstProbeCfg = "autox400DB";
	private static String m_probeDir    = "probes/autox400DB";
	private static String m_tuneDir     = "tune/autox400DB";
	private static String m_tuneCalDir  = "tune/tunecal_autox400DB";
	private static String m_mntDir      = "/tmp/probeid/mnt";
	private static String m_cacheDir    = "/tmp/probeid/cache";
	private ProbeIdIO      m_server     = null;
	private BufferedReader m_in         = null;
	private PrintWriter    m_out        = null;
	private static Integer testNum = 0; // holdover from pre-JUnit testing
	private static Vector<String> m_tmpDirs = new Vector<String>(); 
	final   static String CACHE_TEST_ROOT = "/tmp/tst";

	/**
	 * @throws java.lang.Exception
	 */
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		checkSrc(m_probeDir);
		checkSrc(m_tuneDir);
		checkSrc(m_tuneCalDir);

		System.out.println(ProbeIdIOTest.class.getSimpleName()+
				": initialization complete");
	}

	/**
	 * @throws java.lang.Exception
	 */
	@AfterClass
	public static void tearDownAfterClass() throws Exception {
		System.out.println(ProbeIdIOTest.class.getSimpleName()+
				": cleanup complete");
	}

	/**
	 * @throws java.lang.Exception
	 */
	@Before
	public void setUp() throws Exception {
		// define an initial set of probe directories and subdirectories
		setUp("autox400DB");
		// set up some useful default system properties that can be overridden by the test
		System.setProperty(ProbeId.KEY_PROBEID_MOUNTPOINT, m_mntDir);
		System.setProperty(ProbeId.KEY_PROBEID_CACHE, m_cacheDir);
		System.setProperty(ProbeId.KEY_PROBEID_DBDIR, m_mntDir);
		System.setProperty(ProbeId.KEY_PROBEID_INFO2VJ, "false");
		// initialize the probe
		ProbeIdDb.init(m_mntDir, m_id, m_path);
	}
	
	public void setUp(String cfg) {
		ProbeIdIO.setProbeIdSystemDefaults();
		m_id 		  = "TEST-PROBEID-01234-56789";
		m_tstProbeCfg = cfg;
		m_probeDir    = "probes/" + m_tstProbeCfg;
		m_tuneCalDir  = "tune/tunecal_" + m_tstProbeCfg;
		m_tuneDir     = "tune/" + m_tstProbeCfg;
		// path from "probeid('probedir_opts')" VnmrJ Magical script
		m_path =  m_probeDir;
		m_path += ":" + m_tuneCalDir;
		m_path += ":" + m_tuneDir;
	}

	/**
	 * Verify that the specified source directories exist and are
	 * non-empty.
	 * @param srcdir
	 */
	public static void checkSrc(String srcdir) {
		File dir = getLegacyDir(srcdir);
		if (dir!= null && dir.isDirectory() && dir.list().length > 0)
			return;
		System.out.println("ERROR: can't find test probe source directory "
				+srcdir+" or it is empty");
	}
	
	/**
	 * @throws java.lang.Exception
	 */
	@After
	public static void tearDown() throws Exception {
		for (String dir : m_tmpDirs)
			ProbeIdDb.rmdir(new File(dir));
		m_tmpDirs.clear();
		ProbeIdDb.rmdir(new File(m_mntDir));
		ProbeIdDb.rmdir(new File(m_cacheDir));
		ProbeIdIO.unlock();
	}

	private static String readForValidation(BufferedReader reader)
		throws IOException
	{
		char[] buf = new char[1024];
		int bytes_read = reader.read(buf);
		assert bytes_read > 0;
		return new String(buf).trim();
	}

	/**
	 * A utility function for unit testing that
	 * reads up to the 1st 1K characters from a file.
	 * @param reader
	 * @return up to the 1st 1K characters from a file
	 * @throws IOException
	 */
	private static boolean 
		testWrite(String testName, ProbeIdIO probe, boolean sys) 
			throws IOException 
	{
		String contents_written = "testing 1 2 3";
		String filename = "encrypted_test_" + testName + "@";
		String key = ProbeIdDb.getKeyFromHostUserFile(filename);
		String cipher = System.getProperty(ProbeId.KEY_PROBEID_CIPHER);

		probe.getDb().delete(key); // clear residue from previous tests
		BufferedWriter writer = probe.getBufferedWriter(filename);
		writer.write(contents_written);
		writer.close();
		ProbeIdStore db       = probe.getDbHandle(null);
		BufferedReader reader = probe.getBufferedReaderFromKey(db, key);
		InputStreamReader in  = new InputStreamReader(db.getInputStreamFromKey(key));
		BufferedReader raw    = new BufferedReader(in);
		String contents_read  = readForValidation(reader);
		String contents_raw   = readForValidation(raw);
		System.out.println("test " + testName + 
				" PASSED: raw=(" +
				contents_raw + ") vs out=(" +
				contents_read + ") vs in=(" +
				contents_written + ")");
		// verify contents read match contents written
		assertTrue(contents_read.equals(contents_written));
		// verify that encrypted not same as unencrypted
		assertTrue(cipher.equals(ProbeId.NULL_CIPHER) || !contents_read.equals(contents_raw)); 
		return true;
	}

	@Test
	public void testAppdirs()
	{    
       testNum++;
       // set up and initialize two app directories in addition
       // to user and system directory.
       
       // create identically named probes in both with different params
       assert(false); //tagfile is as expected);
       
       // search for params and verify right one was selected
       
       // reverse search order
       
       // search for params again - should get different search order

       assertTrue(false);  // not done
	}

	@Test
	public void ProbeIdIOTestSetParams() throws Exception {
		testNum++;
		String description = "Probe ID setparams";
		String key         = "Probetst";
		String tstValue    = "9999";
		String tstInitVal  = "0000";
		String tstDate     = "Probedate=12-12-1234";
		String tstCmdArgs  = key+"="+tstValue+","+tstDate;
		String test_setparams[] = {
			"-setparams", tstCmdArgs,
			"-cfg", m_tstProbeCfg, "-opt", m_probeDir,
			"-cache", m_cacheDir, "-mnt", m_mntDir,
			"-nocrypto", "-v"
		};
		ProbeIdIO probe = new ProbeIdIO();
		ByteArrayOutputStream out = new ByteArrayOutputStream(128);
		probe.setOutputStream(out);   // redirect stdout to a ByteArrayStream
		probe.attach();
		
		// setup
		ProbeIdStore db = probe.getDbHandle(null);
		probe.addparam(db, key, tstInitVal, m_tstProbeCfg, m_probeDir);
		String p = probe.getparam(db, key, m_tstProbeCfg, m_probeDir);
		assertTrue(p != null && p.equals(tstInitVal)); // sanity check
		probe.process("ProbeIdIO", ProbeIdIO.parse(test_setparams));
		String result = out.toString().trim();
		assertTrue(matchParam(key, result));
		System.out.println("test " + testNum + " PASSED: "+ description);
	}

	@Test
	public void TestProbeIdIOImportAllDontEncrypt() throws Exception {
		testNum++;
		String description = "written data read from cache w/o probe";
		String test_import_everything[] = {
				"-import", "*",
				"-opt", m_probeDir, "-opt", m_tuneDir, "-opt", m_tuneCalDir,
				"-cache", m_cacheDir, "-mnt", m_mntDir,
				"-nocrypto", "-v"
		};
		long testTime   = now();
		ProbeIdIO probe = new ProbeIdIO();
		probe.attach().process("ProbeIdIO", ProbeIdIO.parse(test_import_everything));
		// list of subdirectories implicitly included in "*" parameter
		String[] subdirs = {m_probeDir, m_tuneDir, m_tuneCalDir};
		// compare subdirectories and their cached counterparts
		for (String subdir : subdirs) {
			File parent = getLegacyDir(subdir);
			assertTrue(parent != null);
			for (String child : parent.list() ) {
				String key = ProbeIdDb.getKeyFromHostUserFile(subdir, child);
				File copy = probe.getPath(key);       // database copy
				File file = new File(parent, child);  // original copy
				assertTrue(copy.exists());
				long fileLength = file.length();
				long copyLength = copy.length();
				assertTrue(fileLength == copyLength); // different if encrypted
				assertTrue(file.lastModified() < copy.lastModified()); // original file should be older
				assertTrue(copy.lastModified() > testTime); // make sure it really was copied
			}
		}		
		System.out.println("test " + testNum + " PASSED: "+ description);
	}
	
	/**
	 * Test ProbeIdIO {@link probeid.ProbeIdIO#start()}.
	 */
	@Test
	public void testProbeIdIOServer()
	{
		try {
			testNum++;
			ProbeIdClient client = new ProbeIdClient();
			// kick of the server and issue the command
			String msg    = "hello";
			String[] args = {"-echo", msg};
			ProbeIdIO server = startServer(args); 

			// read back the results
			String echo = client.read();
			assertTrue(echo.equals(msg));

			// shut down the server
			client.write("-quit");         // send server shutdown message
			String quit = client.read();   // read server shutdown completion message
			try {
				server.waitForCompletion(1000);// wait some ms for the server to exit
			} catch (InterruptedException e) {
				fail("server failed to signal completion");
			}	
			assertTrue(quit.equals(ProbeId.MSG_SERVER_EXITING));
	 		Thread.State serverState = server.getThread().getState();
			assertTrue(serverState.equals(Thread.State.TERMINATED));
		} catch (Exception e) {
			e.printStackTrace();
			System.out.println("test " + testNum.toString() + " FAILED");			
		}
	}

	/**
	 * Test method for {@link probeid.ProbeIdIO#getProbeId()}.
	 *   unencrypted probe id locally mounted
	 * steps:   a) write/read probe ID file and compare to known value
	 *          b) write/read data file
	 * results: valid probe id read
	 */
	@Test
	public void testProbeIdIOUnencrypted() {
		System.setProperty(ProbeId.KEY_PROBEID_ETHERPORT, "eth0");
		System.setProperty(ProbeId.KEY_PROBEID_ENCRYPT, "false");
		System.setProperty(ProbeId.KEY_PROBEID_USEPROBEID, "true");
		System.setProperty(ProbeId.KEY_PROBEID_PASSWORD, "invalid"); // shouldn't be used
		System.setProperty(ProbeId.KEY_PROBEID_CACHE, "");           // effectively disable caching
		ProbeIdIO probe = new ProbeIdIO();
		String probeId = "";
		try {
			Collection<String> dirs = null;
			testNum++;
			dirs = ProbeIdDb.init(System.getProperty(ProbeId.KEY_PROBEID_DBDIR), m_id, m_path);
			m_tmpDirs.addAll(dirs);
			probeId = probe.getProbeId();
			probe = null;                      // release ProbeIdIO object
			assertTrue(probeId.equals(m_id));
			//System.out.println("test " + testNum.toString() + " PASSED: probe id = " + probeId.toString());
		} catch (Exception e1) {
			fail("Couldn't get Probe ID");
			e1.printStackTrace();
			//System.out.println("test " + testNum.toString() + " FAILED");
		}
	}

	@Test
	public void testProbeIdIOUncached() {
		/* tests without caching
		 * test unencrypted w/o cache: probe ID with probe files not encrypted
		 * test encrypted w/o cache: probe ID with probe files encrypted
		 * preconditions:
		 */
		System.setProperty(ProbeId.KEY_PROBEID_ENCRYPT, "true");
		System.setProperty(ProbeId.KEY_PROBEID_USEPROBEID, "true");
		System.setProperty(ProbeId.KEY_PROBEID_PASSWORD, "Varian1");
		System.setProperty(ProbeId.KEY_PROBEID_CACHE, "");           // effectively disable caching
		boolean forceSysFlag = false;
		String[] test_ciphers = {ProbeId.NULL_CIPHER, ProbeId.DEFAULT_CIPHER};
		for (String test_cipher : test_ciphers) try {
			testNum++;
			ProbeIdIO probe = new ProbeIdIO();
			System.setProperty(ProbeId.KEY_PROBEID_CIPHER, test_cipher);
			testWrite(testNum.toString(), probe, forceSysFlag);
			//forceSysFlag = !forceSysFlag; // alternate between system and user files
		} catch (IOException e1) {
			e1.printStackTrace();
		}
	}

	@Test
	public void testProbeIdIOCachedUnencrypted() {
		/* test cached unencrypted: probe ID with cache */
		System.setProperty(ProbeId.KEY_PROBEID_CACHE, CACHE_TEST_ROOT+"/probe_cache");
		System.setProperty(ProbeId.KEY_PROBEID_MOUNTPOINT, CACHE_TEST_ROOT+"/probe_mnt");
		System.setProperty(ProbeId.KEY_PROBEID_DBDIR, System.getProperty(ProbeId.KEY_PROBEID_MOUNTPOINT));
		System.setProperty(ProbeId.KEY_PROBEID_CIPHER, ProbeId.NULL_CIPHER);

		// clean up after previous tests and reload probe database with sample data
		ProbeIdDb.rmdir(new File(CACHE_TEST_ROOT));
		ProbeIdIO probe = new ProbeIdIO();
		probe.getDb().init(m_id, m_path);   // bootstrap a mounted probe file system
		try {
			probe.attach();

			// read a probe-resident file
			testNum++;
			String key0 = ProbeIdDb.getKeyFromFile("factory_rd"); // top-level key
			String entry0 = "reader = 1";  // 1st and only entry in file
			probe.getDb().append(key0, entry0 + '\n');
			ProbeIdStore db          = probe.getDbHandle(null);
			BufferedReader rdFactory = probe.getBufferedReaderFromKey(db,key0);
			String contentsRead0     = readForValidation(rdFactory);
			assertTrue(contentsRead0.equals(entry0));
			System.out.println("test " + testNum + " PASSED: factory data from cache matches");

			// write a new file
			testNum++;
			String key1 = ProbeIdDb.getKeyFromHostFile("host_rdwr_test");
			String entry1 = "writer = 1";
			BufferedWriter wrHost = probe.getBufferedWriterFromKey(db,key1);
			wrHost.write(entry1);
			wrHost.close();
			BufferedReader rdHost = probe.getBufferedReaderFromKey(db,key1);
			String contentsRead1 = readForValidation(rdHost);
			assertTrue(contentsRead1.equals(entry1));
			System.out.println("test " + testNum + " PASSED: data written to cache matches");

			// delete probe mount point, forcing the system to read from cache
			ProbeIdDb.rmdir(new File(System.getProperty(ProbeId.KEY_PROBEID_MOUNTPOINT)));

			// read back both the read-only file from cache
			testNum++;
			BufferedReader rdCache0 = probe.getBufferedReaderFromKey(db,key0);
			String contentsCached0 = readForValidation(rdCache0);
			assertTrue(contentsCached0.equals(entry0));
			System.out.println("test " + testNum + " PASSED: deleted factory data retrieved from read cache");

			// read back the written file from cache
			testNum++;
			BufferedReader rdCache1 = probe.getBufferedReaderFromKey(db,key1);
			String contentsCached1 = readForValidation(rdCache1);
			assertTrue(contentsCached1.equals(entry1));
			System.out.println("test " + testNum + " PASSED: written data read from cache w/o probe");
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	// prep a ProbeIdIO instance for testing
	private ProbeIdIO prepare() {
		ProbeIdIO probe = new ProbeIdIO();
		probe.getDb().init(m_id, m_path);   // bootstrap a mounted probe file system			
		probe.attach();
		return probe;
	}
	
	/**
	 * Check probe resident zipped files against expected values
	 * @param probe
	 * @return
	 * @throws IOException
	 */
	private int zipCheck(ProbeIdIO probe) throws IOException {
		// write a probe-resident compressed archive
		ProbeIdStore db          = probe.getDbHandle(null);
		ZipOutputStream wrZipper = probe.getZipOutput(db,"factory_test.zip","vnmrsys/tune/tunecal_autox400DB");
		ZipEntry wrEntry = null;
		String [] entry = {"zippety = 0", "zippety = 1"};

		// add 2 "files" to the archive
		wrEntry = new ZipEntry("entry0");
		wrZipper.putNextEntry(wrEntry);
		wrZipper.write(entry[0].getBytes());
		wrEntry = new ZipEntry("entry1");
		wrZipper.putNextEntry(wrEntry);
		wrZipper.write(entry[1].getBytes());
		wrZipper.close();

		// read probe-resident compressed archive
		String search = "vnmrsys/tune/tunecal_autox400DB:/vnmr/tune/tunecal_autox400DB:/vnmr/tunecal";
		ZipInputStream rdZipper = probe.getZipInput(db,"factory_test.zip", search);
		// read 2 archive "files"
		int n=0;
		while (rdZipper.getNextEntry() != null) { // advance to next ZipEntry
			BufferedReader zin = new BufferedReader(new InputStreamReader(rdZipper));
			String contentsZipped = readForValidation(zin);
			assertTrue(contentsZipped.equals(entry[n++]));
		}
		return n;
	}

	@Test
	public void TestProbeIdIOZipNoCrypto() throws IOException {
		// test zip: probe ID with compressed probe files w/o crypto
		testNum++;
		ProbeIdDb.rmdir(new File(CACHE_TEST_ROOT));
		String detail = " without cache or crypto";
		ProbeIdIO probe = prepare();
		probe.enableCache(false); 
		probe.enableCrypto(false);
		int n = zipCheck(probe);
		assertTrue(n==2);
		System.out.println("test " + testNum + " PASSED: compressed data" + detail);
	}
	
	@Test
	public void TestProbeIdIOZipCrypto() throws IOException {
		// test zip: probe ID with compressed probe files
		testNum++;
		ProbeIdDb.rmdir(new File(CACHE_TEST_ROOT));
		String detail = " with cache and crypto";
		ProbeIdIO probe = prepare();
		probe.enableCache(true);
		probe.enableCrypto(false);
		int n = zipCheck(probe);
		assertTrue(n==2);
		System.out.println("test " + testNum + " PASSED: compressed data" + detail);
	}


	@Test
	public void TestProbeIdIONoProbeId() {
		// test list: probe files w/o probe ID
		// preconditions:
		System.setProperty(ProbeId.KEY_PROBEID_ENCRYPT, "true");
		System.setProperty(ProbeId.KEY_PROBEID_USEPROBEID, "true");
		try {
			testNum++;
			ProbeIdIO probe = new ProbeIdIO();
			ProbeIdStore db = probe.getDbHandle(null);
			probe.getDb().init(m_id, m_path);   // bootstrap a mounted probe file system			
			probe.attach();
			ProbeIdCmd cmd = new ProbeIdCmd("query", "probecfg");
			Collection<String> configs = probe.query(db,cmd,true);
			assertTrue(configs.size() == 1);
			assertTrue(configs.iterator().next().equals(m_tstProbeCfg));
			System.out.println("test " + testNum + " PASSED: probe configs = " + configs);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Test
	public void TestProbeIdIOLegacy()
	{
		// test legacy mode: probe files w/o probe ID
		// preconditions:
		System.setProperty(ProbeId.KEY_PROBEID_ENCRYPT, "false");
		System.setProperty(ProbeId.KEY_PROBEID_USEPROBEID, "false");
		// expected result: no probe id returned, should be
		//   the same as before Probe ID code was added (i.e.
		//   bypass Probe ID files completely).
		try {
			testNum++;
			// TODO Add legacy tests
			fail("not yet implemented");
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Test
	public void TestProbeIdIOEncrypted() {
		// test encrypted: probe ID with probe files encrypted
		System.setProperty(ProbeId.KEY_PROBEID_ENCRYPT, "true");
		System.setProperty(ProbeId.KEY_PROBEID_USEPROBEID, "true");
		System.setProperty(ProbeId.KEY_PROBEID_PASSWORD, "Varian1");
		try {
			testNum++;
			fail("Not yet implemented");
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@Test
	public void TestProbeIdIORemote() {
		// test remote host: probe ID with remote probe file
		System.setProperty(ProbeId.KEY_PROBEID_REMOTE, "localhost");
		try {
			// TODO add remote tests
			fail("Not yet implemented");				
		} catch (Exception e) {
			e.printStackTrace();
		}		
	}

	private static File getLegacyDir(String subdir) {
		String usrSrc = System.getProperty("user.home") + File.separator + VnmrIO.DEFAULT_USRDIR;
		String usrDir = usrSrc + File.separator + subdir;
		File usrParent = new File(usrDir);
		if (usrParent.isDirectory())
			return usrParent;
		String sysSrc = VnmrProxy.getSystemDir() + File.separator;
		String sysDir = sysSrc + File.separator + subdir;
		File sysParent = new File(sysDir);
		if (sysParent.isDirectory())
			return sysParent;
		return null;
	}

	private static File getLegacyProbePath() {
		File parent = getLegacyDir(m_probeDir);
		String src = parent.getPath() + File.separator + m_tstProbeCfg;
		return new File(src);
	}
	
	// create a unique time stamp for later comparison / sanity checks
	private static long now() {
		long timestamp = Long.MAX_VALUE; // should cause an assertion failure
		try {
			File file = File.createTempFile("probeid", null);
			timestamp = file.lastModified();
	 		Thread.sleep(1000);
			file.delete();
		} catch (IOException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		return timestamp;
	}

	// set up arguments to import a file from the legacy user directory
	private String[] getImportProbeArgs(boolean sys) {
		File src = getLegacyProbePath();
		String args[] = {
				"-import", src.getPath(),
				"-opt", m_tstProbeCfg, "-opt", m_probeDir,
				"-cache", m_cacheDir, "-mnt", m_mntDir,
				"-nocrypto", "-v", (sys ? "-sys" : ""),
		};
		return args;
	}
	
	@Test
	public void TestProbeIdIOImportFileDontEncrypt() throws Exception {
		testNum++;
		String description = "written data read from cache w/o probe";
		File parent = getLegacyDir(m_probeDir);
		String test_import_file[] = getImportProbeArgs(false);
		ProbeIdIO probe = new ProbeIdIO();
		probe.attach().process("ProbeIdIO", ProbeIdIO.parse(test_import_file));
		
		String cachedir = m_cacheDir + File.separator + m_probeDir;
		String dstDir   = cachedir;
		assertTrue(parent != null);
		for (String child : parent.list() ) {
			File file = new File(child);
			File copy = new File(dstDir + File.separator + file);
			if (child.equals(dstDir + File.separator + m_tstProbeCfg)) {
				assertTrue(file.length() == copy.length());
				assertTrue(file.lastModified() < copy.lastModified());
			} else { // only a single file should have been copied
				assertFalse(copy.exists());
			}
		}
		System.out.println("test " + testNum + " PASSED: "+ description);
	}

	private boolean matchParam(String key, String result) throws IOException {
		String path = getLegacyProbePath().getPath();
		String[] awk = {"awk", "/^"+key+" / {print $2}", path };
		Process process = Runtime.getRuntime().exec(awk, null, null);
		BufferedReader stdout = new BufferedReader(new InputStreamReader(process.getInputStream()));
		BufferedReader stderr = new BufferedReader(new InputStreamReader(process.getErrorStream()));
		String match = stdout.readLine();
		String line = null;
		while ((line = stderr.readLine()) != null)
			System.out.println("[stderr] "+line);
		assertTrue(match != null);
		return result.equals(match);
	}
	
	public void populate(boolean doCache, boolean doCrypto) throws Exception {
		File parent = getLegacyDir(m_probeDir);
		String src = parent.getPath() + File.separator + m_tstProbeCfg;
		String populate_probe_db[] = {
				"-import", src,
				"-opt", m_tstProbeCfg, "-opt", m_probeDir,
				"-mnt", m_mntDir,
				"-cache", (doCache ? m_cacheDir : ""), 
				(doCrypto ? "-crypto" : "-nocrypto"),
		};
		ProbeIdIO probe = new ProbeIdIO();
		probe.attach().process("ProbeIdIO", ProbeIdIO.parse(populate_probe_db));
	}

	public void populateAll(boolean doCache, boolean doCrypto, boolean sys)
		throws Exception 
	{
		File parent = getLegacyDir(m_probeDir);
		@SuppressWarnings("unused") // useful for debugging
		String src = parent.getPath() + File.separator + m_tstProbeCfg;
		String populate_probe_db[] = {
				"-import", "*",
				"-opt", m_probeDir, "-opt", m_tuneDir, "-opt", m_tuneCalDir,
				(sys ? "-sys" : ""),
				"-cache", m_cacheDir, "-mnt", m_mntDir,
				"-nocrypto", "-v"
		};
		ProbeIdIO probe = new ProbeIdIO();
		probe.attach().process("ProbeIdIO", ProbeIdIO.parse(populate_probe_db));
	}

	private String[] TestProbeidIOGetParamSetup(String key, boolean doCache, boolean doCrypto) throws Exception {
		setUp("autox400DB");
		populate(doCache, doCrypto);
		String test_getparam[] = {
				"-v", "-getparam", key, "-opt", m_probeDir,
				"-cfg", m_tstProbeCfg, "-mnt", m_mntDir,
				"-cache", (doCache ? m_cacheDir : ""), 
				(doCrypto ? "-crypto" : "-nocrypto"),
		};
		return test_getparam;		
	}
	
	@Test
	public void TestProbeIdIOGetParam() throws Exception {
		testNum++;
		String description = "getparam unencrypted";
		String key = "H1pw90";
		String[] test_getparam = TestProbeidIOGetParamSetup(key, true, false);
		ProbeIdIO probe = new ProbeIdIO();
		ByteArrayOutputStream out = new ByteArrayOutputStream(128);
		probe.setOutputStream(out);   // redirect stdout to a ByteArrayStream
		probe.attach().process("ProbeIdIO", ProbeIdIO.parse(test_getparam)); // process the command-line args
		String result = out.toString().trim();
		assertTrue(matchParam(key, result));
		System.out.println("test " + testNum + " PASSED: "+ description);
	}

	public ProbeIdIO startServer(String[] args) throws FileNotFoundException {
		File fifoIn  = new File(ProbeId.DEFAULT_FIFO_IN);
		File fifoOut = new File(ProbeId.DEFAULT_FIFO_OUT);
		ProbeIdPipeServer.mkfifo(fifoOut.getPath(), true);  // clear and create the FIFOs
		ProbeIdPipeServer.mkfifo(fifoIn.getPath(), true);  
		m_in         = new BufferedReader(new InputStreamReader((new FileInputStream(fifoOut))));
		m_out        = new PrintWriter(fifoIn);
		m_server     = new ProbeIdIO(new ProbeIdCmd("start","noserver"));
		if (args != null) {
			String cmd = ProbeIdClient.getCommand(args);
			m_out.println(cmd);
			m_out.flush();
		}
		return m_server;
	}
	
	public String attachProbe() throws IOException {
		// formally attach a probe
		assert(m_server != null);
		assert(m_out != null);
		assert(m_in != null);
		m_out.println("-attach");
		m_out.flush();
		return m_in.readLine();
	}
	
	public void stopServer() throws IOException {
		m_out.println("-quit");
		m_out.flush();
		// sometimes it is useful to look at value returned for debug purposes
		@SuppressWarnings("unused") 
		String death = m_in.readLine();
	}
	
	class ProbeIdIOWrapper extends ProbeIdIO {
		ByteArrayOutputStream ostream = null;
		PrintWriter    out        	  = null;
		BufferedReader in             = null;
		BufferedReader getReader() {
			ByteArrayInputStream is = new ByteArrayInputStream(ostream.toByteArray());
			return new BufferedReader(new InputStreamReader(is));
		}
		@Override
		public boolean process(String name, ProbeIdCmd cmd) 
			throws ProbeIdMismatchException, IllegalArgumentException, IOException 
		{
			ostream      = new ByteArrayOutputStream();
			out          = new PrintWriter(ostream);
			setOutputStream(ostream);
			boolean quit = super.process(name, cmd);
			in           = getReader();
			return quit;
		}
		ProbeIdIOWrapper() {
			super();
			attach();
		}
		public String readLine() throws IOException { 
			return in == null ? null : in.readLine(); 
		}
	}
	
	private ProbeIdIOWrapper newProbe(int outputBufSize) {
		ProbeIdIOWrapper probe = new ProbeIdIOWrapper();
		m_out = probe.out;
		return probe;
	}
	
	@Test
	/**
	 * Verify that getparam works in server mode using a relatively
	 * raw interface to the pipe.
	 */
	public void TestProbeIdIOGetParamFromServer() throws Exception {
		testNum++;
		String description = "getparam unencrypted from server";
		String key         = "H1pw90";
		String[] args      = TestProbeidIOGetParamSetup(key, false, false);
		startServer(args);
		String result = m_in.readLine();
		assertTrue(matchParam(key, result));
 		stopServer();
 		//m_server.wait(1000);
 		Thread.sleep(1000);
 		Thread.State serverState = m_server.getThread().getState();
		assertTrue(serverState.equals(Thread.State.TERMINATED));
		System.out.println("test " + testNum + " PASSED: "+ description);
	}

	/**
	 * Verify server lock files work as expected.
	 */
	@Test
	public void TestProbeIdIOServerLock() throws IOException {
		testNum++;
		String description = "Probe ID server lock";
		assertTrue(!ProbeIdIO.locked());
		ProbeIdIO server   = new ProbeIdIO(new ProbeIdCmd("start", "noserver"));
		File lock = ProbeIdServer.getLockFile();
		assertTrue(lock.exists());
		assertTrue(ProbeIdServer.locked());
		assertTrue(server.getThread().getState().compareTo(Thread.State.RUNNABLE) == 0);
		ProbeIdIO second   = new ProbeIdIO(new ProbeIdCmd("start", "noserver"));
		assertTrue(second.getThread() == null);
		ProbeIdIO.quit();
		System.out.println("test " + testNum + " PASSED: "+ description);		
	}
	
	private String[] TestProbeidIOExportSetup(boolean doCache, boolean doCrypto)
		throws Exception 
	{
		setUp();
		populate(doCache, doCrypto);
		String test_export_file[] = {
				"-export", m_tstProbeCfg,
				"-opt", m_probeDir,
				"-cfg", m_tstProbeCfg,
				"-cache", (doCache ? m_cacheDir : ""),
				"-mnt", m_mntDir,
				(doCrypto ? "-crypto" : "-nocrypto"),
		};
		return test_export_file;		
	}
	
	@Test
	public void TestProbeIdIOExportFromServer() throws Exception {
		testNum++;
		String description = "Probe ID server export";
		// first import the file into probe the probe then export it again
		String[] args = TestProbeidIOExportSetup(true, true);
		String path   = getLegacyDir(m_probeDir) + File.separator + m_tstProbeCfg;
		File file     = new File(path);
		BufferedReader reader = new BufferedReader(new FileReader(file));

		startServer(args);
		String dst = null;
		String src = null;
		assertTrue(file.length() > 0);
		int lines = 0;
		try {
			m_server.waitForCompletion(1000);
		} catch (InterruptedException e) {
			System.out.println("server failed to signal completion in a timely manner: "+e.getMessage());
			fail("server failed to signal completion in a timely manner: "+e.getMessage());
		}
		while (((dst = m_in.readLine()) != null) && ((src = reader.readLine()) != null)) {
			if (!src.equals(dst)) // make it easy to set a brk pt
				assertTrue(src.equals(dst));
			lines++;
		}
		assertTrue(lines > 0);
		stopServer();
		System.out.println("test " + testNum + " PASSED: "+ description + " "+Integer.toString(lines)+" lines match");
	}
	
	@Test
	public void TestProbeIdIOExport() throws Exception {
		testNum++;
		String description = "Probe ID export";
		String[] args   = TestProbeidIOExportSetup(false, false);
		String path     = getLegacyDir(m_probeDir) + File.separator + m_tstProbeCfg;
		File file       = new File(path);
		BufferedReader reader = new BufferedReader(new FileReader(file));
		ProbeIdIO probe = new ProbeIdIO();
		ByteArrayOutputStream out = new ByteArrayOutputStream((int)file.length());
		probe.setOutputStream(out);   // redirect stdout to a ByteArrayStream
		probe.attach().process("ProbeIdIO", ProbeIdIO.parse(args));
		ByteArrayInputStream is = new ByteArrayInputStream(out.toByteArray());
		BufferedReader in = new BufferedReader(new InputStreamReader(is));
		String dst = null;
		String src = null;
		assertTrue(file.length() > 0);
		Integer lines = 0;
		while (((dst = in.readLine()) != null) && ((src = reader.readLine()) != null)) {
			assertTrue(src.equals(dst));
			lines++;
		}
		assertTrue(lines > 0); // make sure some actual comparisons took place
		System.out.println("test " + testNum + " PASSED: "+ description + " "+lines.toString()+" lines match");		
	}
	
	@Test
	public void TestProbeIdIOCopy() throws Exception {
		testNum++;
		String description = "Probe ID copy";
		setUp();
		String im[] = getImportProbeArgs(false);
		String rm[] = {"-drop",m_tstProbeCfg,"-opt",m_probeDir,"-sys"};
		String ex[] = {"-exists",m_tstProbeCfg,"-opt",m_probeDir,"-sys"};
		String test_copy[] = { // promote user probe cfg to system probe cfg
			"-copy", m_tstProbeCfg, // copy test probe configuration
			"-opt", m_probeDir,     // from probe subdirectory
			"-v"                    // generate verbose output
		};
		
		ProbeIdIOWrapper probe = newProbe(1);
		probe.process("ProbeIdIO", ProbeIdIO.parse(im)); // import probe file to user area
		probe.process("ProbeIdIO", ProbeIdIO.parse(rm)); // delete probe config from system area
		probe.process("ProbeIdIO", ProbeIdIO.parse(ex)); // check existance of probe config in system area
		String nonexistant = probe.readLine();           // shouldn't be there yet
		assertTrue(nonexistant != null && nonexistant.equals("0"));
		
		probe.process("ProbeIdIO", ProbeIdIO.parse(test_copy));
		String copied = probe.readLine();
		assertTrue(copied != null && copied.equals("1"));// copy should have succeeded

		probe.process("ProbeIdIO", ProbeIdIO.parse(ex)); // probe file should exist in system area now
		String exists = probe.readLine();
		assertTrue(exists != null && exists.equals("1"));
		System.out.println("test " + testNum + " PASSED: "+ description);
	}

	private String[] TestProbeidIOExistsSetup(String primary, String secondary, boolean sys) {
		String test_export_file[] = {
				"-exists", (primary == null ? m_tstProbeCfg : primary),
				"-opt", (secondary == null ? m_probeDir : secondary),
				"-cfg", m_tstProbeCfg,
				sys ? "-sys" : "", 
				"-v"
		};
		return test_export_file;		
	}
	
	private File getUserProbePath(ProbeIdIO cfg) {
		String key = ProbeIdDb.getKeyFromHostUserFile(m_probeDir, m_tstProbeCfg);
		return cfg.getPath(key);
	}
	
	private File getSysProbePath(ProbeIdIO cfg) {
		String sysdir = File.separator + m_probeDir;
		String key = ProbeIdDb.getKeyFromHostSubdir(sysdir, m_tstProbeCfg);
		return cfg.getPath(key);
	}
	
	@SuppressWarnings("unused")
	private File getSysFilePath(ProbeIdIO cfg, String partialPath) {
		if (cfg == null) cfg = new ProbeIdIO();
		String sysdir = File.separator + partialPath;
		String key = ProbeIdDb.getKeyFromHostSubdir(sysdir, m_tstProbeCfg);
		return cfg.getPath(key);
	}
	
	@Test
	public void TestProbeIdIOExists() throws Exception {
		testNum++;
		String description = "Probe ID table exists";
		setUp();

		// create an initial a probe configuration 
		ProbeIdIOWrapper probe = newProbe(1);		
		probe.process("ProbeIdIO", ProbeIdIO.parse(getImportProbeArgs(false)));

		// check for an existing user directory probe
		String[] args = TestProbeidIOExistsSetup(null, null, false);
		probe.process("ProbeIdIO", ProbeIdIO.parse(args));
		String exists = probe.readLine();
		assertTrue(exists.equals("1"));

		// check for a nonexistent user probe
		String bogusPrimary = m_tstProbeCfg+"Garbage";
		args = TestProbeidIOExistsSetup(bogusPrimary, null, false);
		probe.process("ProbeIdIO", ProbeIdIO.parse(args));
		exists = probe.readLine();
		assertTrue(exists.equals("0"));
		
		// check for a user probe configuration in a nonexistent directory
		String bogusSecondary = m_probeDir+"Junk";
		args = TestProbeidIOExistsSetup(null, bogusSecondary, true);
		probe.process("ProbeIdIO", ProbeIdIO.parse(args));
		exists = probe.readLine();
		assertTrue(exists.equals("0"));

		// system probe configuration where corresponding user probe exists
		args = TestProbeidIOExistsSetup(null, null, true);
		assertTrue(getUserProbePath(probe).exists());
		assertFalse(getSysProbePath(probe).exists());
		probe.process("ProbeIdIO", ProbeIdIO.parse(args));
		exists = probe.readLine();
		assertTrue(exists.equals("0"));

		// system probe configuration where system and user configurations exist
		probe.process("ProbeIdIO", ProbeIdIO.parse(getImportProbeArgs(true)));
		assertTrue(getUserProbePath(probe).exists());
		assertTrue(getSysProbePath(probe).exists());
		args = TestProbeidIOExistsSetup(null, null, true);
		probe.process("ProbeIdIO", ProbeIdIO.parse(args));
		exists = probe.readLine();
		assertTrue(exists.equals("1"));
		
		// system probe configuration where user configuration doesn't exist
		String rm[] = {"-drop",m_tstProbeCfg,"-opt",m_probeDir};
		probe.process("ProbeIdIO", ProbeIdIO.parse(rm));
		assertFalse(getUserProbePath(probe).exists());
		assertTrue(getSysProbePath(probe).exists());
		args = TestProbeidIOExistsSetup(null, null, true);
		probe.process("ProbeIdIO", ProbeIdIO.parse(args));
		exists = probe.readLine();
		assertTrue(exists.equals("1"));
		
		System.out.println("test " + testNum + " PASSED: "+ description);
	}
	
	private String[] testAddRowSetup(String crypto, String comment, String value) 
		throws Exception 
	{
		setUp();
		populate(true, crypto.equals("-crypto"));
		String test_addrow[] = { // check new probe configuration
			"-addrow", m_probeDir + "/safety_levels",
			"-cfg", m_tstProbeCfg,
			"-opt", comment,
			"-opt", value,
			crypto, "-v"
		};
		return test_addrow;
	}
	
	private void testAddRow(String cryptArg) throws Exception {
		String comment     = "# 100W high band/300W low band";
		String value       = "49.0 49.0 15.0 15.0";
		String[] args      = testAddRowSetup(cryptArg, comment, value);
		ProbeIdIOWrapper probe = newProbe(1);
		probe.process("ProbeIdIO", ProbeIdIO.parse(args));
		String[] check     = {"-readfile", "safety_levels", "-opt", m_probeDir, cryptArg};
		probe.process("ProbeIdIO", ProbeIdIO.parse(check));
		String readComment = probe.readLine();
		String readValue   = probe.readLine();
		assertTrue(comment.equals(readComment));
		assertTrue(value.equals(readValue));
	}

	@Test
	public void ProbeIdIOTestAddRowNoCrypto() throws Exception {
		testNum++;
		String description = "Add Row to unencrypted table";
		testAddRow("-nocrypto");
		System.out.println("test " + testNum + " PASSED: "+ description);
	}

	@Test
	public void ProbeIdIOTestAddRowCrypto() throws Exception {
		testNum++;
		String description = "Add row to encrypted table";
		testAddRow("-crypto");
		System.out.println("test " + testNum + " PASSED: "+ description);
	}
	
	/* run precondition tests */
	private void precondition(ProbeIdIO probe, Map<String,String[]> map, String[] keys) {
		if (keys != null) 
			for (String key: keys)
				try {
					probe.process("precondition",ProbeIdIO.parse(map.get(key)));
				} catch (Exception e) {
					e.printStackTrace();
				}
	}
	
	/* post-condition processing - evaluate results, cleanup, etc */
	private void postcondition(String key) {
		;
	}

	@Test
	public void ProbeIdIOTestTodo() throws Exception {
		// here's the place to run any one-off tests so that we
		// don't have to monkey with setting the command-line
		// parameters in the cumbersome eclipse GUI
		Map<String, String[]> preconditions  = new TreeMap<String, String[]>();
		
		// some of the tests have been moved to ProbeIdIOTest JUnit framework
		//@SuppressWarnings("unused")
		String test_commit_file[] = {
			"-commit", "/vnmr/probeid/test/probes/tmp.vj.pio", // commit file
			"-opt", "test_add", 			 // primary key
			"-opt", "probes/test_add",       // secondary key
			"-sys",                          // system directory
			"-cache", "/vnmr/probeid/cache",
			"-mnt", "/vnmr/probeid/mnt",
			"-nocrypto", "-v"
		};
		@SuppressWarnings("unused")
		String test_getparam[] = {
			"-getparam", "Probegcal",
			"-opt", "probes/autox400DB",
			"-cfg", "autox400DB",
			"-cache", "/vnmr/probeid/cache.noencrypt",
			"-mnt", "/vnmr/probeid/mnt.noencrypt",
			"-v"
		};
		@SuppressWarnings("unused")
		String test_unencrypted_getparam[] = {
			"-getparam", "Probegcal",
			"-opt", "probes/autox400DB",
			"-cfg", "autox400DB",
			"-cache", "/vnmr/probeid/cache.noencrypt",
			"-mnt", "/vnmr/probeid/mnt.noencrypt",
			"-nocrypto", "-v",
		};
		// read from /tmp/tst/probe_mnt/*/probes/autox400DB/autox400DB
		@SuppressWarnings("unused")
		String test_readfile[] = {
			"-readfile", "autox400DB",
			"-param", "",
			"-opt", "probes/autox400DB",
			"-cfg", "autox400DB",
			"-cache", "/vnmr/probeid/cache.noencrypt",
			"-mnt", "/vnmr/probeid/mnt.noencrypt",
			"-v"
		};
		@SuppressWarnings("unused")
		String test_readfile_path[] = {
			"-readfile", "autox400DB",
			"-param", "",
			"-opt", "archive/sample-xyz/dirinfo/macdir:/probes/autox400DB",
			"-cfg", "autox400DB",
			"-cache", "/vnmr/probeid/cache.noencrypt",
			"-mnt", "/vnmr/probeid/mnt.noencrypt",
			"-nocrypto", "-v"
		};
		@SuppressWarnings("unused")
		String test_query_info[] = {
			"-query", "info",
			"-cache", "/vnmr/probeid/cache.noencrypt",
			"-mnt", "/vnmr/probeid/mnt.noencrypt",
			"-v"
		};
		@SuppressWarnings("unused")
		String test_perm[] = { // check permissions of probe directory
			"-perm", "rw",
			"-opt", "probes",
			"-sys", 
			"-cache", "/vnmr/probeid/cache.noencrypt",
			"-mnt", "/vnmr/probeid/mnt.noencrypt",
			"-v"
		};
		//@SuppressWarnings("unused")
		String test_delete[] = { // check deletion
			"-drop", "test_add",
			"-opt", "probes/test_add",
			"-opt", "tune/test_add",
			"-opt", "tune/tunecal_test_add",
			"-cache", "/vnmr/probeid/cache.noencrypt",
			"-mnt", "/vnmr/probeid/mnt.noencrypt",
			"-nocrypto", "-v"
		};
		//@SuppressWarnings("unused")
		String test_add[] = { // check new probe configuration
			"-add", "test_add",
			"-opt", "probes/test_add", // "-sys",
			"-cache", "/vnmr/probeid/cache.noencrypt",
			"-mnt", "/vnmr/probeid/mnt.noencrypt",
			"-nocrypto", "-v"
		};
		@SuppressWarnings("unused")
		String test_add_sys[] = {
			"-add", "test_add",
			"-opt", "probes/test_add", "-sys",
			"-cache", "/vnmr/probeid/cache.noencrypt",
			"-mnt", "/vnmr/probeid/mnt.noencrypt",
			"-nocrypto", "-v"
		};
		@SuppressWarnings("unused")
		String test_query_cache[] = {
			"-query", "cache",
			"-cache", "/vnmr/probeid/cache",
			"-mnt", "/vnmr/probeid/mnt",
			"-nocrypto", "-v"
		};

		// add any test_* that can serve as preconditions here
		preconditions.put("test_addrow", test_add);
		preconditions.put("test_delete", test_delete);
		preconditions.put("test_commit", test_commit_file);

		// Set up a probe with default configuration
		// start an instance of the ProbeIdIO interface after parameter
		ProbeIdIO probe = new ProbeIdIO();
		probe.attach();

        // null or a quick'n'dirty test
		String test[] = test_commit_file;  // test_add;

		// set up for the actual test
		String[] pre = null;//{"test_delete", "test_add"}; // prepare for test_addrow
		// test to check for results and clean up
		String post = null;
		precondition(probe, preconditions, pre);	
		System.out.println("STARTING TEST WITH ARGS");
		
		// parsing and System variable initialization
		probe.process(test[0],ProbeIdIO.parse(test));
		System.out.println("END TEST WITH ARGS");
		postcondition(post);

	}
}