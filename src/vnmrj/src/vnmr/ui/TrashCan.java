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
import java.awt.datatransfer.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.io.*;
import javax.swing.*;
import java.net.*;
import java.util.ArrayList;

import  vnmr.ui.*;
import  vnmr.ui.shuf.*;
import vnmr.ui.shuf.TrashItem.DeleteQuery;
import  vnmr.util.*;
import  vnmr.bo.*;
import  vnmr.templates.*;
import  vnmr.admin.ui.*;

/**
 * trash can
 *
 */
public class TrashCan extends VButton implements DropTargetListener {
    /**
     * constructor
     */
    private boolean bCanvasEntered = false;
    private VnmrCanvas canvas;

    public TrashCan(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare,vif,typ);
        setBorder(BorderFactory.createEmptyBorder(2, 2, 2, 2));
        setIcon(Util.getImageIcon("trashcan.gif"));
        setVisible(true);
        Util.setTrashCan(this);

        addMouseListener(new MouseAdapter() {
        public void mouseClicked(MouseEvent evt) {
            int clicks = evt.getClickCount();
            if(clicks == 2) {
                AppIF appIf = Util.getAppIF();
                if (appIf instanceof VAdminIF)
                {
                    showAdminTrashCan();
                }
                else
                {
                // We have a double click.
                    if(FillDBManager.locatorOff()){
                		String trashdir=TrashItem.trashDir();
                		if(UtilB.iswindows())
                			trashdir=UtilB.unixPathToWindows(trashdir);
                		ExpPanel.openBrowser(trashdir);
                		return;
                    }
                
                    SwingUtilities.invokeLater(new Runnable() {
                         public void run() {
                             openTrashPanel();
                         }
                    });

                    /*******************
                    SessionShare ssshare = ResultTable.getSshare();
                    LocatorHistoryList lhl = ssshare.getLocatorHistoryList();
                    //AppIF appIf = Util.getAppIF();
                    Shuffler shuffler = Util.getShuffler();
                    // Is there a locator opened?
                    if(shuffler == null) {
                        Messages.postError("You must have a locator displayed "
                                + "to open the Trash.");
                        // No locator, bail out
                        return;
                    }
                    ShufflerToolBar shufflerToolBar = shuffler.shufflerToolBar;

                    // If we are already in trash mode, then have a
                    // double click get us out.
                    String activeHistory = lhl.getLocatorHistoryName();
                    if(activeHistory.equals(LocatorHistoryList.TRASH_LH)) {
                        // Set locator back to standard mode
                        shufflerToolBar.showStdButtons();

                        // Set locator type back to the previous one
                        lhl.setLocatorHistoryToPrev();
                    }
                    else {
                        // Set History Active Object type to this type.
                        // (locator_statements_trash.xml)
                        lhl.setLocatorHistory(LocatorHistoryList.TRASH_LH);

                        // Show Trash Mode Buttons in locator.
                        shufflerToolBar.showTrashButtons();
                    }
                    *************/
                }
            }
        }
    });

        if (Util.getAppIF() instanceof VAdminIF)
        {
            // Give TrashItem a handle to this TrashCan so that it can
            // set the icon to full/empty.
            WTrashItem.setTrashCan(this);
        }
        else
        {
            // Give TrashItem a handle to this TrashCan so that it can
            // set the icon to full/empty.
            TrashItem.setTrashCan(this);

            // Update the trash can icon as necessary.
            TrashItem.updateTrashCanIcon();
        }

    } // TrashCan()

    public void dragEnter(DropTargetDragEvent e) {
        try {
           Transferable tr = e.getTransferable();
           Object obj =
                    tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
           if (obj instanceof VnmrCanvas) {
              canvas = (VnmrCanvas) obj;
              bCanvasEntered = true;
              canvas.enterTrashCan(true);
           } 
        } catch (IOException io) {
            io.printStackTrace();
        } catch (UnsupportedFlavorException ufe) {
//            ufe.printStackTrace();
        }
    }

    public void dragExit(DropTargetEvent e) {
        if (bCanvasEntered) {
             canvas.enterTrashCan(false);
             bCanvasEntered = false;
        } 
    }

    public void dragOver(DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        try {
            Transferable tr = e.getTransferable();
            // Catch drag of String (probably from JFileChooser)
            if(tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                Object obj = tr.getTransferData(DataFlavor.stringFlavor);
                e.acceptDrop(DnDConstants.ACTION_COPY);
                e.getDropTargetContext().dropComplete(true);

                // The object, being a String, is the fullpath of the
                // dragged item.
                String fullpath = (String)obj;

                File file = new File(fullpath);
                if(!file.exists()) {
                    Messages.postError("File not found " + fullpath);
                    return;
                }
                String filename =   file.getName();

                // If this is not a locator recognized objType, then
                // disallow the drag.
                String objType = FillDBManager.getType(fullpath);
//                if(objType.equals("?")) {
//                    Messages.postError("Unrecognized drop item " + 
//                                       fullpath);
//                    return;
//                }

                // Create a ShufflerItem
                // Since the code is accustom to dealing with ShufflerItem's
                // being D&D, I will just use that type and fill it in.
                // This allows ShufflerItem.actOnThisItem() and the macro
                // locaction to operate normally and take care of this item.
                ShufflerItem item = new ShufflerItem(fullpath, "BROWSER");

                TrashItem trash = new TrashItem(item.filename,
                                                item.dpath, item.objType,
                                                item.dhost, item.owner,
                                                item.hostFullpath);

                trash.trashIt();

                // We need to update the Browser.  We don't know which
                // one, so update all of them.
                ExpPanel.updateBrowser();


                return;
            }
            // If not a String, then should be in this set of if's
            if (tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR)) {
                Object obj =
                    tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);

                if (obj instanceof ShufflerItem) {
                    // Get the ShufflerItem which was dropped.
                    ShufflerItem item = (ShufflerItem) obj;

                    // Where did this come from?
                    if(item.source.equals("LOCATOR") || item.source.equals("EXPSELECTOR")) {
                        // The drag came from the shuffler, so send cmd
                        // to vnmr to take care of it.
                        e.acceptDrop(DnDConstants.ACTION_MOVE);
                        e.getDropTargetContext().dropComplete(true);

                        
                        TrashItem trash = new TrashItem(item.filename,
                                              item.dpath, item.objType,
                                              item.dhost, item.owner,
                                              item.hostFullpath);
                        trash.trashIt();

                    }
                    else if(item.source.equals("HOLDING")) {
                        // The drag came from the holdling pen.  Don't
                        // actually remove the file or what ever it is.
                        // We just want to take this item out of the
                        // Holding pen.
                        SessionShare sshare = ResultTable.getSshare();
                        HoldingDataModel holdingDataModel =
                                                sshare.holdingDataModel();
                        holdingDataModel.removeItem(item);
                    }
                }
                if(obj instanceof ArrayList<?>) {
                    ArrayList<?> list = (ArrayList<?>)obj;
                    // We currently only allow an ArrayList of ShufflerItems
                    // If anything else, bail out
                    if(!(list.get(0) instanceof ShufflerItem)) {
                        e.rejectDrop();
                        return;
                    }

                    // Query
                    new TrashItem.DeleteQuery("   Delete Files?   ", this);
                
                    // The result of the popup will be in deleteOkay
                    if(!TrashItem.deleteOkay)  {
                        e.rejectDrop();
                        return;
                    }
                        
                    // Go through the list and do the following operation
                    // for each item in the list
                    for(int i=0; i<list.size(); i++) {
                        ShufflerItem item = (ShufflerItem)list.get(i);
                        
                     // Where did this come from?
                        if(item.source.equals("LOCATOR") || item.source.equals("EXPSELECTOR")) {
                            // The drag came from the shuffler, so send cmd
                            // to vnmr to take care of it.

                            TrashItem trash = new TrashItem(item.filename,
                                                  item.dpath, item.objType,
                                                  item.dhost, item.owner,
                                                  item.hostFullpath);

                            
                            trash.trashIt("delete");
                        }
                        
                    }
                    e.acceptDrop(DnDConstants.ACTION_MOVE);
                    e.getDropTargetContext().dropComplete(true);
                }
		/****
                else if (obj instanceof VnmrXCanvas) {
                    e.acceptDrop(DnDConstants.ACTION_MOVE);
                    VnmrXCanvas canvas = (VnmrXCanvas)obj;
                    Point p = canvas.getDragStartPoint();
                    int but = canvas.getDragMouseBut();
                    VIcon icon = canvas.getIcon(p.x, p.y);
                    if (icon != null && icon.isSelected())
                        canvas.deleteIcon(p.x, p.y);
                    else if(Util.getRQPanel() != null) {
                        RQBuilder mgr=Util.getRQPanel().getReviewQueue().getMgr();
                        mgr.requestRemoveImg(p, but);
                    }
                    e.getDropTargetContext().dropComplete(true);
                    return;
                }
		****/
                else if (obj instanceof VnmrCanvas)
                {
                    canvas = (VnmrCanvas)obj;
                    canvas.dropTrashCan();
/**
                    Point p = canvas.getDragStartPoint();
                    VIcon icon = canvas.getIcon(p.x, p.y);
                    if (icon != null && icon.isSelected())
                        canvas.deleteIcon(p.x, p.y);
                    else if(Util.getRQPanel() != null) {
                        int but = canvas.getDragMouseBut();
                        RQBuilder mgr=Util.getRQPanel().getReviewQueue().getMgr();
                        mgr.requestRemoveImg(p, but);
                    }
**/
                    e.acceptDrop(DnDConstants.ACTION_MOVE);
                    e.getDropTargetContext().dropComplete(true);
                    return;
                }
                else if (obj instanceof VFileElement) {
                    e.acceptDrop(DnDConstants.ACTION_MOVE);
                    VFileElement elem=(VFileElement)obj;
                    RQBuilder mgr=(RQBuilder)elem.getTemplate();
                    e.getDropTargetContext().dropComplete(true);
                    if(mgr !=null) {
                        mgr.removeElement(elem);
                        Util.sendToVnmr("vnmrjcmd('tray', 'update')");
                    }
                    return;
                }
                else if (obj instanceof VTreeNodeElement) {
                    e.acceptDrop(DnDConstants.ACTION_MOVE);
                    VTreeNodeElement elem=(VTreeNodeElement)obj;
                    ProtocolBuilder mgr=(ProtocolBuilder)elem.getTemplate();
                    e.getDropTargetContext().dropComplete(true);
                    if(mgr !=null) {
                        mgr.removeElement(elem);
                        Util.sendToVnmr("vnmrjcmd('tray', 'update')");
                    }
                    return;
                }
                else if (obj instanceof WItem)
                {
                    deleteButton((WItem)obj);
                }
                else if( obj instanceof VToolBarButton )
                {
                    VjToolBar toolbar = null;
                    // ExperimentIF expIF = ( ExperimentIF )Util.getAppIF();
                    VnmrjIF  expIF = Util.getVjIF();
                    if (expIF != null)
                       toolbar = expIF.getToolBar();
                    if (toolbar != null) {
                       JComponent toolcomp = ( JComponent ) obj;
                       JButton tool = ( JButton ) toolcomp;
                       toolbar.deleteTool( tool );
                    }
                }
                else if (obj instanceof VObjIF) {
                    VObjIF vobj=(VObjIF)obj;
                    e.acceptDrop(DnDConstants.ACTION_MOVE);
                    JComponent cobj = (JComponent)obj;
                    JComponent pobj = (JComponent)cobj.getParent();
                    Container pp=pobj;
                    String name=vobj.getAttribute(VObjDef.LABEL);
                    while (pp != null) {
                        if (pp instanceof ParamPanel)
                            break;
                        pp = pp.getParent();
                    }
                    if(pp instanceof ParamPanel){
                        ParamPanel pl=(ParamPanel)pp;
                        vobj.destroy();

                        pobj.remove(cobj);
                        if(pl.isTabLabel(name))
                            pl.removeTab(vobj);
                        else if(pobj instanceof VObjIF)
                            ParamEditUtil.setEditObj((VObjIF)pobj);
                        else
                            ParamEditUtil.setEditObj(null);
                    }
                    else
                        pobj.remove(cobj);
                    e.getDropTargetContext().dropComplete(true);
                    return;
                }
            }
        } catch (IOException io) {
            io.printStackTrace();
        } catch (UnsupportedFlavorException ufe) {
            ufe.printStackTrace();
        }
        e.rejectDrop();
    }

    protected void deleteButton(WItem objItem)
    {
        //JComponent comp = (JComponent)obj;
        AppIF objAppIf = Util.getAppIF();
        if (objAppIf instanceof VAdminIF)
        {
            String strArea = objItem.getArea();
            if (strArea.equals(WGlobal.AREA1))
                ((VAdminIF)objAppIf).getItemArea1().deleteItem(objItem);
            else if (strArea.equals(WGlobal.AREA2))
                ((VAdminIF)objAppIf).getItemArea2().deleteItem(objItem);
        }
    }

    protected void showAdminTrashCan()
    {
        AppIF appIf = Util.getAppIF();
        if (appIf instanceof VAdminIF)
        {
            ((VAdminIF)appIf).showTrashCan(true);
        }
    }

    public void dropActionChanged (DropTargetDragEvent e) {}

    private void openTrashPanel() {
        SessionShare ssshare = ResultTable.getSshare();
        LocatorHistoryList lhl = ssshare.getLocatorHistoryList();
        Shuffler shuffler = Util.getShuffler();

        // Is there a locator opened?
        if(shuffler == null) {
            Messages.postError("You must have a locator displayed "
                    + "to open the Trash.");
            // No locator, bail out
            return;
        }

        ShufflerToolBar shufflerToolBar = shuffler.shufflerToolBar;
        if(shufflerToolBar == null)
            return;

        String activeHistory = lhl.getLocatorHistoryName();
        if(!activeHistory.equals(LocatorHistoryList.TRASH_LH)) {
            // Set History Active Object type to this type.
            lhl.setLocatorHistory(LocatorHistoryList.TRASH_LH);

            // Show Trash Mode Buttons in locator.
            shufflerToolBar.showTrashButtons();
        }
        if (Util.isShufflerInToolPanel()) {
            VnmrjIF vjIf = Util.getVjIF();
            if (vjIf != null) {
                vjIf.openComp("Locator", "open");
                return;
            }
        }
        LocatorPopup locPopup = Util.getLocatorPopup();
        if (locPopup != null) {
            locPopup.setState(Frame.NORMAL);
            locPopup.setVisible(false);
            locPopup.toFront();
            locPopup.setVisible(true);
        }
    }

} // class TrashCan
