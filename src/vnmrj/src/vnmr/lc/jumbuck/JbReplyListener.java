/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc.jumbuck;

import java.io.*;

public interface JbReplyListener {

    public boolean receiveReply(int msgCode, JbInputStream in)
        throws IOException;

}
