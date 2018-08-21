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
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.channels.FileLock;

public class ProbeIdFileLock implements ProbeId {
	private FileLock         m_lock = null;
	private FileOutputStream m_lockStream = null;
	private File             m_lockFile = new File(DEFAULT_SERVER_LOCKFILE);
	
	public void acquire() {
		try {
			m_lock = m_lockStream.getChannel().lock();
		} catch (IOException e) {
			ProbeIdIO.error("error acquiring lock file "+m_lockFile.getPath()+": "
							+ e.getMessage());
			e.printStackTrace();
		}
	}
	
	public void release() {
		if (m_lock != null) {
			try {
				m_lock.release();
			} catch (IOException e) {
				ProbeIdIO.error("error releasing lock file "+m_lockFile.getPath()+": "
						+ e.getMessage());
				e.printStackTrace();
			}
		} else 
			ProbeIdIO.warning(m_lockFile.getPath()+": release null lock");
	}

	/** 
	 * Send a request to probe server with exclusive access.  This method uses a 
	 * workaround for incompatibility between java channel locks and linux shell 
	 * scripts flock.
	 * @param options
	 * @return output from the probe server
	 */
	public String run(String options) {
		String result = new String();
		String cmd = "probeid "+options;
		try {
			Process probeid_shell = Runtime.getRuntime().exec(cmd);
			BufferedReader in= new BufferedReader(new InputStreamReader(probeid_shell.getInputStream()));
			String line = null;
			while ((line = in.readLine()) != null) {
				result += line + '\n';
			}
		} catch (IOException e) {
			ProbeIdIO.error("error reading probe server "+ e.getMessage());
			e.printStackTrace();
		}
		return result;
	}
	
	public ProbeIdFileLock() throws FileNotFoundException {
		try {
			m_lockStream = new FileOutputStream(m_lockFile);
		} catch (FileNotFoundException e) {
			throw new FileNotFoundException("lock error on "+ProbeId.DEFAULT_SERVER_LOCKFILE);
		}
	}
}