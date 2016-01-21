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

import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  unascribed
 *
 */

public class WPart11Notify extends ModelessDialog implements ActionListener
{

    protected JPanel m_pnlDisplay;

    protected GridBagLayout m_gbLayout = new GridBagLayout();

    protected GridBagConstraints m_gbc = new GridBagConstraints();

    protected int m_nRow = 0;

    protected static final String m_strFile = "SYSTEM/adm/p11/notice";


    public WPart11Notify()
    {
        super(null);

        m_pnlDisplay = new JPanel();
        JScrollPane spDisplay = new JScrollPane(m_pnlDisplay);
        m_pnlDisplay.setLayout(m_gbLayout);
        getContentPane().add(spDisplay, BorderLayout.CENTER);

        buttonPane.removeAll();

        // Add the buttons to the panel with space between buttons.
        JButton addButton = new JButton("Add");
        buttonPane.add(addButton);
        buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(closeButton);
        buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(abandonButton);
        buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(helpButton);

        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);
        abandonButton.setActionCommand("cancel");
        abandonButton.addActionListener(this);
        addButton.setActionCommand("add");
        addButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);

        setHelpEnabled(false);
        setCloseEnabled(true);
        setAbandonEnabled(true);

        setTitle("Notify");
        setLocation( 300, 500 );
        setResizable(true);

        int nWidth = buttonPane.getWidth();
        int nHeight = buttonPane.getPreferredSize().height +
                            m_pnlDisplay.getPreferredSize().height;
        setSize(nWidth, 300);
    }

    public void setVisible(boolean bVis, String strhelpfile)
    {
        if (bVis)
        {
            build();
            m_strHelpFile = strhelpfile;
        }
        super.setVisible(bVis);
    }

    public void actionPerformed(ActionEvent e)
    {
        String strCmd = e.getActionCommand();

        if (strCmd == null)
            return;

        if (strCmd.equals("close"))
        {
            saveData();
            setVisible(false);
            dispose();
        }
        else if (strCmd.equals("add"))
        {
            addTxf("");
        }
        else if (strCmd.equals("cancel"))
        {
            setVisible(false);
            dispose();
        }
        else if (strCmd.equals("help"))
            displayHelp();
    }

    protected void build()
    {
        BufferedReader reader = WFileUtil.openReadFile(FileUtil.openPath(m_strFile));
        String strLine;

        if (reader == null)
        {
            Messages.postDebug("File not found " + m_strFile);
            return;
        }

        try
        {
            m_pnlDisplay.removeAll();
            while ((strLine = reader.readLine()) != null)
            {
                addTxf(strLine.trim());
            }
            reader.close();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

    }

    protected void saveData()
    {
        int nCompCount = m_pnlDisplay.getComponentCount();
        String strPath = FileUtil.openPath(m_strFile);
        StringBuffer sbData = new StringBuffer();

        if (strPath == null)
        {
            strPath = FileUtil.savePath(m_strFile);
        }

        for (int i = 0; i < nCompCount; i++)
        {
            Component comp = m_pnlDisplay.getComponent(i);
            if (comp instanceof JTextField)
            {
                String strValue = ((JTextField)comp).getText();
                if (strValue != null && !strValue.trim().equals(""))
                {
                    sbData.append(strValue.trim());
                    sbData.append("\n");
                }
            }
        }

        // write the data to the file
        BufferedWriter writer = WFileUtil.openWriteFile(strPath);
        WFileUtil.writeAndClose(writer, sbData);

    }

    protected void addTxf(String strTxt)
    {
        JPanel pnlName = new JPanel();
        JCheckBox  chk1   = new JCheckBox(Util.getImageIcon("boxGray.gif"));
        JTextField txfName = new JTextField(strTxt);
        pnlName.setLayout(m_gbLayout);
        txfName.setCaretPosition(0);

        m_gbc.anchor = GridBagConstraints.NORTHWEST;
        m_gbc.weightx = 0;
        m_gbc.weighty = 1;
        showComp( m_gbLayout, m_gbc, 0, m_nRow, 1, chk1 );
        m_gbc.weightx = 1;
        m_gbc.fill = GridBagConstraints.HORIZONTAL;
        showComp( m_gbLayout, m_gbc, GridBagConstraints.RELATIVE, m_nRow,
                    GridBagConstraints.REMAINDER, txfName );

        m_nRow++;

        validate();
        repaint();
    }

    private void showComp( GridBagLayout gbl, GridBagConstraints gbc,
                                    int gridx, int gridy, int gridwidth,
                                    JComponent comp)
    {
        gbc.gridx = gridx;    gbc.gridy = gridy;
        gbl.setConstraints( comp, gbc );
        m_pnlDisplay.add( comp );
    }

}

