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
 * Summary: Modal dialog with Remove button and list of current
 *          custom statements saved.
 *
 *
 </pre> **********************************************************/

public class CustomStatementRemoval extends ModalDialog
    			      implements ActionListener, PropertyChangeListener {
    /** Save single instance of one of these */
    static protected CustomStatementRemoval customStatementRemoval=null;

    /** Remove Button */
    protected JButton deleteButton;
    /** Table for Custom Statement names */
    protected JTable custTable;
    /** List of Custom Statement names */
    protected String[][] custNames;
    /** Data Model for Jtable. */
    private CustDataModel dataModel;
    /** Currently selected row */
    protected int selectedRow = 0;
    /** Previous Size and position */
    private Rectangle sizeAndPosition=null;
    /* Panel to hold the buttons. */
    protected JPanel panelForBtns;
    protected JScrollPane scrollpane;


    /************************************************** <pre>
     * Summary: Constructor, Add buttons to dialog box
     *
     </pre> **************************************************/
    public CustomStatementRemoval() {
	super(Util.getLabel("_Custom_Locator_Statement_Removal"));


	ArrayList custList;

	// Make a panel for the buttons
	panelForBtns = new JPanel();
	panelForBtns.setBorder(BorderFactory.createEmptyBorder(5, 0, 0, 0));

	deleteButton = new  JButton(Util.getLabel("blDelete"));

	// Add  to panel
	panelForBtns.add(deleteButton);


	// Set the buttons and the text item up with Listeners
	cancelButton.setActionCommand("cancel");
	cancelButton.addActionListener(this);
	helpButton.setActionCommand("help");
	helpButton.addActionListener(this);
	deleteButton.setActionCommand("delete");
	deleteButton.addActionListener(this);
	deleteButton.setMnemonic('d');
	okButton.setActionCommand("ok");
	okButton.addActionListener(this);


	// Create the table for the custom statement names.

        // CustNames will be filled in showDialog
        custNames = new String[1][1];

	String header[] = {Util.getLabel("_Custom_Locator_Statement_List")};
	custTable = new JTable (custNames, header);
	custTable.setShowGrid(false);
	custTable.setSelectionBackground(Global.HIGHLIGHTCOLOR);
	custTable.setSelectionForeground(Color.black);
	custTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
	dataModel = new CustDataModel();
	custTable.setModel(dataModel);

	custTable.addMouseListener(new MouseAdapter() {
	    public void mouseClicked(MouseEvent evt) {
		Point p = new Point(evt.getX(), evt.getY());
		int row = custTable.rowAtPoint(p);

		selectedRow = row;
	    }
	});

	scrollpane = new JScrollPane(custTable);
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
	custTable.setBackground(bgColor);
    }


    /************************************************** <pre>
     * Summary: Listener for all buttons.
     *
     *
     </pre> **************************************************/
    public void actionPerformed(ActionEvent e) {

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
                //Messages.postWarning("Help Not Implemented Yet");
                this.displayHelp();
            }
            else if(cmd.equals("delete")) {
                String name=null;
                try {
                    if(selectedRow >= custNames.length || selectedRow < 0) {
                        setVisible(false);
                        return;
                    }

                    // Get the filename
                    name = custNames[selectedRow][0];

                    // Remove this file
                    deleteCustFile(name);

                    setVisible(false);
                }
                catch(Exception ex) {
                    Messages.postError("Problem deleting " + name);

                    Messages.writeStackTrace(ex,
                                 "Trying to delete row number " + selectedRow);
                }
            }
            else if(cmd.equals("ok")) {
                setVisible(false);
            }
            // Get current position and size and save them for next time.
            sizeAndPosition = new Rectangle(getLocation(), getSize());
            writePersistence();
    }




    /************************************************** <pre>
     * Summary: Show the dialog and wait until finished.
     *
     *
     </pre> **************************************************/
    public void showDialog(String helpFile) {
        ArrayList custList;
        m_strHelpFile = helpFile;
        readPersistence();
        if(sizeAndPosition != null && sizeAndPosition.getWidth() != 0) {
            setLocation(sizeAndPosition.getLocation());
// The buttons do not track very well with resizing.
//	    setSize(sizeAndPosition.getSize());
        }

        // Create the table for the custom statement names.
        custList = getCustNames();

        // We need a String[][] for the call to JTable.  Even though we
        // only have one column and one of the array dimensions is '1',
        // we still need to create a String[][] for the call to JTable.
        custNames = new String[custList.size()][1];
        for(int i=0; i < custList.size(); i++) {
            custNames[i][0] = (String)custList.get(i);
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

	//filepath = new String (perDir + "/CustomStatementPosition");
	filepath =FileUtil.savePath("USER/PERSISTENCE/CustomStatementPosition");

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

	//filepath = new String (dir + "/persistence/CustomStatementPosition");
	filepath =FileUtil.savePath("USER/PERSISTENCE/CustomStatementPosition");

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

    public ArrayList getCustNames() {
        String dirpath;
        File file;
        ArrayList list;

	dirpath =FileUtil.savePath("USER/LOCATOR");
        file = new File(dirpath);

        // Does this directory exist?
        if(!file.isDirectory()) {
            // No, return empty list
            return new ArrayList();
        }

        // Get the list file filenames
        String[] strList = file.list();

        // Convert String[] to ArrayList
        list = new ArrayList();
        for(int i=0; i < strList.length; i++) {
            list.add(strList[i]);
        }

        return list;
    }

    public void deleteCustFile(String name) {
        String filepath;
        File file;
        ArrayList custList;

	filepath =FileUtil.savePath("USER/LOCATOR/" + name);
        file = new File(filepath);

        file.delete();

	// Create the table for the custom statement names.
	custList = getCustNames();

	// We need a String[][] for the call to JTable.  Even though we
	// only have one column and one of the array dimensions is '1',
	// we still need to create a String[][] for the call to JTable.
	custNames = new String[custList.size()][1];
	for(int i=0; i < custList.size(); i++) {
	    custNames[i][0] = (String)custList.get(i);
	}

        // Update the spotter menu so that deleted items are gone
        SessionShare sshare = ResultTable.getSshare();
        SpotterButton spotterButton = sshare.getSpotterButton();
        spotterButton.updateMenu();
    }

    static public CustomStatementRemoval getCustomStatementRemoval() {
        if(customStatementRemoval == null) {
            customStatementRemoval = new CustomStatementRemoval();
        }


        return customStatementRemoval;
    }


/********************************************************** <pre>
 * Summary: DataModel
 *
 *
 </pre> **********************************************************/

    public class CustDataModel extends AbstractTableModel {
	public int getRowCount() {
	    return custNames.length;
	}

	public int getColumnCount() {
	    return 1;
	}

	public Object getValueAt(int row, int column) {
	    return custNames[row][column];
	}

	public String getColumnName(int col) {
	    return Util.getLabel("_Custom_Locator_Statement_List");
	}
    }


}
