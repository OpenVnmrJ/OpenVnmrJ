/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import javax.swing.border.*;
import javax.swing.tree.*;
import javax.swing.event.*;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import java.net.*;
import javax.swing.filechooser.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.bo.*;

/********************************************************** <pre>
 * Summary: File Browser
 *
 * Panel with JFileChooser for Open and SaveAs function
 </pre> **********************************************************/
public class VJFileBrowser extends JPanel implements ActionListener 
{
    protected String root = "";
    public  boolean overwriteOkay= false;
    private  String defaultLookAndFeel=null;

    protected  VCustomFileChooser filechooser = null;
    
    private JTextArea errorItem=null;

    protected   String localHost;
    private SessionShare sshare;
    private Border  m_raisedBorder = 
                                     BorderFactory.createRaisedBevelBorder();

    String userDir = System.getProperty("userdir");
    // Need a way to specify this be whereever someone wants
    // especially for operators.
    String dataHome = null;
    // Need a way to specify this.  probably a 'right'
    // When allowUpFolder=false, the 'up folder' or 'up arrow' is removed
    // so that the user cannot 'go up'.
    boolean allowUpFolder = true;
    JTextField dirLabel=null;
    JPanel topPanel=null;
    JButton upFolderButton=null;
    JLabel filesOfTypeLabel=null;
    String filesOfTypeLabelText;
    JComboBox directoryComboBox;

    // List of path string for programmable buttons (String)
    protected ArrayList btnDirStr;
    // List of button item for programmable buttons (VToolBarButton)
    protected ArrayList btnDir;
    protected ArrayList btnLabels;
    // List of button filter type for each button (String)
    protected ArrayList btnFilterType;
    // If null, we don't want a cancel button, and don't unshow anything.
    private Component parent=null;
    private Hashtable hsTable = null;
    String type;
    javax.swing.filechooser.FileFilter fidFilter;

    // Strings to use in menu of file filters
    public static final String FILTER_NAME_FID = ".fid";
    public static final String FILTER_NAME_PAR = ".par";
    public static final String FILTER_NAME_STUDY = "study";
    public static final String FILTER_NAME_SHIM = "shim";
    public static final String FILTER_NAME_FDF = ".fdf";
    public static final String FILTER_NAME_IMG = ".img";
    public static final String FILTER_NAME_VFS = ".vfs";
    public static final String FILTER_NAME_CRFT = ".crft";
    public static final String FILTER_NAME_PROTOCOL = "protocol";
    public static final String FILTER_NAME_AUTODIR = "automation";
    public static final String FILTER_NAME_RECORD = "record";
    public static final String FILTER_NAME_PANELSNCOMPONENTS = 
                                                       ".xml";
    public static final String FILTER_NAME_WORKSPACE = "workspace";
    public static final String FILTER_NAME_LCSTUDY = "lcstudy";
    public static final String FILTER_NAME_MOLECULE = "molecule";
    public static final String FILTER_NAME_ICON = "icon/image-file";
    public static final String HASH_BUTTON_NAME = "browserBtn";

    private static final int numBtns = 4;


    // Called without type, default to Browser panel.  This should be 
    // a call from the vertical panel area
    public VJFileBrowser(SessionShare sshare) {
        this(sshare, "VBrowser"); // Vertical panel Browser
        // Set the panel in ExpPanel so that when a command comes, it will
        // know what panel to act on.
        ExpPanel.setBrowserPanel(this);
    }

    public VJFileBrowser(SessionShare sshare, String type) {
        // If called with a parent, use this Component to close the panel.
        // That is, when this obj is created from VJOpenSavePopup, and
        // the user click cancel or something, we need to close the parent.
        // When called from the pushpin area, we do not want a cancel button    
        // at all and we do not want to unshow our parent.
        this(sshare, type, null);
    }

        

    /************************************************** <pre>
     * Summary: Constructor, Add label and text fields to dialog box
     *
     </pre> **************************************************/
    public VJFileBrowser(SessionShare sshare, String type, Component parent) {

        this.parent = parent;
        this.sshare = sshare;
        this.type = type;

        // set dataHome based on the first dir in this User's dataDirectories
        // This will need to be modified to deal with operators.
        String curuser = System.getProperty("user.name");
        LoginService loginService = LoginService.getDefault();
        Hashtable userHash = loginService.getuserHash();
        User user = (User)userHash.get(curuser);
        dataHome = new String((String)user.getDataDirectories().get(0));
        root = dataHome;
        
        setLookAndFeel("metal");

        setLayout(new BorderLayout());

        // Fill a rightsList for determining the status of allowUpFolder
        boolean all = false;
        RightsList rightsList = new RightsList(all);
        
        // if approve is false for Can browse Anywhere, then limitUpfolder 
        // is true.
        if(rightsList.approveIsFalse("canbrowseanywhere"))
            allowUpFolder = false;
        else
            allowUpFolder = true;

        retrieveButtonLabels();

        try {

            InetAddress inetAddress = InetAddress.getLocalHost();
            localHost = inetAddress.getHostName();

            setOpaque(true);
            setLayout(new BorderLayout());

            // create the error area
            errorItem = new JTextArea("");
            errorItem.setBackground(Color.RED);

            // Create the modified JFileChooser
            filechooser = createFilechooser(type);

            

            // Set to show both files and directories since things like
            // .fid data are actually directories
            filechooser.setCurrentDirectory(new File(root));

            // Create an object for fid so it can be added first and then
            // be made the default below.
            fidFilter= new fidFileFilter();

            // The type passed in is language dependent
            if(type.equals(Util.getLabel("blOpen")) || type.equals("Browser") || 
                                                       type.equals("VBrowser")) {
                // This is required for JFileChooser objects.
                filechooser.setDragEnabled(true);
                filechooser.setDialogType(JFileChooser.OPEN_DIALOG);

                createOpenFileFilterList();
            }
            else {  // Save Panel
                filechooser.setDialogType(JFileChooser.SAVE_DIALOG);

                createSaveFileFilterList();
            }
            // Set .fid File filter as the default
            filechooser.setFileFilter(fidFilter);

            add(filechooser, BorderLayout.CENTER);

            JPanel upperPanel = new JPanel(new BorderLayout());
            JPanel dirMenuPanel = createDataDirMenuPanel();
            upperPanel.add(dirMenuPanel, BorderLayout.NORTH);

            JPanel progBtnPanel = createProgrammableBtns();
            upperPanel.add(progBtnPanel, BorderLayout.SOUTH);

            add(upperPanel, BorderLayout.NORTH);

            // Set the background of the 'this' panel to be the same as
            // the topPanel.
            //Color bkg = topPanel.getBackground();
            //setBackground(bkg);
            
            readPersistence(type);
            
            updateTree(root);

        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
        // Put the look and feel back
        setLookAndFeel(defaultLookAndFeel);
    }

    public void rescanCurrentDirectory() {
        filechooser.rescanCurrentDirectory();
    }

    protected class GoHomeAction extends AbstractAction {
        protected GoHomeAction() {
            super("Go Home");
        }

        public void actionPerformed(ActionEvent e) {
            updateTree(dataHome);
        }
    }

    protected class upDirAction extends AbstractAction {
        protected upDirAction() {
            super("Up Dir");
        }

        public void actionPerformed(ActionEvent e) {
            filechooser.changeToParentDirectory();

            File file = filechooser.getCurrentDirectory();
            String dir = file.getAbsolutePath();

            updateDirLabel(dir);
        }
    }
    
    
    public void destroyPanel(String type) {
        try {
            writePersistence(type);
            setVisible(false);
        }
        catch (Throwable e) {
        }
 
    
    }

    private void retrieveButtonLabels() {
        int i;
        if (btnLabels == null) {
            btnLabels = new ArrayList(numBtns);
            for (i = 0; i < numBtns; i++)
               btnLabels.add(i, "");
        }
        sshare = ResultTable.getSshare();
        if (sshare == null)
            return;
        hsTable = sshare.userInfo();
        if (hsTable == null)
            return;
        for (i = 0; i < numBtns; i++) {
            String key = HASH_BUTTON_NAME+i;
            String v = (String) hsTable.get(key); 
            if (v != null && v.length() > 0) {
               btnLabels.set(i, v);
            }
        }
    }

    private void saveButtonLabels() {
        if (btnLabels == null || hsTable == null)
            return;
        try {
            for (int i = 0; i < numBtns; i++) {
                String v = (String) btnLabels.get(i);
                String key = HASH_BUTTON_NAME+i;
                if (v != null)
                    hsTable.put(key, v);
                else
                    hsTable.put(key, "");
            }
        }
        catch (Exception e) { }
    }

    private void saveButtonLabel(int id, String str) {
        if (id >= numBtns)
            return;
        if (btnLabels == null || hsTable == null) {
            retrieveButtonLabels();
            if (hsTable == null)
                return;
        }
        String key = HASH_BUTTON_NAME+id;
        if (str == null)
            str = "";
        btnLabels.set(id, str);
        hsTable.put(key, str);
    }

    private void setButtonLabel(int id, String str) {
        try {
            VToolBarButton btn = (VToolBarButton)btnDir.get(id);
            if (btn == null)
               return;
            if (btnLabels == null)
                retrieveButtonLabels();
            String vlabel = null;
            if (id < numBtns && !str.startsWith("Dir")) {
                vlabel = (String) btnLabels.get(id);
                if (vlabel != null && vlabel.length() > 0)
                    str = vlabel;
            }
            btn.setAttribute(VObjDef.ICON, str);
        }
        catch (Exception e) { }
    }

    /*
     * We need the displayed tree to be only '1' deep. If we were to have a
     * tree with multiple levels opened and multiple items opened, we would
     * not know which one to use for the locator filter. I did not see a
     * way to specify a tree to do this, so, when a directory is opened, we
     * will re-create the tree at that level and redisplay.
     */
       
    public void updateTree(String newRoot) {
        if(newRoot == null || newRoot.length() < 1)
            return;    

        File file = new File(UtilB.unixPathToWindows(newRoot));
        filechooser.setCurrentDirectory(file);
        root = newRoot;

        updateDirLabel(root);

        // Clear any error condition
         clearErrorText();
    }


    /* Read the directory for where to start the browser. */
    public void readPersistence(String type) {
        String filepath;

        // If parent == null, then we have a browser, else open/save
        if(parent == null || type.equals("Browser"))
            filepath = FileUtil.openPath("USER/PERSISTENCE/"
                                            + "FileBrowserDir");
        else if(type.equals(Util.getLabel("blOpen")))
            filepath = FileUtil.openPath("USER/PERSISTENCE/"
                                            + "OpenFileBrowserDir");
        else if(type.equals(Util.getLabel("blSave")))
            filepath = FileUtil.openPath("USER/PERSISTENCE/"
                                            + "SaveFileBrowserDir");
        else
            return;
 
        if(filepath != null) {
            BufferedReader in;
            String line;
            try {
                File file = new File(filepath);
                in = new BufferedReader(new FileReader(file));
                // File must start with 'Browser's Last Directory'
                if((line = in.readLine()) != null) {
                    if(!line.startsWith("FileBrowser\'s Last Directories")) {
                        Messages.postWarning("The " + filepath + " file is " +
                                             "old or corrupted and being removed");
                        file = new File(filepath);
                        // Remove the corrupted file.
                        file.delete();
                        // Default to the users data directory
                        root = dataHome;
                    }
                }
                // Get the starting dir path
                root = in.readLine().trim();
                // There should be a 'File Filter:' line after each
                // directory.  
                String ftype = in.readLine().trim();
                // Remove the 'File Filter: ' from the string
                ftype = ftype.substring(13);
                // set this filter type for the current directory
                setFilterType(ftype);
 
                // If parent == null, then we don't have prog buttons, so skip
//                if(parent != null) {
                    // Read any button paths which have been saved.
                    for (int i = 0; i < btnDirStr.size() && in.ready(); i++) {
                        String dir = in.readLine().trim();
                        if (dir.length() > 0) {
                            btnDirStr.remove(i);
                            btnDirStr.add(i, dir);
                            ftype = in.readLine().trim();
                            // There should be a 'File Filter:' line after each
                            // directory.  If not, just set to empty string
                            if(ftype.length() > 0 && ftype.startsWith("File")) {
                                btnFilterType.remove(i);
                                // Remove the 'File Filter: ' from the string
                                ftype = ftype.substring(13);
                                btnFilterType.add(i, ftype);
                            }
                            else {
                                // Default to .fid
                                btnFilterType.remove(i);
                                btnFilterType.add(i, FILTER_NAME_FID);
                            }
                        }

                    }

                    // Update the button label and tooltip for each button
                    updateProgrammableBtns();
//                }
                in.close();
            }
            catch (Exception ioe) { 
                // Default to the users data directory
                root = dataHome;
            }
        }
        else {
            root = dataHome;
        }

        // Be sure we have some type of valid path
        if(root.length() == 0  || (!root.startsWith(File.separator) 
                               && root.indexOf(":") < 1))
            root = File.separator;

        updateTree(root);
    }

    /* Write a persistence file containing the directory where the
       browser was upon closing vnmrj.
    */
    public void writePersistence(String type) {
        String filepath;

        saveButtonLabels();
        // If the directory is empty or just slash, don't write the file.
        if(root.length() <= 1)
            return;

        // These type strings are not translated from properties.
        if(type.equals("Browser") || type.equals("VBrowser"))
            filepath = FileUtil.savePath("USER/PERSISTENCE/"
                                            + "FileBrowserDir");
        else if(type.equals(Util.getLabel("blOpen")))
            filepath = FileUtil.savePath("USER/PERSISTENCE/"
                                            + "OpenFileBrowserDir");
        else if(type.equals(Util.getLabel("blSave")))
            filepath = FileUtil.savePath("USER/PERSISTENCE/"
                                            + "SaveFileBrowserDir");
        else
            return;


        FileWriter fw;
        PrintWriter os=null;
        try {
            File file = new File(filepath);
            fw = new FileWriter(file);
            os = new PrintWriter(fw);
            os.println("FileBrowser's Last Directories");
            File curFile = filechooser.getCurrentDirectory();
            String dirPath = curFile.getAbsolutePath();

            os.println(dirPath);

            // Write out the current file filter type
            javax.swing.filechooser.FileFilter filter = filechooser.getFileFilter();
            String str = "File Filter: " + filter.getDescription();
            os.println(str); 

            // Write out any buttons that have been set
            if (btnDirStr != null) {
                for (int i = 0; i < btnDirStr.size(); i++) {
                    String dir = (String) btnDirStr.get(i);
                    // skip if never set to anything other than 'Dir #'
                    if (!dir.startsWith("Dir")) {
                        os.println(dir);
                        String ftype = (String) btnFilterType.get(i);
                        os.println("File Filter: " + ftype);
                    }
                }
            }
            os.close();
        }
        catch (Exception er) {
            Messages.postError("Problem creating  " + filepath);
            Messages.writeStackTrace(er);
            if (os != null)
                os.close();
        }
    }



    public void treeWillExpand(TreeExpansionEvent evt) 
                                              throws ExpandVetoException{
        String path;

        TreePath treePath = evt.getPath();
        int np = treePath.getPathCount();

        // Avoid stack trace if double clicking the root item
        if(np > 1) {
            FileNode fn = (FileNode) treePath.getPathComponent(np-1);
            path = fn.getPath();

            String objType = FillDBManager.getType(path);

            // If objType is a type that it makes sense to further browse
            // down into, then allow the Expand to continue, else veto it.
            // This keep the browser double clicking consistent with the
            // locator.  Catch ? also.  That means that it is not known
            // as a locator type, and is probably a directory or unknown file.
            if(!objType.equals(Shuf.DB_VNMR_RECORD) &&
                       !objType.equals(Shuf.DB_STUDY) &&
                       !objType.equals(Shuf.DB_LCSTUDY) &&
                       !objType.equals(Shuf.DB_AUTODIR) &&
                       !objType.equals(Shuf.DB_IMAGE_DIR) &&
                       !objType.equals(Shuf.DB_COMPUTED_DIR) &&
                       !objType.equals(Shuf.DB_VFS) &&
                       !objType.equals(Shuf.DB_CRFT) &&
                       !objType.equals("?")) {
                // If it is not any of those things and is not a ?, then it
                // means it is a know locator type and cannot be further
                // browsed.  
                throw new ExpandVetoException(evt);
            }
            // It is an unknown file or directory type to the locator.
            // If it is just a directory, then allow expansion.  If it is
            // a flat file, then veto it
            else if (objType.equals("?")) {
                File file = new File(path);
                if(!file.isDirectory()) {
                    throw new ExpandVetoException(evt);
                }
            }
        }
    }


    // If just browsing into a directory, return false,
    // If opening a file, return true.
    public boolean openFile(String fullpath) {

        // Is this a shuffler type, or just a directory
        // that needs browsed?
        String objType = FillDBManager.getType(fullpath);
        
// Allow unknown to get to locaction, the user may want to do something there.       
//        if(objType.equals("?")) {
//            // Not known type, try to browse into it
//            updateTree(fullpath);
//                
//            return false;
//        }

        // If windows, we need to convert the windows path
        // to a unix type path at this point.
        String uPath;
        if(UtilB.iswindows())
            uPath = UtilB.windowsPathToUnix(fullpath);
        else
            uPath = fullpath;

        String hostFullpath = localHost + ":" + uPath;
                         
        // Is this file accessable by this user?
        // Get the owner from the DB if available
        boolean isAccessible=true;
        FillDBManager dbm = FillDBManager.fillDBManager;
        String owner = dbm.getAttributeValueNoError(objType, 
                                                    uPath, localHost, "owner");

        // If the file is not in the DB or any other problem, 
        // allow this file.
        if(owner.length() == 0)
            isAccessible = true;
        else {
            Hashtable accessHash = LoginService.getaccessHash();
            String curuser = System.getProperty("user.name");
            Access access = (Access)accessHash.get(curuser);
            isAccessible = access.isUserInList(owner);
        }

        if(isAccessible) {
            // Since the code is accustom to dealing with 
            // ShufflerItem's being D&D, I will just use that 
            // type and fill it in.
            ShufflerItem item = new ShufflerItem(fullpath, "BROWSER");

            String target = "Default";

            // If no file, don't try to act on it
            if(item.fileExists()) {
                item.actOnThisItem(target, "Open", "");
                // Clear the Error Text
                clearErrorText();
            }
            else {
                Messages.postWarning(hostFullpath 
                                     + " Does Not Exist or is not Mounted.");
                // Set Error Text
                setErrorText(hostFullpath 
                                     + " Does Not Exist or is not Mounted.");
            }

        }
        else  {
            // If the file is not accessible, do not allow 
            // the loading, and tell the user why.
            Messages.postWarning("Access denied to this "
                                 + "user's ("
                                 + owner + ") files.  \n    Have this "
                                 + "user added to your access list by the "
                                 + "System Admin if access is needed.");
        }
        return true;

    }

    // Create and setup the programmable directory buttons
    // Put them into a JPanel and return that panel
    public JPanel createProgrammableBtns() {
        String strVC=null;
        String strSetVC=null;
        btnDir = new ArrayList(numBtns);
        btnDirStr = new ArrayList(numBtns);
        btnFilterType = new ArrayList(numBtns);
        if (btnLabels == null)
           retrieveButtonLabels();

        // Create a panel to hold the buttons
        GridLayout gridLayout = new GridLayout(1, numBtns);
        JPanel ctrlPanel = new JPanel(gridLayout);

        for(int i=0; i < numBtns; i++) {
            // Button which can save the current dir and go back to it.
            ExpPanel exp=Util.getDefaultExp();
            VToolBarButton btn = new VToolBarButton(sshare, exp, "dirBtn" + i);

            // Save the btn item in the ArrayList
            btnDir.add(i, btn);
            // Default filter type to ".fid"
            btnFilterType.add(i, FILTER_NAME_FID);

            // If parent == null, then we have a browser, else open/save
            if (parent == null || type.equals("Browser")) {
                // Vnmr cmd to execute when button is pressed
                strVC = "vnmrjcmd(\'LOC setVJBrowserBtnDir " + i + "\')";
                // Vnmr cmd to execute when button is pressed and held 3 sec
                strSetVC = "vnmrjcmd(\'LOC saveVJBrowserBtnDir " + i 
                        + "\')";
            }
            else if(type.equals(Util.getLabel("blOpen"))) {
                // Vnmr cmd to execute when button is pressed
                strVC = "vnmrjcmd(\'LOC setOpenBtnDir " + i + "\')";
                // Vnmr cmd to execute when button is pressed and held 3 sec
                strSetVC = "vnmrjcmd(\'LOC saveOpenBtnDir " + i
                        + "\')";
            }
            else if(type.equals(Util.getLabel("blSave"))) {
                // Vnmr cmd to execute when button is pressed
                strVC = "vnmrjcmd(\'LOC setSaveBtnDir " + i + "\')";
                // Vnmr cmd to execute when button is pressed and held 3 sec
                strSetVC = "vnmrjcmd(\'LOC saveSaveBtnDir " + i
                        + "\')";
            }

            // Get the dir name to use for a label
            // Get just the last directory level name.  If nothing was read
            // in from persistence file, Call it Dir i
            String strLabel="Dir " + (i +1);

            btnDirStr.add(strLabel);
            String strToolTip = Util.getLabel("_Press_and_Hold") + " " + Util.getLabel("_Directory");
            int index;

            // If the button has been set to something, set its tooltip
            if(btnDirStr.size() >= i +1) {
                String btnStr = (String) btnDirStr.get(i);
                if(btnStr.length() >1 && !btnStr.startsWith("Dir")) {
                    index = btnStr.lastIndexOf(File.separator);
                    if(index > 0)
                        strLabel = btnStr.substring(index+1);
                    strToolTip = new String(btnStr);
                }
            }
            //  btn.setAttribute(VObjDef.ICON, strLabel);
            setButtonLabel(i, strLabel);
            btn.setAttribute(VObjDef.TOOL_TIP, strToolTip);
            btn.setAttribute(VObjDef.CMD, strVC);
            btn.setAttribute(VObjDef.SET_VC, strSetVC);
            btn.setBorder(m_raisedBorder);


            // Add this button to the panel
            ctrlPanel.add(btn);
        }

        return ctrlPanel;
    }


    // Set the button label and tooltip for each button
    public void updateProgrammableBtns() {
        int index;

        for(int i=0; i < btnDirStr.size(); i++) {
            String btnStr = (String) btnDirStr.get(i);
            String strToolTip=Util.getLabel("_Press_and_Hold") + " " + Util.getLabel("_Directory");
            String strLabel="Dir " + (i +1);

            // Set the tooltip
            if(btnStr.length() >1 && !btnStr.startsWith("Dir")) {
                index = btnStr.lastIndexOf(File.separator);
                if(index > 0)
                    strLabel = btnStr.substring(index+1);
                strToolTip = new String(btnStr);
            }

            VToolBarButton btn = (VToolBarButton)btnDir.get(i);
            //  btn.setAttribute(VObjDef.ICON, strLabel);
            setButtonLabel(i, strLabel);
            btn.setAttribute(VObjDef.TOOL_TIP, strToolTip);
            
        }
    }

    public JPanel createDataDirMenuPanel() {
        JPanel dirPanel = new JPanel(new BorderLayout());
        JLabel label = new JLabel(Util.getLabel("_Choose_Home_Directory"));
        dirPanel.add(label, BorderLayout.BEFORE_LINE_BEGINS);

        // Get the list of data directories for this user and create
        // a String[] to pass onto the combo box
        String curuser = System.getProperty("user.name");
        LoginService loginService = LoginService.getDefault();
        Hashtable userHash = loginService.getuserHash();
        User user = (User)userHash.get(curuser);
        ArrayList dirList = user.getDataDirsNotConanical();
        // We want to exclude dirs that do not exist
        // so first count existing dirs
        int cnt=0;
        // If allowUpFolder is false, don't allow any, the item will not show up.
        if (allowUpFolder) {
            for (int k = 0; k < dirList.size(); k++) {
                String dir = (String) dirList.get(k);
                // Be sure the directory exists and only add it if it does
                UNFile file = new UNFile(dir);
                if (file.exists())
                    cnt++;
            }
        }
        // Include a space for the users userdir directory
        String[] strList = new String[cnt +1];
        int j=0;
        for(int i=0; i < dirList.size(); i++) {
        	String dir = (String) dirList.get(i);
        	// Be sure the directory exists and only add it if it does
        	UNFile file = new UNFile(dir);
        	if(file.exists())
        		strList[j++] = dir;
        }
        strList[j] = System.getProperty("userdir");

        directoryComboBox = new JComboBox(strList);
        directoryComboBox.addActionListener(new DirectoryComboBoxAction());

        label.setLabelFor(directoryComboBox);

        dirPanel.add(directoryComboBox, BorderLayout.CENTER);
        return dirPanel;
    }

    // createFilechooser()
    // Create the JFileChooser.  Modify it as noted below
    // Return the resulting modified VCustomFileChooser
    // The stock JFileChooser puts up an upFolder button that we
    // don't always want there.  It also causes the Home button
    // to go to the 'user.home' directory with no way to change it.
    // Thus the following code, gets the JButton items which are
    // the upFolder and homeButton.  It sets a new actionListener
    // for the homeButton, so we can change the dir to where we want.
    // If directed to do so, it then removed the upFolder item.
    // The panels and buttons for the JFileChooser are created in
    // the Java file MetalFileChooserUI.java .  Based on looking at
    // that file and the order or creation I found that:
    // Item 0 of the JFileChooser is topPanel containing 
    // Item 0 of topPanel is topButtonPanel 
    // Item 0 of topButtonPanel is upFolderButton
    // Item 2 of topButtonPanel is homeFolderButton
    // If allowUpFolder is false, we also need to remove the menu list
    // that allows selecting any parent directory.  The title and
    // the item are #1 and #2 in topPanel

    // If parent = null, then we are probably being created for the
    // pushpin area.  In that case, we do not want a Cancel button

    public VCustomFileChooser createFilechooser(String type) {

        VCustomFileChooser chooser = new VCustomFileChooser(type, this);
        if(chooser.getComponent(0) instanceof JPanel)
        try {
        	topPanel = (JPanel) chooser.getComponent(0);
            JPanel topButtonPanel = (JPanel) topPanel.getComponent(0);
            upFolderButton =(JButton)topButtonPanel.getComponent(0);
            JButton homeButton = (JButton)topButtonPanel.getComponent(2);

            // If this is correct, the Actioncommand for upFolderButton
            // will be 'Go Up' If this is not correct, then we probably
            // changed Java versions and the code adding buttons and
            // panels is probably different.  A programmer will need to
            // look into the Java Code in MetalFileChooserUI.java
            // to see how to fix this.
            String cmd = upFolderButton.getActionCommand();
            if(!cmd.equals("Go Up")) {
                Messages.postError("Problem with buttons in VJFileBrowser. "
                                   + "This is probably due\n    to a change"
                                   + "in the Java version.  This needs to "
                                   + "be repaired in the code.\n"
                                   + "    File JFileBrowser.java (Go Up)");
            }

            // Now we need to remove the previous action listener
            // from the homeButton.  Assume there is only one.
            ActionListener[] list = homeButton.getActionListeners();
            homeButton.removeActionListener(list[0]);
            // Then add our new listener
            homeButton.addActionListener(new GoHomeAction());

            // When the panel is Save, we want to change the text of the
            // filesOfTypeLabel which controls the file filters
            // Save the label item for setting appropriately later
            if(chooser.getComponentCount()>3){
                JPanel bottomPanel = (JPanel)chooser.getComponent(3);
                JPanel filesOfTypePanel = (JPanel)bottomPanel.getComponent(2);
    
                filesOfTypeLabel = (JLabel)filesOfTypePanel.getComponent(0);
                filesOfTypeLabelText = filesOfTypeLabel.getText();
            }

            if(!allowUpFolder) {
                // We need to disable upFolderButton if that is
                // specified.  It is the 0th item in topButtonPanel
                // We need to replace its listener so we can keep
                // dirLabel up to date.
                Icon upFolderIcon = upFolderButton.getIcon();
                list = upFolderButton.getActionListeners();
                upFolderButton.removeActionListener(list[0]);
                upFolderButton.addActionListener(new upDirAction());

                upFolderButton.setIcon(upFolderIcon);
                
                // The button is shown or unshown in updateDirLabel()
                

                // We also need to remove the menu list of parent 
                // directories.  the label and the list are #1 and #2
                // items in topPanel. 
                topPanel.remove(2);
                topPanel.remove(1);

                // Now there is nothing to show what dir we are in.
                // Add a JTextField in place of the choice menu.  Use
                // this instead of a JLabel so that long lines can be
                // scrolled with the arrow keys.
                dirLabel = new JTextField(root);
                dirLabel.setEditable(false);
                topPanel.add(dirLabel, BorderLayout.CENTER);
            }
        }
        catch(Exception e) {
            Messages.writeStackTrace(e);
            Messages.postError("Problem with buttons in VJFileBrowser. "
                               + "This is probably due\n    to a change"
                               + "in the Java version.  This needs to "
                               + "be repaired in the code.\n"
                               + "    File JFileBrowser.java (Exception) ");
            // Just keep going, not fatal, the panel will just be
            // a stock panel.
        }
        chooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
        chooser.addActionListener(this);

        // For non modal popup, don't show Open and Cancel buttons
        if(parent == null)
            chooser.setControlButtonsAreShown(false);
        
        return chooser;
    }

    public void showBrowser(String path) {
        if(path !=null){
            File dir=new File(path);
            if(dir.isDirectory())
                filechooser.setCurrentDirectory(dir);
        }
        showPanel("Open");
    }

    public void showPanel(String cmd) {

        clearErrorText();

        // Make it visible
        setVisible(true); 
        filechooser.rescanCurrentDirectory();
        
        // If parent is not null, show our parent.  If null, then
        // we were probably create by the pushpin area 
        if(parent != null) {
            parent.setVisible(true);
            if (parent instanceof JFrame) {
                JFrame frame = (JFrame)parent;
                frame.setExtendedState(Frame.NORMAL);
                frame.toFront();
            }
        }

    }

    // Make the panel non visible and update root so that if they
    // exit vnmrj, writePersistence will know what to save.
    public void unShowPanel() {
        File file = filechooser.getCurrentDirectory();
        String dir = file.getAbsolutePath();

        // Save for writePersistence()
        root = dir;

        // If parent is not null, unshow our parent.  If null, then
        // we were probably create by the pushpin area so we don't
        // want to unshow anything.
        if(parent != null) {
            parent.setVisible(false);
        }
 
    }

    public boolean readOk(String fullpath) {

        File file = new File(fullpath);

        if(!file.exists()) {
            // Error that File does not exists
            Messages.postError("File  " + fullpath + " does not exist.");
            setErrorText("File " + fullpath + "\ndoes not exist.");
            return false;
        }
        else if(!file.canRead()) {
            // Error that no permission to read
            Messages.postError("File " + fullpath + 
                              " Read Permission denied.");
            setErrorText("File  " + fullpath + 
                         "\nRead Permission denied.");
            return false;
        }
        else {
            return true;
        }
    }

    public boolean writeOk(String canPath, String absPath, String filterType) {
        UNFile file = new UNFile(canPath);

        if(!file.exists()) {
            // No file exists, is the directory writable
            
            if(filterType.equals(FILTER_NAME_FDF)) {
                // If type is .fdf, we just want to check back 3 levels,
                // not just one.  aipWriteData will create two parent 
                // levels if necessary.
                File parentDir = file.getParentFile();
                File parent2Dir = parentDir.getParentFile();
                File parent3Dir = parent2Dir.getParentFile();
                if(parent2Dir.canWrite())
                    return true;
                else if(parent3Dir.canWrite())
                    return true;
                // else, Permission denied
                Messages.postError("Directory " + parent2Dir
                        + " Write Permission denied.");
                setErrorText("Directory  " + parent2Dir
                        + "\nWrite Permission denied.");
                return false;
               
            }    
            else if(filterType.equals(FILTER_NAME_IMG)) {
                // If type is .img, we just want to check back 2 levels,
                // not just one.  aipWriteData will create one parent 
                // levels if necessary.
                File parentDir = file.getParentFile();
                File parent2Dir = parentDir.getParentFile();
                if(parent2Dir.canWrite())
                    return true;
                // else, Permission denied
                Messages.postError("Directory " + parent2Dir
                        + " Write Permission denied.");
                setErrorText("Directory  " + parent2Dir
                        + "\nWrite Permission denied.");
                return false;
                
            }  
            else {
                // Not .img nor .fdf, check parent
                File parentDir = file.getParentFile();
                if(parentDir.canWrite()) {
                    return true;
                }
                // else, Permission denied
                Messages.postError("Directory " + parentDir
                        + " Write Permission denied.");
                setErrorText("Directory  " + parentDir
                        + "\nWrite Permission denied.");
                return false;
            }

		}
        else if(file.canWrite()) {
            
            // Popup a panel asking if we should overwrite the existing file
            new OverwriteQuery(canPath);

            // The result of the popup will be in overwriteOkay
            if(overwriteOkay)
                return true;
            else
                return false;
        }
        else {
        	// Permission denied
            Messages.postError("File " + canPath +
                              " Write Permission denied.");
            setErrorText("File  " + canPath +
                              "\nWrite Permission denied.");
            return false;
        }
    }

    public void setErrorText(String error) {

            errorItem.setText(error);
            add(errorItem, BorderLayout.SOUTH);
            setVisible(true); 
            if(parent != null) {
                JDialog par = (JDialog)parent;

                // If the frame has been dragged to a new size, we need to
                // set that as the preferred size, else, when we do the
                // pack() to get the error panel included, the frame will
                // go back to its previous preferred size
                Dimension dim = par.getSize();
                par.setPreferredSize(dim);

                // Cause the new panel to be included into the frame display
                par.pack();
            }
    }

    public void clearErrorText() {
        try {
            errorItem.setText("");
            if(errorItem != null)
                remove(errorItem);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
    }



    protected void setLookAndFeel(String lookandfeel)
    {
        // Save the look and feel for restoring
        if(defaultLookAndFeel == null)
            defaultLookAndFeel = UIManager.getLookAndFeel().getID();

        /***************
        try
        {
            // lookandfeel can be motif or metal
            String lf = UIManager.getSystemLookAndFeelClassName();
            UIManager.LookAndFeelInfo[] lists = UIManager.getInstalledLookAndFeels();
            if (lookandfeel != null) {
                lookandfeel = lookandfeel.toLowerCase();
                int nLength = lists.length;
                String look;
                for (int k = 0; k < nLength; k++) {
                    look = lists[k].getName().toLowerCase();
                    if (look.indexOf(lookandfeel) >= 0) {
                        lf = lists[k].getClassName();
                        break;
                    }
                }
            }
            UIManager.setLookAndFeel(lf);
        }
        catch (Exception e)
        {
            Messages.postDebug(e.toString());
        }
        ************/
    }

    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
        
        if (cmd.equals(JFileChooser.APPROVE_SELECTION)) {

            File file = filechooser.getSelectedFile();
            String fullpath = file.getAbsolutePath();
            int dialogType = filechooser.getDialogType();

            if(dialogType == JFileChooser.SAVE_DIALOG) {
                String canPath;
                String fileName;
                try {
                    // The canonical path does not work correctly on
                    // Windows for symbolic links.  You can create them
                    // using SFU, but java does not resolve the link.
                	// So for windows, we will get the path to the link.
                    canPath = file.getCanonicalPath();
                }
                catch(Exception ex) {
                    Messages.writeStackTrace(ex);
                    canPath = fullpath;
                }

                // What file filter type has the user chosen?
                javax.swing.filechooser.FileFilter ff;
                ff = filechooser.getFileFilter();
                String filterType = ff.getDescription();

                // If filterType starts with a ".", then we need to be sure
                // the filename ends with that suffix.
                if(filterType.startsWith(".")) {
                    if(!canPath.endsWith(filterType))
                        canPath = canPath.concat(filterType);
                }
                
                // writeOk() removes the current file to avoid asking
                // about overwriting it on the line3 line.
                if(writeOk(canPath, fullpath, filterType)) {
                    // Windows still needs unix path for svf so be sure
                    // it gets a unix path
                    if (Util.iswindows())
                        canPath = UtilB.windowsPathToUnix(canPath);

                    // If this is a link, we need to pass svf the canonical
                    // path so the link is left unaltered.
                    // Use force to avoid a question on line3 about
                    // overwriting.
                    if(filterType.equals(FILTER_NAME_FID))
                         Util.sendToVnmr("svf (\'" + canPath + 
                                        "\', \'force\',\'opt\')");
                    else if(filterType.equals(FILTER_NAME_PAR))
                         Util.sendToVnmr("svp (\'" + canPath + 
                                        "\', \'force\')");
                    else if(filterType.equals(FILTER_NAME_SHIM)) {
                        Util.sendToVnmr("svs (\'" + canPath + "\')");
                    }
                    else if(filterType.equals(FILTER_NAME_VFS)) {
                        Util.sendToVnmr("svvfs (\'" + canPath + "\')");
                    }
                    else if(filterType.equals(FILTER_NAME_IMG)) {
                        // If they selected .img type, then just add the
                        // name 'image' for the fdf files
                        Util.sendToVnmr("aipWriteFmtConvert=\'fdf\'");
                        Util.sendToVnmr("aipWriteData(\'" + canPath
                                + File.separator + "image\')");

                    }
                    else if(filterType.equals(FILTER_NAME_FDF)) {
                        Util.sendToVnmr("aipWriteFmtConvert=\'fdf\'");
                        Util.sendToVnmr("aipWriteData(\'" + canPath + "\')");
                    }
                    else {
                         setErrorText("Saving " + filterType
                                      + " not implemented.");
                         // Return without unshowing panel
                         return;
                    }

                    unShowPanel();
                }
            }
            else if(dialogType == JFileChooser.OPEN_DIALOG) {
                if(readOk(fullpath)) {
                    String objType = FillDBManager.getType(fullpath);
                   
                    if(objType.equals("?") && file.isDirectory()) {
                        filechooser.setCurrentDirectory(file);
                        
                        return;
                    }
                    // openFile returns false if just opening a dir,
                    // in that case, don't unshow the panel
                    if(openFile(fullpath)) {
                        // Don't call for the vertical panel nor the Browser
                        if(parent != null && !type.equals("Browser"))
                            unShowPanel();
                    }
                }
            }
        }

        else if (cmd.equalsIgnoreCase(JFileChooser.CANCEL_SELECTION))
        {
            unShowPanel();
        }

    }

    protected void updateDirLabel(String path) {
        if(dirLabel != null) {
            dirLabel.setText(path);

            // If allowUpFolder == false, and dirLabel is within userDir, 
            // then allow the upFolder anyway.  That is, allow the upFolder
            // as long as it will not go above userDir.
            if(allowUpFolder || path.startsWith(userDir))
                upFolderButton.setVisible(true);
            else
                upFolderButton.setVisible(false);
        }

    }

    public class OverwriteQuery extends ModalDialog implements ActionListener
    {
        public OverwriteQuery(String fullpath) {
            super("Overwrite Query");

            okButton.addActionListener(this);
            cancelButton.addActionListener(this);


            JTextArea msg = new JTextArea("  The File  " + fullpath 
                                          + "  \n  Already Exists, "
                                          + "Overwrite it?  ");
            
            getContentPane().add(msg, BorderLayout.NORTH);
            pack();

            setVisible(true);

        }


        public void actionPerformed(ActionEvent e) {

            String cmd = e.getActionCommand();

            if (cmd.equalsIgnoreCase("ok")) {
                overwriteOkay = true;
            }
            else if (cmd.equalsIgnoreCase("cancel"))
            {
                overwriteOkay = false;
            }

            setVisible(false);
        }
    }


    // Set the Browser root to the path for the indicated button
    public void setBrowserToDirBtnPath(int whichOne) {

        String path = (String) btnDirStr.get(whichOne);
        if(!path.startsWith("Dir")) {
            updateTree(path);
            if(DebugOutput.isSetFor("VJFileBrowser")) {
                Messages.postDebug("VJFileBrowser.setBrowserToDirBtnPath "
                                   + "set root to " + path);
            }
        }
        else if(DebugOutput.isSetFor("VJFileBrowser")) {
            Messages.postDebug("VJFileBrowser.setBrowserToDirBtnPath "
                               + " button " + whichOne + " not set.");
        }
        setFilterType((String)btnFilterType.get(whichOne));
    }

    // Set the File Filter Type to the filter corresponding to this
    // string type
    void setFilterType(String ftype) {
        javax.swing.filechooser.FileFilter[] filterList;
        javax.swing.filechooser.FileFilter filter;
        String filterDesc;

        // Default to .fid
        filter = fidFilter;
        
        // Get the list of file filters we have set up
        filterList = filechooser.getChoosableFileFilters();
        for(int i=0; i < filterList.length; i++) {
            filter = filterList[i];
            filterDesc = filter.getDescription();
            // Look for the one with the matching description
            if(filterDesc.equals(ftype))
                break;
        }
        // Set this filter to the current filechooser
        filechooser.setFileFilter(filter);

    }

    // Save the current directory for this button's path
    public void setDirBtnPath(int whichOne) {

        File file = filechooser.getCurrentDirectory();
        String dir = file.getAbsolutePath();

        File file2 = new File(dir);
        if (!file2.exists() || !file2.isDirectory()) {
            JOptionPane.showMessageDialog(this,
                "Wrong file path: \n"+dir,
                "Java Bug",
                JOptionPane.WARNING_MESSAGE);
            return;
        }
        
        String nameStr = filechooser.getName(file);
        saveButtonLabel(whichOne, nameStr);

        btnDirStr.remove(whichOne);
        btnDirStr.add(whichOne, new String(dir));

        String strToolTip = new String(dir);
        VToolBarButton btn = (VToolBarButton) btnDir.get(whichOne);
        btn.setAttribute(VObjDef.TOOL_TIP, strToolTip);

        // Get the dir name to use for a label
        // Get just the last directory level name
        String strLabel="";
        int index = dir.lastIndexOf(File.separator);
        if(index > 0)
            strLabel = dir.substring(index+1);
        // btn.setAttribute(VObjDef.ICON, strLabel);
        setButtonLabel(whichOne, strLabel);

        // Set the Filter type for this button
        javax.swing.filechooser.FileFilter filter = filechooser.getFileFilter();
        btnFilterType.remove(whichOne);
        btnFilterType.add(whichOne, filter.getDescription());
        
        if(DebugOutput.isSetFor("VJFileBrowser")) {
            Messages.postDebug("VJFileBrowser.setDirBtnPath set btnDirStr"
                               + whichOne + " to " + dir);
        }
    }

    // Create the list of file filter for the Open panel
    public void createOpenFileFilterList() {
    	
    	filechooser.resetChoosableFileFilters();
    	
        // Setup the menu of choices for filtering the file listing
        // These will show up in the order added
        filechooser.setAcceptAllFileFilterUsed(true);
       
        filechooser.addChoosableFileFilter(fidFilter);
        filechooser.addChoosableFileFilter(new parFileFilter());
        filechooser.addChoosableFileFilter(new studyFileFilter());
        filechooser.addChoosableFileFilter(new shimFileFilter());
        filechooser.addChoosableFileFilter(new fdfFileFilter());
        filechooser.addChoosableFileFilter(new imgFileFilter());
        filechooser.addChoosableFileFilter(new protocolFileFilter());
        filechooser.addChoosableFileFilter(new autoFileFilter());
        filechooser.addChoosableFileFilter(new recordFileFilter());
        filechooser.addChoosableFileFilter(new pNcFileFilter());
        filechooser.addChoosableFileFilter(new workspaceFileFilter());
        filechooser.addChoosableFileFilter(new lcStudyFileFilter());
        filechooser.addChoosableFileFilter(new molFileFilter());
        filechooser.addChoosableFileFilter(new vfsFileFilter());
        filechooser.addChoosableFileFilter(new crftFileFilter());
        filechooser.addChoosableFileFilter(new iconFileFilter());

        // Set .fid default
        filechooser.setFileFilter(fidFilter);

    }



    // Create the list of file filter for the Save panel
    public void createSaveFileFilterList() {
    	
    	filechooser.resetChoosableFileFilters();

        // Setup the menu of choices for filtering the file listing
        // Don't use the std 'All' filter
        filechooser.setAcceptAllFileFilterUsed(false);

        // These will show up in the order added
        filechooser.addChoosableFileFilter(fidFilter);
        filechooser.addChoosableFileFilter(new parFileFilter());
        filechooser.addChoosableFileFilter(new shimFileFilter());
        filechooser.addChoosableFileFilter(new imgFileFilter());
        filechooser.addChoosableFileFilter(new fdfFileFilter());
        filechooser.addChoosableFileFilter(new vfsFileFilter());

        filechooser.setFileFilter(fidFilter);

    }

    public class fidFileFilter extends javax.swing.filechooser.FileFilter
    {
        // Need to allow .fid and directories
        public boolean accept(File path) {
            // If .fid, just allow it
            if(path.getName().endsWith(".fid"))
                return true;
            // If it is a directory, allow it if NOT a shuffler type
            else if(path.isDirectory()) {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals("?") || type.equals(Shuf.DB_STUDY))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_FID;
        }
    }


    public class parFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            if(path.getName().endsWith(".par"))
                return true;
            // If it is a directory, allow it if NOT a shuffler type
            else if(path.isDirectory()) {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals("?"))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_PAR;
        }
    }

    public class studyFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a directory, allow it if NOT a shuffler type
            if(path.isDirectory()) {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_STUDY))
                    return true;
                // If it is a directory, allow it if NOT a shuffler type
                else if(type.equals("?"))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_STUDY;
        }
    }

    public class vfsFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a directory, allow it if NOT a shuffler type
            if(path.isDirectory()) {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_VFS))
                    return true;
                // If it is a directory, allow it if NOT a shuffler type
                else if(type.equals("?"))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_VFS;
        }
    }
    public class crftFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a directory, allow it if NOT a shuffler type
            if(path.isDirectory()) {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_CRFT))
                    return true;
                // If it is a directory, allow it if NOT a shuffler type
                else if(type.equals("?"))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_CRFT;
        }
    }

    public class shimFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a directory, allow it
            if(path.getName().endsWith(".fid") || 
                      path.getName().endsWith(Shuf.DB_VFS_SUFFIX)  ||
                      path.getName().endsWith(Shuf.DB_CRFT_SUFFIX))
                return false;
            else if(path.isDirectory()) {
                return true;
            }
            else {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_SHIMS))
                    return true;
                else
                    return false;
            }
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_SHIM;
        }
    }


    public class fdfFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
			// If it is a non .fid directory, allow it
			if (path.getName().endsWith(".fid") || 
                                path.getName().endsWith(Shuf.DB_VFS_SUFFIX) ||
                                path.getName().endsWith(Shuf.DB_CRFT_SUFFIX))
				return false;
			else if (path.isDirectory()) {
				return true;
			} 
			else {
				String type = FillDBManager.getType(path.getAbsolutePath());
				if (type.equals(Shuf.DB_IMAGE_FILE))
					return true;
				else
					return false;
			}
		}

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_FDF;
        }
    }
    
    public class molFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
			// If it is a non .fid directory, allow it
			if (path.getName().endsWith(".fid") || 
                                  path.getName().endsWith(Shuf.DB_VFS_SUFFIX) ||
                                  path.getName().endsWith(Shuf.DB_CRFT_SUFFIX))
				return false;
			else if (path.isDirectory()) {
				return true;
			} 
			else {
				String type = FillDBManager.getType(path.getAbsolutePath());
				if (type.equals(Shuf.DB_MOLECULE))
					return true;
				else
					return false;
			}
		}

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_MOLECULE;
        }
    }

    public class iconFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
			// If it is a non .fid directory, allow it
			if (path.getName().endsWith(".fid") || 
                                  path.getName().endsWith(Shuf.DB_VFS_SUFFIX) ||
                                  path.getName().endsWith(Shuf.DB_CRFT_SUFFIX))
                            return false;
			else if (path.isDirectory()) {
                            return true;
			} 
			else {
                            String type = FillDBManager.getType(path.getAbsolutePath());
                            if (type.equals(Shuf.DB_ICON))
                                return true;
                            else
                                return false;
			}
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_ICON;
        }
    }

    public class imgFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a directory, allow it if NOT a shuffler type
            if(path.isDirectory()) {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_IMAGE_DIR) ||
                   type.equals(Shuf.DB_COMPUTED_DIR))
                    return true;
                // If it is a directory, allow it if NOT a shuffler type
                else if(type.equals("?") || type.equals(Shuf.DB_STUDY))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_IMG;
        }
    }

    public class protocolFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a non .fid directory, allow it
            if(path.getName().endsWith(".fid") || 
                      path.getName().endsWith(Shuf.DB_VFS_SUFFIX) ||
                      path.getName().endsWith(Shuf.DB_CRFT_SUFFIX))
                return false;
            else if(path.isDirectory()) {
                return true;
            }
            else {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_PROTOCOL))
                    return true;
                else
                    return false;
            }
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_PROTOCOL;
        }
    }


    public class autoFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a directory, allow it if NOT a shuffler type
            if(path.isDirectory()) {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_AUTODIR))
                    return true;
                // If it is a directory, allow it if NOT a shuffler type
                else if(type.equals("?"))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_AUTODIR;
        }
    }


    public class recordFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a directory, allow it if NOT a shuffler type
            if(path.isDirectory()) {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_VNMR_RECORD))
                    return true;
                // If it is a directory, allow it if NOT a shuffler type
                else if(type.equals("?"))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_RECORD;
        }
    }


    public class pNcFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a non .fid directory, allow it
            if(path.getName().endsWith(".fid") || 
                      path.getName().endsWith(Shuf.DB_VFS_SUFFIX) ||
                      path.getName().endsWith(Shuf.DB_CRFT_SUFFIX))
                return false;
            else if(path.isDirectory()) {
                return true;
            }
            else {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_PANELSNCOMPONENTS))
                    return true;
                else
                    return false;
            }
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_PANELSNCOMPONENTS;
        }
    }


    public class workspaceFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a directory, allow it if NOT a shuffler type
            if(path.isDirectory()) {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_WORKSPACE))
                    return true;
                // If it is a directory, allow it if NOT a shuffler type
                else if(type.equals("?"))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_WORKSPACE;
        }
    }


    public class lcStudyFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            // If it is a directory, allow it if NOT a shuffler type
            if(path.isDirectory()) {
                String type = FillDBManager.getType(path.getAbsolutePath());
                if(type.equals(Shuf.DB_LCSTUDY))
                    return true;
                // If it is a directory, allow it if NOT a shuffler type
                else if(type.equals("?"))
                    return true;
                else
                    return false;
            }
            else
                return false;
        }

        // This output is used as the line in the menu of choices
        public String getDescription() {
            return FILTER_NAME_LCSTUDY;
        }
    }


    protected class DirectoryComboBoxAction extends AbstractAction {
		protected DirectoryComboBoxAction() {
			super("DirectoryComboBoxAction");
		}

		public void actionPerformed(ActionEvent e) {
			// Set the dataHome string to the selected directory
			dataHome = (String) directoryComboBox.getSelectedItem();
            updateTree(dataHome);
		}
	}

    
}


class VCustomFileChooser extends JFileChooser
{
    protected static final Icon folderIcon;

    static
        {
            FileSystemView view = FileSystemView.getFileSystemView();
            String sysdir = System.getProperty("sysdir");
            folderIcon = view.getSystemIcon(new File(sysdir + File.separator +
                                                     "vnmrrev"));
        }

    // Added code to stop the ability to rename a directory in the
    // JFileChooser.  Else, it you accidently double click too slowly,
    // you can end up in edit mode and accidently rename a directory.
    // This part has been removed due to screwing up Drag and Drop
    public VCustomFileChooser(String type, VJFileBrowser browser) {
        JList list = findFileList(this);

        for (MouseListener l : list.getMouseListeners())
        {
            if (l.getClass().getName().indexOf("FilePane") >= 0)
            {

// This screws up D&D. Stop doing it unless I find a way to get around that
// If I cannot use this code, I need to remove all of the MyMouseListener
// and fileFileList code.
//                list.removeMouseListener(l);
// Try adding mine, not removing the other one so that I can select
// the item on the mouse down

                list.addMouseListener(new MyMouseListener(l, browser));
            }
        }
    }

    private JList findFileList(Component comp) {
        if (comp instanceof JList)
            return (JList)comp;

        if (comp instanceof Container)
        {
            for (Component child : ((Container)comp).getComponents())
            {
                JList list = findFileList(child);
                if (list != null)
                    return list;
            }
        }

        return null;
    }

    private class MyMouseListener extends MouseAdapter {
        VJFileBrowser browser;

        MyMouseListener(MouseListener listenerChain, VJFileBrowser browser) {
            this.browser = browser;
        }
        public void mouseClicked(MouseEvent event) {
            if (event.getClickCount() > 1) {
// don't call this if I have not removed the listener above
// Removing the original listener disables D&D
//                m_listenerChain.mouseClicked(event);

                    
                // Get the fullpath to display in the dirLabel
                if(browser.dirLabel != null) {
                    File file = browser.filechooser.getCurrentDirectory();
                    String fullpath = file.getAbsolutePath();
                    browser.updateDirLabel(fullpath);
                }
            }
        }

// Any chance I can do a selection so that D&D work without
// first selecting the item?

//         public void mousePressed(MouseEvent event) {
//             System.out.println("mousePressed " + event.paramString());
//             m_listenerChain.mousePressed(event);
//             VJFileBrowser vjfb;
//             vjfb = VJFileBrowser.getOpenFileBrowser();
//             File file = vjfb.filechooser.getSelectedFile();
//             String fullpath = file.getAbsolutePath();
//             System.out.println("getSelectedFile " + fullpath);
//         }

    }



    // Force .fid directories to have a file icon.  .
    public Icon getIcon(File f) {
        String name=f.getName();
        if (name !=null && name.endsWith(Shuf.DB_FID_SUFFIX))
            return Util.getGeneralIcon("blueFID.png");
        else if(name !=null && (name.endsWith(Shuf.DB_VFS_SUFFIX) ||
                                (name.endsWith(Shuf.DB_CRFT_SUFFIX))))
            return Util.getGeneralIcon("blueFID.png");
        return super.getIcon(f);
    }

    public boolean isTraversable(File f) {
        Boolean traversable = null;
        FileView uiFileView;
        if (f != null) {
            uiFileView = getUI().getFileView(this);

            if (getFileView() != null) {
                traversable = getFileView().isTraversable(f);
            }
            if (traversable == null && uiFileView != null) {
                traversable = uiFileView.isTraversable(f);
            }
            if (traversable == null) {
                traversable = getFileSystemView().isTraversable(f);
            }

            // This not only disallows the ability to browse into this
            // directory, but it causes the panel to put the name into
            // the text item for the file name so that open operates on it.
            // Perhaps double clicking will do the open function also.
            // It also causes the .fid directories to fall in with the
            // actual files in the panel after the directories.
            String objType = FillDBManager.getType(f.getPath());

            // These vnmrj types will not be traversable (cannot change dir
            // down into these.  They will be treated as objects themselves.
            // Workspaces used to be here also and a jexp was done instead
            // of browsering into the workspace.  2/18/10 Mark Dixon approved
            // allowing browsing into workspaces instead of jexp.
            // Changing this here actually still allows "Open" to do a jexp
            // while double clicking browses into the workspace.

            // To have Open browse into the workspace, Modify
            // about line 1123 in actionPerformed() to check for workspace
            // and do a setCurrentDirectory(file) 
            // Treat .crft and .vfs as an object.  Don't open the dir
            if(objType.equals(Shuf.DB_VNMR_DATA) || 
                    objType.equals(Shuf.DB_VNMR_PAR) || objType.equals(Shuf.DB_VNMR_RECORD)
                    || objType.equals(Shuf.DB_CRFT) || objType.equals(Shuf.DB_VFS))
                traversable = new Boolean(false);
        }
        return (traversable != null && traversable.booleanValue());
    }

    // Windows will throw exceptions on some network directories
    @Override
    public void setCurrentDirectory(File dir) {
       try {
           super.setCurrentDirectory(dir);
       }
       catch (Exception e) { }
       catch (Error e) { }
    }



}
