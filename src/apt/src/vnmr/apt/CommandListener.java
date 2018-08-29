/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

package vnmr.apt;

import java.net.*;
import java.io.*;

public class CommandListener extends Thread {

    private static final int NO_TIMEOUT = 0;

    private int m_port = 0;
    private Executer m_master;
    private ListenerManager m_manager = null;
    private ServerSocket m_ss = null;
    private Socket m_socket = null;
    private BufferedReader m_reader = null;
    private PrintWriter m_writer = null;
    private boolean m_ok = false;
    private boolean m_connected = false;

    public CommandListener(Socket socket, Executer master) {
        Messages.postDebug("CommandListeners",
                           "CommandListener: called with socket=" + socket);
        m_socket = socket;
        m_ok = socket != null;
        m_master = master;
    }

    /**
     * Create a client socket connected to the given port on the local host.
     * @param master The Executer to send replies to.
     * @param host The host to connect to.
     * @param port The port number to connect to.
     * @param manager Who to notify of connection and disconnection.
     * @throws IOException If an IO error occurs when creating the socket.
     * @throws UnknownHostException If could not get an IP address for the host.
     */
    public CommandListener(Executer master, String host, int port,
                           ListenerManager manager)
            throws UnknownHostException, IOException {
        m_master = master;
        m_port = port;
        m_manager = manager;
        m_socket = new Socket(host, port);
    }

    /**
     * Create a command server socket on a system-selected port.
     * @param master The Executer to send replies to.
     * @param manager Who to notify of connection and disconnection.
     */
    public CommandListener(Executer master,
                           ListenerManager manager) {
        boolean noErrors = true;
        m_master = master;
        m_manager = manager;
        try {
            Messages.postDebug("CommandListeners",
                               "CommandListener: opening server socket");
            m_ss = new ServerSocket(0);
            m_port = m_ss.getLocalPort();
        } catch (IOException ioe) {
            Messages.postError("CommandListener: IO exception 1");
            noErrors = false;
        } catch (SecurityException se) {
            Messages.postError("CommandListener: Security exception");
            noErrors = false;
        }

        //ProbeTune.printlnDebugMessage(5, "CommandListener 2: port=" + m_port);

        m_ok = noErrors;
        //ProbeTune.printlnDebugMessage(5, "CommandListener.<init>: Done");
    }

    public CommandListener(int port, ServerSocket serverSocket, Executer master) {
        m_port = port;
        m_ss = serverSocket;
        m_master = master;
        m_ok = (serverSocket != null);
    }

    public void run() {
        setName(m_master.getClass().getSimpleName() + "."
                + this.getClass().getSimpleName());
        m_ok = m_socket != null;
        if (!m_ok && m_ss != null) {
            try {
                m_ss.setSoTimeout(NO_TIMEOUT);
                m_socket = m_ss.accept();
                m_ok = true;
            } catch (SocketTimeoutException ste) {
                Messages.postError("CommandListener(): \"accept\" timed out");
                m_ok = false;
            } catch (java.nio.channels.IllegalBlockingModeException ibme) {
                Messages.postError("CommandListener: Illegal blocking mode");
                m_ok = false;
            } catch (IOException ioe) {
                Messages.postError("CommandListener: IO exception 2");
                m_ok = false;
            } catch (SecurityException se) {
                Messages.postError("CommandListener: Security exception");
                m_ok = false;
            }
        }

        if (m_ok) {
            try {
                m_writer = new PrintWriter(m_socket.getOutputStream(), true);
                m_reader = new BufferedReader(new InputStreamReader
                                            (m_socket.getInputStream()));
                if (m_manager != null) {
                    m_manager.listenerConnected(this);
                }
                m_connected = true;
            } catch (IOException ioe) {
                Messages.postError("CommandListener: IO exception 3");
                m_ok = false;
            }
        }

        String input;
        try {
            while (m_ok && (input = m_reader.readLine()) != null) {
                m_master.exec(input);
            }
        } catch (IOException ioe) {
            Messages.postError("CommandListener: IO exception 4");
            m_ok = false;
        }
        if (m_manager != null) {
            m_manager.listenerDisconnected(this);
        }
        m_connected = false;
    }

    public void close() {
    	if (m_ss != null) { 
    		try {
        		m_ss.close();
				this.join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
		}
    	}
    }
    
    public boolean isConnected() {
        return m_connected;
    }

    public boolean isOk() {
        return m_ok;
    }

    public int getPort() {
        return m_port;
    }

    public PrintWriter getWriter() {
        if (m_socket == null) {
            Messages.postError("CommandListener.getWriter(): no socket");
            return null;
        }

        if (m_writer == null) {
            try {
                m_writer = new PrintWriter(m_socket.getOutputStream(), true);
            } catch (IOException ioe) {
                Messages.postError("CommandListener: "
                                   + "IO exception getting PrintWriter");
            }
        }
        return m_writer;
    }
}
