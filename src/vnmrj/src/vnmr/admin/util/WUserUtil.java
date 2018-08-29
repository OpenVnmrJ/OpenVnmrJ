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
import java.util.*;
import java.awt.*;
import javax.swing.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;
import vnmr.ui.shuf.*;
import vnmr.admin.ui.*;
import vnmr.part11.*;
import vnmr.admin.vobj.*;


/**
 * Title:   WUserUtil
 * Description: Methods for the user panel.
 * Copyright:    Copyright (c) 2002
 *
 */

public class WUserUtil
{

    public static final String USRPROFILE   = "SYSTEM"+File.separator+"USRPROF";
    public static final String SYSPROFILE   = "SYSTEM"+File.separator+"SYSPROF";
    public static final String USRDATADIR   = "SYSTEM"+File.separator+"PROFILES"+
                                                File.separator+"data";
    public static final String USRTEMPLATE  = "SYSTEM"+File.separator+"PROFILES"+
                                                File.separator+"templates";
    public static final String USRPART11DIR = "SYSTEM"+File.separator+"PROFILES"+
                                                File.separator+"p11";
    public static final String OPERATORLIST = "SYSTEM"+File.separator+"USRS"+
                                                File.separator+"operators"+
                                                File.separator+"operatorlist";
    public static final String PASSWORD     = "SYSTEM"+File.separator+"USRS"+
                                                File.separator+"operators"+
                                                File.separator+"vjpassword";
    public static final String ADMINLIST    = "SYSTEM"+File.separator+"USRS"+
                                                File.separator+"administrators"+
                                                File.separator+"adminlist";
    public static final long FILELENGTH     = 50000;
    public static final String DELFILE      = "/etc/auDel";


    private WUserUtil()
    {
        // not meant to be used.
    }

    /**
     *  Reads the userlist file, and returns the list of the vj users.
     */
    public static ArrayList readUserListFile()
    {
        String strPath = FileUtil.openPath("SYSTEM/USRS/userlist");
        BufferedReader reader = WFileUtil.openReadFile(strPath);

        ArrayList aListUsers = new ArrayList();
        String strLine = null;

        if (reader == null)
            return aListUsers;

        try
        {
            while((strLine = reader.readLine()) != null)
            {
                String strTmp = strLine.toLowerCase();
                if (!strTmp.startsWith("infodir"))
                    aListUsers = WUtil.strToAList(strLine);
            }
            reader.close();
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug("Error reading file " + strPath + " " + e.toString());
        }

        return aListUsers;
    }

    /**
     *  Add the list of vj users to the userlist file.
     *  @param aListNewUsers  new vj users created that need to be added to the
     *                        userlist file.
     */
    public static void addToUserListFile(ArrayList aListNewUsers)
    {
        String strPath = FileUtil.openPath("SYSTEM/USRS/userlist");
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        StringBuffer sbData = new StringBuffer();

        if (reader == null)
            return;

        // add the users to the list that are already vj users,
        // if there are non-vj users, then don't add them to the list
        for (int i = 0; i < aListNewUsers.size(); i++)
        {
            String strName = (String)aListNewUsers.get(i);
            if (strName == null || !isVjUser(strName))
                aListNewUsers.remove(strName);
        }

        String strLine = null;
        String strNewUsers = WUtil.aListToStr(aListNewUsers);

        try
        {
            while((strLine = reader.readLine()) != null)
            {
                String strTmp = strLine.toLowerCase();
                if (!strTmp.startsWith("infodir"))
                {
                    // add the new users to the userlist.
                    strLine = new StringBuffer().append(strLine).append(" ").append(
                                strNewUsers).toString();

                    // Check that there are no duplicate names in the userlist.
                    ArrayList aListNew = WUtil.strToAList(strLine, false);

                    // convert the array to string.
                    strLine = WUtil.aListToStr(aListNew);
                }
                sbData.append(strLine);
                sbData.append("\n");
            }
            reader.close();
            BufferedWriter writer = WFileUtil.openWriteFile(strPath);
            WFileUtil.writeAndClose(writer, sbData);
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug("Error writing file " + strPath + " " + e.toString());
        }
    }

    /**
     *  If the current panel displayed is the user panel, then add these new users
     *  to the panel, and show all users.
     */
    public static void showAllUsers(ArrayList aListNames)
    {
        AppIF appIf = Util.getAppIF();
        if (appIf instanceof VAdminIF)
        {
            // Get the current menu item, and if the panel displayed is the user panel,
            // then add the list of new users to its list.
            String strLabel = ((VAdminIF)appIf).getCurrInfo().getSelMenuLabel();
            strLabel = (strLabel != null) ? strLabel.toLowerCase() : "";
            if (strLabel.indexOf("user") >= 0)
            {
                VItemAreaIF objArea = ((VAdminIF)appIf).getItemArea1();
                for (int i = 0; i < aListNames.size(); i++)
                {
                    String strName = (String)aListNames.get(i);
                    objArea.addItemToList(strName);
                }
                VUserToolBar.showAllUsers();
            }
        }
    }

    public static ArrayList getUnixUsers()
    {
        String strPath = FileUtil.sysdir() + "/bin/jvnmruser";
        if (Util.iswindows())
            strPath = UtilB.windowsPathToUnix(strPath);

        // Run the jvnmruser script that gets all the vnmr users including vj users
        String[] cmd = {WGlobal.SHTOOLCMD, "-c", strPath};
        ArrayList aListUsers = getUsers(cmd);

        return aListUsers;
    }

    public static ArrayList getDisabledAcc()
    {
        ArrayList aListLock = getAccounts("LK");

        return aListLock;
    }

    public static ArrayList getPswdAcc()
    {
        ArrayList aListPswd = getAccounts("PS");

        return aListPswd;
    }

    public static ArrayList getNPswdAcc()
    {
        ArrayList aListNPswd = getAccounts("NP");

        return aListNPswd;
    }


    protected static ArrayList getUsers(String[] cmd)
    {
        ArrayList aListUsers = new ArrayList();
        Messages.postDebug("Running script: " + cmd[2]);
        Runtime rt = Runtime.getRuntime();
        String strg = null;
        Process prcs = null;

        try
        {
            prcs = rt.exec(cmd);

            if (prcs == null)
                return null;

            InputStream istrm = prcs.getInputStream();
            if (istrm == null)
                return null;

            BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

            while((strg = bfr.readLine()) != null)
            {
                //System.out.println(strg);
                if (strg.trim().length() > 0 && strg.indexOf(":") < 0)
                    aListUsers.add(strg.trim());
            }
            bfr.close();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
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

        return aListUsers;
    }

    protected static ArrayList getAccounts(String strType)
    {
        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO +
                         WGlobal.SBIN + "aupw -s all"};
        ArrayList aListUsers = getUsers(cmd);
        ArrayList aListAccType = new ArrayList();
        String strLine;
        String strUser;
        String strStatus;

        if (aListUsers == null || aListUsers.isEmpty())
            return null;

        for (int i = 0; i < aListUsers.size(); i++)
        {
            strLine = (String)aListUsers.get(i);
            StringTokenizer sTokLine = new StringTokenizer(strLine);

            strUser =  sTokLine.hasMoreTokens() ? sTokLine.nextToken() : "";
            strStatus = sTokLine.hasMoreTokens() ? sTokLine.nextToken() : "";

            if (strStatus.equals(strType) && !strUser.equals(""))
                aListAccType.add(strUser);
        }
        return aListAccType;
    }

    /**
     *  Creates a new unix user.
     *  @param strName      login name of the user.
     *  @param strHomePath   home directory path of the user.
     */
    public static WMessage createNewUnixUser(String strName, String strHomePath)
    {
        WMessage msg = new WMessage(true, "");
        String strHomeDir = WFileUtil.parseDir(strHomePath, strName);
        if (Util.iswindows())
        {
            strHomeDir = UtilB.windowsPathToUnix(strHomeDir);
            if (!isUnixUser(strName, false) && strHomeDir.indexOf(' ') >= 0)
            {
                msg.bOk = false;
                String strMsg = "Home directory '" + strHomePath + "' is not allowed." +
                                " Please try again.";
                msg.setMsg(strMsg);
                Messages.postError(strMsg);
                return msg;
            }                
        }

        if (strName != null && strName.length() > 0 && (Util.OSNAME == null ||
            !Util.OSNAME.startsWith("Mac")))
        {
            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.SYS_VNMR + "/bin/getgroup"};
            msg = WUtil.runScript(cmd);
            String group = msg.getMsg();
            if (group == null || group.equals(""))
                group = "nmr";
            String strScript = WGlobal.SBIN + "makeuser";

            // the check for windows will fail, since can't find paths with /vnmr,
            // so skip it
            if (!Util.iswindows() && !WFileUtil.scriptExists(strScript))
                Messages.postError("File not found: " + strScript);
            else
            {
                String[] aStrCreateUser = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO
                                            + strScript + " " + strName
                                            + " " + strHomeDir + " " + group + " y"};
                // run the script to create a user.
                WUtil.runScript(aStrCreateUser);
                // run the testuser script to see if the unix user got created.
                msg.bOk = isUnixUser(strName, true);
                
                // Write this user to the operatorlist file
                HashMap hmUser = WFileUtil.getHashMapUser(strName);
                WUserUtil.writeOperatorFile(strName, hmUser);

            }
        }
        if (!msg.isNoError())
            msg.setMsg("Error creating unix user: " + strName);

        return msg;
    }

//    public static boolean writeNewUsersFile(final String strUser, final String strItype,
//                                            final HashMap hmUserDef, final boolean bConvert,
//                                            final JPanel pnlItem)
//    {
//        new Thread(new Runnable()
//        {
//            public void run()
//            {
//                Messages.postInfo("Please wait...");
//                writeNewUserFile(strUser, strItype, hmUserDef, bConvert, pnlItem);
//            }
//        }).start();
//
//        return true;
//    }

    /**
     *  Writes the sysprofile and userprofile files for the new user.
     *  @param strUser      the login name of the user.
     *  @param strItype     the interface string for the user in the following form:
     *                      ~Experimental~Imaging~
     *  @param hmUserDef    the user defaults set by the administrator.
     *  @param bConvert     if it's converting vnmr users to vnmrj users, then true
     *                      else if just creating new vnmrj users, then false.
     *  @param pnlItem      if creating a new user by filling the fields in the panel,
     *                      then use the values from the fields,
     *                      else if creating a new user by using defaults, then it's null.
     */
    public static boolean writeNewUserFile(String strUser, String strItype,
                                            HashMap hmUserDef, boolean bConvert,
                                            JPanel pnlItem, HashMap userInfo)
    {
        String strPbPath = FileUtil.openPath(SYSPROFILE + File.separator + strUser);
        String strPrvPath = FileUtil.openPath(USRPROFILE + File.separator + strUser);
        WMessage msg = new WMessage(true, "");
        HashMap hmUser = null;
        Date date = new Date();

        // if the files don't exist, then it's a new user
        if (strPbPath == null && strPrvPath == null)
        {
            if (!isUserNameOk(strUser))
                return false;
            strPbPath = FileUtil.savePath(SYSPROFILE + File.separator + strUser);
            strPrvPath = FileUtil.savePath(USRPROFILE + File.separator + strUser);

            // a new user is being created by dragging the new user button
            // to the panel, and filling the fields in the panel.
            if (pnlItem != null)
            {
                hmUser = WFileUtil.makeNewHM(pnlItem, strUser);
                // check to see if the homedir entered in the panel
                // is the same as the unix home dir for the existing user.
                if (isUnixUser(strUser, true))
                {
                    String strHomeDir = getUserHome(strUser);
                    String strHomeEntered = (String)hmUser.get("home");
                    if (Util.iswindows() && strHomeDir != null && 
                            strHomeDir.trim().equals(""))
                        strHomeDir = strHomeEntered;
                    if (strHomeDir != null &&
                        !(strHomeDir.trim().equalsIgnoreCase(strHomeEntered)))
                    {
                        Messages.postWarning("User '" + strUser +
                            "' is a defined user with homedir " + strHomeDir);
                        msg.bOk = false;
                        return msg.bOk;
                    }
                }

            }
            // one or more new users are being created either from the
            // multi-users dialog or the convert dialog,
            // and they use the userDefaults to fill all the fields.
            else
            {
                if(userInfo != null) {
                    // Must be adding user from a file with possible optional
                    // information
                    hmUser = WFileUtil.makeNewHM(hmUserDef, strUser, userInfo);
                    strItype = (String) userInfo.get("itype");
                }
                else {
                    hmUser = WFileUtil.makeNewHM(hmUserDef, strUser, strItype);
                }
 
                if (isUnixUser(strUser, true))
                    setUserHome(strUser, hmUser);
                else
                {
                    // if the fullname is null, then put the userlogin as the fullname.
                    String strFullname = (String)hmUser.get("name");
                    if (strFullname == null || strFullname.equals(""))
                        hmUser.put("name", strUser);
                }
                String strAppDir = (String)hmUser.get("appdir");
                strAppDir = getAppDir(strAppDir, strItype);
                hmUser.put("appdir", strAppDir);
                msg.bOk = isReqFieldsFilled(hmUser);
            }

            // For mac, only create users if they are already system users
            if (Util.OSNAME != null && Util.OSNAME.startsWith("Mac") &&
                !isUnixUser(strUser))
            {
                Messages.postError(strUser + " is not a defined system user. " +
                                     "Please create the user using system tools " +
                                     "and try again.");
                return false;
            }

            // check home dir, if it doesn't exist, don't write the files
            msg.bOk = homeDirExists(strUser, hmUser);

            if (msg.bOk)
            {
                // write the file
                writeUserFile(strPbPath, strPrvPath, hmUser, hmUserDef);
                String strItemPath = strPbPath + File.pathSeparator + strPrvPath;
                //updateGroup(strItemPath, strUser);
                writePropFile(strUser, hmUser);
                writeOperatorFile(strUser, hmUser);
                AppIF appIf = Util.getAppIF();
                if (appIf instanceof VAdminIF)
                    ((VAdminIF)appIf).getUserToolBar().firePropertyChng(WGlobal.SAVEUSER_NOERROR, "", strUser);
                msg.bOk = createUser(strUser, hmUser, bConvert);
                if (msg.bOk)
                {
                    // vnmr group is the standard group,
                    // add it to this for backward compatability
                    addToVnmrGroup(strUser);
                    String strMsg = "New user '" + strUser + "' created.";
                    writeAuditTrail(date, strUser, strMsg);
                    Messages.postInfo(strMsg);
                }
                else
                {
                    WUtil.deleteFile(strPbPath);
                    WUtil.deleteFile(strPrvPath);
                    WUtil.deleteFile(FileUtil.openPath(USRDATADIR + File.separator + strUser));
                    WUtil.deleteFile(FileUtil.openPath(USRTEMPLATE + File.separator + strUser));
                }
            }
        }
        else
        {
            Messages.postInfo("User '" + strUser + "' already exists.");
        }

        return msg.bOk;
    }

    protected static void writeUserFile(String strPbPath, String strPrvPath,
                                            HashMap hmUser, HashMap hmUserDef)
    {
        Iterator keySetItr = hmUser.keySet().iterator();
        String strKey = null;
        String strValue = null;
        String strLine = null;
        StringBuffer sbPbLine = new StringBuffer();
        StringBuffer sbPrvLine = new StringBuffer();
        BufferedWriter outPb = WFileUtil.openWriteFile(strPbPath);
        BufferedWriter outPrv = WFileUtil.openWriteFile(strPrvPath);

        try
        {
            while(keySetItr.hasNext())
            {
                strKey = (String)keySetItr.next();
                strValue = (String)hmUser.get(strKey);
                if (strValue == null)
                    strValue = "";

                strLine = strKey + '\t' + strValue + '\n';

                if (WUserDefData.isKeyPublic(strKey, hmUserDef))
                    sbPbLine.append(strLine);
                else
                    sbPrvLine.append(strLine);
            }
            WFileUtil.writeAndClose(outPb, sbPbLine);
            WFileUtil.writeAndClose(outPrv, sbPrvLine);

            if (Util.iswindows())
            {
                String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, "chmod 755 " +
                                 UtilB.windowsPathToUnix(strPbPath) + " " +
                                 UtilB.windowsPathToUnix(strPrvPath)};
                WUtil.runScript(cmd);
            }
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
    }

    protected static String getOperators(String strUser, HashMap hmUser)
    {
        String strOperators = (String)hmUser.get("operators");
//        String strItype = (String)hmUser.get("itype");
        // add the user as one of the operators.
//        if (strItype.equals(Global.WALKUPIF))
//        {
            if (strOperators == null)
                strOperators = "";
            strOperators = strOperators + " " + strUser;
//        }
        return strOperators;
    }

    public static boolean isOperatorNameok(String strOperator, boolean bShowMsg)
    {
        if (strOperator == null || strOperator.trim().equals(""))
            return false;

        int nSize = strOperator.length();
        for (int i = 0; i < nSize; i++)
        {
            char c = strOperator.charAt(i);
            if  (!Character.isLetterOrDigit(c) &&
                 c != '_' && c != '-' && c != '.')
            {
                if (bShowMsg)
                    Messages.postInfo("Operator name " + strOperator + " is not valid.");
                return false;
            }
        }
        return true;
    }

    public static void writeOperatorFile(String strUser, HashMap hmUser)
    {
        String strOperators = getOperators(strUser, hmUser);
        if (strOperators == null)
            return;

        String strPath = FileUtil.openPath(OPERATORLIST);
        StringBuffer sbPassword = new StringBuffer();
        // list of all operators from the operator list file
        HashMap hmOperatorsFile = getOperatorList(strPath);
        String strItype = (String)hmUser.get("itype");
        if (strItype == null || strItype.equals(""))
            strItype= "Spectroscopy";
        String strName = (String)hmUser.get("name");
        if (strName == null || strName.equals(""))
            strName = "null";
        String strEmail = (String)hmUser.get("email");
        if (strEmail == null || strEmail.equals(""))
            strEmail = "null";
        String strProfile = (String)hmUser.get("profile");
        if (strProfile == null || strProfile.equals(""))
            strProfile = "";
        String strUserOwner = (String)hmUser.get("user");
        if (strUserOwner == null || strUserOwner.equals(""))
            strUserOwner = "";
        
        // the user can be a operator for walkup interface
//        if (hmOperatorsFile.containsKey(strUser) && !strItype.equals(Global.WALKUPIF))
//            hmOperatorsFile.remove(strUser);
        if (strPath == null)
        {
            strPath = FileUtil.savePath(OPERATORLIST);
            StringBuffer sbData = new StringBuffer();
            int nSize = WOperators.OPERATORS.length;
            for (int i = 0; i < nSize; i++)
            {
                sbData.append(WOperators.OPERATORS[i]).append(";");
            }
            hmOperatorsFile.put("#", "#  " + sbData.toString());
        }

        // list of operators entered for this user
        ArrayList aListOperators = WUtil.strToAList(strOperators, false, " \t,");
        String password = WOperators.getDefPassword();
        try
        {
            password = PasswordService.getInstance().encrypt(password);
        }
        catch (Exception e) {}

        int nSize = aListOperators.size();
        Set keyset = hmOperatorsFile.keySet();
        try
        {
            for (int i = 0; i < nSize; i++)
            {
                String strOperator = (String)aListOperators.get(i);
                StringBuffer sbOperator = new StringBuffer();
                boolean bOwner = strUser.equals(strOperator);
//                String strShowCommandLine = "no";
//                if (bOwner)
//                {
//                    strShowCommandLine = (String)hmUser.get("cmdArea");
//                    if (strShowCommandLine == null || strShowCommandLine.trim().equals(""))
//                        strShowCommandLine = "yes";
//                    else
//                        strShowCommandLine = strShowCommandLine.toLowerCase();
//                }
                // if operator is already in the file, add the username that is
                // adding this as one of the operators to this operators line
                if (keyset.contains(strOperator))
                {
                    // The operator already exists
                    String strOperatorlist = (String)hmOperatorsFile.get(strOperator);
                    if (bOwner)
                    {
                        StringTokenizer strTok = new StringTokenizer(strOperatorlist, ";\n");
                        StringBuffer sbOperatorlist = new StringBuffer();
                        int j = 0;
                        while (strTok.hasMoreTokens())
                        {
                            String strValue = strTok.nextToken();
//                            if (j == 4)
//                                strValue = strShowCommandLine;
                            sbOperatorlist.append(strValue).append(";");
                            j++;
                        }
                        strOperatorlist = sbOperatorlist.toString();
                    }
                    int nIndex = strOperatorlist.indexOf(';');
                    String strlist = strOperatorlist;
                    if (nIndex >= 0)
                       strlist = strOperatorlist.substring(0, nIndex);
                    strlist = strlist.substring(strOperator.length()+1).trim();
                    if (strlist != null && strlist.equals("null"))
                        strlist = "";
                    // List of users that have this operator in their list
                    ArrayList alistOperatorsFile = WUtil.strToAList(strlist, false, ",");
                    if (!alistOperatorsFile.contains(strUser))
                    {
                        String user;
                        if(strUserOwner == null || strUserOwner.equals("")) 
                            user = strUser;
                        else
                            user =  strUserOwner;

                        if (nIndex >= 0)
                        {    
                            // Add this user to the operators user list
                            sbOperator.append(strOperator).append("  ").append(strlist);
                            if (!strlist.equals(""))
                                sbOperator.append(",");
                            sbOperator.append(user).append(
                                strOperatorlist.substring(nIndex, strOperatorlist.length()));
                        }
                        else
                            sbOperator.append(strOperatorlist).append(",").append(user);
                    }
                    else
                        sbOperator.append(strOperatorlist);
                }
                // write operator adm the username that is adding this as one of the operators
                else
                {
                    // This operator does not exist yet
                    String user;
                    if(strUserOwner == null || strUserOwner.equals("")) 
                        user = strUser;
                    else
                        user =  strUserOwner;

                    String strPanellevel = WGlobal.PANELLEVEL;
                    if (bOwner)
                        strPanellevel = "30";
                    sbOperator.append(strOperator).append("  ").append(
                        user).append(";").append(strEmail).append(";").append(strPanellevel).append(
                        ";");
                    boolean bUnixUser = isUnixUser(strOperator);
                    strName = "null";
                    if (isVjUser(strOperator))
                        strName = (String)WFileUtil.getHashMapUser(strOperator).get("name");
                    else {
                        // If this came from adding via text file, there may
                        // be an operator and it's full name, try it.
                        strName = (String)hmUser.get("operatorFull");
                        if(strName == null)
                            strName = "null";
                    }
                    sbOperator.append(strName);
                    if (!bUnixUser && !bOwner)
                        sbPassword.append(strOperator).append("  ").append(password).append("\n");
                    
                    // Create a default profile here based on itype for the user
                    
                    if(strProfile.equals("")) {
                    if (strItype.equals(Global.WALKUPIF) || strItype.equalsIgnoreCase("Walkup")) {
                        strProfile = "AllLiquids";
                    }
                    else if (strItype.equals(Global.IMGIF)) {
                        strProfile = "AllImaging";
                    }
                    }
                    if(strProfile != "") {
                        sbOperator.append(";").append(strProfile);
                    }
     
                    
                }
                // Replace the files line with the new line
                hmOperatorsFile.put(strOperator, sbOperator.toString());
                RightsList rightsList = new RightsList(true);
                rightsList.operatorProfile(strUser, true);
            }
            
            // Okay, the code above works fine for adding operators for a user.
            // Unfortunately, it does not take into account that an operator
            // may get removed from the list.
            // Note that the panel is oriented by User and
            // its list of operators.  However, the operatorlist file is
            // oriented by operator.  That is, each line has the operator
            // name followed by a list of users that have specified that
            // operator.
            // We are called for a given user (strUser).  We now need to go
            // through every line of the operatorlist file and be sure each
            // operator that has strUser listed is still in strUser's list of
            // operators. 
            
            // Loop through all lines of operatorlist file
            keyset = hmOperatorsFile.keySet();
            Iterator<String> iter = keyset.iterator();
            while(iter.hasNext()) {
                String operator= iter.next();
                // Get a line of the operatorlist file
                String strOperatorlist = (String)hmOperatorsFile.get(operator);
                
                // Get the user list for this operator line
                int nIndex = strOperatorlist.indexOf(';');
                String strlist = strOperatorlist;
                if (nIndex >= 0)
                   strlist = strOperatorlist.substring(0, nIndex);
                if (strlist != null && strlist.equals("null"))
                    strlist = "";
                strlist = strlist.substring(operator.length()+1).trim();
                // List of users that have this operator in their list in
                // the operatorlist file.
                ArrayList<String> alistOperatorsFile = WUtil.strToAList(strlist, false, ",");

                // aListOperators is the list of operators entered for this user
                // in the panel.  See if it contains 'operator'
                if(!aListOperators.contains(operator)) {
                    // No, it must have been removed from the panel
                    // We need to remove operator from hmOperatorsFile for this
                    // user.
                    // The line for this user is strOperatorlist.
                    // alistOperatorsFile has the list of users for this
                    // operator from the file.  We need to recreate the user
                    // list based on alistOperatorsFile with strUser removed
                    alistOperatorsFile.remove(strUser);
                    // Remove everything from strOperatorlist to ";")
                    nIndex = strOperatorlist.indexOf(';');
                    if (nIndex >= 0) {
                        String end = strOperatorlist.substring(nIndex);
                        // Turn 
                        StringBuffer line = new StringBuffer();
                        line.append(operator).append("  ");
                        for(int i=0; i < alistOperatorsFile.size(); i++) {
                            String user = alistOperatorsFile.get(i);
                            line.append(user);
                            if(alistOperatorsFile.size() > i+1) {
                                line.append(",");
                            }
                        }
                        if(alistOperatorsFile.size() == 0) 
                            line.append("null");
                        
                        // Replace the end of the line as it was.
                        line.append(end);
                        
                        // Replace the old line with the new one
                        hmOperatorsFile.put(operator, line.toString());
                    } 
                }
            }
            
            writeOperatorList(strPath, hmOperatorsFile);
            writeOperatorFile(sbPassword);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }

    }

    public static HashMap getOperatorList(String strPath)
    {
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        HashMap hmOperators = new HashMap();
        if (reader == null)
        {
            // If this is a new build there may not be an operatorlist file yet
            // just return the empty HashMap.
            return hmOperators;
        }

        String strLine;
        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                StringTokenizer strTok = new StringTokenizer(strLine, " ");
                String strOperator = null;
                if (strTok.hasMoreTokens())
                    strOperator = strTok.nextToken();

                if (strOperator != null)
                    hmOperators.put(strOperator, strLine);
            }
            reader.close();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
        return hmOperators;
    }

    public static ArrayList getOperatorList()
    {
        String strPath = FileUtil.openPath(OPERATORLIST);
        ArrayList aListOperators = new ArrayList();
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        if (reader == null)
            return null;

        String strLine;
        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                if(strLine.startsWith("#"))
                    continue;
                StringTokenizer strTok = new StringTokenizer(strLine, " ");
                String strOperator = "";
                if (strTok.hasMoreTokens())
                    strOperator = strTok.nextToken();

                if (strOperator != null && strOperator.length() > 0)
                    aListOperators.add(strOperator);
            }
            Collections.sort(aListOperators);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
        return aListOperators;
    }

    public static String getOperatordata(String strOperator, String key)
    {
        boolean bOperator = false;
        String strValue = "";

        Vector vecColumns = WOperators.getDataVector(true);
        Vector vecRows = WOperators.getDataVector(false);

        if (vecColumns == null || vecRows == null)
            return null;

        int nColumn = 0;
        int nLength = vecColumns.size();
        for (int i = 0; i < nLength; i++)
        {
            String strColumn = (String)vecColumns.get(i);
            if (strColumn != null && strColumn.equals(key))
            {
                nColumn = i;
                break;
            }
        }

        nLength = vecRows.size();
        for (int i = 0; i < nLength; i++)
        {
            Vector vecRow = (Vector)vecRows.get(i);
            int nLength2 = vecRow.size();
            for (int j = 0; j < nLength2; j++)
            {
                strValue = (String)vecRow.get(j);
                if (j == 0)
                {
                    if (strValue.equals(strOperator))
                        bOperator = true;
                    else
                        break;
                }
                else if (j == nColumn)
                    return strValue;
            }
            strValue = "";
        }
        return strValue;
    }


    public static void writeOperatorList(String strPath, HashMap hmOperators)
    {
        if (hmOperators == null || hmOperators.isEmpty())
            return;

        BufferedWriter writer = WFileUtil.openWriteFile(strPath);
        if (writer == null)
            return;

        StringBuffer sbOperators = new StringBuffer();
        Iterator iter = hmOperators.keySet().iterator();
        String strOperator;
        String strlist;
        strlist = (String)hmOperators.get("#");
        sbOperators.append(strlist).append("\n");
        while (iter.hasNext())
        {
            strOperator = (String)iter.next();
            strlist = (String)hmOperators.get(strOperator);
            
            if (!strOperator.equals("#"))
            {
                // delete the last semi-colon
                int nLength = strlist.length();
                if (strlist.lastIndexOf(';') == nLength-1)
                    strlist = strlist.substring(0, nLength-1);
            }
            
            if (!strOperator.equals("#"))
                sbOperators.append(strlist).append("\n");
        }
        WFileUtil.writeAndClose(writer, sbOperators);
    }

    public static void writeOperatorFile(StringBuffer sbPassword)
    {
        String strPath = FileUtil.openPath(PASSWORD);
        if (strPath == null)
        {
            strPath = FileUtil.savePath(PASSWORD);
            if (strPath == null)
            {
                Messages.postDebug("Couldn't open file " + PASSWORD);
                return;
            }
        }

        try
        {
            BufferedWriter writer = new BufferedWriter(new FileWriter(strPath, true));
            writer.write(sbPassword.toString());
            writer.flush();
            writer.close();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

    public static ArrayList getAdminList()
    {
        ArrayList aListOperator = new ArrayList();
        String strUser = System.getProperty("user.name");
        String strPath = FileUtil.openPath(ADMINLIST);
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        String strLine;

        if (reader != null)
        {
            try
            {
                while ((strLine = reader.readLine()) != null)
                {
                    if (strLine.startsWith("#"))
                        continue;

                    StringTokenizer strTok = new StringTokenizer(strLine, ";\n");
                    if (strTok.hasMoreTokens())
                    {
                        aListOperator.add(strTok.nextToken());
                    }
                }
            }
            catch (Exception e)
            {
                //e.printStackTrace();
                Messages.writeStackTrace(e);
            }
        }

        if (!aListOperator.contains(strUser))
            aListOperator.add(strUser);

        return aListOperator;
    }

    public static boolean restoreUser(String strName, String strHome)
    {
        // restore the user if there isn't another user with the  same login name.
        // This is an error condition, there might be a new user with the same login
        // name as the deleted one, in this case donot restore the user.
        if (isVjUser(strName))
        {
            Messages.postWarning("Unable to restore user '" + strName + "'"
                                    + " User already exists ");
            return false;
        }

        // create the unix user
        WMessage msg = WUserUtil.createNewUnixUser(strName, strHome);

        if (msg.bOk)
            msg = createPgSqlUser(strName);

        // restore /vnmr/adm/files
        String strPath = FileUtil.openPath(USRPROFILE+File.separator+strName+".del");
        String strRenamePath = FileUtil.openPath(USRPROFILE)+File.separator+ strName;
        WUtil.renameFile(strPath, strRenamePath);

        strPath = FileUtil.openPath(SYSPROFILE+File.separator+strName+".del");
        strRenamePath = FileUtil.openPath(SYSPROFILE) + File.separator +strName;
        WUtil.renameFile(strPath, strRenamePath);

        strPath = FileUtil.openPath(USRDATADIR+File.separator+strName+".del");
        strRenamePath = FileUtil.openPath(USRDATADIR) +File.separator+strName;
        WUtil.renameFile(strPath, strRenamePath);

        strPath = FileUtil.openPath(USRTEMPLATE);
        if (strPath != null)
        {
            File fileobj = new File(strPath);
            String[] files = fileobj.list(new VTemplate(strName+".", ".del"));
            int nSize = files.length;
            for (int i = 0; i < nSize; i++)
            {
                strPath = FileUtil.openPath(WUserUtil.USRTEMPLATE+File.separator+files[i]);
                //System.out.println("panel " + strPath);
                strRenamePath = strPath.substring(0, strPath.indexOf(".del"));
                //System.out.println("panel " + strRenamePath);
                WUtil.renameFile(strPath, strRenamePath);
            }
        }
        strPath = FileUtil.openPath(USRTEMPLATE+File.separator+strName+".del");
        strRenamePath = FileUtil.openPath(USRTEMPLATE) +File.separator+strName;
        WUtil.renameFile(strPath, strRenamePath);

        //Part11Dir
        if (Util.isPart11Sys())
        {
            strPath = FileUtil.openPath(USRPART11DIR+File.separator+strName+".del");
            strRenamePath = FileUtil.openPath(USRPART11DIR) +File.separator+strName;
            WUtil.renameFile(strPath, strRenamePath);

            // remove the username of the restore user from the delete user list
            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,  WGlobal.SUDO +
                                WGlobal.SBIN + "auredt " + DELFILE + " " + strName + " Restore"};
            WUtil.runScript(cmd);
        }

        writeAuditTrail(new Date(), strName, "Restored user '" + strName + "'.");

        return msg.bOk;
    }

    public static void updateUser(String strName)
    {
        String strHomeDir = getUserHome(strName);
        if (strHomeDir != null && strHomeDir.length() > 0)
            createNewUnixUser(strName, strHomeDir);
        writeAuditTrail(new Date(), strName, "Updating user: " + strName);
    }

    public static boolean isUnixUser(String struser)
    {
        return isUnixUser(struser, false);
    }

    /**
     *  Checks if the user is already a unix user.
     */
    public static boolean isUnixUser(String strName, boolean bMessage)
    {
        boolean bUnix = false;
        if (strName == null || strName.length() == 0)
            return bUnix;
        String strScript =  WGlobal.SBIN + "jtestuser";
        if (Util.OSNAME != null && Util.OSNAME.startsWith("Mac"))
            strScript = FileUtil.SYS_VNMR + "/bin/jtestuser";

        String[] aStrTestUser = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO +
                                   strScript + " " + strName };
        // check for windows will fail, since can't find paths with /vnmr so skip it
        if (!Util.iswindows() && !WFileUtil.scriptExists(strScript))
            Messages.postError(strScript + " not found.");
        else
        {
            if (Util.OSNAME != null && Util.OSNAME.startsWith("Mac"))
                aStrTestUser[2] = strScript + " " + strName;
            WMessage msg = WUtil.runScript(aStrTestUser, bMessage);
            bUnix = msg.isNoError();
        }

        return bUnix;
    }

    public static boolean isVjUser(String strName)
    {
        boolean bVJ = false;
        if (strName == null || strName.length() == 0)
            return bVJ;

        String strPbPath = FileUtil.openPath(SYSPROFILE + File.separator + strName);
        String strPrvPath = FileUtil.openPath(USRPROFILE + File.separator + strName);

        bVJ = (strPbPath != null || strPrvPath != null) ? true : false;

        return bVJ;
    }

    public static boolean isUserNameOk(String strName)
    {
        int nLength = strName.length();
        boolean bOk = true;
        for (int i = 0; i < nLength; i++)
        {
            char c = strName.charAt(i);
            if (!Character.isLetterOrDigit(c) &&
                c != '_' && c != '-' && c != '.')
            {
                Messages.postWarning("User name " + strName + " is not valid.");
                return false;
            }
        }

        if (Util.isPart11Sys())
        {
            // if the username is in the list of users that are deleted,
            // then that username cannot be used again.
            BufferedReader reader = WFileUtil.openReadFile(DELFILE);
            String strLine = "";
            ArrayList aListUsers = new ArrayList();
            if (reader == null)
                bOk = true;
            else
            {
                try
                {
                    while ((strLine = reader.readLine()) != null)
                    {
                        aListUsers = WUtil.strToAList(strLine, false);
                        if (aListUsers.contains(strName))
                        {
                            bOk = false;
                            break;
                        }
                    }
                    reader.close();
                }
                catch (Exception e)
                {
                    //e.printStackTrace();
                    Messages.writeStackTrace(e);
                }
            }
            if (!bOk)
                Messages.postError("New user cannot be created with username '" + strName + "'");
        }
        return bOk;
    }

    public static WMessage createPgSqlUser(String strName)
    {
        String sysDir = System.getProperty("sysdir");
        if (sysDir == null)
            sysDir = File.separator+"vnmr";
        if (Util.iswindows())
            sysDir = UtilB.windowsPathToUnix(sysDir);

        String strg = null;

        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, sysDir +
                            "/bin/create_pgsql_user" + " " + strName };
        WMessage msg = WUtil.runScript(cmd);
       return msg;
    }

    public static void updateDB()
    {
        new Thread(new Runnable()
        {
            public void run()
            {
                try
                {
                    Messages.postDebug("Running manageDB update");
                    LoginService loginService = LoginService.getDefault();
                    Hashtable userHash = loginService.getuserHash();
                    ShufDBManager  dbManager = ShufDBManager.getdbManager();
                    dbManager.updateDB(userHash, 0);
                }
                catch (Exception e)
                {
                    //e.printStackTrace();
                    Messages.writeStackTrace(e);
                    Messages.postDebug("ERROR running managedb update " + e.toString());
                }
            }
        }).start();
    }

    public static String getUserHome(String strName)
    {
        String strHomeDir = "";
        if (Util.OSNAME == null || !Util.OSNAME.startsWith("Mac"))
        {
            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.SYS_VNMR + "/bin/getuserinfo " +
                              strName };
            WMessage msg = WUtil.runScript(cmd);
            String strMsg = msg.getMsg();
            if (strMsg == null)
                strMsg = "";

            char cToken = ':';
            if (Util.OSNAME != null && Util.OSNAME.startsWith("Windows"))
                cToken = ';';
            strHomeDir = WUtil.getToken(strMsg,cToken, 2);
        }
        else
            strHomeDir = "/Users/"+strName;

        return strHomeDir;
    }

    public static void setAppMode(String strUser) {
        if (strUser == null || strUser.trim().equals(""))
            return;

        String strScript = WGlobal.SBIN + "makeuser";
        if (!Util.iswindows() && !WFileUtil.scriptExists(strScript))
                Messages.postError("File not found: " + strScript);
        else
            {
                String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO
                                            + strScript +" "+ strUser + " $HOME nmr appmode"};
                // run the script to create a user.
                WUtil.runScript(cmd);
        }
    }

    /**
     *  This method was taken from the VnmrjAdmin class to set the home directory
     *  of the existing unix user.
     */
    public static void setUserHome(String strName, HashMap hmDef)
    {
        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.SYS_VNMR + "/bin/getuserinfo " +
                          strName };
        String strHomeDir = null;
        String strLongName = null;
        boolean bwindows = Util.iswindows();

        if (Util.OSNAME == null || !Util.OSNAME.startsWith("Mac"))
        {
            WMessage msg = WUtil.runScript(cmd, false);
            String strMsg = msg.getMsg();

            char cToken = ':';
            if (bwindows)
                cToken = ';';
            String data = WUtil.getToken(strMsg, cToken, 1);
            if (data != null)
                strLongName = data;
            strHomeDir = WUtil.getToken(strMsg,cToken, 2);
        }
        else
            strHomeDir = "/Users/"+strName;

        if (strLongName != null)
        {
            hmDef.put("name", strLongName);
        }

        if ((strHomeDir != null && !strHomeDir.trim().equals("")))
        {
            hmDef.put("home", strHomeDir);
        }
    }

    public static String getName(String strUser)
    {
        String strName = "";
        if (Util.OSNAME == null || !Util.OSNAME.startsWith("Mac"))
        {
            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.SYS_VNMR + "/bin/getuserinfo " +
                             strUser };
            WMessage msg = WUtil.runScript(cmd);
            String strMsg = msg.getMsg();
            if (strMsg == null)
                strMsg = "";

            char cToken = ':';
            if (Util.iswindows())
                cToken = ';';
            strName = WUtil.getToken(strMsg, cToken, 1);
        }

        return strName;

    }

    public static void setAppdir(HashMap hmDef)
    {
        // set the appdir, according to the interface type selected.
        String strName = (String)hmDef.get(WGlobal.NAME);
        String strAppDirKey = "appdir";
        Object objData = hmDef.get(strAppDirKey);
        String strAppdir = (objData instanceof WUserDefData) ?
                            ((WUserDefData)objData).getValue() : (String)objData;

        objData = hmDef.get("itype");
        String strItype = (objData instanceof WUserDefData) ?
                            ((WUserDefData)objData).getValue() : (String)objData;
        String strAppDir = getAppDir(strAppdir, strItype);
        hmDef.put(strAppDirKey, strAppdir);
        VItemArea1.firePropertyChng(WGlobal.IMGDIR, "", strName);
    }

    public static String getAppDir(String strAppdir, String strItype)
    {
	if(strAppdir.length() < 1) return strItype;
        else return strAppdir;
	// commented out the following because strItype is used for appdir.

/*
        boolean bImg = strItype.equals(Global.IMGIF) ? true : false;
        boolean bWalkup = strItype.equals(Global.WALKUPIF) ? true : false;
        boolean blc = strItype.equals(Global.LCIF) ? true : false;

        String strToken = " ";
        if (Util.iswindows())
            strToken = File.pathSeparator;
        ArrayList aListDirs = WUtil.strToAList(strAppdir, strToken);
        int nIndex = aListDirs.indexOf(FileUtil.sysdir());
        //if (nIndex < 0)
        //    nIndex = aListDirs.indexOf(UtilB.unixPathToWindows("/vnmr"));
        if (nIndex < 0)
            nIndex = aListDirs.size() - 1;

        if (bImg && !aListDirs.contains(WGlobal.IMGDIR))
        {
            if (nIndex >= 0 && nIndex < aListDirs.size())
                aListDirs.add(nIndex, WGlobal.IMGDIR);
        }
        else if (bWalkup && !aListDirs.contains(WGlobal.WALKUPDIR))
        {
            if (nIndex >= 0 && nIndex < aListDirs.size())
                aListDirs.add(nIndex, WGlobal.WALKUPDIR);
        }
        else if (blc && !aListDirs.contains(WGlobal.LCDIR))
        {
            if (nIndex >= 0 && nIndex < aListDirs.size())
                aListDirs.add(nIndex, WGlobal.LCDIR);
        }

        if (!bImg && aListDirs.contains(WGlobal.IMGDIR))
            aListDirs.remove(WGlobal.IMGDIR);
        if (!bWalkup && aListDirs.contains(WGlobal.WALKUPDIR))
            aListDirs.remove(WGlobal.WALKUPDIR);
        if (!blc && aListDirs.contains(WGlobal.LCDIR))
            aListDirs.remove(WGlobal.LCDIR);

        strAppdir = WUtil.aListToStr(aListDirs, strToken);
        return strAppdir;
*/
    }

    /**
     *  Update the group that is in the grouplist of the user.
     *  Called when a new user is created or the info for the existing user is saved.
     */
    public static void updateGroup(String strPath, String strName)
    {
         HashMap hmItem = WFileUtil.getHashMap(strPath);

        try
        {
            HashMap hmValue = new HashMap();
            hmValue.put("item", strName);

            if (hmItem != null)
            {
                String strValue = (String)hmItem.get("grouplist");
                ArrayList aListNames = WUtil.strToAList(strValue, false);
                hmValue.put("list", aListNames);
                VItemAreaIF.firePropertyChng(WGlobal.ITEM_LIST, "All", hmValue);
            }
        }
        catch(Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }
    }

    public static void addToVnmrGroup(String strUser)
    {
        String strPath = FileUtil.openPath("SYSTEM/USRS/group");
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        if (reader == null)
        {
            Messages.postDebug("Could not add " + strUser + " in vnmr group.");
            return;
        }

        String strLine = null;
        boolean bInGroup = false;
        StringBuffer sbData = new StringBuffer();

        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                if (strLine.indexOf("VNMR group") >= 0)
                {
                    if (!isInVnmrGroup(strLine, strUser))
                    {
                        strLine = strLine + "," + strUser;
                    }
                }
                sbData.append(strLine);
                sbData.append("\n");
            }
            reader.close();
            BufferedWriter writer = WFileUtil.openWriteFile(strPath);
            if (writer != null)
            {
                writer.write(sbData.toString());
                writer.flush();
                writer.close();
            }
        }
        catch (Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }
    }

    protected static boolean isInVnmrGroup(String strLine, String strUser)
    {
        boolean bInGroup = false;
        int nIndex = strLine.lastIndexOf(":");
        if (nIndex >= 0 && nIndex+1 < strLine.length())
        {
            String strGrpList = strLine.substring(nIndex+1);
            StringTokenizer strTok = new StringTokenizer(strGrpList, ",");
            while(strTok.hasMoreTokens())
            {
                String strName = strTok.nextToken();
                if (strName != null && strName.trim().equals(strUser))
                    bInGroup = true;
            }
        }
        return bInGroup;
    }

    protected static boolean isReqFieldsFilled(HashMap hmUser)
    {
        boolean bOk = true;
        if (hmUser == null || hmUser.isEmpty())
            return !bOk;

        Iterator keySetItr = hmUser.keySet().iterator();
        String strKey = null;
        String strVal = null;
        String[] aStrReqField;
        if (Util.iswindows())
        {
            if (Util.isPart11Sys())
                aStrReqField = WGlobal.USR_PART11_REQ_FIELDS_WIN;
            else
                aStrReqField = WGlobal.USR_REQ_FIELDS_WIN;
        }
        else
        {
            if (Util.isPart11Sys())
                aStrReqField = WGlobal.USR_PART11_REQ_FIELDS;
            else
                aStrReqField = WGlobal.USR_REQ_FIELDS;
        }

        while(keySetItr.hasNext() && bOk)
        {
            strKey = (String)keySetItr.next();
            if (WFileUtil.isReqField(strKey, aStrReqField))
            {
                strVal = (String)hmUser.get(strKey);
                if (strVal == null || strVal.length() <= 0)
                    bOk = false;
            }
        }
        if (!bOk)
        {
            Messages.postError("Please fill in the required field '" + strKey +
                                    "' in the user defaults");
            Toolkit.getDefaultToolkit().beep();
        }

        return bOk;
    }

    public static void activateAccount(String strUser)
    {
        if (strUser == null || strUser.trim().equals(""))
            return;

        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO +
                          WGlobal.SBIN + "aupw -a " + strUser};
        WUtil.runScriptInThread(cmd, true);
        String strMsg = "Activated locked account '" + strUser + "'.";
        Messages.postInfo(strMsg);

        // add to audit trail
        writeAuditTrail(new Date(), strUser, strMsg);
    }

    /**
     *  Creates a new user. If the user is not a unix user, then makes it a unix user.
     *  The user is then added in the pgsql, and the database is updated.
     *  @param strUser  user that needs to be created.
     *  @param hmUser   hashmap that has user attributes.
     *  @param bConvert true if it's converting vnmr user to vj user.
     */
    private static boolean createUser(String strUser, HashMap hmUser, boolean bConvert)
    {
        WMessage msg = new WMessage(true, "");
        boolean bUnixUser = isUnixUser(strUser, true);

        // check if the deaults is set to run update, even for an existing unix user
        // in this case always run makeuser to update the homedir
        String strUpdate = (String)hmUser.get("update");
        boolean bUpdate = (strUpdate != null && strUpdate.equalsIgnoreCase("no"))
                            ? false : true;

        // create a vj user for a existing vnmr user by running makeuser script
        /*if (bConvert || (bUnixUser && bUpdate))
            msg = makeVjUser(strUser);*/
        // create a unix user if the unix user doesn't already exist,
        // and if it's not converting vnmr users to vnmrj users.
        if ((bConvert || bUpdate || !bUnixUser) && msg.bOk)
            msg = createNewUnixUser(strUser, (String)hmUser.get("home"));

        if (msg.bOk)
            msg = createPgSqlUser(strUser);

        return msg.bOk;
    }

    private static boolean homeDirExists(String strUser, HashMap hmUser)
    {
        boolean bUnixUser = isUnixUser(strUser);
        boolean bOk = true;

        // set the home directory if the unix user already exists.
        if (bUnixUser)
            setUserHome(strUser, hmUser);

        // if the hashmap for this user contains value in the form of $home,
        // then subsitute the actual values before writing the file.
        hmUser = WFileUtil.getAbsoluteValues(hmUser);

        // check if the home directory for this user exists.
        String strHomePath = (String)hmUser.get("home");
        String strHomeDir = WFileUtil.parseDir(strHomePath, strUser);
        bOk = new File(strHomeDir).exists();
        if (!bOk)
        {
            Messages.postError("Error finding or opening " + strHomeDir + ".");
            Messages.postError("Please see that the 'home' in 'User Defaults' dialog in" +
                                    " 'Configure' menu is set correctly");
            Messages.postInfo("For more information, please see 'Setting User Defaults' in the help menu");
        }
        return bOk;
    }


    //==============================================================================
   //   Audit Trail for users methods follows ...
   //============================================================================

   public static void writeAuditTrail(final Date date, final String strUser,
                                        final String strMsg)
    {
        if (!Util.isPart11Sys())
            return;

        new Thread(new Runnable()
        {
            public void run()
            {
                String strPath = Util.getAuditDir();
                writeUserAT(strPath, new UserAuditTrail(date, strUser, strMsg));
            }
        }).start();

    }

    /**
     *  Writes the properties file, and adds the new user to it.
     *  @param strUser  user which needs to be added to the file.
     *  @param hmUser   the hashmap for the user that has the info for the user.
     */
    public static void writePropFile(String strUser, HashMap hmUser)
    {
        if (!Util.isPart11Sys())
            return;

        String strFile = new StringBuffer().append("SYSTEM").append(
                            File.separator).append("USRS").append(
                            File.separator).append("properties").append(
                            File.separator).append("userResources_").append(
                            Util.getHostName()).append(".properties").toString();
        String strPath = FileUtil.openPath(strFile);

        if (strPath == null)
            strPath = FileUtil.savePath(strFile);

        if (strPath == null)
        {
            Messages.postDebug("Administrator does not have write permission for " + strFile);
            return;
        }

        BufferedReader reader = WFileUtil.openReadFile(strPath);
        String strLine;
        StringTokenizer strTok;
        HashMap hmFile = new HashMap();
        String strKey;
        String strValue;

        try
        {
            if (reader != null)
            {
                while ((strLine = reader.readLine()) != null)
                {
                    strTok = new StringTokenizer(strLine, "=");

                    // first token is the key
                    if (strTok.hasMoreTokens())
                        strKey = strTok.nextToken();
                    else
                        strKey = null;

                    // second token is the value
                    if (strTok.hasMoreTokens())
                        strValue = strTok.nextToken();
                    else
                        strValue = null;

                    hmFile.put(strKey, strValue);
                }
                reader.close();
            }

            // append the new user
            String strFullName = (String)hmUser.get("name");
            if (strFullName == null)
                strFullName = "";
            hmFile.put(strUser, strFullName);

            Iterator itr = hmFile.keySet().iterator();
            StringBuffer sbData = new StringBuffer();
            while (itr.hasNext())
            {
                strKey = (String)itr.next();
                strValue = (String)hmFile.get(strKey);
                sbData.append(strKey);
                sbData.append("=");
                sbData.append(strValue);
                sbData.append("\n");
            }

            // write it to the file
            BufferedWriter writer = WFileUtil.openWriteFile(strPath);
            WFileUtil.writeAndClose(writer, sbData);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }

    }

    protected static void writeUserAT(String strPath, UserAuditTrail auditTrail)
    {
        String strDir = new StringBuffer().append(strPath).append(
                        File.separator).append("user").toString();
        strPath = FileUtil.openPath(strDir);
        if (strPath == null)
            strPath = FileUtil.savePath(strDir);

        if (strPath == null)
        {
            Messages.postDebug("File not found " + strDir);
            return;
        }

        File[] files = new File(strDir).listFiles();
        String strAuditFile = getAuditFile(strPath, files);

        BufferedReader reader = WFileUtil.openReadFile(strAuditFile);
        StringBuffer sbData = new StringBuffer();
        String strLine = null;

        try
        {
            if (reader != null)
            {
                // read the data already in the file
                while((strLine = reader.readLine()) != null)
                {
                    sbData.append(strLine);
                    sbData.append("\n");
                }
                reader.close();
            }
            // append the new audittrail
            sbData.append(auditTrail.getDate().toString());
            sbData.append("|");
            sbData.append(WUtil.getCurrentAdmin());
            sbData.append("|");
            sbData.append(auditTrail.getUser());
            sbData.append("|");
            sbData.append(auditTrail.getAction());
            sbData.append("\n");
            // write it to the file
            if (strAuditFile != null)
            {
                BufferedWriter writer = WFileUtil.openWriteFile(new File(strAuditFile));
                WFileUtil.writeAndClose(writer, sbData);
            }
        }
        catch (Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }

    }

    protected static String getAuditFile(String strPath, File[] files)
    {
        String strAuditFile = "";
        if (strPath == null)
            return null;

        // if there are no files in the directory, then start a new file.
        if (files == null || files.length == 0)
        {
            strAuditFile = createNewAuditFile(strPath);
        }
        else
        {
            for(int i = 0; i < files.length; i++)
            {
                File objFile = files[i];
                String strName = objFile.getName();
                if (strName != null && strName.endsWith(".open.uat"))
                {
                    if (objFile.length() < FILELENGTH)
                    {
                        strAuditFile = objFile.getAbsolutePath();
                    }
                    else
                    {
                        // close the existing file
                        String strDate = new Date().toString().replace(' ', '_');
                        strName = strName.replaceAll("open", strDate);
                        File objNewFile = new File(strPath, strName);
                        objFile.renameTo(objNewFile);
                        // create a new file
                        strAuditFile = createNewAuditFile(strPath);
                    }
                    break;
                }
            }
        }
        return strAuditFile;
    }

    protected static String createNewAuditFile(String strPath)
    {
        String strName = new Date().toString() + ".open.uat";
        strName = strName.replace(' ', '_');
        String strAuditFile = FileUtil.savePath(strPath+File.separator+strName);
        return strAuditFile;
    }

    //==============================================================================
   //   Delete user methods follows ...
   //============================================================================

    /**
     *  Deletes the user by deleting the unix user, removing from the vnmr group,
     *  deleting as pgsqluser, and adding to the trashcan.
     *  @param strUser  user to be deleted.
     */
    public static boolean deleteUser(String strUser)
    {
        boolean bNoError = true;
        if (Util.OSNAME == null || (!Util.OSNAME.startsWith("Mac") && !Util.iswindows()))
        {
            // delete the unix user.
            String[] aStrDelUser = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO +
                                    "/usr/sbin/userdel " + strUser };
            WUtil.runScriptInThread(aStrDelUser);
        }

        // write the username being deleted to a file, so that a new user
        // needs to have a different login name.
        if (Util.isPart11Sys())
        {
            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,  WGlobal.SUDO +
                                WGlobal.SBIN + "auredt " + DELFILE + " " + strUser};
            WUtil.runScript(cmd);
        }

        deleteFromVnmrGroup(strUser);
        dropPgSqlUser(strUser);
        String strMsg = "Deleted '" + strUser + "'.";
        Messages.postInfo(strMsg);
        WTrashItem.fullTrashCanIcon();
        writeAuditTrail(new Date(), strUser, strMsg);

        return bNoError;
    }

    public static void deleteUserFiles(String strName)
    {
        // Profiles files
        String strPath = FileUtil.openPath(USRPROFILE+File.separator+strName);
        String strRenamePath = strPath+".del";
        WUtil.renameFile(strPath, strRenamePath);

        strPath = FileUtil.openPath(SYSPROFILE+File.separator+strName);
        strRenamePath = strPath+".del";
        WUtil.renameFile(strPath, strRenamePath);


        // data file
        strPath = FileUtil.openPath(USRDATADIR+File.separator+strName);
        strRenamePath = strPath+".del";
        WUtil.renameFile(strPath, strRenamePath);
        // template file
        strPath = FileUtil.openPath(USRTEMPLATE);
        if (strPath != null)
        {
            File fileobj = new File(strPath);
            String[] files = fileobj.list(new VTemplate(strName+"."));
            int nSize = files.length;
            for (int i = 0; i < nSize; i++)
            {
                strPath = FileUtil.openPath(WUserUtil.USRTEMPLATE+File.separator+files[i]);
                strRenamePath = strPath+".del";
                WUtil.renameFile(strPath, strRenamePath);
            }
        }
        strPath = FileUtil.openPath(USRTEMPLATE+File.separator+strName);
        strRenamePath = strPath+".del";
        WUtil.renameFile(strPath, strRenamePath);

        // part11 file
        if (Util.isPart11Sys())
        {
            strPath = FileUtil.openPath(WUserUtil.USRPART11DIR+File.separator+strName);
            strRenamePath = strPath+".del";
            WUtil.renameFile(strPath, strRenamePath);
        }

    }

    /**
     *  Deletes the user from the vnmrgroup.
     *  @param strUser  the user that needs to be deleted.
     */
    public static void deleteFromVnmrGroup(String strUser)
    {
        String strPath = FileUtil.openPath("SYSTEM/USRS/group");
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        if (reader == null)
            return;

        String strLine = null;
        StringBuffer sbData = new StringBuffer();

        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                if (strLine.indexOf("VNMR group") >= 0)
                {
                    int nIndex = strLine.lastIndexOf(":");
                    if (nIndex < 0)
                        continue;
                    String strTmp = strLine.substring(0, nIndex+1);
                    sbData.append(strTmp);

                    strTmp = strLine.substring(nIndex+1);
                    ArrayList aListUsers = WUtil.strToAList(strTmp, ",");
                    if (aListUsers.contains(strUser))
                        aListUsers.remove(strUser);

                    int nSize = aListUsers.size();
                    for(int i = 0; i < nSize; i++)
                    {
                        sbData.append(aListUsers.get(i));
                        // if it's not the last element in the list, then
                        // append comma.
                        if ((i+1) < nSize)
                            sbData.append(",");
                    }
                    sbData.append("\n");
                }
                else
                {
                    sbData.append(strLine);
                    sbData.append("\n");
                }
            }
            reader.close();
            BufferedWriter writer = WFileUtil.openWriteFile(strPath);
            WFileUtil.writeAndClose(writer, sbData);
        }
        catch (Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }
    }

    /**
     *  Deletes the user from the pgsql database.
     *  @param strName  the user that needs to be deleted.
     */
    public static WMessage dropPgSqlUser(String strName)
    {
        String sysDir = System.getProperty("sysdir");
        if (sysDir == null)
            sysDir = File.separator+"vnmr";
        if (Util.iswindows())
            sysDir = UtilB.windowsPathToUnix(sysDir);

        String strg = null;

        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, sysDir +
                            "/pgsql/bin/dropuser" + " " + strName };
        WMessage msg = WUtil.runScript(cmd);
        return msg;
    }

    /**
     *  Deletes user completely including the home directory and .del files
     *  in /vnmr/adm/users. This method is called from the trashcan to remove
     *  the user from the system, and clean out all the files and the home directory.
     *  @param strName  name of the user
     *  @param strHome  home directory of the user
     *
     */
    public static boolean deleteUserFully(String strName, String strHome)
    {
        // delete the home directory, if the unix user has already been deleted.
        // This is an error case, if another name with the same login name has been
        // created, then don't delete it's directory.
        if (!isUnixUser(strName) && (Util.OSNAME == null ||
                                     (!Util.OSNAME.startsWith("Mac") && !Util.iswindows())))
        {
            // move the directory being deleted to .del, because rm -r might take some time
            String strHomeDel = strHome + ".del";
            String[] cmdMv = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO +
                               WGlobal.SBIN + "vcmdm " + strHome + " " +
                               strHomeDel + " adMin"};
            WUtil.runScript(cmdMv);
            String[] cmdRm = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO +
                                WGlobal.SBIN + "vcmdr " + strHomeDel + " adMin"};
            WUtil.runScriptInThread(cmdRm);
        }

        // delete the files from /vnmr/adm/users
        String strPath = FileUtil.openPath(USRPROFILE+File.separator+strName+".del");
        if (strPath != null)
            WUtil.deleteFile(strPath);
        strPath = FileUtil.openPath(SYSPROFILE+File.separator+strName+".del");
        if (strPath != null)
            WUtil.deleteFile(strPath);
        strPath = FileUtil.openPath(USRDATADIR+File.separator+strName+".del");
        if (strPath != null)
            WUtil.deleteFile(strPath);
        strPath = FileUtil.openPath(USRTEMPLATE+File.separator+strName+".del");
        if (strPath != null)
            WUtil.deleteFile(strPath);
        strPath = FileUtil.openPath(USRTEMPLATE);
        if (strPath != null)
        {
            File fileobj = new File(strPath);
            String[] files = fileobj.list(new VTemplate(strName+"."));
            int nSize = files.length;
            for (int i = 0; i < nSize; i++)
            {
                strPath = FileUtil.openPath(WUserUtil.USRTEMPLATE+File.separator+files[i]);
                if (strPath != null)
                    WUtil.deleteFile(strPath);
            }
        }
        if (Util.isPart11Sys())
        {
            strPath = FileUtil.openPath(USRPART11DIR+File.separator+strName+".del");
            if (strPath != null)
                WUtil.deleteFile(strPath);
        }

        Messages.postInfo("Deleted user '" + strName + "'.");
        writeAuditTrail(new Date(), strName, "Deleted the user home directory: "
                                    + strHome);

        return true;
    }

    public static boolean deleteOperator(String strOperator)
    {
        String strPath = FileUtil.openPath(OPERATORLIST);
        boolean bOperator = true;
        if (strPath != null)
        {
            bOperator = deleteOperatorfromList(strPath, strOperator);
            bOperator = deleteOperator(strPath, strOperator);
        }
        strPath = FileUtil.openPath(PASSWORD);
        if (strPath != null && bOperator)
            bOperator = deleteOperator(strPath, strOperator);
        return bOperator;
    }

    public static boolean deleteOperatorfromList(String strPath, String strOperator)
    {
        HashMap hmOperator = getOperatorList(strPath);
        boolean bOperator = true;
        String strUsers = (String)hmOperator.get(strOperator);
        if (strUsers == null || strUsers.equals(""))
            return bOperator;

        int nIndex = strUsers.indexOf(';');
        String strlist = strUsers;
        if (nIndex >= 0)
            strlist = strUsers.substring(0, nIndex);
        strlist = strlist.substring(strOperator.length()+1).trim();
        ArrayList alistUsers = WUtil.strToAList(strlist, false, ",");
        //ArrayList aListUsers = WUtil.strToAList(strUsers, false, " ,");

        int nSize = alistUsers.size();
        String strUser = "";
        String strDir = "";
        String strOperators = "";
        for (int i = 0; i < nSize; i++)
        {
            strUser = (String)alistUsers.get(i);
            strDir = WFileUtil.getHMPath(strUser);
            HashMap hmUser = WFileUtil.getHashMap(strDir);
            strOperators = (String)hmUser.get("operators");
            if (strOperators != null)
            {
                ArrayList aListOperators = WUtil.strToAList(strOperators, " ,");
                if (aListOperators.contains(strOperator))
                {
                    aListOperators.remove(strOperator);
                    strOperators = WUtil.aListToStr(aListOperators);
                    hmUser.put("operators", strOperators);
                    WFileUtil.updateItemFile(strDir, hmUser);
                }
            }
        }
        return bOperator;
    }

    public static boolean deleteOperator(String strPath, String strOperator)
    {
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        boolean bOperator = true;
        if (reader == null)
            return false;

        StringBuffer sbOperator = new StringBuffer();
        String strLine;
        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                StringTokenizer strTok = new StringTokenizer(strLine, " \t;");
                String strOperator2 = "";
                if (strTok.hasMoreTokens())
                    strOperator2 = strTok.nextToken();

                if (!strOperator.equals(strOperator2))
                    sbOperator.append(strLine).append("\n");
            }
            reader.close();
            BufferedWriter writer = WFileUtil.openWriteFile(strPath);
            WFileUtil.writeAndClose(writer, sbOperator);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            bOperator = false;
            Messages.writeStackTrace(e);
        }
        return bOperator;
    }

}

class VTemplate implements FilenameFilter
{
    protected String m_str1;
    protected String m_str2;

    public VTemplate(String strkey)
    {
        this(strkey, "");
    }

    public VTemplate(String strkey, String strkey2)
    {
        m_str1 = strkey;
        m_str2 = strkey2;
    }

    public boolean accept(File file, String strfile)
    {
        boolean bShow = strfile.startsWith(m_str1);
        if (m_str2 != null && !m_str2.equals(""))
            bShow = bShow && (strfile.endsWith(m_str2));
        return bShow;
    }
}

