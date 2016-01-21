/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import java.io.*;
import java.awt.Toolkit;


public class VFontFactory {

    private static java.util.List<GraphicsFont> fontList = null;
    private static boolean bNewList = false;
    private static boolean bFirstList = true;
    private static long  fileTime = 0;

    private static String[] fontNames = { "Default", "AxisLabel", "AxisNum",
          "GraphLabel", "GraphText", "IntegralLabel", "IntegralNum",
          "PeakLabel", "PeakNum" };

    public VFontFactory() {
    }

    private static void addFont(String name) {
        if (name == null || name.length() < 1)
            return;
        if (name.equals("ScreenDpi"))
            return;

        GraphicsFont newGf = null;

        for (GraphicsFont gf: fontList) {
            if (name.equals(gf.fontName)) {
                newGf = gf;
                break;
            }
        }
        if (newGf != null)
            return;
        bNewList = true;
        newGf = new GraphicsFont(name);
        fontList.add(newGf);
    }

    private static String getPersistenceFile(String type){
        return "USER/PERSISTENCE/"+type;
    }

    private static void saveFonts() {
        String path = FileUtil.savePath(getPersistenceFile(".Fontarray"));
        if (path == null)
            return;

        PrintWriter os = null;

        try {
            os = new PrintWriter(new FileWriter(path));
            for (GraphicsFont gf: fontList) {
                 gf.writeToFile(os);
            }
        }
        catch(IOException er) {
            Messages.postError("FontUtil: can't write persistence file");
        }
        catch(Exception ex) {
        }
        finally {
            try {
                os.close();
            } catch (Exception e) {}
        }
    }

    public static void loadFonts() {
        String path = FileUtil.savePath(getPersistenceFile(".Fontlist"));
        if (path == null) {
            fileTime = 0;
            return;
        }
        File fd = new File(path);
        if (fd == null || !fd.exists() || !fd.canRead()) {
            fileTime = 0;
            return;
        }
        if (fd.lastModified() == fileTime)
            return;
        fileTime = fd.lastModified();
        BufferedReader ins = null;
        String line;
        try {
            ins = new BufferedReader(new FileReader(path));
            while ((line = ins.readLine()) != null) {
                StringTokenizer tok = new StringTokenizer(line, " \t\r\n");
                if (tok.hasMoreTokens()) {
                    addFont(tok.nextToken().trim());
                }
            }
        }
        catch (Exception e) {  }
        finally {
           try {
                if (ins != null)
                    ins.close();
           }
           catch (Exception e2) {}
        }
    }

    public static synchronized java.util.List<GraphicsFont> getList() {
        bNewList = false;
        if (fontList == null) {
            bFirstList = true;
            fontList = Collections.synchronizedList(new LinkedList<GraphicsFont>());
        }
        loadFonts();
        for (int n = 0; n < fontNames.length; n++)
            addFont(fontNames[n]);
        addFont(GraphicsFont.defaultName);
        if (bNewList)
            saveFonts();
        if (bFirstList) {
            bFirstList = false;
            return fontList;
        }
        java.util.List<GraphicsFont> newList =
              Collections.synchronizedList(new LinkedList<GraphicsFont>());

        for (GraphicsFont gf: fontList) {
            GraphicsFont newGf = new GraphicsFont(null, true);
            gf.clone(newGf);
            newList.add(newGf);
        }
        return newList;
    }
}

