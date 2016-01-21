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
import java.util.*;
import java.beans.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;
import vnmr.admin.util.*;
import vnmr.admin.vobj.WSubMenuItem;


/**
 * Title:   VItemArea2
 * Description: The class that implements the methods for dispalying items in the lower left hand panel.
 *              These items could either be group or inventory depending on the menu selection.
 * Copyright:    Copyright (c) 2001
 * Company: Varian Inc.
 * @author  Mamta Rani
 * @version 1.0
 */

public class VItemArea2 extends VItemAreaIF implements ActionListener, PropertyChangeListener
{

    /** The list of the names of the items pertaining to the group in itemArea2. */
    protected ArrayList m_aGroupItemNames = new ArrayList();

    /** The name of the panel.  */
    protected static String m_strPnlName;


    /**
     *  Constructor
     */
    public VItemArea2(SessionShare sshare)
    {
        super(sshare);
        m_bgColor = VnmrRgb.getColorByName(getAttribute(BGCOLOR));

        SwingUtilities.invokeLater(new Runnable()
        {
            public void run()
            {
                setOptions();
            }
        });
    }

    public void initChngListeners()
    {
        WSubMenuItem.addChangeListener(this);
        VUserToolBar.addChangeListener(this);
        VItemAreaIF.addChangeListener(WGlobal.ITEM_LIST, this);
        WFontColors.addChangeListener(this);
    }

    public void setOptions()
    {
        String strColName=WFontColors.getColorOption(WGlobal.ITEM_AREA2+WGlobal.BGCOLOR);
        m_bgColor = (strColName != null) ? WFontColors.getColor(strColName)
                                            : Global.BGCOLOR;
        setAttribute(BGCOLOR, strColName);
        WUtil.doColorAction(m_bgColor, strColName, this);
    }

    public void repaint()
    {
        super.repaint();
        Container container = getParent();

        // sometimes the children of this panel doesn't get repainted,
        // so set the container visible again, and this shows everything.
        if (container != null)
        {
            container.setVisible(false);
            container.setVisible(true);
        }
    }

    /**
     *  Gets the name of the object and then tries to find a file with the same
     *  name by appending .xml to it. Each item object has a details file
     *  associated with it with the same name as the item.
     *  It then fires a propertychange with the new file path that should be read
     *  and displayed in the details area.
     *  @param e    ActionEvent
     */
    public void actionPerformed(ActionEvent e)
    {
        m_objItemSel = (WItem)e.getSource();
        String strPath = WUtil.concat(m_strInfoDir, m_objItemSel.getText());
        String strPropName = getPropName(m_strPnlName, WGlobal.AREA2);
        strPropName = strPropName + WGlobal.SEPERATOR + WGlobal.BUILD;
        firePropertyChng(strPropName, "", strPath);

        // read the file for this item, and highlight the items in ItemArea1.
        highlightItems();
    }

    /**
     *  Fires a property change event to write the file for the m_objItemSel,
     *  which is the item that is currently selected. Also checks if the item
     *  is not the list of items, then adds it to the list and rewrites the item
     *  list file.
     */
    public void writeFile(String strProp)
    {
        if (m_objItemSel != null)
        {
            String strPropName = getPropName(strProp, WGlobal.AREA2);
            strPropName = strPropName + WGlobal.SEPERATOR + WGlobal.WRITE;
            firePropertyChng(strPropName, "", m_objItemSel);
            if (!m_aListNames.contains(m_objItemSel.getText()))
            {
                addItemToList(m_objItemSel.getText());
                writeItemFile(m_strCurrPath);
            }
        }
    }

    public void addItemToList(String strName)
    {
        m_aListNames.add(m_objItemSel.getText());
    }

    /**
     *  Checks if the file for the value exists, and then fires a property change event
     *  to build the panel.
     */
    public static void firePropertyChng(String strArea, String strValue)
    {
        String strDir = WUtil.concat(m_strInfoDir, strValue);
        String strPropName = getPropName(m_strPnlName, strArea);
        strPropName = strPropName + WGlobal.SEPERATOR + WGlobal.BUILD;

        if (FileUtil.fileExists(strDir))
            firePropertyChng(strPropName, "", strDir);
    }

    /**
     *  Checks the property name and does the respective action.
     */
    public void propertyChange(PropertyChangeEvent evt)
    {
        String strPropName  = (String)evt.getPropertyName();
        Object objValue     = evt.getNewValue();

        //System.out.println("Property name " + strPropName);
        if (strPropName == null)
            return;

        if (strPropName.equals(WGlobal.SAVEGROUP))
            writeFile(strPropName);
        else if(strPropName.equals("New_Area2"))
            doNewItemAction();
        else if (strPropName.equals("Sort_Area2"))
            sortItems((String)objValue);
        else if (strPropName.equals("Show_Area2"))
        {
            //showItems(strValue, strPropName);
            String strShowProp = getPropName(m_strPnlName, WGlobal.AREA2);
            strShowProp = strShowProp + WGlobal.SEPERATOR + WGlobal.BUILD;
            firePropertyChng(strShowProp, "", "");
        }
        else if (strPropName.equals("Search_Area2"))
            searchItem((String)objValue);
        /*else if (strPropName.equals(WGlobal.ITEM_LIST))
            doUserListAction(strPropName, objValue);*/
        else if (strPropName.equals(WGlobal.PART11))
            doPart11Display(evt);
        else if (strPropName.indexOf("users") >= 0)
            doDirSetup(evt);
        else if(strPropName.equals(WGlobal.FONTSCOLORS))
            setOptions();
        else if (strPropName.indexOf(WGlobal.AREA2) >= 0)
            doDisplayAction(evt);
    }


    /**
     *  Returns the area string.
     */
    public String getArea()
    {
        return WGlobal.AREA2;
    }

    public void setPnlName(String strPnlName)
    {
        m_strPnlName = strPnlName;
    }

    public String getPnlName()
    {
        return m_strPnlName;
    }

    /**
     *  Adds a new item to the panel.
     */
    public void addNewItem()
    {
        String strName = "NewGroup";
        addItem(strName, getComponentCount());
        m_objCurrItem.doClick();
    }

    /**
     *  Reads the list of items that are displayed in itemArea1 that pertain to the
     *  currently selected item in this panel, and fires a property change to
     *  highlight the items in itemArea1 panel.
     *  e.g. chem100 is the selected group and it contains list of students
     *  that should be highlighted in itemArea1.
     */
    protected void highlightItems()
    {
        String strPath = FileUtil.openPath(WUtil.concat(m_strInfoDir, m_objItemSel.getText()));
        BufferedReader in = WFileUtil.openReadFile(strPath);
        String strLine;
        String strTmp;

        if (in == null)
            return;

        try
        {
            while((strLine = in.readLine()) != null)
            {
                strTmp = strLine.toLowerCase();
                if (strTmp.startsWith("itemlist"))
                {
                    ArrayList aListNames = getItemList(strLine);
                    if (m_aGroupItemNames != aListNames)
                        m_pcsTypesMgr.firePropertyChange(WGlobal.GROUP_CHANGE, "All", aListNames);

                    m_aGroupItemNames = aListNames;
                }
            }
            in.close();
        }
        catch(Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }

    }

    /**
     *  Returns the list of items.
     */
    protected ArrayList getItemList(String strLine)
    {
        StringTokenizer strTokItems = new StringTokenizer(strLine);
        ArrayList aListItems = new ArrayList();
        String strNextTok;

        while(strTokItems.hasMoreTokens())
            aListItems.add(strTokItems.nextToken());

        return aListItems;
    }

    /**
     *  This is the group change action that highlights the items in this panel pertaining
     *  to the current group in the ItemArea2 panel. For each item, compare it to the items
     *  in the list, and if it is in the list, then set the background color brighter,
     *  else set the backgorund color as the bgcolor.
     */
    protected void doUserListAction(String strPropName, Object objValue)
    {
        WItem objItem;
        int nCompCount = getComponentCount();
        boolean bInList = false;
        HashMap hmValue = (HashMap)objValue;
        //WItem objUser = (WItem)hmValue.get("item");
        if (hmValue == null)
            return;

        String strName = (String)hmValue.get("item");
        ArrayList aListNames = (ArrayList)hmValue.get("list");

        for(int i = 0; i < nCompCount; i++)
        {
            objItem = (WItem)getComponent(i);
            String strItemName = objItem.getText();
            bInList = compareItemToList(strItemName, aListNames);
            String strPath = FileUtil.openPath(
                                WUtil.concat(objItem.getInfoDir(), strItemName));
            objItem.updateFile(strPath, "itemlist", strName, strName, bInList);
        }
    }

    protected void doPart11Display(PropertyChangeEvent e)
    {
        /*String strPath = FileUtil.openPath("SYSTEM/part11");
        String strType = "d_auditTrailFiles";*/
        //String strPath = "RecordAuditMenu";
        String strPath = "SystemAuditMenu";
        //doPart11Display(strPath, strType);
        doPart11Display(strPath);
    }

    protected void doDirSetup(PropertyChangeEvent e)
    {
        WDirTabPane tabDir = new WDirTabPane();

        clear();
        setLayout(new BorderLayout());
        add(tabDir, BorderLayout.CENTER);
    }


    /**
     *  Adds an item to the panel with the specified name.
     *  @param strName name of the panel.
     */
    protected void addItem(String strName, int nItem)
    {
        m_objCurrItem = new WItem(strName, "button043.gif", WGlobal.AREA2, m_strInfoDir, this);
        m_objCurrItem.addActionListener(this);
        m_objCurrItem.setBgColor(m_bgColor);
        addComponent( m_objCurrItem,nItem/4, nItem%4);
    }

    protected void addComponent(WItem objItem, int row, int col)
    {
        m_gbConstraints.gridx = col;
        m_gbConstraints.gridy = row;

        m_gbLayout.setConstraints(objItem, m_gbConstraints);
        add(objItem);
    }

}
