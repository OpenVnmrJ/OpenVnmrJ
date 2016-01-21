/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import javax.swing.DefaultBoundedRangeModel;

public class IntegerRangeModel extends DefaultBoundedRangeModel {
    private String name = "";
    private int[] increment = {1, 10, 100};
    private int incrIndex = 0;
    private int minReading = 0;
    private int maxReading = 0;
    private boolean isElastic = false;

    public String getName() { return name; }
    public int getIncrement() { return increment[incrIndex]; }
    public int getIncrementIndex() { return incrIndex; }
    public int getIncrement(int n) {
        return increment[ Math.abs(incrIndex + n) % 3];
    }
    public int getMinReading() { return minReading; }
    public int getMaxReading() { return maxReading; }
    public boolean getBoundsAreElastic() { return isElastic; }

    public void setName(String name) {
    this.name = name;
    fireStateChanged();	// Notify listeners
    }

    public void setIncrement(int incr) {
    increment[incrIndex] = incr;
    fireStateChanged();	// Notify listeners
    }

    public void setIncrement(int n, int incr) {
    increment[Math.abs(incrIndex + n) % 3] = incr;
    fireStateChanged();	// Notify listeners
    }

    public void setIncrements(int incr0, int incr1, int incr2) {
    increment[0] = incr0;
    increment[1] = incr1;
    increment[2] = incr2;
    incrIndex = 0;
    fireStateChanged();	// Notify listeners
    }

    public void setMinReading(int n) {
    if (n > getValue()) {
        n = getValue();
    }
    if (n < getMinimum()) {
        n = getMinimum();
    }
    if (n != minReading) {
        minReading = n;
        fireStateChanged();	// Notify listeners
    }
    }

    public void setMaxReading(int n) {
    if (n < getValue() + getExtent()) {
        n = getValue() + getExtent();
    }
    if (n > getMaximum()) {
        n = getMaximum();
    }
    if (n != maxReading) {
        maxReading = n;
        fireStateChanged();	// Notify listeners
    }
    }

    public void setBoundsAreElastic(boolean b) {
    if (b != isElastic) {
        isElastic = b;
        fireStateChanged();	// Notify listeners
    }
    }

    public void setValue(int n) {
    int extent = getExtent();
    int minimum = getMinimum();
    int maximum = getMaximum();
    if (n > maximum - extent) {
        if (isElastic) {
        maximum = getNextMaximum(n + extent);
        } else {
        n = maximum - extent;
        }
    } else if (n < minimum) {
        if (isElastic) {
        minimum = n;
        } else {
        n = minimum;
        }
    }
    if (n < minReading) {
        minReading = n;
    } else if (n + extent > maxReading) {
        maxReading = n + extent;
    }
    // NB: If max or min Reading changed, value must have changed,
    //     so listeners will be notified by the following:
    setRangeProperties(n, extent, minimum, maximum,
               getValueIsAdjusting());
    }

    public void increment() {
    setValue(getValue() + increment[incrIndex]);
    }

    public void decrement() {
    setValue(getValue() - increment[incrIndex]);
    }

    public void toggleIncrement() {
    //incrIndex ^= 1;
    incrIndex = (incrIndex +1 ) % 3;
    fireStateChanged();	// Notify listeners
    }

    public void setIncrementIndex(int incr) {
    incrIndex = incr % 3;
    fireStateChanged();	// Notify listeners
    }

    public String toString() {
    String modelString = getIntegerModelString();
    return getClass().getName() + "[" + modelString + "]";
    }

    protected String getIntegerModelString() {
    return
        "name=" + getName() +
        "value=" + getValue() +
            ", extent=" + getExtent() +
            ", min=" + getMinimum() +
            ", max=" + getMaximum() +
        ", adj=" + getValueIsAdjusting() +
        ", inc0=" + getIncrement(0) +
        ", inc1=" + getIncrement(1) +
        ", inc2=" + getIncrement(2) +
        ", minReading=" + getMinReading() +
        ", maxReading=" + getMaxReading() +
        ", elastic=" + getBoundsAreElastic();
    }

    private double maxBounds[] = {1.0, 2.0, 5.0};

    public int getNextMaximum(int newValue) {
    int maximum = newValue;
    int i;
    double ln10 = Math.log(10.0);
    double logv = Math.log(newValue) / ln10;
    double floor = Math.floor(logv);
    double rem = logv - floor;
    double tst;
    double ltst;

    if (maximum <= 0) {
        return 0;
    }
    for (i=0; i<maxBounds.length; i++) {
        tst = (i >= maxBounds.length-1 ? maxBounds[0]*10 : maxBounds[i+1]);
        ltst = Math.log(tst) / ln10;
        if (rem < ltst) {
        maximum = (int) Math.rint(tst * Math.pow(10.0, floor));
        break;
        }
    }
    return maximum;
    }
}
