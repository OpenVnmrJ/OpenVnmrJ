/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

public class ProbeIdMismatchException extends Exception {
	// to support Remote Method Invocation at some point in the future
	private static final long serialVersionUID = 5296521625626450529L;
	public final String expectedProbeId;
	
	public ProbeIdMismatchException(String msg) { 
		super(msg); 
		expectedProbeId = null;
	}
	
	public ProbeIdMismatchException(String msg, String probeId) {
		super(msg);
		expectedProbeId = probeId;
	}

	public ProbeIdMismatchException(String msg, String probeId, Throwable t) {
		super(msg, t);
		expectedProbeId = probeId;
	}
}
