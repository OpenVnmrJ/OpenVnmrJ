/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.cryomon;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.StringTokenizer;

/**
 * storage class for fill history data
 */
public class DataObject {
	String data;
	DataObject(String what){
		data=what.replace(",,", ",0,");
	}
	Date getDate(){
		StringTokenizer toker=new StringTokenizer(data,",");
		String str=toker.nextToken();
		return CryoMonitorSocketControl.getDate(str);
	}
	long getTime(){
		return (getDate().getTime()/1000);
	}
	void setTime(long tm){
		StringTokenizer toker=new StringTokenizer(data,",");
		String time=toker.nextToken();
		String heLevel=toker.nextToken();
		String n2Level=toker.nextToken();
		String errors="";
		if(toker.hasMoreTokens())
			errors=toker.nextToken();
		long tval=tm*1000;
		Date date=new Date(tval);
		SimpleDateFormat fmt = new SimpleDateFormat("yy:MM:dd:HH:mm");
		time=fmt.format(date);
		data=time+","+heLevel + "," + n2Level +","+ errors;  		
	}
	double getHeLevel(){
		StringTokenizer toker=new StringTokenizer(data,",");
		toker.nextToken(); // skip date
		String str=toker.nextToken();
		return Double.valueOf(str).doubleValue();
	}
	double getN2Level(){
		StringTokenizer toker=new StringTokenizer(data,",");
		toker.nextToken(); // skip date
		toker.nextToken(); // skip He level
		String str=toker.nextToken();
		return Double.valueOf(str).doubleValue();
	}
	int getStatus(){
		StringTokenizer toker=new StringTokenizer(data,",");
		toker.nextToken(); // skip date
		toker.nextToken(); // skip He level
		toker.nextToken(); // skip N2 level
		if(toker.hasMoreTokens()){
			String str=toker.nextToken();
			return Integer.valueOf(str).intValue();
		}
		return 0;
	}
	String getString(){
		return data.trim();
	}
}
