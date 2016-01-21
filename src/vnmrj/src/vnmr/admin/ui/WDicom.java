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

public class WDicom extends ModelessDialog implements ActionListener
{

    protected JPanel m_pnlDicom = new JPanel();

    public static String DICOMFILE = FileUtil.SYS_VNMR + "/dicom/conf/dicom_store.cfg";
    protected static WDicom m_dicom;
    public static String HOSTTOOLTIP = "TCP/IP address of the server";
    public static String PORTTOOLTIP = "Port number in which the dicom storage server is listening";
    public static String AETITLETOOLTIP = "Application Entity Title of storage server";
    public static String SCUTITLETOOLTIP = "Application Entity Title of storage client";
    public static String BITSTOOLTIP = "The size of image data (8 or 16)";
    public static String[] m_aStrAETitle = {"AE_TITLE", "SERVER TITLE"};
    public static String[] m_aStrSCUTitle = {"SCU_TITLE", "CLIENT TITLE"};


    public WDicom(String strhelpfile)
    {
        super(vnmr.util.Util.getLabel("_admin_DICOM_Storage_Configuration"));
        m_strHelpFile = strhelpfile;

        initLayout();
        setBackground(Util.getBgColor());
        setSize(400, 140);
        setLocationRelativeTo(getParent());

        Container container = getContentPane();
        container.add(m_pnlDicom, BorderLayout.CENTER);

        historyButton.setVisible(false);
        undoButton.setVisible(false);
        setCloseEnabled(true);
        setAbandonEnabled(true);
        closeButton.addActionListener(this);
        closeButton.setActionCommand("ok");
        abandonButton.addActionListener(this);
        abandonButton.setActionCommand("cancel");
        helpButton.addActionListener(this);
        helpButton.setActionCommand("help");
    }

    public static void showDialog(String helpfile)
    {
        if (m_dicom == null)
            m_dicom = new WDicom(helpfile);

        m_dicom.initLayout();
        m_dicom.setVisible(true);
    }

    protected void initLayout()
    {
        GridBagLayout gbLayout = new GridBagLayout();
        GridBagConstraints gbc = new GridBagConstraints(0, 0, 1, 1, 1, 1,
                                                        GridBagConstraints.NORTHWEST,
                                                        GridBagConstraints.HORIZONTAL,
                                                        new Insets(0,0,0,0), 0, 0);
        m_pnlDicom.removeAll();
        m_pnlDicom.setLayout(gbLayout);
        BufferedReader reader = WFileUtil.openReadFile(UtilB.unixPathToWindows(DICOMFILE));
        if (reader == null)
            return;

        String strLine;
        try
        {
            // Read the config file.  Create the panel item labels using the
            // strings in the config file (eg., HOST and PORT).  Then get the
            // values for these from the config file.
            while ((strLine = reader.readLine()) != null)
            {
                if (strLine.startsWith("#"))
                    continue;
                StringTokenizer strTok = new StringTokenizer(strLine, " :\n");
                String strValue1 = "";
                String strValue2 = "";
                if (strTok.hasMoreTokens())
                    strValue1 = strTok.nextToken().trim();
                if (strTok.hasMoreTokens())
                    strValue2 = strTok.nextToken().trim();
                if (!strValue1.equals(""))
                {
                    gbc.gridx = 0;
                    if (strValue1.equals(m_aStrAETitle[0]))
                        strValue1 = m_aStrAETitle[1];
                    else if (strValue1.equals(m_aStrSCUTitle[0]))
                        strValue1 = m_aStrSCUTitle[1];
                    m_pnlDicom.add(new JLabel(strValue1), gbc);
                    gbc.gridx = 1;
                    m_pnlDicom.add(new JTextField(strValue2), gbc);
                    gbc.gridy = gbc.gridy + 1;
                }
            }
            reader.close();
            setPanelTooltip();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (cmd.equals("ok"))
        {
            writeDicomStorage();
            if (isDicomStorage())
                dispose();
            else
                Messages.postWarning("Could not connect to dicom server." +
                                     "Please make sure the server name " +
                                     "and other parameters are correct.");
        }
        else if (cmd.equals("cancel"))
            dispose();
        else if (cmd.equals("help"))
            displayHelp();
    }

    protected void setPanelTooltip()
    {
        int nLength = m_pnlDicom.getComponentCount();
        String strvalue = "";
        for (int i = 0; i < nLength; i++)
        {
            Component comp = m_pnlDicom.getComponent(i);
            if (comp instanceof JLabel)
            {
                JLabel lbl = ((JLabel)comp);
                strvalue = lbl.getText();
                if (strvalue.equalsIgnoreCase("host"))
                    strvalue = HOSTTOOLTIP;
                else if (strvalue.equalsIgnoreCase("port"))
                    strvalue = PORTTOOLTIP;
                else if (strvalue.equalsIgnoreCase("server title"))
                    strvalue = AETITLETOOLTIP;
                else if (strvalue.equalsIgnoreCase("client title"))
                    strvalue = SCUTITLETOOLTIP;
                else if (strvalue.equalsIgnoreCase("bits"))
                    strvalue = BITSTOOLTIP;
                lbl.setToolTipText(strvalue);
            }
            else if (comp instanceof JTextField)
                ((JTextField)comp).setToolTipText(strvalue);
        }
    }

    public static boolean isDicomStorage()
    {
        boolean bdicom = false;
        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.SYS_VNMR + "/bin/dicom_ping"};

        WMessage msg = WUtil.runScript(cmd, true);
        String strline = msg.getMsg();
        if (strline != null && strline.equals("0"))
            bdicom = true;

        return bdicom;
    }

    protected void writeDicomStorage()
    {
        BufferedReader reader = WFileUtil.openReadFile(UtilB.unixPathToWindows(DICOMFILE));
        StringBuffer sbData = new StringBuffer();
        if (m_pnlDicom == null || reader == null)
            return;

        String strLine;
        try
        {
            // write the comments
            while ((strLine = reader.readLine()) != null)
            {
                if (strLine.startsWith("#"))
                    sbData.append(strLine).append("\n");
            }
            reader.close();

            // write the values
            int nLength = m_pnlDicom.getComponentCount();
            for (int i = 0; i < nLength; i++)
            {
                Component comp = m_pnlDicom.getComponent(i);
                if (comp instanceof JLabel)
                {
                    String strValue = ((JLabel)comp).getText().trim();
                    if (strValue.equals(m_aStrAETitle[1]))
                        strValue = m_aStrAETitle[0];
                    else if (strValue.equals(m_aStrSCUTitle[1]))
                        strValue = m_aStrSCUTitle[0];
                    sbData.append(strValue).append(" : ");
                }
                else if (comp instanceof JTextField)
                {
                    String strValue = ((JTextField)comp).getText();
                    sbData.append(strValue).append("\n");
                }
            }

            BufferedWriter writer = WFileUtil.openWriteFile(UtilB.unixPathToWindows(DICOMFILE));
            WFileUtil.writeAndClose(writer, sbData);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

}
