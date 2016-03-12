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
import java.util.Comparator;

/**
 * Compares two persistence file names for ordering.
 * @param <T> Must be a File to avoid a class cast exception.
 */
public class PFileNameComparator<T> implements Comparator<T> {

    @Override
    /**
     * Compare the name of File o1 with that of o2.
     * @param o1 The first file.
     * @param o2 The other file.
     * @return Negative if o1 before o2, positive if after, otherwise 0.
     * @throws ClassCastException if the T is not a File.
     */
    public int compare(T o1, T o2) {
        String s1 = ((File)o1).getName();
        String s2 = ((File)o2).getName();
        try {
            int i1 = Integer.parseInt(s1.substring(s1.indexOf('#') + 1));
            int i2 = Integer.parseInt(s1.substring(s2.indexOf('#') + 1));
            if (i1 < i2) {
                return -1;
            } else if (i1 > i2) {
                return 1;
            }
        } catch (Exception e) {
        }
        return s1.compareTo(s2);
    }
}
