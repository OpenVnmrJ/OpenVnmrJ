/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
 * VSpaceQuotedStringTokenizer.java
 *
 * Created on March 8, 2006, 2:38 PM
 *
 */

package vnmr.util;

import java.io.*;
import java.util.*;
import java.util.regex.*;

/**
 * This class uses regular expressions to parse the string with spaces, and
 * if there are quotes surrounding then it preserves the spaces in the quoted part
 * of the string.
 *
 * @author Mamta
 */
public class VSpaceQuotedStringTokenizer {

    protected String m_strValue;
    protected Matcher m_matcher;
    protected static final String REGEX = "(\"[^\"]*\")|([^ \"]+)";

    /** Creates a new instance of VSpaceQuotedStringTokenizer */
    public VSpaceQuotedStringTokenizer(String str) {
        m_strValue = str;
        m_matcher = Pattern.compile(REGEX).matcher(str);
    }

    public boolean hasMoreTokens()
    {
        return m_matcher.find();
    }

    public String nextToken()
    {
        String strTok = null;
        String g = m_matcher.group(1);

        if(g == null)
            strTok = m_matcher.group(2);
        else
            strTok = m_strValue.substring(m_matcher.start() + 1, m_matcher.end() - 1);

        return strTok;
    }

}
