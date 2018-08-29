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

import javax.swing.*;
import javax.swing.event.ChangeListener;
import javax.swing.event.ChangeEvent;

import vnmr.templates.LayoutBuilder;
import vnmr.util.*;
import vnmr.ui.*;

public class VTabbedPane extends JTabbedPane
  implements VObjIF, VObjDef, VEditIF, VGroupIF,
  ExpListenerIF, DropTargetListener, StatusListenerIF, PropertyChangeListener
{
    private String   type = null;
    private String   label = null;
    private String   fg = null;
    private String   bg = null;
    private String   fontName = null;
    private String   fontStyle = null;
    private String   fontSize = null;
    
    private Color    fgColor = null;
    private Font     font = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    protected ArrayList<Boolean> m_aListInitialized;
    protected ArrayList<Component> m_tabs;

    private MouseAdapter ml;
    private ButtonIF VnmrIf;
    private String side=null;

    public VTabbedPane(SessionShare sshare, ButtonIF vif, String typ) {
        super();
        this.type = typ;
        this.VnmrIf = vif;

        m_aListInitialized = new ArrayList<Boolean>();
        m_tabs = new ArrayList<Component>();

        ml = new MouseAdapter() {
            public void  mouseClicked(MouseEvent evt) {
                int  clicks = evt.getClickCount();
                int  modifier = evt.getModifiers();
                if((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2){
                        VObjIF sel=(VObjIF)evt.getSource();
                        if(sel instanceof VTabbedPane){
                            ParamEditUtil.setEditObj(sel);
                        }
                   }
                }
            }
        };
        setOpaque(false);
        new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
        addChangeListener(new ChangeListener() {
            public void stateChanged(ChangeEvent e) {
                tabChanged();
            }
        });
   }

    /** add a component by parsing an XML file.*/
    public void pasteXmlObj(String file) {
        System.out.println("folder: paste xml file");
        JComponent comp=null;
        try{
            comp=LayoutBuilder.build(this, VnmrIf, file, 0, 0);
        }
        catch(Exception be) {
            Messages.writeStackTrace(be);
        }
    }
    public Component add(Component comp) {
        return add(comp, getComponentCount());
    }

    public Component add(Component comp, int i) {
        Component c=comp;
        if (comp instanceof VGroupIF) {
            c=super.add(comp, i);
            String title=((VObjIF)comp).getAttribute(LABEL);
            if(title != null)
                setTitleAt(i,title);
            m_aListInitialized.add(i, new Boolean(false));
            m_tabs.add(i, comp);
            VGroup grp=(VGroup)comp;
            grp.setFolder(this);
            if(inEditMode){
                grp.setEditMode(true);
                setSelectedIndex(i);
                ParamEditUtil.setEditObj(grp);
            }
        }
        return c;
    }

    
    public void setTabEnabled(Component comp,boolean b){
        int indx=indexOfComponent(comp);
        if(indx>=0)
            setEnabledAt(indx,b);
    }
    
    public void setTabVisible(Component comp,boolean b){   
        if(b)
            showTab(comp);
        else
            hideTab(comp);
    }

    public void hideTab(Component comp){
        if(indexOfComponent(comp)>=0){
            if(m_tabs.contains(comp)){
               // System.out.println("hide tab:"+comp.getName());
                super.remove(comp);
            }
        }       
    }
    public void showTab(Component comp){
        if(indexOfComponent(comp)<0){
            if(m_tabs.contains(comp)){
                //System.out.println("show tab:"+comp.getName());
                super.add(comp);
            }
        }
        
    }

    public void selectTab(String name){
        if(name==null)
            return;
        for (int i=0; i<getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VGroup){
                String label=((VGroup)comp).getAttribute(LABEL);
                if(label !=null && label.equals(name)){
                    setSelectedIndex(i);
                    break;
                }
            }
        }
    }

     private void tabChanged() {
        Component comp = getSelectedComponent();
        if (comp != null && (comp instanceof VObjIF)) {
            if (inEditMode){
                System.out.println("new tab selected:"+comp.getName());
                ParamEditUtil.setEditObj((VObjIF)comp);
            }
            else
            ((VObjIF) comp).updateValue();
        } 
     }
    
     public void paint(Graphics g) {
    	Color c=VnmrJTheme.getTabPaneTabs();
    	for (int i=0; i<getComponentCount(); i++) {
    		 setBackgroundAt(i,c);
    	}
        super.paint(g);
        if (!inEditMode)
            return;
        Dimension  psize = getPreferredSize();
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

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
        if(DisplayOptions.isUpdateUIEvent(evt)){
             Component comp=getParent();
             while(comp !=null){
                if(comp instanceof ModelessDialog || comp instanceof ModalDialog)
                    return; // Dialog class will update UI
                comp=comp.getParent();
            }
            SwingUtilities.updateComponentTreeUI(this);
        }
    }

    // StatusListenerIF interface

    public void updateStatus(String msg){
        for (int i=0; i<getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof StatusListenerIF){
                ((StatusListenerIF)comp).updateStatus(msg);
            }
        }
    }

    // ExpListenerIF interface

    /** called from ExpPanel (pnew)   */
    public void  updateValue(Vector v){
        for(int i=0;i<m_tabs.size();i++){
          Component comp = m_tabs.get(i);
          if (comp instanceof ExpListenerIF){
              ((ExpListenerIF)comp).updateValue(v);
          }           
        }
    }
    /** called from ExperimentIF for first time initialization  */
    public void updateValue() {
        boolean bInitialized = isInitialized();
        try
        {
            if (!bInitialized)
                m_aListInitialized.set(getSelectedIndex(), new Boolean(true));
        }
        catch (Exception e) {}
        for (int i=0; i<m_tabs.size(); i++) {
            Component comp = m_tabs.get(i);
            if (comp instanceof ExpListenerIF){
                ((ExpListenerIF)comp).updateValue();
            }
        }
    }

    /**
     * Checks if the current tab is initialized.
     * @return true if current tab is initialized.
     */
    protected boolean isInitialized()
    {
        boolean bInitialized = false;
        Boolean objInitialized = (Boolean)m_aListInitialized.get(getSelectedIndex());
        if (objInitialized != null)
            bInitialized = objInitialized.booleanValue();

        return bInitialized;
    }

    // VObjIF interface

    /** call "destructors" of VObjIF children  */
    public void destroy() {
        DisplayOptions.removeChangeListener(this);
        for (int i=0; i<getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF){
                ((VObjIF)comp).destroy();
            }
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
        System.out.println("VTabbedPane.setModalMode:"+s);
        inEditMode = s;
        int             k = getComponentCount();
        for (int i = 0; i < k; i++) {
            Component       comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF vobj = (VObjIF) comp;
                vobj.setEditMode(s);
            }
        }
        if (inEditMode) {
            addMouseListener(ml);
        } else {
            removeMouseListener(ml);
        }
    }

    public String getAttribute(int attr) {
        switch (attr) {
        case LABEL:
            return label;
        case TYPE:
            return type;
        case SIDE:
            return side;
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
        default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
          case LABEL:
              label=c;
              break;
          case TYPE:
             type = c;
             break;
          case SIDE:
             if(c.equals("Left"))
                setTabPlacement(LEFT);
             else if(c.equals("Right"))
                setTabPlacement(RIGHT);
             else if(c.equals("Top"))
                setTabPlacement(TOP);
             else if(c.equals("Bottom"))
                setTabPlacement(BOTTOM);
            side=c;
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
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
            repaint();
            break;
        case BGCOLOR:
            bg=c;
            break;
        }
    }

    public ButtonIF getVnmrIF() {
        return VnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        VnmrIf = vif;
    for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF vobj = (VObjIF) comp;
                vobj.setVnmrIF(vif);
            }
        }
    }

    public void setValue(ParamIF pf) {}
    public void setShowValue(ParamIF pf) {
        
    }
    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    public void refresh() {}
    public void setDefLoc(int x, int y) {}
    public void setDefLabel(String s) {}
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        repaint();
    }
    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        System.out.println("drop in folder");
        VObjDropHandler.processDrop(e, this, inEditMode);
    }

    private final static String[] tside = {"Top","Bottom","Left", "Right"};
    public Object[][] getAttributes() { return attributes; }
    private final static Object[][] attributes = {
    {new Integer(LABEL),        "Name of Item:"},
    {new Integer(SIDE),         "Location of Tabs:", "menu", tside}
    };

    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

    public Point getDefLoc() {
    return getLocation();
    }

    public void setSizeRatio(double x, double y) {}

}

