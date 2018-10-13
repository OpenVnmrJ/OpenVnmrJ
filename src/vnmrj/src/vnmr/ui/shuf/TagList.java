/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui.shuf;

import java.io.*;
import java.util.*;
//import postgresql.util.*;

import vnmr.bo.*;
import vnmr.util.*;

public class TagList implements Serializable {
    /** List of ArrayLists, one for each objType */
    static private HashArrayList allTagNames=null;

    public TagList() {
    }



    /************************************************** <pre>
     * Summary: Create a tag table and write out the persistence file.
     *
     </pre> **************************************************/

    static public void writePersistence() {
	/** List of all tag names for an object type. */
	ArrayList allTags;
	/** List of all items with a given tag name. */
	ArrayList entries;
	/** Object type. */
	String objType;
	/** Single tag contained in an object. */
	OneTag oneTag;
	/** List of OneTag objects containing tags. */
	ArrayList tagList;
	String filepath;
	ObjectOutput out;

        // If readPersistence has not been called, then allTagNames will
        // still be null.  In that case, do not try to write anything.
        if(allTagNames == null)
            return;

	ShufDBManager dbMg = ShufDBManager.getdbManager();
	tagList = new ArrayList();

	// Loop thru all object types
	for(int i=0; i < Shuf.OBJTYPE_LIST.length; i++) {
	    objType = Shuf.OBJTYPE_LIST[i];
	    // Within each object type, get the list of all tag name.
	    allTags = getAllTagNames(objType);
	    for(int k=0; k < allTags.size(); k++) {
		// For each tag name, get all entries with this tag and create
		// a OneTag for each tag.
		entries = dbMg.getAllEntriesWithThisTag(objType, 
							(String)allTags.get(k));
		oneTag = new OneTag((String)allTags.get(k), objType, entries);

		// Add this oneTag to the list
		tagList.add(oneTag);
	    }
	}

	// Write out to disk

	filepath = FileUtil.savePath("USER/PERSISTENCE/TagList");

	try {
	    out = new ObjectOutputStream(new FileOutputStream(filepath));
	    // Write it out.
	    out.writeObject(tagList);
            out.close();
	}
	catch (Exception e) {
            Messages.postError("Problem writing " + filepath);
            Messages.writeStackTrace(e, "Error caught in writePersistence.");
	}
    }


    /************************************************** <pre>
     * Summary: Read in the persistence file.
     *
     </pre> **************************************************/
    static public void readPersistence() {
	String filepath;
	ObjectInputStream in;
	ArrayList tags=null;
	OneTag oneTag;
	String objType;

	// Initilalize the static keeper of all tag names to be filled below.
	allTagNames = new HashArrayList();
	for(int i=0; i < Shuf.OBJTYPE_LIST.length; i++) {
	    objType = Shuf.OBJTYPE_LIST[i];
	    allTagNames.put(objType, new ArrayList());
	}

	filepath = FileUtil.openPath("USER/PERSISTENCE/TagList");
	if(filepath==null)
	    return;

	try {
	    in = new ObjectInputStream(new FileInputStream(filepath));
	    // Read it in.
	    tags = (ArrayList) in.readObject();
            in.close();
	}
	catch (ClassNotFoundException ce) {
            Messages.postError("Problem reading " +filepath);
            Messages.writeStackTrace(ce, "Error caught in readPersistence");
	}
	catch (Exception e) {
	    // No error output here.
	}	

	ShufDBManager dbMg = ShufDBManager.getdbManager();

	if(tags != null) {

	    // Go through the tags and put them into the DB.
	    for(int i=0; i < tags.size(); i++) {
		oneTag = (OneTag) tags.get(i);

		// Set this tag name in all entries in the list
		if(oneTag != null && oneTag.entriesThisTag != null && 
		   oneTag.objType != null && oneTag.tagName != null &&
		   oneTag.tagName.length() != 0 ) {

                    // Set this tag into the db
		    dbMg.setTag(oneTag.objType, oneTag.tagName, 
				oneTag.entriesThisTag, false);

                    // Set this tag value into LocAttr
                    dbMg.attrList.addTagValue(oneTag.objType, oneTag.tagName);
		}
		// Keep a list with these tag names.  We want to keep them
		// even if there are no entries with this tag.
		if(oneTag != null && oneTag.tagName != null &&
		       !oneTag.tagName.equals("all") && 
		       !oneTag.tagName.equals("separator"))
		    addToAllTagNames(oneTag.objType, oneTag.tagName);
	    }
	}
    }


    /** 
     *  Update the allTagNames list from the vnmr_data DB table.
     */
    static public void updateTagNamesFromDB() throws Exception{
        ArrayList list=null;
	ShufDBManager dbMg = ShufDBManager.getdbManager();

	for(int j=0; j < Shuf.OBJTYPE_LIST.length; j++) {
	    // pass true for fullSearch so we check the DB (vnmr_data table
	    // and not just the list we are filling now.
            try {
                list = dbMg.getTagListFromDB(Shuf.OBJTYPE_LIST[j]);
            }
            catch (Exception e) {
                throw e;
            }
            if(list == null) {
                Messages.postError("Problem getting attribute list for tag");
                return;
            }

	    // Be sure all of these are in the appropriate list
	    for(int k=0; k < list.size(); k++) {
		addToAllTagNames(Shuf.OBJTYPE_LIST[j], (String)list.get(k));
		if(Shuf.OBJTYPE_LIST[j].equals(Shuf.DB_VNMR_DATA)) {
		    try {
			Thread.sleep(4000);
		    }
		    catch (InterruptedException e) {return;}
		}
	    }
	}
    }


    static public void addToAllTagNames(String objType, String tagName) {
	if(allTagNames == null || tagName == null || tagName.equals("null") || 
	   	      tagName.equals("all")  || tagName.equals("separator") ) {
	    return;
	}
	// Get the list of tags for this objType.
	ArrayList tagList = (ArrayList)allTagNames.get(objType);

	// Add this tag to the correct list
	if(!tagList.contains(tagName)) {
	   tagList.add(tagName);
	}
    }


    static public void deleteFromAllTagNames(String objType, String tagName) {
        if(allTagNames == null)
            return;
        
	// Get the list of tags for this objType.
	ArrayList tagList = (ArrayList)allTagNames.get(objType);

	// Remove tag from list
	int index = tagList.indexOf(tagName);
        if(index >= 0)
            tagList.remove(index);
    }


    static public ArrayList getAllTagNames(String objType) {
        if(allTagNames == null)
            return new ArrayList();
        
	ArrayList list = (ArrayList) allTagNames.get(objType);

	// Return a copy, else the caller is modifying our list.
	ArrayList listcopy = (ArrayList)list.clone();
	return listcopy;
    }

    static public int getTagListSize(String objType) {
        if(allTagNames == null)
            return 0;
        
	ArrayList list = (ArrayList) allTagNames.get(objType);
        return list.size();
    }

}
