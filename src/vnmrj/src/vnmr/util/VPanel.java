/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;
import java.util.*;
import javax.swing.*;
import java.beans.*;



/**
 * Title: VPanel
 * Description:  Wrapper class for JPanel, used to set background colors from DisplayOptions.
 * Copyright:    Copyright (c) 2001
 * Company:
 * @author
 * @version 1.0
 */

public class VPanel extends JPanel implements PropertyChangeListener
{

    public VPanel()
    {
	super();
	setBackground(Util.getBgColor());
	DisplayOptions.addChangeListener(this);
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
	setBackground(Util.getBgColor());
    }
}
