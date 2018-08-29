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


public class VColorMapEditor extends JPanel {
    private VColorLookupPanel lookupTable;
    private VColorPicker colorPicker;
    private VColorMapFile mapFilePanel;
    private VColorMapNameEditor namePanel;
    private SessionShare sshare;
    private ButtonIF vif;
    private String typeName;
    private JTabbedPane tabPan;
    private int colorNum = 0;

    public VColorMapEditor()
    {
         this.colorNum = 0;
         buildGUi();
    }

    private void buildGUi() {
         sshare = Util.getSessionShare();
         vif = (ButtonIF) Util.getActiveView();
         typeName = "colormap";
         setLayout(new BorderLayout());

         colorNum = 64;
         lookupTable = new VColorLookupPanel();
         add(lookupTable, BorderLayout.NORTH);
         lookupTable.setColorNumber(colorNum);

         colorPicker = new VColorPicker();
         colorPicker.setColorNumber(colorNum);
         mapFilePanel = new VColorMapFile();

         /************
         tabPan = new JTabbedPane();
         tabPan.add("Color Editor", colorPicker);
         tabPan.add("Colormaps", mapFilePanel);
         add(tabPan, BorderLayout.CENTER);
         ************/

         add(colorPicker, BorderLayout.CENTER);

         colorPicker.addColorEventListener(lookupTable);
         lookupTable.addColorEventListener(colorPicker);
         lookupTable.setColorMapEditor(this);
         colorPicker.setColorMapEditor(this);
         colorPicker.setLookupPanel(lookupTable);
         mapFilePanel.setLookupPanel(lookupTable);
         namePanel = new VColorMapNameEditor();
         add(namePanel, BorderLayout.SOUTH);
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
        colorPicker.setColorNumber(num);
    }

    public void setDebugMode(boolean b) {
        lookupTable.setDebugMode(b);
        mapFilePanel.setDebugMode(b);
    }

    public void setImage(BufferedImage img) {
        lookupTable.setImage(img, null, 0, 0);
    }

    public void setImage(BufferedImage img, IndexColorModel cm, int grayIndex0, int grayNum) {
        lookupTable.setImage(img, cm, grayIndex0, grayNum);
        if (img == null)
             return;
        int index0 = lookupTable.getFirstIndex();
        int num = lookupTable.getColorSize();
        if (num < 1)
             return;
        colorPicker.setRedValues(index0, num, lookupTable.getRedBytes());
        colorPicker.setGreenValues(index0, num, lookupTable.getGreenBytes());
        colorPicker.setBlueValues(index0, num, lookupTable.getBlueBytes());
    }

    public void setImageColors(byte[] r, byte[] g, byte[] b, byte[] a) {
    }

    public void setImageColorIndex(IndexColorModel cm, int index0, int num) {
    }

    public boolean saveColormap(String path) {
        path = FileUtil.savePath(path, false);
        if (path == null)
            return false;

        PrintWriter outs = null;

        try {
            outs = new PrintWriter(
                new OutputStreamWriter(
                new FileOutputStream(path), "UTF-8"));
        }
        catch(IOException er) { }
        if (outs == null) {
            String mess = "Could not save file '"+path+"'.\n Check permissions of this directory and any existing file.";

            JOptionPane.showMessageDialog(this, mess);
            return false;
        }
        int n;
        double dv; 
        outs.println("# Number of Colors");
        outs.println(VColorMapFile.SIZE_NAME+"  "+Integer.toString(colorNum));
        outs.println("# Translucency, range from 0.0 to 1.0");
        dv = lookupTable.getTranslucency();
        outs.println(VColorMapFile.TRANSLUCENCY_NAME+"  "+Double.toString(dv));

        outs.println("# Color 0 is the color of below minimum scale threshold");
        n = colorNum + 1; 
        outs.print("# Color "+Integer.toString(n));
        outs.println(" is the color of above maximum scale threshold");
        outs.println("# index Red Green Blue Transparent Translucent");

        outs.println(VColorMapFile.BEGIN_NAME);
        lookupTable.saveColormap(outs);
        outs.println(VColorMapFile.END_NAME);

        colorPicker.saveColormap(outs);
        outs.close();
        return true;
    }

    private void setLoadMode(boolean b) {
        lookupTable.setLoadMode(b);
        colorPicker.setLoadMode(b);
    }

    public void loadDefaultColormap(String name) {
        setColorNumber(64);
        lookupTable.loadColormap(name, name);
        colorPicker.loadColormap(name, name);
        colorPicker.loadFromLookup(lookupTable);
        mapFilePanel.setColormapName(name, name);
        namePanel.setColormapName(name, name);
    }

    public boolean loadColormap(String name, String path) {
        if (path != null)
            path = FileUtil.openPath(path);
        setLoadMode(true);
        if (path == null) {
            if (name != null) {
               if (name.equals(VColorMapFile.DEFAULT_NAME))
                   loadDefaultColormap(name);
            }
            setLoadMode(false);
            return false;
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
           colorPicker.loadColormap(name, path);
           colorPicker.loadFromLookup(lookupTable);
           mapFilePanel.setColormapName(name, path);
           namePanel.setColormapName(name, path);
        }
        setLoadMode(false);
        return true;
    }

    public void loadColormap(String name) {
        if (name == null)
            return;
        String path = VColorMapFile.getMapFilePath(name);
        if (path == null)
            return;
        loadColormap(name, path); 
    }
}
