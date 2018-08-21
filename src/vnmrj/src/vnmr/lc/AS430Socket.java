/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;


import java.io.*;
import java.net.*;
import java.util.*;

import vnmr.util.Messages;

public class AS430Socket{

    private final static int SOCKET_TIMEOUT = 0; // milliseconds
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
    
    
    private InputStream m_instream;
    private OutputStream m_outstream;
    private int newByte;

    public AS430Socket(String host, int port) {
        if (host != null) {
            connect(host, port);
        }
    }
    
    /**
     * Keeps trying to make connection.
     * @param host hostname
     * @param port port number 
     */
    public void connect(String host, int port) {
        makeConnection(host, port);
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
            Messages.postError("AS430Socket: makeConnection given null host");
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
                    Messages.postError("AS430Socket: Don't know about host: "
                                       + host);
                    m_connectionErrorType = CONNECTION_NOHOST;
                }
                return false;
            } catch (IOException ioe) {
                if (m_connectionErrorType != CONNECTION_TIMEOUT) {
                    Messages.postError("AS430Socket: no connection to: "
                                       + host + ":" + port
                                       + " ... " + ioe);
                    m_connectionErrorType = CONNECTION_TIMEOUT;
                }
                return false;
            }
            try{
                m_instream = m_connection.getInputStream();
                m_outstream = m_connection.getOutputStream(); 
            } catch (Exception e){
                if (m_connectionErrorType != CONNECTION_BADSTREAM) {
                    Messages.postError("AS430Socket: Error getting IO Stream");
                    m_connectionErrorType = CONNECTION_BADSTREAM;
                }
                return false;
            }
            if (m_connectionErrorType != CONNECTION_OK) {
                Messages.postInfo("AS430Socket: Connection OK");
                m_connectionErrorType = CONNECTION_OK;
            }
            Messages.postDebug("430Comm", "Made AS430 connection");     
            return true;
        } else {
            //SDM DEBUG System.out.println("Connection != null");
            return false;
        } 
    }

    public boolean isConnected(){
        if(m_connection != null){
            return m_connection.isConnected();
        } else return false;
    }

    public void disconnect(){
        boolean wasConnected = isConnected();
        Messages.postDebug("430Comm", "AS430Socket.disconnect(): connected="
                           + wasConnected);
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
            Messages.postDebug("IO exception disonnecting from AS430");
        }
        if (wasConnected) {
            Messages.postWarning("AS 430 disconnected");
        }
    }

    /**
     * Read a byte from the input stream.
     * Blocks until it is available.
     */
    public int read() throws IOException {
        if (m_instream == null) {
            if (m_readErrorType != READ_WAIT) {
                Messages.postDebug("AS430Socket.read Waiting for connection");
                m_readErrorType = READ_WAIT;
            }
            try {
                Thread.sleep(100); // Wait for connection
            } catch (InterruptedException ie) {
            }
            return 0;
        }
        if (m_readErrorType == READ_WAIT) {
            Messages.postDebug("AS430Socket.read Got connection");
            m_readErrorType = READ_OK;
        }
        return m_instream.read();
    }

    /**
     * Send an array of bytes to the socket.
     **/
    public boolean write(byte[] buf) {
        try{
            m_outstream.write(buf);
            return true;       
        } catch (IOException ioe) {
            System.out.println("Error writing message to autosampler");
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
         * Constructor for getting a connection to the AS 430.
         * @param seconds How long to wait between retrys. 
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
                    Messages.postDebug("430Comm", "AS430 connection failed");
                }
            }
        }

    }
}
