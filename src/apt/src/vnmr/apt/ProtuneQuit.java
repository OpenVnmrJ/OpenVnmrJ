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

import java.io.*;
import java.net.*;


public class ProtuneQuit {

    public static void main(String[] args) {
        if (args.length != 2) {
            return;
        }
        String host = args[0];
        int port = Integer.parseInt(args[1]);
        if (port == 0) {
            port = ProbeTune.lockPortNumber;
        }
        Socket socket = getSocket("ProtuneQuit", host, port);
        if (socket != null) {
            try {
                PrintWriter cmdSender
                        = new PrintWriter(socket.getOutputStream(), true);
                cmdSender.println("quitgui");
            } catch (IOException ioe) {
//                 System.err.println("ProtuneQuit: IOException: " + ioe);
            }
        }
    }


    /**
     * Open a socket by hostname and port number.
     * Uses a default timeout set in this method.
     * @param id A string that prefixes any error messages produced
     * by this method.
     * @param host The name or IP address of the host.
     * @param port The port number to connect to.
     * @return The socket, or null on failure.
     */
    public static Socket getSocket(String id, String host, int port) {
        final int TIMEOUT = 500; // ms
        Socket socket = null;
        try {
//             System.err.println(id + ": Trying connection (waiting "
//                                + TIMEOUT + " ms)");
            InetAddress inetAddr = InetAddress.getByName(host);
            InetSocketAddress inetSocketAddr;
            inetSocketAddr = new InetSocketAddress(inetAddr, port);
            socket = new Socket();
            socket.connect(inetSocketAddr, TIMEOUT);
        } catch (IOException ioe) {
            if (ioe instanceof SocketTimeoutException) {
                System.err.println(id + ": Timeout connecting" + ioe);
                socket = null;
            } else {
                String msg = ioe.toString();
                int idx = msg.indexOf("Connection refused");
                if (idx < 0) {
                    System.err.println(id + ": IOException connecting" + ioe);
                }
            }
        }
        return socket;
    }

}
