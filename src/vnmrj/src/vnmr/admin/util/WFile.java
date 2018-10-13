/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.util;

import java.io.*;
import java.util.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 * Company:
 * @author
 * @version 1.0
 */

public class WFile
{
    protected long m_lLastMod;
    protected Object m_objData;

    public WFile(long lastModified, Object objData)
    {
	m_lLastMod = lastModified;
	m_objData = objData;
    }

    public Object getContents()
    {
	return m_objData;
    }

    public void setContents(Object objData)
    {
	m_objData = objData;
    }

    public long getLastModified()
    {
	return m_lLastMod;
    }

    public void setLastModified(long lastMod)
    {
	m_lLastMod = lastMod;
    }

}
