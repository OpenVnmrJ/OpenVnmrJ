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
import java.util.*;
import javax.swing.*;
import javax.swing.table.*;
import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;

/********************************************************** <pre>
 * Summary: Holds one shuffler statement sentence.
 *
 * @author  Glenn Sullivan
 * Details:
 *   A StatementDefinition object is one shuffler statement sentence.
 *   There are only one of these current at any one time.
 * 
 </pre> **********************************************************/

public class StatementDefinition extends JComponent {
    // ==== instance variables
    /** flow layout */
    protected FlowLayout flowLayout;
    /** List of StatmementElement's comprising the statement */
    private ArrayList elementList;
    /** Menu string and statement type identifier */
    private String menuString=null;
    /** Should this statement be limited by the browser's directory */
    private String browserlimited=null;
    /** Object type this statement belongs to */
    private String objType=null;
    /** Action Macro name.  This is called by actOnThisItem() when a
     *  ShufflerItem is double clicked or dragged someplace.  This defaults
     *  to the value set here, but it can be set to a different macro by
     *  any shuffler statement with a line like:
     *     < ActionMacro value="locaction2"/>   
     */
    private String actionMacro="locaction";
    /** Action Command.  If this is set to null, the actionMacro is executed.
     *  If actionCommand is non null, it is executed instead of actionMacro.
     *  It can be set for any shuffler statement with a line like:
     *     < ActionCommand value="vnmrjcmd('window','1 2')"
     */
    private String actionCommand=null;
    

    /************************************************** <pre>
     * Summary: constructor, create display elements.
     *
     * Details:
     *    All members start off null or 0.  We then fill in values
     *    if and only if an appropriate elementType is encountered
     *    in the list.  Thus for elementTypes which are not included,
     *    will lead to null or 0 for the corresponding member.
     *    eList is an ArrayList of StatementElement's.  They should
     *    be put into the ArrayList in the order they should appear
     *    in the Statement display.
     *
     </pre> **************************************************/
    public StatementDefinition(ArrayList eList)  throws Exception {
        StatementElement element;
        String etype;
        String evalues[];
        ArrayList menuList;
        EPopButton  popupMenu;  
        DateRangePanel dateRangePanel;
        boolean gotColumns=false;
        boolean gotSort=false;


        flowLayout = new FlowLayout(FlowLayout.LEFT, 3, 0);
        setLayout(flowLayout);


        // Set the class member
        elementList = eList;

        ShufDBManager dbManager = ShufDBManager.getdbManager();


        // Go thru the list of elements and Create the java display elements
        // on the order they were put into the List.
        for(int j=0; j < elementList.size(); j++) {
            element = (StatementElement) elementList.get(j);
            etype = element.getelementType();
            evalues = element.getelementValues();
            // The first item must be ObjectType
            if(j == 0 && !etype.equals("ObjectType")) {
                Messages.postError("Each shuffler statement defined must\n   " +
                                " contain an ObjectType as the first element.");
                throw  new Exception();
            }

            if(etype.equals("ObjectType")) {
                // Is a value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No value for " + etype);
                    throw  new Exception();
                }
                // Is the value valid?
                boolean foundit = false;
                for(int i=0; i < Shuf.OBJTYPE_LIST.length; i++) {
                    if(evalues[0].equals(Shuf.OBJTYPE_LIST[i])) {
                        foundit = true;
                        break;
                    }
                }

                if(!foundit){
                    Messages.postError("Error in Shuffler Statement.\n    " + 
                                    etype + " = " + evalues[0] + " not valid");
                    throw  new Exception();
                }
                

                element.setdisplayElement(false);       // Always false
                // Save the object type for use with other elements below.
                // and for identifying this StatementDefinition object.
                objType = evalues[0];

                // No display element to create.
            }
            else if(etype.equals("UserType")) {
                // Are values specified?
                if(evalues.length < 3) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "We need 3 values for " + etype);
                    throw  new Exception();
                }
                if(element.getdisplayElement()) {
                    // Create empty menu to be filled later as needed
                    ArrayList menulist = new ArrayList();

                    // Create a pop up menu
                    popupMenu = new EPopButton(menulist);
                    popupMenu.setOpaque(false);
                    add(popupMenu);

                    // Save the widget reference in the element itself
                    element.setjcomponent(popupMenu);

                    // assign it a listener
                    popupMenu.addPopListener(new EPopListener() {
                        public void popHappened(String popStr, 
                                                StatementElement element) {
                            // Update the history statement
                            SessionShare sshare = ResultTable.getSshare();
                            StatementHistory history =sshare.statementHistory();
                            String[] values = element.getelementValues();
                            // The first value will be which UserType, ie
                            // owner, investigator etc.  This is the key
                            // for the Hashtable.
                            history.append(values[0], popStr);
                        }
                    }, element);
                }
            }
            else if(etype.startsWith("Attribute-")) {
                // Is a default value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No default value for " + etype);
                    throw  new Exception();
                }
                String digit = etype.substring(etype.length() -1);
                // Limit to 6
                Integer digitInt = Integer.valueOf(digit);
                int digitint = digitInt.intValue();
                if(digitint > 6) {
                    Messages.postError("Maximum of 6 Attribute's allowed");
                    throw  new Exception();
                }
                
                // Be sure we have an AttrValue-# to go with this.
                String attrname = new String("AttrValue-" + digit);
                StatementElement valueElement = getElementThisEtype(attrname);
                // Do we have the matching AttrValue-#?
                if(valueElement == null) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                             " Missing " + attrname + " to go with " + etype);
                    throw  new Exception();
                }


                // If we are supposed to display this element, create it.
                // If the user wants the value displayed but fixed,
                // Then don't display it, and put the fixed value in
                // a label as well as in the fixed attribute.
                if(element.getdisplayElement()) {
                    // Create empty menu to be filled later as needed.
                    // Search for "update the Attribute- menu" below
                    // for where it is done.
                    ArrayList values = new ArrayList();

                    // Create a pop up menu
                    popupMenu = new EPopButton(values);
                    popupMenu.setOpaque(false);
                    add(popupMenu);

                    // Save the widget reference in the element itself
                    element.setjcomponent(popupMenu);
                    // assign it a listener
                    popupMenu.addPopListener(new EPopListener() {
                        public void popHappened(String popStr, 
                                                StatementElement element) {
                            // With the change to allow editable fields and
                            // wildcards, we could receive an invalid entry
                            // here.  We need to disallow wildcards '%', and
                            // check the entry for validity.
                            if(popStr.indexOf("%") != -1) {
                                Messages.postError("Cannot use wildcards '%' "
                                                   + "for Attribute names.");
                                return;
                            }
                            
                            // To see if this attribute is valid, I need to 
                            // know the objType.  Get it from the current
                            // statement.
                            ShufDBManager dbm = ShufDBManager.getdbManager();
                            SessionShare ss = ResultTable.getSshare();
                            StatementHistory his = ss.statementHistory();
                            Hashtable statement = his.getCurrentStatement();
                            String objType = 
                                        (String)statement.get("ObjectType");
                            if(!dbm.isAttributeInDB(objType, popStr)) {
                                Messages.postError("The Attribute name " 
                                    + popStr + " is not valid for " + objType);
                                return;
                            }



                            // Update the history statement
                            SessionShare sshare = ResultTable.getSshare();
                            StatementHistory history =sshare.statementHistory();
                            // If we have the element for Attrbute-1, here
                            // then update the key Attrbute-1 with the
                            // new value from the menu choice.
                            history.append(element.getelementType(), popStr);
                        }
                    }, element);
                }
            }
            else if(etype.startsWith("AttrValue-")) {
                // Is a default value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    "  +
                                       "No default value for " + etype);
                    throw  new Exception();
                }
                // Create the elementtype name which will be Attribute-#
                // where # is the same digit as the final character
                // in the current etype.
                String digit = etype.substring(etype.length() -1);
                Integer digitInt = Integer.valueOf(digit);
                int digitint = digitInt.intValue();

                if(digitint > 6) {
                    Messages.postError("Maximum of 6 AttrValue's allowed");
                    throw  new Exception();
                }

                String attrname = new String("Attribute-" + digit);
                // Get the current value for this elementtype from
                // its element.  First get the element.
                StatementElement parentElement = getElementThisEtype(attrname);
                // Do we have the matching Attribute-#?
                if(parentElement == null) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "Missing " + attrname + 
                                       " to go with " + etype);
                    throw  new Exception();
                }

                // If we are supposed to display this element, create it.
                if(element.getdisplayElement()) {
                    // Create empty menu to be filled later as needed
                    menuList = new ArrayList();

                    // Create a pop up menu
                    popupMenu = new EPopButton(menuList);
                    popupMenu.setOpaque(false);
                    add(popupMenu);

                    // Save the widget reference in the element itself
                    element.setjcomponent(popupMenu);

                    // assign it a listener
                    popupMenu.addPopListener(new EPopListener() {
                        public void popHappened(String popStr, 
                                                StatementElement element) {
                            // Update the history statement
                            SessionShare sshare = ResultTable.getSshare();
                            StatementHistory history =sshare.statementHistory();
                            // If we have the element for AttrValue-1, here
                            // then update the key AttrValue-1 with the
                            // new value from the menu choice.
                            history.append(element.getelementType(), popStr);
                        }
                    }, element);
                }
            }
            else if(etype.startsWith("BoolAttribute-")) {
                // Is a default value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No default value for " + etype);
                    throw  new Exception();
                }
                // Be sure we have an AttrValue-# to go with this.
                String digit = etype.substring(etype.length() -1);
                Integer digitInt = Integer.valueOf(digit);
                int digitint = digitInt.intValue();
                if(digitint > 6) {
                    Messages.postError("Maximum of 6 BoolAttribute's allowed");
                    throw new Exception();
                }
                String attrname = new String("BoolAttrValue-" + digit);
                StatementElement valueElement = getElementThisEtype(attrname);
                // Do we have the matching BoolAttrValue-#?
                if(valueElement == null) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "Missing " + attrname + 
                                       " to go with " + etype);
                    throw  new Exception();
                }
                // If we are supposed to display this element, create it.
                // If the user wants the value displayed but fixed,
                // Then don't display it, and put the fixed value in
                // a label as well as in the fixed attribute.
                if(element.getdisplayElement()) {
                    // create an empty menu to be update later
                    ArrayList values = new ArrayList();

                    // Create a pop up menu
                    popupMenu = new EPopButton(values);
                    popupMenu.setOpaque(false);
                    add(popupMenu);

                    // Save the widget reference in the element itself
                    element.setjcomponent(popupMenu);
                    // assign it a listener
                    popupMenu.addPopListener(new EPopListener() {
                        public void popHappened(String popStr, 
                                                StatementElement element) {
                            // Update the history statement
                            SessionShare sshare = ResultTable.getSshare();
                            StatementHistory history =sshare.statementHistory();
                            // If we have the element for BoolAttrbute-1, here
                            // then update the key BoolAttrbute-1 with the
                            // new value from the menu choice.
                            history.append(element.getelementType(), popStr);
                        }
                    }, element);
                }
            }
            else if(etype.startsWith("BoolAttrValue-")) {
                // Is a default value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No default value for " + etype);
                    throw  new Exception();
                }
                // Create the elementtype name which will be BoolAttribute-#
                // where # is the same digit as the final character
                // in the current etype.
                String digit = etype.substring(etype.length() -1);
                Integer digitInt = Integer.valueOf(digit);
                int digitint = digitInt.intValue();
                if(digitint > 6) {
                    Messages.postError("Maximum of 6 BoolAttrValue's allowed");
                    throw new Exception();
                }
                String attrname = new String("BoolAttribute-" + digit);
                // Get the current value for this elementtype from
                // its element.  First get the element.
                StatementElement parentElement = getElementThisEtype(attrname);
                // Do we have the matching BoolAttribute-#?
                if(parentElement == null) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "Missing " + attrname + 
                                       " to go with " + etype);
                    throw  new Exception();
                }

                // If we are supposed to display this element, create it.
                if(element.getdisplayElement()) {
                    // Create empty menu to be update later
                    menuList = new ArrayList();

                    // Create a pop up menu
                    popupMenu = new EPopButton(menuList);
                    popupMenu.setOpaque(false);
                    add(popupMenu);

                    // Save the widget reference in the element itself
                    element.setjcomponent(popupMenu);

                    // assign it a listener
                    popupMenu.addPopListener(new EPopListener() {
                        public void popHappened(String popStr, 
                                                StatementElement element) {
                            // Update the history statement
                            SessionShare sshare = ResultTable.getSshare();
                            StatementHistory history =sshare.statementHistory();
                            // If we have the element for BoolAttrValue-1, here
                            // then update the key BoolAttrValue-1 with the
                            // new value from the menu choice.
                            history.append(element.getelementType(), popStr);
                        }
                    }, element);
                }
            }
//          else if(etype.equals("Tag")) {
//              // Is a default value specified?
//              if(evalues.length < 1) {
//                  Messages.postError("Error in Shuffler Statement.\n    " +
//                                     "No default value for " + etype);
//                  throw  new Exception();
//              }
// element.setdisplayElement(false);
//          }
            else if(etype.equals("Date")) {
                // Is a default value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No default value for " + etype);
                    throw  new Exception();
                }
                // Be sure we have a Calendar to to with this
                StatementElement calElement = getElementThisEtype("Calendar");
                if(calElement == null) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "Missing " + "Calendar" + 
                                       " to go with " + etype);
                    throw  new Exception();
                }

                if(element.getdisplayElement()) {
                    // Do not create an empty menu here.  It is not filled
                    // later.
                    // Get all of the attribute names in the DB and then
                    // keep only the ones which start with time_.
                    // Put those into the menu.
                    ArrayList values = 
                           FillDBManager.attrList.getAttrNamesLimited(objType);
                    ArrayList timevalues = new ArrayList();
                    String str;

                    for(int i=0; i < values.size(); i++) {
                        str = (String) values.get(i);
                        if(str.startsWith("time_"))
                            timevalues.add(str);
                    }
                    // Create a pop up menu

                    popupMenu = new EPopButton(timevalues);
                    popupMenu.setOpaque(false);

                    add(popupMenu);

                    // Save the widget reference in the element itself
                    element.setjcomponent(popupMenu);
                    // assign it a listener
                    popupMenu.addPopListener(new EPopListener() {
                        public void popHappened(String popStr, 
                                                StatementElement element) {
                            // Update the history statement
                            SessionShare sshare = ResultTable.getSshare();
                            StatementHistory history =sshare.statementHistory();
                            // If we have the element for Attrbute-1, here
                            // then update the key Attrbute-1 with the
                            // new value from the menu choice.
                            history.append(element.getelementType(), popStr);
                        }
                    }, element);

                }

            }
            else if(etype.equals("Calendar")) {
                // Is a default value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No default value for " + etype);
                    throw  new Exception(); 
                }
                // Be sure we have a Date to go with this.
                StatementElement dateElement = getElementThisEtype("Date");
                if(dateElement == null) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "Missing " + "Date" + 
                                       " to go with " + etype);
                    throw  new Exception();
                }
                // Is the value valid?
                if(!(evalues[0].equals(DateRangePanel.ALL) || 
                     evalues[0].equals(DateRangePanel.SINCE) ||
                     evalues[0].equals(DateRangePanel.BEFORE) ||
                     evalues[0].equals(DateRangePanel.BETWEEN))){
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                    etype + " = " + evalues[0] + " not valid");
                    throw  new Exception();
                }

                if(element.getdisplayElement()) {
                    GregorianCalendar date1 = new GregorianCalendar(1997, 0, 1);
                    GregorianCalendar date2 = new GregorianCalendar(2000, 0, 1);
                    // The Listener is set in DateRangePanel()
                    dateRangePanel = new DateRangePanel(date1, date2);
                    element.setjcomponent(dateRangePanel);
                    add(dateRangePanel);
                }
            }
            else if(etype.equals("Label")) {
                // Is a value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No value for " + etype);
                    throw  new Exception();
                }
                JLabel label = new JLabel(evalues[0]);
//                label.setForeground(Color.black);
                add(label);
                // Save the widget reference in the element itself
                element.setjcomponent(label);
            }
            else if(etype.equals("MenuString")) {
                // Is a value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No value for " + etype);
                    throw  new Exception();
                }
                element.setdisplayElement(false);       // Always false
                // Save the menu String for identification of this 
                // StatementDefinition.
                menuString = evalues[0];

                // readStatementDefinitionFile puts the MenuStrings 
                // into the spotter menu.
            }
            else if(etype.equals("ActionMacro")) {
                // Is a value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No value for " + etype);
                    throw  new Exception();
                }
                element.setdisplayElement(false);       // Always false
                // Save the menu String for identification of this 
                // StatementDefinition.
                actionMacro = evalues[0];
            }
            else if(etype.equals("ActionCommand")) {
                // Is a value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No value for " + etype);
                    throw  new Exception();
                }
                element.setdisplayElement(false);       // Always false
                // Save the menu String for identification of this 
                // StatementDefinition.
                actionCommand = evalues[0];
            }
            else if(etype.equals("Columns")) {
                // Are values specified?
                if(evalues.length < 3) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "3 values are needed for " +etype);
                    throw  new Exception(); 
                }
                element.setdisplayElement(false);       // Always false
                gotColumns = true;
            }
            else if(etype.equals("Sort")) {
                // Are values specified?
                if(evalues.length < 2) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "2 Values are needed for " + etype);
                    throw  new Exception();
                }
                // Is the value valid?
                if(!(evalues[1].equals(Shuf.DB_SORT_DESCENDING) || 
                     evalues[1].equals(Shuf.DB_SORT_ASCENDING))){
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       etype + " (Direction of Sort) = " + 
                                       evalues[1] + " not valid");
                    throw  new Exception();
                }

                element.setdisplayElement(false);       // Always false
                gotSort = true;
            }
            else if(etype.equals("BrowserLimited")) {
                // Is a value specified?
                if(evalues.length < 1) {
                    Messages.postError("Error in Shuffler Statement.\n    " +
                                       "No value for " + etype);
                    throw  new Exception();
                }
                browserlimited = evalues[0];
            }
            else {
                Messages.postError("etype = " + etype + 
                                   " in locator_statement_xxx file Not Known.");

            }
        }
        // Check for existance of Columns and Sort.  Else we crash with
        // no useful information.
        if(!gotColumns) {
            Messages.postError("Each shuffler statement must\n    " +
                               "have a 'Columns' item" +
                               "with 3 values separated by '$'.");
            throw  new Exception();
        }
        if(!gotSort) {
            Messages.postError("Each shuffler statement must\n    " +
                               "have a 'Sort' item" +
                               "with 2 values separated by '$'.");
            throw  new Exception();
        }

        // Add on to each non trash statement an attribute host_fullpath so that
        // we can have the browser limit the items displayed.
        // This not be displayed and should be exclusive. 
        if(!objType.equals(Shuf.DB_TRASH)) {
            StatementElement stElem;
            stElem = new StatementElement("dirLimitAttribute",
                                          "host_fullpath", false);
            elementList.add(stElem);
            stElem = new StatementElement("dirLimitValues",
                                          "all", false);
            elementList.add(stElem);
        }

    }

    /**
     * update statement values
     * @param statement table of values
     */
    public void updateValues(Hashtable statement, boolean force) {
        String str=null;
        String str1, str2, str3, val;
        int numadded;
        String string=null;
        StatementElement element;
        EPopButton popButton;
        DateRangePanel dateRangePanel;
        String etype;
        Double dcolWidth0;
        Double dcolWidth1;
        Double dcolWidth2;
        Double dcolWidth3;
        int    icolWidth0;
        int    icolWidth1;
        int    icolWidth2;
        int    icolWidth3;


        // Get TableColumn's 
        ResultTable resultTable = Shuffler.getresultTable();
        TableColumnModel colModel = resultTable.getColumnModel();
        TableColumn col0 = colModel.getColumn(0);
        TableColumn col1 = colModel.getColumn(1);
        TableColumn col2 = colModel.getColumn(2);
        TableColumn col3 = colModel.getColumn(3);

        // Get total width of the current table
        double colWidthTotal = colModel.getTotalColumnWidth();

        // Get column widths in % from statement
        dcolWidth0 = (Double)statement.get("colWidth0");
        dcolWidth1 = (Double)statement.get("colWidth1");
        dcolWidth2 = (Double)statement.get("colWidth2");
        dcolWidth3 = (Double)statement.get("colWidth3");

        // If these values are not set in the statement, set the first
        // column to 12 and equally divide the other 3 columns.
        if(dcolWidth3 == null) {
            icolWidth0 = 12;
            icolWidth1 = (int)((colWidthTotal -12.0) / 3.0);
            icolWidth2 = (int)((colWidthTotal -12.0) / 3.0);
            icolWidth3 = (int)((colWidthTotal -12.0) / 3.0);
        }
        else {
            // Calc width in pixels
            icolWidth0 = (int)(dcolWidth0.doubleValue() * colWidthTotal);
            icolWidth1 = (int)(dcolWidth1.doubleValue() * colWidthTotal);
            icolWidth2 = (int)(dcolWidth2.doubleValue() * colWidthTotal);
            icolWidth3 = (int)(dcolWidth3.doubleValue() * colWidthTotal);
        }
        // Set the widths
        col0.setPreferredWidth(icolWidth0);
        col1.setPreferredWidth(icolWidth1);
        col2.setPreferredWidth(icolWidth2);
        col3.setPreferredWidth(icolWidth3);

        boolean labelSet = false;
        for(int i=0; i < elementList.size(); i++) {
            element = (StatementElement) elementList.get(i);
            // If display set to false, then there is nothing to update.
            if(element.getdisplayElement()) {
                etype = (String) element.getelementType();
                if(etype.equals("Calendar")) {
                    dateRangePanel = (DateRangePanel)element.getjcomponent();
                    dateRangePanel.updateValues(statement);
                }
                else if(etype.startsWith("Label")) {
                    if(labelSet == false) {
                        // Only allow Label change for internal use statements.
                        // There is only one type of Label, and some other
                        // statements have two or more labels.  If this code
                        // is allowed to execute, it will set them all to the
                        // same string.  Thus, internal use statements should
                        // have only one label.  I have also set it up to only
                        // change the first label encountered just in case that
                        // helps in the future.
                        
                        // See if this is an 'internal use' type
                        String type = (String) statement.get("MenuString");
                        if(!type.endsWith("internal use"))
                            continue;  // not internal use, skip this label

                        // Get the value that was put into the statement
                        String newLabelVal = (String) statement.get("Label");

                        if(newLabelVal != null) {
                            JLabel label = (JLabel) element.getjcomponent();
                            String oldLabel = label.getText();
                            
                            // If we are doing the right thing, the start of
                            // the new and old labels should be the same.
                            if(newLabelVal.startsWith(oldLabel.substring(0,10)))
                                label.setText(newLabelVal);
                        }
                        // disallow setting any more labels in this statement.
                        labelSet = true;
                    }
                }

                // All others besides Calendar
                else { 
                    popButton = (EPopButton) element.getjcomponent();
            
                    // UserType is special because we need to get the value
                    // from statement for UserType first.  This will be
                    // something like owner, investigator etc.  Then we need
                    // to get the value from statement for this owner or 
                    // whatever.
                    if(etype.equals("UserType")) {
                        string = (String) statement.get(etype); 
                        str = (String) statement.get(string);

                        // Lets update the menu list in case something
                        // has be added to the DB.
                        ArrayList menulist;
                        menulist = getUserMenuList(string, Shuf.DB_VNMR_DATA);
                        popButton.resetMenuChoices(menulist);
                    }
                    // Date is special because it is the trigger for the
                    // calenders as well as the popbutton for which date.
                    else if(etype.equals("Date")) {
                        str = (String) statement.get(etype);
                    }
                    // Attribute- is special because we may need to update
                    // the menu for AttrValue- to go with this.
                    else if(etype.startsWith("Attribute-")) {
                        str = (String) statement.get(etype); 
                        String curText = popButton.getText();
                        // If this value has changed, we need to update
                        // the AttrValue menu list and the Attribute- menu.
                        // After adding the ability to edit item and hit a
                        // return, this became a problem.  The item value gets
                        // updated upon hitting the return, thus, when arriving
                        // here, the new value and the text value will always
                        // be equal.  So, stop testing.  It just means that if
                        // the user selects the current item from the menu,
                        // we will update the Value menu anyway.
                        // Create the name for the AttrValue
                        String digit = etype.substring(etype.length() -1);
                        String attrval = new String("AttrValue-" + digit);

                        // Get the AttrValue element
                        StatementElement valueElement = 
                            getElementThisEtype(attrval);
                        // Get the new values list
                        ShufDBManager dbMg = ShufDBManager.getdbManager();
                        ArrayList list = dbMg.attrList.getAttrValueListSort(
                            str, objType);
                        if(list == null) {
                            list = new ArrayList();
                        }
                        list.add(Shuf.SEPARATOR);
                        list.add("all");

                        // Now add any previous AttrValue- values to the top
                        str1 = str.concat("-prev-1");
                        str2 = str.concat("-prev-2");
                        str3 = str.concat("-prev-3");

                        numadded = 0;
                        val = (String)statement.get(str3);
                        if(val != null) {
                            list.add(0, val);
                            numadded++;
                        }
                        val = (String)statement.get(str2);
                        if(val != null) {
                            list.add(0, val);
                            numadded++;
                        }
                        val = (String)statement.get(str1);
                        if(val != null) {
                            list.add(0, val);
                            numadded++;
                        }
                        if(numadded > 0)
                            list.add(numadded, Shuf.SEPARATOR);

                        list.trimToSize();

                        // Get the AttrValue-# jcomponent
                        EPopButton popup = (EPopButton) 
                            valueElement.getjcomponent();
                        // Set the menu list to the new list.
                        popup.resetMenuChoices(list);

                        // Get the current value for AttrValue-#
                        String curval = (String) statement.get(attrval);
                        boolean foundit = false;

                        // If the current value is in the menu list,
                        // then keep it.
                        for(int k=0; k < list.size(); k++) {
                            String st = (String) list.get(k);
                            if(st.equals(curval))
                                foundit = true;
                        }
                        if(!foundit) {
                            // Set a value for the AttrValue menu.
                            // Is there a -prev-1 value for this item?
                            str1 = str + "-prev-1";
                            val = (String) statement.get(str1);
                            if(val != null) {
                                // Yes, use it.
                                statement.put(attrval, val);
                            }
                            else
                                // No prev value, default to all
                                statement.put(attrval, "all");
                        }

                        // Now update the Attribute- menu to contain
                        // the prev items.
                        list = 
                            dbMg.attrList.getAttrNamesLimited(objType);
                        // Remove some attributes  from the list
                        for(int k=0; k < list.size(); k++) {
                            String st = (String) list.get(k);
                            if(st.startsWith("time_"))
                                // remove renumbers the list
                                list.remove(k--);       
                            if(st.startsWith("arch_"))
                                list.remove(k--);
                            else if(st.equals("owner"))
                                list.remove(k--);
                            else if(st.equals("investigator"))
                                list.remove(k--);
                            else if(st.startsWith("operator"))
                                list.remove(k--);
                            else if(st.startsWith("author"))
                                list.remove(k--);
                           // It will find tag0, tag1 etc.
                            else if(st.startsWith("tag"))
                                list.remove(k--);
                        }
                        // Add 'tag'
                        list.add("tag");

                        // Now add any previous Attribute- values to the top
                        str1 = etype.concat("-prev-1");
                        str2 = etype.concat("-prev-2");
                        str3 = etype.concat("-prev-3");
                        numadded = 0;
                        val = (String)statement.get(str3);
                        if(val != null) {
                            list.add(0, val);
                            numadded++;
                        }
                        val = (String)statement.get(str2);
                        if(val != null) {
                            list.add(0, val);
                            numadded++;
                        }
                        val = (String)statement.get(str1);
                        if(val != null) {
                            list.add(0, val);
                            numadded++;
                        }
                        if(numadded > 0)
                            list.add(numadded, Shuf.SEPARATOR);

                        popup = (EPopButton) 
                            element.getjcomponent();
                        // Set the menu list to the new list.
                        popup.resetMenuChoices(list);
                    }
                    // AttrValue is special because we need to add the 
                    // previous menu selections to the top of the menu.
                    // Redo the whole menu.
                    else if(etype.startsWith("AttrValue-")) {  
                        // value for this AttrValue- in statement.
                        str = (String) statement.get(etype); 
                        String curText = popButton.getText();
                        if(!curText.equals(str) || force) {
                            // Get the new values list
                            // Get the name of the parent attribute
                            String digit = etype.substring(etype.length() -1);
                            String attr = ("Attribute-" + digit);
                            String attrName = (String) statement.get(attr); 
                            if(attrName == null) {
                                String mstr;
                                mstr = (String) statement.get("Statement_type"); 
                                Messages.postError("Problem getting parent " +
                                    "attribute name for\n  " + etype + " = " 
                                    + str + " in statement \'" + mstr
                                    + "\'.");
                                attrName = "";
                            }

                            ShufDBManager dbMg = ShufDBManager.getdbManager();
                            ArrayList list = dbMg.attrList.getAttrValueListSort(
                                                         attrName, objType);
                            if(list == null) {
                                list = new ArrayList();
                            }
                            list.add(Shuf.SEPARATOR);
                            list.add("all");

                            // Now add any previous values to the top
                            str1 = attrName.concat("-prev-1");
                            str2 = attrName.concat("-prev-2");
                            str3 = attrName.concat("-prev-3");
                            numadded = 0;
                            val = (String)statement.get(str3);
                            if(val != null) {
                                list.add(0, val);
                                numadded++;
                            }
                            val = (String)statement.get(str2);
                            if(val != null) {
                                list.add(0, val);
                                numadded++;
                            }
                            val = (String)statement.get(str1);
                            if(val != null) {
                                list.add(0, val);
                                numadded++;
                            }
                            if(numadded > 0)
                                list.add(numadded, Shuf.SEPARATOR);

                            list.trimToSize();
                            EPopButton popup = (EPopButton) 
                                                element.getjcomponent();
                            // Set the menu list to the new list.
                            popup.resetMenuChoices(list);

                        }
        
                    }               
                    // BoolAttribute- is special because we may need to update
                    // the menu for BoolAttrValue- to go with this.
                    else if(etype.startsWith("BoolAttribute-")) {
                        str = (String) statement.get(etype); 
                        String curText = popButton.getText();
                        // If this value has changed, we need to update
                        // the BoolAttrValue menu list.
                        if(!curText.equals(str) || force) {
                            String eltype = element.getelementType();
                            // Create the name for the BoolAttrValue
                            String digit = eltype.substring(eltype.length() -1);
                            String attrval = new String("BoolAttrValue-" + 
                                                        digit);

                            // Get the BoolAttrValue element
                            StatementElement valueElement = 
                                              getElementThisEtype(attrval);

                            // Get the new values list
                            ShufDBManager dbMg = ShufDBManager.getdbManager();
                            ArrayList list = dbMg.attrList.getAttrValueListSort(
                                                        str, objType);
                            if(list == null) {
                                list = new ArrayList();
                            }
                            list.add(Shuf.SEPARATOR);
                            list.add("all");
                            list.trimToSize();

                            // Get the BoolAttrValue-# jcomponent
                            EPopButton popup = (EPopButton) 
                                                valueElement.getjcomponent();
                            // Set the menu list to the new list.
                            popup.resetMenuChoices(list);

                            // Get the current value for BoolAttrValue-#
                            String curval = (String) statement.get(attrval);
                            boolean foundit = false;

                            // If the current value is in the menu list,
                            // then keep it.
                            for(int k=0; k < list.size(); k++) {
                                String st = (String) list.get(k);
                                if(st.equals(curval))
                                   foundit = true;
                            }
                            // Default selection to all if current value
                            // is not a valid selection any longer.
                            if(!foundit) {
                                statement.put(attrval, "all");
                            }
                        }
                    }
                    else if(etype.startsWith("BoolAttrValue-")) {
                        // value for this BoolAttrValue- in statement.
                        str = (String) statement.get(etype); 
                        String curText = popButton.getText();
                        if(!curText.equals(str) || force) {
                            // Get the new values list
                            // Get the name of the parent attribute
                            String digit = etype.substring(etype.length() -1);
                            String attr = ("BoolAttribute-" + digit);
                            String attrName = (String) statement.get(attr); 
                            if(attrName == null) {
                                String mstr;
                                mstr = (String) statement.get("Statement_type"); 
                                Messages.postError("Problem getting parent " +
                                    "attribute name for\n  " + etype + " = " 
                                    + str + " in statement \'" + mstr
                                    + "\'.");
                                attrName = "";
                            }

                            ShufDBManager dbMg = ShufDBManager.getdbManager();
                            ArrayList list = dbMg.attrList.getAttrValueListSort(
                                                            attrName, objType);
                            if(list == null) {
                                list = new ArrayList();
                            }
                            list.add(Shuf.SEPARATOR);
                            // Use *** to mean "do nothing with this one"
                            // In fact, the sql cmd will actually include this,
                            // but unless an attribute value is actually ***,
                            // this will always fail to match anything, and thus
                            // will work properly.
                            list.add("***");

                            list.trimToSize();
                            EPopButton popup = (EPopButton) 
                                                element.getjcomponent();
                            // Set the menu list to the new list.
                            popup.resetMenuChoices(list);

                        }
                    }

                    // For all others, just get the string under the key in 
                    // etype
                    else {
                        str = (String) statement.get(etype); 
                    }


                    // Now we should have a str, set it into the jcomponent
                    if(popButton != null) {
                        if (str != null) {
                            popButton.setText(str);
                        } 
                        else {
                            popButton.setDefaultText();
                        }
                    }
                }
            }

        }
    } // updateValues()


    /************************************************** <pre>
     * Summary: Return the prefered height.
     *
     </pre> **************************************************/
    public int getPreferredHeight(int width) {
        StatementElement elem;

        setSize(width, 1);

        flowLayout.layoutContainer(this);
        Insets insets = getInsets();

        // We need a jcomponent to get the size.  Not all elements
        // have jcomponents, loop thru until we find one.
        // We need the last one in case it wraps to a new line
        // so start at the end.
        Component last=null;
        for(int i=elementList.size() -1; i >=0; i--) {
            elem = (StatementElement) elementList.get(i);
            last = elem.getjcomponent();
            if(last != null) {
                break;
            }
        }

        if(last == null) {
            Messages.postError("No java components in the statement");
            return 40;
        }

        if(last.getSize().height > 5)
            return last.getLocation().y + last.getSize().height + insets.bottom;
        else 
            return 40;

    } // getPreferredHeight()



    /************************************************** <pre>
     * Summary: Create the SQL info to be used for the DB search
     *
     *
     </pre> **************************************************/

    public SearchInfo getSearchInfo(Hashtable statement) {
        String           string;
        String           classMode=null;
        String[]         headers; 
        StatementElement element;
        String           vals[];
        String           value;
        String           attrName;
        ArrayList       accInv;
        LoginService    loginService;
        Hashtable       accessHash;
        String          curuser;
        Access          access;
        SearchInfo      info;
        int             isArray;
        ShufDBManager   dbManager = ShufDBManager.getdbManager();
        boolean         useAccess=false;

        info = new SearchInfo();

        // First Loop thru the elementList
        // In the same loop, get the current values from statement.
        for(int i=0; i < elementList.size(); i++) {
            element = (StatementElement) elementList.get(i);
            vals = element.getelementValues();
            String etype = element.getelementType();
            if(etype == null)
                continue;

            if(etype.equals("ObjectType")) {
                // Which table(vnmr_data, workspace, ...)
                info.objectType = (String) statement.get(etype);  
            }
            else if(etype.equals("UserType")) {
                // owner, operator, investigator,etc.
                info.userType = (String) statement.get(etype); 
                // User names
                info.userTypeName = (String) statement.get(info.userType);

                // This is not dynamic yet, just get it out of the element.
                info.userTypeMode = vals[2];       // exclusive or nonexclusive
            }
            else if(etype.startsWith("Attribute-")) {
                // Get the numeric value following Attribute- in etype
                // It is the last character.
                String digit = etype.substring(etype.length() -1);
                String attrvalname = new String("AttrValue-" + digit);
                value = (String) statement.get(attrvalname);

                // If set to all or every, ignore it.
                if(value != null && !value.startsWith("all") && 
                                !value.startsWith("every") &&
                                !value.startsWith("any") &&
                                !value.startsWith("***")) {
                    attrName = (String) statement.get(etype);

                    // Need to determine if array type.
                    isArray = dbManager.isParamArrayType(info.objectType,
                                                         attrName);
                    if(isArray == -1) {
                        Messages.postError(attrName +" is not in "
                                     + "xxx_param_list.  It must be in "
                                     + " the list to be used in a statement.");
                    }
                    else if(isArray == 1) {
                        info.numArrAttributes++;
                        info.arrAttributeNames.add(attrName);
                        // Set the value that goes with this Attribute-#
                        info.arrAttributeValues.add(value);
                    }
                    else if(attrName.startsWith("tag")) {
                        info.tag = value;
                    }
                    else {
                        info.numAttributes++;
                        info.attributeNames.add(attrName);
                        // Set the value that goes with this Attribute-#
                        info.attributeValues.add(value);
                        
                        // If there is a vals[1] entry, it should be
                        // either nonexclusive or exclusive.  If
                        // it is exclusive, record it as such,
                        // else set to nonexclusive.
                        if(vals.length > 1 && vals[1].equals("exclusive")) {
                            info.attributeMode.add("exclusive");
                        }
                        else {
                            info.attributeMode.add("nonexclusive");
                        }
                    }
                }
            }
            else if(etype.startsWith("BoolAttribute-")) {
                // Get the numeric value following BoolAttribute- in etype
                // It is the last character.
                String digit = etype.substring(etype.length() -1);
                String attrvalname = new String("BoolAttrValue-" + digit);
                value = (String) statement.get(attrvalname);

                // If set to all or every, ignore it.
                if(value != null && !value.startsWith("all") && 
                                !value.startsWith("every") &&
                                !value.startsWith("***")) {
                    info.numBAttributes++;
                    info.bAttributeNames.add((String) statement.get(etype));

                    // Get and set the value that goes with this Attribute-#
                    info.bAttributeValues.add(value);
                }
            }
            else if(etype.equals("Tag")) {
// I don't think we are going to use this.  We just trap for attribute = tag
            }
            else if(etype.equals("Date")) {
                info.whichDate = (String) statement.get(etype);
            }
            else if(etype.equals("Calendar")) {
                info.dateRange = (String) statement.get("DateRange");

                if(info.dateRange.equals(DateRangePanel.SINCE)) {
                    info.dateRange = Shuf.DB_DATE_SINCE;
                    info.numDates = 1;
                }
                else if(info.dateRange.equals(DateRangePanel.BEFORE)) {
                    info.dateRange = Shuf.DB_DATE_BEFORE;
                    info.numDates = 1;
                }
                else if(info.dateRange.equals(DateRangePanel.BETWEEN)) {
                    info.dateRange = Shuf.DB_DATE_BETWEEN;
                    info.numDates = 2;
                }
                else {
                    info.dateRange = Shuf.DB_DATE_ALL;
                    info.numDates = 0;
                }
                // Date
                if(info.numDates > 0) {
                    GregorianCalendar cal = 
                                (GregorianCalendar)statement.get("date-0");
                    // Note, MONTH gives 0-11, so +1 to get real months numbers.
                    info.date1 = ((cal.get(Calendar.MONTH) +1) + "/" +
                             cal.get(Calendar.DAY_OF_MONTH)  + "/" +
                             cal.get(Calendar.YEAR));
                }
                if(info.numDates == 2) {
                    GregorianCalendar cal = 
                                (GregorianCalendar)statement.get("date-1");
                    // Note, MONTH gives 0-11, so +1 to get real months numbers.
                    info.date2 = ((cal.get(Calendar.MONTH) +1) + "/" +
                             cal.get(Calendar.DAY_OF_MONTH)  + "/" +
                             cal.get(Calendar.YEAR));
                }       
            }
            else if(etype.equals("dirLimitAttribute")) {
                value = (String) statement.get("dirLimitValues");
                // If set to all, ignore it.
                if(value != null && !value.startsWith("all") && 
                                    !value.startsWith("***")) {
                    info.dirLimitAttribute = 
                                      new String("host_fullpath");
                    info.dirLimitValues = new String(value);
                }
            }


            // Skip any types not listed, like Label.
        }

        SessionShare sshare = ResultTable.getSshare();
        ShufflerService shufflerService = sshare.shufflerService();
        headers = (String[])statement.get("headers");


        // USER_TYPE element
        if(info.userTypeName != null) {
            // There is a userType element
            loginService = LoginService.getDefault();
            accessHash = loginService.getaccessHash();
            curuser = System.getProperty("user.name");
            access = (Access)accessHash.get(curuser);
            accInv = access.getlist();
            // If the selected user is everyone, then replace it with
            // the access list.
            if(info.userTypeName.equals("everyone")) {
                // If menu choice of 'everyone' and an access of 'all'
                // Then we need specify no owners
                if(!accInv.get(0).equals("all")) {
                    // everyone here means everyone we have access to.
                    // send the access list.
                    // Add accInv to userTypeNames
                    info.userTypeNames =  accInv;
                }
            }
            // If the selected user is me, replace it with the current user
            else if(info.userTypeName.equals("me"))
                info.userTypeNames.add(curuser);
            // If the selected user is me only, replace it with the 
            // current user and force exclusive
            else if(info.userTypeName.equals("me only")) {
                info.userTypeNames.add(curuser);
                info.userTypeMode = "exclusive";
            }
            // There is a userTypeName and it is not everyone nor me
            else {
                // If the selected user is not a group, use the
                // user itself.
                Hashtable groupHash = loginService.getgroupHash();
                Group group = (Group)groupHash.get(info.userTypeName);
                if(group == null) { // Not in group list
                    // Just add the selected owner to the string.
                    info.userTypeNames.add(info.userTypeName);
                }
                // If the selected owner is a group, use the group list
                else {
                    info.userTypeNames = group.getmembers();
                    String user;
                    // Go thru each group entry, and change 'me' to curuser.
                    for(int i=0; i < info.userTypeNames.size(); i++) {
                        user =  (String)info.userTypeNames.get(i);
                        if(user.equals("me")) {
                            info.userTypeNames.remove(i);
                            info.userTypeNames.add(i, curuser);
                        }
                    }

                }
            }
        }

        // Get the access list for the current user
        loginService = LoginService.getDefault();
        accessHash = loginService.getaccessHash();
        curuser = System.getProperty("user.name");
        access = (Access)accessHash.get(curuser);

        if(access == null) {
            // This should not be posible, but just in case, just give this
            // user access to himself.
            info.accessibleUsers.add(curuser);
        }
        else {
            info.accessibleUsers = access.getlist();
        }

        
        // Use the user access information for the following objTypes
        if(info.objectType.equals(Shuf.DB_VNMR_DATA) || 
                   info.objectType.equals(Shuf.DB_VNMR_PAR)  ||
                   info.objectType.equals(Shuf.DB_STUDY)  ||
                   info.objectType.equals(Shuf.DB_LCSTUDY)  ||
                   info.objectType.equals(Shuf.DB_AUTODIR)  ||
                   info.objectType.equals(Shuf.DB_IMAGE_DIR)  ||
                   info.objectType.equals(Shuf.DB_COMPUTED_DIR)  ||
                   info.objectType.equals(Shuf.DB_IMAGE_FILE)  ||
                   info.objectType.equals(Shuf.DB_PROTOCOL)) {
            useAccess = true;
        }
        
        


        if(useAccess) {
            // If the first item in the access list is 'all' then ignore
            // the list and use instead, all of the users found in the DB.
            if(info.accessibleUsers.get(0).equals("all")) {
                ArrayList strarr = shufflerService.
                    queryCategoryValues(info.objectType, "owner");
                if(strarr.size() == 0) {
                    // We show nothing in the DB, perhaps the locattrlist is
                    // either bad or not created yet.
                    // Redo the locAttrList persistence file in case it
                    // is screwed up.  If the update is already running,
                    // this will not start another one
                    UpdateAttributeList updateAttrThread = new UpdateAttributeList();
                    updateAttrThread.setPriority(Thread.MIN_PRIORITY);
                    updateAttrThread.setName("Update DB Attr List");
                    updateAttrThread.start();
                }

                // Be sure the current user is in the list even if no
                // DB entries have this owner.  A newly saved file will
                // be owned by this user before the owner makes it into the 
                // list.
                if(!strarr.contains(curuser)) {
                    strarr.add(curuser);
                }

                // Reset info.accessibleUsers with this new list
                info.accessibleUsers = strarr;
            }
        }
        // If not using access list, set to empty list
        else {
            info.accessibleUsers = new ArrayList();
        }
            
        // Get the stuff out of header.
        headers = (String[])statement.get("headers");

        // Do not allow column headers to be array types
        for(int i=0; i < 3; i++) {
            isArray = dbManager.isParamArrayType(info.objectType, headers[i]);
            if(isArray == -1) {
                Messages.postError(headers[i] + ": Column Header parameters "
                                   + "must be in xxx_param_list file.");
            }
            else if(isArray == 1) {
                Messages.postError(headers[i] + " is an array type parameter." +
                                 "\nArray type parameters cannot be used for " +
                                 "column headers.");
            }
        }
        info.attrToReturn[0] = headers[0];
        info.attrToReturn[1] = headers[1];
        info.attrToReturn[2] = headers[2];
        info.sortAttr = headers[3];
        info.sortDirection = headers[4];

        return info;
    }
    // End getSearchInfo



    /************************************************** <pre>
     * Summary: Return the menu list of Users of this type.
     *
     </pre> **************************************************/
    public ArrayList getUserMenuList(String userType, String objType) {

        ShufDBManager dbManager = ShufDBManager.getdbManager();

        // Be sure the userType is a valid user type
        if(!(userType.equals("owner") || 
             userType.startsWith("operator") ||
             userType.startsWith("author") ||
             userType.equals("investigator"))){
            Messages.postError("Error in Shuffler Statement\n    " +
                               "UserType " + userType + " not valid");
            return null;
        }
        // Create a pop up menu
        // Get list of values to put into menu.
        ArrayList list = 
                  dbManager.attrList.getAttrValueListSort(userType, objType);
        if(list == null) {
            Messages.postError("Problem getting attribute list for "
                                + userType);
            // return empty list
            return new ArrayList();

        }

        // Now trim down the list to include ONLY entries accessible
        // by this owner.
        // Get the access list.
        ArrayList menulist;
        String curuser = System.getProperty("user.name");
        Hashtable accessHash = LoginService.getaccessHash();
        Access access  = (Access) accessHash.get(curuser);
        ArrayList accList = access.getlist();
        ArrayList accGroups = access.getgroups();

        // If accList is anything but 'all'
        if(!accList.get(0).equals("all")) {
            // Go thru the list from the DB and check to see if 
            // it is an accessible user.  Skip the last entry 
            // which should be everyone.
            ArrayList alist = new ArrayList();
            for(int i=0; i < list.size() -1; i++) {
                if(accList.contains(list.get(i))) {
                                // This entry is valid, keep it
                    alist.add(list.get(i));
                }
            }
            // Now add all the groups from the access list
            for(int i=0; i < accGroups.size(); i++)
                alist.add(accGroups.get(i));

            // Now alist should contain all the strings that 
            // were valid. Add everyone and me
            alist.add(Shuf.SEPARATOR);
            alist.add("everyone");
            alist.add("me");
            alist.add("me only");

            menulist = alist;
        }
        else {  // We have access to all.
            // Add to the list any groups
            // Are there any?
            if(accGroups.size() > 0) {

                // Add to it the groups.
                for(int i=0; i < accGroups.size(); i++) {
                    list.add(0, (String) accGroups.get(i));
                }
                list.add(0, Shuf.SEPARATOR);
                list.add(0, "everyone");
                list.add(0, "me");
                list.add(0, "me only");

                menulist = list;
            }
            else {
                list.add(Shuf.SEPARATOR);
                list.add("everyone");
                menulist = list;
            }
        }

        if(DebugOutput.isSetFor("getUserMenuList"))
            Messages.postDebug("User menu list for \'" + userType +
                               "\'\n    " + menulist);
        
        return menulist;
    }



        /************************************************** <pre>
     * Summary: Get element with this etype.
     *
     * Loop through each element and compare etypes.
     * Return the first match found.
     </pre> **************************************************/
    public StatementElement getElementThisEtype(String etype) {
        StatementElement element;

        for(int i=0; i < elementList.size(); i++) {
            element = (StatementElement) elementList.get(i);
            if(element.elementType.equals(etype)) {
                return element;
            }
        }
        return null;
    }


    /************************************************** <pre>
     * Summary: Get elementList.
     *
     </pre> **************************************************/
    public ArrayList getelementList() {
        return elementList;
    }

    /************************************************** <pre>
     * Summary: Get actionMacro.
     *
     </pre> **************************************************/
    public String getactionMacro() {
        return actionMacro;
    }

    /************************************************** <pre>
     * Summary: Get actionCommand.
     *
     </pre> **************************************************/
    public String getactionCommand() {
        return actionCommand;
    }

    /************************************************** <pre>
     * Summary: Get menuString.
     *
     </pre> **************************************************/
    public String getmenuString() {
        return menuString;
    }

    /************************************************** <pre>
     * Summary: Get objType.
     *
     </pre> **************************************************/
    public String getobjType() {
        return objType;
    }

    /************************************************** <pre>
     * Output String of appropriate members.
     *
     </pre> **************************************************/
    public String toString() {
        String str;

        str = new String(objType);
        str = str.concat("  " + menuString + "\n");

        for(int i=0; i < elementList.size(); i++) {
            str = str.concat(elementList.get(i) + "\n");
        }
        return str;     
    }
}



