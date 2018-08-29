/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.util.*;
import java.io.*;

import  vnmr.util.*;
import  vnmr.ui.*;
import  vnmr.ui.shuf.*;

/**
 * There is one StatementHistory object per objType. The StatementHistory
 * object maintains a buffer of statements.  Each "statement" is simply
 * represented by a hashtable.
 * 
 * <p>StatementHistory has one additional responsibility, and that is,
 * remembering the last statement of a particular statement type.
 *
 * @author Mark Cao
 */
public class StatementHistory  implements  Serializable {
    // ==== static variables
    /** maximum buffer length */
    private static final int MAXLEN = 20;

    // ==== instance variables
    /** shuffler service */
    private ShufflerService shufflerService;
    /** History buffer. Place latest statements at end. */
    private Vector buffer=null;
    /** Buffer pointer. The buffer pointer points at the statement
     * currently being displayed. If the buffer is empty, the pointer
     * has value -1. */
    private int bufPointer=-1;
    /** The last buffer that was appended. */
    private int lastAppendedBuf;
    /** Most previous active bufPointer when append occured  */
    private int prevBufPointer;
    /** statement listeners */
    private Vector listeners;
    /** previous statements, keyed by type */
    private Hashtable prevStatements;
    /** object type this StatementHistory is used for. */
    private String objectType;

    /**
     * constructor
     * @param shufflerService
     */
    public StatementHistory(ShufflerService shufflerService, String objType,
                            Vector buf, int bufPointer){
        this.shufflerService = shufflerService;
        objectType = objType;
        this.buffer = buf;
        this.bufPointer = bufPointer;


        if(buffer == null)
            buffer = new Vector(MAXLEN);
        listeners = new Vector();
        prevStatements = new Hashtable();
    } // StatementHistory()

    /**
     * Add a statement listener. Such listeners are notified of new
     * statements.
     * @param listener listener
     */
    public void addStatementListener(StatementListener listener) {
        listeners.addElement(listener);
    } // addStatementListener

    /**
     * Append a statement to buffer. Note that append automatically
     * resets the buffer pointer to the end.
     * @param statement statement
     */
    public void append(Hashtable statement) {
        if(statement == null)
            return;

        // prior to appending, delete a statement if necessary
        while (buffer.size() >= MAXLEN) {
            buffer.removeElementAt(0);
        }
        buffer.addElement(statement.clone());
        bufPointer = buffer.size() - 1;

        String statementType = (String)statement.get("Statement_type");
        prevStatements.put(statementType, statement);

        for (Enumeration en = listeners.elements(); en.hasMoreElements(); ) {
            StatementListener listener = (StatementListener)en.nextElement();
            listener.newStatement(statement);
            listener.backMovabilityChanged(canGoBack());
            listener.forwardMovabilityChanged(canGoForward());
        }
    } // append()


    /**
     * Take a copy of the current statement, modify it with the given
     * key/value pair, and append the new (modified) statement to the
     * history buffer.
     * @param key key
     * @param value value
     */
    public void append(String key, Object value) {
        String str1, str2, str3;
        String val;

        Hashtable statement = getCurrentStatement();
        if (statement != null) {
            Hashtable newStatement = (Hashtable)statement.clone();
            newStatement.put(key, value);


            // If Attribute- set AttrValue- to its prev value or all.
            // If Attribute- or AttrValue-, save the previous 3 values.
            if(key.startsWith("Attribute-")) { 
                // If Attribute- set AttrValue- to its prev value or all.
                String digit = key.substring(key.length() -1);
                String attrVal = new String("AttrValue-" + digit);
                String inVal = value.toString();
                String attrPrev = inVal.concat("-prev-1");
                // Try to get the prev value for this Attribute
                val = (String) statement.get(attrPrev);
                if(val != null) {
                    // Yes, use it.
                    newStatement.put(attrVal, val);
                }
                else
                    // No prev value, default to all
                    newStatement.put(attrVal, "all");

                // Only save if it has changed since the last time we
                // saved a prev-1
                // Create the three key names for the Attribute- 
                str1 = key.concat("-prev-1");
                str2 = key.concat("-prev-2");
                str3 = key.concat("-prev-3");
                String prevAttrName = (String)statement.get(key);
                String prevVal = (String) statement.get(str1);
                if(prevVal == null || !prevAttrName.equals(prevVal)) {
                    // rotate -1 and -2 up to -2 and -3 if they exist.
                    val = (String)newStatement.get(str2);
                    if(val != null)
                        newStatement.put(str3, val);
                    val = (String)newStatement.get(str1);
                    if(val != null)
                        newStatement.put(str2, val);
                    // Save the previous one.
                    newStatement.put(str1, prevAttrName);
                }

                // Also save the AttrValue- value we had.
                // Create the three key names using the actual attr name
                str1 = prevAttrName.concat("-prev-1");
                str2 = prevAttrName.concat("-prev-2");
                str3 = prevAttrName.concat("-prev-3");

                // Get the current value of AttrValue-
                String curVal = (String) statement.get(attrVal);

                // Get the value of the -prev-1 entry
                prevVal = (String) statement.get(str1);

                // Only save the values if it has changed since the
                // last time we saved a prev-1
                if(prevVal == null || !curVal.equals(prevVal)) {
                    // rotate -1 and -2 up to -2 and -3 if they exist.
                    val = (String)newStatement.get(str2);
                    if(val != null)
                        newStatement.put(str3, val);
                    val = (String)newStatement.get(str1);
                    if(val != null)
                        newStatement.put(str2, val);
                    // Save the previous one.
                    newStatement.put(str1, curVal);     
                }       
            }
            else if(key.startsWith("AttrValue-")) {
                // Get the Attribute- name itself that goes with
                // this AttrValue-
                String digit = key.substring(key.length() -1);
                String attr = new String("Attribute-" + digit);
                String attrName = (String) newStatement.get(attr); 


                // Create the three key names using the actual attr name
                str1 = attrName.concat("-prev-1");
                str2 = attrName.concat("-prev-2");
                str3 = attrName.concat("-prev-3");
                // rotate -1 and -2 up to -2 and -3 if they exist.
                val = (String)newStatement.get(str2);
                if(val != null)
                    newStatement.put(str3, val);
                val = (String)newStatement.get(str1);
                if(val != null)
                    newStatement.put(str2, val);
 
                newStatement.put(str1, value);
            }
            append(newStatement);
        }
    } // append()

    /**
     * Append the last statement of the given type. Note that memory
     * of these previous statements is not limited to what's in the
     * buffer.
     *
     * <p>If the given statementType has not been encountered before,
     * query a default value from the shuffler service.
     * @param statementType statement type
     */
    public void appendLastOfType(String statementType) {
        prevBufPointer = bufPointer;
        Hashtable statement = (Hashtable)prevStatements.get(statementType);

        if (statement == null)
            statement = shufflerService.getDefaultStatement(statementType);
        append(statement);
        // If no new statement, don't allow anything to be removed later.
        if(prevBufPointer == bufPointer)
            lastAppendedBuf = -1;
        else
            lastAppendedBuf = bufPointer;
        
    } // appendLastOfType()


    /******************************************************************
     * Summary: Return the last statement of this type.
     *
     *****************************************************************/

    public Hashtable getLastOfType(String statementType) {
        prevBufPointer = bufPointer;
        Hashtable statement = (Hashtable)prevStatements.get(statementType);

        if (statement == null)
            statement = shufflerService.getDefaultStatement(statementType);

        return statement;

    }

    /************************************************** <pre>
     * Summary: Remove the last statement which was appended and restore
     *  statement to where it was before the last append.
     *
     </pre> **************************************************/

    public void removeLastAppendedStatement() {
        // Go to the most previous position
        if(prevBufPointer >= 0  && prevBufPointer < buffer.size())
           goToStatementByIndex(prevBufPointer);

        // Remove the last appended statement.
        if(lastAppendedBuf >= 0  && lastAppendedBuf  < buffer.size())
            buffer.removeElementAt(lastAppendedBuf); 

        // Fix the forward and backward arrows.
        for (Enumeration en = listeners.elements();
             en.hasMoreElements(); ) {
            StatementListener listener =
                (StatementListener)en.nextElement();
            listener.backMovabilityChanged(canGoBack());
            listener.forwardMovabilityChanged(canGoForward());
        }
    }


    /************************************************** <pre>
     * Summary: Switch to the given statement without effecting the history.
     *
     </pre> **************************************************/

    public void goToStatementByIndex(int newBufPointer) {
        if (newBufPointer >= 0 && newBufPointer < buffer.size()) {
            bufPointer = newBufPointer;
            updateWithoutNewHistory();
        }
    }

    public int getNumInHistory() {
        return buffer.size();
    }
    /**
     * is it possible to go back?
     * @return boolean
     */
    public boolean canGoBack() {
        return bufPointer > 0;
    } // canGoBack()

    /**
     * Move the buffer pointer to the previous statement.
     */
    public void goBack() {
        if (bufPointer > 0) {
            bufPointer--;
            Hashtable statement = (Hashtable)buffer.elementAt(bufPointer);
            for (Enumeration en = listeners.elements();
                 en.hasMoreElements(); ) {
                StatementListener listener =
                    (StatementListener)en.nextElement();
                listener.newStatement(statement);
                listener.backMovabilityChanged(canGoBack());
                listener.forwardMovabilityChanged(canGoForward());
            }
        }
    } // goBack()


    /**
     * is it possible to go forward?
     * @return boolean
     */
    public boolean canGoForward() {
        return bufPointer + 1 < buffer.size();
    } // canGoForward()

    /**
     * Go forward to the next statement and return that statement. 
     * Return null if going forward is not possible.
     * @return next statement
     */
    public void goForward() {
        if (bufPointer + 1 < buffer.size()) {
            bufPointer++;
            Hashtable statement = (Hashtable)buffer.elementAt(bufPointer);
            for (Enumeration en = listeners.elements();
                 en.hasMoreElements(); ) {
                StatementListener listener =
                    (StatementListener)en.nextElement();
                listener.newStatement(statement);
                listener.backMovabilityChanged(canGoBack());
                listener.forwardMovabilityChanged(canGoForward());
            }
        }
    } // goForward()

    /**
     * get current statement
     * @return current
     */
    public Hashtable getCurrentStatement() {
        if (0 <= bufPointer && bufPointer < buffer.size()) {
            return (Hashtable)buffer.elementAt(bufPointer);
        }
        else {
            String statementType = shufflerService.getDefaultStatementType();
            return shufflerService.getDefaultStatement(statementType);
        }
    } // getCurrentStatement()


    /**
     * Update the current shuffler panels, but no change has taken
     * place that caused the need for a history update.  This is 
     * primarily for use after adding or removing a file from the
     * DB so that we can get the panels updated.
     */
    public void updateWithoutNewHistory() {
        StatementListener listener;
        Enumeration en;

        // If the locator is not being used, get out of here
        if(FillDBManager.locatorOff())
            return;

        try {
            Hashtable statement = getCurrentStatement();
            // If there is nothing in 'buffer', then append this one
            if(buffer.size() == 0) {
                append(statement);
            }
            // If there is already something in 'buffer', then we will have
            // the most recent one.  So, do not append to history.
            else {
                for (en = listeners.elements(); en.hasMoreElements();) {
                    listener =(StatementListener)en.nextElement();
                    listener.newStatement(statement);
                    listener.backMovabilityChanged(canGoBack());
                    listener.forwardMovabilityChanged(canGoForward());
                }
            }
        }
        catch(Exception e) {
            Messages.postError("Problem updating locator statement");
            Messages.writeStackTrace(e);
        }

    }

    /**
     * Update the current shuffler panels, but no change has taken
     * place that caused the need for a history update.  This is 
     * primarily for use after adding or removing a file from the
     * DB so that we can get the panels updated.  Disallow statements 
     * ending in 'internal use'.  They are for temp internal use and 
     * should not be gone to as a default history.
     */
    public void updateWithoutNewHistoryNotInternal() {
        StatementListener listener;
        Enumeration en;
        Hashtable statement=null;
        boolean foundOne=false;

        try {
            // Get the current statement in case the buffer is empty
            statement = getCurrentStatement();

            // Start by looking at the current Statement, if it has a 
            // menuString of 'by objtype', go to the next previous one
            // and test it until we find one which is not by that name.
            while (bufPointer >= 0 && buffer.size() > 0) {

                statement = (Hashtable)buffer.elementAt(bufPointer);
                String menuString = (String) statement.get("MenuString");
                if(!menuString.endsWith("internal use")) {
                    // We found one that is not 'internal use', so break out
                    // of here and use this statement.
                    foundOne = true;
                    break;
                }
                // try the next previous one
                bufPointer--;
            }

            // If there is already something in 'buffer', then we will have
            // the most recent one.  So, do not append to history.
            if(foundOne) {
                for (en = listeners.elements(); en.hasMoreElements();) {
                    listener =(StatementListener)en.nextElement();
                    listener.newStatement(statement);
                    listener.backMovabilityChanged(canGoBack());
                    listener.forwardMovabilityChanged(canGoForward());
                }
            }
            else {
                // If we did not find one, then default to the standard
                // default statement.
                String statementType =shufflerService.getDefaultStatementType();
                statement = shufflerService.getDefaultStatement(statementType);
                append(statement);
            }
        }
        catch(Exception e) {
            Messages.postError("Problem updating locator statement");
            Messages.writeStackTrace(e);
        }

    }

    /** Write out the current shuffler statement to a named file.
     *  This one is for backwards compatibility before label was used
     */
    public void writeCurStatement(String name) {
        // Simply pass name as the label as well as the name itself.
        writeCurStatement(name, name);
    }

    /** Write out the current shuffler statement to a named file.
     *
     */
    public void writeCurStatement(String name, String label) {
        Hashtable curStatement;
        //String dir, shufDir;
        String filepath;
        String filename;
        ObjectOutput out;
        //File file;
        
        curStatement = getCurrentStatement();

        //dir = System.getProperty("userdir");
        //shufDir = new String(dir + "/shuffler");
        //file = new File(shufDir);
        // If this directory does not exist, make it.
        //if(!file.exists()) {
        //    file.mkdir();
        //}

        if(label != null) {
            // Set a value in the statement with the label string
            curStatement.put("MenuLabel", label);
        }

        // Convert all spaces in the name to '_'.
        filename = name.replace(' ', '_');
        
        //filepath = new String (shufDir + "/" + filename);
        filepath=FileUtil.savePath("USER/LOCATOR/"+filename);

        try {
            out = new ObjectOutputStream(new FileOutputStream(filepath));
            // Write it out.
            out.writeObject(curStatement);
            out.close();
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
        
        // Update the spotter menu.
        for (Enumeration en = listeners.elements(); en.hasMoreElements(); ) {
            StatementListener listener = (StatementListener)en.nextElement();
            listener.saveListChanged();
        }
        
        // The following is not really necessary, but when a programmable
        // button is pressed and a new statement is written, there is no
        // visible sign that it has completed.  The following at least
        // causes a blink of the locator table area to let the user know
        // something has happened.
        SessionShare sshare = ResultTable.getSshare();        
        StatementHistory history = sshare.statementHistory();
        if(history != null)
            history.updateWithoutNewHistory();
   }

    /** Remove the Saved Statement by this name from the disk
     */
    public void removeSavedStatement(String name) {
        String filepath;
        String filename;

        // Convert all spaces in the name to '_'.
        filename = name.replace(' ', '_');
        
        //filepath = new String (shufDir + "/" + filename);
        filepath=FileUtil.savePath("USER/LOCATOR/"+filename);

        // Remove the file
        File file = new File(filepath);
        file.delete(); 

        // Update the spotter menu.
        for (Enumeration en = listeners.elements(); en.hasMoreElements(); ) {
            StatementListener listener = (StatementListener)en.nextElement();
            listener.saveListChanged();
        }
    }


    /** Read saved shuffler statement by this name.
     *
     */
     public void readNamedStatement(String name) {
        String filepath;
        String filename;
        ObjectInputStream in;
        //String dir;
        Hashtable statement;

        //dir = System.getProperty("userdir");

        // Convert all spaces in the name to '_'.
        filename = name.replace(' ', '_');

        //filepath = new String (dir + "/shuffler/" + filename);
        filepath=FileUtil.savePath("USER/LOCATOR/"+filename);
        if(filepath==null)
            return;

        try {
            in = new ObjectInputStream(new FileInputStream(filepath));
            // Read it in.
            statement = (Hashtable) in.readObject();
            in.close();
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            Messages.postWarning("This button has not been programmed. "
                    + "\n  Press and hold 3 sec to program it with the current "
                    + "Locator search.");
            return;
        }       

        // Get the objType for the statement we read in.
        // Check to see if it is different from the StatementHistory we
        // are in now.
        String objType = (String)statement.get("ObjectType");
        if(!objType.equals(objectType)) {
            // If the object type has changed, set the new object type as
            // the active one and get the StatementHistory for this object
            // type.  then call append for That StatementHistory object
            // and not the one we are in now.
            SessionShare sshare = ResultTable.getSshare();
            LocatorHistory lh = sshare.getLocatorHistory();
            // Set History Active Object type to this type.
            lh.setActiveObjType(objType);
            // Now get history for this type.
            StatementHistory history = sshare.statementHistory();
            // If history is null, this button may not have been programmed
            if(history == null)  {
                Messages.postWarning("This button has not been programmed. "
                        + "\n  Press and hold 3 sec to program it with the current "
                        + "Locator search.");
                return;
            }
            // append to history list and make it the current statement.
            history.append(statement);
        }
        else
            // append to history list and make it the current statement.
            append(statement);
        
     }

    /** Get the list of named statements from the directory 'shuffler'.
     *
     *  Return an ArrayList of ArrayLists where the inside one is
     *  a list of length 2 containing the filename and menu label as
     *  the two items.
     */
    public ArrayList getNamedStatementList() {
        File[] list;
        String dir;
        String statementType;
        File file;
        Hashtable statement;
        LocatorHistory lHistory;
        ArrayList allObjTypes;
        /** list of custom saved statement menu entries in the form 
            of a list of nameNlabel items */
        ArrayList menuList=null;
        /** list of 2 items, filename and menu label */
        ArrayList nameNlabel;
        ObjectInputStream in;

        dir=FileUtil.savePath("USER/LOCATOR");

        file = new File (dir);
        list = file.listFiles();

        // If the directory does not exist, return an empty list
        if(list == null)
            list = new File[0];

        SessionShare sshare = ResultTable.getSshare();
        lHistory = sshare.getLocatorHistory();
        ArrayList allStatementTypes = lHistory.getallStatementTypes();
        menuList = new ArrayList();
        // Go thru the files and only keep the ones with spotter types
        // which are current.
        
        for(int i=0; i < list.length; i++) {
            try {
                in = new ObjectInputStream(new FileInputStream(list[i]));
                // Read it in.
                statement = (Hashtable) in.readObject();
                in.close();
            }
            catch (ClassNotFoundException e) {
                continue;
            }
            catch (FileNotFoundException e) {
                continue;
            }
            catch (IOException e) {
                continue;
            }
            // This value is of the form objtype/menu_string 
            // eg. 'vnmr_data/by type'
            statementType = (String)statement.get("Statement_type");
            // Does this Type exist currently?
            // Need all satementTypes, in the form objtype/menu_string
            // combine objType and menuString
            if (allStatementTypes.contains(statementType)) {
                // Yes, add this to the menu list
                // Use the value of MenuLabel for the menu here, unless
                // is has the key string of '**skip**', then do not put
                // this item into the menu.
                String menuLabel = (String)statement.get("MenuLabel");
                if(menuLabel == null) {
                    nameNlabel = new ArrayList(2);
                    String name = list[i].getName();
                    // Convert all '_' to spaces.
                    name = name.replace('_', ' ');
                    nameNlabel.add(name);
                    nameNlabel.add(name);
                    
                    menuList.add(nameNlabel);
                }
                else if(!menuLabel.equals("**skip**")) {
                    nameNlabel = new ArrayList(2);
                    String name = list[i].getName();
                    // Convert all '_' to spaces.
                    name = name.replace('_', ' ');
                    nameNlabel.add(name);
                    nameNlabel.add(menuLabel);
                    menuList.add(nameNlabel);
                }
                // else if **skip**, do not add it.
            }
        }

        return menuList;
    }


    public ShufflerService getShufflerService() {
        return shufflerService;
    }

    public Vector getBuffer() {
        return buffer;
    }

    public int getBufPointer() {
        return bufPointer;
    }

    /************************************************** <pre>
     * Summary: Update the column width values for the current statement.
     *
     *   The args are fraction of the total width, where the sum of
     *   the 4 widths = 1.0.
     *
     *
     </pre> **************************************************/
    public  void updateCurStatementWidth(double colWidth0, double colWidth1,
                                         double colWidth2, double colWidth3) {
        Hashtable curStatement;
        

        curStatement = getCurrentStatement();

        curStatement.put("colWidth0", new Double(colWidth0));
        curStatement.put("colWidth1", new Double(colWidth1));
        curStatement.put("colWidth2", new Double(colWidth2));
        curStatement.put("colWidth3", new Double(colWidth3));
 
    }
} // class StatementHistory
