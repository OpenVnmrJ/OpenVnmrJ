/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;
    
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

import javax.swing.text.*;
import javax.swing.border.*;


import java.io.*;
import java.util.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.templates.*;

public class ListItemRenderer extends Object
implements ListCellRenderer
{
    JLabel  theLabel;

    public ListItemRenderer()
    {
	theLabel = new JLabel();
	theLabel.setOpaque( true );
    }

    public Component getListCellRendererComponent( 
						    JList list,
						    Object value,
						    int index,
						    boolean isSelected,
						    boolean cellHasFocus )
    {
	ListItem li = null;

	if( value instanceof ListItem )
	    li = ( ListItem ) value;

	Font fond = new Font( "SanSerif", Font.PLAIN, 12 );
	theLabel.setFont( fond );

	if( isSelected )
	{
	    theLabel.setBackground( list.getSelectionBackground() );
	    theLabel.setForeground( list.getSelectionForeground() );
	}
	else
	{
	    theLabel.setBackground( list.getBackground() );
	    theLabel.setForeground( list.getForeground() );
	}

	if( li != null )
	{
	    theLabel.setText( li.getName() );
	    theLabel.setIcon( li.getIcon() );
	}
	else
	{
	    theLabel.setText( value.toString() );
	}
 
	return theLabel;
    }
}						

