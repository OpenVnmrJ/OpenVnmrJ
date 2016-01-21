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

/********************************************************** <pre>
 * Summary: Definition for one Right, one Tool or one Protocol access control
 *
 * For type = rights and tools, it will have the type, displayname,
 * toolTip, keyword and approve filled in.
 * 
 * For type = protocol, it will have the values type, displayname,
 * toolTip, apptype, apptypeLabel, author and fullpath filled in.
 * 
 * toString() is used by the tree to get its label, thus it
 * outputs only the displayname.
 * 
 * fileOutputLine() is used to write the accessprofile file
 *
 </pre> **********************************************************/

public class Right
{
    public String type="";
    public String name="";
    public String toolTip="";
    public String keyword="";        // For non protocols
    public String approve="";        
    public String adminonly="";
    public String tabLabel="";       // For protocols
    public String author="";         // For protocols
    public String fullpath="";       // For protocols
    public String dirLabel="";       // For protocols
    public String protocolLabel="";  // For protocols

    public Right() {

    }

    // Make an  object given a some information
    public Right(String type, String name, String approve, String dirLabel,
                 String tabLabel, String protocolLabel) {
                    this.type = type;
                    this.name = name;
                    // for protocols, keyword is the protocol name
                    this.keyword = name;
                    this.approve = approve;
                    this.dirLabel = dirLabel;
                    this.tabLabel = tabLabel;
                    this.protocolLabel = protocolLabel;                  
    }

    // Make an  object given a some information
    public Right(String type, String name, String approve, String keyword,
                                                           String adminonly) {
                    this.type = type;
                    this.name = name;
                    this.approve = approve;
                    this.keyword = keyword;
                    this.adminonly = adminonly;

    }


    // Given a single line of the appropriate format which is also the
    // format created by toString().
    // eg., name='Study Queue' keyword=sqok approve=true type=tool
    public Right(String line) {
        // Tokenize the line and keep the '=' chars.  Use StreamTokenizer
        // so that we can keep quoted strings intack.
        StringReader sr = new  StringReader(line);
        StreamTokenizer tok = new StreamTokenizer(sr);
        // Allow '_' as a character
        tok.wordChars('_', '_');
        // tokenize at '=' and dump it.
        tok.whitespaceChars('=','=');

        // tok.parseNumbers() is called by its constructor
        // and thus is always on, and tok.parseNumbers() does not allow
        // you to turn it off.
        // To turn off the numeric thing by hand
        tok.ordinaryChars('+', '.'); // + comma - and period  (Turn off numeric)
        tok.wordChars('+', '.'); // + comma - and period (Set as normal letters)
        tok.ordinaryChars('0', '9'); // all numbers   (Turn off numeric)
        tok.wordChars('0', '9');     // all numbers   (Set as normal letters)

        try {
            while(tok.nextToken() != StreamTokenizer.TT_EOF) {
                if(tok.sval.equals("type")) {
                    tok.nextToken();
                    type = new String(tok.sval);
                }
                else if(tok.sval.equals("name")) {
                    tok.nextToken();
                    name = new String(tok.sval);
                }
                else if(tok.sval.equals("keyword")) {
                    tok.nextToken();
                    keyword = new String(tok.sval);
                }
                else if(tok.sval.equals("approve")) {
                    tok.nextToken();
                    approve = new String(tok.sval);
                }
                else if(tok.sval.equals("tabLabel")) {
                    tok.nextToken();
                    tabLabel = new String(tok.sval);
                }
                else if(tok.sval.equals("author")) {
                    tok.nextToken();
                    author = new String(tok.sval);
                }
                else if(tok.sval.equals("fullpath")) {
                    tok.nextToken();
                    fullpath = new String(tok.sval);
                }
            }
                
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
        
    }

    public String getName() {
        return name;
    }

    public String getKeyword() {
        return keyword;
    }

    public String getApprove() {
        return approve;
    }

    public String getAdminOnly() {
        return adminonly;
    }

    public void setApprove(String newValue) {
        approve = newValue;
    }

    public String getType() {
        return type;
    }

    public String getToolTip() {
        return toolTip;
    }

    public String getTabLabel() {
        return tabLabel;
    }

    public String getAuthor() {
        return author;
    }

    public String getFullpath() {
        return fullpath;
    }

    public String getDirLabel() {
        return dirLabel;
    }

    public String getProtocolLabel() {
        return protocolLabel;
    }

    // This is used for the label for the JTree, so just display the
    // desired label.
    public String toString() {
        if(type.equals("protocol"))
            return protocolLabel;
        else
            return name;
    }

    public String xmlOutputLine() {
        String output;

        // Output as an xml line such as
        //     <protocol name="Noesy" approve="true" />
        if(type.equals("protocol")) {
            // Protocols
            output = "   <" + type + " name=\"" + name
                     + "\" dirLabel=\"" + dirLabel
                     + "\" tablabel=\"" + tabLabel
                     + "\" approve=\"" + approve + "\" />";

        }
        else {
            // Rights and Tools output as an xml line such as
            //    <tool name="Browser" keyword="browserok" approve="true" />
            output = "   <" + type + " name=\"" + name 
                             + "\" keyword=\"" + keyword 
                             + "\" approve=\"" + approve  + "\" />"; 
        }

        return output;
    }

    // Since toString needs to be just the name, this one can be used
    // during debugging and such for a full output.
    public String toFullString() {
        String output;

        output = "type=" + type + " name=" + name
                 + " protocolLabel=" + protocolLabel
                 + " tooltip="  + toolTip
                 + " keyword=" + keyword + " approve=" + approve
                 + " tabLabel=" + tabLabel
                 + " author=" + author + " fullpath=" + fullpath
                 + " dirLabel=" + dirLabel + " adminonly=" + adminonly;

        return output;
    }

    // String output for rights txt file in the form of keyword approve
    // eg., 'locator true'
    public String keywordTxtOutputLine() {
        String output;

        output = keyword  + " " + approve;
        return output;
    }

    // Clear out all elements so the object can be reused.
    public void clear() {
        type="";
        name="";
        toolTip="";
        keyword="";   
        adminonly="";
        approve="";        
        tabLabel=""; 
        author="";         
        fullpath="";
        dirLabel="";
        protocolLabel="";
    }

    // Return a new Right with the contents of the current one
    public Right copy() {

        Right right = new Right();

        right.type=type;
        right.name=name;
        right.toolTip=toolTip;
        right.keyword=keyword;  
        right.adminonly=adminonly;
        right.approve=approve;        
        right.tabLabel=tabLabel; 
        right.author=author;         
        right.fullpath=fullpath;
        right.dirLabel=dirLabel;
        right.protocolLabel=protocolLabel;

        return right;

    }

}
