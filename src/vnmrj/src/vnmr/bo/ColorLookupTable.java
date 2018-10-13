/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import vnmr.util.*;


public class ColorLookupTable
{
    private int[] table;
    public double minData = 0;
    public double maxData = 0.01;
    private int uflowColor = -1;
    private int oflowColor = -1;
    public String function = "";

    public ColorLookupTable() {
	table = new int[0];
    }

    public ColorLookupTable(double minData, double maxData) {
	table = new int[0];
	this.minData = minData;
	this.maxData = maxData;
    }

    public ColorLookupTable(int length) {
	table = new int[length];
    }

    public ColorLookupTable(int[] table) {
	this.table = table;
    }

    public ColorLookupTable(int length, int uflow, int oflow,
			    double minData, double maxData) {
	table = new int[length];
	this.uflowColor = uflow;
	this.oflowColor = oflow;
	this.minData = minData;
	this.maxData = maxData;
    }

    public int[] getTable() { return table; }
    public void setTable(int[] t) { table = t; }
    public void setFunction(String func) { function = func;}
    public void setUflowColor(int color) { uflowColor = color;}
    public void setOflowColor(int color) { oflowColor = color;}

    private static int cltId = 0;
    public String toString() {
        if (++cltId == 0) { ++cltId; }
	int size = table.length * 4 + 80; // Just approximate
	StringBuffer out = new StringBuffer(size);
        out.append("id "+cltId+"\n");
	out.append("function "+function+"\n");
	out.append("minData " + Fmt.fg(6, minData) + "\n");
	out.append("maxData " + Fmt.fg(6, maxData) + "\n");
	out.append("uflowColor "+uflowColor+"\n");
	out.append("oflowColor "+oflowColor+"\n");
	out.append("size "+table.length+"\n");
	out.append("data:\n");
	for (int i = 0; i< table.length; i++) {
	    out.append(table[i]+"\n");
	}
	return out.toString();
    }
}
