/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import javax.swing.*;
import javax.swing.event.*;
import java.awt.*;
import java.io.*;
import java.util.Vector;
import java.awt.event.*;

import vnmr.util.*;
import vnmr.ui.*;

public class VColorMapSelector extends JPanel
   implements  ActionListener, VColorMapListenerIF
{
    private String applySelected = "selected";
    private String applyDisplayed = "displayed";
    private String applyAll = "all";
    private String JFUNC = "jFunc(";
    private String JEVENT = "jMove3(";
    private String mapName = null;
    private DefaultListModel listModel;
    private JList nameList;
    private JPanel ctrlPan;
    private JComboBox applyCb;
    private boolean bDebugMode = false;
    private boolean bSilent = false;
    private Color  applyBg = null;
    private XVButton applyButton;
    private VColorLookupPanel lookupTable;
    private double translucentValue = 0;
    private Vector nameVector;


    public VColorMapSelector()
    {
        listModel = new DefaultListModel();
        nameList = new JList(listModel);
        JScrollPane js = new JScrollPane(nameList,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        js.setAlignmentX(LEFT_ALIGNMENT);

        FlowLayout flow = new FlowLayout();
        flow.setHgap(16);
        ctrlPan = new JPanel(flow);

        applyCb = new JComboBox();
        String str = Util.getLabel("blSelected", "Selected");
        applyCb.addItem(str);
        // str = Util.getLabel("blDisplayed", "Displayed");
        // applyCb.addItem(str);
        str = Util.getLabel("blAll", "All");
        applyCb.addItem(str);
        ctrlPan.add(applyCb);

        applyButton = addButton("Apply", this);

        addButton("Save", this);
        addButton("Editor", this);
 
        setLayout( new BorderLayout());
        add(js, BorderLayout.CENTER);
        add(ctrlPan, BorderLayout.SOUTH);

        applyBg = applyButton.getBgColor();
        nameList.addListSelectionListener(new ListSelectionListener() {
            public void valueChanged(ListSelectionEvent e) {
                listChange();
            }
        });
        mapName = VColorMapFile.DEFAULT_NAME;

        VColorMapFile.addListener(this);
    }

    private XVButton addButton(String label, Object obj)
    {
        XVButton but = new XVButton(Util.getLabel("bl"+label, label));
        but.addActionListener((ActionListener) obj);
        ctrlPan.add(but);
        return but;
    }

    public void setLookupPanel(VColorLookupPanel p) {
        lookupTable = p;
        if (mapName != null)
            openColormap(mapName);
        lookupTable.addTranslucencyListeners(this);
    }

    // VColorMapListenerIF
    public void updateMapList(Vector v) {
        if (v == null)
            return;
        nameVector = v;
        boolean bUpdateMap = true;
        listModel.removeAllElements();
        for (int i = 0; i < v.size(); i++) {
           String name = (String)v.elementAt(i);
           if (name != null) {
              listModel.addElement(name);
              if (bUpdateMap && name.equals(mapName))
                 bUpdateMap = false;
           }
        }
        if (bUpdateMap) { // the loaded map was deleted
           openColormap(VColorMapFile.DEFAULT_NAME);
        }
        else
           openColormap(mapName);
    }

    public void updateTranslucency(double v) {
        int n = applyCb.getSelectedIndex();
        if (translucentValue == v)
            return;
        translucentValue = v;
        ExpPanel exp = Util.getActiveView();
        if (bDebugMode)
            System.out.println("translucency: "+v);
        String mess = new StringBuffer().append(JEVENT).append(VnmrKey.OPAQUE)
                    .append(", ").append(n).append(", ").append(v).append(")\n").toString();
        if (bDebugMode)
            System.out.println("apply cmd: "+mess);
        if (exp == null)
            return;
        exp.sendToVnmr(mess);
    }

    public void rebuildList() {
        Vector v = VColorMapFile.getMapFileList();
        updateMapList(v);
    }

    public void turnOnApply(boolean b) {
        if (bDebugMode)
           System.out.println(" set apply button on: "+b);
        if (b)
           applyButton.setBgColor(Color.yellow);
        else
           applyButton.setBgColor(applyBg);
        applyButton.repaint();
    }

    private void applyMap() {
        int n = applyCb.getSelectedIndex();
        String arg;
        if (n == 0)
           arg = applySelected;
        else
           arg = applyAll;
        ExpPanel exp = Util.getActiveView();
        if (exp == null) {
            turnOnApply(false);
            return;
        }
        mapName = (String)nameList.getSelectedValue();
        if (mapName == null || mapName.length() < 1)
            return;

        String mess0 = new StringBuffer().append(JFUNC).append(VnmrKey.APPLYCMP)
                 .append(", ").append(n).append(", 0, '").append(mapName).append("')\n").toString();
        exp.sendToVnmr(mess0);
        String mess = new StringBuffer().append("aipSetColormap('")
             .append(mapName).append("','").append(arg).append("')\n").toString();
        if (bDebugMode)
           System.out.println("apply cmd: "+mess);
        exp.sendToVnmr(mess);

        turnOnApply(false);
        updateTranslucency(lookupTable.getTranslucency());
    }

    private void saveMap() {
        int n = applyCb.getSelectedIndex();
        String arg;
        if (n == 0)
           arg = applySelected;
        else
           arg = applyAll;
        // else if (n == 1)
        //    arg = applyDisplayed;
        if (bDebugMode)
            System.out.println("save "+arg);
        ExpPanel exp = Util.getActiveView();
        if (exp == null)
            return;
        String mess = new StringBuffer().append("aipSaveColormap('")
            .append(arg).append("')\n").toString();
        if (bDebugMode)
           System.out.println("cmd: "+mess);
        exp.sendToVnmr(mess);
    }

    public void setDebugMode(boolean b) {
        bDebugMode = b;
    }

    public void openColormap(String name) {
        if (name == null || lookupTable == null)
           return;
        String path = VColorMapFile.getMapFilePath(name);
        lookupTable.setLoadMode(true);
        lookupTable.loadColormap(name, path);
        if (!name.equals(mapName)) {
           mapName = name;
           turnOnApply(true);
        }
        lookupTable.setLoadMode(false);
    }

    public void setSelected(String name) {
        if (nameVector == null)
           return; 
        int i;
        boolean bGot = false;
        String str;
        bSilent = true;
        for (i = 0; i < nameVector.size(); i++) {
           str = (String)nameVector.elementAt(i);
           if (str != null) {
               if (str.equals(name)) {
                   nameList.setSelectedIndex(i);
                   bGot = true; 
                   break;
               }
           }
        }
        if (!bGot) {
           Vector paths = VColorMapFile.getMapPathList();
           if (paths != null) {
              for (i = 0; i < paths.size(); i++) {
                  str = (String)paths.elementAt(i);
                  if (str != null) {
                      if (str.equals(name)) {
                          bGot = true;
                          nameList.setSelectedIndex(i);
                          break;
                      }
                   }
               }
           }
        }
        if (bGot)
            turnOnApply(false);
        bSilent = false;
    }

    public void actionPerformed(ActionEvent  evt)
    {
        String cmd = evt.getActionCommand();

        if (cmd.equalsIgnoreCase("apply"))
        {
            applyMap();
            return;
        }
        if (cmd.equalsIgnoreCase("editor"))
        {
            ImageColorMapEditor editor = ImageColorMapEditor.getEditor();
            editor.showDialog(true);
            
            String name = (String)nameList.getSelectedValue();
            if (name == null)
                name = mapName;
            if (name != null) {
                String path = VColorMapFile.getMapFilePath(name);
                ImageColorMapEditor.loadColormap(name, path);
            }
            return;
        }
        if (cmd.equalsIgnoreCase("save"))
        {
            saveMap();
        }
    }

    public void listChange()
    {
        String newName = (String)nameList.getSelectedValue();
        if (bSilent)
            return;
        if (newName != null)
        {
             if (bDebugMode)
                 System.out.println(" listChange: "+newName);
             if (!newName.equals(mapName)) {
                 openColormap(newName);
             }
        }
    }

   
}

