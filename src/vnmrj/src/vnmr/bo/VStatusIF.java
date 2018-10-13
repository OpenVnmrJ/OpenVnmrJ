/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.*;

public interface VStatusIF
{
    public final static int UNSET = 0;
    public final static int NOTPRESENT = 1;
    public final static int OFF = 2;
    public final static int REGULATED = 3;
    public final static int NOTREG = 4;

    public final static int INACTIVE = 0;
    public final static int READY = 1;
    public final static int ACTIVE = 2;
    public final static int REGULATING = 3;
    public final static int INTERACTIVE = 4;
    public final static int ACQUIRING = 5;
    
    public final static String[] states ={"NotPresent","Off","Regulated","NotReg"};
    public final static String[] status_key = {"lock",  "vt",     "spin", "rfmon"};
    public final static String[] status_val = {"lklvl", "vtval",  "spinval", "rfmonval"};
    public final static String[] status_set = {"",      "vtset",  "spinset", ""};
    public final static String[] status_color =  {"fg", "bg"};
}
 
