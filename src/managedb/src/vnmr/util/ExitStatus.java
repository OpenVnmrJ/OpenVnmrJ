/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * 
 *
 */

package vnmr.util;

import vnmr.ui.*;

/********************************************************** <pre>
 * Summary: Dummy exit status for managedb.
 *
 *      FillDBManager.java makes calls to this to determine whether or
 *      not we have been told to exit.  For managedb, this does not apply,
 *      so this is a dummy to satisify the call.
 </pre> **********************************************************/

public class ExitStatus {

    static public boolean exiting() {
        return false;
    }

}
