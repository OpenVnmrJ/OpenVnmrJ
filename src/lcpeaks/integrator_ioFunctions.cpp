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

// $History: integrator_ioFunctions.cpp $
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 8/11/02    Time: 17:40 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* C++ translation of the integration algorithm.  */
/* All main translation bugs fixed after having performed the official */
/* integration tests. */
/*  */
/* *****************  Version 5  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 12/07/02   Time: 14:45 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 1  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:57 */
/* Created in $/Orlando_2000/DLL_Integration/Cpp */

#include "common.h"
#include "integrator_ioFunctions.h"
#include "integrator_ioStructures.h"
#include <fstream>
#include <vector>
#include <map>
#include <cassert>
#include <iostream>

INT_PARAMETERS* CreateParametersFromFile(string filename)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    vector<INT_EVENT> tmpEvts ;

    map<string,int> keywords ;
    keywords[string("EVT")] = 1;
    keywords[string("PAR")] = 2;

    map<string,int> kwds_evtval ;
    kwds_evtval[string("VAL")]   = 1;
    kwds_evtval[string("ONOFF")] = 2;

    map<string,int> kwds_param ;
    kwds_param[string("RDN")] = 1;
    kwds_param[string("URT")] = 2;
    kwds_param[string("CBL")] = 3;
    kwds_param[string("SPK")] = 4;
    kwds_param[string("DT")]  = 5;

    std::ifstream input(filename.c_str());
    if (!input) return NULL ;

    INT_PARAMETERS params ;
// Temporary structure used to keep global parameters (no event)
// because we cannot allocate the full structure before knowing
// the number of events and we want to allocate the structure by
// using the allocate_params function.

    while(input.good()) {
        string keyword ;
        input >> keyword ;

        switch (keywords[keyword]) {
            case 1 : // EVT
                {

                    INT_EVENT dummy_evt ;
                    tmpEvts.push_back(dummy_evt);
                    INT_EVENT& tmpevt = tmpEvts.back();

                    long id       ; input >> id     ; tmpevt.id = id;
                    double time   ; input >> time   ; tmpevt.time = time;
                    string valtyp ; input >> valtyp ;

                    cout<<" id:"<<id<<" "<<flush;
                    cout << "time:" << time << " " << flush ;

                    switch (kwds_evtval[valtyp]){
                        case 1 : // VAL
                            {
                                double value ; input >> value ; tmpevt.value = value ;
                                cout << "value:" << value << " " << flush ;
                                break ;
                            }
                        case 2 : // ONOFF
                            {
                                int value ; input>>value; tmpevt.on = (value==1);
                                cout << "ON/OFF:"<<tmpevt.on<<flush;
                                break ;
                            }
                        default:
                            {
//the key is unknown
                                cout << " Unknown key : " << valtyp << "  " ;
//TODO? go to next line...
                            }
                    }
                    break;
                }

            case 2 : // PAR
                {
                    string key ; input >> key ;
                    switch(kwds_param[key]) {
                        case 1: // RDN
                            {
                                int value ; input >> value ;
                                params.ReduceNoise = (value==1);
                                cout << " Noise reduction : " << params.ReduceNoise<<flush;
                                break;
                            }
                        case 2: // URT
                            {
                                int value ; input >> value ;
                                params.UseRelativeThreshold = (value==1);
                                cout << " Use relative threshold : " << params.UseRelativeThreshold<<flush;
                                break;
                            }
                        case 3: // CBL
                            {
                                int value ; input >> value ;
                                params.ComputeBaselines = (value==1);
                                cout<<" Compute baselines : "<<params.ComputeBaselines<<flush ;
                                break;
                            }
                        case 4: // SPK
                            {
                                double value ; input >> value ;
                                params.SpikeReduction = value ;
                                cout<<" Spike reduction : "<<params.SpikeReduction<<flush ;
                                break;
                            }
                        case 5: // DT
                            {
                                double value ; input >> value ;
                                params.DeltaTime = value ;
                                cout<<" Delta T : "<<params.DeltaTime<<flush ;
                                break;
                            }
                        default:
                            {
                                cout << " Unknown key : " << key ;
                                break;
                            }
                    }
                    break;
                }
            default:
                {
//the key is unknown
                    cout << " Unknown key : " << keyword << "  " ;
//TODO? go to next line...
                }

        }
        cout << endl ;
    }

//We have the events. Now, we put them in the input structure
    INT_PARAMETERS* returnedParams       = Alloc_Parameters(tmpEvts.size());

    returnedParams->ReduceNoise          = params.ReduceNoise ;
    returnedParams->UseRelativeThreshold = params.UseRelativeThreshold ;
    returnedParams->ComputeBaselines     = params.ComputeBaselines ;
    returnedParams->SpikeReduction       = params.SpikeReduction ;
    returnedParams->DeltaTime            = params.DeltaTime ;

    for(unsigned int idxevt=0; idxevt<tmpEvts.size(); ++idxevt)
    {
        returnedParams->events[idxevt] = tmpEvts[idxevt] ;
    }

    return returnedParams ;
}

void FormatResults(INT_RESULTS& results, ostream& out)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    out<<"PEAKS :"<<endl;
    for(int idxpeak=0; idxpeak<results.NumberOfPeaks; ++idxpeak)
    {
        INT_RESULT_PEAK& curpeak = results.Peaks[idxpeak] ;
        out<<"#"<<idxpeak+1
	   <<" \tstart     : "<<curpeak.StartTime <<endl
	   <<" \tRT        : "<<curpeak.RetentionTime<<endl
	   <<" \tstop      : " <<curpeak.EndTime <<endl
	   <<" \theight    : "<<curpeak.Height <<endl
	   <<" \tBL_Height : "<<curpeak. BaseLineHeight_AtRetTime<<endl
	   <<" \tarea      : "<<curpeak.Area <<endl
	   <<" \ttime      : "<<curpeak.RetentionTime<<endl
	   <<" \tlft inf pt: "<<curpeak.LeftInflexionPointTime<<endl
	   <<" \trgt inf pt: "<<curpeak.RightInflexionPointTime<<endl
	   <<" \tlft err   : "<<curpeak.LeftInflexionPointErrorCode<<endl
	   <<" \trgt err   : "<<curpeak.RightInflexionPointErrorCode<<endl
	   <<" \tlft tg A  : "<<curpeak.LeftTgCoeffA<<endl
	   <<" \tlft tg B  : "<<curpeak.LeftTgCoeffB<<endl
	   <<" \trgt tg A  : "<<curpeak.RightTgCoeffA<<endl
	   <<" \trgt tg B  : "<<curpeak.RightTgCoeffB<<endl
	  
	   <<" \tasy phr eu: "<<curpeak.ASYMETRY_PHARMACOP_EUROP<<endl
	   <<" \tasy usp.. : "<<curpeak.ASYMETRY_USP_EMG_EP_ASTM<<endl
	   <<" \tasy hw 44 : "<<curpeak.ASYMETRY_HALF_WIDTHS_44<<endl
	   <<" \tasy hw 10 : "<<curpeak.ASYMETRY_HALF_WIDTHS_10<<endl
	  
	   <<" \tint tg t1 : "<<curpeak.InterTgLb_X1<<endl
	   <<" \tint tg t2 : "<<curpeak.InterTgLb_X2<<endl
	  
	   <<" \tthpl s2   : "<<curpeak.THPLATES_SIGMA2<<endl
	   <<" \tthpl s3   : "<<curpeak.THPLATES_SIGMA3<<endl
	   <<" \tthpl s4   : "<<curpeak.THPLATES_SIGMA4<<endl
	   <<" \tthpl s5   : "<<curpeak.THPLATES_SIGMA5<<endl
	   <<" \tthpl usp  : "<<curpeak.THPLATES_USP<<endl
	   <<" \tthpl astm : "<<curpeak.THPLATES_EP_ASTM<<endl
	   <<" \tthpl emg  : "<<curpeak.THPLATES_EMG<<endl
	   <<" \tthpl areah: "<<curpeak.THPLATES_AREAHEIGHT<<endl
	  
	   <<" \twd base   : "<<curpeak.Width_TgBase<<endl
	   <<" \twd 4.4%   : "<<curpeak.Width_4p4<<endl
	   <<" \twd 5.0%   : "<<curpeak.Width_5p<<endl
	   <<" \twd 10%    : "<<curpeak.Width_10p<<endl
	   <<" \twd 13.4%  : "<<curpeak.Width_13p4<<endl
	   <<" \twd 32.4%  : "<<curpeak.Width_32p4<<endl
	   <<" \twd 50%    : "<<curpeak.Width_50p<<endl
	   <<" \twd 60.7%  : "<<curpeak.Width_60p7<<endl
	  
	   <<" \tmoment 1  : "<<curpeak.Moment1<<endl
	   <<" \tmoment 2  : "<<curpeak.Moment2<<endl
	   <<" \tmoment 3  : "<<curpeak.Moment3<<endl
	   <<" \tmoment 4  : "<<curpeak.Moment4<<endl
	   <<" \tskew      : "<<curpeak.Skew<<endl
	   <<" \tkurtosis  : "<<curpeak.Kurtosis<<endl

	   <<" \tlwd 4.4%  : "<<curpeak.Left_Width_4p4<<endl
	   <<" \tlwd 5%    : "<<curpeak.Left_Width_5p<<endl
	   <<" \tlwd 10%   : "<<curpeak.Left_Width_10p<<endl
	   <<" \tlwd 13.4% : "<<curpeak.Left_Width_13p4<<endl
	   <<" \tlwd 32.4% : "<<curpeak.Left_Width_32p4<<endl
	   <<" \tlwd 50%   : "<<curpeak.Left_Width_50p<<endl
	   <<" \tlwd 60.7% : "<<curpeak.Left_Width_60p7<<endl

	   <<endl;
    }

    out<<endl;
    out<<"LINES :"<<endl;
    for(int idxline=0; idxline<results.NumberOfLines; ++idxline)
    {
        INT_RESULT_LINE& curline = results.Lines[idxline] ;
        out<<"#"<<idxline+1
	   <<" \tstart:"<<curline.StartTime 
	   <<" \tstop:" <<curline.EndTime 
	   <<" \thbeg:" <<curline.StartValue
	   <<" \thend:" <<curline.EndValue
	   <<endl;
    }

}
