// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
//LoadNmr.java

//output to  ins_vnmr are the following:
//acq_pid, main option, source_dir, dest_dir,  pipe, online_help link,
//vnmr link, machine(sos, sol, ibm, sgi), choice ...

import java.text.*;
import javax.swing.*;
import javax.swing.border.Border;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import javax.swing.event.*;
import java.net.*;

public class LoadNmr extends JFrame {

    private static int NUM_LABEL = 3;
    public static String baseDir   = null;
    public static String codeDir   = null;
    public static String destDir   = null;
    public static String nmrUser   = null;
    public static String userDir   = null;
    public static String nmrGroup  = null;
    public static String osVersion = null;
    public static String nmradminemail = null;
    public static boolean showSol = false;
    //public static String leCard    = null;
    //public static String hmeCard   = null;
    //public static StringTokenizer nicTokStrg = null;
    int nicCnt = 0;
    String selectedNic = null;

    int numTab = 8;
    //int numTab = 6;
    int numField = 30;
    int curTabIndexOffset = 0;
    int curTabIndex = 0;
    int selectTabIndex = 0;
    boolean setAcq = false;
    boolean setupDB = true;
    boolean loadVNMR = true;
    //String nmrDb = "no";
    String nmrLink = "yes";
    String manLink = "no";
    boolean notEnoughSpace = false;
    boolean rebootConsole = false;
    boolean userExist = true;
    boolean bMr400 = false;

    String   buttonString[] = {"Install", "Quit"};
    JButton[]  bt = new JButton[buttonString.length];

    JTextField[]        comTextField = null;
    JCheckBox[][]       chkBoxOpt = new JCheckBox[numTab][];
    JCheckBox[]         setacqChkBox = new JCheckBox[numTab];
    JCheckBox           nmrdbChkBox = new JCheckBox();
    JCheckBox 		nmrlkChkBox;
    JComboBox[]         nicComboBox = new JComboBox[numTab];
    CheckBoxListener    chkboxListener = new CheckBoxListener();

    JLabel[]		txFldLabel;
    JLabel[]		txFldDefault;
    JLabel              sumFixLabel = new JLabel("                           Total selected :  ");
    JPanel              sumPanel = new JPanel(new BorderLayout());
    JPanel		labelPane = new JPanel(new GridLayout(0,1));
    JPanel		txFldPane = new JPanel(new GridLayout(0,1));
    JPanel		defaultPane = new JPanel(new GridLayout(0,1));
    JPanel              panel3 = new JPanel(new FlowLayout());
    JPanel              btmPane = new JPanel();
    JPanel              dbMainPan = new JPanel(new FlowLayout());
    JPanel              lkMainPan = new JPanel(new FlowLayout());
    JPanel              panel4 = new JPanel();
    JPanel              panel5 = new JPanel();
    JLabel[][]          passwordLabel = new JLabel[numTab][];
    JPasswordField[][]  passwordField = new JPasswordField[numTab][];
    //JTabbedPane            tabbedPane = new JTabbedPane();

    int grandSum[] = { 0, 0, 0, 0, 0, 0 };
    Vector vecGen[] = new Vector[numTab];
    Vector vecOpt[] = new Vector[numTab];

    JPanel rtPane[] = new JPanel[numTab];
    JLabel sumLabel = new JLabel();

    //400MR
    String consoleType[]   = {"propulse", "vnmrs",    "mr400",   "inova",
			      "mercplus", "mercvx",  "mercury", "vnmrsdd2", "mr400dd2",
                              "g2000",    "uplus",   "unity",
			     };
    String progressTitleConsole[] = {	"ProPulse", "VNMRS", "400-MR", "VNMRSDD@", "400-MRDD2",
					"INOVA", "MERCURYplus/Vx"};


    protected ProgressMonitor m_progb;

    protected static final String EXITMSG =
		">>>>> OpenVnmrJ Installation Program Exited <<<<<";
    ImageIcon icons[] = {null, null, null, null, null, null};

    // These seems to be used only as the icon for the Frame and
    // are NOT the ones used for the Tab images.
    ImageIcon vnsIcon = new ImageIcon(codeDir+"/icon/vnmrs.gif");
    ImageIcon mr4Icon = new ImageIcon(codeDir+"/icon/mr400.gif");
    ImageIcon inoIcon = new ImageIcon(codeDir+"/icon/inova.gif");
    ImageIcon mplIcon = new ImageIcon(codeDir+"/icon/mercplus.gif");
    ImageIcon mvxIcon = new ImageIcon(codeDir+"/icon/mercvx.gif");
    ImageIcon datIcon = new ImageIcon(codeDir+"/icon/datastn.gif");


    String   labelString[] = { "OpenVnmrJ home directory  :   ",
                               "User name  :   ",
                               "Group name  :   ",
                               "Administrator email :"
                             };

    String    labelDefaultValues[] = { "/vnmr",
			               getVnmrJOwner(),
				       getVnmrJGroup(),
				       ""
                                     };

    public LoadNmr() {
        int dotnvExists = 0;
        setTitle( "Load OpenVnmrJ Software" );
///cdrom/code/.nv
	// File xx = new File(codeDir +"/.nv");
	// if (xx.exists()) {
            dotnvExists=1;
	    setIconImage(vnsIcon.getImage());
        // }
        // else {
        //     dotnvExists = 0;
	//     setIconImage(inoIcon.getImage());
        // }

        if ((getVnmrVersion().equals("OpenVnmrJ_SE"))) {
    	    NUM_LABEL = 4;
	    nmradminemail = labelDefaultValues[3];
	}
        setUIAttrs();

        comTextField = new JTextField[NUM_LABEL];

        makeBtmPane();
        JTabbedPane tabbedPane = new JTabbedPane();
        tabbedPane.setBackground(Color.gray);

	if (dotnvExists==1) {
            String origVersion = checkPreviousVnmrVersion();
            curTabIndexOffset = 0;
            // showSol=true;   // to show Solaris and 400MR
	    if (osVersion.equals("rht") || showSol)
            {
               String path = codeDir+"/rht/install";
               File f = new File(path);
               int ok = 0;
               if (f.exists())
               {
                  ok = 1;
                  BufferedReader fin = null;
                  try {
                      fin = new BufferedReader(new FileReader(path));
                  } catch(FileNotFoundException e1) {
                      ok = 0;;
                  }
                  if (ok == 1)
                  {
                     String line;
                     int index = 0;
                     try {
                         // This is where the icons for the tab images are created
                         while ((line = fin.readLine()) != null) {
                            consoleType[index] = line;
                            String basePath = codeDir+"/icon/"+line;
                            String imgPath = "";
                            // Use .png OR .gif icon files
                            String[] extns = {".png", ".gif"};
                            for (String extn : extns) {
                                imgPath = basePath + extn;
                                if (new File(imgPath).exists()) {
                                    break;
                                }
                            }
                            icons[index] = new ImageIcon(imgPath);
                            // invisible "filler" label makes tab taller
                            int h = 10 + icons[index].getIconHeight() / 2;
                            tabbedPane.addTab("<html><body marginheight=" + h + ">",
                               icons[index],
                               createEntryPanel( (int) index ),
                               fin.readLine());
                            progressTitleConsole[index] =  fin.readLine();
			    if ((origVersion != null) && (origVersion.compareTo(line) == 0) ) {
                               selectTabIndex = index;
                             }
                             index += 1;
                         }
                     } catch(IOException e2) {}

                     try {
                         fin.close();
                     } catch(Exception e3) {}
                  }
               }
               if (ok == 0)
               {
                  tabbedPane.addTab(null, vnsIcon, createEntryPanel( (int) 0 ),
                          "Load VNMRS software");
                  tabbedPane.addTab(null, mr4Icon, createEntryPanel( (int) 1 ),
                          "Load 400-MR software");
               }
            }
           
        //400MR
	  /*  if (osVersion.equals("rht"))
          /*      tabbedPane.addTab(null, mr4Icon, createEntryPanel( (int) 0 ),
          /*                "Load 400-MR software");
           */
        } else {
            curTabIndexOffset = 2;
            selectTabIndex = 2;
            curTabIndex = curTabIndexOffset;
            tabbedPane.addTab(null, inoIcon, createEntryPanel( (int) 2 ),
                          "Load Inova software");
            tabbedPane.addTab(null, mplIcon, createEntryPanel( (int) 3 ),
                          "Load MERCURYplus software");
        //    tabbedPane.addTab(null, mvxIcon, createEntryPanel( (int) 4 ),
	//		"Load Mercury VX software");
        }
        //tabbedPane.addTab("  Mercury  ", null, createEntryPanel( (int) 2 ),
	//			"Load Mercury software");
        //tabbedPane.addTab(" GEMINI 2000 ", null, createEntryPanel( (int) 3 ),
	//			"Load Gemini software");
        //tabbedPane.addTab(" UNITYplus ", null, createEntryPanel( (int) 4 ),
	//			"Load Unity+ software");
        //tabbedPane.addTab(" Unity/VXR-S ", null, createEntryPanel( (int) 5 ),
 	//			"Load Unity/VXR-S software");

        tabbedPane.addChangeListener(new TabbedListener());
        getContentPane().setLayout(new BorderLayout());
        getContentPane().add(tabbedPane, BorderLayout.CENTER);
        getContentPane().add(btmPane, BorderLayout.SOUTH);
        getContentPane().setBackground(Color.lightGray);
	addWindowListener();
        //nicTokStrg = getNIC();
        //nicCnt = nicTokStrg.countTokens();
        curTabIndex = selectTabIndex;
        setSumLabel(" " + grandSum[curTabIndex] + "  KB");
        tabbedPane.setSelectedIndex(selectTabIndex);

        setLocation( 200, 10 );
	pack();
    }

    protected void addWindowListener()
    {
        this.addWindowListener(new WindowAdapter()
	{
	    public void windowClosing(WindowEvent e)
	    {
	        System.out.println();
		if (m_progb != null)
		    System.out.println(m_progb.getExitMesg());
		else
		    System.out.println(EXITMSG);
		System.out.println();
	    }

	    public void windowOpened(WindowEvent e) {
                 if (comTextField != null && comTextField[0] != null)
                     comTextField[0].requestFocus();
	    }
	});
    }

    class TabbedListener implements ChangeListener {
        public void stateChanged(ChangeEvent e) {
            JTabbedPane source = (JTabbedPane)e.getSource();
            curTabIndex = source.getSelectedIndex() + curTabIndexOffset;
            setIconImage(icons[curTabIndex].getImage());
/*
	    if (curTabIndex == 0) setIconImage(vnsIcon.getImage());
	    if (curTabIndex == 1) setIconImage(inoIcon.getImage());
	    if (curTabIndex == 2) setIconImage(mplIcon.getImage());
	    if (curTabIndex == 3) setIconImage(mvxIcon.getImage());
 */

            setSumLabel(" " + grandSum[curTabIndex] + "  KB");
            Enumeration eVecGen = vecGen[curTabIndex].elements();
            boolean foundVnmr = false;
            while(eVecGen.hasMoreElements()) {
                if (eVecGen.nextElement().equals("VNMR")) {
                    foundVnmr=true;
                    break;
                }
            }
            if (foundVnmr) {
                panel3.remove(defaultPane);
                panel3.add(txFldPane);
                nmrLink = "yes";
                setupDB = true;
                loadVNMR = true;
//                nmrlkChkBox.setEnabled(true);
                nmrdbChkBox.setEnabled(true);
            }
            else {
                panel3.remove(txFldPane);
                panel3.add(defaultPane);
                nmrLink = "no";
                setupDB = false;
                loadVNMR = false;
//                nmrlkChkBox.setEnabled(false);
                nmrdbChkBox.setEnabled(false);
            }
        }
    }

    private void makeBtmPane() {
        sumLabel = new JLabel("0");
        sumLabel.setForeground(Color.red);
        sumPanel.add(sumFixLabel, BorderLayout.WEST);
        sumPanel.add(sumLabel, BorderLayout.CENTER);

        txFldLabel = new JLabel[NUM_LABEL];
        txFldDefault = new JLabel[NUM_LABEL];

        for (int i = 0; i < NUM_LABEL; i++) {

            txFldLabel[i]  = new JLabel(labelString[i]);
            txFldDefault[i] = new JLabel(labelDefaultValues[i]);
            comTextField[i]  = new JTextField(numField);
            txFldLabel[i].setLabelFor(comTextField[i]);

            labelPane.add(txFldLabel[i]);
            txFldPane.add(comTextField[i]);
            defaultPane.add(txFldDefault[i]);
        }

        comTextField[0].setText(destDir);
        comTextField[1].setText(nmrUser);
        comTextField[2].setText(nmrGroup);
	if (NUM_LABEL == 4)
            comTextField[3].setText(nmradminemail);

        //Put the panels in another panel, labels on left, text fields on right.
        panel3.setBorder(BorderFactory.createEmptyBorder(10, 20, 10, 20));
        panel3.add(labelPane);
/*        panel3.add(txFldPane); */
/*
/*        nmrdbChkBox = new JCheckBox("Set up database for this user.                              ");
/*        nmrlkChkBox = new JCheckBox("Link /vnmr to VnmrJ file system.                          ");
/* */
/*        JButton   dumButton1  = new JButton("dum1");
/*        JButton   dumButton2  = new JButton("dum2");
/*
/*        dbMainPan.add(nmrdbChkBox);
/*        dbMainPan.add(dumButton1);
/*
/*        lkMainPan.add(nmrlkChkBox);
/*        lkMainPan.add(dumButton2);
/*
/*        nmrdbChkBox.setName("nmrdb");
/*        nmrdbChkBox.addItemListener(chkboxListener);
/*        nmrdbChkBox.doClick();
/*
/*        nmrlkChkBox.setName("nmrlink");
/*        nmrlkChkBox.addItemListener(chkboxListener);
/*        nmrlkChkBox.doClick();
/**/
        panel4.setLayout(new BoxLayout(panel4, BoxLayout.Y_AXIS));

/*
/*        dumButton1.setVisible(false);
/*        dumButton2.setVisible(false);
/* */
        panel4.add(lkMainPan);
        panel4.add(dbMainPan);

        for (int i = 0; i < buttonString.length; i++) {
            bt[i] = new JButton(buttonString[i]);
            bt[i].setActionCommand(buttonString[i]);
            bt[i].addActionListener( new ButtonListener() );
            panel5.add(bt[i]);
        }


        btmPane.setLayout( new BoxLayout(btmPane, BoxLayout.Y_AXIS) );
        btmPane.setBorder( BorderFactory.createEtchedBorder() );

        btmPane.add(sumPanel);
//           btmPane.add(new JSeparator());
        btmPane.add(panel3);
        btmPane.add(panel4);
        btmPane.add(panel5);
	if (osVersion.equals("rht"))
	    btmPane.add(new JLabel(datIcon));
    }

    private JPanel createEntryPanel( int index ){

       curTabIndex = index;

       vecGen[index] = new Vector();
       vecOpt[index] = new Vector();

       JPanel panel1 = new genPanel(index);

       JPanel panel2 = new optPanel(index);


/*       StringTokenizer nicTokStrg = getNIC();
/*
/*      if  ( nicTokStrg != null )
/*      {
/*          nicCnt = nicTokStrg.countTokens();
/*
/*          JPanel acqMainPan = new JPanel();
/*          //acqMainPan.setLayout(new BoxLayout(acqMainPan, BoxLayout.Y_AXIS));
/*          //acqMainPan.setLayout(new BorderLayout());
/*          acqMainPan.setLayout(new FlowLayout());
/*
/*          //JCheckBox setacqChkBox = new JCheckBox("Do setacq, Spectrometer connects to SUN via :");
/*          setacqChkBox[index] = new JCheckBox("Do setacq, Spectrometer connects to SUN via :");
/*
/*          setacqChkBox[index].setName("setacq");
/*          setacqChkBox[index].addItemListener(chkboxListener);
/*
/*          //acqMainPan.add(setacqChkBox[index], BorderLayout.WEST);
/*          acqMainPan.add(setacqChkBox[index]);
/*
/*          String[] nicLabel = new String[nicCnt];
/*
/*          int cnt=0;
/*          while(nicTokStrg.hasMoreTokens()) {
/*             nicLabel[cnt] = nicTokStrg.nextToken().trim();
/*             cnt++;
/*          }
/*
/*          nicComboBox[index] = new JComboBox(nicLabel);
/*
/*          if(nicCnt == 1)
/*             nicComboBox[index].setSelectedIndex(0);
/*          else
/*             nicComboBox[index].setSelectedIndex(1);
/*
/*          //selectedNic = (String) nicComboBox[index].getSelectedItem();
/*
/*          nicComboBox[index].addActionListener(new ActionListener() {
/*             public void actionPerformed(ActionEvent e) {
/*                JComboBox cb = (JComboBox)e.getSource();
/*
/*                //pass the nic name to script that calls setacq
/*
/*                if ( setAcq && nmrLink.equals("yes") )
/*                   selectedNic = (String)cb.getSelectedItem();
/*             }
/*          });
/*
/*          //acqMainPan.add(nicComboBox[index], BorderLayout.CENTER);
/*          acqMainPan.add(nicComboBox[index]);
/*          //will revisit when we have clearer direction on how to do auto setacq
/*          //panel4.add(acqMainPan);
/*
/*          nicComboBox[curTabIndex].setVisible(true);
/*       }
/* */

       rtPane[index] = new JPanel();
       rtPane[index].setLayout(new tabPanLayout());
       rtPane[index].add(panel1);
       rtPane[index].add(new JSeparator());
       rtPane[index].add(panel2);

       return rtPane[index];
    }


    public int chkAvailSpace() {
       String strg = null;
         try {

            String[] cmd = {"/bin/sh", "-c", codeDir + "/" + "checkspace" + " " + destDir};

            Runtime rt = Runtime.getRuntime();
            Process prcs = rt.exec(cmd);

            InputStream istrm = prcs.getInputStream();
            BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

            strg = bfr.readLine();
         }

         catch (IOException e)
         {
            System.out.println(e);
         }

	 if (strg == null)
		return 0;

	int mb;
        try {
            mb = Integer.parseInt(strg);
        } catch (NumberFormatException ex) {
            if(strg.length() > 9) mb = Integer.MAX_VALUE;
            else mb = 0;
        }

	return mb;
    }


    public String getVnmrVersion() {
       String strg = "";
         try {

            String[] cmd = {"/bin/sh", "-c", "grep VERSION " + baseDir + "/vnmrrev 2>/dev/null | awk '{print $1}'"};
            Runtime rt = Runtime.getRuntime();
            Process prcs = rt.exec(cmd);

            InputStream istrm = prcs.getInputStream();
            BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

            strg = bfr.readLine();
         }

         catch (IOException e)
         {
            System.out.println(e);
         }

         return strg;
    }


    private String getVnmrJOwner() {
        File vnmr = new File("/vnmr");
        String strg = null;
        if (vnmr.exists()) {
            try {
                Runtime rt = Runtime.getRuntime();
		String[] cmd = {"/bin/sh","-c","cd /vnmr;ls -l vnmrrev"};
                Process prcs = rt.exec(cmd);

                InputStream is = prcs.getInputStream();
                BufferedReader bfr = new BufferedReader(
                                     new InputStreamReader(is));

                strg = bfr.readLine();
            }
            catch (IOException e)
            {
                 System.out.println(e);
                 return(null);
            }
            if (strg != null) {
                StringTokenizer  st = new StringTokenizer(strg," ");
                if (st.countTokens() > 2) {
                    st.nextToken();
		    st.nextToken();
                    String name = st.nextToken();
		    return(name);
                }
            }
        }
        return(null);
    }


    private String getVnmrJGroup() {
        File vnmr = new File("/vnmr");
        String strg = null;
        if (vnmr.exists()) {
            try {
                Runtime rt = Runtime.getRuntime();
		String[] cmd = {"/bin/sh","-c","cd /vnmr;ls -l vnmrrev"};
                Process prcs = rt.exec(cmd);

                InputStream is = prcs.getInputStream();
                BufferedReader bfr = new BufferedReader(
                                     new InputStreamReader(is));

                strg = bfr.readLine();
            }
            catch (IOException e)
            {
                 System.out.println(e);
            }
            if (strg != null) {
                StringTokenizer  st = new StringTokenizer(strg," ");
                if (st.countTokens() > 3) {
                    st.nextToken();
                    st.nextToken();
                    st.nextToken();
                    String name = st.nextToken();
	 	    return(name);
                }
            }
        }
        return(null);
    }

    private String checkPreviousVnmrVersion() {
        String path = File.separator+"vnmr"+File.separator+"vnmrrev";
        File f = new File(path);
        if (!f.exists())
            return null;
        BufferedReader fin = null;
        try {
            fin = new BufferedReader(new FileReader(path));
        } catch(FileNotFoundException e1) {
            return null;
        }
        String line;
        try {
            line = fin.readLine();
            line = fin.readLine();
            line = fin.readLine();
        } catch(IOException e2) { return null;}

        try {
            fin.close();
        } catch(Exception e3) { return null;}
        return line;
    }


    private boolean userExistCheck(String user) {
        String strg = null;
        try {
            Runtime rt = Runtime.getRuntime();
	    String[] cmd = {"/bin/sh","-c","getent passwd "+user };
            Process prcs = rt.exec(cmd);

            InputStream is = prcs.getInputStream();
            BufferedReader bfr = new BufferedReader(
                                     new InputStreamReader(is));

            strg = bfr.readLine();
        }
        catch (IOException e)
        {
             System.out.println(e);
        }
        if (strg != null)
            return (true);
        else
            return(false);
    }

    //for Install button only
    class ButtonListener implements ActionListener {
        public void actionPerformed(ActionEvent evnt) {

           String genList = "+";
           String optList = "+";

           JButton bttn = (JButton) evnt.getSource();
           String actionStr = evnt.getActionCommand();

           if (actionStr.equals("Install")) {
               if (grandSum[curTabIndex] <= 0)
                  return;

               if ( loadVNMR ) {
                    destDir = comTextField[0].getText().trim();
               }
               else {
                    destDir = labelDefaultValues[0];
               }
	       if ((destDir == null) || (destDir.length() < 1)) {
                     JOptionPane.showMessageDialog(getContentPane(),
				"OpenVnmrJ home directory field is empty",
				"Error",
				JOptionPane.ERROR_MESSAGE);
                     return;
	       }

               //check for available space
               int spc = chkAvailSpace();

               if(spc < grandSum[curTabIndex]+10000) {
                  JOptionPane.showMessageDialog(getContentPane(),
			"Not enough disk space,\n" + spc + " Bytes  available from the selected partition.",
			"Error",
			JOptionPane.ERROR_MESSAGE);
                  return;
               }

               if(rebootConsole) {
                  JOptionPane.showMessageDialog(getContentPane(), "Because Do setacq option selected.\n Please, reboot the console before proceeding with the installation");
               } else {
                    //selectedNic = null;
                 }

               String nmruser;
               String nmrgroup;
               if ( loadVNMR ) {
                    nmruser       = comTextField[1].getText().trim();
                    nmrgroup      = comTextField[2].getText().trim();
		    if (NUM_LABEL == 4)
                       nmradminemail = comTextField[3].getText().trim();
               }
               else {
                    nmruser       = labelDefaultValues[1];
                    nmrgroup      = labelDefaultValues[2];
		    if (NUM_LABEL == 4)
                       nmradminemail = labelDefaultValues[3];
               }

	       if ((nmruser == null) || (nmruser.length() < 1)) {
                     JOptionPane.showMessageDialog(getContentPane(),
				"User name field is empty",
				"Error",
                                JOptionPane.ERROR_MESSAGE);

                     return;
	       }
 	       /*if ((userDir == null) || (userDir.length() < 1)) {
                     JOptionPane.showMessageDialog(getContentPane(),
				"User directory field is empty",
				"Error",
                                JOptionPane.ERROR_MESSAGE);
                     return;
	       }*/
 	       if ((nmrgroup == null) || (nmrgroup.length() < 1)) {
                     JOptionPane.showMessageDialog(getContentPane(),
				"Group name field is empty",
				"Error",
				JOptionPane.ERROR_MESSAGE);
                     return;
	       }
               if ( ! userExistCheck(nmruser) ) {
                   userExist = false;
               }

/*               if(!nmrgroup.equals("nmr")) {
/*                  if (! getItem(nmrgroup, "/etc/group")) {
/*                     JOptionPane.showMessageDialog(getContentPane(),
/*				"Not a valid group",
/*				"Error",
/*                               JOptionPane.ERROR_MESSAGE);
/*                     return;
/*                  }
/*               }
/* */
               bttn.setEnabled(false);
               bt[0].setEnabled(false); //Install button
               bt[1].setEnabled(false); //Quit button

               Enumeration eVecGen = vecGen[curTabIndex].elements();
               Enumeration eVecOpt = vecOpt[curTabIndex].elements();

               while(eVecGen.hasMoreElements())
                  genList = genList + eVecGen.nextElement() + "+";

               while(eVecOpt.hasMoreElements()) {
                  int idx = Integer.parseInt( (String) eVecOpt.nextElement() );
                  String pwFld = new String( passwordField[curTabIndex][idx].getPassword() );
                  if( pwFld.length() == 0 )
                     pwFld = "xxxxxxx";
                  optList = optList + chkBoxOpt[curTabIndex][idx].getActionCommand() + "+" + pwFld + "+";
               }

	       String consoletype = consoleType[curTabIndex];

               m_progb = new ProgressMonitor( grandSum[curTabIndex],
				osVersion, nmrLink, manLink,
                                consoletype, genList, optList,
			        setupDB, nmruser, nmrgroup, codeDir,
				destDir, userDir, selectedNic, nicCnt,
				userExist, progressTitleConsole[curTabIndex]);
               m_progb.addWindowListener(new WindowAdapter()
	       {
                   public void windowClosing(WindowEvent e)
		   {
		        System.out.println();
			System.out.println(m_progb.getExitMesg());
			System.out.println();
                        System.exit(0);
                   }
               });

               m_progb.setVisible(true);
               m_progb.go();   //start the actual installation
               setEnabled(false);

	       if ((NUM_LABEL == 4) && (nmradminemail.length() > 1)) {
         	  try {
            	      String[] cmd = {"/bin/sh", "-c", "echo " + nmradminemail + " >> " + destDir + "/adm/p11/notice"};

            	      Runtime rt = Runtime.getRuntime();
            	      Process prcs = rt.exec(cmd);
         	  }
         	  catch (IOException e) { System.out.println(e); }
	       }

           } else if (actionStr.equals("Quit")) {

               try {
                 Runtime rt = Runtime.getRuntime();
                 Process prcs = rt.exec(codeDir + "/" + "kill_insvnmr");
		 System.out.println();
		 System.out.println(EXITMSG);
		 System.out.println();
               }
               catch (IOException e) { System.out.println(e); }


               System.exit(0);

           } else if (actionStr.equals("Help")) {

               bttn.setEnabled(false);

           }
        }
    }

    //place NIC names in "nicTokStrg"
    //public static StringTokenizer nicTokStrg = null;
    public StringTokenizer getNIC() {

       String targetFile = "/etc/path_to_inst";
       StringTokenizer nicTokStrg = null;

       try {

          FileReader fr = new FileReader(targetFile);
          BufferedReader br = new BufferedReader(fr);

          String line = null;
          String nicStrg = " ";

          while((line = br.readLine()) != null) {
             StringTokenizer tokStrg = new StringTokenizer(line, "\"");

             if(tokStrg.countTokens() == 3) {

                String notUsed = tokStrg.nextToken();
                String indexStrg = tokStrg.nextToken().trim();
                String nameStrg = tokStrg.nextToken().trim();

                if(nameStrg.equals("le") || nameStrg.equals("hme")) {
                   nicStrg = nicStrg + " " + nameStrg + indexStrg;
                }
             }
          }

          if(nicStrg.trim().length() > 0) {
             nicTokStrg = new StringTokenizer(nicStrg);
          }
          fr.close();

       } catch(FileNotFoundException f) {
            //ignore
            //setLabel(targetFile + "  not found.");
         }
         catch(IOException g) {
            //setLabel("Error reading  " + targetFile );
         }

       return nicTokStrg;
    }


    public boolean getItem( String item, String targetFile ) {
       try
       {
          FileReader fr = new FileReader(targetFile);
          BufferedReader br = new BufferedReader(fr);
          String line;

          while((line = br.readLine()) != null) {

             StringTokenizer stkst = new StringTokenizer(line, ":");
             String firstTk = stkst.nextToken().trim();
             if(firstTk.equals(item)) {
                fr.close();
                return true;
             }
          }
          fr.close();

       } catch(FileNotFoundException f) {
            //ignore
            //setLabel(targetFile + "  not found.");
         }
         catch(IOException g) {
            //setLabel("Error reading  " + targetFile );
         }
       return false;
    }


    public void setSumLabel(String newText) {
        sumLabel.setText(newText);
    }


    private void showUserPanel(boolean bShow) {
        if (bShow) {
	    panel3.remove(defaultPane);
	    panel3.add(txFldPane);
            nmrLink = "yes";
            setupDB = true;
            loadVNMR = true;
        }
        else {
	    panel3.remove(txFldPane);
	    panel3.add(defaultPane);
            nmrLink = "no";
            setupDB = false;
            loadVNMR = false;
        }
        Dimension d = txFldPane.getSize();
        defaultPane.setMinimumSize(d);
        btmPane.validate();
    }


    private void showPassword(JCheckBox check, boolean bShow) {
        if (check == null || !check.isShowing())
            return;
        int checkId = 0;
        int dataSize = 0;
        StringTokenizer stkst = new StringTokenizer(check.getText());
        String cbName = check.getName();

        if (stkst.hasMoreTokens()) {
             stkst.nextToken();
             stkst.nextToken();
        } 
        try {
            checkId = Integer.parseInt(cbName);
            if (stkst.hasMoreTokens())
                 dataSize = Integer.parseInt(stkst.nextToken().trim());
        }
        catch (NumberFormatException e) {
            return;
        }
        if (curTabIndex >= numTab || checkId >= passwordField[curTabIndex].length)
            return;
        JPasswordField password = passwordField[curTabIndex][checkId];
        JLabel  passLabel = passwordLabel[curTabIndex][checkId];
        if (password == null || passLabel == null)
            return;

        password.setVisible(bShow);
        passLabel.setVisible(bShow);
        if (bShow) {
            grandSum[curTabIndex] += dataSize;
            vecOpt[curTabIndex].addElement(cbName);
            password.enableInputMethods(true);
            password.requestFocus();
        }
        else {
            grandSum[curTabIndex] -= dataSize;
            vecOpt[curTabIndex].removeElement(cbName);
        }
        setSumLabel(" " + grandSum[curTabIndex] + "  KB");
    }

    //for all checkboxes
    class CheckBoxListener implements ItemListener {
        public void itemStateChanged(ItemEvent e) {

           int checkId = 0;
           int dataSize = 0;
           final JCheckBox chkb = (JCheckBox) e.getSource();
           StringTokenizer stkst = new StringTokenizer(chkb.getText());

           String cbName = chkb.getName();

           String tkName  = stkst.nextToken().trim(); //Only "Gen" and "Opt" use this value
           String tkAt    = stkst.nextToken().trim(); //skip this token

           if (e.getStateChange() == ItemEvent.SELECTED) {

                if (cbName.equals("setacq")) {
                    selectedNic = (String) nicComboBox[curTabIndex].getSelectedItem();
                    setAcq = true;
                    //rebootConsole = true;
                    //nicComboBox[curTabIndex].setVisible(true);
                } else if (cbName.equals("nmrdb")) {
                    //nmrDb = "yes";
                    setupDB = true;
                } else if (cbName.equals("nmrlink")) {
                    nmrLink = "yes";
                    if (setAcq)
                       selectedNic = (String) nicComboBox[curTabIndex].getSelectedItem();

                    nmrdbChkBox.setEnabled(true);
                    if (nmrdbChkBox.isSelected())
                       //nmrDb = "yes";
                       setupDB = true;
                    else
                       setupDB = false;
                       //nmrDb = "no";

                    if (setacqChkBox[curTabIndex] != null)
                       setacqChkBox[curTabIndex].setEnabled(true);

/*                    if (nicComboBox[curTabIndex] != null)
/*                       nicComboBox[curTabIndex].setEnabled(true);
/* */
                } else if (cbName.equals("Gen")) {
                    grandSum[curTabIndex] += Integer.parseInt( stkst.nextToken().trim());
                    vecGen[curTabIndex].addElement(tkName);
                } else {
                    SwingUtilities.invokeLater(new Runnable() {
                         public void run() {
                             showPassword(chkb, true);
                         }
                    });

                   /**************************
                    try {
                        checkId = Integer.parseInt(cbName);
                        if (stkst.hasMoreTokens())
                           dataSize = Integer.parseInt(stkst.nextToken().trim());
                    }
                    catch (NumberFormatException nfe) {
                        return;
                    }
                    if (curTabIndex >= numTab ||
                               checkId >= passwordField[curTabIndex].length)
                        return;
                 
                    // grandSum[curTabIndex] += Integer.parseInt( stkst.nextToken().trim());
                    grandSum[curTabIndex] += dataSize;
                    vecOpt[curTabIndex].addElement(cbName);
                    //vecOpt[curTabIndex].addElement(tkName);

                    JPasswordField password = passwordField[curTabIndex][checkId];
                    if (password != null) {
                        password.setVisible(true);
                        passwordLabel[curTabIndex][checkId].setVisible(true);
                        password.enableInputMethods(true);
                        password.requestFocus();
                    }
                    // if (comTextField[0] != null)
                    //     comTextField[0].requestFocus();

                   **************************/
                }
                if (tkName.equals("VNMR")) {
                 /***
                   SwingUtilities.invokeLater(new Runnable() {
                         public void run() {
                             showUserPanel(true);
                         }
                   });
                 ***/
		    panel3.remove(defaultPane);
	  	    panel3.add(txFldPane);
                    nmrLink = "yes";
                    setupDB = true;
                    loadVNMR = true;
                  //  nmrlkChkBox.setEnabled(true);
                  //  nmrdbChkBox.setEnabled(true);
                }

            } else {

                if (cbName.equals("setacq")) {
                    selectedNic = null;
                    setAcq = false;
                    //rebootConsole = false;
                    //nicComboBox[curTabIndex].setVisible(false);
                } else if (cbName.equals("nmrdb")) {
                     setupDB = false;
                    //nmrDb = "no";
                } else if (cbName.equals("nmrlink")) {
                    nmrLink = "no";
                    selectedNic = null;

                    nmrdbChkBox.setEnabled(false);
                    setupDB = false;
                    //nmrDb = "no";

                    if (setacqChkBox[curTabIndex] != null)
                       setacqChkBox[curTabIndex].setEnabled(false);

                    if (nicComboBox[curTabIndex] != null)
                       nicComboBox[curTabIndex].setEnabled(false);


                } else if (cbName.equals("manlink")) {
                    manLink = "no";
                } else if (cbName.equals("Gen")) {
                    grandSum[curTabIndex] -= Integer.parseInt( stkst.nextToken().trim());
                    vecGen[curTabIndex].removeElement(tkName);
                } else {
                    SwingUtilities.invokeLater(new Runnable() {
                         public void run() {
                             showPassword(chkb, false);
                         }
                    });
                    /*************************
                    try {
                        checkId = Integer.parseInt(cbName);
                        if (stkst.hasMoreTokens())
                           dataSize = Integer.parseInt(stkst.nextToken().trim());
                    }
                    catch (NumberFormatException nfe) {
                        return;
                    }
                    if (curTabIndex >= numTab ||
                               checkId >= passwordField[curTabIndex].length)
                        return;
                    // grandSum[curTabIndex] -= Integer.parseInt( stkst.nextToken().trim());
                    grandSum[curTabIndex] -= dataSize;
                    vecOpt[curTabIndex].removeElement(cbName);
                    //vecOpt[curTabIndex].removeElement(tkName);
                    passwordField[curTabIndex][checkId].setVisible(false);
                    passwordLabel[curTabIndex][checkId].setVisible(false);

                    ***********************/
                }
                if (tkName.equals("VNMR")) {
                   /***
                   SwingUtilities.invokeLater(new Runnable() {
                         public void run() {
                             showUserPanel(false);
                         }
                   });
                   ***/

		    panel3.remove(txFldPane);
		    panel3.add(defaultPane);
                    nmrLink = "no";
                    setupDB = false;
                    loadVNMR=false;
                   // nmrlkChkBox.setEnabled(false);
                   //  nmrdbChkBox.setEnabled(false);
                }

            }
            setSumLabel(" " + grandSum[curTabIndex] + "  KB");
            Dimension d = txFldPane.getSize();
            defaultPane.setMinimumSize(d);
            btmPane.validate();
        }
    }

    public void  getTOC( String targetFile, Vector keyVec, Vector valVec ) {
       try {

          Vector tmpKeyVec = new Vector();
          Vector tmpValVec = new Vector();
          Vector kV = null;
          Vector vV = null;

          FileReader fr = new FileReader(targetFile);
	  if (!fr.ready())
		return;
          BufferedReader br = new BufferedReader(fr);

          String line;

          while((line = br.readLine()) != null) {

             StringTokenizer stkst = new StringTokenizer(line);
             String keyStrg = stkst.nextToken().trim();

             String  valStrg = stkst.nextToken().trim() ;
             int     intval = Integer.parseInt( valStrg );

             if( keyStrg.equals("VNMR") ) {
                 kV = keyVec;
                 vV = valVec;
                 //System.out.println("TOC " + keyStrg + " ---");
             } else {
                 kV = tmpKeyVec;
                 vV = tmpValVec;
               }

             int index = kV.indexOf(keyStrg);

             if ( index != -1 ) {  //key exists

                intval += Integer.parseInt( (String) vV.get(index) );
                vV.set(index, new Integer(intval).toString());

             } else {
                  kV.addElement(keyStrg);
                  vV.addElement(valStrg);
               }
          }
          fr.close();

          for(int i = 0; i < tmpKeyVec.size(); i++) {

             keyVec.addElement(tmpKeyVec.get(i));
             valVec.addElement(tmpValVec.get(i));
          }

          //for(int i = 0; i < keyVec.size(); i++) {
          //    System.out.println("index " + i + " = " + keyVec.get(i)+ " " + valVec.get(i));
          //}

       } catch(FileNotFoundException f) {
            //ignore
            //setLabel(targetFile + "  not found.");
         }
         catch(IOException g) {
            //setLabel("Error reading  " + targetFile );
         }
       return ;
    }

    public static void main(String[] args) {

       baseDir   = args[0];
       destDir   = args[1];
       nmrUser   = args[2];
       userDir   = args[3];
       nmrGroup  = args[4];
       osVersion = args[5];
       //leCard    = args[6];
       //hmeCard   = args[7];

       showSol=false;
       if (args.length > 6) {
          if (args[6].compareTo("Sun") == 0)
              showSol = true;
       }
             
       codeDir   = baseDir + "/code";

       LoadNmr nmrFrame = new LoadNmr();
       nmrFrame.setVisible( true );

       nmrFrame.addWindowListener(new WindowAdapter() {
           public void windowClosing(WindowEvent e) {
               System.exit(0);
           }
       });
    }

    private void setUIAttrs() {
        UIManager.put("TabbedPane.tabsOpaque", true);

        // Make background of selected tab match the panel background
        UIManager.put("TabbedPane.selected", getContentPane().getBackground());

        // Eliminate blue line across bottom of tabs
        UIManager.put("TabbedPane.contentBorderInsets", new Insets(0, 2, 2, 2));

        // Clean up top and right side of tabs
        UIManager.put("TabbedPane.darkShadow", Color.lightGray);
        //UIManager.put("TabbedPane.shadow", Color.darkGray);
        //UIManager.put("TabbedPane.highlight", Color.white);

        // Leave margin above and left of tabs
        UIManager.put("TabbedPane.tabAreaInsets", new Insets(5, 10, 0, 0));

        // Leave margin left and right of tab icons
        UIManager.put("TabbedPane.tabInsets", new Insets(0, 10, 0, 10));

        // Avoid extra space to right of tab icons
        UIManager.put("TabbedPane.textIconGap", 0);
    }

    public class optLayout implements LayoutManager {

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
            return new Dimension(w+10, h);
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
                int   y = 4;
                int   k;
                Component m, m1;
		Container p;

                for ( k = 0; k < n; k++) {
                    m = target.getComponent(k);
                    if(m instanceof Container) {
			p = (Container) m;
                        int n2 = p.getComponentCount();
                        if (n2 > 2) {
                            m1 = p.getComponent(0);
                            dim = m1.getPreferredSize();
                            if (dim.width > w)
                                w = dim.width;
                            m1 = p.getComponent(1);
                            dim = m1.getPreferredSize();
                            if (dim.width > w2)
                                w2 = dim.width;
                        }
                    }
                }

		w += 10;
                H3Layout.setHGap(0);
                H3Layout.setFirstX(w);
                H3Layout.setSecondX(w+w2+10);

                Dimension dim0 = target.getSize();
                for (k = 0; k < n; k++) {
                    Component obj = target.getComponent(k);
                    if (obj.isVisible()) {
                        dim = obj.getPreferredSize();
                        obj.setBounds(0, y, dim0.width-4, dim.height);
                        y += dim.height;
                    }
                }
            }
        } // layoutContainer
    } // optLayout

    public class genPanel extends JPanel {
	int id;

       public genPanel(int  id) {
	  this.id = id;
	  Vector genKeyVec = new Vector();
          Vector genValVec = new Vector();

	  String srcDir = codeDir + "/" + osVersion +"/";

	  getTOC( srcDir + consoleType[id] + "." + osVersion , genKeyVec, genValVec );

	  int genChkBox = genKeyVec.size();
          JCheckBox[] chkBoxGen = new JCheckBox[genChkBox];
	  setBorder(BorderFactory.createEmptyBorder(10,10,10,10));
	  setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));

	  for(int k = 0; k < genKeyVec.size(); k++) {

          	String str = (String) genKeyVec.get(k);

          	chkBoxGen[k] = new JCheckBox(str + "      at  " + genValVec.get(k) + " KB");
          	chkBoxGen[k].setActionCommand("CB_command" + k);
          	chkBoxGen[k].setName("Gen");
          	chkBoxGen[k].addItemListener(chkboxListener);
          	if ((str.compareTo("limNet") != 0) &&
                    (str.compareTo("ProTune") != 0) &&
                    (str.compareTo("Chinese") != 0) &&
                    (str.compareTo("Japanese") != 0) &&
                    (str.compareTo("Secure_Environments") != 0) &&
                    (str.compareTo("VAST") != 0) &&
                    (str.compareTo("LC-NMR") != 0) &&
          	    (str.compareTo("768AS") != 0)) chkBoxGen[k].doClick();
          	add(chkBoxGen[k]);
          	// chkBoxGen[k].addItemListener(chkboxListener);
	   }
       }
    } // genPanel

    public class optPanel extends JPanel {
 	int id;
	JScrollPane sc;

       public optPanel(int  id) {
	  this.id = id;
          setLayout(new BorderLayout());
	  setBorder(BorderFactory.createEmptyBorder(0,10,10,10));
	  JPanel jp = new JPanel();
	  JPanel jp2;
	  sc = new JScrollPane(jp,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
	  sc.setBorder(BorderFactory.createEmptyBorder(0,0,0,0));
	  Vector optKeyVec = new Vector();
	  Vector optValVec = new Vector();

          String srcDir = codeDir + "/" + osVersion +"/";

	  getTOC( srcDir + consoleType[id] + ".opt", optKeyVec, optValVec );
	  int optChkBox = optKeyVec.size();
	  chkBoxOpt[id] = new JCheckBox[optChkBox];
          passwordLabel[id] = new JLabel[optChkBox];
          passwordField[id] = new JPasswordField[optChkBox];
	  jp.setLayout(new optLayout());
	  for(int k = 0; k < optKeyVec.size(); k++) {
	      jp2 = new JPanel();
	      jp2.setLayout(new H3Layout());
              jp.add(jp2);
              String str = (String) optKeyVec.get(k);
              chkBoxOpt[id][k] = new JCheckBox(str + "      at  " + optValVec.get(k) + " KB");
              chkBoxOpt[id][k].setActionCommand(str);
	      chkBoxOpt[id][k].setName(Integer.toString(k));

              passwordLabel[id][k] = new JLabel(" Password: ");
              passwordLabel[id][k].setVisible(false);

              passwordField[id][k] = new JPasswordField(12);
              passwordField[id][k].setEchoChar('*');
              passwordField[id][k].setVisible(false);
              passwordField[id][k].enableInputMethods(true);

              jp2.add(chkBoxOpt[id][k]);
              jp2.add(passwordLabel[id][k]);
              jp2.add(passwordField[id][k]);
	      chkBoxOpt[id][k].addItemListener(chkboxListener);
          }

	  jp.setLocation(0, 0);
          add(sc, BorderLayout.CENTER);
	}

	public JComponent getScrollPan() {
	    return sc;
	}

    } // optPanel

    public class tabPanLayout implements LayoutManager {

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
            h += 10;
            return new Dimension(w, h);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

	public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim;
                int   n = target.getComponentCount();
                int   genH = 0;
                int   optH = 0;
                int   w = 0;
                int   h = 0;
                int   h2 = 0;
                int   y = 4;
                int   k;
                Component m, m1;
		Container p;
                Dimension dim0 = target.getSize();

                for ( k = 0; k < n; k++) {
                    m = target.getComponent(k);
                    dim = m.getPreferredSize();
                    if (m instanceof genPanel)
			genH = dim.height;
                    else if (m instanceof optPanel)
			optH = dim.height+50;
		    else
		        h = h + dim.height;
		    if (dim.width > w)
		        w = dim.width;
                }
		h2 = h + genH + optH;
		while (h2 > dim0.height && optH > 30) {
			optH = optH - 10;
			h2 = h + genH + optH;
		}

		y = 0;
                for (k = 0; k < n; k++) {
                    m = target.getComponent(k);
                    dim = m.getPreferredSize();
                    if (m instanceof genPanel) {
			m.setSize( dim0.width, genH);
			m.setLocation(0, 0);
			y = genH;
		    }
                    else if (m instanceof optPanel) {
			m.setBounds(new Rectangle(0, genH+6, dim0.width, optH));
			y = genH + optH + 6;
                    }
                    else {
			m.setSize( dim0.width, dim.height);
			m.setLocation(0, y);
			y = y + dim.height;
                    }
                }
            }
        } // layoutContainer
    } // tabPanLayout
}
