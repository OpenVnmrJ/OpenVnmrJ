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

/********************************************************** <pre>
 * Summary: Comparator for Case Insensitive Strings and numerics as Strings.
 *
 * 
 *	If both input strings are numerics, compare as numeric.
 *	Else, compare as Case Insensitive Strings.
 *	    See String.CASE_INSENSITIVE_ORDER
 </pre> **********************************************************/
public class NumericStringComparator
    		implements Comparator, java.io.Serializable {
    public int compare(Object o1, Object o2) {
	String s1 = (String) o1;
	String s2 = (String) o2;
	int nd1=0, nd2=0;
	int n1=s1.length(), n2=s2.length();
	boolean numeric=true;

	// Check to see if both strings are numbers
	for(int i=0; i < n1 && numeric == true; i++) {
	    if(!Character.isDigit(s1.charAt(i)) && s1.charAt(i) != '.') {
		numeric = false;
	    }
	}
	for(int i=0; i < n2 && numeric == true; i++) {
	    if(!Character.isDigit(s2.charAt(i)) && s2.charAt(i) != '.') {
		numeric = false;
	    }
	}

	// If numeric, determine the number of digits before the decimal
	if(numeric) {
	    for(int i=0; i < n1; i++) {
		if(s1.charAt(i) == '.')
		    break;
		else
		    nd1++;
	    }
	    for(int i=0; i < n2; i++) {
		if(s2.charAt(i) == '.')
		    break;
		else
		    nd2++;
	    }
	}
	
	// If numeric and both are the same length before the decimal, 
	// treat as characters.
	// If numeric and different lengths, then the longer one is bigger.
	if(numeric == false || (numeric == true && nd1 == nd2)) {
	    for (int i1=0, i2=0; i1<n1 && i2<n2; i1++, i2++) {
		char c1 = s1.charAt(i1);
		char c2 = s2.charAt(i2);
		if (c1 != c2) {
		    c1 = Character.toUpperCase(c1);
		    c2 = Character.toUpperCase(c2);
		    if (c1 != c2) {
			c1 = Character.toLowerCase(c1);
			c2 = Character.toLowerCase(c2);
			if (c1 != c2) {
			    return c1 - c2;
			}
		    }
		}
	    }
	}
	if(numeric) {
	    return nd1 - nd2;
	}
	else {
	    return n1 - n2;
	}
    }
}
