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
*     Revision 1.1  2006/08/23 14:09:59  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/08/22 23:30:01  deans
*     *** empty log message ***
*
*     Revision 1.2  2006/07/07 01:11:41  mikem
*     modification to compile with psg
* 
* Contains additions to SGL by Michael L. Gyngell.
* 1) Functions to handle gradient lists
*
***************************************************************************/

#ifndef SGLGRADIENTLISTS_H
#define SGLGRADIENTLISTS_H

extern SGL_EVENT_DEBUG_T	sglEventDebug;
extern struct SGL_GRAD_NODE_T  *cg; 		/* a list of gradient pulses */
extern struct SGL_GRAD_NODE_T  *gl ; 		/* a list of gradient pulses */
extern struct SGL_GRAD_NODE_T  **workingList; 	/* a list of gradient pulses */
extern int gradEventsOverlap;

void   initGradList( struct SGL_GRAD_NODE_T **gradList );
double addToGradList( struct SGL_GRAD_NODE_T **gradList,
						SGL_GRADIENT_T *newGrad,
						char *label,
						SGL_LOGICAL_AXIS_T logicalAxis,
						SGL_GRAD_PLACEMENT_ACTION_T action,
						char *refLabel,
						double actionTime,
						SGL_CHANGE_POLARITY_T invert, 
						double *totalDuration );
void   displayNodeInfo( struct SGL_GRAD_NODE_T *gradNode );
double getDurationOfNode( struct SGL_GRAD_NODE_T *gradNode );
double getStartTimeOfNode( struct SGL_GRAD_NODE_T *gradNode );
double getEndTimeOfNode( struct SGL_GRAD_NODE_T *gradNode );
SGL_GRADIENT_TYPE_T getGradientTypeOfNode( struct SGL_GRAD_NODE_T *gradNode );
double getStartTimeOfRFPulseOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double getEndTimeOfRFPulseOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double getCenterOfRFPulseOfNode( struct SGL_GRAD_NODE_T *gradNode );
double getStartTimeOfAcqOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double getEndTimeOfAcqOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double getCenterOfEchoOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double getMomentOfNode( struct SGL_GRAD_NODE_T *aGradNode );


double getAmplitudeOfAxis( struct SGL_GRAD_NODE_T *gradList,
								                   SGL_LOGICAL_AXIS_T logicalAxis );
double getStartTimeOfList( struct SGL_GRAD_NODE_T *gradList );
double getEndTimeOfList( struct SGL_GRAD_NODE_T *gradList );
double getDurationOfList( struct SGL_GRAD_NODE_T *gradList );
int    getNumListElements( struct SGL_GRAD_NODE_T *gradList );
struct SGL_GRAD_NODE_T *getNodeByLabel( struct SGL_GRAD_NODE_T *cg, 
                                        char *label );
char   *getHeadNodeLabel( struct SGL_GRAD_NODE_T *gradList );
void   processNodeAction( struct SGL_GRAD_NODE_T *gradList,
					                    	struct SGL_GRAD_NODE_T *node );
void   processListActions( struct SGL_GRAD_NODE_T *gradList );
void   changeTimingOfNode( struct SGL_GRAD_NODE_T *head, 
							char *label,
							double incrementTime, 
							double *totalDuration );
void   setTimingOfNode( struct SGL_GRAD_NODE_T *head, 
							char *label,
							double newTime, 
							double *totalDuration );
void setPolarity( struct SGL_GRAD_NODE_T *head, char *label, SGL_CHANGE_POLARITY_T newPolarity );
SGL_CHANGE_POLARITY_T getPolarity( struct SGL_GRAD_NODE_T *head, char *label );
double getTiming( struct SGL_GRAD_NODE_T *gradList,
					SGL_TIMING_ACTION_T startAction, 
					char *startLabel,
					SGL_TIMING_ACTION_T endAction, 
					char *endLabel, 
					int *overlapFlag );
double writeGradList( struct SGL_GRAD_NODE_T* aGradList,
							char *aReadName, char *aPhaseName, char *aSliceName,
							char *aReadName2, char *aPhaseName2, char *aSliceName2,
							double aStartTime, double aDuration,
							double *aReadAmp, double *aPhaseAmp, double *aSliceAmp,
							double *aReadAmp2, double *aPhaseAmp2, double *aSliceAmp2 );
double writeGradListKI( struct SGL_GRAD_NODE_T* aGradList,
						SGL_KERNEL_INFO_T *aRead, SGL_KERNEL_INFO_T *aPhase, SGL_KERNEL_INFO_T *aSlice,
						SGL_KERNEL_INFO_T *aRead2, SGL_KERNEL_INFO_T *aPhase2, SGL_KERNEL_INFO_T *aSlice2,
						double aStartTime, double aDuration );
void writeCompoundGradient( SGL_GRADIENT_T *aGradient, double aStartTime, double aDuration, double *aAmplitude );
void printListLabels( struct SGL_GRAD_NODE_T *gradList );
void   activate_grad_list( struct SGL_GRAD_NODE_T **gradList );
void activate_kernel( struct SGL_GRAD_NODE_T **aKernel );
void   start_grad_list( struct SGL_GRAD_NODE_T **gradList );
void start_kernel( struct SGL_GRAD_NODE_T **aKernel );
double get_timing( SGL_TIMING_ACTION_T startAction, 
					char *startName,
					SGL_TIMING_ACTION_T endAction, 
					char *endName );
void set_comp_info( SGL_KERNEL_INFO_T *aInfo, char *aName );
char *lastKernelLabel( void );
double grad_list_duration();
double kernel_duration();
void   change_timing( char *label,
                        double incrementTime );
void set_timing( char *label, 
                        double newTime );
void   add_gradient( SGL_GRADIENT_T *grad, 
						char *label, 
						SGL_LOGICAL_AXIS_T logicalAxis,
						SGL_GRAD_PLACEMENT_ACTION_T action, 
						char *refLabel,
						double actionTime, 
						SGL_CHANGE_POLARITY_T polarity );
void add_marker( char *label,
				   SGL_GRAD_PLACEMENT_ACTION_T action, 
				   char *refLabel, 
				   double actionTime );

double write_gradient_list( char *readName, 
							char *phaseName, 
							char *sliceName,
							double *readAmp, 
							double *phaseAmp, 
							double *sliceAmp );
double write_comp_gradients( char *readName, 
							char *phaseName, 
							char *sliceName,
							double *readAmp, 
							double *phaseAmp, 
							double *sliceAmp );
double write_comp_grads( SGL_KERNEL_INFO_T *readInfo,
                             SGL_KERNEL_INFO_T *phaseInfo,
							 SGL_KERNEL_INFO_T *sliceInfo );
double write_comp_grads2( SGL_KERNEL_INFO_T *readInfo, SGL_KERNEL_INFO_T *readInfo2, 
							  SGL_KERNEL_INFO_T *phaseInfo, SGL_KERNEL_INFO_T *phaseInfo2,
							  SGL_KERNEL_INFO_T *sliceInfo, SGL_KERNEL_INFO_T *sliceInfo2 );
/*double write_gradient_list_snippet( char *readName, 
									char *phaseName, 
									char *sliceName,
									double startTime, 
									double endTime,
									double *readAmp, 
									double *phaseAmp, 
									double *sliceAmp );
*/
double write_comp_gradients_snippet( char *readName, 
										char *phaseName, 
										char *sliceName,
										double startTime, 
										double endTime,
										double *readAmp, 
										double *phaseAmp, 
										double *sliceAmp );
double write_comp_grads_snippet( SGL_KERNEL_INFO_T *readInfo,
                                       SGL_KERNEL_INFO_T *phaseInfo,
									   SGL_KERNEL_INFO_T *sliceInfo,
							           char *startLabel, char *endLabel );
double write_comp_grads2_snippet( SGL_KERNEL_INFO_T *readInfo, SGL_KERNEL_INFO_T *readInfo2, 
							  		  SGL_KERNEL_INFO_T *phaseInfo, SGL_KERNEL_INFO_T *phaseInfo2,
							  		  SGL_KERNEL_INFO_T *sliceInfo, SGL_KERNEL_INFO_T *sliceInfo2,
									  char *startLabel, char *endLabel );


struct SGL_KERNEL_SUMMARY_T *allocateKernelSummaryNode( void );
void freeKernelSummaryNode( struct SGL_KERNEL_SUMMARY_T **aNode );
void initKernelSummary( struct SGL_KERNEL_SUMMARY_T **aKernelSummary );
struct SGL_KERNEL_SUMMARY_T *newKernelSummary( struct SGL_GRAD_NODE_T *aKernel );
double startTimeOfSummary( struct SGL_KERNEL_SUMMARY_T *aSummary );
double endTimeOfSummary( struct SGL_KERNEL_SUMMARY_T *aSummary );
int kernelSummariesDiffer( struct SGL_KERNEL_SUMMARY_T *aSummary, struct SGL_KERNEL_SUMMARY_T *aAnotherSummary );
int evaluateSummary( struct SGL_GRAD_NODE_T *aList, struct SGL_KERNEL_SUMMARY_T **aSummary );


/**********/

void   x_initGradList( struct SGL_GRAD_NODE_T **gradList );
double x_addToGradList( struct SGL_GRAD_NODE_T **gradList,
						SGL_GRADIENT_T *newGrad,
						char *label,
						SGL_LOGICAL_AXIS_T logicalAxis,
						SGL_GRAD_PLACEMENT_ACTION_T action,
						char *refLabel,
						double actionTime,
						SGL_CHANGE_POLARITY_T invert, 
						double *totalDuration );
void   x_displayNodeInfo( struct SGL_GRAD_NODE_T *gradNode );
double x_getDurationOfNode( struct SGL_GRAD_NODE_T *gradNode );
double x_getStartTimeOfNode( struct SGL_GRAD_NODE_T *gradNode );
double x_getEndTimeOfNode( struct SGL_GRAD_NODE_T *gradNode );
SGL_GRADIENT_TYPE_T x_getGradientTypeOfNode( struct SGL_GRAD_NODE_T *gradNode );
double x_getStartTimeOfRFPulseOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double x_getEndTimeOfRFPulseOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double x_getCenterOfRFPulseOfNode( struct SGL_GRAD_NODE_T *gradNode );
double x_getStartTimeOfAcqOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double x_getEndTimeOfAcqOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double x_getCenterOfEchoOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double x_getMomentOfNode( struct SGL_GRAD_NODE_T *aGradNode );
double x_getAmplitudeOfAxis( struct SGL_GRAD_NODE_T *gradList,
		                     	SGL_LOGICAL_AXIS_T logicalAxis );
double x_getStartTimeOfList( struct SGL_GRAD_NODE_T *gradList );
double x_getEndTimeOfList( struct SGL_GRAD_NODE_T *gradList );
double x_getDurationOfList( struct SGL_GRAD_NODE_T *gradList );
int    x_getNumListElements( struct SGL_GRAD_NODE_T *gradList );
struct SGL_GRAD_NODE_T *x_getNodeByLabel( struct SGL_GRAD_NODE_T *cg, 
                                          char *label );
char   *x_getHeadNodeLabel( struct SGL_GRAD_NODE_T *gradList );
void   x_processNodeAction( struct SGL_GRAD_NODE_T *gradList,
		                      struct SGL_GRAD_NODE_T *node );
void x_processListActions( struct SGL_GRAD_NODE_T *gradList );
void x_changeTimingOfNode( struct SGL_GRAD_NODE_T *head, 
							char *label,
							double incrementTime, 
							double *totalDuration );
void x_setTimingOfNode( struct SGL_GRAD_NODE_T *head, 
							char *label,
							double newTime, 
							double *totalDuration );
void x_setPolarity( struct SGL_GRAD_NODE_T *head, char *label, SGL_CHANGE_POLARITY_T newPolarity );
SGL_CHANGE_POLARITY_T x_getPolarity( struct SGL_GRAD_NODE_T *head, char *label );
double x_getTiming( struct SGL_GRAD_NODE_T *gradList,
					SGL_TIMING_ACTION_T startAction, 
					char *startLabel,
					SGL_TIMING_ACTION_T endAction, 
					char *endLabel, 
					int *overlapFlag );
double x_writeGradList( struct SGL_GRAD_NODE_T* aGradList,
							char *aReadName, char *aPhaseName, char *aSliceName,
							char *aReadName2, char *aPhaseName2, char *aSliceName2,
							double aStartTime, double aDuration,
							double *aReadAmp, double *aPhaseAmp, double *aSliceAmp,
							double *aReadAmp2, double *aPhaseAmp2, double *aSliceAmp2);
double x_writeGradListKI( struct SGL_GRAD_NODE_T* aGradList,
						SGL_KERNEL_INFO_T *aRead, SGL_KERNEL_INFO_T *aPhase, SGL_KERNEL_INFO_T *aSlice,
						SGL_KERNEL_INFO_T *aRead2, SGL_KERNEL_INFO_T *aPhase2, SGL_KERNEL_INFO_T *aSlice2,
						double aStartTime, double aDuration );
void x_writeCompoundGradient( SGL_GRADIENT_T *aGradient, double aStartTime, double aDuration, double *aAmplitude );
void x_printListLabels( struct SGL_GRAD_NODE_T *gradList );
void   x_activate_grad_list( struct SGL_GRAD_NODE_T **gradList );
void x_activate_kernel( struct SGL_GRAD_NODE_T **aKernel );
void   x_start_grad_list( struct SGL_GRAD_NODE_T **gradList );
void x_start_kernel( struct SGL_GRAD_NODE_T **aKernel );
double x_get_timing( SGL_TIMING_ACTION_T startAction, 
						char *startName,
						SGL_TIMING_ACTION_T endAction, 
						char *endName );
void x_set_comp_info( SGL_KERNEL_INFO_T *aInfo, char *aName );
char *x_lastKernelLabel( void );
double x_grad_list_duration();
double x_kernel_duration();
void   x_change_timing( char *label, 
                        double incrementTime );
void   x_set_timing( char *label, 
                        double newTime );
void   x_add_gradient( SGL_GRADIENT_T *grad, 
						char *label, 
						SGL_LOGICAL_AXIS_T logicalAxis,
						SGL_GRAD_PLACEMENT_ACTION_T action, 
						char *refLabel,
						double actionTime, 
						SGL_CHANGE_POLARITY_T polarity );
void x_add_marker( char *label,
				   SGL_GRAD_PLACEMENT_ACTION_T action, 
				   char *refLabel, 
				   double actionTime );
double x_write_gradient_list( char *readName, 
								char *phaseName, 
								char *sliceName,
								double *readAmp, 
								double *phaseAmp, 
								double *sliceAmp );
double x_write_comp_gradients( char *readName, 
								char *phaseName, 
								char *sliceName,
								double *readAmp, 
								double *phaseAmp, 
								double *sliceAmp );
double x_write_comp_grads( SGL_KERNEL_INFO_T *readInfo,
                             SGL_KERNEL_INFO_T *phaseInfo,
							 SGL_KERNEL_INFO_T *sliceInfo );
double x_write_comp_grads2( SGL_KERNEL_INFO_T *readInfo, SGL_KERNEL_INFO_T *readInfo2, 
							  SGL_KERNEL_INFO_T *phaseInfo, SGL_KERNEL_INFO_T *phaseInfo2,
							  SGL_KERNEL_INFO_T *sliceInfo, SGL_KERNEL_INFO_T *sliceInfo2 );
/*double x_write_gradient_list_snippet( char *readName, 
										char *phaseName, 
										char *sliceName,
										double startTime, 
										double endTime,
										double *readAmp, 
										double *phaseAmp, 
										double *sliceAmp );
*/
double x_write_comp_gradients_snippet( char *readName, 
										char *phaseName, 
										char *sliceName,
										double startTime, 
										double endTime,
										double *readAmp, 
										double *phaseAmp, 
										double *sliceAmp );
double x_write_comp_grads_snippet( SGL_KERNEL_INFO_T *readInfo,
                                       SGL_KERNEL_INFO_T *phaseInfo,
									   SGL_KERNEL_INFO_T *sliceInfo,
							           char *startLabel, char *endLabel );
double x_write_comp_grads2_snippet( SGL_KERNEL_INFO_T *readInfo, SGL_KERNEL_INFO_T *readInfo2, 
							  		  SGL_KERNEL_INFO_T *phaseInfo, SGL_KERNEL_INFO_T *phaseInfo2,
							  		  SGL_KERNEL_INFO_T *sliceInfo, SGL_KERNEL_INFO_T *sliceInfo2,
									  char *startLabel, char *endLabel );


struct SGL_KERNEL_SUMMARY_T *x_allocateKernelSummaryNode( void );
void x_freeKernelSummaryNode( struct SGL_KERNEL_SUMMARY_T **aNode );
void x_initKernelSummary( struct SGL_KERNEL_SUMMARY_T **aKernelSummary );
struct SGL_KERNEL_SUMMARY_T *x_newKernelSummary( struct SGL_GRAD_NODE_T *aKernel );
double x_startTimeOfSummary( struct SGL_KERNEL_SUMMARY_T *aSummary );
double x_endTimeOfSummary( struct SGL_KERNEL_SUMMARY_T *aSummary );
int x_kernelSummariesDiffer( struct SGL_KERNEL_SUMMARY_T *aSummary, struct SGL_KERNEL_SUMMARY_T *aAnotherSummary );
int x_evaluateSummary( struct SGL_GRAD_NODE_T *aList, struct SGL_KERNEL_SUMMARY_T **aSummary );

/**********/

void   t_initGradList( struct SGL_GRAD_NODE_T **gradList );
double t_addToGradList( struct SGL_GRAD_NODE_T **gradList,
						SGL_GRADIENT_T *newGrad,
						char *label,
						SGL_LOGICAL_AXIS_T logicalAxis,
						SGL_GRAD_PLACEMENT_ACTION_T action,
						char *refLabel,
						double actionTime,
						SGL_CHANGE_POLARITY_T invert, 
						double *totalDuration );
void   t_displayNodeInfo( struct SGL_GRAD_NODE_T *gradNode );
double t_getDurationOfNode( struct SGL_GRAD_NODE_T *gradNode );
double t_getStartTimeOfNode( struct SGL_GRAD_NODE_T *gradNode );
double t_getEndTimeOfNode( struct SGL_GRAD_NODE_T *gradNode );
SGL_GRADIENT_TYPE_T t_getGradientTypeOfNode( struct SGL_GRAD_NODE_T *gradNode );
double t_getStartTimeOfRFPulseOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double t_getEndTimeOfRFPulseOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double t_getCenterOfRFPulseOfNode( struct SGL_GRAD_NODE_T *gradNode );
double t_getStartTimeOfAcqOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double t_getEndTimeOfAcqOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double t_getCenterOfEchoOfNode ( struct SGL_GRAD_NODE_T *gradNode );
double t_getMomentOfNode( struct SGL_GRAD_NODE_T *aGradNode );
double t_getAmplitudeOfAxis( struct SGL_GRAD_NODE_T *gradList,
							    SGL_LOGICAL_AXIS_T logicalAxis );
double t_getStartTimeOfList( struct SGL_GRAD_NODE_T *gradList );
double t_getEndTimeOfList( struct SGL_GRAD_NODE_T *gradList );
double t_getDurationOfList( struct SGL_GRAD_NODE_T *gradList );
int    t_getNumListElements( struct SGL_GRAD_NODE_T *gradList );
struct SGL_GRAD_NODE_T *t_getNodeByLabel( struct SGL_GRAD_NODE_T *cg, 
                            			  char *label );
char   *t_getHeadNodeLabel( struct SGL_GRAD_NODE_T *gradList );
void   t_processNodeAction( struct SGL_GRAD_NODE_T *gradList,
						      struct SGL_GRAD_NODE_T *node );
void t_processListActions( struct SGL_GRAD_NODE_T *gradList );
void t_changeTimingOfNode( struct SGL_GRAD_NODE_T *head, 
							char *label,
							double incrementTime, 
							double *totalDuration );
void t_setTimingOfNode( struct SGL_GRAD_NODE_T *head, 
							char *label,
							double setTime, 
							double *totalDuration );
void t_setPolarity( struct SGL_GRAD_NODE_T *head, char *label, SGL_CHANGE_POLARITY_T newPolarity );
SGL_CHANGE_POLARITY_T t_getPolarity( struct SGL_GRAD_NODE_T *head, char *label );
double t_getTiming( struct SGL_GRAD_NODE_T *gradList,
					SGL_TIMING_ACTION_T startAction, 
					char *startLabel,
					SGL_TIMING_ACTION_T endAction, 
					char *endLabel, 
					int *overlapFlag );
double t_writeGradList( struct SGL_GRAD_NODE_T* aGradList,
							char *aReadName, char *aPhaseName, char *aSliceName,
							char *aReadName2, char *aPhaseName2, char *aSliceName2,
							double aStartTime, double aDuration,
							double *aReadAmp, double *aPhaseAmp, double *aSliceAmp,
							double *aReadAmp2, double *aPhaseAmp2, double *aSliceAmp2 );
double t_writeGradListKI( struct SGL_GRAD_NODE_T* aGradList,
						SGL_KERNEL_INFO_T *aRead, SGL_KERNEL_INFO_T *aPhase, SGL_KERNEL_INFO_T *aSlice,
						SGL_KERNEL_INFO_T *aRead2, SGL_KERNEL_INFO_T *aPhase2, SGL_KERNEL_INFO_T *aSlice2,
						double aStartTime, double aDuration );
void t_writeCompoundGradient( SGL_GRADIENT_T *aGradient, double aStartTime, double aDuration, double *aAmplitude );
void t_printListLabels( struct SGL_GRAD_NODE_T *gradList );
void   t_activate_grad_list( struct SGL_GRAD_NODE_T **gradList );
void t_activate_kernel( struct SGL_GRAD_NODE_T **aKernel );
void   t_start_grad_list( struct SGL_GRAD_NODE_T **gradList );
void t_start_kernel( struct SGL_GRAD_NODE_T **aKernel );
double t_get_timing( SGL_TIMING_ACTION_T startAction, 
						char *startName,
						SGL_TIMING_ACTION_T endAction, 
						char *endName );
void t_set_comp_info( SGL_KERNEL_INFO_T *aInfo, char *aName );
char *lastKernelLabel( void );;
double t_grad_list_duration();
double t_kernel_duration();
void   t_change_timing( char *label, 
                        double incrementTime );
void   t_set_timing( char *label, 
                        double newTime );
void   t_add_gradient( SGL_GRADIENT_T *grad, 
						char *label, 
						SGL_LOGICAL_AXIS_T logicalAxis,
						SGL_GRAD_PLACEMENT_ACTION_T action, 
						char *refLabel,
						double actionTime, 
						SGL_CHANGE_POLARITY_T polarity );
void t_add_marker( char *label,
				   SGL_GRAD_PLACEMENT_ACTION_T action, 
				   char *refLabel, 
				   double actionTime );
double t_write_gradient_list( char *readName, 
								char *phaseName, 
								char *sliceName,
								double *readAmp, 
								double *phaseAmp, 
								double *sliceAmp );
double t_write_comp_grads( SGL_KERNEL_INFO_T *readInfo,
                             SGL_KERNEL_INFO_T *phaseInfo,
							 SGL_KERNEL_INFO_T *sliceInfo );
double t_write_comp_grads2( SGL_KERNEL_INFO_T *readInfo, SGL_KERNEL_INFO_T *readInfo2, 
							  SGL_KERNEL_INFO_T *phaseInfo, SGL_KERNEL_INFO_T *phaseInfo2,
							  SGL_KERNEL_INFO_T *sliceInfo, SGL_KERNEL_INFO_T *sliceInfo2 );
/*
double t_write_gradient_list_snippet( char *readName, 
										char *phaseName, 
										char *sliceName,
										double startTime, 
										double endTime,
										double *readAmp, 
										double *phaseAmp, 
										double *sliceAmp );
*/
double t_write_comp_gradients_snippet( char *readName, 
										char *phaseName, 
										char *sliceName,
										double startTime, 
										double endTime,
										double *readAmp, 
										double *phaseAmp, 
										double *sliceAmp );
double t_write_comp_grads_snippet( SGL_KERNEL_INFO_T *readInfo,
                                       SGL_KERNEL_INFO_T *phaseInfo,
									   SGL_KERNEL_INFO_T *sliceInfo,
							           char *startLabel, char *endLabel );
double t_write_comp_grads2_snippet( SGL_KERNEL_INFO_T *readInfo, SGL_KERNEL_INFO_T *readInfo2, 
							  		  SGL_KERNEL_INFO_T *phaseInfo, SGL_KERNEL_INFO_T *phaseInfo2,
							  		  SGL_KERNEL_INFO_T *sliceInfo, SGL_KERNEL_INFO_T *sliceInfo2,
									  char *startLabel, char *endLabel );

struct SGL_KERNEL_SUMMARY_T *t_allocateKernelSummaryNode( void );
void t_freeKernelSummaryNode( struct SGL_KERNEL_SUMMARY_T **aNode );
void t_initKernelSummary( struct SGL_KERNEL_SUMMARY_T **aKernelSummary );
struct SGL_KERNEL_SUMMARY_T *t_newKernelSummary( struct SGL_GRAD_NODE_T *aKernel );
double t_startTimeOfSummary( struct SGL_KERNEL_SUMMARY_T *aSummary );
double t_endTimeOfSummary( struct SGL_KERNEL_SUMMARY_T *aSummary );
int t_kernelSummariesDiffer( struct SGL_KERNEL_SUMMARY_T *aSummary, struct SGL_KERNEL_SUMMARY_T *aAnotherSummary );
int t_evaluateSummary( struct SGL_GRAD_NODE_T *aList, struct SGL_KERNEL_SUMMARY_T **aSummary );


#endif
