/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;


public class RunTest implements Executer, ListenerManager {

    /**
     * Run ProTune under this as a separate process.
     * Just calls the RunTest constructor with the supplied command-line
     * arguments.
     * @see #RunTest(String[])
     * @param args Command and arguments to start ProTune as from the
     * command line.
     */
    public static void main(String[] args) {
        new RunTest(args);
    }

    /**
     * Run ProTune as a client, so that ProTune connects to the server socket
     * that this class provides for its commands and replies.
     * User enters commands at this process's console window, and views the
     * replies there.
     * ProTune is started as a separate process,
     * using runTime.exec(String[] args).
     * <p>
     * Appends extra arguments to the supplied arguments to identify
     * the socket to ProTune.
     * This is a switch of the form "<code>-cmdSocket 1234</code>".
     * <p>
     * Typical supplied arguments:
     * <pre>
     * java
     * -cp apt.jar
     * vnmr.apt.ProbeTune
     * -motorIP 127.0.0.1
     * -probe autox400DB
     * -vnmrsystem /vnmr
     * -vnmruser /home/vnmr1/vnmrsys
     * </pre>
     * @param args Command and arguments to start ProTune as from the
     * command line.
     */
    public RunTest(String[] args) {

        // Open a server socket
        CommandListener listener = new CommandListener(this, this);
        listener.start();
        int port = listener.getPort();

        // Copy passed args and add socket flag
        String[] cmds = new String[args.length + 2];
        int idx;
        for (idx = 0; idx < args.length; idx++) {
            cmds[idx] = args[idx];
        }
        cmds[idx++] = "-cmdSocket";
        cmds[idx++] = Integer.toString(port);

        // Start up ProTune
        Runtime rt = Runtime.getRuntime();
        try {
            rt.exec(cmds);
        } catch (IOException e) {
            e.printStackTrace();
        }

        // Listen for user input, and send it to ProTune
        BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
        PrintWriter out = listener.getWriter();
        String line;
        try {
            while ((line = in.readLine()) != null) {
                if (out == null) {
                    out = listener.getWriter();
                }
                out.println(line);
            }
        } catch (IOException e) {
            System.out.println("Read Error: " + e.getMessage());
            System.exit(-1);
        }
    }

    @Override
    /**
     * Receives messages from ProTune.
     * Just display to user.
     */
    public void exec(String cmd) {
        System.out.println("<" + cmd + ">");
    }

    @Override
    public void listenerConnected(CommandListener listener) {
        System.out.println("LISTENER CONNECTED");
    }

    @Override
    public void listenerDisconnected(CommandListener listener) {
        System.out.println("LISTENER DISCONNECTED");
        System.exit(0);
    }

}
