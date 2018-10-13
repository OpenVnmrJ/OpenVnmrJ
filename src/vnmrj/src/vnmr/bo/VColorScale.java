/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.*;
import java.util.*;
import javax.swing.*;

import vnmr.util.*;
import vnmr.ui.*;
import vnmr.templates.*;

public class VColorScale extends JPanel implements ExpListenerIF { 
    private VVsControl vsControl;
    private JPanel topPanel;
    private JPanel vsPanel;
    private JPanel bottomPanel;
    private ButtonIF vif;

    public VColorScale() {

        buildUi();
        ExpPanel.addExpListener(this);
    }

    private void buildUi() {
        setLayout(new BorderLayout());
        vif = (ButtonIF) Util.getActiveView();
        topPanel = new JPanel();
        topPanel.setLayout(new SimpleHLayout());
        createTopPanel();
        add(topPanel, BorderLayout.NORTH);
        
        vsPanel = new JPanel();
        vsPanel.setLayout(new BorderLayout());
        createMiddlePanel();
        add(vsPanel, BorderLayout.CENTER);


        bottomPanel = new JPanel();
        createBottomPanel();
        add(bottomPanel, BorderLayout.SOUTH);
    }
   
    @Override
    public void setVisible(boolean b) {
        super.setVisible(b);
        if (b) 
           updateValue();
    }

    private void updatePanel(JPanel p, Vector v) {
        StringTokenizer tok;
        boolean         got;
        int nums = p.getComponentCount();
        for (int i = 0; i < nums; i++) {
            Component comp = p.getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF vobj = (VObjIF) comp; 
                if (v == null) {
                    vobj.setVnmrIF(vif);
                    vobj.updateValue();
                    continue;
                }
                if (comp instanceof ExpListenerIF) {
                    ((ExpListenerIF)comp).updateValue(v);
                    continue;
                }
                if (vobj instanceof VGroup) {
                    vobj.setVnmrIF(vif);
                    ((VGroup)comp).updateValue(v);
                    continue;
                }
                String names = vobj.getAttribute(VObjDef.VARIABLE);
                if (names == null) 
                    continue;
                tok = new StringTokenizer(names, " ,\n");
                got = false;
                while (tok.hasMoreTokens()) {
                  String name = tok.nextToken();
                  for (int k = 0; k < v.size(); k++) {
                      String paramName = (String) v.elementAt(k);
                      if(name.equals(paramName)) {
                          got = true;
                          vobj.setVnmrIF(vif);
                          vobj.updateValue();
                          break;
                      }
                  }
                  if (got)
                      break;
               }
            }
        }
    }

    // interface of ExpListenerIF 
    public void updateValue(Vector v) {
        if (!isShowing())
           return;
        vif = (ButtonIF) Util.getActiveView();
        if (v == null || vif == null)
           return;
        updatePanel(topPanel, v);
        updatePanel(vsPanel, v);
        updatePanel(bottomPanel, v);
    }

    public void updateValue() {
        vif = (ButtonIF) Util.getActiveView();
        if (!isShowing() || vif == null)
           return;
        updatePanel(topPanel, null);
        updatePanel(vsPanel, null);
        updatePanel(bottomPanel, null);
        vif.sendToVnmr("aipSetVsFunction('hist')");
    }


    private void createTopPanel() {
       String name = "aipSclTop.xml";
       String path = FileUtil.openPath("INTERFACE/"+name);
       if (path == null)
           path = FileUtil.getLayoutPath("default",name);
       if (path == null)
          return;
       try {
          LayoutBuilder.build(topPanel,vif,path);
       }
       catch (Exception e) {
       }
    }

    private void createMiddlePanel() {
       String name = "aipSclMiddle.xml";
       String path = FileUtil.openPath("INTERFACE/"+name);
       if (path == null)
           path = FileUtil.getLayoutPath("default",name);
       if (path == null)
          return;
       try {
          LayoutBuilder.build(vsPanel,vif,path);
       }
       catch (Exception e) {
       }
    }

    private void createBottomPanel() {
       String name = "aipSclBottom.xml";
       String path = FileUtil.openPath("INTERFACE/"+name);
       if (path == null)
           path = FileUtil.getLayoutPath("default",name);
       if (path == null)
          return;
       try {
          LayoutBuilder.build(bottomPanel,vif,path);
       }
       catch (Exception e) {
       }

    }
}

