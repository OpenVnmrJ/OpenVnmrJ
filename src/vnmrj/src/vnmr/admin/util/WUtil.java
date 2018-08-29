/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.util;

import java.io.*;
import java.awt.event.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.vobj.*;
import vnmr.admin.ui.*;

/**
 * Title:   WUtil
 * Description: This class has the utility methods implemented for the Wanda
 *              interface.
 * Copyright:    Copyright (c) 2002
 */

public class WUtil
{

    public static final int FOREGROUND = 10;
    public static final int BACKGROUND = 11;

    private WUtil()
    {
        // not meant to be used.
    }

    /**
     *  Concatenates two objects, and returns the new string.
     */
    public static String concat(Object obj1, Object obj2)
    {
        StringBuffer sbNew = new StringBuffer();

        if (obj1 != null)
            sbNew.append(obj1);
        if (obj2 != null)
            sbNew.append(obj2);

        return sbNew.toString();
    }

    /**
     *  Takes a string and returns the array list for that string.
     */
    public static ArrayList strToAList(String strTokenize)
    {
        return strToAList(strTokenize, true);
    }

    /**
     *  Takes a string and returns the array list for that string.
     *  @strTokenize    string from which an arraylist should be made.
     *  @strDelim       delimeter string for the tokenizer.
     */
    public static ArrayList strToAList(String strTokenize, String strDelim)
    {
        return strToAList(strTokenize, true, strDelim);
    }

    /**
     *  Takes a string and returns the array list for that string.
     *  @param strTokenize  the string from which an arraylist should be made.
     *  @param bAllowDups   true if duplicates should be allowed in the arraylist.
     */
    public static ArrayList strToAList(String strTokenize, boolean bAllowDups)
    {
        return strToAList(strTokenize, bAllowDups, " \t\n\r\f");
    }

    /**
     *  Takes a string and returns the array list for that string.
     *  @param strTokenize  the string from which an arraylist should be made.
     *  @param bAllowDups   true if duplicates should be allowed in the arraylist.
     *  @param strDelim     delimeter string for the tokenizer.
     */
    public static ArrayList strToAList(String strTokenize, boolean bAllowDups,
                                        String strDelim)
    {
        ArrayList<String> aListValues = new ArrayList<String>();
        String strValue;

        if (strTokenize == null)
            return aListValues;

        StringTokenizer strTokTmp = new StringTokenizer(strTokenize, strDelim);
        while(strTokTmp.hasMoreTokens())
        {
            strValue = strTokTmp.nextToken();
            if (strValue != null && strValue.trim().length() > 0)
            {
                if (bAllowDups || !aListValues.contains(strValue))
                    aListValues.add(strValue);
            }
        }

        return aListValues;
    }

    /**
     *  Takes an arraylist and returns a string.
     */
    public static String aListToStr(ArrayList aList)
    {
        return aListToStr(aList, " ");
    }

    public static String aListToStr(ArrayList aList, String strDelimiter)
    {
        StringBuffer sbLine = new StringBuffer();

        if (aList != null)
        {
            for(int i = 0; i < aList.size(); i++)
            {
                sbLine.append(aList.get(i));
                sbLine.append(strDelimiter);
            }
        }
        return sbLine.toString();
    }

    public static void blink(JComponent comp)
    {
        blink(comp, BACKGROUND);
    }

    public static void blink(JComponent comp, int nGround)
    {
        if (comp == null)
            return;

        int fontSize = (comp.getFont() != null) ? comp.getFont().getSize() : 10;
        int x = 0, y = fontSize, space;
        int red =   (int)( 50 * Math.random());
        int green = (int)( 50 * Math.random());
        int blue =  (int)(256 * Math.random());
        Color color = (nGround == FOREGROUND) ? comp.getForeground()
                        : comp.getBackground();

        if (Math.random() < 0.5)
            color = new java.awt.Color((red + y*30) % 256, (green + x/3) % 256, blue);

        if (nGround == BACKGROUND)
        {
            comp.setBackground(color);
        }
        else
        {
            comp.setForeground(color);
        }
    }

    public static boolean deleteFile(String strPath)
    {
        boolean bOk = true;
        if (strPath == null)
            return false;

        File flItem = new File(strPath);
        if (flItem != null)
            flItem.delete();
        else
            bOk = false;

        return bOk;
    }

    public static boolean renameFile(String strPath, String strRenamePath)
    {
        boolean bOk = true;
        if (strPath == null || strRenamePath == null)
            return false;

        /*File flItem = new File(strPath);
        File flRename = new File(strRenamePath);
        if (flItem != null && flRename != null)
            bOk = flItem.renameTo(flRename);
        else
            bOk = false;*/
        strPath = UtilB.windowsPathToUnix(strPath);
        strRenamePath = UtilB.windowsPathToUnix(strRenamePath);
        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, new StringBuffer().append(
                            "/bin/mv ").append(strPath).append(" ").append(
                            strRenamePath).toString()};
        bOk = runScript(cmd, false).isNoError();
        return bOk;
    }

    public static String unixDirToWindows(String strDir)
    {
        ArrayList aListDir = strToAList(strDir, ";");
        StringBuffer sbData = new StringBuffer();

        int nLength = aListDir.size();
        for (int i = 0; i < nLength; i++)
        {
            String strPath = (String)aListDir.get(i);
            StringBuffer sbPath = new StringBuffer(strPath.trim());
            // change escaped spaces to spaces
            int nIndex = sbPath.indexOf("\\");
            while (nIndex > 0)
            {
                sbPath.deleteCharAt(nIndex);
                if (nIndex+2 < sbPath.length())
                    nIndex = sbPath.indexOf("\\", nIndex+2);
                else
                    nIndex = -1;
            }
            sbData.append(UtilB.unixPathToWindows(sbPath.toString())).append(";");
        }
        return sbData.toString();
    }

    public static String windowsDirToUnix(String strDir)
    {
        ArrayList aListDir = strToAList(strDir, ";");
        StringBuffer sbData = new StringBuffer();
        
        int nLength = aListDir.size();
        for (int i = 0; i < nLength; i++)
        {
            String strPath = (String)aListDir.get(i);
            strPath = UtilB.windowsPathToUnix(strPath.trim(), true);
            StringBuffer sbPath = new StringBuffer(strPath);
            // escape spaces in paths
            int nIndex = sbPath.indexOf(" ");
            while (nIndex > 0)
            {
                sbPath.insert(nIndex, '\\');
                if (nIndex+2 < sbPath.length())
                    nIndex = sbPath.indexOf(" ", nIndex+2);
                else
                    nIndex = -1;
            }
            sbData.append(sbPath.toString()).append(";");
        }
        return sbData.toString();
    }

    public static WMessage runScript(String cmd)
    {
        return runScript(cmd, true);
    }

    public static WMessage runScript(String cmd, boolean bPostMsg)
    {
        String strg = "";
        boolean bOk = true;
        WMessage msg = new WMessage(bOk, strg);

        if (cmd == null)
            return msg;
        Process prcs = null;

        try
        {
            if (bPostMsg)
                Messages.postDebug("Running script: " + cmd);
            Runtime rt = Runtime.getRuntime();

            prcs = rt.exec(cmd);

            if (prcs == null)
                return msg;

            InputStream istrm = prcs.getInputStream();
            if (istrm == null)
                return msg;

            BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

            while((strg = bfr.readLine()) != null)
            {
                //System.out.println(strg);
                strg = strg.trim();
                if (bPostMsg)
                    Messages.postDebug(strg);
                String strg2 = strg.toLowerCase();
                bOk = (strg2.equals("-1") || strg2.indexOf("invalid") >= 0) ? false
                            : true;

                msg.bOk = bOk;
                msg.strMsg = strg;
            }
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            bOk = false;
            Messages.postDebug(e.toString());
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }

        return msg;
    }

    public static void runScriptInThread(String[] cmd)
    {
        runScriptInThread(cmd, false);
    }

    public static void runScriptInThread(final String[] cmd, final boolean bPostMsg)
    {
        new Thread(new Runnable()
        {
            public void run()
            {
                WMessage msg = runScript(cmd);
                if (bPostMsg && msg.bOk)
                {
                    Messages.postInfo(msg.getMsg());
                }
            }
        }).start();
    }
    
    public static void runScriptInThread(String cmd)
    {
        runScriptInThread(cmd, false);
    }

    public static void runScriptInThread(final String cmd, final boolean bPostMsg)
    {
        new Thread(new Runnable()
        {
            public void run()
            {
                WMessage msg = runScript(cmd);
                if (bPostMsg && msg.bOk)
                {
                    Messages.postInfo(msg.getMsg());
                }
            }
        }).start();
    }

    public static WMessage runScript(String[] cmd)
    {
        return runScript(cmd, true);
    }

    public static WMessage runScript(String[] cmd, boolean bShowMesg)
    {
        String strg = "";
        boolean bOk = true;
        WMessage msg = new WMessage(bOk, strg);

        if (cmd == null)
            return msg;
        Process prcs = null;

        try
        {
            if (bShowMesg)
                Messages.postDebug("Running script: " + cmd[2]);
            Runtime rt = Runtime.getRuntime();

            prcs = rt.exec(cmd);

            if (prcs == null)
                return msg;

            InputStream istrm = prcs.getInputStream();
            if (istrm == null)
                return msg;

            BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

            while((strg = bfr.readLine()) != null)
            {
                //System.out.println(strg);
                strg = strg.trim();
                if (bShowMesg)
                    Messages.postDebug(strg);
                String strg2 = strg.toLowerCase();
                bOk = (strg2.equals("-1") || strg2.indexOf("invalid") >= 0) ? false
                            : true;

                msg.bOk = bOk;
                msg.strMsg = strg;
            }
        }
        catch (Exception e)
            {
                //e.printStackTrace();
                Messages.writeStackTrace(e);
                bOk = false;
                Messages.postDebug(e.toString());
            }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }

        return msg;
    }

    public static WMessage runScriptNoWait(String cmd)
    {
        return runScriptNoWait(cmd, true);
    }

    public static WMessage runScriptNoWait(String cmd, boolean bPostMsg)
    {
        String strg = "";
        WMessage msg = new WMessage(true, strg);

        if (cmd == null)
            return msg;
        Process prcs = null;

        try
        {
            if (bPostMsg)
                Messages.postDebug("Running script: " + cmd);
            Runtime rt = Runtime.getRuntime();
            prcs = rt.exec(cmd);
        }
        catch (Exception e)
        {
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
        finally {
            try {
                if (prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if (os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if (is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if (is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }

        return msg;
    }

    public static WMessage runScriptNoWait(String[] cmd)
    {
        return runScriptNoWait(cmd, true);
    }

    public static WMessage runScriptNoWait(String[] cmd, boolean bPostMsg)
    {
        String strg = "";
        WMessage msg = new WMessage(true, strg);

        if (cmd == null)
            return msg;
        Process prcs = null;

        try
        {
            if (bPostMsg)
                Messages.postDebug("Running script: " + cmd[2]);
            Runtime rt = Runtime.getRuntime();
            prcs = rt.exec(cmd);
        }
        catch (Exception e)
        {
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
        finally {
            try {
                if (prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if (os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if (is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if (is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }

        return msg;
    }


    public static String getToken(String str, char delimiter, int n)
    {
        int start = 0;
        int ip = 1;
        int max = str.length();
        char c;

        while (start < max)
        {
            c = str.charAt(start);
            if (c == ' ')
                start++;
            else
                break;
        }
        while ((ip < n) && (start < max))
        {
            c = str.charAt(start);
            if (c == delimiter)
                ip++;
            start++;
        }
        ip = start;
        while (ip < max)
        {
            c = str.charAt(ip);
            if (c == delimiter)
                break;
            ip++;
        }
        if (ip - start <= 0)
            return null;
        return str.substring(start, ip);
    }

    public static String getDate()
    {
        String strDate = new Date().toString();
        StringBuffer sbDate = new StringBuffer(strDate);
        int nIndex = 0;
        while ((nIndex = sbDate.indexOf(" ")) > 0)
        {
            sbDate.deleteCharAt(nIndex);
        }
        return sbDate.toString();
    }

    public static String getAdminItype()
    {
        String strItype = null;
        String strUser = System.getProperty("user");
        String strPath = WFileUtil.getHMPath(strUser);
        HashMap hmUser = WFileUtil.getHashMap(strPath);

        if (hmUser != null)
            strItype = (String)hmUser.get("itype");

        return strItype;

    }

    public static void doColorAction(Color bgColor, String strColor, JComponent comp)
    {
        doColorAction(bgColor, strColor, comp, false);
    }

    public static void doColorAction(Color bgColor, String strColor,
                                        JComponent comp, boolean bSuperParent)
    {
        Component objComp = null;
        JComponent jComp = null;

        if (comp == null)
            return;

        Container container =  comp.getParent();
        if (container != null)
        {
            container.setBackground(bgColor);
        }
        comp.setBackground(bgColor);
        int nCompCount = comp.getComponentCount();

        for (int i = 0; i < nCompCount; i++)
        {
            objComp = comp.getComponent(i);
            if (objComp instanceof JComponent && isCompOk(objComp, bSuperParent))
            {
                jComp = (JComponent)objComp;
                jComp.setBackground(bgColor);
                if (jComp instanceof WObjIF)
                {
                    ((WObjIF)jComp).setAttribute(VObjDef.BGCOLOR, strColor);
                }
                if (jComp.getComponentCount() > 0)
                    doColorAction(bgColor, strColor, jComp, bSuperParent);
            }
        }
        comp.validate();
        comp.repaint();
    }

    protected static boolean isCompOk(Component objComp, boolean bSuperParent)
    {
        boolean bCompOk = true;
        // if it's superparent, meaning the container or the panel for the whole interface,
        // then set the background of all the children, except the itemarea panels,
        // and the detailarea panels, since background of those might be set independently.
        if ((objComp instanceof VItemAreaIF || objComp instanceof VDetailArea)
                && bSuperParent)
            bCompOk = false;
        return bCompOk;
    }


    /**
     *  Checks if the right mouse button was clicked/pressed.
     */
    public static boolean isRightMouseClick(MouseEvent e)
    {
        return ((e.getModifiers() & InputEvent.BUTTON3_MASK)
                            == InputEvent.BUTTON3_MASK);
    }

    /**
     *  Checks if the left mouse button was clicked.
     */
    public static boolean isLeftMouseClick(MouseEvent e)
    {
        return ((e.getModifiers() & InputEvent.BUTTON1_MASK)
                            == InputEvent.BUTTON1_MASK);
    }

    public static void setCurrentAdmin(String name)
    {
        AppIF appIf = Util.getAppIF();
        ((VAdminIF)appIf).setCurrentAdmin(name);
    }

    public static String getCurrentAdmin()
    {
        AppIF appIf = Util.getAppIF();
	if(appIf instanceof VAdminIF) {
           return ((VAdminIF)appIf).getCurrentAdmin();
	} else {
	   return System.getProperty("user.name");
	}
    }
}
