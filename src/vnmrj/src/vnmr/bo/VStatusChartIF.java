/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.util.*;
import vnmr.util.*;

public interface  VStatusChartIF {
    public void validate();
    public String getAttribute(int attr);
    public void setAttribute(int attr, String c);
    public void updateValue(Vector params);
    public void updateValue();
    public void setValue(String c);
    public void updateStatus(String msg);
    public void destroy();
    public void setSize(Dimension win);
}

