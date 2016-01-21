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
 *
 */

import java.io.*;
import java.net.*;
import java.lang.*;

public class PlotSocket implements Runnable {
   private PlotIF  socketIf;
   private ThreadGroup  group;
   private PlotSocketHandler  statusHandler = null;

   public PlotSocket(PlotIF pobj) {
	this.socketIf = pobj;
	try
        {
           svr = new ServerSocket(0);
	   socketPort = svr.getLocalPort();
        }
        catch (Exception e)
        {  }
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
	catch (Exception e)
	{
	}
   }

   public void quit() {
	try {
	   svr.close();
	}
	catch (Exception e)
	{  
	}
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
	        PlotUtil.processData(CmdMsge);
	    }
	}
	catch (Exception e) {  
		System.err.println(e);
		return;
        }
	if (statusHandler != null) {
		statusHandler.quit();
		statusHandler = null;
	}
	statusHandler = new PlotSocketHandler(i, socketIf,group);
	statusHandler.start();

   }

   private ServerSocket svr;
   private int  socketPort;
   private boolean  go;
}


class PlotSocketHandler extends Thread  {
   private PlotIF  skIf;
   private boolean finished = false;
   public PlotSocketHandler(Socket i, PlotIF pif, ThreadGroup gp) {
     skIf = pif;
     incoming = i;
     setPriority(Thread.NORM_PRIORITY+2);
   }

   public void run()
   {  
	try {
	 BufferedReader bIn = new BufferedReader(new InputStreamReader(incoming.getInputStream()));

         while (!finished)
         {  
	   String str = bIn.readLine();
	   if (str != null) {
		if (str.length() > 1) {
	            PlotUtil.processData(str);
		}
	    }
	    else
		finished = true;
         }
         incoming.close();
      }
      catch (Exception e)
      {  
	 finished = true;
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

