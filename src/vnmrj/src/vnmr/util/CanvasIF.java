/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.io.*;
import java.awt.image.IndexColorModel;
import vnmr.ui.*;

public interface CanvasIF {
	public Dimension getWinSize();
	public void setTearGC(Graphics g1, Graphics g2);
	public void registerTearOff(VnmrTearCanvas b);
	public void graphFunc(DataInputStream in, int type);
	public void drawText(String s, int x, int y, int l);
        public void drawIcon(String strValue);
        public VIcon getIcon();
	public void setTextColor(int c);
	public void setBusy(boolean s);
	public void setActive(boolean s);
	public void freeObj();
	public void sendMoreEvent(boolean s);
	public void processCommand(String s);
	public void setGraphicRegion();
	public void enableDrag();
	public void disableDrag();
	public void resetDrag();
	public boolean dragEnabled();
	// public void setCursor();
	public void setSuspend(boolean s);
	public void refresh();
        public Color getColor(int c);

} // class CanvasIF
