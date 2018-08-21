/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import javax.swing.JSeparator;

public interface  VMenuitemIF {
    public void setVParent(Container p);
    public Container getVParent();
    public String getAttribute(int nAttr);
    public void changeLook();
    public void updateMe();
    public void initItem();
    public void mainMenuBarItem(boolean b);
    public void setSeparator(JSeparator obj);
    public boolean setShownListener(VSubMenu obj);
    public void setLastShownObj(boolean b);
}

