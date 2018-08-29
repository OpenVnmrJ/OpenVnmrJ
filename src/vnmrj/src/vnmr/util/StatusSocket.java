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
import java.lang.*;

import vnmr.util.Messages;

public class StatusSocket implements Runnable {
   private SocketIF  socketIf;
   private ThreadGroup  group;
   private ServerSocket svr;
   private int  socketPort;
   private boolean  go;
   private StatusSocketHandler  statusHandler = null;

   public StatusSocket(SocketIF pobj, int id) {
	this.socketIf = pobj;
/*
	group = new ThreadGroup("statusSocket#"+id);
*/
	try {
           svr = new ServerSocket(0);
	   socketPort = svr.getLocalPort();
        } catch (Exception e) {
            Messages.writeStackTrace(e);
        }
   }

   public int getPort() {
	return socketPort;
   }

   public void run()
   {
	go = true;
	try
	{  
	   while(go) {
	      Socket incoming = svr.accept( );
   	      acceptit(incoming);
	   }
	}
        catch (SocketException se) {
            // This is normal
        } catch (Exception e) {
            // This is not normal
	    //System.out.println(e);
            Messages.writeStackTrace(e);
	}
   }

   public void quit() {
	try {
	   svr.close();
	} catch (IOException e) { }
	go = false;
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
	    if (k > 0) {
		CmdMsge = new String(cmd,0,k);
                if (!DebugOutput.isSetFor("RFMonData")) {
                    socketIf.processStatusData(CmdMsge);
                }
	    }
	}
	catch (Exception e) {  
		System.err.println(e);
                Messages.writeStackTrace(e);
		return;
        }
	if (statusHandler != null) {
		statusHandler.quit();
		statusHandler = null;
	}
	statusHandler = new StatusSocketHandler(i, socketIf,group);
        statusHandler.setName("StatusSocketHandler");
	statusHandler.start();

   }


  class StatusSocketHandler extends Thread  {
    private SocketIF  skIf;
    private boolean finished = false;
    public StatusSocketHandler(Socket i, SocketIF pif, ThreadGroup gp) {
        skIf = pif;
        incoming = i;
        setPriority(Thread.NORM_PRIORITY+2);
    }

    public void run() {  
        try {
            BufferedReader bIn = new BufferedReader
                    (new InputStreamReader(incoming.getInputStream()));

            while (!finished) {  
                String str = bIn.readLine();
                if (str != null) {
                    if (str.length() > 1) {
                        if (!DebugOutput.isSetFor("RFMonData")) {
                            skIf.processStatusData(str);
                        }
                    }
                } else {
                    finished = true;
                }
            }
            incoming.close();
        } catch (SocketException se) {
            finished = true;
        } catch (Exception e) {  
            finished = true;
            //System.err.println(e);
            Messages.writeStackTrace(e);
        }
    }

    public void quit() {
	finished = true;
    }

    protected void finalize() throws Throwable {
        incoming.close();
    }

    private Socket incoming;
  }
}


