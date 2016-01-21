/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.net.*;
import java.io.*;
import java.util.*;
import java.sql.*;
import java.text.*;


import vnmr.templates.*;
import vnmr.bo.*;
import vnmr.ui.SessionShare;
import vnmr.util.*;

//import postgresql.util.*;



/********************************************************** <pre>
 * Summary: Utilities to start and communicate with Postgres.
 * @author  Glenn Sullivan
 </pre> **********************************************************/


/*********************************************************************
    The network DB  Conversion

    The following refer to the args for the listed methods.
    Direct means the path as it is on the machine where it really is.

    - Code names containing "d", like dhost and dpath, are direct info
      Code names containing "m", like mhost and mpath, are local mounted info

    - getFileListingRecursive()  Needs local mount path

    - 'study' and 'automation' attributes, must be direct so that the search can
      find the right ones, and the studies and automations themselves will be
      under the direct path names.

    - isDBKeyUniqueAndUpToDate() must have direct for hostFullpath(key),
      but MUST use mounted path for fullpath arg.  That is because it looks
      at the date on the file 'fullpath', but looks in the DB for hostFullpath.

    - isDBKeyUnique() needs direct

    - Calling addXXXFileToDB() must use mount path because those routines
      must actually open the files. Then those routines must use mpath and mhost
      in the appropriate spots.
      When something like AddAutodirToDB then calls addVnmrFileToDB it must
      call addVnmrFileToDB with local mounted path.

    - removeEntryFromDB needs all direct, it only accesses the DB

    - setNonTagAttributeValue() needs local mount info

    - isAttributeInDB() does not matter

    - getAttributeValue() Must be passed local mount path

    - addRowToDBInit()  must use direct for hostFullpath(key), but MUST use
      mounted path for fullpath arg.  That is because it looks at the date
      on the file 'fullpath', but looks in the DB for hostFullpath.

    - addRowToDBSetAttr() must receive direct (key)

    - FillATable() needs local mount directory unless setting an attr, then
      the last arg may be direct.

    - setAttributeValue() host and fullpath need to be mount for accessing
      the file directly.



    Network DB Use

    - In every users account, the env variable PGHOST needs to be set to
      the host name of the machine to use for the DB server.  If this env
      variable is not set, then it defaults to the local host.
      Therefore, it is possible for different users to use different DB
      servers or the local host as the server.
    - The computer which is to be a DB server will need the postgres postmaster
      daemon running on it.  Even if it is running, if PGHOST is set to a
      remote computer, the remote computer will be used.
    - The port will default to 5432 unless specified otherwise in every users
      .login with the env variable, PGPORT.  Changing the port should not
      normally be necessary, but might be done if more than one postgres
      daemon is to run on the system at one time.  It should be possible for
      different users (and thus different groups) to use entirely different
      database daemons and databases on the same computer.
    - Non DB server computers will be determined to be using network mode
      DB access by comparing PGHOST with the local host name.  If a remote
      host is specified, then of course it knows to go into network mode.
      The DB server computer will have itself as PGHOST, so all users on
      the server computer needs to have the env variable PGNETWORK_SERVER
      set to 'true' to force it into network mode.  If this is not done,
      access from other computers will not work correctly.  Setting this
      variable on non server computer will not hurt anything, it just
      forces them into network mode which they may already be in.
    - The env variable settings needs to be in place before the dbsetup
      is run during installation.  Else, the DB will not be set up correctly.
    - When a computer is set up for network mode, the attribute 'hostdest'
      in the database will contain the host name where the file actually
      resides (it destination).  The attribute 'host_fullpath' will contain
      hostdest:fullpath where fullpath will be the path as it looks on the
      machine, hostdest.  It will not show the mounted path from the current
      machine.
    - Vnmrj must be able to translate between the mounted path and the
      actual path on the destination computer.
      - It will first look for a file '/usr/varian/mount_name_table'
        If found, it only uses this file.  An example of this file
        is as follows:
           # Table of remote system mount names and paths
           # One line per entry, Syntax:
           #     host:direct_path  mount_path
           mongoose:/export/home    /mongoose_home
           voyager:/export/home     /home
     - If mount_name_table is not found, it looks in /etc/mnttab
       and in /etc/auto_direct and gets relationships out of these.
       If automount relations are pushed from a server, they will not
       be available in these files and will need to be put into a
       mount_name_table file.  Mount from the vfstab file and from
       the local auto_direct file, should work properly without a
       mount_name_table file.


********************************************************************/

public class FillDBManager {
    /** The connection to the database */
    protected java.sql.Connection db = null;
    /** Our statement to run sql queries with */
    protected Statement  sql = null;
    
    // A way to turn off the locator function
    protected static boolean locatoroff=false;

    static protected long dateLimitMS=0;
    // An arbitrary future date in miliseconds.
    private static  long FUTURE_DATE = (long)2000000 * 1000000;


    /** Version Number for screated.  Change this number
        any time something has changed so that the database a user
        has needs to be destroyed and recreated.
    */
//    private static final String DBVERSION = "031027";
    private static final String DBVERSION = "xxxx";


    /** The name of the DataBase.  Individual tables are within this. */
    protected static final String dbName = "vnmr";
    public static final String DB_ATTR_INTEGER = "int";
    public static final String DB_ATTR_FLOAT8 = "float8";
    public static final String DB_ATTR_TEXT = "text";
    public static final String DB_ATTR_INTEGER_ARRAY= "int[]";
    public static final String DB_ATTR_FLOAT8_ARRAY = "float8[]";
    public static final String DB_ATTR_TEXT_ARRAY = "text[]";
    public static final String DB_ATTR_DATE = "timestamp";
    public static final int    NUM_TAGS = 5;
    public static final String T_REAL = "1";
    public static final String T_STRING = "2";
    public static final String ST_INTEGER = "7";
    public static final int    MAX_ARRAY_STR_LEN = 40;
    // Lists of all attributes to put into the locator
    protected HashArrayList shufDataParams;
    protected HashArrayList shufStudyParams;
    protected HashArrayList shufProtocolParams;
    protected HashArrayList shufImageParams;
    // Limited list of attributes to use in menus
    public HashArrayList shufDataParamsLimited;
    public HashArrayList shufStudyParamsLimited;
    public HashArrayList shufProtocolParamsLimited;
    public HashArrayList shufImageParamsLimited;
    // Directory list for finding all appmode param_list's.  When operating
    // in a given appmode, we normally only have access to that subdirectory
    // under the system dir.  For filling the shufXXXParams, we need access
    // to all of the files.  Experimental is in the base dir, so do not
    // include it.  Add new appmodes if they are created.
    protected String[] appmodFileList = {"imaging/shuffler/",
                                         "walkup/shuffler/"};
    public String localHost;
    public String dbHost;
    public static LocAttrList attrList=null;
    public static FillDBManager fillDBManager;
    // Flag whether we are in vnmrj or the managedb standalone.
    public static boolean managedb=false;

    public static SymLinkMap symLinkMap;
    public static SavedDirList savedDirList;

    // Members for message handling to allow shorter commands.
    protected static  int TYPE        = Messages.TYPE;
    protected static  int INFO        = Messages.INFO;
    protected static  int WARN        = Messages.WARN;
    protected static  int ERROR       = Messages.ERROR;
    protected static  int DEBUG       = Messages.DEBUG;

    protected static  int GROUP       = Messages.GROUP;
    protected static  int ACQ         = Messages.ACQ;
    protected static  int OTHER       = Messages.OTHER;

    protected static  int OUTPUT      = Messages.OUTPUT;
    protected static  int LOG         = Messages.LOG;
    protected static  int MBOX        = Messages.MBOX;
    protected static  int STDOUT      = Messages.STDOUT;
    protected static  int STDERR      = Messages.STDERR;
    private   static  int connectMessage = 1;
    private   static  int dbMessage = 1;

    // Temporary way to allow coding for new and old postgres.
    public static String newpostgres = "false";


    /************************************************** <pre>
     * Summary: Start and connect to db_manager.
     *
     </pre> **************************************************/
    public FillDBManager() {

        // Do all the main constructor stuff
        constructorCommonCode();

        symLinkMap = new SymLinkMap();

    } // End FillDBManager()

    /************************************************** <pre>
     * Summary: Start and connect to db_manager.
     *
     *  Allow for not creating a symLinkMap.  If the command to managedb
     *  is createdb, then the DB tables do not yet exist that are used
     *  by SymLinkMap, so we cannot make the SymLinkMap.
     </pre> **************************************************/
    public FillDBManager(boolean makeSymLinkMap) {

        // Do all the main constructor stuff
        constructorCommonCode();

        if(makeSymLinkMap)
            symLinkMap = new SymLinkMap();

    } // End FillDBManager()


    /******************************************************************
     * Summary: Constructor code to allow two constructors without
     *  duplicating this code.
     *
     *
     *****************************************************************/

    private void constructorCommonCode() {
        // Load the driver

        // Read these persistence files
        readLocatorOff(true);
        readDateLimit();

        if(!locatorOff()){
            try {
                if(newpostgres.equals("true"))
                    Class.forName("org.postgresql.Driver");
                else
                    Class.forName("postgresql.Driver");
            }
            catch(ClassNotFoundException e) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                     "Cannot find Class postgresql.Driver");
                return;
            }
        }

        // Get host name once and save for future use.
        try{
            InetAddress inetAddress = InetAddress.getLocalHost();
            localHost = inetAddress.getHostName();
        }
        catch(UnknownHostException ex) {
            Messages.postError("Problem getting hostname. Check that /etc/hosts file"
                               + "    has the computer name along with \'localhost\'.");
            System.exit(1);
        }

        // Get  dbHost.
        dbHost = System.getProperty("dbhost");
        if(dbHost == null) {
            Messages.postDebug("The environment variable 'dbhost'" +
                                " is missing in the managedb\n" +
                                "and/or vnmrj startup script.  This variable" +
                                " should be set to the host\n" +
                                "where the database manager is running." +
                                "  It may be set to the string\n" +
                                "'localhost', to default to the current host.");
            dbHost = localHost;
        }
        else if(dbHost.equals("localhost")) {
            // use the local host.
            dbHost = localHost;
        }

        


        /* Fill the param_list with params to put into shuffler. */
        boolean limited = false;  // Not limited param set
        shufDataParams = ReadParamListFile(Shuf.DB_VNMR_DATA, limited);
        // Use shufStudyParams for DB_STUDY and DB_LCSTUDY
        shufStudyParams = ReadParamListFile(Shuf.DB_STUDY, limited);
        shufProtocolParams = ReadParamListFile(Shuf.DB_PROTOCOL, limited);
        shufImageParams = ReadParamListFile(Shuf.DB_IMAGE_DIR, limited);
        // Fill the lists with a limited param set for menus
        limited = true;          // Limited param set
        shufDataParamsLimited = ReadParamListFile(Shuf.DB_VNMR_DATA, limited);
        shufStudyParamsLimited = ReadParamListFile(Shuf.DB_STUDY, limited);
        shufProtocolParamsLimited = ReadParamListFile(Shuf.DB_PROTOCOL,limited);
        shufImageParamsLimited = ReadParamListFile(Shuf.DB_IMAGE_DIR, limited);

        String curuser = System.getProperty("user.name");
        LoginService loginService = LoginService.getDefault();
        Hashtable userHash = loginService.getuserHash();
        User user = (User)userHash.get(curuser);
        savedDirList = new SavedDirList(user);

        fillDBManager = this;
    }

    public static void main(String[] args) {
        boolean recursive;
        boolean makeSymLinkMap = true;
        DBCommunInfo info = new DBCommunInfo();
        String usage = new String("Usage:\n" +
                             "managedb createdb " +
                             "| destroydb | addfile fullpath user " +
                             "| removeentry fullpath host\n" +
                             "         | removeentrydir dir " +
                             "| filldb dir user table [sleepMs]\n" +
                             "         | filldb_nonrecursive dir user table " +
                             "| cleanup | update [sleepMs]\n" +
                             "         | updatetable table | updatelocattr " +
                             "| checkDBversion | status\n" +
                             "         | updateloop [sleepMs] " +
                             "| setattr table fullpath attr value\n" +
                             "         | updateappdirs " +
                             " | getVnmrjVersion [ debug debug_category]");


        // Flag that we are executing from the managedb main().
        managedb = true;

        // We want managedb to log its messages to a different file
        // from vnmrj, so set the name here we want to use.
        Messages.setLogFileName("ManagedbMsgLog");
        String user = System.getProperty("user.name");
        Messages.postMessage(Messages.OTHER|Messages.LOG,
                             "******************************************");
        Messages.postMessage(Messages.OTHER|Messages.LOG,
                             "******* Starting managedb as " + user +
                             " ********");

        String cmdString = "";
        for(int i=0; i < args.length; i++) {
            cmdString = cmdString.concat(" " + args[i]);
        }
        Messages.postMessage(Messages.OTHER|Messages.LOG,
                              "Command: managedb" + cmdString);

        Messages.postMessage(Messages.OTHER|Messages.LOG,
                             "******************************************");

        if(args.length < 1) {
            Messages.postMessage(OTHER|ERROR|STDOUT, usage);
            System.exit(0);
        }
        // For the createdb command, we cannot make the SymLinkMap object
        // because there are no tables for the DB yet.  Since it is not
        // actually needed for this command, just don't try to make one.
        if(args[0].equals("createdb"))
            makeSymLinkMap = false;

        FillDBManager dbManager = new FillDBManager(makeSymLinkMap);

        if(args[0].endsWith("debug")) {
            Messages.postError("When the debug option is used, it MUST be "
                                 + "the LAST option on the command line.");
            System.exit(0);
        }
        else if(args[0].equals("status") || args[0].equals("dbStatus")) {
            // Output number of items in each DB category
            if(!locatorOff())
                dbManager.dbStatus();
            System.exit(0);
        }

        else if(args[0].equals("createdb")) {
            if(!locatorOff()) {
                dbManager.createEmptyDBandTables();
                dbManager.closeDBConnection();
            }
            System.exit(0);
        }

        if(args[0].equals("destroydb")) {
            if(!locatorOff()) {
                dbManager.destroyDB();
                dbManager.closeDBConnection();
            }
            System.exit(0);
        }

        // Connect to the DB
        dbManager.makeDBConnection();

        if(args[0].equals("checkDBversion")) {
            String result = dbManager.checkDBversion();

            // Print the result which will be okay or bad.
            System.out.print(result);
            System.exit(0);
        }

        if(args[0].equals("getVnmrjVersion")) {
            String result = dbManager.getVnmrjVersion();

            // Print the version.
            System.out.print(result);
            System.exit(0);
        }

        // Create and fill LocAttrList
        attrList = new LocAttrList(dbManager);

        if(args[0].equals("addfile")) {
            if(args.length < 3) {
                Messages.postMessage(OTHER|ERROR|STDOUT, usage);
                System.exit(0);
            }
            dbManager.addFileCommand(args[1], args[2]);
            dbManager.closeDBConnection();

            System.exit(0);
        }
        else if(args[0].equals("removeentry") || args[0].equals("removefile")) {
            dbManager.removeEntryCommand(args);
            dbManager.closeDBConnection();
            System.exit(0);
        }
        else if(args[0].equals("removeentrydir")) {
            dbManager.removeFilesInPath(args[1]);
            dbManager.closeDBConnection();
            System.exit(0);
        }
        else if(args[0].equals("cleanup")) {
            dbManager.cleanupDBList(info);
            dbManager.closeDBConnection();
            System.exit(0);
        }
        else if(args[0].equals("setattr")) {
            if(args.length < 5) {
                Messages.postMessage(OTHER|ERROR|STDOUT, usage);
                System.exit(0);
            }
            boolean foundit = false;
            for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
                if(args[3].equals(Shuf.OBJTYPE_LIST[i])) {
                    foundit = true;
                    break;
                }
            }
            String value = args[4];
            if(foundit) {
                // attr is an objtype, do the conversion
                Vector mp = MountPaths.getPathAndHost(args[4]);
                String dhost = (String) mp.get(Shuf.HOST);
                String dpath = (String) mp.get(Shuf.PATH);
                value = dhost + ":" + dpath;
            }
            String fullpath;
            fullpath = dbManager.getDefaultPath(args[1], args[2]);
            dbManager.setNonTagAttributeValue(args[1], fullpath,
                                  dbManager.localHost, args[3], "text", value);
            System.exit(0);
        }
        else if(args[0].equals("update")) {
            // If the locator is turned off, don't update anything
            if(locatorOff() || locatoroff) {
                Messages.postDebug("Locator Off, skipping DB update");
                System.exit(0);
            }

            // Check DB version
            String result = dbManager.checkDBversion();
            if(result.equals("bad")) {
                Messages.postMessage(OTHER|ERROR|STDOUT,
                         "DataBase contents version is not correct. \n"
                       + "    If using a remote DB server, it must have "
                       + "the same vnmrj version as this machine.\n"
                       + "    If vnmrj version is correct, run \'dbsetup\' on "
                       + dbManager.dbHost + " to create a new DB version." );
                System.exit(0);
            }
            // Optional 2nd arg is sleepMs
            long sleepMs = 0;
            if(args.length > 1 && !args[1].endsWith("debug")) {
                sleepMs = Integer.parseInt(args[1]);
            }

            LoginService loginService = LoginService.getDefault();
            Hashtable userHash = loginService.getuserHash();
            try {
                if(DebugOutput.isSetFor("updatedb"))
                    Messages.postDebug("Update starting updateDB"
                                  + " with sleepMs " + sleepMs);
                dbManager.updateDB(userHash, sleepMs);

                // Write out the attrList, else for large DB, vnmrj
                // can take several minutes to start while it creates this list.
                dbManager.attrList.writePersistence();

            }
            catch (InterruptedException ie) {
                // Don't do anything, just let things close.
            }
            dbManager.closeDBConnection();

            System.exit(0);
        }
        else if(args[0].equals("updateloop")) {
            if(locatorOff())
                System.exit(0);
            // The following bourn shell script code can be used in the
            // vnmrj startup script to start this loop as a separate
            // process running under nice.
            /**********
              # Only start crawler if DB is on this host.
              thishost=`/usr/bin/hostname`
              if [ $thishost = $dbhost -o $dbhost = "localhost" ]
              then
                  # Get pid of vnmrj.  If vnmrj dies, then kill the crawler .
                  vjpid=$!

                  sleep 20
                  # Start a crawler to keep the DB up to date
                  if test x$debug_on = "xy"
                  then
                     /usr/bin/nice -n 19 managedb updateloop debug "$debug" &
                  else
                     /usr/bin/nice -n 19 managedb updateloop &
                  fi

                  # Get pid of the crawler java program, not the script
                  # that starts it.
                  sleep 5
                  mdbpid=`ps -A -o pid,args | \grep java | \grep "managedb" | \grep -v grep | awk '{print $1}'`

                  # wait here until vnmrj dies, then kill managedb
                  wait $vjpid
                  echo "" >> $vnmruser/ManagedbMsgLog
                  date >> $vnmruser/ManagedbMsgLog
                  echo Vnmrj gone, Killing managedb updateloop >> $vnmruser/ManagedbMsgLog
                  echo "" >> $vnmruser/ManagedbMsgLog
                  kill $mdbpid >> $vnmruser/ManagedbMsgLog
              fi
            *************/

            // When this is called via vnmrj, only the first debug option
            // will be passed in.  the rt.exec() command does not allow
            // passing of a quoted string of args as one arg.
            // Check DB version
            String result = dbManager.checkDBversion();
            if(result.equals("bad")) {
                Messages.postMessage(OTHER|ERROR|STDOUT,
                           "DataBase contents version is not correct. \n"
                           + "      Run 'dbsetup' to create a new version." );
                System.exit(0);
            }

            // Optional 2nd arg is sleepMs
            long sleepMs = 1000;
            if(args.length > 1 && !args[1].endsWith("debug")) {
                sleepMs = Integer.parseInt(args[1]);
            }

            LoginService loginService = LoginService.getDefault();
            Hashtable userHash = loginService.getuserHash();

            try {
                // Give time for vnmrj to have started up
// Don't use this if not starting from vnmrj
//                Thread.sleep(120000);

                if(DebugOutput.isSetFor("updateloop"))
                    Messages.postDebug("Updateloop starting fillAllListsFromDB"
                                  + " with sleepMs " + sleepMs/2);
                // First be sure the LocAttrList is up to date
                attrList.fillAllListsFromDB(sleepMs/2);
            }
            catch (Exception e) {
                Messages.writeStackTrace(e);
                // Now just continue.
            }

            while(true) {
                if(DebugOutput.isSetFor("updateloop"))
                    Messages.postDebug("Updateloop starting updateDB cycle "
                                       + "with sleepMs " + sleepMs);
                try {
                    boolean workspace = false;
                    dbManager.updateDB(userHash, sleepMs, workspace);
                    if(DebugOutput.isSetFor("updateloop"))
                        Messages.postDebug("Updateloop finished updateDB");
                    // sleep
                    Thread.sleep(200000);
                }
                catch (InterruptedException ie) {
                    // Don't do anything, just let things close.
                }
            }
        }
        else if(args[0].equals("updatetable")) {
            // Check DB version
            String result = dbManager.checkDBversion();
            if(result.equals("bad")) {
                Messages.postMessage(OTHER|ERROR|STDOUT,
                           "DataBase contents version is not correct. \n"
                           + "      Run 'dbsetup' to create a new version." );
                System.exit(0);
            }
            if(args.length < 2) {
                Messages.postMessage(OTHER|ERROR|STDOUT, usage);
                System.exit(0);
            }
            try {
                dbManager.updateThisTable(args[1]);
            }
            catch (Exception e) {
                Messages.postError("Problem updating DB for type = " + args[1]);
                Messages.writeStackTrace(e);
            }
            dbManager.closeDBConnection();

            System.exit(0);
        }
        // Add files found in the appdirs areas for all users
        // This can be used when using a previous DB and wanting to add the
        // appdir files from the new install area
        else if(args[0].equals("updateappdirs")) {
            if(locatorOff())
                System.exit(0);

            try {
                // Get the user list
                LoginService loginService = LoginService.getDefault();
                Hashtable users = loginService.getuserHash();
                
                boolean workspace = false;
                boolean appdirOnly = true;
                // Update using the appdirs for all users but not the datadirs
                // This get things like protocols, shims and panelsNcomponents 
                // that have special directories.
                // It does not get data, par study, image_dir
                dbManager.updateDB(users, 0, workspace, appdirOnly);

                // Get data, par study, image_dir in vnmrsystem
                String sysdir = System.getProperty("sysdir");
                String owner = "agilent";
                recursive = true;

                // We need to fill from all of the /vnmr, /vnmr/imaging etc
                // directories IF they are in the appdir list.  We do not
                // want all appdirs because we don't want the users home.
                // We do not want all /vnmr possibilities because if it
                // is a liq only site, we don't want imaging etc.
                // So, go through the appdir list and take dirs that start
                // with sysdir.  We have to do this for each user.

                ArrayList appDirsToUse = new ArrayList();
                
                // Go thru the users
                for(Enumeration en = users.elements(); en.hasMoreElements(); ) {
                    User userobj = (User) en.nextElement();

                    // Get appdirs for this user
                    ArrayList appDirs=userobj.getAppDirectories();
                    // Go through the list and keep dirs in sysdir
                    for(int i=0; i < appDirs.size(); i++) {
                        String dir = (String)appDirs.get(i);
                        if(dir.startsWith(sysdir)) {
                            // Weed out duplicates
                            if(!appDirsToUse.contains(dir))
                                appDirsToUse.add(dir);
                        }
                    }
                }
                
                // Now loop through the appDirsToUse and fill some tables
                for (int i = 0; i < appDirsToUse.size(); i++) {
                    String dirToUse = (String) appDirsToUse.get(i);
                    if(DebugOutput.isSetFor("updateappdirs"))
                        Messages.postDebug("updateappdirs filling dir: "
                                           + dirToUse);

                    dbManager.fillATable(Shuf.DB_VNMR_DATA, dirToUse, owner,
                            recursive, info);
                    dbManager.fillATable(Shuf.DB_VNMR_PAR, dirToUse, owner,
                            recursive, info);
                    dbManager.fillATable(Shuf.DB_STUDY, dirToUse, owner,
                            recursive, info);
                    dbManager.fillATable(Shuf.DB_IMAGE_DIR, dirToUse, owner,
                            recursive, info);
                    dbManager.fillATable(Shuf.DB_PROTOCOL, dirToUse, owner,
                            recursive, info);
                }
            }
            catch (Exception e) {
                Messages.postError("Problem with managedb updateappdirs");
                Messages.writeStackTrace(e);
            }
        }
        else if(args[0].equals("updatelocattr")) {
            if(locatorOff())
                System.exit(0);
            try {
                // Fill the list
                dbManager.attrList.fillAllListsFromDB(0);
                // Write the persistence file
                dbManager.attrList.writePersistence();
            }
            catch (Exception e) {
                Messages.postError("Problem updating locator attribute list");
                Messages.writeStackTrace(e);
            }

            dbManager.closeDBConnection();
            System.exit(0);
        }

        else if(args[0].startsWith("filldb")) {
            if(locatorOff() || locatoroff) {
                Messages.postDebug("Locator Off, skipping DB filldb");
                System.exit(0);
            }
            // Check DB version
            String result = dbManager.checkDBversion();
            if(result.equals("bad")) {
                Messages.postMessage(OTHER|ERROR|STDOUT,
                           "DataBase contents version is not correct. \n"
                           + "      Run 'dbsetup' to create a new version." );
                System.exit(0);
            }

            if(args.length < 4) {
                Messages.postMessage(OTHER|ERROR|STDOUT, usage);
                System.exit(0);
            }
            // Optional 5th arg is sleepMs
            long sleepMs = 0;
            if(args.length > 4 && !args[4].endsWith("debug")) {
                sleepMs = Integer.parseInt(args[4]);
            }
            if(args[0].equals("filldb"))
                recursive = true;
            else
                recursive = false;
            String objType = args[3];
            // Allow a type of all, then do all objTypes
            if(objType.equals("all")) {
                dbManager.fillAllTables(args[1], args[2], recursive,
                                     info, sleepMs);
            }
            else {
                // Update the Attribute list for this objType
                dbManager.attrList.updateSingleList((args[3]), sleepMs);

                dbManager.fillATable(args[3], args[1], args[2], recursive,
                                     info, sleepMs);
            }

            if(info.numFilesAdded > 0)
                Messages.postMessage(OTHER|INFO|LOG|STDOUT,
                            "\nNumber of Files added = " + info.numFilesAdded);
            dbManager.updateMacro(Shuf.DB_COMMAND_MACRO);
            dbManager.updateMacro(Shuf.DB_PPGM_MACRO);
        }
         else {
            Messages.postMessage(OTHER|ERROR|STDOUT,
                                 "No such option, " + args[0]);
            Messages.postMessage(OTHER|INFO|STDOUT, usage);
            System.exit(0);
        }
        dbManager.closeDBConnection();
        System.exit(0);
    }

    // Remove all of the file in all tables that are in a subdirectory of the 
    // arg path.  This is designed for use during install to remove all items
    // what were in the previous "/vnmr" directory.  That is, since the
    // actual directory probably still exists, we end up with /vnmr files
    // from each install.  To reuse the database after install, we need to
    // cleanout the previous /vnmr files.
    public void removeFilesInPath(String path) {
        if (locatorOff() && !managedb)
            return;
        for(int i=0; i < Shuf.OBJTYPE_LIST.length; i++) {
            String objType = Shuf.OBJTYPE_LIST[i];
            // be sure path ends with a terminator, else we could remove
            // files in a directory that just starts with this name.
            if(!path.endsWith(File.separator)) {
                path = path.concat(File.separator);
            }
            String cmd = "DELETE from " + objType 
                + " where fullpath like \'%" + path + "%\'";

            if(DebugOutput.isSetFor("removeentrydir"))
                Messages.postDebug("removeentrydir: sql cmd: " + cmd);

            try {
                executeUpdate(cmd);
            }
            catch (Exception e) {
                if(!ExitStatus.exiting()) {
                    Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                              "Problem removing " + path + " files from DB");
                    Messages.writeStackTrace(e,"Error caught in removeFilesInPath");
                }
            }
        }
    }

    /************************************************** <pre>
     * Summary: This is the driver the command line option to add a file.
     *
     *   - Determine which objType
     *     Do this by looking at the suffix.  We do not support
     *     adding of command_n_macro this way nor workspace files this way.
     *     We support suffixes of .fid, .par, .spc and .xml
     *   - get the filename from the fullpath
     *   - Call addFileToDB to do the rest.
     *
     </pre> **************************************************/

    public boolean addFileCommand(String fullpath, String owner) {
        String  filename;
        String  objType;
        int     index;
        boolean success;

        // Insist on fullpath starting at root
        if(!fullpath.startsWith("/")) {
            Messages.postMessage(OTHER|ERROR|STDOUT,
                                 "The fullpath must start with '/'");
            return false;
        }

        // First get the filename.  That should be everything after
        // the last '/'
        index = fullpath.lastIndexOf("/");
        filename = fullpath.substring(index +1);

        objType = getType(fullpath, filename);
        if(objType.equals("?")) {
            Messages.postError("Unknown File Type " + fullpath);
            return false;
        }

        // Add it.
        // Force all vnmrj to update if displaying this type
        boolean notify=true;
        // Force the update of this file, even is time stamp is okay.
        // If only a file like studypar is modified, the study time stamp
        // will show it is up to date.
        boolean force=true;  
        success = addFileToDB(objType, filename, fullpath, owner, notify,force);
        if(success)
            Messages.postMessage(OTHER|INFO|STDOUT,
                                 filename + " added to database.");

        return success;

    }

    public ArrayList getTagListFromDB(String objType) throws SQLException {
        ArrayList output;
        java.sql.ResultSet rs;
        int numTagCol;
        String value;

        output = new ArrayList();

        /* I have not found a way to get SQL to give DISTINCT results
           across several columns.  Thus we have to get the tag values
           for each tag column and weed out duplicated for ourselves.
        */
        /* Get the number of tag columns. */
        try {
            numTagCol = numTagColumns(objType);
        }
        catch (SQLException pe) {
            throw pe;
        }

        String user = System.getProperty("user.name");
        for(int i=0; i < numTagCol; i++) {
            try {
                // Initially I used DISTINCT here, but some of the
                // items took 10-30 sec.  Taking out the DISTINCT
                // gives us back all of the rows and they get
                // weeded out below.
                rs = executeQuery("SELECT \"tag" + i + "\" FROM " +
                                  objType);

                if(rs == null)
                    return output;

                while(rs.next()) {
                    // There is only one column of results, thus, col 1
                    value = rs.getString(1);
                    // Trap out null and user:null
                    // Also trap for only the current user
                    if(value != null && !value.equals("null") &&
                       !value.equals(user + ":" + "null") &&
                       value.startsWith(user + ":")) {
                        int index = value.indexOf(':');
                        if(index >= 0)
                            value = value.substring(index +1);
                        value = value.trim();
                        // Only add it if we do not already have it.
                        if(!output.contains(value)) {
                            // Be sure it is not whitespace
                            if(value.length() > 0) {
                                output.add(value);
                            }
                        }
                    }
                }
                // Try to keep it using minimal cpu
                Thread.sleep(5000);
            } catch (Exception e) { }
        }


        // Alphabetize or numeric order
        Collections.sort(output,  new NumericStringComparator());

        return output;
    }


    /************************************************** <pre>
     * Summary: Get the value for a given attribute for a given filepath.
     *
     * Details:
     *    Example of calling this method:
     *
     *    ShufflerService shufflerService = ShufflerService.getDefault();
     *    ShufDBManager dbManager = shufflerService.getdbManager();
     *    String value = dbManager.getAttributeValue(objType, fullpath,
     *                                                 host, attr);
     *        where:
     *           objType = Shuf.DB_VNMR_DATA, Shuf.DB_PANELSNCOMPONENTS,
     *                      etc.
     *           fullpath = The filename and path starting at /
     *           host =     The host name that added the file to the DB
     *           attr =     Which attribute to get the value for
     *        Note: If the item was Dragged and Dropped, most of this
     *        information is available in the ShufflerItem.
     *
     </pre> **************************************************/

    public String getAttributeValue(String objType, String fullpath,
                                    String host, String attr) {
        java.sql.ResultSet rs;
        String value;
        String dhost, dpath;

        try {
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch (Exception e) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem getting cononical path for\n    " +
                                 fullpath);
            Messages.writeStackTrace(e);
            return "";
        }

        String host_fullpath = new String(dhost + ":" + dpath);
        /* Create SQL command   */
        String cmd = new String("SELECT " + attr + " FROM " + objType +
                          " WHERE host_fullpath = \'" + host_fullpath + "\'");

        // Execute the sql select command
        try {
            rs = executeQuery(cmd);
            if(rs == null)
                return "";

            if(rs.next()) {
                // getString() gives the string representation even if
                // the column type is int or float or date
                value = rs.getString(1);
                return value;
            }
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem getting the value for \n    " +
                                   attr + " in " + fullpath);
                Messages.writeStackTrace(e,
                                         "Error caught in getAttributeValue");
            }
        }
        return null;
    }

    /** Same as getAttributeValue(), but posts No Errors.  Instead, when
        an error occurs, it returns an empty string.
    */
    public String getAttributeValueNoError(String objType, String fullpath,
                                           String host, String attr) {
        java.sql.ResultSet rs;
        String value;
        String dhost, dpath;

        try {
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch (Exception e) {
            // No errors posted
            return "";
        }

        String host_fullpath = new String(dhost + ":" + dpath);
        /* Create SQL command   */
        String cmd = new String("SELECT " + attr + " FROM " + objType +
                          " WHERE host_fullpath = \'" + host_fullpath + "\'");

        // Execute the sql select command
        try {
            rs = executeQuery(cmd);
            if(rs == null)
                return "";

            if(rs.next()) {
                // getString() gives the string representation even if
                // the column type is int or float or date
                value = rs.getString(1);
                return value;
            }
        }
        catch (Exception e) {
            // No errors posted
            return "";
        }
        return "";
    }


    /************************************************** <pre>
     * Summary: Add a file to the DataBase.
     *
     * Details:
     *
     *    Return true for success and false for failure.
     *
     *
     *
     *    Example of calling this method:
     *
     *    ShufflerService shufflerService = ShufflerService.getDefault();
     *    ShufDBManager dbManager = shufflerService.getdbManager();
     *    boolean status = dbManager.addFileToDB(objType, filename,
     *                                           fullpath, owner);
     *        where:
     *           objType = Shuf.DB_VNMR_DATA, Shuf.DB_PANELSNCOMPONENTS,
     *                      etc.
     *           filename = The filename (or directory name for nmr data)
     *           fullpath = The filename and path starting at /
     *           owner = The user name to be installed as the owner of
     *                          this file/data.
     *
     *       Note: If the item was Dragged and Dropped, all of this information
     *       is available in the ShufflerItem except owner.
     *
     *
     *
     </pre> **************************************************/
    public boolean addFileToDB (String objType, String filename,
                                 String fullpath, String owner) {

        // Default to sending notification of the change the the DB
        boolean notify = true;
        return addFileToDB(objType, filename, fullpath, owner, notify);
 
    }

    public boolean addFileToDB (String objType, String filename,
                                String fullpath, String owner, boolean notify) {
        boolean force = false;
        return addFileToDB(objType, filename, fullpath, owner, notify, force);
    }


    public boolean addFileToDB (String objType, String filename,
                                String fullpath, String owner, 
                                boolean notify, boolean force) {
        boolean success;
        long exists;
        DBCommunInfo info = new DBCommunInfo();
        String dhost, dpath;


        java.util.Date starttime=null;
        if(DebugOutput.isSetFor("addFileToDB") ||
                       DebugOutput.isSetFor("locTiming"))
            starttime = new java.util.Date();

        // The CanonicalPath is the real absolute path NOT using
        // any symbolic links.  It is the real path.
        try {
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch (Exception e) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem getting cononical path for\n    " +
                                 fullpath);
            Messages.writeStackTrace(e);
            return false;
        }


        // Is this host and fullpath unique?
        exists = isDBKeyUniqueAndUpToDate(objType, dhost + ":" +  dpath,
                                           fullpath);
        if(exists == 0) {
            // The entry exists in the DB and is up to data, but we are
            // requested to force an update, remove the entry
            if(force)
                removeEntryFromDB(objType, dhost + ":" +  dpath, info);

            // The entry exists in the DB and is up to data
            // If not a study nor automation, then return.  Study's and
            // automation's may have files beneath them that need attention
            // so continue.
            else if(!objType.equals(Shuf.DB_STUDY) &&
                    !objType.equals(Shuf.DB_LCSTUDY) &&
                    !objType.equals(Shuf.DB_AUTODIR))
                return false;
        }
        else if(exists == -1) {
            // The entry exists and is out of date.  Remove it and
                // continue with adding it back in.
            removeEntryFromDB(objType, dhost + ":" +  dpath, info);
        }
        try {
            // If this fullpath is for a system file, set the user to agilent
            String sysdir = System.getProperty("sysdir");
            if(sysdir != null) {
                UNFile sysfile = new UNFile(sysdir);
                if(sysfile != null) {
                    // Need to compare the canonical path
                    String csysdir = sysfile.getCanonicalPath();
                    if(fullpath.startsWith(csysdir + "/"))
                        owner = "agilent";
                }
            }
        }
        catch (Exception e) { }

        /* We need to call the correct function to add these. */
        if(objType.equals(Shuf.DB_VNMR_DATA)) {
            success = addVnmrFileToDB(filename, fullpath, owner, objType, 0, null);
            // Check to see if this vnmr_data is in a study.
            // If so, set the "study" attribute
            getStudyParentAndSet(objType, fullpath);
        }
        else if(objType.equals(Shuf.DB_VNMR_PAR))
            success = addVnmrFileToDB(filename, fullpath, owner, objType, 0, null);
        else if(objType.equals(Shuf.DB_VNMR_RECORD))
            success = addVnmrFileToDB(filename, fullpath, owner, objType, 0, null);
        else if(objType.equals(Shuf.DB_VNMR_REC_DATA))
            success = addVnmrFileToDB(filename, fullpath, owner, objType, 0, null);
        else if(objType.equals(Shuf.DB_WORKSPACE))
            success = addVnmrFileToDB(filename, fullpath, owner, objType, 0, null);
        else if(objType.equals(Shuf.DB_SHIMS))
            success = addVnmrFileToDB(filename, fullpath, owner, objType, 0, null);
        else if(objType.equals(Shuf.DB_IMAGE_DIR)) {
            success = addImageDirToDB(filename, fullpath, owner, objType, 0);
            // Check to see if this image_dir is in a study.
            // If so, set the "study" attribute
            getStudyParentAndSet(objType, fullpath);
        }
        else if(objType.equals(Shuf.DB_COMPUTED_DIR))
            success = addImageDirToDB(filename, fullpath, owner, objType, 0);
        else if(objType.equals(Shuf.DB_IMAGE_FILE))
            success = addVnmrFileToDB(filename, fullpath, owner, objType, 0, null);
        else if(objType.equals(Shuf.DB_PANELSNCOMPONENTS)) {
            Object ret = addXmlFileToDB(filename, fullpath, owner, objType);
            if(ret == null)
                success = false;
            else
                success = true;
        }
        else if(objType.equals(Shuf.DB_STUDY) || 
                objType.equals(Shuf.DB_LCSTUDY))
            success = addStudyToDB(filename, fullpath, owner, objType, 0);
        else if(objType.equals(Shuf.DB_AUTODIR))
            success = addAutodirToDB(filename, fullpath, owner, objType, 0);
        else if(objType.equals(Shuf.DB_PROTOCOL)) {
            DBCommunInfo dbInfo = addXmlFileToDB(filename, fullpath, owner, objType);
            if(dbInfo == null)
                success = false;
            else {
                // For Protocols, we have to get attr from .xml and procpar
                // The call to addXmlFileToDB started the process and the
                // partial information is in info.  Send that to the call to
                // addVnmrFileToDB and let it finish creating the Insert command
                // for adding to the DB.
                success = addVnmrFileToDB(filename, fullpath, owner, objType, 0, dbInfo);
            }
        }
        else if(objType.equals(Shuf.DB_TRASH))
            success = addTrashFileToDB(fullpath);
        else {
           Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                "Problem adding " + fullpath + " to DB");
           Messages.postMessage(OTHER|ERROR|LOG,
                                "    table = " + objType + " not found");
            return false;
        }

        try {
            // Tell all DB connections it has changed.
            executeUpdate("NOTIFY UpdateHappened");
        }
        catch(Exception e) {
            Messages.postMessage(OTHER|WARN|LOG, "NOTIFY CMD: " + e);
        }
        if(DebugOutput.isSetFor("addFileToDB") ||
                       DebugOutput.isSetFor("locTiming")) {
            java.util.Date endtime = new java.util.Date();
            long timems = endtime.getTime() - starttime.getTime();
            Messages.postDebug("addFileToDB: Time(ms) for adding\n   "
                               + fullpath + " = " + timems);
        }

        // Set this dir into the SavedDirList if successful
        if(success && (objType.equals(Shuf.DB_VNMR_DATA)
                       || objType.equals(Shuf.DB_VNMR_PAR)
                       || objType.equals(Shuf.DB_STUDY)
                       || objType.equals(Shuf.DB_LCSTUDY)
                       || objType.equals(Shuf.DB_AUTODIR))) {

            // we want the parent dir of fullpath.
            UNFile file = new UNFile(fullpath);
            String parent = file.getParent();

            // Now add this dir to SavedDirList if necessary
            savedDirList.addToList(parent);

        }

        if(notify) {
            // Send notification that this table of the DB has been modified
            sendTableModifiedNotification(objType);
        }

        return success;
    }

    // Determine if this item is within a study.  If it is, return
    // the HostFullpath for the parent study this item resides within.
    // Look for either the parent directory or the grandparent or 
    // great grandparent directory that contains a studypar file.
    // If a study is found, set the value into the "study" attribute
    // for this item.
    protected boolean getStudyParentAndSet(String objType, String fullpath) {
        String dhost, dpath;

        // Get dhost and dpath
        try {
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch (Exception e) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem getting cononical path for\n    " +
                                 fullpath);
            Messages.writeStackTrace(e);
            return false;
        }

        // Get the parent directory.
        String parent = "";
        int index = fullpath.lastIndexOf(File.separator);
        if(index > 0)
            parent = fullpath.substring(0, index);

        String parPath = new String(parent + File.separator +
                                    "studypar");
        UNFile file = new UNFile(parPath);
        // Does it exist?
        if(file.isFile()) {
            // Yes, this item is within a study, set the
            // 'study' attribute for this.
            try {
                Vector mp = MountPaths.getCanonicalPathAndHost(parent);
                dhost = (String) mp.get(Shuf.HOST);
                dpath = (String) mp.get(Shuf.PATH);
            }
            catch (Exception e) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                     "Problem getting cononical path for\n    " +
                                     parent);
                Messages.writeStackTrace(e);
                return false;
            }


            String parentHostFullpath = dhost + ":" + dpath;
            setNonTagAttributeValue(objType, fullpath,
                                    localHost, "study", "text",
                                    parentHostFullpath);
        }
        else {
            String grandparent = "";
            index = parent.lastIndexOf(File.separator);
            if(index > 0)
                grandparent = parent.substring(0, index);
            parPath = new String(grandparent + File.separator +
                                 "studypar");
            file = new UNFile(parPath);
            // Does it exist?
            if(file.isFile()) {
                // Yes, this item is within a study, set the
                // 'study' attribute for this.
                try {
                    Vector mp =
                        MountPaths.getCanonicalPathAndHost(grandparent);
                    dhost = (String) mp.get(Shuf.HOST);
                    dpath = (String) mp.get(Shuf.PATH);
                }
                catch (Exception e) {
                    Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                         "Problem getting cononical path for\n    " +
                                         grandparent);
                    Messages.writeStackTrace(e);
                    return false;
                }


                String grandparentHostFullpath = dhost + ":" + dpath;
                setNonTagAttributeValue(objType, fullpath,
                                        localHost, "study", "text",
                                        grandparentHostFullpath);
            }
            else {
                String ggrandparent = "";
                index = grandparent.lastIndexOf(File.separator);
                if(index > 0)
                    ggrandparent = grandparent.substring(0, index);
                parPath = new String(ggrandparent + File.separator +
                                     "studypar");
                file = new UNFile(parPath);
                // Does it exist?
                if(file.isFile()) {
                    // Yes, this item is within a study, set the
                    // 'study' attribute for this.
                    try {
                        Vector mp =
                            MountPaths.getCanonicalPathAndHost(ggrandparent);
                        dhost = (String) mp.get(Shuf.HOST);
                        dpath = (String) mp.get(Shuf.PATH);
                    }
                    catch (Exception e) {
                        Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                             "Problem getting cononical path for\n    " +
                                             ggrandparent);
                        Messages.writeStackTrace(e);
                        return false;
                    }


                    String ggrandparentHostFullpath = dhost + ":" + dpath;
                    setNonTagAttributeValue(objType, fullpath,
                                            localHost, "study", "text",
                                            ggrandparentHostFullpath);
                }
            }
        }
        return true;
    }

    public boolean removeEntryCommand(String[] args) {
        boolean success;
        String  objType;
        String  fullpath;
        boolean status;
        DBCommunInfo info = new DBCommunInfo();
        String dhost, dpath;

        fullpath = args[1];

        // Insist on fullpath starting at root
        if(!fullpath.startsWith("/")) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "The fullpath must start with '/'");
            return false;
        }

        try {
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch (Exception e) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem getting cononical path for\n    " +
                                 fullpath);
            Messages.writeStackTrace(e);
            return false;
        }


        objType = getType(fullpath);

        // Be sure this hostfullpath exist in the DB.  The method
        // removeEntryFromDB() does not really know if the file was there
        // and thus removed.
        status = isDBKeyUnique(objType, dhost + ":" + dpath);
        if(status == true) {
            // If isDBKeyUnique() is true, then the file does not exist.
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 dhost + ":" + dpath + " is not in the DB.");
            return false;
        }

        // Remove it
        success = removeEntryFromDB(objType, dpath, dhost, info);
        if(success)
            Messages.postMessage(OTHER|INFO|STDOUT,
                                 fullpath + " removed from database.");
        else
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem removing " + fullpath +
                                 " from database.");
        return success;
    }

    /************************************************** <pre>
     * Summary: Remove a file from the DataBase.
     *
     * Details:
     *
     *    Return true for success and false for failure.
     *
     *    This method can be called to remove a file from the DB after
     *    removing the file from the disk.
     *
     *    Example of calling this method:
     *
     *    ShufflerService shufflerService = ShufflerService.getDefault();
     *    ShufDBManager dbManager = shufflerService.getdbManager();
     *    boolean status = dbManager.removeEntryFromDB(objType, fullpath, host);
     *        where:
     *           objType = Shuf.DB_VNMR_DATA, Shuf.DB_PANELSNCOMPONENTS,
     *                      etc.
     *           fullpath = The filename and path starting at /
     *           host =     The host name that added the file to the DB
     *        Note: If the item was Dragged and Dropped, all of this information
     *        is available in the ShufflerItem.
     *
     </pre> **************************************************/

    public boolean removeEntryFromDB (String objType, String fullpath,
                                     String dhost, DBCommunInfo info) {
        try {
            executeUpdate("DELETE FROM " + objType +
                              " WHERE host_fullpath = \'" +
                              dhost + ":" + fullpath + "\'");
            info.numFilesRemoved++;
            return true;
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem removing " + fullpath + " from DB");
                Messages.writeStackTrace(e,"Error caught in removeEntryFromDB");
            }
            return false;
        }
    }

    public boolean removeEntryFromDB (String objType, String host_fullpath,
                                      DBCommunInfo info) {
        try {
            executeUpdate("DELETE FROM " + objType +
                              " WHERE host_fullpath = \'" +
                              host_fullpath + "\'");

            info.numFilesRemoved++;
            return true;
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                              "Problem removing " + host_fullpath + " from DB");
                Messages.writeStackTrace(e,"Error caught in removeEntryFromDB");
            }
            return false;
        }
    }


    /************************************************** <pre>
     * Summary: This version for callers who do not need to use 'info'.
     *
     *
     </pre> **************************************************/
    public boolean removeEntryFromDB (String objType, String fullpath,
                                      String dhost) {
        DBCommunInfo info = new DBCommunInfo();

        return removeEntryFromDB(objType, fullpath, dhost, info);

    }

    // Fill all of the tables using only this directory
    public void fillAllTables(String baseDir, String userName, boolean recur, 
                              DBCommunInfo info, long sleepMs) {

        String dir;

        dir=FileUtil.vnmrDir(baseDir,"SHIMS");
        if(dir !=null)
            fillATable(Shuf.DB_SHIMS, dir, userName, recur, info);

        dir=FileUtil.vnmrDir(baseDir,"LAYOUT");
        if(dir !=null){
            fillATable(Shuf.DB_PANELSNCOMPONENTS, dir, userName, recur, info);
        }

        dir=FileUtil.vnmrDir(baseDir,"PANELITEMS");
        if(dir !=null){
            fillATable(Shuf.DB_PANELSNCOMPONENTS, dir, userName, recur, info);
        }

        // Get Protocol Files
        dir=FileUtil.vnmrDir(baseDir,"PROTOCOLS");
        if(dir !=null)
            fillATable(Shuf.DB_PROTOCOL, dir, userName, recur, info);

        // Get Trash Files
        dir=FileUtil.vnmrDir(baseDir,"TRASH");
        if(dir !=null)
            fillATable(Shuf.DB_TRASH, dir, userName, false, info, sleepMs);
       

        fillATable(Shuf.DB_VNMR_DATA, baseDir, userName, recur, info, sleepMs);

        fillATable(Shuf.DB_VNMR_PAR, baseDir, userName, recur, info, sleepMs);

        fillATable(Shuf.DB_STUDY, baseDir, userName, recur, info, sleepMs);

        fillATable(Shuf.DB_LCSTUDY, baseDir, userName, recur, info, sleepMs);

        fillATable(Shuf.DB_AUTODIR, baseDir, userName, recur,  info, sleepMs);

        fillATable(Shuf.DB_IMAGE_DIR, baseDir, userName, recur,  info, sleepMs);

    }


    public ArrayList fillATable(String objType, String dir, String user,
                                boolean recursive, DBCommunInfo info,
                                long sleepMs) {

        // Call fillATable with null for attrName and attrVal.
        return fillATable(objType, dir, user, recursive, info, sleepMs,
                          null, null);

    }

    public ArrayList fillATable(String objType, String dir, String user,
                                boolean recursive, DBCommunInfo info) {

        // Call fillATable with a sleepMs = 0
        return fillATable(objType, dir, user, recursive, info, 0);

    }
    /************************************************** <pre>
     * Summary: Fill a single table in the DB.
     *
     *  For fids within a 'study' within an 'automation', the files
     *  will be gone through 5 times as follows:
     *  1 - add the fids recursiverly as normal
     *  2 - add studies (checking all files and directories except fids) fast
     *  3 - go through all fids under this study to set the 'study' attribute
     *  4 - add automations (checking all files and dirs except fids) fast
     *  5 - go thru all fids under this automation to set the 'automation' attr
     *
     </pre> **************************************************/

    public ArrayList fillATable(String objType, String dir, String user,
                          boolean recursive, DBCommunInfo info, long sleepMs,
                          String attrName, String attrVal) {
        String suffix="", prefix="";
        String fullpath=null;
        String hostFullpath;
        String filename;
        boolean success=false;
        ArrayList files = new ArrayList();
        UNFile file;
        int filesAdded=0, totFiles=0;
        java.util.Date starttime=null;

        if (locatorOff())
            return files;
        // Check DB version
        String result = checkDBversion();
        if(result.equals("bad")) {
            // If there is a connection, then the bad version is probably valid.
            if(checkDBConnection() == true)
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                        "DataBase contents version is not correct. \n"
                        + "      Run 'dbsetup' to create a new version." );
            else if (dbMessage == 1)
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                "No DB connection, Locator will not function.");

            /* Avoid a lot of these messages */
            dbMessage = 0;
            return null;
        }

        dbMessage = 1;

        if(DebugOutput.isSetFor("fillATable")) {
            Messages.postDebug("fillATable(): objType = " + objType +" dir = " +
                     dir + " user = " + user + " sleepMs = " +  sleepMs +
                     "\nDBCommunInfo Class = " + info);
        }
        if(DebugOutput.isSetFor("fillATable") ||
                       DebugOutput.isSetFor("locTiming"))
            starttime = new java.util.Date();

        // If dateLimitMS = FUTURE_DATE and this is not a system directory, 
        // skip it

        if(objType.equals(Shuf.DB_VNMR_DATA) && dateLimitMS == FUTURE_DATE) {
            String sysdir = System.getProperty("sysdir");
            if(sysdir != null) {
                UNFile sysfile = new UNFile(sysdir);
                if(sysfile != null) {
                    // Need to compare the canonical path
                    String csysdir = sysfile.getCanonicalPath();

                    if(!dir.startsWith(csysdir + "/")) {
                        // Not varian file, just exit
                        if(DebugOutput.isSetFor("fillATable"))
                            Messages.postDebug("fillATable(): skipping " + dir);

                        return null;
                    }
                }
            }  
        }

        // This try is to catch any unspecified exceptions encountered in the
        // code.  Catch it here so we can print out information about what
        // we were trying to do instead of catching it later and having no
        // information about what we were doing.
        try {
            // Set up suffix and prefix for use in getting files to put
            // into DB
            if(objType.equals(Shuf.DB_VNMR_DATA))
                suffix = Shuf.DB_FID_SUFFIX;
            else if(objType.equals(Shuf.DB_VNMR_PAR))
                suffix = Shuf.DB_PAR_SUFFIX;
            else if(objType.equals(Shuf.DB_VNMR_RECORD))
                suffix = Shuf.DB_REC_SUFFIX;
            else if(objType.equals(Shuf.DB_WORKSPACE))
                prefix = Shuf.DB_WKS_PREFIX;
            else if(objType.equals(Shuf.DB_PANELSNCOMPONENTS))
                suffix = Shuf.DB_PANELSNCOMPONENTS_SUFFIX;
            else if(objType.equals(Shuf.DB_SHIMS))
                suffix = "";
            else if(objType.equals(Shuf.DB_PROTOCOL))
                suffix = Shuf.DB_PROTOCOL_SUFFIX;
            else if(objType.equals(Shuf.DB_STUDY))
                suffix = Shuf.DB_STUDY_SUFFIX;
            else if(objType.equals(Shuf.DB_LCSTUDY))
                suffix = Shuf.DB_LCSTUDY_SUFFIX;
            else if(objType.equals(Shuf.DB_IMAGE_DIR))
                suffix = Shuf.DB_IMG_DIR_SUFFIX;
            else if(objType.equals(Shuf.DB_IMAGE_FILE))
                suffix = Shuf.DB_IMG_FILE_SUFFIX;
            else if(objType.endsWith(Shuf.DB_MACRO)) {
                fillMacro(objType);
                return files;
            }

            // If not recursive, for studies and autodirs, just use this file
            if(!recursive && (objType.equals(Shuf.DB_STUDY) ||
                              objType.equals(Shuf.DB_LCSTUDY) ||
                              objType.equals(Shuf.DB_AUTODIR))) {
                // If dir ends with a /, remove it.
                if(dir.endsWith(File.separator))
                    files.add(new UNFile(dir.substring(0, dir.length() -1)));
                else
                    files.add(new UNFile(dir));
            }
            else {
                /* Get all the files of this type to be added */

                getFileListing(files, prefix, suffix, dir, recursive, objType);
                /* if objType is DB_VNMR_RECORD, then we need to get the
                   files with the lower case suffix also.  calling
                   getFileListing() will add to the list in files, so just call
                   it again.
                */
                if(objType.equals(Shuf.DB_VNMR_RECORD)) {
                    suffix = Shuf.DB_REC_SUFFIX;
                    suffix = suffix.toLowerCase();
                    getFileListing(files, prefix, suffix, dir, recursive,
                                   objType);
                }
            }
            if(DebugOutput.isSetFor("fillATable"))
                Messages.postDebug("fillATable Checking " + files.size() +
                                   " files");
            java.util.Date starttime2 = new java.util.Date();
            java.util.Date endtime2;
            long timems2;

            /* Now  loop thur the files adding them to the DB. */
            for(int i=0; i < files.size(); i++) {
                file = (UNFile) files.get(i);
                filename = file.getName();
                // The CanonicalPath is the real absolute path NOT using
                // any symbolic links.  It is the real path.
                try {
                    fullpath = file.getCanonicalPath();

                    // If this fullpath is for a system file,
                    // set the user to varian
                    String sysdir = System.getProperty("sysdir");
                    if(sysdir != null) {
                        UNFile sysfile = new UNFile(sysdir);
                        if(sysfile != null) {
                            // Need to compare the canonical path
                            String csysdir = sysfile.getCanonicalPath();
                            if(fullpath.startsWith(csysdir + "/"))
                                user = "agilent";
                        }
                    }
                }
                catch (Exception e) {
                    Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                         "Problem getting cononical path for " +
                                         filename);
                    Messages.writeStackTrace(e, "Error caught in fillATable");
                    return null;
                }

                try {
                    // If workspace, be sure user is actual owner of the file.
                    if(objType.equals(Shuf.DB_WORKSPACE)) {
                        String owner = getUnixOwner(fullpath);

                        if(!owner.equals(user)) {
                            Messages.postDebug("Cannot add " + fullpath
                                            + "\n   to DB.  It is not owned by "
                                            + user);
                            continue;
                        }
                    }

                    // Add each of these to the DB.
                    // success=false is okay, it may just mean that the entry
                    // is already in the DB.
                    if(objType.equals(Shuf.DB_VNMR_DATA) ||
                       objType.equals(Shuf.DB_WORKSPACE) ||
                       objType.equals(Shuf.DB_VNMR_PAR)) {
                        success =  addVnmrFileToDB(filename, fullpath, user,
                                                   objType, sleepMs, attrName, attrVal, null);
                        getStudyParentAndSet(objType, fullpath);

                        // If we are in a thread and we want to cause mimimal
                        // interruption, sleep between  files.  wait less if
                        // not successful.  That usually means the file already
                        // exists and nothing is being done.

                        if(sleepMs > 0 && !success) {
                            try {
                                Thread.sleep(sleepMs/2);
                            } catch(Exception e) {}
                        }
                        else if(sleepMs > 0) {
                            try {
                                Thread.sleep(sleepMs);
                            } catch(Exception e) {}
                        }
                    }
                    else if(objType.equals(Shuf.DB_IMAGE_DIR) ||
                            objType.equals(Shuf.DB_COMPUTED_DIR)) {
                        success = addImageDirToDB(filename, fullpath, user,
                                                 objType, sleepMs);
                        getStudyParentAndSet(objType, fullpath);
                    }
                    else if(objType.equals(Shuf.DB_VNMR_RECORD))
                        success = addRecordToDB(filename, fullpath, user,
                                                 objType, sleepMs);
                    else if(objType.equals(Shuf.DB_PANELSNCOMPONENTS)) {
                        Object ret = addXmlFileToDB(filename, fullpath, user,
                                                 objType);
                        if(ret == null)
                            success = false;
                        else
                            success = true;
                    }
                    else if(objType.equals(Shuf.DB_SHIMS))
                        success = addVnmrFileToDB(filename, fullpath, user,
                                                  objType, sleepMs, null);
                    else if(objType.equals(Shuf.DB_STUDY) || 
                            objType.equals(Shuf.DB_LCSTUDY)) {
                        success = addStudyToDB(filename, fullpath, user,
                                                  objType, sleepMs);
                    }
                    else if(objType.equals(Shuf.DB_AUTODIR))
                        success = addAutodirToDB(filename, fullpath, user,
                                                  objType, sleepMs);
                    else if(objType.equals(Shuf.DB_PROTOCOL)) {
                        DBCommunInfo dbInfo = addXmlFileToDB(filename, fullpath, user,
                                                 objType);
                        if(dbInfo == null)
                            success = false;
                        else {
                            // For Protocols, we have to get attr from .xml and procpar
                            // The call to addXmlFileToDB started the process and the
                            // partial information is in info.  Send that to the call to
                            // addVnmrFileToDB and let it finish creating the Insert command
                            // for adding to the DB.
                            // The fullpath and filename passed to addVnmrFile is the .xml 
                            //file and we need the procpar.
                            // It looks like if .xml is  "abc/templates/vnmrj/protocols/myprotocol.xml" 
                            // then the path we need is  "abc/parlib/myprotocol.par"
                            
                            // Create the filename by stripping off the "xml" and adding "par"
                            String parname = filename.substring(0, filename.length() -3);
                            parname = parname + "par";
                            
                            // Create the par full path by stripping off the 
                            // "templates/vnmrj/protocols/myprotocol.xml" and 
                            // adding "parlib/myprotocol.par"
                            int index = fullpath.indexOf("templates/vnmrj");
                            if(index > -1) {
                                String parpath = fullpath.substring(0, index);
                                parpath = parpath + "parlib/" + parname;

                                success = addVnmrFileToDB(parname, parpath, user, objType, 0, dbInfo);
                            }
                            else {
                                // Just debug output while looking for a problem.  It shouldn't happen
                                Messages.postDebug("Problem trying to remove \"templates/vnmrj\" "
                                                   + "from fullpath (" + fullpath 
                                                   + ") in fillATable() while adding protocols.");
                            }
                        }  
                    }
                    else if(objType.equals(Shuf.DB_TRASH))
                        success = addTrashFileToDB(fullpath);

                    if(success) {
                        info.numFilesAdded++;
                        if(info.numFilesAdded%20 == 0)
                            System.out.print(".");
                        if(info.numFilesAdded%500 == 0) {
                            System.out.print("\n");
                            if(DebugOutput.isSetFor("locTiming")) {
                                endtime2 = new java.util.Date();
                                timems2 = endtime2.getTime() -
                                    starttime2.getTime();
                                Messages.postDebug("Time for 500 " + objType
                                                   + " additions "
                                                   + "(ms): " + timems2);
                                // Restart for next timing cycle
                                starttime2 = new java.util.Date();
                            }

                        }
                        // Debug output showing each file added
                        if(DebugOutput.isSetFor("fillATableV")) {
                            Messages.postDebug("fillATable added\n    " +
                                               fullpath);
                        }




                        // Future additions and tests to see if files are
                        // already in the DB go faster if it is vacuumed
                        // now and then.
                        if(filesAdded%10000 == 0 || filesAdded == 50)
                            vacuumDB(objType);

                        filesAdded++;

                    }
                    else {
                        // Output a , every so often for files checked.
                        // This is to keep it from looking like nothing is
                        // going on while it checks 1000's of files.
                        if(totFiles > 0 && totFiles%100 == 0)
                            System.out.print(",");
                        if(totFiles > 0 && totFiles%1000 == 0)
                            System.out.print("\n");


                        // Debug output showing each file not added
                        if(DebugOutput.isSetFor("fillATableV")) {
                            Messages.postDebug("fillATable: file apparently " +
                                              "already in DB:\n    "+ fullpath);
                        }
                    }
                    totFiles++;  // Number of files tested
                }
                catch (Exception ex) {
                    if(!ExitStatus.exiting()) {
                        Messages.writeStackTrace(ex, "Problem in fillATable()");
                        // continue with the for loop of files if not exiting.
                    }
                    else
                        // vnmrj is shutting down, just return.
                        return files;
                }
                Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
                String dhost  = (String) mp.get(Shuf.HOST);
                String dpath = (String) mp.get(Shuf.PATH);

                // Skip .gz files.  They shouldn't be here anyway.
                if(dpath.endsWith(".gz"))
                    continue;

                hostFullpath = dhost + ":" + dpath;
                // Is this file in the DB and are we being requested
                // to set anything?
                
                boolean unique = isDBKeyUnique(objType, hostFullpath);
                if(attrName != null && !unique) {
                    // The problem is that if there are studies
                    // within studies, they will be added in a recursive manner.
                    // In doing so, we can end up setting the study attribute
                    // to the higher level study, and we want it set to the
                    // lowest level study (closest parent).  Thus, we want to
                    // set the attribute to the lowest level which will be
                    // the longest path.
                    String val = getAttributeValue(objType, fullpath, localHost,
                                                   attrName);
                    // If the new val is longer, set it
                    if(val == null || (attrVal.length() > val.length())) {
                        // If an attrName was passed in, set that attr to
                        // attrVal. The attr must be allowed in isParamInList()
                        setNonTagAttributeValue(objType, fullpath, localHost,
                                                attrName, "text", attrVal);
                    }
                }

            }
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                           "Problem with file\n    \'" + fullpath +
                           "\' \n    while filling " +objType +" table in DB");
                Messages.postMessage(OTHER|ERROR|LOG,
                     "fillATable failed: objType = " + objType + " dir = " +
                     dir + " user = " + user + " sleepMs = " +  sleepMs +
                     "\nfullpath = " + fullpath +
                     " DBCommunInfo Class = " + info);
                Messages.writeStackTrace(e);
            }
        }

        if(DebugOutput.isSetFor("fillATable") ||
                       DebugOutput.isSetFor("locTiming")) {
            java.util.Date endtime = new java.util.Date();
            long timems = endtime.getTime() - starttime.getTime();
            Messages.postDebug("fillATable: Time(ms) adding "
                               + filesAdded + " " + objType + " files and testing "
                               + totFiles + " files = " + timems);
        }

        if(filesAdded > 0) {
            // Send notification that this table of the DB has been modified
            sendTableModifiedNotification(objType);
        }

        return files;
    }

    public void updateMacro(String objType) {
        long prevdate=0;
        long curdate=0;
        String filepath;
        ArrayList list;
        String inLine;
        BufferedReader in;
        String datepath;

        // This is just to determine if the list is empty or not.
        if(attrList == null)
            return;
        
        
        list= attrList.getAttrValueList("name", objType);

        if(objType.equals(Shuf.DB_COMMAND_MACRO)) {
            filepath = FileUtil.openPath("LOCATOR/commands.xml");
            // Note: use savePath in case this file does not exist yet.
            // In that case, we will create it below.
            datepath = FileUtil.savePath("USER/.macro_date_stamp");
        }

        else if(objType.equals(Shuf.DB_PPGM_MACRO)) {
            filepath = FileUtil.openPath("LOCATOR/pulse_sequence_macros.xml");
            // Note: use savePath in case this file does not exist yet.
            // In that case, we will create it below.
            datepath = FileUtil.savePath("USER/.ppgm_macro_date_stamp");
        }
        else
            return;

        if(list != null && list.size() != 0) {
            // If new date on xml file, do it.

            // Get the date stamp from the file
            UNFile f = new UNFile(filepath);
            curdate = f.lastModified();
            prevdate = 0;

            if(datepath != null) {
                // Open the  file.
                try {
                    in = new BufferedReader(new FileReader(datepath));
                    try {
                        // Read one line at a time.
                        while ((inLine = in.readLine()) != null) {
                            // Date is in sec since epoch
                            prevdate = Long.valueOf(inLine).longValue();
                        }
                        in.close();
                    }
                    catch(Exception e) { }
                }
                catch(Exception e) {
                    // Either file doesn't exit or cannot open it.
                    // Either way, leave prevdate  set to 0 and continue.
                }
            }

            // Write out the curdate to the file.
            try {
                FileWriter fw = new FileWriter(datepath);
                String str = String.valueOf(curdate);
                fw.write(str, 0, str.length());
                fw.close();
                // Change the mode to all write access
                String[] cmd = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION,
                                "chmod 666 " + datepath};
                Runtime rt = Runtime.getRuntime();
                Process prcs = null;
                try {
                    prcs = rt.exec(cmd);
                }
                finally {
                    // It is my understanding that these streams are left
                    // open sometimes depending on the garbage collector.
                    // So, close them.
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
            }
            catch(Exception e) { }
        }
        // Only refill if the date stamp changed or table is empty.
        if(list != null && (list.size() == 0  || prevdate != curdate)) {
            // Fill the DB
            fillMacro(objType);
        }

    }

    /************************************************** <pre>
     * Summary: Fill the command_n_macros table.
     *
     *  Create a single sql command for each command/macro
     *  which will look something like:
     *    INSERT INTO command_n_macro (name, seqtype, cmdtype, dimension,
     *    exectype) VALUES ('flipflop', 'solids', 'pulsesequence', '1D',
     *    'macro')
     *  where the first list in () are the attribute names and
     *  the second list in () are the values go go with the names.
     </pre> **************************************************/
    public void fillMacro(String objType) {
        ArrayList commandDefinitionList;
        CommandDefinition cd;
        String name;
        Hashtable attribList;
        int    numAttr;
        String key;
        String filepath;
        int numCmds;
        boolean ok=true;
        DBCommunInfo info;

        if(objType.equals(Shuf.DB_COMMAND_MACRO))
            filepath = FileUtil.openPath("LOCATOR/commands.xml");
        else
            filepath = FileUtil.openPath("LOCATOR/pulse_sequence_macros.xml");


        commandDefinitionList = new ArrayList();

        // Parse the file
        try {
            CommandBuilder.build(commandDefinitionList, filepath);
        }
        catch (Exception e) {}

        // Clear out the current command_n_macro table
        clearOutTable(objType, "all");

        numCmds = commandDefinitionList.size();
        // Loop thru each command/macro and add one at a time
        // Create a single sql command for each one.
        for(int i=0; i < numCmds; i++) {
            cd = (CommandDefinition)commandDefinitionList.get(i);
            name = cd.getcommandName();

            // Initialize a row adding sequence
            info = addRowToDBInit(objType, name, filepath);

            // add the "name" attribute.
            if(info != null) {
                ArrayList List = new ArrayList();
                List.add(name);
                ok = addRowToDBSetAttr(objType, "name", "text",
                                             List, name, info);
            }
            else
                return;

            // Get attributes and values to set
            attribList = cd.getattrList();
            numAttr = attribList.size();
            Enumeration en = attribList.keys();

            // put the names of the attributes in order that the values
            // will follow below.
            for(int k=0; k < numAttr; k++) {
                // Get the attribute name and add the name to the list.
                key = (String)en.nextElement();

                if(ok) {
                    // All Macro/command attributes are text type.
                    // If this ever changes, we have to determine what
                    // the type is.  For example any attribute starting
                    // with "time_" could be set to date.
                    ArrayList List = new ArrayList();
                    List.add(attribList.get(key));
                    ok = addRowToDBSetAttr(objType, key, "text",
                                           List , name, info);
                }
            }
            addRowToDBExecute(objType, name, info);
        }
    }

    /************************************************** <pre>
     * addRowToDBInit(), addRowToDBSetAttr() and addRowToDBExecute()
     * Add a row with all its attributes.  Add a column if necessary.
     *
     *    Setting attributes in the db one at a time is very time
     *    consuming.  This function Creates one long command setting
     *    multiple attributes, even though it is called once for each
     *    attribute.  It assembles the SQL command in a static and
     *    keeps adding attributes to the command until "first_post"
     *    is set to "POST", meaning this is the call after all
     *    request are complete and it is time to send the SQL command.
     *
     *    A command will look something like the following:
     *      INSERT INTO command_n_macro (name, seqtype, cmdtype, dimension,
     *      exectype) VALUES ('flipflop', 'solids', 'pulsesequence', '1D',
     *      'macro')
     *  where the first list in () are the attribute names and
     *  the second list in () are the values to go with the names.
     *
     *  Call addRowToDBInit() One time to start the command assembly
     *  Call addRowToDBSetAttr() one time per attribute being set.
     *       For the command_n_macro table, "name" must be one of the
     *       attributes which is set.  For other tables, host_fullpath
     *       must be set.
     *  Call addRowToDBExecute() once after all attributes are entered
     *       to execute the assembled sql command.
     *
     </pre> **************************************************/
    public DBCommunInfo addRowToDBInit(String objType,
                                           String host_fullpath,
                                           String fullpath) {
        long fileStatus;
        DBCommunInfo info = new DBCommunInfo();

        /* We are going to create one very long string by appending each
           attribute. Then the string will be sent to the DB. */

        // Be sure we do not duplicate entries.
        fileStatus = isDBKeyUniqueAndUpToDate(objType, host_fullpath, fullpath);
        // Do we need to update it?
        if(fileStatus == -1) {
                removeEntryFromDB(objType, host_fullpath, info);
        }
        // Is is unique?
        else  if(fileStatus == 0) {
            // Key is not unique, But it is up to date, return null.
            return null;
        }

        // Front part of the command string.
        info.addCmd = new StringBuffer ("INSERT INTO " + objType + " (");

        // End of cmd string with all of the values
        info.cmdValueList = new StringBuffer(") VALUES (");

        // Save the host_fullpath to be sure we don't change tables
        // in mid stream.
        info.hostFullpath = host_fullpath;

        // Reset flags since we are starting a new command string
        info.firstAttr = true;
        info.keySet = false;

        return info;
    }


    /************************************************** <pre>
     * addRowToDBInit(), addRowToDBSetAttr() and addRowToDBExecute()
     * Add a row with all its attributes.  Add a column if necessary.
     *
     *  Args:
     *    - attrType.  This is needed if we have to create a new column.
     *  Output:
     *    - return true for success and false for failure.
     *  Comment:
     *    If the attribute is new, add a new column to the db.
     *
     *    Setting attributes in the db one at a time is very time
     *    consuming.  This set of functions creates one long command setting
     *    multiple attributes, even though it is called once for each
     *    attribute.  It assembles the SQL command in a static and
     *    keeps adding attributes to the command until addRowToDBExecute()
     *    is called to execute the assembled command.
     *
     *    A command will look something like the following:
     *        INSERT INTO command_n_macro (name, seqtype, cmdtype, dimension,
     *        exectype) VALUES ('flipflop', 'solids', 'pulsesequence', '1D',
     *        'macro')
     *  where the first list in () are the attribute names and
     *  the second list in () are the values to go with the names.
     *
     </pre> **************************************************/
    public boolean addRowToDBSetAttr(String objType, String attr,
                                     String attrType, ArrayList values,
                                     String host_fullpath,
                                     DBCommunInfo info) {
        int index;
        String newValue;
        boolean foundit;

        if(info.hostFullpath == null) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem adding " + host_fullpath + " to DB");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "addRowToDBInit() must be called before " +
                                 "calling addRowToDBSetAttr()" +
                                  "\n    Problem occured in " + host_fullpath);
            return false;
        }
     // This Test fails when we are adding procpar attributes to a protocol
     // because info.hostFullpath is for the protocol.xml file
     // and host_fullpath is for parlib/protocol.par file.
     // I think this testing was done in the early stages of locator development
     // and is not necessary now, so just stop doing it. GRS 10/12/11
//        if(!info.hostFullpath.equals(host_fullpath)) {
//            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
//                                 "Problem adding " + host_fullpath + " to DB");
//            Messages.postMessage(OTHER|ERROR|LOG,
//                                 "addRowToDBSetAttr: objType changed after " +
//                                 "call to addRowToDBInit()" +
//                                  "\n    Problem occured in " + host_fullpath);
//            return false;
//        }

        // host_fullpath must be one of the attributes set.  Update the
        // flag if it is taken care of.
        if(objType.endsWith(Shuf.DB_MACRO) && attr.equals("name"))
            info.keySet = true;
        else if(attr.equals("host_fullpath"))
           info.keySet = true;

        // Be sure we do not already have this attr in the command
        // else we get an error and fail to add the file at all.
        // If the attr already exist, let the first one win.
        // Do not give error if attr dup is owner.
        // Do not give output if image_file, because it normally has
        // dups between the .img parents procpar and the .fdf header
        if(info.attrList.contains(attr)) {
//            if(!attr.equals("owner") && !objType.equals(Shuf.DB_IMAGE_FILE)) {
                // Messages.postMessage(OTHER|WARN|LOG,
                //                  "addRowToDBSetAttr: Duplicate attr named " +
                //                  attr + " found when adding\n    " +
                //                  host_fullpath +
                //                  "\n    skipping the second entry.");
//            }
            // return with or without error
            return true;
        }


        /* See if this attribute exists in the db. If not, it will be
           added if possible.
        */

        foundit = isAttributeInDB(objType, attr, attrType, host_fullpath);
        /* If unsucessful, error exit. */
        if(!foundit)
            return false;


        // if attritute is a time and is nothing but whitespace, then
        // skip it.
        if(attr.startsWith("time_")) {
            if(((String)values.get(0)).trim().length() == 0) {
                return false;
            }
        }
        /* if the value has a single quote character in it, it screws up the SQL
           command.      Trap them and skip them. */
        try {
            for(int i=0; i < values.size(); i++) {
                index = 0;
                String value = (String)values.get(i);
                if(value != null) {
                    while((index = value.indexOf('\'', index)) != -1) {
                        // copy value to newValue, skipping each single quote
                        newValue = value.substring(0, index);
                        newValue = newValue.concat(value.substring(index+1,
                                                            value.length()));
                        values.remove(i);
                        values.add(i, newValue);
                        value = new String(newValue);
                    }
                }
            }
        }
        catch(Exception e) {
            Messages.writeStackTrace(e);
            return false;
        }


        // If we are on the first one, do not put a leading comma
        if(!info.firstAttr) {
            info.cmdValueList.append(", ");
            info.addCmd.append(", ");
        }

        // Put quotes around the name so that posgres does not force the
        // attr name to lower case.
        info.addCmd.append("\"" + attr + "\"");
        info.attrList.add(attr);

        // If array type, need values within {}s
        if(attrType.endsWith("[]")) {
            info.cmdValueList.append("\'{");
            for(int i=0; i < values.size(); i++) {
                if(i != 0)
                    info.cmdValueList.append(", ");
                String value = (String)values.get(i);
                info.cmdValueList.append("\"").append(value).append("\"");
            }
            info.cmdValueList.append("}\'");
        }
        // If not array, put single value in string.
        else {
            if(values.size() == 0) {
                info.cmdValueList.append("\'\'");
            }
            else 
                info.cmdValueList.append("\'").append(values.get(0)).append("\'");
        }
        info.firstAttr = false;

        return true;
    }


    /************************************************** <pre>
     * addRowToDBInit(), addRowToDBSetAttr() and addRowToDBExecute()
     * Add a row with all its attributes.  Add a column if necessary.
     *
     *    Setting attributes in the db one at a time is very time
     *    consuming.  This set of functions creates one long command setting
     *    multiple attributes, even though it is called once for each
     *    attribute.  It assembles the SQL command in a static and
     *    keeps adding attributes to the command until addRowToDBExecute()
     *    is called to execute the assembled command.
     *
      </pre> **************************************************/
    public boolean addRowToDBExecute(String objType, String host_fullpath,
                                     DBCommunInfo info) {
        boolean status=true;

        // Have things been set up?
        if(info.hostFullpath == null)
            return false;

        if(objType.endsWith(Shuf.DB_MACRO) && !info.keySet) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem adding " + host_fullpath + " to DB");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "addRowToDBExecute: name MUST be one" +
                                 " of the attributes set to add a row." +
                                 "\n    Problem occured in " + host_fullpath);
        }
        else if(!info.keySet){
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem adding " + host_fullpath + " to DB");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "addRowToDBExecute: host_fullpath MUST be one"+
                                  " of the attributes set to add a row." +
                                  "\n    Problem occured in " + host_fullpath);
        }
// This Test fails when we are adding procpar attributes to a protocol
// because info.hostFullpath is for the protocol.xml file
// and host_fullpath is for parlib/protocol.par file.
// I think this testing was done in the early stages of locator development
// and is not necessary now, so just stop doing it. GRS 10/12/11
//        if(!info.hostFullpath.equals(host_fullpath)) {
//            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
//                                 "Problem adding " + host_fullpath + " to DB");
//            Messages.postMessage(OTHER|ERROR|LOG,
//                                 "addRowToDB: objType changed between calls" +
//                                  "\n    Problem occured in " + host_fullpath);
//            return false;
//        }
        // Close the attr values list.
        info.cmdValueList.append(")");

        // Create the full command
        info.addCmd.append(info.cmdValueList);

        // If sql debug is specified, this will help know where the sql
        // command came from when it gets printed out in executeUpdate()
        if(DebugOutput.isSetFor("sqlCmd"))
            Messages.postDebug("addRowToDBExecute()");

        try {
            executeUpdate(info.addCmd.toString());
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem adding" + host_fullpath + " to DB");
                Messages.writeStackTrace(e,"Error caught in addRowToDBExecute");
            }
            status = false;
        }

        info.hostFullpath = "";
        info.keySet = false;
        return status;
    }


    /************************************************** <pre>
     * See if this attribute exists in the db.  If not, it will be
     * added as a new column if possible, unless attrType is null.
     *
     *  Return:
     *      true  if attribute is already in the db or if we sucessfully
     *            added it to the db.
     *      false if we could not add this column.
     *
     </pre> **************************************************/

    public boolean isAttributeInDB(String objType, String attr,
                                   String attrType, String fullpath) {
        boolean foundit=false;


        // These are not in the LocAttrList table, but they should
        // already be in the table from its creation.
        if(attr.startsWith("long_time"))
            return true;

        if(DebugOutput.isSetFor("isAttributeInDB")) {
            Messages.postDebug("isAttributeInDB called for " + attr
                               + " of type " + attrType + " in objType "
                               + objType);
        }

        foundit = isAttributeInDB(objType, attr);

        /* If this attr is not found, add it to the table. */
        if(!foundit  && attrType != null) {

            try {
                if(DebugOutput.isSetFor("isAttributeInDB")) {
                    Messages.postDebug("isAttributeInDB attempting to add "
                                       + attr + " to " + objType + " table");
                }
                /* Add the new column to the table. */
                executeUpdate("ALTER table "  + objType + " ADD \"" +
                              attr + "\" " + attrType);

                // add to attrList
                ArrayList attrValues = new ArrayList();
                // Add this attribute with an empty list of values.
                attrValues.add("");
                attrList.updateValues(attr, attrValues, objType);
            }
            catch (Exception e) {
                if(!ExitStatus.exiting()) {
                    Messages.postMessage(OTHER|ERROR|MBOX|LOG,
                                     "Problem adding " + fullpath + " to DB");
                    Messages.postMessage(OTHER|ERROR|LOG,
                                     "isAttributeInDB: Failed to add " +
                                     attr + " to DB");
                    Messages.writeStackTrace(e,
                                             "Error caught in isAttributeInDB");
                }

                // If we failed at adding the row, return false
                return false;
            }
            // Now it is in the DB, so return true
            return true;
        }
        else {
            if(!foundit  && attrType == null)
                return false;
        }

        return true;
    }

    /************************************************** <pre>
     * See if this attribute exists in the db.
     *
     *  Return:
     *      true  if attribute is already in the db
     *      false if not in the DB
     *
     </pre> **************************************************/

    public boolean isAttributeInDB(String objType, String attr) {
        ArrayList list;

        list = attrList.getAllAttributeNames (objType);
        /* Get each attribute name, one at a time and test it. */
        for(int i=0; i < list.size(); i++) {
            if(attr.equals(list.get(i))) {
                return true;
            }
        }
        return false;
    }


    /************************************************** <pre>
     * Summary: Be sure host_fullpath is unique.  Do not allow duplicates.
     *
     *   Send a search command for this host_fullpath.
     *   If the results have 0 rows, then it is unique.
     *
     </pre> **************************************************/
    public boolean isDBKeyUnique(String objType, String key) {
        java.sql.ResultSet rs;
        String cmd;

        if(objType.equals("unknown") || objType.equals("directory") ||
                objType.equals("file"))
            return true;
        
        if(objType.endsWith(Shuf.DB_MACRO))
            cmd = new String("SELECT name from " + objType +
                             " WHERE name = \'" + key + "\'");
        else
            cmd = new String("SELECT host_fullpath from " + objType +
                             " WHERE host_fullpath = \'" + key + "\'");

        try {
            rs = executeQuery(cmd);
            if(rs == null)
                return false;

            if(rs.next()) {
                                // Found a row, not unique.
                return false;
            }
            else {
                                // No row found, must be unique.
                return true;
            }

        } catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem with " + key + " in DB");
                Messages.writeStackTrace(e, "Error caught in isDBKeyUnique");
            }
            haveConnection=false;
        }

        return false;
    }

    /************************************************** <pre>
     * Summary: Determine whether this DBKey exists and if so if it
     *          is up to date.
     *
     *          Return: 1  =>  Key is unique
     *                  0  =>  Key is not unique, But it is up to date
     *                  -1 =>  Key is not unique, And it is out of date
     *
     </pre> **************************************************/
    public long isDBKeyUniqueAndUpToDate(String objType, String key,
                                         String fullpath) {
        java.sql.ResultSet rs;
        String cmd;
        long dbdate=0, filedate;
        UNFile file;
        int status;

        if(objType.equals("unknown") || objType.equals("directory") ||
                objType.equals("file"))
            return 1;

        if(objType.endsWith(Shuf.DB_MACRO)) {
            // For macros, don't really look for up to date, it doesn't have it
           cmd = new String("SELECT name from " + objType +
                             " WHERE name = \'" + key + "\'");
        }
        else if(objType.equals(Shuf.DB_VNMR_RECORD) ||
                objType.equals(Shuf.DB_VNMR_REC_DATA) ||
                objType.equals(Shuf.DB_AUTODIR) ||
                objType.equals(Shuf.DB_LCSTUDY) ||
                objType.equals(Shuf.DB_STUDY)) {
            cmd = new String("SELECT host_fullpath, long_time_saved, " +
                             "corrupt from " + objType +
                             " WHERE host_fullpath = \'" + key + "\'");
        }
        else {
            cmd = new String("SELECT host_fullpath, long_time_saved from " +
                            objType + " WHERE host_fullpath = \'" + key + "\'");
        }

        try {
            rs = executeQuery(cmd);
            if(rs == null)
                //  Not found, is unique
                return 1;

            if(rs.next()) {
                // Found a row, not unique.

                if(objType.endsWith(Shuf.DB_MACRO))
                    return 0;

                // Here is the date as a long from the DB for this entry
                dbdate = rs.getLong(2);

                file = new UNFile(fullpath);
                // Here is the current date when this file was last modified.
                filedate = file.lastModified()/1000;

                if(dbdate != filedate)
                    // Not unique,  Not up to Date
                    status = -1;
                else
                    // Not unique, Up to Date
                    status = 0;

                if(objType.equals(Shuf.DB_VNMR_RECORD) ||
                   objType.equals(Shuf.DB_VNMR_REC_DATA) ||
                   objType.equals(Shuf.DB_AUTODIR) ||
                   objType.equals(Shuf.DB_LCSTUDY) ||
                   objType.equals(Shuf.DB_STUDY)) {

                    // We need to see if the checksum.flag file
                    // existance matched the DB entry value.  If not, then
                    // tag as out of date.
                    String value;
                    value = rs.getString(3);

                    file = new UNFile(fullpath + File.separator +
                                    "checksum.flag");
                    // If
                    if(file.exists() && value.equals("corrupt") &&
                                                             status == 0)
                        status = 0;
                    else if(!file.exists() && value.equals("valid") &&
                                                             status == 0)
                        status = 0;
                    else
                        status = -1;
                }
            }
            else {
                                // No row found, must be unique.
                status = 1;
            }

        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem with " + key + " in DB");
                Messages.writeStackTrace(e,
                                   "Error caught in isDBKeyUniqueAndUpToDate");
            }
            status = -1;
            haveConnection = false;
        }

        return status;
    }



    /************************************************** <pre>
     * Summary: Destroy the vnmr DataBase completely.
     *
     </pre> **************************************************/
    public boolean destroyDB() {
        Runtime rt;
        Process chkit=null;
        String upath="";
        String cmd;

//        if(sql == null)
//            return false;

        // The persistence file for attrList will be out of date it the
        // DB is destroyed and rebuild, get rid of it.
        LocAttrList.removePersistenceFile();

        try {
            closeDBConnection();

            rt = Runtime.getRuntime();


            String sysdir = FileUtil.sysdir();

            // For Windows, the pg_ctl cmds require the -D option with
            // the path to the DB starting with /SFU in unix style.
            // Start with getting the sysdir in unix style.
            if(UtilB.OSNAME.startsWith("Windows")) {
                UNFile file = new UNFile(sysdir);
                upath = file.getCanonicalPath();
                upath = upath + "/pgsql/data";
                if(upath.startsWith(UtilB.SFUDIR_INTERIX)) {
                    // Remove it, but then add the /SFU back on
                    upath = upath.substring(
                        UtilB.SFUDIR_INTERIX.length(),upath.length());
                    upath = "/SFU" + upath;
                }
            }

            // If any process is connected to the DB, it cannot be destroyed.
            // To force everyone to be disconnected, kill the daemon and
            // then restart the daemon.  Then drop the DB.

            if(UtilB.OSNAME.startsWith("Windows")) 
                cmd = sysdir + "/pgsql/bin/pg_ctl.exe stop -m fast -D " +upath;
            else {
                // See if the /vnmr/pgsql/bin/pg_ctl exists.
                // if so, use it, if not, call pg_ctl without a fullpath.
                // This is to allow us to run the system postgres by
                // just moving /vnmr/pgsql/bin to a saved name out of the way.
                UNFile file = new UNFile("/vnmr/pgsql/bin/pg_ctl");
                if(file.canExecute()) {
                    // Use our released one
                    cmd = "/vnmr/pgsql/bin/pg_ctl stop -m fast";
                }
                else {
                    // Use the system one
                    cmd =  "pg_ctl stop -m fast";
                }
            }

            if(DebugOutput.isSetFor("destroydb"))
               Messages.postDebug("destroyDB stopping daemon, cmd:" + cmd);
            String[] cmds = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION, cmd};
  
            try {
                chkit = rt.exec(cmds);
                // Wait here as long is the process is alive.
                chkit.waitFor();
            }
            finally {
                // It is my understanding that these streams are left
                // open sometimes depending on the garbage collector.
                // So, close them.
                if(chkit != null) {
                    OutputStream os = chkit.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = chkit.getInputStream();
                    if(is != null)
                        is.close();
                    is = chkit.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }



            if(UtilB.OSNAME.startsWith("Windows")) 
                cmd = sysdir + "/pgsql/bin/pg_ctl.exe start -l " + sysdir + 
                    File.separator + "pgsql/pgsql.log -o -i -D " + upath;
            else {
                // See if the /vnmr/pgsql/bin/pg_ctl exists.
                // if so, use it, if not, call pg_ctl without a fullpath.
                // This is to allow us to run the system postgres by
                // just moving /vnmr/pgsql/bin to a saved name out of the way.
                UNFile file = new UNFile("/vnmr/pgsql/bin/pg_ctl");
                if(file.canExecute()) {
                    // Use our released one
                    cmd = "/vnmr/pgsql/bin/pg_ctl start -l " + sysdir + File.separator +
                        "pgsql/pgsql.log";
                }
                else {
                    // Use the system one
                    cmd =  "pg_ctl start -l " + sysdir + File.separator +
                        "pgsql/pgsql.log";
                }
            }
            if(DebugOutput.isSetFor("destroydb"))
                Messages.postDebug("destroyDB re-starting  daemon, cmd:" + cmd);
            String cmdss[] = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION, cmd};

            try {
                chkit = rt.exec(cmdss);
                // Wait here as long is the process is alive.
                chkit.waitFor();
            }
            finally {
                // It is my understanding that these streams are left
                // open sometimes depending on the garbage collector.
                // So, close them.
                if(chkit != null) {
                    OutputStream os = chkit.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = chkit.getInputStream();
                    if(is != null)
                        is.close();
                    is = chkit.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }


            Thread.sleep(500);
            if(DebugOutput.isSetFor("destroydb"))
               Messages.postDebug("destroyDB executing 'dropdb vnmr'");

            if(UtilB.OSNAME.startsWith("Windows")) 
                cmd = sysdir + "/pgsql/bin/dropdb.exe " + dbName;
            else
                cmd = "dropdb " + dbName;

            try {
                chkit = rt.exec(cmd);
                // Wait here as long is the process is alive.
                chkit.waitFor();
            }
            finally {
                // It is my understanding that these streams are left
                // open sometimes depending on the garbage collector.
                // So, close them.
                if(chkit != null) {
                    OutputStream os = chkit.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = chkit.getInputStream();
                    if(is != null)
                        is.close();
                    is = chkit.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
        }
        catch (Exception e) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem destroying the DB. ");
            Messages.writeStackTrace(e, "Error caught in destroyDB");
            return false;
        }

        if(DebugOutput.isSetFor("destroydb"))
            Messages.postDebug("DB Destroyed");

        Messages.postMessage(OTHER|INFO|LOG, "DB Destroyed");

        return true;

    }


    /************************************************** <pre>
     * Summary: Create a new vnmr DB and create empty tables.
     *
     </pre> **************************************************/
    public boolean createEmptyDBandTables() {
        String          cmd;
        ShufDataParam   par;
        Runtime         rt;
        Process         chkit=null;
        boolean         status;

        // The persistence file for attrList will be out of date it the
        // DB is destroyed and rebuild, get rid of it.
        LocAttrList.removePersistenceFile();

        if (locatorOff() && !managedb)
            return false;
        /* Now create the database anew */
        try {
            rt = Runtime.getRuntime();

            String dir = System.getProperty("sysdir");

            if(UtilB.OSNAME.startsWith("Windows"))
                // What do we for system installed postgresql
                // IF we go back to having Windows having a locator. ???
                cmd = dir + "/pgsql/bin/createdb.exe " + dbName;
            else {
                // See if the /vnmr/pgsql/bin/createdb exists.
                // if so, use it, if not, call createdb without a fullpath.
                // This is to allow us to run the system postgres by
                // just moving /vnmr/pgsql/bin to a saved name out of the way.
                UNFile file = new UNFile("/vnmr/pgsql/bin/createdb");
                if(file.canExecute()) {
                    // Use our released one
                    cmd = "/vnmr/pgsql/bin/createdb " + dbName;
                }
                else {
                    // Use the system one
                    cmd =  "createdb " + dbName;
                }
            }
            String[] cmds = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION, cmd};
            try {
                chkit = rt.exec(cmds);
                // Wait here as long is the process is alive.
                chkit.waitFor();
            }
            finally {
                // It is my understanding that these streams are left
                // open sometimes depending on the garbage collector.
                // So, close them.
                if(chkit != null) {
                    OutputStream os = chkit.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = chkit.getInputStream();
                    if(is != null)
                        is.close();
                    is = chkit.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
        }
        catch (Exception e) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.writeStackTrace(e,
                                     "Error caught in createEmptyDBandTables");
            return false;
        }

        // Connect to the new dB
        status = makeDBConnection();
        if(status == false)
            return false;

        /* Fill the param_list with params to put into shuffler. */
        shufDataParams = ReadParamListFile(Shuf.DB_VNMR_DATA, false);
        shufStudyParams = ReadParamListFile(Shuf.DB_STUDY, false);
        shufProtocolParams = ReadParamListFile(Shuf.DB_PROTOCOL, false);
        shufImageParams = ReadParamListFile(Shuf.DB_IMAGE_DIR, false);

        /* Now create the tables. */
        /* DB_VNMR_DATA table */

        /* We need to create it with all of the attributes in shufDataParams
         */

        // DB_VNMR_DATA
        /* Create the start of the command. */
        cmd = new String("create table " + Shuf.DB_VNMR_DATA + " (");

        /* Now add in all of the attributes in shufDataParams. */
        for(int i=0; i < shufDataParams.size(); i++) {
            par = (ShufDataParam)shufDataParams.get(i);

            // The param temp is not usable in postgres, so change the
            // string to "temperature".
            if(par.equals("temp"))
                par.setParam(par.getParam().concat("erature"));

            // Put quotes around the attr name, otherwise, postgres will
            // convert upper case to lower case.
            cmd = cmd.concat("\"" + par.getParam() + "\" "  + par.getType());
            if(i == shufDataParams.size() -1)
                /* End of command, primary key will not allow duplicates
                   of this column. */
                cmd = cmd.concat(", PRIMARY KEY(host_fullpath));");
            else
                 cmd = cmd.concat(",");
        }

        try {
            int istatus = executeUpdate(cmd);
            if(istatus == 0) {
                Messages.postError("Problem creating DB table on cmd: " + cmd);
                return false;
            }
            executeUpdate("GRANT ALL ON " + Shuf.DB_VNMR_DATA +
                          " TO PUBLIC");
// Adding indices, add substancially to the time it takes to add fids
// to the DB.  It does not seem to help with the search time, so don't
// bother doing it.
//             executeUpdate("CREATE INDEX fn_inx on vnmr_data (filename)");
//             executeUpdate("CREATE INDEX ow_inx on vnmr_data (owner)");
//             executeUpdate("CREATE INDEX fp_inx on vnmr_data (fullpath)");
//             executeUpdate("CREATE INDEX seq_inx on vnmr_data (seqfil)");
        }
        catch (Exception e) {
            haveConnection = false;
            if(!locatorOff()) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
                Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
                Messages.writeStackTrace(e);
            }

            return false;
        }

        // DB_VNMR_PAR
        /* Create the start of the command. */
        cmd = new String("create table " + Shuf.DB_VNMR_PAR + " (");

        /* Now add in all of the attributes in shufDataParams. */
        for(int i=0; i < shufDataParams.size(); i++) {
            par = (ShufDataParam)shufDataParams.get(i);

            // The param temp is not usable in postgres, so change the
            // string to "temperature".
            if(par.equals("temp"))
                par.setParam(par.getParam().concat("erature"));

            // Put quotes around the attr name, otherwise, postgres will
            // convert upper case to lower case.
            cmd = cmd.concat("\"" + par.getParam() + "\" " + par.getType());
            if(i == shufDataParams.size() -1)
                /* End of command, primary key will not allow duplicates
                   of this column. */
                cmd = cmd.concat(", PRIMARY KEY(host_fullpath));");
            else
                 cmd = cmd.concat(",");
        }
        try {
            int istatus = executeUpdate(cmd);
            if(istatus == 0) {
                Messages.postError("Problem creating DB table on cmd: " + cmd);
                return false;
            }
            executeUpdate("GRANT ALL ON " + Shuf.DB_VNMR_PAR +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);

            return false;
        }


        // DB_IMAGE_DIR
        /* Create the start of the command. */
        cmd = new String("create table " + Shuf.DB_IMAGE_DIR + " (");

        /* Now add in all of the attributes in shufImageParams. */
        for(int i=0; i < shufImageParams.size(); i++) {
            par = (ShufDataParam)shufImageParams.get(i);

            // The param temp is not usable in postgres, so change the
            // string to "temperature".
            if(par.equals("temp"))
                par.setParam(par.getParam().concat("erature"));

            // Put quotes around the attr name, otherwise, postgres will
            // convert upper case to lower case.
            cmd = cmd.concat("\"" + par.getParam() + "\" " + par.getType());
            if(i == shufImageParams.size() -1)
                /* End of command, primary key will not allow duplicates
                   of this column. */
                cmd = cmd.concat(", PRIMARY KEY(host_fullpath));");
            else
                 cmd = cmd.concat(",");
        }

        try {
            int istatus = executeUpdate(cmd);
            if(istatus == 0) {
                Messages.postError("Problem creating DB table on cmd: " + cmd);
                return false;
            }
            executeUpdate("GRANT ALL ON " + Shuf.DB_IMAGE_DIR +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);

            return false;
        }

        // DB_VNMR_RECORD
        /* Create the start of the command. */
        cmd = new String("create table " + Shuf.DB_VNMR_RECORD + " (");
                         

        /* Now add in all of the attributes in shufDataParams. */
        for(int i=0; i < shufDataParams.size(); i++) {
            par = (ShufDataParam)shufDataParams.get(i);

            // The param temp is not usable in postgres, so change the
            // string to "temperature".
            if(par.equals("temp"))
                par.setParam(par.getParam().concat("erature"));

            cmd = cmd.concat("\"" + par.getParam() + "\" " + par.getType());
            if(i == shufDataParams.size() -1)
                /* End of command, primary key will not allow duplicates
                   of this column. */
                cmd = cmd.concat(", PRIMARY KEY(host_fullpath));");
            else
                 cmd = cmd.concat(",");
        }
        try {
            int istatus = executeUpdate(cmd);
            if(istatus == 0) {
                Messages.postError("Problem creating DB table on cmd: " + cmd);
                return false;
            }
            executeUpdate("GRANT ALL ON " + Shuf.DB_VNMR_RECORD +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);

            return false;
        }

        // DB_VNMR_REC_DATA
        /* Create the start of the command. */
        cmd = new String("create table " + Shuf.DB_VNMR_REC_DATA + " (");

        /* Now add in all of the attributes in shufDataParams. */
        for(int i=0; i < shufDataParams.size(); i++) {
            par = (ShufDataParam)shufDataParams.get(i);

            // The param temp is not usable in postgres, so change the
            // string to "temperature".
            if(par.equals("temp"))
                par.setParam(par.getParam().concat("erature"));

            cmd = cmd.concat("\"" + par.getParam() + "\" " + par.getType());
            if(i == shufDataParams.size() -1)
                /* End of command, primary key will not allow duplicates
                   of this column. */
                cmd = cmd.concat(", PRIMARY KEY(host_fullpath));");
            else
                 cmd = cmd.concat(",");
        }
        try {
            int istatus = executeUpdate(cmd);
            if(istatus == 0) {
                Messages.postError("Problem creating DB table on cmd: " + cmd);
                return false;
            }
            executeUpdate("GRANT ALL ON " + Shuf.DB_VNMR_REC_DATA +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);

            return false;
        }

        /* DB_WORKSPACE table */
        /* Create the start of the command. */
        cmd = new String("create table " + Shuf.DB_WORKSPACE + " (");

        /* Now add in all of the attributes in shufDataParams. */
        for(int i=0; i < shufDataParams.size(); i++) {
            par = (ShufDataParam)shufDataParams.get(i);
            cmd = cmd.concat("\"" + par.getParam() + "\" " + par.getType());
            if(i == shufDataParams.size() -1)
                /* End of command, primary key will not allow duplicates
                   of this column. */
                cmd = cmd.concat(", PRIMARY KEY(host_fullpath));");
            else
                 cmd = cmd.concat(",");
        }

        try {
            int istatus = executeUpdate(cmd);
            if(istatus == 0) {
                Messages.postError("Problem creating DB table on cmd: " + cmd);
                return false;
            }
            executeUpdate("GRANT ALL ON " + Shuf.DB_WORKSPACE +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

        /* panelsNcomponents table */
        cmd = new String( "create table " + Shuf.DB_PANELSNCOMPONENTS +
                          " (host_fullpath text, " +
                          "filename text, " +
                          "fullpath text, " +
                          "owner text, " +
                          "hostname text, " +
                          "hostdest text, " +
                          "arch_lock int, " +
                          "element_type text, " +
                          "element text, " +
                          "panel_type text, " +
                          "type text, " +
                          "comment text, " +
                          "name text, " +
                          "directory text, " +
                          "language text, " +
                          "long_time_saved" + " " + DB_ATTR_INTEGER + ", " +
                          "tag0 text, " +
                          Shuf.DB_TIME_SAVED + " " + DB_ATTR_DATE + ", " +
                          "PRIMARY KEY(host_fullpath)" +
                          ")");

        try {
            executeUpdate(cmd);
            executeUpdate("GRANT ALL ON " + Shuf.DB_PANELSNCOMPONENTS +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

        /* DB_SHIMS table */
        cmd = new String( "create table " + Shuf.DB_SHIMS +
                          " (host_fullpath text, " +
                          "filename text, " +
                          "fullpath text, " +
                          "owner text, "  +
                          "investigator text, "  +
                          "hostname text, "  +
                          "hostdest text, " +
                          "shims text, " +
                          "probe text, " +
                          "directory text, " +
                          "arch_lock int, " +
                          "long_time_saved" + " " + DB_ATTR_INTEGER + ", " +
                          Shuf.DB_TIME_SAVED +  " " + DB_ATTR_DATE + ", " +
                          "solvent text, " +
                          "tag0 text, " +
                          "PRIMARY KEY(host_fullpath)" +
                          ")");

        try {
            executeUpdate(cmd);
            executeUpdate("GRANT ALL ON " + Shuf.DB_SHIMS +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

        /* DB_COMMAND_MACRO table */
        cmd = new String( "create table " + Shuf.DB_COMMAND_MACRO +
                          " (name text, " +
                          "tag0 text, " +
                          "PRIMARY KEY(name)" +
                          ")");

        try {
            executeUpdate(cmd);
            executeUpdate("GRANT ALL ON " + Shuf.DB_COMMAND_MACRO +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

        /* DB_PPGM_MACRO table */
        cmd = new String( "create table " + Shuf.DB_PPGM_MACRO +
                          " (name text, " +
                          "label text, " +
                          "tag0 text, " +
                          "PRIMARY KEY(name)" +
                          ")");

        try {
            executeUpdate(cmd);
            executeUpdate("GRANT ALL ON " + Shuf.DB_PPGM_MACRO +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

        /* DB_STUDY table */
        /* Create the start of the command. */
        cmd = new String("create table " + Shuf.DB_STUDY + " (");

        /* Now add in all of the attributes in shufStudyParams. */
        for(int i=0; i < shufStudyParams.size(); i++) {
            par = (ShufDataParam)shufStudyParams.get(i);

            // The param temp is not usable in postgres, so change the
            // string to "temp ".
            if(par.equals("temp"))
                par.setParam(par.getParam().concat("erature"));

            cmd = cmd.concat("\"" + par.getParam() + "\" " + par.getType());
            if(i == shufStudyParams.size() -1)
                cmd = cmd.concat(");"); /* End of command */
            else
                 cmd = cmd.concat(",");
        }

        try {
            int istatus = executeUpdate(cmd);
            if(istatus == 0) {
                Messages.postError("Problem creating DB table on cmd: " + cmd);
                return false;
            }
            executeUpdate("GRANT ALL ON " + Shuf.DB_STUDY +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

        /* DB_LCSTUDY table */
        /* Create the start of the command. */
        cmd = new String("create table " + Shuf.DB_LCSTUDY + " (");

        /* Now add in all of the attributes in shufStudyParams. */
        for(int i=0; i < shufStudyParams.size(); i++) {
            par = (ShufDataParam)shufStudyParams.get(i);

            // The param temp is not usable in postgres, so change the
            // string to "temp ".
            if(par.equals("temp"))
                par.setParam(par.getParam().concat("erature"));

            cmd = cmd.concat("\"" + par.getParam() + "\" " + par.getType());
            if(i == shufStudyParams.size() -1)
                cmd = cmd.concat(");"); /* End of command */
            else
                 cmd = cmd.concat(",");
        }

        try {
            int istatus = executeUpdate(cmd);
            if(istatus == 0) {
                Messages.postError("Problem creating DB table on cmd: " + cmd);
                return false;
            }
            executeUpdate("GRANT ALL ON " + Shuf.DB_LCSTUDY +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

        /* DB_AUTODIR table */
        cmd = new String( "create table " + Shuf.DB_AUTODIR +
                          " (host_fullpath text, " +
                          "datatype text, " +
                          "directory text, " +
                          "fullpath text, " +
                          "filename text, " +
                          "owner text, "  +
                          "investigator text, "  +
                          "hostname text, "  +
                          "hostdest text, " +
                          "subtype text, "  +
                          "corrupt text, "  +
                          "tag0 text, " +
                          "arch_lock int, " +
                          "long_time_saved" + " " + DB_ATTR_INTEGER + ", " +
                          Shuf.DB_TIME_SAVED +  " " + DB_ATTR_DATE + ", " +
                          "PRIMARY KEY(host_fullpath)" +
                          ")");

        try {
            int istatus = executeUpdate(cmd);
            if(istatus == 0) {
                Messages.postError("Problem creating DB table on cmd: " + cmd);
                return false;
            }
            executeUpdate("GRANT ALL ON " + Shuf.DB_AUTODIR +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }


        /* DB_PROTOCOL table */
        /* Create the start of the command. */
        cmd = new String("create table " + Shuf.DB_PROTOCOL + " (");

        /* Now add in all of the attributes in shufProtocolParams. */
        for(int i=0; i < shufProtocolParams.size(); i++) {
            par = (ShufDataParam)shufProtocolParams.get(i);

            // The param temp is not usable in postgres, so change the
            // string to "temp ".
            if(par.equals("temp"))
                par.setParam(par.getParam().concat("erature"));

            cmd = cmd.concat("\"" + par.getParam() + "\" " + par.getType());

            // If adding sfrq, then we need to create "field" also to
            // hold the Tesla field associated with the sfrq freq.
// Actually, field should be in the protocol_param_list.img file
// in which case, it will be added.  If added here, we can
// end up with it twice and get an error.
//            if(par.getParam().equals("sfrq")) {
//                cmd = cmd.concat(",field float");
//            }
            if(i == shufProtocolParams.size() -1)
                cmd = cmd.concat(");"); /* End of command */
            else
                 cmd = cmd.concat(",");
        }

        try {
            int istatus = executeUpdate(cmd);
            if(istatus == 0) {
                Messages.postError("Problem creating DB table on cmd: " + cmd);
                return false;
            }
            executeUpdate("GRANT ALL ON " + Shuf.DB_PROTOCOL +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

        /* DB_AVAIL_SUB_TYPES table */
        // All objTypes that can be subtypes should be entered into
        // this table with the objType name as the key value in types.
        // There should then be an attribute name for each objType that
        // can have subtypes. The value for each of these will be yes or null
        // for each types. For example
        //     types has values of study, vnmr_data, record, etc
        //     for types = study, attr study = yes, attr record = null
        //                        attr vnmr_data = yes, attr shim = null etc.
        //     This says that studies can have subtypes of study and vnmr_data.
        // filename, fullpath, hostname, owner and arch_lock are not used, but
        // must exist for the search to succeed.

        cmd = new String( "create table " + Shuf.DB_AVAIL_SUB_TYPES +
                          " (types text, " +
                          "blank1 text, " +
                          "blank2 text, "  +
                          "filename text, "  +
                          "fullpath text, "  +
                          "hostname text, "  +
                          "hostdest text, "  +
                          "owner text, "  +
                          "arch_lock int, " +
                          Shuf.DB_TIME_RUN + " " + DB_ATTR_DATE + ", " +
                          "PRIMARY KEY(types)" +
                          ")");
        try {
            executeUpdate(cmd);
            executeUpdate("GRANT ALL ON " + Shuf.DB_AVAIL_SUB_TYPES +
                          " TO PUBLIC");

        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Tables");
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

//         /* DB_PRESCRIPTION table */
//         cmd = new String( "create table " + Shuf.DB_PRESCRIPTION +
//                           " (host_fullpath text, " +
//                           "filename text, " +
//                           "fullpath text, " +
//                           "owner text, "  +
//                           "hostname text, "  +
//                           "tag0 text, " +
//                           "arch_lock int, " +
//                           "long_time_saved" + " " + DB_ATTR_INTEGER + ", " +
//                           Shuf.DB_TIME_SAVED +  " " + DB_ATTR_DATE + ", " +
//                           "PRIMARY KEY(host_fullpath)" +
//                           ")");

//         try {
//             executeUpdate(cmd);
//             executeUpdate("GRANT ALL ON " + Shuf.DB_PRESCRIPTION +
//                           " TO PUBLIC");
//         }
//         catch (Exception e) {
//             haveConnection = false;
//             Messages.postMessage(OTHER|ERROR|LOG|MBOX,
//                                  "Problem creating empty DB Tables");
//             Messages.postMessage(OTHER|ERROR|LOG,
//                                  "Problem in createEmptyDBandTables " +
//                                  "with command: " + cmd);
//             Messages.writeStackTrace(e);
//             return false;
//         }

//        /* DB_GRADIENTS table */
//         cmd = new String( "create table " + Shuf.DB_GRADIENTS +
//                           " (host_fullpath text, " +
//                           "filename text, " +
//                           "fullpath text, " +
//                           "owner text, "  +
//                           "hostname text, "  +
//                           "tag0 text, " +
//                           "arch_lock int, " +
//                           "long_time_saved" + " " + DB_ATTR_INTEGER + ", " +
//                           Shuf.DB_TIME_SAVED +  " " + DB_ATTR_DATE + ", " +
//                           "PRIMARY KEY(host_fullpath)" +
//                           ")");

//         try {
//             executeUpdate(cmd);
//             executeUpdate("GRANT ALL ON " + Shuf.DB_GRADIENTS +
//                           " TO PUBLIC");
//         }
//         catch (Exception e) {
//             haveConnection = false;
//             Messages.postMessage(OTHER|ERROR|LOG|MBOX,
//                                  "Problem creating empty DB Tables");
//             Messages.postMessage(OTHER|ERROR|LOG,
//                                  "Problem in createEmptyDBandTables " +
//                                  "with command: " + cmd);
//             Messages.writeStackTrace(e);
//             return false;
//         }

        /* DB_TRASH table */
        cmd = new String( "create table " + Shuf.DB_TRASH +
                          " (host_fullpath text, " +
                          "filename text, " +
                          "fullpath text, " +
                          "orig_fullpath text, " +
                          "orig_host_fullpath text, "  +
                          "objtype text, " +
                          "hostname text, "  +
                          "hostdest text, " +
                          "owner text, "  +
                          "tag0 text, " +
                          "datetrashed " + DB_ATTR_DATE + ", " +
                          "arch_lock int, " +
                          "long_time_saved" + " " + DB_ATTR_INTEGER + ", " +
                          "PRIMARY KEY(host_fullpath)" +
                          ")");

        try {
            executeUpdate(cmd);
            executeUpdate("GRANT ALL ON " + Shuf.DB_TRASH +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Table, " +
                                 Shuf.DB_TRASH);
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }
        /* DB_VERSION table */
        cmd = new String( "create table " + Shuf.DB_VERSION +
                          " (hostname text, " +
                          "version text, " +
                          "networkMode text, " +
                          "vnmrjversion text" +
                          ");");

        try {
            executeUpdate(cmd);
            executeUpdate("GRANT ALL ON " + Shuf.DB_VERSION +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Table, " +
                                 Shuf.DB_VERSION);
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

        // Get value for networkMode
        String networkMode = determineNetworkMode();
        String vnmrjversion = getVnmrjVersion();
        String version = getPostgresVersion();
        /* Add version to DB_VERSION */
        cmd = new String("INSERT INTO " + Shuf.DB_VERSION +
                         " VALUES (\'" + localHost + "\', \'" +
                         version + "\', \'" + networkMode + "\', \'" + 
                         vnmrjversion + "\');");

        try {
            executeUpdate(cmd);
        }
        catch (Exception e) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem adding" + Shuf.DB_VERSION + " to DB");
            Messages.writeStackTrace(e);
        }

        /* NOTIFY table */
        try {
            cmd = "create table notify (host text, PRIMARY KEY(host), ";
            // Now add a column for the name of each objType vnmr
            // plans to use this notification for.
            for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
                cmd = cmd.concat(Shuf.OBJTYPE_LIST[i] + " text ");

                // No trailing comma
                if(i != Shuf.OBJTYPE_LIST.length -1)
                    cmd = cmd.concat(", ");
                else
                    cmd = cmd.concat(")");
            }
            executeUpdate(cmd);
            executeUpdate("GRANT ALL ON notify" +
                          " TO PUBLIC");
            // add a row to the DB for this host
            cmd = "INSERT INTO notify (host) VALUES (\'" + localHost + "\')";
            executeUpdate(cmd);
        }
        catch(Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Table, " +
                                 Shuf.DB_LINK_MAP);
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }


        /* DB_LINK_MAP table */
        cmd = new String( "create table " + Shuf.DB_LINK_MAP +
                          " (dhost_dpath text, " +
                          " dhost text, " +
                          " dpath text, " +
                          " PRIMARY KEY(dhost_dpath));");

        try {
            executeUpdate(cmd);
            executeUpdate("GRANT ALL ON " + Shuf.DB_LINK_MAP +
                          " TO PUBLIC");
        }
        catch (Exception e) {
            haveConnection = false;
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem creating empty DB Table, " +
                                 Shuf.DB_LINK_MAP);
            Messages.postMessage(OTHER|ERROR|LOG,
                                 "Problem in createEmptyDBandTables " +
                                 "with command: " + cmd);
            Messages.writeStackTrace(e);
            return false;
        }

        Messages.postMessage(OTHER|INFO|LOG, "Empty DB Created");
        return true;
    }

    public String getVnmrjVersion() {
        String strPath = FileUtil.openPath(FileUtil.SYS_VNMR+"/vnmrrev");
        BufferedReader reader;
        String strLine;
        StringTokenizer tok;
        String verNum="", revision="";

        try {
            reader = new BufferedReader(new FileReader(strPath));
            strLine = reader.readLine();
            tok = new StringTokenizer(strLine);
            if(tok.countTokens() < 3) {

            }
            // Skip the first 2 tokens which should be Vnmrj and VERSION
            tok.nextToken();
            tok.nextToken();
            // The third one should be the version basic number
            verNum = tok.nextToken().trim();
            // If there is a revision, there will be 2 more tokens
            if(tok.countTokens() > 1) {
                // Skip 'REVISION' string
                tok.nextToken();
                // get the revision letter or string
                revision = tok.nextToken().trim();
                // Only keep the first character
                revision = revision.substring(0, 1);
            }
        }
        catch(Exception ex) {
            // Something went wrong.  Log to the log file and return 
            // empty string
            Messages.postLog("getVnmrjVersion: Problem getting vnmrj version from /vnmr/vnmrrev");
        
        }

        verNum = verNum + revision;

        return verNum;     
    }


    // This is only to be called by createEmptyDBandTables which should
    // in turn only be called if PGHOST = the local host.  In that case,
    // the only reason to have networkMode other than false, is if we
    // have been directed to do so by finding a file
    //   /usr/varian/config/NMR_NETWORK_DB
    private String determineNetworkMode() {
        String usingNetServer;
        UNFile file;

        if(UtilB.OSNAME.startsWith("Windows"))
            file = new UNFile(UtilB.SFUDIR_WINDOWS +
                              "/usr/varian/config/NMR_NETWORK_DB");
        else
            file = new UNFile("/usr/varian/config/NMR_NETWORK_DB");
        if(file.exists())
            usingNetServer = "true";
        else
            usingNetServer = "false";


        return usingNetServer;

    }

    /************************************************** <pre>
     * Summary: Read the shuffler_param_list file and fill a list with
     *          all of the params to be used.
     *
     *  If limited = false
     *      Create a full lists using all of the different appmode files.
     *  If limited = true
     *      Create a limited menu list using the users local files if they
     *      exist, else using the file for the current appmode.
     *
     </pre> **************************************************/
    public HashArrayList ReadParamListFile(String objType, boolean limited) {
        String          filepath;
        String          filename;
        BufferedReader  in;
        String          param, type, inLine=null;
        StringTokenizer tok;
        ShufDataParam   par;
        boolean         dup;
        HashArrayList   paramList;

        paramList = new HashArrayList();

        /* Go ahead and add default params. */
        paramList.put("host_fullpath",
                           new ShufDataParam("host_fullpath", DB_ATTR_TEXT));
        paramList.put("filename",
                           new ShufDataParam("filename", DB_ATTR_TEXT));
        paramList.put("fullpath",
                           new ShufDataParam("fullpath", DB_ATTR_TEXT));
        paramList.put("directory",
                           new ShufDataParam("directory", DB_ATTR_TEXT));
        paramList.put("owner",
                           new ShufDataParam("owner", DB_ATTR_TEXT));
        paramList.put("hostname",
                           new ShufDataParam("hostname", DB_ATTR_TEXT ));
        paramList.put("hostdest",
                           new ShufDataParam("hostdest", DB_ATTR_TEXT ));
        paramList.put("arch_lock",
                           new ShufDataParam("arch_lock", DB_ATTR_INTEGER));
        paramList.put("long_time_saved",
                           new ShufDataParam("long_time_saved",
                                             DB_ATTR_INTEGER));
        paramList.put("time_created",
                           new ShufDataParam("time_created",
                                             DB_ATTR_DATE));
        paramList.put(Shuf.DB_TIME_RUN,
                           new ShufDataParam(Shuf.DB_TIME_RUN,
                                             DB_ATTR_DATE));
        paramList.put(Shuf.DB_TIME_SAVED,
                           new ShufDataParam(Shuf.DB_TIME_SAVED,
                                             DB_ATTR_DATE));
        // Several objTypes use shufDataParams which is filled based on
        // the type DB_VNMR_DATA
        if(objType.equals(Shuf.DB_VNMR_DATA)) {
            paramList.put("pslabel",
                          new ShufDataParam("pslabel", DB_ATTR_TEXT));
            paramList.put("operator_",
                          new ShufDataParam("operator_",DB_ATTR_TEXT ));
            paramList.put("operator",
                          new ShufDataParam("operator", DB_ATTR_TEXT));
            paramList.put("investigator",
                          new ShufDataParam("investigator", DB_ATTR_TEXT));
            paramList.put("dataid",
                          new ShufDataParam("dataid", DB_ATTR_TEXT));
            paramList.put(Shuf.DB_AUTODIR,
                          new ShufDataParam(Shuf.DB_AUTODIR, DB_ATTR_TEXT));
            paramList.put(Shuf.DB_STUDY,
                          new ShufDataParam(Shuf.DB_STUDY, DB_ATTR_TEXT));
            paramList.put(Shuf.DB_LCSTUDY,
                          new ShufDataParam(Shuf.DB_LCSTUDY, DB_ATTR_TEXT));
            paramList.put("corrupt",
                          new ShufDataParam("corrupt", DB_ATTR_TEXT));
            paramList.put("fda",
                          new ShufDataParam("fda", DB_ATTR_TEXT));
            paramList.put("record",
                          new ShufDataParam("record", DB_ATTR_TEXT));
            paramList.put("exp",
                          new ShufDataParam("exp", DB_ATTR_INTEGER));
            // This autodir is to hold the DB_AUTODIR information as the local
            // mounted path.
            paramList.put("autodir",
                          new ShufDataParam("autodir", DB_ATTR_TEXT));
        }
        
        if(objType.equals(Shuf.DB_STUDY) || objType.equals(Shuf.DB_LCSTUDY)) {
            paramList.put(Shuf.DB_AUTODIR,
                          new ShufDataParam(Shuf.DB_AUTODIR, DB_ATTR_TEXT));
            // This autodir is to hold the DB_AUTODIR information as the local
            // mounted path.
            paramList.put("autodir",
                          new ShufDataParam("autodir", DB_ATTR_TEXT));
            paramList.put(Shuf.DB_STUDY,
                          new ShufDataParam(Shuf.DB_STUDY, DB_ATTR_TEXT));
            paramList.put(Shuf.DB_LCSTUDY,
                          new ShufDataParam(Shuf.DB_LCSTUDY, DB_ATTR_TEXT));
            paramList.put("corrupt",
                          new ShufDataParam("corrupt", DB_ATTR_TEXT));
        }
        if(objType.equals(Shuf.DB_IMAGE_DIR)) {
            // Hold parent's path for DB_IMAGE_FILE
            paramList.put(Shuf.DB_IMAGE_DIR,
                          new ShufDataParam(Shuf.DB_IMAGE_DIR, DB_ATTR_TEXT)); 
            paramList.put(Shuf.DB_STUDY,
                          new ShufDataParam(Shuf.DB_STUDY, DB_ATTR_TEXT));
        }
        if(objType.equals(Shuf.DB_PROTOCOL)) {
            paramList.put("language", 
                          new ShufDataParam("language", DB_ATTR_TEXT));
            paramList.put("operator_",
                          new ShufDataParam("operator_",DB_ATTR_TEXT ));
            paramList.put("operator",
                          new ShufDataParam("operator", DB_ATTR_TEXT));
            paramList.put("investigator",
                          new ShufDataParam("investigator", DB_ATTR_TEXT));

        }
        /* Now add some tag columns. */
        for(int k=0; k < NUM_TAGS; k++) {
            paramList.put("tag" + Integer.toString(k),
                               new ShufDataParam("tag" + Integer.toString(k),
                                                 DB_ATTR_TEXT));
        }

        // There can be one xxx_param_list for each appmode.  For adding files
        // to the DB, we must have the full list, so we need to accumulate the
        // params from all appmode lists.  For the limited list, we just
        // use the xxx_param_list in LOCATOR/xxx_param_list.


        // Create a list of files to loop through, adding only one for the
        // limited list.  Then the code below can just always loop in the
        // same manner.
        // Start with the beginning of the path
        ArrayList fileList = new ArrayList();
        if(limited) {
            fileList.add("LOCATOR/");
        }
        else {
            fileList.add("SYSTEM/LOCATOR/");
            for(int i=0; i < appmodFileList.length; i++)
                fileList.add("SYSTEM/" + appmodFileList[i]);
        }

        // Now, depending on the objType, we need to add the actual filename
        // to all items in the fileList
        for(int i=0; i < fileList.size(); i++) {
            if(objType.equals(Shuf.DB_VNMR_DATA)) {
                filename = (String) fileList.get(i);
                filename = filename.concat("shuffler_param_list");
                // Replace this position in the list with the full path needed
                fileList.remove(i);
                fileList.add(i, filename);
            }
            else if(objType.equals(Shuf.DB_STUDY)) {
                filename = (String) fileList.get(i);
                filename = filename.concat("study_param_list");
                // Replace this position in the list with the full path needed
                fileList.remove(i);
                fileList.add(i, filename);
            }
            else if(objType.equals(Shuf.DB_PROTOCOL)) {
                filename = (String) fileList.get(i);
                filename = filename.concat("protocol_param_list");
                // Replace this position in the list with the full path needed
                fileList.remove(i);
                fileList.add(i, filename);
            }
            else if(objType.equals(Shuf.DB_IMAGE_DIR) ||
                    objType.equals(Shuf.DB_COMPUTED_DIR)) {
                filename = (String) fileList.get(i);
                filename = filename.concat("image_param_list");
                // Replace this position in the list with the full path needed
                fileList.remove(i);
                fileList.add(i, filename);
            }
            else {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                     "Param list not supported for " + objType);
                return paramList;
            }
        }


        // Now loop through the paths in fileList
        for(int i=0; i < fileList.size(); i++) {
            // Get the absolute path from the utility
            filename = (String) fileList.get(i);
            filepath = FileUtil.openPath(filename);
            if(filepath == null) {
                continue;
            }

            try {
                UNFile file = new UNFile(filepath);
                FileReader fr = new FileReader(file);
                if(fr == null) {
                    continue;
                }
                in = new BufferedReader(fr);
            } catch(Exception e) {
                continue;
            }
            try {
                // Read one line at a time.
                while ((inLine = in.readLine()) != null) {
                    /* skip blank lines and comment lines. */
                    if (inLine.length() > 1 && !inLine.startsWith("#")) {
                        inLine.trim();

                        tok = new StringTokenizer(inLine);
                        if(tok.countTokens() < 2) {
                            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                          "Each non comment line in " +
                                          filepath +
                                          "\n    Must contain two entries" +
                                          " where the first is the param name"+
                                          "\n    and the second is the data" +
                                          " type (text, int, float or date)");
                            in.close();
                            continue;
                        }
                        param = tok.nextToken().trim();
                        type =  tok.nextToken().trim();

                        // Be sure date type is the correct one.
                        if(type.equals("date"))
                            type = DB_ATTR_DATE;

                        // If float, make it float8
                        if(type.startsWith("float") && type.endsWith("[]"))
                            type = DB_ATTR_FLOAT8_ARRAY;
                        else if(type.startsWith("float"))
                            type = DB_ATTR_FLOAT8;
                        // Try making all integers, float8 also
                        else if(type.startsWith("int") && type.endsWith("[]"))
                            type = DB_ATTR_FLOAT8_ARRAY;
                        else if(type.startsWith("int"))
                            type = DB_ATTR_FLOAT8;


                        dup = false;
                        // Be sure this item is not already in the list
                        for(int k=0; k < paramList.size(); k++) {
                            par = (ShufDataParam)paramList.get(k);
                            if(param.equals(par.getParam())) {
                                dup = true;
                                break;
                            }
                        }
                        if(!dup) {
                            // Not a duplicate, add the item.
                            paramList.put(param,new ShufDataParam(param, type));
                        }
                    }
                }
            }
            catch(Exception e) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                     "Problem reading " + filename);
                Messages.postMessage(OTHER|ERROR|LOG,
                                     "ReadParamListFile failed on " + filepath +
                                     " on line: '" + inLine + "'");
                Messages.writeStackTrace(e);

            }

            try {
                in.close();
            }
            catch(Exception e) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                     "Problem closing " + filename);
                Messages.writeStackTrace(e,"Error caught in ReadParamListFile");
            }
        }
        if(DebugOutput.isSetFor("ReadParamListFile"))
            Messages.postDebug("ReadParamListFile Attributes: " +
                               paramList.keySet());

        return paramList;
    }



    /************************************************** <pre>
     * Summary: Return > 0 if the param of this type is in the
     *          list of params to use and 0 if param not to be used.
     *          Return 1 if param is not an array, and 2 if param is an array.
     *
     </pre> **************************************************/
    public int isParamInList(String objType, String param, String type) {
        ShufDataParam   par;
        int             result;

        // For shims, disallow everything starting with x, y or z.
        if(objType.equals(Shuf.DB_SHIMS)) {
            if(param.startsWith("x") ||
                         param.startsWith("y") ||
                         param.startsWith("z"))
                return 0;
            else
                return 1;

        }
        // For Study
        else if(objType.equals(Shuf.DB_STUDY) || 
                objType.equals(Shuf.DB_LCSTUDY)) {
            for(int i=0; i < shufStudyParams.size(); i++) {
                par = (ShufDataParam)shufStudyParams.get(i);
                result = par.equals(param, type);
                if(result > 0)
                    return result;
            }
        }
        // For Protocol
        else if(objType.equals(Shuf.DB_PROTOCOL)) {
            for(int i=0; i < shufProtocolParams.size(); i++) {
                par = (ShufDataParam)shufProtocolParams.get(i);
                result = par.equals(param, type);
                if(result > 0)
                    return result;
            }
        }
        // For DB_IMAGE_DIR and DB_IMAGE_FILE
        else if(objType.equals(Shuf.DB_IMAGE_DIR) || 
                objType.equals(Shuf.DB_COMPUTED_DIR) ||
                objType.equals(Shuf.DB_IMAGE_FILE)) {
            for(int i=0; i < shufImageParams.size(); i++) {
                par = (ShufDataParam)shufImageParams.get(i);
                result = par.equals(param, type);
                if(result > 0)
                    return result;
            }
        }
        // For AutoDir allow everything
        else if(objType.equals(Shuf.DB_AUTODIR)) {
            return 1;
        }
        else {
            for(int i=0; i < shufDataParams.size(); i++) {
                par = (ShufDataParam)shufDataParams.get(i);
                result = par.equals(param, type);
                if(result > 0)
                    return result;
            }
        }
        return 0;
    }

    static boolean inUse=false;
    static boolean haveConnection=false;

    public boolean checkDBConnection() {
        if(haveConnection)
            return true;
        else
            return makeDBConnection();
    }

    // reduce the number of error messages when postgres_deamon is not running.
    static  int errCnt=0;

    public boolean makeDBConnection() {
        String port=null;
        
        // If locator is turned off, just return true
//        if(locatorOff() && !managedb)
        if (locatorOff())
            return false;
        
        if(inUse) {
            return false;
        }
        haveConnection = false;

        // Get the port number
        if(port == null) {
            port = System.getProperty("dbport");
            if(port == null)
                port = "5432";
        }

        
        try { 
            if(newpostgres.equals("true"))
                Class.forName("org.postgresql.Driver");
            else
                Class.forName("postgresql.Driver");
        } 
        catch (ClassNotFoundException e) {
            Messages.postDebug("PostgreSQL JDBC Driver Not Found!");
            e.printStackTrace();
 
        }   
        
        // Create url with host, port and /dbname
        // For some reason Windows had a problem with an actual name
        // for dbhost, but using the string localhost works.
        String url;
        if(dbHost.equals(localHost))
            url = new String("jdbc:postgresql://" + "localhost" + ":" +
                             port + "/" + dbName);
        else
            url = new String("jdbc:postgresql://" + dbHost + ":" +
                             port + "/" + dbName);

        // Get the current user name
        String user = System.getProperty("user.name");
        String passwd = new String("jjj");
//        String passwd = new String("");

        // Connect to database
        try {
            if(DebugOutput.isSetFor("makeDBConnection"))
                Messages.postDebug("makeDBConnection url = " + url + " user = "
                                   + user + " passwd = " + passwd);

            db = DriverManager.getConnection(url, user, passwd);

            if(DebugOutput.isSetFor("makeDBConnection"))
                Messages.postDebug("DriverManager connection = " + db);

            sql = db.createStatement();
        }
        catch (SQLException e) {
            /* Avoid a lot of these messages */
            if (connectMessage == 1) {
               Messages.postError("makeDBConnection failed on " + dbHost +
                            " port " + port + ",\n   be sure database " +
                            "exists and that " + user +
                            " has access to it.\n   Try having the " +
                            "system administrator execute " +
                            "the command 'createuser " + user + "'." +
                            "\n   Also check that if a special port number " +
                            " is used for postgres,\n   that this user " +
                            "has the environment variable \'PGPORT\' set " +
                            " correctly.\n   Is it possible that a specific" +
                            " error can be found in the log file:\n" +
                            "      /vnmr/pgsql/pgsql.log");
               Messages.writeStackTrace(e);
            }


            // Remove the locAttrList persistence file in case it
            // is screwed up
            connectMessage = 0;
            LocAttrList.removePersistenceFile();

            return false;
        }


        connectMessage = 1;
        haveConnection=true;
        inUse=false;

        try {
            executeUpdate("SET DateStyle TO \'ISO\'");
        }
        catch (Exception e) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem connecting to DB.  The locator will" +
                                 " not function.");
            Messages.writeStackTrace(e, "Error caught in makeDBConnection");
            inUse=false;
            return false;
        }


        return true;
    }

    public void closeDBConnection() {
        if(DebugOutput.isSetFor("closeDBConnection"))
           System.out.println("**closeDBConnection**");

        try {
            if(sql != null)
                sql.close();
            if(db != null)
                db.close();
        }
        catch (Exception e) {}
    }


    /************************************************** <pre>
     * Summary: Add a xml file to the database.
     *
     </pre> **************************************************/
    private DBCommunInfo addXmlFileToDB (String filename, String fullpath,
                                   String user, String objType) {
        String          dhostFullpath;
        BufferedReader  in;
        String          inLine=null;
        String          rootFilename;
        String          string=null;
        int             index;
        String          attr;
        String          value;
        boolean         foundName=false;
        StringReader    sr;
        StreamTokenizerQuotedNewlines tok;
        StringTokenizer tokstring;
        long            timeSavedSec;
        UNFile          file;
        int             typeMatch;
        String          directory;
        java.util.Date  timeSavedDate;
        DBCommunInfo    info=null;
        String          dpath, dhost;
        String          language;


        if(DebugOutput.isSetFor("addFileToDB")) {
            Messages.postDebug("addXmlFileToDB(): filename = " + filename +
                            " fullpath = " + fullpath + "\nuser = " +
                            user + " objType = " + objType);
        }
        try {
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost  = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch(Exception e) {
            Messages.postDebug("Problem getting cononical path for " +
                                fullpath);
            Messages.writeStackTrace(e);
            dhost = localHost;
            dpath = fullpath;
        }

        dhostFullpath = new String(dhost + ":" + dpath);

        file = new UNFile(fullpath);

        // Open the file.
        try {
            in = new BufferedReader(FileUtil.getUTFReader(file));
        } catch(Exception e) {
            Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem opening " + fullpath);
            return null;
        }

        timeSavedSec = file.lastModified();

        // strip suffix off of the filename before adding to DB.
        index = filename.lastIndexOf('.');
        rootFilename = filename.substring(0, index);

        // While we are at it, see if there is a language extension
        // such as filename.ja.xml
        language=FileUtil.langExt(rootFilename);
        if(language!=null){
            //rootFilename=rootFilename.replace(language,"");
            language=language.replace(".","");
        }
        else
            language="en";

        // Strip off the filename and create a directory string.
        index = fullpath.lastIndexOf(File.separator);
        if(index > 0)
            directory = fullpath.substring(0, index);
        else
            directory = fullpath;

        // Now get just the last directory level name
        index = directory.lastIndexOf(File.separator);
        if(index > 0)
            directory = directory.substring(index+1);

        // Start the process of adding a row to the DB
        info = addRowToDBInit(objType, dhostFullpath, fullpath);
        // If failure, leave.
        if(info == null) {
            try {
                in.close();
            } catch(Exception e) {}
            return null;
        }

        // Add some standard attributes.
        ArrayList List = new ArrayList();
        List.add(dhostFullpath);
        addRowToDBSetAttr(objType,  "host_fullpath",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(rootFilename);
        addRowToDBSetAttr(objType,  "filename",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(dpath);
        addRowToDBSetAttr(objType,  "fullpath",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(user);
        addRowToDBSetAttr(objType,  "owner",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(localHost);
        addRowToDBSetAttr(objType,  "hostname",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(dhost);
        addRowToDBSetAttr(objType,  "hostdest",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(String.valueOf(timeSavedSec/1000));
        addRowToDBSetAttr(objType,  "long_time_saved",
                              DB_ATTR_INTEGER, List, dhostFullpath, info);
        List.clear();
        List.add(directory);
        addRowToDBSetAttr(objType,  "directory",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(language);
        addRowToDBSetAttr(objType,  "language",
                              DB_ATTR_TEXT, List, dhostFullpath, info);

        List.clear();
        timeSavedDate = new java.util.Date(timeSavedSec);
        // Convert timeSavedDate to date string
        SimpleDateFormat formatter =
                new SimpleDateFormat ("MMM d, yyyy HH:mm:ss");
        String str = formatter.format(timeSavedDate);
        List.add(str);
        addRowToDBSetAttr(objType,  Shuf.DB_TIME_SAVED,
                              DB_ATTR_DATE, List, dhostFullpath, info);

        // Get the line/s to parse from the first occurence of a '<'
        // followed by an alpha character and continuing to the next '>'.
        try {
            // Read one line at a time.
            while ((inLine = in.readLine()) != null) {
               inLine = inLine.trim();
               if (string == null && inLine.length() > 1 &&
                                                inLine.startsWith("<")) {
                   // Is next char a letter?
                   if(Character.isLetter(inLine.charAt(1))) {
                       // We are at the start of our string, save it.
                       string = new String(inLine);

                       // If this line contains the closing '>', then
                       // stop reading now.
                       if ((index = inLine.indexOf('>')) != -1) {
                           // Only take the string up to the '>'
                           string = inLine.substring(0, index);
                           break;
                       }
                   }
               }
               // concatonate to string until '>' is found.
               else if (string != null && inLine.indexOf('>') == -1) {
                   string = string.concat(" " + inLine);
               }
               // Include the line up to the '>'
               else if (string != null && (index = inLine.indexOf('>')) != -1) {
                   // Concatonate up to the '>' and include a space.
                   string = string.concat(" " + inLine.substring(0, index));
                   break;
               }

            }
            in.close();

            if(string == null)
                throw (new Exception("Badly formated xml file"));
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem reading " + fullpath);
                Messages.postMessage(OTHER|ERROR|LOG,
                                 "addXmlFileToDB failed on line: '" +
                                 inLine + "'");
                Messages.writeStackTrace(e);
            }

            try {
                in.close();
            }
            catch(Exception  eio) {}
            return null;
        }
        // We should now have something like
        //    <template panel_type="acquisition" element_type="panels"
        // Tokenize the string.

        // Use StreamTokenizer because we can keep the quoted string
        // as a single token.  StringTokenizer does not allow that.
        // I had to create StreamTokenizerQuotedNewlines to allow quoted
        // string to contain newLines.  The standard one ends the quote
        // at the end of a line.  Sun had a bug reported about this
        // (4239144) and refused to add an option to allow multiple lines
        // in a quoted string.  GRS
        sr = new  StringReader(string);
        tok = new StreamTokenizerQuotedNewlines(sr);
        // Define the quote character
        tok.quoteChar('\"');
        // Define additional char to be treated as normal letter.
        tok.wordChars('_', '_');
        try {
            // Dump the first two tokens.  They will be '<' and the
            // word following '<'.
            tok.nextToken();
            tok.nextToken();
            // Now we should have a sequence of TT_WORD, '=', quoted string
            // for each attribute being defined.
            while(tok.nextToken() != StreamTokenizerQuotedNewlines.TT_EOF) {
                if(tok.ttype != StreamTokenizerQuotedNewlines.TT_WORD) {
                    Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                      "Problem with syntax setting attributes" +
                                      " from the file:\n    " + fullpath);
                    Messages.postMessage(OTHER|ERROR|LOG,
                                 "addXmlFileToDB failed on line: '" +
                                 string + "'");
                    break;  // Bail out
                }
                // Get the name of the attribute being set.
                attr = tok.sval;
                tok.nextToken();
                if(tok.ttype != '=') {
                    Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                  "Problem with syntax (=) setting attributes" +
                                  " from the file:\n" + fullpath);
                    Messages.postMessage(OTHER|ERROR|LOG,
                                 "addXmlFileToDB failed on line: '" +
                                 string + "'");
                    break;  // Bail out
                }
                tok.nextToken();
                if(tok.ttype != '\"') {
                    Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem with syntax (\") setting attributes" +
                                 " from the file:\n" + fullpath);
                    Messages.postMessage(OTHER|ERROR|LOG,
                                 "addXmlFileToDB failed on line: '" +
                                 string + "'");
                    break;  // Bail out
                }
                // Get the value for this attribute.
                value = tok.sval;

                // We cannot add an empty string to the DB so if the value 
                // is empty, make it a single space.
                if(value.length() == 0)
                    value = " ";
                
                if(attr.equals("name"))
                    foundName = true;

                List = new ArrayList();
                List.add(value);



                /* For Protocols, see if we want to put this param in the db
                   or not. Returns 0 for false, 1 for true and
                   2 for true except item in DB is array type.
                */
                if(objType.equals(Shuf.DB_PROTOCOL)) {
                    if(attr.startsWith("time_"))
                        typeMatch = isParamInList(objType, attr, DB_ATTR_DATE);
                    else if(attr.equals("field") || attr.equals("sfrq"))
                        typeMatch = isParamInList(objType, attr, DB_ATTR_FLOAT8);
                    else
                        typeMatch = isParamInList(objType, attr, DB_ATTR_TEXT);
                }
                else
                    typeMatch = 1;

                /* If == 1, the type matches and is not an array. */
                if(typeMatch == 1) {
                    if(attr.startsWith("time_"))
                        addRowToDBSetAttr(objType, attr, DB_ATTR_DATE,
                                          List, dhostFullpath, info);
                    else
                        addRowToDBSetAttr(objType, attr, DB_ATTR_TEXT,
                                          List, dhostFullpath, info);
                }
                /* We have a param match and it is an array type. */
                if(typeMatch == 2) {
                    // We need to parse the string into individual values.
                    // They should be comma separated all in one string.
                    string = new String((String)List.get(0));
                    tokstring = new StringTokenizer(string, ",");
                    // Make a new empty list to fill in
                    List = new ArrayList();

                    while(tokstring.hasMoreTokens()) {
                        List.add(tokstring.nextToken().trim());
                    }

                    addRowToDBSetAttr(objType, attr, DB_ATTR_TEXT_ARRAY,
                                      List, dhostFullpath, info);
                }
            }
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem reading " + fullpath);
                Messages.postMessage(OTHER|ERROR|LOG,
                                 "addXmlFileToDB failed on line: '" +
                                 string + "'");
            }
            return null;

        }

        // If 'name' was not given, use filename.
        if(!foundName) {
            List = new ArrayList();
            List.add(rootFilename);
            addRowToDBSetAttr(objType, "name",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        }

        // Complete the process of adding a row to the DB
        // For Protocol, don't call the execute.  Instead, the caller
        // will pass the returned info to addVnmrFileToDB to get more attributes
        // from the Protocol procpar file.
        if(!objType.equals(Shuf.DB_PROTOCOL))
            addRowToDBExecute(objType, dhostFullpath, info);

        // The returned "info", is only used for Protocols
        return info;
    }

    public boolean addTrashFileToDB(String fullpath) {
        DBCommunInfo info;
        boolean status=true;
        TrashInfo trashInfo;
        String tHostFullpath;
        String dhost, dpath;

        // We need to open and read the trashinfo file from fullpath
        // into a TrashInfo object.
        trashInfo = TrashInfo.readTrashInfoFile(fullpath);

        // null means file was not found
        if(trashInfo == null)
            return false;

        try {
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost  = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch(Exception e) {
            Messages.postDebug("Problem getting canonical path for " +
                                fullpath);
            Messages.writeStackTrace(e);
            dhost = localHost;
            dpath = fullpath;
        }

        // The trashInfo.hostFullpath and trashInfo.fullpath are set to
        // the original values.  We need to add this item with its
        // new path which is the arg fullpath.
        tHostFullpath = new String(dhost + ":" + dpath);

        // Start the process of adding a row to the DB
        info = addRowToDBInit(Shuf.DB_TRASH, tHostFullpath,
                                trashInfo.fullpath);
        // If failure, leave.
        if(info == null) {
            return false;
        }
        // Values are passed as an ArrayList in case they are to be an array.
        // Note: the first arg of addRowToDBSetAttr() is the objType, but
        //       in this case, that is DB_TRASH, not the objType of the
        //       file that has been trashed.
        ArrayList List = new ArrayList();

        List.add(tHostFullpath);
        addRowToDBSetAttr(Shuf.DB_TRASH,  "host_fullpath",
                          DB_ATTR_TEXT, List, tHostFullpath, info);
        List.clear();
        List.add(trashInfo.filename);
        addRowToDBSetAttr(Shuf.DB_TRASH,  "filename",
                          DB_ATTR_TEXT, List, tHostFullpath, info);
        List.clear();
        List.add(trashInfo.fullpath);
        addRowToDBSetAttr(Shuf.DB_TRASH,  "fullpath",
                          DB_ATTR_TEXT, List, tHostFullpath, info);
        List.clear();
        List.add(trashInfo.origFullpath);
        addRowToDBSetAttr(Shuf.DB_TRASH,  "orig_fullpath",
                          DB_ATTR_TEXT, List, tHostFullpath, info);
        List.clear();
        List.add(trashInfo.origHostFullpath);
        addRowToDBSetAttr(Shuf.DB_TRASH,  "orig_host_fullpath",
                          DB_ATTR_TEXT, List, tHostFullpath, info);
        List.clear();
        List.add(trashInfo.owner);
        addRowToDBSetAttr(Shuf.DB_TRASH,  "owner",
                          DB_ATTR_TEXT, List, tHostFullpath, info);
        List.clear();
        List.add(trashInfo.objType);
        addRowToDBSetAttr(Shuf.DB_TRASH,  "objtype",
                          DB_ATTR_TEXT, List, tHostFullpath, info);
        List.clear();
        List.add(localHost);
        addRowToDBSetAttr(Shuf.DB_TRASH,  "hostname",
                          DB_ATTR_TEXT, List, tHostFullpath, info);
        List.clear();
        List.add(trashInfo.dhost);
        addRowToDBSetAttr(Shuf.DB_TRASH,  "hostdest",
                          DB_ATTR_TEXT, List, tHostFullpath, info);
        List.clear();
        SimpleDateFormat formatter =
                new SimpleDateFormat ("MMM d, yyyy HH:mm:ss");
        String str = formatter.format(trashInfo.dateTrashed);
        List.add(str);
        addRowToDBSetAttr(Shuf.DB_TRASH,  "datetrashed",
                          DB_ATTR_DATE, List, tHostFullpath, info);

       UNFile dir = new UNFile(fullpath);
       long timeSavedSec = dir.lastModified();
       // The division by 1000 is to get seconds instead of milliseconds.
        // milliseconds over flows the int.
       List.clear();
       List.add(String.valueOf(timeSavedSec/1000));
       addRowToDBSetAttr(Shuf.DB_TRASH,  "long_time_saved", DB_ATTR_INTEGER,
                             List, tHostFullpath, info);


        status = addRowToDBExecute(Shuf.DB_TRASH,tHostFullpath, info);

        return status;
    }


    /******************************************************************
     * Summary: Add a record to DB_VNMR_RECORD and add all of its
     * subdirectories to DB_VNMR_REC_DATA.
     *
     * For the DB_VNMR_RECORD entry, get params from *.REC/acqfil/curpar
     * For the DB_VNMR_REC_DATA entries, get params from *.REC/X/curpar
     *****************************************************************/

    public boolean addRecordToDB(String filename, String fullpath,
                                 String owner, String objType, long sleepMs){
        boolean     success=false;
        File[]      files;
        UNFile      file;
        String      name, path=null;

        // Add the record itself which will get params from
        // *.REC/acqfil/procpar

        try {
            // Be sure fullpath is the Canonical path
            UNFile recDir = new UNFile(fullpath);
            path = recDir.getCanonicalPath();
            success = addVnmrFileToDB(filename, path, owner,
                                      Shuf.DB_VNMR_RECORD, sleepMs, null);

            if(DebugOutput.isSetFor("addRecordToDB")) {
                if(success)
                    Messages.postDebug("Added Record " + path);
                else
                    Messages.postDebug("Failed to Add Record " + path);
            }

            // Now, add all of the subdirectories under the .REC/.rec directory
            // including acqfil.  This means that basically, acqfil is in the
            // DB twice, once as the params for the record and once as the
            // acqfil file itself.
            files = recDir.listFiles();

            if(files == null)
                return false;

            /* Now  loop thur the files adding them to the DB. */
            for(int i=0; i < files.length; i++) {
                file = (UNFile)files[i];
                name = file.getName();
                // The CanonicalPath is the real absolute path NOT using
                // any symbolic links.  It is the real path.
                path = file.getCanonicalPath();

                success =  addVnmrFileToDB(name, path, owner,
                                           Shuf.DB_VNMR_REC_DATA, sleepMs, null);
                if(DebugOutput.isSetFor("addRecordToDB")) {
                    if(success)
                        Messages.postDebug("Added Record Data " + path);
                    else
                        Messages.postDebug("Failed to Add Record Data " + path);
                }
            }
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem adding " + path + " to DB");
                Messages.writeStackTrace(e);
            }
        }

        return success;

    }



    /******************************************************************
     * Summary: Add an automation to the DB and add all of its
     * subdirectories.
     *
     *****************************************************************/

    public boolean addAutodirToDB(String filename, String fullpath,
                                 String owner, String objType, long sleepMs){
        boolean success=false;
        String subType=null;
        File[]    files;
        UNFile    file;
        String    name, path=null;
        String    autoHostFullpath;
        ArrayList fileList = new ArrayList();
        String dhost, dpath;

        // Add the automation itself
        try {
            // Be sure fullpath is the Canonical path
            UNFile autoDir = new UNFile(fullpath);
            path = autoDir.getCanonicalPath();


            // Create the hostFullpath for this automation to use as the
            // attribute value for automation for the items under this directory.
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost  = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
            autoHostFullpath = new String(dhost + ":" + dpath);

            success = addVnmrFileToDB(filename, path, owner,
                                      Shuf.DB_AUTODIR, sleepMs, null);
            if(!success) {
                // If failure if because the automation exists and is up to date
                // then we need to proceed with looking at the files
                // within this automation.  First see if is is indeed an
                // automation run.
                String autopar = fullpath + File.separator + "autopar";
                UNFile sfile = new UNFile(autopar);
                if(!sfile.exists()) {
                    autopar = fullpath + File.separator + "doneQ";
                    sfile = new UNFile(autopar);
                    if(!sfile.isFile()) {
                        return false;
                    }
                 }

                long status = isDBKeyUniqueAndUpToDate(objType,
                                                autoHostFullpath, fullpath);
                if(status != 0)
                    return false;
            }


            if(DebugOutput.isSetFor("addAutodirToDB")) {
                if(success)
                    Messages.postDebug("Added Autodir " + path);
                else
                    Messages.postDebug("Failed to Add Autodir " + path);
            }

            // Now determine whether this automation contains studies or
            // vnmr_data files.  It should not contain both, just one or the
            // other.  This must not be a recursive search, else I will
            // indeed find the .fids inside of studies.  Do recursive below.
            files = autoDir.listFiles();

            if(files == null)
                return false;


            // Now  loop thur the list and see if any studies or .fids are there
            for(int i=0; i < files.length; i++) {
                file = (UNFile)files[i];
                String fpath = file.getCanonicalPath();

                String type = getType(fpath);
                // If there is a study, use that type even if there are
                // also vnmr_data files, therefore break if one is found
                if(type.equals(Shuf.DB_STUDY)) {
                    subType = Shuf.DB_STUDY;
                    break;
                }
                // If there is a lcstudy, use that type even if there are
                // also vnmr_data files, therefore break if one is found
                if(type.equals(Shuf.DB_LCSTUDY)) {
                    subType = Shuf.DB_LCSTUDY;
                    break;
                }
                if(type.equals(Shuf.DB_VNMR_DATA)) {
                    // If a vnmr_data file if found, set the subType.
                    // Keep cycling to see if a study is found, because
                    // if a study is found, it takes precedent
                    subType = Shuf.DB_VNMR_DATA;
                }
            }
            // If nothing was found, default to study
            if(subType == null)
                subType = Shuf.DB_STUDY;


            // Set attribute 'subtype' for this automation showing what type
            // of data is under it.
            setNonTagAttributeValue(Shuf.DB_AUTODIR, path, localHost, "subtype",
                              "text", subType);

            /* Now we need the recursive file list of files.  If subtype is
               STUDY, then we do not want to include all of the .fids in
               the studies.  They will be taken care of when the studies
               are added. */
            // Get the recursive list.
            if(subType.equals(Shuf.DB_STUDY) || 
               subType.equals(Shuf.DB_LCSTUDY)) {
                // Get the recursive list for studies
                getFileListing(fileList, "", "", fullpath, true, subType);
            }
            else {
                // Get the recursive list for fids
                getFileListing(fileList, "", Shuf.DB_FID_SUFFIX, fullpath,
                               true, Shuf.DB_VNMR_DATA);
            }

            // Check to see if an autostudies file exist.  If so we want to
            // be sure we include all of the files listed in it.  We also
            // want to eliminate duplicates with the list already obtained.
            // The autotsudies file should contain a list of all studies in
            // this automation.  Note that the studies do not have to reside in
            // this automation directory itself, but can be anyplace.
            boolean dup;
            UNFile  newfile;
            String  inLine;
            String  aspath = fullpath + File.separator + "autostudies";
            file = new UNFile(aspath);
            if(file.isFile()) {
                BufferedReader in = new BufferedReader(new FileReader(file));
                try {
                    // Read one line at a time.
                    while ((inLine = in.readLine()) != null) {
                        name = inLine.trim();
                        if(!name.startsWith("/") && name.indexOf(":") < 1) {
                            // Not a root path, we need to prepend fullpath
                            name = fullpath.concat(File.separator + name);
                        }
                        newfile = new UNFile(name);
                        dup = false;
                        for(int i=0; i < fileList.size(); i++) {
                            file = (UNFile) fileList.get(i);
                            if(file.compareTo(newfile) == 0) {
                                dup = true;
                                break;
                            }
                        }
                        if(!dup) {
                            // Was not a duplicate, add it to the list.
                            fileList.add(newfile);
                        }
                    }
                    in.close();
                }
                catch(Exception e) {
                    Messages.postDebug("Problem Reading " + aspath);
                    Messages.writeStackTrace(e);
                    // Now just continue with the list we have already.
                }
            }




            /* Now  loop thur the files adding them to the DB. */
            for(int i=0; i < fileList.size(); i++) {
                file = (UNFile) fileList.get(i);
                // If subtype is study, only proceed if file is a study
                if(subType.equals(Shuf.DB_STUDY)) {
                    // Only proceed if this files is really a study.
                    String studypar = file.getPath() + File.separator +
                                                       "studypar";
                    UNFile sfile = new UNFile(studypar);
                    if(!sfile.exists()) {
                        continue;
                    }
                }
                if(subType.equals(Shuf.DB_LCSTUDY)) {
                    // Only proceed if this files is really a study.
                    String studypar = file.getPath() + File.separator +
                                                       "lcpar";
                    UNFile sfile = new UNFile(studypar);
                    if(!sfile.exists()) {
                        continue;
                    }
                }

                name = file.getName();
                // The CanonicalPath is the real absolute path NOT using
                // any symbolic links.  It is the real path.
                path = file.getCanonicalPath();

                if(subType.equals(Shuf.DB_VNMR_DATA))
                    success =  addVnmrFileToDB(name, path, owner, subType,
                                               sleepMs, null);
                else
                    success =  addStudyToDB(name, path, owner, subType,
                                               sleepMs);

                // Make the hostFullpath for this item.
                mp = MountPaths.getCanonicalPathAndHost(path);
                dhost  = (String) mp.get(Shuf.HOST);
                dpath = (String) mp.get(Shuf.PATH);
                String hFullpath = dhost + ":" + dpath;

                // If the file was successfully added, we need to set the
                // 'automation' attribute showing the parent of this item.
                // If the file is already in the DB, we need to be sure
                // this attribute is set.  The file may have gotten added
                // during an update of the item type itself.
                // success will show false if the path was not the right
                // type as well as if the item is already in the DB.
                // Check the DB itself.
                if(isDBKeyUniqueAndUpToDate(subType, hFullpath, path) < 1) {
                    setNonTagAttributeValue(subType, path, localHost,
                                      Shuf.DB_AUTODIR,
                                      "text", autoHostFullpath);
                    setNonTagAttributeValue(subType, path, localHost,
                                      "autodir",
                                      "text", fullpath);

                }

                if(DebugOutput.isSetFor("addAutodirToDB")) {
                    if(success)
                        Messages.postDebug("Added Autodir Data " + path);
                    else
                        Messages.postDebug("Failed to Add Autodir Data " +path);
                }
                // Output a , every so often for files checked.
                // This is to keep it from looking like nothing is
                // going on while it checks 1000's of files.
                if(i > 0 && i%100 == 0)
                    System.out.print(",");
                if(i > 0 && i%1000 == 0)
                    System.out.print("\n");

            }
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem adding " + path + " to DB");
                Messages.writeStackTrace(e);
            }
        }

        return success;

    }


    /******************************************************************
     * Summary: Add a DB_STUDY or DB_LCSTUDY to the DB and add all of the data 
     * it contains
     *
     *****************************************************************/

    public boolean addStudyToDB(String filename, String fullpath,
                                 String owner, String objType, long sleepMs){
        boolean   success=false;
        UNFile    file;
        String    studyHostFullpath;
        String    parent="";
        DBCommunInfo info = new DBCommunInfo();
        String    dhost, dpath;

        // Add the study or lcstudy itself
        try {
            // Create the hostFullpath for this study to use as the
            // attribute value for study for the items under this directory.
            try {
                Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
                dhost = (String) mp.get(Shuf.HOST);
                dpath = (String) mp.get(Shuf.PATH);
            }
            catch (Exception e) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                   "Problem getting cononical path for\n    " +
                                   fullpath);
                Messages.writeStackTrace(e);
                return false;
            }
            studyHostFullpath = new String(dhost + ":" + dpath);

            success = addVnmrFileToDB(filename, fullpath, owner,
                                      objType, sleepMs, null);
            if(!success){
                // If failure if because the study exists and is up to date
                // then we need to proceed with looking at the files
                // within this study
                long status = isDBKeyUniqueAndUpToDate(objType,
                                                studyHostFullpath, fullpath);
                if(status != 0)
                    return false;
            }

            // Check to see if this study is in an automation directory.
            // If so, then set the automation attr.  If we are in here because
            // the automation itself is being added, then this will be
            // duplicated.  That will be a waste of time, but will not
            // hurt anything.
            // Get the parent path for this study.
            // Strip off the filename and create a directory string.
            int index = fullpath.lastIndexOf(File.separator);
            if(index > 0)
                parent = fullpath.substring(0, index);

            // Is this parent an automation? Having to look to see if the parent
            // is an automation should only be necessary if this is a new dir
            // and we are actively putting studies into it.  Thus, only look
            // for the file autopar.
            String parPath = new String(parent + File.separator + "autopar");
            file = new UNFile(parPath);
            if(file.isFile()) {
                // The parent is an automation, set the automation and autodir
                // attributes for this study.
                Vector mp = MountPaths.getCanonicalPathAndHost(parent);
                dhost = (String) mp.get(Shuf.HOST);
                dpath = (String) mp.get(Shuf.PATH);

                String parentHostFullpath = dhost + ":" + dpath;
                setNonTagAttributeValue(objType, fullpath, localHost,
                                  Shuf.DB_AUTODIR, "text", parentHostFullpath);

                setNonTagAttributeValue(objType, fullpath, localHost,
                                  "autodir", "text", parent);


            }

            if(DebugOutput.isSetFor("addStudyToDB")) {
                if(success)
                    Messages.postDebug("Added Study " + fullpath);
                else
                    Messages.postDebug("Failed to Add Study " + fullpath);
            }

            // recursively add any vnmr_data or studies below this.
            // Add anything that is supposed to be in study_data here.
            // Set the 'study' attribute to studyHostFullpath even if the
            // files are already in the DB.

            fillATable(Shuf.DB_VNMR_DATA, fullpath, owner, true, info, sleepMs,
                       objType, studyHostFullpath);

            fillATable(Shuf.DB_STUDY, fullpath, owner, true, info, sleepMs,
                       objType, studyHostFullpath);

            fillATable(Shuf.DB_LCSTUDY, fullpath, owner, true, info, sleepMs,
                       objType, studyHostFullpath);
       }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem adding " + fullpath + " to DB");
                Messages.writeStackTrace(e);
            }
        }

        return success;

    }


    public boolean addImageDirToDB(String filename, String fullpath,
                                 String owner, String objType, long sleepMs) {
        boolean success=false;
        UNFile    file;
        UNFile    procparFile;
        String    name, path=null;
        String    imageHostFullpath;
        ArrayList fileList = new ArrayList();
        String dhost, dpath;

        // Add the image dir itself
        try {
            // Be sure fullpath is the Canonical path
            UNFile imageDir = new UNFile(fullpath);
            path = imageDir.getCanonicalPath();


            // Create the hostFullpath for this image dir to use as the
            // attribute value for image_dir for the items under this directory.
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost  = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
            imageHostFullpath = new String(dhost + ":" + dpath);

            success = addVnmrFileToDB(filename, path, owner,
                                      objType, sleepMs, null);
            if(!success) {
                // If failure if because the image_dir exists and is up to date
                // then we need to proceed with looking at the files
                // within this image_dir.  First see if there is an procpar.
                String procpar = fullpath + File.separator + "procpar";
                procparFile = new UNFile(procpar);
                if(!procparFile.exists()) {
                    // No procpar file, bail out
                    return false;
                }

                long status = isDBKeyUniqueAndUpToDate(objType,
                                                imageHostFullpath, fullpath);
                if(status != 0)
                    return false;
            }


            if(DebugOutput.isSetFor("addImagedirToDB")) {
                if(success)
                    Messages.postDebug("Added " + objType + " " + path);
                else
                    Messages.postDebug("Failed to Add "  + objType + " " + path);
            }
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem adding " + path + " to DB");
                Messages.writeStackTrace(e);
            }
        }

        return success;

    }



    public boolean addVnmrFileToDB(String filename, String fullpath,
                                   String owner, String objType, long sleepMs,
                                   DBCommunInfo dbInfo){
        return addVnmrFileToDB(filename, fullpath, owner, objType,
                               sleepMs, null,null,dbInfo);
    }

    public boolean addVnmrFileToDB(String filename, String fullpath,
                                   String owner, String objType, long sleepMs,
                                   String attrName, String attrVal, DBCommunInfo dbInfo){
        String          dhostFullpath;
        BufferedReader  in=null;
        String          rootFilename;
        String          parPath;
        ArrayList       valuesAsString=new ArrayList();
        String          dataType;
        String          param="";
        String          directory;
        int             numVals, index;
        String          basicType, subType;
        StreamTokenizerQuotedNewlines tok;
        boolean         investigatorFound=false;
        boolean         time_savedFound=false;
        int             typeMatch;
        char            ch;
        UNFile          file;
        UNFile          dir;
        long            timeSavedSec;
        java.util.Date  timeSavedDate;
        DBCommunInfo    info;
        String          expNum="";
        String          dhost, dpath;


        if(DebugOutput.isSetFor("addFileToDB")) {
            Messages.postDebug("addVnmrFileToDB(): filename = " + filename +
                               " fullpath = " + fullpath + "\nowner = " +
                               owner + " objType = " + objType +
                               " sleepMs = " + sleepMs);
        }

        /* if we have a workspace, then be sure the char following
           the prefix is numeric and the last char is numeric.
           Get the numeric part and keep it for later.
        */
        if(objType.equals(Shuf.DB_WORKSPACE)) {
            // Is the name at least long enough to contain a suffix numeral
            if(filename.length() < Shuf.DB_WKS_PREFIX.length() +1)
                return false;   /* Go on to next file */
            ch = filename.charAt(Shuf.DB_WKS_PREFIX.length());
            if(!Character.isDigit(ch))
                return false;   /* Go on to next file */
            /* Be sure the last char is a numeric */
            ch = filename.charAt(filename.length() -1);
            if(!Character.isDigit(ch))
                return false;   /* Go on to next file */

            // Get the numeric part following Shuf.DB_WKS_PREFIX
            expNum = filename.substring(Shuf.DB_WKS_PREFIX.length(),
                                               filename.length());

        }

        if(objType.equals(Shuf.DB_SHIMS)) {
            // If shims, be sure path is a shim directory
            if(fullpath.indexOf("shims/") == -1)
                return false;   /* Go on to next file */
        }

        try {
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost  = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch(Exception e) {
            Messages.postDebug("Problem getting cononical path for " +
                                fullpath);
            Messages.writeStackTrace(e);
            dhost = localHost;
            dpath = fullpath;
        }

        dhostFullpath = new String(dhost + ":" + dpath);

        /* AUTODIR can be detected by the presence of any of 3 files.
           Look for all three.  */
        if(objType.equals(Shuf.DB_AUTODIR)) {

            parPath = new String(fullpath + File.separator + "autopar");
            file = new UNFile(parPath);
            if(!file.isFile()) {
                // No autopar, try doneQ
                String qFile = new String(fullpath + File.separator + "doneQ");
                file = new UNFile(qFile);
                if(!file.isFile()) {
                    // No doneQ, try enterQ
                    qFile = new String(fullpath + File.separator + "enterQ");
                    file = new UNFile(qFile);
                    if(!file.isFile()) {
                        // No autopar, doneQ nor enterQ, thus, not an automation
                        return false;
                    }
                }
                // We must have found doneQ or enterQ, but not autopar
                // Create an empty BufferedReader
                try {
                    in = new BufferedReader(new StringReader(new String("")));
                } catch(Exception e) {
                    Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                         "Problem creating empty Reader");
                    Messages.writeStackTrace(e);
                    return false;
                }
            }
            else {
                // It is DB_AUTODIR and it has a autopar file.
                // Open the file.
                try {
                    file = new UNFile(parPath);
                    in = new BufferedReader(new FileReader(file));
                } catch(Exception e) {
                    Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                         "Problem opening " + parPath);
                    Messages.writeStackTrace(e, "Error caught in "
                                             + "addVnmrFileToDB");
                    return false;
                }
            }

        }
        else {
            /* For workspace, use curpar, for vnmr_data files use procpar,
               for shims, use fullpath, for records, use procpar
               for record_data use curpar. */
            if(objType.equals(Shuf.DB_WORKSPACE) ||
               objType.equals(Shuf.DB_VNMR_REC_DATA) )
                parPath = new String(fullpath + File.separator + "curpar");
            else if(objType.equals(Shuf.DB_SHIMS))
                parPath = fullpath;
            else if(objType.equals(Shuf.DB_STUDY)) {
                parPath = new String(fullpath + File.separator + "studypar");
            }
            else if(objType.equals(Shuf.DB_LCSTUDY)) {
                parPath = new String(fullpath + File.separator + "lcpar");
            }
            else if(objType.equals(Shuf.DB_VNMR_RECORD))
                parPath = new String(fullpath + File.separator + "acqfil" +
                                     File.separator + "curpar");
            // for DB_IMAGE_FILE, we need to use the procpar of the DB_IMAGE_DIR
            // or DB_COMPUTED_DIR above it.  Then after this, we will get 
            // params from the .fdf file header
            else if(objType.equals(Shuf.DB_IMAGE_FILE)) {
                // Does parent end in .img or .cmp?
                file = new UNFile(fullpath);
                String parent = file.getParent();
                if(parent.endsWith(Shuf.DB_IMG_DIR_SUFFIX)) {
                    // We have the proper type parent, add procpar to the path
                    parPath = new String(parent + File.separator + "procpar");
                }
                else {
                    // Oops, the .fdf is not in an .img nor .cmp directory.  
                    // Error out, until we know what to do about this.
                    Messages.postError("Failed while trying to add\n    "
                                       + fullpath + "\n    which is not in a "
                                       + Shuf.DB_IMG_DIR_SUFFIX
                                       + " directory.");
                    return false;
                }
            }
            else if(objType.equals(Shuf.DB_PROTOCOL)) {
                // If protocol, the filename and fullpath may still have ".xml" on it.          
                // Create the filename by stripping off the "xml" and adding "par"
                String parname;
                if(filename.endsWith(Shuf.DB_PROTOCOL_SUFFIX)) {
                    parname = filename.substring(0, filename.length() -3);
                    parname = parname + "par";
                }
                else
                    parname = filename;
                
                // Create the par full path by stripping off the 
                // "templates/vnmrj/protocols/myprotocol.xml" and 
                // adding "parlib/myprotocol.par"
                int ind = fullpath.indexOf("templates/vnmrj");
                if(ind > -1) {
                    parPath = fullpath.substring(0, ind);
                    parPath = parPath + "parlib" + File.separator 
                              + parname + File.separator + "procpar";
                }
                else
                    parPath = new String(fullpath + File.separator + "procpar");
            }
            else 
                parPath = new String(fullpath + File.separator + "procpar");
            

            // Test for the file first.
            file = new UNFile(parPath);
            if(!file.isFile()) {
                // If Protocol but the fullpath for the .par passed in does not exist,
                // then just fill the Db with what arrived in dbInfo from the .xml file
                // This will be the case for a composite protocol
                if(objType.equals(Shuf.DB_PROTOCOL)) {
                    // Send the cmd to the DB to add all the stuff in dbInfo
                    addRowToDBExecute(objType, dhostFullpath, dbInfo);
                    
                    // Now get out of here, there is no .par file to parse
                    return true;
             
                }
                return false;
            }

            // check to see if limits are in effect and if this file is too old
            if(objType.equals(Shuf.DB_VNMR_DATA) || 
                   objType.equals(Shuf.DB_IMAGE_FILE)) {
                long fileModified = file.lastModified();
                if(fileModified < dateLimitMS) {
                    // date is too old, but keep varian files anyway
                    String sysdir = System.getProperty("sysdir");
                    if(sysdir != null) {
                        UNFile sysfile = new UNFile(sysdir);
                        if(sysfile != null) {
                            // Need to compare the canonical path
                            String csysdir = sysfile.getCanonicalPath();
                            if(!fullpath.startsWith(csysdir + "/"))
                                // Not varian file, just exit
                                return false;
                        }
                    }
                }
            }

            // Open the file.
            try {
                in = new BufferedReader(new FileReader(file));
            } catch(Exception e) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                     "Problem opening " + parPath);
                Messages.writeStackTrace(e, "Error caught in addVnmrFileToDB");
                return false;
            }
        }
        /* The DB takes backslashes as excape characters.  The directory
         * paths on Windows use backslashes.  For these to work in the DB, we
         * need to use \\ so the backslashes are kept as part of the string.
         * Unix paths are uneffected because they will not have any backslashes.
         */
        dhostFullpath = UtilB.escapeBackSlashes(dhostFullpath);
        dpath = UtilB.escapeBackSlashes(dpath);

        // Start the process of adding a row to the DB
        if(dbInfo != null) {
            // Protocols need to add xml and procpar info, so we pass in info.
            info = dbInfo;
        }
        else
            info = addRowToDBInit(objType, dhostFullpath, fullpath);
        if(info == null) {
            // File may already be in DB and be up to date is most common
            // reason to bail out here.
            try {
                in.close();
            } catch(Exception eio) {}
            return false;
        }

        dir = new UNFile(fullpath);

        timeSavedSec = dir.lastModified();
        timeSavedDate = new java.util.Date(timeSavedSec);

        // strip suffix off of the filename before adding to DB.
        // Not for automation nor study
        if(objType.equals(Shuf.DB_AUTODIR) || 
                      objType.equals(Shuf.DB_STUDY) ||
                      objType.equals(Shuf.DB_LCSTUDY)) {
            rootFilename = filename;
        }
        else {
            index = filename.lastIndexOf('.');
            if(index > 0)
                rootFilename = filename.substring(0, index);
            else
                rootFilename = filename;
        }


        // Strip off the filename and create a directory string.
        // If ends with File.separator, just strip it off.
        index = fullpath.lastIndexOf(File.separator);
        if(index == fullpath.length() -1)
            fullpath = fullpath.substring(0,fullpath.length() -1);

        // In case we stripped off a slash, get the index again
        index = fullpath.lastIndexOf(File.separator);
        if(index > 0)
            directory = fullpath.substring(0, index);
        else
            directory = fullpath;

        // Now get just the last directory level name
        index = directory.lastIndexOf(File.separator);
        if(index > 0)
            directory = directory.substring(index+1);

        // Add some standard attributes that will not be in the file.
        ArrayList List = new ArrayList();
        List.add(dhostFullpath);
        addRowToDBSetAttr(objType,  "host_fullpath",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(rootFilename);
        addRowToDBSetAttr(objType,  "filename",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(dpath);
        addRowToDBSetAttr(objType,  "fullpath",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(owner);
        addRowToDBSetAttr(objType,  "owner",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(localHost);
        addRowToDBSetAttr(objType,  "hostname",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(dhost);
        addRowToDBSetAttr(objType,  "hostdest",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        List.clear();
        List.add(directory);
        addRowToDBSetAttr(objType,  "directory",
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        // The division by 1000 is to get seconds instead of milliseconds.
        // milliseconds over flows the int.
        List.clear();
        List.add(String.valueOf(timeSavedSec/1000));
        addRowToDBSetAttr(objType,  "long_time_saved", DB_ATTR_INTEGER,
                              List, dhostFullpath, info);
        if(objType.equals(Shuf.DB_WORKSPACE)) {
            List.clear();
            List.add(expNum);
            addRowToDBSetAttr(objType,  "exp",
                              DB_ATTR_INTEGER, List, dhostFullpath, info);

        }
        if(objType.equals(Shuf.DB_VNMR_RECORD) ||
           objType.equals(Shuf.DB_VNMR_REC_DATA) ||
           objType.equals(Shuf.DB_AUTODIR) ||
           objType.equals(Shuf.DB_LCSTUDY) ||
           objType.equals(Shuf.DB_STUDY)) {

            List.clear();

            // Is this data corrupt?  If so, there should be a file
            // named 'checksum.flag'
            file = new UNFile(fullpath + File.separator + "checksum.flag");
            String corrupt;
            if(file.exists())
                corrupt = new String("corrupt");
            else
                corrupt =  new String("valid");
            List.add(corrupt);
            addRowToDBSetAttr(objType,  "corrupt",
                              DB_ATTR_TEXT, List, dhostFullpath, info);

            if(objType.equals(Shuf.DB_VNMR_RECORD) ||
               objType.equals(Shuf.DB_VNMR_REC_DATA)) {
                List.clear();
                // Is this data FDA (.REC) or Non-FDA (.rec)?
                if(fullpath.indexOf(Shuf.DB_REC_SUFFIX) > -1) {
                    List.add("fda");
                }
                else
                    List.add("nonfda");
                addRowToDBSetAttr(objType,  "fda",
                                  DB_ATTR_TEXT, List, dhostFullpath, info);
            }
        }
        if(objType.equals(Shuf.DB_VNMR_REC_DATA)) {
            // We need the parent record name and path that this data belongs
            // to.  That should be the name which ends with .rec or .REC in
            // the fullpath of this rec_data file.  Thus, we want the string
            // before .rec or .REC.
            index = dhostFullpath.indexOf(Shuf.DB_REC_SUFFIX);
            if(index == -1)
                index = dhostFullpath.indexOf(Shuf.DB_REC_SUFFIX.toLowerCase());
            if(index == -1) {
                Messages.postError("Cannot find record name for " +
                                   dhostFullpath);
            }
            else {
                String record = dhostFullpath.substring(0, index);

                List.clear();
                List.add(record);
                addRowToDBSetAttr(objType,  "record",
                                  DB_ATTR_TEXT, List, dhostFullpath, info);
            }
        }
        // If attrName is set, then add that to the list
        if(attrName != null) {
            List.clear();
            List.add(attrVal);
            addRowToDBSetAttr(objType,  attrName,
                              DB_ATTR_TEXT, List, dhostFullpath, info);
        }

        // Use StreamTokenizer because we can keep the quoted string
        // as a single token.  StringTokenizer does not allow that.
        // I had to create StreamTokenizerQuotedNewlines to allow quoted
        // string to contain newLines.  The standard one ends the quote
        // at the end of a line.  Sun had a bug reported about this
        // (4239144) and refused to add an option to allow multiple lines
        // in a quoted string.  GRS
        tok = new StreamTokenizerQuotedNewlines(in);
        // Define the quote character
        tok.quoteChar('\"');
        // Define additional chars to be treated as normal letters.
        // Most things within a comment, sample name etc will be within
        // double quotes and do not need to be listed here.  We need things
        // here that can be in parameter names.
        tok.wordChars('_', '_');

        // I incountered 2 Java bugs in using StreamTokenizer.
        // One is that is does not handle 1e18 format of numbers.
        // I am not worried about it not turning 1e18 format into numbers
        // because I don't need the numeric value anyway.  I would just
        // turn it into a string and send it to the DB.
        // Two is that tok.parseNumbers() is called by its constructor
        // and thus is always on, and tok.parseNumbers() does not allow
        // you to turn it off.
        // To turn off the numeric thing by hand, first call ordinaryChars,
        // then specify numbers to be normal word characters.
        // Then when we encounter 1.3e-13 we get it as one string token.
        // tok.nval will never contain anything, because ALL letters
        // and numbers will be turned into 'String' tokens.
        tok.ordinaryChars('+', '.'); // + comma - and period  (Turn off numeric)
        tok.wordChars('+', '.'); // + comma - and period (Set as normal letters)
        tok.ordinaryChars('0', '9'); // all numbers   (Turn off numeric)
        tok.wordChars('0', '9');     // all numbers   (Set as normal letters)
        tok.wordChars('$', '$');     // all numbers   (Set as normal letters)
        try {
            // Parse the vnmr file.
            while(tok.nextToken() != StreamTokenizerQuotedNewlines.TT_EOF) {
                // There should be 11 tokens on the first line and we
                // only want the first 3.  The first is a string and
                // the 2nd and 3rd are integers
                param = tok.sval;
                tok.nextToken();
                subType = tok.sval;
                // If null or negative, this is a bad format file
                // If not an integer, an exception will be thrown.
                try {
                    if(subType == null || Integer.parseInt(subType) < 0) {
                        if(in != null)
                            in.close();
                        return false;
                    }
                }
                catch (Exception e) {
                    try {
                        in.close();
                    } catch(Exception eio) {}
                    return false;
                }

                tok.nextToken();
                basicType = tok.sval;

                // This would mean a bad format, so skip this file
                if(basicType == null ||
                   (!basicType.equals("1") && !basicType.equals("2"))) {
                    try {
                        in.close();
                    } catch(Exception eio) {}
                    return false;
                }

                // Dump the next 8
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();

                tok.nextToken();
                numVals = Integer.parseInt(tok.sval);
                if(basicType.equals(T_REAL)) {
// Try forcing all reals to float8
//                    if(subType.equals(ST_INTEGER)) {
//                        dataType = DB_ATTR_INTEGER;
//                    }
//                    else {
                        dataType = DB_ATTR_FLOAT8;
//                    }

                    /* See if we want to put this param in the db or
                       not. We wait to check until now, because we
                       have to read everything out of the procpar
                       file anyway.
                       Returns 0 for false, 1 for true and
                       2 for true except item in DB is array type.
                    */
                    typeMatch = isParamInList(objType, param, dataType);

                    // The param temp is not usable in postgres, so change the
                    // string to "temperature".
                    if(param.equals("temp"))
                        param = "temperature";

                    /* If == 1, the type matches and is not an array.
                       Loop thru in case the procpar had an array.
                       If == 0, no match, so just loop thru to dump them. */
                    if(typeMatch < 2) {
                        for (int i=0; i < numVals; i++) {
                            tok.nextToken();
                            // Only take first value and
                            // only if param is wanted.
                            if(i == 0 && typeMatch == 1) {
                                // We only want the first value.
                                valuesAsString.add(tok.sval);

                                /* Just put the first value into the db. */
                                addRowToDBSetAttr(objType, param, dataType,
                                                  valuesAsString, dhostFullpath,
                                                  info);
                                valuesAsString.clear();
                                // If protocol AND param is sfrq, then we
                                // want to calc a field in Tesla and add it
                                if(objType.equals(Shuf.DB_PROTOCOL) && param.equals("sfrq")) {
                                    double field = convertFreqToTesla(tok.sval);
                                    valuesAsString.add(Double.toString(field));
                                    addRowToDBSetAttr(objType, "field", dataType,
                                            valuesAsString, dhostFullpath,
                                            info);
                                    valuesAsString.clear();

                                }
                            }
                            // else, not the first, just let them be dumped.
                        }
                    }
                    /* We have a param match and it is an array type.
                       Even if procpar only has one value, we still have
                       to send sql command to set an array.
                    */
                    if(typeMatch == 2) {
                        for (int i=0; i < numVals; i++) {
                            tok.nextToken();
                            valuesAsString.add(tok.sval);
                        }
                        // attr Type sent as arg must be type including
                        // the [].  Since we know this is an array type,
                        // add the [] to dataType.
                        addRowToDBSetAttr(objType, param, dataType + "[]",
                                          valuesAsString, dhostFullpath, info);
                        valuesAsString.clear();
                    }

                    /*  Read the enumeration values and ignore */
                    tok.nextToken();
                    numVals = Integer.parseInt(tok.sval);
                    for (int i=0; i < numVals; i++)
                        tok.nextToken();
                }
                else if(basicType.equals(T_STRING)) {
                    if(param.startsWith("time"))
                        dataType = DB_ATTR_DATE;
                    else
                        dataType = DB_ATTR_TEXT;

                    /* We need to know if investigator is set, so
                       check and if so, set the flag. */
                    if(param.equals("investigator"))
                        investigatorFound = true;

                    /*
                      Do we want this parameter?
                      Returns 0 for false, 1 for true and
                      2 for true except item in DB is array type.
                    */
                    typeMatch = isParamInList(objType, param, dataType);
                    /* If == 1, the type matches and is not an array.
                       Loop thru in case the procpar had an array.
                       If == 0, no match, so just loop thru to dump them. */
                    if(typeMatch < 2) {
                        for (int i=0; i < numVals; i++) {
                            tok.nextToken();
                            // We only want the first value and
                            // only if param is wanted
                            if(i == 0 && typeMatch == 1) {
                                String value = tok.sval;

                                /* If the name starts with 'time', we may
                                   need to convert a trailing 'Z' to 'UTC'.
                                   ISO 8601 specifies 'Z' but Postgres
                                   accepts 'UTC'. Actually it accepts 'UT'
                                   also.
                                */
                                if(param.startsWith("time")) {
                                    if(value.endsWith("Z")) {
                                        value = value.substring(
                                            0,value.length()-1);
                                        value = value.concat("UT");
                                    }
                                    // If we found TIME_SAVED and it has a
                                    // value then set the flag.
                                    if(param.equals(Shuf.DB_TIME_SAVED) &&
                                       value.trim().length() != 0)
                                        time_savedFound = true;
                                }
                                // Limit string value length. Too many causes
                                // an sql error
                                if(value.length() > MAX_ARRAY_STR_LEN) {
                                    value = value.substring(0,
                                                            MAX_ARRAY_STR_LEN);
                                }
                                
                                // If empty string, don't use it
//                                if(!value.equals(""))
                                    valuesAsString.add(value);

                                /*
                                  Do we want this parameter?
                                  Returns 0 for false, 1 for true and 2
                                  for true except item in DB is array type.
                                */
                                /* Just put the first value into the db. */
                                addRowToDBSetAttr(objType, param, dataType,
                                                  valuesAsString, dhostFullpath,
                                                  info);

                                valuesAsString.clear();
                            }
                        }
                        // else, not the first, just let them be dumped.
                    }
                    /* We have a param match and it is an array type.
                       Even if procpar only has one value, we still have
                       to send sql command to set an array.
                    */
                    if(typeMatch == 2) {
                        for (int i=0; i < numVals; i++) {
                            tok.nextToken();
                            String value = tok.sval;
                            // Limit string value length.  Too many causes
                            // an sql error
                            if(value.length() > MAX_ARRAY_STR_LEN) {
                                value = value.substring(0, MAX_ARRAY_STR_LEN);
                            }
                            valuesAsString.add(value);
                        }
                        // attr Type sent as arg must be type including
                        // the [].  Since we know this is an array type,
                        // add the [] to dataType.
                        addRowToDBSetAttr(objType, param, dataType + "[]",
                                          valuesAsString, dhostFullpath, info);
                        valuesAsString.clear();
                    }

                    /*  Read the enumeration values and ignore */
                    tok.nextToken();
                    numVals = Integer.parseInt(tok.sval);
                    for (int i=0; i < numVals; i++)
                        tok.nextToken();
                }
            }
            in.close();
        }
        catch (Exception e) {
            // Only output error if not running update in background.
            // This will be given as any time we are called with  sleepMs == 0
            if(sleepMs == 0) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                     "Problem adding " + filename +
                                     " to database near param = " + param);
                Messages.writeStackTrace(e, "Error caught in addVnmrFileToDB");
            }

            try {
                in.close();
            } catch(Exception eio) {}
            return false;
        }

        /* If investigator is not set for this entry, then set it to
           the owner. */
        if(!investigatorFound) {
            List = new ArrayList();
            List.add(owner);
            addRowToDBSetAttr(objType, "investigator", DB_ATTR_TEXT,
                              List, dhostFullpath, info);
        }

        if(!time_savedFound) {
            // If the parameter time_saved is not in procpar, then
            // just use the unix date stamp and set time_saved.

            // Convert timeSavedDate to date string
            SimpleDateFormat formatter =
                new SimpleDateFormat ("MMM d, yyyy HH:mm:ss");
            String str = formatter.format(timeSavedDate);
            List = new ArrayList();
            List.add(str);
            addRowToDBSetAttr(objType, Shuf.DB_TIME_SAVED, DB_ATTR_DATE,
                              List, dhostFullpath, info);
        }

        // Send the cmd to the DB to add all the stuff from above.
        addRowToDBExecute(objType, dhostFullpath, info);


        try {
            in.close();
        } catch(Exception e) {  }

        return true;
    }


    /************************************************** <pre>
     * Summary:  Remove all rows of the current user from the given DB table.
     *
     </pre> **************************************************/

    public boolean clearOutTable(String objType, String user) {

        try {
            if(user.equals("all") )
                executeUpdate("DELETE FROM " + objType);
            else
                executeUpdate("DELETE FROM " + objType +
                                  " WHERE owner = \'" + user + "\'");
            return true;
        }
        catch (Exception e) {
            haveConnection = false;
            if(!ExitStatus.exiting()) {
                 Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem removing all items from" +
                                 objType + " DB table for owner = " + user);
                 Messages.writeStackTrace(e, "Error caught in clearOutTable");
            }
            return false;
        }
    }

    public void getFileListing(ArrayList fileList, String prefix,
                                    String suffix, String directory,
                                    boolean recursive, String objType) {
        int entryCounter = 0;
        java.util.Date starttime = new java.util.Date();
        java.util.Date endtime;
        long timems;

        // if (locatorOff() && !managedb)
        if (locatorOff())
            return;
        // Skip the updateSymLinks if not recursive and if Interix
        if(recursive) {
            // Start a thread to check for symbolic links.
            UpdateSymLinks updateSymLinks;
            updateSymLinks = new UpdateSymLinks(directory, symLinkMap);
            updateSymLinks.start();
        }

        getFileListingRecursive(fileList, prefix, suffix, directory, recursive,
                                objType, entryCounter);

        if(DebugOutput.isSetFor("getFileListing")) {
            endtime = new java.util.Date();
            timems = endtime.getTime() - starttime.getTime();
            Messages.postDebug("**Time for getFileListing " + objType + ": "
                            + timems + "\n    Dir: " + directory + "\n    "
                            + "entryCount: " + entryCounter + "  fileCount: "
                            + fileList.size());
        }

    }

    /************************************************** <pre>
     * Summary: Get the file with this prefix and suffix in the
     *    given directory or recursively if requested.
     *
     *    The arg fileList an empty ArrayList to be filled with 'File' objects.
     *    From the File objects, the caller can get the path to each file
     *    found.
     *
     </pre> **************************************************/
    private void getFileListingRecursive(ArrayList fileList, String prefix,
                                         String suffix, String directory,
                                         boolean recursive, String objType,
                                         int entryCounter) {
        DirFilter filterMatch, filterNonMatch;
        UNFile    dir;
        UNFile    file;
        File[]    files;
        String[]  MatchSuffixList;
        String[]  NonMatchSuffixList;

        if(entryCounter > 200) {
            Messages.postError("Problem getting file listing of " +
                           directory + "\n    There may be a cyclic reference"
                           + " someplace in this persons directory.");
            return;
        }

        // accept() is set up to only work with one matching suffix,
        MatchSuffixList = new String[1];
        NonMatchSuffixList = new String[6];
        MatchSuffixList[0] = suffix;
        NonMatchSuffixList[0] = Shuf.DB_FID_SUFFIX;
        NonMatchSuffixList[1] = Shuf.DB_PAR_SUFFIX;
        NonMatchSuffixList[2] = Shuf.DB_SPC_SUFFIX;
        NonMatchSuffixList[3] = Shuf.DB_PANELSNCOMPONENTS_SUFFIX;
        NonMatchSuffixList[4] = Shuf.DB_REC_SUFFIX;
        NonMatchSuffixList[5] = new String(Shuf.DB_REC_SUFFIX.toLowerCase());

        filterMatch = new DirFilter(prefix, MatchSuffixList, true, objType);
        filterNonMatch = new DirFilter("", NonMatchSuffixList, false);
        dir = new UNFile(directory);

        if(dir == null)
            return;

        // Did we receive a single file or single data directory?
        // See if it is a file, or if 'directory' has prefix or suffix.
        // If so, just return a list of this one item.
        if(dir.isFile() ||
                (suffix.length() > 0 && directory.endsWith(suffix)) ||
                (suffix.length() > 0 && directory.endsWith(suffix + "/"))) {
            fileList.add(dir);
            return;
        }

        // Traverse down the directory tree.
        try {
            if(recursive) {
                // Get the desired files from this directory
                files = dir.listFiles(filterMatch);

                // Transfser the files found to the output list
                if(files != null) {
                    for(int i=0; i < files.length; i++) {
                        fileList.add(files[i]);
                    }
                }

                // Now Get any sub directories within this directory, but
                // do not include .par nor .fid entries.

                // Get the directory listing excluding files/dir with suffix
                files = dir.listFiles(filterNonMatch);
                if(files != null) {
                    for(int i=0; i < files.length; i++) {
                        file = (UNFile)files[i];

                        // Try to catch cyclic links and tell the user
                        // what and where the problem is, then skip the dir
                        if(file.getParent().startsWith(
                                                   file.getCanonicalPath())) {
                            Messages.postError("Problem getting directory list."
                                            + " The directory\n    "
                                            + file.getAbsolutePath()
                                            + "\n    Appears to link back to "
                                            + file.getCanonicalPath()
                                            + "\n    Skipping this directory.");
                        }
                        // Skip the directory if it appears to be cyclic.
                        else {
                            if(file.isDirectory()) {
                                getFileListingRecursive(fileList, prefix,
                                           suffix,
                                           file.getCanonicalPath(), recursive,
                                           objType, entryCounter+1);
                            }
                        }
                    }
                }
            }
            // Only this directory
            else {
                filterNonMatch = new DirFilter("", NonMatchSuffixList, false);
                files = dir.listFiles(filterMatch);
                if(files != null) {
                    // Transfser the files found to the output list
                    for(int i=0; i < files.length; i++)
                        fileList.add(files[i]);
                }
            }
        }
        catch (Exception e) {
            if(!ExitStatus.exiting())
                Messages.writeStackTrace(e,
                                    "Error caught in getFileListingRecursive");
        }
        if(DebugOutput.isSetFor("filelist")) {
            // The size given in the output from this will grow as it recursively
            // goes down the tree.  
            Messages.postDebug("Size of filelist being accumulated " + directory 
                             + " is " + fileList.size());
        }


        return;

    }

    /************************************************** <pre>
     * Summary: Remove DB entries which no longer exist.
     *
    </pre> **************************************************/
    public void cleanupDBList(DBCommunInfo info) {
        // If no arg, pass 0 for sleepMs
        cleanupDBList(0, info);
    }

    /************************************************** <pre>
     * Summary: Remove DB entries which no longer exist.
     *
    </pre> **************************************************/
    public void cleanupDBList(long sleepMs, DBCommunInfo info) {

        info.numFilesRemoved=0;
        cleanupDBListThisType(Shuf.DB_VNMR_DATA, sleepMs, info);
        cleanupDBListThisType(Shuf.DB_VNMR_PAR,  sleepMs, info);
        cleanupDBListThisType(Shuf.DB_VNMR_RECORD,  sleepMs, info);
        cleanupDBListThisType(Shuf.DB_VNMR_REC_DATA,  sleepMs, info);
        cleanupDBListThisType(Shuf.DB_PROTOCOL,  sleepMs, info);
        cleanupDBListThisType(Shuf.DB_STUDY,  sleepMs, info);
        cleanupDBListThisType(Shuf.DB_LCSTUDY,  sleepMs, info);
        cleanupDBListThisType(Shuf.DB_AUTODIR,  sleepMs, info);
        cleanupDBListThisType(Shuf.DB_IMAGE_DIR,  sleepMs, info);
        cleanupDBListThisType(Shuf.DB_WORKSPACE, info);
        cleanupDBListThisType(Shuf.DB_TRASH, info);
        cleanupDBListThisType(Shuf.DB_PANELSNCOMPONENTS, info);
        cleanupDBListThisType(Shuf.DB_SHIMS, info);

    }




    /************************************************** <pre>
     * Summary: Remove DB entries of this type which no longer exist.
     *
     *   If the entry is from a different host, skip it.
     *   Each host should take care of its own entries.
     *
     *   Check to see if each entry of this type exist, if it no longer
     *   exists then remove it from the DB.
     *
     </pre> **************************************************/
    public boolean cleanupDBListThisType(String objType, DBCommunInfo info) {
        // If no second arg, pass 0 for sleepMs
        return cleanupDBListThisType(objType, 0, info);

    }


    /************************************************** <pre>
     * Summary: Remove DB entries of this type which no longer exist.
     *
     *   If the entry is from a different host, skip it.
     *   Each host should take care of its own entries.
     *
     *   Check to see if each entry of this type exist, if it no longer
     *   exists then remove it from the DB.
     *
     </pre> **************************************************/
    public boolean cleanupDBListThisType(String objType, long sleepMs,
                                         DBCommunInfo info) {
        java.sql.ResultSet rs;
        String host, dhost;
        String fullpath;
        String mpath;
        UNFile file;

        if(DebugOutput.isSetFor("cleanupDBListThisType"))
            Messages.postDebug("cleanupDBListThisType on " + objType);

        // Get the dateLimit for keeping vnmr_data in the locator
        // dataLimitDays = 0 means nothing is kept except varian data
        // dataLimitDays = -1 means everything is kept
        // dataLimitDays greater than 0 means keep this many weeks of data
        // dataLimitDays is set with -Ddatelimit=# in the java startup cmd line
        Calendar dateLimit = Calendar.getInstance();
        java.util.Date dateToday = new java.util.Date();
        Calendar calToday = Calendar.getInstance();
        calToday.setTime(dateToday);

        // Get ALL entries for this table.
        try {
            rs = executeQuery("SELECT hostname, fullpath, hostdest FROM "
                              + objType);
            if(rs == null) {
                return false;
            }
            while(rs.next()) {
                host = rs.getString(1);
                // If there is no host, something is wrong with the
                // entry.  We do not even have the information to get
                // rid of the entry so just skip it.
                if(host == null)
                    continue;
                // If not put into the DB from this host, go on to the next one.
                if(!host.equals(localHost))
                    continue;

                fullpath = rs.getString(2);
                dhost = rs.getString(3);

                // fullpath here will be the direct path on the machine
                // where it actually exists.  We need to get the mounted
                // path so that we can access the actual file.
                mpath = MountPaths.getMountPath(dhost, fullpath);

                // If Windows, we need a windows path for File
                if(UtilB.OSNAME.startsWith("Windows"))
                    mpath = UtilB.unixPathToWindows(mpath);

                file = new UNFile(mpath);
                // Does this fullpath still exist?
                if(!file.exists()) {
                    // No, then remove it from the DB.
                    removeEntryFromDB (objType, fullpath, dhost, info);
                }
                // If the file is older than the date specified as the
                // limit, then remove it.
                long fileModified = file.lastModified();
                if((objType.equals(Shuf.DB_VNMR_DATA) || 
                    objType.equals(Shuf.DB_IMAGE_FILE)) && 
                        fileModified < dateLimitMS) {
                    // date is too old, but keep varian files anyway
                    String sysdir = System.getProperty("sysdir");
                    if(sysdir != null) {
                        UNFile sysfile = new UNFile(sysdir);
                        if(sysfile != null) {
                            // Need to compare the canonical path
                            String csysdir = sysfile.getCanonicalPath();
                            if(!fullpath.startsWith(csysdir + "/"))
                                // Not varian file, remove it
                                removeEntryFromDB (objType, fullpath, dhost, info);
                        }
                    }
                }
                try {
                    Thread.sleep(sleepMs);
                } catch (Exception e) {}
            }
        }
        catch (Exception e) {
            // If sleepMs > 0, we must be in a bkg thread, no error output
            if(sleepMs == 0) {
                Messages.postDebug("Problem clearing out " + objType +
                                 " table in DB");
                Messages.writeStackTrace(e,
                                      "Error caught in cleanupDBListThisType");
            }

        }
        return true;
    }

    public void updateDB(Hashtable users, long sleepMs)
                                          throws InterruptedException {
        // Default to filling of workspace as true.
        boolean workspace = true;
        updateDB(users, sleepMs, workspace);
    }

    public void updateDB(Hashtable users, long sleepMs, boolean workspace) 
                                          throws InterruptedException {
        // Default to filling of appdirOnly as false.
        boolean appdirOnly = false;
        updateDB(users, sleepMs, workspace, appdirOnly);
    
    }
    

    public void updateDB(Hashtable users, long sleepMs, boolean workspace, 
                         boolean appdirOnly) throws InterruptedException {
        User      user;
        String    dir;
        String    userName;
        ArrayList dataDirs;
        java.util.Date endtime;
        long timems;
        java.util.Date starttime = new java.util.Date();
        DBCommunInfo info = new DBCommunInfo();
        ArrayList dirChecked = new ArrayList();

        
        if(attrList == null)
            return;
        
        
        // workspace = false, means don't update workspaces from the disk
        // files.  The DB may have been updated on the fly as params change.
        // The thread crawler needs to skip the workspace update.

        try {

            // If no connection, this will output one error, then we can quit.
            if(checkDBConnection() == false)
               return;

            // Vacuum the DB before updating for better access during update.
            vacuumDB();

            // Update the macros and commands if necessary.
            updateMacro(Shuf.DB_COMMAND_MACRO);
            if(DebugOutput.isSetFor("updatedb"))
                Messages.postDebug("Updating Command Macros");

            updateMacro(Shuf.DB_PPGM_MACRO);
            if(DebugOutput.isSetFor("updatedb"))
                Messages.postDebug("Updating PPGM Macros");

            // appdirOnly means we are only updating the apdir directories
            // and are probably trying to use a previous DB and and wanting
            // to go as quickly as possible.  Cleanup can take a long time
            // when there are a lot of files in the DB
            if(!appdirOnly) {
                // Remove any entries which are no longer on the disk
                if(DebugOutput.isSetFor("updatedb"))
                    Messages.postDebug("updateDB calling cleanupDBList");
                cleanupDBList(sleepMs, info);

                // Clean up the SymLinkList.
                symLinkMap.cleanDirChecked();
            }
            // Get the system  DB_PANELSNCOMPONENTS files.

            //dir = FileUtil.vnmrDir("SYSTEM/PANELITEMS");
            // Force user for these to vnmrj.
            //fillATable(Shuf.DB_PANELSNCOMPONENTS, null, dir,
            //                       "vnmrj", recursive);

            // Get the system shim files
            // Force Owner to vnmrj

            //dir = FileUtil.vnmrDir("SYSTEM/SHIMS");

            //fillATable(Shuf.DB_SHIMS, null, dir, "vnmrj", recursive);

            // Go thru the users
            if(DebugOutput.isSetFor("updatedb"))
                Messages.postDebug("Going through the users to be updated");

            for(Enumeration en = users.elements(); en.hasMoreElements(); ) {
                user = (User) en.nextElement();
                userName = user.getAccountName();

                // Skip the datadirs if appdir Only
                if(!appdirOnly) {
                    dataDirs = user.getDataDirectories();
                    // Do not search in directories or children of directories
                    // already searched.  Get a clone so we can change the list.
                    ArrayList cDataDirs;
                    String dirCked;
                    int index;
                    cDataDirs = (ArrayList)dataDirs.clone();
                    for(int k=0; k < dirChecked.size(); k++) {
                        dirCked = (String) dirChecked.get(k);
                        // First Dups
                        if((index=cDataDirs.indexOf(dirCked)) > -1) {
                            // Found dup, remove it from list to check
                            cDataDirs.remove(index);
                            continue;
                        }
                        // Look for children in cDataDirs of parents in dirChecked
                        for(int j=0; j < cDataDirs.size(); j++) {
                            String dir1 = (String) cDataDirs.get(j);
                            String dir2 = (String) dirChecked.get(k);
                            if(dir1.startsWith(dir2)) {
                                // We found that cDataDirs(j) is a child of
                                // dirChecked(k).  Remove the child.
                                cDataDirs.remove(j);
                                j--;
                            }
                        }
                    }

                    // Add cDataDirs to dirChecked for future checks
                    for(int j=0; j < cDataDirs.size(); j++) {
                        dirChecked.add(cDataDirs.get(j));
                    }

                    // Check this users list of directories in SavedDirList
                    // These are directories where the user has saved files
                    // to.  It actually comes from addFileToDB().  Add the
                    // unique ones to cDataDirs for use below.
                    // Get a clone of the list.
                    ArrayList cSavedDirList;
                    cSavedDirList = savedDirList.getSavedDirList();

                    for(int k=0; k < dirChecked.size(); k++) {
                        dirCked = (String) dirChecked.get(k);
                        // First Dups
                        if((index=cSavedDirList.indexOf(dirCked)) > -1) {
                            // Found dup, remove it from list to check
                            cSavedDirList.remove(index);
                            continue;
                        }
                        // Look for children in cSavedDirList of parents
                        // in dirChecked
                        for(int j=0; j < cSavedDirList.size(); j++) {
                            String dir1 = (String) cSavedDirList.get(j);
                            String dir2 = (String) dirChecked.get(k);
                            if(dir1.startsWith(dir2)) {
                                // We found that cSavedDirList(j) is a child of
                                // dirChecked(k).  Remove the child.
                                cSavedDirList.remove(j);
                                j--;
                            }
                        }
                    }
                    // Now add any directories found in cSavedDirList to
                    // cDataDirs and dirChecked
                    for(int j=0; j < cSavedDirList.size(); j++) {
                        cDataDirs.add(cSavedDirList.get(j));
                        dirChecked.add(cSavedDirList.get(j));
                    }




                    if(DebugOutput.isSetFor("updatedb"))
                        Messages.postDebug("Updating for user, " + userName
                                    + "\n        Data Dir List: " + cDataDirs);

                    // Now loop thru all of this users data directories as
                    // specified in userList.

                    for(int i=0; i < cDataDirs.size(); i++) {
                        dir = (String)cDataDirs.get(i);

                        if(DebugOutput.isSetFor("updatedb"))
                            Messages.postDebug("    Updating vnmr_data in " +dir);
                        // Fill DB with files in this directory and below.
                        fillATable(Shuf.DB_VNMR_DATA, dir, userName,
                                   true, info, sleepMs);
                        if(DebugOutput.isSetFor("updatedb"))
                            Messages.postDebug("    Updating vnmr_par in " +dir);
                        fillATable(Shuf.DB_VNMR_PAR, dir, userName, true,
                                   info, sleepMs);
                        if(DebugOutput.isSetFor("updatedb"))
                            Messages.postDebug("    Updating study in " +dir);
                        fillATable(Shuf.DB_STUDY, dir, userName, true,
                                   info, sleepMs);
                        if(DebugOutput.isSetFor("updatedb"))
                            Messages.postDebug("    Updating lcstudy in " +dir);
                        fillATable(Shuf.DB_LCSTUDY, dir, userName, true,
                                   info, sleepMs);
                        if(DebugOutput.isSetFor("updatedb"))
                            Messages.postDebug("    Updating " + Shuf.DB_AUTODIR
                                               + " in " +dir);
                        fillATable(Shuf.DB_AUTODIR, dir, userName, true,
                                   info, sleepMs);
                        if(DebugOutput.isSetFor("updatedb"))
                            Messages.postDebug("    Updating " + Shuf.DB_IMAGE_DIR
                                               + " in " +dir);
                        fillATable(Shuf.DB_IMAGE_DIR, dir, userName, true,
                                   info, sleepMs);

                        // Get Protocol Files
                        // If dir ends with "data", remove it and use the
                        // base path ahead of "data"
                        String baseDir=dir;
                        if(dir.endsWith(File.separator + "data")) {
                            baseDir = dir.substring(0, dir.length() -5);
                        }
                        String pdir=FileUtil.vnmrDir(baseDir,"PROTOCOLS");
                        if(pdir !=null) {
                            if(DebugOutput.isSetFor("updatedb"))
                                Messages.postDebug("    Updating protocol in " +pdir);
                            fillATable(Shuf.DB_PROTOCOL, pdir, userName,
                                       true, info, sleepMs);
                        }

                        // Non-FDA records should be in the cDataDirs list of
                        // directories.
                        if(DebugOutput.isSetFor("updatedb"))
                            Messages.postDebug("    Updating record in " +dir);
                        fillATable(Shuf.DB_VNMR_RECORD, dir, userName, true,
                                   info, sleepMs);
                    }

                    // Get directories to use for FDA records (.REC)
                    // Fill records and the record data within the record
                    dataDirs = user.getP11Directories();
                    for(int i=0; i < dataDirs.size(); i++) {
                        dir = (String)dataDirs.get(i);
                        if(DebugOutput.isSetFor("updatedb"))
                            Messages.postDebug("    Updating P11 dir, " + dir);
                        // Fill DB with files in this directory and below.
                        fillATable(Shuf.DB_VNMR_RECORD, dir, userName, true,
                                   info, sleepMs);
                    }

                }
                // Get this users shim directory and fill from it.
                ArrayList appDirs=user.getAppDirectories();

                for(int i=0; i < appDirs.size(); i++) {
                    String base=(String)appDirs.get(i);
                    if(DebugOutput.isSetFor("updatedb"))
                        Messages.postDebug("    Updating appDir base dir, "
                                           + base);
                    dir=FileUtil.vnmrDir(base,"SHIMS");

                    if(dir !=null)
                        fillATable(Shuf.DB_SHIMS, dir, userName,
                                   true, info);


                    // Get this users xml directory and fill from it.

                    dir=FileUtil.vnmrDir(base,"LAYOUT");
                    if(dir !=null){
                        fillATable(Shuf.DB_PANELSNCOMPONENTS, dir, userName,
                                   true, info);
                    }

                    dir=FileUtil.vnmrDir(base,"PANELITEMS");
                    if(dir !=null){
                        fillATable(Shuf.DB_PANELSNCOMPONENTS, dir, userName,
                                   true, info);
                    }

                    // Get Protocol Files
                    dir=FileUtil.vnmrDir(base,"PROTOCOLS");
                    if(dir !=null)
                        fillATable(Shuf.DB_PROTOCOL, dir, userName,
                                   true, info);

                    // Get Trash Files
                    dir=FileUtil.vnmrDir(base,"TRASH");

                    if(dir !=null)
                        fillATable(Shuf.DB_TRASH, dir, userName,
                                   false, info, sleepMs);

                }

                
                if(!appdirOnly) {
                    // Fill this users Workspaces if requested.  The thread crawler
                    // does not want to do this, since workspace attributes can be
                    // updated on the fly and thus the disk file is out of date.
                    if(workspace) {
                        String path = user.getVnmrsysDir();
                        if(DebugOutput.isSetFor("updatedb"))
                            Messages.postDebug("    Updating workspaces from "
                                               + path);
                        fillATable(Shuf.DB_WORKSPACE, path, userName,
                                   false, info, sleepMs);
                    }
                    // Get the imported directories/files
                    ArrayList list = readImportedDirs(user);
                    // Loop thru the list of files/dir and add them
                    // non recursively.
                    for(int k=0; k < list.size(); k++) {
                        String path = (String) list.get(k);
                        String type = getType(path);
                        
                        // Do not allow the user's home directory in this list.
                        // Too many customers have ended up with their home directory
                        // in the list and it takes a long time to search all of
                        // the extraneous directories.
                        String userdir = user.getHomeDir();
                        if(path.equals(userdir))
                            continue;

                        fillATable(type, path, userName,
                                   false, info, sleepMs);
                    }
                }
            }
            if(info.numFilesAdded > 0)
                Messages.postMessage(OTHER|INFO|LOG,
                                     "Number of Files added/updated = " +
                                     info.numFilesAdded);
            info.numFilesAdded = 0;

            // Update the LocAttrList.  Pass sleepMs on along.
            // If this is set, the update will sleep this long between
            // attributes.  We need to call this after adding all the files,
            // to get the values into the table.
            if(DebugOutput.isSetFor("updatedb"))
                Messages.postDebug("updateDB starting fillAllListsFromDB");

            attrList.fillAllListsFromDB(sleepMs * 4);
        }
        catch (InterruptedException ie) {
            // This normally means that we are closing vnmrj and
            // we need to forward this information up the line.
            throw new InterruptedException();
        }
        catch (Exception e) {
            if(!ExitStatus.exiting())
                Messages.writeStackTrace(e);
        }

        // Vacuum after updating for better user access
        vacuumDB();

        if(DebugOutput.isSetFor("updatedb")) {
            endtime = new java.util.Date();
            timems = endtime.getTime() - starttime.getTime();
            Messages.postDebug("**Time for updatedb: " + timems);
        }

    }


    /************************************************** <pre>
     * Summary: Return the names of all Attributes except tags.
     *
     </pre> **************************************************/

    public ArrayList getAllAttributeNamesFromDB(String objType)
                                                  throws Exception {
        java.sql.ResultSet rs;
        ResultSetMetaData md;
        int numCols;
        ArrayList output;
        String attr;


        java.util.Date starttime=null;
        if(DebugOutput.isSetFor("getAllAttributeNamesFromDB") ||
                       DebugOutput.isSetFor("locTiming"))
            starttime = new java.util.Date();

        output = new ArrayList();
        // Get a list of all attributes for this objType
        try {
            rs = executeQuery("SELECT * FROM " + objType +
                              " WHERE \'false\'::bool");
            if(rs == null)
                return output;

            // What we have now is all the columns, with no rows.
            // Get the column information.
            md = rs.getMetaData();
            numCols = md.getColumnCount();

            /* Get each attribute name, one at a time and test it. */
            for(int i=1; i <= numCols; i++) {
                // Get the column name
                attr = md.getColumnName(i);

                // Cull out the long_time_saved entries and tags.
                // The tag names here will have the number attached,
                // just add the attr 'tag' below.
                if(!attr.equals("long_time_saved") &&
                                 !attr.startsWith("tag"))
                    output.add(attr);
            }

            // add in a tag attr
            output.add("tag");

        } catch (Exception e) {
            haveConnection = false;
            throw e;
        }
        // Put into alphabetical or numeric order
        Collections.sort(output, new NumericStringComparator());

        if(DebugOutput.isSetFor("getAllAttributeNamesFromDB"))
            Messages.postDebug("Attribute name list: " + output);

        if(DebugOutput.isSetFor("getAllAttributeNamesFromDB") ||
                       DebugOutput.isSetFor("locTiming")) {
            java.util.Date endtime = new java.util.Date();
            long timems = endtime.getTime() - starttime.getTime();
            Messages.postDebug("getAllAttributeNamesFromDB: Time(ms) "
                               + timems + " for " + objType);
        }

        return output;

    }


    public int numTagColumns(String objType) throws SQLException {
        String colName;
        int numTags=0;
        long numCols;
        java.sql.ResultSet rs;
        ResultSetMetaData md;

        try {
            rs = executeQuery("SELECT * FROM " + objType +
                                  " WHERE \'false\'::bool");
            if(rs == null)
                return 0;

            // What we have now is all the columns, with no rows.
            // Get the column information.
            md = rs.getMetaData();
            numCols = md.getColumnCount();

            /* Get each attribute name, one at a time and test it.
               Look for tag columns and get number following tag.
            */
            for(int i=1; i <= numCols; i++) {
                colName = md.getColumnName(i);
                if(colName.startsWith("tag")) {
                    if(Character.isDigit(colName.charAt(3))) {
                        numTags++;
                    }
                }
            }
        }
        catch (SQLException pe) {
            throw pe;
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postMessage(OTHER|ERROR|LOG|MBOX,
                                 "Problem getting information from DB");
                Messages.postMessage(OTHER|ERROR|LOG,
                                 "numTagColumns failed while getting info" +
                                 " for " + objType);
                Messages.writeStackTrace(e);
            }
            return 0;
        }

        return numTags;

    }



    /************************************************** <pre>
     * Summary: Return all values for this attr from the main DB table.
     *
     *
     </pre> **************************************************/

    public ArrayList getAttrValueListFromDB(String attr, String objType,
                                            long sleepTimeMs)
                         throws Exception {
        ArrayList output;
        java.sql.ResultSet rs;
        int numTagCol;
        String value;

        java.util.Date starttime=null;
        if(DebugOutput.isSetFor("getAttrValueListFromDB") ||
                       DebugOutput.isSetFor("locTiming"))
            starttime = new java.util.Date();

        output = new ArrayList();
        if(attr.startsWith("tag")) {
            /* I have not found a way to get SQL to give DISTINCT results
               across several columns.  Thus we have to get the tag values
               for each tag column and weed out duplicated for ourselves.
            */
            /* Get the number of tag columns. */
            try {
                numTagCol = numTagColumns(objType);
            }
            catch (SQLException pe) {
                throw pe;
            }
            String user = System.getProperty("user.name");
            for(int i=0; i < numTagCol; i++) {
                try {
                    // Initially I used DISTINCT here, but some of the
                    // items took 10-30 sec.  Taking out the DISTINCT
                    // gives us back all of the rows and they get
                    // weeded out below.
                    rs = executeQuery("SELECT \"tag" + i + "\" FROM " +
                                      objType);

                    if(rs == null)
                        return output;

                    while(rs.next()) {
                        // There is only one column of results, thus, col 1

                        value = rs.getString(1);
                        // Trap out null and user:null
                        // Also trap for only the current user
                        if(value != null && !value.equals("null") &&
                           !value.equals(user + ":" + "null") &&
                           value.startsWith(user + ":")) {
                            int index = value.indexOf(':');
                            if(index >= 0)
                                value = value.substring(index +1);
                            value = value.trim();
                            // Only add it if we do not already have it.
                            if(!output.contains(value)) {
                                // Be sure it is not whitespace
                                if(value.length() > 0) {
                                    output.add(value);
                                }
                            }
                        }
                    }
                    if(sleepTimeMs > 0) {
                        try {
                            Thread.sleep(sleepTimeMs);
                        }
                        catch(InterruptedException ie) {
                            // This normally means that we are closing
                            // vnmrj and we need to forward this
                            // information up the line.
                            throw ie;
                        }
                        catch(Exception e) {
                            if(!ExitStatus.exiting())
                                Messages.writeStackTrace(e);
                            throw e;
                        }
                    }
                }
                catch (InterruptedIOException ioe) {
                    // This typically happens during shutdown.  We do not
                    // want the error printed out in that case, so
                    // throw it.
                    throw ioe;
                }
                catch (SocketException se) {
                    // This typically happens during shutdown.  We do not
                    // want the error printed out in that case, so
                    // throw it.
                    throw se;
                }
                catch (Exception e) {
                    if(!ExitStatus.exiting())
                        Messages.writeStackTrace(e);
                    throw e;
                }
            }
        }

        // Is this attr for this objType an 'array' type?
        else {
            int isArray = isParamArrayType(objType, attr);
            if(isArray == -1) {
                // Stop this error temporarily during the transition of
                // removing these two items from the DB.  
                if(!objType.equals(Shuf.DB_COMPUTED_DIR) && 
                                         !objType.equals(Shuf.DB_IMAGE_FILE) &&
                                         !attr.equals(Shuf.DB_COMPUTED_DIR)) {
                    Messages.postDebug(attr + " is not in xxx_param_list.\n"
                                   + "    It must be in the list to be used in "
                                   + "a statement for " + objType + ".");
                }
                return output;
            }

            if(isArray == 0) {
                // Not an array
                try {
                    // Limit the results else we could get a million items
                    // here and that causes problems.  As long as the results
                    // are used for menus, thousands does not make sense anyway.
                    // Note: the menus are created of type EPopButton which
                    //       has colLength defined statically and thus
                    //       partially controls how many items end up in the menu.
                    rs = executeQuery("SELECT DISTINCT \"" + attr +
                                      "\" FROM " + objType + " LIMIT 390");

                    if(rs == null)
                        return output;

                    while(rs.next()) {
                        // There is only one column of results, therefore, col 1
                        value = rs.getString(1);
                        if(value != null) {
                            // Be sure it is not whitespace
                            value = value.trim();
                            if(value.length() > 0)
                                output.add(value);
                        }
                    }
                }
                catch (InterruptedIOException ioe) {
                    // This typically happens during shutdown.  We do not
                    // want the error printed out in that case, so
                    // throw it.
                    throw ioe;
                }
                catch (Exception e) {
                    if(!ExitStatus.exiting())
                        Messages.writeStackTrace(e);
                }
            }
            // Array type attribute
            else {
                /* There is not a way to use DISTINCT on an array type.
                   Thus we have to get all values for all rows and weed out
                   duplicated for ourselves.
                */

                try {
                    // Initially I used DISTINCT here, but some of the
                    // items took 10-30 sec.  Taking out the DISTINCT
                    // gives us back all of the rows and they get
                    // weeded out below.

                    rs = executeQuery("SELECT \"" + attr + "\" FROM " +
                                      objType);

                    if(rs == null)
                        return output;

                    while(rs.next()) {
                        // Loop through the rows.

                        // Loop through each value of each row.  I can only
                        // figure out how to get a string of all values
                        // from an array type column.  So, I have to
                        // parse it myself.  The string looks like
                        // '{120,123,130}' or '{"str1", "str2"}'
                        value = rs.getString(1);
                        if(value != null && !value.equals("null")) {
                            StringTokenizer tok;
                            if(value.indexOf('\"') != -1)
                                // This will leave commas alone,  however,
                                // commas between items will end up as lone
                                // commas and we need to skip them below.
                                tok = new StringTokenizer(value, "{}\"");
                            else
                                tok = new StringTokenizer(value, "{},");
                            while(tok.hasMoreTokens()) {
                                // Loop through each value for this row.
                                String string = tok.nextToken();

                                // Add only if not already in the list and
                                // does not begin with a comma.
                                if(!output.contains(string) &&
                                   !string.startsWith(",")) {
                                    // Be sure it is not whitespace
                                    if(string.length() > 0) {

                                        output.add(string);
                                    }
                                }
                            }
                        }
                    }
                }
                catch (InterruptedIOException ioe) {
                    // This typically happens during shutdown.  We do not
                    // want the error printed out in that case, so
                    // throw it.
                    throw ioe;
                }
                catch (Exception e) {
                    if(!ExitStatus.exiting())
                        Messages.writeStackTrace(e);
                }
            }
        }

        // Alphabetize or numeric order
        Collections.sort(output,  new NumericStringComparator());

        if(DebugOutput.isSetFor("getAttrValueListFromDB")) {
            java.util.Date endtime = new java.util.Date();
            long timems = endtime.getTime() - starttime.getTime();
            Messages.postDebug("getAttrValueListFromDB: Time(ms) " + timems
                + " for " + objType + ":" + attr);
        }
        return output;
    }

    // Execute sql commands which return search results (rs)
    public java.sql.ResultSet executeQuery(String cmd) throws Exception {
        java.sql.ResultSet rs=null;
        
        // if(locatorOff() && !managedb)
        if(locatorOff())
            return null;

        checkDBConnection();
        if(sql == null || !haveConnection) {
            return null;
        }

        if(DebugOutput.isSetFor("sqlCmd"))
            Messages.postDebug("executeQuery SQL Command: " + cmd);

        rs = sql.executeQuery(cmd);

        return rs;
    }

    // Determine the number of rows which will result from executing
    // the given sql command and return that count.
    // Unfortunately, the newest postgresql.jar drivers do not give
    // a ResultSet that is scrollable.  That means that I can no
    // longer just execute rs.last() and look at the row #.
    // I tried using the "count" sql command, but it does not like
    // my long and complicated sql commands.
    // I have run out of ideas, so now I just do the full search
    // twice.  Here I do it once and count the rows.  Then the
    // code will caall executeQuery() which will do the search again.
    // Yes, Yes, it has now been slowed down by a factor of 2.
    // The only saving grace is that the newer postgresql drivers
    // can search and sort about twice as fast, so it will probably
    // be the same speed at it used to be. (GRS 8/11/11)
    public int executeQueryCount(String cmd) throws Exception {
        java.sql.ResultSet rs=null;
        int count=0;
        String countCmd, end;
        
        // if(locatorOff() && !managedb)
        if (locatorOff())
            return count;

        checkDBConnection();
        if(sql == null || !haveConnection) {
            return count;
        }

        if(DebugOutput.isSetFor("sqlCmd"))
            Messages.postDebug("executeQueryCount SQL Command: " + cmd);

 
        try {
            rs = sql.executeQuery(cmd);
            while (rs.next()) {
                count++;
            }
        }
        catch (SQLException se) {
            throw se;
        }
        return count;
    }


    // Execute sql commands not returning search results
    public int executeUpdate(String cmd) throws SQLException {
        int status=0;
        
        // if(locatorOff() && !managedb)
        if (locatorOff())
            return 0;

        checkDBConnection();
        if(sql == null || !haveConnection) {
            return 0;
        }

        if(DebugOutput.isSetFor("sqlCmd"))
            Messages.postDebug("executeUpdate SQL Command: " + cmd);

        try {
            status = sql.executeUpdate(cmd);
        }
        catch (SQLException se) {
            // Trap for errors about duplicate rows or columns and do
            // not throw them up.  These are okay.
            if(se != null && se.getMessage() != null) {
                if(se.getMessage().indexOf("already exists") != -1 &&
                   se.getMessage().indexOf("duplicate key") != -1) {
                    throw se;
                }
                else
                    return 0;
            }
            else {
                if(!ExitStatus.exiting()) {
                    Messages.writeStackTrace(se, "executeUpdate: " + cmd);
                }
                else
                    throw se;

                return 0;
            }
        }
        // Catch other exceptions like nullpointer and just exit
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.writeStackTrace(e, "executeUpdate: ");
            }
            return 0;
        }
        // If failure, output debug.
        // When tags are being cleared, there are normal failures since
        // every tag is not set.  Trap these and do not output them.
        // if(status == 0  &&  cmd.indexOf("SET tag") == -1) {

        //     if(!ExitStatus.exiting())
        //         Messages.postDebug("executeUpdate failed on command:\n   "
        //                            + cmd);
        // }
        // return status;
        return 1;
    }

    // Get the postgres version number from the pg_ctl command
    public String getPostgresVersion() {
        String cmd;
        Runtime rt = Runtime.getRuntime();
        // See if the /vnmr/pgsql/bin/pg_ctl exists.
        // if so, use it, if not, call pg_ctl without a fullpath.
        // This is to allow us to run the system postgres by
        // just moving /vnmr/pgsql/bin to a saved name out of the way.
        UNFile file = new UNFile("/vnmr/pgsql/bin/pg_ctl");
        if(file.canExecute()) {
            // Use our released one
            cmd = "/vnmr/pgsql/bin/pg_ctl --version";
        }
        else {
            // Use the system one
            cmd =  "pg_ctl --version";
        }
        String[] cmds = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION, cmd};

        Process prcs = null;
        String version="";
        try {
            prcs = rt.exec(cmds);
            BufferedReader str = (new BufferedReader
                    (new InputStreamReader
                     (prcs.getInputStream())));

            version = str.readLine();
            
            str.close();
        }
        catch (Exception ex) {  }
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
            catch (Exception ex) {}
        }
        
        return version;

    }

    // Utility to tell if the locator is turned off.  Include when run as adm tool.
    public static boolean locatorOff() {
        /********
        String sysdir = System.getProperty("sysdir");
        String usedb = System.getProperty("usedb");
        if(usedb != null && (usedb.equalsIgnoreCase("false")||usedb.equalsIgnoreCase("no")))
            return true;
        String persona = System.getProperty("persona");
        if(persona != null && persona.equalsIgnoreCase("adm"))
            return true;

        // If the pgsql data directory does not exists, force to locator off
//        String pgsqlPath = sysdir + FileUtil.separator + "pgsql" +  FileUtil.separator + "data";
//        UNFile dataFile = new UNFile(pgsqlPath);
// If using the system installed postgresql, it will not be at this path
// UNLESS, we specify it to be there.  Don't tst it for now.
//        if(!dataFile.exists())
//            return true;
        
        ********/

        return locatoroff;
    }

    // Set variable determining if locator is active or not
    public void setLocatorOff(boolean setting) {
        locatoroff = setting;

        // Write out the new value to the persistence file
        writeLocatorOff(setting);
        
        // Cause another shuffle.
        SessionShare sshare = ResultTable.getSshare();
        StatementHistory history = sshare.statementHistory();
        history.updateWithoutNewHistory();

        String persona = System.getProperty("persona");
        if (persona != null && persona.equalsIgnoreCase("adm"))
            locatoroff = true;
    }

    // Convienent method to accept value as String
    static public void setDateLimitMsFromDays(String limitDays) {
        int limit;

        try {
            limit = Integer.parseInt(limitDays);
        }
        catch (Exception e) {
            // No error output
            limit = -1;
        }
        setDateLimitMsFromDays(limit);
    }
     
    // Get the dateLimit for keeping vnmr_data in the locator
    // dataLimitDays = 0 means nothing is kept except varian data
    // dataLimitDays = -1 means everything is kept
    // dataLimitDays greater than 0 means keep this many weeks of data
    // dataLimitDays is set with -Ddatelimit=# in the java startup cmd line    
    static public void setDateLimitMsFromDays(int dataLimitDays) {
        Calendar dateLimit = Calendar.getInstance();
        java.util.Date dateToday = new java.util.Date();

        if(dataLimitDays < 0) {
            // a neg number means keep data forever, thus a limit of 0
            dateLimit.setTime(new java.util.Date(0));
        }
        else if(dataLimitDays == 0) {
            // 0 days means nothing is kept, just use a future date here
            dateLimit.setTimeInMillis(FUTURE_DATE);
        }
        else {
            // must have a positive number of days specified
            // Subtract that amount from today for the limit
            dateLimit.setTime(dateToday);
            dateLimit.add(Calendar.DAY_OF_MONTH, -dataLimitDays);
        }
        // Save this
        dateLimitMS = dateLimit.getTimeInMillis();

        // Write out the value in days to the persistence file
        writeDateLimit(dataLimitDays);
    }


    static protected void writeDateLimit(int limit) {
	String filepath;
        FileWriter fw;
        PrintWriter os;

        filepath = FileUtil.savePath(FileUtil.sysdir()
                                         + "/pgsql/persistence/AgeLimit");

	try {
            fw = new FileWriter(filepath);
            os = new PrintWriter(fw);
            os.println("DateLimit  " + limit);
            os.close();
            // Change the mode to all write access
            String[] cmd = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION,
                            "chmod 666 " + filepath};
            Runtime rt = Runtime.getRuntime();
            Process prcs = null;
            try {
                prcs = rt.exec(cmd);
            }
            finally {
                // It is my understanding that these streams are left
                // open sometimes depending on the garbage collector.
                // So, close them.
                if(prcs != null) {
                    OutputStream ost = prcs.getOutputStream();
                    if(ost != null)
                        ost.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
	}
	catch (Exception e) {
            Messages.postError("Problem writing " + filepath);
            Messages.writeStackTrace(e);
	}
    }

    static public String getLocatorOffFile() {
        return FileUtil.savePath(FileUtil.sysdir()
                         + "/pgsql/persistence/LocatorOff");
    }

    static public void writeLocatorOff(boolean setting) {
	String filepath;
        FileWriter fw;
        PrintWriter os;

        // filepath = FileUtil.savePath(FileUtil.sysdir()
        //                                  + "/pgsql/persistence/LocatorOff");

        filepath = getLocatorOffFile();
        Process prcs = null;
	try {
            fw = new FileWriter(filepath);
            os = new PrintWriter(fw);
            if(setting)
                os.println("locatorOff true");
            else
                os.println("locatorOff false");
            os.close();
            // Change the mode to all write access
            String[] cmd = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION,
                            "chmod 666 " + filepath};
            Runtime rt = Runtime.getRuntime();
            prcs = rt.exec(cmd);
	}
	catch (Exception e) {
            Messages.postError("Problem writing " + filepath);
            Messages.writeStackTrace(e);
	}
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(prcs != null) {
                    OutputStream ost = prcs.getOutputStream();
                    if(ost != null)
                        ost.close();
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
    }

    static public boolean readLocatorOff(boolean usePersona) {
        BufferedReader in;
        String line;
        String value;
        StringTokenizer tok;

        String usedb = System.getProperty("usedb");
        if (usedb != null) {
            if (usedb.equalsIgnoreCase("false")||usedb.equalsIgnoreCase("no")) {
               locatoroff = true;
               return locatoroff;
            }
        }
        if (usePersona) {
            String persona = System.getProperty("persona");
            if (persona != null && persona.equalsIgnoreCase("adm")) {
                locatoroff = true;
                return locatoroff;
            }
        }

        String filepath = FileUtil.openPath(FileUtil.sysdir()
                         + "/pgsql/persistence/LocatorOff");

        // Temporarily try the users file in case they had set this in
        // a previous version.
        
        if(filepath==null) {
            filepath = FileUtil.openPath("USER/PERSISTENCE/LocatorOff");
            if(filepath==null)
            {
                locatoroff = false;
                return locatoroff;
            }
        }
	try {
            in = new BufferedReader(new FileReader(filepath));
            line = in.readLine();
            if(!line.startsWith("locatorOff")) {
                in.close();
                File file = new File(filepath);
                // Remove the corrupted file.
                file.delete();
                value = "false";
            }
            else {
                tok = new StringTokenizer(line, " \t\n");
                value = tok.nextToken();
                if (tok.hasMoreTokens()) {
                    value = tok.nextToken();
                }
                else
                    value = "false";
                in.close();
            }
	}
	catch (Exception e) {
	    // No error output here.
            value = "false";
	}

        if(value.startsWith("t"))
            locatoroff = true;
        else
            locatoroff = false;

        return locatoroff;
    }

    static public String readDateLimit() {
	String filepath;
        BufferedReader in;
        String line;
        String value;
        StringTokenizer tok;
        int newValue;


        filepath = FileUtil.openPath(FileUtil.sysdir()
                                         + "/pgsql/persistence/AgeLimit");
        if(filepath==null) {
            // Temporarily try the users file in case they had set this in
            // a previous version.
            filepath = FileUtil.openPath("USER/PERSISTENCE/DateLimit");
            if(filepath==null)
                return "-1";
        }
	try {
            in = new BufferedReader(new FileReader(filepath));
            line = in.readLine();
            if(!line.startsWith("DateLimit")) {
                in.close();
                File file = new File(filepath);
                // Remove the corrupted file.
                file.delete();
                // -1 means keep everything
                value = "-1";
            }
            else {
                tok = new StringTokenizer(line, " \t\n");
                value = tok.nextToken();
                if (tok.hasMoreTokens()) {
                    value = tok.nextToken();
                    in.close();
                }
                else {
                    in.close();
                    File file = new File(filepath);
                    // Remove the corrupted file.
                    file.delete();
                    // -1 means keep everything
                    value = "-1";
                }
            }
	}
	catch (Exception e) {
	    // No error output here.
            value = "-1";
	}

        try {
            newValue = Integer.parseInt(value);
        }
        catch (Exception e) {
            // No error output
            newValue = -1;

            File file = new File(filepath);
            // Remove the corrupted file.
            file.delete();
        }

        setDateLimitMsFromDays(newValue);

        // return the time in days
        return value;
    
    }



    // We need 3 possible return values
    // 0 = false
    // 1 = true
    // -1 = param not found

    // I found a way to find out if an attribute in the DB is an
    // array type (starting with "_").  I ended up not using it but wanted to
    // note it here for future reference
    //     Get the sql datatype.  Those starting with "_" are arrays
    //     datatype = md.getColumnTypeName(i);

    public int isParamArrayType(String objType, String param) {
        ShufDataParam sdp;

        if(objType.equals(Shuf.DB_VNMR_DATA) ||
                objType.equals(Shuf.DB_VNMR_PAR) ||
                objType.equals(Shuf.DB_WORKSPACE) ||
                objType.equals(Shuf.DB_VNMR_REC_DATA) ||
                objType.equals(Shuf.DB_VNMR_RECORD)) {

            sdp = (ShufDataParam)shufDataParams.get(param);

            // If the param does not exist in the list, return null to
            // notify the caller of this.
            if(sdp == null)
                return -1;

            if(sdp != null) {
                if(sdp.getType().endsWith("[]"))
                    return 1;
                else
                    return 0;
            }
            else
                return 0;
        }
        else if(objType.equals(Shuf.DB_STUDY) || 
                objType.equals(Shuf.DB_LCSTUDY)) {
            sdp = (ShufDataParam)shufStudyParams.get(param);
            // If the param does not exist in the list, return null to
            // notify the caller of this.
            if(sdp == null)
                return -1;

             if(sdp != null && sdp.getType().endsWith("[]"))
                 return 1;
             else
                 return 0;

        }
        else if(objType.equals(Shuf.DB_PROTOCOL)) {
            sdp = (ShufDataParam)shufProtocolParams.get(param);
            // If the param does not exist in the list, return null to
            // notify the caller of this.
            if(sdp == null)
                return -1;

             if(sdp != null && sdp.getType().endsWith("[]"))
                 return 1;
             else
                 return 0;

        }
        else if(objType.equals(Shuf.DB_IMAGE_DIR) ||
                objType.equals(Shuf.DB_IMAGE_FILE) ||
                objType.equals(Shuf.DB_COMPUTED_DIR)) {
            sdp = (ShufDataParam)shufImageParams.get(param);
            // If the param does not exist in the list, return -1 to
            // notify the caller of this.
            if(sdp == null)
                return -1;

             if(sdp != null && sdp.getType().endsWith("[]"))
                 return 1;
             else
                 return 0;

        }
        else
            return 0;

    }

    /******************************************************************
     * Summary: Read in the list of imported directories.
     *
     *   This list is not used ImportDialog, but ImportDialog kepts it up
     *   to date for use when the DB needs to be recreated.
     *   This static read is put in here because managedb does not
     *   have access to ImportDialog.
     *****************************************************************/
    static public ArrayList readImportedDirs(User user) {
        ArrayList list;
        UNFile file;

// Stop Using this file and use the Vnmrj adm Data Directories instead 7/13
list = new ArrayList();
return list;

//       String filepath = FileUtil.userDir(user, "PERSISTENCE") +
//                          File.separator + "ImportedDirs";
//
//        list = new ArrayList();
//
//        if(filepath != null) {
//
//            BufferedReader in;
//            String line;
//            String path;
//            StringTokenizer tok;
//            try {
//                file = new UNFile(filepath);
//                in = new BufferedReader(new FileReader(file));
//                // File must start with 'Imported Directories'
//                if((line = in.readLine()) != null) {
//                    if(!line.startsWith("Imported Directories")) {
//                        Messages.postWarning("The " + filepath + " file is " +
//                                             "corrupted and being removed");
//                        // Remove the corrupted file.
//                        file.delete();
//                        return list;
//                    }
//                }
//                while ((line = in.readLine()) != null) {
//                    if(!line.startsWith("#")) {// skip comments.
//                        tok = new StringTokenizer(line, " :,\t\n");
//                        path = tok.nextToken();
//                        if(path.indexOf(File.separator) >= 0) {
//                            // See if the file/dir still exists.  Otherwise,
//                            // files which disappear will stay in this list
//                            // forever.
//                            file = new UNFile(path);
//                            if(file.exists()) {
//                                // It exists, add it
//                                list.add(path);
//                            }
//                        }
//                    }
//                }
//                in.close();
//            }
//            catch (Exception ioe) { }
//        }
//        return list;
    }


    /******************************************************************
     * Summary: Determine the objType for the file/dir at this path.
     *
     *
     *****************************************************************/
    static public String getType(String path) {
        UNFile file = new UNFile(path);
        String name = file.getName();

        return getType(path, name);
    }



    /******************************************************************
     * Summary: Determine the objType for the file/dir at this path.
     *
     *
     *****************************************************************/
    static public String getType(String path, String name) {
        String type = "?";
        
        // If path contains a colon, it means we have been passed the
        // hostfullpath.  We need to remove the 
        try {
            String dirName = path;
            UNFile pathFile = new UNFile(path);
            UNFile studyfile = new UNFile(path + File.separator + "studypar");
            UNFile curparfile = new UNFile(path + File.separator + "curpar");
            UNFile lcfile = new UNFile(path + File.separator + "lcpar");
            UNFile autofile = new UNFile(path + File.separator + "autopar");
            int last = path.lastIndexOf(File.separator, path.length()-1);
            int second = path.lastIndexOf(File.separator, last-1);
            if(last != -1 && second != -1 && last > second+1)
                dirName = path.substring(second+1, last);

            if(name.endsWith(Shuf.DB_FID_SUFFIX))
                type = Shuf.DB_VNMR_DATA;
            else if(name.endsWith(Shuf.DB_PAR_SUFFIX))
                type = Shuf.DB_VNMR_PAR;
            else if(name.endsWith(Shuf.DB_REC_SUFFIX))
                type = Shuf.DB_VNMR_RECORD;
            else if(name.endsWith(Shuf.DB_REC_SUFFIX.toLowerCase()))
                type = Shuf.DB_VNMR_RECORD;
            // If file is within .rec or .REC, it must be rec_data
            else if(dirName.endsWith(Shuf.DB_REC_SUFFIX) || 
                    dirName.endsWith(Shuf.DB_REC_SUFFIX.toLowerCase()))
                type = Shuf.DB_VNMR_REC_DATA;
            else if(name.endsWith(Shuf.DB_IMG_DIR_SUFFIX))
                type = Shuf.DB_IMAGE_DIR;
            // Even though we are not adding .fdf, .vfs and .crft files 
            // to the DB, keep this type for D&D perposes
            else if(name.endsWith(Shuf.DB_IMG_FILE_SUFFIX))
                type = Shuf.DB_IMAGE_FILE;
            else if(name.endsWith(Shuf.DB_VFS_SUFFIX))
                type = Shuf.DB_VFS;
            else if(name.endsWith(Shuf.DB_CRFT_SUFFIX))
                type = Shuf.DB_CRFT;
            else if(dirName.equals(Shuf.DB_SHIMS))
                type = Shuf.DB_SHIMS;
            else if(dirName.equals("maclib"))
                type = Shuf.DB_COMMAND_MACRO;
            else if(dirName.equals("protocols"))
                type = Shuf.DB_PROTOCOL;
            // Allow icons to be in the icons directory here, OR if nothing
            // else catches the type, try at the end to see if it is an
            // image file type
            else if(dirName.equals("icons"))
                type = Shuf.DB_ICON;
            // There can be sub-directories under mollib, do not tag them
            // as molecule types.
            else if(dirName.equals("mollib")) {
                if(!pathFile.isDirectory())
                    type = Shuf.DB_MOLECULE;
            }
            // They want any arbitrary subdirectories under mollib 
            // (except icons) to be molecule type.  If we have here a file
            // (not a directory) and it was not caught by the "icons" test
            // above, then see if mollib is anyplace in the path.  If it
            // is, set to molecule type.
            else if(!pathFile.isDirectory() && path.indexOf("mollib") != -1) {
                type = Shuf.DB_MOLECULE;
            }
            // See if the directory contains a studypar file
            else if(studyfile.exists())
                type = Shuf.DB_STUDY;
             // See if the directory contains an lcpar file
            else if(lcfile.exists())
                type = Shuf.DB_LCSTUDY;
             // See if the directory contains an autopar file
            else if(autofile.exists())
                type = Shuf.DB_AUTODIR;
           // Check for .xml which could be protocol or panelNcomponent
            else if(name.endsWith(Shuf.DB_PROTOCOL_SUFFIX)) {
                // See if it is a protocol by looking into the file
                BufferedReader in;
                String line;
                in = new BufferedReader(new FileReader(pathFile));
                while ((line = in.readLine()) != null) {
                    if(line.indexOf("protocol title") != -1) {
                        type = Shuf.DB_PROTOCOL;
                        break;
                    }
                }
                in.close();

                // If it was not a protocol, then tag it as panelNcomp
                if(type.equals("?"))
                    type = Shuf.DB_PANELSNCOMPONENTS;
            }
            else if(name.startsWith(Shuf.DB_WKS_PREFIX)) {
                // Is probably a workspace, but check for a curpar
                if(curparfile.exists()) {
                    type = Shuf.DB_WORKSPACE;
                }
            }
            else if(dirName.equals("studies"))
                type = Shuf.DB_STUDY;
            else {
                // Okay, if we got here, it has not been identified at all.
                // Check to see if it is an image type file readable by the
                // "imagefile" vnmr command.  This ends up using the system
                // command /usr/bin/convert to convert to .gif format
                // It should work for anything "convert" supports, what ever
                // that list is.
                // Right now the .tif and .tiff don't seem to work, but I believe
                // that is "convert".
                if(path.endsWith(".gif") || path.endsWith(".jpg") || 
                   path.endsWith(".jpeg") || path.endsWith(".png") ||
                   path.endsWith(".bmp") || path.endsWith(".tif") ||
                   path.endsWith(".tiff")) {

                    type = Shuf.DB_ICON;
                }

                // Now they want .mol in any directory also.  If it is in mollib
                // then it was caught above.  Else, see if the file is .mol
                if(path.endsWith(".mol"))
                    type = Shuf.DB_MOLECULE;

            }
        }
        catch (Exception e) {
            if(!ExitStatus.exiting())
                Messages.writeStackTrace(e,"Caught in FillDBManager.getType()");
        }

        return type;
    }

    protected String checkDBversion() {
        String cmd;
        java.sql.ResultSet rs;
        String version="";
        String vnmrjversion="";

        /**** I need to make the locator work with any release of postgres.
              Thus, I will not know what version they are running.
              See if we can get by with not checking.
        *****/ 
        return "okay";





         // if(locatorOff() && !managedb)
         //     return "okay";
        
         // // Get host string from DB
         // /* Create SQL command   */
         // cmd = new String("SELECT hostname  FROM " + Shuf.DB_VERSION);
         // try {

         //     rs = executeQuery(cmd);
         //     if(rs == null)
         //         return "bad";
         // }
         // catch (Exception e) {
         //     if(!ExitStatus.exiting())
         //         Messages.postMessage(OTHER|LOG,
         //                       "Problem getting hostname from version DB table");
         //     return "bad";
         // }


       //  // Get version string from the version table in the DB
       //  /* Create SQL command   */
       //  cmd = new String("SELECT version  FROM " + Shuf.DB_VERSION);
       //  try {
       //      rs = executeQuery(cmd);
       //      if(rs == null)
       //          return "bad";

       //      if(rs.next()) {
       //          // getString() gives the string representation even if
       //          // the column type is int or float or date
       //          version = rs.getString(1);
       //      }
       //  }
       //  catch (Exception e) {
       //      if(!ExitStatus.exiting())
       //          Messages.postMessage(OTHER|LOG,
       //                         "Problem getting version from version DB table");
       //      return "bad";
       //  }

       //  // Compare version retrieved to current version in DBVERSION
       //  if(!version.equals(DBVERSION)) {
       //      // If we got here, the versions don't match.
       //      return "bad";
       //  }

       //  // Now see if the current DB was created by this version of
       //  // vnmrj/managedb.  If not, have it rebuilt.  This is to avoid
       //  // having to rebuild multiple times for those of us who install 
       //  // multiple times.  This is only for managedb, not vnmrj

        // if (managedb) {
        //     // Get vnmrjversion string from the version table in the DB
        //     /* Create SQL command */
        //     cmd = new String("SELECT vnmrjversion  FROM " + Shuf.DB_VERSION);
        //     try {
        //         rs = executeQuery(cmd);
        //         // If this field does not exist yet, then cause a rebuild
        //         if (rs == null)
        //             return "bad";

        //         if (rs.next()) {
        //             vnmrjversion = rs.getString(1);
        //         }
        //     }
        //     catch (Exception e) {
        //         if (!ExitStatus.exiting())
        //             // Do not post an error, just return bad
        //             Messages.postMessage(OTHER|LOG,
        //                   "Problem getting vnmrjversion from the version table");
        //         return "bad";
        //     }
        //     // Get the current vnmrj version
        //     String curVer = getVnmrjVersion();

        //     // Compare vnmrjversion retrieved to current version
        //     if (!vnmrjversion.equals(curVer)) {
        //         // We need to rebuild
        //         return "bad";
        //     }
        // }

        // return "okay";
    }


    public static String getUnixOwner(String fullpath) {
        Process   proc=null;
        String   owner="";
        try {
            Runtime rt = Runtime.getRuntime();
            String dir = System.getProperty("sysdir");

            // If a file or directory does not exist, fileowner will
            // return an owner of 'root'
            String cmd;
            
            // Windows needs double quotes around the filename in case
            // there are any spaces.  Unix fileowner did not like the quotes.
            if(UtilB.OSNAME.startsWith("Windows"))
                cmd = dir +"/bin/fileowner \"" + fullpath + "\"";
            else
                cmd = dir +"/bin/fileowner " + fullpath ;

            proc = rt.exec(cmd);

            BufferedReader str = (new BufferedReader
                                      (new InputStreamReader
                                       (proc.getInputStream())));

            owner = str.readLine();
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postDebug("Problem getting owner of " + fullpath);
                Messages.writeStackTrace(e);
            }
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(proc != null) {
                    OutputStream os = proc.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = proc.getInputStream();
                    if(is != null)
                        is.close();
                    is = proc.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }
        return owner;
    }

    /******************************************************************
     * Summary: Output the number if items in each table of the DB to Log file.
     *
     *
     *****************************************************************/
    public void dbStatus() {
        java.sql.ResultSet rs;
        int numRows;
        String cmd;
        String version="empty";
        String networkmode="unknown";

        try {
            infoIfManagedbLogIfVnmrj("** Start DataBase Contents Report **");

            cmd = new String("SELECT version, networkmode  FROM "
                             + Shuf.DB_VERSION);
            rs = executeQuery(cmd);

            if(rs != null && rs.next()) {
                // getString() gives the string representation even if
                // the column type is int or float or date
                version = rs.getString(1);
                networkmode = rs.getString(2);
            }

            infoIfManagedbLogIfVnmrj("DB version: " + version
                             + "  DB Host: " +  dbHost
                             + "\n              Local Host: " + localHost
                             + "     Network Mode: " + networkmode);

            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_VNMR_DATA);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_VNMR_DATA
                               + " items in DB = " + numRows);

            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_VNMR_PAR);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_VNMR_PAR
                               + " items in DB = " + numRows);

            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_VNMR_RECORD);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_VNMR_RECORD
                               + " items in DB = " + numRows);

            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_VNMR_REC_DATA);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_VNMR_REC_DATA
                               + " items in DB = " + numRows);

            numRows = 0;
            rs = executeQuery("SELECT fullpath FROM " + Shuf.DB_AUTODIR);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_AUTODIR
                               + " items in DB = " + numRows);

            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_WORKSPACE);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_WORKSPACE
                               + " items in DB = " + numRows);

            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_PANELSNCOMPONENTS);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_PANELSNCOMPONENTS
                               + " items in DB = " + numRows);

            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_SHIMS);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_SHIMS
                               + " items in DB = " + numRows);

            numRows = 0;
            rs = executeQuery("SELECT name FROM " + Shuf.DB_COMMAND_MACRO);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_COMMAND_MACRO
                               + " items in DB = " + numRows);

            numRows = 0;
            rs = executeQuery("SELECT name FROM " + Shuf.DB_PPGM_MACRO);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_PPGM_MACRO
                               + " items in DB = " + numRows);


            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_PROTOCOL);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_PROTOCOL
                               + " items in DB = " + numRows);

            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_STUDY);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_STUDY
                               + " items in DB = " + numRows);
            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_LCSTUDY);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_LCSTUDY
                               + " items in DB = " + numRows);
            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_IMAGE_DIR);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_IMAGE_DIR
                               + " items in DB = " + numRows);
            numRows = 0;
            rs = executeQuery("SELECT filename FROM " + Shuf.DB_TRASH);
            if(rs != null)
                while(rs.next())
                    numRows++;
            infoIfManagedbLogIfVnmrj("Number of " + Shuf.DB_TRASH
                               + " items in DB = " + numRows);

            infoIfManagedbLogIfVnmrj("Total Number of " + Shuf.DB_VNMR_DATA +
                               " attributes = " + shufDataParams.size());

            infoIfManagedbLogIfVnmrj("Total Number of " + Shuf.DB_STUDY +
                               " attributes = " + shufStudyParams.size());

            infoIfManagedbLogIfVnmrj("Total Number of " + Shuf.DB_PROTOCOL +
                               " attributes = " + shufProtocolParams.size());

            infoIfManagedbLogIfVnmrj("Total Number of " + Shuf.DB_IMAGE_DIR +
                               " attributes = " + shufImageParams.size());



            infoIfManagedbLogIfVnmrj("** End DataBase Contents Report **");
        }
        catch (Exception e) {
            if(!ExitStatus.exiting())
                Messages.writeStackTrace(e);
        }
    }

    // Test to see whether we are in managedb or vnmrj.  If we are in
    // managedb, write the output via postDebug so that it goes into the
    // log file and to stdout.    If Vnmrj, write out to postLog
    static public void infoIfManagedbLogIfVnmrj(String msg) {
        if(managedb)
            Messages.postDebug(msg);
        else
            Messages.postLog(msg);
    }

    public int getNumFidsInDB() {
        java.sql.ResultSet rs;
        int numRows=0;

        try {
        rs = executeQuery("SELECT filename FROM " + Shuf.DB_VNMR_DATA);
        if(rs != null)
            while(rs.next())
                numRows++;
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return 0;
        }
        return numRows;
    }

    public void updateThisTable (String objType) {
        User      user;
        String    userName;
        String    dir;
        ArrayList dataDirs;
        DBCommunInfo info = new DBCommunInfo();
        boolean   foundit=false;

        // If no connection, this will output one error, then we can quit.
        if(checkDBConnection() == false)
            return;

        if(DebugOutput.isSetFor("updateThisTable"))
            Messages.postDebug("updateThisTable Updating " + objType
                               + " table.");

        if(objType.equals("all")) {
            try {
                LoginService loginService = LoginService.getDefault();
                Hashtable users = loginService.getuserHash();

                // Just call the normal updateDB routine and let it go
                updateDB(users, 0);
            }
            catch (InterruptedException e) {}
            if(DebugOutput.isSetFor("updateThisTable"))
                Messages.postDebug("updateThisTable finished updating.");

            return;

        }
        // Both macro types are taken care of here.  They are not related
        // to users.
        if(objType.endsWith("macro")) {
            updateMacro(Shuf.DB_COMMAND_MACRO);
            updateMacro(Shuf.DB_PPGM_MACRO);
            if(DebugOutput.isSetFor("updateThisTable"))
                Messages.postDebug("updateThisTable finished updating.");

            // Update the LocAttrList.
            try {
                attrList.fillAllListsFromDB(0);
            }
            catch (Exception e) {
                if(!ExitStatus.exiting()) {
                    Messages.postError("Problem updating LocattrList");
                    Messages.writeStackTrace(e);
                }
                return;
            }
            return;
        }

        // Check to see that this is a valid table type
        for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
            if(objType.equals(Shuf.OBJTYPE_LIST[i]))
                foundit = true;
        }
        if(!foundit) {
            Messages.postError(objType + " not a valid DB table.");
            return;
        }


        // Update the LocAttrList DB table.
//         try {
//             attrList.fillAllListsFromDB(0);
//         }
//         catch (Exception e) {
//             if(!ExitStatus.exiting()) {
//                 Messages.postError("Problem updating LocattrList");
//                 Messages.writeStackTrace(e);
//             }
//             else
//                 return;
//         }

        cleanupDBListThisType(objType, info);

        // Clean up the SymLinkList.
        if(symLinkMap != null)
            symLinkMap.cleanDirChecked();


        LoginService loginService = LoginService.getDefault();
        Hashtable users = loginService.getuserHash();


        // Go thru the users
        for(Enumeration en = users.elements(); en.hasMoreElements(); ) {
            user = (User) en.nextElement();
            userName = user.getAccountName();

            if(DebugOutput.isSetFor("updateThisTable"))
                Messages.postDebug("updateThisTable Updating for user, "
                                   + userName);


            // Now loop thru all of this users data directories as
            // specified in userList.
            dataDirs = user.getDataDirectories();

            for(int i=0; i < dataDirs.size(); i++) {
                dir = (String)dataDirs.get(i);
                if(DebugOutput.isSetFor("updateThisTable"))
                    Messages.postDebug("    Updating dir, " + dir
                                       + " for objType " + objType);
                if(objType.startsWith("vnmr_") ||
                                 objType.equals(Shuf.DB_AUTODIR) ||
                                 objType.equals(Shuf.DB_STUDY) ||
                                 objType.equals(Shuf.DB_LCSTUDY) ||
                                 objType.equals(Shuf.DB_IMAGE_DIR) ||
                                 objType.equals(Shuf.DB_COMPUTED_DIR))
                    // Fill DB with files in this directory and below.
                    fillATable(objType, dir, userName, true,
                               info, 0);
            }

            // Get directories to use for FDA records (.REC)
            // Fill records and the record data within the record
            if(objType.equals(Shuf.DB_VNMR_RECORD)) {
                dataDirs = user.getP11Directories();
                for(int i=0; i < dataDirs.size(); i++) {
                    dir = (String)dataDirs.get(i);
                    if(DebugOutput.isSetFor("updatedb"))
                        Messages.postDebug("    Updating P11 dir, " + dir);
                    // Fill DB with files in this directory and below.
                    fillATable(Shuf.DB_VNMR_RECORD, dir, userName, true,
                               info, 0);
                }
            }

            ArrayList appDirs=user.getAppDirectories();

            for(int i=0; i < appDirs.size(); i++) {
                String base=(String)appDirs.get(i);
                if(DebugOutput.isSetFor("updatedb"))
                    Messages.postDebug("    Updating appDir base dir, " + base);

                if(objType.equals(Shuf.DB_SHIMS)) {
                    dir=FileUtil.vnmrDir(base,"SHIMS");
                    if(dir !=null)
                        fillATable(Shuf.DB_SHIMS, dir, userName,
                                   true, info);
                    if(DebugOutput.isSetFor("updateThisTable"))
                        Messages.postDebug("updateThisTable finished.");
                }

                // Get this users xml directory and fill from it.

                else if(objType.equals(Shuf.DB_PANELSNCOMPONENTS)) {
                    dir=FileUtil.vnmrDir(base,"LAYOUT");
                    if(dir !=null){
                        fillATable(Shuf.DB_PANELSNCOMPONENTS, dir, userName,
                                   true, info);
                    }

                    dir=FileUtil.vnmrDir(base,"PANELITEMS");
                    if(dir !=null){
                        fillATable(Shuf.DB_PANELSNCOMPONENTS, dir, userName,
                                   true, info);
                    }
                    if(DebugOutput.isSetFor("updateThisTable"))
                        Messages.postDebug("updateThisTable finished.");
                }

                // Get Protocol Files
                else if(objType.equals(Shuf.DB_PROTOCOL)) {
                    dir=FileUtil.vnmrDir(base,"PROTOCOLS");
                    if(dir !=null)
                        fillATable(Shuf.DB_PROTOCOL, dir, userName,
                                   true, info);
                    if(DebugOutput.isSetFor("updateThisTable"))
                        Messages.postDebug("updateThisTable finished.");
                }
                // Get Trash Files
                else if(objType.equals(Shuf.DB_TRASH)) {
                    dir=FileUtil.vnmrDir(base,"TRASH");

                    if(dir !=null)
                        fillATable(Shuf.DB_TRASH, dir, userName,
                                   false, info, 0);
                    if(DebugOutput.isSetFor("updateThisTable"))
                        Messages.postDebug("updateThisTable finished.");
                }


            }
            // Fill this users Workspaces
            if(objType.equals(Shuf.DB_WORKSPACE)) {
                String path = user.getVnmrsysDir();
                if(DebugOutput.isSetFor("updatedb"))
                    Messages.postDebug("    Updating workspaces from " + path);
                fillATable(Shuf.DB_WORKSPACE, path, userName,
                           false, info, 0);
                if(DebugOutput.isSetFor("updateThisTable"))
                    Messages.postDebug("updateThisTable finished.");
            }

        }
        if(info.numFilesAdded > 0)
            Messages.postMessage(OTHER|INFO|LOG,
                                 "Number of Files added/updated = " +
                                 info.numFilesAdded);
        info.numFilesAdded = 0;

        // Update the LocAttrList.
        try {
            attrList.fillAllListsFromDB(0);
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem updating LocattrList");
                Messages.writeStackTrace(e);
            }
        }
        if(DebugOutput.isSetFor("updateThisTable"))
        Messages.postDebug("updateThisTable finished with update.");

    }


    /************************************************** <pre>
     * Summary: Set the value for a given attribute for a given filepath where
     *          the attribute is not a tag.
     *
     * Details:
     *    Example of calling this method:
     *
     *    ShufflerService shufflerService = ShufflerService.getDefault();
     *    ShufDBManager dbManager = shufflerService.getdbManager();
     *    boolean value = dbManager.setAttributeValue(objType, fullpath,
     *                                          host, attr, attrType, attrVal);
     *        where:
     *           objType = Shuf.DB_VNMR_DATA, Shuf.DB_PANELSNCOMPONENTS,
     *                      etc.
     *           fullpath = The filename and path starting at /
     *           host =     The host name that added the file to the DB
     *           attr =     Which attribute to set the value for
     *           attrType = DB data type: text, int, float8 or date
     *           attrVal =  Text string with new value
     *
     *        Return true for success and false for failure
     *
     *    This same job can be done by calling ShufDBManager.setAttributeValue()
     *    However, ShufDBManager is not available from managedb.  managedb
     *    needs to set some non tag attributes and including the tag stuff
     *    here in FillDBManager, causes compile problems.  So, this is basically
     *    a copy of ShufDBManager.setAttributeValue() with the tag stuff
     *    removed.
     *
     </pre> **************************************************/

    public boolean setNonTagAttributeValue(String objType, String fullpath,
                                     String host, String attr, String attrType,
                                     String attrVal) {
        StringBuffer cmd;
        String dpath, dhost;
        UNFile file;

        // if (locatorOff() && !managedb)
        if (locatorOff())
            return false;

        if(DebugOutput.isSetFor("setAttributeValue")) {
            Messages.postDebug("setNonTagAttributeValue: objType = " + objType
                               + " attr = " + attr + " attrVal = " + attrVal
                               + "\nfullpath = " + fullpath);
        }

        try {
            file = new UNFile(fullpath);

            // If the file does not exist, bail out
            if(!file.exists()) {
                return false;
            }

            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch (Exception e) {
            Messages.postError("Problem updating attribute value for\n    "
                               + fullpath);
            Messages.writeStackTrace(e);
            return false;
        }

        // See if this attribute exists.  If not, it will be added.
        if(isAttributeInDB(objType, attr, attrType, dpath)) {
            if(objType.endsWith(Shuf.DB_MACRO)) {
                cmd = new StringBuffer("UPDATE ");
                cmd.append(objType).append(" SET \"").append(attr);
                cmd.append("\" = \'").append(attrVal);
                cmd.append("\' WHERE name = \'").append(dpath);
                cmd.append("\'");
            }
            else {
                cmd = new StringBuffer("UPDATE ");
                cmd.append(objType).append(" SET \"").append(attr);
                cmd.append("\" = \'").append(attrVal);
                cmd.append("\' WHERE host_fullpath = \'");
                cmd.append(dhost).append(":").append(dpath);
                cmd.append("\'");
            }
            try {
                executeUpdate(cmd.toString());
                return true;
            }
            catch (Exception e) {
                Messages.postError("Problem setting value for \n    " +
                                   attr + " in " + dpath);
                Messages.writeStackTrace(e,"setNonTagAttributeValue: objType = "
                                         + objType + "\fullpath = "
                                         + dpath + "\nattr = " +
                                         attr + " attrType = " + attrType
                                         + " attrVal = " + attrVal);
                return false;
            }
        }

        return true;
    }

    /******************************************************************
     * Summary: Set entries in DB_AVAIL_SUB_TYPES as per list.
     *
     *    The DB_AVAIL_SUB_TYPES table is not used as other tables in
     *    the DB.  It is being used so that a specified set of types
     *    can be shown to the user in the locator.  Given the structure
     *    of the coding, the easiest way to put something into the
     *    locator was to create a DB table and have it contain what
     *    needs to be shown.
     *****************************************************************/
    public void setAvailSubTypes(ArrayList typeList) {
        String cmd;

        // First empty the table
        clearOutTable(Shuf.DB_AVAIL_SUB_TYPES, "all");

        // if (locatorOff() && !managedb)
        if (locatorOff())
            return;

        // Now loop through the list and insert the types into the table.
        for(int i=0; i < typeList.size(); i++) {
            String type = (String) typeList.get(i);
            cmd = "INSERT INTO " + Shuf.DB_AVAIL_SUB_TYPES
                  + " (types, filename, fullpath) VALUES (\'"
                + type + "\', \'" + type + "\', \'" + type + "\')";
            try {
                executeUpdate(cmd);
            }
            catch(Exception e) {
                Messages.postError("Problem filling " +Shuf.DB_AVAIL_SUB_TYPES);
                Messages.writeStackTrace(e);
            }

        }
    }

    /******************************************************************
     * Summary: Get a list of available subtypes for this type.
     *
     *    Look in the DB to see if which types have this item
     *    set in an attribute.  The point is that if one type is available
     *    to simply show that type.  If more than one type is available,
     *    then show the list in the locator for the user to choose from.
     *****************************************************************/
    public ArrayList getAvailSubTypeList(String objType, String hostFullpath) {
        ArrayList subTypeList = new ArrayList();
        String cmd;
        java.sql.ResultSet rs;

        // Loop through all DB types
        for(int i=0; i < Shuf.OBJTYPE_LIST.length; i++) {
            // Search to see if any items of objType have this
            // attribute set to hostFullpath.
            cmd = "SELECT filename FROM " + Shuf.OBJTYPE_LIST[i]
                + " WHERE " + objType + " = \'" + hostFullpath + "\'";

            try {
                rs = executeQuery(cmd);
                if(rs != null) {
                    if(rs.next())
                        subTypeList.add(Shuf.OBJTYPE_LIST[i]);
                }
            }
            // Don't do anything with errors, just keep going.
            catch(Exception e) {
            }
        }
        if(DebugOutput.isSetFor("subTypeList"))
            Messages.postDebug("subTypeList for " + objType + " and "
                               + hostFullpath + " is:\n" + subTypeList);
        return subTypeList;
    }


    /******************************************************************
     * Summary: Run the VACUUM command on the DB.
     *
     *  The DB needs to be vacuum'ed at least once a week.  It it done
     *  by updateDB each night IF vnmrj is running.  In case vnmrj is
     *  shutdown each afternoon, write the time vacuum'ed so that it
     *  can be checked to see if the DB is in need of vacuum'ing.
     *****************************************************************/

    void vacuumDB() {
        vacuumDB("");
    }

    void vacuumDB(String objTypeInput) {
        String filepath;
        PrintWriter out;
        java.util.Date starttime=null;
        java.util.Date endtime;
        long timems;
        String objType;

        objType = new String(objTypeInput);

        // If the objType is a container type (study, automation, image_dir,
        // lcstudyor record) then we will just vacuum all  of the DB, else if we
        // don't vac a table that is within the container, the DB slows
        // down while being filled.
        if(objType.equals(Shuf.DB_STUDY) || 
                  objType.equals(Shuf.DB_IMAGE_DIR) ||
                  objType.equals(Shuf.DB_COMPUTED_DIR) ||
                  objType.equals(Shuf.DB_AUTODIR) ||
                  objType.equals(Shuf.DB_VNMR_RECORD) ||
                  objType.equals(Shuf.DB_LCSTUDY)) {
            objType = "";  // Force full vacuum
        }

        // Vacuum the DB
        try {
            if(DebugOutput.isSetFor("vacuumDB")) {
                starttime = new java.util.Date();
            }
            String cmd = "VACUUM ANALYZE "+ objType;
            executeUpdate(cmd);
            if(DebugOutput.isSetFor("vacuumDB")) {
                endtime = new java.util.Date();
                timems = endtime.getTime() - starttime.getTime();
                if(objType.equals(""))
                    Messages.postDebug("Time for Vacuum of All (ms): " 
                                       + timems);
                else
                    Messages.postDebug("Time for Vacuum of " + objType 
                                       + " (ms): " + timems);
            }
        }
        catch(Exception e) {
            // Put error in log
            Messages.postDebug("Problem Vaccuming the DB.  This will not cause "
                                             + "any problem with the system.");
            Messages.writeStackTrace(e);
        }

        // Save the date/time that Vacuum was last run
        java.util.Date date = new java.util.Date();
        long longTime = date.getTime();
        // This is time in ms from 1970.  Convert to hours
        longTime /= 3600000;

        try {
            // If dir does not exist or is not writable, just exit.
            UNFile dir = new UNFile(FileUtil.sysdir() + "/pgsql/persistence");
            if(!dir.canWrite()) {
                if(DebugOutput.isSetFor("vacuumDB")) {
                    Messages.postDebug(FileUtil.sysdir()
                                       + "/pgsql/persistence\n"
                                       + "does not exist or is not writable."
                                       + "\nSkipping writing of vacuumDate.");
                    return;
                }
            }

            filepath = FileUtil.savePath(FileUtil.sysdir()
                                         + "/pgsql/persistence/vacuumDate");

            // savePath returns path on Windows in Windows format, change it
            // to unix.
            filepath = UtilB.windowsPathToUnix(filepath);
        }
        catch (Exception e) {
            Messages.postError(" Problem writing "
                        + FileUtil.sysdir() + "/pgsql/persistence/vacuumDate\n"
                        + "        Skipping writing of vacuumDate.");
            Messages.writeStackTrace(e);
            return;
        }

        if(filepath == null) {
            Messages.postDebug(" Problem opening "
                               + FileUtil.sysdir() + "/pgsql/persistence\n"
                               + "        Skipping writing of vacuumDate.");
            return;
        }
        try {
            // Write out the file in plain text
            UNFile file = new UNFile(filepath);
            out = new PrintWriter(new FileWriter(file));
            // Write out a header line
            out.println("Last VacuumDB time in hours since 1970");
            out.println(longTime);
            out.close();
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
        // savePath returns path on Windows in Windows format, change it
        // to unix if necessary.
        filepath = UtilB.windowsPathToUnix(filepath);

        // The file should now exist, be sure it has world write access
        // so that future users can access it.
        String[] cmd = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION, 
                        "chmod 666 " + filepath};
        Process prcs = null;
        try {
            Runtime rt = Runtime.getRuntime();
            prcs = rt.exec(cmd);

            // Be sure this completes before continuing.
            prcs.waitFor();
        }
        catch (Exception e) {
            Messages.postDebug(" Problem with cmd: " + cmd[0] + cmd[1] + cmd[2]
                               + "\nSkipping writing of vacuumDate.");
            Messages.writeStackTrace(e);
            return;
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            if(prcs != null) {
                try {
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
                catch (Exception ex) {
                    Messages.writeStackTrace(ex);
                }
            }
        }
    }


    /******************************************************************
     * Summary: Check date of last DB vacuum to see if it is overdue.
     *
     *  The DB needs to be vacuum'ed frequently.  It it done
     *  by updateDB each night IF vnmrj is running.  In case vnmrj is
     *  shutdown each afternoon, check to see if the DB is in need of
     *  vacuum'ing.
     *****************************************************************/

    public boolean vacuumOverdue() {
        String filepath;
        BufferedReader in;
        String line;
        int    prevTime=0;


        filepath=FileUtil.openPath(FileUtil.sysdir()
                                   + "/pgsql/persistence/vacuumDate");
        // If the file does not exist, report that update is needed
        // It should have been created during an installation in dbsetup
        if(filepath==null)
            return true;

        try {
            UNFile file = new UNFile(filepath);
            in = new BufferedReader(new FileReader(file));

            // The file must start with "Last VacuumDB" or ignore the rest.
            line = in.readLine();
            if(line != null && line.indexOf("VacuumDB") > 0) {
                // The second line should be the number of hours since 1970
                line = in.readLine();
                if(line != null) {
                    // Convert to int
                    prevTime = Integer.parseInt(line);
                }
            }
            in.close();
        } catch (IOException e) {
            // No error output here.
        } catch (Exception e) {
            Messages.writeStackTrace(e);
        }

        // Now prevTime will either be 0, or the value from the file.
        // Get the current time in hr from 1970
        java.util.Date date = new java.util.Date();
        long curTime = date.getTime();
        // This is time in ms from 1970.  Convert to hours
        curTime /= 3600000;

        // Compare the hours
        if(curTime - prevTime > 1)  // 1 means every 1 hours
            return true;  // Needs update
        else
            return false; // Does not need update

    }


    private static boolean errSent=false;

    public boolean getNetworkModeFromDB() {
        String cmd, mode;
        boolean  networkmode = false;
        java.sql.ResultSet rs;


        // If no locator don't try to do this stuff
        if(locatorOff()) {
            return false;
        }

        try {
            cmd = new String("SELECT networkmode  FROM "
                             + Shuf.DB_VERSION);
            rs = executeQuery(cmd);

            if(rs != null && rs.next()) {
                // getString() gives the string representation even if
                // the column type is int or float or date
                mode = rs.getString(1);
                if(mode.equals("true")) {
                    networkmode = true;
                }
            }
        }
        catch (Exception e) {
            Messages.postDebug("Problem getting network mode from DB");
            Messages.writeStackTrace(e);
            return false;
        }

        // If networkmode = false, then check to be sure dbhost is
        // the local host.  If not, give error and try local host
        if(!errSent) {
            if(networkmode == false && !dbHost.equals(localHost)) {
                Messages.postError("The database being specified on "
                           + dbHost + "\n    is set up for non-network mode."
                           + " Either specify PGHOST on " + localHost
                           + "\n    as \'" + localHost
                           + "\' or rebuild the database on " + dbHost
                           + " as a network \n    mode database."
                           + "  Trying to switch to local host as "
                           + "database server.");

                // Only do the test and error output once.
                errSent = true;

                // We cannot allow this machine to access the remote DB if it
                // has networkmode set to false.  So, reset dbHost to be
                // the local host, then disconnect and reconnect to the DB.
                dbHost = localHost;
                closeDBConnection();
                makeDBConnection();

            }
        }


        // The result should be true or false.
        return networkmode;


    }

    public String getDefaultPath(String objType, String fname) {
        String fpath;

        if(fname.startsWith(File.separator) || fname.indexOf(":") == 1 ) {
            // arg contains a full path
            fpath = new String(fname);
        }
        else {
            // Not a full path, get default full path for this objType.
            if(objType.equals(Shuf.DB_VNMR_DATA))
                fpath = FileUtil.openPath("USER" + File.separator + "DATA" +
                                          File.separator + fname);
            else if(objType.equals(Shuf.DB_VNMR_PAR))
                fpath = FileUtil.openPath("USER" + File.separator + "DATA" +
                                          File.separator + fname);
            else if(objType.equals(Shuf.DB_PANELSNCOMPONENTS))
                fpath = FileUtil.openPath("USER" + File.separator +"PANELITEMS"+
                                          File.separator + fname);
            else if(objType.equals(Shuf.DB_SHIMS))
                fpath = FileUtil.openPath("USER" + File.separator + "SHIMS" +
                                          File.separator + fname);
            else if(objType.equals(Shuf.DB_STUDY))
                fpath = FileUtil.openPath("USER" + File.separator + "STUDIES" +
                                          File.separator + fname);
            else if(objType.equals(Shuf.DB_LCSTUDY))
                fpath = FileUtil.openPath("USER" + File.separator + "LCSTUDIES"+
                                          File.separator + fname);
            else if(objType.equals(Shuf.DB_AUTODIR))
                fpath = FileUtil.openPath("USER" + File.separator + "AUTODIR" +
                                          File.separator + fname);
            else if(objType.equals(Shuf.DB_PROTOCOL))
                fpath = FileUtil.openPath("USER" + File.separator +"PROTOCOLS" +
                                          File.separator + fname);
            else if(objType.equals(Shuf.DB_IMAGE_DIR))
                fpath = FileUtil.openPath("USER" + File.separator + "IMAGES" +
                                          File.separator + fname);
            else if(objType.equals(Shuf.DB_COMPUTED_DIR))
                fpath = FileUtil.openPath("USER" + File.separator + "IMAGES" +
                                          File.separator + fname);
            else if(objType.equals(Shuf.DB_IMAGE_FILE))
                fpath = FileUtil.openPath("USER" + File.separator + "IMAGES" +
                                          File.separator + fname);
            else {
                Messages.postError("DB type " + objType + " Not available.");

                return null;
            }
        }
        return fpath;
    }

    // Set the table notify as modified for this objType for all hosts
    // in the table, except for this localHost.  That is, notify everyone
    // else, but no need to notify ourselves.
    public void sendTableModifiedNotification(String objType) {
        String cmd;
        java.sql.ResultSet rs;
        ArrayList hostList = new ArrayList();
        String host, value;

        // First get the list of hosts who have registered to be notified
        // excluding this host.
        try {
            rs = executeQuery("SELECT DISTINCT host FROM notify");
            if(rs == null)
                return;
            
            while(rs.next()) {
                // There is only one column of results, therefore, col 1
                value = rs.getString(1);
                if(value != null) {
                    // Be sure it is not whitespace
                    value = value.trim();
                    if(value.length() > 0) {
                        // Eliminate this host from the list unless we are
                        // in managedb, then go ahead with localhost to
                        // trigger vnmrj.
                        if(!value.equals(localHost) || managedb == true) {
                            hostList.add(value);
                        }
                    }
                }
            }
        }
        catch (Exception e) {
            // If managedb, do not output any errors here.  It is possible
            // for the notify table to not exist yet at this point if
            // vnmrj has not been executed yet.
            if(!managedb)
                Messages.writeStackTrace(e);
        }

        // Set the table for each host in the list
        for(int i=0; i < hostList.size(); i++) {
            host = (String) hostList.get(i);
            cmd = "UPDATE notify SET " + objType + " = \'modified\' WHERE "
                + "host = \'" + host + "\'";
            try {
                executeUpdate(cmd);
            }
            catch (Exception e) {
            }
        }
    }

    // Take a string value then call the acqual calculation
    public static Double convertFreqToTesla(String h1freq) {
        if(h1freq == null)
            h1freq = "0";
        Double field = new Double(h1freq);
        return convertFreqToTesla(field.doubleValue());
    }
    
    // Calculate the Tesla field for this proton frequency.
    // Round to one decimal place.  If the result is near one of the
    // standard fields, then force it to that value
    public static Double convertFreqToTesla(double h1freq) {
        double field = h1freq / 42.58;
        // Round to one decimal place
        long tmp = Math.round(field * 10.0);
        field = (double)tmp/10.0;

        if(field > 4.5 && field < 4.9)
            field = 4.7;
        else if(field > 6.8  && field <  7.2)
            field = 7.0;
        else if(field > 9.2 && field < 9.6)
            field = 9.4;
        else if(field > 11.4 && field < 12.0)
            field = 11.7;
        else if(field > 13.8 && field < 14.4)
            field = 14.1;
        else if(field > 16.1 && field < 16.7)
            field = 16.4;
        else if(field > 18.5 && field < 19.1)
            field = 18.8;


        return field;
    }


} // class FillDBManager



/********************************************************** <pre>
 * Summary: Information used while building and executing sql commands.
 *
 * This is designed as a separate class, so that each caller
 * working with sql commands, will have his own copy of this info.
 * Otherwise, different threads can end up adding things to the DB
 * at the same time and step on each other.  That is, this is to
 * make the code re-entrant.
 </pre> **********************************************************/
class DBCommunInfo {
    public StringBuffer addCmd;
    public StringBuffer cmdValueList;
    public String hostFullpath;
    public boolean firstAttr;
    public boolean keySet;
    public int numFilesAdded;
    public int numFilesRemoved;
    public ArrayList attrList;

    DBCommunInfo() {
        numFilesAdded = 0;
        numFilesRemoved = 0;
        attrList = new ArrayList();
    }

    public String toString() {
        String str = "addCmd = " + addCmd + " cmdValueList = " + cmdValueList +
                     " hostFullpath = " + hostFullpath + " firstAttr = " +
                     firstAttr + "\nkeySet = " + keySet + " numFilesAdded = " +
                     numFilesAdded + " numFilesRemoved = " + numFilesRemoved +
                     "\nattrList = " + attrList;
        return str;
    }

}

class UpdateSymLinks extends Thread {
    String dirToSearch;
    SymLinkMap symLinkMapLocal;

    public UpdateSymLinks(String directory, SymLinkMap symLinkMap) {
        dirToSearch = directory;
        symLinkMapLocal = symLinkMap;
        setPriority(Thread.MIN_PRIORITY);
    }

    public void run() {
        // Bail out if Windows
        if(UtilB.OSNAME.startsWith("Windows"))
            return;

        symLinkMapLocal.checkForSymLinks(dirToSearch);
    }

}
