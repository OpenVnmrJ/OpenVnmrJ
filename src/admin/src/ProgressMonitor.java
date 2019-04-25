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

//ProgressMonitor.java

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.io.*;
import java.util.*;
import java.net.*;
import java.text.*;

public class ProgressMonitor extends JFrame {

    private  JProgressBar   progressBar;
    private  JButton        cancelButton;
    private  int            current = 0;
    private  JTextArea      textArea;
    private  String         newline = "\n";
    private  String         shToolCmd = System.getProperty("shtoolcmd");
    protected String        shToolOption = System.getProperty("shtooloption");
    private  RealWorks      workThread;

    private  int            lengthOfTask;
    private  String         osVersion;
    private  String         nmrLink;
    private  String         manLink;
    private  String         consoleType;
    private  String         genList;
    private  String         optList;
    private  boolean        setupDB;
    //private  String         nmrDb;
    private  String         nmrUser;
    private  String         nmrGroup;
    private  String         codeDir;
    private  String         destDir;
    private  String         userDir;
    private  String         selectedNic;
    private  int            nicCnt;
    private  boolean        userExist = true;

    //private  boolean        doSetacq = false;

    public ProgressMonitor( int sum,       String osver,    String nmrlnk,
			    String manlnk, String contype,  String genlist,
			    String optlist,boolean setupdb, String user,
			    String group,  String codedir,  String destdir,
			    String userdir,String selectednic, int niccnt,
			    boolean userexist, String consoleName ) {

        //extending progress bar to do database and other after software loading
        lengthOfTask = sum+(sum/100*5);
        osVersion = osver;
        nmrLink  = nmrlnk;
        manLink  = manlnk;
        consoleType = contype;
        genList = genlist;
        optList = optlist;
        setupDB   = setupdb;
        nmrUser  = user;
        nmrGroup = group;
        codeDir = codedir;
        destDir = destdir;
        userDir = userdir;
        selectedNic = selectednic;
        nicCnt  = niccnt;
        userExist = userexist;
	
	if (shToolCmd == null)
	    shToolCmd = "/bin/bash";
	if (shToolOption == null)
	    shToolOption = "-c";

        setTitle( "Installing "+consoleName+" Software Package" );
        /*setSize( 580 , 300 );
	  setLocation( 210, 80 );*/
        Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
        double width = dim.width/2;
        double height = dim.height/2;
        double x = width/2;
        double y = height/2;
        setSize( (int)width , (int)height );
        setLocation( (int)x, (int)y );
	
	if (osVersion != null && osVersion.equals("win"))
        {
            codeDir = InstallVJ.windowsPathToUnix(codeDir);
            destDir = InstallVJ.windowsPathToUnix(destDir);
            userDir = InstallVJ.windowsPathToUnix(userDir);
        }

        cancelButton = new JButton("Cancel");
        cancelButton.setActionCommand("cancel");
        cancelButton.addActionListener(new ButtonListener());

        progressBar = new JProgressBar(0, lengthOfTask);
        progressBar.setValue(0);
        progressBar.setStringPainted(true);

        textArea = new JTextArea(3, 20);
        textArea.setMargin(new Insets(5,5,5,5));
        textArea.setEditable(false);

        //Box box = new Box(BoxLayout.Y_AXIS);
        Box box = Box.createVerticalBox();
        box.setBackground(Color.cyan);
        box.add(Box.createVerticalStrut(10));
        box.add(progressBar);
        box.add(Box.createVerticalStrut(10));
        box.add(cancelButton);
        //box.add(cancelButton,BorderLayout.CENTER);

        JPanel contentPane = new JPanel();
        contentPane.setLayout(new BorderLayout());
        contentPane.add(new JScrollPane(textArea), BorderLayout.CENTER);
        contentPane.add(box, BorderLayout.SOUTH);
        //contentPane.add(panel, BorderLayout.SOUTH);
        contentPane.setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));
        setContentPane(contentPane);
    }

    //start the task.
    public void go() {
        current = 0;
	workThread = new RealWorks();
    }

    public String getExitMesg()
        {
            String strMsg = "";
            if (cancelButton.getActionCommand() == "Done") {
                strMsg = ">>>>> End of VNMRJ Installation Program <<<<<";
            }
            else
                strMsg = ">>>>> VNMRJ Installation Cancelled <<<<<";
            return strMsg;
        }

    //The actual long running task.  This task runs on its own thread.
    public class RealWorks implements Runnable
    {
        protected Thread myThread;
	private boolean did_vnmr;
        private String oldVnmrPath="";

        public RealWorks() {

            myThread = new Thread(this);
            myThread.start();
        }

        public void run()
            {
                // To reuse the database from the previous install (if there
                // was one), we need to remove all of the DB items from the DB
                // which are in the previous /vnmr area.  To do that, we need
                // to get the canonical path of /vnmr before we remove it and
                // create another one.
                boolean bwindows = osVersion.equals("win");
                try {
                    String path = "/vnmr";
                    if (bwindows)
                        path = InstallVJ.unixPathToWindows(path);
                    File file = new File(path);
                    if (file.exists())
                        oldVnmrPath = file.getCanonicalPath();
                }
                // If error, just keep going with empty oldVnmrPath
                catch (Exception e) {}

                try
                {
                    String execStr = codeDir + "/" + "ins_vnmr"  + " " + osVersion
                        + " " + consoleType
                        + " " + codeDir
                        + " " + destDir
                        + " " + nmrUser
                        + " " + nmrGroup
                        + " " + userDir
                        + " " + nmrLink
                        + " " + manLink
                        + " " + genList
                        + " " + optList;
                    String[] cmd = {shToolCmd, shToolOption, execStr};

                    Runtime rt = Runtime.getRuntime();
                    Process prcs = rt.exec(cmd);

                    shellIO shr = new shellIO(prcs);

                    //InputStream istrm = prcs.getInputStream();
                    //BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

                    // String strg = bfr.readLine();
                    String strg = shr.readLine();
                    while(strg != null)
                    {
                        did_vnmr = (genList.indexOf("VNMR") != -1 ||
				                           genList.indexOf("vnmr") != -1);

                        if (strg.trim().equals("Software Installation Completed.")) {
			                  // if windows, run makeuser even if user exists 
                           if (bwindows)
                                userExist = false; 

                            if( ! userExist  && did_vnmr) {
                                makeUserVnmrj( nmrUser );
                            }

                            if( setupDB && did_vnmr) {
                                setupDatabase( codeDir, oldVnmrPath );
                            }

                            // disable the stuff below for now,
                            //  until we straighten out jsetacq
                            selectedNic = null;
                            if( selectedNic != null ) {

                                JOptionPane.showMessageDialog(getContentPane(),
                                                              "You selected the Do setacq option.\n Please, reboot the console now.");

                                textArea.append("\n\nRunning -- setacq --");
                                doSetAcq( selectedNic, nicCnt );
                            }


                            // perform any post installation action via the vjpostinstallaction script
                            String VnmrInstalled;
                            if (did_vnmr)
                               VnmrInstalled = "y";
                            else
                               VnmrInstalled = "n";

                            String argStr = " " + osVersion
                                    + " " + consoleType
                                    + " " + codeDir
                                    + " " + destDir
                                    + " " + VnmrInstalled
                                    + " " + nmrUser
                                    + " " + nmrGroup
                                    + " " + userDir
                                    + " " + nmrLink
                                    + " " + manLink
                                    + " " + genList
                                    + " " + optList;

                            // textArea.append("\n\nrunPostInstall - ins_vnmr " + newline);
                            // textArea.append("argStr: " + argStr + newline);
                            runPostInstallScript(nmrUser, destDir, argStr);


                            progressBar.setValue(lengthOfTask);
                            Toolkit.getDefaultToolkit().beep();
                            cancelButton.setText("Done");
                            cancelButton.setActionCommand("Done");
                            cancelButton.setEnabled(true);
                            if (bwindows)
                                cancelButton.doClick();
                            return;
                        }

                        textArea.append(strg + newline);
                        textArea.setCaretPosition(textArea.getDocument().getLength());

                        String valStr = null;
                        StringTokenizer tkTest = new StringTokenizer(strg);
                        while( tkTest.hasMoreTokens() )
                        {
                            String firstTk  = tkTest.nextToken().trim();

                            if( firstTk.equals("DONE:") | firstTk.equals("SKIPPED:") ) {
                                valStr = tkTest.nextToken().trim();

                                int    rtVal   = Integer.parseInt( valStr );

                                current += rtVal;    //make some progress
                                if (current > lengthOfTask) {
                                    current = lengthOfTask;
                                }

                                progressBar.setValue(current);

                                //if (current >= lengthOfTask) {
                                //   textArea.append("\nALL REQUESTED SOFTWARE EXTRACTED\n");
                                //}
                            }

                        }

                        // strg = bfr.readLine();
                        strg = shr.readLine();
                    }

//
// NOTE:
//  it would appear to me (GMB) that any code below this point never get exxecuted...   12/08/2009
//
                    // At the end, if the installation did not get completed
                    // e.g. if only installing an option that requires a  password field,
                    // and the password is incorrect.
                    if( ! userExist && did_vnmr)
                    {
                        makeUserVnmrj( nmrUser );
                    }

                    if( setupDB && did_vnmr)
                    {
                        setupDatabase( codeDir, oldVnmrPath);
                    }

                    String VnmrInstalled;
                    if (did_vnmr)
                       VnmrInstalled = "y";
                    else
                       VnmrInstalled = "n";

                    String argStr = " " + osVersion
                            + " " + consoleType
                            + " " + codeDir
                            + " " + destDir
                            + " " + VnmrInstalled
                            + " " + nmrUser
                            + " " + nmrGroup
                            + " " + userDir
                            + " " + nmrLink
                            + " " + manLink
                            + " " + genList
                            + " " + optList;

                    // textArea.append("\n\nrunPostInstall - mode 2 " + newline);
                    // textArea.append("argStr: " + argStr + newline);
                    runPostInstallScript(nmrUser, destDir, argStr);
                    progressBar.setValue(lengthOfTask);
                    textArea.append("Software Installation Completed." + newline);
                    Toolkit.getDefaultToolkit().beep();
                    cancelButton.setText("Done");
                    cancelButton.setActionCommand("Done");
                    cancelButton.setEnabled(true);
                }

                catch (IOException e)
                {
                    System.out.println(e);
                }
            }

        void doSetAcq( String name, int num )
            {
                try
                {
                    String[] cmd = {shToolCmd, shToolOption, destDir + "/bin/jsetacq" + " " + name + " " + num};
                    Runtime rt = Runtime.getRuntime();
                    Process prcs = rt.exec(cmd);

                    shellIO shr = new shellIO(prcs);

                    //InputStream istrm = prcs.getInputStream();
                    //BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

                    //String strg = bfr.readLine();
                    String strg = shr.readLine();
                    while(strg != null)
                    {
                        if (strg.trim().equals("NMR Console setting up complete")) {

                            textArea.append(strg + newline + newline);
                            return;
                        }

                        textArea.append(strg + newline);
                        textArea.setCaretPosition(textArea.getDocument().getLength());

                        // strg = bfr.readLine();
                        strg = shr.readLine();
                    }
                }
                catch (IOException e)
                {
                    System.out.println(e);
                }
            }

        void makeUserVnmrj( String name )
            {
                try
                {
                    String[] cmd = {shToolCmd, shToolOption, codeDir + "/mkvnmrjadmin" + " " + destDir + " " + name };
                    Runtime rt = Runtime.getRuntime();
                    Process prcs = rt.exec(cmd);

                    InputStream istrm = prcs.getInputStream();
                    BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));

                    String strg = bfr.readLine();
                    while(strg != null)
                    {
                        if (strg.trim().equals("Automatic configuration of user account '"+name+"' done.")) {

                            textArea.append(strg + newline + newline);
                            return;
                        }
                        textArea.append(strg + newline);
                        textArea.setCaretPosition(textArea.getDocument().getLength());

                        strg = bfr.readLine();
                    }
                }
                catch (IOException e)
                {
                    System.out.println(e);
                }
            }



        void setupDatabase( String codedir, String oldVnmrPath )
            {

                String strg=null;
                String estrg=null;

                int exitValue = 0;

                try
                {
                    // Run dbsetup in a mode to preserve previous database
//                    String[] cmd = {shToolCmd, shToolOption, codedir + "/" + "dbsetup" + " " + nmrUser + " " + destDir + " preserveDB " + oldVnmrPath};
                    // Run dbsetup in a mode to remove everything and rebuild from scratch - Takes longer
                    String[] cmd = {shToolCmd, shToolOption, codedir + "/" + "dbsetup" + " " + nmrUser + " " + destDir};
                    Runtime rt = Runtime.getRuntime();
                    String user = System.getProperty("user.name");
                    // Output the command being executed for logging.
                    textArea.append(cmd[0] + " " + cmd[1] +
                                    " " + cmd[2] + newline);
                    textArea.setCaretPosition(textArea.getDocument().getLength());

                    Process prcs = rt.exec(cmd);

                    InputStream istrm = prcs.getInputStream();
                    // The InputStream is only stdin, we also need stderr
                    InputStream estrm = prcs.getErrorStream();
                    BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));
                    BufferedReader ebfr = new BufferedReader(new InputStreamReader(estrm));

                    // Since we are reading two input streams, we cannot
                    // do a normal read and have them block.   Else, we would
                    // stay blocked on one or the other.  So, check to see
                    // if anything is in the BufferedReader before trying
                    // to read it.
                    if(bfr.ready())
                        strg = bfr.readLine();
                    if(ebfr.ready())
                        estrg = ebfr.readLine();

                    for(int timeout=0; timeout < 10000; timeout++) {
                        if(strg != null) {
                            if (strg.trim().indexOf("End of Database Setup.")>=0) {
                                try {
                                    // If the process has not exited, then we cannot
                                    // look at its exitValue.  Wait here.
                                    exitValue = prcs.waitFor();
                                } catch(Exception e) {}
                                if(exitValue != 0) {
                                    textArea.append("dbsetup terminated with error!\n");
                                }
                                textArea.append(strg + newline + newline);
                                return;
                            }
                            textArea.append(strg + newline);
                            textArea.setCaretPosition(textArea.getDocument().getLength());
                        }
                        // Catch stuff going to stderr and show it to the user
                        if(estrg != null) {
                            // Get rid of DEBUG messages.
                            if(!estrg.trim().startsWith("DEBUG")) {
                                textArea.append(estrg + newline);
                                textArea.setCaretPosition(textArea.getDocument().getLength());
                            }
                        }

                        // Since we have effectively stop the blocking of
                        // the read, do something to stop this method from
                        // running at full cpu usage.
                        Thread.sleep(50);

                        // Initialize the strings to null so we will know
                        // if anything was read.
                        strg = null;
                        estrg = null;

                        // Since we are reading two input streams, we cannot
                        // do a normal read and have them block. Else, we would
                        // stay blocked on one or the other.  So, check to see
                        // if anything is in the BufferedReader before trying
                        // to read it.
                        if(bfr.ready()) {
                            strg = bfr.readLine();
                            // reset the timer when we get something.
                            timeout = 0;
                        }
                        if(ebfr.ready()) {
                            estrg = ebfr.readLine();
                            // reset the timer when we get something.
                            timeout = 0;
                        }

                    }
                    // Come here if timeout.
                    try {
                        // If the process has not exited, then we cannot
                        // look at its exitValue.  Wait here.
                        exitValue = prcs.waitFor();
                    } catch(Exception e) {}
                    if(exitValue != 0) {
                        textArea.append("dbsetup terminated with an error!\n");
                    }
                }
                catch (Exception e)
                {
                    textArea.append(e.toString());
                }
            }
     }

     void runPostInstallScript(String adminUser, String VnmrPath, String argList)
     {
          String strg=null;
          int exitValue = 0;

          try
          {
             String[] cmd = {shToolCmd, shToolOption, codeDir + "/" + "vjpostinstallaction" + " " + argList};

             // textArea.append("invoke: " +cmd[2]  + newline);
             Runtime rt = Runtime.getRuntime();
             Process prcs = rt.exec(cmd);

             shellIO shr = new shellIO(prcs);

             strg = shr.readLine();
             while(strg != null)
             {
                 if (strg.trim().equals("Post Action Completed.")) 
                 {
                     return;
                 }
                 if (strg.trim().equals("Post Action Skipped.")) 
                 {
                     return;
                 }
             
                 textArea.append(strg + newline);
                 textArea.setCaretPosition(textArea.getDocument().getLength());

                 strg = shr.readLine();
             }
          }
          catch (Exception e)
          {
            textArea.append(e.toString());
          }
     }

    /**
     *  OK, enough with missing the stderr output of the shell script...
     *  This class presents the reader with a single source that contains
     *  both stdout and stderr.
     *  This class uses a pipes to combine two sources to a single output
     *  A separate thread is run to read the stdout & stderr from the shell
     *  and writes it to a single BufferWriter.
     *  The main thread that calls shellIO readLine, reads from the BufferReader
     *  that was filled by the Thread with both stdout & stderr.
     *
     *  stdout & stderr -> BufferedWriter -> PipedWriter -> PipedReader -> BufferedReader.readLine()
     * Author  Greg Brissey
     */

    class shellIO implements Runnable
    {
        protected Thread myThread;
        Process shellProc;
        BufferedReader bfr, ebfr, shr;
        PipedWriter pw = null;
        BufferedWriter bw = null;
        PipedReader pr = null;

        public shellIO( Process prcs ) throws IOException
        {
            InputStream istrm = prcs.getInputStream(); // stdout
            InputStream estrm = prcs.getErrorStream(); // sderr
            bfr = new BufferedReader(new InputStreamReader(istrm)); // stdout
            ebfr = new BufferedReader(new InputStreamReader(estrm)); // stderr
            pw = new PipedWriter();
            pr = new PipedReader(pw);
            shr = new BufferedReader(pr);
            bw = new BufferedWriter(pw);
            myThread = new Thread(this);
            myThread.start(); // let the reading stdout & stderr begin....
        }

        // This thread reads the stdout & stderr from the shell process
        // and writes them into a single BufferedWriter
        public void run() {
          String stdout, stderr;
          try {
            //System.out.println("Starting shellIO reader thread.");
            while (true)
            {
              if(bfr.ready())
              {
                stdout = bfr.readLine();
                //System.out.println("stdout:" + stdout);
                // write, newline (need's this so that readLine() doesn't block), and flush it
                bw.write(stdout); bw.newLine(); bw.flush();
              }
              if(ebfr.ready())
              {
                stderr = ebfr.readLine();
                //System.out.println("stderr:" + stderr);
                bw.write(stderr); bw.newLine(); bw.flush();
              }
              // Since we have effectively stop the blocking of
              // the read, do something to stop this method from
              // running at full cpu usage.
              Thread.sleep(50);
            }
          }
          catch (Exception e) {
              System.err.println(e);
          }
          finally {
             try {  bw.close(); } catch (Exception e) { };
          }
        }

       // read the combined streams of stdout & stderr
       String readLine() throws IOException
       {
          return shr.readLine();
      }
   }

    class ButtonListener implements ActionListener
    {
        public void actionPerformed(ActionEvent evt)
            {
                File file;
                String strPath;
            	boolean loadedVnmrJ = (genList.indexOf("VNMR") != -1);
                boolean bwindows = osVersion.equals("win");
                try
                {
                    String[] cmd = {shToolCmd, shToolOption, codeDir + "/" + "kill_insvnmr"};
                    Runtime rt = Runtime.getRuntime(); 
                    Process prcs = rt.exec(cmd);
                    System.out.println();
                    System.out.println(getExitMesg());
                    System.out.println();

                    // Write out contents of textArea to log file
                    Date date = new Date();
                    SimpleDateFormat sdf;
                    sdf = new SimpleDateFormat("yyyyMMdd-HHmm");
                    String datestr = sdf.format(date);

		    strPath = new String(destDir+"/adm/log");
		    if (bwindows)
                        strPath = InstallVJ.unixPathToWindows(strPath);
                    file = new File(strPath);
		    if (!file.exists())
                        file.mkdirs();
		    file = new File(strPath + File.separator + "vnmrj" + datestr);

                    FileWriter writer = new FileWriter(file);
                    textArea.write(writer);
                    System.out.println("Log written to: " + file.toString());
                }
                catch (IOException e) {
                    System.out.println(e);
                }
                strPath = new String(destDir+"/pw_fault");
                file = new File(strPath);
                String tmpString;
                if ( file.exists() ) {
                  System.out.println("\n\n");
                  System.out.println("One or more passwords for the following options were incorrect ");
                  try {
                     BufferedReader br = new BufferedReader( new FileReader(strPath) );
                     tmpString = br.readLine();
                     while (tmpString != null) {
                         System.out.println(tmpString);
                         tmpString = br.readLine();
                     }
                     br.close();
                     file.delete();
                     System.out.println("     If you have the correct password, you can ");
                     System.out.println("     (re)load the option separately.");
                     System.out.println("     Run load.nmr again, and only select the option(s)");
                  }
                  catch (IOException e) {
                      System.out.println(e);
                  }
                }
                    
                if (loadedVnmrJ) {
                  System.out.println("\n\n");
 		  System.out.println("Check www.spinsights.net for the latest patches.");
 		  System.out.println("If your system is a spectrometer:");
                  System.out.println("    1. Log in as the VnmrJ adminstrator account, vnmr1.");
                  System.out.println("    2. Exit all Vnmr/VnmrJ programs.");
                  System.out.println("    3. Be sure Ethernet ports have been configured properly.");
                  System.out.println("    4. SeLinux and the Firewall must be disabled.");
                  System.out.println("    5. Run /vnmr/bin/setacq");
                  System.out.println("On all systems:");
                  System.out.println("    1. Update all users.");
                  System.out.println("       You can use vnmrj adm for this");
                  System.out.println("       See Configure -> Users-> Update users...");
		  System.out.println("       Or each user can run /vnmr/bin/makeuser");
                  System.out.println("    2. In the VnmrJ interface from the");
                  System.out.println("       Edit (non-imaging) or Tools (imaging) menu,");
                  System.out.println("       select 'System Settings...' and then click");
                  System.out.println("       'System config'");
                }
                System.exit(0);
            }
    }
}
