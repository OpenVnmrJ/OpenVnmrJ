/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * TimeTest.java
 *
 * Created on April 12, 2005, 3:20 AM
 *
 *
 * @author frits
 */

package accounting;

import java.text.*;
import java.util.*;

public class TimeTest {
    SimpleDateFormat sdf;
    public static void main(String[] args) {
        TimeTest tt = new TimeTest();
        
    }
    
    /** Creates a new instance of TimeTest */
    public TimeTest() {
        Date d = new Date(1113599334L*1000);
        System.out.println("Date is :"+d+":");
        sdf = new SimpleDateFormat("EEE MMM d hh:mm:ss yyyy");
        System.out.println("TimeTest(): "+
                timeDiff("Wed Mar 1 13:21:01 2006","Thu Mar 30 14:39:22 2006") );
        System.out.println("TimeTest(): "+
                timeDiff("Wed Mar 30 13:21:01 2006","Thu Mar 30 14:39:22 2006") );
        System.out.println("TimeTest(): "+
                timeDiff("Wed Mar 30 14:21:01 2006","Thu Mar 30 14:39:22 2006") );
        System.out.println("TimeTest(): "+
                timeDiff("Wed Mar 30 14:39:22 2006","Thu Mar 30 14:39:22 2006") );
    }
    
    public String timeDiff(String date1, String date2) {
        ParsePosition pp = new ParsePosition(0);
        Date from = sdf.parse(date1,pp);
        // System.out.println("TimeDiff: from ="+from);
        GregorianCalendar cFrom = new GregorianCalendar();
        cFrom.setTime(from);
        pp.setIndex(0);
        Date to = sdf.parse(date2,pp);
        // System.out.println("TimeDiff: to ="+to);
        GregorianCalendar cTo = new GregorianCalendar();
        cTo.setTime(to);
        long mills = cTo.getTimeInMillis() - cFrom.getTimeInMillis();
        System.out.println("TimeDiff = "+mills);
        int days = (int)(mills/1000/60/60/24);
        int hr = (int)(mills/1000/60/60);
        int min = (int)(mills/1000/60)%60;
        int sec = (int)(mills/1000)%60;
        StringBuilder sb = new StringBuilder("(");
        if (days>0)
            sb.append(days).append("+");
        if (hr < 10)
            sb.append("0");
        sb.append(hr).append(":");
        if (min < 10)
            sb.append("0");
        sb.append(min).append(":");
        if (sec < 10)
            sb.append("0");
        sb.append(sec).append(")");
        return( sb.toString() );
    }
    
}
