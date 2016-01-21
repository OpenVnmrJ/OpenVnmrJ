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

public class TextProcess implements Runnable {
   private Process   chkit;
   private String   prog;

   public TextProcess(String cmd) {
	this.prog = cmd;
   }

	
   public void run() {
	try {
	      Runtime rt = Runtime.getRuntime();
              // exec and get back a Process class
              chkit = rt.exec(""+prog);
              // wait for process to exit
              int ret = chkit.waitFor();
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
}

