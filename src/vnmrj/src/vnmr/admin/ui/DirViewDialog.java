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
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  unascribed
 *
 */

public class DirViewDialog extends ModelessDialog implements ActionListener
{

    public static final int DFTABLE     = 10;
    public static final int FILESYSTEM  = 11;
    public static final int ALL         = 12;


    public DirViewDialog()
    {
        this(ALL);
    }

    public DirViewDialog(int nComp)
    {
        super(vnmr.util.Util.getLabel("_admin_View_Directories","View Directories"));
        layoutComps(nComp);
        registerActionListeners();
    }

    public void actionPerformed( ActionEvent e )
    {
        String cmd = e.getActionCommand();

        if( cmd.equals( "ok" ) || cmd.equals("cancel"))
            setVisible(false);
   }

    protected void layoutComps(int nComp)
    {
        DFFileTableModel model = new DFFileTableModel();
        TableSorter sorter = new TableSorter(model);
        JTable table = new JTable(sorter);
        sorter.addMouseListenerToHeaderInTable(table);
        JPanel pnlTable = new JPanel(new BorderLayout());
        pnlTable.add(table, BorderLayout.CENTER);
        pnlTable.add(table.getTableHeader(), BorderLayout.NORTH);

        WDirChooser filech = new WDirChooser(this);
        filech.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);

        JComponent compDisplay;

        if (nComp == DFTABLE)
        {
            compDisplay = pnlTable;
        }
        else if (nComp == FILESYSTEM)
        {
            compDisplay = filech;
        }
        else
        {
            JSplitPane pane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true, pnlTable, filech);
            compDisplay = new JScrollPane(pane);
        }

        getContentPane().add(compDisplay, BorderLayout.CENTER);

        setLocation( 200, 300 );
        int nWidth = compDisplay.getPreferredSize().width;
        int nHeight = compDisplay.getPreferredSize().height;
        setSize( nWidth, nHeight+100);
    }

    protected void registerActionListeners()
    {
        setAbandonEnabled(true);
        setCloseEnabled(true);
        
        closeButton.setActionCommand("ok");
        closeButton.addActionListener(this);
        abandonButton.setActionCommand("cancel");
        abandonButton.addActionListener(this);
    }
}



