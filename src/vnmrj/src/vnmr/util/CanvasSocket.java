/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.util;

import java.io.*;
import java.net.*;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

import javax.swing.SwingUtilities;

public class CanvasSocket implements Runnable {
    private SocketIF  socketIf;
    private ThreadGroup  group;
    private ServerSocket svr;
    private int  socketPort;
    private boolean  go;

    public CanvasSocket(SocketIF pobj, int id) {
        this.socketIf = pobj;
        group = new ThreadGroup("Socket#"+id);
        try
        {
            svr = new ServerSocket(0);
            socketPort = svr.getLocalPort();
            svr.setReceiveBufferSize(120000);
        }
        catch (IOException e)
        {
            Messages.writeStackTrace(e, "Error caught in Socket thread.");
        }
    }

    public int getPort() {
        return socketPort;
    }

    public void run()
    {
        Thread.currentThread().setName("CanvasSocketServer");
        go = true;
        try
        {
            while(go) {
                Socket incoming = svr.accept( );
                acceptit(incoming);
            }
        }
        catch (IOException e)
        {
            if (go) {
                Messages.writeStackTrace(e,"Error caught in ServerSocket.");
            }
        }
    }

    public void quit() {
        go = false;
        try {
            svr.close();
        }
        catch (IOException e)
        {
        }
    }

    protected void finalize() throws Throwable {
        svr.close();
    }

    public void acceptit(Socket i)
    {
        byte[] cmd = new byte[250];

        String CmdMsge;
        int k = 0;

        try {
            InputStream is = i.getInputStream();
            k = 0;
            while (true)
            {
                is.read(cmd,k,1);
                if ( cmd[k] == 10 )
                {
                    break;
                }
                k++;
                if (k >= 250) {
                    return;
                }
            }
            CmdMsge = new String(cmd,0,k);
            if (CmdMsge.equals("graph"))
            {

                GraphSocketHandler gsh;
                gsh = new GraphSocketHandler(i, socketIf,group);
                gsh.setName("GraphSocketHandler");
                gsh.start();
                return;
            }
            if (CmdMsge.equals("common"))
            {
                ComSocketHandler csh;
                csh = new ComSocketHandler(i, socketIf,group);
                csh.setName("ComSocketHandler");
                csh.start();
                return;
            }
            if (CmdMsge.equals("mcomm"))
            {
                MasterSocketHandler msh;
                msh = new MasterSocketHandler(i, socketIf,group);
                msh.setName("MasterSocketHandler");
                msh.start();
                return;
            }
            if (CmdMsge.equals("alpha"))
            {
                AlphaSocketHandler ash;
                ash = new AlphaSocketHandler(i, socketIf,group);
                ash.setName("AlphaSocketHandler");
                ash.start();
                return;
            }
            if (CmdMsge.startsWith("addr"))
            {
                socketIf.processComData(CmdMsge);
                return;
            }
            if (CmdMsge.equals("print"))
            {
                PrintSocketHandler gsh;
                gsh = new PrintSocketHandler(i, socketIf,group);
                gsh.setName("PrintSocketHandler");
                gsh.start();
                return;
            }
            Messages.postError("CanvasSocket: Unknown command from Vnmrbg:"
                               + " \"" + CmdMsge + "\"");
            return;
        }
        catch (IOException e) {
            Messages.writeStackTrace(e, "Error caught in Server accept.");
            Messages.postError("Socket Server error on accept ");
        }
    }


    class GraphSocketHandler extends Thread  {
        private Socket incoming;
        private boolean done = false;
        private SocketIF  skIf;
        public GraphSocketHandler(Socket i, SocketIF pif, ThreadGroup gp) {
            skIf = pif;
            incoming = i;
            setPriority(Thread.NORM_PRIORITY+2);
        }

        public void run()
        {
            try {
                BufferedInputStream bIn = new BufferedInputStream
                        (incoming.getInputStream(), 1024 * 120);
                DataInputStream in = new DataInputStream (bIn);

                skIf.graphSocketReady();
                while (!done)
                {
                    int  type = in.readInt();
                    skIf.processGraphData(in, type);
                }
                incoming.close();
            }
            catch (IOException e)
            {
                skIf.graphSocketError(e);
                done = true;
            }
        }

        public void quit() {
            done = true;
        }

        protected void finalize() throws Throwable {
            incoming.close();
            done = true;
        }

    }

    class ComSocketHandler extends Thread  {
        private Socket incoming;
        private SocketIF  skIf;
        private boolean finished = false;
        private List m_msgList
            = Collections.synchronizedList(new LinkedList());

        public ComSocketHandler(Socket i, SocketIF pif, ThreadGroup gp) {
            skIf = pif;
            incoming = i;
            setPriority(Thread.NORM_PRIORITY+2);
        }

        /**
         * Waits for messages from VnmrBG and puts them in a message list
         * as they come in. Makes sure processData() is running in the
         * Event-Dispatch thread whenever there are messages in the list.
         */
        public void run() {
            try {
                BufferedReader bIn = new BufferedReader
                        (new InputStreamReader(incoming.getInputStream()));

                while (!finished) {
                    // NB: This sleeps until input is available
                    String msg = bIn.readLine();
                    if (msg == null) {
                        break;
                    } else if (msg.length() > 1) {
                        if (DebugOutput.isSetFor("ComSocket")) {
                            System.out.println(System.currentTimeMillis()
                                               + " ComSocket got msg: "
                                               + Fmt.safeAscii(msg));
                        }
                        m_msgList.add(msg);
                        if (m_msgList.size() == 1) {
                            // Queue up data processing in the Event Thread
                            SwingUtilities.invokeLater(new Runnable() {
                                    public void run() { processData(); } }
                                                       );
                        }
                    }
                }
            } catch (IOException e) {
                skIf.comSocketError(e);
            }

            try {
                incoming.close();
            } catch (IOException ioe) {}
        }

        /**
         * This method is run only in the Event-Dispatching Thread and
         * processes the next message in the message list.
         * Blocks other messages from being queued up until done
         * processing this message.
         */
        protected void processData() {
            // We assume other threads only add to the list.
                boolean empty = (m_msgList.size() == 0);
                if (!empty) {
                    try {
                        String msg = (String)m_msgList.remove(0);
                        empty = (m_msgList.size() == 0);
                        skIf.processComData(msg);
                    } catch (Exception e) {
                        Messages.writeStackTrace
                                (e, "Error in ComSocket processing: ");
                    }
                }
                if (!empty) {
                    if (DebugOutput.isSetFor("ComSocket")) {
                        System.out.println(System.currentTimeMillis()
                                           + " ComSocket queueing msg: "
                                           + Fmt.safeAscii
                                           ((String)m_msgList.get(0)));
                    }
                    SwingUtilities.invokeLater(new Runnable() {
                            public void run() { processData(); } }
                                               );
                }
        }

        public void quit() {
            finished = true;
        }

        protected void finalize() throws Throwable {
            incoming.close();
        }

    }

    class AlphaSocketHandler extends Thread  {
        private Socket incoming;
        private SocketIF  skIf;
        private boolean finished = false;
        public AlphaSocketHandler(Socket i, SocketIF pif, ThreadGroup gp) {
            skIf = pif;
            incoming = i;
        }

        public void run()
        {
            try {
                BufferedReader bIn = new BufferedReader
                        (new InputStreamReader(incoming.getInputStream()));

                while (!finished)
                {
                    String str = bIn.readLine();
                    if (str != null) {
                        if (str.length() > 0)
                            skIf.processAlphaData(str);
                    }
                    else
                        finished = true;
                }
                incoming.close();
            }
            catch (IOException e)
            {
                finished = true;
                skIf.alphaSocketError(e);
            }
        }

        public void quit() {
            finished = true;
        }

        protected void finalize() throws Throwable {
            incoming.close();
        }

    }

    class MasterSocketHandler extends Thread  {
        private SocketIF  skIf;
        private boolean finished = false;
        private Socket incoming;
        private List m_msgList
            = Collections.synchronizedList(new LinkedList());

        public MasterSocketHandler(Socket i, SocketIF pif, ThreadGroup gp) {
            skIf = pif;
            incoming = i;
            setPriority(Thread.NORM_PRIORITY+2);
        }

        public void run()
        {
            try {
                BufferedReader bIn = new BufferedReader
                        (new InputStreamReader(incoming.getInputStream()));

                while (!finished) {
                    // NB: This sleeps until input is available
                    String msg = bIn.readLine();
                    if (msg == null) {
                        break;
                    } else if (msg.length() > 1) {
                        if (DebugOutput.isSetFor("MasterSocket")) {
                            System.out.println(System.currentTimeMillis()
                                               + " MasterSocket got msg: "
                                               + Fmt.safeAscii(msg));
                        }
                        m_msgList.add(msg);
                        if (m_msgList.size() == 1) {
                            // Queue up data processing in the Event Thread
                            SwingUtilities.invokeLater(new Runnable() {
                                    public void run() { processData(); } }
                                                       );
                        }
                    }
                }
            } catch (IOException e) {
                skIf.comSocketError(e);
            }

            try {
                incoming.close();
            } catch (IOException ioe) {}
        }

        /**
         * This method is run only in the Event-Dispatching Thread and
         * processes the next message in the message list.
         * Blocks other messages from being queued up until done
         * processing this message.
         */
        protected void processData() {
            // We assume other threads only add to the list.
                boolean empty = (m_msgList.size() == 0);
                if (!empty) {
                    try {
                        String msg = (String)m_msgList.remove(0);
                        empty = (m_msgList.size() == 0);
                        skIf.processMasterData(msg);
                    } catch (Exception e) {
                        Messages.writeStackTrace
                                (e, "Error in MasterSocket processing: ");
                    }
                }
                if (!empty) {
                        if (DebugOutput.isSetFor("MasterSocket")) {
                            System.out.println(System.currentTimeMillis()
                                               + " MasterSocket queueing msg: "
                                               + Fmt.safeAscii
                                               ((String)m_msgList.get(0)));
                        }
                    SwingUtilities.invokeLater(new Runnable() {
                            public void run() { processData(); } }
                                               );
                }
        }

        public void quit() {
            finished = true;
        }

        protected void finalize() throws Throwable {
            incoming.close();
        }
    }

    class PrintSocketHandler extends Thread  {
        private Socket incoming;
        private boolean done = false;
        private SocketIF  skIf;
        public PrintSocketHandler(Socket i, SocketIF pif, ThreadGroup gp) {
            skIf = pif;
            incoming = i;
            setPriority(Thread.NORM_PRIORITY);
        }

        public void run()
        {
            try {
                BufferedInputStream bIn = new BufferedInputStream
                        (incoming.getInputStream(), 1024 * 120);
                DataInputStream in = new DataInputStream (bIn);

                while (!done)
                {
                    int  type = in.readInt();
                    skIf.processPrintData(in, type);
                }
                incoming.close();
            }
            catch (IOException e)
            {
                // skIf.graphSocketError(e);
                done = true;
            }
        }

        public void quit() {
            done = true;
        }

        protected void finalize() throws Throwable {
            incoming.close();
            done = true;
        }
    }
}
