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
import java.awt.Component;
import java.awt.Dimension;
import java.awt.IllegalComponentStateException;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.event.*;
import javax.swing.Timer;

import vnmr.templates.*;
import vnmr.bo.*;
import vnmr.ui.shuf.*;
import java.io.*;
import java.util.*;

/**<pre>
 * utility class for directory path location and generation
 * @author      Dean Sindorf
 *
 *  <u>search option static string arguments</u>
 *  <b>Base Directory Search Options</b>
 *
 *     variable     description
 *
 *     SYSTEM       search only the system directory specified
 *                  by the environment variable $sysdir
 *     USER         search only the user directory specified
 *                  by the environment variable $userdir
 *
 *  <b>Sub Directory Search Options</b>
 *
 *     ---- Symbolic directories -----
 *
 *     LAYOUT           templates/layout
 *     VNMRJ            templates/vnmrj
 *     ANNOTATION       templates/vnmrj/annotation
 *     TEXTBOX		templates/vnmrj/textbox
 *     INTERFACE        templates/vnmrj/interface
 *     PANELITEMS       templates/vnmrj/panelitems
 *     PROPERTIES       templates/vnmrj/properties
 *     PROTOCOLS        templates/vnmrj/protocols
 *     CHOICEFILES      templates/vnmrj/choicefiles
 *     PERSISTENCE      persistence
 *     IBROWSER         ibrowser
 *     IBMATH           ibrowser/math/expressions/bin
 *     LIB              lib
 *     IMAGES           lib/images
 *     DATA             data
 *     STUDIES          data/studies
 *     AUTOMATION       data/autodir
 *     ADMIN            adm
 *     USRS             adm/users
 *     USRPROF          adm/users/profiles/user
 *     SYSPROF          adm/users/profiles/system
 *     PROFILES         adm/users/profiles
 *     PERSISTENCE      persistence
 *     LOCATOR          shuffler
 *     ICONLIB          iconlib
 *     HELP             help
 *     MACLIB           maclib
 *     MENULIB          menulib
 *     PARLIB           parlib
 *     SEQLIB           seqlib
 *     PSGLIB           psglib
 *     SPINCAD          spincad
 *     SHAPELIB         shapelib
 *     SHIMS            shims
 *     TABLIB           tablib
 *     TRASH            trash
 *     IMAGING          imaging
 *     DICOMCONF        dicom/conf
 *     RUDY_IF          templates/vnmrj/interface/rudy
 *
 *</pre>
 */
public class FileUtil
{
    //++++++ directory-file code strings ++++++++++++

    static public String SYS_VNMR               = "/vnmr";
    
    static public String VNMRSYS               = "vnmrsys";
    static public String VNMR                  = "vnmr";

    //++++++ Java -D command line option keys +++++++

    static public String SYSTEM                 = "SYSTEM";
    static public String USER                   = "USER";
    static public String DEFAULT                = "DEFAULT";
    static public String DFLTDIR                = "default";

    static public boolean print_errors=false;
    static private boolean initialized=false;
    static Hashtable<String,String> vnmr_dirs = new Hashtable<String,String>();

    static String SysDir=null;
    static String UsrDir=null;
    static String AppDir=null;
    static String Language= null;           // -Dlanquage=$LANQUAGE
    static String Country= null;            // -Dcountry=$COUNTRY
    static String Variant= null;            // -Dcountry=$COUNTRY_variant
    static String AppType=null;
    static public String English=Locale.ENGLISH.getLanguage();
    static public String US=Locale.US.getCountry();
    
    static private  boolean  openAll=true;
    static ArrayList<String> appDirs=null;
    static ArrayList<String> appDirLabels=null;
    static ArrayList<String> appTypes=null;
    static boolean debug=true;
    static boolean appexts=true;
    static private  Timer  timer = null;
    static private  timerObj  timeObj = null;
    static private  int  timeCount;
    static private  boolean  timeResult;
    static private  boolean  hasUserRespondedToPopup;
    static private  Process timeProc = null;

    static private final String NEWLINE = System.getProperty("line.separator");
    static public String separator="/";

    private static void init_dirs() {
        if(initialized)
            return;
        separator="/";
        String dir;
        dir=System.getProperty("appexts");
        if(dir !=null && dir.equals("no"))
            appexts=false;     
        dir=System.getProperty("language");
        if(dir!=null && dir.length()==2) // only ISO 639-1 codes allowed
            Language=dir;
        else
            Language=Locale.getDefault().getLanguage();
        dir=System.getProperty("country");
        if(dir!=null && dir.length()==2) // -Dcountry=$COUNTRY
            Country=dir;
        else
            Country=Locale.getDefault().getCountry();
        if(Country!=null && Country.length()!=2)
            Country=null;
        dir=System.getProperty("variant");
        if(dir!=null && dir.length()>0) // -Dvariant=$VARIANT
            Variant=dir;
        
        if(Country==null)
            Locale.setDefault(new Locale(Language));
        else if(Variant==null)
            Locale.setDefault(new Locale(Language,Country));
        else
            Locale.setDefault(new Locale(Language,Country,Variant));
                    
        dir=System.getProperty("sysdir");
        SysDir=(dir==null)?SYS_VNMR:dir;
            
        String          sdir;
        dir=System.getProperty("user.home");
        sdir=System.getProperty("userdir");
        
        if(sdir==null || sdir.length()==0)
            UsrDir = new StringBuffer().append( dir).append(FileUtil.separator).
                         append(VNMRSYS).toString();
        else
            UsrDir = sdir;

        // Directories under ~/vnmrsys or /vnmr

        vnmr_dirs.put("TEMPLATES",              "templates");
        vnmr_dirs.put("LAYOUT",                 "templates"+FileUtil.separator+"layout");
        vnmr_dirs.put("TOOLPANELS",             "templates"+FileUtil.separator+"layout"+FileUtil.separator+"toolPanels");
        vnmr_dirs.put("VNMRJ",                  "templates"+FileUtil.separator+"vnmrj");
        vnmr_dirs.put("PANELITEMS",             "templates"+FileUtil.separator+"vnmrj"+
                                                    FileUtil.separator+"panelitems");
        vnmr_dirs.put("PROPERTIES",             "templates"+FileUtil.separator+"vnmrj"+
                                                    FileUtil.separator+"properties");
        vnmr_dirs.put("INTERFACE",              "templates"+FileUtil.separator+"vnmrj"+
                                                    FileUtil.separator+"interface");
        vnmr_dirs.put("ANNOTATION",             "templates"+FileUtil.separator+"vnmrj"+
                                                    FileUtil.separator+"annotation");
        vnmr_dirs.put("TEXTBOX",		"templates"+FileUtil.separator+"vnmrj"+
                                                    FileUtil.separator+"textbox");
        vnmr_dirs.put("RUDY_IF",                "templates"+FileUtil.separator+"vnmrj"+
                                                    FileUtil.separator+"interface"+
                                                    FileUtil.separator+"rudy");
        vnmr_dirs.put("CHOICEFILES",            "templates"+FileUtil.separator+"vnmrj"+
                                                    FileUtil.separator+"choicefiles");
        vnmr_dirs.put("PROTOCOLS",              "templates"+FileUtil.separator+"vnmrj"+
                                                    FileUtil.separator+"protocols");
        vnmr_dirs.put("ADMIN",                  "adm");
        vnmr_dirs.put("USRS",                   "adm"+FileUtil.separator+"users");
        vnmr_dirs.put("USRPROF",                "adm"+FileUtil.separator+"users"+
                                                    FileUtil.separator+"profiles"+
                                                    FileUtil.separator+"user");
        vnmr_dirs.put("SYSPROF",                "adm"+FileUtil.separator+"users"+
                                                    FileUtil.separator+"profiles"+
                                                    FileUtil.separator+"system");
        vnmr_dirs.put("PROFILES",               "adm"+FileUtil.separator+"users"+
                                                    FileUtil.separator+"profiles");
        vnmr_dirs.put("GRPS",                   "adm"+FileUtil.separator+"groups");
        vnmr_dirs.put("PERSISTENCE",            "persistence");
        vnmr_dirs.put("ICONLIB",                "iconlib");
        vnmr_dirs.put("MOLLIB",                 "mollib");
        vnmr_dirs.put("DATA",                   "data");
        vnmr_dirs.put("STUDIES",                "data"+FileUtil.separator+"studies");
        vnmr_dirs.put("LCSTUDIES",              "data");
        vnmr_dirs.put(Shuf.DB_AUTODIR,          "data"+FileUtil.separator+"autodir");
        vnmr_dirs.put("IBROWSER",               "ibrowser");
        vnmr_dirs.put("IBMATH",                 "ibrowser"+FileUtil.separator+"math"+
                                                    FileUtil.separator+"expressions"+
                                                    FileUtil.separator+"bin");
        vnmr_dirs.put("HELP",                   "help");
        vnmr_dirs.put("LOCATOR",                "shuffler");
        vnmr_dirs.put("MACLIB",                 "maclib");
        vnmr_dirs.put("MANUAL",                 "manual");
        vnmr_dirs.put("MENULIB",                "menulib");
        vnmr_dirs.put("PARLIB",                 "parlib");
        vnmr_dirs.put("SEQLIB",                 "seqlib");
        vnmr_dirs.put("PSGLIB",                 "psglib");
        vnmr_dirs.put("SPINCAD",                "spincad");
        vnmr_dirs.put("SHAPELIB",               "shapelib");
        vnmr_dirs.put("SHIMS",                  "shims");
        vnmr_dirs.put("TABLIB",                 "tablib");
        vnmr_dirs.put("PART11",                 "p11");
        vnmr_dirs.put("TRASH",                  "trash");
        vnmr_dirs.put("ACQQUEUE",               "acqqueue");
        vnmr_dirs.put("LANGUAGES",              "languages");
        vnmr_dirs.put("IMAGING",                "imaging");
        vnmr_dirs.put("DICOMCONF",              "dicom"+FileUtil.separator+"conf");

        appDirs=new ArrayList<String>();
        appDirLabels=new ArrayList<String>();
	// appDirs will be set by readuserListFile in User.
	// But it needs to set to SysDir temporarily here
        // so openPath will work when User opens userList or profiles.
        appDirs.add(SysDir);
        appDirLabels.add("System");

        appTypes=new ArrayList<String>();

        initialized=true;
    }

    //~~~~~~~~~~~~~~~~ file I/O utilities ~~~~~~~~~~~~~~~~~~~~~~~~~~
    //----------------------------------------------------------------
    /** Replace symbol and /vnmr portions of an appdir path. */
    //----------------------------------------------------------------
    public static String getAppdirPath(String str) {
		String systok = null;
		String path = str;
		if (str.equalsIgnoreCase("USERDIR")) {
			path = FileUtil.usrdir();
		} else if (str.startsWith("USERDIR")) {
		 	path = FileUtil.usrdir() + str.substring("USERDIR".length());
		} else if (str.startsWith("SYSTEMDIR"))
			systok = "SYSTEMDIR";
		else if (str.startsWith("/vnmr"))
			systok = "/vnmr";
		if (systok != null) {
			String f = str.substring(systok.length());
			path = FileUtil.sysdir() + f;
		}
		return path;
	}

    //----------------------------------------------------------------
    /** Set appDirs from a list of directories passed from User profile. */
    //----------------------------------------------------------------
    public static void setAppDirs(ArrayList<String> a, ArrayList<String> labels) {    
        appDirs.clear();
        appDirLabels.clear();
        String dir=null;
        String label=null;
        for(int j=0;j<a.size();j++){
            dir=a.get(j);
            label=labels.get(j);
            if(Language !=null){
                String ldir=dir+"/"+Language;
                if(canRead(ldir)) {
                    appDirs.add(ldir);  
                    appDirLabels.add(label);
                }               
            }
            // if(canRead(dir)) {
               appDirs.add(dir);
               appDirLabels.add(label);
            // }
        }
     }

    //----------------------------------------------------------------
    /** Return list of file search base directories. */
    //----------------------------------------------------------------
    public static ArrayList getAppDirs() {
        init_dirs();
        return appDirs;
    }

    //----------------------------------------------------------------
    /** Return list of file search base directory labels. */
    //----------------------------------------------------------------
    public static ArrayList getAppDirLabels() {
        init_dirs();
        return appDirLabels;
    }
 
    //----------------------------------------------------------------
    /** Return list of file search extensions. */
    //----------------------------------------------------------------
    public static ArrayList getAppTypes() {
        return appTypes;
    }

    //----------------------------------------------------------------
    /** Set appTypes from a list of extension names passed from User.java. */
    //----------------------------------------------------------------
    public static void setAppTypes(ArrayList<String>a) {    
        appTypes=a;
    }

    //----------------------------------------------------------------
    /** Set language preference. */
    //----------------------------------------------------------------
    public static void setLanguage(String s) {
        Language=s;
        Locale.setDefault(new Locale(s));
    }

    public static void setLanguage(String s, String c) {
	if(c == null || c.length() < 1) setLanguage(s);
	else {
          Language=s;
	  Country =c;
	  Locale.setDefault(new Locale(s, c));
	}
    }

    //----------------------------------------------------------------
    /** Set language preference. */
    //----------------------------------------------------------------
    public static void setOpenAll(boolean all) {
        openAll=all;
    }

    //----------------------------------------------------------------
    /** Get language preference. */
    //----------------------------------------------------------------
    public static String getLanguage() {
        return Language;
    }

    public static String getCountry() {
        return Country;
    }

    //----------------------------------------------------------------
    /** Set system directory. */
    //----------------------------------------------------------------
    public static void setSysdir(String s) {
        SysDir=s;
    }

    //----------------------------------------------------------------
    /** Set user's home directory. */
    //----------------------------------------------------------------
    public static void setUsrdir(String s) {
        UsrDir=s;
    }

    //----------------------------------------------------------------
    /** Return system directory path. */
    //----------------------------------------------------------------
    public static String sysdir() {
        init_dirs();
        return SysDir;
    }

    //----------------------------------------------------------------
    /** Return default user vnmrsys directory path. */
    //----------------------------------------------------------------
    public static String usrdir() {
        init_dirs();
        return UsrDir;
    }

    //----------------------------------------------------------------
    /** Return user sub-directory for file operation. */
    //----------------------------------------------------------------
    public static String userDir(User user, String type) {
        String dir = user.getVnmrsysDir();
        init_dirs();
        if(vnmr_dirs.containsKey(type))
            type=(String)vnmr_dirs.get(type);
        return  (new StringBuffer().append(dir).append(FileUtil.separator).
                 append(type).toString());
    }

    public static String getSymbolicPath(String type)
    {
        String strPath = "";
        if (type != null)
            strPath = (String)vnmr_dirs.get(type);
        return strPath;
    }

    //----------------------------------------------------------------
    /** Test if file exists and is readable. */
    //----------------------------------------------------------------
    public static boolean canRead(String path) {
        if(path==null || path.length()==0)
            return false;
        String dir=path;
        if (UtilB.iswindows())
            dir = UtilB.unixPathToWindows(dir);
        File file=new File(dir);
        if(file.exists() && file.canRead())
            return true;
        return false;
    }
    
    //----------------------------------------------------------------
    /** <pre>Return first XML file in path that exists and matches name. 
    * <u>Search order</u>
    *    ../NAME[<.type>]<.language>_<country>_<variant>.xml
    *    ../NAME[<.type>]<.language>_<country>.xml
    *    ../NAME[<.type>]<.language>.xml
    *    ../NAME[<.type>].xml
    *  </pre>
    */
    //----------------------------------------------------------------
    public static String XmlPath(String f, String atype) {
        if(!f.contains(".xml"))
            return f;
               
        if(!openAll){
            if(canRead(f))
                return f;
            return null;            

        }
        String dir=pathBase(f);
        String name=pathName(f);
        int i=name.indexOf(".");
        name=name.substring(0,i);
        
        String path=dir+name;
        
        String lpath=null;
        String type="";
        if(atype !=null)
            type="."+atype;
        
        if(Country !=null && Variant !=null){
            lpath=path+type+"."+Language+"_"+Country+"_"+Variant+".xml";
            if(canRead(lpath))
                return lpath;
        }
        if(Country !=null){
            lpath=path+type+"."+Language+"_"+Country+".xml";
            if(canRead(lpath))
                return lpath;
        }
        if(Language !=null){
            lpath=path+type+"."+Language+".xml";
            if(canRead(lpath))
                return lpath;
        }
        lpath=path+type+".xml";
        if(canRead(lpath))
            return lpath;           
        else
            return null;
    }
    
    //----------------------------------------------------------------
    /** Strip UTF-8 BOM from input stream (e.g. xml files). */
    //----------------------------------------------------------------
    static public InputStream skipUtf8Bom(InputStream stream) {
        stream.mark(3);
        try {
        if (stream.available() < 3
                || stream.read() != 0xEF
                || stream.read() != 0xBB
                || stream.read() != 0xBF)
            {
                // No UTF-8 BOM -- use entire stream
                stream.reset();
            }
        } catch (IOException ioe) {
        } 
        return stream;
    } 
 
    //----------------------------------------------------------------
    /** return appropriate InputStreamReader based on file BOM. */
    //----------------------------------------------------------------
    static public InputStreamReader getUTFReader(File file) {
        try {
            InputStream stream = new FileInputStream(file);
            if(stream.available() < 3){
                return new InputStreamReader(stream);
            }
            int byte1 = stream.read();
            int byte2 = stream.read();
            if((byte1 == 0xFE && byte2 == 0xFF) || (byte1 == 0xFF && byte2 == 0xFE)) {
                stream.close();
                stream = new FileInputStream(file);
                return new InputStreamReader(stream, "UTF-16");
            }
            int byte3 = stream.read();
            if(byte1 == 0xEF && byte2 == 0xBB && byte3 == 0xBF) {
                return new InputStreamReader(stream, "UTF-8");
            } else {
                stream.close();
                stream = new FileInputStream(file);
                return new InputStreamReader(stream,"UTF-8");
            }
        } catch(Exception e) {
            return null;
        }
    } 

    //----------------------------------------------------------------
    /** Return expanded path. */
    //----------------------------------------------------------------
    public static String expandPath(String f) {
        String type=null;
        String dir=null;
        init_dirs();
        
        if(f==null || f.length()==0)
            return "";
        StringBuffer s=new StringBuffer().append(f);
        //if(f.startsWith(File.separator) || f.startsWith("/"))
        //    f=f.substring(1);

        StringTokenizer tok=new StringTokenizer(f,File.separator);
        while(tok.hasMoreTokens()){
            type=(String)tok.nextToken();
            if(vnmr_dirs.containsKey(type)){
                dir=(String)vnmr_dirs.get(type);
                String r=s.toString().replace(type, dir);
                s=new StringBuffer().append(r);
            }        
        }
        return s.toString();
    }

    //----------------------------------------------------------------
    /** Return vnmr directory path. */
    //----------------------------------------------------------------
    public static String fullPath(String base, String f) {
        String type=null;
        String dir=null;
        init_dirs();

        if(f==null || f.length()==0)
            return base;
        if(f.startsWith(File.separator) || f.startsWith("/"))
            f=f.substring(1);

        StringTokenizer tok=new StringTokenizer(f,File.separator+FileUtil.separator);
        if(tok.hasMoreTokens()){
            type=(String)tok.nextToken();
            if(vnmr_dirs.containsKey(type))
            {
                dir=(String)vnmr_dirs.get(type);
                if(dir!=null)
                    f=new StringBuffer().append(dir).append(f.substring(type.length())).toString();
            }
            else
            {
                tok=new StringTokenizer(f,"/");
                if(tok.hasMoreTokens()){
                    type=(String)tok.nextToken();
                    if(vnmr_dirs.containsKey(type))
                        dir=(String)vnmr_dirs.get(type);
                    if(dir!=null)
                        f=new StringBuffer().append(dir).append(f.substring(type.length())).toString();
                }
            }
        }
        return (new StringBuffer().append(base).append(FileUtil.separator).append(f).toString());
    }

    //----------------------------------------------------------------
    /** <pre> Return general path for save operation.
     *  returns absolute "path" if specified directory and file can be
     *  created, else returns null.
     *
     *  note: A partial or full directory path. The parent directory
     *        is created if the fullpath can be generated or not.
     *
     *  <u>examples of usage</u>
     *     FileUtil.savePath("/export/home/vnmr1/vjtest/test.xml");
     *     FileUtil.savePath("~/vjtest/template/xml/test.xml");
     *     FileUtil.savePath("PANELITEMS/test.xml");
     *     FileUtil.savePath("USER/PERSISTENCE/backup1");
     *  </pre>
     */
    //----------------------------------------------------------------
    /** Return path for save absolute(/..) or user (~/.). */
    //----------------------------------------------------------------
    public static String savePath(String f, boolean verbose)  {
        String path=null;

        init_dirs();
        if (f == null)
            return null;

        // This would be a full path for unix
        if (f.startsWith(File.separator) || f.startsWith("/"))
            path=f;
        // This would a full path for Windows
        else if (UtilB.OSNAME.startsWith("Windows") &&
                 f.indexOf(':') == 1) {
            path=f;
        }
        else if (f.startsWith("~/")) {
            path=new StringBuffer().append(System.getProperty("user.home")).
                      append(FileUtil.separator).append(f.substring(2)).toString();
        }
        else if(f.startsWith(SYSTEM)){
            f=f.substring(SYSTEM.length());
            path=fullPath(sysdir(),f);
        }
        else if(f.startsWith(USER)){
            f=f.substring(USER.length());
            path=fullPath(usrdir(),f);
        }
        else{
            path=fullPath(usrdir(),f);
        }
        if(path!=null) {
            // Convert path to Windows type if necessary
            path = UtilB.unixPathToWindows(path);

            boolean good = true;
            File file=new File(path);
            if (!file.exists()) {
                 String pdir = file.getParent();
                 if (pdir != null) {
                     File fp = new File(pdir);
                     if (!fp.exists()) {
                          if (!fp.mkdirs())
                              good = false;
                     }
                     else if (!fp.canWrite())
                         good = false;
                 }
                 else
                     good = false;
            }
            else if (!file.canWrite()) {
                  good = false;
            }
/*
            File parent=new File(file.getParent());
            parent.mkdirs();
            if(parent.exists()){
                if(!file.exists() || file.canWrite())
                    return path;
            }
*/
            if (!good) {
                if (verbose)
                    Messages.postError(new StringBuffer().append(
                        "savePath error in writing\n    " ).append(path ).
                        append( "\n    Check unix permissions of this " ).
                        append("directory and any existing file.").toString());
                return null;
            }
            return path;
        }
        if (verbose)
            Messages.postError(new StringBuffer().append(
                        "FileUtil.savePath error in writing " ).
                        append( f).toString());
        return null;
    }

    public static String savePath(String f)  {
        return savePath(f, true);
    }

   //----------------------------------------------------------------
   /** <pre> Return default path for open operation.
     *  returns absolute "path" if file "f" is found, else returns null.
     *  <u>examples of usage</u>
     *     FileUtil.openPath("/export/home/vnmr1/vjtest/template/xml/test.xml");
     *     FileUtil.openPath("~/vjtest/template/xml/test.xml");
     *     FileUtil.openPath("PANELITEMS/test.xml");
     *     FileUtil.openPath("USER/PANELITEMS/test.xml");
     *  </pre>
     */
    public static String openPath(String f) {
        /*
         * NB: Should be kept compatible with openPaths().
         * Could rewrite this to call openPaths(f, true, false).
         */
        String path=null;
        String dir=null;        
        File file;
        int i,j;

        if (f == null)
            return null;

        init_dirs();
        // This would be a full path for unix
        if (f.startsWith(File.separator) || f.startsWith("/"))
            path=f;
        else if (f.startsWith("~/"))
            path=new StringBuffer().append(System.getProperty("user.home")).
                      append(FileUtil.separator).append(f.substring(2)).toString();
        else if(f.startsWith(SYSTEM)){
            f=f.substring(SYSTEM.length());
            path=fullPath(sysdir(),f);
        }
        else if(f.startsWith(USER)){
            f=f.substring(USER.length());
            path=fullPath(usrdir(),f);
        }
        else{
            if(appexts && f.contains(".xml")){
                // check for name extended files in all directories
                for(i=0;i<appTypes.size();i++){
                    AppType=appTypes.get(i);
                    for(j=0;j<appDirs.size();j++){
                        dir=fullPath((String)appDirs.get(j),f);
                        dir=XmlPath(dir,AppType);
                        if(canRead(dir))
                        	return dir;
                    }
                }
                // check for non type name-extended files in all directories
                for(j=0;j<appDirs.size();j++){
                    dir=fullPath((String)appDirs.get(j),f);
                    dir=XmlPath(dir,null);
                    if(canRead(dir))
                    	return dir;
                }
            }
            else{           
                 for(i=0;i<appDirs.size();i++){
                    dir=fullPath((String)appDirs.get(i),f);
                    if(dir==null)
                        continue;
                    if(canRead(dir))
                        return dir;
                 }
            }
        }
        if(canRead(path))
            return path;
        return null;
    }

    /**
     * Find the paths specified by the string "f" that exist, and
     * return the expanded full paths.  More than one path will
     * be returned only if "f" specifies a relative path.
     * @param f The abstract file path specification.
     * @param firstOnly If true, return only the first path found.
     * @return The array of full paths; zero length if no valid
     * path is found.
     */
    public static String[] openPaths(String f,
                                     boolean firstOnly, boolean dirsOnly) {
        String path = null;
        File file;
        ArrayList list = null;  // Only create it if needed
        String[] rtn = null;    // Create it when size is known

        if (f == null)
            return null;
        init_dirs();
        if (f.startsWith(File.separator) || f.startsWith("/") ||
                (UtilB.iswindows() && f.indexOf(':') == 1)) {
            path=f;
        }  
        else if (f.startsWith("~/")) {
            path=new StringBuffer().append(System.getProperty("user.home")).
                      append(FileUtil.separator).append(f.substring(2)).toString();
        } else if (f.startsWith(SYSTEM)) {
            f=f.substring(SYSTEM.length());
            path=fullPath(sysdir(),f);
        } else if (f.startsWith(USER)) {
            f=f.substring(USER.length());
            path=fullPath(usrdir(),f);
        } else {
            for(int i=0;i<appDirs.size();i++){
                String dir=fullPath((String)appDirs.get(i),f);
                file=new File(dir);
                if(file.exists() && file.canRead()) {
                    if (firstOnly) {
                        path = dir;
                        break;
                    } else {
                        if (list == null) {
                            list = new ArrayList();
                        }
                        if (!dirsOnly || file.isDirectory()) {
                            list.add(dir);
                        }
                    }
                }
            }
        }
        if (path != null) {
            // Return one path
            file = new File(path);
            if (file.exists() && file.canRead()
                && (!dirsOnly || file.isDirectory()))
            {
                rtn = new String[1];
                rtn[0] = path;
                return rtn;
            }
        } else if (list != null) {
            // Return list of paths
            rtn = (String[])list.toArray(new String[0]);
            return rtn;
        }
        // No paths found
        return new String[0];
    }

    //----------------------------------------------------------------
    /** Return vnmr directory path. */
    //----------------------------------------------------------------
    public static String vnmrDir(String base, String type) {
        String dir=fullPath(base,type);
        if(dir!=null){
            File file=new File(dir);
            if(file.exists() && file.isDirectory())
                return dir;
        }
        return null;
    }

    //----------------------------------------------------------------
    /** Return vnmr directory path. */
    //----------------------------------------------------------------
    public static String vnmrDir(String f) {
        String dir=openPath(f);
        if(dir!=null){
            File file=new File(dir);
            if(file.exists() && file.isDirectory())
                return dir;
        }
        return null;
    }

    /**
     * Get all the paths to existing directories that fit the
     * given spec.
     */
    public static String[] getAllVnmrDirs(String spec) {
        if (spec == null) {
            return new String[0];
        }
        return openPaths(spec, false, true);
    }

    //----------------------------------------------------------------
    /** Test if file exists. */
    //----------------------------------------------------------------
    public static boolean fileExists(String file) {
        return openPath(file)==null?false:true;
    }

    //----------------------------------------------------------------
    /** Return filename sans extension. */
    //----------------------------------------------------------------
    public static String fileName(String name) {
        String fname = pathName(name);
        int index = name.indexOf(".");
        if(index > 0)
            fname = fname.substring(0, index);
        return fname;
    }

    //----------------------------------------------------------------
    /** Return filename extension. */
    //----------------------------------------------------------------
    public static String fileExt(String name) {
        String ext = null;
        String fname = pathName(name);
        int index = fname.indexOf(".");
        if(index > 0)
            ext = fname.substring(index, fname.length());
        return ext;
    }

    //----------------------------------------------------------------
    /** Return language extension. */
    //----------------------------------------------------------------
    public static String langExt(String name) {
        String ext = fileExt(name);
        if(ext==null)
            return ext;
        int index = ext.lastIndexOf('.');
        ext = ext.substring(index,ext.length());
        if(ext.length()==3 && !ext.equals(".lc"))
            return ext;
        return null;
    }

    //----------------------------------------------------------------
    /** Return the last directory or filenname in a path. */
    //----------------------------------------------------------------
    public static String pathName(String path) {
        int i=path.lastIndexOf("/");
        int j=path.lastIndexOf("\\");
        int k=i>j?i:j;
        if(k<0)
            return path;
        return path.substring(k+1,path.length());
    }

    //----------------------------------------------------------------
    /** Return dir part of path. */
    //----------------------------------------------------------------
    public static String pathBase(String path) {
        int i=path.lastIndexOf("/");
        int j=path.lastIndexOf("\\");
        int k=i>j?i:j;
        if(k<0)
            return path;
        return path.substring(0,k+1);
    }

    //----------------------------------------------------------------
    /** Return true if reference found. */
    //----------------------------------------------------------------
    public static boolean findReference(String dir, String fname) {
        debug=false;
        boolean found=false;
        if(getLayoutPath(dir,fname)!=null)
            found=true;
        debug=true;
        return found;
    }
    
    //----------------------------------------------------------------
    /** Return the first layout dir containing a file. */
    //----------------------------------------------------------------
    public static String getLayoutPath(String dir, String fname) {

        String path=openPath(new StringBuffer().append("LAYOUT").append(
                                FileUtil.separator).append(dir).append(
                                FileUtil.separator).append(fname).toString());

        if(path==null){
            String ldir=openPath(new StringBuffer().append("LAYOUT").append(
                                    FileUtil.separator).append(dir).append(
                                    FileUtil.separator).append("DEFAULT").toString());
            String dname="default";
            if(ldir !=null)
                dname=readDefaultFile(ldir);
            path=openPath(new StringBuffer().append("LAYOUT").append(
                            FileUtil.separator).append(dname).append(
                            FileUtil.separator).append(fname).toString());
        }
        if(path==null)
             path=openPath(new StringBuffer().append("LAYOUT").append(
                            FileUtil.separator).append("default").append(
                            FileUtil.separator).append(fname).toString());
        if(debug && DebugOutput.isSetFor("layout"))
            Messages.postDebug(new StringBuffer().append("path for ").append(dir).
                               append(FileUtil.separator).append(fname).append(
                                " = ").append(path).toString());
        return path;
    }

    //----------------------------------------------------------------
    /** Return an appropriale default file name. */
    //----------------------------------------------------------------
    public static void setDefaultName(String dir, String name) {

        // find first dir in appdirs (e.g. ../s2pul)

        String ldir=savePath(new StringBuffer().append("LAYOUT").append(
                                FileUtil.separator).append(dir).append(
                                FileUtil.separator).append(DEFAULT).toString());
        if(ldir!=null){
           // check for DEFAULT file  (e.g ../s2pul/DEFAULT)
            writeDefaultFile(ldir,name);
        }
        if(DebugOutput.isSetFor("layout"))
            Messages.postDebug(new StringBuffer().append("default for ").append(dir).
                               append(" = ").append(name).toString());
    }

    //----------------------------------------------------------------
    /** Return the path specified by a DEFAULT file. */
    //----------------------------------------------------------------
        public static String writeDefaultFile(String path, String name)
        {
                try {
                        BufferedWriter out = new BufferedWriter(new FileWriter(path));
            String str=new StringBuffer().append("set default ").append(name).toString();
            out.write(str,0,str.length());
            out.newLine();
            out.flush();
            out.close();
                }
                catch(IOException ioe) {
                        Messages.postError(new StringBuffer().append("error writing " ).
                               append( path).toString());
                        Messages.writeStackTrace(ioe);
                }
                return name;
        }

    //----------------------------------------------------------------
    /** Return true if default file exists. */
    //----------------------------------------------------------------
    public static boolean hasDefaultFile(String dir) {
        String dfile=openPath(new StringBuffer().append("LAYOUT").append(
                                FileUtil.separator).append(dir).append(
                                FileUtil.separator).append(DEFAULT).toString());
        if(dfile==null)
            return false;
        return true;
    }

    //----------------------------------------------------------------
    /** Return true if user's default file exists. */
    //----------------------------------------------------------------
    public static boolean hasUserDefaultFile(String dir) {
        String dfile=openPath(new StringBuffer().append("USER").append(
                                FileUtil.separator).append("LAYOUT").append(
                                FileUtil.separator).append(dir).append(
                                FileUtil.separator).append(DEFAULT).toString());
        if(dfile==null)
            return false;
        return true;
    }

    //----------------------------------------------------------------
    /** If layout directory has no files, delete it and return true */
    //----------------------------------------------------------------
    public static boolean deleteEmptyLayoutDir(String dir) {
        String ldir=openPath(new StringBuffer().append("USER").append(
                                FileUtil.separator).append("LAYOUT").append(
                                FileUtil.separator).append(dir).toString());
        if(ldir==null)
            return false;
        File fp=new File(ldir);
        String files[]=fp.list();
        if(files.length!=0)
            return false;
        fp.delete();
        return true;
    }

    //----------------------------------------------------------------
    /** Return true if layout directory has no files */
    //----------------------------------------------------------------
    public static boolean isEmptyLayoutDir(String dir) {
        String ldir=openPath(new StringBuffer().append("USER").append(
                                FileUtil.separator).append("LAYOUT").append(
                                FileUtil.separator).append(dir).toString());
        if(ldir==null)
            return false;
        File fp=new File(ldir);
        String files[]=fp.list();
        return files.length>0?false:true;
    }

    //----------------------------------------------------------------
    /** Remove user default file. If no other files remove directory */
    //----------------------------------------------------------------
    public static boolean removeUserDefaultFile(String dir) {
        String dfile=openPath(new StringBuffer().append("USER").append(
                                FileUtil.separator).append("LAYOUT").append(
                                FileUtil.separator).append(dir).append(
                                FileUtil.separator).append(DEFAULT).toString());
        if(dfile==null)
            return false;
        File fp=new File(dfile);
        fp.delete();
        return true;
    }

    //----------------------------------------------------------------
    /** Delete a directory and its files */
    //----------------------------------------------------------------
    public static boolean deleteDir(File dir) {
        if (dir == null)
            return false;
        if (!dir.exists())
            return true;
        if (!dir.isDirectory())
            return false;
        String[] list = dir.list();

        // Some JVMs return null for File.list() when the
        // directory is empty.
        if (list != null) {
           for (int i = 0; i < list.length; i++) {
              File entry = new File(dir, list[i]);
              if (entry.isDirectory())
              {
                 if (!deleteDir(entry))
                    return false;
              }
              else
              {
                 if (!entry.delete())
                    return false;
              }
           }
        }
        return dir.delete();
    }

    public static boolean deleteDir(String path) {
        String ldir=openPath(path);

        if (ldir==null)
            return false;
        File fd = new File(ldir);
        return deleteDir(fd);
    }

    //----------------------------------------------------------------
    /** Return appropriate default file name. */
    //----------------------------------------------------------------
    public static String getDefaultName(String dir) {

        // find first dir in appdirs (e.g. ../s2pul)
                String name="default";

        String ldir=openPath(new StringBuffer().append("LAYOUT").append(
                                FileUtil.separator).append(dir).append(
                                FileUtil.separator).append("DEFAULT").toString());
        if(ldir!=null){
           // check for DEFAULT file  (e.g ../s2pul/DEFAULT)
            name=readDefaultFile(ldir);
        }
        if(DebugOutput.isSetFor("layout"))
            Messages.postDebug(new StringBuffer().append("default for ").
                               append(dir).append(" = ").append(name).toString());
        return name;
    }

    //----------------------------------------------------------------
    /** Return path specified by DEFAULT file. */
    //----------------------------------------------------------------
    public static String readDefaultFile(String path) {
        String name="default";
        if(path !=null){
            try {
                BufferedReader in=new BufferedReader(new FileReader(path));
                String line=in.readLine();
                StringTokenizer tok = new StringTokenizer(line);
                if(tok.countTokens()==3){
                    tok.nextToken();
                    tok.nextToken();
                    name=tok.nextToken();
                }
                in.close();
            }
            catch (IOException ioe) {
                Messages.postError(new StringBuffer().append("error reading ").
                                   append(path).toString());
                Messages.writeStackTrace(ioe);
            }
        }
        return name;
    }

    /**
     * Expand any file paths found in the given string.
     * Deals with substrings of the form: "$VJFILE:'any thing in here'".
     * Looks for every occurance of "$VJFILE:" in the given string.
     * The following character is taken as a separator; here it is "'".
     * The stuff inside the single quotes is expanded by openPath(),
     * the "$VJFILE:" is discarded, and the single quotes are left
     * surrounding the expanded string.
     */
    public static String expandPaths(String sInString) {
        if (sInString == null || sInString.length() == 0) {
            return sInString;
        }
        final String key = "$VJFILE:";
        final int keylen = key.length();
        final String delims = " \t\n\r\f";
        StringBuffer sbOutString = null;
        int i = 0;              // Start of file spec
        int j = 0;              // End of file spec
        while ((i = sInString.indexOf(key, j)) >= 0) {
            if (sbOutString == null) {
                sbOutString = new StringBuffer();
            }
            sbOutString.append(sInString.substring(j, i));
            j = i + keylen;     // Point to initial quote
            QuotedStringTokenizer qtoker;
            String quote = sInString.substring(j, j+1);
            qtoker = new QuotedStringTokenizer(sInString.substring(j),
                                               delims,
                                               quote);
            j++;                // Point to string
            if (qtoker.hasMoreTokens()) {
                String path = qtoker.nextToken();
                j += path.length(); // Point to final quote
                path = openPath(path);
                sbOutString.append(quote);
                if (path != null) {
                    sbOutString.append(path);
                }
            }
        }
        if (sbOutString != null) {
            sbOutString.append(sInString.substring(j));
            return sbOutString.toString();
        } else {
            return sInString;
        }
    }

    //----------------------------------------------------------------
    /** Wait for user to respond to a popup. */
    //----------------------------------------------------------------
    public static boolean checkFileSystem() {
        String strShToolCmd = System.getProperty("shtoolcmd");
        String strShToolOption = System.getProperty("shtooloption");
        if (strShToolCmd == null)
            strShToolCmd = "/bin/sh";
        if (strShToolOption == null)
            strShToolOption = "-c";
        String[] cmd = {strShToolCmd, strShToolOption, "df -k" };
        if (timeObj == null)
           timeObj = new timerObj();
        if (timer == null)
              timer = new Timer(2000, timeObj);
        timeCount = 0;
        timeResult = true;
        hasUserRespondedToPopup = false;
         if (timer != null && (!timer.isRunning()))
              timer.restart();
        try {
             Runtime rt = Runtime.getRuntime();
             timeProc = rt.exec(cmd);
             boolean isProcRunning = true;
             while (isProcRunning && !hasUserRespondedToPopup) {
                 try {
                     Thread.sleep(250);
                     timeProc.exitValue(); // Throws exception if still running
                     isProcRunning = false; // Only get here if proc exited
                 } catch (IllegalThreadStateException itse) {
                     // Proc is still running
                 } catch (InterruptedException ie) {}
             }

             // NB: Wouldn't get past this line if filesystem is hung up!
             //timeProc.waitFor();

             timeProc = null;
        }
        catch (IOException e) {
           Messages.writeStackTrace(e);
           timeResult = false;
        }
        timeProc = null;
        return timeResult;
    }

    private static class timerObj extends JComponent implements ActionListener {

        public void actionPerformed(ActionEvent e) {

           Object obj = e.getSource();
           if (obj instanceof Timer) {
                timeCount++;
                if (timeProc == null)
                   timer.stop();
                if (timeCount > 5) {
                   timer.stop();
                   if (timeProc != null) {
                      Object[] options = {"Continue", "Exit" };
                      int rt = JOptionPane.showOptionDialog(null,
                        "File system hung up!",
                        "Warning",
                        JOptionPane.YES_NO_OPTION,
                        JOptionPane.QUESTION_MESSAGE,
                        null, options, options[0]);
                      if (rt == 1) {
                          timeResult = false;
                      }
                      hasUserRespondedToPopup = true;
                      timeProc.destroy();
                   }
                }
           }
        }
    }

    //----------------------------------------------------------------
    /** Cat a list of files. */
    //----------------------------------------------------------------
    public static boolean cat(String[] srcFiles, String dstFile) {
        boolean rtn = true;
        // Open output file
        BufferedWriter fw = null;
        PrintWriter pw = null;
        try {
            fw = new BufferedWriter(new FileWriter(dstFile));
            pw = new PrintWriter(fw);
        } catch (IOException ioe) {
            Messages.postError("Util.cat: Cannot open output file: " + dstFile);
            return false;
        }
        BufferedReader in = null;
        for (int i = 0; i < srcFiles.length; i++) {
            try {
                String line;
                in = new BufferedReader(new FileReader(srcFiles[i]));
                while ((line = in.readLine()) != null) {
                    pw.println(line);
                }
            } catch (IOException ioe2) {
                rtn = false;
                Messages.postError("Util.cat: Error reading file: "
                                   + srcFiles[i]);
                try { in.close(); } catch (Exception e) {}
            }
        }
        try { pw.close(); } catch (Exception e) {}
        try { in.close(); } catch (Exception e) {}

        return rtn;
    }

    /**
     * Set the location and size of a given component according to
     * the geometry stored in a persistence file.
     * @param file The name of the persistence file to read.
     * @param component The component to remember.
     * @return True iff the geometry is set successfully.
     * @see #readPersistenceRect(String)
     */
    public static boolean setGeometryFromPersistence(String file,
                                                     Component component) {
        Rectangle frameRect = readPersistenceRect(file);
        if (frameRect != null) {
            component.setSize(frameRect.width, frameRect.height);
            component.setPreferredSize(new Dimension(frameRect.width,
                                                     frameRect.height));
            component.setLocation(new Point(frameRect.x, frameRect.y));
            return true;
        } else {
            return false;
        }
    }

    /**
     * Read a specified persistence file and parse just the size and
     * location information.
     * <p>File format:
     * <pre>
     * x:309
     * y:1031
     * width:172
     * height:169
     * </pre>
     * @param file The name of the persistence file to read.
     * @return A Rectangle giving the saved window size and location.
     */
    public static Rectangle readPersistenceRect(String file) {
        Rectangle rect = null;
        String strFile = FileUtil.openPath("PERSISTENCE/" + file);

        if (strFile != null) {
            rect = new Rectangle();
            BufferedReader reader = null;
            try {
                reader = new BufferedReader(new FileReader(strFile));
                String str;
                while ((str = reader.readLine()) != null) {
                    StringTokenizer toker = new StringTokenizer(str, ":");
                    String strLabel = null;
                    if (toker.hasMoreTokens()) {
                        strLabel = toker.nextToken().trim();
                    }
                    while (toker.hasMoreTokens()) {
                        int value = Integer.parseInt(toker.nextToken().trim());
                        if (strLabel.equals("x")) {
                            rect.x = value;
                        } else if (strLabel.equals("y")) {
                            rect.y = value;
                        } else if (strLabel.equals("width")) {
                            rect.width = value;
                        } else if (strLabel.equals("height")) {
                            rect.height = value;
                        }
                    }
                }
            } catch (NumberFormatException e) {
                Messages.writeStackTrace(e);
            } catch (FileNotFoundException e) {
                Messages.writeStackTrace(e);
            } catch (IOException e) {
                Messages.writeStackTrace(e);
            }
            try {
                if (reader != null) {
                    reader.close();
                }
            } catch (IOException e) { }
        }
        return rect;
    }

    /**
     * Write out a persistence file with the location and size of
     * a given component.
     * @param file The name of the persistence file to write.
     * @param component The component to remember.
     * @return True iff the file is written successfully.
     * @see #writePersistenceRect(String, Rectangle)
     */
    public static boolean writeGeometryToPersistence(String file,
                                                     Component component) {
        Point loc = null;
        try {
            loc = component.getLocationOnScreen();
        } catch (IllegalComponentStateException ecse) {
            Messages.postDebug("FileUtil",
                               "FileUtil.writeGeometryToPersistence(): "
                               + "Cannot get component location -- "
                               + "not on screen");
            return false;
        }
        Dimension size = component.getSize();
        Rectangle rect = new Rectangle(loc.x, loc.y, size.width, size.height);
        return writePersistenceRect(file, rect);
    }

    /**
     * Write a specified persistence file recording the size and
     * location information specified.
     * <p>File format:
     * <pre>
     * x:309
     * y:1031
     * width:172
     * height:169
     * </pre>
     * @param file The name of the persistence file to write.
     * @param rect The size and location to save.
     * @return True iff the file is written successfully.
     */
    public static boolean writePersistenceRect(String file, Rectangle rect) {
        String strFile = FileUtil.openPath("PERSISTENCE/" + file);
        if (strFile == null) {
            strFile = FileUtil.savePath("PERSISTENCE/" + file);
        }
        if (strFile == null) {
            return false;
        }

        BufferedReader reader = null;
        StringBuffer sbPreviousContents = new StringBuffer();
        try {
            reader = new BufferedReader(new FileReader(strFile));
            String str;
            while ((str = reader.readLine()) != null) {
                if (!str.startsWith("x:")
                    && !str.startsWith("y:")
                    && !str.startsWith("width:")
                    && !str.startsWith("height:"))
                {
                    sbPreviousContents.append(str).append(NEWLINE);
                }
            }
        } catch (FileNotFoundException e) {
            // No problem
        } catch (IOException e) {
            Messages.writeStackTrace(e);
        }
        try {
            if (reader != null) {
                reader.close();
            }
        } catch (IOException e) { }

        boolean rtn = true;
        BufferedWriter writer = null;
        try {
            writer = new BufferedWriter(new FileWriter(strFile));
            writer.write(sbPreviousContents.toString());
            writer.write("x:" + rect.x);
            writer.newLine();
            writer.write("y:" + rect.y);
            writer.newLine();
            writer.write("width:" + rect.width);
            writer.newLine();
            writer.write("height:" + rect.height);
            writer.newLine();
            writer.flush();
        } catch (IOException e) {
            Messages.writeStackTrace(e);
            rtn = false;
        }
        try {
            if (writer != null) {
                writer.close();
            }
        } catch (IOException e) { }
        return rtn;
    }

    /**
     * Get the value string from a given persistence file corresponding
     * to a given key.
     * The format of lines in the persistence file must be:
     * <pre>key:value</pre>
     * The path to the persistence file is found through
     * <tt>FileUtil.openPath("PERSISTENCE/"+file)</tt>
     * @param file The name of the persistence file.
     * @param key The key string to look for.
     * @return The value as a String, or null if the file or key is not found.
     */
    public static String readStringFromPersistence(String file, String key) {
        String value = null;
        String strFile = FileUtil.openPath("PERSISTENCE/" + file);

        if (strFile != null) {
            BufferedReader reader = null;
            try {
                reader = new BufferedReader(new FileReader(strFile));
                String str;
                while ((str = reader.readLine()) != null) {
                    StringTokenizer toker = new StringTokenizer(str, ":");
                    String strLabel = null;
                    if (toker.hasMoreTokens()) {
                        strLabel = toker.nextToken().trim();
                    }
                    if (toker.hasMoreTokens() && strLabel.equals(key)) {
                        value = toker.nextToken().trim();
                        break;
                    }
                }
            } catch (FileNotFoundException e) {
                Messages.writeStackTrace(e);
            } catch (IOException e) {
                Messages.writeStackTrace(e);
            }
            try {
                if (reader != null) {
                    reader.close();
                }
            } catch (IOException e) { }
        }
        return value;
    }

    /**
     * Get the int value from a given persistence file corresponding
     * to a given key.
     * The format of lines in the persistence file must be:
     * <pre>key:value</pre>
     * The path to the persistence file is found through
     * <tt>FileUtil.openPath("PERSISTENCE/"+file)</tt>
     * @param file The name of the persistence file.
     * @param key The key string to look for.
     * @param defaultValue The value to return if the file or key is not found
     * or if the value string found does not represent an int.
     * @return The int value.
     */
    public static int readIntFromPersistence(String file,
                                             String key, int defaultValue) {
        int rtnVal = defaultValue;
        String strVal = readStringFromPersistence(file, key);
        try {
            rtnVal = Integer.parseInt(strVal);
        } catch (NullPointerException npe) {
        } catch (NumberFormatException nfe) {
        }
        return rtnVal;
    }

    /**
     * Writes a "key:value" line into a given persistence file.
     * The path to the persistence file is found through
     * <tt>FileUtil.openPath("PERSISTENCE/"+file)</tt>
     * @param file The name of the persistence file.
     * @param key The key string.
     * @param value The value string written is <tt>value.toString()</tt>.
     * @return True if file is successfully written/modified, otherwise false.
     */
    public static boolean writeValueToPersistence(String file,
                                                  String key,
                                                  Object value) {
        String strFile = FileUtil.openPath("PERSISTENCE/" + file);
        if (strFile == null) {
            strFile = FileUtil.savePath("PERSISTENCE/" + file);
        }
        if (strFile == null) {
            return false;
        }
        key = key + ":";

        BufferedReader reader = null;
        StringBuffer sbPreviousContents = new StringBuffer();
        try {
            reader = new BufferedReader(new FileReader(strFile));
            String str;
            while ((str = reader.readLine()) != null) {
                if (!str.startsWith(key)) {
                    sbPreviousContents.append(str).append(NEWLINE);
                }
            }
        } catch (FileNotFoundException e) {
            // No problem
        } catch (IOException e) {
            Messages.writeStackTrace(e);
        }

        boolean rtn = true;
        BufferedWriter writer = null;
        try {
            writer = new BufferedWriter(new FileWriter(strFile));
            writer.write(sbPreviousContents.toString());
            writer.write(key + value);
            writer.newLine();
            writer.flush();
        } catch (IOException e) {
            Messages.writeStackTrace(e);
            rtn = false;
        }
        try {
            if (writer != null) {
                writer.close();
            }
        } catch (IOException e) { }
        return rtn;
    }

    /**
     * Determines whether a FID file (actually a directory) exists.
     * Takes a path to a FID file, possibly without the ".fid" or ".REC" suffix.
     * If the path already includes a suffix, just checks if that directory
     * exists. Otherwise, looks for directories with the possible suffixes.
     * @param path The trial path.
     * @return The path with suffix, or null if the directory does not exist.
     */
    public static String getFidPath(String path) {
        final String[] suffixes = {".fid", ".REC"};

        // Maybe it already has a suffix
        for (String suffix : suffixes) {
            if (path.endsWith(suffix)) {
                return (new File(path).isDirectory()) ? path : null;
            }
        }

        // Maybe it needs a suffix
        for (String suffix : suffixes) {
            String pathPlus = path + suffix;
            if (new File(pathPlus).isDirectory()) {
                return pathPlus;
            }
        }
        return null;

    }
}
