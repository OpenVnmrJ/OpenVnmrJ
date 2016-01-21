/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 */
package vnmr.cryo;


import java.io.*;
import java.net.*;
import java.util.*;


public class CryoSocket{

    private final static int SOCKET_TIMEOUT = 2000; // milliseconds
    private final static int CONNECTION_OK = 0;
    private final static int CONNECTION_NOHOST = 1;
    private final static int CONNECTION_TIMEOUT = 2;
    private final static int CONNECTION_BADSTREAM = 3;
    private final static int READ_OK = 0;
    private final static int READ_WAIT = 1;

    private int m_connectionErrorType = CONNECTION_OK;
    private int m_readErrorType = READ_OK;
    private Socket m_connection = null;
    private TimeOut m_connectTO = null;
    private MessageIF m_cryoMsg;
    private String m_host;
    private int m_port;
    private InputStream m_instream;
    private OutputStream m_outstream;

    public CryoSocket(String host, int port, MessageIF cryoMsg) {
        m_host = host;
        m_port = port;
        m_cryoMsg = cryoMsg;
        if (host != null) {
            connect();
        }
    }

    /**
     * Keeps trying to make connection.
     */
    public void connect() {
        makeConnection(m_host, m_port);
        /*
        if (m_connectTO != null) {
            m_connectTO.cancel();
            m_connectTO = null;
        }
        try{
            m_connectTO = new TimeOut(10, host, port);
        } catch (Exception e){
            return;
        }
        */
    }

    public boolean makeConnection(String host, int port){
        if (host == null) {
            m_cryoMsg.postError("CryoSocket: makeConnection given null host");
            return false;
        }
        if(!isConnected()) {
            //remoteHost = host;
            //port_num = port;

            SocketAddress socketAddr;
            socketAddr = new InetSocketAddress(host, port);
            m_connection = new Socket();
            try{
                m_connection.connect(socketAddr, SOCKET_TIMEOUT);
            } catch (UnknownHostException e){
                if (m_connectionErrorType != CONNECTION_NOHOST) {
                    m_cryoMsg.postError("CryoSocket: Don't know about host: "
                                       + host);
                    m_connectionErrorType = CONNECTION_NOHOST;
                }
                return false;
            } catch (IOException ioe) {
                if (m_connectionErrorType != CONNECTION_TIMEOUT) {
                    m_cryoMsg.postError("CryoSocket: no connection to: "
                                       + host + ":" + port
                                       + " ; " + ioe);
                    m_connectionErrorType = CONNECTION_TIMEOUT;
                }
                return false;
            }
            try{
                m_instream = m_connection.getInputStream();
                m_outstream = m_connection.getOutputStream(); 
            } catch (Exception e){
                if (m_connectionErrorType != CONNECTION_BADSTREAM) {
                    m_cryoMsg.postError("CryoSocket: Error getting IO Stream");
                    m_connectionErrorType = CONNECTION_BADSTREAM;
                }
                return false;
            }
            if (m_connectionErrorType != CONNECTION_OK) {
                m_cryoMsg.postInfo("CryoSocket: Connection OK");
                m_connectionErrorType = CONNECTION_OK;
            }
            m_cryoMsg.postDebug("cryocmd", "Made CryoBay connection");
            m_cryoMsg.postWarning("CryoBay port " + m_port + " connected");
            return true;
        } else {
            m_cryoMsg.postWarning("Connection != null");
            return false;
        } 
    }

    public boolean isConnected(){
        if(m_connection != null){
            return m_connection.isConnected();
        } else {
            return false;
        }
    }

    public void disconnect(){
        boolean wasConnected = isConnected();
        m_cryoMsg.postDebug("cryoinit", "CryoSocket.disconnect(): "
                           + "wasConnected=" + wasConnected);
        if (m_connectTO != null) {
            m_connectTO.cancel();
            m_connectTO = null;
        }
        try{
            if (m_connection != null) {
                m_connection.close();
            }
            if (m_instream != null) {
                m_instream.close();
            }
            if (m_outstream != null) {
                m_outstream.close();
            }
            m_connection = null;
        } catch (IOException ioe){
            m_cryoMsg.postDebug("IO exception disonnecting from CryoBay");
        }
        if (wasConnected) {
            m_cryoMsg.postWarning("CryoBay port " + m_port + " disconnected");
        }
    }

    /**
     * Read a byte from the input stream.
     * Blocks until it is available.
     * @return The byte of data, or -1 if the end of the stream is reached,
     * or 0 if there is no socket connection.
     * @throws IOException If there is an I/O Exception.
     */
    public int read() throws IOException {
        if (m_instream == null) {
            if (m_readErrorType != READ_WAIT) {
                m_cryoMsg.postDebug("CryoSocket.read Waiting for connection");
                m_readErrorType = READ_WAIT;
            }
            try {
                Thread.sleep(100); // Wait for connection
            } catch (InterruptedException ie) {
            }
            return 0;
        }
        if (m_readErrorType == READ_WAIT) {
            m_cryoMsg.postDebug("CryoSocket.read Got connection");
            m_readErrorType = READ_OK;
        }
        return m_instream.read();
    }

    /**
     * Send an array of bytes to the socket.
     * @param buf The array of bytes.
     * @return True if successful.
     **/
    public boolean write(byte[] buf) {
        try{
            m_outstream.write(buf);
            return true;       
        } catch (IOException ioe) {
            m_cryoMsg.postError("Writing message to cryobay failed");
            return false;
        } 
    }

    /**
     * TimeOut class for waiting for bytes and connection.
     */
    public class TimeOut {
        public boolean done;

        private Timer mm_timer;
        private String mm_host;
        private int mm_port;

        /**
         * Constructor for getting a byte from the socket.
         * @param seconds How long to wait between retrys. 
         */
        public TimeOut(int seconds) {
            done= false;
            mm_timer = new Timer();
            mm_timer.schedule(new CheckTO(), 0, seconds * 1000);
        }

        /**
         * Constructor for getting a connection to the CryoBay.
         * @param seconds How long to wait between retrys.
         * @param host The host name or IP of the Cryobay.
         * @param port The port number to use.
         */
        public TimeOut(int seconds, String host, int port) {
            done= false;
            mm_timer = new Timer();
            mm_timer.schedule(new CheckConnection(), 0, seconds * 1000);
            mm_host= host;
            mm_port= port;
        }

        public void cancel(){
            mm_timer.cancel();
            done= true;
        }

        class CheckTO extends TimerTask {
            public void run() {
                mm_timer.cancel();
                done= true;
            }
        }

        class CheckConnection extends TimerTask {
            public void run() {
                if(makeConnection(mm_host, mm_port)){
                    mm_timer.cancel(); 
                } else {
                    m_cryoMsg.postDebug("cryocmd", "CryoBay connection failed");
                }
            }
        }

    }
}
