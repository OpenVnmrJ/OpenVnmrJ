/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import java.util.*;
import vnmr.util.*;

public interface  ParamListenerIF {
    public void updateValue(Vector params);
    public void updateAllValue();
}

