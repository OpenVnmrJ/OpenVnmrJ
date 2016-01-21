/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.vjclient;

/**
 * A proxy interface to VnmrJ.
 * Based on Chris Price's ProTune.send2Vnmr.
 */
import java.io.*;
import java.net.InetAddress;
import java.net.Socket;

public class VnmrProxy implements VnmrIO {
	Integer m_vnmrPid        = null;
	Integer m_vnmrPort       = null;
	String  m_vnmrHost       = null;
	Socket  m_socket         = null;
	OutputStream   m_ostream = null;
	BufferedWriter m_writer  = null;
	
	static final String cmdClearId   = "setvalue('probeid',' ','systemglobal') ";
	static final String cmdDisconnect= "probeid('disconnect') ";
	static final String cmdConnect   = "probeid('connect') ";
	static final String cmdSetValue(final String param, final String value, final String tree) {
		return "setvalue('"+param+"','"+value+"','"+tree+"') ";
	}
	static final String cmdNotification(final String notice) {
		return "probeid('notice','"+notice+"') ";
	}
	static final String cmdPnew      = "vnmrjcmd('pnew','probeid')";
	
	VnmrProxy(final String talkFilePath) {
		readTalkFile(talkFilePath);
	}
	
	private void open() {
		try {
			m_socket  = new Socket(InetAddress.getLocalHost(), m_vnmrPort);
			m_ostream = m_socket.getOutputStream();
			m_writer  = new BufferedWriter(new OutputStreamWriter(m_ostream));
		} catch (IOException ioe) {
			ioe.printStackTrace();
		}
	}
	
	private void readTalkFile(final String talkFilePath) {
	    BufferedReader reader = null;
		try {
			FileInputStream    in = new FileInputStream(new File(talkFilePath));
			reader = new BufferedReader(new InputStreamReader(in));
			String           line = reader.readLine();
			String []      fields = line.split(" ");
			String         reason = "mangled talk file contents";
			if (fields.length != 3)
				throw new IOException(talkFilePath + reason);
			reason = ": unexpected token '" + fields[0] + "'";
			if (!fields[0].startsWith("vnmr-vm")) 
				throw new IOException(talkFilePath + reason);
			
			reason = ": unexpected token '" + fields[1] + "'";
			m_vnmrPid = Integer.decode(fields[1]);

			reason = ": unexpected token '" + fields[1] + "'";
			m_vnmrPort = Integer.decode(fields[2]);
		} catch (IOException ioe) {
			// TODO Auto-generated catch block
			ioe.printStackTrace();
		} catch (NumberFormatException ne) {
			
			ne.printStackTrace();
	        } finally {
	            try { reader.close(); } catch(Exception e) { }
		}
	}

	/**
	 * return user directory in the host file system, which is not
	 * necessarily that specified by the "user.home" system property.
	 * @return
	 */
	static public String getRelUserDir() {
		String usrDir = System.getProperty(KEY_VNMR_USRDIR);
		if (usrDir == null || usrDir.isEmpty())
			usrDir = System.getProperty(KEY_USRDIR, 
										DEFAULT_USRDIR);
		return usrDir;
	}
	
	static public String getUserDir() {
		String usrDir = getRelUserDir();
		if (!usrDir.startsWith(File.separator))
			usrDir = System.getProperty("user.home") + File.separator + usrDir;
		return usrDir;
	}

	static public String getSystemDir() {
		String sysDir = System.getProperty(KEY_VNMR_SYSDIR);
		if (sysDir == null)
			sysDir = System.getProperty(KEY_SYSDIR, DEFAULT_SYSDIR);
		return sysDir;
	}

	static public String getPersistenceDir() {
		String dir = getUserDir() + File.separator + DEFAULT_PERSISTENCE_DIR;
		return dir;
	}
	
	/**
	 * find the send2Vnmr "talk" file in the user vnmrsys file system.
	 * @return talk file
	 */
	static public File getTalkFile() {
		String usrDir = getUserDir();
		String talker = System.getProperty(KEY_TALK2VNMRJ, 
						   DEFAULT_TALK2VNMRJ);
		String pidPath = usrDir + File.separator + talker;
		if (!pidPath.startsWith("/"))
			pidPath = System.getProperty("user.home") + File.separator + pidPath;

		File pidFile = new File(pidPath);
		
		if (!pidFile.canRead()) {
			System.out.println("can't read " + pidFile.getPath());
			Messages.postError("can't read " + pidFile.getPath() + ": please issue 'listenon' command");
		}
		assert pidFile.canRead();
		return pidFile;
	}
	
	static public File getTalkExec() {
		String sysDir = getSystemDir();
		String binDir = sysDir + File.separator + "bin";
		String exePath = binDir + File.separator + "send2Vnmr";
		File exeFile = new File(exePath);
		if (!exeFile.canExecute()) {
			System.out.println("can't execute " + exeFile.getPath());
		}
		assert exeFile.canExecute();
		return exeFile;
	}
	
	// FIXME: replace this with a registered callback
	static public void sendDisconnectNotification() {
		//sendToVnmr(cmdClear + cmdClearId + cmdPnew);
		sendToVnmr(cmdClearId + cmdDisconnect + cmdPnew);
	}
	
	// FIXME: replace this with a registered callback
	static public void sendNewProbeNotification(final String probeId, final String tree) {
		String id = (probeId==null) ? " " : probeId;
		sendToVnmr(cmdSetValue("probeid", id, tree) + cmdConnect + cmdPnew);// + cmdClear + cmdPnew)
	}
	
	// FIXME: replace this with a registered callback
	static public void sendNewProbeNotification(final String id) {
		sendNewProbeNotification(id, "systemglobal");
	}
	
	static public void sendNotification(final String notice) {
		sendToVnmr(cmdNotification(notice));
	}
	
	static class VnmrTalkAlarm extends Thread {
		Thread target = null;
		static boolean expired = false;
		
        VnmrTalkAlarm() { 
        	target = Thread.currentThread(); 
        	expired = false;
        }
        public void cancel() { interrupt(); }
        static boolean hasExpiredOnce() { return expired; }
		@Override
		public void run() {
			try {
				Thread.sleep(TIMEOUT_VNMRJ);
			} catch (InterruptedException e) {
				return;
			}
			expired = true;
			target.interrupt();
		}
	}
	/** allow send2Vnmr attempts again */
	static void reset() { VnmrTalkAlarm.expired = false; }
	
	static public synchronized boolean sendToVnmr(final String command) {
		if (VnmrTalkAlarm.expired)
			return VnmrTalkAlarm.hasExpiredOnce();
		File info = getTalkFile();
		File exec = getTalkExec();
		if (!info.exists() || !exec.canExecute())
			return false;
		VnmrTalkAlarm alarm = new VnmrTalkAlarm();
		String reason = "";
		Messages.postDebug("VnmrProxy.sendToVnmr", "talk file = \""
				+ info.getPath() + "\", cmd=\"" + command + "\"");
		String[] cmdArray = { exec.getPath(), info.getPath(), command };
		if (command != null) try {
			//ProbeIdIO.info("probeid: send2Vnmr " + command);
			Process process = Runtime.getRuntime().exec(cmdArray);
			alarm.start();
			@SuppressWarnings("unused")   // to facilitate debug
			int exit = process.waitFor();
			alarm.cancel();
			return true;
			//ProbeIdIO.debug("probeid "+command+" exited with value "+Integer.toString(exit));
		} catch (IOException e) {
			reason = "send2Vnmr shell command failed: " + e; 
		} catch (InterruptedException e) {
			if (!VnmrTalkAlarm.expired) {
				reason="send2Vnmr shell command interrupted: " + e;
				e.printStackTrace();
			} else {
				reason="send2Vnmr timed out - make sure it is enabled in VnmrJ with 'listenon'";
			}
		} catch (Exception e) {
			reason = "send2Vnmr shell command exception " + e;
		} else reason = " null command";
		if (!reason.isEmpty()) {
			Messages.postError("Cannot send message to Vnmr: " + reason);
		}
		return false;
   }

	/**
	 * Create a parameter with the specified value.  Because the VnmrJ
	 * 'create' command doesn't assign value if it already exists, and
	 * because we don't get the return value back from the command,
	 * the value is assigned separately with a call to 'setvalue'.
	 * 
	 * Also, type is inferred from the type of the 'value' parameter.
	 * 
	 * 'create' semantics are therefore slightly different than the
	 * VnmrProxy.create semantics.
	 * 
	 * @param param
	 * @param tree
	 * @param value
	 */
	public static void create(final String param, String tree, final Object value) {
		String cmd = "create('" + param + "'";
		String end = ")";
		String type = "";
		String val = "";
		tree = tree == null ? "" : ",'" + tree + "'";
		if (value != null) {
			if (value instanceof Double)
				type = ",'real'";
			else if (value instanceof String)
				type = ",'string'";
			else if (value instanceof Integer)
				type = ",'integer'";
			else
				Messages.postError("create: type of " + value.toString() + " is not supported");
			val = ",'" + value.toString() + "'";
		}
		cmd += type + tree + val + end;
		sendToVnmr(cmd);
		if (!val.isEmpty()) {
			String set = "setvalue('" + param + "'" + val + tree + end;
			sendToVnmr(set);
		}
	}
	
	// assume 'global' tree and 'string' type by default
	public static void create(final String param, final Object value) {
		create(param, "global", value);
	}
	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		String vnmrTalkPath = getUserDir() + File.separator + ".talk";
		VnmrProxy vnmr = new VnmrProxy(vnmrTalkPath);
		vnmr.open();
	}
}
