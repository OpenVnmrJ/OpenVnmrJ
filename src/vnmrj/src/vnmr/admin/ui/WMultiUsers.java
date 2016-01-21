/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.util.*;
import vnmr.bo.VGroupIF;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 *
 *
 *
 */

public class WMultiUsers extends WConvertUsers implements ActionListener
{

    protected Container m_contentPane   = null;
    protected JCheckBox m_chkExp        = null;
    protected JCheckBox m_chkImg        = null;
    protected JCheckBox m_chkWlk        = null;
    protected JCheckBox m_chkAdm        = null;
    protected String[]  m_aStrItype     = {Global.IMGIF, Global.WALKUPIF};
    protected ButtonGroup m_rdGroup     = new ButtonGroup();
    protected ButtonGroup cd_rdGroup     = new ButtonGroup();
    protected JLabel    m_lblNames      = null;
    protected JTextField m_txfNames     = null;
    protected JButton m_browseBtn       = null;
    protected JFileChooser browsePopup  = null;
    protected ActionListener m_alItype  = null;
    JPanel    pnlDisplay                = null;
    JPanel    pnlItype                  = null;
    JPanel    pnlcd                     = null;
    JPanel    pnlradio                  = null;
    JCheckBox ckBox                     = null;
    protected Boolean useFileInput      = false;
    HashMap<String,HashMap> userInfoFromFile=null;


    public WMultiUsers(SessionShare sshare)
    {
        super(sshare);

        m_contentPane = getContentPane();

        historyButton.setEnabled(false);
        helpButton.setEnabled(false);
        undoButton.setEnabled(false);

        abandonButton.setActionCommand("cancel");
        abandonButton.addActionListener(this);
        closeButton.setActionCommand("add");
        closeButton.addActionListener(this);
        closeButton.setText("Add Users");

        m_alItype = new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                if (m_rdGroup.getSelection() != null)
                {
                    closeButton.setEnabled(true);
                }
                else
                {
                    closeButton.setEnabled(false);
                }
            }
        };

        setTitle("Create/Delete 2 or more users");
        initlayout();
        setResizable(true);
    }

    public void setVisible(boolean bVis)
    {
        super.setVisible(bVis);
//        setCloseButtonText(null);
    }

    protected void initlayout()
    {
        pnlDisplay = new JPanel();
        pnlDisplay.setLayout(new BorderLayout());

        m_lblNames = new JLabel("Login names:");
        m_txfNames = new JTextField();
        m_browseBtn = new JButton("Browse");
        m_txfNames.addActionListener(m_alItype);
        m_browseBtn.setActionCommand("browse");
        m_browseBtn.addActionListener(this);


        m_lblNames.setToolTipText("Login names of users");
        m_txfNames.setToolTipText("Enter login names of users seperated by space");

        pnlDisplay.add(m_lblNames, BorderLayout.WEST);
        pnlDisplay.add(m_txfNames, BorderLayout.CENTER);
        pnlDisplay.add(m_browseBtn, BorderLayout.EAST);
        // Only show the Browse button if adding multiple users via file input
        m_browseBtn.setVisible(false);

        // Add interface checkbox.
        pnlItype = new JPanel();
        pnlcd = new JPanel();
        pnlradio = new JPanel();
        pnlradio.setLayout(new BorderLayout());
        /*m_chkExp = new JCheckBox(Global.EXPIF);
        m_chkImg = new JCheckBox(Global.IMGIF);
        m_chkWlk = new JCheckBox(Global.WALKUPIF);
        m_chkAdm = new JCheckBox(Global.ADMINIF);
        m_chkExp.addActionListener(m_alItype);
        m_chkImg.addActionListener(m_alItype);
        m_chkWlk.addActionListener(m_alItype);
        m_chkAdm.addActionListener(m_alItype);*/

        // Add buttons for itype
        int nSize = m_aStrItype.length;
        for (int i = 0; i < nSize; i++)
        {
            JRadioButton rdInterface = new JRadioButton(m_aStrItype[i]);
            m_rdGroup.add(rdInterface);
            pnlItype.add(rdInterface);
            rdInterface.setActionCommand(m_aStrItype[i]);
        }
        // Add buttons for Create/Delete
        JRadioButton cBtn = new JRadioButton("Create");
        cBtn.setActionCommand("Create");
        cd_rdGroup.add(cBtn);
        pnlcd.add(cBtn);
        JRadioButton dBtn = new JRadioButton("Delete");
        dBtn.setActionCommand("Delete");
        cd_rdGroup.add(dBtn);
        pnlcd.add(dBtn);
        
        cBtn.addItemListener(new ItemListener() {
            public void itemStateChanged(ItemEvent evt) {
                // Catch state change of radio btn
                int state = evt.getStateChange();
                // Change the text of the closeButton as per
                // the state of the create btn
                if(state == ItemEvent.SELECTED) {
                    closeButton.setText("Add Users");
                    closeButton.setActionCommand("add");
                }
                else {
                    closeButton.setText("Delete Users");
                    closeButton.setActionCommand("delete");
               }
            }
        });
        
        // Default Create to be selected
        cBtn.setSelected(true);
        
        // Add the two radio button groups to another panel
        pnlradio.add(pnlItype, BorderLayout.NORTH);
        pnlradio.add(pnlcd, BorderLayout.CENTER);
        
        // Start with the create/delete buttons not shown
        pnlcd.setVisible(false);

        String strDefaultItype = WUtil.getAdminItype();
        if (strDefaultItype != null)
        {
            if (strDefaultItype.equalsIgnoreCase(Global.WALKUPIF))
                setSelected(true, Global.WALKUPIF);
            else if (strDefaultItype.equalsIgnoreCase(Global.IMGIF))
                setSelected(true, Global.IMGIF);
        }

        ckBox = new JCheckBox("Use File for User list input");
        ckBox.setToolTipText("Uncheck to enter names in panel, check to input from file");
        
        ckBox.addItemListener(new ItemListener() {
            public void itemStateChanged(ItemEvent evt) {
                // Catch state change of checkbox
                int state = evt.getStateChange();
                // Show or unshow items for proper state of either
                // using the string of users entered, or the file entered.
                if(state == ItemEvent.SELECTED) {
                    m_lblNames.setText("Input File Path:");
                    m_lblNames.setToolTipText("Login names of users");
                    m_browseBtn.setVisible(true);
                    pnlItype.setVisible(false);
                    pnlcd.setVisible(true);
                    useFileInput = true;
                    setCloseButtonText(true);
                }
                else {
                    m_lblNames.setText("Login names:");
                    m_lblNames.setToolTipText("Login names of users");
                    m_browseBtn.setVisible(false);
                    pnlItype.setVisible(true);
                    pnlcd.setVisible(false);
                    useFileInput = false;
                    setCloseButtonText(false);
                }
            }
        });

        pnlDisplay.add(ckBox, BorderLayout.SOUTH);

        m_contentPane.add(pnlDisplay, BorderLayout.NORTH);
        m_contentPane.add(pnlradio, BorderLayout.CENTER);

        setLocation( 300, 500 );

        int nWidth = buttonPane.getWidth();
        int nHeight = buttonPane.getPreferredSize().height +
                        pnlDisplay.getPreferredSize().height + 100;
        setSize(500, nHeight);
    }

        

    // Set the text of the closeButton based on ckBox and cBtn
    protected void setCloseButtonText(boolean fileBoxSelected) {
        // Get state of Create or Delete radio button
        if (cd_rdGroup != null && ckBox != null) {
            String selCmd = cd_rdGroup.getSelection().getActionCommand();
//            if(fileBoxSelected == null)
//            // Get state of ckBox
//            fileBoxSelected = ckBox.getModel().isPressed();
            if (!fileBoxSelected) {
                // If they have not checked the ckBox (File input not selected)
                // force the closeButton to "Add Users"
                closeButton.setText("Add Users");
                closeButton.setActionCommand("add");
            }
            else {
                // File input is selected, base the button text on the
                // state of the Create/Delete radio buttons
                if (selCmd.equals("Create")) {
                    closeButton.setText("Add Users");
                    closeButton.setActionCommand("add");
                }
                else {
                    closeButton.setText("Delete Users");
                    closeButton.setActionCommand("delete");
                }
            }
        }
    }
    
    
    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (cmd.equals("add") && isVisible()) {
            // Use string in panel as a list of users or a filepath
            String strNamesOrPath = m_txfNames.getText();
            if (strNamesOrPath != null && strNamesOrPath.length() > 0) {
                setVisible(false);
                if(useFileInput) {
                    // strNamesOrPath should be a path to the text
                    // input file.  We are not ready to use all the
                    // info in that file, but we need to know if we
                    // are adding users or operators.
                    HashMap<String,HashMap> userInfoFromFile2;
                    userInfoFromFile2 = getUserInfoFromFile(strNamesOrPath);
                    if(userInfoFromFile2 == null) {
                        return;
                    }
                    // Look into one of the operator/user HashMaps to see
                    // if "operator" is one of the keys.  If so, this is the
                    // flag for us to add operators from the file.
                    Iterator keySetItr = userInfoFromFile2.keySet().iterator();
                    String operator = null;
                    if (keySetItr.hasNext()) {
                        operator = (String) keySetItr.next();
                        HashMap hmUser = (HashMap) userInfoFromFile2.get(operator);
                        if (hmUser.containsKey("operator")) {
                            // Add Operators
                            addOperatorsFromFileList(strNamesOrPath);
                            return;
                        }
                    }
                }
                // Add users
                addUsers(strNamesOrPath);
            }
            else {
                if(useFileInput) {
                    Messages.postWarning("Please enter file path with users to add.");
                }
                else {
                    Messages.postWarning("Please enter login names for new users.");
                }
            }
        }
        else if (cmd.equals("delete")) {
            String strPath = m_txfNames.getText();
            if (strPath != null && strPath.length() > 0) {
                setVisible(false);
                if(useFileInput) {
                    // strPath should be a path to the text
                    // input file.  We are not ready to use all the
                    // info in that file, but we need to know if we
                    // are deleting users or operators.
                    HashMap<String,HashMap> userInfoFromFile2;
                    boolean delete = true;
                    userInfoFromFile2 = getUserInfoFromFile(strPath, delete);
                    if(userInfoFromFile2 == null) {
                        return;
                    }
                    // Look into one of the operator/user HashMaps to see
                    // if "operator" is one of the keys.  If so, this is the
                    // flag for us to delete operators from the file.
                    Iterator keySetItr = userInfoFromFile2.keySet().iterator();
                    String operator = null;
                    if (keySetItr.hasNext()) {
                        operator = (String) keySetItr.next();
                        HashMap hmUser = (HashMap) userInfoFromFile2.get(operator);
                        if (hmUser.containsKey("operator")) {
                            // Delete Operators
                            deleteOperatorsFromFileList(strPath);
                        }
                        else {
                            // Delete Users
                            deleteUsersFromFileList(strPath);
                        }
                    }
                }
            }
            else {
                Messages.postWarning("Please enter file path with users "
                        + "or operators to delete.");
            }

        }
        else if (cmd.equals("cancel")) {
            // undoAction();
            setVisible(false);
            dispose();
        }
        else if (cmd.equals("browse")) {
            if(browsePopup == null) {
                browsePopup = new JFileChooser();

                browsePopup.setFileSelectionMode(JFileChooser.FILES_ONLY);
            }
            
            int result = browsePopup.showOpenDialog(this);
            if(result == JFileChooser.APPROVE_OPTION) {
                File file = browsePopup.getSelectedFile();
                String path = file.getAbsolutePath();
                m_txfNames.setText(path);
            }
            else if(result == JFileChooser.ERROR_OPTION) {
                Messages.postError("Problem Opening File");
            }
        }
    }
    
    // Accept a space separated list of names to be added.  
    public void addUsers(String strNamesOrPath) {
        if (strNamesOrPath != null && strNamesOrPath.length() > 0)
        {
            // run the script to save data
            saveData();
            clearFields();
            setVisible(false);
            dispose();
        }
    }
    
    // If not useFileInput, then create an ArrayList of the names that were
    // entered in the text field.  If useFileInput == true, then the text
    // field contains a filepath to get users and info from.
    protected void saveData()
    {
        final ArrayList<String> aListNames;
        final HashMap hmUsers = new HashMap();
        String strItype;
        
        if(useFileInput) {
            String filepath = m_txfNames.getText();
            aListNames = new ArrayList<String>();
            // Retrieve info from the file.  We get back a HashMap
            // which has userName (login) for keys and another level of HashMap
            // for value.  This second level of HashMap has optionKeys
            // for keys and optionValue for values.  The optionKeys will
            // be the option name from the userDefaults file and the
            // optionValue will be the value for that key from filepath.
            userInfoFromFile = getUserInfoFromFile(filepath);
            if(userInfoFromFile == null)
                return;
            
            // We need An ArrayList of user login name, so create it
            // from the keys of userInfoFromFile.
            Iterator keySetItr = userInfoFromFile.keySet().iterator();
            String strKey = null;
            while(keySetItr.hasNext())
            {
                strKey = (String)keySetItr.next();
                aListNames.add(strKey);
            }
            // If they want itype set, it should be in the file.
            strItype = "";
        }
        else {
            aListNames = WUtil.strToAList(m_txfNames.getText(), false, " ,");
            strItype = getItype();            
        }

        addUsersToList(aListNames, hmUsers, strItype);

        new Thread(new Runnable()
        {
            public void run()
            {
                // This is calling the parent's method (WConvertUsers)
                saveData(aListNames, hmUsers, false, userInfoFromFile);
            }
        }).start();
    }

    // Call this one if we are not doing a delete
    protected HashMap<String,HashMap> getUserInfoFromFile(String filepath) {
        boolean delete = false;
        return getUserInfoFromFile(filepath, delete);
    }

    

    // Retrieve info from the file.  We give back a HashMap
    // which has userName for keys and another level of HashMap
    // for value.  This second level of HashMap has optionKeys
    // for keys and optionValue for values.  The optionKeys will
    // be the option name from the userDefaults file and the
    // optionValue will be the value for that key from filepath.
    protected HashMap<String,HashMap> getUserInfoFromFile(String filepath, boolean delete)
    {
        String strLine = null;
        ArrayList<String> optionKeys = new ArrayList<String>();
        boolean keyLineFound = false;
        HashMap<String,HashMap> userInfoFromFileL = new HashMap<String,HashMap>();
        
        // Be sure the file exist
        UNFile file = new UNFile(filepath);
        if(!file.exists()) {
            Messages.postError("File not found for creating users:\n   " + filepath);
            return null;
        }

        // Get the option keys from the first non comment line in filepath
        BufferedReader reader = WFileUtil.openReadFile(filepath);
        if(reader == null)
            return null;

        try {
            // read each line of the file
            while ((strLine = reader.readLine()) != null) {
                if (strLine.startsWith("#") || strLine.startsWith("@")
                        || strLine.length() == 0)
                    continue;
                
                // If there is an empty field in a spreadsheet, when it is
                // output to a .csv, it gives ",," for the empty field.
                // Unfortunately, the StringTokenizer skips that field
                // entirely.  ", ," (space between them) works fine.
                // I will replace all occurances of ",," with ", ,"
                // before creating the tokenizer.
                strLine = strLine.replaceAll(",,", ", ,");
                strLine = strLine.replaceAll(",,", ", ,");

                StringTokenizer sTokLine = new StringTokenizer(strLine, ",");
                
                if (!keyLineFound) {                    
                    // We must be at the first non comment, non empty line
                    // Get the keywords to use for the options. These should
                    // come from userDefaults and should be comma separated.
                    // The first one should be login which is not in userDefaults
                    while (sTokLine.hasMoreTokens()) {
                        optionKeys.add(sTokLine.nextToken().trim());
                    }
                    // This section is just for checking if proper column keywords exist
                    // for each possibility.
                    // If adding users, be sure we have at least the two tokens, login and
                    // itype. If not, they may not have used commas as separators
                    // or they may have skipped one of them.
                    // If adding operators, be sure we have the two tokens, operator and user.
                    // If deleting, we just need login or operator
                    
                    // Check for login or operator, we need one of them.
                    if(!optionKeys.contains("login") && !optionKeys.contains("operator")) {
                        // Did they include these without commas?  If so, both
                        // strings will probably be in the first and only value
                        String teststr = optionKeys.get(0);
                        int index1 = teststr.indexOf("operator");
                        int index2 = teststr.indexOf("login");
                        if(index1 > -1 || index2 > -1) {
                            // One of them was specified but probably without commas
                            Messages.postError("The column keywords need to be "
                                    + "specified as a comma separated\n list."
                                    + "    The following line was found: \n" 
                                    + "    \"" + strLine + "\"");
                            return null;
                        }
                        else {
                            // Neither was included
                            Messages.postError("\"login\" or \"operator\" columns "
                                    + "must be specified in the\n    file to add or delete users."  
                                    + "    The following line was found:\n " 
                                    + "    \"" + strLine + "\"");
                            return null;

                        }
                    }
                    // Don't allow both login and operator.
                    if(optionKeys.contains("login") && optionKeys.contains("operator")) {
                        Messages.postError("To add users, specify \"login\",\n"
                                + "    to add operators "
                                + "specify \"operator\", not both.\n"
                                + "    The following line was found:\n" 
                                + "    \"" + strLine + "\"");
                        return null;

                    }
                    if(!delete)  { 
                        // We must be adding users or operators, which one?
                        if(optionKeys.contains("login")) {
                            // Adding users
                            if(!optionKeys.contains("itype")) {
                                // Did they include these without commas?  If so, both
                                // strings will probably be in the first and only value
                                String teststr = optionKeys.get(0);
                                int index1 = teststr.indexOf("login");
                                int index2 = teststr.indexOf("itype");
                                if(index1 > -1 && index2 > -1) {
                                    // They were both specified but without commas
                                    Messages.postError("The column keywords need to be "
                                                       + "specified as a comma separated\n list. "
                                                       + "    The following line was found: " 
                                                       + "\"" + strLine + "\"");
                                    return null;
                                }
                                else {
                                    // Apparently login and itype were not both included
                                    Messages.postError("\"login\" and \"itype\" columns "
                                                       + "must both be\n    included"
                                                       +     " in the file to add users.\n"  
                                                       + "    The following line was found:\n" 
                                                       + "    \"" + strLine + "\"");
                                    return null;
                                }
                            }
                        }
                        else if(optionKeys.contains("operator")) {
                            // Adding operators
                            if(!optionKeys.contains("user")) {
                                // Did they include these without commas?  If so, both
                                // strings will probably be in the first and only value
                                String teststr = optionKeys.get(0);
                                int index1 = teststr.indexOf("operator");
                                int index2 = teststr.indexOf("user");
                                if(index1 > -1 && index2 > -1) {
                                    // They were both specified but without commas
                                    Messages.postError("The column keywords need to be "
                                                       + "specified as a comma separated\n list. "
                                                       + "    The following line was found:\n" 
                                                       + "    \"" + strLine + "\"");
                                    return null;
                                }
                                else {
                                    // Apparently operator and user were not both included
                                    Messages.postError("\"operator\" and \"user\" columns "
                                                       + "must both be\n"
                                                       + "    included in the file to add operators.\n"
                                                       + "    The following line was found:\n" 
                                                       + "    \"" + strLine + "\"");
                                    return null;
                                }
                            }
                        }
                    }

                    keyLineFound = true;
                }
                else {
                    // We should already have the optionKeys, now get the
                    // values of the options and the user names.  If there are
                    // fewer values than columns (keys) then it will just   
                    // skip that column for this user.
                    HashMap<String,String> userHM = new HashMap<String,String>();

                    for (int i=0; sTokLine.hasMoreTokens() && i < optionKeys.size(); i++) {
                        // This should be the value of optionKey "i"
                        String optionVal = sTokLine.nextToken().trim();
                        if(optionVal == null)
                            optionVal = "";
                        // The tokenizer will leave double quotes, so we need
                        // to remove them if they are present.
                        int index1 = optionVal.indexOf("\"");
                        int index2 = optionVal.lastIndexOf("\"");
                        if(index1 >= 0 && index2 > 0) {
                            optionVal = optionVal.substring(index1+1, index2);
                        }
                        // If value is empty, don't put into HashMap.  Then
                        // the default will still be used and not overwritten
                        if(optionVal.length() > 0 && !optionVal.equals(" ")) {
                            if(optionKeys.get(i).equals("itype")) {
                                // The values for itype need to start with capital
                                // letters (Spectroscopy and Imaging).  All the user
                                // to enter lower case and catch it and fix it.
                                if(optionVal.toLowerCase().startsWith("spec")) {
                                    optionVal = "Spectroscopy";
                                }
                                if(optionVal.toLowerCase().startsWith("imag")) {
                                    optionVal = "Imaging";
                                } 
                            }
                            // Fill a HashMap for each user
                            String optionKey = optionKeys.get(i);
                            userHM.put(optionKey, optionVal);
                        }
                    }
                    // Get the user login name to use as a key for the next
                    // level of HashMap.
                    String login = userHM.get("login");
                    if(login != null && login.length() > 0)
                        userInfoFromFileL.put(login, userHM);
                    else {
                        // If no login, then try operator
                        String operator = userHM.get("operator");
                        if(operator != null && operator.length() > 0)
                            userInfoFromFileL.put(operator, userHM);
                    }
                }

            }
        } 
        catch (Exception e) {
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
            return null;
        }
        
        
        return userInfoFromFileL;
    }
    
    protected void addOperatorsFromFileList(String addUserFile) {
        BufferedReader reader;
        String strPath = null;
        
        userInfoFromFile = getUserInfoFromFile(addUserFile);
        if(userInfoFromFile == null)
            return;
        // loop through login names from the keys of userInfoFromFile.
        Iterator keySetItr = userInfoFromFile.keySet().iterator();
        String operator = null;
        while(keySetItr.hasNext())
        {
            operator = (String)keySetItr.next();
            HashMap hmUser = (HashMap)userInfoFromFile.get(operator);
            // Get the user that this operator is to belong to
            String user = (String)hmUser.get("user");

            // Modify the profiles/user/username file to add this operator.
            // We want to just copy the current file, except we want to append
            // the operator name to the current "operator" line in the file.
            String outFile = FileUtil.openPath("USRPROF"+File.separator+user);
            StringBuffer outBuffer;
            try {
            reader = new BufferedReader(new FileReader(outFile));
            String inLine;
            outBuffer = new StringBuffer();
            boolean firstLine = true;
            while ((inLine = reader.readLine()) != null) {
                if(inLine.trim().startsWith("operator")) {
                    // This is the operator line, see if the new operator
                    // is already in the list.  String.matches() takes regexp as
                    // the arg.  In this regexp, \\b means a word boundary.
                    // The . means any char and the * means any number of ".".
                    // That is .* is used for match any sequence.
                    boolean found = inLine.matches(".*\\b" + operator + "\\b.*");

                    if(!found)
                        // Append the operator if it was not already there
                        inLine = inLine + " " + operator;
                }
                // If there were previous lines, add a newline
                if(!firstLine) 
                    outBuffer.append("\n");
                
                // Write it out whether or not it was changed.
                outBuffer.append(inLine);
                firstLine = false;
            }
            reader.close();
            }
            catch(Exception ex) {
                Messages.postError("Problem reading " + addUserFile);
                Messages.writeStackTrace(ex);
                return;
            }

            // Now open the same file for writing and write the new info
            // to adm/users/profiles/user/username
            BufferedWriter writer = WFileUtil.openWriteFile(outFile);
            WFileUtil.writeAndClose(writer, outBuffer);
            
            // Get the paths of /vnmr/adm/users/profiles/system/username
            // and /vnmr/adm/users/profiles/user/username separated by a colon
            String strProfDir = FileUtil.openPath("PROFILES");
            String strProfUserDir = strProfDir + File.separator + "user"  
                                    + File.separator + user;
            String strProfSysDir = strProfDir + File.separator + "system"  
            + File.separator + user;
            strPath = strProfSysDir + File.pathSeparator + strProfUserDir;
            
            HashMap hmItem = WFileUtil.getHashMap(strPath);
            
            // Now I need to get some items out of hmUser and add them to
            // hmItem so they can be used further down the line.  Basically,
            // further on, hmItem is used to get the info normally in HmItem
            // as retrieved above PLUS some of the new code to add operators
            // via list will be looking for email and profile.
            String email = (String)hmUser.get("email");
            String profile = (String)hmUser.get("profile");
            String operatorFull = (String)hmUser.get("name");
            hmItem.put("email", email);
            hmItem.put("profile", profile);
            hmItem.put("operatorFull", operatorFull);

            // Write the operatorlist file
            WUserUtil.writeOperatorFile(user, hmItem);
        }
        
        // This clears out the upper right quadrant since it may be
        // incorrect now.
        VAdminIF appIf = (VAdminIF)Util.getAppIF();
        VDetailArea vif = appIf.getDetailArea1();
        if(vif != null)
            vif.fillValues((Object) "", "");

        
           
        
    }
    
    public void deleteUsersFromFileList(String filepath) {
        // Retrieve info from the file.  We get back a HashMap
        // which has userName (login) for keys and another level of HashMap
        // for value.  This second level of HashMap has optionKeys
        // for keys and optionValue for values.  The optionKeys will
        // be the option name from the userDefaults file and the
        // optionValue will be the value for that key from filepath.
        // For deleting, we only need the list of login values from
        // the keys.
        boolean delete = true;
        userInfoFromFile = getUserInfoFromFile(filepath, delete);
        if(userInfoFromFile == null)
            return;
        
        // loop through login names from the keys of userInfoFromFile.
        Iterator keySetItr = userInfoFromFile.keySet().iterator();
        String login = null;
        while(keySetItr.hasNext())
        {
            login = (String)keySetItr.next();
            deleteUserByName(login);
        }
    }
    

    public void deleteUserByName(String strName) {
        VItemArea1 itemArea;
        AppIF appIf = Util.getAppIF();
        if (appIf instanceof VAdminIF) 
            itemArea = (VItemArea1) ((VAdminIF)appIf).getItemArea1();
        else
            return;
        
        itemArea.deleteUserByName(strName);  
    }
    
    public void deleteOperatorsFromFileList(String filepath) {
        // Retrieve info from the file.  We get back a HashMap
        // which has userName (operator) for keys and another level of HashMap
        // for value.  This second level of HashMap has optionKeys
        // for keys and optionValue for values.  The optionKeys will
        // be the option name from the userDefaults file and the
        // optionValue will be the value for that key from filepath.
        // For deleting, we only need the list of operator values from
        // the keys.
        boolean delete = true;
        userInfoFromFile = getUserInfoFromFile(filepath, delete);
        if(userInfoFromFile == null)
            return;
        
        // loop through operator names from the keys of userInfoFromFile.
        Iterator keySetItr = userInfoFromFile.keySet().iterator();
        String operator = null;
        while(keySetItr.hasNext())
        {
            operator = (String)keySetItr.next();
            deleteOperatorByName(operator);
        }
        
        // This clears out the upper right quadrant since it may be
        // incorrect now.
        VAdminIF appIf = (VAdminIF)Util.getAppIF();
        VDetailArea vif = appIf.getDetailArea1();
        if(vif != null)
            vif.fillValues((Object) "", "");

    }
    
    public void deleteOperatorByName(String operator) {
        WUserUtil.deleteOperator(operator);
    }
   
    protected String getItype()
    {
        StringBuffer sbItype = new StringBuffer();

        /*if (m_chkExp.isSelected())
        {
            sbItype.append("~");
            sbItype.append(m_chkExp.getText());
        }
        if (m_chkImg.isSelected())
        {
            sbItype.append("~");
            sbItype.append(m_chkImg.getText());
        }
        if (m_chkWlk.isSelected())
        {
            sbItype.append("~");
            sbItype.append(m_chkWlk.getText());
        }
        if (m_chkAdm.isSelected())
        {
            sbItype.append("~");
            sbItype.append(m_chkAdm.getText());
        }*/

        String strItype = Global.WALKUPIF;
        ButtonModel model = m_rdGroup.getSelection();
        if (model != null)
            strItype = model.getActionCommand();

        return strItype;
    }

    protected void setSelected(boolean bSelect, String strItype)
    {
        Enumeration enumeration = m_rdGroup.getElements();
        while (enumeration.hasMoreElements())
        {
            JRadioButton rdButton = (JRadioButton)enumeration.nextElement();
            if (rdButton.getText().equals(strItype))
            {
                rdButton.setSelected(bSelect);
                break;
            }
        }
    }

    /**
     *  Make a arraylist of all the users, and store the user
     *  with it's interface type in a hash map.
     *  @aListData  arraylist of the data returned from the table, this includes
     *              the list of users for the given interface type.
     *  @aListUsers arraylist of the new users.
     *  @strItype   the interface type string.
     *  @hmUsers    the hash map that stores the interface type string as the value,
     *              and the name of the user as the key.
     */
    protected void addUsersToList(ArrayList aListNames, HashMap hmUsers, String strItype)
    {
        if (aListNames == null || aListNames.isEmpty())
            return;

        for(int i = 0; i < aListNames.size() ; i++)
        {
            String strName = (String)aListNames.get(i);
            hmUsers.put(strName, strItype);
        }
    }

    protected void clearFields()
    {
        m_txfNames.setText("");
        /*m_chkExp.setSelected(false);
        m_chkImg.setSelected(false);
        m_chkWlk.setSelected(false);
        m_chkAdm.setSelected(false);*/
        setSelected(true, Global.WALKUPIF);
        setSelected(false, Global.IMGIF);
    }

}
