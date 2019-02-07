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

#include "integrator_types.h"
#include "integrator_calcul.h"
#include "SignalReader.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "debug.h"

int main(int argc, char *argv[]){
    if (argc<2) {
        cout<<"need a file name"<<endl;
        return 10;
    }

    SetVerboseLevel(2);

    float _threshold ;
    if (argc>2) {
        sscanf(argv[2],"%f",&_threshold);
    }
    else
        _threshold = 0.1 ;

    cout<<endl<<"-------- Testing integration library -------- "<<endl<<endl;

    cout<<endl<<". Threshold = "<<_threshold <<endl;
    cout << ". Getting signal"<<endl;
    cout << "  File to be opened : " << argv[1] << endl;
    TSignalReader sgnrd ;
    sgnrd.ReadFile(argv[1]);
    cout << "  Number of data points : " << sgnrd.length() << endl ;

//cout << "  Data : " ;
//for (int i=0;i<sgnrd.length(); i++) cout<<sgnrd.Signal[i]<<"  ";
//cout << endl<<endl;

    TInternalIntegrator InternalIntegrator ;
    INT_SIGNAL smoothedSignal ;

    InternalIntegrator.Signal = sgnrd.Signal ;
    InternalIntegrator.SignalSize = sgnrd.length();

    smoothedSignal = new INT_FLOAT[sgnrd.length()];
    InternalIntegrator.SignalSmooth = smoothedSignal ;
    InternalIntegrator.FReduceNoise = true ;
    InternalIntegrator.FUseRelativeThreshold = true ;

    InternalIntegrator.FComputeBaseLines = true ;
    InternalIntegrator.FSpikeReduceParameter = 1.0 ;

    InternalIntegrator.DeltaTime = 0.010 ;

    TIntegrationEvent* aevent ;
    double tmpdble = _threshold ;
    cout<<endl<<". Threshold = "<<_threshold <<endl;
    aevent = new TIntegrationEvent_VALUE1( "ST", "ST",
        0.0,
        tmpdble,
        INTEGRATIONEVENT_SETTHRESHOLD,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT);
    InternalIntegrator.Events.push_back(aevent);

    aevent = new TIntegrationEvent_VALUE1( "SPW", "SPW",
        0.0,
        0.9,
        INTEGRATIONEVENT_SETPEAKWIDTH,
        INTEGRATION_PRIORITY_NORMAL,
        IEC_PARAMDETECT);
    InternalIntegrator.Events.push_back(aevent);

//InternalIntegrator.ResultPeaks := Integrator.OutputPeaks ;
//InternalIntegrator.ResultLines := Integrator.OutputBaselines ;

//InternalIntegrator.FAutoThresholdResults := Integrator.AutoThresholdResults ;
//Integrator.AutoThresholdResults.FreeAll;
//Integrator.AutoThresholdResults.Clear;

    cout<<endl<<". Smoothing and various initializations"<<endl;
    InternalIntegrator.Initialize() ; // smooth signal and compute derivatives

    cout<<endl<<". Integrating"<<endl;
    InternalIntegrator.FindPeaksAverageSlope() ;
//Integrator.OutputNoiseSDev := InternalIntegrator.FnoiseSDev ;

    cout<<endl<<". Computing noise"<<endl;
    InternalIntegrator.ComputeNoiseFromProcessedIntegrationEvents() ;
//Integrator.OutputNoise := InternalIntegrator.FNoiseComputer.GetNoise ;
//Integrator.OutputRMSNoise := InternalIntegrator.FNoiseComputer.GetRMSNoise ;
//tmp division by zero
//Integrator.OutputDrift := InternalIntegrator.FNoiseComputer.GetDrift ;

    cout<<endl<<endl<<". RESULTS :"<<endl;
    cout<<endl<<"   peaks : "<<endl;
    for(TIntegrationPeakList::iterator peak_it = InternalIntegrator.ResultPeaks.begin();
    peak_it!=InternalIntegrator.ResultPeaks.end();
    peak_it++)
    {
        TIntegrationPeak& curpeak = **peak_it ;
        cout<<curpeak.StartTime()<<" - "<<curpeak.EndTime()<<endl;
    }

    cout<<endl<<endl<<"   lines : "<<endl;
    for(TBaseLineList::iterator line_it = InternalIntegrator.ResultLines.begin();
    line_it!=InternalIntegrator.ResultLines.end();
    line_it++)
    {
        TBaseLine& curline = **line_it ;
        cout<<curline.StartTime()<<" - "<<curline.EndTime()<<endl;
    }

    std::ofstream afile("smooth") ;
    for (int i=0; i<sgnrd.length(); i++) afile<<smoothedSignal[i]<<" ";
    afile << endl;
    delete [] smoothedSignal ;
    cout<<endl<<". End"<<endl;
    return 0;
}
