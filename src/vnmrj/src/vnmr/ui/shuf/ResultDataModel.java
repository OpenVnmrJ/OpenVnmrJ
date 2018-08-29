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
import java.util.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.*;
import java.io.*;


import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;


/**
 * data model used for ResultTable
 */
public class ResultDataModel extends AbstractTableModel {
    // ==== static variables
    /** number of attributes */
    private static final int NUMATTRIBS = 3;

    // ==== instance variables
    /** session share */
    private SessionShare sshare;
    /** attribute names */
    static private HeaderValue[] headers;
    /** results listing */
    private SearchResults[] results;
    /** current matrix of results, derived from 'results' and search cache*/
    private SearchResults[][] rmatrix;
    /** isCellEditable() will return true for row=0, col=editColumn */
    private long editColumn = -1;

    /**
     * constructor
     * @param sshare session share
     */
    public ResultDataModel(SessionShare sshare) {
        this.sshare = sshare;

        headers = new HeaderValue[]     {
                            new HeaderValue(null, true),
                            new HeaderValue("filename", true),
                            new HeaderValue("owner", true),
                            new HeaderValue(Shuf.DB_TIME_RUN, true)      };
        
        setResults(new SearchResults[0]);
    } // SessionShare()

    /**
     * set search results
     * @param results search results
     */

    /******************************************************************
     * Summary: Fill rmatrix for the locator window with the search results.
     *
     *****************************************************************/

    public synchronized void setResults(SearchResults[] results) {
        String hostFullpath;
        ShufflerItem item;
        HoldingDataModel holdingDataModel;
        int inc;
        SearchResults[] results_out;
        Object[] cont;
        boolean foundNewlySaved=false;
        SearchResults matchRow=null;
        Object  newrow[];

        if(results == null)
            return;

        

        ShufflerService shufflerService = sshare.shufflerService();
        Hashtable searchCache = sshare.searchCache();

        int newlySavedNum = Shuf.newlySavedFile.size();
        if(newlySavedNum > 0) {
            // Add one for separator
            inc = 1 + newlySavedNum;

            // We need to create a new results with the extra rows as we go.
            results_out = new SearchResults[results.length + inc];
        }
        else {
            inc = 0;
            results_out = results;
        }
        // Turn off editing.  It will only come back on if necessary.
        editColumn = -1;

        // derive rmatrix
        int len = results.length + inc;
        if(len < 1)
            len = 1;
        rmatrix = new SearchResults[len][9];     

        for (int i = 0; i < results.length; i++)  {
            matchRow = results[i];
            // If newlySavedFile is in use, fill the new results array
            if(newlySavedNum > 0) 
                results_out[i + inc] = matchRow;
            cont = (Object[])(matchRow.content);

            // Element 7 is the host name and element 5 is the fullpath.
            // hostFullpath is create as the key.
            hostFullpath = cont[7]+":"+cont[5];
            Object[] contents = (Object[])searchCache.get(hostFullpath);

            // We receive an arg 'results' which is an array of
            // SearchResults (rows) which has in each of them, an array of
            // Objects (columns) giving the value of each column for that
            // row.  We are now going to create a new set of SearchResults
            // where the 'content' is no longer an array of Objects but is
            // a single value.  This is used by getValueAt() for filling
            // the panel.  Rather than taking the info directly from the
            // results input arg, we are just getting the filename from the
            // input arg, and then getting the rest of the information from
            // the hashtable.  We have filled both the hashtable and the
            // results array.

            // Loop through all newlySavedFile's to see if any match this
            for(int k=0; k < newlySavedNum; k++) {
                if(Shuf.newlySavedFile.get(k).equals(hostFullpath)) {
                    // If newlySavedFile is in use, put this one in row k
                    // and a separator in row newlySavedNum.  If sep changes, 
                    // the code in // ResultTable.dragGestureRecognized() needs 
                    // to be changed // to trap for this separator.
                    foundNewlySaved = true;
                    String sep = 
                        "------------------------------------------------";
                    results_out[k] = matchRow;
                    results_out[newlySavedNum] = new SearchResults(1, sep, 
                                                       new ID (sep), sep, sep);
                    for (int j = 0; j < contents.length; j++) {
                        // This rmatrix is where the actual panel gets its 
                        // information.  The first arg set to 2, means green.
                        rmatrix[k][j] = new SearchResults(2, contents[j], 
                                              new ID ((String)contents[5]),
                                              (String)cont[7], (String)cont[6]);
                        rmatrix[newlySavedNum][j] = results_out[newlySavedNum];
                    }

                    // If objType is not DB_PANELSNCOMPONENTS and if one of 
                    // the columns is "filename", set that cell editable.
                    if(!contents[6].equals(Shuf.DB_PANELSNCOMPONENTS)) {
                        for(int j=1; j < headers.length; j++) {
                            if(headers[j].getText().equals("filename")) {
                                editColumn = j;
                            }
                        }
                    }
                }
            }
            for (int j = 0; j < contents.length; j++) {
                // This rmatrix is where the actual panel gets its info
                rmatrix[i+inc][j] = new SearchResults(matchRow.match, 
                                           contents[j], 
                                           new ID ((String)contents[5]),
                                           (String)cont[7], (String)cont[6]);
            }

            // We need to keep the Holding pen up to date.  Send it the
            // current row of information in the form of the ShufflerItem
            // and let it determine whether it has that item or not.
            item = shufflerService.getShufflerItem(hostFullpath);
            holdingDataModel = sshare.holdingDataModel();
            holdingDataModel.update(item);
        }
        // If no newly saved file was found we have newlySavedNum rows at
        // the top of rmatrix which have not been filled.
        if(newlySavedNum > 0 && !foundNewlySaved) {
            // The item that was in newlySavedFile, was not in the search
            // results.  If we leave things this way, the top 2 or more
            // lines will be wrong.  Clear out the newlySavedFile list
            // and recall this method we are in to do it all again.
            Shuf.newlySavedFile.clear();
            setResults(results);
            // If we just called ourselves again, it is that invocation that
            // needs to set this.results, so we want to just exit from
            // this point and let the other invocation do the job.
            return;
        }
        this.results = results_out;

        // everything has changed
        fireTableChanged(new TableModelEvent(this));

    } // setResults()

    /**
     * given the ID of a given row
     * @param row row index
     */
    public Object getID(int row) {
        Object obj;

        try {
            obj = (Object)results[row].id;
        }
        catch (ArrayIndexOutOfBoundsException e) {
            Messages.postError("Row index selected is out of bounds");
            Messages.writeStackTrace(e, "Error caught in getID");
            return null;
        }

        return obj;

    } // getID()

    /**
     * toggle lock status of given row
     * @param row row index
     */
    public void toggleLock(int row) {
        SearchResults lockStatusObj = (SearchResults)rmatrix[row][0];
        LockStatus lockStatus = (LockStatus)lockStatusObj.content;
        LockStatus newStatus = lockStatus.inverse();

        setLockStatus(row, newStatus);

    } // toggleLock()

     /**
     * set lock status of given row
     * @param row row index
     */
    public void setLockStatus(int row, LockStatus status) {
        String strLockStatus;

        try {
            SearchResults lockStatusObj = (SearchResults)rmatrix[row][0];
            LockStatus lockStatus = (LockStatus)lockStatusObj.content;
            lockStatusObj.content = status;
            setValueAt(lockStatusObj, row, 0); // to trigger event

            // We need to update the DB also.
            ShufDBManager dbManager =ShufDBManager.getdbManager();
            SearchResults host = rmatrix[row][7];
            SearchResults fullpath = rmatrix[row][5];
            SearchResults objType = rmatrix[row][6];
            if(status.isLocked())
                strLockStatus = "1";
            else
                strLockStatus = "0";

            // The fullpath and host here are direct and we need mounted
            // fullpath and local host for setAttributeValue()
            String mpath = MountPaths.getMountPath((String) host.content, 
                                                   (String) fullpath.content);
            String localHost = dbManager.localHost;

            dbManager.setAttributeValue((String) objType.content, 
                                        (String) mpath,
                                        (String) localHost, 
                                        "arch_lock", "int", strLockStatus);
        }
        catch (Exception e) {
            Messages.postWarning("Problem setting the locator lock status");
            Messages.writeStackTrace(e);
        }
    } // setLockStatus()

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
        return results.length;
    } // getRowCount()

    /**
     * get column name
     * @param column column index
     */
    public String getColumnName(int column) {
        return ""; // unused
    } // getColumnName()

    /**
     * return values based on results.
     * This method is called by JTable to get its values for the shuffler
     * results table.  This SearchResults object ends up going to 
     * ShufCellRenderer.setValue() where it is actually used.
     */
    public Object getValueAt(int row, int col) {
        int r, c;
        r = row;
        c = col;

        // For some unknown reason, sometimes a request is made for an
        // out of bound object.  Trap and keep in bounds.
        if(row >= rmatrix.length)
            r = rmatrix.length -1;
        if(col >= rmatrix[0].length)
            c = rmatrix[0].length -1;
        Object obj = rmatrix[r][c];  // Test Code Line
        return rmatrix[r][c];
    } // getValueAt()

    /**
     * set value
     */
    public void setValueAt(Object value, int row, int col) {
        String filename;
        String user;

        if(value instanceof SearchResults)
            rmatrix[row][col] = (SearchResults)value;
        else {
            // Do not allow spaces in the value.  This should a new name entered
            // into the locator after the saving of a file.  Remove this when
            // we get everything working for spaces.
            if(value instanceof String) {
                if (!verifyName((String)value)) {
                    // Contains an illegal character
                    Messages.postError(value + " Contains an illegal character, Aborting.");
                    return;
                }
            }
            SearchResults oldsr = rmatrix[row][col];
            // The oldsr.id contains the fullpath for the old filename
            // It will be the direct path on host.  Convert to the mount
            // path before continuing
            // Change it to the new string and keep any extension
            // that the old name had.
            // Get any extension.
            String oldFullpath =  oldsr.id.toString();
            // Convert to local mount path
            oldFullpath = MountPaths.getMountPath(oldsr.host, oldFullpath);
            
            int extIndex = oldFullpath.lastIndexOf(".");
            int slashIndex = oldFullpath.lastIndexOf(File.separator);
            if(slashIndex == -1) {
                Messages.postError("Problem with filename: " + oldFullpath);
                return;
            }
            String front = oldFullpath.substring(0, slashIndex +1);
            String newFullPath;
            if(extIndex != -1) {
                String ext   = oldFullpath.substring(extIndex);
                newFullPath = new String(front + value + ext);
                filename = new String(value + ext);
            }
            else {
                newFullPath = new String(front + value);
                filename = new String((String)value);
            }

            if(!newFullPath.equals(oldFullpath)) {

                // Give the file/directory the new name.
                File oldFile = new File(oldFullpath);
                File newFile = new File(newFullPath);
                oldFile.renameTo(newFile);

                // Remove the old entry from the DB
                ExpPanel exp=Util.getDefaultExp();
                exp.removeFromShuffler(oldsr.objType, oldFullpath, oldsr.host,
                                       false);

                // Add the new entry to the DB
                user = System.getProperty("user.name");
                exp.addToShuffler(oldsr.objType, filename, newFullPath, 
                                  user, false);

 
            }
        }
        fireTableChanged(new TableModelEvent(this, row));
    } // setValueAt()

    
    
    final static char illegal_fchars[] = {
        ' ', '!', '"', '$', '&', '\'', '(', ')', '*', ';', '<', '>', '?',
        '\\', '[', ']', '^', '`', '{', '}', '|', ',', '\0' };
    // Verify that all of the chars in the arg are valid.
    // I took this from vnmrbg tools.c and adapted it
    public boolean verifyName(String name) {
        char lchar, tchar;
        int iter, jter, len;

        len = name.length();
        if (len <= 0)
            return (false);

        for (iter = 0; iter < len; iter++) {
            tchar = name.charAt(iter);
            if (tchar < 32 || tchar > 126)
                return (false);
            jter = 0;
            while ((lchar = illegal_fchars[jter++]) != '\0')
                if (lchar == tchar)
                    return (false);
        }
        return (true);
    }

     public boolean isCellEditable(int row, int col) {
         if(row == 0 && col == editColumn)
             return true;
         else
             return false;
     } // isCellEditable()

    /**
     * given an attribute-column index, get the type of the attribute
     * @param index the attribute index
     * @return attribute type
     */
    public String getAttribute(int index) {
        if (index >= 0 && index <= NUMATTRIBS) {
            return headers[index].getText();
        } else {
            return "?";
        }
    } // getAttribute()


    public void setAttribute(int index, String newText) {
        if (index >= 0 && index <= NUMATTRIBS) {
            headers[index].setText(newText);
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

    /**
     * Set the header corresponding to the given index
     */
    public void setKeyedHeader(int index) {
        for (int i = 0; i < headers.length; i++) {
            HeaderValue value = headers[i];
            value.setKeyed(i == index);
        }
    } // setKeyedHeader()


    /* Get information about header columns needed for the search.
       Assuming NUMATTRIBS=3,
       headers[0-2] will be the three header strings,
       headers[3] is the column to sort on
       headers[4] is ASC or DESC indicating the direction of sort.
    */
    public String[] getHeaderInfo(TableColumnModel colModel) {
        int i;
       
        String[] headerList = new String[5];
        // The first column is the lock status, so use i+1.
        for(i=0; i < NUMATTRIBS; i++) {
            headerList[i] = new String(getAttribute(i+1));

            TableColumn col = colModel.getColumn(i+1);
            HeaderValue hValue = (HeaderValue)col.getHeaderValue();

            if(hValue.isKeyed()) {
                headerList[NUMATTRIBS] = new String(headerList[i]);
                if(hValue.isAscending()) 
                    headerList[NUMATTRIBS +1] = new String("ASC");
                else
                    headerList[NUMATTRIBS +1] = new String("DESC");
            }
        }
        // If no sorting column is set, take the first column in ASC order.
        if(headerList[NUMATTRIBS] == null) {
            headerList[NUMATTRIBS] = new String(headerList[0]);
            headerList[NUMATTRIBS +1] = new String("ASC");
        }

        return headerList;
    }


    /************************************************** <pre>
     * Summary: Return the default header info as per args and fill member
     *          variable, headers.
     </pre> **************************************************/
    public static String[] getDefaultHeaderInfo(String statementType,
                                                String[] columns,String[] sort){
        String[] headerList = new String[5];

        // Set the ResultDataModel member 'headers' to the defaults
        headers = new HeaderValue[]     {
            new HeaderValue(null, true),
                new HeaderValue(columns[0], true),
                new HeaderValue(columns[1], true),
                new HeaderValue(columns[2], true)      };

        headerList[0] = new String(columns[0]);
        headerList[1] = new String(columns[1]);
        headerList[2] = new String(columns[2]);
        headerList[3] = new String(sort[0]);    // Sort column
        headerList[4] = new String(sort[1]);

        // Now return the defaults as String[].
        
        return headerList;
    }

} // class ResultDataModel
