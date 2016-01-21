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
import javax.swing.*;

import java.beans.*;
import java.awt.*;
import java.util.*;
import java.awt.dnd.*;
import java.awt.event.*;
import javax.swing.event.*;
import java.awt.datatransfer.*;
import javax.swing.plaf.basic.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.templates.*;
import vnmr.admin.util.*;

/**
 * Title: VUserToolBar
 * Description: The toolbar for the user and the group panel.
 * Copyright:    Copyright (c) 2001
 */

public class VUserToolBar extends JToolBar implements DragSourceListener, DragGestureListener,
                                                        ActionListener, PropertyChangeListener
{
    /** True if the new item is being dragged, false otherwise. */
    protected boolean m_bIsNewItemDrag = false;

    /** The item in the toolbar that could be dragged. */
    protected JButton m_btnDragItem;

    protected JButton m_btnSave;

    static protected WMultiUsers m_dlgMultiUsers = null;

    protected static JComboBox m_cmbView;

    protected JButton m_btnEnable;

    /** The drag source. */
    protected DragSource m_objDragSource = new DragSource();

    /** The action listener for the save action.  */
    protected ActionListener m_alSave;

    /** The action listener for the search action. */
    protected ActionListener m_alSearch;

    /** Property Change Support object.  */
    protected static PropertyChangeSupport m_pcsTypesMgr;

    /** The area that the toolbar corresponds to area1 or area2. */
    protected String m_strArea;

    /** The sessionshare object. */
    protected SessionShare m_sshare;

    /** The background color. */
    protected Color m_bgColor = Global.BGCOLOR;

    protected JComboButton m_cmbUser;

    protected JPopupMenu m_popMenu;

    protected java.util.Timer timer;
    protected String labelString;       // The label for the window
    protected int delay;                // the delay time between blinks

    public static final String BLINK_SAVE = WGlobal.SAVEUSER+WGlobal.SEPERATOR+WGlobal.FONTSCOLORS;
    
    public static final String SINGLE_USER  = vnmr.util.Util.getLabel("_admin_Single_User");
    public static final String MULTI_USERS  = vnmr.util.Util.getLabel("_admin_Multiple_Users");
    public static final String ALL_VJ_USERS = vnmr.util.Util.getLabel("_admin_Show_all_VJ_Users");
    
    public static final String IMG_USERS    = java.text.MessageFormat.format(vnmr.util.Util.getLabel("_admin_Show_Users"), 
            new Object[] {WGlobal.IMGIFLBL});
    public static final String WALKUP_USERS = java.text.MessageFormat.format(vnmr.util.Util.getLabel("_admin_Show_Users"), 
            new Object[] {WGlobal.WALKUPIFLBL});
    public static final String LC_USERS     = java.text.MessageFormat.format(vnmr.util.Util.getLabel("_admin_Show_Users"), 
            new Object[] {WGlobal.LCIFLBL});
      
    public static final String UNIX_USERS   = vnmr.util.Util.getLabel("_admin_Show_non-VJ_Users");
    public static final String DISABLE_ACC  = vnmr.util.Util.getLabel("_admin_Show_disabled_accounts");


    /**
     *  Constructor.
     */
    public VUserToolBar(SessionShare sshare, JButton btnItem, String strArea)
    {
        super();

        m_sshare = sshare;
        m_btnDragItem = btnItem;
        m_strArea = strArea;

        setLayout(new FlowLayout());
        setBackground(m_bgColor);

        doLayout(strArea);
        setOptions();
        m_dlgMultiUsers = new WMultiUsers(sshare);
        m_pcsTypesMgr=new PropertyChangeSupport(this);
        WFontColors.addChangeListener(this);
        initBlink();

        m_btnDragItem.addActionListener(this);
        m_btnDragItem.setActionCommand("new");

        if (m_cmbUser != null)
        {
            final JList listUser = m_cmbUser.getList();
            listUser.addListSelectionListener(new ListSelectionListener()
            {
                public void valueChanged(ListSelectionEvent e)
                {
                    int nSelected = listUser.getMaxSelectionIndex();
                    String strSelection = (String)(listUser.getModel().getElementAt(nSelected));
                    String strText = (strSelection.equals(MULTI_USERS)) ?
                            "<HTML> "+vnmr.util.Util.getLabel("_admin_New_Users")+
                            " <P>("+vnmr.util.Util.getLabel("_admin_2_or_more")+
                             ")</HTML>" : vnmr.util.Util.getLabel("_admin_New_User");
                    String strFile = (strSelection.equals(MULTI_USERS)) ? "newusers" : "newuser";
                    /*ImageIcon icon = new ImageIcon("/usr25/mrani/vnmrCOProject/vnmr/images/"+strFile+".gif");
                    if (icon != null)
                        m_btnDragItem.setIcon(icon);
                    else*/
                        m_btnDragItem.setText(strText);
                    m_btnDragItem.getParent().repaint();
                }
            });
        }

        DragGestureRecognizer dragRecognizer = m_objDragSource.createDefaultDragGestureRecognizer(
                                                null,
                                                DnDConstants.ACTION_COPY_OR_MOVE,
                                                this );
        dragRecognizer.setComponent( m_btnDragItem );
    }

    /**
     *  Adds the compNew to this toolbar.
     */
    public Component add(Component compNew)
    {
        Component comp = super.add(compNew);
        comp.setBackground(m_bgColor);

        return comp;
    }

    public void propertyChange(PropertyChangeEvent e)
    {
        String strPropName = e.getPropertyName();
        if (strPropName.equals(WGlobal.FONTSCOLORS))
        {
            setOptions();
        }
    }

    /**
     *  Do the layout of buttons, and menus for this toolbar.
     *  @param strArea  the area that the toolbar belongs to area1 or area2.
     */
    protected void doLayout(String strArea)
    {
        String strName = null;
        String[] sortChoices = null;
        String[] aStrShowChoices = {ALL_VJ_USERS, IMG_USERS,
                                    UNIX_USERS, WALKUP_USERS, LC_USERS};
        String[] aStrShowChoices2 = {ALL_VJ_USERS, IMG_USERS,
                                     UNIX_USERS, WALKUP_USERS, LC_USERS,
                                     DISABLE_ACC};
        String strNewTxt = m_btnDragItem.getText();

        Font font = m_btnDragItem.getFont();
        Font fontBtn = new Font(font.getName(), font.getStyle(), 12);

        if (strArea.equals(WGlobal.AREA1))
        {
            strName = WGlobal.SAVEUSER;
            sortChoices = new String[]{"Sort By Name", "Reverse order"};
            //lblUsers.setText("Other Users");

            m_btnEnable = new JButton("Activate Account");
            m_btnEnable.setFont(fontBtn);
            m_btnEnable.setVisible(false);
            m_btnEnable.addActionListener(this);
            m_btnEnable.setActionCommand("activate");

            // Number of users
            String[] aStrUsers = {SINGLE_USER, MULTI_USERS};
            m_cmbUser = new JComboButton(aStrUsers, m_btnDragItem);
            add(m_cmbUser);
            add(m_btnEnable);
            /*ImageIcon icon = new ImageIcon("/usr25/mrani/vnmrCOProject/vnmr/images/newuser.gif");
            if (icon != null)
            {
                m_btnDragItem.setText("");
                m_btnDragItem.setIcon(icon);
            }*/
        }
        else if (strArea.equals(WGlobal.AREA2))
        {
            strName = WGlobal.SAVEGROUP;
            sortChoices = new String[]{"Sort by Name", "Reverse order"};
        }

        // Drag button
        //add(m_btnDragItem);
        m_btnDragItem.setText(vnmr.util.Util.getLabel("_admin_New_User"));
        m_btnDragItem.setFont(fontBtn);
        m_btnDragItem.setToolTipText(java.text.MessageFormat.format(
                vnmr.util.Util.getLabel("_admin_Create_a"), new Object[] {strNewTxt})
                );

        // Save Button
        m_btnSave = new JButton(strName);
        m_btnSave.addActionListener(this);
        m_btnSave.setActionCommand("save");
        add(m_btnSave);
        /*ImageIcon icon = new ImageIcon("/usr25/mrani/vnmrCOProject/vnmr/images/saveuser.gif");
        if (icon != null)
            m_btnSave.setIcon(icon);*/
        m_btnSave.setToolTipText(vnmr.util.Util.getLabel("blSave"));
        m_btnSave.setFont(fontBtn);

        // Sort Menu
        if (strArea.equals(WGlobal.AREA1))
        {
            m_cmbView = new JComboBox(aStrShowChoices);
            if (Util.isPart11Sys())
                m_cmbView = new JComboBox(aStrShowChoices2);
            m_cmbView.setName("Show");

            m_cmbView.addActionListener(this);
            m_cmbView.setActionCommand("show");
            m_cmbView.setFont(fontBtn);
            add(m_cmbView);
        }

        // Search Panel
        JLabel lblSearch = new JLabel(vnmr.util.Util.getLabel("_admin_Find"));
        JTextField txfSearch = new JTextField();
        JPanel pnlSearch = new JPanel();
        txfSearch.addActionListener(this);
        txfSearch.setActionCommand("search");
        txfSearch.setName("Search");
        txfSearch.setFont(fontBtn);
        lblSearch.setFont(fontBtn);
        pnlSearch.setLayout(new GridLayout(1, 1));
        pnlSearch.add(lblSearch);
        pnlSearch.add(txfSearch);
        add(pnlSearch);
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (cmd.equals("new"))
        {
            doActionForNew(e);
        }
        else if(cmd.equals("save"))
        {
            doSave();
            blinkStop();
        }
        else if (cmd.equals("search"))
        {
            doSearchAction(e);
        }
        else if (cmd.equals("activate"))
        {
            String strPropName = WGlobal.ACTIVATE + WGlobal.SEPERATOR + m_strArea;
            m_pcsTypesMgr.firePropertyChange(strPropName, "All", "");
        }
        else if (cmd.equals("show"))
        {
            viewSelectedMode();
            blinkStop();
        }

    }

    protected void initBlink()
    {
        String blinkFrequency = null;
        delay = (blinkFrequency == null) ? 400 :
            (1000 / Integer.parseInt(blinkFrequency));
        labelString = null;
        if (labelString == null)
            labelString = "Blink";
        Font font = new java.awt.Font("TimesRoman", Font.PLAIN, 24);
        setFont(font);
    }

    /**
     *  Returns the item that is draggable for this toolbar.
     */
    public JButton getDragItem()
    {
        return m_btnDragItem;
    }

    public static void firePropertyChng(String strPropName, Object oldValue, Object newValue)
    {
        m_pcsTypesMgr.firePropertyChange(strPropName, oldValue, newValue);
    }

    public JButton getSaveButton()
    {
        return m_btnSave;
    }

    /**
     *  Returns true if the new item is dragged, false otherwise.
     */
    public boolean getNewItemDrag()
    {
        return m_bIsNewItemDrag;
    }

    /**
     *  Sets the new item drag value.
     */
    public void setNewItemDrag(boolean bNewItemDrag)
    {
        m_bIsNewItemDrag = bNewItemDrag;
    }

    /**
     *  Sets the visibility of this toolbar, and the visibility of components it contains.
     *  @param bVisible true if the components should be set visible.
     */
    public void setItemsVisible(boolean bVisible)
    {
        setVisible(bVisible);
        int nCompCount = getComponentCount();
        for(int i = 0; i < nCompCount; i++)
        {
            Component comp = getComponent(i);
            comp.setVisible(bVisible);

            // don't set the Activate button to be visible.
            if (comp instanceof JButton)
            {
                JButton btn = (JButton)comp;
                String strTxt = btn.getText();
                if (strTxt != null && strTxt.trim().indexOf("Activate") >= 0)
                {
                    comp.setVisible(false);
                }
            }
        }
        if (m_cmbView != null)
            m_cmbView.setSelectedIndex(0);
    }

    public static void showAllUsers()
    {
        m_pcsTypesMgr.firePropertyChange("Show_Area1", "All", m_cmbView.getSelectedItem());
    }

    public void blinkSaveBtn()
    {
        if (timer != null)
            timer.cancel();

        timer = new java.util.Timer();
        timer.schedule(new TimerTask() {
            public void run() {
               WUtil.blink(m_btnSave, WUtil.BACKGROUND);
               m_btnSave.setForeground(Color.white);
           }
        }, delay, delay);
    }

    public void blinkStop()
    {
        if (timer != null)
            timer.cancel();

        m_btnSave.setBackground(m_bgColor);
        m_btnSave.setForeground(Color.black);
    }

    public void doSave()
    {
         if (m_btnSave.getText().equalsIgnoreCase(WGlobal.SAVEUSER)) {
	
            m_pcsTypesMgr.firePropertyChange(WGlobal.SAVEUSER, "ALL", "");

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

        } else if (m_btnSave.getText().equalsIgnoreCase(WGlobal.SAVEGROUP))
            m_pcsTypesMgr.firePropertyChange(WGlobal.SAVEGROUP, "ALL", "");
    }

    protected void setOptions()
    {
        String strColName=WFontColors.getColorOption(WGlobal.ADMIN_BGCOLOR);
        m_bgColor = (strColName != null) ? WFontColors.getColor(strColName)
                                            : Global.BGCOLOR;
        WUtil.doColorAction(m_bgColor, strColName, this);
    }

    protected void doSearchAction(ActionEvent e)
    {
        JTextField txfSearch = (JTextField)e.getSource();
        String strPropName = txfSearch.getName() + WGlobal.SEPERATOR + m_strArea;
        m_pcsTypesMgr.firePropertyChange(strPropName, "All", txfSearch.getText());
    }

    protected void doActionForNew(ActionEvent e)
    {
        boolean bMulti = false;
        if (m_cmbUser != null)
        {
            JList listUser = m_cmbUser.getList();
            int nSelected = listUser.getMaxSelectionIndex();
            if (nSelected < 0) nSelected = 0;
            String strSelection = (String)(listUser.getModel().getElementAt(nSelected));
            if (strSelection != null && strSelection.equals(MULTI_USERS))
            {
                m_dlgMultiUsers.setVisible(true);
                bMulti = true;
            }
            else
            {
                m_pcsTypesMgr.firePropertyChange("New_"+m_strArea, "all", "");
            }
        }
    }

    /**
     *  Appends the name of the combobox to the area, and fires a property change,
     *  so that the panels that are listening could do the respective action.
     */
    protected void viewSelectedMode()
    {
        String strPropName = m_cmbView.getName() + "_" + m_strArea;
        String strItem = (String)m_cmbView.getSelectedItem();
        boolean bVJUsers = false;
        if (strItem.equals(ALL_VJ_USERS) ||
            strItem.equals(IMG_USERS) || strItem.equals(WALKUP_USERS) ||
            strItem.equals(LC_USERS))
            bVJUsers = true;

        m_cmbUser.setVisible(bVJUsers);
        m_btnSave.setVisible(bVJUsers);
        m_btnEnable.setVisible(strItem.equals(DISABLE_ACC));
        m_pcsTypesMgr.firePropertyChange(strPropName, "All", strItem);
    }


    //===========================================================================
   //           PropertyChange support implementation.
   //===========================================================================

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
        {
            m_pcsTypesMgr.addPropertyChangeListener(strProperty, l);
        }
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
      m_bIsNewItemDrag = true;
      Component comp = e.getComponent();
      LocalRefSelection ref = new LocalRefSelection( comp );
      m_objDragSource.startDrag( e, DragSource.DefaultMoveDrop, ref, this);
    }

    public void dragDropEnd( DragSourceDropEvent e ) {}
    public void dragEnter( DragSourceDragEvent e ) {}
    public void dragExit( DragSourceEvent e ) {}
    public void dragOver( DragSourceDragEvent e ) {}
    public void dropActionChanged( DragSourceDragEvent e ) {}
    
    
    // This object is created at startup.  Allow the object to be
    // obtained so that users can be added using the methods in
    // WMultiUsers and WConvertUsers which are already setup.
    static public WMultiUsers getWMultiUsers() {
        return m_dlgMultiUsers;
    }
}


