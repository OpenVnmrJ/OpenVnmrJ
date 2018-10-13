/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.part11;

import java.io.File;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.io.IOException;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.filechooser.*;
import javax.swing.table.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.util.*;

public class MenuEditor extends ModalDialog implements ActionListener{

    String m_type = null; // records or system
    String m_path = null; // path of menu file
    final String[] m_header = {"Path", "Label", "Type"}; 
    JTable m_table = null; // table of menu items in editing popup
    DefaultTableModel m_model = null;
    Hashtable m_filetypes = null; // types of audit trails, depending on m_type 
    ComboFileTable m_parent = null; // parent where MenuEditor is poped up.
    String m_defaultLookAndFeel = null;

    final String RECORDS = "Records";
    final String TRASH = "Trashed records";
    final String SESSIONS = "Sessions audit trails";
    final String UAUDIT = "user account audit trails";
    final String AAUDIT = "auditing audit trails";

    public MenuEditor(ComboFileTable parent, Frame owner, String path) {
        super(owner, "MenuEditor");

	m_parent = parent;
	m_defaultLookAndFeel = m_parent.getdefaultLookAndFeel();
	m_path = path;
	if(m_path.indexOf("RecordAuditMenu") != -1)
		m_type = "records";
	else if(m_path.indexOf("SystemAuditMenu") != -1)
		m_type = "system";

	makeFileTypes();
 
        //Create the table first, because the action listeners
        //need to refer to it.

	m_table = new JTable();
	m_table.setColumnSelectionAllowed(false);
	m_table.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);

	//m_table.setModel(getCurrentMenu());

        JScrollPane logScrollPane = new JScrollPane(m_table);

        //Create the default button
        JButton defaultButton = new JButton("Default menu");
        defaultButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
		m_table.setModel(getDefaultMenu());
	    }
        });

        //Create the remove button
        JButton removeButton = new JButton("remove selected row(s)");
        removeButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
		int ind = -1;
		while((ind = m_table.getSelectedRow()) != -1) {
		    m_model = (DefaultTableModel)m_table.getModel();
		    m_model.removeRow(ind);
		}
	    }
        });

        //Create a file chooser
        final JFileChooser fc = new JFileChooser();

	fc.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);

	FileFilter[] filters = fc.getChoosableFileFilters();
	for(int i=0; i<filters.length; i++)
	    fc.removeChoosableFileFilter(filters[i]);

	if(m_type.equals("records")) {
    	    RecordFileFilter recordfilter = new RecordFileFilter();
    	    TrashFileFilter trashfilter = new TrashFileFilter();
	    fc.addChoosableFileFilter(recordfilter);
	    fc.addChoosableFileFilter(trashfilter);
    	    fc.setFileFilter(recordfilter);
	} else if(m_type.equals("system")) {
    	    SessionFileFilter sessionfilter = new SessionFileFilter();
    	    UauditFileFilter uauditfilter = new UauditFileFilter();
    	    AauditFileFilter aauditfilter = new AauditFileFilter();
	    fc.addChoosableFileFilter(sessionfilter);
	    fc.addChoosableFileFilter(uauditfilter);
	    fc.addChoosableFileFilter(aauditfilter);
    	    fc.setFileFilter(sessionfilter);
	}

        //Create the browser button
        JButton browserButton = new JButton("Add a directory...");
        browserButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                int returnVal = fc.showDialog(MenuEditor.this, "Add");

                if (returnVal == JFileChooser.APPROVE_OPTION) {
                    File file = fc.getSelectedFile();
                    //this is where a real application would add the file.
		    String newpath = file.getPath();
		    FileFilter curFilter = fc.getFileFilter();	    
		    String key = curFilter.getDescription();
		    String type = (String) m_filetypes.get(key);
		    String label = newpath +": "+ type;
		    if(lastRowEmpty() ) {
                        m_table.setValueAt(newpath, m_table.getRowCount()-1, 0);
                        m_table.setValueAt(label, m_table.getRowCount()-1, 1);
                        m_table.setValueAt(type, m_table.getRowCount()-1, 2);
                    } else {

		    	Vector row = new Vector();
		    	row.add(newpath);
		    	row.add(label);
		    	row.add(type);
		    	m_model.addRow(row); 
                    	m_table.setModel(m_model);
                    }
                } 
            }
        });

        //For layout purposes, put the buttons in a separate panel
        JPanel buttonPanel = new JPanel();
        buttonPanel.add(defaultButton);
        buttonPanel.add(browserButton);
        buttonPanel.add(removeButton);

        //Add the buttons and the log to the frame
        Container contentPane = getContentPane();
        contentPane.add(buttonPanel, BorderLayout.NORTH);
        contentPane.add(logScrollPane, BorderLayout.CENTER);

	addWindowListener(new WindowAdapter() {
	    public void windowClosing(WindowEvent we) {
	    okAction();
            }
        });

        // Set the buttons up with Listeners
        okButton.addActionListener(this);
        okButton.setActionCommand("ok");
        cancelButton.addActionListener(this);
        cancelButton.setActionCommand("cancel");
        helpButton.addActionListener(this);
        helpButton.setActionCommand("help");

   	pack();

    }

    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
        // OK
	if(cmd.equals("ok")) {
	    okAction();
        }
        // Cancel
        else if(cmd.equals("cancel")) {
	    cancelAction();
        }
        // Help
        else if(cmd.equals("help")) {
            displayHelp();
        }
    }

    private void cancelAction() {

        setVisible(false);
	
        m_parent.setLookAndFeel(m_defaultLookAndFeel);
    }

    private void okAction() {

        saveCurrentMenu();
        setVisible(false);
	
        m_parent.setLookAndFeel(m_defaultLookAndFeel);

	Vector paths = new Vector();
	paths.add(m_parent.getMenuPath());
        m_parent.updateFileMenu(paths, 0);
    }

    public void showDialog() {

	m_table.setModel(getCurrentMenu());
        setVisible(true);
	transferFocus();
    }

    public boolean lastRowEmpty() {
        int last = m_table.getRowCount()-1;
        if(last < 0) return false;
        for(int i=0; i<m_header.length; i++) {
            String path = (String)m_table.getValueAt(last, i);
            path.trim();
            if(path.length() > 0) return false;
        }

        return true;
    }

    private DefaultTableModel getCurrentMenu() {
	
	Vector colNames = new Vector();
	for(int i=0; i<m_header.length; i++)
	    colNames.add(m_header[i]);

	Vector entries = new Vector();

	if(m_path != null) {

	  BufferedReader in;
          String inLine;

          try {
            in = new BufferedReader(new FileReader(m_path));
            while ((inLine = in.readLine()) != null) {
                if (inLine.length() > 1 && !inLine.startsWith("#") &&
            !inLine.startsWith("%") && !inLine.startsWith("@")) {
                    inLine.trim();
                    StringTokenizer tok = new StringTokenizer(inLine, "|\n", false);
                    if (tok.countTokens() > 2) {
                        String path = tok.nextToken();
                        String label = tok.nextToken();
                        String type = tok.nextToken();
			Vector row = new Vector();
			row.add(path);
			row.add(label);
			row.add(type);
			entries.add(row);
            	    }
		}
            }
          } catch(IOException e) {
                //Messages.postError("ERROR: read " + m_path);
	  }
	} 

	if(entries.size() == 0) {

	  Vector row = new Vector();
	  row.add("");
	  row.add("");
	  row.add("");
	  entries.add(row);	
	}

	m_model = new DefaultTableModel(entries, colNames);
	return m_model;
    }

    private DefaultTableModel getDefaultMenu() {

	//remove menu file.

	if(m_path.indexOf("AuditMenu") != -1) {
	String[] cmd = {vnmr.admin.ui.WGlobal.SHTOOLCMD, "-c", "rm -f " + m_path};
	WUtil.runScript(cmd);
	}

	if(m_type.equals("records"))
		Audit.writeRecordAuditMenu();
	else if(m_type.equals("system"))
		Audit.writeSystemAuditMenu();

	return getCurrentMenu();
    }

    private void saveCurrentMenu() {

	if(m_path == null) return;

	FileWriter fw;
        PrintWriter os;
        try {
              fw = new FileWriter(m_path, false);
              os = new PrintWriter(fw);

	      for(int i=0; i<m_table.getRowCount(); i++) {
		String path = (String)m_table.getValueAt(i, 0); 
		String label = (String)m_table.getValueAt(i, 1); 
		String type = (String)m_table.getValueAt(i, 2); 

		if(label.length() > 0 && path.length() > 0
			&& type.length() > 0) {
		    String str = path +" | "+ label +" | "+ type;
		    os.println(str);
		}
	      }
	      os.close();
        } catch(Exception er) {
             //Messages.postDebug("Problem creating file, " + m_path);
             return;
        }
        return;
	
    }

    private void makeFileTypes() {
	m_filetypes = new Hashtable();
	
	m_filetypes.put(RECORDS, "records");
	m_filetypes.put(TRASH, "recordTrashInfo");
	m_filetypes.put(SESSIONS, "s_auditTrailFiles");
	m_filetypes.put(UAUDIT, "u_auditTrailFiles");
	m_filetypes.put(AAUDIT, "a_auditTrailFiles");

    }

  class RecordFileFilter extends FileFilter {
    
    // Accept all files and directories.
    public boolean accept(File f) {
            return true;
    }
    
    // The description of this filter
    public String getDescription() {
        return RECORDS;
    }
  }

  class TrashFileFilter extends FileFilter {
    
    // Accept all files and directories.
    public boolean accept(File f) {
            return true;
    }
    
    // The description of this filter
    public String getDescription() {
        return TRASH;
    }
  }

  class SessionFileFilter extends FileFilter {
    
    // Accept all files and directories.
    public boolean accept(File f) {
            return true;
    }
    
    // The description of this filter
    public String getDescription() {
        return SESSIONS;
    }
  }

  class UauditFileFilter extends FileFilter {
    
    // Accept all files and directories.
    public boolean accept(File f) {
            return true;
    }
    
    // The description of this filter
    public String getDescription() {
        return UAUDIT;
    }
  }

  class AauditFileFilter extends FileFilter {
    
    // Accept all files and directories.
    public boolean accept(File f) {
            return true;
    }
    
    // The description of this filter
    public String getDescription() {
        return AAUDIT;
    }
  }
/*
    public static void main(String[] args) {
        JFrame frame = new MenuEditor(this, "SystemAuditMenu");

        frame.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                System.exit(0);
            }
        });

        frame.pack();
        frame.setVisible(true);
    }
*/
}

