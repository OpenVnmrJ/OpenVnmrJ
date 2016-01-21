/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.event.*;
import java.util.*;

import vnmr.bo.*;

/**
 * This class stores a list of values recently sent to VnmrBG.  It
 * also saves the time the value was sent.  When "setValue" is called
 * as the result of a pnew from VnmrBG, you can check if that is just
 * the response of a recent command sent to VnmrBG.  The remembered
 * values are thrown out if they get too old.  The time limit on this
 * is dynamically adjusted according to how long recent commands have
 * taken.  Call setLastVal() before sending the command, which will
 * add it to the list.  Then call isValExpected() to see if a value
 * coming from VnmrBG needs to be acted on.  If it is expected, it
 * probably doesn't.
 * <p>
 * A second function is to provide a "throttle" to prevent commands
 * from being sent too often.  Call setThrottle(time) to set the
 * minimum time between commands.  Then call sendVnmrCmd(ButtonIF ...)
 * in this class whenever a command is to be sent to VnmrBG.  If
 * another command has been sent recently, no command will be sent
 * until the specified time after the previous command.  Then, only
 * the last command received will be sent.
 * <p>
 * A third, minor, function is to use getLastVal() to check if
 * a value you are about to send is the same as the last value sent.
 * If it is, you might not want to send it.
 */
public class VObjCommandFilter {

    private int m_multipleUpdates = 0;
    private int m_throttle_ms = 0;
    private long m_lastCmd_ms = 0;
    private ButtonIF m_vnmrIf = null;
    private VObjIF m_vobj = null;
    private String m_vnmrCmd = null;
    private javax.swing.Timer m_timer = null;

    private long m_shelfLife_ms;
    private long m_delay = m_shelfLife_ms;
    private long m_delayDate = 0;
    private String m_lastVal;
    private List m_expectedList = Collections.synchronizedList(new ArrayList());


    /**
     * Constructor specifying how long a sent value is considered
     * valid.  This will automatically be extended if there is a
     * record of how long recent commands have taken.  The multiple
     * reply parameter is set to false.
     * @param shelfLife_ms The aging time in ms.
     * @see #VObjCommandFilter(long, boolean).
     */
    public VObjCommandFilter(long shelfLife_ms) {
        m_shelfLife_ms = shelfLife_ms;
    }

    /**
     * Constructor specifying how long a sent value is considered
     * valid and whether to expect multiple update replies.  Note that
     * there are likely to be multiple replies to some updates and no
     * replies to others, unless setValue() is called directly from
     * the "pnew" message instead of going through the updateValue()
     * method.
     * @param shelfLife_ms The aging time in ms.
     * @param multipleReplies True if there may be multiple replies to
     * the same value change.
     */
    public VObjCommandFilter(long shelfLife_ms, boolean multipleReplies) {
        m_shelfLife_ms = shelfLife_ms;
        m_multipleUpdates = multipleReplies ? 1 : 0;
    }

    private void initTimer() {
        if (m_timer != null) {
            return;
        }
        // Set up throttle timer
        ActionListener throttleTimeout = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                sendVnmrCmd();
            }
        };
        m_timer = new javax.swing.Timer(100, throttleTimeout);
        m_timer.setRepeats(false);
    }

    /**
     * Send a command to VnmrBG, possibly delaying it first.
     * @param vnmrIf Who to send the message to.
     * @param vobj Who to say it was from.
     * @param vnmrCmd The message.
     */
    synchronized public void sendVnmrCmd(ButtonIF vnmrIf,
                                         VObjIF vobj, String vnmrCmd) {
        Messages.postDebug("VObjCommandFilter",
                           "VObjCommandFilter.sendVnmrCmd("
                           + vnmrCmd + "): throttle=" + m_throttle_ms);
        if (m_throttle_ms <= 0) {
            // Send it right away
            vnmrIf.sendVnmrCmd(vobj, vnmrCmd);
        } else {
            // Has it been long enough since last command?
            long now = new Date().getTime();
            long timeLeft = m_lastCmd_ms - now + m_throttle_ms;
            if (timeLeft <= 0) {
                // Send it right away
                vnmrIf.sendVnmrCmd(vobj, vnmrCmd);
                m_lastCmd_ms = now;
            } else {
                // Save it to send later
                m_vnmrIf = vnmrIf;
                m_vobj = vobj;
                m_vnmrCmd = vnmrCmd;
                if (!m_timer.isRunning()) {
                    m_timer.setInitialDelay((int)timeLeft);
                    m_timer.start();
                }
            }
        }
    }

    /**
     * Send the queued up command to VnmrBG right now.
     */
    synchronized public void sendVnmrCmd() {
        m_vnmrIf.sendVnmrCmd(m_vobj, m_vnmrCmd);
    }

    /**
     * Set a maximum rate at which commands can be sent.
     */
    public void setThrottle(int delay) {
        m_throttle_ms = delay;
        initTimer();
    }

    /**
     * Add a value string to be remembered.
     * @param val The value string.
     */
    public void setLastVal(String val) {
        m_lastVal = val;
        if (val != null) {
            m_expectedList.add(new Command(val));
            Messages.postDebug("VObjCommandFilter",
                               "VObjCommandFilter.setLastVal() :" + val
                               + ", values in list: " + m_expectedList.size());
        }
    }

    /**
     * Get the last value remembered, regardless of age.
     * @return The last value string remembered.
     */
    public String getLastVal() {
        return m_lastVal;
    }

    /**
     * See if a value is the same as one recently sent.  The list of
     * values sent is searched chronologically, oldest first.  expired
     * values are deleted from the list.  When a recent match is
     * found, that value, and all previous ones, are deleted from the
     * list.
     * @return True if it was sent recently, otherwise false.
     */
    public boolean isValExpected(String str) {
        synchronized(m_expectedList) {
            long now = new Date().getTime();
            int n = m_expectedList.size();
            long shelfLife = m_shelfLife_ms;
            if (now < m_delayDate + m_shelfLife_ms) {
                shelfLife = m_delay + m_shelfLife_ms;
                Messages.postDebug("VObjCommandFilter",
                                   "VObjCommandFilter.isValExpected()"
                                   + ": shelfLife=" + shelfLife
                                   + ", entries: " + m_expectedList.size());
            }
            for (int i = 0; i < n; i++) {
                Command old = (Command)m_expectedList.get(i);
                if (now > old.date + shelfLife) {
                    Messages.postDebug("VObjCommandFilter",
                                       "VObjCommandFilter.isValExpected()"
                                       + ": Remove old entry: " + old.cmd);
                    m_expectedList.remove(i);
                    --n;
                    --i;
                } else if (old.cmd.equals(str)) {
                    i -= m_multipleUpdates; // Decrement i if multiple updates
                    Messages.postDebug("VObjCommandFilter",
                                       "VObjCommandFilter.isValExpected()"
                                       + ": string matches: '" + str + "'"
                                       + ", remove " + (i + 1) + " entries"
                                       + ", remaining: "
                                       + (m_expectedList.size() - (i + 1)));
                    m_delay = now - old.date;
                    m_delayDate = now;
                    // Remove all previous entries and maybe this one
                    for ( ; i >= 0; --i) {
                        m_expectedList.remove(i);
                    }
                    return true;
                } else {
                    Messages.postDebug("VObjCommandFilter",
                                       "VObjCommandFilter.isValExpected() :'"
                                       + old.cmd + "' doesn't match '" + str
                                       + "'");
                }
            }
        }
        return false;
    }


    /**
     * A structure class for holding a value string ("command") and
     * a time.
     */
    public class Command {
        /** The time the value was saved in ms since 1970/0/0. */
        public long date;

        /** The value string. */
        public String cmd;

        /**
         * Create a Command with a given value string.
         * The date is added automatically.
         * @param cmd The value string.
         */
        public Command(String cmd) {
            this.cmd = cmd;
            this.date = new Date().getTime();
        }
    }
}
