/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.awt.*;
import java.beans.*;
import javax.swing.*;
import java.util.*;
import java.awt.dnd.*;
import java.awt.event.*;
import javax.swing.border.*;
import java.awt.datatransfer.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.vobj.*;
import vnmr.admin.util.*;


/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 * Company:
 * @author
 * @version 1.0
 */

public class WItem extends JButton implements WObjIF, DragSourceListener, DragGestureListener,
                                                DropTargetListener
{
    /** The area that this item belongs to i.e. itemArea1 or itemArea2. */
    protected String m_strArea;

    /** The directory path that contains the info for this item. */
    protected String m_strInfoDir;

    /** The item area that this item belongs to. */
    protected VItemAreaIF m_objItemArea = null;

    protected static WItem m_itemDragged = null;

    /** Border variables.  */
    protected Border m_bdrBevel = BorderFactory.createBevelBorder(BevelBorder.RAISED);
    protected Border m_bdrEtched = BorderFactory.createEtchedBorder();
    protected Border m_bdrEmpty = null;
    protected Border m_bdrLine = null;

    /** Property Change Support object.  */
    protected static PropertyChangeSupport m_pcsTypesMgr;

    protected String m_strBgColor = null;

    /** The background color. */
    protected Color m_bgColor = Global.BGCOLOR;

    protected JPopupMenu m_objPopMenu;
    protected JMenuItem m_mItemSave = new JMenuItem(SAVE);
    protected JMenuItem m_mItemDelete = new JMenuItem(DELETE);

    /** Constants.  */
    public static final int EMPTYBORDER  = 1;
    public static final int ETCHEDBORDER = 2;
    public static final int LINEBORDER   = 3;
    public static final String SAVE      = vnmr.util.Util.getLabel("blSave");
    public static final String DELETE    = vnmr.util.Util.getLabel("blDelete");

     /* Drag and drop variables */
    protected static DragSource m_objDragSource         = new DragSource();
    protected DragGestureRecognizer m_objDragRecognizer = m_objDragSource.createDefaultDragGestureRecognizer(
                                                            null,
                                                            DnDConstants.ACTION_COPY_OR_MOVE,
                                                            this);
    protected DropTarget m_objDropTarget                = new DropTarget(
                                                                this,  // component
                                                                DnDConstants.ACTION_COPY_OR_MOVE,
                                                                this );  // DropTargetListener


    /**
     *  Constructor.
     */
    public WItem(String strLabel, String strImage, String strArea,
                                  String strInfoDir, VItemAreaIF objParent)
    {
        super(strLabel);

        m_strArea = strArea;
        m_strInfoDir = strInfoDir;
        m_objItemArea = objParent;
        initLayout(strImage);

        m_pcsTypesMgr=new PropertyChangeSupport(this);

        addMouseListener(new MouseAdapter()
        {
            public void  mouseEntered(MouseEvent e)
            {
                //setBorder(m_bdrBevel);
                setBackground(Util.changeBrightness(m_bgColor, 10));
            }

            public void mouseExited(MouseEvent e)
            {
                //setBorder();
                setBackground(m_bgColor);
            }

            public void mouseClicked(MouseEvent e)
            {
                doMouseClickAction(e);
            }
        });

        addActionListener(new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                WItem objSelItem = (WItem)e.getSource();
                String strName = objSelItem.getText();

                if (m_objItemArea != null)
                {
                    int nCompCount = m_objItemArea.getComponentCount();
                    // set the borders of all the other items in the panel
                    // to empty border
                    for (int i = 0; i < nCompCount; i++)
                    {
                        Component comp = m_objItemArea.getComponent(i);
                        if (comp instanceof WItem)
                        {
                            WItem objItem = (WItem)comp;
                            if (strName != null && !strName.equals(objItem.getText()))
                                objItem.setBorder();
                        }
                    }

                }
            }

        });
    }


    /**
     *  Initializes the layout for this item.
     *  @param strImage image for the button.
     */
    protected void initLayout(String strImage)
    {
        ImageIcon icon = Util.getImageIcon(strImage);
        setIcon(icon);
        m_objDragRecognizer.setComponent(this);
        setBackground(m_bgColor);

        Insets insets = m_bdrEtched.getBorderInsets(this);
        m_bdrEmpty = BorderFactory.createEmptyBorder(insets.top, insets.left, insets.bottom, insets.right);
        m_bdrLine = BorderFactory.createLineBorder(Color.green.darker(), insets.left);
        setBorder();

        m_objPopMenu = new JPopupMenu();
        m_mItemSave.setActionCommand(SAVE);
        m_mItemDelete.setActionCommand(DELETE);
        m_objPopMenu.add(m_mItemSave);
        m_objPopMenu.add(m_mItemDelete);

        ActionListener alMenuItem = new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                doMenuItemAction(e);
            }

        };

        m_mItemSave.addActionListener(alMenuItem);
        m_mItemDelete.addActionListener(alMenuItem);

    }

    public String getValue()
    {
        return getText();
    }

    public void setValue(String strValue)
    {
        setText(strValue);
    }

    public String getAttribute(int nAttr)
    {
        switch(nAttr)
        {
            case VObjDef.BGCOLOR:
                return m_strBgColor;
            default:
                return null;
        }
    }

    public void setAttribute(int nAttr, String strValue)
    {
        switch(nAttr)
        {
            case VObjDef.BGCOLOR:
                m_strBgColor = strValue;
                if (strValue != null)
                {
                    m_bgColor = VnmrRgb.getColorByName(strValue);
                    setBgColor(m_bgColor);
                }
                break;
            default:
                break;
        }
    }

    /**
     *  Returns the area string that this item belongs to.
     */
    public String getArea()
    {
        return m_strArea;
    }

    /**
     *  Returns the item area (parent panel) in which this item resides.
     */
    public VItemAreaIF getItemArea()
    {
        return m_objItemArea;
    }

    /**
     *  Returns the directory that contains the file for this item.
     */
    public String getInfoDir()
    {
        return m_strInfoDir;
    }

    /**
     *  Sets the border of this item.
     *  @nBorder    border type to be set.
     */
    public void setBorder(int nBorder)
    {
        switch(nBorder)
        {
            case EMPTYBORDER:
                setBorder(m_bdrEmpty);
                break;
            case ETCHEDBORDER:
                setBorder(m_bdrEtched);
                break;
            case LINEBORDER:
                setBorder(m_bdrLine);
                break;
            default:
                setBorder(m_bdrBevel);
                break;

        }
    }

    /**
     *  Sets the border of this item to empty if this item is not currently
     *  selected in the panel.
     */
    protected void setBorder()
    {
        setBorder(m_bdrBevel);
        if (m_objItemArea != null)
        {
            WItem objItem = m_objItemArea.getItemSel();
            String strName = (objItem != null) ? objItem.getText() : null;
            if (strName != null && strName.equals(getText()))
                setBorder(LINEBORDER);
        }
    }

    public void setBgColor(Color bgColor)
    {
        m_bgColor = bgColor;
        setBackground(m_bgColor);
    }

    /**
     *  Sets the border, if the item selected in the itemArea is the same as this item,
     *  then set the line border else set the empty border.
     */
    protected void doAction()
    {
        if (m_objItemArea != null)
        {
            WItem objItem = m_objItemArea.getItemSel();
            if (objItem != null && objItem.equals(this))
                setBorder(m_bdrLine);
            else
                setBorder(m_bdrBevel);
        }
    }

    /**
     *  For a right mouse click, show the popup menu.
     */
    protected void doMouseClickAction(MouseEvent evt)
    {
        if (WUtil.isRightMouseClick(evt))
        {
            int nXPos = this.getWidth() / 2;
            int nYPos = this.getHeight() / 2;
            m_objPopMenu.show(this, nXPos, nYPos);

            WItem objItemSel = m_objItemArea.getItemSel();
            if (objItemSel != null && objItemSel.equals(this))
                m_mItemSave.setEnabled(true);
            else
                m_mItemSave.setEnabled(false);
        }
        else
        {
            AppIF appIf = Util.getAppIF();
            if (appIf instanceof VAdminIF)
                ((VAdminIF)appIf).getUserToolBar().blinkStop();
            doAction();
        }

    }

    /**
     *  Does the 'save' and 'delete' actions selected from the right click menu.
     */
    protected void doMenuItemAction(ActionEvent e)
    {
        String strAction = e.getActionCommand();
        if (strAction.equals(SAVE))
        {
            String strSave = (m_strArea.equals(WGlobal.AREA1)) ? WGlobal.SAVEUSER
                    : WGlobal.SAVEGROUP;
            VUserToolBar.firePropertyChng(strSave, "All", "");
            // Note, full name in WOperator and full name in DetailArea 
            // should be the same (although they are saved in different files)
            // So here we need to update WOperator full name 
            VAdminIF adminif = (VAdminIF) Util.getAppIF();
            WDialogHandler handler = adminif.getWDialogHandler();

            // update operatorlist file (if full name is different)
            String loginname = adminif.getDetailArea1().getItemValue(WGlobal.NAME);
            String fullname = adminif.getDetailArea1().getItemValue(WGlobal.FULLNAME);
            handler.updateOperatorFullName(loginname, fullname);

            // Bring up the Edit Operators panel so the user can set the
            // desired profile
            PropertyChangeEvent evt = new PropertyChangeEvent(this, "add operators", null, null);
            handler.propertyChange(evt);
        }
        else if (strAction.equals(DELETE))
        {
            m_objItemArea.deleteItem(this);
        }
    }

    /**
     *  Opens and updates the info file for this item.
     *  This method is called from drop().
     *  @param objItem   item to be added to the list.
     */
    protected void writeFiles(WItem objItem)
    {
        /**
         *  Commented out since the first release will not have the group feature.
         */

        // Original File
        /*String strItmDropPath = FileUtil.openPath(m_strInfoDir+getText());
        HashMap hmItem = updateFile(strItmDropPath, "itemlist", objItem.getText(),
                                        objItem.getText(), true);

        if (m_itemDragged != null)
        {
            String strItmDragPath = getUserPath(m_itemDragged.getInfoDir(),
                                                    m_itemDragged.getText());
            hmItem = updateFile(strItmDragPath, "grouplist", getText(),
                                    m_itemDragged.getText(), true);
        }*/
    }

    protected HashMap updateFile(String strPath, String strKeyWord, String strValue,
                                    String strName, boolean bAddToList)
    {
        if (strPath == null)
            return null;

        HashMap hmItem = WFileUtil.getHashMap(strPath);

        String strLine = "";
        String strTmp;
        boolean bListExists = false;
        boolean bHasItem = false;
        try
        {
            /**
             *  Commented out since the first release will not have the group feature.
             */

            // Get the value saved for the key,
            // and if the new value is not in the list, then append it to the list
            // and update the hashmap.
            /*if (hmItem != null && !hmItem.isEmpty())
            {
                String strOldValue = (String)hmItem.get(strKeyWord);
                if (strOldValue == null || strOldValue.equals("null"))
                    strOldValue = "";

                bHasItem = hasItem(strValue, strOldValue);
                if (bAddToList && !bHasItem)
                    strLine = strOldValue + " " + strValue;
                else if (!bAddToList && bHasItem)
                    strLine = removeItem(strName, strOldValue);
                else
                    strLine = strOldValue;

                hmItem.put(strKeyWord, strLine.trim());
                WFileUtil.updateItemFile(strPath, hmItem);
            }*/
        }
        catch(Exception e)
        {
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
        return hmItem;
    }

    /**
     *  Returns true if objItem is already in the file,
     *  and false otherwise.
     *  @param objItem  the item to be compared to.
     *  @param strLine  the 'itemlist' line from the file, that has all the items.
     */
    protected boolean hasItem(String strName, String strLine)
    {
        boolean bHasItem = false;
        StringTokenizer strTokNames = new StringTokenizer(strLine);
        String strNextTok;
        if (strName != null)
            strName = strName.trim();

        while(strTokNames.hasMoreTokens() && !bHasItem)
        {
            strNextTok = strTokNames.nextToken();
            if (strNextTok != null && strNextTok.trim().equals(strName))
                bHasItem = true;
        }

        return bHasItem;
    }

    protected String getUserPath(String strDir, String strName)
    {
        String strPbPath = FileUtil.openPath(strDir+"system/"+strName);
        String strPrvPath = FileUtil.openPath(strDir+"user/"+strName);

        return (strPbPath + File.pathSeparator + strPrvPath);
    }

    protected String removeItem(String strName, String strLine)
    {
        StringTokenizer strTokNames = new StringTokenizer(strLine);
        String strNextTok;
        StringBuffer sbItems = new StringBuffer();

        while(strTokNames.hasMoreTokens())
        {
            strNextTok = strTokNames.nextToken();
            if (!strName.equals(strNextTok))
            {
                sbItems.append(strNextTok);
                sbItems.append(" ");
            }
        }

        return sbItems.toString();
    }

    private void highLight(WItem objItemDrop)
    {
        WItem objSelItem = m_objItemArea.getItemSel();
        if(objSelItem != null && objSelItem.equals(this))
            objItemDrop.setBackground(objItemDrop.getBackground().brighter());
    }

    //==============================================================================
   //   PropertyChange methods follows ...
   //============================================================================

    /**
     *  Adds the specified change listener.
     *  @param l    the property change listener to be added.
     */
    public static void addChangeListener(PropertyChangeListener l)
    {
        if (m_pcsTypesMgr != null)
            m_pcsTypesMgr.addPropertyChangeListener(l);
    }

    /**
     *  Adds the specified change listener with the specified property.
     *  @param strProperty  the property to which the listener should be listening to.
     *  @param l            the property change listener to be added.
     */
    public static void addChangeListener(String strProperty, PropertyChangeListener l)
    {
        if (m_pcsTypesMgr != null)
            m_pcsTypesMgr.addPropertyChangeListener(strProperty, l);
    }

    /**
     *  Removes the specified change listener.
     */
    public static void removeChangeListener(PropertyChangeListener l)
    {
        if(m_pcsTypesMgr != null)
            m_pcsTypesMgr.removePropertyChangeListener(l);
    }


    //===========================================================================
   //           DragGestureListener and DragSourceListener implementations.
   //===========================================================================

   public void dragGestureRecognized( DragGestureEvent e )
   {
        Component comp = e.getComponent();
        m_itemDragged = (WItem)comp;

        LocalRefSelection ref = new LocalRefSelection( comp );
        m_objDragSource.startDrag( e, DragSource.DefaultMoveDrop, ref, this );
    }

    public void dragDropEnd( DragSourceDropEvent e ) {}
    public void dragEnter( DragSourceDragEvent e ) {}
    public void dragExit( DragSourceEvent e ) {}
    public void dragOver( DragSourceDragEvent e ) {}
    public void dropActionChanged( DragSourceDragEvent e ) {}

    //==================================================================
    //   DropTargetListener implementation Follows ...
    //==================================================================

    public void dragEnter( DropTargetDragEvent e ) {}
    public void dragExit( DropTargetEvent e ) {}
    public void dragOver( DropTargetDragEvent e ) {}
    public void dropActionChanged( DropTargetDragEvent e ) {}

    public void drop( DropTargetDropEvent e )
    {
        String dropLabel = "";
        Point pt = e.getLocation();

        try
        {
            Transferable tr = e.getTransferable();
            if( tr.isDataFlavorSupported( LocalRefSelection.LOCALREF_FLAVOR ))
            {
                Object obj = tr.getTransferData( LocalRefSelection.LOCALREF_FLAVOR );
                JComponent comp = ( JComponent ) obj;
                Container container = comp.getParent();

                if ((comp instanceof WItem)
                        && (container instanceof VItemAreaIF))
                {
                    VItemAreaIF pnlItem = (VItemAreaIF)container;
                    if (isPnlDropValid(pnlItem))
                    {
                        WItem objItemDrop = (WItem)comp;
                        if (objItemDrop != null)
                            writeFiles(objItemDrop);

                        highLight(objItemDrop);
                        e.acceptDrop( DnDConstants.ACTION_MOVE );
                        e.getDropTargetContext().dropComplete( true );

                        validate();
                        repaint();
                    }
                }

            }
            return;
        }
        catch( IOException io )
        {
            Messages.writeStackTrace(io);
            e.rejectDrop();
            Messages.postDebug(io.toString());
        }
        catch( UnsupportedFlavorException ufe )
        {
            Messages.writeStackTrace(ufe);
            e.rejectDrop();
            Messages.postDebug(ufe.toString());
        }
    }

    protected boolean isPnlDropValid(VItemAreaIF pnlItem)
    {
        boolean bValid = false;
        if (pnlItem != null)
        {
            String strArea = pnlItem.getArea();
            if (!(strArea.equals(m_strArea))
                    && m_strArea.equals(WGlobal.AREA2))
                bValid = true;
        }
        return bValid;
    }

}
