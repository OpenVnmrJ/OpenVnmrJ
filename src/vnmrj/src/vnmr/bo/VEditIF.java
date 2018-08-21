/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

/**
 * An interface that some VObjIF components implement to control what
 * attributes the ParamInfoPanel allows to be edited.
 * @see vnmr.util.ParamInfoPanel
 */
public interface  VEditIF {
    /**
     * Get a list of attributes to be edited and prompting messages.
     * @return
     * An array of Object arrays.  The primary dimension is the number
     * of attributes.  Each Object array contains, first, the
     * attribute index wrapped as an Integer object, and, second, a
     * String appropriate for prompting for the attrubute value.  More
     * fields may be added in the future to indicate the type of
     * widget to use the set the value, choice options, etc.
     */
    public Object[][] getAttributes();
    public void setAttribute(int t, String s);
    public String getAttribute(int t);
}

