/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import javax.print.attribute.PrintRequestAttribute;

public class SunPageSelection implements PrintRequestAttribute {

    public static final SunPageSelection ALL = new SunPageSelection(0);
    public static final SunPageSelection RANGE = new SunPageSelection(1);
    public static final SunPageSelection SELECTION = new SunPageSelection(2);
    public SunPageSelection(int value) {
    }

    public final Class<SunPageSelection> getCategory() {
        return SunPageSelection.class;
    }

    public final String getName() {
        return "sun-page-selector";
    }
    
}

