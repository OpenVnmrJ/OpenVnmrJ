/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.util.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 *
 *
 *
 */

public class WPanelManager
{
    HashMap m_cache = null;

    public WPanelManager()
    {
        m_cache = new HashMap();
    }

    public Object getPanel(String strPath)
    {
        return m_cache.get(strPath);
    }

    public void setPanel(String strPath, Object objDisplay)
    {
        if (strPath != null && objDisplay != null)
            m_cache.put(strPath, objDisplay);
    }

}
