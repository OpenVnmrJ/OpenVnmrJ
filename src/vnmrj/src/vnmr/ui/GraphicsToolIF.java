/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;

public interface GraphicsToolIF {

    public void setOrientation( int o );
    public int  getOrientation();
    public void setDockPosition( int o );
    public int  getDockPosition();
    public void setDefaultOrientation( int o );
    public int  getDefaultOrientation();
    public void setDockTopPosition( int o );
    public void setDockBottomPosition( int o );
    public void setToolBar(Component o);
    public Component getToolBar();
    public void resetSize();
    public void setPosition(int x, int y); // setLocation
    public Point getPosition(); // getLocation
    public void setShow(boolean b);  // setVisible
    public boolean isShow();  // isVisible
    public Rectangle getBounds();
    public void  setBounds(int x, int y, int w, int h);
    public int getPreferredWidth();
    public void adjustDockLocation();
}
