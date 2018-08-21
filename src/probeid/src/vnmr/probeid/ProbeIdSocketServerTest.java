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

import org.junit.BeforeClass;
import org.junit.Test;

public class ProbeIdSocketServerTest {
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		ProbeIdIO.setProbeIdSystemDefaults(); // set up with default configuration
		System.setProperty(ProbeId.KEY_PROBEID_ETHERPORT, "eth0");
		System.setProperty(ProbeId.KEY_PROBEID_ENCRYPT, "false");
		System.setProperty(ProbeId.KEY_PROBEID_USEPROBEID, "true");
		System.setProperty(ProbeId.KEY_PROBEID_PASSWORD, "invalid"); // shouldn't be used
		System.setProperty(ProbeId.KEY_PROBEID_MOUNTPOINT, "/tmp/probe_mnt");
		System.setProperty(ProbeId.KEY_PROBEID_CACHE, "");           // effectively disable caching
		System.setProperty(ProbeId.KEY_PROBEID_DBDIR, "/tmp/probe_mnt");
	}
	
	@Test
	public void testProbeIdSocketServer() {
		System.setProperty(ProbeId.KEY_PROBEID_REMOTE, "localhost");
		int portNumber = ProbeId.DEFAULT_PORT; // TODO get this from command line and/or config file
		byte [] data_in = new byte [1024];
		byte [] data_out = "0123456789".getBytes(); // sample data
		String host = "testhost";        // host part of the key
		String user = "testuser";        // user part of the key
		String file = "testfile";        // filename portion of the key
		String key = ProbeIdDb.getKeyFromHostUserFile(host, user, file);
		ProbeIdServer server = new ProbeIdSocketServer(portNumber);
		ProbeIdClient client = new ProbeIdClient(portNumber);
		System.out.println("start probeid server");
		server.start();                  // kick off the server
		
		client.write(data_out, key);     // write some data to the database
		client.read(data_in, key);       // read the data back
		boolean match = true;
		for (int i=0; i<data_out.length; i++)
			match = match && data_in[i] == data_out[i];
		assertTrue(match);               // verify that they match
		System.out.println("ProbeIdServer Unit Test: PASSED");
	}
}
