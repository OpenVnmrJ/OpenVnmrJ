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
 * The ResultTable displays shuffler-query results.
 *
 */
public class ResultTable extends JTable implements StatementListener, VTooltipIF {
    /** session share */
    private static SessionShare sshare;
    /** table header */
    private JTableHeader header;
    /** height for a line using header's font */
    private int headerStringHeight;
    /** attribute popup menu */
    private JPopupMenu attribPopup=null;
    /** data model */
    private ResultDataModel dataModel;
    /** column model */
    private TableColumnModel colModel;
    /** drag source */
    private DragSource dragSource;
    /** Save column selected for uses after selection is made. */
    private int columnSelected;
    /** Row where mouse is pressed down */
    private int mouseDownRow;
    /** Column where mouse is pressed down */
    private int mouseDownCol;
    /** items selected upon mouse down */
    private ArrayList <String> mouseDownList;
    private static DataFlavor dataFlavor = null;


    static boolean tceObtained=false;

    /**
     * constructor
     * @param sshare session share
     */
    public ResultTable(SessionShare sshare) {
	this.sshare = sshare;
	dataModel = new ResultDataModel(sshare);
	setModel(dataModel);

	header = getTableHeader();
        // Do not allow rearranging of header columns.
        header.setReorderingAllowed(false);

	headerStringHeight =
	    header.getFontMetrics(header.getFont()).getHeight();
	
	setDragEnabled(true);
    dataFlavor = LocalRefSelection.LOCALREF_FLAVOR;
    TransferHandler transferHandler = new ResultTransferHandler();
    this.setTransferHandler(transferHandler);

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
//	header.setForeground(Color.blue);
	// Allow only a single selection
	// For tags we will need to allow MULTIPLE_INTERVAL_SELECTION
	// If I leave it Multiple all the time, we get strange
	// selections when dragging from the table.
	// Hopefully, we can change the ListSelectionModel JUST while
	// we have the tags popup up, and then change it back to Single.
	//
	// 11/17/99, It turns out that setting the selected row with
	// mouseDownRow and setSelectionInterval() seems to stop the
	// multiple selection problem and thus we do not have to set it
	// to single selection here.  BUT, since we do not allow dragging
	// and dropping multiple things to the holding area nor the trash,
	// I will keep the setting here for Single selection to keep the
	// user from thinking he/she can do multiple selection drags.
	//
	// 2/24/00 Now I need Multiple selection for the Tag menu and
	// panel so set it.
	setMultipleSelectionMode();

	setRowSelectionAllowed(true);

	// turn off auto drag event
	setAutoscrolls(false);

	// set cell renderers
	colModel = header.getColumnModel();
	int numCols = colModel.getColumnCount();
	for (int i = 0; i < numCols; i++) {
	    TableColumn col = colModel.getColumn(i);
	    col.setCellRenderer(new ShufCellRenderer(this));
	    col.setHeaderRenderer(new HeaderValRenderer(header));
	    col.setHeaderValue(dataModel.getHeaderValue(i));
	}

	// set width of lock column
	int lockWidth = HeaderValRenderer.getLockWidth();
	TableColumn col0 = colModel.getColumn(0);
	col0.setPreferredWidth(lockWidth);
	col0.setMinWidth(lockWidth);
	col0.setMaxWidth(lockWidth);

	// install behaviors
	// Since we (ResultTable) implements StatementListener, we add ourselves
	// to statementHistory's listener list with 'this'.
	LocatorHistoryList lhl = sshare.getLocatorHistoryList();
	if(lhl!=null)
		lhl.addAllStatementListeners(this);

	header.addMouseListener(new MouseAdapter() {
	    public void mouseClicked(MouseEvent evt) {
		JTableHeader hdr = ResultTable.this.header;
		int y = evt.getY();
		int colIndex =
		    hdr.columnAtPoint(new Point(evt.getX(), y));

                int btn = evt.getButton();

		if (btn == MouseEvent.BUTTON1) {
		    // Come here when for button 1 to set this column as
                    // the sorting column or to change the sort direction
		    SessionShare sshare2 = ResultTable.getSshare();
		    TableColumn col =
			ResultTable.this.colModel.getColumn(colIndex);
		    HeaderValue hValue =
			(HeaderValue)col.getHeaderValue();
		    if (hValue.isKeyed()) {
			hValue.setAscending(!hValue.isAscending());
		    }
		    else {
			ResultTable.this.dataModel.setKeyedHeader(colIndex);
		    }
		    // Update the header gui
		    header.repaint();

		    // Update the headers into the statement hashtable
		    // The append will cause a new search
		    sshare2.statementHistory().append("headers",
					   dataModel.getHeaderInfo(colModel));

		}
		else if (btn == MouseEvent.BUTTON3) {
		    // Come here to bring up a menu to choose a new column
                    // attribute.
		    if (colIndex >= 1) {
			// At this point we know which column was clicked on,
			// namely, colIndex.  However, when actionPerformed()
			// is called after the selection we do not have
			// that information.  So, save it now for use
			// by actionPerformed().
			ResultTable.this.columnSelected = colIndex;

			Rectangle rect = hdr.getHeaderRect(colIndex);

                        if(attribPopup == null) {
                            attribPopup = createPopupMenu (Shuf.DB_VNMR_DATA);
                        }
			ResultTable.this.attribPopup.
			    show(hdr, rect.x, rect.height);
		    }
		}
	    }
            // If mouse is released, it may have been a drag to change the
            // width of a column.  Get the widths and be sure they are
            // set correctly in the current statement.
            public void mouseReleased(MouseEvent evt) {
                TableColumn col;
                double colWidth0;
                double colWidth1;
                double colWidth2;
                double colWidth3;
                double colWidthTotal;
		JTableHeader hdr = ResultTable.this.header;

                // Get the total width of all 4 columns
                colWidthTotal = colModel.getTotalColumnWidth();

                // Get the width of each of the columns
                col = colModel.getColumn(0);
                colWidth0 = col.getWidth();
                col = colModel.getColumn(1);
                colWidth1 = col.getWidth();
                col = colModel.getColumn(2);
                colWidth2 = col.getWidth();
                col = colModel.getColumn(3);
                colWidth3 = col.getWidth();

                // Convert widths from pixels fraction of colWidthTotal
                colWidth0 = colWidth0 / colWidthTotal;
                colWidth1 = colWidth1 / colWidthTotal;
                colWidth2 = colWidth2 / colWidthTotal;
                colWidth3 = colWidth3 / colWidthTotal;

                SessionShare sshare2 = ResultTable.getSshare();
                StatementHistory history = sshare2.statementHistory();
                history.updateCurStatementWidth(colWidth0, colWidth1,
                                                colWidth2, colWidth3);

            }

	});

    // add MousesAdapter to the JTable
	addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                Point p = new Point(evt.getX(), evt.getY());
                int colIndex = columnAtPoint(p);
                if(evt.getButton() == MouseEvent.BUTTON1) {
                    if (colIndex == 0) {
                        int rowIndex = rowAtPoint(p);
                        ResultTable.this.dataModel.toggleLock(rowIndex);
                    }
                    // else not in lock column, look for double click.
                    else {
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
                            SessionShare ssshare = ResultTable.getSshare();
                            ShufflerService shufflerService =
                                ssshare.shufflerService();
                            ShufflerItem item =
                              shufflerService.getShufflerItem(hostFullpath);

                            if(item == null)
                                return;

                            String target = "Default";

                            // If Protocol or Study, we need to specify a
                            // target based on the locator history type.
                            LocatorHistoryList lh = 
                                          ssshare.getLocatorHistoryList();
                            lh = ssshare.getLocatorHistoryList();
                            // This will be protocols, edit_panel or default
                            String historyType = lh.getLocatorHistoryName();
                            if(Util.getRQPanel() != null && 
                               item.objType.equals(Shuf.DB_STUDY) &&
                               item.objType.equals(Shuf.DB_LCSTUDY) &&
                               Util.getViewPortType().equals(Global.REVIEW)) {
                                target="ReviewQueue";
                            } else if(historyType.equals(LocatorHistoryList.DEFAULT_LH)) {
                                if(item.objType.equals(Shuf.DB_PROTOCOL) ||
                                   item.objType.equals(Shuf.DB_STUDY) ||
                                   item.objType.equals(Shuf.DB_LCSTUDY) ) {

                                    target="StudyQueue";
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

                    String objType = FillDBManager.getType(hostFullpath);

                    // Get the mountpath (ie local)
                    String mPath = MountPaths.getMountPath(hostFullpath);

                    // Treat like a double click
                    String action = "DoubleClick";

                    String target = "Default";

                    String arg = "\'" + action + "\', \'" + objType 
                                + "\', \'" + host + "\', \'" + mPath + "\', \'"
                                + target + "\', \'\'";

                    Point position = evt.getPoint();
                    // Put up the appropriate menu
                    ContextMenu.popupMenu("locator_item", arg, 
                                    (JComponent)ResultTable.this.getParent(),
                                    position);
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
                mouseDownList = getSelectionList();


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
//	    dragSource = new DragSource();
//	    dragSource.createDefaultDragGestureRecognizer(this,
//				    DnDConstants.ACTION_COPY,
//				    new ShufDragGestureListener());
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

    } // ResultTable()

    private void debugDnD(String msg){
	if(DebugOutput.isSetFor("dnd"))
            Messages.postDebug("RT: " + msg);
    }

    public void setTooltip(String str) { }

    public String getTooltip(MouseEvent ev) {
        return getToolTipText(ev);
    }


    /**
     * Returns true to indicate that the height of the viewport determines
     * the table height. The default, a return value of false, results in
     * a "short" table appearance sometimes.
     *
     * CURRENTLY COMMENTED OUT BECAUSE SCROLLBAR DOESN'T APPEAR FOR LONGER
     * LISTS.
     * @return true
     */
    /* public boolean getScrollableTracksViewportHeight() {
	return true;
    } // getScrollableTracksViewportHeight()
    */

    /**
     * notification of a new statement
     * @param statement new statement
     */
    public void newStatement(Hashtable statement) {
	String returnObjType = (String)statement.get("ObjectType");

	clearSelection();
	attribPopup = createPopupMenu (returnObjType);
	// Initiate the actual database search via thread
	StartSearch searchThread = new StartSearch(statement);
	searchThread.setName("Shuffler Search");
	searchThread.start();

	// Update menus
	StatementDefinition curStatement = sshare.getCurrentStatementType();
//	curStatement.updateValues(statement, true);


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


    class ShufDragSourceListener implements DragSourceListener {
	public void dragDropEnd (DragSourceDropEvent evt) {}
	public void dragEnter (DragSourceDragEvent evt) {
	    debugDnD("dragEnter");
	}
	public void dragExit (DragSourceEvent evt) {
	    //debugDnD("dragExit");
	}
	public void dragOver (DragSourceDragEvent evt) {}
	public void dropActionChanged (DragSourceDragEvent evt) {
	}
    } // class ShufDragSourceListener

    class ShufDragGestureListener implements DragGestureListener {
	public void dragGestureRecognized(DragGestureEvent evt) {
	    // Don't use the selected row.  Instead use the row we
	    // saved when the mouse was pressed down.  Because the selected
	    // row ends up changing beyond my control as the mouse is
	    // dragged.  I think this is a Java bug.
	    // We used to do a getSelectedRow() call here.
	    int row = mouseDownRow;
	    

	    // getID(row) gives the unique filename for this row.
	    ID id = (ID) dataModel.getID(row);
	    // Trap for null or for the separator line and bail out.
	    if(id == null || id.toString().startsWith("-----"))
		return;

	    String hostFullpath = id.getName();
	    ShufflerService shufflerService = sshare.shufflerService();
	    ShufflerItem item = shufflerService.getShufflerItem(hostFullpath);

	    if(item == null)
		return;

	    // If we have clicked on the first row and it is editable
	    // it means we do not want to allow the D&D function, so return.
	    // Otherwise, we confuse the cell editor and the human
	    // by being in edit mode and in D&D mode at the same time.
	    if(isCellEditable(mouseDownRow, mouseDownCol))
		return;

	    // Since the selection keeps changing on me, reset it to
	    // the value when the drag started.  I am not using this here
	    // but it keeps the selected item highlighted so it
	    // looks normal to the user.
  	    clearSelection();
   	    getSelectionModel().setSelectionInterval(row, row);

	    // Set the source in the ShufflerItem so the Listener will
	    // know from whence this came.
	    item.source = "LOCATOR";

	    // Set the sourceRow so the listener will know which
	    // row this came from.
	    item.sourceRow = row;

	    // Pass a ShufflerItem object for the Drag and Drop item.
	    LocalRefSelection ref = new LocalRefSelection(item);
	    // We pass this item on to be caughted where this
	    // is dropped.
	    dragSource.startDrag(evt, null, ref, new ShufDragSourceListener());
	}
    } // class ShufDragGestureListener


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
	ResultTable.this.header.repaint();
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



    static String curPopupObjType = new String("");
    static JPopupMenu curPopup = new JPopupMenu();
    static final int colLength = 30;

    // For internal use to get a new popup menu.  Only create a new
    // one if the objType has changed.  Else, just return the
    // current one.
    private JPopupMenu createPopupMenu (String objType) {
	JMenuItem item;
	int rows;
	String attr;
        int numItems=0;

	if(curPopupObjType.equals(objType))
	   return curPopup;

	// Set the static copy for comparison next time.
	curPopupObjType = objType;

	ShufflerService shufflerService = sshare.shufflerService();
	ShufDBManager dbManager = ShufDBManager.getdbManager();



	curPopup = new JPopupMenu();

	// attribute menu
	ActionListener attribListener = new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
		SessionShare sshare2 = ResultTable.getSshare();
		// Set the new values into the buttons.
		int colIndex = ResultTable.this.columnSelected;

		// Set the new header label for this column.
		updateHeader(evt.getActionCommand(), colIndex);

		// Update the headers into the statement hashtable
		sshare2.statementHistory().append("headers",
					   dataModel.getHeaderInfo(colModel));

	    }
	};

	// set flag to let VnmrXCanvas to do paint correctly.
	curPopup.addPopupMenuListener(new PopupMenuListener() {
                public void popupMenuWillBecomeVisible(PopupMenuEvent e) {
                    Util.setMenuUp(true, curPopup);
                }
                public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
                    Util.setMenuUp(false);
                }
                public void popupMenuCanceled(PopupMenuEvent e) {
                }
         });


// 	item = curPopup.add("owner");
// 	item.addActionListener(attribListener);
// 	item = curPopup.add("exptype");
// 	item.addActionListener(attribListener);
// 	curPopup.addSeparator();

	// Get the list of attributes to put into the menu for chosing headers
	ArrayList attr_list =
                          dbManager.attrList.getAttrNamesLimited(objType);
	if (attr_list == null)
	    return curPopup;

	// Disallow array type attributes.  We cannot have them as column
	// headers.  Also keep arch_lock out of the list and tags
        // If array types are allowed, they cannot be sorted on.
        // Trying to sort on them gives an sql error.  If we ever allow
        // arrays in columns, we have to disallow that column for sorting.
	for (int i = 0; i < attr_list.size(); i++) {
	    attr = (String) attr_list.get(i);
            int isArray = dbManager.isParamArrayType(objType, attr);
	    if(isArray == 0 &&
               	       !attr.startsWith("arch_") && !attr.startsWith("tag")) {
		item = curPopup.add(attr);
                numItems++;
		item.addActionListener(attribListener);
	    }
	}


	// Set up for multiple column menu.  Since
	if(numItems >= colLength)
	    rows = colLength;
	// Trap for zero length list
	else if(numItems == 0)
	    rows = 1;
	else
	    rows = numItems;
	GridLayoutCol lm = new GridLayoutCol(rows, 0);
	curPopup.setLayout(lm);


	return curPopup;
    }

    public ResultDataModel getdataModel() {
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
	ArrayList <String> list=null;

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


    // Creating a ResultTransferHandler basically just saves the dragged
    // ShufflerItem list.
    class ResultTransferHandler extends TransferHandler {

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
            ResultTransferable tf;
            
            SessionShare ssshare = ResultTable.getSshare();
            ShufflerService shufflerService = ssshare.shufflerService();
            // If only one item selected, just transfer a single ShufflerItem
            if(list.size() == 1 ) {
                String hostFullpath = list.get(0);
                ShufflerItem item =
                    shufflerService.getShufflerItem(hostFullpath);
                item.source = "LOCATOR";
                tf = new ResultTransferable(item);
            }
            else {
                // Create a list of ShufflerItems from the fullpath list
                ArrayList<ShufflerItem> items = new ArrayList<ShufflerItem>();
                for(int i=0; i < list.size(); i++) {
                    String hostFullpath = list.get(i);
                    ShufflerItem item =
                        shufflerService.getShufflerItem(hostFullpath);
                    if(item == null)
                        continue;
                    item.source = "LOCATOR";

                    items.add(item);
                }
                tf = new ResultTransferable(items);
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

        
    }

    class ResultTransferable implements Transferable {
        Object componentToPass;
        
        public ResultTransferable(Object comp) {
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


} // class ResultTable
