/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import javax.swing.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.border.*;
import java.beans.*;
import java.io.*;

import vnmr.util.*;
import vnmr.bo.*;
import vnmr.ui.*;

/********************************************************** <pre>
 * Summary: Modal dialog with two additional buttons for delete and edit
 *          and a table of tags.
 *
 *
 </pre> **********************************************************/

public class TagEditDialog extends ModalDialog
    			      implements ActionListener, PropertyChangeListener {
    /** Add Button */
    protected JButton deleteButton;
    /** Remove Button */
    protected JButton editButton;
    /** Table for tag names */
    protected JTable tagTable;
    /** List of tag names */
    protected String[][] tagNames;
    /** Object type currently in the table. */
    protected String objType;
    /** Data Model for Jtable. */
    private TagDataModel dataModel;
    /** Specify if any row is editable and if so, which one. */
    protected int editRow = -1;
    /** Currently selected row */
    protected int selectedRow = 0;
    /** parent for setting previous tags list */
    private TagButton parent;
    /** Previous Size and position */
    private Rectangle sizeAndPosition=null;
    /* Panel to hold the buttons. */
    protected JPanel panelForBtns;
    protected JScrollPane scrollpane;


    /************************************************** <pre>
     * Summary: Constructor, Add buttons to dialog box
     *
     </pre> **************************************************/
    public TagEditDialog(TagButton parent) {
	super(Util.getLabel("_Locator_Group_Edit"));

	this.parent = parent;

	ArrayList tagList;

	// Make a panel for the buttons
	panelForBtns = new JPanel();
	panelForBtns.setBorder(BorderFactory.createEmptyBorder(5, 0, 0, 0));

	// Create the two items.
	deleteButton = new  JButton(Util.getLabel("_Locator_Delete_Group"));
	editButton = new  JButton(Util.getLabel("_Locator_Edit_Group_Name"));

	// Add items to panel
	panelForBtns.add(deleteButton);
	panelForBtns.add(editButton);


	// Set the buttons and the text item up with Listeners
	cancelButton.setActionCommand("cancel");
	cancelButton.addActionListener(this);
	helpButton.setActionCommand("help");
	helpButton.addActionListener(this);
	deleteButton.setActionCommand("delete");
	deleteButton.addActionListener(this);
	deleteButton.setMnemonic('d');
	editButton.setActionCommand("edit");
	editButton.addActionListener(this);
	editButton.setMnemonic('e');
	okButton.setActionCommand("ok");
	okButton.addActionListener(this);

	// Need objType
	SessionShare sshare = ResultTable.getSshare();
	StatementHistory history = sshare.statementHistory();
	Hashtable statement = history.getCurrentStatement();
	objType = (String)statement.get("ObjectType");

	// Create the table for the tag name.
	tagList = TagList.getAllTagNames(objType);


	// We need a String[][] for the call to JTable.  Even though we
	// only have one column and one of the array dimensions is '1',
	// we still need to create a String[][] for the call to JTable.
	tagNames = new String[tagList.size()][1];
	for(int i=0; i < tagList.size(); i++) {
	    tagNames[i][0] = (String)tagList.get(i);
	}



	String header[] = {Util.getLabel("_Locator_Group_Names")};
	tagTable = new JTable (tagNames, header);
	tagTable.setShowGrid(false);
	tagTable.setSelectionBackground(Global.HIGHLIGHTCOLOR);
	tagTable.setSelectionForeground(Color.black);
	tagTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
	dataModel = new TagDataModel();
	tagTable.setModel(dataModel);

	tagTable.addMouseListener(new MouseAdapter() {
	    public void mouseClicked(MouseEvent evt) {
		Point p = new Point(evt.getX(), evt.getY());
		int row = tagTable.rowAtPoint(p);

		selectedRow = row;

		// Get the list of all items with this tag.
		ShufDBManager dbMg = ShufDBManager.getdbManager();
		ArrayList items =
		    dbMg.getAllEntriesWithThisTag(objType, tagNames[row][0]);


		// Highlight the list of items in the locator.
		ResultTable resultTable = Shuffler.getresultTable();
		resultTable.setHighlightSelections(items);

	    }
	});

	scrollpane = new JScrollPane(tagTable);
	Border outsideBorder =
	    BorderFactory.createMatteBorder(1, 1, 1, 1, Global.BGCOLOR);
	Border insideBorder =
	    BorderFactory.createBevelBorder(BevelBorder.LOWERED);
	scrollpane.setBorder(BorderFactory.createCompoundBorder(outsideBorder,
						     insideBorder));

	// setSize() and setBounds() do not work here for some reason,
	// but setPreferredSize() works.
	scrollpane.setPreferredSize(new Dimension(250,300));

	// Add scroll pane to top
	getContentPane().add(scrollpane, BorderLayout.NORTH);

	// Add the panel with buttons to middle
	getContentPane().add(panelForBtns, BorderLayout.CENTER);

	setBgColor(Util.getBgColor());
	DisplayOptions.addChangeListener(this);

	pack();

    }

    public void propertyChange(PropertyChangeEvent evt)
    {
	setBgColor(Util.getBgColor());
    }

    protected void setBgColor(Color bgColor)
    {
	setBackgroundColor(bgColor);

	panelForBtns.setBackground(bgColor);
	deleteButton.setBackground(bgColor);
	editButton.setBackground(bgColor);
	tagTable.setBackground(bgColor);
    }


    /************************************************** <pre>
     * Summary: Listener for all buttons.
     *
     *
     </pre> **************************************************/
    public void actionPerformed(ActionEvent e) {
	// The locator's JTable is not updated correctly when the
	// edit panel is unshown.  This seems to get around that java bug.
	// by forcing it to be updated.
	ResultTable resultTable = Shuffler.getresultTable();
	resultTable.repaint();

	String cmd = e.getActionCommand();
	// Cancel
	if(cmd.equals("cancel")) {
	    setVisible(false);
	}
	// Help
	else if(cmd.equals("help")) {
	    // Do not call setVisible(false);  That will cause
	    // the Block to release. This way
	    // the panel stays up and the Block stays in effect.
	    displayHelp();
	}
	else if(cmd.equals("delete")) {
            int tagListSize = TagList.getTagListSize(objType);
            if(selectedRow >= tagListSize || selectedRow < 0) {
                setVisible(false);
                return;
            }

	    String tag = tagNames[selectedRow][0];
	    ShufDBManager dbMg = ShufDBManager.getdbManager();
	    // Take it out of the DB
	    dbMg.deleteThisTag(objType, tag);

	    // Take it out of allTagNames
	    TagList.deleteFromAllTagNames(objType, tag);

	    // Put it into previous list at top of menu
	    parent.setPrevious(objType, tag);

	    setVisible(false);

	}
	else if(cmd.equals("edit")) {
	    editRow = selectedRow;
	    TagEditDialog.this.tagTable.editCellAt(selectedRow, 0);
	}
	else if(cmd.equals("ok")) {
	    // Have ok do same as a return if we are editing.
	    JTextField jtf =
		(JTextField)TagEditDialog.this.tagTable.getEditorComponent();
	    if(jtf != null)
		jtf.postActionEvent();

	    setVisible(false);

	}
	// Get current position and size and save them for next time.
	sizeAndPosition = new Rectangle(getLocation(), getSize());
	writePersistence();

    }




    /************************************************** <pre>
     * Summary: Show the dialog and wait until finished.
     *
     *	    The Dialog will be positioned over the component passed in.
     *	    This way, the cursor will not need to be moved.
     *	    If null is passed in, the dialog will be positioned in the
     *	    center of the screen.
     *
     </pre> **************************************************/
    public void showDialog(Component comp) {
	readPersistence();
	if(sizeAndPosition != null && sizeAndPosition.getWidth() != 0) {
	    setLocation(sizeAndPosition.getLocation());
//It does not resize gracefully, so don't keep any new size.
//	    setSize(sizeAndPosition.getSize());
	}
	else {
	    // Get Screen location of comp
	    Point pt = comp.getLocationOnScreen();

	    // Set the location so it is off of the locator.
	    pt.setLocation(pt.getX() + 60, pt.getY() - 100);
	    setLocation(pt);
	}
	// Show the dialog
	showDialogWithThread();

	return;
    }

    public void writePersistence() {
	String filepath;
	//File file;
	ObjectOutput out;
	//String dir, perDir;

	//dir = System.getProperty("userdir");
	//perDir = new String(dir + "/persistence");
	//file = new File(perDir);
	// If this directory does not exist, make it.
	//if(!file.exists()) {
	//    file.mkdir();
	//}

	//filepath = new String (perDir + "/TagEditPosition");
	filepath = FileUtil.savePath("USER/PERSISTENCE/TagEditPosition");

	try {
	    out = new ObjectOutputStream(new FileOutputStream(filepath));
	    // Write it out.
	    out.writeObject(sizeAndPosition);
            out.close();
	}
	catch (Exception ioe) {
            Messages.postError("Problem writing filepath");
            Messages.writeStackTrace(ioe, "Error caught in writePersistence");
	}

    }


    public void readPersistence() {
	String filepath;
	ObjectInputStream in;
	//String dir;

	// clear size in case there is a problem reading the file.
	sizeAndPosition= null;

	//dir = System.getProperty("userdir");

	//filepath = new String (dir + "/persistence/TagEditPosition");
	filepath = FileUtil.savePath("USER/PERSISTENCE/TagEditPosition");

	try {
	    in = new ObjectInputStream(new FileInputStream(filepath));
	    // Read it in.
	    sizeAndPosition = (Rectangle) in.readObject();
            in.close();
	}
	catch (ClassNotFoundException ce) {
            Messages.postError("Problem reading filepath");
            Messages.writeStackTrace(ce, "Error caught in readPersistence");
	}
	catch (Exception e) {
	    // No error output here.
	}
    }


/********************************************************** <pre>
 * Summary: DataModel to allow editing of desired cell.
 *
 *
 </pre> **********************************************************/

    public class TagDataModel extends AbstractTableModel {
	public int getRowCount() {
	    return tagNames.length;
	}

	public int getColumnCount() {
	    return 1;
	}

	public Object getValueAt(int row, int column) {
	    return tagNames[row][column];
	}

	public boolean isCellEditable(int row, int column) {
	    if(TagEditDialog.this.editRow == row)
		return true;
	    else
		return false;
	}

	// Come here when the user hits a return after editing a cell.
	public void setValueAt(Object value, int row, int col) {

	    // If no change, just exit.
	    if(tagNames[row][col].equals((String) value))
	       return;

	    String origTagName = tagNames[row][col];

	    // Set the new name
	    tagNames[row][col] = (String) value;

	    // disable editing of this row.
	    editRow = -1;

	    // Get the current list of items with this tag.
	    ShufDBManager dbMg = ShufDBManager.getdbManager();
	    ArrayList items =
		    dbMg.getAllEntriesWithThisTag(objType, origTagName);


	    // Remove this tag from all of the items.
	    dbMg.deleteThisTag(objType, origTagName);
	    // Take it out of allTagNames
	    TagList.deleteFromAllTagNames(objType, origTagName);

	    // Add the new tag name to the list of items.
	    dbMg.setTag(objType, tagNames[row][col], items, true);
	    TagList.addToAllTagNames(objType, tagNames[row][col]);

	    // Put it into previous list at top of menu
	    parent.setPrevious(objType, origTagName);
	    // Put it into previous list at top of menu
	    parent.setPrevious(objType, tagNames[row][col]);

	    // Update statement menus
	    StatementDefinition curStatement;
	    SessionShare sshare = ResultTable.getSshare();
	    StatementHistory history = sshare.statementHistory();
	    Hashtable statement = history.getCurrentStatement();
	    curStatement = sshare.getCurrentStatementType();
	    curStatement.updateValues(statement, true);
	}

	public String getColumnName(int col) {
	    return "Locator Group Names";
	}
    }


}
