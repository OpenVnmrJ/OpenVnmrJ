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
 * VResourceBundle creates a PropertyResourceBundle using a UTF reader:
 * m_ResourceBundle = new PropertyResourceBundle(getUTFReader(path)); where path
 * may be a baseName (where "PROPERTIES/" + baseName will be used) or fullpath
 * of baseName (with/without .properties) or full path of xml file (where
 * baseName of xml file will be used).
 * 
 * getUTFReader finds property file based on locale, then use
 * FileUtil.getUTFReader(new File(path)) to create a reader.
 * 
 * locale = Locale.getDefault(), assuming FileUtil already set it properly.
 * 
 * The format of property file is key=value Both key and value are String, and
 * spaces are allowed.
 * 
 * If characters cannot be represented in ISO-8859-1, property files need to be
 * UTF-8 or UTF-16 encoding.
 * 
 * To get the value of a key: VResourceBundle resource = new
 * VResourceBundle(baseName); String value = resource.getString(key);
 * 
 * getString returns the key back if failed to create m_resourceBundle for the
 * given baseName, or no match for the key.
 * 
 **/

public class VResourceBundle {

    private PropertyResourceBundle m_ResourceBundle = null;

    private String enc = "UTF-8";

    public VResourceBundle(String str) {
        BufferedReader reader = getUTFReader(str);

        if (reader != null) {

            try {
                m_ResourceBundle = new PropertyResourceBundle(reader);
                reader.close();
            } catch (IOException e) {
                m_ResourceBundle = null;
            } catch (NullPointerException e) {
                m_ResourceBundle = null;
            }
        }
        
    }

    public String getString(String key) {
        if (m_ResourceBundle == null)
            return key;

        try {
            String value = m_ResourceBundle.getString(key);
            return value;
            // } catch (MissingResourceException e) {
        } catch (Exception e) {
            return key;
        }
    }

    public BufferedReader getUTFReader(String str) {
        // str may be a baseName, or fullpath of default property file (i.e.,
        // .properties
        // without _locale), or full path of xml file (baseName of xml file will
        // be used).

        BufferedReader reader = null;

        if (str.indexOf(File.separator) == -1) {
            str = "PROPERTIES/" + str;
        } else if (str.endsWith(".xml")) {
            str = str.substring(0, str.lastIndexOf(".xml"));
        } else if (str.endsWith(".properties")) {
            str = str.substring(0, str.lastIndexOf(".properties"));
        }

        String language = Locale.getDefault().getLanguage();
        String country = Locale.getDefault().getCountry();
        String variant = Locale.getDefault().getVariant();

        String path;
        String ext = "";
        InputStream in;
        if (country != null && country.length() > 0 && variant != null
                && variant.length() > 0) {
            ext = "_" + language + "_" + country + "_" + variant
                    + ".properties";
            path = FileUtil.openPath(str + ext);
            if (path != null) {
                try {
                    in = new FileInputStream(path);
                    // return new BufferedReader(new InputStreamReader(in,
                    // enc));
                    return new BufferedReader(FileUtil.getUTFReader(new File(
                            path)));
                } catch (Exception e) {
                    return null;
                }
            }
        }

        variant = "";
        if (country != null && country.length() > 0) {
            ext = "_" + language + "_" + country + ".properties";
            path = FileUtil.openPath(str + ext);
            if (path != null) {
                try {
                    in = new FileInputStream(path);
                    // return new BufferedReader(new InputStreamReader(in,
                    // enc));
                    return new BufferedReader(FileUtil.getUTFReader(new File(
                            path)));
                } catch (Exception e) {
                    return null;
                }
            }
        }

        country = "";
        if (language != null) {
            ext = "_" + language + ".properties";
            path = FileUtil.openPath(str + ext);
            if (path != null) {
                try {
                    in = new FileInputStream(path);
                    // return new BufferedReader(new InputStreamReader(in,
                    // enc));
                    return new BufferedReader(FileUtil.getUTFReader(new File(
                            path)));
                } catch (Exception e) {
                    return null;
                }
            }
        }

        language = "";
        ext = ".properties";
        path = FileUtil.openPath(str + ext);
        if (path != null) {
            try {
                in = new FileInputStream(path);
                // return new BufferedReader(new InputStreamReader(in, enc));
                return new BufferedReader(FileUtil.getUTFReader(new File(path)));
            } catch (Exception e) {
                return null;
            }
        } else {
            return null;
        }
    }
}
