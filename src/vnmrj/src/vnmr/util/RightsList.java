/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;
import java.util.*;

import javax.xml.parsers.*;
import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.*;
import java.awt.event.*;

import vnmr.admin.util.*;
import vnmr.ui.*;
import vnmr.ui.ExpSelEntry;
import vnmr.bo.*;


/********************************************************** <pre>
 * Summary: Read rights file and create a list of rights
 *
 * When the admin uses this, the protocols will likely come from different
 * place instead of from the system rightsList file.  When a user uses this,
 * the rights, tools and protocols will all be in the file.
 *
 * Example of input and output file format:
 *   name='Browser' keyword=browserok value=true type=tool
 *   name='Locator' keyword=locatorok value=false type=tool
 *   name='Study Queue' keyword=sqok value=true type=tool
 *   name='Holding Area' keyword=holdingok=true type=tool
 *   name='Can Edit Panels' keyword=caneditpanels value=false type=right
 *   name='Can Use Day Queue' keyword=canusedayqueue value=true type=right
 *   name='User Level' keyword=userlevel value=30 type=right
 *   name='Cosy' type=protocol acqtype=homo2d approve=true
 *
 *   Where keyword specifies the right
 *
 * The information in the rightsList can be obtained as follows:
 *  Set set = rightsList.entrySet();
 *  Iterator iter = set.iterator();
 *  while(iter.hasNext()) {
 *     Map.Entry entry = (Map.Entry)iter.next();
 *     String key = (String) entry.getKey();
 *     Right right = (Right) entry.getValue();
 *     Right right = (Right) rightsList.get(i);
 *     String name = right.getName();
 *     String keyword = right.getKeyword();
 *     String value = right.getValue();
 *     String type = right.getType();
 *     String apptype = right.getApptype();
 *     ....
 *     Do Something with the info
 * }
 *
 * When creating a new profile, we need to read a file containing the
 * possible tools and rights.  Then we need to get the list of protocols
 * from the system protocol directory.
 *
 * When opening an existing profile, it will contain the rights/tools and
 * the current 'value' and it will contain the protocols which are
 * approved for this profile.  We need to add ALL of the protocols from
 * the system area for the current appmode, and then we need to set
 * the protocol approved element to true for the items in the profile file
 * and leave the approved element false for other protocols.
 </pre> **********************************************************/


public class RightsList
{

    // Just one of these lists.  If it gets changed dynamically, anyone
    // using it will get the new one.  This contains (keyword, right)
    private HashArrayList rightsList;
    
    static private RightsExpHandler rightsExpHandler=null;


    // Constructor to take list of dir paths and create an ArrayList of Right's
    // based on the parsing of the protocols in those directories.
    // This constructor clears out the rightsList and starts over.
    // For the admin level panel.
    public RightsList(HashMap protocolDirPath) {


        rightsList = new HashArrayList();
        // Go through the list of directories and add the protocols
        // within each to the protocol list.
        Set set = protocolDirPath.entrySet();
        Iterator iter = set.iterator();
        while(iter.hasNext()) {
            Map.Entry entry = (Map.Entry)iter.next();
            String path = (String) entry.getValue();
            String label = (String) entry.getKey();

            // Add the protocols in this path including the dirLabel
            // The dirlabel will be used as a tree node to hold these items.
            parseProtocolDir(path, label);
        }

        // Now read the master rights list to get the rights and tools
        String rightsFileName=FileUtil.openPath("SYSTEM/USRS/rightsList.xml");

        if(rightsFileName == null) {
            Messages.postWarning("The file .../rights/rightsList.xml"
                             + " does not exist.\n    This file should "
                             + "contain the master list of rights and tools.");
            return;
        }

        openProfile(rightsFileName, true, true);
        if(rightsExpHandler == null)
            rightsExpHandler = new RightsExpHandler();
        ExpPanel.addExpListener(rightsExpHandler);

        // sort protocols into the order in ProtocolLabelHandler
//        sortProtocols();
    }

    // this is used by User.initAppDirs
    public RightsList(String operator, boolean all) {
        rightsList = new HashArrayList();

        // Now read the master rights list to get the rights and tools
        // and in particular, the adminonly flags.
        String rightsFileName=FileUtil.openPath("SYSTEM/USRS/rightsList.xml");
        if(rightsFileName != null) {
            openProfile(rightsFileName, true, true);
        }

        // Open this operators designated profile and add items to the
        // rightsList
        boolean success = operatorProfile(operator, all);
    }

    // Constructor for the operator level information.
    // First fill based on this operators designated profile, then use his
    // persistence file to turn some things off.
    // all = true means we need all protocols, even if unapproved.
    // all = false means include only approved protocols
    public RightsList(boolean all) {
        String operator = Util.getCurrOperatorName();
        rightsList = new HashArrayList();

        // Now read the master rights list to get the rights and tools
        // and in particular, the adminonly flags.
        String rightsFileName=FileUtil.openPath("SYSTEM/USRS/rightsList.xml");
        if(rightsFileName != null) {
            openProfile(rightsFileName, true, true);
        }

        // Open this operators designated profile and add items to the
        // rightsList
        boolean success = operatorProfile(operator, all);
        if(!success)
            return;

        // Now for an operator (as opposed to the admin), we need to add
        // the local protocols.
        String dirPath = FileUtil.usrdir();
        dirPath = dirPath + File.separator + "templates"
                  + File.separator + "vnmrj" + File.separator + "protocols";

        parseProtocolDir(dirPath, "Local");


        // Now open this operators persistence file and turn off items that
        // he has turned off.  Ignore the persistence 'true' items.  This
        // way, an operator cannot turn on items that are disallowed in
        // the profile.  He can just turn off things that he would normally
        // be allowed to see.
        operatorPersistence();
  
        // Keep a static copy of rightsExpHandler so that addExpListener
        // will only add one of them.  addExpListener check for dups.
        // This listener is to catch a change in operator and handle
        // rightsLists which are out of date and things that need to be updated
        if(rightsExpHandler == null)
            rightsExpHandler = new RightsExpHandler();
        ExpPanel.addExpListener(rightsExpHandler);
        
        // sort protocols into the order in ProtocolLabelHandler
//        sortProtocols();

    }

    // Open this operators persistence file and turn off items that
    // he has turned off.  Ignore the persistence 'true' items.  This
    // way, an operator cannot turn on items that are disallowed in
    // the profile.  He can just turn off things that he would normally
    // be allowed to see.  Since the operator is only allowed to modify
    // his protocol list, we can take this opportunity to remove non
    // protocol items from the rightsList all together.  Else, it screws
    // up the setSelectionPaths() command to have items which are not
    // actually in the tree.
    public void operatorPersistence() {
        BufferedReader  in;
        String inLine, keyword, approve;
        QuotedStringTokenizer tok;
        FileWriter fw2 = null;
        PrintWriter os2=null;

        // There is a persistence file for each operator
        String curOperator = Util.getCurrOperatorName();
        
        // Open the persistence file
        String filepath=FileUtil.openPath("USER/PERSISTENCE/RightsConfig_"
                    + curOperator + ".txt");

        if(DebugOutput.isSetFor("rightslist")) {
            Messages.postDebug("Opening " + filepath 
                               + " to modify rightsList as specified therein.");
        }
        // If no file was saved yet, create an empty one.  If no file
        // exists, funny results are displayed in the panel.  To tell the
        // truth, I don't know why, but having any file there, even empty,
        // causes it to work properly.
        if (filepath == null) {
            filepath=FileUtil.savePath("USER/PERSISTENCE/RightsConfig_"
                    + curOperator + ".txt");
            try {
                UNFile file2 = new UNFile(filepath);
                fw2 = new FileWriter(file2);
                os2 = new PrintWriter(fw2);

                // Write txt comment at top of that file
                os2.println("# This file is only used to further limit access to "
                                + "items which are already ");
                os2.println("# approved in the profile assigned to this operator.");

                os2.println("\n# Protocols");
                os2.close();
                fw2.close();
            }
            catch (Exception e) {
                os2.close();
                Messages.postError("Problem creating file, " + filepath);
                Messages.writeStackTrace(e);
            }
        }
            
        try {
            UNFile file = new UNFile(filepath);
            FileReader fr = new FileReader(file);
            if(fr == null) {
                return;
            }
            in = new BufferedReader(fr);
        }
        catch(Exception e) {
            return;
        }

        try {
            // Read one line at a time.
            while ((inLine = in.readLine()) != null) {
                /* skip blank lines and comment lines. */
                if (inLine.length() > 1 && !inLine.startsWith("#")) {
                    inLine.trim();

                    tok = new QuotedStringTokenizer(inLine);

                    keyword = tok.nextToken().trim();

                    // If this keyword is not in the list, just keep going
                    // if(!rightsList.containsKey(keyword))
                    //     continue;

                    if(!tok.hasMoreTokens()) {
                        Messages.postWarning("Syntax problem in " +
                                             filepath);
                        // Just skip this line
                        continue;
                    }

                    approve = tok.nextToken().trim();

                    if (!rightsList.containsKey(keyword)) {
                       Right right = new Right("protocol", keyword, approve, "unknown",
                                           "unknown",  "unknown");
                       // Add to the rightsList list
                       rightsList.put(right.keyword, right);
                    }

                    Right rt = (Right) rightsList.get(keyword);

                    // Only make a change if the files specifies false
                    // This is the actual object, so changing it will
                    // change its value in the list.
                    if(approve.equals("false")) {
                        rt.approve = approve;
                    }

                    if(DebugOutput.isSetFor("rightslist")) {
                        Messages.postDebug(" Modified: " +
                                           rt.toFullString());
                    }
                }
            }
            in.close();
        }
        catch(Exception e) {

        }

            
        // Since the operator is only allowed to modify
        // his protocol list, we can take this opportunity to remove non
        // protocol items from the rightsList all together.  Else, it screws
        // up the setSelectionPaths() command to have items which are not
        // actually in the tree.        
        
        // Start at the top so as not to screw up the numbering of items
        // in the list
        
// This definitely scews things up.  After this code, there are no rights
// left in rightsList, so things in Java that check for rights fail.
//        for(int i=rightsList.size() -1; i >= 0 ; i--) {
//            Right right = (Right)rightsList.get(i);
//            // If this is not a protocol, remove it
//            if(right.type != "protocol")
//                rightsList.remove(i);
//        }
    }

    public int size() {
        return rightsList.size();
    }

    public Right getRight(String keyword) {
        return (Right)rightsList.get(keyword);
    }

    public Right getRight(int index) {
        return (Right)rightsList.get(index);
    }


    // Get the list of all protocols in the given directory.
    // Parse each one of these and create a 'right' for each.
    // Add the rights to the rightsList.
    // Default the approve value to false. This way, when we possibily read
    // in a profile later, we can approve the items in the profile.
    public void parseProtocolDir(String dirPath, String dirLabel) {
        Right right = new Right();
        SAXParser parser = getSAXParser();
        // The parser will leave the results in the 'right' passed in here
        DefaultHandler protocolSAXHandler = new MyProtocolHandler(right);
        UNFile dir;
        File file;

        // Get the list of protocols in this directory
        dir = new UNFile(dirPath);

        if(!dir.exists() || !dir.isDirectory() || !dir.canRead()) {
            // Silent return
            return;
        }

        // Get a list of all files in the directory
        File[] fileList = dir.listFiles();

        // Loop through the list of files
        for(int i=0; i < fileList.length; i++) {
            file = fileList[i];

            if(!file.getName().endsWith(".xml") || !file.canRead()) {
                // Silently skip the file
                continue;
            }

            // Clear out the right object so it can be refilled
            right.clear();

            try {
                // Parse the file, which will leave its results in 'right'
                parser.parse(file, protocolSAXHandler);
            }
            catch (Exception e) {
                Messages.postDebug("Problem parsing " + file);
                Messages.writeStackTrace(e);
            }
            // Fill in a few more elements of 'right'
            right.dirLabel = dirLabel;
            right.fullpath = file.getAbsolutePath();
            // Translate to English label
            right.tabLabel = getTabFromProtocolList(right.name);

            // Get protocol label if it exists
            right.protocolLabel = getProtocolLabelFromProtocolList(right.name);

            // If filling from the Local directory, set approve to true
            // This allows operators to have access to their own protocols
            // For other directories, default to false.
            if(dirLabel.equals("Local"))
                right.approve = "true";
            else
                right.approve = "false";

            // Set the ToolTip to Name by Author at fullpath
            right.toolTip = right.name + " by " + right.author
                + " at " + right.fullpath;

            // For protocols, set keyword to name
            right.keyword = right.name;
            // Add it to the list.  The 'right' object is being used
            // as a scratch area by the parser to return the results.
            // Create a new copy of it to add to the rightsList.
            rightsList.put(right.keyword, right.copy());

            if(DebugOutput.isSetFor("rightslist"))
                Messages.postDebug(right.toFullString());

        }
    }


    // Write out a pair of files that can be read back in for the creation of
    // this class.  Ie., can be used for writing a persistence file.
    // The .xml file will have all of the information we need, thus in this
    // program, we will only be reading the .xml file.  We are writing out
    // a .txt file which only contains the rights for use by vnmrbg.
    public void writeFiles(String filepath) {

        // We are going to write out the rights in a .txt file and
        // the protocols to an .xml file.  The filename being passed
        // in should be the .xml file.  If not, add .xml
        if(!filepath.endsWith(".xml"))
            filepath = filepath.concat(".xml");

        // Now create the .txt filename
        String rightsFilename = filepath.replaceFirst(".xml", ".txt");


        FileWriter fw1;
        PrintWriter os1=null;
        FileWriter fw2;
        PrintWriter os2=null;
        try {
            UNFile file1 = new UNFile(filepath);
            fw1 = new FileWriter(file1);
            os1 = new PrintWriter(fw1);

            UNFile file2 = new UNFile(rightsFilename);
            fw2 = new FileWriter(file2);
            os2 = new PrintWriter(fw2);


            // Write xml id at top
            os1.println("<?xml  version=\"1.0\" ?>\n");
            os1.println("<rights-list>");

            // Write txt comment at top of that file
            os2.println("# Rights and Tools");

            // Go thru the HashMap of rights and write each line to a file
            // in the form of keyword approve (eg, locator true)
            Set set = rightsList.entrySet();
            Iterator iter = set.iterator();
            while(iter.hasNext()) {
                Map.Entry entry = (Map.Entry)iter.next();
                Right right = (Right) entry.getValue();
                if(!right.type.equals("protocol")) {
                    // Write out the right to the .txt file
                    String str = right.keywordTxtOutputLine();
                    os2.println(str);

                    // Write out the right to the .xml file
                    str = right.xmlOutputLine();
                    os1.println(str);
                }
            }
            // Now write out the protocols
            set = rightsList.entrySet();
            iter = set.iterator();
            while(iter.hasNext()) {
                Map.Entry entry = (Map.Entry)iter.next();
                Right right = (Right) entry.getValue();
                if(right.type.equals("protocol")) {
                    String str = right.xmlOutputLine();
                    os1.println(str);
                }
            }
            os1.println("</rights-list>");
            os1.close();
            os2.close();
        }
        catch(Exception e) {
            os1.close();
            os2.close();
            Messages.postError("Problem creating file, " +
                               filepath);
            Messages.writeStackTrace(e);
        }
    }

    // This will write a .txt file with protocols.
    // Rights and tools are not disallowed by the user, just the admin
    public void writePersistence() {
        FileWriter fw2 = null;
        PrintWriter os2=null;

        // Write out a persistence file for each operator
        String curOperator = Util.getCurrOperatorName();
        String filepath = FileUtil.savePath("USER/PERSISTENCE/RightsConfig_"
                    + curOperator + ".txt");

        try {
            UNFile file2 = new UNFile(filepath);
            fw2 = new FileWriter(file2);
            os2 = new PrintWriter(fw2);

            // Write txt comment at top of that file
            os2.println("# This file is only used to further limit access to "
                        + "items which are already ");
            os2.println("# approved in the profile assigned to this operator.");

            os2.println("\n# Protocols");
            Set set = rightsList.entrySet();
            Iterator iter = set.iterator();
            while(iter.hasNext()) {
                Map.Entry entry = (Map.Entry)iter.next();

                Right right = (Right) entry.getValue();
                if(right.type.equals("protocol")) {
                    String str = right.keywordTxtOutputLine();
                    os2.println(str);
                }
            }
            os2.close();
            fw2.close();
        }
        catch(Exception e) {
            os2.close();
            Messages.postError("Problem creating file, " +
                               filepath);
            Messages.writeStackTrace(e);
        }

    }

    static boolean firstCall = true;
    
    // Initialize list with the current operator's profile
    public boolean operatorProfile(String operator, boolean all) {
        String profile;
        // Check to see if this user has a profile set in the operator
        // login info
        // Get the Profile Name column string for access to the operator data
        String pnStr = vnmr.util.Util.getLabel("_admin_Profile_Name");
        profile = WUserUtil.getOperatordata(operator, pnStr);

        // If no profile exists, try to determine a default
        if(profile == null || profile.length() == 0) {
            User user = LoginService.getDefault().getUser(operator);
            if(user != null) {
                String itype = user.getIType();
                profile = "";
                if (itype.equals(Global.WALKUPIF)) {
                    profile = "AllLiquids.xml";
                }
                else if (itype.equals(Global.IMGIF)) {
                    profile = "AllImaging.xml";
                }

                if(profile == "") {
                    String persona = System.getProperty("persona");
                    // Skip output for admin tool
                    if(!persona.equalsIgnoreCase("adm")) {
                        Messages.postLog("No profile assigned for " + operator 
                                + " Either this user \n    is not an operator, or the operator "
                                + "     does not have a profile assigned.");
                    }
                    return false;
                }
                else {
                    String persona = System.getProperty("persona");
                    // Skip output for admin tool
                    if (persona !=null && !persona.equalsIgnoreCase("adm")) {
                        // Only output one Warning to the vnmrj error panel.
                        if (firstCall) {
                            Messages.postLog("Using default profile ("
                                 + profile  + ") for "
                                 + operator
                                 + ".  To assign\n    a different profile, use the Configure->"
                                 + "Operators->Edit operators... panel\n    in the vnmrj adm tool.");
                            firstCall = false;
                        }
                    }
                }
            }
        }

        // If profile does not end in .xml, add it
        if(profile == null || profile.length() == 0)
        {
            profile = "AllLiquids.xml";
            if (firstCall) {
               Messages.postLog("Using default profile ("
                  + profile  + ") for " + operator
                  + ".  To assign\n    a different profile, use the Configure->"
                  + "Operators->Edit operators... panel\n    in the vnmrj adm tool.");
               firstCall = false;
            }
        }
        else if(!profile.endsWith(".xml"))
            profile = profile + ".xml";

        // The area to get these profiles is /vnmr/adm/users/userProfiles
        String filepath = FileUtil.openPath("SYSTEM/USRS/userProfiles/" + profile);

        openProfile(filepath, all, false);

        return true;
    }

    // Open and parse a profile.  To call this to create a new list from
    // scratch, empty rightsList first.  To overlay the profile over the
    // master admin list, do that first, then call this.
    public void openProfile(String filename, boolean all, boolean masterList) {

        // The profiles are actually two files. The .xml file contains
        // everything.  The .txt file is only for use by other programs
        // like vnmrbg.  Thus, only use the .xml file here

        if(filename == null || filename.length() == 0)
            return;

        SAXParser parser = getSAXParser();
        DefaultHandler protocolSAXHandler = new MyProfileHandler(all, 
                                                                 masterList);
        UNFile file = new UNFile(filename);
        try {
            // Add the tools and rights from a profile
            parser.parse(file, protocolSAXHandler);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
    }

    public boolean containsKey(String keyword) {
        return rightsList.containsKey(keyword);
    }

    // Return false unless this 'keyword' is in the list and has 
    // approve = false. That is, if it does not exist, return false
    public boolean approveIsFalse(String keyword) {
        if(rightsList.containsKey(keyword)) {
            Right right = (Right) rightsList.get(keyword);

            String approve = right.getApprove();
            if(approve.equals("false"))
                return true;
            else
                return false;
        }
        else {
            return false;
        }
    }


    public boolean isApproved(String keyword) {
        if(rightsList.containsKey(keyword)) {
            Right right = (Right) rightsList.get(keyword);

            String approve = right.getApprove();
            if(approve.equals("false"))
                return false;
            else
                return true;
        }
        else {
            return true;
        }
    }   
    // Sort the protocols in the list to the same order as in the
    // ProtocolLabel.xml file. Put non protocols up front.
//     public void sortProtocols() {
//         HashArrayList sorted = new HashArrayList();
//         HashArrayList protocolTransTable = ProtocolLabelHandler
//                 .getProtocolTransTable();
//         // First put all of the non protocol stuff into the new output list
//         for (int i = 0; i < rightsList.size(); i++) {
//             Right right = (Right) rightsList.get(i);
//             if (!right.type.equals("protocol"))
//                 sorted.put(rightsList.getKey(i), right);
//         }
//         // Now put in the protocols that are in protocolTransTable in
//         // the order they are in there.
//         for (int i = 0; i < protocolTransTable.size(); i++) {
//             String protocol = (String) protocolTransTable.getKey(i);
//             // Is this protocol in the arg list? If so, add to the
//             // new list at this point.
//             if (rightsList.containsKey(protocol))
//                 sorted.put(protocol, rightsList.get(protocol));
//         }
//         // Now get protocols that were not gotten above.
//         // That means they were not in the ProtocolLabels.xml file
//         for (int i = 0; i < rightsList.size(); i++) {
//             Right right = (Right) rightsList.get(i);
//             if (right.type.equals("protocol")) {
//                 String key = (String)rightsList.getKey(i);
//                 if(!sorted.containsKey(key))
//                     sorted.put(key, right);
//             }
//         }
        
//         rightsList = sorted;
//     }
        

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

    // Save Static operator for comparison when the pnew for operator arrives
    static String operator=null;
    public class RightsExpHandler implements ExpListenerIF {
        // Method for ExpListenerIF
        public void updateValue() {
            // Should only arrive here during initialization
            // Save the current operator
            String curOperator = Util.getCurrOperatorName();
            operator= curOperator;
        }
        
        // Method for ExpListenerIF
        public void updateValue(Vector vec) {
            // Catch change in 'operator' so we can mark some rightsLists
            // out of date and update ExpCards.
            int size;

            size = vec.size();
            // Loop thru the attribute names
            for (int i = 0; i < size; i++) {
                String paramName = (String) vec.elementAt(i);
                if (paramName.equals("operator")) {
                    String curOperator = Util.getCurrOperatorName();
                    // Is the new current operator different from the saved one?
                    if(!curOperator.equals(operator)) {
                        // It is different, destroy some panels and null
                        // out rightsList
                        operator = curOperator;
                        ShufflerItem.setRightsListNull();
                        ExpPanel.resetBrowsers();
                        
                        // Update the Experiment Selector
                       /**********
                        ActionEvent aevt = new ActionEvent(this, 1, "force");
                        if(ExpSelector.updateCards != null)
                            ExpSelector.updateCards.actionPerformed(aevt);

                        //At least for now, there are two Exp Selectors
//                        if(ExpSelector.updateCards != null)
//                            ExpSelector.updateCards.actionPerformed(aevt);
                        
                        if(ExpSelTree.updateCards != null)
                            ExpSelTree.updateCards.actionPerformed(aevt);
                       **********/
                    }
                }
            }
        }
    }

    public class MyProtocolHandler extends DefaultHandler {
        Right right;

        // Save a Right object for putting the results into
        public MyProtocolHandler(Right right) {
            this.right = right;
        }

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
                right.name = attr.getValue("name");
                right.tabLabel = getTabFromProtocolList(right.name);
                right.author = attr.getValue("author");
            }
            else if (qName.equals("protocol")) {
                right.type = (qName);
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


    public class MyProfileHandler extends DefaultHandler {
        // Add all items or just approve = true items?
        boolean all, masterList;

        // Save a Right object for putting the results into
        public MyProfileHandler(boolean all, boolean masterList) {
            this.all = all;
            this.masterList = masterList;
        }
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

            if(qName.equals("rights-list"))
               return;
            else if (qName.equals("protocol")) {
                String pname = attr.getValue("name");
                String name = Util.getLabel(pname);
                String keyword = name;
                if(rightsList.containsKey(keyword)) {
                    Right rt = (Right) rightsList.get(keyword);
                    // This is the actual object, so changing it will
                    // change its value in the list.
                    rt.approve = attr.getValue("approve");

                    if(DebugOutput.isSetFor("UserRightsPanel"))
                        Messages.postDebug(" Modified: " + rt.toFullString());
                }
                else {
                    // It does not exists yet, create a right.
                    // If all = false, only add the item if approve = true
                    String approve = attr.getValue("approve");
                    if(!all && approve.equals("false"))
                       return;

                    String dirLabel = attr.getValue("dirLabel");
                    String tabLabel = getTabFromProtocolList(pname);
                    String protocolLabel = getProtocolLabelFromProtocolList(name);
 
                    Right right = new Right(qName, name, approve, dirLabel,
                                           tabLabel,  protocolLabel);

                    // Add to the rightsList list
                    rightsList.put(right.keyword, right);

                    if(DebugOutput.isSetFor("UserRightsPanel"))
                        Messages.postDebug(" Added: " + right.toFullString());
                }
            }
            else {
                // type = qName.  Create a right and fill it
                // If all = false, only add the item if approve = true
                String name = attr.getValue("name");
                name = Util.getLabel(name);
                String approve = attr.getValue("approve");
                String keyword =  attr.getValue("keyword");
                String adminonly =  attr.getValue("adminonly");

                if(adminonly == null)
                    adminonly = "false";

                // If the right already exists, check adminonly for the
                // existing one.  If adminonly = true and all = false
                // don't add this item.
                if(rightsList.containsKey(keyword)) {
                    Right curRight = (Right) rightsList.get(keyword);
                    adminonly = curRight.getAdminOnly();

                    // If all is false, and adminonly is true, then
                    // remove this right from the list.  When the operator
                    // profile is being read, all will be false.  We don't
                    // want operators to have items that have adminonly = true
                    // in the rightsList.xml file.
                    if(adminonly.equals("true") && !all) {
                        rightsList.remove(keyword);
                        return;
                    }
                    
                    // The right already exists, is the approve value different?
                    if(!approve.equals(curRight.getApprove()))
                        // Set the new approve value
                        curRight.setApprove(approve);
                }
                // If this item does not already exist, and we are not
                // filling from the masterList, then don't add any new rights.
                else if(!masterList) {
                    return;
                }

                // 'all' = true means add all items.  false means don't
                // add items which are true.  That is so that the user cannot
                // add items to be true which were not already there.
                if(!all && approve.equals("false"))
                    return;

                // We need to add a new item
                String nodeName;
                if(qName.equalsIgnoreCase("right") || qName.equalsIgnoreCase("tool"))
                   nodeName = qName+"s";
                else nodeName = qName;
                Right right = new Right(Util.getLabel(nodeName), name, approve, keyword,
                                        adminonly);

                // Add to the rightsList list
                rightsList.put(right.keyword, right);

                if(DebugOutput.isSetFor("UserRightsPanel"))
                    Messages.postDebug(" Added: " + right.toFullString());
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
            Messages.postError(new StringBuffer().append("SAX Parse Exception: Aborting ").
                            append("reading profile, repair or remove this ").
                            append(spe.getSystemId()).
                            append(" line ").append(spe.getLineNumber()).
                            append(" column ").append(spe.getColumnNumber()).
                            toString());
        }

        public void fatalError(SAXParseException spe) {
            Messages.postError(new StringBuffer().append("SAX Parse Exception: Aborting ").
                            append("reading profile, repair or remove this ").
                            append(spe.getSystemId()).
                            append(" line ").append(spe.getLineNumber()).
                            append(" column ").append(spe.getColumnNumber()).
                            toString());
        }
    }
    
    
    // Return the first "tab" entry for a protocol of the input name.
    private String getTabFromProtocolList(String protocolName) {

        ArrayList<ExpSelEntry> protocolList = ExpSelector.getProtocolList();

        // Go through the list of ExpSelEntries in protocolList looking for the
        // first instance of protocolName.  Return the tab for that entry.
        for (int i = 0; i < protocolList.size(); i++) {
            ExpSelEntry protocol = protocolList.get(i);
            if(protocol.getName().equals(protocolName)) {
                String tab = protocol.getTab();
                return tab;
            }
        }
        return new String("Misc");
    }

    // Return the first "label" entry for a protocol of the input name.
    private String getProtocolLabelFromProtocolList(String protocolName) {

        ArrayList<ExpSelEntry> protocolList = ExpSelector.getProtocolList();

        // Go through the list of ExpSelEntries in protocolList looking for the
        // first instance of protocolName.  Return the label for that entry.
        for (int i = 0; i < protocolList.size(); i++) {
            ExpSelEntry protocol = protocolList.get(i);
            if(protocol.getName().equals(protocolName)) {
                String label = protocol.getLabel();
                // Return the label from the ExperimentSelector.xml file
                return label;
            }
        }
        // If not found, just return the protocol name
        return protocolName;
    }

}
