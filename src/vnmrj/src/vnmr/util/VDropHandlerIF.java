/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.dnd.*;
import java.awt.datatransfer.*;
import javax.swing.*;

public interface  VDropHandlerIF {
    public void processDrop(DropTargetDropEvent e, JComponent c, boolean bEdit);
}
