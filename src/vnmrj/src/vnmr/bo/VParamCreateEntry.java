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
import java.awt.event.*;
import java.awt.dnd.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.ui.shuf.*;
import vnmr.templates.*;
import javax.swing.border.Border;

/**
 * VParamCreateEntry class is used by by SpinCAD & JPsg for grouping various
 * parameter information to enable Vnmr ro create those new parameters.
 * It is essentially similar to VGroup class, but with some getAttributes()
 * modified.
 *
 */

public class VParamCreateEntry extends JPanel
  implements VObjIF, VEditIF, VObjDef, VGroupIF, ExpListenerIF,
   DropTargetListener, StatusListenerIF, PropertyChangeListener
{
    public String reference = null;
    public String type = null;
    public String title = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String vnmrCmd2 = null;
    public String showVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public Color  fgColor = Color.blue;
    public Color  bgColor=null;
    public Font   font = null;
    private String borderType = null;
    private String titlePosition = null;
    private String titleJustification = null;
    private String tab = "no";
    private String useRef="no";
    private String showCmd=null;
    private String hideCmd=null;

    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private boolean bNeedUpdate = true;
    private int    width = 0;
    private int    height = 0;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private Vector compList = null;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);

    public boolean showing = false;

    public VParamCreateEntry(SessionShare sshare, ButtonIF vif, String typ) {
		super();
		this.type = typ;
		this.vnmrIf = vif;
		//this.fg = "black";
		//this.fontSize = "8";
		//this.fontStyle = "Plain";
		this.tab = "no";
		setOpaque(false);	// overlapping groups allowed
		setLayout(new vGrpLayout());
 		setBackground(null);
        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                    }
                }
            }
		};
	addComponentListener(new ComponentListener() {
            public void componentResized(ComponentEvent e) {}

            public void componentMoved(ComponentEvent e) {}

            public void componentShown(ComponentEvent e) {
                updateChildren(true);
            }

            public void componentHidden(ComponentEvent e) {}

        });

/***
	addHierarchyListener(new HierarchyListener() {
	    public void hierarchyChanged(HierarchyEvent e) {
		if (e.getChangeFlags() == HierarchyEvent.SHOWING_CHANGED)
		    updateChildren(true);
	    }
	});
**/
	new DropTarget(this, this);
    	DisplayOptions.addChangeListener(this);
    }

    public boolean isTab(){
        return tab.equals("yes");
    }

    public boolean isTabGroup() {
        if(tab.equals("yes") && getAttribute(LABEL)!=null)
            return true;
        return false;
    }

    private ParamLayout getParamLayout(){
        Container cont = (Container)getParent();
        while (cont != null) {
           if (cont instanceof ParamLayout)
               break;
           cont = cont.getParent();
        }
        return (ParamLayout) cont;
    }

    // PropertyChangeListener interface

	public void propertyChange(PropertyChangeEvent evt){
		if(fg!=null){
            fgColor=DisplayOptions.getColor(fg);
        	setForeground(fgColor);
        }
		if(bg!=null && bg.length()>0 && !bg.equals("transparent") && !bg.equals("default")){
            bgColor=DisplayOptions.getColor(bg);
        	setBackground(bgColor);
        }
        else
        	setBackground(null);
 		changeFont();
     }

	// StatusListenerIF interface
	public void updateStatus(String msg){
		for (int i=0; i<getComponentCount(); i++) {
		    Component comp = getComponent(i);
		    if (comp instanceof StatusListenerIF)
				((StatusListenerIF)comp).updateStatus(msg);
		}
	}

    // ExpListenerIF interface

    public void  updateChildren(boolean visibleOnly) {
	if (!bNeedUpdate)
	    return;
	if (!isShowing() && visibleOnly)
	    return;
	bNeedUpdate = true;
	for (int i=0; i<getComponentCount(); i++) {
	    Component comp = getComponent(i);
	    if (comp instanceof VObjIF) {
	        if (comp instanceof VParamCreateEntry) {
		    ((VParamCreateEntry)comp).updateChildren(true);
		}
		else
		    ((VObjIF)comp).updateValue();
	    }
	}
    }

    /** called from ExpPanel (pnew)   */

    public void  updateValue(Vector params) {
        String          vars, v;

	if (!isShowing()) {
	    bNeedUpdate = true;
	}
	if(showVal != null && hasVariable(this,params) != null)
	{
	    vnmrIf.asyncQueryShow(this, showVal);
	}

	if (!isShowing()) {
            for (int i = 0; i < getComponentCount(); i++) {
            	Component comp = getComponent(i);
            	if (comp instanceof VParamCreateEntry) {
                    ((VParamCreateEntry)comp).updateValue(params);
		}
	    }
	    return;
	}

	bNeedUpdate = true;
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF obj = (VObjIF) comp;
		String vname;
                String varstring;
                if (obj instanceof ExpListenerIF) {
                    ((ExpListenerIF)obj).updateValue(params);
                } else if ((varstring=obj.getAttribute(VARIABLE)) != null &&
                           (vname=hasVariable(varstring, params)) != null)
                {
		    // If SHOW attribute is empty, and there is only
		    // one name in the obj's varstring, and the SETVAL
		    // string is "$VALUE=vname", and we have the
		    // parameter value, just set obj to the parameter
		    // value.
		    String wantExpr = "$VALUE=" + vname;
		    int k = params.indexOf(vname);
		    String setval = obj.getAttribute(SETVAL);
		    String shoval = obj.getAttribute(SHOW);
		    int pnum = params.size() / 2;
		    String val;
		    if (varstring.trim().equals(vname) &&
                        k >= 0 && k < pnum &&
			(shoval==null || shoval.trim().length()==0) &&
			equalsIgnoreSpaces(setval, wantExpr) &&
			!(val=(String)params.elementAt(k+pnum)).equals("\033"))
		    {
			obj.setValue(new ParamIF("", "", val));
		    } else {
			obj.updateValue();
		    }
		}
	    }
	}
    }

    /**
     * Checks if str1 is the same as str2, ignoring any extra spaces in str1.
     */
    private boolean equalsIgnoreSpaces(String str1, String str2) {
	if (str1 == null) {
	    return false;
	}
	int i, j;
	for (i=j=0; i<str1.length(); ++i) {
	    char c;
	    if ((c=str1.charAt(i)) != ' ') { // Skip spaces
		if(j>=str2.length())
		    return false;
		if (c != str2.charAt(j)) {
		    return false;
		}
		++j;
	    }
	}
	return true;
    }

    public void group_show() {
	if(!isShowing() && showCmd !=null && showCmd.length()>0){
	    vnmrIf.sendVnmrCmd(this, showCmd);
	    //System.out.println("show "+showCmd);
	}
	setVisible(true);
    }

    public void group_hide(){
	if(isShowing() && hideCmd !=null && hideCmd.length()>0){
	    vnmrIf.sendVnmrCmd(this, hideCmd);
	    //System.out.println("hide "+hideCmd);
	}
	setVisible(false);
    }

    public void update(Vector params){
        updateValue(params);
/*
	if(showVal != null && hasVariable(this,params) != null)
	    vnmrIf.asyncQueryShow(this, showVal);
*/
	//updateObject(this,params);
    }

    /**
	 * Returns the first variable in the VARIABLE list of the
	 * VObjIF "obj" that matches one of the strings in the Vector
	 * "params".
	 * Returns null if there is no match.
	 */
    private String hasVariable(VObjIF obj, Vector params){
	String  vars=obj.getAttribute(VARIABLE);
	if (vars == null)
	    return null;
	StringTokenizer tok=new StringTokenizer(vars, " \t,\n");
	while (tok.hasMoreTokens()) {
	    String var = tok.nextToken().trim();
	    for (int k = 0; k < params.size(); k++){
		if (var.equals(params.elementAt(k)))
		    return var;
	    }
	}
	return null;
    }

    /**
	 * Returns the first variable in the given "vars" string
	 * that matches one of the names in the Vector "params".
	 * Returns null if there is no match.
	 */
    private String hasVariable(String vars, Vector params){
	if (vars == null)
	    return null;
	StringTokenizer tok=new StringTokenizer(vars, " \t,\n");
	while (tok.hasMoreTokens()) {
	    String var = tok.nextToken().trim();
	    for (int k = 0; k < params.size(); k++){
		if (var.equals(params.elementAt(k)))
		    return var;
	    }
	}
	return null;
    }

    private void updateObject(VObjIF obj, Vector params){
        String  vars=obj.getAttribute(VARIABLE);
        if (vars == null)
	    return;
	StringTokenizer tok=new StringTokenizer(vars, " ,\n");
        while (tok.hasMoreTokens()) {
            String var = tok.nextToken();
            for (int k = 0; k < params.size(); k++) {
                if (var.equals(params.elementAt(k))) {
                    obj.updateValue();
                    return;
                }
            }
        }
    }

    /** called from ExperimentIF for first time initialization  */
    public void updateValue() {
	bNeedUpdate = true;
        if (showVal != null)
	   vnmrIf.asyncQueryShow(this, showVal);
	for (int i=0; i<getComponentCount(); i++) {
           Component comp = getComponent(i);
           if (comp instanceof VObjIF)
                ((VObjIF)comp).updateValue();
        }
	setBorder();
    }



    /** call "destructors" of VObjIF children  */
    public void destroy() {
        DisplayOptions.removeChangeListener(this);
		for (int i=0; i<getComponentCount(); i++) {
		    Component comp = getComponent(i);
		    if (comp instanceof VObjIF)
				((VObjIF)comp).destroy();
		}
    }

    public void setDefColor(String c) {
        fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public boolean getEditMode() {
	    return inEditMode;
    }

    public void setEditMode(boolean s) {
		inEditMode = s;
		int             k = getComponentCount();
		for (int i = 0; i < k; i++) {
			Component comp = getComponent(i);
			if (comp instanceof VObjIF) {
				VObjIF vobj = (VObjIF) comp;
				vobj.setEditMode(s);
			}
		}
	if(s) {
	    addMouseListener(ml);
	    defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
	}
	else
	    removeMouseListener(ml);
	if(bg==null || bg.equals("transparent") || bg.equals("default") || inEditMode )
            setOpaque(false);
        else
            setOpaque(true);
    }

    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return type;
        case FGCOLOR:
            return fg;
        case BGCOLOR:
             return bg;
        case FONT_NAME:
            return fontName;
        case FONT_STYLE:
            return fontStyle;
        case FONT_SIZE:
            return fontSize;
        case BORDER:
             return borderType;
        case SIDE:
            return titlePosition;
        case JUSTIFY:
            return titleJustification;
        case LABEL:
            return title;
        case TAB:
            return tab;
        case SHOW:
            return showVal;
        case VARIABLE:
            return vnmrVar;
        case REFERENCE:
            return reference;
        case USEREF:
            return useRef;
        case CMD:
            return vnmrCmd;
        case CMD2:
            return hideCmd;
        case VALUE:
          StringBuffer sb = new StringBuffer();
	  for (int i=0; i<getComponentCount(); i++) {
             Component comp = getComponent(i);
             if (comp instanceof VObjIF)
                 sb.append( (((VObjIF)comp).getAttribute(VALUE)) + " ");
          }
          return sb.toString();
        default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
	if (c != null)
	    c = c.trim();
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
            setBorder();
            repaint();
            break;
        case BGCOLOR:
            bg=c;
            if (c == null || c.length()==0 || c.equals("default")|| c.equals("transparent")){
 			    bgColor=null;
                setOpaque(false);
			}
            else{
                bgColor = DisplayOptions.getColor(c);
                setOpaque(true);
            }
           	setBackground(bgColor);
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
        case BORDER:
            borderType = c;
            setBorder();
            break;
        case LABEL:
            title=c;
		    if(inEditMode && c!=null){
                ParamLayout p=getParamLayout();
                title=title.trim();
//                if(p!=null)
//       	            p.relabelTab(this,isTab());
       	    }
            setBorder();
            break;
        case SIDE:
            titlePosition = c;
            setBorder();
            break;
        case JUSTIFY:
            titleJustification = c;
            setBorder();
            break;
        case TAB:
            String oldtab=tab;
        	if(c.equals("True")||c.equals("true")||c.equals("yes"))
            	tab = "yes";
            else
            	tab = "no";
            ParamLayout p=getParamLayout();
            if(inEditMode && title !=null && p!=null && !tab.equals(oldtab)){
                if(tab.equals("no")) {
                    p.rebuildTabs();
                } else {
//       	            p.relabelTab(this,true);
                }
            }
            break;
       case VARIABLE:
            vnmrVar = c;
            break;
        case SHOW:
            showVal = c;
            break;
        case REFERENCE:
            reference=c;
            break;
        case USEREF:
            useRef=c;
            break;
        case CMD:
            vnmrCmd=c;
            break;
        case CMD2:
            hideCmd=c;
            break;
       }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
	    for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF vobj = (VObjIF) comp;
                vobj.setVnmrIF(vif);
            }
        }
    }

    public void setValue(ParamIF pf) { }

    public void showGroup(boolean vis) {
 		setVisible(vis);
		for (int i=0; i<getComponentCount(); i++) {
			Component comp = getComponent(i);
			if(comp instanceof VParamCreateEntry)
				((VParamCreateEntry)comp).showGroup(vis);
			else
				comp.setVisible(vis);
		}
		setBorder();
		repaint();
    }

    public void setShowValue(ParamIF pf) {
       if (pf != null && pf.value != null && !inEditMode) {
            String  s = pf.value.trim();
            boolean oldshow=showing;
	    	showing =Integer.parseInt(s)>0?true:false;
	    	if(isTabGroup()){
	    	    ParamLayout p=getParamLayout();
	    	    if(p!=null && oldshow!=showing)
                    p.setChangedTab(title,showing);
	    	    return;
	    	}
	        showGroup(showing);
	    }
    }

    public String toString () {
        return title;
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    public void paint(Graphics g) {
		super.paint(g);
		if (!inEditMode)
		    return;
		Dimension  psize = getSize();
		if (isEditing) {
		    if (isFocused)
		        g.setColor(Color.yellow);
		    else
		        g.setColor(Color.green);
		}
		else
		    g.setColor(Color.blue);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }

    public Component add(Component comp) {
	    return add(comp, -1);
    }

    public Component add(Component comp, int index){

      if (compList == null) {
        compList = new Vector();
      }
      compList.add(comp);
      return super.add(comp, index);
    }

    public void remove(Component comp) {

      if (compList != null) {
        compList.remove(comp);
      }
      super.remove(comp);
    }

    public void refresh() {}
    public void setDefLabel(String s) {}
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void setSizeRatio(double x, double y) {
        double xRatio =  x;
        double yRatio =  y;
        if (x > 1.0)
            xRatio = x - 1.0;
        if (y > 1.0)
            yRatio = y - 1.0;
	if (defDim.width <= 0)
	    defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double)defDim.width * xRatio);
        curDim.height = (int) ((double)defDim.height * yRatio);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
    }

    public void setDefLoc(int x, int y) {
         defLoc.x = x;
         defLoc.y = y;
    }

    public Point getDefLoc() {
         tmpLoc.x = defLoc.x;
         tmpLoc.y = defLoc.y;
         return tmpLoc;
    }

    public void reshape(int x, int y, int w, int h) {
        if (inEditMode) {
           defLoc.x = x;
           defLoc.y = y;
           defDim.width = w;
           defDim.height = h;
        }
        curLoc.x = x;
        curLoc.y = y;
        curDim.width = w;
        curDim.height = h;
        super.reshape(x, y, w, h);
    }

    public Point getLocation() {
        if (inEditMode) {
           tmpLoc.x = defLoc.x;
           tmpLoc.y = defLoc.y;
        }
        else {
           tmpLoc.x = curLoc.x;
           tmpLoc.y = curLoc.y;
        }
        return tmpLoc;
    }


    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        setBorder();
        validate();
        repaint();
    }

    class vGrpLayout implements LayoutManager {
        private Dimension psize;

        public void addLayoutComponent(String name, Component comp) {}
        public void removeLayoutComponent(Component comp) {}
        public Dimension preferredLayoutSize(Container target) {
            if (width <= 0) {
                psize = getPreferredSize();
                width = psize.width;
                height = psize.height;
            }
            else
                psize = new Dimension(width, height);
            return  psize; // unused
        } // preferredLayoutSize()

       public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
              Point  loc;
              int nmembers = target.getComponentCount();
                for (int i = 0; i < nmembers; i++) {
                    Component comp = target.getComponent(i);
                    psize = comp.getPreferredSize();
                    loc = comp.getLocation();
                    comp.setBounds(loc.x, loc.y, psize.width, psize.height);
                    if (comp instanceof VObjIF) {
                        JComponent pobj = (JComponent) comp;
                        pobj.setBounds(loc.x, loc.y, psize.width, psize.height);
                    }
                }
            }
        }
    } // vGrpLayout

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
	    VObjDropHandler.processDrop(e, this,inEditMode);
    } // drop

    public Object[][] getAttributes() {	return attributes; }

    private final static String[] bdrTypes =
    {"None", "Etched", "RaisedBevel", "LoweredBevel"};
    private final static String[] ttlPosn =
    {"Top","AboveTop","BelowTop","Bottom","AboveBottom","BelowBottom"};
    private final static String[] ttlJust = {"Left","Right","Center"};
    private final static String[] yes_no = {"yes","no"};

    private final static Object[][] attributes = {
	{new Integer(LABEL), 		"Label of item:"},
	{new Integer(VARIABLE),		"Vnmr variables:    "},
	{new Integer(SHOW),			"Show condition:"},
	{new Integer(CMD),      	"Vnmr command on show:"},
	{new Integer(CMD2),      	"Vnmr command on hide:"},
    {new Integer(BGCOLOR),		"Background color:", "color"},
	{new Integer(BORDER), 		"Border type:", "menu", bdrTypes},
	{new Integer(SIDE),			"Label position:", "menu", ttlPosn},
	{new Integer(JUSTIFY), 		"Label justification:", "menu", ttlJust},
	{new Integer(TAB), 			"Tab to this group:", "radio", yes_no},
	{new Integer(USEREF), 		"Save as Reference:", "radio", yes_no},
        {new Integer(VALUE),            "Value:" },
    };

    private void setBorder() {
		if(borderType==null || borderType.equals("None")){
		    setBorder(BorderFactory.createEmptyBorder());
		    return;
		}
		Color titleColor = fgColor==null ? getForeground() : fgColor;
		Font titleFont = font==null ? getFont() : font;
		Border border= BorderDeli.createBorder(borderType,
							title,
							titlePosition,
							titleJustification,
							titleColor,
							titleFont);
		if(border !=null)
			setBorder(border);
    }
    public void setModalMode(boolean s) {}

    public void sendVnmrCmd() {
       if (vnmrIf == null)
            return;
        if (vnmrCmd != null)
	{
            vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
      }
    }

