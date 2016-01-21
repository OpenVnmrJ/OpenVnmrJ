/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import javax.swing.table.*;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;


public class VJTable extends ResizableContainer implements TableModelListener, GraphSeries {
   public int  id;
   public boolean  bValid = true;
   public boolean  bPrinted = false;
   public Color    fgColor;
   public  String  filePath = null;
   private JTable  table;
   private String[] titles;
   private long  fileDate = 0;
   private JScrollPane scrollpane;
   private DefaultTableModel tableModel;


   public VJTable(int n) {
       super();
       this.id = n;
       buildUI();
   }

   private void buildUI() {
       setLayout(new tabLayout());
       createTableModel();
       table = new JTable(tableModel);
       scrollpane = new JScrollPane(table);
       add(scrollpane);

       setBackground(null);
       JViewport viewPort = scrollpane.getViewport();
       scrollpane.setBackground(null);
       viewPort.setBackground(null);
       table.getModel().addTableModelListener(this);
       scrollpane.setSize(400, 200);
   }

   public int getId() {
       return id;
   }

   public void load(String fileName) {
        int  cols, cindex;
        String line, data, value;
        String[] rowStrs = null;
        BufferedReader fin = null;
        StringTokenizer tok;

        String fpath = FileUtil.openPath(fileName);
        if (fpath == null) {
            fpath = FileUtil.openPath("USER/"+fileName);
            if (fpath == null)
                return;
        }
        File fd = new File(fpath);
        if (fpath.equals(filePath)) {
            if (fd.lastModified() == fileDate)
                return;
        }
        else
            filePath = fpath;
        fileDate = fd.lastModified();

        if (scrollpane == null)
            buildUI();

        tableModel.setNumRows(0);
        cols = 0;
        try {
            fin = new BufferedReader(new FileReader(fpath));
            while ((line = fin.readLine()) != null) {
                tok = new StringTokenizer(line, ",\r\n");
                while (tok.hasMoreTokens()) {
                    tok.nextToken();
                    cols++;
                }
                if (cols > 0) {
                    titles = new String[cols];
                    rowStrs = new String[cols];
                    tok = new StringTokenizer(line, ",\r\n");
                    cindex = 0;
                    while (tok.hasMoreTokens()) {
                        titles[cindex] = tok.nextToken();
                        cindex++;
                    }
                    break;
                }
            }
            if (cols > 0) {
               while ((line = fin.readLine()) != null) {
                    tok = new StringTokenizer(line, ",\r\n");
                    cindex = 0;
                    while (cindex < cols) {
                        if (!tok.hasMoreTokens())
                             break;
                        rowStrs[cindex] = tok.nextToken();
                        cindex++;
                    }
                    if (cindex > 0)
                        tableModel.addRow(rowStrs); 
                }
            }
        }
        catch (Exception e) { }
        finally {
            try {
               if (fin != null)
                   fin.close();
               fin = null;
            }
            catch (Exception e2) {}
        }
        if (cols > 0)
            tableModel.setColumnCount(cols);
        scrollpane.revalidate();
        doLayout();
   }

   private void createTableModel() {
        tableModel = new DefaultTableModel() {
            public int getColumnCount() {
                 int n = 2;

                 if (titles != null)
                    n = titles.length;
                 return n;
            }

            public int getRowCount() {
                 return super.getRowCount();
            }

            public Object getValueAt(int row, int col) {
                 return super.getValueAt(row, col);
            }

            public String getColumnName(int column) {
                 if (titles != null)
                     return titles[column];
                 return "null";
            }

            public void setValueAt(Object aValue, int row, int column) {
                 super.setValueAt(aValue, row, column);
            }
        };
   }

   public void tableChanged(TableModelEvent e) {
        int row = e.getFirstRow();
        int column = e.getColumn();
        TableModel model = (TableModel)e.getSource();
        int cols = model.getColumnCount();
        if (cols < 1 || row < 0 || column < 0)
            return;
        StringBuffer sb = new StringBuffer().append("jFunc(").append(VnmrKey.TABLEACT).append(",").append(id).append(",").append(row).append(",");
        for (int i = 0; i < cols; i++) {
            String s = (String) model.getValueAt(row, i);
            sb.append("'");
            sb.append(s.trim());
            if (i < (cols -1))
               sb.append("',");
            else
               sb.append("'");
        }
        sb.append(")\n");
        Util.sendToVnmr(sb.toString());
   }

   public void clear() {
        table = null;
        scrollpane = null;
        tableModel = null;
        titles = null;
        filePath = null;
        removeAll();
   }

   public void setColor(Color c) {
      fgColor = c;
      if (c != null)
          table.setForeground(c);
   }

   public Color getColor() {
      return fgColor;
   }

   public boolean isValid() {
      return bValid;
   }

   public void setValid(boolean s) {
      bValid = s;
   }

   public void setXorMode(boolean b) {
   }

   public boolean isXorMode() {
       return false;
   }

   public void setTopLayer(boolean b) {
   }

   public boolean isTopLayer() {
       return false;
   }

   public void setLineWidth(int n) {
   }

   public int getLineWidth() {
       return 1;
   }

   public void setAlpha(float n) {
   }

   public float getAlpha() {
        return 1.0f;
   }

   public boolean intersects(int x, int y, int x2, int y2) {
       return false;
   }

   public void setConatinerGeom(int x, int y, int w, int h) {
   }

   @Override
   public void print(Graphics g) {
       if (table == null)
          return;
       JTableHeader header = table.getTableHeader();
       Dimension dim = header.getSize();
       Dimension pDim = header.getPreferredSize();
       int w = scrollpane.getWidth();
       int h = scrollpane.getHeight();
       if (w < 10)
            return;
       if (dim.width > w) {
            dim.width = w - 2;
            header.setSize(dim);
       }
       else if (dim.width < 2) { // uninitialized
            if (pDim.width > 2)
                dim.width = pDim.width;
            else
                dim.width = w - 2;
            if (pDim.height > 2)
                dim.height = pDim.height;
            else if (dim.height < 2)
                dim.height = 18;
            header.setSize(dim);
       }
       header.print(g);
       g.translate(0, dim.height);

       dim = table.getSize();
       if (dim.width < 2) {
            pDim = table.getPreferredSize();
            if (pDim.width > 2)
                dim.width = pDim.width;
            else
                dim.width = w - 2;
            if (pDim.height > 2)
                dim.height = pDim.height;
            else if (dim.height < 2)
                dim.height = h - 10;
            table.setSize(dim);
       }
       table.printAll(g);
           // scrollpane.paint(g);
   }

   public void draw(Graphics2D g, boolean bVertical, boolean bRight) {
       if (scrollpane == null)
           return;
       scrollpane.paint(g);
   }

   private class tabLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(500, 500);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim = target.getSize();
                int width = dim.width;
                int height = dim.height;
                if (width < 2 || height < 2)
                    return;

                if (scrollpane != null) {
                   scrollpane.setBounds(borderWidth, borderWidth, width - borderWidth * 2, height - borderWidth * 2);
                   scrollpane.validate();
                }
            }
        }

   }
}

