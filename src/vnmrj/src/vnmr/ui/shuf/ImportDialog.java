/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import javax.swing.JTable;
import javax.swing.table.AbstractTableModel;
import javax.swing.border.*;
import javax.swing.tree.*;
import javax.swing.event.TreeExpansionEvent;
import javax.swing.event.TreeExpansionListener;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import java.beans.*;
import java.net.*;

import vnmr.util.*;
import vnmr.ui.*;
import vnmr.bo.*;

/********************************************************** <pre>
 * Summary: Dialog box to allow importing of files.
 *
 *
 </pre> **********************************************************/
public class ImportDialog extends ModalDialog implements ActionListener,
                TreeExpansionListener {

    private static ImportDialog importDialog = null;
    protected JTreeTableImport treeTable;
    protected TreeTableModel treeTableModel;
    public ImportTableModel tableModel;
    protected JComboBox roots;
    protected JTable selected;
    //protected String importCMD;
    protected String root = "";
    ArrayList dataDirs;  // Directories in the form host:fullpath
    HashArrayList selectedfiles = new HashArrayList();
    // Keep all dir imported for rebuilding DB.
    ArrayList importedDirs;
    TreeExpansionListener tel;

    String userName = System.getProperty("user.name");
    //String userDir = System.getProperty("userdir");
    Vector parents;
    String lastdir;

    JButton add2listButton;
    JButton removeButton;
    JScrollPane listScroller;
    int dirSize;

    ShufDBManager dbManager = ShufDBManager.getdbManager();
    DBCommunInfo info = new DBCommunInfo();

    final String[] tableHeader = {Util.getLabel("_Full_path"), "bytes", "type"};
    JTextField totalEntries;
    JLabel stopButton;
    boolean stopPressed = false;
    final String totalEntryText = Util.getLabel("_Total_entries_to_import");
    private javax.swing.Timer timer;
    public final static int ONE_SECOND = 1000;

    private static class ThreadVar {
        private Thread thread;
        ThreadVar(Thread t) { thread = t; }
        synchronized Thread get() { return thread; }
        synchronized void clear() { thread = null; }
    }

    private ThreadVar threadVar;
    public static String defaultLookAndFeel;
    //protected Color m_bgColor = Global.BGCOLOR;

    /************************************************** <pre>
     * Summary: Constructor, Add label and text fields to dialog box
     *
     </pre> **************************************************/
    public ImportDialog(String title, String helpFile) {

        super(title);
        tel = this;
        m_strHelpFile = helpFile;

        //Create a timer.
        timer = new javax.swing.Timer(ONE_SECOND, new TimerListener());

        // persistence file ParentDirs contains the list of parent dirs
        // and the dimension of the split pane.
        readPersistence();

        // Read list of previously imported directories.  This list is
        // kept for the the purpose of recreating the DB when necessary.
        String curuser = System.getProperty("user.name");
        LoginService loginService = LoginService.getDefault();
        Hashtable userHash = loginService.getuserHash();
        User user = (User)userHash.get(curuser);
        importedDirs = FillDBManager.readImportedDirs(user);

        JLabel browserlabel = new JLabel(Util.getLabel("_Select_a_directory"), JLabel.LEFT);
//        browserlabel.setForeground(Color.black);

        root = (String) parents.elementAt(0);
        lastdir = root;

        makeRootsComboBox();

        JLabel cleanUp = new JLabel();
        cleanUp.setIcon(Util.getImageIcon("blank.gif"));

        cleanUp.setToolTipText("clean up directory list.");
        JPanel rootsPane = new JPanel();
        rootsPane.setLayout(new BoxLayout(rootsPane, BoxLayout.X_AXIS));
        rootsPane.add(browserlabel);
        rootsPane.add(Box.createRigidArea(new Dimension(20, 0)));
        rootsPane.add(roots);
        rootsPane.add(cleanUp);

        JLabel upDir = new JLabel();
        upDir.setIcon(Util.getImageIcon("blueUpArrow.gif"));

        JPanel pane = new JPanel(new BorderLayout());
        pane.add(rootsPane, BorderLayout.NORTH);
        pane.add(upDir, BorderLayout.WEST);

        //dataDirs contains dirs and files that are already in the Locator.
        dataDirs = getShuffDirs();

        treeTable = new JTreeTableImport();
        treeTable.setTreeTableFont(new Font("Dialog", Font.BOLD, 14));
        updateTree(root);

        JScrollPane spane = new JScrollPane(treeTable);
        spane.setPreferredSize(new Dimension(
                spane.getPreferredSize().width, 280));

        roots.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                JComboBox cb = (JComboBox)e.getSource();
                root = (String)cb.getSelectedItem();
                lastdir = root;
                updateTree(root);
            }
        });

        cleanUp.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                parents.clear();
                parents.add("/");
                root = (String)roots.getSelectedItem();
                lastdir = root;
                if(!root.equals("/")) parents.add(root);
                writePersistence();
            }
        });

        upDir.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                root = (String)roots.getSelectedItem();
                if(!root.equals(File.separator)) {
                    int last = root.lastIndexOf(File.separator);
                    if(last == 0) root = File.separator;
                    else root = root.substring(0, last);
                    lastdir = root;
                    if(!parents.contains(root)) parents.add(root);
                    roots.setSelectedItem(root);
                    updateTree(root);
                }
            }
        });

//        JLabel iconLabel1 = new JLabel();
//        iconLabel1.setIcon(Util.getImageIcon("open.gif"));
//        JLabel iconLabel2 = new JLabel();
//        iconLabel2.setIcon(Util.getImageIcon("middle.gif"));
//        iconLabel2.setText("already in the Locator");

        JPanel iconPane1 = new JPanel();
        iconPane1.setLayout(new BoxLayout(iconPane1, BoxLayout.X_AXIS));
//        iconPane1.add(iconLabel1);
        iconPane1.add(Box.createRigidArea(new Dimension(5, 0)));
//        iconPane1.add(iconLabel2);

//        JLabel iconLabel3 = new JLabel();
//        iconLabel3.setIcon(UIManager.getIcon("Tree.closedIcon"));
//        JLabel iconLabel4 = new JLabel();
//        iconLabel4.setIcon(UIManager.getIcon("Tree.leafIcon"));
//        iconLabel4.setText("not in the Locator");

        JPanel iconPane2 = new JPanel();
        iconPane2.setLayout(new BoxLayout(iconPane2, BoxLayout.X_AXIS));
//        iconPane2.add(iconLabel3);
        iconPane2.add(Box.createRigidArea(new Dimension(5, 0)));
//        iconPane2.add(iconLabel4);

        JPanel iconPane = new JPanel();
        iconPane.setLayout(new BoxLayout(iconPane, BoxLayout.Y_AXIS));
        iconPane.add(iconPane1);
        iconPane.add(iconPane2);

        add2listButton = new JButton(Util.getLabel("_Import_Add"));
        add2listButton.setBorder(new BevelBorder(BevelBorder.RAISED));

        JPanel iconButtonPane = new JPanel();
        iconButtonPane.setLayout(new BoxLayout(iconButtonPane, BoxLayout.X_AXIS));
        iconButtonPane.add(iconPane);
        iconButtonPane.add(Box.createRigidArea(new Dimension(30, 0)));
        iconButtonPane.add(add2listButton);

        JPanel upPane = new JPanel();
        upPane.setLayout(new BoxLayout(upPane, BoxLayout.Y_AXIS));
        upPane.setBorder(new CompoundBorder(
                        BorderFactory.createEtchedBorder(),
                        BorderFactory.createEmptyBorder(10,10,10,10)));
        upPane.add(pane);
        upPane.add(spane);
        upPane.add(Box.createRigidArea(new Dimension(0, 10)));
        upPane.add(iconButtonPane);

        pane.setAlignmentX(Component.CENTER_ALIGNMENT);
        spane.setAlignmentX(Component.CENTER_ALIGNMENT);
        iconButtonPane.setAlignmentX(Component.CENTER_ALIGNMENT);

        selected = new JTable();
        selected.setFont(new Font("Dialog", Font.BOLD, 14));
        initializeImportTable();

        selected.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                Point p = new Point(evt.getX(), evt.getY());
                int colIndex = selected.columnAtPoint(p);
                int rowIndex = selected.rowAtPoint(p);
                if(tableModel.isCellEditable(rowIndex, colIndex)) {
                        selected.editCellAt(rowIndex, colIndex);
                }
            }
        });

        listScroller = new JScrollPane(selected);
        listScroller.setPreferredSize(new Dimension(
                listScroller.getPreferredSize().width, 150));

        removeButton = new JButton(Util.getLabel("_Remove"));
        removeButton.setBorder(new BevelBorder(BevelBorder.RAISED));

        add2listButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                addToTable();
            }
        });


        removeButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                removeFromTable();
            }
        });

        JLabel entryLabel = new JLabel(Util.getLabel("_Total_entries_to_import"));
        totalEntries = new JTextField("0           ");
        totalEntries.setEditable(false);
        stopButton = new JLabel();
        stopButton.setIcon(Util.getImageIcon("stop.gif"));
        stopButton.setEnabled(false);

        stopButton.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                stopTimer();
                if(stopPressed) stopPressed = false;
                else stopPressed = true;
            }
        });

        JPanel entryPane = new JPanel();
        entryPane.add(entryLabel);
        entryPane.add(totalEntries);
        entryPane.add(stopButton);
        entryPane.add(Box.createRigidArea(new Dimension(20, 0)));
        entryPane.add(removeButton);

        JPanel loPane = new JPanel();
        loPane.setLayout(new BoxLayout(loPane, BoxLayout.Y_AXIS));
        loPane.setBorder(new CompoundBorder(
                        BorderFactory.createEtchedBorder(),
                        BorderFactory.createEmptyBorder(10,20,10,20)));

        loPane.add(listScroller);
        loPane.add(Box.createRigidArea(new Dimension(0, 10)));
        loPane.add(entryPane);

        listScroller.setAlignmentX(Component.CENTER_ALIGNMENT);
        entryPane.setAlignmentX(Component.CENTER_ALIGNMENT);

        // destroy the dialog if closed from Window menu.
        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent we) {

                writePersistence();
                if(timer.isRunning()) timer.stop();
                setVisible(false);
                setLookAndFeel(defaultLookAndFeel);
                destroy();
            }
        });

        JPanel mainPane = new JPanel();
        mainPane.setLayout(new BoxLayout(mainPane, BoxLayout.Y_AXIS));
        mainPane.setBorder(BorderFactory.createEmptyBorder(0,0,10,0));
        mainPane.add(upPane);
        mainPane.add(loPane);
        getContentPane().add(mainPane, BorderLayout.NORTH);

        // Set the buttons up with Listeners
        okButton.addActionListener(this);
        okButton.setActionCommand("ok");
        cancelButton.addActionListener(this);
        cancelButton.setActionCommand("cancel");
        helpButton.addActionListener(this);
        helpButton.setActionCommand("help");

        //m_bgColor = mainPane.getBackground();
        //setBackgroundColor(m_bgColor);
        //DisplayOptions.addChangeListener(this);

        // Make the frame fit its contents.
        pack();
    }

    public static ImportDialog getImportDialog(Component comp, String helpFile) {

        ImportDialog.defaultLookAndFeel = UIManager.getLookAndFeel().getID();

        ImportDialog.setLookAndFeel("Metal");

        if(importDialog == null) {
            importDialog = new ImportDialog(Util.getLabel("_Import_Dialog"), helpFile);

            // Set this dialog on top of the component passed in.
            importDialog.setLocationRelativeTo(comp);
        } else {
            // get up to date ShuffDirs.
            importDialog.dataDirs = importDialog.getShuffDirs();
            // reload the tree so it is up to date.
            //String currentRoot = (String)importDialog.roots.getSelectedItem();
            String currentRoot = importDialog.lastdir;
            importDialog.roots.setSelectedItem(currentRoot);
            importDialog.updateTree(currentRoot);

            importDialog.initializeImportTable();
            importDialog.totalEntries.setText("0           ");
        }

        return importDialog;
    }

    private void initializeImportTable() {

        selectedfiles.clear();

        String[][] data = new String[0][3];
        tableModel = new ImportTableModel(data, tableHeader);
        selected.setModel(tableModel);
        setTableLayout();
        //selected.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);

    }

    private void updateSelectedfiles() {

        startTimer();

        ArrayList files = treeTable.getSelectedFiles();
        for(int i=0; i<files.size(); i++) {
           String path = (String)files.get(i);
           if(IsOkPath(path) && !selectedfiles.containsKey(path)) {
             DirEntries dirEntries = new DirEntries(path);
           }
        }

        stopTimer();
        stopPressed = false;
        updateTable();
    }

    public void freeObj() {
        System.gc();
        System.runFinalization();
    }

    public void destroy() {
        removeAll();
        treeTable.tree.removeTreeExpansionListener(tel);
        treeTable = null;
        roots = null;
        selected = null;
        dataDirs = null;
        selectedfiles = null;
        tel = null;
        parents = null;
        add2listButton = null;
        removeButton = null;
        listScroller = null;
        dbManager = null;
        stopButton = null;
        totalEntries = null;
        timer = null;
        importDialog = null;
        this.dispose();
        System.gc();
        System.runFinalization();
    }

    public void keyPressed(KeyEvent e) {
        super.keyPressed(e);

    }

    /************************************************** <pre>
     * Summary: Listener for all buttons and the text field.
     *
     *
     </pre> **************************************************/
    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
        // OK
        if(cmd.equals("ok")) {
            if(selectedfiles.size() == 0) updateSelectedfiles();
            updateLocatorAndCloseWindow();

            setLookAndFeel(defaultLookAndFeel);

        }
        // Cancel
        else if(cmd.equals("cancel")) {
            writePersistence();
            setVisible(false);
            if(timer.isRunning()) timer.stop();
            setLookAndFeel(defaultLookAndFeel);
            freeObj();

        }
        // Help
        else if(cmd.equals("help")) {
            displayHelp();
        }

    }

    public void showDialogAndGetPaths() {

        // Show the dialog and wait for the results. (Blocking call)
        //showDialogWithThread();
        setVisible(true);

        // If I don't do this, and the user hits a 'return' on the keyboard
        // the dialog comes visible again.
        transferFocus();
    }

    public void treeExpanded(TreeExpansionEvent event) {
        TreePath treePath = event.getPath();
        int np = treePath.getPathCount();
        FileNode fn = (FileNode) treePath.getPathComponent(np-1);
        lastdir = fn.getPath();
        if(!parents.contains(lastdir)) parents.add(lastdir);
        roots.setSelectedItem(lastdir);
    }
    public void treeCollapsed(TreeExpansionEvent event) {
    }

    public void readPersistence() {

        String filepath =
         FileUtil.openPath("USER/PERSISTENCE/ParentDirs");

        parents = new Vector();

        if(filepath == null) {
            parents.add("/");
        } else {

          BufferedReader in;
          String line;
          String word;
          StringTokenizer tok;
          try {
              UNFile file = new UNFile(filepath);
              in = new BufferedReader(new FileReader(file));
              while ((line = in.readLine()) != null) {
                  if(!line.startsWith("#")) {// skip comments.
                      tok = new StringTokenizer(line, " :,\t\n");
                      word = tok.nextToken();
                      if(word.equals("Parent")) word = tok.nextToken();
                      if(word.equals("Directory")) word = tok.nextToken();
                      if(word.indexOf(File.separator) >= 0) parents.add(word);
                  }
              }
              in.close();
          } catch (Exception e) { }
        }
    }

    public void writePersistence() {

        String filepath =
         FileUtil.savePath("USER/PERSISTENCE/ParentDirs");

        FileWriter fw;
        PrintWriter os;
        try {
              UNFile file = new UNFile(filepath);
              fw = new FileWriter(file);
              os = new PrintWriter(fw);
              os.println("Parent Directory " + lastdir);
              for(int i=0;i<parents.size();i++) {
                  String s = (String) parents.elementAt(i);
                  if(!s.equals(lastdir)) os.println("Parent Directory " + s);
              }

              os.close();
        } catch(Exception er) {
             Messages.postError("Problem creating persistence file, " +
                                filepath);
             Messages.writeStackTrace(er);
        }
    }

    /******************************************************************
     * Summary: Return list of all entries in the shuffler
     *          for this user.
     *
     *
     *****************************************************************/

    private ArrayList getShuffDirs() {

        ArrayList shufDirs = new ArrayList();
        //String userdir;
        //String sysdir;
        String dir;
        User user = null;
        String hostFullpath;
/*
        Hashtable userHash = User.readuserListFile();
*/
        LoginService loginService = LoginService.getDefault();
        Hashtable userHash = loginService.getuserHash();

        for(Enumeration en = userHash.elements(); en.hasMoreElements(); ) {
            user = (User) en.nextElement();

            ArrayList dirs = user.getDataDirectories();
// Stop tagging directories just because they are in dataDir.
//            for(int i=0; i<dirs.size(); i++)
//                shufDirs.add(dirs.get(i));

            //userdir = user.getVnmrsysDir();
            //dir = userdir.concat(File.separator + "exp");
            //shufDirs.add(dir);
            //dir = userdir.concat(File.separator + "shims");
            //shufDirs.add(dir);
            //dir = userdir.concat(File.separator + "templates" + File.separator +
            //                     "layout");
            //shufDirs.add(dir);
            dir=FileUtil.userDir(user,"SHIMS");
            if(dir !=null) {
                // If windows, this path may have mixed forward and back
                // slashes.  Convert to all forward slashes.
                dir = UtilB.windowsPathToUnix(dir);
                // We need host:fullpath where fullpath is the path on the
                // host given, namely, the same as host_fullpath in the DB.
                hostFullpath = MountPaths.getCanonicalHostFullpath(dir);
                shufDirs.add(hostFullpath);
            }
            dir=FileUtil.userDir(user,"LAYOUT");
            if(dir !=null) {
                // If windows, this path may have mixed forward and back
                // slashes.  Convert to all forward slashes.
                dir = UtilB.windowsPathToUnix(dir);
                hostFullpath = MountPaths.getCanonicalHostFullpath(dir);
                shufDirs.add(hostFullpath);
            }
        }
        //sysdir = System.getProperty("sysdir");
        //dir = sysdir.concat(File.separator + "user_templates" +
        //                    File.separator + "layout");
        //shufDirs.add(dir);
        //dir = sysdir.concat(File.separator + "shims");
        //shufDirs.add(dir);
        dir=FileUtil.vnmrDir("SYSTEM/SHIMS");
        if(dir !=null) {
            hostFullpath = MountPaths.getCanonicalHostFullpath(dir);
            shufDirs.add(hostFullpath);
        }
        dir=FileUtil.vnmrDir("SYSTEM/SHIMS");
        if(dir !=null) {
            hostFullpath = MountPaths.getCanonicalHostFullpath(dir);
            shufDirs.add(hostFullpath);
        }
        ArrayList allFiles = new ArrayList();
        for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
            // Get full list of host_fullpath entry from DB
            ArrayList files = dbManager.attrList.getAttrValueList(
                                          "host_fullpath",Shuf.OBJTYPE_LIST[i]);
            for(int j=0; j<files.size(); j++) {
                String path = (String)files.get(j);
                // create a local copy
                allFiles.add(path);
            }
        }

        // Go through list from DB and remove the ones in the shufDirs list
        // of directories.  That is, we want all host_fullpath that are
        // NOT in any of the shufDirs directories.
        for(int i=0; i<allFiles.size(); i++)
            for(int j=0; j<shufDirs.size(); j++)
                if(((String) allFiles.get(i)).startsWith(
                                                (String) shufDirs.get(j)))
                        allFiles.set(i," ");

        // Now add files that were outside of shufDirs to the shufDirs list
        for(int i=0; i<allFiles.size(); i++)
            if(!allFiles.get(i).equals(" ")) {
                shufDirs.add((String) allFiles.get(i));
            }
        if(DebugOutput.isSetFor("ShufDirs")) {
            if(user != null)
                Messages.postDebug("Shuf Dirs for " + user.getAccountName() +
                                   "\n" + shufDirs);
        }
        return shufDirs;
    }

    private void updateLocatorDB() {
        // Save list of types encountered, for updating attribute list when
        // finished.
        ArrayList objTypes=new ArrayList();

        // Get the host name
        String host = "";
        try {
            InetAddress inetAddress = InetAddress.getLocalHost();
            host = inetAddress.getHostName();
        }
        catch(Exception e) {
            Messages.postError("Problem getting HostName");
        }
        int filesAdded = 0;
        ArrayList allItems = new ArrayList();
        ArrayList items;
        startTimer();
        boolean recursive;

        while(selectedfiles.size() > 0) {
            String path = (String) selectedfiles.getKey(0);
            String type = (String) ((ArrayList)selectedfiles.get(path)).get(1);
            String user = userName;
            String fullpath="";
            String hostFullpath="";

            // Save list of types encountered, for updating attribute list when
            // finished.
            if(!objTypes.contains(type))
               objTypes.add(type);

            if(!stopPressed) {
                if(type.length() > 0 && type.indexOf("?") == -1) {
                    if(path.endsWith("/")) recursive = true;
                    else recursive = false;

                    try {
                        if(type.equals(Shuf.DB_VNMR_DATA) ||
                                 type.equals(Shuf.DB_VNMR_PAR)) {

                            items = dbManager.fillATable(type, path, user,
                                                         recursive, info);
                            // Add path to list for recreating the DB.
                            if(info.numFilesAdded > filesAdded) {
                                filesAdded = info.numFilesAdded;
                                for(int j=0; j<items.size(); j++) {
                                    try {
                                        UNFile file = (UNFile)items.get(j);
                                        fullpath = file.getCanonicalPath();
                                    }
                                    catch (Exception e) {
                                        Messages.postError("Problem " +
                                                    "getting fullpath for " +
                                                     path);
                                        Messages.writeStackTrace(e);
                                        return;
                                    }
                                    hostFullpath = host +":"+ fullpath;
                                    if(!dataDirs.contains(hostFullpath))
                                        dataDirs.add(hostFullpath);
                                    if(!allItems.contains(hostFullpath))
                                        allItems.add(hostFullpath);
                                }
                            }
                        }
                        else {
                            // If it is a known locator type, add this item,
                            // then add the directory.
                            String objType = dbManager.getType(path);
                            if(objType.equals(type)) {
                                UNFile file = new UNFile(path);
                                String filename = file.getName();
                                dbManager.addFileToDB(type, filename, path,
                                                      userName, true);

                                // the attrList must be updated, or this item
                                // will not show as in the DB.
                                dbManager.attrList.updateListFromSingleRow(
                                                  path, host, type, false);

                            }
                            items = dbManager.fillATable(type, path, user,
                                                         recursive, info);
                           if(info.numFilesAdded > filesAdded) {
                                filesAdded = info.numFilesAdded;
                                for(int j=0; j<items.size(); j++) {
                                    try {
                                        UNFile file = (UNFile)items.get(j);
                                        fullpath = file.getCanonicalPath();
                                    }
                                    catch (Exception e) {
                                            Messages.postError("Problem " +
                                                       "getting fullpath for " +
                                                        path);
                                        Messages.writeStackTrace(e);
                                        return;
                                    }
                                    hostFullpath = host +":"+ fullpath;
                                    if(!dataDirs.contains(hostFullpath))
                                        dataDirs.add(hostFullpath);
                                    if(!allItems.contains(hostFullpath))
                                        allItems.add(hostFullpath);
                                }
                            }
                        }
                        // Do not add to importedDirs if this directory is
                        // in the users 'dataDirectories' list.
                        LoginService loginService = LoginService.getDefault();
                        Hashtable userHash = loginService.getuserHash();
                        String curuser = System.getProperty("user.name");
                        User usr = (User)userHash.get(curuser);
                        ArrayList dirs = usr.getDataDirectories();
                        // Both path and dirs are in the form of fullpath
                        if(!treeTable.isLocatorDir(path, dirs)) {
                            // Add path to list for recreating the DB.
                            importedDirs.add(new String(path));
                        }

                    }
                    catch (Exception e) {
                        selectedfiles.remove(path);
                        continue;
                    }
                }
            }
            else {
                stopTimer();
                stopPressed = false;
                updateTable();
                try{
                    updateLocatorInfo(allItems);
                } catch (Exception e) {interrupt(); return; }

                interrupt();
                return;
            }
            
            selectedfiles.remove(path);
            updateTable();

            // Update attrList for this file.
            // last arg says not to do tags.  There could not be any
            // tags in a newly imported file so don't waste the time.
            dbManager.attrList.updateListFromSingleRow(fullpath, host, type,
                                                       false);
        }


        stopTimer();
        stopPressed = false;
        writePersistence();

        // Write dir list for recreating the DB
        writeImportedDirs();

        setVisible(false);
        try{
            updateLocatorInfo(allItems);
        } catch (Exception e) { }
        freeObj();
    }

    private void updateLocatorInfo(ArrayList allItems) {

        int filesAdded = allItems.size();

        // info.numFilesAdded is total number of files.
        // while as filesAdded is the number of files last added.

        System.err.println("\nNumber of Files added = " + filesAdded);

        if(filesAdded > 0) {

            for(int i=0; i<allItems.size(); i++)
                System.err.println((String) allItems.get(i));

            SessionShare sshare = ResultTable.getSshare();
            StatementHistory history = sshare.statementHistory();
            history.updateWithoutNewHistory();

            ResultTable resultTable = Shuffler.getresultTable();
            resultTable.setHighlightSelections(allItems);

            // Update the menus
            Hashtable statement = history.getCurrentStatement();
            StatementDefinition curStatement;
            curStatement = sshare.getCurrentStatementType();
            curStatement.updateValues(statement, true);
        }
    }

    private boolean IsOkPath(String path) {
        // root /, sysdir, user home directories and files are already
        // in the Locator are not valid path.

        String userHome = System.getProperty("user.home");

        if(path.equals("/") ||
           path.equals(userHome) ||
           path.equals(System.getProperty("sysdir")) ||
           path.equals(userHome.substring(0,userHome.indexOf(userName)))) {
                Messages.postError("Cannot import " +path+ " to the Locator.");
                return false;
        }
/*
        Hashtable userHash = User.readuserListFile();
*/
        LoginService loginService = LoginService.getDefault();
        Hashtable userHash = loginService.getuserHash();

        for(Enumeration en = userHash.elements(); en.hasMoreElements(); ) {
            User user = (User) en.nextElement();
            String userhome = user.getHomeDir();
            if(path.equals(userhome)) {
                Messages.postError("Cannot import user home directories to the Locator.");
                return false;
            }
        }


        // The path required for this test is the host:fullpath
        String hostFullpath = MountPaths.getCanonicalHostFullpath(path);
        if(treeTable.isLocatorDir(hostFullpath, dataDirs)) {
            return false;
        }
        return true;
    }


    class DirEntries {
        protected String path;
        protected HashArrayList entries = new HashArrayList();
        boolean complete = true;

        public DirEntries(String p) {
            path = p;
            UNFile file = new UNFile(path);

            //if(file.isFile() && isLocatorFile(path, file.getName())) {
            if(file.isFile()) {
                ArrayList size = new ArrayList();
		dirSize = 0;
                dirSize = getDirSize(file);
                size.add(String.valueOf(dirSize));
                String type = FillDBManager.getType(path, file.getName());
                if(!type.equals("?")) {
                    size.add(type);
                    entries.put(path,size);
                    if(IsOkPath(path) && !selectedfiles.containsKey(path)) {
                        selectedfiles.put(path, size);
                        //updateTable();
                    }
                }
            } else if(!file.isFile()) countFiles(file);

        }

        private void countFiles(File file) {

        //stop is set true when stopButton is pressed
        //recursively count the files if stopButton is not pressed.
         if(!stopPressed) {
           File[] children = file.listFiles();
           if(children != null) {
                String parent="";
                // Since UNFile.getCanonicalPath() always returns a unix
                // style path, we need to add a unix file separator even
                // if we are on a windows machine.
                try {
                    parent = file.getCanonicalPath() + "/";
               }
                catch (Exception e) {
                    Messages.writeStackTrace(e);
                }
                if(isLocatorData(parent, file.getName())) {
                        ArrayList size = new ArrayList();
 			dirSize = 0;
		        dirSize = getDirSize(file);
                        size.add(String.valueOf(dirSize));
                        String type = FillDBManager.getType(parent,
                                                            file.getName());
                        if(!type.equals("?")) {
                            size.add(type);
                            entries.put(parent, size);
                            if(IsOkPath(parent) && !selectedfiles.containsKey(parent)) {
                                selectedfiles.put(parent, size);
                                //updateTable();
                            }
                        }
                } else {
                    for(int i = 0; i < children.length; i++) {
                        String child="";
                        try {
                            child = children[i].getCanonicalPath();
                        }
                        catch (Exception e) {
                            Messages.writeStackTrace(e);
                        }

                        if(children[i].isFile() && isLocatorFile(child,children[i].getName())) {
                            ArrayList size = new ArrayList();
 			    dirSize = 0;
		            dirSize = getDirSize(children[i]);
                            size.add(String.valueOf(dirSize));
                            String type = FillDBManager.getType(child,
                                                      children[i].getName());
                            if(!type.equals("?")) {
                                size.add(type);
                                entries.put(child, size);
                                if(IsOkPath(child) && !selectedfiles.containsKey(child)) {
                                    selectedfiles.put(child, size);
                                    //updateTable();
                                }
                            }
                    } else countFiles(children[i]);
                  }
                }
            }
          } else {
        //return if stopButton is pressed.
                complete = false;
                interrupt();
                return;
          }
        }

        protected int getDirSize(File file) {
           if(file == null) {
		dirSize = 0;
	   	return dirSize;
	   }
           if(file.isFile()) {
		dirSize = (int)file.length();
	   	return dirSize;
	   }

           File[] children = file.listFiles();
           if(children != null) {
               for(int i = 0; i < children.length; i++) {
                    if(children[i].isFile()) {
			dirSize += children[i].length();
                    } else {
			getDirSize(children[i]);
		    }
                }
            }
	    return dirSize;
        }

        protected boolean isLocatorData(String parent, String name) {

            // only vnmrData, studies, automation and shims are imported
            //as directories.

            if(name.endsWith(Shuf.DB_FID_SUFFIX) ||
			name.endsWith(Shuf.DB_REC_SUFFIX) ||
			name.endsWith(Shuf.DB_REC_SUFFIX.toLowerCase()) ||
                          name.endsWith(Shuf.DB_PAR_SUFFIX))
                    return true;

            if(name.equals(Shuf.DB_SHIMS))
                return true;

            // Studies and automation are harder, since they have no extension.

            // We need to see if the studypar file exist inside of this
            // directory.
            UNFile studyfile = new UNFile(parent +"studypar");
            if(studyfile.exists())
                return true;

            // We need to see if the autopar file exist inside of this
            // directory.
            UNFile autofile = new UNFile(parent +"autopar");
            if(autofile.exists())
                return true;

            return false;
        }

        protected boolean isLocatorFile(String path, String name) {

            String dirName = path;
            int last = path.lastIndexOf(File.separator, path.length()-1);
            int second = path.lastIndexOf(File.separator, last-1);
            if(last != -1 && second != -1 && last > second+1)
                dirName = path.substring(second+1, last);

            // Test for prefix or suffix.  This will pick up protocols also
            if(name.endsWith(Shuf.DB_PANELSNCOMPONENTS_SUFFIX)) return true;
            else if(dirName.equals(Shuf.DB_SHIMS) ||
                    name.endsWith("." + Shuf.DB_SHIMS))
                return true;
            return false;
        }

        protected long getCount() {
            return (long) entries.size();
        }
    }

    private long TotalEntryCount() {

        if(selectedfiles == null) return -1;

        return selectedfiles.size();
    }

    class ImportTableModel extends AbstractTableModel {

        final String[][] data;

        final String[] columnNames;


        public ImportTableModel(String[][] d, String[] c) {
            super();
            data = d;
            columnNames = c;
        }

        public int getColumnCount() {
            return columnNames.length;
        }

        public int getRowCount() {
            return data.length;
        }

        public String getColumnName(int col) {
            if(col >= columnNames.length) return "";
            else return columnNames[col];
        }

        public Object getValueAt(int row, int col) {
            if(row >= data.length || col >= data[row].length)
	    return "";
	    else return data[row][col];
        }

        public boolean isCellEditable(int row, int col) {
            if(col >= columnNames.length) return false;
            if(columnNames[col] == "type") return true;
            else return false;
        }

    }

    private void setTableLayout() {

        selected.getColumnModel().getColumn(0).setMinWidth(160);
        selected.getColumnModel().getColumn(0).setPreferredWidth(160);
        selected.getColumnModel().getColumn(1).setPreferredWidth(60);
        selected.getColumnModel().getColumn(1).setPreferredWidth(60);
        selected.sizeColumnsToFit(0);
        //selected.repaint();
    }

    /**
     * The actionPerformed method in this class
     * is called each time the Timer "goes off".
     */
    class TimerListener implements ActionListener {
        public void actionPerformed(ActionEvent evt) {
                stopButton.setEnabled(true);
                totalEntries.setText(String.valueOf(TotalEntryCount()));
        }
    }

    private void updateTable() {

                String[][] newlist = new String[selectedfiles.size()][3];
                for(int i=0; i<selectedfiles.size(); i++) {
                   newlist[i][0] = (String)selectedfiles.getKey(i);
                   newlist[i][1] =
                        (String)((ArrayList)selectedfiles.get(newlist[i][0])).get(0);
                   newlist[i][2] =
                        (String)((ArrayList)selectedfiles.get(newlist[i][0])).get(1);
                }
                tableModel = new ImportTableModel(newlist, tableHeader);
                selected.setModel(tableModel);
                setTableLayout();
                listScroller.repaint();

                totalEntries.setText(String.valueOf(TotalEntryCount()));
    }

    private void removeFromTable() {

                int[] rows = selected.getSelectedRows();
                for(int i=0; i<rows.length; i++) {
                    String sel = (String) selected.getValueAt(rows[i], 0);
                    for(int j=0; j<selectedfiles.size(); j++) {
                        String key = (String)selectedfiles.getKey(j);
                        if(key.equals(sel))
                            selectedfiles.remove(key);
                    }
                }
                totalEntries.setText(String.valueOf(TotalEntryCount()));
                updateTable();
    }

    private void updateLocatorAndCloseWindow() {

        Runnable updateFiles = new Runnable() {
            public void run() {
                try {
                    updateLocatorDB();
                } finally {
                    threadVar.clear();
                }
            }
        };

        Thread t = new Thread(updateFiles);
        threadVar = new ThreadVar(t);
        t.start();
    }

    private void addToTable() {

        Runnable updateFiles = new Runnable() {
            public void run() {
                try {
                    updateSelectedfiles();
                } finally {
                    threadVar.clear();
                }
            }
        };

        Thread t = new Thread(updateFiles);
        threadVar = new ThreadVar(t);
        t.start();
    }

    /**
     * A new method that interrupts the thread.
     */
    public void interrupt() {
        Thread t = threadVar.get();
        if (t != null) {
            t.interrupt();
        }
        threadVar.clear();
    }

    public static void setLookAndFeel(String lookAndfeel) {

        // JavaLookAndFeel: Metal.
        // MotifLookAndFeel: CDE/Motif.

        lookAndfeel = lookAndfeel.toLowerCase();

        /*************
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
        **********/
    }

    private void stopTimer() {

        if(timer.isRunning()) timer.stop();
        stopButton.setEnabled(false);

        add2listButton.setEnabled(true);
        removeButton.setEnabled(true);
        okButton.setEnabled(true);
        cancelButton.setEnabled(true);
    }

    private void startTimer() {

        if(!timer.isRunning()) timer.start();

        //TimerListener will enable stopButton after 1 sec.

        add2listButton.setEnabled(false);
        removeButton.setEnabled(false);
        okButton.setEnabled(false);
        cancelButton.setEnabled(false);

    }

    private void updateTree(String root) {

        System.gc();
        System.runFinalization();
        setLookAndFeel("Metal");
        treeTableModel = new FileSystemModel(root);
        treeTable .setTreeTableModel(treeTableModel, dataDirs);
        treeTable.tree.addTreeExpansionListener(tel);
   }

   private void makeRootsComboBox() {

        roots = new JComboBox(parents);
        roots.setSelectedItem(root);
        roots.setEditable(true);
        //roots.setLightWeightPopupEnabled(false);
    }



    /**************************************************
     * Summary: Write out the list of imported directories.
     *
     *  This file is not used by this class, but is kept up to date
     *  for use when the DB needs to be recreated.
     **************************************************/
    public void writeImportedDirs() {

        String filepath = FileUtil.savePath("USER/PERSISTENCE/ImportedDirs");

        FileWriter fw;
        PrintWriter os;
        try {
            UNFile file = new UNFile(filepath);
            fw = new FileWriter(file);
            os = new PrintWriter(fw);
            os.println("Imported Directories and Files");
            for(int i=0;i<importedDirs.size();i++) {
                String s = (String) importedDirs.get(i);
                os.println(s);
            }

            os.close();
        }
        catch(Exception er) {
            Messages.postError("Problem creating  " + filepath);
            Messages.writeStackTrace(er);
        }
    }
}
