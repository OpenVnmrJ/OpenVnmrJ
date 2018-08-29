/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;


import static java.lang.Double.NaN;
import static vnmr.apt.ChannelInfo.Limits;


/**
 * This class holds the specifications for a tune channel to be loaded.
 */
public class ChannelSpec {
    /** The channel number. */
    private int m_chanNum;

    /** The frequency range to use with this channel (Hz). */
    private Limits m_freqRange;


    /**
     * Construct a ChannelSpec with the given channel.
     * @param ichan The channel number.
     */
    public ChannelSpec(int ichan) {
        this(ichan, NaN, NaN);
    }

    /**
     * Construct a ChannelSpec with the given channel number and
     * frequency range.
     * @param ichan The channel number.
     * @param min The minimum frequency (Hz).
     * @param max The maximum frequency (Hz).
     */
    public ChannelSpec(int ichan, double min, double max) {
        m_chanNum = ichan;
        m_freqRange = new Limits(min, max);
    }

    /**
     * @return The channel number.
     */
    public int getChanNum() {
        return m_chanNum;
    }

    /**
     * @return The frequency range (Hz).
     */
    public Limits getFreqRange_MHz() {
        return m_freqRange;
    }

    public String getName() {
        return "chan#" + getChanNum();
    }
}
