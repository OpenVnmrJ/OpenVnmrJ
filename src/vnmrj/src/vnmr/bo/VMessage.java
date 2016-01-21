/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.text.DateFormat;
import java.util.*;

import static java.text.DateFormat.*;


public class VMessage {
    public String msg;
    public int type;
    public int cnt;
    public Date date;

    /**
     * Used to format the date.
     */
    private static final DateFormat dateFmt
        = DateFormat.getDateTimeInstance(MEDIUM, MEDIUM);

    /**
     * If true, the message count is included in the toString() output.
     */
    private static boolean sm_showCount = false;

    /**
     * If true, the date is included in the toString() output.
     */
    private static boolean sm_showDate = false;
 
     public VMessage(String s, int t){
        msg=s;
        type=t;
        cnt=0;
        date=new Date();
    }
   
    public VMessage(String s, int t, int c){
        msg=s;
        type=t;
        cnt=c;
        date=new Date();
    }

    public static void setShowCount(boolean b) {
        sm_showCount = b;
    }

    public static void setShowDate(boolean b) {
        sm_showDate = b;
    }

    public String toString() {
        StringBuffer sb = new StringBuffer();
        if (sm_showCount) {
            sb.append(cnt).append("\t");
        }
        if (sm_showDate) {
            sb.append(dateFmt.format(date)).append("\t");
        }
        sb.append(msg);
        return sb.toString();
    }
    
} // class VMessage
