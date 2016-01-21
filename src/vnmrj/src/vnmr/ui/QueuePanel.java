/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import java.util.List;

import javax.swing.*;
import javax.swing.border.*;
import java.beans.*;

import javax.swing.tree.*;
import vnmr.ui.shuf.*;
import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;

import com.sun.xml.tree.*;

import static vnmr.templates.ProtocolBuilder.*;

/**
 * The study queue panel.
 */
public class QueuePanel extends JPanel 
    implements VObjDef, ExpListenerIF, PropertyChangeListener
{
    SessionShare 		sshare;
    ButtonIF 	 		expIF=null;
    JScrollPane 		scrollpane;
    JTree 		        jtree;
    ProtocolBuilder 	mgr;
    JPanel              infopanel;
    JPanel              selectpanel;
    String              infofile="INTERFACE/StudyQueue";
    String              selectfile="INTERFACE/StudyQueueSelect.xml";
    final String        m_showStatusKey = "qlShowStatus";
    String              m_showStatusLabel;
    final String        m_showTimesKey = "qlShowExpTimes";
    String              m_showTimesLabel;
    final String        m_showIconsKey = "qlShowIcons";
    String              m_showIconsLabel;
    final String        m_expandNewKey = "qlExpandNew";
    String              m_expandNewLabel;
    final String        m_deleteErrorsKey = "qlDelErrNode";
    String              m_deleteErrorsLabel;
    final String        m_confDelKey = "qlConfirmDel";
    String              m_confDelLabel;
    String              status_label;
    boolean             showStatus=true;
    SubMenu             menu=null;
    MouseAdapter        ma=null;
    StudyQueue          sq=null;
    ActionItemListener m_actionItemListener = new ActionItemListener();
    private boolean m_confirmDelete;
    private boolean m_writeSqDone = false;
    static String 		persistence_file = "QueuePanel";

    /**
     * constructor
     */
    public QueuePanel(SessionShare sshare, String id, String macro) {
        this.sshare = sshare;
        sq=new StudyQueue();
        mgr=sq.getMgr();
        mgr.newTree();

        if(macro != null && macro.length() > 0) sq.setMacro(macro);

        if(id == null || id.length() <= 0) id = "SQ";
        sq.setSqId(id);
        if(id.equals("SQ")) {
            infofile += ".xml";    
        } else {
            infofile += "_"+id+".xml";    
        }

        String str = System.getProperty("SQAllowNesting");
        if(str !=null && (str.equals("no") || str.equals("false")))
            mgr.setAllowNesting(false);

        status_label=Util.getLabel("qlStatusLabel","Experiment Queue");
        m_showStatusLabel = Util.getLabel(m_showStatusKey,"Show Study panel");
        m_showTimesLabel = Util.getLabel(m_showTimesKey,"Show Times");
        m_showIconsLabel = Util.getLabel(m_showIconsKey,"Show Icons");
        m_expandNewLabel = Util.getLabel(m_expandNewKey,"Expand New Nodes");
        m_deleteErrorsLabel = Util.getLabel(m_deleteErrorsKey,
                                            "Delete Error Nodes");
        m_confDelLabel = Util.getLabel(m_confDelKey, "Confirm Data Deletion");


        setOpaque(true);
        setLayout(new BorderLayout());

        scrollpane = new JScrollPane();

        jtree=mgr.buildTreePanel();

        scrollpane.setViewportView(jtree);
        JViewport vp = scrollpane.getViewport();

        add(scrollpane, BorderLayout.CENTER);
        if(buildInfoPanel()) {
            add(infopanel, BorderLayout.SOUTH);
        }
        if(buildSelectPanel()) {
            mgr.setSelectPanel(selectpanel);
            add(selectpanel, BorderLayout.NORTH);
        }

        mgr.expandTree();

        menu=new SubMenu();
        ma = new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                int modifier = e.getModifiers();
                VElement sel = null;
                int x = e.getX();
                int y = e.getY();
                TreePath path = jtree.getPathForLocation(x, y);
                if (path != null) {
                    sel=(VElement)path.getLastPathComponent();
                }
                if ((modifier & InputEvent.BUTTON1_MASK) != 0) {
                    // Left click -- selection may need modification
                    // TODO: Check for shift/ctl
                    if (sel != null) {
                        if ("true".equals(sel.getAttribute(ATTR_SQUISHED))) {
                            boolean isSelected = jtree.isPathSelected(path);
                            ArrayList<VElement> elems;
                            elems = selectContiguousSquishedNodes(sel);
                            for (VElement ielem : elems) {
                                TreePath ipath = mgr.getPath(ielem);
                                if (isSelected) {
                                    jtree.addSelectionPath(ipath);
                                } else {
                                    jtree.removeSelectionPath(ipath);
                                }
                            }
                        }
                    }
                } else if ((modifier & InputEvent.BUTTON3_MASK) != 0) {
                    // Deal with right clicks
                    TreePath[] paths = jtree.getSelectionPaths();
                    String nodeIds = getNodeIds(paths);

                    boolean clickedOnAction = false;
                    if (sel != null && sel instanceof VActionElement) {
                        clickedOnAction = true;
                        VActionElement action = (VActionElement)sel;
                        String selId = action.getAttribute("id");
                        if (!nodeIds.contains(selId)) {
                            // Clicked on unselected node...
                            // ...set node selection to that single node.
                            nodeIds = selId;
                            jtree.setSelectionPath(path);
                        }
                        // Prepend nodeIds list with duplicate of primary
                        // node id.
                        nodeIds = selId + " " + nodeIds;
                    }
                    if (clickedOnAction) {
                        // Show action item's popup menu
                        StringBuffer cmd = new StringBuffer("xmGetMenuForNode(")
                            .append(" ").append(x).append(",").append(y)
                            .append(",'")
                            .append(nodeIds)
                            //.append(((VActionElement)sel).getAttribute("id"))
                            .append("')");
                        Util.sendToVnmr(cmd.toString());
                    } else {
                        // Show SQ panel's popup menu
                        menuAction(e);
                    }
                }
            }
        };
        jtree.addMouseListener(ma);
        Util.setStudyQueue(this, id);
        ExpPanel.addExpListener(this); // register as a vnmrbg listener
        DisplayOptions.addChangeListener(this);

    } // QueuePanel()

    protected ArrayList<VElement> selectContiguousSquishedNodes(VElement elem) {
        TreePath[] paths = jtree.getSelectionPaths();
        String nodeIds = getNodeIds(paths);
        String[] ids = nodeIds.split(" +");
        return mgr.getContiguousSquishedNodes(elem);
    }

    private String getNodeIds(TreePath[] paths) {
        StringBuffer sb = new StringBuffer();
        if (paths != null) {
            for (TreePath path : paths) {
                VElement elem = (VElement)path.getLastPathComponent();
                if (elem instanceof VActionElement) {
                    sb.append( ((VActionElement)elem).getAttribute("id") );
                    sb.append(" ");
                }
            }
        }
        return sb.toString().trim();
    }

    /** PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        if(DisplayOptions.isUpdateUIEvent(evt)){
           // SwingUtilities.updateComponentTreeUI(this);
            SwingUtilities.updateComponentTreeUI(menu);
           // SwingUtilities.updateComponentTreeUI(infopanel);
           // SwingUtilities.updateComponentTreeUI(selectpanel);
        }
    }

    protected JPopupMenu getPopupMenuFor(VActionElement sel) {
        JPopupMenu menu = new JPopupMenu();
        String filename = "INTERFACE/SQActionMenu.txt";
        String menuPath = FileUtil.openPath(filename);
        if (menuPath == null) {
            Messages.postError("SQ menu file not found: " + filename);
        } else {
            BufferedReader reader = null;
            try {
                reader = new BufferedReader(new FileReader(menuPath));
                String str;
                while ((str = reader.readLine()) != null) {
                    String[] key_value = str.split("=", 2);
                    String label = key_value[0];
                    String cmd = "";
                    if (key_value.length > 1) {
                        cmd = key_value[1];
                    }
                    SQMenuItem item = new SQMenuItem(label, sel);
                    item.setActionCommand(cmd);
                    item.addActionListener(m_actionItemListener);
                    menu.add(item);

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

//        String cmd = "Delete";
//        // TODO: translate cmd
//        SQMenuItem item = new SQMenuItem(cmd, sel);
//        item.setActionCommand("xmaction('delete','$ATTRIBUTE(id)')");
//        item.addActionListener(m_actionItemListener);
//        menu.add(item);
        return menu;
    }

    protected void showContextMenu(int x, int y, String id, String path) {
        JPopupMenu menu = new JPopupMenu();
        String menuPath = FileUtil.openPath(path);
        if (menuPath == null) {
            Messages.postError("SQ menu file not found: " + path);
        } else {
            BufferedReader reader = null;
            try {
                reader = new BufferedReader(new FileReader(menuPath));
                String str;
                while ((str = reader.readLine()) != null) {
                    String[] key_value = str.split("=", 2);
                    String label = key_value[0];
                    String cmd = "";
                    if (key_value.length > 1) {
                        cmd = key_value[1];
                    }
                    SQMenuItem item = new SQMenuItem(label, null); // TODO: Get the node from the ID
                    item.setActionCommand(cmd);
                    item.addActionListener(m_actionItemListener);
                    menu.add(item);

                }
                // Don't delete menu file after use; responsibility of macros
                //new File(menuPath).delete();
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
        menu.show(jtree, x, y);
    }

    // ExpListenerIF interface
    /** called from ExpPanel (pnew)   */
    public synchronized void  updateValue(Vector v){
        for (int i=0; i<infopanel.getComponentCount(); i++) {
            Component comp = infopanel.getComponent(i);
            if (comp instanceof ExpListenerIF)
                ((ExpListenerIF)comp).updateValue(v);
        }
        if (selectpanel != null)
          for (int i=0; i<selectpanel.getComponentCount(); i++) {
            Component comp = selectpanel.getComponent(i);
            if (comp instanceof ExpListenerIF)
                ((ExpListenerIF)comp).updateValue(v);
        }
    }

    /** called from ExperimentIF for first time initialization  */
    public synchronized void  updateValue(){
        for (int i=0; i<infopanel.getComponentCount(); i++) {
            Component comp = infopanel.getComponent(i);
            if (comp instanceof ExpListenerIF)
                ((ExpListenerIF)comp).updateValue();
        }
        if (selectpanel != null)
          for (int i=0; i<selectpanel.getComponentCount(); i++) {
            Component comp = selectpanel.getComponent(i);
            if (comp instanceof ExpListenerIF)
                ((ExpListenerIF)comp).updateValue();
        }
    }

    //============ private Methods  ==================================
    private ButtonIF getExpIF(){
		return Util.getViewArea().getDefaultExp();
    }

    private boolean buildSelectPanel() {
        String path = FileUtil.openPath(selectfile);
        if(path == null) return false;

        selectpanel=new JPanel();

        try {
             LayoutBuilder.build(selectpanel,getExpIF(),path);
        }
        catch (Exception e) {
             String strError = "Error building file: " + selectfile;
             Messages.postError(strError);
             Messages.writeStackTrace(e);
             return false;
        }
        return true;
    }

    private boolean buildInfoPanel() {
        String path = FileUtil.openPath(infofile);
        if(path == null) return false;

        infopanel=new JPanel();

        try {
             LayoutBuilder.build(infopanel,getExpIF(),path);
        }
        catch (Exception e) {
             String strError = "Error building file: " + infofile;
             Messages.postError(strError);
             Messages.writeStackTrace(e); 
             return false;
        }
	return true;
    }

    private void setDebug(String s){
        if(DebugOutput.isSetFor("SQ"))
            Messages.postDebug("QueuePanel "+s);
    }

    //----------------------------------------------------------------
    /**
     * Show pop-up context menu for the Study Queue Panel.
     * @param e The MouseEvent that triggered this action.
     */
    //----------------------------------------------------------------
    private void menuAction(MouseEvent  e) {
        menu.updateMenu().show(e.getComponent(), e.getX(), e.getY());
    }

    //----------------------------------------------------------------
	/** clear all tree items. */
	//----------------------------------------------------------------
    public synchronized void clearTree(){
        mgr.clearTree();
    }
 
    //----------------------------------------------------------------
	/** set the study queue io processor. */
	//----------------------------------------------------------------
    public void setStudyQueue(StudyQueue q){
        sq=q;
        mgr=sq.getMgr();
    }

    //----------------------------------------------------------------
	/** get the study queue io processor. */
	//----------------------------------------------------------------
    public StudyQueue getStudyQueue(){
        return sq;
    }

    protected String[] getErrorActionIds() {
        List<VElement> list = new ArrayList<VElement>();
        list = mgr.getElements(null, ACTIONS);
        for (int i = list.size() - 1; i >= 0; --i) {
            VElement elem = list.get(i);
            if (!ProtocolBuilder.SQ_ERROR.equals(elem.getAttribute(ATTR_STATUS))) {
                list.remove(i);
            }
        }

        if (!"Imaging".equals(Util.getUserAppType())) {
            for (int i = list.size() - 1; i >= 0; --i) {
                VElement elem = list.get(i);
                if ("on".equals(elem.getAttribute(ATTR_LOCK))) {
                    list.remove(i);
                }
            }
        }

        String[] ids = new String[list.size()];
        for (int i = 0; i < ids.length; i++) {
            ids[i] = list.get(i).getAttribute(ATTR_ID);
        }
        return ids;
    }

    //============ Methods called from ProtocolBuilder (sq) ==========

    //----------------------------------------------------------------
	/** Save the tree as an XML file. */
	//----------------------------------------------------------------
    public synchronized void saveStudy(String path){
        sq.saveStudy(path);
    }

    //----------------------------------------------------------------
	/** Return name of study in study dir. */
	//----------------------------------------------------------------
    public String studyName(String dir){
       return sq.studyName(dir);
    }

    //----------------------------------------------------------------
	/** Return true if study can be loaded. */
	//----------------------------------------------------------------
    public boolean testRead(String dir){
        return sq.testRead(dir);
    }

    //----------------------------------------------------------------
	/** Return true if element can be moved. */
	//----------------------------------------------------------------
    public boolean testMove(VElement src, VElement dst){
          return sq.testMove(src, dst);
    }
    
    //----------------------------------------------------------------
	/** Return true if element can be deleted. */
	//----------------------------------------------------------------
    public boolean testDelete(VElement src){
        return sq.testDelete(src);
    }

    //----------------------------------------------------------------
	/** Return true if element can be added here. */
	//----------------------------------------------------------------
    public boolean testAdd(VElement elem){
        return sq.testAdd(elem);
    }

    //----------------------------------------------------------------
	/** Click in treenode. */
	//----------------------------------------------------------------
    public void wasClicked(String id, String dst,int mode){
		sq.wasClicked(id, dst, mode);
    }

    //----------------------------------------------------------------
	/** Double click in treenode. */
	//----------------------------------------------------------------
    public void wasDoubleClicked(String id){
		sq.wasDoubleClicked(id);
    }

    //----------------------------------------------------------------
	/** Tree was modified. */
	//----------------------------------------------------------------
    public void setDragAndDrop(String id){
		sq.setDragAndDrop(id);
    }

    public void setConfirmDelete(boolean b) {
        m_confirmDelete = b;
        int sel = b ? 1 : 0;
        Util.sendToVnmr("xmConfirmDataDelete(" + sel + ")");
    }

    public void setConfirmDelete(String cmd) {
        QuotedStringTokenizer toker = new QuotedStringTokenizer(cmd);
        try {
            toker.nextToken(); // Throw away the command
            int bool = Integer.parseInt(toker.nextToken());
            setConfirmDelete(bool != 0);
        } catch (NoSuchElementException nsee) {
        }
    }

    public boolean getConfirmDelete() {
        return m_confirmDelete;
    }

    private boolean processSelectedCommand(String str) {
        StringTokenizer tok= new StringTokenizer(str, " \t\r\n");
        String  paramStr = null;
        String  macroStr = null;
        String  cmdStr = null;
        String  argStr = null;
        int    count = 0;

        while (tok.hasMoreTokens()) {
            String data = tok.nextToken();
            if (count == 0)
                cmdStr = data;
            else if (count == 1)
                argStr = data;
            else if (count == 2)
                paramStr = data;
            else if (count == 3) {
                macroStr = data;
                break;
            }
            count++;
        }
        if (macroStr == null || macroStr.length() < 1)
            return false;
        if (!argStr.equals("selected"))
            return false;
        String nodeIds = null;
        TreePath[] paths = jtree.getSelectionPaths();
        if (paths != null)
            nodeIds = getNodeIds(paths);
        if (nodeIds == null)
            nodeIds = "";
        StringBuffer sb =
            new StringBuffer(paramStr).append("='").append(nodeIds).append("'\n");
        Util.sendToVnmr(sb.toString());
        Util.sendToVnmr(macroStr);
        
        return true;
    }

    //================ Methods called from Vnmrbg ====================

    //----------------------------------------------------------------
    /** Process a Vnmrbg command  */
    //----------------------------------------------------------------
    public synchronized void processCommand(String str) {
        Messages.postDebug("QueuePanel", str);
        if (str.startsWith("showContextMenu")) {
            showContextMenu(str);
        } else if (str.startsWith("setConfirmDataDelete")) {
            setConfirmDelete(str);
        } else if (str.startsWith("WriteSqDone")) {
            m_writeSqDone = true;
        } else {
            if (str.startsWith("get")) {
                if (processSelectedCommand(str))
                    return;
            }
            sq.processCommand(str);
        }
    }

    private void showContextMenu(String str) {
        QuotedStringTokenizer toker = new QuotedStringTokenizer(str);
        try {
            toker.nextToken(); // Throw away the command
            int x = Integer.parseInt(toker.nextToken());
            int y = Integer.parseInt(toker.nextToken());
            String id = toker.nextToken();
            String path = toker.nextToken();
            showContextMenu(x, y, id, path);
        } catch (NoSuchElementException nsee) {
        }
    }

    //============== Local classes ===============================

    class SQMenuItem extends JMenuItem {
        private VActionElement m_actionElement;

        public SQMenuItem(String cmd, VActionElement elem) {
            super(cmd);
            m_actionElement = elem;
        }

        VActionElement getActionElement() {
            return m_actionElement;
        }
    }

    class ActionItemListener implements ActionListener {

        @Override
        public void actionPerformed(ActionEvent e) {
            String cmd = e.getActionCommand();
            Object obj = e.getSource();
            VActionElement actionElement = null;
            if (obj instanceof SQMenuItem) { // Should always be true (for now)
                actionElement = ((SQMenuItem)obj).getActionElement();
                cmd = substituteAttributes(cmd, actionElement).trim();
                if (cmd.startsWith("JAVA ")) {
                    cmd = cmd.substring(5).trim(); // Skip leading "JAVA "
                    execCmdInJava(cmd, actionElement);
                } else if (cmd.startsWith("xmmenuaction(`clone`,")) {
                    // NB: Sleazy workaround to handle cloning multiple nodes.
                    sendCloneCommands(cmd);
                } else {
                    Util.sendToVnmr(cmd);
                }
            }
        }

        /**
         * Split a "clone" command into multiple commands to be sent
         * one at a time.
         * E.g., the command:<pre>
         * xmmenuaction(`clone`,`n001 n001 n002 n003`)
         * </pre>
         * is turned into:<pre>
         * xmmenuaction(`clone`,`n001`)
         * xmmenuaction(`clone`,`n002`)
         * xmmenuaction(`clone`,`n003`)
         * </pre>
         * @param cmd The command to be split into multiple commands.
         */
        private void sendCloneCommands(String cmd) {
            String[] cmds = null;
            String[] args = cmd.split("[,`]+");
            if (args.length > 2) {
                String[] nodes = args[2].split(" +");
                cmds = new String[nodes.length - 1];
                for (int i = 0; i < nodes.length - 1; i++) {
                    int j = i + 1;
                    cmds[i] = "xmmenuaction(`clone`,`" + nodes[j]
                           + " " + nodes[j] + "`)";
                }
                new CommandSender(cmds).start();
            }
        }

        /**
         * Substitutes attribute keys in the "cmd" string with the values
         * of the attributes in "actionElement".
         * An attribute key is of the form "<code>$ATTRIBUTE(name)</code>",
         * where "<code>name</code>" is the name of the attribute
         * (e.g., "id", "type", etc.).
         * @param cmd The command string, possibly containing attribute keys.
         * @param actionElement The VActionElement the command came from.
         * @return The modified command, with attribute values substituted.
         */
        private String substituteAttributes(String cmd,
                                            VActionElement actionElement) {
            String pattern1 = "\\$ATTRIBUTE *\\( *";
            String pattern2 = " *\\)";
            String[] substrs = cmd.split(pattern1, 2);
            for ( ; substrs.length == 2; substrs = cmd.split(pattern1, 2)) {
                String[] attr = substrs[1].split(pattern2, 2);
                String attrValue = getAttribute(attr[0], actionElement);
                cmd = substrs[0] + attrValue;
                if (attr.length > 1) {
                    cmd = cmd + attr[1];
                }
            }
            return substrs[0];
        }

        private String getAttribute(String key, VActionElement actionElement) {
            String rtn = "";
            if (actionElement != null) {
                rtn = actionElement.getAttribute(key);
            }
            if (rtn != null && rtn.length() == 0) {
                rtn = null;
            }
            return rtn;
        }

    }

    class SubMenu extends JPopupMenu { 

        ActionListener itemListener;


        public SubMenu() {
            readPersistence();

            itemListener = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    JMenuItem item=(JMenuItem)evt.getSource();
                    String s=evt.getActionCommand();
                    if(s.equals(m_showStatusKey)){
                        showStatus=item.isSelected()?true:false;
                        infopanel.setVisible(showStatus);
                    }
                    else if(s.equals(m_showTimesKey)){
                        mgr.setTimeShown(item.isSelected()?true:false);
                    }
                    else if(s.equals(m_showIconsKey)){
                        mgr.setShowIcons(item.isSelected()?true:false);
                    }
                    else if(s.equals(m_expandNewKey)){
                        mgr.setExpandNew(item.isSelected()?true:false);
                    }
                    else if(s.equals(m_deleteErrorsKey)){
                        deleteErrorNodes();
                    }
                    else if(s.equals(m_confDelKey)){
                        setConfirmDelete(item.isSelected());
                    }
                    writePersistence();
                }
            };
        }

        protected void deleteErrorNodes() {
            String[] list = getErrorActionIds();
            if (list.length > 0) {
                String nodeList = list[0];
                for (String id : list) {
                    nodeList += " " + id;
                }
                String cmd = "xmmenuaction('delete','" + nodeList + "')";
                Util.sendToVnmr(cmd);
            }
        }

        public SubMenu updateMenu() {
            this.removeAll();

            // Show/Hide status bar
            JCheckBoxMenuItem citem;
            /*********
            citem = new JCheckBoxMenuItem(m_showStatusLabel, showStatus);
            citem.setActionCommand(m_showStatusKey);
            citem.addActionListener(itemListener);
            add(citem);
            *********/

            // Expand new nodes option
            citem = new JCheckBoxMenuItem(m_expandNewLabel, mgr.getExpandNew());
            citem.setActionCommand(m_expandNewKey);
            citem.addActionListener(itemListener);
            add(citem);

            // Confirm data deletions option
            if ("Imaging".equals(Util.getUserAppType())) {
                citem = new JCheckBoxMenuItem(m_confDelLabel,
                                              getConfirmDelete());
                citem.setActionCommand(m_confDelKey);
                citem.addActionListener(itemListener);
                add(citem);
            }

            String[] list = getErrorActionIds();
            if (list.length > 0) {
                JMenuItem mitem = new JMenuItem(m_deleteErrorsLabel);
                mitem.addActionListener(itemListener);
                mitem.setActionCommand(m_deleteErrorsKey);
                add(mitem);
            }

            return this;
        }

        /**
         * read persistence file ~/vnmrsys/persistence/QueuePanel
         */
        void readPersistence() {
            BufferedReader in;
            String line;
            String symbol;
            String value;
            StringTokenizer tok;
            String filepath = FileUtil.openPath("USER/PERSISTENCE/"+persistence_file);
            if(filepath==null)
                return;
            try {
                in = new BufferedReader(new FileReader(filepath));
                while ((line = in.readLine()) != null) {
                    tok = new StringTokenizer(line, " ");
                    if (!tok.hasMoreTokens())
                        continue;
                    symbol = tok.nextToken().trim();
                    if (tok.hasMoreTokens()){
                        value = tok.nextToken().trim();
                        boolean state=value.equals("true")?true:false;
                        if(symbol.equals(m_showStatusKey)){
                            // showStatus=state;
                            // infopanel.setVisible(showStatus);
                        }
                        if(symbol.equals(m_showTimesKey))
                            mgr.setTimeShown(state);
                        if(symbol.equals(m_showIconsKey))
                            mgr.setShowIcons(state);
                        if(symbol.equals(m_expandNewKey))
                            mgr.setExpandNew(state);
                        if(symbol.equals(m_confDelKey))
                            setConfirmDelete(state);
                    }
                }
                in.close();
            }
            catch (Exception e) { 
                System.out.println("QueuePanel: problem reading persistence file");
                Messages.writeStackTrace(e);
            }
        }

        /** write persistence file ~/vnmrsys/persistence/QueuePanel */

        void writePersistence() {
            String filepath = FileUtil.savePath("USER/PERSISTENCE/"+persistence_file);

            PrintWriter os;
            try {
                os = new PrintWriter(new FileWriter(filepath));
                String value;
                value=showStatus?"true":"false";
                os.println(m_showStatusKey + " " + value);
                value=mgr.isTimeShown()?"true":"false";
                os.println(m_showTimesKey + " " + value);
                value=mgr.getShowIcons()?"true":"false";
                os.println(m_showIconsKey + " " + value);
                value=mgr.getExpandNew()?"true":"false";
                os.println(m_expandNewKey + " " + value);
                value=getConfirmDelete()?"true":"false";
                os.println(m_confDelKey + " " + value);
                os.close();
            }
            catch(Exception er) {
                System.out.println("Error: could not create "+filepath);
                Messages.writeStackTrace(er);
            }
        }
    } // class SubMenu


    /**
     * This class is specifically for a workaround for cloning multiple nodes.
     * The commands in an array are executed serially.
     * After each command, wait for it to complete before sending the next.
     * VnmrBG sends a message to indicate completion via:
     * <pre>
     * vnmrjcmd('SQ WriteSqDone')
     * </pre>
     * Otherwise, the wait times out in 5 seconds.
     */
    class CommandSender extends Thread {

        String[] mm_cmds;

        /**
         * Construct a CommandSender with the given array of commands.
         * @param commands The commands to be executed.
         */
        CommandSender(String[] commands) {
            mm_cmds = commands;
        }

        @Override
        /**
         * This CommandSender's commands are sent serially to VnmrBG.
         * After each command, wait for it to complete before sending the next.
         * VnmrBG sends a message to set the m_writeSqDone flag via:
         * <code>vnmrjcmd('SQ WriteSqDone')</code>.
         * Otherwise, the wait times out in 5 seconds.
         */
        public void run() {
            for (int i = 0; i < mm_cmds.length; i++) {
                m_writeSqDone = false;
                Util.sendToVnmr(mm_cmds[i]);
                for (int j = 0; j < 50; j++) {
                    if (m_writeSqDone) {
                        m_writeSqDone = false;
                        break;
                    }
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException e) {
                    }
                }
            }
        }
    }


    public void execCmdInJava(String cmd, VActionElement actionElement) {
        // TODO Auto-generated method stub
        
    }

} // class QueuePanel
