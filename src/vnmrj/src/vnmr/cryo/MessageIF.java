/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.cryo;

import java.awt.Window;

import vnmr.ui.StatusManager;

public interface MessageIF {
    public void postDebug(String msg);
    public void postDebug(String category, String msg);
    public void postError(String msg);
    public void postInfo(String msg);
    public void postWarning(String msg);
    public void processStatusData(String msg);
    public void writeAdvanced(String msg);
    public void writeStackTrace(Exception e);
    // TODO: public void popupError(String msg);
    public void setStopEnabled(boolean b);
    public void setStartEnabled(boolean b);
    public void setDetachEnabled(boolean b);
    public void setVacPurgeEnabled(boolean b);
    public void setPumpProbeEnabled(boolean b);
    public void setThermCycleEnabled(boolean b);
    public void setCryoSendEnabled(boolean b);
    public void setThermSendEnabled(boolean b);
    public void stopNMR();

    public void setDateFormat(String dateFormat);
    public String getDateFormat();

    public Window getFrame();
    public CryoSocketControl getCryobay();
    public void setCryobay(CryoSocketControl cryoSocketControl);
    public void setStatusManager(StatusManager statusManager);
    public void setUploadCount(int i);
}
