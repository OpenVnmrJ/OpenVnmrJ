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
import javax.swing.table.TableColumn;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.part11.*;
import vnmr.templates.*;

public class VComboFileTable extends ComboFileTable
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
    private boolean tableValid=false;
    private boolean tableUpdate=false;
    private boolean debug=false;
    private File m_objFile;
    private long m_lTime = 0;
    private FileMenu m_menu;
    private FileTable m_table;
    private FileTableModel m_tableModel;

    public VComboFileTable(SessionShare ss, ButtonIF vif, String typ) {
        this.sshare = ss;
        this.type = typ;
        this.vnmrIf = vif;
    orgBg = getBackground();
        bgColor = Util.getBgColor();
        setOpaque(false);
        setBackground(bgColor);

    m_menu = getFileMenu();
        m_table = getTable();

    m_table.setBackground(Color.white);

        ml = new MouseAdapter() {
        public void mouseClicked(MouseEvent evt) {
        if(debug)
                System.out.println("VComboFileTable.mouseClicked "+vnmrCmd);

                int clicks = evt.getClickCount();

                if(inModalMode || vnmrIf == null)
                    return;
                if (inAddMode ||inChangeMode ||  inEditMode)
                    return;
                if (isActive < 0)
                    return;

                if (clicks >= 2) {
            sendValueCmd();
        } else if (clicks == 1) {
            setValueCmd(m_table.getSelectedRow());
        }
             }
        };
    m_table.addMouseListener(ml);

    m_menu.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {

                JComboBox cb = (JComboBox)e.getSource();
                String path = (String)cb.getSelectedItem();

                updateFileTable();
            }
        });

        new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
    }

    public String getRowValue(int row) {
        return((String)m_tableModel.getRowValue(row));
    }

    public Vector getOutputTypes() {
        return(m_tableModel.getOutputTypes());
    }

    public boolean isCellEditable(int row, int column) {
            return false;
    }

    private void setValueCmd(int row) {

    if(fileType.equals("cmdHistory")) vnmrCmd = getRowValue(row);
    else if(fileType.equals("records")) vnmrCmd = "rt('" + getRowValue(row) + "')";
    else vnmrCmd = null;

    }

    private boolean getComboTable(String file) {

        if(inAddMode)
            return false;

// assuming if file starts with "/", it is a full path.
// otherwise append file to USER/PERSISTENCE/

        String str = file;
        if(!file.startsWith("/")) str = FileUtil.savePath("USER/PERSISTENCE/"+file);

    String label = null;
    if(fileType.equals("comdHistory")) label = "Select a type";
    else label = "Select a path";

    Vector paths = new Vector();
    paths.addElement(str);

    removeAll();
    makeComboFileTable(paths, label, 0);

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

    public void setEditMode(boolean s) { }

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
        k = m_table.getSelectedRow();
            return (String)getRowValue(k);
        case VALUE:
        k = m_table.getSelectedRow();
            return (String)getRowValue(k);
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
        tableValid=false;
            break;
        case PANEL_TYPE:
            fileType = c;
        tableValid=false;
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
        tableUpdate=true;
        int k = m_table.getSelectedRow();
            setValueCmd(k);
            break;
        case SETVAL:
            setVal = c;
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case VALUE:
            //inChangeMode = true;
        k = m_table.getSelectedRow();
            setValueCmd(k);
            //inChangeMode = false;
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
        System.out.println("VComboFileTable.sendContentQuery "+fileName);
    if (fileName != null)
       vnmrIf.asyncQueryParamNamed("content", this, fileName);
    else {
       setVisible(false);
    }
    }

    private void  sendValueQuery(){
    if(debug)
        System.out.println("VComboFileTable.sendValueQuery");
    if (setVal != null)
        vnmrIf.asyncQueryParam(this, setVal);
    }

    private void  sendShowQuery(){
    if(debug)
        System.out.println("VComboFileTable.sendShowQuery");
    if (showVal != null)
            vnmrIf.asyncQueryShow(this, showVal);
    }

    private void  sendValueCmd(){
    if(debug)
        System.out.println("VComboFileTable.sendValueCmd "+vnmrCmd);
    if (vnmrCmd != null)
            vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    // ExpListenerIF interface

    public void  updateValue(Vector params){
    if (vnmrIf == null)
        return;
    if(debug)
        System.out.println("VComboFileTable.updateValue "+params);
    if(tableUpdate)
        updateContent(params);
        String vars=getAttribute(VARIABLE);
    if (vars == null) return;
    StringTokenizer tok=new StringTokenizer(vars, " ,\n");
        while (tok.hasMoreTokens()) {
            String var = tok.nextToken();
            for (int k = 0; k < params.size(); k++) {
                if (var.equals(params.elementAt(k))) {
            if(debug)
            System.out.println("VComboFileTable.updateValue "+var);
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
            System.out.println("VComboFileTable.updateContent "+var);
                    tableValid=false;
            if(fileExpr)
            sendContentQuery();
            else {
                getComboTable(fileName);
            }
                }
            }
        }
    }

    public void updateValue() {
        if (vnmrIf == null || inAddMode || inEditMode)
            return;
    if(debug)
        System.out.println("VComboFileTable.updateValue");
        if(fileName != null && !tableValid){
            if(fileExpr)
                sendContentQuery();
            else {
        getComboTable(fileName);
        }
        }
    else if (!inEditMode) {
      if (m_table.getRowCount() <= 0)
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
        System.out.println("VComboFileTable.setValue "+pf.value+" "+pf.name);

        if(pf.name.equals("content")){
            tableValid=false;
        getComboTable(pf.value);
    }

    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
        isActive = Integer.parseInt(s);

        if(debug)
        System.out.println("VComboFileTable.setShowValue "+pf.value+" "+pf.name);

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
        if(!tableValid && !tableUpdate && !fileExpr) {
        getComboTable(fileName);
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
        tableValid = false;
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
        System.out.println("VComboFileTable.sendVnmrCmd");
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

    public void setSizeRatio(double w, double h) {}

    public Point getDefLoc() {
    return getLocation();
    }

}

