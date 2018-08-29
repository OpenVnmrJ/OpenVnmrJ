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
import java.util.StringTokenizer;
import javax.swing.*;
import java.beans.*;
import vnmr.util.*;
import vnmr.templates.*;
import vnmr.bo.*;

public class SysToolBar extends JMenuBar implements ExpListenerIF, PropertyChangeListener {
    protected SessionShare sshare;
    protected ButtonIF vnmrIf;
    protected int id;

    public SysToolBar(SessionShare sh, ButtonIF vif, int num)
    {
        this(sh, vif, num, "ToolMenu.xml");
    } // SysToolBar()


    public SysToolBar(SessionShare sh, ButtonIF vif, int num, String strFilePath)
    {
        this.sshare = sh;
        this.vnmrIf = vif;
        this.id = num;
       //  setOpaque(false);
        setBackground(Util.getToolBarBg());
        setBorder( BorderFactory.createEmptyBorder() );
        strFilePath = FileUtil.openPath("INTERFACE/"+ strFilePath);
        if(strFilePath != null) {
            try {
                LayoutBuilder.build(this,vnmrIf,strFilePath);
            }  catch(Exception e) {
                Messages.writeStackTrace(e);
            }

        }
        setLayout(new toolBarLayout());
        DisplayOptions.addChangeListener(this);
        setButtonMenu();
        setBg();
    } // SysToolBar()


    public void setBg() {
        int k = getComponentCount();
        Color bg = Util.getToolBarBg();
        setBackground(bg);
        for (int i = 0; i < k; i++) {
            Component m = getComponent(i);
            if (m != null) {
               m.setBackground(bg);
            }
        }
    }

    private void setButtonMenu() {
        int i;
        int k = getComponentCount();
        if (k < 1)
            return;
        Component compList[] = new Component[k];
        for (i = 0; i < k; i++)
            compList[i] = getComponent(i);
        removeAll();
        for (i = 0; i < k; i++) {
            Component m = compList[i];
            if (m instanceof VMenuitemIF) {
                VMenuitemIF vf = (VMenuitemIF) m;
                vf.mainMenuBarItem(true);
                vf.changeLook();
                if (m instanceof VSubMenu) {
                    ((VSubMenu)m).setButtonMenu(true);
                    Component button = ((VSubMenu)m).getMenuButton();
                    add(button);
                }
            }
            if (m instanceof VButton) {
                ((VButton)m).setToolBarButton(true);
            }
            add(m);
        }
    }

    public void propertyChange(PropertyChangeEvent evt){
        if(DisplayOptions.isUpdateUIEvent(evt))
            setBg();
    }

    public void  destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void updateValue() {
        int k = getComponentCount();
        for (int i = 0; i < k; i++) {
            Component m = getComponent(i);
            if (m != null && (m instanceof VObjIF)) {
                VObjIF e = (VObjIF) m;
                e.updateValue();
            }
        }
    }

    private boolean hasVariable(String vars, Vector params){
        if (vars == null)
            return false;
        StringTokenizer tok=new StringTokenizer(vars, " \t,\n");
        int nLength = params.size();
        while (tok.hasMoreTokens()) {
            String var = tok.nextToken().trim();
            for (int k = 0; k < nLength; k++){
                if (var.equals(params.elementAt(k)))
                    return true;
            }
        }
        return false;
    }

    public void updateValue (Vector params) {
        int k = getComponentCount();
        VObjIF obj;
        String varstring;
        for (int i = 0; i < k; i++) {
            Component m = getComponent(i);
            if (m != null && (m instanceof VObjIF)) {
                if (m instanceof ExpListenerIF) {
                    ((ExpListenerIF) m).updateValue(params);
                }
                else {
                    obj = (VObjIF) m;
                    varstring = obj.getAttribute(VObjDef.VARIABLE);
                    if (hasVariable(varstring, params))
                        obj.updateValue();
                }
            }
        }
    }

    private class toolBarLayout implements LayoutManager {

       public void addLayoutComponent(String name, Component comp) {}

       public void removeLayoutComponent(Component comp) {}

       public Dimension preferredLayoutSize(Container target) {
          int w = 0;
          int h = 0;
          int k = getComponentCount();
          for (int i = 0; i < k; i++) {
            Component m = getComponent(i);
            if (m != null) {
                Dimension d = m.getPreferredSize();
                w = w + d.width + 2;
                if (m instanceof VSubMenu)
                   w += 4;
                if (d.height > h)
                   h = d.height;
            }
          }
          if (k > 0) {
             h += 2;
          }   
          return new Dimension(w, h);
       }

       public Dimension minimumLayoutSize(Container target) {
           return new Dimension(0, 0); // unused
       }

       public void layoutContainer(Container target) {
           synchronized (target.getTreeLock()) {
              Dimension dim = target.getSize();
              int dw = dim.width;
              int dh = dim.height - 2;
              int x = 2;
              int y = 1;
              int k = getComponentCount();
              boolean bMenu = false;
              if (dh < 6 || dw < 10) // any number
                  return;
              for (int i = 0; i < k; i++) {
                  Component m = getComponent(i);
                  if (m != null && m.isVisible()) {
                     Dimension d = m.getPreferredSize();
                     if (m instanceof VSubMenu) {
                         if (!bMenu)
                            x = x - 2;
                         m.setBounds(x, y, d.width+2, dh);
                         x = x + d.width + 6;
                         bMenu = true;
                     }
                     else {
                         m.setBounds(x, y, d.width, dh);
                         x = x + d.width + 2;
                         bMenu = false;
                     }
                  }
              }
           }
       }
   }
} // class SysToolBar
