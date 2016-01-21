/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;


/**
 * An ParamIF provides the interface to store parameter value from VNMR
 * If value is 'null' then the parameter doesn't exist.
 */
public class  ParamIF {
    public String name;
    public String type;
    public String value;  // for single value or min
    public String value2; // for max
    public String values[];

    public ParamIF (String name, String type, String data) {
	this.name = name;
	this.type = type;
	this.value = data;
    }

    public ParamIF (String name, String type, String data[]) {
	this.name = name;
	this.type = type;
	this.values = data;
    }

    public ParamIF (String name, String type, String data, String data2) {
	this.name = name;
	this.type = type;
	this.value = data;
	this.value2 = data2;
    }

    public void paramString () {
        System.err.print(" param "+name+" type "+ type);
	if (type.indexOf("Array") > 0) {
	    System.err.println(" size "+values.length);
	    for (int m = 0; m < values.length; m++) 
	       System.err.print(" "+m+": "+values[m]);
	    System.err.println(" ");
	}
	else
	    System.err.println(" value "+value);
    }

} // ParamIF
