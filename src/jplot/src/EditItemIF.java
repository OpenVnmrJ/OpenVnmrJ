/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * 
 *
 */

import java.awt.*;
import java.io.*;

public interface  EditItemIF {
    public void drawItem(Graphics g);
    public void drawItem();
    public void move(int dx, int dy, int step);
    public void move(Graphics g, int dx, int dy, int step);
    public void highlight (boolean hilit);
    public Rectangle getDim();
    public Rectangle getRegion();
    public Rectangle getVector();
    public int getType();
    public void setFgColor (Color fg);
    public void setColors (Color fg, Color bg, Color hg);
    public void setRatio (double r, int pw, int ph);
    public void setTextInfo(TextInput t);
    public void setPreference();
    public void showPreference();
    public void writeToFile(PrintWriter os);

} // EditItemIF

