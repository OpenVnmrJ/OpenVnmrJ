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
import java.awt.*;
import javax.swing.*;
import java.util.*;
import java.beans.*;
import java.awt.event.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.util.*;
// import vnmr.bo.TwoColumnPanel;

/**
 * Title:   WPart11Dialog
 * Description: This class displays the part11 stuff.
 * Copyright:    Copyright (c) 2002
 */

public class WPart11Dialog extends ModelessDialog implements ActionListener
{
    /** The panel where the components are displayed.  */
    protected JPanel m_pnlDisplay;

    protected ArrayList m_aListComp         = new ArrayList();

    protected String m_strPath              = null;

    protected int m_nType                   = 0;

    protected JButton validateButton        = new JButton("Validate");

    protected AccPolicyPanel m_pnlAccPolicy = null;

    protected JButton m_btnChecksum         = null;

    protected ChecksumPanel m_pnlChecksum   = null;

    protected JComboBox m_cmbPath           = null;

    protected JComboBox m_cmbChecksum       = null;

    protected JTextArea m_txaChecksum       = null;

    protected javax.swing.Timer m_objTimer  = null;

    protected java.util.Timer timer;
    protected String labelString;       // The label for the window
    protected int delay;                // the delay time between blinks

    protected static final String[] m_aStrMode = {"FDA", "Non-FDA"};

    public final static int DEFAULT             = 1;
    public final static int CONFIG              = 2;
    public final static int VALIDATE            = 3;
    public final static int CHECKSUM            = 4;

    public final static String MAXWEEKS         = "MAXWEEKS";
    public final static String RETRIES          = "RETRIES";
    public final static String PASSLENGTH       = "PASSLENGTH";

     /** Property Change Support object.  */
    protected static PropertyChangeSupport m_pcsTypesMgr;
    private String m_username = WUtil.getCurrentAdmin();

    /**
     *  Constructor
     */
    public WPart11Dialog()
    {
        super(null);
        setVisible(false);

        m_pnlDisplay = new JPanel();
        m_pcsTypesMgr=new PropertyChangeSupport(this);
        JScrollPane spDisplay = new JScrollPane(m_pnlDisplay);
        m_pnlDisplay.setLayout(new WGridLayout(0, 2));
        addComp(spDisplay);
        initBlink();

        buttonPane.removeAll();
        // Add the buttons to the panel with space between buttons.
        m_btnChecksum = new JButton("Make new checksum");
        buttonPane.add(m_btnChecksum);
        buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(validateButton);
        buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(closeButton);
        buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(abandonButton);
        buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(helpButton);
        setHelpEnabled(false);

        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);
        validateButton.setActionCommand("validate");
        validateButton.addActionListener(this);
        abandonButton.setActionCommand("cancel");
        abandonButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);
        m_btnChecksum.setActionCommand("checksum");
        m_btnChecksum.addActionListener(this);

        setCloseEnabled(true);
        setAbandonEnabled(true);

        setTitle("Configuration");
        setLocation( 300, 500 );
        setResizable(true);
        setSize(450, 300);

    }

    /**
     *  Builds the components from the file and displays it.
     *  @param strFile  the file to be read.
     */
    public void build(int nType, String strFile, String strhelpfile)
    {
        m_nType = nType;
        m_strPath = (strFile != null) ? FileUtil.openPath(strFile) : "";
        m_strHelpFile = strhelpfile;
        boolean bValidate = false;
        boolean bChecksum = false;

        if (nType == CONFIG)
        {
            setTitle("Configuration");
            buildConfig();
        }
        else
        {
            JComponent compDisplay = null;
            if (nType == DEFAULT)
            {
				m_pnlAccPolicy = new AccPolicyPanel(m_strPath);
                compDisplay = m_pnlAccPolicy;
				setTitle("Password Configuration");
            }
            else if (nType == CHECKSUM)
            {
                m_pnlChecksum = new ChecksumPanel(m_strPath);
                compDisplay = m_pnlChecksum;
                setTitle("Checksum Configuration");
                bValidate = true;
                bChecksum = true;
            }
            else
            {
                setTitle("Perform System Validation");
                compDisplay = new JTextArea();
                ((JTextArea)compDisplay).setEditable(false);
                bValidate = true;
                doBlink();
            }
            m_pnlDisplay.removeAll();
            m_pnlDisplay.setLayout(new BorderLayout());
            m_pnlDisplay.add(compDisplay, BorderLayout.CENTER);
            setVisible(true);
        }
        validateButton.setVisible(bValidate);
        // abandonButton.setVisible(!bValidate);
        setAbandonEnabled(bValidate);
        m_btnChecksum.setVisible(bChecksum);
    }

    public void buildConfig()
    {
        BufferedReader in = WFileUtil.openReadFile(m_strPath);
        String strLine = null;

        if (in == null)
        {
            Messages.postError("Error opening file " + m_strPath);
            return;
        }

        try
        {
            m_pnlDisplay.removeAll();
            m_pnlDisplay.setLayout(new WGridLayout(0, 2));
            m_aListComp.clear();
            while((strLine = in.readLine()) != null)
            {
                if (strLine.startsWith("#") || strLine.startsWith("%")
                        || strLine.startsWith("@"))
                    continue;
                StringTokenizer sTokLine = new StringTokenizer(strLine, File.pathSeparator);
                createJComps(sTokLine);
            }
            setVisible(true);
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if(cmd.equals("close"))
        {
            if (m_nType == CONFIG)
                saveData();
            else if (m_nType == DEFAULT && m_pnlAccPolicy != null)
                m_pnlAccPolicy.saveData();
            setVisible(false);
            dispose();
        }
        else if (cmd.equals("validate"))
        {
            if (timer != null)
                timer.cancel();
            if (m_nType == CHECKSUM && m_pnlChecksum != null)
                m_pnlChecksum.checksumValidation();
            else
                doSysValidation();
            validateButton.setBackground(closeButton.getBackground());
        }
        else if (cmd.equals("checksum"))
        {
            String strValue = m_pnlChecksum.getChecksum();
            m_pnlChecksum.setData(strValue);
        }
        else if (cmd.equals("checksumdir"))
        {
            m_pnlChecksum.setData("");
        }
        else if (cmd.equals("cancel"))
        {
            //build(m_bAccPolicy);
            setVisible(false);
            dispose();
        }
        else if (cmd.equals("help"))
            displayHelp();
        else if (cmd.equals("switchFDA"))
        {
            Date date = new Date();
            String tm = date.toString();
            JComponent comp = null;
            String strValue = null;
            int i;

            i = 0;
            while (i < m_aListComp.size())
            {
                comp = (JComponent)m_aListComp.get(i);
                if (comp != null)
                {
                   if (comp instanceof JComboBox)
                   {
                      strValue = (String)((JComboBox)comp).getSelectedItem();
                   }
                }
                i++;
            }
            if (strValue != null)
            {
                String line = tm + " aaudit " + m_username + " data type "+
                              " set to " +strValue;
                String[] cmd2 = {WGlobal.SHTOOLCMD, "-c",
                                 "/vnmr/p11/bin/writeAaudit -l \"" + line + "\""};
                WUtil.runScriptInThread(cmd2);
            }
        }
    }

    protected void initBlink()
    {
        String blinkFrequency = null;
        delay = (blinkFrequency == null) ? 400 :
            (1000 / Integer.parseInt(blinkFrequency));
        labelString = null;
        if (labelString == null)
            labelString = "Blink";
        Font font = new java.awt.Font("TimesRoman", Font.PLAIN, 24);
        setFont(font);
    }

    /**
     *  Creates the components based on the information in each line.
     *  @param sTokLine the tokens in a line.
     *
     *  The file is of the following format:
     *  auditDir:/vnmr/part11/auditTrails:system
     *  part11Dir:/vnmr/part11/data:system
     *  part11Dir:/vnmr/part11/data:system
     *  file:standard:cmdHistory:yes
     *  file:standard:fid:yes
     *  file:standard:procpar:yes
     *  file:standard:conpar:yes
     *
     *  Based on this format, the line is parsed as follows:
     *  1) if the line starts with the keyword 'file',
     *      then it skips over the first two tokens in the line
     *      which are 'file' and 'standard', and the third token in the line
     *      is the label of the item, and the fourth token in the line corresponds
     *      to the value of the checkbox.
     *   2) if the line doesnot start with the keyword 'file',
     *      then the second and the fourth tokens are skipped,
     *      first token in the line is the label of the item,
     *      and the third token is the value of the textfield.
     */
    protected void createJComps(StringTokenizer sTokLine)
    {
        boolean bFile = false;
        boolean bMode = false;
        JComponent compValue = null;
        boolean bDir = false;

        for (int i = 0; sTokLine.hasMoreTokens(); i++)
        {
            String strTok = sTokLine.nextToken();
            if (i == 0)
            {
                bFile = strTok.equalsIgnoreCase("file") ? true : false;
                bMode  = strTok.equalsIgnoreCase("dataType") ? true : false;
                bDir = false;
                if (strTok.equalsIgnoreCase("dir"))
                    bDir = true;

                // Case 2, create a label for the first token.
                if (!bFile && !bDir)
                    createLabel(strTok, m_pnlDisplay);
            }
            else
            {
                // Case 1 => file:standard:conpar:yes
                if (bFile)
                {
                    // skip over the first tokenwhich is the keyword 'file'
                    if (i == 1)
                        continue;
                    // else check for checkbox value.
                    else if (strTok.equalsIgnoreCase("yes")
                                || strTok.equalsIgnoreCase("no"))
                        compValue = createChkBox(strTok, m_pnlDisplay);
                    // else create a label.
                    else
                        createLabel(strTok, m_pnlDisplay);
                }
                // Case 2 => part11Dir:/vnmr/part11/data:system Or dataType:Non-FDA
                else if (!bDir)
                {
                    if (i == 1)
                    {
                        // create a combobox
                        if (bMode)
                            compValue = createCombo(strTok, m_pnlDisplay);
                        // create a textfield and sets it's value.
                        else
                            compValue = createTxf(strTok, m_pnlDisplay);
                    }
                }
            }
        }
        m_aListComp.add(compValue);
    }

    /**
     *  Save the data to the file.
     *  Read the data from the file, and copy everything to the stringbuffer.
     *  The lines in the file are of the form:
     *  auditDir:system:/vnmr/part11/auditTrails:system
     *  file:standard:text:yes
     *  Based on these formats, get the value of either the directory or the
     *  checkbox from the corresponding components, and replace that value
     *  in the stringbuffer.
     */
    protected void saveData()
    {
        BufferedReader reader = WFileUtil.openReadFile(m_strPath);
        String strLine = null;
        StringBuffer sbData = new StringBuffer();
        if (reader == null)
            return;

        int nLineInd = 0;
        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                // if the line starts with a comment from sccs, add '#' to it,
                // to make it a comment
                if (strLine.startsWith("%") || strLine.startsWith("@"))
                    strLine = "# " + strLine;

                if (strLine.startsWith("#"))
                {
                    sbData.append(strLine+"\n");
                    continue;
                }
                StringTokenizer sTok = new StringTokenizer(strLine, File.pathSeparator);
                boolean bFile = false;
                boolean bLineEnd = false;
                boolean bDir = false;

                for(int i = 0; sTok.hasMoreTokens() ; i++)
                {
                    String strValue = sTok.nextToken();
                    if (i == 0)
                    {
                        bFile = strValue.equalsIgnoreCase("file") ? true : false;
                        bDir = false;
                        if (strValue.equalsIgnoreCase("dir"))
                            bDir = true;
                    }
                    else
                    {
                        // Case => file:standard:procpar:yes
                        // skip over 'standard', 'procpar',
                        // and get the value 'yes' or 'no' from the checkbox
                        if (bFile)
                        {
                            if (i == 3)
                            {
                                String strNewValue = getValue(nLineInd);
                                if (strNewValue != null)
                                    strValue = strNewValue;
                                bLineEnd = true;
                            }
                            else
                                bLineEnd = false;
                        }
                        else if (bDir)
                        {
                            bLineEnd = false;
                            if (i == 3)
                                bLineEnd = true;
                        }
                        // Case => part11Dir:/vnmr/part11/data 0r
                        // Case => dataType:FDA
                        // skip over 'part11Dir'
                        // and get the value of the directory from the textfield,
                        // or from the combobox
                        else
                        {
                            if (i == 1)
                            {
                                String strNewValue = getValue(nLineInd);
                                if (strNewValue != null)
                                    strValue = strNewValue;
                                bLineEnd = true;
                            }
                            else
                                bLineEnd = false;
                        }
                    }
                    // copy the value to the stringbuffer
                    sbData.append(strValue);
                    if (!bLineEnd)
                        sbData.append(File.pathSeparator);
                }
                nLineInd++;
                sbData.append("\n");
            }
            // write the data to the file
            reader.close();
            BufferedWriter writer = WFileUtil.openWriteFile(m_strPath);
            WFileUtil.writeAndClose(writer, sbData);
            m_objTimer = new javax.swing.Timer(200, new ActionListener()
            {
                public void actionPerformed(ActionEvent e)
                {
                    VItemAreaIF.firePropertyChng(Global.PART11_CONFIG_FILE, "all", "");
                    m_objTimer.stop();
                }
            });
            m_objTimer.start();
        }
        catch (Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }

    }

    protected void doSysValidation()
    {
        int nComps = m_pnlDisplay.getComponentCount();
        JTextArea txaMsg = null;

        for (int i = 0; i < nComps; i++)
        {
            Component comp = m_pnlDisplay.getComponent(i);
            if (comp instanceof JTextArea)
                txaMsg = (JTextArea)comp;
        }

        if (txaMsg != null)
        {
            txaMsg.append("VALIDATING SYSTEM FILES...\n");
            txaMsg.append("\n");
        }
        runScripts(txaMsg);
    }

    public void runScripts(final JTextArea txaMsg)
    {
        new Thread(new Runnable()
        {
            public void run()
            {
                String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.SYS_VNMR + "/bin/chVJlist -v"};
                txaMsg.append("Checking list of system files... \n");
                runScript(cmd, txaMsg);

                String[] cmd2 = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,  WGlobal.SUDO +
                                     WGlobal.SBIN + "chchsums " + FileUtil.SYS_VNMR + "/adm/p11/syschksm"};
                txaMsg.append("Performing checksum of files... \n");
                runScript(cmd2, txaMsg);

                String[] cmd3 = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.SYS_VNMR + "/bin/ckDaemon"};
                txaMsg.append("Checking that the deamon has started... \n");
                runScript(cmd3, txaMsg);
            }
        }).start();
    }

    /**
     *  Returns the value of the component
     *  @param nLineInd index of the line in the file.
     */
    protected String getValue(int nLineInd)
    {
        JComponent comp = null;
        String strValue = null;

        // Get the component corresponding to the line of the file from the list.
        if (nLineInd < m_aListComp.size())
        {
            comp = (JComponent)m_aListComp.get(nLineInd);
        }

        if (comp == null)
            return strValue;

        // Get the value of the component
        if (comp instanceof JTextField)
            strValue = ((JTextField)comp).getText();
        else if (comp instanceof JCheckBox)
            strValue = ((JCheckBox)comp).isSelected() ? "yes" : "no";
        else if (comp instanceof JComboBox)
            strValue = (String)((JComboBox)comp).getSelectedItem();

        // if the value is of the form: $home, then parse the value
        /*if (strValue.indexOf('$') >= 0)
        {
            String strNewValue = WFileUtil.parseValue(strValue, new HashMap());
            if (strNewValue != null && strNewValue.trim().length() > 0)
                strValue = strNewValue;
        }*/

        return strValue;
    }

    protected void doBlink()
    {
        if (timer != null)
            timer.cancel();

        timer = new java.util.Timer();
        timer.schedule(new TimerTask() {
            public void run() {WUtil.blink(validateButton);}
        }, delay, delay);
    }

    protected void runScript(String[] cmd, JTextArea txaMsg)
    {
        String strg = "";

        if (cmd == null)
            return;

        Process prcs = null;
        try
        {
            Messages.postDebug("Running script: " + cmd[2]);
            Runtime rt = Runtime.getRuntime();

            prcs = rt.exec(cmd);

            if (prcs == null)
                return;

            InputStream istrm = prcs.getInputStream();
            if (istrm == null)
                return;

            BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

            while((strg = bfr.readLine()) != null)
            {
                //System.out.println(strg);
                strg = strg.trim();
                //Messages.postDebug(strg);
                strg = strg.toLowerCase();
                if (txaMsg != null)
                {
                    txaMsg.append(strg);
                    txaMsg.append("\n");
                }
            }
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }
    }


    /**
     *  Creates a label and sets it's value.
     *  @param strText  the text of the label.
     */
    protected JLabel createLabel(String strText, JPanel pnlDisplay)
    {
        JLabel lblField = new JLabel(strText);
        pnlDisplay.add(lblField);
        return lblField;
    }

    /**
     *  Creates a checkbox and sets it's value.
     *  @param strText  either 'yes' or 'no'
     *                  if yes then set the checkbox selected.
     */
    protected JCheckBox createChkBox(String strText, JPanel pnlDisplay)
    {
        JCheckBox chkField = new JCheckBox();
        boolean bSelected = (strText.equalsIgnoreCase("yes")) ? true : false;
        chkField.setSelected(bSelected);
        pnlDisplay.add(chkField);
        return chkField;
    }

    protected JComboBox createCombo(String strText, JPanel pnlDisplay)
    {
        JComboBox cmbMode = new JComboBox(m_aStrMode);
	if(!strText.equalsIgnoreCase(m_aStrMode[0]) && 
	 	!strText.equalsIgnoreCase(m_aStrMode[1])) strText=m_aStrMode[0];	
        cmbMode.setSelectedItem(strText);
        cmbMode.setActionCommand("switchFDA");
        cmbMode.addActionListener(this);
        pnlDisplay.add(cmbMode);
        return cmbMode;
    }

    /**
     *  Creates a textfield and sets it's value.
     *  @param strText the value of the textfield.
     */
    protected JTextField createTxf(String strText, JPanel pnlDisplay)
    {
        JTextField txfField = new JTextField(strText);
        pnlDisplay.add(txfField);
        return txfField;
    }

    /**
     *  Adds a component to the contentpane.
     *  @comp   component to be added.
     */
    protected void addComp(JComponent comp)
    {
        Container contentPane = getContentPane();
        contentPane.add(comp, BorderLayout.CENTER);
    }

    //==============================================================================
   //   PropertyChange methods follows ...
   //============================================================================

    /**
     *  Adds the specified change listener.
     *  @param l    the property change listener to be added.
     */
    public static void addChangeListener(PropertyChangeListener l)
    {
        if (m_pcsTypesMgr != null)
            m_pcsTypesMgr.addPropertyChangeListener(l);
    }

    /**
     *  Adds the specified change listener with the specified property.
     *  @param strProperty  the property to which the listener should be listening to.
     *  @param l            the property change listener to be added.
     */
    public static void addChangeListener(String strProperty, PropertyChangeListener l)
    {
        if (m_pcsTypesMgr != null)
            m_pcsTypesMgr.addPropertyChangeListener(strProperty, l);
    }

    /**
     *  Removes the specified change listener.
     */
    public static void removeChangeListener(PropertyChangeListener l)
    {
        if(m_pcsTypesMgr != null)
            m_pcsTypesMgr.removePropertyChangeListener(l);
    }



    protected class AccPolicyPanel extends JPanel
    {
        public AccPolicyPanel(String strPath)
        {
            setLayout(new WGridLayout(0, 2));
            buildPanel(strPath);
        }

        protected void buildPanel(String strPath)
        {
            BufferedReader reader = WFileUtil.openReadFile(strPath);
            String strLine;

            if (reader == null)
                return;

            try
            {
                while ((strLine = reader.readLine()) != null)
                {
                    if (strLine.startsWith("#") || strLine.startsWith("%")
                        || strLine.startsWith("@"))
                        continue;

                    StringTokenizer sTokLine = new StringTokenizer(strLine, ":");

                    // first token is the label e.g. Password Length
                    if (sTokLine.hasMoreTokens())
                    {
                        createLabel(sTokLine.nextToken(), this);
                    }

                    // second token is the value
                    String strValue = sTokLine.hasMoreTokens() ? sTokLine.nextToken()
                                        : "";
                    if (strValue.equalsIgnoreCase("yes") || strValue.equalsIgnoreCase("no"))
                        createChkBox(strValue, this);
                    else
                        createTxf(strValue, this);
                }
            }
            catch (Exception e)
            {
                Messages.writeStackTrace(e);
                //e.printStackTrace();
                Messages.postDebug(e.toString());
            }

        }

        /**
         *  Saves the data in the following format:
         *  Label:value
         *  e.g. Password Retries:3
         */
        protected void saveData()
        {
            StringBuffer sbData = new StringBuffer();
            Component[] comps = getComponents();
            String strLabel = null;
            String strValue = null;
            String strCmd = null;

            for (int i = 0; i < comps.length; i++)
            {
                Component comp = comps[i];
                boolean bCmd = false;
                if (comp instanceof JLabel)
                {
                    strLabel = ((JLabel)comp).getText();
                    sbData.append(strLabel);
                    sbData.append(":");

                    // append to the cmd string
                    if (strLabel.equalsIgnoreCase("Password Retries"))
                        strCmd = RETRIES;
                    else if (strLabel.indexOf("Default Expiration") >= 0)
                        strCmd = MAXWEEKS;
                    else if (strLabel.indexOf("Minimum Length") >= 0)
                        strCmd = PASSLENGTH;
                    else
                        strCmd = "";
                }
                else if (comp instanceof JTextField)
                {
                    strValue = ((JTextField)comp).getText();
                    if (strValue == null)
                        strValue = "";
                    sbData.append(strValue);
                    sbData.append("\n");

                    // append to the cmd string
                    if ((strCmd.indexOf(RETRIES) >= 0) ||
                            (strCmd.indexOf(MAXWEEKS) >= 0) ||
                            (strCmd.indexOf(PASSLENGTH) >= 0))
                    {
                        strCmd = strCmd + "= " + strValue;
                        bCmd = true;
                    }
                }
                else if (comp instanceof JCheckBox)
                {
                    strValue = ((JCheckBox)comp).isSelected() ? "yes" : "no";
                    sbData.append(strValue);
                    sbData.append("\n");
                }

                // call the unix script that can set these values
                if (bCmd)
                {
                    String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO +
                                            WGlobal.SBIN + "auredt" + " " + strCmd};
                    WUtil.runScript(cmd);
                }

            }
            // write to the file
            BufferedWriter writer = WFileUtil.openWriteFile(m_strPath);
            WFileUtil.writeAndClose(writer, sbData);
        }

    }

    protected class ChecksumPanel extends JPanel
    {

        public ChecksumPanel(String strPath)
        {
            super();

            setLayout(new BorderLayout());
            m_cmbChecksum = new JComboBox();
            m_txaChecksum = new JTextArea();
            buildPanel(strPath);

            m_cmbPath.addActionListener(WPart11Dialog.this);
            m_cmbPath.setActionCommand("checksumdir");
            setData("");
        }

        protected void buildPanel(String strPath)
        {
            strPath = FileUtil.openPath(strPath);
            ArrayList aListPath = new ArrayList();
            BufferedReader reader = WFileUtil.openReadFile(strPath);
            if (reader == null)
                return;

            String strLine;
            try
            {
                while ((strLine = reader.readLine()) != null)
                {
                    StringTokenizer strTok = new StringTokenizer(strLine, ":");
                    if (strTok.countTokens() < 4)
                        continue;

                    boolean bChecksum = false;
                    boolean bShow = false;

                    String strDir = strTok.nextToken();
                    String strChecksum = strTok.nextToken();

                    if (strChecksum.equalsIgnoreCase("checksum"))
                        bChecksum = true;

                    if (bChecksum && (strDir.equals("file") ||
                                      strDir.equals("dir")))
                    {
                        String strValue = strTok.nextToken();
                        String strShow = strTok.nextToken();
                        if (strShow.equalsIgnoreCase("yes"))
                            bShow = true;

                        if (bShow)
                            aListPath.add(strValue);
                    }
                }

                m_cmbPath = new JComboBox(aListPath.toArray());

                JPanel pnlDisplay = new JPanel(new GridBagLayout());
                GridBagConstraints gbc = new GridBagConstraints(0, 0, 1, 1, 0.2, 0.2,
                                                                GridBagConstraints.NORTHWEST,
                                                                GridBagConstraints.HORIZONTAL,
                                                                new Insets(0,0,0,0), 0, 0);
                pnlDisplay.add(m_cmbPath, gbc);
                gbc.gridx = 1;
                pnlDisplay.add(m_cmbChecksum, gbc);
                add(pnlDisplay, BorderLayout.NORTH);
                add(m_txaChecksum, BorderLayout.CENTER);
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }

        protected String getChecksum()
        {
            String strValue = (String)m_cmbPath.getSelectedItem();
            String strChecksum = "";
            if (strValue == null || strValue.equals(""))
                return strChecksum;

            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO + WGlobal.SBIN +
                                                     "makeP11checksums " + strValue};
            WMessage msg = WUtil.runScript(cmd);
            strChecksum = msg.getMsg();
            m_txaChecksum.append("Generated Checksum: " + strChecksum + "\n");

            return strChecksum;
        }

        protected void setData(String strChecksum)
        {
            String strValue = (String)m_cmbPath.getSelectedItem();
            if (strValue == null || strValue.equals(""))
                return;

            m_cmbChecksum.removeAllItems();

            String strPath = strValue.substring(strValue.lastIndexOf("/")+1);
            strPath = UtilB.unixPathToWindows(FileUtil.SYS_VNMR + "/p11/checksums/"+strPath);
            strPath = FileUtil.openPath(strPath);
            if (strPath != null)
            {
                File file = new File(strPath);
                String[] files = file.list();
                int nSize = files.length;
                for (int i = 0; i < nSize; i++)
                {
                    String strfile = files[i];
                    m_cmbChecksum.addItem(strfile);
                }

                int nIndex = strChecksum.lastIndexOf("/");
                if (nIndex < 0)
                    nIndex = strChecksum.lastIndexOf(File.separator);
                if (nIndex+1 < strChecksum.length())
                    strChecksum = strChecksum.substring(nIndex+1);

                m_cmbChecksum.setSelectedItem(strChecksum);
            }
        }

        protected void checksumValidation()
        {
            String strValue = (String)m_cmbPath.getSelectedItem();
            int nIndex = strValue.lastIndexOf("/");
            if (nIndex < 0)
                nIndex = strValue.lastIndexOf(File.separator);
            String strPath = strValue.substring(nIndex+1);
            strPath = new StringBuffer().append(FileUtil.SYS_VNMR).append("/p11/checksums/").append(
                             strPath).append("/").append(
                             m_cmbChecksum.getSelectedItem()).toString();
            strPath = UtilB.unixPathToWindows(strPath);
            strPath = FileUtil.openPath(strPath);
            if (strPath == null)
                return;

            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, WGlobal.SUDO + WGlobal.SBIN +
                                                     "chchsums " + strPath};
            runScript(cmd, m_txaChecksum);
        }
    }

}
