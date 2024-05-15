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
import javax.swing.table.*;

import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.util.*;
// import vnmr.admin.*;
import vnmr.templates.*;
import vnmr.admin.util.*;
import vnmr.admin.vobj.*;

/**
 * Title:   VItemArea1
 * Description: The class that implements the methods for displaying items in the upper left hand panel.
 *              These items could either be user or inventory or something else depending on the menu selection.
 * Copyright:    Copyright (c) 2001
 * Company: Varian Inc.
 * @author Mamta Rani
 * @version 1.0
 */

public class VItemArea1 extends VItemAreaIF implements ActionListener, PropertyChangeListener
{

    /** The name of the panel.  */
    protected static String m_strPnlName;
    
    /** Current active user being edited */
    protected static String activeUser=null;

    /** The current view mode of the panel, the default is all vj users.  */
    protected String m_strViewMode = VUserToolBar.ALL_VJ_USERS;

    /** The cache object.   */
    protected static WFileCache m_objCache;

    protected JFrame m_frame = null;

    /** The list of the names of the items pertaining to the group in itemArea2. */
    protected ArrayList m_aGroupItemNames = new ArrayList();

    protected static final String m_strPrefix = "auditTrailImg";

    /**
     *  Constructor
     */
    public VItemArea1(SessionShare sshare)
    {
        super(sshare);
        m_objCache = new WFileCache(10);
        m_bgColor = VnmrRgb.getColorByName(getAttribute(BGCOLOR));
        setOptions();
    }

    public void initChngListeners()
    {
        WSubMenuItem.addChangeListener(this);
        VUserToolBar.addChangeListener(this);
        VItemAreaIF.addChangeListener(WGlobal.GROUP_CHANGE, this);
        WFontColors.addChangeListener(this);
    }

    public void setOptions()
    {
        String strColName=WFontColors.getColorOption(WGlobal.ITEM_AREA1+WGlobal.BGCOLOR);
        m_bgColor = (strColName != null) ? WFontColors.getColor(strColName)
                                            : Global.BGCOLOR;
        setAttribute(BGCOLOR, strColName);
        WUtil.doColorAction(m_bgColor, strColName, this);
    }

    /**
     *  Gets the name of the object that got clicked, and then tries to find
     *  a file with the same name and appending .xml to it. Each item object
     *  has a details file associated with it that has the same name as the item.
     *  It then fires a property change event with the new file path that should
     *  be read and displayed in the details area.
     *  @param e    ActionEvent
     */
    public void actionPerformed(ActionEvent e)
    {
        m_objItemSel = (WItem)e.getSource();
        activeUser = m_objItemSel.getText();

        String strDir = getItemDir(m_strPnlName, m_strInfoDir, activeUser);

        String strPropName = getPropName(m_strPnlName, WGlobal.AREA1);
        strPropName = strPropName + WGlobal.SEPERATOR + WGlobal.BUILD;
        firePropertyChng(strPropName, "", strDir);
        firePropertyChng(WGlobal.USER_CHANGE, "all", activeUser);

    }

    /**
     *  Fires a property change event to write the file for the m_objItemSel,
     *  which is the item that is currently selected. Also checks if the item
     *  is not in the list of items, then adds it to the list and rewrites the item
     *  list file.
     */
    public void writeFile(String strProp)
    {
        if (m_objItemSel != null)
        {
            String strPropName = getPropName(strProp, WGlobal.AREA1);
            strPropName = strPropName + WGlobal.SEPERATOR + WGlobal.WRITE;
            firePropertyChng(strPropName, "", m_objItemSel);
            if (!m_aListNames.contains(m_objItemSel.getText()))
            {
                addItemToList(m_objItemSel.getText());
                //writeItemFile(m_strCurrPath);
            }
        }
    }

    public void writeFile(String strName, String strProp)
    {
        String strPropName = getPropName(strProp, WGlobal.AREA1);
        strPropName = strPropName + WGlobal.SEPERATOR + WGlobal.WRITE;
        firePropertyChng(strPropName, "", FileUtil.savePath("USRPROF"+File.separator+strName));
        if (!m_aListNames.contains(strName))
        {
            addItemToList(strName);
            //writeItemFile(m_strCurrPath);
        }
    }

    public void addItemToList(String strName)
    {
        if (!m_aListNames.contains(strName))
            m_aListNames.add(strName);
    }
    
    static public String getCurAdminToolUser() {
        return activeUser;
    }

    /**
     *  Checks if the file for the value exists, and then fires a property change event
     *  to build the panel.
     */
    public static void firePropertyChng(String strArea, String strValue)
    {
        String strDir = getItemDir(m_strPnlName, m_strInfoDir, strValue);

        String strPropName = getPropName(m_strPnlName, strArea);
        strPropName = strPropName + WGlobal.SEPERATOR + WGlobal.BUILD;

        if (isFileExists(strDir))
            firePropertyChng(strPropName, "", strDir);
    }

    /**
     *  This method is called from WEntry when a user presses 'return'
     *  in an entry box. There are two cases when this method is called,
     *  the user hit return in the name(login) field or in some other field.
     *
     *  Case 1: If the return was pressed in the name (login) field
     *  then bName is true, and this is a new user, fill the entries with the defaults.
     *  Error Case: If the return was pressed in the name(login) field,
     *  and the file already exists, then they entered a name of the existing user.
     *
     *  Case 2: a) If the return was pressed in some other field, and bName is true,
     *  then this is a new user, fill the entries with the defaults.
     *          b) If the return was pressed in some other field, and bName is false,
     *  then this might not be a new user, update the values of the entries with
     *  the new value.
     *
     *  There are two different cases because based on the property name fired here,
     *  the editability of the various fields in the detail area are set.
     */
    public static void firePropertyChng(String strArea, String strValue,
                                            HashMap hmDef, boolean bName)
    {
        String strDir = getItemDir(m_strPnlName, m_strInfoDir, strValue);

        String strPropName = getPropName(m_strPnlName, strArea);
        strPropName = strPropName + WGlobal.SEPERATOR + WGlobal.BUILD;

        // if the file exists, then it's not a new user.
        if (isFileExists(strDir))
        {
            // this is the case where a name of an existing user has been entered in
            // the accname textfield, when entering information for a new user
            // so add 'newuserclicked' to the propertyname so that
            // an message could notify that the user already exists.
            if (bName)
            {
                strPropName = WUtil.concat(strPropName, "error");
            }
            firePropertyChng(strPropName, "", strDir);
        }
        // else if bName is true then this is a new user
        // otherwise this might be an update for an exisiting user
        else if (hmDef != null)
        {
            if (bName)
            {
                // for new user, blink save button.
                AppIF appIf = Util.getAppIF();
                if (appIf instanceof VAdminIF)
                    ((VAdminIF)appIf).getUserToolBar().blinkSaveBtn();
                //System.out.println("new user");
            }
            else
            {
                String strName = (String)hmDef.get(WGlobal.NAME);
                strDir = getItemDir(m_strPnlName, m_strInfoDir, strName);
                // if the file exists, then it's not a new user
                // then just update.
                if (isFileExists(strDir))
                    strPropName = WUtil.concat(strPropName, "update");
            }

            // fire property change
            VItemAreaIF.firePropertyChng(strPropName, "", hmDef);
        }

    }

    /**
     *  Checks the property name and does the respective action.
     */
    public void propertyChange(PropertyChangeEvent evt)
    {
        String strPropName  = (String)evt.getPropertyName();
        Object objValue     = evt.getNewValue();

        if (strPropName == null)
            return;

        if (strPropName.equals(WGlobal.SAVEUSER))
        {
            doSaveUserAction(strPropName, (String)objValue);
        }
        else if (strPropName.equals("New_Area1"))
            doNewItemAction();
        else if (strPropName.equals("Sort_Area1"))
            sortItems((String)objValue);
        else if (strPropName.equals("Show_Area1"))
           showItems(evt);
        else if (strPropName.equals("Search_Area1"))
            searchItem((String)objValue);
        else if (strPropName.indexOf(WGlobal.ACTIVATE) >= 0)
            activateAccount();
        else if (strPropName.indexOf(WGlobal.PART11) >= 0)
            doPart11Display(evt);
        else if (strPropName.indexOf(WGlobal.AREA1) >= 0)
            doDisplayAction(evt);
        else if (strPropName.equals(WGlobal.GROUP_CHANGE))
            doGroupChngAction(strPropName, objValue);
        else if (strPropName.equals(WGlobal.FONTSCOLORS))
            setOptions();
        // part 11 menu this is for the whole screen not just the itemarea1,
        // but this is the place for now.
        else if (strPropName.indexOf("capture") >= 0)
            doCapture(strPropName);
        else if (strPropName.equals("archive"))
            doArchive();
    }

    /**
     *  Returns the list of items that are in this panel.
     *  @param bCurViewNames    true if only return names that are in the current view
     *                          e.g. only exp users, false if all names should be returned
     */
    public ArrayList getNamesList(boolean bCurViewNames)
    {
        return (!bCurViewNames ? m_aListNames
                    : getShowItems(m_aListNames, m_strViewMode));
    }

    /**
     *  Sets the list of items for this panel.
     *  @param aListNames   new list of items.
     */
    public void setNamesList(ArrayList aListNames)
    {
        m_aListNames = aListNames;
    }

    public static WMRUCache getPnlCache()
    {
        return m_objCache;
    }

    public static long getLastModified(String strFName)
    {
        return m_objCache.getLastModified(strFName);
    }

    public static void setLastModified(String strFName, long lastMod)
    {
        m_objCache.setLastModified(strFName, lastMod);
    }

    /**
     *  Returns the area string.
     */
    public String getArea()
    {
        return WGlobal.AREA1;
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
        String strName = "NewUser";
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
    protected void updateGroup()
    {
        if (m_objItemSel == null)
            return;

        String strName = m_objItemSel.getText();
        String strPath = getItemPath(m_strPnlName, m_strInfoDir, strName);
        WUserUtil.updateGroup(strPath, strName);
    }

    protected void doSaveUserAction(String strPropName, String strValue)
    {
        if (strValue == null || strValue.trim().equals(""))
            writeFile(strPropName);
        else
            writeFile(strValue, strPropName);

	AppIF appIf = Util.getAppIF();
	if (appIf instanceof VAdminIF) {
	   ((VAdminIF)appIf).getDetailArea1().setAppMode();
	}	

        updateGroup();
    }

    protected void doCapture(String strPropName)
    {
        if (strPropName.indexOf("save") >= 0)
        {
            String strPath = FileUtil.openPath("USER"+File.separator+"PART11"+
                                                File.separator+"images");
            if (strPath == null)
                strPath = FileUtil.savePath("USER"+File.separator+"PART11"+
                                             File.separator+"images");
            doCaptureSave(strPath);
        }
        else if (strPropName.indexOf("print") >= 0)
        {
            JFrame frame = getFrame();
            CaptureImage.doPrint(frame.getSize(), frame.getLocationOnScreen());
        }

    }

    protected void doCaptureSave(final String strPath)
    {
        if (strPath == null)
        {
            Messages.postDebug("File not found P11/images");
            return;
        }

        new Thread(new Runnable()
        {
            public void run()
            {
                JFrame frame = getFrame();
                String strFile = strPath + File.separator + m_strPrefix + WUtil.getDate() + ".jpeg";
                boolean bOk = CaptureImage.doCapture(strFile, frame.getSize(), frame.getLocationOnScreen());
                if (bOk)
                {
                    Messages.postInfo("Captured Image to " + strFile);
                }
            }
        }).start();

    }

    protected void doArchive()
    {
        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.sysdir() + "/bin/arAuditing"};

        WUtil.runScriptInThread(cmd, true);
    }

    protected JFrame getFrame()
    {
        if (m_frame == null)
        {
            Container container = getParent();
            while (container != null)
            {
                if (container instanceof JFrame)
                {
                    m_frame = (JFrame)container;
                    break;
                }
                container = container.getParent();
            }
        }
        return m_frame;
    }

    /**
     *  This is the group change action that highlights the items in this panel pertaining
     *  to the current group in the ItemArea2 panel. For each item, compare it to the items
     *  in the list, and if it is in the list, then set the background color brighter,
     *  else set the backgorund color as the bgcolor.
     */
    protected void doGroupChngAction(String strPropName, Object objValue)
    {
        WItem objItem;
        int nCompCount = getComponentCount();
        boolean bInList = false;

        Container container = getParent();
        container.setVisible(false);
        for(int i = 0; i < nCompCount; i++)
        {
            objItem = (WItem)getComponent(i);
            bInList = compareItemToList(objItem.getText(), objValue);
            Color bgColor = bInList ? m_bgColor.brighter().brighter() : m_bgColor;
            objItem.setBackground(bgColor);
        }
        container.setVisible(true);
    }

    protected void activateAccount()
    {
        if (m_objItemSel != null)
        {
            // activate account
            WUserUtil.activateAccount(m_objItemSel.getText());

            // update users in the panel
            VUserToolBar.showAllUsers();
        }
    }

    protected void doBgColorAction(String strPropName, Object objValue)
    {
        if (objValue != null && objValue instanceof Color)
        {
            Color bgColor = (Color)objValue;
            int nCompCount = getComponentCount();
            WItem objItem;

            for (int i = 0; i < nCompCount; i++)
            {
                objItem = (WItem)getComponent(i);
                objItem.setBackground(bgColor);
            }
        }
    }

    protected void doPart11Display(PropertyChangeEvent e)
    {
        /*String strPath = FileUtil.openPath("SYSTEM/part11");
        String strType = "s_auditTrailFiles";*/
        String strPath = "SystemAuditMenu";
        m_audit = null;
        //doPart11Display(strPath, strType);
        doPart11Display(strPath);
    }

    /**
     *  Adds a item to the panel with the specified name.
     *  @param strName  Name of the item.
     */
    protected void addItem(String strName, int nItem)
    {
        m_objCurrItem = new WItem(strName, "star.gif", WGlobal.AREA1, m_strInfoDir, this);
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

    /*protected void doDirSetup()
    {
        clear();
        setLayout(new BorderLayout());
        JPanel pnlTab = new JPanel();
        m_tabPane.addTab("Users/Groups", m_pnlUsers);
        m_tabPane.addTab("Operators", m_pnlOps);
        pnlTab.add(new JButton("Click"));
        add(pnlTab, BorderLayout.CENTER);
    }*/

    private static boolean isFileExists(String strDir)
    {
        boolean bFile = FileUtil.fileExists(strDir);
        int nIndex = strDir.indexOf(File.pathSeparator);
        if (nIndex >= 0)
        {
            String strPath1 = strDir.substring(0, nIndex);
            String strPath2 = strDir.substring(nIndex+1);
            bFile = FileUtil.fileExists(strPath1) || FileUtil.fileExists(strPath2);
        }
        return bFile;
    }

    protected void showItems(PropertyChangeEvent e)
    {
        WItem objSelItem = m_objItemSel;
        String strValue = (String)e.getNewValue();
        String strShowProp = getPropName(m_strPnlName, e.getPropertyName());

        ArrayList aListShowItems = showItems(strValue, strShowProp);
        strShowProp = strShowProp + WGlobal.SEPERATOR + WGlobal.BUILD;
        firePropertyChng(strShowProp, WGlobal.BUILD, "");

        if (m_objItemSel != null && aListShowItems.contains(m_objItemSel.getText()))
        {
            m_objItemSel.doClick();
            m_objItemSel.setBorder(BorderFactory.createLineBorder(Color.green));
        }

    }

    /**
     *  Shows the items that are of the same interface type as strIType.
     *  e.g. Wanda Interface, Sylvia Interface etc.
     */
    protected ArrayList showItems(String strValue, String strPropName)
    {
        //System.out.println("Show items");
        ArrayList aListNames = getNamesList(false);
        ArrayList aListShowItems  = new ArrayList();

        if (strValue == null || aListNames == null)
            return aListShowItems;

        m_strViewMode = strValue;
        aListShowItems = getShowItems(aListNames, strValue);

        // Display the items of the same itype in the panel.
        displayItems(aListShowItems);
        return aListShowItems;
    }

    private ArrayList getShowItems(ArrayList aListNames, String strValue)
    {
        ArrayList aListShowItems = new ArrayList();

        if (strValue.equals(VUserToolBar.ALL_VJ_USERS))
            aListShowItems = aListNames;
        else if (strValue.equals(VUserToolBar.UNIX_USERS))
            aListShowItems = WUserUtil.getUnixUsers();
        else if (strValue.equals(VUserToolBar.DISABLE_ACC))
            aListShowItems = WUserUtil.getDisabledAcc();
        else if (strValue.indexOf(WGlobal.WALKUPIFLBL) >= 0)
            aListShowItems = getItypeItems(aListNames, Global.WALKUPIF);
        else if (strValue.indexOf(WGlobal.IMGIFLBL) >= 0)
            aListShowItems = getItypeItems(aListNames, Global.IMGIF);
        else if (strValue.indexOf(WGlobal.LCIFLBL) >= 0)
            aListShowItems = getItypeItems(aListNames, Global.LCIF);
        else if (strValue.indexOf(WGlobal.ADMINIFLBL) >= 0)
            aListShowItems = getItypeItems(aListNames, Global.ADMINIF);
        // the value is something else, just use the old view value
        // to get the list of names
        else
        {            
           // aListShowItems = getShowItems(aListNames, m_strViewMode);
           // note: above line causes infinite recursion if strValue not in choices
            return aListNames;
        }
        return aListShowItems;
    }

    private ArrayList getItypeItems(ArrayList aListNames, String strIType)
    {
        ArrayList aListShowItems  = new ArrayList();
        HashMap hmItem;
        String strKeyVal;

        // Get the itype for each item,
        // and if it is the same as itype then add it to the aLisShowItems.
        for(int i = 0; i < aListNames.size(); i++)
        {
            String strName = (String)aListNames.get(i);
            String strFile = getItemPath(m_strPnlName, m_strInfoDir, strName);
            hmItem = WFileUtil.getHashMap(strFile);
            if (hmItem != null && !hmItem.isEmpty())
            {
                strKeyVal = (String)hmItem.get("itype");
                if (strKeyVal != null && strKeyVal.indexOf(strIType) >= 0)
                    aListShowItems.add(strName);
            }
        }
        return aListShowItems;
    }

    protected class WFileCache extends WMRUCache
    {
        public WFileCache(int nMax)
        {
            super(nMax);
        }

        public WFile readFile(String strPath)
        {
            File objFile = new File(strPath);
            String strType = WFileUtil.parseItemName(strPath);
            ArrayList aListNames = getShowItems(getNamesList(false), strType);

            return new WFile(0, aListNames);
        }

    }

}
