/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.io.*;

/**
 * A lock status indicates locked (true) or unlocked (false).
 * LockStatus objects should be treated as immutable! For details,
 * see the instance variable 'lock'.
 *
 * @author Mark Cao
 */
public class LockStatus  implements  Serializable {
    // ==== final variables
    /** locked constant */
    public static final LockStatus locked = new LockStatus(true);
    /** unlocked constant */
    public static final LockStatus unlocked = new LockStatus(false);

    // ==== instance variables
    /** Lock indicator. Although this variable is made public, it should
     * never be assigned to, except by the constructor! The reason is that
     * multiple LockStatus references may share the same LockStatus
     * object, and hence changing the status of 'lock' from one reference
     * will affect all other references. This variable has been made
     * public only for performance reasons. */
    public boolean lock;

    /**
     * constructor
     * @param match match indicator
     */
    public LockStatus(boolean lock) {
	this.lock = lock;
    } // LockStatus()

    /**
     * is lock on?
     * @return boolean
     */
    public boolean isLocked() {
	return lock;
    } // isLocked()

    /**
     * Get a lock that is the inverse (e.g., return unlocked if given
     * locked).
     * @return inverted lock
     */
    public LockStatus inverse() {
	return lock ? unlocked : locked;
    } // inverse()

} // class LockStatus
