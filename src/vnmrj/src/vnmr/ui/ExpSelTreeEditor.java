/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Point;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.Transferable;
import java.awt.datatransfer.UnsupportedFlavorException;
import java.awt.event.*;
import java.io.*;

import javax.swing.*;
import javax.swing.tree.*;


import java.util.*;

import vnmr.bo.LoginService;
import vnmr.bo.User;
import vnmr.ui.ExpSelEntry;
import vnmr.util.DebugOutput;
import vnmr.util.FileUtil;
import vnmr.util.UNFile;
import vnmr.util.Util;
import vnmr.util.Messages;
import vnmr.util.ModalDialog;
import vnmr.util.HashArrayList;

/********************************************************** <pre>
 * Summary: Create a panel for rearranging the order of the tabs in
 *          the Experiment Selector
 *
 </pre> **********************************************************/

public class ExpSelTreeEditor extends ModalDialog implements ActionListener
{

    private static DataFlavor dataFlavor = null;
    static private DefaultMutableTreeNode nodeClicked;
    // Array of paths for Copy/Paste
    static private TreePath[] copiedSelPaths=null;
    static private DefaultMutableTreeNode selNode;
    static private TreePath selPath=null;

    private JScrollPane scrollPane;
    private static JTree tree;
    private JPanel upanel = null;

    private DefaultMutableTreeNode rootNode;
    private DefaultTreeModel treeModel;
    ArrayList<String> tabNameList;
    // HashArrayList of ExpSelEntry objects.  This is the Tree info.
    HashArrayList tabList;
    JButton defaultButton;
    String strOk = vnmr.util.Util.getLabel("blOk");
    String strCancel = vnmr.util.Util.getLabel("blCancel");
    String strDefault = vnmr.util.Util.getLabel("Default");
    // The first level0 (tab) label we display
    // This is likely Common or Favorites
    String firstLevel0NodeLabel=null;
    // We want to display the Tree with the first node expanded
    DefaultMutableTreeNode firstNode=null;
   
    public ExpSelTreeEditor() {

        super(vnmr.util.Util.getLabel("Edit Exp Sel Order"));

        try {
            dataFlavor = new DataFlavor(DataFlavor.javaJVMLocalObjectMimeType+";class="+DefaultMutableTreeNode.class.getName());
        } 
        catch (ClassNotFoundException e) {
            Messages.writeStackTrace(e, "Problem Editing Exp Sel Order");
            return;
        }
        
        // Create the top root node
        rootNode = new DefaultMutableTreeNode(vnmr.util.Util.getLabel("Edit Exp Sel Order"));
        treeModel = new DefaultTreeModel(rootNode);

        // Create the JTree
        tree = new JTree(rootNode);
        tree.setModel(treeModel);

        // Don't show the root node
        tree.setRootVisible(true);
        // Show the expand/collapse control for the top level nodes
        tree.setShowsRootHandles(true);
        
        tree.setDragEnabled(true);

//        tree.setDropMode(DropMode.INSERT);
        tree.setDropMode(DropMode.ON_OR_INSERT);
        TransferHandler transferHandler = new ESTransferHandler();
        tree.setTransferHandler(transferHandler);
        
        tree.addMouseListener(new treeMouseListener());
        
        // Allow editing of non-leaf nodes
        tree.setEditable(true);
        TreeCellEditor editor = tree.getCellEditor();
        DefaultTreeCellRenderer renderer = (DefaultTreeCellRenderer) tree.getCellRenderer();
        tree.setCellEditor(new nonLeafCellEditor(tree, renderer, editor));
        
        createTreeNodes();

        upanel = new JPanel();
        // Put the panel into the Frame
        getContentPane().add(upanel, BorderLayout.NORTH);

        // Create a new ScrollPane with this new tree
        scrollPane = new JScrollPane(tree);
        
        // I cannot get this scrollpane or the JTree to track in size
        // with the JFrame.  I had to set the sizes manually to get it
        // to look right.
        scrollPane.setPreferredSize(new Dimension(300, 500));

        upanel.add(scrollPane);
        
        defaultButton = new JButton(strDefault);
        defaultButton.setActionCommand(strDefault);
        buttonPane.add(defaultButton);
        
        // Add listeners to the buttons in ModalDialog
        okButton.addActionListener(this);
        cancelButton.addActionListener(this);
        defaultButton.addActionListener(this);

        pack();
        setLocationRelativeTo(getParent());
    }

 
    public void createTreeNodes() {
        tabList = ExpSelTree.getTabList();
        fillTree(tabList);

    }
    
    // Fill the tree from tabList
    public void fillTree (HashArrayList esTreeList) {
        
        if(esTreeList == null || esTreeList.size() == 0) {
            Messages.postError("TabList is Empty");
            return;
        }
        
        // Clear any previous nodes
        rootNode.removeAllChildren();
        
        //Loop through the esTreeList and create nodes
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
            
            // Save the first node so we can do the "Add To ???" menu item
            // It will add to the top node.
            if(firstNode == null) {
                firstNode = node1;
                // We also want the label for this node saved
                firstLevel0NodeLabel = (String)node1.getUserObject();
                
            }

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
                            node4.add(node5);

                            
                        }
                       
                        
                    }

                }


            }


        }
        // This is required to get the new nodes to show up
        TreePath rootpath = new TreePath(treeModel.getPathToRoot(rootNode));
        tree.expandPath(rootpath);
        
    }
    
    
    // Respond to button clicks. If OK, write out a ExperimentSelector.xml
    // file to the users interface directory.  This will override the system
    // file and will order items approprately.
    
    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();

        if (cmd.equalsIgnoreCase(strOk)) {
            RandomAccessFile esXmlFile=null;

            unShowPanel();

            // There is a persistence file for each operator
            String curOperator = Util.getCurrOperatorName();

            String path = FileUtil.savePath("USER/PERSISTENCE/ExpSelOrder_"
                                            +  curOperator + ".xml");
            if(path == null) {
                Messages.postError("Problem opening " + path);
                return;
            }
            // Make a new file for output with 'new' appended to the name.  Then if we
            // error out, we have not destroyed the previous file (if any)
            String tmppath = path + ".new";

            UNFile file = new UNFile(path);
            UNFile outFile = new UNFile(tmppath);
            if(outFile.exists()) {
                outFile.delete();
            }
            // The file does not exist, create it and write the opening
            // line, the tabOrder line and closing lines to it.
            try {
                esXmlFile = new RandomAccessFile(outFile, "rw");
                // Write first line of file
                esXmlFile.writeBytes("<ProtocolList> \n");

                // Write the protocol lines.  Loop through all of the current Tree nodes
                // and write a line for each protocol in the order given.  Start with
                // rootNode.
                Enumeration <TreeNode> enumer = rootNode.preorderEnumeration();
                while(enumer.hasMoreElements()) {
                    String name="", label="", level0="", level1="", level2="";
                    DefaultMutableTreeNode node = (DefaultMutableTreeNode) enumer.nextElement();
                    
                    // We only want the bottom level which can be found by
                    // checking if children are allowed
                    if(node.getAllowsChildren())
                        continue;
                    
                    // This must be a protocol, Get the array of nodes above this.
                    // ppath[max] = label
                    // ppath[0] = root node
                    // ppath[1] = level0;
                    // ppath[2] = level1;  May not exist
                    // ppath[3] = level2;  May not exist
                    TreeNode[] ppath = node.getPath();
                    DefaultMutableTreeNode ppathmax = (DefaultMutableTreeNode)ppath[ppath.length -1];
                    label = (String)ppathmax.getUserObject();
                    name = ExpSelTree.getProtocolName(label);
                    String begStr;
                    String endStr;
                    int index;
                    while((index = label.indexOf(" \n"))>= 0) {
                       begStr = label.substring(0, index);
                       endStr = label.substring(index +2);
                       label = begStr + "\\\\n" + endStr;
                    }
                    DefaultMutableTreeNode ppath1 = (DefaultMutableTreeNode)ppath[1];
                    level0 = (String)ppath1.getUserObject();
                    if(ppath.length >=  4) {
                        DefaultMutableTreeNode ppath2 = (DefaultMutableTreeNode)ppath[2];
                        level1 = (String)ppath2.getUserObject();
                    }
                    if(ppath.length >=  5) {
                        DefaultMutableTreeNode ppath3 = (DefaultMutableTreeNode)ppath[3];
                        level2 = (String)ppath3.getUserObject();
                    }

                    esXmlFile.writeBytes("  <protocol name=\"" + name + "\"");
                    esXmlFile.writeBytes(" label=\"" + label + "\"");
                    esXmlFile.writeBytes(" level0=\"" + level0 + "\"");
                    esXmlFile.writeBytes(" level1=\"" + level1 + "\"");
                    esXmlFile.writeBytes(" level2=\"" + level2 + "\" /> \n");
                }

                // Final line of the file
                esXmlFile.writeBytes("\n</ProtocolList>\n");
                esXmlFile.close();

                if(file.exists()) {
                    file.delete();
                }
                // Rename the new file to the proper name
                outFile.renameTo(file);
            }
 

            
            catch (Exception ex)  {
                Messages.writeStackTrace(ex, "Problem creating " + path);
                return;
            }

            // If the user has created a new folder which is still empty, warn him
            // that it will be discarded.  Just find the first one.  If there are more than
            // one, he should get the message that he can't have empty folders.
            Enumeration <TreeNode> enumer = rootNode.preorderEnumeration();
            while(enumer.hasMoreElements()) {
                String name="", label="", level0="", level1="", level2="";
                DefaultMutableTreeNode node = (DefaultMutableTreeNode) enumer.nextElement();
                boolean giveWarning = false;
                if(node.getAllowsChildren()) {
                    int count = node.getChildCount();
                    if(count == 0) {
                        giveWarning = true;
                    }
                    else if(count == 1){
                        // There is only one child.  See if it is the place holder
                        // with 3 spaces.
                        DefaultMutableTreeNode child = (DefaultMutableTreeNode) node.getChildAt(0);
                        String childlabel = (String)child.getUserObject();
                        if(childlabel.equals("   ")) 
                            giveWarning = true;

                    }
                    if(giveWarning) {
                        label = (String)node.getUserObject();
                        Messages.postWarning("An empty folder was found.  Since empty "
                                + "folders cannot be kept, the folder \"" + label + "\" is "
                                + "being discarded.");
                        break;
                    }
                }
            }
        
            

            // Update the Experiment Selector immediately
            ExpSelTree.updateExpSel();
            ExpSelector.updateExpSel();
        }
        else if (cmd.equalsIgnoreCase(strCancel)) {
            unShowPanel();
        }
        else if (cmd.equalsIgnoreCase(strDefault)) {
            // Rename this users ExperimentSelector.xml file so that the
            // system will go back to using the system file.
            
            // There is a persistence file for each operator
            String curOperator = Util.getCurrOperatorName();

            String path = FileUtil.savePath("USER/PERSISTENCE/ExpSelOrder_"
                                            +  curOperator + ".xml");

            // If it does not exist for this operator, see if one exists for the user.
            // If so, then don't remove it, but tell the operator what is going on.
            if(path == null) {
                String curuser = System.getProperty("user.name");
                LoginService loginService = LoginService.getDefault();
                Hashtable userHash = loginService.getuserHash();
                User user = (User) userHash.get(curuser);

                String userName = user.getAccountName();
                path = FileUtil.openPath("USER/PERSISTENCE/ExpSelOrder_"
                                             + userName + ".xml");
                if (path != null) {
                    // The operator does not have a ExpSelOrder_op.xml file,
                    // but the user does.  That means that the user level file
                    // is controlling things, and the operator cannot go to the
                    // system defaults.
                    Messages.postWarning("This operator does not have his own Order "
                            + "specified and is thus already operating on the user level "
                            + "defaults.  Aborting.");
                    return;
                }
                else
                    // No ExpSelOrder_ file exists, just exit
                    return;
            }

            String outPath = path + ".bk";
            UNFile file = new UNFile(path);
            UNFile outFile = new UNFile(outPath);
            file.renameTo(outFile);

            // We need to regenerate the ExperimentSelector_operator.xml file
            Util.getAppIF().sendToVnmr("updateExpSelector");
            
            // Update the Experiment Selector immediately
            ExpSelTree.updateExpSel();
            ExpSelector.updateExpSel();

            
            // Close the panel.  If it were to be left open, we would have to
            // update it.  There is not an update available at the moment.
            unShowPanel();

        }
 
    }
    
    public void showPanel() {
        setVisible(true);
    }

    public void unShowPanel() {
        setVisible(false);
    }
    
    public void destroyPanel() {
        try {
            dispose();
            finalize();
        }
        catch (Throwable e) {
            
        }
    }
    
    public boolean nodeContainsLabel(String label, DefaultMutableTreeNode node) {
        
        Enumeration <TreeNode> list = node.children();
        while(list.hasMoreElements()) {
            DefaultMutableTreeNode child = (DefaultMutableTreeNode) list.nextElement();
            String cLabel = (String)child.getUserObject();
            if(cLabel.equals(label)) {
                Messages.postWarning(label + " is a duplicate and cannot be inserted here.");
                return true;
            }
        }

        return false;
    }
    
    // Since user protocols may contain their own definitions as to level0
    // level1 and level2, we cannot let the editor try to delete or move these.
    // If we do, they come back as soon as the panel is updated.
    public boolean isUserProtocol(String label) {
        // Get the fullpath for this item
        String fullpath = getFullpath(label);
        
        // See if the fullpath starts with this user's home
        // If this is a folder label, the fullpath will be null.
        if(fullpath != null && fullpath.indexOf(Util.USERDIR) > -1) {
            // This protocol is in the user's area
            return true;
        }
        else
            return false;
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

    
    // Creating a ESTransferHandler basically just saves the dragged
    // component which is the JTree.
    class ESTransferHandler extends TransferHandler {

        // Tell caller what mode we want to allow
        // For reordering the tabs, just allow MOVE.
        // The ES does not allow duplicate tab names
        public int getSourceActions(JComponent c) {
            return TransferHandler.MOVE;
        }

        public Transferable createTransferable(JComponent comp)
        {
            // Transfer the component, which is the JTree
            ESTransferable tf = new ESTransferable(comp);

            return tf;
        }

        public boolean canImport(TransferSupport supp) {
            // Check for flavor
            if (!supp.isDataFlavorSupported(dataFlavor)) {
                return false;
            }
                 
            return true;
        }

        // A node has been dropped and we are to move it
        public boolean importData(TransferSupport supp) {
            DefaultMutableTreeNode fromParent;
            DefaultMutableTreeNode toParent;

            if (!canImport(supp)) {
                return false;
            }

            // Fetch the Transferable and its data
            Transferable t = supp.getTransferable();
            try {
                Object data = t.getTransferData(dataFlavor);
                // data is a JTree
                JTree dTree = (JTree)data;
                
                // tPath is the array of all selected items being dragged
                TreePath tPath[] = ExpSelTreeEditor.tree.getSelectionPaths();
                
                
                // Get the first item and use it to get the parent of the group
                TreePath path0 = tPath[0];
                Object[] obj0 = path0.getPath();
                fromParent = (DefaultMutableTreeNode)obj0[obj0.length -2];

                ArrayList <String> labels = new ArrayList <String> ();
                ArrayList <DefaultMutableTreeNode> nodes = new ArrayList <DefaultMutableTreeNode> ();

                // Get the list of nodes to be dragged and the labels
                for(int i=0; i < tPath.length; i++) {
                    TreePath item = tPath[i];
                    DefaultMutableTreeNode tNode;
                    tNode = (DefaultMutableTreeNode)item.getLastPathComponent();
                    nodes.add(tNode);
                    String label = (String)tNode.getUserObject();
                    labels.add(label);
                }

                // Get the drop location as an index down the tree.
                // With the current one level tree, index = 0 means the drop
                // was above the first item and index = max means it is after
                // the last item.
                JTree.DropLocation dloc= ((JTree)data).getDropLocation();
                TreePath location = dloc.getPath();
                toParent = (DefaultMutableTreeNode)location.getLastPathComponent();
                
                // -1 means dropped on a parent itself.  Assume 0 in that case
                int indexTo = dloc.getChildIndex();
                if(indexTo == -1)
                    indexTo = 0;
                
                // What is the index of the node being moved/copied?
                int indexFrom = fromParent.getIndex(nodes.get(0));


                // Is this a copy or move
                int mode = supp.getUserDropAction();
                if (mode == TransferHandler.COPY){
                    // Cntrl drag ONLY works if you start the drag and THEN
                    // press control.  If you press cntrl and then click
                    // and drag, it does not allow dragging at all.  I see
                    // in Java's BasicTreeUI.mousePressedDND() around line 3483 it says
                    // that if Cntrl, don't do anything yet.  Unfortunately,
                    // it never does anything.  I believe this is a
                    // JAVA BUG.  Specify in the manual that they need to start
                    // the drag first.
                    
                    // 11/2/12 Java has been modified so that the above is still
                    // true, however, as soon as you press the Ctrl key, it shows
                    // the "no drop" icon.  Ie., they fixed the confusion by
                    // disallowing Ctrl Drop entirely.  So, we can never get here.
                    
                    // Make a copy of the node and insert it so that we
                    // keep the original node in place also.

                    // For Tab Ordering, we do not want to copy anyway since
                    // there cannot be duplicate tabs.  We may need this when
                    // we expand to D&D protocols, menus etc.
//                    DefaultMutableTreeNode newNode = (DefaultMutableTreeNode)node.clone();
//                    treeModel.insertNodeInto(newNode, toParent, indexTo);              
                }
                else if (mode == TransferHandler.MOVE) {
                    // Note: The removal and insertion are done by calling
                    // methods of the DefaultTreeModel, NOT the rootNode
                                        
                    // If we are moving the node up, then we just insert
                    // it at indexTo.  If however, we are moving it down,
                    // the insert location will have been changed (-1) due to
                    // the removal of the node. Only if same parent.
                    if (fromParent == toParent && indexTo > indexFrom)
                        indexTo = indexTo -1;
                    
                    if(!toParent.getAllowsChildren()) {
                        // They must have dropped on a protocol.  Get its parent
                        // We allow dropping on a leaf in case it is a new folder
                        // with no contents yet.
                        DefaultMutableTreeNode pnode = toParent;
                        toParent = (DefaultMutableTreeNode)toParent.getParent();
                        
                        // Get the index of this protocol node within its parent
                        indexTo = toParent.getIndex(pnode);   
                    }
                    
                    // We do not want to allow two protocols with the same label
                    // to be displayed in the same node of the tree.  So, If the
                    // toParent and fromParent are the same, we are dragging within
                    // the same node.  Allow a protocol of the dragged name to
                    // exist in this node, since it will be the one we are dragging.
                    // If, however, toParent != fromParent, then don't allow the
                    // drop to complete if a protocol exist with this label already.
                    if(toParent != fromParent) {
                        for(int i=0; i < labels.size(); i++) {
                            String label = labels.get(i);

                            if(nodeContainsLabel(label, toParent)) {
                                // toParent already contains a protocol with this label,
                                // nodeContainsLabel() outputs a warning, just bail out
                                return false;
                            }
                        }
                    }

                    // If this is a folder, isUserProtocol will return false
                    for(int i=0; i < labels.size(); i++) {
                        String label = labels.get(i);
                        if(isUserProtocol(label)) {
                            // This is a user protocol, Warn them
                            Messages.postWarning("You are moving the entry for a "
                                    + "local protocol.  If this location is defined "
                                    + "in the protocol itself, it will come back when "
                                    + "the Exp Sel is updated.  To remove the protocol itself, use "
                                    + "the Experiment Selector.");
                        }
                    }

                    if(toParent == fromParent) {
                        // If dragging within the same parent, add nodes in order
                        for(int i=0; i < nodes.size(); i++) {
                            DefaultMutableTreeNode node = nodes.get(i);
                            // Remove the node from its current location
                            treeModel.removeNodeFromParent(node);

                            // Put it into the new location
                            treeModel.insertNodeInto(node, toParent, indexTo);

                            // If this node contains a node with a blank label,
                            // we want to remove it.  It was added in "New Folder"
                            // just to make the node look like a folder.
                            // Go through nodes in toNode and see if any have
                            // UserObject == 3 spaces
                            Enumeration <TreeNode>enumer;
                            //                    enumer = node.preorderEnumeration();            
                            enumer = toParent.children();            
                            while(enumer.hasMoreElements()) {
                                DefaultMutableTreeNode tnode = (DefaultMutableTreeNode) enumer.nextElement();
                                String userobj = (String)tnode.getUserObject();
                                if(userobj.equals("   ")) {
                                    treeModel.removeNodeFromParent(tnode);
                                    break;
                                }
                            }
                        }
                    }
                    else {
                        // If dragging to a different parent, add nodes in reverse order
                        for(int i=nodes.size() -1; i >= 0; i--) {
                            DefaultMutableTreeNode node = nodes.get(i);
                            // Remove the node from its current location
                            treeModel.removeNodeFromParent(node);

                            // Put it into the new location
                            treeModel.insertNodeInto(node, toParent, indexTo);

                            // If this node contains a node with a blank label,
                            // we want to remove it.  It was added in "New Folder"
                            // just to make the node look like a folder.
                            // Go through nodes in toNode and see if any have
                            // UserObject == 3 spaces
                            Enumeration <TreeNode>enumer;
                            //                    enumer = node.preorderEnumeration();            
                            enumer = toParent.children();            
                            while(enumer.hasMoreElements()) {
                                DefaultMutableTreeNode tnode = (DefaultMutableTreeNode) enumer.nextElement();
                                String userobj = (String)tnode.getUserObject();
                                if(userobj.equals("   ")) {
                                    treeModel.removeNodeFromParent(tnode);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            catch(Exception ex) {
                Messages.writeStackTrace(ex);
                return false;
            }

            return true;
        }

    }

    class ESTransferable implements Transferable {
        JComponent componentToPass;
        
        public ESTransferable(JComponent comp) {
            // Save the component, which is actually the JTree
            componentToPass = comp;
        }
 
        public Object getTransferData(DataFlavor flavor)
                throws UnsupportedFlavorException, IOException {
            // Return the component, which is actually the JTree
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

    private class treeMouseListener extends MouseAdapter  {

        public treeMouseListener() {
            super();
        }

        public void mouseClicked(MouseEvent evt) {
            Point pos = new Point(evt.getX(), evt.getY());
            int btn = evt.getButton();

            if(btn == MouseEvent.BUTTON3) {
                // Create the Menu
                JPopupMenu editMenu = new JPopupMenu();
                
                String copyLabel = Util.getLabel("Copy");
                JMenuItem copyMenuItem = new JMenuItem(copyLabel);
                copyMenuItem.setActionCommand("Copy");
                editMenu.add(copyMenuItem);
                
                String pasteLabel = Util.getLabel("Paste");
                JMenuItem pasteMenuItem = new JMenuItem(pasteLabel);
                pasteMenuItem.setActionCommand("Paste");
                editMenu.add(pasteMenuItem);
                
                String deleteLabel = Util.getLabel("Delete Entry");
                JMenuItem deleteMenuItem = new JMenuItem(deleteLabel);
                deleteMenuItem.setActionCommand("Delete");
                editMenu.add(deleteMenuItem);
                               
                String newLabel = Util.getLabel("New Folder");
                JMenuItem newMenuItem = new JMenuItem(newLabel);
                newMenuItem.setActionCommand("New Folder");
                editMenu.add(newMenuItem);
                
                // Make a menu item of "Add To [top node]"
                // It is meant to be something like "Add To Favorites"
                String addLabel = Util.getLabel("Add To");
                addLabel = addLabel + " " + firstLevel0NodeLabel;
                JMenuItem addMenuItem = new JMenuItem(addLabel);
                addMenuItem.setActionCommand("Add To");
                editMenu.add(addMenuItem);
                
                // Java does not select a node due to Btn3 click.  However,
                // we need to know what has been clicked on to do the
                // copy/paste/delete
                // Get the location where the click took place
                Point location = evt.getPoint();
                selPath = tree.getClosestPathForLocation(location.x, location.y);
                selNode = (DefaultMutableTreeNode)selPath.getLastPathComponent();
                
                
                ActionListener alMenuItem = new ActionListener()
                    {
                        public void actionPerformed(ActionEvent ae)
                        {
                            String cmd = ae.getActionCommand();
                            int indexTo;
                            
                            // This depends on a selection already having been made.
                            // Right click does not set the selection.
                            
                            if(cmd.equals("Copy")) {
                                // This gets a little convoluted.  If the user is just
                                // right clicking on a single item, there may not be 
                                // any selections, or the selections may be wrong.
                                // On the other hand, they may be trying to drag a group
                                // and not just the one that was right clicked on.
                                // Sooooo, Check to see if the item right clicked on
                                // is in the selection group.  If so, copy the group.
                                // If the item right clicked on is not in the group,
                                // then just copy the item clicked on.
                                copiedSelPaths = ExpSelTreeEditor.tree.getSelectionPaths();
                                if(copiedSelPaths == null) {
                                    // Nothing was selected, so used the item which was
                                    // right clicked on
                                    copiedSelPaths = new TreePath[] {selPath};
                                }
                                else {
                                    boolean foundMatch = false;
                                    // See if selPath is in copiedSelPaths
                                    for(int i=0; i < copiedSelPaths.length; i++) {
                                        TreePath item = copiedSelPaths[i];
                                        if(item == selPath) {
                                            // They clicked within the selected group
                                            // Use copiedSelPaths as it is.
                                            foundMatch = true;
                                            break;
                                        }
                                    }
                                    if(!foundMatch)
                                        // They did not click in the selected group,
                                        // so just use the selPath
                                        copiedSelPaths = new TreePath[] {selPath};
                                }
                            }
                            if(cmd.equals("Paste")) {
                                DefaultMutableTreeNode toNode;
                                DefaultMutableTreeNode parent;

                                ArrayList <String> labels = new ArrayList <String> ();
                                parent = (DefaultMutableTreeNode)selNode.getParent();


                                if(copiedSelPaths == null) {
                                    Messages.postWarning("No items have been Copied.  Aborting Paste");
                                    return;
                                }
                                
                                // Get the labels of all nodes selected
                                for(int i=0; i < copiedSelPaths.length; i++ ) {
                                    TreePath item = copiedSelPaths[i];
                                    DefaultMutableTreeNode node;
                                    node = (DefaultMutableTreeNode)item.getLastPathComponent();
                                    String label = (String)node.getUserObject();
                                    labels.add(label);
                                }

                                // If node clicked is a protocol, then copy to its parent
                                if(selNode.getAllowsChildren()) {
                                    // They clicked on a folder, use it
                                    // Index 0 means add at the top of the folder
                                    indexTo = 0;
                                    toNode = selNode;
                                }
                                else {
                                    // They clicked on a protocol, get the parent
                                    toNode = parent;

                                    // Get the position where to insert.  Ask the
                                    // parent where this selNode is located
                                    indexTo = parent.getIndex(selNode);
                                }

                                // We do not want to allow two protocols with the same label
                                // to be displayed in the same node of the tree.  
                                // Don't allow the paste to complete if a protocol exist in parent
                                // with this label already
                                for(int i=0; i < labels.size(); i++) {
                                    String label = labels.get(i);
                                    if(nodeContainsLabel(label, toNode)) {
                                        // toParent already contains a protocol with this label,
                                        // nodeContainsLabel() outputs a warning, just bail out
                                        return;
                                    }
                                }

                                // Insert the saved nodes ahead of the current row
                                // Do this in reverse order so that they end up in the
                                // same order they were in the original location.
                                for(int i=copiedSelPaths.length -1; i >= 0; i-- ) {
                                    TreePath item = copiedSelPaths[i];
                                    DefaultMutableTreeNode node;
                                    node = (DefaultMutableTreeNode)item.getLastPathComponent();
                                    DefaultMutableTreeNode newnode = (DefaultMutableTreeNode)node.clone();
                                    treeModel.insertNodeInto(newnode, toNode, indexTo); 
                                }

                                // If this node contains a node with a blank label,
                                // we want to remove it.  It was added in "New Folder"
                                // just to make the node look like a folder.
                                // Go through nodes in toNode and see if any have
                                // UserObject == 3 spaces
                                Enumeration <TreeNode>enumer;
                                enumer = toNode.preorderEnumeration();            
                                while(enumer.hasMoreElements()) {
                                    DefaultMutableTreeNode node = (DefaultMutableTreeNode) enumer.nextElement();
                                    String userobj = (String)node.getUserObject();
                                    if(userobj.equals("   ")) {
                                        treeModel.removeNodeFromParent(node);
                                        //                                            return;
                                    }
                                }

                            }
                            if(cmd.equals("Delete")) {  
                                String label = (String)selNode.getUserObject();
                                
                                // If this is a folder, isUserProtocol will return false
                                if(isUserProtocol(label)) {
                                    // This is a user protocol, Warn them
                                    Messages.postWarning("You are deleting the entry for a "
                                           + "local protocol.  If this location is defined "
                                           + "in the protocol itself, it will come back when "
                                           + "the Exp Sel is updated.  To remove the protocol "
                                           + "itself, use the Experiment Selector.");
                                }
                                // Unselect any previously seleted items.
                                tree.clearSelection();
                                
                                // Remove it even if it is a user protocol.  This location
                                // may not be the one defined in the protocol.xml file
                                treeModel.removeNodeFromParent(selNode);
                            }
                            if(cmd.equals("New Folder")) {
                                DefaultMutableTreeNode toNode;
                                DefaultMutableTreeNode parent;
                                parent = (DefaultMutableTreeNode)selNode.getParent();

                                DefaultMutableTreeNode newNode = new DefaultMutableTreeNode("New Folder");
                                newNode.setAllowsChildren(true);
                                // If the newNode does not have any children, it shows up
                                // without the + and looks like a protocol.  Add a blank node
                                // then remove this node when the first real protocol
                                // is pasted or dropped to this newNode
                                DefaultMutableTreeNode blankNode = new DefaultMutableTreeNode("   ");
                                blankNode.setAllowsChildren(false);
                                newNode.add(blankNode);

                                // If node is a protocol, then copy to its parent
                                if(selNode.getAllowsChildren()) {
                                    // They clicked on a folder, use it
                                    // Index 0 means add at the top of the folder
                                    indexTo = 0;
                                    toNode = selNode;
                                }
                                else {
                                    // They clicked on a protocol, get the parent
                                    toNode = parent;

                                    // Get the position where to insert.  Ask the
                                    // parent where this selNode is located
                                    indexTo = parent.getIndex(selNode);
                                }

                                // We only want to allow 3 levels since that is all that
                                // the original ExpSel can handle.  To allow more levels
                                // than that, we would have to separate the two ES displays
                                int count;
                                DefaultMutableTreeNode levelup = toNode;
                                boolean tooManyLevels = true;
                                for(count=0; count < 3; count++) {
                                    levelup = (DefaultMutableTreeNode)levelup.getParent();
                                    if(levelup == null) {
                                        tooManyLevels = false;
                                        break;
                                    }
                                }

                                if(tooManyLevels) {
                                    Messages.postWarning("Only 3 levels of folders are "
                                            + "available, aborting New Folder command.");
                                    return;
                                }

                                treeModel.insertNodeInto(newNode, toNode, indexTo);  

                                // Set it as selected
                                // Get the TreeNode path
                                TreeNode nodes[] = newNode.getPath();
                                // Convert to TreePath
                                TreePath newPath = new TreePath(nodes);
                                // Set the new selection and put it into edit mode
                                // Note, the user must hit a return after his editing
                                // for the new name to take effect.
                                tree.setSelectionPath(newPath);
                                tree.startEditingAtPath(newPath);


                            }
                            if(cmd.equals("Add To")) {

                                // Unselect any previously seleted items.
                                tree.clearSelection();

                                // Copy the currently selected node to the top node
                                treeModel.insertNodeInto(selNode, firstNode, 0);             
                            }
                        }
                    };
                    
                copyMenuItem.addActionListener(alMenuItem);
                pasteMenuItem.addActionListener(alMenuItem);
                deleteMenuItem.addActionListener(alMenuItem);
                addMenuItem.addActionListener(alMenuItem);
                newMenuItem.addActionListener(alMenuItem);

                // The Y value obtained from the MouseEvent is the position as if
                // the scroll panel was displayed in its full length.  We want to
                // position on the display of the scrolled items.  Thus, get the
                // Y value of the top of the scroll pane and subtract that from
                // the Y value from the event.
                int scrollTop = scrollPane.getVerticalScrollBar().getValue();
                editMenu.show(ExpSelTreeEditor.this, (int)pos.getX(), 
                             (int)pos.getY() - scrollTop);
            }  
        }
    }  /* End treeMouseListener class */

    
    // Only allow editing of 
    class nonLeafCellEditor extends DefaultTreeCellEditor {

        public nonLeafCellEditor(JTree tree, DefaultTreeCellRenderer renderer, TreeCellEditor editor) {
            super(tree, renderer, editor);
        }

        public boolean isCellEditable(EventObject event) {
            boolean returnValue = super.isCellEditable(event);
            if (returnValue) {
                Object node = tree.getLastSelectedPathComponent();
                if ((node != null) && (node instanceof TreeNode)) {
                    TreeNode treeNode = (TreeNode) node;
                    returnValue = treeNode.getAllowsChildren();
                }
            }
            return returnValue;
        }
    }
}

