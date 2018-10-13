/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.lc;

import  vnmr.util.*;

import java.util.*;
import java.io.*;



/********************************************************** <pre>
 * Summary: Analyze LC data
 *
 *      Call vjLCAnalysis to analyze lc peaks.  It will put the results
 *      into m_vjresults.  Then parse that file and create a TreeSet
 *      of LcPeak objects, one for each line in vjresults.
 *
 *      vjLCAnalysis is a standalone based on Galaxie.
 *
 *      Usage example:
 *          double threshold = 40.0;
 *          double peakwidth = 0.234;
 *          String filepath = "/mydirectory/testdata.lcd";
 *          // Create an LcAnalysis with these values
 *          LcAnalysis lca = new LcAnalysis(filepath, threshold, peakwidth);
 *          // Call vjLCAnalysis which leave its results in m_vjresults
 *          lca.doAnalysis();
 *          // Parse the vjresults file putting LcPeak's in a TreeSet
 *          TreeSet peaks = lca.getResults();
 *
 *          // Example access of the peaks
 *          Iterator iter = peaks.iterator();
 *          LcPeak peak;
 *          while(iter.hasNext()) {
 *              peak = (LcPeak)iter.next();
 *              System.out.println(peak.id + "  " + peak.channel + "  " 
 *                                 + peak.time + "  " + 
 *                                 peak.fwhm + "  " + peak.area + "  " 
 *                                 + peak.height + "  " + 
 *                                 peak.units);
 *          }
 *
 </pre> **********************************************************/
public class LcAnalysis {

    private String m_filepath;
    private double  m_threshold = 1000;
    private double  m_peakwidth = 1;
    private boolean[] m_activeTraces;
    private String m_vjresults;


    /******************************************************************
     * Summary: Constructor, set filepath, threshold and peakwidth for later 
     *          use.
     *
     *      If threshold or peakwidth are passed in as 0, they will
     *      be set to a default value later.
     *****************************************************************/
    public LcAnalysis(String fpath, double thresh, double width,
                      boolean[] traces) {
        if (!fpath.endsWith(".lcd")) {
            fpath += ".lcd";
        }
        m_filepath = fpath;
        m_threshold = thresh;
        m_peakwidth = width;
        m_activeTraces = traces;
        m_vjresults = "/tmp/vjresults-" + System.getProperty("user");
        
        // Get rid of any existing results file
        File file = new File(m_vjresults);
        file.delete();
    }

    public LcAnalysis(String filepath) {
        if (!filepath.endsWith(".lcd")) {
            filepath += ".lcd";
        }
        m_filepath = filepath;
        m_vjresults = "/tmp/vjresults-" + System.getProperty("user");
        
        // Get rid of any existing results file
        File file = new File(m_vjresults);
        file.delete();
    }

    /******************************************************************
     * Summary: Parse the output file from the analysis.
     *
     *   Make an LcPeak object from each peak.  Put the LcPeak objects
     *   into a TreeSet.
     *****************************************************************/
    public SortedSet<LcPeak> getResults(SortedSet<LcPeak> peakList) {
        StringTokenizer tokstring;
        BufferedReader  in;
        String          inLine=null;
        int id=-1;
        int chan=-1;
        double time=0.0;
        double fwhm=0.0;
        double area=0.0;
        double height=0.0;
        String units = "";
        double startTime=0.0;
        double endTime=0.0;
        double blStartTime=0.0;
        double blEndTime=0.0;
        double blStartVal=0.0;
        double blEndVal=0.0;

        if (peakList == null) {
            peakList = new TreeSet<LcPeak>();
        } else {
            peakList.clear();
        }
        try {
            File file = new File(m_vjresults);
            if(!file.canRead()) {
                Messages.postError("Cannot read LC Analysis results file: " 
                                   + m_vjresults);
                // Return the empty list.
                return peakList;
            }
            // Open the file.
            in = new BufferedReader(new FileReader(m_vjresults));
            inLine = in.readLine();
            // Check for empty file
            if(inLine == null) {
                Messages.postError("Empty LC results file: \""
                                   + m_vjresults + "\"");
                return peakList;
            }
            // Check for error from lc analysis
            if(inLine.startsWith("Error:")) {
                // Just send this error on to the user.
                Messages.postError(inLine);
                return peakList;
            }
            // Check to see if we have the proper header line on this file.
            if(!inLine.startsWith("Time")) {
                Messages.postError("Bad Format in LC results file: \""
                                   + m_vjresults + "\"");
                return peakList;
            }

            // Following the header line, should be one line for each peak
            // with 10 columns (Time Peak# Channel# Max_Height Area Width50
            // blStartTime blEndTime blStartVal blEndVal)
            while ((inLine = in.readLine()) != null) {
                id = chan = -1;
                time = area = height = fwhm = 0.0;
                tokstring = new StringTokenizer(inLine, ",");
                if(tokstring.hasMoreTokens())
                    time = Double.parseDouble(tokstring.nextToken().trim());
                if(tokstring.hasMoreTokens())
                    id= Integer.parseInt(tokstring.nextToken().trim());
                if(tokstring.hasMoreTokens())
                    chan = Integer.parseInt(tokstring.nextToken().trim());
                if(tokstring.hasMoreTokens())
                    height = Double.parseDouble(tokstring.nextToken().trim());
                if(tokstring.hasMoreTokens())
                    area = Double.parseDouble(tokstring.nextToken().trim());
                if(tokstring.hasMoreTokens())
                    fwhm = Double.parseDouble(tokstring.nextToken().trim());
                 if(tokstring.hasMoreTokens())
                    startTime = 
                           Double.parseDouble(tokstring.nextToken().trim());
                if(tokstring.hasMoreTokens())
                    endTime = 
                           Double.parseDouble(tokstring.nextToken().trim());
                 if(tokstring.hasMoreTokens())
                    blStartTime = 
                           Double.parseDouble(tokstring.nextToken().trim());
                if(tokstring.hasMoreTokens())
                    blEndTime = 
                           Double.parseDouble(tokstring.nextToken().trim());
                if(tokstring.hasMoreTokens())
                    blStartVal = 
                           Double.parseDouble(tokstring.nextToken().trim());
                if(tokstring.hasMoreTokens())
                    blEndVal = Double.parseDouble(tokstring.nextToken().trim());
               
                // We don't have the units, but it is a required arg, so send
                // an empty string.
                if(id == -1 || chan == -1) {
                    Messages.postError("Badly Formated line in LC results "
                                       + "file: \""
                                       + m_vjresults + "\"");
                                       
                }
                else {
                    // Add this peak to the list of peaks to be returned.
                    // Sub 1 from id to start counting at 0
                    LcPeak peak = new LcPeak(id-1, chan, time, fwhm, area,
                                             height, units, startTime, endTime,
                                             blStartTime, blEndTime, blStartVal,
                                             blEndVal);

                    peakList.add(peak);
                }          
            }

        } 
        catch(Exception e) { 
            Messages.postError("Problem opening or parsing " 
                               + "LC results file: \""
                               + m_vjresults + "\"");
            Messages.writeStackTrace(e);
            return peakList;
        }

        return peakList;

    }

    /******************************************************************
     * Summary: Start LC peak analysis and wait for completion.
     *
     *    Call the vjLCAnalysis standalone.  It will find the peaks and
     *    put them into m_vjresults.
     *****************************************************************/
    public void doAnalysis(double thresh, double width, int chan) {
        Process    proc;
        Runtime    rt;
        String     cmd;

        this.m_threshold = thresh;
        this.m_peakwidth = width;

        try {
            rt = Runtime.getRuntime();
            File file = new File(m_filepath);
            if (file == null || !file.canRead()) {
                String errmsg = "doAnalysis(): Cannot read file: \""
                                       + m_filepath + "\"";
                m_filepath = file.getParent() + File.separator + "lcdata";
                file = new File(m_filepath);
                if (file == null || !file.canRead()) {
                    errmsg += " or \"" + m_filepath + "\"";
                    Messages.postError(errmsg);
                    return;
                }
            }
            cmd = "vjLCAnalysis -t " + m_threshold + " -p " + m_peakwidth
                                   + " -f " + m_filepath + " -c " + chan;
            if (DebugOutput.isSetFor("Galaxie")) {
                cmd += " -debug";
            }
            Messages.postDebug("Galaxie",
                               "LcAnalysis.doAnalysis: cmd= " + cmd);
            Messages.postDebug("Galaxie", "Calling vjLCAnalysis ...");
            proc = rt.exec(cmd); // Start it.
            proc.waitFor(); // Wait here for the anlysis to complete.
            Messages.postDebug("Galaxie", "... vjLCAnalysis done.");
        } catch (Exception e) {
            Messages.postDebug("Galaxie", "... Could not exec vjLCAnalysis");
            Messages.postError("Problem running vjLCAnalysis program");
            Messages.writeStackTrace(e,
                                     "Error caught in LcAnalysis.doAnalysis");
        }
    }

    public static void analyzeData(SortedSet<LcPeak>[] out,
                                   String filepath, String args) {

        Messages.postDebug("Galaxie", "LcAnalysis.analyzeData() starting");
        // Create an LcAnalysis for these data
        LcAnalysis lca = new LcAnalysis(filepath);

        StringTokenizer toker = new StringTokenizer(args);
        while (toker.hasMoreTokens()) {
            int nargs = toker.countTokens();
            if (nargs < 2) {
                Messages.postError("Invalid analysis parameters: " + args);
                return;
            }
            String arg = toker.nextToken();
            if (arg.equalsIgnoreCase("off")) {
                // Grab the channel number to turn off
                try {
                    int chan = Integer.parseInt(toker.nextToken()) - 1;
                    if (out[chan] != null) {
                        out[chan].clear();
                    }
                } catch (NumberFormatException nfe) {
                    Messages.postError("Invalid analysis parameters: " + args);
                    return;
                }
            } else {
                // Grab Threshold, LineWidth, and Channel#
                if (nargs < 3) {
                    Messages.postError("Invalid analysis parameters: " + args);
                    return;
                }
                try {
                    // Note: already parsed the first argument
                    double thresh = Double.parseDouble(arg);
                    double width = Double.parseDouble(toker.nextToken());
                    int chan = Integer.parseInt(toker.nextToken()) - 1;
                    Messages.postDebug("Galaxie", "Call doAnalysis ...");
                    lca.doAnalysis(thresh, width, chan);
                    Messages.postDebug("Galaxie", "Call getResults ...");
                    out[chan] = lca.getResults(out[chan]);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Invalid analysis parameters: " + args);
                    return;
                }
            }
        }

        return;
    }
}
