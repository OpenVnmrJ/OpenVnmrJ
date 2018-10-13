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
import java.io.*;
import javax.swing.*;
import javax.swing.event.MouseInputAdapter;
import java.awt.geom.*;
import java.awt.event.*;
import java.awt.image.*;
import java.beans.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.templates.*;


public class VColorMapPanel extends JPanel implements VObjIF, VObjDef {
    private VColorLookupPanel lookupTable;
    private VColorScale vsEditor;
    private VColorMapSelector mapSelector;
    private SessionShare sshare;
    private ButtonIF vif;
    private String typeName;
    private JTabbedPane tabPan;
    private int colorNum = 0;
    private JSplitPane split;
    private JPanel colorPanel;
    private VColorImageSelector imgSelectPanel;

    public VColorMapPanel(SessionShare sshare, ButtonIF vif, String typ) {
         this.colorNum = 0;
         buildGUi();
    }

    public VColorMapPanel()
    {
         this(null, null, "colormap");
    }

    private void buildGUi() {
         sshare = Util.getSessionShare();
         vif = (ButtonIF) Util.getActiveView();
         typeName = "colormap";
         setLayout(new BorderLayout());

         colorPanel = new JPanel();
         colorPanel.setLayout(new BorderLayout());
         split = new JSplitPane(JSplitPane.VERTICAL_SPLIT,false);
         split.setDividerSize(6);

         colorNum = 64;
         lookupTable = new VColorLookupPanel(true);
         colorPanel.add(lookupTable, BorderLayout.NORTH);
         lookupTable.setColorNumber(colorNum);

         tabPan = new JTabbedPane();

         vsEditor = new VColorScale();
         mapSelector = new VColorMapSelector();
         tabPan.add("Maps", mapSelector);
         tabPan.add("Scale", vsEditor);

         colorPanel.add(tabPan, BorderLayout.CENTER);
         split.setTopComponent(colorPanel);

         imgSelectPanel = new VColorImageSelector();
         split.setBottomComponent(imgSelectPanel);
         
         add(split, BorderLayout.CENTER);

         mapSelector.setLookupPanel(lookupTable);
         imgSelectPanel.setLookupPanel(lookupTable);
         imgSelectPanel.setMapSelector(mapSelector);

         split.setResizeWeight(0.82);
         ImageColorMapEditor.addColormapList(this);
         Util.setColormapPanel(this);
    }

    public void setColorNumber(int n) {
        int num = 0;
        int[] vlist = VColorMapFile.SIZE_VALUE_LIST;

        for (int i = 0; i < vlist.length; i++) {
           if (n == vlist[i]) {
                num = vlist[i];
                break;
           }
        }
        if (num == 0)
           num = vlist[0];
        if (num == colorNum)
           return;

        colorNum = num;
        lookupTable.setColorNumber(num);
    }

    public void setDebugMode(boolean b) {
        lookupTable.setDebugMode(b);
        mapSelector.setDebugMode(b);
    }

    public void saveColormap(String path) {
    }

    private void setLoadMode(boolean b) {
        lookupTable.setLoadMode(b);
    }

    public void loadDefaultColormap(String name) {
        setColorNumber(64);
        lookupTable.loadColormap(name, name);

    }

    public void loadColormap(String name, String path) {
        if (path != null)
            path = FileUtil.openPath(path);
        setLoadMode(true);
        if (path == null) {
            if (name != null) {
               if (name.equals(VColorMapFile.DEFAULT_NAME))
                   loadDefaultColormap(name);
            }
            setLoadMode(false);
            return;
        }
        int num = 0;
        BufferedReader ins = null;
        String line;

        try {
            ins = new BufferedReader(new FileReader(path));
            while ((line = ins.readLine()) != null) {
                if (line.startsWith("#"))
                    continue;
                StringTokenizer tok = new StringTokenizer(line, " \t\r\n");
                if (tok.hasMoreTokens()) {
                    String data = tok.nextToken();
                    if (data.equals(VColorMapFile.SIZE_NAME)) {
                        if (tok.hasMoreTokens()) {
                            data = tok.nextToken();
                            num = Integer.parseInt(data);
                            break;
                        }
                    }
                }
            }
        } catch (Exception e) {
            setLoadMode(false);
            num = 0;
        }
        finally {
            try {
               if (ins != null)
                   ins.close();
            }
            catch (Exception e2) {}
        }
        if (num > 2) {
           setColorNumber(num);
           lookupTable.loadColormap(name, path);
        }
        setLoadMode(false);
    }

    public void loadColormap(String name) {
        if (name == null)
            return;
        String path = VColorMapFile.getMapFilePath(name);
        if (path == null)
            return;
        loadColormap(name, path); 
    }

    public void setColorInfo(int id, int order, int mapId,
                              int transparency, String imgName) {
       imgSelectPanel.addImageInfo(id, order,mapId,transparency, imgName);
    }

    public void selectColorInfo(int id) {
       imgSelectPanel.selectImageInfo(id);
    }

    // VObjIF interface

    public void setAttribute(int t, String s) {
    }

    public String getAttribute(int t) {
        return null;
    }

    public void setEditStatus(boolean s) { }
    public void setEditMode(boolean s) {}
    public void addDefChoice(String s) { }
    public void addDefValue(String s) { }
    public void setDefLabel(String s) { }
    public void setDefColor(String s) { }
    public void setDefLoc(int x, int y) { }
    public void refresh() { }
    public void changeFont() { }
    public void updateValue() { }
    public void setValue(ParamIF p) { }
    public void setShowValue(ParamIF p) { }
    public void changeFocus(boolean s) { }
    public ButtonIF getVnmrIF() {
         return null;
    }
    public void setVnmrIF(ButtonIF vif) { }
    public void destroy() { }
    public void setModalMode(boolean s) { }
    public void sendVnmrCmd() { }
    public void setSizeRatio(double w, double h) { }
    public Point getDefLoc() {
         return new Point(0, 0);
    }

}
