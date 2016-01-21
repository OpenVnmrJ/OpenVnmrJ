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
import java.awt.event.*;

import javax.swing.*;
import javax.swing.border.*;

import java.util.*;
import java.io.*;
import java.beans.*;

import vnmr.util.*;
import vnmr.bo.*;
import vnmr.templates.*;
import vnmr.admin.ui.*;

/**
 * The status bar.
 */
public class ExpStatusBar extends ParamPanel implements StatusListenerIF,
        ExpListenerIF, PropertyChangeListener {

    public ExpStatusBar(SessionShare sshare, ExpPanel exp) {
        this(sshare, exp, "INTERFACE/HardwareBar.xml");
    }

    /**
     * constructor
     */
    public ExpStatusBar(SessionShare sshare, ExpPanel exp, String strFile) {

        setBorder(BorderFactory.createEtchedBorder());

        setBackground(Util.getBgColor());
        DisplayOptions.addChangeListener(this);

        TemplateLayout paramsLayout = new TemplateLayout();
        setLayout(paramsLayout);
        try {
            String path = FileUtil.openPath(strFile);
            LayoutBuilder.build(this, exp, path);
        } catch (Exception e) {
            Messages.writeStackTrace(e, "error building HardwareBar.xml");
            return;
        }

        if (Util.getAppIF() instanceof VAdminIF)
            WInfoStat.addStatusListener(this);
        else
            ExpPanel.addStatusListener(this); // register as a Infostat listener

        setPanelFile("HardwareBar.xml");
        setPanelName("HardwareBar");
        ParamInfo.addEditListener(this);

    } // ExpStatusBar()

    public Object[][] getAttributes() {
        return null;
    }

    public void setEditMode(boolean s) {
        boolean b=editMode;
        if(!s)
            super.setEditMode(s);
        else{
            int edits=ParamInfo.getEditItems();
            if((edits & ParamInfo.STATUSBAR)>0)
                super.setEditMode(true);
            else
                super.setEditMode(false);               
        } 
        if(b!=editMode)
            repaint();
    }   
    public void propertyChange(PropertyChangeEvent evt) {
        setBackground(Util.getBgColor());
    }

    // StatusListenerIF interface
    public void updateStatus(String msg) {
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof StatusListenerIF)
                ((StatusListenerIF) comp).updateStatus(msg);
        }
        invalidate();
        repaint();
    }

    // ExpListenerIF interface
    /** called from ExpPanel (pnew) */
    public void updateValue(Vector v) {
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof ExpListenerIF)
                ((ExpListenerIF) comp).updateValue(v);
        }
    }

    /** called from ExperimentIF for first time initialization */
    public void updateValue() {
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof ExpListenerIF)
                ((ExpListenerIF) comp).updateValue();
        }
    }

} // class ExpStatusBar

