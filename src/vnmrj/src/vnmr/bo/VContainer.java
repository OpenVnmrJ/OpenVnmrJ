/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.util.Vector;

import vnmr.ui.*;
import vnmr.util.*;


/**
 * A general purpose panel for holding VObjIF's
 */
public class VContainer extends VObj implements ExpListenerIF {

    public VContainer(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
    }

    /**
     * Causes any component that is interested in one of the specified
     * parameters to update itself.
     * Called when pnew is received from VnmrBG.
     * @param v The parameter names and values to update
     */
    public void updateValue(Vector v) {
        ParamListenerUtils.updateValue(this, v);
    }

    /**
     * Causes everything in the panel to update itself.
     */
    public void updateValue() {
        ParamListenerUtils.updateAllValue(this);
    }
}
