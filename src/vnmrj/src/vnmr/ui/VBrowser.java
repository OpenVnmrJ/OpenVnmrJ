/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.*;
import java.util.*;

import vnmr.util.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class VBrowser
{

    public static final String URLPREFIX = "file://";
    public static final String HTTPPREFIX = "http://";
    public static final String HTTPSPREFIX = "https://";
    public static final String WWWPREFIX = "www.";

    public VBrowser()
    {

    }

	public static void displayURL(String url) {
		int nSize = URLPREFIX.length();
		if (url != null && url.startsWith(URLPREFIX))
			url = url.substring(nSize);

		if (url != null && !url.startsWith(HTTPPREFIX)
                                && !url.startsWith(HTTPSPREFIX)
				&& !url.startsWith(WWWPREFIX)) {
			url = FileUtil.openPath(url);

			if (!UtilB.iswindows() && url != null && url.startsWith("/"))
				url = new StringBuffer().append(URLPREFIX).append(url)
						.toString();
		}

		String cmd[] = {
				WGlobal.SHTOOLCMD,
				WGlobal.SHTOOLOPTION,
				new StringBuffer().append(FileUtil.SYS_VNMR)
						.append("/bin/vjhelp").append(" '").append(url)
						.append("'").toString() };

		if (UtilB.iswindows()) {
			try {
				Runtime.getRuntime().exec("cmd /c start " + url);
			} catch (Exception e) {
			}
		} else {
			try {
				WUtil.runScriptInThread(cmd, true);
			} catch (Exception e) {
				// e.printStackTrace();
				Messages.writeStackTrace(e);
			}
		}
	}

}
