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

import java.text.*;
import javax.swing.*;
import javax.swing.border.Border;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import javax.swing.event.*;
import java.net.*;

public class VnmrAdmin extends JFrame {

    JButton deleteButton;

    int curTabIndex = 0;
    int numField = 15;

    JTextField[][] comTextField = new JTextField[3][];

    JLabel labelBottom;
    JButton addButton = null;

    Vector userList = null;
    Vector groupList = null;
    UserPanel userPan;
    GroupPanel groupPan;
    GroupListPanel gListPan;
    UserListPanel uListPan;
    JComboBox userBox;
    JButton userAddButton;
    JButton userDeleteButton;
    final String addUserString =  "Add New User";
    final String saveUserString =  "Save Changes";

    //String tempFile = "/export/home/chin/vadmin/willgoaway";
    //String destFile[] = {"/export/home/chin/vadmin/userList",
    //                     "/export/home/chin/vadmin/group",
    //                     "/export/home/chin/vadmin/access"};

    String shToolCmd  = "/bin/bash";
    String vnmrSystem = "/vnmr";
    String vnmrAdmin = "vnmr1";
    String dbUser="";
    String tempFile = vnmrSystem + "/adm/users/willgoaway";

    String userFile = "userlist";
    String sysFileDir = vnmrSystem+"/adm/users/profiles/system/";
    String userFileDir = vnmrSystem+"/adm/users/profiles/user/";
    String destFile[] = {vnmrSystem + "/adm/users/"+userFile,
                         vnmrSystem + "/adm/users/group",
                         vnmrSystem + "/adm/users/access"};

    String   entryLabelString[][] = { {"User Login Name :",
                                       "User Long Name :",
                                       "User Home Directory:",
                                       "Password :",
                                       "Data Directory :",
                                       "Access Level :"},

                                      {"Group Name :",
                                       "Group Long Name :",
                                       "Users and Group List :"},

                                      {"User Login Name :",
                                       "User and Group Access List :"},
                                    };

    String   textFieldDefVal[][] = { { vnmrAdmin,
                                       "VNMRJ admin",
                                       "/export/home/" + vnmrAdmin,
                                       "",
                                       "/export/home/" + vnmrAdmin + "/vnmrsys/data",
                                       "2"},

                                      {vnmrAdmin,
                                       "VNMRJ group",
                                       vnmrAdmin},

                                      {vnmrAdmin,
                                       "all"},
                                    };

    final String bottomStrg =  "Using this interface to add " +
                               "Users, Groups, and Access to Vnmr database";

    String addMessage[] = {"  ",
                           "Users in a group are separated by commas",
                           "Access could be all, group name, or user name"};

    Border padding = BorderFactory.createEmptyBorder(20,20,5,20);

    String   sysDir;
    String   destDir;
    boolean  legalUser = false;
    boolean  debug = false;
    String   userEntry[] = { "Login Name :",
                             "Long Name :",
                             "Home Directory :",
                             "VNMR User Directory:",
                             "VNMR System Directory:",
                             "VNMR Data Directory :",
                             "Application Directory :",
                             "User Owned Directory :",
                             "Access Level :",
                             "Accessible Group :"
                        };
    final int  UNAME = 0;
    final int  ULNAME = 1;
    final int  HOMEDIR = 2; // Home Directory
    final int  USERDIR = 3; // Vnmr User Directory
    final int  SYSDIR = 4; // Vnmr System Directory
    final int  DDIR = 5;  // Data Directory
    final int  SEARCHDIR = 6;  // App Directory
    final int  OWNEDDIR = 7;  // Owned Directory
    final int  ACCESSLVL = 8;  // Access Level
    final int  ACCESSGRP = 9;  // Access Group
    final int  userItems = 10;

    String   groupEntry[] = { "Group Name :",
                              "Group Long Name :",
                              "Users in Group :"
                        };
    final int  GNAME = 0;
    final int  LGNAME = 1;
    final int  GUSERS = 2;
    final int  groupItems = 3;

    public VnmrAdmin() {
        setUpDefaultAttr();
        setTitle("VNMR Admintool");

        Container content = getContentPane();
        content.setLayout(new GridLayout(1,1));
        JPanel mainPan = new JPanel();

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                saveToFiles();
                System.exit(0);
            }
        });

        setForeground(Color.black);
        sysDir = System.getProperty("debug");
        if (sysDir != null && sysDir.equals("yes"))
            debug = true;

        sysDir = System.getProperty("sysdir");
        if (sysDir == null)
            sysDir = File.separator+"vnmr";
        destDir = sysDir+File.separator+"adm"+File.separator+"users"+File.separator;

        checkPermision();
        userList = new Vector();
/*
        buildUserList();
*/
        createUserList();
        groupList = new Vector();
        buildGroupList();
        addUser2Group();
        labelBottom = new JLabel( bottomStrg, JLabel.CENTER );
        JTabbedPane tabbedPane = new JTabbedPane();
        userPan = new UserPanel();
        groupPan = new GroupPanel();
        if (userList.size() > 0) {
           UserData user = (UserData)userList.elementAt(0);
           userPan.setUser(user.name);
        }
        else
           userPan.setUser(System.getProperty("user.name"));
        if (groupList.size() > 0) {
           GroupData grp = (GroupData)groupList.elementAt(0);
           groupPan.setGroup(grp);
        }
        gListPan = new GroupListPanel();
        uListPan = new UserListPanel();

/*
        tabbedPane.addTab("  User  ", null, createEntryPanel( (int) 0 ), addMessage[0]); //with tooltip text
        tabbedPane.addTab(" Group ", null, createEntryPanel( (int) 1 ), addMessage[1]);
        tabbedPane.addTab(" Access ", null, createEntryPanel( (int) 2 ), addMessage[2]);
        tabbedPane.addTab("Show Users", null, createListbox(destFile[0]), "Show user");
        tabbedPane.addTab("Show Groups", null, createListbox(destFile[1]), "Show group");
        tabbedPane.addTab("Show Access", null, createListbox(destFile[2]), "Show access");
*/
        tabbedPane.addTab("  User  ", null, userPan, null);
        tabbedPane.addTab("  Group  ", null, groupPan, addMessage[1]);

        tabbedPane.addTab("Show Users", null, uListPan, "Show user");
        tabbedPane.addTab("Show Groups", null, gListPan, "Show group");
        tabbedPane.addChangeListener(new TabbedListener());

        mainPan.setLayout(new BorderLayout());
        mainPan.add(tabbedPane, BorderLayout.CENTER);
        mainPan.add(labelBottom, BorderLayout.SOUTH);
        labelBottom.setBorder(BorderFactory.createEmptyBorder(10,10,10,10));

        content.add(mainPan);
    }


    private JPanel createListbox(String dsplFile) {

       String deleteString = " Delete ";
       DefaultListModel listModel = new DefaultListModel();

        showFile(dsplFile, listModel);

        //Create the list and put it in a scroll pane
        JList listBox = new JList(listModel);
        listBox.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        listBox.setSelectedIndex(0);
        //listBox.addListSelectionListener(this);
        JScrollPane listScrollPane = new JScrollPane(listBox);

        deleteButton = new JButton(deleteString);
        deleteButton.setActionCommand(deleteString);
        deleteButton.addActionListener(new DeleteButtonListener());
        deleteButton.putClientProperty("listbox", listBox);

        JPanel intermPane = new JPanel();
        intermPane.setBorder(BorderFactory.createEmptyBorder(15, 20, 0, 20));
        intermPane.setLayout(new BorderLayout());
        intermPane.add(listScrollPane, BorderLayout.NORTH);
        intermPane.add(deleteButton, BorderLayout.SOUTH);
        intermPane.putClientProperty("listbox", listBox);
        intermPane.putClientProperty("button", deleteButton);
        intermPane.setBorder(padding);

        return intermPane;
    }

    /** Listens to the Show tabs. */
    class TabbedListener implements ChangeListener {
        public void stateChanged(ChangeEvent e) {
            JTabbedPane source = (JTabbedPane)e.getSource();
            curTabIndex = source.getSelectedIndex();
            JPanel panel = (JPanel)source.getComponentAt(curTabIndex);
            if (panel == gListPan) {
                gListPan.updateList();
            }
            if (panel == uListPan) {
                uListPan.updateList();
            }
            if (panel == groupPan) {
                groupPan.redisplay();
            }
            if (panel == userPan) {
                userPan.redisplay();
            }

/*
            if (curTabIndex > 2) {
               JButton dButton = (JButton) panel.getClientProperty("button");
               JList listBox = (JList) panel.getClientProperty("listbox");
               DefaultListModel listM = (DefaultListModel) listBox.getModel();


               listM.clear();
               showFile(destFile[curTabIndex-3], listM);

               if (listM.getSize() > 0)
                  dButton.setEnabled(true);
               else
                  dButton.setEnabled(false);

            }
*/
        }
    }

    public boolean showFile( String targetFile, DefaultListModel listModel ) {

       try {
          FileReader fr = new FileReader(targetFile);
          BufferedReader br = new BufferedReader(fr);
          String line;

          while((line = br.readLine()) != null) {
             listModel.addElement(line);
          }
          fr.close();

       } catch(FileNotFoundException f) {
            //ignore
            //setLabel(targetFile + "  not found.");
         }
         catch(IOException g) {
            setLabel("Error reading  " + targetFile );
         }

       return false;
    }

    //Add user to "group" file, under "vnmr1" group only
    public boolean addToGroupFile( String targetFile, String grpName, String targetStr ) {
       try {

          FileReader fr = new FileReader(targetFile);
          BufferedReader br = new BufferedReader(fr);
          String line = null;
          //String tmp = null;

          while((line = br.readLine()) != null) {

              StringTokenizer stkst = new StringTokenizer(line, ":");

              //if not match save the line
              if ( ! grpName.equals(stkst.nextToken().trim())) {
                 appendToFile( tempFile, line + "\n" );
              } else {

                     String tmp = line + ", " + targetStr;
                     appendToFile( tempFile, tmp + "\n" );

                }
          }
          fr.close();

       } catch(FileNotFoundException f) {
            //ignore
            //setLabel(targetFile + "  not found.");
         }
         catch(IOException g) {
            setLabel("Error reading  " + targetFile );
         }

       File f1 = new File(tempFile);
       File f2 = new File(targetFile);

       if ( f1.exists() )
          f1.renameTo(f2);
       else
            f2.delete();

       return true;
    }


    public boolean deleteFromFile( String targetFile, String targetStr ) {
       try {

          FileReader fr = new FileReader(targetFile);
          BufferedReader br = new BufferedReader(fr);
          String line;

          while((line = br.readLine()) != null) {

              StringTokenizer stkst = new StringTokenizer(line, ":");

              //if not match save the line
              if ( ! targetStr.equals(stkst.nextToken().trim())) {
                 appendToFile( tempFile, line + "\n" );
              }
          }
          fr.close();

       } catch(FileNotFoundException f) {
            //ignore
            //setLabel(targetFile + "  not found.");
         }
         catch(IOException g) {
            setLabel("Error reading  " + targetFile );
         }

       File f1 = new File(tempFile);
       File f2 = new File(targetFile);

       if ( f1.exists() )
          f1.renameTo(f2);
       else
            f2.delete();

       return true;
    }

    //Listen to Listboxes' Delete buttons
    class DeleteButtonListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {

            //This method can be called only if there's a valid selection
            //so go ahead and remove whatever's selected.
            JButton delButton = (JButton) e.getSource();
            JList listBox = (JList) delButton.getClientProperty("listbox");
            int index = listBox.getSelectedIndex();

            if (index < 0) {
               return;
            }

            DefaultListModel listModel = (DefaultListModel) listBox.getModel();
            StringTokenizer stkst = new StringTokenizer((String) listModel.get(index), ":");

            //remove item from listbox and file  here
            listModel.remove(index);
            deleteFromFile(destFile[curTabIndex-3], stkst.nextToken());

            int size = listModel.getSize();
            //Nobody's left, disable delete button.
            if (size == 0) {
                //deleteButton.setEnabled(false);
                delButton.setEnabled(false);

            } else {
                //Adjust the selection.
                if (index == listModel.getSize())//removed item in last position
                    index--;
                listBox.setSelectedIndex(index);   //otherwise select same index
            }
        }
    }

    private JPanel createEntryPanel( int index ){

       String[] entryLabel = entryLabelString[index];

       JLabel[]   comLabel = new JLabel[entryLabel.length];
       comTextField[index] = new JTextField[entryLabel.length];

       addButton = new JButton(" Add ");
       addButton.addActionListener(new AddButtonListener());

       JPanel labelPane = new JPanel();
       labelPane.setLayout(new GridLayout(0, 1));

       JPanel txFieldPane = new JPanel();
       txFieldPane.setLayout(new GridLayout(0, 1));

       for (int i = 0; i < entryLabel.length; i++) {

          comLabel[i]      = new JLabel(entryLabel[i]);
          comTextField[index][i]  = new JTextField(numField);
          comTextField[index][i].setText(textFieldDefVal[index][i]);

          comLabel[i].setLabelFor(comTextField[index][i]);

          labelPane.add(comLabel[i]);
          txFieldPane.add(comTextField[index][i]);
       }


       //Put the panels in another panel, labels on left, text fields on right.
       JPanel contentPane = new JPanel();
       contentPane.setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));
       contentPane.setLayout(new BorderLayout());
       contentPane.add(labelPane, BorderLayout.CENTER);
       contentPane.add(txFieldPane, BorderLayout.EAST);

       JPanel box = new JPanel();
       JLabel label = new JLabel(addMessage[index]);
       box.setLayout( new BoxLayout(box, BoxLayout.Y_AXIS) );
       box.add(label);
       box.add(contentPane);

       JPanel pane = new JPanel();
       pane.setLayout(new BorderLayout());
       pane.add(box, BorderLayout.NORTH);
       pane.add(addButton, BorderLayout.SOUTH);
       pane.setBorder(padding);

       return pane;
    }

    class AddButtonListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {

            JTextField[] cTextField = null;

            switch (curTabIndex) {
               case 0: cTextField = comTextField[0]; break;
               case 1: cTextField = comTextField[1]; break;
               case 2: cTextField = comTextField[2]; break;
               default: break;
            }

            dbUser = cTextField[0].getText();

            //User didn't type in a name...
            if (dbUser.equals("")) {
                setLabel("Not complete informations - User name");
                Toolkit.getDefaultToolkit().beep();
                return;
            }
            if (isStringExist( destFile[curTabIndex], dbUser )){
                setLabel(dbUser + "  already exist");
                return;
            }

            String tempStrg = "";
            String mesgStrg = "";

            switch (curTabIndex) {                                                //The order of userList file
               case 0:  tempStrg =                   dbUser + ":" +               //User Login Name
                                    cTextField[3].getText() + ":" +               //passwd
                                    cTextField[1].getText() + ":" +               //User Long Name
                                    cTextField[2].getText() + "/vnmrsys" + ":" +  //User Home Directory
                                    cTextField[4].getText() + ":" +               //Data Directory
                                    cTextField[5].getText() + "\n";               //Access Level
                        mesgStrg= "User " + dbUser;

                        String rtn = createUser( dbUser );
                        if ( rtn.trim().equals("DONE") ) {

                           appendToFile( destFile[curTabIndex], tempStrg );
                           //all users go to vnmr1 group for now
                           addToGroupFile( destFile[1], "vnmr1", dbUser );

                           setLabel(mesgStrg + "  added");
                        }
                        else {
                           if (rtn.trim().equals("invalid user"))
                              setLabel(mesgStrg + " is an " + rtn);
                           else
                              setLabel("Problem adding" + dbUser);
                        }
                        break;

               case 1:  tempStrg =                   dbUser + ":" +
                                    cTextField[1].getText() + ":" +
                                    cTextField[2].getText() + "\n";

                        if ( appendToFile( destFile[curTabIndex], tempStrg ))
                           setLabel("Group " + dbUser + "  added");
                        else
                           setLabel("Problem adding" + dbUser);
                        break;

               case 2:  tempStrg =                   dbUser + ":" +
                                    cTextField[1].getText() + "\n";

                        if (isStringExist( destFile[2], dbUser ) || ! isStringExist( destFile[0], dbUser )) {
                           setLabel( dbUser + "  is either not exist or already got access permission ");
                           return;
                        }
                        else {
                            if ( appendToFile( destFile[curTabIndex], tempStrg ))
                               setLabel("Access permission for user " + dbUser + "  added");
                            else
                               setLabel("Problem adding" + dbUser);
                        }
                        break;

               default: break;
            }

            //for (int i=0; i<cTextField.length; i++) {
            //    cTextField[i].setText("");
            //}
            return;
        }
    }

    public String getUserHomeDir(String name )
    {
       String strg = null;
       try
       {
          String[] cmd = {shToolCmd, "-c", "getent passwd " + name };
          Runtime rt = Runtime.getRuntime();
          Process prcs = rt.exec(cmd);
          if (prcs == null)
                return null;

          InputStream istrm = prcs.getInputStream();
          if (istrm == null)
                return null;
          BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

          strg = bfr.readLine();
       }
       catch (IOException e)
       {
          return null;
       }
       return strg;
    }

    public String createUser( String name )
    {
       String strg = null;
       try
       {
          String[] cmd = {shToolCmd, "-c", sysDir + "/bin/create_pgsql_user" + " " + name };
          Runtime rt = Runtime.getRuntime();
          Process prcs = rt.exec(cmd);
          if (prcs == null)
                return null;

          InputStream istrm = prcs.getInputStream();
          if (istrm == null)
                return null;
          BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

          strg = bfr.readLine();
          while(strg != null)
          {
              if (strg.trim().equals("DONE") | strg.trim().equals("invalid user")) {
                      break;
              }
              strg = bfr.readLine();
           }
       }
       catch (IOException e)
       {
          System.out.println(e);
          return null;
       }
       return strg;
    }


    public boolean isStringExist( String targetFile, String targetStr ) {

       try {

          FileReader fr = new FileReader(targetFile);
          BufferedReader br = new BufferedReader(fr);
          String line;

          while((line = br.readLine()) != null) {

              StringTokenizer stkst = new StringTokenizer(line, ":");

              if (targetStr.equals(stkst.nextToken().trim()))
                 return true;
          }
          fr.close();

       } catch(FileNotFoundException f) {
            //ignore
            //setLabel(targetFile + "  not found.");
         }
         catch(IOException g) {
            setLabel("Error reading  " + targetFile );
         }

       return false;
    }

    public boolean appendToFile( String destiFile, String str ) {

        char buf[] = new char[str.length()];
        str.getChars(0, str.length(), buf, 0);

        try {

           FileWriter f2 = new FileWriter(destiFile, true); //true --> append
           f2.write(buf);
           f2.close();
           return true;

        } catch(FileNotFoundException f) {
             //ignore
             //setLabel(destiFile + "  not found.");
          }
          catch(IOException g) {
             setLabel("Error reading  " + destiFile );
          }
          return false;
    }

    public void setLabel(String newText) {
        labelBottom.setText(newText);
        labelBottom.setHorizontalAlignment(JLabel.LEFT);
    }

    public static void setUpDefaultAttr() {
        String def = System.getProperty("font");
        if (def == null)
            def = "Dialog plain 16";
        StringTokenizer tok = new StringTokenizer(def, " ,\n");
        if (!tok.hasMoreTokens())
            return;
        String  name = tok.nextToken();
        int  style = Font.PLAIN;
        int  s = 12;
        if (tok.hasMoreTokens()) {
            def = tok.nextToken().toLowerCase();
            if (def.equals("bold"))
                style = Font.BOLD;
            else if (def.equals("italic"))
                style = Font.ITALIC;
        }
        if (tok.hasMoreTokens()) {
            s = Integer.parseInt(tok.nextToken());
        }
        Font ft = new Font(name, style, s);

        if (ft == null)
            return;
        UIManager.put("ToggleButton.font", ft);
        UIManager.put("Label.font", ft);
        UIManager.put("Menu.font", ft);
        UIManager.put("MenuItem.font", ft);
        UIManager.put("PopupMenu.font", ft);
        UIManager.put("JMenuItem.font", ft);
        UIManager.put("JMenu.font", ft);
        UIManager.put("JButton.font", ft);
        UIManager.put("JComboBox.font", ft);
        UIManager.put("JLabel.font", ft);
        UIManager.put("JList.font", ft);
        UIManager.put("TextField.font", ft);
        UIManager.put("JTextField.font", ft);
        Color c = Color.black;
        UIManager.put("Label.foreground", c);
        UIManager.put("TextField.foreground", c);
    }

    public void checkPermision() {
        String fp = destDir+userFile;
        File fd = new File(fp);
        File fdp = new File(fd.getParent());

        if(!fdp.exists()) {
            fdp.mkdirs();
            fd = new File(fp);
        }
        if(!fdp.exists() || !fdp.canWrite())
            return;
        if(fd.exists() && !fd.canWrite())
            return;
        fp = destDir+"group";
        fd = new File(fp);
        if(fd.exists() && !fd.canWrite())
            return;
        fp = destDir+"access";
        fd = new File(fp);
        if(fd.exists() && !fd.canWrite())
            return;
        legalUser = true;
    }

    public void saveSysProfile(UserData rec) {
        String fp = sysFileDir+rec.name;
        PrintWriter fout = null;
        if (debug)
            fp = "/tmp/adm/sys_"+rec.name;
        try {
           fout = new PrintWriter( new OutputStreamWriter(
                 new FileOutputStream(fp)));
        }
        catch(IOException er) { }
        if (fout == null) {
             return;
        }
        if (rec.name != null && rec.name.length() > 0)
            fout.println("accname    "+rec.name );
        if (rec.longName != null && rec.longName.length() > 0)
            fout.println("name    "+rec.longName);
        if (rec.homeDir != null && rec.homeDir.length() > 0)
            fout.println("home    "+rec.homeDir);
        if (rec.ownedDir != null && rec.ownedDir.length() > 0)
            fout.println("owned   "+rec.ownedDir);
        if (rec.accessLevel != null && rec.accessLevel.length() > 0)
            fout.println("usrlvl  "+rec.accessLevel);
        if (rec.grpData != null && rec.grpData.length() > 0)
            fout.println("access  "+rec.grpData);
        fout.close();
    }

    public void saveUserProfile(UserData rec) {
        String fp = userFileDir+rec.name;
        PrintWriter fout = null;
        if (debug)
            fp = "/tmp/adm/usr_"+rec.name;
        try {
           fout = new PrintWriter( new OutputStreamWriter(
                 new FileOutputStream(fp)));
        }
        catch(IOException er) { }
        if (fout == null) {
             return;
        }
        if (rec.userDir != null && rec.userDir.length() > 0)
            fout.println("userdir  "+rec.userDir);
        if (rec.sysDir != null && rec.sysDir.length() > 0)
            fout.println("sysdir   "+rec.sysDir);
        if (rec.appDir != null && rec.appDir.length() > 0)
            fout.println("appdir   "+rec.appDir);
        if (rec.dataDir != null && rec.dataDir.length() > 0)
            fout.println("datadir  "+rec.dataDir);
        fout.close();
    }

    public void saveUserList() {
        String fp = destDir+userFile;
        PrintWriter fout = null;
        if (debug)
            fp = "/tmp/adm/userlist";
        try {
           fout = new PrintWriter( new OutputStreamWriter(
                 new FileOutputStream(fp)));
        }
        catch(IOException er) { }
        if (fout == null) {
             return;
        }
        for (int k = 0; k < userList.size(); k++) {
            UserData usr = (UserData)userList.elementAt(k);
            if (!usr.isRemoved)
                fout.print(usr.name+" ");
        }
        fout.println("");
        fout.close();
    }

    public void saveGroupList() {
        String fp = destDir+"group";
        String str;
        PrintWriter fout = null;
        if (debug)
            fp = "/tmp/adm/group";
        try {
           fout = new PrintWriter( new OutputStreamWriter(
                 new FileOutputStream(fp)));
        }
        catch(IOException er) { }
        if (fout == null) {
             return;
        }
        for (int k = 0; k < groupList.size(); k++) {
            GroupData grp = (GroupData)groupList.elementAt(k);
            str = grp.name+":"+grp.longName+":"+grp.userData;
            fout.println(str);
        }
        fout.close();
    }

    public void saveToFiles() {
        if (!legalUser && !debug)
             return;
        saveUserList();
        saveGroupList();
        for (int k = 0; k < userList.size(); k++) {
            UserData usr = (UserData)userList.elementAt(k);
            if (!usr.isRemoved && usr.needSave) {
                saveSysProfile(usr);
                saveUserProfile(usr);
            }
        }
        System.err.println("Please run 'managedb update' to update user info.");
/*
        String[] cmd = {shToolCmd, "-c", sysDir+"/bin/managedb update &" };
        try {
           Runtime rt = Runtime.getRuntime();
           rt.exec(cmd);
        }
        catch (IOException e ) { }
*/
    }

    public void writeFiles() {
        if (!legalUser && !debug)
             return;
        String fp = destDir+userFile;
        String str;
        PrintWriter fout = null;
        int k;
        try {
           fout = new PrintWriter( new OutputStreamWriter(
                 new FileOutputStream(fp)));
        }
        catch(IOException er) { }
        if (fout == null) {
             return;
        }

        fout.println("#username:passwd:Long Name:Vnmrsys dir: data dir, data dir, ...:user level ");
        fout.println("#Add vnmr1 to the access file for each user so that system files are visible.");

        for (k = 0; k < userList.size(); k++) {
            UserData usr = (UserData)userList.elementAt(k);
            str = usr.name+"::"+usr.longName+":"+usr.homeDir+":"+usr.dataDir+":"+usr.accessLevel;
            fout.println(str);
        }
        fout.close();

        fp = destDir+"access";
        try {
           fout = new PrintWriter( new OutputStreamWriter(
                 new FileOutputStream(fp)));
        }
        catch(IOException er) { }
        if (fout == null) {
             return;
        }
        for (k = 0; k < userList.size(); k++) {
            UserData usr = (UserData)userList.elementAt(k);
            if (!usr.isRemoved) {
                if (usr.inGroup("all"))
                    str = usr.name+":all";
                else
                    str = usr.name+":"+usr.grpData;
                fout.println(str);
            }
        }
        fout.close();

        fp = destDir+"group";
        try {
           fout = new PrintWriter( new OutputStreamWriter(
                 new FileOutputStream(fp)));
        }
        catch(IOException er) { }
        if (fout == null) {
             return;
        }
        for (k = 0; k < groupList.size(); k++) {
            GroupData grp = (GroupData)groupList.elementAt(k);
            str = grp.name+":"+grp.longName+":"+grp.userData;
            fout.println(str);
        }
        fout.close();
    }

    public String getToken(String str, char delimiter, int n) {
            int start = 0;
            int ip = 1;
            int max = str.length();
            char c;

            while (start < max) {
                c = str.charAt(start);
                if (c == ' ')
                    start++;
                else
                    break;
            }
            while ((ip < n) && (start < max)) {
                c = str.charAt(start);
                if (c == delimiter)
                    ip++;
                start++;
            }
            ip = start;
            while (ip < max) {
                c = str.charAt(ip);
                if (c == delimiter)
                   break;
                ip++;
            }
            if (ip - start <= 0)
                return null;
            return str.substring(start, ip);
    }

    public int getTokenNum(String str, char delimiter) {
            int pos;
            int ip;
            int max;
            String data = str.trim();
            char c;

            max = data.length();
            if (max <= 0)
                return 0;
            pos = 0;
            ip = 0;
            while (pos < max) {
                c = data.charAt(pos);
                if (c == delimiter) {
                    if (pos < max -1 )
                        ip++;
                }
                pos++;
            }
            ip++; // at least 1

            return ip;
        }


    public UserData getUserData(String name, boolean toCreate) {
        UserData user;
        for (int k = 0; k < userList.size(); k++) {
           user = (UserData)userList.elementAt(k);
           if (name.equals(user.name)) {
                return user;
           }
        }
        if (!toCreate)
           return null;
        user = new UserData(name);
        userList.add(user);
        return user;
    }

    public void setUserProfile(UserData rec) {
        BufferedReader fd = null;
        String fp = userFileDir+rec.name;
        String str, data;
        try  {
                fd = new BufferedReader(new FileReader(fp));
        } catch (FileNotFoundException e) { return; }

        if (fd == null)
            return;
        try  {
          str = fd.readLine();
          while (str != null) {
            /*StringTokenizer tok = new StringTokenizer(str, " ,\n");
            if (tok.hasMoreTokens()) {
                String type = tok.nextToken();
                if (type.equals("userdir")) {
                    data = str.substring(8).trim();
                    rec.userDir = data;
                }
                else if (type.equals("sysdir")) {
                    data = str.substring(7).trim();
                    rec.sysDir = data;
                }
                else if (type.equals("appdir")) {
                    data = str.substring(7).trim();
                    rec.appDir = data;
                }
                else if (type.equals("datadir")) {
                    data = str.substring(8).trim();
                    rec.dataDir = data;
                }
            }*/
            str = fd.readLine();
          }
          fd.close();
          readUserValues(fp, rec);
        } catch (IOException e) { return; }
    }

    protected void readUserValues(String strPath, UserData rec)
    {
        if (strPath == null)
            return;

        try
        {
            FileInputStream fs=new FileInputStream(strPath);
            Properties props=new Properties();
            props.load(fs);

            String strValue = (String)props.get("userdir");
            rec.userDir = (strValue != null) ? strValue.trim() : "";

            strValue = (String)props.get("sysdir");
            rec.sysDir = (strValue != null) ? strValue.trim() : "";

            strValue = (String)props.get("appdir");
            rec.appDir = (strValue != null) ? strValue.trim() : "";

            strValue = (String)props.get("datadir");
            rec.dataDir = (strValue != null) ? strValue.trim() : "";

        }
        catch(Exception e)
        {
            //e.printStackTrace();
        }

    }

    public void setSysProfile(UserData rec) {
        BufferedReader fd = null;
        String fp = sysFileDir+rec.name;
        String str, data;
        try  {
                fd = new BufferedReader(new FileReader(fp));
        } catch (FileNotFoundException e) { return; }

        if (fd == null)
            return;
        try  {
          str = fd.readLine();
          while (str != null) {
            /*StringTokenizer tok = new StringTokenizer(str, " ,\n");
            if (tok.hasMoreTokens()) {
                String type = tok.nextToken();
                if (type.equals("name")) {
                    data = str.substring(5).trim();
                    rec.longName = data;
                }
                else if (type.equals("home")) {
                    data = str.substring(5).trim();
                    rec.homeDir = data;
                }
                else if (type.equals("owned")) {
                    data = str.substring(6).trim();
                    rec.ownedDir = data;
                }
                else if (type.equals("usrlvl")) {
                    data = str.substring(7).trim();
                    rec.accessLevel = data;
                }
                else if (type.equals("access")) {
                    data = str.substring(7).trim();
                    rec.access = data;
                    rec.grpData = data;
                    rec.setGroupList(data);
                }
            }*/
            str = fd.readLine();
          }
          fd.close();
          readSysValues(fp, rec);
        } catch (IOException e) { return; }
    }

    protected void readSysValues(String strPath, UserData rec)
    {
        if (strPath == null)
            return;
        try
        {
            FileInputStream fs=new FileInputStream(strPath);
            Properties props=new Properties();
            props.load(fs);

            String strValue = (String)props.get("name");
            rec.longName = (strValue != null) ? strValue.trim() : "";

            strValue = (String)props.get("home");
            rec.homeDir = (strValue != null) ? strValue.trim() : "";

            strValue = (String)props.get("owned");
            rec.ownedDir = (strValue != null) ? strValue.trim() : "";

            strValue = (String)props.get("usrlvl");
            rec.accessLevel = (strValue != null) ? strValue.trim() : "2";

            strValue = (String)props.get("access");
            strValue = (strValue != null) ? strValue.trim() : "";
            rec.access = strValue;
            rec.grpData = strValue;
            rec.setGroupList(strValue);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
        }
    }

    public void createUserList() {
        UserData newUser;
        String   name;
        BufferedReader fd = null;

        String fp = destDir+userFile;
        String str;
        try  {
                fd = new BufferedReader(new FileReader(fp));
        } catch (FileNotFoundException e) { return; }

        if (fd == null)
            return;
        try  {
          str = fd.readLine();
          while (str != null) {
            if (str.startsWith("#"))
                str = "";
            StringTokenizer tok = new StringTokenizer(str, " ,\n");
            while (tok.hasMoreTokens()) {
                name = tok.nextToken();
                if (name.length() > 0) {
                    newUser = getUserData(name, true);
                    newUser.newAccount = false;
                    newUser.needSave = false;
                    newUser.isRemoved = false;
                }
            }
            str = fd.readLine();
          }
          fd.close();
        } catch (IOException e) { return; }
    }

    public void buildUserList() {
        UserData newUser;
        String   name;
        BufferedReader fd = null;

        String fp = destDir+userFile;
        String d;
        try  {
                fd = new BufferedReader(new FileReader(fp));
        } catch (FileNotFoundException e) { return; }

        if (fd == null)
            return;
        try  {
          d = fd.readLine();
          while (d != null) {
            d = d.trim();
            if ((d.length() > 2) && !(d.startsWith("#"))) {
                name = getToken(d, ':', 1);
                newUser = getUserData(name, true);
                newUser.setLongName(getToken(d, ':', 3));
                newUser.setHomeDir(getToken(d, ':', 4));
                newUser.setDataDir(getToken(d, ':', 5));
                newUser.setAccessLevel(getToken(d, ':', 6));
            }
            d = fd.readLine();
          }
          fd.close();
        } catch (IOException e) { return; }

        fp = destDir+"access";
        try  {
                fd = new BufferedReader(new FileReader(fp));
        } catch (FileNotFoundException e) { return; }

        if (fd == null)
            return;
        try  {
          d = fd.readLine();
          while (d != null) {
            d = d.trim();
            if ((d.length() > 2) && !(d.startsWith("#"))) {
                name = getToken(d, ':', 1);
                newUser = getUserData(name, true);
                newUser.addGroupList(getToken(d, ':', 2));
            }
            d = fd.readLine();
          }
          fd.close();
        } catch (IOException e) { return; }
    }

    public GroupData getGroupData(String name, boolean toCreate) {
        GroupData grp;
        for (int k = 0; k < groupList.size(); k++) {
           grp = (GroupData)groupList.elementAt(k);
           if (name.equals(grp.name)) {
                return grp;
           }
        }
        if (!toCreate)
           return null;
        grp = new GroupData(name);
        groupList.add(grp);
        return grp;
    }

    public void buildGroupList() {
        BufferedReader fd = null;
        String fp = destDir+"group";
        String d;
        try  {
            fd = new BufferedReader(new FileReader(fp));
        } catch (FileNotFoundException e) { return; }

        if (fd == null)
            return;
        try  {
          d = fd.readLine();
          while (d != null) {
            d = d.trim();
            if ((d.length() > 2) && !(d.startsWith("#"))) {
                String name = getToken(d, ':', 1);
                GroupData newGrp = getGroupData(name, true);
                newGrp.setLongName(getToken(d, ':', 2));
                newGrp.addUserList(getToken(d, ':', 3));
            }
            d = fd.readLine();
          }
          fd.close();
        } catch (IOException e) { return; }
    }

    public void addUser2Group() {
        for (int k = 0; k < userList.size(); k++) {
        }
    }

    public static void main(String[] args) {

/*
        setUpDefaultAttr();
        JFrame frame = new JFrame("VNMR Admintool");

        Container contentPane = frame.getContentPane();
        contentPane.setLayout(new GridLayout(1,1));
        VnmrAdmin va = new VnmrAdmin(frame);
        contentPane.add(va);

        frame.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                va.writeFiles();
                System.exit(0);
            }
        });

        frame.pack();
        frame.setVisible(true);
*/
        VnmrAdmin frame = new VnmrAdmin();
        frame.pack();
        frame.setLocation(100, 50);
        frame.setVisible(true);
    }


    class UserPanel extends JPanel {
        private JPanel pans[];
        private JPanel attrPan;
        private JTextField txts[];
        private JLabel labels[];
/*
        private JMenu menu[];
*/
        private JComboBox menu[];
        private SimpleH2Layout xLayout;
        private SimpleH2Layout x2Layout;

        public UserPanel() {
            setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));
            setLayout(new BorderLayout());

            xLayout = new SimpleH2Layout(SimpleH2Layout.LEFT, 4, 0, true, true);
            x2Layout = new SimpleH2Layout(SimpleH2Layout.LEFT, 4, 0, false, true);
            ActionListener act = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                   int k;
                   JComponent comp = (JComponent) evt.getSource();
                   if (comp == txts[UNAME]) {
                        String data = txts[UNAME].getText().trim();
                        if (data.length() > 0)
                                showUser(data);
                   }
                   for (k = 0; k < userItems; k++) {
                      if (comp == txts[k])
                        break;
                   }
                   if (k == ACCESSGRP)
                        k = 0;
                   else k++;
                   txts[k].requestFocus();
                }
            };

            pans = new JPanel[userItems];
            txts = new JTextField[userItems];
            labels = new JLabel[userItems];
/*
            menu = new JMenu[userItems];
*/
/*
            menu = new JComboBox[userItems];
*/
            attrPan = new JPanel();
            attrPan.setLayout(new attrLayout());

            JPanel mp = new JPanel();
            JLabel mlabel = new JLabel("  ");
            userBox = new JComboBox();
            mp.add(mlabel);
            mp.add(userBox);
            mp.setLayout(x2Layout);
            attrPan.add(mp);

            for (int i = 0; i < userItems; i++) {
                pans[i] = new JPanel();
                pans[i].setLayout(xLayout);
                labels[i] = new JLabel(userEntry[i]);
                txts[i] = new JTextField(" ", 20);
                pans[i].add(labels[i]);
                pans[i].add(txts[i]);
/*
                menu[i] = new JComboBox();
                pans[i].add(menu[i]);
                menu[i].setVisible(false);
*/
                attrPan.add(pans[i]);
                txts[i].addActionListener(act);
            }

            labels[ACCESSLVL].setToolTipText("Level could be any integer number");
            labels[ACCESSGRP].setToolTipText("Group could be all or group name");

            add(attrPan, BorderLayout.CENTER);

            userAddButton = new JButton(addUserString);
            userAddButton.setActionCommand("add");
            userAddButton.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        String cmd = e.getActionCommand();
                        if (cmd != null && cmd.equals("add"))
                                addUser(true);
                        else
                                addUser(false);
                    }
            });
            if (legalUser || debug)
                add(userAddButton, BorderLayout.SOUTH);

            if (userList.size() > 0) {
                for (int k = 0; k < userList.size(); k++) {
                    UserData user = (UserData)userList.elementAt(k);
                    userBox.addItem(user.name);
                }
            }
            else
                userBox.setVisible(false);
            userBox.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    String name = (String) userBox.getSelectedItem();
                    setUser(name);
                }
            });
        }

        public void addUser(boolean isNewUser) {
            String data = txts[UNAME].getText().trim();
            if (data.length() < 1)
                return;
            String mesgStrg= "User " + data;
            if (isNewUser) {
                String rtn = createUser( data );
                if (rtn == null) {
                        setLabel("Problem adding " + data);
                        return;
                }
                rtn = rtn.trim();
                if (rtn.equals("invalid user")) {
                        setLabel(mesgStrg + " is an " + rtn);
                        return;
                }
            }
            if (isNewUser)
               setLabel(mesgStrg + "  added");
            else
               setLabel(mesgStrg + "  updated");
            UserData newUser = getUserData(data, true);
            if (newUser == null)
                return;
            for (int k = 1; k < userItems; k++) {
                newUser.setValue(k, txts[k].getText().trim());
            }
            newUser.newAccount = false;
            newUser.isRemoved = false;
            newUser.needSave = true;
            boolean toAdd = true;
            for (int k = 0; k < userBox.getItemCount(); k++) {
                String s = (String) userBox.getItemAt(k);
                if (data.equals(s)) {
                    toAdd = false;
                    break;
                }
            }
            if (toAdd) {
                userBox.addItem(data);
                if (!userBox.isVisible())
                        userBox.setVisible(true);
                userBox.setSelectedItem(data);
            }
            userAddButton.setText(saveUserString);
            userAddButton.setActionCommand("save");
        }

        public void setUser(String name) {
            txts[UNAME].setText(name);
            showUser(name);
        }

        public void showUserData(UserData user) {
            if (user.needLoad) {
                user.needLoad = false;
                setSysProfile(user);
                setUserProfile(user);
            }
            for (int k = 1; k < userItems; k++) {
                txts[k].setText(user.getValue(k));
            }
        }

        public void showUser(String name) {
            String d = null;
            String data;
            String userHomeDir;
            String longName;
            UserData user;
            if (name == null)
                return;
            userAddButton.setText(addUserString);
            userAddButton.setActionCommand("add");
            for (int k = 0; k < userList.size(); k++) {
                user = (UserData)userList.elementAt(k);
                if (name.equals(user.name)) {
                    showUserData(user);
                    if (!user.newAccount) {
                        userAddButton.setText(saveUserString);
                        userAddButton.setActionCommand("save");
                    }
                    return;
                }
            }

            user = getUserData(name, true);
            if (user == null)
                return;

            BufferedReader fd = null;

            try  {
                fd = new BufferedReader(new FileReader("/etc/passwd"));
            } catch (FileNotFoundException e) { return; }

            if (fd == null)
                return;
            try  {
                d = fd.readLine();
                while (d != null) {
                    d = d.trim();
                    if (d.length() > 1) {
                       String pname = getToken(d, ':', 1);
                        if (name.equals(pname)) {
                             break;
                        }
                    }
                    d = fd.readLine();
                }
                fd.close();
            } catch (IOException e) { return; }
            userHomeDir = "";
            longName = "";
            if (d == null)
                d = getUserHomeDir(name);
            if (d == null) {
                data = System.getProperty("user.name");
                if (data != null && data.equals(name)) {
                   userHomeDir = System.getProperty("user.home");
                }
            }
            else {
                data = getToken(d, ':', 5);
                if (data != null)
                    user.longName = data;
                userHomeDir = getToken(d,':', 6);
            }
            user.newAccount = true;
            user.isRemoved = true;
            user.needSave = false;
            user.name = name;
            user.homeDir = userHomeDir;
            user.userDir = userHomeDir+"/vnmrsys";
            user.dataDir = userHomeDir+"/vnmrsys/data"+"  "+userHomeDir+"/vnmrsys/parlib"+"  "+userHomeDir+"/vnmrsys/shims";
            user.appDir = userHomeDir+"/vnmrsys"+"  "+sysDir+"/imaging"+"  "+sysDir;
            user.ownedDir = userHomeDir;
            user.sysDir = sysDir;
            user.accessLevel = "2";
            user.grpData = "all";
            showUserData(user);

        } // showUser

        public void redisplay() {
        }

    }

    class GroupPanel extends JPanel {
        private JPanel pans[];
        private JPanel attrPan;
        private JTextField txts[];
        private JLabel labels[];
        private SimpleH2Layout xLayout;
        private JButton addButton;
        private GroupData curGrp = null;

        public GroupPanel() {
            setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));
            setLayout(new BorderLayout());

            xLayout = new SimpleH2Layout(SimpleH2Layout.LEFT, 4, 0, true, true);
            ActionListener act = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                   int k;
                   JComponent comp = (JComponent) evt.getSource();
                   if (comp == addButton) {
                        addGroup();
                        return;
                   }
                   if (comp == txts[GNAME]) {
                        String data = txts[GNAME].getText().trim();
                        if (data.length() >0)
                             showGroup(data);
                   }
                   for (k = 0; k < groupItems; k++) {
                      if (comp == txts[k])
                        break;
                   }
                   if (k == GUSERS)
                        k = 0;
                   else k++;
                   txts[k].requestFocus();
                }
            };

            pans = new JPanel[groupItems];
            txts = new JTextField[groupItems];
            labels = new JLabel[groupItems];
            attrPan = new JPanel();
            attrPan.setLayout(new attrLayout());
            for (int i = 0; i < groupItems; i++) {
                pans[i] = new JPanel();
                pans[i].setLayout(xLayout);
                labels[i] = new JLabel(groupEntry[i]);
                txts[i] = new JTextField(" ", 20);
                pans[i].add(labels[i]);
                pans[i].add(txts[i]);
                attrPan.add(pans[i]);
                txts[i].addActionListener(act);
            }
            labels[GUSERS].setToolTipText("Users are separated by commas");
            add(attrPan, BorderLayout.CENTER);
            if (legalUser || debug) {
                addButton = new JButton(" Add ");
                add(addButton, BorderLayout.SOUTH);
                addButton.addActionListener(act);
            }
        }

        public void setGroup(String name) {
            txts[GNAME].setText(name);
            showGroup(name);
        }

        public void setGroup(GroupData gp) {
            curGrp = gp;
            txts[GNAME].setText(gp.name);
            txts[LGNAME].setText(gp.longName);
            txts[GUSERS].setText(gp.userData);
        }

        public void showGroup(String name) {
            if (name == null || name.length() < 1)
                return;
            GroupData grp;
            for (int k = 0; k < groupList.size(); k++) {
                grp = (GroupData)groupList.elementAt(k);
                if (name.equals(grp.name)) {
                    curGrp = grp;
                    txts[LGNAME].setText(grp.longName);
                    txts[GUSERS].setText(grp.userData);
                    return;
                }
            }
        }

        public void redisplay() {
            if (curGrp != null)
                setGroup(curGrp);
        }

        public void addGroup() {
            String data = txts[GNAME].getText().trim();
            if (data.length() < 1)
                return;
            GroupData grp = getGroupData(data, true);
            grp.longName = txts[LGNAME].getText().trim();
            grp.setUserList(txts[GUSERS].getText().trim());
            grp.updateUserList();
            for (int k = 0; k < groupList.size(); k++) {
                GroupData g2 = (GroupData)groupList.elementAt(k);
                if (grp == g2)
                    return;
            }
            groupList.add(grp);
            gListPan.updateList();
            setLabel("Group " + data + "  added");
        }
    }


    class GroupListPanel extends JPanel {

       String deleteString = " Delete ";
        JList listBox;
        JScrollPane listScrollPane;
        JButton deleteButton;
        DefaultListModel listModel = new DefaultListModel();

        public GroupListPanel() {
            setBorder(BorderFactory.createEmptyBorder(20, 20, 0, 20));
            setLayout(new BorderLayout());
            updateList();
            listBox = new JList(listModel);
            listBox.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            listBox.setSelectedIndex(0);
            listScrollPane = new JScrollPane(listBox);

            add(listScrollPane, BorderLayout.CENTER);
            if (legalUser || debug) {
                deleteButton = new JButton(deleteString);
                add(deleteButton, BorderLayout.SOUTH);
                deleteButton.addActionListener( new ActionListener() {
                    public void actionPerformed(ActionEvent evt) {
                        deleteGroup();
                        return;
                    }
                });
            }

        }

        public void updateList() {
            String s;
            listModel.removeAllElements();
            for (int k = 0; k < groupList.size(); k++) {
                GroupData grp = (GroupData)groupList.elementAt(k);
                s = grp.name+":"+grp.longName+":"+grp.userData;
                listModel.addElement(s);
            }
        }

        public void deleteGroup() {
            int index = listBox.getSelectedIndex();
            if (index < 0)
               return;
            String d = listBox.getSelectedValue().toString();
            listModel.remove(index);
            if (getTokenNum(d, ':') < 1)
                return;
            String gname = getToken(d, ':', 1);
            if (gname == null || (gname.length() < 1))
                return;
            for (int k = 0; k < groupList.size(); k++) {
                GroupData grp = (GroupData)groupList.elementAt(k);
                if (gname.equals(grp.name))
                   groupList.removeElement(grp);
            }
            setLabel("Group " + gname + "  was deleted");
        }
    }

    class UserListPanel extends JPanel {

       String deleteString = " Delete ";
        JList listBox;
        JScrollPane listScrollPane;
        JButton deleteButton;
        DefaultListModel listModel = new DefaultListModel();

        public UserListPanel() {
            setBorder(BorderFactory.createEmptyBorder(20, 20, 0, 20));
            setLayout(new BorderLayout());
            updateList();
            listBox = new JList(listModel);
            listBox.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            listBox.setSelectedIndex(0);
            listScrollPane = new JScrollPane(listBox);

            add(listScrollPane, BorderLayout.CENTER);
            if (legalUser || debug) {
                deleteButton = new JButton(deleteString);
                add(deleteButton, BorderLayout.SOUTH);
                deleteButton.addActionListener( new ActionListener() {
                    public void actionPerformed(ActionEvent evt) {
                        deleteUser();
                        return;
                    }
                });
            }

        }

        public void updateList() {
            String s;
            listModel.removeAllElements();
            for (int k = 0; k < userList.size(); k++) {
                UserData usr = (UserData)userList.elementAt(k);
                if (!usr.isRemoved) {
                   s = usr.name+"::"+usr.longName+":"+usr.homeDir+":"+usr.dataDir+":"+usr.accessLevel;
                   listModel.addElement(s);
                }
            }
        }

        public void deleteUser() {
            int index = listBox.getSelectedIndex();
            if (index < 0)
               return;
            String d = listBox.getSelectedValue().toString();
            listModel.remove(index);
            if (getTokenNum(d, ':') < 1)
                return;
            String uname = getToken(d, ':', 1);
            if (uname == null || (uname.length() < 1))
                return;
            for (int k = 0; k < userList.size(); k++) {
                UserData ud = (UserData)userList.elementAt(k);
                if (uname.equals(ud.name)) {
                   ud.isRemoved = true;
                   ud.newAccount = true;
/*
                   userList.removeElement(ud);
*/
                }
            }
            for (int k = 0; k < groupList.size(); k++) {
                GroupData grp = (GroupData)groupList.elementAt(k);
                grp.removeUser(uname);
            }
            setLabel("User " + uname + "  was deleted");
            index = userBox.getSelectedIndex();
            if (index >= 0) {
                d = (String) userBox.getItemAt(index);
                if (d.equals(uname))
                   index = 1;
                else
                   index = 0;
            }
            userBox.removeItem(uname);
            if (userBox.getItemCount() <= 0)
                userBox.setVisible(false);
            else if (index == 1)
                userBox.setSelectedIndex(0);
        }
    }



    class UserData {
        public String name;
        public String longName;
        public String homeDir;
        public String userDir;
        public String dataDir;
        public String sysDir;
        public String appDir;
        public String ownedDir;
        public String accessLevel;
        public String access;
        public String grpData;
        public Vector gList;
        public boolean needLoad = true;
        public boolean newAccount = true;
        public boolean needSave = false;
        public boolean isRemoved = false;

        public UserData(String n) {
            this.name = n;
            this.longName = "";
            this.homeDir = "";
            this.userDir = "";
            this.dataDir = "";
            this.sysDir = "";
            this.appDir = "";
            this.ownedDir = "";
            this.accessLevel = "2";
            this.access = "all";
            gList = new Vector();
            this.grpData = "all";
        }

        public String getValue(int type) {
            switch (type) {
                case UNAME:
                           return name;
                case ULNAME:
                           return longName;
                case HOMEDIR:
                           return homeDir;
                case USERDIR:
                           return userDir;
                case SYSDIR:
                           return sysDir;
                case DDIR:
                           return dataDir;
                case SEARCHDIR:
                           return appDir;
                case OWNEDDIR:
                           return ownedDir;
                case ACCESSLVL:
                           return accessLevel;
                case ACCESSGRP:
                           return grpData;
            }
            return null;
        }

        public void setValue(int type, String s) {
            switch (type) {
                case UNAME:
                           name = s;
                           break;
                case ULNAME:
                           longName = s;
                           break;
                case HOMEDIR:
                           homeDir = s;
                           break;
                case USERDIR:
                           userDir = s;
                           break;
                case SYSDIR:
                           sysDir = s;
                           break;
                case DDIR:
                           dataDir = s;
                           break;
                case SEARCHDIR:
                           appDir = s;
                           break;
                case OWNEDDIR:
                           ownedDir = s;
                           break;
                case ACCESSLVL:
                           accessLevel = s;
                           break;
                case ACCESSGRP:
                           if (s == null)
                              grpData = "";
                           else
                              grpData = s;
                           setGroupList();
                           break;
            }
        }

        public void setLongName(String n) {
            this.longName = n;
        }

        public void setHomeDir(String n) {
            this.homeDir = n;
        }

        public void setDataDir(String n) {
            this.dataDir = n;
        }

        public void setAccessLevel(String n) {
            this.accessLevel = n;
        }

        public void addGroupList(String n) {
            if (grpData.length() > 0)
               grpData = grpData+", "+n;
            else
               grpData = n;
            setGroupList();
        }

        public void setGroupList(String n) {
            if (n == null)
               grpData = "";
            else
               grpData = n;
            setGroupList();
        }

        public void setGroupList() {
            if (gList != null)
                gList.clear();
            else
                gList = new Vector();
            String tok;
            String gname;
            boolean isExist;
            for (int k = 0; k < getTokenNum(grpData, ','); k++) {
                tok = getToken(grpData, ',', k+1);
                if (tok != null) {
                    gname = tok.trim();
                    if (gname.length() > 0) {
                        isExist = false;
                        for (int x = 0; x < gList.size(); x++) {
                            String grp = (String) gList.elementAt(x);
                            if (gname.equals(grp)) {
                                isExist = true;
                                break;
                            }
                        }
                        if (!isExist)
                            gList.add(gname);
                    }
                }
            }
        }

        public void addGroup(String g) {
            if (g == null)
                return;
            String gname = g.trim();
            if (gname.length() <= 0)
                return;
            for (int k = 0; k < gList.size(); k++) {
                String s = (String) gList.elementAt(k);
                if (gname.equals(s))
                    return;
                if (gname.equals("all"))
                    return;
            }
            gList.add(gname);
            if (grpData.length() > 0)
               grpData = grpData+", "+gname;
            else
               grpData = gname;
        }

        public void updateGroupList() {
            boolean toAll = false;
            if (name == null || (name.length() < 1))
                return;
            for (int k = 0; k < gList.size(); k++) {
                String gname = (String) gList.elementAt(k);
                if (gname.equals("all")) {
                    toAll = true;
                    break;
                }
            }
            if (toAll) {
                for (int k = 0; k < groupList.size(); k++) {
                   GroupData grp = (GroupData)groupList.elementAt(0);
                   grp.addUser(name);
                }
                return;
            }
            for (int k = 0; k < gList.size(); k++) {
                String s = (String) gList.elementAt(k);
                GroupData grp = getGroupData(s, false);
                if (grp != null)
                   grp.addUser(name);
            }
        }

        public void removeGroup(String g) {
            if (g == null || (g.length() < 1))
                return;
            for (int k = 0; k < gList.size(); k++) {
                String gname = (String) gList.elementAt(k);
                if (gname.equals(g)) {
                     gList.removeElement(gname);
                }
            }
        }

        public boolean inGroup(String g) {
            if (g == null || (g.length() < 1))
                return false;
            for (int k = 0; k < gList.size(); k++) {
                String gname = (String) gList.elementAt(k);
                if (gname.equals("all"))
                    return true;
                if (gname.equals(g))
                    return true;
            }
            return false;
        }
    }

    class GroupData {
        public String name;
        public String longName;
        public String userData;
        public Vector uList;

        public GroupData(String n) {
            this.name = n;
            this.longName = "";
            userData = "";
            uList = new Vector();
        }

        public void setLongName(String n) {
            this.longName = n;
        }

        public void addUserList(String n) {
            if (userData.length() > 0)
               userData = userData+", "+n;
            else
               userData = n;
            setUserList();
        }

        public void setUserList(String n) {
            if (n == null)
                userData = "";
            else
                userData = n;
            setUserList();
        }


        public void setUserList() {
            if (uList != null)
                uList.clear();
            else
                uList = new Vector();
            String tok;
            String s;
            int k;
            boolean isExist;
            for (k = 0; k < getTokenNum(userData, ','); k++) {
                tok = getToken(userData, ',', k+1);
                if (tok != null) {
                    s = tok.trim();
                    if (s.length() > 0) {
                        isExist = false;
                        for (int x = 0; x < uList.size(); x++) {
                            String user = (String) uList.elementAt(x);
                            if (s.equals(user)) {
                                isExist = true;
                                break;
                            }
                        }
                        if (!isExist)
                            uList.add(s);
                    }
                }
            }
        }

        public void addUser(String u) {
            if (u == null)
                return;
            String user = u.trim();
            if (user.length() <= 0)
                return;
            for (int k = 0; k < uList.size(); k++) {
                String uname = (String) uList.elementAt(k);
                if (uname.equals(user))
                    return;
            }
            uList.add(user);
            if (userData.length() > 0)
               userData = userData+", "+user;
            else
               userData = user;
        }

        public void removeUser(String u) {
            if (u == null)
                return;
            String user = u.trim();
            if (user.length() <= 0)
                return;
            userData = "";
            for (int k = 0; k < uList.size(); k++) {
                String uname = (String) uList.elementAt(k);
                if (uname.equals(user))
                    uList.removeElement(user);
            }
            for (int k = 0; k < uList.size(); k++) {
                if (k == 0)
                     userData = (String) uList.elementAt(k);
                else
                     userData = userData+", "+ (String) uList.elementAt(k);
            }
        }

        public void updateUserList() {
            for (int k = 0; k < uList.size(); k++) {
                String uname = (String) uList.elementAt(k);
                UserData newUser = getUserData(uname, false);
                if (newUser != null)
                    newUser.addGroup(name);
            }
        }
    }


    class attrLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}
        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            Dimension dim;
            int   w = 0;
            int   h = 0;
            int   k;
            int   n = target.getComponentCount();
            for ( k = 0; k < n; k++) {
                Component m = target.getComponent(k);
                dim = m.getPreferredSize();
                if (dim.width > w)
                    w = dim.width;
                h += dim.height+2;
            }
            return new Dimension(w, h);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim;
                int   n = target.getComponentCount();
                int   w = 0;
                int   w2 = 0;
                int   h = 4;
                int   k;
                Component m, m1;

                for ( k = 0; k < n; k++) {
                    m = target.getComponent(k);
                    if(m instanceof Container) {
                        m1 =((Container)m).getComponent(0);
                        if (m1 != null) {
                            dim = m1.getPreferredSize();
                            if (dim.width > w)
                                w = dim.width;
                        }
                        int n2 = ((Container)m).getComponentCount();
                        if (n2 > 2) {
                            m1 =((Container)m).getComponent(2);
                            if (m1 != null) {
                                dim = m1.getPreferredSize();
                                if (dim.width > w2)
                                    w2 = dim.width;
                            }
                        }
                    }
                }
                m = null;
                m1 = null;

                SimpleH2Layout.setFirstWidth(w);
                SimpleH2Layout.setLastWidth(w2);

                Dimension dim0 = target.getSize();
                for (k = 0; k < n; k++) {
                    Component obj = target.getComponent(k);
                    if (obj.isVisible()) {
                        dim = obj.getPreferredSize();
                        obj.setBounds(2, h, dim0.width-4, dim.height);
                        h += dim.height;
                    }
                }
            }
        }
    } // class attrLayout

}
