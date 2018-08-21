/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.File;
import java.util.ArrayList;
import java.util.Hashtable;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTabbedPane;
import javax.swing.Timer;
import java.awt.event.*;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import org.xml.sax.Attributes;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.DefaultHandler;

import vnmr.bo.*;
import vnmr.ui.SessionShare;
import vnmr.util.DisplayOptions;
import vnmr.util.FileUtil;
import vnmr.util.Util;
import vnmr.util.Messages;
import vnmr.util.UNFile;
import vnmr.util.ProtocolLabelHandler;
import vnmr.util.RightsList;
import vnmr.util.HashArrayList;
import vnmr.util.DebugOutput;
import vnmr.templates.*;
import vnmr.admin.util.*;

public class ExpCards extends JPanel implements	PropertyChangeListener {


    ArrayList	appDirs;
    HashArrayList fromProtocolList = new HashArrayList();
    ArrayList   userOffList = new ArrayList();
    ArrayList   allLabels = new ArrayList();
    long	lastMod[] = new long[10];
    Container   c;
    Dimension	zero = new Dimension(0,0);
    Font	labelFont;
    JTabbedPane jtp;
    SessionShare sshare;
    String      protocolType, protocolName;
    Timer times = null;

    DroppableList listPanel;
    protected MouseAdapter m_mouseListener;
   
    static public UpdateCards updateCards=null;

    public ExpCards(SessionShare sshare) {
        super( new BorderLayout() );
        this.sshare = sshare;
        labelFont = DisplayOptions.getFont(null,null,null);
        
        updateCards = new UpdateCards();

        jtp = new JTabbedPane();
        jtp.setFont(labelFont);
        jtp.setForeground(Color.BLUE);
        JScrollPane jsp = new JScrollPane(jtp);
        this.add(jsp,BorderLayout.CENTER);

        appDirs = getAppDirs();

        times = new Timer(10000, new UpdateCards());
        times.setInitialDelay(0);
        times.start();

        DisplayOptions.addChangeListener(this);
    }

    /* PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent pce) {
        if (jtp != null) {
            jtp.setForeground(DisplayOptions.getColor("PlainText"));
        }
        String strProperty = pce.getPropertyName();
        if (strProperty == null)
            return;
        strProperty = strProperty.toLowerCase();
        if (strProperty.indexOf("vjbackground") >= 0)
        {
            Color bgColor = Util.getBgColor();
            setBackground(bgColor);
            WUtil.doColorAction(bgColor, "", this);
        }
    }

    public ArrayList getAppDirs() {
        ArrayList dirs = sshare.user().getAppDirectories();
        return(dirs);
    }

    protected MouseAdapter getMouselistener()
    {
        if (m_mouseListener == null)
        {
            m_mouseListener = new MouseAdapter()
            {
                public void mouseClicked(MouseEvent e)
                {
                    DraggableLabel label = (DraggableLabel)e.getSource();
                    ShufflerItem shufflerItem = label.getShufflerItem();
                    QueuePanel queuePanel = Util.getStudyQueue();
                    if (queuePanel != null)
                    {
                        StudyQueue studyQueue = queuePanel.getStudyQueue();
                        if (studyQueue != null)
                        {
                            ProtocolBuilder protocolBuilder = studyQueue.getMgr();
                            protocolBuilder.appendProtocol(shufflerItem);
                        }
                    }
                }
            };
        }
        return m_mouseListener;
    }


    public void finalize() {
        if (times != null)
           times.stop();
        times = null;
        if (jtp != null)
           jtp.removeAll();
        jtp = null;
    }

    public class UpdateCards implements ActionListener {

       String protocolFamily = null;
       String protocolTitle  = null;
       String protocolName   = null;
       String protocolAppl   = null;
       String protocolAuthor = null;
       public void actionPerformed(ActionEvent ae) {
           boolean update = false;
           int i,j;
           File tmpFile=null;
           String tmpString;
           String profile, operator;
           String iface = "Experimental";
           String idir = FileUtil.SYS_VNMR;
           DefaultHandler listSAXHandler=null;
           boolean useProfile=false;
           File profileFile=null;
           boolean foundOne;

           // If cmd = force, then we the operator has changed and we
           // need to force an update.
           String cmd = ae.getActionCommand();
           if(cmd != null && cmd.equals("force"))
               update = true;
           
           // Check to see if this user has a profile set in the operator
           // login info
           operator = Util.getCurrOperatorName();
           // Get the Profile Name column string for access to the operator data
           String pnStr = vnmr.util.Util.getLabel("_admin_Profile_Name");
           profile = WUserUtil.getOperatordata(operator, pnStr);

           // This could be temporary, but DefaultApproveAll does not exist
           // as a profile and AllLiquids does exist, so use it.
           if(profile.equals("DefaultApproveAll")) {
               profile = "AllLiquids";
           }
           if(profile == null || profile.length() == 0) {
               profile = "AllLiquids";
           }

           // We arrive here about every 10 sec.

           // If the profile is specified and exists, then use it,
           // else, use the older protolList<iface>.xml file
           if(profile != null && profile.length() > 0) {
               // If profile does not end in .xml, add it
               if(!profile.endsWith(".xml"))
                   profile = profile + ".xml";

               // The area to get these profiles is /vnmr/adm/users/rights
               String filepath;
               filepath = FileUtil.openPath("USRS/userProfiles/" + profile);
				if (filepath != null) {
					profileFile = new File(filepath);
					if (profileFile.exists()) {
						listSAXHandler = new ProfileHandler();
						// This tmpFile is used below to as an arg to parse
						// to get this file parsed.
						tmpFile = profileFile;
						useProfile = true;

						// Parse the profile only the first time through at
						// which
						// time, lastMod[0] will still be at 0.
						if (lastMod[0] == 0) {
							update = true;
							lastMod[0] = 1;
						}
					}
				}
               // The users vnmrsys directory needs to be in idir below
               // for opening the protocols
               idir = FileUtil.usrdir();
           }

           if(useProfile) {
               // There is a persistence file for each operator
               String curOperator = Util.getCurrOperatorName();
               
               // Check the operators RightsConfig.txt file for changes
               String filepath = FileUtil.openPath("USER" + File.separator 
                                             + "PERSISTENCE" + File.separator 
                                             + "RightsConfig_" + curOperator
                                             + ".txt");
               if(filepath != null) {
                   File file = new File(filepath);
                   long modTime = file.lastModified();
                   // Use 0 for the RightsConfig file and 1 for the protocol dir
                   if (modTime > lastMod[0]) {
                       update = true;
                       lastMod[0] = modTime;
                   }
               }
               // Check the users protocol directory for date change
               // Get the current user object
               String curuser = System.getProperty("user.name");
               LoginService loginService = LoginService.getDefault();
               Hashtable userHash = loginService.getuserHash();
               User user = (User)userHash.get(curuser);
               filepath = FileUtil.userDir(user, "PROTOCOLS");
              
               if(filepath != null) {
                   File file = new File(filepath);
                   long modTime = file.lastModified();
                   // Use 0 for the RightsConfig file and 1 for the protocol dir
                   if (modTime > lastMod[1]) {
                       update = true;
                       lastMod[1] = modTime;
                   }
               }
               
               // Check the users assigned profile for changes
               if(profileFile != null) {
                   long modTime = profileFile.lastModified();
                   // lastMod[2] is the profile
                   if (modTime > lastMod[2]) {
                       update = true;
                       lastMod[2] = modTime;
                   }
               }
           }
           // If no profile was set, do it the old way
           else {

               for (i=0; i<appDirs.size(); i++) {
                   tmpString = (String)appDirs.get(i);
                   if (i == (appDirs.size()-2)) {
                       int starti = tmpString.lastIndexOf(File.separator) + 1;
                       int endi = tmpString.length();
                       if (starti < endi)
                       {
                           String nface;
                           idir = tmpString;
                           iface = tmpString.substring(starti,endi);
                           nface = iface.substring(0,1).toUpperCase() + iface.substring(1,iface.length());
                           iface = nface;
                       }
                   }
               }
// System.out.println("interface = "+iface+"   directory = "+idir);
               for (i=0; i<appDirs.size(); i++) {
                   tmpString = (String)appDirs.get(i);
                   if (tmpString.startsWith(FileUtil.SYS_VNMR)) continue;
                   tmpFile = new File(new StringBuffer().append(tmpString).append(File.separator).
                                      append("templates").append(File.separator).
                                      append("vnmrj").append(File.separator).
                                      append("protocols").toString());
                   long modTime = tmpFile.lastModified();
// System.out.println((String)tmpString+" at "+lastMod[i]+", now="+modTime);
                   if (modTime > lastMod[i]) {
                       update = true;
                       lastMod[i] = modTime;
                   }
               }
               tmpString = FileUtil.sysdir();
               tmpFile = new File(new StringBuffer().append(tmpString).append(File.separator).
                                  append("adm").append(File.separator).append("users").append(File.separator).
                                  append("protocolList").append(iface).append(".xml").toString());
               if ( ! tmpFile.exists() ) {
                   StringBuffer Sbuffer = new StringBuffer().append(tmpString).append(File.separator).
                       append("adm").append(File.separator).append("users").append(File.separator).
                       append("protocolListWalkup.xml");
                   tmpFile = new File(Sbuffer.toString());
                   if (( ! tmpFile.exists()) && ( update )) {
                       Messages.postError("Cannot find "+Sbuffer.toString());
                       System.out.println("Cannot find "+Sbuffer.toString());
                   }
               }
               long modTime = tmpFile.lastModified();
// System.out.println((String)tmpString+" at "+lastMod[i]+", now="+modTime);
               if (modTime > lastMod[i]) {
                   update = true;
                   lastMod[i] = modTime;
               }

// System.out.println("update="+update);
               if ( ! update) return;

//           System.out.println("ExpCards(): Filling Cards using ");

               listSAXHandler = new ProtocolListHandler();
           }

           // clear out the list of protocols and start over
           fromProtocolList.clear();


           if(!update) {
               return;
           }

           if(DebugOutput.isSetFor("expcards")) {
               Messages.postDebug("UpdateCards updating Exp Selector for "
                       + operator);
           }
           // If using a profile, now get the local protocols and add
           // them to fromProtocolList
           if(useProfile) {
               getLocalProtocols();
           }

           SAXParser parser = getSAXParser();
           DefaultHandler protocolSAXHandler = new MySaxHandler();
           allLabels.clear();
           jtp.removeAll();

           try {
               parser.parse(tmpFile, listSAXHandler);
           }
           catch (Exception e) {
               Messages.writeStackTrace(e);
           }


           DraggableLabel tmpLabel;
           ProtocolEntry pe;
           JPanel panel;
           // rightsList will contain the list of protocols that the user
           // does not want display with an approve = false
           boolean all = false;
           RightsList rightsList =  new RightsList(all);
           
           // Sort fromProtocolList so that the name are in the same order
           // as protocolTransTable
           HashArrayList sorted = new HashArrayList();
           HashArrayList protocolTransTable = 
                                  ProtocolLabelHandler.getProtocolTransTable();
           for(int k=0; k < protocolTransTable.size(); k++) {
               String protocol = (String)protocolTransTable.getKey(k);
               // take things out of fromProtocolList in the order 
               // encountered in protocolTransTable
               if(fromProtocolList.containsKey(protocol) && !sorted.containsKey(protocol))
                   sorted.put(protocol, fromProtocolList.get(protocol));
           }
           // If there are items that were in fromProtocolList but not
           // protocolTransTable, then put them at the end
           for(int k=0; k < fromProtocolList.size(); k++) {
               String protocol = (String)fromProtocolList.getKey(k);
               // We don't have this one yet, add it
               if(!sorted.containsKey(protocol))
                   sorted.put(protocol, fromProtocolList.get(protocol));             
           }
           // Now replace fromProtocolList with the sorted version
           fromProtocolList = sorted;
          
 
           // Get the list of appdir directories
           ArrayList appDirs = FileUtil.getAppDirs();
           
           // Get and save the relative path to the protocols below each
           // of the appdirs
           String relativePath = FileUtil.getSymbolicPath("PROTOCOLS");


// System.out.println("protocolList has "+fromProtocolList.size()+" parts");
           for (i=0; i<fromProtocolList.size(); i++) {
               File xmlFile;
               String xmlString;
               pe = (ProtocolEntry)fromProtocolList.get(i);

               // If the operator has a RightsConfig.txt persistence file,
               // it may request that some of these protocols NOT be
               // displayed.  Test the list from persistence, and if a
               // protocol is tagged 'false', leave it out of this list.
               // For protocols, the keyword in rightsList is equal to name,
               // so use name here for the keyword
               if(rightsList.approveIsFalse(pe.name))
                   continue;

               // See if the protocol exists in one of the appdir directories
               // If not, don't put it into the exp selector

               // We want to keep track of whether or one we found a copy
               // of each protocol, init the flag
               foundOne = false;
               
               // Go thru the list
               for(int k=0; k < appDirs.size(); k++) {
                   String dir = appDirs.get(k) + File.separator + relativePath;
                   String name = pe.name + ".xml";
                   String path = dir + File.separator + name;

                   xmlFile = new File(path);
                   // Does a protocol exist in this appdir?
                   if (xmlFile.exists() ) {
                       try {
                           // Yes, parse it
                           parser.parse( xmlFile, protocolSAXHandler);
                           // translate the protocol name to the desired label and
                           // use that in the creation of tmpLabel below.
                           String protocolLabel;
                           protocolLabel = ProtocolLabelHandler.getLabel(protocolName,
                                                                         "protocol");

                           tmpLabel = new DraggableLabel(pe.label);
                           tmpLabel.setFileName(path);
                           tmpLabel.setName(protocolLabel);
                           tmpLabel.setApplication(protocolAppl);
                           tmpLabel.setAuthor(protocolAuthor);
                           tmpLabel.setFont(labelFont);
                           tmpLabel.setForeground(Color.BLUE);
                           tmpLabel.setSource("EXPSELECTOR");

                           tmpLabel.addMouseListener(getMouselistener());
                           // Get the translated apptype from ProtocolLabels.xml
                           String apptypeLabel = ProtocolLabelHandler.getLabel(pe.type, "apptype");

                           tmpLabel.setAppTab(apptypeLabel);
                           allLabels.add(tmpLabel);
                           
                           // Set flag that we found it
                           foundOne = true;
                           
                       }
                       catch (Exception e) {
                           Messages.writeStackTrace(e);
                           // check the next appdir directory
                           continue;
                       }
                       // If we found and successfully parsed one, do not
                       // go on to the next directory in appdir, go to the
                       // next protocol to test.
                       break;
                   }
               }
               
               // If we did not find this protocol, then log that fact
               if(!foundOne) {
                   Messages.postDebug("Cannot find protocol "+ pe.name + ".xml"
                           + " for Experiment Selector."
                           + "\n   Possibly in user\'s profile, but not "
                           + "in protocols dir.");
               }
           }


           if(!useProfile) {
               for (i=0; i<appDirs.size(); i++) {
                   File   oneDir;
                   String[] protocolList;
                   tmpString = (String)appDirs.get(i);
// System.out.println("appDirs["+i+"]='"+tmpString+"'");
                   if (tmpString.startsWith(FileUtil.SYS_VNMR)) continue;
                   oneDir = new File(new StringBuffer().append(tmpString).append(File.separator).
                                     append("templates").append(File.separator).
                                     append("vnmrj").append(File.separator).
                                     append("protocols").append(File.separator).
                                     toString());
                   if (oneDir.exists() ) {
                       protocolList = oneDir.list();
                       for (j=0; j<protocolList.length; j++) {
                           String xmlFile = new StringBuffer().append(oneDir).
                               append(File.separator).
                               append(protocolList[j]).
                               toString();
// System.out.println("    "+protocolList[j]);
                           try {
                               parser.parse( new File(xmlFile), protocolSAXHandler);
                           }
                           catch (Exception e) {
                               Messages.writeStackTrace(e);
                           }
//                       System.out.println("   type='"+protocolFamily+"'");

                           tmpLabel = new DraggableLabel(protocolTitle);
                           tmpLabel.setFileName(xmlFile);
                           tmpLabel.setName(protocolName);
                           tmpLabel.setApplication(protocolAppl);
                           tmpLabel.setAuthor(protocolAuthor);
                           tmpLabel.setFont(labelFont);
                           tmpLabel.setForeground(Color.BLUE);
                           tmpLabel.addMouseListener(getMouselistener());
                           tmpLabel.setAppTab("Local");
                           allLabels.add(tmpLabel);
                       }
                   }
               }
           }
           // We want Tabs create in the order apptypes are listed in
           // ProtocolLabels.xml.  So, create these tabs first.  Then
           // let the next section of code add tmpLabels and add any
           // tabs that were not in ProtocolLabels.xml.
           // First, get the list of all apptypes in allLabels now.
           ArrayList allApptypes = new ArrayList();
           for(int k=0; k < allLabels.size(); k++) {
               tmpLabel = (DraggableLabel) allLabels.get(k);
               protocolFamily = tmpLabel.getAppTab();
               if(!allApptypes.contains(protocolFamily))
                   allApptypes.add(protocolFamily);
           }
           
           if(DebugOutput.isSetFor("expcards")) {
               Messages.postDebug("UpdateCards creating tabs: " + allApptypes
                       + " for " + operator);
           }
           // Now create the tabs needed in order
           HashArrayList apptypeTransTable =   
                                  ProtocolLabelHandler.getApptypeTransTable();
           for(int k=0; k < apptypeTransTable.size(); k++) {
               String apptypeLabel = (String)apptypeTransTable.get(k);
               if(allApptypes.contains(apptypeLabel)) {
                   if (jtp.indexOfTab(apptypeLabel) == -1 ) {
                       panel = new JPanel(new GridLayout(0,2));
                       jtp.addTab(apptypeLabel, panel);
                   }
               }
           }
           
           // This will create any tabs not in ProtocolLabels.xml
           // and add in the tmpLabels
           int  n=allLabels.size();
           for (i=0; i<n; i++)
           {
               tmpLabel = (DraggableLabel) allLabels.get(i);
               protocolFamily = tmpLabel.getAppTab();
               if (jtp.indexOfTab(protocolFamily) == -1 ) {
//System.out.println("Adding Tab: "+pe.type);
                   panel = new JPanel(new GridLayout(0,2));
                   jtp.addTab(protocolFamily, panel);
               }
//System.out.println("  Adding Label: "+pe.label);
               panel = (JPanel)jtp.getComponentAt(jtp.indexOfTab(protocolFamily));
               panel.add(tmpLabel);
           }
           WUtil.doColorAction(Util.getBgColor(), "", jtp);
       }


        public class MySaxHandler extends DefaultHandler {

            public void endDocument() {
                //System.out.println("End of Document");
            }

            public void endElement(String uri, String localName, String qName) {
                //System.out.println("End of Element '"+qName+"'");
            }
            public void startDocument() {
                //System.out.println("Start of Document");
            }
            public void startElement(String uri,   String localName,
                                    String qName, Attributes attr) {
//                System.out.println("Start of Element '"+qName+"'");
//                int numOfAttr = attr.getLength();
//                System.out.println("   Number of Attributes is "+numOfAttr);
//                for (int i=0; i<numOfAttr; i++) {
//                   System.out.println("   with attr["+i+"]='"+attr.getValue(i)+"'");
//                }
                if (qName.equals("template")) {
                    protocolFamily = attr.getValue("apptab");
                    protocolName = attr.getValue("name");
                    protocolAppl = attr.getValue("application");
                    protocolAuthor = attr.getValue("author");
                }
                else if (qName.equals("protocol"))
                    protocolTitle = attr.getValue("title");
                if (protocolFamily==null) {
                    protocolFamily = new String("Other");
                }

            }
            public void warning(SAXParseException spe) {
                System.out.println("Warning: SAX Parse Exception:");
                System.out.println( new StringBuffer().append("    in ").
			append(spe.getSystemId()).
                        append(" line ").  append(spe.getLineNumber()).
                        append(" column ").append(spe.getColumnNumber()).
			toString());
            }

            public void error(SAXParseException spe) {
                System.out.println("Error: SAX Parse Exception:");
                System.out.println(new StringBuffer().append("    in ").
			append(spe.getSystemId()).
                        append(" line ").append(spe.getLineNumber()).
                        append(" column ").append(spe.getColumnNumber()).
			toString());
            }

            public void fatalError(SAXParseException spe) {
                System.out.println("FatalError: SAX Parse Exception:");
                System.out.println(new StringBuffer().append("    in ").
			append(spe.getSystemId()).
                        append(" line ").append(spe.getLineNumber()).
                        append(" column ").append(spe.getColumnNumber()).
			toString());
            }
        }
    }
    public SAXParser getSAXParser() {
        SAXParser parser = null;
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            spf.setValidating(false);  // set to true if we get DOCTYPE
            spf.setNamespaceAware(false); // set to true with referencing
            parser = spf.newSAXParser();
        } catch (ParserConfigurationException pce) {
            System.out.println(new StringBuffer().
                               append("The underlying parser does not support ").
                               append("the requested feature(s).").
                               toString());
        } catch (FactoryConfigurationError fce) {
            System.out.println("Error occurred obtaining SAX Parser Factory.");
        } catch (Exception e) {
            Messages.writeStackTrace(e);
        }
        return (parser);
    }

    // Get all of the local protocols and add them to the fromProtocolList list
    void getLocalProtocols() {
        String usrDir;
        SAXParser parser = getSAXParser();
        DefaultHandler protocolSAXHandler = new MyProtocolHandler();


        // Get the current user object
        String curuser = System.getProperty("user.name");
        LoginService loginService = LoginService.getDefault();
        Hashtable userHash = loginService.getuserHash();
        User user = (User)userHash.get(curuser);

        // Get this users templates/vnmrj/protocols directory
        usrDir = FileUtil.userDir(user, "PROTOCOLS");
        if(usrDir == null)
            return;

        File dir = new UNFile(usrDir);
        if(!dir.exists() && !dir.canRead())
            return;

        // Get the list of file in this directory
        File[] files = dir.listFiles();


        // Go through the protocols listed and create ProtocolEntry objects
        for(int i=0; i < files.length; i++) {
            File file = files[i];
            if(!file.getName().endsWith(".xml") || !file.canRead()) {
                // Silently skip the file
                continue;
            }

            // Parse the protocol and fill in a ProtocolEntry
            try {
                // Parse the file, which will leave its results in 'right'
                parser.parse(file, protocolSAXHandler);
            }
            catch (Exception e) {
                Messages.writeStackTrace(e);
            }

            String protocolLabel = ProtocolLabelHandler.getLabel(protocolName,
                                                                "protocol");
            ProtocolEntry pe=  new ProtocolEntry();
            pe.name = protocolName;
            pe.type = protocolType;
            pe.label = protocolLabel;

            // Disallow duplicates
            boolean addIt = true;
            for(int k=0; k < fromProtocolList.size(); k++) {
                ProtocolEntry pe2 = (ProtocolEntry)fromProtocolList.get(k);
                if(pe2.name.equals(protocolName))
                    addIt = false;
            }

            if(addIt)
                fromProtocolList.put(protocolName, pe);
        }
        return;
    }
    public class MyProtocolHandler extends DefaultHandler {

        public void endDocument() {
            //System.out.println("End of Document");
        }

        public void endElement(String uri, String localName, String qName) {
            //System.out.println("End of Element '"+qName+"'");
        }
        public void startDocument() {
            //System.out.println("Start of Document");
        }
        public void startElement(String uri,   String localName,
                                 String qName, Attributes attr) {
            if (qName.equals("template")) {
                protocolType = attr.getValue("apptype");
                protocolName = attr.getValue("name");
            }
        }
        public void warning(SAXParseException spe) {
            System.out.println("Warning: SAX Parse Exception:");
            System.out.println( new StringBuffer().append("    in ").
                                append(spe.getSystemId()).
                                append(" line ").  append(spe.getLineNumber()).
                                append(" column ").append(spe.getColumnNumber()).
                                toString());
        }

        public void error(SAXParseException spe) {
            System.out.println("Error: SAX Parse Exception:");
            System.out.println(new StringBuffer().append("    in ").
                               append(spe.getSystemId()).
                               append(" line ").append(spe.getLineNumber()).
                               append(" column ").append(spe.getColumnNumber()).
                               toString());
        }

        public void fatalError(SAXParseException spe) {
            System.out.println("FatalError: SAX Parse Exception:");
            System.out.println(new StringBuffer().append("    in ").
                               append(spe.getSystemId()).
                               append(" line ").append(spe.getLineNumber()).
                               append(" column ").append(spe.getColumnNumber()).
                               toString());
        }
    }

     String currentType;
     /* Used to parse protocol-lists file */
     public class ProtocolListHandler extends DefaultHandler {

         ProtocolEntry pe;

         public void endDocument() {
              //System.out.println("End of Document");
         }

         public void endElement(String uri, String localName, String qName) {
               //System.out.println("End of Element '"+qName+"'");
         }
         public void startDocument() {
               //System.out.println("Start of Document");
         }

         public void startElement(String uri, String localName, String qName,
                             Attributes attr) {
             // System.out.println("Start-of-Element: "+ qName);
             if (qName.compareTo("page")==0) {
                 //System.out.println("  Attr[name]: " + attr.getValue("name"));
                 currentType=attr.getValue("name");
             }
             else {
                 if (qName.compareTo("protocol")==0) {
               //System.out.println("  Attr[name]: " + attr.getValue("name"));
               //System.out.println("  Attr[label]: " + attr.getValue("label"));
                     pe = new ProtocolEntry();
                     pe.type=(currentType);
                     pe.name=(attr.getValue("name"));
                     pe.label=(attr.getValue("label"));
                     fromProtocolList.put(pe.name, pe);
                 }
                 else {
                     if (qName.compareTo("protocol-list")!=0)
                         System.out.println("Illegal Element in protocol list file");
                 }
             }
        }
        public void warning(SAXParseException spe) {
            System.out.println("Warning: SAX Parse Exception:");
            System.out.println( new StringBuffer().append("    in ").
			append(spe.getSystemId()).
                        append(" line ").  append(spe.getLineNumber()).
                        append(" column ").append(spe.getColumnNumber()).
			toString());
        }

        public void error(SAXParseException spe) {
            System.out.println("Error: SAX Parse Exception:");
            System.out.println(new StringBuffer().append("    in ").
			append(spe.getSystemId()).
                        append(" line ").append(spe.getLineNumber()).
                        append(" column ").append(spe.getColumnNumber()).
			toString());
        }

        public void fatalError(SAXParseException spe) {
            System.out.println("FatalError: SAX Parse Exception:");
            System.out.println(new StringBuffer().append("    in ").
			append(spe.getSystemId()).
                        append(" line ").append(spe.getLineNumber()).
                        append(" column ").append(spe.getColumnNumber()).
			toString());
        }
    }

    /* Used to parse profile file */
    /* Output is a ProtocolEntry item added to fromProtocolList */
    public class ProfileHandler extends DefaultHandler {

        ProtocolEntry pe;
        boolean oneErrorOutputComplete = false;

        public void endDocument() {
            //System.out.println("End of Document");
        }

        public void endElement(String uri, String localName, String qName) {
            //System.out.println("End of Element '"+qName+"'");
        }
        public void startDocument() {
            //System.out.println("Start of Document");
        }

        public void startElement(String uri, String localName, String qName,
                                 Attributes attr) {
            if(qName.equals("protocol-list"))
                return;

            // There will be tools and rights which we will just skip here
            else if (qName.compareTo("protocol")==0) {
                String testVersion = attr.getValue("apptypeLabel");
                if(testVersion != null) {
                    // Only output one message, else there could be a lot of them.
                    if(!oneErrorOutputComplete) {
                        Messages.postError("Old version of profile, "
                                 + " found.\n    To convert profiles to new version,"
                                 + " Use\n    \"vnmrj adm\"/Edit Profiles... to \"Open\" \n"
                                 + "    and then \"Save\" each profile.");
                        oneErrorOutputComplete = true;
                    }

                }
                pe = new ProtocolEntry();
                String name = attr.getValue("name");
                String apptype = attr.getValue("apptype");

                pe.type =  apptype;
                // Get the actual protocol name
                pe.name = name;
                // Get the label that is desired
                String label = ProtocolLabelHandler.getLabel(name, "protocol");
                pe.label = label;
                String approve = attr.getValue("approve");

                // If the user does not want this protocol, don't add it
                if(!userOffList.contains(pe.name) && approve.equals("true"))
                    fromProtocolList.put(pe.name, pe);
            }
        }
        public void warning(SAXParseException spe) {
            System.out.println("Warning: SAX Parse Exception:");
            System.out.println( new StringBuffer().append("    in ").
                                append(spe.getSystemId()).
                                append(" line ").  append(spe.getLineNumber()).
                                append(" column ").append(spe.getColumnNumber()).
                                toString());
        }

        public void error(SAXParseException spe) {
            System.out.println("Error: SAX Parse Exception:");
            System.out.println(new StringBuffer().append("    in ").
                               append(spe.getSystemId()).
                               append(" line ").append(spe.getLineNumber()).
                               append(" column ").append(spe.getColumnNumber()).
                               toString());
        }

        public void fatalError(SAXParseException spe) {
            System.out.println("FatalError: SAX Parse Exception:");
            System.out.println(new StringBuffer().append("    in ").
                               append(spe.getSystemId()).
                               append(" line ").append(spe.getLineNumber()).
                               append(" column ").append(spe.getColumnNumber()).
                               toString());
        }
    }


    public class ProtocolEntry {
        String type, name, label;

        public void clear() {
            type=null;
            name=null;
            label=null;
        }
    }
}

