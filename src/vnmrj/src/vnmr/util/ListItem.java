/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import javax.swing.Icon;


/**
 * This class is used in conjunction with ListItemRenderer to hold
 * icon/name pairs to display in a JList.
 */
public class ListItem
{
    protected Icon   icon;
    protected String name;

    /**
     * Construct an item with the given icon and file name.
     * @param s The file name.
     * @param i The icon.
     */
    public ListItem( String s, Icon i )
    {
	icon = i;
	name = s;
    }

    public Icon getIcon()
    {
	return icon;
    }

    public String getName()
    {
	return name;
    }
}


