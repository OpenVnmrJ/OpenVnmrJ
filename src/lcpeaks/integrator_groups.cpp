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

// $History: integrator_groups.cpp $
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
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#include "common.h"
#include <cmath>
#include <cassert>
#include "integrator_groups.h"
#include "GeneralLibrary.h"

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

bool TPeaksToSkim::TestDaughterPeak(TCandidatePeak& MotherPeak,
    TCandidatePeak& DaughterPeak,
    INT_FLOAT ratio)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT HeightBase;
    if (FIncrement == 1)
        HeightBase = FSignal[DaughterPeak.StartIndex] ;
    else
        HeightBase = FSignal[DaughterPeak.EndIndex]   ;

    INT_FLOAT HeightMother = MaxHeight(MotherPeak,HeightBase) ;
    INT_FLOAT HeightDaughter = MaxHeight(DaughterPeak,HeightBase) ;

    bool result ;
    if (HeightDaughter > 0)
        result = (HeightMother/HeightDaughter) >= ratio ;
    else
        result = false ;
    return result ;
}

/*------------------------------------------------------------
      FUNCTION:    TPeaksToSkim.TestPlausibleDaughterPeak
  AUTHOR:    BO
  DATE:      16/02/00 18:00:07
  PURPOSE:   determine wheter a peak may be a daughter peak
             on the basis of a simple geometrical criterion.
	     Should work for both positive and negative peaks
  HISTORY:   BO 15/11/01 This criterion did not work for peaks with
             negatives values at start/end. See DT4838.
	     GO 05.06.02 : translated into C++
    ------------------------------------------------------------*/
bool TPeaksToSkim::TestPlausibleDaughterPeak(TCandidatePeak& DaughterPeak)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT Hs = (FSignal[DaughterPeak.StartIndex]) ;
    INT_FLOAT He = (FSignal[DaughterPeak.EndIndex]) ;
    int diffsign = sign<INT_FLOAT>(Hs-He);
    int PeakSign = (DaughterPeak.FisPositive)?1:-1 ;

    return ( (FIncrement ==  1) && ( diffsign ==  PeakSign )
        ||
        (FIncrement == -1) && ( diffsign == -PeakSign ) );
}

INT_FLOAT TPeaksToSkim::MaxHeight(TCandidatePeak& peak,
    INT_FLOAT base)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    INT_FLOAT maxH = INT_MINFLOAT ;
    for (int idxSignal = peak.StartIndex ;
    idxSignal <= peak.EndIndex;
    idxSignal++)
    {
        INT_FLOAT val = fabs(FSignal[idxSignal]) ;
        if (val > maxH) maxH = val ;
    }
    return fabs(maxH-fabs(base)) ;
}

void TPeaksToSkim::ProcessSubGroup(TCandidatePeak& MotherPeak,
    TCandidatePeakList& Daughters)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(Daughters.size() >= 1) ;

    TCandidatePeak& lastPeak  = *(Daughters.back()) ;

    MotherPeak.FisMother = true ;
// extend limits in order to include daughter peaks
// remark: maybe we should forbid this in case the mother peak
// has fixed limits
    if (FIncrement == 1)
    {
        MotherPeak.EndIndex = lastPeak.EndIndex ;
// detach the peak from its daughters
        MotherPeak.FFusedEnd = false ;
        MotherPeak.FEndIsValley = false ;

        TCandidatePeak& tmpPeak = *(Daughters.front());
        tmpPeak.FFusedStart = false ;
        tmpPeak.FStartIsValley = false ;
    }
    else
        {
        MotherPeak.StartIndex = lastPeak.StartIndex ;
// detach the peak from its daughters
        MotherPeak.FFusedStart = false ;
        MotherPeak.FStartIsValley = false ;

        TCandidatePeak& tmpPeak = *(Daughters.front());
        tmpPeak.FFusedEnd = false ;
        tmpPeak.FEndIsValley = false ;
    }

// process daughter list
    MotherPeak.pFDaughters = new TCandidatePeakList;
    for(TCandidatePeakList::iterator peak_it = Daughters.begin();
    peak_it!=Daughters.end();
    peak_it++)
    {
        TCandidatePeak& curDaughter = **peak_it ;
        curDaughter.FHasEventBaseLine = false;
        curDaughter.FisDaughter = true ;
        curDaughter.FmyLineType = FlineType ;
        curDaughter.SetSkimmingCodes(FlineType) ;
        MotherPeak.pFDaughters->push_back(&curDaughter) ;
    }

    TCandidatePeak& curDaughter = *(Daughters.back()); ;
    if (FIncrement == 1)
    {
        curDaughter.FFusedEnd = false ;
        curDaughter.FEndIsValley = false ;
        pFIntegrator->PropagateValleyChanges(curDaughter,curDaughter) ;
    }
    else
        {
        curDaughter.FFusedStart = false ;
        curDaughter.FStartIsValley = false ;
        pFIntegrator->PropagateValleyChanges(curDaughter,curDaughter) ;
    }
}

bool TPeaksToSkim::CheckSubGroup(TCandidatePeak& MotherPeak,
    TCandidatePeakList& Daughters)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
// check if subgroup can be processed
// it must have at least 1 daughter
// and all peaks involved must have horizontal baselines if they have event baselines
    bool result ;
    if (Daughters.size() == 0)
        result = false ;
    else
        if (MotherPeak.FHasEventBaseLine != MotherPeak.FEventLineIsHorizontal)
//xor translated by != (same logic table IN CASE OF BOOLEAN - to compare ints it
//could be better to translate (a xor b) by (!a != !b). GO.
        result = false ;
    else
        if (!TestPlausibleDaughterPeak( *(Daughters.back()) ) )
            result = false ;
    else
        {
        result = true ;
        for(TCandidatePeakList::iterator peak_it = Daughters.begin();
        peak_it!=Daughters.end();
        peak_it++)
        {
            TCandidatePeak& curPeak = **peak_it ;
            result = result && (!(curPeak.FHasEventBaseLine != curPeak.FEventLineIsHorizontal));
//see previous comment on the translation of xor. GO.
        }
    }
    return result;
}

TPeaksToSkim::TPeaksToSkim (bool directionIsRight,
    INT_SIGNAL Signal,
    INT_FLOAT deltatime,
    TBaseLineType linetype,
    TInternalIntegrator* integrator)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    assert(integrator!=NULL);

    FSignal = Signal ;
    FDeltaTime = deltatime ;
    FlineType = linetype ;
    FIncrement = directionIsRight?1:-1;
    pFIntegrator = integrator ;
}

TPeaksToSkim::~TPeaksToSkim()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
}

void TPeaksToSkim::Add(TCandidatePeak& aPeak)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FPeaks.push_back(&aPeak);
}

void TPeaksToSkim::Clear()
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    FPeaks.clear();
}

void TPeaksToSkim::Process(TTimeAndValue& SkimRatios)
/* --------------------------
     * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
    if (FPeaks.size() > 1)
    {
        TCandidatePeakList subGroup;
        TCandidatePeak* potMotherPeak = NULL ;

        for(TCandidatePeakList::iterator peak_it = FPeaks.begin();
        peak_it != FPeaks.end();
        peak_it++)
        {
            TCandidatePeak& curpeak = **peak_it ;
// process current peak
            if (potMotherPeak == NULL)
                potMotherPeak = &curpeak;
            else
            {
                INT_FLOAT ratio = SkimRatios.ValueAt(curpeak.StartIndex * FDeltaTime) ;
                if (TestDaughterPeak(*potMotherPeak,curpeak,ratio))
                    subGroup.push_back(&curpeak) ;
                else
                {
                        if (CheckSubGroup(*potMotherPeak, subGroup))
                        {
                            ProcessSubGroup(*potMotherPeak, subGroup) ;
                        }
                        subGroup.clear() ;
                        potMotherPeak = &curpeak ;
                }
            }
        }
        if (CheckSubGroup(*potMotherPeak, subGroup)) ProcessSubGroup(*potMotherPeak, subGroup) ;
    }
}

