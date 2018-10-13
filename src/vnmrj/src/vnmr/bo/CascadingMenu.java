/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.bo;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.SortedSet;
import java.util.TreeSet;

import javax.swing.JComponent;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import javax.swing.JPopupMenu;

import vnmr.util.FileUtil;
import vnmr.util.Messages;

public class CascadingMenu extends JPopupMenu {

    private ActionListener m_actionListener;
    private boolean m_showHidden;
    public String m_menuDir;


    public CascadingMenu(ActionListener listener,
                         String menuDir,
                         boolean showHidden) {
        m_actionListener = listener;
        m_menuDir = menuDir;
        m_showHidden = showHidden;
    }

    public void showMenu(Component comp, int x, int y) {
        buildMenu(this, "", m_menuDir);
        show(comp, x, y);
    }

    /**
     * Build a menu containing the directories and files found in a
     * given directory.
     * Note that branchName is probably rooted at the directory at the
     * base of the menu, giving values like "" (for entries an the
     * root of the menu tree) or "pfizer/examples", while branchDir is
     * rooted at userdir or systemdir, so we have things like
     * "lc/lcmethods" or "lc/lcmethods/pfizer/examples".
     * @param branchMenu The menu to which the items are added.
     * @param branchName The directory path prepended to file names.
     * @param branchDir The directory searched for files. If not
     * absolute, looks relative to both the sytem and user
     * directories.
     */
    private void buildMenu(JComponent branchMenu,
                           String branchName, String branchDir) {
        //Messages.postDebug("buildMenu: name=" + branchName + ", dir=" + branchDir);/*CMP*/
        if (branchName.length() > 0) {
            branchName = branchName + File.separator;
        }
        if (branchDir.length() > 0) {
            branchDir = branchDir + File.separator;
        }

        branchMenu.removeAll();

        String[] dirs = FileUtil.getAllVnmrDirs(branchDir);
        //Messages.postDebug("dirs=" + Arrays.deepToString(dirs));/*CMP*/

        // First, add subdirectories to branchMenu
        SortedSet<String> subdirs = new TreeSet<String>(); // List of dir names
        for (String dir : dirs) {
            File[] dirfiles = new File(dir)
                    .listFiles(new MenuFileFilter(false, // ~showFiles
                                                  true, // showDirs
                                                  m_showHidden));
            for (File subdir : dirfiles) {
                subdirs.add(subdir.getName());
            }
        }
        for (String subdir : subdirs) {
            //Messages.postDebug("subdir=" + subdir);/*CMP*/
            JMenu submenu = new JMenu(subdir);
            //submenu.setBackground(Util.getBgColor());
            branchMenu.add(submenu);
            submenu.addMouseListener(new MouseMenuListener(branchDir));
        }

        // Next, add regular files to branchMenu (with actions)
        SortedSet<String> fnames = new TreeSet<String>(); // List of file names
        for (String dir : dirs) {
            File[] files = new File(dir)
                    .listFiles(new MenuFileFilter(true, // showFiles
                                                  false, // ~showDirs
                                                  m_showHidden));
            for (File file : files) {
                fnames.add(file.getName());
            }
        }
        for (String file : fnames) {
            JMenuItem item = new JMenuItem(file);
            item.setActionCommand(branchDir + file);
            item.addActionListener(m_actionListener);
            //item.setBackground(Util.getBgColor());
            branchMenu.add(item);
        }
    }

//    public void actionPerformed(ActionEvent e) {
//        // TODO Auto-generated method stub
//        Messages.postDebug("CascadingMenu.actionPerformed: "
//                           + e.getActionCommand());
//        for (ActionListener listener : m_listeners) {
//            listener.actionPerformed(e);
//        }
//    }


    /**
     * A FilenameFilter that selects for directories or plain files.
     */
    class MenuFileFilter implements FilenameFilter {

        private boolean m_showFiles;
        private boolean m_showDirectories;
        private boolean m_showHidden;

        /**
         * Instantiate a FilenameFilter that selects for directories
         * and/or plain files.
         * @param showFiles Include plain files in selection.
         * @param showDirectories Include directories in selection.
         * @param showHidden Include hidden files/directories in selection.
         */
        public MenuFileFilter(boolean showFiles,
                              boolean showDirectories,
                              boolean showHidden) {
            m_showFiles = showFiles;
            m_showDirectories = showDirectories;
            m_showHidden = showHidden;
        }

        public boolean accept(File path, String name) {
            boolean bShow = true;
            File file = new File(path, name);
            if ((file.isHidden() && !m_showHidden)
                || (file.isDirectory() && !m_showDirectories)
                || (!file.isDirectory() && !m_showFiles))
            {
                bShow = false;
            }
            return bShow;
        }
    }


    class MouseMenuListener extends MouseAdapter {
        private String m_dir;
        public MouseMenuListener(String dir) {
            m_dir = dir;
        }

        public void mouseEntered(MouseEvent me) {
            //Messages.postDebug("mouseEntered " + me.getComponent());/*CMP*/
            JMenu menu = (JMenu)me.getComponent();
            String name = menu.getText();
            String dir = m_dir + name;
            buildMenu(menu, name, dir); 
        }
    }

}
