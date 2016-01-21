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

import vnmr.templates.LayoutBuilder;


public class ProtocolLabelHandler extends DefaultHandler {
    static VLabelResource m_resource  = null;
    static private HashArrayList apptypeTransTable = new HashArrayList();
    static private HashArrayList protocolTransTable = new HashArrayList();
    static private ProtocolLabelHandler ath = 
                                             new ProtocolLabelHandler();

    public ProtocolLabelHandler() {
        UNFile file = null;

        String filename = FileUtil.openPath("INTERFACE/ProtocolLabels.xml");
        if(filename != null) {
	    m_resource = LayoutBuilder.getLabelResource(filename);
            file = new UNFile(filename);
	}
        if(file == null || !file.exists()) {
//             Messages.postWarning("The file " + filename 
//                     + " is needed to \n   translate apptypes "
//                     + "and protocols to english labels."
//                     + "  That file is not found,\n   so apptype and "
//                     + "protocol names will be used directly.");

            // When getLabel() is called, it will default to returning
            // apptype or protocol name if no translation exists in the table
            return;
        }
	
        SAXParser parser = getSAXParser();

        try {
            // Parse the file, which will leave the results in transTable
            // The results can then be retrieved as needed via getLabel()
            parser.parse(file, this);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
    }

    private String getLabelString(String str) {
/*
        if(m_resource == null) return str = Util.getLabelString(str);
        else return m_resource.getString(str);
*/
        return str = Util.getLabel(str);
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
        int numOfAttr = attr.getLength();
        if (qName.equals("apptype")) {
            String apptype = attr.getValue("name");
            String label = attr.getValue("label");
	    label = getLabelString(label);
            apptypeTransTable.put(apptype, label);
        }
        else if(qName.equals("protocol")) {
            String protocol = attr.getValue("name");
            String label = attr.getValue("label");
	    label = getLabelString(label);
            protocolTransTable.put(protocol, label);
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


    // Get a label for apptype or protocol.  Specify which to look for.
    static public String getLabel(String inputName, String type) {

        if(type == "apptype") {
            // If no match, just use apptype, unless it is an empty string,
            // then return "Misc"
            if(inputName == null || inputName.length() == 0)
                return "Misc";
            String label = (String) apptypeTransTable.get(inputName);
            if(label == null) {
                return inputName;
            }
            else
                return label;
        }
        else if(type == "protocol") {
            // If no match, just use inputName, unless it is an empty string
            if(inputName == null || inputName.length() == 0)
                return "Error";
            String label = (String) protocolTransTable.get(inputName);
            if(label == null) {
                return inputName;
            }
            else
                return label;
        }
        else
            return "Error";
    }
    
    // Utility method to sort an ArrayList of apptypeLabels
    // This is so that when the UserRightsPanel is created, the nodes
    // are in the order specified in the ProtocolLabels.xml file.
    // Otherwise, the order is based on files read from the protocol dirs
    static public ArrayList sortApptypeLabels(ArrayList apptypeLabels) {
        ArrayList sorted = new ArrayList();

        for(int i=0; i < apptypeTransTable.size(); i++) {
            String apptype = (String)apptypeTransTable.get(i);
            // Is this apptypeLabel in the arg list?  If so, add to the
            // new list at this point.
            if(apptypeLabels.contains(apptype) && !sorted.contains(apptype))
                sorted.add(apptype);

        }
        return sorted;
    }
    
    static public HashArrayList getProtocolTransTable() {
        return protocolTransTable;
    }
       
    
    static public HashArrayList getApptypeTransTable() {
        return apptypeTransTable;
    }
     
    static public void updateProtocolLabelHandler() {
        ath = new ProtocolLabelHandler();

    }
}
