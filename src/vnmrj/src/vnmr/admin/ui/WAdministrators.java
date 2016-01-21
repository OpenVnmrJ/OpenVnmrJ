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

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class WAdministrators extends ModalDialog implements ActionListener
{

    protected JPanel m_pnlOperators;
    protected JPanel m_pnlDelete;
    protected JPanel m_pnlConfigure;
    protected JScrollPane m_pnlShow;
    protected VTable m_table;
    protected MouseAdapter m_mouseListener;
    protected JTabbedPane m_tabPane;
    protected ArrayList m_aListMsg;
    protected DefaultTableModel m_model;

    protected static Vector m_vecColumns;
    protected static Vector m_vecRows;

    public static String ADMIN = "Administrator;Full Name";

    public WAdministrators(String strhelpfile)
    {
        super("Configure Administrators", strhelpfile);

        m_pnlOperators = new JPanel(new BorderLayout());
        m_pnlShow = new JScrollPane();
        m_tabPane = new JTabbedPane(JTabbedPane.TOP);
        m_tabPane.addTab("Modify", m_pnlShow);
        m_tabPane.addTab("Delete", m_pnlDelete);
        m_aListMsg = new ArrayList();

        cancelButton.addActionListener(this);
        cancelButton.setActionCommand("cancel");
        okButton.addActionListener(this);
        okButton.setActionCommand("ok");
        helpButton.addActionListener(this);
        helpButton.setActionCommand("help");

        Container container = getContentPane();
        container.add(m_pnlOperators);

        setLocationRelativeTo(getParent());
        setSize(400, 300);

        m_mouseListener = new MouseAdapter()
        {
            public void mouseClicked(MouseEvent e)
            {
                if (e.getButton() == MouseEvent.BUTTON3)
                    modifytable("add");
            }

        };

        m_pnlShow.addMouseListener(m_mouseListener);
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (cmd.equals("ok"))
        {
            boolean bOperator = addOperators();
            deleteOperators();
            if (bOperator)
                setVisible(false);
        }
        else if (cmd.equals("cancel"))
        {
            setVisible(false);
            dispose();
        }
        else if (cmd.equals("help"))
            displayHelp();
    }

    public void setVisible(boolean bShow)
    {
        if (bShow)
            dolayout();
        super.setVisible(bShow);
    }

    protected void dolayout()
    {
        if (m_table == null)
            m_table = new VTable();
        setTitle("Modify Administrators");

        setDataVector();
        m_model.setDataVector(m_vecRows, m_vecColumns);
        m_pnlShow.setViewportView(m_table);
        m_pnlOperators.removeAll();

        m_pnlDelete = new JPanel(new GridLayout(0, 1));
        JScrollPane paneDelete = new JScrollPane(m_pnlDelete);

        ArrayList aListOperators = WUserUtil.getAdminList();
        if (aListOperators != null) {
            int nSize = aListOperators.size();
            String strUser = System.getProperty("user.name");
            String strOperator = "";
            for (int i = 0; i < nSize; i++) {
                strOperator = (String) aListOperators.get(i);
                JCheckBox chkOperator = new JCheckBox(strOperator);
                if (!strOperator.equals(strUser))
                    m_pnlDelete.add(chkOperator);
            }
            m_tabPane.setComponentAt(1, paneDelete);
        }
        m_pnlOperators.add(m_tabPane);
    }

    public static void setDataVector()
    {
        String strPath = FileUtil.openPath(WUserUtil.ADMINLIST);
        m_vecColumns = getColumnNames();
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
                if (strLine.startsWith("#"))
                    continue;

                Vector vecRow = new Vector();
                StringTokenizer strTok = new StringTokenizer(strLine, ";\n");
                // add operator info.
                while (strTok.hasMoreTokens())
                {
                    String strValue = strTok.nextToken(";").trim();
                    if (strValue.equals("null"))
                        strValue = "";
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

    }

    protected static Vector getColumnNames()
    {
        Vector vecColumns = new Vector();

        StringTokenizer strTok = new StringTokenizer(ADMIN, ";");
        while (strTok.hasMoreTokens())
        {
            String strColumn = strTok.nextToken();
            vecColumns.add(strColumn);
        }
        return vecColumns;

    }

    protected boolean addOperators()
    {
        String strPath = FileUtil.savePath(WUserUtil.ADMINLIST);
        boolean bOperator = true;
        if (strPath == null || m_table == null)
            return bOperator;
        setCellEditing();
        Vector vecData = m_model.getDataVector();
        StringBuffer sbData = new StringBuffer();
        int nColumns = m_model.getColumnCount();
        sbData.append("# ");
        for (int i = 0; i < nColumns; i++)
        {
            String strColumn = (String)m_model.getColumnName(i);
            sbData.append(strColumn).append(";");
        }
        sbData.append("\n");

        // get current operators
        ArrayList aListOperators = WUserUtil.getAdminList();

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
                }
                if (j != 0)
                    sbData.append(";");

                String strColumnName = m_model.getColumnName(j);
                if (Util.isPart11Sys() && strColumnName.equals("Full Name") &&
                    (strColumn == null || strColumn.trim().equals("")))
                {
                    Messages.postError("Please enter full name for administrator '" +
                                       strOperator + "'");
                    return false;
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

        nLength = m_aListMsg.size();
        for (int i = 0; i < nLength; i++)
        {
            Object[] msg = (Object[])m_aListMsg.get(i);
            if (msg != null && msg[2] != null)
                WUserUtil.writeAuditTrail((Date)msg[0], (String)msg[1], (String)msg[2]);
        }
        return bOperator;

    }

    protected void deleteOperators()
    {
        String strPath = FileUtil.openPath(WUserUtil.ADMINLIST);
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
                bOperator = WUserUtil.deleteOperator(strPath, strOperator);
                if (bOperator)
                {
                    m_pnlDelete.remove(chkOperator);

                    WUserUtil.writeAuditTrail(new Date(), strOperator,
                                          "Deleted administrator: " + strOperator);
                }
                else
                    i = i+1;
            }
            else
                i = i+1;
        }
    }

    protected void setCellEditing()
    {
        TableCellEditor celleditor = m_table.getDefaultEditor(m_table.getColumnClass(0));
        if (celleditor != null && m_table.isEditing())
            celleditor.stopCellEditing();
    }

    protected void modifytable(String cmd)
    {
        if (m_model == null)
            return;

        if (cmd.equals("add"))
            m_model.addRow(new Vector());
        else if (cmd.equals("delete"))
            m_model.removeRow(0);
        else if (cmd.equals("column"))
            m_model.addColumn("New");
    }

    class VTable extends JTable
    {
        public VTable()
        {
            super();
            m_model = new DefaultTableModel();
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
                ArrayList aListOperators = WUserUtil.getAdminList();
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
                        Messages.postInfo("Administrator " + strValue + " already exists.");
                    }
                }
                if (!WUserUtil.isUnixUser((String)value))
                {
                    Messages.postError(value + " cannot be added as an administrator " +
                                      "since it's not a UNIX user");
                    value = "";
                }
                if (!value.equals(""))
                    msg[2] = "Added administrator: "+value;
            }
            else
            {
                String strValue = (String)getValueAt(row, column);
                if (strValue != null && !strValue.equals(value))
                    msg[2] = new StringBuffer().append("Modified Administrator '").append(
                                 getColumnName(column)).append(
                                 "'. (Was '").append(strValue).append(
                                 "' Now '").append(value).append("')").toString();
            }

            m_aListMsg.add(msg);
            super.setValueAt(value, row, column);
        }
    }
}
