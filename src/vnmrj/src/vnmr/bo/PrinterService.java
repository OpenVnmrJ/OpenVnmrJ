/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.util.*;
import java.io.*;
import vnmr.ui.*;
import vnmr.util.*;

/**
 * Provide printer services.
 *
 * @author Mark Cao
 */
public class PrinterService {
    // ==== static variables
    /** printer service -- probably just a singleton is needed */
    private static PrinterService printerService = new PrinterService();

    // ==== instance variables
    /** vector of printers */
    private Vector printers;
    private String printer=null;

    /**
     * get default printer service
     */
    public static PrinterService getDefault() {
	return printerService;
    } // getDefault()

    /**
     * constructor (private to disallow creation by others)
     * parses /vnmr/devicenames to build a list of printers
     */
    private PrinterService() {
	printers = new Vector();
	String sysdir = new String(System.getProperty("sysdir"));
	if(sysdir==null)
	    return;
	String filepath=sysdir+"/devicenames";
	BufferedReader  in;
	try {
	    in = new BufferedReader(new FileReader(filepath));
	    String		inLine;
	    while ((inLine = in.readLine()) != null) {
	    	if (inLine.length() <= 1 || inLine.startsWith("#"))
	    	    continue;
		inLine.trim();
		StringTokenizer tok = new StringTokenizer(inLine);
		int ntokens = tok.countTokens();
		if(ntokens != 2)
		    continue;
		String key=tok.nextToken().trim();
		if(key.equals("Name"))
		    printers.addElement(tok.nextToken());
	    }
	    printer=getDefaultPrinter();
	} catch(IOException e) { 
	    System.out.println("could not read "+filepath);
	    return; 
	}
    } // PrinterService()

    /**
     * get printers enumeration
     * @return printers
     */
    public Enumeration getPrinters() {
	return printers.elements();
    } // getPrinters()

    /**
     * get default printer
     * @return default printer
     */
    public String getDefaultPrinter() {
	if (printers.size() == 0)
	    return null;
	// assume the first printer is the default
	return (String)printers.elementAt(0);
    } 

    public void setDefaultPrinter() {
	setPrinter(getDefaultPrinter());
    } 

    public String getPrinter() {
	return printer;
    } 

    public void setPrinter(String s) {
	printer=s;
    } 
} // class User
