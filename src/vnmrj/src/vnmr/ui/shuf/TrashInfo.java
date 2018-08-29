/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.io.*;
import java.util.*;
import java.text.*;

import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.util.*;

/********************************************************** <pre>
 * Summary: Contain info about one trash entry
 *
 * @author  Glenn Sullivan
 * Details:
 *   -
 * 
 </pre> **********************************************************/



public class TrashInfo implements  Serializable {

    public String filename;  // Filename without extension
    public String fullpath;  // Full path of trash dir for this file (direct)
    public String hostFullpath;
    public String origFullpath;  // Orig full path of trashed file (direct)
    public String origHostFullpath;// Orig hostFullpath of trashed file (direct)
    public String objType;   // Spotter type
    public String dhost;  // Host name where trash file actually exists 
    public String owner;
    public String parFile;  // For protocols only and then only sometimes
    public String dparPath;   // For protocols, path where par came from
    public Date   dateTrashed;
    public boolean protocolBrowser; // trash request originated in Protocol Browser


    /**************************************************
     * Summary: constructor, Fill member variables from args and set
     *          date equal to current date. 
     <**************************************************/
    public TrashInfo(String fName, String ofpath, String oType, String hName, 
                     String own, String ohfpath, boolean protBrowser) {

        filename = fName;
        origFullpath = ofpath;
        objType = oType;
        dhost = hName;
        origHostFullpath = ohfpath;
        owner = own;
        parFile = null;
        dparPath = null;
        dateTrashed = new Date();
        protocolBrowser = protBrowser;
        
    }

    /**************************************************
     * Summary: constructor, Fill all member variables from args and set
     *          date from arg also. 
     *
     <**************************************************/
    public TrashInfo(String fName, String fPath, String hfPath, String oType, 
                     String hName, String own, String ofpath, String ohfpath, 
                     String par, String pPath, Date date) {

        filename = fName;
        fullpath = fPath;
        objType = oType;
        dhost = hName;
        hostFullpath = hfPath;
        origFullpath = ofpath;
        origHostFullpath = ohfpath;
        parFile = par;
        dparPath = pPath;
        owner = own;
        dateTrashed = date;
        protocolBrowser = false;
    }
    /**************************************************
     * Summary: Write a TrashInfo file for this item.
     *
     *
     **************************************************/
    public boolean writeTrashInfoFile(String filepath) {
	PrintWriter out;

	try {
	    // Write out the file in plain text
            UNFile file = new UNFile(filepath);
	    out = new PrintWriter(new FileWriter(file));
	    // Write out a header line
	    out.println("TrashInfo");
            out.println(toString());
	    out.close();
	}
	catch (Exception e) {
            Messages.postError("Problem creating " + filepath);
	    Messages.writeStackTrace(e);
            return false;
	}

        return true;
    }

    /**************************************************
     * Summary: Read the file and create a TrashInfo item.
     *
     *
     **************************************************/
    static public TrashInfo readTrashInfoFile(String filepath) {
	BufferedReader in;
	String var;
	String value;
	String line;
	QuotedStringTokenizer tok;
        TrashInfo trashInfo;
        String name=null;  // filename
        String path=null;  // fullpath
        String type=null;  // objType
        String host=null;  // hostName
        String hfpath=null; //hostFullpath
        String own=null;   // owner
        String ofpath=null; // orig fullpath
        String ohfpath=null; // orig hostfullpath
        String pFile=null;  // par file name
        String pPath=null;   // par file full path
        Date   date=null;  // dateTrashed
        boolean protocolBrowser=false;
        String mpathW;

	try {

            // If file does not exist, return null
            UNFile file = new UNFile(filepath + File.separator + "trashinfo");
            if(!file.isFile())
                return null;

	    in = new BufferedReader(new FileReader(file));

	    // The file must start with HoldingData or ignore the rest.
	    line = in.readLine();
	    if(line != null) {
		tok = new QuotedStringTokenizer(line, " \t");
		if (tok.hasMoreTokens()) {
		    var = tok.nextToken();
		    if(!var.equals("TrashInfo")) {
                        throw(new Exception(filepath +
                                            " is not a TrashInfo file."));
		    }
		}
		else
		    throw(new Exception(filepath +
                                        " is not a TrashInfo file."));
	    }
	    while ((line = in.readLine()) != null) {
		tok = new QuotedStringTokenizer(line, " \t");
		if (tok.hasMoreTokens())
		    var = tok.nextToken();
		else
		    continue;
		

                if (tok.hasMoreTokens())
		    value = tok.nextToken();
		else
		    value = "";

                if (var.equals("filename"))
                    name = value;
                else if (var.equals("fullpath"))
                    path = value;
                else if (var.equals("objType"))
                    type = value;
                else if (var.equals("hostName"))
                    host = value;
                else if (var.equals("owner"))
                    own = value;
                else if (var.equals("origFullpath"))
                    ofpath = value;
                else if (var.equals("origHostFullpath"))
                    ohfpath = value;
                else if (var.equals("hostFullpath"))
                    hfpath = value;
                // Allow pFile and pPath to be null references.
                else if (var.equals("parFile")) {
                    if(!value.equals("null"))
                        pFile = value;
                }
                else if (var.equals("parPath")) {
                    if(!value.equals("null"))
                        pPath = value;
                }
                else if (var.equals("dateTrashed")) {
                    SimpleDateFormat df;
                    df = new SimpleDateFormat("MM/dd/yyyy HH:mm:ss");
                    date = df.parse(value);
                }
                else if (var.equals("protocolBrowser")) {
                    if(value.equals("true"))
                        protocolBrowser = true;
                }

	    }
            in.close();

            // Test to see that all necessary values were obtained
            // Allow pFile and pPath to be null.
            if(name == null || path == null || type == null || host == null
                            || own == null || date == null || ofpath == null
                            || hfpath == null || ohfpath == null) {
                Messages.postError(filepath + " Not a complete transinfo file");
                return null;
            }


	}
 	catch (Exception e) {
            Messages.postError("Problem reading " + filepath);
	    Messages.writeStackTrace(e);
            return null;
	}
        trashInfo = new TrashInfo(name, path, hfpath, type, host, own, 
                                  ofpath, ohfpath, pFile, pPath, date);
        return trashInfo;
    }



    public String toString() {
        SimpleDateFormat df = new SimpleDateFormat("MM/dd/yyyy HH:mm:ss");
        return("filename " + filename + "\n" + 
               "fullpath " + fullpath + "\n" + 
               "objType " + objType  + "\n" +
               "hostName " + dhost + "\n" +
               "hostFullpath " + hostFullpath + "\n" +
               "owner " + owner  + "\n" +
               "parFile " + parFile + "\n" +
               "parPath " + dparPath + "\n" +
               "origFullpath " + origFullpath  + "\n" +
               "origHostFullpath " + origHostFullpath  + "\n" +
               "dateTrashed \"" + df.format(dateTrashed)  + "\"\n");
    }

}
