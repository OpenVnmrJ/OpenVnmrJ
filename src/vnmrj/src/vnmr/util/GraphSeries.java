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

public interface GraphSeries {
    public void setColor(Color c);
    public Color getColor(); 
    public void setValid(boolean b);
    public boolean isValid();
    public void setXorMode(boolean b);
    public boolean isXorMode();
    public void setTopLayer(boolean b);
    public boolean isTopLayer();
    public void setLineWidth(int n);
    public int getLineWidth();
    public void setAlpha(float n);
    public float getAlpha();
    public boolean intersects(int x, int y, int x2, int y2);
    public void setConatinerGeom(int x, int y, int w, int h);
    public void draw(Graphics2D g, boolean bVertical, boolean bRight);
}
