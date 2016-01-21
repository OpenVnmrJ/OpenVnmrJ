/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

public class Polyline
{
    public int length;
    public int[] x;
    public int[] y;

    public Polyline(int length) {
	this.length = length;
	x = new int[length];
	y = new int[length];
    }
}
