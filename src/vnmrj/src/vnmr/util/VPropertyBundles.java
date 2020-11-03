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

}
