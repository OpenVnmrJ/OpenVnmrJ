/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

import java.awt.*;
import java.util.*;

public abstract class  GenPlateIF {
    public long timeOfUpdated = 0;
    public long timeOfLastModified = 0;
    public String sampleDataDir = null;
    public void setZone(int s) {};
    public boolean setSelect(int x, int y, int type, int clicks) {
        return false; }
    public void clearSelect() {}
    public boolean hilitSelect() { return false; }
    public boolean setScale(int n) { return true; }
    public void paintSample(Graphics g) {}
    public void paintSample(Graphics g, boolean paintSelected) {}
    public LayoutManager getLayoutMgr() { return null; }
    public SmsSample[] getSampleList() { return null; }
    public Vector<SmsSample> getSelectList() { return null; }
    public void setRackId(int n) {}
    public int getRackId() { return 1; }
    public int getSampleNum() { return 0; }
    public void clearAllSample() {}
    public void invalidateAllSample() {}
    public void revalidateAllSample() {}
    public SmsSample getSample(int zone, int id) { return null; }
    public SmsSample locateSample(int x, int y) { return null; }
    public void setSelectable(boolean b) {}
    public boolean setSelection(Vector<String> v) { return false; }
    public boolean setSelection(boolean b) { return false; }

    public long[] dateOfQfiles;
}
