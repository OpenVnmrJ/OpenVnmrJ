/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;


/* This is a set of some utilities that are used in both vnmrj and managedb,
   thus this file is included for both.  Some other utilities which are
   for vnmrj only are in UtilA.  Currently, these items are primarily related
   to Windows versus Unix conversions.
*/

public class UtilB {

    // ==== static variables
    public static String OSNAME = System.getProperty("os.name");
    public static String SYSDIR = System.getProperty("sysdir");
    public static String SFUDIR_WINDOWS =
                                       System.getProperty("sfudirwindows");
    public static String SFUDIR_INTERIX =
                                       System.getProperty("sfudirinterix");
    public static String SHTOOLCMD = 
                                       System.getProperty("shtoolcmd");
    public static String SHTOOLOPTION = 
                                       System.getProperty("shtooloption");
    static  {
        // Don't allow these to be null, default to empty strings,
        // or unix appropriate strings.
        if(SHTOOLCMD == null)
            SHTOOLCMD = "/bin/sh";
        if(SHTOOLOPTION == null)
            SHTOOLOPTION = "-c";
        if(SFUDIR_WINDOWS == null)
            SFUDIR_WINDOWS = "";
        if(SFUDIR_INTERIX == null)
            SFUDIR_INTERIX = "";
        if (SYSDIR == null)
            SYSDIR = "/vnmr";
    }



    private UtilB() {
    }

    /* Prepend the Windows path (SFUDIR_WINDOWS) IF we are running
     * on Windows AND IF it is not already the start of the path.
     **/
    static public String addWindowsPathIfNeeded(String path) {
        return unixPathToWindows(path);
    }

    /* The postgres DB takes backslashes as excape characters.  The directory
     * paths on Windows use backslashes.  For these to work in the DB, we
     * need to use \\ so the backslashes are kept as part of the string.
     * Unix paths are uneffected because they will not have any backslashes.
     */
    static public String escapeBackSlashes(String path) {
        if (path == null)
            return path;
        
        String newPath;
        path = path.trim();

        // Use File.separator instead of backslash.  Jave catches the
        // backslashes and as escape char.
        if(OSNAME.startsWith("Windows")) {
            newPath = path.replaceAll("\\\\",
                                      "\\\\\\\\");
            return newPath;
        }
        else
            return path;
    }

    /* If path starts with the windows type path, remove it leaving
     * the unix type path.
     **/
    static public String removeWindowsPathIfNeeded(String path) {
        if (path == null)
            return path;
        
        String newPath;
        path = path.trim();

        if(!OSNAME.startsWith("Windows") && path.startsWith(SFUDIR_WINDOWS)) {
            newPath = path.substring(SFUDIR_WINDOWS.length());
            return newPath;
        }
        else
            return path;

    }

    // Convert unix style path to Windows style path.  If not in Window,
    // or if path is already Windows, then no change is made.  
    public static String unixPathToWindows(String strPath)
    {
        if (strPath == null || strPath.length() == 0 ||
            OSNAME == null || !OSNAME.startsWith("Windows"))
            return strPath;
        
        StringBuffer sbPath = new StringBuffer(strPath.trim());
        String strunix = "/dev/fs/";
        int nIndex = sbPath.indexOf(strunix);
        int index = sbPath.indexOf("\\dev\\fs");
        if(strPath.indexOf(":") != 1)
        {
            // /vnmr might be symbolic link e.g. /vnmr
            if (strPath.equals("/vnmr") || strPath.startsWith("/vnmr/"))
                sbPath.replace(0, 5, SYSDIR);
            
            if (nIndex >= 0 || index >= 0)
            {
                // delete unix string
                sbPath.delete(0, strunix.length());
                
                // insert ':\' e.g. C:\ or ':' e.g C:
                if (sbPath.length() == 1)
                   //sbPath.insert(1, ":\\");
                sbPath.insert(1, ":/");
                else if (sbPath.charAt(1) != ':') 
                    sbPath.insert(1, ':');
            }
            else
            {
                // If it starts with /SFU, remove it
                if (sbPath.charAt(0) == '/')
                    sbPath.insert(0, SFUDIR_WINDOWS);
            }
        }
        // replace symbolic link with full path
        else if (strPath.indexOf("/SFU/vnmr") == 2)
            sbPath.replace(0, 11, SYSDIR);
        
        //replace '\' with '/'
        while ((nIndex = sbPath.indexOf("\\")) >= 0)
        {
            sbPath.replace(nIndex, nIndex+1, "/");
        }
        while ((nIndex = sbPath.indexOf("//")) >= 0)
        {
            sbPath.replace(nIndex, nIndex+2, "/");
        }

        return sbPath.toString();
    }
    
    /**
     *  Returns path in unix format 
     *  e.g. /dev/fs/C/SFU/vnmrj
     *  @strPath    path in windows format
     *  @return     path in unix format
     */
    public static String windowsPathToUnix(String strPath)
    {
        return windowsPathToUnix(strPath, false);
    }

    /**
     *  Returns path in unix format
     *  e.g. /dev/fs/C/SFU/vnmrj
     *  @strPath    path in windows format
     *  @bUseSymbolicLink   if the unix path should use /vnmr as the symbolic link
     *                      e.g. return path in format /vnmr instead of full path
     *  @return     path in unix format
     */
    public static String windowsPathToUnix(String strPath, boolean bUseSymbolicLink)
    {
        if (strPath == null || strPath.length() == 0 ||
            OSNAME == null || !OSNAME.startsWith("Windows"))
            return strPath;
        
        StringBuffer sbPath = new StringBuffer(strPath.trim());
        if (bUseSymbolicLink)
        {
            String strSymLink = "/vnmr";
            int nIndex = sbPath.indexOf(SYSDIR);
            if (nIndex >= 0)
                sbPath.replace(nIndex, SYSDIR.length(), strSymLink);
        }
        String strwindows = ":";
        int nIndex = sbPath.indexOf(strwindows);
        // delete ":"
        if (nIndex >= 0)
            sbPath.deleteCharAt(nIndex);

        // replace '\' with '/'
        while ((nIndex = sbPath.indexOf(File.separator)) >= 0)
        {
            sbPath.replace(nIndex, nIndex+1, "/");
        }
        if(sbPath.indexOf("/") != 0)
            // insert /dev/fs if it does not start with '/'
            sbPath.insert(0, "/dev/fs/");

        return sbPath.toString();
    }
    
    /*
     *  The forward slashes are set for all the paths by reading the strPath.
     *  e.g. /vnmr/adm/users/profiles/user has the format C:\SFU\vnmr, this will
     *  be replaced by C:\SFU\vnmr. 
     */
    public static String setForwardSlashes(String strPath)
    {
        StringBuffer sbData = new StringBuffer();
        if (strPath == null || strPath.equals(""))
            return strPath;
        
        try
        {
            BufferedReader reader = new BufferedReader(new FileReader(strPath));
            String strLine;
            while ((strLine = reader.readLine()) != null)
            {
                sbData.append(strLine.trim()).append("\n");
            }
            reader.close();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        
        String strData = sbData.toString().replace('\\', '/');
        return strData;
    }

    public static boolean iswindows()
    {
        return (OSNAME != null && OSNAME.startsWith("Windows"));
    }

    public static String runScript(String[] cmd, boolean bPostMsg)
    {
        String strg = "";
        boolean bOk = true;
        String strMsg = "";

        if (cmd == null)
            return null;
        Process prcs = null;
        try
        {
            Runtime rt = Runtime.getRuntime();

            prcs = rt.exec(cmd);

            if (prcs == null)
                return null;

            InputStream istrm = prcs.getInputStream();
            if (istrm == null)
                return null;

            BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

            while((strg = bfr.readLine()) != null)
            {
                strg = strg.trim();
                String strg2 = strg.toLowerCase();
                bOk = (strg2.equals("-1") || strg2.indexOf("invalid") >= 0) ? false
                            : true;

                strMsg = strg;
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            bOk = false;
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


        return strMsg;
    }
} // UtilB
