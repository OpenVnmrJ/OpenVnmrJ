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

import java.awt.*;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.StreamTokenizer;
import java.io.StringReader;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.*;

import vnmr.bo.LockStatus;
import vnmr.bo.ProtocolType;
import vnmr.bo.SearchResults;
import vnmr.ui.*;
import vnmr.util.ExitStatus;
import vnmr.util.FileUtil;
import vnmr.util.Messages;
import vnmr.util.SimpleVLayout;
import vnmr.util.StreamTokenizerQuotedNewlines;
import vnmr.util.UNFile;
import vnmr.util.Util;


/********************************************************** <pre>
 * Summary: Protocol Browser Tabbed Pane to hold the two panels used
 * for the Protocol Browser
 *
 </pre> **********************************************************/

// Adding a JTabbedPane directly to ExpViewArea ends up leaving parts of
// the viewport titles showing, because the TabbedPane is not a
// rectangle.  We want to block off ALL of the VP stuff, so create
// a nice rectangle JPanel and then add the TabbedPane to that.

public class ProtocolBrowser extends JPanel {
    public static final String T_REAL = "1";
    public static final String T_STRING = "2";
    public static final String DB_ATTR_FLOAT8 = "float8";
    public static final String DB_ATTR_DATE = "timestamp";
    public static final String DB_ATTR_TEXT = "text";
    public static final int    MAX_ARRAY_STR_LEN = 40;
    
    // I want this to be flexible for future expansion to Liquids
    // and other as yet unknown uses.  For now, hard code the object
    // types here.
    // Type to use for browsing keywords
    String browserType = Shuf.DB_PROTOCOL;
    // Type to use when showing data which used browserType
    String dataType = Shuf.DB_IMAGE_DIR;

    public ProtocolBrowserPanel protocolBrowserPanel = null;
    private String studyCardTabFullpath = "";
    static public ProtocolBrowserResults dataScrollPane = null;
    static public JPanel topLevelResultPanel = null;
    static public JPanel topLevelDataPanel = null;
    static public JPanel topLevelStudyCardPanel = null;
    static public ProtocolBrowserResults resultScrollPane = null;
    static public ProtocolBrowserResults studyCardScrollPane = null;
    static public JPanel resultParamsPanel = null;
    public JPanel dataParamsPanel = null;
    static public JPanel studyCardParamsPanel = null;

    static JTabbedPane tabbedPane = null;
    static private ProtocolKeywords keywords = null;
    static private String pFilename = "";
    


    public ProtocolBrowser () {

        // Create and Fill the ProtocolKeywords object
        // Pass "this" so it can update the display when the system
        // parameters are ready.
        keywords = new ProtocolKeywords(this);

        // Show the pane and set its size to the current size of
        // its parent (ExpViewArea)
        initPanel();
    }

    // Set pane size to the current size of the parent (ExpViewArea)
    public void updateSize() {
        // Get the size of the parent and set this pane to it
//        if (!isShowing())
//            return;

        ExpViewArea expViewArea = Util.getViewArea();
        Dimension dim = expViewArea.getSize();
        setSize(dim);
        if(tabbedPane != null) {
            // Kludge Alert:
            // The height of the resultScrollPane automatically adjusts itself
            // when it is added directly to the tabPane.  HOWEVER, when I add
            // the scrollPane to a JPanel (topLevelResultPanel) so that I can also 
            // add the paramPanel, it no longer sizes itself to fit within the tabPane.
            // So, I am manually setting the height.  I am subtracting enough for
            // the Tabs and for the paramPanel.  I have come up with the number
            // empirically and thus it will probably be wrong sometime in the
            // future when something changes.  The width is self correcting.
            tabbedPane.setPreferredSize(dim);
            int width = (int)dim.getWidth();
            int height = (int)dim.getHeight();
            height = height - 56;
            Dimension newdim = new Dimension(width, height);
            if(resultScrollPane != null) 
                resultScrollPane.setPreferredSize(newdim);
            if(dataScrollPane != null)
                dataScrollPane.setPreferredSize(newdim);
            if(studyCardScrollPane != null)
                studyCardScrollPane.setPreferredSize(newdim);

        }
        if(protocolBrowserPanel != null) {
            protocolBrowserPanel.setNewSize(dim);
        }
    }

    public void initPanel() {
        if(tabbedPane == null) {
            // Create a JTabbedPane for implementing Tabs
            tabbedPane = new JTabbedPane();
            tabbedPane.setVisible(true);
            // Add tabbedPane to JPanel
            add(tabbedPane);

            // Create the actual panel with keywords and add it to the tabbedPane
            createProtocolBrowserPanel();
            
            // Create the actual panel with results and add it to the tabbedPane
            createResultPanel();
        }
        // Set the size of the panels
        updateSize();

        showPanel();
    }
    
    public void showPanel() {
        setVisible(true);
    }

    public void unshow() {
        // setVisible(false);
    }
    
    // Make the actual panel with all of the content 
    // Add it to the tabbedPane
    protected void createProtocolBrowserPanel() {
        // Make a new one if needed
//        if(protocolBrowserPanel == null) {
            protocolBrowserPanel = new ProtocolBrowserPanel(this);
            protocolBrowserPanel.setVisible(true);
            
            // add to the Tab pane.  Note, the panel is not complete yet.
            // It gets completed in finishPanel() below.
            tabbedPane.add(protocolBrowserPanel);
            tabbedPane.setVisible(true);
//        }
//        else {
//            protocolBrowserPanel.setVisible(true);
//        }
    
    }
    
    // This should be called by Protocolkeywords.updatePanel() when it has
    // all of the system parameters filled.  Before the callback in ProtocolKeywords
    // happens, we don't have enough information to create the panel contents.
    // It seems a little weird, but we will be finishing the panel upon a call
    // from one of our subordinates in an async manner.  That is, of course,
    // because we only have async communications with vnmrbg.
    public void finishPanel() {
        protocolBrowserPanel.fillPanel(keywords); 
        showPanel();
        // Without the repaint(), the items added in 
        // protocolBrowserPanel.finishPanel don't show up until the panel
        // is clicked or resized.  
        repaint();
    }

    static public ProtocolKeywords getKeywords() {
        return keywords;
    }
    
    static public void setKeywords(ProtocolKeywords newKeywords) {
        keywords = newKeywords;
    }

    // Replace a keyword within keywords with a new version which presumably has
    // something changed.
    public void replaceAKeyword(String keywordName, ProtocolKeyword newKeyword) {
        keywords.replaceAKeyword(keywordName, newKeyword);
    }

    static public String getProtocolFilename() {
        return pFilename;
    }
    
    public String getStudyCardTabFullpath() {
        return studyCardTabFullpath;
    }
       
    static public void updateResultParamsPanel() {
        topLevelResultPanel.remove(resultParamsPanel);
        resultParamsPanel = ProtocolBrowserPanel.createParamsPanel(keywords);
        topLevelResultPanel.add(resultParamsPanel);
    }
    
    public void updateDataParamsPanel() {
        topLevelDataPanel.remove(dataParamsPanel);
        dataParamsPanel = ProtocolBrowserPanel.createParamsPanel(keywords);
        topLevelDataPanel.add(dataParamsPanel);
    }
    
    static public void updateStudyCardParamsPanel(String fullpath) {
        topLevelStudyCardPanel.remove(studyCardParamsPanel);
        studyCardParamsPanel = ProtocolBrowserPanel.createParamsPanel(keywords, fullpath);
        topLevelStudyCardPanel.add(studyCardParamsPanel);
    }

    
    public void updateResultPanel() {
        // Remove the old pane from the tabbed pane
        tabbedPane.remove(topLevelResultPanel);

        // Set to null so that createResultPanel() will create a new one
        topLevelResultPanel = null;

        // Make a new one using the current keywords values
        createResultPanel();
        
        // Update the Table
        doDBSearch(keywords, "protocol");

        // Show this Tab
        enableResultPanel();
    }
    
    // This gets called via ExpPanel after the menu selection to search
    // and display the Data Tab.
    public void updateDataPanel(String filename, String dType, String scName) {
        dataType = dType;
        pFilename = filename;
        // Remove the old pane from the tabbed pane
        tabbedPane.remove(topLevelDataPanel);

        // Set to null so that createDataPanel() will create a new one
        dataScrollPane = null;
        // Make a new one using the current keywords values
        creatDataPanel(scName);
        
        // Update the Table
        doDBSearch(keywords, dataType, pFilename);

        // Show this Tab
        enableDataPanel();

    }
    
    // This gets called via ExpPanel 
    // They want to be able to go down into a study card (which is a protocol)
    // and show the list of protocols within that study card.  While the search
    // and results are done via DB, this does not involve the DB.  Instead, it
    // just opens the study card and gets the list of nodes (protocols) and
    // puts them into a Study Card Tab.
    public void updateStudyCardPanel(String fullpath) {
        // Remove the old pane from the tabbed pane
        tabbedPane.remove(topLevelStudyCardPanel);

        // Set to null so that createStudyCardPanel() will create a new one
        studyCardScrollPane = null;
        
        // Make a new one using fullpath
        createStudyCardPanel(fullpath);

        // Show this Tab
        enableSCPanel();
    }
 
    // Call to update Data Tab after changing number of columns.  This assumes
    // it has already been displayed and the values of dataType and pFilename
    // were filled at the time of the previous display.  Use those values.
    public void updateDataPanel() {
        updateDataPanel(pFilename, dataType, null);
    }


    private void createResultPanel() {
        resultScrollPane = new ProtocolBrowserResults(keywords, browserType);
        topLevelResultPanel = new JPanel();
        SimpleVLayout layout = new SimpleVLayout();
        topLevelResultPanel.setLayout(layout);

        resultParamsPanel = ProtocolBrowserPanel.createParamsPanel(keywords);

        topLevelResultPanel.add(resultScrollPane);
        topLevelResultPanel.add(resultParamsPanel);

        topLevelResultPanel.setName("Protocols");
        // add to the Tab pane.  Note, the panel is not complete yet.
        // It gets completed in finishPanel() below.
        // Add to position "1" (meaning the second tab)
        tabbedPane.add(topLevelResultPanel, 1);
        tabbedPane.setVisible(true);
        topLevelResultPanel.setVisible(true);

    }

    private void creatDataPanel(String scName) {
        dataScrollPane = new ProtocolBrowserResults(keywords, dataType);

        topLevelDataPanel = new JPanel();
        SimpleVLayout layout = new SimpleVLayout();
        topLevelDataPanel.setLayout(layout);

        dataParamsPanel = ProtocolBrowserPanel.createParamsPanel(keywords, scName);

        topLevelDataPanel.add(dataScrollPane);
        topLevelDataPanel.add(dataParamsPanel);

        topLevelDataPanel.setName("Data");
        // add to the Tab pane.  Note, the panel is not complete yet.
        // It gets completed in finishPanel() below.
        // Add to position "2" (meaning the third tab)
        tabbedPane.add(topLevelDataPanel, 2);
        tabbedPane.setVisible(true);
        topLevelDataPanel.setVisible(true);

    }
    
    private void createStudyCardPanel(String fullpath) {
        studyCardScrollPane = new ProtocolBrowserResults(keywords, Shuf.DB_PROTOCOL, fullpath);

        topLevelStudyCardPanel = new JPanel();
        SimpleVLayout layout = new SimpleVLayout();
        topLevelStudyCardPanel.setLayout(layout);

        // Even though the study card panel does not actually use the keywords
        // for searching, it will use keywords contents for the param info to display.
        studyCardParamsPanel = ProtocolBrowserPanel.createParamsPanel(keywords, fullpath);

        topLevelStudyCardPanel.add(studyCardScrollPane);
        topLevelStudyCardPanel.add(studyCardParamsPanel);

        topLevelStudyCardPanel.setName("Study Card");
        
        // This seems to work properly without adding a position
        // This way, it ends up being the last of the tabs
        tabbedPane.add(topLevelStudyCardPanel);

        updateStudyCardResults(fullpath, keywords);

        tabbedPane.setVisible(true);
        topLevelStudyCardPanel.setVisible(true);
        
        // Save the fullpath we are showing in this Tab
        studyCardTabFullpath = fullpath;

    }
    public void enableBrowserPanel() {
        tabbedPane.setSelectedComponent(protocolBrowserPanel);
    }
    
    static public void enableResultPanel() {
        tabbedPane.setSelectedComponent(topLevelResultPanel);
//        tabbedPane.setSelectedComponent(resultScrollPane);
    }
    
    static public void enableDataPanel() {
        tabbedPane.setSelectedComponent(topLevelDataPanel);
    }
    
    static public void enableSCPanel() {
        tabbedPane.setSelectedComponent(topLevelStudyCardPanel);
    }
    
    // DB search for Data where we find Data with protocol matching 'filename' arg
    static public void doDBSearch(ProtocolKeywords keywords, String objType, String filename) {
        SearchInfo info;
        SearchResults[] results=null;

        // First fill the SearchInfo object to be passed on
        ArrayList<String> names = new ArrayList<String>();
        ArrayList<String> values = new ArrayList<String>();
        ArrayList<String> modes = new ArrayList<String>();
        
        ShufDBManager dbManager = ShufDBManager.getdbManager();
        
        // We need to know the number of attrToReturn items BEFORE we can   
        // create the SearchInfo object.  So fill a temp variable attrToReturn
        
        // Fill the attrToReturn array with the keyword-search and protocol-display keywords
        // that need to be returned from the DB search.
        // For Data, some keywords may not be in the DB. Check to see if the
        // keyword is in the DB and if not, don't ask for it.
        ArrayList<String> attrToReturn = new ArrayList<String>();

        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.getKeywordByIndex(i);
            // We want to catch items with objType=protocol and type=keyword-search
            // or protocol-display OR objType!= data and type=data
            if(objType.equals(Shuf.DB_PROTOCOL)) {
                if(keyword.type.equals("keyword-search") || 
                   keyword.type.equals("protocol-display")) {
                    String keywordName = keyword.getName();

                    // Is this a valid attribute in the DB?
                    boolean foundit = dbManager.isAttributeInDB(objType, keywordName);
                    if(foundit)
                        // Valid, add it
                        attrToReturn.add(keywordName);
                }
            }
            else {
                // Must be for Data Tab
                if(keyword.type.equals("data-display")) {
                    String keywordName = keyword.getName();
                    // Is this a valid attribute in the DB?
                    dbManager = ShufDBManager.getdbManager();
                    boolean foundit = dbManager.isAttributeInDB(objType, keywordName);
                    if(foundit)
                        // Valid, add it
                        attrToReturn.add(keywordName);
                }
            }

                
        }
        // Get number columns to be returned
        int numReturned = attrToReturn.size();

        // Now create the SearchInfo object of the correct size
        info = new SearchInfo(numReturned);
        
        // Fill the info.attrToReturn 
        for(int i=0; i < numReturned; i++) {
            info.attrToReturn[i] = attrToReturn.get(i);
        }

        info.objectType = objType;

        names.add("protocol");
        values.add(filename);
        modes.add("nonexclusive");

        info.numAttributes = names.size();
        info.attributeNames = names;
        info.attributeValues = values;
        info.attributeMode = modes;


        // Don't use the accessibleUsers feature, just allow all users
        info.accessibleUsers.add("all");
        
        // Sort info for protocol
        if(objType.equals(Shuf.DB_PROTOCOL)) {
            info.sortAttr = keywords.getPSortKeyword();
            info.sortDirection = keywords.getPSortDirection();
        }
        // Sort info for data
        else {
            info.sortAttr = keywords.getDSortKeyword();
            info.sortDirection = keywords.getDSortDirection();           
        }
        
        // If we were told to search the objType "image_dir", then we also want
        // to include results from the "vnmr_data" table.
        if(objType.equals(Shuf.DB_IMAGE_DIR))
           info.secondObjType = Shuf.DB_VNMR_DATA;

        String matchFlag = "match";
        
        SessionShare sshare = ResultTable.getSshare();
        Hashtable searchCache = sshare.searchCache();
        dbManager = ShufDBManager.getdbManager();

        // Do the DB search
        results = dbManager.doSearch(info, searchCache, matchFlag);

        // Update the panel with the results from the DB
        dataScrollPane.updateProtocolBrowser(results, keywords);
        
        // Bring the Result panel to the top
        enableDataPanel();

        
    }

    
    // Create and fill a SearchInfo object so that we can use standard
    // locator code to do the search based on that object
    static public void doDBSearch(ProtocolKeywords keywords, String objType) {
        SearchInfo info;
        SearchResults[] results=null;

        ArrayList<String> names = new ArrayList<String>();
        ArrayList<String> values = new ArrayList<String>();
        ArrayList<String> modes = new ArrayList<String>();

        // Get number columns to be returned
        int numReturned = keywords.getNumColumns();
        
        info = new SearchInfo(numReturned);
        info.objectType = objType;
        // Loop through the keywords getting attribute names and values
        // That we will search on.
        // We need two ArrayList's, one for names and one for values.
        // These are the keywords to search against and are used for
        // Only for Protocol
        if(objType.equals(Shuf.DB_PROTOCOL)) {
            for(int i=0; i < keywords.size(); i++) {
                ProtocolKeyword keyword = keywords.getKeywordByIndex(i);
                String name = keyword.getName();
                String value = keyword.getValue();
                String type = keyword.getType();
                if(name != null && name.length() > 0 && value != null 
                   && value.length() > 0 && !value.equals("All")
                   && (type.equals("keyword-search") || type.equals("system-search")
                       || type.equals("fixed-value")) &&  !keyword.skip) {
                    
                    names.add(name);
                    values.add(value);
                    modes.add("nonexclusive");
                }
            }
        }
   
        info.numAttributes = names.size();
        info.attributeNames = names;
        info.attributeValues = values;
        info.attributeMode = modes;
        
        // Fill the attrToReturn array with the protocol-display keywords
        // that need to be returned from the DB search
        for(int i=0, j=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.getKeywordByIndex(i);
            if(!keyword.type.startsWith("protocol-display"))
                continue;
            String keywordName = keyword.getName();
            info.attrToReturn[j++] = keywordName;
        }

        if(objType.equals(Shuf.DB_PROTOCOL)) {
            info.sortAttr = keywords.getPSortKeyword();
            info.sortDirection = keywords.getPSortDirection();
        }
        // Sort info for data
        else {
            info.sortAttr = keywords.getDSortKeyword();
            info.sortDirection = keywords.getDSortDirection();           
        }

        String matchFlag = "match";
        
        SessionShare sshare = ResultTable.getSshare();
        Hashtable searchCache = sshare.searchCache();
        ShufDBManager dbManager = ShufDBManager.getdbManager();

        // Do the DB search
        results = dbManager.doSearch(info, searchCache, matchFlag);

        // Update the panel with the results from the DB
        resultScrollPane.updateProtocolBrowser(results, keywords);
        updateResultParamsPanel();
        
        // Bring the Result panel to the top
        enableResultPanel();

    }
    
    // Update the Study Card panel. 
    // Accumulate the results for these nodes into an array and
    // have them put into the table.
    static public void updateStudyCardResults(String fullpath, ProtocolKeywords keywords) {
        SearchResults[] compositeResults=null;
        ArrayList<String> nodeFullpaths = new ArrayList<String>();     
        ArrayList<ProtocolKeyword> headerKeywords;
        ArrayList<KeywordNvalue> keyNvalList;
        int numReturned;
        
        numReturned = keywords.getNumSCColumns();
            
        InetAddress inetAddress;
        try {
            inetAddress = InetAddress.getLocalHost();
        }
        catch (UnknownHostException e) {
            Messages.postError("Problem getting local host name");
            Messages.writeStackTrace(e, "Error caught in updateStudyCardResults");
            return;
        }
        String localHost = inetAddress.getHostName();
        
        // We only want to find one protocol at a time, namely the ones
        // that are the nodes for this study card
        nodeFullpaths = getAllSCNodes(fullpath);
        compositeResults = new SearchResults[nodeFullpaths.size()];
        int match = 1;

        // Go through the list of protocols which are nodes for the study card
        for(int cnt=0, i=0; i < nodeFullpaths.size(); i++) {
            SearchResults singleResult=null;
            Object newrow[] = new Object[numReturned + 6];
            
            // Get the fullpath for this node
            String nodepath = nodeFullpaths.get(i);
            String host_fullpath = localHost + ":" + nodepath;
            int startIndex = nodepath.lastIndexOf(File.separator);
            int endIndex = nodepath.lastIndexOf(".");
            // Make the filename
            String filename = nodepath.substring(startIndex +1, endIndex);
            
            newrow[numReturned +1] = filename;
            newrow[numReturned +2] = nodepath;
            newrow[numReturned +3] = Shuf.DB_PROTOCOL;
            newrow[numReturned +4] = localHost;
            newrow[numReturned +5] = "";
            newrow[0] = LockStatus.unlocked;
            
            // We need to get the header keyword names and then parse this
            // protocol and get values for which are defined header names.
            // These values should go into newrow[1] to newrow[numReturned -1]
            
            // ToDo  Include procpar parameters here
            
            
            // This gives us all of the keyword/value pairs from the .xml file
            keyNvalList = parseProtocol(nodepath) ;
            
            // Get the keywords defined in the procpar
            ArrayList<KeywordNvalue> procparKeyNvalList = parseProcpar(nodepath);
            
            // The size of keyNvalList will change as we add more below, but we only need to
            // loop through the xml values in the list at this time.
            int xmlsize = keyNvalList.size();

            if(procparKeyNvalList != null) {
                // Combine the two lists of parameters but remove duplicates.
                // Keep the .xml value and ignore the dup procpar value
                for(int j=0; j < procparKeyNvalList.size(); j++) {
                    boolean dup = false;
                    KeywordNvalue procPair = procparKeyNvalList.get(j);
                    // Go through the xml params and if not a dup, then add it
                    // If it is a dup, but the xml param does not have a value,
                    // then go ahead and take the procpar value.
                    for(int k=0; k < xmlsize; k++) {
                        KeywordNvalue xmlPair = keyNvalList.get(k);
                        if(xmlPair.keyword.equals(procPair.keyword) 
                                && xmlPair.value.length() > 0) {
                            // Skip this procpar param, it is a dup
                            dup = true;
                            break;
                        }
                    }
                    // If not a dup, add it
                    if(!dup)
                        keyNvalList.add(procPair);
                }
            }
            
            // Now get the list of header keywords.
            headerKeywords = keywords.getSCHeaderKeywords();
            
            // Hopefully, numReturned and headerKeywords are the same length
            // since they should both refer to the header keywords
            if(headerKeywords.size() != numReturned) {
                Messages.postError("Header size discrepancy in ProtocolBrowser.  Aborting");
                return;
            }
            
            // Okay, headerKeywords should have the header keywords in the same
            // order as was expected if this were all being done via DB search.
            // So, fill newrow in order where newrow[0] has already been
            // filled with lock status, so the newrow index will be +1
            // Get the header name for each position from headerKeywords
            // and then see if there is a value for that header name in keyNvalList
            // which came from the protocol itself.
            for(int k=0; k < numReturned; k++) {
                ProtocolKeyword kwd = headerKeywords.get(k);
                String headerName = kwd.getName();
                String value = "";
                
                // Fill in some keywords we know about
                if(headerName.equals("filename"))
                    value = filename;
                else if(headerName.equals("fullpath"))
                    value = nodepath;
                else if(headerName.equals("host_fullpath"))
                    value = host_fullpath;
                else {
                    // Get the value for this header name from keyNVal if it exists.
                    // If it does not exist, use an empty string.
                    // Go through keyNvalList and see if the name exists
                    for(int j=0; j < keyNvalList.size(); j++) {
                        KeywordNvalue keyNval = keyNvalList.get(j);
                        String key = keyNval.keyword;
                        if(key.equals(headerName))
                            value = keyNval.value;
                    }
                }
                
                
                newrow[k+1] = value;
            }

            
            // Fill singleResult with info for this node
            ProtocolType id = new ProtocolType(host_fullpath);
            
            singleResult = new SearchResults(match, newrow, id, localHost, Shuf.DB_PROTOCOL);
            

           
            compositeResults[cnt++] = singleResult;

        }

        // Fill the Panel with the assembled results
        studyCardScrollPane.updateProtocolBrowser(compositeResults, keywords);
        
        updateStudyCardParamsPanel(fullpath);
        
        // Bring the Result panel to the top
        enableSCPanel();


    }
    
    // Open the Study Card at fullpath and get the list of protocol nodes
    // it contains.  Get the fullpath of each and return the list.
    static private ArrayList<String> getAllSCNodes(String fullpath) {
        ArrayList<String> nodes = new ArrayList<String>();
        ArrayList<String> names = new ArrayList<String>();

        BufferedReader  in;
        String          inLine=null;
        String          string=null;
        int             index;
        String          attr=null;
        String          value=null;
        StringReader    sr;
        StreamTokenizerQuotedNewlines tok;
        UNFile          file;
        String          node;

        if(fullpath == null)
            return nodes;
        
        file = new UNFile(fullpath);

        // Open the file.
        try {
            in = new BufferedReader(FileUtil.getUTFReader(file));
        } catch(Exception e) {
            Messages.postError("Problem opening " + fullpath);
            Messages.writeStackTrace(e, "Error caught in getAllSCNodes");
            // Return an empty list
            return new ArrayList<String>();
        }

        // Get the line/s to parse from the first occurrence of a '<'
        // followed by an alpha character and continuing to the next '>'.
        try {
            // Read one line at a time.
            while ((inLine = in.readLine()) != null) {
                inLine = inLine.trim();
                if (string == null && inLine.length() > 1 &&
                        inLine.startsWith("<")) {
                    // Is next char a letter?
                    if(Character.isLetter(inLine.charAt(1))) {
                        // We are at the start of our string, save it.
                        string = new String(inLine);

                        // If this line contains the closing '>', then
                        // stop reading now.
                        if ((index = inLine.indexOf('>')) != -1) {
                            // Only take the string up to the '>'
                            string = inLine.substring(0, index);
                            break;
                        }
                    }
                }
                // Concatenate to string until '>' is found.
                else if (string != null && inLine.indexOf('>') == -1) {
                    string = string.concat(" " + inLine);
                }
                // Include the line up to the '>'
                else if (string != null && (index = inLine.indexOf('>')) != -1) {
                    // Concatenate up to the '>' and include a space.
                    string = string.concat(" " + inLine.substring(0, index));
                    break;
                }

            }
            in.close();

            if(string == null)
                throw (new Exception("Badly formated xml file"));
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem reading " + fullpath);
                Messages.postError("getAllSCNodes failed on line: '" +
                        inLine + "'");
                Messages.writeStackTrace(e);
            }

            try {
                in.close();
            }
            catch(Exception  eio) {}
            return new ArrayList<String>();
        }
        // We should now have something like
        //    <template panel_type="acquisition" element_type="panels"
        // Tokenize the string.

        // Use StreamTokenizer because we can keep the quoted string
        // as a single token.  StringTokenizer does not allow that.
        // I had to create StreamTokenizerQuotedNewlines to allow quoted
        // string to contain newLines.  The standard one ends the quote
        // at the end of a line.  Sun had a bug reported about this
        // (4239144) and refused to add an option to allow multiple lines
        // in a quoted string.  GRS
        sr = new  StringReader(string);
        tok = new StreamTokenizerQuotedNewlines(sr);
        // Define the quote character
        tok.quoteChar('\"');
        // Define additional char to be treated as normal letter.
        tok.wordChars('_', '_');
        try {
            // Dump the first two tokens.  They will be '<' and the
            // word following '<'.
            tok.nextToken();
            tok.nextToken();
            // Now we should have a sequence of TT_WORD, '=', quoted string
            // for each attribute being defined.
            while(tok.nextToken() != StreamTokenizerQuotedNewlines.TT_EOF) {
                if(tok.ttype != StreamTokenizerQuotedNewlines.TT_WORD) {
                    Messages.postError("Problem with syntax setting attributes" +
                            " from the file:\n    " + fullpath);
                    Messages.postError("getAllSCNodes failed on line: '" +
                            string + "'");
                    break;  // Bail out
                }
                // Get the name of the attribute being set.
                attr = tok.sval;
                tok.nextToken();
                if(tok.ttype != '=') {
                    Messages.postError("Problem with syntax (=) setting attributes" +
                            " from the file:\n" + fullpath);
                    Messages.postError("getAllSCNodes failed on line: '" +
                            string + "'");
                    break;  // Bail out
                }
                tok.nextToken();
                if(tok.ttype != '\"') {
                    Messages.postError("Problem with syntax (\") setting attributes" +
                            " from the file:\n" + fullpath);
                    Messages.postError("getAllSCNodes failed on line: '" +
                            string + "'");
                    break;  // Bail out
                }
                // Get the value for this attribute.
                value = tok.sval;

                // We cannot add an empty string to the DB so if the value 
                // is empty, make it a single space.
                if(value.length() == 0)
                    value = " ";

                // If we have found "scans", then we want this "value" to get the
                // list of nodes out of.  It will be a comma separate list of
                // protocol names.  This is all we were after.
                if(attr.equals("scans")) {
                    break;
                }
            }
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem reading " + fullpath);
                Messages.postError("getAllSCNodes failed on line: '" +
                        string + "'");
            }
            return new ArrayList<String>();
        }   

        // Now we should have the comman separated list of protocol nodes
        // in "value".  First check that we really did end up with "scans".
        if(attr == null || !attr.equals("scans")) {
            Messages.postError("Did not find \'scans\' keyword in " + fullpath);
            return new ArrayList<String>();
        }

        // Be sure we have something in "value", then proceed to parse the list
        if(value == null || value.length() == 0) {
            Messages.postError("Did not find value for \'scans\' keyword in " + fullpath);
            return new ArrayList<String>();
        }
        
        // Okay, after all of that, we should have a comma separate list of
        // protocol name.  Parse the string.
        StringTokenizer stok = new StringTokenizer(value, ",");
        try {
            while(stok.hasMoreTokens()) {
                // Get the protocol name
                node = stok.nextToken();

                if(node != null)
                    names.add(node);
            }
        }
        catch (Exception ex) {

        }
        
        // Get the name of this SC which will be the directory name within
        // the studycardlib directory.  The nodes will be in
        // studycardlib/SCName
        index = fullpath.lastIndexOf(File.separator);
        String scName = fullpath.substring(index +1, fullpath.length() -4);
        

        // We have the list of protocol names, now we need the fullpaths for each
        for(int i=0; i < names.size(); i++) {
            String name = names.get(i);
            if(!name.endsWith(".xml"))
                name = name.concat(".xml");
            
            
            // Find fullpath for this node within studycardlib
            // Use studycardlib which is associated with the 'fullpath' base dir.
            // The standard protocol dir is basedir/template/vnmrj/protocol/
            // We want basedir/studycardlib
            // Get the basedir
            String searchStr = "templates" + File.separator + "" +
            		           "vnmrj"+ File.separator +
            		           "protocols";
            int baseIndex = fullpath.indexOf(searchStr);
            String basedir;
            if(baseIndex == -1) {
                // If the path is not within a template/vnmrj/protocol/
                // directory, then see if we are within a studycardlib dir
                searchStr = "studycardlib" + File.separator;
                baseIndex = fullpath.indexOf(searchStr);
                basedir = fullpath.substring(0, baseIndex);

            }
            else {
                basedir = fullpath.substring(0, baseIndex);
            }
            
            // Create the fullpath to this node
            String nodepath;
            nodepath =  basedir + "studycardlib" + File.separator  
                        + scName + File.separator + name;
            if(nodepath == null)
                continue;
            
            // We need to canonical path for DB use, so convert it
            UNFile nodeFile = new UNFile(nodepath);
            if(nodeFile.exists()) {
                nodepath = nodeFile.getCanonicalPath();
                nodes.add(nodepath);
            }
            
        }
        
        return nodes;
    }

    // We need to get the values of items defined in the protocol .xml
    // files.  The result returned is a list of keywords and values.
    // Return null if failure.
    static public ArrayList<KeywordNvalue> parseProtocol(String fullpath) {
        BufferedReader  in;
        UNFile file;
        ArrayList<KeywordNvalue> returnInfo = new ArrayList<KeywordNvalue>();
        String          inLine=null;
        String          rootFilename;
        String          string=null;
        int             index;
        String          attr;
        String          value;
        boolean         foundName=false;
        StringReader    sr;
        StreamTokenizerQuotedNewlines tok;
        StringTokenizer tokstring;

        
        file = new UNFile(fullpath);

        // Open the file.
        try {
            in = new BufferedReader(FileUtil.getUTFReader(file));
        } catch(Exception e) {
            Messages.postError("Problem opening " + fullpath);
            Messages.writeStackTrace(e, "Error caught in ProtocolBrowser");
            return null;
        }
        // Get the line/s to parse from the first occurrence of a '<'
        // followed by an alpha character and continuing to the next '>'.
        try {
            // Read one line at a time.
            while ((inLine = in.readLine()) != null) {
               inLine = inLine.trim();
               if (string == null && inLine.length() > 1 &&
                                                inLine.startsWith("<")) {
                   // Is next char a letter?
                   if(Character.isLetter(inLine.charAt(1))) {
                       // We are at the start of our string, save it.
                       string = new String(inLine);

                       // If this line contains the closing '>', then
                       // stop reading now.
                       if ((index = inLine.indexOf('>')) != -1) {
                           // Only take the string up to the '>'
                           string = inLine.substring(0, index);
                           break;
                       }
                   }
               }
               // Concatenate to string until '>' is found.
               else if (string != null && inLine.indexOf('>') == -1) {
                   string = string.concat(" " + inLine);
               }
               // Include the line up to the '>'
               else if (string != null && (index = inLine.indexOf('>')) != -1) {
                   // Concatenate up to the '>' and include a space.
                   string = string.concat(" " + inLine.substring(0, index));
                   break;
               }

            }
            in.close();

            if(string == null)
                throw (new Exception("Badly formated xml file"));
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem reading " + fullpath
                        + "\n   failed on line: '" +inLine + "'");
                Messages.writeStackTrace(e);
            }

            try {
                in.close();
            }
            catch(Exception  eio) {
            }
            return null;
        }
        // We should now have something like
        //    <template panel_type="acquisition" element_type="panels"
        // Tokenize the string.

        // Use StreamTokenizer because we can keep the quoted string
        // as a single token.  StringTokenizer does not allow that.
        // I had to create StreamTokenizerQuotedNewlines to allow quoted
        // string to contain newLines.  The standard one ends the quote
        // at the end of a line.  Sun had a bug reported about this
        // (4239144) and refused to add an option to allow multiple lines
        // in a quoted string.  GRS
        sr = new  StringReader(string);
        tok = new StreamTokenizerQuotedNewlines(sr);
        // Define the quote character
        tok.quoteChar('\"');
        // Define additional char to be treated as normal letter.
        tok.wordChars('_', '_');
        try {
            // Dump the first two tokens.  They will be '<' and the
            // word following '<'.
            tok.nextToken();
            tok.nextToken();
            // Now we should have a sequence of TT_WORD, '=', quoted string
            // for each attribute being defined.
            while(tok.nextToken() != StreamTokenizerQuotedNewlines.TT_EOF) {
                if(tok.ttype != StreamTokenizerQuotedNewlines.TT_WORD) {
                    Messages.postError("Problem with syntax setting attributes" +
                                      " from the file:\n    " + fullpath);
                    Messages.postError("\n   Failed on line: '" + string + "'");
                    break;  // Bail out
                }
                // Get the name of the attribute being set.
                attr = tok.sval;
                tok.nextToken();
                if(tok.ttype != '=') {
                    Messages.postError("Problem with syntax (=) setting attributes" +
                                  " from the file:\n" + fullpath);
                    Messages.postError("Failed on line: '" + string + "'");
                    break;  // Bail out
                }
                tok.nextToken();
                if(tok.ttype != '\"') {
                    Messages.postError("Problem with syntax (\") setting attributes" +
                                 " from the file:\n" + fullpath);
                    Messages.postError("Failed on line: '" + string + "'");
                    break;  // Bail out
                }
                // Get the value for this attribute.
                value = tok.sval;

                if(attr.equals("name"))
                    foundName = true;

                KeywordNvalue info = new KeywordNvalue(attr, value);
                returnInfo.add(info);            
            }
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem reading " + fullpath
                                  + "\n    Failed on line: '" + string + "'");
            }
            return null;
        }

        return returnInfo;
    }
    
    // We need to get the values of items defined in the protocol's procpar 
    // file.  The result returned is a list of keywords and values.
    // Return null if failure.
    static public ArrayList<KeywordNvalue> parseProcpar(String fullpath) {
        ArrayList<KeywordNvalue> returnInfo = new ArrayList<KeywordNvalue>();
        
        BufferedReader  in=null;
        String          parPath;
        String          dataType;
        String          param="";
        int             numVals;
        String          basicType, subType;
        StreamTokenizerQuotedNewlines tok;
        int             typeMatch;
        UNFile          file;

         
        // In Study Cards, the protocols are in studycardlib and the .xml files
        // and .par directories are in the same location.  Thus just create
        // a parPath that is identical to fullpath but ends in .par instead of .xml
        if(fullpath.endsWith(Shuf.DB_PROTOCOL_SUFFIX)) {
            parPath = fullpath.substring(0, fullpath.length() -3);
            parPath = parPath + "par" + File.separator + "procpar";
        }
        else {
            Messages.postWarning("Problem getting procpar for " + fullpath);
            return null;
        }

        // Test for the file first.
        file = new UNFile(parPath);
        if(!file.isFile()) {      
            // Now get out of here, there is no .par file to parse
            return null;
        }

        // Open the file.
        try {
            in = new BufferedReader(new FileReader(file));
        } 
        catch(Exception e) {
            Messages.postWarning("Problem opening " + parPath);
            Messages.writeStackTrace(e, "Error caught in parseProcpar");
            return null;
        }
        
        // Get dbManager for utility calls later
        ShufDBManager dbManager = ShufDBManager.getdbManager();
        
        // Use StreamTokenizer because we can keep the quoted string
        // as a single token.  StringTokenizer does not allow that.
        // I had to create StreamTokenizerQuotedNewlines to allow quoted
        // string to contain newLines.  The standard one ends the quote
        // at the end of a line.  Sun had a bug reported about this
        // (4239144) and refused to add an option to allow multiple lines
        // in a quoted string.  GRS
        tok = new StreamTokenizerQuotedNewlines(in);
        // Define the quote character
        tok.quoteChar('\"');
        // Define additional chars to be treated as normal letters.
        // Most things within a comment, sample name etc will be within
        // double quotes and do not need to be listed here.  We need things
        // here that can be in parameter names.
        tok.wordChars('_', '_');

        // I incountered 2 Java bugs in using StreamTokenizer.
        // One is that is does not handle 1e18 format of numbers.
        // I am not worried about it not turning 1e18 format into numbers
        // because I don't need the numeric value anyway.  I would just
        // turn it into a string and send it to the DB.
        // Two is that tok.parseNumbers() is called by its constructor
        // and thus is always on, and tok.parseNumbers() does not allow
        // you to turn it off.
        // To turn off the numeric thing by hand, first call ordinaryChars,
        // then specify numbers to be normal word characters.
        // Then when we encounter 1.3e-13 we get it as one string token.
        // tok.nval will never contain anything, because ALL letters
        // and numbers will be turned into 'String' tokens.
        tok.ordinaryChars('+', '.'); // + comma - and period  (Turn off numeric)
        tok.wordChars('+', '.'); // + comma - and period (Set as normal letters)
        tok.ordinaryChars('0', '9'); // all numbers   (Turn off numeric)
        tok.wordChars('0', '9');     // all numbers   (Set as normal letters)
        tok.wordChars('$', '$');     // all numbers   (Set as normal letters)
        try {
            // Parse the vnmr file.
            while(tok.nextToken() != StreamTokenizerQuotedNewlines.TT_EOF) {
                // There should be 11 tokens on the first line and we
                // only want the first 3.  The first is a string and
                // the 2nd and 3rd are integers
                param = tok.sval;
                tok.nextToken();
                subType = tok.sval;
                // If null or negative, this is a bad format file
                // If not an integer, an exception will be thrown.
                try {
                    if(subType == null || Integer.parseInt(subType) < 0) {
                        if(in != null)
                            in.close();
                        return null;
                    }
                }
                catch (Exception e) {
                    try {
                        in.close();
                    } catch(Exception eio) {}
                    return null;
                }

                tok.nextToken();
                basicType = tok.sval;

                // This would mean a bad format, so skip this file
                if(basicType == null ||
                   (!basicType.equals("1") && !basicType.equals("2"))) {
                    try {
                        in.close();
                    } catch(Exception eio) {}
                    return null;
                }

                // Dump the next 8
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();
                tok.nextToken();

                tok.nextToken();
                numVals = Integer.parseInt(tok.sval);
                if(basicType.equals(T_REAL)) {

                    dataType = DB_ATTR_FLOAT8;

                    /* See if we want use this param or not. We wait to check until now, 
                     * because we have to read everything out of the procpar file anyway.
                       Returns 0 for false, 1 for true and
                       2 for true except item in DB is array type.
                    */
                    typeMatch = dbManager.isParamInList(Shuf.DB_PROTOCOL, param, dataType);

                    // The param temp is not usable in postgres, so change the
                    // string to "temperature".
                    if(param.equals("temp"))
                        param = "temperature";

                    /* If == 1, the type matches and is not an array.
                       Loop thru in case the procpar had an array.
                       If == 0, no match, so just loop thru to dump them. */
                    if(typeMatch < 2) {
                        for (int i=0; i < numVals; i++) {
                            tok.nextToken();
                            // Only take first value and
                            // only if param is wanted.
                            if(i == 0 && typeMatch == 1) {
                                // We only want the first value.                                
                                KeywordNvalue kNv = new KeywordNvalue(param, tok.sval);
                                returnInfo.add(kNv);            


                                
                                // If protocol AND param is sfrq, then we
                                // want to calc a field in Tesla and add it
                                if(param.equals("sfrq")) {
                                    double field = dbManager.convertFreqToTesla(tok.sval);
                                    kNv = new KeywordNvalue("field", Double.toString(field));
                                    returnInfo.add(kNv);            
                                }
                            }
                            // else, not the first, just let them be dumped.
                        }
                    }
                    /* We have a param match and it is an array type.
                       Even if procpar only has one value, we still have
                       to send sql command to set an array.
                    */
                    if(typeMatch == 2) {
                        for (int i=0; i < numVals; i++) {
                            tok.nextToken();
                        }
                    }

                    /*  Read the enumeration values and ignore */
                    tok.nextToken();
                    numVals = Integer.parseInt(tok.sval);
                    for (int i=0; i < numVals; i++)
                        tok.nextToken();
                }
                else if(basicType.equals(T_STRING)) {
                    if(param.startsWith("time"))
                        dataType = DB_ATTR_DATE;
                    else
                        dataType = DB_ATTR_TEXT;

                    /*
                      Do we want this parameter?
                      Returns 0 for false, 1 for true and
                      2 for true except item in DB is array type.
                    */
                    typeMatch = dbManager.isParamInList(Shuf.DB_PROTOCOL, param, dataType);
                    /* If == 1, the type matches and is not an array.
                       Loop thru in case the procpar had an array.
                       If == 0, no match, so just loop thru to dump them. */
                    if(typeMatch < 2) {
                        for (int i=0; i < numVals; i++) {
                            tok.nextToken();
                            // We only want the first value and
                            // only if param is wanted
                            if(i == 0 && typeMatch == 1) {
                                String value = tok.sval;

                                /* If the name starts with 'time', we may
                                   need to convert a trailing 'Z' to 'UTC'.
                                   ISO 8601 specifies 'Z' but Postgres
                                   accepts 'UTC'. Actually it accepts 'UT'
                                   also.
                                */
                                if(param.startsWith("time")) {
                                    if(value.endsWith("Z")) {
                                        value = value.substring(
                                            0,value.length()-1);
                                        value = value.concat("UT");
                                    }
                                }
                                // Limit string value length. Too many causes
                                // an sql error
                                if(value.length() > MAX_ARRAY_STR_LEN) {
                                    value = value.substring(0,
                                                            MAX_ARRAY_STR_LEN);
                                }
                                
                                /* Just get the first value. */
                                KeywordNvalue kNv = new KeywordNvalue(param, value);
                                returnInfo.add(kNv);            
                            }
                        }
                        // else, not the first, just let them be dumped.
                    }
                    /* We have a param match and it is an array type.
                    */
                    if(typeMatch == 2) {
                        for (int i=0; i < numVals; i++) {
                            tok.nextToken();
                        }
                    }

                    /*  Read the enumeration values and ignore */
                    tok.nextToken();
                    numVals = Integer.parseInt(tok.sval);
                    for (int i=0; i < numVals; i++)
                        tok.nextToken();
                }
            }
            in.close();
        }
        catch (Exception e) {
            
                Messages.postError("Problem adding " +  parPath+
                                     " to Study Card Tab near param = " + param);
                Messages.writeStackTrace(e, "Error caught in parseProcpar");
            

            try {
                in.close();
            } 
            catch(Exception eio) {}
            return null;
        }


        try {
            in.close();
        } catch(Exception e) {  }
     
        
        
        return returnInfo;
    }

}

