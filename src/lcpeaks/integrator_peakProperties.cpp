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
#include <cmath>
#include <cassert>
#include <algorithm>
#include "debug.h"
#include "integrator_peakProperties.h"
#include "integrator_lineTools.h"
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

const INT_FLOAT EPSILON_ZERO  =  1.0E-9 ;

PEAK_COMPUTER::PEAK_COMPUTER(
	INT_SIGNAL Signal,
	int SignalSize,
	INT_SIGNAL SignalSmooth,
	INT_FLOAT DeltaTimeMinutes)
/* --------------------------
	 * Author  : Bruno Orsier                : Original Delphi code
             Diego Segura / Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
	m_signal = Signal;
	m_signalsize = SignalSize;
	m_signalSmooth = SignalSmooth;
	m_deltaTimeMinutes = DeltaTimeMinutes;
	m_firstDeriv = NULL ;
	Reset();
}

PEAK_COMPUTER::~PEAK_COMPUTER()
/* --------------------------
	 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
	delete [] m_firstDeriv ;
}

void PEAK_COMPUTER::Integrate(TCandidatePeak& aPeak)
/* --------------------------
	 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose : Compute the properties of the peak
 *           EndIndex, StartIndex,
 * History :
 * -------------------------- */
{
	pCurpeak = &aPeak ;

	aPeak.StartTime = aPeak.StartIndex * m_deltaTimeMinutes ; //+ m_deltaTimeMinutes ;
	aPeak.EndTime   = aPeak.EndIndex   * m_deltaTimeMinutes ; //+ m_deltaTimeMinutes ;

// peaks reduced to a single point are allowed here
	if (pCurpeak->EndIndex  > pCurpeak->StartIndex)
	{
		Initialize();

// but the following computations are not meaningful for peak reduced to a single point
		if ((pCurpeak->EndIndex - pCurpeak->StartIndex) > 0)
		{
			LocateInflexionPoints();
			ComputeAsymetry();
			ComputePlates();
		}
	}
	else
		{
		Reset();
	}
}

void PEAK_COMPUTER::ComputeWidthsAtHeight(INT_FLOAT HeightRatio,
	INT_FLOAT& HalfWidthLeft,
	INT_FLOAT& HalfWidthRight)
/* --------------------------
	 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History : BO 08/01/2001 Fixed DT3625 (a value that should be <=0
	     *           take sometimes very small positive values like 4E-15)
 * -------------------------- */
{
	const INT_FLOAT MAX_ALLOWED_CORRECTION = 1E-8; //DT3625

	INT_FLOAT
	correctionRight,
	correctionLeft,
	absoluteHeight;
	bool
	stop;
	INT_FLOAT
	v,
	pv;
	int
	curPositionLeft,
	curPositionRight;

	assert((HeightRatio >= 0) && (HeightRatio <= 1.0));
	assert((pCurpeak->RetentionTime >= 0)); // integrate must have been called before using ComputeWidthsAtHeight

	absoluteHeight = pCurpeak->BaseLineHeight_AtRetTime + pCurpeak->Height * HeightRatio;

	stop = false;
	curPositionLeft = pCurpeak->ApexIndex;
	v = m_signal[curPositionLeft] - absoluteHeight;

	while ((curPositionLeft > pCurpeak->StartIndex) && !stop)
	{
		--curPositionLeft;
		pv = v;
		v = m_signal[curPositionLeft] - absoluteHeight;
		stop = pv * v < 0;
	}

// we know now that there is an intersection between the horizontal line y(x) =  absoluteHeight
// and the signal inside [curPositionLeft, curPositionLeft+1]
// we now locate more precisely the intersection point
// also, we take into account that pCurpeak->StartIndex may not correspond exactly to the
// the peak start time due to rouding effects.

	if (stop &&
		(fabs(m_signal[curPositionLeft+1] - m_signal[curPositionLeft]) > 0))
	{
		correctionLeft = (absoluteHeight - m_signal[curPositionLeft]) / (m_signal[curPositionLeft + 1] - m_signal[curPositionLeft]);
	}
	else
		{
		if (curPositionLeft==pCurpeak->StartIndex)
		{
			correctionLeft = (pCurpeak->StartTime-pCurpeak->StartIndex*m_deltaTimeMinutes) / m_deltaTimeMinutes;
			assert((correctionLeft <= 0) || (fabs(correctionLeft) <= MAX_ALLOWED_CORRECTION)); //DT3625
			if (correctionLeft > 0) correctionLeft = 0; //DT3625
		}
		else
			{
			correctionLeft = 0;
		}
	}

	HalfWidthLeft = (pCurpeak->ApexIndex - curPositionLeft - correctionLeft) * m_deltaTimeMinutes;

	stop = false;
	curPositionRight = pCurpeak->ApexIndex;
	v = m_signal[curPositionRight] - absoluteHeight;

	while ((curPositionRight < pCurpeak->EndIndex) && !stop)
	{
		++curPositionRight;
		pv = v;
		v = m_signal[curPositionRight] - absoluteHeight;
		stop = pv * v < 0;
	}

// same remark as above
	if (stop &&
		(fabs(m_signal[curPositionRight] - m_signal[curPositionRight-1]) > 0))
	{
		correctionRight = 1 - (absoluteHeight - m_signal[curPositionRight-1]) / (m_signal[curPositionRight] - m_signal[curPositionRight-1]);
	}
	else
		{
		if (curPositionRight==pCurpeak->EndIndex)
		{
			correctionRight = (pCurpeak->EndIndex * m_deltaTimeMinutes - pCurpeak->EndTime) / m_deltaTimeMinutes;
			assert((correctionRight <= 0) || (fabs(correctionRight) <= MAX_ALLOWED_CORRECTION)); //DT3625
			if (correctionRight > 0) correctionRight = 0; // DT3625
		}
		else
			{
			correctionRight = 0;
		}
	}

	HalfWidthRight = (curPositionRight - pCurpeak->ApexIndex - correctionRight) * m_deltaTimeMinutes;
}

void PEAK_COMPUTER::Reset()
/* --------------------------
	 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
	delete [] m_firstDeriv ;
	m_firstDeriv = NULL ;
	PrevValleyHeight = 0 ;
}

void PEAK_COMPUTER::Initialize()
/* --------------------------
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
	INT_FLOAT darea ;
	INT_FLOAT maxh ;
	int posmaxh;
	INT_FLOAT
	areaBeg, areaEnd, area,
	mt1, mt2,
	m4, m4Beg, m4End,
	m3, m3Beg, m3End,
	m2, m2Beg, m2End,
	m1, m1Beg, m1End,
	curh,curTime, prevTime,
	curTrueh;


	TCandidateBaseLine* pCurLine = pCurpeak->pFmyLine ;

    //The indices are recomputed to match with the original delphi code. 
    //This should maybe changed ?
    //The recomputed indices are only used for the peak properties computations
    //and are restored at the end of this function.
    //This could maybe be a bit refactored but I am waiting for the answer to 
    //the previous question. 
    //
    //GO. (During translation)

    int StartIndex_Saved = pCurpeak->StartIndex ;
    int EndIndex_Saved   = pCurpeak->EndIndex ;
    long double tmpdble ;
    tmpdble = pCurpeak->StartTime/m_deltaTimeMinutes ;
    pCurpeak->StartIndex = (int) ceil(tmpdble);
    tmpdble = pCurpeak->EndTime/m_deltaTimeMinutes ;
    pCurpeak->EndIndex = (int) floor(tmpdble);

// perform basic computations
	TLineTool* plinetool ;
	if (pCurLine == NULL)
	{
		plinetool = new TStraightLineTool(0, 0, 1, 0) ; // default baseline
	}
	else
		{ //with FLine do
		switch (pCurLine->type)
		{
			case BASELINE_STRAIGHT :
				{
					plinetool = new TStraightLineTool(
						pCurLine->StartTime,
						pCurLine->StartValue,
						pCurLine->EndTime,
						pCurLine->EndValue);
					break;
				}

			case BASELINE_EXPONENTIAL :
				{
					INT_FLOAT coeffA, coeffB, coeff_t0 ;
					pCurLine->GetExponentialCoefficients(coeffA, coeffB, coeff_t0);
					plinetool = new TExpLineTool(coeffA, coeffB, coeff_t0);
					break;
				}
			default:
				{
					plinetool = NULL; // should not occur
					break;
				}
		}
	}

	assert(plinetool!=NULL);

// maxh is used to determine the retention time and is based
// on the smoothed signal
#ifdef DEBUG
	dbgstream() << "Integrating peak ("
	<< pCurpeak->StartTime
	<<"min "
	<< pCurpeak->EndTime
	<< "min) at indexes ("
	<< pCurpeak->StartIndex
	<< " "
	<< pCurpeak->EndIndex
	<< ")" << endl;
#endif

	assert(pCurpeak->EndIndex <= m_signalsize-1) ;

// first, compute retention time because it is needed for normalized moments
	curTime   = pCurpeak->StartTime;
	maxh      = m_signalSmooth[pCurpeak->StartIndex] - plinetool->ValueAtTime(curTime);
	pCurpeak->MinHeight = m_signal[pCurpeak->StartIndex];
	pCurpeak->MaxHeight = pCurpeak->MinHeight;
	posmaxh   = pCurpeak->StartIndex;
	PrevValleyHeight = fabs(maxh);

	vector<bool> ForbiddenIndexes(pCurpeak->EndIndex - pCurpeak->StartIndex + 1, true) ;
	if (pCurpeak->pFDaughters != NULL)
	{
		for (TCandidatePeakList::iterator itDaughter = (*(pCurpeak->pFDaughters)).begin();
		itDaughter != (*(pCurpeak->pFDaughters)).end();
		++itDaughter)
		{
			TCandidatePeak& curDaughter= **itDaughter ;
			replace(
				ForbiddenIndexes.begin() + curDaughter.StartIndex - pCurpeak->StartIndex ,
				ForbiddenIndexes.begin() + curDaughter.EndIndex - pCurpeak->StartIndex ,
				true,
				false );
		}
	}

	for(int i = pCurpeak->StartIndex+1; i<=pCurpeak->EndIndex; ++i)
	{
		curTime += m_deltaTimeMinutes;
		curh     = m_signalSmooth[i] - plinetool->ValueAtTime(curTime);
		curTrueh = m_signal[i];

		bool AuthorizedIndex = ForbiddenIndexes[i - pCurpeak->StartIndex];
		if (AuthorizedIndex && (fabs(curh) > fabs(maxh)) )
		{
			maxh = curh;
			posmaxh = i;
		}
		if (AuthorizedIndex)
		{
			if (curTrueh > pCurpeak->MaxHeight)
				pCurpeak->MaxHeight = curTrueh ;
			else
				if (curTrueh < pCurpeak->MinHeight)
					pCurpeak->MinHeight = curTrueh;
		}
	}

// now we must take into account the special rare case
// where the max lies at the very beginning or end of the peak
	INT_FLOAT h1 =
		m_signal[pCurpeak->StartIndex-1] +
		(pCurpeak->StartTime - (pCurpeak->StartIndex - 1) * m_deltaTimeMinutes) / m_deltaTimeMinutes * (m_signal[pCurpeak->StartIndex] - m_signal[pCurpeak->StartIndex-1])
	- plinetool->ValueAtTime(pCurpeak->StartTime);

	INT_FLOAT h2 =
		m_signal[pCurpeak->EndIndex] +
		(pCurpeak->EndTime - curTime) / m_deltaTimeMinutes * (m_signal[min(pCurpeak->EndIndex + 1, m_signalsize-1)] - m_signal[pCurpeak->EndIndex])
	- plinetool->ValueAtTime(pCurpeak->EndTime);

	if (fabs(h1) > fabs(maxh))
	{
		maxh = h1;
		posmaxh = -1;
	}
	if (fabs(h2) > fabs(maxh))
	{
		maxh = h2;
		posmaxh = -2;
	}

	if (posmaxh==-1)
	{
		pCurpeak->RetentionTime = pCurpeak->StartTime;
		pCurpeak->Height = maxh;
		pCurpeak->ApexIndex = pCurpeak->StartIndex;
	}
	else
		{
		if (posmaxh==-2)
		{
			pCurpeak->RetentionTime = pCurpeak->EndTime;
			pCurpeak->Height = maxh;
			pCurpeak->ApexIndex = pCurpeak->EndIndex;
		}
		else
			if (pCurpeak->EndIndex > pCurpeak->StartIndex)
		{
// normal Result
			pCurpeak->ApexIndex = posmaxh;
			pCurpeak->RetentionTime = posmaxh *  m_deltaTimeMinutes;
//pCurpeak->Height = m_signal[posmaxh + 1] - plinetool->ValueAtTime(pCurpeak->RetentionTime); //maxh ;
			pCurpeak->Height = m_signal[posmaxh] - plinetool->ValueAtTime(pCurpeak->RetentionTime); //maxh ;
		}
		else
			{
// very special case: pCurpeak->EndIndex = FstartIndex, in the case where the user
// has splitted a peak into very small pieces.
// the normal result is not ok since FretTime may be > pCurpeak->EndTime
			h1 = m_signal[pCurpeak->StartIndex-1] +
				(pCurpeak->StartTime - (pCurpeak->StartIndex - 1) *  m_deltaTimeMinutes) /  m_deltaTimeMinutes * (m_signal[pCurpeak->StartIndex] - m_signal[pCurpeak->StartIndex-1])
			- plinetool->ValueAtTime(pCurpeak->StartTime);

			h2 = m_signal[pCurpeak->StartIndex-1] +
				(pCurpeak->EndTime - (pCurpeak->StartIndex - 1) *  m_deltaTimeMinutes) /  m_deltaTimeMinutes * (m_signal[pCurpeak->StartIndex] - m_signal[pCurpeak->StartIndex-1])
			- plinetool->ValueAtTime(pCurpeak->EndTime);

			if (fabs(h1)>fabs(h2))
			{
				pCurpeak->ApexIndex = pCurpeak->StartIndex;
				pCurpeak->RetentionTime = pCurpeak->StartTime;
				pCurpeak->Height = h1;
			}
			else
				{
				pCurpeak->ApexIndex = pCurpeak->EndIndex;
				pCurpeak->RetentionTime = pCurpeak->EndTime;
				pCurpeak->Height = h2;
			}
		}
	}
//assert(FRetTime >= pCurpeak->StartTime);
//assert(FRetTime <= pCurpeak->EndTime);

	if (pCurpeak->StartIndex > pCurpeak->EndIndex)
	{
// special case : peak is very small and is located between two data points
		h1 = m_signal[pCurpeak->StartIndex-1] +
			(pCurpeak->StartTime - (pCurpeak->StartIndex - 1) *  m_deltaTimeMinutes) /  m_deltaTimeMinutes * (m_signal[pCurpeak->StartIndex] - m_signal[pCurpeak->StartIndex-1])
		- plinetool->ValueAtTime(pCurpeak->StartTime);

		h2 = m_signal[pCurpeak->StartIndex-1] +
			(pCurpeak->EndTime - (pCurpeak->StartIndex - 1) *  m_deltaTimeMinutes) /  m_deltaTimeMinutes * (m_signal[pCurpeak->StartIndex] - m_signal[pCurpeak->StartIndex-1])
		- plinetool->ValueAtTime(pCurpeak->EndTime);

		areaBeg = (h1 + h2) / 2.0 * (pCurpeak->EndTime - pCurpeak->StartTime);
		areaBeg = ((areaBeg<0) && (pCurpeak->isUserSlice)) ? 0.0 : areaBeg ;
		areaEnd = 0;
		area = 0;
		prevTime = pCurpeak->StartTime;
		curTime = pCurpeak->EndTime;
		mt1 = prevTime - pCurpeak->RetentionTime;
		mt2 = curTime - pCurpeak->RetentionTime;

		m1 = (h1 * mt1 + h2 * mt2) / 2.0;
		m2 = (h1 * pow(mt1,2) + h2 * pow(mt2,2)) / 2.0;
		m3 = (h1 * pow(mt1,2) * mt1 + h2 * pow(mt2,2) * mt2) / 2.0;
		m4 = (h1 * pow(mt1,2) * pow(mt1,2) + h2 * pow(mt2,2) * pow(mt2,2)) / 2.0;
		m1End = 0;
		m2End = 0;
		m3End = 0;
		m4End = 0;
		m1Beg = 0;
		m2Beg = 0;
		m3Beg = 0;
		m4Beg = 0;
	}
	else
		{
// now integrate and compute moments
		curTime = pCurpeak->StartIndex *  m_deltaTimeMinutes;
		h1 = m_signal[pCurpeak->StartIndex] - plinetool->ValueAtTime(curTime);
		if (curTime > pCurpeak->StartTime)
		{
// estimate lost area at the beginning of the peak
			h2 = m_signal[pCurpeak->StartIndex - 1] +
				(pCurpeak->StartTime - (pCurpeak->StartIndex - 1) *  m_deltaTimeMinutes) /  m_deltaTimeMinutes * (m_signal[pCurpeak->StartIndex] - m_signal[pCurpeak->StartIndex-1])
			- plinetool->ValueAtTime(pCurpeak->StartTime);

			areaBeg = (h1 + h2) / 2.0 * (curTime - pCurpeak->StartTime);
			areaBeg = ((areaBeg<0) && (pCurpeak->isUserSlice)) ? 0.0 : areaBeg ;

#ifdef DEBUG
			dbgstream() << "Integrating BEG segment ("
			<< pCurpeak->StartTime
			<< "min "
			<< curTime
			<< "min) ("
			<< h2
			<< " "
			<< h1
			<<") ->"
			<< areaBeg
			<< endl;
#endif
		}
		else
			{
			areaBeg = 0;
		}

		m1Beg = (curTime - pCurpeak->RetentionTime) * areaBeg;
		m2Beg = (curTime - pCurpeak->RetentionTime) * m1Beg;
		m3Beg = (curTime - pCurpeak->RetentionTime) * m2Beg;
		m4Beg = (curTime - pCurpeak->RetentionTime) * m3Beg;

		area = 0;
		m1 = 0;
		m2 = 0;
		m3 = 0;
		m4 = 0;
		for (int i = pCurpeak->StartIndex+1; i<=pCurpeak->EndIndex; ++i)
		{
			prevTime = curTime;
			curTime += m_deltaTimeMinutes;

			h2 = m_signal[i] - plinetool->ValueAtTime(curTime);
			darea = (h1 + h2) / 2.0;
			darea = ((darea<0) && (pCurpeak->isUserSlice)) ? 0 : darea ;
			area += darea;

			mt1 = prevTime - pCurpeak->RetentionTime;
			mt2 = curTime - pCurpeak->RetentionTime;

			m1 += (h1 * mt1 + h2 * mt2) / 2.0;
			m2 += (h1 * pow(mt1,2) + h2 * pow(mt2,2)) / 2.0;
			m3 += (h1 * pow(mt1,2) * mt1 + h2 * pow(mt2,2) * mt2) / 2.0;
			m4 += (h1 * pow(mt1,2) * pow(mt1,2) + h2 * pow(mt2,2) * pow(mt2,2)) / 2.0;

			h1 = h2;
		}

		if (curTime < pCurpeak->EndTime)
		{
// estimate lost area at the end of the peak...
			h2 = m_signal[pCurpeak->EndIndex] +
				(pCurpeak->EndTime - curTime) /  m_deltaTimeMinutes * (m_signal[min(pCurpeak->EndIndex+1, m_signalsize-1)] - m_signal[pCurpeak->EndIndex])
			- plinetool->ValueAtTime(pCurpeak->EndTime);

			areaEnd = (h1 + h2) / 2.0 * (pCurpeak->EndTime - curTime);
			areaEnd = ((areaEnd<0) && (pCurpeak->isUserSlice))?0:areaEnd;

#ifdef DEBUG
			dbgstream() << "Integrating END segment ("
			<< curTime
			<< "min "
			<< pCurpeak->EndTime
			<< "min) ("
			<< h1
			<< " "
			<< h2
			<< ") ->"
			<< areaEnd
			<< endl ;
#endif
		}
		else
			{
			areaEnd = 0;
		}

		m1End = areaEnd * (curTime - pCurpeak->RetentionTime);
		m2End = m1End * (curTime - pCurpeak->RetentionTime);
		m3End = m2End * (curTime - pCurpeak->RetentionTime);
		m4End = m2End * (curTime - pCurpeak->RetentionTime);
	}

	pCurpeak->Area = area *  m_deltaTimeMinutes + areaBeg + areaEnd;

	if (fabs(pCurpeak->Area) > 0)
	{
		pCurpeak->Moment1 = (m1 *  m_deltaTimeMinutes + m1Beg + m1End) / pCurpeak->Area; // Moment1 is normalized by Moment0 (area)
		pCurpeak->Moment2 = (m2 *  m_deltaTimeMinutes + m2Beg + m2End) / pCurpeak->Area; // id
		pCurpeak->Moment3 = (m3 *  m_deltaTimeMinutes + m3Beg + m3End) / pCurpeak->Area; // id
		pCurpeak->Moment4 = (m4 *  m_deltaTimeMinutes + m4Beg + m4End) / pCurpeak->Area; // id
	}
	else
		{
		pCurpeak->Moment1 = 0;
		pCurpeak->Moment2 = 0;
		pCurpeak->Moment3 = 0;
		pCurpeak->Moment4 = 0;
	}
	if (pCurpeak->Moment2 > EPSILON_ZERO)
	{
		pCurpeak->Skew = pCurpeak->Moment3 / pow(pCurpeak->Moment2, 1.5);
		pCurpeak->Kurtosis = pCurpeak->Moment4 / pow(pCurpeak->Moment2,2) - 3.0;
	}
	else
		{
		pCurpeak->Skew = INTEGRATION_COMPUT_RESULT_NOTDEFINED;
		pCurpeak->Kurtosis = INTEGRATION_COMPUT_RESULT_NOTDEFINED;
	}

	pCurpeak->BaseLineHeight_AtRetTime = plinetool->ValueAtTime(pCurpeak->RetentionTime);
	delete plinetool ;

    pCurpeak->StartIndex = StartIndex_Saved ;
    pCurpeak->EndIndex = EndIndex_Saved ;
}

void PEAK_COMPUTER::ComputeAsymetry() // implemented on the basis of section 5.3: PEAK STANDARD PARAMETERS
/* --------------------------
	 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
	INT_FLOAT
	hw_left,
	hw_right,
	hw_left_5p,
	hw_right_5p ;

	if ( (fabs(pCurpeak->InterTgLb_X2) > EPSILON_ZERO)
		&& (fabs(pCurpeak->InterTgLb_X1) > EPSILON_ZERO)
		&& (pCurpeak->InterTgLb_X2 > pCurpeak->InterTgLb_X1) )
	{
		pCurpeak->Width_TgBase = pCurpeak->InterTgLb_X2 - pCurpeak->InterTgLb_X1;
	}
	else
		{
		pCurpeak->Width_TgBase = INTEGRATION_COMPUT_RESULT_NOTDEFINED;
	}

//    ASYMETRY_PHARMACOP_EUROP :

	ComputeWidthsAtHeight(0.05, hw_left_5p, hw_right_5p);
	pCurpeak->Width_5p = hw_left_5p + hw_right_5p;
	pCurpeak->Left_Width_5p = hw_left_5p;

	ComputeWidthsAtHeight(0.5, hw_left, hw_right);
	pCurpeak->Width_50p = hw_left + hw_right;
	pCurpeak->Left_Width_50p = hw_left;

	if (hw_left_5p > EPSILON_ZERO)
		pCurpeak->ASYMETRY_PHARMACOP_EUROP = pCurpeak->Width_5p / (hw_left_5p * 2.0);
	else
		pCurpeak->ASYMETRY_PHARMACOP_EUROP = INTEGRATION_COMPUT_RESULT_NOTDEFINED;

//    ASYMETRY_USP_EMG_EP_ASTM :

	if (hw_left_5p > EPSILON_ZERO)
		pCurpeak->ASYMETRY_USP_EMG_EP_ASTM = pCurpeak->Width_5p / (2.0 * hw_left_5p);
	else
		pCurpeak->ASYMETRY_USP_EMG_EP_ASTM = INTEGRATION_COMPUT_RESULT_NOTDEFINED;

//    ASYMETRY_HALF_WIDTHS_44 :

	ComputeWidthsAtHeight(0.044, hw_left, hw_right);
	pCurpeak->Width_4p4 = hw_left + hw_right;
	pCurpeak->Left_Width_4p4 = hw_left;

	if (hw_left > EPSILON_ZERO)
		pCurpeak->ASYMETRY_HALF_WIDTHS_44 = hw_right / hw_left;
	else
		pCurpeak->ASYMETRY_HALF_WIDTHS_44 = INTEGRATION_COMPUT_RESULT_NOTDEFINED;

//    ASYMETRY_HALF_WIDTHS_10 :

	ComputeWidthsAtHeight(0.1, hw_left, hw_right);
	pCurpeak->Width_10p = hw_left + hw_right;
	pCurpeak->Left_Width_10p = hw_left;

	if (hw_left > EPSILON_ZERO)
		pCurpeak->ASYMETRY_HALF_WIDTHS_10 = hw_right / hw_left;
	else
		pCurpeak->ASYMETRY_HALF_WIDTHS_10 = INTEGRATION_COMPUT_RESULT_NOTDEFINED;
}

void PEAK_COMPUTER::ComputePlates() // implemented on the basis of section 5.3: PEAK STANDARD PARAMETERS
/* --------------------------
	 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
	INT_FLOAT
	halfwidthleft,
	halfwidthright,
	w;

//    THPLATES_SIGMA2 :

	ComputeWidthsAtHeight(0.607, halfwidthleft, halfwidthright);
	w = halfwidthleft + halfwidthright;
	if (w > EPSILON_ZERO)
		pCurpeak->THPLATES_SIGMA2 = 4 * pow(pCurpeak->RetentionTime / w,2);
	else
		pCurpeak->THPLATES_SIGMA2 = INTEGRATION_COMPUT_RESULT_NOTDEFINED;
	pCurpeak->Width_60p7 = w;
	pCurpeak->Left_Width_60p7 = halfwidthleft;

//    THPLATES_SIGMA3 :

	ComputeWidthsAtHeight(0.324, halfwidthleft, halfwidthright);
	w = halfwidthleft + halfwidthright;
	if (w > EPSILON_ZERO)
		pCurpeak->THPLATES_SIGMA3 = 9 * pow(pCurpeak->RetentionTime / w,2);
	else
		pCurpeak->THPLATES_SIGMA3 = INTEGRATION_COMPUT_RESULT_NOTDEFINED;
	pCurpeak->Width_32p4 = w;
	pCurpeak->Left_Width_32p4 = halfwidthleft;

//    THPLATES_SIGMA4 :

	ComputeWidthsAtHeight(0.134, halfwidthleft, halfwidthright);
	w = halfwidthleft + halfwidthright;
	if (w > EPSILON_ZERO)
		pCurpeak->THPLATES_SIGMA4 = 16 * pow(pCurpeak->RetentionTime / w,2);
	else
		pCurpeak->THPLATES_SIGMA4 = INTEGRATION_COMPUT_RESULT_NOTDEFINED;
	pCurpeak->Width_13p4 = w;
	pCurpeak->Left_Width_13p4 = halfwidthleft;

//    THPLATES_SIGMA5 :

	w = pCurpeak->Width_4p4; // already computed
	if (w > EPSILON_ZERO)
		pCurpeak->THPLATES_SIGMA5 = 25 * pow(pCurpeak->RetentionTime / w,2);
	else
		pCurpeak->THPLATES_SIGMA5 = INTEGRATION_COMPUT_RESULT_NOTDEFINED;

//    THPLATES_USP:

	if (fabs(pCurpeak->Width_TgBase) > EPSILON_ZERO)
		pCurpeak->THPLATES_USP = 16 * pow(pCurpeak->RetentionTime / pCurpeak->Width_TgBase,2);
	else
		pCurpeak->THPLATES_USP = INTEGRATION_COMPUT_RESULT_NOTDEFINED;

//    THPLATES_EP_ASTM :

	w = pCurpeak->Width_50p; // already computed
	if (w > EPSILON_ZERO)
		pCurpeak->THPLATES_EP_ASTM = 8.0 * log(2.0) * pow(pCurpeak->RetentionTime / w,2);
	else
		pCurpeak->THPLATES_EP_ASTM = INTEGRATION_COMPUT_RESULT_NOTDEFINED;

//    THPLATES_EMG :
// DT 4407 Fixed BO 19/9/01 computation was wrong due to a missing sqr in the numerator below

	ComputeWidthsAtHeight(0.1, halfwidthleft, halfwidthright); // already computed, but we need half widths
	w = halfwidthleft + halfwidthright;
	if ((w > EPSILON_ZERO) && (halfwidthright > EPSILON_ZERO))
		pCurpeak->THPLATES_EMG = 41.7 * pow(pCurpeak->RetentionTime / w,2) / (1.25 + halfwidthleft / halfwidthright);
	else
		pCurpeak->THPLATES_EMG = INTEGRATION_COMPUT_RESULT_NOTDEFINED;

//    THPLATES_AREAHEIGHT :

	if (fabs(pCurpeak->Area) > EPSILON_ZERO)
		pCurpeak->THPLATES_AREAHEIGHT = 16 * pow(pCurpeak->RetentionTime * pCurpeak->Height / (4 * 0.399 * pCurpeak->Area),2);
	else
		pCurpeak->THPLATES_AREAHEIGHT = INTEGRATION_COMPUT_RESULT_NOTDEFINED;
}

void PEAK_COMPUTER::LocateInflexionPoints()
/* --------------------------
	 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History : BO 19/02/01 Fixed DT3878 (fix could be improved
	 *           by studying more precisely what could be done in the case of
	 *           such a range error. For the moment we just do nothing thus the
	 *           computation of the tangent is not good, but we do not care because
	 *           it just for very small peaks near the end of the signal.)
 * -------------------------- */
{
	int
	nbPointsLeft,
	nbPointsRight,
	nbpoints,
	LeftIndex,
	RightIndex;

// calculation is based on the first derivative
	nbpoints = pCurpeak->EndIndex - pCurpeak->StartIndex;
	assert(nbpoints > 0);

	delete [] m_firstDeriv ;
	m_firstDeriv = new INT_FLOAT[nbpoints] ;

	for (int i=0; i<nbpoints; ++i)
	m_firstDeriv[i] = fabs(m_signalSmooth[pCurpeak->StartIndex + i] - m_signalSmooth[pCurpeak->StartIndex + i -1]);

	if ( (pCurpeak->ApexIndex - pCurpeak->StartIndex) >= (INTEGRATOR_OPTIMAL_POINTS_PER_PEAK / 2) )
	{
		LeftIndex = LocateMaxDeriv(pCurpeak->ApexIndex - pCurpeak->StartIndex - 1, 0, -1, pCurpeak->LeftInflexionPointErrorCode);
		if (pCurpeak->LeftInflexionPointErrorCode == -1)
		{
			LeftIndex += pCurpeak->StartIndex;
			pCurpeak->LeftInflexionPointTime = LeftIndex * m_deltaTimeMinutes;
			LineFitter.ClearPoints();

// avoid using points that are too close from the apex or extremity, or the tangent will look strange
			nbPointsLeft = max( (LeftIndex - pCurpeak->StartIndex)/10, INTEGR_NBPOINTS_TANGENTES); // 10% of the left points
			nbPointsRight = max((pCurpeak->ApexIndex - LeftIndex) / 10, INTEGR_NBPOINTS_TANGENTES); //
			if (LeftIndex - pCurpeak->StartIndex < INTEGR_NBPOINTS_TANGENTES) nbPointsLeft = 0;
			if (pCurpeak->ApexIndex - LeftIndex >= INTEGR_NBPOINTS_TANGENTES) nbPointsRight = 0;

			for(int i =-nbPointsLeft; i<=nbPointsRight; ++i)
			{
// If introduced to solve DT3878
				if ((LeftIndex + i + 1 > 0)
					&& (LeftIndex + i + 1 <= m_signalsize))
				LineFitter.AddPoint((LeftIndex + i) * m_deltaTimeMinutes, m_signalSmooth[LeftIndex + i]);
			}

			if (LineFitter.GetCoefs(pCurpeak->LeftTgCoeffA,pCurpeak->LeftTgCoeffB))
			{
				pCurpeak->InterTgLb_X1 = max(pCurpeak->StartTime, min(pCurpeak->EndTime, ComputeTgIntersectionWithBL(pCurpeak->LeftTgCoeffB, pCurpeak->LeftTgCoeffA)));
			}
		}
		else
			{
			pCurpeak->LeftInflexionPointTime = -1;
		}
	}
	else
		{
		pCurpeak->LeftInflexionPointTime = -1;
	}

	if (pCurpeak->EndIndex - pCurpeak->ApexIndex >= INTEGRATOR_OPTIMAL_POINTS_PER_PEAK / 2)
	{
		RightIndex = LocateMaxDeriv(pCurpeak->ApexIndex - pCurpeak->StartIndex + 1, pCurpeak->EndIndex - pCurpeak->StartIndex, 1, pCurpeak->RightInflexionPointErrorCode);
		if (pCurpeak->RightInflexionPointErrorCode == -1)
		{
			RightIndex += pCurpeak->StartIndex ;
			pCurpeak->RightInflexionPointTime = RightIndex * m_deltaTimeMinutes;
			LineFitter.ClearPoints();
// avoid using points that are too close from the apex or extremity, or the tangent will look strange
//TODO BO 15/03/01 the algo is slightly different from the one used above, I do not remember if there is a good reason, to be Checked...
			if (RightIndex - pCurpeak->ApexIndex >= INTEGR_NBPOINTS_TANGENTES)
				nbPointsLeft = INTEGR_NBPOINTS_TANGENTES ;
			else
				nbPointsLeft = 0;

			if (pCurpeak->EndIndex - RightIndex >= INTEGR_NBPOINTS_TANGENTES)
				nbPointsRight = INTEGR_NBPOINTS_TANGENTES ;
			else
				nbPointsRight = 0;

			for(int i = -nbPointsLeft; i<=nbPointsRight; ++i)
			{
// If introduced to solve DT3878
				if ( (RightIndex + i + 1 > 0)
					&& (RightIndex + i + 1 <= m_signalsize) )
				LineFitter.AddPoint((RightIndex + i) * m_deltaTimeMinutes, m_signalSmooth[RightIndex + i]);
			}

			if (LineFitter.GetCoefs(pCurpeak->RightTgCoeffA, pCurpeak->RightTgCoeffB))
			{
				pCurpeak->InterTgLb_X2 = max(pCurpeak->StartTime, min(pCurpeak->EndTime, ComputeTgIntersectionWithBL(pCurpeak->RightTgCoeffB, pCurpeak->RightTgCoeffA)));
			}
		}
		else
			pCurpeak->RightInflexionPointTime = -1;
	}
	else
		pCurpeak->RightInflexionPointTime = -1;
}

int PEAK_COMPUTER::LocateMaxDeriv(int idxfrom,
	int idxto,
	int step,
	int& errCode)
/* --------------------------
	 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose :
 * History :
 * -------------------------- */
{
	assert(((step == +1) && (idxfrom <= idxto))
		||
		((step == -1) && (idxfrom >= idxto)));

// locate the max
	INT_FLOAT valmax = INT_MINFLOAT;
	int idxmax = -1;
	int i = idxfrom;

	while (i != idxto)
	{
		assert( (i >= 0) && (i < m_signalsize) );
//Format('Inflexion Point Error1: %d %d', [i, Length(FFirstDeriv)]));

		if (m_firstDeriv[i] > valmax)
		{
			valmax = m_firstDeriv[i];
			idxmax = i;
		}
		i += step;
	}

	if (idxmax > -1)
	{
		errCode = -1 ;
		return idxmax;
	}
	else
		{
		errCode = 0;
		return -1;
	}
}

INT_FLOAT PEAK_COMPUTER::ComputeTgIntersectionWithBL(INT_FLOAT TgCoeffA,
	INT_FLOAT TgCoeffB)
{
// by default the baseline is horizontal
	INT_FLOAT a = 0;
	INT_FLOAT b = 0;
	INT_FLOAT result = -1 ;

	try
	{
		if ( pCurpeak->pFmyLine != NULL)
		{
			a = (pCurpeak->pFmyLine->StartValue - pCurpeak->pFmyLine->EndValue)
			/ (pCurpeak->pFmyLine->StartTime  - pCurpeak->pFmyLine->EndTime );

			b = pCurpeak->pFmyLine->StartValue - pCurpeak->pFmyLine->StartTime * a;

			result = (b - TgCoeffB) / (TgCoeffA - a);
		}
	}
	catch(...)
	{
		result = -1;
	}
	return result ;
}

