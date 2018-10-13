/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;

import vnmr.util.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;

/**
 * <p>Title: VLoginPassword </p>
 * <p>Description: Changes login passwords for vnmrj users </p>
 * <p>Copyright: Copyright (c) 2002</p>
 */

public class VLoginPassword extends ModalDialog implements ActionListener
{

    protected static JTextField m_txfUsername = new JTextField();
    protected static JPasswordField m_passwordFieldOld = new JPasswordField();
    protected static JPasswordField m_passwordFieldNew = new JPasswordField();
    protected static JPasswordField m_passwordFieldNew2 = new JPasswordField();
    
    protected static VLoginPassword m_loginPassword;
    public static final String LOGIN = FileUtil.SYS_VNMR + "/bin/loginpasswordVJ " +
                                       FileUtil.SYS_VNMR + "/bin/loginpassword";
    public static final String LOGIN_WIN = "posix /u /c " + FileUtil.SYS_VNMR +
                                            "/bin/loginpassword_win";

    public VLoginPassword(String helpFile)
    {
        super(Util.getLabel("_Change_Login_Password"));
        m_strHelpFile = helpFile;
        
        // *Warning, working around a Java problem*
        // When we went to the T3500 running Redhat 5.3, the JPasswordField
        // fields sometimes does not allow ANY entry of characters.  Setting
        // the enableInputMethods() to true fixed this problem.  There are
        // comments that indicate that this could cause the typed characters
        // to be visible.  I have not found that to be a problem.
        // This may not be required in the future, or could cause characters
        // to become visible in the future if Java changes it's code.
        // GRS  8/20/09
        m_passwordFieldOld.enableInputMethods(true);
        m_passwordFieldNew.enableInputMethods(true);
        m_passwordFieldNew2.enableInputMethods(true);

        initLayout();
        //        setBackground(Util.getBgColor());
        setSize(400, 300);
        setLocationRelativeTo(getParent());

        okButton.addActionListener(this);
        okButton.setActionCommand("ok");
        cancelButton.addActionListener(this);
        cancelButton.setActionCommand("cancel");
        helpButton.addActionListener(this);
        helpButton.setActionCommand("help");
    }

    public static void showDialog(String helpFile)
    {
        if (m_loginPassword == null)
            m_loginPassword = new VLoginPassword(helpFile);
        initFields();
        m_loginPassword.setVisible(true);
    }

    protected void initLayout()
    {
        GridBagLayout gbLayout = new GridBagLayout();
        GridBagConstraints gbc = new GridBagConstraints(0, 0, 1, 1, 1, 1,
                                                        GridBagConstraints.NORTH,
                                                        GridBagConstraints.HORIZONTAL,
                                                        new Insets(0,0,0,0), 0, 0);
        JPanel pnlPassword = new JPanel(gbLayout);
        pnlPassword.add(new JLabel(Util.getLabel("_Operator")), gbc);
        gbc.gridx = 1;
        pnlPassword.add(m_txfUsername, gbc);
        gbc.gridx = 0;
        gbc.gridy = 1;
        pnlPassword.add(new JLabel(Util.getLabel("_Password")), gbc);
        gbc.gridx = 1;
        pnlPassword.add(m_passwordFieldOld, gbc);
        gbc.gridx = 0;
        gbc.gridy = 2;
        pnlPassword.add(new JLabel(Util.getLabel("_New_Password")), gbc);
        gbc.gridx = 1;
        pnlPassword.add(m_passwordFieldNew, gbc);
        gbc.gridx = 0;
        gbc.gridy = 3;
        pnlPassword.add(new JLabel(Util.getLabel("_New_Password(reenter)")), gbc);
        gbc.gridx = 1;
        pnlPassword.add(m_passwordFieldNew2, gbc);

        Container contentPane = getContentPane();
        contentPane.add(pnlPassword, BorderLayout.CENTER);
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (cmd.equals("ok"))
        {
            boolean blogin = changePassword();
            if (blogin)
                dispose();
        }
        else if (cmd.equals("cancel"))
            dispose();
        else if (cmd.equals("help"))
            displayHelp();
    }

    protected static void initFields()
    {
        m_txfUsername.setText("");
        m_txfUsername.setCaretPosition(0);
        m_passwordFieldOld.setText("");
        m_passwordFieldOld.setCaretPosition(0);
        m_passwordFieldNew.setText("");
        m_passwordFieldNew.setCaretPosition(0);
        m_passwordFieldNew2.setText("");
        m_passwordFieldNew2.setCaretPosition(0);
    }

    protected boolean changePassword()
    {
        char[] aCharNewPassword1 = m_passwordFieldNew.getPassword();
        char[] aCharNewPassword2 = m_passwordFieldNew2.getPassword();
        char[] aCharOldPassword = m_passwordFieldOld.getPassword();
        String strUsername = m_txfUsername.getText();
        boolean bLogin = false;

        if (strUsername == null || strUsername.trim().length() == 0)
        {
            Messages.postError("Please enter user name");
            return bLogin;
        }
        if (aCharNewPassword1.length == 0 || aCharNewPassword2.length == 0)
        {
            Messages.postError("Please enter new password");
            return bLogin;
        }

        int nSize1 = aCharNewPassword1.length;
        int nSize2 = aCharNewPassword2.length;
        for (int i = 0; i < nSize1; i++)
        {
            if ((nSize1 != nSize2) || (aCharNewPassword1[i] != aCharNewPassword2[i]))
            {
                Messages.postError("New Passwords don't match");
                m_passwordFieldNew.setText("");
                m_passwordFieldNew2.setText("");
                return bLogin;
            }
        }

        String strfile = FileUtil.openPath(WUserUtil.PASSWORD);
        String[] cmd = {WGlobal.SHTOOLCMD, "-c", ""};
        String strLogin = LOGIN;
        if (Util.iswindows())
        {
            strLogin = LOGIN_WIN;
            strfile = UtilB.windowsPathToUnix(strfile);
        }
        if (LoginBox.unixPassword(strUsername, aCharOldPassword))
        {
            String strPath = FileUtil.savePath(FileUtil.usrdir()+"/.vj");
            if (!Util.iswindows())
            {
                cmd[2] = new StringBuffer().append(strLogin).append(" ").append(
                     strUsername).append(" \"").append(
                     String.valueOf(LoginBox.getPassword(aCharOldPassword))).append(
                     "\" \"").append(
                     String.valueOf(LoginBox.getPassword(aCharNewPassword1))).append(
                     "\"").toString();
            //LoginBox.unixPassword(strPath, cmd[2]);
            //cmd[2] = strPath;
                WMessage msg = WUtil.runScript(cmd, false);
                bLogin = isLoginOk(msg);
            }
            else
            {
                String strQuotes = "\"\"";
                if (aCharOldPassword.length == 0)
                    strQuotes= "\"";
                cmd[2] = new StringBuffer().append(strLogin).append(" ").append(
                     strUsername).append(" ").append(strQuotes).append(
                     String.valueOf(LoginBox.getPassword(aCharOldPassword))).append(
                     strQuotes).append(" \"\"").append(
                     String.valueOf(LoginBox.getPassword(aCharNewPassword1))).append(
                     "\"\"").toString();
                WMessage msg = WUtil.runScript(cmd[2], false);
                bLogin = isLoginOk(msg);
            }
        }

        if (!bLogin && LoginBox.vnmrjPassword(strUsername, aCharOldPassword))
        {
            try
            {
                String strNewPassword = PasswordService.getInstance().encrypt(new String(aCharNewPassword1));
                cmd[2] = new StringBuffer().append(FileUtil.SYS_VNMR).append(
                         "/bin/loginvjpassword ").append(
                         strfile).append(" ").append(strUsername).append(
                         " '").append(strNewPassword).append("'").toString();
                WMessage msg = WUtil.runScript(cmd, false);
                bLogin = isLoginOk(msg);
            }
            catch (Exception e)
            {
                //e.printStackTrace();
                Messages.writeStackTrace(e);
            }
        }

        if(!bLogin)
            Messages.postError("Password not changed. Please check that the current " +
                               "Username/Password is correct and the new password " +
                               " meets system requirements. ");
        else
            Messages.postInfo("Password successfully changed");

        return bLogin;
    }

    protected boolean isLoginOk(WMessage objMessage)
    {
        boolean bLogin = objMessage.isNoError();
        if (bLogin)
        {
            String strSu = objMessage.getMsg();
            if (strSu != null)
                strSu = strSu.toLowerCase();
            if (strSu == null || strSu.indexOf("killed") >= 0 ||
                strSu.indexOf("error") >= 0)
                bLogin = false;
        }
        return bLogin;
    }

}
