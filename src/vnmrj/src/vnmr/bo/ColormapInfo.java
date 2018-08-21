/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

public class ColormapInfo
{
    private String label;
    private int firstColor;
    private int numColors;
    private int zeroColor;
    private int uflowColor;
    private int oflowColor;
    private boolean negative = false;

    public ColormapInfo(String label,
			int firstColor,
			int numColors) {
	this.label = label;
	this.firstColor = firstColor;
	this.numColors = numColors;
	this.zeroColor = -1;
	this.uflowColor = firstColor;
	this.oflowColor = firstColor + numColors - 1;
    }

    public String getLabel() { return label; }
    public int getFirstColor() { return firstColor; }
    public int getNumColors() { return numColors; }
    public int getLastColor() { return firstColor + numColors - 1; }
    public int getZeroColor() { return zeroColor; }
    public int getUflowColor() { return uflowColor; }
    public int getOflowColor() { return oflowColor; }
    public boolean getNegative() { return negative; }

    public void setLabel(String s) { label = s; }
    public void setFirstColor(int i) { firstColor = i; }
    public void setNumColors(int i) { numColors = i; }
    public void setZeroColor(int i) { zeroColor = i; }
    public void setUflowColor(int i) { uflowColor = i; }
    public void setOflowColor(int i) { oflowColor = i; }
    public void getNegative(boolean b) { negative = b; }
}
