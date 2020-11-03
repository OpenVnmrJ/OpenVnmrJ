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

public class VnmrProcess implements Runnable {
   private SocketIF  socketIf;
   private String    info;
   private int       socketPort;
   private int       id;
   private Process   chkit;

   public VnmrProcess(SocketIF pobj, int port, int id, String info) {
	this.socketIf = pobj;
	this.socketPort = port;
	this.id = id;
	this.info = info;
   }

   public VnmrProcess(SocketIF pobj, int port, int id) {
	this(pobj, port, id, null);
   }

   public void killProcess() {
	int k = 0;
	while (chkit != null) {
	   k++;
	   try {
	   	if (k < 12)
	      	   Thread.sleep(500);
	   	else
	      	   break;
	   } catch (InterruptedException e) { }
	}
	if (chkit != null)
	   chkit.destroy();
   }

   public void run() {
       try {
           Runtime rt = Runtime.getRuntime();
           // exec and get back a Process class
           String cmd = "Vnmrbg master  -port "+socketPort+" -view "+id;
           if (Util.iswindows())
           {
               cmd = FileUtil.sysdir() + "/bin/Vnmrbg.exe master -port " +
                            socketPort + " -view " + id;
           }
           if (info != null)
               cmd = cmd + " "+info;
           chkit = rt.exec(cmd);
           new VnmrStderrReader().start();

           // wait for process to exit
           int ret = chkit.waitFor();
           chkit = null;
           socketIf.childProcessExit();
       }
       catch (IOException e)
       {
           System.out.println(e);
       }
       catch (InterruptedException ex)
       {
           System.out.println(ex);
       }
   }

    class VnmrStderrReader extends Thread {
        public void run() {
            setName("VnmrStderrReader");
            try {
                BufferedReader bIn = (new BufferedReader
                                      (new InputStreamReader
                                       (chkit.getErrorStream())));
                while (true) {
                    String msg = bIn.readLine();
                    if (msg == null) {
                        break;
                    }
                    if (msg.startsWith("<APT")) {
                        if (msg.startsWith("<APT-")) {
                            Messages.postDebug(msg);
                        } else {
                            Messages.postDebug("apt", msg);
                        }
                    } else {
                        Messages.postDebug("vnmr", "<VBG>: "+msg);
                    }
                }
            } catch (Exception e) {
                Messages.postError("VnmrJ: Error reading Standard Error"
                                   + " output from VnmrBG");
                Messages.writeStackTrace(e);
            }
        }
    }

}

