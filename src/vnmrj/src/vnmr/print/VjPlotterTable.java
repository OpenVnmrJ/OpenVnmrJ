/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import java.awt.*;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.event.MouseEvent;
import java.io.*;
import java.util.*;
import javax.swing.*;
import javax.swing.table.*;
import javax.swing.event.*;
import javax.swing.border.*;

import vnmr.admin.ui.*;
import vnmr.admin.util.*;
import vnmr.util.*;

public class VjPlotterTable extends JPanel implements ActionListener {
    private boolean bAdmin;
    private boolean dummyLine;
    private boolean bChanged;
    private boolean bViewOnly;
    private JTable  table;
    private java.util.List <VjPlotterObj> origDeviceList;
    private java.util.List <VjPlotterObj> deviceList;
    private TableRowSorter<DefaultTableModel> tableSorter;
    private JButton   addBut, deleteBut, resetBut;
    private JComboBox printerCb, typeCb;
    private ButtonGroup plotterGrp;
    private ButtonGroup printerGrp;
    private TableColumn printerColumn;
    private TableColumn layoutColumn;
    private DefaultTableCellRenderer cbRender;
    private VjPlotterConfig configPan;
    private VjPlotterType layoutHandler;
    private DefaultTableModel tableModel;
    private ActionListener radioListener;
    private VjPlotterObj defaultPlotter = null;
    private PrintWriter fout;
    private Class<?>[] classes;
    private int VNMR_PLOTTER;
    private int VNMR_PRINTER;
    private int VNMR_DEVICE;
    private int OS_PRINTER;
    private int LAYOUT;
    private int SYSTEM_USER;
    private int new_id;
    private long userFileDate;
    private long sysFileDate;
    private String userFilePath;
    private String sysFilePath;

    // private final String[] adminTitles = { "Vnmr Device Name",
    //                    "Linux Printer Name", "Page Type" };
    private final String[] adminTitles = { "VNMR Device Name",
                        "OS Printer Name" };
    private final String[] adminTitleNames = { "_VNMR_Device_Name",
                        "_OS_Printer_Name" };
    private final Class<?>[] adminClasses = { String.class,
                         String.class, String.class, String.class };

    // private final String[] userTitles = { "On", "Vnmr Plotter Name",
    //                     "Linux Printer Name", "Page Type", "System/User" };
    /**********
    private final String[] userTitles = { "Plotter","Printer", "VNMR Device Name",
                      "OS Printer Name", "System/User" };
    private final String[] userTitleNames = { "_Plotter","_Printer", "_VNMR_Device_Name",
                      "_OS_Printer_Name", "_System/User" };
    private final Class<?>[] userClasses = { JRadioButton.class, JRadioButton.class,
                       String.class, String.class, String.class, String.class };
    **********/
    private final String[] userTitles = { "Printer", "VNMR Device Name",
                      "OS Printer Name", "System/User" };
    private final String[] userTitleNames = { "_Printer", "_VNMR_Device_Name",
                      "_OS_Printer_Name", "_System/User" };
    private final Class<?>[] userClasses = { JRadioButton.class,
                       String.class, String.class, String.class, String.class };

    private static String addCmd = "add new";
    private static String deleteCmd = "delete";
    private static String resetCmd = "reset";
    private static String UNKNOWN = "unknown";
    private static String sysFile =
                           FileUtil.sysdir()+File.separator+"devicenames";
    private static String userFile =
                           FileUtil.usrdir()+File.separator+".devicenames";
    public static String tmpStr = "xxtmp_layout";
    public static String appendStr = "_layout";
    public static String noneDevice = "none";
    private String[] titles = adminTitles;
    private String[] groupLabels;
    

    public VjPlotterTable(boolean admin, boolean viewOnly, VjPlotterConfig cf) {
        this.bAdmin = admin;
        this.bViewOnly = viewOnly;
        setLayout(new BorderLayout());
        this.configPan = cf;

        radioListener = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                radioAction(e);
            }
        };

        groupLabels = new String[2];
        groupLabels[0] = Util.getLabel("_System", "System");
        groupLabels[1] = Util.getLabel("_User", "User");
 
        checkFiles();
        buildList();
        createTable();
        createCtrlPanel();
        setupPlotterTable();
        this.bChanged = false;
    }

    public VjPlotterTable(boolean admin, VjPlotterConfig cf) {
        this(admin, false, cf);
    }

    public void rebuild(boolean bNewLayout) {
        boolean bNewFile = checkFiles();
        if (!bNewLayout && !bNewFile) {
            if (defaultPlotter != null) {
                if (defaultPlotter.printerName.equals(UNKNOWN)) {
                   String name = VjPrintUtil.getDefaultPrinterName();
                   if (name != null)
                       defaultPlotter.printerName = name;
                }
            }
            return;
        }
        if (tableModel != null)
            tableModel.setNumRows(0);
        buildList();
        setupPlotterTable();
    }

    private boolean checkFiles() {
        boolean bNew = false;
        File fd;

        if (!bAdmin && !bViewOnly) {
            userFilePath = FileUtil.openPath(userFile);
            if (userFilePath != null) {
                fd = new File(userFilePath);
                if (fd.exists()) { 
                   if (fd.lastModified() != userFileDate) {
                       userFileDate = fd.lastModified();
                       bNew = true;
                   }
                }
                else
                   userFilePath = null;
             }
        }
        else
            userFilePath = null;
        sysFilePath = FileUtil.openPath(sysFile);
        if (sysFilePath != null) {
            fd = new File(sysFilePath);
            if (fd.exists()) {
               if (fd.lastModified() != sysFileDate) {
                  sysFileDate = fd.lastModified();
                  bNew = true;
               }
             }
         } 
         return bNew;
    }

    private void buildList() {
        if (deviceList == null)
            deviceList = new ArrayList<VjPlotterObj>(40);
        else
            deviceList.clear();

        plotterGrp = new ButtonGroup();
        printerGrp = new ButtonGroup();
        if (!bAdmin) {
            addNullDevice();
        }

        if (sysFilePath != null)
            addDeviceNames(sysFilePath, true, bAdmin);

        int n, k;
        boolean bTest = false;
        new_id = 0;
        if (bAdmin) {
            k = adminTitles.length;
            titles = new String[k];
            for (n = 0; n < k; n++)
                titles[n] = Util.getLabel(adminTitleNames[n], adminTitles[n]);
                
            // titles = adminTitles;
            classes = adminClasses;
            VNMR_DEVICE = 0;
            OS_PRINTER = 1;
            SYSTEM_USER = 2;
            LAYOUT = 97;
            VNMR_PLOTTER = 99;
            VNMR_PRINTER = 98;
        }
        else {
            if (!bViewOnly) {
                if (userFilePath != null)
                     addDeviceNames(userFilePath, false, true);
            }
            k = userTitles.length;
            titles = new String[k];
            for (n = 0; n < k; n++)
                titles[n] = Util.getLabel(userTitleNames[n], userTitles[n]);
            classes = userClasses;
            VNMR_PLOTTER = 0;
            VNMR_DEVICE = 1;
            OS_PRINTER = 2;
            SYSTEM_USER = 3;

            /*****
            VNMR_PRINTER = 1;
            VNMR_DEVICE = 2;
            OS_PRINTER = 3;
            SYSTEM_USER = 4;
            *******/

            VNMR_PRINTER = 98;
            LAYOUT = 97;
        }
 
        printerCb = new JComboBox();
        String[] psnames = VjPrintUtil.lookupPrintNames();
        if (psnames != null) {
            for (n = 0; n < psnames.length; n++) {
                if (psnames[n] != null)
                    printerCb.addItem(psnames[n]);
            }
        }

        typeCb = new JComboBox();

        if (configPan != null) {
            layoutHandler = configPan.getLayoutHandler();
            bTest = configPan.isTest();
        }
        Iterator<VjPlotterObj> itr;
        VjPlotterObj obj;
        if (layoutHandler != null) {
            layoutHandler.linkPageLayouts(deviceList, false);
            java.util.List <VjPlotterObj> newList =
                     new ArrayList<VjPlotterObj>(deviceList.size());
            itr = deviceList.iterator();
            while (itr.hasNext()) {
                obj = itr.next();
                if (bTest)
                    System.out.print(" device: "+obj.deviceName);
                // if (obj.deviceName.equals(noneDevice))
                if (obj.bExtra)  // none or '' 
                {
                    newList.add(obj);
                    if (obj.getAttributeSet() == null)
                        obj.setAttributeSet(layoutHandler.createPageAttributes(""));
                }
                else {
                    if (obj.getAttributeSet() != null)
                        newList.add(obj);
                    else {
                        if (bTest)
                            System.out.print(" removed ");
                        bChanged = true;
                    }
                }
                if (bTest)
                    System.out.println(" ");
            }
            deviceList = newList;
        }

        itr = deviceList.iterator();
        while (itr.hasNext()) {
            obj = itr.next();
            // if (obj.printerName == null || obj.printerName.length() < 1) {
            //      if (!obj.deviceName.equals(noneDevice))
            //         obj.printerName = obj.deviceName;
            // }
            if (obj.plotterCheck != null) {
                plotterGrp.add(obj.plotterCheck);
                obj.plotterCheck.addActionListener(radioListener);
            }
            if (obj.printerCheck != null) {
                printerGrp.add(obj.printerCheck);
                obj.printerCheck.addActionListener(radioListener);
            }
        }

        plotterGrp.clearSelection();
        printerGrp.clearSelection();
        origDeviceList = new ArrayList<VjPlotterObj>(deviceList);
    }

    public void setLayoutHandler(VjPlotterType types) {
        if (types == null || (types == layoutHandler)) {
           layoutHandler = types;
           return;
        }
        layoutHandler = types;
        layoutHandler.linkPageLayouts(deviceList, false);
        setLayoutList(layoutHandler.getLayoutList());
    }

    public void setLayoutList(String[] list) {
        if (list == null)
            typeCb = new JComboBox();
        else
            typeCb = new JComboBox(list);

        typeCb.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                changeType();
            }
        });

        if (layoutColumn != null)
            layoutColumn.setCellEditor(new DefaultCellEditor(typeCb));
    }


    private void radioAction(ActionEvent e) {
         table.repaint();
    }

    private void setupPlotterTable() {
        if (table == null)
            return;
        if (tableModel != null)
            tableModel.setNumRows(deviceList.size());
        if (cbRender != null)
            return;
        cbRender = new DefaultTableCellRenderer() {
            public void setValue(Object value) {
                super.setValue(value);
            }
        };
        int k = titles.length;
        TableColumnModel cm = table.getColumnModel();

        // printerColumn = table.getColumn(titles[OS_PRINTER]);
        printerColumn = cm.getColumn(OS_PRINTER);
        if (LAYOUT < k)
            layoutColumn = cm.getColumn(LAYOUT);
        if (!bAdmin) {
            // TableColumn radioColumn = table.getColumn(titles[VNMR_PLOTTER]);
            TableColumn radioColumn = cm.getColumn(VNMR_PLOTTER);
            radioColumn.setCellRenderer(new RadioButtonRenderer());
            radioColumn.setCellEditor(new RadioButtonEditor(new JCheckBox()));
            radioColumn.setPreferredWidth(64);
            radioColumn.setMaxWidth(90);
            radioColumn.setMinWidth(12);

            // radioColumn = table.getColumn(titles[VNMR_PRINTER]);
            if (VNMR_PRINTER < k) {
              radioColumn = cm.getColumn(VNMR_PRINTER);
              radioColumn.setCellRenderer(new RadioButtonRenderer());
              radioColumn.setCellEditor(new RadioButtonEditor(new JCheckBox()));
              radioColumn.setPreferredWidth(64);
              radioColumn.setMaxWidth(90);
              radioColumn.setMinWidth(12);
            }
 
            // TableColumn systemColumn = table.getColumn(titles[SYSTEM_USER]);
            TableColumn systemColumn = cm.getColumn(SYSTEM_USER);
            systemColumn.setPreferredWidth(82);
            systemColumn.setMaxWidth(86);
            systemColumn.setMinWidth(12);
            if (bViewOnly)
               table.removeColumn(systemColumn);
        }

        printerColumn.setCellEditor(new DefaultCellEditor(printerCb));
        printerColumn.setCellRenderer(cbRender);
        printerColumn.setPreferredWidth(32);

        if (layoutColumn != null) {
           layoutColumn.setCellEditor(new DefaultCellEditor(typeCb));
           layoutColumn.setCellRenderer(cbRender);
           layoutColumn.setPreferredWidth(32);
        }
    }

    private void createTableModel() {
        tableModel = new DefaultTableModel() {
            public int getColumnCount() {
                 return titles.length;
            }

            public int getRowCount() {
                 return super.getRowCount();
            }

            public Object getValueAt(int row, int col) {
                 VjPlotterObj obj = null;

                 try {
                    obj = deviceList.get(row);
                 }
                 catch (IndexOutOfBoundsException e) { }

                 if (obj == null)
                     return null;
                 
                 if (col == VNMR_PLOTTER)
                     return obj.plotterCheck;
                 if (col == VNMR_PRINTER)
                     return obj.printerCheck;
                 if (col == VNMR_DEVICE)
                     return obj.deviceName;
                 if (col == OS_PRINTER)
                     return obj.printerName;
                 if (col == LAYOUT)
                     return obj.type;
                 if (col == SYSTEM_USER) {
                     if (obj.bSystem) {
                        return (groupLabels[0]);
                        //  return groups[0]; // system
                     }
                     else {
                        return (groupLabels[1]);
                        // return groups[1]; // user
                     }
                 }
                 return null;
            }

            public String getColumnName(int column) {
                 return titles[column];
            }

            public Class<?> getColumnClass(int c) {
                 return classes[c];
            }

            public boolean isCellEditable(int row, int col) {
                 if (bAdmin) {
                     return col != SYSTEM_USER; // System/User
                 }
                 VjPlotterObj obj = null;
                 try {
                     obj = deviceList.get(row);
                 }
                 catch (IndexOutOfBoundsException e) {
                     return false;
                 }
                 if (obj == null)
                     return false;
                 if (col == VNMR_PLOTTER || col == VNMR_PRINTER)
                     return true;
                 if (col == SYSTEM_USER)
                     return false;
                 return obj.bChangeable;
            }

            public void setValueAt(Object aValue, int row, int column) {
                 VjPlotterObj obj = null;
                 try {
                     obj = deviceList.get(row);
                 }
                 catch (IndexOutOfBoundsException e) {
                     return;
                 }
                 if (obj == null)
                     return;
                 if (column == VNMR_PLOTTER || column == VNMR_PRINTER)
                     return;
                 String s = null;
                 if (aValue instanceof JRadioButton) {
                     // rb = (JRadioButton) aValue;
                     return;
                 }
                 if (aValue instanceof String)
                     s = (String) aValue;
                 if (s == null)
                     return;
                 bChanged = true;   
                 if (column == VNMR_DEVICE) {
                     obj.deviceName = s.trim();
                     return;
                 }
                 if (column == OS_PRINTER) {
                     obj.printerName = s;
                     return;
                 }
                 if (column == LAYOUT) {
                     obj.type = s;
                     return;
                 }
                 /***
                 if (column == SYSTEM_USER) {
                     if (s.equals(groups[0]))
                         obj.bSystem = true;
                     else
                         obj.bSystem = false;
                     return;
                 }
                 ***/
            }
        };
    }

    private void createTable() {
        createTableModel();
        table = new VjJTable(tableModel);
        tableSorter = new TableRowSorter<DefaultTableModel>(tableModel);
        table.setRowSorter(tableSorter);
        table.setRowHeight(28);
        table.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        table.getSelectionModel().addListSelectionListener(
               new RowSelectionHandler());
       // table.getModel().addTableModelListener(new ColumnChangedHandler());

        JScrollPane scrollpane = new JScrollPane(table);
        add(scrollpane, BorderLayout.CENTER);
    }

    private void createCtrlPanel() {
        JPanel ctrlPan = new JPanel();
        ctrlPan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        ctrlPan.setLayout(new SimpleH2Layout(SimpleH2Layout.CENTER, 12, 5, false));
        String lblStr = Util.getLabel("_Add_new_device","Add new device");
        addBut = new JButton(lblStr);
        addBut.setActionCommand(addCmd);
        addBut.addActionListener(this);
        lblStr = Util.getLabel("_Delete_selected", "Delete selected");
        deleteBut = new JButton(lblStr);
        deleteBut.setActionCommand(deleteCmd);
        deleteBut.addActionListener(this);
        lblStr = Util.getLabel("_Reset_to_origin", "Reset to origin");
        resetBut = new JButton(lblStr);
        resetBut.setActionCommand(resetCmd);
        resetBut.addActionListener(this);
        ctrlPan.add(addBut);
        ctrlPan.add(deleteBut);
        // ctrlPan.add(resetBut);
        if (!bViewOnly) {
            add(ctrlPan, BorderLayout.SOUTH);
        }
    }

    private VjPlotterObj getItemAt(int row) {
        int n =  deviceList.size();
        if (n < 1 || row < 0)
            return null;

        String plotter, printer;
        plotter = (String)table.getValueAt(row, VNMR_DEVICE);
        printer = (String)table.getValueAt(row, OS_PRINTER);
        // type = (String)table.getValueAt(row, LAYOUT);

        VjPlotterObj retObj = null;
        int s = tableSorter.convertRowIndexToModel(row);
        if (s >= 0 && s < n)
            retObj = deviceList.get(s);
        if (plotter == null || printer == null)
            return retObj;
        if (retObj != null) {
            if (retObj.deviceName.equals(plotter)) {
               if (retObj.printerName.equals(printer))
                   return retObj;
            }
        }

        for (int k = 0; k < n; k++) {
            VjPlotterObj obj = deviceList.get(k);
            if (obj != null && obj.deviceName.equals(plotter)) {
                if (obj.printerName.equals(printer)) {
                     if (obj.bChangeable)
                         return obj;
                     if (retObj == null)
                         retObj = obj;
                }
            }
        }
        return retObj;
    }

    private boolean removePlotter(int row) {
        VjPlotterObj obj = getItemAt(row);
        if (obj == null || !obj.bChangeable)
            return false;
        layoutHandler.unLinkPageLayout(obj); 
        obj.dispose();
        return deviceList.remove(obj);
    }


    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
        int row, n;
        if (cmd.equals(addCmd)) {
            VjPlotterObj vobj = new VjPlotterObj("", bAdmin, bAdmin);
            vobj.bNewSet = true;
            vobj.bChangeable = true;
            bChanged = true;
            if (printerCb != null && printerCb.getItemCount() > 0)
                vobj.printerName = (String) printerCb.getItemAt(0);
            else
                vobj.printerName = "";
            new_id++;
            vobj.type = tmpStr+new_id;
            vobj.setAttributeSet(layoutHandler.createPageAttributes(vobj.type));
           /***
            if (typeCb != null && typeCb.getItemCount() > 0)
                vobj.type = (String) typeCb.getItemAt(0);
            else
                vobj.type = "PS_AR";
            ***/
            vobj.shared = "Yes";
            vobj.use = "Both";
            deviceList.add(vobj);
            if (vobj.plotterCheck != null) {
                plotterGrp.add(vobj.plotterCheck);
                vobj.plotterCheck.addActionListener(radioListener);
            }
            if (vobj.printerCheck != null) {
                printerGrp.add(vobj.printerCheck);
                vobj.printerCheck.addActionListener(radioListener);
            }
            row = tableModel.getRowCount();
            tableModel.setNumRows(row + 1);
            table.revalidate();
            n = tableSorter.convertRowIndexToView(row);
            if (n >= 0 && n <= row) {
               Rectangle r = table.getCellRect(n, 0, false);
               table.scrollRectToVisible(r);
               table.setRowSelectionInterval(n, n);
               table.editCellAt(n, VNMR_DEVICE);
               Component comp = table.getEditorComponent();
               if (comp != null)
                   comp.requestFocus();
            }

            return;
        }
        if (cmd.equals(deleteCmd)) {
            n = table.getSelectedRow();
            if (n < 0)
                return;
            if (removePlotter(n)) {
                bChanged = true;
                tableModel.removeRow(n);
                row = tableModel.getRowCount();
                if (row > 1) {
                  //   tableSorter.rowsUpdated(0, row -1);
                }
                if (n < row)
                    table.setRowSelectionInterval(n, n);
            }
            return;
        }
        if (cmd.equals(resetCmd)) {
            deviceList = new ArrayList<VjPlotterObj>(origDeviceList);
            tableModel.setNumRows(deviceList.size());
            table.revalidate();
            bChanged = false;
            return;
        }
    }

    public void setVnmrPlotter(String name) {
         if (name == null)
             return;
         name = name.trim();
         VjPlotterObj obj;
         int k = deviceList.size();
         int n = 0;
         for (n = 0; n < k; n++) {
             obj = deviceList.get(n);
             if (name.equals(obj.deviceName)) {
                 obj.setPlotterSelected(true);
                 break;
             }
         }
         if (n < k) {
             if (table.getSelectedRow() < 0)
                 table.setRowSelectionInterval(n, n);
         }
         table.repaint();
    }

    public void setVnmrPrinter(String name) {
         if (name == null)
             return;
         name = name.trim();
         VjPlotterObj obj = null;
         for (int n = 0; n < deviceList.size(); n++) {
             obj = deviceList.get(n);
             if (name.equals(obj.deviceName)) {
                 obj.setPrinterSelected(true);
                 break;
             }
         }
         table.repaint();
    }
    
    public String getSelectedPageLayout() {
        int row = table.getSelectedRow();
        if (row < 0)
            return null;
        VjPlotterObj obj = getItemAt(row);
        if (obj == null)
            return null;
        return obj.type;
    }

    private void setPageLayout() {
        if (!isVisible())
            return;
        int row = table.getSelectedRow();
        if (row < 0)
            return;
        VjPlotterObj obj = getItemAt(row);
        if (obj == null)
            return;
        configPan.setPlotterDevice(obj);
        /***
        if (obj.getAttributeSet() != null)
            configPan.setPageAttribute(obj.getAttributeSet());
        else
            configPan.setPageLayout(obj.type);
        ***/
    }

    public void setVisible(boolean s) {
        updateEditing(); 
        super.setVisible(s);
        if (!s)
            return;
        setPageLayout();
    }

    private void addToList(String name, VjPlotterObj newObj) {
         int num = deviceList.size();

         VjPlotterObj obj;
         for (int n = 0; n < num; n++) {
             obj = deviceList.get(n);
             if (name.compareToIgnoreCase(obj.deviceName) < 0) {
                 deviceList.add(n, newObj);
                 return;
             }
             if (name.equals(obj.deviceName))
                 return;
         }
         deviceList.add(newObj);
    }
     
    private void addDeviceNames(String fpath, boolean bSystem, boolean bWritable) {
        String data, type, value;
        StringTokenizer tok;
        BufferedReader fin = null;
        VjPlotterObj pObj = null;

        if (fpath == null)
           return;
        try {
           fin = new BufferedReader(new FileReader(fpath));
           while ((data = fin.readLine()) != null) {
              if (data.startsWith("#"))
                  continue;
              tok = new StringTokenizer(data, " \t\r\n");
              if (tok.countTokens() < 2)
                  continue;
              type = tok.nextToken();
              value = tok.nextToken("\t\r\n");
              value=value.trim();
              if (type.equals("Name")) {
                  value = value.trim();
                  if (value.length() > 0) {
                     pObj = new VjPlotterObj(value, bSystem, bAdmin);
                     addToList(value, pObj);
                     pObj.bChangeable = bWritable;
                  }
                  continue;
              }
              if (pObj == null)
                  continue;
              if (type.equals("Use")) {
                  pObj.use = value;
                  continue;
              }
              if (type.equals("Type")) {
                  pObj.type = value;
                  continue;
              }
              if (type.equals("Host")) {
                  pObj.host = value;
                  continue;
              }
              if (type.equals("Port")) {
                  pObj.port = value;
                  continue;
              }
              if (type.equals("Baud")) {
                  pObj.baud = value;
                  continue;
              }
              if (type.equals("Shared")) {
                  pObj.shared = value;
                  continue;
              }
              if (type.equals("Printer")) {
                  pObj.printerName = value;
                  continue;
              }
           }
        }
        catch(IOException e)
        {
        }
        
        finally {
           try {
               if (fin != null) 
                   fin.close();
           }
           catch(IOException e) {}
        }

    }

    private void addNullDevice() {
        if (bAdmin)
            return;
        VjPlotterObj pObj =  new VjPlotterObj(noneDevice, true, false);
        addToList(noneDevice, pObj);
        pObj.bChangeable = false;
        pObj.bExtra = true;
        // String pname = VjPrintUtil.getDefaultPrinterName();
        String pname = VjPrintUtil.getSaveDefaultPrinterName();
        if (pname == null)
            pname = UNKNOWN;
        defaultPlotter =  new VjPlotterObj("", true, false);
        addToList("", defaultPlotter);
        defaultPlotter.bChangeable = false;
        defaultPlotter.printerName = pname;
        defaultPlotter.bExtra = true;
    }

    private void updateEditing() {
        if (!isVisible())
            return;
        TableCellEditor editor = table.getCellEditor();
        if (editor != null) {
            editor.stopCellEditing();
        }
    }


    private void changeType() {
        setPageLayout();
    }

    private class RowSelectionHandler implements ListSelectionListener {
         public void valueChanged(ListSelectionEvent e) {
            if (e.getSource() == table.getSelectionModel()
                         && table.getRowSelectionAllowed()) {
                int row = table.getSelectedRow();
                VjPlotterObj obj = getItemAt(row);
                if (obj != null && (obj.bChangeable))
                    deleteBut.setEnabled(true);
                else
                    deleteBut.setEnabled(false);
                if (obj != null)
                    setPageLayout();
            } 
         }
    }

    private void writeDummyLine() {
        if (dummyLine)
           return;
        fout.println("################################################");
        dummyLine = true;
    }

    private void writeObj(String name, String value) {
        fout.print(name);
        int n;
        if (value != null) {
            n = name.length();
            while (n < 9) {
                fout.print(" ");
                n++;
            }
            fout.print(value);
        }
        fout.println("");
    }

    private boolean saveObjToFile(int n, VjPlotterObj obj) {
        boolean bGoodItem = true;
        String s = obj.deviceName;
        String s2 = obj.printerName;
        if (s == null || s2 == null)
            bGoodItem = false;
        else {
            s = obj.deviceName.trim();
            s2 = obj.printerName.trim();
            if (s.length() < 1) {
                s = null;
                bGoodItem = false;
            }
            if (s2.length() < 1) {
                s2 = null;
                bGoodItem = false;
            }
        }
        if (!bGoodItem) {
            if (!isVisible())
               VjPrintUtil.showConfigTab(this);
            int row = tableSorter.convertRowIndexToView(n);
            Rectangle r = table.getCellRect(row, 0, false);
            table.scrollRectToVisible(r);
            table.setRowSelectionInterval(row, row);
            if (s == null)
               JOptionPane.showMessageDialog(null,
                       "Plotter name is empty.",
                       "Save devicenames", JOptionPane.ERROR_MESSAGE);
            else
               JOptionPane.showMessageDialog(null,
                       "Printer is null.",
                       "Save devicenames", JOptionPane.ERROR_MESSAGE);
            return bGoodItem;
        }
        writeDummyLine();
        writeObj("Name", s);
        writeObj("Printer", s2);
        writeObj("Use", obj.use);
        writeObj("Type", obj.type);
        if (obj.new_host != null)
           writeObj("Host", obj.new_host);
        else
           writeObj("Host", obj.host);
        if (obj.new_port != null)
           writeObj("Port", obj.new_port);
        else
           writeObj("Port", obj.port);
        writeObj("Baud", obj.baud);
        writeObj("Shared", obj.shared);
        dummyLine = false;
        return bGoodItem;
    }

    private void setLayoutCount() {
        if (layoutHandler == null)
           return;

        layoutHandler.clearLayoutLinkCount();
        VjPageAttributes layout;
        int i; 
        VjPlotterObj obj;
        for (i = 0; i < deviceList.size(); i++) {
            obj = deviceList.get(i);
            layout = obj.getAttributeSet();
            if (layout != null) {
                if (!obj.bRemoved) {
                    layout.increaseLinkCount();
                    if (layout.bChangeable) {
                        if (layout.typeName.startsWith(tmpStr)) {
                            obj.type = obj.deviceName+appendStr;
                            layout.typeName = obj.type;
                        }
                    }
                }
            }
            if (obj.bChangeable) {
                if (obj.bChanged)
                    bChanged = true;
                if (obj.bNewSet)
                    bChanged = true;
            }
        }
    }

    private boolean saveFile() {
        String fileName;
        String inPath = null;
        String outPath = null;
        boolean bTest = false;
        
        if (bViewOnly)
            return true;
        updateEditing(); 
        setLayoutCount();
        if (!bChanged)
            return true;

        if (configPan != null)
            bTest = configPan.isTest();

        if (bAdmin) {
            if (bTest)
                System.out.println(" save devicenames to tmpdir ");
            fileName = sysFile;
            if (bTest)
                inPath = FileUtil.openPath(sysFile);
            else
                inPath = FileUtil.savePath(sysFile, false);
        }
        else {
            if (bTest)
                System.out.println(" save devicenames to userdir ");
            fileName = userFile;
            inPath = FileUtil.savePath(userFile, false);
        }
        if (inPath == null) {
            JOptionPane.showMessageDialog(null,
                    "Could not open file "+fileName+".\n Permission denined.",
                    "Save devicenames", JOptionPane.ERROR_MESSAGE );
            return false;
        }

        if (bAdmin) {
            if (bTest)
                fileName = "/tmp/devicenames";
            else
                fileName = sysFile+".tmp";
            outPath = FileUtil.savePath(fileName, false);
        }
        else {
            fileName = userFile+".tmp";
            outPath = FileUtil.savePath(fileName, false);
        }
        if (outPath == null) {
            JOptionPane.showMessageDialog(null,
                    "Could not open file "+fileName+".\n Permission denined.",
                    "Save devicenames", JOptionPane.ERROR_MESSAGE );

            return false;
        }

        BufferedReader fin = null;
        dummyLine = false;
        fout = null;
        String inData;

        try {
            fout = new PrintWriter( new OutputStreamWriter(
                   new FileOutputStream(outPath), "UTF-8"));
        }
        catch(IOException er1) { return false; }

        try {
             fin = new BufferedReader(new FileReader(inPath));
             while ((inData = fin.readLine()) != null) {
                   if (inData.startsWith("#")) {
                       if (inData.startsWith("####")) {
                           if (!dummyLine) {
                                fout.println(inData);
                                dummyLine = true;
                           }
                       }
                       else {
                           fout.println(inData);
                           dummyLine = false;
                       }
                   }
             }
        }
        catch(IOException er2) { }

        VjPlotterObj obj = null;
        boolean bGoodItem = true;
        for (int i = 0; i < deviceList.size(); i++) {
            obj = deviceList.get(i);
            if (bAdmin) {
                 if (!obj.bSystem)
                     obj = null;
            }
            else {
                 if (obj.bSystem)
                     obj = null;
            }
            if (obj != null) {
                 if (!obj.bRemoved) {
                     bGoodItem = saveObjToFile(i, obj);
                     if (!bGoodItem)
                         break;
                 }
            }
        }
        writeDummyLine();

        try {
            fout.close();
            if (fin != null)
                fin.close();
        }
        catch(IOException e) { bGoodItem = false; }

        if (!bGoodItem) {
            File f = new File(outPath);
            f.delete();
            return false;
        }
/***
        if (bAdmin)
            fileName = sysFile+".bak";
        else
            fileName = userFile+".bak";
        String bakPath = FileUtil.savePath(fileName, false);
        if (bakPath != null) {
            String [] cmds1 = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
                    "/bin/mv " +"\""+inPath + "\" \"" + bakPath + "\""};
            WUtil.runScript(cmds1);
        }
***/
        outPath = UtilB.windowsPathToUnix(outPath);
        inPath = UtilB.windowsPathToUnix(inPath);

        String [] cmds2 = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION,
                    "/bin/mv " +"\""+outPath + "\" \"" + inPath + "\""};
        if (!bTest)
            WUtil.runScript(cmds2);
        return true;
    }

    public boolean saveToFile() {
        boolean bSuccess = true;
        if (!bViewOnly) {
            bSuccess = saveFile();
            if (bSuccess) {
                if (layoutHandler != null)
                    bSuccess = layoutHandler.saveToFile();
            }
        }
        if (bSuccess)
            bChanged = false;
        if (bAdmin || !bSuccess) {
           return bSuccess;
        }

        String plotter = null;
        String printer = null;
        for (int i = 0; i < deviceList.size(); i++) {
            VjPlotterObj obj = deviceList.get(i);
            if (obj.isPlotterSelected())
                plotter = obj.deviceName;
            if (obj.isPrinterSelected())
                printer = obj.deviceName;
        }
        printer = plotter;
        if (plotter == null && printer == null)
            return bSuccess;
        StringBuffer sb = new StringBuffer();
        if (printer != null)
            sb.append("printer='").append(printer.trim()).append("'");
        if (plotter != null) {
            sb.append(" plotter='").append(plotter.trim()).append("'");
            sb.append(" exists('sysplotter','parameter','global'):$exx ");
            sb.append(" if $exx>0.5 then sysplotter='");
            sb.append(plotter.trim()).append("' endif");
        }
        Util.sendToActiveView(sb.toString());
        return bSuccess;
    }

    /****
    private class ColumnChangedHandler implements TableModelListener {
         public void tableChanged(TableModelEvent e) {
            switch (e.getType()) {
              case TableModelEvent.INSERT:
                break;
              case TableModelEvent.UPDATE:
                break;
              case TableModelEvent.DELETE:
                break;
            }
         }
    }
    ****/

    private class RadioButtonRenderer implements TableCellRenderer {
        public Component getTableCellRendererComponent(JTable table1, Object value,
                 boolean isSelected, boolean hasFocus, int row, int column) {
             if (value == null) return null;
             return (Component)value;
        }
    }


    private class RadioButtonEditor extends DefaultCellEditor {
       private JRadioButton button;

       public RadioButtonEditor(JCheckBox checkBox) {
            super(checkBox);
       }

       public Component getTableCellEditorComponent(JTable table1, Object value,
                   boolean isSelected, int row, int column) {
            if (value == null) return null;
            button = (JRadioButton)value;
            button.setSelected(true);
            table1.revalidate();
            return (Component)value;
       }

       public Object getCellEditorValue() {
            return button;
       }
    }

    private class VjJTable extends JTable {

       public VjJTable(TableModel dm) {
           super(dm);
       }

       public String getToolTipText(MouseEvent event) {
           Point p = event.getPoint();
           int column = columnAtPoint(p);
           int row = rowAtPoint(p);
           VjPlotterObj obj = null;
           try {
               obj = deviceList.get(row);
           }
           catch (IndexOutOfBoundsException e) { }
           if (obj == null)
               return null;
           if (column == VNMR_PLOTTER)
               return "Click to set VNMR plotter";
           if (column == VNMR_PRINTER)
               return "Click to set VNMR printer";

           if (!obj.bChangeable)
               return null;
           if (column == VNMR_DEVICE)
               return "Click to edit device name";
           if (column == OS_PRINTER)
               return "Click to change printer";
           return null;
       }           
    }
} // end of VjPlotterTable

