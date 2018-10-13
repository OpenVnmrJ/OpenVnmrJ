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
package vnmr.cryomon;


import java.io.*;
import java.net.*;
import java.util.*;

public class CryoMonitorSocket{

    private final static int SOCKET_TIMEOUT = 0;//initial connection max timeout (milliseconds) 0=wait forever
    private final static int CONNECTION_OK = 0;
    private final static int CONNECTION_NOHOST = 1;
    private final static int CONNECTION_TIMEOUT = 2;
    private final static int CONNECTION_BADSTREAM = 3;

    private int m_connectionErrorType = CONNECTION_OK;
    private Socket m_connection = null;
     
    private InputStream m_instream=null;
    private OutputStream m_outstream=null;
    
    public static boolean debug=false;

    public CryoMonitorSocket(String host, int port) {
        if (host != null) {
            connect(host, port);
        }
    }
    
    /**
     * Make a socket connection.
     * @param host hostname
     * @param port port number 
     */
    public boolean connect(String host, int port) {
        if(!isConnected()) {
            SocketAddress socketAddr;
            socketAddr = new InetSocketAddress(host, port);
            m_connection = new Socket();
            try{
                m_connection.connect(socketAddr, SOCKET_TIMEOUT);
            } catch (UnknownHostException e){
                if (m_connectionErrorType != CONNECTION_NOHOST) {
                	 System.out.println("CryoSocket: Don't know about host: "
                                       + host);
                    m_connectionErrorType = CONNECTION_NOHOST;
                }
                return false;
            } catch (IOException ioe) {
                if (m_connectionErrorType != CONNECTION_TIMEOUT) {
                    System.out.println("CryoSocket: no connection to: "
                                       + host + ":" + port
                                       + " ... " + ioe);
                    m_connectionErrorType = CONNECTION_TIMEOUT;
                }
                return false;
            }
            catch (Exception e){
            	DataFileManager.writeMsgLog("CryoSocket connection failed", debug);   	            
                return false;
            }
            try{
                m_instream = m_connection.getInputStream();
                m_outstream = m_connection.getOutputStream(); 
            } catch (Exception e){
                if (m_connectionErrorType != CONNECTION_BADSTREAM) {
                	System.out.println("CryoSocket: Error getting IO Stream");
                    m_connectionErrorType = CONNECTION_BADSTREAM;
                }
                return false;
            }
            if (m_connectionErrorType != CONNECTION_OK) {
                //Messages.postInfo("CryoSocket: Connection OK");
                m_connectionErrorType = CONNECTION_OK;
            }
        	DataFileManager.writeMsgLog("CryoSocket established connection", debug);   	            
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
        //boolean wasConnected = isConnected();
        //Messages.postDebug("cryocmd", "CryoSocket.disconnect(): connected="
        //                   + wasConnected);
        try{
            if (m_connection != null) {
            	m_connection.shutdownInput();
            	m_connection.shutdownOutput();            	
                m_connection.close(); // this will also close input and output streams
            }
            m_instream=null;           
            m_outstream=null;
            m_connection = null;
            //if (wasConnected) 
            	DataFileManager.writeMsgLog("CryoSocket closed", debug);   	            
       } catch (IOException ioe){
        	DataFileManager.writeMsgLog("IO exception disconnecting from CryoMonitor", debug);   	
            //System.out.println("IO exception disconnecting from CryoMonitor");
        }
    }

    /**
     * Read a byte from the input stream.
     * Blocks until it is available.
     */
    public int read() throws IOException {
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
            return false;
        } 
    }

}
