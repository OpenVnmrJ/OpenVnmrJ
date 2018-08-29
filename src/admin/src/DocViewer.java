/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 *
 */
//DocViewer.java
//Software Design Document Viewer

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.event.*;
import java.net.*;
import java.io.*;
import java.util.*;
import java.util.regex.*;
import javax.swing.table.*;

import java.beans.*;
import java.awt.print.*;
import java.awt.Graphics;


public class DocViewer extends JPanel implements Printable {

    public  String  baseDir     = null;
    public  String  baseEditDir = null;
    public  String  fiPathStr   = null;
    public  String  category    = null;
    public  String  editFile    = null;
    public  File    fiPath      = null;

    public  DefaultMutableTreeNode   rootMutNode;
    public  DefaultTreeModel         defTreeModel;
    public  JTable                   jtable;
    public  JTree                    jtree;
    public  JEditorPane              docPane;
    public  JPanel                   topPan;
    public  JButton                  prtButton;
    public  JScrollPane              docScroPan;
    public  JSplitPane               splitPane;
    public  JPanel                   catButtonPan;
    public  JPanel                   prntPan;
    public  JPanel                   m_pnlShow;
    public  JButton                  lockButton;
    public  JButton                  unlockButton;
    public  JButton                  cancelButton;
    public  JComboBox                m_cmbShow;
    public  JTextField               m_txfUser;
    public  JLabel                   msge;
    public  int colwidth[] = {250, 100, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};
    public  int panesplit = 200;

    protected DefaultTableModel m_tableModel;
    protected Vector m_vecAllRows;
    protected Vector m_vecHeader;

    protected final String[] m_header =  {"Task", "File", "Resp", "Est", "Actual",
                                            "TaskStat", "DocEst", "Act", "Stat",
                                            "Tester", "Est", "Act", "Stat" };


    public DocViewer () {

        baseDir = System.getProperty("docdir");
        if( baseDir == null ) {
           System.out.println("\n The docdir environmental parameter need to be set \n");
           System.exit(0);
        }

        baseEditDir = System.getProperty("doceditdir");
        if( baseEditDir == null ) {
           baseEditDir = System.getProperty("docdir");
        }

        prtButton = new JButton("Print");
        prtButton.addActionListener(new PrintButtonListener());
        lockButton = new JButton("Lock tasks for editing");
        LockTasksListener lt = new LockTasksListener();
        lockButton.addActionListener(lt);
        unlockButton = new JButton("Finish editing");
        unlockButton.addActionListener(new UnlockTasksListener(lt));
        cancelButton = new JButton("Cancel editing");
        cancelButton.addActionListener(new CancelTasksListener(lt));
        msge = new JLabel("already being edited");


        prntPan = new JPanel();
        topPan = new JPanel();
        topPan.setLayout(new BorderLayout());
        catButtonPan = new JPanel();
        m_pnlShow = new JPanel();

        makeCategoryButton(baseDir);
        if (System.getProperty("tasks") != null) {
           makeCategoryEditButton(baseEditDir);
        }

        prntPan.add(prtButton);
        topPan.add(prntPan, BorderLayout.EAST);
        topPan.add(catButtonPan, BorderLayout.WEST);
        topPan.add(m_pnlShow, BorderLayout.SOUTH);
        setLayout(new BorderLayout());
        add(topPan, BorderLayout.NORTH);
        JLabel lblShow = new JLabel("Show Tasks");
        JLabel lblUser = new JLabel("Show Tasks for Developer");
        String[] aStrDone = {"All", "Done", "Active", "Not Done"};
        m_cmbShow = new JComboBox(aStrDone);
        m_txfUser = new JTextField("                   ");
        m_txfUser.setCaretPosition(0);
        m_pnlShow.add(lblShow);
        m_pnlShow.add(m_cmbShow);
        m_pnlShow.add(Box.createHorizontalStrut(5));
        m_pnlShow.add(lblUser);
        m_pnlShow.add(m_txfUser);
        m_pnlShow.setVisible(false);
        m_cmbShow.addActionListener(new ShowActionListener("TaskStat"));
        m_txfUser.addActionListener(new ShowActionListener("Resp"));

        rootMutNode  = new DefaultMutableTreeNode(null);

        defTreeModel = new DefaultTreeModel(rootMutNode);
        jtree = new JTree(defTreeModel);

        jtree.putClientProperty("JTree.lineStyle", "Angled");
        jtree.addTreeSelectionListener(new TreeListener());

        JScrollPane treeScroPan = new JScrollPane(jtree);

        docPane = new JEditorPane();
        Font font = new Font("monospaced",Font.PLAIN,12);
        if(font != null)
        {
          docPane.setFont(font);
          jtree.setFont(font);
        }

        docPane.setEditable(false);
        docScroPan = new JScrollPane();

        splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
        splitPane.setLeftComponent(treeScroPan);
        splitPane.setRightComponent(docScroPan);
        splitPane.setDividerSize((int) 5);
        splitPane.setPreferredSize(new Dimension(800, 600));
        splitPane.setDividerLocation(panesplit);

        add(splitPane, BorderLayout.CENTER);

        m_tableModel = new DefaultTableModel();
    }

    public void makeCategoryEditButton(String bdir) {

        File    catPath      = new File(bdir);
        File[]  catList      = catPath.listFiles();
        JButton cButton[]    = new JButton[catList.length];
        File    infoFile;

        for(int i = 0; i < catList.length; i++) {   // file LIST loop

           if(catList[i].isDirectory() & ! catList[i].isHidden()) {
              infoFile = new File(catList[i].getPath() + "/.taskinfo");
              if (infoFile.canRead() ) {
                 String cate = catList[i].getName().trim();

                 cButton[i] = new JButton("tasks from " + cate);
                 cButton[i].addActionListener(
                       new CategoryEditButtonListener(infoFile.getPath(), cate));

                 catButtonPan.add(cButton[i]);
              }
           }
        }
    }

    class CategoryEditButtonListener implements ActionListener {
        String dir;
        String fname;
        public CategoryEditButtonListener (String fileName, String dirname) {
           fname = fileName;
           dir = dirname;
        }
        public void actionPerformed(ActionEvent e) {
            editFile = fname;
            fillJtable();
            TableColumn column = null;
            for (int i = 0; i < 13; i++) {
                column = jtable.getColumnModel().getColumn(i);
                //column.setHeaderRenderer(m_renderer);
                column.setPreferredWidth( colwidth[i] );
            }
            jtable.validate();
            docScroPan.setViewportView(jtable);
            rootMutNode.removeAllChildren();
            rootMutNode.setUserObject(dir);
            m_pnlShow.setVisible(true);

            int curSplit = splitPane.getDividerLocation();
            if (curSplit > 30) {
               panesplit = curSplit;
            }
            splitPane.setDividerLocation(0);
            jtree.updateUI();
            prntPan.removeAll();
            prntPan.add(lockButton);
            prntPan.add(prtButton);
            topPan.remove(prntPan);
            topPan.add(prntPan, BorderLayout.EAST);
            topPan.validate();
       }
    }

    public void makeCategoryButton(String bdir) {

        File    catPath      = new File(bdir);
        File[]  catList      = catPath.listFiles();
        JButton cButton[]    = new JButton[catList.length];

        for(int i = 0; i < catList.length; i++) {   // file LIST loop

           if(catList[i].isDirectory() & ! catList[i].isHidden()) {
              String cate = catList[i].getName().trim();

              cButton[i] = new JButton(cate);
              cButton[i].addActionListener(new CategoryButtonListener());

              catButtonPan.add(cButton[i]);
           }
        }
    }

    class CategoryButtonListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {

            JButton cb = (JButton) e.getSource();
            category = cb.getText();
            fiPathStr = baseDir + "/" + category;

            docScroPan.setViewportView(docPane);
            rootMutNode.removeAllChildren();
            rootMutNode.setUserObject(category);
            populateTree(fiPathStr);
            m_pnlShow.setVisible(false);

            splitPane.setDividerLocation(panesplit);
            jtree.updateUI();
            prntPan.removeAll();
            prntPan.add(prtButton);
            topPan.remove(prntPan);
            topPan.add(prntPan, BorderLayout.EAST);
            topPan.validate();
       }
    }

    class LockTasksListener implements ActionListener {
        boolean locked = false;
        File lockFile;

        public void actionPerformed(ActionEvent e) {
           lockFile = new File(editFile + ".lck");
           try {
              if (lockFile.createNewFile()) {
                 locked = true;
                // reread file
                if (!m_cmbShow.equals("All") || !m_txfUser.getText().trim().equals(""))
                {
                 fillJtable();
                 jtable.validate();
                }
                 m_pnlShow.setVisible(false);
                 lockFile.deleteOnExit();
                 docScroPan.setViewportView(jtable);
                 prntPan.removeAll();
                 prntPan.add(unlockButton);
                 prntPan.add(cancelButton);
                 prntPan.add(prtButton);
                 topPan.remove(catButtonPan);
                 topPan.remove(prntPan);
                 topPan.add(prntPan, BorderLayout.EAST);
                 topPan.validate();
                 topPan.repaint();
              } else {
                 locked = false;
                 prntPan.removeAll();
                 prntPan.add(msge);
                 prntPan.add(lockButton);
                 prntPan.add(prtButton);
                 topPan.remove(prntPan);
                 topPan.remove(catButtonPan);
                 topPan.add(catButtonPan, BorderLayout.WEST);
                 topPan.add(prntPan, BorderLayout.EAST);
                 topPan.validate();
                 topPan.repaint();
              }
           } catch (IOException ee) {
// already locked
              locked = false;
              //m_pnlShow.setVisible(true);
              prntPan.removeAll();
              prntPan.add(msge);
              prntPan.add(lockButton);
              prntPan.add(prtButton);
              topPan.remove(prntPan);
              topPan.remove(catButtonPan);
              topPan.add(catButtonPan, BorderLayout.WEST);
              topPan.add(prntPan, BorderLayout.EAST);
              topPan.validate();
              topPan.repaint();
           }
        }

        public boolean isLocked() {
           return locked;
        }

        public void unlockFile() {
           locked = false;
           lockFile.delete();
        }
    }

    class UnlockTasksListener implements ActionListener {
        LockTasksListener lt;
        public UnlockTasksListener (LockTasksListener listener) {
           lt = listener;
        }
        public void actionPerformed(ActionEvent e) {
           if (lt.isLocked()) {
// write updated info
              File fdbk = new File(editFile + ".bkup");
              File fd = new File(editFile);
              fdbk.delete();
              fd.renameTo(fdbk);
              saveJtable();
              lt.unlockFile();
              m_pnlShow.setVisible(true);
              m_cmbShow.setSelectedItem("All");
              m_txfUser.setText("             ");
              prntPan.removeAll();
              prntPan.add(lockButton);
              prntPan.add(prtButton);
              topPan.remove(prntPan);
              topPan.remove(catButtonPan);
              topPan.add(catButtonPan, BorderLayout.WEST);
              topPan.add(prntPan, BorderLayout.EAST);
              topPan.validate();
              topPan.repaint();
           }
        }
    }

    class CancelTasksListener implements ActionListener {
        LockTasksListener lt;
        public CancelTasksListener (LockTasksListener listener) {
           lt = listener;
        }
        public void actionPerformed(ActionEvent e) {
           if (lt.isLocked()) {
              lt.unlockFile();
              m_pnlShow.setVisible(true);
              /*m_cmbShow.setSelectedItem("All");
              m_txfUser.setText("           ");*/
              prntPan.removeAll();
              prntPan.add(lockButton);
              prntPan.add(prtButton);
              topPan.remove(prntPan);
              topPan.remove(catButtonPan);
              topPan.add(catButtonPan, BorderLayout.WEST);
              topPan.add(prntPan, BorderLayout.EAST);
              topPan.validate();
              topPan.repaint();
           }
        }
    }

    class PrintButtonListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {

           Object source = e.getSource();
           if(source == prtButton) {
             printIt();
           }

            //JButton cb = (JButton) e.getSource();
            //if (e.getSource() instanceof JButton) {
            //   PrinterJob printJob = PrinterJob.getPrinterJob();
            //   printJob.setPrintable(this);
            //   if (printJob.printDialog()) {
            //      try {
            //          printJob.print();
            //      } catch (Exception ex) {
            //          ex.printStackTrace();
            //       }
            //   }
            //}
        }
    }


    class TreeListener implements TreeSelectionListener {
            public void valueChanged(TreeSelectionEvent e) {

                JTree refjtree = (JTree) e.getSource();
                DefaultMutableTreeNode dmtNode = (DefaultMutableTreeNode)
                                                  refjtree.getLastSelectedPathComponent();

                if (dmtNode == null) return;
                Object nodeInfo = dmtNode.getUserObject();

                if (! dmtNode.isRoot()) {
                    DocInfo book = (DocInfo)nodeInfo;
                    dsplDocByURL(book.bookURL);
                }
            }
    }

    public class ShowActionListener implements ActionListener
    {
        protected String m_strKey = "";

        public ShowActionListener(String strKey)
        {
            super();
            m_strKey = strKey;
        }

        public void actionPerformed(ActionEvent e)
        {
            Object obj = e.getSource();
            String strValue = null;
            if (obj instanceof JComboBox)
            {
                strValue = (String)m_cmbShow.getSelectedItem();
                //m_txfUser.setText("              ");
            }
            else
            {
                strValue = m_txfUser.getText();
                //m_cmbShow.setSelectedItem("All");
            }
            int nCol = m_vecHeader.indexOf(m_strKey);
            if (strValue != null && nCol >= 0)
            {
                updateJtable(m_strKey, strValue, nCol);
                jtable.validate();
                docScroPan.setViewportView(jtable);
                jtree.updateUI();
            }
        }
    }

    public class DocInfo {
        public  String  docTitle;
        public  URL     bookURL;

        public DocInfo(String title, String filepath) {

            docTitle = title;
            try {
                bookURL = new URL("file:" + filepath);
            } catch (java.net.MalformedURLException exc) {
                System.err.println("Attempted to create a DocInfo " + "with a bad URL: " + bookURL);
                bookURL = null;
            }
        }

        public String toString() {
            return docTitle;
        }
    }

    public void dsplDocByURL(URL url) {
        try {
            docPane.setPage(url);
        } catch (IOException e) {
            System.err.println("Attempted to read a bad URL: " + url);
        }
    }

    public void fillJtable()
    {
        fillJtable("All", "");
        m_cmbShow.setSelectedItem("All");
        m_txfUser.setText("         ");
    }

    public void fillJtable(String strShow, String strShowValue) {

        File    filePath   = new File(editFile);
        int rows;

        if (m_vecHeader == null)
        {
            m_vecHeader = new Vector();
            for (int i = 0; i < m_header.length; i++)
            {
                m_vecHeader.add(m_header[i]);
            }
        }

        try {
           FileReader fr = new FileReader(filePath);
           BufferedReader rb = new BufferedReader(fr);
           String line = null;
           int row = -1;
           int col = -1;

           if ((line = rb.readLine()) != null) {

              StringTokenizer tokStrg = new StringTokenizer(line, ":");
              String key = tokStrg.nextToken();
              rows = Integer.parseInt( tokStrg.nextToken().trim() );
              //String data[][] = new String[rows][13];
              m_vecAllRows = new Vector();
              Vector vecRow = new Vector();
              while((line = rb.readLine()) != null) {
                    tokStrg = new StringTokenizer(line, ":");
                    key = tokStrg.nextToken();
                    String val = tokStrg.nextToken();
                    if (val != null)
                        val = val.trim();
                    if (key.equals("task")) {
                      row++;
                      col = -1;
                      m_vecAllRows.add(vecRow);
                      vecRow = new Vector(13);
                    } else if (key.equals("subtask")) {
                      row++;
                      col = -1;
                      m_vecAllRows.add(vecRow);
                      vecRow = new Vector(13);
                      /*data[row][6] = "--";
                      data[row][7] = "--";
                      data[row][8] = "--";
                      data[row][9] = "--";
                      data[row][10] = "--";
                      data[row][11] = "--";
                      data[row][12] = "--";*/

                      for (int i = 0; i < 13; i++)
                      {
                        vecRow.add(i, "--");
                      }
                    }
                    col++;
                    //data[row][col] = val;
                    vecRow.add(col, val);
              }
              m_tableModel.setDataVector(m_vecAllRows, m_vecHeader);
              //jtable = new JTable(data, header);
              TableSort sorter = new TableSort(m_tableModel);
              jtable = new JTable(m_tableModel);
              jtable.setModel(sorter);
              sorter.addMouseListenerToHeaderInTable(jtable);
              /*TableColumn column = null;
              for (int i = 0; i < 13; i++) {
                 column = jtable.getColumnModel().getColumn(i);
                 //column.setHeaderRenderer(m_renderer);
                 column.setPreferredWidth( colwidth[i] );
              }*/
           }
           rb.close();
       }
       catch (IOException e) {
          e.printStackTrace();
       }
    }

    public void updateJtable(String strKey, String strShowValue, int nCol) {
        // if the show menu is selected, then show the selected values
        // in the table.
        Vector vecTableData = new Vector();
        if (strKey != null && strKey.length() > 0) {
            strShowValue = strShowValue.trim();
            Pattern pattern = Pattern.compile(".*\\b" + strShowValue + "\\b.*",
                                              Pattern.CASE_INSENSITIVE);
            for (int nRow = 0; nRow < m_vecAllRows.size(); nRow++) {
                Vector vecColValues = (Vector)m_vecAllRows.get(nRow);
                if (vecColValues == null || vecColValues.size() < 6)
                    continue;
                String strValue = (String)vecColValues.get(nCol);
				String strDone = "done";
                if (strValue == null) {
                    strValue = "";
                }
                strValue = strValue.trim();
                if (strShowValue.equals("All") || strShowValue.equals("")
                        || pattern.matcher(strValue).matches() ||
						(strShowValue.equalsIgnoreCase("Not done") &&
						 !strValue.equalsIgnoreCase(strDone)))
                {
                    vecTableData.add(vecColValues);
                }
            }
        } else {
            vecTableData = m_vecAllRows;
        }
        //m_tableModel = new DefaultTableModel();
        m_tableModel.setDataVector(vecTableData, m_vecHeader);
        //jtable = new JTable(data, header);
        TableSort sorter = new TableSort(m_tableModel);
        sorter.fireTableDataChanged();
        jtable = new JTable(m_tableModel);
        jtable.setModel(sorter);
        sorter.addMouseListenerToHeaderInTable(jtable);
        TableColumn column = null;
        for (int i = 0; i < 13; i++) {
            column = jtable.getColumnModel().getColumn(i);
            //column.setHeaderRenderer(m_renderer);
            column.setPreferredWidth( colwidth[i] );
        }
    }

    public void saveJtable() {

        File    filePath   = new File(editFile);
        final String[] header =  {"task", "file", "resp", "taskEst", "taskAct",
           "taskStat", "docEst", "docAct", "docStat", "tester", "testEst",
           "testAct", "testStat" };
        final String[] sheader =  {"subtask", "file", "resp", "taskEst", "taskAct",
           "taskStat" };
        int rows =  jtable.getRowCount();
        try {
           FileWriter fr = new FileWriter(filePath);
           BufferedWriter rb = new BufferedWriter(fr);
           rb.write("Rows: " + rows + "\n");
           for (int row = 0; row < rows; row++) {
              String strValue = (String)jtable.getValueAt(row, 12);
              if (strValue != null)
              {
                if (strValue.equals("--")) {
                    for (int col = 0; col < 6; col++) {
                        strValue = (String)jtable.getValueAt(row,col);
                        rb.write(sheader[col] + ": " + jtable.getValueAt(row,col) + "\n");
                    }
                } else {
                    for (int col = 0; col < 13; col++) {
                        strValue = (String)jtable.getValueAt(row,col);
                        rb.write(header[col] + ": " + jtable.getValueAt(row,col) + "\n");
                    }
                }
              }
           }
		   rb.flush();
           rb.close();
           Runtime rt = Runtime.getRuntime();
           Process prcs = rt.exec("chmod 666 " + editFile);

       }
       catch (IOException e) {
          e.printStackTrace();
       }
    }

    public    Vector fileNameListV = new Vector();
    public    Vector fileNodeListV = new Vector();
    public    Vector existNameV    = new Vector();
    public    Vector existNodeV    = new Vector();
    public    Vector refEeNameV    = new Vector();
    public    Vector refErNameV    = new Vector();

    public void populateTree( String fiPathStr ) {

        File    fiPath   = new File(fiPathStr);
        File[]  fList   = fiPath.listFiles();

        DefaultMutableTreeNode  branchNode      = null;
        String fullname = null;
        String title    = null;
        int    index    = -1;

        for(int i = 0; i < fList.length; i++) {   // file LIST loop

           if(fList[i].isFile() & ! fList[i].isHidden()) {
              String fname = fList[i].getName().trim();
              fileNameListV.addElement(fname);

              int refCnt = 0;
              fullname = fList[i].toString();

              try {
                 FileReader fr = new FileReader(fullname);
                 BufferedReader rb = new BufferedReader(fr);
                 String line = null;

                 boolean gotTitle = false;
                 boolean gotOneReference = false;
                 boolean gotAReference,htmlLine;
                 while((line = rb.readLine()) != null) {
                    htmlLine = gotAReference = false;
                    StringTokenizer tokStrg = new StringTokenizer(line, ">:");
                    while(tokStrg.hasMoreTokens()) {
                       String xxx = tokStrg.nextToken();
                               if (xxx.length() > 1) {
                         if ( (!htmlLine) && (xxx.charAt(0) == '<') &&
                              (xxx.charAt(1) == 'P'))
                            htmlLine = true;
                       }
                       if ((!gotTitle) && (xxx.equals("Title"))) {
                                      if (htmlLine)
                              title = stripHTML(tokStrg, rb);
                                      else
                                      {
                                        if (tokStrg.hasMoreTokens())
                                title = tokStrg.nextToken().trim();
                             else
                                title = "";
                                      }
                          // System.out.println("title: "+title);
                          gotTitle = true;
                          break;
                       }
                       if (xxx.equals("Reference")) {
                          gotOneReference = true;
                          gotAReference = true;

                                      String ref;
                          if (htmlLine)
                             ref = stripHTML(tokStrg, rb);
                          else
                          {
                             if (tokStrg.hasMoreTokens())
                                ref = tokStrg.nextToken().trim();
                             else
                                ref = "";
                          }
                          // System.out.println("reference: "+ref.trim());
                          refEeNameV.addElement(fname);
                          refErNameV.addElement(ref.trim());
                          refCnt++;
                          break;
                       }
                    }
                    if ((gotOneReference) && (!gotAReference))
                       break;
                 }
                 rb.close();

                 //create Node object here, have to wait to extract title off the file
                 branchNode = new DefaultMutableTreeNode(new DocInfo(title, fList[i].toString()));
                 fileNodeListV.addElement(branchNode);
              }
              catch (IOException e) {
                  e.printStackTrace();
              }

              if( refCnt == 0 ) {  //file with no ref. add to main trunk
                 index = fileNameListV.indexOf(fname);
                 branchNode = (DefaultMutableTreeNode) fileNodeListV.get(index);

                 rootMutNode.add(branchNode);
                 existNameV.addElement(fname);
                 existNodeV.addElement(branchNode);
              }
           }
        }

        //All ref. to not exist files go to main trunk
        index = -1;
        Enumeration eV = refErNameV.elements();
        while(eV.hasMoreElements()) {

            String refErName = (String) eV.nextElement();
            index = fileNameListV.indexOf(refErName);

            if( index < 0 ) {   //bogus ref. add to main trunk

                    index = refErNameV.indexOf(refErName);
                    String refEeName = (String) refEeNameV.get(index);

                    index = fileNameListV.indexOf(refEeName);
                    branchNode = (DefaultMutableTreeNode) fileNodeListV.get(index);

                    rootMutNode.add(branchNode);
                    existNameV.addElement(refEeName.trim());
                    existNodeV.addElement(branchNode);
                    refErNameV.remove(refErName);
                    refEeNameV.remove(refEeName);
            }
        }

        //go over refErNameV ,compare to filelist if not match is bogus ref. add to main trunk
        index = -1;
        eV = null;
        int leafIndex = -1;
        DefaultMutableTreeNode  leafNode      = null;
        int num = refErNameV.size();
        while( num > 0 ) {

             eV = refErNameV.elements();
             while(eV.hasMoreElements()) {

                 String refErName = (String) eV.nextElement();
                 index = existNameV.indexOf(refErName);

                 if( index > -1 ) {

                    branchNode = (DefaultMutableTreeNode) existNodeV.get(index);

                    index = refErNameV.indexOf(refErName);
                    String refEeName = (String) refEeNameV.get(index);

                    index = fileNameListV.indexOf(refEeName);
                    leafNode = (DefaultMutableTreeNode) fileNodeListV.get(index);

                    branchNode.add(leafNode);
                    existNameV.addElement(refEeName.trim());
                    existNodeV.addElement(leafNode);
                    refErNameV.remove(refErName);
                    refEeNameV.remove(refEeName);
                 }
             }

             if( refErNameV.size() == num ) {
                 String refErName = (String) refErNameV.firstElement();
                 index = existNameV.indexOf(refErName);

                 if( index > -1 ) {

                    branchNode = (DefaultMutableTreeNode) existNodeV.get(index);

                    index = refErNameV.indexOf(refErName);
                    String refEeName = (String) refEeNameV.get(index);

                    index = fileNameListV.indexOf(refEeName);
                    leafNode = (DefaultMutableTreeNode) fileNodeListV.get(index);

                    branchNode.add(leafNode);
                    existNameV.addElement(refEeName);
                    existNodeV.addElement(leafNode);
                    refErNameV.remove(refErName);
                    refEeNameV.remove(refEeName);
                 }
             }
             num = refErNameV.size();
       }
    }


        //Print Section
        //public PrinterJob prjob = PrinterJob.getPrinterJob();
        //public PageFormat pformat = prjob.defaultPage();

        public void printIt() {

           PrinterJob prjob = PrinterJob.getPrinterJob();
           PageFormat pformat = prjob.defaultPage();

           //prjob.setPrintable(this, pformat);
           prjob.setPrintable(this, pformat);
           if (! prjob.printDialog())
                return;

           try {
               prjob.print();
           }
           catch(Exception e) {
               e.printStackTrace();
           }
        }

        //method required for implementing Printable
        public int print(Graphics g, PageFormat pf, int pageIndex) throws PrinterException {

           if (pageIndex >= 1)
              return Printable.NO_SUCH_PAGE;

           Graphics2D g2 = (Graphics2D) g;
           g2.translate(pf.getImageableX(), pf.getImageableY());
           paint(g2);
           System.gc();
           return Printable.PAGE_EXISTS;
       }

    private String stripHTML(StringTokenizer tokStrg,BufferedReader rb) throws IOException
    {
       boolean AtEnd = false;
       boolean gotToken = false;
       String line, token, str;
       line = token = null;
       int tokenLen = 0;
       str = "";
       StringBuffer text = new StringBuffer(str);
       while(!AtEnd)
       {
          while(gotToken = tokStrg.hasMoreTokens())
          {
             token = tokStrg.nextToken(">").trim();
             tokenLen = token.length();
             // System.out.println("token: "+token);
             if (tokenLen > 0)
             {
              if (token.charAt(0) == '<' )
              {
                if (tokenLen > 2)
                {
                  if ( (token.charAt(1) == '/') &&
                     (token.charAt(2) == 'P') )
                  {
                            AtEnd = true;
                            break;
                  }
                }
                continue;
              }
             }
             AtEnd = testEndPara(token);
             String sToken = StripTagFromEnd(token);
                 text.append(sToken);
                 text.append(" ");
             //System.out.println("stoken: '"+sToken+"' StrBuf: '"+text.toString()+"'");
          }

          if (AtEnd)
          {
                    str = text.toString();

          }
          else if (!gotToken)
          {
             line = rb.readLine();
             tokStrg = new StringTokenizer(line, ">", false);
          }
       }
       return (str.trim());
    }

    private boolean testEndPara(String token)
    {
      boolean foundEndPara = false;
      int tokenLen = token.length();
      if (token.charAt(0) == '<' )
      {
        if (tokenLen > 2)
        {
          if ( (token.charAt(1) == '/') &&
             (token.charAt(2) == 'P') )
          {
            foundEndPara = true;
          }
        }
      }
      else
      {
        String tag = null;
        String text = null;
        int loc = token.lastIndexOf('<');
        if (loc > -1)
        {
          tag = token.substring(loc,tokenLen);
          foundEndPara = testEndPara(tag);
        }
      }
      return(foundEndPara);
    }

    private String StripTagFromEnd(String token)
    {
      String tag = null;
      String text = null;
      int loc = token.lastIndexOf('<');
      if (loc > -1)
      {
         int len = token.length();
         tag = token.substring(loc,len);
         text = token.substring(0,loc);
      }
      else
      {
        text = token;
      }
      text = StripColon(text);
      return(text);
    }

    private String StripColon(String token)
    {
      String text = token;
      int tokenLen = token.length();
      if (tokenLen > 0)
      {
        if (token.charAt(0) == ':' )
        {
          text = token.substring(1,tokenLen);
        }
      }
      return(text);
    }

    public static void main(String[] args) {

        JFrame frame = new JFrame(" VNMR Software Design Document Viewer");

        frame.getContentPane().add( new DocViewer());

        frame.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                System.exit(0);
            }
        });

        frame.pack();
        frame.setVisible(true);
    }

}
