/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/**************************************************************** 
   Context-Sensitive-Help API for WebHelp Pro, WebHelp , Adobe AIR Application
   
   Copyright 2008 Adobe Systems Incorporated. All rights reserved.

     Syntax:
    public static boolean RH_ShowHelp(int hParent, String a_pszHelpFile, int uCommand, int dwData)

     hParent
          Handle to the parent window. This is currently reserved for future versions of the API.
          Can be a window handle or 0.

     a_pszHelpFile
          WebHelp:
               Path to help system start page ("http://www.myurl.com/help/help.htm" or "/help/help.htm")
               For custom windows (defined in Help project), add ">" followed by the window name ("/help/help.htm>mywin")

          WebHelp Enterprise:
               Path to RoboEngine server ("http://RoboEngine/roboapi.asp")
               If automatic merging is turned off in RoboEngine Configuration Manager, specify the project name in the URL ("http://RoboEngine/roboapi.asp?project=myproject")
               For custom windows (defined in Help project), add ">" followed by the window name ("http://RoboEngine/roboapi.asp>mywindow")

     uCommand
          Command to display help. One of the following:
                    HH_HELP_CONTEXT     ' Displays the topic associated with the Map ID sent in dwData
										if 0, then default topic is displayed.
               The following display the default topic and the Search, Index, or TOC pane.
               Note: The pane displayed in WebHelp Enterprise will always be the window's default pane.
                    HH_DISPLAY_SEARCH
                    HH_DISPLAY_INDEX
                    HH_DISPLAY_TOC

     dwData
          Map ID associated with the topic to open (if using HH_HELP_CONTEXT), otherwise 0

     Examples:
           RoboHelp_CSH.RH_ShowHelp(0, "http://www.myurl.com/help/help.htm", HH_HELP_CONTEXT, 10); 
           RoboHelp_CSH.RH_ShowHelp(0, "C:\Program Files\MyApp\help\help.htm",HH_HELP_CONTEXT, 100);   


   RH_AIR_ShowHelp
		[in] a_pszViewerPath	- Path to the installation directory of AIR Help Application.
		[in] a_pszHelpId		- [optional] id of help content to be viewed in Viewer. It is specified in .helpcfg file.
		[in] a_pszWindowName	- [optional] customized named output window
		[in] ulMapNum			- Content Sensitive Map Number. 
		[in] a_pszMapId			- [optional] String representation of ulMapNum. If specified this takes priority and ulMapNum is ignored.
		[in] a_pszTopicURL		- [optional] URL of topic to be shown. If specified this takes priority over ulMapNum and a_pszMapId.
		The return value is	true  for a successful result and false if an error occurs.
******************************************************************************************/

package  vnmr.ui;

import vnmr.util.*;

import java.net.*;
import java.util.*;
import java.io.*;
import java.lang.reflect.Method;

public class RoboHelp_CSH
{
        // FlashHelp Constants
        public static final String FH_UD_DELIM_START = "<updatestring string=\"";
        public static final String FH_UD_DELIM_END = "\"/>";
        public static final int MAX_QS_COUNT = 20;
        public static final String FH_UPDATE_FILE = "wf_update.xml";
        public static final String FH_QUICK_START = "wf_startqs.htm"; 
        
        public static final String FH_WND_X = "x=";
        public static final String FH_WND_Y = "y=";
        public static final String FH_WND_WIDTH = "width=";
        public static final String FH_WND_HEIGHT = "height=";
        public static final String FH_WND_VIEW = "viewtype=";
        public static final String FH_WND_WORK = "workspace=";
        public static final String FH_WND_CAPTION = "caption=";
        public static final String FH_WND_NAME = "wndname=";
        public static final String FH_WND_DEFAULT = "wndusedef=";
        public static final String FH_WND_TOOLBAR = "toolbar=";
        public static final String FH_WND_LOCATIONBAR = "locationbar=";
        public static final String FH_WND_MENU = "menu=";
        public static final String FH_WND_RESIZE = "resizeable=";
        public static final String FH_WND_STATUSBAR = "statusbar=";
        public static final String FH_BROWSESEQ = "BrowseSequence=";
        public static final String FH_SEARCHINPUT = "SearchInput=";
        public static final String FH_DEFAULTAGT = "defaultagent=";
        public static final String FH_AGT = "agent=";
        public static final String FH_CTXTOPIC = "ctxTopic=";
        public static final String FH_UPDATE = "update=";
        public static final String FH_SINGLE_PANE = "singlepane";
        public static final String FH_DOUBLE_PANE = "doublepane";
        public static final String FH_POT = "paneopt";
        public static final String FH_WOT = "windowopt";
        
	public static final int FHWO_LOCATION = 1;
	public static final int FHWO_MENUBAR = 2;
	public static final int FHWO_RESIZABLE = 4;
	public static final int FHWO_TOOLBAR = 8;
	public static final int FHWO_STATUS = 16;
        
	public static final int PANE_OPT_SEARCH = 1;
	public static final int PANE_OPT_BROWSESEQ = 2;

        // General Constants
	public static final int	HH_DISPLAY_TOPIC = 0;
	public static final int HH_DISPLAY_TOC = 1;
	public static final int HH_DISPLAY_INDEX = 2;
	public static final int HH_DISPLAY_SEARCH = 3;
	public static final int HH_HELP_CONTEXT = 15;        
        
        // FlashHelp Globals
        public static String[] gsHelpPath = new String[MAX_QS_COUNT];
        public static String[] gsQSPath = new String[MAX_QS_COUNT];
        public static int[] gnUpdate = new int[MAX_QS_COUNT];
        public static int gnQSCount = 0;

        public static boolean RH_AssociateQuickStart(String strPrimaryHelpSource, String strQuickStartSource, int nUpdate) 
        {
            boolean bResult = false;
            if (gnQSCount < MAX_QS_COUNT)
            {
                gsHelpPath[gnQSCount] = strPrimaryHelpSource;
                gsQSPath[gnQSCount] = strQuickStartSource;
                gnUpdate[gnQSCount] = nUpdate;
                gnQSCount++;
                bResult = true;
            }

            return bResult;
        }
						
	public static boolean RH_ShowHelp(int hParent/* reserved */, String a_pszHelpFile, int uCommand, int dwData)
	{
		String strHelpPath = a_pszHelpFile;
		String strWnd = "";
		int nPos = a_pszHelpFile.indexOf(">");
		if (nPos != -1)
		{
			strHelpPath = a_pszHelpFile.substring(0, nPos);
			strWnd = a_pszHelpFile.substring(nPos+1); 
		}
		
		if (isServerBased(a_pszHelpFile))
		{
                        int nQSIndex = -1;
                        for (int i = 0; i < gnQSCount; i++)
                        {
                            if (strHelpPath == gsHelpPath[i])
                            {
                                nQSIndex = i;
                                i = gnQSCount;
                            }
                        }
                        
                        if (nQSIndex >=0)
                        {
                            boolean bResult = false;
                            bResult = ShowFlashHelp_QS(strHelpPath, gsQSPath[nQSIndex], strWnd, uCommand, dwData, gnUpdate[nQSIndex]);
                            if (!bResult)
                            {
                                bResult = ShowWebHelpServer_CSH(strHelpPath, strWnd, uCommand, dwData);
                            }
                            return bResult;
                        }
                        else
                        {
                            return ShowWebHelpServer_CSH(strHelpPath, strWnd, uCommand, dwData);
                        }
		}
		else 
			return ShowWebHelp_CSH(strHelpPath, strWnd, uCommand ,dwData);
	}
        
        private static String FH_GetUpdateString(String strQuick)
        {
            String strResult = "";
            String strUpdate = "";
            String strLnBuffer = "";
            
            strUpdate = strQuick + FH_UPDATE_FILE;
 
            try{
                String strTemp = "";
                boolean bFound = false;
                FileReader fr = new FileReader(strUpdate);
                BufferedReader br = new BufferedReader(fr);
                
                while ((strLnBuffer = br.readLine()) != null && !bFound)
                {
                    int nPos = 0;
                    boolean bFirst = false;
                    
                    if ((nPos = strLnBuffer.indexOf(FH_UD_DELIM_START)) != -1)
                    {
                        strLnBuffer = strLnBuffer.substring(nPos + FH_UD_DELIM_START.length());
                        bFirst = true;
                    }
                    
                    if (bFirst)
                    {
                        strTemp += strLnBuffer;
                        if ((nPos = strTemp.indexOf(FH_UD_DELIM_END)) != -1)
                        {
                            strResult = strTemp.substring(0,nPos);
                            bFound = true;
                        }
                    }
                }
            }
            catch (IOException e){
                strResult = "";
            }
            
            return strResult;
        }
        
        private static String FH_PostUpdateData(String strHelpFile, String strPostData)
        {
            String strResult = "";
            
            URL url;
            URLConnection urlConnect;
            DataOutputStream dataOut;
            DataInputStream dataIn;
            
            try{
                String strHostName = "";
                int nPos = 0;
                nPos = strHelpFile.indexOf("//");
                if (nPos>=0)
                {
                    nPos+=2;
                    strHostName = strHelpFile.substring(nPos);
                    nPos = strHostName.indexOf("/");
                    if (nPos >= 0)
                        strHostName = strHostName.substring(0,nPos);
                    else
                        strHostName = "";
                }

                if (strHostName.length() > 0)
                {              
                    url = new URL("http://" + strHostName + "/robo/bin/robo.dll?mgr=sys&cmd=updinf");
                    urlConnect = url.openConnection();
                    urlConnect.setDoInput(true);
                    urlConnect.setDoOutput(true);
                    urlConnect.setUseCaches(false);
                    urlConnect.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
                    dataOut = new DataOutputStream(urlConnect.getOutputStream());
                    dataOut.writeBytes(strPostData);
                    dataOut.flush();
                    dataOut.close();

                    dataIn = new DataInputStream(urlConnect.getInputStream());

                    String strTemp="";
                    while (null != ((strTemp = dataIn.readLine())))
                    {
                        strResult += strTemp + "\n";
                    }
                    dataIn.close();
                }
            }
            catch (MalformedURLException e) {
                strResult = "";
            }
            catch (IOException e) {
                strResult = "";
            }
            
            return strResult;
        }
        
        private static Map FH_ParseSvrInfo(String strServerInfo)
        {
            String strSvrData = strServerInfo;
            String strItem = "";
            String strValue = "";
            String strTemp = "";
            int nPos = 0;
            
            String strUpdates = "";
            String strAgts = "";
            int nWndProp = 0;
            int nPanProp = 0;
            
            Map pSvrInfo = new HashMap();
            
            while (strSvrData.length() > 0)
            {
                nPos = strSvrData.indexOf("\n");
                strTemp = strSvrData.substring(0,nPos);
                strSvrData = strSvrData.substring(nPos+1);

                if (nPos == -1)
                    strSvrData = "";
                
                // Get the item name and value
                nPos = strTemp.indexOf("=");
                strItem = strTemp.substring(0,nPos+1);
                strValue = strTemp.substring(nPos+1);
                
                // Store the values
                if (strItem.compareTo(FH_WND_VIEW)==0)
                {
                    if (strValue.compareTo(FH_SINGLE_PANE)==0)
                        strValue = "1";
                    else
                        strValue = "2";
                    pSvrInfo.put(strItem, strValue);
                }
                else if (strItem.compareTo(FH_WND_TOOLBAR)==0)
                {
                    nWndProp += Integer.parseInt(strValue) * FHWO_TOOLBAR; 
                }
                else if (strItem.compareTo(FH_WND_LOCATIONBAR)==0)
                {
                    nWndProp += Integer.parseInt(strValue) * FHWO_LOCATION;
                }
                else if (strItem.compareTo(FH_WND_MENU)==0)
                {
                    nWndProp += Integer.parseInt(strValue) * FHWO_MENUBAR;
                }
                else if (strItem.compareTo(FH_WND_RESIZE)==0)
                {
                    nWndProp += Integer.parseInt(strValue) * FHWO_RESIZABLE;
                }
                else if (strItem.compareTo(FH_WND_STATUSBAR)==0)
                {
                    nWndProp += Integer.parseInt(strValue) * FHWO_STATUS;
                }
                else if (strItem.compareTo(FH_BROWSESEQ)==0)
                {
                    nPanProp += Integer.parseInt(strValue) * PANE_OPT_BROWSESEQ;
                }
                else if (strItem.compareTo(FH_SEARCHINPUT)==0)
                {
                    nPanProp += Integer.parseInt(strValue) * PANE_OPT_SEARCH;
                }
                else if (strItem.compareTo(FH_AGT)==0)
                {
                    strAgts += strValue + "|";
                }
                else if (strItem.compareTo(FH_UPDATE)==0)
                {
                    strUpdates += strValue + "|";
                }
                else
                {
                    pSvrInfo.put(strItem, strValue);
                }
            }

            if (strAgts.indexOf("|", strAgts.length() - 1) == strAgts.length()-1)
                strAgts = strAgts.substring(0,strAgts.length()-1);
            
            pSvrInfo.put(FH_WOT, String.valueOf(nWndProp));
            pSvrInfo.put(FH_POT, String.valueOf(nPanProp));
            pSvrInfo.put(FH_UPDATE, strUpdates);
            pSvrInfo.put(FH_AGT, strAgts);
          
            return pSvrInfo;
        }
        
        private static boolean FH_DownloadFile(String fileURL, String newFileLocation)
        {
            boolean bResult = true;
            try{
                URL newURL = new URL(fileURL);
                
                BufferedInputStream bis = new BufferedInputStream(newURL.openConnection().getInputStream());
                BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(newFileLocation));
                byte[] buff = null;
                int bytesRead;
                while(-1 != (bytesRead = bis.read())) {
                    bos.write(bytesRead);
                }
                bis.close();
                bos.close();
            }
            catch (Exception e)
            {
                bResult = false;
            }    
            return bResult;
        }
        
        private static boolean FH_UpdateQS(String strUpdateList, String strEngineURL, String strQuick)
        {
            boolean bResult = true;
            String strFileList = strUpdateList;
            String strFileName = "";
            String strURL = "";
            int nLast = 0;
            int nPos = 0;
            
            if (strEngineURL.substring(strEngineURL.length()-1) == "\\")
            {
                strEngineURL = strEngineURL.substring(0,strEngineURL.length()-1);
            }
            
            while (strFileList.length() > 0)
            {
                // Get the next outdated file
                nPos = strFileList.indexOf("|");
                strURL = strFileList.substring(0, nPos);
                strFileList = strFileList.substring(nPos+1);
                
                // extract the file name
                nLast = 0;
                nPos  = strURL.indexOf("/");
                while (nPos >= 0)
                {
                    nLast = nPos + 1;
                    nPos = strURL.indexOf("/",nLast);
                }
                
                if (nLast >= 0)
                    strFileName = strURL.substring(nLast);
                
                strURL = strEngineURL + strURL;
                
                if (!FH_DownloadFile(strURL, strQuick + strFileName))
                {
                    bResult = false;
                }
            }
            
            return bResult;
        }
        
        private static String FH_URLEncode(String strIn)
        {
            String strResult = strIn;
            String strChars = "%:/ ";
            String strCurChar = "";
            String strStart = "";
            String strEnd = "";
            String strCharCode="";
            int nAscii = 0;
            int nPos = 0;

            for (int i = 0; i < strChars.length(); i++)
            {   
                strCurChar = strChars.substring(i,i+1);
                nAscii = strChars.charAt(i);
                strCharCode = "%" + java.lang.Integer.toHexString(nAscii);
                nPos = strResult.indexOf(strCurChar);
                while (nPos >=0)
                {
                    strStart = strResult.substring(0, nPos);
                    strEnd = strResult.substring(nPos+1);
                    strResult = strStart + strCharCode;
                    nPos = strResult.length();
                    strResult += strEnd;                                  
                    nPos = strResult.indexOf(strCurChar,nPos);
                }
            }
            
            return strResult;
        }
        
        private static boolean FH_WriteOptionsJs(Map pSvrInfo, String strProject, String strSvrPath, String strQuick)
        {
            boolean bResult = true;
            String strResult = "svr="+strSvrPath + ">>prj=" + strProject;
           
            if (pSvrInfo.containsKey(FH_WND_NAME))
                strResult += ">>wnm=" + pSvrInfo.get(FH_WND_NAME);
            if (pSvrInfo.containsKey(FH_WND_X))
                strResult += ">>x=" + pSvrInfo.get(FH_WND_X);
            if (pSvrInfo.containsKey(FH_WND_Y))
                strResult += ">>y=" + pSvrInfo.get(FH_WND_Y);
            if (pSvrInfo.containsKey(FH_WND_WIDTH))
                strResult += ">>dx=" + pSvrInfo.get(FH_WND_WIDTH);
            if (pSvrInfo.containsKey(FH_WND_HEIGHT))
                strResult += ">>dy=" + pSvrInfo.get(FH_WND_HEIGHT);
            if (pSvrInfo.containsKey(FH_WND_DEFAULT))
                strResult += ">>udw=" + pSvrInfo.get(FH_WND_DEFAULT);
            if (pSvrInfo.containsKey(FH_WOT))
              strResult += ">>wot=" + pSvrInfo.get(FH_WOT);
            if (pSvrInfo.containsKey(FH_WND_WORK))
                strResult += ">>wrk=" + pSvrInfo.get(FH_WND_WORK);
            if (pSvrInfo.containsKey(FH_WND_VIEW))
                strResult += ">>pan=" + pSvrInfo.get(FH_WND_VIEW);
            if (pSvrInfo.containsKey(FH_POT))
                strResult += ">>pot=" + pSvrInfo.get(FH_POT);
            if (pSvrInfo.containsKey(FH_DEFAULTAGT))
                strResult += ">>pdb=" + pSvrInfo.get(FH_DEFAULTAGT);
            if (pSvrInfo.containsKey(FH_AGT))
                strResult += ">>pbs=" + pSvrInfo.get(FH_AGT);
            if (pSvrInfo.containsKey(FH_CTXTOPIC))
                strResult += ">>url=" + pSvrInfo.get(FH_CTXTOPIC);
            if (pSvrInfo.containsKey(FH_WND_CAPTION))
                strResult += ">>cap=" + pSvrInfo.get(FH_WND_CAPTION);
            
            strResult = FH_URLEncode(strResult);
            
            try{
                FileWriter fw = new FileWriter(strQuick + "wf_hash.js");
                PrintWriter pw = new PrintWriter(fw);
                strResult = "gsHash=\"" + strResult + "\";";
                pw.println(strResult);
                pw.close();
                fw.close();
            }
            catch (IOException e){
                bResult = false;
            }
            return bResult;
        }
        
        private static boolean ShowFlashHelp_QS(String strHelpURL, String strQuick, String strWnd, int uCommand, int nMapId, int nUpdate)
        {            
            boolean bResult = false;
            String strUpdate = "";
            String strPostData = "";
            String strServerData = "";
            String strEngineURL = "";
            String strProjectName = "";
            int nPos;
        
            // Strip the asp page name and get the project name
            nPos = strHelpURL.indexOf("?project=");
            if (nPos != 0)
            {
                strProjectName = strHelpURL.substring(nPos + 9);
            }

            nPos = strHelpURL.indexOf("roboapi.asp");
            if (nPos >= 0)
            {
                strEngineURL = strHelpURL.substring(0, nPos - 1);
            }
            
            // Get the Update String
            strUpdate = FH_GetUpdateString(strQuick);
           
            if (strUpdate.length() > 0)
            {
                // Build the post data
                
                // Add the project Information
                strPostData += "prj=";
                if (strProjectName.length() >0)
                {
                    strPostData += strProjectName;
                }
                
                // Add Window Information
                strPostData += "&wnd=";
                if (strWnd.length() > 0)
                {
                    strPostData += strWnd;
                }
                
                // Add Ctx ID
                strPostData += "&ctx=";
                if (uCommand == HH_HELP_CONTEXT)
                {
                   strPostData += nMapId;
                }
                
                // Add Update data
                strPostData += "&update=" + strUpdate;
         
                // Post the data
                strServerData = FH_PostUpdateData(strHelpURL, strPostData);
                System.out.println(strServerData);
                if (strServerData.length() > 0)
                {
                    bResult = true;
                    
                    // Parse the server response
                    Map pSvrInfo = FH_ParseSvrInfo(strServerData);
                                       
                    // Update the outdated files
                    String strUpdateFiles = "";
                    strUpdateFiles = String.valueOf(pSvrInfo.get(FH_UPDATE));
                    if (strUpdateFiles.length() > 0)
                        bResult = FH_UpdateQS(strUpdateFiles,strEngineURL, strQuick);
                    
                    // Show the help system
                    if (bResult)
                    {
                        String strQSPath = "";
                        
                        strQSPath = strQuick + FH_QUICK_START;
                        strQSPath +="#newwnd=false";
                                        
                        if (FH_WriteOptionsJs(pSvrInfo, strProjectName, strEngineURL, strQuick))
                        {
                            ShowHelpURL(strQSPath);                       
                        }
                        else
                            bResult = false;
                    }
                }
            }
            
            return bResult;
        }
        
	private static boolean ShowWebHelpServer_CSH(String strHelpPath, String strWnd, int uCommand, int nMapId)
	{
		String strUrl = "";
		if (strHelpPath != null)
		{
			if (uCommand == HH_HELP_CONTEXT)
			{
				if (strHelpPath.indexOf("?") == -1)
					strUrl = strHelpPath + "?ctxid=" + nMapId;
				else
					strUrl = strHelpPath + "&ctxid=" + nMapId;
			}
			else
			{
				if (strHelpPath.indexOf("?") == -1)
					strUrl = strHelpPath + "?ctxid=0";
				else
					strUrl = strHelpPath + "&ctxid=0";
			}	
			if (strWnd != null && strWnd.length() > 0)
			{
				strUrl += ">" + strWnd;
			}
			return ShowHelpURL(strUrl);
		}
		return false;
	}

	private static boolean ShowWebHelp_CSH(String strHelpPath, String strWnd, int uCommand, int nMapId)
	{
		String strHelpFile = "";
		if (strHelpPath != null)
		{
			if (uCommand == HH_DISPLAY_TOPIC)
			{
				strHelpFile = strHelpPath + "#<id=0";
			}
			if (uCommand == HH_HELP_CONTEXT)
			{
				strHelpFile = strHelpPath + "#<id=" + nMapId;
			}
			else if (uCommand == HH_DISPLAY_INDEX)
			{
				strHelpFile = strHelpPath + "#<cmd=idx";
			}
			else if (uCommand == HH_DISPLAY_SEARCH)
			{
				strHelpFile = strHelpPath + "#<cmd=fts";
			}
			else if (uCommand == HH_DISPLAY_TOC)
			{
				strHelpFile = strHelpPath + "#<cmd=toc";
			}
			if (strHelpFile.length() > 0)
			{
				if (strWnd != null && strWnd.length() > 0)
				{
					strHelpFile += ">>wnd=" + strWnd;
				}
				strHelpFile += ">>newwnd=false";
				return ShowHelpURL(strHelpFile);
			}

		}
		return false;
	}


	private static boolean ShowHelpURL(String strUrl)
	{
		String strPath=null;
			
		String strOSName = System.getProperty("os.name").toLowerCase();
		if (strOSName.startsWith("window"))
		{
			if (isFromMS())
			{
				String strWndPath = System.getProperty("com.ms.windir");
				if (strWndPath != null)
				{
					strPath = strWndPath.substring(0, 2) + "\\Program Files\\Internet Explorer\\IEXPLORE.EXE";
				}
			}
			if (strPath == null)
			{
				String strPaths = System.getProperty("java.library.path");
				String strSeperator=System.getProperty("path.separator");
				int nPos = 0;
				boolean bFound = false;
				int nPosx = -1;
				do 
				{
					nPosx = strPaths.indexOf(strSeperator, nPos);
					if (nPosx != -1)
					{
						strPath = strPaths.substring(nPos, nPosx);
						strPath.replace('/', '\\');
						nPos = nPosx + 1;
						int nFindPos = -1;
						if ((nFindPos = strPath.toLowerCase().indexOf("\\program files")) != -1)
						{
							if (strPath.length() == nFindPos + 14 ||
								strPath.charAt(nFindPos + 14)=='\\')
							{
								bFound = true;
								break;
							}
						}
						else if ((nFindPos = strPath.toLowerCase().indexOf("\\progra~1")) != -1)
						{
							if (strPath.length() == nFindPos + 9 ||
								strPath.charAt(nFindPos + 9)=='\\')
							{
								bFound = true;
								break;
							}
						}
						else if ((nFindPos = strPath.toLowerCase().indexOf("\\windows")) != -1)
						{
							if (strPath.length() == nFindPos + 8 ||
								strPath.charAt(nFindPos + 8)=='\\')
							{
								strPath = strPath.substring(0,nFindPos + 1) + "program files";
								bFound = true;
								break;
							}
						}						
					}
				} while (nPosx != -1);
				if (bFound)
				{
					int nPos1 = strPath.toLowerCase().indexOf("program files");
					strPath = strPath.substring(0, nPos1 + 13);
					strPath += "\\Internet Explorer\\IEXPLORE.EXE";
				}
			}
		}
		else if(strOSName.startsWith("mac"))
		{
			try
			{
				File fTemp = File.createTempFile("rhcsh", ".htm");
				fTemp.deleteOnExit();
				BufferedWriter out = new BufferedWriter(new FileWriter(fTemp));
				out.write("<html><script language=\"javascript\">\n\twindow.location=\"" + strUrl + "\";\n</script><body></body></html>");
				out.close();
				strUrl = fTemp.getPath();
				if(strUrl.startsWith("/"))
					strUrl = "file://" + strUrl;
				
				Class fileManager = Class.forName("com.apple.eio.FileManager");
				Method openURLMethod = fileManager.getDeclaredMethod("openURL", new Class[] {String.class});
				openURLMethod.invoke(null, new Object[] {strUrl});
				Thread.sleep(4000);
				return true;
			}
			
			catch(Exception e)
			{
				return false;
			}
			
		}
		else
		{
			strPath = "firefox";
		}

		if (strPath != null)
		{
			try {
				Runtime.getRuntime().exec(new String[] {strPath, strUrl} );
				 if(DebugOutput.isSetFor("help"))
		                Messages.postDebug("RoboHelp Cmd: " + strPath + " \"" + strUrl + "\"");
				return true;
			}
			catch (Exception e)
			{
				return false;
			}
		}
		else
			return false;
	}

	private static boolean isFromMS()
	{
		String s = System.getProperty("java.vendor");
		if (s.toLowerCase().indexOf("microsoft") == 0)
			return true;
		else
			return false;
	}

	private static boolean isFromSun()
	{
		String s = System.getProperty("java.vendor");
		if (s.toLowerCase().indexOf("sun") == 0)
			return true;
		else
			return false;
	}

	private static boolean isWindows()
	{
		String s = System.getProperty("os.name");
		if (s.toLowerCase().indexOf("window") == 0)
			return true;
		else
			return false;
	}
	
	private static boolean isServerBased(String strHelpFile)
	{
		if (strHelpFile.length() > 0)
		{
			int nPos = strHelpFile.lastIndexOf('.');
			if (nPos != -1 && strHelpFile.length() >= nPos + 4)
			{
				String sExt = strHelpFile.substring(nPos, nPos + 4);
				if (sExt.equalsIgnoreCase(".htm"))
				{
					return false;
				}
			}
		}
		return true;
	}
	
	private static String makeUrlForServerBased(String a_pszHelpFile, String strContextID)
	{
		return a_pszHelpFile + "?context=" + strContextID;
	}

	private static String makeUrlForWebHelpBased(String a_pszHelpFile, String strContextID)
	{
		return a_pszHelpFile + "#context=" + strContextID;
	}

	// return a URL build from the local and url parameters
	private URL makeURL (URL base, String local, String url) throws MalformedURLException
	{
		try {
				//return new URL (base, local);
				String protocol = base.getProtocol();
				String host		= base.getHost();
				String file		= base.getFile();
				int	   port		= base.getPort();

				String fileNew0 = tuHtmlToText(file);
				String fileNew1 = GetNormalizedLocal(fileNew0);
				String fileNew	= TruncURLtoQuestionMark(fileNew1);
				URL baseNew = new URL(protocol,host,port,fileNew);

				String localNew0 = tuHtmlToText(local);
				String localNew = GetNormalizedLocal(localNew0);
				return new URL (baseNew, localNew);
		}
		catch (MalformedURLException x) {
			x.printStackTrace();

			// no it isn't - but it might be
			// a MS-type file spec (eg e:\temp\foo)
			// so try opening it as a file

			File f = new File (local);
			if (f.exists ()) {
				// OK - it's a local file, try appending
				// "file:/" onto it and opening it - even
				// though the slashes will be all funny
				// this seems to work!
				return new URL ("file:/" + local);
			}
			else {

				// the file doesn't exist....
				// should display a dialog box here
				// and now try the secondary URL

				return new URL (base, url);
			}
		}
	}
	
	private String TruncURLtoQuestionMark(String str)
	{
		String strRc = str;
		int nQuestionMarkPos = str.indexOf('?');
		if (nQuestionMarkPos != -1) {
			strRc = str.substring(0, nQuestionMarkPos);
		}
		return strRc;
	}
	
	private static String GetNormalizedLocal(String str)
	{
		String local = str;
		// fix up special characters (for netscape)
		for (int i = 0; i < local.length(); i++)
		{
			if (local.charAt(i) > 127)
			{
				local = local.substring(0, i) + "%" + Integer.toString(local.charAt(i), 16) + local.substring(i + 1, local.length());
			}
		}
		return local;
	}
	
	//To Do: need a utility to convert html<->text
	//Copy from SiteMapParserToContents
	//String fixSpecialCharacters(String value) 
	private static String  tuHtmlToText(String value) 
	{
		if (value == null)
			return null;
		
		// TODO: This should be common in SiteMapParser or something
		int i = value.indexOf('&');
		
		if (i < 0) return value;

		String strOut = "";

		// have to convert &amp; and the like
		while (i > -1 && i < value.length() - 2) {
			strOut += value.substring(0, i);
//	System.out.println("1strOut is now '" + strOut + "'");
			String str = value.substring(i);
			String strEnd = "";
			int j = str.indexOf(';');
			if (j < 0) {
				strOut += str;
//	System.out.println("2strOut is now '" + strOut + "'");
				break;	// no termination
			}
			if (j < str.length()-1) {
				value = str.substring(j+1);
			}
			else {
				value = "";
			}
			str = str.substring(1, j);
//	System.out.println("value is now '" + value + "'");
//	System.out.println("str is now '" + str + "'");
			// TODO: Should use a hashtable and support more characters
			switch (Character.toUpperCase(str.charAt(0))) 
			{
			case 'A':
				if (str.equalsIgnoreCase("amp")) {
					str = "&";
				}
				break;
			case 'C':
				if (str.equalsIgnoreCase("copy")) {
					str = "(c)";
				}
				break;
			case 'G':
				if (str.equalsIgnoreCase("gt")) {
					str = ">";
				}
				break;
			case 'L':
				if (str.equalsIgnoreCase("lt")) {
					str = "<";
				}
				break;
			case 'N':
				if (str.equalsIgnoreCase("nbsp")) {
					str = " ";
				}
				break;
			case 'Q':
				if (str.equalsIgnoreCase("quot")) {
					str = "\"";
				}
				break;
			case 'R':
				if (str.equalsIgnoreCase("reg")) {
					str = "(R)";
				}
				break;
			}
			strOut += str;
			i = value.indexOf('&');
			if (i < 0) {
				strOut += value;
//	System.out.println("3strOut is now '" + strOut + "'");
			}
		}

//	System.out.println("4strOut is now '" + strOut + "'");
		return strOut;
	}
	
	private static boolean DoesFileExists(String strFilePath)
	{
		File f = new File(strFilePath);
		return f.exists();
	}
	
	public static boolean RH_AIR_ShowHelp(String a_pszViewerPath, 
										String a_pszHelpId,
										String a_pszWindowName,
										long ulMapNum,
										String a_pszMapId,
										String a_pszTopicURL)
	{
		String strCommandLine = a_pszViewerPath;
		
		
		boolean bRetVal = false;
		if(DoesFileExists(a_pszViewerPath))
		{
			strCommandLine += " -csh" ;
			if( a_pszHelpId.length() > 0)
			{
				strCommandLine += " helpid \"" + a_pszHelpId + "\"" ; 
			}
			
			if(a_pszWindowName.length()>0)
			{
				strCommandLine += " window \"" + a_pszWindowName + "\"" ; 
			}
			
			if(a_pszTopicURL.length() > 0)
			{
				strCommandLine += " topicurl \"" + a_pszTopicURL + "\"" ; 
			}
			
			else if(a_pszMapId.length() > 0)
			{
				strCommandLine += " mapid \"" + a_pszMapId + "\"" ; 
			}
			else
			{
				strCommandLine += " mapnumber " + ulMapNum;
			}
			try
			{
				Runtime.getRuntime().exec(strCommandLine);
				bRetVal = true;
			}
			catch(Exception e)
			{
				bRetVal = false;
			}
		}
		return bRetVal;
	}
}
