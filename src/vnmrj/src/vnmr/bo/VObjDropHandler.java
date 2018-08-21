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
import java.awt.event.*;
import java.awt.dnd.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.ui.shuf.*;
import vnmr.templates.*;

// This is a generic handler to deal with the object dropped into 
// parameter panel.

public abstract class  VObjDropHandler {

   public static synchronized 
   void processDrop(DropTargetDropEvent e, JComponent d) {
       processDrop(e,d,true);
   } // processDrop

   public static synchronized 
   void processDrop(DropTargetDropEvent e, JComponent d, boolean editMode) {
        // d is the object which got the drop event
        Point pt, pte;
        JComponent comp;
        ParamPanel panel;
        int   x, y;
        try {
            Transferable tr = e.getTransferable();
            if (!tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR) &&
                !tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                e.rejectDrop();
                return;
            }
            Object obj;
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

            if (editMode) {
                comp = (JComponent)d;
                Container cont = (Container)comp.getParent();
                while (cont != null) {
                      if (cont instanceof ParamPanel)
                          break;
                      cont = cont.getParent();
                }
                if (cont == null) {
                    e.rejectDrop();
                    return;
                }
                panel = (ParamPanel) cont;
                pt = d.getLocationOnScreen();
                pte = e.getLocation();
                // the x and y are relative to the ParamLayout
                x = pt.x + pte.x - 10;
                y = pt.y + pte.y - 10;
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                if (obj instanceof VObjIF) {  // drag within panel window
                    e.acceptDrop(DnDConstants.ACTION_MOVE);
                    comp = (JComponent)obj;
                    e.getDropTargetContext().dropComplete(true);
                    comp.getParent().remove(comp);
                    cont.add(comp);
                    comp.setLocation(x, y);
                    ParamEditUtil.relocateObj((VObjIF) comp);
                    return;
                }
                // drag from Shuffler window
                if(obj instanceof ShufflerItem) {
                    // Get the ShufflerItem which was dropped.
                    ShufflerItem item = (ShufflerItem) obj;
                    if (item.objType.equals(Shuf.DB_PANELSNCOMPONENTS)) {
                        e.acceptDrop(DnDConstants.ACTION_COPY);
                        e.getDropTargetContext().dropComplete(true);
                        VObjIF vobj = (VObjIF) d;
                        comp = null;
                        if (x < 0) x = 0;
                        if (y < 0) y = 0;
                        try{
                            comp=LayoutBuilder.build ((JComponent)cont, 
                                       vobj.getVnmrIF(),item.getFullpath(),x,y);
                        }
                        catch(Exception be) {
                            Messages.writeStackTrace(be);
                        }
                        if ((comp == null) || !(comp instanceof VObjIF))
                            return;
                        //vobj = (VObjIF) comp;
                        panel.dropAction(comp,x,y);
//                        if(ParamLayout.isTabGroup(comp)){
//                            cont.remove(comp);
//                            panel.add(comp);
//                            comp.setLocation(0, 0);
//                            vobj.setEditMode(true);
//                            panel.rebuildTabs();
//                            panel.setSelected(comp);
//                        }
//                        else{
//                            vobj.setEditMode(true);
//                            comp.setLocation(x, y);
//                            ParamEditUtil.setEditObj (vobj);
//                            ParamEditUtil.relocateObj(vobj);
//                            if(panel.isTab(comp))
//                                panel.rebuildTabs();
//                        }
//                        return;
                    }
                }
            } 
            else  { // !editMode
                VShimSet shimset = inShimSet(d);
                if (obj instanceof VTreeNodeElement) { 
                    VElement elem=(VTreeNodeElement)obj;
                    e.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
                    ProtocolBuilder mgr=(ProtocolBuilder)elem.getTemplate();
                    if(mgr !=null)
                        mgr.setDragAndDrop();
                    e.getDropTargetContext().dropComplete(true);
                    return;
                } else if (shimset != null) {
                    // Process drops on ShimSets
                    shimset.setBackground(shimset.getSaveBackground());
                    shimset.setOpaque(false);
                    if(obj instanceof ShufflerItem) {
                        // Get the ShufflerItem that was dropped.
                        ShufflerItem item = (ShufflerItem) obj;
                        if (item.objType.equals(Shuf.DB_PARAM)
                            || item.objType.equals(Shuf.DB_VNMR_DATA))
                        { 
                            String newFullpath = item.getFullpath() + 
                                              File.separator + "procpar";
                            item.setFullpath(newFullpath);
                        } 
                        else if (item.objType.equals(Shuf.DB_SHIMS)) {
                            // item.fullpath = item.fullpath;
                        }
                        else {
                            return;
                        }
                        e.acceptDrop(DnDConstants.ACTION_MOVE);
                        item.actOnThisItem("ShimPanel", "DragNDrop", "");
                        e.getDropTargetContext().dropComplete(true);
                    }
                }
            }
        } // try 
        catch (IOException io) {}
        catch (UnsupportedFlavorException ufe) { }
        e.rejectDrop();
    } // processDrop

   public static synchronized 
   void processDragEnter(DropTargetDragEvent dtde,
                         JComponent d,
                         boolean editMode) {
       VShimSet shimset = inShimSet(d);
       if (shimset != null) {
           if (dtde.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR) ||
                   dtde.isDataFlavorSupported(DataFlavor.stringFlavor)) {
               shimset.setDropOK(true);
               Color svBkg = shimset.getSaveBackground();
               if (svBkg == null) {
                   svBkg = shimset.getBackground();
                   shimset.setSaveBackground(svBkg);
               }
               shimset.setBackground(svBkg.darker());
               shimset.setOpaque(true);
           } else {
               dtde.rejectDrag();
           }
       }
   }


    public static synchronized
    void processDragExit(DropTargetEvent dte,
                         JComponent d,
                         boolean editMode) {
        VShimSet shimset = inShimSet(d);
        if (shimset != null && shimset.getDropOK()) {
            shimset.setDropOK(false);
            shimset.setBackground(shimset.getSaveBackground());
            shimset.setOpaque(false);
        }
    }
       
       
    /**
     * Determine if a component is contiained in a VShimSet group.
     * @param comp The component we are asking about.
     * @return The VShimSet group that contains it, or null.
     */
    static public VShimSet inShimSet(Container comp) {
        for (Container p=comp; p != null ; p=p.getParent()) {
            if (p instanceof VShimSet) {
                return (VShimSet)p;
            }
        }
        return null;
    }
}
