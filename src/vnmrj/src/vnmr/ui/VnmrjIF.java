/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;


import vnmr.util.*;
import vnmr.bo.*;


/**
 * An VnmrjIF object stores the "Vnmrj" object, and is the interface
 *  for top-level user-interface components.
 * 
 */
public interface VnmrjIF {

    public Rectangle getCommandLineLoc();
    public VjToolBar getToolBar();
    public CommandInput getCmdLine();
    public boolean isKeyBinded(String strName);
    public String  getKeyBinded(String strName);
    public String  getKeyBindedForMenu(String strName);
    public boolean isCmdLineOpen();
    public void    sendCmdLineOpen();
    public void    setCmdLine(boolean b);
    public void    raiseComp(Component c, boolean top);
    public void    raiseToolPanel(boolean top);
    public void    openComp(String name, String cmd);
    public void    showJMolPanel(boolean b);
    public void    setSysToolBar(JComponent comp);
    public void    setSysLeftToolBar(JComponent comp);
    public void    createGraphicsToolBar();
    public void    frameEventProcess(WindowEvent e);
    public void    addFrameEventListener(Component o);
    public void    removeFrameEventListener(Component o);
    public void    addToolbarListener(Component o);
    public void    removeToolbarListener(Component o);
    public void    setGraphToolMoveStatus(boolean b);
    public void    graphToolMoveTo(int x, int y);
    public int     getGraphToolDockPosition();
    public boolean checkObject(String objName);
    public boolean checkObjectExist(String objName);
    public boolean checkObject(String objName, VObjIF obj);
    public boolean checkObjectExist(String objName, VObjIF obj);
    public boolean checkObject(String objName, int id);
    public boolean checkObjectExist(String objName, int id);
    public void    canvasSizeChanged(int id);
    public RightsList getRightsList();
    public void    moveComp(String name, int x, int y);
    public HelpOverlay  getHelpOverlay();
    public void    setDividerMoving(boolean b, PushpinIF obj);
    public void    setDividerColor(Color c);
    public void    setVerticalDividerLoc(int location);
    public void    setHorizontalDividerLoc(int location);

} // class VnmrjIF
