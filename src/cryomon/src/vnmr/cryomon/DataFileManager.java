/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.cryomon;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.StringTokenizer;

/**
 * storage class for fill history data
 */
public class DataFileManager implements CryoMonitorDefs {
	
	public static String dataFile;
	public static String lockFile;
	public static String prefsFile;
	public static String logFile;
	public static String msgFile;
	public static String versionFile;
	public static String tmpFile;
	public static String monitorType="E5025";
	public static String firmwareVersion="V3";
    public static boolean debug = false;
 
	public static double levelValue[][];
	public static int HL=50;
	protected static final long CONNECT_WAIT    = 2000;     // minimum time to wait between connection attempts

	public static synchronized void init(String DataDir){
		dataFile = DataDir + File.separator + "cryomonData.txt";
		lockFile = DataDir + File.separator + "cryomon.lock";
		prefsFile = DataDir + File.separator + "cryomon.prefs";
		logFile = DataDir + File.separator + "cryomon.log";
		msgFile = DataDir + File.separator + "cryomon.msg";
		tmpFile = DataDir + File.separator + "cryomon.tmp";
		versionFile = DataDir + File.separator + "cryomon.version";
		
	   	levelValue=new double[2][3];
    	
		levelValue[NITROGEN][SEL_WARN]=N2_WARN_LEVEL;
    	levelValue[NITROGEN][SEL_FILL]=N2_FILL_LEVEL;
    	levelValue[NITROGEN][SEL_ALERT]=N2_ALERT_LEVEL;
		levelValue[HELIUM][SEL_WARN]=HE_WARN_LEVEL;
    	levelValue[HELIUM][SEL_FILL]=HE_FILL_LEVEL;
    	levelValue[HELIUM][SEL_ALERT]=HE_ALERT_LEVEL;

	}
    public static String getDateString(){
    	Calendar rightNow = Calendar.getInstance();
		Date today=rightNow.getTime();
		SimpleDateFormat fmt=new SimpleDateFormat("MM/dd/yy HH:mm:ss");
		return fmt.format(today);  	
    }

	public static synchronized void writeTmpFile(){
		try { 
			String line=getLockID();
	   		FileWriter writer=new FileWriter(tmpFile, false);
			writer.write(line+"\n");
			//if(debug)
			//	System.out.println(line);
			writer.flush();
			writer.close();

		}
    	catch (IOException e) {
    		System.err.println("DataFileManager:: error " + e + msgFile);
    	}		
	}

	/**
	 * Write an entry to the message log
	 * @param msg   message to add
	 * @param echo  print message to command line if true
	 */
	public static synchronized void writeMsgLog(String msg,boolean echo){
		try { 
			String date=getDateString();
			String proc=getLockID();
			String line="";
			if(proc!=null)
				line=proc+" ";
			line=line+date+" "+msg;
	   		FileWriter writer=new FileWriter(msgFile, true);
			writer.write(line+"\n");
			if(echo)
				System.out.println(line);
			writer.flush();
			writer.close();

		}
    	catch (IOException e) {
    		System.out.println("DataFileManager:: error " + e + msgFile);
    	}		
	}

	/**
	 * Wait a minimum time after socket was last closed
	 */
	public static void waitReconnectTime(){
		long time_unlocked=0;
		//if(DataFileManager.isLocked())
		time_unlocked=DataFileManager.lastModified();
			
		if (time_unlocked>0){
		    Calendar now = Calendar.getInstance();
		    long time_now=now.getTimeInMillis();
		    long delta_time=(time_now-time_unlocked);
		    if(debug)
		    	System.out.println("time since last locked:"+delta_time/1000.0);
		    if(delta_time<CONNECT_WAIT){
		    	long wait_time=CONNECT_WAIT-delta_time;
		       	//DataFileManager.writeMsgLog("waiting "+wait_time/1000.0+" seconds before connecting", debug);   	
				try {
					Thread.sleep(wait_time);
				}
				catch (Exception e){
					System.out.println("could not sleep:"+e);
				}
		    }				
		}
    }

	 /**
     * Create a fill history file from an array of DataObjects
     * @param file
     * @param array
     */
    public static synchronized void writeFillData(String path, ArrayList<DataObject> array){
       	try { 
       		FileWriter writer=new FileWriter(path, false);	//open file and clear data
			String line= new String("DATE, HE, N2, STATUS"+"\n");
			writer.write(line);
			for(int i=0;i<array.size();i++){
				writer.write(array.get(i).getString()+"\n");
			}
			writer.flush();
			writer.close();
       	}
    	catch (IOException e) {
    		System.out.println("DataFileManager:: could not write data file " + e + dataFile);
    	}
    }

    public static synchronized void writeFillData(ArrayList<DataObject> array){
    	writeFillData(dataFile,array);
    }
    /**
     * This method is called whenever a new entry is written to the data file.
     * @param line is a text string containing fill data information
     */
	public static synchronized void writeDataToFile(String line) {
    	ArrayList<DataObject> array=readFillData();
    	DataObject next=new DataObject(line);
		array.add(next);
		writeFillData(array);
    }
    
    
    /**
     * Clear data fill history file
     * @param file
     */
    public static synchronized void clearFillData(){
    	ArrayList<DataObject> array=readFillData();
    	int n = array.size();
    	if(n==0)
    		return;
     	DataObject last=array.get(n-1);
     	array.clear();
     	array.add(last);
     	writeFillData(array);
    }

    /**
     * Clear message log file
    */
    public static synchronized void clearMsgs(){
    	File file=new File(msgFile);
    	if(file.canWrite()){
    		file.delete();
    		writeMsgLog("Message Log Cleared",debug);
     	}
    }

    /**
     * Reads a fill history file and returns a list of DataObjects.
     */
    public static synchronized ArrayList<DataObject> readFillData() {
	    ArrayList<DataObject> array = new ArrayList<DataObject>();
        try{
        	FileInputStream fin = new FileInputStream(dataFile);
        	BufferedReader dataReader = new BufferedReader
        		(new InputStreamReader(fin));
        	
           	String line= dataReader.readLine();	//throw away first line
           	while( (line= dataReader.readLine())!=null){
           		if(line.length()>2)
           		array.add(new DataObject(line));
           	}
        	fin.close();
  
        } catch(Exception e){
        }
       	return array;
    }

    /**
     * Reads a log file and returns a list of DataObjects.
     */
    public static synchronized ArrayList<DataObject> readLogData(String logPath) {
	    ArrayList<DataObject> array = new ArrayList<DataObject>();
        try{
        	FileInputStream fin = new FileInputStream(logPath);
        	BufferedReader dataReader = new BufferedReader
        		(new InputStreamReader(fin));
        	
           	String line= dataReader.readLine();	//throw away first line
           	while( (line= dataReader.readLine())!=null){
           		StringTokenizer tok=new StringTokenizer(line);
           		if(!tok.hasMoreElements())
           			continue; // sometimes get an empty line
           		String date=tok.nextToken();
           		if(!tok.hasMoreElements())
           			continue; // sometimes get '*'
           		String time=tok.nextToken();
           		SimpleDateFormat fmt=new SimpleDateFormat("d/M/yy HH:mm:ss");
           		Date ldate=fmt.parse(date+" "+time);
           		SimpleDateFormat fmt2=new SimpleDateFormat("yy:MM:dd:HH:mm");
           		time=fmt2.format(ldate);
           		String hlvl1=tok.nextToken();
           		String hlvl2=tok.nextToken();
           		tok.nextToken(); // skip
           		tok.nextToken(); // skip
           		String hlen=tok.nextToken();
           		Integer a=Integer.valueOf(hlvl1);
           		Integer b=Integer.valueOf(hlvl2);
           		Integer l=Integer.valueOf(hlen)-50; // in V15 firmware at least 50 is subtracted from returned length ??
           		int h=(100*(Math.max(a, b)-HL))/(l-HL); // subtract off 50 mm heater length
            	//int h=(100*Math.max(a, b))/(l-HL); // subtract off 50 mm heater length
          		//h=Math.min(h,100);
           		tok.nextToken(); // skip
           		tok.nextToken(); // skip
           		tok.nextToken(); // skip
           		String nlvl=tok.nextToken();
           		tok.nextToken(); // skip
           		tok.nextToken(); // skip
           		tok.nextToken(); // skip
           		String temp=tok.nextToken(); //  temperature
           		String sba=tok.nextToken();
           		String sbb=tok.nextToken();
           		a=Integer.valueOf(sba);
           		b=Integer.valueOf(sbb);
           		int errors=a+(b<<8);
           		errors|=HE_NEW_READ;
           		String str=time+","+h+","+nlvl+","+errors;
           		//System.out.println(str);
           		DataObject obj=new DataObject(str);
           		array.add(obj);
           	}
        	fin.close();
  
        } catch(Exception e){
        	e.printStackTrace();
        }
       	return array;
    }

    /**
     * Read user preferences file
     */
    public static synchronized void readPrefs() {
		BufferedReader in;
		String line;
		String symbol;
		String value;
		StringTokenizer tok;
		try {
			in = new BufferedReader(new FileReader(prefsFile));
			while ((line = in.readLine()) != null) {
				if(line.startsWith("#"))
					continue;
				tok = new StringTokenizer(line, " \t");
				if (!tok.hasMoreTokens())
					continue;				
				symbol = tok.nextToken();
				if (!tok.hasMoreTokens())
					continue;
				value = tok.nextToken();
				if (symbol.startsWith("Level")) {
					double dval = Double.parseDouble(value);
					if (symbol.equals("LevelN2Fill"))
						levelValue[NITROGEN][SEL_FILL] = dval;
					else if (symbol.equals("LevelN2Fill"))
						levelValue[NITROGEN][SEL_FILL] = dval;
					else if (symbol.equals("LevelHeFill"))
						levelValue[HELIUM][SEL_FILL] = dval;
					else if (symbol.equals("LevelN2Warn"))
						levelValue[NITROGEN][SEL_WARN] = dval;
					else if (symbol.equals("LevelHeWarn"))
						levelValue[HELIUM][SEL_WARN] = dval;
					else if (symbol.equals("LevelN2Alert"))
						levelValue[NITROGEN][SEL_ALERT] = dval;
					else if (symbol.equals("LevelHeAlert"))
						levelValue[HELIUM][SEL_ALERT] = dval;
				}
			}
			in.close();
		} catch (Exception e) {
			// Messages.writeStackTrace(e);
		}
	}
    /**
     * Write user preferences file
     */
    public static synchronized void writePrefs(){
       	try { 
       		FileWriter writer=new FileWriter(prefsFile, false);	//open file and clear data
			writer.write("LevelN2Fill "+levelValue[NITROGEN][SEL_FILL]+"\n");
			writer.write("LevelHeFill "+levelValue[HELIUM][SEL_FILL]+"\n");
			writer.write("LevelN2Warn "+levelValue[NITROGEN][SEL_WARN]+"\n");
			writer.write("LevelHeWarn "+levelValue[HELIUM][SEL_WARN]+"\n");
			writer.write("LevelN2Alert "+levelValue[NITROGEN][SEL_ALERT]+"\n");
			writer.write("LevelHeAlert "+levelValue[HELIUM][SEL_ALERT]+"\n");
			writer.flush();
			writer.close();
       	}
    	catch (IOException e) {
     		System.out.println("CryoMonitor:: could not write prefs file: " + prefsFile);
    	}
    }

    private static boolean writeVersionFile(){
     	//File file=new File(versionFile);
    	try {
    		//if(file.exists())
    		//	return false;
    		FileWriter writer=new FileWriter(versionFile, false);	//open file and clear data
    		writer.write(monitorType+"-"+firmwareVersion+"\n"); // write process id   			
 			writer.flush();
			writer.close();
       		return true;   	
    	}
    	catch (Exception e){
    		System.out.println("could not write :"+versionFile);
    		return false;
    	}
    }

    public static void setFirmwareVersion(String name){
    	firmwareVersion=name;
    	writeVersionFile();
    }

    public static void setMonitorType(String name){
    	monitorType=name;
    }

    /**
     * @return version string
     */
    public static synchronized String getMonitorType(){
    	File file=new File(versionFile);
    	if(!file.exists())
    		return null;
        try{
        	FileInputStream fin = new FileInputStream(versionFile);
        	BufferedReader dataReader = new BufferedReader
        		(new InputStreamReader(fin));
        	
           	String line= dataReader.readLine();	//get first line
        	fin.close();
        	return line;
  
        } catch(Exception e){
        	System.out.println("Could not read version file");
        }
        return null;  	
    }

    /**
     * @param s  pid string of new server process
     * @return true if lock file was written
     */
    public static synchronized boolean setLock(String s){
     	File lock=new File(lockFile);
    	try {
    		if(lock.exists())
    			return false;
    		FileWriter writer=new FileWriter(lockFile, false);	//open file and clear data
    		writer.write(s+"\n"); // write process id   			
 			writer.flush();
			writer.close();
        	DataFileManager.writeMsgLog("setLock "+s, debug);
       		return true;   	
    	}
    	catch (Exception e){
    		System.out.println("could not write lockfile:"+lockFile);
    		return false;
    	}
    }
    
    /**
     * Remove lock file 
     * - Save tmp file for lastModified test
     */
    public static synchronized void removeLock(){
    	File lock=new File(lockFile);
    	if(lock.canWrite()){
        	DataFileManager.writeMsgLog("removeLock ", debug);
        	writeTmpFile();
    		lock.delete();
     	}
    }
    /**
     * @return true if lock file exists
     */
    public static synchronized boolean isLocked(){
    	File lock=new File(lockFile);
    	return lock.exists();
    }
    /**
     * @return time elapsed since socket was last closed
     */
    public static synchronized long lastModified(){
    	File tmp=new File(tmpFile);
    	if (tmp.exists())
    		return tmp.lastModified();
    	return 0;
    }
    
    /**
     * @return pid of running server
     */
    public static synchronized String getLockID(){
    	File lock=new File(lockFile);
    	if(!lock.exists())
    		return null;
        try{
        	FileInputStream fin = new FileInputStream(lockFile);
        	BufferedReader dataReader = new BufferedReader
        		(new InputStreamReader(fin));
        	
           	String line= dataReader.readLine();	//get first line
        	fin.close();
        	return line;
  
        } catch(Exception e){
        	System.out.println("Could not read lock file");
        }
        return null;  	
    }

    /**
     * Try to kill the currently running server process
     * @return false if process was killed and lock is free
     */
    public static synchronized boolean killServer(){
     	if(!isLocked())
    		return false; // lock already freed
    	
    	String pidStr=getLockID();
    	if(pidStr==null)
    		return true; // old style lock file - no pid info so need to kill using ps
    	String pid=null;

  		StringTokenizer tok=new StringTokenizer(pidStr,"@");
		if(tok.hasMoreTokens()){
			pid=tok.nextToken();
			if(pid !=null && pid.length()>0){
				ProcessBuilder pb = new ProcessBuilder("kill",pid); // kill nice so process can clean up
				int maxtries=4;
				int trys=maxtries;
				int wait_time=1000;
                int i=1;
				try {
					Process p=pb.start();
					p.waitFor();
					while(i<maxtries){
						Thread.sleep(wait_time);
						if(!isLocked()){
							return false;
						}
						i++;
					}
			    	DataFileManager.writeMsgLog("Could not kill server", debug); 
					removeLock();
					pb = new ProcessBuilder("kill","-9",pid); // kill with extreme prejudice
					p=pb.start();
					p.waitFor();
					return false; // hope for the best					
				} 
				catch (Exception e) {
			    	DataFileManager.writeMsgLog("breakLock Error:"+e, debug); 
				}
			}
		}
        return true;  // some problem occurred
    }

}