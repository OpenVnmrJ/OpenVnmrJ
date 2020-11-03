/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.util.*;
import java.net.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.*;

/**
 * <p>Title: WInfoStat</p>
 * <p>Description: Starts up the infostat process. Most of the code is the
 *                  the same as in ExpPanel to start the infostat.</p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 */

public class WInfoStat implements SocketIF
{

    private int winId = 0;
    private StatusProcess statusProcess = null;
    private StatusSocket stSocket;
    private Thread stSocketThread = null;
    private int statusPort = 0;
    private Thread statusThread = null;
    private static java.util.Map statusMessages = null;
    private static java.util.List statusListenerList = null;
    private AppIF appIf;

    public WInfoStat(AppIF ap)
    {
        this.appIf = ap;

        if (statusListenerList == null)
        {
            statusListenerList = Collections.synchronizedList(new LinkedList());
        }
        if (statusMessages == null)
        {
            statusMessages = Collections.synchronizedMap(new HashMap());
        }

        new Thread(new Runnable()
        {
            public void run()
            {
                runInfoStat();
            }
        }).start();
    }

    public void runInfoStat()
    {
        if (winId == 0 && statusProcess == null)
        {
            File f = new File(System.getProperty("sysdir")+"/acqbin/acqpresent");
            if (!f.exists())
            {
                  return;
            }
            f = new File(System.getProperty("sysdir")+"/bin/Infostat");
            if (!f.exists())
            {
                  return;
            }
            stSocket = new StatusSocket(this, winId);
            stSocketThread = new Thread(stSocket);
            statusPort = stSocket.getPort();
            stSocketThread.setName("SocketThread");
            stSocketThread.start();

            statusProcess = new StatusProcess(this, statusPort);
            statusThread = new Thread(statusProcess);
            statusThread.setName("Run Infostat");
            statusThread.start();
        }
    }

    static public String getStatusValue(String parm)
    {
        return (String)statusMessages.get(parm);
    }

    public void  statusProcessExit()
    {
        statusProcess = null;
    }

    public void  childProcessExit()
    {

    }

    public void  processAlphaData(String  str)
    {
        appIf.appendMessage(str+"\n");
    }

    public synchronized void  processGraphData(DataInputStream ins, int type)
    {
    }

    public void  processComData(String  str)
    {
    }

    public void  processMasterData(String  str)
    {
    }

    public void  statusProcessError()
    {
    }

    public void  graphSocketReady()
    {
    }

    public void  graphSocketError(Exception e)
    {

    }

    public void  comSocketError(Exception e)
    {
    }

    public void  alphaSocketError(Exception e)
    {
    }

    public void processPrintData(DataInputStream in, int type)
    {
    }

    public synchronized void quit()
    {
        if (statusProcess != null)
        {
            statusProcess.killProcess();
        }
        if (statusThread != null)
        {
            if (statusThread.isAlive())
            {
                try
                {
                     statusThread.join(500);
                }
                catch (InterruptedException e) { }
            }
        }
        if (stSocketThread != null)
        {
            if (stSocketThread.isAlive())
            {
                stSocket.quit();
                try
                {
                     stSocketThread.join(500);
                }
                catch (InterruptedException e) { }
            }
        }

        statusProcess = null;
        statusThread = null;
        stSocket = null;
    }

    public void  processStatusData(String  str)
    {
        /* Put this message in hash table */
        StringTokenizer tok = new StringTokenizer(str);
        if ( ! tok.hasMoreTokens())
        {
            return;
        }
        String parm = tok.nextToken();
        String v = (String) statusMessages.get(parm);
        if (str.equals(v))
        {
             return;
        }
        statusMessages.put(parm, str);

        /* Send message to all listeners */
        synchronized(statusListenerList)
        {
            Iterator itr = statusListenerList.iterator();
            while (itr.hasNext())
            {
                StatusListenerIF pan = (StatusListenerIF)itr.next();
                pan.updateStatus(str);
            }
        }
    }

    static public void addStatusListener(StatusListenerIF pan)
    {
        if (statusListenerList == null)
        {
            statusListenerList = Collections.synchronizedList(new LinkedList());
        }

        if (!statusListenerList.contains(pan))
        {
            statusListenerList.add(pan);

            if (statusMessages == null)
            {
                statusMessages = Collections.synchronizedMap(new HashMap());
            }

            /* Send all the current status data to the new listener */
            synchronized(statusMessages)
            {
                Iterator itr = statusMessages.values().iterator();
                while (itr.hasNext())
                {
                    pan.updateStatus((String)itr.next());
                }
            }
        }
    }

    static public void updateStatusListener(StatusListenerIF pan)
    {
        /* Send all the current status data to the specified listener */
        synchronized(statusMessages)
        {
            Iterator itr = statusMessages.values().iterator();
            while (itr.hasNext())
                pan.updateStatus((String)itr.next());
        }
    }

    static public void removeStatusListener(StatusListenerIF pan)
    {
        statusListenerList.remove(pan);
    }
}
