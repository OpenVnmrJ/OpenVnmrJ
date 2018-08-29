/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import vnmr.util.*;

/**
 * An abstract class for making classes that need some of the VObjIF
 * functionality.  E.g., it is used in vnmr.bo.VUpDownButton to create
 * an inner class to provide an alternate object to receive setValue()
 * calls.
 */

public abstract class VObjAdapter implements VObjIF {
    public void setAttribute(int t, String s){}
    public String getAttribute(int t){return null;}
    public void setEditStatus(boolean s){}
    public void setEditMode(boolean s){}
    public void addDefChoice(String s){}
    public void addDefValue(String s){}
    public void setDefLabel(String s){}
    public void setDefColor(String s){}
    public void setDefLoc(int x, int y){}
    public void refresh(){}
    public void changeFont(){}
    public void updateValue(){}
    public void setShowValue(ParamIF p){}
    public void changeFocus(boolean s){}
    public ButtonIF getVnmrIF(){return null;}
    public void setVnmrIF(ButtonIF vif){}
    public void setValue(ParamIF pf){}
    public void destroy() {}
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}
    public Point getDefLoc() { return new Point(0, 0); }
    public void setSizeRatio(double x, double y) {}
}
