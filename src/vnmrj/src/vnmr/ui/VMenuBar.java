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
import java.util.Vector;
import javax.swing.*;
import java.beans.*;
import vnmr.util.*;
import vnmr.templates.*;
import vnmr.bo.*;

public class VMenuBar extends JMenuBar implements ExpListenerIF, PropertyChangeListener {
    protected SessionShare sshare;
    protected ButtonIF vnmrIf;
    protected int id;

    public VMenuBar(SessionShare sh, ButtonIF vif, int num)
    {
        this(sh, vif, num, "MainMenu.xml");
    } // VMenuBar()


    public VMenuBar(SessionShare sh, ButtonIF vif, int num, String strFilePath)
    {
        this.sshare = sh;
        this.vnmrIf = vif;
        this.id = num;
        setOpaque(true);
        // Util.setBgColor(this);
        setBackground(Util.getMenuBarBg());
        setBorder( BorderFactory.createEmptyBorder() );
        /*String fpath = "MainMenu.xml";
        fpath = FileUtil.openPath("INTERFACE/"+ fpath);*/
        strFilePath = FileUtil.openPath("INTERFACE/"+ strFilePath);
        if(strFilePath != null) {
            try {
                LayoutBuilder.build(this,vnmrIf,strFilePath);
            }  catch(Exception e) {
                Messages.writeStackTrace(e);
            }

        }
        DisplayOptions.addChangeListener(this);
        setBg();
        cleanStart();
    } // VMenuBar()

    public JComponent getMenuBar() {
        return this;
    }

    public void setBg() {
        int k = getMenuCount();
        Color bg = Util.getMenuBarBg();
        for (int i = 0; i < k; i++) {
            JMenu m = getMenu(i);
            if (m != null && (m instanceof VMenuitemIF)) {
                VMenuitemIF vf = (VMenuitemIF) m;
                vf.mainMenuBarItem(true);
                vf.changeLook();
            }
        }

        // Util.setBgColor(this);
        setBackground(bg);
    }

    public void propertyChange(PropertyChangeEvent evt){
        setBg();
    }

    public void  destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void updateValue(boolean bAll) {
        int k = getMenuCount();
        for (int i = 0; i < k; i++) {
            JMenu m = getMenu(i);
            if (m != null) {
               if (m instanceof VSubFileMenu) {
                  VSubFileMenu s = (VSubFileMenu) m;
                  String l = s.getAttribute(VObjDef.LABEL);
                  if (l != null && l.equalsIgnoreCase("view")) {
                       s.updateValue();
                  }
                  else {
                       if (bAll)
                           s.updateValue();
                       else
                           s.updateShow();
                  }
               }
               else {
                   if (m instanceof ExpListenerIF) {
                      ExpListenerIF e = (ExpListenerIF) m;
                      e.updateValue();
                   }
                   else if (m instanceof VObjIF) {
                      VObjIF obj = (VObjIF) m;
                      obj.updateValue();
                   }
               }
            }
        }
    }

    public void updateValue() {
        updateValue(true);
    }

    public void updateValue (Vector params) {
        int k = getMenuCount();
        for (int i = 0; i < k; i++) {
            JMenu m = getMenu(i);
            if (m != null) {
                if (m instanceof ExpListenerIF) {
                   ExpListenerIF e = (ExpListenerIF) m;
                   e.updateValue(params);
                }
            }
        }
    }

    // set first File item visible
    private void cleanStart() {
        int k = getMenuCount();
        for (int i = 1; i < k; i++) {
            JMenu m = getMenu(i);
            if (m != null) {
               if (m instanceof VObjIF) {
                  VObjIF obj = (VObjIF) m;
                  String s = obj.getAttribute(VObjDef.SHOW);
                  if (s != null && s.length() > 8)
                       m.setVisible(false);
               }
            }
        }
    }

} // class VMenuBar
