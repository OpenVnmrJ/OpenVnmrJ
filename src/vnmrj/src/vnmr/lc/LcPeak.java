/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;


import vnmr.util.Fmt;


/**
 * Specifies an event in the LC run.  Could be an automatically or
 * manually detected peak, or just some arbitrary time.  It's just a
 * time when we need to take some special action.
 */
public class LcPeak implements Comparable {
    /*
     * Implement Comparable to sort by peak number.
     */

    /** Peak number for this peak - should be unique - first is 0 */
    public int id;
    /** Channel number for this peak - first channel is 0 */
    public int channel;
    /** Retention time for this peak in minutes */
    public double time;
    /** The Full Width at Half Maximum in minutes */
    public double fwhm;
    /** Peak area in (minutes * detector_units) */
    public double area;
    /** Peak height in detector_units */
    public double height;
    /** What are the detector units */
    public String units;
    /** Start time for this peak in time units */
    public double startTime = -1;
    /** End time for this peak in time units */
    public double endTime = -1;
    /** Baseline Start for this peak in time units */
    public double blStartTime = -1;
    /** Baseline End for this peak in time units */
    public double blEndTime = -1;
    /** Baseline Starting point for this peak */
    public double blStartVal = 0;
    /** Baseline Ending point for this peak */
    public double blEndVal = 0;

    /**
     * Constructs a peak with the specified values.
     */
    public LcPeak(int id, int channel, double time, double fwhm,
                  double area, double height, String units, double startTime,
                  double endTime, double blStartTime, double blEndTime, 
                  double blStartVal, double  blEndVal) {
        this.id = id;
        this.channel = channel;
        this.time = time;
        this.fwhm = fwhm;
        this.area = area;
        this.height = height;
        this.units = units;
        this.startTime = startTime;
        this.endTime = endTime;
        this.blStartTime = blStartTime;
        this.blEndTime = blEndTime;
        this.blStartVal = blStartVal;
        this.blEndVal = blEndVal;
    }

    /**
     * Constructs a peak with the specified values.
     * Other variables are left set to "invalid" values.
     */
    public LcPeak(int id, int channel, double time, double fwhm,
                  double area, double height, String units) {
        this.id = id;
        this.channel = channel;
        this.time = time;
        this.fwhm = fwhm;
        this.area = area;
        this.height = height;
        this.units = units;
    }

    /**
     * Get a string that identifies this peak and channel number.
     */
    public String getPeakIdString() {
        return (channel + 1) + "-" + Fmt.d(2, id + 1, false, ' ');
    }

    /**
     * Do a primary sort on time with a secondary sort on channel.
     * Presumably, there cannot be two peaks at the same time on
     * the same channel.
     */
    public int compareTo(Object ob2) {
        LcPeak pk2 = (LcPeak)ob2;
        /*System.out.println("LcPeak.compareTo(): t1=" + time
          + ", t2=" + pk2.time);/*CMP*/
        if (time > pk2.time) {
            return 1;
        } else if (time < pk2.time) {
            return -1;
        } else {
            if (channel > pk2.channel) {
                return 1;
            } else if (channel < pk2.channel) {
                return -1;
            } else {
                return 0;
            }
        }
    }
}
