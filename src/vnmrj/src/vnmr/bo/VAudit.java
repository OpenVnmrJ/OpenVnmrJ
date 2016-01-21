/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.util.*;
import java.beans.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.part11.*;
import vnmr.templates.*;

public class VAudit extends JPanel
    implements VObjIF, VObjDef, DropTargetListener, ExpListenerIF,
    PropertyChangeListener
{
    private String type = null;
    private String fileName = null;
    private String fileType="file";
    private String fg = null;
    private String bg = null;
    private String selVars = null;
    private String vnmrCmd = null;
    private String vnmrCmd2 = null;
    private String showVal = null;
    private String setVal = null;
    private String fontName = null;
    private String fontStyle = null;
    private String fontSize = null;
    private Color  fgColor = null;
    private Color  bgColor, orgBg;
    private Font   font = null;
    private String keyStr = null;
    private MouseAdapter ml;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private boolean inChangeMode = false;
    private boolean inAddMode = false;
    private int isActive = 1;
    private ButtonIF vnmrIf;
    private SessionShare sshare;
    private boolean inModalMode = false;
    private boolean fileExpr = false;
    private boolean auditValid=false;
    private boolean auditUpdate=false;
    private boolean debug=false;
    private File m_objFile;
    private long m_lTime = 0;
    private vnmr.part11.Audit m_audit = null;
    private Point defLoc = new Point(0, 0);

    public VAudit(SessionShare ss, ButtonIF vif, String typ) {
        this.sshare = ss;
        this.type = typ;
        this.vnmrIf = vif;
    orgBg = getBackground();
        bgColor = Util.getBgColor();
        setOpaque(false);
        setBackground(bgColor);

        m_audit = new vnmr.part11.Audit();
    add(m_audit);

        new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
    }

    private boolean getAudit(String file) {

        if(m_audit == null)
            return false;

        if(inAddMode)
            return false;


// assuming if file starts with "/", it is a full path.
// otherwise append file to USER/PERSISTENCE/

    String str = file;
    if(!file.startsWith("/")) str = FileUtil.savePath("USER/PERSISTENCE/"+file);

    String filepath =
         FileUtil.savePath("USER/PERSISTENCE/tmpAudit");

// if m_audit valid, get current menu and table selections

    // for upper panel
    int locMselect = -2;
    int locTablemax = -2;
    int locTablemin = -2;

    JPanel loc = m_audit.getLocator();
    int nums = loc.getComponentCount();
    for (int i = 0; i < nums; i++) {
        Component comp = loc.getComponent(i);
        if (comp instanceof ComboFileTable) { 
	    ComboFileTable locCombo = (ComboFileTable) comp;
	    locMselect = locCombo.getFileMenu().getSelectedIndex();
	    int locRows = locCombo.getTable().getSelectedRowCount();
	    int[] locTselect = locCombo.getTable().getSelectedRows();
	    locTablemin = locCombo.getTable().getRowCount();
    	    for(int j=0; j<locRows; j++) {
	  	if(locTselect[j] > locTablemax) locTablemax = locTselect[j];
		if(locTselect[j] < locTablemin) locTablemin = locTselect[j];
    	    }
	}
    }

    //for loer panel
    vnmr.part11.ComboFileTable auditCombo = m_audit.getAudit();
    int auditMselect = auditCombo.getFileMenu().getSelectedIndex();
    int auditRows = auditCombo.getTable().getSelectedRowCount();
    int[] auditTselect = auditCombo.getTable().getSelectedRows();
    int auditTablemax = -2;
    int auditTablemin = auditCombo.getTable().getRowCount();
    for(int i=0; i<auditRows; i++) {
	if(auditTselect[i] > auditTablemax) auditTablemax = auditTselect[i];
	if(auditTselect[i] < auditTablemin) auditTablemin = auditTselect[i];
    }
	
        Vector paths1 = new Vector();
    paths1.addElement(str);
    String label1 = "Select a path";

        Vector paths2 = new Vector();
    paths2.addElement(filepath);
    String label2 = "Select a type";

    m_audit.makeAudit(paths1, label1, paths2, label2, true);

    loc = m_audit.getLocator();
    nums = loc.getComponentCount();
    for (int i = 0; i < nums; i++) {
        Component comp = loc.getComponent(i);
        if (comp instanceof ComboFileTable) { 
	    ComboFileTable locCombo = (ComboFileTable) comp;
	    if(locMselect >= 0)
    	    locCombo.getFileMenu().setSelectedIndex(locMselect);
	    if(locTablemin >= 0 && locTablemax >= 0)
    	    locCombo.getTable().setRowSelectionInterval(locTablemin, locTablemax);
	}
    }

    auditCombo = m_audit.getAudit();
    if(auditMselect >= 0)
    auditCombo.getFileMenu().setSelectedIndex(auditMselect);
    if(auditTablemin >= 0 && auditTablemax >= 0)
    auditCombo.getTable().setRowSelectionInterval(auditTablemin, auditTablemax);

        return(true);

    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
    fgColor=DisplayOptions.getColor(fg);
        setForeground(fgColor);
    changeFont();
    bgColor = Util.getBgColor();
    setBackground(bgColor);
    }

    // VObjIF interface

    public void setDefLabel(String s) {
    }

    public void setDefColor(String c) {
        this.fg = c;
        fgColor = VnmrRgb.getColorByName(c);
        setForeground(fgColor);
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {}

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        repaint();
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    public String getAttribute(int attr) {
        int             k;
        String s;
        switch (attr) {
        case TYPE:
            return type;
        case KEYSTR:
            return keyStr;
        case PANEL_FILE:
            return fileName;
        case PANEL_TYPE:
            return fileType;
        case FGCOLOR:
            return fg;
        case BGCOLOR:
            return bg;
        case SHOW:
            return showVal;
        case FONT_NAME:
            return fontName;
        case FONT_STYLE:
            return fontStyle;
        case FONT_SIZE:
            return fontSize;
        case CMD:
            return vnmrCmd;
        case SETVAL:
            return setVal;
        case VARIABLE:
            return selVars;
    case VAR2:
            return vnmrCmd;
        case VALUE:
            return vnmrCmd;
         default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        Vector v;
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case PANEL_FILE:
            fileName = c;
        if (c != null) {
               if(c.startsWith("$VALUE") || c.startsWith("if"))
                  fileExpr=true;
        }
        auditValid=false;
            break;
        case PANEL_TYPE:
            fileType = c;
        auditValid=false;
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
            repaint();
            break;
        case BGCOLOR:
            bg = c;
            if (c == null || c.length()==0 || c.equals("default"))
        bgColor = Util.getBgColor();
            else
                bgColor = DisplayOptions.getColor(c);
            setBackground(bgColor);
            repaint();
            break;
        case SHOW:
            showVal = c;
            break;
        case FONT_NAME:
            fontName = c;
            break;
        case FONT_STYLE:
            fontStyle = c;
            break;
        case FONT_SIZE:
            fontSize = c;
            break;
        case VARIABLE:
            selVars = c;
            break;
    case VAR2:
            auditUpdate=true;
            break;
        case SETVAL:
            setVal = c;
            break;
        case CMD:
            vnmrCmd = c;
            break;
    case VALUE:
            break;
        }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    private void  sendContentQuery(){
    if(debug)
        Messages.postDebug("VAudit.sendContentQuery "+fileName);
    if (fileName != null)
       vnmrIf.asyncQueryParamNamed("content", this, fileName);
    else {
       setVisible(false);
    }
    }

    private void  sendValueQuery(){
    if(debug)
        Messages.postDebug("VAudit.sendValueQuery");
    if (setVal != null)
        vnmrIf.asyncQueryParam(this, setVal);
    }

    private void  sendShowQuery(){
    if(debug)
        Messages.postDebug("VAudit.sendShowQuery");
    if (showVal != null)
            vnmrIf.asyncQueryShow(this, showVal);
    }

    private void  sendValueCmd(){
    if(debug)
        Messages.postDebug("VAudit.sendValueCmd "+vnmrCmd);
    if (vnmrCmd != null)
            vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    // ExpListenerIF interface

    public void  updateValue(Vector params){
    if (vnmrIf == null)
        return;
    if(debug)
        Messages.postDebug("VAudit.updateValue "+params);
    if(auditUpdate)
        updateContent(params);
        String vars=getAttribute(VARIABLE);
    if (vars == null) return;
    StringTokenizer tok=new StringTokenizer(vars, " ,\n");
        while (tok.hasMoreTokens()) {
            String var = tok.nextToken();
            for (int k = 0; k < params.size(); k++) {
                if (var.equals(params.elementAt(k))) {
            if(debug)
                Messages.postDebug("VAudit.updateValue "+var);
                    if (showVal != null)
                        sendShowQuery();
                    else if (setVal != null)
                        sendValueQuery();
                    return;
                }
            }
        }
    }

    private void  updateContent(Vector params){
        if (vnmrIf == null || fileName == null)
        return;
        String  vars=getAttribute(VAR2);
    if (vars == null) return;
        StringTokenizer tok=new StringTokenizer(vars, " ,\n");
        while (tok.hasMoreTokens()) {
            String var = tok.nextToken();
            for (int k = 0; k < params.size(); k++) {
                if (var.equals(params.elementAt(k))) {
            if(debug)
                Messages.postDebug("VAudit.updateContent "+var);
                    auditValid=false;
            if(fileExpr)
            sendContentQuery();
            else {
                getAudit(fileName);
            }
                }
            }
        }
    }

    public void updateValue() {
        if (vnmrIf == null || inAddMode || inEditMode)
            return;
    if(debug)
        Messages.postDebug("VAudit.updateValue");
        if(fileName != null && !auditValid){
            if(fileExpr)
                sendContentQuery();
            else {
        getAudit(fileName);
        }
        }
        else if (!inEditMode) {
          if (m_audit == null)
             setVisible(false);
          else
             setVisible(true);
        }

        if (showVal != null)
            sendShowQuery();
        else if (setVal != null)
            sendValueQuery();
    }


    public void setValue(ParamIF pf) {

        if(pf == null || pf.value == null)
            return;
    if(debug)
        Messages.postDebug("VAudit.setValue "+pf.value+" "+pf.name);

        if(pf.name.equals("content")){
            auditValid=false;
        getAudit(pf.value);
    }

    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
        isActive = Integer.parseInt(s);

        if(debug)
            Messages.postDebug("VAudit.setShowValue "+pf.value+" "+pf.name);

            if (isActive > 0)
                setBackground(bgColor);
            else {
                if (isActive == 0)
                    setBackground(Global.IDLECOLOR);
                else
                    setBackground(Global.NPCOLOR);
            }
            if (isActive >= 0) {
                setEnabled(true);
                if (setVal != null)
            sendValueQuery();
            }
            else
                setEnabled(false);
        }
    }


    public void paint(Graphics g) {
        super.paint(g);

    // Check if the file has been modified, and if so then get the current version.
    getCurrentFile();

    if (!isEditing)
            return;
        if(!auditValid && !auditUpdate && !fileExpr) {
        getAudit(fileName);
    }

        Dimension  psize = getPreferredSize();
        if (isFocused)
            g.setColor(Color.yellow);
        else
            g.setColor(Color.green);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }

    /**
     *  Checks if the file has been modified by comparing the modify stamp.
     */
    private void getCurrentFile()
    {
    if (m_objFile != null)
    {
        long lCurrTime = m_objFile.lastModified();
        if (m_lTime != lCurrTime)
        {
        auditValid = false;
        fileExpr = false;
        if (fileName != null) {
           if (fileName.startsWith("$VALUE") || fileName.startsWith("if"))
               fileExpr = true;
        }
            updateValue();
        }
    }
    }

    public void refresh() {}

    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void addDefChoice(String c) {}
    public void addDefValue(String c) {}

    public void itemStateChanged(ItemEvent e){}

    public void setDefLoc(int x, int y) {}
    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
    VObjDropHandler.processDrop(e, this, inEditMode);
    }

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if (vnmrCmd == null || vnmrIf == null)
            return;
    if(debug)
        Messages.postDebug("VAudit.sendVnmrCmd");
        sendValueCmd();
    }

    private final static String[] m_types = {"records", "s_auditTrailFiles",
    "cmdHistory", "s_auditTrail", "d_auditTrail" };

    private final static Object[][] attributes = {
    {new Integer(VARIABLE),		"Selection variables:"},
    {new Integer(VAR2),		    "Content variables:"},
    {new Integer(SETVAL),		"Value of item:"},
    {new Integer(SHOW),		    "Enable condition:"},
    {new Integer(CMD),			"Vnmr command:"},
    {new Integer(PANEL_FILE),	"Table source:"},
    {new Integer(PANEL_TYPE),	"Table type:",m_types},
    };
    public Object[][] getAttributes()  { return attributes; }

    public Point getDefLoc() { return defLoc; }
    public void setSizeRatio(double x, double y) {}
}

