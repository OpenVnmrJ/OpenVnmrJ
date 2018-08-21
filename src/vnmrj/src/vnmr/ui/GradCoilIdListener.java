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

public class GradCoilIdListener implements StatusListenerIF {

    ExpPanel expPanel;

    public GradCoilIdListener(ExpPanel expPanel) {

        this.expPanel = expPanel;

        // Register as a status listener.  updateStatus() will be called
        // for every status variable that comes back from the console.
        ExpPanel.addStatusListener(this);
    }

    public void updateStatus(String str) {
        if(str.indexOf("gradCoilId") != -1) {
            String newGradCoilId=str.substring("gradCoilId".length()+1).trim();
            if (newGradCoilId.equals("-")) return;
            Util.sendToVnmr("coilidchanged('"+newGradCoilId+"')");
        }
    }
}
