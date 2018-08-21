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

public class CanvasOutSocket {
   private SocketIF  socketIf;
   private Socket vnmrSocket = null;
   private int  socketPort;
   private String  host;
   private PrintWriter out2Vnmr = null;

   public CanvasOutSocket(SocketIF obj, String host,  int port) {
	this.socketIf = obj;
	this.host = host;
	this.socketPort = port;
   }

   public void outPut(String str) {
	if ((vnmrSocket == null) || (out2Vnmr == null)) {
	  try
          {
	      vnmrSocket = new Socket (host, socketPort);
	      if (vnmrSocket == null)
		return;
	      vnmrSocket.setTcpNoDelay(true);
	      out2Vnmr = new PrintWriter (
                        vnmrSocket.getOutputStream(), true);
	      if (out2Vnmr == null)
		return;
	      out2Vnmr.println(str);
/*
           out2Vnmr.close();
	   vnmrSocket.close();
*/
          }
          catch (Exception e)
          {
              System.out.println(e);
              Messages.writeStackTrace(e);
          }
	}
	else
	    out2Vnmr.println(str);
   }

/***
   public void outPut(String str) {
	try
        {
	   PrintWriter out2Vnmr = new PrintWriter (
                        vnmrSocket.getOutputStream(), true);
	   out2Vnmr.println(str);
           out2Vnmr.close();
        }
        catch (Exception e)
        {  System.out.println(e);
        }
   }
**/
}

