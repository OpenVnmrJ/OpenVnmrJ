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

public interface PlotObj
{
	public void setColors(Color a, Color b, Color c);
	public void setHilitColor(Color c);
	public void setFgColor(Color c);
	public void setLineWidth(int c);
	public void setPreference();
	public void showPreference();
	public void setPrefWindow(PlotItemPref p);
}

