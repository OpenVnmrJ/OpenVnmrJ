/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;


/**************************************************************************
    Author:	    Glenn Sullivan		Date: 2/4/99

    Purpose:	    Containing one line of results of a search

    Algorithm/Details:
	Containing one line of results from a shuffler db_manager search.

	Each line/row is one of these objects.	Each of these is
	an array of objects containing the lock status and several
	strings.

	
	
**************************************************************************/

public class SearchResults {
    /** match indicator */
    public int match;
    /** content object */
    public Object content;
    public ID id;
    public String host;
    public String objType;

    /**
     * constructor
     * @param match match indicator
     */
    public SearchResults(int match, Object content, ID id, String host,
			 String objType) {
	this.match = match;
	this.content = content;
	this.id = id;
	this.host = new String(host);
	this.objType = new String(objType);
    } // SearchResults()

    public String toString() {
	return (String)content;

// 	Object[] cont;

// 	cont = (Object[]) content;

// //	String str = new String(match);
// //	String str1 = new String((String)cont[0]);
// 	String str2 = new String((String)cont[1]);
// 	String str3 = new String((String)cont[2]);
// 	String str4 = new String((String)cont[3]);
// 	String str5 = new String((String)cont[4]);
// 	return (new String(
// 	      str2 + " " + str3 + " " + str4 + " " + str5)); 

    }

} // class SearchResults
