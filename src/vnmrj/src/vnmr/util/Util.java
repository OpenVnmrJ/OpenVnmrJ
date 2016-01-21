/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
 *
 * This Class has functions (i.e., static methods) that serve as
 * general utilities.
 *
 */

package vnmr.util;

import java.awt.*;
import java.util.*;
import java.io.*;
import java.text.*;
import java.awt.event.*;
import java.beans.*;
import java.net.*;
import javax.swing.*;
import javax.swing.border.BevelBorder;
import javax.swing.border.Border;
import java.awt.image.BufferedImage;
import javax.imageio.ImageIO;

import vnmr.images.*;
import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.templates.*;
import vnmr.ui.shuf.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;

public class Util implements VnmrKey {
    // ==== static variables
    public static final  String[] IMG_TYPES = {"",".png",".gif",".jpg",".jpeg"};
    /** top directory */
    public static String TOPDIR = FileUtil.vnmrDir("SYSTEM","user_templates");
    public static String VNMRDIR = FileUtil.sysdir();
    /** image directory */
    public static String IMGDIR =TOPDIR+"/lib/images";
    public static String USERDIR = FileUtil.usrdir();
    public static String GRAPHICS = System.getProperty("nativeGraphics");
    public static String XWIN = System.getProperty("xwindow");
    public static String OSNAME = System.getProperty("os.name");
    public static final String SFUDIR_WINDOWS = System.getProperty("sfudirwindows");
    public static final String SFUDIR_INTERIX = System.getProperty("sfudirinterix");
    /** cache for image icons */
    private static final Hashtable<String,ImageIcon> iconCache
            = new Hashtable<String,ImageIcon>();
    private static final Hashtable<String,ImageIcon> fileIconCache
            = new Hashtable<String,ImageIcon>();
    private static final Hashtable<String,String> fileNameCache
            = new Hashtable<String,String>();
    private static final Hashtable<String,Long> fileDateCache
            = new Hashtable<String, Long>();
    private static VnmrImageUtil imgUtil = null;
    private static AppIF appIf = null;
    private static VnmrjIF vjIf = null;
    private static JFrame mainFrame = null;
    private static int windowId = 0;
    private static String expDir;
    private static ExpPanel activeView = null;
    private static ExpPanel dropView = null;
    private static ExpViewArea viewArea = null;
    private static ControlPanel controlPanel = null;
    private static Shuffler shuffler = null;
    private static LocatorPopup locatorPopup = null;
    private static HoldingArea holdingArea = null;
    private static DisplayPalette displayPalette = null;
    private static ExpStatusBar statusBar = null;
    private static NoteEntryComp noteEntryComp = null;
    private static RobotViewComp robotViewComp = null;
    protected static int m_nStatusMessages = 1000;
    private static PublisherNotesComp publisherNotesComp = null;
    private static VColorMapPanel colormapPanel = null;
    private static DisplayOptions displayOptions = null;
    private static ProtocolEditor protocoleditor = null;
    private static Hashtable studyQueues = new Hashtable();
    private static boolean bShufferInPanel = false;
    private static boolean menuIsUp = false;
    private static boolean bEnglishLocale = false;
    protected static boolean bFocusTraversal = false;
    protected static String ostype;
    private static JComponent popupMenu = null;
    private static JComponent vpMenu = null; // viewport menu
    private static JComponent sysToolBar = null;
    private static JComponent usrToolBar = null;
    private static JButton  arrowButton = null; 
    private static SimpleDateFormat m_dateFormat = null;
    private static VTabbedToolPanel vTabbedToolPanel = null;
    private static ToolPanelUtil toolPanelUtil = null;
    private static RQPanel rqPanel = null;
    private static User user = null;
    protected static String m_strCurrOperator = null;
    protected static String userAppType = null;
    private static GraphicsToolIF graphicsToolBar;
    private static VResourceBundle paramResource = null; 
    private static VResourceBundle cmdResource = null; 
    private static VLabelResource tooltipResource = null; 
    private static VLabelResource messageResource = null; 
    private static VLabelResource labelResource = null; 
    private static VLabelResource labelResource2 = null; 
    private static VResourceBundle vjLabels = null; 
    private static VResourceBundle helpIDs = null; 
    private static VResourceBundle vjAdmLabels = null; 
    private static VResourceBundle vjOptions = null; 
    private static boolean bAdminIF = false;
    private static boolean bMultSq = false;

// These are for calture language resource to files.
    private static boolean captureResource = Util.setCaptureResource();
    private static Collator alphaOrder = Collator.getInstance();
    private static SortedSet<String> allLabels = new TreeSet<String>(alphaOrder);
    private static SortedSet<String> allVJLabels = new TreeSet<String>(alphaOrder);
    private static SortedSet<String> allVJAdmLabels = new TreeSet<String>(alphaOrder);
    private static SortedSet<String> allLabels2 = new TreeSet<String>(alphaOrder);
    private static SortedSet<String> allVJLabels2 = new TreeSet<String>(alphaOrder);
    private static SortedSet<String> allVJAdmLabels2 = new TreeSet<String>(alphaOrder);

    /** Bundle of localized labels */
    private static ResourceBundle
        labels = ResourceBundle.getBundle("vnmr.properties.Labels");

    /** Bundle of localized admin labels */
    private static ResourceBundle
        admlabels = ResourceBundle.getBundle("vnmr.properties.AdmLabels");

    /** Bundle of options (non-localized) */
    private static ResourceBundle
        options = ResourceBundle.getBundle("vnmr.properties.Options");

    public static final int NONFDA = 1;
    public static final int FDA = 2;
    private static Color activeBg = null; // active title background
    private static Color activeFg = null; // active title text color
    private static Color inactiveBg = null; // inactive title background
    private static Color inactiveFg = null; // inactive title text color
    private static Color panelFg = null; // panel text color
    private static Color menuBg = null; // menu background
    private static Color menuFg = null; // menu text
    private static Color hilitBg = null; // selected item background
    private static Color hilitFg = null; // selected item color
    private static Color controlHighlight = null; // control highlight color
    private static Color gridFg = null;  // color of grids 
    private static Color menuBarBg = Global.BGCOLOR; // menubar background
    private static Color toolBarBg = Global.BGCOLOR; // active title background
    private static Color vjBg = Global.BGCOLOR;
    private static Color vjListBg = Global.BGCOLOR;
    private static Color vjListSelectBg = null;
    private static Color vjListSelectFg = null;
    private static Color separatorBg = Global.BGCOLOR;
    private static Color vjButtonBg = Global.BGCOLOR;
    private static Color inputBg = Color.white;
    private static Color inputFg = Color.black;

    private static ImageIcon horBumpIcon = null;
    private static ImageIcon verBumpIcon = null;
    
    private static TrashCan trashCan = null;
    public static int vpOverlayMode = 0; 
    // 0=off, 1=on, 2=on and spectrua overlaid 3= on and stacked

    protected static long m_lastModified = 0;
    protected static int m_nMode = NONFDA;
    protected static String m_strP11Dir = "";
    protected static String m_strAuditDir = "";
    protected static ActionHandler vpactionHandler;
    private static Hashtable<Integer,String> elabels = null;

    /**
     * Constructor is private, so nobody can construct an instance.
     */
    private Util() {  }


    //##################  Label Display utities (Internationalization) #################

    /**
     * Tests if a property label exists.
     * @param symbol property file key string
     * @return       true if symbol found else return false
     */
    public static Boolean labelExists(String symbol) {
        String label = null;
        try {
            label = labels.getString(symbol);
            return true;
        } catch(MissingResourceException e) {
            try {
                label = admlabels.getString(symbol);
                return true;
            } catch(MissingResourceException e2) {
                return false;
            }
        }
    }

    /**
     * Returns a property String for a key String using Labels and AdmLabels resource files.
     * @param symbol property file key string
     * @param dflt   String returned if "symbol" not found in properties files
     * @return       String returned from properties files or "dflt" if symbol not found
     */
    public static String getLabel(String symbol, String dflt) {

        boolean found=false;
	String label = getMyLabel(symbol);
        if(elabels==null)
            setAttributeLabels();
	if(label.equals(symbol)) {
           label = getMyLabel(dflt);
	   if(!label.equals(dflt)) found=true;
	} else found=true;
	
        if(!found) {
          try {
            label = labels.getString(symbol);
            if (!bEnglishLocale) {
               if(DebugOutput.isSetFor("labels"))
                   Messages.postDebug(symbol + "=" + label);
            }
          } catch(MissingResourceException labelExp) {
            try {
                label = admlabels.getString(symbol);
                if (!bEnglishLocale) {
                  if(DebugOutput.isSetFor("labels"))
                    Messages.postDebug(symbol + "=" + label);
                }
            } catch(MissingResourceException e) {
                // Messages.postWarning("property label not found: "+symbol);
               if (!bEnglishLocale) {
                   Messages.postDebug("WARNING: label property not found: "
                        + symbol);
               }
            }
	  }
        }
        if(captureResource) {
	   addLabel2List(symbol,label,allVJLabels, allVJLabels2);
        } 
        return label;
    }

    /**
     * Returns a property String resource for a key String.
     * @param symbol property file key string
     * @return  String returned from properties files or "symbol" if symbol not found
    */
    public static String getLabel(String symbol) {
        return getLabel(symbol, symbol);    
    }

    /**
     * Returns a property String for a key String using "AdmLabels" resource file.
     * @param symbol property file key string
     * @param dflt   String returned if "symbol" not found in properties files
     * @return       String returned from properties file or "dflt" if symbol not found
     */
    public static String getAdmLabel(String symbol, String dflt) {

        boolean found=false;
        String label = getMyAdmLabel(symbol);
	if(label.equals(symbol)) {
	   label = getMyAdmLabel(dflt);
	   if(!label.equals(dflt)) found=true;
	} else found=true;

	if(!found) {
          try {
            label=admlabels.getString(symbol);
            if(DebugOutput.isSetFor("labels"))
                Messages.postDebug(symbol+"="+label);
          }
          catch (MissingResourceException e) {
            if (!bEnglishLocale)
               Messages.postDebug("WARNING: property not found in AdmLabels resource file: "+symbol);
	  }
        }

        if(captureResource) {
	   addLabel2List(symbol,label,allVJAdmLabels, allVJAdmLabels2);
        } 
        return label;
    }

    /**
     * Returns a property String for a key String using "AdmLabels" resource file.
     * @param symbol property file key string
     * @return String returned from labels.properties or "dflt" if symbol not found
    */
    public static String getAdmLabel(String symbol) {
        return getAdmLabel(symbol, symbol);    
    }

    /**
     * Returns a property String for a key String using "Labels" resource file.
     * @param symbol property file key string
     * @param dflt   String returned if "symbol" not found in properties files
     * @return       String returned from properties file or "symbol" if symbol not found
     */
    public static String getStdLabel(String symbol, String dflt) {

        String label = getMyLabel(symbol);
	if(label.equals(symbol)) {
	   label = getMyLabel(dflt);
	   if(!label.equals(dflt)) return label;
	} else return label;
	
        try {
            label = labels.getString(symbol);
            if(DebugOutput.isSetFor("labels"))
                Messages.postDebug(symbol+"="+label);
        }
        catch (MissingResourceException e) {
            if (!bEnglishLocale)
               Messages.postDebug("WARNING: property not found in Labels resource file: "+symbol);
        }
        return label;
    }
    /**
     * Returns a property String for a key String using "Labels" resource file.
     * @param  symbol property file key string
     * @return  String returned from properties file or "symbol" if symbol not found
    */
    public static String getStdLabel(String symbol) {
        return getStdLabel(symbol, symbol);    
    }

    /**
     * Returns a property String for a key String using "Options" resource file.
     * @param symbol property file key string
     * @param dflt   String returned if "symbol" not found in properties files
     * @return       String returned from properties file or "dflt" if symbol not found
     */
    public static String getOption(String symbol, String dflt) {
	
        String label = getMyOption(symbol);
	if(label.equals(symbol) && dflt != null) {
	   label = getMyOption(dflt);
	   if(!label.equals(dflt)) return label;
	} else if(label.equals(symbol)) {
	   label = dflt;
	} else return label;
	
        try {
            label = options.getString(symbol);
            if(DebugOutput.isSetFor("labels"))
                Messages.postDebug(symbol+"="+label);
        }
        catch (MissingResourceException e) {
            //Messages.postDebug("WARNING: property not found in Options resource file: "+symbol);
        }
        return label;
    }
    /**
     * Returns a property String for a key String using "Options" resource file.
     * @param  symbol property file key string
     * @return String returned from properties file or null if symbol not found
    */
    public static String getOption(String symbol) {
        return getOption(symbol, null);    
    }

    /**
     * Returns a property String resource for a Panel Editor attribute field.
     * @param  id VObjDef enumeration value (e.g. VObjDef.CMD)
     * @return String returned from labels.properties with key=pe<id> (e.g. peCMD)
     * --------------------------------
     * Supported VObjDef.<id> values for util.getLabel(int)
     * --------------------------------
     * eLABEL         Label of Item: 
     * eSETCHOICE     Labels of Choices:
     * eSETCHVAL      Values of Choices:
     * eVARIABLE      Vnmr Variables:
     * eLABELVARIABLE Label Variable:
     * eLABELVALUE    Label Value:
     * eCMD           Vnmr Command:
     * eCMD2          Vnmr Command 2:
     * eSETVAL        Value of Item:
     * eSHOW          Enable Condition:
     * eNUMDIGIT      Decimal Places:
     * eICON          Icon of Item:
     * eBGCOLOR       Background Color:
     * eTITLE         Title:
     * eSTATPAR       Status Variables:
     * eSTATCOL       Status Color Hint:
     * eSTATKEY       Status Variable:
     * eSTATSHOW      Enable Status Values:
     * eEDITABLE      Editable:
     * eWRAP          Wrap Lines:
     * eMIN           Min Value:
     * eMAX           Max Value:
     * eTOOLTIP       Tooltip:
     */   
    public static String getLabel(int a){
        if(elabels==null)
            setAttributeLabels();
        Integer key=new Integer(a);
        String sym=elabels.get(key);
        if(sym==null){
            if(DebugOutput.isSetFor("labels"))
                Messages.postWarning("missing PE label property: "+key);
            return "*****"; // better to show something than get a null pointer error
        }
        return getLabel(sym,sym.replace("e",""));
    }

    private static void setAttributeLabels(){
        elabels = new Hashtable<Integer,String>();
        elabels.put(VObjDef.LABEL,"eLABEL");
        elabels.put(VObjDef.SETCHOICE,"eSETCHOICE");
        elabels.put(VObjDef.SETCHVAL,"eSETCHVAL");
        elabels.put(VObjDef.VARIABLE,"eVARIABLE");
        elabels.put(VObjDef.LABELVARIABLE,"eLABELVARIABLE");
        elabels.put(VObjDef.LABELVALUE,"eLABELVALUE");
        elabels.put(VObjDef.CMD,"eCMD");
        elabels.put(VObjDef.CMD2,"eCMD2");
        elabels.put(VObjDef.SHOW,"eSHOW");
        elabels.put(VObjDef.NUMDIGIT,"eNUMDIGIT");
        elabels.put(VObjDef.STATPAR,"eSTATPAR");
        elabels.put(VObjDef.ICON,"eICON");
        elabels.put(VObjDef.BGCOLOR,"eBGCOLOR");
        elabels.put(VObjDef.TITLE,"eTITLE");
        elabels.put(VObjDef.SETVAL,"eSETVAL");
        elabels.put(VObjDef.STATCOL,"eSTATCOL");
        elabels.put(VObjDef.STATKEY,"eSTATKEY");
        elabels.put(VObjDef.STATSHOW,"eSTATSHOW");
        elabels.put(VObjDef.SHOWMAX,"eSHOWMAX");
        elabels.put(VObjDef.EDITABLE,"eEDITABLE");         
        elabels.put(VObjDef.WRAP,"eWRAP");
        elabels.put(VObjDef.MIN,"eMIN");
        elabels.put(VObjDef.MAX,"eMAX");
        elabels.put(VObjDef.STATVAL,"eSTATVAL");
        elabels.put(VObjDef.RADIOBUTTON,"eRADIOBUTTON");
        elabels.put(VObjDef.TOOL_TIP,"eTOOLTIP");
        elabels.put(VObjDef.TOOLTIP,"eTOOLTIP");
        Locale locale = Locale.getDefault(); 
        if (locale != null) {
           String lang = locale.getDisplayLanguage();
           if (lang != null && (lang.equalsIgnoreCase("english")))
               bEnglishLocale = true;
        }
    }

    //##################  Image utities #####################
   
    /**
     * get the full file path for a given image file
     * @param imageFile image file
     * @return full file path
     */
    public static String getStdImageFile(String imageFile) {
        return new StringBuffer().append(IMGDIR).append(File.separator).
                    append( imageFile).toString();
    } // getStdImageFile()

    public static String getSysImageFile(String imageFile) {
        return new StringBuffer().append(VNMRDIR).append( File.separator).
                    append("iconlib").append(File.separator ).append( imageFile).toString();
    } // getSysImageFile()

    public static String getImageFile(String imageFile) {
        return new StringBuffer().append(IMGDIR ).append( File.separator ).
                    append( imageFile).toString();
    } // getImageFile()

    public static String getUserImageFile(String imageFile) {
        return new StringBuffer().append(USERDIR ).append( File.separator).append("iconlib").
                    append(File.separator ).append( imageFile).toString();
    } // getUserImageFile()

    public static String getUserButtonImage(String imageFile) {
        return new StringBuffer().append(USERDIR ).append( File.separator).
                    append("iconlib").append(File.separator ).append( imageFile).toString();
    } // getUserButtonImage()

    public static String getSysButtonImage(String imageFile) {
        return new StringBuffer().append(VNMRDIR).append(File.separator).append( "iconlib").
                    append(File.separator ).append( imageFile).toString();
    } // getSysButtonImage()

    public static ImageIcon getImageFileIcon(String f, boolean bUpdate) {
        ImageIcon imageIcon = null;
        boolean  bRootPath = false;
        boolean  bDotType = false;
        File fd = null;

        synchronized (fileIconCache) {
            imageIcon = fileIconCache.get(f);
            String fpath = fileNameCache.get(f);
            if (imageIcon != null) {
                if (!bUpdate)
                    return imageIcon;

                Long date = fileDateCache.get(f);
                if (fpath != null && date != null) {
                    fd = new File(fpath);
                    if (fd.isFile() && fd.canRead()) {
                        if (date.longValue() == fd.lastModified())
                            return imageIcon;
                    }
                }
            }
            else if (fpath != null) { // had been called before
                if (!bUpdate)
                    return null;
            }
            fpath = null;

            if (f.startsWith(File.separator) || f.startsWith("/"))
                bRootPath = true;
            else if (UtilB.OSNAME.startsWith("Windows") && f.indexOf(':') == 1)
                bRootPath = true;
            if (f.indexOf('.') > 0)
                bDotType = true;
            if (bRootPath) {
                fd = new File(f);
                if (fd.isFile() && fd.canRead())
                   fpath = f;
            }
            else {
                for (int i = 0; i < IMG_TYPES.length && imageIcon == null; i++) {
                    String s = f+IMG_TYPES[i];
                    fpath = getStdImageFile(s);
                    fd = new File(fpath);
                    if (fd.isFile() && fd.canRead())
                        break;
                    fpath = getUserImageFile(s);
                    fd = new File(fpath);
                    if (fd.isFile() && fd.canRead())
                        break;
                    fpath = getSysImageFile(s);
                    fd = new File(fpath);
                    if (fd.isFile() && fd.canRead())
                        break;
                    fpath = null;
                    if (bDotType)
                        break;
                }
            }
            BufferedImage img = null;
            if (fpath != null) {
                // imageIcon = new ImageIcon(fpath);
                try {
                    img = ImageIO.read(fd);
                }
                catch (IOException e) {
                    img = null;
                }
            }
            if (img != null) {
                imageIcon = new ImageIcon(img);
                if (img.getWidth() < 100 && img.getHeight() < 100) {
                   fileNameCache.put(f, fpath);
                   fileIconCache.put(f, imageIcon);
                   fileDateCache.put(f, new Long(fd.lastModified()));
                }
                else {
                   fileIconCache.remove(f);
                   fileNameCache.remove(f);
                }
            }
            else
                fileNameCache.put(f, f);
        }
        return imageIcon;
    } 

    /**
     * Factory function to get an image icon for the given image file.
     * Looks for the following files, in this order:<br>
     * imageFile <br>
     * imageFile.png <br>
     * imageFile.gif <br>
     * imageFile.jpg <br>
     * imageFile.jpeg <br>
     * Caches icons that it finds in a hashtable.
     * @param imageFile image file
     * @return image icon
     */
    public static ImageIcon getImageIcon(String imageFile) {
        ImageIcon imageIcon = null;
        if (imgUtil == null)
           imgUtil = new VnmrImageUtil();
        synchronized (iconCache) {
            imageIcon = iconCache.get(imageFile);
            if (imageIcon == null) {
                for (int i = 0; i < IMG_TYPES.length && imageIcon == null; i++) {
                    String filename = imageFile + IMG_TYPES[i];
                    imageIcon = imgUtil.getVnmrJIcon(filename); // get from jar
                    if (imageIcon == null) {
                       String s = null;
                       if (filename.startsWith("/"))
                          s = FileUtil.openPath(filename);
                       else
                          s = FileUtil.openPath("ICONLIB/"+filename);
                       if (s != null)
                          imageIcon = new ImageIcon(s);
                     }
                }
            if(imageIcon!=null)
                iconCache.put(imageFile, imageIcon);
            }
        }
        return imageIcon;
    } // getImageIcon()

    public static Image  getImage(String imageFile) {
        Image     img = null;
        String    fpath;
        File      fd;
        synchronized (iconCache) {
            ImageIcon icon = iconCache.get(imageFile);
            if (icon != null) {
                   return icon.getImage();
                }
            img = getVnmrImage(imageFile);
            if (img == null) {
                fpath = getUserImageFile(imageFile);
                fd = new File(fpath);
                if (fd.isFile() && fd.canRead())
                   img = Toolkit.getDefaultToolkit().getImage(fpath);
                else {
                   fpath = getSysImageFile(imageFile);
                   fd = new File(fpath);
                   if (fd.isFile() && fd.canRead())
                      img = Toolkit.getDefaultToolkit().getImage(fpath);
               }
            }
        }
        return img;
    } // getImage()

    public static ImageIcon getVnmrImageIcon(String imageFile) {
        ImageIcon imageIcon = null;
        String    fpath;
        File      fd;
        synchronized (iconCache) {
            imageIcon = iconCache.get(imageFile);
            if (imageIcon == null) {
                fpath = getUserImageFile(imageFile);
                fd = new File(fpath);
                if (fd.isFile() && fd.canRead())
                    imageIcon = new ImageIcon(fpath);
                else{
                    fpath = getSysImageFile(imageFile);
                    fd = new File(fpath);
                    if (fd.isFile() && fd.canRead())
                        imageIcon = new ImageIcon(fpath);
                }
                if(imageIcon!=null)
                    iconCache.put(imageFile, imageIcon);
            }
        }
        return imageIcon;
    } // getVnmrImageIcon()

    public static ImageIcon getButtonIcon(String imageFile, int dir) {
        ImageIcon imageIcon = null;
        String    fpath;
        File      fd;

        if (imageFile == null)
           return null;
        imageIcon = getImageFileIcon(imageFile, false);
       /********
        if (imageIcon == null) {
           synchronized (iconCache) {
              imageIcon = iconCache.get(imageFile);
              if (imageIcon == null) {
                  fpath = getUserButtonImage(imageFile);
                  fd = new File(fpath);
                  if (fd.isFile() && fd.canRead())
                      imageIcon = new ImageIcon(fpath);
                  else {
                    fpath = getSysButtonImage(imageFile);
                    fd = new File(fpath);
                    if (fd.isFile() && fd.canRead())
                        imageIcon = new ImageIcon(fpath);
                  }
                  iconCache.put(imageFile, imageIcon);
               }
            }
        }
        *********/
        return imageIcon;
    } // getButtonIcon()

    public static Image getVnmrImage(String imageFile) {
        if (imgUtil == null)
           imgUtil = new VnmrImageUtil();
        if (imageFile == null)
           return null;
        Image img = null;
        ImageIcon imgIcon = getImageFileIcon(imageFile, false);
        if (imgIcon != null)
            img = imgIcon.getImage();
        if (img == null) 
            img = imgUtil.getVnmrJImage(imageFile); // retrieve from jar
        return img;
    }

    public static InputStream getVnmrImageStream(String imageFileName) {
        InputStream stream = null;
        if (imgUtil == null) {
            imgUtil = new VnmrImageUtil();
        }
        stream = imgUtil.getVnmrJImageStream(imageFileName);
        return stream;
    }

    public static ImageIcon getGeneralIcon(String name) {
        if (name == null)
           return null;
        ImageIcon icon = getImageFileIcon(name, true);
        if (icon != null)
           return icon;
       
        /**********
        String imageName;
        if (name.startsWith("/")) {
           imageName = name;
           File f = new File(name);
           if (!f.exists()) {
               if (name.indexOf('.') < 0) {
                  imageName = name+".gif";
                  f = new File(imageName);
               }
           }
           if (!f.exists()) {
               return null;
           }
           icon = new ImageIcon(imageName);
           return icon;
        }
        if (name.indexOf('.') < 0)
           imageName = name + ".gif";
        else
           imageName = name;
        icon = getImageIcon(imageName);
        **********/
        icon = getImageIcon(name);
        return icon;
    }

    //##################  Message log utities #####################

    /**
     * Log a given message to console.
     * @param message log message
     */
    public static void log(String message) {
        System.out.println(new StringBuffer().append("log: " ).append( message).toString());
    } // log()

    /**
     * Print error message, do not abort.
     * @param message error message
     * @param throwable a Throwable where the error happened
     */
    public static void error(String message, Throwable throwable) {
        if (message != null)
            System.out.println(new StringBuffer().append("error: " ).append( message).toString());
        throwable.printStackTrace(System.out);
    } // error()

    /**
     * Print error message, do not abort.
     * @param message error message
     */
    public static void error(String message) {
        System.out.println(new StringBuffer().append("error: " ).append( message).toString());
    } // error()

    /**
     * Print error message, then abort.
     * @param message error message
     * @param throwable a Throwable where the error happened
     */
    public static void abort(String message, Throwable throwable) {
        error(message, throwable);
        System.exit(1);
    } // abort()

    //##################  AppIF utities #####################

    public static void setAppIF(AppIF apIf) {
        if (apIf != null) {
           if (apIf instanceof VnmrjIF)
              vjIf = (VnmrjIF) apIf;
        }
        else
           vjIf = null;
        appIf = apIf;
    }

    public static AppIF getAppIF() {
        return appIf;
    }

    public static VnmrjIF getVjIF() {
        return vjIf;
    }

    public static void removeAppIF() {
        if (appIf instanceof VnmrjIF)
           vjIf = null;
        appIf = null;
    }

    public static VjToolBar getToolBar() {
        if (vjIf == null)
           return null;
        return vjIf.getToolBar();
    }

    public static boolean isPart11Sys() {
        // for now, just check to see whether part11Config exists.

        String strPath = FileUtil.openPath("SYSTEM/PART11/part11Config");
        if (strPath == null)
            return false;
        else
            return true;
    }

    public static int getPart11Mode() {
        String strPath = FileUtil.openPath("SYSTEM/PART11/part11Config");
        int nMode = 0;
        if (strPath == null)
            return nMode;

        File objFile = new File(strPath);
        long lastModified = objFile.lastModified();
        // if the file has been changed, then read the file again.
        if (lastModified != m_lastModified) {
            setPart11Fields(strPath);
            m_lastModified = lastModified;
        }
        return m_nMode;
    }

    public static String getP11Dir() {
        String strPath = FileUtil.openPath("SYSTEM/PART11/part11Config");

        if (strPath == null)
            return "";

        File objFile = new File(strPath);
        long lastModified = objFile.lastModified();
        // if the file has been changed, then read the file again.
        if (lastModified != m_lastModified) {
            setPart11Fields(strPath);
            m_lastModified = lastModified;
        }
        return m_strP11Dir;
    }

    public static String getAuditDir() {
        String strPath = FileUtil.openPath("SYSTEM/PART11/part11Config");

        if (strPath == null)
            return "";

        File objFile = new File(strPath);
        long lastModified = objFile.lastModified();
        // if the file has been changed, then read the file again.
        if (lastModified != m_lastModified) {
            setPart11Fields(strPath);
            m_lastModified = lastModified;
        }
        return m_strAuditDir;
    }

    protected static void setPart11Fields(String strPath) {
        BufferedReader reader = vnmr.admin.util.WFileUtil.openReadFile(strPath);
        String strLine;
        String strMode = "";
        if (reader == null)
            return;

        try {
            while ((strLine = reader.readLine()) != null) {
                StringTokenizer sTokLine = new StringTokenizer(strLine, ":");
                String strTok = null;
                if (sTokLine.hasMoreTokens()) {
                    strTok = sTokLine.nextToken();
                    if (strTok.equalsIgnoreCase("dataType")
                            && sTokLine.hasMoreTokens()) {
                        strMode = sTokLine.nextToken();
                    } else if (strTok.equalsIgnoreCase("auditDir")
                            && sTokLine.hasMoreTokens()) {
                        m_strAuditDir = sTokLine.nextToken();
                        strPath = FileUtil.savePath(m_strAuditDir + "/tmp");
                        if (strPath == null)
                            Messages.postError("Error creating/opening file: "
                                    + m_strAuditDir + " Please check the "
                                    + "permissions for this directory.");
                        else
                            WUtil.deleteFile(strPath);
                    } else if (strTok.equalsIgnoreCase("part11Dir")
                            && sTokLine.hasMoreTokens()) {
                        m_strP11Dir = sTokLine.nextToken();
                        strPath = FileUtil.savePath(m_strP11Dir + "/tmp");
                        if (strPath == null)
                            Messages.postError("Error creating/opening: "
                                    + m_strP11Dir + " Please check the "
                                    + "permissions for this directory");
                        else
                            WUtil.deleteFile(strPath);
                    }
                }
            }
            reader.close();

            if (strMode != null && strMode.length() > 0) {
                if (strMode.equalsIgnoreCase("non-fda"))
                    m_nMode = NONFDA;
                else
                    m_nMode = FDA;
            }
        } catch (Exception e) {
            Messages.writeStackTrace(e);
            // e.printStackTrace();
            Messages.postDebug(e.toString());
        }
    }

    public static void addVpListener(ActionListener a) {
        if (vpactionHandler == null)
            vpactionHandler = new ActionHandler();

        vpactionHandler.addActionListener(a);
    }

    public static ActionHandler getVpActionHandler()
    {
        return vpactionHandler;
    }

    public static void setFocus()
    {
        String strpath = FileUtil.openPath("vjfocus");
        if (strpath != null)
            bFocusTraversal = true;
    }

    public static boolean isFocusTraversal()
    {
        return bFocusTraversal;
    }

    public static boolean islinux()
    {
        return (OSNAME != null && OSNAME.startsWith("Linux"));
    }

    public static boolean iswindows()
    {
        return (OSNAME != null && OSNAME.startsWith("Windows"));
    }

    public static boolean isMacOs()
    {
        return (OSNAME != null && OSNAME.startsWith("Mac"));
    }

    public static boolean isVJmol()
    {
        boolean bVJmol = false;
        String strPath = FileUtil.openPath(FileUtil.SYS_VNMR + "/java/vjmol.jar");
        if (strPath != null)
            bVJmol = true;
        return bVJmol;
    }

    //################## VnmrBg IO utities #####################

    public static void setDropView(ExpPanel exp) {
        dropView = exp;
    }

    public static void removeDropView() {
        dropView = null;
    }

    public static void sendToVnmr(String data) {
        // Note, dropView is set by VnmrCanvas before drop and removed after.
        if (dropView != null) 
            dropView.sendToVnmr(data);

        else if (appIf != null)
            appIf.sendToVnmr(data);
    }

    public static void sendToAllVnmr(String data) {
        if (appIf != null)
            appIf.sendToAllVnmr(data);
    }

    /**
     * exit VNMR and close window
    **/
    public static void exitVnmr() {
        if (appIf != null)
            appIf.exitVnmr();
    }

    public synchronized static ParamIF syncQueryParam(String param) {
        if (activeView != null) {
            ParamIF d = activeView.syncQueryVnmr(PVAL, new StringBuffer().append("$VALUE= ").
                                                            append(param).toString());
            return d;
        }
        else {
            return null;
        }
    }

    public static int syncQueryIntParam(String param, int defaultValue) {
        ParamIF pif = syncQueryParam(param);
        if (pif != null) {
            try {
                defaultValue = Integer.parseInt(pif.value);
            } catch (NumberFormatException ex) {
            }
        }
        return defaultValue;
    }

    public synchronized static ParamIF syncQueryValue(String cmd) {
        if (activeView != null) {
            ParamIF d = activeView.syncQueryVnmr(PVAL, cmd);
            return d;
        }
        else {
            return null;
        }
    }

    public synchronized static ParamIF syncQueryDGroup(String param) {
        if (activeView != null)
            return activeView.syncQueryVnmr(PGRP, param);
        else
            return null;
    }

    public synchronized static ParamIF syncQueryPStatus(String param) {
        if (activeView != null)
            return activeView.syncQueryVnmr(PSTATUS, param);
        else
            return null;
    }

    public synchronized static ParamIF syncQueryMinMax(String param) {
        if (activeView != null)
            return activeView.syncQueryVnmr(PMINMAX, param);
        else
            return null;
    }

    public synchronized static ParamIF syncQueryExpr(String param) {
        if (activeView != null)
            return activeView.syncQueryExpr(param);
        else
            return null;
    }

    //################## VnmrJ Componant utities #####################

    public static SessionShare getSessionShare() {
        if (appIf != null)
            return appIf.getSessionShare();
        return null;
    }

    public static void setControlPanel(ControlPanel p) {
        controlPanel = p;
    }

    public static ControlPanel getControlPanel() {
        return controlPanel;
    }

    public static void setStatusMessagesNumber(int number)
    {
        m_nStatusMessages = number;
    }

    public static int getStatusMessagesNumber()
    {
        return m_nStatusMessages;
    }

    public static void setShuffler(Shuffler p) {
        shuffler = p;
    }

    public static Shuffler getShuffler() {
        return shuffler;
    }

    public static void shufflerInToolPanel(boolean b) {
        bShufferInPanel = b;
    }

    public static boolean isShufflerInToolPanel() {
        return bShufferInPanel;
    }

    public static void setLocatorPopup(LocatorPopup p) {
        locatorPopup = p;
    }

    public static LocatorPopup getLocatorPopup() {
        return locatorPopup;
    }

    // PublisherNotesComp utilities

    /** set PublisherNotesComp. */
    public static void setPublisherNotesComp(PublisherNotesComp p) {
        publisherNotesComp = p;
    }

    /** get PublisherNotesComp. */
    public static PublisherNotesComp getPublisherNotesComp() {
        return publisherNotesComp;
    }
    // QueuePanel utilities

    /** set QueuePanel. */
    public static void setStudyQueue(QueuePanel p) {
        studyQueues.put("SQ",p);
    }

    /** get QueuePanel. */
    public static QueuePanel getStudyQueue() {
        return (QueuePanel)studyQueues.get("SQ");
    }

    /** set QueuePanel. */
    public static void setStudyQueue(QueuePanel p, String id) {
        studyQueues.put(id, p);
    }

    /** get QueuePanel. */
    public static QueuePanel getStudyQueue(String id) {
        return (QueuePanel)studyQueues.get(id);
    }

    // DisplayOptions Dialog utilities

    /** set DisplayOptions dialog. */
    public static void setDisplayOptions(DisplayOptions p) {
        displayOptions = p;
    }

    /** get DisplayOptions dialog. */
    public static DisplayOptions getDisplayOptions() {
        if (displayOptions == null) {
            // No DisplayOptions panel or not owned by main frame; make it now
            ExpViewArea expViewArea = Util.getAppIF().expViewArea;
            if (expViewArea != null) {
                SessionShare sshare = expViewArea.getSessionShare();
                displayOptions = new DisplayOptions(sshare, null);
                displayOptions.setVnmrIf(expViewArea.getDefaultExp());
                displayOptions.build();
            }
        }
        return displayOptions;
    }

    /** show DisplayOptions dialog. */
    public static void showDisplayOptions() {
        showDisplayOptions(null);
    }

    public static void showDisplayOptions(String strhelpfile) {
        try {
            getDisplayOptions().setVisible(true, strhelpfile);
        } catch (NullPointerException npe) {
        }
    }

    // ProtocolEditor Dialog utilities

    /** get ProtocolEditor dialog. */
    public static ProtocolEditor getProtocolEditor() {
        return protocoleditor;
    }

    /** show pProtocolEditor dialog. */
    public static void showProtocolEditor(String helpFile) {
        if(protocoleditor==null)
            protocoleditor=new ProtocolEditor(helpFile);
        protocoleditor.setVisible();
    }

    // VTabbedToolPanel utilities

    /** set VTabbedToolPanel. */
    public static void setVTabbedToolPanel(VTabbedToolPanel p) {
        vTabbedToolPanel = p;
    }

    /** get VTabbedToolPanel. */
    public static VTabbedToolPanel getVTabbedToolPanel() {
        return vTabbedToolPanel;
    }

    public static ToolPanelUtil getToolPanelUtil() {
        if (toolPanelUtil==null)
            toolPanelUtil = new ToolPanelUtil();
        toolPanelUtil.setVisible(true);
        return toolPanelUtil;
    }

    // RQPanel utilities

    /** set RQPanel. */
    public static void setRQPanel(RQPanel p) {
        rqPanel = p;
    }

    /** get RQPanel. */
    public static RQPanel getRQPanel() {
        return rqPanel;
    }

    public static JComponent getViewportMenu() {
        return vpMenu;
    }

    public static void setViewportMenu(JComponent c) {
        vpMenu = c;
    }

    public static int fontStyle(String strStyle) {
        int rtn = Font.PLAIN;
        strStyle = strStyle.toLowerCase();
        if (strStyle.contains("bold")) {
            rtn |= Font.BOLD;
        }
        if (strStyle.contains("italic")) {
            rtn |= Font.ITALIC;
        }
        return rtn;
    }

    public static void setHoldingArea(HoldingArea p) {
        holdingArea = p;
    }

    public static HoldingArea getHoldingArea() {
        return holdingArea;
    }

    public static void setParamPanel(String name, ParameterPanel p) {
        if (controlPanel != null)
            controlPanel.setParamPanel(name, p);
    }

    public static ParameterPanel getParamPanel(String name) {
        if (controlPanel != null)
            return controlPanel.getParamPanel(name);
        else
            return null;
    }

    public static ParamInfoPanel getPanelEditor()
    {
        return ParamInfo.getInfoPanel();
    }

    public static ParameterPanel getParamPanel() {
        if (controlPanel != null)
            return controlPanel.getParamPanel();
        else
            return null;
    }

    public static JComponent getActionPanel(String name) {
        if (controlPanel != null)
            return controlPanel.getActionPanel(name);
        else
            return null;
    }

    public static JComponent getActionPanel() {
        if (controlPanel != null)
            return controlPanel.getActionPanel();
        else
            return null;
    }

    public static void setWindowId(int id) {
        windowId = id;
    }

    public static int getWindowId() {
        return windowId;
    }

    public static void setViewArea(ExpViewArea ev) {
        viewArea = ev;
    }

    public static ExpViewArea getViewArea() {
        return viewArea;
    }

    public static void setActiveView(ExpPanel exp) {
        if (exp == null)
           return; 
        if (exp != activeView) {
           activeView = exp;
           if (sysToolBar != null)
             ((SysToolPanel)sysToolBar).setActiveExp(exp);
        }
    }

    public static ExpPanel getActiveView() {
        return activeView;
    }

    public static void setDisplayPalette(DisplayPalette p) {
        displayPalette = p;
    }

    public static DisplayPalette getDisplayPalette() {
        return displayPalette;
    }

    public static void setGraphicsToolBar(GraphicsToolIF p) {
        graphicsToolBar = p;
    }

    public static GraphicsToolIF getGraphicsToolBar() {
        return graphicsToolBar;
    }


    public static void setExpStatusBar(ExpStatusBar p) {
        statusBar = p;
    }

    public static ExpStatusBar getExpStatusBar() {
        return statusBar;
    }

    public static void setNoteEntryComp(NoteEntryComp p) {
        noteEntryComp = p;
    }

    public static NoteEntryComp getNoteEntryComp() {
        return noteEntryComp;
    }

    public static void setRobotViewComp(RobotViewComp p) {
        robotViewComp = p;
    }

    public static RobotViewComp getRobotViewComp() {
        return robotViewComp;
    }

    public static ExpPanel getDefaultView() {
        if (viewArea != null)
            return viewArea.getDefaultExp();
        else
            return null;
    }

    public static ExpPanel getDefaultExp() {
        if (viewArea != null)
            return viewArea.getDefaultExp();
        else
            return null;
    }

    public static void sendToActiveView(String data) {
        if (activeView != null)
            activeView.sendToVnmr(data);
    }

    public static String  checkUiLayout(boolean create, String name) {
        if (name.length() <= 0)
            return null;
        String persona = System.getProperty("persona");
        if (persona == null)
            persona = "std";
        String  fpath, fname;
        String  fdir;
        boolean hasRoot;
        File fd;

        hasRoot = false;
        if (name.startsWith("/")) {
            fd = new File(name);
            fdir = fd.getParent();
            fname = fd.getName();
            hasRoot = true;
        }
        else {
            fdir =new StringBuffer().append( USERDIR).append("/templates/vnmrj/geom/").
                       append(persona).toString();
            fname = name;
        }
        if (create) {
            fd = new File(fdir);
            if (!fd.exists())
                 fd.mkdirs();
            // fpath = USERDIR+"/templates/vnmrj/geom/"+persona+"/"+name;
            fpath =new StringBuffer().append( fdir).append(File.separatorChar).
                        append(fname).toString();
            return fpath;
        }
        // fpath = USERDIR+"/templates/vnmrj/geom/"+persona+"/"+name;
        fpath =new StringBuffer().append( fdir).append(File.separatorChar).
                    append(fname).toString();
        fd = new File(fpath);
        if ((!fd.exists()) || (!fd.canRead())) {
           if (hasRoot)
                return null;
           fpath =new StringBuffer().append( TOPDIR).append("/vnmrj/geom/").
                       append(persona).append("/").append(fname).toString();
           fd = new File(fpath);
           if ((!fd.exists()) || (!fd.canRead())) {
                return null;
           }
        }
        return fpath;
    }

    public static void openUiLayout(String name) {
        if (appIf == null)
            return;
        // String  f = checkUiLayout(false, name);
        String f = FileUtil.openPath(name);
        if (f == null)
            return;
        appIf.openUiLayout(f);
    }

    public static void openUiLayout(String name, int vpNum) {
        if (appIf == null)
            return;
        // String  f = checkUiLayout(false, name);
        String f = FileUtil.openPath(name);
        if (f == null)
            return;
        appIf.openUiLayout(f, vpNum);
    }

    public static void saveUiLayout(String name) {
        if (appIf == null)
            return;
        String f = FileUtil.savePath(name, false);
        if (f == null)
            f = checkUiLayout(true, name);
        if (f != null)
            appIf.saveUiLayout(f);
    }

    public static boolean isNativeGraphics() {
        if (OSNAME != null) {
            if (OSNAME.startsWith("Mac ") || OSNAME.startsWith("Windows"))  // Mac Os
                return false;
        }

        boolean bNative = false;

        if (islinux())
            bNative = false;

        if (GRAPHICS != null) {
            if (GRAPHICS.equals("yes") || GRAPHICS.equals("true"))
                bNative = true;
        }
        if (XWIN != null) {
            if (XWIN.equals("yes") || XWIN.equals("true"))
                bNative = true;
        }
        return bNative;
    }

    public static boolean isMenuUp() {
        return menuIsUp;
    }

    public static JComponent getPopupMenu() {
        return popupMenu;
    }

    public static void setMenuUp(boolean s, JComponent c) {
        menuIsUp = s;
        if (s)
           popupMenu = c;
        else
           popupMenu = null;
    }

    public static void setMenuUp(boolean s) {
        menuIsUp = s;
        popupMenu = null;
    }

    public static void setExpDir(String s) {
        expDir = s;
    }

    public static String getExpDir() {
        return expDir;
    }

    public static void setSysToolBar(JComponent c) {
        sysToolBar = c;
    }

    public static JComponent getSysToolBar() {
        return sysToolBar;
    }

    public static void setUsrToolBar(JComponent c) {
        usrToolBar = c;
    }

    public static JComponent getUsrToolBar() {
        return usrToolBar;
    }

 
    /**
     *  Sets the tool tip for the button.
     *  @param strLabel label of the button.
     */
    public static String getTooltipText(String strLabel, String strTooltip) {
        if (strLabel == null) {
            return "";
        }
        int nEndIndex = strLabel.indexOf(".gif");
        // the icon string may contain directory names e.g.
        // /vnmr/iconlib/dfsa.gif
        // therefore just get the icon name "dfsa" from this string.
        if (nEndIndex > 0) {
            int nStartIndex = strLabel.lastIndexOf('/') + 1;
            if (nStartIndex < 0)
                nStartIndex = 0;
            strLabel = strLabel.substring(nStartIndex, nEndIndex);
        }

        // if there is a key accelerator for this button,
        // then append that key string to the tool tip.
        // if (ExperimentIF.isKeyBinded(strLabel))
/*
        if (vjIf != null && vjIf.isKeyBinded(strLabel)) {
            String strKey = vjIf.getKeyBinded(strLabel);

            if (strTooltip == null) {
                strTooltip = new StringBuffer().append("\b ").append(strKey)
                        .append(" ").toString();
            } else {
                int nIndex = strTooltip.indexOf('\b');
                if (nIndex >= 0)
                    strTooltip = strTooltip.substring(0, nIndex);
                strTooltip += '\b' + "    " + strKey + " ";
            }
        }
*/
        return strTooltip;
    }

    public static String getHostName() {
        String host;
        try {
            InetAddress inetAddress = InetAddress.getLocalHost();
            host = inetAddress.getHostName();
        }
        catch(Exception e) {
            Messages.postError("Error getting HostName");
            Messages.writeStackTrace(e);
            host = new String("");
        }
        return host;
    }

    public static String getViewPortType() {
        // need to change this if plan=0, current=1, review=2 no longer true.

        if(activeView == null) return "";

        int vpId = activeView.getViewId();
        if(vpId == 0) return Global.PLAN;
        else if(vpId == 1) return Global.CURRENT;
        else if(vpId == 2) return Global.REVIEW;
        else return "";
    }

    public static void dbAction(String action, String type) {
        if (viewArea != null)
            viewArea.dbAction(action, type);
    }

    public static void setUser(User u) {
        user = u;
    }

    public static User getUser() {
        return user;
    }

    public static String getUserAppType() {
        if (user == null)
           return null;
        if (userAppType == null) {
           ArrayList types = user.getAppTypes();
           if (types != null && types.size() > 0)
              userAppType = (String)types.get(0);
        }
        return userAppType;
    }

    public static boolean isImagingUser() {
        if (userAppType == null) { 
           getUserAppType();
           if (userAppType == null)
              return false;
        }
        if (userAppType.equals(Global.IMGIF))
           return true;
 
        return false;
    }

    public static boolean isWalkupUser() {
        if (userAppType == null) { 
           getUserAppType();
           if (userAppType == null)
              return false;
        }
        if (userAppType.equals(Global.WALKUPIF))
           return true;
        if (userAppType.equals(Global.LCIF))
           return true;
 
        return false;
    }



    public static void updateVpMenuLabel() {
        if (sysToolBar != null)
             ((SysToolPanel)sysToolBar).updateVpMenuLabel();
    }

    public static void updateVpMenuStatus() {
        if (sysToolBar != null)
             ((SysToolPanel)sysToolBar).updateVpMenuStatus();
    }

    public static void setVpNum(int n) {
        // if (viewArea != null)
        //     viewArea.setCanvasNum(n);
    }

    public static void setMainFrame(JFrame f) {
        mainFrame = f;
    }

    public static JFrame getMainFrame() {
        return mainFrame;
    }

    public static void setCurrOperatorName(String strOperator) {
        m_strCurrOperator = strOperator;
	if(user != null) user.setCurrOperatorName(strOperator);
    }

    public static String getCurrOperatorName() {
        String strOperator = m_strCurrOperator;
        // if operator is null which might be the case when no operators are
        // assigned to a owner,
        // then return the current user name
        if (strOperator == null && user != null)
            strOperator = user.getAccountName();

        return strOperator;
    }

    /**
     * Get the index of the array element that is nearest a given value. If two
     * elements are equally distant, returns the element with the smaller value.
     * 
     * @param table
     *            A monotonically increasing or decreasing array of values.
     * @param value
     *            The value we're looking for in the table.
     */
    public static int getNearest(float[] table, double value) {
        int n = table.length;
        boolean ascending = table[n-1] > table[0];
        int iLower = -1;
        int iUpper = n;

        while (iUpper - iLower > 1) {
            int mid = (iUpper + iLower) / 2;
            if ((value > table[mid]) == ascending) {
                iLower = mid;
            }else {
                iUpper = mid;
            }
        }
        if (iLower < 0) {
            return 0;
        } else if (iUpper == n) {
            return n - 1;
        } else {
            double d1 = value - table[iLower];
            double d2 = table[iUpper] - value;
            return (d1 > d2 == ascending) ? iUpper : iLower;
        }
    }

    public static boolean isLanguageSupported(String language, String country) {
	String str = "PROPERTIES/labelResources";
	if(language != null && language.length() > 0)
	    str += "_"+language;
	else return false;

	if(country != null && country.length() > 0)
	    str += "_"+country;

	str += ".properties";
	if(FileUtil.openPath(str) == null) return false;
	else return true;
    }

    public static void switchLanguage(String language, String country) {
    // language can be ja for Japanese, or zh for Chinese
	if(!isLanguageSupported(language, country)) {
	   Messages.postError("Language "+language + " is not supported.");
	   return;
	}
	if(FileUtil.getLanguage().equals(language)) return;

	suspendUI();
	FileUtil.setLanguage(language, country);

	vjLabels = null;
	vjAdmLabels = null;
	vjOptions = null;

	paramResource = null;
	cmdResource = null;
	tooltipResource = null;
	messageResource = null;
	labelResource = null;
	labelResource2 = null;

        labels = ResourceBundle.getBundle("vnmr.properties.Labels");
        admlabels = ResourceBundle.getBundle("vnmr.properties.AdmLabels");
        options = ResourceBundle.getBundle("vnmr.properties.Options");
	
//	ProtocolLabelHandler.updateProtocolLabelHandler();
	rebuildUI();
    }

    public static void suspendUI() {
    // do suspend
        if (appIf != null) {
            appIf.sendToVnmr("appdir('send')");
            // appIf.suspendUI();
        }
    }

    public static void rebuildUI() {
        if (mainFrame != null && (mainFrame instanceof VNMRFrame))
           ((VNMRFrame)mainFrame).rebuildApp();
        ParamInfo.restoreEditMode();
    }

    public static void setAppDirs(ArrayList<String> appDirs, 
                                  ArrayList<String> labels, boolean rebuild) {

        if (DebugOutput.isSetFor("appdirs"))
            Messages.postDebug("Util.setAppDirs called");
        
	if(user == null) return;

	for(int i=0; i<appDirs.size(); i++) 
	    Messages.postDebug("### appDir "+i+" "+(String)appDirs.get(i));
	
        if(user.setAppDirs(appDirs, labels) && rebuild) {
            Messages.postDebug("### rebuildUI");
                rebuildUI();
        }

        if (appIf != null)
            appIf.resumeUI();
    }

    //##################  JComponant color utities #####################

    /**
     * Set the background in the specified container and all its children,
     * down to the last generation.
     * @param container The container to set the background of.
     * @param bg The background color to set.
     */
    public static void setAllBackgrounds(Container container, Color bg) {
        container.setBackground(bg);
        Component[] comps = container.getComponents();
        for (Component comp : comps) {
            if (comp instanceof Container) {
                setAllBackgrounds((Container)comp, bg);
            }
        }
    }
  
    /**
     * Construct a Color that is suitable as a highlighted background
     * given the un-highlighted foreground and background colors.
     * @param fg The un-highlighted foreground color.
     * @param bg The un-highlighted background color.
     * @return The highlighted background color.
     */
    public static Color[] getSelectFgBg(Color fg, Color bg) {
        Color[] fgbg = {bg, fg}; // Return inverted fg/bg for now
        return fgbg;
    }

    /**
     * Get the background color of a component's parent.
     * If the parent's background is null, goes up the container
     * heirarchy until a non-null background is found.
     */
    public static Color getParentBackground(Component comp) {
        Color bkg = null;
        Container parent = comp.getParent();
        if (parent != null) {
            bkg = parent.getBackground();
        }
        return bkg;
    }

    //##################  UI color utities #####################

    public static void setBgColor(Color c){
        vjBg=c;
    }
    public static Color getBgColor() {
        return vjBg;
    }

    public static void setButtonBgColor(Color c){
        vjButtonBg = c;
    }

    public static void setListBgColor(Color c){
        vjListBg = c;
    }

    public static Color getButtonBgColor() {
        return vjButtonBg;
    }

    public static Color getListBgColor() {
        return vjListBg;
    }

    public static void setListSelectBg(Color c){
        vjListSelectBg = c;
    }

    public static Color getListSelectBg() {
        return vjListSelectBg;
    }

    public static void setListSelectFg(Color c){
        vjListSelectFg = c;
    }

    public static Color getListSelectFg() {
        return vjListSelectFg;
    }

    public static void setMenuBarBg(Color c){
        menuBarBg=c;
    }

    public static Color getMenuBarBg() {
         return menuBarBg;
    }

    public static void setToolBarBg(Color c){
        toolBarBg=c;
    }

    public static Color getToolBarBg() {
         return toolBarBg;
    }

    public static void setSeparatorBg(Color c){
        separatorBg=c;
    }

    public static Color getControlHighlight() {
        if (controlHighlight == null)
            controlHighlight = SystemColor.controlHighlight;
        return controlHighlight;
    }

    public static void setControlHighlight(Color c){
        controlHighlight=c;
    }

    public static Color getSeparatorBg() {
        return separatorBg;
    }

    public static void setActiveBg(Color c) {
        activeBg = c;
    }

    public static Color getActiveBg() {
        if (activeBg == null) {
            activeBg = SystemColor.activeCaption;
        }
        return activeBg;
    }

    public static void setActiveFg(Color c) {
        activeFg = c;
    }
    public static Color getActiveFg() {
        if (activeFg == null) {
            if (islinux())
                activeFg = SystemColor.textHighlightText;
            else
                activeFg = SystemColor.activeCaptionText;
        }
        return activeFg;
    }

    public static void setInactiveFg(Color c) {
        inactiveFg = c;
    }
    public static Color getInactiveFg() {
        if (inactiveFg == null) {
            inactiveFg = SystemColor.inactiveCaptionText;
        }
        return inactiveFg;
    }

    public static void setInactiveBg(Color c) {
        inactiveBg = c;
    }
    public static Color getInactiveBg() {
        if (inactiveBg == null) {
            inactiveBg = SystemColor.inactiveCaption;
        }
        return inactiveBg;
    }

    public static void setPanelFg(Color c) {
        panelFg = c;
    }
    public static Color getPanelFg() {
        if (panelFg == null)
            panelFg = SystemColor.windowText;
        return panelFg;
    }

    public static void setMenuBg(Color c) {
        menuBg = c;
    }
    public static Color getMenuBg() {
        if (menuBg == null)
            menuBg = SystemColor.menu;
        return menuBg;
    }

    public static void setMenuFg(Color c) {
        menuFg = c;
    }
    public static Color getMenuFg() {
        if (menuFg == null)
            menuFg = SystemColor.menuText;
        return menuFg;
    }

    public static void setSelectFg(Color c) {
        hilitFg = c;
    }
    public static Color getSelectFg() {
        if (hilitFg == null)
            hilitFg = SystemColor.textHighlightText;
        return hilitFg;
    }

    public static void setSelectBg(Color c) {
        hilitBg = c;
    }
    public static Color getSelectBg() {
        if (hilitBg == null)
            hilitBg = SystemColor.textHighlight;
        return hilitBg;
    }

    public static void setGridColor(Color c) {
        gridFg = c;
    }
    public static Color getGridColor() {
        if (gridFg == null)
            gridFg = SystemColor.controlHighlight;
        return gridFg;
    }

    public static Color getInputBg() {
         return inputBg;
    }

    public static void setInputBg(Color c) {
         inputBg = c;
    }

    public static Color getInputFg() {
         return inputFg;
    }

    public static void setInputFg(Color c) {
        inputFg = c;
    }

    public static Border entryBorder() {
        Color bg=Util.getBgColor();
        Color shadow=Util.darken(bg, 0.5);
        Color highlight=Util.brighten(bg, 0.2);
        return BorderFactory.createBevelBorder(BevelBorder.LOWERED, highlight, shadow);
    }

    //##################  Value display utities #####################

    /**
     * Reformats a number string to "f" format with a given number
     * of fraction digits.
     * @param value The number to format
     * @param precision The desired number of digits after the decimal
     * @return The reformatted String
     */
    public static String setPrecision(String value, String precision) {
        if (value == null || precision == null) {
            return value;
        }
        String rtn = value;
        try {
            double dValue = Double.parseDouble(value);
            int nPrecision = Integer.parseInt(precision);
            NumberFormat nf = NumberFormat.getNumberInstance();
            nf.setMaximumFractionDigits(nPrecision);
            nf.setMinimumFractionDigits(nPrecision);
            rtn = nf.format(dValue);
        } catch (NumberFormatException nfe) {}
        return rtn;
    }

    /**
     * Gets a standard, easily machine readable, representation of
     * the current date and time.
     */
    public static String getStandardDateTimeString() {
        return getStandardDateTimeString(System.currentTimeMillis());
    }

    /**
     * Gets a standard, easily machine readable, representation of
     * the given date and time.
     */
    public static String getStandardDateTimeString(long time) {
        if (m_dateFormat == null) {
            m_dateFormat = new SimpleDateFormat();
        }
        m_dateFormat.applyPattern("yyyy-MM-dd H:mm:ss.S");
        return m_dateFormat.format(new Date(time));
    }

    /**
     * Get short day string for current locale  
     * @param   short day string in default locale (en_US)  (e.g. Mon, Tue)
     * @return  short day string in language of current locale
     */
    public static String getShortWeekday(String day) { 
        DateFormatSymbols symbols;
        String[] defaultDays;
        String[] Days;

        symbols = new DateFormatSymbols(new Locale("en","US")); // for default locale
        defaultDays = symbols.getShortWeekdays();
        symbols=new DateFormatSymbols(); // for current locale
        Days=symbols.getShortWeekdays();
        int i=0;
        for (i = 0; i < defaultDays.length; i++) {
            if(day.equals(defaultDays[i]))
                 return Days[i];
        }
        return day; // not found
    }
    /**
     * Get short day string for default locale (us_EN) 
     * @param   day short day string in language of current locale 
     * @return  short day string in default locale (en_US)
     */
    public static String getDfltShortWeekday(String day) { 
        DateFormatSymbols symbols;
        String[] defaultDays;
        String[] Days;
        symbols = new DateFormatSymbols(new Locale("en","US")); // for default locale
        defaultDays = symbols.getShortWeekdays();
        symbols=new DateFormatSymbols(); // for current locale
        Days=symbols.getShortWeekdays();
        int i=0;
        for (i = 0; i < Days.length; i++) {
            if(day.equals(Days[i]))
                 return defaultDays[i];
        }
        return day; // not found 
    }

    /**
     * Puts an escape character ("\") in front any of the specified
     * special characters in the given string.
     * @param str The string to operate on.
     * @param targets The list of "special" characters to excape.
     * @return The string with escapes added.
     */
    public static String escape(String str, String targets) {
        int targetLen = targets.length();
        StringBuffer sbData = new StringBuffer().append(".*[").append(targets).
                                   append("].*");
        if (str == null
            || str.length() == 0
            || (targetLen <= 1 && str.indexOf(targets) < 0)
            || (targetLen > 1 && !str.matches(sbData.toString())))
        {
            return str;         // No targets in this string
        }

        for (int i = 0; i < targetLen; i++) {
            String target = targets.substring(i, i + 1);
            String replace =new StringBuffer().append( "\\\\" ).append( target).toString();
            str = str.replaceAll(target, replace);
        }
        return str;
    }

    /**
     * Mix two colors by a given amount  
     */
    public static Color mix(Color c1, Color c2, double f) {
        if(f>=1.0)
            return c2;

        if(f<=0)
            return c1;

        double r1 = c1.getRed();
        double g1 = c1.getGreen();
        double b1 = c1.getBlue();
        double r2 = c2.getRed();
        double g2 = c2.getGreen();
        double b2 = c2.getBlue();
        double r=f*r2+(1-f)*r1;
        double g=f*g2+(1-f)*g1;
        double b=f*b2+(1-f)*b1;
        
        Color c3=new Color(Math.min((int)(r), 255),
                Math.min((int)(g), 255),
                Math.min((int)(b), 255));
       
        return c3;
    }

    /**
     * Brighten a color by a given amount  
     */
    public static Color brighten(Color c, double f) {
        return mix(c,Color.white, f);
    }

    /**
     * Darken a color by a given amount  
     */
    public static Color darken(Color c, double f) {
        return mix(c,Color.black,f);
    }


    /**
     * Return either BLACK or WHITE, whichever contrasts with the given color.
     */
    public static Color getContrastingColor(Color c) {
        if (c == null) {
            return Color.BLACK;
        }
        float f[] = new float[3];
        c.getColorComponents(f);
        double r = f[0];
        double g = 1.1 *f [1];
        double b = f[2];
        double mag = (r * r + g * g + b * b);
        return (mag <= 1 ? Color.WHITE : Color.BLACK);
    }

    /**
     * Construct a Color that is a brighter or darker version of a given
     * color.
     * Positive percent changes make the color brighter, unless it
     * is already as bright as it can get without changing the hue.
     * A percent change of -100 will always give black.
     * @param c The original color.
     * @param percent The desired percentage change in brightness.
     * @return The new color.
     */
    public static Color changeBrightness(Color c, int percent) {
        if (c == null) {
            return c;
        }
        int r = c.getRed();
        int g = c.getGreen();
        int b = c.getBlue();
        float[] hsb = Color.RGBtoHSB(r, g, b, null);
        float fraction = (float)percent / 100;
        float remainder = (float)0;

        hsb[2] = hsb[2] * (1 + fraction);
        if (hsb[2] < 0) {
            remainder = hsb[2];
            hsb[2] = 0;
        } else if (hsb[2] > 1) {
            // Can't make it bright enough with Brightness alone,
            // decrease Saturation.
            remainder = hsb[2] - 1;
            hsb[2] = 1;
            hsb[1] = hsb[1] * (1 - 5 * remainder);
            if (hsb[1] < 0) {
                hsb[1] = 0;
            }
        }
        return Color.getHSBColor(hsb[0], hsb[1], hsb[2]);
    }

    /**
     * Construct a Color that is a brighter or darker version of a given
     * color. The absolute value of the "percent" parameter determines
     * the amount of change, the direction (brighter or darker) depends
     * on whether the color "c" is nearer black or white.
     * It's made darker if it's bright and brighter if it's dark,
     * regardless of the sign of "percent".
     * @param c The original color.
     * @param percent The desired percentage change in brightness.
     * @return The new color.
     */
    public static Color changeAbsBrightness(Color c, int percent) {
        if (c == null) {
            return c;
        }
        int r = c.getRed();
        int g = c.getGreen();
        int b = c.getBlue();
        float[] hsb = Color.RGBtoHSB(r, g, b, null);

        percent = Math.abs(percent);
        if (hsb[2] > 0.5) {
            percent = -percent;
        }
        return changeBrightness(c, percent);
    }

    public static String getLabel4String2(String str) {
	if(labelResource2 == null) labelResource2 = new VLabelResource("labelResources2");

	if(labelResource2 == null) return str;
	else return labelResource2.getString(str);
    }

    public static String getLabel4String(String str) {
	if(labelResource == null) labelResource = new VLabelResource("labelResources");

	if(labelResource == null) return getLabel4String2(str);
	else {
	   String label = labelResource.getString(str);
	   if(!label.equals(str)) return label;
	   else return getLabel4String2(str); 
	}
    }

    public static String getLabelString(String str) {
	// return if str is a file path
	if(str.indexOf(File.separator) == 0) {
	  return(str);
	}

        str=str.trim();
        String label = getLabel4String(str);

	if(captureResource) { 
	   addLabel2List(str,label,allLabels, allLabels2);
	}

	boolean en = (FileUtil.getLanguage().equals("en"));
        if(en || !label.equalsIgnoreCase(str)) return label; // found
	
	// call getLabel4String for each substrings separated by (), etc...
        if(str.indexOf("(") >= 0 && str.indexOf(")") >= 0) 
	  return getLabelForTokens(str, "()");
        if(str.indexOf("[") >= 0 && str.indexOf("]") >= 0) 
	  return getLabelForTokens(str, "[]");
        if(str.indexOf(",") > 0) 
	  return getLabelForTokens(str, ",");
        if(str.indexOf(".") > 0) 
	  return getLabelForTokens(str, ".");
        if(str.indexOf(":") > 0) 
	  return getLabelForTokens(str, ":");
        if(str.indexOf("=") > 0) 
	  return getLabelForTokens(str, "=");
        if(str.indexOf("/") > 0) 
	  return getLabelForTokens(str, "/");
        if(str.indexOf("_") > 0) 
	  return getLabelForTokens(str, "_");
        if(str.indexOf("-") > 0) 
	  return getLabelForTokens(str, "-");
        else // call getLabel4String for each word. 
    	  return getLabelForWords(str);
    }
  
    public static String getLabelForTokens(String str, String delim) {
 
        StringTokenizer tok = new StringTokenizer(str, delim);
        if(tok.countTokens() == 0) return getLabelForWords(str);

	   StringBuffer newStr = new StringBuffer();
	   int count = 0; // this is to count position of delimiters
	   if(str.indexOf("(") == 0) {
	     newStr.append("(");
	     count++;
	   }
	   if(str.indexOf(")") == 0) {
	     newStr.append(")");
	     count++;
	   }
	   String newTok;
	   String newLabel;
	   while(tok.hasMoreTokens()) {
	     newTok = tok.nextToken();
	     count = count + newTok.length();
	     newLabel = getLabel4String(newTok.trim());
	     // if substring does not match, try each word.
	     if(newLabel.equals(newTok.trim())) newLabel = getLabelForWords(newTok);
	     else if(captureResource) {
	   	  addLabel2List(newTok,newLabel,allLabels,  allLabels2);
	     }
	     newStr.append(newLabel);
	     if(count < str.length()) { // append delimiter.
		newStr.append(str.charAt(count));
	        count++;
	     }
	   }
	   return newStr.toString(); 
    }

    public static String getLabelForWords(String str) {

	// call getLabel4String for each word if the # of words is less than maxToks.
        int maxToks = 12;
        StringTokenizer tok = new StringTokenizer(str, " \t\r\n");
        if(tok.countTokens() <= 0 || tok.countTokens() > maxToks) return str; 

	// we cannot use tokens because 
          char[] cArray = str.toCharArray();
          StringBuffer word = new StringBuffer();
          StringBuffer newStr = new StringBuffer();
          String newstrstr;
          String strstr;
          int i=0;
          while(i<cArray.length) {
	   if(!Character.isLetter(cArray[i]) && !Character.isDigit(cArray[i])) {
              if(word.length() > 1) {
		strstr = word.toString();
	 	newstrstr = getLabel4String(strstr);
	        newStr.append(newstrstr).append(cArray[i]);
	        word = new StringBuffer();

        	if(captureResource && !newstrstr.equalsIgnoreCase(strstr)) {
	   	  addLabel2List(strstr,newstrstr,allLabels,  allLabels2);
        	} 
	   
              } else if(word.length() > 0) {
	     	newStr.append(word.toString()).append(cArray[i]); 
	        word = new StringBuffer();
              } else {
	        newStr.append(cArray[i]);
	      }
           } else {
              word.append(cArray[i]);
	   }
           i++;
	  }
          if(word.length() > 1) {
		strstr = word.toString();
	 	newstrstr = getLabel4String(strstr);
	        newStr.append(newstrstr);

        	if(captureResource && !newstrstr.equalsIgnoreCase(strstr)) {
	   	  addLabel2List(strstr,newstrstr,allLabels,  allLabels2);
        	} 

          } else if(word.length() > 0) {
	     	newStr.append(word.toString()); 
	  }
	  return newStr.toString(); 
    }

    // list1 is for all labels
    // list2 is for not translated labels.
    private static void addLabel2List(String str, String newStr, SortedSet<String> list1, SortedSet<String> list2) {
       if(!captureResource) return;
       if(str == null || str.length() < 1) return; 
       if(list1.size() < 1) readLabelLists();

       try {
	 Integer.parseInt(str);
       } catch (NumberFormatException ex) {
	   StringBuffer strbuf = new StringBuffer().append(str).append('=');
           String strstr;
           if(!newStr.equals(str)) { // translated
                strbuf.append(newStr);
                strstr = strbuf.toString();
		if(list1 != null && !list1.contains(strstr)) {
		   list1.add(strstr);
                   System.out.println("Translated Label "+strstr);
		}
           } else { // not translated
                strstr = strbuf.toString();
                if(list2 != null && !list2.contains(strstr)) {
		   list2.add(strstr);
		   System.out.println("Not translated Label "+strstr);
		}
	   }
       } 
    }

    private static void readLabelList(SortedSet<String> labelList, String path) {
	path = FileUtil.openPath(path);
        if(path==null) return;

        String line;
        BufferedReader in;

        try {
	    in = new BufferedReader(new FileReader(path));
	    while ((line = in.readLine()) != null) {
		if(line.length()>0 && !labelList.contains(line)) labelList.add(line);
            }
	    in.close();
	} catch(IOException er) {
            Messages.postError("writeLabelList: cannot read "+path);
        }
    }

    // called by setCaptureResource to pre-load label lists.
    // Note, the value of captureResource is a dir name to save the lists.
    public static void readLabelLists() {

        if(!captureResource) return;
	String dir = FileUtil.usrdir()+"/templates/vnmrj/properties";
	String type = System.getProperty("captureResource");
	dir = dir + "/" + type;
	String str = "_"+FileUtil.getLanguage()+"_"+FileUtil.getCountry();

	readLabelList(allLabels,dir+"/labelResources"+str+".properties"); 
	readLabelList(allVJLabels,dir+"/vjLabels"+str+".properties"); 
	readLabelList(allVJAdmLabels,dir+"/vjAdmLabels"+str+".properties"); 
	readLabelList(allLabels2,dir+"/labelResources2"+str+".properties"); 
	readLabelList(allVJLabels2,dir+"/vjLabels2"+str+".properties"); 
	readLabelList(allVJAdmLabels2,dir+"/vjAdmLabels2"+str+".properties"); 
    }

    private static void writeLabelList(SortedSet<String> labelList, String path) {
	if(labelList.size() < 1) return;
	path = FileUtil.savePath(path);
        if(path==null) return;

        PrintWriter os;
        try {
	    os = new PrintWriter(new FileWriter(path));
	    for (String value : labelList) {
                os.println(value);
            }
	    os.close();
	} catch(IOException er) {
            Messages.postError("writeLabelList: cannot write "+path);
        }
    }

    // called by exit to save resource files.
    public static void saveLabelLists() {
        if(!captureResource) return;
	String dir = FileUtil.usrdir()+"/templates/vnmrj/properties";
	String type = System.getProperty("captureResource");
	dir = dir + "/" + type;
	String str = "_"+FileUtil.getLanguage()+"_"+FileUtil.getCountry();

	writeLabelList(allLabels,dir+"/labelResources"+str+".properties"); 
	writeLabelList(allVJLabels,dir+"/vjLabels"+str+".properties"); 
	writeLabelList(allVJAdmLabels,dir+"/vjAdmLabels"+str+".properties"); 
	writeLabelList(allLabels2,dir+"/labelResources2"+str+".properties"); 
	writeLabelList(allVJLabels2,dir+"/vjLabels2"+str+".properties"); 
	writeLabelList(allVJAdmLabels2,dir+"/vjAdmLabels2"+str+".properties"); 
    }

    public static boolean setCaptureResource() {
        boolean b = false;
        String str = System.getProperty("captureResource");
        if(str != null && str.length()>0) {
	  b = true;
	}
        return b;
    }

    private static void readLabelResource(Hashtable<String,String> labelTable, String path) {

	path = FileUtil.openPath(path);
        if(path==null) return;

        String line;
        BufferedReader in;

        try {
	    in = new BufferedReader(new FileReader(path));
	    while ((line = in.readLine()) != null) {
		if(line.length()>0 && !line.startsWith("#")) {
		  int ind = line.indexOf("=");
	 	  if(ind > 0) {
		   String key = line.substring(0,ind).trim();
		   String value = line.substring(ind+1).trim();
		   if(value.length()<1) value=key;
		   if(value.indexOf("=") < 0) labelTable.put(key,value);
		  }
		}
            }
	    in.close();
	} catch(IOException er) {
            Messages.postError("readLabelList: cannot read "+path);
        }
    }

    // file1 and file2 are fullpath
    // file2 will be overwritten  
    // if an entry exists in both file1 and file2, file1 will overwrite file2
    public static void mergeResourceFiles(String file1, String file2, String option) {
       // option='all', file2 will contain all unique entries. This is default. 
       // option='translated', file2 will contain only translated entries 
       // option='notTranslated', file2 will contain only not translated entries 
       // option='list', file2 will contain only keys, not values.
       Hashtable<String,String> newTable = new Hashtable<String,String>();
       readLabelResource(newTable,file2); 
       readLabelResource(newTable,file1); 
       SortedSet<String> newList = new TreeSet<String>(alphaOrder);
       for(Enumeration en = newTable.keys(); en.hasMoreElements(); ) {
          String key = (String) en.nextElement();
	  String value = (String)newTable.get(key);
	  if(option.equals("translated") && (value.length()<1 || key.equals(value)))
	   continue;
	  if(option.equals("notTranslated") && (value.length()>0 && !key.equals(value)))
	   continue;
	  if(option.equals("list")) value="";
	  StringBuffer str = new StringBuffer().append(key).append('=').append(value);
	  newList.add(str.toString());
       }
       writeLabelList(newList,file2); 
    }
   
    public static void writeResourceFiles(String path) {
        String language = Locale.getDefault().getLanguage();
        String country = Locale.getDefault().getCountry();
        String variant = Locale.getDefault().getVariant();
	String ext;
	if(country !=null && country.length() > 0 &&
           variant !=null && variant.length() > 0) {
            ext = "_"+language+"_"+country+"_"+variant+".properties";
	} else if(country !=null && country.length() > 0) {
            ext = "_"+language+"_"+country+".properties";
	} else {
            ext = "_"+language+".properties";
 	}
	String file = path+ext;
	if(labelResource != null) labelResource.writeResourceFile(file);
	file = path+"2"+ext;
	if(labelResource2 != null) labelResource2.writeResourceFile(file);
    }

    public static String getParamDescription(String str) {
	if(paramResource == null) paramResource = new VResourceBundle("paramResources");

	if(paramResource == null) return str;
	else return paramResource.getString(str);
    }

    public static String getcmdDescription(String str) {
	if(cmdResource == null) cmdResource = new VResourceBundle("cmdResources");

	if(cmdResource == null) return str;
	else return cmdResource.getString(str);
    }

    public static String getTooltipString(String str) {
	if(tooltipResource == null) tooltipResource = new VLabelResource("tooltipResources");

	if(tooltipResource == null) return str;
	else return tooltipResource.getString(str);
    }

    public static String getMessageString(String str) {
	if(messageResource == null) messageResource = new VLabelResource("messageResources");

	if(messageResource == null) return str;
	else return messageResource.getString(str);
    }

    public static String getMyLabel(String str) {
        if(str == null) return "";
	if(vjLabels == null) vjLabels = new VResourceBundle("vjLabels");

	if(vjLabels == null) return getMyAdmLabel(str);
	else {
	   String label = vjLabels.getString(str);
	   if(!label.equals(str)) return label;
	   else return getMyAdmLabel(str); 
	}
    }

    public static String getMyAdmLabel(String str) {
	if(vjAdmLabels == null) vjAdmLabels = new VResourceBundle("vjAdmLabels");

	if(vjAdmLabels == null) return str;
	else return vjAdmLabels.getString(str);
    }

    public static String getMyOption(String str) {
	if(vjOptions == null) vjOptions = new VResourceBundle("vjOptions");

	if(vjOptions == null) return str;
	else return vjOptions.getString(str);
    }

    public static void setTrashCan(TrashCan c) {
        trashCan = c;
    }

    public static TrashCan getTrashCan() {
        return trashCan;
    }

    public static void setColormapPanel(VColorMapPanel panel) {
        colormapPanel = panel;
    }

    public static VColorMapPanel getColormapPanel() {
        return colormapPanel;
    }

    public static void setArrowButton(JButton btn) {
         arrowButton = btn;
    }

    public static JButton getArrowButton() {
         return arrowButton;
    }

    public static void setHorBumpIcon(ImageIcon icon) {
          horBumpIcon = icon;
    }

    public static ImageIcon getHorBumpIcon() {
         return horBumpIcon;
    }

    public static void setVerBumpIcon(ImageIcon icon) {
          verBumpIcon = icon;
    }

    public static ImageIcon getVerBumpIcon() {
         return verBumpIcon;
    }

    public static void setAdminIf(boolean b) {
         bAdminIF = b;
    }

    public static boolean isAdminIf() {
        return bAdminIF;
    }

    public static void setMultiSq(boolean b) {
         bMultSq = b;
    }

    public static boolean isMultiSq() {
       return bMultSq;
    }


    // Get the list of all possible appdirs and append the protocol
    // directory relative path to each.
    public static HashMap<String,String> getAllPossibleAppProtocolDirsHash() {
        HashMap<String, String> appdirProtocolList = new HashMap();

        // Get all possible appdirs directories
        HashMap<String, String> appdirList = getAllPossibleAppDirsHash();
        
        Set set = appdirList.entrySet();
        Iterator iter = set.iterator();
        while(iter.hasNext()) {
            Map.Entry entry = (Map.Entry)iter.next();
            String path = (String) entry.getValue();
            String label = (String) entry.getKey();
            // Append the relative protocols path to each
            path = path + File.separator + FileUtil.getSymbolicPath("PROTOCOLS");
            appdirProtocolList.put(label, path);
        }

        return appdirProtocolList;
    }

    public static ArrayList getAllPossibleAppDirs() {
        HashMap<String, String> dirsNlabels = getAllPossibleAppDirsHash();
        ArrayList<String> appDirs = new ArrayList<String>();


        Set set = dirsNlabels.entrySet();
        Iterator iter = set.iterator();
        while(iter.hasNext()) {
            Map.Entry entry = (Map.Entry)iter.next();
            String path = (String) entry.getValue();

            appDirs.add(path);
        } 
        return appDirs;

    }

    public static ArrayList getAllPossibleAppDirLabels() {
        HashMap<String, String> dirsNlabels = getAllPossibleAppDirsHash();
        ArrayList<String> labels = new ArrayList<String>();

        Set set = dirsNlabels.entrySet();
        Iterator iter = set.iterator();
        while(iter.hasNext()) {
            Map.Entry entry = (Map.Entry)iter.next();
            String label = (String) entry.getKey();

            labels.add(label);
        } 
        return labels;

    }

    
    static HashMap<String, String> appdirList = null;

   // Parse the /vnmr/adm/users/userProfiles/appdir*.txt  files
   // to get all possible appdirs for admin use.
   public static HashMap<String,String> getAllPossibleAppDirsHash() {
       // If it has already been filled, just return the list
       if(appdirList != null)
           return appdirList;

        appdirList = new HashMap<String, String>();
        String strLine;
        String appdir, appdirLabel;
        BufferedReader reader;
        StringTokenizer tok;

        
        // Get the directory where the files are located
        String dirpath = FileUtil.openPath("SYSTEM/USRS/userProfiles");
        // Be sure it exists
        if(dirpath == null) {
            Messages.postLog(dirpath + " Not Found.  Using default appdirs "
                             + "list.");
            appdirList.put("Varian System", "/vnmr");
            appdirList.put(Global.WALKUPIF, "/vnmr");
            appdirList.put(Global.IMGIF, "/vnmr/imaging");
        }
        else {
            // Get the file list for all appdir*.txt files in this directory
            UNFile file = new UNFile(dirpath);
            File[] files = file.listFiles(AppdirFileFilter.filter);
            if(files != null) {
                for (int i = 0; i < files.length; i++) {
                    // Open and parse each file
                    try {
                        reader = new BufferedReader(new FileReader(files[i]));

                        // Go through all lines in the file
                        while ((strLine = reader.readLine()) != null) {
                            // Skip comment lines
                            if (strLine.startsWith("#"))
                                continue;
                            tok = new StringTokenizer(strLine, ";");
                            if (tok.countTokens() >= 3) {
                                // Skip first token
                                tok.nextToken();
                                appdir = tok.nextToken().trim();
                                appdirLabel = tok.nextToken().trim();
                                // If it is not already in the list, add it
                                // If autotest or USERDIR, skip it
                                if (!appdirList.containsValue(appdir) && 
                                        !appdir.endsWith("autotest") &&
                                        !appdir.equals("USERDIR")) {
                                    appdirList.put(new String(appdirLabel), 
                                            new String(appdir));
                                }
                            }
                        }
                        if(reader != null)
                            reader.close();
                    }
                    catch (Exception ex) {
                        Messages.postWarning("Problem reading "
                                + files[i].getAbsolutePath());
                        continue;
                    }

                }
            }
        }
        int size = appdirList.size();
        return appdirList;

     
    }

    // FilenameFilter to exclude hidden files, ie., starting with '.'
    static public class AppdirFileFilter implements FilenameFilter {

        static AppdirFileFilter filter = new Util.AppdirFileFilter();
        
        public boolean accept(File parentDir, String name) {
            if(name.startsWith("appdir") && name.endsWith(".txt"))
                return true;
            else
                return false;
            
        }
    }


} // class Util

