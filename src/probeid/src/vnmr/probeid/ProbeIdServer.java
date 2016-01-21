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
import java.net.SocketTimeoutException;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import vnmr.vjclient.VnmrProxy;

/**
 * This class simulates a ProbeId server.
 *   	
 *   The specified probe Id file, if it exists, may be cached from
 *   the Probe ID file system to a local file.  The client (VnmrJ, VW)
 *   then works on the local file, which is copied back when it is 
 *   closed.
 *   
 *   The current prototype system currently does not cache the files.
 *   
 *   This simple approach minimizes network traffic, since some 
 *   Probe ID files are written to frequently, and makes for ready
 *   backwards compatibility with the VnmrJ and eventually VW.
 *
 *   The data will typically be encrypted, unless the files are local
 *   files.
 */
public abstract class ProbeIdServer implements ProbeId, Runnable {
 	protected BufferedReader     m_in       = null;
	protected BufferedWriter     m_out      = null;
	protected InputStream        m_istream  = null;
	protected InputStreamReader  m_reader   = null;
	protected OutputStreamWriter m_writer   = null; // i.e. stdout
	protected OutputStream       m_ostream  = null; 	
	private Thread               m_thread   = null;
	private Semaphore            m_notifier = null;
	
	ProbeIdServer() { 
		m_notifier = new Semaphore(1);
	}
	
	public Thread start() {
		if (m_thread == null) {
			m_thread = new Thread(this);
			m_thread.start();
		}
		return m_thread;
	}
	
	protected void exit(int status) {
		try {
			m_in.close();
			m_reader.close();
			m_istream.close();
			m_out.close();
			m_ostream.close();
			m_writer.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} finally {
			VnmrProxy.sendNotification("shutdown");
			Runtime.getRuntime().exit(status);
		}
	}

	static public boolean status(int pid) {
		String[] cmd = {"ps", "-p", Integer.toString(pid), "h", "o", "comm"};
		try {
			Process p = Runtime.getRuntime().exec(cmd);
			BufferedReader r = new BufferedReader(new InputStreamReader(p.getInputStream()));
			String status= r.readLine();
			if (status != null && status.length() > 0)
				return true;
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return false;
	}
	
	/**
	 * get a process ID on a POSIX-compliant system.
	 * @return process ID of this process
	 */
	public static int getPid() {
		String [] cmd =	{"bash", "-c", "echo $PPID"};
		Process p;
		int pid = -1;
		try {
			p = Runtime.getRuntime().exec(cmd);
			BufferedReader r = new BufferedReader(new InputStreamReader(p.getInputStream()));
			pid = Integer.valueOf(r.readLine());
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return pid;
	}
	
	static public File getLockFile() {
		String pidPath = VnmrProxy.getUserDir() + DEFAULT_SERVER_PIDFILE;
		return new File(pidPath); 
	}
	
	protected void killLockerOrDie() {
	    Integer lockPid = getLockPid();
	    int myPid = getPid();
	    // 
	    if (lockPid == myPid) { // kill the other guy
	        try {
	            close();
	        } catch (IOException e1) {
	            e1.printStackTrace();
	        }
	        String quit = "probeid -quit -nolock -log /tmp/quit.log";
	        try {
                Process shell = Runtime.getRuntime().exec(quit);
                shell.waitFor();
            } catch (IOException e) {
                ProbeIdIO.error("probeid: could not send quit to zombie probe server "
                                +Integer.toString(lockPid)+" giving up");
                Runtime.getRuntime().exit(1);
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            ProbeIdIO.warning("probeid: sent termination request to zombie probe server ");
	    } else if (status(lockPid)){ // this process is the interloper
	        exit(1);
	    }
	}
	
	static protected boolean locked() {
	    Integer pid = getLockPid();
        if (pid != null) {
            int myPid = getPid();
            if (pid != myPid) {
                if (status(pid))
                    System.err.println("Probe Server: "+getLockFile().getPath()+" already locked by process "+pid.toString());
                else
                    return false;
            }
            return true;
        }
        return false;
	}
	
	static protected Integer getLockPid() {
		File lock = getLockFile();
		if (lock.exists()) {
			try {
				BufferedReader reader = new BufferedReader(new InputStreamReader(new FileInputStream(lock)));
				String line = null;
				String last = null;
				while ((line = reader.readLine()) != null)
					last = line;
				return last != null ? Integer.valueOf(last) : null;
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		return null;
	}
	
	static protected void lock() {
		File lock = getLockFile();
		PrintWriter writer;
		try {
			writer = new PrintWriter(new FileWriter(lock));
			writer.println(Integer.toString(getPid()));
			writer.close();
		} catch (IOException e) {
			ProbeIdIO.error("Probe ID server: couldn't lock file "+getLockFile().getPath());
			e.printStackTrace();
		}
	}
	
	static protected void unlock() {
		File lock = getLockFile();
		if (lock.exists()) 
			lock.delete();
	}
	
	static File getFifoIn() {
		return new File(VnmrProxy.getUserDir()+DEFAULT_FIFO_IN);
	}
	//        pipe or socket server.
	static protected void quit() {
		try {
			File fifoIn     = getFifoIn();
			PrintWriter out = new PrintWriter(fifoIn);
			out.println("-quit");
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public Thread getThread() { return m_thread; }
	
	public PrintStream getOutputStream() { 
		return m_ostream == null ? System.out : new PrintStream(m_ostream);
	}
	
	public void setOutputStream(OutputStream ostream) {
		m_ostream = ostream;
	}
	
	abstract void accept(); // accept an incoming connection
	abstract void close() throws IOException;  // close an incoming connection
	abstract void openIn();
	abstract void openOut();
	
	protected void write(ProbeIdDb.Tokenizer tok) throws IOException {
		BufferedReader in = m_in;
		BufferedWriter out = new BufferedWriter(new OutputStreamWriter(ProbeIdDb.writer(tok)));
		ProbeIdDb.copy(in, out);
	}
	
	protected void read(ProbeIdDb.Tokenizer tok) throws IOException {
		BufferedReader in = new BufferedReader(new InputStreamReader(ProbeIdDb.reader(tok)));
		BufferedWriter out = m_out;
		ProbeIdDb.copy(in, out);
	}
	
	protected void write(String line) {
		try {
			m_writer.write(line);
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	protected abstract boolean process(String line) throws IOException;
	
	public void waitForCompletion(long timeout) throws InterruptedException { 
		m_notifier.tryAcquire(timeout, TimeUnit.MILLISECONDS); 
	}
	
	public void run() {
		boolean quit = false;
		this.getThread().setName("ProbeServer");
		ProbeIdIO.debug("ProbeIdServer.ConsoleReader running");
		while (!quit) {
			accept();                // one atomic operation per command
			String line = "";
			try {
				line = m_in.readLine();
				if (line == null) {
					ProbeIdIO.warning("probeid: null request received - is there another probe server instance running for "
					        + ProbeIdDb.getUserId() + " on this server?");
					killLockerOrDie();
				} else {
					ProbeIdIO.info("probeid: got command: \"" + line + "\"");
					quit = process(line);
					m_notifier.release(); // supports unit test
				}
			} catch (SocketTimeoutException stoe) {
                ProbeIdIO.error("probeid: Socket read timeout");
                quit = true;
            } catch (IOException ioe) {
                ProbeIdIO.error("ProbeIdServer: Socket read failed: " + ioe);
                quit = true;
            }
 		}
		unlock();
		exit(0);
	}
}
