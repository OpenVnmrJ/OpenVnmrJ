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

public class ToVnmrSocket {
   private Socket vnmrSocket;
   private int  socketPort;
   private String  host;

   public ToVnmrSocket(String host,  int port) {
	this.host = host;
	this.socketPort = port;
   }

   public void send(String str) {
	try
        {
	   vnmrSocket = new Socket (host, socketPort);
	   if (vnmrSocket == null)
		return;
	   vnmrSocket.setTcpNoDelay(true);
	   PrintWriter out2Vnmr = new PrintWriter (
                        vnmrSocket.getOutputStream(), true);
	   out2Vnmr.println(str);
           out2Vnmr.close();
	   vnmrSocket.close();
        }
        catch (Exception e)
        {  System.out.println(e);
        }
   }
}

