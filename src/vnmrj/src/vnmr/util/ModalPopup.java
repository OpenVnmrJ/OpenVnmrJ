/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import javax.swing.*;

import java.awt.*;
import java.awt.event.*;
import java.beans.*;
import java.io.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.templates.*;


public class ModalPopup extends ModalDialog implements ActionListener, VObjDef {

    private static ModalPopup modalPopup = null;
    private static ButtonIF currentVnmrIf;

    // these are needed for a modal dialog to talk to vnmr
    private ButtonIF vnmrIf;
    private AppIF appIf;
    private SessionShare sshare;
    //
    private JPanel panel = null;
    private JScrollPane scrollPane = null;
    private static String currentXml = "";

    protected boolean m_bRebuild = false;
    protected boolean m_pnewupdate = false;

    private static File m_file = null;
    private static long m_time = 0;

    protected int m_nWidth;
    protected int m_nHeight;
    protected String m_strXmlFile;
    protected String m_strCancel;
    protected String m_strOk;
    protected static HashMap m_hmModalpopups = new HashMap();

    public ModalPopup(SessionShare ss, ButtonIF vif, AppIF aif, String title,
            String xmlfile, int width, int height, boolean bRebuild,
            String helpFile, String cancel, String okCmd, String pnewupdate) {
        super(title);
        DisplayOptions.addChangeListener(this);

        this.vnmrIf = vif;
        this.appIf = aif;
        this.sshare = ss;
        m_bRebuild = bRebuild;
        m_strXmlFile = xmlfile;
        m_nWidth = width;
        m_nHeight = height;
        m_strCancel = cancel;
        m_strOk = okCmd;
        m_strHelpFile = helpFile;

        setTitle(title);

        if(m_strCancel != null) {
            m_strCancel = cancel.trim();
            if(m_strCancel.equals("disable")) {
                cancelButton.setEnabled(false);
                m_strCancel = "";
            }
            else if(m_strCancel.equals("hide")) {
                cancelButton.setVisible(false);
                m_strCancel = "";
            }
        }

        if(m_strOk != null) {
            if(m_strOk.equals("disable"))
                okButton.setEnabled(false);
            else if(m_strOk.equals("hide"))
                okButton.setVisible(false);
        }

        panel = new JPanel();
        panel.setLayout(new TemplateLayout());

        scrollPane = new JScrollPane(panel,
                JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);

        if(m_nWidth > 0 && m_nHeight > 0) {
            Dimension dim = new Dimension(m_nWidth, m_nHeight);
            scrollPane.setPreferredSize(dim);
        }
        Container contentPane = getContentPane();
        contentPane.add(scrollPane, BorderLayout.CENTER);
 
        if(pnewupdate != null
               && (pnewupdate.equalsIgnoreCase("true") || pnewupdate
                        .equalsIgnoreCase("yes"))){
            m_hmModalpopups.put(xmlfile, this);
            m_pnewupdate=true;
        }
       buildPanel();

        // panel will be destroyed if closed from the Window's menu.
        // when the dialog is popped again, the panel will be rebuilt
        // from the xml file.
        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent we) {
                sendCancelButtonCmd(panel);
                if(vnmrIf != null && m_strCancel != null
                    && !m_strCancel.trim().equals(""))
                    vnmrIf.sendToVnmr(m_strCancel);
                setVisible(false);
                m_hmModalpopups.clear();
                modalPopup.destroy();
            }
        });

        // Set the buttons up with Listeners
        okButton.addActionListener(this);
        okButton.setActionCommand("ok");
        cancelButton.addActionListener(this);
        cancelButton.setActionCommand("cancel");
        helpButton.addActionListener(this);
        helpButton.setActionCommand("help");

        // okButton.setEnabled(false);
        // panel.setBackground(Util.getBgColor());

        // this allows a quick confirmation popup wihtout xml file,
        // but wide enough fro the title
        if(xmlfile.length() != 0)
            pack();
        else
            setSize(width, 100);

        // do not allow a height greater than vnmrj
        Dimension fsize = getSize();
        VNMRFrame vnmrFrame = VNMRFrame.getVNMRFrame();
        Dimension vsize = vnmrFrame.getSize();
        if(fsize.height > vsize.height) {
            fsize.setSize(fsize.width, vsize.height);
            setSize(fsize);
        }

    } // constructor

    public void setVisible(boolean bVisible) {
        if(m_bRebuild && bVisible)
            buildPanel();
        super.setVisible(bVisible);
    }

    protected void buildPanel() {
        // panel for xml layout
        panel.removeAll();
        try {
            LayoutBuilder.build(panel, vnmrIf, m_strXmlFile);
        } catch(Exception e) {
            Messages.writeStackTrace(e);
        }
        updateAllValue();
        validate();
        repaint();

        // set modal mode for all the widgets in the panel.
        setModalMode(panel);
    }

    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
        // OK
        if(cmd.equals("ok")) {
            updateVnmrbg(panel);
            sendOkButtonCmd(panel);
            if(vnmrIf != null && m_strOk != null && !m_strOk.trim().equals(""))
                vnmrIf.sendToVnmr(m_strOk);
            setVisible(false);
            m_hmModalpopups.clear();
            modalPopup.destroy();
        }
        // Cancel
        else if(cmd.equals("cancel")) {
            sendCancelButtonCmd(panel);
            if(vnmrIf != null && m_strCancel != null
                    && !m_strCancel.trim().equals(""))
                vnmrIf.sendToVnmr(m_strCancel);
            setVisible(false);
            m_hmModalpopups.clear();
            modalPopup.destroy();
        }
        // Help
        else if(cmd.equals("help")) {
            /*
             * sendHelpButtonCmd(panel); Messages.postError("Help Not
             * Implemented Yet");
             */
            displayHelp();
        }
    }

    public void destroy() {
        for(int i = 0; i < panel.getComponentCount(); i++) {
            Component comp = panel.getComponent(i);
            if(comp instanceof VObjIF)
                ((VObjIF)comp).destroy();
        }
        panel.removeAll();
        scrollPane.removeAll();
        removeAll();
        panel = null;
        modalPopup = null;
        scrollPane = null;

        System.gc();
        System.runFinalization();

    }

    public void changeVnmrIf(ButtonIF vif) {
        for(int i = 0; i < panel.getComponentCount(); i++) {
            Component comp = panel.getComponent(i);
            if(comp instanceof VObjIF)
                ((VObjIF)comp).setVnmrIF(vif);
        }
    }

    private void rebuildPanel(String xmlfile) {

        for(int i = 0; i < panel.getComponentCount(); i++) {
            Component comp = panel.getComponent(i);
            if(comp instanceof VObjIF)
                ((VObjIF)comp).destroy();
        }
        panel.removeAll();
        try {
            LayoutBuilder.build(panel, vnmrIf, xmlfile);
        } catch(Exception e) {
            Messages.writeStackTrace(e);
        }
        updateAllValue();
    }

    public static ModalPopup getModalPopup(SessionShare ss, ButtonIF vif,
            AppIF aif, String title, String xmlfile, int width, int height,
            boolean bRebuild, String helpFile, String strCancel, String okCmd,
            String strPnewupdate) {

        // xmlfile is a fullpath.
        m_file = new File(xmlfile);
        long tm = 0;
        if(m_file != null)
            tm = m_file.lastModified();

        currentVnmrIf = vif;

        // no xml file ("") allows for a quick confimation popup using the title
        // but if you use more than one, better rebuild.
        if(modalPopup != null && !xmlfile.equals("")
                && xmlfile.equals(currentXml) && m_time == tm) {
            modalPopup.updateAllValue();
        } else if(modalPopup == null && m_hmModalpopups.isEmpty()) {
            modalPopup = new ModalPopup(ss, vif, aif, title, xmlfile, width,
                    height, bRebuild, helpFile, strCancel, okCmd, strPnewupdate);
        } else {
            modalPopup.setVisible(false);
            if(!m_hmModalpopups.containsValue(modalPopup))
                modalPopup.destroy();
            if(m_hmModalpopups.containsKey(xmlfile)
                    && (modalPopup = (ModalPopup)m_hmModalpopups.get(xmlfile)) != null)
            {
                if (bRebuild)
                   modalPopup.rebuildPanel(xmlfile);
                else
                   modalPopup.updateAllValue();
            }
            else
                modalPopup = new ModalPopup(ss, vif, aif, title, xmlfile,
                        width, height, bRebuild, helpFile, strCancel, okCmd,
                        strPnewupdate);

            /*
             * modalPopup.rebuildPanel(xmlfile);
             */
        }

        currentXml = xmlfile;
        m_time = tm;

        if (modalPopup.vnmrIf != vif) {
             modalPopup.changeVnmrIf(vif);
             modalPopup.updateAllValue();
        }

        return modalPopup;
    }

    public void showDialogAndSetParms(String loc) {

        // Set this dialog on top of the component passed in.
        Component comp = null;

        if(loc.equals("holdingArea"))
            comp = Util.getHoldingArea();
        if(loc.equals("shuffler"))
            comp = Util.getShuffler();
        if(loc.equals("displayPalette"))
            comp = appIf.displayPalette;
        if(loc.equals("expViewArea"))
            comp = appIf.expViewArea;
        if(loc.equals("controlPanel"))
            comp = appIf.controlPanel;
        if(loc.equals("statusBar"))
            comp = appIf.statusBar;
        if(loc.equals("pulseTool"))
            comp = appIf.pulseTool;
        if(loc.equals("topMenuBar"))
            comp = appIf.topMenuBar;
        if(loc.equals("topToolBar"))
            comp = appIf.topToolBar;
        if(comp == null)
            comp = VNMRFrame.getVNMRFrame();
        setLocationRelativeTo(comp);

        // Show the dialog and wait for the results. (Blocking call)
        showDialogWithThread();

        // If I don't do this, and the user hits a 'return' on the keyboard
        // the dialog comes visible again.
        transferFocus();
    }

    public void showDialogXYAndSetParms(String xloc, String yloc) {
        Point location;
        int x;
        try {
          x = Integer.parseInt(xloc);
        }
        catch (NumberFormatException e) {
          x = 0;
        }
        int y;
        try {
          y = Integer.parseInt(yloc);
        }
        catch (NumberFormatException e) {
          y = 0;
        }
        location = new Point(x, y);
        setLocation(location);

        // Show the dialog and wait for the results. (Blocking call)
        showDialogWithThread();

        // If I don't do this, and the user hits a 'return' on the keyboard
        // the dialog comes visible again.
        transferFocus();
    }

    private void updateGrpAllValue(JComponent p) {
        int nums = p.getComponentCount();
        for(int i = 0; i < nums; i++) {
            Component comp = p.getComponent(i);
            if(comp instanceof VObjIF) {
                if(comp instanceof VGroupIF) {
                    updateGrpAllValue((JComponent)comp);
                }
                VObjIF obj = (VObjIF)comp;
                obj.updateValue();
            }
        }
    }

    public void updateAllValue() {
        if (vnmrIf != currentVnmrIf)
            return;
        int nums = panel.getComponentCount();
        for(int i = 0; i < nums; i++) {
            Component comp = panel.getComponent(i);
            if(comp instanceof VObjIF) {
                if(comp instanceof VGroupIF) {
                    updateGrpAllValue((JComponent)comp);
                    // continue;
                }
                VObjIF obj = (VObjIF)comp;
                obj.updateValue();
            }
        }
    }

    public static void updateModalPopups(Vector vecParams) {
        Iterator iter = m_hmModalpopups.keySet().iterator();
        while(iter.hasNext()) {
            ModalPopup popup = (ModalPopup)m_hmModalpopups.get((String)iter
                    .next());
            // if(popup != null && popup.isVisible())
            if(popup != null)
                popup.updatePnewParams(vecParams);
        }
    }

    public static void updateModalPopups(ButtonIF vif, Vector vecParams) {
        Iterator iter = m_hmModalpopups.keySet().iterator();
        while(iter.hasNext()) {
            ModalPopup popup = (ModalPopup)m_hmModalpopups.get((String)iter
                    .next());
            if (popup != null) {
                if (popup.vnmrIf == vif)
                popup.updatePnewParams(vecParams);
            }
        }
    }


    protected void updatePnewParams(Vector vecParams) {
        StringTokenizer tok;
        boolean got;
        // if(vecParams == null || !isVisible())
        if (vecParams == null)
            return;
        if (!isShowing())
            return;

        int nums = panel.getComponentCount();
        for(int i = 0; i < nums; i++) {
            Component comp = panel.getComponent(i);
            if(comp instanceof VObjIF) {
                if(comp instanceof ExpListenerIF) {
                    // NB: If comp is a ExpListenerIF, it will update its
                    // children.
                    ((ExpListenerIF)comp).updateValue(vecParams);
                    continue;
                }
                VObjIF obj = (VObjIF)comp;
                String names = obj.getAttribute(VARIABLE);
                if(names == null)
                    continue;
                tok = new StringTokenizer(names, " ,\n");
                got = false;
                while(tok.hasMoreTokens()) {
                    String name = tok.nextToken();
                    int nLength = vecParams.size();
                    for(int k = 0; k < nLength; k++) {
                        String paramName = (String)vecParams.elementAt(k);
                        if(name.equals(paramName)) {
                            got = true;
                            obj.updateValue();
                            break;
                        }
                    }
                    if(got)
                        break;
                }
            }
        }
    }

    public void setModalMode(JComponent p) {

        for(int i = 0; i < p.getComponentCount(); i++) {
            Component comp = p.getComponent(i);
            if(comp instanceof VObjIF) {
                VObjIF obj = (VObjIF)comp;

                String cmd = obj.getAttribute(CMD);
                if(cmd != null){
                	if(m_pnewupdate)
                		obj.setModalMode(false);
                	else if(cmd.indexOf("Modeless") == -1)
                		obj.setModalMode(true);
                }
                if(obj instanceof VGroupIF)
                    setModalMode((JComponent)obj);
            }
        }
    }

    protected void updateVnmrbg(JComponent p) {
        for(int i = 0; i < p.getComponentCount(); i++) {
            Component comp = p.getComponent(i);
            if(comp instanceof VObjIF) {
                VObjIF obj = (VObjIF)comp;

                String cmd = obj.getAttribute(CMD);
                if(cmd != null && cmd.indexOf("Modeless") < 0
                        && cmd.indexOf("okButton") < 0
                        && cmd.indexOf("cancelButton") < 0
                        && cmd.indexOf("cancelButton") < 0) {
                    // if(comp.isVisible() && comp.isEnabled())
                    if(comp.isEnabled()) {
                        if (!(comp instanceof VButton))
                             obj.sendVnmrCmd();
                    }
                }
                if (obj instanceof VGroupIF)
                {
                    if(obj instanceof VGroup) {
                         VGroup grp = (VGroup) obj;
                         int v = grp.getShowValue();
                         if (v > 0)
                             updateVnmrbg((JComponent)obj);
                    }
                    else
                        updateVnmrbg((JComponent)obj);
                }
            }
        }
    }

    private void sendOkButtonCmd(JComponent p) {
        for(int i = 0; i < p.getComponentCount(); i++) {
            Component comp = p.getComponent(i);
            if(comp instanceof VObjIF) {
                VObjIF obj = (VObjIF)comp;

                String cmd = obj.getAttribute(CMD);
                if(cmd != null && !(cmd.indexOf("okButton") < 0)) {
                    obj.sendVnmrCmd();
                }
                if(obj instanceof VGroupIF)
                {
                    if(obj instanceof VGroup) {
                         VGroup grp = (VGroup) obj;
                         int v = grp.getShowValue();
                         if (v > 0)
                             sendOkButtonCmd((JComponent)obj);
                    }
                    else
                       sendOkButtonCmd((JComponent)obj);
                }
            }
        }
    }

    private void sendCancelButtonCmd(JComponent p) {
        for(int i = 0; i < p.getComponentCount(); i++) {
            Component comp = p.getComponent(i);
            if(comp instanceof VObjIF) {
                VObjIF obj = (VObjIF)comp;

                String cmd = obj.getAttribute(CMD);
                if(cmd != null && !(cmd.indexOf("cancelButton") < 0)) {
                    obj.sendVnmrCmd();
                }
                if(obj instanceof VGroupIF)
                {
                    if(obj instanceof VGroup) {
                         VGroup grp = (VGroup) obj;
                         int v = grp.getShowValue();
                         if (v > 0)
                             sendCancelButtonCmd((JComponent)obj);
                    }
                    else
                       sendCancelButtonCmd((JComponent)obj);
                }
            }
        }
    }

    private void sendHelpButtonCmd(JComponent p) {
        for(int i = 0; i < p.getComponentCount(); i++) {
            Component comp = p.getComponent(i);
            if(comp instanceof VObjIF) {
                VObjIF obj = (VObjIF)comp;

                String cmd = obj.getAttribute(CMD);
                if(cmd != null && !(cmd.indexOf("helpButton") < 0)) {
                    obj.sendVnmrCmd();
                }
                if(obj instanceof VGroupIF)
                    sendHelpButtonCmd((JComponent)obj);
            }
        }
    }
}
