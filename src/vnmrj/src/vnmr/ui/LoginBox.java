/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import java.beans.*;

import javax.swing.*;
import javax.swing.plaf.basic.*;
import java.net.*;

import  vnmr.bo.*;
import  vnmr.util.*;
import vnmr.ui.shuf.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;

/*
 * Login box.
 *
 * @author Mark Cao
 */
public class LoginBox extends ModalDialog implements ActionListener , MouseListener, PropertyChangeListener
{
    protected JComboBox m_cmbUser;
    protected Component comboTextField;
    protected JPasswordField m_passwordField;
    protected JLabel m_lblLogin;
    protected JLabel m_lbltitle;
    protected JLabel m_lblSampleName;
    protected JLabel m_lblUsername;
    protected JLabel m_lblPassword;
    protected JPanel m_pnlTrays;
    protected String m_strHost;
    protected VBox[] m_pnlVast = new VBox[5];
    protected String m_strDir;
    protected JComponent m_pnlSampleName;
    protected javax.swing.Timer m_trayTimer;
    protected static int width = 0;
    protected static int height = 0;
    protected static Point position=null;
    protected static ShufDBManager m_dbmanager;
    protected static java.sql.ResultSet m_dbResult = null;
    protected static String m_strHostname = "localhost";
    protected static final String LOGIN = FileUtil.SYS_VNMR + "/bin/loginpasswordVJ " +
                                          FileUtil.SYS_VNMR + "/bin/loginpasswordcheck";
    protected static final String LOGIN_WIN = "posix /u /c " +
                                               FileUtil.SYS_VNMR + "/bin/loginpasswordcheck_win";
    protected static String[] m_aStrVast = new String[96];
    protected static final String VAST = "Vast";
    protected static final String OTHER = "Other";
    public static final String TRAYINFO = FileUtil.SYS_VNMR + "/asm/info/currentRacks";
    public static final String TRAY = "Sample tray Present";
    public static final String NOTRAY = "Sample tray Not Present";
    public static final String TRAYACTIVE = "Sample tray Active";
    protected static Properties pwprops = null;

    public LoginBox()
    {
        super("VnmrJ Login");
        
        dolayout("", "", "");
        setDefaultCloseOperation(JDialog.DO_NOTHING_ON_CLOSE);
        //setVast();
        DisplayOptions.addChangeListener(this);

        try
        {
            InetAddress inetAddress = InetAddress.getLocalHost();
            m_strHostname = inetAddress.getHostName();
        }
        catch (Exception e)
        {
            m_strHostname = "localhost";
        }

        VNMRFrame vnmrFrame = VNMRFrame.getVNMRFrame();
        Dimension size = vnmrFrame.getSize();
        position = vnmrFrame.getLocationOnScreen();
        AppIF appIF = Util.getAppIF();
        int h = appIF.statusBar.getSize().height;
        width = size.width;
        height = size.height-h;

        // Allow resizing and use the previous size and position
        // To stop resizing, use setResizable(false);
        readPersistence();
 
//        setSize(width, height);
//        setLocation(position);
//        setResizable(false);
        
        
        setBackgroundColor(Util.getBgColor());

        m_trayTimer = new javax.swing.Timer(6000, new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                setTrays();
            }
        });
    }

    public void propertyChange(PropertyChangeEvent e)
    {
        String strProp = e.getPropertyName();
        if (strProp == null)
            return;
        strProp = strProp.toLowerCase();
        if (strProp.indexOf("heading3") >= 0 || strProp.indexOf("plaintext") >= 0)
            setPref();
    }

    protected void dolayout(String strDir, String strFreq, String strTraynum)
    {
        // TopBar
        String strTitle = gettitle(strFreq);
        Color color = DisplayOptions.getColor("Heading3");

        //Center Panel
        JPanel panelCenter = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints(0, 0, 1, 1, 0.2, 0,
                                                        GridBagConstraints.WEST,
                                                        GridBagConstraints.HORIZONTAL,
                                                        new Insets(0,0,0,0), 0,0);
        ImageIcon icon = getImageIcon();
        //panelCenter.add(new JLabel(icon));
        addComp(panelCenter, new JLabel(icon), gbc, 0, 0);
        strTitle = getSampleName(strDir, strTraynum);
        m_lblSampleName = new JLabel(strTitle);
        if (strTitle == null || !strTitle.trim().equals(""))
            strTitle = "3";
        Font font = m_lblSampleName.getFont();
        font = DisplayOptions.getFont(font.getName(), Font.BOLD, 300);
        m_lblSampleName.setFont(font);
        m_lblSampleName.setForeground(color);
        //panelCenter.add(m_lblSampleName);
        m_pnlSampleName = new JPanel(new CardLayout());
        m_pnlSampleName.add(m_lblSampleName, OTHER);
        addComp(panelCenter, m_pnlSampleName, gbc, 1, 0);
        m_pnlSampleName.setVisible(false);
        m_pnlTrays = new JPanel(new GridLayout(1,0));
        // Vast panels
        VBox pnlVast1 = new VBox(m_pnlTrays, "1");
        m_pnlTrays.add(pnlVast1);
        m_pnlVast[0] = pnlVast1;
        VBox pnlVast2 = new VBox(m_pnlTrays, "2");
        m_pnlTrays.add(pnlVast2);
        m_pnlVast[1] = pnlVast2;
        VBox pnlVast3 = new VBox(m_pnlTrays, "3");
        m_pnlTrays.add(pnlVast3);
        m_pnlVast[2] = pnlVast3;
        VBox pnlVast4 = new VBox(m_pnlTrays, "4");
        m_pnlTrays.add(pnlVast4);
        m_pnlVast[3] = pnlVast4;
        VBox pnlVast5 = new VBox(m_pnlTrays, "5");
        m_pnlTrays.add(pnlVast5);
        m_pnlVast[4] = pnlVast5;
        m_pnlSampleName.add(m_pnlTrays, VAST);

        // Login Panel
        JPanel panelThird = new JPanel(new BorderLayout());
        JPanel panelLogin = new JPanel(new GridBagLayout());
        gbc = new GridBagConstraints();
        panelThird.add(panelLogin);
        Object[] aStrUser = getOperators();
        m_cmbUser = new JComboBox(aStrUser);
        BasicComboBoxRenderer renderer = new BasicComboBoxRenderer();
        m_cmbUser.setRenderer(renderer);
        m_cmbUser.setEditable(true);
        m_passwordField = new JPasswordField();
        // *Warning, working around a Java problem*
        // When we went to the T3500 running Redhat 5.3, the JPasswordField
        // fields sometimes does not allow ANY entry of characters.  Setting
        // the enableInputMethods() to true fixed this problem.  There are
        // comments that indicate that this could cause the typed characters
        // to be visible.  I have not found that to be a problem.
        // This may not be required in the future, or could cause characters
        // to become visible in the future if Java changes it's code.
        // GRS  8/20/09
        m_passwordField.enableInputMethods(true);

        m_lblLogin = new VLoginLabel(null,"Incorrect username/password \n Please try again ",0, 0);
        m_lblLogin.setVisible(false);
        okButton.setActionCommand("enter");
        okButton.addActionListener(this);
        cancelButton.setActionCommand("cancel");
        cancelButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);
        m_passwordField.addKeyListener(new KeyAdapter()
        {
            public void keyPressed(KeyEvent e)
            {
                if (e.getKeyCode() == KeyEvent.VK_ENTER)
                    enterLogin();
            }
        });
        
        // These is some wierd problem such that sometimes, the vnmrj command
        // area gets the focus and these items in the login box do not get
        // the focus.  Even clicking in these items does not bring focus to
        // them.  Issuing requestFocus() does not bring focus to them.
        // I can however, determing if either the operator entry box or
        // the password box has focus and if neither does, I can unshow and
        // reshow the panel and that fixes the focus.  So, I have added this
        // to the mouseClicked action.  If neither has focus, it will 
        // take focus with setVisible false then true.
        comboTextField = m_cmbUser.getEditor().getEditorComponent();
        m_passwordField.addMouseListener(this);
        comboTextField.addMouseListener(this);

        m_lblUsername = new JLabel(Util.getLabel("_Operator"));
        addComp(panelLogin, m_lblUsername, gbc, 0, 0);
        addComp(panelLogin, m_cmbUser, gbc, 1, 0);
        m_lblPassword = new JLabel(Util.getLabel("_Password"));
        addComp(panelLogin, m_lblPassword, gbc, 0, 1);
        addComp(panelLogin, m_passwordField, gbc, 1, 1);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.gridwidth = GridBagConstraints.REMAINDER;
        gbc.gridheight = GridBagConstraints.REMAINDER;
        addComp(panelLogin, m_lblLogin, gbc, 2, 0);
        setPref();

        Container container = getContentPane();
        JPanel panelLoginBox = new JPanel(new BorderLayout());
        panelLoginBox.add(panelCenter, BorderLayout.CENTER);
        panelLoginBox.add(panelThird, BorderLayout.SOUTH);
        container.add(panelLoginBox, BorderLayout.CENTER);
    }

    public void setVisible(boolean bShow, String title)
    {
        if (bShow)
        {
            String strDir = "";
            String strFreq = "";
            String strTraynum = "";
            m_strHelpFile = getHelpFile(title);
            String strSampleName = getSampleName(title);
            String frameBounds = getFrameBounds(title);
            StringTokenizer tok = new QuotedStringTokenizer(title);
            if (tok.hasMoreTokens())
                strDir = tok.nextToken();
            if (tok.hasMoreTokens())
                strFreq = tok.nextToken();
            if (tok.hasMoreTokens())
                strTraynum = tok.nextToken();
            else
            {
                try
                {
                    Integer.parseInt(strDir);
                    // if strdir is number, then strdir is empty, and the
                    // strfreq is the number
                    strTraynum = strFreq;
                    strFreq = strDir;
                    strDir = "";
                }
                catch (Exception e) {}
            }
            try
            {
                setTitle(gettitle(strFreq));
                m_lblSampleName.setText("3");
                boolean bVast = isVast(strTraynum);
                CardLayout layout = (CardLayout)m_pnlSampleName.getLayout();
                if (!bVast) {
                    if (strSampleName == null) {
                        strSampleName = getSampleName(strDir, strTraynum);
                    }
                    m_lblSampleName.setText(strSampleName);
                    layout.show(m_pnlSampleName, OTHER);
                }
                else
                {
                    m_strDir = strDir;
                    setTrays();
                    layout.show(m_pnlSampleName, VAST);
                    m_trayTimer.start();
                }
                boolean bSample = bVast || !strSampleName.trim().equals("");
                m_pnlSampleName.setVisible(bSample);
                m_lblLogin.setForeground(getBackground());
                m_lblLogin.setVisible(false);
                m_passwordField.setText("");
                m_passwordField.setCaretPosition(0);
            }
            catch (Exception e)
            {
                Messages.writeStackTrace(e);
            }
            setBounds(frameBounds);
            ExpPanel exp = Util.getActiveView();
            if (exp != null)
                exp.waitLogin(true);
        }
        writePersistence();
        setVisible(bShow);
    }

    /**
     * Set the placement of the login window according to the given keyword.
     * Options are:
     * <br> "variable": Put it where the last operator left it.
     * <br> "fixed": Put it where the location defined by the Unix user.
     * <br> "default": Put it on top of the VJ window (except hardware bar).
     * @param frameBounds The keyword. Default is "default".
     */
    private void setBounds(String frameBounds) {
        if ("variable".equalsIgnoreCase(frameBounds)) {
            readPersistence();
        } else if ("fixed".equalsIgnoreCase(frameBounds)) {
            readPersistence("USER/PERSISTENCE/LoginPanelFixedBounds");
        } else { // Default is "default"
            setDefaultSizePosition();
        }
    }

    /**
     * Checks the passwords of the user
     * @param e The ActionEvent for this action.
     */
    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        // checks if the user has a unix password, if it matches, then logs the user
        // in the interface, otherwise checks for the vnmrj password,
        // if neither password matches, then the user is not logged in the interface
        if (cmd.equalsIgnoreCase("enter"))
            enterLogin();
        else if (cmd.equalsIgnoreCase("cancel"))
        {
            m_passwordField.setText("");
            m_lblLogin.setForeground(getBackground());
            //setVisible(false);
        }
        else if (cmd.equalsIgnoreCase("help"))
            displayHelp();
        
    }

    protected void enterLogin()
    {
        char[] password = m_passwordField.getPassword();
        String strUser = (String)m_cmbUser.getSelectedItem();
        setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
        boolean blogin = WUserUtil.isOperatorNameok(strUser, false);
        if (blogin)
        {
            blogin = vnmrjPassword(strUser, password);
            if (!blogin)
                blogin = unixPassword(strUser, password);
        }
        if (blogin)
        {
            m_lblLogin.setForeground(Color.black);
            m_lblLogin.setText("Login Successful");
            m_lblLogin.setVisible(true);
            // Get the Email column string for access to the operator data
            String emStr = vnmr.util.Util.getLabel("_admin_Email");
            String stremail = WUserUtil.getOperatordata(strUser, emStr);

            // Get the Panel Level column string for access to the operator data
            String plStr = vnmr.util.Util.getLabel("_admin_Panel_Level");
            String strPanel = WUserUtil.getOperatordata(strUser, plStr);

            if (stremail == null || stremail.equals("null"))
                stremail = "";
            try
            {
                Integer.parseInt(strPanel);
            }  catch(Exception e) {
                strPanel = WGlobal.PANELLEVEL;
            }
            m_trayTimer.stop();
            Messages.postDebug(" Login: "+ strUser);
            // set the current operator
            Util.setCurrOperatorName(strUser);

            ExpPanel exp = Util.getActiveView();
            if (exp != null) {
                exp.startLogin();
                exp.sendToVnmr(new StringBuffer().append("appdir('reset','").append(
                                       strUser).append("','").append(
                                       stremail).append("',").append(
                                       strPanel).append(")").toString());
                exp.sendToVnmr("vnmrjcmd('util', 'bgReady')\n");
            }
            setVisible(false);
            
            // Save the current position and size of this panel in case it 
            // was changed
            Dimension size = getSize();
            width = size.width;
            height = size.height;
            position = getLocation();
            writePersistence();

            // Call the macro to update this operator's 
            // ExperimentSelector_operatorName.xml file
            // from the protocols themselves.  This macro will
            // cause an update of the ES when it is finished

            // Util.getAppIF().sendToVnmr("updateExpSelector");

            // I am not sure why we need to force updates since updateExpSelector
            // should have caused an update by writing to ES_op.xml file.
            // However, it works better if we do the force update.
            // ExpSelector.setForceUpdate(true);
        }
        else
        {
            m_lblLogin.setForeground(DisplayOptions.getColor("Error"));
            //m_lblLogin.setText("<HTML>Incorrect username/password <p> Please try again </HTML>");
            m_lblLogin.setVisible(true);
        }
        setCursor(Cursor.getDefaultCursor());
    }

    /**
     * Gets the list of the vnmrj users(operators) for the current unix user logged in
     * @return the list of vnmrj users
     */
    protected Object[] getOperators()
    {
        String strUser = System.getProperty("user.name");
        User user = LoginService.getDefault().getUser(strUser);
        ArrayList<String> aListOperators = user.getOperators();
        if (aListOperators == null || aListOperators.isEmpty())
            aListOperators = new ArrayList<String>();
        Collections.sort(aListOperators);
        if (aListOperators.contains(strUser))
            aListOperators.remove(strUser);
        aListOperators.add(0, strUser);
        return (aListOperators.toArray());
    }

    protected String getHost()
    {
        if (m_strHost == null)
            m_strHost = Util.getHostName();
        return m_strHost;
    }

    protected ShufDBManager getdbmanager()
    {
        if (m_dbmanager == null)
          m_dbmanager = ShufDBManager.getdbManager();
        return m_dbmanager;
    }

    /**
     * Gets the imageicon. If the user has a predefined icon, get that, otherwise
     * get the varian icon.
     * @return the imageicon
     */
    protected ImageIcon getImageIcon()
    {
        ImageIcon icon = null;
        String strPath = WOperators.getDefIcon();
        if (strPath != null)
        {
           icon = Util.getImageIcon(strPath);
        }
        if (icon == null)
           icon = Util.getImageIcon(WOperators.ICON);
        return icon;
    }

    protected void setTrays()
    {
        String strPath = FileUtil.openPath(TRAYINFO);
        if (strPath == null)
            return;

        setTrayPresent(strPath);
        setTrayActive(m_strDir);
        repaint();
    }

    protected void setTrayPresent(String strPath)
    {
        String strzone = "zones";
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        if (reader == null)
            return;

        String strLine;
        try
        {
            // if the zones are set to 0, then there is no tray present
            // set rackInfo(1,zones) 3 => tray 1 present
            // set rackInfo(2,zones) 0 => tray 2 not present
            while ((strLine = reader.readLine()) != null)
            {
               int index = strLine.indexOf(strzone);
               if (index < 0)
                   continue;

               String strTray = strLine.substring(index-2, index-1);
               int nTray = 0;
               try
               {
                   nTray = Integer.parseInt(strTray);
               }
               catch (Exception e) {}

               if (nTray <= 0)
                   continue;

               int nZone = 0;
               try
               {
                   strTray = strLine.substring(index+strzone.length()+2, strLine.length());
                   nZone = Integer.parseInt(strTray);
               }
               catch (Exception e) {}

               Color colorbg = Color.white;
               String strTooltip = TRAY;
               if (nZone <= 0)
               {
                   colorbg = Color.black;
                   strTooltip = NOTRAY;
               }
               VBox pnlVast = m_pnlVast[nTray-1];
               pnlVast.setbackground(colorbg);
               pnlVast.setToolTipText(Util.getTooltipString(strTooltip));
            }
            reader.close();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

    protected void setTrayActive(String strDir)
    {
        String strTray;
        String strStudy;
        String strActive = "Active";
        String cmd = "SELECT vrack_,studystatus from study WHERE (autodir=\'"+
                      strDir+"\') AND (hostname=\'"+getHost()+"\')";
        m_dbResult = null;

        try {
            m_dbResult = getdbmanager().executeQuery(cmd);
        }
        catch (Exception e) {
            return;
        }
        if (m_dbResult == null)
            return;

        int nTray = 0;
        try
        {
            while (m_dbResult.next())
            {
                strTray = m_dbResult.getString(1);
                strStudy = m_dbResult.getString(2);
                if (strStudy != null && strStudy.equalsIgnoreCase(strActive))
                {
                    try
                    {
                        nTray = Integer.parseInt(strTray);
                    }
                    catch (Exception e) {}

                    if (nTray > 0)
                    {
                        m_pnlVast[nTray-1].setbackground(Color.blue);
                        m_pnlVast[nTray-1].setToolTipText(Util.getTooltipString(TRAYACTIVE));
                        break;
                    }
                }
            }
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }

    }

    protected boolean isVast(String strTraynum)
    {
        int nTray = 0;
        boolean bVast = false;
        try
        {
            nTray = Integer.parseInt(strTraynum);
        }
        catch (Exception e) {}
        if (nTray > 0)
        {
            if (nTray == 96 || nTray == 768)
                bVast = true;
        }

        return bVast;
    }

    protected String getSampleName(String strDir, String strTray)
    {
        String strSampleName = "";
        String strSample;
        int nTray = 0;
        try
        {
            nTray = Integer.parseInt(strTray);
        }
        catch (Exception e) {}

        if (nTray <= 0)
            return strSampleName;

        String cmd = "SELECT loc_,studystatus from study WHERE (autodir=\'"+
                      strDir+"\') AND (hostname=\'"+getHost()+"\')";
        // NB: bArrSample[0] is not used; array implicitly initialized false
        boolean[] bArrSample = new boolean[nTray + 1];
        m_dbResult = null;

        try {
          m_dbResult = getdbmanager().executeQuery(cmd);
       }
       catch (Exception e) {
           return strSampleName;
       }
       if (m_dbResult == null)
           return strSampleName;


        try
        {
            while(m_dbResult.next())
            {
                strSample = m_dbResult.getString(1);
                int nSample = 0;
                nSample = Integer.parseInt(strSample);
                try {
                    bArrSample[nSample] = true;
                } catch (IndexOutOfBoundsException ioobe) {
                    Messages.postDebug("getSampleName: index out of bounds: "
                                       + nSample);
                }
            }
            for (int i = 1; i < bArrSample.length; i++)
            {
                if (!bArrSample[i])
                {
                    strSampleName = String.valueOf(i);
                    break;
                }
            }
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
        return strSampleName;
    }

    protected String gettitle(String strFreq)
    {
        StringBuffer sbufTitle = new StringBuffer().append("VnmrJ  ");
        String strPath = FileUtil.openPath(FileUtil.SYS_VNMR+"/vnmrrev");
        BufferedReader reader = WFileUtil.openReadFile(strPath);
        String strLine;
        String strtype = "";
        if (reader == null)
            return sbufTitle.toString();

        try
        {
            while ( (strLine = reader.readLine()) != null)
            {
                strtype = strLine;
            }
            strtype = strtype.trim();
            if (strtype.equals("merc"))
                strtype = "Mercury";
            else if (strtype.equals("mercvx"))
                strtype = "Mercury-Vx";
            else if (strtype.equals("mercplus"))
                strtype = "MERCURY plus";
            else if (strtype.equals("inova"))
                strtype = "INOVA";
            String strHostName = m_strHostname;
            if (strHostName == null)
                strHostName = "";
            sbufTitle.append("    ").append(strHostName);
            sbufTitle.append("    ").append(strtype);
            sbufTitle.append(" - ").append(strFreq);
            reader.close();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.logError(e.toString());
        }

        return sbufTitle.toString();
    }

    protected static boolean unixPassword(String strUser, char[] password)
    {
        password = getPassword(password);
        boolean bSu = true;
        String strSu;
        PrintWriter fout = null;

        if (!Util.iswindows())
        {
            String filepath = FileUtil.savePath("USER/PERSISTENCE/passwd");
            if (filepath == null) {
               filepath = FileUtil.savePath("USER/PERSISTENCE/tmp_passwd");
            }
            try {
                fout = new PrintWriter(new FileWriter(filepath));
                if (fout != null)
                    fout.println(String.valueOf(password));
            }
            catch(IOException er) { }
            finally {
                try {
                    if (fout != null)
                        fout.close();
                } catch (Exception e) {}
            }

            /**********
            String strPath = new StringBuffer().append(LOGIN).append(" ").append(
                                strUser).append(" \"").append(String.valueOf(
                                password)).append("\"").toString();
            **********/

            String strPath = new StringBuffer().append(LOGIN).append(" ").append(
                                strUser).append(" \"").append(filepath).append(
                                "\"").toString();
            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, strPath};
            WMessage objMessage = WUtil.runScript(cmd, false);
            bSu = objMessage.isNoError();
            strSu = objMessage.getMsg();
        }
        else
        {
            String strQuotes = "\"\"";
            if (password.length == 0)
                strQuotes = "\"";
            String cmd = new StringBuffer().append(LOGIN_WIN).append(" ").append(
                                    strUser).append(" ").append(strQuotes).append(
                                    String.valueOf(password)).append(strQuotes).toString();
            WMessage objMessage = WUtil.runScript(cmd, false);
            bSu = objMessage.isNoError();
            strSu = objMessage.getMsg();
        }
        if (bSu)
        {
            if (strSu != null)
                strSu = strSu.toLowerCase();
            if (strSu == null || strSu.trim().equals("") ||
                strSu.indexOf("killed") >= 0 || strSu.indexOf("error") >= 0)
                bSu = false;
        }

        return bSu;
    }

    protected static boolean vnmrjPassword(String strUser, char[] password)
    {
        boolean blogin = false;
        try
        {
            PasswordService objPassword = PasswordService.getInstance();
            String encrPassword = objPassword.encrypt(new String(password));
            if(pwprops == null) {
                String strPath = FileUtil.openPath(WUserUtil.PASSWORD);
                if (strPath == null)
                    return blogin;
                pwprops = new Properties();
                FileInputStream fis = new FileInputStream(strPath);
                pwprops.load(fis);
                fis.close();
            }
            String stoPassword = pwprops.getProperty(strUser);
            if (encrPassword.equals(stoPassword))
                blogin = true;
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }

        return blogin;
    }

    protected static char[] getPassword(char[] password)
    {
        int size = password.length;
        int data = 0;
        boolean bwindows = Util.iswindows();
        for (int i = 0; i < size; i++)
        {
            if (!bwindows)
            {
                if (password[i]=='`' || password[i]=='"' || password[i]=='\\')
                    data = data+1;
            }
            else
            {
                if (password[i]=='"')
                    data = data+1;
                else if (password[i]==' ')
                    data = data+2;
            }
        }
        char[] password2 = new char[size+data];
        int size2 = password2.length;
        int j = 0;
        for (int i = 0; i < size2; i++)
        {
            if (j >= size)
                break;
            boolean bSpace = false;

            if (!bwindows)
            {
                if (password[j]=='`' || password[j]=='"' || password[j]=='\\')
                {
                    password2[i]='\\';
                    i=i+1;
                }
            }
            else
            {
                if (password[j]=='"')
                {
                    password2[i]='\\';
                    i=i+1;
                }
                else if (password[j]==' ')
                {
                    password2[i]='"';
                    i=i+1;
                    password2[i] = password[j];
                    i=i+1;
                    password2[i]='"';
                    //i=i+1;
                    bSpace = true;
                }
            }
            if (!bSpace)
                password2[i] = password[j];
            j=j+1;
        }

        return password2;
    }

    protected static char[] getPasswordForWindows(char[] password, char[] passwordnew)
    {
        int size = password.length;
        int data = 0;
        int nCount = 0;
        //char[] passwordOrig = (char[])password.clone();
        for (int i = 0; i < size; i++)
        {
            if (password[i]=='"')
            {
                data = data+1;
                nCount = nCount+1;
            }
            /*else if ((password[i]=='^' || password[i]=='<' ||
                        password[i]=='>' || password[i]=='|' ||
                        password[i]=='&' || password[i]=='(' ||
                        password[i]==')') && nCount%2 != 0)
                data = data+1;*/
        }

        char[] password2 = new char[size+data];
        int size2 = password2.length;
        int j = 0;
        nCount = 0;
        for (int i = 0; i < size2; i++)
        {
            if (j >= size)
                break;

            // for windows, escape character for posix is ^
            if (password[j]=='"')
            {
                password2[i]='\\';
                i=i+1;
                nCount = nCount+1;
            }
            // for the following characters, there should be odd number of double quotes
            /*else if ((password[j] == '^' || password[j]=='<' || password[j]=='>'||
                        password[j] == '|' || password[j] == '&' ||
                        password[j] == '(' || password[j] == ')') &&
                        nCount%2 != 0)
            {
                   password2[i]='^';
                   i=i+1;
                   //data2 = data2+1;
            }*/

            password2[i] = password[j];
            j=j+1;
        }
        return password2;
    }

    protected void setVast()
    {
        String strPath = FileUtil.openPath("SYSTEM/asm/gilsonNumber");
        if (strPath == null)
            return;

        BufferedReader reader = WFileUtil.openReadFile(strPath);
        if (reader == null)
            return;

        String strLine;
        int i = 0;
        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                strLine = strLine.trim();
                if (i == 0 || !m_aStrVast[i-1].equals(strLine))
                {
                    m_aStrVast[i] = strLine;
                    i = i+1;
                }
            }
            reader.close();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

    /**
     * Set the helpfile string from the given "title".
     * The "title" is a series of tokens of the form:
     * <pre>
     * autodir h1freq traymax [help:path] [nextloc:loc]
     * <pre>
     * @param title The "title" passed in.
     * @return The path to the help file, or null.
     */
    protected String getHelpFile(String title)
    {
        StringTokenizer tok = new QuotedStringTokenizer(title);
        String path = null;
        while (tok.hasMoreTokens()) {
            String strValue = tok.nextToken();
            if (strValue.startsWith("help:")) {
                path = strValue.substring(5);
            }
        }
        return path;
    }

    /**
     * Get the "next sample" string from the given "title".
     * The "title" is a series of tokens of the form:
     * <pre>
     * autodir h1freq traymax [help:path] [nextloc:loc] [frameBounds:keyword]
     * <pre>
     * @param title The "title" passed in.
     * @return The next available sample location, or null.
     */
    protected String getSampleName(String title) {
        String name = null;
        StringTokenizer tok = new QuotedStringTokenizer(title);
        while (tok.hasMoreTokens()) {
            String strValue = tok.nextToken();
            if (strValue.startsWith("nextloc:")) {
                name = strValue.substring(8);
            }
        }
        return name;
    }

    /**
     * Gets the keyword telling where to place the login frame.
     * The "title" is a series of tokens of the form:
     * <pre>
     * autodir h1freq traymax [help:path] [nextloc:loc] [frameBounds:keyword]
     * <pre>
     * @param title The "title" passed in.
     * @return The frame location keyword.
     */
    private String getFrameBounds(String title) {
        String bounds = null;
        StringTokenizer tok = new QuotedStringTokenizer(title);
        while (tok.hasMoreTokens()) {
            String strValue = tok.nextToken();
            if (strValue.startsWith("frameBounds:")) {
                bounds = strValue.substring(12);
            }
        }
        return bounds;
    }

    protected void setBackgroundColor(Color color)
    {
        super.setBackgroundColor(color);
        if (m_cmbUser != null)
            ((BasicComboBoxRenderer)m_cmbUser.getRenderer()).setBackground(color);
    }

    protected void setPref()
    {
        Color color = DisplayOptions.getColor("PlainText");
        Font font = DisplayOptions.getFont("PlainText");
        m_lblUsername.setForeground(color);
        m_lblUsername.setFont(font);
        m_lblPassword.setForeground(color);
        m_lblPassword.setFont(font);
        m_cmbUser.getEditor().getEditorComponent().setForeground(color);
        m_cmbUser.getEditor().getEditorComponent().setFont(font);
        m_passwordField.setForeground(color);
        m_passwordField.setFont(font);

        color = DisplayOptions.getColor("Heading3");
        font = DisplayOptions.getFont("Heading3");
        m_lblSampleName.setForeground(color);
        m_pnlTrays.setBorder(BorderDeli.createBorder("Empty", "Sample Trays",
                                                "Bottom", "Center", color, font));
        VBox.setforeground(color);
        VBox.setfont(font);
    }

    protected void addComp(JPanel panel, Component comp, GridBagConstraints gbc,
                           int x, int y)
    {
        gbc.gridx = x;
        gbc.gridy = y;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        panel.add(comp, gbc);
    }

    /**
     * Write file with position and size of the login box
     */
    static public void writePersistence() {

        Messages.postDebug("LoginBox", "LoginBox.writePersistence");
        // If the panel has not been created, don't try to write a file
        if(position == null)
            return;

        String filepath = FileUtil.savePath("USER/PERSISTENCE/LoginPanel");

        FileWriter fw;
        PrintWriter os;
        try {
              File file = new File(filepath);
              fw = new FileWriter(file);
              os = new PrintWriter(fw);
              os.println("Login Panel");
              
              os.println(height);
              os.println(width);
              double xd = position.getX();
              int xi = (int) xd;
              os.println(xi);
              double yd = position.getY();
              int yi = (int) yd;
              os.println(yi);
                            
              os.close();
        }
        catch(Exception er) {
             Messages.postError("Problem creating  " + filepath);
             Messages.writeStackTrace(er);
        }
    }

    /**
     * Read previous position and size of the login box from the
     * "LoginPanel" file in the persistence directory.
     */
    public void readPersistence() {
        readPersistence("USER/PERSISTENCE/LoginPanel");
    }

    /**
     * Read position and size of the login box from a given abstract path.
     * @param abstractPath The path to the file.
     */
    public void readPersistence(String abstractPath) {
        String filepath = FileUtil.openPath(abstractPath);

        if(filepath != null) {
            BufferedReader in;
            String line;
            try {
                File file = new File(filepath);
                in = new BufferedReader(new FileReader(file));
                // File must start with 'Login Panel'
                if((line = in.readLine()) != null) {
                    if(!line.startsWith("Login Panel")) {
                        Messages.postWarning("The " + filepath + " file is " +
                                             "corrupted and being removed");
                        // Remove the corrupted file.
                        file.delete();
                        // Set the size and position to the full vnmrj frame
                        setDefaultSizePosition();

                        return;
                    }
                }
                String h=null, w=null, x=null, y=null;
                int xi, yi;
            
                if(in.ready())
                    h = in.readLine().trim();

                if(in.ready())
                    w = in.readLine().trim();

                if(in.ready())
                    x = in.readLine().trim();

                if(in.ready())
                    y = in.readLine().trim();

                in.close();

                // Save width and height for later use also
                height = Integer.decode(h).intValue();
                width = Integer.decode(w).intValue();
                xi = Integer.decode(x).intValue();
                yi = Integer.decode(y).intValue();
                // Save point for later use also
                position = new Point(xi, yi);

                // Set them
                setSize(width, height);
                setLocation(position);
                

                // If we got what we need and set the size and position,
                // just return now.
                return;
            }
            // If an exception, continue below
            catch (Exception e) { }
        }

        // No file or an excpetion happened, set default size and position
        // Be sure the file is gone
        try {
            if(filepath != null) {
                File file = new File(filepath);
                if(file != null)
                    file.delete();
            }
        }
        // If an exception, just continue below
        catch (Exception e) { }

        // Set the size and position to the full vnmrj frame
        setDefaultSizePosition();

    }

    // Get the size of the full vnmrj frame and set the Login Panel
    // to that size and position.
    public void setDefaultSizePosition() {
        VNMRFrame vnmrFrame = VNMRFrame.getVNMRFrame();
        Dimension size = vnmrFrame.getSize();

        // Save point for later use also
        position = vnmrFrame.getLocationOnScreen();
        AppIF appIF = Util.getAppIF();
        int h = appIF.statusBar.getSize().height;

        // Save width and height for later use also
        width = size.width;
        height = size.height-h;

        // Go ahead and set the size and position
        setSize(width, height);
        setLocation(position);
    }

    
    // Methods for MouseListener
    public   void  mouseEntered(MouseEvent e)  {}
    public   void  mouseExited(MouseEvent e)  {}
    public   void  mousePressed(MouseEvent e)  {}

    // These is some wierd problem such that sometimes, the vnmrj command
    // area gets the focus and these items in the login box do not get
    // the focus.  Even clicking in these items does not bring focus to
    // them.  Issuing requestFocus() does not bring focus to them.
    // I can however, determing if either the operator entry box or
    // the password box has focus and if neither does, I can unshow and
    // reshow the panel and that fixes the focus.  So, I have added this
    // to the mouseClicked action.  If neither has focus, it will 
    // take focus with setVisible false then true.
    public void mouseClicked(MouseEvent e) {
        boolean pfocus = m_passwordField.hasFocus();
        boolean ofocus = comboTextField.hasFocus();
        if (!pfocus && !ofocus) {
            setVisible(false);
            setVisible(true);
        }
    }

    public void mouseReleased(MouseEvent e) {}

}

class MemComboAgent extends KeyAdapter
{
    protected JComboBox   m_comboBox;
    protected JTextField  m_editor;

    public MemComboAgent(JComboBox comboBox)
    {
        m_comboBox = comboBox;
        m_editor = (JTextField)comboBox.getEditor().getEditorComponent();
        m_editor.addKeyListener(this);
    }

    public void keyReleased(KeyEvent e)
    {
        char ch = e.getKeyChar();
        if (ch == KeyEvent.CHAR_UNDEFINED || Character.isISOControl(ch))
            return;
        int pos = m_editor.getCaretPosition();
        String str = m_editor.getText();
        if (str.length() == 0)
            return;

        for (int k = 0; k < m_comboBox.getItemCount(); k++)
        {
            String item = m_comboBox.getItemAt(k).toString();
            if (item.startsWith(str))
            {
                m_editor.setText(item);
                m_editor.setCaretPosition(item.length());
                m_editor.moveCaretPosition(pos);
                m_comboBox.setSelectedItem(item);
                break;
            }
        }
    }
}

class VBox extends JComponent
{
    Color bgColor = Color.blue;
    Component comp;
    int width;
    int height;
    int x = 20;
    int y = 60;
    String strTray= "";
    static Color fgColor;
    static Font font;

    public VBox(int w, int h, int x1, int y1, Color colorbg)
    {
        super();
        width = w;
        height = h;
        x = x1;
        y = y1;
        bgColor = colorbg;
    }

    public VBox(Component parent, String tray)
    {
        super();
        comp = parent;
        strTray = tray;
    }

    protected void setbackground(Color colorbg)
    {
        bgColor = colorbg;
    }

    protected static void setfont(Font fontbox)
    {
        font = fontbox;
    }

    protected static void setforeground(Color color)
    {
        fgColor = color;
    }

    public void paint(Graphics g)
    {
        if (comp != null)
        {
            width = comp.getWidth()/6;
            height = comp.getHeight()*2/3;
        }
        g.setColor(bgColor);
        g.fillRect(x,y,width,height);
        g.setColor(fgColor);
        g.setFont(font);
        g.drawString(strTray, x/2+width/2, y+height+10);
        super.paint(g);
    }
}

class VLoginLabel extends JLabel
{
    int width,height,num_lines,line_height,line_ascent,max_width,max_height;
    int[] line_widths;

    int btnMarginWidth =7;
    int btnMarginHeight=7;
    int btnPlacement   =3;   // South (0=North,1=West,2=East,3=South)

    private String lines[]=null;            // multi-line text

    /*
     * Call this constructor with Image icon set to null
     * if all you want is to create a multi-line button.
     */
    VLoginLabel(Image icon, String s, int w, int h) {
        super("");
        if (icon==null) newLabel(s);
    }

    private void newLabel(String s) {
        StringTokenizer tkn=new StringTokenizer(s,"\n");
        num_lines=tkn.countTokens();
        lines=new String[num_lines];
        line_widths=new int[num_lines];
        for (int i=0;i<num_lines;i++)
        {
            lines[i] = tkn.nextToken();
        }
    }

    private void measure() {
        FontMetrics fontmetrics=getFontMetrics(getFont());
        if (fontmetrics==null) return;
        line_height=fontmetrics.getHeight();
        line_ascent=fontmetrics.getAscent();
        max_width=0;
        for (int i=0;i<num_lines;i++) {
            line_widths[ i ]=fontmetrics.stringWidth(lines[ i ]);
            if (line_widths[ i ]>max_width)
                max_width=line_widths[ i ];
        }
        max_width+=2*btnMarginWidth;
        max_height=num_lines*line_height+2*btnMarginHeight;
    }

    public void addNotify() {
        super.addNotify();
        measure();
    }

    public Dimension getPreferredSize() {
        return getMinimumSize();
    }

    public Dimension getMinimumSize() {
        return new Dimension(max_width,max_height);
    }

    public void paint(Graphics g) {
        Dimension d=getSize();
        int j=line_ascent+btnMarginHeight;
        for (int k=0;k<num_lines;k++) {
            int i=(d.width-line_widths[k])/2;
            g.drawString(lines[k],i,j);
            j+=line_height;
        }
        if (btnPlacement==1 || btnPlacement==2) setSize(d.width,max_height);
    }
}

