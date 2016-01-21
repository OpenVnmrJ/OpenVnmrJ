/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.event.*;
import java.io.*;
import java.net.*;
import java.util.*;


public class SerialSocketDeviceIO {

    private final static int SOCKET_TIMEOUT = 1000; // ms

    private ButtonIF m_vnmrIf;
    private String m_hostName;
    private int m_portNumber = 0;
    private Socket m_socket = null;
    private SocketReader m_reader = null;
    private BufferedReader m_inputStream = null;
    private PrintWriter m_outputStream = null;
    private boolean m_connectionFailed = false;
    private boolean m_gaveConnectionWarning = false;

    private String m_sVnmrMacro = null;
    private String m_sVnmrPrefix = null;
    private String m_sVnmrSuffix = null;
    private int m_iValueIndex = -1;

    /**
     * Create an instance to communicate with a given device.
     * @param vnmrIf For the standard communication with Vnmr.
     * @param hostname The name of the device (should be in /etc/hosts).
     */
    public SerialSocketDeviceIO(ButtonIF vnmrIf, String hostname) {
        m_vnmrIf = vnmrIf;
        m_hostName = hostname;
    }

    /**
     * Execute a command specified by a list of tokens.
     * The first token is the command.  Further tokens are command
     * dependent arguments.
     */
    public void exec(StringTokenizer toker) {
        if (!toker.hasMoreTokens()) {
            Messages.postDebug("Null command for device \""
                               + m_hostName + "\"");
            return;
        }
        String cmd = toker.nextToken();
        if (cmd.equals("open")) {
            // Set new port number if they've specified one here
            if (toker.hasMoreTokens()) {
                String sPort = toker.nextToken();
                try {
                    m_portNumber = Integer.parseInt(sPort);
                } catch (NumberFormatException nfe) {
                    Messages.postDebug("SerialSocketDeviceIO.exec open: "
                                       + "invalid port number: " + sPort);
                }
            }
            Messages.postDebug("socketIO",
                               "OPEN port " + m_portNumber + " on "
                               + m_hostName);
            openPort(false);
        } else if (cmd.equals("send")) {
            String msg = "";
            try {
                msg = toker.nextToken("").trim();
            } catch (NoSuchElementException nsee) {
                Messages.postDebug("SerialSocketDeviceIO.exec send: "
                                   + "No message specified");
                return;
            }
            println(msg);
        } else if (cmd.equals("macro")) {
            try {
                m_sVnmrMacro = toker.nextToken("").trim();
            } catch (NoSuchElementException nsee) {
                Messages.postDebug("SerialSocketDeviceIO.exec macro: "
                                   + "No macro specified");
                m_sVnmrMacro = null;
            }
            if (m_sVnmrMacro.length() == 0) {
                m_sVnmrMacro = null;
            }
            if (m_sVnmrMacro != null) {
                m_iValueIndex = m_sVnmrMacro.indexOf("$VALUE");
                if (m_iValueIndex >= 0) {
                    m_sVnmrPrefix = m_sVnmrMacro.substring(0, m_iValueIndex);
                    m_sVnmrSuffix = m_sVnmrMacro.substring(m_iValueIndex + 6);
                }
            }
        } else if (cmd.equals("close")) {
            closePort();
        } else {
            Messages.postDebug("SerialSocketDeviceIO.exec: "
                               + "Unknown command: \"" + cmd + "\"");
        }
    }

    private void println(String msg) {
        Messages.postDebug("socketIO","SerialSocketDeviceIO.println("
                           + msg + ")");
        if (m_outputStream == null) {
            Messages.postDebug("Cannot send message - no socket connection");
        } else  {
            m_outputStream.println(msg);
        }
    }

    /**
     * Open a socket to communicate with the Ethernet device, or
     * just make sure that it is (thought to be) open.
     * @param ifCheck If true, only open port if it is currently closed.
     */
    public boolean openPort(boolean ifCheck) {
        m_gaveConnectionWarning = false;
        if (ifCheck && isPortOpen()) {
            return true;        // Port is already open
        } else if (isPortOpen()) {
            closePort();
        }

        // Try to open the port
        m_connectionFailed = false;
        Messages.postDebug("socketIO", "SerialSocketDeviceIO.openPort(): "
                                       + "hostName="
                           + m_hostName);
        try {
            Messages.postDebug("socketIO",
                               "SerialSocketDeviceIO.openPort(): "
                                       + "creating socket ...");
            m_socket = new Socket();
            SocketAddress socketAddr
                    = new InetSocketAddress(m_hostName, m_portNumber);
            m_socket.connect(socketAddr, SOCKET_TIMEOUT);
            Messages.postDebug("socketIO",
                               "SerialSocketDeviceIO.openPort(): "
                                       + "... got socket");
            try {
                m_socket.setSoTimeout(10000);
            } catch (SocketException se) {
                Messages.postDebug("Cannot set timeout on socket");
            }
            m_outputStream = new PrintWriter(m_socket.getOutputStream(), true);
            m_inputStream = new BufferedReader
                    (new InputStreamReader(m_socket.getInputStream()));
            m_reader = new SocketReader(); // Uses m_inputStream
            m_reader.start();
        } catch (UnknownHostException uhe) {
            Messages.postError("Don't know about host: " + m_hostName);
        } catch (IOException ioe) {
            Messages.postError("Couldn't get socket for "
                               + "the connection to: " + m_hostName);
        }
        Messages.postDebug("socketIO",
                           "openPort(): m_socket = " + m_socket);


        boolean ok = (m_socket != null);
        if (ifCheck && !ok) {
            m_connectionFailed = true;
        }
        return ok;
    }

    public void closePort() {
        if (m_socket != null) {
            try {
                m_socket.close();
            } catch (IOException ioe) {}
            m_socket = null;
            m_inputStream = null;
            m_outputStream = null;
            m_connectionFailed = false;
            //m_reader.quit(); // Will quit anyway as soon as socket goes away
            //m_reader = null;
        }
        Messages.postDebug("socketIO", "m_socket closed");
    }

    public boolean isPortOpen() {
        return (m_socket != null);
    }

    public boolean isPortCheckBad() {
        return m_connectionFailed;
    }

    synchronized public void readMessage(String strMsg) {
        Messages.postDebug("socketIO", "Message from " + m_hostName
                           + ": time=" + System.currentTimeMillis()
                           + " \"" + strMsg + "\"");
        // If requested, send message to VBG
        if (m_sVnmrMacro != null) {
            String str;
            if (m_iValueIndex >= 0) {
                str = m_sVnmrPrefix + strMsg + m_sVnmrSuffix;
            } else {
                str = m_sVnmrMacro + "('" + strMsg + "')";
            }
            m_vnmrIf.sendToVnmr(str);
        }
    }

    public void sendCommand(String cmd) {
        println(cmd);
    }

    /**
     * Close the port when we are scooped up by the garbage collector.
     */
    protected void finalize() throws Throwable {
        Messages.postDebug("SerialSocketDeviceIO.finalize: closing port");
        closePort();
        super.finalize();
    }


    /**
     * This class listens for messages from the socket port.
     */
    class SocketReader extends Thread {
        private boolean m_quit = false;

        public SocketReader() {
        }

        public void quit() {
            m_quit = true;
        }

        public synchronized void run() {
            String msg = "";
            while (!m_quit && msg != null) {
                try {
                    msg = m_inputStream.readLine();
                    readMessage(msg);
                } catch (SocketTimeoutException stoe) {
                    Messages.postDebug("socketIO++",
                                       "SerialSocketDeviceIO.SocketReader: "
                                       + "Read timed out");
                } catch (IOException ioe) {
                    Messages.postDebug("SerialSocketDeviceIO.SocketReader: "
                                       + "Read failed");
                    m_quit = true;
                }
            }
            Messages.postDebug("socketIO", "Reader thread exiting");
        }
    }
}
