/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import javax.swing.*;
import java.io.*;
import java.util.*;
import java.awt.image.IndexColorModel;

public class VColorModelPool
{
    private Vector modelVector = null;
    private boolean bTranslucent = true;
    private static VColorModelPool pool = null;
    

    public VColorModelPool() {
        init();
    }

    public static VColorModelPool getInstance() {
        if (pool == null)
            pool = new VColorModelPool();
        return pool;
    }


    public void setTranslucent(boolean b) {
        bTranslucent = b;
    }

    // create or update colormap
    public VColorModel openColorModel(int id, String name, String path) {
        if (name == null || name.length() < 1)
            return null;
        if (modelVector == null)
            modelVector = new Vector();
        if (id < 0)
            id = 0;
        VColorModel cm = null;
        if (id >= modelVector.size())
            modelVector.setSize(id + 10);
        cm = (VColorModel) modelVector.elementAt(id);
        if (cm == null) {
            cm = new VColorModel(id, name, path, bTranslucent);
            modelVector.setElementAt(cm, id);
        }
        else {
            cm.load(path);
            if (name != null)
                cm.setName(name);
        }
        return cm;
    }

    public VColorModel openColorModel(String name, String path) {
        return openColorModel(0, name, path);
    }

    public VColorModel openColorModel(String name) {

        String path = VColorMapFile.getMapFilePath(name);
        if (path == null) {
             name = VColorMapFile.DEFAULT_NAME;
             path = VColorMapFile.DEFAULT_NAME;
        }
        return openColorModel(0, name, path);
    }

    public VColorModel getColorModel(String name) {
        if (name == null || name.length() < 1)
            name = VColorMapFile.DEFAULT_NAME;
        if (modelVector == null)
            modelVector = new Vector();

        VColorModel cm = null;
        for (int i = 0; i < modelVector.size(); i++) {
            cm = (VColorModel) modelVector.elementAt(i);
            if (cm != null) {
               if (name.equals(cm.getName()))
                   break;
            }
            cm = null;
        }
        if (cm != null)
            return cm;
        cm = openColorModel(name);
        return cm;
    }

    public VColorModel getColorModel(int num) {
        VColorModel cm = null;
        if (modelVector != null) {
            if (num < modelVector.size())
              cm = (VColorModel) modelVector.elementAt(num);
        }
        if (cm == null)
            cm = openColorModel(num, VColorMapFile.DEFAULT_NAME, VColorMapFile.DEFAULT_NAME);
        return cm;
    }


    public VColorModel getColorModel() {
        return getColorModel(VColorMapFile.DEFAULT_NAME);
    }

    public boolean reloadColormap(String name, String filePath) {
        if (name == null || name.length() < 1)
           return false;
        if (modelVector == null)
           return false;
        boolean bUpdated = false;
        for (int i = 0; i < modelVector.size(); i++) {
            VColorModel cm = (VColorModel) modelVector.elementAt(i);
            if (cm != null) {
               if (name.equals(cm.getName()) || name.equals(cm.getMapName())) {
                   cm.load(filePath);
                   if (i > 0)  // 0 is editor's map
                       bUpdated = true;
               }
            }
        }
        return bUpdated;
    }

    private void init() {
        bTranslucent = true;
        openColorModel(VColorMapFile.DEFAULT_NAME);
    }
}

