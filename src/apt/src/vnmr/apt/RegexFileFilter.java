/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.io.File;
import java.io.FilenameFilter;
import java.util.regex.Pattern;

public class RegexFileFilter implements FilenameFilter {
    private Pattern m_pattern;

    public RegexFileFilter(String regex) {
        m_pattern = Pattern.compile(regex);
    }

    public boolean accept(File dir, String name) {
        return m_pattern.matcher(name).matches();
    }
}
