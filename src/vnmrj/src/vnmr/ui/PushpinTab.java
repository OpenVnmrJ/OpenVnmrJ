/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.beans.*;
import javax.swing.border.*;

import vnmr.util.*;

/**
 * tab button
 */
public class PushpinTab extends TabButton {
    public  boolean onTop = false;
    public  boolean bSet = false;
    public  int   posX = 0;
    public  int   posY = 0;
    public  float refY = 0;
    public  float refX = 0;
    private int   orient = SwingConstants.TOP;
    private String name = null;
    private PushpinIF  pinObj = null;
    private PushpinControler controler;
    /**
     * constructor
     */

    public PushpinTab() {
	this("");
    }

    public PushpinTab(String text) {
        super(text);
        addMouseListener(new MouseAdapter() {
            public void mouseEntered(MouseEvent evt) {
               processMouse(true, false);
            }

            public void mouseExited(MouseEvent ev) {
               processMouse(false, false);
              //  processExit(ev.getX(), ev.getY());
            }
            public void mouseClicked(MouseEvent evt) {
               //  int nClick = evt.getClickCount();
               processMouse(false, true);
            }
        });
    }


    public void setName(String n) {
        name = n;
    }

    public String getName() {
        return name;
    }

    public void setTabComp(JComponent comp) {
        if (comp != null && (comp instanceof PushpinIF))
           pinObj = (PushpinIF) comp;
        else
           pinObj = null;
    }

    public PushpinIF getTabComp() {
        return pinObj;
    }

    public void setOrientation(int n) {
        orient = n;
    }

    public int getOrientation() {
        return orient;
    }

    public void setControler(PushpinControler c) {
        controler = c;
    }

    private void processMouse(boolean bEnter, boolean bClick) {
        if (pinObj == null)
           return;
        if (bClick) {
           if (controler != null)
               controler.clickTab(this);
           else 
               pinObj.openFromHide();
           return;
        }
        if (bEnter) {
           if (controler != null)
               controler.enterTab(this);
           else
               pinObj.pinShow(true);   
           return;
        }
        if (controler != null)
           controler.exitTab(this);
    }

    public void processExit(int x, int y) {
        if (pinObj == null)
            return;
        if (!pinObj.isHide())
            return;
        boolean  bExit = false;
        Dimension dim = getSize();
        switch (orient) {
          case SwingConstants.LEFT:
                   if (x < 0)
                       bExit = true;
                   else if (y < 0 || y > dim.height)
                       bExit = true;
                   break;
          case SwingConstants.RIGHT:
                   if (x > dim.width)
                       bExit = true;
                   else if (y < 0 || y > dim.height)
                       bExit = true;
                   break;
          case SwingConstants.TOP:
                   if (x > dim.width || x < 0)
                       bExit = true;
                   else if (y < 0)
                       bExit = true;
                   break;
          case SwingConstants.BOTTOM:
                   if (x > dim.width || x < 0)
                       bExit = true;
                   else if (y > dim.height)
                       bExit = true;
                   break;
               
        }
        if (bExit) {
            pinObj.pinShow(false);
        }
    }
} // class PushpinTab
