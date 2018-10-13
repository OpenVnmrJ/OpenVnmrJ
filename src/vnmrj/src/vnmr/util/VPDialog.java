/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.templates.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  unascribed
 *
 */

public class VPDialog extends ModelessPopup implements ActionListener
{

	protected static VPDialog m_vpPopup;
	protected static long m_time;
	protected static String m_strXmlFile;
	protected AppIF m_appIF;
	protected ArrayList m_aListLabel = new ArrayList();
	protected VjToolBar m_toolbar;
	protected static String m_strCmd;
	protected static ArrayList m_aListRdGeometry;
	protected static final String[] m_aStrGeometry = {"1x1", "1x2", "1x4", "1x5",
													    "1x6", "1x7", "1x8", "1x9",
														"2x1", "2x2", "2x3", "2x4",
														"3x1", "3x2", "3x3",
														"4x1", "4x2", "5x1", "6x1",
														"7x1", "8x1", "9x1"};
	protected static final String[] m_aStrLabel = {"1", "2", "3", "4", "5", "6",
													"7", "8", "9"};

    public VPDialog(SessionShare ss, ButtonIF vif, AppIF aif, String title,
                    String xmlfile, int width, int height, boolean bRebuild,
                    String helpFile, String closeCmd)
	{
		super(ss, vif, aif, title, xmlfile, width, height, bRebuild, helpFile, closeCmd);

		m_aListRdGeometry = new ArrayList();
		m_toolbar = aif.topToolBar;

		//initializeList(panel);
		// closeButton.setEnabled(true);
                setCloseEnabled(true);
		//closeButton.addActionListener(this);

		addWindowListener(new WindowAdapter()
		{
			public void windowClosing(WindowEvent e)
			{
			    if (m_toolbar != null)
				{
					m_toolbar.setVpEditMode(false);
				}
			}

			public void windowClosed(WindowEvent e)
			{
				if (m_toolbar != null)
				{
					m_toolbar.setVpEditMode(false);
				}
			}

			public void windowOpened(WindowEvent e)
			{
			    if (m_toolbar != null)
				{
					m_toolbar.setVpEditMode(true);
				}
			}
		});

		addComponentListener(new ComponentAdapter()
		{
		    public void componentShown(ComponentEvent e)
			{
			    if (m_toolbar != null)
					m_toolbar.setVpEditMode(true);
			}

			public void componentHidden(ComponentEvent e)
			{
			    if (m_toolbar != null)
					m_toolbar.setVpEditMode(false);
			}
		});
    }

	public void actionPerformed(ActionEvent e)
	{
	    String cmd = e.getActionCommand();

		if (cmd.equalsIgnoreCase("close"))
		{
		        if(m_closeCmd.length() > 0) vnmrIf.sendToVnmr(m_closeCmd);
			updateVnmrbg(panel);
			saveVPBtns();
		    //executeGeometryCmd();
            setVisible(false);
		}
        else
            super.actionPerformed(e);
	}

	public void buildPanel()
	{
	    super.buildPanel();
		setModalMode(panel, false);
	}

    public void setDefaultVp()
    {
        String strPath = FileUtil.openPath("USER/INTERFACE/"+m_toolbar.getVpfile());
        if (strPath != null)
        {
            File objFile = new File(strPath);
            if (objFile.exists())
                objFile.delete();
        }
        m_toolbar.resetToVpTools();
    }

    protected void saveVPBtns()
    {
        m_aListLabel.clear();
        saveVP(panel);
        // VjToolBar toolbar = ((ExperimentIF)Util.getAppIF()).getToolBar();
	VjToolBar toolbar = Util.getToolBar();
        if (toolbar != null) {
		toolbar.addVP(m_aListLabel);
		toolbar.saveVpChanges();
        }
    }

	/**
	 *  Saves the list of the labels for the viewport buttons and saves it's attributes.
	 */
    protected void saveVP(JComponent compPanel)
    {
        int nComp = compPanel.getComponentCount();
        Component comp;
        String strLabel;
        String strVpNum;
        int nVp;
        String strComp;

        for(int i = 0; i < nComp; i++)
        {
            comp = compPanel.getComponent(i);
            if (comp instanceof VGroup)
                saveVP((JComponent)comp);
            else if (comp instanceof VEntry)
            {
                VEntry objEntry = (VEntry)comp;
		if (!objEntry.isEnabled())
                     continue;

                String strKey = objEntry.getAttribute(VObjDef.KEYWORD);
				strLabel = objEntry.getText();
				// saves the label of the viewport buttons, and saves the attributes
				// of each button
                if (strKey != null && strKey.trim().indexOf(Global.VIEWPORT) >= 0
					    && strLabel != null)
				{
					strVpNum = strKey.substring(2, strKey.length());
					strLabel = strLabel.trim();
                    strComp = ">=";
                    if (strVpNum != null && strVpNum.equals("1"))
                        strComp = ">";
                    HashMapAttrs hmAttrs = new HashMapAttrs(strVpNum);
                    hmAttrs.put(new Integer(VObjDef.ICON), strLabel);
					hmAttrs.put(new Integer(VObjDef.SUBTYPE), Global.VIEWPORT+strVpNum);
                 //  hmAttrs.put(new Integer(VObjDef.CMD), "M@vplayout"+strVpNum);
                    hmAttrs.put(new Integer(VObjDef.CMD), "vnmrjcmd('vpactive "+strVpNum+"')");
					hmAttrs.put(new Integer(VObjDef.SETVAL), "$VALUE=jviewportlabel["+
                                                              strVpNum+"]");
                    hmAttrs.put(new Integer(VObjDef.SHOW), "if jviewports[1]"+strComp+
                                                           strVpNum+" then $VALUE=1 " +
                                                           "else $VALUE=0 endif");
					hmAttrs.put(new Integer(VObjDef.VARIABLE), "jviewports jviewportlabel");
					m_aListLabel.add(hmAttrs);
				}
            }
        }
        Collections.sort(m_aListLabel);
	}

	/**
	 *  Saves the window geometry command, so that it could be executed when
	 *  the dialog is closed.
	 */
	protected void saveRadioCmd(ActionEvent e)
	{
	    VRadio rdGeometry = (VRadio)e.getSource();
		m_strCmd = rdGeometry.getAttribute(CMD);
	}

	protected void executeGeometryCmd()
	{
		appIf.sendCmdToVnmr(m_strCmd);
	}

	public static VPDialog getModalVpPopup(SessionShare ss, ButtonIF vif, AppIF aif,
                                           String title, String xmlfile, int width,
                                           int height, boolean bRebuild, String helpFile,String closeCmd) {

        //xmlfile is a fullpath.
        File file = new File(xmlfile);
        long tm = 0;
        if(file != null)
			tm = file.lastModified();

        if(m_vpPopup != null && xmlfile.equals(m_strXmlFile) && m_time == tm) {
            m_vpPopup.updateAllValue();
        } else if(m_vpPopup == null) {
            m_vpPopup = new VPDialog(ss, vif, aif, title, xmlfile, width, height, bRebuild, helpFile, closeCmd);
        } else {

            m_vpPopup.setVisible(false);
            m_vpPopup.destroy();
            m_vpPopup = new VPDialog(ss, vif, aif, title, xmlfile, width, height, bRebuild, helpFile, closeCmd);
        }

		m_strXmlFile = xmlfile;
        m_time = tm;

		return m_vpPopup;
	}

	public void setModalMode(JComponent p)
	{
	    setModalMode(p, true);
	}

	public void setModalMode(JComponent p, boolean bMode) {

        for (int i=0; i<p.getComponentCount(); i++) {
            Component comp = p.getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF obj = (VObjIF) comp;

				String cmd=obj.getAttribute(CMD);
                if(cmd!=null && cmd.indexOf("Modal") == -1)
				    obj.setModalMode(bMode);

				if (obj instanceof VGroup)
				{
					if (cmd != null && cmd.indexOf("Modal") >= 0)
					    setModalMode((JComponent)obj, true);
					else
						setModalMode((JComponent)obj, false);
				}
             }
        }
    }

	private void updateVnmrbg(JComponent p) {
        for (int i=0; i<p.getComponentCount(); i++) {
            Component comp = p.getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF obj = (VObjIF) comp;

                String cmd=obj.getAttribute(CMD);
                if(cmd!=null && cmd.indexOf("Modal") >= 0
					&& cmd.indexOf("closeButton") < 0
                    && cmd.indexOf("abandonButton") < 0) {
				    //obj.sendVnmrCmd();
				    if (obj instanceof VGroupIF)
					    sendCmd((JComponent)obj);
			    }
				if(obj instanceof VGroupIF)
				    updateVnmrbg((JComponent)obj);
            }
        }
    }

	/**
	 *  Sends the commands for the window geometry.
	 */
	private void sendCmd(JComponent p)
	{
	    int nCount = p.getComponentCount();
		for (int i = 0; i < nCount; i++)
		{
		    Component comp = p.getComponent(i);
			if (comp instanceof VObjIF)
			{
				VObjIF obj = (VObjIF)comp;
				if (((JComponent)obj).isEnabled())
				{
					String cmd=obj.getAttribute(CMD);
				    if (((obj instanceof JRadioButton &&
                          ((JRadioButton)obj).isSelected() &&
                          ((JRadioButton)obj).getParent().isVisible()) ||
                         (obj instanceof JCheckBox)) &&
                        cmd != null)
					{
					    obj.sendVnmrCmd();
                        //System.out.println("the cmd is " + cmd);
					}
				}
			}
		}
	}

    class HashMapAttrs extends HashMap implements Comparable
    {
        protected Integer nVp;

        public HashMapAttrs(String strVp)
        {
            nVp = new Integer(strVp);
            if (nVp == null)
                nVp = new Integer(0);
        }

        public int compareTo(Object obj)
        {
            return nVp.compareTo(((HashMapAttrs)obj).nVp);
        }
    }
}
