/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.part11;

import java.awt.*;
import java.util.*;
import javax.swing.*;
import java.awt.event.*;
import javax.swing.table.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.util.*;

public class ComboFileTable extends JPanel
{
    private FileMenu m_menu;
    private FileTable m_table;
    private JScrollPane m_tableScrollPane;
    private JPanel m_menuPane;
    private MenuEditor m_menuEditor = null;
    private String m_menuPath = null;

    String m_defaultLookAndFeel;

    public ComboFileTable() {
    super();
        setOpaque(false);
    m_menu = new FileMenu();
    m_table = new FileTable();
    m_tableScrollPane = new JScrollPane();
    m_menuPane = new JPanel();
    }

    public ComboFileTable(String path, String label) {
    super();
        setOpaque(false);

        Vector paths = new Vector();
        paths.addElement(path);
    makeComboFileTable(paths, label, 0);
    }

    public ComboFileTable(Vector paths, String label) {
    super();
        setOpaque(false);

    makeComboFileTable(paths, label, 0);
    }

    public void makeComboFileTable(Vector paths, String label, int menuIndex) {

        setLayout(new BorderLayout());
        makeFileMenu(paths, menuIndex);
        m_menuPane = new JPanel();
        if(!label.endsWith(":")) label = label +":";
        JLabel l= new JLabel(label);
        m_menuPane.setLayout(new BoxLayout(m_menuPane, BoxLayout.X_AXIS));
        m_menuPane.setBorder(BorderFactory.createEmptyBorder(0, 0, 10, 10));
        m_menuPane.add(Box.createHorizontalGlue());
        m_menuPane.add(l);
        m_menuPane.add(Box.createRigidArea(new Dimension(10, 0)));
        m_menuPane.add(m_menu);

	m_menuPath = (String)paths.elementAt(0);

	//create and add editButton if menu is an AuditMenu.

	if(m_menuPath.indexOf("AuditMenu") != -1) {

	  JButton editButton = new JButton("Edit");

	  editButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {

	    JButton source = (JButton)e.getSource();
	    JPanel menuPane = (JPanel)source.getParent();
	    ComboFileTable parent = (ComboFileTable)menuPane.getParent();

	    m_defaultLookAndFeel = 
	    	UIManager.getLookAndFeel().getID();

	    setLookAndFeel("Metal");

/* commented out because getModelessPopup is no longer a static method.
   popup is the auditing popup. It is passed to m_menuEditor as the owner
   of m_menuEditor. But for now, VNMRFrame will be the owner.
   will make change to get the popup from an instance.

	    Frame popup = null;
	    String xmlfile = FileUtil.openPath("INTERFACE/audit.xml");
	    if(xmlfile != null)
	        popup = ExpPanel.getModelessPopup(xmlfile);

	    if(popup != null) { 
	    	m_menuEditor = new MenuEditor(parent, popup, m_menuPath);
	    } else {
*/
	    	m_menuEditor = 
		   new MenuEditor(parent, VNMRFrame.getVNMRFrame(), m_menuPath);

/* commented out because getModelessPopup is no longer a static method.
	    }
*/
            m_menuEditor.showDialog();

            }
          });
	  m_menuPane.add(editButton);
	}

        if (m_table == null)
        {
            m_table = new FileTable();
        }
        makeFileTable();

        m_tableScrollPane = new JScrollPane(m_table);
        m_tableScrollPane.setViewportView(m_table);

        //setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
        add(m_menuPane, BorderLayout.NORTH);
        //add(Box.createRigidArea(new Dimension(0,5)));
        add(m_tableScrollPane, BorderLayout.CENTER);
        setBorder(BorderFactory.createEmptyBorder(10,10,10,10));

        m_menu.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {

                JComboBox cb = (JComboBox)e.getSource();
                String path = (String)cb.getSelectedItem();

                makeFileTable();
            }
        });

    }

    public void updateComboFileTable(Vector paths, String label, int menuIndex)
    {
        //setLayout(new BorderLayout());
        updateFileMenu(paths, menuIndex);
        //m_menuPane = new JPanel();
        m_menuPane.removeAll();
        if(!label.endsWith(":")) label = label +":";
        JLabel l= new JLabel(label);
        m_menuPane.setLayout(new BoxLayout(m_menuPane, BoxLayout.X_AXIS));
        m_menuPane.setBorder(BorderFactory.createEmptyBorder(0, 0, 10, 10));
        m_menuPane.add(Box.createHorizontalGlue());
        m_menuPane.add(l);
        m_menuPane.add(Box.createRigidArea(new Dimension(10, 0)));
        m_menuPane.add(m_menu);

        makeFileTable();

        //m_tableScrollPane = new JScrollPane(m_table);
        m_tableScrollPane.setViewportView(m_table);

        //setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
        add(m_menuPane, BorderLayout.NORTH);
        //add(Box.createRigidArea(new Dimension(0,5)));
        //add(m_tableScrollPane, BorderLayout.CENTER);
        setBorder(BorderFactory.createEmptyBorder(10,10,10,10));
    }

    public boolean  makeFileMenu(Vector paths, int menuIndex) {
        m_menu = new FileMenu(paths);

        if(menuIndex >=0 && menuIndex < m_menu.getItemCount())
            m_menu.setSelectedIndex(menuIndex);

        boolean b = true;

        if(m_menu.getItemCount() <= 0) {
            b = false;
        }

        return(b);
    }

    public boolean  updateFileMenu(Vector paths, int menuIndex) {

    if (m_menu == null)
    {
        m_menu = new FileMenu(paths);
    }
    else {
        m_menu.updateFileMenu(paths);
    }

    if(menuIndex >=0 && menuIndex < m_menu.getItemCount())
    m_menu.setSelectedIndex(menuIndex);

        boolean b = true;

    if(m_menu.getItemCount() <= 0) {
            b = false;
        }

        return(b);
    }

    public void makeFileTable() {

    Vector paths = new Vector();
    String type = null;

    Object[] selected = m_menu.getSelectedObjects();

    if(selected.length == 1 && ((String)selected[0]).equals("All")) {
        type = m_menu.getSelectedType("All");
        for(int i=0; i<m_menu.getItemCount(); i++) {
        String key = (String)m_menu.getItemAt(i);
        if(key.equals("All") || key.equals("Trash") ) continue;
            String path = m_menu.getSelectedPath(key);
            String t = m_menu.getSelectedType(key);
            if(t.equals(type) && !paths.contains(path)) paths.addElement(path);
        }

    } else if(selected.length == 1) {
    String key = (String)selected[0];
        String path = m_menu.getSelectedPath(key);
        type = m_menu.getSelectedType(key);
        paths.addElement(path);

    } else {
      for(int i=0; i<selected.length; i++) {
        String key = (String)selected[i];
    if(key.equals("All") || key.equals("Trash") ) continue;
        String path = m_menu.getSelectedPath(key);
        String t = m_menu.getSelectedType(key);
        if(i == 0) type = t;
        if(t.equals(type) && !paths.contains(path)) {
            paths.addElement(path);
        }
      }
    }

    m_table.makeFileTable(paths, type);
    if(m_tableScrollPane != null)
    m_tableScrollPane.repaint();
    }

    public void updateFileTable() {

    Vector paths = new Vector();
    String type = null;

    Object[] selected = m_menu.getSelectedObjects();

    if(selected.length == 1 && ((String)selected[0]).equals("All")) {
        type = m_menu.getSelectedType("All");
        for(int i=0; i<m_menu.getItemCount(); i++) {
            String key = (String)m_menu.getItemAt(i);
            if(key.equals("All") || key.equals("Trash") ) continue;
            String path = m_menu.getSelectedPath(key);
            String t = m_menu.getSelectedType(key);
            if(t.equals(type) && !paths.contains(path)) paths.addElement(path);
        }

    } else if(selected.length == 1) {
        String key = (String)selected[0];
        String path = m_menu.getSelectedPath(key);
        type = m_menu.getSelectedType(key);
        paths.addElement(path);

    } else {
      for(int i=0; i<selected.length; i++) {
        String key = (String)selected[i];
        if(key.equals("All") || key.equals("Trash") ) continue;
        String path = m_menu.getSelectedPath(key);
        String t = m_menu.getSelectedType(key);
        if(i == 0) type = t;
        if(t.equals(type) && !paths.contains(path)) {
            paths.addElement(path);
        }
      }
    }

    m_table.updateFileTable(paths, type);
    if(m_tableScrollPane != null)
    m_tableScrollPane.repaint();
    }

    public JScrollPane getScrollPane()
    {
        return m_tableScrollPane;
    }

    public FileTable getTable() {
        return(this.m_table);
    }

    public FileMenu getFileMenu() {
    return(this.m_menu);
    }

    public String getMenuPath() {
	return m_menuPath;
    }

    public String getdefaultLookAndFeel() {
	return m_defaultLookAndFeel;
    }

    public void setLookAndFeel(String lookAndfeel) {

        // JavaLookAndFeel: Metal.
        // MotifLookAndFeel: CDE/Motif.

        lookAndfeel = lookAndfeel.toLowerCase();

        try {
            UIManager.LookAndFeelInfo[] lafs = UIManager.
                                            getInstalledLookAndFeels();
            for (int i=0; i<lafs.length; i++) {
                String laf = lafs[i].getName().toLowerCase();
                if (laf.indexOf(lookAndfeel) >= 0) {
                    lookAndfeel = lafs[i].getClassName();
                    break;
                }
            }

            UIManager.setLookAndFeel(lookAndfeel);
        } catch (Exception ex) {
            Messages.postError("unable to set UI " + ex.getMessage());
            Messages.writeStackTrace(ex, "Error caught in setLookAndFeel()");
        }

    }
}

