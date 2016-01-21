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

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;

public class InstallVJ extends JFrame
{
    
    protected static JTextArea m_txaInstallVJ;
    protected static JProgressBar m_progressBar;
    protected static JButton m_btnCancel;
    protected static JButton m_btnDone;
    protected static ProgressMonitor m_progressMonitor;
    protected static int m_nValue = 0;
    protected static int m_nSize = 0;
    protected static String m_strConsole = "inova";
    protected static String m_strOsVersion;
    protected static String m_strSourceDir;
    protected static String m_strDestDir;
    protected static String m_strUser;
    protected static String m_strGroup;
    protected static String m_strHome;
    protected static String m_strVnmrLnk;
    protected static String m_strManLnk;
    protected static String m_strGenList;
    protected static String m_strOptList;
    
    public static String SHTOOLCMD = System.getProperty("shtoolcmd");
    public static String SHTOOLOPTION = System.getProperty("shtooloption");

    
    public InstallVJ(int nComps)
    {
        m_txaInstallVJ = new JTextArea();
        m_txaInstallVJ.setWrapStyleWord(true);
        JScrollPane scrollPane = new JScrollPane(m_txaInstallVJ);
        
        JPanel pnl = new JPanel(new BorderLayout());
        m_progressBar = new JProgressBar(0, nComps+2);
        m_progressBar.setStringPainted(true);
        pnl.add(m_progressBar, BorderLayout.CENTER);
        JPanel pnl2 = new JPanel(new BorderLayout());
        m_btnDone = new JButton("Done");
        m_btnCancel = new JButton("Cancel");
        pnl2.add(m_btnDone, BorderLayout.WEST);
        pnl2.add(m_btnCancel, BorderLayout.WEST);
        pnl.add(pnl2, BorderLayout.NORTH);
        
        Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
        int width = (int)(dim.width * 0.50);
        int height = (int)(dim.height * 0.50);
        setSize(width, height);
        setLocation((int)((dim.width/2)-(width/2)), (int)((dim.height/2)-(height/2)));
        
        Container contentPane = getContentPane();
        contentPane.setLayout(new BorderLayout());
        contentPane.add(scrollPane, BorderLayout.CENTER);
        contentPane.add(pnl, BorderLayout.SOUTH);
       
    }
    
    protected static void setSize(String strList, boolean bGeneral)
    {
        if (strList == null || strList.equals(""))
            return;
        
        StringTokenizer strTok = new StringTokenizer(strList, "+\\");
        ArrayList aListComps = new ArrayList();
        while (strTok.hasMoreTokens())
        {
            String strValue = strTok.nextToken();
            if (!aListComps.contains(strValue))
                aListComps.add(strValue);
        }
        
        StringBuffer sbPath = new StringBuffer().append(m_strSourceDir).append(
                            File.separator).append("win").append(
                            File.separator).append(m_strConsole);
        if (bGeneral)
            sbPath.append(".win");
        else
            sbPath.append(".opt");
        
        try
        {
            BufferedReader reader = new BufferedReader(new FileReader(sbPath.toString()));
            String strLine;
            if (reader == null)
                return;
            
            while ((strLine = reader.readLine()) != null)
            {
                strTok = new StringTokenizer(strLine);
                String strValue = "";
                if (strTok.hasMoreTokens())
                    strValue = strTok.nextToken();
                if (aListComps.contains(strValue) && strTok.hasMoreTokens())
                {
		    int nValue = 0;
		    try
		    {
			nValue = Integer.parseInt(strTok.nextToken());
		    }
		    catch (Exception e)
                    {
                        nValue = 0;
                    }
                    m_nSize = m_nSize + nValue;
                    //m_progressBar.setMaximum(m_nSize);
                }
            }
            
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
    
    protected static boolean userExistCheck(String user) {
        String strLine = null;
        boolean bUser = false;
        String strValue = "";
        
        try {
            String strPath = windowsPathToUnix(m_strSourceDir) + "/win/bin/getuserinfo";
            Runtime rt = Runtime.getRuntime();
	    String[] cmd = {SHTOOLCMD,SHTOOLOPTION, strPath + " " +user };
            Process prcs = rt.exec(cmd);

            InputStream is = prcs.getInputStream();
            BufferedReader bfr = new BufferedReader(
                                     new InputStreamReader(is));
            
            while ((strLine = bfr.readLine()) != null)
            {
                strValue = strLine;
                if (strValue.indexOf(";") >= 0)
                {
                    bUser = true;
                    break;
                }
            }
        }
        catch (IOException e)
        {
             System.out.println(e);
        }
        
        return bUser;
    }
    
    public static void main(String[] args)
    {
        if (args.length == 0)
            return;
        
        StringBuffer sbArgs = new StringBuffer();
        // the file names might have spaces in it, therefore the arguments are 
        // seperated by commas. Put the arguments together in a stringbuffer, and 
        // then get individual arguments from it.
        for (int i = 0; i < args.length; i++)
        {
            String strArg = args[i];
            sbArgs.append(strArg).append(" ");
        }
        
        StringTokenizer strTok = new StringTokenizer(sbArgs.toString(), ",");
        String strPath = "";
        if (strTok.hasMoreTokens())
            strPath = strTok.nextToken().trim();
        
        if (strTok.hasMoreTokens())
            m_strOsVersion = strTok.nextToken().trim();
        
        if (strTok.hasMoreTokens())
            m_strConsole = strTok.nextToken().trim();
        if (m_strConsole != null)
            m_strConsole = m_strConsole.replace('\'', ' ').trim(); 
        
        if (strTok.hasMoreTokens())
            m_strSourceDir = strTok.nextToken().trim();
        if (m_strSourceDir != null)
            m_strSourceDir = m_strSourceDir.replace('\'', ' ').trim();
        
        if (strTok.hasMoreTokens())
            m_strDestDir = strTok.nextToken().trim();
        if (m_strDestDir != null)
            m_strDestDir = m_strDestDir.replace('\'', ' ').trim();
        
        if (strTok.hasMoreTokens())
            m_strUser = strTok.nextToken().trim();
        
        if (strTok.hasMoreTokens())
            m_strGroup = strTok.nextToken().trim();
        
        if (strTok.hasMoreTokens())
            m_strHome = strTok.nextToken().trim();
        
        if (strTok.hasMoreTokens())
            m_strVnmrLnk = strTok.nextToken().trim();
        
        if (strTok.hasMoreTokens())
            m_strManLnk = strTok.nextToken().trim();
        
        if (strTok.hasMoreTokens())
            m_strGenList = strTok.nextToken().trim();
        
        if (strTok.hasMoreTokens())
            m_strOptList = strTok.nextToken().trim();
        
        setSize(m_strGenList, true);
        setSize(m_strOptList, false);
        boolean bUserExist = userExistCheck(m_strUser);
               
        /*System.out.println(" cmd " + sbArgs.toString());
        System.out.println("osversion: " + m_strOsVersion + " console: " + m_strConsole +
                            " sourcedir: " + m_strSourceDir + " destdir: " + m_strDestDir +
                            " user: " + m_strUser + " group: " + m_strGroup + 
                            " home: " + m_strHome + " vnmrlnk: " + m_strVnmrLnk + 
                            " genList: " + m_strGenList + " optlist: " + m_strOptList);*/
        
        String[] cmd = {"ksh.bat", "-lc", "\"/bin/winpath2unix " + args[0] + "\""};
        strPath = windowsPathToUnix(strPath);
        
        if (strPath != null && !strPath.equals(""))
        {
            strPath = strPath.trim();
            
            boolean bSetupDb = false;
            if (m_strVnmrLnk != null && (m_strVnmrLnk.equalsIgnoreCase("yes") ||
                    m_strVnmrLnk.equalsIgnoreCase("true")))
                    bSetupDb = true;
            
            m_progressMonitor = new ProgressMonitor(m_nSize, m_strOsVersion,
                                                m_strVnmrLnk, m_strManLnk, m_strConsole, 
                                                m_strGenList, m_strOptList, bSetupDb,
                                                m_strUser, m_strGroup, m_strSourceDir, 
                                                m_strDestDir, m_strHome, null, 0,
                                                bUserExist, m_strConsole);
            
            m_progressMonitor.addWindowListener(new WindowAdapter()
	       {
                   public void windowClosing(WindowEvent e)
		   {
		        System.out.println();
			System.out.println(m_progressMonitor.getExitMesg());
			System.out.println();
                        System.exit(0);
                   }
               });

               m_progressMonitor.setVisible(true);
               m_progressMonitor.go();   //start the actual installation
               //setEnabled(false);
            
        }
    }
    
    public static String unixPathToWindows(String strPath)
    {
        if (strPath == null || strPath.length() == 0)
            return strPath;
        
        StringBuffer sbPath = new StringBuffer(strPath);
        String strunix = "/dev/fs/";
        int nIndex = sbPath.indexOf(strunix);
        int index = sbPath.indexOf("\\dev\\fs");
        if (nIndex >= 0 || index >= 0)
        {
            // delete unix string
            sbPath.delete(0, strunix.length());
            //strPath = strPath.substring(strunix.length());
            // insert ':\' e.g. C:\ or ':' e.g C:
            if (sbPath.length() == 1)
                sbPath.insert(1, ":\\");
           else if (sbPath.charAt(1) != ':') 
                //strPath = strPath.substring(0, 1) + ':' + strPath.substring(1);
                sbPath.insert(1, ':');
        }
        else
        {
            if (sbPath.charAt(0) == '/')
                sbPath.insert(0, "C:\\SFU");
        }
        // replace '/' with '\'
        while ((nIndex = sbPath.indexOf("/")) >= 0)
        {
            sbPath.replace(nIndex, nIndex+1, "\\");
        }

        return sbPath.toString();
    }
    
    public static String windowsPathToUnix(String strPath)
    {
        if (strPath == null || strPath.length() == 0)
            return strPath;
        
        StringBuffer sbPath = new StringBuffer(strPath);
        String strwindows = ":";
        int nIndex = sbPath.indexOf(strwindows);
        // delete ":"
        if (nIndex >= 0)
            sbPath.deleteCharAt(nIndex);

        // replace '\' with '/'
        while ((nIndex = sbPath.indexOf(File.separator)) >= 0)
        {
            sbPath.replace(nIndex, nIndex+1, "/");
        }
        if(sbPath.indexOf("/") != 0)
            // insert /dev/fs if it does not start with '/'
            sbPath.insert(0, "/dev/fs/");

        return sbPath.toString();
    }
    
}
