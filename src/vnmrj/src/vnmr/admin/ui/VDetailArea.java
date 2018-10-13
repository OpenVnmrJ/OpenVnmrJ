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
import java.awt.event.*;

import vnmr.templates.*;
import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.util.*;
import vnmr.part11.*;
import vnmr.admin.util.*;
import vnmr.admin.ui.*;
import vnmr.admin.vobj.*;

/**
 * Title:   VDetailArea
 * Description: The two right hand side panels are the detail area panels,
 *              and the methods for these panels are implemented here.
 * Copyright:    Copyright (c) 2001
 */

public class VDetailArea extends WObj implements PropertyChangeListener, VGroupIF
{
    /** The tab button.  */
    private TabButton tab;

    /** The pop menu for the background color.   */
    protected JPopupMenu m_objPopMenu;

    protected WPanelManager m_pnlMngr;

    /** The background color of the panel.  */
    protected Color m_bgColor = Global.BGCOLOR;

    /** The background color string of the panel.  */
    protected String m_strBgColor = "ivory";

    protected String m_strPart11File = "";

    protected static vnmr.part11.Audit m_audit;

    protected JTextField m_txfName = null;

    protected java.util.Timer timer;
    protected String labelString;       // The label for the window
    protected int delay;                // the delay time between blinks

    /**
     *  Constructor.
     */
    public VDetailArea(SessionShare sshare, ExpPanel expPanel, String strName)
    {
        super(sshare);
        setName(strName);
        tab = new TabButton("");
        vnmrIf = expPanel;
        m_pnlMngr = new WPanelManager();
        setName(strName);
        initBlink();
        //doPart11Display("RecordAuditMenu");

        m_objPopMenu = new JPopupMenu("Set Background");
        WScrollPopMenu objScPop = new WScrollPopMenu(VnmrRgb.getColorNameList());
        objScPop.makePopMenuScroll(m_objPopMenu);

        addMouseListener(new MouseAdapter()
        {
            public void mouseClicked(MouseEvent e)
            {
                if (WUtil.isRightMouseClick(e))
                {
                    doMCAction(e);
                }
                if (timer != null)
                    timer.cancel();

            }
        });

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
        VItemAreaIF.addChangeListener(this);
        WSubMenuItem.addChangeListener(this);
        WUsersConfig.addChangeListener(this);
        WFontColors.addChangeListener(this);
    }

    public void setOptions()
    {
        String strArea = getFullAreaName();
        m_strBgColor=WFontColors.getColorOption(strArea+WGlobal.BGCOLOR);
        m_bgColor = (m_strBgColor != null) ? WFontColors.getColor(m_strBgColor)
                                            : Global.BGCOLOR;
        WUtil.doColorAction(m_bgColor, m_strBgColor, this);
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

    // PropertyChangeListener interface
    public void propertyChange(PropertyChangeEvent evt)
    {
        String strPropName = evt.getPropertyName();
        String strAction = (String)evt.getOldValue();
        Object objVal = evt.getNewValue();

        if (strPropName.equals(WGlobal.FONTSCOLORS))
        {
            setOptions();
        }
        else if (strPropName.indexOf(getName()) >= 0)
        {
            try
            {
                if (strPropName.indexOf(WGlobal.BUILD) >= 0)
                {
                    if (objVal != null)
                    {
                        fillValues(objVal, strPropName);
                    }

                }
                else if (strPropName.indexOf(WGlobal.WRITE) >= 0)
                {
                    createSaveItem(objVal, strPropName);
                }
                else if (strPropName.indexOf(WGlobal.SET_VISIBLE) >= 0)
                {
                    if (objVal != null && objVal instanceof HashMap)
                        setVisibility((HashMap)objVal);
                }
                else if (strPropName.indexOf(WGlobal.PART11) >= 0)
                {
                    if (getArea().equals(WGlobal.AREA1))
                        m_audit = null;
                    doPart11Display(evt);
                }
                else if (strPropName.indexOf("users") >= 0
                            && getArea().equals(WGlobal.AREA2))
                {
                    displayFileTable(evt);
                }
                // path of a file to be displayed.
                else if (objVal instanceof String)
                {
                    String strPath = FileUtil.openPath((String)objVal + ".xml");
                    displayTemplate(strPath);
                }
                repaint();
            }
            catch(Exception e)
            {
                Messages.writeStackTrace(e);
                //e.printStackTrace();
                Messages.postDebug(e.toString());
            }
        }
    }

    /**
     *  Displays the xml template given the path of the file.
     *  @param strPath  the path of the file that has the layout.
     */
    public void displayTemplate(String strPath)
    {
        clear();
        if (strPath != null)
        {
            try
            {
                Container container = getParent();
                /*if (container != null)
                    container.setVisible(false);*/

                LayoutBuilder.build(this, null, strPath);
                dolayout();

                if (container != null)
                {
                    container.setBackground(m_bgColor);
                    //container.setVisible(true);
                }
            }
            catch (Exception e)
            {
                Messages.writeStackTrace(e);
                //e.printStackTrace();
                Messages.postDebug(e.toString());
            }
        }
    }

    public void fillValues(Object objVal, String strPropName)
    {
        // fill the values in the fields
        WFileUtil.fillValues(objVal, this, strPropName);
        // set the color/blink if needed
        setColor(objVal);
    }

    protected void createSaveItems(final Object objVal, final String strPropName)
    {
        new Thread(new Runnable()
        {
            public void run()
            {
                try
                {
                    createSaveItems(objVal, strPropName);
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                }
            }
        }).start();
    }

    protected void createSaveItem(Object objVal, String strPropName)
    {
        if (objVal == null || !isPanelOk())
        {
            return;
        }

        boolean bOk = true;
        String strName = getItemValue(WGlobal.NAME);
        boolean bNewUser = isNewUser(objVal);
        HashMap hmDef = new HashMap();

        AppIF appIf = Util.getAppIF();
        VUserToolBar userToolBar = null;
        if (appIf instanceof VAdminIF)
        {
            hmDef = ((VAdminIF)appIf).getUserDefaults();
            userToolBar = ((VAdminIF)appIf).getUserToolBar();
            userToolBar.blinkStop();
        }

        // check for a new user, and create a new unix user
        // if it doesn't already exist
        if (bNewUser)
        {
            bOk = WUserUtil.isUserNameOk(strName);
            if (!bOk)
                return;
            Messages.postInfo("Creating new user '" + strName + "'.");
            setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
            bOk = WUserUtil.writeNewUserFile(strName, "", hmDef, false, this, null);
            if (objVal instanceof JButton && bOk)
            {
                ((JButton)objVal).setText(strName);
                ArrayList aListNewUser = new ArrayList();
                aListNewUser.add(strName);
                WUserUtil.addToUserListFile(aListNewUser);
                WUserUtil.updateDB();
            }
        }
        else
        {
            // save the info for the user.
            WFileUtil.writeInfoFile(objVal, this, strPropName);
            if (userToolBar != null)
                userToolBar.firePropertyChng(WGlobal.SAVEUSER_NOERROR, "", strName);
        }

        if (!bOk && (Util.OSNAME == null || !Util.OSNAME.startsWith("Mac")))
            Messages.postError("Error writing file for user '" + strName + "'.");
        setCursor(Cursor.getDefaultCursor());
    }

    protected void doPart11Display(String strFile)
    {
        // assuming if file starts with "/", it is a full path.
        // otherwise append file to USER/PERSISTENCE/

        if (strFile == null || strFile.trim().length() <= 0)
            return;

        String str = strFile;
        if(!strFile.startsWith("/"))
            str = FileUtil.savePath("USER/PERSISTENCE/"+strFile);

        String filepath = FileUtil.savePath("USER/PERSISTENCE/tmpAudit");

        Vector paths1 = new Vector();
        paths1.addElement(str);
        String label1 = "Select a path";

        Vector paths2 = new Vector();
        paths2.addElement(filepath);
        String label2 = "Select a type";

        //Audit audit = new Audit(strPath, strType, "", "");
        m_audit = new vnmr.part11.Audit(paths1, label1, paths2, label2);

    }


    protected void doPart11Display(PropertyChangeEvent e)
    {
        if (m_audit == null)
            doPart11Display("RecordAuditMenu");
        JComponent compFileTable = (getArea().equals(WGlobal.AREA1)) ? m_audit.getLocator()
                            : m_audit.getAudit();
        /*JScrollPane sPaneTable = (compFileTable instanceof ComboFileTable) ?
                                    ((ComboFileTable)compFileTable).getScrollPane() :
                                    m_audit.getLocScrollPane();
        if (sPaneTable != null)
            sPaneTable.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_NEVER);*/

        clear();
        setVisible(false);
        setLayout(new BorderLayout());
        if (getArea().equals(WGlobal.AREA1))
            add(getPart11TitleBar(), BorderLayout.NORTH);
        add(compFileTable, BorderLayout.CENTER);
        setVisible(true);

        /*clear();
        Object obj = e.getNewValue();
        if (obj instanceof vnmr.part11.ComboFileTable)
        {
            vnmr.part11.ComboFileTable tableRight = (vnmr.part11.ComboFileTable)obj;
            setVisible(false);
            clear();
            setLayout(new BorderLayout());
            add(tableRight, BorderLayout.CENTER);
            setVisible(true);
        }*/
    }

    public void displayFileTable(PropertyChangeEvent e)
    {
        DFFileTableModel model = new DFFileTableModel();
        TableSorter sorter = new TableSorter(model);
        JTable table = new JTable(sorter);
        sorter.addMouseListenerToHeaderInTable(table);
        JPanel pnlTable = new JPanel(new BorderLayout());
        pnlTable.add(table, BorderLayout.CENTER);
        pnlTable.add(table.getTableHeader(), BorderLayout.NORTH);

        WDirChooser filech = new WDirChooser();
        filech.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);

        JTabbedPane tabPane = new JTabbedPane();
        tabPane.addTab(vnmr.util.Util.getLabel("_admin_Select_Directory"), filech);
        tabPane.addTab(vnmr.util.Util.getLabel("_admin_Disk_Space"),pnlTable);

        clear();
        setLayout(new BorderLayout());
        add(tabPane, BorderLayout.CENTER);
    }

    protected void doPart11Action(ActionEvent e)
    {
        String strPropName = WGlobal.PART11 + WGlobal.SEPERATOR + getArea();
        VItemAreaIF.firePropertyChng(strPropName, "all", m_strPart11File);
    }

    protected void setColor(Object objUser)
    {
        boolean bBlink = false;
        String strInfo = vnmr.util.Util.getLabel("_admin_Enter_login_name_and_press_'RETURN'");
        if (isNewUser(objUser) && m_txfName != null)
        {
            String strName = m_txfName.getText();
            if (strName == null || strName.trim().length() <= 0 ||
                    strName.equals(strInfo))
            {
                bBlink = true;
                if (timer != null)
                    timer.cancel();

                m_txfName.setText(strInfo);

                timer = new java.util.Timer();
                timer.schedule(new TimerTask() {
                    public void run() {WUtil.blink(m_txfName, WUtil.FOREGROUND);}
                }, delay, delay);
            }
        }

        if (!bBlink && timer != null)
        {
            timer.cancel();
            m_txfName.setForeground(Color.black);
        }
    }

    /**
     *  Checks if all the required fields in the detail area are filled,
     *  and there are no errors.
     */
    public boolean isPanelOk()
    {
        boolean bOk = true;
        int nCompCount = getComponentCount();
        WObjIF objItem = null;
        String strKey = null;
        String strValue = null;
        String strLabel = null;
        //Object[] aObjReq = getReqFields();

        // For each component, check if the field is required,
        // if it is then it should be filled.
        for(int i = 0; i < nCompCount; i++)
        {
            objItem = (WObjIF)getComponent(i);
            strValue = objItem.getValue();
            strKey = objItem.getAttribute(VObjDef.KEYWORD);
            strLabel = objItem.getAttribute(VObjDef.LABEL);
            boolean bVisible = ((JComponent)objItem).isVisible();
            String[] aStrReqFields = Util.isPart11Sys() ? WGlobal.USR_PART11_REQ_FIELDS
                                        : WGlobal.USR_REQ_FIELDS;
            if (Util.iswindows())
                aStrReqFields = Util.isPart11Sys() ? WGlobal.USR_PART11_REQ_FIELDS_WIN
                                        : WGlobal.USR_REQ_FIELDS_WIN;
            if (WFileUtil.isReqField(strKey, aStrReqFields))
            {
                if (strKey != null && strKey.equals("itype")){
                    String keyval=objItem.getAttribute(VObjDef.KEYVAL);
                    if(keyval!=null)
                        strValue=keyval;
                    strValue = parseItype(strValue);
                }
                if (strValue == null || strValue.trim().equals(""))
                {
                    String strMsg = (strLabel != null && strLabel.length() > 0) ? strLabel
                                        : strKey;
                     if (bVisible)
                        Messages.postError("Please fill in the required field: " + strMsg);
                    else
                    {
                        Messages.postError("The value for '" + strKey + " is not set");
                        Messages.postError("Please try the following:");
                        Messages.postError("a) Make sure the values in 'User Defaults' " +
                                                "dialog in 'Configure' Menu are set correctly." +
                                                " For more info see 'Setting user defaults' in the help menu.");
                        Messages.postError("b) Press 'RETURN' in the 'login name' textfield and click 'Save User'");
                    }

                    return false;
                }
            }

        }
        return bOk;

    }

    /**
     *  Sets the visibility of the components in this panel.
     *  @hmDef  Hashmap of the defaults for the user panel.
     */
    public void setVisibility(HashMap hmDef)
    {
        int nCompCount = getComponentCount();
        String strKey = "";
        String strValue = "";
        WObjIF objItem = null;
        boolean bVisible = true;

        Container container = getParent();
        /*if (container != null)
            container.setVisible(false);*/

        if (hmDef == null || hmDef.isEmpty())
            return;

        // For each component, check if the default is set to show the component,
        // and if it is then show it, else hide it.
        for (int i = 0; i < nCompCount; i++)
        {
            objItem = (WObjIF)getComponent(i);
            strKey = objItem.getAttribute(VObjDef.KEYWORD);
            if (strKey != null)
            {
                Object objValue = hmDef.get(strKey);
                if (objValue instanceof WUserDefData)
                    strValue = ((WUserDefData)objValue).getShow();
                else
                    strValue = null;
            }
            bVisible = (strValue != null && strValue.equalsIgnoreCase("no")) ? false : true;
            if (objItem instanceof WGroup)
                ((WGroup)objItem).showGroup(bVisible);
            else if (objItem instanceof JComponent)
                ((JComponent)objItem).setVisible(bVisible);
        }
        /*if (container != null)
        {
            container.validate();
            container.setVisible(true);
        }*/
        validate();
        repaint();
    }

    /**
     *  Returns the string: "Detail_Area1" or "Detail_Area2".
     */
    public String getFullAreaName()
    {
        return (getName().equals(WGlobal.AREA1)) ? WGlobal.DETAIL_AREA1
                                                    : WGlobal.DETAIL_AREA2;
    }

    /**
     *  Returns the area name, if it's area1 or area2.
     */
    public String getArea()
    {
        return getName();
    }

    public String getBgColor()
    {
        return m_strBgColor;
    }

    public SessionShare getsshare()
    {
        return m_sshare;
    }

    protected void dolayout()
    {
        int nCount = getComponentCount();
        setLayout(new WGridLayout(0, 1));

        // check the defaults file, and set the visibility of the components.
        if (Util.getAppIF() instanceof VAdminIF)
        {
            VAdminIF appIf = (VAdminIF)Util.getAppIF();
            HashMap hmDef = appIf.getUserDefaults();
            setVisibility(hmDef);
        }

        JComponent comp = (JComponent)getPnlComp(this, WGlobal.NAME, true);
        if (comp instanceof JTextField)
            m_txfName = (JTextField)comp;

        // add mouse listener to the name field
        m_txfName.addMouseListener(new MouseAdapter()
        {
            public void mouseClicked(MouseEvent e)
            {
                if (timer != null)
                    timer.cancel();

                m_txfName.setForeground(Color.black);
                String strName = m_txfName.getText();
                strName = (strName != null) ? strName.toLowerCase() : "";
                if (strName.startsWith("enter login name"))
                {
                    m_txfName.setText("");
                }
            }
        });
    }

    protected boolean isNewUser(Object objVal)
    {
        boolean bNewUser = false;
        String strName = "";

        if (objVal instanceof WItem)
        {
            WItem objItem = (WItem)objVal;
            strName = objItem.getText();
        }
        else if (objVal instanceof String)
        {
            String strUser = (String)objVal;
            int nIndex = strUser.lastIndexOf(File.separator);
            if (nIndex >= 0 && nIndex+1 < strUser.length())
            {
                strName = strUser.substring(nIndex+1);
            }
        }

        // Check for a new user.
        if (strName.equalsIgnoreCase("NewUser"))
            bNewUser = true;

        return bNewUser;

    }

    /**
     *  Returns the value for a component, given a key for the component.
     */
    public String getItemValue(String strQueryKey)
    {
        int nCompCount = getComponentCount();
        WObjIF obj = null;
        String strValue = null;
        String strKey;

         // get the current value entered in the component
        for (int i = 0; i < nCompCount; i++)
        {
            Component comp = getComponent(i);
            if (comp instanceof WObjIF)
            {
                obj = (WObjIF)comp;
                strKey = obj.getAttribute(VObjDef.KEYWORD);
                if (strKey != null && strKey.equals(strQueryKey) )
                {
                    strValue = obj.getValue();
                    break;
                }
            }
        }

        return strValue;
    }

    public void setAppMode() {
        String strName = getItemValue(WGlobal.NAME);
	WUserUtil.setAppMode(strName);
	
    }

    /**
     *  Sets the appdir based on the interface type.
     *  @param strItype interface type selected (imaging or experimental).
     *
     *  The appdir determines if either sylvia or lary interface comes up
     *  when the user types vnmrj. If the appdir contains "/vnmr/imaging",
     *  then it's the imaging interface, otherwise it's the exprimental interface.
     *  Therefore based on the interface type selected, modify the appdir.
     */
    public void setAppDir(String strItype, boolean bResetAppDir)
    {
        boolean bImg =  (strItype.equals(Global.IMGIF)) ? true : false;
        JTextField txfAppDir = null;

        Component comp = getAppDirTxf(this);
        if (comp != null)
            txfAppDir = (JTextField)comp;

//        Component panel = getPnlComp(this, "operators", true);
//        if (panel != null)
//        {
//            if (strItype.equals(Global.WALKUPIF))
//                panel.setEnabled(true);
//            else
//                panel.setEnabled(false);
//        }

        if (txfAppDir != null)
        {
            String strAppDir = strItype;
            if(!bResetAppDir) strAppDir = txfAppDir.getText();
            ArrayList aListDirs = WUtil.strToAList(strAppDir);
            int nIndex = aListDirs.indexOf(FileUtil.SYS_VNMR);
            if (nIndex < 0)
                nIndex = aListDirs.size() - 1;

            // if the imaging interface has been selected,
            // and the appdir doesn't contain imgdir, then add it.
            /*if (bImg && !aListDirs.contains(WGlobal.IMGDIR))
            {
                if (nIndex >= 0 && nIndex < aListDirs.size())
                    aListDirs.add(nIndex, WGlobal.IMGDIR);
                if (aListDirs.contains(WGlobal.WALKUPDIR))
                    aListDirs.remove(WGlobal.WALKUPDIR);
            }
            // else if the walkup interface has been selected, add the walkup interface
            else if (bWalkup && !aListDirs.contains(WGlobal.WALKUPDIR))
            {
                if (nIndex >= 0 && nIndex < aListDirs.size())
                    aListDirs.add(nIndex, WGlobal.WALKUPDIR);
                if (aListDirs.contains(WGlobal.IMGDIR))
                    aListDirs.remove(WGlobal.IMGDIR);
            }
            // if the experimental interface has been selected,
            // and the appdir contains imgdir, then remove it.
            else if (!bImg && aListDirs.contains(WGlobal.IMGDIR))
            {
                aListDirs.remove(WGlobal.IMGDIR);
            }
            else if (!bWalkup && aListDirs.contains(WGlobal.WALKUPDIR))
                aListDirs.remove(WGlobal.WALKUPDIR);
            strAppDir = WUtil.aListToStr(aListDirs);*/
	 
            // if appdir contains path, set it to Itype, 
            // and call SAVEUSER after appdir is set to new value.
            boolean bSaveUser = false;
            if (strAppDir.contains("/")) {
                bSaveUser = true;
                strAppDir = strItype;
            }
            strAppDir = WUserUtil.getAppDir(strAppDir, strItype);
            txfAppDir.setText(strAppDir);
            String strProperty = WGlobal.IMGDIR;
            if (bImg)
                strProperty = strProperty + " " + WGlobal.IMGDIR;
            else if (strItype.equals(Global.WALKUPIF))
                strProperty = strProperty + " " + WGlobal.WALKUPDIR;

            VItemArea1.firePropertyChng(strProperty, "", getNameEntered());

            // call SAVEUSER here
            if (bSaveUser)
                VUserToolBar.firePropertyChng(WGlobal.SAVEUSER, "ALL", "");
        }
    }

    /**
     *  Removes the components in this panel and repaints.
     */
    public void clear()
    {
        removeAll();
        repaint();
    }

    protected JComponent getPart11TitleBar()
    {
        JPanel pnlLbl = new JPanel(new BorderLayout());
        pnlLbl.add(Box.createHorizontalStrut(2) , BorderLayout.WEST);
        JButton lbl = new JButton("Record Trails");
        pnlLbl.add(lbl, BorderLayout.CENTER);
        pnlLbl.add(Box.createHorizontalStrut(2), BorderLayout.EAST);
        return pnlLbl;
    }

    protected String parseItype(String strValue)
    {
        String strItype = "";

        if (strValue != null)
        {
            StringTokenizer strTok = new StringTokenizer(strValue, " ~");
            while(strTok.hasMoreTokens())
            {
                strItype = strItype + " " + strTok.nextToken();
            }
        }
        return strItype;
    }

    /**
     *  This method looks at all the components, and returns the textfield
     *  that contains the appdir value.
     */
    public JComponent getAppDirTxf(JComponent comp)
    {
        String strAppDir = "appdir";
        return getPnlComp(comp, strAppDir, true);
    }

    /**
     *  Get the name entered in the name field in the panel.
     */
    public String getNameEntered()
    {
        String strName = getItemValue(WGlobal.NAME);
        /*JComponent comp = getPnlComp(this, WGlobal.NAME, true);
        if (comp != null && comp instanceof WEntry)
        {
            strName = ((WEntry)comp).getText();
        }*/
        strName = (strName != null) ? strName.trim() : "";
        return strName;
    }

    /**
     *  This method looks at all the components in the panel,
     *  and returns the coomponent that has the same keyword.
     */
    public JComponent getPnlComp(JComponent comp, String strKeyWord, boolean bEntry)
    {
        int nCompCount = comp.getComponentCount();

        for(int i = 0; i < nCompCount; i++)
        {
            Component compChild = comp.getComponent(i);
            if (compChild instanceof WGroup)
            {
                String strKey = ((WGroup)compChild).getAttribute(KEYWORD);
                if (strKey != null && strKey.equalsIgnoreCase(strKeyWord))
                    return getPnlComp((WGroup)compChild, strKeyWord, bEntry);
            }
            else if (compChild instanceof WEntry && bEntry)
            {
                /*String strKey = ((WEntry)compChild).getAttribute(KEYWORD);
                if (strKey != null && strKey.equalsIgnoreCase(strKeyWord))*/
                    return (WEntry)compChild;
            }
            else if (compChild instanceof WLabel && !bEntry)
            {
                /*String strKey = ((WLabel)compChild).getAttribute(KEYWORD);
                if (strKey != null && strKey.equalsIgnoreCase(strKeyWord))*/
                    return (WLabel)compChild;
            }
        }
        return null;
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
        m_bgColor = VnmrRgb.getColorByName(strColor);
        m_strBgColor = strColor;
        WUtil.doColorAction(m_bgColor, strColor, this);

        String strArea = getFullAreaName();
        WFontColors.setColorOption(strArea+WGlobal.BGCOLOR, strColor);
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

