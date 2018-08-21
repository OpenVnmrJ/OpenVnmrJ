/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.Color;

public interface VColorMapIF {
    public void setRgbs(int index, int[] r, int[] g, int[] b);
    public void setRgbs(int[] r, int[] g, int[] b);
    public void setRgbs(int index, byte[] r, byte[] g, byte[] b, byte[] a);
    public void setRgbs(byte[] r, byte[] g, byte[] b, byte[] a);
    public void setOneRgb(int index, int r, int g, int b);
    public void setColor(int index, Color c);
    public void setSelectedIndex(int i);
    public void setColorEditable(boolean b);
    public void setColorNumber(int n);
    public void addColorEventListener(VColorMapIF l);
    public void clearColorEventListener();
    public int[] getRgbs();
    public Color[] getColorArray();
}

