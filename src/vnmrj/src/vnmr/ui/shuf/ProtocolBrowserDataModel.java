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
public class ProtocolBrowserDataModel extends AbstractTableModel {
    // ==== static variables
    /** number of attributes */
    private  int numColumns;

    // ==== instance variables
    /** session share */
    private SessionShare sshare;
    /** attribute names */
    private HeaderValue[] headers;
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
    public ProtocolBrowserDataModel(SessionShare sshare, ProtocolKeywords keywords, 
                                    String objType, String fullpath) {
        this.sshare = sshare;

        // If all keywords are not in DB then we will not have columns for
        // all of them.  Determine the # of columns by eliminating keywords
        // which are not in the DB and create a list of valid header keywords
        ArrayList<String> keywordList = new ArrayList<String>();
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.getKeywordByIndex(i);
            // For objType=protocol, get user and display keywords
            // For other objType (Data) get "data-display" keywords
            if(objType.equals(Shuf.DB_PROTOCOL) && fullpath == null) {
                if(!keyword.type.equals("protocol-display"))
                    continue;
            }
            else if(objType.equals(Shuf.DB_PROTOCOL) && fullpath != null) {
                if(!keyword.type.equals("studycard-display"))
                    continue;
            }
            else {
                if(!keyword.type.equals("data-display"))
                    continue;
            }
              
            String keywordName = keyword.getName();
            
            // Is this a valid attribute in the DB?
            ShufDBManager dbManager = ShufDBManager.getdbManager();
            boolean foundit = dbManager.isAttributeInDB(objType, keywordName);
            if(foundit) {
                // Valid, add it
                keywordList.add(keywordName);
               
            }
        }
        numColumns = keywordList.size();


        headers = new HeaderValue[numColumns];
        
        for(int i=0; i < numColumns; i++) {
            headers[i] = new HeaderValue(keywordList.get(i), true);
        }        
        
        setResults(new SearchResults[0]);
    } // SessionShare()

    /**
     * set search results
     * @param results search results
     */

    /******************************************************************
     * Summary: Fill rmatrix for the result window with the search results.
     *
     *****************************************************************/

    public synchronized void setResults(SearchResults[] results) {
        String hostFullpath;
        ShufflerItem item;
        HoldingDataModel holdingDataModel;
        int inc;
        SearchResults[] results_out;
        Object[] cont;
        SearchResults matchRow=null;

        if(results == null)
            return;

        

        ShufflerService shufflerService = sshare.shufflerService();
        Hashtable searchCache = sshare.searchCache();


        inc = 0;
        results_out = results;
        
        // Turn off editing.  It will only come back on if necessary.
        editColumn = -1;

        // derive rmatrix
        int len = results.length + inc;
        if(len < 1)
            len = 1;
        
        rmatrix = new SearchResults[len][numColumns +6];     

        for (int i = 0; i < results.length; i++)  {
            matchRow = results[i];
            cont = (Object[])(matchRow.content);

            // Element numColumns +4 is the host name and element numColumns +2 is the fullpath.
            // hostFullpath is create as the key.
            hostFullpath = cont[numColumns +4]+":"+cont[numColumns+2];
//            Object[] contents = (Object[])searchCache.get(hostFullpath);
            
            Object[] contents = (Object [])results[i].content;

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


            for (int j = 0; j < contents.length -1; j++) {
                // This rmatrix is where the actual panel gets its info
                String obj = (String)contents[j+1];
                SearchResults sr = new SearchResults(matchRow.match, 
                        contents[j+1], 
                        new ID ((String)contents[numColumns +2]),
                        (String)cont[numColumns  +4], 
                        (String)cont[numColumns +3]);
                rmatrix[i+inc][j] = sr;
            }

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
            obj = results[row].id;
        }
        catch (ArrayIndexOutOfBoundsException e) {
            Messages.postError("Row index selected is out of bounds");
            Messages.writeStackTrace(e, "Error caught in getID");
            return null;
        }

        return obj;

    } // getID()

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
        Object obj = rmatrix[r][c];
        return obj;
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
        if (index >= 0 && index <= numColumns) {
            return headers[index].getText();
        } else {
            return "?";
        }
    } // getAttribute()


    public void setAttribute(int index, String newText) {
        if (index >= 0 && index <= numColumns) {
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
    */
    public String[] getHeaderInfo(TableColumnModel colModel) {
        int i;
       
        String[] headerList = new String[numColumns +2];
        for(i=0; i < numColumns; i++) {
            headerList[i] = new String(getAttribute(i));

            TableColumn col = colModel.getColumn(i);
            HeaderValue hValue = (HeaderValue)col.getHeaderValue();

            if(hValue.isKeyed()) {
                headerList[numColumns] = new String(headerList[i]);
                if(hValue.isAscending()) 
                    headerList[numColumns +1] = new String("ASC");
                else
                    headerList[numColumns +1] = new String("DESC");
            }
        }
        // If no sorting column is set, default to first column
        if(headerList[numColumns] == null) {
            ProtocolKeywords keywords = ProtocolBrowser.getKeywords();
            ArrayList<String> keywordList = keywords.getKeywordColumnList();
            // Get the first one
            headerList[numColumns] = keywordList.get(0);
            headerList[numColumns +1] = "ASC";
        }

        return headerList;
    }


    /************************************************** <pre>
     * Summary: Return the default header info as per args and fill member
     *          variable, headers.  headerList[numColumns] = sort column
     *          headerList[numColumns +1] = sort direction
     </pre> **************************************************/
//    public String[] createDefaultHeaderInfo(ProtocolKeywords keywords){
//         
//        // add 2, one for sort column and one for sort direction
//        String[] headerList = new String[numColumns +2];
//
//        // Add the keyword-search and protocol-display keyword names to be
//        // the columns
//        for(int i=0, j=0; i < keywords.size(); i++) {
//            ProtocolKeyword keyword = keywords.getKeywordByIndex(i);
//            if(!keyword.type.equals("keyword-search") && (!keyword.type.equals("protocol-display")))
//                continue;
//
//            String keywordName = keyword.getName();
//            // If first column, set it for the sort column
//            if(j==0) {
//                // Set first column as the sort column
//                headerList[numColumns] = keywordName;
//                headerList[numColumns+1] = "ASC";
//            }
//
//            headerList[j++] = keywordName;
//
//        }
//        
//        return headerList;
//    }
//    
} // class ProtocolBrowserDataModel
