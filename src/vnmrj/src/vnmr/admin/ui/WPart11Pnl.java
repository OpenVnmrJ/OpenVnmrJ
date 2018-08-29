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

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.part11.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 *
 *
 *
 */

public class WPart11Pnl extends JPanel
{

    protected vnmr.part11.FileTable     m_tblAudit      = null;
    protected JButton       m_btnFile       = null;
    protected JLabel        m_lblFile       = null;
    protected JFileChooser  m_fcBrowser     = null;
    protected JComponent    m_compParent    = null;
    protected MouseListener m_mlistener     = null;
    protected String        m_strPropName   = null;
    protected JPanel        m_pnlFile       = null;
    protected vnmr.part11.FileMenu      m_objFileMenu   = null;
    protected JComboBox     m_cmbPart11     = null;
    protected String        m_strPart11File = null;

    protected static String[] m_aStrPart11 = {"Audit Trail", "Command History"};

    public WPart11Pnl(JComponent parent)
    {
        m_compParent = parent;
        m_tblAudit = new vnmr.part11.FileTable();
        m_fcBrowser = new JFileChooser(FileUtil.sysdir());
        m_fcBrowser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
        String strArea = getArea();
        m_strPropName = VItemAreaIF.getPropName(WGlobal.PART11, strArea);

        //Create the open button
        m_btnFile = new JButton(Util.getVnmrImageIcon("open.gif"));
        // Create the textfield for the file name
        m_lblFile = new JLabel("");
        m_objFileMenu = new vnmr.part11.FileMenu(FileUtil.sysdir());
        // add it to the panel
        m_pnlFile = new JPanel(new WGridLayout(1, 0));
        m_pnlFile.add(new JLabel("Current Directory:"));
        m_pnlFile.add(m_lblFile);
        //m_pnlFile.add(m_objFileMenu);
        m_pnlFile.add(m_btnFile);
        m_cmbPart11 = new JComboBox(m_aStrPart11);

        // Create the toolbar
        JToolBar tbarPart11 = new JToolBar();
        if (m_compParent instanceof VItemAreaIF)
            tbarPart11.add(m_pnlFile);
        else if (m_compParent instanceof VDetailArea)
            tbarPart11.add(m_cmbPart11);

        setLayout(new BorderLayout());
        add(tbarPart11, BorderLayout.NORTH);
        add(m_tblAudit, BorderLayout.CENTER);

        m_mlistener = new MouseAdapter()
        {
            public void mouseClicked(MouseEvent e)
            {
                int nRow = m_tblAudit.getSelectedRow();
                String strFile = m_tblAudit.getRowValue(nRow);
                System.out.println("The file isdf " + strFile);
                firePropertyChng(m_strPropName, "all", strFile);
            }
        };

        m_btnFile.addActionListener(new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                int returnVal = m_fcBrowser.showOpenDialog(WPart11Pnl.this);

                if (returnVal == JFileChooser.APPROVE_OPTION)
                {
                    File file = m_fcBrowser.getSelectedFile();
                    String strNewPath = file.getAbsolutePath();
                    m_lblFile.setText(strNewPath);
                    showTable(strNewPath, m_tblAudit.getName());
                }
            }
        });

        m_cmbPart11.addActionListener(new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                doPart11Action(e);
            }
        });
    }

    protected void showTable(String strPath, String strType)
    {
        remove(m_tblAudit);
        m_tblAudit = new vnmr.part11.FileTable(strPath, strType);
        m_tblAudit.setName(strType);
        m_tblAudit.addMouseListener(m_mlistener);
        add(m_tblAudit, BorderLayout.CENTER);
    }

    protected void doPart11Action(ActionEvent e)
    {
        String strPropName = WGlobal.PART11 + WGlobal.SEPERATOR + getArea();
        VItemAreaIF.firePropertyChng(strPropName, "all", m_strPart11File);
    }

    protected void firePropertyChng(String strPropName, String oldValue, String newValue)
    {
        if (m_compParent instanceof VItemAreaIF)
        {
            VItemAreaIF.firePropertyChng(strPropName, oldValue, newValue);
        }
    }

    protected String getArea()
    {
        String strArea = "";
        if (m_compParent instanceof VItemAreaIF)
            strArea = ((VItemAreaIF)m_compParent).getArea();
        else if (m_compParent instanceof VDetailArea)
            strArea = ((VDetailArea)m_compParent).getArea();

        return strArea;
    }

}
