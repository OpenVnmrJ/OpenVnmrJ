/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import vnmr.util.Messages;


/**
 * Specifies an event in the LC run.  Could be an automatically or
 * manually detected peak, or just some arbitrary time.  It's just a
 * time when we need to take some special action.
 */
public class LcEvent implements Comparable, Cloneable {
    /*
     * Implements Comparable so that events can be put in a SortedList,
     * and we can easily get the first event.  So the primary sorting
     * is on "time_ms".  To allow events to have the same times, make
     * the unique "id" the secondary sorting criterion.
     */

    /** Running ID number for events */
    static private long masterId = 0;

    /** Id number for this event */
    private long id;

    /**
     * Time event should happen, in ms of flow,
     * measured from the beginning of the run.
     */
    public long time_ms;
    
    /**
     * The transfer time appropriate for this type of run, in ms.
     */
    public long delay_ms;

    /**
     * Which collector loop is switched in.
     */
    public int loop;
    
    /**
     * Time event actually happened, in ms of flow,
     * measured from the beginning of the run.  Not defined
     * until after the event has happened.
     */
    public long actual_ms = 0;
    
    /** How long flow was stopped. */
    public long length_ms = 0;

    /**
     * Nearest index of event in data, starting at one (1) at the
     * first data point.
     */
    public long index;
    
    /** Channel number event was on; first channel is 1. */
    public int channel;

    /** What to do when the event happens */
    public String action;

    /** What triggered the event */
    public EventTrig trigger = EventTrig.UNKNOWN;

    /** What kind of marker we want */
    public MarkType markType = MarkType.PEAK;

    /**
     * Index into the NMR data for this peak.
     * <pre>
     *  0: no data
     *  Negative: peak number not assigned yet
     * </pre>
     */
    public int peakNumber = 0;

    /**
     * Indicates what triggered the event and how to label the trigger.
     */
    public enum EventTrig {
        UNKNOWN("Unknown"),
        CLICK_PLOT("Click on Plot"),
        PEAK_DETECT("Peak Detect"),
        USER_HOLD("User Hold"),
        METHOD_TABLE("Method"),
        USER_ACTION("User Action"),
        TIME_SLICE("Timer");

        /** A String describing the trigger for the event. */
        private final String m_label;

        /**
         * Describes the trigger for an event.
         * @param label A String describing the trigger.
         */
        EventTrig(String label) {
            m_label = label;
        }

        /**
         * Get a String describing the trigger for the event.
         */
        public String label() {
            return m_label;
        }

        /**
         * Get an EventTrig matching the given label.
         * @param label A String describing the trigger,
         * as given in the constructor.
         * @return A matching event, or UNKNOWN if the label is not found.
         */
        public static EventTrig getType(String label) {
            if (label != null) {
                for (EventTrig t : EventTrig.values()) {
                    if (label.equals(t.label())) {
                        return t;
                    }
                }
            }
            return UNKNOWN;
        }
    }

    /**
     * Enumerates the differnt marks that can indicate an event on the plot.
     */
    public enum MarkType {
        NONE,
        PEAK,
        SLICE,
        SLICEOFF,
        HOLD,
        ELUTION,
        COLLECTION,
        COMMAND,
        INJECT_ON,
        INJECT_OFF;
    }


    /**
     * Construct an event giving only the "action" and "markType".
     * Used to create generic events that can be put in a queue after
     * having the time filled in.
     * @param action The command to be executed.
     * @param markType The code for the type of mark to make.
     */
    public LcEvent(String action, MarkType markType) {
        this(0, 0, 0, 0, null, markType, action);
    }

    /**
     * Constructs an event with specified time, and channel.
     * Only one channel can be specified.
     * @param time Time in ms
     * @param channel Channel number (starting at 1)
     */
    public LcEvent(long time, long delay,
                   int channel, int loop, EventTrig trigger, String action) {
        this(time, delay, channel, loop, trigger, MarkType.PEAK, action);
    }

    public LcEvent(long time, long delay,
                   int channel, int loop,
                   EventTrig trigger, MarkType markType, String action) {
        Messages.postDebug("LcEvent",
                           "LcEvent(" + time + ", " + delay + ", "
                           + channel + ", " + loop + ", " + markType
                           + ", " + action + ")");
        this.id = ++masterId;
        this.time_ms = time;
        this.delay_ms = delay;
        this.channel = channel;
        this.loop = loop;
        this.trigger = trigger == null ? EventTrig.UNKNOWN : trigger;
        this.markType = markType == null ? MarkType.NONE : markType;
        this.action = action == null ? "" : action;
    }

    /**
     * Return a copy of this object.
     */
    public Object clone() {
        try {
            return super.clone();
        } catch (CloneNotSupportedException e) {
            // Should never happen
            Messages.writeStackTrace(e);
            return null;
        }
    }

    /**
     * Returns true if this event calls for stopping the flow.
     */
    public boolean stopsFlow() {
        return true;
    }

    public boolean equals(Object ob2) {
        /*System.out.println("LcEvent.equals():  id1=" + id
                           + ", id2=" + ((LcEvent)ob2).id);/*CMP*/
        return (id == ((LcEvent)ob2).id);
    }

    public int compareTo(Object ob2) {
        LcEvent le2 = (LcEvent)ob2;
        /*System.out.println("LcEvent.compareTo(): t1=" + time_ms
                           + ", t2=" + le2.time_ms
                           + ", id1=" + id + ", id2=" + le2.id);/*CMP*/
        if (time_ms > le2.time_ms) {
            return 1;
        } else if (time_ms < le2.time_ms) {
            return -1;
        } else if (id > le2.id) {
            return 1;
        } else if (id < le2.id) {
            return -1;
        } else {
            return 0;
        }
    }
}
