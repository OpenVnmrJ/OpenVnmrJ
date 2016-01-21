/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.util;

import java.util.*;
import java.io.*;


/********************************************************** <pre>
 * Summary: Combination Hashtable and ArrayList
 *
 * @author  Glenn Sullivan
 * Details:
 *   This class is created for those time when you need to search
 *   a list by a key and ALSO need to get items by index.
 *   It simply consist of both a Hashtable AND and ArrayList
 *   and has most of the methods contained by both of those.
 *   Both will be kept up to date as things are added to and
 *   removed from this object.
 * 
 </pre> **********************************************************/
public class HashArrayList extends Hashtable implements  Serializable {

    private ArrayList list;
    private ArrayList keylist;

    /************************************************** <pre>
     * Summary: Constructor.  Create a Hashtable and ArrayList.
     </pre> **************************************************/
    public HashArrayList () {
        // ArrayList of values at the corresponding positions of each key
        // in the keylist.
	list = new ArrayList();
        // ArrayList of keys which correspond to values in 'list'
	keylist = new ArrayList();
    }

    /************************************************** <pre>
     * Summary: Constructor.  Create a Hashtable and ArrayList given a
     *  Hashtable with current information.
     </pre> **************************************************/
    public HashArrayList (Hashtable initTable) {
        // ArrayList of values at the corresponding positions of each key
        // in the keylist.
	list = new ArrayList();
        // ArrayList of keys which correspond to values in 'list'
	keylist = new ArrayList();

        Set set = initTable.entrySet();
        Iterator iter = set.iterator();
        while(iter.hasNext()) {
           Map.Entry entry = (Map.Entry)iter.next();
           // Fill the two array lists
           list.add(entry.getValue());
           keylist.add(entry.getKey());
           // Fill this Hashtable
           put(entry.getKey(), entry.getValue());
        }
    }


    /************************************************** <pre>
     * Summary: Clear the Hashtable and the ArrayList.
     </pre> **************************************************/
    public void clear() {
	super.clear();
	list.clear();
	keylist.clear();
    }

    /************************************************** <pre>
     * Summary: Returns the value at the specified position in the list.
     </pre> **************************************************/
    public Object get(int index) {
	return list.get(index);
    }


    /************************************************** <pre>
     * Summary: Returns the key at the specified position in the list.
     </pre> **************************************************/
    public Object getKey(int index) {
	return keylist.get(index);
    }

    /************************************************** <pre>
     * Summary: Returns the list of keys as an ArrayList.
     </pre> **************************************************/
    public ArrayList getKeyList() {
	return keylist;
    }

    /************************************************** <pre>
     * Summary: Returns ordered element list of values.
     </pre> **************************************************/
    public Enumeration elements() {
        Vector v=new Vector(list);
	    return v.elements();
    }

    /************************************************** <pre>
     * Summary: Add this object to both the Hashtable and the ArrayList
     *
     * Details:
     * If it already exists, update the current entry, else
     * append to end of list.
     * Return the index where this item was put or updated
     *
     </pre> **************************************************/
    public int set(Object key, Object value)  {
	if(keylist.contains(key)) {
	    int index = keylist.indexOf(key);
	    list.set(index, value); // Set the new value in list
	    // The keylist should be okay.
	}
	else {
	    // Does not exist, add to list and keylist.
	    list.add(value);
	    keylist.add(key);
	}

	// Hashtable will automatically update the item if it exists.
	super.put(key,value);

	// Return the index where this item was put or updated
	return keylist.indexOf(key);
    }

    /************************************************** <pre>
     * Summary: Add this object to both the Hashtable and the ArrayList at
     *    the specified location.
     *
     * Details:
     * If it already exists, remove the current entry and add current entry.
     * at the row specified.
     * Return the index where this item was put or updated
     *
     </pre> **************************************************/
    public int add(int index, Object key, Object value)  {
	if(keylist.contains(key)) {
            // It exists, remove the one in the current location
            int oldIndex = keylist.indexOf(key);
            // It already exist and is already in this location, just return
            if(oldIndex == index)
                return index;

            // If the one we are removing is a lower index than where we are
            // moving it to, then we will be screwing up the index when
            // we remove the original one.  Subtract one from index to
            // compensate for removing one ahead of this.
            if(index > oldIndex)
                index -= 1;

            // Now go ahead and remove the original entry.
            list.remove(oldIndex);
            keylist.remove(key);
	}
        // add to list and keylist.
        list.add(index, value);
        keylist.add(index, key);

	// Hashtable will automatically update the item if it exists.
	super.put(key,value);

	// Return the index where this item was put
	return index;
    }

    public Object put(Object key, Object value)  {
        int i=set(key, value);
        return new Integer(i);
    }

    /************************************************** <pre>
     * Summary: Remove object with key from both the Hashtable and the
     * ArrayList.
     </pre> **************************************************/
    public Object remove(Object key)  {
	// Get the index that goes with this key
	int index = keylist.indexOf(key);

	// Remove via index from the list and the keylist
	list.remove(index);
	keylist.remove(index);

	// Remove from the Hashtable
	return super.remove(key);
    }

    /************************************************** <pre>
     * Summary: Remove object with this index from both the Hashtable
     * and the ArrayList.
     </pre> **************************************************/
    public Object remove(int index) {
	// Get the key that goes with this index
	String key = (String) keylist.get(index);
	list.remove(index);
	keylist.remove(index);

	// Now remove hash entry via key
	return 	super.remove(key);
    }

    /************************************************** <pre>
     * Summary: Replaces the element at the specified position in this list 
     * with the specified element.
     </pre> **************************************************/
    public void set(int index, Object element, Object key) {
	// Does this key already exist and is it the same index?
	int listindex = keylist.indexOf(key);
	if(index != listindex) {
	    Messages.postError(
                "HashArrayList: set: Cannot set two keys the same");
	    return;
	}
	list.set(index, element);
	keylist.set(index, key);

	// Hashtable will automatically update the item if it exists.
	put(key, element);	
    }

    /************************************************** <pre>
     * Summary: Return number of entries in ArrayList which should
     * be the number of entries in the Hashtable also.
     </pre> **************************************************/
    public int size() {
	int sizel = list.size();
	int sizeh = super.size();
	if(sizel != sizeh)
	    Messages.postError(
		"HashArrayList: ArrayList size and Hashtable size differ.");

	return sizel;
    }

    /************************************************** <pre>
     * Summary: Trim the ArrayList to size.
     </pre> **************************************************/
    public void trimToSize() {
	list.trimToSize();
    }

    /************************************************** <pre>
     * Summary: Return the index of this object.
     </pre> **************************************************/
    public int indexOf(Object obj) {
	int index = list.indexOf(obj);

	return index;
    }

    /******************************************************************
     * Summary: Return a Hashtable representing all of the info in here.
     *
     *
     *****************************************************************/

    public Hashtable getHashtable() {
        Hashtable ht = new Hashtable();

        Set set = entrySet();
        Iterator iter = set.iterator();
        // Go through the Hashtable part of this HashArrayList and create
        // a Hashtable.
        while(iter.hasNext()) {
            Map.Entry entry = (Map.Entry)iter.next();
            String key = (String)entry.getKey();
            ArrayList al = (ArrayList)entry.getValue();

            ht.put(key, al);
        }

        return ht;

    }

}
