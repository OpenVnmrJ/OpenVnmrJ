/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
*   HISTORY:
*     Revision 1.2  2006/08/29 17:25:29  deans
*     1. changed sgl includes to use quotes (vs brackets)
*     2. added ncomm sources
*     3. mods to Makefile
*     4. added makenvpsg.sgl and makenvpsg.sgl.lnx
*
*     Revision 1.1  2006/08/23 14:09:59  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/08/22 23:30:03  deans
*     *** empty log message ***
*
*     Revision 1.5  2006/07/11 20:09:58  deans
*     Added explicit prototypes for getvalnowarn etc. to sglCommon.h
*     - these are also defined in  cpsg.h put can't #include that file because
*       gcc complains about the "extern C" statement which is only allowed
*       when compiling with g++ (at least for gcc version 3.4.5-64 bit)
*
*     Revision 1.4  2006/07/11 17:50:57  deans
*     mods to sgl
*     moved all globals to sglCommon.c
*
*     Revision 1.3  2006/07/07 20:10:19  deans
*     sgl library changes
*     --------------------------
*     1.  moved most core functions in sgl.c to sglCommon.c (new).
*     2. sgl.c now contains only pulse-sequence globals and
*          parameter initialization functions.
*     3. sgl.c is not built into the nvpsg library but
*          is compiled in with the user sequence using:
*          include "sgl.c"
*        - as before ,so sequences don't need to be modified
*     4. sgl.h is no longer used and has been removed from the project
*
*     Revision 1.2  2006/07/07 01:11:03  mikem
*     modification to compile with psg
*
*   
* Contains SGL Addition by Michael L. Gyngell.
* 1) Extensions to SGL that allow the creation of complex multi gradient
*    channel waveforms from basic SGL waveforms.
* 2) Extensions to SGL for deriving gradient duty cycle demands
*
***************************************************************************/
#include "sglCommon.h"
#include "sglDutyCycle.h"



/********************************************************************
* The following are utility functions for the Compound Shape functions.
*
*********************************************************************/

/********************************************************************
* Function Name: eulerMatrix
* Example:		eulerMatrix( theta, psi, phi, &eulerM )
* Purpose: 	calculates an euler rotation matrix from the euler angles
* Input
*	Formal:	theta
*				psi
*				phi
*	Private:	none
*	Public:	none
* Output
*	Return:	none
*	Formal:	eulerM - pointer to an SGL_EULER_MATRIX_T structure
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
void eulerMatrix( double theta, double psi, double phi, SGL_EULER_MATRIX_T *eulerM )
{
    double D_R;
    double sinang1,cosang1,sinang2,cosang2,sinang3,cosang3;
    double m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double im11,im12,im13,im21,im22,im23,im31,im32,im33;
    double tol = 1.0e-14;

    /* Convert the input to the basic mag_log matrix */
    D_R = M_PI / 180;

    cosang1 = cos(D_R*theta);
    sinang1 = sin(D_R*theta);
       
    cosang2 = cos(D_R*psi);
    sinang2 = sin(D_R*psi);
       
    cosang3 = cos(D_R*phi);
    sinang3 = sin(D_R*phi);

    m11 = (sinang2*cosang1 - cosang2*cosang3*sinang1);
    m12 = (-1.0*sinang2*sinang1 - cosang2*cosang3*cosang1);
    m13 = (sinang3*cosang2);

    m21 = (-1.0*cosang2*cosang1 - sinang2*cosang3*sinang1);
    m22 = (cosang2*sinang1 - sinang2*cosang3*cosang1);
    m23 = (sinang3*sinang2);

    m31 = (sinang1*sinang3);
    m32 = (cosang1*sinang3);
    m33 = (cosang3);

    if (fabs(m11) < tol) m11 = 0;
    if (fabs(m12) < tol) m12 = 0;
    if (fabs(m13) < tol) m13 = 0;
    if (fabs(m21) < tol) m21 = 0;
    if (fabs(m22) < tol) m22 = 0;
    if (fabs(m23) < tol) m23 = 0;
    if (fabs(m31) < tol) m31 = 0;
    if (fabs(m32) < tol) m32 = 0;
    if (fabs(m33) < tol) m33 = 0;

    /* Generate the transform matrix for mag_log ******************/

    /*HEAD SUPINE*/
        im11 = m11;       im12 = m12;       im13 = m13;
        im21 = m21;       im22 = m22;       im23 = m23;
        im31 = m31;       im32 = m32;       im33 = m33;

    /*Transpose intermediate matrix and return***********/
    eulerM->tm11 = im11;     eulerM->tm21 = im12;     eulerM->tm31 = im13;
    eulerM->tm12 = im21;     eulerM->tm22 = im22;     eulerM->tm32 = im23;
    eulerM->tm13 = im31;     eulerM->tm23 = im32;     eulerM->tm33 = im33;
}

/********************************************************************
* Function Name: eulerRotate
* Example:	eulerRotate( readPt, phasePt, slicePt, &eulerM, &xPt, &yPt, &zPt )
* Purpose: 	rotates a set logical points in physical points
* Input
*	Formal:	readPt
*				phasePt
*				slicePt
*				eulerM
*	Private:	none
*	Public:	none
* Output
*	Return:	none
*	Formal:	xPt
*				yPt
*				zPt
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
void eulerRotate( double readPt, double phasePt, double slicePt,
			SGL_EULER_MATRIX_T *euler, double *xPt, double *yPt, double *zPt)
{
		*xPt =  euler->tm11*readPt + euler->tm12*phasePt +euler->tm13*slicePt;
		*yPt =  euler->tm21*readPt + euler->tm22*phasePt +euler->tm23*slicePt;
		*zPt =  euler->tm31*readPt + euler->tm32*phasePt +euler->tm33*slicePt;	
}

/********************************************************************
* Function Name: meanSquareCurrent
* Example:	meanSquareCurrent( gradPt, gMax, iMax, duration )
* Purpose: 	calculates the mean square current for a gradient point
* Input
*	Formal:	gradPt
*				gMax
*				iMax
*				duration
*	Private:	none
*	Public:	none
* Output
*	Return:	mean square current
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
double meanSquareCurrent( double dataPt, double gmax, double imax, double duration )
{
	double _temp;
	
	_temp = dataPt * imax / gmax;
	_temp = _temp * _temp * duration;

	return _temp;
}


/********************************************************************
* Function Name: initDutyCycle
* Example:		initDutyCycle( &msCurrents )
* Purpose: 	initializes duty cycle calculations
* Input
*	Formal:	none
*	Private:	none
*	Public:	none
* Output
*	Return:	none
*	Formal:	msCurrents 
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
void initDutyCycle( SGL_MEAN_SQUARE_CURRENTS_T *ms,
							SGL_MEAN_SQUARE_CURRENTS_T *rms )
{
	ms->x = 0.0;
	ms->y = 0.0;
	ms->z = 0.0;

	rms->x = 0.0;
	rms->y = 0.0;
	rms->z = 0.0;
	
	eulerMatrix( theta, psi,phi, &mainRotationMatrix );
}

/********************************************************************
* Function Name: dutyCycle
* Example:	dutyCycle( (void *)&readGrad, (void *)&phaseGrad, (void *)&sliceGrad,
*						scaleRead, scalePhase, scaleSlice, theta, psi, phi, gMax,
*						&msCurrents )
* Purpose: 	initializes duty cycle calculations
* Input
*	Formal:	none
*	Private:	none
*	Public:	none
* Output
*	Return:	none
*	Formal:	msCurrents 
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
void dutyCycleWithEulerMatrix( SGL_GRADIENT_T *readGrad, 
				SGL_GRADIENT_T *phaseGrad,
				SGL_GRADIENT_T *sliceGrad,
				double scaleR, double scaleP, double scaleS,
				SGL_EULER_MATRIX_T *rot,
				double gmax,
							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents )
{
	long _numPoints;
	int _i;
	double *_pointsR = NULL;	
	double *_pointsP = NULL;	
	double *_pointsS = NULL;
	double _x, _y, _z;
	ERROR_NUM_T _error;

	_numPoints = 0;

	if(( readGrad == NULL ) && ( phaseGrad == NULL ) && (sliceGrad == NULL ))
	{
		_error = ERR_GRAD_PULSES;
		displayError( _error, __FILE__, __FUNCTION__, __LINE__ );
	}
	if( readGrad != NULL )
	{
		_numPoints = getNumPoints( readGrad );
		_pointsR = getDataPoints( readGrad );
	}
	if( phaseGrad != NULL )
	{
		if( _numPoints == 0)
		{
			_numPoints = getNumPoints( phaseGrad );
		}
		else if( _numPoints != getNumPoints( phaseGrad ))
		{
			_error = ERR_GRAD_DURATION;
			displayError( _error, __FILE__, __FUNCTION__, __LINE__ );
		}			
		_pointsP = getDataPoints( phaseGrad );

	}
	if( sliceGrad != NULL )
	{
		if( _numPoints == 0)
		{
			_numPoints = getNumPoints( sliceGrad );
		}
		else if( _numPoints != getNumPoints( sliceGrad ))
		{
			_error = ERR_GRAD_DURATION;
			displayError( _error, __FILE__, __FUNCTION__, __LINE__ );
		}			
		_pointsS = getDataPoints( sliceGrad );
	}

 	for( _i=0; _i< _numPoints; _i++ )
	{
		eulerRotate( ( readGrad == NULL ? 0.0: _pointsR[_i]*scaleR  ),
				( phaseGrad == NULL ? 0.0: _pointsP[_i]*scaleP ),
				( sliceGrad == NULL ? 0.0: _pointsS[_i]*scaleS ),
				rot, &_x, &_y, &_z );

		msCurrents->x += meanSquareCurrent( _x, gmax, coilLimits.current, GRADIENT_RES );
		msCurrents->y += meanSquareCurrent( _y, gmax, coilLimits.current, GRADIENT_RES );
		msCurrents->z += meanSquareCurrent( _z, gmax, coilLimits.current, GRADIENT_RES );
	}
}

void dutyCycle( SGL_GRADIENT_T *readGrad, 
				SGL_GRADIENT_T *phaseGrad,
				SGL_GRADIENT_T *sliceGrad,
				double scaleR, double scaleP, double scaleS,
				double gmax,
							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents )
{
	dutyCycleWithEulerMatrix( readGrad, phaseGrad,sliceGrad,
				scaleR, scaleP,  scaleS,
				&mainRotationMatrix,
				 gmax, msCurrents );
}

void dutyCycleWithEulerAngles( SGL_GRADIENT_T *readGrad, 
				SGL_GRADIENT_T *phaseGrad,
				SGL_GRADIENT_T *sliceGrad,
				double scaleR, double scaleP, double scaleS,
				double theta, double psi, double phi,
				double gmax,
							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents )
{
	SGL_EULER_MATRIX_T euler; 

	eulerMatrix( theta, psi, phi, &euler ); 

	dutyCycleWithEulerMatrix( readGrad, phaseGrad,sliceGrad,
				scaleR, scaleP,  scaleS,
				&euler,
				 gmax, msCurrents );
}

/********************************************************************
* Function Name: gradListDutyCycle
* Example:	dutyCycle( (void *)&readGrad, (void *)&phaseGrad, (void *)&sliceGrad,
*						scaleRead, scalePhase, scaleSlice, theta, psi, phi, gMax,
*						&msCurrents )
* Purpose: 	initializes duty cycle calculations
* Input
*	Formal:	none
*	Private:	none
*	Public:	none
* Output
*	Return:	none
*	Formal:	msCurrents 
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
void dutyCycleOfGradListWithEulerMatrix(
							struct SGL_GRAD_NODE_T* gradList,
							double startTime, double duration,
							SGL_EULER_MATRIX_T *rot,
							double gmax,
							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents )
{
	double _startTime, _endTime, _duration;
	struct SGL_GRAD_NODE_T* _current = gradList;
	double _relStart;
	int i;
	double _x, _y, _z;
	int _startIdx, _endIdx;

	GENERIC_GRADIENT_T _s, _r, _p;
	
	_startTime = getStartTimeOfList( gradList );
	_endTime	= getEndTimeOfList( gradList );
	_duration = _endTime - _startTime;

	if( startTime < _startTime )
	{
		abort_message("dutyCycleOfGradListWithEulerMatrix: bad start time!");
	}
	if( (duration > 0)&& ((startTime + duration) > _endTime ))
	{
		abort_message("dutyCycleOfGradListWithEulerMatrix: bad end time!");
	}
	if( startTime == 0.0 )
	{
		startTime = _startTime;
	}
	if( duration == 0.0 )
	{
		duration = _endTime - startTime;
	}
	
	initOutputGradient( &_r, "READ", _duration, GRADIENT_RES );
	initOutputGradient( &_p, "PHASE", _duration, GRADIENT_RES );
	initOutputGradient( &_s, "SLICE", _duration, GRADIENT_RES );
	
	while( _current != NULL )
	{
		_relStart = _current->startTime - _startTime;

		switch( _current->logicalAxis )
		{
			case READ:
					copyToOutputGradient( &_r, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				break;
			case PHASE:
					copyToOutputGradient( &_p, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				break;
			case SLICE:
					copyToOutputGradient( &_s, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
			default:
				break;
		}
		_current = _current->next;
	}
	

	_startIdx = startTime/ GRADIENT_RES;
	_endIdx = (startTime + duration)/GRADIENT_RES;

 	for(i=_startIdx; i< _endIdx; i++ )
	{
		eulerRotate( _r.dataPoints[i], _p.dataPoints[i], _s.dataPoints[i],
			rot, &_x, &_y, &_z );

		msCurrents->x += meanSquareCurrent( _x, gmax, coilLimits.current, GRADIENT_RES );
		msCurrents->y += meanSquareCurrent( _y, gmax, coilLimits.current, GRADIENT_RES );
		msCurrents->z += meanSquareCurrent( _z, gmax, coilLimits.current, GRADIENT_RES );
	}
}

void dutyCycleOfGradList( struct SGL_GRAD_NODE_T* gradList,
							double gmax,
							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents )
{
	dutyCycleOfGradListWithEulerMatrix( gradList, 0.0, 0.0, &mainRotationMatrix,
							gmax, msCurrents );
}

void dutyCycleOfGradListSnippet( struct SGL_GRAD_NODE_T* gradList,
							double startTime, double duration,
							double gmax,
							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents )
{
	dutyCycleOfGradListWithEulerMatrix( gradList, startTime, duration, &mainRotationMatrix,
							gmax, msCurrents );
}

void dutyCycleOfGradListWithEulerAngles( struct SGL_GRAD_NODE_T* gradList,
							double theta, double psi, double phi,
							double gmax,
							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents )
{
	SGL_EULER_MATRIX_T euler; 

	eulerMatrix( theta, psi, phi, &euler ); 

	dutyCycleOfGradListWithEulerMatrix( gradList, 0.0, 0.0, &euler,
							gmax, msCurrents );

}

double calcDutyCycle( SGL_MEAN_SQUARE_CURRENTS_T *msCurrents, double time,
								SGL_MEAN_SQUARE_CURRENTS_T *rmsCurrents )
{
	double _rmsX, _rmsY, _rmsZ;
	double _minTr;
	double _tr = time;
	ERROR_NUM_T error;

	
	_rmsX = sqrt( msCurrents->x/time );
	_rmsY = sqrt( msCurrents->y/time );
	_rmsZ = sqrt( msCurrents->z/time );

	if( (_rmsX > coilLimits.xrms) ||
			(_rmsY > coilLimits.yrms) ||
			(_rmsZ > coilLimits.zrms))
	{
		_minTr = MAX(msCurrents->x/(coilLimits.xrms * coilLimits.xrms ),
					MAX(msCurrents->y/(coilLimits.yrms * coilLimits.yrms ),
						msCurrents->z/(coilLimits.zrms * coilLimits.zrms )));

		error = ERR_DUTY_CYCLE;
		displayError( error, __FILE__, __FUNCTION__, __LINE__ );
		_tr = _minTr;
	}
	
	rmsCurrents->x = _rmsX;
	rmsCurrents->y = _rmsY;
	rmsCurrents->z = _rmsZ;


	return _tr;
}

void init_duty_cycle( )
{
	initDutyCycle( &msCurrents, &rmsCurrents );
}

void duty_cycle_of_list( )
{
	dutyCycleOfGradList( *workingList, gmax, &msCurrents );
}

void duty_cycle_of_list_snippet( double startTime, double duration )
{
	dutyCycleOfGradListSnippet( *workingList, startTime, duration,
							gmax, &msCurrents );
}

double calc_duty_cycle( double time )
{
	return calcDutyCycle( &msCurrents, time, &rmsCurrents );
}
