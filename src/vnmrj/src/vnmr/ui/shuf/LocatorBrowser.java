/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import javax.swing.table.*;
import javax.swing.border.*;
import javax.swing.tree.*;
import javax.swing.event.*;
import javax.swing.plaf.*;
import javax.swing.*;
import java.awt.*;
import java.awt.datatransfer.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import java.beans.*;
import java.net.*;
import java.awt.dnd.*;

import vnmr.util.*;
import vnmr.ui.*;
import vnmr.bo.*;

/********************************************************** <pre>
 * Summary: Browser integrated with Locator
 *
 *
 </pre> **********************************************************/
public class LocatorBrowser extends JPanel  implements TreeExpansionListener,
                                    TreeWillExpandListener, KeyListener
{
    protected static String root = "";
    private static String btnDir1 = "";
    private static String btnDir2 = "";
    private VToolBarButton dirBtn1 = null;
    private VToolBarButton dirBtn2 = null;
    private String upToolTip;
    private JButton upDir;

    // Keep a copy of ourselves to return to callers who do not have it.
    private static LocatorBrowser locatorBrowser=null;
    private SessionShare sshare;
    private BrowserDropTargetListener dropListener;
    private DropTarget dropTarget;
    protected String localHost;
    /** drag source */
    private DragSource dragSource;
    protected JTree tree;
    protected TreeModel treeModel;
    protected TreeExpansionListener tel;
    protected JScrollPane spane;

    private final static Border  m_raisedBorder = 
                                     BorderFactory.createRaisedBevelBorder();

    String userName = System.getProperty("user.name");
    String userDir = System.getProperty("userdir");

    ShufDBManager dbManager = ShufDBManager.getdbManager();
    DBCommunInfo info = new DBCommunInfo();

    JTextField totalEntries;
    JLabel stopButton;
    boolean stopPressed = false;
    final String totalEntryText = "Total entries to import";
    public final static int ONE_SECOND = 1000;

    protected Color m_bgColor = Global.BGCOLOR;

    /************************************************** <pre>
     * Summary: Constructor, Add label and text fields to dialog box
     *
     </pre> **************************************************/
    public LocatorBrowser(SessionShare sshare) {

        this.sshare = sshare;
        tel = this;
        try {

            InetAddress inetAddress = InetAddress.getLocalHost();
            localHost = inetAddress.getHostName();

            setOpaque(true);
            setLayout(new BorderLayout());

            // persistence file ParentDirs contains the list of parent dirs
            // and the dimension of the split pane.
            readPersistence();

            upDir = new JButton();
            upDir.setIcon(Util.getImageIcon("blueUpArrow.gif"));
            upDir.setBorder(m_raisedBorder);

            
            // set tooltip to where this would take us, ie root -1 level
             int last = root.lastIndexOf(File.separator);
             if(last == 0) 
                 upToolTip = File.separator;
             else 
                 upToolTip = root.substring(0, last);
             upDir.setToolTipText(upToolTip);

            setMinimumSize(new Dimension(0,0));
            spane = new JScrollPane();
            updateTree(root);

            // Go Up One Level
            upDir.addMouseListener(new MouseAdapter() 
                {
                    public void mouseClicked(MouseEvent evt) {
                        String rootPath = LocatorBrowser.getRootPath();
                        if(!rootPath.equals("/") && 
                                      !rootPath.equals(UtilB.SFUDIR_WINDOWS)) {
                            String path;

                            int last = rootPath.lastIndexOf(File.separator);
                            if(last == 0) {
                                if(UtilB.OSNAME.startsWith("Windows"))
                                    path = UtilB.SFUDIR_WINDOWS;
                                else
                                    path = File.separator;
                            }
                            else 
                                path = rootPath.substring(0, last);

                            updateTree(path);
                        }
                    }
                });

            JButton homeDir = new JButton("Home");
            homeDir.setBorder(m_raisedBorder);


            // Go to Users data dir when clicked
            homeDir.addMouseListener(new MouseAdapter() 
                {
                    public void mouseClicked(MouseEvent evt) {
                        String path;
                        path = FileUtil.openPath("USER" +File.separator+"DATA");
                        // Go to Users data dir when clicked
                        updateTree(path);
                    }
                });

            JButton cdDir = new JButton("CD/DVD");
            cdDir.setBorder(m_raisedBorder);
            // set tooltip to where this would take us
            cdDir.setToolTipText("/cdrom");
            Font font11=DisplayOptions.getFont("Dialog", Font.PLAIN, 11);
            cdDir.setFont(font11);

            cdDir.addMouseListener(new MouseAdapter() 
                {
                    public void mouseClicked(MouseEvent evt) {
                        String path = "/cdrom";
                        // Go to cd dir when clicked
                        updateTree(path);
                    }
                });

            // Button which can save the current dir and go back to it.
            ExpPanel exp=Util.getDefaultExp();
            dirBtn1 = new VToolBarButton(sshare, exp, "dirBtn1");
            // Vnmr cmd to execute when button is pressed
            String strVC = "vnmrjcmd(\'LOC setBrowserDir 1\')";
            // Vnmr cmd to execute when button is pressed and held 3 sec
            String strSetVC = "vnmrjcmd(\'LOC saveBrowserBntDir 1\')";
            // Get the dir name to use for a label
            // Get just the last directory level name.  If nothing was read
            // in from persistence file, Call it Dir 1
            String strLabel="";
            String strToolTip;
            int index;
            if(btnDir1.length() >1) {
                index = btnDir1.lastIndexOf(File.separator);
                if(index > 0)
                    strLabel = btnDir1.substring(index+1);
                strToolTip = new String(btnDir1);
            }
            else {
                strLabel = "Dir 1";
                strToolTip = Util.getLabel("_Press_and_Hold") + " " + Util.getLabel("_Directory");
            }

            dirBtn1.setAttribute(VObjDef.ICON, strLabel);
            dirBtn1.setAttribute(VObjDef.BGCOLOR, null);
            dirBtn1.setAttribute(VObjDef.TOOL_TIP, strToolTip);
            dirBtn1.setAttribute(VObjDef.CMD, strVC);
            dirBtn1.setAttribute(VObjDef.SET_VC, strSetVC);
            dirBtn1.setBorder(m_raisedBorder);

            // Another Button which can save the current dir and go back to it.
            dirBtn2 = new VToolBarButton(sshare, exp, "dirBtn2");
            // Vnmr cmd to execute when button is pressed.  This cmd will
            // execute setBrowserToDirBtnPath()
            String strVC2 = "vnmrjcmd(\'LOC setBrowserDir 2\')";
            // Vnmr cmd to execute when button is pressed and held 3 sec
            // This cmd will execute setDirBtnPath()
            String strSetVC2 = "vnmrjcmd(\'LOC saveBrowserBntDir 2\')";
            // Get the dir name to use for a label
            // Get just the last directory level name
            String strLabel2="";
            String strToolTip2;
            if(btnDir2.length() >1) {
                index = btnDir2.lastIndexOf(File.separator);
                if(index > 0)
                    strLabel2 = btnDir2.substring(index+1);
                strToolTip2 = new String(btnDir2);
            }
            else {
                strLabel2 = "Dir 2";
                strToolTip2 =  Util.getLabel("_Press_and_Hold") + " " + Util.getLabel("_Directory");
            }

            dirBtn2.setAttribute(VObjDef.ICON, strLabel2);
            dirBtn2.setAttribute(VObjDef.BGCOLOR, null);
            dirBtn2.setAttribute(VObjDef.TOOL_TIP, strToolTip2);
            dirBtn2.setAttribute(VObjDef.CMD, strVC2);
            dirBtn2.setAttribute(VObjDef.SET_VC, strSetVC2);
            dirBtn2.setBorder(m_raisedBorder);


            // Control Panel for changing directories
            GridLayout gridLayout = new GridLayout(1, 3);
            JPanel ctrlPanel = new JPanel(gridLayout);
            ctrlPanel.add(upDir);
            ctrlPanel.add(cdDir);
            ctrlPanel.add(dirBtn1);
            ctrlPanel.add(dirBtn2);

            add(ctrlPanel, BorderLayout.NORTH);

            add(spane);


        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
        
        // Keep in static for callers who do not have this object.
        // They should check for null before using.
        locatorBrowser = this;

    }


    /* We need the displayed tree to be only '1' deep.  If we were to have
       a tree with multiple levels opened and multiple items opened, we 
       would not know which one to use for the locator filter.   I did not
       see a way to specify a tree to do this, so, when a directory is 
       opened, we will re-create the tree at that level and redisplay.
    */
       
    public void updateTree(String newRoot) {
        if(newRoot.length() < 1)
            return;

         root = newRoot;
         System.gc();
         System.runFinalization();
         treeModel = new FileSystemModelBrowser(root);
         tree = new BrowserJTree(treeModel);
         tree.addTreeExpansionListener(tel);
         tree.addTreeWillExpandListener(this);
         tree.setEditable(true);
         tree.addKeyListener(this);

         spane.setViewportView(tree);

         // Set the BrowserJTree as a drop listener.  I want to be able
         // to drag a ShufflerItem from the Locator to the browser and
         // have the browser change to the directory of this item.
         dropListener = new BrowserDropTargetListener();
         dropTarget = new DropTarget(tree, dropListener);

         dragSource = new DragSource();
         dragSource.createDefaultDragGestureRecognizer(tree,
                                          DnDConstants.ACTION_COPY,
                                          new BrowserDragGestureListener());
         tree.addMouseListener(new MouseAdapter() 
             {
                 public void mouseClicked(MouseEvent evt) {

                     int clickCount = evt.getClickCount();
                     if(clickCount == 2) {
                         TreePath path = tree.getSelectionPath();
                         // Double clicking on the '+' gives a null path
                         if(path == null) {
                             return;
                         }

                         Object[] obj = path.getPath();
                         // If obj[1] does not exist, they have double 
                         // clicked on the root item.  ignore this.
                         if(obj.length < 2)
                             return;

                         String fullpath =  obj[0] + File.separator + obj[1];

                         // Is this a shuffler type, or just a directory
                         // that needs browsed?
                         String objType = FillDBManager.getType(fullpath);

                         // If windows, we need to convert the windows path
                         // to a unix type path at this point.
                         String uPath;
                         if(UtilB.OSNAME.startsWith("Windows"))
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
                             LoginService loginService = 
                                                   LoginService.getDefault();
                             Hashtable accessHash = 
                                                   loginService.getaccessHash();
                             String curuser = System.getProperty("user.name");
                             Access access = (Access)accessHash.get(curuser);
                             isAccessible = access.isUserInList(owner);
                         }

                         if(isAccessible) {
                             // Since the code is accustom to dealing with 
                             // ShufflerItem's being D&D, I will just use that 
                             // type and fill it in.
                             ShufflerItem item = new ShufflerItem(fullpath, 
                                                                  "BROWSER");

                             String target = "Default";

                             // If no file, don't try to act on it
                             if(item.fileExists())
                                 item.actOnThisItem(target, "DoubleClick", "");
                             else
                                 Messages.postWarning(hostFullpath 
                                        + " Does Not Exist or is not Mounted.");

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

                     }
                 }
             });

         if(isLocatorOpen()) {
             // Cause the locator to update thus only showing items below 
             // this root path.
             SessionShare sshare = ResultTable.getSshare();
             StatementHistory history = sshare.statementHistory();
             history.updateWithoutNewHistory();
         }

         // Update the tool tip for the up arrow button
         // set tooltip to where this would take us, ie root -1 level
//          int last = root.lastIndexOf("/");
//          if(last == 0) 
//              upToolTip = "/";
//          else 
//              upToolTip = root.substring(0, last);
//          upDir.setToolTipText(upToolTip);


    }


    public void keyPressed(KeyEvent e) {

    }

    /******************************************************************
     * Summary: Catch a return/enter following editing of the root node
     *  and set the edited strings as the new root node.
     *
     *  That is, allow editing of the root node to a new directory path
     *  and immediately go to that new directory.
     *****************************************************************/

    public void keyReleased(KeyEvent e) {

        TreePath path = tree.getSelectionPath();
        // Double clicking on the '+' gives a null path
        if(path == null) {
            return;
        }
        Object[] obj = path.getPath();
        if(obj.length > 1)
            return;

        String fullpath =  obj[0].toString();

        // Only update if a change took place
        if(!fullpath.equals(root)) {
            try {
                // Be sure this new path exists and is a directory
                File file = new File(fullpath);
                if(file.exists() && file.isDirectory()) {
                    // Get the canonical path.  Otherwise the locator will not
                    // be able to match this path
                    String canonicalPath = file.getCanonicalPath();

                    // Do the update
                    updateTree(canonicalPath);
                }
            }
            catch (Exception ex) {}
        }
    }

    public void keyTyped(KeyEvent e) {

    }


    /* Called when a directory expansion is being requested. */
    public void treeExpanded(TreeExpansionEvent event) {
        TreePath treePath = event.getPath();
        int np = treePath.getPathCount();

        // Avoid stack trace if double clicking the root item
        if(np > 1) {
            FileNode fn = (FileNode) treePath.getPathComponent(np-1);
            root = fn.getPath();

            // Have the browser tree start at this root path
            updateTree(root);
        }
    }

    public void treeCollapsed(TreeExpansionEvent event) {
    }


    /* Read the directory for where to start the browser. */
    public void readPersistence() {
        String filepath = FileUtil.openPath("USER/PERSISTENCE/BrowserDir");

        if(filepath != null) {
            BufferedReader in;
            String line;
            String path;
            try {
                File file = new File(filepath);
                in = new BufferedReader(new FileReader(file));
                // File must start with 'Browser's Last Directory'
                if((line = in.readLine()) != null) {
                    if(!line.startsWith("Browser\'s Last Dir")) {
                        Messages.postWarning("The " + filepath + " file is " +
                                             "corrupted and being removed");
                        file = new File(filepath);
                        // Remove the corrupted file.
                        file.delete();
                        root = userDir;
                    }
                }
                root = in.readLine().trim();
                if(in.ready())
                    btnDir1 = in.readLine().trim();
                if(in.ready())
                    btnDir2 = in.readLine().trim();
                in.close();
            }
            catch (Exception ioe) { }
        }
        else {
            root = userDir;
        }

        // Be sure we have some type of valid path
        if(root.length() == 0  || (!root.startsWith(File.separator) 
                               && root.indexOf(":") < 1))
            root = File.separator;
    }

    /* Write a persistence file containing the directory where the
       browser was upon closing vnmrj.
    */
    static public void writePersistence() {

        // If the directory is empty or just slash, don't write the file.
        if(root.length() <= 1)
            return;

        String filepath = FileUtil.savePath("USER/PERSISTENCE/BrowserDir");

        FileWriter fw;
        PrintWriter os;
        try {
              File file = new File(filepath);
              fw = new FileWriter(file);
              os = new PrintWriter(fw);
              os.println("Browser's Last Directories");
              
              os.println(root);

              // Do not write the btn dir unless it has been set.
              if(btnDir1.length() > 1) 
                  os.println(btnDir1);
              if(btnDir2.length() > 1)
                  os.println(btnDir2);
              
              os.close();
        }
        catch(Exception er) {
             Messages.postError("Problem creating  " + filepath);
             Messages.writeStackTrace(er);
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

    public void treeWillCollapse(TreeExpansionEvent evt) 
                                        throws ExpandVetoException{
        String path;

        TreePath treePath = evt.getPath();
        int np = treePath.getPathCount();

        // Less than 2 paths means that this is the root.
        // Do notallow collapse of the root.
        if(np <= 1) {
            throw new ExpandVetoException(evt);
        }

    }


    public boolean isLocatorOpen() {
        VToolPanel toolPanel;
        boolean result;

        // Go through all vToolPanels and see if any have a key by this
        // name in use.
        VTabbedToolPanel vToolPanel = Util.getVTabbedToolPanel();
        ArrayList vToolPanels = vToolPanel.getToolPanels();

        for(int i=0; i < vToolPanels.size(); i++) {
            toolPanel = (VToolPanel) vToolPanels.get(i);
            result = toolPanel.isPaneOpen("Shuffler");
            if(result == true)
                return true;
        }
        return false;
    }

    public static String getRootHostFullpath() {
        Vector hostNpath;
        String dhost, dpath, hostFullpath;

        // If root has not been set yet, it will be an empty string,
        // in that case, just return 'all'
        if(root.length() == 0)
            return "all";

        // Generate the host:fullpath needed
        hostNpath = MountPaths.getPathAndHost(root);
        dhost = (String) hostNpath.get(Shuf.HOST);
        dpath = (String) hostNpath.get(Shuf.PATH);

        hostFullpath = dhost + ":" + dpath;

        return hostFullpath;
    }

    public static String getRootPath() {
        return root;
    }

    class BrowserDragSourceListener implements DragSourceListener {
	public void dragDropEnd (DragSourceDropEvent evt) {}
	public void dragEnter (DragSourceDragEvent evt) {
	}
	public void dragExit (DragSourceEvent evt) {
	}
	public void dragOver (DragSourceDragEvent evt) {}
	public void dropActionChanged (DragSourceDragEvent evt) {}
    } // class BrowserDragSourceListener

    class BrowserDragGestureListener implements DragGestureListener {
	public void dragGestureRecognized(DragGestureEvent evt) {
            LoginService   loginService;
            Hashtable      accessHash;
            Access         access;
            String         owner;
            String         curuser;
            boolean        isAccessible=true;
            

            FillDBManager dbm = FillDBManager.fillDBManager;
            TreePath path = tree.getSelectionPath();
            Object[] obj = path.getPath();
            String fullpath =  obj[0] + File.separator + obj[1];
            String filename =  obj[1].toString();

            // If this is not a locator recognized objType, then
            // disallow the drag.
            String type = FillDBManager.getType(fullpath);
            if(type.equals("?")) {
                return;
            }

            String hostFullpath = localHost + ":" + fullpath;

            // We need to only allow dragging of files that this user
            // has access to.  In Java I cannot find any way of finding the
            // owner of a given file.  However, if the file is in the locator,
            // I can get its owner from there.  So, this restriction will
            // only work for files in the database.
            // Get the owner of hostFullpath from the DB
            owner = dbm.getAttributeValueNoError(type, fullpath, 
                                                 localHost, "owner");

            // If the file is not in the DB or any other problem, 
            // allow this file.
            if(owner.length() == 0)
                isAccessible = true;
            else {
                loginService = LoginService.getDefault();
                accessHash = loginService.getaccessHash();
                curuser = System.getProperty("user.name");
                access = (Access)accessHash.get(curuser);
                isAccessible = access.isUserInList(owner);
            }

            // If the file is accessible by this user, allow the drag
            if(isAccessible) {
                // Since the code is accustom to dealing with ShufflerItem's
                // being D&D, I will just use that type and fill it in.
                // This allows ShufflerItem.actOnThisItem() and the macro
                // locaction to operate normally and take care of this item.
                ShufflerItem item = new ShufflerItem(fullpath, "BROWSER");

                if(item.fileExists()) {
                    // Pass TreePath array for the Drag and Drop item.
                    LocalRefSelection ref = new LocalRefSelection(item);
                    // We pass this item on to be caughted where this
                    // is dropped.
                    dragSource.startDrag(evt, null, ref, 
                                         new BrowserDragSourceListener());
                }
            }
            else {
                // If the file is not accessible, do not allow the drag,
                // and tell the user why.
                Messages.postWarning("Access denied to this user's ("
                                     + owner + ") files.  \n    Have this "
                                     + "user added to your access list by the "
                                     + "System Admin if access is needed.");
            }

	}
    } // class BrowserDragGestureListener


    /******************************************************************
     * Summary: If an item is dragged from the locator to the browser,
     *  move the browser to the directory where that item exists.
     *
     *
     *****************************************************************/
    class BrowserDropTargetListener implements DropTargetListener {
        public void dragEnter(DropTargetDragEvent evt) { }
        public void dragExit(DropTargetEvent evt) { }
        public void dragOver(DropTargetDragEvent evt) { }
        public void dropActionChanged (DropTargetDragEvent evt) {}
        public void drop(DropTargetDropEvent evt) {
            try {
                Transferable tr = evt.getTransferable();
                Object obj =
                    tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
                if (obj == null) {
                    evt.rejectDrop();
                    return;
                }
		else if (obj instanceof ShufflerItem) {
                    ShufflerItem item = (ShufflerItem) obj;

                    // Get the directory where the dragged item exists.
                    String fullpath = item.getFullpath();

                    // Bail out if no name
                    if(fullpath.length() == 0) {
                        evt.rejectDrop();
                        return;
                    }

                    int index = fullpath.lastIndexOf(File.separator);
                    String directory = fullpath.substring(0,index);

                    // Update the browser to start at this directory
                    updateTree(directory);
                }
            }
            catch(Exception e) {
                Messages.writeStackTrace(e);
            }
        }
    }

    // Save the current root path as the path for the desired button
    public void setDirBtnPath(int whichOne) {
        if(whichOne == 1) {
            btnDir1 = new String(root);
            String strToolTip = new String(btnDir1);
            dirBtn1.setAttribute(VObjDef.TOOL_TIP, strToolTip);

            // Get the dir name to use for a label
            // Get just the last directory level name
            String strLabel="";
            int index = btnDir1.lastIndexOf(File.separator);
            if(index > 0)
                strLabel = btnDir1.substring(index+1);
            dirBtn1.setAttribute(VObjDef.ICON, strLabel);

        }
        else if(whichOne == 2) {
            btnDir2 = new String(root);
            String strToolTip = new String(btnDir2);
            dirBtn2.setAttribute(VObjDef.TOOL_TIP, strToolTip);

            // Get the dir name to use for a label
            // Get just the last directory level name
            String strLabel="";
            int index = btnDir2.lastIndexOf(File.separator);
            if(index > 0)
                strLabel = btnDir2.substring(index+1);
            dirBtn2.setAttribute(VObjDef.ICON, strLabel);

        }
        
        if(DebugOutput.isSetFor("locatorbrowser")) {
            Messages.postDebug("locatorBrowser.setDirBtnPath set btnDir"
                               + whichOne + " to " + root);
        }
    }

    // Set the Browser root to the path for the indicated button
    public void setBrowserToDirBtnPath(int whichOne) {
        if(whichOne == 1) {
            updateTree(btnDir1);
            if(DebugOutput.isSetFor("locatorbrowser")) {
                Messages.postDebug("locatorBrowser.setBrowserToDirBtnPath "
                                   + "set root to " + btnDir1);
            }
                                       

        }
        else if(whichOne == 2) {
            updateTree(btnDir2);
            if(DebugOutput.isSetFor("locatorbrowser")) {
                Messages.postDebug("locatorBrowser.setBrowserToDirBtnPath "
                                   + "set root to " + btnDir2);
            }
        }
    }


    static public LocatorBrowser getLocatorBrowser() {
        return locatorBrowser;
    }
}

/********************************************************** <pre>
 * Summary: The sole purpose of creating this class is so that I could 
 *          allow only the root node to be editable. 
 * 
 </pre> **********************************************************/

class BrowserJTree extends JTree {

    public BrowserJTree(TreeModel newModel) {
        super(newModel);
    }

    // Override this method in JTree and specify that only the root
    // node is editable
    public boolean isPathEditable(TreePath path) {
        Object[] obj = path.getPath();

        // If we have the root node, obj[1] will not exist.
        if(obj.length < 2)
            // Root is editable
            return true;
        else
            // Nothing else is editable
            return false;
    }


}


