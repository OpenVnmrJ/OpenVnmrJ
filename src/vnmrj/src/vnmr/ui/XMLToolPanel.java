/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.util.*;

import javax.swing.*;

import java.awt.*;
import java.beans.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;

public class XMLToolPanel extends JPanel implements VObjDef, ExpListenerIF,
        PropertyChangeListener, StatusListenerIF,EditListenerIF {

    // these are needed for a modal dialog to talk to vnmr
    protected ButtonIF vnmrIf;
    protected SessionShare sshare;
    ParamPanel pane = null;
    JScrollPane spane=null;
    String layoutDir = "toolPanels";
    String pname = null;
    String fname = null;
    private VRubberPanLayout paramsLayout;
    private boolean bNeedUpdate = true;
    private boolean bScrollBar = true;

    public XMLToolPanel(SessionShare ss, String pname, String fname) {

        this.vnmrIf = (ButtonIF)Util.getViewArea().getDefaultExp();
        this.sshare = ss;
        this.pname = pname;
        this.fname = fname;

        pane = new ParamPanel();
        pane.setPanelFile(fname);
        pane.setPanelName(pname);
        pane.setType(ParamInfo.TOOLPANEL);
        pane.setLayoutName("toolPanels");
        pane.setAttributes(null);

        paramsLayout = new VRubberPanLayout();
        if(!Util.isImagingUser())
            paramsLayout.setSquish(1.0);

        DisplayOptions.addChangeListener(this);
        ExpPanel.addStatusListener(this);
        // ExpPanel.addExpListener(this);

        setLayout(new xpPanelLayout());
    } // constructor

    public Dimension getActualSize() {
        return paramsLayout.preferredLayoutSize(this);
    }
    
    public void setEditMode(boolean s) {
        if(!s)
            pane.setEditMode(s);
        else{
            int edits=ParamInfo.getEditItems();
            if((edits & ParamInfo.TOOLPANELS)>0)
                pane.setEditMode(true);
            else
                pane.setEditMode(false);               
        }       
    }
    public void updateStatus(String str){
        if(pane == null) return;
        for (int i=0; i<pane.getComponentCount(); i++) {
            Component comp = pane.getComponent(i);
            if (comp instanceof StatusListenerIF) 
             ((StatusListenerIF)comp).updateStatus(str);
        }
    }

    public void setReferenceSize(int w, int h) {
        paramsLayout.setReferenceSize(w, h);
    }

    public void buildPanel() {
        String path = FileUtil.getLayoutPath(layoutDir, fname);
        if(path == null){
            Messages.postError("Could not open xml file:"+fname);
            return;
        }
        removeAll();
        try {
            LayoutBuilder.build(pane, vnmrIf, path);
        } catch(Exception e) {
            String strError = "Error building file: " + path;
            Messages.postError(strError);
            Messages.writeStackTrace(e);
            return;
        }

        if(bScrollBar){
            spane = new JScrollPane(pane, JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
            add(spane);
        }
        else{
            spane=null;
            add(pane);
        }
        paramsLayout.preferredLayoutSize(pane);
    }

    public void setScrollAble(boolean b){
        bScrollBar = b;
    }

    public void setViewPort(int id) {
        if(pane == null)
            return;

        // ButtonIF vif = Util.getViewArea().getActiveVp();
        ButtonIF vif = Util.getViewArea().getExp(id);

        if(vif == null)
            return;

        int nums = pane.getComponentCount();

        for(int i = 0; i < nums; i++) {
            Component comp = pane.getComponent(i);
            if(comp instanceof VObjIF) {
                VObjIF obj = (VObjIF)comp;
                obj.setVnmrIF(vif);
            }
        }
    }

    public void  updateValue(Vector v){
       bNeedUpdate = false;
       if(pane == null) return;
       for (int i=0; i<pane.getComponentCount(); i++) {
           Component comp = pane.getComponent(i);
           if (comp instanceof ExpListenerIF) 
            ((ExpListenerIF)comp).updateValue(v);
       }
    }

    public void updateValue(){ 
        bNeedUpdate = false;
	if(pane == null) return;
	for (int i=0; i<pane.getComponentCount(); i++) {
        Component comp = pane.getComponent(i);
        if (comp instanceof ExpListenerIF)
            ((ExpListenerIF)comp).updateValue();
        }
    }

    public void updateAllValue() {
        if(pane == null)
            return;
        bNeedUpdate = false;
        int nums = pane.getComponentCount();
        for(int i = 0; i < nums; i++) {
            Component comp = pane.getComponent(i);
            if(comp instanceof VObjIF) {
                VObjIF obj = (VObjIF)comp;
                obj.updateValue();
            }
        }
    }

    public void valueChanged() {
        bNeedUpdate = true;
    }

    public void updateChange() {
        if (!bNeedUpdate)
           return;
        bNeedUpdate = false;
        updateValue();
    }


    public void propertyChange(PropertyChangeEvent evt) {
        if(DisplayOptions.isUpdateUIEvent(evt))
            SwingUtilities.updateComponentTreeUI(this);
    }

    class xpPanelLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        /**
         * calculate the preferred size
         * 
         * @param target
         *            component to be laid out
         * @see #minimumLayoutSize
         */
        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // preferredLayoutSize()

        /**
         * calculate the minimum size
         * 
         * @param target
         *            component to be laid out
         * @see #preferredLayoutSize
         */
        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            /*
             * Algorithm is as follows: - the "left-hand" components take 33% of
             * horizontal space - scrollPane takes the remaining space
             */
            synchronized(target.getTreeLock()) {
                if(pane == null || spane == null)
                    return;
                Dimension dim = target.getSize();
                if(dim.height < 20)
                    return;

                spane.setBounds(new Rectangle(0, 0, dim.width, dim.height));
                // Dimension dim0 = new Dimension(dim.width-2, dim.height-2);
                // pane.setPreferredSize(dim0);
                // JViewport viewPort = spane.getViewport();
                // viewPort.setViewSize(dim0);
                // paramsLayout.setReferenceSize(dim0.width, dim0.height);
            }
        }
    }
}
