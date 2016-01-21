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
import javax.swing.border.Border;


import java.awt.*;
import java.awt.event.*;
import java.io.File;
import java.util.*;

import vnmr.bo.BorderDeli;
import vnmr.bo.SearchResults;
import vnmr.ui.ExpViewArea;
import vnmr.ui.SessionShare;
import vnmr.util.DisplayOptions;
import vnmr.util.Messages;
import vnmr.util.Util;
import vnmr.util.SimpleHLayout;
import vnmr.util.SimpleVLayout;


/********************************************************** <pre>
 * Summary: Protocol Browser Panel
 *
 * Create a Panel for browsing protocols
 </pre> **********************************************************/

public class ProtocolBrowserPanel extends JPanel implements ItemListener{

    static ProtocolBrowser parentPanel = null;
    // Keep current keyword btn to get information of the location
    private JButton activeKeyBtn;
    // Keep so that its location can be set after the panel is complete
    private JPanel innerPanel;
    private JScrollPane scrollPane;
    private ArrayList <JButton> btns;

    public ProtocolBrowserPanel(ProtocolBrowser protocolBrowser) {
        
        // Save the parent.
        parentPanel = protocolBrowser;

        Color bgColor = Util.getBgColor();
//        setBackground(bgColor);
        // Name to show up in Tab
        setName(Util.getLabel("Search", "Search"));
        
        
        // Setup the Layout so the items show correctly
        BorderLayout layout = new BorderLayout();
        setLayout(layout);
        
        this.setComponentOrientation(ComponentOrientation.LEFT_TO_RIGHT);
        
        // We need the system keyword (all of the keyword) information
        // before we can really get serious about the panel.  When this
        // system info is complete, our parent (ProtocolBrowser) will call
        // us to create the panel.  Thus, we will leave it as just an
        // empty shell right now to be completed in createPanel()
                  
    }


    // Create the part of the panel where they can go backwards
    // along the series of keywords such as "owner->species->anatomy".
    // When they click on an item before the last one (such as species above)
    // the panel will be updated to have species be the active position.
    // We want two rows.  The second will be the value of the keyword
    // and should be directly below the keyword.
    public JPanel createBrowsingPanel (ProtocolKeywords keywords) {
        // Set the button background so that is does not look like a btn
        // but looks like text on the panel.
        Color bgColor = Util.getBgColor();
        boolean firstItem=true;
        int horizMargin = 4;
 
        JPanel browsingPanel = new JPanel();
               
        // Left justify the contents of this subpanel
        FlowLayout layout = new FlowLayout(FlowLayout.LEFT);
        browsingPanel.setLayout(layout);

        int activePos = keywords.getActivePosition();
        
        // Only display the items (starting at 0) which are "keyword-search" keywords.
        // If position is greater than keywords size, then stop.
        // Highlight the one at position "position"
        for(int i=0, curPosition=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.getKeywordByIndex(i);
            if(!keyword.type.equals("keyword-search"))
                continue;
            
            // Only display btns for keywords up to and including the active one
            if(curPosition > activePos)
                break;

            // Add ">" if this is not the first item
            if(!firstItem) {
                JLabel label = new JLabel(">");
                browsingPanel.add(label);
                firstItem = true;
            }
            firstItem = false;
            
            String keywordName = keyword.getName();
            // If it ends in underscore, remove the underscore
            if( keywordName.endsWith("_") ) {
                keywordName = keywordName.substring(0, keywordName.length() -1);
            }

            // It is a keyword-search parameter, so add it.  
            // Cannot make a multi line button normally, but you can if
            // you use html syntax. Display both the keyword name
            // and its current value.
//            JButton btn = new JButton("<html>" + keywordName + "<p>"
//                    + keyword.getValue() + "</html>");
            
            // Just show keyword, not value
            JButton btn = new JButton(keywordName);
            
            btn.setBorderPainted(false);
            
            // Make the margins somewhat small so there is not too much
            // space between ">" and the names
            Insets margin = new Insets(2, horizMargin, horizMargin, 2);
            btn.setMargin(margin);
            
            // Use system colors and font
//            btn.setBackground(bgColor);
            
//            Color fg = DisplayOptions.getColor("Heading1");
//            btn.setForeground(fg);
            
            Font font = DisplayOptions.getFont("Heading1");
            btn.setFont(font);

            
            // Set the actionCommand so we can set the proper activePosition
            // when this btn is clicked
            btn.setActionCommand(String.valueOf(curPosition));
            btn.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    String newPos = evt.getActionCommand();
                    ProtocolKeywords keywords = parentPanel.getKeywords();
                    // Set active position to the position of this button
                    // if it has changed
                    boolean changed =keywords.setActivePosition(newPos);
                    // Update the panel based on this new active position
                    // if changed
                    if(changed)
                        updatePanel(keywords);
                }
            });
            // If this is the active keyword, then we need to save it
            // for getting the location later.
            if(curPosition == activePos)
                activeKeyBtn = btn;

            browsingPanel.add(btn);
            curPosition++;      
            
        }
        return browsingPanel;
        
    }


    // Do a DB search using "distinct" to get the list of values available
    // for this position in the keywords.
    // The layout requested is to create the buttons in this panel as
    // a single column.  Then that column of btns needs to be positioned
    // so that it is located under the appropriate keyword button in the
    // browsingPanel.  To do this, I create a JPanel to put the buttons
    // in as a single column.  Then create a second JPanel which will be
    // the one placed into "this".  Then the button panel will be place
    // within the outer panel using setLocation() to position it.
    public JPanel createDynamicKeywordBtnPanel(ProtocolKeywords keywords) {
        ArrayList<String> possibleValues;
        JPanel dynamicPanel = new JPanel();
        innerPanel = new JPanel();
        
        // Use SimpleVLayout so I can specify the button sizes to all be the same
        SimpleVLayout layout = new SimpleVLayout();
        innerPanel.setLayout(layout);
        
        // Use SimpleHLayout so I can later specify the exact location for
        // the innerPanel.
        dynamicPanel.setLayout(new SimpleHLayout());
        
        possibleValues = getPossibleValues(keywords);
        if(possibleValues == null)
            return dynamicPanel;
    
        // The default layout manager is FlowLayout
        // which allows buttons to be the size they want, meaning short words
        // make short buttons.  Then when a row is full, they go onto the next
        // row.  


        // Create the buttons
        btns = new ArrayList <JButton>();
        
        for (int i = 0; i < possibleValues.size(); i++) {
            String label = possibleValues.get(i);
            JButton btn = new JButton(label);
            
            // Save of of the buttons for resizing later
            btns.add(btn);

            // Add an action command so we know which btn was clicked
            btn.setActionCommand(label);
             
            // Add the action listener that will take care of clicking this
            // btn
            btn.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    String label = evt.getActionCommand();
                    ProtocolKeywords keywords = parentPanel.getKeywords();

                    // Get the current active keyword
                    ProtocolKeyword keyword = keywords.getActiveKeyword();

                    // Update keywords with this value(label) for the
                    // current active position.
                    keyword.setValue(label);
                    keywords.replaceAKeyword(keyword.getName(), keyword);

                    // Try to bump the active position up by one
                    // If we are at the last position, then do a DB search
                    // and swap to the protocol display panel.
                    int activePos = keywords.getActivePosition();
                    if (keywords.isLastPosition(activePos)) {
                        // We are at the last position, Do a DB search
                        parentPanel.doDBSearch(keywords, "protocol");
                    }
                    else {
                        // If we were not already at the last position,
                        // then bump the position by one and update the
                        // panel.
                        boolean changed = keywords.setActivePosition(activePos + 1);
                        if (changed)
                            // Update the panel based on this new info
                            updatePanel(keywords);
                    }

                }
            });
            
            innerPanel.add(btn);

        }
        scrollPane = new JScrollPane(innerPanel);
        Border empty = BorderFactory.createEmptyBorder();
        scrollPane.setBorder(empty);

        dynamicPanel.add(scrollPane);
        

        return dynamicPanel;
    }
    
    // Allow calling without studyCardName
    public static JPanel createParamsPanel(ProtocolKeywords keywords) {
        return createParamsPanel(keywords, null);
    }
    
    
    // Display all of the defined keywords with their values
    // Put these into a panel of their own.
    // Make this a static call so that this panel can also be used below
    // in the ProtocolBrowserResults Tab
    // This is a helper method to create param panels for Tab other than
    // Search (Search is this local class)
    public static JPanel createParamsPanel(ProtocolKeywords keywords, 
                                           String studyCardPath) {
        JPanel paramPanel = new JPanel();
        
        // Left justify the contents of this subpanel
        FlowLayout layout = new FlowLayout(FlowLayout.LEFT);
        paramPanel.setLayout(layout);

        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.getKeywordByIndex(i);
            if(!keyword.type.equals("system-search") && 
               !keyword.type.equals("keyword-search"))
                continue;
            
            // If this keyword has "skip" set, then put "All" as its value
            JLabel sysLabel;
            if(keyword.type.equals("system-search") && keyword.skip) {
                sysLabel = new JLabel(keyword.getName() + ": " 
                                      + "All   ");
            }
            else {
                // This is a system or search type keyword, use it
                sysLabel = new JLabel(keyword.getName() + ": " 
                                          + keyword.getValue() + "   ");
            }
            
            // Use system colors and font
//            Color bgColor = Util.getBgColor();
//            sysLabel.setBackground(bgColor);

//            Color fg = DisplayOptions.getColor("Label1");
//            sysLabel.setForeground(fg);
            
            Font font = DisplayOptions.getFont("Label1");
            sysLabel.setFont(font);

            
            paramPanel.add(sysLabel);
        }
        // If we need to display the studycard name, do so now
        // This can handle studyCardPath being a single name, or a fullpath.
        if(studyCardPath != null && studyCardPath.length() > 0) {
            // we have the fullpath, get the studycard name
            int index = studyCardPath.lastIndexOf(File.separator);
            String studyCardName = studyCardPath.substring(index +1);
            
            // If it ends in .xml (and it should) remove the .xml
            if(studyCardName.endsWith(".xml")) {
                studyCardName = studyCardName.substring(0, studyCardName.length() -4);
            }
            
            JLabel sysLabel;
            sysLabel = new JLabel("StudyCard: " + studyCardName);
            
            // Use system colors and font
//            Color bgColor = Util.getBgColor();
//            sysLabel.setBackground(bgColor);

//            Color fg = DisplayOptions.getColor("Label1");
//            sysLabel.setForeground(fg);
            
            Font font = DisplayOptions.getFont("Label1");
            sysLabel.setFont(font);

            paramPanel.add(sysLabel);
        }
        
        Border border = BorderDeli.createBorder("Etched", null, null,
                                                null, null, null);
        paramPanel.setBorder(border);
        
        return paramPanel;
    }

    public JPanel createSearchParamsPanel(ProtocolKeywords keywords) {
        JPanel paramPanel = new JPanel();
        
        // Left justify the contents of this subpanel
        FlowLayout layout = new FlowLayout(FlowLayout.LEFT);
        paramPanel.setLayout(layout);

        for(int i=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.getKeywordByIndex(i);
            if(!keyword.type.equals("system-search") && 
               !keyword.type.equals("keyword-search"))
                continue;
            
            // Only show the defined keywords for the Search Tab.  That is, up to 
            // but NOT including the active one.  Get the active keyword and 
            // compare to this one.  If we are on the active one, get out of here.
            ProtocolKeyword activeKeyword = keywords.getActiveKeyword();
            if(activeKeyword.getName().equals(keyword.getName()))
                break;
            
            // If system param, put a checkbox in front of it.
            if(keyword.type.equals("system-search")) {
                JCheckBox box = new JCheckBox();
                box.setSelected(!keyword.getSkip());
                // Save the keyword name for this keyword for retrieval within
                // the ItemListener
                box.setActionCommand(keyword.getName());
                paramPanel.add(box);
               
                box.addItemListener(ProtocolBrowserPanel.this);
            }
            
            // If this keyword has "skip" set, then put "All" as its value
            JLabel sysLabel;
            if(keyword.type.equals("system-search") && keyword.skip) {
                sysLabel = new JLabel(keyword.getName() + ": " 
                                      + "All   ");
            }
            else {
                // This must be a search type keyword, use it
                sysLabel = new JLabel(keyword.getName() + ": " 
                                          + keyword.getValue() + "   ");
            }
            
            // Use system colors and font
//            Color bgColor = Util.getBgColor();
//            sysLabel.setBackground(bgColor);

//            Color fg = DisplayOptions.getColor("Label1");
//            sysLabel.setForeground(fg);
            
            Font font = DisplayOptions.getFont("Label1");
            sysLabel.setFont(font);

            
            paramPanel.add(sysLabel);
        }
        Border border = BorderDeli.createBorder("Etched", null, null,
                                                null, null, null);
        paramPanel.setBorder(border);
        
        return paramPanel;
    }

    public void itemStateChanged(ItemEvent ev) {
        if (ev.getStateChange() == ItemEvent.SELECTED) {
            JCheckBox ckbox = (JCheckBox)ev.getItem();
            // We used ActionCommand for the keyword name
            String name = ckbox.getActionCommand();
            // Get the keyword with this name and set its skip 
            // status to use this keyword in searches.
            ProtocolKeywords keywords = parentPanel.getKeywords();
            ProtocolKeyword keywd = keywords.getKeywordByName(name);
            // Set the keyword's skip status for search
            // to not skip (use) this keyword
            keywd.setSkip(false);
            parentPanel.replaceAKeyword(name, keywd);
            
            // Update the param panel and the search btns
            ProtocolBrowserPanel.this.updatePanel(keywords);
        } 
        else {
            JCheckBox ckbox = (JCheckBox)ev.getItem();
            // We used ActionCommand for the keyword name
            String name = ckbox.getActionCommand();
            // Get the keyword with this name and set its skip 
            // status to use this keyword in searches.
            ProtocolKeywords keywords = parentPanel.getKeywords();
            ProtocolKeyword keywd = keywords.getKeywordByName(name);
            // Set the keyword's skip status for search
            // to skip (not use) this keyword
            keywd.setSkip(true);
            parentPanel.replaceAKeyword(name, keywd);
            
            // Update the param panel and the search btns
            ProtocolBrowserPanel.this.updatePanel(keywords);
        }
    }

    // Update the panel. Do this by removing all five items that were added,
    // and then filling again from scratch.
    public void updatePanel(ProtocolKeywords keywords) {
        this.removeAll();
        fillPanel(keywords);
        parentPanel.repaint();
    }
    
    
    // Fill the panel based on the current keywords and values
    public void fillPanel(ProtocolKeywords keywords) {

        JPanel browsingPanel = createBrowsingPanel(keywords);
        // Stop the browsingPanel from growing when the main panel is stretched 
        browsingPanel.setMaximumSize(browsingPanel.getPreferredSize());
        browsingPanel.setAlignmentX(LEFT_ALIGNMENT); // ??????
        this.add(browsingPanel, BorderLayout.NORTH);
        
        
        JPanel dynamicKeywordBtnPanel = createDynamicKeywordBtnPanel(keywords);
        this.add(dynamicKeywordBtnPanel, BorderLayout.CENTER);
        

        boolean showAll = false;
        JPanel paramsPanel = createSearchParamsPanel(keywords);
        // Stop from growing when the main panel is stretched
        paramsPanel.setMaximumSize(paramsPanel.getPreferredSize());
        paramsPanel.setAlignmentX(LEFT_ALIGNMENT);
        this.add(paramsPanel, BorderLayout.SOUTH);
        

        
        this.validateTree();

        // Set the position of panel to be directly under the active keyword
        Point curLoc = scrollPane.getLocation();
        Point activeLoc = activeKeyBtn.getLocation();
        int x = (int)activeLoc.getX();
        int y = (int)curLoc.getY();
        scrollPane.setLocation(x, y);
        
        
        Dimension panelDim = innerPanel.getSize(null);
                
        // Resize all of the buttons to be the size of the widest button
        for(int i=0; i < btns.size(); i++) {
            JButton btn = btns.get(i);
            Dimension btnDim = btn.getSize();
            // Plus 30 to make the button wide enough to allow the scroll
            // bar to not overlap part of the button text.  
            btnDim.setSize(panelDim.getWidth() +30, btnDim.getHeight());
            btn.setPreferredSize(btnDim);
        }
        scrollPane.setHorizontalScrollBarPolicy(ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER);
        // For some reason, the scrollpane thinks it is tall enough to hold
        // everything and scroll bars never appear.  This is to set the size
        // to match the current viewport area.
        ExpViewArea expViewArea = Util.getViewArea();
        Dimension dim = expViewArea.getSize();
        Dimension scrollDim = scrollPane.getPreferredSize();
        // The -105 is to account for the space taken up by the top (browsingPanel)
        // panel and the bottom (systemParamsPanel) panel.
        int height = (int)dim.getHeight() -105;
        int width = (int)scrollDim.getWidth();
        
        // If the new height is less than the scrollPane preferred, then
        // allow some more width to account for the scroll bar.  It is crap
        // that I am having to take care of all of these details.
        if(height < scrollDim.getHeight())
            width = width + 30;
        
        Dimension newDim = new Dimension(width, height);
        scrollPane.setPreferredSize(newDim);
        scrollPane.setSize(newDim);
            

        this.setVisible(true);
    }
    
    // 
    void setNewSize(Dimension dim) {
        setSize(dim);
        if(scrollPane != null) {
            // For some reason, the scrollpane thinks it is tall enough to hold
            // everything and scroll bars never appear.  This is to set the size
            // to match the current viewport area.
            Dimension scrollDim = scrollPane.getPreferredSize();
            // The -100 is to account for the space taken up by the top (browsingPanel)
            // panel and the bottom (systemParamsPanel) panel.
            int height = (int)dim.getHeight() -105;
            int width = (int)scrollDim.getWidth();
            
            Dimension newDim = new Dimension(width, height);
            scrollPane.setPreferredSize(newDim);
               
        }
    }
    

    // Determine what the possible values are for the keyword position given.
    // This is done using "DISTINCT" in an sql search command.
    // We need to do the search using the given values for all "system"
    // keywords AND the keyword-search keywords that come before the one
    // specified in "position".
    // The sql command should look something like:
    //     SELECT DISTINCT gcoil FROM protocol WHERE (application LIKE 'imaging' 
    //     AND owner LIKE 'varian');
    // This will get all possible values of gcoil where applications and owner
    // match the "where" test.

    public ArrayList<String> getPossibleValues(ProtocolKeywords keywords) {
        ArrayList<String> possibleValues = new ArrayList<String>();
        String keywordToGet=null;
        int pos = keywords.getActivePosition();
        
        // Always add a value of "All" so that the user can choose that
        // allow all values of the current keyword
        possibleValues.add("All");

        
        // First part of the command
        String sqlCmd = "SELECT DISTINCT ";
        
        // Now we need to add the keyword at "position" (only counting keyword-search keywords)
        for(int i=0, index=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.getKeywordByIndex(i);
            if(!keyword.type.equals("keyword-search"))
                continue;
            else {
 
                if(index == pos) {
                    sqlCmd = sqlCmd.concat(keyword.getName());
                    keywordToGet = keyword.getName();
                    // We got what we needed
                    break;
                }
                index++;
            } 
        }
        sqlCmd = sqlCmd.concat(" FROM " + Shuf.DB_PROTOCOL);
        
        // We need to save this part of the sql cmd in case there are no
        // entries in the WHERE portion so we can revert to this.
        String saveSql = new String(sqlCmd);

        sqlCmd = sqlCmd.concat(" WHERE (");

        // The keywords object should be sorted so that all system and fixed keywords
        // come before all keyword-search keywords.  The "position" will refer to the
        // index into the "user" keywords, starting at 0.
        boolean addedItem = false;
        for(int i=0, index=0; i < keywords.size(); i++) {
            ProtocolKeyword keyword = keywords.getKeywordByIndex(i);
            String value = keyword.getValue();
            String type = keyword.getType();
            // We have to do the check here and not in the for statement
            // so that we allow system and fixed keywords to go into the
            // sqlcmd even if there are no keyword-search keywords.
            if(type.equals("keyword-search") && index >= pos) 
                break;
            
            // Only add this keyword to the sql cmd if it has a value and
            // the value is not "All" and is not tagged "skip"
            if(value != null && value.trim().length() > 0 && 
                        !value.equals("All") && !keyword.getSkip()) {
                // Need AND for all but first one
                if(addedItem)
                    sqlCmd = sqlCmd.concat(" AND ");
            
                // Add this keyword and value to the sql command.
                // If the keyword is field, we need to treat it like a float
                // and not use the LIKE command, but instead use "=".
                if(keyword.getName().equals("field")) {
                    sqlCmd = sqlCmd.concat(keyword.getName() + " = \'" 
                                           + keyword.getValue() + "\'");
                }
                else {
                    sqlCmd = sqlCmd.concat(keyword.getName() + " LIKE \'" 
                                           + keyword.getValue() + "\'");
                }
                addedItem = true;
            }
            // If this is a user keyword, increment index so that we know
            // when to stop.  That is, we only want to take user keywords
            // up to, but not including "position".
            if(keyword.type.equals("keyword-search"))
                index++;
        }
        sqlCmd = sqlCmd.concat(")");
        
        // If we did not add any items to the WHERE portion, then revert to
        // the sql cmd saved before that section.
        if(!addedItem)
            sqlCmd = saveSql;
            
        ShufDBManager dbManager = ShufDBManager.getdbManager();   
        String value;
        try {
            // Execute the DB search
            java.sql.ResultSet result = dbManager.executeQuery(sqlCmd);
            if(result == null) {
                // It should already have All as the first position
                return possibleValues;
            }
            
            while(result.next()) {
                // There is only one column of results, therefore, col 1
                value = result.getString(1);
                if(value != null) {
                    // Be sure it is not whitespace
                    value = value.trim();
                    if(value.length() > 0) {
                        // Add this value to the list
                        possibleValues.add(value);
                    }
                }
            }
        }
        catch (Exception ex) {
            Messages.postError("Problem getting DB values for " + keywordToGet
                    + ".\n    sqlcmd: " + sqlCmd);
        }

        return possibleValues;
    }

    
    public void unshowTabbedPane() {
        parentPanel.unshow();
        
    }

    static public Point getParentLocation() {
        if(parentPanel == null)
            return null;
        else
            return parentPanel.getLocationOnScreen();
    }

}  // ProtocolBrowserPanel
