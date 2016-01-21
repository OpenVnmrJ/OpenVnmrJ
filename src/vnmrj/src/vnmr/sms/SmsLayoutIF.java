/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

public interface  SmsLayoutIF {
    public void setSampleList(SmsSample s[]);
    public void setStartSample(int s);
    public void setLastSample(int s);
    public int  zoomDir();
}
