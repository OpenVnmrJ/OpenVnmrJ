/* 
 * Varian, Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */


#include <iostream>
#include <fstream>
#include <strstream>
#include <stdio.h>
#include "SignalReader.h"
#include "integrator_types.h"
#include "integrator_ioFunctions.h"
#include "integrator_manager_Cstruct.h"

#define MAX_CHANNELS 5


typedef struct {
    float deltaTime;             // Delta time between points (dwell)
    int numChannels;             // Number of data columns (not counting time)
    int traceNumbers[MAX_CHANNELS]; // Trace # of column, starting from 0.
    float transferTimes[MAX_CHANNELS]; //Transfer time for each channel, not NMR
    float refTime;              // Reference transfer time
    float timeFirstPt;          // Time for the first point.
    bool active[MAX_CHANNELS];  // Active status for each channel, not NMR
} LCinfo;

static bool debug;

void writeResultsFileForVJ(INT_RESULTS& , ostream&, int, float, float);
void readVJLCfile(char *filepath, LCinfo& lcinfo);
void fatalError(const char *msg);
void fatalError(const char *msg1, const char *msg2);
static char *getGalaxieInputFilePath(int i);
static char *getResultFilePath();
static char *getDebugFilePath();

std::ofstream dbg(getDebugFilePath());


int main(int argc, char *argv[]){
    char filepath[1024] = "\0";
    float threshold = 20.0;
    float peakwidth = 0.2;
    int trace = 0;

    //checking command line arguments
    debug = false;
    if (argc<2) {
        dbg<<"Need a file name."<<endl;
        return -1;
    }
    // If only one arg, it must be the filepath.
    if (argc == 2)
        strcpy(filepath, argv[1]);
    else {
        for(int i=1; i < argc; i++) {
            if(strcmp(argv[i], "-t") == 0) {
                if(argc > i+1) {
                    threshold = atof(argv[++i]);
                }
            }
            else if(strcmp(argv[i], "-p") == 0) {
                if(argc > i+1) {
                    peakwidth = atof(argv[++i]);
                }
            }
            else if(strcmp(argv[i], "-f") == 0) {
                if(argc > i+1) {
                    strcpy(filepath, argv[++i]);
                }
            }
            else if(strcmp(argv[i], "-c") == 0) {
                if(argc > i+1) {
                    trace = atoi(argv[++i]) + 1;
                }
            }
            else if(strcmp(argv[i], "-debug") == 0) {
                debug = true;
            }
        }
    }

    if (debug) {
        dbg << "vjLCAnalysis: Filepath = " << filepath
             << "  Threshold = " << threshold
             << "  Peakwidth = " << peakwidth
             << "  Trace = " << trace << endl;
    }

    // Trap for bad or missing filepath.  Write error to result file
    // so java can find and report it.  This is a fatal error.
    if(strlen(filepath) < 1) {
        fatalError("No filepath given for LC peak analysis.");
    }

    // Trap for bad threshold and peakwidth.  Just default them and keep going.
    if(threshold <= 0.0)
        threshold = 50;
    if(peakwidth <= 0.0)
        peakwidth = 0.2;


    // Make an empty container for the results to come back in and initialize
    // its members
    LCinfo lcinfo;
    lcinfo.deltaTime = 0.0;
    lcinfo.numChannels = 0;
    lcinfo.timeFirstPt = 0.0;
    for(int i=0; i < MAX_CHANNELS; i++) {
        lcinfo.transferTimes[i] = -1.0;
        lcinfo.active[i] = false;
    }

    // Read the VJ LC file and create one file per each active channel
    // in a format for the Galaxie code to use.
    readVJLCfile(filepath, lcinfo);

    if (debug) {
        dbg << "Input Info" << endl;
        dbg << "Delta " << lcinfo.deltaTime << endl;
        dbg << "numchannels " << lcinfo.numChannels << endl;
        dbg << "Trace numbers: ";
        for (int i = 0; i < lcinfo.numChannels; i++) {
            dbg << lcinfo.traceNumbers[i] << " ";
        }
        dbg << endl;
        dbg << "Refrence time: " << lcinfo.refTime << endl;
        dbg << "transfer times - ref time: " 
             << lcinfo.transferTimes[0] 
             << "  " << lcinfo.transferTimes[1] 
             << "  " << lcinfo.transferTimes[2] 
             << "  " << lcinfo.transferTimes[3]  << endl;
        dbg << "Time for 1st point: " << lcinfo.timeFirstPt << endl;
        dbg << "Active: " << lcinfo.active[0] 
             << "  " << lcinfo.active[1] 
             << "  " << lcinfo.active[2] << endl;
    }

    // Open vj format output file
    std::ofstream vjresultfile(getResultFilePath());
    vjresultfile << "Time      Peak# Channel Max Height  Area  Width50   " 
                 << "StartTime  EndTime   "
                 << "BlStart    BlEnd   BlStartVal  BlEndVal" << endl;

    // Go through the channels, doing analysis on the active ones.
    // Write out a file per active channel with results from Galaxie.
    for(int i=0; i < lcinfo.numChannels; i++) {
        if (debug) {
            dbg << "Trace[" << i << "]=" << lcinfo.traceNumbers[i] << endl;
        }
        if (lcinfo.traceNumbers[i] == trace && lcinfo.active[i]) {
            //Reading the signal
            TSignalReader sgnrd ;
            sgnrd.ReadFile(getGalaxieInputFilePath(i));
            
            INT_SIGNAL signal   = sgnrd.Signal  ; 
            int        nbpoints = sgnrd.length();

            INT_PARAMETERS *parameters = new INT_PARAMETERS;

            // Set up params and events for the analysis
            parameters->ReduceNoise = true;
            parameters->UseRelativeThreshold = true;
            parameters->ComputeBaselines = true;
            parameters->SpikeReduction = 1.0;
            parameters->DeltaTime = lcinfo.deltaTime;
//parameters->UseRelativeThreshold = false;
            parameters->NumberOfEvents = 4;
            parameters->events = new INT_EVENT[parameters->NumberOfEvents];
            // Peak width
            parameters->events[0].id = INTEGRATIONEVENT_SETPEAKWIDTH;
            parameters->events[0].time = 0.0;
            parameters->events[0].value = peakwidth;
            // Threshold
            parameters->events[1].id = INTEGRATIONEVENT_SETTHRESHOLD;
            parameters->events[1].time = 0.0;
            parameters->events[1].value = threshold;
            // Turn analysis off at the start
            parameters->events[2].id = INTEGRATIONEVENT_INTEGRATION;
            parameters->events[2].time = 0.0;
            parameters->events[2].on = false;
            // Turn analysis back on
            parameters->events[3].id = INTEGRATIONEVENT_INTEGRATION;
            parameters->events[3].time = 0.1;
            parameters->events[3].on = true;


            //Calling integration algorithm
//            TIntegration_Manager_Cstruct IntgMan(signal,
//                                                 nbpoints,  
//                                                 parameters);

//            IntgMan.integrate();

            INT_RESULTS* pResults = Integrate( signal, nbpoints, parameters);

            //Get the results
//            INT_RESULTS* pResults = IntgMan.AllocateResults() ; 

            if (debug) {
                // Print on std out
                FormatResults(*pResults, dbg);
            }

            // Write VJ formatted result file for this channel
            writeResultsFileForVJ(*pResults, vjresultfile, trace - 1, 
                                 lcinfo.transferTimes[i] + lcinfo.timeFirstPt,
                                 lcinfo.deltaTime);

            //Free allocated memory
            Free_Results(pResults);
            Free_Parameters(parameters);
            delete [] signal ;
        }
    }
    vjresultfile.close();

    return 0;
}


/*******************
    readVJLCfile
    
    Open the VJ format LC data file
    Check the File format version
    Get the dwell, number of channels, and Transfer times
    Starting at the keyword 'data:', read n channels of data
        Add the time of the first point to the Transfer times.  In Galixie,
              the first point will be 0.0 time.
        From the 2nd point on, ignore the time parameter.
        Each line will have a time, followed by N values for the N channels
        Write out N files which have the number of points as the first
              line, followed by values, one per line for that channel.
        Name the files 0, 1, 2, ... for the channel designator
    Return the dwell, number of channels, and Transfer times (modified)

*******************/

void readVJLCfile(char *filepath, LCinfo& lcinfo) {
    string str;
    char cstr[200];
    std::istrstream *is;
    std::ofstream *outfile[MAX_CHANNELS];
    

    std::ifstream input(filepath);
    if (!input) 
        return;


    while(input.good()) {
        input.getline(cstr, 160);
        str = cstr;
        // NB: Leave off first char of tags in case they're capitalized
        if(str.find("ctive channels") != string::npos) {
            // Line following, contains something like #TRUE#,#FALSE#,#TRUE#
            // Determine how many channels and which ones are active.
            input.getline(cstr, 192);
            is = new  istrstream(cstr);
            int i=0;
            // Fill the transfer strings array with the transfer times
            while(is->good()) {
                is->getline(cstr, 40, ',');
                if(strcmp(cstr, "#TRUE#") == 0)
                    lcinfo.active[i] = true;
                else
                    lcinfo.active[i] = false;
                i++;
            }
            lcinfo.numChannels = i;  // Number of channels not counting NMR
            delete is;
        }
        else if(str.find("umber of channels") != string::npos) {
            // Line following is the Number of Channels (columns)
            input.getline(cstr, 192);
            lcinfo.numChannels = atoi(cstr);
        }
        else if(str.find("race numbers") != string::npos) {
            // Line following has Trace numbers
            input.getline(cstr, 192);
            is = new  istrstream(cstr);
            for (int i = 0; i < lcinfo.numChannels && is->good(); i++) {
                is->getline(cstr, 40, ',');
                lcinfo.traceNumbers[i] = atoi(cstr);
            }
            delete is;
        }
        else if(str.find("ransfer times") != string::npos) {
            // Line following has Transfer times
            input.getline(cstr, 192);
            is = new  istrstream(cstr);
            int i=0;
            // Fill the transfer strings array with the transfer times
            while(is->good()) {
                is->getline(cstr, 40, ',');
                lcinfo.transferTimes[i] = atof(cstr);
                i++;
            }
            delete is;
        }
        else if(str.find("eference time") != string::npos) {
            // Line following is the Reference transfer time
            input.getline(cstr, 192);
            lcinfo.refTime = atof(cstr);
        }
        else if(str.find("elta time") != string::npos) {
            // Line following Delta (dwell) time
            input.getline(cstr, 192);
            lcinfo.deltaTime = atof(cstr);
        }
        else if(str.find("ata:") != string::npos) {
            // Be sure the necessary values in lcinfo were filled in above.
            if(lcinfo.deltaTime <= 0.0)
                lcinfo.deltaTime = 1.0;  // default to 1 
            if(lcinfo.numChannels <= 0)
                fatalError("No active channels found in", filepath);
            for(int i=0; i < lcinfo.numChannels; i++) {
                if(lcinfo.transferTimes[i] < 0.0)
                    lcinfo.transferTimes[i] = 0.0;  // Default to 0.0
                lcinfo.transferTimes[i] -= lcinfo.refTime;
            }

            // Lines following have time, amp1, amp2, amp3 ...
            // We need to write out a file for each active channel.
            // Open the files, and write out as we read in each line.
            for(int i=0; i < lcinfo.numChannels; i++) {
                if(lcinfo.active[i]) {
                    outfile[i] = new std::ofstream(getGalaxieInputFilePath(i));

                    // Write a dummy "number of points" in the first location
                    // After we determine the number of points, we will
                    // fill in the correct value.  Write spaces so that when
                    // a number is written, the remaining spaces will not
                    // effect the intrepretation.  Write more spaces than
                    // the number of points can ever occupy.
                    *outfile[i] << "         " << endl;
                }
            }
            
            // Now go through the data, and write to the opened file(s)
            float time = 0;
            int k=0;
            while(input.good()) {
                input.getline(cstr, 192);
                is = new  istrstream(cstr);
                // The first item in each line is the current time.
                is->getline(cstr, 40, ',');
                float t = atof(cstr);
                if (t != 0) {
                    time = t;
                }
                if(k == 0) 
                    // Save time of first point
                    lcinfo.timeFirstPt = time;
                int isdata=false;
                for (int i = 0; i < lcinfo.numChannels && is->good(); i++) {
                    isdata = true;
                    is->getline(cstr, 40, ',');
                    // Write out value if channel active
                    if(lcinfo.active[i]) {
                        *outfile[i] << cstr << endl;
                    }
                }
                delete is;
                // Only increment if data was found.
                if(isdata)
                    k++;
            }
            // The number of points is now k, reset to the beginning of
            // the streams, and write this value.
            for(int i=0; i < lcinfo.numChannels; i++) {
                if(lcinfo.active[i]) {
                    outfile[i]->seekp(0);
                    *outfile[i] << k;
                    outfile[i]->flush();
                    outfile[i]->close();
                }
            }

            if (k > 1) {
                // Calculate the time between data points
                // (Overrides any value in header)
                if (debug) {
                    dbg << "k=" << k << ", firstTime=" << lcinfo.timeFirstPt
                        << ", time=" << time << endl;
                }
                lcinfo.deltaTime = (time - lcinfo.timeFirstPt) / (k - 1);
            }
        }
    }
}




/*******************
    writeResultsFileForVJ

    Write out results to a file for this channel.
    There will be one for each channel, and these will be merged
    before handing off to vnmrj.  offsetTime is the sum of the
    time for the first point plus the transferTime.
********************/

void writeResultsFileForVJ(INT_RESULTS& results, ostream& out, int channel,
                           float offsetTime, float deltaTime) {

    for(int idxpeak=0; idxpeak<results.NumberOfPeaks; ++idxpeak) {
        INT_RESULT_PEAK& peak = results.Peaks[idxpeak] ;
        out << peak.RetentionTime + offsetTime << ",   " 
            << idxpeak+1 << ",    "
            << channel << ",    "
            << peak.Height << ",  " 
            << peak.Area << ",  " 
            << peak.Width_50p << ",  "
            << peak.StartTime + offsetTime << ",  "
            << peak.EndTime + offsetTime << ",  ";

        // Get the baseline that goes with this peak.
        INT_RESULT_LINE& line = results.Lines[peak.BaselineIdx];
        out << line.StartTime + offsetTime  << ",  " 
            << line.EndTime + offsetTime << ",  "
            << line.StartValue << ",  "
            << line.EndValue << endl;

    }
}

void fatalError(const char *msg) {
        dbg << msg << endl;
        std::ofstream vjresultfile(getResultFilePath());
        vjresultfile << msg << endl;
        vjresultfile.close();
        exit(0);
}

void fatalError(const char *msg1, const char *msg2) {
    char msg[512];
    sprintf(msg, "Error: %s %s", msg1, msg2);
    fatalError(msg);
}

/**
 * Get the pathname for a data file that Galaxie reads as input.
 * The channel number is included in the file's name.
 * @param i The channel number.
 */
static char *getGalaxieInputFilePath(int i) {
    static char path[512];
    // Add user name to file name so other users don't have to delete it
    // On Linux you can't delete other people's files in /tmp
    sprintf(path, "/tmp/vjlc%d-%s", i, getenv("USER"));
    return path;
}

/**
 * Get the pathname for the results file for VJ.
 */
static char *getResultFilePath() {
    static char path[512];
    // Add user name to file name so other users don't have to delete it
    // On Linux you can't delete other people's files in /tmp
    sprintf(path, "/tmp/vjresults-%s", getenv("USER"));
    return path;
}

/**
 * Get the pathname for the debug log file.
 */
static char *getDebugFilePath() {
    static char path[512];
    sprintf(path, "%s/GalaxieMsgLog", getenv("vnmruser"));
    return path;
}
