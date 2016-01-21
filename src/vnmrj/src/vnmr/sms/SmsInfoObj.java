/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.sms;

import java.awt.*;
import javax.swing.*;

import vnmr.util.*;
import vnmr.ui.*;
import vnmr.bo.*;

public class SmsInfoObj extends JComponent implements VObjIF, VObjDef
{
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String setVal = null;
    public int retType = 0;
    private String statusParam = null;
    protected ButtonIF vnmrIf;


    public SmsInfoObj(String str, int n) {
         this.setVal = str;
         this.retType = n;
    }


    public void setDefLabel(String s) {
    }

    public void setDefColor(String c) {
    }

    public void setEditStatus(boolean s) {
    }

    public void setEditMode(boolean s) {
    }


    public void changeFont() {
    }

    public void changeFocus(boolean s) {
    }

    public String getAttribute(int attr) {
         return null;
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
        case VALUE:
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        case STATPAR:
            statusParam = c;
            updateStatus(ExpPanel.getStatusValue(statusParam));
            break;
        case STATSHOW:
            updateStatus(ExpPanel.getStatusValue(statusParam));
            break;
        }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void updateStatus(String msg)
    {
    }

    public void setValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
              SmsUtil.setVnmrInfo(retType, pf.value);
        }
    }

    public void setShowValue(ParamIF pf) {
    }

    public void updateValue() {
        if (setVal == null)
            return;
        if (vnmrIf == null)
            vnmrIf = Util.getActiveView();
        if (vnmrIf != null)
            vnmrIf.asyncQueryParam(this, setVal);
    }


    public void refresh() { }
    public void destroy() { }

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}


    public void setModalMode(boolean s) {
    }

    public void sendVnmrCmd() {
        if (vnmrCmd == null || vnmrIf == null)
            return;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    public void setSizeRatio(double x, double y) {
    }


    public void setDefLoc(int x, int y) {
    }

    public Point getDefLoc() {
        return null;
    }

}



