/* 
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */
/* DISCLAIMER :
     * ------------
     *
 * This is a beta version of the GALAXIE integration library.
 * This code is under development and is provided for information purposes.
 * The classes names and interfaces as well as the file names and
 * organization is subject to changes. Moreover, this code has not been
 * fully tested.
 *
 * For any bug report, comment or suggestion please send an email to
 * gilles.orazi@varianinc.com
 *
 * Copyright Varian JMBS (2002)
 */

#include "common.h"
#include <iostream>
#include <fstream>
#include "SignalReader.h"
#include "integrator_types.h"
#include "integrator_ioFunctions.h"
#include "integrator_manager_Cstruct.h"

int main(int argc, char *argv[]){

//check command line arguments
    if (argc<2) {
        cout<<"Need a file name."<<endl;
        return -1;
    }

//Read the signal
    TSignalReader sgnrd ;
    sgnrd.ReadFile(argv[1]);
    INT_SIGNAL signal   = sgnrd.Signal  ;
    int        nbpoints = sgnrd.length();

//Read the integration events
    string evtfilename = string(argv[1]) + string(".ievt");
    INT_PARAMETERS* parameters = CreateParametersFromFile(evtfilename);

//Call integration algorithm
    INT_RESULTS* pResults = Integrate( signal, nbpoints, parameters) ;

//Print out results on std out
    FormatResults(*pResults, cout);

//Print out results in a file
    std::ofstream resultfile("results");
    FormatResults(*pResults, resultfile);
    resultfile.close();

//Free allocated memory
    Free_Results(pResults);
    Free_Parameters(parameters);
    delete [] signal ;

    return 0;
}
