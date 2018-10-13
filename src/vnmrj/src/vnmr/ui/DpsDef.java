/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

public interface DpsDef
{
    static final int ACQUIRE = 1;
    static final int GRAD = 19;
    static final int SHGRAD = 76;
    static final int PEGRAD = 38;
    static final int PULSE = 48;
    static final int SHPULSE = 77;
    static final int SPINLK = 86;
    static final int XGATE = 93;
    static final int GRPPULSE = 146;
    static final int DPESHGR = 161;
    static final int DPESHGR2 = 162;
    static final int JSHPULSE = 271;
    static final int JSHGRAD = 272;
    static final int JGRPPULSE = 273;

    static final int LOOP = 301;
    static final int ENDLOOP = 302;
    static final int RDBYTE = 303;
    static final int WRBYTE = 304;
}

