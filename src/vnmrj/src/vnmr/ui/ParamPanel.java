/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.util.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.dnd.*;
import java.awt.datatransfer.*;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.*;

import javax.swing.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.shuf.*;
import vnmr.templates.*;

/**
 *  the panel to display parameters.
 */
public class ParamPanel extends JPanel
        implements VObjIF, VEditIF, VObjDef, StatusListenerIF, PropertyChangeListener,EditListenerIF
{
    /** session share */
    protected SessionShare sshare;
    protected int snapGap = 8;
    protected boolean snapFlag = true;
    protected boolean gridFlag = true;
    protected boolean editMode=false;
    protected Color gridColor = Color.gray;
    protected JComponent panel;
    protected VObjIF vobj;
    protected MouseAdapter ml;
    protected ButtonIF VnmrIf;
    protected String   panelName;
    protected String   panelFile;
    protected String   panelType;
    protected String   restoreFile;
    protected String   layoutDir;
    protected VRubberPanLayout paramsLayout;
    protected boolean changeMode=false;
    protected boolean enabled=true;
    protected int type;
    
    public ParamPanel() {
        sshare=Util.getSessionShare();
        VnmrIf=Util.getViewArea().getDefaultExp();
        
        panel=this;
        type=ParamInfo.PARAMPANEL;
        
        paramsLayout = new VRubberPanLayout();
        setLayout(paramsLayout);
        setMouseAdaptor();
        new DropTarget(this, new LayoutDropTargetListener());
        DisplayOptions.addChangeListener(this);
    }
    /**
     * constructor
     * @param sshare session share
     */
    public ParamPanel(SessionShare sshare, ButtonIF vif) {
        this.sshare = sshare;
        this.VnmrIf = vif;
        this.panel = this;
        paramsLayout = new VRubberPanLayout();
        setLayout(paramsLayout);
        
        setMouseAdaptor();

        // drag-and-drop behavior

        new DropTarget(this, new LayoutDropTargetListener());
        DisplayOptions.addChangeListener(this);

    } // ParamPanel()

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt) {
        gridColor=Util.getGridColor();
        setGridColor(gridColor);

    }
    protected void setMouseAdaptor() {
		ml = new MouseAdapter() {
	        public void mouseEntered(MouseEvent me) {
	        	if(editMode)
	        		ParamEditUtil.testNewPanel(ParamPanel.this);
	        }
			public void mouseClicked(MouseEvent evt) {
				int clicks = evt.getClickCount();
				int modifier = evt.getModifiers();
                ParamEditUtil.clickInPanel(evt);
				if ((modifier & InputEvent.BUTTON3_MASK) != 0) {
					ParamEditUtil.menuAction(evt);
				} else if ((modifier & (1 << 4)) != 0) {
					if (clicks >= 2) {
						ParamEditUtil.setEditObj(ParamPanel.this);
					}
				}
			}
		};

    }

    protected void setDebug(String s){
        if(DebugOutput.isSetFor("ParamPanel")){
            Messages.postDebug(new StringBuffer().append("ParamPanel ").append(s).toString());
        }
    }

    /** set file name strings.*/
    public void setFileInfo(ExpPanInfo info) {
        restoreFile = info.fpathIn;   // where first found
        panelName=info.name;          // e.g. Setup
        panelFile=info.fname;         // e.g. sample.xml
        layoutDir=info.layoutDir;     // e.g. dept
        panelType=info.ptype;

        enablePanel(info.bActive);
    }

    public void setType(int t){
        type=t;
    }

    public int getType(){
        return type;
    }

    /** return save directory name.*/
    public String getSaveDir() {
        String dir=FileUtil.savePath("USER/LAYOUT");
        return (new StringBuffer().append(dir).append("/").append(layoutDir).toString());

    }

    /** return panel type.*/
    public String getPanelType() {
        return panelType;
    }
    
    /** return layout directory path.*/
    public String getLayoutName() {
        return layoutDir;
    }

    /** set layout directory name.*/
    public void setLayoutName(String dir) {
        layoutDir=dir;
    }

    /** return default directory name.*/
    public String getDefaultName() {
         return getAttribute(PANEL_FILE);
    }

    /** return default directory name.*/
    public String getDefaultDir() {
        String dir=getDefaultName();
        return FileUtil.savePath(new StringBuffer().append("USER/LAYOUT/").append(dir).toString());
    }

    /** set default directory name.*/
    public void setDefaultName(String dir) {
        setAttribute(PANEL_FILE,dir);
    }

    /** return panel file name.*/
    public String getPanelFile() {
        return panelFile;
    }

    /** set panel file name.*/
    public void setPanelFile(String s) {
        panelFile=s;
    }

    /** return panel name.*/
    public String getPanelName() {
        return panelName;
    }

    /** set panel name.*/
    public void setPanelName(String name) {
        panelName = name;
    }

    /** remove all JComponents.  */
    public void removeAll() {
        destroy();
        super.removeAll();
    }

    public void freeObj() {
        freeLists();
        removeAll();
        sshare = null;
        gridColor = null;
        panel = null;
        vobj = null;
        ml = null;
        VnmrIf = null;
        paramsLayout = null;
    }

    private void freeLists() {
        //groups.removeAllElements();
    }

    public void clearAll() {
        ParamEditUtil.clearDummyObj();
        freeLists();
        removeAll();
        validate();
        repaint();
    }

    public void restore() {
        ParamEditUtil.setEditObj(this);
        setDebug("restore()");
        removeAll();
        try {
            LayoutBuilder.build(panel,VnmrIf,restoreFile);
        }
        catch(Exception e) {
            Messages.postError("error rebuilding panel");
            Messages.writeStackTrace(e);
        }
        validate();
        repaint();
    }

    public void setDefSize(int w, int h) {
    }

    public SessionShare getSession() {
        return this.sshare;
    }

    public Dimension getActualSize() {
        return paramsLayout.preferredLayoutSize(this);
    }
    public void setPreferredLayoutSize(){
        paramsLayout.preferredLayoutSize(this);
    }

	public void adjustSize() {
    	Dimension dim1=getActualSize();
    	Dimension dim2=getSize();
    	int w1=dim1.width;
    	int h1=dim1.height;
    	int w2=dim2.width;
    	int h2=dim2.height;
    	int x=w1>w2?w1:w2;
    	int y=h1>h2?h1:h2;
    	Dimension dim3=new Dimension(x,y);
    	setPreferredSize(dim3);
	}

    public void changeOpaque() {
        if (editMode && gridFlag) {
            if (snapGap > 2) {
                setOpaque(false);
                return;
            }
        }
        setOpaque(true);
    }

    public void setGrid(boolean s) {
        gridFlag = s;
        changeOpaque();
        repaint();
    }

    public void setSnap(boolean s) {
        snapFlag = s;
        ParamEditUtil.setSnap(s);
    }

    public void setSnapGap(int k) {
        if (snapGap != k) {
            snapGap = k;
            changeOpaque();
            repaint();
        }
    }

    public int getSnapGap() {
        return snapGap;
    }

    public void setSnapGap(String s) {
        s.trim();
        if (s.length() < 1)
           return;
        int n = 10;
        try {
            n = Integer.parseInt(s);
        }
        catch (NumberFormatException e) { }

        if (n < 1)
           n = 1;
        setSnapGap(n);
    }

    public void setGridColor(Color c) {
        gridColor = c;
        repaint();
    }

    public Point getRelativeLoc(Component obj) {
        Point pt = obj.getLocation();
        Container pp = obj.getParent();
        while (pp != null) {
            if (pp == this)
                break;
            Point pt2 = pp.getLocation();
            pt.x += pt2.x;
            pt.y += pt2.y;
            pp = pp.getParent();
        }
        return pt;
    }

    public void paint(Graphics g) {
        Dimension size = getPreferredSize();

        if (editMode && gridFlag) {
            if (snapGap > 2) {
                Color c=g.getColor();
                g.setColor(gridColor);
                int x = snapGap;
                while (x < size.width) {
                    g.drawLine(x, 0, x,  size.height);
                    x += snapGap;
                }
                x = snapGap;
                while (x < size.height) {
                    g.drawLine(0, x, size.width, x);
                    x += snapGap;
                }
                g.setColor(c);
            }
        }
        super.paint(g);
    }

    public void setEditModeToAll(boolean s) {
        int k = getComponentCount();
        for (int i = 0; i < k; i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF obj = (VObjIF) comp;
                obj.setEditMode(s);
            }
        }
    }

    public void setVisibleToAll(boolean s) {
        int k = getComponentCount();
        for (int i = 0; i < k; i++) {
            Component comp = getComponent(i);
            comp.setVisible(s);
        }
        setVisible(s);
    }

    /** update StatusListenerIF objects.*/
    public void updateStatus(String msg) {
        int ncomps = getComponentCount();
        for (int i=0; i<ncomps; i++) {
            Component comp = getComponent(i);
            if(!comp.isShowing())
                 continue;
            if (comp instanceof VGroupIF)
                updateGroupStatus((JComponent)comp, msg);
            else if (comp instanceof StatusListenerIF) {
                StatusListenerIF statcomp = (StatusListenerIF)comp;
                statcomp.updateStatus(msg);
            }
        }
    }
    protected void updateGroupStatus(JComponent p, String msg) {
        int ncomps = p.getComponentCount();
        for (int i=0; i<ncomps; i++) {
            Component comp = p.getComponent(i);
            if (comp instanceof VGroupIF) {
                updateGroupStatus((JComponent)comp, msg);
            } else if (comp instanceof StatusListenerIF) {
                StatusListenerIF statcomp = (StatusListenerIF)comp;
                statcomp.updateStatus(msg);
            }
        }
    }

    protected boolean hasVariable(VObjIF obj, Vector params){
        String vars = obj.getAttribute(VARIABLE);
        if (vars == null)
             return false;
        int pnum = params.size();
        StringTokenizer tok = new StringTokenizer(vars, " ,\n");
        while (tok.hasMoreTokens()) {
             String v = tok.nextToken();
             for (int k = 0; k < pnum; k++) {
                 if (v.equals(params.elementAt(k)))
                     return true;
             }
         }
         return false;
    }

    /** update all VObjIF values using a parameter list.*/

    public void updateValue (Vector params) {
        int nums = getComponentCount();

        setDebug(new StringBuffer().append("updateValue(").append(params.firstElement()).
                 append(", ..)").toString());

        for (int i = 0; i < nums; i++) {
            Component comp = getComponent(i);
            if (!(comp instanceof VObjIF))
                continue;
            VObjIF obj = (VObjIF) comp;
            if (comp instanceof ExpListenerIF) {
                ((ExpListenerIF)comp).updateValue(params);
            }
            else if(hasVariable(obj,params)){
                 obj.updateValue();
           }
        }
    }

    /** update all all group VObjIF values.*/
    public void updateAllValue() {
        setDebug("updateAllValue()");
        try {
            int nums = getComponentCount();
            for (int i = 0; i < nums; i++) {
                Component comp = getComponent(i);
                if (comp instanceof VObjIF) {
                    // NB: If comp is a VGroup, it will update its children.
                    ((VObjIF)comp).updateValue();
                }
            }
        } catch (Exception e) {
            Messages.writeStackTrace(e, "Error updating panel");
        }
    }

    /** update all group VObjIF values.*/
    protected void updateGrpAllValue (VGroup grp) {
        setDebug(new StringBuffer().append("updateGrpAllValue(").
                 append(grp.getAttribute(LABEL)).append(")").toString());
        grp.updateValue();
   }

    public void setReferenceSize(int w, int h) {
        paramsLayout.setReferenceSize(w, h);
    }

    public void setSizeRatio(double xr,double yr){
        paramsLayout.setSizeRatio(xr,yr);
    }

    /** enable or disable panel controls. */
    public void enablePanel(boolean enable) {
        if (enabled != enable) {
           enabled=enable;
//           int nLength = groups.size();
//           for (int i = 0; i < nLength; i++) {
//               VGroup grp = (VGroup)groups.get(i);
//               grp.enableChildren(enable);
//           }
       }
    }



    /** test if a Component is a tab (VTab or VGroup). */
    static public boolean isTab(Component obj) {
        return false;
    }

    /** test if a Component is a VTab.*/
    static public boolean isTabComp(Component obj) {
        return false;
    }

    /** test if a Component is a tabgroup (VGroup with tab and label).*/
    static public boolean isTabGroup(Component obj) {
        if (obj instanceof VGroup){
            if(((VGroup)obj).isTabGroup())
                return true;
        }
        return false;    
        }

    /** return true if comp is the currently selected tab group.*/
    public boolean isSelectedTabGroup(Component comp) {
         return false;
    }

    public void removeTab(VObjIF obj){
        
    }
    public ParameterPanel getParameterPanel() {
        return null;
    }
    public boolean isTabLabel(String name) {
        return false;
    }

    /** add a component by parsing an XML file.*/
    public void pasteXmlObj(String file, int x, int y) {
        JComponent comp=null;
        try{
            comp=LayoutBuilder.build(panel, VnmrIf, file, x, y);
        }
        catch(Exception be) {
            Messages.writeStackTrace(be);
        }
        if ((comp == null) || !(comp instanceof VObjIF))
            return;
        comp.setVisible(true);
        VObjIF vobj = (VObjIF) comp;
        Point pt = getLocationOnScreen();
        x = pt.x + x;
        y = pt.y + y;

        ParamEditUtil.setEditObj(vobj);
        vobj.setEditMode(true);
        comp.setLocation(x, y);
        ParamEditUtil.relocateObj(vobj);
    }

    public VObjIF  reloadContent(String file) {
        
        JPanel pnl=new JPanel();
        try{
            LayoutBuilder.build(pnl, VnmrIf, file);
        }
        catch(Exception be) {
            Messages.postError(new StringBuffer().append("error reloading ").
                               append(file).toString());
            Messages.writeStackTrace(be);
            return null;
        }
        setDebug(new StringBuffer().append("reloadObject(").append(file).
                 append(")").toString());

        int k = getComponentCount();
        int index=0;
        VObjIF obj=null;
        for (index = 0; index < k; index++) {
            Component comp = getComponent(index);
            if (comp instanceof VGroup) {
               obj= (VObjIF)comp;
               break;              
            }
        }
       //VObjIF obj=(VObjIF)firstGroup();
        Component ref=pnl.getComponent(0);
        if (ref == null || obj==null  || !(ref instanceof VObjIF)){
            Messages.postError(new StringBuffer().append("error reloading ").
                               append(file).toString());
            return null;
        }

        obj.destroy();
        removeAll();
        vobj=(VObjIF)ref;
        add(ref);
        ((JComponent)ref).setVisible(true);
        vobj.setEditMode(editMode);

        ParamEditUtil.setEditObj(this);        
        return this;    
    }
    /** reload a reference group.*/
    public VObjIF  reloadObject(VObjIF obj, String file, boolean expand_page, boolean select) {
        return obj;        
    }

    public boolean getEditStatus(){
        return editMode;
    }

    // VObjIF interface

    public void setEditStatus(boolean s) {}
    public void setEditMode(boolean s) {
        setDebug(new StringBuffer().append("setEditMode(").append(s).append(")").toString());
        boolean echange=(s!=editMode);
        paramsLayout.setEditMode(s);
        editMode = s;
        if (editMode) {
            setSnapGap(ParamEditUtil.getSnapGap());
            setGridColor(Util.getGridColor());
            addMouseListener(ml);
            changeOpaque();
        }
        else{
            ParamEditUtil.setEditObj(null);
            removeMouseListener(ml);
            setOpaque(true);
        }
        if(echange){
            setEditModeToAll(editMode);
            if(!editMode)
                updateAllValue();
        }
    }
    public void addDefChoice(String c) {
        if (vobj != null)
           vobj.addDefChoice(c);
    }

    public void addDefValue(String c) {
        if (vobj != null)
           vobj.addDefValue(c);
    }

    public void setDefLabel(String c) { }

    public void setDefColor(String c) {
        if (vobj != null)
           vobj.setDefColor(c);
    }

    public void setDefLoc(int x, int y) {
    }

    public void refresh() {}
    public void changeFont() {}
    public void updateValue() {}
    public void setValue(ParamIF p) {}
    public void setShowValue(ParamIF p) {}
    public void changeFocus(boolean s) {}
    public ButtonIF getVnmrIF() {
        return VnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        VnmrIf=vif;
    }

    /** call "destructors" of VObjIF children  */
    public void destroy() {
        for (int i=0; i<getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF)
                ((VObjIF)comp).destroy();
        }
    }

    public void setModalMode(boolean s){}
    public void sendVnmrCmd() {}
   // public void setSizeRatio(double w, double h) {}
    public Point getDefLoc() { return getLocation();}

    // VObjDef interface

    private Object[][] attributes = {
    {new Integer(PANEL_FILE),  "Default Directory:"},
    };
    public Object[][] getAttributes() { return attributes; }
    public void setAttributes(Object[][] a) { attributes=a; }
    public void setAttribute(int attr, String c){
        switch (attr) {
        case PANEL_FILE:
            if(layoutDir == null)
                break;
            if(c==null || c.length()==0)
                FileUtil.removeUserDefaultFile(layoutDir);
            else if(c.equals(FileUtil.DFLTDIR)){
                if(FileUtil.hasUserDefaultFile(layoutDir))
                    FileUtil.removeUserDefaultFile(layoutDir);
                else{
                    String dname=FileUtil.getDefaultName(layoutDir);
                    if(dname!=null && !dname.equals(FileUtil.DFLTDIR))
                        FileUtil.setDefaultName(layoutDir,c);
                }
            }
            else{
                String dname=FileUtil.getDefaultName(layoutDir);
                if(dname!=null && !dname.equals(c))
                    FileUtil.setDefaultName(layoutDir,c);
            }
            // remove empty layout directory

            FileUtil.deleteEmptyLayoutDir(layoutDir);
            break;
        }
    }
    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return "panel";
        case PANEL_FILE:
             return FileUtil.getDefaultName(layoutDir);
        default:
            return null;
        }
    }

    public void dropAction(JComponent comp,int x,int y){
        if ((comp == null) || !(comp instanceof VObjIF))
            return;
        VObjIF vobj = (VObjIF) comp;
        vobj.setEditMode(editMode);
        comp.setLocation(x, y);
        ParamEditUtil.setEditObj (vobj);
        ParamEditUtil.relocateObj(vobj);
    }
    class LayoutDropTargetListener implements DropTargetListener {
    public void dragEnter(DropTargetDragEvent e) {}
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent evt) {}

    public void drop(DropTargetDropEvent e) {
        try {
                Transferable tr = e.getTransferable();
                // Catch drag of String (probably from JFileChooser)
                if (tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                    Object obj = tr.getTransferData(DataFlavor.stringFlavor);
                    e.acceptDrop(DnDConstants.ACTION_COPY);
                    e.getDropTargetContext().dropComplete(true);

                    // The object, being a String, is the fullpath of the
                    // dragged item.
                    String fullpath = (String) obj;

                    File file = new File(fullpath);
                    if (!file.exists()) {
                        Messages.postError("File not found " + fullpath);
                        return;
                    }

                    // If this is not a locator recognized objType, then
                    // disallow the drag.

                    // Create a ShufflerItem
                    // Since the code is accustom to dealing with ShufflerItem's
                    // being D&D, I will just use that type and fill it in.
                    // This allows ShufflerItem.actOnThisItem() and the macro
                    // locaction to operate normally and take care of this item.
                    ShufflerItem item = new ShufflerItem(fullpath, "BROWSER");

                    if (item.objType.equals(Shuf.DB_PANELSNCOMPONENTS)) {
                        if (editMode) {
                            if (obj instanceof VTreeNodeElement) {
                                e.rejectDrop();
                                return;
                            }
                            Point pt = getLocationOnScreen();
                            Point pte = e.getLocation();
                            int x = pt.x + pte.x - 10;
                            int y = pt.y + pte.y - 10;
                            if (x < 0)
                                x = 0;
                            if (y < 0)
                                y = 0;
                            e.acceptDrop(DnDConstants.ACTION_MOVE);
                            e.getDropTargetContext().dropComplete(true);

                            // ** Code to get dropped item info. (GRS)

                            JComponent comp = null;
                            try {
                                comp = LayoutBuilder.build(panel, VnmrIf, item
                                        .getFullpath(), x, y);
                            } catch (Exception be) {
                                Messages.writeStackTrace(be);
                            }
                            if ((comp == null) || !(comp instanceof VObjIF))
                                return;

                            comp.setVisible(true);
                            vobj = (VObjIF) comp;
                            //vobj.setEditMode(true);
                            dropAction(comp,x,y);
                        }
                    }
                    return;
            }
            // If not a String, then should be in this set of if's
            if (tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR)) {
                Object obj = tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
                if (editMode) {
                    if (obj instanceof VTreeNodeElement) {
                        e.rejectDrop();
                        return;
                    }
                    Point pt = getLocationOnScreen();
                    Point pte = e.getLocation();
                    int x = pt.x + pte.x - 10;
                    int y = pt.y + pte.y - 10;
                    if (x < 0) x = 0;
                    if (y < 0) y = 0;
                    if (obj instanceof VObjIF) {          // drag in Panel
                        e.acceptDrop(DnDConstants.ACTION_MOVE);
                        JComponent comp = (JComponent)obj;
                        e.getDropTargetContext().dropComplete(true);
                        comp.setLocation(x, y);
                        ParamEditUtil.relocateObj((VObjIF) comp);
                        return;
                    }
                    if(obj instanceof ShufflerItem) {     // drag from Shuffler window
                        // Get the ShufflerItem which was dropped.
                        ShufflerItem item = (ShufflerItem) obj;
                        if (item.objType.equals(Shuf.DB_PANELSNCOMPONENTS)) {
                            e.acceptDrop(DnDConstants.ACTION_MOVE);
                            e.getDropTargetContext().dropComplete(true);

                            // ** Code to get dropped item info. (GRS)

                            JComponent comp=null;
                            try{
                                comp=LayoutBuilder.build(panel,VnmrIf,item.getFullpath(),x,y);
                            }
                            catch(Exception be) {
                                Messages.writeStackTrace(be);
                            }
                            if ((comp == null) || !(comp instanceof VObjIF))
                                return;

                            comp.setVisible(true);
                            vobj = (VObjIF)comp;
                            //vobj.setEditMode(true);
                            dropAction(comp,x,y);  // do special post drop actions
                        }
                        return;
                    }
                } // editMode
                else{
                     if (obj instanceof VTreeNodeElement) {
                         VElement elem=(VTreeNodeElement)obj;
                         e.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
                         ProtocolBuilder mgr=(ProtocolBuilder)elem.getTemplate();
                         if(mgr !=null)
                            mgr.setDragAndDrop();
                         e.getDropTargetContext().dropComplete(true);
                         return;
                     }
                }
            }
        } catch (IOException io) {
            io.printStackTrace();
        } catch (UnsupportedFlavorException ufe) {
            ufe.printStackTrace();
        }
        e.rejectDrop();
     }
    } // LayoutDropTargetListener

} // class ParamPanel
