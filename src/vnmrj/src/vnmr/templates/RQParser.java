/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import java.awt.*;
import java.io.*;
import java.util.*;
import vnmr.ui.*;

public class RQParser
{
    ArrayList glist = null;
    //RQTreeTableModel model = null;

    public RQParser() 
    {
	glist = new ArrayList();
	//RQPanel rq = Util.getRQPanel();
	//if(rq != null) model = rq.getTreeTable().getModel();

    } // RQParser()
   
    public ArrayList parseStatement(String str) { 

	if(str == null || str.length() <= 0) return new ArrayList();

	ArrayList slist = new ArrayList();
	if(str.indexOf("G") == -1 && str.indexOf("g") == -1) {
	    slist.add(parseSentence(str));
	    return slist;
	}
 
	StringTokenizer tok = new StringTokenizer(str, "Gg");
	String value;
	while(tok.hasMoreTokens()){
	     value = tok.nextToken().trim();
	     slist.add(parseSentence("G"+value));
	}

	return slist;
    }

    public String parseSentence(String str) {

	if(str == null || str.length() <= 0) return "";

	String groups = null;
	String images = null;
	String frames = null;

	if(str.startsWith("G")) {
	    str.toLowerCase();
	    int s = str.indexOf("(");
	    if(s == -1) s = str.indexOf("s");
	    if(s != -1) groups = str.substring(1,s);

	    int f = str.indexOf("[");
	    if(f == -1) f = str.indexOf("f");
	    if(s != -1 && f != -1) {
	        images = str.substring(s+1,f);
		frames = str.substring(f+1);
	    } else if(f != -1) {
		groups = str.substring(1,f);
		frames = str.substring(f+1);
	    } else if(s != -1) {
		images = str.substring(s+1);
	    } else {
		groups = str.substring(1);
	    }
	} else {
	    str.toLowerCase();
	    int f = str.indexOf("[");
	    if(f == -1) f = str.indexOf("f");

	    if(f != -1) {
		images = str.substring(0,f);
		frames = str.substring(f+1);
	    } else 
		images = str;
	}

	String newstr = "";
	if(groups != null) {
	    //newstr += parseSelection(groups) + " ";
	    StringTokenizer tok = new StringTokenizer(groups, " ()[];\n\t");
	    if(tok.hasMoreTokens())
	    newstr += tok.nextToken().trim() + " ";
	    else newstr += "all ";
	} else newstr += "all ";
	if(images != null) {
	    //newstr += parseSelection(images) + " ";
	    StringTokenizer tok = new StringTokenizer(images, " ()[];\n\t");
	    if(tok.hasMoreTokens())
	    newstr += tok.nextToken().trim() + " ";
	    else newstr += "all ";
	} else newstr += "all ";
	if(frames != null) {
	    //newstr += parseSelection(frames);
	    StringTokenizer tok = new StringTokenizer(frames, " ()[];\n\t");
	    if(tok.hasMoreTokens())
	    newstr += tok.nextToken().trim();
	    else newstr += "all";
	} else newstr += "all";
	return newstr;
    }

    public String parseSelection(String str, Hashtable hs) {

	if(hs != null) str = replaceVariables(str, hs);

	return parseSelection(str);
    }

    public String parseSelection(String str) {

	if(str == null || str.length() <= 0) return "";

	StringTokenizer tok = new StringTokenizer(str, " ()[];,\n\t");
	str = "";
	String s;
	while(tok.hasMoreTokens()){
	    s = tok.nextToken().trim();
	    int step = -1;
	    int lower = -1;
	    int upper = -1;
	    Integer value;
	    if(s.indexOf(":") != -1) {
		try {
		    String tmpstr = s.substring(s.indexOf(":")+1);
		    if(tmpstr.indexOf("*") != -1 ||
			tmpstr.indexOf("+") != -1 ||
			tmpstr.indexOf("/") != -1) tmpstr = replaceMath(tmpstr);

		    value = Integer.valueOf(tmpstr);
		    if(value != null) step = value.intValue();
		} catch(NumberFormatException e) {
		    System.out.println("Syntax error at "+s);	    
		}
	    }
	    if(s.indexOf("-") != -1) {
		try {
		    String tmpstr = s.substring(0,s.indexOf("-"));
		    if(tmpstr.indexOf("*") != -1 ||
			tmpstr.indexOf("+") != -1 ||
			tmpstr.indexOf("/") != -1) tmpstr = replaceMath(tmpstr);

		    value = Integer.valueOf(tmpstr);
		    if(value != null) lower = value.intValue();
		} catch(NumberFormatException e) {
		    System.out.println("Syntax error at "+s);	    
		}

		
	        if(s.indexOf(":") != -1)
		try {
		    String tmpstr = s.substring(s.indexOf("-")+1,s.indexOf(":"));
		    if(tmpstr.indexOf("*") != -1 ||
			tmpstr.indexOf("+") != -1 ||
			tmpstr.indexOf("/") != -1) tmpstr = replaceMath(tmpstr);

		    value = Integer.valueOf(tmpstr);
		    if(value != null) upper = value.intValue();
		} catch(NumberFormatException e) {
		    System.out.println("Syntax error at "+s);	    
		}
		else 
		try {
		    String tmpstr = s.substring(s.indexOf("-")+1);
		    if(tmpstr.indexOf("*") != -1 ||
			tmpstr.indexOf("+") != -1 ||
			tmpstr.indexOf("/") != -1) tmpstr = replaceMath(tmpstr);

		    value = Integer.valueOf(tmpstr);
		    if(value != null) upper = value.intValue();
		} catch(NumberFormatException e) {
		    System.out.println("Syntax error at "+s);	    
		}
	    }

	    if(lower >= 0 && upper >= lower) {
	        if(step <= 0 || step > upper) step = 1;
		for(int i=lower; i<=upper; i+=step) {
		   str = str +" "+ String.valueOf(i);
		}
	    } else if(s.startsWith("r") || s.startsWith("R") ||
			s.startsWith("c") || s.startsWith("C") ) {
		    str = str +" "+ s;
	    } else {
		try {
		    if(s.indexOf("*") != -1 ||
			s.indexOf("+") != -1 ||
			s.indexOf("/") != -1) s = replaceMath(s);

		    value = Integer.valueOf(s);
		    if(value != null) str = str +" "+ 
			String.valueOf(value.intValue());
		} catch(NumberFormatException e) {
		    System.out.println("Syntax error at "+s);	    
		}
	    }
	}
	return str;
    }

    public String replaceMath(String str) {

	StringTokenizer tok1 = new StringTokenizer(str,"-");
	String value1;
	int i1 = 0;
	while(tok1.hasMoreTokens()){
	    value1 = tok1.nextToken().trim();
	    StringTokenizer tok2 = new StringTokenizer(value1,"+");
	    String value2;
	    int i2 = 0;
	    while(tok2.hasMoreTokens()){
	        value2 = tok2.nextToken().trim();
	        StringTokenizer tok3 = new StringTokenizer(value2,"/");
	        String value3;
		int i3 = 0;
	        while(tok3.hasMoreTokens()){
	            value3 = tok3.nextToken().trim();
	            StringTokenizer tok4 = new StringTokenizer(value3,"*");
	            String value4;
		    int i4 = 0;
	            while(tok4.hasMoreTokens()){
		 	value4 = tok4.nextToken().trim();
		  	try {
			    Integer v = Integer.valueOf(value4);
			    if(v != null && i4 != 0) i4 *= v.intValue();
			    else if(v != null) i4 = v.intValue();
			} catch (NumberFormatException e) {
                    	    System.out.println("Syntax error at "+value4);
			}
		    }
		    if(i3 != 0 && i4 != 0) i3 /= i4;
                    else i3 = i4;
		}
		i2 += i3;
	    }
	    if(i1 != 0) i1 -= i2;
	    else i1 = i2;
	}
	return String.valueOf(i1);
    }

    private String replaceVariables(String str, Hashtable hs) {
	//if(model == null) str;
	String key;
	String value;
	for(Enumeration e=hs.keys(); e.hasMoreElements();) {
           key = (String)e.nextElement();
           value = (String)hs.get(key);
	   if(value != null) str = str.replaceAll(key,value);
        }

	if(str.indexOf("{") != -1 && str.indexOf("}") != -1) {
	    String oldstr = str.substring(str.indexOf("{")+1,str.indexOf("}")); 
	    String newstr = replaceMath(oldstr);

	    str = str.substring(0, str.indexOf("{"))+newstr+
		str.substring(str.indexOf("}")+1);
	}
	
	return str;
    }
} 
