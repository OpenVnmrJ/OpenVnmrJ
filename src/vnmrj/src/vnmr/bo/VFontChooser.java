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

public class VFontChooser extends JComboBox 
implements VObjIF, VObjDef, VEditIF, ActionListener,PropertyChangeListener
{ 
    public static String selected_fonts="Dialog DialogInput Monospaced Serif SansSerif Courier Palatino";
    private static String SEPSTRING="-------------";
    public String type = null;
    public String vnmrVar = null;
    public String setVal = null;
	
    private static String[] types=new String[4];
	private static  Vector sysfonts=null;
	private static  Vector selsys=null;
	private static  Vector allfonts=null;
	private static  Vector vnmrfonts=null;
    private String  font="Dialog";
    private String  displayStr=null;
    private int displayVal = DisplayOptions.SELSYS;
    private boolean initialized=false;
    private ButtonIF vnmrIf;
    private SessionShare sshare;
    private Color  fgColor = null;
    private Color  bgColor;
    private String fg = null;
    //private String bg = "transparent";
    private String bg = null;
    private MouseAdapter ml;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private boolean inChangeMode = false;
    private String fontName = null;
    private String fontStyle = null;
    private String fontSize = null;
    private String keyStr = null;
    private String vnmrCmd = null;
    
    private  Object[][] attributes = {
	{new Integer(VARIABLE),		Util.getLabel(VARIABLE)},
	{new Integer(SETVAL),		Util.getLabel(SETVAL)},
	{new Integer(KEYSTR),		"Font value"},
	{new Integer(SHOW),			Util.getLabel(SHOW)},
	{new Integer(CMD),			Util.getLabel(CMD)},
	{new Integer(DISPLAY),Util.getLabel("fcDISPLAY"),"menu",(types=DisplayOptions.getShowTypes())},};
	 
    public VFontChooser(SessionShare ss, ButtonIF vif, String typ) {
    	this.sshare = ss;
    	this.type = typ;
    	this.vnmrIf = vif;
    	bgColor = null;
    	setOpaque(false);
    	addActionListener(this);
   		ml = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    //if (clicks >= 2) {
                        ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                    //}
                }
             }
        };
        //setOpaque(true);
        setEditable(false);

       // setBgColor();
        DisplayOptions.addChangeListener(this);
    }

    public void  destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void propertyChange(PropertyChangeEvent evt){
    	changeFont();
        setBgColor();
        repaint();
    }

    private void setBgColor(){
        setBackground(Util.getBgColor());
    }

	private void buildMenu(Vector list){
		setModel(new DefaultComboBoxModel(list));
	}
    private void fillMenuList() {
        if(!isShowing())
            return;
        inChangeMode=true; 
       	switch(displayVal){
	    case DisplayOptions.SYSTEM:
		     buildMenu(getSystem());
			 break;
	    case DisplayOptions.SELSYS:
		     buildMenu(getSelected());
			 break;
	    case DisplayOptions.VNMR:
		     buildMenu(getVnmr());
			 break;
	     case DisplayOptions.ALL:
		     buildMenu(getAll());
			 break;
	     }
       	 if(!initialized){
			 setMenuChoice(font);
		 }

	     initialized=true;
         inChangeMode=false;
   }

    /** return a list of System and DisplayOption names. */
	public static Vector getAll(){
		if(allfonts!=null)
			return allfonts;

   		allfonts=getVnmr();
		
		allfonts.add(SEPSTRING);
		allfonts.addAll(getSystem());

		return allfonts;
	}

    /** return a list of System names. */
	public static Vector getSystem(){
		if(sysfonts!=null)
			return sysfonts;
		
		sysfonts=new Vector();	
       	GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
	    Hashtable hash=new Hashtable();
    
	    String[] flist = ge.getAvailableFontFamilyNames();
	    if(flist == null)
	    	return getSelected();
	    else {
		    for (int k = 0; k < flist.length; k++) {
		        if (flist[k].length() > 0){
                    StringTokenizer tok=new StringTokenizer(flist[k]," \t.");
				    if(tok.hasMoreTokens()){
				        String str=tok.nextToken();
				        String test=str.toUpperCase();
				        if(test.length()>13)
				        	test=test.substring(0,13);
				        if(hash.containsKey(test))
				           continue;
				        hash.put(test,test);
				        sysfonts.add(str);
			        }
		        }
	        }
	    }
	    return sysfonts;
	}

    /** return a list of DisplayOption names. */
	public static Vector getVnmr(){
		if(vnmrfonts!=null)
			return vnmrfonts;

        vnmrfonts=new Vector(DisplayOptions.getTypes(DisplayOptions.FONT));
		return vnmrfonts;
	}

    /** return a default list of selected font names 
     *  list = Dialog DialogInput Monospaced Serif SansSerif Courier Palatino
     */
	public static Vector getSelected(){
		if(selsys!=null)
			return selsys;
		selsys=new Vector();
		StringTokenizer tok=new StringTokenizer(selected_fonts," ");
		while(tok.hasMoreTokens()){
			selsys.add(tok.nextToken());
        }
		return selsys;
	}

    public void paint(Graphics g){
       if(!initialized)
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
        font=(String)getSelectedItem();
        if (inEditMode || inChangeMode || vnmrCmd == null)
            return;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
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
			return font;
        case DISPLAY:
        	return displayStr;
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
            fgColor=DisplayOptions.getColor(fg);
            //setForeground(fgColor);
            repaint();
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
        	font=c;
        	setMenuChoice(c);
        	inChangeMode=false;
            break;
        case DISPLAY:
        	int i=0;
       		if(c!=null){
        		for(i=0;i<types.length;i++){
        			if(types[i].equals(c))
        		   	break;
        		}
        		if(i>=types.length)
        			break;
        	}
        	displayVal=i;
        	displayStr=types[displayVal];
       	    fillMenuList();
          	break;
        }
    }
    public void setValue(ParamIF pf) {
        if(pf != null && !font.equals(pf.value)) {
	    inChangeMode=true;
	    font = pf.value;
	    setMenuChoice(font);
	    inChangeMode=false;
	}
    }

    public void setDefColor(String c) {
        fg = c;
        fgColor = DisplayOptions.getColor(fg);
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
		if (s) {
         	addMouseListener(ml);
		}
		else {
           	removeMouseListener(ml);
		}
        inEditMode = s;
    }
    
 	// VEditIF interface 
   
    public Object[][] getAttributes()  { return attributes; }	    

    public void setSizeRatio(double w, double h) {}
    public Point getDefLoc() { return getLocation(); }

}

