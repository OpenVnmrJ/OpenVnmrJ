/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

import java.awt.Color;

public interface SmsDef
{
    static final int SMS = 1;
    static final int CAROSOL = 2;
    static final int VAST = 3; // Gilson
    static final int SMS100 = 4;
    static final int GRK49 = 5;  // traymax = 49
    static final int GRK97 = 6;  // traymax = 97
    static final int GRK12 = 7;  // traymax = 12
    
    static final int BYROW = 1;
    static final int BYCOL = 2;
    static final int T_FRONT = 1;
    static final int B_FRONT = 2;
    static final int T_BACK = 3;
    static final int B_BACK = 4;
    static final int L_FRONT = 1;   // left and front
    static final int R_FRONT = 2;   // right and front
    static final int L_BACK = 3;    // left and back
    static final int R_BACK = 4;    // right and back
    static final int TOP_DOWN = 1;  // from back
    static final int BOT_UP = 2;    //  from front 

    static final int OPEN = 0;
    static final int DONE = 1;
    static final int ERROR = 2;
    static final int QUEUED = 3;
    static final int NIGHTQ = 4;
    static final int ACTIVE = 5;
    static final int SELECTED = 6;

    static final int STATUS_NUM = 7;

/********
    static final int XOPEN = 0;
    static final int OPEN = 1;
    static final int XDONE = 2;
    static final int DONE = 3;
    static final int XERR = 4;
    static final int ERR = 5;
    static final int XQUEUED = 6;
    static final int QUEUED = 7;
    static final int XNIGHTQ = 8;
    static final int NIGHTQ = 9;
    static final int XACTIVE = 10;
    static final int ACTIVE = 11;
    static final int OTHER = 1;
*********/

    static final int RACK = 1;
    static final int ZONE = 2;

    static final int DONEQ = 1;
    static final int ENTERQ = 2;
    static final int PSGQ = 3;

    static final int NUMQ = 3;
    static final Color nq = new Color(160, 32, 240); /* purple */

    public static String[] STATUS_STR = {"empty", "complete", "error", 
        "queued", "nightqueue", "active", "selected" };

    public static Color[] STATUS_COLOR = {
        Color.gray, Color.green, Color.red,
        Color.yellow, nq, Color.cyan, Color.white };

    public static Color[] sampleColor =
           { Color.gray, Color.gray, Color.green.darker(),
             Color.green, Color.red.darker(), Color.red,
             Color.yellow.darker(), Color.yellow, nq.darker(),
             nq, Color.cyan.darker(), Color.cyan, Color.white };

    public static Color[] numColor = 
           { Color.white, Color.white, Color.white,
             Color.black, Color.white, Color.white,
             Color.white, Color.black, Color.white,
             Color.white, Color.white, Color.black, Color.white, Color.white };
/*
    public static Color[] sampleColor =
           { Color.gray, Color.green, Color.red, Color.blue, Color.yellow,
             Color.gray, Color.green.darker(), Color.red.darker(),
             Color.blue.darker(), Color.yellow.darker() }; 

    public static Color[] numColor = 
           { Color.white, Color.black, Color.black, Color.black, Color.black,
             Color.white, Color.white, Color.white, Color.white, Color.black };
*/

    // public static Color[] hColor = {Color.red, Color.yellow, Color.green };
    public static Color[] hColor = {new Color(0, 133, 213),
            new Color(255, 255, 0), new Color(8, 187, 249) };
}
