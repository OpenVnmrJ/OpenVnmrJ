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
import java.io.IOException;

public class ProbeIdSimulatedProbe implements ProbeId {
	/**
	 * simulate physically connecting a probe.  This will be a lot
	 * more flexible once Java supports symbolic links in Java SE1.7.
	 * For now everything lives in the same file system.
	 * 
	 * @param dst - where to "mount" the probe file system
	 * @param src - where to store "disconnected probes"
	 * @return
	 */
	public static void link(String src, String dst) throws IOException {
		String link = "ln -s "+src+" "+dst;
		try {
			Runtime.getRuntime().exec(link).waitFor();
		} catch (InterruptedException e) {
			ProbeIdIO.error("probe server: I/O error while linking probe "+src+" to "+dst);
		}
	}
	
	public static File linkedTo(File link) {
		try {
			return link.getCanonicalFile();
		} catch (IOException e) {
			ProbeIdIO.error("probe server: I/O error accessing probe "+link.getPath());
		}
		return null;
	}
	
	/**
	 * Logically connect the probe to the system.  May be a simulated probe.
	 * @param probePath
	 * @return
	 */
	public static boolean connect(String probePath, boolean report) {
		String mountDir = System.getProperty(ProbeId.KEY_PROBEID_MOUNTPOINT,
											 ProbeId.DEFAULT_LOGICAL_MNT);
		File mnt = new File(mountDir);
		File probe = new File(probePath);
		int population = mnt.exists() ? mnt.listFiles().length : 0;
		if (population != 0) {
			if (report)
				ProbeIdIO.error("probe server: mount point "+mountDir+" not empty");
			return false;
		}
		try {
			if (mnt.exists())
				mnt.delete();
			link(probe.getPath(), mnt.getPath());
		} catch (IOException e) {
			ProbeIdIO.error("probe server: I/O error while connecting probe "+probe.getPath());
			return false;
		}
		return true;
	}

	/**
	 * Logically disconnect the probe from the system.
	 * @return success or failure
	 */
	public static boolean disconnect() {
		String mountDir = System.getProperty(ProbeId.KEY_PROBEID_MOUNTPOINT,
											 ProbeId.DEFAULT_LOGICAL_MNT);
		try {
			File mnt = new File(mountDir);
			int population = mnt.exists() ? mnt.listFiles().length : 0;
			if (ProbeIdDb.isLink(mnt) || mnt.exists() && population > 0) {
				if (!ProbeIdDb.isLink(mnt)) {
					ProbeIdIO.error("probe server: current probe mountpoint "+mountDir
									+" is not a symblic link to the probe directory");
					return false;
				}
				mnt.delete();		        // it's only a symbolic link...
				return(mnt.mkdir());        // to make it look right
			}
			return true;                    // it's already deleted
		} catch (IOException e) {
			ProbeIdIO.error("probe server: I/O error while disconnecting probe "+mountDir);
		}
		return false;
	}
	
	public static boolean delete(String probePath) {
		String mountDir = System.getProperty(KEY_PROBEID_MOUNTPOINT, DEFAULT_LOGICAL_MNT);
		File mnt = new File(mountDir);
		File probe = null;
		try {
			// get a handle on the canonical location of probePath
			boolean rooted = false;
			if (probePath != null) {
				for (File root : File.listRoots())
					rooted |= probePath.startsWith(root.getCanonicalPath());
				
			}
			// if it isn't rooted, then put it in the default location
			probe = rooted 
				? new File(probePath) : new File(mnt.getParentFile(), probePath);

			// verify that the probe is in the right canonical directory
			// i.e. must be a subdirectory of mountpoint's parent
			String probeCanon = probe.getCanonicalPath();
			String mountCanon = mnt.getParentFile().getCanonicalPath();
			boolean inProbePath = probeCanon.startsWith(mountCanon);
			if (inProbePath)
				ProbeIdDb.rmdir(probe);
			else
				throw(new IOException("probe path must be in "
									  +mnt.getParentFile().getCanonicalPath()));
			
			// disconnect the probe if it is mounted
			boolean probeIsMounted = mnt.getCanonicalPath().equals(probe.getCanonicalPath());
			if (probePath == null || probeIsMounted)
				disconnect();
			return !probe.exists();
		} catch (IOException e)	{
			ProbeIdIO.error("probe server: I/O error while disconnecting probe "
							+probe.getPath());
		}
		return false;
	}
}