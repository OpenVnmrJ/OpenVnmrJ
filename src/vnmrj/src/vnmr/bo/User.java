/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.io.*;
import java.util.*;

import vnmr.ui.shuf.FillDBManager;
import vnmr.util.*;

/**
 * This item gives basic information about a user.  We can probably
 * assume that the 'account' attribute is unique.
 *
 */
public class User {
    /** account name for specific user */
    private String accountName = null;
    /** application type for this user */
    private String appType = null;
    private String iType = null;
    /** user's full name */
    /** password from userList file */
    private String passWord = null;
    private String fullName = null;
    /** List of directories containing nmr data for the shuffler
        The first one is where data should be saved to. */
    private ArrayList dataDirectories = null;
    private ArrayList dataDirsNotConanical = null;
    /** users p11 directory list */
    private ArrayList p11Directories = null;
    /** user's vnmrsys directory */
    private String vnmrsysDir = null;
    private int userLevel = 2;
    private String vnmrDir = null;
    private String m_currOperatorName = null;

    private ArrayList appDirectories = null;
    private ArrayList appDirLabels = null;
    private ArrayList ownedDirectories = null;
    private ArrayList accessList = null;
    private String homeDir = null;
    //    private String m_cmdArea = null;
    /** list of Application type Strings (e.g. Walkup, Imaging, LC-NMR)  */
    private ArrayList appTypes = new ArrayList();
    /** list of Application type file extensions (e.g. walkup, imaging, lc) */
    private ArrayList<String> appTypeExts = new ArrayList<String>();
    private ArrayList operators = new ArrayList();

    /** constructor */
    public User(String accountName, int userLevel, String fullName,
            String passWord, ArrayList dataDirectories,
            ArrayList dataDirsNotConanical, String vnmrsysDir,
            ArrayList p11Directories) {

        this.accountName = accountName.trim();
        this.userLevel = userLevel;
        this.fullName = fullName.trim();
        this.passWord = passWord.trim();
        this.dataDirectories = dataDirectories;
        this.dataDirsNotConanical = dataDirsNotConanical;
        this.p11Directories = p11Directories;
        this.vnmrsysDir = vnmrsysDir.trim();
        this.appDirectories = new ArrayList();
        this.appDirLabels = new ArrayList();

        homeDir = new StringBuffer().append("/export/home/")
                .append(accountName).toString();

        ownedDirectories = new ArrayList();
        ownedDirectories.add(homeDir);

        accessList = new ArrayList();
        homeDir = new StringBuffer().append("/export/home/")
                .append(accountName).toString();

        setAppType(this, userLevel);

    }

    /** constructor */
    public User(String accountName) {
        this.accountName = accountName.trim();
        appDirectories = new ArrayList();
        appDirLabels = new ArrayList();
        ownedDirectories = new ArrayList();
        accessList = new ArrayList();
        dataDirectories = new ArrayList();
        p11Directories = new ArrayList();
        dataDirsNotConanical = new ArrayList();
    }

    /** get user's account name */
    public String getAccountName() {
        return accountName;
    }

    /** get account type */
    public String getAppType() {
        return appType;
    }

    private void setIType(String type) {
        // The "Walkup" and "Imaging" values here need to match the strings
        // used in userlist.xml for "keyval" when the radio buttons are
        // selected in the admin panel.
        if(type.equals("Walkup"))
           iType = Global.WALKUPIF;
        else if (type.equals("Imaging"))
           iType = Global.IMGIF;
        else
            iType = type;
    }

    public String getIType() {
        return iType;
    }

    /** set account type */
    private static void setAppType(User user, int lvl) {
        user.userLevel = lvl;
        if (lvl <= 3)
            user.appType = Global.WALKUPIF;
        else
            user.appType = Global.WALKUPIF;
    }

    public ArrayList getAppTypes() {
        // For compatability with vnmr_jadmin, add the appType
        // to the appTypes arraylist. vnmr_jadmin sets the appType
        // with the access level, whereas WandaIF have check boxes
        // for the appType and can select more than one appType.
        // So until we fully switch to WandaIF for adding users,
        // add the appType to the list.
        if (appTypes.isEmpty())
            appTypes.add(appType);

        return appTypes;
    }

    public String getAppTypesStr() {
        return aListToStr(appTypes);
    }

    /** get user's given (first) name */
    public String getFullName() {
        return fullName;
    }

    /** get user's data directory list. */
    public ArrayList getDataDirectories() {
        return dataDirectories;
    }

    /** get user's data directory list, but not in conanical form. */
    public ArrayList getDataDirsNotConanical() {
        return dataDirsNotConanical;
    }

    /** get user's p11 directory list. */
    public ArrayList getP11Directories() {
        return p11Directories;
    }

    public void setCurrOperatorName(String strOperator) {
        m_currOperatorName = strOperator;
    }

    public String getCurrOperatorName() {
        return m_currOperatorName;
    }

    /* called by ExpPanel's processRebuild() */
    public boolean setAppDirs(ArrayList<String> appDirs, 
                              ArrayList<String> labels) {

        boolean bChanged = false;

        // test for change
        if (appDirectories.size() != appDirs.size())
            bChanged = true;
        else
            for (int i = 0; i < appDirs.size(); i++) {
                if (!((String) appDirectories.get(i)).equals((String) appDirs
                        .get(i)))
                    bChanged = true;
            }

        if (!bChanged)
            return bChanged;

        // update appDirectories
        appDirectories.clear();
        appDirLabels.clear();
        for (int i = 0; i < appDirs.size(); i++) {
            appDirectories.add((String) appDirs.get(i));
            appDirLabels.add((String) labels.get(i));
        }

        // update FileUtil
        FileUtil.setAppDirs(appDirectories, appDirLabels);

        return bChanged;
    }

    /** get user's app directory list. */
    public ArrayList getAppDirectories() {
        // If we are running from managedb, the appdirs may not have
        // been filled yet.
        if(FillDBManager.managedb && appDirectories.size() == 0) {
            appDirectories = FileUtil.getAppDirs();
            appDirLabels = FileUtil.getAppDirLabels();
        }
        return appDirectories;
    }

    /** get user's app directory labels. */
    public ArrayList getAppLabels() {
        return appDirLabels;
    }

    /** get list of directories "owned" by user. */
    public ArrayList getOwnedDirectories() {
        return ownedDirectories;
    }

    public ArrayList<String> getOperators() {
        return operators;
    }

    //    public String getCommandArea() {
    //        if (m_cmdArea == null)
    //            m_cmdArea = "yes";
    //        return m_cmdArea;
    //    }

    /** get user's access list. */
    public ArrayList getAccessList() {
        return accessList;
    }

    /** get user's vnmrsys directory. */
    public String getVnmrsysDir() {
        return vnmrsysDir;
    }

    /** get user's home directory. */
    public String getHomeDir() {
        return homeDir;
    }

    /** display user data. */
    public String toString() {
        int i;
        String strToken = " ";
        if (UtilB.OSNAME.startsWith("Windows"))
            strToken = ";";
        StringBuffer sbData = new StringBuffer().append("User          ")
                .append(accountName).append("\n");

        sbData.append("fullName      ").append(fullName).append("\n");
        sbData.append("appType       ").append(appType).append("\n");
        sbData.append("userLevel     ").append(userLevel).append("\n");
        sbData.append("vnmrsysDir    ").append(vnmrsysDir).append("\n");
        sbData.append("vnmrDir       ").append(vnmrDir).append("\n");
        sbData.append("homeDir       ").append(homeDir).append("\n");
        //        sbData.append("commandArea   ").append(m_cmdArea).append("\n");

        if (appTypes.size() > 0) {
            sbData.append("InterfaceTypes    ");
            for (i = 0; i < appTypes.size(); i++) {
                sbData.append(appTypes.get(i)).append(" ");
            }
            sbData.append("\n");
        }
        if (dataDirectories.size() > 0) {
            sbData.append("dataDirs      ");
            for (i = 0; i < dataDirectories.size(); i++)
                sbData.append(dataDirectories.get(i)).append(strToken);
            sbData.append("\n");
        }
        if (p11Directories.size() > 0) {
            sbData.append("p11Dirs      ");
            for (i = 0; i < p11Directories.size(); i++)
                sbData.append(p11Directories.get(i)).append(strToken);
            sbData.append("\n");
        }
        if (appDirectories.size() > 0) {
            sbData.append("appDirs       ");
            for (i = 0; i < appDirectories.size(); i++)
                sbData.append(appDirectories.get(i)).append(strToken);
            sbData.append("\n");
        }
        if (ownedDirectories.size() > 0) {
            sbData.append("ownedDirs     ");
            for (i = 0; i < ownedDirectories.size(); i++)
                sbData.append(ownedDirectories.get(i)).append(strToken);
            sbData.append("\n");
        }
        if (operators.size() > 0) {
            sbData.append("operators     ");
            for (i = 0; i < operators.size(); i++)
                sbData.append(operators.get(i)).append(" ");
            sbData.append("\n");
        }
        if (accessList.size() > 0) {
            sbData.append("accessList    ");
            for (i = 0; i < accessList.size(); i++)
                sbData.append(accessList.get(i)).append(" ");
            sbData.append("\n");
        }
        return (sbData.toString());
    }

    // build the userList file.

    public static Hashtable readuserListFile() {
        HashArrayList hash = new HashArrayList();
        String path;
        path = FileUtil.openPath("SYSTEM/USRS/userlist");
        if (path != null)
            readSystemProfiles(path, hash);
        else {
            path = FileUtil.openPath("SYSTEM/USRS/userList");
            if (path != null)
                readUserListFile(path, hash);
        }
        readUserProfiles(hash);
        if (DebugOutput.isSetFor("userListfile")) {

            Enumeration users = hash.elements();
            while (users.hasMoreElements()) {
                User user = (User) users.nextElement();
                Messages.postDebug(user.toString());
            }
        }
        return hash;
    }

    // read the system profile files.

    private static void readSystemProfiles(String path, Hashtable userHash) {
        BufferedReader in;
        String inLine;
        HashArrayList hash = new HashArrayList();
        String owner;
        String dir;
        StringTokenizer tok;
        int userLevel = 2;

        // read list of owners from ownerList and owners/

        try {
            UNFile file = new UNFile(path);
            in = new BufferedReader(new FileReader(file));

            while ((inLine = in.readLine()) != null) {
                if (inLine.startsWith("#"))
                    continue;
                tok = new StringTokenizer(inLine);
                while (tok.hasMoreTokens()) {
                    owner = tok.nextToken();
                    dir = FileUtil.openPath(new StringBuffer()
                            .append("SYSPROF").append(File.separator).append(
                                    owner).toString());

                    if (dir != null)
                        hash.put(owner, dir);
                }
            }
            in.close();
            int nSize = hash.size();
            for (int i = 0; i < nSize; i++) {
                String name = (String) hash.getKey(i);
                dir = (String) hash.get(i);
                Properties props = new Properties();
                UNFile file2 = new UNFile(dir);
                InputStream inputStream;
                if (UtilB.iswindows()) {
                    byte[] data = UtilB.setForwardSlashes(file2.getPath())
                            .getBytes();
                    inputStream = new ByteArrayInputStream(data);
                } else
                    inputStream = new FileInputStream(file2);
                props.load(inputStream);
                inputStream.close();
                User user = new User(name);

                String val = (String) props.get("username");
                if (val != null)
                    user.fullName = val.trim();
                else
                    user.fullName = name;
                val = (String) props.get("home");

                if (val != null) {
                    if (UtilB.OSNAME.startsWith("Windows")) {
                        UNFile file3 = new UNFile(val.trim());
                        if (file3 != null) {
                            user.homeDir = file3.getCanonicalPath();
                        }
                    } else
                        user.homeDir = val.trim();
                } else
                    user.homeDir = new StringBuffer().append("/export/home/")
                            .append(name).toString();

                readProfiles(props, user);
                setAppType(user, userLevel);
                userHash.put(name, user);
            }
        } catch (Exception e) {
            Messages.postError(new StringBuffer().append(
                    "Problem reading owner files ").append(path).toString());
            Messages.writeStackTrace(e);
        }
    }

    // read the user profile files.

    public static void readUserProfiles(Hashtable userHash) {

        Enumeration users = userHash.elements();
        boolean bwindows = UtilB.OSNAME.startsWith("Windows");
        while (users.hasMoreElements()) {
            User user = (User) users.nextElement();
            String name = user.getAccountName();

            String ppath = FileUtil.openPath(new StringBuffer().append(
                    "USRPROF").append(File.separator).append(name).toString());
            try {
                if (ppath == null) { // make default profile
                    user.vnmrsysDir = new StringBuffer().append(user.homeDir)
                            .append(File.separator).append("vnmrsys")
                            .toString();
                    user.vnmrDir = UtilB.addWindowsPathIfNeeded(FileUtil
                            .sysdir());
                    String fDir = new StringBuffer().append(user.vnmrsysDir)
                            .append(File.separator).append("data").toString();
                    if (bwindows)
                        fDir = UtilB.unixPathToWindows(fDir);
                    UNFile file = new UNFile(fDir);
                    user.dataDirsNotConanical.add(fDir);

                    String cDir = file.getCanonicalPath();
                    user.dataDirectories.add(cDir);

                    user.p11Directories.add(new StringBuffer().append(
                            user.vnmrsysDir).append(File.separator).append(
                            "data").append(File.separator).append("p11")
                            .toString());
                    /*
                     user.appDirectories.add(user.vnmrsysDir);
                     user.appDirectories.add(user.vnmrDir);
                     */
                    continue;
                }
            } catch (Exception e) {
                Messages.postError("Problem reading profile for " + name);
                Messages.writeStackTrace(e);
            }
            // Read this users profile file into props.
            Properties props = new Properties();
            try {
                UNFile file = new UNFile(ppath);
                InputStream inputStream;
                if (UtilB.iswindows()) {
                    byte[] data = UtilB.setForwardSlashes(file.getPath())
                            .getBytes();
                    inputStream = new ByteArrayInputStream(data);
                } else
                    inputStream = new FileInputStream(file);
                props.load(inputStream);
                inputStream.close();
                readProfiles(props, user);
            } catch (Exception e) {
                Messages.postError("Problem reading profile for " + name);
                Messages.writeStackTrace(e);
            }
        }
    }

    /** set appTypeExts type */
    private void setAppTypeExts(String name) {
        if (name.equals(Global.IMGIF))
            appTypeExts.add(Global.IMAGING);
        else if (name.equals(Global.LCIF))
            appTypeExts.add(Global.LC);
    }

    /** set appTypeExts type */
    public ArrayList getAppTypeExts() {
        return appTypeExts;
    }

    private static int readProfiles(Properties props, User user) {
        String dir;
        StringTokenizer tok;
        int userLevel = 2;
        ArrayList canonList;
        boolean bwindows = UtilB.OSNAME.startsWith("Windows");

        String val = (String) props.get("userdir");
        if (val != null)
            user.vnmrsysDir = val;
        val = (String) props.get("sysdir");
        if (val != null)
            user.vnmrDir = val;
        val = (String) props.get("datadir");
        try {
            if (val != null) {
                user.dataDirectories = new ArrayList();
                user.dataDirsNotConanical = new ArrayList();
                if (bwindows)
                    tok = new StringTokenizer(val, ";");
                else
                    tok = new StringTokenizer(val);
                String strVnmrdir = UtilB.SFUDIR_WINDOWS + "vnmr"
                        + File.separator;
                while (tok.hasMoreTokens()) {

                    dir = tok.nextToken().trim();

                    // Disallow the root directory
                    if (dir.length() <= 1) {
                        continue;
                    }
                    // Disallow the user's home directory.  That causes the 
                    // DB to look into too many extraneous dirs.
                    String userdir = user.getHomeDir();
                    if(dir.equals(userdir)) {
                        continue;
                    }


                    if (bwindows)
                        dir = UtilB.unixPathToWindows(dir);
                    if (dir.startsWith("/") || dir.startsWith(File.separator)
                            || (bwindows && dir.indexOf(':') == 1)) {
                        if (bwindows && dir.startsWith(strVnmrdir))
                            dir = FileUtil.sysdir() + File.separator
                                    + dir.substring(strVnmrdir.length());

                        user.dataDirsNotConanical.add(dir);
                        UNFile file = new UNFile(dir);

                        String cDir = file.getCanonicalPath();
                        
                        user.dataDirectories.add(cDir);
                    } else {
                        String fDir = new StringBuffer()
                                .append(user.vnmrsysDir).append(File.separator)
                                .append(dir).toString();

                        user.dataDirsNotConanical.add(fDir);
                        UNFile file = new UNFile(fDir);

                        String cDir = file.getCanonicalPath();
                        user.dataDirectories.add(cDir);
                    }
                }
            }
        } catch (Exception e) {
            Messages.postError("Problem getting datadir");
            Messages.writeStackTrace(e);
        }
        // System Profiles
        val = (String) props.get("passwrd");
        if (val != null)
            user.passWord = val.trim();
        val = (String) props.get("usrlvl");
        if (val != null)
            userLevel = Integer.valueOf(val.trim()).intValue();
        val = (String) props.get("itype");
        if (val != null) {
            user.setIType(val);

            tok = new StringTokenizer(val, "~");
            if (tok.hasMoreTokens()) {
                while (tok.hasMoreTokens()) {
                    dir = tok.nextToken();
                    user.appTypes.add(dir);
                }
            }
            if (val.trim().indexOf(Global.ADMINIF) < 0)
                userLevel = 2;
            else
                userLevel = 4;
            setAppType(user, userLevel);
        }
        String itype = val;
        val = (String) props.get("apptype");
        if (val != null) {
            user.appTypeExts.clear();
            tok = new StringTokenizer(val);
            if (tok.hasMoreTokens()) {
                while (tok.hasMoreTokens()) {
                    dir = tok.nextToken();
                    user.appTypeExts.add(dir);
                }
            }
        } else if (itype != null) {
            // the "apptype" property needs to be supported by vnmrj-adm
            // in the same way that appdir is. In the meantime though, just
            // return an appropriate value based on "itype"  
            user.appTypeExts.clear();
            user.setAppTypeExts(itype);
        }
        val = (String) props.get("operators");
        if (val != null) {
            tok = new StringTokenizer(val);
            while (tok.hasMoreTokens()) {
                dir = tok.nextToken();
                user.operators.add(dir);
            }
        }
        val = (String) props.get("dataacq");
        if (val != null) {
        }
        val = (String) props.get("acqerror");
        if (val != null) {
        }
        val = (String) props.get("owned");
        if (val != null) {
            if (bwindows)
                tok = new StringTokenizer(val, ";");
            else
                tok = new StringTokenizer(val);
            while (tok.hasMoreTokens()) {
                dir = tok.nextToken().trim();
                user.ownedDirectories.add(dir);
            }
        } else
            user.ownedDirectories.add(user.homeDir);
        //        val = (String) props.get("cmdArea");
        //        if (val != null)
        //            user.m_cmdArea = val;

        val = (String) props.get("access");
        if (val != null) {
            tok = new StringTokenizer(val);
            while (tok.hasMoreTokens()) {
                dir = tok.nextToken();
                user.accessList.add(dir);
            }
        }

        // profiles/data
        // This seems like a bit of a kluge, but user data directories
        // current exist in two places.  One is the 'datadir' line in
        // /vnmr/adm/users/profiles/user/$USER which is obtained above
        // to fill user.dataDirectories .  The other place is in
        // /vnmr/adm/users/profiles/data/$USER where each line of the
        // file contains a label followed by a ':' followed by a directory.
        // We will now add those directories to the ones already obtained
        // above into user.dataDirectories
        String userDataDirs = FileUtil.openPath(new StringBuffer().append(
                "PROFILES").append(File.separator).append("data").append(
                File.separator).append(user.accountName).toString());
        if (userDataDirs != null) {
            try {
                BufferedReader in;
                String inLine;
                UNFile file = new UNFile(userDataDirs);
                in = new BufferedReader(new FileReader(file));
                // Read one line at a time.
                while ((inLine = in.readLine()) != null) {

                    if (inLine.length() > 1 && !inLine.startsWith("#")) {
                        inLine.trim();
                        // Create a Tokenizer object to work with inc ':'
                        if (inLine.indexOf(File.pathSeparator) == -1)
                            tok = new StringTokenizer(inLine, " ,\n", false);
                        else
                            tok = new StringTokenizer(inLine,
                                    File.pathSeparator + ",\n", false);
                        if (tok.countTokens() > 1) {
                            // Dump the name for this directory
                            tok.nextToken();
                            // Get the directory portion
                            dir = tok.nextToken().trim();

                            file = new UNFile(dir);
                            String cDir = file.getCanonicalPath();
                            // Disallow the root directory
                            if (cDir.length() <= 1) {
                                Messages.postWarning("Skipping \'" + cDir
                                        + "\'.  Check " + userDataDirs);
                                continue;
                            }

                            // Go through the items already in the list
                            // and try to avoid subdirectories of parents
                            // in the list.  ie.,  just keep the highest
                            // level in the list because we fill the
                            // DB from the list recursively.
                            String dirInList;
                            boolean addIt = true;
                            for (int i = 0; i < user.dataDirectories.size(); i++) {
                                dirInList = (String) user.dataDirectories
                                        .get(i);

                                // If we have this parent or dup, don't add
                                if (cDir.startsWith(dirInList))
                                    addIt = false;
                                // If the new one is a parent, remove the
                                // current item and add the new one.
                                else if (dirInList.startsWith(cDir)) {
                                    user.dataDirectories.remove(i);
                                    user.dataDirectories.add(cDir);
                                    addIt = false;
                                    // do dataDirsNotConanical also
                                    user.dataDirsNotConanical.remove(i);
                                    user.dataDirsNotConanical.add(dir);

                                }
                            }
                            if (addIt) {
                                // Must be unique, add it
                                user.dataDirectories.add(cDir);
                                user.dataDirsNotConanical.add(dir);
                            }
                        }
                    }
                }
                in.close();
            } catch (Exception e) {
                Messages.postError(new StringBuffer().append(
                        "Problem reading user dir ").append(userDataDirs)
                        .toString());
                Messages.writeStackTrace(e);
            }
        }

        // regardless of where directories in dataDirectories came from
        // we need to eliminate duplicated and children of parents.
        String diri;
        String dirk;

        // Start by converting all paths to canonical paths so we don't
        // end up with two symbolic links pointing to the same place
        // or something.
        UNFile file;
        canonList = new ArrayList();
        String canonPath;
        for (int i = 0; i < user.dataDirectories.size(); i++) {
            diri = (String) user.dataDirectories.get(i);

            file = new UNFile(diri);

            try {
                canonPath = file.getCanonicalPath();
                canonList.add(canonPath);
            } catch (Exception e) {
                Messages.writeStackTrace(e);
            }
        }
        // replace dataDirectories with canonList
        user.dataDirectories = canonList;

        for (int i = 0; i < user.dataDirectories.size(); i++) {
            diri = (String) user.dataDirectories.get(i);
            for (int k = 0; k < user.dataDirectories.size(); k++) {
                // Obviously don't compare with itself
                if (k == i)
                    continue;
                dirk = (String) user.dataDirectories.get(k);
                if (dirk.startsWith(diri)) {
                    user.dataDirectories.remove(k);
                    user.dataDirsNotConanical.remove(k);
                    if (k <= i)
                        i--;
                    break;
                }
                if (diri.startsWith(dirk)) {
                    user.dataDirectories.remove(i);
                    user.dataDirsNotConanical.remove(i);
                    i--;
                    break;
                }
            }
        }

        // profiles/p11
        // It appears that the files in p11 (by users names) contain
        // directory information in the same format as profiles/data
        // just above.  These are both different in format than the
        // other directories specified in in profiles like profile/user
        // So, I will just parse the p11 just as the profiles/data above
        // and not try to follow the system that calls this method.
        String p11DataDirs = FileUtil.openPath(new StringBuffer().append(
                "PROFILES").append(File.separator).append("p11").append(
                File.separator).append(user.accountName).toString());

        user.p11Directories = new ArrayList();

        if (p11DataDirs != null) {
            try {
                BufferedReader in;
                String inLine;
                file = new UNFile(p11DataDirs);
                in = new BufferedReader(new FileReader(file));
                // Read one line at a time.
                while ((inLine = in.readLine()) != null) {
                    if (inLine.length() > 1 && !inLine.startsWith("#")) {
                        inLine.trim();
                        // Create a Tokenizer object to work with inc ':'
                        if (inLine.indexOf(File.pathSeparator) == -1)
                            tok = new StringTokenizer(inLine, " ,\n", false);
                        else
                            tok = new StringTokenizer(inLine,
                                    File.pathSeparator + ",\n", false);
                        if (tok.countTokens() > 1) {
                            // skip the label
                            tok.nextToken();
                            // Get the directory
                            dir = tok.nextToken();
                            user.p11Directories.add(dir);
                        }
                    }
                }
                in.close();
            } catch (Exception e) {
                Messages.postError(new StringBuffer().append(
                        "Problem reading p11 dir ").append(p11DataDirs)
                        .toString());
                Messages.writeStackTrace(e);
            }
        }

        if (DebugOutput.isSetFor("UserProfile")) {
            Messages.postDebug(new StringBuffer().append("User Object:\n")
                    .append(user.toString()).toString());

        }
        return userLevel;
    }

    // Open, read and parse the (oldstyle) userList file.

    private static void readUserListFile(String path, Hashtable userHash) {
        BufferedReader in;
        String inLine;
        String filepath;
        String colon;
        String string;
        String dir;
        String accountName, passWord, fullName, dataDirectory;
        String vnmrsysDirectory;
        ArrayList dataDirectories, dataDirsNotConanical;
        int userLevel;
        StringTokenizer tok, dirtok;
        User user;

        String Usage = new StringBuffer().append(
                "syntax:\n  username:passwd:Full Name:Vnmrsys dir:Data dir, ")
                .append("Data dir, ...:user level\n").toString();

        filepath = FileUtil.openPath(path);
        if (filepath == null)
            filepath = "";

        boolean bwindows = UtilB.OSNAME.startsWith("Windows");

        // Open the userList file.
        try {
            UNFile file = new UNFile(filepath);
            in = new BufferedReader(new FileReader(file));
        } catch (Exception e) {
            Messages.postError(new StringBuffer().append("The file ").append(
                    filepath).append(
                    "\nmust exist, and contain all of the user who are ")
                    .append("authorized to login to vnmrj.").append(
                            "\nThe format of the file is: \n    " + Usage)
                    .toString());
            Messages.writeStackTrace(e);
            return;
        }
        try {
            // Read one line at a time.
            while ((inLine = in.readLine()) != null) {
                // Catch lines with backslash, and convert to ','
                // That is, the list can be delineated with ',' or '\'
                while (inLine != null && inLine.endsWith("\\")) {
                    String str = in.readLine();
                    // Be sure it is not null.  If it is, bad formated file.
                    if (str == null) {
                        Messages.postError(new StringBuffer().append(
                                "Problem with syntax following ").append(
                                "backslash in ").append(filepath).append(
                                "\n    ").append(Usage).toString());
                        continue;
                    } else {
                        inLine = inLine.concat(str);
                    }
                }
                inLine = inLine.replace('\\', ','); // convert '\' to ','
                if (inLine.length() > 1 && !inLine.startsWith("#")) {
                    inLine.trim();
                    // Create a Tokenizer object to work with including ':'
                    tok = new StringTokenizer(inLine, ":", true);

                    // Get each token and put it where it belongs.
                    accountName = tok.nextToken();

                    colon = tok.nextToken();
                    // Be sure it was a :
                    if (!colon.equals(":")) {
                        Messages.postError(new StringBuffer().append(
                                "Problem with syntax in ").append(filepath)
                                .append("\n    ").append(Usage).toString());
                        continue;
                    }
                    string = tok.nextToken();
                    if (string.equals(":"))
                        // If password was not entered, create an empty string.
                        passWord = "";
                    else {
                        passWord = string;

                        colon = tok.nextToken();
                        // Be sure it was a :
                        if (!colon.equals(":")) {
                            Messages.postError(new StringBuffer().append(
                                    "Problem with syntax in ").append(filepath)
                                    .append("\n    ").append(Usage).toString());

                            continue;
                        }
                    }

                    string = tok.nextToken();

                    if (string.equals(":"))
                        fullName = "";
                    else {
                        fullName = string;

                        colon = tok.nextToken();
                        // Be sure it was a :
                        if (!colon.equals(":")) {
                            Messages.postError(new StringBuffer().append(
                                    "Problem with syntax in ").append(filepath)
                                    .append("\n    ").append(Usage).toString());
                            continue;
                        }
                    }

                    string = tok.nextToken();
                    if (string.equals(":"))
                        vnmrsysDirectory = "";
                    else {
                        vnmrsysDirectory = string.trim();

                        colon = tok.nextToken();
                        // Be sure it was a :
                        if (!colon.equals(":")) {
                            Messages.postError(new StringBuffer().append(
                                    "Problem with syntax in ").append(filepath)
                                    .append("\n    ").append(Usage).toString());
                            continue;
                        }
                    }

                    dataDirectories = new ArrayList();
                    dataDirsNotConanical = new ArrayList();

                    string = tok.nextToken();
                    if (!string.equals(":")) {
                        // dataDirectory will now be all directories listed.
                        dataDirectory = string;
                        // We need to break this up into the separate dirs.
                        dirtok = new StringTokenizer(dataDirectory, ",");
                        while (dirtok.hasMoreTokens()) {
                            dir = dirtok.nextToken().trim();
                            // Be sure something is there.
                            if (dir.length() > 0) {
                                // If the data directory starts with '/'
                                // then take it as it is.  Else, prepend
                                // the vnmrsysDirectory.
                                if (bwindows)
                                    dir = UtilB.unixPathToWindows(dir);
                                if (dir.startsWith("/")
                                        || dir.startsWith(File.separator)
                                        || (bwindows && dir.indexOf(':') == 1)) {
                                    UNFile file = new UNFile(dir);
                                    dataDirsNotConanical.add(dir);
                                    String cDir = file.getCanonicalPath();
                                    dataDirectories.add(cDir);
                                } else {
                                    String fDir = new StringBuffer().append(
                                            vnmrsysDirectory).append(
                                            File.separator).append(dir)
                                            .toString();
                                    UNFile file = new UNFile(fDir);
                                    dataDirsNotConanical.add(fDir);
                                    String cDir = file.getCanonicalPath();
                                    dataDirectories.add(cDir);
                                }
                            }
                        }
                        dataDirectories.trimToSize();
                        dataDirsNotConanical.trimToSize();

                        try {
                            colon = tok.nextToken();
                        } catch (Exception e) {
                            Messages.postMessage(Messages.OTHER
                                    | Messages.ERROR | Messages.MBOX
                                    | Messages.STDERR | Messages.LOG,
                                    new StringBuffer().append(
                                            "Missing entry in ").append(
                                            filepath).append("\n    ").append(
                                            Usage).append("\n ** Exiting **")
                                            .toString());
                            Messages.writeStackTrace(e);
                            System.exit(0);
                        }
                        // Be sure it was a :
                        if (!colon.equals(":")) {
                            System.out.println(Usage);
                            continue;
                        }
                    }

                    string = tok.nextToken();
                    userLevel = Integer.valueOf(string).intValue();

                    ArrayList p11Dir = new ArrayList();
                    // Create an User object with all the info.
                    user = new User(accountName, userLevel, fullName, passWord,
                            dataDirectories, dataDirsNotConanical,
                            vnmrsysDirectory, p11Dir);
                    // Add one to the userHash Hashtable.
                    userHash.put(accountName, user);
                }
            }

        } catch (IOException e) {
        }

        // If no users in the file, tell someone about the problem.
        if (userHash.size() < 1) {
            Messages.postError(new StringBuffer().append("The file ").append(
                    filepath).append("\nexist, but contains no users. ")
                    .append("It must contain all of the user who are ").append(
                            "\nauthorized to login to vnmrj.").append(
                            "The format of the file is: \n").append(
                            "  username:passwd:Long Name:directory:user level")
                    .toString());
        }

        try {
            in.close();
        } catch (IOException e) {
        }
    }

    /**
     *  Takes an arraylist and returns a string.
     */
    public String aListToStr(ArrayList aList) {
        StringBuffer sbLine = new StringBuffer();

        if (aList != null) {
            for (int i = 0; i < aList.size(); i++) {
                sbLine.append(" ");
                sbLine.append(aList.get(i));
            }
        }
        return sbLine.toString();
    }

} // class User

