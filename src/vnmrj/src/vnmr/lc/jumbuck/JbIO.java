/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc.jumbuck;

import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.nio.ByteOrder;

import javax.imageio.stream.ImageOutputStream;
import javax.imageio.stream.MemoryCacheImageOutputStream;

import vnmr.lc.LcMsg;


public class JbIO implements JbDefs {

    private final static int SOCKET_CONNECT_TIMEOUT = 10000; // ms
    private final static int SOCKET_READ_TIMEOUT = 0; // ms

    private final static int ERROR_OK = 0;
    private final static int ERROR_IOEXCEPTION = 1;
    private final static int ERROR_DISCONNECTED = 2;
    private final static int ERROR_NOHOST =3;

    /** The host name (or IP address) of the instrument. */
    private String m_hostName = null;

    /** The port number to use for the connection to the instrument. */
    private int m_portNumber = 0;

    /** For reading from the instrument. */
    private JbInputStream m_in;

    /** For writing to the instrument. */
    private ImageOutputStream m_out;

    /** True if the last "open" try failed. */
    private boolean m_checkFailed = false;

    /** This is the thread that reads from the instrument. */
    private Reader m_reader;

    /** Socket for communication with the instrument. */
    private Socket m_socket;

    /** Who deals with responses from the instrument. */
    private JbReplyListener m_replyListener = null;

    /** Track I/O errors. */
    private int m_errorCount = 0;

    /** What our current error status is. */
    private int m_error = ERROR_DISCONNECTED;


    /**
     * 
     */
    public JbIO() {
    }

    /** Get the internal error count. */
    public int getErrorCount() {
        return m_errorCount;
    }

    /** Set the internal error count. */
    public void setErrorCount(int count) {
        m_errorCount = count;
    }

    public void setReplyListener(JbReplyListener l) {
        m_replyListener = l;
    }

    /**
     * Reopen a socket connection to the instrument.  Use the previous
     * host name and port number.
     */
    public boolean openPort() {
        return openPort(m_hostName, m_portNumber);
    }

    /**
     * Open a socket connection to the instrument with the default
     * port number.
     */
    public boolean openPort(String hostName) {
        return openPort(hostName, DEFAULT_PORT_NUMBER);
    }

    /**
     * Open a socket connection to the instrument.
     */
    public boolean openPort(String hostName, int portNumber) {
        boolean ok = true;
        if (isPortOpen()) {
            closePort();
        }
        m_hostName = hostName;
        m_portNumber = portNumber;

        LcMsg.postDebug("jbIO", "JbIO.openPort(): hostName=" + hostName);

        try {
            LcMsg.postDebug("jbIO", "JbIO.openPort(): creating socket ...");
            m_socket = new Socket();
            SocketAddress socketAddr
                    = new InetSocketAddress(hostName, portNumber);
            m_socket.connect(socketAddr, SOCKET_CONNECT_TIMEOUT);
            LcMsg.postDebug("jbIO", "JbIO.openPort(): ... got socket");
            try {
                m_socket.setSoTimeout(SOCKET_READ_TIMEOUT);
            } catch (SocketException se) {
                LcMsg.postDebug("Cannot set timeout on PDA335 communication");
            }
            OutputStream outputStream = m_socket.getOutputStream();
            m_out = new MemoryCacheImageOutputStream(outputStream);
            m_out.setByteOrder(ByteOrder.LITTLE_ENDIAN);
            m_reader = new Reader();
            m_reader.start();
            if (m_error != ERROR_OK) {
                LcMsg.postInfo("PDA 335 connection OK");
                m_error = ERROR_OK;
            }
        } catch (UnknownHostException uhe) {
            ok = false;
            if (m_error != ERROR_NOHOST) {
                LcMsg.postError("JbIO.openPort: Don't know about host: \""
                                + hostName + "\" " + uhe);
                m_error = ERROR_NOHOST;
            }
        } catch (IOException ioe) {
            ok = false;
            if (m_error != ERROR_IOEXCEPTION) {
                LcMsg.postError("JbIO.openPort: problem connecting to \""
                                + hostName + "\" " + ioe);
                m_error = ERROR_IOEXCEPTION;
            }
        }
        LcMsg.postDebug("jbIO", "JbIO.openPort(): m_socket = " + m_socket);

        return ok;
    }

    public void closePort() {
        if (m_socket != null) {
            if (m_error == ERROR_OK) {
                LcMsg.postWarning("PDA 335 disconnecting");
                m_error = ERROR_DISCONNECTED;
            }
            try {
                m_socket.close();
            } catch (IOException ioe) {}
            m_socket = null;
            m_in = null;
            m_out = null;
            m_checkFailed = false;
        }
        LcMsg.postDebug("jbio", "m_socket closed");
    }

    public boolean isPortOpen() {
        return (m_socket != null && m_in != null && m_out != null);
    }

    public ImageOutputStream getWriter() {
        return m_out;
    }

    public JbInputStream getReader() {
        return m_in;
    }

    public class Reader extends Thread {

        private boolean mm_quit = false;

        public Reader() throws IOException, SocketException {
            setName("JbIO.Reader");
            m_in = new JbInputStream(m_socket.getInputStream());
            //m_in.setByteOrder(ByteOrder.LITTLE_ENDIAN);
        }            

        public void quit() {
            mm_quit = true;
        }

        public void run() {
            int code = 0;
            while (!mm_quit) {
                try {
                    code = m_in.readUnsignedShort();
                    boolean echo = LcMsg.isSetFor("jbio");
                    if (code == ID_RSP_RETURN_STATUS.intValue()) {
                        // Careful about echoing too much
                        echo = LcMsg.isSetFor("jbstatus");
                    }
                    if (echo) {
                        LcMsg.postDebug("\nPDA335.Reader got code: 0x"
                                        + Integer.toHexString(code));
                    }
                    if (m_replyListener != null) {
                        //m_socket.setSoTimeout(100);/* Don't read next msg */
                        m_replyListener.receiveReply(code, m_in);
                        //m_socket.setSoTimeout(0);/* Reset timeout */
                    }
                } catch (SocketTimeoutException stoe) {
                    if (m_error == ERROR_OK) {
                        LcMsg.postError("PDA335.Reader: Socket read timeout");
                    }
                    mm_quit = true;
                } catch (IOException ioe) {
                    if (m_error == ERROR_OK) {
                        LcMsg.postError("PDA335.Reader: Socket read failed");
                    }
                    mm_quit = true;
                }
            }
            if (m_error == ERROR_OK) {
                LcMsg.postError("PDA335.Reader: quitting");
                m_error = ERROR_IOEXCEPTION;
            }
            if (m_socket != null) {
                try {
                    m_socket.close();
                } catch (IOException ioe) {
                }
            }

            // TODO: Retry on communication loss
        }
    }

}
