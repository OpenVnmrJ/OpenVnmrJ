/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.text.ParsePosition;
import java.util.*;
import javax.swing.JOptionPane;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.DefaultHandler;

import vnmr.util.Messages;
import vnmr.util.UNFile;

/*************************************************************** <pre>
 * Utilities for the conversion of xml log files to CSV files.
 * This utility is used in vnmrj for doing the conversion based on
 * items selected in a panel.  It also has a main() at the end so that
 * it can be called from a script.  People wanted to be able to call
 * the conversion from a cron job.
 * 
 * It allows for limiting the conversion to 
 *     specific date ranges
 *     specific parameters
 *     acquisition/login/both entries
 *
 ** The following is all related to the command line command usage of this utility **
 * 
 * xmlToCsv.sh is a script that runs vnmrj starting up in the main() in this
 * file.  The arguments are passed into main() from that script.
 * 
 * Usage: xmlToCsv inputLogFilePath outputCsvPath 
 *         [-type acq/login/all] [-paramlist p1 p2 p3 p4 ...]
 *         [-startdate "Month Date Year"] [-enddate "Month Date Year"]
 *         [-debug]
 *
 *  -type can have values of "acq", "login", or "all"
 *  -paramlist should be followed by a space separate list of parameters
 *             that are desired in the csv file
 *  -startdate specifies the beginning of the date range to be included
 *             in the output file.  The dates must be in the form
 *                 "Month Date Year" 
 *             and surrounded by double quotes
 *  -enddate specifies the end of the date range to be included
 *             in the output file.  The dates must be in the form
 *                 "Month Date Year" 
 *             and surrounded by double quotes
 *  -debug Causes the command to output all of the values it will be operating with.
 *         Basically a verbose output to the command line.
 *
 * Only the input and output file paths are mandatory.  If other options
 * are not specified, then "all" will be assumed for those.
 *
 * Example command line (Note: the actual command is a single line):
 *   xmlToCsv /vnmr/adm/accounting/acctLog.xml /home/vnmr1/testout2.csv  
 *   -type login -paramlist type start end operator owner
  *  -startdate "Jan 01 2014" -enddate "Feb 1 2014"
 *
 </pre> **********************************************************/

public class XmlToCsvUtil {
    // The following need to be filled before calling createCSVFile
    public ArrayList<String> outputParamList = new ArrayList<String>();
    public Date startDateRange = new Date();
    public Date endDateRange = new Date();
    public boolean acquisitionSelected;
    public boolean loginSelected;
    public String outputFilePath;
    public String logFilepath;
    private static boolean bNoUI = false;

    // Locally used params
    private String  logType="gorecord";
    // Fill locally in the Sax parser
    // Each entry in the log file is one entry in the ArrayList.
    // Each entry is save here as a Hashtable of keyword:value
    private ArrayList<Hashtable<String, String>> logEntryList = new ArrayList<Hashtable<String, String>>();
    // Filled in Sax parser.  Only used if outputParamList is null
    private ArrayList<String> possibleParamList = new ArrayList<String>();
    
    public XmlToCsvUtil()  {
    }

    public void convertXmlToCsv() {
        boolean success, goodEntry;
        boolean bNoTimeZone;
        String param;
        String value;
        SimpleDateFormat sdf;
        SimpleDateFormat sdfNoZone;
        FileWriter fWriter;
        BufferedWriter bufWriter=null;
        String date;
        Date entryStartDate;

        sdf = new SimpleDateFormat("EEE MMM d HH:mm:ss zzz yyyy");
        
        success = parseLogFile();
        if(!success) {
            return;
        }
                
        sdfNoZone = new SimpleDateFormat("EEE MMM d HH:mm:ss yyyy");

        if(acquisitionSelected)
            logType = "gorecord";
        if (loginSelected)
            // Just "log" so it can match "login" and "logout"
            logType = "log";
        // If both or neither selected, set to "all"
        if((acquisitionSelected && loginSelected) ||
                (!acquisitionSelected && !loginSelected)) {
            logType = "all";
        }
        
        // Be sure the output file ends in .csv
        if(!outputFilePath.endsWith(".csv") && !outputFilePath.endsWith(".CSV")) {
            outputFilePath = outputFilePath.concat(".csv");
        }
        
        File file = new UNFile(outputFilePath);
        File aFile = new File(file.getAbsolutePath());
        File parent = aFile.getParentFile();
        if(!parent.canWrite()) {
            Messages.postError("Cannot write output file: " + outputFilePath);
            outputFilePath = null;
        }
        if (outputFilePath == null) {
            if (!bNoUI) {
               JOptionPane.showMessageDialog(null,
                 "Cannot write output file: "+outputFilePath,
                 "XML to CSV",
                 JOptionPane.WARNING_MESSAGE);
            }
            return;
        }
        
        // If outputParamList was set to null, it means output all of the params
        // encountered in the log file.  Those were saved while parsing the log file
        // in possibleParamList.  Use possibleParamList for the outputParamList
        if(outputParamList == null) {
            outputParamList = possibleParamList;
            // The order will be the order keywords were found in the Sax parser.
            // Reorder so that end and endsec in the list beside start and startsec
            int indexStart = outputParamList.indexOf("start");
            if(indexStart > -1) {
                int indexEnd = outputParamList.indexOf("end");
                if(indexEnd > -1) {
                    // Remove end from its original location in the list
                    outputParamList.remove(indexEnd);
                    // Add it back in after "start"
                    outputParamList.add(indexStart +1, "end");
                }
            }
            int indexStartsec = outputParamList.indexOf("startsec");
            if(indexStartsec > -1) {
                int indexEndsec = outputParamList.indexOf("endsec");
                if(indexEndsec > -1) {
                    // Remove endsec from its original location in the list
                    outputParamList.remove(indexEndsec);
                    // Add it back in after "startsec"
                    outputParamList.add(indexStartsec +1, "endsec");
                }
            }                
        }

        if (outputParamList.size() < 1) {
            if (!bNoUI) {
               JOptionPane.showMessageDialog(null,
                 "No parameters were selected.",
                 "XML to CSV",
                 JOptionPane.WARNING_MESSAGE);
            }
            return;
        }
        bNoTimeZone = false;

        // Now we have all of the information we need to proceed with
        // creating a CSV file.
        try {
            fWriter = new FileWriter(outputFilePath);
            bufWriter = new BufferedWriter(fWriter);
            // Write the first line as a comma separated list of param names
            // from selectedParamList
            for(int i=0; i < outputParamList.size(); i++) {
                param = outputParamList.get(i);
                if(i != outputParamList.size() -1)
                    bufWriter.write(param + ",");
                else
                    bufWriter.write(param + "\n");

            }

            // Loop through logEntryList taking only items in outputParamList
            // and write to file/stream.  Skip if type is not as selected.
            for(int i=0; i < logEntryList.size(); i++) {
                Hashtable<String, String> entry = logEntryList.get(i);
                String type = entry.get("type");
                // If type is not the one selected, then skip this entry.
                // Catch "login" and "logout" as startswith "log"
                if(logType.equals("all") || type.startsWith(logType)){
                    if(type.equals("logout")) 
                        date = entry.get("end");
                    else
                        date = entry.get("start");
                    success = true;
                    goodEntry = true;
                    // Skip all entries not within the selected date range.
                    if (bNoTimeZone)
                        entryStartDate = sdfNoZone.parse(date, new ParsePosition(0));
                    else
                        entryStartDate = sdf.parse(date, new ParsePosition(0));
                    // If endDateRange and/or startDateRange are null, in means
                    // use all dates for that range
                    try {
                        if (endDateRange != null) {
                             if (!entryStartDate.before(endDateRange))
                                 goodEntry = false;
                        }
                        if (startDateRange != null) {
                             if (!entryStartDate.after(startDateRange))
                                 goodEntry = false;
                        }
                    }
                    catch(Exception ex) {
                        success = false;
                    }
                    if(!success) {
                        bNoTimeZone = !bNoTimeZone;
                        if (bNoTimeZone)
                            entryStartDate = sdfNoZone.parse(date, new ParsePosition(0));
                        else
                            entryStartDate = sdf.parse(date, new ParsePosition(0));
                        try {
                            if (endDateRange != null) {
                                 if (!entryStartDate.before(endDateRange))
                                     goodEntry = false;
                            }
                            if (startDateRange != null) {
                                 if (!entryStartDate.after(startDateRange))
                                     goodEntry = false;
                            }
                        }
                        catch(Exception ex) {
                            goodEntry = false;
                        }
                    }
                    if (goodEntry) {
                        // It matches the range, write the line for this entry
                        for(int k=0; k < outputParamList.size(); k++) {
                            param = outputParamList.get(k);
                            value = entry.get(param);
                            // We need to write something even if the param does
                            // not exist in this entry.  Set to " " if null
                            if(value == null)
                                value = " ";
                            
                            // Write out this param value
                            if(k != outputParamList.size() -1)
                                bufWriter.write("\"" + value + "\",");
                            else
                                bufWriter.write("\"" + value + "\"\n");
                        }
                    }
                }
            }
            bufWriter.close();
            bufWriter = null;
        }
        catch(Exception ex) {
            Messages.writeStackTrace(ex);
        }
        finally {
            try {
               if(bufWriter != null)
                   bufWriter.close();
            }
            catch (Exception e2) {}
         }
    }
    
    // Parse the log file.  Put each entry in the log file into a hashtable.
    // Then fill an ArrayList with these entries
    private boolean parseLogFile() {
        SAXParser parser = getSAXParser();
        DefaultHandler paramSAXHandler = new MySaxHandler();

        UNFile logFile = new UNFile(logFilepath);
        if(logFile.exists()) {
            // Read log file and create a list of entries
            logEntryList.clear();
            try {
                parser.parse(logFile, paramSAXHandler);
            }
            catch (Exception e) {
                Messages.writeStackTrace(e);
            }
        }
        else {
            Messages.postError("Cannot find " + logFilepath);
            return false;
        }
        return true;
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
        // Create the list of entries based on the XML file
        public void startElement(String uri,   String localName,
                                String qName, Attributes attr) {
            if (qName.equals("entry")) {
                Hashtable<String,String> entry;
                entry = new Hashtable<String,String>();

                for(int i=0; i < attr.getLength(); i++) {
                    // Each entry in the log file will be put into a Hashtable
                    // Then each entry Hashtable will be added to an ArrayList
                    String key = attr.getQName(i);
                    String value = attr.getValue(i);
                    entry.put(key, value);
                    
                    // In case convertXmlToCsv() is called with 
                    // outputParamList == null, we need to keep a list of all
                    // possible params to use for the output.
                    if(!possibleParamList.contains(key))
                        possibleParamList.add(key);
                }
                logEntryList.add(entry);
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
    
    /*************************************************************** <pre>
     * People wanted to be able to call the conversion from a cron job.
     * The first version of this will assume they want all dates and
     * all keywords/parameters from the xml file.  The utility above
     * is setup so that a future version of this could take arguments
     * for date range and output params.
     </pre> **********************************************************/    
    public static void main(String[] args) {
        String usage = "Usage: xmlToCsv inputLogFilePath outputCsvPath "
                        + "[-type acq/login/all] [-paramlist p1 p2 p3 p4 ...]\n"
                        + "       [-startdate \"Month Date Year\"] "
                        + "[-enddate \"Month Date Year\"\n]"
                        + "       [-debug]";
        boolean typeSet = false;
        boolean paramsSet = false;
        boolean startSet = false;
        boolean endSet = false;
        boolean debugSet = false;

        bNoUI = true;
        SimpleDateFormat sdf = new SimpleDateFormat("MMM d yyyy");

        // Create an instance of XmlToCsvUtil.  Fill in all of the information
        // needed for the conversion, then call xml2csv.convertXmlToCsv()
        XmlToCsvUtil xml2csv = new XmlToCsvUtil();
        
        // Get the input and output filepaths from the args
        // The first arg is the input path and the 2nd arg is the output path
        
        if(args.length < 2) {
            System.out.println(usage);
            System.exit(0);
        }
        xml2csv.logFilepath = args[0];
        xml2csv.outputFilePath = args[1];
        
        int i = 2;  // Start on args[2] and go through the args
        while(args.length > i) {
            if(args[i].equals("-type") && args.length > i+1) {
                i++;
                typeSet = true;
                if(args[i].startsWith ("acq")) 
                    xml2csv.acquisitionSelected = true;
                else if(args[i].equals("login"))
                    xml2csv.loginSelected = true;
                else if(args[i].equals("both") || args[i].equals("all")) {
                    xml2csv.loginSelected = true;
                    xml2csv.acquisitionSelected = true;
                }
                else {
                    System.out.println(usage);
                    System.exit(0);
                }
                i++;
            }
            else if(args[i].equals("-paramlist") && args.length >= i+1) {
                // Take the following space separated args up to the end of
                // the args or to the next "-"
                i++;
                // While there are still args and we have not hit the next option
                while(args.length > i && !args[i].startsWith("-")) {
                    // Add to the list of params to use
                    xml2csv.outputParamList.add(args[i]);
                    i++;
                    paramsSet = true;
                }
            }
            else if(args[i].equals("-startdate") && args.length >= i+1) {
                i++;
                try {
                    Date entryStartDate = sdf.parse(args[i]);
                    if(entryStartDate != null) {
                        xml2csv.startDateRange = entryStartDate;
                        startSet = true;
                    }
                    else {
                        System.out.println("-startdate syntax error: " + args[i]);
                        System.exit(0);
                    }
                }
                catch(Exception ex) {
                    System.out.println("-startdate syntax error: " + args[i]);
                    System.exit(0);
                }
                i++;
            }
            else if(args[i].equals("-enddate") && args.length >= i+1) {
                i++;
                try {
                    Date entryEndDate = sdf.parse(args[i]);
                    if(entryEndDate != null) {
                        xml2csv.endDateRange = entryEndDate;
                        endSet = true;
                    }
                    else {
                        System.out.println("-enddate syntax error: " + args[i]);
                        System.exit(0);
                    }
                }
                catch(Exception ex) {
                    System.out.println("-enddate syntax error: " +
                    		"#" + args[i]);
                    System.exit(0);
                }
                i++;
            }
            else if(args[i].equals("-debug")) {
                debugSet = true;
                i++;
            }
                

        }
        if(!typeSet) {
            // If type was not set via arg, setfor the output of 
            // both login and acquisition data
            xml2csv.loginSelected = true;
            xml2csv.acquisitionSelected = true;
        }
        
        // If not set via args, set for the output of all dates
        if(!startSet)
            xml2csv.startDateRange = null;
        if(!endSet)
            xml2csv.endDateRange = null;
        
        // If not set via arg, set for output of all params in the xml file
        if(!paramsSet)
            xml2csv.outputParamList = null;
        
        // If requested, output all of the input information for debugging
        // purposes
        if(debugSet) {
            System.out.println("Input xml file: " + xml2csv.logFilepath);
            System.out.println("Output csv file: " + xml2csv.outputFilePath);
            System.out.println("login entries: " + xml2csv.loginSelected);
            System.out.println("acquisition entries: " + xml2csv.acquisitionSelected);
            if(xml2csv.startDateRange == null)
                System.out.println("start date: ALL");
            else
                System.out.println("start date: " + xml2csv.startDateRange);
            if(xml2csv.endDateRange == null)
                System.out.println("end date: ALL");
            else
                System.out.println("end date: " + xml2csv.endDateRange);
            if(xml2csv.outputParamList == null)
                System.out.println("param list: ALL");
            else 
                System.out.println("param list: " + xml2csv.outputParamList);
       }

        // Do the conversion and write the output file
        xml2csv.convertXmlToCsv();
    }

}

