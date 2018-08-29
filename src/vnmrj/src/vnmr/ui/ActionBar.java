/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.io.File;
import java.util.*;
import java.awt.*;

import javax.swing.*;

import vnmr.templates.LayoutBuilder;
import vnmr.util.*;
import vnmr.bo.*;

/**
 * A tool bar that handles parameter changes for its child VObj's
 */
public class  ActionBar extends ParamPanel implements ParamListenerIF, StatusListenerIF {
    private static final long serialVersionUID = 1L;
    private String name = null;
    public static boolean bSquish = true;
    public VActionBarLayout vlayout;
    public  int MAXHT=25;

    public ActionBar() {
        setPanelName("ActionBar");
        vlayout = new VActionBarLayout();
        setLayout(vlayout);
        ExpPanel.addStatusListener(this);
    }

    /** set file name strings.*/
    public void setFileInfo(ExpPanInfo info) {
        restoreFile = info.afpathIn;   // where first found
        panelName=info.name;          // e.g. Setup
        panelFile=info.afname;         // e.g. sample.xml
        layoutDir=info.layoutDir;     // e.g. dept
        panelType=info.ptype;
        vlayout.setName(info.name);
        
        //System.out.println(info);
    }
    public void setDebug(String s){
        if(DebugOutput.isSetFor("ActionBar"))
            Messages.postDebug("ActionBar("+panelName+")"+s);
    }
    public void setName(String str) {
        name = str;
        setPanelName(name);
    }

    public Dimension getActualSize() {
        return vlayout.preferredLayoutSize(this);
    }
    public void setPreferredLayoutSize(){
        vlayout.preferredLayoutSize(this);
    }
    public void setEditMode(boolean s){
        int edits=ParamInfo.getEditItems();
        if(s && (edits & ParamInfo.ACTIONBAR)==0)
            s=false;
        if(editMode==s)
            return;
        vlayout.setEditMode(s);
        super.setEditMode(s);       
        adjustLayout();
        repaint();
    }
    public void restore() {
        File file=new File(restoreFile);
        if (!file.exists()) {
            Messages.postError("restore : "+restoreFile+" does not exist");
            return;
        }

        setDebug("restore()");
        removeAll();
        try {
            LayoutBuilder.build(this,VnmrIf,restoreFile);
        }
        catch(Exception e) {
            Messages.postError("error rebuilding panel");
            Messages.writeStackTrace(e);
        }
        if(editMode){           
            super.setEditMode(false); 
            vlayout.setEditMode(false);
            adjustLayout();
            super.setEditMode(true); 
            vlayout.setEditMode(true);            
            adjustLayout();
        }
        validate();
        repaint();
    }
    public void updateStatus(String msg)
    {
        int nCompCount = getComponentCount();
        Component comp;
        for (int i = 0; i < nCompCount; i++)
        {
            comp = getComponent(i);
            if (comp instanceof StatusListenerIF)
                ((StatusListenerIF)comp).updateStatus(msg);
        }
    }

    public void updateValue(Vector params) {
        ParamListenerUtils.updateValue(this, params);
    }

    public void updateAllValue() {
        ParamListenerUtils.updateAllValue(this);
    }

    private void checkSquish(JComponent p) {
         int count = p.getComponentCount();

         for (int k = 0; k < count; k++) {
             Component comp = p.getComponent(k);
             if (comp != null && (comp instanceof VGroup)) {
                 String s = ((VObjIF)comp).getAttribute(VObjDef.SQUISH);
                 if (s != null) {
                    if (s.equalsIgnoreCase("no")) {
                         bSquish = false;
                         return;
                    }
                 }
                 checkSquish((JComponent) comp);
             }
          }
    }

    private void checkSquish() {
         bSquish=true;
         int count = getComponentCount();

         for (int k = 0; k < count; k++) {
             Component comp = getComponent(k);
             if (comp != null && (comp instanceof VGroup)) {
                 String s = ((VObjIF)comp).getAttribute(VObjDef.SQUISH);
                 if (s != null) {
                     if (s.equalsIgnoreCase("no")) {
                         bSquish = false;
                         return;
                     }
                 }
                 checkSquish((JComponent) comp);
                 if (!bSquish)
                     return;
             }
         }
    }


    public void adjustLayout(JComponent p, int deep) {
         int count = p.getComponentCount();

         for (int k = 0; k < count; k++) {
             Component comp = p.getComponent(k);
             if (comp != null) {
                 if (comp instanceof VGroup) {
                      JComponent c = (JComponent) comp;
                      VActionBarLayout alayout = new VActionBarLayout(deep);
                      alayout.setName(panelName+":Group:"+alayout.getLevel());
                      alayout.setSquish(bSquish);
                      alayout.setEditMode(editMode);
                      c.setLayout(alayout);
                      adjustLayout(c, deep+1);
                      if (bSquish)
                          comp.setPreferredSize(null);
                 }
                 else if (!editMode && bSquish) {
                     if (comp instanceof VSizeIF) {
                          Dimension dim = ((VSizeIF)comp).getMinSize();
                          dim.height += 10;
                          dim.width += 8;
                          if (dim.height > MAXHT)
                              dim.height = MAXHT;
                          comp.setPreferredSize(dim);
                     }
                     else {
                          if (!(comp instanceof JLabel)) {
                              comp.setPreferredSize(null);
                              Dimension dimx = comp.getPreferredSize();
                              if (dimx.height > MAXHT)
                                   dimx.height = MAXHT;
                              comp.setPreferredSize(dimx);
                          }
                     }
                 }
             }
         }
    }

    public void adjustLayout() {
         checkSquish();
         vlayout.setSquish(bSquish);
         vlayout.setEditMode(editMode);
         int count = getComponentCount();
         setDebug(".adjustLayout children:"+count+" depth:"+vlayout.getLevel());

         for (int k = 0; k < count; k++) {
             Component comp = getComponent(k);
             if (comp != null) { // NOTE: should only be one subgroup in panel
                 if (comp instanceof VGroup) { 
                      JComponent c = (JComponent) comp;
                      VActionBarLayout alayout = new VActionBarLayout(1);
                      alayout.setName(panelName+":Group:"+alayout.getLevel());
                      alayout.setSquish(bSquish);
                      alayout.setEditMode(editMode);
                      c.setLayout(alayout);
                      adjustLayout(c, 2);
                      if (bSquish)
                          comp.setPreferredSize(null);
                 }
                 else if (bSquish) { 
                     if (comp instanceof VSizeIF) {
                          Dimension dim = ((VSizeIF)comp).getMinSize();
                          dim.height += 6;
                          dim.width += 8;
                          if (dim.height > MAXHT)
                              dim.height = MAXHT;
                          comp.setPreferredSize(dim);
                     }
                     else {
                          if (!(comp instanceof JLabel)) {
                              comp.setPreferredSize(null);
                              Dimension dimx = comp.getPreferredSize();
                              if (dimx.height > MAXHT)
                                   dimx.height = MAXHT;
                              comp.setPreferredSize(dimx);
                          }
                      }
                 }
             }
         }
         setPreferredSize(null);  // forces size to be defined by that of topmost group
    }

    public static boolean isSquishAble() {
         return bSquish;
    }
}
