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

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.templates.*;

public class ModelessPopup extends ModelessDialog
                      implements ActionListener, VObjDef {

// these are needed for a modal dialog to talk to vnmr
    protected ButtonIF vnmrIf;
    protected AppIF appIf;
    protected SessionShare sshare;
//
    protected JPanel            panel=null;
    private JScrollPane       scrollPane=null;

    protected boolean m_bRebuild = false;
    protected boolean m_bNewXml = false;
    protected int m_nWidth;
    private String m_strXmlFile;
    protected String m_closeCmd="";
    protected int m_nHeight;
    private long  m_XmlDate = 0;

    public ModelessPopup(Frame owner,SessionShare ss, ButtonIF vif, AppIF aif, String title,
                         String xmlfile, int width, int height, boolean bRebuild,
                         String helpFile, String closeCmd) {
        super(owner, title);
        buildUi(ss,vif,aif,xmlfile,width,height,bRebuild, helpFile,closeCmd);
    }

    public ModelessPopup(SessionShare ss, ButtonIF vif, AppIF aif, String title,
                         String xmlfile, int width, int height, boolean bRebuild,
                         String helpFile, String closeCmd) {
        super(title);
        buildUi(ss,vif,aif,xmlfile,width,height,bRebuild, helpFile,closeCmd);
    }

    private void buildUi(SessionShare ss, ButtonIF vif, AppIF aif,
                         String xmlfile, int width, int height, boolean bRebuild,
                         String helpFile, String closeCmd) {
        this.vnmrIf = vif;
        this.appIf = aif;
        this.sshare = ss;
        m_bRebuild = bRebuild;
        m_strXmlFile = xmlfile;
        m_nWidth = width;
        m_nHeight = height;
        m_strHelpFile = helpFile;
 	m_closeCmd=closeCmd;

        LinuxFocusBug.workAroundFocusBugInWindow(this);

        // panel for xml layout
        panel=new JPanel();
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
        buildPanel();

        // panel will be destroyed if closed from the Window's menu.
        // when the dialog is popped again, the panel will be rebuilt
        // from the xml file.
        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent we) {
	      if(m_closeCmd.length() > 0) vnmrIf.sendToVnmr(m_closeCmd); 
              setVisible(false);
            //destroy();
            }
        });

        // Set the buttons and the text item up with Listeners
        historyButton.setActionCommand("history");
        historyButton.addActionListener(this);
        undoButton.setActionCommand("undo");
        undoButton.addActionListener(this);
        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);
        abandonButton.setActionCommand("abandon");
        abandonButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);
        historyButton.setEnabled(false);
        undoButton.setEnabled(false);
        abandonButton.setEnabled(false);
        //helpButton.setEnabled(false);

        pack();

    } // constructor

    public void setVisible(boolean bVisible)
    {
        boolean bUpdate = false;

        if (!isVisible() && bVisible)
            bUpdate = true;
        if (m_bRebuild && bVisible) {
            buildPanel();
            if (m_bNewXml)
                bUpdate = true;  // new content
        }

        if (bUpdate)
            updateAllValue();
        super.setVisible(bVisible);
    }

    public void setAppIF(AppIF aif)
    {
        appIf = aif;
    }

    public synchronized void setVnmrIF(ButtonIF vif)
    {
        VObjIF vobj;
        Component comp;
        int i;
        int nSize = getComponentCount();
        vnmrIf = vif;
        for (i = 0; i < nSize; i++) {
            comp = getComponent(i);
            if (comp instanceof VObjIF) {
                vobj = (VObjIF) comp;
                vobj.setVnmrIF(vif);
            }
        }
        if (panel == null)
            return;
        nSize = panel.getComponentCount();
        for (i = 0; i < nSize; i++) {
            comp = panel.getComponent(i);
            if (comp instanceof VObjIF) {
                vobj = (VObjIF) comp;
                vobj.setVnmrIF(vif);
            }
        }
    }

    public void buildPanel()
    {
        UNFile file = new UNFile(m_strXmlFile);
        m_bNewXml = false;
        if (file == null || (!file.exists()))
           return;

        if (file.lastModified() == m_XmlDate) {
           return;
        }

        m_XmlDate = file.lastModified();
        panel.removeAll();
        try {
            LayoutBuilder.build(panel,vnmrIf,m_strXmlFile);
        } catch(Exception e) {
            Messages.writeStackTrace(e);
            return;
        }
        m_bNewXml = true;

        String fName = file.getName();
        String pName;
        int index = fName.indexOf('.');
        if (index > 1)
             pName = fName.substring(0, index) + " Dialog";
        else
             pName = fName + " Dialog";
        setName(pName);

        // updateAllValue();
    }

    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
        // OK
        if(cmd.equals("history")) {
            // Do not set setVisible(false) for history, it brings up a menu.

        }
        else if(cmd.equals("undo")) {
            undoButton.setEnabled(false);
        }
        else if(cmd.equals("close")) {
	    if(m_closeCmd.length() > 0) vnmrIf.sendToVnmr(m_closeCmd); 
            setVisible(false);
        }
        else if(cmd.equals("abandon")) {
            setVisible(false);
        }
        // Help
        else if(cmd.equals("help")) {
            // Do not call setVisible(false);  That will cause
            // the Block to release and the code which create
            // this object will try to use userText.  This way
            // the panel stays up and the Block stays in effect.
            //Messages.postError("Help Not Implemented Yet");
            displayHelp();
        }

    }

    public void destroy() {
        for (int i=0; i<panel.getComponentCount(); i++) {
            Component comp = panel.getComponent(i);
            if (comp instanceof VObjIF) ((VObjIF)comp).destroy();
        }
    panel.removeAll();
        System.gc();
        System.runFinalization();
    }

    public void showDialogAndSetParms(String loc) {

        // Set this dialog on top of the component passed in.

    Component comp = VNMRFrame.getVNMRFrame();

    if (loc.equals("holdingArea")) comp = Util.getHoldingArea();
    else if (loc.equals("shuffler")) comp = Util.getShuffler();
    else if (loc.equals("displayPalette")) comp = appIf.displayPalette;
    else if (loc.equals("expViewArea")) comp = appIf.expViewArea;
    else if (loc.equals("controlPanel")) comp = appIf.controlPanel;
    else if (loc.equals("statusBar")) comp = appIf.statusBar;
    else if (loc.equals("pulseTool")) comp = appIf.pulseTool;
    else if (loc.equals("topMenuBar")) comp = appIf.topMenuBar;
    else if (loc.equals("topToolBar")) comp = appIf.topToolBar;
    if(comp != null) {
        Point location;
        if ("".equals(loc)) {
            int x = comp.getX() + comp.getWidth() / 2 - getWidth() / 2;
            int y = comp.getY() + comp.getHeight() / 2 - getHeight() / 2;
            location = new Point(x, y);
        } else {
            location = comp.getLocation();
        }
        setLocation(location);
    }

        // Show the dialog and wait for the results. (Blocking call)
        //showDialogWithThread();

    setVisible(true);

        // If I don't do this, and the user hits a 'return' on the keyboard
        // the dialog comes visible again.
        transferFocus();
    }

   /**********
    private void updateGrpAllValue (JComponent p) {
        int nums = p.getComponentCount();
        for (int i = 0; i < nums; i++) {
            Component comp = p.getComponent(i);
            if (comp instanceof VObjIF) {
                if (comp instanceof VGroupIF) {
                    updateGrpAllValue((JComponent) comp);
                }
                VObjIF obj = (VObjIF) comp;
                obj.updateValue();
            }
        }
    }
   **********/

    public void updateAllValue (){
        int nums = panel.getComponentCount();
        for (int i = 0; i < nums; i++) {
            Component comp = panel.getComponent(i);
            if (comp instanceof VObjIF) {
/*
                if (comp instanceof VGroupIF) {
                    updateGrpAllValue((JComponent) comp);
                    //continue;
                }
*/
                VObjIF obj = (VObjIF) comp;
                obj.updateValue();
            }
        }
    }

    public void updatePnewParams(Vector<String> paramVector) {

     StringTokenizer tok;
     boolean         got;

     if(this == null || paramVector == null) return;
        if(!this.isVisible()) return;

     int nums = panel.getComponentCount();
     for (int i = 0; i < nums; i++) {
       Component comp = panel.getComponent(i);
       if (comp instanceof VObjIF) {
           if (comp instanceof ExpListenerIF) {
                // NB: If comp is a ExpListenerIF, it will update its children.
                    ((ExpListenerIF)comp).updateValue(paramVector);
                    continue;
           }
           VObjIF obj = (VObjIF) comp;
           String names = obj.getAttribute(VARIABLE);
           if(names == null) continue;
           tok = new StringTokenizer(names, " ,\n");
           got = false;
           while (tok.hasMoreTokens()) {
              String name = tok.nextToken();
              for (int k = 0; k < paramVector.size(); k++) {
                String paramName = (String) paramVector.elementAt(k);
                    if(name.equals(paramName)) {
            		got = true;
            		obj.updateValue();
            		break;
            	    }
              }
              if (got) break;
           }
        }
     }
    }

}
