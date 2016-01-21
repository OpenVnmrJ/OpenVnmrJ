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
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintStream;

import vnmr.vjclient.VnmrProxy;

/**
 * ProbeIdPipeServer provides an interface between the Probe ID
 * back-end services VnmrJ.  Probe ID information between VnmrJ 
 * and Probe ID is not encrypted, since magical macros at this point
 * cannot handle encrypted messages (which may change at some point).
 * 
 * @author dirk
 *
 */
public abstract class ProbeIdPipeServer extends ProbeIdServer {
    private File m_fifo_in  = null;
    private File m_fifo_out = null;
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		ProbeIdIO probe = new ProbeIdIO();
		String fifoIn = getFifoInPath(null);
		String fifoOut = getFifoOutPath(null);
		probe.start(fifoIn, fifoOut, true);
	}
	
	public ProbeIdPipeServer() {}

	private void openFifoIn() throws FileNotFoundException {
		ProbeIdIO.debug("opening input stream "+m_fifo_in.getPath());
		m_istream  = new FileInputStream(m_fifo_in);
		ProbeIdIO.debug("opening input stream "+m_fifo_in.getPath());
		m_reader   = new InputStreamReader(m_istream);
		ProbeIdIO.debug("opening input stream reader "+m_fifo_in.getPath());
		m_in       = new BufferedReader(m_reader);
	}
	
	private void openFifoOut() throws FileNotFoundException {
		ProbeIdIO.debug("opening output stream "+m_fifo_out.getPath());
		m_ostream  = new FileOutputStream(m_fifo_out);;
		ProbeIdIO.debug("opening output writer "+m_fifo_out.getPath());
		m_writer   = new OutputStreamWriter(m_ostream);
		ProbeIdIO.debug("opening output buffered writer "+m_fifo_out.getPath());
		m_out      = new BufferedWriter(m_writer);
	}
	
	public static String getFifoInPath(String in) {
		return in != null ? in : VnmrProxy.getUserDir() + DEFAULT_FIFO_IN;
	}

	public static String getFifoOutPath(String out) {
		return out != null ? out : VnmrProxy.getUserDir() + DEFAULT_FIFO_OUT;
	}

	public Thread start(String in, String out, boolean server) {
		if (locked()) {
			ProbeIdIO.warning("Another probe server is already running");
			return null;
		}
		m_fifo_out = mkfifo(getFifoOutPath(out), false);
		m_fifo_in  = mkfifo(getFifoInPath(in), false);
		lock();
        ProbeIdIO.info("started with in="+m_fifo_in.getPath()
                +" out="+m_fifo_out.getPath()
                +" lock="+getLockFile().getPath());

		Thread thread = super.start();
		return thread;
	}
	
	public Thread start() { 
	    return start(null, null, false); 
	}
	
	protected void exit() {
		PrintStream printer = new PrintStream(m_ostream);
		printer.println(ProbeId.MSG_SERVER_EXITING);
		printer.close();
		close();
		unlock();
		ProbeIdIO.info(MSG_SERVER_EXITING);
	}

	protected void close() {
		try {
			if (m_out != null) {
				m_out.close();
				m_writer.close();
				m_ostream.close();
			}
			if (m_in != null) {
				m_in.close();
				m_reader.close();
				m_istream.close();
			}
		} catch (IOException e) {
			ProbeIdIO.error("I/O exception while closing I/O channels "
					+e.getMessage());
			e.printStackTrace();
		}
	}

	public void accept() {
		try {
			close();
			open();
		} catch (FileNotFoundException e) {
			ProbeIdIO.error("I/O channel file not found "+e.getMessage());
			e.printStackTrace();
		}
	}
	
	public void openIn() { 
	    try {
            openFifoIn();
        } catch (FileNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } 
	}
	
	public void openOut() { 
	    try {
            openFifoOut();
        } catch (FileNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } 
	}
	
	public void open() throws FileNotFoundException {
		openIn();
		openOut();
	}

	/**
	 * VnmrJ magical scripts are more geared towards writing to
	 * files than they are to sockets, so the server supports
	 * FIFOs (i.e. named pipes).
	 * @param pipe
	 * @param clear - destroys the FIFO before creating it, which
	 *                may be useful when called from unit tests.
	 * @return
	 */
	public static File mkfifo(String pipe, boolean clear) {
		File fifo = new File(pipe);
		if (fifo.exists() && clear)
			fifo.delete();
		if (!fifo.exists()) {
			String mkfifo = "/usr/bin/mkfifo " + fifo.getPath();
			try {
				Runtime.getRuntime().exec(mkfifo).waitFor();
			} catch (IOException e) {
				String reason = mkfifo +" failed: "+ e;
				ProbeIdIO.error(reason);
				System.out.println(reason);
				e.printStackTrace();
			} catch (InterruptedException e) {
				ProbeIdIO.error(" unexpected interrupt while creating I/O channel "
						+ e.getMessage());
				e.printStackTrace();
			}
		}
		return fifo;
	}
}
