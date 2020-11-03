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
import vnmr.ui.*;
import vnmr.admin.ui.*;

public class StatusProcess implements Runnable {
   private SocketIF  socketIf;
   private int       socketPort;
   private Process   chkit;

   public StatusProcess(SocketIF pobj, int port) {
	this.socketIf = pobj;
	this.socketPort = port;
   }

   public void killProcess() {
	int k = 0;
/*
	while (chkit != null) {
	   k++;
	   try {
	   	if (k < 12)
	      	   Thread.sleep(500);
	   	else
	      	   break;
	   } catch (InterruptedException e) { }
	}
*/
	if (chkit != null)
	   chkit.destroy();
   }

   public void run() {
       try {
           String cmd = "Infostat -port "+socketPort;
           Runtime rt = Runtime.getRuntime();
           // exec and get back a Process class
           chkit = rt.exec(cmd);
           int ret = chkit.waitFor();
           chkit = null;
           socketIf.statusProcessExit();
       }
       catch (IOException e)
       {
           System.out.println(e);
           chkit = null;
           socketIf.statusProcessError();
       }
       catch (InterruptedException er)
       {
           System.out.println(er);
           chkit = null;
           socketIf.statusProcessError();
       }
   }
}

