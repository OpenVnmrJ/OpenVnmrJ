/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc.jumbuck;


public interface JbReplyProcessor {

    /**
     * Called when spectrum has been read into the detector parameter
     * table and needs to be processed.
     */
    public void processReply(int msgCode, int level);

    /**
     * This is called when data is received, for synchronization.  The
     * data has not yet been read.  This may be followed by zero, one,
     * or more calls to processReply() when
     */
    public void dataReceived();

}
