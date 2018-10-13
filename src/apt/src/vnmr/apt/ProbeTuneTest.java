/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import static org.junit.Assert.*;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.Vector;
import java.util.concurrent.Semaphore;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import vnmr.probeid.ProbeIdIOTest;
import vnmr.probeid.ProbeIdIO;

public class ProbeTuneTest {
	static Thread mmThread = null;
	static Thread mtuneThread = null;
	static Mtune  mTune = null;
	static ProbeIdIOTest probeIdIOTest = null;
	static ProbeIdIO server = null;
	static String probeId = null;
	static String[] argsLegacy = {
			"-motorIP", "127.0.0.1", 	// motor simulator interface
			"-probe", "autox400DB",		// define probe configuration
			"-noGui"                    // disable GUI
	};
	static String[] argsProbeId = {
			"-motorIP", "127.0.0.1", 	// motor simulator interface
			"-probe", "autox400DB",		// define probe configuration
			"-noGui",                   // disable GUI	
			"-probeid"					// enable Probe ID
	};
	// TODO: static String[][] argsTest = { argsLegacy, argsProbeId };
	static String[][] argsTest = { argsProbeId };
	
	static class MotorThread implements Runnable {
		public void run() {
			String[] args = {};
			MotorModule.main(args);
		}
	}
	
	static class MtuneThread implements Runnable {
		public void run() {
			String[] args = {};
			mTune = Mtune.setup(args);
		}
	}
	
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		// let ProbeTune know that we're not talking to VnmrJ
    	System.setProperty("INFO2VJ", Boolean.toString(false));
		// clean up after a previous run in case it crashed
		ProbeIdIOTest.tearDown();
		ProbeIdIOTest.tearDownAfterClass();
		// start the motor and data acquisition simulators
		startSimulators();
		// set up Probe ID test environment
		ProbeIdIOTest.setUpBeforeClass();
		// access the Probe ID test infrastructure
		probeIdIOTest = new ProbeIdIOTest();
		// populate the initial probe with crypto and caching enabled
		probeIdIOTest.setUp();
		probeIdIOTest.populateAll(true, true, true);
		// start the probe server in a separate thread and attach a probe
		probeIdIOTest.startServer(null);
		probeId = probeIdIOTest.attachProbe();
		ProbeIdIO.io_synch = true;
		ProbeIdIO.io_lock = new Semaphore(1);
		ProbeIdIO.io_lock.acquire();
		AptProbeIdClient.io_synch = true;

		System.out.println("--------------<setupBeforeClass done>");
	}

	private static void startSimulators() {
		System.out.println("--------------<startSimulators start>");
		// start motor simulator
		mmThread = new Thread(null, new MotorThread(), "MotorModule");
		mmThread.start();
		// start data simulator
		mtuneThread = new Thread(null, new MtuneThread(), "Mtune");
		mtuneThread.start();
		System.out.println("--------------<startSimulators done>");
	}

	@AfterClass
	public static void tearDownAfterClass() throws Exception {
		System.out.println("--------------<teardown start>");
		stopSimulators();
		probeIdIOTest.stopServer();
		ProbeIdIOTest.tearDown();
		probeIdIOTest = null;
		ProbeIdIOTest.tearDownAfterClass();
		System.out.println("--------------<teardown done>");
	}

	private static void writeSimulator(int port, String cmd) throws IOException {
		Socket socket = new Socket();
		InetAddress inetAddr = InetAddress.getByName("127.0.0.1");
        InetSocketAddress sockAddr = new InetSocketAddress(inetAddr, port);
        socket.connect(sockAddr, 30000);
        PrintWriter out = new PrintWriter(socket.getOutputStream());
        out.write(cmd);
        out.flush();
        socket.close();
	}
	
	private static void stopSimulators() {
		try {
			writeSimulator(AptDefs.DEBUG_MOTOR_PORT,"quit");
			MotorModule.quit(10000);
			mmThread.interrupt();
			mmThread.join(10000);
			mTune.quit(10000);
		} catch (IOException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}

	@Before
	public void setUp() throws Exception {
	}

	@After
	public void tearDown() throws Exception {
	}

	private void tune(boolean probeid) throws InterruptedException {
		String probeName = "autox400DB";
		String[] tuneCmd = {"-exec", "tuneto 400 fine 1"};
		String[] args = {
				"-motorIP", "127.0.0.1", 
				"-probe", probeName,
				"-vnmrsystem", "/vnmr",
				"-vnmruser", System.getProperty("user.home")+File.separator+"vnmrsys",
				"-noGui", 
				"-probeid", Boolean.toString(probeid),
				"-sweepTimeout", Integer.toString(9999999),
		};
		System.setProperty("apt.motorHostname", args[1]);
		ProbeTune.setup(args);
		MtuneControl.reset();
		ProbeTune tuner = new ProbeTune(probeName, probeName);
		tuner.execTest(tuneCmd[1]);
		tuner.quit(1000); // no more ProbeTune threads should be running after this
	}

	@Test
	public void testTuning() throws InterruptedException {
		tune(true);
		resetSimulators();
		tune(false);
		//checkEquivalence();
	}

	private void resetSimulators() {
		stopSimulators();
		startSimulators();
	}

	private void validate(ProbeTune tst,
						  Vector<Boolean> chk, Vector<String> names, 
						  String probeName, boolean sys, 
						  boolean expected) 
	{
		boolean ok = tst.checkProbeName(probeName,sys);
		chk.add(ok);
		// verify that the legacy and probeid answers match
		if (expected)
			assertTrue(chk.lastElement());
		else
			assertFalse(chk.lastElement());

		if (chk.size()>1)
			assertTrue(chk.lastElement().equals(chk.firstElement()));

		//String name = tst.getProbeName();
		//names.add(name == null ? "" : name);
		//assertTrue(name != null && name.equals(probeName));
	}
	
	/**
	 * Test that the checkProbeName interface between legacy and probe ID
	 * runs match. 
	 */
	@Test
	public void testCheckProbeName() {
		Vector<Boolean> chkBoth     = new Vector<Boolean>(argsTest.length);
		Vector<Boolean> chkSys      = new Vector<Boolean>(argsTest.length);
		Vector<Boolean> chkBogusSys = new Vector<Boolean>(argsTest.length);
		Vector<Boolean> chkBogusUsr = new Vector<Boolean>(argsTest.length);
		Vector<String>  id  = new Vector<String>(argsTest.length);

		for (String[] args : argsTest) {
			ProbeTune tst = null;
			try {
				MtuneControl.reset();
				String probeName = ProbeTune.setup(args);
				tst = new ProbeTune(probeName, probeName);
			} catch (Exception e) {
				e.printStackTrace();
				fail("unexpected exception "+e.getMessage());
			}

			// should be in system directory
			validate(tst, chkBoth, id, args[3], false, true);

			// should be found in system directory but not in user
			validate(tst, chkSys, id, args[3], true, true);

			// shouldn't be found in system directory
			validate(tst, chkBogusSys, id, "bogus", false, false);

			// shouldn't be found in system or user directory
			validate(tst, chkBogusUsr, id, "bogus", true, false);

			// other combinations are possible and should be
			// tested as part of ProbeIdIO unit tests
		}
	}

	@Test
	public void testGetProbeFile() throws InterruptedException {
		String[] args = {"-motorIP", "127.0.0.1", "-probe", "autox400DB"};
		System.setProperty("apt.motorHostname", args[1]);
		ProbeTune tst = new ProbeTune(args[3], args[3]);
		tst.getProbeFile(args[3], false);
	}

	@Test
	public void testGetPersistenceReader() {
		fail("Not yet implemented");
	}

	@Test
	public void testGetPersistenceWriter() {
		fail("Not yet implemented");
	}

}
