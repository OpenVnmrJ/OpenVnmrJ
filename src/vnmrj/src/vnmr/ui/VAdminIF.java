/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.io.*;
import java.awt.event.*;
import java.beans.*;
import java.util.*;
import javax.swing.*;
import javax.swing.plaf.basic.*;

import  vnmr.bo.*;
import  vnmr.ui.shuf.*;
import  vnmr.util.*;
import  vnmr.ui.*;
import  vnmr.admin.vobj.*;
import  vnmr.admin.ui.*;
import  vnmr.admin.util.*;

/**
 * Title:   VAdminIF
 * Description: The administrator interface. This class does the initial layout,
 *              and initializes the methods and variables.
 * Copyright:    Copyright (c) 2001
 */

public class VAdminIF extends AppIF
{

    private static int LAYER0 = JLayeredPane.DEFAULT_LAYER.intValue();
    private static int LAYER1 = LAYER0 + 10;
    private static int LAYER2 = LAYER0 + 20;
    private static int LAYER3 = LAYER0 + 30;
    private Rectangle  m_frameRec;
    private WandaMenuBar  m_vmenuBar;
    protected PropertyChangeListener m_pclAdmin;

    protected VItemArea1 m_pnlItemArea1;
    protected VItemArea2 m_pnlItemArea2;
    protected VDetailArea m_pnlDetailArea1;
    protected VDetailArea m_pnlDetailArea2;
    protected JPanel m_pnlErrorBar;
    protected JPanel m_pnlMenuBar;
    protected VUserToolBar m_userToolBar;
    protected VUserToolBar m_groupToolBar;
    protected JSplitPane m_spLeftPane;
    protected JSplitPane m_spRightPane;
    protected JSplitPane m_spFullPane;
    protected JScrollPane m_scpPane1;
    protected JScrollPane m_scpPane2;
    protected WDialogHandler m_dialogHandler;
    protected WTrashCan m_trashCan;

    protected static WCurrInfo m_objCurrInfo;
    protected int m_nLeftMinWidth = 0;
    protected int m_nRightMinWidth = 0;
    protected static AppInstaller m_appInstaller;
    protected String m_strCurrentAdmin;


    public VAdminIF(AppInstaller appInstaller, User user)
    {
        super(appInstaller, user);
        Util.setUser(user);
        Util.setAppIF(this);
        m_appInstaller = appInstaller;

        // to check if there is layout info for  last session
        Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
        dim.width = dim.width * 4 / 5;
        dim.height = dim.height * 4 / 5;
        m_frameRec = new Rectangle(10, 10, dim.width, dim.height);
        //initUiLayout();
        initListeners();
        m_objCurrInfo = new WCurrInfo();

        try
        {
            setLayout(new BorderLayout());
            AdminLayered objLayered = new AdminLayered(sshare, this);
            add(objLayered, BorderLayout.CENTER);

            // when the interface comes up, show the users panel.
            WSubMenuItem.showUsersPanel();

            // start infostat
            WInfoStat infoStat = new WInfoStat(this);

            WSubMenuItem.addChangeListener("exit", m_pclAdmin);

        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
        if(DebugOutput.isSetFor("needupdateuser"))
            Messages.postWarning("You need to \"Configure -> Users -> Update users\" in this panel");
    }

    public void setMenuBar(JComponent b)
    {
        if (topMenuBar != null)
            topMenuBar.setMenuBar(b);
    }

    public void showLoginBox()
    {
        ArrayList aListUsers = WUserUtil.getAdminList();
        if (aListUsers != null && aListUsers.size() > 1)
            WLoginBox.showLoginBox(true, "help:SYSTEM/jhelp/dialog/login.html");
    }

    public String getCurrentAdmin()
    {
        if (m_strCurrentAdmin == null)
            m_strCurrentAdmin = System.getProperty("user.name");
        return m_strCurrentAdmin;
    }

    public void setCurrentAdmin(String strAdmin)
    {
        m_strCurrentAdmin = strAdmin;
    }

    public void showTrashCan(boolean bVisible)
    {
        m_trashCan.setVisible(bVisible);
    }

    public VItemAreaIF getItemArea1()
    {
        return m_pnlItemArea1;
    }

    public VItemAreaIF getItemArea2()
    {
        return m_pnlItemArea2;
    }

    public VDetailArea getDetailArea1()
    {
        return m_pnlDetailArea1;
    }

    public VDetailArea getDetailArea2()
    {
        return m_pnlDetailArea2;
    }

    public VUserToolBar getUserToolBar()
    {
        return m_userToolBar;
    }

    public VUserToolBar getGroupToolBar()
    {
        return m_groupToolBar;
    }

    public void setDragPanelVis(boolean bVisible)
    {
         m_userToolBar.setItemsVisible(bVisible);
        //m_groupToolBar.setItemsVisible(bVisible);
    }

    public WCurrInfo getCurrInfo()
    {
        return m_objCurrInfo;
    }

    public JSplitPane getFullPane()
    {
        return m_spFullPane;
    }

    public JSplitPane getLeftPane()
    {
        return m_spLeftPane;
    }

    public JSplitPane getRightPane()
    {
        return m_spRightPane;
    }

    public HashMap getUserDefaults()
    {
        return m_dialogHandler.getUsersConfigObj().getDefaults();
    }

    public void setLeftSplitPane(double dLocation, double dWeight, boolean bExpand)
    {
        setSplitPane(m_spLeftPane, dLocation, dWeight, bExpand);
    }

    public void setRightSplitPane(double dLocation, double dWeight, boolean bExpand)
    {
        setSplitPane(m_spRightPane, dLocation, dWeight, bExpand);
    }

    public void setFullSplitPane(double dLocation, double dWeight, boolean bExpand)
    {
        setSplitPane(m_spFullPane, dLocation, dWeight, bExpand);
    }

    public void setSplitPaneDefaults()
    {
        setPaneSettings(m_spLeftPane);
        setPaneSettings(m_spRightPane);
        setPaneSettings(m_spFullPane);
    }

    public void setSplitPane(JSplitPane objPane, double dLocation,
                                    double dWeight, boolean bExpand)
    {
        objPane.setOneTouchExpandable(bExpand);
        objPane.setResizeWeight(dWeight);
        objPane.setDividerLocation(dLocation);
    }

    public void clearPanels()
    {
        m_pnlItemArea1.removeAll();
        m_pnlItemArea2.removeAll();
        m_pnlDetailArea1.removeAll();
        m_pnlDetailArea2.removeAll();
        repaint();
    }

    public void notifyLogout()
    {
        //expViewArea.logout();
    }

    public void initListeners()
    {
        m_pclAdmin = new PropertyChangeListener()
        {
            public void propertyChange(PropertyChangeEvent e)
            {
                String strPropName = e.getPropertyName();
                //writeEventFile();
                if (strPropName.equalsIgnoreCase("Exit"))
                {
                    m_appInstaller.exitAll();
                }
            }

        };

    }

    protected void setPaneSettings(JSplitPane objPane)
    {
        objPane.setOneTouchExpandable(true);
        //objPane.setDividerSize(2);
        objPane.setResizeWeight(0.5);
        objPane.setDividerLocation(0.5);
    }

    public void initLayout()
    {
        Container pp = getParent();
        while (pp != null)
        {
           if (pp instanceof JFrame)
           {
                JFrame frame = (JFrame) pp;
                initLayout(frame);
                break;
           }
           pp = pp.getParent();
        }
    }

    public void initLayout(JFrame f)
    {
        Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
        if (m_frameRec.width > dim.width)
            m_frameRec.width = dim.width - 10;
        if (m_frameRec.height > dim.height)
            m_frameRec.height = dim.height - 10;
        Point pt = new Point (m_frameRec.x, m_frameRec.y);
        if (m_frameRec.x + m_frameRec.width >  dim.width)
            pt.x = dim.width - m_frameRec.width;
        if (m_frameRec.y + m_frameRec.height >  dim.height)
            pt.y = dim.height - m_frameRec.height;
        f.setLocation(pt);
        f.setSize(new Dimension(m_frameRec.width, m_frameRec.height));
    }

    public WDialogHandler getWDialogHandler() {
        return m_dialogHandler;
    }

    /**
     * The panel's primary responsibilities are application containment
     * and layout management.
     */
    class AdminLayered extends JLayeredPane implements PropertyChangeListener
    {

        /**
         * constructor
         * @param sshare session share
         */
        public AdminLayered(SessionShare sshare, VAdminIF ap)
        {

            setLayout(new AdminLayout());

            initUILayout(ap);

            WFontColors.addChangeListener(this);
            // fire the propertychange after initializing everything,
            // to set the initial colors as in the persistence file.
            WFontColors.firePropertyChng();
        } // AdminLayered()

        public void propertyChange(PropertyChangeEvent e)
        {
            String strPropName = e.getPropertyName();

            if (strPropName.equals(WGlobal.FONTSCOLORS))
            {
                String strColName=WFontColors.getColorOption(WGlobal.ADMIN_BGCOLOR);
                Color bgColor = (strColName != null) ? WFontColors.getColor(strColName)
                                            : Global.BGCOLOR;
                setBackground(bgColor);
                WUtil.doColorAction(bgColor, strColName, this, true);
            }
        }

        protected void initUILayout(VAdminIF ap)
        {
            m_pnlMenuBar = new JPanel();
            m_pnlMenuBar.add(new JLabel("Menu Bar"));
            m_pnlMenuBar.setBackground(Color.cyan);

            expViewArea = new ExpViewArea(sshare, ap);
            WFileUtil objFileUtil = new WFileUtil();

            topMenuBar = new MenuPanel();
            add(topMenuBar, new Integer(LAYER1));

            m_vmenuBar = new WandaMenuBar(sshare, null, 0);
            setMenuBar(m_vmenuBar);

            m_dialogHandler = new WDialogHandler(sshare, ap, expViewArea);

            JButton btnItemDrag1 = new JButton("New User");
            m_userToolBar = new VUserToolBar(sshare, btnItemDrag1, WGlobal.AREA1);
            m_userToolBar.setVisible(false);

            JButton btnItemDrag2 = new JButton("New Group");
            m_groupToolBar = new VUserToolBar(sshare, btnItemDrag2, WGlobal.AREA2);
            m_groupToolBar.setVisible(false);

            JPanel pnl2 = new JPanel();
            m_pnlItemArea2 = new VItemArea2(sshare);
            JScrollPane scrollPane2 = new JScrollPane(pnl2);
            //add(m_pnlItemArea2, new Integer(LAYER1));
            m_scpPane2 = new JScrollPane(m_pnlItemArea2);
            pnl2.setLayout(new BorderLayout(5, 10));
            pnl2.setBackground(Global.BGCOLOR);
            pnl2.add(m_groupToolBar, BorderLayout.NORTH);
            pnl2.add(m_scpPane2, BorderLayout.CENTER);

            JPanel pnl1 = new JPanel();
            m_pnlItemArea1 = new VItemArea1(sshare);
            JScrollPane scrollPane1 = new JScrollPane(pnl1);
            //add(m_pnlItemArea1, new Integer(LAYER1));
            m_scpPane1 = new JScrollPane(m_pnlItemArea1);
            pnl1.setLayout(new BorderLayout(5, 10));
            pnl1.setBackground(Global.BGCOLOR);
            pnl1.add(m_userToolBar, BorderLayout.NORTH);
            pnl1.add(m_scpPane1, BorderLayout.CENTER);

            m_pnlDetailArea1 = new VDetailArea(sshare, expViewArea.getDefaultExp(), WGlobal.AREA1);
            //add(m_pnlDetailArea1, new Integer(LAYER2));
            JScrollPane scrollPane3 = new JScrollPane(m_pnlDetailArea1);
            scrollPane3.setBackground(Global.BGCOLOR);

            m_pnlDetailArea2 = new VDetailArea(sshare, expViewArea.getDefaultExp(), WGlobal.AREA2);
            //add(m_pnlDetailArea2, new Integer(LAYER2));
            JScrollPane scrollPane4 = new JScrollPane(m_pnlDetailArea2);
            scrollPane4.setBackground(Global.BGCOLOR);

            m_pnlErrorBar = new JPanel();
            m_pnlErrorBar.add(new JLabel(""));
            add(m_pnlErrorBar, new Integer(LAYER1));

            statusBar = new ExpStatusBar(sshare, expViewArea.getDefaultExp(),
                                                    "INTERFACE/AdminHardwareBar.xml");
            add(statusBar, new Integer(LAYER1));

            m_trashCan = new WTrashCan();

            m_spRightPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT, true, scrollPane3, scrollPane4);
            m_spLeftPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT, true, pnl1, pnl2);
            m_spFullPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true,
                                            m_spLeftPane, m_spRightPane);
            /*m_spTopPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true, pnl1, scrollPane3);
            m_spBottomPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true, pnl2, scrollPane4);
            m_spFullPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT, true, m_spTopPane, m_spBottomPane);*/

            setPaneSettings(m_spLeftPane);
            setPaneSettings(m_spRightPane);
            setPaneSettings(m_spFullPane);

            m_spLeftPane.setUI(new WBasicSplitPaneUI());
            m_spRightPane.setUI(new WBasicSplitPaneUI());
            m_spFullPane.setUI(new WBasicSplitPaneUI());

            add(m_spFullPane, new Integer(LAYER2));

            // initialize the change listeners for the panels after everything
            // is initialized.
            m_pnlItemArea1.initChngListeners();
            m_pnlItemArea2.initChngListeners();
            m_pnlDetailArea1.initChngListeners();
            m_pnlDetailArea2.initChngListeners();
            m_vmenuBar.initChngListeners();
        }

        protected class WBasicSplitPaneUI extends BasicSplitPaneUI
        {
            /**
             * Creates the default divider.
             */
            public BasicSplitPaneDivider createDefaultDivider()
            {
                return new WBasicSplitPaneDivider(this);
            }

        }

        protected class WBasicSplitPaneDivider extends BasicSplitPaneDivider
        {
            public WBasicSplitPaneDivider(WBasicSplitPaneUI ui)
            {
                super(ui);
            }

        }


        /**
         * The layout manager for WalkupLayered.  By defining this layout
         * as an inner class of WalkupLayered, much redundancy is reduced.
         */
        class AdminLayout implements LayoutManager {

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
                 *   - let toolbar, statusBar have preferred heights
                 *   - divide the rest of the space into four quadrants
                 */
                synchronized (target.getTreeLock()) {
                    Dimension targetSize = target.getSize();
                    Insets insets = target.getInsets();
                    int usableWidth =
                        targetSize.width - insets.left - insets.right;

                    // horizontal partitions
                    int h0 = insets.left;
                    int h1 = h0 + m_pnlItemArea1.getPreferredSize().width;
                    int h2 = h0 + (int)Math.rint(0.50 * usableWidth);
                    int h3 = h0 + (int)Math.rint(0.78 * usableWidth);
                    int h4 = targetSize.width - insets.right;

                    // vertical partitions
                    int v0 = insets.top;
                    int v1 = v0 + m_pnlMenuBar.getPreferredSize().height;
                    //int v10 = v1 + m_userToolBar.getPreferredSize().height;
                    int v10 = v1;
                    int v7 = targetSize.height - insets.bottom;
                    int v6 = v7 - statusBar.getPreferredSize().height;
                    int v8 = v6 - m_pnlErrorBar.getPreferredSize().height;
                    int usableHeight = v6; // remaining height
                    int v3 = v1 + (int)Math.rint(0.30 * usableHeight);

                    topMenuBar.setBounds(new Rectangle(h0, v0, h4 - h0, v1 - v0));
                    statusBar.setBounds(new Rectangle(h0, v6, h4 - h0, v7 - v6));
                    m_pnlErrorBar.setBounds(new Rectangle(h0, v8, h4 - h0, v6 - v8));
                    //m_userToolBar.setBounds(new Rectangle(h0, v1, h2 - h0, v10 - v1));
                    //m_groupToolBar.setBounds(new Rectangle(h0, v3 + v10, h2 - h0, v3 - v0));
                    m_spFullPane.setBounds(new Rectangle(h0, v10, h4-h0, v8-v10));

                    int nLeftHeight = m_spLeftPane.getMinimumSize().height;
                    int nRightHeight = m_spRightPane.getMinimumSize().height;
                    int nDivLoc = m_spFullPane.getDividerLocation();
                    m_spLeftPane.setMinimumSize(new Dimension(m_nLeftMinWidth, nLeftHeight));
                    m_spRightPane.setMinimumSize(new Dimension(m_nRightMinWidth, nRightHeight));
                    m_scpPane1.setMaximumSize(new Dimension(nDivLoc, nLeftHeight));
                    m_scpPane2.setMaximumSize(new Dimension(nDivLoc, nLeftHeight));
                    m_scpPane1.setBounds(0, 0, nDivLoc, nLeftHeight);
                }
            } // layoutContainer()

        } // class AdminLayout

    } // class AdminLayered

}
