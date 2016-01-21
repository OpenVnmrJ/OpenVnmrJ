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
import java.net.*;

/**
 * VPropertyBundles maintains a static Hashtable for VResourceBundles,
 * using baseNames as keys, and VResourceBundles as values.
 *
 * The following public static methods are available:
 *   public static VResourceBundle getResourceBundle(String baseName);
 *   public static String getValue(String baseName, String key);
 *   public static void reset(); 
 *   public static boolean updateUserPropertyfile();
 *
 * property file(s) of given baseName should exist, or getResourceBundle
 * returns null, and getValue returns key.
 *
 * existing property files:
 * PROPERTIES/cmdResources.properties  (baseName = "cmdResources");
 * PROPERTIES/paramResources.properties  (baseName = "paramResources");
 * PROPERTIES/userResources<_host>.properties  (baseName = "userResources");
 *
 * To get the value of a key:
 * VResourceBundle resource = VResourceBundle.getResourceBundle(baseName);
 * if(resource != null) String str = resource.getString(key);
 *
 * or
 * String str = VResourceBundle.getValue(baseName, key);
 *
**/

public class VPropertyBundles {

    private static Hashtable m_propertyBundles = new Hashtable();

    public static VResourceBundle getResourceBundle(String baseName) {

	if(!m_propertyBundles.containsKey(baseName)) {
	    m_propertyBundles.put(baseName, new VResourceBundle(baseName));
	}
	return (VResourceBundle)m_propertyBundles.get(baseName);
    }

    public static String getValue(String baseName, String key) {

	if(!m_propertyBundles.containsKey(baseName)) {
	    m_propertyBundles.put(baseName, new VResourceBundle(baseName));
        }
	VResourceBundle bundle = (VResourceBundle)m_propertyBundles.get(baseName);
	if(bundle != null) {
            return bundle.getString(key);
        }
	else return key;
    }

    public static void reset() {
	//call this when locale changed.
	m_propertyBundles.clear();
    }

    public static boolean updateUserPropertyfile() {
// this is not used anywhere. don't remember why wrote it. 

	String host = Util.getHostName();

	if(host.length()>0) host = "_" + host;

	String filepath = "PROPERTIES/userResources" + host + ".properties";
	String propertyfile = FileUtil.savePath(filepath);
        if(propertyfile == null) return false;

	String sysProf = FileUtil.openPath("SYSTEM/SYSPROF");
	if(sysProf == null) return false;

	File file=new File(sysProf);
	File[] children = file.listFiles();
	if(children == null) return false;

	FileWriter fw;
        PrintWriter os;
	try
        {
          fw = new FileWriter(propertyfile, false);
          os = new PrintWriter(fw);

          for(int i = 0; i < children.length; i++) {
            String child = children[i].getPath();
            String path = FileUtil.openPath(child);
            if(path == null) continue;

            BufferedReader in;
            String inLine;

            try {
                in = new BufferedReader(new FileReader(path));
            	while ((inLine = in.readLine()) != null) {
                    if (inLine.length() > 1 && !inLine.startsWith("#")) {
                        inLine.trim();

                        StringTokenizer tok = new StringTokenizer(inLine, " \t\n", false);
                    	if (tok.countTokens() > 1) {
                            String key = tok.nextToken();
                            if(key.equals("name")) {
                                String value = "";
                                while(tok.hasMoreElements()){
                                    value+=tok.nextToken(" \t");
                                    if(tok.hasMoreElements())
                                        value+=" ";
                                }
                                if(path.endsWith(".del")) value += "*";
                                os.println(children[i].getName() +"="+ value);
                                break;
                            }
                        }
                    }

                }
                in.close();
            } catch(IOException e) {
                Messages.writeStackTrace(e);
                Messages.postDebug(e.toString());
                return false;
            }
        }

        fw.flush();
        fw.close();
    }
    catch(IOException e)
    {
        Messages.writeStackTrace(e);
        Messages.postDebug(e.toString());
	    return false;
    }

	return true;
    }
}
