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
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintStream;

import vnmr.vjclient.VnmrProxy;

/**
 * Class to hold utilities for small files to hold small amounts
 * of probe server state information.
 * @author dirk
 *
 */
public class ProbeIdTinyFiles implements ProbeId {
	/**
	 * The probe server is largely stateless, but ProTune does not
	 * currently know about probe id.  Until it does, we save the
	 * state of the last "attached" probe so that it can be restored
	 * on startup, in case the probe server is restarted before
	 * another probe is attached.
	 * TODO: modify APT so that it passes the probe id along with
	 *       each probe server request, then remove statefulness.
	 * @return
	 */
	private static File getStateFile() {
		File dir = new File(VnmrProxy.getPersistenceDir(), DEFAULT_PROBEID_PERSIST_DIR);
		File file = new File(dir, DEFAULT_STATE_FILENAME);
		return file;
	}
	
	public static File getFlushFile(File dir) {
		String key = ProbeIdDb.getKeyFromHostFile(DEFAULT_FLUSH_FILENAME);
		File file = ProbeIdDb.getPath(dir.getPath(), key);
		return file;
	}
	
	public static boolean needsFlush(File timestamp, File target) {
		if (timestamp != null && timestamp.exists())
			return target.lastModified() > timestamp.lastModified();
		return true;
	}
	
	public static void timestampFlush(File dir) {
		File timestamp = getFlushFile(dir);
		timestamp(timestamp);
	}
	
	public static void timestamp(File timestamp) {
		if (timestamp != null)
			writeln(timestamp, Long.toString(System.currentTimeMillis()), "cache time stamp");
	}

	public static String getSavedId() {
		return readln(getStateFile(), "Probe ID persistence file");
	}
	
	public static void setSavedId(String id) {
		writeln(getStateFile(), id, "probe ID");
	}
	
	private static String readln(File file, String desc) {
		BufferedReader reader = null;
		if (file.canRead()) {
			try {
				reader = new BufferedReader(new FileReader(file));
				String line = reader.readLine();
				if (line != null) {
					String id = line.trim();
					return id.isEmpty() ? null : id;
				}
			} catch (FileNotFoundException e) {
			} catch (IOException e) {
				ProbeIdIO.error("IO error reading "+desc+file.getPath());
				e.printStackTrace();
			} finally {
				if (reader != null) try {
					reader.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}
		return null;
	}

	private static void writeln(File file, String id, String desc) {
		PrintStream writer = null;
		try {
			try {
				file.getParentFile().mkdirs();
				FileOutputStream os = new FileOutputStream(file);
				writer = new PrintStream(os);
			} catch (FileNotFoundException e) {
				ProbeIdIO.error("error saving "+desc+" "+file.getPath());
				e.printStackTrace();
			}
			if (writer != null)
				writer.println(id);
		} finally {
			writer.close();
		}
	}
}
