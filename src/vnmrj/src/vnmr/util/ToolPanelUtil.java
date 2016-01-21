/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.LayoutManager;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollBar;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTree;
import javax.swing.event.TreeSelectionEvent;
import javax.swing.event.TreeSelectionListener;
import javax.swing.plaf.basic.BasicSplitPaneDivider;
import javax.swing.plaf.basic.BasicSplitPaneUI;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeModel;
import javax.swing.tree.TreeNode;
import javax.swing.tree.TreePath;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.DefaultHandler;

import vnmr.bo.VObjDef;
import vnmr.bo.VUndoableText;
import vnmr.ui.VnmrjIF;
import vnmr.util.ToolPanelUtil.ToolInfo;

public class ToolPanelUtil extends ModelessDialog implements VObjDef,
        ActionListener {
    private static final long   serialVersionUID = 1L;
    private static final int    XMLTAB           = 1;
    private static final int    LOCATOR          = 2;
    private static final int    LOCATORTAB       = 3;
    private static final int    DIRECTORY        = 4;
    private static final int    UP               = 1;
    private static final int    DOWN             = 2;

    /** The split pane used for display. */
    JSplitPane                  splitPane;
    JPanel                      attrpanel        = new JPanel();
    JScrollPane                 treeScroll       = null;
    JTree                       leftTree;
    JTree                       rightTree;

    DefaultMutableTreeNode      leftRoot;
    DefaultMutableTreeNode      rightRoot;
    DefaultMutableTreeNode      locatorNode;
    DefaultMutableTreeNode      directoryNode;
    String                      directoryPath    = null;
    DefaultMutableTreeNode      selectedNode     = null;
    JTree                       selectedTree     = null;

    Hashtable<String, ToolInfo> xmltabs          = new Hashtable<String, ToolInfo>();
    Hashtable<String, ToolInfo> loctabs          = new Hashtable<String, ToolInfo>();

    TreeSelector                leftTreeSelector;
    TreeSelector                rightTreeSelector;
    VUndoableText               nameEntry;
    VUndoableText               fileEntry;
    VUndoableText               viewEntry;
    JLabel                      fileLabel;

    int                         windowWidth      = 650;
    int                         scrollH          = 30;
    int                         etcH             = 130;
    int                         sbW              = 0;

    ButtonDividerUI             m_objDividerUI;

    JButton                     arrowbtn         = new JButton();
    JButton                     upbtn            = new JButton();
    JButton                     downbtn          = new JButton();
    JButton                     revertbtn        = new JButton("Revert");

    Container                   m_contentPane;
    // reduced list of locator panels
    final static String[]       toolNames        = { "Locator", "Holding",
            "Sq", "Rq", "ExperimentSelector", "ExperimentSelTree", "NotePad",
            "Instructions", "Browser"           };

    public ToolPanelUtil() {
        super("Tool Panel Editor");
        setLocation(800, 200);
        initComponents();
        pack();
    }

    @Override
    public void setVisible(boolean bVis) {
        if (bVis){
            updateLists();
        }
        setRevertBtn();
        super.setVisible(bVis);
    }
    
    protected void initComponents() {
        setVisible(false);
        Dimension sdim = new Dimension(200, 200);
        leftRoot = new DefaultMutableTreeNode("Current Tabs");
        leftTree = new JTree(leftRoot);
        leftTree.setModel(new DefaultTreeModel(leftRoot));
        leftTree.setExpandsSelectedPaths(true);
        leftTree.setRootVisible(true);
        leftTreeSelector = new TreeSelector(leftTree, leftRoot);

        rightRoot = new DefaultMutableTreeNode("Available Tabs");
        rightTree = new JTree(rightRoot);
        rightTree.setModel(new DefaultTreeModel(rightRoot));
        rightTree.setExpandsSelectedPaths(true);
        rightTree.setRootVisible(true);
        rightTreeSelector = new TreeSelector(rightTree, rightRoot);

        leftTree.setPreferredSize(sdim);
        rightTree.setPreferredSize(sdim);

        splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true, leftTree,
                rightTree);

        treeScroll = new JScrollPane(splitPane);
        treeScroll.setPreferredSize(new Dimension(windowWidth - 2, 300));

        m_contentPane = getContentPane();

        setAbandonEnabled(true);
        setCloseEnabled(true);

        abandonButton.setActionCommand("cancel");
        abandonButton.addActionListener(this);
        closeButton.setActionCommand("apply");
        closeButton.addActionListener(this);
        closeButton.setToolTipText("Apply changes");

        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);

        closeButton.setText(vnmr.util.Util.getLabel("Apply"));
        closeButton.setEnabled(false);
        abandonButton.setText(vnmr.util.Util.getLabel("blCancel"));
        abandonButton.setToolTipText("Cancel without saving");

        historyButton.setVisible(false);
        undoButton.setVisible(false);
        
        buttonPane.add(revertbtn);
        revertbtn.addActionListener(this);
        revertbtn.setActionCommand("revert");
        revertbtn.setToolTipText("Revert to system defaults");
        
        arrowbtn.setIcon(Util.getImageIcon("nextfid.gif"));
        arrowbtn.addActionListener(this);
        arrowbtn.setActionCommand("move");
        arrowbtn.setEnabled(false);
       
        upbtn.setIcon(Util.getImageIcon("up11x17.gif"));
        upbtn.addActionListener(this);
        upbtn.setActionCommand("up");
        upbtn.setEnabled(false);

        downbtn.setIcon(Util.getImageIcon("down11x17.gif"));
        downbtn.addActionListener(this);
        downbtn.setActionCommand("down");
        downbtn.setEnabled(false);

        m_objDividerUI = new ButtonDividerUI();

        setResizable(true);

        splitPane.setUI(getDividerUI());
        splitPane.setDividerLocation(0.5);
        splitPane.setResizeWeight(0.5);
        JPanel content = new JPanel();
        content.add(treeScroll);

        attrpanel.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 4, 10,
                false));

        attrpanel.setPreferredSize(new Dimension(windowWidth - 10, 35));
        JLabel label = new JLabel("Name");
        attrpanel.add(label);
        nameEntry = new VUndoableText();
        nameEntry.setPreferredSize(new Dimension(150, 25));
        nameEntry.addActionListener(this);
        nameEntry.setActionCommand("attredit");
        attrpanel.add(nameEntry);

        label = new JLabel("View");
        attrpanel.add(label);
        viewEntry = new VUndoableText();
        viewEntry.setPreferredSize(new Dimension(60, 25));
        viewEntry.addActionListener(this);
        viewEntry.setActionCommand("attredit");
        attrpanel.add(viewEntry);

        fileLabel = new JLabel("File");
        attrpanel.add(fileLabel);
        fileEntry = new VUndoableText();
        fileEntry.setPreferredSize(new Dimension(150, 25));
        attrpanel.add(fileEntry);

        content.add(attrpanel);
        content.setLayout(new PanelLayout());
        m_contentPane.add(content, BorderLayout.CENTER);
        setRevertBtn();

    }

    @Override
    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
        if (cmd.equals("apply")) {
            apply();
            setVisible(false);
        } else if (cmd.equals("cancel")) {
            setVisible(false);
        } else if (cmd.equals("attredit")) {
            if (selectedNode != null) {
                setNodeFromInfo(selectedNode);
                closeButton.setEnabled(true);
            }
            if (selectedTree != null) {
                selectedTree.repaint();
            }
        }
        else if (cmd.equals("move")) {
            if (selectedTree==leftTree) 
                removeSelectedtab();
            else 
                addSelectedtab();           
        }
        else if (cmd.equals("up")) {
            moveNodeUpDown(UP);           
        }
        else if (cmd.equals("down")) {
            moveNodeUpDown(DOWN);         
        }
        else if (cmd.equals("revert")) {
            revert();         
        }
    }

    public BasicSplitPaneUI getDividerUI() {
        return m_objDividerUI;
    }

    public void showAllTabs(){
        VnmrjIF vjIf = Util.getVjIF();
        if (vjIf == null) {
            return;
        }
        Enumeration<ToolInfo> tooltabs=xmltabs.elements();
        while (tooltabs.hasMoreElements()){
            ToolInfo tool=tooltabs.nextElement();
            vjIf.openComp(tool.name, "open");
        }
        tooltabs=loctabs.elements();
        while (tooltabs.hasMoreElements()){
            ToolInfo tool=tooltabs.nextElement();
            vjIf.openComp(tool.name, "open");
        }
    }
    void updateLists() {
        closeButton.setEnabled(false);
        buildLeftTree();
        buildRightTree();
        showAllTabs();
    }

    void apply(){
        String path=FileUtil.savePath("USER/INTERFACE/");
        if(path==null){
            System.out.println("Could not create directory:"+path);
            return;
        }
        createTabbedToolPanelXML();
        createToolPanelXML();
        Util.rebuildUI();
    }
    
    void revert(){
        String type = getAppType();
        String name="TabbedToolPanel";
        if(type!=null)
            name=name+"."+type;
        name=name+".xml";
        String path=FileUtil.openPath("USER/INTERFACE/"+name);
        if(path !=null){
            File file=new File(path);
            file.delete();
        }
        name="ToolPanel";
        if(type!=null)
            name=name+"."+type;
        name=name+".xml";
        path=FileUtil.openPath("USER/INTERFACE/"+name);
        if(path !=null){
            File file=new File(path);
            file.delete();
        } 
        setRevertBtn();
        Util.rebuildUI();
        setVisible(false);
    }
    void setRevertBtn(){
        revertbtn.setEnabled(userFile());
    }
    boolean userFile(){
        String type = getAppType();
        String name="TabbedToolPanel";
        if(type!=null)
            name=name+"."+type;
        name=name+".xml";
        String path=FileUtil.openPath("USER/INTERFACE/"+name);
        if(path !=null)
            return true;
        name="ToolPanel";
        if(type!=null)
            name=name+"."+type;
        name=name+".xml";
        path=FileUtil.openPath("USER/INTERFACE/"+name);
        if(path !=null)
            return true;
        return false;
    }
    
    void setButtons(){
        if(selectedNode==null){
            arrowbtn.setEnabled(false);
            upbtn.setEnabled(false);
            downbtn.setEnabled(false);
        }
        else{
            arrowbtn.setEnabled(true);
            if(selectedTree==leftTree){
                upbtn.setEnabled(true);
                downbtn.setEnabled(true);  
            }
            else{
                upbtn.setEnabled(false);
                downbtn.setEnabled(false);                  
            }
        }       
    }
    /**
     * Write out new TabbedToolPanel.xml to ~/vnmrsys/templates/vnmrj/interface
     */
    void createTabbedToolPanelXML(){
        String type = getAppType();
        String name="TabbedToolPanel";
        if(type!=null)
            name=name+"."+type;
        name=name+".xml";
        String path=FileUtil.savePath("USER/INTERFACE/"+name);
        PrintWriter os=null;
        try {
            os = new PrintWriter(
            new OutputStreamWriter(
            new FileOutputStream(path), "UTF-8"));
        }
        catch(IOException er) { }
        if (os == null) {
            Messages.postError(new StringBuffer().append("error writing: ").
                                append(path).toString());
            return;
        }
        os.println("<?xml version=\"1.0\" ?>");
        os.println("");
        os.println("<toolDoc>");
        TreeModel model = leftTree.getModel();
        Object root = model.getRoot();
        int cc = model.getChildCount(root);
        for (int i = 0; i < cc; i++) {
            Object child = model.getChild(root, i);
            DefaultMutableTreeNode node = (DefaultMutableTreeNode) child;
            ToolInfo info = (ToolInfo) node.getUserObject();
            if(info.type==LOCATOR){
                os.print("    <tool name=\"Locator\" ");
            }
            else{
                os.print("    <tool name=\"XMLToolPanel\" label=\"");
                os.print(info.name);
                os.print("\" file=\""+info.file+"\"");
                if(info.viewport!=null && info.viewport.length()>0){
                    os.print(" viewport=\""+info.viewport+"\"");
                }
            }
            os.println("/>");
        }
        os.println("</toolDoc>");
        os.close();
    }
    
    /**
     * Write out new ToolPanel.xml to ~/vnmrsys/templates/vnmrj/interface
     */
   void createToolPanelXML(){
        findLocatorNode(leftTree);
        if(locatorNode==null){
            System.out.println("createToolPanelXML:could not find locator node in tree");
            return;
        }
        String type = getAppType();
        String name="ToolPanel";
        if(type!=null)
            name=name+"."+type;
        name=name+".xml";
        String path=FileUtil.savePath("USER/INTERFACE/"+name);
        PrintWriter os=null;
        try {
            os = new PrintWriter(
            new OutputStreamWriter(
            new FileOutputStream(path), "UTF-8"));
        }
        catch(IOException er) { }
        if (os == null) {
            Messages.postError(new StringBuffer().append("error writing: ").
                                append(path).toString());
            return;
        }
        os.println("<?xml version=\"1.0\" ?>");
        os.println("");
        os.println("<toolDoc>");
        TreeModel model = leftTree.getModel();
        Object root = locatorNode;
        int cc = model.getChildCount(root);
        for (int i = 0; i < cc; i++) {
            Object child = model.getChild(root, i);
            DefaultMutableTreeNode node = (DefaultMutableTreeNode) child;
            ToolInfo info = (ToolInfo) node.getUserObject();
            os.print("    <tool name=\"");
            os.print(info.name);
            os.print("\"");
            if(info.viewport!=null && info.viewport.length()>0){
                os.print(" viewport=\""+info.viewport+"\"");            
            }
            os.println("/>");
        }       
        os.println("</toolDoc>");
        os.close();
    }
    
    void setInfoFromNode(DefaultMutableTreeNode node) {
        ToolInfo info = (ToolInfo) node.getUserObject();
        if (info.type == XMLTAB)
            fileLabel.setText("File");
        else
            fileLabel.setText("Macro");

        nameEntry.setText(info.name);
        fileEntry.setText(info.file);
        viewEntry.setText(info.viewport);
    }

    void setNodeFromInfo(DefaultMutableTreeNode node) {
        ToolInfo info = (ToolInfo) node.getUserObject();
        info.name = nameEntry.getText();
        info.file = fileEntry.getText();
        info.viewport = viewEntry.getText();
        node.setUserObject(info);
    }

    /**
     * rebuild "Current" tree from xml files
     */
    void buildLeftTree() {
        leftRoot.removeAllChildren();
        DefaultTreeModel model = (DefaultTreeModel) leftTree.getModel();
        model.reload();
        locatorNode = null;
        xmltabs.clear();
        getXMLNodes("INTERFACE/TabbedToolPanel.xml", leftRoot, xmltabs);
        if (locatorNode != null) {
            loctabs.clear();
            getLocatorNodes("INTERFACE/ToolPanel.xml", locatorNode, loctabs);
        }
        for (int i = 0; i < leftTree.getRowCount(); i++) {
            leftTree.expandRow(i);
        }
    }

    /**
     * Parse a file given by "path" and extract all "XML" nodes
     * @param path      full path to file (TabbedToolPanel.xml)
     * @param root      root node in tree
     * @param hash      hash table of currently extracted nodes (to avoid duplicates)
     */
    void getXMLNodes(String path, DefaultMutableTreeNode root,
            Hashtable<String, ToolInfo> hash) {
        String toolPanelFile = FileUtil.openPath(path);
        if (toolPanelFile == null) {
            System.out.println("could not open tab list file");
            return;
        }
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            spf.setValidating(false); // set to true if we get DOCTYPE
            spf.setNamespaceAware(false); // set to true with referencing
            SAXParser parser = spf.newSAXParser();
            parser.parse(new File(toolPanelFile), new TabbedToolsParser(root,
                    hash));

        } catch (Exception e) {
            System.out.println("Error parsing tab list file");
        }

    }

    void getLocatorNodes(String path, DefaultMutableTreeNode root,
            Hashtable<String, ToolInfo> hash) {
        String toolPanelFile = FileUtil.openPath(path);
        if (toolPanelFile == null) {
            System.out.println("could not open locator list file");
            return;
        }
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            spf.setValidating(false); // set to true if we get DOCTYPE
            spf.setNamespaceAware(false); // set to true with referencing
            SAXParser parser = spf.newSAXParser();
            parser.parse(new File(toolPanelFile), new ToolsParser(root, hash));
        } catch (Exception e) {
            System.out.println("Error parsing locator list file");
        }
    }

    /**
     * Sax parser for TabbedToolsPanel.xml
     */
   public class TabbedToolsParser extends DefaultHandler {
        DefaultMutableTreeNode      root;
        Hashtable<String, ToolInfo> hash;

        TabbedToolsParser(DefaultMutableTreeNode parent,
                Hashtable<String, ToolInfo> h) {
            root = parent;
            hash = h;
        }

        @Override
        public void error(SAXParseException spe) {
            System.out.println("Error at line " + spe.getLineNumber()
                    + ", column " + spe.getColumnNumber());
        }

        @Override
        public void startElement(String uri, String localName, String qName,
                Attributes attr) {
            if (qName.equals("tool")) {
                ToolInfo info = new ToolInfo();
                info.name = attr.getValue("label");
                if(attr.getValue("viewport")!=null)
                    info.viewport = attr.getValue("viewport");
                else
                    info.viewport="all";
                String type = attr.getValue("name");
                if (type.equals("XMLToolPanel")) {
                    info.type = XMLTAB;
                    if(attr.getValue("file")!=null)
                        info.file = attr.getValue("file");
                    String f = "LAYOUT" + File.separator + "toolPanels"
                            + File.separator + info.file;
                    if (FileUtil.openPath(f) == null)
                        return;
                    if (hash != null) {
                        if (hash.containsKey(info.file))
                            return;
                        else
                            hash.put(info.file, info);
                    }
                } else if (type.equals("Locator")) {
                    info.name = "Locator";
                    info.type = LOCATOR;
                }
                DefaultMutableTreeNode node = new DefaultMutableTreeNode(info);
                root.add(node);
                if (info.type == LOCATOR) {
                    locatorNode = node;
                }
            }
        }
    }

    /**
     * Sax parser for ToolsPanel.xml
     */
    public class ToolsParser extends DefaultHandler {
        DefaultMutableTreeNode      root;
        Hashtable<String, ToolInfo> hash;

        @Override
        public void error(SAXParseException spe) {
            System.out.println("Error at line " + spe.getLineNumber()
                    + ", column " + spe.getColumnNumber());
        }

        ToolsParser(DefaultMutableTreeNode parent, Hashtable<String, ToolInfo> h) {
            root = parent;
            hash = h;
        }

        @Override
        public void startElement(String uri, String localName, String qName,
                Attributes attr) {
            if (qName.equals("tool")) {
                ToolInfo info = new ToolInfo();
                info.name = attr.getValue("name");
                if(attr.getValue("viewport")!=null)
                    info.viewport = attr.getValue("viewport");
                info.type = LOCATORTAB;
                if (hash != null) {
                    if (hash.containsKey(info.name))
                        return;
                    else
                        hash.put(info.name, info);
                }
                DefaultMutableTreeNode node = new DefaultMutableTreeNode(info);
                root.add(node);
            }
        }
    }

    /**
     * @return User appmode type (e.g. "", "imaging", "lc")
     */
    public String getAppType() {
        ArrayList<?> apptypes = FileUtil.getAppTypes();
        if (apptypes.size() == 0)
            return null;
        return apptypes.get(0).toString();
    }

    /**
     * Build tree list of Available nodes (i.e. those not currently in use)
     */
    void buildRightTree() {
        String type = getAppType();
        String sysdir = FileUtil.sysdir();
        String basedir = sysdir;
        if (type != null)
            basedir = basedir + "/" + type;

        basedir = basedir + "/INTERFACE/";
        rightRoot.removeAllChildren();
        DefaultTreeModel model = (DefaultTreeModel) rightTree.getModel();
        model.reload();
        getLocatorNodes(rightRoot, loctabs);

        String[] tooldirs = FileUtil
                .openPaths("LAYOUT/toolPanels", false, true);
        for (int i = 0; i < tooldirs.length; i++) {
            String dir = tooldirs[i];
            ToolInfo info = new ToolInfo();
            info.name = dir;
            info.type = DIRECTORY;
            info.file = dir;
            DefaultMutableTreeNode node = new DefaultMutableTreeNode(info);
            rightRoot.add(node);

            String tooldir = FileUtil.expandPath(tooldirs[i]);
            getFileNodes(tooldir, node, xmltabs);

        }

        for (int i = 0; i < rightTree.getRowCount(); i++) {
            rightTree.expandRow(i);
        }
    }

    void getLocatorNodes(DefaultMutableTreeNode root,
            Hashtable<String, ToolInfo> hash) {
        ToolInfo info = new ToolInfo();
        info.name = "Locator";
        info.type = LOCATOR;
        DefaultMutableTreeNode node1 = new DefaultMutableTreeNode(info);
        root.add(node1);
        for (int i = 0; i < toolNames.length; i++) {
            if (!hash.containsKey(toolNames[i])) {
                info = new ToolInfo();
                info.name = toolNames[i];
                info.type = LOCATORTAB;
                DefaultMutableTreeNode node2 = new DefaultMutableTreeNode(info);
                node1.add(node2);
                hash.put(info.name, info);
            }
        }
    }

    void getFileNodes(String tooldir, DefaultMutableTreeNode root,
            Hashtable<String, ToolInfo> hash) {
        if (tooldir != null && tooldir.length() > 0) {
            File dir = new File(tooldir);
            String[] files = dir.list();
            for (int j = 0; j < files.length; j++) {
                String file = files[j];
                ToolInfo info = new ToolInfo();
                info.name = FileUtil.fileName(file);
                info.viewport = "all";
                info.type = XMLTAB;
                info.file = file;
                if (!hash.containsKey(file)) {
                    DefaultMutableTreeNode node = new DefaultMutableTreeNode(
                            info);
                    root.add(node);
                    hash.put(file, info);
                }
            }
        }
    }

    /**
     * Move a node from the "Available" tree to the "Current" tree
     */
    void addSelectedtab(){
        ToolInfo info = (ToolInfo) selectedNode.getUserObject();
        if(info.type==LOCATORTAB){
            findLocatorNode(rightTree);
            if(locatorNode!=null){
                DefaultTreeModel model = (DefaultTreeModel) rightTree.getModel();
                model.removeNodeFromParent(selectedNode);
                findLocatorNode(leftTree);
                if(locatorNode!=null){
                    model = (DefaultTreeModel) leftTree.getModel();
                    model.insertNodeInto(selectedNode, locatorNode, locatorNode.getChildCount());                
                }
                closeButton.setEnabled(true);
            }
        }
        else if(info.type==XMLTAB){
            // 1. remove node from right tree parent (DIRECTORY node)
            DefaultTreeModel model = (DefaultTreeModel) rightTree.getModel();
            model.removeNodeFromParent(selectedNode);
            // 2. add node to left tree root 
            model = (DefaultTreeModel) leftTree.getModel();
            DefaultMutableTreeNode root = (DefaultMutableTreeNode)model.getRoot();
            model.insertNodeInto(selectedNode, root, root.getChildCount());
            closeButton.setEnabled(true);
        }
    }
    /**
     * Move a node from the "Current" tree to the "Available" tree
     */
    void removeSelectedtab(){
        ToolInfo info = (ToolInfo) selectedNode.getUserObject();
        if(info.type==LOCATORTAB){
            findLocatorNode(leftTree);
            if(locatorNode!=null){
                DefaultTreeModel model = (DefaultTreeModel) leftTree.getModel();
                model.removeNodeFromParent(selectedNode);
                findLocatorNode(rightTree);
                if(locatorNode!=null){
                    model = (DefaultTreeModel) rightTree.getModel();
                    model.insertNodeInto(selectedNode, locatorNode, locatorNode.getChildCount());
                }
            }
            closeButton.setEnabled(true);
        }
        else if(info.type==XMLTAB){
            //  1. remove node from left tree root
            DefaultTreeModel model = (DefaultTreeModel) leftTree.getModel();
            model.removeNodeFromParent(selectedNode);
            //  2. use "openpath" to find appdir directory containing node file 
            directoryPath=null;
            String path="LAYOUT/toolPanels/"+info.file;
            path=FileUtil.openPath(path);  // get fullpath to first hit in appdir search
            if(path !=null)
                path=FileUtil.pathBase(path);  // remove filename
            if(path !=null&& path.length()>1)
                path=path.substring(0,path.length()-1); // remove trailing "/"
            //  3. find directory node in right tree
            if(path!=null&& path.length()>1){
                directoryPath=path;
                findDirectoryNode(rightTree);
            //  4. add node to right tree directory parent node
                if(directoryNode !=null){
                    model = (DefaultTreeModel) rightTree.getModel();
                    model.insertNodeInto(selectedNode, directoryNode, directoryNode.getChildCount());
                    closeButton.setEnabled(true);
                }
            }
        }
    }
    /**
     * move a tree node up or down in parent's child list
     */
    void moveNodeUpDown(int direction){
        DefaultTreeModel model = (DefaultTreeModel) leftTree.getModel();
        DefaultMutableTreeNode parent=(DefaultMutableTreeNode)selectedNode.getParent();
        int loc=model.getIndexOfChild(parent, selectedNode);
        if(direction==UP){
            if(loc==0)
                return;
            loc--;
        }
        else{
            int cnt=parent.getChildCount(); // parent child count
            if(loc==cnt) 
                return;
            if(loc<cnt-1)
                loc++;
        }
        model.removeNodeFromParent(selectedNode);
        model.insertNodeInto(selectedNode, parent,loc);
        TreeNode[] nodes = model.getPathToRoot(selectedNode);  
        TreePath path = new TreePath(nodes);  
        //System.out.println(path.toString());    // Able to get the exact node here  
        leftTree.setExpandsSelectedPaths(true);                
        leftTree.setSelectionPath(new TreePath(nodes));  
        closeButton.setEnabled(true);
    }
    /**
     * Find the top-level Locator node in either tree
     * @param tree
     */
    void findLocatorNode(JTree tree) {
        locatorNode = null;
        TreeModel model = tree.getModel();
        if (model != null) {
            Object root = model.getRoot();
            findLocatorNode(model, root);
        } else
            System.out.println("Tree is empty.");
    }

    void findLocatorNode(TreeModel model, Object o) {
        int cc;
        cc = model.getChildCount(o);
        for (int i = 0; i < cc; i++) {
            Object child = model.getChild(o, i);
            DefaultMutableTreeNode node = (DefaultMutableTreeNode) child;
            ToolInfo info = (ToolInfo) node.getUserObject();
            if (info.type == LOCATOR) {
                locatorNode = node;
            } else if (!model.isLeaf(child))
                findLocatorNode(model, child);
        }
    }

    /**
     * Find a directory node by name in the "Available" tree
     * @param tree
     */
    public void findDirectoryNode(JTree tree) {
        directoryNode = null;
        TreeModel model = tree.getModel();
        if (model != null) {
            Object root = model.getRoot();
            findDirectoryNode(model, root);
        } else
            System.out.println("Tree is empty.");
    }

    /**
     * Recursive tree-traversal helper function for findDirectoryNode(JTree tree)
     * @param model
     * @param o
     */
    void findDirectoryNode(TreeModel model, Object o) {
        int cc;
        cc = model.getChildCount(o);
        for (int i = 0; i < cc; i++) {
            Object child = model.getChild(o, i);
            DefaultMutableTreeNode node = (DefaultMutableTreeNode) child;
            ToolInfo info = (ToolInfo) node.getUserObject();
            if (info.type == DIRECTORY && info.file.equals(directoryPath)) {
                directoryNode = node;
            } else if (!model.isLeaf(child))
                findDirectoryNode(model, child);
        }
    }

    class ButtonDividerUI extends BasicSplitPaneUI {

        @Override
        public BasicSplitPaneDivider createDefaultDivider() {
            BasicSplitPaneDivider divider = new BasicSplitPaneDivider(this) {
                private static final long serialVersionUID = 1L;

                @Override
                public int getDividerSize() {
                    return upbtn.getPreferredSize().width;
                }
            };

            int nHeight = treeScroll.getSize().height / 2;
            nHeight = (nHeight <= 0) ? 30 : nHeight;
            divider.setLayout(new BoxLayout(divider, BoxLayout.Y_AXIS));
            divider.add(Box.createVerticalStrut(nHeight));
            divider.add(arrowbtn);
            divider.add(upbtn);
            divider.add(downbtn);
            divider.add(Box.createVerticalStrut(nHeight));
            return divider;
        }
    }

    class ToolInfo {
        public String name     = "";
        public String file     = "";
        public String viewport = "";
        int           type     = 0;

        ToolInfo() {
        }

        @Override
        public String toString() {
            return name;
        }
    }

    class TreeSelector implements TreeSelectionListener {
        DefaultMutableTreeNode root;
        JTree                  tree;

        TreeSelector(JTree t, DefaultMutableTreeNode r) {
            tree = t;
            root = r;
            tree.addTreeSelectionListener(this);
        }

        public void valueChanged(TreeSelectionEvent e) {
            DefaultMutableTreeNode node = (DefaultMutableTreeNode) tree
                    .getLastSelectedPathComponent();

            if (node == null || node == root)
                return;
            ToolInfo info = (ToolInfo) node.getUserObject();
            if (info.type == LOCATOR || info.type == DIRECTORY)
                return;
            if (selectedNode != null){
                setNodeFromInfo(selectedNode);
                if(selectedTree !=tree)
                    selectedTree.getSelectionModel().clearSelection();
            }
            selectedTree = tree;
            if(tree==leftTree)
                arrowbtn.setIcon(Util.getImageIcon("nextfid.gif"));
            else
                arrowbtn.setIcon(Util.getImageIcon("prevfid.gif"));
            setInfoFromNode(node);
            selectedNode = node;
            setButtons();

        }

    }

    private class PanelLayout implements LayoutManager {
        @Override
        public void addLayoutComponent(String name, Component comp) {
        }

        @Override
        public void removeLayoutComponent(Component comp) {
        }

        @Override
        public Dimension preferredLayoutSize(Container target) {
            Dimension dim;
            int w = 0;
            int h = 0;
            int k;
            int n = target.getComponentCount();
            for (k = 0; k < n; k++) {
                Component m = target.getComponent(k);
                dim = m.getPreferredSize();
                if (dim.width > w)
                    w = dim.width;
                h += dim.height + 2;
            }
            return new Dimension(w, h);
        }

        @Override
        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        @Override
        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim;
                int n = target.getComponentCount();
                int h = 0;
                int hs = 0;
                int k;

                for (k = 0; k < n; k++) {
                    Component m = target.getComponent(k);
                    dim = m.getPreferredSize();
                    if (m != treeScroll)
                        hs += dim.height;
                }
                if (hs > 500) // something wrong
                    hs = etcH;
                else
                    etcH = hs;

                Dimension dim0 = target.getSize();
                windowWidth = dim0.width;
                hs = dim0.height - hs - 4;
                if (hs < 20)
                    hs = 20;
                scrollH = hs;
                if (sbW == 0) {
                    JScrollBar jb = treeScroll.getVerticalScrollBar();
                    if (jb != null) {
                        dim = jb.getPreferredSize();
                        sbW = dim.width;
                    } else
                        sbW = 20;
                }
                for (k = 0; k < n; k++) {
                    Component obj = target.getComponent(k);
                    if (obj.isVisible()) {
                        dim = obj.getPreferredSize();
                        if (obj == treeScroll) {
                            obj.setBounds(2, h, dim0.width - 4, hs);
                            h += hs;
                        } else {
                            if (dim.height > 500) {
                                if (obj == attrpanel)
                                    dim.height = 100;
                            }
                            obj.setBounds(2, h, dim0.width - 4, dim.height);
                            h += dim.height;
                        }
                    }
                }
            }
        }
    } // class PanelLayout
}
