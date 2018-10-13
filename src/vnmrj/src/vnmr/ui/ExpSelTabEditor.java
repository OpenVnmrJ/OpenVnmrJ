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
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.Transferable;
import java.awt.datatransfer.UnsupportedFlavorException;
import java.awt.event.*;
import java.io.*;

import javax.swing.*;
import javax.swing.tree.*;
import java.util.*;

import vnmr.util.FileUtil;
import vnmr.util.UNFile;
import vnmr.util.Util;
import vnmr.util.Messages;
import vnmr.util.ModalDialog;

/********************************************************** <pre>
 * Summary: Create a panel for rearranging the order of the tabs in
 *          the Experiment Selector
 *
 * This is meant to mimic a future GUI for editing the ES.  This
 * early version is ONLY for rearranging the order of the tabs.
 * Since it is the forerunner, it will be designed using a tree
 * structure like the later GUI is expected to have.

 </pre> **********************************************************/

public class ExpSelTabEditor extends ModalDialog implements ActionListener
{

    private static DataFlavor dataFlavor = null;
    private JScrollPane scrollPane;
    private JTree tree;
    private JPanel upanel = null;

    private DefaultMutableTreeNode rootNode;
    private DefaultTreeModel treeModel;
    ArrayList<String> tabNameList;
    String strOk = vnmr.util.Util.getLabel("blOk");
    String strCancel = vnmr.util.Util.getLabel("blCancel");
//    private ESDropTargetListener dropListener;

   
    public ExpSelTabEditor() {

        super(vnmr.util.Util.getLabel("Edit Tab Order"));

        try {
            dataFlavor = new DataFlavor(DataFlavor.javaJVMLocalObjectMimeType+";class="+DefaultMutableTreeNode.class.getName());
        } 
        catch (ClassNotFoundException e) {
            Messages.writeStackTrace(e, "Problem Editing Tab Order");
            return;
        }
        
        // Create the top root node
        rootNode = new DefaultMutableTreeNode(vnmr.util.Util.getLabel("Edit Tab Order"));
        treeModel = new DefaultTreeModel(rootNode);

        // Create the JTree
        tree = new JTree(rootNode);
        tree.setModel(treeModel);

        // Don't show the root node
        tree.setRootVisible(false);
        // Show the expand/collapse control for the top level nodes
        tree.setShowsRootHandles(true);
        
        tree.setDragEnabled(true);

        // Only allow inserting.  That is, don't allow replacing the
        // item at the drop site with a new item.
        tree.setDropMode(DropMode.INSERT);
        TransferHandler transferHandler = new ESTransferHandler();
        tree.setTransferHandler(transferHandler);
        
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
        
        // Add listeners to the buttons in ModalDialog
        okButton.addActionListener(this);
        cancelButton.addActionListener(this);

        pack();
        setLocationRelativeTo(getParent());

    }

 
    // For this beginning tab only rearranger, just create one level
    // of nodes.  The list is contained in ExpSelector.tabNameList
    // as well in in the file ~/vnmrsys/menulib/tabList.
    public void createTreeNodes() {
        tabNameList = ExpSelector.getTabNameList();
        fillTree();

    }
    
    // Fill the tree with tabs from tabNameList
    public void fillTree () {
        
        if(tabNameList == null || tabNameList.size() == 0) {
            Messages.postError("Tab List is Empty");
            return;
        }
        // Loop through the tab list and create nodes
        for(int i=0; i < tabNameList.size(); i++) {
            String tabName = tabNameList.get(i);
            DefaultMutableTreeNode node;
            node = new DefaultMutableTreeNode(tabName);
            rootNode.add(node);

        }
        // This is required to get the new nodes to show up
        TreePath rootpath = new TreePath(treeModel.getPathToRoot(rootNode));
        tree.expandPath(rootpath);
        
    }
    
    
    // Respond to button clicks.  If Ok, write tab order information
    // to a file.
    // 3/16/10, I was writing this to the ExperimentSelector_op.xml file
    // however, as of now, that file is being removed and recreated
    // by the updateExpSelector macro.  So, I am just changing the file
    // name designation to tabOrder_op.xml.  I am going to leave the code
    // that retains the rest of the file in place in case we change back
    // to the original file or something.  Right now, there will be nothing
    // in tabOrder_op.xml except the tab order, so the code will not be doing
    // much.
    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();

        if (cmd.equalsIgnoreCase(strOk)) {
            RandomAccessFile esXmlFile=null;
            RandomAccessFile tmpFile=null;
            unShowPanel();

            // Update tabNameList from the tree
            updateTabNameList();

            // We need to get the tab information from the tree and write
            // it to tabOrder_op.xml file
            
            // This gets the tabOrder line to use below
            String lineToWrite = createTabOrderLine();
            
            String curOperator = Util.getCurrOperatorName();
            String path = FileUtil.savePath("USER/INTERFACE/tabOrder_"
                                      + curOperator + ".xml");
            if(path == null) {
                Messages.postError("Problem opening " + path);
                return;
            }
            // Make a new file for output with 'new' appended to the name
            String tmppath = path + ".new";

            UNFile file = new UNFile(path);
            UNFile outFile = new UNFile(tmppath);
            if(outFile.exists()) {
                outFile.delete();
            }
            if (!file.exists()) {
                // The file does not exist, create it and write the opening
                // line, the tabOrder line and closing lines to it.
                try {
                    esXmlFile = new RandomAccessFile(file, "rw");
                    esXmlFile.writeBytes("<ProtocolList userOnly=\"false\" tooltip=\"label\" iconForCategory=\"false\"> \n");
                    // Write the tabOrder line
                    esXmlFile.writeBytes(lineToWrite);
                    esXmlFile.writeBytes("\n</ProtocolList>\n");
                    esXmlFile.close();
                }
                catch (Exception ex) 
                {
                    Messages.writeStackTrace(ex, "Problem creating " + path);
                    return;
                }
            }
            // The file already exists
            else {
                try {
                    String line;
                    // Open an output file writer
                    tmpFile  = new RandomAccessFile(outFile, "rw");
                    esXmlFile = new RandomAccessFile(file, "r");
                    // See if the tabOrder entry exists, if so, remove it
                    // from "<tabOrder" to "/>", That is, don't write this part
                    // back out to the new file.
                    while((line = esXmlFile.readLine()) != null) {
                        if(line.contains("tabOrder")) {
                            // We don't want to write out any line until /> is found.
                            if(line.contains("/>"))
                                // We hit the end, continue with the rest of the file
                                continue;
                            // Keep looking for the end of this entry
                            while((line = esXmlFile.readLine()) != null) {
                                if(line.contains("/>"))
                                    break;
                            }
                        }
                        else {
                            // Not the tabOrder entry, write this line out to
                            // the new replacement file
                            tmpFile.writeBytes(line + "\n");
                        }
                    }
                    // We have a new file with the tabOrder removed
                    tmpFile.close();
                    esXmlFile.close();
                    // remove the previous file so we start over
                    file.delete();
                    // Start it over
                    file = new UNFile(path);

                    // Now read the tmpFile, write the tabOrder line in the
                    // right place, by writing back into the esXmlFile.
                    tmpFile  = new RandomAccessFile(outFile, "r");
                    esXmlFile = new RandomAccessFile(file, "rw");
                    while((line = tmpFile.readLine()) != null) {
                        if(line.contains("<ProtocolList")) {
                            // write it
                            esXmlFile.writeBytes(line + "\n");

                            if(line.contains(">")) {
                                // ProtocolList was all one line and we have
                                // already written it.  Write the tabOrder line 
                                // and continue with the rest of the file
                                esXmlFile.writeBytes(lineToWrite);
                                continue;
                            } 
                            // The ProtocolList line is multiple lines
                            else {
                                // Keep looking for the end of this entry
                                while ((line = tmpFile.readLine()) != null) {
                                    if (line.contains(">")) {
                                        // We have the end of the ProtocolList line
                                        // write it and write the tabOrder line below this.
                                        esXmlFile.writeBytes(line + "\n");
                                        esXmlFile.writeBytes(lineToWrite);
                                        break;
                                    }
                                    else {
                                        // Not the end yet, write the line
                                        esXmlFile.writeBytes(line + "\n");
                                    }
                                }
                            }
                        }
                        else if(line.contains("/ProtocolList")) {
                            // This must be the last legitimate line, write  it
                            // out and break out of the loop.
                            esXmlFile.writeBytes(line + "\n");
                            break;
                        }
                        // Just write out the line
                        else {
                            esXmlFile.writeBytes(line + "\n");
                        }
                    }
                    tmpFile.close();
                    esXmlFile.close();

                    // Cleanup
                    if(outFile != null && outFile.exists())
                        outFile.delete();
                }
                catch (Exception ex) 
                {
                    Messages.writeStackTrace(ex, "Problem writing to " + path);
                    return;
                }
            }
            // Update the Experiment Selector immediately
            ActionEvent aevt = new ActionEvent(this, 1, "force");
            if(ExpSelector.updateCards != null)
                ExpSelector.updateCards.actionPerformed(aevt);

        }
        else if (cmd.equalsIgnoreCase(strCancel)) {
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
    
    // Create the tabOrder line for the tabOrder_op.xml file
    // Use tab list currently in tabNameList as a comma separated list.
    // The line should look something like:
    //    <tabOrder list="Common, Std1D, My Favorites" </tabOrder>
    private String createTabOrderLine() {
        String line = "\t<tabOrder list=\"";
        for(int i=0; i < tabNameList.size(); i++) {
            if(i != 0)
                line = line + ", ";
            // add a newline every 5 entries
            if(i != 0 && i % 5 == 0)
                line = line + "\n\t\t";

            line = line + tabNameList.get(i);
        }
        line = line + "\" />\n";
        
        return line;
    }

    // Update the ArrayList, tabNameList from the tree
    private void updateTabNameList() {
        // Start with a new empty list
        tabNameList = new ArrayList<String>();
        
        int num = rootNode.getChildCount();
        // Fill the list with tab names from the tree
        for(int i=0; i < num; i++) {
            DefaultMutableTreeNode child = (DefaultMutableTreeNode)rootNode.getChildAt(i);
            String name = (String)child.getUserObject();
            tabNameList.add(name);
        }
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
            DefaultMutableTreeNode node;
            
            if (!canImport(supp)) {
                return false;
            }

            // Fetch the Transferable and its data
            Transferable t = supp.getTransferable();
            try {
                Object data = t.getTransferData(dataFlavor);
                // data is a JTree
                TreePath path = ((JTree)data).getSelectionPath();
                int count = path.getPathCount();
                // For this tab reordering tree, the tree should always be
                // 2 deep where level one is the root.
                if (count != 2) {
                    Messages.postError("Problem with Experiment Selector Tab Editor, Aborting.");
                    unShowPanel();
                    return false;
                }
                Object[] obj = path.getPath();
                
                // Here is the node we are dragging
                node = (DefaultMutableTreeNode)obj[1];
                
                // Get the drop location as an index down the tree.
                // With the current one level tree, index = 0 means the drop
                // was above the first item and index = max means it is after
                // the last item.
                JTree.DropLocation dloc= ((JTree)data).getDropLocation();
                int indexTo = dloc.getChildIndex();
                
                // What is the index of the node being moved/copied?
                int indexFrom = rootNode.getIndex(node);

                // Is this a copy or move
                int mode = supp.getUserDropAction();
                if (mode == TransferHandler.COPY){
                    // Cntrl drag ONLY works is you start the drag and THEN
                    // press control.  If you press cntrl and then click
                    // and drag, it does not allow dragging at all.  I see
                    // in Java's BasicTreeUI.mousePressedDND() around line 3483 it says
                    // that if Cntrl, don't do anything yet.  Unfortunately,
                    // it never does anything.  I believe this is a
                    // JAVA BUG.  Specify in the manual that they need to start
                    // the drag first.
                    
                    // Make a copy of the node and insert it so that we
                    // keep the original node in place also.

                    // For Tab Ordering, we do not want to copy anyway since
                    // there cannot be duplicate tabs.  We may need this when
                    // we expand to D&D protocols, menus etc.
                    DefaultMutableTreeNode newNode = (DefaultMutableTreeNode)node.clone();
                    treeModel.insertNodeInto(newNode, rootNode, indexTo);             

                    
                }
                else if (mode == TransferHandler.MOVE) {
                    // Note: The removal and insertion are done by calling
                    // methods of the DefaultTreeModel, NOT the rootNode
                    
                    // Remove the node from its current location
                    treeModel.removeNodeFromParent(node);
                    
                    // If we are moving the node up, then we just insert
                    // it at indexTo.  If however, we are moving it down,
                    // the insert location will have been changed (-1) due to
                    // the removal of the node.
                    if (indexTo > indexFrom)
                        indexTo = indexTo -1;
                    treeModel.insertNodeInto(node, rootNode, indexTo);             
                }

            }
            catch(Exception ex) {
                
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

}

