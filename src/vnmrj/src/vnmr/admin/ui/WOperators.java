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
import java.awt.*;
import javax.swing.*;

import java.awt.event.*;
import javax.swing.event.*;
import javax.swing.table.*;
import javax.swing.border.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * <p>Title: WOperators </p>
 * <p>Description: Does the delete, configure for operators </p>
 * <p>Copyright: Copyright (c) 2002</p>
 */

public class WOperators extends ModalDialog implements ActionListener
{

    protected JPanel m_pnlOperators;
    protected JPanel m_pnlDelete;
    protected JPanel m_pnlConfigure;
    protected JScrollPane m_pnlShow;
    protected VTable m_table;
    protected String m_strProp;
    protected JTextField m_txfPassword;
    protected JTextField m_txfIcon;
    protected JLabel m_lblPassword;
    protected MouseAdapter m_mouseListener;
    protected PopupMenu m_tableMenu;
    protected JTabbedPane m_tabPane;
    protected boolean m_bTab = false;
    protected ArrayList m_aListMsg;

    protected static String m_strPassword;
    protected static String m_strIcon;
    protected static Vector m_vecColumns;
    protected static Vector m_vecRows;
    protected static long m_timer;

    // If columns are added to the panel, they must also be added to the
    // "_admin_operatorlist_columns".  The strings for each column head
    // ** MUST be identical ** because searches are based on these strings.
    public static final String[] OPERATORS = {
        vnmr.util.Util.getLabel("_admin_Operator"),
        vnmr.util.Util.getLabel("_admin_Users"),
        vnmr.util.Util.getLabel("_admin_Email"),
        vnmr.util.Util.getLabel("_admin_Panel_Level"), 
        vnmr.util.Util.getLabel("_admin_Full_Name"), 
        vnmr.util.Util.getLabel("_admin_Profile_Name")
        };
    public static final String PREFERENCES = "SYSTEM/USRS/operators/preferences";
    public static final String ICON = "loginIcon.gif";

    public WOperators(String strProp)
    {
        super("Configure Operators");
        m_pnlOperators = new JPanel(new BorderLayout());
        m_pnlShow = new JScrollPane();
        m_tabPane = new JTabbedPane(JTabbedPane.TOP);
        m_tabPane.addTab(vnmr.util.Util.getLabel("_admin_Modify_Operators"), m_pnlShow);
        m_tabPane.addTab(vnmr.util.Util.getLabel("_admin_Delete_Operator"), m_pnlDelete);
        m_aListMsg = new ArrayList();

        cancelButton.addActionListener(this);
        cancelButton.setActionCommand("cancel");
        okButton.addActionListener(this);
        okButton.setActionCommand("ok");
        helpButton.addActionListener(this);
        helpButton.setActionCommand("help");

        Container container = getContentPane();
        container.add(m_pnlOperators);

        setSize(800, 400);
        setLocationRelativeTo(getParent());

        m_mouseListener = new MouseAdapter()
        {
            public void mouseClicked(MouseEvent e)
            {
                if (e.getButton() == MouseEvent.BUTTON3)
                    modifytable("add");
            }
        };

        m_pnlShow.addMouseListener(m_mouseListener);

        m_tabPane.addChangeListener(new ChangeListener()
        {
            public void stateChanged(ChangeEvent e)
            {
                int index = m_tabPane.getSelectedIndex();
                m_strProp = (index == 0) ? "add" : "delete";
                if (!m_bTab)
                {
                    setVisible(isShowing(), m_strProp, m_strHelpFile);
                    if (index == 0)
                        m_bTab = true;
                }
                else
                    m_bTab = true;
            }
        });
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        boolean bOperator = true;
        if (cmd.equals("ok"))
        {
            if (m_strProp.indexOf("delete") >= 0 || m_strProp.indexOf("add") >= 0)
            {
                bOperator = addOperators();
		updateUserFullNames();
                deleteOperators();
                if (bOperator)
                    m_bTab = false;
            }
            else if (m_strProp.indexOf("password") >= 0)
                resetPassword();
            else
                writePreferences();
            if (bOperator)
                dispose();
        }
        else if (cmd.equals("cancel"))
        {
            setVisible(false);
            dispose();
        }
        else if (cmd.equals("help"))
            displayHelp();
    }

    public void setVisible(boolean bShow, String strProp, String strhelpfile)
    {
        if (bShow)
        {
        	// Be sure the RightsList has been created, thus
        	// m_vecColumns may be empty here.
        	RightsList rightsList = new RightsList(true);
            // If there were no operators, there will be no columns and
            // continuing will cause exceptions
            if(m_vecColumns == null) {
                Messages.postError("There are no operators, so this panel cannot "
                            + "be used.  Clicking the \n   Save User button for "
                            + "some users or 'Update users' will cause operators\n"
                            + "    to be created for them.  Then you will need to "
                            + "close and reopen \n    the vnmrj adm panel.");
                return;
            
            }
            m_strProp = strProp;
            m_strHelpFile = strhelpfile;
            if (strProp.indexOf("delete") >= 0 || strProp.indexOf("add") >= 0)
            {
                int nIndex = (strProp.indexOf("add") >= 0) ? 0 : 1;
                dolayout();
                dolayoutShow();
                m_tabPane.setSelectedIndex(nIndex);
            }
            else if (strProp.indexOf("password") >= 0)
                dolayoutPassword();
            else
                dolayoutConfigure();
            m_aListMsg.clear();
        }
        super.setVisible(bShow);
    }

    protected void dolayout()
    {
        m_pnlDelete = new JPanel(new GridLayout(0, 1));
        setTitle(vnmr.util.Util.getLabel("_admin_Delete_Operators"));
        JScrollPane paneDelete = new JScrollPane(m_pnlDelete);
        m_pnlOperators.removeAll();

        ArrayList aListOperators = WUserUtil.getOperatorList();
        if (aListOperators != null) {
            int nSize = aListOperators.size();
            String strOperator = "";
            for (int i = 0; i < nSize; i++) {
                strOperator = (String) aListOperators.get(i);
                JCheckBox chkOperator = new JCheckBox(strOperator);
                m_pnlDelete.add(chkOperator);
            }
            //m_pnlOperators.add(paneDelete);
            m_tabPane.setComponentAt(1, paneDelete);
            m_tabPane.setSelectedIndex(1);
            m_pnlOperators.add(m_tabPane);
        }
    }

    protected void dolayoutConfigure()
    {
        GridBagLayout gbLayout = new GridBagLayout();
        GridBagConstraints gbc = new GridBagConstraints(0, 0, 1, 1, 0.2, 0,
                                                        GridBagConstraints.NORTHWEST,
                                                        GridBagConstraints.HORIZONTAL,
                                                        new Insets(0,0,0,0), 0, 0 );
        m_pnlConfigure = new JPanel(gbLayout);
        setTitle(vnmr.util.Util.getLabel("_admin_Preferences"));
        m_pnlOperators.removeAll();

        m_txfPassword = new JTextField(getDefPassword());
        JLabel lblPassword = new JLabel(vnmr.util.Util.getLabel("_admin_Default_Password_for_VnmrJ_Operators:"));
        gbc.gridx = 0;
        gbc.gridy = 0;
        m_pnlConfigure.add(lblPassword, gbc);
        gbc.gridx = 1;
        m_pnlConfigure.add(m_txfPassword, gbc);

        JLabel lblIcon = new JLabel(vnmr.util.Util.getLabel("_admin_Icon_for_Login_Screen:"));
        m_txfIcon = new JTextField(getDefIcon());
        m_txfIcon.setToolTipText("Path of the icon");
        gbc.gridx = 0;
        gbc.gridy = 1;
        m_pnlConfigure.add(lblIcon, gbc);
        gbc.gridx = 1;
        m_pnlConfigure.add(m_txfIcon, gbc);
        m_pnlOperators.add(m_pnlConfigure);
    }

    protected void dolayoutPassword()
    {
        GridBagLayout gbLayout = new GridBagLayout();
        GridBagConstraints gbc = new GridBagConstraints(0, 0, 1, 1, 0.06, 0,
                                                        GridBagConstraints.CENTER,
                                                        GridBagConstraints.HORIZONTAL,
                                                        new Insets(0,0,0,0), 0,0);
        JPanel pnlOperators = new JPanel(gbLayout);
        setTitle(vnmr.util.Util.getLabel("_admin_Reset_Operators'_Password"));
        m_pnlOperators.removeAll();

        m_lblPassword = new JLabel(vnmr.util.Util.getLabel("_admin_VnmrJ_Operators:"));
        pnlOperators.add(m_lblPassword, gbc);
        gbc.gridx = 1;
        m_txfPassword = new JTextField();
        pnlOperators.add(m_txfPassword, gbc);
        m_pnlOperators.add(pnlOperators);
    }

    protected void dolayoutShow()
    {
        if (m_table == null)
            m_table = new VTable();
        setTitle(vnmr.util.Util.getLabel("_admin_Modify_Operators"));

        setDataVector();
        m_table.setDataVector(m_vecRows, m_vecColumns);
        
        // create menu for show command line
        String[] aStrCmdLine = {"yes", "no"};
        JComboBox comboBox = new JComboBox(aStrCmdLine);
        TableCellEditor cellEditor = new DefaultCellEditor(comboBox);
        
        // create menu for profiles from directory listing of userProfiles
        String filepath = FileUtil.openPath("SYSTEM/USRS/userProfiles");
        UNFile file = new UNFile(filepath);
        String[] fileList= file.list();
        if(DebugOutput.isSetFor("profile")) 
            Messages.postDebug("Getting profile list from: " + filepath);
        
        JComboBox profComboBox = new JComboBox();
        // The list cmd got all files in the directory even though we only
        // want the .xml files.  I could have used FilenameFilter, but I
        // also want to strip off the  .xml.  So, just go through the list
        // keep only the .xml and strip off the .xml
        for(int i=0; i < fileList.length; i++) {
            if(DebugOutput.isSetFor("profile"))
                Messages.postDebug("userProfiles file found: " + fileList[i]);
            if(fileList[i].endsWith(".xml")) {
                int index = fileList[i].indexOf(".xml");
                String fullname = fileList[i];
                String name = fullname.substring(0, index);
                if(DebugOutput.isSetFor("profile"))
                    Messages.postDebug("userProfiles file added to menu: " + name);
                profComboBox.addItem(name);
            }
        }
        TableCellEditor profCellEditor = new DefaultCellEditor(profComboBox);
        
//        m_table.getColumnModel().getColumn(4).setCellEditor(cellEditor);
        m_table.getColumnModel().getColumn(4).setCellEditor(profCellEditor);
       m_table.setRowHeight(comboBox.getPreferredSize().height);
        m_pnlShow.setViewportView(m_table);
        m_pnlOperators.removeAll();
        //m_pnlOperators.add(m_pnlShow);
        m_tabPane.setSelectedIndex(0);
        m_pnlOperators.add(m_tabPane);
    }

    public static void setDataVector()
    {
        String strPath = FileUtil.openPath(WUserUtil.OPERATORLIST);
        m_vecColumns = new Vector();
        m_vecRows = new Vector();
        if (strPath == null)
            return;
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        if (reader == null)
            return;

        String strLine;
        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                // Somehow the operatorlist file on one system ended up
                // with lines starting with "null".  Just skip those lines
                // since they cannot be valid.
                if(strLine.startsWith("null"))
                    continue;
                
                // Unfortunately, the StringTokenizer skips fields
                // entirely when two delimiters are together such
                // as ";;"  whereas, a space between them works fine.
                // I will replace all occurrences of ";;" with "; ;"
                // before creating the tokenizer.
                strLine = strLine.replaceAll(";;", "; ;");

                if (strLine.startsWith("\ufeff"))
                    strLine=strLine.substring(1);
                if (strLine.startsWith("#"))
                {
                    m_vecColumns = getColumnNames(strLine);
                    continue;
                }
                Vector vecRow = new Vector();
               // 3/26/10 I needed to move the control of cmdline to
                // being a "right".  That means removing a column of info
                // from this file.  However, we need to be able to deal with
                // the previous versions files.  Thus, check the number of
                // tokens.
                // Oddly enough, the operatorlist file is written such that
                // the first two elements are space separated and the rest are
                // ";" separated.  So, get the number of ";" separated tokens
                // to determine the new versus old file.  Then go back to
                // parsing the way it was always done by getting the first
                // token as space separated and the rest as ";" separated.
                // What a kludge.
                
                // Get tokenizer for purpose of getting number of items
                StringTokenizer strTok = new StringTokenizer(strLine, ";");
                int num=0;
                if (strTok.hasMoreTokens())
                    num = strTok.countTokens();
                
                // Get tokenizer for getting the first item
                strTok = new StringTokenizer(strLine, " ");
                // add operator name (space separated)
                if (strTok.hasMoreTokens())
                    vecRow.add(strTok.nextToken().trim());
 
                // add operator info. ";" separated tokens
                for (int i=0; strTok.hasMoreTokens(); i++)
                {
                    // Get the rest of the items as ";" separated items
                    String strValue = strTok.nextToken(";").trim();
                    if (strValue.equals("null"))
                        strValue = "";
                    // If we have an old operatorlist file, it will have
                    // num=6, a new version will have num=5.  If num == 6
                    // then we need to skip item number 4
                    if(num != 6 || i != 4)
                        vecRow.add(strValue);
                }
                m_vecRows.add(vecRow);
            }
            reader.close();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
        
        // If m_vecColumns did not get filled, fill it now with default system
        // columns.  At least once, an operatorlist file had no column definition
        // header on the file.  This will allow those defective files to work.
        m_vecColumns = getColumnNames("");
    }

    public static Vector getDataVector(boolean bColumn)
    {
        String strPath = FileUtil.openPath(WUserUtil.OPERATORLIST);

        if(strPath == null || strPath.length() == 0)
            return null;

        long timer = new File(strPath).lastModified();
        if (m_timer != timer)
        {
            setDataVector();
            m_timer = timer;
        }
        if (bColumn)
            return m_vecColumns;
        return m_vecRows;
    }

    public static String getDefPassword()
    {
        if (m_strPassword == null)
            setPreferences();
        return m_strPassword;
    }

    public static String getDefIcon()
    {
        if (m_strIcon == null)
            setPreferences();
        return m_strIcon;
    }

    protected static void setPreferences()
    {
        String[] aStrPref = readPreferences();
        m_strPassword = aStrPref[0];
        m_strIcon = aStrPref[1];
    }

    protected void deleteOperators()
    {
        int i = 0;
        boolean bOperator = true;
        if (m_pnlDelete == null)
            return;
        while (i < m_pnlDelete.getComponentCount())
        {
            JCheckBox chkOperator = (JCheckBox)m_pnlDelete.getComponent(i);
            if (chkOperator.isSelected())
            {
                String strOperator = chkOperator.getText();
                bOperator = WUserUtil.deleteOperator(strOperator);
                if (bOperator)
                {
                    m_pnlDelete.remove(chkOperator);
                    deleteOperator(strOperator);
                }
                else
                    i = i+1;
            }
            else
                i = i+1;
        }
    }

    protected void deleteOperator(String strOperator)
    {
        if (m_table == null)
            return;

        Vector vecData = m_table.getDataVector();
        if (vecData == null || vecData.isEmpty())
            return;

        int nLength = vecData.size();
        for (int i = 0; i < nLength; i++)
        {
            Vector vecRow = (Vector)vecData.get(i);
            String strValue = (String)vecRow.get(0);
            if (strOperator.equals(strValue))
            {
                m_table.removeRow(i);
                WUserUtil.writeAuditTrail(new Date(), strOperator,
                                          "Deleted operator: " + strOperator);
                break;
            }
        }
    }

    // called by "Save User" button (doSave) to update OperatorList file in case fullname 
    // of this operator is changed. This is to make sure we don't end up have different 
    // fullname for the same user (operator).
    // Note, user (who has unix account) is also an operator. 
    // But user info and operator info are set by different UI components and 
    // are kept in different files.
    public void updateOperatorFullName(String strOperator, String fullname)
    {
	String strPath = FileUtil.savePath(WUserUtil.OPERATORLIST);
	HashMap hmOperatorsFile = WUserUtil.getOperatorList(strPath);
	if(hmOperatorsFile == null) return;

	// get strOperator from hmOperatorsFile, and replace fullname if different
	if(hmOperatorsFile.containsKey(strOperator)) { 

	   String strOperatorlist = (String)hmOperatorsFile.get(strOperator);
	   StringTokenizer strTok = new StringTokenizer(strOperatorlist, ";\n");
           StringBuffer sbOperatorlist = new StringBuffer();
           int j = 0;
           while (strTok.hasMoreTokens())
           {   // note, hard coded j==3 (4th column) as fullname
               String strValue = strTok.nextToken();
               if (j == 3 && strValue.equals(fullname)) return;
	       else if (j == 3) strValue = fullname; // replace fullname
               sbOperatorlist.append(strValue).append(";");
               j++;
           }
           strOperatorlist = sbOperatorlist.toString();
	   // Replace line for stroperator.
	   hmOperatorsFile.put(strOperator,strOperatorlist);
	   WUserUtil.writeOperatorList(strPath, hmOperatorsFile);
	}
    }

    // called by ok button of "Modify Operators" popup to 
    // update users full name in users/profiles/system/* files
    protected void updateUserFullNames()
    {
        if (m_table == null) return;

        Vector vecData = m_table.getDataVector();
        if (vecData == null || vecData.isEmpty())
            return;

        int nLength = vecData.size();
        for (int i = 0; i < nLength; i++)
        {
            Vector vecRow = (Vector)vecData.get(i);
            int nLength2 = vecRow.size();
            String strUserName = "";
            for (int j = 0; j < nLength2; j++)
            {
                String strColumn = (String)vecRow.get(j);
                if (j == 0)
                {
                    // operator name
                    if (strColumn == null || strColumn.equals("null") ||
                        strColumn.trim().equals("") || !WUserUtil.isUnixUser(strColumn))
                    	strUserName ="";
		    else strUserName = strColumn;
                }
                String strColumnName = m_table.getcolumnName(j);
                if (!strUserName.equals("") && !strColumn.equals("") && 
			strColumnName.equals(vnmr.util.Util.getLabel("_admin_Full_Name")) )
                {
			String strDir = WFileUtil.getHMPath(strUserName);
            		HashMap hmUser = WFileUtil.getHashMap(strDir); 
			String fullname = (String)hmUser.get(WGlobal.FULLNAME);
			if (fullname == null || fullname.trim().equals("") 
				|| !fullname.trim().equals(strColumn))
            		{
				hmUser.put(WGlobal.FULLNAME,strColumn);
                    		WFileUtil.updateItemFile(strDir, hmUser);
			}
		}
	     }
	}
     }

    protected boolean addOperators()
    {
        String strPath = FileUtil.openPath(WUserUtil.OPERATORLIST);
        boolean bOperator = true;
        if (strPath == null || m_table == null)
            return bOperator;
        setCellEditing();
        Vector vecData = m_table.getDataVector();
        StringBuffer sbData = new StringBuffer();
        StringBuffer sbPassword = new StringBuffer();
        int nColumns = m_table.getcolumnCount();
        sbData.append("# ");
        
        for (int i = 0; i < nColumns; i++)
        {
            String strColumn = (String)m_table.getcolumnName(i);
            sbData.append(strColumn).append(";");
        }
        sbData.append("\n");

        // get current operators
        ArrayList aListOperators = WUserUtil.getOperatorList();
        String password = WOperators.getDefPassword();
        try
        {
            password = PasswordService.getInstance().encrypt(password);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }

        // add rows
        int nLength = vecData.size();
        for (int i = 0; i < nLength; i++)
        {
            Vector vecRow = (Vector)vecData.get(i);
            int nLength2 = vecRow.size();
            boolean brow = false;
            String strOperator = "";
            for (int j = 0; j < nLength2; j++)
            {
                String strColumn = (String)vecRow.get(j);
                if (j == 0)
                {
                    // operator name
                    strOperator = strColumn;
                    if (strColumn == null || strColumn.equals("null") ||
                        strColumn.trim().equals(""))
                        break;
                    else if ((aListOperators == null ||
                             !aListOperators.contains(strColumn)) &&
                             !WUserUtil.isUnixUser(strColumn))
                        sbPassword.append(strColumn).append("  ").append(
                            password).append("\n");
                }
                if (j == 1)
                    sbData.append("  ");
                else if (j != 0)
                    sbData.append(";");

                String strColumnName = m_table.getcolumnName(j);
                if (Util.isPart11Sys() && strColumnName.equals(vnmr.util.Util.getLabel("_admin_Full_Name")) &&
                    (strColumn == null || strColumn.trim().equals("")))
                {
                    Messages.postError("Please enter full name for operator '" +
                                       strOperator + "'");
                    return false;
                }

                if (strColumnName.equals(vnmr.util.Util.getLabel("_admin_Panel_Level")) &&
                        !(strColumn == null || strColumn.equals("null") || strColumn.trim().equals("")))
                {
                   try
                   {
                       Integer.parseInt(strColumn);
                   }  catch(NumberFormatException e) {
                      Messages.postError("Please enter an integer Panel Level for operator '" +
                                       strOperator + "'");
                      return false;
                   }
                }

//                if (strColumnName.equals(OPERATORS[5]) && WUserUtil.isVjUser(strOperator))
//                {
//                    if (strColumn != null && !strColumn.trim().equals(""))
//                    {
//                        String strCmdArea = strColumn.substring(0, 1).toUpperCase() +
//                                            strColumn.substring(1);
//                        String strOperatorFile = WFileUtil.getHMPath(strOperator);
//                        HashMap hmUser = WFileUtil.getHashMap(strOperatorFile, true);
//                        String strCmdArea2 = (String)hmUser.get("cmdArea");
//                        if (!strCmdArea.equals(strCmdArea2))
//                        {
//                            hmUser.put("cmdArea", strCmdArea);
//                            WFileUtil.updateItemFile(strOperatorFile, hmUser);
//                        }
//                    }
//                 }

                if (strColumnName.equals(vnmr.util.Util.getLabel("_admin_Profile_Name")) &&
                        (strColumn == null || strColumn.equals("null") || strColumn.trim().equals("")))
                {
                    // If no profile is given, use a default name.
                    // When reading the file, if it does not exist,
                    // everything will be approved, thus this file does
                    // not have to actually exist.
                    strColumn = "AllLiquids";
                }

                if (strColumn == null || strColumn.trim().equals(""))
                    strColumn = "null";
                sbData.append(strColumn.trim());
                brow = true;
            }
            if (brow)
                sbData.append("\n");
        }
        BufferedWriter writer = WFileUtil.openWriteFile(strPath);
        WFileUtil.writeAndClose(writer, sbData);
        WUserUtil.writeOperatorFile(sbPassword);

        nLength = m_aListMsg.size();
        for (int i = 0; i < nLength; i++)
        {
            Object[] msg = (Object[])m_aListMsg.get(i);
            if (msg != null && msg[2] != null)
                WUserUtil.writeAuditTrail((Date)msg[0], (String)msg[1], (String)msg[2]);
        }
        return bOperator;
    }

    protected static Vector getColumnNames(String strLine)
    {
        Vector vecColumns = new Vector();
        
        // If there is a language translation, get it here and
        // use in strLine instead of the one from the actual file.
        if(Util.labelExists("_admin_operatorlist_columns")){
            strLine=Util.getLabel("_admin_operatorlist_columns");
        }
        else{
            if (strLine.startsWith("#"))
                strLine = strLine.substring(1);
        }
        StringTokenizer strTok = new StringTokenizer(strLine, ";");
        while (strTok.hasMoreTokens())
        {
            vecColumns.add(strTok.nextToken().trim());
        }

        // if the default columns are not in the file, then add it.
        int nSize = OPERATORS.length;
        for (int i = 0; i < nSize; i++)
        {
            String strColumn = OPERATORS[i];
            if (!(vecColumns.contains(strColumn)))
                vecColumns.add(strColumn);
        }
        return vecColumns;
    }

    protected void modifytable(String cmd)
    {
        if (cmd.equals("add"))
            m_table.addRow(new Vector());
        else if (cmd.equals("delete"))
            m_table.removeRow(0);
        else if (cmd.equals("column"))
            m_table.addcolumn("New");
    }

    protected void setCellEditing()
    {
        TableCellEditor celleditor = m_table.getDefaultEditor(m_table.getColumnClass(0));
        if (celleditor != null && m_table.isEditing())
            celleditor.stopCellEditing();
    }

    protected void resetPassword()
    {
       String strOperators = m_txfPassword.getText();
       ArrayList aListOperators = WUtil.strToAList(strOperators, false, " ,\t");
       StringBuffer sbData = new StringBuffer();

       String strPath = FileUtil.openPath(WUserUtil.PASSWORD);
       BufferedReader reader = WFileUtil.openReadFile(strPath);
       if (reader == null || aListOperators == null || aListOperators.isEmpty())
       {
           return;
       }

       String password = getDefPassword();
       try
       {
           password = PasswordService.getInstance().encrypt(password);
       }
       catch (Exception e)
       {
           //e.printStackTrace();
           Messages.writeStackTrace(e);
       }

       String strLine;
       try
       {
           while ((strLine = reader.readLine()) != null)
           {
               StringTokenizer strTok = new StringTokenizer(strLine);
               String strOperator = strTok.nextToken();
               if ((strOperator != null) && aListOperators.contains(strOperator))
               {
                   sbData.append(strOperator).append("  ").append(password).append("\n");
                   WUserUtil.writeAuditTrail(new Date(), strOperator, "Reset Password for "+
                                                                       strOperator);
                   aListOperators.remove(aListOperators.indexOf(strOperator) );
               }
               else
                   sbData.append(strLine).append("\n");
           }
           if (! aListOperators.isEmpty())
           {
              ArrayList existingOperators = WUserUtil.getOperatorList();
              while ( ! aListOperators.isEmpty())
              {
                 String name = (String) aListOperators.get(0);
                 if ( (name != null) && existingOperators.contains(name))
                 {
                    sbData.append(name).append("  ").append(password).append("\n");
                    WUserUtil.writeAuditTrail(new Date(), name, "Reset Password for "+
                                                                       name);
                 }
                 aListOperators.remove(0);
              }
           }
           reader.close();
           BufferedWriter writer = WFileUtil.openWriteFile(strPath);
           WFileUtil.writeAndClose(writer, sbData);
       }
       catch (Exception e)
       {
           //e.printStackTrace();
           Messages.writeStackTrace(e);
       }
    }

    protected void writePreferences()
    {
        String strPath = FileUtil.openPath(PREFERENCES);
        if (strPath == null)
        {
            strPath = FileUtil.savePath(PREFERENCES);
            if (strPath == null)
            {
                Messages.postDebug("Can't open file " + PREFERENCES);
                return;
            }
        }

        BufferedWriter writer = WFileUtil.openWriteFile(strPath);
        if (writer == null)
            return;

        String strLine;
        StringBuffer sbPref = new StringBuffer();
        try
        {
            sbPref.append("Password").append("  ").append(m_txfPassword.getText()).append("\n");
            String strIcon = m_txfIcon.getText();
            if (strIcon == null || strIcon.trim().equals(""))
                strIcon = ICON;
            sbPref.append("Icon").append("  ").append(m_txfIcon.getText()).append("\n");
            WFileUtil.writeAndClose(writer, sbPref);
            setPreferences();
            WUserUtil.writeAuditTrail(new Date(), "", "Set operator default password to: " +
                                                       m_txfPassword.getText());
            WUserUtil.writeAuditTrail(new Date(), "", "Set operator icon to: " +
                                                       strIcon);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

    protected static String[] readPreferences()
    {
        String strPath = FileUtil.openPath(PREFERENCES);
        String[] aStrPref = {"", ICON};
        if (strPath == null)
            return aStrPref;

        BufferedReader reader = WFileUtil.openReadFile(strPath);
        if (reader == null)
            return aStrPref;

        String strLine;
        String strPref = "";
        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                strLine = strLine.trim();
                int nIndex = strLine.indexOf(" ");
                if (nIndex > 0 && nIndex+1 < strLine.length())
                {
                    strPref = strLine.substring(0, nIndex);
                    if (strPref.equalsIgnoreCase("password"))
                        aStrPref[0] = strLine.substring(nIndex+1).trim();
                    else if (strPref.equalsIgnoreCase("icon")) {
                        // Catch if they have an old Varian icon in their
                        // preference and replace with the std Agilent Icon
                        String icon = strLine.substring(nIndex+1).trim();
                        if(icon.indexOf("varian") > -1)
                            icon = WOperators.ICON;
                        
                        aStrPref[1] = icon;
                    }
                }
            }
            reader.close();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
        return aStrPref;
    }

    class VTable extends JTable
    {

        protected DefaultTableModel m_model;
        protected DefaultCellEditor m_tableCellEditor;

        public VTable()
        {
            super();
            m_model = new DefaultTableModel();
            m_tableCellEditor = new DefaultCellEditor(new JTextField());
            setModel(new TableSorter(m_model));
            tableHeader.setReorderingAllowed(false);
            addMouseListener(m_mouseListener);
            ((TableSorter)getModel()).addMouseListenerToHeaderInTable(this);
        }

        public boolean isCellEditable(int row, int column)
        {
            String value = (String)getValueAt(row, column);
            if (column == 0 && value != null)
            {
                ArrayList aListOperators = WUserUtil.getOperatorList();
                return (!aListOperators.contains(value));
            }
            return true;
        }

        public void setValueAt(Object value, int row, int column)
        {
            Object[] msg = {new Date(), value, null};
            if (column == 0 && value != null)
            {
                int nLength = getRowCount();
                for (int i = 0; i < nLength; i++)
                {
                    String strValue = (String)getValueAt(i, 0);
                    if (i != row && value.equals(strValue))
                    {
                        value = "";
                        Messages.postInfo("Operator " + strValue + " already exists.");
                    }
                }
                if (!WUserUtil.isOperatorNameok((String)value, true))
                    value = "";
                if (!value.equals(""))
                    msg[2] = "Created operator: "+value;
            }
            else
            {
                String strValue = (String)getValueAt(row, column);
                if (strValue != null && !strValue.equals(value))
                    msg[2] = new StringBuffer().append("Modified Operator '").append(
                             getColumnName(column)).append(
                             "'. (Was '").append(strValue).append(
                             "' Now '").append(value).append("')").toString();
            }

            m_aListMsg.add(msg);
            super.setValueAt(value, row, column);
        }

        public void setDataVector(Vector vecRows, Vector vecColumns)
        {
            m_model.setDataVector(vecRows, vecColumns);
            removeColumn(getColumn(Util.getLabel("_admin_Users")));
        }

        public Vector getDataVector()
        {
            return m_model.getDataVector();
        }

        public void addRow(Vector vecRow)
        {
            m_model.addRow(vecRow);
        }

        public void removeRow(int n)
        {
            m_model.removeRow(n);
        }

        public void addcolumn(String column)
        {
            m_model.addColumn(column);
        }

        public String getcolumnName(int n)
        {
            return m_model.getColumnName(n);
        }

        public int getcolumnCount()
        {
            return m_model.getColumnCount();
        }

    }

}
