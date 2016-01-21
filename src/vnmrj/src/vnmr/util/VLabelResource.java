/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import java.io.*;

/**
 * VLabelResource creates a PropertyResourceBundle using a UTF reader:
 *    m_ResourceBundle = new PropertyResourceBundle(getUTFReader(path));
 * where path may be a baseName (where "PROPERTIES/" + baseName will be used)
 * or fullpath of baseName (with/without .properties)
 * or full path of xml file (where baseName of xml file will be used).
 *
 * getUTFReader finds property file based on locale, then
 * use FileUtil.getUTFReader(new File(path)) to create a reader.
 *
 * locale = Locale.getDefault(), assuming FileUtil already set it properly. 
 *
 * The format of property file is
 * key=value
 * Both key and value are String, and spaces are allowed.
 * 
 * If characters cannot be represented in ISO-8859-1, property files need
 * to be UTF-8 or UTF-16 encoding.
 *
 * To get the value of a key:
 * VLabelResource resource = new VLabelResource(baseName); 
 * String value = resource.getString(key);
 * 
 * getString returns the key back if failed to create m_resourceBundle 
 * for the given baseName, or no match for the key.
 * 
**/

public class VLabelResource {

    private Hashtable<String,String> m_ResourceBundle = null;

    private String enc = "UTF-8";

    public VLabelResource(String str) {
	BufferedReader reader = getUTFReader(str);

        if(reader != null) {

	  if(m_ResourceBundle == null) 
		m_ResourceBundle = new Hashtable<String, String>();
	  else m_ResourceBundle.clear();
	  String tmpstr;
          String line;
          String key;
          String value;
          StringTokenizer tok;
          try {
            while ((line = reader.readLine()) != null) {
              if (line.startsWith("#") ) continue;

	      key = "";
	      value = "";
              if (line.startsWith("`") && line.endsWith("`")) {
                 tok = new StringTokenizer(line, "`");
                 if(tok.countTokens() < 2) continue;
		 while(tok.hasMoreTokens()) {
		   tmpstr = tok.nextToken().trim();
		   if(tmpstr.length() > 0 && key.length() > 0) {
                     value = tmpstr;
		   } else if(tmpstr.length() > 0) {
		     key = tmpstr;
		   }
		 }
              } else {
                 tok = new StringTokenizer(line, "=");
                 if(tok.countTokens() < 2) continue;
                 key = tok.nextToken().trim();
                 value = tok.nextToken().trim();
	      }
//System.out.println("VLabelResource " +" "+ key+" "+value);
	      if(key.length() > 0 && value.length() > 0)
                  m_ResourceBundle.put(key, value);
            }
          } catch (Exception e) {
	     m_ResourceBundle = null;
          }

	} else {
	     m_ResourceBundle = null;
	} 
    }

    public Hashtable getLabelResource() {
	return m_ResourceBundle;
    }

    public String getString(String name) {
	if(name == null || name.length() < 0) return "";
 	if(m_ResourceBundle == null || m_ResourceBundle.size() < 1) return name;

	String key;
      	for(Enumeration en = m_ResourceBundle.keys(); en.hasMoreElements(); ) {
            key = (String) en.nextElement();
	    if(name.trim().equals(key)) return (String)m_ResourceBundle.get(key);
        }
	// no match. return if name is a single character
 	if(name.length() < 2) return name;
	// otherwise try ignore case
      	for(Enumeration en = m_ResourceBundle.keys(); en.hasMoreElements(); ) {
            key = (String) en.nextElement();
	    if(name.trim().equalsIgnoreCase(key)) return (String)m_ResourceBundle.get(key);
        }
        return name;
    }

    public void writeResourceFile(String path) {
 	if(m_ResourceBundle == null || m_ResourceBundle.size() < 1) {
	  Messages.postWarning("Resource file "+path+" is empty.");
	  return;
	}
	try
        {
	    PrintWriter out = new PrintWriter(new FileWriter(path));

            String key;
            String value;

      	    for(Enumeration en = m_ResourceBundle.keys(); en.hasMoreElements(); ) {
              key = (String) en.nextElement();
	      if(key != null) {
	        value = (String)m_ResourceBundle.get(key);
		if(value != null) {
		  out.println(key+"="+value);
		}
	      }
            }
            out.close();
        }
        catch(IOException e)
        {
	    Messages.postError(new StringBuffer().append("error writing " ).
                               append( path).toString());
            Messages.writeStackTrace(e);
        }
    }

    public BufferedReader getUTFReader(String str) {
    //str may be a baseName, or fullpath of default property file (i.e., .properties
    // without _locale), or full path of xml file (baseName of xml file will be used).  

	BufferedReader reader = null;

	if(str.indexOf(File.separator) == -1 ) {
	    str = "PROPERTIES/" + str;
	} else if(str.endsWith(".xml")) {
	    str = str.substring(0, str.lastIndexOf(".xml"));
	} else if(str.endsWith(".properties")) {
	    str = str.substring(0, str.lastIndexOf(".properties"));
	} 

        String language = Locale.getDefault().getLanguage();
        String country = Locale.getDefault().getCountry();
        String variant = Locale.getDefault().getVariant();

	String path;
	String ext = "";
	InputStream in;
        if(country !=null && country.length() > 0 && 
	   variant !=null && variant.length() > 0) {
            ext = "_"+language+"_"+country+"_"+variant+".properties";
	    path = FileUtil.openPath(str + ext);
	    if(path != null) {
	      try {
		in = new FileInputStream(path);
		//return new BufferedReader(new InputStreamReader(in, enc));
		return new BufferedReader(FileUtil.getUTFReader(new File(path)));
	      } catch (Exception e) {
		return null;
	      }
	    }
        }
	
	variant = "";
        if(country !=null && country.length() > 0) {
            ext = "_"+language+"_"+country+".properties";
	    path = FileUtil.openPath(str + ext);
	    if(path != null) {
	      try {
		in = new FileInputStream(path);
		//return new BufferedReader(new InputStreamReader(in, enc));
		return new BufferedReader(FileUtil.getUTFReader(new File(path)));
	      } catch (Exception e) {
		return null;
	      }
	    }
        }

	country = "";
        if(language !=null) {
            ext = "_"+language+".properties";
	    path = FileUtil.openPath(str + ext);
	    if(path != null) {
	      try {
		in = new FileInputStream(path);
		//return new BufferedReader(new InputStreamReader(in, enc));
		return new BufferedReader(FileUtil.getUTFReader(new File(path)));
	      } catch (Exception e) {
		return null;
	      }
	    }
        } 

	language = "";
        ext = ".properties";
	path = FileUtil.openPath(str + ext);
	if(path != null) {
	      try {
		in = new FileInputStream(path);
		//return new BufferedReader(new InputStreamReader(in, enc));
		return new BufferedReader(FileUtil.getUTFReader(new File(path)));
	      } catch (Exception e) {
		return null;
	      }
	}
	else { 
	  return null;
	}
    }
}
