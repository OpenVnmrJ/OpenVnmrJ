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

public class OneTag implements Serializable {
    public String tagName;
    /** List of entries with this tag set */
    public ArrayList entriesThisTag;
    public String objType;

    public OneTag(String name, String objType,  ArrayList entries) {
	tagName = name;
	entriesThisTag = entries;
	this.objType = objType;
    }

    public String toString() {
	String string = "Object Type = " + objType + 
	                "  tagName =     " + tagName + "\n" + entriesThisTag;
	return string;
    }
}
