/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.util.*;

import vnmr.ui.*;
import vnmr.util.*;

/**
 * <p>Title: WTrashItem</p>
 * <p>Description: Contains info about trashentry, this class is mirrored from
 *                  TrashItem but does not use shuffler and for now updates the trashicon.</p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 */

public class WTrashItem
{

    static private TrashCan trashCan;

    public WTrashItem()
    {

    }

    /******************************************************************
     * Summary: Save the TrashCan item in a static so that we can
     *          change the icon when needed.
     *
     *
     *****************************************************************/
    static public void setTrashCan(TrashCan tc) {
        trashCan = tc;
    }

    /******************************************************************
     * Summary: Change the TrashCan icon to the full one.
     *
     *
     *****************************************************************/
    static public void fullTrashCanIcon()
    {
        if (trashCan != null)
            trashCan.setIcon(Util.getImageIcon("trashcanFull.gif"));
    }

    /******************************************************************
     * Summary: Change the TrashCan icon to the empty one.
     *
     *
     *****************************************************************/
    static public void emptyTrashCanIcon()
    {
        if (trashCan != null)
            trashCan.setIcon(Util.getImageIcon("trashcan.gif"));
    }
}
