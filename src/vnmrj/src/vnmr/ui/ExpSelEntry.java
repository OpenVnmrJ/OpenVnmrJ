/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import vnmr.util.*;


public class ExpSelEntry {
    String name, label, tab, menu1, menu2, bykeywords;
    String fullpath, fgColorTx, bgColorTx,noShow;

    HashArrayList entryList;

    public String getName() {
        return name;
    }

    public String getTab() {
        return tab;
    }

     public String getLabel() {
        return label;
    }

    public void setLabel(String newLabel) {
        label = newLabel;
    }

     public HashArrayList getEntryList() {
         return entryList;
     }

    public int entryListCount() {
        if(entryList == null)
            return 0;

        return entryList.size();
    }

    public String getBykeywords() {
        if(bykeywords == null)
            return "";
        else
            return bykeywords;
    }

    public String getFullpath() {
        return fullpath;
    }

    public void clear() {
        name = null;
        label = null;
        fgColorTx = null;
        bgColorTx = null;
        tab = null;
        menu1 = null;
        menu2 = null;
        bykeywords = null;
        fullpath = null;
        // Default noShow to false
        noShow = "false";
        // This is the list of ExpSelEntry items below this item
        // In a filled item, if this is null, the item must be
        // a protocol. Else it must be a menu and these are the
        // choices. At each level down, the same is true.
        entryList = null;
    }
}
