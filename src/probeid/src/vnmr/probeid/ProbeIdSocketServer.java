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
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;

import vnmr.vjclient.Messages;

public class ProbeIdSocketServer extends ProbeIdServer {
	private final static int SOCKET_READ_TIMEOUT = 0;
	private ServerSocket       m_ss       = null;
	private Socket             m_socket   = null;

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// run the unit test by default {link #ProbeIdSocketServerTest}
		org.junit.runner.JUnitCore.main("vnmr.apt.ProbeIdSocketServerTest");
		System.exit(0);
	}
	
	public void open() throws IOException {
		m_istream = m_socket.getInputStream();
		m_reader  = new InputStreamReader(m_istream);
        setOutputStream(m_socket.getOutputStream());
        m_writer  = new OutputStreamWriter(getOutputStream());
		m_in      = new BufferedReader(m_reader);
		m_out     = new BufferedWriter(m_writer);		
	}
	
	public void close() throws IOException {
	    m_istream.close();
	    m_reader.close();
	    m_writer.close();
	    m_socket.close();
	    m_in.close();
	    m_out.close();
	}
	public void accept() {
		try {
			m_socket  = m_ss.accept();
			open();
        } catch (SocketTimeoutException ste) {
            Messages.postError("ProbeIdServer: \"accept\" timed out");
            return;
        } catch (java.nio.channels.IllegalBlockingModeException ibme) {
            Messages.postError("ProbeIdServer: Illegal blocking");
            return;
        } catch (IOException ioe) {
            Messages.postError("ProbeIdServer: IO exception starting socket: "
                            + ioe);
            return;
        } catch (SecurityException se) {
            Messages.postError("ProbeIdServer: Security exception");
            return;
        }
        Messages.postDebug("ProbeIdServer is connected");
        
        try {
            m_socket.setSoTimeout(SOCKET_READ_TIMEOUT);
        } catch (SocketException se) {
            Messages.postDebug("ProbeIdServer: Cannot set timeout "
                               + "on ProbeId communication");
        }
    }
	
	protected boolean process(String line) throws IOException {
		ProbeIdDb.Tokenizer toker = new ProbeIdDb.Tokenizer(line);
		String cmd = toker.nextToken().trim();

		if (cmd.equals(CMD_QUIT.trim())) {
			return true;
		} else if (cmd.equals(CMD_READ.trim())) {
			read(toker);
		} else if (cmd.equals(CMD_WRITE.trim())) {
			write(toker);
		} else {
			Messages.postError("ProbeIdServer: unkown command \"" + toker + "\"");
			// TODO handle error condition unknown command
		}
		return false;
	}
	
	public ProbeIdSocketServer(int port) {
		this();
		try {
			Messages.postDebug("ProbeIdServer: opening server socket on port "
								+ port);
			m_ss = new ServerSocket(port);
		} catch (IOException ioe) {
			Messages.postError("ProbeIdServer: IO exception getting socket: "
					           + ioe);
			Messages.postError("socket may be in use - try identifying  the "+
					" interfering process with \"netstat -n -a -o --program\"");
			return;
		} catch (SecurityException se) {
			Messages.postError("ProbeIdServer: Security exception: " + se);
			return;
		}
		assert(m_ss != null);
	}
	
	public ProbeIdSocketServer() { super(); }

    @Override
    void openIn() {
        // TODO Auto-generated method stub
        FileNotFoundException e = new FileNotFoundException("not implemented yet");
        ProbeIdIO.error(" init I/O error "+e.getMessage());
        e.printStackTrace();        
    }

    @Override
    void openOut() {
        // TODO Auto-generated method stub
        FileNotFoundException e = new FileNotFoundException("not implemented yet");
        ProbeIdIO.error(" init I/O error "+e.getMessage());
        e.printStackTrace();
    }
}
