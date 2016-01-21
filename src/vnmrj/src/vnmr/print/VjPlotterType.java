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
import java.awt.event.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import javax.swing.table.*;
import javax.swing.event.*;

import vnmr.util.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;


public class VjPlotterType extends JPanel implements ActionListener {
    private boolean bAdmin;
    private java.util.List <VjPageAttributes> pageList;
    private java.util.List <VjPageAttributes> origPageList;
    private JTable  table;
    private TableRowSorter<DefaultTableModel> tableSorter;
    private JButton   addBut, deleteBut, resetBut;
    private DefaultTableModel tableModel;
    private VjPlotterConfig configPan;
    private PrintWriter fout = null;
    private JScrollPane scrollpane;
    private boolean dummyLine = false;
    private boolean bNewList;
    private boolean bChanged;
    private boolean bViewOnly;
    private int tmp_id = 99; 
    private long userFileDate;
    private long sysFileDate;
    private String userFilePath;
    private String sysFilePath;

    private static int nameSize = 20; 
    private String[] titles;

    final String[] sysTitles = { "Name" };
    final String[] userTitles = { "Name", "System/User" };
    final String[] belongs = { "System", "User" };
    private static String addCmd = "add new";
    private static String deleteCmd = "delete";
    private static String resetCmd = "reset";
    private static String sysFile =
                         FileUtil.sysdir()+File.separator+"devicetable";
    private static String userFile =
                         FileUtil.usrdir()+File.separator+".devicetable";


    public VjPlotterType(boolean admin,  boolean viewOnly, VjPlotterConfig cf) {
        this.bAdmin = admin;
        this.bViewOnly = viewOnly;
        setLayout(new BorderLayout());
        this.configPan = cf;
        this.bNewList = true;
        checkFiles();
        buildList();
        createTable();
        createCtrlPanel();
        // setupTable();
    }

    public VjPlotterType(boolean admin, VjPlotterConfig cf) {
        this(admin, false, cf);
    }
 
    public boolean rebuild() {
        if (!checkFiles())
            return false;
        buildList();
        if (tableModel != null)
            tableModel.setNumRows(pageList.size());
        // setupTable();
        return true;
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
        if (pageList == null)
             pageList =  new ArrayList<VjPageAttributes>();
        else
             pageList.clear();
        bChanged = false;
        tmp_id = 99; 
        if (userFilePath != null)
            addPageLayouts(userFilePath, false, true);

        if (sysFilePath != null)
            addPageLayouts(sysFilePath, true, bAdmin);

        for (int n = 0; n < pageList.size(); n++) {
            VjPageAttributes obj = pageList.get(n);
            if (obj.rasterStr != null && (obj.rasterStr.equals("0"))) {
               if (obj.formatStr == null || obj.paperWidthStr == null) { 
                  obj.bOldHpgl = true;
                  obj.bChangeable = false;
               }
            }
        }

        origPageList = new ArrayList<VjPageAttributes>(pageList);
    }

    public boolean saveToFile() {
        String fileName;
        String inPath = null;
        String outPath = null;
        boolean bTest = false;

        if (bViewOnly)
            return true;
        updateEditing();
        if (!bChanged) {
            if (!isLayoutChanged())
                return true;
        }
        if (configPan != null)
            bTest = configPan.isTest();
        if (bAdmin) {
            fileName = sysFile;
            if (bTest) {
                System.out.println(" save devicetable to tmp "+bTest);
                inPath = FileUtil.openPath(sysFile);
            }
            else
                inPath = FileUtil.savePath(sysFile, false);
        }
        else {
            if (bTest)
               System.out.println(" save devicetable to userdir ");
            fileName = userFile;
            inPath = FileUtil.savePath(userFile, false);
        }
        if (inPath == null) {
            JOptionPane.showMessageDialog(null,
                    "Could not open file "+fileName+".\n Permission denined.",
                    "Save devicetable", JOptionPane.ERROR_MESSAGE );
            return false;
        }
        if (bAdmin) {
            if (bTest)
                fileName = "/tmp/devicetable";
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
                    "Save devicetable", JOptionPane.ERROR_MESSAGE );
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

        boolean bSkip = false;
        StringTokenizer tok;
        String type, value;

        try {
            fin = new BufferedReader(new FileReader(inPath));
            while ((inData = fin.readLine()) != null) {
                  if (inData.startsWith("#")) {
                      if (inData.indexOf("end of ") < 0) {
                           if (bSkip) {
                               if (!dummyLine) {
                                  fout.println(inData);
                                  dummyLine = true;
                               }
                           }
                           else {
                               fout.println(inData);
                               dummyLine = true;
                           }
                      }
                  }
                  else {
                     tok = new StringTokenizer(inData, " \t\r\n");
                     if (tok.countTokens() > 1) {
                         type = tok.nextToken();
                         if (type.equals("PrinterType")) {
                             value = tok.nextToken();
                             if (value.endsWith(VjPlotterTable.appendStr)) {
                                  bSkip = true;
                             }
                             else
                                  bSkip = false;
                         }
                     }
                     if (!bSkip) {
                          fout.println(inData);
                          dummyLine = false;
                     }
                  }
             } // end of while
        }
        catch(IOException er2) { }

        VjPageAttributes obj;
        boolean bGoodItem = true;
        
        for (int i = 0; i < pageList.size(); i++) {
            obj = pageList.get(i);
            if (bAdmin) {
                 if (!obj.bSystem)
                     obj = null;
            }
            else {
                 if (obj.bSystem)
                     obj = null;
            }
            if (obj != null) {
                 bGoodItem = saveObjToFile(i, obj);
                 if (!bGoodItem)
                     break;
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
            return bGoodItem;
        }
/****
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

        bChanged = false;
        return bGoodItem;
    }

    private void writeDummyLine() {
        if (dummyLine)
           return;
        fout.println("################################################");
        dummyLine = true;
    }

    private void writeObj(String name, String value) {
        fout.print(name);
        int n = name.length();
        while (n < nameSize) {
            fout.print(" ");
            n++;
        }
        if (value != null)
            fout.println(value);
        else
            fout.println("0");
    }

    private String formatDouble(double dval) {
         String s = Double.toString(dval);
         int k = s.length();
         int i = s.indexOf('.');
         
         if (i < 0 || ((k - i) <= 5))
             return s;
         return s.substring(0, i + 4);
    }

    private boolean saveObjToFile(int n, VjPageAttributes obj) {
        boolean bGoodItem = true;
        String s = obj.typeName;

        if (s == null || (s.length() < 1)) {
           /***
            if (!isVisible())
               VjPrintUtil.showConfigTab(this);
            int row = tableSorter.convertRowIndexToView(n);
            Rectangle r = table.getCellRect(row, 0, false);
            if (row > 0) {
               r.y -= 20;
               r.height += 50;
            } 
            table.scrollRectToVisible(r);
            table.setRowSelectionInterval(row, row);
            JOptionPane.showMessageDialog(null,
                       "Page type name is empty.", 
                       "Save devicetable", JOptionPane.ERROR_MESSAGE );
            ***/
            return false;
        }
        if (!s.endsWith(VjPlotterTable.appendStr))
            return true;
        if (obj.linkCount() < 1)
            return true;

        writeDummyLine();
        obj.init();
        writeObj("PrinterType", obj.typeName);
       /**
        if (obj.printCap != null)
            writeObj("Printcap", obj.printCap);
       **/
        double d = 0.0;
        double d2 = 0.0;
        double res = 0.0;
        int   k;
        s = VjPageAttributes.getAttr(obj, VjPlotDef.RESOLUTION).trim();
        res = VjPrintUtil.getDouble(s);
        if (res < 12.0) {
            s = obj.resolutionStr.trim();
            res = VjPrintUtil.getDouble(s);
            if (res < 12.0)
               res = 150.0;
        }
        if (obj.raster > 0 && obj.raster < 3) { // PCL
            res = VjPrintUtil.getPclDpi(res);
        }
        d = res / 25.4;
        writeObj("ppmm", formatDouble(d));
        s = VjPageAttributes.getAttr(obj, VjPlotDef.RASTER).trim();
        writeObj("raster", s);
        s = VjPageAttributes.getAttr(obj, VjPlotDef.FORMAT);
        if (s != null)
            writeObj("format", s);
        if (obj.raster > 0 && obj.raster < 3) { // PCL
            k = (int) (15.0 * res / 150.0 + 0.5);
            writeObj("raster_charsize", Integer.toString(k));
            k = (int) res;
            writeObj("raster_resolution", Integer.toString(k));
        }
        else {
            writeObj("raster_charsize", "0");
            writeObj("raster_resolution", "0");
        }
        // if (obj.raster_pageStr != null)
        //     writeObj("raster_page", obj.raster_pageStr);
        if (obj.raster > 0) {
           /**
            writeObj("xoffset", formatDouble(obj.xoffset));
            writeObj("yoffset", formatDouble(obj.yoffset));
            writeObj("xoffset1", formatDouble(obj.xoffset1));
            writeObj("yoffset1", formatDouble(obj.yoffset1));
            ***/
            writeObj("xoffset", "0");
            writeObj("yoffset", "0");
            writeObj("xoffset1", "0");
            writeObj("yoffset1", "0");
            // k = (int) (15.0 * res / 300.0 + 0.5);
            k = 15;
            writeObj("xcharp1", Integer.toString(k));
            writeObj("ycharp1", Integer.toString(k * 2));
        }
        else {
            writeObj("xoffset", obj.xoffsetStr);
            writeObj("yoffset", obj.yoffsetStr);
            writeObj("xoffset1", obj.xoffset1Str);
            writeObj("yoffset1", obj.yoffset1Str);
            if (obj.xcharp1 <= 0.0)
                k = (int) (15.0 * res / 127.0);
            else
                k = obj.xcharp1;
            writeObj("xcharp1", Integer.toString(k));
            if (obj.ycharp1 <= 0.0)
                k = (int) (30.0 * res / 127.0);
            else
                k = obj.ycharp1;
            writeObj("ycharp1", Integer.toString(k));
        }
        s = VjPageAttributes.getAttr(obj,VjPlotDef.WC_MAX);
        d = VjPrintUtil.getDouble(s);
        d2 = d / 5.0; 
        writeObj("wcmaxmin", formatDouble(d2));
        writeObj("wcmaxmax", formatDouble(d));

        s = VjPageAttributes.getAttr(obj, VjPlotDef.WC2_MAX);
        d = VjPrintUtil.getDouble(s);
        d2 = d / 5.0; 
        writeObj("wc2maxmin", formatDouble(d2));
        writeObj("wc2maxmax", formatDouble(d));

        writeObj("color", VjPageAttributes.getAttr(obj, VjPlotDef.COLOR));
        writeObj("linewidth", VjPageAttributes.getAttr(obj, VjPlotDef.LINEWIDTH));
        writeObj("papersize", VjPageAttributes.getAttr(obj, VjPlotDef.PAPER));
        writeObj("paperwidth", VjPageAttributes.getAttr(obj, VjPlotDef.PAPER_WIDTH));
        writeObj("paperheight", VjPageAttributes.getAttr(obj, VjPlotDef.PAPER_HEIGHT));

        writeObj("right_edge", VjPageAttributes.getAttr(obj, VjPlotDef.RIGHT_EDGE));
        writeObj("left_edge", VjPageAttributes.getAttr(obj, VjPlotDef.LEFT_EDGE));
        writeObj("top_edge", VjPageAttributes.getAttr(obj, VjPlotDef.TOP_EDGE));
        writeObj("bottom_edge", VjPageAttributes.getAttr(obj, VjPlotDef.BOTTOM_EDGE));
        dummyLine = false;
        return bGoodItem;
    }


    public String[] getLayoutList() {
        int num = pageList.size();
        if (num < 1)
            return null;
        String[] list = new String[num];
        VjPageAttributes obj;
        for (int i = 0; i < num; i++) {
            obj = pageList.get(i);
            list[i] = obj.typeName;
        }
        return list;
    } 

    public VjPageAttributes getPageLayout(String name, boolean systemOnly) {
        if (name == null)
            return null;
        VjPageAttributes obj;
        int num = pageList.size();
        for (int i = 0; i < num; i++) {
            obj = pageList.get(i);
            if (name.equals(obj.typeName)) {
                if (systemOnly) {
                   if (obj.bSystem)
                      return obj;
                }
                else
                   return obj;
            }
        }
        return null;
    } 


    public void clearLayoutLinkCount() {
        VjPageAttributes obj;
        int num = pageList.size();
        for (int i = 0; i < num; i++) {
            obj = pageList.get(i);
            obj.clearLinkCount();
        }
    } 


    public void showPageType(String name) {
        if (name == null)
            return;
        int num = pageList.size();
        int index = -1;
        for (int i = 0; i < num; i++) {
            VjPageAttributes obj = pageList.get(i);
            if (name.equals(obj.typeName)) {
                index = i;
                break;
            }
        }
        if (index < 0)
            return;
        int row = tableSorter.convertRowIndexToView(index);
        if (row >= 0)
            table.setRowSelectionInterval(row, row);
    }

    private void showPageType() {
        if (!isVisible())
            return;
        int row = table.getSelectedRow();
        if (row < 0)
            return;
        VjPageAttributes obj = getItemAt(row);
        if (obj == null)
            return;
        configPan.setPageAttribute(obj);
        Rectangle r = table.getCellRect(row, 0, false);
        if (row > 0) {
            r.y -= 20;
            r.height += 50;
        } 
        table.scrollRectToVisible(r);
    }

    private boolean isLayoutChanged() {
        int num = pageList.size();
        for (int i = 0; i < num; i++) {
            VjPageAttributes layout = pageList.get(i);
            if (layout.isChanged()) {
                return true;
            }
        }
        return false;
    }

    private void updateEditing() {
        if (!isVisible())
            return;
        TableCellEditor editor = table.getCellEditor();
        if (editor != null) {
            editor.stopCellEditing();
        }
    }


    public void setVisible(boolean s) {
        updateEditing();
        super.setVisible(s);
        if (!s)
            return;
        showPageType();
    }


    private void addToList(String name, VjPageAttributes newObj) {
        /**
        for (int n = 0; n < num; n++) {
            obj = pageList.get(n);
            if (name.compareToIgnoreCase(obj.typeName) < 0) {
                pageList.add(n, newObj);
                return;
            }
        }
      **/
        pageList.add(newObj);
    }

    private void addPageLayouts(String fpath, boolean bSystem, boolean bWritable) {
        String data, type, value, vStr;
        StringTokenizer tok;
        BufferedReader fin = null;
        VjPageAttributes pObj = null;

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
              vStr = tok.nextToken();
              value = vStr.trim();
              if (type.equals("PrinterType")) {
                  pObj = new VjPageAttributes(value, bSystem);
                  addToList(value, pObj);
                  pObj.bChangeable = bWritable;
                  continue;
              }
              if (pObj == null)
                  continue;
              if (type.equals("Printcap")) {
                  pObj.printCap = value;
                  continue;
              }
              if (type.equals("ppmm")) {
                  pObj.ppmmStr = value;
                  continue;
              }
              if (type.equals("raster")) {
                  pObj.rasterStr = value;
                  continue;
              }
              if (type.equals("raster_charsize")) {
                  pObj.raster_charsizeStr = value;
                  continue;
              }
              if (type.equals("raster_page")) {
                  pObj.raster_pageStr = value;
                  continue;
              }
              if (type.equals("raster_resolution")) {
                  pObj.raster_resolutionStr = value;
                  continue;
              }
              if (type.equals("right_edge")) {
                  pObj.right_edgeStr = value;
                  continue;
              }
              if (type.equals("left_edge")) {
                  pObj.left_edgeStr = value;
                  continue;
              }
              if (type.equals("top_edge")) {
                  pObj.top_edgeStr = value;
                  continue;
              }
              if (type.equals("bottom_edge")) {
                  pObj.bottom_edgeStr = value;
                  continue;
              }
              if (type.equals("xoffset")) {
                  pObj.xoffsetStr = value;
                  continue;
              }
              if (type.equals("yoffset")) {
                  pObj.yoffsetStr = value;
                  continue;
              }
              if (type.equals("xoffset1")) {
                  pObj.xoffset1Str = value;
                  continue;
              }
              if (type.equals("yoffset1")) {
                  pObj.yoffset1Str = value;
                  continue;
              }
              if (type.equals("xcharp1")) {
                  pObj.xcharp1Str = value;
                  continue;
              }
              if (type.equals("ycharp1")) {
                  pObj.ycharp1Str = value;
                  continue;
              }
              if (type.equals("wcmaxmin")) {
                  pObj.wcmaxminStr = value;
                  continue;
              }
              if (type.equals("wcmaxmax")) {
                  pObj.wcmaxmaxStr = value;
                  continue;
              }
              if (type.equals("wc2maxmin")) {
                  pObj.wc2maxminStr = value;
                  continue;
              }
              if (type.equals("wc2maxmax")) {
                  pObj.wc2maxmaxStr = value;
                  continue;
              }
              if (type.equals("papersize")) {
                  pObj.paperSize = value;
                  continue;
              }
              if (type.equals("paperwidth")) {
                  pObj.paperWidthStr = value;
                  continue;
              }
              if (type.equals("paperheight")) {
                  pObj.paperHeightStr = value;
                  continue;
              }
              if (type.equals("format")) {
                  pObj.formatStr = value;
                  continue;
              }
              if (type.equalsIgnoreCase("color")) {
                  pObj.colorStr = value;
                  continue;
              }
              if (type.equalsIgnoreCase("linewidth")) {
                  pObj.linewidthStr = value;
                  continue;
              }
           }
        }
        catch(IOException e)
        { }

        finally {
           try {
               if (fin != null)
                   fin.close();
           }
           catch(IOException e) {}
        }
    }

    /****
    private void setupTable() {
        if (table == null)
            return;
        if (tableModel != null)
            tableModel.setNumRows(pageList.size());
        TableColumn nameColumn = table.getColumn(titles[0]);
        nameColumn.setPreferredWidth(120);
        if (table.getColumnCount() > 1) {
            TableColumn systemColumn = table.getColumn(titles[1]);
            systemColumn.setPreferredWidth(82);
            systemColumn.setMaxWidth(86);
            systemColumn.setMinWidth(12);
        }
    }
    ****/

    private void createTableModel() {
        tableModel = new DefaultTableModel() {
            public int getColumnCount() {
                 return titles.length;
            }

            public int getRowCount() {
                 return super.getRowCount();
            }

            public Object getValueAt(int row, int col) {
                 String retStr = null;
                 VjPageAttributes obj = null;

                 try {
                    obj = pageList.get(row);
                 }
                 catch (IndexOutOfBoundsException e) {
                     return retStr;
                 }
                 if (obj == null)
                      return null;
                 switch (col) {
                      case 0:
                            retStr = obj.typeName;
                            break;
                       case 1:
                            if (obj.bSystem)
                               retStr = belongs[0]; // system
                            else
                               retStr = belongs[1]; // user
                            break;
                 }
                 return retStr;
            }

            public String getColumnName(int column) {
                 return titles[column];
            }

            public Class<String> getColumnClass(int c) {
                 return String.class;
            }

            public boolean isCellEditable(int row, int col) {
                 if (bAdmin) {
                     return true;
                 }
                 VjPageAttributes obj = null;
                 try {
                     obj = pageList.get(row);
                 }
                 catch (IndexOutOfBoundsException e) {
                     return false;
                 }
                 if (obj == null)
                     return false;
                 if (col == 1)
                     return false;
                 return obj.bChangeable;
            }

            public void setValueAt(Object aValue, int row, int column) {
                 VjPageAttributes obj = (VjPageAttributes)pageList.get(row);
                 if (obj == null)
                      return;
                 if (!(aValue instanceof String))
                      return;
                 String s = (String) aValue;
                 bChanged = true;
                 switch (column) {
                      case 0:
                             obj.typeName = s;
                             break;
                      case 1:
                             if (s.equals(belongs[0]))
                                obj.bSystem = true;
                             else
                                obj.bSystem = false;
                             break;
                 }
            }
        };
    }

    private void createTable() {
        if (bAdmin)
            titles = sysTitles;
        else
            titles = userTitles;
        createTableModel();
        table = new JTable(tableModel);
        tableSorter = new TableRowSorter<DefaultTableModel>(tableModel);
        table.setRowSorter(tableSorter);
        table.setRowHeight(28);
        table.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        table.getSelectionModel().addListSelectionListener(
               new RowSelectionHandler());

        scrollpane = new JScrollPane(table);
        add(scrollpane, BorderLayout.CENTER);
    }

    private void createCtrlPanel() {
        JPanel ctrlPan = new JPanel();
        // ctrlPan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        ctrlPan.setLayout(new SimpleH2Layout(SimpleH2Layout.CENTER, 12, 5, false));
        addBut = new JButton("Add new type");
        addBut.setActionCommand(addCmd);
        addBut.addActionListener(this);
        deleteBut = new JButton("Delete selected");
        deleteBut.setActionCommand(deleteCmd);
        deleteBut.addActionListener(this);
        resetBut = new JButton("Reset to origin");
        resetBut.setActionCommand(resetCmd);
        resetBut.addActionListener(this);
        ctrlPan.add(addBut);
        ctrlPan.add(deleteBut);
        ctrlPan.add(resetBut);
        if (!bViewOnly)
            add(ctrlPan, BorderLayout.SOUTH);
    }

    
    private VjPageAttributes getItemAt(int row) {
        int n =  pageList.size();
        if (n < 1 || row < 0)
            return null;
        VjPageAttributes retObj = null;
        int s = tableSorter.convertRowIndexToModel(row);

        if (s >= 0 && s < n)
            retObj = pageList.get(s);

        String type = (String)table.getValueAt(row, 0);
        if (type == null)
            return retObj;
        if (retObj != null) {
            if (type.equals(retObj.typeName)) {
                return retObj;
            }
        }

        for (int k = 0; k < n; k++) {
            VjPageAttributes obj = pageList.get(k);
            if (obj != null && obj.typeName.equals(type)) {
                  if (obj.bChangeable)
                      return obj;
                  if (retObj == null)
                      retObj = obj;
            }
        }
        return retObj;
    }

    private boolean removeType(int row) {
        VjPageAttributes obj = getItemAt(row);
        if (obj == null || !obj.bChangeable)
            return false;
        return pageList.remove(obj);
    }

    public boolean isNewList() {
        return bNewList;
    }

    public void setNewList(boolean b) {
        bNewList = b;
    }

    public VjPageAttributes clonePageAttributes(VjPageAttributes orgObj,
                                            String name) {
        VjPageAttributes newObj = createPageAttributes(name, false);
        VjPageAttributes.cloneAttrs(newObj, orgObj);
        // newObj.init();
        return newObj;
    }

    public void removePageAttributes(VjPageAttributes obj) {
        if (obj.printCap != null) // old style
           return;
        if (bAdmin) {
           if (!obj.bSystem)
               return;
        }
        else {
           if (obj.bSystem)
               return;
        }
        if (pageList.contains(obj))
           pageList.remove(obj);
    }

    public VjPageAttributes createPageAttributes(String name, boolean bInit) {
        bNewList = true;
        bChanged = true;
        VjPageAttributes vobj = new VjPageAttributes(name, bAdmin);
        vobj.bNewSet = true;
        vobj.bChangeable = true;
        if (bInit)
            vobj.init();
        pageList.add(vobj);
        return vobj;
    }

    public VjPageAttributes createPageAttributes(String name) {
        return createPageAttributes(name, true);
    }


    public void linkPageLayouts(java.util.List<VjPlotterObj> list, boolean bCreate) {
        if (list == null)
           return;
        clearLayoutLinkCount();
        VjPlotterObj plotter;
        VjPageAttributes layout;
        String pageName;
        for (int i = 0; i < list.size(); i++) {
            plotter = list.get(i);
            layout = getPageLayout(plotter.type, plotter.bSystem);
            if (plotter.deviceName != null && plotter.deviceName.length() > 0) {
                if (plotter.deviceName.equals(VjPlotterTable.noneDevice))
                    pageName = plotter.deviceName;
                else
                    pageName = plotter.deviceName+VjPlotterTable.appendStr;
            }
            else {
                tmp_id++;
                pageName = VjPlotterTable.tmpStr+tmp_id;
            }
            if (layout != null && plotter.bChangeable) {
                if (!layout.bOldHpgl) {
                   if (!pageName.equals(plotter.type)) {
                       layout = clonePageAttributes(layout, pageName);
                       plotter.type = pageName;
                       layout.bSystem = plotter.bSystem;
                       layout.bChangeable = plotter.bChangeable;
                       plotter.bNewSet = true;
                   }
                }
            }
            if (layout == null && bCreate && plotter.bChangeable) {
                plotter.type = pageName;
                plotter.bNewSet = true;
                layout = createPageAttributes(pageName);
                layout.bSystem = plotter.bSystem;
                layout.bChangeable = plotter.bChangeable;
            }
            if (layout != null)
                layout.increaseLinkCount();
            plotter.setAttributeSet(layout);
        }
    }

    public void unLinkPageLayout(VjPlotterObj plotter) {
        if (plotter == null)
            return;
        VjPageAttributes layout = plotter.getAttributeSet();
        if (layout == null)
            return;
        layout.decreaseLinkCount();
        bChanged = true;
    }

    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
        int row, n;
        if (cmd.equals(addCmd)) {
            row = tableModel.getRowCount();
            tableModel.setNumRows(row + 1);
            table.revalidate();
            n = tableSorter.convertRowIndexToView(row);
            if (n >= 0 && n <= row) {
               Rectangle r = table.getCellRect(n, 0, false);
               if (n > 0) {
                  r.y -= 20; 
                  r.height += 50;
               }
               table.scrollRectToVisible(r);
               table.setRowSelectionInterval(n, n);
               table.editCellAt(n, 0);
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
            bNewList = true;
            bChanged = true;
            if (removeType(n)) { 
                tableModel.removeRow(n);
                row = tableModel.getRowCount();
                if (n < row)
                    table.setRowSelectionInterval(n, n);
            }
            return;
        }

        if (cmd.equals(resetCmd)) {
            bNewList = true;
            bChanged = false;
            pageList = new ArrayList<VjPageAttributes>(origPageList);
            tableModel.setNumRows(pageList.size());
            table.revalidate();
            return;
        }
    }



    private class RowSelectionHandler implements ListSelectionListener {
        public void valueChanged(ListSelectionEvent e) {
            if (e.getSource() == table.getSelectionModel()
                         && table.getRowSelectionAllowed()) {
                int row = table.getSelectedRow();
                if (row < 0)
                     return;
                showPageType();
                VjPageAttributes obj = getItemAt(row);
                if (obj == null) {
                     deleteBut.setEnabled(false);
                     return;
                }
                if (!obj.bChangeable)
                     deleteBut.setEnabled(false);
                else
                     deleteBut.setEnabled(true);
                // configPan.setPageType(obj.type);
                return;
            }
        }
    }

} // end of VjPlotterType

