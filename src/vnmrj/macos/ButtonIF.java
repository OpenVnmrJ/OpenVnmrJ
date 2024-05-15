/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;
import java.io.*;
import java.awt.event.*;
import vnmr.bo.*;


/**
 * An ButtonIF provides the interface for button callback.
 *
 */
public interface  ButtonIF {
    public void buttonActiveCall(int num);
/*
    public void tearOffOpen(boolean b);
 */
    public void childMouseProc(MouseEvent ev, boolean release, boolean drag);
    public void sendToVnmr(String cmd);
    public void sendVnmrCmd(VObjIF obj, String cmd);
    public void sendVnmrCmd(VObjIF obj, String cmd, int nKey);
    public void setBusy(boolean b);
    public ParamIF syncQueryParam(String s);
    public ParamIF syncQueryPStatus(String s);
    public ParamIF syncQueryMinMax(String s);
    public ParamIF syncQueryExpr(String s);
    public ParamIF syncQueryVnmr(int type, String s);
    public void asyncQueryParam(VObjIF o, String s);
    public void asyncQueryParam(VObjIF o, String s, String s2);
    public void asyncQueryShow(VObjIF o, String s);
    public void asyncQueryParamNamed(String name, VObjIF o, String s);
    public void asyncQueryParamNamed(String name, VObjIF o, String s, String u);
    public void asyncQueryShowNamed(String name, VObjIF o, String s);
    public void asyncQueryMinMax(VObjIF o, String s);
    public void requestActive();
    public void setInputFocus();
    public void asyncQueryARRAY(VObjIF o, int cmdType, String s);
    public void queryPanelInfo(String name);
    public void queryPanelInfo();
    public void setBusyTimer(boolean b);
    public VContainer getLcPanel();
    public void setPanelGeometry(VContainer comp, int size, char pos);
    public void processMousePressed(MouseEvent e);
    public void processMouseReleased(MouseEvent e);
} // ButtonIF
