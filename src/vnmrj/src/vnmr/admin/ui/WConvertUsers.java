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
import java.awt.*;
import javax.swing.*;
import java.util.*;
import java.awt.event.*;
import javax.swing.table.*;
import javax.swing.event.*;

import vnmr.admin.util.*;
import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.util.*;
import vnmr.admin.vobj.*;

/**
 * Title:   WConvertUsers
 * Description: This class provides a tool to convert existing vnmr users
 *              to vnmrj users.
 * Copyright:    Copyright (c) 2002
 */

public class WConvertUsers extends AddRemoveTool
{

    /** Hashmap of checkboxes, the key string is the name of the interface. */
    protected HashMap   m_hmChkBox  = new HashMap();

    /** Array of interface types */
    protected String[]  m_aStrITypes = {
            Global.IMGIF,
            Global.WALKUPIF,
            Global.LCIF
    };
    /** Array of labels corresponding to interface types */
    protected String[]  m_aStrILabels = {
            WGlobal.IMGIFLBL,
            WGlobal.WALKUPIFLBL,
            WGlobal.LCIFLBL
    };
    protected String m_strType;

    /** Table that displays users in all the interface types. */
    protected JTable    m_tblItype  = null;

    /** Table Model used for jtable.  */
    protected ResultsModel m_tableModel;

    protected SessionShare m_sshare;

    public static final String CONVERT = "convert";

    public static final String UPDATE  = "update";


    /**
     *  Constructor
     */
    public WConvertUsers(SessionShare sshare)
    {
        this(sshare, CONVERT);
    }

    /**
     *  Constructor
     */
    public WConvertUsers(SessionShare sshare, String strType)
    {
        super(vnmr.util.Util.getLabel("_admin_Change_vnmr_users_to_vnmrj_users"));

        m_sshare = sshare;
        m_strType = strType;
        initComponents();

        m_btnEast.setToolTipText(vnmr.util.Util.getLabel("_admin_Convert_Users"));
        m_btnWest.setToolTipText(vnmr.util.Util.getLabel("blUndo"));

        m_tblItype.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
        m_tblItype.setColumnSelectionAllowed(true);
        m_tblItype.setRowSelectionAllowed(true);

        m_listLeft.addListSelectionListener(new ListSelectionListener()
        {
            public void valueChanged(ListSelectionEvent e)
            {
                if (timer != null)
                    timer.cancel();

                timer = new java.util.Timer();
                timer.schedule(new TimerTask() {
                    public void run() {WUtil.blink(m_btnEast);}
                }, delay, delay);
            }
        });

        closeButton.setText(vnmr.util.Util.getLabel("_admin_Update_Users"));
        abandonButton.setText(vnmr.util.Util.getLabel("blCancel"));
        
        historyButton.setVisible(false);
        undoButton.setVisible(false);
        setLocationRelativeTo(getParent());
    }

    public void setVisible(boolean bVis, String strhelpfile)
    {
        m_strHelpFile = strhelpfile;
        if (m_tableModel != null && bVis == true)
        {
            m_tableModel.clear();
            updateLeftList();
        }
        super.setVisible(bVis);
    }

    /**
     *  Initializes ui components.
     */
    protected void initComponents()
    {
        JToolBar tbarConvert = initToolBar();
        boolean bConvert = m_strType.equals(CONVERT) ? true : false;
        String[] aStrUpdate = {vnmr.util.Util.getLabel("_admin_Update_Users")};
        String[] aStrColumn = bConvert ? m_aStrILabels : aStrUpdate;

        m_tableModel = new ResultsModel(aStrColumn);
        m_tblItype = new JTable(m_tableModel);
        m_tableModel.addMouseListenerToHeaderInTable(m_tblItype);

        //Create the scroll pane and add the table to it.
        JScrollPane scrollPane = new JScrollPane(m_tblItype);

        m_pnlRight.setLayout(new BorderLayout());
        if (bConvert)
        {
            m_pnlRight.add(tbarConvert, BorderLayout.NORTH);
            setTitle(vnmr.util.Util.getLabel("_admin_Change_vnmr_users_to_vnmrj_users"));
            m_btnEast.setToolTipText(vnmr.util.Util.getLabel("_admin_Convert_Users"));
        }
        else
        {
            setTitle(vnmr.util.Util.getLabel("_admin_Update_VnmrJ_Users"));
            m_btnEast.setToolTipText("Update users");
        }
        m_pnlRight.add(scrollPane, BorderLayout.CENTER);

        m_spDisplay = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true,
                                        m_pnlLeft, m_pnlRight);
        m_spDisplay.setUI(getDividerUI());
        m_spDisplay.setDividerLocation(0.5);
        m_spDisplay.setResizeWeight(0.5);
        getContentPane().add(m_spDisplay);
    }

    /**
     *  Does the layout of the components.
     *  Overrides super.doLayout().
     */
    protected void dolayout()
    {
        JScrollPane scpLeft = new JScrollPane(m_listLeft);

        m_pnlLeft.add(scpLeft);
        //m_aListLeft = WFileUtil.readUserListFile();
        /*m_aListLeft.add("vnmr_abc1");
        m_aListLeft.add("vnmr_abc2");
        m_aListLeft.add("vnmr_abc3");
        m_aListLeft.add("vnmr_abc5");
        m_aListLeft.add("abc1");
        m_aListLeft.add("abc2");*/

        updateLeftList();
        setLocation( 300, 500 );

        /*int nWidth = buttonPane.getWidth();
        int nHeight = buttonPane.getPreferredSize().height +
                        m_spDisplay.getPreferredSize().height + 100;*/
        setSize(600, 300);
    }

    /**
     *  Intializes toolbar for this dialog and returns it.
     */
    protected JToolBar initToolBar()
    {
        String strName = null;
        String strLabel = null;
        //JCheckBox chkInterface = null;
        JRadioButton rdInterface = null;
        ButtonGroup group = new ButtonGroup();

//        JLabel lblConvert = new JLabel(vnmr.util.Util.getLabel("_admin_Select_Interface"));
// 6/4/09 the getLabel is not working.  for now, just use the english string
        JLabel lblConvert = new JLabel("Select Interface");

        JToolBar tbarCnvrt = new JToolBar();
        tbarCnvrt.setLayout(new GridLayout(0, 3));
        tbarCnvrt.add(lblConvert);
        tbarCnvrt.setFloatable(false);

        // for each interface type, create a check box
        // and add it to the hashmap.
        int nSize = m_aStrITypes.length;
        for (int i = 0; i < nSize; i++)
        {
            strName = m_aStrITypes[i];
            strLabel=m_aStrILabels[i];
            //chkInterface = new JCheckBox(strName);
            rdInterface = new JRadioButton(strLabel);
            String strAdmItype = WUtil.getAdminItype();
            boolean bSelected = (strAdmItype != null && strAdmItype.equalsIgnoreCase(strName))
                                    ? true : false;
            rdInterface.setSelected(bSelected);
            tbarCnvrt.add(rdInterface);
            group.add(rdInterface);
            m_hmChkBox.put(strLabel, rdInterface);
            if ((i+1)%2 == 0 && i+1 < nSize)
               tbarCnvrt.add(new JLabel("")); 
        }

        return tbarCnvrt;
    }

    protected void updateLeftList()
    {
        if (m_tableModel == null)
            return;

        // Get the current vj users
        ArrayList aListVjUsers = WUserUtil.readUserListFile();

        if (m_strType.equals(CONVERT))
            getVnmrUsers(aListVjUsers);
        else
        {
            m_aListLeft.clear();
            m_aListLeft = (ArrayList)aListVjUsers.clone();
        }
        
        Collections.sort(m_aListLeft);
        m_listLeft.setListData(m_aListLeft.toArray());
    }

    protected void getVnmrUsers(ArrayList aListVjUsers)
    {       
        ArrayList aListUnixUser = WUserUtil.getUnixUsers();
        m_aListLeft.clear();
        if (aListUnixUser == null)
            return;
        
        int nLength = aListUnixUser.size();
        for (int i = 0; i < nLength; i++)
        {
            String strUser = (String)aListUnixUser.get(i);
            if (strUser != null && !strUser.equals("") &&
                aListVjUsers != null && !aListVjUsers.contains(strUser))
                m_aListLeft.add(strUser.trim());
        }
    }

    /**
     *  Updates the data in the table.
     */
    protected void updateListData(ActionEvent e)
    {
        if (e.getActionCommand().equals("east"))
        {
            // Get the list of the selected values to be added to the table.
            Object[] aItems = m_listLeft.getSelectedValues();
            if (aItems == null)
                return;

            if (m_strType.equals(CONVERT))
            {
                // check to see which interface type is selected,
                // and then add the users to the table.
                for (int i = 0; i < m_aStrILabels.length; i++) {
                    String strColName = m_aStrILabels[i];
                    //JCheckBox chkBox = (JCheckBox)m_hmChkBox.get(strColName);
                    JRadioButton rdBox = (JRadioButton) m_hmChkBox.get(
                        strColName);
                    if (rdBox.isSelected()) {
                        m_tableModel.update(aItems, strColName);
                    }
                }
            }
            else
            {
                String strColName = m_tableModel.getColumnName(0);
                m_tableModel.update(aItems, strColName);
            }

            // remove the items from the left
            for (int i = 0; i < aItems.length; i++)
            {
                String strValue = (String)aItems[i];
                m_aListLeft.remove(strValue);
            }
        }
        else if (e.getActionCommand().equals("west"))
        {
            int[] aRows = m_tblItype.getSelectedRows();
            int[] aCols = m_tblItype.getSelectedColumns();
            int index = 0;
            boolean bOk = true;
            ArrayList aListValue = new ArrayList();

            for(int i = 0; i < aCols.length; i++)
            {
                int nCol = aCols[i];
                for (int j = 0; j < aRows.length; j++)
                {
                    int nRow = aRows[j];
                    String strValue = (String)m_tblItype.getValueAt(nRow, nCol);
                    strValue = (strValue != null) ? strValue.trim() : strValue;
                    aListValue.add(strValue);
                }
                m_tableModel.delete(nCol, aListValue);
            }
        }
        m_listLeft.setListData(m_aListLeft.toArray());
    }

    /**
     * Called when the user presses 'OK' on the popup.
     * All the data in the table is then saved.
     */
    protected void saveData()
    {
        if (m_strType.equals(CONVERT))
            convertUsers();
        else
            updateUsers();
    }

    protected void convertUsers()
    {
        final ArrayList aListUsers = new ArrayList();
        final HashMap hmUsers = new HashMap();

        // for each interface type, get the list of the users,
        // and add it to the vnmrj user directories
        for (int i = 0; i < m_aStrITypes.length; i++)
        {
            ArrayList aListData = m_tableModel.getColumnData(i);
            if (aListData != null)
            {
                // make a arraylist of all the users,
                // and store the user with it's interface type in a hash map.
                addUsersToList(aListData, aListUsers, m_aStrITypes[i], hmUsers);
            }
        }

        new Thread(new Runnable()
        {
            public void run()
            {
                saveData(aListUsers, hmUsers, true, null);
            }
        }).start();

    }

    protected void updateUsers()
    {

        new Thread(new Runnable()
        {
            public void run()
            {
                ArrayList aListData = m_tableModel.getColumnData(0);
                if (aListData == null)
                    return;

                for (int i = 0; i < aListData.size(); i++)
                {
                    String strName = (String)aListData.get(i);
                    Messages.postInfo("Running makeuser for " + strName);
                    WUserUtil.updateUser(strName);
                    Messages.postInfo(strName + " updated");
                }
                Messages.postInfo("Update User Completed All Requested Users");
            }
        }).start();


    }

    protected void saveData(ArrayList aListUsers, HashMap hmUsers, boolean bConvert,
                            HashMap userInfoFromFile)
    {
         // now traverse through the user list, and save each user
        aListUsers = saveUsers(aListUsers, hmUsers, bConvert, userInfoFromFile);
        if (aListUsers != null && !aListUsers.isEmpty())
        {
            WUserUtil.addToUserListFile(aListUsers);
            WUserUtil.showAllUsers(aListUsers);
        }
    }

    /**
     *  Gets the list of the users and the hash map that contains the interface
     *  type string for each user, and calls to write a vj user file for each user.
     *  @param aListUsers   list of all the new vj users.
     *  @param hmUsers      hashmap that contains the name of the user as the key,
     *                      and the interface string as the value
     */
    protected ArrayList saveUsers(ArrayList aListUsers, HashMap hmUsers, 
            boolean bConvert, HashMap userInfoFromFile)
    {
        HashMap userInfo=null;
        
        ArrayList aListUserAdded = (ArrayList)aListUsers.clone();
        if (aListUsers == null || aListUsers.isEmpty()
                || hmUsers == null || hmUsers.isEmpty())
            return null;

        HashMap hmUserDef = getUserDefaults();
        Messages.postInfo("Creating VJ users...");

        for (int i = 0; i < aListUsers.size(); i++)
        {
            String strName = (String)aListUsers.get(i);
            String strItype = (String)hmUsers.get(strName);
            // Get the HashMap for this user if there is one.  It will 
            // contain option keywords as keys and option values as values.
            if(userInfoFromFile != null)
                userInfo = (HashMap)userInfoFromFile.get(strName);
            VItemArea1.firePropertyChng(WGlobal.USER_CHANGE, "all", strName);

            // If the user is already a Unix user, use his existing home dir
            // when setUserHome() sets the value in the HashMap passed in,
            // it sets key=name and home=homedir.  userInfo has login and home
            // as the keys.  So, pass in an empth HashMap to get the correct
            // value for home and put that into userInfo.
            if (WUserUtil.isUnixUser(strName)) {
                HashMap hmHome = new HashMap();
                WUserUtil.setUserHome(strName, hmHome);
                String homedir = (String)hmHome.get("home");
                if(homedir != null && homedir.length()>0)
                    if(userInfo == null)
                        userInfo = new HashMap();
                    userInfo.put("home", homedir);
            }

            boolean bOk = WUserUtil.writeNewUserFile(strName, strItype,
                                                     hmUserDef, bConvert, null,
                                                     userInfo);

            if (!bOk)
            {
                Messages.postError("Error creating user " + strName);
                aListUserAdded.remove(strName);
            }
        }
        if (!aListUserAdded.isEmpty())
            WUserUtil.updateDB();
        return aListUserAdded;
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
    protected void addUsersToList(ArrayList aListData, ArrayList aListUsers,
                                    String strItype, HashMap hmUsers)
    {
        if (aListData == null || aListData.isEmpty() || aListUsers == null)
            return;

        for(int i = 0; i < aListData.size() ; i++)
        {
            String strName = (String)aListData.get(i);
            if (!aListUsers.contains(strName))
                aListUsers.add(strName);
            String strValue = (String)hmUsers.get(strName);
            if (strValue == null)
                strValue = "";
            //hmUsers.put(strName, strValue+"~"+strItype);
            hmUsers.put(strName, strItype);
        }
    }

    /**
     *  Gets the user defaults for the system.
     */
    protected HashMap getUserDefaults()
    {
        AppIF appIf = Util.getAppIF();
        HashMap hmDef = null;

        if (appIf instanceof VAdminIF)
        {
            hmDef = ((VAdminIF)appIf).getUserDefaults();
        }
        return hmDef;
    }

    /**
     *  The table model class which is extended from abstract table model class.
     *  This models uses a hashmap and stores the information for each of the columns
     *  as an arraylist, and uses the name of the column to look up and store info.
     *  e.g. the list of users in the 'Experimental' column is stored as a arraylist,
     *  and the name of this column is used as the key for the hashmap.
     */
    public class ResultsModel extends AbstractTableModel
    {
        HashMap m_hmRows = new HashMap();
        String[] m_aStrColNames = null;
        boolean m_bAscending = true;

        /**
         *  Constructs a default <code>DefaultTableModel</code>
         *  which is a table of zero columns and zero rows.
         */
        public ResultsModel(String[] aStrColumn)
        {
            m_aStrColNames = aStrColumn;
        }

        public String getColumnName(int column)
        {
            if (column >= m_aStrColNames.length)
                return "";

            String strName = m_aStrColNames[column];
            if (strName == null)
                strName = "";

            return strName;
        }

        public boolean isCellEditable(int row, int column)
        {
            return false;
        }

        public void clear()
        {
            m_hmRows.clear();
        }

        public int getColumnCount()
        {
            return m_aStrColNames.length;
        }

        public int getRowCount()
        {
            int nCount = 0;
            ArrayList aListColumn;

            for (int i = 0; i < 4; i++)
            {
                aListColumn = getColumnData(i);
                if (aListColumn != null && aListColumn.size() > nCount)
                    nCount = aListColumn.size();
            }
            return nCount;
        }

        public ArrayList getColumnData(int column)
        {
            String strColumn = getColumnName(column);
            return (ArrayList)m_hmRows.get(strColumn);
        }

        public Object getValueAt(int row, int column)
        {
            Object objValue = null;
            String strColumn = getColumnName(column);
            ArrayList tmpColumn = (ArrayList)m_hmRows.get(strColumn);

            if (tmpColumn != null && tmpColumn.size() > row)
                objValue = tmpColumn.get(row);
            return objValue;
        }

        public void setValueAt(Object objValue, int row, int column)
        {
            String strColumn = getColumnName(column);
            ArrayList aListColumn = (ArrayList)m_hmRows.get(strColumn);
            if (aListColumn != null)
                aListColumn.set(row, objValue);

            // generate notification
            fireTableChanged(new TableModelEvent(this, row, row, column));
        }

        public  void update(Object[] objItems, String strColName)
        {
            String strItem = null;

            if (objItems == null)
                return;

            ArrayList aListRows = (ArrayList)m_hmRows.get(strColName);
            if (aListRows == null)
                aListRows = new ArrayList();

            try
            {
                for (int i = 0; i < objItems.length; i++)
                {
                    strItem = (String)objItems[i];
                    if (!aListRows.contains(strItem))
                        aListRows.add(strItem);
                }
                m_hmRows.put(strColName, aListRows);
                // generate a notification.
                fireTableChanged(new TableModelEvent(this, 0, getRowCount()-1,
                             TableModelEvent.ALL_COLUMNS, TableModelEvent.INSERT));
            }
            catch (Exception e)
            {
                Messages.writeStackTrace(e);
                Messages.postDebug(e.toString());
            }
        }

        public boolean delete(int column, ArrayList aListValue)
        {
            String strColName = getColumnName(column);
            ArrayList aListColumn = (ArrayList)m_hmRows.get(strColName);
            boolean bOk = true;

            ArrayList aListRows = (ArrayList)m_hmRows.get(strColName);
            if (aListRows == null)
                aListRows = new ArrayList();

            for (int i = 0; i < aListValue.size(); i++)
            {
                String strValue = (String)aListValue.get(i);
                bOk = aListRows.remove(strValue);
                if (bOk)
                    m_aListLeft.add(strValue);
            }

            m_hmRows.put(strColName, aListRows);
            // generate a notification.
            fireTableChanged(new TableModelEvent(this, 0, getRowCount(),
                                TableModelEvent.ALL_COLUMNS, TableModelEvent.DELETE));
            return bOk;
        }

        // There is no-where else to put this.
        // Add a mouse listener to the Table to trigger a table sort
        // when a column heading is clicked in the JTable.
        public void addMouseListenerToHeaderInTable(JTable table)
        {
            final JTable tableView = table;
            tableView.setColumnSelectionAllowed(false);
            MouseAdapter listMouseListener = new MouseAdapter()
            {
                public void mouseClicked(MouseEvent e)
                {
                    TableColumnModel columnModel = tableView.getColumnModel();
                    int viewColumn = columnModel.getColumnIndexAtX(e.getX());
                    int column = tableView.convertColumnIndexToModel(viewColumn);
                    ArrayList aListData = m_tableModel.getColumnData(column);
                    if (e.getClickCount() == 1 && column != -1 && aListData != null)
                    {
                        Collections.sort(aListData);
                        if (m_bAscending)
                            Collections.reverse(aListData);
                        m_bAscending = !(m_bAscending);
                         // generate a notification.
                        fireTableStructureChanged();
                    }
                }
            };
            JTableHeader th = tableView.getTableHeader();
            th.addMouseListener(listMouseListener);
        }
    }

}
