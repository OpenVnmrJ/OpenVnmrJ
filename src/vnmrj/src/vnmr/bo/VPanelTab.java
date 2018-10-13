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
import java.beans.*;
import java.util.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VPanelTab extends JLayeredPane
implements VObjIF, VObjDef, VGroupIF, DropTargetListener,
           PropertyChangeListener,StatusListenerIF

{
    public String type = null;
    public String label = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String pname = null;
    public String ptype = null;
    public String pfile = null;
    public String actionFile = null;
    public String param = null;
    public Color  fgColor = null;
    public Color  bgColor, orgBg;
    public Color  selectedColor;
    public Color  unselectedColor;
    public Font   font = null;
    public JComponent paramPanel;
    public JPanel tabPanel;
    public boolean needsNewParamPanel = true;
    public ActionBar actionPanel=null;
    public int tabId = 0;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private int isActive = 1;
    private MouseAdapter mlEditor;
    private MouseAdapter mlNonEditor;
    protected String m_helplink = null;
    private ButtonIF vnmrIf;
    private TabButton tab;
    private Point defLoc = new Point(0, 0);
    private String paneltab=null;

    public VPanelTab(SessionShare sshare, ButtonIF vif, String typ) {
	this.type = typ;
	this.vnmrIf = vif;
	this.fg = "black";
        this.fontSize = "8";
        this.label = "button";
	setLayout(new PanelTabLayout());
	tab = new TabButton("");
	add(tab);
        setBgColors();

	mlEditor = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        ParamEditUtil.setEditObj((VObjIF)evt.getSource());
                    }
                }
             }
	};
	
    mlNonEditor = new CSHMouseAdapter();
    tab.addMouseListener(mlNonEditor);

	new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
        setActive(false);
    }

    private void setBgColors() {
	selectedColor = Util.getBgColor();
        unselectedColor = Util.changeBrightness(selectedColor, -20);
	bgColor = orgBg = unselectedColor;
    }

    /** PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        if(fg!=null){
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        changeFont();
        setBgColors();
        setActive(tab.isActive()); // To set the bkg.
    }

    /** StatusListenerIF interface */
    public void updateStatus(String msg){
        int nSize = getComponentCount();
        for (int i=0; i<nSize; i++) {
            Component comp = getComponent(i);
            if (comp instanceof StatusListenerIF)
                ((StatusListenerIF)comp).updateStatus(msg);
        }
        if(actionPanel!=null){
            nSize = actionPanel.getComponentCount();
            for (int i = 0; i < nSize; i++) {
                JComponent comp = (JComponent) actionPanel.getComponent(i);
                if (comp instanceof StatusListenerIF)
                    ((StatusListenerIF)comp).updateStatus(msg);
            }
        }
    }

    public void setPanelTab(String s){
        paneltab=s;

    }
    public String getPanelTab(){
        return paneltab;
    }

    public void setTabAction( ActionListener a) {
	tab.addActionListener(a);
    }

    public void setActive(boolean s) {
        tab.setActive(s);
        int nSize = getComponentCount();
        for (int i = 0; i < nSize; i++) {
            Component comp = getComponent(i);
            if (s) {
		comp.setBackground(selectedColor);
            } else {
		comp.setBackground(unselectedColor);
            }
        }
    }

    public void setDefLabel(String s) {
    	this.label = s;
	tab.setText(s);
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        if(actionPanel !=null)
            actionPanel.setEditMode(s);
        if(paramPanel !=null)
            ((ParameterPanel)paramPanel).setEditMode(s);
        if (s) {
            // Be sure both are cleared out
            removeMouseListener(mlNonEditor);
            removeMouseListener(mlEditor);
            addMouseListener(mlEditor);
        }
        else {
            // Be sure both are cleared out
            removeMouseListener(mlEditor);
            removeMouseListener(mlNonEditor);
            addMouseListener(mlNonEditor);
        }
        if (bg == null)
            setOpaque(s);
        inEditMode = s;
    }

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
        switch (attr) {
          case TYPE:
                     return type;
          case LABEL:
          case VALUE:
                     return label;
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
          case VARIABLE:
    		     return vnmrVar;
          case PANEL_NAME:
    		     return pname;
          case PANEL_TYPE:
    		     return ptype;
          case PANEL_FILE:
    		     return pfile;
          case PANEL_PARAM:
    		     return param;
          case ACTION_FILE:
    		     return actionFile;
          case HELPLINK:
              return m_helplink;
          default:
                    return null;
        }
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
          case TYPE:
                     type = c;
                     break;
          case LABEL:
          case VALUE:
                     label = c;
                     tab.setText(c);
                     break;
          case FGCOLOR:
                     fg = c;
                     fgColor=DisplayOptions.getColor(fg);
                     setForeground(fgColor);
                     repaint();
                     break;
          case BGCOLOR:
                     bg = c;
		     if (c != null) {
                        bgColor = VnmrRgb.getColorByName(c);
                        setOpaque(true);
                     }
                     else {
                        bgColor = orgBg;
                        setOpaque(inEditMode);
                     }
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
          case SETVAL:
                     setVal = c;
                     break;
          case CMD:
    		     vnmrCmd = c;
                     break;
          case VARIABLE:
    		     vnmrVar = c;
                     break;
          case PANEL_NAME:
    		     pname = c;
		     tab.setText(c);
		     validate();
                     break;
          case PANEL_FILE:
    		     pfile = c;
                     break;
          case PANEL_TYPE:
    		     ptype = c;
                     break;
          case PANEL_PARAM:
    		     param = c;
                     break;
          case ACTION_FILE:
    		     actionFile = c;
                     break;
          case HELPLINK:
              m_helplink = c;
              break;
        }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
        int nSize = getComponentCount();
        for (int i = 0; i < nSize; i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF vobj = (VObjIF) comp;
                vobj.setVnmrIF(vif);
            }
        }
    }

    public void setValue(ParamIF pf) {
	if (pf != null && pf.value != null) {
            label = pf.value;
            tab.setText(label);
        }
    }

    public void setShowValue(ParamIF pf) {
	if (pf != null && pf.value != null) {
	    String  s = pf.value.trim();
	    isActive = Integer.parseInt(s);
	    if (isActive > 0) {
                setVisible(true);
		tab.setVisible(true);
                setEnabled(true);
		tab.setEnabled(true);
                if (setVal != null) {
	    	    vnmrIf.asyncQueryParam(this, setVal);
                }
            }
            else if (isActive == 0) {
                setVisible(true);
		tab.setVisible(true);
                setEnabled(false);
		tab.setEnabled(false);
	    }
            else {
                setVisible(false);
		tab.setVisible(false);
            }
        }
    }

    public void updateValue() {
        if (vnmrIf == null)
            return;
        if (setVal != null) {
            vnmrIf.asyncQueryParam(this, setVal);
        }
        else if (showVal != null) {
            vnmrIf.asyncQueryShow(this, showVal);
        }
        int nSize = getComponentCount();
        for (int i = 0; i < nSize; i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF vobj = (VObjIF) comp;
                vobj.updateValue();
            }
        }
    }

    public void paint(Graphics g) {
	Dimension  psize = getPreferredSize();
	super.paint(g);
	if (!isEditing)
	    return;
	if (isFocused)
            g.setColor(Color.yellow);
	else
            g.setColor(Color.green);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }

    public void refresh() {
    }

    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void setDefLoc(int x, int y) {}
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void setSizeRatio(double x, double y) {}
    public Point getDefLoc() { return defLoc; }


    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
	    VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop

    class PanelTabLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        /**
         * calculate the preferred size
         * @param target component to be laid out
         * @see #minimumLayoutSize
         */
        public Dimension preferredLayoutSize(Container target) {
            Insets insets = target.getInsets();
            Dimension dim = tab.getPreferredSize();
	    int w = insets.left + insets.right + dim.width;
	    int h = insets.top + insets.bottom + dim.height;
	    int w1 = 0;
	    int childs = getComponentCount();
	    int tnum = childs;
	    for (int i = 0; i < childs; i++) {
		Component comp = getComponent(i);
           	if (comp.equals(tab)) {
		    tnum = i;
		}
           	else {
                    dim = comp.getPreferredSize();
		    w1 = w1 + dim.width + 2;
		    if (dim.height > h)
			h = dim.height;
		    if (i > tnum) {
			target.remove(comp);
			target.add(comp, 0);
		    }
		}
		JComponent obj = (JComponent) comp;
	   	obj.setOpaque(true);
	    }
	    if (w1 > 0) {
		insets = tab.getBorderInsets();
		insets.left = w1;
		tab.setBorderInsets(insets);
		tab.validate();
	    }
            return new Dimension(w, h);
        } // preferredLayoutSize()

        /**
         * calculate the minimum size
         * @param target component to be laid out
         * @see #preferredLayoutSize
         */
        public Dimension minimumLayoutSize(Container target) {
            Insets insets = target.getInsets();
            Dimension btnSize = tab.getMinimumSize();

            return new Dimension(insets.left + insets.right + btnSize.width,
                                 insets.top + insets.bottom + btnSize.height);
        } // minimumLayoutSize()

	public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension targetSize = target.getSize();
                Insets insets = target.getInsets();

                int h0 = insets.left;
                int v0 = insets.top;
                int usableWidth = targetSize.width - h0 - insets.right;
                int usableHeight = targetSize.height - v0 - insets.bottom;
                tab.setBounds(new Rectangle(h0, v0, usableWidth,
                                                   usableHeight));
		int childs = getComponentCount();
		h0 += 2;
		for (int i = 0; i < childs; i++) {
		    Component comp = getComponent(i);
           	    if (!comp.equals(tab)) {
                        Dimension dim = comp.getPreferredSize();
			int v1 = (usableHeight - dim.height) / 2;
			if (v1 < 0)
			    v1 = 0;
                	comp.setBounds(new Rectangle(h0 , v1, dim.width, dim.height));
			h0 = h0 + dim.width + 2;
		    }
		}
            }
        } // layoutContainer()

    } // class PanelTabLayout
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}
    
    
    /* CSHMouseAdapter
     * 
     * Mouse Listener to put up Context Sensitive Help (CSH) Menu and
     * respond to selection of that menu.  The panel's .xml file must have
     * "helplink" set to the keyword/topic for Robohelp.  It must be a 
     * topic listed in the .properties file for this help manual.
     * If helplink is not set, it will open the main manual.
     * 
     * For VPanelTab, helplink cannot be accessed via the Panel Editor because
     * this item is not edited in the editor as near as I can tell.  However,
     * if the TopPanel.xml file is created to have helplink set, the CSH menu
     * works.  Since this file is created by Agilent, hopefully this will be okay.
     */
    private class CSHMouseAdapter extends MouseAdapter  {

        public CSHMouseAdapter() {
            super();
        }

        public void mouseClicked(MouseEvent evt) {
            int btn = evt.getButton();
            if(btn == MouseEvent.BUTTON3) {
                // Find out if there is any help for this item. If not, bail out
                String helpstr=m_helplink;
                if(helpstr==null){
                    Container group = getParent();
                    if(group instanceof VGroup)
                        helpstr=((VGroup)group).getAttribute(HELPLINK);
                    if(helpstr==null){
                        Container group2 = group.getParent();
                        if(group2 instanceof VGroup)
                            helpstr=((VGroup)group2).getAttribute(HELPLINK);
                        if(helpstr==null){
                            Container group3 = group2.getParent();
                            if(group3 instanceof VGroup)
                                helpstr=((VGroup)group3).getAttribute(HELPLINK);
                        }
                    }
                }
                // If no help is found, don't put up the menu, just abort
                if(helpstr == null) {
                    return;
                }
                
                // Create the menu and show it
                JPopupMenu helpMenu = new JPopupMenu();
                String helpLabel = Util.getLabel("CSHMenu");
                JMenuItem helpMenuItem = new JMenuItem(helpLabel);
                helpMenuItem.setActionCommand("help");
                helpMenu.add(helpMenuItem);
                    
                ActionListener alMenuItem = new ActionListener()
                    {
                        public void actionPerformed(ActionEvent e)
                        {
                            // Get the helplink string for this object
                            String helpstr=m_helplink;
                            
                            // If helpstr is not set, see if there is a higher
                            // level VGroup that has a helplink set.  If so, use it.
                            // Try up to 3 levels of group above this.
                            if(helpstr==null){
                                  Container group = getParent();
                                  if(group instanceof VGroup)
                                         helpstr=((VGroup)group).getAttribute(HELPLINK);
                                  if(helpstr==null){
                                      Container group2 = group.getParent();
                                      if(group2 instanceof VGroup)
                                             helpstr=((VGroup)group2).getAttribute(HELPLINK);
                                      if(helpstr==null){
                                          Container group3 = group2.getParent();
                                          if(group3 instanceof VGroup)
                                                 helpstr=((VGroup)group3).getAttribute(HELPLINK);
                                      }
                                  }
                            }
                            // Get the ID and display the help content
                            CSH_Util.displayCSHelp(helpstr);
                        }

                    };
                helpMenuItem.addActionListener(alMenuItem);
                    
                Point pt = evt.getPoint();
                helpMenu.show(VPanelTab.this, (int)pt.getX(), (int)pt.getY());

            }
             
        }
    }  /* End CSHMouseAdapter class */
    
}

