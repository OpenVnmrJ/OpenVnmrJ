/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import vnmr.templates.*;
import vnmr.util.*;
import vnmr.bo.*;

// this is not used.
public class AcceleratorKeyEditor extends ModelessDialog implements ActionListener
{
    private static AcceleratorKeyEditor acceleratorKeyEditor = null;
    private AcceleratorKeyTable accTable = null;
    JTable table = null;
    TableSorter sorter = null;
    JTabbedPane tpane = null;
    JPanel editPane = null;
    JPanel prefPane = null;

    JTextField labelEntry = null;
    JTextField keyEntry = null;
    JTextField cmdEntry = null;

    JTextField kdelayEntry = null;
    JTextField qdelayEntry = null;

    String label = "";
    String keys = "";
    String cmd = "";

    public AcceleratorKeyEditor(String helpFile) {
        super("Accelerator Keys");
        m_strHelpFile = helpFile;
        accTable = AcceleratorKeyTable.getAcceleratorKeyTable();

        sorter = new TableSorter(accTable.getTableModel());
        table = new JTable(sorter);
        sorter.addMouseListenerToHeaderInTable(table);

        table.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                if(evt.getClickCount() == 2) {
                    Point p = new Point(evt.getX(), evt.getY());
                    int row = table.rowAtPoint(p);
                    if(row >= 0 && row < table.getRowCount()) {
                        String cmd = (String)table.getValueAt(row, 2);
                        System.out.println("cmd "+cmd);
                        if(cmd.indexOf("[") == -1) Util.sendToVnmr(cmd);
                    }
                }
            }

            public void mouseReleased(MouseEvent evt) {
                Point p = new Point(evt.getX(), evt.getY());
                int row = table.rowAtPoint(p);
                if(row >= 0 && row < table.getRowCount()) {
                    labelEntry.setText((String)table.getValueAt(row,0));
                    keyEntry.setText((String)table.getValueAt(row,1));
                    cmdEntry.setText((String)table.getValueAt(row,2));
                }
            }
        });

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent we) {
                closeAction();
            }
        });

        if(table.getRowCount() > 0) {
            table.setRowSelectionInterval(0,0);
            label = (String)table.getValueAt(0,0);
            keys = (String)table.getValueAt(0,1);
            cmd = (String)table.getValueAt(0,2);
        }
        table.setBackground(Util.getBgColor());

        JScrollPane spane = new JScrollPane(table, JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                                            JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
        spane.setPreferredSize(new Dimension(400,300));
        JPanel entryPane = new JPanel(new GridLayout(4, 1, 10, 5));

        JPanel pane = new JPanel();
        pane.setLayout(new BoxLayout(pane, BoxLayout.X_AXIS));
        pane.setBackground(Util.getBgColor());
        Font font = DisplayOptions.getFont("Dialog", Font.PLAIN, 12);
        JLabel llabel = new JLabel(accTable.LABEL + " ");
        llabel.setFont(font);
        pane.add(llabel);
        labelEntry = new JTextField(label);
        labelEntry.setPreferredSize(new Dimension(200,24));
        pane.add(labelEntry);
        entryPane.add(pane);

        pane = new JPanel();
        pane.setLayout(new BoxLayout(pane, BoxLayout.X_AXIS));
        pane.setBackground(Util.getBgColor());
        JLabel lkeys = new JLabel(accTable.KEYS + " ");
        lkeys.setFont(font);
        pane.add(lkeys);
        keyEntry = new JTextField(keys);
        keyEntry.setPreferredSize(new Dimension(200,24));
        pane.add(keyEntry);
        entryPane.add(pane);

        pane = new JPanel();
        pane.setLayout(new BoxLayout(pane, BoxLayout.X_AXIS));
        pane.setBackground(Util.getBgColor());
        JLabel lcmd = new JLabel(accTable.CMD + "  ");
        lcmd.setFont(font);
        pane.add(lcmd);
        cmdEntry = new JTextField(cmd);
        cmdEntry.setPreferredSize(new Dimension(200,24));
        pane.add(cmdEntry);
        entryPane.add(pane);

        pane = new JPanel();
        pane.setLayout(new BoxLayout(pane, BoxLayout.X_AXIS));
        pane.setBackground(Util.getBgColor());
        JButton deleteButton = new JButton("delete");
        deleteButton.addActionListener(new ActionListener() {
            public void  actionPerformed(ActionEvent e) {
                int row = table.getSelectedRow();
                if (row < 0 || row >= table.getRowCount())
                    return;
                String key = (String) table.getValueAt(row, 1);
                accTable.removeKeys(key);
                accTable.saveMenu();
                sorter = new TableSorter(accTable.getTableModel());
                //sorter.setModel(accTable.getTableModel());
                table.setModel(sorter);
                sorter.addMouseListenerToHeaderInTable(table);
                if (row > table.getRowCount() - 1)
                    row = table.getRowCount() - 1;
                if (row >= 0 && row < table.getRowCount()) {
                    table.setRowSelectionInterval(row, row);
                    labelEntry.setText( (String) table.getValueAt(row, 0));
                    keyEntry.setText( (String) table.getValueAt(row, 1));
                    cmdEntry.setText( (String) table.getValueAt(row, 2));
                }
            }
        });

        JButton saveButton = new JButton("save");
        saveButton.addActionListener(new ActionListener() {
            public void  actionPerformed(ActionEvent e) {
                String label = labelEntry.getText();
                String key = keyEntry.getText();
                String cmd = cmdEntry.getText();
                accTable.addKeys(label, key, cmd, "");
                accTable.saveMenu();
                sorter = new TableSorter(accTable.getTableModel());
                //sorter.setModel(accTable.getTableModel());
                table.setModel(sorter);
                sorter.addMouseListenerToHeaderInTable(table);
                int row = table.getRowCount()-1;
                table.setRowSelectionInterval(row,row);
            }
        });

        pane.add(Box.createRigidArea(new Dimension(30, 20)));
        pane.add(deleteButton);
        pane.add(saveButton);
        entryPane.add(pane);

        editPane = new JPanel();
        editPane.setLayout(new BoxLayout(editPane, BoxLayout.Y_AXIS));
        editPane.add(spane);
        editPane.add(Box.createRigidArea(new Dimension(20, 20)));
        editPane.add(entryPane);

        prefPane = new JPanel();
        prefPane.setLayout(new BoxLayout(prefPane, BoxLayout.Y_AXIS));

        pane = new JPanel();
        pane.setLayout(new BoxLayout(pane, BoxLayout.X_AXIS));
        pane.setBackground(Util.getBgColor());
        JLabel kdelay = new JLabel("maximum delay between key strokes (in sec): ");
        pane.add(kdelay);
        kdelayEntry = new JTextField(String.valueOf(0.001*accTable.getKeyDelay()));
        kdelayEntry.setMaximumSize(new Dimension(100,24));
        kdelayEntry.addActionListener(new ActionListener() {
            public void  actionPerformed(ActionEvent e) {
                double tm = Double.valueOf(kdelayEntry.getText()).doubleValue();
                accTable.setKeyDelay(tm);
            }
        });
        prefPane.add(Box.createRigidArea(new Dimension(20, 20)));
        pane.add(kdelayEntry);
        prefPane.add(pane);

        pane = new JPanel();
        pane.setLayout(new BoxLayout(pane, BoxLayout.X_AXIS));
        pane.setBackground(Util.getBgColor());
        JLabel qdelay = new JLabel("key strokes in queue expired (in sec): ");
        pane.add(qdelay);
        qdelayEntry = new JTextField(String.valueOf(0.001*accTable.getQueueDelay()));
        qdelayEntry.setMaximumSize(new Dimension(100,24));
        qdelayEntry.addActionListener(new ActionListener() {
            public void  actionPerformed(ActionEvent e) {
                double tm = Double.valueOf(qdelayEntry.getText()).doubleValue();
                accTable.setQueueDelay(tm);
            }
        });
        prefPane.add(Box.createRigidArea(new Dimension(20, 20)));
        pane.add(qdelayEntry);
        prefPane.add(pane);

        tpane = new JTabbedPane();

        tpane.addTab("Editor", null, editPane,"");
        tpane.addTab("Preference", null, prefPane,"");
        tpane.setSelectedIndex(0);

        getContentPane().add(tpane, BorderLayout.NORTH);

        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);

    	setPanelColor();
        pack();
    }

    public static void showAcceleratorKeyEditor(String helpFile) {
        if(acceleratorKeyEditor == null) {
            acceleratorKeyEditor = new AcceleratorKeyEditor(helpFile);
        }

        acceleratorKeyEditor.setVisible(true);
        acceleratorKeyEditor.setState(Frame.NORMAL);
        acceleratorKeyEditor.repaint();
    }

    private void closeAction() {
        double tm = Double.valueOf(kdelayEntry.getText()).doubleValue();
        accTable.setKeyDelay(tm);
        tm = Double.valueOf(qdelayEntry.getText()).doubleValue();
        accTable.setQueueDelay(tm);
	accTable.saveMenu();
    }

    public void actionPerformed(ActionEvent e) {
        String str = e.getActionCommand();
        if(str.equals("close")) {
            double tm = Double.valueOf(kdelayEntry.getText()).doubleValue();
            accTable.setKeyDelay(tm);
            tm = Double.valueOf(qdelayEntry.getText()).doubleValue();
            accTable.setQueueDelay(tm);
	    accTable.saveMenu();
            setVisible(false);
        }
        else if (str.equals("help"))
            displayHelp();
    }

    public void setPanelColor(){
	Color color = Util.getBgColor();
	tpane.setBackground(color);
	editPane.setBackground(color);
	for (int i=0; i<editPane.getComponentCount(); i++) {
            editPane.getComponent(i).setBackground(color);
	}
	prefPane.setBackground(color);
	for (int i=0; i<prefPane.getComponentCount(); i++) {
            prefPane.getComponent(i).setBackground(color);
	}
	setBackground(color);
	labelEntry.setBackground(color);
	keyEntry.setBackground(color);
	cmdEntry.setBackground(color);
	kdelayEntry.setBackground(color);
	qdelayEntry.setBackground(color);

	table.setSelectionBackground(Global.HIGHLIGHTCOLOR);
        table.setSelectionForeground(table.getForeground());
    }
}
