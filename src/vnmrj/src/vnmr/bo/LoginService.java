/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import  vnmr.util.*;
import  vnmr.ui.shuf.*;
import  vnmr.ui.*;
import  vnmr.templates.*;
import java.util.*;
import java.io.*;


/**
 * Provides various services related to login.
 *
 * @author Mark Cao
 */
public class LoginService {
    // ==== static variables
    /** login service -- probably just a singleton is needed */
    private static LoginService loginService = new LoginService();

    // Hash tables filled from sys admin files.
    private static Hashtable userHash;
    private static Hashtable groupHash;
    private static Hashtable accessHash;

    /**
     * get default login service
     * @return login service
     */
    public static LoginService getDefault() {
	return loginService;
    } // getDefault()

    /**
     * constructor
     */
    public LoginService() {

	// Create the userHash Hashtable by reading the
	// userList file.
	// If this is called a second time, the first Hashtable is just
	// discarded and the new reference saved.
	userHash = User.readuserListFile();
	groupHash = Group.readGroupFile();
	// Pass groupHash and userHash to readAccessFile because 
	// it cannot get to it thru this LoginService yet because we are 
	// still initializing.
	accessHash = Access.readAccessFile(userHash, groupHash);

    } // LoginService()


    /**
     * Check given account/password for validity.
     * @param account the account to be accessed
     * @param password attempted password of account
     * @return password valid or not
     */
    public boolean checkPassword(String account, String password) {
	String[] allUsers;
	// This implementation doesn't check password validity. 
	// Rather, just if user is known.
	allUsers = getAllUsers();
	for (int i = 0; i < allUsers.length; i++)
	    if (account.equals(allUsers[i]))
		return true;
	return false;
    } // checkPassword()

    /**
     * Check given account/password for validity.
     * @param account the account to be accessed
     * @param password attempted password of account
     * @return User object if password is valid. Otherwise, return null.
     */
    public User getUser(String userAccount) {
	User user = (User) userHash.get(userAccount);
	// If user not found, it we just return null
	return user;

    } // getUser()


    static public String[] getAllUsers() {
	String[]  users;

	users = new String[userHash.size()];
	Enumeration ee = userHash.keys();
	for(int i=0; ee.hasMoreElements(); i++) 
	    users[i] = (String)ee.nextElement();

	return users;
    }

    public Hashtable getuserHash() {
	return userHash;
    }
    public Hashtable getgroupHash() {
	return groupHash;
    }

    public static Hashtable getaccessHash() {
	return accessHash;
    }

    public void setuserHash(Hashtable userHash) {
	this.userHash = userHash;
    }

    public void setgroupHash(Hashtable groupHash) {
	this.groupHash = groupHash;
    }

    public void setaccessHash(Hashtable accessHash) {
	this.accessHash = accessHash;
    }

} // class LoginService
