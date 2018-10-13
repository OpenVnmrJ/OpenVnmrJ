/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

import vnmr.vjclient.VnmrProxy;

/**
 * ProbeIdIOMonitor monitors the mount point for probe connection
 * and disconnection events by comparing the probe ID on the probe
 * against the expected probe ID for the purpose of keeping VnmrJ
 * and its probe-related GUI elements up to date.  ProbeId checks
 * probe connectivity independently of this monitor thread.
 * 
 * If there is no probe ID, then the probe is assumed to be
 * disconnected.  VnmrJ is sent a command to clear the value of 
 * the probe and probe ID parameters.
 * 
 * If the probe ID matches the expected probe, then no event notification
 * takes place.  
 * Otherwise VnmrJ is sent a command to clear the value of the probe
 * parameter and updates the value of the probe ID parameter.
 * 
 * @author dirk
 *
 */
public class ProbeIdMonitor implements ProbeId, Runnable {
	final public int stopTimeout = 1000;  // a generous timeout (ms)
	private int           m_interval = DEFAULT_MONITOR_INTERVAL; // ms
	private ProbeIdIO     m_probeId  = null;
	private static Thread m_thread   = null;
	private boolean       m_run      = false;
	
	public int getInterval()  { return m_interval; }
	public Thread getThread() { return m_thread; }
	
	public Thread start() {
		assert(m_probeId != null);
		m_run = true;
		if (m_thread == null) {
			m_thread = new Thread(this);
			m_thread.start();
		}
		return m_thread;
	}
	
	public Thread stop() {
		Thread dead = m_thread;
		m_run = false;
		try {
			m_thread.join(m_interval + stopTimeout);
			m_thread = null;
		} catch (InterruptedException e) {
			ProbeIdIO.error("Monitor failed to stop "+e.getMessage());
			e.printStackTrace();
			dead = null;
		}
		return dead;
	}
	
	public void run() {
		boolean wasConnected = false;
		String  wasProbeId   = "";
		this.getThread().setName("Monitor");
		while (m_run) { // false if stop is called
			try {
				Thread.sleep(m_interval);
			} catch (InterruptedException e) {
				ProbeIdIO.error("ProbeId Monitor: interrupted "+e.getMessage());
			}
			String probeId    = m_probeId.readProbeId(false);
			boolean connected = probeId != null;
			boolean matched   = connected && m_probeId.checkProbeId(null, null);
			boolean changed   = connected && !probeId.equals(wasProbeId);
			
			if (wasConnected && !connected) {        // disconnect
				VnmrProxy.sendDisconnectNotification();
				wasConnected = false;
			} else if (connected && (!wasConnected || !matched)) {
				Boolean send = Boolean.valueOf(System.getProperty(KEY_PROBEID_INFO2VJ, 
																  INFORM_VNMRJ.toString()));
				m_probeId.attach(); // reconnect caches, etc.
				if (send)
					VnmrProxy.sendNewProbeNotification(probeId);
				wasConnected = true;
				wasProbeId   = probeId;
			} else if (changed) {
				Boolean send = Boolean.valueOf(System.getProperty(KEY_PROBEID_INFO2VJ, 
						  INFORM_VNMRJ.toString()));
				if (send)
					VnmrProxy.sendNewProbeNotification(probeId);
				wasProbeId = probeId;
			}
		}
	}
	
	ProbeIdMonitor() {
		m_interval = Integer.parseInt(
				System.getProperty(KEY_PROBEID_MONITOR_INTERVAL, 
								   DEFAULT_MONITOR_INTERVAL.toString()));
	}
	
	ProbeIdMonitor(ProbeIdIO probeId) {
		this();
		m_probeId  = probeId;
	}
	
	public static boolean isRunning(ProbeIdMonitor monitor) {
		return monitor != null && monitor.isAlive();
	}
	
	private boolean isAlive() {
		return m_thread != null && m_thread.isAlive();
	}
}
