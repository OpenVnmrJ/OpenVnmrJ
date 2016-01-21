/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**
 *
 * Title:       AcqPanelTransformer.java
 * Description: AcqPanelTransformer.java handles the actual editing of xml file (acq.xml)
 *              to confirm that the page tag (SpinCAD PS) exists or inserts it if
 *              necessary
 */

package vnmr.util;

import java.io.*;
import java.util.*;

import org.xml.sax.*;
import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.helpers.AttributesImpl;

import javax.xml.parsers.SAXParserFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;

public class AcqPanelTransformer extends DefaultHandler
{

    private static Writer  out;
    private boolean groupTagFound = false;
    private boolean groupTagExist = true;
    private String  groupTagName;
    private String  groupTagType;
    private AttributesImpl groupAtts ;


    /** insert group tag name if it does not exist in xml file (acq.xml)
     *
     * @param fName xml file name
     * @param gType group type to be checked or inserted
     * @param gName group name or label to be checked or inserted
     * @param gAtts group attributes
     */
    public void insertGroup(String fName, String gType, String gName, AttributesImpl gAtts) {

        // set group name and attributes
        groupTagType  = gType;
        groupTagName  = gName;
        groupTagFound = false;
        groupAtts     = gAtts;

        // Use an instance of this class as the SAX event handler
        DefaultHandler handler = this;

        // Use the default (non-validating) parser
        SAXParserFactory factory = SAXParserFactory.newInstance();
        try {
            // set up the input stream
            File finp = new File(fName);
            if ( ! finp.canWrite() ) {
              String errStr = "Panels not made since source panel file "+fName+" did not exist or file write permission error";
              Messages.postError(errStr);
              Util.getAppIF().appendMessage(errStr+"\n");
              return;
            }

            // Set up output stream
            File fout            = new File(fName+".xfm");
            OutputStream foutstr = new FileOutputStream(fout);
            OutputStream bout    = new BufferedOutputStream(foutstr);
            out                  = new OutputStreamWriter(bout,"UTF-8");

            // Parse the input
            SAXParser saxParser = factory.newSAXParser();
            saxParser.parse(finp, handler);

            if (groupTagExist) {

              finp.setLastModified( (new Date()).getTime() );
              fout.delete();

            } else {

              finp.renameTo(new File(fName+".bak"));
              fout.renameTo(new File(fName));

            }

        } catch (Exception e) {
            String errStr = "error in manipulating acq.xml file for inclusion of SpinCAD PS page";
            Messages.writeStackTrace(e,errStr);
            // e.printStackTrace();
        }
   }


    //===========================================================
    // SAX DocumentHandler methods
    //===========================================================

    public void startDocument()
    throws SAXException
    {
        emit("<?xml version='1.0' encoding='UTF-8'?>");
        nl();
    }

    public void endDocument()
    throws SAXException
    {
        try {
            nl();
            out.flush();
            out.close();
        } catch (IOException e) {
            String errStr = "I/O error in manipulating acq.xml file for inclusion of SpinCAD PS page";
            Messages.postError(errStr);
            Messages.writeStackTrace(e,errStr);
            throw new SAXException("I/O error", e);
        }
    }

    public void startElement(String namespaceURI,
                             String sName, // simple name
                             String qName, // qualified name
                             Attributes attrs)
    throws SAXException
    {
        String eName = sName; // element name
        if ("".equals(eName)) eName = qName; // not namespaceAware

        boolean isAGroup = false;
        if (eName.equals("group")) isAGroup=true;

        emit("<"+eName);
        if (attrs != null) {
            for (int i = 0; i < attrs.getLength(); i++) {
                String aName = attrs.getLocalName(i); // Attr name
                if ("".equals(aName)) aName = attrs.getQName(i);
                emit(" ");

                // filter for special XML characters
                StringBuffer value    = new StringBuffer(attrs.getValue(i));
                StringBuffer valueMod = new StringBuffer();
                for (int j=0; j<value.length(); j++) {

                  char ch = value.charAt(j);
                  switch (ch) {

                    case '<' :
                       valueMod.append("&lt;");
                       break;
                    case '>' :
                       valueMod.append("&gt;");
                       break;
                    case '&' :
                       valueMod.append("&amp;");
                       break;
                    case '"' :
                       valueMod.append("&quot;");
                       break;
                    default :
                       valueMod.append(ch);
                       break;
                  }
                }

                emit(aName+"=\""+valueMod+"\"");
                if ( isAGroup && !groupTagFound && (aName.equals("reference")) ) {
                   if ((value.toString()).equals(groupTagName)) {
                      groupTagFound = true;
                      Messages.postInfo(groupTagName+" group tag found in acq.xml file");
                   }
                }

            }
        }
        emit(">");
    }


    public void endElement(String namespaceURI,
                           String sName,     // simple name
                           String qName      // qualified name
                          )
    throws SAXException
    {
        String eName = sName;                // element name
        if ("".equals(eName)) eName = qName; // not namespaceAware

        if ( !groupTagFound && eName.equals("template") ) {
           Messages.postInfo("adding new group tag in acq.xml");
           groupTagExist = false;
           startGroup("", groupTagType, groupTagType, groupAtts);
           nl();
           endGroup("", groupTagType, groupTagType);
           nl();
        }

        emit("</"+eName+">");
    }

    public void characters(char buf[], int offset, int len)
    throws SAXException
    {
        String s = new String(buf, offset, len);
        emit(s);
    }


    public void startGroup(String namespaceURI,
                             String sName, // simple name
                             String qName, // qualified name
                             Attributes attrs)
    throws SAXException
    {
        String eName = sName; // element name
        if ("".equals(eName)) eName = qName; // not namespaceAware

        boolean isAGroup = false;
        if (eName.equals("group")) isAGroup=true;

        emit("  <"+eName);
        if (attrs != null) {
            for (int i = 0; i < attrs.getLength(); i++) {
                String aName = attrs.getLocalName(i); // Attr name
                if ("".equals(aName)) aName = attrs.getQName(i);
                emit(" ");

                // filter for special XML characters
                StringBuffer value    = new StringBuffer(attrs.getValue(i));
                StringBuffer valueMod = new StringBuffer();
                for (int j=0; j<value.length(); j++) {

                  char ch = value.charAt(j);
                  switch (ch) {

                    case '<' :
                       valueMod.append("&lt;");
                       break;
                    case '>' :
                       valueMod.append("&gt;");
                       break;
                    case '&' :
                       valueMod.append("&amp;");
                       break;
                    case '"' :
                       valueMod.append("&quot;");
                       break;
                    default :
                       valueMod.append(ch);
                       break;
                  }
                }

                emit(aName+"=\""+valueMod+"\"");

            }
        }
        emit(">");
    }


    public void endGroup(String namespaceURI,
                           String sName,     // simple name
                           String qName      // qualified name
                          )
    throws SAXException
    {
        String eName = sName;                // element name
        if ("".equals(eName)) eName = qName; // not namespaceAware

        emit("  </"+eName+">");
    }



    //===========================================================
    // Utility Methods ...
    //===========================================================

    // Wrap I/O exceptions in SAX exceptions, to
    // suit handler signature requirements
    private void emit(String s)
    throws SAXException
    {
        try {
            out.write(s);
            out.flush();
        } catch (IOException e) {
	    String errStr = "I/O Error in writing panel XML files";
            Messages.writeStackTrace(e,errStr);
            throw new SAXException("I/O error", e);
        }
    }

    // Start a new line
    private void nl()
    throws SAXException
    {
        String lineEnd =  System.getProperty("line.separator");
        try {
            out.write(lineEnd);
        } catch (IOException e) {
	    String errStr = "I/O Error in copying panel XML files";
            Messages.writeStackTrace(e,errStr);
            throw new SAXException("I/O error", e);
        }
    }

}   // AcqPanelTransformer class
