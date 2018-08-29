/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.part11;

import java.io.*;
import java.awt.*;
import java.util.*;
import javax.swing.*;
import java.awt.event.*;
import javax.swing.table.*;
import javax.swing.border.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.admin.util.*;
import vnmr.admin.ui.*;

public class Audit extends JSplitPane
{
    private JPanel m_locator;
    private ComboFileTable m_audit;
    private FileTable m_auditTable;
    private FileTable m_locatorTable;
    private FileMenu m_locatorMenu;
    private Vector m_paths1 = new Vector();
    private Vector m_paths2 = new Vector();
    private String m_label1 = null;
    private String m_label2 = null;
    private String m_vnmrCmd = null;
    protected boolean m_bInit = true;
    protected boolean m_bNew = false;
    public static Hashtable flagtable = new Hashtable();

    private int m_type;
    private int m_process;
    private ComboFileTable m_locComboFileTable;
    private JButton m_locCopyButton;
    private JButton m_locUpdateButton;
    private JButton m_locCancelButton;
    private int m_nRowIndex = 1;

    private String m_username = WUtil.getCurrentAdmin();

    public Audit() {
    super();
        setOpaque(false);
    m_locator = new JPanel();
    m_audit = new ComboFileTable();
    m_auditTable = new FileTable();
    m_locatorTable = new FileTable();
    m_locatorMenu = new FileMenu();
    }

    public Audit(String path1, String label1, String path2, String label2) {
    super();
        setOpaque(false);

        Vector paths1 = new Vector();
    paths1.addElement(path1);
        Vector paths2 = new Vector();
        paths2.addElement(path2);
    makeAudit(paths1, label1, paths2, label2, false);
    }

    public Audit(Vector paths1, String label1, Vector paths2, String label2) {
    super();
        setOpaque(false);

    makeAudit(paths1, label1, paths2, label2, false);
    }

    public void makeAudit(Vector paths1, String label1, Vector paths2,
                                String label2, boolean bCreateNew) {

    copyPropertyfiles();
        writeSystemAuditMenu();
        writeRecordAuditMenu();

    m_type = 0;
        m_paths1.clear();
        m_paths2.clear();
        m_bNew = bCreateNew;
        for(int i=0; i<paths1.size(); i++) {
        String str = (String)paths1.elementAt(i);
        if(str.endsWith("RecordAuditMenu") ||
        str.endsWith("RecordAuditMenu/")) m_type = 1;
            m_paths1.addElement(str);
    }
        for(int i=0; i<paths2.size(); i++)
            m_paths2.addElement(paths2.elementAt(i));
        m_label1 = label1;
        m_label2 = label2;

    makeLocator();
        m_audit = new ComboFileTable();

        //setOrientation(JSplitPane.HORIZONTAL_SPLIT);
        setOrientation(JSplitPane.VERTICAL_SPLIT);
        setTopComponent(m_locator);
        setBottomComponent(m_audit);
        setOneTouchExpandable(true);

        if (getParent() != null)
        {
            Dimension pSize = getParent().getPreferredSize();

            setPreferredSize(pSize);
            int div = (pSize.height)/2;
            setDividerLocation(div);
        }
        else
        {
            setDividerLocation(0.5);
        }

        m_locatorTable = m_locComboFileTable.getTable();
    if(m_locatorTable.getRowCount() > 0)
        m_locatorTable.setRowSelectionInterval(0,0);
        m_locatorTable.setBackground(Color.white);

        m_locatorTable.addMouseListener(new MouseAdapter() {

            public void mouseReleased(MouseEvent evt) {
                writeTableSelection2File();
        }

            public void mouseClicked(MouseEvent evt) {

                int clicks = evt.getClickCount();
                Point p = new Point(evt.getX(), evt.getY());
                locatorTableMouseAction(p, clicks);
            }
        });

        m_locatorTable.addKeyListener(new KeyAdapter()
        {
            public void keyReleased(KeyEvent e)
            {
                int nKey = e.getKeyCode();
                if (nKey == KeyEvent.VK_DOWN || nKey == KeyEvent.VK_UP)
                {
                    int nRowIndex = (nKey == KeyEvent.VK_DOWN) ? m_nRowIndex + 1 :
                                    m_nRowIndex - 1;
                    locatorTableMouseAction(nRowIndex, 1);
                    writeTableSelection2File();
                }
            }
        });

        m_locatorMenu = m_locComboFileTable.getFileMenu();

        m_locatorMenu.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {

                locatorMenuAction();
            }
        });
        if (m_bNew)
        {
            makeAudit();
        }
        else
        {
            updateAudit();
        }
    }

    private void makeLocator() {

        m_locator = new JPanel();
        m_locator.setLayout(new BoxLayout(m_locator, BoxLayout.Y_AXIS));
        m_locComboFileTable = new ComboFileTable(m_paths1, m_label1);
        JToolBar locatorLower = new JToolBar();
        if(m_type == 1) m_locCopyButton = new JButton("Copy record(s)");
        else m_locCopyButton = new JButton("Copy audit trail(s)");
        if(m_type == 1) m_locUpdateButton = new JButton("Check record(s)");
        else m_locUpdateButton = new JButton("Update unix audit trail(s)");
    m_locCancelButton = new JButton("Cancel");
    m_locCancelButton.setEnabled(false);

    m_locCopyButton.setBorder(new BevelBorder(BevelBorder.RAISED));
    m_locUpdateButton.setBorder(new BevelBorder(BevelBorder.RAISED));
    m_locCancelButton.setBorder(new BevelBorder(BevelBorder.RAISED));

    locatorLower.setLayout(new WGridLayout(1, 3, 10, 10));
    locatorLower.add(m_locCopyButton);
    // Only add the Check records button, the Update unix audit trail
    // is related to bsm for unix
    if(m_type == 1) 
        locatorLower.add(m_locUpdateButton);
    locatorLower.add(m_locCancelButton);

    m_locator.add(locatorLower);
    m_locator.add(m_locComboFileTable);

    m_locCopyButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                locCopyButtonAction();
            }
        });

    m_locUpdateButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {

                locUpdateButtonAction();
            }
        });

    m_locCancelButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {

                locCancelButtonAction();
            }
        });
    }

    public void locCopyButtonAction() {

        writeTableSelection2File();
        m_locCopyButton.setEnabled(false);
        m_locUpdateButton.setEnabled(false);
        m_locCancelButton.setEnabled(true);
	m_process = 1;

        new Thread(new Runnable()
        {
            public void run()
            {
                String[] cmd = {WGlobal.SHTOOLCMD, "-c", "auditcp -l"};
                WMessage msg = WUtil.runScript(cmd);
                if(msg.isNoError()) {
                    String str = msg.getMsg();
                    Messages.postInfo(str);
                    m_locComboFileTable.updateFileTable();
                    //m_audit.updateFileTable();
            // need to update both menu and table
            updateAudit();
                }
                m_locCopyButton.setEnabled(true);
                m_locUpdateButton.setEnabled(true);
        	m_locCancelButton.setEnabled(false);
		m_process = 0;
            }
        }).start();
    }

    public void updateFlagtable(String path) {

        BufferedReader in;
        String inLine;

        try {
          in = new BufferedReader(new FileReader(path));
          while ((inLine = in.readLine()) != null) {
            if (inLine.length() > 1 && !inLine.startsWith("#")) {
                inLine.trim();
                StringTokenizer tok = new StringTokenizer(inLine, " \n", false);
                if (tok.countTokens() > 1) {
                    String file = tok.nextToken();
                    String flag  = tok.nextToken();
		    if(file != null && flag != null)
                    flagtable.put(file, flag);
                }
            }
          }

        } catch(IOException e) { System.err.println("ERROR: read " + path);}

        return;
    }

    public void locUpdateButtonAction() {

        String[] cmd = {WGlobal.SHTOOLCMD, "-c", null};
        if(m_type == 1) {
	    m_process = 2;
            //cmd[2] = WGlobal.SUDO + WGlobal.SBIN + "chchsums -R";
	    
    	    writeMenuSelection2File();
	    String outpath = 
              FileUtil.savePath("USER/PERSISTENCE/audit.flags");
	    if(outpath == null) outpath = "/tmp/audit.flags";

            cmd[2] = "/vnmr/bin/chchsums -R " + outpath;
        } else {
	    m_process = 3;
            cmd[2] = WGlobal.SUDO + WGlobal.SBIN + "aupurge /tmp/uaut";
        }

        if(cmd[2] == null) return;
        updateButtonAction(cmd);
    }

    protected void updateButtonAction(final String[] cmd)
    {
        m_locCopyButton.setEnabled(false);
        m_locUpdateButton.setEnabled(false);
    m_locCancelButton.setEnabled(true);

        new Thread(new Runnable()
        {
            public void run()
            {
                WMessage msg = WUtil.runScript(cmd);
                if(msg.isNoError()) {
                    String str = msg.getMsg();
                    Messages.postInfo(str);
                    // get outpath if exists.
                    StringTokenizer tok = new StringTokenizer(cmd[2], " \n", false);
                    String outpath = null; 
                    if (tok.countTokens() > 2) {
                        tok.nextToken();
                        tok.nextToken();
                        outpath = tok.nextToken();
                    }
		    if(outpath != null && cmd[2].indexOf("chchsums") != -1) {
			updateFlagtable(outpath.trim());
		    } else if(outpath != null && cmd[2].indexOf("aupurge") != -1) {
			String destpath = Audit.getAuditDir() + "/uaudit"; 
			if(destpath != null) {
			   cmd[2] = "mkdir -p " + destpath;  
                           WUtil.runScript(cmd);
			   cmd[2] = "cp -rf " + outpath +"/*.cond " + destpath;  
                           WUtil.runScript(cmd);
                        }
                    } 
                    m_locComboFileTable.updateFileTable();
                    //m_audit.updateFileTable();
            // need to update both menu and table
                        updateAudit();
                }
                m_locCopyButton.setEnabled(true);
                m_locUpdateButton.setEnabled(true);
        	m_locCancelButton.setEnabled(false);
	        m_process = 0;
            }
        }).start();
    }

    public void locCancelButtonAction() {


        String path =
         FileUtil.openPath("USER/PERSISTENCE/killAuditProcess");

        String[] cmd = {WGlobal.SHTOOLCMD, "-c", null};

        if(m_process == 1) {
            if(path != null) {
                cmd[2] = path;
                WUtil.runScript(cmd);
        System.err.println("cmd " + cmd[2]);
            }
        } else if(m_process == 2) {
            cmd[2] = WGlobal.SUDO + WGlobal.SBIN + "killch";

        System.err.println("cmd " + cmd[2]);
            WUtil.runScript(cmd);
        } else if(m_process == 3) {
            cmd[2] = WGlobal.SUDO + WGlobal.SBIN + "killau";
            WUtil.runScript(cmd);
        System.err.println("cmd " + cmd[2]);
        }

        if(path != null) {
            String[] cmd2 = {WGlobal.SHTOOLCMD, "-c", "rm -f " + path};
            WUtil.runScript(cmd2);
        }
    }

    public void locatorMenuAction() {
    m_locComboFileTable.updateFileTable();
    if(m_locatorTable.getRowCount() > 0)
    m_locatorTable.setRowSelectionInterval(0,0);
    if (m_bNew)
    {
        makeAudit();
    }
    else
    {
        updateAudit();
    }
    writeAuditingAudit();
    }

    public void locatorTableMouseAction(Point p, int clicks) {

        if(m_locComboFileTable == null) return;

        int nRowIndex = m_locComboFileTable.getTable().rowAtPoint(p);
        //int rowIndex = m_locatorTable.rowAtPoint(p);

        locatorTableMouseAction(nRowIndex, clicks);
    }

    protected void locatorTableMouseAction(int nRow, int clicks)
    {
        if(nRow != -1) {
            m_nRowIndex = nRow;
            if (clicks >= 2) {
                sendLocatorCmd();
            } else if (clicks == 1) {
                setLocatorCmd();
                if (m_bNew)
                {
                    makeAudit();
                }
                else
                {
                    updateAudit();
                }
            }
        }
    }

    private void setLocatorCmd() {

        String str = m_locatorTable.getRowValue(m_locatorTable.getSelectedRow());
        String type = m_locatorTable.getFileTableModel().getType();
        if(!str.equals(null) && type.equals("records"))
            m_vnmrCmd = "RT('" + str + "')\n";
        else m_vnmrCmd = null;
    }

    private void sendLocatorCmd() {

        String type = m_locatorTable.getFileTableModel().getType();
        if(type.equals("records") && m_vnmrCmd != null) {
       AppIF appIf = Util.getAppIF();
       if(appIf != null) appIf.sendCmdToVnmr(m_vnmrCmd);
    }
    }

    public void makeAudit() {

    int row = -1;
    if(m_locatorTable != null)
    row = m_locatorTable.getSelectedRow();

    if(row < 0) return;

    boolean flag = m_locatorTable.isRowFlaged(row);
    String path = m_locatorTable.getRowValue(row);
    String type = m_locatorMenu.getSelectedType();

    FileMenu auditMenu = m_audit.getFileMenu();

    int menuIndex = auditMenu.getSelectedIndex();
    if(menuIndex < 0) menuIndex = 0;

    writeAuditPaths(path, type, flag);

    m_audit.removeAll();
    m_audit.makeComboFileTable(m_paths2, m_label2, menuIndex);

    m_auditTable = m_audit.getTable();
    m_auditTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

    remove(m_audit);
    setBottomComponent(m_audit);

    if (getParent() != null)
    {
        Dimension pSize = getParent().getPreferredSize();

        setPreferredSize(pSize);
        int div = (pSize.height)/2;
        setDividerLocation(div);
    }
    else
    {
        setDividerLocation(0.5);
    }
    writeTableSelection2File();
    writeMenuSelection2File();
    }

    public void updateAudit() {

    int row = -1;
    if(m_locatorTable != null)
    row = m_locatorTable.getSelectedRow();

    if(row < 0) return;

    boolean flag = m_locatorTable.isRowFlaged(row);
    String path = m_locatorTable.getRowValue(row);
    String type = m_locatorMenu.getSelectedType();

    FileMenu auditMenu = m_audit.getFileMenu();

    int menuIndex = auditMenu.getSelectedIndex();
    if(menuIndex < 0) menuIndex = 0;

    writeAuditPaths(path, type, flag);

    //m_audit.removeAll();
    if (m_bInit)
    {
        m_audit.makeComboFileTable(m_paths2, m_label2, menuIndex);
        m_bInit = false;
    }
    else
    {
        m_audit.updateComboFileTable(m_paths2, m_label2, menuIndex);
    }
    m_auditTable = m_audit.getTable();
    m_auditTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

    //remove(m_audit);
    //setBottomComponent(m_audit);

    if (getParent() != null)
    {
        Dimension pSize = getParent().getPreferredSize();

        setPreferredSize(pSize);
        int div = (pSize.height)/2;
        setDividerLocation(div);
    }
    else
    {
        setDividerLocation(0.5);
    }
    writeTableSelection2File();
    writeMenuSelection2File();
    }

    private boolean writeAuditingAudit() {

    if( m_locatorMenu == null) return(false);

    Date date = new Date();
    String tm = date.toString();

    int good = m_locatorTable.getFileTableModel().getGoodRows();
    int bad = m_locatorTable.getFileTableModel().getBadRows();
    int total = good+bad;

    String str = m_locatorMenu.getSelectedPath();

    String line = tm + " aaudit " + m_username + " load entries "+total +
        " entries from " +str+ ", " + bad + " are corrupted.";

    String[] cmd = {WGlobal.SHTOOLCMD, "-c",
        "/vnmr/p11/bin/writeAaudit -l \"" + line + "\""};

    WUtil.runScriptInThread(cmd);
	
    return(true);
    }

    private boolean writeMenuSelection2File() {

    if( m_locatorMenu == null) return(false);

    String filepath =
         FileUtil.savePath("USER/PERSISTENCE/auditMenuSelection");

        FileWriter fw;
        PrintWriter os;
        try {
              fw = new FileWriter(filepath, false);
              os = new PrintWriter(fw);

          String str = m_locatorMenu.getSelectedPath();
      if(str.equals("All")) {

               for(int i=0; i<m_locatorMenu.getItemCount(); i++) {
                  String key = (String)m_locatorMenu.getItemAt(i);
                  String strPath = m_locatorMenu.getSelectedPath(key);

                  if(strPath != null && !strPath.equals("All")
            && !strPath.equals("Trash") )
            os.println("path " +strPath);
               }
          } else if(str != null) os.println("path " +str);

              os.close();
        } catch(Exception er) {
             Messages.postDebug("Problem creating file, " + filepath);
         return(false);
        }
    return(true);
    }

    private boolean writeTableSelection2File() {

    if(m_locatorTable == null) return(false);

    String filepath =
         FileUtil.savePath("USER/PERSISTENCE/auditTableSelection");

    String type = m_locatorTable.getFileTableModel().getType();
    Vector list = new Vector();

        FileWriter fw;
        PrintWriter os;
        try {
              fw = new FileWriter(filepath, false);
              os = new PrintWriter(fw);

          int nrow = m_locatorTable.getSelectedRowCount();
          int[] rows = m_locatorTable.getSelectedRows();

          for(int i=0; i<nrow; i++) {

            String str = m_locatorTable.getRowValue(rows[i]);
        if(type.equals("records"))
        str = str.substring(0, str.lastIndexOf(File.separator) + 1);

        if(!list.contains(str)) list.add(str);

          }
      for(int i=0; i<list.size(); i++)
            os.println("path " +list.elementAt(i));
              os.close();
        } catch(Exception er) {
             Messages.postDebug("Problem creating file, " + filepath);
         return(false);
        }
    return(true);
    }

    private boolean writeAuditPaths(String path, String type, boolean flag) {

    String file = (String)m_paths2.elementAt(0);

    FileWriter fw;
        PrintWriter os;
        try {
              fw = new FileWriter(file, false);
              os = new PrintWriter(fw);
          String str = null;
          if(type.equals("s_auditTrailFiles")) {
        str = path +"|vnmr audit trail|s_auditTrail";
                os.println(str);
        str = path +"|cmd history|cmdHistory";
                os.println(str);
          } else if(type.equals("records") && flag) {
        str = path +"/checksum.flag|record info|recordChecksumInfo";
                os.println(str);
        str = path +"/auditTrail|record audit trail|d_auditTrail";
                os.println(str);
        str = path +"/cmdHistory|records cmd history|cmdHistory";
                os.println(str);
          } else if(type.equals("records")) {
        str = path +"/auditTrail|record audit trail|d_auditTrail";
                os.println(str);
        str = path +"/cmdHistory|records cmd history|cmdHistory";
                os.println(str);
          } else if(type.equals("a_auditTrailFiles")) {
        str = path +"|auditing audit trail|a_auditTrails";
                os.println(str);
          } else if(type.equals("u_auditTrailFiles")) {
        str = path +"|user account|u_auditTrails";
                os.println(str);
          } else if(type.equals("x_auditTrailFiles")) {
        str = path +"|record deletions|x_auditTrails_recDele";
                os.println(str);
        str = path +"|failed executions|x_auditTrails_failure";
                os.println(str);
        str = path +"|delete|x_auditTrails_unlink";
                os.println(str);
        str = path +"|rename|x_auditTrails_rename";
                os.println(str);
        str = path +"|open to write|x_auditTrails_openWrite";
                os.println(str);
          } else if(type.equals("recordTrashInfo")) {
        str = path +"|info|recordTrashInfo2";
                os.println(str);
          } else {
        str ="none|none|none";
                os.println(str);
          }
              os.close();
        } catch(Exception er) {
             Messages.postDebug("Problem creating file, " + file);
         return(false);
        }
    return(true);
    }

    public ComboFileTable getAudit() {
        return(this.m_audit);
    }

    public JPanel getLocator() {
        return(this.m_locator);
    }

    public JScrollPane getLocScrollPane()
    {
        return (m_locComboFileTable != null ? m_locComboFileTable.getScrollPane() : null);
    }

    private void copyPropertyfiles() {

        String host = Util.getHostName();

        if(host.length()>0) host = "_" + host;

        String filename = "userResources" + host + ".properties";
        String syspropertyfile = FileUtil.openPath("SYSTEM/USRS/properties/" + filename);
        if(syspropertyfile == null) return;

        String usrpropertyfile = FileUtil.savePath("USER/PROPERTIES/" + filename);
        if(usrpropertyfile == null) return;

        String[] cmd = {WGlobal.SHTOOLCMD, "-c", "cp -f " + syspropertyfile
        +" "+ usrpropertyfile};
        WUtil.runScriptInThread(cmd);
    }

    public static String getAuditDir() {

// get audirDir from part11Config.

        String auditDir = null;

        String part11Dir = FileUtil.openPath("SYSTEM/PART11");

        if(part11Dir == null) part11Dir = "/vnmr/p11";

        String configPath = FileUtil.openPath(part11Dir +"/part11Config");

        if(configPath != null) {

          BufferedReader in;
          String inLine;

          try {
            in = new BufferedReader(new FileReader(configPath));
            while ((inLine = in.readLine()) != null) {
                if (inLine.length() > 1 && !inLine.startsWith("#") &&
                   !inLine.startsWith("%") && !inLine.startsWith("@")) {
                    inLine.trim();
                    StringTokenizer tok = new StringTokenizer(inLine, ":,\n", false);
                    if (tok.countTokens() > 1) {
                        String key = tok.nextToken();
                        String dir = tok.nextToken();
                        if(key.equals("auditDir")) {
                            auditDir = dir;
                            break;
                        }
                    }
                }
            }
          } catch(IOException e) {
                Messages.postError("ERROR: read " + configPath);
          }
        }

        if(auditDir == null) auditDir = part11Dir +"/auditTrails";

        return auditDir;
    }

    public static boolean writeSystemAuditMenu() {

// return if file already exists.

    String file =
         FileUtil.openPath("USER/PERSISTENCE/SystemAuditMenu");

    if(file != null) return(true);

    String auditDir = getAuditDir();

    file = FileUtil.savePath("USER/PERSISTENCE/SystemAuditMenu");

    FileWriter fw;
        PrintWriter os;
        try {
              fw = new FileWriter(file, false);
              os = new PrintWriter(fw);
          String str = null;
          str = auditDir +"/sessions|sessions audit trails|s_auditTrailFiles";
              os.println(str);
          str = auditDir +"/user|user account audit trails|u_auditTrailFiles";
              os.println(str);
          str = auditDir +"/aaudit|auditing audit trails|a_auditTrailFiles";
              os.println(str);

              os.close();
        } catch(Exception er) {
             Messages.postDebug("Problem creating file, " + file);
         return(false);
        }
    return(true);
    }

    public static boolean writeRecordAuditMenu() {

// return if file already exists.
    String file =
         FileUtil.openPath("USER/PERSISTENCE/RecordAuditMenu");

// If it returns when the file exists, it never gets new users into the menu
// Make it recreate the menu file/list every time so that it is update.
//    if(file != null) return(true);

// get userlist from $vnmrsystem/adm/users/userlist.

        Vector userlist = new Vector();
    String systemDir = System.getProperty("sysdir");

    if(systemDir == null) systemDir = "/vnmr";

    String userlistPath = FileUtil.openPath(systemDir +"/adm/users/userlist");

    if(userlistPath != null) {

          BufferedReader in;
          String inLine;

          try {
            in = new BufferedReader(new FileReader(userlistPath));
            while ((inLine = in.readLine()) != null) {
                if (inLine.length() > 1 && !inLine.startsWith("#")) {
                    inLine.trim();
                    StringTokenizer tok = new StringTokenizer(inLine, ", \n", false);
            while(tok.hasMoreTokens()){
                        userlist.add((String)tok.nextToken());
                    }
        }
        }
          } catch(IOException e) {
                Messages.postError("ERROR: read " + userlistPath);
      }
        }

    Vector part11Dirs = new Vector();

    for(int i=0; i<userlist.size(); i++) {

       String path = FileUtil.openPath(systemDir +
        "/adm/users/profiles/p11/" + userlist.elementAt(i));

       if(path != null) {

             BufferedReader in;
             String inLine;

             try {
              in = new BufferedReader(new FileReader(path));
              while ((inLine = in.readLine()) != null) {
                if (inLine.length() > 1 && !inLine.startsWith("#") &&
            !inLine.startsWith("%") && !inLine.startsWith("@")) {
                    inLine.trim();
                    StringTokenizer tok = new StringTokenizer(inLine, ":,\n", false);
                    if (tok.countTokens() > 1) {
                        String key = tok.nextToken();
                        part11Dirs.add((String)tok.nextToken());
            }
        }
          }
            } catch(IOException e) {
                Messages.postError("ERROR: read " + path);
        }
      }
    }

    String trashDir = null;

    String p11Dir = FileUtil.openPath("SYSTEM/PART11");

    if(p11Dir == null) p11Dir = "/vnmr/p11";

    String configPath = FileUtil.openPath(p11Dir +"/part11Config");

    if(configPath != null) {

          BufferedReader in;
          String inLine;

          try {
            in = new BufferedReader(new FileReader(configPath));
            while ((inLine = in.readLine()) != null) {
                if (inLine.length() > 1 && !inLine.startsWith("#") &&
            !inLine.startsWith("%") && !inLine.startsWith("@")) {
                    inLine.trim();
                    StringTokenizer tok = new StringTokenizer(inLine, ":,\n", false);
                    if (tok.countTokens() > 1) {
                        String key = tok.nextToken();
                        String dir = tok.nextToken();
            if(key.equals("part11Dir")) {
                            trashDir = dir +"/trash";
                break;
            }
            }
        }
        }
          } catch(IOException e) {
                Messages.postError("ERROR: read " + configPath);
      }
    }

    file = FileUtil.savePath("USER/PERSISTENCE/RecordAuditMenu");

    FileWriter fw;
        PrintWriter os;
        try {
              fw = new FileWriter(file, false);
              os = new PrintWriter(fw);
           os.println("All|All|records");
          for(int i=0; i<part11Dirs.size(); i++) {
            String str =
        part11Dirs.elementAt(i) +"|"+ part11Dirs.elementAt(i) + "|records";
                os.println(str);
          }

     if(trashDir != null)
           os.println(trashDir +"|Trash|recordTrashInfo");

              os.close();
        } catch(Exception er) {
             Messages.postDebug("Problem creating file, " + file);
         return(false);
        }
    return(true);
    }
}

