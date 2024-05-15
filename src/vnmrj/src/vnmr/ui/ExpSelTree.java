/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.Transferable;
import java.awt.datatransfer.UnsupportedFlavorException;
import java.awt.dnd.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.*;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.Vector;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.text.Position;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeCellRenderer;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeNode;
import javax.swing.tree.TreePath;
import java.util.List;
import java.awt.event.*;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import org.xml.sax.Attributes;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.DefaultHandler;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;
import vnmr.admin.util.*;
import vnmr.util.Util;

// The Experiment Selector Code for the Button panel was copied and
// modified for the Tree version.  Thus, many of the names and comments
// will refer to the Buttons version.  No attempt has been made to 
// modify all of the comments.


// Each button in each tab panel can be either a protocol, or a menu.
// There can be two levels of menu.  The ExperimentSelector.xml file can
// specify for each protocol, a tab, menu1 and menu2 for its location.

// The central information structure for this is as follows:
// All tab, menu1, menu2 and protocols will be have ExpSelEntry items
// created for them.  Each tab item will contain an entryList of
// ExpSelEntry items that will exist in that tab panel.  An item with
// an empty entryList will be a protocol.  If entryList is not empty, this
// item will be a menu and its entryList is the items in this menu.  Menus within
// the tab level are from the menu1 designation.  Each of these menu items
// will contain an entryList of ExpSelEntry items. Within these lists can
// be one more level of list from the menu2 designations.  Each of those
// can contain a list of ExpSelEntry item, but no further menu levels are
// allowed.  If there are menu2 level menus, then tabList will be a list
// four deep of HashArrayList's

// Shared directories (not system nor user) can also have ExperimentSelector.xml
// files in the interface directory.  After reading the user file and the
// system file, go through the shared directories and find those files
// and read them.  

// After reading all ExperimentSelector.xml, we need to look through all
// user and shared protocols for protocols that do not have enties from
// the ExperimentSelector.xml files.  If any user protocols are found,
// put them in a tab called User.  If any shared protocols are found, put
// them in a tab by the directory name/label.

// The ExperimentSelector_user.xml file can contain a first line such as 
// <ProtocolList userOnly="false" tooltip="label" labelWithCategory="false" iconForCategory='false'>
// To set various flags.  These were enabled like this for applab testing
// so not all have been fully tested.  The defaults within the code are
// the values given in the example line above.

public class ExpSelTree extends JPanel implements   PropertyChangeListener {
    public static ExpSelTree expSelTree=null;
    // tabList is a list of ExpSelEntry items which are each, one of
    // the tab panels. Each one contains a list of ExpSelEntry items in
    // that panel.
    static HashArrayList tabList = new HashArrayList();

    static boolean forceUpdate = true;
    static boolean bWaitLogin = false;

    // List of protocols from the ExperimentSelector file
    static public ArrayList<ExpSelEntry> protocolList = new ArrayList<ExpSelEntry>();
    
    
    // List of the names for each level that exist
    static public ArrayList<String> tabNameList = new ArrayList<String>();
    static public ArrayList<String> tabOrderList = new ArrayList<String>();
    static ArrayList<String> menu1NameList = new ArrayList<String>();
    static ArrayList<String> menu2NameList = new ArrayList<String>();

    static final int NOTDUP = 0;
    static final int COMPLETEDUP = 1;
    static final int DUPALLBUTPROTO = 2;
    static final int FILENUM = 12;

    // true -> Allow only protocols from this users ExperimentSelector_xxx.xml
    // file and not the system files
    static boolean userOnly = false;

    // true -> do not use menu1 and menu2
    static boolean sharedFlat = false;
    
    // true -> do not use menu1 and menu2
    static boolean userFlat = false;

    // true -> Show protocols that are not specified in ExperimentSelector.xml
    // in a tab with the name of that directory (ie., a catchall tab)
    static boolean showNonUserNonSpecifiedAppdirProtoInCatchAll = false;
    static boolean showUserNonSpecifiedAppdirProtoInCatchAll = true;

    // true -> put user and shared protocols in the normal panel with the
    // system protocols.  false -> put User protocols in User menus and
    // shared protocols in tabs with directory label name.
    static boolean allWithSystem = true;

    static String tooltipMode = "label";

    static boolean labelWithCategory = false;

    static boolean iconForCategory = false;
    private static boolean firstCall = true;

    private static long lastMod[] = new long[FILENUM];
    private static long fileLength[] = new long[FILENUM];

    Dimension zero = new Dimension(0, 0);

    Font labelFont;
    
    String fontName;

    static JScrollPane jsp = null;

    SessionShare sshare;

    String protocolType, protocolName;

    private static Timer times = null;

    private static int updateCount = 0;

    DroppableList listPanel;

    static public  UpdateCards updateCards = null;


    static String setPosition = "center";

    // protected boolean m_bSetInvisible = false;
    JPopupMenu popup = null;

    DragSource dragSource;
    LocalRefSelection lrs;

    public static ExpSelEntry lastDraggedEntry=null;
    
    public static String trashWhat="cancel";
    
    static public ArrayList<String> locTabList = new ArrayList<String>();
    public static JTree tree;
    private String uiOperator = null;
    private DefaultMutableTreeNode rootNode;
    private DefaultTreeModel treeModel;
    private static DataFlavor dataFlavor = null;
    JTextField searchText;
    DefaultTreeCellRenderer renderer;
    static private DefaultMutableTreeNode selNode;
    static private TreePath selPath=null;


    
    

    // constructor
    public ExpSelTree(SessionShare sshare) {
        // JPanel contructor
        super(new BorderLayout());
        this.sshare = sshare;
        expSelTree = this;
        
        if (DebugOutput.isSetFor("expselector")) {
            Messages.postDebug("ExpSelector: Creating Experiment Selector");
        }

        dataFlavor = LocalRefSelection.LOCALREF_FLAVOR;


        // Create the ActionListener which is called every 10 sec to see
        // if anything needs to be updated. Save this for use by
        // external functions that need to force the update.
        updateCards = new UpdateCards();

        // Create the top root node
        rootNode = new DefaultMutableTreeNode(vnmr.util.Util.getLabel("Experiment Selector"));
        treeModel = new DefaultTreeModel(rootNode);

        // Create the JTree
        tree = new JTree(rootNode);
        tree.setModel(treeModel);
        tree.setExpandsSelectedPaths(true);

        // Don't show the root node
        tree.setRootVisible(false);
        // Show the expand/collapse control for the top level nodes
        tree.setShowsRootHandles(true);
        
        tree.setDragEnabled(true);
        TransferHandler transferHandler = new ESTransferHandler();
        tree.setTransferHandler(transferHandler);

        tree.addMouseListener(new treeMouseListener());

        // Turn off the folder icon.  It is cleaner with on icons here
        renderer = (DefaultTreeCellRenderer)tree.getCellRenderer();
//        renderer = new DefaultTreeCellRenderer();
        renderer.setClosedIcon(null);
        renderer.setOpenIcon(null);
        renderer.setLeafIcon(null);
        tree.setCellRenderer(renderer);
 
        jsp = new JScrollPane();

        // Don't fill the tree now.  Let updateCards do it via timer
        // Otherwise, it is getting done twice.
//        createTreeNodes();

        this.add(jsp, BorderLayout.CENTER);
        JPanel searchPanel = createSearchPanel();
        
        this.add(searchPanel, BorderLayout.PAGE_END);

        DisplayOptions.addChangeListener(this);
        
        /**********
        times = new Timer(5000, updateCards);
        times.setInitialDelay(6000);
        times.start();
        **********/
    }

    static public ExpSelTree getInstance() {
        return expSelTree;
    }

    /* PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt) {
//System.out.println(evt.getPropertyName());
        String strProperty = evt.getPropertyName();
        if (strProperty == null)
            return;
        
        if (strProperty.indexOf("DisplayOptions") >= 0) {     

// TODO
// Do nothing with color right now.  Hung is working on colors and will have info on
// how to deal with colors here.
            
            // PlainText is set in
            // Display Options panel -> "Labels" ->  "Plain Text"
//            renderer.setTextNonSelectionColor(DisplayOptions.getColor("PlainText"));
//            renderer.setTextSelectionColor(DisplayOptions.getColor("PlainText"));
            // VJSelectedText is set in
            // Display Options panel -> "UI Colors" -> "Text" -> "Selected
//            renderer.setBackgroundSelectionColor(DisplayOptions.getColor("VJSelectedText"));
            // VJEntryBG is set in 
            // Display Options panel -> "UI Colors" -> "Background" -> "Entry"
//            renderer.setBackgroundNonSelectionColor(DisplayOptions.getColor("VJEntryBG"));
            
            SwingUtilities.updateComponentTreeUI(this);
            
            // Turn off the folder icon.  It is cleaner with no icons
            // This has to be done every time after changing themes
            renderer = (DefaultTreeCellRenderer)tree.getCellRenderer();
            renderer.setClosedIcon(null);
            renderer.setOpenIcon(null);
            renderer.setLeafIcon(null);
            tree.setCellRenderer(renderer);

        }
    }

    // Utility to get the AppDirs list
    public ArrayList<String> getAppDirs() {
        ArrayList<String> dirs = FileUtil.getAppDirs();
        return (dirs);
    }


    // Decide whether a fullpath is System, User or Shared (Other)
    // If Shared, return the appDirLabel for this directory
    public String getAppDirLabel(String path) {
        // If no label found, default to Shared
        String label = "Shared";
        if(path.startsWith(FileUtil.sysdir()))
            return "System";
        else if(path.startsWith(FileUtil.usrdir()))
            return "User";
        else {
            // Must be a Shared directory.  Findout which index in the
            // appDirs this is and then get the label for that index.
            ArrayList<String> appDirs = FileUtil.getAppDirs();
            ArrayList<String> labels = FileUtil.getAppDirLabels();
            for (int k = 0; k < appDirs.size(); k++) {
                String dir = appDirs.get(k);
                if(path.startsWith(dir)) {
                    label = labels.get(k);
                    return label;
                }
            }
        }
        return label;
        
    }

    // This is to get the 'system' protocol of this name even though
    // the appdirs scheme may give a user or a shared protocol.
    // Go through the appdirs and only look in the system ones.
    String getSystemProtPath(String name) {
        ArrayList<String> appDirs = FileUtil.getAppDirs();
        String relativePath = FileUtil.getSymbolicPath("PROTOCOLS");
        for (int k = 0; k < appDirs.size(); k++) {
            String dir = appDirs.get(k);
            // Just take the system dirs
            if (dir.startsWith(FileUtil.sysdir())) {
                String path = dir  + File.separator + relativePath 
                    + File.separator + name;
                // Be sure it has the .xml suffix
                if(!path.endsWith(".xml"))
                    path = path + ".xml";
                UNFile file = new UNFile(path);
                if(file.exists())
                    return path;
            }
        }
        // Not found
        return null;
    }



    // Cleanup
    public void finalize() {
        if (times != null)
            times.stop();
        times = null;
        if (jsp != null)
            jsp.removeAll();
        jsp = null;
    }


    // Create an ExpSelEntry from the comma separate command string
    // used as the ActionCmd

    private ExpSelEntry parseActionCmd(String cmd) {
        StringTokenizer tok;
        ExpSelEntry entry = new ExpSelEntry();

        if(cmd == null)
            return null;
        tok = new StringTokenizer(cmd, ",");
        int count = tok.countTokens();
        if(count < 4) {
            Messages.postWarning("Problem removing Exp Sel entry");
            return null;
        }
        entry.fullpath = tok.nextToken();
        entry.name = tok.nextToken();
        entry.label = tok.nextToken();
        entry.tab = tok.nextToken();
        if(tok.hasMoreTokens())
            entry.menu1 = tok.nextToken();
        else
            entry.menu1 = "";
        
        if(tok.hasMoreTokens())
            entry.menu2 = tok.nextToken();
        else
            entry.menu2 = "";
       
        return entry;

    }



    static public void updateExpSel() {
        // Force an update of the ExpSel
        /*****
        updateCount = 1;
        if(updateCards != null) {
            ActionEvent aevt = new ActionEvent(updateCards, 1, "force");
            updateCards.actionPerformed(aevt);
        }
        *****/
        setForceUpdate(true);
    }

    /* DragSourceListener */
    public void dragDropEnd (DragSourceDropEvent e) {
    }

    public void dragEnter (DragSourceDragEvent e) {
    }

    public void dragExit (DragSourceEvent e) {
    }

    public void dragOver (DragSourceDragEvent e) {
    }

    public void dropActionChanged (DragSourceDragEvent e) {
    }

    /* DragGestureListener */
 
    static public void setForceUpdate(boolean value) {
        if (value) {
           forceUpdate = value;
           updateCount = 0;
           if (times != null) {
               times.setInitialDelay(2000);
               times.restart();
           }
        }
        else {
           if (updateCount > 3)
               updateCount = 3;
           if (times != null) {
               if (!times.isRunning())
                   times.restart();
           }
        }
    }

    static public synchronized void waitLogin(boolean bWait) {
        bWaitLogin = bWait;
        if (bWait) {
            if (times != null)
                times.stop();
        }
        if (!bWait) {
            updateCount = 0;
            for (int i = 0; i < FILENUM; i++) {
                lastMod[i] = 0;
                fileLength[i] = 0;
            }
        }
    }


    // Called when pnew happens so we can check expselnu and expselu
    // for changes. Set the local variable and force an update of the exp selector
    
    // 3/16/10, the variables showUser... and showNon... are currently not
    // used.  The logic was moved to the macro updateExpSelector.  Here we
    // still need to flag the "update" so that changes to the vnmr parameters
    // will be caught and cause an update to the ES.
    static public void updatePnewValues(Vector<String> parVal) {
        int size;
        boolean update=false;

        size = parVal.size();
        for (int i = 0; i < size/2; i++) {
            String paramName = parVal.elementAt(i);
            if (paramName.equals("expselnu")) {
                // Found it, now get the value from the last half of the Vector
                String val = parVal.elementAt(size/2 +i);
                if(val.equals("1"))
                    showNonUserNonSpecifiedAppdirProtoInCatchAll = true;
                else
                    showNonUserNonSpecifiedAppdirProtoInCatchAll = false;

            }
            else if (paramName.equals("expselu")) {
                // Found it, now get the value from the last half of the Vector
                String val = parVal.elementAt(size/2 +i);
                if(val.equals("1"))
                    showUserNonSpecifiedAppdirProtoInCatchAll = true;
                else
                    showUserNonSpecifiedAppdirProtoInCatchAll = false;
                // Unfortunately, we get a call from pnew for each parameter
                // so only trigger an update for one of them
                update=true;
            }
            
        }

        if(update) {
            // Run the updateExpSelector macro to create (if needed) a new ES_op.xml file
            Util.getAppIF().sendToVnmr("updateExpSelector");

            // the updateExpEelector will write a new ExperimentSelector_op.xm
            // file which will trigger the ES to update.
        }

    }

    // Create the search functionality in a separate JPanel
    public JPanel createSearchPanel() {
        JPanel panel = new JPanel();
        ImageIcon findIcon =  Util.getVnmrImageIcon("search_20.png");
        ImageIcon collapseIcon =  Util.getVnmrImageIcon("collapse_20.png");
        ImageIcon expandIcon =  Util.getVnmrImageIcon("expand_20.png");
        searchText = new JTextField(15);
        JButton searchBtn = new JButton(findIcon);
        JButton collapseBtn = new JButton(collapseIcon);
        JButton expandBtn = new JButton(expandIcon);
        EmptyBorder border = new EmptyBorder(0,0,0,0);
        searchBtn.setBorder(border);
        collapseBtn.setBorder(border);
        expandBtn.setBorder(border);

        searchBtn.addActionListener(new SearchListener());
        expandBtn.addActionListener(new SearchListener());
        collapseBtn.addActionListener(new SearchListener());
        searchText.addActionListener(new SearchListener());
        searchBtn.setActionCommand("search");
        expandBtn.setActionCommand("expand");
        collapseBtn.setActionCommand("collapse");
        searchText.setActionCommand("text");
        searchBtn.setToolTipText("Do Search");
        expandBtn.setToolTipText("Expand All");
        collapseBtn.setToolTipText("Collapse All");
        searchText.setToolTipText("Enter Search String");
        
        // The following is so that the text field stretches with the panel 
        // width.
        SpringLayout layout = new SpringLayout();
        panel.setLayout(layout);
        panel.add(searchText);

        panel.add(expandBtn);
        panel.add(searchBtn);
        panel.add(collapseBtn);

        //Adjust constraints for the label so it's at (5,5).
        layout.putConstraint(SpringLayout.WEST, searchText,
                5,
                SpringLayout.WEST, panel);
        layout.putConstraint(SpringLayout.NORTH, searchText,
                5,
                SpringLayout.NORTH, panel);

        layout.putConstraint(SpringLayout.WEST, searchBtn,
                5,
                SpringLayout.EAST, searchText);
        layout.putConstraint(SpringLayout.NORTH, searchBtn,
                5,
                SpringLayout.NORTH, panel);

        layout.putConstraint(SpringLayout.WEST, collapseBtn,
                5,
                SpringLayout.EAST, searchBtn);
        layout.putConstraint(SpringLayout.NORTH, collapseBtn,
                5,
                SpringLayout.NORTH, panel);

        layout.putConstraint(SpringLayout.WEST, expandBtn,
                5,
                SpringLayout.EAST, collapseBtn);
        layout.putConstraint(SpringLayout.NORTH, expandBtn,
                5,
                SpringLayout.NORTH, panel);

        layout.putConstraint(SpringLayout.EAST, panel,
                5,
                SpringLayout.EAST, expandBtn);

        // Set bottom to fit largest item.
        layout.putConstraint(SpringLayout.SOUTH, panel,
                1,
                SpringLayout.SOUTH, searchBtn);




        return panel;
    }

    static public synchronized void fillTabList() {
        // clear out the list of protocols and start over
        protocolList.clear();
        tabList.clear();
        tabNameList.clear();
        tabOrderList.clear();

            
            
        // Clear/default the mode flags
        // true -> Allow only protocols from this users
        // ExperimentSelector_xxx.xml file and not the system files
        userOnly = false;
        // true -> do not use menu1 and menu2
        sharedFlat = false;
        // true -> do not use menu1 and menu2
        userFlat = false;
        allWithSystem = true;
        tooltipMode = "label";
        labelWithCategory = false;
        iconForCategory = false;

        // Now we are ready to really start updating if that was
        // determined to be necessary

        if (DebugOutput.isSetFor("expselector")) {
            String operator = Util.getCurrOperatorName();        
            Messages.postDebug("UpdateCards: updating Exp Selector for "
                               + operator);
        }

        // fill the protocolList 
        fillProtocolList();
            
        // check the list for protocol names that are duplicated of 
        // menu names and disallow.  checkProtocolList() will output an
        // error message.
        if(!checkProtocolList(protocolList))
            return;
            
        // Now we should have all protocols added into the protocolList
        // Be sure each one exists, be sure each one is approved and
        // then make buttons and menus.


        // rightsList will contain the list of protocols that the user
        // does not want display with an approve = false
        boolean all = true;
        RightsList rightsList = new RightsList(all);

        // Now go through the list of protocols we have and see if they exist
        // and if they are supposed to be displayed. If so, create the buttons
        for (int i = 0; i < protocolList.size(); i++) {
            ExpSelEntry protocol = protocolList.get(i);

            // If the operator has a RightsConfig.txt persistence file,
            // it may request that some of these protocols NOT be
            // displayed. Test the list from persistence, and if a
            // protocol is tagged 'false', leave it out of this list.
            // For protocols, the keyword in rightsList is equal to name,
            // so use name here for the keyword
            if (rightsList.approveIsFalse(protocol.name))
                continue;
                
            if(protocol.noShow != null && protocol.noShow.equals("true"))
                continue;

            // See if the protocol exists in one of the appdir directories
            // If not, don't put it into the exp selector

            String ppath = "PROTOCOLS" + File.separator + protocol.name
                + ".xml";
            // Returns null if file not found
            String fullpath = FileUtil.openPath(ppath);
            if (fullpath != null) {
                // File found, set the fullpath in the item, since we have
                // it here.
                protocol.fullpath = fullpath;
                // If the tab is not set, then skip this one
                if (protocol.tab == null || protocol.tab.equals("")) {
                    Messages.postWarning(protocol.name
                                         + " Missing 'tab' designation.");
                    continue;
                }
                try {
                    // Does the tab designated exist yet?
                    ExpSelEntry tab;
                    if (!tabList.containsKey(protocol.tab)) {
                        // This tab does not exist yet, make it
                        tab = new ExpSelEntry();
                        tab.label = new String(protocol.tab);
                        tab.entryList = new HashArrayList();
                        // Add to tabList
                        tabList.put(protocol.tab, tab);
                            
                    }
                    else {
                        // The tab already exists, get this one
                        tab = (ExpSelEntry) tabList.get(protocol.tab);
                            
                    }
                    // Save a list of tab labels for the panel menu
                    // and for the tab order editing panel
                    if(!locTabList.contains(tab.label))
                        locTabList.add(tab.label);

                    // Now we have the ExpSelEntry item for this tab,
                    // Check to see if the current protocolList item goes
                    // directly in this tab, or if a menu1 is specified.
                    ExpSelEntry menu1;
                    ExpSelEntry menu2;
                    if (protocol.menu1 == null || protocol.menu1.equals("")) {
                        // The protocol goes directly in this tab
                        // See if there is already a menu1 by this name
                        // in this tab, we cannot have two items in the
                        // hashtable with the same key.
                        ExpSelEntry entry = (ExpSelEntry) tab.entryList.get(protocol.label);
                        if(entry == null)
                            // Nothing by that name yet, add it
                            tab.entryList.put(protocol.label, protocol);
                        else {
                            // There is already something by that name.  Is
                            // is the same type item?  If not, give error.
                            // In this case it should either be a protocol
                            // or a menu1.  If it is a menu, fullpath will
                            // not be set.
                            if((entry.fullpath == null  && protocol.fullpath != null) ||
                               (protocol.fullpath == null && entry.fullpath != null)) {
                                // They are not the same type, error out
                                Messages.postError("Cannot specify a protocol and " 
                                                   + "a \'menu1\' of the same label ("
                                                   + protocol.label + ")");
                                continue;
                            }
                        }
                    }
                    else {
                        // The protocol is at least one level down, so we need
                        // to see if the specified menu1 already exists.
                        // If not, create one.
                        ExpSelEntry entry = (ExpSelEntry) tab.entryList.get(protocol.menu1);
                        if (entry == null) {
                            // It does not exist yet
                            menu1 = new ExpSelEntry();
                            menu1.label = new String(protocol.menu1);
                            menu1.entryList = new HashArrayList();
                            // Add to the tab
                            tab.entryList.put(protocol.menu1, menu1);
                                
                            // Save a list of menu1 labels for the panel menu
                            if(!menu1NameList.contains(menu1.label))
                                menu1NameList.add(menu1.label);
  
                        }
                        else {
                            // It exists, is the existing entry a menu1
                            // or a protocol?  If it is a protocol, error out.
                            // We cannot add a menu1 and a protocol of the
                            // same name.  Protocols have fullpath != null
                            if(entry.fullpath != null) {
                                // It is a protocol, error out
                                Messages.postError("Cannot specify a protocol and " 
                                                   + "a \'menu1\' of the same name ("
                                                   + protocol.menu1 + ")");
                                continue;
                            }
                                
                            // It exists, save it
                            menu1 = entry;
                        }
                        // We need to know if the protocol goes directly into
                        // menu1 or is there is another level.
                        // Does it have a menu2 set?
                        if (protocol.menu2 == null
                            || protocol.menu2.equals("")) {
                            // No menu2, it goes in the menu1 level
                            // See if there is already a menu2 by this name.
                            // we cannot have a protocol and a menu2 by the
                            // same name.
                            entry = (ExpSelEntry) menu1.entryList.get(protocol.label);
                            if(entry == null)
                                // Nothing by that name yet, add it
                                menu1.entryList.put(protocol.label, protocol);
                            else {
                                // There is already something by that name.  Is
                                // is the same type item?  If not, give error.
                                // In this case it should either be a protocol
                                // or a menu.  If it is a menu, fullpath will
                                // not be set.
                                if((entry.fullpath == null  && protocol.fullpath != null) ||
                                   (protocol.fullpath == null && entry.fullpath != null)) {
                                    // They are not the same type, error out
                                    Messages.postError("Cannot specify a protocol and " 
                                                       + "a \'menu2\' of the same label ("
                                                       + protocol.label + ")");
                                    continue;
                                }
                            }
                        }
                        else {
                            // Okay, there is a menu2, lets go one more
                            // level.
                            // Does this menu2 exists yet within menu1?
                            entry = (ExpSelEntry) menu1.entryList.get(protocol.menu2);
                            if (entry == null) {
                                // It does not exist yet
                                menu2 = new ExpSelEntry();
                                menu2.label = new String(protocol.menu2);
                                menu2.entryList = new HashArrayList();
                                // Add to menu1
                                menu1.entryList.put(protocol.menu2, menu2);
                                    
                                // Save a list of menu2 labels for the panel menu
                                if(!menu2NameList.contains(menu2.label))
                                    menu2NameList.add(menu2.label);
                            }
                            else {
                                // It exists, is the existing entry a menu2
                                // or a protocol?  If it is a protocol, error out.
                                // We cannot add a menu2 and a protocol of the
                                // same name.  Protocols have fullpath != null
                                if(entry.fullpath != null) {
                                    // They are not the same type, error out
                                    Messages.postError("Cannot specify a protocol and " 
                                                       + "a \'menu2\' of the same name ("
                                                       + protocol.menu2 + ")");
                                    continue;
                                }
                                
                                // It exists, save it
                                menu2 = entry;
                            }
                            // There cannot be any more levels, if we reach
                            // this point, add the protocol.
                            menu2.entryList.put(protocol.label, protocol);
                        }
                    }

                }
                catch (Exception e) {
                    Messages.writeStackTrace(e);
                    // check the next appdir directory
                    continue;
                }

            }
            else {
                // If we did not find this protocol, then log that fact
                Messages.postDebug("Cannot find protocol " + protocol.name
                                   + ".xml" + " for Experiment Selector."
                                   + "\n   Possibly in user\'s profile "
                                   + "or ExperimentSelector.xml file, but not "
                                   + "in protocols dir.");
            }
        }
    }


    static public synchronized void update() {
         if (expSelTree == null)
             return;

         expSelTree.fillTabList();
         // Now we have all of the information we need in the tabList tree
         // We need to create all of the tabs and menus as specified.
         // Go through the list of tabs

         expSelTree.fillTree(tabList);

         // Read the persistence only on the first time the Tree is update/created
         if(firstCall) {
              expSelTree.readPersistence();
              firstCall = false;
         }
    }



//    public void createTreeNodes(ArrayList<ExpSelEntry> pList) {
//        tabList = getTabList(pList);
//        if(tabList == null) {
//            Messages.postWarning("Problem filling Experiment Selector Tree");
//            return;
//        }
//            
//        fillTree(tabList);
//
//    }

    
    static public HashArrayList getTabList() {
            // If empty, try to fill it
        if(tabList.size() == 0)
            fillTabList();
        
            // If still empty, return null
        if(tabList.size() == 0)
            return null;
        else
            return tabList;
    }
    
    // Find the currently selected item (node or leaf) and write a file
    // with one path to that item.
    public static void writePersistence() {
        // If the Exp Sel Tree has not been created, just bail out.
        if(expSelTree == null)
            return;
       
        String selName[] = {"","","","",""};
        DefaultMutableTreeNode node[] = {null, null, null, null, null};
        // One extra path items since the loop tries to get an extra one.
        TreePath path[] = {null, null, null, null, null, null};
        
        path[0] = tree.getSelectionPath();
        for(int i=0; i < 5; i++) {
            if(path[i] != null) {
                node[i] = (DefaultMutableTreeNode) path[i].getLastPathComponent();
                if(node[i] != null) {
                    selName[i] = (String) node[i].getUserObject();
                    path[i+1] = path[i].getParentPath();
                }
            }
        }
        
        // Now we should have the names for all levels and empty strings for those
        // not defined.  The highest level will be the root node which we will not use.
        

        Messages.postDebug(selName[0] + "  " + selName[1] + "  " + selName[2] + "  " + selName[3] + "  " + selName[4]);

        // Be sure something as been set
        if(selName[0].length() == 0 || selName[1].length() == 0) {
            return;
        }
        
        // Write out the file including operator designation
        String operator = Util.getCurrOperatorName();        
        String filepath = FileUtil.savePath("USER/PERSISTENCE/"
                + "ESTreeSelected_" + operator);

        FileWriter fw;
        PrintWriter os=null;
        try {
            File file = new File(filepath);
            fw = new FileWriter(file);
            os = new PrintWriter(fw);
            os.println("Experiment Selector Tree Last Selected Item");

            // Just write out the selected item and its parent.  We should not
            // need 5 levels to define this.
            os.println(selName[0] + ", " + selName[1]);

            os.close();
        }
        catch (Exception er) {
            Messages.postError("Problem creating  " + filepath);
            Messages.writeStackTrace(er);
            if (os != null)
                os.close();
        }
    }
    
    public void readPersistence() {
        // Read the file.  Tokenizer using comma
        // Do this AFTER the tree is complete
        // Perhaps look for this item (first token) and see if it's parent
        // in the current tree matches the 2nd token.  If so, that should
        // be close enough.
        // It should work if a folder was selected instead of a protocol
        
        
        String operator = Util.getCurrOperatorName();        
        String filepath = FileUtil.openPath("USER/PERSISTENCE/"
                + "ESTreeSelected_" + operator);
        if(filepath == null) {
            // If no file or problem with file, just expand the first node
            tree.expandRow(0);
            return;
        }
        
        UNFile file = new UNFile(filepath);
        
        if(file == null || !file.canRead()) {
            tree.expandRow(0);
            return;
        }
        String  inLine=null;
        BufferedReader in=null;
        try {
            in = new BufferedReader(new FileReader(filepath));
            // The first line must have the correct string to identify the file
            inLine = in.readLine();
            if(inLine == null || !inLine.startsWith("Experiment Selector Tree")) {
                tree.expandRow(0);
                return;
            }
            
            // The second line should have the name and parent separated by a comma
            // Parse the line below.
            inLine = in.readLine();    
            
            if(in != null)
                in.close();

        }
        catch (Exception ex) {
            tree.expandRow(0);
            return;
        }
        
        StringTokenizer tokstring;
        String name="", parent="";
        
        // Get the selected item's name and its parent from the second line
        // of the persistence file.
        if(inLine != null) {
            tokstring = new StringTokenizer(inLine, ",");
            if(tokstring.hasMoreTokens())
                name = tokstring.nextToken().trim();
            if(tokstring.hasMoreTokens())
                parent = tokstring.nextToken().trim();
            Messages.postDebug(name + "  " + parent);
        }
        
        // Find the node which has this name (label) and it has this parent
        DefaultMutableTreeNode node = findNode(name, parent);

        if(node != null) {
            // Make it selected and expanded
            TreeNode nodes[] = node.getPath();
            // Convert to TreePath
            TreePath newPath = new TreePath(nodes);
            // Set the new selection
            tree.setSelectionPath(newPath);

            // Make this selected row visible in the scrollpane
            if ((tree.getParent() instanceof JViewport)) {
                JViewport viewport = (JViewport)tree.getParent();
                int selrow = tree.getMinSelectionRow();
                Rectangle rect = tree.getRowBounds(selrow);
                // Set the view to the top, else repeated calls
                // to scrollRectToVisible don't work
                viewport.setViewPosition(new Point (0,0));
                viewport.scrollRectToVisible(rect);
            } 
        }
        else {
            // No match found, just expand the first row
            tree.expandRow(0);
        }
    }

    private boolean isFileChanged(int index, String path) {
        long modTime, len;
        File file;
        boolean bChanged = false;

        if (path != null) {
             file = new File(path);
             modTime = file.lastModified();
             len = file.length();
             if (modTime != lastMod[index] || len != fileLength[index]) {
                 bChanged = true;
                 lastMod[index] = modTime;
                 fileLength[index] = len;
            }
        }
        else if (fileLength[index] != 0) {
            bChanged = true;
            fileLength[index] = 0;
        }

        return bChanged;
    }

    
    // ActionListener called every 10 sec to see if anything needs
    // to be updated. If cmd = force, then update anyway
    public class UpdateCards implements ActionListener {
        // Use firstCall to know whether to readPersistence() after the Tree
        // is created.

        public void actionPerformed(ActionEvent ae) {
            boolean update = false;
            String profile;
            // File profileFile = null;
            int  index;
            
            updateCount++;
            if (updateCount > 5) {
                if (times != null) {
                   if (times.isRunning())
                       times.stop();
                }
            }
            if (DebugOutput.isSetFor("expselector")) {
                Messages.postDebug("UpdateCards: Check to see if update is necessary");
            }

            // If cmd = force, then the operator has changed and we
            // need to force an update.
            String cmd = ae.getActionCommand();
            if (cmd != null && cmd.equals("force")) {
                update = true;
            }
            
            // When the UI is first coming up, AppDirLabels may not have been 
            // filled yet. Get the appdir list and see if the number of entries 
            // is the same.
            ArrayList<String> labels1;
            // Get the appDirLabels list
            labels1 = FileUtil.getAppDirLabels();
        
            // bail out if appdirs not filled yet.  The timer will bring us back
            // shortly to try again.  If the initial timer start is set
            // long enough, we will have this set when we arrive the first
            // time.  This test is just in case things are running slow.
            if(labels1.size() == 0) {
                ArrayList<String> appdirs = FileUtil.getAppDirs();

                if (DebugOutput.isSetFor("expselector"))
                    Messages.postDebug("ExpSelector: AppDirLabels empty, "
                                       + "skipping update this time.  appdirs size = "
                                       + appdirs.size() );
                return;    
            }

            // Check to see if this user has a profile set in the operator
            // login info
            String curOperator = Util.getCurrOperatorName();
            if (uiOperator == null || !(curOperator.equals(uiOperator))) {
                uiOperator = curOperator;
                update = true;
            }

            // Get Profile Name column string for access to the operator data
            String pnStr = vnmr.util.Util.getLabel("_admin_Profile_Name");
            profile = WUserUtil.getOperatordata(curOperator, pnStr);

            // This could be temporary, but DefaultApproveAll does not exist
            // as a profile and AllLiquids does exist, so use it.
            if (profile == null || profile.length() == 0) {
                profile = "AllLiquids";
            }
            else if (profile.equals("DefaultApproveAll")) {
                profile = "AllLiquids";
            }

            // We arrive here about every 10 sec.

            // There is a persistence file for each operator

            index = 0;
            // Check the operators RightsConfig.txt file for changes
            String filepath = FileUtil.openPath("USER" + File.separator
                    + "PERSISTENCE" + File.separator + "RightsConfig_"
                    + curOperator + ".txt");
            if (isFileChanged(index, filepath))
                update = true;

            if(forceUpdate) {
                // Force an update only on the first call
                update = true;
                
                // Reset back to non forcing
                forceUpdate = false;
            }

            // Check the users protocol directory for date change
            // Get the current user object
            String curuser = System.getProperty("user.name");
            LoginService loginService = LoginService.getDefault();
            Hashtable userHash = loginService.getuserHash();
            User user = (User) userHash.get(curuser);
            filepath = FileUtil.userDir(user, "PROTOCOLS");

            index = 1;
            if (isFileChanged(index, filepath))
                update = true;

            // Check the users assigned profile for changes
            /*********
            if (profileFile != null) {
                index = 2;
                modTime = profileFile.lastModified();
                // lastMod[2] is the profile
                if (modTime != lastMod[2]) {
                    update = true;
                    lastMod[2] = modTime;
                }
            }
            *********/

            index = 3;
            // Check the users ExperimentSelector_xxx.xml file for changes
            filepath = FileUtil.openPath("USER/INTERFACE/ExperimentSelector_"
                    + curOperator + ".xml");
            if (isFileChanged(index, filepath))
                update = true;

            index = 4;
                // The operator must not have a file.  Try the User's file
            String userName = user.getAccountName();
            filepath = FileUtil.openPath("USER/INTERFACE/ExperimentSelector_"
                                             + userName + ".xml");
            if (isFileChanged(index, filepath))
                update = true;

            index = 5;
            // Check the users appdir file for changes
            filepath = FileUtil.openPath("USER" + File.separator
                    + "PERSISTENCE" + File.separator + "appdir_"
                    + curOperator);

            if (isFileChanged(index, filepath))
                update = true;

            index = 6;
            // Check the system ExperimentSelector.xml file for changes
            filepath = FileUtil.openPath("INTERFACE/ExperimentSelector.xml");

            if (isFileChanged(index, filepath))
                update = true;

            if (!update) {
                return;
            }
            
            update();
        }

    }

    // Listener for Search panel including collapse and expand
    public class SearchListener implements ActionListener {

        public void actionPerformed(ActionEvent ae) {
            Enumeration <TreeNode>enumer;
            String cmd = ae.getActionCommand();
            String searchString = searchText.getText();
            TreePath treePath;
            selNode=null;
            ArrayList <DefaultMutableTreeNode> nodeList = new ArrayList <DefaultMutableTreeNode>();
            String label;
            boolean skip=false;
            int loop=1;
            
            if(cmd.equals("collapse")) {
                collapseTree();
                return;
            }
            if(cmd.equals("expand")) {
                expandTree();
                return;
            }

            // Don't do case sensitive searches
            searchString = searchString.toLowerCase();
            
            treePath = tree.getSelectionPath();
            if(treePath != null) {
                selNode = (DefaultMutableTreeNode) treePath.getLastPathComponent();
                // Skip nodes until selNode is found and start there
                skip = true;
            }
            
            // We need to be able to go forward and backwards though the nodes
            // for Next and Prev.  I only see a way to go forward using the
            // Enumeration.  So, I will go through and create an ArrayList of
            // the nodes so I can then go in either direction.
            enumer = rootNode.preorderEnumeration();            
            while(enumer.hasMoreElements()) {
                DefaultMutableTreeNode node = (DefaultMutableTreeNode) enumer.nextElement();
                nodeList.add(node); 
            }
            
            // For Next, we start at 0 and increment by +1
            // For Prev, we start at nodeList.size()-1 and increment by -1
            int inc = 1;
            int start = 0;
            int size = nodeList.size();
            if(cmd.equals("prev")) {
                inc = -1;
                start = nodeList.size() -1;
            }

            

            // If starting at some location other that the top, then loop
            // twice to catch things back at the top
            if(treePath != null)
                loop = 2;
            for(int i=0; i < loop; i++) {
                for(int j=0, k=start; j < size; k+= inc, j++) {
                    DefaultMutableTreeNode node = nodeList.get(k);

                    // If there was a selection and we have hit that selected node,
                    // then un-set the skip flag
                    if(skip && selNode == node) {
                        skip = false;
                        // We don't want to find the one we are already on
                        continue;
                    }
                    // If we are skipping until be get to the selected node and
                    // we have not reached it yet, then skip this node.
                    if(skip)
                        continue;

                    // Only search for protocols
                    if(node.getAllowsChildren())
                        continue;

                    label = (String)node.getUserObject();
                    label = label.toLowerCase();


                    if(label.indexOf(searchString) > -1) {
                        // Collapse the tree first, or we end up with too much open
                        collapseTree();
                        
                        // Found one.  Set it selected then bail out
                        // Get the TreeNode path
                        TreeNode nodes[] = node.getPath();
                        // Convert to TreePath
                        TreePath newPath = new TreePath(nodes);
                        // Set the new selection
                        tree.setSelectionPath(newPath);
 
                        // Make this selected row visible in the scrollpane
                        if ((tree.getParent() instanceof JViewport)) {
                            JViewport viewport = (JViewport)tree.getParent();
                            int selrow = tree.getMinSelectionRow();
                            Rectangle rect = tree.getRowBounds(selrow);
                            // Set the view to the top, else repeated calls
                            // to scrollRectToVisible don't work
                            viewport.setViewPosition(new Point (0,0));
                            viewport.scrollRectToVisible(rect);
                        }
                        // We are Done, bail out
                        return;
                    }
                }
            }
        }
    }
    
    // Collapse all nodes of the Tree
    public void collapseTree() {
        int row = tree.getRowCount() - 1;
        while (row >= 0) {
            tree.collapseRow(row);
            row--;
        }

    }
    
    // Find a node with this label and this parent's label
    public DefaultMutableTreeNode findNode(String label, String parent) {
        Enumeration <TreeNode>enumer;
 
        enumer = rootNode.preorderEnumeration();            
        while(enumer.hasMoreElements()) {
            DefaultMutableTreeNode node = (DefaultMutableTreeNode) enumer.nextElement();
            String thisLabel = (String)node.getUserObject();
            if(label.equals(thisLabel)) {
                DefaultMutableTreeNode parentNode = (DefaultMutableTreeNode) node.getParent();
                String thisParent = (String)parentNode.getUserObject();
                if(parent.equals(thisParent)) {
                    // A match was found, return it.
                    return node;
                }
            }
        }
        
        // A match was not found
        return null;

        
    }
    
    // Expand all nodes of the Tree
    public void expandTree() {
        // The while loop does one level.  Loop through 4 times to get all levels
        for(int i=0; i < 4; i++) {
            int row = tree.getRowCount() - 1;
            while (row >= 0) {
                tree.expandRow(row);
                row--;
            }
        }
    }
    
    // Fill the tree from tabList
    public synchronized void fillTree (HashArrayList esTreeList) {
        
        if(esTreeList == null || esTreeList.size() == 0) {
            Messages.postError("TabList is Empty");
            return;
        }
        
        // Clear any previous nodes
        rootNode.removeAllChildren();
        
        // Loop through the esTreeList and create nodes
        for(int i=0; i < esTreeList.size(); i++) {
            ExpSelEntry level1 = (ExpSelEntry) esTreeList.get(i);
            String label1 = level1.getLabel();
            DefaultMutableTreeNode node1;
            node1 = new DefaultMutableTreeNode(label1);
                        
            // If no sub levels, then this is a leaf (no children)
            if(level1.entryListCount() == 0)
                node1.setAllowsChildren(false);
            rootNode.add(node1);
            // This is the old Tab level
            if (DebugOutput.isSetFor("expselector"))
                Messages.postDebug(label1);
            
            // Find sub nodes within this node
            HashArrayList list2 = level1.getEntryList();
            for(int k=0; list2 != null && k < list2.size(); k++) {
                ExpSelEntry level2 = (ExpSelEntry) list2.get(k);
                String label2 = level2.getLabel();
                // This is the old menu1 level
                if (DebugOutput.isSetFor("expselector")) 
                    Messages.postDebug("---" + label2);
                DefaultMutableTreeNode node2;
                node2 = new DefaultMutableTreeNode(label2);
                
                // If no sub levels, then this is a leaf (no children)
                if(level2.entryListCount() == 0)
                    node2.setAllowsChildren(false);

                node1.add(node2);
                
                
                HashArrayList list3 = level2.getEntryList();
                for(int j=0; list3 != null && j < list3.size(); j++) {
                    ExpSelEntry level3 = (ExpSelEntry) list3.get(j);
                    String label3 = level3.getLabel();
                    // This is the old menu2 level and protocols within the menu1 level
                    if (DebugOutput.isSetFor("expselector")) 
                        Messages.postDebug("------" + label3);
                    DefaultMutableTreeNode node3;
                    node3 = new DefaultMutableTreeNode(label3);
                    
                    // If no sub levels, then this is a leaf (no children)
                    if(level3.entryListCount() == 0)
                        node3.setAllowsChildren(false);

                    node2.add(node3);

                    HashArrayList list4 = level3.getEntryList();
                    for(int l=0; list4 != null && l < list4.size(); l++) {
                        ExpSelEntry level4 = (ExpSelEntry) list4.get(l);
                        String label4 = level4.getLabel();
                        // This would be akin to a menu3 level which we don't have yet
                        // And protocols within the menu2 level
                        if (DebugOutput.isSetFor("expselector")) 
                            Messages.postDebug("---------" + label4);
                        DefaultMutableTreeNode node4;
                        node4 = new DefaultMutableTreeNode(label4);
                        
                        // If no sub levels, then this is a leaf (no children)
                        if(level4.entryListCount() == 0)
                            node4.setAllowsChildren(false);

                        node3.add(node4);

                        HashArrayList list5 = level4.getEntryList();
                        for(int m=0; list5 != null && m < list5.size(); m++) {
                            ExpSelEntry level5 = (ExpSelEntry) list5.get(l);
                            String label5 = level5.getLabel();
                            // This would be protocols within the menu3 level
                            if (DebugOutput.isSetFor("expselector")) 
                                Messages.postDebug("------------" + label5);
                            DefaultMutableTreeNode node5;
                            node5 = new DefaultMutableTreeNode(label5);
                            
                            // If no sub levels, then this is a leaf (no children)
                            if(level5.entryListCount() == 0)
                                node5.setAllowsChildren(false);

                            node4.add(node5);
                        }  
                    }
                }
            }
        }
        // This is all required to get the new nodes to show up
        jsp.setViewportView(tree);
        treeModel = new DefaultTreeModel(rootNode);
        tree.setModel(treeModel);
        
        // Expand the first row of the Tree
//        tree.expandRow(0);

        
    }
    

    public static void fillProtocolList() {
        // default to not being called for admin
        fillProtocolList(false);
    }

    public static void fillProtocolList(boolean admin) {
        
        String curOperator = Util.getCurrOperatorName();
        String curuser = System.getProperty("user.name");
        LoginService loginService = LoginService.getDefault();
        Hashtable userHash = loginService.getuserHash();
        User user = (User) userHash.get(curuser);
        ArrayList<String> appDirs;
        ArrayList<String> labels;
        
        // Get the SAX parser to call. We pass this our local class that
        // will know what to do with the parsed information
        SAXParser parser = getSAXParser();

        // Set up for parsing the ExperimentSelector_operator.xml file
        // Our local class to deal with parsed info from
        // ExperimentSelector.xml type files
        DefaultHandler expSelSAXHandler = new ExpSelTree.ExpSelSaxHandler(false, 
                                                                  protocolList);
        
        // Parse the ExpSelOrder_op.xml file if it exists
        String filepath = FileUtil.openPath("USER/PERSISTENCE/ExpSelOrder_"
                + curOperator + ".xml");

        Boolean foundESOrder = false;

        if (filepath != null) {
            if((DebugOutput.isSetFor("expselector")))
               Messages.postDebug("ES parsing " + filepath);
            File file = new UNFile(filepath);
            // Parse this operators ExpSelOrder_operator.xml file
            // This will add ExpSelEntry items to protocolList
            if (file.length() > 20) {
              try {
                parser.parse(file, expSelSAXHandler);
                foundESOrder = true;
              }
              catch (Exception e) {
                Messages.writeStackTrace(e);
              }
            }
        }
        else {
            // The operator must not have a file.  Try the User's file
            String userName = user.getAccountName();
            filepath = FileUtil.openPath("USER/PERSISTENCE/ExpSelOrder_"
                                         + userName + ".xml");
            if (filepath != null) {
                if((DebugOutput.isSetFor("expselector")))
                   Messages.postDebug("ES parsing " + filepath);
                File file = new UNFile(filepath);
                // Parse this user's ExperimentSelector_user.xml file
                // This will add ExpSelEntry items to protocolList
                if (file.length() > 20) {
                  try {
                    parser.parse(file, expSelSAXHandler);
                    foundESOrder = true;
                  }
                  catch (Exception e) {
                    Messages.writeStackTrace(e);
                  }
                }
            } 
        }

        // Look for a ES_operator.xml file
        String es_op_filepath = ExpSelector.getOperatorFileName();
        if (es_op_filepath == null)
            es_op_filepath = FileUtil.openPath("USER/INTERFACE/ExperimentSelector_"
                + curOperator + ".xml");

        if (es_op_filepath != null) {
            if((DebugOutput.isSetFor("expselector")))
               Messages.postDebug("ES parsing " + es_op_filepath);
            File file = new UNFile(es_op_filepath);
            // Parse this operators ExperimentSelector_operator.xml file
            // This will add ExpSelEntry items to protocolList
            if (file.length() > 20) {
              try {
                parser.parse(file, expSelSAXHandler);
              }
              catch (Exception e) {
                Messages.writeStackTrace(e);
              }
            }
        }
        else {
            // The operator must not have a file.  Try the User's file
            String userName = user.getAccountName();
            es_op_filepath = FileUtil.openPath("USER/INTERFACE/ExperimentSelector_"
                                         + userName + ".xml");
            if (es_op_filepath != null) {
                if((DebugOutput.isSetFor("expselector")))
                   Messages.postDebug("ES parsing " + es_op_filepath);
                File file = new UNFile(es_op_filepath);
                // Parse this user's ExperimentSelector_user.xml file
                // This will add ExpSelEntry items to protocolList
                if (file.length() > 20) {
                  try {
                    parser.parse(file, expSelSAXHandler);
                  }
                  catch (Exception e) {
                    Messages.writeStackTrace(e);
                  }
                }
            } 
        }

        // I don't think we want to use this any longer with the new Tree

        // // Parse the tabOrder.xml file if it exists.  This info used to be in
        // // the ES_op.xml file, but because the macro updateExpSelector, removed
        // // that file and recreates it, this info was being lost.
        // String tabOrderPath = FileUtil.openPath("USER/INTERFACE/tabOrder_"
        //                                         + curOperator + ".xml");
        // if (tabOrderPath != null) {
        //     File file = new UNFile(tabOrderPath);
        //     // Parse this operators tabOrder_operator.xml file
        //     // to fill tabOrderList
        //     try {
        //         parser.parse(file, expSelSAXHandler);
        //     }
        //     catch (Exception e) {
        //         Messages.writeStackTrace(e);
        //     }
        // }


        
        // If admin, it means use all possible appdirs, not just the ones
        // this user has set.

        if(admin) {
            // Get all possible appdirs
            appDirs = Util.getAllPossibleAppDirs();
            labels = Util.getAllPossibleAppDirLabels();
        }
        else {
            // Get appdirs for this user
            appDirs = FileUtil.getAppDirs();
            labels = FileUtil.getAppDirLabels();
        }

        
        
        // Now parse the  ExperimentSelector.xml file.
        // If we have already parsed an ESOrder file, then we want
        // to use the ES.xml file to be sure we have the right protocols.
        if (!userOnly && !admin && !foundESOrder) {
            // No ExpSelOrder_ file found, just use the ES.xml file
            filepath = FileUtil
                    .openPath("INTERFACE/ExperimentSelector.xml");
            if (filepath != null) {
                if((DebugOutput.isSetFor("expselector")))
                   Messages.postDebug("ES parsing " + filepath);
                File file = new UNFile(filepath);
                // This will add ExpSelEntry items to protocolList
                if (file.length() > 20) {
                  try {
                    parser.parse(file, expSelSAXHandler);
                  }
                  catch (Exception e) {
                    Messages.writeStackTrace(e);
                  }
                }
            }
            else {
                // There should be an ExperimentSelector.xml in some appdir
                Messages.postWarning("No ExperimentSelector.xml file found");
            }
        }
        else if(foundESOrder) {
            // An ExpSelOrder_ file was found.  Thus we want to use the ES.xml
            // file for only two things:
            // - If a protocol is not in the ES.xml file, remove it from the list
            // - If ES.xml has a protocol which is not already in the list,
            //   add it to the end.

            // First, parse the ES.xml file into a separate list (pList) so we
            // can compare it with protocolList.
            ArrayList<ExpSelEntry> prList = new ArrayList<ExpSelEntry>();

            expSelSAXHandler = new ExpSelTree.ExpSelSaxHandler(false, prList);
            filepath = FileUtil.openPath("INTERFACE/ExperimentSelector.xml");
            if (filepath != null) {
                if((DebugOutput.isSetFor("expselector")))
                    Messages.postDebug("ES parsing " + filepath);
                File file = new UNFile(filepath);
                // This will add ExpSelEntry items to pLrist
                if (file.length() > 20) {
                  try {
                    parser.parse(file, expSelSAXHandler);
                  }
                  catch (Exception e) {
                    Messages.writeStackTrace(e);
                  }
                }
            }
            // Add to the list the protocols in ES_op as protocols to keep
            if(es_op_filepath != null) {
                if((DebugOutput.isSetFor("expselector")))
                    Messages.postDebug("ES parsing " + es_op_filepath);
                File file = new UNFile(es_op_filepath);
                // This will add ExpSelEntry items to pLrist
                if (file.length() > 20) {
                  try {
                    parser.parse(file, expSelSAXHandler);
                  }
                  catch (Exception e) {
                    Messages.writeStackTrace(e);
                  }
                }
            }
            
// TODO  Test this code
            // Now go through the list and check each protocol to be sure
            // it is in the ES.xml.  If not, remove it from the list
            for (int i = 0; i < protocolList.size(); i++) {
                boolean matchfound = false;
                ExpSelEntry protocol = protocolList.get(i);
                for (int k = 0; k < prList.size(); k++) {
                    ExpSelEntry prot = prList.get(k);
                    if(protocol.name.equals(prot.name)) {
                        matchfound = true;
                        break;
                    }    
                }
                if(!matchfound) {
                    // The protocol in ESOrder was not found in the ES.xml
                    // file so remove it from the list
                    protocolList.remove(i);
                    if (DebugOutput.isSetFor("expselector")) {
                        Messages.postDebug("Removing " + protocol.name 
                                + " from ExpSel");
                    }

                    // fix counter since we just removed item "i" from the list
                    i--;
                }
            }
            // Now, if ES.xml has a protocol not in the EXOrder, then add it
            // to the end.
            for (int i = 0; i < prList.size(); i++) {
                boolean matchfound = false;
                ExpSelEntry protocol = prList.get(i);
                for (int k = 0; k < protocolList.size(); k++) {
                    ExpSelEntry prot = protocolList.get(k);
                    if(protocol.name.equals(prot.name)) {
                        matchfound = true;
                        break;
                    }    
                }
                if(!matchfound) {
                    // This protocol is in ES.xml, but not ESOrder.  First off,
                    // that means that the ES.xml has changed since the ESOrder
                    // was created.  Nonetheless, we want to go ahead and add
                    // this protocol to the list.
                    protocolList.add(protocol);
                }
            }
            
        }
        if (admin) {
            // Look for shared directory ExperimentSelector.xml files
            // parse them, but don't dup and let any previous entry hold.
            // Loop through the appdirs and skip user directories
            String relativePath = FileUtil.getSymbolicPath("INTERFACE");
            // The arg, true, means the parser will ignore the 'tab' entry
            // in the file and instead use the directory name/label.
            expSelSAXHandler = new ExpSelSaxHandler(true, protocolList);

            for (int k = 0; k < appDirs.size(); k++) {
                String adir = appDirs.get(k);
                
                // Skip user dirs
                if (!adir.startsWith(FileUtil.usrdir())) {
                    // We must have what we call a shared dir or a system dir, create the
                    // fullpath for the ExperimentSelector.xml file
                   
                    // Get a list of all ExperimentSelector_*.xml files
                    String dir = adir + File.separator + relativePath;
                    File dirFile = new UNFile(dir);
                    ESFileFilter filter = new ESFileFilter();
                    File files[] = dirFile.listFiles(filter);

                    if(files != null) {
                        // Loop through the file list and parse each of them
                        for(int i=0; i < files.length; i++) {

                            filepath = FileUtil.openPath(files[i].getAbsolutePath());
                            if (DebugOutput.isSetFor("expselector")) {
                                Messages.postDebug("ExpSelector: Parsing " + filepath);
                            }

                            if (filepath != null) {
                                File file = new UNFile(filepath);
                                // This will add ExpSelEntry items to protocolList
                                try {
                                    parser.parse(file, expSelSAXHandler);
                                }
                                catch (Exception e) {
                                    Messages.writeStackTrace(e);
                                }
                            }
                        }
                    }
                }
            }
        }     
        
        // Now we have finished with all of the possible
        // ExperimentSelector.xml
        // files. For backwards compatibility and just to be sure protocols
        // are not forgotten, we will go through all non system protocols
        // and see we have them in protocolList. If not, we need to add them
        String relativePath = FileUtil.getSymbolicPath("PROTOCOLS");
        // Go through appdirs, skipping user directories
        
// Stop it looking for protocols for now
// updateExpSelector macro should be picking up the users protocols
//        for (int k = 0; k < appDirs.size(); k++) {
//            String dir = appDirs.get(k);
// 
//            // Skip system dirs and shared dirs if userOnly is true
//            // skip if showNonUserNonSpecifiedAppdirProtoInCatchAll = false
//
//            boolean skip=false;
//
//            // Skip if this is the sysdir or an appdir within the sysdir
//            // That is, possibly allow non sysdir appdirs and userdir
//            if(dir.startsWith(FileUtil.sysdir()))
//                continue;
//
//// This logic is now done in the macro updateExpSelector
////            // If neither NonUser nor User is specified, skip
////            if(!showNonUserNonSpecifiedAppdirProtoInCatchAll && 
////               !showUserNonSpecifiedAppdirProtoInCatchAll) {
////                skip=true;
////            }
////            // If show User but not his dir, skip
////            if(!showNonUserNonSpecifiedAppdirProtoInCatchAll &&
////               (showUserNonSpecifiedAppdirProtoInCatchAll && 
////                !dir.startsWith(FileUtil.usrdir()))) {
////                skip=true;
////            }
////            // If show NonUser but don't show User and it is userdir, skip
////            if(showNonUserNonSpecifiedAppdirProtoInCatchAll &&
////               (!showUserNonSpecifiedAppdirProtoInCatchAll &&
////                dir.startsWith(FileUtil.usrdir()))) {
////                skip=true;
////            }
//            // If userOnly, then only allow userdir
//            if(userOnly && !dir.startsWith(FileUtil.usrdir())) {
//                skip=true;
//            }
//
//            if (!skip) {
//                String pPath = dir + File.separator + relativePath;
//                UNFile file = new UNFile(pPath);
//                if (file != null && file.isDirectory()) {
//
//                    // Get the list of protocols in this directory
//                    File[] files = file.listFiles();
//                    for (int i = 0; i < files.length; i++) {
//                        String name = files[i].getName();
//                        // We need to strip off the .xml from the end of
//                        // the actual filename to get the name we use
//                        // for the protocol
//                        if (name.endsWith(".xml") && files[i].isFile())
//                            name = name.substring(0, name.length() - 4);
//                        else
//                            // If not .xml or not normal file, skip it,
//                            // bogus file
//                            continue;
//
//                        // This call to isNotDup() only looks at the
//                        // protocol name since that is all we have.
//                        if (isNotDup(name, pList) == NOTDUP) {
//                            // Not a dup, we need to add it unless it has
//                            // ExpSelNoShow=true.  Open the .xml and see if
//                            // is has this set.
//                            RandomAccessFile input=null;
//                            String line;
//                            boolean noshow = false;
//                            String paramName = "ExpSelNoShow";
//                            int paramLen = paramName.length();
//                            try {
//                                input = new RandomAccessFile(files[i], "r");
//                                while ((line = input.readLine()) != null) {
//                                    int index = line.indexOf(paramName);
//                                    if(index > -1) {
//                                        String val = line.substring(index + paramLen, index + paramLen + 7);
//                                        if(val.indexOf("true") > -1) {
//                                            noshow = true;
//                                            input.close();
//                                            break;
//                                        }
//                                    }
//                                }
//
//                            }
//                            catch(Exception ex) {
//                                try {
//                                    noshow = true;
//                                    input.close();
//                                }
//                                catch (Exception exc) {
//                                
//                                }
//                            }
//
//                            if(noshow == false) {
//                                ExpSelEntry eSEntry = new ExpSelEntry();
//                                eSEntry.name = new String(name);
//                                // We don't have a label specification, so use
//                                // the protocol name
//                                eSEntry.label = new String(name);
//                                // Get the appdir label for this directory
//                                String shareLabel = labels.get(k);
//
//                                eSEntry.tab = shareLabel;
//                                // Add the new entry
//                                pList.add(eSEntry);
//                            }
//                        }
//                    }
//                }
//            }
//        }

    }

    // If called with no arg, simply call the other one with null for args
    // and return whatever it returns
    private static boolean checkProtocolList(ArrayList<ExpSelEntry> pList) {
        return checkProtocolList(null, null, null, null, pList);
    }
    
    // check the protocolList to be sure there are no protocol labels that
    // are the same as a menu name within a given tab.  That is, you can
    // have duplicate protocol/menu names in different tabs.  
    // protocolList must have been filled before this is called.
    private static boolean checkProtocolList(String label, String tab, String menu1, 
                                             String menu2, ArrayList<ExpSelEntry> pList) {
        ArrayList<String> menuNameList = new ArrayList<String>();
        ArrayList<String> tabLList  = new ArrayList<String>();
        
        // Loop through the protocolList and check all entries for strings
        // containing "\\n".  For the old ES, this will cause a newline.
        // For the Tree, just replace this with a space.
        // The String.replaceAll()
        // method will not work on the "\\n" string even 
        // though indexOf() does work.  Allow more that one
        // newline giving any multiple of lines.
        for (int i = 0; i < pList.size(); i++) {
            ExpSelEntry protocol = pList.get(i);
            int index;
            String lab = protocol.getLabel();
            while((index = lab.indexOf("\\\\n"))>= 0) {
                String begStr = lab.substring(0, index);
                String endStr = lab.substring(index +3);
                lab = begStr + " \n" + endStr;
                protocol.setLabel(lab);
            }
        }


        // Loop through the protocolList and create a list of all of the
        // Tab names.
        for (int i = 0; i < pList.size(); i++) {
            ExpSelEntry protocol = pList.get(i);
            if(!tabLList.contains(protocol.tab))
                tabLList.add(protocol.tab);
        }
        // If one arrived in the args, add it
        if(tab != null)
            tabLList.add(tab);

        // Loop through the tab name list
        for(int k=0; k < tabLList.size(); k++) {
            String tabName = tabLList.get(k);
            // Clear the menuNameList list and fill it for this tab
            menuNameList.clear();
            
            // Loop through the protocolList and create a list of all of the
            // menu1 and menu2 names within this tab name.
            for (int i = 0; i < pList.size(); i++) {
                ExpSelEntry protocol = pList.get(i);
                if(protocol.tab.equals(tabName)) {
                    if(!menuNameList.contains(protocol.menu1))
                        menuNameList.add(protocol.menu1);
                    if(!menuNameList.contains(protocol.menu2))
                        menuNameList.add(protocol.menu2);
                }
            }
            // If menus arrived in the args for this tab, add them
            if(tabName.equals(tab)) {
                if(menu1 != null &&  !menuNameList.contains(menu1))
                    menuNameList.add(menu1);
                if(menu2 != null &&  !menuNameList.contains(menu2))
                    menuNameList.add(menu2);
            }

            // Now we have a list of all menu names in this tab.  Loop through the
            // protocolList again and see if any protocol names are duplicates
            // of any menu names within this tab.
            for (int i = 0; i < pList.size(); i++) {
                ExpSelEntry protocol = pList.get(i);
                if(protocol.tab.equals(tabName)) {
                    if(menuNameList.contains(protocol.label)) {                            
                        // If the dup is with an input arg, don't remove
                        // an  entry from protocolList, else remove it.
                        if(!protocol.label.equals(menu1) && !protocol.label.equals(menu2)) {
                            pList.remove(i--);
                            Messages.postError("Found protocol name the same as a menu "
                                    + "name (" + protocol.label  + ")");
                        }
                        else {
                            Messages.postError("Found protocol name the same as a menu "
                                    + "name (" + protocol.label + ")");
                            return false;
                        }
                        
                        continue;
                    }
                    // Check the arg label
                    if(label != null && menuNameList.contains(label)) {
                        Messages.postError("Found menu name the same as a protocol "
                                               + "name (" + label + ")");
                        return false;

                    }
                }
            }
        }
        
        // No duplicates found
        return true;
    }
 
    
    
    // Call when we only have a protocol name to go on.
    // If there is any entry by this name, then consider it a dup
    public static int isNotDup(String protocolName, ArrayList<ExpSelEntry> protlist) {
        ExpSelEntry eSEntry;
        for (int i = 0; i < protlist.size(); i++) {
            eSEntry = protlist.get(i);
            // Check name
            if (eSEntry.name.equals(protocolName)) {
                // It is a dup
                return COMPLETEDUP;
            }
        }
        return NOTDUP;
    }
    // We want to allow duplicate protocol entries provided they
    // are not in the exact location in the panel/tab/menu system.
    // Check to see if this is really an exact duplicate.
    // It is permissible to have the same protocol in the same
    // location but with two different labels.
    public static int isNotDup(ExpSelEntry newESE, ArrayList<ExpSelEntry> protlist) {
        ExpSelEntry eSEntry;
        for (int i = 0; i < protlist.size(); i++) {
            eSEntry = protlist.get(i);
            // Check label, tab, menu1 and menu2.  We do not want
            // dup location and diff protocol, so don't check protocol
            if (eSEntry.tab.equals(newESE.tab)
                    && eSEntry.label.equals(newESE.label)
                    && ((eSEntry.menu1 != null) && eSEntry.menu1
                            .equals(newESE.menu1))
                    && ((eSEntry.menu2 != null) && eSEntry.menu2
                            .equals(newESE.menu2))) {
                
                // It is a dup, excluding the protocol name, check that
                if(eSEntry.name.equals(newESE.name))
                    return COMPLETEDUP;
                else
                    return DUPALLBUTPROTO;
            }
        }
        // It must not be a dup
        return NOTDUP;
    }
    
    
    public static int isNotDup(String name, String tab, String label, 
                                String menu1, String menu2) {
        ExpSelEntry eSEntry;
        
// TODO does this really have to be filled every time??
        ExpSelTree.fillProtocolList();
        for (int i = 0; i < protocolList.size(); i++) {
            eSEntry = protocolList.get(i);
            // Check label, tab, menu1 and menu2.  We do not want
            // dup location and diff protocol, so don't check protocol
            if (eSEntry.tab.equals(tab)
                    && eSEntry.label.equals(label)
                    && ((eSEntry.menu1 != null) && eSEntry.menu1
                            .equals(menu1))
                    && ((eSEntry.menu2 != null) && eSEntry.menu2
                            .equals(menu2))) {
                // It is a dup, excluding the protocol name, check that
                if(eSEntry.name.equals(name)) {
                    return COMPLETEDUP;
                }
                else {
                    return DUPALLBUTPROTO;
                }
            }
        }
        // It must not be a dup
        return NOTDUP;
    }    
    
    static public void makeTrashWhatQuery(String protocol) {
        new TrashWhatQuery(protocol);
    }

    static public class TrashWhatQuery extends ModalDialog implements ActionListener
    {
        public TrashWhatQuery(String protocol) {
            super("What to Trash");

            okButton.addActionListener(this);
            cancelButton.addActionListener(this);
            helpButton.addActionListener(this);

            // Okay, this is a little wierd, but is faster than making  
            // a new panel.  They want Cancel on the right where as
            // in the ModalDialog, it is in the middle.  So, just change
            // the Text labels and make helpButton be Cancel and 
            // cancelButton be Protocol
            okButton.setText("Remove Protocol");
            cancelButton.setText("Cancel");


            JTextArea msg = new JTextArea("Remove the actual protocol from the Disk?");
            
            getContentPane().add(msg, BorderLayout.NORTH);
            pack();
            
            // Set the popup position just above the trash can
            TrashCan trash = Util.getTrashCan();
            Point p = trash.getLocationOnScreen();
            p.translate(0, -100);
            setLocation(p);

            setVisible(true, true);

        }


        public void actionPerformed(ActionEvent e) {

            String cmd = e.getActionCommand();

            if (cmd.equalsIgnoreCase("Remove Protocol")) {
                trashWhat = "protocol";
            }
            else if (cmd.equalsIgnoreCase("Cancel"))
            {
                trashWhat = "cancel";
            }
            else if (cmd.equalsIgnoreCase("Entry Only"))
            {
                trashWhat = "entry";
            }
            else if (cmd.equals("help"))
                displayHelp();
            setVisible(false);
        }
    }

    static public ArrayList<String> getTabNameList() {
        return tabNameList;
    }


    static public boolean isAdmin() {
        boolean admin;
        AppIF appIf = Util.getAppIF();
        if (appIf instanceof VAdminIF)
            admin = true;
        else
            admin = false;
        return admin;
    }
    
    static public SAXParser getSAXParser() {
        SAXParser parser = null;
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            spf.setValidating(false); // set to true if we get DOCTYPE
            spf.setNamespaceAware(false); // set to true with referencing
            parser = spf.newSAXParser();
        }
        catch (ParserConfigurationException pce) {
            System.out.println(new StringBuffer().append(
                    "The underlying parser does not support ").append(
                    "the requested feature(s).").toString());
        }
        catch (FactoryConfigurationError fce) {
            System.out.println("Error occurred obtaining SAX Parser Factory.");
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
        return (parser);
    }

    // Deal with parsed output from the ExperimentSelector.xml files
    // Fill a 'eSEntry' for each protocol and put the eSEntry into protocolList.
    // Also fill userOnly, sharedFlat and userFlat mode variables
    static public class ExpSelSaxHandler extends DefaultHandler {

        ExpSelEntry eSEntry;
        ArrayList<ExpSelEntry> protList;

        // If sharedType = true, then use the directory name/label in place
        // of the tab specified
        boolean sharedType = false;

        public ExpSelSaxHandler(boolean sharedType, ArrayList<ExpSelEntry> protlist) {
            this.sharedType = sharedType;
            // List where to put the results of the parsing
            protList = protlist;
        }

        public ArrayList<ExpSelEntry> getProtList() {
            return protList;
        }
        public void endDocument() {
            // System.out.println("End of Document");
        }

        public void endElement(String uri, String localName, String qName) {
            // System.out.println("End of Element '"+qName+"'");
        }

        public void startDocument() {
            // System.out.println("Start of Document");
        }

        public void startElement(String uri, String localName, String qName,
                Attributes attr) {
            if (qName.compareTo("ProtocolList") == 0) {
                // These were defaulted to false, so if they are not set
                // then they will remain defaulted. The system file should
                // not have these, so they will be whatever was left from
                // the default or the users file
                String mode = attr.getValue("userOnly");
                if (mode != null && mode.equals("true"))
                    userOnly = true;
                else if (mode != null && mode.equals("false"))
                    userOnly = false;
                mode = attr.getValue("sharedFlat");
                if (mode != null && mode.equals("false"))
                    sharedFlat = false;
                else if (mode != null && mode.equals("true"))
                    sharedFlat = true;
                mode = attr.getValue("userFlat");
                if (mode != null && mode.equals("false"))
                    userFlat = false;
                else if (mode != null && mode.equals("true"))
                    userFlat = true;
                mode = attr.getValue("allWithSystem");
                if (mode != null && mode.equals("false"))
                    allWithSystem = false;
                else if (mode != null && mode.equals("true"))
                    allWithSystem = true;
                mode = attr.getValue("tooltip");
                if (mode != null && (mode.equals("false") || mode.equals("none")))
                    tooltipMode = "false";
                else if (mode != null && mode.equals("label"))
                     tooltipMode = "label";
                else if (mode != null && mode.equals("fullpath"))
                     tooltipMode = "fullpath";
                mode = attr.getValue("labelWithCategory");
                if (mode != null && mode.equals("false"))
                    labelWithCategory = false;
                else if (mode != null && mode.equals("true"))
                    labelWithCategory = true;
                mode = attr.getValue("iconForCategory");
                if (mode != null && mode.equals("false")) {
                    iconForCategory = false;
                    setPosition = "center";
                }
                else if (mode != null && mode.equals("true")) {
                    iconForCategory = true;
                    setPosition = "left";
                }

            }
            // Add a line in the ES_op.xml file to define the order of the
            // tabs in the panel.  The tabOrder name is expected to be a
            // comma separated list like
            // <tabOrder list="New Tab, Sel2D, Dosy 2D, (HH)Homo2D " />
            else if (qName.compareTo("tabOrder") == 0) {
                String list = attr.getValue("list");
                StringTokenizer tok;
                tok = new StringTokenizer(list, ",");
                int count = tok.countTokens();
                for(int i=0; i < count; i++) {
                    String tabname = tok.nextToken().trim();
                    ExpSelEntry tab;
                    if (!tabList.containsKey(tabname)) {
                        // This tab does not exist yet, make it
                        tab = new ExpSelEntry();
                        tab.label = new String(tabname);
                        tab.entryList = new HashArrayList();
                        // Add to tabList
                        tabList.put(tabname, tab);
                                
                        // Save a list of tab labels for the panel menu
                        if(!tabOrderList.contains(tab.label))
                            tabOrderList.add(tab.label);
                    }
                }
            }
            else if (qName.compareTo("protocol") == 0) {
                eSEntry = new ExpSelEntry();
                eSEntry.name = Util.getLabel(attr.getValue("name"));
                eSEntry.label = Util.getLabel(attr.getValue("label"));
                // If no label it given, use the protocol name
                if(eSEntry.label.length() == 0)
                    eSEntry.label = new String(eSEntry.name);

                eSEntry.fgColorTx = attr.getValue("fgcolor");
                eSEntry.bgColorTx = attr.getValue("bgcolor");
                eSEntry.noShow = attr.getValue("ExpSelNoShow");
                if(eSEntry.noShow == null)
                    eSEntry.noShow = "false";

                // Should the actual protocol file be chosen by keywords
                // (parameters)?
                eSEntry.bykeywords = attr.getValue("bykeywords");
                
                // Try all appdirs until this protocol is found, then
                // continue.  It used to just check the appdir path where
                // the ExperimentSelector.xml file was located, but that
                // ended up missing some protocols (Like ATP)
                ArrayList<String> appDirs;
                if(isAdmin()) {
                    // Get all possible appdirs
                    appDirs = Util.getAllPossibleAppDirs();
                }
                else {
                    // Get appdirs for this user
                    appDirs = FileUtil.getAppDirs();
                }
                String ppath;
                String fullpath=null;
                for (int k = 0; k < appDirs.size(); k++) {
                    String dir = appDirs.get(k);
                    String relativePath = FileUtil.getSymbolicPath("PROTOCOLS");
                    ppath = dir + File.separator + relativePath + File.separator 
                    + eSEntry.name + ".xml";
                    fullpath = FileUtil.openPath(ppath);
                    // If file is found, continue below with this path
                    if (fullpath != null)
                        break;
                    // If null and we are out of appDirs to look in,
                    // abort the addition of this protocol
                    else if(k == appDirs.size() -1)
                        return;
                }
                if(fullpath == null)
                    return;
                
                // Assume that system files start with FileUtil.sysdir()
                // And User files start with FileUtil.usrdir()
                // 'Shared' Dir
                if (!fullpath.startsWith(FileUtil.sysdir())
                        && !fullpath.startsWith(FileUtil.usrdir())) {
                    if(allWithSystem) {
                        // Put Shared protocols with system, but add
                        // '(Sh)' to the label if requested
                        if(labelWithCategory)
                            eSEntry.label = eSEntry.label + "(Sh)";
                            
                        eSEntry.tab = attr.getValue("tab");
                        eSEntry.menu1 = attr.getValue("menu1");
                        eSEntry.menu2 = attr.getValue("menu2");
                        // Allow level1 and 2 to be the same as menu1,2
                        // and level0 same as tab
                        String level0 = attr.getValue("level0");
                        String level1 = attr.getValue("level1");
                        String level2 = attr.getValue("level2");
                        if(level0 != null)
                            eSEntry.tab = level0;
                        if(level1 != null)
                            eSEntry.menu1 = level1;
                        if(level2 != null)
                            eSEntry.menu2 = level2;
                        
                        if(eSEntry.tab == null)
                            eSEntry.tab = "Shared";

                       
                    }

                    // Not system and not user so must be 'shared'. They don't
                    // want shared mixed with system, so put into a tab called
                    // the directory's name/label
                    else if (sharedFlat) {
                        // Flat, so put all of them into a single tab
                        eSEntry.tab = "Shared";
                    }
                    else {
                        // Get the appdir label list
                        ArrayList<String> labels = FileUtil.getAppDirLabels();
                        // Get the appdir list
                        ArrayList<String> appdirs = FileUtil.getAppDirs();
                        // Determine which appdir this protocol was found in
                        // by going through the list until it matches
                        int k;
                        for (k = 0; k < appdirs.size(); k++) {
                            if (fullpath.startsWith(appdirs.get(k)))
                                break;
                        }
                        String shareLabel;
                        if (labels.size() > k)
                            shareLabel = labels.get(k);
                        else
                            shareLabel = "Shared";

                        eSEntry.tab = shareLabel;

                        // Not flat, so use the menus specified
                        eSEntry.menu1 = attr.getValue("menu1");
                        eSEntry.menu2 = attr.getValue("menu2");
                        // Allow level1 and 2 to be the same as menu1,2
                        // and level0 same as tab
                        String level1 = attr.getValue("level1");
                        String level2 = attr.getValue("level2");
                        if(level1 != null)
                            eSEntry.menu1 = level1;
                        if(level2 != null)
                            eSEntry.menu2 = level2;

                    }

                }
                // Assume that User files start with FileUtil.usrdir()
                // 'User' Dir
                else if (fullpath.startsWith(FileUtil.usrdir())) {
                    if(allWithSystem) {
                        // Put User protocols with system, but add
                        // '(U)' to the label if requested
                        if(labelWithCategory)
                            eSEntry.label = eSEntry.label + "(U)";
                        eSEntry.tab = attr.getValue("tab");
                        if(eSEntry.tab == null)
                            eSEntry.tab = "User";
                        eSEntry.menu1 = attr.getValue("menu1");
                        eSEntry.menu2 = attr.getValue("menu2");
                        // Allow level1 and 2 to be the same as menu1,2
                        // and level0 same as tab
                        String level0 = attr.getValue("level0");
                        String level1 = attr.getValue("level1");
                        String level2 = attr.getValue("level2");
                        if(level0 != null)
                            eSEntry.tab = level0;
                        if(level1 != null)
                            eSEntry.menu1 = level1;
                        if(level2 != null)
                            eSEntry.menu2 = level2;

                    }
                    else {
                        // so put into the appropriate tab, but then put into
                        // a menu called User
                        eSEntry.tab = attr.getValue("tab");
                        eSEntry.menu1 = "User";
                        // If not flat, then go ahead and take the user specified
                        // menu2
                        if (!userFlat)
                            eSEntry.menu2 = attr.getValue("menu2");
                    }
                }
                // Else, continue below to add normally
                else {
                    eSEntry.tab = attr.getValue("tab");
                    eSEntry.menu1 = attr.getValue("menu1");
                    eSEntry.menu2 = attr.getValue("menu2");
                    // Allow level1 and 2 to be the same as menu1,2
                    // and level0 same as tab
                    String level0 = attr.getValue("level0");
                    String level1 = attr.getValue("level1");
                    String level2 = attr.getValue("level2");
                    if(level0 != null)
                        eSEntry.tab = level0;
                    if(level1 != null)
                        eSEntry.menu1 = level1;
                    if(level2 != null)
                        eSEntry.menu2 = level2;
                        

                }
                // Disallow  duplicate entries where they have the same
                // label, tab, menu1 and menu2
                int status = isNotDup(eSEntry, protList);
                if (status == NOTDUP)
                    protList.add(eSEntry);
            }
        }


    }

    public void warning(SAXParseException spe) {
        System.out.println("Warning: SAX Parse Exception:");
        System.out.println(new StringBuffer().append("    in ").append(
                spe.getSystemId()).append(" line ").append(spe.getLineNumber())
                .append(" column ").append(spe.getColumnNumber()).toString());
    }

    public void error(SAXParseException spe) {
        System.out.println("Error: SAX Parse Exception:");
        System.out.println(new StringBuffer().append("    in ").append(
                spe.getSystemId()).append(" line ").append(spe.getLineNumber())
                .append(" column ").append(spe.getColumnNumber()).toString());
    }

    public void fatalError(SAXParseException spe) {
        System.out.println("FatalError: SAX Parse Exception:");
        System.out.println(new StringBuffer().append("    in ").append(
                spe.getSystemId()).append(" line ").append(spe.getLineNumber())
                .append(" column ").append(spe.getColumnNumber()).toString());
    }

    // The DefaultMutableTreeNode only allows one piece of information
    // to be kept with it and that is the label that it displays.
    // When a node is dragged, we need the fullpath.  tabList is the
    // source of these nodes and has the labels and fullpaths.  They
    // are in tree format, so we need to go through each element of
    // the tree looking for this "label" and then return the fullpath.
    // 
    // Duplicate labels should not be allowed unless they point to the
    // same fullpath.  We will find the first one.
    public String getFullpath(String label) {
        // Loop through the tabList and look for a node with this label
        // if the node is 
        for(int i=0; i < tabList.size(); i++) {
            ExpSelEntry level1 = (ExpSelEntry) tabList.get(i);
            String label1 = level1.getLabel();
            // If Leaf, check to see if label matches
            if(level1.entryListCount() == 0) {
                if(label1.equals(label)) {
                    return level1.getFullpath();
                }
            }
            // Find sub nodes within this node
            HashArrayList list2 = level1.getEntryList();
            for(int k=0; list2 != null && k < list2.size(); k++) {
                ExpSelEntry level2 = (ExpSelEntry) list2.get(k);
                String label2 = level2.getLabel();
                // If Leaf, check to see if label matches
                if(level2.entryListCount() == 0) {
                    if(label2.equals(label)) {
                        return level2.getFullpath();
                    }
                }
                HashArrayList list3 = level2.getEntryList();
                for(int j=0; list3 != null && j < list3.size(); j++) {
                    ExpSelEntry level3 = (ExpSelEntry) list3.get(j);
                    String label3 = level3.getLabel();
                    // If Leaf, check to see if label matches
                    if(level3.entryListCount() == 0) {
                        if(label3.equals(label)) {
                            return level3.getFullpath();
                        }
                    }
                    HashArrayList list4 = level3.getEntryList();
                    for(int l=0; list4 != null && l < list4.size(); l++) {
                        ExpSelEntry level4 = (ExpSelEntry) list4.get(l);
                        String label4 = level4.getLabel();
                        // If Leaf, check to see if label matches
                        if(level4.entryListCount() == 0) {
                            if(label4.equals(label)) {
                                return level4.getFullpath();
                            }
                        }                        
                        HashArrayList list5 = level4.getEntryList();
                        for(int m=0; list5 != null && m < list5.size(); m++) {
                            ExpSelEntry level5 = (ExpSelEntry) list5.get(l);
                            String label5 = level5.getLabel();
                            // If Leaf, check to see if label matches
                            if(level5.entryListCount() == 0) {
                                if(label5.equals(label)) {
                                    return level5.getFullpath();
                                }
                            }
                        }  
                    }
                }
            }
        }  
        // If nothing was found, return null
        return null;
    }

    // The DefaultMutableTreeNode only allows one piece of information
    // to be kept with it and that is the label that it displays.
    // Sometimes, we need the Protocol's actual name.  tabList is the
    // source of these nodes and has the labels and names.  They
    // are in tree format, so we need to go through each element of
    // the tree looking for this "label" and then return the name.
    // 
    // Dublicate labels should not be allowed unless they point to the
    // same fullpath.  We will find the first one.
    public static String getProtocolName(String label) {
        // Loop through the tabList and look for a node with this label
        // if the node is 
        for(int i=0; i < tabList.size(); i++) {
            ExpSelEntry level1 = (ExpSelEntry) tabList.get(i);
            String label1 = level1.getLabel();
            // If Leaf, check to see if label matches
            if(level1.entryListCount() == 0) {
                if(label1.equals(label)) {
                    return level1.getName();
                }
            }
            // Find sub nodes within this node
            HashArrayList list2 = level1.getEntryList();
            for(int k=0; list2 != null && k < list2.size(); k++) {
                ExpSelEntry level2 = (ExpSelEntry) list2.get(k);
                String label2 = level2.getLabel();
                // If Leaf, check to see if label matches
                if(level2.entryListCount() == 0) {
                    if(label2.equals(label)) {
                        return level2.getName();
                    }
                }
                HashArrayList list3 = level2.getEntryList();
                for(int j=0; list3 != null && j < list3.size(); j++) {
                    ExpSelEntry level3 = (ExpSelEntry) list3.get(j);
                    String label3 = level3.getLabel();
                    // If Leaf, check to see if label matches
                    if(level3.entryListCount() == 0) {
                        if(label3.equals(label)) {
                            return level3.getName();
                        }
                    }
                    HashArrayList list4 = level3.getEntryList();
                    for(int l=0; list4 != null && l < list4.size(); l++) {
                        ExpSelEntry level4 = (ExpSelEntry) list4.get(l);
                        String label4 = level4.getLabel();
                        // If Leaf, check to see if label matches
                        if(level4.entryListCount() == 0) {
                            if(label4.equals(label)) {
                                return level4.getName();
                            }
                        }   
                        // This level is not used yet and may not ever be used
                        HashArrayList list5 = level4.getEntryList();
                        for(int m=0; list5 != null && m < list5.size(); m++) {
                            ExpSelEntry level5 = (ExpSelEntry) list5.get(l);
                            String label5 = level5.getLabel();
                            // If Leaf, check to see if label matches
                            if(level5.entryListCount() == 0) {
                                if(label5.equals(label)) {
                                    return level5.getName();
                                }
                            }
                        }  
                    }
                }
            }
        }  
        // If nothing was found, return null
        return null;
    }



    /**
     * listens to menu selections
     */
    class PopActionListener implements ActionListener {
        /** button */
        private DraggableLabel label;

        /**
         * constructor
         * 
         * @param popButton
         *            PopButton
         */
        PopActionListener(DraggableLabel label) {
            this.label = label;
        } // PopActionListener()

        /**
         * action performed
         * 
         * @param evt
         *            event
         */
        public void actionPerformed(ActionEvent evt) {
            Util.setMenuUp(false);
            String cmd = evt.getActionCommand();
            // We should have a protocol here and the cmd is the fullpath of the
            // protocol along with a comma separated list of info.  We just
            // want the fullpath up to the first comma
            int index = cmd.indexOf(",");
            String fullpath = cmd.substring(0, index);

            ShufflerItem shufflerItem;
            // If the Ctrl key was held down, go for the system
            // protocol if there is one
            int modifierMask = evt.getModifiers();
            if((modifierMask & ActionEvent.CTRL_MASK) != 0) {
                // We need to get the system protocol instead of the
                // one given in the fullpath in cmd.  Get the protocol name
                // and then get the path we want
                index = fullpath.lastIndexOf(File.separator);
                String name = fullpath.substring(index +1);
                String systemProtPath = getSystemProtPath(name);
                if(systemProtPath != null)
                    shufflerItem = new ShufflerItem(systemProtPath,
                                                "EXPSELECTOR");
                else
                    // If null was returned, a system protocol was
                    // not found, just use the default one
                    shufflerItem = new ShufflerItem(fullpath, "EXPSELECTOR");
            }
            else
                shufflerItem = new ShufflerItem(fullpath, "EXPSELECTOR");
            
            QueuePanel queuePanel = Util.getStudyQueue();
            if (queuePanel != null) {
                StudyQueue studyQueue = queuePanel.getStudyQueue();
                if (studyQueue != null) {
                    ProtocolBuilder protocolBuilder = studyQueue.getMgr();
                    // Add this item to the studyQueue
                    protocolBuilder.appendProtocol(shufflerItem);
                }
            }
            // No StudyQ, load it.
            else {
                shufflerItem.actOnThisItem("Canvas", "DragNDrop", "");
            }

        } // actionPerformed()
    } // class PopActionListener



    // Allow us to get a list of all ExperimentSelector_*.xml files
    static public class ESFileFilter implements FilenameFilter
    {
        // Allow all ExperimentSelector_*.xml files
        public boolean accept(File dir, String name) {
            if(name.endsWith(".xml")  &&  name.indexOf("ExperimentSelector") != -1)
                return true;
            else
                return false;
        }
    }


    // Creating a ESTransferHandler basically just saves the dragged
    // ShufflerItem list.
    class ESTransferHandler extends TransferHandler {

        // Tell caller what mode we want to allow
        public int getSourceActions(JComponent c) {
            return TransferHandler.COPY;
        }

        // Create and return the transferable which is an ArrayList of
        // ShufflerItem's
        public Transferable createTransferable(JComponent comp)
        {
            // Get the list of fullpath items that were dragged
            ArrayList<String> list = getSelectionList();
            
            // If list is empty, this is probably a parent node and
            // not a protocol.  Just bail out here and the drag never
            // gets started.  The user just sees that he can't drag
            // this item.
            if(list.size() == 0)
                return null;
            ESTransferable tf;
            
            // If only one item selected, just transfer a single ShufflerItem
            if(list.size() == 1 ) {
                String fullpath = list.get(0);
                ShufflerItem item = new ShufflerItem(fullpath, "EXPSELECTOR");
                tf = new ESTransferable(item);
            }
            else {
                // Create a list of ShufflerItems from the fullpath list
                ArrayList<ShufflerItem> items = new ArrayList<ShufflerItem>();
                for(int i=0; i < list.size(); i++) {
                    String fullpath = list.get(i);
                    ShufflerItem item = new ShufflerItem(fullpath, "EXPSELECTOR");

                    if(item == null)
                        continue;

                    items.add(item);
                }

                tf = new ESTransferable(items);
            }

            return tf;
        }


        public boolean canImport(TransferSupport supp) {
            // Check for flavor
            if (!supp.isDataFlavorSupported(dataFlavor)) {
                return false;
            }
            return false;
        }

        public ArrayList<String> getSelectionList() {
            ArrayList<String> list=null;
            TreePath[] treePaths;
            TreePath treePath;
            DefaultMutableTreeNode node;

            list = new ArrayList<String>();

            treePaths = tree.getSelectionPaths();

            for(int i=0; i < treePaths.length; i++) {
                treePath = treePaths[i];
                node = (DefaultMutableTreeNode) treePath.getLastPathComponent();
                String label = (String)node.getUserObject();
                
                // The node only contains the name and not the fullpath.
                // look in tabList for the fullpath for this name.
                String fullpath = getFullpath(label);
                if(fullpath != null)
                    list.add(fullpath);
            }
            return list;
        }
    }

    class ESTransferable implements Transferable {
        Object componentToPass;
        
        public ESTransferable(Object comp) {
            // Save the component which is an ArrayList of ShufflerItem
            componentToPass = comp;
        }
 
        public Object getTransferData(DataFlavor flavor)
                throws UnsupportedFlavorException, IOException {
            // Return the component, which is an ArrayList of ShufflerItem
            return componentToPass;
        }
        public DataFlavor[] getTransferDataFlavors() {
            // Make a list of length 1 containing our dataFlavor
            DataFlavor flavorList[] = new DataFlavor[1];
            flavorList[0] = dataFlavor;
            return flavorList;
        }
        public boolean isDataFlavorSupported(DataFlavor flavor) {
            if(flavor.equals(dataFlavor))
                return true;
            else
                return false;
        }

    }
    
    // Double Click Btn 1 Opens protocol in Study Queue
    private class treeMouseListener extends MouseAdapter  {

        public treeMouseListener() {
            super();
        }

        public void mouseClicked(MouseEvent evt) {
            int btn = evt.getButton();
            
            // Double Click Btn 1 Opens protocol in SQ
            if(btn == MouseEvent.BUTTON1 && evt.getClickCount() == 2) {
                DefaultMutableTreeNode nodeClicked;

                TreePath path = ExpSelTree.tree.getSelectionPath();
                if(path == null)
                    return;
                nodeClicked = (DefaultMutableTreeNode)path.getLastPathComponent();
                
                // Only open leaf nodes (protocols)
                if(!nodeClicked.getAllowsChildren()) {
                    String label = (String)nodeClicked.getUserObject();
                    String fullpath = getFullpath(label);
                    ShufflerItem shufflerItem = new ShufflerItem(fullpath, "EXPSELECTOR");
                    QueuePanel queuePanel = Util.getStudyQueue();
                    if (queuePanel != null) {
                        StudyQueue studyQueue = queuePanel.getStudyQueue();
                        if (studyQueue != null) {
                            ProtocolBuilder protocolBuilder = studyQueue.getMgr();
                            // Add this item to the studyQueue
                            protocolBuilder.appendProtocol(shufflerItem);
                        }
                    }
                    // There is no studyQ, send to canvas
                    else {
                        shufflerItem.actOnThisItem("Canvas", "DragNDrop", "");
                    }
                }
            }
            else if(btn == MouseEvent.BUTTON3) {
                // Java does not select a node due to Btn3 click.  However,
                // we need to know what has been clicked on
                // Get the location where the click took place
                Point location = evt.getPoint();
                selPath = tree.getClosestPathForLocation(location.x, location.y);
                selNode = (DefaultMutableTreeNode)selPath.getLastPathComponent();
                
                
                String label = (String)selNode.getUserObject();
                String protocol = getProtocolName(label);
                
                // Check for help entry for protocol name.  If none, try the label
                boolean haveit = CSH_Util.haveTopic(protocol);
                if(!haveit)
                    haveit = CSH_Util.haveTopic(label);
                
                // If no help, don't put up the menu
                if(!haveit)
                    return;


                // Create the menu and show it
                 JPopupMenu helpMenu = new JPopupMenu();
                 String helpLabel = Util.getLabel("Sequence Help");
                 JMenuItem helpMenuItem = new JMenuItem(helpLabel);
                 helpMenuItem.setActionCommand("help");
                 helpMenu.add(helpMenuItem);
                     
                 ActionListener alMenuItem = new ActionListener()
                     {
                         public void actionPerformed(ActionEvent e)
                         {
                             // Get the protocol for this object
                             String label = (String)selNode.getUserObject();
                             String protocol = getProtocolName(label);
                             
                             boolean haveit = CSH_Util.haveTopic(protocol);
                             if(haveit) {
                              // Get the ID and display the help content
                                 CSH_Util.displayCSHelp(protocol);
                             }
                             else {
                                 // If not help for protocol name, use the label
                                 haveit = CSH_Util.haveTopic(label);
                                 if(haveit) {
                                     // Get the ID and display the help content
                                     CSH_Util.displayCSHelp(label);
                                 }
                             }
                         }
                     };
                 helpMenuItem.addActionListener(alMenuItem);
                     
                 // Use "tree" as the component to set relative to.  If I use
                 // "this" or "jsp", the menu is in the wrong position
                 // due to the scrolling of the tree within the scrollpane
                 helpMenu.show(tree, (int)location.getX(), (int)location.getY());

            }

        }
    }  /* End treeMouseListener class */

  

}
