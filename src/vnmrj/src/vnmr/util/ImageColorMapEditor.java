/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.lang.Math;
import java.util.*;
// import java.io.ByteArrayOutputStream;
import java.io.*;
import javax.swing.*;
import javax.swing.event.ChangeListener;
import javax.swing.event.ChangeEvent;
import javax.swing.plaf.basic.BasicSplitPaneUI;
import javax.imageio.*;
import vnmr.bo.*;
import vnmr.ui.SessionShare;
import vnmr.ui.VNMRFrame;


@SuppressWarnings("serial")
public class ImageColorMapEditor extends JDialog {
    private static boolean  bTest = false;
    private static boolean  bDebugMode = false;
    private static ImageColorMapEditor editor = null;
    private static ImageColorMapEditor viewer = null;
    private static java.util.List <VColorMapPanel> mapList = null;
    private static java.util.List <VColorImageListenerIF> imageInfoList = null;
    private boolean bEditMode = true;
    private VColorMapEditor colorEditor = null;
    private VColorMapPanel colorPanel = null;
    protected VColorMapFile mapFile = null;

    public ImageColorMapEditor(boolean isEditor) {
        super(VNMRFrame.getVNMRFrame(), "Colormap", false);
        this.bEditMode = isEditor;
        setAlwaysOnTop(true); 
        buildGUi();
        loadColormap(VColorMapFile.DEFAULT_NAME, null);
        if (isEditor) {
           setTitle("Colormap Editor");
           this.editor = this;
        }
        else
           this.viewer = this;
    }

    public ImageColorMapEditor() {
        this(true);
    }

    public static ImageColorMapEditor getInstance() {
        if (viewer == null)
            viewer = new ImageColorMapEditor(false);
        return viewer;
    }

    public static ImageColorMapEditor getEditor() {
        if (editor == null)
            editor = new ImageColorMapEditor(true);
        return editor;
    }

    public static void close() {
        if (bTest)
           System.exit(0);
        else if (editor != null) {
           editor.saveGeom();
           editor.setVisible(false);
        }
    }

    public static void addColormapList(VColorMapPanel map) {
         if (mapList == null)
            mapList = Collections.synchronizedList(new LinkedList<VColorMapPanel>());
         if (!mapList.contains(map)) {
            mapList.add(map);
         }
    }

    public static void removeColormapList(VColorMapPanel map) {
         if (mapList == null)
             return;
         mapList.remove(map);
    }

    public static void addImageInfoListener(VColorImageListenerIF listener) {
         if (imageInfoList == null)
            imageInfoList = Collections.synchronizedList(new LinkedList<VColorImageListenerIF>());
         if (!imageInfoList.contains(listener)) {
            imageInfoList.add(listener);
         }
    }

    public static void removeImageInfoListener(VColorImageListenerIF listener) {
         if (imageInfoList == null)
             return;
         imageInfoList.remove(listener);
    }


    public static void setColorInfo(int id, int order, int mapId,
                              int transparency, String imgName) {
         if (imageInfoList == null)
             return;
         synchronized(imageInfoList) {
             Iterator itr = imageInfoList.iterator();
             while (itr.hasNext()) {
                  VColorImageListenerIF listener = (VColorImageListenerIF) itr.next();
                  listener.addImageInfo(id, order, mapId, transparency, imgName);
             }
         }

         /***********
         if (mapList == null)
             return;
         synchronized(mapList) {
             Iterator itr = mapList.iterator();
             while (itr.hasNext()) {
                  VColorMapPanel map = (VColorMapPanel) itr.next();
                  map.setColorInfo(id, order, mapId, transparency, imgName);
             }
         }
         ***********/
    }

    public static void selectColorInfo(int id) {
         if (imageInfoList == null)
             return;
         synchronized(imageInfoList) {
             Iterator itr = imageInfoList.iterator();
             while (itr.hasNext()) {
                  VColorImageListenerIF listener = (VColorImageListenerIF) itr.next();
                  listener.selectImageInfo(id);
             }
         }

         /***********
         if (mapList == null)
             return;
         synchronized(mapList) {
             Iterator itr = mapList.iterator();
             while (itr.hasNext()) {
                  VColorMapPanel map = (VColorMapPanel) itr.next();
                  map.selectColorInfo(id);
             }
         }
         ***********/
    }

    public static void selectColorInfo(VColorImageListenerIF caller, int id) {
         if (imageInfoList == null)
             return;
         synchronized(imageInfoList) {
             Iterator itr = imageInfoList.iterator();
             while (itr.hasNext()) {
                  VColorImageListenerIF listener = (VColorImageListenerIF) itr.next();
                  if (listener != caller)
                     listener.selectImageInfo(id);
             }
         }
    }

    public void showDialog(boolean bShow) {
        if (!bShow) {
           close();
           return;
        }
        setVisible(true);
    }

    public void setDebugMode(boolean b) {
        bDebugMode = b;
        VColorModel.setDebugMode(b);
        if (colorEditor != null)
            colorEditor.setDebugMode(b);
        if (colorPanel != null)
            colorPanel.setDebugMode(b);
    }

    private void buildGUi() {
         int w = 700;
         int h = 700;
         int x = 10;
         int y = 20;
         Hashtable hs = null;

         if (bTest) {
            if (bEditMode)
                x = 400;
            else
                x = 200;
         }
         else {
            SessionShare ss = Util.getSessionShare();
            if (ss != null)
               hs = ss.userInfo();
         }
         if (hs != null) {
            Integer ix = (Integer) hs.get("cmpWidth");
            if (ix != null)
                w = ix.intValue();
            ix = (Integer) hs.get("cmpHeight");
            if (ix != null)
                h = ix.intValue();
            ix = (Integer) hs.get("cmpLocX");
            if (ix != null)
                x = ix.intValue();
            ix = (Integer) hs.get("cmpLocY");
            if (ix != null)
                y = ix.intValue();
        }

        Container c = getContentPane();

        if (bEditMode) {
            colorEditor = new VColorMapEditor();
            c.add(colorEditor, BorderLayout.CENTER);
        }
        else {
            colorPanel = new VColorMapPanel();
            c.add(colorPanel, BorderLayout.CENTER);
        }

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent event) {
                 if (bTest)
                    System.exit(0);
            }

            public void windowOpened(WindowEvent event) {
            }
        });

        pack();
        setLocation(x, y);
        setSize(w, h);
    }

    public void saveGeom() {
         if (bTest || !bEditMode)
            return;
         Hashtable hs = null;
         SessionShare ss = Util.getSessionShare();
         if (ss != null)
            hs = ss.userInfo();
         if (hs == null)
            return;
         Dimension dim = getSize();
         if (dim != null) {
             hs.put("cmpWidth", new Integer(dim.width));
             hs.put("cmpHeight", new Integer(dim.height));
         }
         Point pt = getLocation();
         if (pt != null) {
             hs.put("cmpLocX", new Integer(pt.x));
             hs.put("cmpLocY", new Integer(pt.y));
         }
    }

    private void editorSetImage(BufferedImage img) {
        if (colorEditor != null)
            colorEditor.setImage(img);
    }

    private void editorSetImage(BufferedImage img, IndexColorModel cm,
                                int index0, int num) {
        if (colorEditor != null)
            colorEditor.setImage(img, cm, index0, num);
    }

    private boolean editorLoadColormap(String name, String path) {
        boolean bResult = false;
        if (colorEditor != null)
            bResult = colorEditor.loadColormap(name, path);
        if (name != null) {
            if (bEditMode)
               setTitle("Colormap Editor: "+name);
            else
               setTitle("Colormap: "+name);
        }
        else if (path != null) {
            if (bEditMode)
                setTitle("Colormap Editor: "+path);
            else
                setTitle("Colormap: "+path);
        }
        else {
            if (bEditMode)
                setTitle("Colormap Editor");
            else
                setTitle("Colormap");
        }
        return bResult;
    }

    private boolean editorSaveColormap(String path) {
        boolean bResult = false;
        if (colorEditor != null && path != null)
            bResult = colorEditor.saveColormap(path);
        return bResult;
    }

    private void editorSetColorNumber(int n) {
        if (colorEditor != null)
            colorEditor.setColorNumber(n);
    }

    // will be called by VColorMapFile
    public static boolean loadColormap(String name, String path) {
        boolean bResult = false;
        if (editor != null)
            bResult = editor.editorLoadColormap(name, path);
        return bResult;
    }

    // will be called by VColorMapFile
    public static boolean saveColormap(String path) {
        boolean bResult = false;
        if (editor != null)
            bResult = editor.editorSaveColormap(path);
        return bResult;
    }

    public static void setImage(BufferedImage img) {
        if (editor != null)
            editor.editorSetImage(img);
    }

    public static void setImage(BufferedImage img,IndexColorModel cm, int index0, int num) {
        if (editor != null)
            editor.editorSetImage(img, cm, index0, num);
    }

    public static void setImageColors(byte[] r, byte[] g, byte[] b, byte[] a) {
    }


    public static void setImageColorIndex(IndexColorModel cm, int gray0, int grayNum) {
    }

    public static void setColorNumber(int n) {
        if (editor != null)
            editor.editorSetColorNumber(n);
    }

    public static boolean isOpen() {
        if (editor != null)
            return editor.isShowing();
        return false;
    }

    public void openFilePopup() {
        if (mapFile == null) {
            mapFile = new VColorMapFile();
            //  mapFile.setAlwaysOnTop(true);
            Point loc = getLocationOnScreen();
            mapFile.setLocation(loc.x + 100, loc.y + 200);
        }
        mapFile.setVisible(true);
    }

    private static void initAppDirs(String path, ArrayList appDirectories, ArrayList appDirLabels) {
        // read persistence file
        // do nothing if does not exist.
        if (path == null)
            return;

        try {
            BufferedReader reader = new BufferedReader(new FileReader(path));

            String line;
            while ((line = reader.readLine()) != null) {
                if (line.startsWith("#") || line.startsWith("0"))
                    continue;

                StringTokenizer tok = new StringTokenizer(line, ";");
                if (tok.hasMoreTokens()
                        && tok.nextToken().trim().startsWith("0"))
                    continue;
                if (tok.hasMoreTokens()) {
                    String str = tok.nextToken().trim();
                    if (str.equalsIgnoreCase("USERDIR"))
                        str = FileUtil.usrdir();
                    str=UtilB.addWindowsPathIfNeeded(str);
                    appDirectories.add(str);
                    if (tok.hasMoreTokens()) {
                        // Save the Labels also
                        String lstr = tok.nextToken().trim();
                        appDirLabels.add(lstr);
                    }
                    else {
                        // If no label for some reason, just use the path
                        appDirLabels.add(str);
                    }
                }
            }

            reader.close();

        } catch (Exception e) { }
    }

    public static void initAppDirs(User user) {

        if (user == null)
            return;
        ArrayList appDirectories = user.getAppDirectories();
        ArrayList appDirLabels = user.getAppLabels();

        if (appDirectories == null) {
            appDirectories = new ArrayList();
        } else
            appDirectories.clear();

        if (appDirLabels == null) {
            appDirLabels = new ArrayList();
        } else
            appDirLabels.clear();
        String userName = user.getAccountName();
        if (userName == null)
            return;
        String path = null;
        boolean bCurrUser = userName.equals(System.getProperty("user"));
        if (!bCurrUser) return;

        String operatorName = user.getCurrOperatorName();

        if (operatorName != null) {
           path = FileUtil.openPath("USER/PERSISTENCE/appdir_"
                    + operatorName);
        }

        if(path != null) initAppDirs(path, appDirectories, appDirLabels);

        if (appDirectories.size() < 1) {
            String itype = user.getIType();
            if (itype != null && itype.equals(Global.IMGIF)) {
                path = FileUtil
                        .openPath("SYSTEM/USRS/userProfiles/appdirImaging.txt");
            } else if (itype != null && itype.equals(Global.WALKUPIF)) {
                path = FileUtil
                        .openPath("SYSTEM/USRS/userProfiles/appdirWalkup.txt");
            } else if (itype != null && itype.equals(Global.LCIF)) {
                path = FileUtil
                        .openPath("SYSTEM/USRS/userProfiles/appdirLcNmrMs.txt");
            } else {
                path = FileUtil
                        .openPath("SYSTEM/USRS/userProfiles/appdirWalkup.txt");
            }
            initAppDirs(path, appDirectories, appDirLabels);
        }
        FileUtil.setAppDirs(appDirectories, appDirLabels);


    }

    public static void main(String[] args) {
         boolean bEditor;
         ImageColorMapEditor ed; 
         bTest = true;
         LoginService loginService = LoginService.getDefault();
         String user = System.getProperty("user.name");
         User usr=loginService.getUser(user);

         if (usr != null) {
             initAppDirs(usr);
             FileUtil.setAppTypes(usr.getAppTypeExts());
         }
         bEditor = false;
         for (int i = 0; i < args.length; i++) {
             if (args[i].equalsIgnoreCase("editor"))
                 bEditor = true;
         }
         ed = new ImageColorMapEditor(bEditor);
         ed.setDebugMode(true);
         ed.setVisible(true);
    }
}
