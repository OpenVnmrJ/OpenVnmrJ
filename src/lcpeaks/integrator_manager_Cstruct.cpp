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

// $History: integrator_manager_Cstruct.cpp $
/*  */
/* *****************  Version 4  ***************** */
/* User: Go           Date: 4/11/02    Time: 11:54 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DT6566 */
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 1  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:57 */
/* Created in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */

#include "common.h"
#include <string.h>
#include <math.h>
#include "GeneralLibrary.h"
#include "integrator_manager_Cstruct.h"
#include "integrator_abstract_manager.h"
#include "integrator_classes.h"
#include "integrator_calcul.h"

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

void Smooth(INT_SIGNAL signal, int size, INT_SIGNAL smoothed, INT_FLOAT factor) {
/* --------------------------
     * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose :
 * History :
 * -------------------------- */

    INT_FLOAT dummy ;
    SmoothWithWaveletsProc(
        signal,
        size,
        smoothed,
        factor,
        dummy);
}

bool CheckLimits(int EventCode , INT_FLOAT value , char* error) {
/* --------------------------
     * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose :
 * History :
 * -------------------------- */

    bool result = true ;
    string ErrorDesc("Value must be in interval ") ;

    switch (EventCode) {
        case INTEGRATIONEVENT_SETPEAKWIDTH :
            {
                if ((value >= 0) && (value <=99999)) {
                    result = true ;
                    } else {
                    ErrorDesc += "[0; 99999]" ;
                    result = false ;
                }
                break;
            }

        case INTEGRATIONEVENT_SETTHRESHOLD:
            {
                if ((value >= 0) && (value <=1.0E7)) {
                    result = true;
                    } else {
                    result = false ;
                    ErrorDesc +=  "[0; 10000000]" ;
                }
                break;
            }

        case INTEGRATIONEVENT_SETTHRESHOLD_SOLVENT:
            {
                if ((value >= 0) && (value <=1.0E8)) {
                    result = true;
                    } else {
                    result = false ;
                    ErrorDesc += "[0; 100000000]" ;
                }
                break;
            }

        case INTEGRATIONEVENT_SETSKIMRATIO:
            {
                if ((value > 0) && (value <=1.0E6)) {
                    result = true;
                    } else {
                    result = false ;
                    ErrorDesc += "[0; 100000]" ;
                }
                break;
            }

        case INTEGRATIONEVENT_MIN_AREA_PERCENT :
            {
                if ((value >= 0) && (value <=100)) {
                    result = true ;
                    } else {
                    result = false ;
                    ErrorDesc += "[0; 100]" ;
                }
                break;
            }

        case INTEGRATIONEVENT_MIN_HEIGHT_PERCENT :
            {
                if ((value >= 0) && (value <=100)) {
                    result = true ;
                    } else {
                    result = false ;
                    ErrorDesc += "[0; 100]" ;
                }
                break;
            }

        case INTEGRATIONEVENT_MIN_AREA :
            {
                if ((value >= 0) && (value <=1.0E8)) {
                    result = true ;
                    } else {
                    result = false ;
                    ErrorDesc += "[0; 100000000]" ;
                }
                break;
            }

        case INTEGRATIONEVENT_MIN_HEIGHT :
            {
                if ((value >= 0) && (value <=1.0E8)) {
                    result = true ;
                    } else {
                    result = false ;
                    ErrorDesc += "[0; 100000000]" ;
                }
                break;
            }

    }
    if (result) {
        error = NULL ;
        } else {
        strncpy(error,ErrorDesc.c_str(),256);
    }
    return result;
}

INT_RESULTS* Integrate(INT_SIGNAL signal, int signalsize, INT_PARAMETERS* params)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    TIntegration_Manager_Cstruct manager( 
        signal,
        signalsize,
        params);

    manager.integrate();
    INT_RESULTS* pResults = Alloc_Results( manager.NbLines(), manager.NbPeaks(), manager.NbAutoThresholds() );
    manager.FillResults(pResults) ;

    return pResults ;
}

void UpdatePeakProperties(INT_SIGNAL signal, int signalsize, INT_PARAMETERS* params, INT_RESULTS* res)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : october 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    TIntegration_Manager_Cstruct manager( 
        signal,
        signalsize,
        params);

    manager.SetResults(res);
    manager.RecomputePeakProperties();
    manager.FillResults(res) ;
}

/***********************************************************************************/

void TIntegration_Manager_Cstruct::RecomputePeakProperties()
/* --------------------------
 * Author  : Gilles Orazi
 * Created : Oct. 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT 
        totArea, 
        totHeight ;

    FIntegrator.Initialize() ; // smooth signal and compute derivatives
    FIntegrator.ComputePeaksProperties(totArea, totHeight);
    FIntegrator.CorrectForSkimming();

    //compute %area and %height using totals computed
    //before skimming corrections. 
    //Is it a BUG ?
    //I keep it like that since it was done this way in
    //the Delphi code, before the translation.
    //GO. 15.10.02
    for(int peakidx = 0 ;
        peakidx<FIntegrator.Peaks().size();
        ++peakidx)
    {
        TCandidatePeak& curpeak = FIntegrator.Peaks()[peakidx] ;
        curpeak.pcArea   = 100.0 * curpeak.Area   / totArea ;
        curpeak.pcHeight = 100.0 * curpeak.Height / totHeight ;
     }
}
                                                           
TIntegration_Manager_Cstruct::TIntegration_Manager_Cstruct(INT_SIGNAL _signal,
    int _signalsize,
    INT_PARAMETERS* _params)
:TAbstract_Integration_Manager(_signal,_signalsize)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FParams = _params;
    GetParameters();
}

void TIntegration_Manager_Cstruct::integrate()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    Process_Integration();
}

void TIntegration_Manager_Cstruct::GetParameters()
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    //setting global parameters
    ReduceNoise(FParams->ReduceNoise) ;
    UseRelativeThreshold(FParams->UseRelativeThreshold) ;
    ComputeBaselines(FParams->ComputeBaselines) ;
    SpikeReduction(FParams->SpikeReduction) ;
    DeltaTime(FParams->DeltaTime) ;
    DeadTime(FParams->DeadTime) ;
    PeakSat_Max(FParams->PeakSat_Max);
    PeakSat_Min(FParams->PeakSat_Min);

    //reading event list
    ClearEventList();
    for (int idxevent = 0; idxevent<FParams->NumberOfEvents; ++idxevent)
    {
        StoreEvent(FParams->events[idxevent]);
    }  
}

void TIntegration_Manager_Cstruct::DeleteResults(INT_RESULTS* pResults)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    Free_Results(pResults);
}

void TIntegration_Manager_Cstruct::SetResults(INT_RESULTS* pResults)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : Oct. 2002
 * Purpose :
 * History :
 * -------------------------- */
{
    for(int lineidx = 0;
        lineidx<pResults->NumberOfLines;
        ++lineidx)
    {
        TCandidateBaseLine& curline = FIntegrator.Lines().push_back();
        INT_RESULT_LINE& structline = pResults->Lines[lineidx];
        curline.StartTime = structline.StartTime;
        curline.EndTime = structline.EndTime;
        curline.StartValue = structline.StartValue;
        curline.EndValue = structline.EndValue;
        curline.type = TBaseLineType(structline.LineType);
    }

    int peakidx ;
    for(peakidx = 0 ;
        peakidx<pResults->NumberOfPeaks;
        ++peakidx)
    {
        TCandidatePeak& curpeak = FIntegrator.Peaks().push_back() ;
        INT_RESULT_PEAK& structPeak = pResults->Peaks[peakidx];

        curpeak.StartTime = structPeak.StartTime ;
        curpeak.RetentionTime = structPeak.RetentionTime ;
        curpeak.EndTime = structPeak.EndTime ;
        curpeak.BaseLineHeight_AtRetTime = structPeak.BaseLineHeight_AtRetTime;
        curpeak.MotherIndex  = structPeak.MotherIdx ;
        curpeak.StartIndex = (int)ceil(curpeak.StartTime / FParams->DeltaTime);
        curpeak.EndIndex = min( (int)(floor(curpeak.EndTime /FParams->DeltaTime)), (signal_size()-1));
        //Note : StartIndex and EndIndex are recomputed. This may be 
        //       unstable because the results of the divisions should
        //       very often be "equal" to an integer. 
        //       It could be better to keep the index.
        //GO (during translation) 
        curpeak.code = string(structPeak.code) ;

        if (structPeak.BaselineIdx>-1)
        {
            curpeak.pFmyLine = &(FIntegrator.Lines()[structPeak.BaselineIdx]) ;
        }
        else
        {
            curpeak.pFmyLine = NULL ;
        }

    }

    for(peakidx = 0 ;
        peakidx < FIntegrator.Peaks().size();
        ++peakidx)
    {
        TCandidatePeak& curpeak = FIntegrator.Peaks()[peakidx] ;
        if (curpeak.MotherIndex>-1)
        {
            TCandidatePeak& Mother = (FIntegrator.Peaks())[curpeak.MotherIndex] ;
            if (Mother.pFDaughters == NULL)
                Mother.pFDaughters = new TCandidatePeakList ;
            Mother.pFDaughters->push_back(&curpeak);
        }
    }

}

void TIntegration_Manager_Cstruct::FillResults(INT_RESULTS* pResults)
/* --------------------------
 * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    pResults->NoiseSdev = FIntegrator.FnoiseSDev ;
    pResults->Noise = FIntegrator.FNoiseComputer.GetNoise() ;
    pResults->RMSNoise = FIntegrator.FNoiseComputer.GetRMSNoise() ;
    pResults->Drift = FIntegrator.FNoiseComputer.GetDrift() ;

    for(int peakidx = 0 ;
    peakidx<FIntegrator.Peaks().size();
//peakidx < curline.FAssociatedPeaks.size();
        ++peakidx)
    {
//TCandidatePeak& curpeak = *(curline.FAssociatedPeaks[peakidx]) ;
        TCandidatePeak& curpeak = FIntegrator.Peaks()[peakidx] ;
        INT_RESULT_PEAK* pStructPeak = (pResults->Peaks)+peakidx;
        INT_RESULT_PEAK& structPeak = *pStructPeak ;

        structPeak.StartTime = curpeak.StartTime;
        structPeak.RetentionTime = curpeak.RetentionTime;
        structPeak.EndTime = curpeak.EndTime;
        structPeak.Area = curpeak.Area;
        structPeak.Height = curpeak.Height;
        structPeak.pcArea = curpeak.pcArea;
        structPeak.pcHeight = curpeak.pcHeight;
        structPeak.isUserSlice = curpeak.isUserSlice ;
        structPeak.LeftInflexionPointTime = curpeak.LeftInflexionPointTime;
        structPeak.RightInflexionPointTime  = curpeak.RightInflexionPointTime;
        structPeak.LeftInflexionPointErrorCode  = curpeak.LeftInflexionPointErrorCode;
        structPeak.RightInflexionPointErrorCode  = curpeak.RightInflexionPointErrorCode;
        structPeak.LeftTgCoeffA  = curpeak.LeftTgCoeffA;
        structPeak.LeftTgCoeffB  = curpeak.LeftTgCoeffB;
        structPeak.RightTgCoeffA  = curpeak.RightTgCoeffA;
        structPeak.RightTgCoeffB  = curpeak.RightTgCoeffB;
        structPeak.InterTgLb_X1 = curpeak.InterTgLb_X1;
        structPeak.InterTgLb_X2 = curpeak.InterTgLb_X2;
        structPeak.PrevValleyHeight = curpeak.PrevValleyHeight;
        structPeak.ASYMETRY_PHARMACOP_EUROP = curpeak.ASYMETRY_PHARMACOP_EUROP ;
        structPeak.ASYMETRY_USP_EMG_EP_ASTM = curpeak.ASYMETRY_USP_EMG_EP_ASTM;
        structPeak.ASYMETRY_HALF_WIDTHS_44 = curpeak.ASYMETRY_HALF_WIDTHS_44;
        structPeak.ASYMETRY_HALF_WIDTHS_10 = curpeak.ASYMETRY_HALF_WIDTHS_10;
        structPeak.THPLATES_SIGMA2 = curpeak.THPLATES_SIGMA2;
        structPeak.THPLATES_SIGMA3 = curpeak.THPLATES_SIGMA3;
        structPeak.THPLATES_SIGMA4 = curpeak.THPLATES_SIGMA4;
        structPeak.THPLATES_SIGMA5 = curpeak.THPLATES_SIGMA5;
        structPeak.THPLATES_USP = curpeak.THPLATES_USP;
        structPeak.THPLATES_EP_ASTM = curpeak.THPLATES_EP_ASTM;
        structPeak.THPLATES_EMG = curpeak.THPLATES_EMG;
        structPeak.THPLATES_AREAHEIGHT = curpeak.THPLATES_AREAHEIGHT;
        structPeak.Width_TgBase = curpeak.Width_TgBase;
        structPeak.Width_4p4 = curpeak.Width_4p4;
        structPeak.Width_5p = curpeak.Width_5p;
        structPeak.Width_10p = curpeak.Width_10p;
        structPeak.Width_13p4 = curpeak.Width_13p4;
        structPeak.Width_32p4 = curpeak.Width_32p4;
        structPeak.Width_50p = curpeak.Width_50p;
        structPeak.Width_60p7 = curpeak.Width_60p7;
        structPeak.Moment1 = curpeak.Moment1;
        structPeak.Moment2 = curpeak.Moment2;
        structPeak.Moment3 = curpeak.Moment3;
        structPeak.Moment4 = curpeak.Moment4;
        structPeak.Skew = curpeak.Skew;
        structPeak.Kurtosis = curpeak.Kurtosis;
        structPeak.Left_Width_4p4 = curpeak.Left_Width_4p4;
        structPeak.Left_Width_5p = curpeak.Left_Width_5p;
        structPeak.Left_Width_10p = curpeak.Left_Width_10p;
        structPeak.Left_Width_13p4 = curpeak.Left_Width_13p4;
        structPeak.Left_Width_32p4 = curpeak.Left_Width_32p4;
        structPeak.Left_Width_50p = curpeak.Left_Width_50p;
        structPeak.Left_Width_60p7 = curpeak.Left_Width_60p7;
        structPeak.BaseLineHeight_AtRetTime = curpeak.BaseLineHeight_AtRetTime;
        structPeak.Resolution_HalfWidth = curpeak.Resolution_HalfWidth ;
        structPeak.Resolution_USP = curpeak.Resolution_USP ;
        structPeak.Resolution_Valleys = curpeak.Resolution_Valleys ;
        structPeak.Capacity = curpeak.Capacity ;
        structPeak.Selectivity = curpeak.Selectivity ;


        delete [] structPeak.code ; //
        structPeak.code = new char[curpeak.code.length()+1] ;
        strcpy(structPeak.code, curpeak.code.c_str());

        structPeak.BaselineIdx =  FIntegrator.Lines().indexof(*(curpeak.pFmyLine)) ;
        structPeak.MotherIdx = curpeak.MotherIndex ;
    }

    for(int lineidx = 0;
    lineidx<FIntegrator.Lines().size();
    ++lineidx)
    {
        TCandidateBaseLine& curline = FIntegrator.Lines()[lineidx];
        INT_RESULT_LINE* pStructline = (pResults->Lines)+lineidx;
        INT_RESULT_LINE& structline = *pStructline ;

        structline.StartTime = curline.StartTime;
        structline.EndTime = curline.EndTime;
        structline.StartValue = curline.StartValue;
        structline.EndValue = curline.EndValue;
        structline.LineType = curline.type ;
    }

    int idxatr = 0 ;
    for (vector<TAutoThresholdResult>::iterator atrit = FIntegrator.AutoThresholdResults.begin() ;
        atrit != FIntegrator.AutoThresholdResults.end();
        ++atrit )
        {
            TAutoThresholdResult& atr = *atrit ;
            INT_RESULT_AUTOTHRESHOLD* atr_struct = (pResults->AutoThresholds)+idxatr ;
            atr_struct->RelativeThreshold = atr.RelativeThreshold ;
            atr_struct->Threshold = atr.Threshold ;
            atr_struct->Time = atr.Time ;
            ++idxatr;
        }
}

void TIntegration_Manager_Cstruct::StoreEvent(INT_EVENT& aevent)
/* --------------------------
     * Author  : Gilles Orazi
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    TIntegrationEvent* newevent = FEventsFactory.CreateEvent(aevent.id,
        aevent.time,
        aevent.value,
        aevent.on);
    if(newevent!=NULL) events().push_back(newevent);
}


