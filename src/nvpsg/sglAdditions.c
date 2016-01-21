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
*
*     Revision 1.2  2006/08/29 17:25:29  deans
*     1. changed sgl includes to use quotes (vs brackets)
*     2. added ncomm sources
*     3. mods to Makefile
*     4. added makenvpsg.sgl and makenvpsg.sgl.lnx
*
*     Revision 1.1  2006/08/23 14:09:58  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/08/22 23:30:02  deans
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
*     Revision 1.2  2006/07/07 01:10:44  mikem
*     modification to compile with psg
*
*   
* 
*
* Author: Michael L. Gyngell
* Contains additions to SGL by Michael L. Gyngell.
* 1) Utilities for extracting general information from SGL structures
*
***************************************************************************/
#include "sglCommon.h"
#include "sglAdditions.h"

int countListNodes( struct SGL_LIST_T *aList )
{
	struct SGL_LIST_T *current;
	int count;

	current = aList;
	count = 0;
	while( current )
	{
		count++;
		current = current->next;
	}
	return count;
}

struct SGL_LIST_T *headOfList( struct SGL_LIST_T *aList )
{
	struct SGL_LIST_T *current;
	if( !(current = aList) ) return NULL;

	while( current->next )
	{
		current = current->next;
	}
	return current;
}

char *allocateString( int aSize )
{
	char *value;
	if( (value = (char *)malloc(aSize*sizeof(char))) == NULL ) {
        displayError(ERR_MALLOC, __FILE__, __FUNCTION__, __LINE__); 
		exit(0);
	}
	return value;
}

void freeString( char **aString )
{
	free( *aString );
	*aString = NULL;
}

char *duplicateString( char *aString )
{
	char *value;
	int len;

	if( aString == NULL ) return NULL;

	len = strlen( aString );
	value = allocateString( len + 1 );

	if( len > 0 ) {
		strcpy( value, aString );
	}
	value[len] = 0;

	return value;
}

void replaceString( char **aString, char *aNewString )
{
	freeString( aString );
	*aString = duplicateString( aNewString );
}

char *concatenateStrings( char *aString, char *a2ndString )
{
	char *string;

	int len;

	len = strlen( aString ) + strlen( a2ndString ) + 1;
	string = allocateString( len );
	strcpy( string, aString );
	len = strlen(string);
	strcpy( string+len, a2ndString );

	return string;
}

void appendString( char **aString, char *aAddString )
{
	char *string;

	if( *aString == NULL ) {
		*aString = duplicateString( aAddString );
	} else {
		string = concatenateStrings( *aString, aAddString );
		freeString( aString );
		*aString = string;
	}
}

void appendFormattedString( char **aString, char *aAddString, ... )
{
	char rString[MAXSTR];
	va_list ap;

	if( aAddString != NULL ) {
		va_start( ap, aAddString );
		vsprintf( rString, aAddString, ap );
		va_end( ap );
	}
	appendString( aString, rString );
}


/********************************************************************
* Function Name: set_pe_order
* Example:	table = set_pe_order();
* Purpose: 	define the phase encode order based on a VnmrJ parameter petype.
*           petype is evaluated, and an appropriate phase encoding table i
*				written to tablib as required. The VnmrJ parameter, petable, is
*				set according to the name of the phase encoding table
*				that was generated.
*
*				defined values of petype:
*						0 - linear phase encoding - no table generated
*						1 - user defined pe - users set petable explicitly
*						2 - center to max phase encoding
*						3 - max to center phase encoding
*						other values undefined
* Input
*	Formal:	none
*	Private:	none
*	Public:	none
* Output
*	Return:	1 if a table was generated, 0 otherwise
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
int set_pe_order()
{
	/* short routine to set the phase encoding order
	 	petype = 0 is standard linear
	 	petype = 1 is user defined. This means that the user
						defines their own pe table from a macro 
						and set "petable" to be the tabfile name 
		petype = 2 is center-out - generated from nv 
	   other petype to be defined as required for types > 2
	*/

	int pe_type;
	int *pe_order;
	int i;
	int inv;
	int table = 0;
	int numPESteps;

	pe_type = getvalnwarn("petype");
	
	switch( pe_type )
	{
		case 1: 
			/*  Check for external PE table ***************************/
			if (strcmp(petable,"n") && strcmp(petable,"N") && strcmp(petable,"")) {
				loadtable(petable);
				table = 1;
			}			
			break;
		case 2:
			/*center to max phase encoding */
			/* sets t1 to the phase encoding order */
			numPESteps = nv;
			if(( pe_order = (int *)malloc(numPESteps*sizeof(int)))==NULL)
			{
				abort_message("set_pe_order: Memory allocation problem!");
			}
			inv = 1;
			pe_order[0] = 0;
			for(i=1;i<numPESteps;i++)
			{
				inv = -inv;
				pe_order[i] = pe_order[i-1] + inv*i;
			}
			strcpy(petable,"centertomax");
			write_tab_file(petable,1,1,numPESteps,pe_order);
			settable(t1,numPESteps,pe_order);
			putstring("petable",petable);
			table=1;
			break;
		case 3:
			/* max to center phase encoding*/
			numPESteps = nv;
			if(( pe_order = (int *)malloc(numPESteps*sizeof(int)))==NULL)
			{
				abort_message("set_pe_order: Memory allocation problem!");
			}
			inv = 1;
			pe_order[numPESteps-1] = 0;
			for(i=1;i<numPESteps;i++)
			{
				inv = -inv;
				pe_order[numPESteps-i-1] = pe_order[numPESteps-i] + inv*i;
			}
			strcpy(petable,"maxtocenter");
			write_tab_file(petable,1,1,numPESteps,pe_order);
			settable(t1,numPESteps,pe_order);
			putstring("petable",petable);
			table=1;
			break;

		default:
			/* this the default of center-out so make sure that petable is an empty string */
			putstring("petable","");
			table = 0;			
			break;
	}
	return table;
}


/********************************************************************
* Function Name:write_tab_file
* Example:	write_tab_file("myTabFile",2,1,t1size,t1array,2,t2size,t2array);
* Purpose: 	writes an arbitrary number number of tables to a tab file.
*				this function uses a variable length arguement list. 
*
* Input
*	Formal:	tableFilename  - a string containin the name of the tab file
*				numTable 		- int - number of tables to be written
*		for each of the tables there are 3 arguements
*				tableNumber		- int - refers to the table number
*				tableSize		- int - the table size
*				tableArray		- int pointer - array of values
*	Private:	none
*	Public:	none
* Output
*	Return:	none
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
void write_tab_file( char *tableFileName, int numTables, ... )
{
	va_list ap;
	FILE *fp;
	char tablePath[MAX_STR];
	int i,j;
	int _size;
	int _list;
	int *_ptr;

	sprintf( tablePath,"%s/tablib/%s",userdir,tableFileName );
	
	if(( fp = fopen( tablePath, "w" )) == NULL )
	{
		abort_message("write_tab_file: error opening file%s\n",tableFileName);
	}
	
	va_start(ap, numTables);
	for(i=0;i<numTables;i++)
	{
		_list = va_arg(ap, int);
		_size = va_arg(ap, int);
		_ptr = va_arg(ap, int*);
		
		if((_list < 1 )||(_list > 60 ))
		{
			abort_message("write_tab_file: invalid table number = %d", _list);
		}
		
		fprintf(fp,"t%d= ",_list);
		for(j=0;j<_size;j++) fprintf(fp,"%d ",_ptr[j]);
		fprintf(fp,"\n");
		fclose(fp);
	}
	va_end(ap);
}


/********************************************************************
* Function Name: getResolution
* Example:		getAmplitude( SGL_GRADIENT_T *grad  )
* Purpose: 	retrieves the gradient amplitude from the waveform. This
*		  	SGL extension includes and extra field in the gradient
*			structure so that the gradient type can be automatically
*			identified. This allows generic functions to access the 
*			parameters of an arbitrary gradient without having to cast
*			the pointer.
* Input
*	Formal:	sglGradient	- pointer to a gradient structure
*	Private:	none
*	Public:	none
* Output
*	Return:	Gradient amplitude in G/cm
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
double getResolution( SGL_GRADIENT_T *grad )
{
	double _res;
	
	switch ( grad->_as_GENERIC_GRADIENT.type )
	{
		case GENERIC_GRADIENT_TYPE:
		case REFOCUS_GRADIENT_TYPE:
		case DEPHASE_GRADIENT_TYPE:
		case PHASE_GRADIENT_TYPE:
			_res = grad->_as_GENERIC_GRADIENT.resolution;
			break;	
		case SLICE_SELECT_GRADIENT_TYPE:
			_res = grad->_as_SLICE_SELECT_GRADIENT.resolution;
			break;	
		case READOUT_GRADIENT_TYPE:
			_res = grad->_as_READOUT_GRADIENT.resolution;
			break;	
		case FLOWCOMP_TYPE:
			_res = grad->_as_FLOWCOMP_GRADIENT.resolution;
			break;	
		case KERNEL_MARKER_TYPE:
			_res = GRADIENT_RES;
			break;
		default:
			_res = GRADIENT_RES;
			break;
	}
	return _res;
}
/********************************************************************
* Function Name: setName
* Example:		setName( &SGL_GRADIENT_T *grad, char name[]  )
* Purpose: 	retrieves the gradient amplitude from the waveform. This
*		  	SGL extension includes and extra field in the gradient
*			structure so that the gradient type can be automatically
*			identified. This allows generic functions to access the 
*			parameters of an arbitrary gradient without having to cast
*			the pointer.
* Input
*	Formal:	sglGradient	- pointer to a gradient structure
*	Private:	none
*	Public:	none
* Output
*	Return:	Gradient amplitude in G/cm
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
void setName( SGL_GRADIENT_T *grad, char name[] )
{	
	switch ( grad->_as_GENERIC_GRADIENT.type )
	{
		case GENERIC_GRADIENT_TYPE:
		case REFOCUS_GRADIENT_TYPE:
		case DEPHASE_GRADIENT_TYPE:
		case PHASE_GRADIENT_TYPE:
			strcpy((char*)(grad->_as_GENERIC_GRADIENT.name), name);
			break;	
		case SLICE_SELECT_GRADIENT_TYPE:
			strcpy(grad->_as_SLICE_SELECT_GRADIENT.name, name);
			break;	
		case READOUT_GRADIENT_TYPE:
			strcpy(grad->_as_READOUT_GRADIENT.name, name);
			break;	
		case FLOWCOMP_TYPE:
			strcpy(grad->_as_FLOWCOMP_GRADIENT.name, name);
			break;	
		case KERNEL_MARKER_TYPE:
			break;
		default:
			break;
	}
}

/********************************************************************
* Function Name: getName
* Example:		*getName( SGL_GRADIENT_T *grad )
* Purpose: 	retrieves the gradient amplitude from the waveform. This
*		  	SGL extension includes and extra field in the gradient
*			structure so that the gradient type can be automatically
*			identified. This allows generic functions to access the 
*			parameters of an arbitrary gradient without having to cast
*			the pointer.
* Input
*	Formal:	sglGradient	- pointer to a gradient structure
*	Private:	none
*	Public:	none
* Output
*	Return:	Gradient amplitude in G/cm
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
char *getName( SGL_GRADIENT_T *grad )
{
	char *_name = 0;
	
	switch ( grad->_as_GENERIC_GRADIENT.type )
	{
		case GENERIC_GRADIENT_TYPE:
		case REFOCUS_GRADIENT_TYPE:
		case DEPHASE_GRADIENT_TYPE:
		case PHASE_GRADIENT_TYPE:
			_name = grad->_as_GENERIC_GRADIENT.name;
			break;	
		case SLICE_SELECT_GRADIENT_TYPE:
			_name = grad->_as_SLICE_SELECT_GRADIENT.name;
			break;	
		case READOUT_GRADIENT_TYPE:
			_name = grad->_as_READOUT_GRADIENT.name;
			break;	
		case FLOWCOMP_TYPE:
			_name = grad->_as_FLOWCOMP_GRADIENT.name;
			break;	
		case KERNEL_MARKER_TYPE:
			_name[0] = '\0';	
		default:
			_name[0] = '\0';
			break;
	}
	return _name;
}

/********************************************************************
* Function Name: getAmplitude
* Example:		getAmplitude( &ro_grad )
* Purpose: 	retrieves the gradient amplitude from the waveform. This
*		  	SGL extension includes and extra field in the gradient
*			structure so that the gradient type can be automatically
*			identified. This allows generic functions to access the 
*			parameters of an arbitrary gradient without having to cast
*			the pointer.
* Input
*	Formal:	sglGradient	- pointer to a gradient structure
*	Private:	none
*	Public:	none
* Output
*	Return:	Gradient amplitude in G/cm
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
double getAmplitude( SGL_GRADIENT_T *grad )
{
	double _amp;
	
	switch ( grad->_as_GENERIC_GRADIENT.type )
	{
		case GENERIC_GRADIENT_TYPE:
		case REFOCUS_GRADIENT_TYPE:
		case DEPHASE_GRADIENT_TYPE:
		case PHASE_GRADIENT_TYPE:
			_amp = grad->_as_GENERIC_GRADIENT.amp;
			break;	
		case SLICE_SELECT_GRADIENT_TYPE:
			_amp = grad->_as_SLICE_SELECT_GRADIENT.amp;
			break;	
		case READOUT_GRADIENT_TYPE:
			_amp = grad->_as_READOUT_GRADIENT.amp;
			break;	
		case FLOWCOMP_TYPE:
			_amp = grad->_as_FLOWCOMP_GRADIENT.amp;
			break;	
		case KERNEL_MARKER_TYPE:
			_amp = 0.0;	
		default:
			_amp = 0.0;
			break;
	}
	return _amp;
}

/********************************************************************
* Function Name: getDuration
* Example:		getDuration( & )
* Purpose: 	retrieves the duration of the gradient the waveform.
*			NOTE: This SGL extension includes and extra field in the
*			gradient structure so that the gradient type can be
*			automatically identified. This allows generic functions
*           to access the parameters of an arbitrary gradient without
*			having to cast the pointer.
* Input
*	Formal:	sglGradient	- pointer to a gradient structure
*	Private:	none
*	Public:	none
* Output
*	Return:	Gradient duration in seconds (double)
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
double getDuration( SGL_GRADIENT_T	*grad )
{
	double _duration;
	
	switch ( grad->_as_GENERIC_GRADIENT.type )
	{
		case GENERIC_GRADIENT_TYPE:
		case REFOCUS_GRADIENT_TYPE:
		case DEPHASE_GRADIENT_TYPE:
		case PHASE_GRADIENT_TYPE:
			_duration = grad->_as_GENERIC_GRADIENT.duration;
			break;	
		case SLICE_SELECT_GRADIENT_TYPE:
			_duration = grad->_as_SLICE_SELECT_GRADIENT.duration;
			break;	
		case READOUT_GRADIENT_TYPE:
			_duration = grad->_as_READOUT_GRADIENT.duration;
			break;	
		case FLOWCOMP_TYPE:
			_duration = grad->_as_FLOWCOMP_GRADIENT.duration;
			break;
		case KERNEL_MARKER_TYPE:
			_duration = 0.0;	
		default:
			_duration = 0.0;
			break;
	}
	return _duration;
}

/********************************************************************
* Function Name: getNumPoints
* Example:		getNumPoints( &ro_grad )
* Purpose: 	retrieves the number of points defined in the gradient 
*			waveform.
*			NOTE: This SGL extension includes and extra field in the
*			gradient structure so that the gradient type can be
*			automatically identified. This allows generic functions
*           to access the parameters of an arbitrary gradient without
*			having to cast the pointer.
* Input
*	Formal:	sglGradient	- pointer to a gradient structure
*	Private:	none
*	Public:	none
* Output
*	Return:	Number of data points in the gradient waveform (long)
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
long getNumPoints( SGL_GRADIENT_T	*grad )
{
	long _numPoints;
	
	switch ( grad->_as_GENERIC_GRADIENT.type )
	{
		case GENERIC_GRADIENT_TYPE:
		case REFOCUS_GRADIENT_TYPE:
		case DEPHASE_GRADIENT_TYPE:
		case PHASE_GRADIENT_TYPE:
			_numPoints = grad->_as_GENERIC_GRADIENT.numPoints;
			break;	
		case SLICE_SELECT_GRADIENT_TYPE:
			_numPoints = grad->_as_SLICE_SELECT_GRADIENT.numPoints;
			break;	
		case READOUT_GRADIENT_TYPE:
			_numPoints = grad->_as_READOUT_GRADIENT.numPoints;
			break;	
		case FLOWCOMP_TYPE:
			_numPoints = grad->_as_FLOWCOMP_GRADIENT.numPoints;
			break;	
		case KERNEL_MARKER_TYPE:
			_numPoints = 0;
			break;
		default:
			_numPoints = 0;
			break;
	}
	return _numPoints;
}

/********************************************************************
* Function Name: getDataPoints
* Example:		getDataPoints( &ro_grad )
* Purpose: 	retrieves a pointer to the data points defined in the gradient 
*			waveform.
*			NOTE: This SGL extension includes and extra field in the
*			gradient structure so that the gradient type can be
*			automatically identified. This allows generic functions
*           to access the parameters of an arbitrary gradient without
*			having to cast the pointer.
* Input
*	Formal:	sglGradient	- pointer to a gradient structure
*	Private:	none
*	Public:	none
* Output
*	Return:	pointer to the data points in the gradient waveform (double*)
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
double *getDataPoints( SGL_GRADIENT_T	*grad )
{
	double *_dataPoints;
	
	switch ( grad->_as_GENERIC_GRADIENT.type )
	{
		case GENERIC_GRADIENT_TYPE:
		case REFOCUS_GRADIENT_TYPE:
		case DEPHASE_GRADIENT_TYPE:
		case PHASE_GRADIENT_TYPE:
			_dataPoints = grad->_as_GENERIC_GRADIENT.dataPoints;
			break;	
		case SLICE_SELECT_GRADIENT_TYPE:
			_dataPoints = grad->_as_SLICE_SELECT_GRADIENT.dataPoints;
			break;	
		case READOUT_GRADIENT_TYPE:
			_dataPoints = grad->_as_READOUT_GRADIENT.dataPoints;
			break;	
		case FLOWCOMP_TYPE:
			_dataPoints = grad->_as_FLOWCOMP_GRADIENT.dataPoints;
			break;	
		case KERNEL_MARKER_TYPE:
			_dataPoints = NULL;
			break;
		default:
			_dataPoints = NULL;
			break;
	}
	return _dataPoints;
}

/********************************************************************
* Function Name: initOutputGradient
* Example:		initOutputGradient( &outputGradient, gradName,
*						duration, resolution)
* Purpose: 	initializes an outout gradient for a compound gradient axis
* Input
*	Formal:	outputGradient	- pointer to a output gradient
*				gradName
*				duration
*				resolution
*	Private:	none
*	Public:	none
* Output
*	Return:	none
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
void initOutputGradient( GENERIC_GRADIENT_T *outputGradient,
							char name[], double duration, double resolution )
{
	long _numBytes;
	long _i;
	
	outputGradient->type			= GENERIC_GRADIENT_TYPE;
	strcpy( outputGradient->name, name );
	
	outputGradient->resolution = resolution;

	outputGradient->numPoints = (long)ROUND(duration / resolution);
	_numBytes = sizeof(double)*outputGradient->numPoints;
	
	if (( outputGradient->dataPoints = (double *)malloc(_numBytes)) == NULL )
	{
		displayError(ERR_MALLOC, __FILE__,__FUNCTION__,__LINE__);
		return;
	}
	for( _i=0; _i<outputGradient->numPoints; _i++ )
	{
		outputGradient->dataPoints[_i] = 0.0;
	}
	outputGradient->amp = 0.0;
}

/********************************************************************
* Function Name: copyToOutputGradient
* Example:		copyToOutputGradient( &outputGradient, &gradientPoints,
*						numPoints, startTime, polarity )
* Purpose: 	initializes an output gradient for a compound gradient axis
* Input
*	Formal:	outputGradient	- pointer to a output gradient
*				gradientpoints - pointer to gradient points
*				numPoints
*				startTime
*				polarity
*	Private:	none
*	Public:	none
* Output
*	Return:	none
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
void copyToOutputGradient( GENERIC_GRADIENT_T *outputGradient, 
					double *gradientPoints, long numPoints,
					double startTime, SGL_CHANGE_POLARITY_T invert )
{
	long _startIndex;
	long _i;
	
	double _scale;
		
	_scale = invert == INVERT ? -1.0 : 1.0;
	
	_startIndex = (long)ROUND(startTime/outputGradient->resolution);
	if( _startIndex + numPoints > outputGradient->numPoints )
	{
		abort_message("copy_to_gradient: copy will overrun!");
	}
	for( _i=0; _i<numPoints; _i++ )
	{
		outputGradient->dataPoints[_i+_startIndex] += gradientPoints[_i]*_scale;
		outputGradient->amp = MAX( fabs(outputGradient->dataPoints[_i+_startIndex]),
											fabs(outputGradient->amp) );
	}
}

/********************************************************************
* Function Name: writeOutputGradient
* Example:		writeOutputGradient( &outputGradient, shift )
* Purpose: 	writes an output gradient for a compound gradient axis
* Input
*	Formal:	outputGradient	- pointer to a output gradient
*				shift
*	Private:	none
*	Public:	none
* Output
*	Return:	none
*	Formal:	none
*	Private:	none
*	Public:	none
* Notes:		none
*********************************************************************/
double writeOutputGradientDBStr(SGL_GRADIENT_T *aOutputGradient,
			double aStartTime, double aDuration, char *aGradParams )
{
   char *gradParams;
   char _tempname[MAX_STR];
   int	_error;
	int _startIdx, _endIdx;
	
	double *_vec;
	long _nPts;
	
	_vec = getDataPoints( aOutputGradient );
	_nPts = getNumPoints( aOutputGradient );
	
	_startIdx = (long)ROUND(aStartTime/GRADIENT_RES);
	_endIdx = (long)ROUND((aStartTime + aDuration)/GRADIENT_RES);

	if( _startIdx >= 0 && _endIdx <= _nPts )
	{
		_vec += _startIdx;
		_nPts = _endIdx - _startIdx;
	}
   
	gradParams = NULL;
	appendFormattedString( &gradParams, "%g %ld", getAmplitude(aOutputGradient), _nPts);

	if( aGradParams != NULL )
	{
		appendFormattedString( &gradParams, " %s", aGradParams );
	}

	if( !gradShapeWritten( getName(aOutputGradient), gradParams, _tempname ) )
	{
		setName( aOutputGradient, _tempname );
		_error = writeToDisk( _vec, _nPts, _vec[0],
      					getResolution(aOutputGradient), 1, getName(aOutputGradient) );
	}
	else
	{
		setName( aOutputGradient, _tempname );
	}
	
	return _nPts*GRADIENT_RES;
}

double getMaxAmplitude( GENERIC_GRADIENT_T  *gradient, double startTime, double duration, int *numPoints )
{
	int _i;
	int _startIdx, _endIdx;
	
	double _amp, _mag;
	
	_startIdx = (long)ROUND(startTime/GRADIENT_RES);
	_endIdx = (long)ROUND((startTime + duration)/GRADIENT_RES);

	*numPoints = _endIdx - _startIdx;

	_mag = 0.0;
	_amp = 0.0;
	for( _i = _startIdx; _i < _endIdx; _i++ )
	{
		_mag = fabs( gradient->dataPoints[_i]);
		_amp = _amp > _mag ? _amp : _mag;
	}
	return _amp;	
}

double writeOutputGradient(SGL_GRADIENT_T *outputGradient,
			double startTime, double duration)
{
	return writeOutputGradientDBStr( outputGradient, startTime, duration, "" );
}

double write_gradient( SGL_GRADIENT_T *outputGradient )
{
	return writeOutputGradient( outputGradient, 0.0, 0.0 );
}

double write_gradient_snippet( SGL_GRADIENT_T *outputGradient,
						double startTime, double duration )
{
	return writeOutputGradient( outputGradient, startTime, duration );
}


