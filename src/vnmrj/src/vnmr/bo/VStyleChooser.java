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

public class VStyleChooser extends JComboBox 
implements VObjIF, VObjDef, VEditIF, ActionListener,PropertyChangeListener
{ 
    public String vnmrVar = null;
    public String setVal = null;
    private static  Vector allstyles=null;
    private String  style="Plain";
    private boolean initialized=false;
    private ButtonIF vnmrIf;
    private SessionShare sshare;
    private Color  fgColor = null;
    private String fg = null;
    private MouseAdapter ml;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private boolean inChangeMode = false;
    private String type = null;
    private String fontName = null;
    private String fontStyle = null;
    private String fontSize = null;
    private String keyStr = null;
    private String vnmrCmd = null;

    private  Object[][] attributes = {
    {new Integer(VARIABLE), "Vnmr variables:    "},
    {new Integer(SETVAL),   "Value of item:  "},
    {new Integer(KEYSTR),           "Style value"},
    {new Integer(SHOW), "Enable condition:    "},
    {new Integer(CMD),  "Vnmr command:      "}};
    
    public VStyleChooser(SessionShare ss, ButtonIF vif, String typ) {
        this.sshare = ss;
        this.type = typ;
        this.vnmrIf = vif;
        addActionListener(this);
        ml = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    //if (clicks >= 2) {
                        ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                   // }
                }
             }
        };
        setOpaque(false);
        setEditable(false);

        setBgColor();
        DisplayOptions.addChangeListener(this);
    }

    public void  destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    private void setBgColor(){
        //setBackground(Util.getBgColor());
    }

    private void buildMenu(Vector list){
        setModel(new DefaultComboBoxModel(list));
    }
    private void fillMenuList() {
        if(!isShowing())
            return;
        inChangeMode=true;
        getStyles();
        setModel(new DefaultComboBoxModel(allstyles));
        if(!initialized){
            setMenuChoice(style);
        }
        inChangeMode=false;
        initialized=true;
   }

    /** return a list of style names. */
    public static Vector getStyles(){
        if(allstyles!=null)
            return allstyles;

        allstyles=new Vector();

        allstyles.add("Plain");
        allstyles.add("Bold");
        allstyles.add("Italic");
        allstyles.add("Bold-Italic");
        
        return allstyles;
    }

    public void paint(Graphics g){
       if(isShowing() && !initialized)
           fillMenuList();
       super.paint(g);
    }

    //----------------------------------------------------------------
    /** set the menu choice. */
    //----------------------------------------------------------------
    private void setMenuChoice(String c) {
        if(getItemCount()==0)
            return;
        for (int i = 0; i < getItemCount(); i++) {
            String s=(String)getItemAt(i);
            if (s.equals(c)) {
                setSelectedIndex(i);
                return;
            }
        }
    }

    public void actionPerformed(ActionEvent e) {
        if (inEditMode || inChangeMode || vnmrCmd == null)
            return;
        style=(String)getSelectedItem();
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    public void propertyChange(PropertyChangeEvent evt){
    	changeFont();
        setBgColor();
        repaint();
    }

    // VObjIF interface 

    public void changeFont() {
        Font f=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(f);
        repaint();
    }

    public String getAttribute(int attr) {
        int             k;
        String s;
        switch (attr) {
        case TYPE:
            return type;
        case CMD:
            return vnmrCmd;
        case SETVAL:
	   return setVal;
        case KEYSTR:
           return keyStr;
        case FGCOLOR:
            return fg;
        case FONT_NAME:
            return fontName;
        case FONT_STYLE:
            return fontStyle;
        case FONT_SIZE:
            return fontSize;
        case VARIABLE:
	    return vnmrVar;
        case SETCHVAL:
        case SETCHOICE:
            return null;
        case CHVAL:
        case KEYVAL:
        case VALUE:
            return style;
        default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        Vector v;
        switch (attr) {
        case CMD:
            vnmrCmd = c;
            break;
        case KEYSTR:
            keyStr=c;
            break;
        case FGCOLOR:
            fg = c;
            //fgColor=DisplayOptions.getColor(fg);
            //setForeground(fgColor);
            //repaint();
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
        case TYPE:
            type = c;
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        case SETVAL:
            setVal=c;
            break;
        case CHVAL:
        case KEYVAL:
        case VALUE:
            inChangeMode=true;
            style=c;
            setMenuChoice(c);
            inChangeMode=false;
            break;
        }
    }
    public void setValue(ParamIF pf) {
	if(pf != null || !style.equals(pf.value)) {
	    inChangeMode=true;
	    style = pf.value;
            setMenuChoice(style);
	    inChangeMode=false;
	}
    }
    public void setDefColor(String c) {
        fg = c;
        //fgColor = DisplayOptions.getColor(fg);
      //setForeground(fgColor);
    }
    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }
    public void sendVnmrCmd() {
        if (vnmrCmd == null || vnmrIf == null)
            return;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }
    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }
    public void setModalMode(boolean s) { }
    public void changeFocus(boolean s) {}
    public void setShowValue(ParamIF pf) {}
    public void updateValue(Vector params) {
        StringTokenizer tok;
        String v;
        int pnum = params.size();

        if(vnmrVar == null)
            return;
        tok = new StringTokenizer(vnmrVar, " ,\n");
        while(tok.hasMoreTokens()) {
            v = tok.nextToken();
            for(int k = 0; k < pnum; k++) {
                if(v.equals(params.elementAt(k))) {
                    updateValue();
                    return;
                }
            }
        }
    }
    public void updateValue() {
        if (vnmrIf == null)
            return;
        if (!initialized)
            fillMenuList();
        if(setVal != null && setVal.indexOf("VALUE") >= 0) {
            vnmrIf.asyncQueryParam(this, setVal, null);
        }
    }
    public void refresh() { }    
    public void setDefLoc(int x, int y) {}
    public void addDefValue(String c) { }
    public void setDefLabel(String s) { }   
    public void addDefChoice(String c) { }
    public void writeValue(PrintWriter fd, int gap) { }
    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }
    public void setEditMode(boolean s) {
        inEditMode = s;
        if (s) {
            addMouseListener(ml);
        }
        else {
            removeMouseListener(ml);
        }
    }

    public Point getDefLoc() { return getLocation(); }
    public void setSizeRatio(double x, double y) {}

    
    // VEditIF interface 
   
    public Object[][] getAttributes()  { return attributes; }   
}

