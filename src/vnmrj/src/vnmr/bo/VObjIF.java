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
import java.util.*;
import vnmr.util.*;

public interface  VObjIF {
    public void setAttribute(int t, String s);
    public String getAttribute(int t);
    public void setEditStatus(boolean s); // This component is being edited
    public void setEditMode(boolean s);	  // Containing panel is in edit mode
    public void addDefChoice(String s);
    public void addDefValue(String s);
    public void setDefLabel(String s);
    public void setDefColor(String s);
    public void setDefLoc(int x, int y);
    public void refresh();
    public void changeFont();
    public void updateValue();
    public void setValue(ParamIF p);
    public void setShowValue(ParamIF p);
    public void changeFocus(boolean s);
    public ButtonIF getVnmrIF();
    public void setVnmrIF(ButtonIF vif);
    public void destroy();
    public void setModalMode(boolean s);
    public void sendVnmrCmd();
    public void setSizeRatio(double w, double h);
    public Point getDefLoc();
}

