/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.text.SimpleDateFormat;


public class OptimaFWUpdate {

    private static final int CONNECT_TIMEOUT = 10000; // ms
    private static final int MOTOR_SOCKET_TIMEOUT = 10000; // ms

    /** Data bytes per block; doesn't count 3 bytes hdr + 1 byte checksum */
    private static final int BLOCKSIZE = 528;

    /** Header bytes in motor message blocks */
    private static final int HDRSIZE = 3;

    private static final int SET_DOWNLOAD = 0xe0;
    private static final int DATA_DOWNLOAD = 0xe1;
    private static final int LAST_DATA = 0xe2;
    private static final int END_DOWNLOAD = 0xe3;
    private static final int ABORT_DOWNLOAD = 0xe4;

    private static final int READY_FOR_DATA = 0x7a;
    private static final int RCVD_DATA = 0x7b;
    private static final int RCVD_FINAL_DATA = 0x7c;
    private static final int RESEND_DATA = 0x7d;
    private static final int BUSY_DATA = 0x7e;
    private static final int DOWNLOAD_FAILED = 0x7f;

    static private SimpleDateFormat dfmt = new SimpleDateFormat("HH:mm:ss.SSS");
    static private String m_binFilePath = "Optima.bin";
    static private String m_ip = "172.16.0.245";
    static private int m_port = 23;

    private OutputStream m_motorWriter;
    private InputStream m_motorReader;


    /**
     * @param args [-file PATH] [-ip HOST] [-port PORT] [-debug CAT1,CAT2,...]
     */
    public static void main(String[] args) {
        // Set class variables from args
        int len = args.length;
        for (int i = 0; i < len; i++) {
            if (args[i].equalsIgnoreCase("-file") && i + 1 < len) {
                m_binFilePath = args[++i];
            } else if (args[i].equalsIgnoreCase("-ip") && i + 1 < len) {
                m_ip = args[++i];
            } else if (args[i].equalsIgnoreCase("-port") && i + 1 < len) {
                m_port = Integer.parseInt(args[++i]);
            } else if (args[i].equalsIgnoreCase("-debug") && i + 1 < len) {
                DebugOutput.addCategories(args[++i]);
            }
        }
        new OptimaFWUpdate();
    }

    public OptimaFWUpdate() {
        // Open binary socket
        Socket motorSocket = initializeSocket(m_ip, m_port);
        if (motorSocket == null) {
            return;
        }
        downloadBinFile(m_binFilePath);
        try {
            motorSocket.close();
        } catch (Exception e) {}
    }

    public OptimaFWUpdate(String ip, String filepath,
                          String bootloaderVersion) {
        // TODO: use bootloaderVersion to set download method
        // Open binary socket
        Socket motorSocket = initializeSocket(ip, m_port);
        if (motorSocket == null) {
            return;
        }
        downloadBinFile(filepath);
        try {
            motorSocket.close();
        } catch (Exception e) {}
    }

    private boolean downloadBinFile(String filepath) {
        // Open the firmware .bin file
        InputStream fileIn = initializeFileInput(filepath);
        if (fileIn == null) {
            return false;
        }

        // Download file
        return downloadFirmware(fileIn, m_motorReader);
    }

    private boolean downloadFirmware(InputStream fileIn,
                                     InputStream fromMotor) {
        boolean ok = true;
        String msg = "";
        try {
            // Switch Optima to download mode (binary)
            msg = "sending FirmwareUpdate command";
            writeToMotor("FirmwareUpdate\r\n".getBytes());
            byte[] byteReply = new byte[32];
            fromMotor.read(byteReply);
            String strReply = new String(byteReply);
            if (!strReply.toLowerCase().contains("update mode on")) {
                ok = false;
                errorMsg("Bad reply to FirmwareUpdate command: " + strReply);
            }

            /************** EXTRA DELAY ****************/
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }/*DBG*/
            /*******************************************/

            if (ok) {
                byte[] msg0 = {(byte)SET_DOWNLOAD,
                    (byte)0,
                    (byte)0,
                    (byte)7,
                    };
                msg = "sending SET_DOWNLOAD cmd";
                writeToMotor(msg0);
                int reply;
                msg = "reading SET_DOWNLOAD reply";
                reply = fromMotor.read();
                //logMsg("Reply=0x" + Integer.toHexString(reply));
                if (reply != READY_FOR_DATA) {
                    errorMsg("Optima refused going into download mode");
                    ok = false;
                } else {
                    ok = sendData(fileIn, fromMotor);
                }
            }
        } catch (IOException e) {
            ok = false;
            errorMsg("Error " + msg + ": " + e);
        }
        if (ok) {
            logMsg("New firmware downloaded");
            try {
                // TODO: Wait for message that Optima is ready?
                Thread.sleep(20000); // Wait for module to reboot
            } catch (InterruptedException e) { }
            logMsg("New firmware ready");
        } else {
            errorMsg("Firmware download failed");
        }
        return ok;
    }

    private boolean sendData(InputStream fileIn, InputStream fromMotor) {
        boolean ok = true;
        boolean done = false;
        byte[] dataBuf = new byte[HDRSIZE + BLOCKSIZE + 1];
        short checksum = 0;
        int nbusys = 0;
        String msg = "";
        int bytesSent = 0; 
        try {
            for (int blockNum = 0; ok && !done; blockNum++) {
                msg = "reading data block " + blockNum + " from file";
                int len = fileIn.read(dataBuf, HDRSIZE, BLOCKSIZE);
                msg = "checking for eof";
                boolean eof = (len < BLOCKSIZE) || (fileIn.available() == 0);
                dataBuf[0] = (byte)(eof ? LAST_DATA : DATA_DOWNLOAD);
                dataBuf[1] = (byte)((len & 0xff00) >> 8);
                dataBuf[2] = (byte)(len & 0xff);
                int checkByte = calcCheckbyte(dataBuf, 0, HDRSIZE + len);
                dataBuf[HDRSIZE + len] = (byte)checkByte;
                checksum += checkByte;
                //logMsg("Sending block " + blockNum);
                int retries = 3;
                int reply = 0;
                boolean gotFinalReply = false;
                for (int i = 0; i < retries && !gotFinalReply; ) {
                    if (i == 0 || reply == RESEND_DATA) {
                        if (i > 0) {
                            logMsg("Retry sending block " + blockNum);
                        } else {
                            bytesSent += len;
                        }
                        msg = "sending data block " + blockNum + " to Optima";
                        // Don't always write whole buffer -- last block short
                        writeToMotor(dataBuf, HDRSIZE + len + 1);
                        logMsg("Sent block " + blockNum
                               + ", checkByte=0x"
                               + String.format("%02X", checkByte)
                               + ", total bytes downloaded=" + bytesSent
                               );
                        i++;
                    }
                    msg = "reading block " + blockNum + " ack from Optima";
                    reply = fromMotor.read();
                    if (reply == RESEND_DATA) {
                        errorMsg("Block " + blockNum + " rejected by Optima");
                    } else if (reply == BUSY_DATA && eof) {
                        // NB: 0.1.3 version of updating
                        fromMotor.skip(1); // Skip cksum byte
                        logMsg("Processed block " + (nbusys++));
                    } else {
                        gotFinalReply = true;
                    }
                }
                if (!gotFinalReply) {
                    errorMsg("Failed to download data block " + blockNum);
                    ok = false;
                } else if (reply == DOWNLOAD_FAILED) {
                    errorMsg("Optima reported an error on block " + blockNum);
                    ok = false;
                } else if (reply == RCVD_FINAL_DATA && eof) {
                    if (nbusys > 0) {
                        // NB: Old (0.1.3) version of updating
                        logMsg("Processed last block: " + (nbusys++));
                    }
                    short optChecksum = 0;
                    fromMotor.skip(2); // Skip data length (always 0)
                    optChecksum += fromMotor.read() << 8;
                    optChecksum += fromMotor.read();
                    if (optChecksum == checksum) {
                        byte[] finalBuf = {
                            (byte)END_DOWNLOAD,
                            (byte)0,
                            (byte)0,
                            (byte)0x1f,
                            };
                        msg = "sending END_DOWNLOAD to Optima";
                        writeToMotor(finalBuf);
                        logMsg("Cumulative checksum ok: " + checksum);
                        done = true; // Successful finish
                    } else {
                        errorMsg("Checksum mismatch: host=" + checksum
                                 + ", Optima=" + optChecksum);
                        ok = false;
                    }
                } else if (reply != RCVD_DATA) {
                    errorMsg("Bad reply from Optima on block " + blockNum
                             + ": 0x" + Integer.toHexString(reply));
                    ok = false; // Something unexpected happened
                }
                if (!ok) {
                    byte[] errorBuf = {
                        (byte)ABORT_DOWNLOAD,
                        (byte)0,
                        (byte)0,
                        (byte)0x27,
                        };
                    msg = "Sending abort command to Optima";
                    errorMsg(msg);
                    writeToMotor(errorBuf);
                }
            }
        } catch (IOException e) {
            errorMsg("Error " + msg + ": " + e);
            ok = false;
        }
        return ok;
    }

    /**
     * Write a given byte array to the motor.
     * @param msg Array containing the bytes to send.
     * @throws IOException
     */
    private void writeToMotor(byte[] msg) throws IOException {
        writeToMotor(msg, msg.length);
    }

    /**
     * Write a given number of bytes to the motor.
     * @param msg Array containing the bytes to send.
     * @param len The initial "len" bytes of the array are sent.
     * @throws IOException
     */
    private void writeToMotor(byte[] msg, int len) throws IOException {
        if (DebugOutput.isSetFor("FwDownload")) {
            StringBuilder sb = new StringBuilder();
            sb.append(": Sending ").append(len).append(" bytes:");
            for (int i = 0; i < len; i++) {
                sb.append(String.format(" %02X", msg[i]));
            }
            // Prepend time stamp
            sb.insert(0, dfmt.format(System.currentTimeMillis()));
            logMsg(sb.toString());
        }
        m_motorWriter.write(msg, 0, len);
        m_motorWriter.flush();
    }

    private int calcCheckbyte(byte[] buf, int start, int end) {
        int sum = 0;
        for (int i = start; i < end; i++) {
            sum += buf[i];
            int c = ((sum & 0x80) == 0) ? 0 : 1; // get top bit
            sum = (sum << 1) | c; // rotate top bit into lsb
        }
        sum &= 0xff;
        return sum;
    }

    private InputStream initializeFileInput(String path) {
        InputStream fileInput = null;
        try {
            fileInput = new FileInputStream(path);
            Messages.postDebug("Input: " + new File(path).getAbsolutePath() + ": "
                               + new File(path).length() + " bytes");
        } catch (FileNotFoundException e) {
            errorMsg("File not found: " + path);
        }
        return fileInput;
    }

    private Socket initializeSocket(String ip, int port) {
        Socket motorSocket = connectSocket(ip, port);
        if (motorSocket == null || !getBinaryIOStreams(motorSocket)) {
            try {
                motorSocket.close();
            } catch (Exception e) {}
            motorSocket = null;
        }
        return motorSocket;
    }

    private boolean getBinaryIOStreams(Socket motorSocket) {
        boolean ok = motorSocket != null;
        if (ok) {
            try {
                m_motorWriter = motorSocket.getOutputStream();
                m_motorReader = motorSocket.getInputStream();
            } catch (IOException ioe) {
                errorMsg("IOException: " + ioe);
                ok = false;
            }
        }
        return ok;
    }

    private Socket connectSocket(String ip, int port) {
        Socket socket = null;
        InetAddress inetAddr = null;
        try {
            inetAddr = InetAddress.getByName(ip);
        } catch (UnknownHostException uhe) {
            errorMsg("No IP address found for Motor module \""
                         + ip + "\"");
        }

        if (inetAddr != null) {
            try {
                logMsg("Connect to "
                           + inetAddr.getHostAddress() + ":" + port
                           + " (timeout=" + (CONNECT_TIMEOUT / 1000.0) + " s)");
                InetSocketAddress inetSocketAddr
                        = new InetSocketAddress(inetAddr, port);
                socket = new Socket();
                socket.connect(inetSocketAddr, CONNECT_TIMEOUT);
            } catch (IOException ioe) {
                if (ioe instanceof SocketTimeoutException) {
                    errorMsg("Timeout connecting to Motor socket \""
                                 + ip + "\"");
                } else {
                    errorMsg("IOException connecting to \""
                                 + ip + "\"");
                }
                socket = null;
            }
        }

        if (socket != null) {
            SocketAddress socketAddr = socket.getRemoteSocketAddress();

            if (socketAddr == null) {
                errorMsg("Could not get socket address");
            } else {
                logMsg("Socket address = " + socketAddr);
            }

            /*
             * Set the timeout for read() function so the MotorReadThread will
             * not block forever
             */
            try {
                socket.setSoTimeout(MOTOR_SOCKET_TIMEOUT);
            } catch (SocketException se) {
                errorMsg("Can't set socket timeout");
            }
        }
        return socket;
    }

    private void logMsg(String msg) {
        Messages.postDebug(msg);
    }

    private void errorMsg(String msg) {
        Messages.postError(msg);
    }
}
