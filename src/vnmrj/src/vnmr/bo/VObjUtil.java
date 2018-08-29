/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;


public class  VObjUtil {
    private static VObjIF focusedObj = null;

    public static void setFocusedObj(VObjIF obj) {
        focusedObj = obj;
    }

    public static void removeFocusedObj(VObjIF obj) {
        if (obj == null || obj != focusedObj)
            return;
        focusedObj = null;
    }

    public static VObjIF getFocusedObj() {
        return focusedObj;
    }
}

