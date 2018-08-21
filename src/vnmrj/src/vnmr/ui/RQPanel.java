/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;
import java.beans.*;

import javax.swing.tree.*;
import vnmr.ui.shuf.*;
import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;
import com.sun.xml.tree.*;

/**
 * The review queue panel.
 */
public class RQPanel extends JPanel 
    implements VObjDef, ExpListenerIF, PropertyChangeListener ,
    DropTargetListener
{
    SessionShare        sshare;
    ButtonIF            expIF=null;
    JScrollPane         scrollpane;
    JTreeTable          jtreetable = null;
    RQBuilder           mgr = null;
    JPanel              toolpanel;
    JPanel              pnl;
    String              rqtype = null;
    String              rqpath = null;
    RQParser            rqparser = null;
    String              toolfile="INTERFACE/ReviewQueue_tools.xml";
    String              rqfile="USER/INTERFACE/ReviewQueue_";
    String              show_status_str;
    String              expand_new_str;
    String              status_label;
    boolean             showStatus=true;
    Fullpopup           popup=null;
    MouseAdapter        ma=null;
    ReviewQueue         rq=null;
    static String       persistence_file = "RQPanel";

    JPanel pane = null;
    Dimension prevDim = new Dimension(0,0);
    ArrayList prevWidths = null;
    ArrayList currWidths = null;
    
    /**
     * constructor
     */
    public RQPanel(SessionShare sshare) {
        this.sshare = sshare;
        rq = new ReviewQueue();
        mgr = rq.getMgr();
        rqparser = new RQParser();

        DisplayOptions.addChangeListener(this);   

        String str = System.getProperty("SQAllowNesting");
        if (str != null && (str.equals("no") || str.equals("false")))
            mgr.setAllowNesting(false);

        status_label = Util.getLabel("qlStatusLabel", "Experiment Queue");
        show_status_str = Util.getLabel("qlShowStatus", "Show Review panel");
        expand_new_str = Util.getLabel("qlExpandNew", "Expand New Nodes");

        setLayout(new BorderLayout());
        setOpaque(false);

        pane = new JPanel();
        pane.setLayout(new myLayout());

        scrollpane = new JScrollPane();
        pane.add(scrollpane, BorderLayout.CENTER);
        add(pane, BorderLayout.CENTER);

        // buildToolPanel();
        toolpanel = new JPanel();
        add(toolpanel, BorderLayout.SOUTH);

        toolpanel.addMouseListener(new MouseAdapter() {
            public void mouseEntered(MouseEvent e) {
                getReviewQueue().getMgr().stopTableCellEditing();
            }
        });

        // buildRQPanel();

        popup = new Fullpopup();

         Util.setRQPanel(this);

        new DropTarget(this, this);

    } // RQPanel()

    /** PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        if(DisplayOptions.isUpdateUIEvent(evt))
            SwingUtilities.updateComponentTreeUI(this);
    }

    // this is called after vnmrbg is ready

    public void buildRQPanel(String type) {
        buildToolPanel();

        if (type == null || type.length() <= 0)
            type = "imgstudy";
        rqtype = type;

        mgr.newTree();
        buildNewRQ();

        if (rq != null)
            rq.setRQtype(rqtype);
    }

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        if (mgr == null || jtreetable == null)
            return;
        mgr.processDrop(e, jtreetable);
    } // drop

    public JTreeTable getTreeTable() {
        return jtreetable;
    }

    public RQParser getRQParser() {
        return rqparser;
    }

    public void buildNewRQ() {

        jtreetable = mgr.buildTreePanel();

        pnl = new JPanel();
        pnl.setLayout(new BoxLayout(pnl, BoxLayout.Y_AXIS));
        pnl.add(jtreetable.getTableHeader());
        pnl.add(jtreetable);

        scrollpane.setViewportView(pnl);
        JViewport vp = scrollpane.getViewport();
        int w = scrollpane.getWidth();
        mgr.initColumnSizes(w);

        mgr.expandTree();

        pnl.addMouseListener(ma);
    }


    public void setViewPort(int id) {
        if (toolpanel == null)
            return;

        // ButtonIF vif = Util.getViewArea().getActiveVp();
        ButtonIF vif = Util.getViewArea().getExp(id);

        if (vif == null)
            return;

        for (int i = 0; i < toolpanel.getComponentCount(); i++) {
            Component comp = toolpanel.getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF obj = (VObjIF) comp;
                obj.setVnmrIF(vif);
            }
        }
    }

    // ExpListenerIF interface
    /** called from ExpPanel (pnew) */
    public void updateValue(Vector v) {
        if (toolpanel == null)
            return;
        for (int i = 0; i < toolpanel.getComponentCount(); i++) {
            Component comp = toolpanel.getComponent(i);
            if (comp instanceof ExpListenerIF)
                ((ExpListenerIF) comp).updateValue(v);
        }
    }

    /** called from ExperimentIF for first time initialization */
    public void updateValue() {
        if (toolpanel == null)
            return;
        for (int i = 0; i < toolpanel.getComponentCount(); i++) {
            Component comp = toolpanel.getComponent(i);
            if (comp instanceof ExpListenerIF)
                ((ExpListenerIF) comp).updateValue();
        }
    }

    public String getRQtype() {
        if (toolpanel == null)
            return "imgstudy";
        updateRQtype(toolpanel);
        return rqtype;
    }

    public void updateRQtype(JComponent panel) {
        if (panel == null)
            return;
        for (int i = 0; i < panel.getComponentCount(); i++) {
            Component comp = panel.getComponent(i);
            if (comp instanceof VMenu) {
                String vq = ((VMenu) comp).getAttribute(VARIABLE);
                if (vq.equals("rqtype")) {
                    rqtype = ((JComboBox) comp).getSelectedItem().toString();
                    return;
                }
            } else if (comp instanceof ExpListenerIF) {
                updateRQtype((JComponent) comp);
            }
        }
    }

    // ============ private Methods ==================================
    private ButtonIF getExpIF(){
        return Util.getViewArea().getDefaultExp();
    }
    public void buildToolPanel() {
        toolpanel.setLayout(new TemplateLayout());

        String path = FileUtil.openPath(toolfile);
        try {
            LayoutBuilder.build(toolpanel, getExpIF(), path);
        } catch (Exception e) {
            String strError = "Error building file: " + toolfile;
            Messages.postError(strError);
            Messages.writeStackTrace(e);
            return;
        }
    }

    private void setDebug(String s){
        if(DebugOutput.isSetFor("SQ"))
            Messages.postDebug("RQPanel "+s);
    }

    //----------------------------------------------------------------
    /** clear all tree items. */
    //----------------------------------------------------------------
    public void clearTree(){
        mgr.clearTree();
    }
 
    //----------------------------------------------------------------
    /** set the review queue io processor. */
    //----------------------------------------------------------------
    public void setReviewQueue(ReviewQueue q){
        rq=q;
        mgr=rq.getMgr();
    }

    //----------------------------------------------------------------
    /** get the review queue io processor. */
    //----------------------------------------------------------------
    public ReviewQueue getReviewQueue(){
        return rq;
    }
    
    //============ Methods called from ProtocolBuilder (rq) ==========

    //----------------------------------------------------------------
    /** Save the tree as an XML file. */
    //----------------------------------------------------------------
    public void saveReview(String path){
        rq.saveReview(path);
    }

    //----------------------------------------------------------------
    /** Return name of review in review dir. */
    //----------------------------------------------------------------
    public String reviewName(String dir){
       return rq.reviewName(dir);
    }

    //----------------------------------------------------------------
    /** Return true if review can be loaded. */
    //----------------------------------------------------------------
    public boolean testRead(String dir){
        return rq.testRead(dir);
    }

    //----------------------------------------------------------------
    /** Click in treenode. */
    //----------------------------------------------------------------
    public void wasClicked(String id, int mode){
        rq.wasClicked(id, mode);
    }

    //----------------------------------------------------------------
    /** Double click in treenode. */
    //----------------------------------------------------------------
    public void wasDoubleClicked(String id){
        rq.wasDoubleClicked(id);
    }

    //----------------------------------------------------------------
    /** Tree was modified. */
    //----------------------------------------------------------------
    public void setDragAndDrop(String id){
        rq.setDragAndDrop(id);
    }
            
    //================ Methods called from Vnmrbg ====================

    //----------------------------------------------------------------
    /** Process a Vnmrbg command  */
    //----------------------------------------------------------------
    public void processCommand(String str) {
        rq.processCommand(str);
    }

    public void togglePanel(String mode) {
        if (!Util.getViewPortType().equals(Global.REVIEW))
            mode = "fit";

        boolean b = popup.getMode();
        if (!b && !mode.equals("full"))
            return;
        if (b && mode.equals("full")) {
            popup.show(this, 0, 0);
            return;
        }

        popup.setToggle(mode);
        if (mode.equals("full")) {
            popup.show(this, 0, 0);
        } else {
            popup.remove(toolpanel);
            add(toolpanel, BorderLayout.SOUTH);
            validate();
            repaint();
        }
    }

    public void updatePopup(String tab) {
        if (!popup.getMode())
            return;
        boolean b = false;
        if (tab.equalsIgnoreCase("Locator"))
            b = true;
        popup.setVisible(b, false);
    }

    public void setVisible(boolean b) {
        super.setVisible(b);

        if (!popup.getMode())
            return;

        popup.setVisible(b);
    }

    //============== Local classes ===============================

    class Fullpopup extends JPopupMenu implements DropTargetListener {
        boolean visible = false;
        boolean full = false;
        JScrollPane spane = null;
        JPanel panel = null;

        public Fullpopup() {

            setLightWeightPopupEnabled(false);
            panel = new JPanel();
            panel.setLayout(new BorderLayout());
            spane = new JScrollPane();
            panel.add(spane, BorderLayout.CENTER);
            spane.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            spane.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
            add(panel, BorderLayout.CENTER);
            new DropTarget(Fullpopup.this, Fullpopup.this);
        }

        public void setToggle(String mode) {
            if (mode.equals("full")) {
                full = true;
                visible = true;
                mgr.setAdjustWidth(false);
                // int h = scrollpane.getHeight();
                int h = pane.getParent().getHeight();
                int w = pnl.getWidth();
                Dimension mdim = VNMRFrame.vnmrFrame.getSize();
                if (h > mdim.height)
                    h = mdim.height;
                if (w > mdim.width)
                    w = mdim.width;
                scrollpane.remove(pnl);
                spane.setViewportView(pnl);
                spane.setBounds(new Rectangle(0, 0, w, h));
                JViewport viewPort = spane.getViewport();
                viewPort.setViewSize(new Dimension(w, h));
                setPopupSize(new Dimension(w, h));
                remove(toolpanel);
                panel.add(toolpanel, BorderLayout.SOUTH);

            } else {
                full = false;
                visible = false;
                mgr.setAdjustWidth(true);
                spane.remove(pnl);
                scrollpane.setViewportView(pnl);
                setVisible(false);
            }

        }

        public void setVisible(boolean b, boolean v) {
            visible = v;
            setVisible(b);
        }

        public boolean getMode() {
            return full;
        }

        public void setVisible(boolean b) {
            // uncomment this line if want the popup stay.
            // if(Util.getViewPortType().equals(Global.REVIEW) && !b && visible)
            // return;
            visible = b;
            super.setVisible(b);

            if (!b) {
                Util.sendToVnmr("rqfull=0");
                togglePanel("fit");
            }
        }

        public void dragEnter(DropTargetDragEvent e) {
        }

        public void dragExit(DropTargetEvent e) {
        }

        public void dragOver(DropTargetDragEvent e) {
        }

        public void dropActionChanged(DropTargetDragEvent e) {
        }

        public void drop(DropTargetDropEvent e) {
            if (mgr == null || jtreetable == null)
                return;
            mgr.processDrop(e, jtreetable);
        } // drop

    } // class Fullpopup

    class myLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        /**
         * calculate the preferred size
         * @param target component to be laid out
         * @see #minimumLayoutSize
         */
        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // preferredLayoutSize()

        /**
         * calculate the minimum size
         * @param target component to be laid out
         * @see #preferredLayoutSize
         */
        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                if (pane == null || scrollpane == null)
                    return;
                Dimension dim = target.getSize();
                if (dim.width == prevDim.width && dim.height == prevDim.height
                        && !widthChanged())
                    return;
                Insets insets = target.getInsets();
                pane.setPreferredSize(dim);
                scrollpane.setBounds(new Rectangle(0, 0, dim.width, dim.height));
                JViewport viewPort = scrollpane.getViewport();
                viewPort.setViewSize(dim);
                prevDim = new Dimension(dim.width, dim.height);
                prevWidths = currWidths;
            }
        }

        private boolean widthChanged() {
            currWidths = mgr.getColumnWidths();
            if (currWidths == null || prevWidths == null)
                return true;

            if (currWidths.size() != prevWidths.size())
                return true;

            for (int i = 0; i < currWidths.size(); i++) {
                if (!currWidths.get(i).equals(prevWidths.get(i)))
                    return true;
            }
            return false;
        }
    }

    public int getDragMode() {
    return rq.getDragMode();
    }

} // class RQPanel
