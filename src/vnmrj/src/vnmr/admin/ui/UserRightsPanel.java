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
import javax.swing.*;
import javax.swing.tree.*;

import java.awt.*;
import java.util.*;
import java.awt.event.*;
import javax.xml.parsers.*;

import vnmr.util.*;
import vnmr.ui.ExpSelector;
import vnmr.ui.ExpSelEntry;



/********************************************************** <pre>
 * Summary: CheckBox JTree for selecting user rights by admin
 *
 * This tool is for opening, creating, editing and saving "access profile".
 * Each user/operator will be assigned one of these "access profile".
 * Create a JTree then turn it into a tree with CheckBoxs for each item.
 * The contents of the tree, including which items are selected.
 *
 * For a new profile:
 * - first get of the the protocols from the system directory
 * - then read the system rightsList.xml file to get rights and tools.  This
 *   system rightsList.xml file can also specify defaults for the checkboxs
 *   on or off for any protocols desired.  No protocols are required in
 *   this file.  This file should be different for each appmode, and thus
 *   needs to be saved someplace like /vnmr/templasts/vnmrj/rights
 *
 * For opening an existing profile:
 * - Clear the current rightsList
 * - Open and parse the profile being opened
 * - Replace any existing JTree etc with new ones filled with new info
 *
 * Saving a profile:
 * - The profile will be saved in /vnmr/adm/users/rights
 *   These are not dependent on appmode, so should be in /vnmr/adm/users/rights
 *
 </pre> **********************************************************/

public class UserRightsPanel extends JFrame implements ActionListener
{
    private static String defaultLookAndFeel=null;

    private JScrollPane scrollPane;
    private RightsList rightsList;
    private CheckTreeManager checkTreeManager;
    private JTree tree;
    private DefaultMutableTreeNode rootNode;
    private DefaultTreeModel treeModel;
    private JFileChooser openSavePopup = null;
    private JPanel upanel = null;
    private String openedFile = null;
    // True for the adm panel, and false for the user panel
    private boolean adminPanel; 
    private HashMap leaves = new HashMap();
 
    String strOpen = vnmr.util.Util.getLabel("blOpen");
    String strSave = vnmr.util.Util.getLabel("blSave");
    String strSave_As = vnmr.util.Util.getLabel("blSave_As");
    String strNew = vnmr.util.Util.getLabel("blNew");
    String strDelete = vnmr.util.Util.getLabel("blDelete");
    String strClose = vnmr.util.Util.getLabel("blClose");

    // For panelType 'adm', open panel with a new info
    // For panelType 'operator', open panel with the current operator's
    // profile and any existing persistence
    // For operator 
    //     Don't have Open (they can only edit their own persistence)
    //     Don't have Save As (they can only save their own persistence)
    //     Save persistence as a list of items to NOT use.
    public UserRightsPanel(String panelType) {

        if(panelType.equals("adm"))
            adminPanel = true;
        else
            adminPanel = false;

        // Set the look and feel to metal for the creation of
        // this set of panels.
        setLookAndFeel("metal");

        getContentPane().setLayout(new BorderLayout());
        
        int width=Integer.parseInt(vnmr.util.Util.getOption("_admin_UserRightPanel_Frame_Width","600"));       
        Dimension dim = new Dimension(width, 600);

        setSize(dim);
        setPreferredSize(dim);
        
        if(adminPanel) {
            setTitle(vnmr.util.Util.getLabel("_admin_Edit_User_Rights_Profile")
                    +": "+vnmr.util.Util.getLabel("blNew"));
           // Create the initial list from the protocol files in the protocol 
            // dir
            // **** need correct directories

            // **** Reading the access profile should be done from the user
            // *****opening a profile.  It is in here for testing ***
            // Read the Rights list and create a RightsList object.  The list
            // of rights and tools along with defaults should be in this file.
            newProfile();
        }
        // User level configuration panel
        else {
            setTitle(vnmr.util.Util.getLabel("_admin_Edit_User_Config_Profile"));
            // Open this operators profile and overlay his persistence file
            // If none exists, RightsList will output an error
            // and give an empty rightsList.
            // In that case we need to abort the creation of the panel.
            boolean masterList = false;
            rightsList = new RightsList(masterList);
            if(rightsList.size() == 0)
                return;

            updateTree();
        }

        // Create the lower panel
        JPanel lPanel = new JPanel();
        // Layout buttons across the panel in one row
        lPanel.setLayout(new GridLayout(1,7));
        lPanel.setPreferredSize(new Dimension(width-20, 30));

        // Create All of the Buttons
        JButton openButton = new JButton(strOpen);
        openButton.addActionListener(this);

        JButton saveButton = new JButton(strSave);
        saveButton.addActionListener(this);

        JButton saveasButton = new JButton(strSave_As);
        saveasButton.addActionListener(this);

        JButton newButton = new JButton(strNew);
        newButton.addActionListener(this);

        JButton deleteButton = new JButton(strDelete);
        deleteButton.addActionListener(this);

        JButton closeButton = new JButton(strClose);
        closeButton.addActionListener(this);

        // For operator, skip the open, save as, and new buttons
        if(adminPanel) 
            lPanel.add(openButton);

        lPanel.add(saveButton);

        if(adminPanel) {
            lPanel.add(saveasButton);
            lPanel.add(newButton);
            lPanel.add(deleteButton);
        }

        lPanel.add(closeButton);

        // Add the lower panel to the bottom of the this frame
        getContentPane().add(BorderLayout.SOUTH, lPanel);


        // I cannot get the JTree to track size with the JFrame.  When
        // the user resizes the frame, the JTree stays put.  I had to
        // manually set the sizes to get things to fit.  Now disallow
        // resizing, else it looks pretty stupid.
        setResizable(false);

        showPanel();

        // Restore the L&F
        setLookAndFeel(defaultLookAndFeel);

    }


    public void showPanel() {
        setVisible(true);
    }

    public void unShowPanel() {
        setVisible(false);
    }


    // Create the protocol node, then subnodes named by directory, then all of
    // the protocol apptypes of protocols in that subnode as another layer
    // of sub nodes under protocol, then all protocols as leaves under that.
    //
    // Create a node for each types (currently tools and rights), then create
    // leaves for each tool or right.
    // Save all of the nodes so that we can set the check boxes on the
    // appropriate ones.  That does not seem to work from here, but works
    // later.

    public void createTreeNodes() {
        int size;
        HashMap typeNodes = new HashMap();
        HashMap dirSubNodes = new HashMap();
        HashMap pSubNodesList = new HashMap();
        HashMap pSubNodes;
        HashMap tabLists = new HashMap();
 
        // Create a protocol node first so that it always show up at
        // the top of the tree.
        DefaultMutableTreeNode node;
        node = new DefaultMutableTreeNode(vnmr.util.Util.getLabel("_admin_protocol"));
        // Save the type nodes in a HashMap
        typeNodes.put("protocol", node);
        // Add the type nodes to the top level
        rootNode.add(node);

       // Now, this next section is to get the tabs in the order they
       // are given in ExperimentSelector.xml.  
       size = rightsList.size();
       for(int i=0; i < size; i++) {
           Right right = rightsList.getRight(i);
           String type = right.getType();
           if(type.equals("protocol")) {
               String dirLabel = right.getDirLabel();
               String protocolName = right.getName();

               String tabLabel = getTabFromProtocolList(protocolName);
               // If this protocol is not in the ExperimentSelector.xml file
               // the tabLabel returned will be "Misc".
               // I don't know the best thing to do, but Krish has complained that
               // this tree has more protocols than the ExperimentSelector itself.
               // Thus,I will try just eliminating protocols with no Tab specified.
               if(tabLabel.equals("Misc"))
                   continue;

               // If we don't have this dirLabel sub node yet, add it
               if(!dirSubNodes.containsKey(dirLabel)) {
                   DefaultMutableTreeNode subNode;
                   subNode = new DefaultMutableTreeNode(dirLabel);
                   dirSubNodes.put(dirLabel, subNode);

                   // Add this dirLabel subnode to the protocol type node
                   DefaultMutableTreeNode parent;
                   parent = (DefaultMutableTreeNode)typeNodes.get(type);
                   parent.add(subNode);
               }
               // Make an apptype list entry for this dirLabel if necessary
               if(!tabLists.containsKey(dirLabel))
                   tabLists.put(dirLabel, new ArrayList());
               
               // Get the apptype list for this dirLabel
               ArrayList tabList = (ArrayList)tabLists.get(dirLabel);
               // See if this list already contains this tab
               // and add if necessary
               if(!tabList.contains(tabLabel))
                   tabList.add(tabLabel);
           }
       }

       // Now we have a HashMap (key = dirLabel) of ArrayLists where the 
       // ArrayLists are the tabLabels for each dirLabel. 


        // The following will take care of protocols not already taken care of.
        // Go through the rightsList and get all 'type's to use as the
        // next level nodes.
        size = rightsList.size();
        for(int i=0; i < size; i++) {
            Right right = rightsList.getRight(i);
            String type = right.getType();
            // If this type is not in the list yet, add it and create a node
            if(!typeNodes.containsKey(type)) {
                node = new DefaultMutableTreeNode(type);
                // Save the type nodes in a HashMap
                typeNodes.put(type, node);
                // Add the type nodes to the top level if adminPanel is true
                // We don't want to allow an operator to limit his own rights,
                // just protocols.
                if(adminPanel)
                    rootNode.add(node);
            }

            // There is most likely a protocol type, so we will need to
            // create the subtypes for protocol based on the apptypeLabel
            // Catch all protocols and then make a list of subtypes and nodes
            // There could be one level higher than the apptypeLabel
            // and that is the dirLabel.  That is, the directory where each
            // protocol comes from will be used as a node and below that
            // will be the apptypeLabel level.
            if(type.equals("protocol")) {
                String tabLabel = right.getTabLabel();
                String dirLabel = right.getDirLabel();
                if(!adminPanel && tabLabel.equals("Misc"))
                    continue;

                // If we don't have this dirLabel sub node yet, add it
                if(!dirSubNodes.containsKey(dirLabel)) {
                    DefaultMutableTreeNode subNode;
                    subNode = new DefaultMutableTreeNode(dirLabel);
                    dirSubNodes.put(dirLabel, subNode);

                    // Add this dirLabel subnode to the protocol type node
                    DefaultMutableTreeNode parent;
                    parent = (DefaultMutableTreeNode)typeNodes.get(type);
                    parent.add(subNode);
                }
                // If we don't have this psubNode in our list create one
                if(!pSubNodesList.containsKey(dirLabel)) {
                    HashMap pSubNode = new HashMap();
                    pSubNodesList.put(dirLabel, pSubNode);
                }
                pSubNodes = (HashMap) pSubNodesList.get(dirLabel);
                if(!pSubNodes.containsKey(tabLabel)) {
                    DefaultMutableTreeNode subNode;
                    subNode = new DefaultMutableTreeNode(tabLabel);
                    pSubNodes.put(tabLabel, subNode);

                    // Add this subnode to the dirLabel node
                    DefaultMutableTreeNode parent;
                    parent = (DefaultMutableTreeNode)dirSubNodes.get(dirLabel);
                    parent.add(subNode);
                }
            }
        }
        
//        // Sort the protocols in the order of ProtocolLabels.xml
//        rightsList.sortProtocols();
//        
        
        // Go through the rightsList and add each item to the correct type
        int listsize = rightsList.size();
        for(int i=0; i < listsize; i++) {
            Right right = (Right) rightsList.getRight(i);
            String type = right.getType();
            String dirLabel = right.getDirLabel();
            String protocolName = right.getName();

            String tabLabel = getTabFromProtocolList(protocolName);
            if(!adminPanel && tabLabel.equals("Misc"))
                continue;

            // Create the node for this item
            DefaultMutableTreeNode leaf;
            leaf = new DefaultMutableTreeNode(right);

            // Get the parent node for this type
            DefaultMutableTreeNode parent;
            if(type.equals("protocol")) {
                // Get the pSubNodes hash map for this dirLabel
                pSubNodes = (HashMap) pSubNodesList.get(dirLabel);
                parent = (DefaultMutableTreeNode)pSubNodes.get(tabLabel);
                // Add this node to the proper parent node
                parent.add(leaf);

                // Save leaves
                leaves.put(right.getName(), leaf);

                // If approved, we need to set the checkbox for the CheckTree
                // to true.
//                String approve = right.getApprove();
            }
            else {
                parent = (DefaultMutableTreeNode)typeNodes.get(type);
                // Add this node to the proper parent node
                parent.add(leaf);

                // Save leaves
                leaves.put(right.getName(), leaf);
            }


        }

        // This is required to get the new nodes to show up
        TreePath rootpath = new
             TreePath(treeModel.getPathToRoot(rootNode));
        tree.expandPath(rootpath);

    }

    // Go through each leaf and set its check box if it is approved
    public void setCheckBoxes() {

        // leaves contains the list of all leaf nodes
        Set set = leaves.entrySet();
        Iterator iter = set.iterator();
        ArrayList alPaths = new ArrayList();

        while(iter.hasNext()) {
            Map.Entry entry = (Map.Entry)iter.next();
            DefaultMutableTreeNode leaf =
                         (DefaultMutableTreeNode) entry.getValue();

            // The UserObject for the nodes are the rights
            Right right = (Right) leaf.getUserObject();
            String approve = right.getApprove();

            if(approve.equals("true")) {
                TreePath path = new
                    TreePath(treeModel.getPathToRoot(leaf));
                // Save the array of paths which are approved
                alPaths.add(path);
            }
        }
        TreeSelectionModel selModel =
            checkTreeManager.getSelectionModel();

        // Set all of the paths saved as selected
        // It needs them as an array so convert the ArrayList to an array
        TreePath [] paths = (TreePath []) alPaths.toArray(new TreePath[1]);
        selModel.addSelectionPaths(paths);
    }

    // Start with stock profile
    public void newProfile() {
        // Create the initial list from the protocol files in the protocol dir
        // **** need correct directories
        fillRightsList();

        updateTree();
    }


    public void updateTree() {
        // Create the top root node
        rootNode = new DefaultMutableTreeNode(vnmr.util.Util.getLabel("_admin_User_Rights"));

        treeModel = new DefaultTreeModel(rootNode);

        // Create the JTree
        tree = new JTree(rootNode);

        tree.setModel(treeModel);


        // Turn off the Icons.  This has to be before creating CheckTreeManager
        DefaultTreeCellRenderer renderer = new DefaultTreeCellRenderer();
        renderer.setLeafIcon(null);
        renderer.setOpenIcon(null);
        renderer.setClosedIcon(null);
        tree.setCellRenderer(renderer);

        // Don't show the root node
        tree.setRootVisible(false);

        // Show the expand/collapse control for the top level nodes
        tree.setShowsRootHandles(true);

        // Make it a check box tree
        checkTreeManager = new CheckTreeManager(tree);


        // Create the nodes for the JTree. This has to be before creating JTree
        // This is where all the protocols are put into the tree.
        createTreeNodes();


        // Set appropriate check boxes to 'on'
        try {
            setCheckBoxes();
        }
        catch (Exception e) {
            Messages.writeStackTrace(e, 
                        "Problem setting CheckBoxes Profile Editor");
        }

        // Create a new panel if needed or remove the scrollPane if it exists
        if(upanel == null) {
            upanel = new JPanel();
            // Put the panel into the Frame
            getContentPane().add(BorderLayout.NORTH, upanel);
        }
        else
            upanel.removeAll();

        // Create a new ScrollPane with this new tree
        scrollPane = new JScrollPane(tree);

        // I cannot get this scrollpane or the JTree to track in size
        // with the JFrame.  I had to set the sizes manually to get it
        // to look right.
        int width=Integer.parseInt(vnmr.util.Util.getOption("_admin_UserRightPanel_Frame_Width","600"));
        scrollPane.setPreferredSize(new Dimension(width-20, 528));

        upanel.add(scrollPane);
        showPanel();

    }


    // Get the on/off status of all nodes and set them into the rights
    // in the rightsList.  When a upper node is fully selected, the lower
    // nodes under it do not show selected.  We have to just know that
    // is the case.
    public void updateFromCheckBoxes() {
        final int TYPE = 1;
        final int NAME = 2;
        final int DIRLABEL = 2;
        final int APPTYPELABEL = 3;
        final int PROTOCOLLABEL = 4;
        TreePath[] paths;
        TreePath tp;
        int count;

        // Before getting the items checked in the tree, we need to turn OFF
        // every item in the rights list.  Then we will turn back on the items
        // which are selected.
        int size = rightsList.size();
        for(int i=0; i < size; i++) {
            Right right = rightsList.getRight(i);
            right.setApprove("false");
        }
        
        // Now get the items with selected checkboxes
        paths = checkTreeManager.getSelectionModel().getSelectionPaths();

        // This is an array of TreePath objects.
        // Each one describes a branch or leaf that is selected.
        // If all items below a branch are selected, the only object we
        // will get here is the branch itself, and no sub branches or leaves
        // will show up.  Thus, we are responsible for finding all items
        // below this top selected level.

        for(int i=0; i < paths.length; i++) {
            String dirLabel="";
            String tabLabel="";
            String protocolLabel="";
            String name="";
            String type;
            String rootType="";

            // Get the TreePath for this selected node
            tp = paths[i];

            // Get the number of elements in this TreePath
            count = tp.getPathCount();

            if(count == 1) {
                type = "root";
                rootType = ((DefaultMutableTreeNode)tp.getLastPathComponent()).toString();
            }
            else
                // Protocols are different than rights and tools
                type = ((DefaultMutableTreeNode)tp.getPathComponent(TYPE)).toString();
            if(type.equals("protocol")) {
                // element 0 = root node
                // element 1 = type
                // element 2 = dirLabel
                // element 3 = apptypeLabel
                // element 4 = protocolLabel
                // Thus, if there is a count of 5, we are setting a single
                //    protocol in this protocol/dirLabel/apptypeLabel branch
                // If there is a count of 4, we need to change all rights
                //    in the branch protocol/dirLabel/apptypeLabel
                // If there is a count of 3, we need to change all rights
                //    in the branch protocol/dirLabel
                // If there is a count of 2, we need to change all rights
                //    in the branch 'protocol'
                // We need to go through all of the rights and see which ones
                // are effected by this tree information.
                if(count >= 3) {
                    dirLabel = ((DefaultMutableTreeNode)tp.getPathComponent(DIRLABEL)).toString();
                }
                if(count >= 4) {
                    tabLabel = ((DefaultMutableTreeNode)tp.getPathComponent(APPTYPELABEL)).toString();
                }
                if(count >= 5) {
                    protocolLabel =
                              ((DefaultMutableTreeNode)tp.getPathComponent(PROTOCOLLABEL)).toString();
                }
            }
            else if(!type.equals("root")) {
                // Fill non protocol items
                if(count >= 3)
                    name = ((DefaultMutableTreeNode)tp.getPathComponent(NAME)).toString();
            }
            // We have the info on this branch, now loop through the
            // rights and see which ones are effected and set approve=true
            int listsize = rightsList.size();
            for(int k=0; k < listsize; k++) {
                Right right = rightsList.getRight(k);
                // If type is root, then just set them all for this right.type
                if(type.equals("root")) {
                    right.setApprove("true");
                    continue;
                }

                String rtType = right.getType();
                if(rtType.equals("protocol") && type.equals("protocol")) {
                    if(count == 2 || count == 1) {
                        // Set for the entire protocol branch, regardless
                        // of any specific information
                        right.setApprove("true");
                    }
                    else if(count == 3) {
                        // Set true for all protocols of this dirLabel
                        if(right.getDirLabel().equals(dirLabel))
                            right.setApprove("true");
                    }
                    else if(count == 4) {
                        // Set true for all protocols of this dirLabel
                        //    and this tabLabel
                        if(right.getDirLabel().equals(dirLabel) &&
                                right.getTabLabel().equals(tabLabel)){
                            right.setApprove("true");
                        }
                    }
                    else if(count == 5) {
                        // Set true for all protocols of this dirLabel,
                        //    this aptypeLabel and this protocolLabel
                        if(right.getDirLabel().equals(dirLabel) &&
                                right.getTabLabel().equals(tabLabel) &&
                                right.getProtocolLabel().equals(protocolLabel)){
                            right.setApprove("true");
                        }
                    }
                }
                // Not protocol
                else  if(!rtType.equals("protocol"))  {
                    // if not a protocol, but count == 1, we still just set
                    // them all
                    if(count == 1)
                        right.setApprove("true");
                    else {
                        // We must not have the root node set, and we must
                        // not be a protocol.  See if type matches,
                        // set approve = true
                        if(count == 2) {
                            if(rtType.equals(type)){
                                right.setApprove("true");
                            }
                        }
                        else if(count == 3) {
                        // We must not have the root node set, and we must
                        // not be a protocol.  See if type and name matches,
                        // set approve = true
                            String rtName = right.getName();
                            if(rtType.equals(type) && rtName.equals(name)){
                                right.setApprove("true");
                            }
                        }
                    }
                }
            }
        }
    }

    /************************************************** <pre>
     * Summary: Listener for all buttons.
     *
     *
     </pre> **************************************************/
    public void actionPerformed(ActionEvent e) {

        String cmd = e.getActionCommand();
        // Cancel
        if(cmd.equals(strClose)) {
            unShowPanel();
        }
        else if(cmd.equals(strOpen)) {
            if(openSavePopup == null) {
                openSavePopup = new JFileChooser();
                // Remove the ability to change directories upwards
                removeDirUp(openSavePopup);

                // Setting the SelectionMode to Files, still shows files and
                // directories in the chooser.  It allows browsing, but only
                // comes out of the showOpenDialog() if a file is selected.
                // To stop the ability to browse, I set to both files and dirs
                // Then we come out of showOpenDialog for either and I can
                // trap for dir and error out.
                openSavePopup.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);


                // Turn off the All filter
                openSavePopup.setAcceptAllFileFilterUsed(false);
                openSavePopup.setFileFilter(new xmlFileFilter());
            }
            
            // Always force the directory to the desired directory
            // The area to save these profiles is 
            //       /vnmr/adm/users/userProfiles
            String path = FileUtil.openPath("SYSTEM/USRS/userProfiles/");
            // if path is null, the file does not exists yet, make one
            if(path == null) {
                path = FileUtil.savePath("SYSTEM/USRS/userProfiles/");
                File dir = new File(path);
                dir.mkdir();
            }

            // On Windows, for some reason using UNFile here fails.
            // Since the path is already OS specific coming from openPath()
            // just using File here works.
            File dir = new File(path);

            openSavePopup.setCurrentDirectory(dir);

            int result = openSavePopup.showOpenDialog(this);
            if(result == JFileChooser.APPROVE_OPTION) {
                File file = openSavePopup.getSelectedFile();
                if(file.isDirectory())
                    Messages.postError("Cannot open " + file.getPath()
                                       + " it is a directory.");
                else if(!file.canRead())
                    Messages.postError("Cannot open " + file.getPath()
                                       + " Permission denied.");
                else {
                    // Start will all protocols available
                    fillRightsList();

                    // Open this profile and set the check boxes
                    rightsList.openProfile(file.getAbsolutePath(), true, false);

                    // Create a new update tree
                    updateTree();

                    // Put the filename in the Title
                     setTitle(vnmr.util.Util.getLabel("_admin_Edit_User_Rights_Profile")+": "
                            + file.getName());

                    // Save the filepath for use with the Save button
                    openedFile = file.getAbsolutePath();
                }
            }
            else if(result == JFileChooser.ERROR_OPTION) {
                System.out.println("Problem Opening File");
            }
        }
        // Save a profile
        else if(cmd.equals(strSave_As) || cmd.equals(strSave)) {
            // Get the updated checkbox information into the rightsList rights
            updateFromCheckBoxes();

            if(cmd.equals("Save As") || (openedFile == null && adminPanel)) {
                if(openSavePopup == null) {
                    openSavePopup = new JFileChooser();
                    // Remove the ability to change directories upwards
                    removeDirUp(openSavePopup);

                    openSavePopup.setFileSelectionMode(
                                         JFileChooser.FILES_AND_DIRECTORIES);

                    // Turn off the All filter
                    openSavePopup.setAcceptAllFileFilterUsed(false);
                    openSavePopup.setFileFilter(new xmlFileFilter());
                }
                // The area to save these profiles is 
                //     /vnmr/adm/users/userProfiles
                String path = FileUtil.savePath("SYSTEM/USRS/userProfiles/");

                File dir = new File(path);
                // If the rights directory does not exist, make one
                if(!dir.exists()) 
                    dir.mkdir();

                openSavePopup.setCurrentDirectory(dir);

                int result = openSavePopup.showSaveDialog(this);
                if(result == JFileChooser.APPROVE_OPTION) {
                    File file = openSavePopup.getSelectedFile();

                    openedFile = file.getAbsolutePath();

                    // If it does not end in .xml, add it
                    if(!openedFile.endsWith(".xml"))
                        openedFile = openedFile.concat(".xml");

                    // Do not allow them to overwrite the system installed files.
                    // We need to be able to update these at install time.  They
                    // just need to use a different name for any profiles they want.

                    // Add a debug flag that will allow us to create new files internally
                    if(!DebugOutput.isSetFor("AllowAllProfileNames")) {
                        String dirPath = FileUtil.savePath("SYSTEM/USRS/userProfiles/");
                        String allPath = dirPath + "All";
                        String basicPath = dirPath + "Basic";
                        
                        if(openedFile.startsWith(allPath) || openedFile.startsWith(basicPath)) {
                            Messages.postError("Users cannot save files starting with \"All\" nor"
                                               + "\n    \"Basic\".  Choose a different name");
                            return;
                        }
                    }

                    // Write the Profile
                    rightsList.writeFiles(openedFile);

                    // Put the filename in the Title
                    setTitle(vnmr.util.Util.getLabel("_admin_Edit_User_Rights_Profile")+": "
                            + file.getName());
                    Messages.postInfo(openedFile + " Saved");
                }
                else if(result == JFileChooser.ERROR_OPTION) {
                    System.out.println("Problem Saving File");
                }
            }
            else {
                // Must be Save and openedFile is set, or !adminPanel
                if(adminPanel) {
                    // Do not allow them to overwrite the system installed files.
                    // We need to be able to update these at install time.  They
                    // just need to use a different name for any profiles they want.

                    // Add a debug flag that will allow us to create new files internally
                    if(!DebugOutput.isSetFor("AllowAllProfileNames")) {
                        String dirPath = FileUtil.savePath("SYSTEM/USRS/userProfiles/");
                        String allPath = dirPath + "All";
                        String basicPath = dirPath + "Basic";
                        
                        if(openedFile.startsWith(allPath) || openedFile.startsWith(basicPath)) {
                            Messages.postError("Users cannot save files starting with \"All\" nor"
                                               + "\n    \"Basic\".  Choose a different name");
                            return;
                        }
                    }

                    // Write the Profile
                    rightsList.writeFiles(openedFile);
                    Messages.postInfo(openedFile + " Saved");

                }
                // Must be operator panel saving to operator's file
                else {
                    rightsList.writePersistence();
                    unShowPanel();
                 }
            }

        }
        else if(cmd.equals(strDelete)) {
            if(openSavePopup == null) {
                openSavePopup = new JFileChooser();
                // Remove the ability to change directories upwards
                removeDirUp(openSavePopup);

                openSavePopup.setFileSelectionMode(
                    JFileChooser.FILES_AND_DIRECTORIES);

                // Turn off the All filter
                openSavePopup.setAcceptAllFileFilterUsed(false);
                openSavePopup.setFileFilter(new xmlFileFilter());
            }
            // The area to save these profiles is 
            //     /vnmr/adm/users/userProfiles
            String path = FileUtil.openPath("SYSTEM/USRS/userProfiles");
            File dir = new File(path);
            // If the rights directory does not exist, make one
            if(!dir.exists()) 
                dir.mkdir();

            openSavePopup.setCurrentDirectory(dir);

            // Set the OK button to 'Delete'
            int result = openSavePopup.showDialog(this, vnmr.util.Util.getLabel("blDelete"));
            if(result == JFileChooser.APPROVE_OPTION) {
                File file = openSavePopup.getSelectedFile();
                // Remove this file
                file.delete();
            }

        }
        else if(cmd.equals(strNew)) {
            // Create a new stock profile, thus discarding any previous
            // changes or any profiles read in.
            newProfile();

            // Set opened file to null as a flag that nothing is opened.
            openedFile = null;

            // Reset the title showing nothing opened
            setTitle(vnmr.util.Util.getLabel("_admin_Edit_User_Rights_Profile")
                    +": "+vnmr.util.Util.getLabel("blNew"));
        }

    }

    
    public void fillRightsList() {
        // This is used for the admin tool and needs to use all possible
        // appdirs, not just the ones active for this user.
        HashMap<String, String> allDirList = new HashMap<String, String>();
        allDirList = Util.getAllPossibleAppProtocolDirsHash();
        rightsList = new RightsList(allDirList);

    }


    public SAXParser getSAXParser() {
        SAXParser parser = null;
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            spf.setValidating(false);  // set to true if we get DOCTYPE
            spf.setNamespaceAware(false); // set to true with referencing
            parser = spf.newSAXParser();
        } catch (ParserConfigurationException pce) {
            System.out.println(new StringBuffer().
                               append("The underlying parser does not support ").
                               append("the requested feature(s).").
                               toString());
        } catch (FactoryConfigurationError fce) {
            System.out.println("Error occurred obtaining SAX Parser Factory.");
        } catch (Exception e) {
            Messages.writeStackTrace(e);
        }
        return (parser);
    }


    protected void setLookAndFeel(String lookandfeel)
    {
        // Save the look and feel for restoring
        if(defaultLookAndFeel == null)
            defaultLookAndFeel = UIManager.getLookAndFeel().getID();

        /*************
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
        ***********/
    }

    /** Remove the up arrow icon and the directory list and the new folder item
     *  We do not want to allow the user to get out of this directory
     *  There is no normal java class way to do this that I know of, so
     *  we will dig our way to get the new folder object.  
     *  The panels and buttons for the JFileChooser are created in
     *  the Java file MetalFileChooserUI.java .  Based on looking at
     *  that file and the order or creation I found that:
     *  Item 0 of the JFileChooser is topPanel containing 
     *  Item 0 of topPanel is topButtonPanel 
     *  Item 0 of topButtonPanel is upFolderButton
     *  Item 2 of topButtonPanel is homeFolderButton
     *  Item 4 of topButtonPanel is newFolderButton
     */

    public void removeDirUp(JFileChooser chooser) {
        // We need to disable upFolderButton.  It is the 0th item in 
        // topButtonPanel.
        JPanel topPanel = (JPanel) chooser.getComponent(0);
        JPanel topButtonPanel = (JPanel) topPanel.getComponent(0);
        topButtonPanel.remove(4);
        topButtonPanel.remove(2);
        topButtonPanel.remove(0);
        // We also need to remove the menu list of parent 
        // directories.  the label and the list are #1 and #2
        // items in topPanel. 
        topPanel.remove(2);
        // Get rid of the home icon also
        topPanel.remove(1);

    }
    
    public void destroyPanel() {
        try {
            dispose();
            finalize();
        }
        catch(Throwable e) {
            
        }
    }

    // Return the first "tab" entry for a protocol of the input name.
    private String getTabFromProtocolList(String protocolName) {

        ArrayList<ExpSelEntry> protocolList = ExpSelector.getProtocolList();

        // Go through the list of ExpSelEntries in protocolList looking for the
        // first instance of protocolName.  Return the tab for that entry.
        for (int i = 0; i < protocolList.size(); i++) {
            ExpSelEntry protocol = protocolList.get(i);
            if(protocol.getName().equals(protocolName)) {
                String tab = protocol.getTab();
                return tab;
            }
        }

        // If not found, use Misc
        return new String("Misc");
    }

 


    public class xmlFileFilter extends javax.swing.filechooser.FileFilter
    {
        public boolean accept(File path) {
            if(path.isDirectory() ||
                        (path.isFile() && path.getName().endsWith(".xml")))
                return true;
            else
                return false;
        }

        public String getDescription() {
            return "XML Files";
        }

    }

}
