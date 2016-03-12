/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

package vnmr.apt;

/**
 * This class represents a closed interval in the integer number line.
 */
class Interval {

    /** The index of the first point in the interval. */
    public int start = -1;

    /** The index of the last point in the interval. */
    public int stop = -1;

    /**
     * Create an Interval with the given "start" and "stop" points.
     * @param start The index of the first point in the interval.
     * @param stop The index of the last point in the interval.
     */
    public Interval(int start, int stop) {
        this.start = start;
        this.stop = stop;
    }

    /**
     * Create an Interval with the given "start" and "stop" points.
     * @param intervalString The interval, in the form "37:295".
     */
    public Interval(String intervalString) {
        if (intervalString == null) {
            this.start = this.stop = -1;
        } else {
            try {
                String[] tokens = intervalString.split("[: ]+", 2);
                this.start = Integer.parseInt(tokens[0]);
                this.stop = Integer.parseInt(tokens[1]);
            } catch (Exception e) {
                Messages.postDebugWarning("Illegal interval spec: \""
                                          + intervalString + "\"");
            }
        }
    }

    /**
     * Determine if <code>ix</code> is in this interval.
     * That is, <code>ix</code> must satisfy:
     * <br><code> start &le; ix &le; stop </code>.
     * @param ix The integer to test.
     * @return True if <code>ix</code> is in the interval.
     */
    public boolean contains(int ix) {
        return ix <= stop && ix >= start;
    }

    /**
     * Produces a readable specification of this interval.
     * @return A string of the form "[start]:[stop]".
     */
    public String toString() {
        return start + ":" + stop;
    }
}