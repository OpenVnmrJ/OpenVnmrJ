/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.cryo;

public class CryoUtils {

    private static long m_timeOffset = 0;

    static public String canonicalizeCommand(String cmd) {
        //m_cryoMsg.postDebug("editcryocmd",
        //                    "canonicalizeCommand: \"" + cmd + "\"");
         if (cmd != null) {
             cmd = cmd.trim();
             if (!cmd.startsWith("<")) {
                 cmd = "<" + cmd;
             }
             if (cmd.charAt(cmd.length() - 1) != '>') {
                 cmd = cmd + ">";
             }
             cmd = cmd.toUpperCase();
         }
         //m_cryoMsg.postDebug("editcryocmd", "  ---> \"" + cmd + "\"");
         return cmd;
        
    }

    /**
     * Correct a possibly overflowed time stamp to make it represent a time
     * within 2^31 seconds of the current time.
     * @param t_sec The 32-bit time stamp; seconds since 1970.
     * @param now_ms The "current time" in ms since 1970.
     * @return The corrected time stamp, maybe more than 32 bits.
     */
    static public long fixDateOverflow(long t_sec, long now_ms) {
        long maxTime_sec = now_ms / 1000 + (1L << 31);
        t_sec += (1L << 32) * ((maxTime_sec - t_sec) / (1L << 32));
        return t_sec;
    }

    /**
     * Truncate the given time stamp so that it will not overflow a
     * signed 32 bit integer.
     * @param t_sec The time stamp in seconds.
     * @return The truncated time stamp.
     */
    static public long truncateTimeStamp(long t_sec) {
        t_sec &= 0xffffffffL; // Force wrap at 32 bits
        if (t_sec >   0x7fffffffL) {
            t_sec -= 0x100000000L; // Don't overflow a signed int32
        }
        return t_sec;
    }

    /**
     * Get the current time in ms. Normally equivalent to
     * System.currentTimeMillis(), but allows for the possibility of
     * introducing a time offset for testing purposes.
     * @return The current time in ms since 1970.
     */
    static public long timeMs() {
        return m_timeOffset  + System.currentTimeMillis();
    }

    /**
     * Set the time offset to be used by the timeMs method.
     * The offset will be the given time minus the current host time.
     * @param time The desired current time in ms since 1970.
     */
    static public void setTimeMs(long time) {
        m_timeOffset = time - System.currentTimeMillis();
    }
}
