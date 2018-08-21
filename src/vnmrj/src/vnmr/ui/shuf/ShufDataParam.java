/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui.shuf;


/**
 * Parameter for vnmr_data table
 *
 * @author Glenn Sullivan
 */
public class ShufDataParam {
    String param;
    String type;

    /**
     * constructor
     */
    public ShufDataParam(String param, String type) {
	this.param = param;
	if(type.equals("float"))
	    this.type = FillDBManager.DB_ATTR_FLOAT8;
	else
	    this.type = type;
    }

    /** Check for param name and type equals args.
	Return 0 for false, 1 if exact match, 
	2 if basic match, but type is array.
    */
    public int equals(String param, String type) {
	// all 'float' as well as float8, thus the startsWith call.
	// starts with also allows for array types to get caught.
	if(param.equals(this.param) && this.type.startsWith(type)) {
	    // The basic type matches, now see if it is an array
	    if(this.type.endsWith("[]"))
		return 2;
	    else
		return 1;
	}

	return 0;
    }

    public boolean equals(String param) {
	if(param.equals(this.param))
	    return true;

	return false;
    }


    public String getParam() {
	return param;
    }

    public String getType() {
	return type;
    }

    public void setParam(String newParam) {
	param = newParam;
    }

} 
