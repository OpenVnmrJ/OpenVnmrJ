/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.io.*;
import java.util.*;

public class PatientTableComm implements Runnable {

 private Vector PatientTableListeners = new Vector();
 private List ActiveEvents = new java.util.LinkedList();
 private  String         shToolCmd = "/bin/csh";
 private Thread myThread;
 private  MsgQueue  PtInputMsgQ;
 private  MsgQueue  PtOutputMsgQ;
 private  PtInput   PTin;
 private  PtOutput  PTout;
 private InputStreamReader inputSR;
 private BufferedReader bfr;
 private OutputStreamWriter outputSW;
 private static final String CmdPrompt = "Cmds:";
 private int cmdsActive = 0;
 // public List cmdll, ptCmdResponce;
 public List ptCmdResponce;



 public PatientTableComm(String execPath, String term)
 {
      // String execStr = Directory + "/" + "ptalign_dbx " + term;
      String execStr = execPath + " " + term;
      String[] cmd = {shToolCmd, "-c", execStr};

      Runtime rt = Runtime.getRuntime();
      Process prcs = null;
      try
      {
        prcs = rt.exec(cmd);
      }
      catch(IOException ie)
      {
        System.out.println(ie);
      }

     // cmdll = new java.util.LinkedList();
      // ptCmdResponce = new java.util.LinkedList();
      ptCmdResponce = new java.util.ArrayList();
      PtInputMsgQ = new MsgQueue();
      PtOutputMsgQ = new MsgQueue();
      PTin = new PtInput(PtInputMsgQ, prcs.getInputStream());

      /* diagnostic use, can give Patient Table Cmd Prompt ('Cmds:') though key board */
      //PTin = new PtInput(PtInputMsgQ, System.in);

      PTout = new PtOutput(PtOutputMsgQ, prcs.getOutputStream());

      /* diagnostic use, directs patient table cmds to standard out */
      //PTout = new PtOutput(PtOutputMsgQ, System.out);

       myThread = new Thread(this);
       int tPrior = myThread.getPriority();
       System.out.println("Priority Max: "+Thread.MAX_PRIORITY+", Min: "+Thread.MIN_PRIORITY);
       System.out.println("PatientTableComm: Thread Priority: "+tPrior);
       myThread.setPriority(Thread.MAX_PRIORITY);
       myThread.setDaemon(true);
       myThread.start();

      // inputSR = new InputStreamReader(prcs.getInputStream());
      // bfr = new BufferedReader(inputSR);
      // outputSW = new OutputStreamWriter(prcs.getOutputStream());
      //buttonObserver bObserv = new buttonObserver();
      //addObserver(bObserv);
 }

 public synchronized void moveX(int Xincr)
 {
   int type;
   String cmd;
   PatientTableEvent pte;
   if (Xincr >= 0)
   {
      type = PatientTableEvent.TABLE_EVENT_MOTION_IN;
      cmd = "MX+"+String.valueOf(Xincr)+"\n";
   }
   else
   {
      type = PatientTableEvent.TABLE_EVENT_MOTION_OUT;
      cmd = "MX"+String.valueOf(Xincr)+"\n";
   }
   pte = new PatientTableEvent(this,type,cmd);
   ActiveEvents.add(pte);
   cmdsActive++;
   PtOutputMsgQ.putmsg(cmd);
 }

 public synchronized void moveY(int Yincr)
 {
   int type;
   String cmd;
   PatientTableEvent pte;
   if (Yincr >= 0)
   {
      type = PatientTableEvent.TABLE_EVENT_MOTION_UP;
      cmd = "MY+"+String.valueOf(Yincr)+"\n";
   }
   else
   {
      type = PatientTableEvent.TABLE_EVENT_MOTION_DOWN;
      cmd = "MY"+String.valueOf(Yincr)+"\n";
   }
   pte = new PatientTableEvent(this,type,cmd);
   ActiveEvents.add(pte);
   cmdsActive++;
   PtOutputMsgQ.putmsg(cmd);
 }

 public synchronized void getYLoc()
 {
   int type;
   type = PatientTableEvent.TABLE_EVENT_QWERY_CMPLT;
   String cmd = "Y\n";
   PatientTableEvent pte = new PatientTableEvent(this,type,cmd);
   ActiveEvents.add(pte);
   cmdsActive++;
   PtOutputMsgQ.putmsg(cmd);
 }

 public synchronized void getXLoc()
 {
   int type;
   type = PatientTableEvent.TABLE_EVENT_QWERY_CMPLT;
   String cmd = "X\n";
   PatientTableEvent pte = new PatientTableEvent(this,type,cmd);
   ActiveEvents.add(pte);
   cmdsActive++;
   PtOutputMsgQ.putmsg(cmd);
 }

 //public synchronized void resetActive()
 //{
 //  cmdsActive = 0;
 //}
/*
 public synchronized void sendCmd(String cmd)
 {
   // cmdsActive++;
    cmdll.add(cmd);
    PtOutputMsgQ.putmsg(cmd);
 }
*/
 public String getCmd()
 {
    String msg = (String) PtInputMsgQ.getmsg();
    return(msg);
 }

 /*public boolean isActive()
 {
   // boolean entry = PtOutputMsgQ.hasEntry();
   return (cmdsActive > 0);
 }
*/
 public boolean hasEntry()
 {
    return(PtInputMsgQ.hasEntry());
 }

 public void waitForCmdPrompt()
 {
    /*
    String prompt = "";
    while (CmdPrompt.compareToIgnoreCase(prompt) != 0)
    {
         prompt = getCmd().trim();
         System.out.println("waitForCmdPrompt: '"+prompt+"'");
    }
    cmdsActive--;
    setChanged();
    notifyObservers(this);
    */

    while (cmdsActive > 0)
    {
       try { Thread.sleep(200); } catch(InterruptedException ie) { };
    }
 }

   public void run()
   {
     System.out.println("Starting Comm thread......");
     while(true)
     {
       String prompt = "";
       PatientTableEvent pte;
       Object cmdResponce;
       while (CmdPrompt.compareToIgnoreCase(prompt) != 0)
       {
         prompt = getCmd().trim();
         ptCmdResponce.add(prompt);
         System.out.println("Com Thread - waitForCmdPrompt: '"+prompt+"'");
       }
       if (ActiveEvents.size() > 0)
       {
          pte =  (PatientTableEvent) ActiveEvents.get(0);
          System.out.println("pte: "+pte.toString());
          ActiveEvents.remove(0);
          if (ptCmdResponce.size() > 0)
          {
            List resp = new ArrayList(ptCmdResponce);
            // pte.setData(ptCmdResponce.get(0));
            pte.setData(resp);
            ptCmdResponce.clear();
          }
          firePatientTableEvent(pte);
       }
       else
       {
          pte = null;
          ptCmdResponce.clear();
       }

       cmdsActive--;
       //setChanged();
       //notifyObservers(this);
    }
   }

   private void firePatientTableEvent(PatientTableEvent pte)
   {
     Vector t1;
     synchronized (this) {
        t1 = (Vector) PatientTableListeners.clone();
     }
     int size = t1.size();
     if (size == 0)
        return;

     for (int i = 0; i < size; ++i)
     {
        PatientTableListener ptl = (PatientTableListener) PatientTableListeners.get(i);
        int type = pte.getID();
        switch(type)
        {
          case PatientTableEvent.TABLE_EVENT_MOTION_UP:
              ptl.tableMotionUpCmplt(pte);
              break;

          case PatientTableEvent.TABLE_EVENT_MOTION_DOWN:
              ptl.tableMotionDownCmplt(pte);
              break;

          case PatientTableEvent.TABLE_EVENT_MOTION_IN:
              ptl.tableMotionInCmplt(pte);
              break;

          case PatientTableEvent.TABLE_EVENT_MOTION_OUT:
              ptl.tableMotionOutCmplt(pte);
              break;

          case PatientTableEvent.TABLE_EVENT_QWERY_CMPLT:
              ptl.tableMotionQweryCmplt(pte);
              break;
        }
     }
   }

   public synchronized void addPatientTableListener(PatientTableListener l)
   {
      if (PatientTableListeners.contains(l))
         return;
      PatientTableListeners.add(l);
   }

   public synchronized void removePatientTableListener(PatientTableListener l)
   {
      PatientTableListeners.remove(l);
   }

 public static void main(String[] args )
 {
    String answer;
    PatientTableComm ptc = new PatientTableComm(".","/dev/term/a");
   // buttonObserver bObserv = new buttonObserver();
   // ptc.addObserver(bObserv);
    //try { Thread.sleep(5000); } catch(InterruptedException itre) { };
    // Thread.yield();
    ptc.waitForCmdPrompt();
    // ptc.resetActive();
    /*
    while(ptc.hasEntry())  {
      answer = ptc.getCmd();
      System.out.println("Main: "+answer);
    }
    */
    ptc.moveY(10);
    //ptc.sendCmd("XY+10\n");
    ptc.waitForCmdPrompt();
   // answer = ptc.getCmd();
   // System.out.println("Main: "+answer);
   ptc.moveX(10);
   // ptc.sendCmd("XX+10\n");
    ptc.waitForCmdPrompt();
   // answer = ptc.getCmd();

   // System.out.println("Main: "+answer);
    while(true)
    {
      System.out.println("main: check entry.");
      try { Thread.sleep(750); } catch(InterruptedException itre) { };
      // Thread.yield();
      while(ptc.hasEntry())  {
      answer = ptc.getCmd();
      System.out.println("Main: "+answer);
      }
    }

 }

 private class buttonObserver implements java.util.Observer {
      public buttonObserver() { }
      public void update(java.util.Observable o, Object arg) {
        System.out.println("update called: "+o+" Arg: "+arg);
      }
  }


 private class PtInput implements Runnable
 {
   private Thread myThread;
   private InputStreamReader inputSR;
   private MsgQueue msgQ;
   private boolean cmdActive;

   public PtInput(MsgQueue msgq, InputStream is)
   {
       inputSR = new InputStreamReader(is);
       int tPrior;
       msgQ = msgq;
	    // OutputStreamWriter sw =
            //               new OutputStreamWriter(incoming.getOutputStream());
	    // BufferedReader br = new BufferedReader(sr);
	    // CmdMsge = br.readLine();
       myThread = new Thread(this);
       tPrior = myThread.getPriority();
       System.out.println("Priority Max: "+Thread.MAX_PRIORITY+", Min: "+Thread.MIN_PRIORITY);
       System.out.println("PtInput: Thread Priority: "+tPrior);
       myThread.setPriority(Thread.MAX_PRIORITY);
       myThread.setDaemon(true);
       myThread.start();
   }

   public void run()
   {
     String strg = "";
     try
     {
      BufferedReader bfr = new BufferedReader(inputSR);
      while(true)
      {
         strg = bfr.readLine();
         if (strg != null)
         {
            System.out.println("PtInput Thread Received: '"+strg+"'");
            msgQ.putmsg(strg);
         }
      }
     }
     catch (IOException e)
     {
         System.out.println(e);
     }
   }
 }

 private class PtOutput implements Runnable
 {
   private Thread myThread;
   private OutputStreamWriter outputSW;
   private MsgQueue msgQ;
   public PtOutput(MsgQueue msgq, OutputStream os)
   {
       int tPrior;
       outputSW = new OutputStreamWriter(os);
       msgQ = msgq;
       myThread = new Thread(this);
       tPrior = myThread.getPriority();
       System.out.println("PtOutput: Thread Priority: "+tPrior);
       myThread.setPriority(Thread.MAX_PRIORITY-2);
       myThread.setDaemon(true);
       myThread.start();
       tPrior = myThread.getPriority();
       System.out.println("PtOutput: Thread Priority: "+tPrior);
   }

   public void run()
   {
     String cmd = "";
     try
     {
      while(true)
      {
         cmd = (String) msgQ.getmsg();
         if (cmd != null)
         {
          System.out.println("PtOutput Thread: Send Cmd: '"+cmd+"'");
	      outputSW.write(cmd);
          outputSW.flush();
          // System.out.println("PtOutput: flushed buffer");
         }
      }
     }
     catch (IOException e)
     {
         System.out.println(e);
     }
   }
 }


// Example of a Two lock queue
// 1st lock is the Object itself  (e.g. synchronize)
// 2nd lock is a seperate Object lock (e.g. synchronize(tailLock)
// This allows both put and get to happen simitaniously
//
   private class MsgQueue {
      private Node head;
      private Node tail;
      private Object tailLock;
      //private long id = 0; // diagnostic ID (check for lost msges)
      //private long in = 0; // running count of msges received (diagnostic)
      //private long out = 0; // running count of msges gotten (diagnostic)

      public MsgQueue() {
         head = tail = new Node(null,null);
		 tailLock = new Object();  // synchronizing lock for msgput()
		// 'this' is the synchronizing lock for msgget()
      }

      public synchronized boolean hasEntry()
      {
        return((head.next != null));
      }

      public synchronized void putmsg(Object msg)
      {
         //id++;
         //in++;
         //Node node = new Node(msg,null,id);  // diagnostic Node
         Node node = new Node(msg,null);
         synchronized(tailLock) { // insert at end of queue
	      tail.next = node;
	      tail = node;
	 //notifyAll();		// unblock any one wait for a msg
	      notify();		// unblock any one wait for a msg
         }
      //System.out.println("putmsg: Id: "+node.id+", In: "+in+", Out: "+out+" Q'd: "+(in-out));
      }

      public synchronized Object getmsg()
      {
         Object x = null;
         awaitCond();      Node first = head.next; // first real node after head
         x = first.value;
         head = first;  // second is now first
         //out++;
         //System.out.println("getmsg: Id: "+first.id+",In: "+in+", Out: "+out+" Q'd: "+(in-out));
         return( x );
      }

     // good form is to encapsulate the waited condition
      protected synchronized void awaitCond()
      {
         while (head.next == null)
         {
			try
            {
	   	  wait();
			}
			catch (InterruptedException ex)
			{
                 System.out.println(ex);
			}
         }
      }

      private final class Node {
        Object value;
	    Node next;

	    Node(Object val, Node nxt)
	    {
	      value = val;
	      next = nxt;
	    }
    }
   }

}
