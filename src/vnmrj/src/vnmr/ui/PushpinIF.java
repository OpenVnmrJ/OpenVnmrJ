/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import javax.swing.*;


public interface PushpinIF {

    public boolean isHide();
    public boolean isOpen();
    public boolean isClose();  /* this is invisible */
    public boolean isPopup();
    public void    pinClose(boolean doAct); /* make this invisible */
    public void    pinOpen(boolean doAct);  /* make this visible */
    public void    pinHide(boolean doAct);  /* make this in hide mode */
    public void    pinShow(boolean onOff);  /* set pin default mode */
    public void    pinPopup(boolean upDn);
    public void    openFromHide();
    public void    removeTab();
    public void    setTab(PushpinTab t);
    public void    setTitle(String s);
    public void    setName(String s);
    public void    setPin(boolean onOff);
    public void    showTab(boolean s);
    public void    showTitle(boolean s);
    public void    showPushPin(boolean s);
    public void    setPopup(boolean s, boolean originator);
    public void    setControler(PushpinControler c);
    public void    setContainer(JComponent c);
    public void    setSuperContainer(JComponent c);
    public void    setStatus(String s);
    public String  getTitle();
    public String  getName();
    public String  getStatus();
    public float   getRefX();
    public float   getRefY();
    public float   getRefH();
    public int     getDividerLoc();
    public int     getDividerOrientation();
    public void    setRefX(float f);
    public void    setRefY(float f);
    public void    setRefH(float f);
    public void    setDividerLoc(int v);
    public void    timerProc();
    public PushpinControler getControler();
    public JComponent getPinContainer();
    public JComponent getTab();
    public JComponent getPinObj();
    public void    setUpperComp(JComponent c);
    public void    setLowerComp(JComponent c);
    public void    setAvailable(boolean s);
    public void    setTabOnTop(boolean s);
    public boolean isTabOnTop();
    public boolean isAvailable();
    public JComponent getUpperComp();
    public JComponent getLowerComp();
    public Rectangle getDividerBounds();

} // class PushpinIF
