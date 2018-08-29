/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.util.*;
import java.io.*;
import java.net.*;
import javax.swing.event.*;
import javax.swing.table.*;

import  vnmr.bo.*;
import  vnmr.util.*;

/**
 * data model used for HoldingTable
 *
 * @author Mark Cao
 */
public class HoldingDataModel extends AbstractTableModel {
    // ==== instance variables
    /** List of ShufflerItem objects */
    private HashArrayList tableItems=null;
    /** attribute names */
    private HeaderValue[] headers = new HeaderValue[] {
	new HeaderValue("", false),
 	new HeaderValue("", false),
	new HeaderValue("", false),
   };

    /**
     * constructor
     */
    public HoldingDataModel() {
	// Try to get the previous information
	readPersistence();
	// If not found, create an empty list.
	if(tableItems == null)
	    tableItems = new HashArrayList();
    } // HoldingDataModel()

    /**
     * get column count
     * @return column count
     */
    public int getColumnCount() {
	return headers.length;
    } // getColumnCount()

    /**
     * get row count
     * @return row count
     */
    public int getRowCount() {
	return tableItems.size();
    } // getRowCount()

    /**
     * get column name
     * @param column column index
     */
    public String getColumnName(int column) {
	return ""; // unused
    } // getColumnName()

    /**
     * return values based on results
     * This method is called by JTable to get its values for the  table.  
     * This returned object ends up going to 
     * HoldingCellRenderer.setValue() where it is actually used.
     * We need to send both the attribute name and its value.
     */
    public Object getValueAt(int row, int col) {
	int r, c;

	r = row;
	c = col;

	if(row >= tableItems.size())
	    r = tableItems.size() -1;
	ShufflerItem item = (ShufflerItem) tableItems.get(r);
	String[] returnString = new String[2];

	if(col >= item.attrname.length  || col >= item.value.length)
	    c = item.attrname.length -1;
	returnString[0] = item.attrname[c];
	returnString[1] = item.value[c];


	return returnString;
	
    } // getValueAt()

    /**
     * set value
     */
    public void setValueAt(Object value, int row, int col) {
	
	ShufflerItem item = (ShufflerItem) value;
	// Use hostFullpath as key for HashArrayList
	tableItems.set(row, value, item.hostFullpath);
	fireTableChanged(new TableModelEvent(this, row));
    } // setValueAt()

    /**
     * Add an item to the end of the holding area. If already there, don't add.
     * @param item ShufflerItem object
     */
    public void add(ShufflerItem item) {
	if(tableItems.containsKey(item.hostFullpath)) {
	    // This item already exists.  put will just update it.
	    int row = tableItems.set(item.hostFullpath, item);
	    // Tell the table to update this row.
	    fireTableChanged(new TableModelEvent(this, row));
	}
	else {
	    int row = tableItems.set(item.hostFullpath, item);
            HoldingArea hArea = Util.getHoldingArea();
            int lastRow = hArea.m_holdingTable.getRowCount();
            // Update all rows
	    fireTableChanged(new TableModelEvent(this, 0, lastRow,
					     TableModelEvent.ALL_COLUMNS,
					     TableModelEvent.INSERT));
	}

    } // add()

    /**
     * Add an item to the specified row of the holding area. 
     * If already there, remove the old one and insert the new one.
     * @param item ShufflerItem object
     */
    public void add(int index, ShufflerItem item) {
        tableItems.add(index, item.hostFullpath, item);

        HoldingArea hArea = Util.getHoldingArea();
        int lastRow = hArea.m_holdingTable.getRowCount();
        // Update all rows.
        fireTableChanged(new TableModelEvent(this, 0, lastRow,
					     TableModelEvent.ALL_COLUMNS,
					     TableModelEvent.INSERT));
    } // add()

    /************************************************** <pre>
     * Summary: Update holding area item with item received.
     </pre> **************************************************/
    public void update(ShufflerItem item) {
	// Does this item exist in the Holding Table?
	if(tableItems.containsKey(item.hostFullpath)) {
	    // Yes, it is in the table.  Just 'add' this item and
	    // it will be updated.
	    add(item);
	}
    }

     /************************************************** <pre>
      * Update just a single param in an item in the holding area.
      *
      *   This method is primarily for use when the type of item to be updated
      *   is NOT the type in the shuffler at this time.  Thus, the item
      *   in the holding area will not normally be update.  This allows
      *   the updating of a single param if that param is one of the 
      *   columns.
     </pre> **************************************************/
    public void updateSingleParam(String hostFullpath, String param, 
				  String value) {
	ShufflerItem item;
	boolean foundOne=false;
	int	row;

	// Does this item exist in the Holding Table?
	if(tableItems.containsKey(hostFullpath)) {
	    // Yes, it is in the table. Does the item have param as one
	    // of the columns?  If so, update it.
	    item = (ShufflerItem) tableItems.get(hostFullpath);
	    for(int i=0; i < 3; i++) {
		if(item.attrname[i].equals(param)) {
		    item.value[i] = value;

		    // Tell the table to  update.
		    row = tableItems.indexOf(item);
		    fireTableChanged(new TableModelEvent(this, row));

		}
	    }
	}
    }

    /**
     * get the header value for the given index
     * @param index index
     * @return header value
     */
    public HeaderValue getHeaderValue(int index) {
	if (index >= 0 && index < headers.length)
	    return headers[index];
	else
	    return null;
    } // getHeaderValue()


    /************************************************** <pre>
     * Summary: Get the ShufflerItem for this row.
     </pre> **************************************************/
    public ShufflerItem getRowItem(int row) {
	return (ShufflerItem) tableItems.get(row);
    }

    /************************************************** <pre>
     * Summary: Get the ShufflerItem for this file and host.
     </pre> **************************************************/
    public ShufflerItem getItemThisFile(String file, String host) {
	String hostFullpath = host + ":" + file;
	ShufflerItem item = (ShufflerItem) tableItems.get(hostFullpath);
	return(item); 
    }


    public void removeItem(ShufflerItem item) {
	// Get the row # for the table update.
	int row = tableItems.indexOf(item);

	// Remove by key
	tableItems.remove(item.hostFullpath);

	// Update the table displayed.
	fireTableChanged(new TableModelEvent(this, row, row,
					     TableModelEvent.ALL_COLUMNS,
					     TableModelEvent.DELETE));
    }

    /************************************************** <pre>
     * Summary: Remove the Holding table entry for this file.
     </pre> **************************************************/
    public void removeFileEntry(String file, String host) {
	ShufflerItem item = getItemThisFile(file, host);
	if(item != null)
	    removeItem(item);
    }


    /************************************************** <pre>
     * Summary: Write out the persistence file in plain text format
     *
     </pre> **************************************************/

    public void writePersistence() {
	String filepath;
	PrintWriter out;

	File file;

        filepath=FileUtil.savePath("USER/PERSISTENCE/HoldingArea");
	try {
	    // Write out the file in plain text
	    out = new PrintWriter(new FileWriter(filepath));
	    // Write out a header line
	    out.println("HoldingData");
	    for(int i=0; i < tableItems.size(); i++) {
		ShufflerItem si = (ShufflerItem)tableItems.get(i);
		// terminate each set of ShufflerItem info with a ":"
		out.println(si.toString() + ":");
	    }
	    out.close();
	}
	catch (Exception e) {
	    Messages.writeStackTrace(e);
	}	
    }

    /************************************************** <pre>
     * Summary: Read in the plain text persistence file.
     *
     </pre> **************************************************/
    public void readPersistence() {
	String filepath;
	BufferedReader in;
	String var;
	String value;
	String line;
	StringTokenizer tok;
	ShufflerItem si=null;

        filepath=FileUtil.openPath("USER/PERSISTENCE/HoldingArea");
        if(filepath==null)
            return;
	
	try {
	    in = new BufferedReader(new FileReader(filepath));
	    tableItems = new HashArrayList();

	    // The file must start with HoldingData or ignore the rest.
	    line = in.readLine();
	    if(line != null) {
		tok = new StringTokenizer(line, " \t");
		if (tok.hasMoreTokens()) {
		    var = tok.nextToken();
		    if(var.equals("HoldingData")) {
			// Start of file
			si = new ShufflerItem();
		    }
		}
		else
		    throw(new Exception("Holding Data persistence file must have the string HoldingData as the first line"));
	    }
	    while ((line = in.readLine()) != null) {
		tok = new StringTokenizer(line, " \t");
		if (tok.hasMoreTokens())
		    var = tok.nextToken();
		else
		    continue;
		
		if(var.equals(":")) {
		    // We are at the end of one set of ShufflerItem info
		    // put it into the tableItems list.
		    tableItems.put(si.hostFullpath, si);

		    // Start a new si in case there are more to follow
		    si = new ShufflerItem();
		    continue;
		}
                if (tok.hasMoreTokens())
		    value = tok.nextToken();
		else
		    value = "";
		// Set the value for this var(iable) in si
		si.setValue(var, value);
	    }
            in.close();

	} catch (IOException e) {
	    // No error output here.
	} catch (Exception e) {
            Messages.writeStackTrace(e);
        }
   }

    /** Remove any entries whose files have disappeared.
     */
    public void cleanupHoldingArea() {
	ShufflerItem item;
	String localHost;
	UNFile   file;

	try {
	    InetAddress inetAddress = InetAddress.getLocalHost();
	    localHost = inetAddress.getHostName();
	}
	catch(Exception e) {
	    Messages.postError("Error getting HostName");
            Messages.writeStackTrace(e);
	    localHost = new String("");
	}

	for(int i=0; i < tableItems.size(); i++) {
	    item = (ShufflerItem) tableItems.get(i);
	    // Only work with things from the localHost.  This test
	    // will also eliminate macros and commands since they have
	    // no host set.
	    if(localHost.equals(item.dhost)) {
                String mpath = item.getFullpath();
		file = new UNFile(mpath);
		if(!file.exists()) {
		    removeItem(item);
		    // removing an item, renumbers the rest of them.
		    // Since there are not too many, just start over
		    // when one is removed.  tableItems.size() will
		    // also adjust as we go.
		    i=0;
		}
	    }
	}
    }

} // class HoldingDataModel
