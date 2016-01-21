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
import java.awt.*;
import java.io.*;
import java.util.Vector;
import java.awt.event.*;

import vnmr.util.*;
import vnmr.ui.*;

public class VColorMapNameEditor extends JPanel
   implements  ActionListener, VColorMapListenerIF
{
    private String mapName = null;
    private JPanel namePane;
    private JPanel ctrlPane;
    private JComboBox nameCb;
    private JLabel message;
    private boolean bUpdateMap = false;
    // private boolean bDebugMode = false;
    // private VColorLookupPanel lookupTable;


    public VColorMapNameEditor()
    {
        namePane = new JPanel();
        SimpleHLayout layout = new SimpleHLayout();
        layout.setExtendLast(true);
        layout.setVgap(10);
        namePane.setLayout(layout);
        String str = Util.getLabel("blMap_Name", "Map Name")+": ";
        JLabel lb = new JLabel(str);
        namePane.add(lb);
        nameCb = new JComboBox();
        nameCb.setEditable(true);
        if (nameCb.getMaximumRowCount() < 9)
           nameCb.setMaximumRowCount(9);
        nameCb.setActionCommand("Name");
        namePane.add(nameCb);
        ComboBoxEditor editor = nameCb.getEditor();
        if (editor != null) {
           JTextField text = (JTextField)editor.getEditorComponent();
           if (text != null) 
               text.setBackground(Color.white);
        }

        rebuildList();

        FlowLayout flow = new FlowLayout();
        flow.setHgap(16);
        ctrlPane = new JPanel(flow);
        addButton("Open", this);
        addButton("Save", this);
        addButton("Delete", this);
        addButton("Close", this);
        message = new JLabel("   ");;

        SimpleVLayout vLayout = new SimpleVLayout();
        setLayout(vLayout);
        add(namePane);
        add(ctrlPane);
        add(message);

        mapName = VColorMapFile.DEFAULT_NAME;

        VColorMapFile.addListener(this);
        nameCb.addActionListener(this);
    }


    private XVButton addButton(String label, Object obj)
    {
        XVButton but = new XVButton(Util.getLabel("bl"+label, label));
        but.setActionCommand(label);
        but.addActionListener((ActionListener) obj);
        ctrlPane.add(but);
        return but;
    }

    public void setLookupPanel(VColorLookupPanel p) {
        // lookupTable = p;
    }

    // VColorMapListenerIF
    public void updateMapList(Vector v) {
        if (v == null)
            return;
        bUpdateMap = true;
        nameCb.removeAllItems();
        for (int i = 0; i < v.size(); i++) {
           String name = (String)v.elementAt(i);
           if (name != null) {
              nameCb.addItem(name);
           }
        }
        if (v.size() > 1) {
           if (mapName != null)
               nameCb.setSelectedItem(mapName);
        }
        bUpdateMap = false;
    }

    public void setColormapName(String name, String path) {
        mapName = name;
        if (name != null)
            nameCb.setSelectedItem(name);
   }


    private void rebuildList() {
        Vector v = VColorMapFile.getMapFileList();
        updateMapList(v);
    }

    private void getMapName() {
        mapName = nameCb.getSelectedItem().toString();
        if (mapName != null)
            mapName = mapName.trim();
        if (mapName == null || mapName.length() < 1) {
            mapName = null;
            String mess = "Name field is empty. ";
            JOptionPane.showMessageDialog(this, mess);
            return;
        }
    }

    private void saveMap() {
        getMapName();
        if (mapName == null)
            return;
        String data = VColorMapFile.getUserMapFileName(mapName);
        String path = FileUtil.savePath(data, false);
        if (path == null) {
            data = "Could not save file '"+data+"'.\n Check permissions of this directory and any existing file.";
            JOptionPane.showMessageDialog(this, data);
            return;
        }
        boolean bChanged = false;
        File fd = new File(path);
        if (fd.exists()) {
            bChanged = true;
            data = "File '"+fd.getParent()+"' already exists.\n Do you want to replace the existing file?";
            int isOk = JOptionPane.showConfirmDialog(this,
                    data, "Save Colormap",
                    JOptionPane.OK_CANCEL_OPTION);
            if (isOk != 0)
                return;
        }
        if (ImageColorMapEditor.saveColormap(path)) {
             message.setText("Saved to "+path);
             VColorMapFile.updateListeners();
             VColorModelPool pool = VColorModelPool.getInstance();
             if (pool.reloadColormap(mapName, path)) {
                 if (bChanged) {
                    ExpPanel exp = Util.getActiveView();
                    if (exp != null) {
                        VnmrCanvas canvas = exp.getCanvas();
                        if (canvas != null)
                            canvas.reloadColormap(mapName);
                    }
                 }
             }
        }
    }

    private void openMap() {
        getMapName();
        if (mapName == null)
            return;
        String path = VColorMapFile.getMapFilePath(mapName);
        if (path == null && !mapName.equals(VColorMapFile.DEFAULT_NAME)) {
            String mess = " Colormap '"+mapName+"' does not exist. ";
            JOptionPane.showMessageDialog(this, mess);
            return;
        }
        if (ImageColorMapEditor.loadColormap(mapName, path))
            message.setText("Opened "+path);
    }

    private void deleteMap() {
        getMapName();
        if (mapName == null)
            return;
        String data = VColorMapFile.getUserMapDir(mapName);
        String path = FileUtil.openPath(data);
        if (path == null) {
            data = VColorMapFile.getSysMapDir(mapName);
            path = FileUtil.openPath(data);
            if (path == null) {
                String mess = " Colormap '"+mapName+"' does not exist. ";
                JOptionPane.showMessageDialog(this, mess);
                return;
            }
        }

        File fd = new File(path);
        if (!fd.canWrite()) {
            data = "Could not delete file '"+path+"'.\n Permissions denied.";
            JOptionPane.showMessageDialog(this, data);
            return;
        }
        data = "File '"+path+"' will be deleted!\n Are you sure you want to delete it?";
        int isOk = JOptionPane.showConfirmDialog(this,
                    data, "Delete Colormap",
                    JOptionPane.OK_CANCEL_OPTION);
        if (isOk != 0)
            return;
        if (fd.isDirectory())
            FileUtil.deleteDir(fd);
        else
            fd.delete();
        mapName = null;
        message.setText("Deleted "+path);
        VColorMapFile.updateListeners();
    }

    public void setDebugMode(boolean b) {
        // bDebugMode = b;
    }

    public void updateTranslucency(double v) {
    }

    public void actionPerformed(ActionEvent  evt)
    {
        String cmd = evt.getActionCommand();

        if (cmd.equalsIgnoreCase("name"))
        {
            if (!bUpdateMap)
                 message.setText("  ");
            return;
        }
        if (cmd.equalsIgnoreCase("save"))
        {
            saveMap();
            return;
        }
        if (cmd.equalsIgnoreCase("delete"))
        {
            deleteMap();
            return;
        }
        if (cmd.equalsIgnoreCase("open"))
        {
            openMap();
            return;
        }
        if (cmd.equalsIgnoreCase("close"))
        {
            ImageColorMapEditor.close();
            return;
        }
    }
}

