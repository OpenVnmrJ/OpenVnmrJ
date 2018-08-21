/*
 * XmlChecker.java
 * Created on March 10, 2007, 9:02 AM
 */


import java.io.*;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import org.xml.sax.*;
import org.xml.sax.helpers.DefaultHandler;


/**
 *
 * @author frits
 */
public class XmlChecker {
    
    BufferedWriter xmlResults;
    SAXParser parser;
    StringBuffer msg;
    
    
    /** Creates a new instance of XmlChecker */
    public XmlChecker( BufferedWriter bfw) {
        xmlResults = bfw;
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            parser = spf.newSAXParser();
        }
        catch (ParserConfigurationException spe ) {
            msg = new StringBuffer("Problems creating  SAXParserFactory or parser");
            try {
                xmlResults.write(msg.toString(), 0, msg.length());
                xmlResults.newLine();
            }
            catch(IOException ioe) {
            }
        }
        catch (SAXException se) {
            
        }
    }
             
    public void xmlParser(File f) {
        try {
            parser.parse(f, new MyDefaultHandler() );
        }
        catch (SAXException se) {
            
        }
        catch (IOException ioe) {
            
        }
    }
    
    public class MyDefaultHandler extends DefaultHandler {
        public void startDocument() {
//            System.out.println("New Document");
        }
        public void endDocument() {
//            System.out.println("End Document");
        }
        public void startElement(String uri, String localName, String qName, Attributes attrs) {
            
        }
        public void endElement(String uri, String localName, String qName) {
            
        }
        public void error (SAXParseException spe) {
            msg = new StringBuffer("   Error in line "+spe.getLineNumber()+" at "+spe.getColumnNumber());
            try {
                xmlResults.write(msg.toString(),0,msg.length());
                xmlResults.newLine();
            }
            catch (IOException ioe) {
                
            }
        }
        public void fatalError (SAXParseException spe) {
            msg = new StringBuffer("   Fatal Error in line "+spe.getLineNumber()+" at "+spe.getColumnNumber());
            try {
                xmlResults.write(msg.toString(),0,msg.length());
                xmlResults.newLine();
            }
            catch (IOException ioe) {
                
            }
        }
        public void waring (SAXParseException spe) {
            msg = new StringBuffer("   Waring in line "+spe.getLineNumber()+" at "+spe.getColumnNumber());
            try {
                xmlResults.write(msg.toString(),0,msg.length());
                xmlResults.newLine();
            }
            catch (IOException ioe) {
                
            }
        }
    }
}
