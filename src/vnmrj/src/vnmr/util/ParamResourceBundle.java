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

/**
 * ParamResourceBundle maintains a static VResourceBundle object 
 * for to "PROPERTIES/paramResources"
 *
 * The following public static methods are available:
 *      public static ParamResourceBundle getParamResourceBundle();
 *      public static String getValue(String key);
 *
 * To get the value of a key:
 * ParamResourceBundle resource = ParamResourceBundle.getParamResourceBundle();
 * if(resource != null) String str = resource.getString(key);
 *
 * or
 * String str = ParamResourceBundle.getValue(key);
 *
 * getParamResourceBundle returns null if failed to create resource.
 * getString or getValue returns key if no match for the key. 
 *
**/

public class ParamResourceBundle {

    private static VResourceBundle m_paramResourceBundle = null;

    private static final String m_baseName = 
		"PROPERTIES/paramResources";

    public static String getValue(String key) {

        if(m_paramResourceBundle == null) {
            m_paramResourceBundle = new VResourceBundle(m_baseName);
        }

	if(m_paramResourceBundle != null) {
	    return m_paramResourceBundle.getString(key);
	} else {
	    return key;
	}
    }

    public static VResourceBundle getParamResourceBundle() {

	if(m_paramResourceBundle == null) { 
	    m_paramResourceBundle = new VResourceBundle(m_baseName);
	}

	return m_paramResourceBundle;
    }

    public static void reset() {
	m_paramResourceBundle = null;
    }
}
