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
import java.awt.*;
import java.util.*;

import javax.swing.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.*;
// import vnmr.admin.*;
import vnmr.admin.vobj.*;
import vnmr.admin.ui.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2001
 * Company:
 * @author
 * @version 1.0
 */

public class WFileUtil
{

    /** The name of the item. */
    protected static String m_strItemName;

    /** The directory path that contains the info for the item. */
    protected static String m_strInfoDir;

    /** The file cache object.  */
    protected static WFileCache m_objCache;


    public WFileUtil()
    {
        m_objCache = new WFileCache(100);
    }

    /**
     * Opens the file for reading.
     * @param strPath   the path where the file is located.
     * @return BufferedReader the BufferedReader that would read the file.
     */
    public static BufferedReader openReadFile(String strPath)
    {
        BufferedReader in = null;
        try
        {
//            if (Util.iswindows())
//                strPath = UtilB.unixPathToWindows(strPath);
            if (strPath != null) {
                UNFile file = new UNFile(strPath);
                if(file.exists())
                    in = new BufferedReader(new FileReader(file));
                else
                    Messages.postDebug("openReadFile: File not found " + strPath);
            }
        }
        catch (FileNotFoundException e)
        {
            Messages.postDebug("openReadFile: File not found " + strPath);
        }
        return in;
    }

    /**
     *  Opens the file for writing.
     *  @param strFile  the path where the file is located.
     *  @return BufferedWriter  the Writer that would write to the file.
     */
    public static BufferedWriter openWriteFile(String strFile)
    {
        BufferedWriter out = null;
        try
        {
//            if (Util.iswindows())
//                strFile = UtilB.unixPathToWindows(strFile);
            if (strFile != null) {
                UNFile file = new UNFile(strFile);
//                if(file.exists())
                    out = new BufferedWriter(new FileWriter(strFile));
//                else
//                    Messages.postDebug("openWriteFile: File not found " + strFile);
            }
        }
        catch (IOException e)
        {
            Messages.postDebug("openWriteFile: File not found " + strFile);
            //System.out.println("File not found " + strFile);
        }
        return out;
    }

    /**
     *  Opens the file for writing.
     *  @param strFile  the path where the file is located.
     *  @return BufferedWriter  the Writer that would write to the file.
     */
    public static BufferedWriter openWriteFile(File objFile)
    {
        BufferedWriter out = null;
        try
        {
            if (objFile != null)
                out = new BufferedWriter(new FileWriter(objFile));
        }
        catch (IOException e)
        {
            Messages.postDebug("File not found " + objFile.toString());
            //System.out.println("File not found " + objFile.toString());
        }
        return out;
    }


    /**
     *  Opens and returns the interface path given a filename.
     *  @param strFileName  Name of the file
     */
    public static String openInterfacePath(String strFileName)
    {
        String strPath =  FileUtil.openPath("INTERFACE" + File.separator + strFileName);
        return strPath;
    }

    /**
     *  Fills values in the given panel Area.
     *  @param objVal   either the button that was clicked or the path of the file to be read.
     *  @param pnlArea  The panel area whose component values need to be filled.
     */
    public static void fillValues(Object objVal, JPanel pnlArea, String strPropName)
    {
        String strPath = null;
        if (objVal instanceof String)
        {
            String strDir = (String)objVal;
            if (strDir != null && !strDir.equals(""))
            {
                strPath = getItemPath(strDir);
            }
            HashMap hmItem = getHashMap(strPath);
            fillValues(hmItem, pnlArea, false);
            inputName(strPath, hmItem, pnlArea);
        }
        else if (objVal instanceof HashMap)
        {
            // Defaults for the New user
            fillValues((HashMap)objVal, pnlArea, true);
        }
        boolean bUpdate = (strPropName.indexOf("update") >= 0) ? true : false;

        makeFieldsEditable(pnlArea, strPath, strPropName, objVal, bUpdate);
    }

    /**
     *  Fills values of the components in the given panel area.
     *  @param strPath  the path of the file to be read that has the values.
     *  @param pnlArea  the panel whose components' values need to be filled.
     */
    public static void fillValues(HashMap hmItem, JPanel pnlArea, boolean bSetName)
    {
        int nCompCount = pnlArea.getComponentCount();
        String strValue = null;
        WObjIF obj = null;

        try
        {
            //HashMap hmItem = getHashMap(strPath);

            // For each component in the panel, it gets the key attribute,
            // and then gets the value of the key from the file,
            // and sets the value of the component.
            for (int i = 0; i < nCompCount; i++)
            {
                Component comp = pnlArea.getComponent(i);
                if (comp instanceof WObjIF)
                {
                    obj = (WObjIF)comp;
                    strValue = getValue(obj, hmItem, bSetName);
                    obj.setValue(strValue);
                }
            }
        }
        catch(Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }
    }


    /**
     *  Sets the hashmap for each item in the arraylist.
     *  @param aListNames   the list of the item names.
     *  @param strDirPath   the directory path that has the info files for each item in the list.
     */
    public static void setItemHM(ArrayList aListNames, String strDirPath)
    {
        String strFile = null;
        String strName;
        int nLength = aListNames.size();

        for (int i = 0; i < nLength; i++)
        {
            strName = (String)aListNames.get(i);
            strFile = strDirPath + strName;
            //System.out.println("The file is " + strFile);

            try
            {
                // Get the hashmap for this item,
                // getHashMap() looks up the hash map for the item in the file,
                // if it's there it gets it, otherwise creates a new hashmap,
                // and sets it for the given item.
                HashMap hmTmp = getHashMap(strFile);
            }
            catch(Exception e)
            {
                Messages.writeStackTrace(e);
                //e.printStackTrace();
                Messages.postDebug(e.toString());
            }
        }
    }

    /**
     *  Writes the file for the given object or path.
     *  @param objVal   the path of the file to be written,
     *                  or the object whose file should be written.
     *  @param pnlArea  the panel whose item file is being written.
     */
    public static boolean writeInfoFile(Object objVal, JPanel pnlArea, String strPropName)
    {
        String strPath = null;
        String strName = null;
        boolean bPathConcat = true;
        boolean bSuccess = false;

        // Todo => strPath.
        if (objVal instanceof String)
            strPath = (String)objVal;
        else if (objVal instanceof WItem)
        {
            WItem objItem = (WItem)objVal;
            strName = objItem.getText();
            m_strInfoDir = objItem.getInfoDir();
            if (m_strInfoDir != null)
                strPath = FileUtil.openPath(m_strInfoDir);
        }

        makeFieldsEditable(pnlArea, strPath, strPropName, objVal, false);
        if (strPath != null)
        {
            bSuccess = writeInfoFile(strPath, pnlArea, objVal, strPropName, strName);
        }
        postMessage(bSuccess, strName);

        return bSuccess;
    }

    /**
     *  Writes the file for the given object.
     *  @param strPath  path of the file to be written.
     *  @param pnlArea  panel that contains the object.
     *  @param objVal   object whose file is being written.
     */
    public static boolean writeInfoFile(String strPath, JPanel pnlArea, Object objVal,
                                            String strPropName, String strItemName)
    {
        int nCompCount = pnlArea.getComponentCount();
        StringBuffer sbPbLine = new StringBuffer();
        StringBuffer sbPrvLine = new StringBuffer();

        String strItemPath = FileUtil.openPath("SYSPROF"+File.separator+strItemName) + File.pathSeparator +
                                FileUtil.openPath("USRPROF"+File.separator+strItemName);
        HashMap hmUser = WFileUtil.getHashMap(strItemPath);

        Object[] aObjBW = getInfoWriter(strPath, strItemName, strPropName, pnlArea);
        BufferedWriter outPublic = (BufferedWriter)aObjBW[0];
        BufferedWriter outPrivate = (BufferedWriter)aObjBW[1];

        if (outPublic == null)
            return false;

        String strName = changeBtnName(objVal);
        HashMap hmDefaults = getUserDefaults();
        Value objValue = null;
        // write the file in the following format:
        // key   value
        try
        {
            for (int i = 0; i < nCompCount; i++)
            {
                objValue = getKeyValLine(i, pnlArea, strName, hmDefaults, hmUser);
                if (objValue.bPublic || (strPropName.indexOf(WGlobal.SAVEUSER) < 0))
                    sbPbLine.append(objValue.value);
                else
                    sbPrvLine.append(objValue.value);
            }
            writeAndClose(outPublic, sbPbLine);
            writeAndClose(outPrivate, sbPrvLine);
            hmUser = getHashMapUser(strName);
            WUserUtil.writeOperatorFile(strName, hmUser);
        }
        catch (Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug("Error writing file " + strPath + " " + e.toString());
            return false;
        }
//        WUserUtil.setAppMode(strName);
        
        return true;
    }

    protected static Object[] getInfoWriter(String strPath, String strItemName,
                                            String strProp, JPanel pnlArea)
    {
        String strPbPath = FileUtil.openPath(strPath+strItemName);
        String strPrvPath = null;
        BufferedWriter outPublic = openWriteFile(strPbPath);
        BufferedWriter outPrivate = null;
        File objFile = null;

        // if it's the user panel, then get the two user and the system path,
        // and get the writers for both paths.
        if (strProp.indexOf(WGlobal.SAVEUSER) >= 0)
        {
            strPbPath = FileUtil.openPath(strPath+"system"+File.separator+strItemName);
            strPrvPath = FileUtil.openPath(strPath+"user"+File.separator+strItemName);

            outPublic = openWriteFile(strPbPath);
            outPrivate = openWriteFile(strPrvPath);

            if (outPublic == null || outPrivate == null)
            {
                if (outPublic == null )
                {
                    objFile = getNewFile(pnlArea, "system");
                    outPublic = openWriteFile(objFile);
                    strPbPath = objFile.getAbsolutePath();
                }
                if (outPrivate == null )
                {
                    objFile = getNewFile(pnlArea, "user");
                    outPrivate = openWriteFile(objFile);
                    strPrvPath = objFile.getAbsolutePath();
                }
            }
            else
                m_strItemName = null;
        }
        // else just get the writer for the path that's given.
        else
        {
            if (outPublic == null)
            {
                objFile = getNewFile(pnlArea, "");
                outPublic = openWriteFile(objFile);
                strPbPath = objFile.getAbsolutePath();
            }
        }
        Object[] aObjBW = {outPublic, outPrivate};

        return aObjBW;
    }

    /**
     *  Open the file for writing and update the file.
     *  @param strPath  the path of the file to be updated.
     *  @param pnlArea  the panel belonging to the item.
     *  @param hmItem   the hashmap of the given item.
     */
    public static void updateItemFile(String strPath, HashMap hmItem)
    {
        if (hmItem == null || hmItem.isEmpty())
            return;

        String strPbPath = strPath;
        String strPrvPath = null;
        int nIndex = strPath.indexOf(File.pathSeparator);
        // if there is more than one path, then get the system and the user path.
        if (nIndex >= 0)
        {
            strPbPath = strPath.substring(0, nIndex);
            strPrvPath = strPath.substring(nIndex+1);
        }

        BufferedWriter outPb = openWriteFile(strPbPath);
        BufferedWriter outPrv = openWriteFile(strPrvPath);

        if (outPb == null)
            return;

        updateItemFile(outPb, outPrv, hmItem);
    }

    /**
     *  Read the hashmap, get the values for each key, and update the file
     *  in the system.
     *  @param strPath  the path of the file to be updated.
     *  @param pnlArea  the panel belonging to the item.
     *  @param hmItem   the hashmap of the given item.
     */
    public static void updateItemFile(BufferedWriter outPb, BufferedWriter outPrv,
                                        HashMap hmItem)
    {
        WObjIF objItem = null;
        String strKey = null;
        String strValue = null;
        String strLine = null;

        HashMap hmDef = getUserDefaults();
        StringBuffer sbPbLine = new StringBuffer();
        StringBuffer sbPrvLine = new StringBuffer();
        Iterator keySetItr = hmItem.keySet().iterator();


        try
        {
            while(keySetItr.hasNext())
            {
                strKey = (String)keySetItr.next();
                strValue = (String)hmItem.get(strKey);
                if (strValue == null)
                    strValue = "";
                strLine = strKey + '\t' + strValue + '\n';

                if (WUserDefData.isKeyPublic(strKey, hmDef))
                    sbPbLine.append(strLine);
                else
                    sbPrvLine.append(strLine);
            }
            writeAndClose(outPb, sbPbLine);
            writeAndClose(outPrv, sbPrvLine);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug("Error writing file ");
        }


    }

    /**
     *  Writes the persistence file.
     *  @param strPerFile   the persistence file.
     *  @param strKey       the key whose value needs to be updated.
     *  @param strValue     the new value for the key.
     */
    public static void writePersistence(String strPerFile, String strKey, String strValue)
    {
        String strPath = FileUtil.openPath(strPerFile);
        if(strPath==null)
            strPath = FileUtil.savePath(strPerFile);
        String strLine;

        BufferedReader in = openReadFile(strPath);

        try
        {
            StringBuffer sbLine = new StringBuffer();
            sbLine.append(strKey).append(" ").append(strValue).append("\n");

            while((strLine = in.readLine()) != null)
            {
                StringTokenizer strTok = new StringTokenizer(strLine);
                if (strTok.hasMoreTokens())
                {
                    if (!strTok.nextToken().equals(strKey))
                    {
                        sbLine.append(strLine).append("\n");
                    }
                }
            }
            in.close();
            BufferedWriter out = openWriteFile(strPath);
            WFileUtil.writeAndClose(out, sbLine);
        }
        catch(IOException er)
        {
            /*System.out.println("DisplayOptions error: could not create persistence file");
            er.printStackTrace();*/
            Messages.writeStackTrace(er);
            Messages.postDebug("DisplayOptions error: could not create persistence file");
        }
    }

    /**
     *  Gets the value for the given key from the persistence file.
     *  @param strKey       key whose value is returned.
     *  @param strPerFile   the persistence file.
     */
    public static String getPersisValue(String strKey, String strPerFile)
    {
        BufferedReader reader = openReadFile(FileUtil.openPath(strPerFile));
        String strValue = null;
        String strLine;

        if (reader == null)
            return strValue;

        try
        {
            while((strLine = reader.readLine()) != null)
            {
                StringTokenizer strTok = new StringTokenizer(strLine);
                if (strTok.hasMoreTokens())
                {
                    String strFileKey = strTok.nextToken();
                    if (strFileKey.equals(strKey) && strTok.hasMoreTokens())
                    {
                        strValue = strTok.nextToken();
                        break;
                    }
                }
            }
            reader.close();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
        return strValue;
    }

    /**
     *  Gets the value in the following form:
     *   key value
     *   and if the key is public.
     */
    protected static Value getKeyValLine(int nComp, JPanel pnlArea, String strName,
                                            HashMap hmDefaults, HashMap hmUser)
    {
        String strValue = null;
        String strLine =  null;
        boolean bDetailArea1 = isDetailArea1(pnlArea);

        WObjIF obj = (WObjIF)pnlArea.getComponent(nComp);
        String strKey = obj.getAttribute(VObjDef.KEYWORD);

        boolean bPublic = WUserDefData.isKeyPublic(strKey, hmDefaults);

        if (strKey != null && strKey.length() > 0)
        {
            strValue = obj.getValue();
            String strOldValue = (String)hmUser.get(strKey.trim());
            if (strValue != null && strValue.indexOf('$') >= 0)
                strValue = parseValue(strValue, pnlArea);

            strValue = (strValue != null) ? strValue.trim() : "";
            strOldValue = (strOldValue != null) ? strOldValue.trim() : "";

            if (strKey.equalsIgnoreCase("itype"))
                updateIType(strValue, strName);
            else if (strKey.equalsIgnoreCase("operators") && !strOldValue.equals(strValue))
            {
                ArrayList aListOperators = WUtil.strToAList(strValue, false, " ,\t\n");
                ArrayList aListOperators2 = WUserUtil.getOperatorList();
                int i = 0;
                while (i < aListOperators.size())
                {
                    String strOperator = (String)aListOperators.get(i);
                    if (!WUserUtil.isOperatorNameok(strOperator, true))
                        aListOperators.remove(i);
                    else if (Util.isPart11Sys() && !aListOperators2.contains(strOperator))
                    {
                        aListOperators.remove(i);
                        Messages.postError("Operator " + strOperator + " not found.\n" +
                                           "Please create " + strOperator +
                                           " in Modify Operators dialog and try again");
                    }
                    else
                        i = i + 1;
                }
                strValue = WUtil.aListToStr(aListOperators);
                hmUser.put("operators", strValue);
            }
            /*if (bDetailArea1 &&
                strKey.equalsIgnoreCase("grouplist"))
                updateList(strValue, strName);*/

            if (!strOldValue.equals(strValue))
            {
                String strMsg = "Modified " + strKey + ". (Was '" + strOldValue
                                    + "' Now '" + strValue + "').";
                WUserUtil.writeAuditTrail(new Date(), strName, strMsg);
            }
            strLine = strKey + '\t' + strValue + '\n';
        }
        return new Value(bPublic, strLine);
    }

    protected static boolean isDetailArea1(JPanel pnlArea)
    {
        boolean bArea1 = false;
        if (pnlArea instanceof VDetailArea)
        {
            String strArea = ((VDetailArea)pnlArea).getName();
            if (strArea.equals(WGlobal.AREA1))
                bArea1 = true;
        }
        return bArea1;
    }

    public static void writeAndClose(BufferedWriter writer, StringBuffer sbData)
    {
        try
        {
            if (writer != null)
            {
                if (sbData != null)
                    writer.write(sbData.toString());
                writer.flush();
                writer.close();
            }
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
    }

    protected static void inputName(String strDir, HashMap hmItem, JPanel pnlArea)
    {
        if (hmItem == null)
            return;

        String strUser = (String)hmItem.get(WGlobal.NAME);
        if (strDir == null || strDir.indexOf(":") < 0 ||
            (strUser != null && !strUser.trim().equals("")))
            return;

        // if the loginname is empty or null (old users), then get the name from
        // the filename, and set that as the loginname, this will not write it
        // to the user file, when save is clicked then it would write to the user file.
        int nIndex = strDir.lastIndexOf("/");
        if (nIndex < 0)
            nIndex = strDir.lastIndexOf(File.separator);
        if (nIndex >= 0 && nIndex+1 < strDir.length())
        {
            strUser = strDir.substring(nIndex+1);
            hmItem.put(WGlobal.NAME, strUser);
            if (pnlArea instanceof VDetailArea)
            {
                Component comp  = ((VDetailArea)pnlArea).getPnlComp(pnlArea,
                                                                    WGlobal.NAME, true);
                if (comp instanceof WObjIF)
                    ((WObjIF)comp).setValue(strUser);
            }
        }
    }

    /**
     *  Gets the value of the key given the file path and the key.
     *  @param objItem  the object whose value should be set.
     *  @param hmItem   the hashmap that contains the key value pair for the item.
     */
    protected static String getValue(WObjIF objItem, HashMap hmItem, boolean bSetName)
    {
        String strValue = null;

        // For the objItem in the panel, it gets the key attribute,
        // and then gets the value of the key from the file,
        // and sets the value of the component.
        if (hmItem != null && !hmItem.isEmpty())
        {
            String strKey = objItem.getAttribute(VObjDef.KEYWORD);
            if (strKey != null && strKey.length() > 0)
            {
                strValue = lookUpValue(hmItem, strKey);

                if (strValue != null && strValue.indexOf('$') >= 0)
                    strValue = parseValue(strValue, hmItem);
                if (strValue != null)
                    strValue = strValue.trim();

                if (bSetName && strKey.equalsIgnoreCase(WGlobal.NAME))
                {
                    strValue = objItem.getValue();
                    hmItem.put(strKey, strValue);
                }
            }
        }

        return strValue;
    }

    public static String parseValue(String strKey, Object objSource)
    {
        if (strKey == null || objSource == null)
            return strKey;

        String strValue = "";
        char cToken = ' ';
        if (Util.iswindows())
        {
            cToken = ';';
            strKey = UtilB.unixPathToWindows(strKey);
        }
        StringBuffer sbKey = new StringBuffer(strKey);

        if (sbKey != null && sbKey.length() > 0)
        {
            int nBegIndex = strKey.indexOf('$');
            while (nBegIndex >= 0)
            {
                int nTmpIndex1 = strKey.indexOf(File.separatorChar, nBegIndex);
                int nTmpIndex2 = strKey.indexOf(cToken, nBegIndex);
                int nEndIndex = nTmpIndex1;

                if ((nTmpIndex1 < nTmpIndex2 && nTmpIndex1 >= 0)
                        || nTmpIndex2 < 0)
                    nEndIndex = nTmpIndex1;
                else
                    nEndIndex = nTmpIndex2;


                if (nEndIndex <= 0)
                    nEndIndex = strKey.length();

                if (nEndIndex >= 0 && nEndIndex <= sbKey.length())
                {
                    String strNewKey = sbKey.substring(nBegIndex+1, nEndIndex);

                    if (objSource instanceof HashMap)
                        strValue = lookUpValue((HashMap)objSource, strNewKey.trim());
                    else if (objSource instanceof VDetailArea)
                        strValue = ((VDetailArea)objSource).getItemValue(strNewKey.trim());

                    // if the value is not in the user defaults, then try to get
                    // the value from system property
                    if(strValue == null || strNewKey.equals("sysdir"))
                    {
                        strValue = System.getProperty(strNewKey);
                    }

                    // if the value is not in the user defaults, and not in the
                    // system property, then try to get it from unix.
                    // if it's vnmruser, then don't get it from unix, since
                    // this would be different for all users, and when
                    // wanda is echoing it then it would give it's specific information.
                    /*if (strValue == null && !(strNewKey.equals("vnmruser")) &&
                            !strNewKey.equalsIgnoreCase("user") && !strNewKey.equalsIgnoreCase(home))
                    {
                        String[] cmd = {WGlobal.SHTOOLCMD, "-c", " echo $" + strNewKey};
                        strValue = WUtil.runScript(cmd).getMsg();
                        if (strValue == null || strValue.trim().length() == 0)
                            strValue = "\\$" +strNewKey;
                    }*/

                    if (strValue == null)
                        strValue = "";

                    sbKey.replace(nBegIndex, nEndIndex, strValue.trim());
                    strKey = sbKey.toString();
                    nBegIndex = strKey.indexOf('$');

                    // check for escape characters,
                    // e.g. if the value for a variable was not found,
                    // then we might want to leave the variable name,
                    // for this an escape character is used before '$'
                    /*int nEscIndex = nBegIndex - 1;
                    if (nEscIndex >= 0 && strKey.charAt(nEscIndex) == '\\')
                    {
                        nEscIndex = nBegIndex+1;
                        if (nEscIndex >= 0 && nEscIndex < strKey.length())
                            nBegIndex = strKey.indexOf('$', nBegIndex+1);
                    }*/
                }
            }

        }
        return sbKey.toString();
    }

    public static WFileCache getObjCache()
    {
        if (m_objCache == null)
            m_objCache = new WFileCache(100);
        return m_objCache;
    }

    public static String getHMPath(String strUser)
    {
        if (strUser == null)
            return null;

        String strPath = FileUtil.openPath("SYSPROF"+File.separator+strUser) + File.pathSeparator +
                                FileUtil.openPath("USRPROF"+File.separator+strUser);
        return strPath;
    }

    public static HashMap getHashMapUser(String strUser)
    {
        String strPath = getHMPath(strUser);
        return getHashMap(strPath);
    }

    public static HashMap getHashMap(String strPath)
    {
        return getHashMap(strPath, true);
    }

    /**
     *  Returns the hashMap for the given file from the cache.
     *  @param strPath  the file path of the item.
     */
    public static HashMap getHashMap(String strPath, boolean bCreateNew)
    {
        HashMap hmItem = null;
        if (strPath != null)
            hmItem = (HashMap)getObjCache().getData(strPath, bCreateNew);

        return hmItem;
    }

    public static void setHashMap(String strKey, HashMap hmItem)
    {
        if (strKey != null && hmItem != null)
        {
            getObjCache().setData(strKey, hmItem);
        }
    }

    public static boolean isReqField(String strKey, String[] aObjReq)
    {
        String strReq = null;
        boolean bReq = false;
        int nSize = aObjReq.length;

        // for each item, check if it is a required field,
        for(int j = 0; j < nSize ; j++)
        {
            strReq = (String)aObjReq[j];
            if (strReq.equalsIgnoreCase(strKey))
            {
                bReq = true;
            }
        }
        return bReq;
    }

    public static String parseLabel(String strPropName)
    {
        String strLabel = null;
        if (strPropName != null && strPropName.length() > 0)
        {
            int nEndIndex = strPropName.indexOf(WGlobal.SEPERATOR);
            if (nEndIndex >= 0 && nEndIndex <= strPropName.length())
                strLabel = strPropName.substring(0, nEndIndex);
        }

        return strLabel;
    }

    public static String parseDir(String strDirPath, String strName)
    {
        String strDir = strDirPath;
        int nIndex = strDirPath.lastIndexOf(File.separator);
        if (nIndex < 0)
            nIndex = strDirPath.lastIndexOf("/");
        if (nIndex >= 0)
        {
            String strParsedName = strDirPath.substring(nIndex+1);
            if (strParsedName != null && strParsedName.equals(strName))
                strDir = strDirPath.substring(0, nIndex);
        }
        return strDir;
    }

    /**
     *  Parses and returns the item name given the path of the file.
     *  @param strPath  the path of the file.
     */
    public static String parseItemName(String strPath)
    {
        String strName = null;
        int nIndex = strPath.lastIndexOf('/');
        if (nIndex < 0)
            nIndex = strPath.lastIndexOf(File.separator);
        if (nIndex > 0 && nIndex+1 < strPath.length())
            strName = strPath.substring(nIndex+1);
        else
            strName = strPath;

        return strName;
    }

    protected static boolean scriptExists(String strScript)
    {
        boolean bExists = false;

        if (strScript != null && strScript.length() > 0)
        {
            if (Util.iswindows())
                strScript = UtilB.addWindowsPathIfNeeded(strScript);

            File flScript = new File(strScript);
            bExists = flScript.exists();

            if (!bExists)
            {
                Messages.postError(strScript + " not found.");
            }
        }

        return bExists;
    }

    /**
     *  Sets the hashmap for the given path of the file.
     *  @param strPath  the path of the file.
     *  @param objMap   the hashmap that needs to be set.
     */
    private static void setHM(String strPath, HashMap objMap)
    {
        String strPublicPath = strPath;
        String strPrivatePath = null;

        int nIndex = strPath.lastIndexOf(File.pathSeparator);
        if (nIndex >= 0)
        {
            strPublicPath = strPath.substring(0, nIndex);
            if (strPublicPath.length() + 1 > 0)
                strPrivatePath  = strPath.substring(strPublicPath.length()+1);
        }

        BufferedReader readerPb = openReadFile(strPublicPath);
        BufferedReader readerPrv = openReadFile(strPrivatePath);

        try
        {
            //setHM(in, objMap);
            if (readerPb != null)
            {
                setHM(readerPb, objMap);
                readerPb.close();
            }
            if (readerPrv != null)
            {
                setHM(readerPrv, objMap);
                readerPrv.close();
            }
        }
        catch(Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }
    }

    /**
     *  Sets a new hashmap for the given item.
     *  It reads each key and value pair from the file,
     *  and puts it in the hashmap.
     *  @param in       the input to be read.
     *  @param hmItem   the hashmap that needs to be set.
     */
    private static HashMap setHM(BufferedReader in, HashMap hmItem)
    {
        String strLine;
        String strValue = null;
        String strKey   = null;
        try
        {
            while((strLine = in.readLine()) != null)
            {
                int nIndex = strLine.trim().indexOf('\t', 0);
                if (nIndex <= 0)
                    nIndex = strLine.trim().indexOf(' ', 0);
                if (nIndex < 0)
                    continue;
                strKey = strLine.substring(0, nIndex);
                if (strKey != null)
                    strKey = strKey.trim();
                strValue = strLine.substring(nIndex);
                if (strValue != null)
                    strValue = strValue.trim();
                hmItem.put(strKey, strValue);
                //System.out.println("In detail area key " + strKey + " value " + strValue);
            }
        }
        catch (Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }
        return hmItem;
    }

    /**
     *  Returns a new file writer by creating a new file for the new name entered.
     */
    protected static File getNewFile(JPanel pnlArea, String strSubDir)
    {
        String strKey = null;
        WObjIF obj = null;
        int nCompCount = pnlArea.getComponentCount();
        UNFile objFile = null;

        try
        {
            // get the current name entered in the component,
            // and then create a new file with this name in the infodir.
            for (int i = 0; i < nCompCount; i++)
            {
                obj = (WObjIF)pnlArea.getComponent(i);
                strKey = obj.getAttribute(VObjDef.KEYWORD);
                if (strKey != null && strKey.equals(WGlobal.NAME))
                {
                    m_strItemName = obj.getValue();
                    String strPath = FileUtil.openPath(m_strInfoDir+strSubDir);
                    objFile = new UNFile(strPath, m_strItemName);
                    if (!objFile.exists())
                        objFile.createNewFile();
                    break;
                }
            }
        }
        catch(Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }

        return objFile;
    }

    protected static HashMap makeNewHM(HashMap hmUserDef, String strUser, String strItype)
    {
        String strKey = null;
        WUserDefData userData = null;
        String strValue = null;
        HashMap hmUser = new HashMap();

        if (hmUserDef == null)
            return hmUser;

        Iterator keySetItr = hmUserDef.keySet().iterator();

        try
        {
            hmUser.put(WGlobal.NAME, strUser);

            // get the value from the user defaults,
            // and make a hashmap for the user with key and value.
            while(keySetItr.hasNext())
            {
                strKey = (String)keySetItr.next();
                userData = (WUserDefData)hmUserDef.get(strKey);
                if (userData != null)
                    strValue = userData.getValue();


                if (strKey.indexOf("itype") >= 0)
                    strValue = strItype;

                if (strValue != null)
                    hmUser.put(strKey, strValue);
            }
            // get the absolute values
            hmUser = getAbsoluteValues((HashMap)hmUser.clone());
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }

        return hmUser;
    }

    protected static HashMap makeNewHM(JPanel pnlItem, String strUser)
    {
        String strKey = null;
        String strValue = null;
        HashMap hmUser = new HashMap();
        WObjIF objItem = null;

        if (pnlItem == null || !(pnlItem instanceof WObjIF))
            return hmUser;

        int nCompCount = pnlItem.getComponentCount();
        hmUser.put(WGlobal.NAME, strUser);

        for(int i = 0; i < nCompCount; i++)
        {
            // get the value from the fields,
            // and make a hashmap for the user with key and value.
            objItem = (WObjIF)pnlItem.getComponent(i);
            strKey = objItem.getAttribute(VObjDef.KEYWORD);

            if (strKey != null)
                strValue = objItem.getValue();

            if (strValue != null)
                hmUser.put(strKey, strValue);
        }
        return hmUser;
    }

    // Create a HashMap for a user.  Start with the defaults and then modify
    // as per the options supplied.
    protected static HashMap makeNewHM(HashMap hmUserDef, String strUser, 
                                       HashMap userInfo)
    {
        String strKey = null;
        WUserDefData userData = null;
        String strValue = null;
        HashMap<String, String> hmUser = new HashMap<String, String>();

        if (hmUserDef == null)
            return hmUser;

        // The hmUserDef HashMap has WUserDefData objects for values.
        // The hmUser HashMap needs to have Strings for values
        // Thus we have to loop through and create the hmUser HashMap.
        // First just make one that has the defaults.
        Iterator keySetItr = hmUserDef.keySet().iterator();

        try
        {
            hmUser.put(WGlobal.NAME, strUser);

            // get the value from the user defaults,
            // and make a hashmap for the user with key and value.
            while(keySetItr.hasNext())
            {
                strKey = (String)keySetItr.next();
                userData = (WUserDefData)hmUserDef.get(strKey);
                if (userData != null)
                    strValue = userData.getValue();

                if (strValue != null)
                    hmUser.put(strKey, strValue);
            }
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }

        // Now we have a converted copy of the HashMap (hmUser).
        // We will go through the options and modify hmUser based on those.
        // The HashMap, userInfo, has option keywords as keys (see userDefaults)
        // and values are the values for that option.
        keySetItr = userInfo.keySet().iterator();
        while(keySetItr.hasNext())
        {
            strKey = (String)keySetItr.next();
            strValue = (String)userInfo.get(strKey);
            // Overwrite entries in hmUser with any that were specified
            // in userInfo.
            hmUser.put(strKey, strValue);
        }
        // get the absolute values, ie., resolve $home and $userdir items
        hmUser = getAbsoluteValues((HashMap<String, String>)hmUser.clone());

        return hmUser;
    }


    protected static HashMap<String, String> getAbsoluteValues(HashMap hmItem)
    {
        Iterator keySetItr = hmItem.keySet().iterator();
        String strKey = null;
        String strValue = null;

        while(keySetItr.hasNext())
        {
            strKey = (String)keySetItr.next();
            strValue = (String)hmItem.get(strKey);
            if (strValue != null && strValue.indexOf('$') >= 0)
                strValue = parseValue(strValue, hmItem);

            if (strValue != null)
                strValue = strValue.trim();

            hmItem.put(strKey, strValue);
        }
        return hmItem;
    }

    protected static void postMessage(boolean bSuccess, String strName)
    {
        if (bSuccess)
            Messages.postInfo("Info for '" + strName + "' saved. Set Profile(s) as Needed.");
        else
            Messages.postError("Error saving information for '" + strName + "'");
    }

    /**
     *  Changes the name of the given object.
     *  @param objVal the object whose name is being changed.
     */
    protected static String changeBtnName(Object objVal)
    {
        String strName = null;
        if (objVal instanceof JButton)
        {
            if (m_strItemName != null)
                ((JButton)objVal).setText(m_strItemName);
            strName = ((JButton)objVal).getText();
        }
        return strName;
    }

    protected static void makeFieldsEditable(JPanel pnlArea, String strPath, String strPropName,
                                                Object objVal, boolean bUpdate)
    {
        boolean bNameEdit = false;
        boolean bAllEdit = true;

        boolean bNewUser = (objVal instanceof String && ((String)objVal).indexOf("NewUser") >= 0);

        // if it's just update, then one of the fields has been changed,
        // it's not a new user
        if (bUpdate && !bNewUser)
            bNameEdit = false;
        // Make the name entry editable if new user
        else if (objVal instanceof HashMap || bNewUser)
            bNameEdit = true;

        // if a new user was clicked it made the name entry editable,
        // and then an existing user name has been entered,
        // then make all the fields uneditable for the existing user so the info can't be
        // modified.
        if (strPath != null && strPropName.indexOf("error") >= 0)
            bAllEdit = false;

        setFieldsEdit(pnlArea, bAllEdit, bNameEdit);
    }

    protected static void setFieldsEdit(JPanel pnlArea, boolean bEdit,
                                                boolean bNameEdit)
    {
        int nCompCount = pnlArea.getComponentCount();
        Component comp;
        String strEdit = bEdit ? "yes" : "no";
        WObjIF objItem;

        for (int i = 0; i < nCompCount; i++)
        {
            comp = pnlArea.getComponent(i);

            if (comp instanceof WGroup)
                setFieldsEdit((WGroup)comp, bEdit, bNameEdit);
            else if (comp instanceof WObjIF)
            {
                objItem = (WObjIF)comp;
                String strKey = objItem.getAttribute(VObjDef.KEYWORD);
                if (strKey == null) strKey = "";

                // if it's the name entry, keep it editable if 'newuser',
                // else make it uneditable; this is the opposite of the bEdit
                // condition for the other entry fields.
                if (strKey.equalsIgnoreCase(WGlobal.NAME))
                {
                    String strNameEdit = (!bEdit || bNameEdit) ? "yes" : "no";
                    objItem.setAttribute(VObjDef.EDITABLE, strNameEdit);
                    setNameBgColor(bEdit, objItem);
                }
                else
                    objItem.setAttribute(VObjDef.EDITABLE, strEdit);
            }
        }
    }

    protected static void setNameBgColor(boolean bEdit, WObjIF objItem)
    {
        if (!(objItem instanceof WEntry))
            return;

        WEntry objEntry = (WEntry)objItem;
        String strColor = "";
        if (!bEdit)
        {
            strColor = "red";
            Messages.postError("User " + objItem.getValue() + " already exists " );
        }
        else
        {
            Container parent = objEntry.getParent();
            if (parent instanceof WObjIF)
                strColor = ((WObjIF)parent).getAttribute(VObjDef.BGCOLOR);
        }
        objEntry.setAttribute(VObjDef.BGCOLOR, strColor);
    }

    protected static void updateIType(String strValue, String strItemName)
    {
        String[] aStrITypes = {Global.ADMINIF, Global.WALKUPIF,
                                Global.IMGIF};
        ArrayList aListValues = WUtil.strToAList(strValue, "~");
        WMRUCache pnlCache = VItemArea1.getPnlCache();
        int nSize = aStrITypes.length;

        // For each itype, check it is in the value array,
        // if it is then add it to the cache
        // else remove it from the cache.
        for (int i = 0; i < nSize; i++)
        {
            String strIType = aStrITypes[i];
            String strFile = FileUtil.SYS_VNMR + "/tmp/" + strIType;
            ArrayList aListCache = (ArrayList)pnlCache.getData(strFile, false);

            //System.out.println("The arraylist is " + aListCache);
            if (aListCache == null || aListCache.isEmpty())
                continue;

            // if the value array has the itype, then add it to the cache array.
            if (aListValues.contains(strIType))
            {
                if (!aListCache.contains(strItemName))
                    aListCache.add(strItemName);
            }
            else
                aListCache.remove(strItemName);

            // reset the cache array.
            pnlCache.setData(strFile, aListCache);
        }
    }

    protected static HashMap getUserDefaults()
    {
        HashMap hmDef = null;
        if (Util.getAppIF() instanceof VAdminIF)
        {
            hmDef = ((VAdminIF)Util.getAppIF()).getUserDefaults();
        }
        return hmDef;
    }

    protected static String getItemPath(String strDir)
    {
        String strPath = FileUtil.openPath(strDir);

        int nIndex = strDir.indexOf(File.pathSeparator);
        if (nIndex >= 0)
        {
            String strPath1 = FileUtil.openPath(strDir.substring(0, nIndex));
            String strPath2 = FileUtil.openPath(strDir.substring(nIndex+1));
            strPath = strPath1 + File.pathSeparator + strPath2;
        }

        return strPath;
    }

    protected static void updateList(String strValue, String strItemName)
    {
        VItemArea2.firePropertyChng("Write", "all", "");
    }

    private static void writeFile(BufferedReader in, BufferedWriter out, String strKey)
    {
        String strLine = "";
        try
        {
            while((strLine = in.readLine()) != null)
            {
                StringTokenizer strTok = new StringTokenizer(strLine);
                if (strTok.hasMoreTokens())
                {
                    if (!strTok.nextToken().equals(strKey))
                    {
                        out.write(strLine);
                        out.newLine();
                    }
                }
            }
            out.flush();
            out.close();
            in.close();
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
    }

    /**
     *  Looks up and returns the value of the given key from the given hashmap.
     *  @param hmItem   the hashmap used for lookup.
     *  @param strKey   the key needed to be used for lookup.
     */
    public static String lookUpValue(HashMap hmItem, String strKey)
    {
        String strValue = null;
        if (hmItem != null && strKey != null)
        {
            Object objValue = hmItem.get(strKey);
            if (objValue instanceof String)
                strValue = (String)objValue;
            else if (objValue instanceof WUserDefData)
                strValue = ((WUserDefData)objValue).getValue();
        }
        return strValue;
    }

    /**
     *  Deletes the hashmap for the given item.
     *  @param strPath  the path of the file.
     */
    protected static void deleteUserHM(String strPath, String strLabel)
    {
        if (strPath != null)
        {
            getObjCache().remove(strPath);
        }
    }

    protected static class WFileCache extends WMRUCache
    {
        public WFileCache(int nMax)
        {
            super(nMax);
        }

        public WFile readFile(String strPath)
        {
            if (Util.iswindows())
                strPath = UtilB.unixPathToWindows(strPath);
            File objFile = new File(strPath);
            HashMap hmConts = new HashMap();
            setHM(strPath, hmConts);

            return new WFile(objFile.lastModified(), hmConts);
        }

    }

    protected static class Value
    {
        protected boolean bPublic = true;
        protected String value = "";

        public Value(boolean pub, String strVal)
        {
            bPublic = pub;
            value = strVal;
        }

    }

}
