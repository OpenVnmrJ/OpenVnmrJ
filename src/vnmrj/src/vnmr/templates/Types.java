/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import java.io.*;

/** type codes and strings */

public interface Types
{
    //++++++ shuffler instruction strings ++++++++++
    
	//     ---------------------------------------------------------
	//     keyword for build    		key string used by shuffler
	//     ---------------------------------------------------------
    static public String PTYPE 			= "type";
    static public String ETYPE 			= "element"; 
    static public String NAME 			= "name"; 

	//     ---------------------------------------------------------
	//     keyword for build    		string displayed in shuffler menu
	//     ---------------------------------------------------------
    static public String ACQUISITION 	= "acquisition";  // PTYPE options
    static public String PROCESSING  	= "processing";
    static public String SETUP  	 	= "setup";
    static public String BASIC  	 	= "basic";

	// etype(elememt) options
    
    static public String PANEL 			= "panels";		  // ETYPE options
    static public String ELEMENT 		= "elements";
    static public String COMPOSITE  	= "composites";

}
