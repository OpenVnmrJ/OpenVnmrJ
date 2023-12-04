/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.text.SimpleDateFormat;
import java.util.*;
import javax.swing.*;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.DefaultHandler;

import vnmr.util.*;

/********************************************************** <pre>
 * This brings up a panel for the purpose of setting information for
 * the conversion of xml log files into a Comma Separated Values (CSV) file.
 * The panel will allow 
 * - choosing of which parameters are to be included in the conversion
 * - choosing a Date range for selecting entries
 * - choosing a directory and filename for the output file
 * 
 </pre> **********************************************************/


public class LogToCSVPanel extends JDialog implements ActionListener {
    public ArrayList<String> selectedParamList = new ArrayList<String>();
    private LogDateChooser startDateCal, endDateCal;
    private JPanel mainPanel = null;
    private ArrayList<String> possibleParamList = new ArrayList<String>();
    private ArrayList<JCheckBox> checkBoxList = new ArrayList<JCheckBox>();
    private JCheckBox acqRecordBtn;
    private JCheckBox loginRecordBtn;
    private JTextField filePathField;
    private String logFilepath;
    // ArrayList of Log entries.  Each entry is a Hashtable with
    // keyword = param and value = value of param
    private ArrayList<Hashtable<String, String>> logEntryList = new ArrayList<Hashtable<String, String>>();

    public LogToCSVPanel(String filepath) {
        super(VNMRFrame.getVNMRFrame(), "Log to CSV Conversion");
        logFilepath = filepath;
        mainPanel = new JPanel();
        BorderLayout layout = new BorderLayout();
        mainPanel.setLayout(layout);

        createLayout();
        getContentPane().add(mainPanel);
        pack();        
        
    }

    
    private void createLayout() {
        JPanel dateRangePanel;
        JPanel filepathPanel;
        JPanel paramRadioPanel;
        JScrollPane paramScrollPane;
        
        // Create all of the portions of the panel
        dateRangePanel = createDateRangePanel();
        filepathPanel = createLowerPanelArea();
        paramRadioPanel = createParamCkBoxPanel();
        paramScrollPane = new JScrollPane(paramRadioPanel);
        paramScrollPane.setPreferredSize(new Dimension(200,190));   
        
        mainPanel.add(dateRangePanel, BorderLayout.WEST);
        mainPanel.add(paramScrollPane, BorderLayout.EAST);
        mainPanel.add(filepathPanel, BorderLayout.SOUTH);
    }
   
    // Create a panel with two calendars.  One for the beginning date
    // and one for the ending date.  Return the JPanel.
    private JPanel createDateRangePanel() {
        JPanel dateRangePanel = new JPanel();
        
        startDateCal= new LogDateChooser(Util.getLabel("Start Date:"));
        endDateCal  = new LogDateChooser(Util.getLabel("End Date:")); 
        dateRangePanel.add(startDateCal);
        dateRangePanel.add(endDateCal);
        
        
        return dateRangePanel;
    }
    
    // Create a panel with a Text field for the outputFilepath.  Have a
    // "Browse" button for obtaining the directory.
    private JPanel createLowerPanelArea() {
        JPanel lowerPanel = new JPanel();
        JPanel pathPanel = new JPanel();
        JPanel btnPanel = new JPanel();
        JPanel typePanel = new JPanel();

        JLabel label = new JLabel(Util.getLabel("Output File:"));
        filePathField = new JTextField(38);
        JButton browseBtn = new JButton(Util.getLabel("Browse"));
        browseBtn.addActionListener(this);
        pathPanel.add(label);
        pathPanel.add(filePathField);
        pathPanel.add(browseBtn);
        
        JButton convertBtn = new JButton(Util.getLabel("Convert"));
        JButton cancelBtn = new JButton(Util.getLabel("blCancel", "Cancel"));
        convertBtn.addActionListener(this);
        cancelBtn.addActionListener(this);
        btnPanel.add(convertBtn);
        btnPanel.add(cancelBtn);
        
        // If more log files are written in the correct XML format and they
        // are to be converted by this panel, add them to the group of
        // checkboxes.
        acqRecordBtn = new JCheckBox(Util.getLabel("Acquisition Records"));
        loginRecordBtn = new JCheckBox(Util.getLabel("Login Records"));
        typePanel.add(acqRecordBtn);
        typePanel.add(loginRecordBtn);

        BorderLayout layout = new BorderLayout();
        lowerPanel.setLayout(layout);
        lowerPanel.add(typePanel, BorderLayout.NORTH);
        lowerPanel.add(pathPanel, BorderLayout.CENTER);
        lowerPanel.add(btnPanel, BorderLayout.SOUTH);

        return lowerPanel;
    }
    
    // Get list of params from config file and add the ones automatically
    // added to the log file.  Create checkboxes for each and put in scroll pane.
    // Currently the config file is /vnmr/adm/accounting/loggingParamList
    private JPanel createParamCkBoxPanel() {
        BufferedReader input=null;
        UNFile file;
        String inLine;
        String sysDir;
        String paramListPath;
        StringTokenizer tok;
        
        // Add params that may not be in the config list
        possibleParamList.clear();
        possibleParamList.add("type");
        possibleParamList.add("start");
        possibleParamList.add("startsec");
        possibleParamList.add("end");
        possibleParamList.add("endsec");
        possibleParamList.add("operator");
        possibleParamList.add("owner");
        possibleParamList.add("seqfil");
        possibleParamList.add("systemname");
        possibleParamList.add("vnmraddr");
        possibleParamList.add("goflag");
        possibleParamList.add("loc");
        
        // Config file
        sysDir = System.getProperty("sysdir");
        paramListPath = sysDir + File.separator + "adm" + File.separator + 
                             "accounting"+ File.separator + "loggingParamList";
        
        file = new UNFile(paramListPath);
        if(file.exists()) {
            try {
                input = new BufferedReader(new FileReader(file));
            } catch(Exception e) {
                Messages.postError("Problem opening " + paramListPath);
                Messages.writeStackTrace(e, "Error caught in createParamCheckboxPanel");
            }
            
            try {
                if(input != null) {
                    while((inLine = input.readLine()) != null) {
                        /* skip blank lines and comment lines. */
                        if (inLine.length() > 1 && !inLine.startsWith("#")) {
                            tok = new StringTokenizer(inLine, ": \t");

                            // Get a param from the list.  We only need the name from
                            // the first column, ignore the other columns in the file.
                            String param = tok.nextToken().trim();
                            // If it is not already in the list, add it
                            if(!possibleParamList.contains(param)) {
                                possibleParamList.add(param);
                            }
                        }
                    }
                    input.close();
                }
            }
            catch(Exception e) {
                // Do not error out.  If paramList has things added to it,
                // it will be intact, just return unless debug set.
                if(DebugOutput.isSetFor("logToCSV")) {
                    Messages.postDebug("Problem parsing " + paramListPath);
                    Messages.writeStackTrace(e, "Error caught in createParamCheckboxPanel");
                }
            }             
        }

        JLabel title = new JLabel(Util.getLabel("Parameters To Include:"));
        // Now, paramList should be the list of params to create radio buttons from
        JPanel paramPanel = new JPanel();
       
        GridLayout layout = new GridLayout(possibleParamList.size() +1, 1);
        paramPanel.setLayout(layout);        
        paramPanel.add(title);

        // Create a checkbox for each param and add to panel and save the
        // button item for retrieving its status
        checkBoxList.clear();
        for(int i=0; i < possibleParamList.size(); i++) {
            String param = possibleParamList.get(i);
            JCheckBox checkbox = new JCheckBox(param);
            paramPanel.add(checkbox);
            checkBoxList.add(checkbox);
        }

        return paramPanel;
    }
    
    public void showPanel() {
        setVisible(true);
    }
    
    // Create the CSV file based on the input information
    private boolean createCSVFile() {
        boolean success;
        String param;
        String value;
        // SimpleDateFormat sdf;
        FileWriter fWriter;
        BufferedWriter bufWriter=null;
        String date;
        
        // Create an instance of XmlToCsvUtil.  Fill in all of the information
        // needed for the conversion, then call xml2csv.convertXmlToCsv()
        XmlToCsvUtil xml2csv = new XmlToCsvUtil();

        // sdf = new SimpleDateFormat("EEE MMM d HH:mm:ss zzz yyyy");
        
        // Fill list of params selected to write out
        if (!fillSelectedParamList())
            return false;
        xml2csv.outputParamList = selectedParamList;

        // Get dates
        xml2csv.startDateRange = startDateCal.getDate();
        xml2csv.endDateRange = endDateCal.getEndDate();
        
        // Get acq/login selection
        xml2csv.acquisitionSelected = acqRecordBtn.isSelected();
        xml2csv.loginSelected = loginRecordBtn.isSelected();
        
        
        // Get Output file path and be sure it has a .cvs suffix
        xml2csv.outputFilePath = filePathField.getText();
        if(xml2csv.outputFilePath.trim().length() < 2) {
            // Messages.postError("The Output File Path must be set.");
            JOptionPane.showMessageDialog(this,
                 "The output file path must be set.",
                 "Log to CSV",
                 JOptionPane.WARNING_MESSAGE);

            return false;
        }
        
        xml2csv.logFilepath = this.logFilepath;
        
        // Now we have all of the information we need to proceed with
        // creating a CSV file.  Go do the work.
        xml2csv.convertXmlToCsv();
        return true;
    }
    
    // Loop through the param checkboxes and keep a list of selected params
    private boolean fillSelectedParamList() {
        int num = 0;
        selectedParamList.clear();

        for(int i=0; i < checkBoxList.size(); i++) {
            JCheckBox ckBox = checkBoxList.get(i);
            if(ckBox.isSelected()) {
                String param = ckBox.getText();
                selectedParamList.add(param);
                num++;
            }
        }
        if (num < 1) {
            JOptionPane.showMessageDialog(this,
                 "No parameters were selected.",
                 "Log to CSV",
                 JOptionPane.WARNING_MESSAGE);
            return false;
        }
        return true;
    }
    
    // Browse button calls this.
    // Put up a JFileChooser and let the user browse to the directory/file
    // where they want the CSV output to go.
    private void setLogFile(){
        String outputFilepath;
        JFrame saveLog= new JFrame();
        saveLog.setTitle("Set Output CSV File");
        
        // Get the current contents of the text field
        outputFilepath = filePathField.getText();

        JFileChooser fc = new JFileChooser();
        
        fc.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
        if(outputFilepath != null)
            fc.setSelectedFile(new File(outputFilepath));
        int returnVal = fc.showDialog(saveLog, "Select Output File");
        
        if(returnVal != JFileChooser.APPROVE_OPTION) {
            Messages.postError("Problem with selected file. Aborting");
            return;
        }
        File file = fc.getSelectedFile();
        outputFilepath = file.getPath();
        
        // Set the Text Field
        filePathField.setText(outputFilepath);
        
        return;
    }    
    
    // One of the buttons was clicked
    public void actionPerformed(ActionEvent evt) {
        
        String cmd = evt.getActionCommand();
        
        if(cmd.equals("Convert")) {
            // Create the output file
            if (createCSVFile())
                this.setVisible(false);
        }
        else if(cmd.equals("Cancel")) {
            this.setVisible(false);
        }
        else if(cmd.equals("Browse")) {
            // Bring up a filechooser and let the user browse to a file
            setLogFile();
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

}













