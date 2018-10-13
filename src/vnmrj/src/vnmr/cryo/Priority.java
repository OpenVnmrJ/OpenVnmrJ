/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.cryo;

public enum Priority {
    /** Send to high-priority queue */
    PRIORITY1(1),
    /** Send to normal-priority queue */
    PRIORITY2(2),
    /** Send to normal-priority queue only if not already queued up */
    PRIORITY3(3);

    private int m_priority;

    Priority(int priority) {
        m_priority = priority;
    }

    public int getPriority() {
        return m_priority;
    }
}
