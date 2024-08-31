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
import java.util.*;
import java.beans.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;

import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.*;

import vnmr.util.*;
import vnmr.bo.*;
import vnmr.templates.*;
import vnmr.ui.shuf.*;

/**
 * The ParameterPanel is used to setup the acquisition of data from
 * an experiment.
 *
 */
public class ParameterPanel extends JLayeredPane implements StatusListenerIF, PropertyChangeListener {
    /** parameters panel */
    private ParamLayout paramLayout;
    /** scroll pane */
    private JScrollPane scrollPane;
    /** view port */
    private JViewport viewPort;
    /** header popup */
    private PopButton headerPop;
    private ParamBoder border;
    private TabList tabList;
    private DefaultListModel listModel = new DefaultListModel();
    private JScrollPane listScrollPane;
    private boolean isEditMode = false;
    private boolean isNewPanel = true;
    private int viewWidth;
    private int viewHeight;
    private int fontHeight = 16;
    private ButtonIF  vnmrIf;
    private Font listFont = null;
    private Color listColor = null;

    /**
     * constructor
     * @param sshare session share
     */

     public ParameterPanel(SessionShare sshare, ButtonIF vif, ExpPanInfo info) {
        this.vnmrIf = vif;
        this.isNewPanel = true;
        
        DisplayOptions.addChangeListener(this);

        border = new ParamBoder(EtchedBorder.RAISED);
        // setBorder(border);
        setBorder(null);
        setLayout(new pPanelLayout());
        tabList = new TabList(listModel);
        listScrollPane = new JScrollPane(tabList,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        listScrollPane.setAlignmentX(LEFT_ALIGNMENT);
        listScrollPane.setAlignmentY(TOP_ALIGNMENT);
        // if (listBg != null)
        //     tabList.setBackground(listBg);
        add(listScrollPane);

        paramLayout = new ParamLayout(sshare, vnmrIf);
        paramLayout.setFileInfo(info);
        paramLayout.setParameterPanel(this);

        scrollPane = new JScrollPane(paramLayout,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
        add(scrollPane);
        paramLayout.setTabList(tabList);

        viewPort = scrollPane.getViewport();
        if (!info.fpathIn.equals("NOTFOUND")) {
            try{
                LayoutBuilder.build(paramLayout,vnmrIf,info.fpathIn);
            }
            catch(Exception e) {
                /***********
                paramLayout = null;
                listScrollPane = null;
                tabList = null;
                listModel = null;
                scrollPane = null;
                ***********/
                Messages.writeStackTrace(e);
                return;
            }
        }

        tabList.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));

        setDebug("new panel");
        paramLayout.rebuildTabs();
        /*SwingUtilities.invokeLater(new Runnable()
        {
            public void run() {
                paramLayout.rebuildTabs();
                paramLayout.setLastSelectedTab();
            }
        });*/

/**
        headerPop = new PopButton();
        headerPop.setOpaque(false);
        headerPop.setAutoUpdate(true);
        headerPop.setNumberedMenu(paramLayout.getParamNames());
        add(headerPop, PALETTE_LAYER);
**/

/**
        headerPop.addPopListener(new PopListener() {
            public void popHappened(String popStr) {
                int index = Integer.parseInt(popStr);
                headerPop.setText(index);
                int x = paramLayout.setHPos(index);
                viewPort.setViewPosition(new Point(limitViewXPos(x), 0));
            }
        });
**/
        setUiColor();
        setSize(600, 400);
    } // ParameterPanel()


    private void setDebug(String s){
        if(DebugOutput.isSetFor("panels"))
            Messages.postDebug("ParameterPanel "+s);
    }

    private void setUiColor() {
            Color bgColor = UIManager.getColor("Panel.background");
            tabList.setBackground(bgColor);
            paramLayout.setBackground(bgColor);
            setBackground(bgColor);
          /****
            Color listColor = Util.getListBgColor();
            if (listColor != null)
                 tabList.setBackground(listColor);
            listColor = Util.getListSelectBg();
            if (listColor != null)
                 tabList.setSelectionBackground(listColor);
            listColor = Util.getListSelectFg();
            if (listColor != null)
                 tabList.setSelectionForeground(listColor);
            Color bgColor = UIManager.getColor("Panel.background");
            paramLayout.setBackground(bgColor);
            listScrollPane.setBackground(bgColor);
            viewPort.setBackground(bgColor);
            setBackground(bgColor);
          ****/
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
        Font newFont = DisplayOptions.getFont("", "Menu1", "");
        listColor = DisplayOptions.getColor("Menu1"); 
        if(DisplayOptions.isUpdateUIEvent(evt)){
            setUiColor();

          /*********
            Color bgColor = Util.getBgColor();
            setBackground(bgColor);
            listScrollPane.setBackground(bgColor);
            tabList.setBackground(bgColor);
            paramLayout.setBackground(bgColor);
            viewPort.setBackground(bgColor);
            listFont = DisplayOptions.getFont("", "Menu1", ""); 
            listColor = DisplayOptions.getColor("Menu1"); 
            SwingUtilities.updateComponentTreeUI(this);
            // It seems that a child of JScrollPane (e.g. paramLayout) isn't a direct part of 
            // an object's componant tree so requires an explicit update.
            SwingUtilities.updateComponentTreeUI(paramLayout); 
            SwingUtilities.updateComponentTreeUI(tabList); 
          *********/
        }
        if (tabList != null) {
            /*******
            if (!newFont.equals(listFont)) {
                listFont = newFont;
                SwingUtilities.updateComponentTreeUI(tabList); 
            }
            else
            *******/
            if (newFont != null)
                listFont = newFont;
            tabList.repaint();
        }
    }

    protected void finalize() throws Throwable   {
        super.finalize();
    }

    public void setFileInfo(ExpPanInfo info) {
        if (paramLayout != null)
            paramLayout.setFileInfo(info);
    }

    public void setTabLabel(String page) {
        if (paramLayout != null){
            paramLayout.setTabLabel(page);
        }
    }

    public void selectPage(String page) {
        setDebug("selectPage("+page+") index "+listModel.indexOf(page));
        if (paramLayout != null && listModel.contains(page)){
            tabList.setSelectedValue(page, true);
        } else if (paramLayout != null ) {
            paramLayout.saveSelectedPage(page);
        }
    }

    public ParamLayout getParamLayout() {
        return paramLayout;
    }

    public void setBorderGap(int x, int w) {
        border.setGap(x, x + w);
    }

    public void updateStatus(String msg) {
        if (paramLayout != null)
            paramLayout.updateStatus(msg);
    }

    /** call hide command for outgoing panel. */
    public void hidePage() {
        if(paramLayout!=null)
            paramLayout.hideGroup();
    }
    /** call show command for incoming panel. */
    public void showPage() {
        if(paramLayout!=null)
            paramLayout.showGroup();
    }

    public void updateValue (Vector parms) {
        if (paramLayout == null)
            return;
        if (!isShowing()) {
            isNewPanel = true;
            return;
        }
        setDebug("updateValue(params)");
        paramLayout.updateValue(parms);
    }

    public void updateAllValue () {
        setDebug("updateAllValue()");
        if (paramLayout == null || !isNewPanel)
            return;
        isNewPanel = false;
        paramLayout.updateAllValue();
    }

    public void updateAllValue (boolean always) {
        setDebug("updateAllValue(always)");
        if (paramLayout == null)
            return;
        if (!always && !isNewPanel)
            return;
        if (!isShowing()) {
            isNewPanel = true;
            return;
        }
        isNewPanel = false;
        paramLayout.updateAllValue();
    }

    public ParamLayout getViewPanel() {
        return paramLayout;
    }

    public void freeObj() {
        if (paramLayout != null)
            paramLayout.freeObj();

        paramLayout = null;
        scrollPane = null; viewPort = null;
        headerPop = null; border = null;
        tabList = null; listModel = null;
        listScrollPane = null; vnmrIf = null;
    }

    public void adjustViewBounds() {
        Rectangle rect = paramLayout.getSelectedTabPos();
        Dimension dim = paramLayout.getActualSize();

        Point pt = new Point(rect.x, rect.y);
        int h = dim.height - rect.y;
        Rectangle rect2 = viewPort.getBounds();

        if (rect2.height >= dim.height)
            pt.y = 0;
        else {
            h = rect2.height - h;
            if (h > 0)
                pt.y = pt.y - h;
        }

        if (pt.y < 0)
            pt.y = 0;
        if (pt.x <= 10)
            pt.x = 0;
        else
            pt.x = rect.x - 10;
        rect = paramLayout.getBounds();
        if (pt.x + rect2.width > rect.width)
            dim.width = pt.x + rect2.width;

        viewPort.setViewSize(dim);
        paramLayout.setPreferredSize(dim);
        viewPort.setViewPosition(pt);
    }

    public void setEditMode(boolean s) {
        int edits=ParamInfo.getEditItems();
        if(s && (edits & ParamInfo.PARAMPANELS)==0)
            s=false;
        boolean oldEditMode = isEditMode;
        isEditMode = s;
        setDebug("setEditMode("+s+")");

        if (paramLayout != null && oldEditMode != s) {
            paramLayout.setEditMode(isEditMode);
        }
    }

    public int getViewHeight() {
        return viewHeight;
    }

    public int getViewWidth() {
        return viewWidth;
    }

    /**
     * Return a "limited" x position such that the viewport would never
     * try to show past the right edge of the params panel.
     * @return x pos
     */
    private int limitViewXPos(int x) {
        x = Math.min(x, paramLayout.getSize().width -
                     scrollPane.getViewportBorderBounds().width);
        return Math.max(x, 0);
    } // limitViewXPos()


    /**
     * A JList that manages Drag & Drop actions in the Tab window
     */
    class TabList extends JList
    implements DragGestureListener,
               DragSourceListener,
               DropTargetListener,
               MouseListener
    {
        DragSource dragSource;
        DropTarget dropTarget;
        int        selectedIndex=0;

        public TabList(DefaultListModel model) {
            super(model);
            setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            listFont = DisplayOptions.getFont("", "Menu1", ""); 
            listColor = DisplayOptions.getColor("Menu1"); 
            if (listFont == null)
               listFont = UIManager.getFont("VJ.font");
            if (listFont != null) {
               FontMetrics fm = getFontMetrics(listFont);
               fontHeight = fm.getHeight();
               // setCellRenderer(new TabListRenderer());
            }
            setValueIsAdjusting(true);
            addMouseListener(TabList.this);
            dragSource = new DragSource();
            dropTarget = new DropTarget(TabList.this,TabList.this);
            dragSource.createDefaultDragGestureRecognizer(TabList.this,
                                    DnDConstants.ACTION_MOVE,
                                    TabList.this
                                    );
        }
        public void paintComponent(Graphics g) {
            /*
             * Override the paintComponent method to synchronize
             * on the ListModel.  Without this we can get exceptions
             * when Java tries to paint us while the model is being
             * changed.  Since repainting can happen any time, this
             * must be a pretty general problem if the model is
             * mutable.  See also synchronization on listModel in
             * ParamLayout.clearMenuList().
             */
            try {
                synchronized (getModel()) {
                    super.paintComponent(g);
                }
            } catch (NullPointerException npe) {
                // getModel() was probably null
                if (getModel() == null) {
                    super.paintComponent(g);
                } else {
                    Messages.writeStackTrace(npe);
                }
            }
        }

        public boolean isRequestFocusEnabled()
        {
            CommandInput cmdLine = Util.getAppIF().commandArea;
            if (cmdLine != null && cmdLine.isShowing())
            {
                vnmrIf.setInputFocus();
                return false;
            }
            return super.isRequestFocusEnabled();
        }

        public void setSelectedValue(String name, boolean b){
            setDebug("setSelectedValue("+name+")");
            super.setSelectedValue(name,b);
            selectedIndex=getSelectedIndex();
            select();
        }

        public void setSelectedIndex(int index){
            setDebug("setSelectedIndex("+index+")");
            selectedIndex=index;
            super.setSelectedIndex(index);
            select();
        }

        public Dimension getMaximumSize() {
            return new Dimension(200, super.getMaximumSize().height);
        }

        // MouseListener interface

        public void mouseClicked(MouseEvent e){ }
        public void mouseEntered(MouseEvent e){ }
        public void mouseExited(MouseEvent e){ }
        public void mouseReleased(MouseEvent e){
            Point p = new Point(e.getX(), e.getY());
            int i=locationToIndex(p);
            setDebug("mouseReleased("+i+")");
            if(i>=0 && !isEditMode)
                setSelectedIndex(i);
        }
        public void mousePressed(MouseEvent e){
            Point p = new Point(e.getX(), e.getY());
            int i=locationToIndex(p);
            setDebug("mousePressed("+i+")");
            if(i>=0 && isEditMode)
                setSelectedIndex(i);
        }

        private void select() {
            String s=getSelectedValue().toString();
            if(s==null)
                return;
            paramLayout.selectTab(s);
            adjustViewBounds();
        }

        // DragGestureListener interface

        public void dragGestureRecognized(DragGestureEvent evt){
            if(isEditMode){
                LocalRefSelection ref = new LocalRefSelection(TabList.this);
                setSelectedIndex(selectedIndex);
                dragSource.startDrag(evt, DragSource.DefaultMoveDrop, ref,
                                 TabList.this);
            }
        }

        // DragSourceListener interface

        public void dragDropEnd (DragSourceDropEvent evt) {  }
        public void dragEnter (DragSourceDragEvent evt) { }
        public void dragExit (DragSourceEvent evt) {}
        public void dragOver (DragSourceDragEvent evt) { }
        public void dropActionChanged (DragSourceDragEvent evt) { }

        // DropTargetListener interface

        public void dragEnter(DropTargetDragEvent e) {}
        public void dragExit(DropTargetEvent e) {}
        public void dragOver(DropTargetDragEvent e) {}
        public void dropActionChanged (DropTargetDragEvent e) {}
        public void drop(DropTargetDropEvent e) {
        try {
            Transferable tr = e.getTransferable();
            Point  pte = e.getLocation();
            int row=locationToIndex(pte);
            if(row<0 || row>listModel.getSize()-1)
                row=listModel.getSize()-1;
            Object obj;

            if (tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR) ||
                tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {

                // Catch drag of String (probably from JFileChooser)
                // Create a ShufflerItem and continue with code below this.
                if(tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                    obj = tr.getTransferData(DataFlavor.stringFlavor);

                    // The object, being a String, is the fullpath of the
                    // dragged item.
                    String fullpath = (String)obj;
                    File file = new File(fullpath);
                    if(!file.exists()) {
                        Messages.postError("File not found " + fullpath);
                        return;
                    }
                    // If this is not a locator recognized objType, then
                    // disallow the drag.
// Allow unknown to get to locaction, the user may want to do something there.
//                    String objType = FillDBManager.getType(fullpath);
//                     if(objType.equals("?")) {
//                         Messages.postError("Unrecognized drop item " + 
//                                            fullpath);
//                         return;
//                     }

                    // This assumes that the browser is the only place that
                    // a string is dragged from.  If more places come into
                    // being, we will have to figure out what to do
                    ShufflerItem item = new ShufflerItem(fullpath, "BROWSER");
                    // Replace the obj with the ShufflerItem, and continue
                    // below.
                    obj = item;
                }
                // Not string, get the dragged object
                else {
                    obj = tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
                }

                if (obj instanceof ShufflerItem) {
                    ShufflerItem item = (ShufflerItem) obj;

                    if(isEditMode && item.objType.equals(Shuf.DB_PANELSNCOMPONENTS)) {
                        e.acceptDrop(DnDConstants.ACTION_MOVE);
                        e.getDropTargetContext().dropComplete(true);

                        // check for drag of tabgroup into menu

                        JComponent comp=null;
                        try{
                            comp=LayoutBuilder.build(paramLayout,vnmrIf,item.getFullpath(),0,0);
                        }
                        catch(Exception be) {
                            Messages.writeStackTrace(be);
                        }
                        if ((comp == null) || !(comp instanceof VObjIF))
                            return;
                        if(ParamLayout.isTabGroup(comp)){
                            comp.setVisible(true);
                            ((VObjIF)comp).setEditMode(true);
                            comp.setLocation(0, 0);
                            ParamInfo.setItemEdited(paramLayout);
                            paramLayout.rebuildTabs();
                            paramLayout.setSelected(comp);
                        }
                        else{
                            paramLayout.remove(comp);
                        }
                        return;
                    }
                }
                else if (!isEditMode && obj instanceof VTreeNodeElement) {
                    VElement elem=(VTreeNodeElement)obj;
                    e.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
                    ProtocolBuilder mgr=(ProtocolBuilder)elem.getTemplate();
                    if(mgr !=null)
                        mgr.setDragAndDrop();
                    e.getDropTargetContext().dropComplete(true);
                    return;
                }
                else if (isEditMode && obj instanceof TabList) {
                    e.acceptDrop(DnDConstants.ACTION_MOVE);
                    e.getDropTargetContext().dropComplete(true);
                    String s=getSelectedValue().toString();
                    paramLayout.moveTab(s,row);
                    selectedIndex=row;
                    setSelectedIndex(row);
                    return;
                }
              }
            } catch (IOException io) {
                io.printStackTrace();
            } catch (UnsupportedFlavorException ufe) {
                ufe.printStackTrace();
            }
            e.rejectDrop();
        } // drop

        // TabList classes

        class TabListRenderer extends JLabel implements ListCellRenderer {

            public TabListRenderer() {
                setOpaque(true);
            }

            public Component getListCellRendererComponent(
                JList list,
                Object value,
                int index,
                boolean isSelected,
                boolean cellHasFocus)
            {
                   String name=value.toString();
                   setText(name);
                   if (isSelected) {
                      // setBackground(Util.getSelectBg());
                      // setForeground(Util.getSelectFg());
                      // setForeground(list.getSelectionForeground());
                      setBackground(list.getSelectionBackground());
                      setForeground(listColor);
                   }
                   else {
                      // setForeground(Util.getPanelFg());
                      // setBackground(list.getBackground());
                      // setForeground(list.getForeground());
                      setBackground(Util.getBgColor());
                      setForeground(listColor);
                   }
                   setFont(listFont);
                   return this;
            }

            public Dimension getPreferredSize() {
                Dimension dim = super.getPreferredSize();
                if (dim.height > 60 || dim.height < 6)
                     dim.height = fontHeight + 2;
                return dim;
            }
        } // class TabListRenderer
     } // class TabList

    /**
     * The layout manager for AcquirePanel.  By defining this layout
     * as an inner class of AcquirePanel, much redundancy is reduced.
     */
    class pPanelLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

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

        /**
         * do the layout
         * @param target component to be laid out
         */
        public void layoutContainer(Container target) {
            /* Algorithm is as follows:
             *   - the "left-hand" components take 33% of horizontal space
             *   - scrollPane takes the remaining space
             */
            synchronized (target.getTreeLock()) {
                if (paramLayout == null)
                    return;
                Dimension targetSize = target.getSize();
                Insets insets = target.getInsets();
/*
                Dimension headerDim = headerPop.getPreferredSize();
*/
                Dimension dim0 = new Dimension(0, 0);

                // horizontal partitions

                int h0 = insets.left;
                int h6 = targetSize.width - insets.right;

                // vertical partitions

                int v0 = insets.top + 4;
                int v9 = targetSize.height - insets.bottom;

                if (h6 < 0 || v9 < 0)
                    return;
                int hParam;
                hParam = h0;
                viewHeight = v9 - v0;
                if (listModel.size() > 0) {
                    dim0 = listScrollPane.getPreferredSize();
                    hParam = h0 + dim0.width;
                    listScrollPane.setBounds(new Rectangle(h0, v0, dim0.width, viewHeight));
                    if (!listScrollPane.isVisible()) {
                        listScrollPane.setVisible(true);
                        listScrollPane.revalidate();
                    }
/*
                    dim2 = listScrollPane.getPreferredSize();
                    if (dim2.width != dim0.width) {
                        listScrollPane.setBounds(new Rectangle(h0, v0, dim2.width, viewHeight));
                        hParam = h0 + dim2.width;
                        listScrollPane.validate();
                    }
*/
                }
                else
                    listScrollPane.setVisible(false);
                viewWidth = h6 - hParam;

                //setDebug("layoutContainer "+hParam+" "+viewWidth+" "+listModel.size());

                scrollPane.
                    setBounds(new Rectangle(hParam, v0, viewWidth, viewHeight));
/*
                headerPop.
                    setBounds(new Rectangle(h3, v0, h4 - h3, v3 - v0));
*/
                paramLayout.setReferenceSize(viewWidth - 2, viewHeight -2);
                // paramLayout.validate();
                scrollPane.revalidate();
            }
        } // layoutContainer()
    } // class pPanelLayout
} // class ParameterPanel
