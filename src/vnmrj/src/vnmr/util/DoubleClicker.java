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

import java.io.*;
import java.util.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.templates.*;


public class DoubleClicker extends MouseAdapter
{
    protected JList list;
    private   VjToolBar		toolbar	       = null;
    private   AddToolsDialog    toolsEditor    = null;
    // private   boolean 	  initClicked    = true;
    // private   ExperimentIF     expIF       = null;

    public DoubleClicker( JList ls )
    {
	list = ls;
        // expIF = ( ExperimentIF ) Util.getAppIF();
	// toolbar = expIF.getToolBar();
    }

    public void mouseClicked( MouseEvent e )
    {
/*
        if (initClicked)
	{
	   toolsEditor = toolbar.getToolsEditor();
	   initClicked = false;
        }
*/
        if (toolsEditor == null) {
             toolbar = Util.getToolBar();
             if (toolbar == null)
                    return;
	     toolsEditor = toolbar.getToolsEditor();
	     if( toolsEditor == null )
                    return;
        }
	if( e.getClickCount() == 2 )
	{
	    int index = list.locationToIndex( e.getPoint() );
	    String selectedImg = toolsEditor.getImgFile( index );

	    list.ensureIndexIsVisible( index );
	    toolbar.updateToolImg( selectedImg );
	    toolsEditor.keepTrackDataFields( false );
	}
    }
}
