/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * CloseTabbedPane.java
 * Created on June 2, 2007, 6:33 PM
 *
 * @author frits
 */

package accounting;

import java.awt.event.MouseEvent;
import javax.swing.*;
import java.util.EventListener;


public class CloseTabbedPane extends JTabbedPane {
    
    private int overTabIndex = -1;
    private CloseTabPaneUI paneUI;
    
    /** Creates a new instance of CloseTabbedPane */
    public CloseTabbedPane() {
        setTabLayoutPolicy(JTabbedPane.SCROLL_TAB_LAYOUT);
       paneUI = new CloseTabPaneUI();
       setUI(paneUI);
    }
    
    /**
     * Remove the 'overTabIndex; tab
     */
    public void fireCloseTabEvent(MouseEvent e, int overTabIndex) {
        System.out.println("fireCloseTabEvent fired");
	this.overTabIndex = overTabIndex;
        remove (overTabIndex);
    }
}
    
  