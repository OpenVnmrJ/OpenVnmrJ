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
*     Revision 1.8  2006/11/10 02:20:36  mikem
*     fixed bug resulting from missing reeturn for get_timing()
*
*     Revision 1.7  2006/09/21 22:45:49  mikem
*     added include sglAdditions
*
*     Revision 1.3  2006/09/21 21:59:04  mikem
*     Added #include sglAdditions.h to top of file
*
*     Revision 1.2  2006/08/29 17:25:29  deans
*     1. changed sgl includes to use quotes (vs brackets)
*     2. added ncomm sources
*     3. mods to Makefile
*     4. added makenvpsg.sgl and makenvpsg.sgl.lnx
*
*     Revision 1.1  2006/08/23 14:10:00  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/08/22 23:30:04  deans
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
*     Revision 1.3  2006/07/07 20:10:18  deans
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
*     Revision 1.2  2006/07/07 01:11:38  mikem
*     modification to compile with psg
*
*   
* Contains additions to SGL by Michael L. Gyngell.
* 1) Functions to handle gradient lists
*
***************************************************************************/
#include "sglCommon.h"
#include "sglGradientLists.h"
#include "sglAdditions.h"

/********************************************************************
* Function Name: initGradList
* Example:		initGradList( &gradList )
* Purpose: 	initializes a gradient list
* Input
*	Formal:		gradList	- pointer to a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		none
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions: none
* Notes:		none
*********************************************************************/
void initGradList( struct SGL_GRAD_NODE_T **gradList )
{
	struct SGL_GRAD_NODE_T* _current = *gradList;
	struct SGL_GRAD_NODE_T* _next;

	while( _current != NULL )
	{
		_current->grad = NULL;

		_next = _current->next;
		freeString(&(_current->label));
		freeString(&(_current->refLabel));
		free(_current);
		_current = _next;
	}	
	
	*gradList = NULL;
}

/********************************************************************
* Function Name: addToGradList
* Example:		addToGradList( &gradList, (void *)&newGrad, "read"
*					logicalAxis, BEHIND, 0.001, "slice", polarity,
					&totalDuration)
* Purpose: 	adds a gradient event in a gradient list
* Input
*	Formal:	gradList	- pointer to a gradient list
*			newGrad		- pointer to a SGL gradient structure 
*			label		- a unique string that identifies the gradient event 
*			logicalAxis - either of READ, PHASE, SLICE to determine in which
*						  logical axis the gradient is to be played out
*			action		- gradient event placement action. his defines how the
*						  gradient event is timed with respect to the gradient
*						  event defined refLabel. by Takes the values
*						  START_TIME, BEFORE, BEHIND, SAME_START, SAME_END.
*			refLabel	- a string that refers to a gradient event already
*						  defined in the gradient list.
*			actionTime	- the timing of the gradient event.
*			polarity	- defines the polarity of the gradient event. Allowed
*						  values are PRESERVE or INVERT.
*	Private:	none
*	Public:		none
* Output
*	Return:		the end time of the new gradient event
*	Formal:	t	otalDuration	- total duration of the gradient list
*	Private:	none
*	Public:		none
* Error Conditions:
*	Abort:		label is undefined of empty
*	Abort:		label is already defined in list
*	Abort:		action requires a reference label
*	Abort:		action is not known
*	Abort:		logical axis is badly defined
*	Abort:		gradient duration is incorrectly granulated
*	Abort:		first gradient action must be START_TIME
* Notes:		none
*********************************************************************/

double addToGradList( struct SGL_GRAD_NODE_T **gradList,
							SGL_GRADIENT_T *newGrad,
							char *label,
							SGL_LOGICAL_AXIS_T logicalAxis,
							SGL_GRAD_PLACEMENT_ACTION_T action,
							char *refLabel,
							double actionTime,
							SGL_CHANGE_POLARITY_T invert, double *totalDuration )
{
	struct SGL_GRAD_NODE_T* _newNode = (struct SGL_GRAD_NODE_T*)malloc(sizeof(struct SGL_GRAD_NODE_T));
	SGL_GRAD_PLACEMENT_ACTION_T _action;	
	
	double _duration = 0.0;
	double _startTime = 0.0;

	struct SGL_GRAD_NODE_T* _existingNode;
	struct SGL_GRAD_NODE_T* _existingRefNode;

	_newNode->label = NULL;
	_newNode->refLabel = NULL;	

	_existingNode = NULL;
	_existingRefNode = NULL;
	_action = action;
	
	/* check for errors */

	if( (label == NULL) || (label[0] == 0) )
	{
		abort_message("addToGradList: label is undefined or empty");
	}

	if( strcmp( label, "START_OF_LIST" ) == 0 )
	{
		abort_message("addToGradList: START_OF_LIST is a reserved label");
	}

	if( strcmp( label, "END_OF_LIST" ) == 0 )
	{
		abort_message("addToGradList: END_OF_LIST is a reserved label");
	}

	if( strcmp( label, "START_OF_KERNEL" ) == 0 )
	{
		abort_message("addToGradList: START_OF_KERNEL is a reserved label");
	}

	if( strcmp( label, "END_OF_KERNEL" ) == 0 )
	{
		abort_message("addToGradList: END_OF_KERNEL is a reserved label");
	}

	if( (_action <= NO_GRAD_PLACEMENT_ACTION)||(_action >= NUM_GRAD_PLACEMENT_ACTIONS))
	{
		abort_message("addToGradList: action is not known");
	}

	if((logicalAxis < LOG_AXIS_MIN) || (logicalAxis > LOG_AXIS_MAX) )
	{
		abort_message("addToGradList: logical axis badly defined: %d", logicalAxis);
	}
	
	if(( *gradList == NULL ) && ( _action != START_TIME ))
	{
		/* we have a fatal error */
		abort_message("addToGradList: first gradient action must be START_TIME");
	}

	_newNode->grad = newGrad;
	_duration = getDuration( _newNode->grad );
//	if( granularity( _duration, GRADIENT_RES ) != _duration )
	if( FP_NEQ(granularity( _duration, GRADIENT_RES ), _duration ) )
	{
		/* we have a fatal error */
		abort_message("addToGradList: gradient duration is incorrectly granulated %d %g",
							1 + getNumListElements(*gradList), _duration);
	}
	
	/* BEHIND_LAST has two behaviours:
		if refLabel is empty or NULL or does not exist then last means the head node of the kernel.
		if refLabel is defined and exists in the kernel then the last entry with the root refLabel
			will be used.
	*/
	if( (refLabel == NULL ) || (strlen(refLabel) == 0 ) ) {

		if( _action == START_TIME ) {
			_newNode->refLabel = NULL;
		} else if( _action == BEHIND_LAST ) {
			_action = BEHIND;
			_newNode->refLabel = duplicateString( getHeadNodeLabel( *gradList ));
		} else {
			abort_message("addToGradList: action requires a reference label");
		}
	} else {
		if( (_existingRefNode = getNodeByLabel( (*gradList), refLabel )) == NULL ) {
			if( _action == BEHIND_LAST ) {
				_action = BEHIND;
				_newNode->refLabel = duplicateString( getHeadNodeLabel( *gradList ));
			} else {
			    abort_message("addToGradList: reference label %s is does not exist in kernel", refLabel);
			}
		} else {
			if( _action == BEHIND_LAST ) {
				_action = BEHIND;
				if( _existingRefNode->count == 0 ) {
					_newNode->refLabel = duplicateString( getHeadNodeLabel( *gradList ));
				} else {
					appendFormattedString( &(_newNode->refLabel), "%s%d", _existingRefNode->label, _existingRefNode->count );
				}
			} else {
				_action = action;
				_newNode->refLabel = duplicateString( refLabel );
			}
		}
	}	

	/* if a node exists with the same label and they have the same shape struct */
	if( ((_existingNode = getNodeByLabel( *gradList, label )) != NULL )	&& 
		( _existingNode->grad == _newNode->grad ) ) {
		_existingNode->count++;
		appendFormattedString( &_newNode->label,"%s%d",label,_existingNode->count);
	} else {
		_newNode->label = duplicateString( label );
	}		
	
	_newNode->action = _action;
	_newNode->count = 0;
	_newNode->logicalAxis = logicalAxis;
	_newNode->actionTime = actionTime;
	_newNode->invert = invert;

	processNodeAction( *gradList, _newNode );

	_newNode->next = (*gradList);
	(*gradList) = _newNode;
	
	*totalDuration = getDurationOfList( *gradList );

	if( sglEventDebug == PRINT_EVENT_TIMING )
	{
		displayNodeInfo( _newNode );
	}

	return _startTime + _duration;			
}

void displayNodeInfo( struct SGL_GRAD_NODE_T *gradNode )
{
	printf("label = %s\n",gradNode->label);
	printf("count = %d\n",gradNode->count);
	printf("logicalAxis = %d\n",gradNode->logicalAxis);
	printf("startTime = %.9f\n",gradNode->startTime);
	printf("action = %d\n",gradNode->action);
	printf("refLabel = %s\n",gradNode->refLabel);
	printf("actionTime = %.9f\n",gradNode->actionTime);
	printf("invert = %d\n",gradNode->invert);

	SGL_GRADIENT_TYPE_T type = getGradientTypeOfNode( gradNode );

	printf("%s start %g, end %g", gradNode->label, getStartTimeOfNode( gradNode ),
				getEndTimeOfNode( gradNode ));
	if( type == SLICE_SELECT_GRADIENT_TYPE )
	{
		printf(", rf start %g, rf center %g, rf end %g\n",
						getStartTimeOfRFPulseOfNode( gradNode ), 
						getEndTimeOfRFPulseOfNode( gradNode ), 
						getCenterOfRFPulseOfNode( gradNode )); 
	}
	else if( type == READOUT_GRADIENT_TYPE )
	{
		printf(", acq start %g, acq end %g, acq center %g\n",
						getStartTimeOfAcqOfNode( gradNode ), 
						getEndTimeOfAcqOfNode( gradNode ), 
						getCenterOfEchoOfNode( gradNode )); 
	}
	else
	{
		printf("\n");
	}
}

/********************************************************************
* Function Name: getDurationOfNode
* Example:	time = getDurationOfNode( &gradNode )
* Purpose: 	returns the duration of a node in a gradient list 
* Input
*	Formal:		node - pointer to a node in a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		duration of node
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions: none
* Notes:		none
*********************************************************************/
double getDurationOfNode( struct SGL_GRAD_NODE_T *gradNode )
{
	if( gradNode->grad == NULL ) {
		abort_message("getDurationOfNode: gradNode->grad is NULL");
	}
	return getDuration(gradNode->grad);
}

/********************************************************************
* Function Name: getStartTimeOfNode
* Example:	time = getStartTimeOfNode( &gradNode )
* Purpose: 	returns the start time of a node in a gradient list 
* Input
*	Formal:		node - pointer to a node in a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		start time of node
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions: none
* Notes:		none
*********************************************************************/
double getStartTimeOfNode( struct SGL_GRAD_NODE_T *gradNode )
{
	if( gradNode->grad == NULL ) {
		abort_message("getStartTimeOfNode: gradNode->grad is NULL");
	}
	return gradNode->startTime;
}

/********************************************************************
* Function Name: getEndTimeOfNode
* Example:	time = getEndTimeOfNode( &gradNode )
* Purpose: 	returns the end time of a gradient node 
* Input
*	Formal:		node - pointer to a node in a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		end time of node
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions: none
* Notes:		none
*********************************************************************/
double getEndTimeOfNode( struct SGL_GRAD_NODE_T *gradNode )
{
	if( gradNode->grad == NULL ) {
		abort_message("getStartTimeOfNode: gradNode->grad is NULL");
	}
	return getStartTimeOfNode(gradNode) + getDurationOfNode(gradNode);
}

/********************************************************************
* Function Name: getGradientTypeOfNode
* Example:	type = getGradientTypeOfNode( &gradNode )
* Purpose: 	returns the gradient type (SGL_GRADIENT_TYPE_T) of the
*			SGL gradient contained in a node of a gradient list 
* Input
*	Formal:		node - pointer to a node in a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		gradient type (SGL_GRADIENT_TYPE_T)
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions: none
* Notes:		none
*********************************************************************/
SGL_GRADIENT_TYPE_T getGradientTypeOfNode( struct SGL_GRAD_NODE_T *gradNode )
{
	if( gradNode->grad == NULL ) {
		abort_message("getGradientTypeOfNode: gradNode->grad is NULL");
	}
	return gradNode->grad->_as_READOUT_GRADIENT.type;
}

/********************************************************************
* Function Name: getStartTimeOfRFPulseOfNode
* Example:	time = getStartTimeOfRFPulseOfNode( &gradNode )
* Purpose: 	returns the start time of RF pulse of a slice selection 
*			gradient contained in a node of gradient list 
* Input
*	Formal:		node - pointer to a node in a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		start time of the RF pulse. The start time is referenced
*					to beginning of the gradient list.
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions:
*	Abort:		gradient type is not slice selction	
* Notes:		none
*********************************************************************/
double getStartTimeOfRFPulseOfNode ( struct SGL_GRAD_NODE_T *gradNode )
{
	SGL_GRADIENT_TYPE_T type;
	if( gradNode->grad == NULL ) {
		abort_message("getStartTimeOfRFPulseOfNode: gradNode->grad is NULL");
	}
	type = getGradientTypeOfNode( gradNode );

	if( type != SLICE_SELECT_GRADIENT_TYPE )
	{
		abort_message("getStartTimeOfRFPulseOfNode: gradient type must be slice select - type is %d",type);
	}
	return getStartTimeOfNode( gradNode )
				+ gradNode->grad->_as_SLICE_SELECT_GRADIENT.rfDelayFront;
}

/********************************************************************
* Function Name: getEndTimeOfRFPulseOfNode
* Example:	time = getEndTimeOfRFPulseOfNode( &gradNode )
* Purpose: 	returns the end time of RF pulse of a slice selection 
*			gradient contained in a node of a gradient list 
* Input
*	Formal:		node - pointer to a node in a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		end time of the RF pulse. The end time is referenced
*					to beginning of the gradient list.
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions:
*	Abort:		gradient type is not slice selction	
* Notes:		none
*********************************************************************/
double getEndTimeOfRFPulseOfNode ( struct SGL_GRAD_NODE_T *gradNode )
{
	SGL_GRADIENT_TYPE_T type;
	if( gradNode->grad == NULL ) {
		abort_message("getEndTimeOfRFPulseOfNode: gradNode->grad is NULL");
	}
	type = getGradientTypeOfNode( gradNode );

	if( type != SLICE_SELECT_GRADIENT_TYPE )
	{
		abort_message("getEndTimeOfRFPulseOfNode: gradient type must be slice select - type is %d",type);
	}
	return getEndTimeOfNode( gradNode )
				- gradNode->grad->_as_SLICE_SELECT_GRADIENT.rfDelayBack;
}

/********************************************************************
* Function Name: getCenterOfRFPulseOfNode
* Example:	time = getCenterOfRFPulseOfNode( &gradNode )
* Purpose: 	returns the time the RF center of RF pulse of a slice
*			selection gradient contained in a node of a gradient list 
* Input
*	Formal:		node - pointer to a node in a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		time of RF center of the RF pulse. The time is referenced
*					to beginning of the gradient list.
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions:
*	Abort:		abort if gradient type is not slice selction	
* Notes:		none
*********************************************************************/
double getCenterOfRFPulseOfNode( struct SGL_GRAD_NODE_T *gradNode )
{
	SGL_GRADIENT_TYPE_T type;
	if( gradNode->grad == NULL ) {
		abort_message("getCenterOfRFPulseOfNode: gradNode->grad is NULL");
	}
	type = getGradientTypeOfNode( gradNode );

	if( type != SLICE_SELECT_GRADIENT_TYPE )
	{
		abort_message("getEndTimeOfRFPulseOfNode: gradient type must be slice select - type is %d",type);
	}
	return getEndTimeOfNode( gradNode )
				- gradNode->grad->_as_SLICE_SELECT_GRADIENT.rfCenterBack;
}

/********************************************************************
* Function Name: getStartTimeOfAcqOfNode
* Example:	time = getStartTimeOfAcqOfNode( &gradNode )
* Purpose: 	returns the start time the acquire of a readout
*			gradient contained in a node of a gradient list 
* Input
*	Formal:		node - pointer to a node in a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		start time of acquisition. The time is referenced
*					to beginning of the gradient list.
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions:
*	Abort:		abort if gradient type is not readout	
* Notes:		none
*********************************************************************/
double getStartTimeOfAcqOfNode ( struct SGL_GRAD_NODE_T *gradNode )
{
	SGL_GRADIENT_TYPE_T type;
	if( gradNode->grad == NULL ) {
		abort_message("getStartTimeOfAcqOfNode: gradNode->grad is NULL");
	}
	type = getGradientTypeOfNode( gradNode );

	if( type != READOUT_GRADIENT_TYPE )
	{
		abort_message("getStartTimeOfAcqOfNode: gradient type must be readout - type is %d", type);
	}
	return getStartTimeOfNode( gradNode )
				+ gradNode->grad->_as_READOUT_GRADIENT.atDelayFront;
}

/********************************************************************
* Function Name: getEndTimeOfAcqOfNode
* Example:	time = getEndTimeOfAcqOfNode( &gradNode )
* Purpose: 	returns the end time the acquire of a readout
*			gradient contained in a node in a gradient list 
* Input
*	Formal:		node - pointer to a node in a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		end time of acquisition. The time is referenced
*					to beginning of the gradient list.
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions:
*	Abort:		abort if gradient type is not readout	
* Notes:		none
*********************************************************************/
double getEndTimeOfAcqOfNode ( struct SGL_GRAD_NODE_T *gradNode )
{
	SGL_GRADIENT_TYPE_T type;
	if( gradNode->grad == NULL ) {
		abort_message("getEndTimeOfAcqOfNode: gradNode->grad is NULL");
	}
	type = getGradientTypeOfNode( gradNode );

	if( type != READOUT_GRADIENT_TYPE )
	{
		abort_message("getEndTimeOfAcqOfNode: gradient type must be readout - type is %d", type);
	}
	return getEndTimeOfNode( gradNode )
				- gradNode->grad->_as_READOUT_GRADIENT.atDelayBack;
}

/********************************************************************
* Function Name: getCenterOfEchoOfNode
* Example:	time = getCenterOfEchoOfNode( &gradNode )
* Purpose: 	returns the time of the echo center of a readout
*			gradient contained in a gradient node 
* Input
*	Formal:		node - pointer to a node in a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		echo cente time of acquisition. The time is referenced
*					to beginning of the gradient list.
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions:
*	Abort:	abort is gradient type is not readout	
* Notes:		none
*********************************************************************/
double getCenterOfEchoOfNode ( struct SGL_GRAD_NODE_T *gradNode )
{
	SGL_GRADIENT_TYPE_T type;
	if( gradNode->grad == NULL ) {
		abort_message("getCenterOfEchoOfNode: gradNode->grad is NULL");
	}
	type = getGradientTypeOfNode( gradNode );

	if( type != READOUT_GRADIENT_TYPE )
	{
		abort_message("getEndTimeOfAcqOfNode: gradient type must be readout - type is %d", type);
	}
	return getStartTimeOfNode( gradNode )
				+ gradNode->grad->_as_READOUT_GRADIENT.timeToEcho;
}

double getMomentOfNode( struct SGL_GRAD_NODE_T *aGradNode )
{
	double moment=0.0;
	if( aGradNode->grad ) {
		switch( getGradientTypeOfNode( aGradNode ) ) 
		{
			case GENERIC_GRADIENT_TYPE:
			case REFOCUS_GRADIENT_TYPE:
			case DEPHASE_GRADIENT_TYPE:
			case PHASE_GRADIENT_TYPE:
				moment = aGradNode->grad->_as_GENERIC_GRADIENT.m0;
				break;
			case SLICE_SELECT_GRADIENT_TYPE:
				moment = aGradNode->grad->_as_SLICE_SELECT_GRADIENT.m0;
				break;
			case READOUT_GRADIENT_TYPE:
				moment = aGradNode->grad->_as_READOUT_GRADIENT.m0;
				break;
		}
	} else {
		moment = 0;
	}
	return moment;
}


/********************************************************************
* Function Name: getAmplitudeOfAxis
* Example:		getAmplitudeOfAxis( &gradList, READ )
* Purpose: 	retrieves the maximum amplitude of a gradient list
* Input
*	Formal:		gradList - pointer to a gradient list
*				logicalAxis
*	Private:	none
*	Public:		none
* Output
*	Return:		Gradient amplitude of logical axis of list
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions: none
* Notes:		none
*********************************************************************/
double getAmplitudeOfAxis( struct SGL_GRAD_NODE_T *gradList,
								SGL_LOGICAL_AXIS_T logicalAxis )
{
	double _amp = 0;
	struct SGL_GRAD_NODE_T* _current = gradList;
	double _temp;
	int _count = 0;
	
	while( _current != NULL )
	{
		if( _current->logicalAxis == logicalAxis )
		{
			_temp = getAmplitude( _current->grad );
			_amp = (_count == 0) ? _temp : ( _temp > _amp ? _temp : _amp );
			_count++;
		}
		_current = _current->next;
	}
	return _amp;
}

/********************************************************************
* Function Name: getStartTimeOfList
* Example:		getStartTimeOfList( &gradList )
* Purpose: 	retrieves the start time of a gradient list
* Input
*	Formal:		gradList - pointer to a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		start time of the gradient list
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions: none
* Notes:		none
*********************************************************************/
double getStartTimeOfList( struct SGL_GRAD_NODE_T *gradList )
{
	double _startTime = 0;
	struct SGL_GRAD_NODE_T* _current = gradList;
	int _count = 0;
	
	while( _current != NULL )
	{
		_startTime = (_count == 0) ? _current->startTime :
					( _current->startTime < _startTime ? _current->startTime : _startTime );
		_count++;
		_current = _current->next;
	}
	return _startTime;
}

/********************************************************************
* Function Name: getEndTimeOfList
* Example:		getEndTimeOfList( &gradList )
* Purpose: 	retrieves the end time of a gradient list
* Input
*	Formal:		gradList - pointer to a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		end time of gradient list
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions: none
* Notes:		none
*********************************************************************/
double getEndTimeOfList( struct SGL_GRAD_NODE_T *gradList )
{
	double _endTime = 0;
	double _temp;
	struct SGL_GRAD_NODE_T* _current = gradList;
	int _count = 0;
	
	while( _current != NULL )
	{
		_temp = _current->startTime + getDuration( _current->grad );
		_endTime = (_count == 0) ?
							_temp :( _temp > _endTime ? _temp : _endTime );
		_count++;
		_current = _current->next;
	}
	return _endTime;
}

/********************************************************************
* Function Name: getDurationOfList
* Example:		getDurationOfList( &gradList )
* Purpose: 	retrieves the duration of a gradient list
* Input
*	Formal:		gradList - pointer to a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		duration of gradient list
*	Formal:		none
*	Private:	none
*	Public:		none
* Error Conditions: none
* Notes:		none
*********************************************************************/
double getDurationOfList( struct SGL_GRAD_NODE_T *gradList )
{
	return getEndTimeOfList( gradList ) - getStartTimeOfList( gradList );
}

/********************************************************************
* Function Name: getNumListElements
* Example:		getNumListElements( &gradList )
* Purpose: 		retrieves the number of elements in a gradient list
* Input
*	Formal:		gradList - pointer to a compound gradient
*	Private:	none
*	Public:		none
* Output
*	Return:		numListElements
*	Formal:		none
*	Private:	none
*	Public:	none
* Error Conditions: none
* Notes:		none
*********************************************************************/
int getNumListElements( struct SGL_GRAD_NODE_T *gradList )
{
	int _count = 0;
	struct SGL_GRAD_NODE_T* _current = gradList;

	while( _current != NULL )
	{
		_count++;
		_current = _current->next;
	}	
	return( _count );
}

/********************************************************************
* Function Name: getNodeByLabel
* Example:		getNodeByLabel( &gradList , "slice")
* Purpose: 		return the pointer to a gradient node of a compound gradient
* Input
*	Formal:		gradList	- pointer to a compound gradient
*				label 		- string contain the name of the node
*	Private:	none
*	Public:		none
* Output
*	Return:		pointer to the node, or NULL if the label is unknown
*	Formal:	none
*	Private:	none
*	Public:	none
* Error Conditions: none
* Notes:		none
*********************************************************************/
struct SGL_GRAD_NODE_T *getNodeByLabel( struct SGL_GRAD_NODE_T *gradList, char *label )
{
	struct SGL_GRAD_NODE_T* _current = gradList;
	
	while( _current != NULL )
	{
		if( strcmp(_current->label,label) == 0 ) return _current;
		
		_current = _current->next;
	}
	return NULL;
}

/********************************************************************
* Function Name: getHeadNodeLabel
* Example:		getHeadNodeLabel( &gradList , "slice")
* Purpose: 		return the pointer to a gradient node of a compound gradient
* Input
*	Formal:		gradList	- pointer to a compound gradient
*	Private:	none
*	Public:		none
* Output
*	Return:		label of the head node of a gradient list
*	Formal:	none
*	Private:	none
*	Public:	none
* Error Conditions: none
* Notes:		none
*********************************************************************/
char *getHeadNodeLabel( struct SGL_GRAD_NODE_T *gradList )
{
	return gradList->label;
}

/********************************************************************
* Function Name: processNodeAction
* Example:	processNodeAction( &gradList , node )
* Purpose: 	sets the start time of a node based on the timing action
*			for this node. 
* Input
*	Formal:	gradList	- pointer to a compound gradient
*			label - string contain the name of the node
*	Private:	none
*	Public:	none
* Output
*	Return:	gradListDuration
*	Formal:	none
*	Private:	none
*	Public:	none
* Error Conditions:
*	Abort:		no gradient found with the reference label	
*	Abort:		bad action	
*	Abort:		gradient startTime is incorrectly granulated	
* Notes:		none
*********************************************************************/
void processNodeAction( struct SGL_GRAD_NODE_T *gradList,
						struct SGL_GRAD_NODE_T *node )
{
	struct SGL_GRAD_NODE_T *_refNode = 0;

	if( node->action != START_TIME )
	{
		_refNode = getNodeByLabel(gradList, node->refLabel);
		if( _refNode == NULL )
		{
			/* we have a fatal error */
			abort_message("processNodeAction: no gradient found with referencelabel %s",
					node->refLabel);
		}			
	}
	
	switch( node->action )
	{
		case START_TIME:
			node->startTime = node->actionTime;
			break;
		case BEFORE:
			node->startTime = _refNode->startTime - getDurationOfNode(node) - node->actionTime;
			break;
		case BEHIND:
			node->startTime = _refNode->startTime + getDurationOfNode(_refNode) + node->actionTime;
			break;
		case SAME_START:
			node->startTime = _refNode->startTime + node->actionTime;
			break;
		case SAME_END:
			node->startTime = _refNode->startTime + getDurationOfNode(_refNode)
						- getDurationOfNode(node) + node->actionTime;
			break;
		default:
			abort_message("processNodeAction: bad action");
			/* we have a fatal error */
			break;
	}

//	if( granularity( node->startTime, GRADIENT_RES ) != node->startTime )
	if( FP_NEQ(granularity( node->startTime, GRADIENT_RES ),node->startTime) )
	{
		abort_message("processNodeAction: \"%s\" gradient startTime %f is incorrectly granulated",node->label,node->startTime);
	}
}

/********************************************************************
* Function Name: processListActions
* Example:	processListActions( &gradList )
* Purpose: 	sets the start times of all nodes in a gradient list
* Input
*	Formal:		gradList - pointer to a gradient list
*	Private:	none
*	Public:		none
* Output
*	Return:		none
*	Formal:		none
*	Private:	none
*	Public:	none
* Error Conditions: none
* Notes:		none
*********************************************************************/
void processListActions( struct SGL_GRAD_NODE_T *gradList )
{
	struct SGL_GRAD_NODE_T* _current = gradList;
	static struct SGL_GRAD_NODE_T** _nl = 0;

	static int _nlSize = 0;
	
	int _listSize;
	int _ctr;
	int i;
	
	_listSize = getNumListElements( gradList );
	_ctr = _listSize;

	if( _nlSize < _listSize )
	{
		free( _nl );
		_nlSize = _listSize;
		_nl = (struct SGL_GRAD_NODE_T**)malloc(_nlSize*sizeof(struct SGL_GRAD_NODE_T*));
	}
	
	while( 0 <= --_ctr )
	{
		_nl[_ctr] = _current;
		_current = _current->next;
	}

	for( i = 0; i < _listSize; i++ )
	{
		processNodeAction( gradList, _nl[i] );
	}
}

/********************************************************************
* Function Name: changeTimingOfNode
* Example:	changeTimingOfNode( &gradList, "read", newTime, &totalDuration )
* Purpose: 	change the timing of a node
* Input
*	Formal:		gradList - pointer to a gradient list
*				label		- label of node for the timing change
*				newTime		- new time of node
*	Private:	none
*	Public:		none
* Output
*	Return:		total duration of the gradient list
*	Formal:		none
*	Private:	none
*	Public:	none
* Error Conditions:
*	Abort:		label is not known
* Notes:		none
*********************************************************************/
void changeTimingOfNode( struct SGL_GRAD_NODE_T *head, char *label,
							double incrementTime, double *totalDuration )
{
	struct SGL_GRAD_NODE_T *node;
	
	node = getNodeByLabel( head, label );
	if( node == NULL )
	{
		abort_message("changeTimingOfNode: label is not known");
	}
	
	node->actionTime += incrementTime;
	
	processListActions( head );

	*totalDuration = getDurationOfList( head );
	
	if( sglEventDebug == PRINT_EVENT_TIMING )
	{
		displayNodeInfo( node );
	}	
}

void setTimingOfNode( struct SGL_GRAD_NODE_T *head, char *label,
							double newTime, double *totalDuration )
{
	struct SGL_GRAD_NODE_T *node;
	
	node = getNodeByLabel( head, label );
	if( node == NULL )
	{
		abort_message("setTimingOfNode: label is not known");
	}
	
	node->actionTime = newTime;
	
	processListActions( head );

	*totalDuration = getDurationOfList( head );
	
	if( sglEventDebug == PRINT_EVENT_TIMING )
	{
		displayNodeInfo( node );
	}	
}

void setPolarity( struct SGL_GRAD_NODE_T *head, char *label, SGL_CHANGE_POLARITY_T newPolarity )
{
	struct SGL_GRAD_NODE_T *node;

	node = getNodeByLabel( head, label );
	if( node == NULL )
	{
		abort_message("getPolarity: label is not known");
	}
	
	node->invert = newPolarity;

	if( sglEventDebug == PRINT_EVENT_TIMING )
	{
		displayNodeInfo( node );
	}	
}

SGL_CHANGE_POLARITY_T getPolarity( struct SGL_GRAD_NODE_T *head, char *label )
{
	struct SGL_GRAD_NODE_T *node;

	node = getNodeByLabel( head, label );
	if( node == NULL )
	{
		abort_message("getPolarity: label is not known");
	}
	return node->invert;	
}

/********************************************************************
* Function Name: getTiming
* Example:	getTiming( &gradList, FROM, "slice", TO, "read" &overlapFlag)
* Purpose: 	get timing between events in a gradient list
* Input
*	Formal:		gradList 	- pointer to a gradient list
*				startAction - action for starting label
*				startLabel	- label of starting node
*				endAction	- action for ending label
*				endLabel	- label of ending node
*	Private:	none
*	Public:		none
* Output
*	Return:		timing
*	Formal:		overlapFlag
*	Private:	none
*	Public:	none
* Error Conditions:
*	Abort:		start label is not known
*	Abort:		end label is not known
*	Abort:		illegal timing action for start action
*	Abort:		illegal timing action for end action
* Notes:
*********************************************************************/
double getTiming( struct SGL_GRAD_NODE_T *gradList,
						SGL_TIMING_ACTION_T startAction, char *startLabel,
						SGL_TIMING_ACTION_T endAction, char *endLabel, int *overlapFlag )
{
	struct SGL_GRAD_NODE_T *startNode;
	struct SGL_GRAD_NODE_T *endNode;
	
	double _startTime, _fStart, _sStart;
	double _endTime, _fEnd, _sEnd;
	double _time;
	int _extraProcF, _extraProcS;
	
	_extraProcF = 0;
	_extraProcS = 0;
	_startTime = 0;
	_endTime = 0;
	_fStart = 0;
	_fEnd = 0;
	_sStart = 0;
	_sEnd = 0;
	*overlapFlag = 0;
	
	if( (strcmp( startLabel, "START_OF_KERNEL" ) == 0 ) ||
		(strcmp( startLabel, "START_OF_LIST" ) == 0 ) ){
		_startTime = getStartTimeOfList( gradList );

		if( endAction == NO_TIMING_ACTION ) {
			return _startTime;
		}
	} else {
		startNode = getNodeByLabel( gradList, startLabel );
		if( startNode == NULL )
		{
			abort_message("getTiming: start label is not known");
		}
		switch( startAction )
		{
			case START_TIME_OF:
				return getStartTimeOfNode( startNode );
				break;
			case END_TIME_OF:
				return getEndTimeOfNode( startNode );
				break;
			case FROM:
				_fStart = getStartTimeOfNode( startNode );
				_fEnd = getEndTimeOfNode( startNode );
				_extraProcF = 1;
				break;		
			case FROM_RF_PULSE_OF:
				_fStart = getStartTimeOfRFPulseOfNode( startNode );
				_fEnd = getEndTimeOfRFPulseOfNode( startNode );
				_extraProcF = 1;
				break;
			case FROM_RF_CENTER_OF:
				_startTime = getCenterOfRFPulseOfNode( startNode );
				break;
			case FROM_ACQ_OF:
				_fStart = getStartTimeOfAcqOfNode( startNode );
				_fEnd = getEndTimeOfAcqOfNode( startNode );
				_extraProcF = 1;
				break;
			case FROM_ECHO_OF:
				_startTime = getCenterOfEchoOfNode( startNode );
				break;
			case FROM_START_OF:
				_startTime = getStartTimeOfNode( startNode );
				break;
			case FROM_END_OF:
				_startTime = getEndTimeOfNode( startNode );
				break;
			case FROM_START_OF_RF_PULSE_OF:
				_startTime = getStartTimeOfRFPulseOfNode( startNode );
				break;
			case FROM_END_OF_RF_PULSE_OF:
				_startTime = getEndTimeOfRFPulseOfNode( startNode );
				break;
			case FROM_START_OF_ACQ_OF:
				_startTime = getStartTimeOfAcqOfNode( startNode );
				break;
			case FROM_END_OF_ACQ_OF:
				_startTime = getEndTimeOfAcqOfNode( startNode );
				break;
			default:
				abort_message("getTiming: illegal timing action for start action!");
				break;
		}
	}

	if( (strcmp( endLabel, "END_OF_KERNEL" ) == 0 ) ||
		(strcmp( endLabel, "END_OF_LIST" ) == 0 ) ){
		_endTime = getEndTimeOfList( gradList );
	} else {
		endNode = getNodeByLabel( gradList, endLabel );
		if( endNode == NULL )
		{
			abort_message("getTiming: end label is not known = %s", endLabel);
		}

		switch( endAction )
		{
			case TO_START_OF:
				_endTime = getStartTimeOfNode( endNode );
				break;
			case TO_END_OF:
				_endTime = getEndTimeOfNode( endNode );
				break;
			case TO:
				_sStart = getStartTimeOfNode( endNode );
				_sEnd = getEndTimeOfNode( endNode );
				_extraProcS = 1;
				break;		
			case TO_RF_PULSE_OF:
				_sStart = getStartTimeOfRFPulseOfNode( endNode );
				_sEnd = getEndTimeOfRFPulseOfNode( endNode );
				_extraProcS = 1;
				break;
			case TO_RF_CENTER_OF:
				_endTime = getCenterOfRFPulseOfNode( endNode );
				break;
			case TO_ACQ_OF:
				_sStart = getStartTimeOfAcqOfNode( endNode );
				_sEnd = getEndTimeOfAcqOfNode( endNode );
				_extraProcS = 1;
				break;
			case TO_ECHO_OF:
				_endTime = getCenterOfEchoOfNode( endNode );
				break;
			case TO_START_OF_RF_PULSE_OF:
				_endTime = getStartTimeOfRFPulseOfNode( endNode );
				break;
			case TO_END_OF_RF_PULSE_OF:
				_endTime = getEndTimeOfRFPulseOfNode( endNode );
				break;
			case TO_START_OF_ACQ_OF:
				_endTime = getStartTimeOfAcqOfNode( endNode );	
				break;
			case TO_END_OF_ACQ_OF:
				_endTime = getEndTimeOfAcqOfNode( endNode );	
				break;
			default:
				abort_message("getTiming: illegal timing action for end action!");
				break;
		}
	}
	if( !_extraProcF && !_extraProcS )
	{
		_time = _endTime - _startTime;
	}
	else if( _extraProcF && !_extraProcS )
	{
		/* first has two values */
//		if( _endTime > _fEnd )
		if( FP_GT(_endTime,_fEnd ) )
		{
			_time = _endTime - _fEnd; /* +ve */
		}
//		else if ( _endTime < _fStart )
		else if ( FP_LT(_endTime,_fStart) )
		{
			_time =  _fStart - _endTime; /* -ve */
		}
		else
		{
			/* second event lies within first */
			_time = _endTime - _fStart;
			*overlapFlag = 1;
		}
	}
	else if( !_extraProcF && _extraProcS )
	{
		/* second has two values */
//		if( _startTime < _sStart )
		if( FP_LT(_startTime,_sStart) )
		{
			_time = _sStart - _startTime; /* +ve */
		}
//		else if ( _startTime > _sEnd )
		else if ( FP_GT(_startTime,_sEnd) )
		{
			_time = _startTime - _sEnd; /* -ve */
		}
		else
		{
			/* first event lies within second */
			_time = _startTime - _sStart; /* -ve */
			*overlapFlag = 1;
		}
	}
	else
	{
//		if( _fEnd < _sStart )
		if( FP_LT(_fEnd,_sStart ) )
		{
			_time = _sStart - _fEnd; /* +ve */
		}
//		else if( _fStart > _sEnd )
		else if( FP_GT(_fStart, _sEnd) )
		{
			_time = _sEnd - _fStart; /* -ve */
		}
		else 
		{
			/* events overlap so set a flag to warn of this */
			_time = _sStart - _fStart;
			*overlapFlag = 1;
		}
	}
	return _time;
}

/********************************************************************
* Function Name: writeGradList
* Example:		writeGradList( &gradList, readFileName, phaseFileName
*							sliceFileName, 0.0, 0.0,
*							&readAmp, &phaseAmp, &sliceAmp )
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
double writeGradList( struct SGL_GRAD_NODE_T* aGradList,
							char *aReadName, char *aPhaseName, char *aSliceName,
							char *aReadName2, char *aPhaseName2, char *aSliceName2,
							double aStartTime, double aDuration,
							double *aReadAmp, double *aPhaseAmp, double *aSliceAmp,
							double *aReadAmp2, double *aPhaseAmp2, double *aSliceAmp2 )
{
	double _startTime, _duration;
	struct SGL_GRAD_NODE_T* _current = aGradList;
	double _relStart;
	
	GENERIC_GRADIENT_T slice, read, phase;
	GENERIC_GRADIENT_T slice2, read2, phase2;
	
	/* init the database strings */
	
	_startTime = getStartTimeOfList( aGradList );
	_duration = getDurationOfList( aGradList );

	if( FP_LT(aStartTime, 0 ))
	{
		abort_message("writeGradList: bad start time!");
	}
	if( FP_GT(aStartTime + aDuration, _duration ))
	{
		abort_message("writeGradList: bad duration %.12f %.12f",aStartTime + aDuration ,_duration);
	}
	/* construct arrays of zeroes */

	if( aReadName != NULL ) initOutputGradient( &read, aReadName, _duration, GRADIENT_RES );
	if( aReadName2 != NULL ) initOutputGradient( &read2, aReadName2, _duration, GRADIENT_RES );
	if( aPhaseName != NULL ) initOutputGradient( &phase, aPhaseName, _duration, GRADIENT_RES );	
	if( aPhaseName2 != NULL ) initOutputGradient( &phase2, aPhaseName2, _duration, GRADIENT_RES );	
	if( aSliceName != NULL ) initOutputGradient( &slice, aSliceName, _duration, GRADIENT_RES );
	if( aSliceName2 != NULL ) initOutputGradient( &slice2, aSliceName2, _duration, GRADIENT_RES );

	while( _current != NULL )
	{
		_relStart = _current->startTime - _startTime;

		switch( _current->logicalAxis )
		{
			case READ:
				if( aReadName != NULL ) {
					copyToOutputGradient( &read, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			case READ_2:
				if( aReadName2 != NULL ) {
					copyToOutputGradient( &read2, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			case PHASE:
				if( aPhaseName != NULL ) {
					copyToOutputGradient( &phase, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			case PHASE_2:
				if( aPhaseName2 != NULL ) {
					copyToOutputGradient( &phase2, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			case SLICE:
				if( aSliceName != NULL ) {
					copyToOutputGradient( &slice, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			case SLICE_2:
				if( aSliceName2 != NULL ) {
					copyToOutputGradient( &slice2, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			default:
				break;
		}
		_current = _current->next;
	}

	if( aReadName != NULL ) {
		writeCompoundGradient( (void *)&read, aStartTime, aDuration, aReadAmp );
		strcpy( aReadName, read.name );
		free(read.dataPoints);
    }
	if( aReadName2 != NULL ) {
		writeCompoundGradient( (void *)&read2, aStartTime, aDuration, aReadAmp2 );
		strcpy( aReadName2, read2.name );
		free(read2.dataPoints);
    }
	if( aPhaseName != NULL ) {
		writeCompoundGradient( (void *)&phase, aStartTime, aDuration, aPhaseAmp );
		strcpy( aPhaseName, phase.name );
		free(phase.dataPoints);
	}
	if( aPhaseName2 != NULL ) {
		writeCompoundGradient( (void *)&phase2, aStartTime, aDuration, aPhaseAmp2 );
		strcpy( aPhaseName2, phase2.name );
		free(phase2.dataPoints);
	}
	if( aSliceName != NULL ) {
		writeCompoundGradient( (void *)&slice, aStartTime, aDuration, aSliceAmp );
		strcpy( aSliceName, slice.name );
		free(slice.dataPoints);
	}
	if( aSliceName2 != NULL ) {
		writeCompoundGradient( (void *)&slice2, aStartTime, aDuration, aSliceAmp2 );
		strcpy( aSliceName2, slice2.name );
		free(slice2.dataPoints);
	}
	return aDuration;
}

double writeGradListKI( struct SGL_GRAD_NODE_T* aGradList,
						SGL_KERNEL_INFO_T *aRead, SGL_KERNEL_INFO_T *aPhase, SGL_KERNEL_INFO_T *aSlice,
						SGL_KERNEL_INFO_T *aRead2, SGL_KERNEL_INFO_T *aPhase2, SGL_KERNEL_INFO_T *aSlice2,
						double aStartTime, double aDuration )
{
	double _startTime, _duration;
	struct SGL_GRAD_NODE_T* _current = aGradList;
	double _relStart;
	
	GENERIC_GRADIENT_T slice, read, phase;
	GENERIC_GRADIENT_T slice2, read2, phase2;
	
	/* init the database strings */
	
	_startTime = getStartTimeOfList( aGradList );
	_duration = getDurationOfList( aGradList );

	if( FP_LT(aStartTime, 0 ))
	{
		abort_message("writeGradList: bad start time!");
	}
	if( FP_GT(aStartTime + aDuration, _duration ))
	{
		abort_message("writeGradList: bad duration %.12f %.12f",aStartTime + aDuration ,_duration);
	}
	/* construct arrays of zeroes */

	if( aRead != NULL ) initOutputGradient( &read, aRead->name, _duration, GRADIENT_RES );
	if( aRead2 != NULL ) initOutputGradient( &read2, aRead2->name, _duration, GRADIENT_RES );
	if( aPhase != NULL ) initOutputGradient( &phase, aPhase->name, _duration, GRADIENT_RES );	
	if( aPhase2 != NULL ) initOutputGradient( &phase2, aPhase2->name, _duration, GRADIENT_RES );	
	if( aSlice != NULL ) initOutputGradient( &slice, aSlice->name, _duration, GRADIENT_RES );
	if( aSlice2 != NULL ) initOutputGradient( &slice2, aSlice2->name, _duration, GRADIENT_RES );

	while( _current != NULL )
	{
		_relStart = _current->startTime - _startTime;

		switch( _current->logicalAxis )
		{
			case READ:
				if( aRead != NULL ) {
					copyToOutputGradient( &read, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			case READ_2:
				if( aRead2 != NULL ) {
					copyToOutputGradient( &read2, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			case PHASE:
				if( aPhase != NULL ) {
					copyToOutputGradient( &phase, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			case PHASE_2:
				if( aPhase2 != NULL ) {
					copyToOutputGradient( &phase2, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			case SLICE:
				if( aSlice != NULL ) {
					copyToOutputGradient( &slice, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			case SLICE_2:
				if( aSlice2 != NULL ) {
					copyToOutputGradient( &slice2, getDataPoints( _current->grad ) ,
							getNumPoints( _current->grad ), _relStart, _current->invert );
				}
				break;
			default:
				break;
		}
		_current = _current->next;
	}

	if( aRead != NULL ) {
		writeCompoundGradient( (void *)&read, aStartTime, aDuration, &(aRead->amp) );
		replaceString( &(aRead->name), read.name );
		aRead->dur = aDuration;
		free(read.dataPoints);
    }
	if( aRead2 != NULL ) {
		writeCompoundGradient( (void *)&read2, aStartTime, aDuration, &(aRead2->amp) );
		replaceString( &(aRead2->name), read2.name );
		aRead2->dur = aDuration;
		free(read2.dataPoints);
    }
	if( aPhase != NULL ) {
		writeCompoundGradient( (void *)&phase, aStartTime, aDuration, &(aPhase->amp) );
		replaceString( &(aPhase->name), phase.name );
		aPhase->dur = aDuration;
		free(phase.dataPoints);
	}
	if( aPhase2 != NULL ) {
		writeCompoundGradient( (void *)&phase2, aStartTime, aDuration, &(aPhase2->amp) );
		replaceString( &(aPhase2->name), phase2.name );
		aPhase2->dur = aDuration;
		free(phase2.dataPoints);
	}
	if( aSlice != NULL ) {
		writeCompoundGradient( (void *)&slice, aStartTime, aDuration, &(aSlice->amp) );
		replaceString( &(aSlice->name), slice.name );
		aSlice->dur = aDuration;
		free(slice.dataPoints);
	}
	if( aSlice2 != NULL ) {
		writeCompoundGradient( (void *)&slice2, aStartTime, aDuration, &(aSlice2->amp) );
		replaceString( &(aSlice2->name), slice2.name );
		aSlice2->dur = aDuration;
		free(slice2.dataPoints);
	}
	return aDuration;
}


void writeCompoundGradient( SGL_GRADIENT_T *aGradient, double aStartTime, double aDuration, double *aAmplitude )
{
	char *dbStr;
	int numPoints;
	dbStr = NULL;
	*aAmplitude = getMaxAmplitude( (void *)aGradient, aStartTime, aDuration, &numPoints);
	appendFormattedString( &dbStr, "%g %ld",*aAmplitude, numPoints );
	writeOutputGradientDBStr( (void *)aGradient, aStartTime, aDuration, dbStr );
	freeString( &dbStr );
}





void printListLabels( struct SGL_GRAD_NODE_T *gradList )
{
	struct SGL_GRAD_NODE_T* _current = gradList;
	
	while( _current != NULL )
	{
		printf("%s\n",_current->label );
		_current = _current->next;
	}
}

void set_comp_info( SGL_KERNEL_INFO_T *aInfo, char *aName )
{
//	freeString( &(aInfo->name) );
	aInfo->name = duplicateString( aName );
}


/********************************************************************
 * gradient list wrappers
 *******************************************************************/
char *lastKernelLabel( void )
{
	return (*workingList)->label;
}


void activate_grad_list( struct SGL_GRAD_NODE_T **gradList )
{
	workingList = gradList;
}

void activate_kernel( struct SGL_GRAD_NODE_T **aKernel )
{
	workingList = aKernel;
}

void start_grad_list( struct SGL_GRAD_NODE_T **gradList )
{
	activate_grad_list( gradList );
	initGradList( workingList );
}

void start_kernel( struct SGL_GRAD_NODE_T **aKernel )
{
	activate_kernel( aKernel );
	initGradList( workingList );
}

double get_timing( SGL_TIMING_ACTION_T startAction, char *startName,
				SGL_TIMING_ACTION_T endAction, char *endName )
{
	return getTiming( *workingList, startAction, startName, endAction, endName, &gradEventsOverlap );
}

double get_time( char *startName )
{
	return getTiming( *workingList, START_TIME_OF, startName, NO_TIMING_ACTION, "", &gradEventsOverlap );
}

double grad_list_duration()
{
	return getDurationOfList( *workingList );
}

double kernel_duration()
{
	return getDurationOfList( *workingList );
}

void change_timing( char *label, double newTime )
{
	double _totalDuration;

	changeTimingOfNode( *workingList, label, newTime, &_totalDuration );
}

void set_timing( char *label, double incrementTime )
{
	double _totalDuration;

	setTimingOfNode( *workingList, label, incrementTime, &_totalDuration );
}

void add_gradient( SGL_GRADIENT_T *grad, char *label, SGL_LOGICAL_AXIS_T logicalAxis,
					SGL_GRAD_PLACEMENT_ACTION_T action, char *refLabel,
					double actionTime, SGL_CHANGE_POLARITY_T polarity )
{
	double _eventStart, _totalTime;
	
	_eventStart = addToGradList( workingList, grad, label, logicalAxis,
						action, refLabel, actionTime, polarity, &_totalTime );
}

void add_marker( char *label, SGL_GRAD_PLACEMENT_ACTION_T action, char *refLabel, double actionTime )
{
	double _eventStart, _totalTime;
	
	_eventStart = addToGradList( workingList, (void *)&marker, label, MARKER,
						action, refLabel, actionTime, PRESERVE, &_totalTime );
} 


double write_gradient_list( char *readName, char *phaseName, char *sliceName,
							double *readAmp, double *phaseAmp, double *sliceAmp )
{
	return writeGradList( *workingList, readName, phaseName, sliceName, NULL, NULL, NULL,
							0.0, getDurationOfList(*workingList),
							readAmp, phaseAmp, sliceAmp, NULL, NULL, NULL );
}

double write_comp_gradients( char *readName, char *phaseName, char *sliceName,
							double *readAmp, double *phaseAmp, double *sliceAmp )
{
	return writeGradList( *workingList, readName, phaseName, sliceName, NULL, NULL, NULL,
							0.0, getDurationOfList(*workingList),
							readAmp, phaseAmp, sliceAmp, NULL, NULL, NULL );
}

double write_comp_grads( SGL_KERNEL_INFO_T *readInfo, SGL_KERNEL_INFO_T *phaseInfo, SGL_KERNEL_INFO_T *sliceInfo )
{
	double duration;

	duration = writeGradListKI( *workingList,
								readInfo, phaseInfo, sliceInfo,
								NULL, NULL, NULL,
								0.0, getDurationOfList(*workingList) );
	return  duration;
}

double write_comp_grads2( SGL_KERNEL_INFO_T *readInfo, SGL_KERNEL_INFO_T *readInfo2, 
							  SGL_KERNEL_INFO_T *phaseInfo, SGL_KERNEL_INFO_T *phaseInfo2,
							  SGL_KERNEL_INFO_T *sliceInfo, SGL_KERNEL_INFO_T *sliceInfo2 )
{
	double duration;

	duration = writeGradListKI( *workingList,
								readInfo, phaseInfo, sliceInfo,
								readInfo2, phaseInfo2, sliceInfo2,
								0.0, getDurationOfList(*workingList) );
	return  duration;
}

/*double write_gradient_list_snippet( char *readName, char *phaseName, char *sliceName,
							double startTime, double duration,
							double *readAmp, double *phaseAmp, double *sliceAmp )
{
	return writeGradList( *workingList, readName, phaseName, sliceName,
							startTime, duration, readAmp, phaseAmp, sliceAmp );
}
*/
double write_comp_gradients_snippet( char *readName, char *phaseName, char *sliceName,
							double startTime, double duration,
							double *readAmp, double *phaseAmp, double *sliceAmp )
{
	return writeGradList( *workingList, readName, phaseName, sliceName, NULL, NULL, NULL,
							startTime, duration, readAmp, phaseAmp, sliceAmp, NULL, NULL, NULL );
}

double write_comp_grads_snippet( SGL_KERNEL_INFO_T *readInfo, SGL_KERNEL_INFO_T *phaseInfo, SGL_KERNEL_INFO_T *sliceInfo,
							char *startLabel, char *endLabel )
{
	double startTime;
	double duration;
	double outDuration;
	
	startTime = get_time( startLabel );
	duration = get_timing( FROM_START_OF, startLabel, TO_END_OF, endLabel );

	outDuration = writeGradListKI( *workingList,
								readInfo, phaseInfo, sliceInfo,
								NULL, NULL, NULL,
								startTime, duration );

	return  outDuration;
}

double write_comp_grads2_snippet( SGL_KERNEL_INFO_T *readInfo, SGL_KERNEL_INFO_T *readInfo2, 
							  		  SGL_KERNEL_INFO_T *phaseInfo, SGL_KERNEL_INFO_T *phaseInfo2,
							  		  SGL_KERNEL_INFO_T *sliceInfo, SGL_KERNEL_INFO_T *sliceInfo2,
									  char *startLabel, char *endLabel )
{
	double startTime;
	double duration;
	double outDuration;
	
	startTime = get_time( startLabel );
	duration = get_timing( FROM_START_OF, startLabel, TO_END_OF, endLabel );

	outDuration = writeGradListKI( *workingList,
								readInfo, phaseInfo, sliceInfo,
								readInfo2, phaseInfo2, sliceInfo2,
								startTime, duration );

	return  outDuration;
}

/* keeping a summary of a gradient list */

struct SGL_KERNEL_SUMMARY_T *allocateKernelSummaryNode( void )
{
	struct SGL_KERNEL_SUMMARY_T *node;

	if( (node = (struct SGL_KERNEL_SUMMARY_T *)malloc(sizeof(struct SGL_KERNEL_SUMMARY_T))) == NULL ) {
        displayError(ERR_MALLOC, __FILE__, __FUNCTION__, __LINE__); 
		exit(0);
	}

	node->startTime = 0.0;
	node->amplitude = 0.0;
	node->endTime = 0.0;
	node->moment = 0.0;

	return node;
}

void freeKernelSummaryNode( struct SGL_KERNEL_SUMMARY_T **aNode )
{
	if( *aNode ) {
		free( *aNode );
		*aNode = NULL;	
	}
}

void initKernelSummary( struct SGL_KERNEL_SUMMARY_T **aKernelSummary )
{
	struct SGL_KERNEL_SUMMARY_T *current;
	struct SGL_KERNEL_SUMMARY_T *next;
	
	current = *aKernelSummary;

	while( current )
	{
		next = current->next;
		freeKernelSummaryNode( &current );
		current = next;
	}	
	
	*aKernelSummary = NULL;
}

struct SGL_KERNEL_SUMMARY_T *newKernelSummary( struct SGL_GRAD_NODE_T *aKernel )
{	
	struct SGL_KERNEL_SUMMARY_T *summary;
	struct SGL_KERNEL_SUMMARY_T *new;
	struct SGL_GRAD_NODE_T *current;

	summary = NULL;
	current = aKernel;
	
	while( current )
	{
		if( current->logicalAxis != MARKER ) {
			new = allocateKernelSummaryNode();
			new->next = NULL;
			new->startTime = getStartTimeOfNode( current );
			new->endTime = getEndTimeOfNode( current );
			new->amplitude = getAmplitude( current->grad ) * (current->invert == INVERT ? -1.0 : 1.0 );
			new->moment = getMomentOfNode( current );

			new->next = summary;
			summary = new;

/*			if( summary ) {
				summary->next = new;
				summary = summary->next;
			} else {
				summary = new;
			}
*/
		}
		current = current->next;
	}
	return summary;
}

double startTimeOfSummary( struct SGL_KERNEL_SUMMARY_T *aSummary )
{
	struct SGL_KERNEL_SUMMARY_T *current;
	double startTime;

	if( aSummary ) {
		startTime = aSummary->startTime;
		current = aSummary->next;
	} else {
		return 0.0;
	}

	while( current )
	{
		if( startTime > current->startTime ) {
			startTime = current->startTime;
		}
		current = current->next; 		
	}
	return startTime;
}

double endTimeOfSummary( struct SGL_KERNEL_SUMMARY_T *aSummary )
{
	struct SGL_KERNEL_SUMMARY_T *current;
	double endTime;

	if( aSummary ) {
		endTime = aSummary->endTime;
		current = aSummary->next;
	} else {
		return 0.0;
	}

	while( current )
	{
		if( endTime < current->endTime ) {
			endTime = current->endTime;
		}
		current = current->next; 		
	}
	return endTime;
}

int kernelSummariesDiffer( struct SGL_KERNEL_SUMMARY_T *aSummary, struct SGL_KERNEL_SUMMARY_T *aAnotherSummary )
{
	struct SGL_KERNEL_SUMMARY_T *current;
	struct SGL_KERNEL_SUMMARY_T *anotherCurrent;

	int aSummarySize = countListNodes( (struct SGL_LIST_T *)aSummary ); 
	int aAnotherSummarySize = countListNodes( (struct SGL_LIST_T *)aAnotherSummary ); 



	if(  aSummarySize != aAnotherSummarySize ) {
		return 1;
	}

//fprintf(stdout,"aSummary address = %d, size = %d\n",aSummary,aSummarySize);
//fprintf(stdout,"aAnotherSummary address = %d, size = %d\n",aAnotherSummary,aAnotherSummarySize);

	current = aSummary;
	anotherCurrent = aAnotherSummary;

	while( current )
	{
//fprintf(stdout,"startTime = %f (current), %f (anotherCurrent)\n",current->startTime,anotherCurrent->startTime);
//fprintf(stdout,"endTime = %f (current), %f (anotherCurrent)\n",current->endTime,anotherCurrent->endTime);
//fprintf(stdout,"amplitude = %f (current), %f (anotherCurrent)\n",current->amplitude,anotherCurrent->amplitude);
//fprintf(stdout,"moment = %f (current), %f (anotherCurrent)\n",current->moment,anotherCurrent->moment);

		if( FP_NEQ(current->startTime,anotherCurrent->startTime) ) return 1;
		if( FP_NEQ(current->endTime,anotherCurrent->endTime) ) return 1;
		if( FP_NEQ(current->amplitude,anotherCurrent->amplitude) ) return 1;
		if( FP_NEQ(current->moment,anotherCurrent->moment) ) return 1;
		current = current->next;
		anotherCurrent = anotherCurrent->next;
	}
	return 0;
}

int evaluateSummary( struct SGL_GRAD_NODE_T *aList, struct SGL_KERNEL_SUMMARY_T **aSummary )
{
	struct SGL_KERNEL_SUMMARY_T *new;

	new = newKernelSummary( aList );
	if( *aSummary ) {
		if( kernelSummariesDiffer( new, *aSummary ) ) {
			initKernelSummary( aSummary );
			*aSummary = new;
			return 1;
		} else {
			initKernelSummary( &new );
			return 0;
		}
	} else {
		*aSummary = new;
	}
	return 1;
}

