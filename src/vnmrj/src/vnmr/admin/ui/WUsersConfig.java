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
import javax.swing.*;
import java.beans.*;
import java.awt.*;
import java.awt.event.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * Title: WUsersConfig
 * Description: This class provides a tool to set the user defaults/configuration
 *              for the Wanda interface.
 * Copyright:    Copyright (c) 2002
 */

public class WUsersConfig extends ModalDialog implements ActionListener
{

    /** The application interface.  */
    protected AppIF m_appIF;

    /** The panel for the display.  */
    protected JPanel m_pnlDisplay = null;

    /** The hash map that contains the user defaults.  */
    protected HashMap m_hmDef = new HashMap();

    /** The stamp when the file was modified. */
    protected long m_lastModified = 0;

    /** Property Change Support object.  */
    protected static PropertyChangeSupport m_pcsTypesMgr;

    /** The path for the file that has the user defaults. */
    protected final static String DEFFILE = "USRS/userDefaults";

    protected final static String[] m_aStrFields = {
        vnmr.util.Util.getLabel("_admin_Name"), 
        vnmr.util.Util.getLabel("_admin_Show"), 
        vnmr.util.Util.getLabel("_admin_Private"), 
        vnmr.util.Util.getLabel("_admin_Value")
    };


    public WUsersConfig(AppIF appIf)
    {
        super(null);

        m_appIF = appIf;
        m_pnlDisplay = new JPanel();
        m_pcsTypesMgr=new PropertyChangeSupport(this);

        initLayout();
        buildDlg();

        cancelButton.setActionCommand("cancel");
        cancelButton.addActionListener(this);
        okButton.setActionCommand("ok");
        okButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if(cmd.equals("ok"))
        {
            // run the script to save data
            saveData();
            setVisible(false);
            dispose();
        }
        else if (cmd.equals("cancel"))
        {
            undoAction();
            setVisible(false);
            dispose();
        }
        else if (cmd.equals("help"))
            displayHelp();
    }

    public void setVisible(boolean bShow, String strhelpfile)
    {
        m_strHelpFile = strhelpfile;
        setVisible(bShow);
    }

    /**
     *  Returns the user defaults. If the file in the system has a different
     *  lastModified stamp then the current one, then build the dialog again,
     *  which reads the file from the system, and then returns the hashmap of
     *  the user defaults.
     */
    public HashMap getDefaults()
    {
        String strPath = FileUtil.openPath(DEFFILE);
        if (strPath == null)
            return null;

        File objFile = new File(strPath);
        long lastModified = objFile.lastModified();
        if (lastModified != m_lastModified)
        {
            buildDlg();
            m_lastModified = lastModified;
        }

        return ((HashMap)m_hmDef.clone());
    }

    protected void initLayout()
    {
        String strPath = FileUtil.openPath(DEFFILE);
        if (strPath != null)
            m_lastModified = (new File(strPath)).lastModified();

        JScrollPane spDisplay = new JScrollPane(m_pnlDisplay);
        m_pnlDisplay.setLayout(new WGridLayout(0, m_aStrFields.length));
        initPnlDisplay();

        Container container = getContentPane();
        container.add(spDisplay, BorderLayout.CENTER);

        setLocation( 300, 500 );

        int nWidth = buttonPane.getWidth();
        int nHeight = buttonPane.getPreferredSize().height +
                        spDisplay.getPreferredSize().height + 100;
        setSize(600, 300);
    }

    protected void buildDlg()
    {
        // open the file to read
        BufferedReader reader = WFileUtil.openReadFile(FileUtil.openPath(DEFFILE));
        String strLine = null;
        String strKey = null;
        String strValue = "";
        String strShow = "no";
        String strPrivate = "yes";
        //String strReq =  "no";

        if(reader == null)
        {
            //System.err.println("File not found " + DEFFILE);
            Messages.postError("File not found " + DEFFILE);
            return;
        }
        try
        {
            initPnlDisplay();

            // read each line of the file
            while((strLine = reader.readLine()) != null)
            {
                if (strLine.startsWith("#") || strLine.startsWith("@"))
                    continue;

                StringTokenizer sTokLine = new StringTokenizer(strLine);

                // for each line the first thing is the keyword displayed as label,
                if (sTokLine.hasMoreTokens())
                    strKey = sTokLine.nextToken().trim();

                // and the second thing is the show condition which is either yes or no.
                if (sTokLine.hasMoreTokens())
                    strShow = sTokLine.nextToken().trim();

                // and the third thing is whether the field is private.
                if (sTokLine.hasMoreTokens())
                    strPrivate = sTokLine.nextToken().trim();

                // and the fourth thing is whether the field is a required field.
                /*if (sTokLine.hasMoreTokens())
                    strReq = sTokLine.nextToken();*/

                // and the fourth thing is the value displayed in the entry field.
                if (sTokLine.hasMoreTokens())
                    strValue = sTokLine.nextToken("\n").trim();
                else
                    strValue = "";

                createComps(strKey, strValue, strShow, strPrivate);
                m_hmDef.put(strKey, new WUserDefData(strValue, strShow, strPrivate));
            }

            reader.close();
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
    }

    protected void createComps(String strKey, String strValue,
                                String strShow, String strPrivate)
    {
        JLabel lblKeyword = new JLabel();
        JTextField txfValue = new JTextField();
        JCheckBox chkShow = new JCheckBox();
        JCheckBox chkPrivate = new JCheckBox();
        //JCheckBox chkReq = new JCheckBox();

        // first token is the keyword, displayed as the label
        lblKeyword.setText(strKey.trim());

        // second token is the show condition, displayes as the checkbox.
        if (strShow.equalsIgnoreCase("yes"))
            chkShow.setSelected(true);

        // third token is the private condition, displayed as the checkbox.
        if (strPrivate.equalsIgnoreCase("yes"))
            chkPrivate.setSelected(true);

        // fourth token is the required condition, displayed as the checkbox.
        /*if (strReq.equalsIgnoreCase("yes"))
            chkReq.setSelected(true);*/

        // fourth token is the value, displayes as the value.
        txfValue.setText(strValue.trim());

        m_pnlDisplay.add(lblKeyword);
        m_pnlDisplay.add(chkShow);
        m_pnlDisplay.add(chkPrivate);
        //m_pnlDisplay.add(chkReq);
        m_pnlDisplay.add(txfValue);
    }

    protected void saveData()
    {
        int nCompCount = m_pnlDisplay.getComponentCount();
        StringBuffer sbData = new StringBuffer();
        String strValue = null;

        // start the loop from the fourth component,
        // since comps 0-3 are labels like: Name, Show, Private, Value,
        // and are written in the comment line and does not contain any info.
        for(int i = m_aStrFields.length; i < nCompCount; i++)
        {
            Component comp = m_pnlDisplay.getComponent(i);
            if (comp instanceof JLabel)
            {
                strValue = ((JLabel)comp).getText();
                addToBuffer(sbData, strValue);
            }
            else if (comp instanceof JCheckBox)
            {
                strValue = ((JCheckBox)comp).isSelected() ? "yes" : "no";
                addToBuffer(sbData, strValue);
            }
            else if (comp instanceof JTextField)
            {
                strValue = ((JTextField)comp).getText();
                addToBuffer(sbData, strValue);
                sbData.append("\n");
            }
            //m_hmDef.put(strKey, strValue);
        }
        writeFile(sbData);
        String strPropName = WGlobal.AREA1 + WGlobal.SEPERATOR + WGlobal.SET_VISIBLE;
        m_pcsTypesMgr.firePropertyChange(strPropName, "", getDefaults());
    }

    protected void addToBuffer(StringBuffer sbData, String strValue)
    {
        strValue = (strValue != null) ? strValue.trim() : strValue;
        sbData.append(strValue);
        sbData.append('\t');
    }

    protected void writeFile(StringBuffer sbData)
    {
        String strPath = FileUtil.openPath(DEFFILE);
        if (strPath == null)
            return;

        // open the file to write
        BufferedWriter writer = WFileUtil.openWriteFile(strPath);
        if (writer == null)
        {
            Messages.postDebug("Error opening file " + DEFFILE);
            return;
        }

        try
        {
            String strComment = getComment();
            // write the comment line
            writer.write(strComment);

            // write the data
            writer.write(sbData.toString());
            writer.flush();
            writer.close();
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
    }

    protected void undoAction()
    {
        // rebuild the dialog by reading the file
        buildDlg();
    }

    private String getComment()
    {
        StringBuffer sbComm = new StringBuffer();
        sbComm.append("# ");

        for (int i = 0; i < m_aStrFields.length; i++)
        {
            sbComm.append(m_aStrFields[i]);
            sbComm.append("\t");
        }
        sbComm.append("\n");
        return sbComm.toString();
    }

    private void initPnlDisplay()
    {
        m_pnlDisplay.removeAll();

        for (int i = 0; i < m_aStrFields.length; i++)
        {
            m_pnlDisplay.add(new JLabel(m_aStrFields[i]));
        }
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

}
