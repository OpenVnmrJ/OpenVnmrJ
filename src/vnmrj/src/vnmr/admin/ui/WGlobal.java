/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.util.*;

import vnmr.util.*;

/**
 * Title: WGlobal
 * Description: This file has the global variables implemented for the Wanda
 *              interface.
 * Copyright:    Copyright (c) 2002
 */

public class WGlobal
{
    public static final String AREA1        = "Area1";
    public static final String AREA2        = "Area2";
    public static final String INFODIR      = "Infodir";
    public static final String BUILD        = "Build";
    public static final String WRITE        = "Write";
    public static final String CLEAR        = "Clear";
    public static final String ALL          = "All";
    public static final String SET_VISIBLE  = "Set visible";
    public static final String SAVEUSER     = vnmr.util.Util.getLabel("_admin_Save_User");
    public static final String SAVEGROUP    = "Save Group";
    public static final String SAVEUSER_NOERROR = "Save User No Error";
    public static final String USER_CHANGE  = "User Change";
    public static final String GROUP_CHANGE = "Group Change";
    public static final String DELETE_USER  = "Delete User";
    public static final String ITEM_LIST    = "Item List";
    public static final String ACTIVATE     = "Activate Account";
    public static final String BGCOLOR      = "BgColor";
    public static final String FONTSCOLORS  = "Fonts Colors";
    public static final String SEPERATOR    = "_";
    public static final String SUDO         = getSudo();
    public static final String SHTOOLCMD    = System.getProperty("shtoolcmd");
    public static final String SHTOOLOPTION = System.getProperty("shtooloption");
    public static final String NAME         = "accname";
    public static final String FULLNAME         = "name";
    public static final String PART11       = "part11";
    public static final String ITEM_AREA1   = "Item" + SEPERATOR + AREA1;
    public static final String ITEM_AREA2   = "Item" + SEPERATOR + AREA2;
    public static final String DETAIL_AREA1 = "Detail" + SEPERATOR + AREA1;
    public static final String DETAIL_AREA2 = "Detail" + SEPERATOR + AREA2;
    public static final String ADMIN_BGCOLOR = "Wanda" + BGCOLOR;
    public static final String PANELLEVEL   = "30";
    public static final String IMGDIR       = FileUtil.sysdir()+File.separator+Global.IMAGING;
    public static final String WALKUPDIR    = FileUtil.sysdir();
    public static final String LCDIR        = FileUtil.sysdir()+File.separator+Global.LC;
    public static final String SBIN         = getSbin();
    public static final String OPT_PER_FILE ="USER/PERSISTENCE/AdminOptions";// persistence file
    public static final String TRASHCAN_FILE = "SYSTEM/PROFILES/trashcan";
    
    /** label for admin interface */
    public static String ADMINIFLBL = Util.getLabel("_Global_Administrative",Global.ADMINIF);
    /** label for walkup interface */
    public static String WALKUPIFLBL = Util.getLabel("_Global_Walkup",Global.WALKUPIF);
    /** label for imaging interface */
    public static String IMGIFLBL = Util.getLabel("_Global_Imaging",Global.IMGIF);
    /** label for lc interface */
    public static String LCIFLBL = Util.getLabel("_Global_LCIF",Global.LCIF);

    /** The array of requried fields for a user.  */
    public static final String[] USR_REQ_FIELDS = {NAME, "home", "userdir",
                                                        "sysdir", "datadir", "itype"};
                                                        //"sysdir", "datadir", "appdir", "itype"};
    public static final String[] USR_PART11_REQ_FIELDS = {NAME, "home", "name", "userdir",
                                                            "sysdir", "datadir", "itype"};
                                                            //"sysdir", "datadir", "appdir", "itype"};
    public static final String[] USR_REQ_FIELDS_WIN = {NAME, "home", "userdir", "sysdir", 
                                                        "datadir", "itype"};
    public static final String[] USR_PART11_REQ_FIELDS_WIN = {NAME, "home", "name", 
                                                                "userdir", "sysdir", 
                                                                "datadir", 
                                                                "itype"};     

                     
    public static String getPropName(String strLabel)
    {
        String strPropName = strLabel;
        String strTmp = strLabel.toLowerCase();

        if (strTmp.startsWith("fonts"))
            strPropName = FONTSCOLORS;

        return strPropName;
    }
    
    public static String getSbin()
    {
        String strSbin = "/vnmr/p11/sbin/";
        if (Util.iswindows())
        {
           strSbin = "/vnmr/bin/";
        }
        else
        {
           UNFile file = new UNFile(strSbin);
           if ( !file.exists() )
              strSbin = FileUtil.sysdir()+"/bin/";
        }

        return strSbin;
    }
    
    public static String getSudo()
    {
        // Default to /usr/bin
        String strSudo = "/usr/bin/sudo ";

        if (Util.iswindows())
            strSudo = "";

        UNFile file = new UNFile("/usr/bin/sudo");
        if(!file.exists()) {
            // If no /usr/bin/sudo, then try /usr/local/bin
            file = new UNFile("/usr/local/bin/sudo");
            if(file.exists())
                strSudo = "/usr/local/bin/sudo "; 
        }
        //Messages.postDebug("SUDO path=" + strSudo);
        return strSudo;
    }

    private WGlobal()
    {
        // not meant to be used.
    }
}
