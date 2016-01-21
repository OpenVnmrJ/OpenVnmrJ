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
import java.awt.event.*;
import java.io.File;
import java.io.FilenameFilter;
import java.util.SortedSet;
import java.util.TreeSet;

import javax.swing.*;

import vnmr.ui.*;
import vnmr.util.ButtonIF;
import vnmr.util.FileUtil;
import vnmr.util.ParamIF;

import static java.awt.event.InputEvent.*;

public class VCascadingMenu extends VObj implements ActionListener {

    private JPopupMenu m_popupMenu;

    /** Whether to show hidden files. */
    private String m_sShowDotFiles = "no";

    private MouseMenuListener m_mouseMenuListener;

    /**
     * Path to the root of the menu tree.
     * If this is a relative path, like "lc/lcmethods", the system and
     * user directory contents will be merged.
     * May be an abstract path, like "SYSTEM/lc/lcmethods".
     */
    private String m_sMenuDirPath = "";


    /**
     * Normal constructor.
     * @param sshare The session share object.
     * @param vif The button callback interface.
     * @param typ The type of the object.
     */
    public VCascadingMenu(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
        setBorder(BorderFactory.createRaisedBevelBorder());
        m_popupMenu = new JPopupMenu();

        MouseListener mouseListener = new MouseAdapter() {
            public void mousePressed(MouseEvent me) {
                int modifier = me.getModifiers();
                if ((modifier & BUTTON1_MASK) == BUTTON1_MASK) {
                    Component comp = me.getComponent();
                    buildMenu(m_popupMenu, "", m_sMenuDirPath);
                    m_popupMenu.show(comp, 0, comp.getHeight());
                }
            }
        };
        addMouseListener(mouseListener);
        m_mouseMenuListener = new MouseMenuListener();
    }

    public void paintComponent(Graphics g) {
        // Background
        if (isOpaque()) {
            g.setColor(getBackground());
            g.fillRect(0, 0, getWidth(), getHeight());
        }

        // Draw arrow button at right end of bar
        Insets insets = getInsets();
        int width = getWidth();
        int height = getHeight();
        int x = width - height + insets.left; // Make button square
        int y = insets.top;
        height -= insets.top + insets.bottom;
        width = height;         // Make button square

        // Etched border between main area and arrow button
        g.setColor(getBackground().brighter());
        g.drawLine(x - 1, y, x - 1, y + height - 1);
        g.setColor(getBackground().darker());
        g.drawLine(x - 2, y, x - 2, y + height - 1);

        // Arrow on middle of button
        g.setColor(getBackground().darker().darker());
        final int ICON_HEIGHT = 4;
        int[] xpts = new int[3];
        int[] ypts = new int[3];
        ypts[0] = ypts[1] = y + (height / 2) - ICON_HEIGHT / 2;
        ypts[2] = y + (height / 2) + ICON_HEIGHT / 2 - 1;
        xpts[2] = x + (width / 2) - 1;
        xpts[0] = xpts[2] - ICON_HEIGHT + 1;
        xpts[1] = xpts[2] + ICON_HEIGHT - 1;
        g.drawPolygon(xpts, ypts, 3);
        g.fillPolygon(xpts, ypts, 3);

        // Name on the main area
        String strValue = getAttribute(VALUE);
        if (strValue == null) {
            strValue = getAttribute(LABEL);
            if (strValue == null) {
                strValue = "";
            }
        }
        String displayStr = strValue.replaceAll("\\Q" + File.separator + "\\E",
                                                " " + File.separator + " ");
        FontMetrics fontMetrics = g.getFontMetrics();
        x = insets.left + 1;
        y = getHeight() - insets.bottom - fontMetrics.getDescent();
        width = getWidth() - getHeight() - insets.left - 2;
        // Trim leading end of string to fit within width
        String clippedStr = displayStr;
        int strWidth = fontMetrics.stringWidth(clippedStr);
        if (strWidth > width) {
            int len = displayStr.length();
            int start = 0;
            int delta = len / 2;
            while (delta != 0) {
                if (strWidth == width) {
                    delta = 0;
                } else if (strWidth < width) {
                    start -= delta;
                } else {
                    start += delta;
                }
                delta /= 2;
                clippedStr = "..." + displayStr.substring(start);
                strWidth = fontMetrics.stringWidth(clippedStr);
            }
            // Last step could have made string too long, so...
            while (strWidth > width) {
                start = Math.max(0, Math.min(len, start + 1));
                clippedStr = "..." + displayStr.substring(start);
                strWidth = fontMetrics.stringWidth(clippedStr);
            }
        }
        g.setColor(fgColor);
        g.setFont(getFont());
        g.drawString(clippedStr, x, y);
    }

    public String getAttribute(int attr) {
        switch (attr) {
        case PANEL_FILE:
            return m_sMenuDirPath;
        case STATSHOW:
            return m_sShowDotFiles;
        default:
            return super.getAttribute(attr);
        }
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
        case VALUE:
            value = c;
            repaint();
            break;
        case PANEL_FILE:
            m_sMenuDirPath = c;
            break;
        case STATSHOW:
            m_sShowDotFiles = c;
            break;
        default:
            super.setAttribute(attr, c);
            break;
        }
    }

    public void setValue(ParamIF pf) {
        setAttribute(VALUE, pf.value);
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
        if (branchName.length() > 0) {
            branchName = branchName + File.separator;
        }
        if (branchDir.length() > 0) {
            branchDir = branchDir + File.separator;
        }

        branchMenu.removeAll();

        String[] dirs = FileUtil.getAllVnmrDirs(branchDir);
        boolean showHidden = m_sShowDotFiles.startsWith("y");

        // First, add subdirectories to branchMenu
        SortedSet<String> subdirs = new TreeSet<String>(); // List of dir names
        for (String dir : dirs) {
            File[] dirfiles = new File(dir)
                    .listFiles(new MenuFileFilter(false, // ~showFiles
                                                  true, // showDirs
                                                  showHidden));
            for (File subdir : dirfiles) {
                subdirs.add(subdir.getName());
            }
        }
        for (String subdir : subdirs) {
            JMenu submenu = new JMenu(subdir);
            //submenu.setBackground(Util.getBgColor());
            branchMenu.add(submenu);
            submenu.addMouseListener(m_mouseMenuListener);
        }

        // Next, add regular files to branchMenu (with actions)
        SortedSet<String> fnames = new TreeSet<String>(); // List of file names
        for (String dir : dirs) {
            File[] files = new File(dir)
                    .listFiles(new MenuFileFilter(true, // showFiles
                                                  false, // ~showDirs
                                                  showHidden));
            for (File file : files) {
                fnames.add(file.getName());
            }
        }
        for (String file : fnames) {
            JMenuItem item = new JMenuItem(file);
            item.setActionCommand(branchName + file);
            item.addActionListener(this);
            //item.setBackground(Util.getBgColor());
            branchMenu.add(item);
        }
    }


    protected final static String[] m_aStrShow = {"yes", "no"};

    private final static Object[][] attributes = {
        {new Integer(LABEL),        "Label of item:"},
        {new Integer(VARIABLE),     "Selection variables:"},
        {new Integer(SETVAL),       "Value of item:"},
        {new Integer(SHOW),         "Enable condition:"},
        {new Integer(CMD),          "Vnmr command:"},
        {new Integer(PANEL_FILE),   "Menu source:"},
        {new Integer(STATSHOW),     "Show Dot Files:", "radio", m_aStrShow},
    };

    public Object[][] getAttributes()  { return attributes; }


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
                              boolean showDirectories, boolean showHidden) {
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

    public void actionPerformed(ActionEvent ae) {
        String cmd = ae.getActionCommand();
        setAttribute(VALUE, cmd);
        sendVnmrCmd();
    }


    class MouseMenuListener extends MouseAdapter {

        public void mouseEntered(MouseEvent me) {
            JMenu menu = (JMenu)me.getComponent();
            String name = menu.getText();
            String dir = getAttribute(PANEL_FILE) + File.separator + name;
            buildMenu(menu, name, dir); 
        }
    }

}
