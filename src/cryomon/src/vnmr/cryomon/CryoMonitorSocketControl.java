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
 */
package vnmr.cryomon;


import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.text.ParsePosition;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;
import java.util.TreeMap;
import java.text.DecimalFormat;


import javax.swing.*;
import java.awt.*;

/**
 * This is the interface for communicating with the E5025 Cryogen Monitor.
 * There are three Thread inner classes:
 * <p>
 * Sender - takes commands from a list and sends them in order.
 * This is so the caller never has to wait while the E5025 is busy, it puts
 * the command in the queue and returns immediately.
 * <br>
 * Reader - continuously listens for responses from the E5025. When it gets
 * a complete response it calls processReply(what, value), which deals
 * with notifying anyone who needs to know about it.
 * <br>
 * StatusPoller - sends requests for status every so often.
 * <p>
 * The basic strategy to execute a command and is:
 * <br>
 * 
 * 
 */
public class CryoMonitorSocketControl implements CryoMonitorDefs, CryoMonitor {

    protected static final int CRYO_OK = 0;
    protected static final int CRYO_NIL = 1;
    
    protected static final int CRYO_UNRESPONSIVE = 2;

//    private static final int STATUS_UPDATE = 3600000; // 1 hr ms
    protected static final int WAITTIME = 2000; // ms
   
    // MISC
    
    /**
     * Protocol Function Codes
     */
    
    static private final int TEMPCONTROLLER = 403;
 
    private CryoMonitorSocket m_CryoSocket = null;
    private int m_retries =2;  // # of times to send command if no response
    public StatusPoller m_statusPoller = null;
    //private StatusManager m_statusManager;
    private Map<Integer, String> m_statusMessages
        = new HashMap<Integer, String>(100);
    private Map<Integer, String> m_pfcNames = new HashMap<Integer, String>(100);
    private Map<Integer, String> m_statusNames = new HashMap<Integer, String>(10);
    private Sender m_sender = null;
    private Reader m_reader = null;
    private Map<String, Boolean> m_resultTable
        = new TreeMap<String, Boolean>();
    protected String m_command;    // Last command sent
    protected int m_replyFailType = CRYO_NIL;
    //private CryoDiagnostics m_cryoDiagnostics;
    private CryoMonitorControls m_cryoPanel;
    private int m_port;
    private boolean m_write2Log = false;
    protected int statusUpdate=0;
    protected int dataUpdate=0;
    protected int initialDelay=2000;
    
    private Date last_date=null;
    private Date prev_read=null;
    private Date last_read=null;
    
    public static boolean debug = false;
        
    /**
     * List of possible Cryo commands and their Integer codes.
     */
    private static final Object[][] STATUS_CODES = {
        {new Integer(0), 	"IDLE"}, 
        {new Integer(1), 	"VACUUM PUMPING"}, 
        {new Integer(2), 	"COOLING"}, 
        {new Integer(3), 	"OPERATING"}, 
        {new Integer(4), 	"WARMING"}, 
        {new Integer(5), 	"FAULT"}, 
        {new Integer(6), 	"SAFE"},  
        {new Integer(7), 	"PURGE"},		//never used  
        {new Integer(8), 	"THERMAL CYCLE"},        
    };

    public CryoMonitorSocketControl(String host,
            int port, 
            String file,
     		int updateStatus,
    		int updateData
            ){ 
    	statusUpdate=updateStatus;
    	dataUpdate=updateData;
        	
     	populateStatusTable(STATUS_CODES);
    	populateMessageTable(1, "<GETSTATE>");
    	
    	m_port = port;
     	connect(host, port);
    }
    public void setCryoPanel(CryoMonitorControls cryopanel){
    	m_cryoPanel= cryopanel;
    }
    
    
    /* *****************Start of AutoSampler interface**************** */

    /**
     * Return the model number of the cryobay.
     */

    public String getName(){
        return "CRYO";
    }
    
    /**
     * Return a Date object from a " " or ":" separated date string
     * String format: yy:MM:dd:kk:mm[:ss] or yy MM dd kk mm[ ss] 
     *                seconds field (ss) is ignored
     * @param s
     * @return
     */
    public static Date getDate(String s){
    	//System.out.println(s);
    	try {
	   		StringTokenizer dateToker = new StringTokenizer(s, " :");
	   		if(!dateToker.hasMoreTokens())
	   			return null;
	   		String	year=dateToker.nextToken();
	   		if(!dateToker.hasMoreTokens())
	   			return null;
	   		String	month=dateToker.nextToken();
	   		String	day=dateToker.nextToken();
	   		String	hour=dateToker.nextToken();
	   		String	min=dateToker.nextToken();
	   		//String	sec=dateToker.nextToken();
	    	String time=year+month+day+hour+min;
			SimpleDateFormat f = new SimpleDateFormat("yyMMddHHmm");
			ParsePosition pos=new ParsePosition(0);
			Date date=f.parse(time,pos);
			return date;
    	}
    	catch (Exception e){
    		System.out.println("Date format error:"+e);
    		return null;
    	}
	}
    /** Open communication with the cryobay. */
    public void connect(String host, int port){
        m_CryoSocket = new CryoMonitorSocket(host, port); // startup delay is here
        m_reader = new Reader();
        m_reader.start();
    }

    public void pause(boolean b) {
        if (m_statusPoller !=null)
            m_statusPoller.pause(b);
    }

    public boolean paused() {
        if (m_statusPoller !=null)
            return m_statusPoller.paused();
        return false;
    }

    public void pollStatus() {
        if (statusUpdate > 0){
            m_statusPoller = new StatusPoller(statusUpdate);
            m_statusPoller.start();
        } 
    }

    public boolean wait(int time){
        try{
            Thread.sleep(time);
        } catch (Exception e){
            System.err.println("Cannot sleep");
            return false;
        } 
        return true;
    }
    
    public boolean isConnected() {
        return m_CryoSocket != null && m_CryoSocket.isConnected();
    }

    /** Shut down communication with the autosampler. */
    public void disconnect() {
        if (m_reader != null) {
            m_reader.quit();
            m_reader.interrupt();
        }
        if (m_sender != null) {
            m_sender.quit();
        	m_sender.interrupt();
        }
    	if(m_statusPoller!=null){
    		m_statusPoller.quit();
    		m_statusPoller.interrupt();
    	}
        if (m_CryoSocket != null) {
             m_CryoSocket.disconnect();
             m_CryoSocket=null;
        }
    }
 

    /**
     * Called whenever a full reply has been received from the 430.
     * Also called with value=-1 if a timeout occurs waiting for a
     * reply or if a garbled reply is received.
     * @param what The parameter that the value refers to.  For one
     * byte replies (ACK, NACK, or NACK0) the command that produced
     * this reply.
     * @param value The value of the "what" parameter.  For one byte
     * replies, the <i>negated</i> value of that byte as an integer.
     */
    protected void processReply(String what, boolean value) {
        if (SwingUtilities.isEventDispatchThread()) {
            processReplyInEventThread(what, value);
        } else {
            SwingUtilities.invokeLater(new ReplyProcessor(what, value));
        }
    }

    /**
     * Does the work for processReply.
     * @see #processReply
     */
    private void processReplyInEventThread(String what, boolean value) {
    	
//        Messages.postDebug("cryocmd", "CryoSocketControl.processReply("
//                           + what + ", " + value + ")");
    	
        if(debug)
        	System.out.println("CryoSocketControl.gotReply: " + what);
    	if(what!=null){
    		m_sender.gotReply(value);
    		processStatus(what, value);   		
    	}
    }

    public boolean sendCommand(String command) {
        if(debug)
        	System.out.println("CryoSocketControl.sendCommand: " + command);
        //Messages.postDebug("cryocmd",
        //                   "CryoSocketControl.sendCommand: " + command );
        boolean result=sendToCryoBay(command);
        return result ;
    }
    
    /**
     * Send a command to the cryobay.
     * @param commandMessage The string to send to the cryobay.
     * @return True if there is a connection to the cryobay
     */
    synchronized public boolean sendToCryoBay(String command) {
        if (m_sender == null) {
            m_sender = new Sender(null, m_retries, this);
            m_sender.start();
        }
        if (isConnected()) {
            m_sender.addCommand(command);
            // TODO: (Java 5) Release semaphore here
            //m_sender.interrupt(); // Wake up the run() method
            return true;
        } else {
            return false;
        }
    }
    
    /** send String directly to the cryobay
     * @param cmd as a string
     * @return boolean if sent correctly
     */
    public boolean send(String cmd) {
    	
        if (this.isConnected()) {
            if(debug)
            	System.out.println("CryoSocketControl.send: " + cmd);

            //Messages.postDebug("cryocmd",
            //                   "CryoSocketControl.send(\"" + cmd + "\")");
            m_CryoSocket.write(cmd.getBytes());
            return true;
        } else {
            return false;
        }
    }
    
    /**
     * Send the byte array to the autosampler.
     * @param par as byte array.
     * @return true if successful.
     */
    public boolean send(byte[] par) {
        if (this.isConnected()) {
            m_CryoSocket.write(par);
            //Messages.postDebug("cryocmd",
            //                   "Sent to CryoBay: " + arrayToHexString(par));
            return true;
        } else {
            return false;
        }
    }

    /**
     * This method is called whenever a new entry is written to the data file.
     * @param line is a text string containing fill data information
     */
    protected void writeDataToFile(String line) {
    	ArrayList<DataObject> array=DataFileManager.readFillData();
    	DataObject next=new DataObject(line);
		array.add(next);
		DataFileManager.writeFillData(array);
    }
    
    protected void writeDataToFile(Date date, String heLevel, String n2Level, int errors) {
    	SimpleDateFormat fmt = new SimpleDateFormat("yy:MM:dd:HH:mm");
		String time=fmt.format(date);
		if(prev_read != null && last_read.getTime()!=prev_read.getTime())
			errors |=HE_NEW_READ;
		writeDataToFile(time+","+heLevel + "," + n2Level +","+ errors +"\n");
		m_cryoPanel.updateUI();
    }
    /**
     * This method is called whenever a status message is received from
     * the Cryogen Monitor.
     * @param iWhat An integer coding what the value is for (status=152).
     * @param iValue Is true.
     */
    protected void processStatus(String iWhat, boolean iValue) {
    	StringTokenizer toker, dateToker;
    	String elem, elem2;
		int errors= 0;
		String time=" ";
		String heLevel="000";
		String n2Level="000";
		//System.out.println("received:"+iWhat);
    	
		iWhat=iWhat.replace("Connected to E5025", "");
		 
        if (!iValue) {
        	if(m_cryoPanel!= null) {
        		m_cryoPanel.cryomonNotCommunicating();
        	}
            return;
        }
        //String key = new String(iWhat);
        if(iWhat==null)
        	return;
		if(m_cryoPanel!= null)
    		m_cryoPanel.writeAdvanced("Received: " + iWhat + "\n");

        toker = new StringTokenizer(iWhat, ",");
       
        if (iWhat.endsWith("Z*")) {
        	//m_statusPoller.clrDataCount();
        } else if (iWhat.endsWith("NH*")) {
        	//m_statusPoller.clrDataCount();
        } else if (iWhat.endsWith("EH*")) {
    		if(m_cryoPanel!= null) {
        		//String msg=iWhat.replace("Connected", "");
        		m_cryoPanel.setErrors(iWhat);
        	}
        } else if (iWhat.endsWith(" t*")) {
        	//String s=iWhat.replace("Connected","");
        	Date date=getDate(iWhat);
        	last_date=date;
        	if(m_cryoPanel!= null)
        		m_cryoPanel.setTime(iWhat);
    	} else if (iWhat.startsWith("V")) {
        	if(m_cryoPanel!= null) {
        		String msg=iWhat.replace("*","");
    		//System.out.println("CryoSocketControl version: " + msg);
        		DataFileManager.setFirmwareVersion(msg);
        	}
        } else if(iWhat.startsWith("Date")){
        	m_cryoPanel.writeAdvanced("Reading Log: " + iWhat + "\n");
        	//open file & read rest of lines into file
        	String logFile= m_cryoPanel.getLogFile();
        	try { 
        		m_write2Log= true;
        		pause(true);
        		//System.out.println("Opening log file " + logFile);
        		FileWriter outLog = new FileWriter(logFile, false);	//open file and do not append data
        		//System.out.println("Writing log file " + logFile);
        		outLog.write(iWhat + "\n");
        		outLog.flush();
        	} catch (Exception e){
        		//System.out.println("Could not write log file.");
        	}
        } else if (iWhat.endsWith("W*")) {
        	if(m_cryoPanel!= null){
        		errors=0;
        		heLevel="000";
        		n2Level="000";
        		while (toker.hasMoreTokens()){
        			elem = toker.nextToken();
        			if(elem.startsWith("E5")||elem.startsWith("E6")){
        	        	if(m_cryoPanel!= null)
        	        		DataFileManager.setMonitorType(elem);        				
        			}else if(elem.startsWith("ND")){
                		elem= elem.substring(2,5);
                		m_cryoPanel.writeAdvanced("N2 level: " + elem + "\n");
                		n2Level= elem;
            		} else if(elem.startsWith("HL")){
                		elem= elem.substring(2,6);
        				try {
        					int HL=Integer.parseInt(elem);
                 		    DataFileManager.HL=HL;
                 		    m_cryoPanel.writeAdvanced("HL level: " + HL + "\n");
        				}
        				catch (Exception e){
        					System.out.println("Input format error in HL");
        				}

                	} else if(elem.startsWith("SA")){
                		errors= Integer.parseInt(elem.substring(2));
            		} else if(elem.startsWith("SB")){
                		int sb=Integer.parseInt(elem.substring(2));
                		errors+=sb<<8;
                		//m_cryoPanel.setStatus(errors);
            		} else if(elem.startsWith("P1D")){
            			elem2= toker.nextToken();	//get P2D for second probe
            			if(elem.substring(8,12).startsWith("+")){
            				elem= elem.substring(9, 12);
            			} else{
            				elem= elem.substring(8, 12);
            			}
            			if(elem2.substring(8,12).startsWith("+")){
            				elem2= elem2.substring(9, 12);
            			} else{
            				elem2= elem2.substring(8, 12);
            			}
            			m_cryoPanel.writeAdvanced("He levelP1: " + elem + " levelP2: " + elem2 + "\n");
            			if(elem.startsWith("#") && elem2.startsWith("#")) // no probes enabled
            				heLevel="000";
            			else if(elem.startsWith("#"))
            				heLevel=elem2;
            			else if(elem2.startsWith("#"))
            				heLevel=elem;
            			else{  // both probes enabled
            				heLevel=elem;
            				try {
            					int v1=Integer.parseInt(elem);
            					int v2=Integer.parseInt(elem2);
            					int max_val=v1>v2?v1:v2;
            					DecimalFormat myFormatter = new DecimalFormat("000");
            					String output = myFormatter.format(max_val);
            					//System.out.println("He1:"+elem+" He2:"+elem2+" ave:"+output);
            					heLevel = output;
            				}
            				catch (Exception e){
            					System.out.println("Input format error in He level");
            				}
            			}
            		} else if(elem.startsWith("t")){
            			time=elem.replace("t", "");
            			dateToker = new StringTokenizer(time, " ");
	            		if(dateToker.hasMoreTokens()){
	            			prev_read=last_read;
	            			last_read=getDate(time);
            			}
            		} 
        		}
        		if(last_date !=null){
        			writeDataToFile(last_date,heLevel,n2Level,errors);
        		}
        	} 
        } else if(m_write2Log){
    		if(iWhat.startsWith("*")){
    			//close file
    			m_cryoPanel.popupMsg("Log File", "Cryogen Monitor log written", false);
    			m_cryoPanel.writeAdvanced("Reading Log Done.\n");
	        	m_write2Log= false;
	    		pause(false);
	        	m_cryoPanel.setReadLogFlag(false);
    		} else {
    	        String logFile= m_cryoPanel.getLogFile();
    	        try { 
    	        	FileWriter outLog = new FileWriter(logFile, true);	//open file and append data
    	        	outLog.write(iWhat + "\n");
    	        	outLog.flush();
    	        } catch (Exception e){
    	        	//System.out.println("Could not write log file.");
    	        }
    		}
        } 
    }
    
    public boolean getReadLogFlag(){
    	if(m_cryoPanel==null)
    		return false;
    	return m_cryoPanel.getReadLogFlag();
    }
    
    private void populateMessageTable(int type, String entries) {
        int size = entries.length();
        for (int i = 0; i < size; i++) {
            //int key = 10000 * type + ((Integer)entries[i][0]).intValue();
            m_statusMessages.put(type, (String)entries);
        }
    }
    
    private void populateFunctionTable(Object[][] entries) {
        int size = entries.length;
        for (int i = 0; i < size; i++) {
            int key = ((Integer)entries[i][0]).intValue();
            m_pfcNames.put((Integer)entries[i][0], (String)entries[i][1]);
        }
    }

    
    private void populateStatusTable(Object[][] entries) {
        int size = entries.length;
        for (int i = 0; i < size; i++) {
            int key = ((Integer)entries[i][0]).intValue();
            m_statusNames.put((Integer)entries[i][0], (String)entries[i][1]);
        }
    }
      
    /**
     * Blocks until a message is received from the Cryobay with the
     * PFC set to "key".
     * @return The value in the response.
     */
    protected boolean waitForConfirmation(int key) {
        boolean result = false;
        while (!result) {
            boolean nResult = m_resultTable.remove(key);
            /*if (nResult == null) {
                // TODO: (Java 5) Use semaphore
                try {
                    Thread.sleep(50);
                } catch (InterruptedException ie) {
                }
            } else {*/
                result = nResult;
           // }
        }
        return result;
    }

    /**
     * Sets the confirmation value for a given key.
     * Overrides any previously set value for this key.
     * @param key The key.
     * @param value The value to set.
     */
    protected void setConfirmation(String key, boolean value) {
        m_resultTable.put(new String(key), new Boolean(value));
    }

    /**
     * Clears out any old confirmations for the given key.
     * @param key The key to remove.
     */
    protected void clearConfirmation(String key) {
        m_resultTable.remove(new String(key));
    }


    /**
     * Interprets a reply. Done as a task in the Event Thread.
     */
    class ReplyProcessor implements Runnable {
        String mm_what;
        boolean mm_value;

        ReplyProcessor(String what, boolean value) {
            mm_what = what;
            mm_value = value;
        }

        public void run() {
            processReplyInEventThread(mm_what, mm_value);
        }
    }

    /**
     * This thread listens for messages from the socket port.
     */
    class Sender extends Thread {

        private byte[] mm_commandMessage;
        private int mm_numTries;
        private int mm_numTry;
        private CryoMonitor mm_asListener;
        private List<String> mm_commandList;
        private boolean mm_quit = false;
        private boolean mm_readyForCommand = true;
        private Timer mm_replyTimer;
        private String mm_rtnMsg = new String();
        private boolean mm_rtnFlag = false;
       // private static final int REPLY_TIMEOUT = 600000; // ms
        private static final int REPLY_TIMEOUT = 60000; // 60 second retry delay
        private static final int COMMAND_DELAY = 200;   // time delay between commands


        public Sender(String command, int numTries,
                      CryoMonitor asListener) {

            setName("CryogenMonitorControl.Sender");

            //mm_commandMessage = commandMessage;
            mm_numTries = numTries;
            mm_asListener = asListener;
            m_command = command;
            mm_commandList = new ArrayList<String>();
            ActionListener replyTimeout = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    //Messages.postDebug("CryoSocketControl.replyTimeout");
                    checkForReply();
                }
            };
            mm_replyTimer = new Timer(REPLY_TIMEOUT, replyTimeout);
        }

        public void run() {

            while (!mm_quit) {
                if (mm_commandList.size() == 0 || !mm_readyForCommand) {
                    // TODO: (Java 5) Use java.util.concurrent.Semaphore
                    try {
                        Thread.sleep(COMMAND_DELAY);
                    } catch (InterruptedException ie) {
                     }
                } else if (mm_readyForCommand) {
                    execNextCommand();
                }
            }
            mm_readyForCommand=false;
            mm_replyTimer.stop();
        }

        public void quit() {
            mm_quit = true;
            mm_replyTimer.stop();
        }

        public void addCommand(String cmd) {
            String cmdMessage;
            cmdMessage = new String(cmd);
            mm_commandList.add(cmd);
        }

        public void gotReply(boolean value) {
            mm_replyTimer.stop();
            if (value != false && m_replyFailType != CRYO_OK) {
                //Messages.postInfo("Cryobay connection OK");
            	if(m_cryoPanel!=null)
            		m_cryoPanel.setConnected(true);
            		//m_cryoPanel.txtInfo.append("Cryobay connection OK\n");
                m_replyFailType = CRYO_OK;
            }
            else if (value == false){
            }
            mm_readyForCommand = true;
            // TODO: (Java 5) Release semaphore here
            //interrupt();        // Wake up the run() method
        }

        private void execNextCommand() {
            String cmdMessage = mm_commandList.remove(0);
            if (cmdMessage != null) {
                m_command = (cmdMessage);
                //mm_commandMessage = (byte[])cmdMessage[1];
                //Messages.postDebug("cryocmd",
                //                   "Executing next cmd= " + m_command);
                //System.out.println("Executing next command: " + m_command);
                mm_numTry = 1;
                mm_rtnMsg = m_command;
                mm_rtnFlag = false;
                mm_readyForCommand = false;
                send(m_command);
                mm_replyTimer.start();
            }
        }

        private synchronized void checkForReply() {
            mm_replyTimer.stop();
            if (!mm_rtnFlag) {
                if (mm_numTry >= mm_numTries) {
                    // Hard failure
                    if (m_replyFailType != CRYO_UNRESPONSIVE) {
                        if(m_port==TEMPCONTROLLER){ 
                        	//SDM FIXME if(m_cryoPanel!= null) m_cryoPanel.txtrFault.setText("Temp Controller is not responding");
                    	}else {
                        	DataFileManager.writeMsgLog("Cryomon is not responding after "+mm_numTries+" retries", debug);

                    		//System.out.println("Cryomon is not responding after "+mm_numTries+" retries");
                    		//SDM FIXME if(m_cryoPanel!= null) m_cryoPanel.txtrFault.setText("CryoBay is not responding");
                    		//SDM FIXME m_cryoPanel.writeInfo("\nCryobay is not responding");
                        }
                        m_replyFailType = CRYO_UNRESPONSIVE;
                    }
                    processReply(mm_rtnMsg, false);
                } else {
                    // Try again
                    //Messages.postDebug("  ... " + m_port + " resending command to Cryobay" + m_command);
                    mm_numTry++;
                	//DataFileManager.writeMsgLog("Sender retry:" + mm_numTry, debug);

                    //if(debug)
                    //	System.out.println("Sender: "+m_command+" retry:" + mm_numTry);
                    send(m_command);
                    mm_replyTimer.start();
                }
            } else {
                // Got the reply
                //mm_asListener.newMessage(mm_rtnMsg);
                if (m_replyFailType != CRYO_OK) {
                    if(m_port==TEMPCONTROLLER){ 
                    	//Messages.postInfo("Temp Controller connection OK");
                    	if(m_cryoPanel!=null) 
                    		//m_cryoPanel.txtInfo.append("Temp Controller connection OK\n");
                    		m_cryoPanel.writeInfo("\nTemp Controller connection OK");
                    } else { 
                    	//Messages.postInfo("Cryogen Monitor connection OK");
                    	if(m_cryoPanel!=null) 
                    		//m_cryoPanel.txtInfo.append("CryoBay connection OK\n");
                    		m_cryoPanel.writeInfo("\nCryogen Monitor connection OK\n");
                    }
                    m_replyFailType = CRYO_OK;
                }
                processReply(mm_rtnMsg, mm_rtnFlag);
            }
        }
    }        // End of Sender class


    /**
     * This thread monitors the input from the AS430 and assembles replies.
     * At the end of each reply calls processReply(int what, int value).
     * The following characters mark the beginning of a reply:
     * STX, ACK, NACK, NACK0.
     * These characters mark the end of a reply:
     * ETX, ACK, NACK, NACK0.
     */
    class Reader extends Thread {
       // CryoMonitorSocket mm_socket;
        boolean m_quit = false;

        public Reader() {
            setName("CryoMonitorSocketControl.Reader");
            //mm_socket = socket;
        }

        public synchronized void quit() {
            m_quit = true;
        }

        public void run() {
            final int BUFLEN = 500; 
            char[] buf = new char[BUFLEN];
            int idx = 0;        // Which byte we're waiting for in buffer

            while (!m_quit) {
                try {
                	int ch = m_CryoSocket.read();
                    //int ch = mm_socket.read();
                    buf[idx++] = (char) ch;
                    // Check for end of message
                    //SDM TESTING if ( ((char) ch == (char) ('*')) || ( ((char) ch == (char) ('\n'))) && (m_write2Log) ) {
                    if ( ((char) ch == (char) ('*')) || ( ((char) ch == (char) ('\n'))) && getReadLogFlag()  ) {
                            idx = 0; // Ready for next message
                            processReply(String.valueOf(buf).trim(), true);
                            buf= null;
                            buf = new char[BUFLEN];
                    } else if (idx == BUFLEN) {
                        // Message is too long
                        //Messages.postDebug("cryocmd", "CryoSocketControl: No ETX received at "
                        //                   + idx + " bytes");
                        idx = 0; // Wait for a new message*/
                    }
                } catch (IOException ioe) {
                    // Exit the thread
                	//DataFileManager.writeMsgLog("Reader exception: " + ioe, debug);
                	//System.out.println("CryoSocketControl.Reader exception: " + ioe);
                    m_quit = true;
                }
            }
        }
    }
                        

    /**
     * This thread just sends status requests every so often.
     */
    class StatusPoller extends Thread {
        private boolean m_quit = false;
        private int m_rate_ms;
        private boolean m_paused = false;
        protected int dataCount=0;

        public StatusPoller(int period_s) {
            setName("CryoMonitorSocketControl.StatusPoller");
            m_rate_ms = period_s*1000;
        }

        public void pause(boolean b) {
        	//System.out.println("pause:"+b);
        	m_paused=b;
        }
        public boolean paused() {
        	return m_paused;
        }

        public synchronized void clrDataCount(){
        	dataCount=0;
        }
        public void quit() {
        	//Messages.postDebug("cryocmd", "QUITTING status poller");
            m_quit = true;
        }

		public synchronized void run() {
			while (!m_quit) {
				// System.out.println("Sending statuspoller W!! ");
				try {
					if (m_cryoPanel != null && !m_paused) {
						if (initialDelay > 0) {
							Thread.sleep(initialDelay);
							initialDelay=0;
						} 
						if (dataCount == 0) {
							dataCount++;
							sendCommand(CMD_CLOCK);
							sendCommand(CMD_PARAMS);
						} else {
							dataCount++;
							sendCommand(CMD_CLOCK);
						}
						if (dataCount == dataUpdate)
							dataCount = 0;
						
						Thread.sleep(m_rate_ms);
					}
				} catch (InterruptedException ie) {
				}
			}
		}
    }

}
