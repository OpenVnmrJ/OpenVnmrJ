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
import java.beans.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.vobj.*;
import vnmr.admin.util.*;
import vnmr.print.*;

/**
 * Title:   WDialogHandler
 * Description: This class handles the visibility of the modal and
 *              the modaless dialogs for the Wanda interface.
 * Copyright:    Copyright (c) 2002
 */

public class WDialogHandler implements PropertyChangeListener
{
    /** Session share object */
    protected SessionShare m_sshare;

    /** The application interface.   */
    protected AppIF m_appIf;

    /** Help dialog. */
    protected VnmrJHelp m_objVnmrjHelp          = null;

    /** Command Area.  */
    //protected WCommandArea m_objCmdArea         = null;

    /** Part11 dialog.   */
    protected WPart11Dialog m_objPart11Dlg      = null;

    protected WPart11Notify m_objPart11Notify   = null;

    /** Fonts and colors dialog.  */
    protected WFontColors m_objFontColors       = null;

    /** Dialog to convert vnmr users to vnmrj users.   */
    protected WConvertUsers m_objConvertDlg     = null;

    protected WConvertUsers m_objConvertDlg2    = null;

    protected WOperators m_objOperators         = null;

    protected WAutomation m_objAutomation       = null;

    protected WInvestigator m_objInvestigator   = null;

    protected WAdministrators m_objAdministrator = null;

    /** Dialog for configuring users.   */
    protected WUsersConfig  m_objUsersConfig    = null;

    protected DirViewDialog m_objDirDlg         = null;

    protected DirViewDialog m_objDfTable        = null;

    protected UserRightsPanel m_objProfileEditor = null;
    
    protected WLocatorConfig m_objLocator            = null;
    
    protected WSharedConsoleDialog m_objShared = null;

    private static LogToCSVPanel logToCSVPanel = null;


    public WDialogHandler(SessionShare sshare, AppIF ap, ExpViewArea expViewArea)
    {
        m_sshare = sshare;
        m_appIf = ap;

        initComps();
        WSubMenuItem.addChangeListener(this);
    }

    protected void initComps()
    {
        m_objFontColors = new WFontColors();
        m_objUsersConfig = new WUsersConfig(m_appIf);
    }

    public void updateOperatorFullName(String loginName, String fullName)
    {
        if (m_objOperators == null)
            m_objOperators = new WOperators("add operators");
	m_objOperators.updateOperatorFullName(loginName, fullName);
    }

    /**
     *  Checks the name of the property and opens the respective dialog.
     */
    public void propertyChange(PropertyChangeEvent e)
    {
        String strPropName = e.getPropertyName().toLowerCase();
        // If this is to display a help file, get strPropName again but
        // this time do not convert to lower case, the filename needs to
        // be case sensitive.
        if (strPropName.indexOf("help") >= 0) {
            strPropName = e.getPropertyName();
        }

        String strhelpfile = "";
        if (strPropName == null)
            strPropName = "";

        String strprop = strPropName;
        StringTokenizer strTok = new StringTokenizer(strprop, "'\n");
        if (strTok.hasMoreTokens())
            strPropName = strTok.nextToken();
        if (strTok.hasMoreTokens())
        {
            strhelpfile = strTok.nextToken();
            if (strhelpfile.trim().equals("") && strTok.hasMoreTokens())
                strhelpfile = strTok.nextToken();
        }
        if (strhelpfile.startsWith("help:"))
            strhelpfile = strhelpfile.substring(5);

        // All Help menu item here
        if (strPropName.indexOf("help") >= 0)
        {
            if (strPropName.indexOf("about") >= 0)
            {
                // Call the getversion macro to write the file /vnmr/tmp/version
                getVersion();
                
                ExpPanel exp=Util.getDefaultExp();
                String cmd = "popup mode:modal file:About.xml cancel:hide title:About VnmrJ";
                exp.processPopupCmd(cmd);
            }
            else
            {
                // For help menus other than about, the key giving the
                // strhelpfile, will actually be the command to be
                // executed including things like firefox and acroread
                String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
                                 strhelpfile};
                WUtil.runScriptInThread(cmd, true);
            }  
        }
        else if (strPropName.indexOf("command") >= 0)
        {
            /*if (m_objCmdArea == null)
                m_objCmdArea = new WCommandArea(m_sshare, m_appIf);
            m_objCmdArea.setVisible(true);*/
            String strterminal = "/usr/dt/bin/dtterm";
            if (Util.islinux())
                strterminal = "/usr/bin/gnome-terminal";
            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, strterminal};
            WUtil.runScriptInThread(cmd);
        }
        else if (strPropName.indexOf("printer") >= 0)
        {
          VjPlotterConfig pltConfig = new VjPlotterConfig(true);
          pltConfig.setVisible(true);
          return;
        }
        else if (strPropName.indexOf("accounting") >= 0)
        {
            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, 
                             FileUtil.sysdir() + "/bin/vnmr_accounting"};
            WUtil.runScriptInThread(cmd);
        }
        else if (strPropName.indexOf("validate") >= 0)
        {
            if (m_objPart11Dlg == null)
                m_objPart11Dlg = new WPart11Dialog();
            m_objPart11Dlg.build(WPart11Dialog.VALIDATE, "", strhelpfile);
        }
        else if (strPropName.indexOf("fonts colors") >= 0)
        {
            if (m_objFontColors == null)
                m_objFontColors = new WFontColors();
            m_objFontColors.setVisible(true);
        }
        else if (strPropName.indexOf("database") >= 0)
        {
            if (m_objPart11Dlg == null)
                m_objPart11Dlg = new WPart11Dialog();
            m_objPart11Dlg.build(WPart11Dialog.CONFIG, "SYSTEM/PART11/part11Config", strhelpfile);
        }
        else if (strPropName.indexOf("user policies") >= 0 ||
                    strPropName.indexOf("passwords") >= 0)
        {
            if (m_objPart11Dlg == null)
                m_objPart11Dlg = new WPart11Dialog();
            m_objPart11Dlg.build(WPart11Dialog.DEFAULT, "SYSTEM/PROFILES/accPolicy", strhelpfile);
        }
        else if (strPropName.indexOf("checksum") >= 0)
        {
            if (m_objPart11Dlg == null)
                m_objPart11Dlg = new WPart11Dialog();
            m_objPart11Dlg.build(WPart11Dialog.CHECKSUM, "SYSTEM/PART11/part11Config", strhelpfile);
        }
        else if (strPropName.indexOf("notify") >= 0)
        {
            if (m_objPart11Notify == null)
                m_objPart11Notify = new WPart11Notify();
            m_objPart11Notify.setVisible(true, strhelpfile);
        }
        else if (strPropName.indexOf("convert users") >= 0)
        {
            if (m_objConvertDlg == null)
                m_objConvertDlg = new WConvertUsers(m_sshare, "convert");
            m_objConvertDlg.setVisible(true, strhelpfile);
        }
        else if (strPropName.indexOf("update users") >= 0)
        {
            if (m_objConvertDlg2 == null)
                m_objConvertDlg2 = new WConvertUsers(m_sshare, "update");
            m_objConvertDlg2.setVisible(true, strhelpfile);
        }
        else if (strPropName.indexOf("user defaults") >= 0)
        {
            if (m_objUsersConfig == null)
                m_objUsersConfig = new WUsersConfig(m_appIf);
            m_objUsersConfig.setVisible(true, strhelpfile);
        }
        else if (strPropName.indexOf("operators") >= 0)
        {
            if (m_objOperators == null)
                m_objOperators = new WOperators(strPropName);
            m_objOperators.setVisible(true, strPropName, strhelpfile);
        }
        else if (strPropName.indexOf("automation") >= 0)
        {
            if (m_objAutomation == null)
                m_objAutomation = new WAutomation(strhelpfile);
            m_objAutomation.setVisible(true);
        }
        else if (strPropName.indexOf("investi") >= 0)
        {
            if (m_objInvestigator == null)
                m_objInvestigator = new WInvestigator(strhelpfile);
            m_objInvestigator.setVisible(true);
        }
        else if (strPropName.indexOf("dicom") >= 0)
        {
            WDicom.showDialog(strhelpfile);
        }
        else if (strPropName.indexOf("logout") >= 0)
        {
            WLoginBox.showLoginBox(true, "help:"+strhelpfile);
        }
        else if (strPropName.indexOf("administrators") >= 0)
        {
            if (m_objAdministrator == null)
                m_objAdministrator = new WAdministrators(strhelpfile);
            m_objAdministrator.setVisible(true);
        }
        else if (strPropName.indexOf("file system") >= 0)
        {
            if (m_objDirDlg == null)
                m_objDirDlg = new DirViewDialog();
            m_objDirDlg.setVisible(true);
        }
        else if (strPropName.indexOf("data storage") >= 0)
        {
            if (m_objDfTable == null)
                m_objDfTable = new DirViewDialog(DirViewDialog.DFTABLE);
            m_objDfTable.setVisible(true);
        }
        else if (strPropName.indexOf("profile") >= 0)
        {
            if (m_objProfileEditor == null)
                m_objProfileEditor = new UserRightsPanel("adm");
            m_objProfileEditor.setVisible(true);
        }
        else if (strPropName.indexOf("locator") >= 0)
        {
            if (m_objLocator == null)
                m_objLocator = new WLocatorConfig(strhelpfile);
            m_objLocator.setVisible(true);
        }
        else if (strPropName.indexOf("shared console") >= 0)
        {
            WSharedConsoleDialog.openPanel();
        }
        else if (strPropName.indexOf("export log to csv") >= 0)
        {
            if(logToCSVPanel == null) {
                String sysDir = System.getProperty("sysdir");
                String logFilepath = sysDir + File.separator + "adm" + File.separator + 
                                     "accounting"+ File.separator + "acctLog.xml";
                logToCSVPanel = new LogToCSVPanel(logFilepath); 
            }
            logToCSVPanel.showPanel();
        }
    }

    /**
     *  Returns the object for configuring users.
     */
    public WUsersConfig getUsersConfigObj()
    {
        if (m_objUsersConfig == null)
            m_objUsersConfig = new WUsersConfig(m_appIf);
        return m_objUsersConfig;
    }

    protected void runScript(String strScript)
    {
        final String strPath = FileUtil.openPath(strScript);
        if (strPath == null)
            return;

        new Thread(new Runnable()
        {
            public void run()
            {
                String[] cmd = {WGlobal.SHTOOLCMD, "-c", WGlobal.SUDO + " " + strPath};
                WUtil.runScript(cmd);
            }
        }).start();
    }

    // getVersion() is used to write the current version of vnmrj
    // and patches to /vnmr/tmp/version.  This is then displayed when the
    // "About VnmrJ" menu is selected.  This is normally done by the
    // macro "gitversion".  We cannot call macros from Vnmrj adm since
    // there is no Vnmrbg.  So, we will spawn Vnmrbg and have it do the
    // task.
    private void getVersion() {
        Process proc=null;

        String[] cmd = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION,
                        "Vnmrbg -mback -e1 getversion > /dev/null 2>&1" };
        
        try {
            Runtime rt = Runtime.getRuntime();
            proc = rt.exec(cmd);
            proc.waitFor();
        }
        catch(Exception e) { 
            Messages.postDebug("Problem executing the macro getversion");
            Messages.writeStackTrace(e);
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(proc != null) {
                    OutputStream os = proc.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = proc.getInputStream();
                    if(is != null)
                        is.close();
                    is = proc.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }

    }

}
