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
import java.text.*;
import java.awt.*;
import java.beans.*;
import javax.swing.*;
import java.awt.event.*;
import java.awt.dnd.*;
import java.awt.datatransfer.*;
import javax.swing.border.*;

// import vnmr.admin.*;
import vnmr.ui.*;
import vnmr.util.*;
import vnmr.part11.*;
import vnmr.admin.util.*;
import vnmr.templates.*;
import vnmr.admin.vobj.*;


/**
 * Title:   VItemAreaIF
 * Description: The abstract class that has methods for the item panels in the interface.
 * Copyright:    Copyright (c) 2001
 * Company: Varian Inc.
 * @author  Mamta Rani
 * @version 1.0
 */

public abstract class VItemAreaIF extends WObj implements DragGestureListener,
DragSourceListener, DropTargetListener, Cloneable
{

    /** Session Share object.  */
    protected SessionShare m_sshare;

    protected LayoutManager m_pnlLayout;

    /** The current group being added to the panel.  */
    protected WItem m_objCurrItem = null;

    /** The item that's currently selected in the panel.  */
    protected WItem m_objItemSel  = null;

    /** The path of the file that has the list of the items displayed in this panel.  */
    protected String m_strCurrPath;

    /** The directory path that has the information for each item displayed in this panel.  */
    protected static String m_strInfoDir;

    /** The list of the names of the items in this panel.  */
    protected ArrayList m_aListNames = new ArrayList();

    protected WPart11Pnl m_pnlPart11;

    /** The background color of the panel.  */
    protected Color m_bgColor = Global.BGCOLOR;

    /** Property Change Support object.  */
    protected static PropertyChangeSupport m_pcsTypesMgr;

    protected JPopupMenu m_objPopMenu;

    protected static Audit m_audit;

    /** The cache object.   */
    protected static WPanelManager m_pnlMngr;

    /* Drag and drop variables */
    private DragSource        m_objDragSource        = new DragSource();
    private DropTarget m_objDropTarget = new DropTarget(
                                            this,  // component
                                            DnDConstants.ACTION_COPY_OR_MOVE,
                                            this );  // DropTargetListener
    protected GridBagLayout m_gbLayout = new GridBagLayout();
    protected GridBagConstraints m_gbConstraints = new GridBagConstraints();

    /**
     *  Constructor
     */
    public VItemAreaIF(SessionShare sshare)
    {
        super(sshare);

        m_sshare = sshare;
        m_pcsTypesMgr=new PropertyChangeSupport(this);
        m_pnlMngr = new WPanelManager();

        //setLayout(new GridLayout(0, 4, 2, 2));
        m_pnlLayout = m_gbLayout;
        setLayout(m_gbLayout);
        setConstraints(1, 1, 1, 1);
        setAttribute(BGCOLOR, "slateBlue");
        dolayout();

        /*if (Util.isPart11Sys())
            doPart11Display("SystemAuditMenu");*/
    }

    /**
     *  Adds an item to the panel with the specified path.
     */
    protected abstract void addItem(String strPath, int nItem);

    /**
     *  Adds a new item to the panel.
     */
    public abstract void addNewItem();

    /**
     *  Gets the path of the file that has a list of items to be displayed in this panel.
     */
    //public abstract String getCurrPath();

    /**
     *  Sets the path of the file that has a list of items to be displayed in this panel.
     */
    //public abstract void setCurrPath(String strPath);

    /**
     *  Gets the directory that has the information for each item.
     */
    //public abstract String getInfoDir();

    /**
     *  Sets the directory that has the information for each item.
     */
    //public abstract void setInfoDir(String strDir);

    /**
     *  Gets the selected item in the panel.
     */
    //public abstract WItem getItemSel();

    public abstract void addItemToList(String strName);

    /**
     *  Sets the name of the panel.
     */
    public abstract void setPnlName(String strPnlName);

    public abstract String getPnlName();

    /**
     *  Gets the list of the items in the panel.
     */
    //public abstract ArrayList getNamesList(boolean bCurViewNames);

    /**
     *  Sets the list of the items in the panel.
     */
    //public abstract void setNamesList(ArrayList aListNames);

    /**
     *  Gets the area string i.e. if it's area1 or area2.
     */
    public abstract String getArea();

    //public abstract WPart11Pnl getPart11Pnl();

    //public abstract void setPart11Pnl(WPart11Pnl pnl);

    //public abstract void setBgColor(Color bgColor);

    //public abstract Color getBgColor();


    /**
     *  Returns the path of the file that contains the list of items
     *  displayed in this panel.
     */
    public String getCurrPath()
    {
        return m_strCurrPath;
    }

    public void setCurrPath(String strPath)
    {
        m_strCurrPath = strPath;
    }

    /**
     *  Returns the directory path that contains the information for each item
     *  displayed in this panel.
     */
    public String getInfoDir()
    {
        return m_strInfoDir;
    }

    /**
     *  Sets the directory path that contains the information for each item
     *  displayed in this panel.
     *  @param strDir   the new directory path.
     */
    public void setInfoDir(String strDir)
    {
        m_strInfoDir = strDir;
    }

    /**
     *  Returns the item that is currently selected in the panel.
     */
    public WItem getItemSel()
    {
        return m_objItemSel;
    }

    /**
     *  Returns the list of items that are in this panel.
     *  @param bCurViewNames    true if only the names in the current view
     *                          should be display e.g. names in a current group
     *                          false if all the names should be displayed
     */
    public ArrayList getNamesList(boolean bCurViewNames)
    {
        return m_aListNames;
    }

    /**
     *  Sets the list of items for this panel.
     *  @param aListNames   new list of items.
     */
    public void setNamesList(ArrayList aListNames)
    {
        m_aListNames = aListNames;
    }

    public WPart11Pnl getPart11Pnl()
    {
        return m_pnlPart11;
    }

    public void setPart11Pnl(WPart11Pnl pnl)
    {
       m_pnlPart11 = pnl;
    }

    public void setBgColor(Color bgColor)
    {
        m_bgColor = bgColor;
    }

    public Color getBgColor()
    {
        return m_bgColor;
    }


    /**
     *  Clears the panel and repaints.
     */
    public void clear()
    {
        removeAll();
        repaint();
    }

    public void hideAll(JComponent comp, boolean bShow)
    {
        int nCompCount = comp.getComponentCount();

        for (int i = 0; i < nCompCount; i++)
        {
            Component child = comp.getComponent(i);
            child.setVisible(bShow);
            if (child instanceof JComponent)
            {
                JComponent jchild = (JComponent)child;
                if (jchild.getComponentCount() > 0)
                    hideAll(jchild, bShow);
            }
        }
    }

    public void setConstraints(int width, int height, int weightX, int weightY)
    {
        m_gbConstraints.gridwidth = width;
        m_gbConstraints.gridheight = height;
        m_gbConstraints.weightx = weightX;
        m_gbConstraints.weighty = 0;

        m_gbConstraints.anchor = GridBagConstraints.NORTH;
    }

    public void dolayout()
    {
        m_objPopMenu = new JPopupMenu("Set Background");
        WScrollPopMenu objScPop = new WScrollPopMenu(VnmrRgb.getColorNameList());
        objScPop.makePopMenuScroll(m_objPopMenu);

        addMouseListener(new MouseAdapter()
        {
            public void mouseClicked(MouseEvent e)
            {
                if (WUtil.isRightMouseClick(e))
                    doMCAction(e);
            }
        });
    }

    /**
     *  Opens the file with the specified path and adds the item to the panel.
     */
    public void displayItems(String strPath, String strLabel)
    {
        BufferedReader in = WFileUtil.openReadFile(strPath);

        if (in == null)
        {
            //System.err.println("No users in file");
            Messages.postDebug("File " + strPath + " is empty.");
            return;
        }
        String strLine = "";

        try
        {
            clear();
            String strName = "";
            while ((strLine = in.readLine()) != null)
            {
                // set the directory that has the info for the items in this panel.
                if (strLine.startsWith(WGlobal.INFODIR))
                {
                    String strDir = parseDir(strLine);
                    setInfoDir(strDir);
                    continue;
                }
                if (getInfoDir() == null)
                {
                    String strInfoDir = getArea().equals(WGlobal.AREA1) ? "PROFILES"+File.separator
                                            : "GRPS"+File.separator+"profiles"+File.separator;
                    setInfoDir(strInfoDir);
                }
                //ArrayList aListNames = readSpaceSepLine(strLine);
                ArrayList aListNames = WUtil.strToAList(strLine);
                setNamesList(aListNames);
                sortItems(aListNames, "");
            }
            in.close();
            //printUsers();
        }
        catch (Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }
    }

    /**
     *  Deletes the given item from the panel, and from the arraylist of names,
     *  and repaints the panel.
     */
    public void deleteItem(WItem compItem)
    {
        Container container = getParent();
        /*if (container != null)
            container.setVisible(false);*/

        String strName = compItem.getText();
        if (strName.equals(Util.getUser().getAccountName()))
        {
            Messages.postInfo("Administrator cannot be deleted");
            return;
        }

        remove(compItem);
        getNamesList(false).remove(strName);
        writeItemFile(getCurrPath());

        String strPropName = getArea() + WGlobal.SEPERATOR + WGlobal.BUILD;
        firePropertyChng(strPropName, "All", "");
        firePropertyChng(WGlobal.DELETE_USER, "All", strName);

        // stop the save button if blinking
        AppIF appIf = Util.getAppIF();
        if (appIf instanceof VAdminIF)
            ((VAdminIF)appIf).getUserToolBar().blinkStop();

        // save the info from the passwd file
        String strHome = WUserUtil.getUserHome(strName);
        addToDelFile(strName, strHome);

        // delete the user
        boolean bOk = WUserUtil.deleteUser(strName);

        // mark the file as deleted from the directory.
        WUserUtil.deleteUserFiles(strName);
        /*if (container != null)
            container.setVisible(true);*/

        displayItems(getNamesList(true));
    }

    public void deleteUserByName(String strName) {
        
        if (strName.equals(Util.getUser().getAccountName()))
        {
            Messages.postInfo("Administrator cannot be deleted");
            return;
        }

//        remove(compItem);
        getNamesList(false).remove(strName);
        writeItemFile(getCurrPath());
//
        String strPropName = getArea() + WGlobal.SEPERATOR + WGlobal.BUILD;
//??        firePropertyChng(strPropName, "All", "");
//??        firePropertyChng(WGlobal.DELETE_USER, "All", strName);

        // stop the save button if blinking
//        AppIF appIf = Util.getAppIF();
//        if (appIf instanceof VAdminIF)
//            ((VAdminIF)appIf).getUserToolBar().blinkStop();
//
        // save the info from the passwd file
        String strHome = WUserUtil.getUserHome(strName);
        addToDelFile(strName, strHome);

        // delete the user
        boolean bOk = WUserUtil.deleteUser(strName);

        // mark the file as deleted from the directory.
        WUserUtil.deleteUserFiles(strName);
//        /*if (container != null)
//            container.setVisible(true);*/
//
        displayItems(getNamesList(true));
    }
    /**
     *  Adds the user to the delete file.
     *  This method uses a hashmap to read all the users currently in the file,
     *  and then adds the current user being deleted. The hashmap allows only
     *  one copy of the username to be in the delete file at a time, since the
     *  restore method can only save the recent user deleted with the same login
     *  name.
     *  e.g. if user abc12 was deleted, and created again and then deleted,
     *  the latest copy of abc12 being deleted would be in the deletion file.
     *
     *  @param strName  the name of the user being deleted.
     *  @param strHome  the homedir of the user being deleted.
     */
    static protected void addToDelFile(String strName, String strHome)
    {
        String strPath = FileUtil.openPath(WGlobal.TRASHCAN_FILE);
        if (strPath == null)
            strPath = FileUtil.savePath(WGlobal.TRASHCAN_FILE);

        BufferedReader reader = WFileUtil.openReadFile(strPath);
        StringBuffer sbData = new StringBuffer();
        StringTokenizer sTokLine;
        HashMap hmUser = new HashMap();
        String strLine = null;
        String strDelName;
        String strKey;
        String strValue;

        try
        {
            // read the file
            if (reader != null)
            {
                while((strLine = reader.readLine()) != null)
                {
                    sTokLine = new StringTokenizer(strLine, "*\n");
                    strDelName = (sTokLine.hasMoreTokens()) ? sTokLine.nextToken() : null;
                    if (strDelName != null)
                        hmUser.put(strDelName, strLine);
                }
                reader.close();
            }

            // append the user being deleted to the hashmap.
            //strLine = strName + "*" + strHome + "*" + new Date().toLocaleString(); // deprecated
            strLine = strName + "*" + strHome + "*" + DateFormat.getDateInstance().format(new Date());
            hmUser.put(strName, strLine);

            // add all the users being deleted to the data.
            Iterator keySetItr = hmUser.keySet().iterator();
            while(keySetItr.hasNext())
            {
                strKey = (String)keySetItr.next();
                strValue = (String)hmUser.get(strKey);
                sbData.append(strValue);
                sbData.append("\n");
            }

            // write to the file.
            BufferedWriter writer = WFileUtil.openWriteFile(strPath);
            WFileUtil.writeAndClose(writer, sbData);
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
    }

    protected void deleteFile(String strFile)
    {
        if (strFile != null)
        {
            File flItem = new File(strFile);
            flItem.delete();
        }
    }

    public void writeItemFile(String strPath)
    {
        ArrayList aNameList = getNamesList(false);
        //System.out.println("In write file  thefsdfo " + strPath);
        StringBuffer sbData = new StringBuffer();

        try
        {        
            String strName = WUtil.aListToStr(aNameList);
            BufferedWriter writer = WFileUtil.openWriteFile(strPath);
            writer.write(strName);
            writer.flush();
            writer.close();
        }
        catch (Exception e)
        {
            Messages.writeStackTrace(e);
            //e.printStackTrace();
            Messages.postDebug(e.toString());
        }
    }

    protected static String getPropName(String strPnlName, String strArea)
    {
        return (strPnlName + WGlobal.SEPERATOR + strArea);
    }

    /**
     *  Displays the items in this panel.
     */
    protected void doDisplayAction(PropertyChangeEvent evt)
    {
        String strPropName = evt.getPropertyName();
        String strLabel = (String)evt.getOldValue();
        String strPath = (String)evt.getNewValue();
        if (strPath == null || strPath.length() <= 0)
            clear();
        else
        {
            setLayout(m_pnlLayout);
            String strCurrPath = FileUtil.openPath(strPath);
            if (strCurrPath == null)
                strCurrPath = FileUtil.savePath("SYSTEM"+File.separator+strPath);
            setCurrPath(strCurrPath);
            if (strCurrPath != null)
                displayItems(strCurrPath, strLabel);
        }
        setPnlName(WFileUtil.parseLabel(strPropName));
    }

    /*protected WPart11Pnl doPart11Display(String strPath, String strType)
    {
        String strKey = strPath + WGlobal.SEPERATOR + getArea();
        //WPart11Pnl pnlPart11 = getPart11Pnl();
        WPart11Pnl pnlPart11 = (WPart11Pnl)m_pnlMngr.getPanel(strKey);

        if (pnlPart11 == null)
        {
            pnlPart11 = new WPart11Pnl(this);
            setPart11Pnl(pnlPart11);
            m_pnlMngr.setPanel(strKey, pnlPart11);
            System.out.println("In part11 display");
        }

        //JPanel pnl = (JPanel)m_objPnlCache.getData(strPath + strType);
        clear();
        setLayout(new BorderLayout());
        add(pnlPart11, BorderLayout.CENTER);
        pnlPart11.showTable(strPath, strType);
        repaint();

        return pnlPart11;
    }*/

    protected void doPart11Display(String strFile)
    {
        // assuming if file starts with "/", it is a full path.
        // otherwise append file to USER/PERSISTENCE/
        if (m_audit == null)
        {
            if (strFile == null || strFile.trim().length() <= 0)
                return;

            String str = strFile;
            if(!strFile.startsWith("/"))
                str = FileUtil.savePath("USER"+File.separator+"PERSISTENCE"+
                        File.separator+strFile);

            String filepath = FileUtil.savePath("USER"+File.separator+"PERSISTENCE"+
                                File.separator+"tmpAudit");

            Vector paths1 = new Vector();
            paths1.addElement(str);
            String label1 = "Select a path";

            Vector paths2 = new Vector();
            paths2.addElement(filepath);
            String label2 = "Select a type";

            //Audit audit = new Audit(strPath, strType, "", "");
            m_audit = new Audit(paths1, label1, paths2, label2);
            //System.out.println("making audit " + getArea());
        }

        JPanel fileTable = (getArea().equals(WGlobal.AREA1)) ? m_audit.getLocator()
                                : m_audit.getAudit();
        JPanel pnlLbl = null;
        /*JScrollPane sPaneTable = (fileTable instanceof ComboFileTable) ?
                                    ((ComboFileTable)fileTable).getScrollPane() :
                                    m_audit.getLocScrollPane();
        if (sPaneTable != null)
            sPaneTable.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_NEVER);*/

        // Add header for the panel
        if (getArea().equals(WGlobal.AREA1))
        {
            pnlLbl = new JPanel(new BorderLayout());
            pnlLbl.add(Box.createHorizontalStrut(2) , BorderLayout.WEST);
            JButton lbl = new JButton("System Trails");
            pnlLbl.add(lbl, BorderLayout.CENTER);
            pnlLbl.add(Box.createHorizontalStrut(2), BorderLayout.EAST);
        }

        setVisible(false);
        clear();
        setLayout(new BorderLayout());
        if (pnlLbl != null)
            add(pnlLbl, BorderLayout.NORTH);
        add(fileTable, BorderLayout.CENTER);
        setVisible(true);
    }

    protected JComponent getAuditTable()
    {
        if (m_audit == null)
        {
            return null;
        }

        if (getArea().equals(WGlobal.AREA1))
        {
            return m_audit.getLocator();
        }
        else
        {
            return m_audit.getAudit();
        }
    }

    protected void doNewItemAction()
    {
        addNewItem();
        m_pcsTypesMgr.firePropertyChange(WGlobal.BGCOLOR, "all", getBgColor());

        validate();
        repaint();
    }

    /**
     *  Parses the directory path from the line.
     *  The line is in the following format:
     *  InfoDir   /usr25/mrani....
     */
    protected String parseDir(String strLine)
    {
        String strDir = null;
        int nIndex = strLine.indexOf('\t');
        if (nIndex <= 0)
            nIndex = strLine.indexOf(' ');
        if (nIndex > 0)
            strDir = strLine.substring(nIndex + 1);

        if (strDir != null)
        {
            int nLength = strDir.length();
            if (strDir.charAt(nLength-1) != '/')
                strDir = strDir + '/';
        }
        return strDir;
    }

    /**
     *  Parses the string and adds the names of the items to the arraylist.
     *  @param strLine  the line that contains the names of the items.
     */
    protected ArrayList readSpaceSepLine(String strLine)
    {
        int nBegIndex = 0;
        int nSepIndex = 0;
        String strName;
        ArrayList aListNames = new ArrayList();
        StringTokenizer strTokNames = new StringTokenizer(strLine);

        while(strTokNames.hasMoreTokens())
        {
            strName = strTokNames.nextToken();
            if (strName != null)
                aListNames.add(strName.trim());
        }

        return aListNames;
    }

    /**
     *  Displays the list of the items in the panel in a sorted manner.
     *  @param aListNames   the list of the names of the items in the panel.
     */
    protected void displayItems(ArrayList aListNames)
    {
        if (aListNames == null)
            return;


        boolean bVisible = false;
        sortItems(aListNames, "");
    }

    /**
     *  Copy the backup file to the original file.
     *  @param strPath      the path of the original file.
     *  @param strBakPath   the path of the backup file.
     */
    protected void copyToOriginal(String strPath, String strBakPath)
    {
        if (strPath != null && strBakPath != null)
        {
            File originalFile = new File(strPath);
            File bakFile = new File(strBakPath);
            bakFile.renameTo(originalFile);
        }
        else
        {
            Messages.postDebug("Error copying file to " + strPath + " from " + strBakPath);
        }
    }

    protected static String getItemDir(String strPnlName, String strInfoDir, String strItemName)
    {
        String strDir = strInfoDir + strItemName;

        if (strPnlName.equalsIgnoreCase("users"))
        {
            String strPbDir = strInfoDir+"system"+File.separator+strItemName;
            String strPrvDir = strInfoDir+"user"+File.separator+strItemName;
            strDir = strPbDir + File.pathSeparator + strPrvDir;
        }
        return strDir;
    }

    protected static String getItemPath(String strPnlName, String strInfoDir, String strItemName)
    {
        String strDir = getItemDir(strPnlName, strInfoDir, strItemName);
        String strPath = FileUtil.openPath(strDir);
        int nIndex = strDir.indexOf(File.pathSeparator);

        if (nIndex > 0)
        {
            String strPbPath = FileUtil.openPath(strDir.substring(0, nIndex));
            String strPrvPath = FileUtil.openPath(strDir.substring(nIndex+1));
            strPath = strPbPath + File.pathSeparator + strPrvPath;
        }
        return strPath;
    }

    /**
     *  Returns the admininterface.
     */
    protected VAdminIF getAdminIF()
    {
        return getAdminIF(m_sshare);
    }

    /**
     *  Returns the admininterface.
     */
    protected VAdminIF getAdminIF(SessionShare sshare)
    {
        AppIF objAppIf = Util.getAppIF();
        if (objAppIf instanceof VAdminIF)
            return (VAdminIF)objAppIf;
        else
            return null;
    }
    
    /**
     *  Compares the given item name to the items in the list,
     *  and returns true if the item name is in the list.
     */
    protected boolean compareItemToList(String strName, Object objValue)
    {
        boolean bInList = false;

        if (objValue != null && (objValue instanceof ArrayList))
        {
            ArrayList aListItems = (ArrayList)objValue;
            bInList = aListItems.contains(strName);

            /*for (int j = 0; j < aListItems.size(); j++)
            {
                strItem = (String)aListItems.get(j);
                if (strName.equals(strItem))
                {
                    bInList = true;
                    break;
                }
            }*/
        }
        return bInList;
    }

    /**
     *  Sorts the item list given the selection which tells how to sort it,
     *  i.e. sort by name or sort in reverse etc.
     */
    protected void sortItems(String strSelection)
    {
        ArrayList aListItems = getItems(true);
        sortItems(aListItems, strSelection);
    }

    /**
     *  Sorts the item list given the list and the menu selection string,
     *  and displays the sorted items in the panel.
     */
    protected void sortItems(ArrayList aListItems, String strSelection)
    {
        aListItems = sort(aListItems, strSelection);

        clear();

        Container container = getParent();
        /*if (container != null)
            container.setVisible(false);*/
        for (int i = 0; i < aListItems.size(); i++)
        {
            String strItem = (String)aListItems.get(i);
            //System.out.println("Name is " + strItem);
            addItem(strItem, i);
        }

        /*if (container != null)
            container.setVisible(true);*/
        validate();
        repaint();
    }

    /**
     *  Sorts the given list of items.
     *  @param aListItems   the list of items to be sorted.
     *  @param strSelection the string that describes how the items should be sorted.
     */
    protected ArrayList sort(ArrayList aListItems, String strSelection)
    {
        if (strSelection != null && aListItems != null)
        {
            strSelection = strSelection.toLowerCase();
            if (strSelection.indexOf("sort by name") >= 0)
                Collections.sort(aListItems);
            else if (strSelection.indexOf("reverse") >= 0)
            {
                Collections.sort(aListItems);
                Collections.reverse(aListItems);
            }
            else
                Collections.sort(aListItems);
        }
        //System.out.println("Sorting");
        return aListItems;
    }

    /**
     *  Returns the arrayList of items in the panel.
     *  @param bStringVals  if true arraylist of item names should be returned,
     *                      else arraylist of witems should be returned.
     */
    protected ArrayList getItems(boolean bStringVals)
    {
        ArrayList aListItems = new ArrayList();
        int nCompCount = getComponentCount();
        WItem objItem;
        String strItem;

        for(int i = 0; i < nCompCount; i++)
        {
            objItem = (WItem)getComponent(i);
            strItem = objItem.getText();
            if (!bStringVals)
                aListItems.add(objItem);
            else
                aListItems.add(strItem);
        }

        return aListItems;
    }

    protected void searchItem(String strValue)
    {
        int nCompCount = getComponentCount();
        WItem objItem;
        Color bgColor;
        if (strValue == null || strValue.trim().length() <= 0)
            return;

        for (int i = 0; i < nCompCount; i++)
        {
            objItem = (WItem)getComponent(i);
            String strItemName = objItem.getText();
            if (strItemName.equalsIgnoreCase(strValue.trim()))
            {
                bgColor = getBgColor().darker();
                scrollRectToVisible(objItem.getBounds());
            }
            else
                bgColor = getBgColor();
            objItem.setBackground(bgColor);
        }
    }

    protected void doMCAction(MouseEvent e)
    {
        int nXPos = e.getX();
        int nYPos = e.getY();
// Stop making a background menu.  This does not fit with the rest of the SW
//        m_objPopMenu.show(this, nXPos, nYPos);
    }

    protected void doColorAction(String strColor)
    {
        Color bgColor = VnmrRgb.getColorByName(strColor);
        setBgColor(bgColor);
        setBackground(bgColor);
        WUtil.doColorAction(bgColor, strColor, this);

        String strArea = getArea().equals(WGlobal.AREA1) ? WGlobal.ITEM_AREA1
                                                        : WGlobal.ITEM_AREA2;
        WFontColors.setColorOption(strArea+WGlobal.BGCOLOR, strColor);
    }

    //==============================================================================
   //   PropertyChange methods follows ...
   //============================================================================

   /*public static void firePropertyChng(String strPropName, String strAction, String strPath)
    {
        m_pcsTypesMgr.firePropertyChange(strPropName,strAction,strPath);
    }*/

    public static void firePropertyChng(String strPropName, String strAction, Object objValue)
    {
        m_pcsTypesMgr.firePropertyChange(strPropName, strAction, objValue);
    }

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

    //==============================================================================
   //   DragGestureListener implementation follows ...
   //============================================================================
    public void dragGestureRecognized( DragGestureEvent e )
    {
        Component comp = e.getComponent();

        LocalRefSelection ref = new LocalRefSelection( comp );
        m_objDragSource.startDrag( e, DragSource.DefaultMoveDrop, ref, this );
    }

    //=================================================================
    //   DragSourceListener implementation follows ...
    //=================================================================

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
                //System.out.println("new item");
                Object obj = tr.getTransferData( LocalRefSelection.LOCALREF_FLAVOR );
                JComponent comp = ( JComponent ) obj;
                if (!(comp instanceof WItem))
                {
                    doNewItemAction();

                    e.acceptDrop( DnDConstants.ACTION_MOVE );
                    e.getDropTargetContext().dropComplete( true );
                }

            }
            return;
        }
        catch( IOException io )
        {
            Messages.writeStackTrace(io);
            e.rejectDrop();
            //io.printStackTrace();
            Messages.postDebug(io.toString());
        }
        catch( UnsupportedFlavorException ufe )
        {
            Messages.writeStackTrace(ufe);
            e.rejectDrop();
            //ufe.printStackTrace();
            Messages.postDebug(ufe.toString());
        }
    }

    protected class WScrollPopMenu extends ScrollPopMenu
    {
        public WScrollPopMenu(Object[] items)
        {
            super(items);
        }

        public void doAction(String strSelection)
        {
            doColorAction(strSelection);
        }
    }

}
