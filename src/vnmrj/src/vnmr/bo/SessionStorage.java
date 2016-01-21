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
import vnmr.util.*;

/**
 * This service has two responsibilities: store all session information
 * associated with a user, and restore the last session information for
 * that user. User information is read from/written to a serializable
 * object (such as a Hashtable).
 *
 * <p>This implementation just writes to/reads from a file. 
 * A more realistic implementation might talk to a database instead.
 *
 */
public class SessionStorage {
    // ==== static variables
    /** session storage service */
    private static SessionStorage defaultStorage =
    new SessionStorage(FileUtil.savePath("USER/PERSISTENCE/session"));

    // ==== instance variables */
    /** directory in which user storage files are kept */
    private String storeFile;

    /**
     * get default SessionStorage object
     * @return default SessionStorage
     */
    public static SessionStorage getDefaultStorage() {
	return defaultStorage;
    } // getDefaultStorage()

    /**
     * constructor
     * @param storeDir path of top-level directory
     */
    public SessionStorage(String file) {
	this.storeFile = file;
    } // SessionStorage()

    /**
     * Put info into a persistent store. If given user is null, do
     * nothing.
     * @param user affected user
     * @param obj contains info to be stored
     */
    public void store(User user, Object obj) {
	if (user == null || storeFile == null)
	    return;
	FileOutputStream ostream = null;
	ObjectOutputStream p = null;
	try {
	    ostream = new FileOutputStream(storeFile);
	    p = new ObjectOutputStream(ostream);
	    p.writeObject(obj);
	    p.flush();
	    ostream.close();
	} catch (Exception e) {
            Messages.postError("Problem writing to " + storeFile +
                               "\n  Check unix permission of that directory " +
                               "and any existing file.");
	    Messages.writeStackTrace(e);
	    return;
	}
    } // store()

    /**
     * Restore stored info to one object. If no object can be restored,
     * return null.
     * @param user affected user
     * @return restored info
     */
    public Object restore(User user) {
	if (user == null || storeFile == null)
	    return null;

	FileInputStream istream = null;
	ObjectInputStream p = null;
	Object result = null;
	try {
	    istream = new FileInputStream(storeFile);
	} catch (FileNotFoundException e) {
	    return null;
	}
	try {
	    p = new ObjectInputStream(istream);
	    result = p.readObject();
	    istream.close();
	} catch (Exception e) {
            Messages.postError("Problem reading " + storeFile);
            Messages.writeStackTrace(e);
	    return null;
	}
	return result;
    } // restore()

} // class SessionStorage
