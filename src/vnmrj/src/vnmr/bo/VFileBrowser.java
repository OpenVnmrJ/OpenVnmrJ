/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import javax.swing.plaf.basic.*;

import vnmr.ui.*;
import vnmr.util.*;


/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class VFileBrowser extends VGroup implements ExpListenerIF, ActionListener
{

    protected VEntry m_objEntry;
    protected JFileChooser m_fileBrowser;
    protected VButton m_btnfileBrowser;
    protected String m_strDefaultLookandfeel;

    public VFileBrowser(SessionShare sshare, ButtonIF vif, String typ)
    {
        super(sshare, vif, typ);
        m_strDefaultLookandfeel = UIManager.getLookAndFeel().getID();

        //m_objEntry = new JTextField();
        m_fileBrowser = new JFileChooser(FileUtil.openPath("USER"));
        m_fileBrowser.setDialogTitle("Select file");
        m_fileBrowser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
        m_fileBrowser.setPreferredSize(new Dimension(500, 500));
    }

    public Component add(Component comp)
    {
        Component component = super.add(comp);
        if (component instanceof VButton)
        {
            m_btnfileBrowser = (VButton)component;
            m_btnfileBrowser.setActionCommand("show");
            m_btnfileBrowser.addActionListener(this);
        }
        else if (component instanceof VEntry)
        {
            m_objEntry = (VEntry) component;
        }
        return component;
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (cmd.equals("show"))
        {
            setLookandfeel("metal");
            // SwingUtilities.updateComponentTreeUI(m_fileBrowser);
            int nValue = m_fileBrowser.showDialog(this, "OK");
            if (nValue == JFileChooser.APPROVE_OPTION)
            {
                File file = m_fileBrowser.getSelectedFile();
                String strPath = file.getAbsolutePath();
                if (strPath.length() < 1)
                    return;
                if (Util.iswindows())
                    strPath = UtilB.escapeBackSlashes(strPath);
                // set the file name in the entry
                File f = new File(strPath);
                if (f.isDirectory())
                    strPath = new StringBuffer().append(strPath).append(File.separator).toString();
                m_objEntry.setAttribute(VALUE, strPath);
                m_objEntry.sendVnmrCmd();
                setLookandfeel(m_strDefaultLookandfeel);
            }
            else if (nValue == JFileChooser.CANCEL_OPTION)
            {
                setLookandfeel(m_strDefaultLookandfeel);
            }
        }
    }

    protected void setLookandfeel(String lookandfeel)
    {
       /***********
        try
        {
            // lookandfeel can be motif or metal
            String lf = UIManager.getSystemLookAndFeelClassName();
            UIManager.LookAndFeelInfo[] lists = UIManager.getInstalledLookAndFeels();
            if (lookandfeel != null) {
                lookandfeel = lookandfeel.toLowerCase();
                int nLength = lists.length;
                String look;
                for (int k = 0; k < nLength; k++) {
                    look = lists[k].getName().toLowerCase();
                    if (look.indexOf(lookandfeel) >= 0) {
                        lf = lists[k].getClassName();
                        break;
                    }
                }
            }
            UIManager.setLookAndFeel(lf);
        }
        catch (Exception e)
        {
            Messages.postDebug(e.toString());
        }
        *****************/
    }

}
