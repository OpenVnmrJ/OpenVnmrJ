/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.util;

import java.io.*;
import java.awt.*;
import java.beans.*;
import javax.swing.*;

import vnmr.admin.ui.*;
import vnmr.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  unascribed
 *
 */

public class WDirTabPane extends JTabbedPane implements PropertyChangeListener
{

    protected JPanel m_pnlDataDir;
    protected JPanel m_pnlTemplates;
    protected JPanel m_pnlP11Dir;
    protected int m_nFDAMode;
    protected boolean m_bPart11Sys = false;
    protected UserDirectoryDialog m_userDirDlg;

    protected final static String DATADIR       = "SYSTEM/PROFILES/data/";
    protected final static String TEMPLATEDIR   = "SYSTEM/PROFILES/templates/";
    protected final static String P11DIR        = "SYSTEM/PROFILES/p11/";

    protected final static String DATADEFDIRFILE    = "SYSTEM/INTERFACE/dataDirDefault";
    protected final static String TEMPLATEDEFFILE  = "SYSTEM/PROPERTIES/filename_templates";

    protected final static String DATADIR_TITLE     = vnmr.util.Util.getLabel("_admin_Data_Directories",
                                                                              "Data Directories");
    protected final static String TEMPLATE_TITLE    =  vnmr.util.Util.getLabel("_admin_User_Templates",
                                                                              "User Templates");
    protected final static String PART11_TITLE      = "FDA Directories";

    public final static String  IMGTEMPLATE = "SYSTEM/imaging/"+
                                             FileUtil.getSymbolicPath("PROPERTIES")+
                                             "/filename_templates";
    public final static String WALKUPTEMPLATE = "SYSTEM/"+
                                                FileUtil.getSymbolicPath("PROPERTIES")+
                                                "/filename_templates";

    public WDirTabPane()
    {
        super();

        m_userDirDlg = new UserDirectoryDialog(UserDirectoryDialog.INITALIZE);
        m_bPart11Sys = Util.isPart11Sys();

        m_pnlDataDir     = m_userDirDlg.makeDataDirPanel(DATADIR, DATADEFDIRFILE, false);
        m_pnlTemplates   = m_userDirDlg.makeUserTemplate(TEMPLATEDIR, TEMPLATEDEFFILE);
        if (m_bPart11Sys)
        {
            m_pnlP11Dir  = m_userDirDlg.makeDataDirPanel(P11DIR, "SYSTEM/PART11/part11Config", true);
            VItemAreaIF.addChangeListener(Global.PART11_CONFIG_FILE, this);
        }
        addTab(DATADIR_TITLE, m_pnlDataDir);
        //addTab(PART11_TITLE, m_pnlP11Dir);
        addTab(TEMPLATE_TITLE, m_pnlTemplates);
        setP11Settings();
        WDirChooser.addChangeListener(WGlobal.INFODIR, this);
    }

    public void propertyChange(PropertyChangeEvent e)
    {
        Component comp = getSelectedComponent();
        String strPropName = e.getPropertyName();
        String strValue = (String)e.getNewValue();

        if (strPropName.equals(WGlobal.INFODIR))
        {
            if (comp instanceof JPanel)
            {
                setDir((JPanel)comp, strValue);
            }
        }
        else if (m_bPart11Sys && strPropName.equals(Global.PART11_CONFIG_FILE))
        {
            setP11Settings();
        }
    }

    protected void setP11Settings()
    {
        if (m_bPart11Sys && m_pnlDataDir != null && m_pnlP11Dir != null)
        {
            m_nFDAMode = Util.getPart11Mode();
            int nTab = 0;

            if (m_nFDAMode == Util.NONFDA)
            {
                nTab = indexOfTab(PART11_TITLE);
                if (nTab >= 0)
                    removeTabAt(nTab);
                nTab = indexOfTab(DATADIR_TITLE);
                if (nTab < 0)
                   insertTab(DATADIR_TITLE, null, m_pnlDataDir, null, 0);

                // rename the part11 dir
               /*renameToBak(P11DIR);
               renameToOrig(DATADIR);*/
            }
	    else
            {
                nTab = this.indexOfTab(DATADIR_TITLE);
                if (nTab < 0)
                    insertTab(DATADIR_TITLE, null, m_pnlDataDir, null, 0);
                nTab = this.indexOfTab(PART11_TITLE);
                if (nTab < 0)
                    insertTab(PART11_TITLE, null, m_pnlP11Dir, null, 1);

                // rename the ".bak" to original dirs
                /*renameToOrig(P11DIR);
                renameToOrig(DATADIR);*/
            }

            validate();
            repaint();
            Container container = getParent();
            if (container != null)
                container.repaint();
        }

    }

    protected void renameToBak(String strDir)
    {
        String strPath = FileUtil.openPath(strDir);
        if (strPath != null)
        {
            int nIndex = strPath.lastIndexOf(File.separator);
            if (nIndex == strPath.length() - 1)
                strPath = strPath.substring(0, nIndex);
            WUtil.renameFile(strPath, strPath+".bak");
        }
    }

    protected void renameToOrig(String strDir)
    {
        int nIndex = strDir.lastIndexOf(File.separator);
        if (nIndex == strDir.length() - 1)
            strDir = strDir.substring(0, nIndex);
        String strPath = FileUtil.openPath(strDir+".bak");
        if (strPath != null)
        {
            File bakFile = new File(strPath);
            nIndex = strPath.indexOf(".bak");
            strPath = strPath.substring(0, nIndex);
            File origFile = new File(strPath);
            bakFile.renameTo(origFile);
        }
    }

    protected void setDir(JComponent pnlComp, String strValue)
    {
        int nCompCount = pnlComp.getComponentCount();

        for (int i = 0; i < nCompCount; i++)
        {
            Component compChild = pnlComp.getComponent(i);
            if (compChild instanceof JTextField)
            {
                boolean bSet = m_userDirDlg.setDir((JTextField)compChild, strValue);
                if (bSet)
                    break;
            }
            else if (compChild instanceof JComponent
                        && ((JComponent)compChild).getComponentCount() > 0)
            {
                setDir((JComponent)compChild, strValue);
            }
        }
    }

}
