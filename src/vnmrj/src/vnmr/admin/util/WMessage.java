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
 *
 *
 *
 */

public class WMessage
{

    protected boolean bOk = true;
    protected String strMsg = "";

    public WMessage(boolean bNoError, String msg)
    {
        bOk = bNoError;
        strMsg = msg;
    }

    public boolean isNoError()
    {
        return bOk;
    }

    public void setNoError(boolean bError)
    {
        bOk = bError;
    }

    public String getMsg()
    {
        return strMsg;
    }

    public void setMsg(String msg)
    {
        strMsg = msg;
    }
}
