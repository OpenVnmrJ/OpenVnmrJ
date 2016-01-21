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
import java.awt.event.*;
import javax.swing.*;
import javax.swing.table.*;

import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * <p>Title: WTrashCan </p>
 * <p>Description: This class has all the methods for deleting/restoring
 *                  items from the administrator interface. </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  unascribed
 *
 */

public class WTrashCan extends ModelessDialog implements ActionListener
{

    protected JPanel m_pnlDisplay = new JPanel();
    protected JToolBar m_tbarTrash = new JToolBar();
    protected String[] m_aStrColNames = {
            vnmr.util.Util.getLabel("_admin_User_Name"), 
            vnmr.util.Util.getLabel("_admin_Home"), 
            vnmr.util.Util.getLabel("_admin_Date_Deleted")
            };
    protected Vector m_vecColNames = new Vector();
    protected Vector m_vecRows = new Vector();
    protected JTable m_table;
    protected DefaultTableModel m_model = new DefaultTableModel();

    public WTrashCan()
    {
        super(vnmr.util.Util.getLabel("_admin_Trash_Can"));

        layoutComps();

        buttonPane.setVisible(false);

        setLocation( 300, 500 );
        setResizable(true);

        int nWidth = (m_tbarTrash.getWidth() < 415) ? 415 : m_tbarTrash.getWidth();
        int nHeight = buttonPane.getPreferredSize().height +
                            m_pnlDisplay.getPreferredSize().height;
        setSize(nWidth, nWidth);

        updateTrashCanIcon();

    }

    public void setVisible(boolean bVisible)
    {
        if (bVisible)
        {
            readFile();
        }
        super.setVisible(bVisible);
    }

    protected void layoutComps()
    {
        for (int i = 0; i < m_aStrColNames.length; i++)
        {
            m_vecColNames.add(m_aStrColNames[i]);
        }

        initToolBar();

        m_table = new JTable(new Vector(), m_vecColNames);
        m_table.setModel(m_model);
        JScrollPane spDisplay = new JScrollPane(m_table);
        addComp(spDisplay, BorderLayout.CENTER);
    }

    protected void initToolBar()
    {
        JButton btnRestore = new JButton(vnmr.util.Util.getLabel("_admin_Restore"));
        JButton btnDelete = new JButton(vnmr.util.Util.getLabel("blDelete"));
        JButton btnEmpty = new JButton(vnmr.util.Util.getLabel("_admin_Empty_Trash"));
        JButton btnExit = new JButton(vnmr.util.Util.getLabel("_admin_Exit"));

        ImageIcon icon = Util.getImageIcon("restore.gif");
        btnRestore.setIcon(icon);
        icon = Util.getImageIcon("delete.gif");
        btnDelete.setIcon(icon);
        icon = Util.getImageIcon("emptytrash.gif");
        btnEmpty.setIcon(icon);
        icon = Util.getImageIcon("exit.gif");
        btnExit.setIcon(icon);

        btnDelete.setToolTipText(vnmr.util.Util.getLabel("_admin_Delete_Users_Fully"));
        btnRestore.setToolTipText(vnmr.util.Util.getLabel("_admin_Restore_Users"));
        btnEmpty.setToolTipText(vnmr.util.Util.getLabel("_admin_Empty_Trash"));
        btnExit.setToolTipText(vnmr.util.Util.getLabel("_admin_Exit"));

        btnDelete.setActionCommand("delete");
        btnDelete.addActionListener(this);
        btnRestore.setActionCommand("restore");
        btnRestore.addActionListener(this);
        btnEmpty.setActionCommand("empty");
        btnEmpty.addActionListener(this);
        btnExit.setActionCommand("exit");
        btnExit.addActionListener(this);

        m_tbarTrash.add(btnDelete);
        m_tbarTrash.add(btnRestore);
        m_tbarTrash.add(btnEmpty);
        m_tbarTrash.add(btnExit);

        addComp(m_tbarTrash, BorderLayout.NORTH);
    }

    public void actionPerformed(ActionEvent e)
    {
        final String cmd = e.getActionCommand();

        if (cmd.equals("exit"))
        {
            setVisible(false);
            dispose();
        }
        else
        {
            new Thread(new Runnable()
            {
                public void run()
                {
                    doAction(cmd);
                }
            }).start();
        }
    }

    protected void readFile()
    {
        String strPath = FileUtil.openPath(WGlobal.TRASHCAN_FILE);
        BufferedReader reader = WFileUtil.openReadFile(strPath);

        if(reader == null)
            return;

        String strLine = null;
        StringTokenizer strTok;
        Vector vecRow;

        try
        {
            m_vecRows.clear();
            while((strLine = reader.readLine()) != null)
            {
                vecRow = new Vector();
                strTok = new StringTokenizer(strLine, "*");

                for(int i = 0; strTok.hasMoreTokens(); i++)
                {
                    String strValue = strTok.nextToken();
                    vecRow.add(strValue);
                }
                m_vecRows.add(vecRow);
            }
            reader.close();
            m_model.setDataVector(m_vecRows, m_vecColNames);
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }

    }

    protected void doAction(String cmd)
    {
        ArrayList aListUsers = new ArrayList();
        boolean bOk = true;
        int[] aIntRows = null;
        int nRow = 0;
        int nIndex = 0;

        while((aIntRows = getSelectedRows(cmd)) != null && aIntRows.length > 0
                    && nIndex < aIntRows.length)
        {
            nRow = aIntRows[nIndex];
            if (nRow < m_model.getRowCount())
            {
                String strUser = (String)m_model.getValueAt(nRow, 0);
                String strHome = (String)m_model.getValueAt(nRow, 1);
                aListUsers.add(strUser);

                if (cmd.equals("delete") || cmd.equals("empty"))
                {
                    Messages.postInfo("Deleting user '" + strUser + "'.");
                    bOk = WUserUtil.deleteUserFully(strUser, strHome);
                }
                else if (cmd.equals("restore"))
                {
                    Messages.postInfo("Restoring user '" + strUser + "'.");
                    bOk = WUserUtil.restoreUser(strUser, strHome);
                }
                if (bOk)
                {
                    m_model.removeRow(nRow);
                }
                else
                {
                    nIndex++;
                    aListUsers.remove(strUser);
                }
            }
        }

        if (cmd.equals("restore") && !aListUsers.isEmpty())
        {
            ArrayList aListResUsers = (ArrayList)aListUsers.clone();
            WUserUtil.addToUserListFile(aListResUsers);
            WUserUtil.showAllUsers(aListResUsers);
            if (bOk && !aListResUsers.isEmpty())
                WUserUtil.updateDB();
        }

        updateTrashCanFile(aListUsers);
        updateTrashCanIcon();
    }

    protected void updateTrashCanFile(ArrayList aListUsers)
    {
        String strPath = FileUtil.openPath(WGlobal.TRASHCAN_FILE);
        BufferedReader reader = WFileUtil.openReadFile(strPath);

        if (reader == null || aListUsers == null)
            return;

        String strLine = "";
        StringBuffer sbData = new StringBuffer();

        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                StringTokenizer strTok = new StringTokenizer(strLine, "*");
                String strUser = strTok.hasMoreTokens() ? strTok.nextToken() : "";

                // if the user is not being deleted from trash can,
                // then keep it in the file.
                if (!aListUsers.contains(strUser))
                {
                    sbData.append(strLine);
                    sbData.append("\n");
                }
            }
            reader.close();
            BufferedWriter writer = WFileUtil.openWriteFile(strPath);
            WFileUtil.writeAndClose(writer, sbData);
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
    }

    protected void updateTrashCanIcon()
    {
        String strPath = FileUtil.openPath(WGlobal.TRASHCAN_FILE);
        BufferedReader reader = WFileUtil.openReadFile(strPath);

        if(reader == null)
        {
            WTrashItem.emptyTrashCanIcon();
            return;
        }

        try
        {
            ArrayList aListUsers = WUtil.strToAList(reader.readLine());
            // if the file is empty, then set the icon as empty.
            if (aListUsers == null || aListUsers.isEmpty())
            {
                WTrashItem.emptyTrashCanIcon();
            }
            // if there is even one line, then set the icon as full.
            else
            {
                WTrashItem.fullTrashCanIcon();
            }
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }

    }

    protected int[] getSelectedRows(String cmd)
    {
        if (cmd.equals("empty"))
            return getAllRows();
        else
            return m_table.getSelectedRows();
    }

    protected int[] getAllRows()
    {
        int nRows = m_model.getRowCount();

        if (nRows == 0)
            return null;

        int[] aIntRows = new int[nRows];

        for (int i = 0; i < nRows; i++)
        {
            aIntRows[i] = i;
        }

        return aIntRows;
    }

    /**
     *  Adds a component to the contentpane.
     *  @comp   component to be added.
     */
    protected void addComp(JComponent comp, String strLayout)
    {
        Container contentPane = getContentPane();
        contentPane.add(comp, strLayout);
    }
}
