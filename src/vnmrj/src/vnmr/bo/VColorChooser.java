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
import java.lang.reflect.Method;

import javax.swing.*;

import vnmr.util.*;
import vnmr.ui.*;

import javax.swing.border.*;
import javax.swing.plaf.basic.*;
import javax.swing.plaf.basic.BasicComboBoxUI.*;

import vnmr.admin.ui.WFontColors;

public class VColorChooser extends JPanel
    implements VObjIF,VEditIF, VObjDef, 
    DropTargetListener,ActionListener,PropertyChangeListener
{
    public static String selected_colors="black white red orange yellow blue green cyan magenta";
    public String fontStyle = null;
    public String type = null;
    public String label = null;
    public String value = null;
    public String precision = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String setVal = null;
    public String showVal = null;
    public String displayStr = null;
    public int displayVal = DisplayOptions.SYSTEM;
    public Color  fgColor = null;
    public Color  bgColor = null;
    public Font   font = null;
    protected boolean inAddMode = false;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    protected boolean bActive = true;
    protected String keyStr = null;
    protected String valStr = null;
    private Dimension psize=new Dimension(130,22);
    
    protected static Vector vnmrcolors=null;
    private static Vector selsyscolors=null;
    private static Vector systemcolors=null;
    private static Vector allcolors=null;
 
    private static Vector selsysColorValues=null;
    private static Vector systemColorValues=null;
 
    private static String SEPSTRING="-------------";
    protected String types[]=null;
    private ArrayList listeners=new ArrayList();
    private boolean inChangeMode = false;

    private boolean initialized=false;

    protected String color="black";

    protected JButton button=null;
    public MyComboBox menu=null;
    protected JLabel tbox=null;
    
    private  Object[][] attributes = {
    {new Integer(LABEL),  Util.getLabel(LABEL)},
    {new Integer(VARIABLE), Util.getLabel(VARIABLE) },
    {new Integer(SETVAL), Util.getLabel("LABELVALUE","Value of item")},
    {new Integer(KEYVAL), Util.getLabel("ccKEYVAL","Color value")},
    {new Integer(KEYSTR), Util.getLabel("ccKEYSTR","Display Variable")},
    {new Integer(CMD),	  Util.getLabel(CMD)},
    {new Integer(DISPLAY),Util.getLabel("ccDISPLAY","Color Names"),"menu",
                            (types=(Util.getAppIF() instanceof VAdminIF) ?
                                WFontColors.getShowTypes() : DisplayOptions.getShowTypes())},
    };

    private VObjIF vobj;

    public VColorChooser(SessionShare sshare, ButtonIF vif, String typ){
        this.type = typ;
        this.vnmrIf = vif;
        vobj=this;
        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    //if (clicks >= 2) {
                        ParamEditUtil.setEditObj(vobj);
                   // }
                }
             }
        };
        new DropTarget(this, this);
        setLayout(new ColorChooserLayout());
        types=(Util.getAppIF() instanceof VAdminIF) ? WFontColors.getShowTypes()
                        : DisplayOptions.getShowTypes();
        label=new String("");
        tbox=new JLabel();
        tbox.setText(label);

        add(tbox);
        menu=new MyComboBox();
        menu.setActionCommand("menu");
        
        menu.addActionListener(this);
        menu.setEditable(true);
        menu.setRenderer(new MenuListRenderer());
        add(menu);
        button=new JButton();
        button.setIcon(new VRectIcon(10, 10));
        button.setBorder(new VButtonBorder());
        button.setActionCommand("button");
        button.addActionListener(this);
        button.setEnabled(false);
        button.setOpaque(false);
 
        add(button);
        setPreferredSize(new Dimension(120,22));
        setOpaque(false);
        changeFont();
        DisplayOptions.addChangeListener(this);
   }

    public VColorChooser(){
        this((SessionShare)null,(ButtonIF)null,"colorchooser");
    }

    public void  destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    private void setFgColor(){
        String dstr = DisplayOptions.getOption(keyStr);
        if(dstr != null && !color.equals(dstr)) {
            setColor(dstr);
        }
        repaint();
    }

    // PropertyChangeListener interface
    public void propertyChange(PropertyChangeEvent evt) {
        String dstr = DisplayOptions.getOption(keyStr);
        if(dstr != null && !color.equals(dstr)) {
            setColor(dstr);
            changeFont();
        }
        repaint();
    }

    public void setActive(boolean b) {
        bActive = b;
        if (button != null) {
           if (displayVal == DisplayOptions.SELSYS)
               button.setEnabled(false);
           else
               button.setEnabled(b);
        }
        if (menu != null) {
           menu.setEnabled(b);
        }
    }

    public void setEnabled(boolean b) {
        if (button != null)
           button.setEnabled(b);
//        if (menu != null)
//           menu.setEnabled(b);
    }

    protected void buildMenu(Vector list){
        menu.setModel(new DefaultComboBoxModel(list));
    }

    private void fillMenuList() {
        if(!isShowing())
            return;
        inChangeMode=true;
        switch(displayVal){
        case DisplayOptions.SYSTEM:
             buildMenu(getSystem());
             if (bActive)
                 button.setEnabled(true);
             break;
        case DisplayOptions.SELSYS:
             buildMenu(getSelected());
             button.setEnabled(false);
             break;
        case DisplayOptions.VNMR:
             buildMenu(getVnmr());
             if (bActive)
                 button.setEnabled(true);
             break;
         case DisplayOptions.ALL:
             if (bActive)
                 button.setEnabled(true);
            buildMenu(getAll());
             break;
         }
         setMenuChoice(color);
         initialized=true;
         inChangeMode=false;
    }

    //----------------------------------------------------------------
    /** set the menu choice. */
    //----------------------------------------------------------------
    protected void setMenuChoice(String c) {
        setColor(c);
        for (int i = 0; i < menu.getItemCount(); i++) {
            String s=(String)menu.getItemAt(i);
            if (s.equals(c)) {
                menu.setSelectedIndex(i);
                return;
            }
        }
    }

    /** add an ActionListener */
    public void addActionListener(ActionListener a){
        if(a instanceof ActionListener)
            listeners.add(a);
    }

    /** return a list of System and DisplayOption names. */
    public static Vector getAll(){
        if(allcolors!=null)
            return allcolors;

        allcolors=getVnmr();
        allcolors.add(SEPSTRING);
        allcolors.addAll(getSystem());
        return allcolors;
    }

    /** return a list of System names. */
    public static Vector getSystem(){
        if(systemcolors !=null)
            return systemcolors;

        systemcolors=new Vector();
        systemColorValues=new Vector();
        String  s = VnmrRgb.getColorName(0);
        int i=0;
        while (s != null) {
            systemcolors.add(s);
            Color c=VnmrRgb.getColorByIndex(i);
            i++;
            systemColorValues.add(c);
            s = VnmrRgb.getColorName(1);
        }
        return systemcolors;
    }

    /** return a list of selected colors names
     *  list = black white red orange yellow blue green cyan magenta
     */
    public static Vector getSelected(){
        if(selsyscolors!=null)
            return selsyscolors;

        selsyscolors=new Vector();
        selsysColorValues=new Vector();
        StringTokenizer tok=new StringTokenizer(selected_colors, " ");
        while(tok.hasMoreTokens()){
        	String name=tok.nextToken();
            selsyscolors.add(name);
            Color c=VnmrRgb.getColorByName(name);
            selsysColorValues.add(c);
        }
        return selsyscolors;
    }

    /** return a list of DisplayOption names. */
    public static Vector getVnmr(){
        if(vnmrcolors !=null)
            return vnmrcolors;
        vnmrcolors=new Vector(DisplayOptions.getTypes(DisplayOptions.COLOR));
        return vnmrcolors;
    }

    public void setPreferredSize(Dimension d) {
        super.setPreferredSize(d);
        psize=d;
        setSize(psize);
    }

    public Dimension getPreferredSize() {
        return psize;
    }

    public String colorToString(Color c){
    	String s;
		if(DisplayOptions.outputHex())
	   		s = DisplayOptions.colorToHexString(c);
		else
	   		s = DisplayOptions.colorToRGBString(c);
		return s;
    }

    public void setColor(String s){
    	 boolean wasChanging=inChangeMode;
         inChangeMode=true;
         Color c=DisplayOptions.getColor(s);
         color=s;
         menu.setSelectedItem(s);
         ((VRectIcon)button.getIcon()).setColor(c);
         if(!wasChanging)
        	 inChangeMode=false;
    }
    
    protected Color getIconColor() {
        return ((VRectIcon)button.getIcon()).getColor();
    }

    public void paint(Graphics g){
       if(!initialized && !inChangeMode)
           fillMenuList();
       super.paint(g);
    }

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
        if (showVal != null) {
            vnmrIf.asyncQueryShow(this, showVal);
        }
        else if(setVal != null && setVal.indexOf("VALUE") >= 0) {
            vnmrIf.asyncQueryParam(this, setVal, precision);
        }
    }
    // VObjIF interface

    public void setEditStatus(boolean s) {isEditing = s;}
    public void setEditMode(boolean s) {
		if (s) {
         	addMouseListener(ml);
         	tbox.addMouseListener(ml);
         	button.addMouseListener(ml);
         	menu.addMouseListener(ml);
		}
		else {
           	removeMouseListener(ml);
         	tbox.removeMouseListener(ml);
         	button.removeMouseListener(ml);
         	menu.removeMouseListener(ml);
		}
        inEditMode = s;
    }
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void setDefLabel(String s) {}
    public void setDefColor(String s) {fg=s;}
    public void setDefLoc(int x, int y) {}
    public void refresh() {}
    public void setValue(ParamIF pf) {
        if(pf != null) {
            value = pf.value;
            if(value.contains(",")) {
                // color = DisplayOptions.RGBToHexString(value);
                color = VnmrRgb.getNameByRGB(value);
                setColor(color);
                repaint();
            } else {
                color = value;
                setColor(color);
                repaint();
            }
        }
    }

    public void setShowValue(ParamIF p) {
        if (p == null || p.value == null)
            return;
        String s = p.value.trim();
        int isActive = Integer.parseInt(s);
        if (setVal != null && setVal.indexOf("VALUE") >= 0)
            vnmrIf.asyncQueryParam(this, setVal, precision);
        if (isActive > 0)
            setActive(true);
        else
            setActive(false);
    }

    public void changeFocus(boolean s) {
    	isFocused = s;
    }
    public ButtonIF getVnmrIF() {return vnmrIf;}
    public void setVnmrIF(ButtonIF vif) {vnmrIf=vif;}

    public void changeFont() {
        Font font = DisplayOptions.getFont(fontStyle);
        if(font !=null){
           tbox.setFont(font); 
           menu.setFont(font);
        }
        repaint();
    }

    public String getAttribute(int attr) {
        switch (attr) {
        case FONT_STYLE:
            return fontStyle;
        case TYPE:
            return type;
        case LABEL:
            return label;
        case SETVAL:
            return setVal;
        case KEYSTR:
            return keyStr;
        case VALUES:
            return value;
        case VALUE:
        case KEYVAL:
            return color;
        case ICON:
            return null;
        case FGCOLOR:
            return fg;
        case BGCOLOR:
            return bg;
        case DISPLAY:
            return displayStr;
        case CMD:
            return vnmrCmd;
        case VARIABLE:
            return vnmrVar;
        case SAVEKIDS:
            return "false";
        case SHOW:
            return showVal;
        }
        return null;
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
        default:
            break;
        case TYPE:
            type = c;
            break;
        case LABEL:
        	label=c;
            tbox.setText(label);
            break;
        case FONT_STYLE:
            fontStyle = c;
            break;
        case ICON:
            break;
        case VALUE:
        case KEYVAL:
        	 boolean wasChanging=inChangeMode;
            inChangeMode=true;
            color=DisplayOptions.formatColorString(c);
            setMenuChoice(c);
            if(!wasChanging)
            	inChangeMode=false;
            break;
        case SETVAL:
            setVal=c;
            break;
        case KEYSTR:
    	    if(c!=null && c.indexOf("VALUE") == -1) 
	        	keyStr=c;
	    else keyStr = null;
           
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
           // menu.setForeground(fgColor);
            repaint();
            break;
        case BGCOLOR:
            bg = c;
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
            displayStr=types[i];
            if(!initialized || i!=displayVal){
                displayVal=i;
            	fillMenuList();
            }
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case SHOW:
            showVal = c;
            break;
       }
        
    }

   // DropTargetListener interface

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop

    // VEditIF interface

    public Object[][] getAttributes()  { return attributes; }
    
    // ActionListener interface

    public void actionPerformed(ActionEvent  e) {
        if(inEditMode && !ParamEditUtil.isEditObj(this))
        	ParamEditUtil.setEditObj(this);
        if(inChangeMode /*|| inEditMode*/ )
            return;
        String cmd=e.getActionCommand();
        if(cmd.equals("button")){
            Color bcol = getIconColor();
            Color newColor = JColorChooser.showDialog(this, Util.getLabel("ccChooseColor"), bcol);
            if (newColor != null) {
            	String s;
            	if(DisplayOptions.outputHex())
               		s = DisplayOptions.colorToHexString(newColor);
            	else
               		s = DisplayOptions.colorToRGBString(newColor);
                setColor(s);
                repaint();
                sendVnmrCmd();
            }
        }
        else if (cmd.equals("menu")){
            Object obj=menu.getSelectedItem();
            if(obj !=null){
            	inChangeMode=true;
            	String s=menu.getSelectedItem().toString();
            	setColor(s);
    	        sendVnmrCmd(); // note: this may cause an updateUI event
     	        inChangeMode=false;            		
                button.repaint();
            }
        }

        //if(!inEditMode)
        for(int i=0;i<listeners.size();i++){
            ActionListener a=(ActionListener)(listeners.get(i));
            a.actionPerformed(e);
        }
    }

	public class MyComboBox extends JComboBox {
		// This seems to fix a null pointer exception that is thrown in
		// BasicComboBoxUI.actionPerformed when SwingUtilities.updateComponentTreeUI
		// is called after editing the text entry field in the menu 
	    public void updateUI(){
	    	if(!inChangeMode)
	    		super.updateUI();
	    }		
	}

    class MenuListRenderer extends  BasicComboBoxRenderer {
        public Component getListCellRendererComponent(
            JList list,
            Object value,
            int index,
            boolean isSelected,
            boolean cellHasFocus)
        {
        	super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
            if (value != null)
                setText(value.toString());
            Color c=Util.getBgColor();
            switch(displayVal){
            case DisplayOptions.SYSTEM:
            	if(index>=0)
            		c=(Color)systemColorValues.get(index);
            	setBackground(c);
            	setForeground(Util.getContrastingColor(c));
                break;
            case DisplayOptions.SELSYS:
            	if(index>=0)
            		c=(Color)selsysColorValues.get(index);
            	setBackground(c);
            	setForeground(Util.getContrastingColor(c));
                break;
            case DisplayOptions.VNMR:
            case DisplayOptions.ALL:
            	c=Util.getBgColor();
            	setBackground(c);
            	setForeground(Util.getContrastingColor(c));
                break;
            }
            return this;
        }
    }

    class ColorChooserLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}
        public void removeLayoutComponent(Component comp) {}
        public Dimension preferredLayoutSize(Container target) {
            return  psize; // unused
        } // preferredLayoutSize()

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(130, 20); // unused
        } // minimumLayoutSize()
        public void layoutContainer(Container target) {
           synchronized (target.getTreeLock()) {
               int x=psize.width;
               int y=psize.height;
               int mw=110; // menu width
               int bw=22;  // button width
               
               if(label !=null && label.length()>0){
                   button.setBounds(x-bw, 0, bw, y);
                   menu.setBounds(x-mw-bw, 0, mw, y);
             	   tbox.setBounds(0, 0, x-mw, y);
               }
               else{
                   menu.setBounds(0, 0, x-bw, y);
                   button.setBounds(x-bw, 0, bw, y);
               }
           }
       }
    } // ColorChooserLayout

    public void setModalMode(boolean s) {}

    public void sendVnmrCmd() {
        if (inEditMode || vnmrIf == null || vnmrCmd == null)
            return;

        if (vnmrCmd.indexOf("VALUE") >= 0) {
            Color c = DisplayOptions.getColor(color);
            if (c == null) {
                value = "null";
            } else {
                int red = c.getRed();
                int grn = c.getGreen();
                int blu = c.getBlue();
                value = red + "," + grn + "," + blu;
            }
        }
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    public void setSizeRatio(double rx, double ry) {}

    public Point getDefLoc() {
    return getLocation();
    }

}

