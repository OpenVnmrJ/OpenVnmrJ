/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

import java.io.*;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import vnmr.vjclient.Messages;

public class ProbeIdClient implements ProbeId, ProbeIdIOCommands {
	BufferedReader   m_reader    = null;
	BufferedWriter   m_writer    = null;
	ProbeIdFileLock  m_lock      = null;
	OutputStream     m_ostream   = null;
	InputStream      m_istream   = null;
	String           m_etherName = ProbeId.DEFAULT_NIC; 
	File             m_fifo_in   = null;
	File             m_fifo_out  = null;
	int              m_port      = -1;
	
	public static boolean io_synch = false; // for unit test support
	
	// This intervening call interface insulates clients (i.e. ProTune)
	// from future changes to the way probe database files are
	// implemented, hopefully allowing us to insert a bona fide
	// database where close() commits the changes.
	public PrintWriter getPrintWriter(String file, String subdir, boolean sysdir, boolean usrdir) {
		try {
			File blob = blobWrite(file, subdir, sysdir, usrdir);
			if (blob != null)
				return new PrintWriter(blob);
		} catch (FileNotFoundException e) {
			Messages.postDebug("ProbeId", "Invalid tuning file \""
					+ subdir + File.separator + file + "\"");
			e.printStackTrace();
		}
		return null;
	}
	
	private void command(String action, String key) throws IOException {
		String cmd = action + key;
		command(cmd);
	}
	
	private void command(String msg) throws IOException {
		m_writer.write(msg);
		m_writer.newLine();
		m_writer.flush();		
	}
	
	public String read() throws IOException {
		if (io_synch) try {
			ProbeIdIO.io_lock.acquire();
		} catch (InterruptedException e) {
			System.err.println("probe client: io lock error "+e);
			e.printStackTrace();
		}
		String line = m_reader.readLine();
		closeReader();
		return line;
	}
	
	public static String getCommand(String[] args) {
		String cmd = "";
		for (String arg : args)
			cmd = cmd + " " + arg;
		return cmd.trim();
	}
	
	public int read(byte [] result, String key) {
		open();
		try {
			command(CMD_READ, key);
			return m_istream.read(result, 0, result.length);			
		} catch (IOException e) {
			error("read: socket I/O error");
			e.printStackTrace();
			return 0;
		}
	}

	public void write(String data) {
		try {
			openWriter();
			command(data);
			m_writer.flush();
		} catch (FileNotFoundException e) {
			error("write: socket file not found "+e.getMessage());
			e.printStackTrace();
		} catch (IOException e) {
			error("write: socket I/O error "+e.getMessage());
			e.printStackTrace();
		} finally {
			closeWriter();
		}
	}
	
	public void write(byte [] data, String key) {
		open();
		try {
			command(CMD_WRITE, key); // get ready for OutputStreamWriter to take over
			m_ostream.write(data, 0, data.length);
			m_ostream.close(); 		 // generate an EOF at the far end
			m_writer.close();  		 // redundant?
		} catch (IOException e) {
			error("socket I/O error");
			e.printStackTrace();
		}
	}
	
	private void openSocket(InetAddress addr, int port) throws UnknownHostException, IOException {
		Socket socket = new Socket(addr, port);
		m_ostream = socket.getOutputStream();
		m_istream = socket.getInputStream();
		m_reader  = new BufferedReader(new InputStreamReader(m_istream));
		m_writer  = new BufferedWriter(new OutputStreamWriter(m_ostream));
	}

	private void openWriter() throws FileNotFoundException {
		m_ostream = new FileOutputStream(m_fifo_out);
		m_writer  = new BufferedWriter(new OutputStreamWriter(m_ostream));
	}
	
	private void closeWriter() {
		try {
			m_writer.close();
			m_ostream.close();
		} catch (IOException e) {
			error("probe client: I/O error "+e);
			e.printStackTrace();
		}
	}
	
	private void closeReader() {
		try {
			m_reader.close();
			m_istream.close();
		} catch (IOException e) {
			error("probe client: I/O error "+e);
			e.printStackTrace();
		}
	}
	/**
	 * TODO: separate ProbeIdSocketClient and ProbeIdFifoClient
	 * with an independent ProbeIdClient Interface, if the "if"
	 * is esthetically too displeasing in the long run.
	 */
	private void open() {
		InetAddress addr = null;
		try {
			addr = InetAddress.getLocalHost();
			openSocket(addr, m_port);
		} catch (UnknownHostException e) {
			ProbeIdIO.error("unknown host \"localhost\"");
			e.printStackTrace();
		} catch (IOException e) {
			ProbeIdIO.error(e.getMessage());
			e.printStackTrace();
		}
	}
	
	private void check(String probeMnt) {
		if ( !probeMnt.isEmpty() ) {
			String probeMntPt = probeMnt + "/";
			if ( System.getProperty("InitializeProbeId").equalsIgnoreCase("true") )
				ProbeIdDb.init(probeMntPt, "0000-00000 0000-000010", null);
		}
 	}

	public String find(String[] alternates) {
		ProbeIdIO.notYetImplemented("ProbeIdClient find method not implemented yet"); 
		return null;
	}
	
	public InputStream getBufferedReader(String path) throws IOException {
		String probeMntPt = System.getProperty(KEY_PROBEID_MOUNTPOINT);
		check(probeMntPt);		
		command(CMD_READ, path);
		return m_istream;
	}
	
	public OutputStream getBufferedWriter(String path) throws IOException {
		String probeMntPt = System.getProperty(KEY_PROBEID_MOUNTPOINT);
		check(probeMntPt);
		command(CMD_WRITE, path);
		return m_ostream;
	}

	// TODO: make sure that read/write/quit don't trounce BufferedReader/Writer
	public void quit() {
		open();
		try {
			m_writer.write(CMD_QUIT);
			m_writer.newLine();
		} catch (IOException e) {
			error("quit: I/O error "+e.getMessage());
			e.printStackTrace();
		} finally {
			try {
				m_writer.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	private File exportLink(String[] cmd) {
		File result = null;
		String command = getCommand(cmd);
		ProbeIdIO.info("probe client: "+command);
		String tmpname = m_lock.run(command).trim();
		assert(tmpname != null);
		if (tmpname != null && !tmpname.trim().isEmpty()) {
			result = new File(tmpname);
		} else {
			ProbeIdIO.info("probe client: "+getCommand(cmd)+" returned \""+tmpname+"\"");
		}
		return result;
	}
	
	/**
	 * Export a writable handle to a BLOB.  Doesn't create the BLOB if it doesn't
	 * already exist.
	 * @param file - the root of the BLOB subdirectory
	 * @param dir - the location of the BLOB subdirectory w/in VNMR filesystem
	 * @param system - indicates whether it is a system file
	 * @return - a handle to the BLOB
	 */
	public File blobLink(String file, String dir, boolean system, boolean user) {
		String blob_link_args[] = {
				CMD_BLOB+"link:r+", file, 
				(dir == null ? "" : CMD_OPTION), (dir == null ? "" : dir), 
				(system ? CMD_SYSTEM : ""),
				(user ? CMD_USER: ""),
		};
		return exportLink(blob_link_args);
	}
	
	/**
	 * Create a writable handle to a BLOB, creating a place-holder for the BLOB
	 * if it doesn't already exist.  This comes about because of the use case
	 * where the original data is taken from the system area but the calibrated
	 * results are written back to the user area, which may or may not exist yet.
	 * @param file is the root of the BLOB subdirectory
	 * @param dir is the location of the BLOB subdirectory w/in VNMR filesystem
	 * @param system indicates whether it is a system file
	 * @return a handle to the BLOB
	 */
	public File blobWrite(String file, String dir, boolean system, boolean user) {
		String blob_link_args[] = {
				CMD_BLOB+"link:a+", file, 
				(dir == null ? "" : CMD_OPTION), (dir == null ? "" : dir), 
				(system ? CMD_SYSTEM : ""),
				(user ? CMD_USER: ""),
		};
		File link = exportLink(blob_link_args);
		if (link == null)
			Messages.postWarning("Probe server could not create directory for "
					+ dir + File.separator+file);
		return link;
	}
	
	public String getProbeId() {
		String[] probe_id_args = { CMD_PROBE_ID };
		String command = getCommand(probe_id_args);
		String result = m_lock.run(command).trim();
		return result;
	}
	
	// Get Probe ID from socket interface
	public String getProbeId(String probeMntPt) {
		String key = ProbeIdDb.getProbeIdKey();
		byte [] probeIdBuf = new byte [MAX_BUF_SIZE];
		open();
		int bytes_read = read(probeIdBuf, key);
		if ( bytes_read < 1 ) {
			Messages.postError("ProbeIdClient: invalid ProbeId");
		}
		return probeIdBuf.toString().trim();
	}
	
	void error(String msg) { ProbeIdIO.error(msg); }
	
	public ProbeIdClient() throws FileNotFoundException {
		m_fifo_in  = ProbeIdPipeServer.mkfifo(DEFAULT_FIFO_OUT, false);
		m_fifo_out = ProbeIdPipeServer.mkfifo(DEFAULT_FIFO_IN, false);
 		m_lock     = new ProbeIdFileLock();
	}
	
	ProbeIdClient(int port) {
		m_port = port;
	}
}
