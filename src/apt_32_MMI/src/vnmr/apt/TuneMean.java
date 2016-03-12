/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.util.List;


/**
 * Utility math function for getting a stable mean of a data set.
 * Prunes the data set by throwing out bad points.
 *
 */
public class TuneMean {

    public static int deleteLeadingLowPoints(List<Double> data) {
        int n = 0;
        return n;
    }

    /**
     * Utility math function for getting a stable mean of a data set.
     * Prunes the data set by throwing out bad points.
     * Considers outlier points one at a time. The current outlier is the
     * point farthest from the mean value. If ignoring the outlier
     * substantially reduces the standard deviation, the point is thrown out
     * and the next outlier considered.
     * 
     * @param data The list of doubles to find the mean of.
     * @param debug If true, print debug messages.
     * @return A "Stats" object giving the mean and standard deviation.
     */
    public static Stats getFilteredMean(List<Double> data, boolean debug) {
        final double SDV_REDUCTION_FACTOR = 0.5;
        Stats stats = new Stats();

        int n = data.size();
        boolean[] omit = new boolean[n];
        double meanPrev;
        double sdvPrev;

        //        fprintf(stderr,"diffs={");
        //        for (i = 0; i < n; i++) {
        //            fprintf(stderr,"%.6f, ", data[i]);
        //        }
        //        fprintf(stderr,"}\n");

        for (int i = 0; i < n; i++) {
            omit[i] = false;
        }
        stats.mean = getMean(data, omit);
        stats.sdv = getSdv(data, omit, stats.mean, true);
        // Remove point with largest residual until fit stops improving
        // or we run out of points.
        boolean done = numberUsed(n, omit) <= 2 || stats.sdv == 0;
        while (!done) {
            int bigIdx;
            meanPrev = stats.mean;
            sdvPrev = stats.sdv;
            bigIdx = getOutlier(data, omit, stats.mean);
            if (bigIdx < 0) {
                // No outlier found
                done = true;
            } else {
                omit[bigIdx] = true;
                stats.mean = getMean(data, omit);
                stats.sdv = getSdv(data, omit, stats.mean, true);
                //                fprintf(stderr,"SDV reduction factor[%d]: %f\n",
                //                        bigIdx, stats.sdv / sdvPrev);
                if ((stats.sdv / sdvPrev) > SDV_REDUCTION_FACTOR) {
                    // Not getting much smaller
                    omit[bigIdx] = false;
                    stats.mean = meanPrev;
                    stats.sdv = sdvPrev;
                    done = true;
                } else if (debug) {
                    Messages.postDebug("Sensitivity",
                                       "Ignore outlier: data[" + bigIdx + "]="
                                       + data.get(bigIdx));
                }
                if (numberUsed(n, omit) <= 2) {
                    // Ran out of points
                    done = true;
                }
            }
        }
        //        fprintf(stderr,"Number of points used: %d\n", numberUsed(omit));
        stats.nPointsUsed = numberUsed(n, omit);
        stats.nPointsOmitted = n - stats.nPointsUsed;
        if (debug) {
            Messages.postDebug("Sensitivity",
                               stats.nPointsOmitted + " outliers ignored");
        }
        return stats;
    }

    /**
     * Get the index of the point in the given data set that lies farthest
     * from the given value.
     * The criterion is abs(data[i] - mean) is maximum.
     * Ignores points that have been marked in the "omit" list.
     * @param data The list of data values.
     * @param omit The list of points to omit.
     * @param mean The value to compare to in finding outliers.
     * @return The index of the outlier point.
     */
    private static int getOutlier(List<Double> data,
                                  boolean[] omit,
                                  double mean) {
        int n = data.size();
        int omitLength = omit.length;
        int idx = -1;      // Index of outlier
        double value = -1; // Deviation of outlier
        for (int i = 0; i < n; i++) {
            if (omit == null || omitLength <= i || !omit[i]) {
                double diff = Math.abs(data.get(i) - mean); 
                if (value < diff) {
                    value = diff;
                    idx = i;
                }
            }
        }
        return idx;
    }

    /**
     * Calculates the number of data points used, given the total available
     * points and a list of omitted points.
     * @param n The total of available points.
     * @param omit Array of flags indicating points to ignore.
     * @return The number of points used.
     */
    private static int numberUsed(int n, boolean[] omit) {
        int omitLength = Math.min(omit.length, n);
        int nOmitted = 0;
        if (omit != null) {
            for (int i = omitLength - 1; i >= 0; --i) {
                if (omit[i]) {
                    nOmitted++;
                }
            }
        }
        return n - nOmitted;
    }

    /**
     * Calculates the Standard Deviation of a data set about a given mean
     * value. A list of points to ignore in the calculation can also be
     * provided.
     * @param data The list of data points.
     * @param omit Array of flags indicating points to ignore.
     * @param mean The mean value used to calculate the standard deviation.
     * @param isMeanFromData True if mean is derived from the same data.
     * @return The sample standard deviation.
     */
    private static double getSdv(List<Double> data, boolean[] omit,
                                 double mean, boolean isMeanFromData) {
        int n = data.size();
        int omitLength = omit.length;
        int nUsed = 0;
        double sdv = 0;
        for (int i = 0; i < n; i++) {
            if (omit == null || omitLength <= i || !omit[i]) {
                double diff = data.get(i) - mean;
                sdv += diff * diff;
                nUsed++;
            }
        }
        int nfree = isMeanFromData ? nUsed - 1 : nUsed;
        if (nfree < 1) {
            return Double.NaN;
        } else {
            sdv = Math.sqrt(sdv / nfree);
            return sdv;
        }
    }

    /**
     * Get the mean value of a given data set, omitting specified elements.
     * @param data The list of values to average.
     * @param omit If the i'th value of this array is true, the i'th
     * data element is ignored.
     * @return The mean value.
     */
    private static double getMean(List<Double> data, boolean[] omit) {
        int n = data.size();
        int omitLength = omit.length;
        int nUsed = 0;
        double mean = 0;
        for (int i = 0; i < n; i++) {
            if (omit == null || omitLength <= i || !omit[i]) {
                mean += data.get(i);
                nUsed++;
            }
        }
        if (nUsed == 0) {
            return Double.NaN;
        } else {
            return mean / nUsed;
        }
    }

    /**
     * Container class to hold statistics about a data set.
     *
     */
    public static class Stats {
        /** The mean value of the data. */
        public double mean;

        /** The computed standard deviation of the individual data points. */
        public double sdv;

        /** The number of points used in computing the mean. */
        public int nPointsUsed;

        /** The number of points rejected in computing the mean. */
        public int nPointsOmitted;
    }

}
