/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.*;

import vnmr.sms.SmsInfoObj;
import vnmr.util.FileUtil;
import vnmr.util.Messages;
import vnmr.util.ParamIF;
import vnmr.util.UNFile;


/********************************************************** <pre>
 * Summary: Protocol Keywords for Protocol Browser
 *
 * This holds and operates on the keywords used in the Protocol Browser.
 * It holds the keyword objects in the order they are specified in the config
 * file and values and type for each keyword.
 </pre> **********************************************************/

public class ProtocolKeywords {
    public static final int RFCOIL = 1;
    public static final int GCOIL = 2;
    public static final int H1FREQ = 3;
    private static String rfcoilValue = null;
    private static String gcoilValue = null;
    private static String h1freqValue = null;
    private  ProtocolBrowser parentPanel;
    private ArrayList<ProtocolKeyword> keywords=null;
    private int activePosition=0;
    private int numSearchKeywords=0;
    private int numDisplayKeywords=0;
    private int numSCKeywords=0;
    // Protocol Tab sort info
    private String pSortKeyword="filename";
    private String pSortDirection="ASC";
    // Data Tab sort info
    private String dSortKeyword="filename";
    private String dSortDirection="ASC";

    // Constructor.
    public ProtocolKeywords(ProtocolBrowser parPanel) {
 
        // Save the parentPanel so we can trigger it to finish creating its objects
        // after the system values are complete.
        parentPanel = parPanel;
        
        if(h1freqValue == null)
            getSystemParams();
        
        createAndFillKeywords();
        
        // Count the various types of  keywords for future needs
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keywordObj = keywords.get(i);
            if(keywordObj.getType().equals("keyword-search")) 
                numSearchKeywords++;
            else if(keywordObj.getType().equals("protocol-display"))
                numDisplayKeywords++;
            else if(keywordObj.getType().equals("studycard-display"))
                numSCKeywords++;
        }
       
    }

    public void createAndFillKeywords() {
        boolean success;
        // Create an empty list.
        keywords = new ArrayList<ProtocolKeyword>();
        

        // Read the local file, or if none, read the system config
        // file /vnmr/shuffler/ProtocolBrowserKeywords.
        // Fill "keywords" based on the information read.
        success = readPersistenceOrConfigFile();
        
        // if success is false, we failed.  One reason may be that the user
        // had an old ProtocolBrowserKeywords file.  In that case, it was
        // deleted in readPersistenceOrConfigFile(), so we want to call again
        // and let it use the system file.  We don't really care about the
        // return this time, because we are not going to try again.
        if(!success)
            readPersistenceOrConfigFile();
        
        fillSystemKeywords();
    }
    
    
    // Create a keyword object and add it to the list
    public void addAKeyword(String keyword, String value, String type) {
        ProtocolKeyword keywordObj = new ProtocolKeyword(keyword, value, type);
        keywords.add(keywordObj);
        if(type.equals("protocol-display"))
            numDisplayKeywords++;
        else if(type.equals("keyword-search"))
            numSearchKeywords++;
        else if(type.equals("studycard-display"))
            numSCKeywords++;


    }

    // Remove a keyword from the list.  Only allow removal of "protocol-display"
    //  and "data-display" keywords.  If type is keyword-search, then try to
    // find and remove a protocol-display type with the same name.  That is,
    // if we are requested to remove a column from the display, remove the
    // display keyword item but keep the search item.
    public void removeAKeyword(String keyword) {
        // Find the object for this keyword
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword obj = keywords.get(i);
            // If matching keyword AND is is protocol-display, remove it
            String type = obj.getType();
            
            // If type is keyword-search, then just continue looking for keyword
            // objects with this name.  There is likely another one of type
            // protocol-display that will be found and removed below.
            if(type.equals("keyword-search")) {
                continue;
            }
            if(obj.getName().equals(keyword) && type.equals("protocol-display")) {
                keywords.remove(i);
                numDisplayKeywords--;
                break;
            }
            if(obj.getName().equals(keyword) && type.equals("studycard-display")) {
                keywords.remove(i);
                numSCKeywords--;
                break;
            }
            if(obj.getName().equals(keyword) && type.equals("data-display")) {
                keywords.remove(i);
                break;
            }
        }
    }

    // Replace a keyword with a new version which presumably has
    // something changed.
    public void replaceAKeyword(String keywordName, ProtocolKeyword newKeyword) {
        // Find the object for this keyword
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword obj = keywords.get(i);
            if(obj.getName().equals(keywordName)) {
                // Remove the current one
                keywords.remove(i);
                // Add the new one at the same position as the old was
                keywords.add(i, newKeyword);
                break;
            }
        }
    }

    // Go through the keyword objects and find one with this keyword
    // There are not expected to ever be more than 10 keywords, so
    // just loop through them and check.
    public ProtocolKeyword getKeywordByName(String key) {
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keywordObj = keywords.get(i);
            if(keywordObj.getName().equals(key)) {
                return keywordObj;
            }
        }
        // None was found with this key
        return null;
    }

    // Just get the ProtocolKeyword obj at this index and return it
    public ProtocolKeyword getKeywordByIndex(int index) {
        ProtocolKeyword keywordObj = keywords.get(index);
        return keywordObj;
    }

    // Create a list of keywords and return it
    public ArrayList<String> getKeywordList() {
        ArrayList<String> keywordList = new ArrayList<String>();
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keywordObj = keywords.get(i);
            keywordList.add(keywordObj.getName());
        }
        return keywordList;
    }

    // Create a list of keywords of type keyword-search and return it
    public ArrayList<String> getKeywordColumnList() {
        ArrayList<String> keywordList = new ArrayList<String>();
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keywordObj = keywords.get(i);
            if(keywordObj.type.startsWith("user") || keywordObj.type.startsWith("display"))
                keywordList.add(keywordObj.getName());
        }
        return keywordList;
    }

    // Return the keyword-search in the active position
    // This means count only the keyword-search items
    public ProtocolKeyword getActiveKeyword() {
        int pos=0;
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keywordObj = keywords.get(i);
            
            // Skip over non keyword-search keywords
            if(!keywordObj.getType().equals("keyword-search"))
                continue;
            
            if(pos == activePosition)
                // Found it
                return keywordObj;
            else
                pos++;
        }
        // If we arrive here and pos is still 0 then there must not be
        // any keyword-search keywords.  In that case, return null and let
        // the caller deal with it.
        if(pos == 0)
            return null;
        
        // We should not arrive here unless there is a problem
        // with the active position.  So, if we get here, reset
        // the active position to 0 and then call ourselves
        // recursively to get the keyword for the 0th position
        activePosition = 0;
        return getActiveKeyword();

    }
    
    
    public int size() {
        return keywords.size();
    }
    
    public String getPSortKeyword() {
        return pSortKeyword;
    }
    
    public void setPSortKeyword(String newSortKeyword) {
        pSortKeyword = newSortKeyword;
    }
    
    public String getPSortDirection() {
        return pSortDirection;
    }
    
    public void setPSortDirection(String newSortDirection) {
        pSortDirection = newSortDirection;
    }

    
    public String getDSortKeyword() {
        return dSortKeyword;
    }
    
    public void setDSortKeyword(String newSortKeyword) {
        dSortKeyword = newSortKeyword;
    }
    
    public String getDSortDirection() {
        return dSortDirection;
    }
    
    public void setDSortDirection(String newSortDirection) {
        dSortDirection = newSortDirection;
    }
    

    // Read the users persistence file if found, else read the config file
    // Return false if failure, true for success.
    // For backwards compatibality, if an old ProtocolBrowserKeywords is
    // encountered, it is removed and we return with false.  The caller
    // should try calling this twice.  The second time, we should find the
    // system ProtocolBrowserKeywords.
    static boolean firstCall = true;
    public boolean readPersistenceOrConfigFile() {
        keywords = new ArrayList<ProtocolKeyword>();
        ArrayList<ProtocolKeyword> keywdsSorted=new ArrayList<ProtocolKeyword>();
        String filepath=null;
        BufferedReader in=null;
        String line;
        StringTokenizer tok;

        filepath = FileUtil.openPath("LOCATOR/ProtocolBrowserKeywords");
        if(filepath == null) {
            Messages.postError("System ProtocolBrowserKeyword file not found");
            return false;
        }
                    
        try {
            UNFile file = new UNFile(filepath);
            if(file.exists()) {

                in = new BufferedReader(new FileReader(file));

                // The file should be one line per keyword.
                // Each line should be ";" separated items which are
                // "keyword"; "type"; "value"
                while ((line = in.readLine()) != null) {
                    if(line.startsWith("#") || line.length() == 0)
                        continue;

                    String keyword=null, type=null, value="All";
                    tok = new StringTokenizer(line, ":");
                    if(tok.hasMoreTokens()) 
                        keyword = tok.nextToken().trim();
                    if(tok.hasMoreTokens()) {
                        type = tok.nextToken().trim();
                        // Check type to see if this is an old file
                        if(type.equals("system") || type.equals("user-specified")) {
                            // This is an old format file.  If it is the user 
                            // file, remove it.
                            String userDir = System.getProperty("userdir");
                            if(filepath.startsWith(userDir)) {
                                file.delete();
                                in.close();
                                return false;
                            }
                        }
                            
                    }
                    if(tok.hasMoreTokens()) 
                        value = tok.nextToken().trim();

                    // Be sure there was a keyword and a type.
                    // If not, skip this line.  If okay, add a keyword to the list.
                    if(keyword != null && type != null) {
                        ProtocolKeyword keywdObj = new ProtocolKeyword(keyword, value, type);
                        // Add to keywords list
                        keywords.add(keywdObj);
                    }
                }
            }
        }
        catch (Exception ex) { 
            Messages.postError("Problem reading  " + filepath);
            Messages.writeStackTrace(ex);
        }
        
        // Try to close the BufferedReader
        try {
            if (in != null)
                in.close();
        }
        catch (Exception ex) { 
            // No error
        }
        
        // We have the list of keywords as read from the file
        // Be sure we have at least one keyword for protocol-display
        // and studycard-display and data-display.  If not, add filename.
        boolean protocolFilenameFound=false;
        boolean studycardFilenameFound=false;
        boolean dataFilenameFound=false;

        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.get(i);
            if(keyword.type.equals("protocol-display") && keyword.name.equals("filename"))
                protocolFilenameFound = true;
            if(keyword.type.equals("data-display") && keyword.name.equals("filename"))
                dataFilenameFound = true;
            if(keyword.type.equals("studycard-display") && keyword.name.equals("filename"))
                studycardFilenameFound = true; 
        }
        if(!protocolFilenameFound) {
            String keyword="filename", type="protocol-display", value="All";
            ProtocolKeyword keywdObj = new ProtocolKeyword(keyword, value, type);
            keywords.add(keywdObj);
        }
        if(!dataFilenameFound) {
            String keyword="filename", type="data-display", value="All";
            ProtocolKeyword keywdObj = new ProtocolKeyword(keyword, value, type);
            keywords.add(keywdObj);
        }
        if(!studycardFilenameFound) {
            String keyword="filename", type="studycard-display", value="All";
            ProtocolKeyword keywdObj = new ProtocolKeyword(keyword, value, type);
            keywords.add(keywdObj);
        }
        
        // Now we want to sort them so that the type="system"
        // keywords come first, then fixed-value and then the type="user"
        // and protocol-display. and then data-display
        // We need to keep the order of each type the same
        // order they were already in.
        // Get the system items first
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.get(i);
            if(keyword.type.equals("system-search"))
                keywdsSorted.add(keyword);  
        }
        // Now fixed-value
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.get(i);
            if(keyword.type.equals("fixed-value"))
                keywdsSorted.add(keyword);  
        }
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.get(i);
            if(keyword.type.equals("keyword-search"))
                keywdsSorted.add(keyword);  
        }
        // Now get the protocol-display items
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.get(i);
            if(keyword.type.equals("protocol-display"))
                keywdsSorted.add(keyword);  
        }
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.get(i);
            if(keyword.type.equals("data-display"))
                keywdsSorted.add(keyword);  
        }
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.get(i);
            if(keyword.type.equals("studycard-display"))
                keywdsSorted.add(keyword);  
        }
        
        
        // Don't allow a system keyword to also be a search keyword
        // Get list of system keywords
        ArrayList <String> systemKeywords = new ArrayList <String>();
        for(int i=0; i < keywdsSorted.size(); i++) {
            ProtocolKeyword keyword = keywdsSorted.get(i);
            if(keyword.type.equals("system-search"))
                systemKeywords.add(keyword.getName());
        }
        // Check all keyword-search keywords to see if any are system keywords
        // Start looking after system keywords.
        for(int i=systemKeywords.size(); i < keywdsSorted.size(); i++) {
            ProtocolKeyword keyword = keywdsSorted.get(i);
            String searchKeyName = keyword.getName();
            for(int k=0; k < systemKeywords.size(); k++) {
                String systemKeyName = systemKeywords.get(k);
                if(systemKeyName.equals(searchKeyName) && 
                        keyword.getType().equals("keyword-search")) {
                    // system-search should have been sorted to be first in
                    // the list.  Remove the system-search.  If they have defined
                    // this keyword also as a keyword-search, then let the
                    // keyword-search stand and remove the system-search.
                    keywdsSorted.remove(k);
                    // Remove this system keyword from the systemKeywords list
                    // for subsequent tests
                    systemKeywords.remove(k);
                    // We don't want to output multiple messages, so only do
                    // this the first time we are called
                    if(firstCall) {
                        Messages.postWarning("The keyword \"" + searchKeyName 
                                + "\" has been defined twice.  The system-search "
                                + "specification is being ignored.");
                    }
                    // Account for numbering change due to removed item
                    i--;  
                    break;
                }
            }
        }
        
        keywords = keywdsSorted;
        firstCall = false;
        return true;
   }

    // Write persistence file with current keyword information
    // Write info contained in "keywords" list.
    public void writePersistenceFile() {
        String filepath;
        FileWriter fw;
        PrintWriter os=null;

        filepath = FileUtil.savePath("USER/LOCATOR/ProtocolBrowserKeywords");
        try {
            UNFile file = new UNFile(filepath);
            fw = new FileWriter(file);
            os = new PrintWriter(fw);
            // Write a line to show what this file is.  Use this for
            // confirmation when reading.
            os.println("# Protocol Browser Keywords");

            for(int i=0; i < keywords.size(); i++) {
                // Get each keyword Object and write its contents to the file
                ProtocolKeyword kwObj = keywords.get(i);
                os.println(kwObj.getName() + ": " + kwObj.getType() + ": " 
                           + kwObj.getValue());
            }  
            os.close();
        }
        catch (Exception er) {
            Messages.postError("Problem creating  " + filepath);
            Messages.writeStackTrace(er);
            if (os != null)
                os.close();
        }
    }
    
    // Go through the keywords list.  Find the ones with type = system-search
    // and fill their value from the system values.
    // Allowed keywords and their global system values are:
    //   gcoil -> sysgcoil
    //   rfcoil -> RFCOIL
    //   field -> h1freq/42.58 
    //   sfrq -> h1freq
    private void fillSystemKeywords() {
        String stValue;        

        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keywordObj = keywords.get(i);
            String key = keywordObj.getName();
            String type = keywordObj.getType();
            if(type.equals("system-search") && key.equals("gcoil")) {
                // for gcoil, get the system value of sysgcoil
                stValue = gcoilValue;
                
                // Set the value into this keyword object
                keywordObj.setValue(stValue);
            }
            else if(type.equals("system-search") && key.equals("rfcoil")) {
                stValue = rfcoilValue;
                // Set the value into this keyword object
                keywordObj.setValue(stValue);
            }
            else if(type.equals("system-search") && key.equals("field")) {    
                Double field = FillDBManager.convertFreqToTesla(h1freqValue);
                // Set the value into this keyword object
                keywordObj.setValue(field.toString());
            }
        }
    }
    
    // We need to get some values from vnmrj global parameters.  This has to
    // be done asynchronously by communications with vnmrbg.  When the values
    // come back, SystemParamSmsInfoObj.setValue() will be called for each param.
    public void getSystemParams() {
        
        // Get system vnmr parameter values
        SystemParamSmsInfoObj rfcoilIF = new SystemParamSmsInfoObj("$VALUE=RFCOIL", RFCOIL);
        SystemParamSmsInfoObj gcoilIF = new SystemParamSmsInfoObj("$VALUE=sysgcoil", GCOIL);
        SystemParamSmsInfoObj h1freqIF  = new SystemParamSmsInfoObj("$VALUE=h1freq", H1FREQ);

        // Fire off the request of an update of these parameters
        rfcoilIF.updateValue();
        gcoilIF.updateValue();
        h1freqIF.updateValue();
    }
    
    public int getActivePosition() {
        return activePosition;
    }
    
    
    // allow setting the active position with a string input
    // If there was no change, return false.
    public boolean setActivePosition(int newPosition) {
        // If the new position is the same as the old, then return false
        // so that the caller will know not to bother updating things
        if(activePosition == newPosition)
            return false;
        else {
            activePosition = newPosition;
            return true;
        } 

    }

    public int getNumSearchKeywords() {
        return numSearchKeywords;
    }

    public int getNumDisplayKeywords() {
        return numDisplayKeywords;
    }

    // The number of columns of information will be the sum of the number
    // of "user" keywords and "protocol-display" keywords.
    public int getNumColumns() {
        return numDisplayKeywords;
    }

    public int getNumSCColumns() {
        return numSCKeywords;
    }  

    // allow setting the active position with a string input
    // If there was no change, return false.
    public boolean setActivePosition(String strPosition) {
        int newPosition;
        // Convert string to int
        try {
            Integer inter = new Integer(strPosition);
            newPosition = inter.intValue();
        }
        catch(Exception ex) {
            Messages.postWarning("Problem converting active position string to int");
            newPosition = 0;
        }
        return setActivePosition(newPosition);
    }

    // Is the keyword-search position given in the arg the last position in this
    // keywords list?  Remember, we only count keyword-search items
    public boolean isLastPosition(int position) {
        // Start counting position at -1 since 0 is the first actual position
        int pos=-1;
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keywordObj = keywords.get(i);
            
            // Count only keyword-search
            if(!keywordObj.getType().equals("keyword-search"))
                continue;
            else
                // Count the keyword-search keywords
                pos++;
        }
        // pos should be the last position available
        if(position >= pos)
            return true;
        else
            return false;
    }
    
    // Does the keywords list already contain a keyword with the name, "name"
    // includeSystem indicated whether or not to include type=system keywords
    // in determination.  forceSystemKeyword=true, means return false for
    // system keywords even if they are in the keyword list.
    public boolean contains(String name, boolean forceSystemKeyword) {
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keywordObj = keywords.get(i);
            if(keywordObj.name.equals(name)) {
                if(forceSystemKeyword && keywordObj.type.equals("system"))
                    return false;
                else
                    return true;
            }
        }
        
        // Not found
        return false;
    }
    
    // Return an ArrayList of keyword objects which represent the header columns
    // of the Protocol Result table.  
    public ArrayList<ProtocolKeyword> getProtocolHeaderKeywords() {
        ArrayList<ProtocolKeyword> list = new ArrayList<ProtocolKeyword>();
        
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keywordObj = keywords.get(i);
            String type = keywordObj.getType();
            if(type.startsWith("protocol-display")) {
                list.add(keywordObj);
            }
        }
        
        return list;
    }
    
    // Return an ArrayList of keyword objects which represent the header columns
    // of the Protocol Result table for the Study Card tab.  
    public ArrayList<ProtocolKeyword> getSCHeaderKeywords() {
        ArrayList<ProtocolKeyword> list = new ArrayList<ProtocolKeyword>();
        
        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keywordObj = keywords.get(i);
            String type = keywordObj.getType();
            if(type.startsWith("studycard-display")) {
                list.add(keywordObj);
            }
        }
        
        return list;
    }    
         
    // We need to get some values from vnmrj global parameters.  This has to
    // be done asynchronously by communications with vnmrbg.  When the values
    // come back, setValue() will be called for each param.
    private class SystemParamSmsInfoObj extends SmsInfoObj
    {
        private int  type;
        
        public SystemParamSmsInfoObj(String str, int typ) {
            super(str, 0);
            this.type = typ;
        }
      
        public void setValue(ParamIF pf)  {
            // the parameter value will be  p.value
            if (pf != null && pf.value != null) {
                String value = pf.value;
                
                // Set the values to be used to fill in the keywords list
                if (type == RFCOIL) {
                    rfcoilValue = value;
                }
                else if (type == GCOIL) {
                    gcoilValue = value;
                }
                else if (type == H1FREQ) {
                    h1freqValue = value;
                }
            } 
            // If all values are now set, finish doing the work of
            // creating this class object and then fire off finishPanel
            // so ProtocolBrowser can finish creating its panel objects
            // based on this information.
            if(h1freqValue != null && gcoilValue != null && rfcoilValue != null) {
                createAndFillKeywords();
                
                // Now that we have the sys values, fire off the update of
                // the actual parentPanel (our parent).
                parentPanel.finishPanel();
            }
        }   
    } // End SystemParamSmsInfoObj
    
}  // End ProtocolKeywords
