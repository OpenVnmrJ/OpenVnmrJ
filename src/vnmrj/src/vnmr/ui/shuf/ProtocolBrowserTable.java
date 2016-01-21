/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.awt.*;
import java.awt.datatransfer.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.io.IOException;
import java.util.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;


/**
 * The ProtocolBrowserTable displays shuffler-query results.
 *
 */
public class ProtocolBrowserTable extends JTable implements StatementListener, VTooltipIF {
    /** session share */
    private static SessionShare sshare;
    /** table header */
    private JTableHeader header;
    /** height for a line using header's font */
    private int headerStringHeight;
    /** attribute popup menu */
    private JMenu attribMenu=null;
    /** data model */
    private ProtocolBrowserDataModel dataModel;
    /** column model */
    private TableColumnModel colModel;
    /** Save column selected for uses after selection is made. */
    private int columnSelected;
    /** Row where mouse is pressed down */
    private int mouseDownRow;
    /** Column where mouse is pressed down */
    private int mouseDownCol;
    /** object type */
    public String objType;
    /** fullpath if opening study card */
    /** I hope making this static does not cause trouble in the future.
     * I am making it static so that a Protocol Tab will have access to the
     * fullpath for a SC Tab.
     */
    static public String fullpath=null;
    /** Is this created for a Study Card? */
    public boolean studyCard=false;

    static boolean tceObtained=false;
    private static DataFlavor dataFlavor = null;

    /**
     * constructor
     */
    public ProtocolBrowserTable(SessionShare sshare, ProtocolKeywords keywords, 
                                String objType, String fullpath) {
        this.sshare = sshare;
        this.objType = objType;
        if(fullpath != null)
            this.fullpath = fullpath;
        if(fullpath != null && fullpath.length() > 0)
            studyCard = true;

        dataModel = new ProtocolBrowserDataModel(sshare, keywords, objType, fullpath);
        setModel(dataModel);
        dataFlavor = LocalRefSelection.LOCALREF_FLAVOR;

        header = getTableHeader();
        // Do not allow rearranging of header columns.
        header.setReorderingAllowed(false);

        headerStringHeight =
            header.getFontMetrics(header.getFont()).getHeight();

        // set various attributes
        setShowGrid(false);
        setIntercellSpacing(new Dimension(0, 0));
        setSelectionBackground(Global.HIGHLIGHTCOLOR);
        setCellSelectionEnabled(false);
    
        // For some unknown reason, the default height is one pixel too short and
        // letters like 'g' are cut off on the bottom.  Increasing the size by
        // one pixel seems to fix the problem.
        int height = getRowHeight();
        setRowHeight(height +1);
    
        header.setResizingAllowed(true);
//        header.setForeground(Color.blue);
       
        setMultipleSelectionMode();
        
        setRowSelectionAllowed(true);

        // turn off auto scroll event
        setAutoscrolls(false);

        setDragEnabled(true);

        TransferHandler transferHandler = new PBTransferHandler();
        setTransferHandler(transferHandler);

        // set cell renderers
        colModel = header.getColumnModel();
        int numCols = colModel.getColumnCount();
        for (int i = 0; i < numCols; i++) {
            TableColumn col = colModel.getColumn(i);
            col.setCellRenderer(new ProtocolBrowserCellRenderer(this));
            col.setHeaderRenderer(new HeaderValRenderer(header));
            col.setHeaderValue(dataModel.getHeaderValue(i));
        }


        // install behaviors
        // Since we (ProtocolBrowserTable) implements StatementListener, we add ourselves
        // to statementHistory's listener list with 'this'.
        LocatorHistoryList lhl = sshare.getLocatorHistoryList();
        if(lhl!=null)
            lhl.addAllStatementListeners(this);

        header.addMouseListener(new MouseAdapter() {
                public void mouseClicked(MouseEvent evt) {
                    JTableHeader hdr = ProtocolBrowserTable.this.header;
                    int y = evt.getY();
                    int x = evt.getX();
                    int colIndex =
                        hdr.columnAtPoint(new Point(evt.getX(), y));
                    
                    // At this point we know which column was clicked on,
                    // namely, colIndex.  However, when actionPerformed()
                    // is called after the selection we do not have
                    // that information.  So, save it now for use
                    // by actionPerformed().
                    ProtocolBrowserTable.this.columnSelected = colIndex;

                    int btn = evt.getButton();

                    if (!studyCard && btn == MouseEvent.BUTTON1) {
                        // Come here when for button 1 to set this column as
                        // the sorting column or to change the sort direction
                        ProtocolKeywords keywords = ProtocolBrowser.getKeywords();
                        TableColumn col =
                            ProtocolBrowserTable.this.colModel.getColumn(colIndex);
                        HeaderValue hValue =
                            (HeaderValue)col.getHeaderValue();
                        String headerText = hValue.getText();
                        if(ProtocolBrowserTable.this.objType.equals(Shuf.DB_PROTOCOL)) {
                            // Protocol Tab sort change
                            keywords.setPSortKeyword(headerText);
                            // Is this column already the sort (keyed) column?
                            if (hValue.isKeyed()) {
                                // Reverse the sort direction
                                hValue.setAscending(!hValue.isAscending());
                                // Set the keywords info to match the header
                                if(hValue.isAscending())
                                    keywords.setPSortDirection(Shuf.DB_SORT_ASCENDING);
                                else
                                    keywords.setPSortDirection(Shuf.DB_SORT_DESCENDING);
                            }
                            // New sort column
                            else {
                                ProtocolBrowserTable.this.dataModel.setKeyedHeader(colIndex);
                                // Set the keywords info to match the header
                                if(hValue.isAscending())
                                    keywords.setPSortDirection(Shuf.DB_SORT_ASCENDING);
                                else
                                    keywords.setPSortDirection(Shuf.DB_SORT_DESCENDING);
                            }
                        }
                        else {
                            // Data Tab sort change
                            keywords.setDSortKeyword(headerText);
                            if (hValue.isKeyed()) {
                                hValue.setAscending(!hValue.isAscending());
                                if(hValue.isAscending())
                                    keywords.setDSortDirection(Shuf.DB_SORT_ASCENDING);
                                else
                                    keywords.setDSortDirection(Shuf.DB_SORT_DESCENDING);
                            }
                            else {
                                ProtocolBrowserTable.this.dataModel.setKeyedHeader(colIndex);
                            }
                         }

                        // Save the update keywords object
                        ProtocolBrowser.setKeywords(keywords);
                        
                        // Update the header gui
                        header.repaint();
                        
                        // Update the Table
                        if(ProtocolBrowserTable.this.objType.equals(Shuf.DB_PROTOCOL))
                            ProtocolBrowser.doDBSearch(keywords, ProtocolBrowserTable.this.objType);
                        else {
                            String protocolFilename = ProtocolBrowser.getProtocolFilename();
                            ProtocolBrowser.doDBSearch(keywords, ProtocolBrowserTable.this.objType,
                                    protocolFilename);
                        }

                    }
                    else if (btn == MouseEvent.BUTTON3) {
                        // Come here to bring up a menu to choose a new column
                        // attribute.  
                        int numCols = colModel.getColumnCount();

                        if(attribMenu == null) {
                            // Create the JMenu with all possible keywords
                            // for adding a new column as JMenuItems
                            attribMenu = createAttrMenu (ProtocolBrowserTable.this.objType);
                        }
                        JPopupMenu mainMenu = new JPopupMenu();

                        JMenuItem deleteItem = new JMenuItem("Delete Column");
                        deleteItem.addActionListener(deleteListener);

                        mainMenu.add(attribMenu);
                        mainMenu.add(deleteItem);


                        hdr.add(mainMenu);
                        mainMenu.show(hdr, x, y);
// Allow menu enables on all columns.  Note, it will still append at the right most
//                        if (colIndex == numCols -1) {
//                            // Only enable the append menu for the far right column
//                            attribMenu.setEnabled(true);
//                        }
//                        else {
//                            attribMenu.setEnabled(false);
//                        }


                        // We need to only allow the delete menu for keyword
                        // with a type of "protocol-display".

                        // Find out what the type is for the keyword who's
                        // name is in this column
                        ProtocolKeywords keywords = ProtocolBrowser.getKeywords();
                        TableColumn col =
                            ProtocolBrowserTable.this.colModel.getColumn(colIndex);
                        HeaderValue hValue = (HeaderValue)col.getHeaderValue();

                        // Name for this column header
                        String name = hValue.getText();
                        
                        // Don't allow deleting of "filename"
                        if(name.equals("filename")) {
                            deleteItem.setEnabled(false);
                        }
                        else {
                            deleteItem.setEnabled(true);
                        }
                    }
                }
            });

        // add MousesAdapter to the JTable
        addMouseListener(new MouseAdapter() {
                public void mouseClicked(MouseEvent evt) {
                    Point pos = new Point(evt.getX(), evt.getY());
                    int colIndex = columnAtPoint(pos);
                    if(evt.getButton() == MouseEvent.BUTTON1) {
                        int clicks = evt.getClickCount();
                        if(clicks == 2) {
                            // We have a double click
                            // Be sure the Shift nor Cntrl keys were down.
                            if(evt.isControlDown() || evt.isShiftDown())
                                return;


                            // getID(row) gives the unique filename 
                            // for this row.
                            ID id = (ID) dataModel.getID(mouseDownRow);
                            if(id == null)
                                return;
                            String hostFullpath = id.getName();
//                            SessionShare ssshare = ProtocolBrowserTable.getSshare();
 //                           ShufflerService shufflerService =
 //                               ssshare.shufflerService();
 //                           ShufflerItem item =
 //                               shufflerService.getShufflerItem(hostFullpath);
                            
                            // Get the fullpath without the host
                            int index = hostFullpath.indexOf(":");
                            String fullpath = hostFullpath.substring(index +1);
                            ShufflerItem item = new ShufflerItem(fullpath, 
                                                                 "PROTOCOLBROWSER");

                            if(item == null)
                                return;

                            String target = "Default";

                            if(item.objType.equals(Shuf.DB_PROTOCOL) ) {
                                target="StudyQueue";
                            }
                            // If .img or Study, set target to ReviewViewport
                            else if(Util.getRQPanel() != null) {
                               if(item.objType.equals(Shuf.DB_STUDY) ||
                                       item.objType.equals(Shuf.DB_IMAGE_DIR)) {
                                    target="ReviewViewport";
                               }
                            }
                               

                            if(item.fileExists()) {
                                // Decide what is to be done with this item.
                                // Default target to Default, and
                                // action is double click.
                                item.actOnThisItem(target, "DoubleClick", 
                                                   "");
                            }
                            else {
                                Messages.postWarning(hostFullpath
                                                     + " Does Not Exist or is not Mounted.");
                            }
                        }
                    }
                    // Button3 Click
                    else if(evt.getButton() == MouseEvent.BUTTON3) {

                        // The menu is expected to call the macro locaction
                        // so set up the args into a string for that macro
                        // Namely: action, objtype, host, filepath, target command

                        // Get the hostFullpath of the item selected
                        ID id = (ID) dataModel.getID(mouseDownRow);
                        if(id == null)
                            return;
                        String hostFullpath = id.getName();
                    
                        // Get the host from the front of the name
                        int index = hostFullpath.indexOf(":");
                        String host = hostFullpath.substring(0, index);
                        String fullpath = hostFullpath.substring(index +1);
                        // Get the mountpath (ie local)
                        String mPath = MountPaths.getMountPath(hostFullpath);

                        String objType = FillDBManager.getType(mPath);
                        FillDBManager dbm = FillDBManager.fillDBManager;
                        // Determine if this is "study card" type
                        String protocolType = "";
                        // Note: If this fullpath is from the Study Card Tab,
                        // then it is not in the DB and the attribute will not
                        // be found here, thus just set it below
                        if(objType.equals(Shuf.DB_PROTOCOL))
                            protocolType = dbm.getAttributeValue(objType, fullpath, 
                                                                 host, "type");
                        if(studyCard)
                            protocolType = "study card protocol";

                        // Given mpath (the fullpath), we need the name without
                        // the path and without the suffix.
                        UNFile file = new UNFile(mPath);
                        String filename = file.getName();
                        // We have the filename, now remove the suffix.
                        index = filename.lastIndexOf(".");
                        if(index >= 0)
                            filename = filename.substring(0, index);
                        
                        // Treat like a double click
                        String action = "DoubleClick";
                        

                        // If objType is data (study or .img) use different
                        // menu and open command
                        if(objType.equals(Shuf.DB_STUDY) ) {
                            String cmd = "loadStudyData";
                            String arg = "\'" + cmd + "\', \'" + mPath + "\', \'" 
                                        + action + "\'";
                            
                            if(DebugOutput.isSetFor("protocolbrowser"))
                                Messages.postDebug("Protocol Browser arg sent to data_item.xml to Open Study Data:\n   "
                                                  + arg);

                            // Put up the appropriate menu
// Lada asked to not have the menu.  Try it without it.
//                            ContextMenu.popupMenu("data_item", arg, 
//                                                  (JComponent)ProtocolBrowserTable.this.getParent(),
//                                                  position);
                        }
                        else if(objType.equals(Shuf.DB_IMAGE_DIR) || 
                                objType.equals(Shuf.DB_IMAGE_FILE) ||
                                objType.equals(Shuf.DB_VNMR_DATA)) {
                            String cmd = "loadData";
                            String arg = "\'" + cmd + "\', \'" + mPath + "\', \'" 
                            + action + "\'";
                            
                            if(DebugOutput.isSetFor("protocolbrowser"))
                                Messages.postDebug("Protocol Browser arg sent to data_item.xml to Open Image Data:\n   "
                                                  + arg);

                            // Put up the appropriate menu
// Lada asked to not have the menu.  Try it without it.
//                            ContextMenu.popupMenu("data_item", arg, 
//                                                  (JComponent)ProtocolBrowserTable.this.getParent(),
//                                                  position);

                        }
                        else if(objType.equals(Shuf.DB_PROTOCOL) && protocolType != null && 
                                protocolType.equals("study card")) {
                            String target = "StudyQueue";
                            String arg = "\'" + action + "\', \'" + objType 
                            + "\', \'" + host + "\', \'" + mPath + "\', \'"
                            + target + "\', \'\', \'" + filename + "\'";
                            
                            if(DebugOutput.isSetFor("protocolbrowser"))
                                Messages.postDebug("Protocol Browser arg sent to study_card_item.xml to Open Study Card:\n   "
                                                  + arg);
                            ContextMenu.popupMenu("study_card_item", arg, 
                                    ProtocolBrowserTable.this,
                                    pos);

                        }
                        else if(protocolType != null && protocolType.equals("study card protocol")) {
                            String target = "StudyQueue";
                            String arg = "\'" + action + "\', \'" + objType 
                                + "\', \'" + host + "\', \'" + mPath + "\', \'"
                                + target + "\', \'\', \'" + filename + "\'";
                            // Put up the appropriate menu
                            ContextMenu.popupMenu("sc_protocol_item", arg, 
                                                  ProtocolBrowserTable.this,
                                                  pos);
                        }
                        else if(protocolType != null) {
                            String target = "StudyQueue";
                            String arg = "\'" + action + "\', \'" + objType 
                                + "\', \'" + host + "\', \'" + mPath + "\', \'"
                                + target + "\', \'\', \'" + filename + "\'";
                            // Put up the appropriate menu
                            ContextMenu.popupMenu("protocol_item", arg, 
                                                  ProtocolBrowserTable.this,
                                                  pos);
                        }

                    }
                }

                public void mousePressed(MouseEvent evt) {

                    // This was taking on the order of .5 sec, so stop
                    // doing it at startup time and wait until first clicked.
                    if(!tceObtained) {
                        // I determined the arg of Object.class by looking at
                        // the source code in Jtable.java.createDefaultEditors()
                        DefaultCellEditor tce =
                            (DefaultCellEditor) getDefaultEditor(Object.class);
                        // Set the click count of the editor to 1,
                        // it defaults to 2.
                        tce.setClickCountToStart(1);
                        tceObtained = true;
                    }

                    // Find out where the mouse is right now.
                    Point p = new Point(evt.getX(), evt.getY());
                    int row = rowAtPoint(p);
                    int col = columnAtPoint(p);

                    
                    // Save this row for use in dragGestureRecognized
                    // Else the selected row ends up changing beyond my
                    // control as the mouse is dragged.  I think this is
                    // a Java bug.
                    mouseDownRow = row;
                    mouseDownCol = col;

                    if(evt.getButton() == MouseEvent.BUTTON3) {
                        // The following 4 lines not used yet, but they will give
                        // the file when btn 3 is pressed (not clicked)
                        ID id = (ID) dataModel.getID(mouseDownRow);
                        if(id == null)
                            return;
                        String hostFullpath = id.getName();
                    }

                }
            });
        // End of addMouseListener()


        String version = System.getProperty("java.specification.version");
        // make this object a drag source
        // If we are executing under Java version 1.2x, use this code
        //if(version.indexOf("1.2") != -1) {
//        dragSource = new DragSource();
//        dragSource.createDefaultDragGestureRecognizer(this,
//                                                      DnDConstants.ACTION_COPY,
//                                                      new ShufDragGestureListener());
        //}
        //else {
        // For java 1.3 and higher use this code
        // use VDnDHandler to replace java DnD
        //VDnDHandler dh = new VDnDHandler();
        //dh.setDragSource(this);
        //}

        if (Util.isNativeGraphics()) {
            ToolTipManager.sharedInstance().unregisterComponent(this);
            VTipManager.sharedInstance().registerComponent(this);
        }

    } // ProtocolBrowserTable()


    // Delete a column
    ActionListener deleteListener = new ActionListener() {
        public void actionPerformed(ActionEvent evt) {
            int colIndex = ProtocolBrowserTable.this.columnSelected;
            ProtocolBrowser protocolBrowser = ExpPanel.getProtocolBrowser();
            ProtocolKeywords keywords = protocolBrowser.getKeywords();

            // Get the keyword named in the header
            TableColumn col = ProtocolBrowserTable.this.colModel
                    .getColumn(colIndex);
            HeaderValue hValue = (HeaderValue) col.getHeaderValue();
            String keywordName = hValue.getText();

            keywords.removeAKeyword(keywordName);
            protocolBrowser.setKeywords(keywords);

            // Update (recreate) the result panel
            if(studyCard) {
                // Since "keywords" is static and used for both SC and Protocol
                // Tabs, we need to update both of them after a change.
                protocolBrowser.updateResultPanel();
               
                protocolBrowser.updateStudyCardPanel(fullpath);
            }
            else if(objType.equals(Shuf.DB_PROTOCOL)) {
                // Since "keywords" is static and used for both SC and Protocol
                // Tabs, we need to update both of them after a change.
                protocolBrowser.updateStudyCardPanel(fullpath);
                
                protocolBrowser.updateResultPanel();
            }
            else
                protocolBrowser.updateDataPanel();
            
            // Update the persistence file
            keywords.writePersistenceFile();
        }
    };

    private void debugDnD(String msg){
        if(DebugOutput.isSetFor("dnd"))
            Messages.postDebug("RT: " + msg);
    }

    public void setTooltip(String str) { }

    public String getTooltip(MouseEvent ev) {
        return getToolTipText(ev);
    }

    /**
     * notification of a new statement
     * 
     * @param statement
     *            new statement
     */
    public void newStatement(Hashtable statement) {
        String returnObjType = (String) statement.get("ObjectType");

        clearSelection();
        attribMenu = createAttrMenu(returnObjType);
        // Initiate the actual database search via thread
        StartSearch searchThread = new StartSearch(statement);
        searchThread.setName("Shuffler Search");
        searchThread.start();

        // Update menus
        StatementDefinition curStatement = sshare.getCurrentStatementType();
        // curStatement.updateValues(statement, true);

    } // newStatement()

    /**
     * notification that back movability has changed
     * @param state boolean
     */
    public void backMovabilityChanged(boolean state) {}

    /**
     * notification that forward movability has changed
     * @param state boolean
     */
    public void forwardMovabilityChanged(boolean state) {}

    /**
     * notification of a change in the list of saved statements
     * @param
     */
    public void saveListChanged() {}

    public Transferable getTransferData() {
        int row = mouseDownRow;

        if (dataModel == null) {
            return null;
        }
        ID id = (ID) dataModel.getID(row);

        if(id == null || id.toString().startsWith("-----"))
            return null;

        String hostFullpath = id.getName();
        ShufflerService shufflerService = sshare.shufflerService();
        ShufflerItem item = shufflerService.getShufflerItem(hostFullpath);

        if(item == null)
            return null;

        if(isCellEditable(mouseDownRow, mouseDownCol))
            return null;

        clearSelection();
        getSelectionModel().setSelectionInterval(row, row);

        item.source = "LOCATOR";

        item.sourceRow = row;

        LocalRefSelection ref = new LocalRefSelection(item);

        return ref;
    } // getTransferData



    /************************************************** <pre>
     * Summary:  Update one of the column headers to a new attribute.
     </pre> **************************************************/
    public void updateHeader(String newHeaderString, int column) {

        dataModel.setAttribute(column, newHeaderString);

        // updateAllHeaders() is called also in the chain of
        // events, so we do not need to call setHeaderValue() here.
    }

    /************************************************** <pre>
     * Summary:  Update all of the column headers to new attributes.
     </pre> **************************************************/
    public void updateAllHeaders(Hashtable statement) {
        String[] headers;
        HeaderValue hValue;

        headers = (String[])statement.get("headers");

        for(int i=0; i < 3; i++) {
            dataModel.setAttribute(i+1, headers[i]);

            // To make this take effect, we have to assign this
            // HeaderValue to TableColumn.
            hValue = dataModel.getHeaderValue(i+1);

            // The sort column is set in headers[3], set it for dataModel.
            if(headers[3].equals(headers[i])) {
                dataModel.setKeyedHeader(i+1);
                if(headers[4].equals("ASC"))
                    hValue.setAscending(true);
                else
                    hValue.setAscending(false);
            }

            TableColumn col = colModel.getColumn(i+1);
            col.setHeaderValue(hValue);
        }
        ProtocolBrowserTable.this.header.repaint();
    }


    /************************************************** <pre>
     * Summary: Static method to return sshare.
     *
     *  This used to return the local copy.  However, it the locator has
     *  not been started and other methods need sshare, it will be null
     *  Thus for compatibility with all the code calling here to get sshare,
     *  we will just have this method call the one in Util.java
     *
     </pre> **************************************************/
    static public SessionShare getSshare() {
        return Util.getSessionShare();
    }



    static String curMenuObjType = new String("");
    static JMenu curMenu = new JMenu("Append New Column");
    static final int colLength = 30;

    // For internal use to get a new menu.  Only create a new
    // one if the objType has changed.  Else, just return the
    // current one.
    private JMenu createAttrMenu (String objType) {
        JMenuItem item;
        int numAttrs;
        String attr;
        int numItems=0;
          
        if(curMenuObjType.equals(objType) && attribMenu != null)
            return curMenu;

        // Set the static copy for comparison next time.
        curMenuObjType = objType;

        ShufDBManager dbManager = ShufDBManager.getdbManager();

        curMenu = new JMenu("Append New Column");

        // attribute menu
        ActionListener attribListener = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    SessionShare sshare2 = ProtocolBrowserTable.getSshare();
                    // Set the new values into the buttons.
                    int colIndex = ProtocolBrowserTable.this.columnSelected;
                    
                    // evt.actionCommand gives selected new keyword
                    String keywordName = evt.getActionCommand();
                    
                    // Add this to the keywords list as a type of protocol-display
                    // if protocol and data-display if not protocol
                    ProtocolBrowser protocolBrowser = ExpPanel.getProtocolBrowser();
                    ProtocolKeywords keywords = protocolBrowser.getKeywords();
                    if(studyCard) {
                        keywords.addAKeyword(keywordName, "All", "studycard-display");
                        // Save the modified list
                        protocolBrowser.setKeywords(keywords);
                        
                        // Write an updated persistence file
                        keywords.writePersistenceFile();
                        
                        // Since "keywords" is static and used for both SC and Protocol
                        // Tabs, we need to update both of them after a change.
                        protocolBrowser.updateResultPanel();
                        protocolBrowser.updateStudyCardPanel(fullpath);
                    }
                    else if(curMenuObjType.equals(Shuf.DB_PROTOCOL)) {
                        keywords.addAKeyword(keywordName, "All", "protocol-display");
                        // Save the modified list
                        protocolBrowser.setKeywords(keywords);
                        
                        // Write an updated persistence file
                        keywords.writePersistenceFile();
                        
                        // Since "keywords" is static and used for both SC and Protocol
                        // Tabs, we need to update both of them after a change.
                        protocolBrowser.updateStudyCardPanel(fullpath);
                        // Update (recreate) the result panel
                        protocolBrowser.updateResultPanel();
                    }
                    else {
                        keywords.addAKeyword(keywordName, "All", "data-display");
                        // Save the modified list
                        protocolBrowser.setKeywords(keywords);
                        
                        // Write an updated persistence file
                        keywords.writePersistenceFile();
                        
                         // Update (recreate) the Data panel
                        protocolBrowser.updateDataPanel();
                    }
                }
            };

        // Get the list of attributes to put into the menu for choosing headers
        ArrayList attr_list = dbManager.attrList.getAttrNamesLimited(objType);
        if (attr_list == null)
            return curMenu;
        
        ProtocolBrowser protocolBrowser = ExpPanel.getProtocolBrowser();
        ProtocolKeywords keywords = protocolBrowser.getKeywords();

        // Disallow array type attributes.  We cannot have them as column
        // headers.  Also keep arch_lock out of the list and tags
        // If array types are allowed, they cannot be sorted on.
        // Trying to sort on them gives an sql error.  If we ever allow
        // arrays in columns, we have to disallow that column for sorting.
        // Also eliminate any attributes that are already in the keyword list.
        for (int i = 0; i < attr_list.size(); i++) {
            attr = (String) attr_list.get(i);
            int isArray = dbManager.isParamArrayType(objType, attr);
            if(isArray == 0 && !attr.startsWith("arch_") && !attr.startsWith("tag")) {
                // Create and add a JMenuItem to the curMenu
                item = curMenu.add(attr);
                numItems++;
                item.addActionListener(attribListener);
            }
        }


        // Set up for multiple column menu.  Since
        if(numItems >= colLength)
            numAttrs = colLength;
        // Trap for zero length list
        else if(numItems == 0)
            numAttrs = 1;
        else
            numAttrs = numItems;
        
                
        // Setting a Layout Manager does not work for JMenu so the following
        // was done.
        // If the menu is too long for the screen, this will allow scrolling
        // up and down to access all items in the menu.  Set to 35 rows as max
        // for the display
        ScrollingMenu.setScrollerFor(curMenu, 35);
        
        return curMenu;
    }

    public ProtocolBrowserDataModel getdataModel() {
        return dataModel;
    }

    public JTableHeader getHeader()
    {
        return header;
    }


    /************************************************** <pre>
     * Summary: Set Shuffler to Single selection mode.
     *
     *    This is the normal mode when the tag popup is NOT up.  The reason
     *    for keeping it in Single selection mode, is that sometimes when
     *    you click and drag and entry, you end up with multiple items
     *    selected even though you clicked on only one.  I think this is
     *    a bug, and the work around is to force only one selection
     *    at a time.
     *
     </pre> **************************************************/
    public void setSingleSelectionMode() {
        setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
    }


    /************************************************** <pre>
     * Summary: Set Shuffler to Multiple selection mode..
     *
     *    This is the normal mode when the tag popup IS up.  The reason
     *    for keeping it in Single selection mode normally, is that sometimes
     *    when you click and drag and entry, you end up with multiple items
     *    selected even though you clicked on only one.  I think this is
     *    a bug, and the work around is to force only one selection
     *    at a time.  When the tag pupup is up, we need to be able to
     *    select multiple entries.
     *
     </pre> **************************************************/
    public void setMultipleSelectionMode() {
        setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
    }

    /************************************************** <pre>
     * Summary: Return a list of all currently selected shuffler rows.
     *
     *
     </pre> **************************************************/
    public ArrayList <String> getSelectionList() {
        ID id;
        String hostFullpath;
        int[] indices;
        ArrayList <String>list=null;

        list = new ArrayList<String>();

        indices = getSelectedRows();
        for(int i=0; i < indices.length; i++) {
            id = (ID) dataModel.getID(indices[i]);
            if(id == null)
                return list;
            hostFullpath = id.getName();
            // Macros will have colon as first char, skip it.
            if(hostFullpath.charAt(0) == ':')
                hostFullpath = hostFullpath.substring(1);
            list.add(hostFullpath);

        }

        return list;
    }

    /************************************************** <pre>
     * Summary: Set the entries given to be highlighted.
     *
     *    The arg 'entries' is a list of host_fullpath strings.
     *    We need to go thru the list and determine what row each
     *    entry is on, and then highlight that row.
     </pre> **************************************************/
    public void setHighlightSelections(ArrayList entries) {
        int row;
        ID id;
        String hostFullpath;
        ListSelectionModel lsmodel;
        // the entries are host_fullpath entries
        // We need to go thru each row and determine see if it is
        // in the entry list and if so, then highlight that row.

        // Clear any current selections
        clearSelection();

        lsmodel = getSelectionModel();

        for(row=0; row < getRowCount(); row++) {
            id = (ID) dataModel.getID(row);
            if(id == null)
                return;

            hostFullpath = id.getName();
            // Macros have colon as first char, skip it.
            if(hostFullpath.charAt(0) == ':')
                hostFullpath = hostFullpath.substring(1);
            if(entries != null && entries.contains(hostFullpath)) {
                lsmodel.addSelectionInterval(row, row);
            }
        }
    }

    // Creating a PBTransferHandler basically just saves the dragged
    // ShufflerItem list.
    class PBTransferHandler extends TransferHandler {

        // Tell caller what mode we want to allow
        public int getSourceActions(JComponent c) {
            return TransferHandler.COPY;
        }

        // Create and return the transferable which is an ArrayList of
        // ShufflerItem's
        public Transferable createTransferable(JComponent comp)
        {
            // Get the list of hostFullpath items that were dragged
            ArrayList<String> list = getSelectionList();

            // Create a list of ShufflerItems from the hostFullpath list
            ArrayList<ShufflerItem> items = new ArrayList<ShufflerItem>();
            for(int i=0; i < list.size(); i++) {
                String hostFullpath = list.get(i);
                // Get the fullpath without the host
                int index = hostFullpath.indexOf(":");
                String fullp = hostFullpath.substring(index +1);
                ShufflerItem item = new ShufflerItem(fullp, "PROTOCOLBROWSER");

                if(item == null)
                    continue;

                items.add(item);
            }

            PBTransferable tf = new PBTransferable(items);

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
            ID id;
            String hostFullpath;
            int[] indices;
            ArrayList list=null;

            list = new ArrayList();

            indices = getSelectedRows();
            for(int i=0; i < indices.length; i++) {
                id = (ID) dataModel.getID(indices[i]);
                if(id == null)
                    return list;
                hostFullpath = id.getName();
                // Macros will have colon as first char, skip it.
                if(hostFullpath.charAt(0) == ':')
                    hostFullpath = hostFullpath.substring(1);
                list.add(hostFullpath);

            }

            return list;
        }


    }

    class PBTransferable implements Transferable {
        Object componentToPass;
        
        public PBTransferable(Object comp) {
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


} // class ProtocolBrowserTable
