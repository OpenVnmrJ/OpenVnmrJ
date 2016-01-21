/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

public interface LcDef
{
    /**
     * This is the max number of traces that can be displayed.
     * Also the number of data columns in the chromatogram file.
     */
    static final int MAX_TRACES = 5;
    static final int TEMP_DATASET = MAX_TRACES * 2;

    static final int MAX_DETECTORS = 3;
    static final double HOLD_Y_POSITION = Double.POSITIVE_INFINITY;
    static final double SLICE_Y_POSITION = Double.POSITIVE_INFINITY;
    static final double ELUTION_Y_POSITION = 0;
    static final double COLLECTION_Y_POSITION = 0;
    static final double DEFAULT_Y_POSITION = Double.NEGATIVE_INFINITY;
}
