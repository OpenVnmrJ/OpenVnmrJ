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
import vnmr.ui.shuf.*;
import vnmr.util.*;

/************************************************************************
    Class Name:   Access

    Author:    Glenn Sullivan        Date: 4/20/99

    Purpose:      

    Algorithm/Details:
    access file format:
	       user: comma separated list of users and groups
    Example:
	       Rudy: George
	       Sylvia: ProfMosherGroup, Rudy, Larry
	       Wanda: all
	       
    Always include the user in his access list.
*************************************************************************/

public class Access {
    private String    userName;
    private ArrayList list;
    private ArrayList groups;

    public Access(String  userName,  ArrayList list, ArrayList groups) {
        this.userName = userName;
        this.list = list;
        this.groups = groups;
    }

    public String getuserName() {
        return userName;
    }

    public ArrayList getlist() {
        return list;
    }

    public ArrayList getgroups() {
        return groups;
    }

    public String toString() {
        String string;
        int i;

        string = new String(userName + ":");
        for(i=0; i < list.size(); i++)
            string += " "+list.get(i);
        string += " :";
        for(i=0; i < groups.size(); i++)
            string += " "+groups.get(i);
        return string;
    }
    
    /** build the access list */
  
    public static Hashtable readAccessFile(Hashtable userHash, 
        Hashtable groupHash) 
    {
        HashArrayList accessList= new HashArrayList();
        String path=FileUtil.openPath("SYSTEM/USRS/userlist");
        
        if(path !=null)
            makeAccessList(userHash,groupHash,accessList);
        else{
            path = FileUtil.openPath("SYSTEM/USRS/access");
            if(path !=null)
                readAccessFile(path,userHash,groupHash,accessList);
        }
        if(DebugOutput.isSetFor("accessfile")){
            Enumeration elems=groupHash.elements();
            while(elems.hasMoreElements()){
                Group group=(Group)elems.nextElement();
                Messages.postDebug(group.toString());
            }
            elems=accessList.elements();
            while(elems.hasMoreElements()){
                Access access=(Access)elems.nextElement();
                Messages.postDebug(access.toString());
            }
        }
        return accessList;
    }

    /** build the access list from User access ArrayLists */
    
    public static void makeAccessList(Hashtable userHash,
            Hashtable groupHash, Hashtable accessHash)
    {
        Access     access;
        Group      grp;
        Enumeration users=userHash.elements();
        while(users.hasMoreElements()){
            User user=(User)users.nextElement();
            String username=user.getAccountName();
	    
            ArrayList userAccess=user.getAccessList();
            Hashtable AHash=new Hashtable();
            Hashtable GHash=new Hashtable();
            ArrayList accessList=new ArrayList();
            ArrayList groupList=new ArrayList();
            String item;
            boolean allset=false;

            for(int i=0;i<userAccess.size();i++){
                String name=(String)userAccess.get(i);
                if(name.equals("all") || name.equals("All")) {
                    allset=true;
                    continue;
                }
                if(groupHash.containsKey(name)) {
                    grp = (Group)groupHash.get(name);
                    ArrayList mlist = grp.getmembers();
                    for(int m=0; m < mlist.size(); m++){
                        item=(String)mlist.get(m);
                        AHash.put(item,item);
                    }
                    item=grp.getname();
                    GHash.put(item,item);
                }
                else 
                    AHash.put(name,name);
                AHash.put(username,username);
            }
            
            if(allset){
                AHash.clear();
                AHash.put("all","all");
                Enumeration groups=groupHash.elements();
                while(groups.hasMoreElements()){
                    grp = (Group)groups.nextElement();
                    item=grp.getname();
                    GHash.put(item,item);
                }
            }
            // make array lists from hash tables
            
            Enumeration items=AHash.elements();
            while(items.hasMoreElements())
                accessList.add((String)items.nextElement());

            items=GHash.elements();
            while(items.hasMoreElements())
                groupList.add((String)items.nextElement());

            access = new Access(username, accessList, groupList);
            accessHash.put(username, access);
        }
        addCurrentUser(accessHash);
    }

    /** make sure the unix user is in the access list and has varian
        in his list
    */

    private static void addCurrentUser(Hashtable accessHash){
        // Get the current user
        String curuser = System.getProperty("user.name");

        // See if current user is there.
        if(accessHash.containsKey(curuser) == false) {
            String err = "The current user, " + curuser +
		  ", is not in the access file.\n" +
		  "   This user will have access only to his/her own files.\n";
            Messages.postError(err);

	        // Add this user to the list
            ArrayList accessList = new ArrayList(1);
            accessList.add(curuser);
            Access access = new Access(curuser, accessList, new ArrayList(1));
            accessHash.put(curuser, access);
        }
        // Now we have this user for sure, get his list and be sure
        // varian is in it.
        Access access = (Access) accessHash.get(curuser);
        ArrayList accessList = (ArrayList) access.getlist();
        if(!accessList.contains("varian"))
            accessList.add("varian");
    }




    /** Is the user passed in, in the access list */
    public boolean isUserInList(String user) {
	String		member;

	for(int i=0; i < list.size(); i++) {

	    member = (String)list.get(i);
	    if(member.equals(user) || member.equals("all"))
	        return true;
	}
	return false;
    }
    
    /** build the access list by parsing the (old-style) access file. */
    
    public static void readAccessFile(String filepath, Hashtable userHash,
					   Hashtable groupHash, Hashtable accessHash) 
	{
	BufferedReader  in;
	String		inLine;
	String		val1, val2;
	ArrayList	accessList;
	ArrayList	groupsList;
	StringTokenizer tok;
	int		num_elements1, num_elements2;
	Access		access;
        FileReader      fr;

	groupsList = new ArrayList();

	// Open the access file.
	try {
            fr = new FileReader(filepath);
	    in = new BufferedReader(fr);
	} catch(Exception e) { 
	    Messages.postError("\nThe file " + filepath +
                      "\ncontain all of the access definations. " +
                      "The format of the file is:" + 
                      "   \nuser: comma separated list of users and groups \n");

            Messages.writeStackTrace(e);
	    return; 
	}
	try {
	    // Read one line at a time.
	   while ((inLine = in.readLine()) != null) {
                if (inLine.length() > 1 && !inLine.startsWith("#")) {
		    inLine.trim();
		    // Create a Tokenizer object to work with.  This one
		    // is just for the purpose of finding out if both
		    // section of each line exist.
		    tok = new StringTokenizer(inLine, ":");

		    // Create new lists to fill for this user.
		    accessList = new ArrayList();
		    groupsList = new ArrayList();

		    num_elements1 = tok.countTokens();

		    if (num_elements1 < 2) {
			Messages.postError("The line:  " + inLine +
                              "\nIn the file:  " + filepath +
                              "  has too few entries." +
                              "\nThe file format should be:" +
                              "  accessname:Long Name: user1, user2, user3\n");
                        in.close();
			return;
		    }
		    // Get a new StringTokenizer with both deliminators.
		    tok = new StringTokenizer(inLine, ":,");
		    num_elements2 = tok.countTokens();

		    String username = new String(tok.nextToken().trim());

		    String  user;
		    User inv;
		    boolean allset=false;

		    // Now get the list of users for this user
		    for(int i=0; i < num_elements2 - 1; i++) {
			user = new String(tok.nextToken().trim());
			
			// If 'all', just put the word 'all' as the first
			// item in the list.
			if(user.equals("all") || user.equals("All")) {
			    accessList.add("all");
			    allset = true;
			}
			else {
			    // If this is a group name, add the users from this
			    // group to the list.
			    // If all, then all groups will be added later
			    if(groupHash.containsKey(user) && !allset) {
				Group grp = (Group)groupHash.get(user);
				    
				    // put each users name into the tmp hash.
				ArrayList mlist = grp.getmembers();
				for(int m=0; m < mlist.size(); m++){
				    accessList.add(mlist.get(m));
				}
				// Also keep the group name.
				groupsList.add(user);
			    }
			    else {
				// Not a group, just add the user to the list.
				accessList.add(user);
			    }
			}
		    }

		    // If the current user is not in the list, 
		    // nor is 'all', add the current user.
		    if(!accessList.contains("all") && 
		      		 !accessList.contains(username)) {
			accessList.add(username);
		    }

		    // For users with 'all' access, include all groups
		    // from the group file for menu display.
		    if(allset) {
			Enumeration glist = groupHash.keys();
			while(glist.hasMoreElements()) {
			    String string = (String)glist.nextElement();
			    groupsList.add(string);
			}
		    }

		    // If a user is listed individually AND is in a group
		    // that is listed, he/she will be in the list twice.
		    // Weed out duplicates.
		    int k;
		    for(int i=0; i < accessList.size(); i++) {
			do {
			    k = accessList.lastIndexOf(accessList.get(i));
			    // if last occurance is not same as first, then
			    // we must have more than one.
			    if(k != i) {
				accessList.remove(k);
			    }
			} while (k != i);
		    }
		    // Since the lists will never grow, trim them down.
		    accessList.trimToSize();
		    groupsList.trimToSize();

 		    // Create an Access object with all the info.
 		    access = new Access(username, accessList, groupsList);

 		    // Add the object to the accessHash Hashtable.
 		    accessHash.put(username, access);
		}
	   }
	   
	   // See if the current user is in the list
	   
           addCurrentUser(accessHash);
           in.close();
	} catch(IOException e) { }

    }
}
