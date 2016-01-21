/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.util.*;

/********************************************************** <pre>
 * Summary: Container class to hold all the information required for
 *    doing a DB search.
 *
 * @author  Glenn Sullivan
 * 
 </pre> **********************************************************/

public class SearchInfo {
    /** Which table (vnmr_data, workspace, etc) */
    public String    objectType=null;
    /** Use UNION sql cmd to get results from this obj type also */
    public String    secondObjType=null;
    /** operator, investigator, owner */
    public String    userType=null;         
    /** user name */
    public String    userTypeName=null;
    /** expanded user name list */
    public ArrayList userTypeNames;
    /** exclusive or nonexclusive */
    public String    userTypeMode=null;     
    /** Number of attributes to search on */
    public int    numAttributes=0;  
    /** The list of names of attributes */
    public ArrayList attributeNames=null;   
    /** The list of values to test against. */
    public ArrayList attributeValues=null;
    /** The list of exclusive or nonexclusive values for the attributeValues */
     public ArrayList attributeMode=null;  
    /** Number of Boolean attributes to search on */
    public int    numBAttributes=0; 
    /** The list of names of bool attributes */
    public ArrayList bAttributeNames=null; 
    /** The list of values to test against. */
    public ArrayList bAttributeValues=null; 
    /** Number of Array attributes to search on */
    public int    numArrAttributes=0; 
    /** The list of names of array attributes */
    public ArrayList arrAttributeNames=null; 
    /** The list of values to test against. */
    public ArrayList arrAttributeValues=null; 
    /** Number of Array attributes to search on */
    public int    numBArrAttributes=0; 
    /** The list of names of bool array attributes */
    public ArrayList arrBAttributeNames=null; 
    /** The list of values to test against. */
    public ArrayList arrBAttributeValues=null; 
    /** date_on, date_all, date_since, etc. */
    /** Attribute, normally host_fullpath, to limit locator as per browser */
    public String dirLimitAttribute=null;
    /** List of host_fullpath values to use */
    public String dirLimitValues=null;
    public String    dateRange=Shuf.DB_DATE_ALL;           
    /** time_run, time_stored, etc. */
    public String    whichDate=null;       
    /** Number of dates to be sent */
    public int       numDates=0;            
    /** First date */
    public String    date1=null;           
    /** Second date if needed (date_between) */
    public String    date2=null;            
    /** Tag value to search on. */
    public String    tag="";
    /** accessible users list */
    public ArrayList    accessibleUsers;
    /** Attribute to sort on */
    public String    sortAttr=null;                 
    /** Direction of sort (DESC/ASC) */
    public String    sortDirection=null;   
    /** Attributes to return */
    public String[] attrToReturn;
    /** usingUnion is set by the ProtocolBrowser to get the first of 
     *  two sql cmds that will be joined with the sql UNION command.
     *  default to false. */
    public boolean usingUnion = false;
    
        
    // Allow arg defined number of attributes to be returned.
    public SearchInfo (int numReturn) {
        userTypeNames = new ArrayList();
        attributeNames = new ArrayList();
        attributeValues = new ArrayList();
        attributeMode = new ArrayList();
        bAttributeNames = new ArrayList();
        bAttributeValues = new ArrayList();
        arrAttributeNames = new ArrayList();
        arrAttributeValues = new ArrayList();
        accessibleUsers = new ArrayList();
        arrBAttributeNames = new ArrayList();
        arrBAttributeValues = new ArrayList();
        attrToReturn = new String[numReturn];
    }
    
    // Default for locator with 3 attributes to return
    public SearchInfo () {
        userTypeNames = new ArrayList();
        attributeNames = new ArrayList();
        attributeValues = new ArrayList();
        attributeMode = new ArrayList();
        bAttributeNames = new ArrayList();
        bAttributeValues = new ArrayList();
        arrAttributeNames = new ArrayList();
        arrAttributeValues = new ArrayList();
        accessibleUsers = new ArrayList();
        arrBAttributeNames = new ArrayList();
        arrBAttributeValues = new ArrayList();
        attrToReturn = new String[3];
    }
}
