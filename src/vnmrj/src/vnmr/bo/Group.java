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
import java.awt.*;
import javax.swing.*;
import vnmr.ui.*;
import vnmr.util.*;



/************************************************************************
	Class Name:   Group

	Author:       Glenn Sullivan        Date: 4/16/99

	Purpose:      

	Algorithm/Details:
	    group file format
		groupname:Long Name: user1, user2, user3
	    Example:
	        ChemDept:Chemistry Department: Rudy, Sylvia, Larry, Victor, \
                         George, Susan, Peter, Wanda
                ProfMosherGroup: Professor Moshers Group: Sylvia, Susan, Peter
*************************************************************************/

public class Group {
    private String    name;
    private String    longName;
    private ArrayList members;

    public Group(String  name, String  longName, ArrayList  members) {
	this.name = name;
	this.longName = longName;
	this.members = members;

    }

    public String getname() {
	return name;
    }

    public String getlongName() {
	return longName;
    }

    public ArrayList getmembers() {
	return members;
    }

    public String toString() {
	String string;

	string = new String(name + "  '" + longName + "'");
	for(int i=0; i < members.size(); i++)
	    string += "  "+ members.get(i);
	return (string);
    }

    // Open, read and parse the group file.
    public static Hashtable readGroupFile() {
	BufferedReader  in;
	String		inLine;
	String		filepath, val1, val2;
	String		groupName, longGroupName;
	ArrayList	groupMembers;
	StringTokenizer tok;
	int		num_elements1, num_elements2;
	Group		group;
	Hashtable 	groupHash;
	Hashtable	group_tmp;
        boolean         foundVarianAndMe=false;

	groupHash = new Hashtable();

	filepath = new String(System.getProperty("sysdir") +
			      "/adm/users/group");

	// Open the group file.
	try {
            UNFile file = new UNFile(filepath);
	    in = new BufferedReader(new FileReader(file));
	} catch(Exception e) {
            // We need to add agilent and me even if the file is not there
            groupName = new String("agilent and me");
            longGroupName = new String("Myself and system");
            groupMembers = new ArrayList();
            groupMembers.add("me");
            groupMembers.add("Agilent");
            groupMembers.add("agilent");
            groupMembers.add("varian");


            // Create an Group object with all the info.
            group = new Group(groupName, longGroupName, groupMembers);

            // Add one to the groupHash Hashtable.
            groupHash.put(groupName, group);
 
	    Messages.postWarning("Missing file " + filepath +
                         "\n    should contain all of the group definitions. " +
                         "The format of the file is:" +
                         "\n    groupname:Long Name: user1, user2, user3");
            Messages.writeStackTrace(e);
	    return groupHash; 
	}
	try {
	    // Read one line at a time.
	   while ((inLine = in.readLine()) != null) {
                if (inLine.length() > 1 && !inLine.startsWith("#")) {
		    inLine.trim();
		    // Create a Tokenizer object to work with.  This one
		    // is just for the purpose of finding out if all three
		    // section of each line exist.
		    tok = new StringTokenizer(inLine, ":");

		    num_elements1 = tok.countTokens();

		    if (num_elements1 < 3) {
			Messages.postError("The line:  " + inLine +
                             "\n    In the file:  " + filepath +
                             "  has too few entries." +
                             "\n    The file format should be:" +
                             "  groupname:Long Name: user1, user2, user3\n");
			return groupHash;
		    }
		    // Get a new StringTokenizer with both deliminators.
		    tok = new StringTokenizer(inLine, ":,");
		    num_elements2 = tok.countTokens();

		    groupName = new String(tok.nextToken().trim());
                    if(groupName.equals("agilent and me"))
                        foundVarianAndMe = true;
		    longGroupName = new String(tok.nextToken().trim());

		    // Now get the list of users for this group
		    groupMembers = new ArrayList();
		    for(int i=0; i < num_elements2 - 2; i++) {
			groupMembers.add(tok.nextToken().trim());
		    }


 		    // Create an Group object with all the info.
 		    group = new Group(groupName, longGroupName, groupMembers);

 		    // Add one to the groupHash Hashtable.
 		    groupHash.put(groupName, group);
		}
	   }
           // If the group file does not contain "agilent and me", we need
           // to add a group of this name, else some locator statements 
           // will not work.
           if(!foundVarianAndMe) {
               groupName = new String("agilent and me");
               longGroupName = new String("Myself and system");
               groupMembers = new ArrayList();
               groupMembers.add("me");
               groupMembers.add("Agilent");
               groupMembers.add("agilent");
               groupMembers.add("varian");

               // Create an Group object with all the info.
               group = new Group(groupName, longGroupName, groupMembers);

               // Add one to the groupHash Hashtable.
               groupHash.put(groupName, group);
           }
           in.close();
	   
		   
	} catch(IOException e) { }

	return groupHash;

    }

}
