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
*     Revision 1.2  2006/07/07 01:11:06  mikem
*     modification to compile with psg
*
*
* Contains SGL Addition by Michael L. Gyngell.
* 1) Extensions to SGL that allow the creation of complex multi gradient
*    channel waveforms from basic SGL waveforms.
* 2) Extensions to SGL for deriving gradient duty cycle demands
*
***************************************************************************/
#ifndef SGLDUTYCYCLE_H
#define SGLDUTYCYCLE_H

extern SGL_EULER_MATRIX_T mainRotationMatrix;
extern SGL_MEAN_SQUARE_CURRENTS_T msCurrents;
extern SGL_MEAN_SQUARE_CURRENTS_T rmsCurrents;

void   eulerMatrix( double theta, 
                    double psi, 
									  									double phi, 
											  							SGL_EULER_MATRIX_T *eulerM );
void   eulerRotate( double readPt, 
                    double phasePt, 
																				double slicePt,
                 			SGL_EULER_MATRIX_T *euler, 
																				double *xPt, 
																				double *yPt, 
																				double *zPt);
double meanSquareCurrent( double dataPt, 
                          double gmax, 
																										double imax, 
																										double duration );
void   initDutyCycle( SGL_MEAN_SQUARE_CURRENTS_T *ms, 
                      SGL_MEAN_SQUARE_CURRENTS_T *rms);
void   dutyCycleWithEulerMatrix( SGL_GRADIENT_T *readGrad, 
                             				SGL_GRADIENT_T *phaseGrad,
                             				SGL_GRADIENT_T *sliceGrad,
                             				double scaleR, 
																																	double scaleP, 
																																	double scaleS,
                             				SGL_EULER_MATRIX_T *rot,
                             				double gmax,
                          							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   dutyCycle( SGL_GRADIENT_T *readGrad, 
              				SGL_GRADIENT_T *phaseGrad,
               			SGL_GRADIENT_T *sliceGrad,
              				double scaleR, 
																		double scaleP, 
																		double scaleS,
              				double gmax,
           							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   dutyCycleWithEulerAngles( SGL_GRADIENT_T *readGrad, 
			                             	SGL_GRADIENT_T *phaseGrad,
                             				SGL_GRADIENT_T *sliceGrad,
                             				double scaleR, 
												  																			double scaleP,
														  																	double scaleS,
                             				double theta, 
																		  													double psi,
																				  											double phi,
                             				double gmax,
							                          SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   dutyCycleOfGradListWithEulerMatrix( struct SGL_GRAD_NODE_T* gradList,
                                    							double startTime, 
									  																																double endTime,
                                    							SGL_EULER_MATRIX_T *rot,
                                   	 						double gmax,
                                    							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   dutyCycleOfGradList( struct SGL_GRAD_NODE_T* gradList,
                   					  	 double gmax,
                   							  SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   dutyCycleOfGradListSnippet( struct SGL_GRAD_NODE_T* gradList,
                          		  					double startTime, 
																										  							double endTime,
                          				  			double gmax,
                          						  	SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   dutyCycleOfGradListWithEulerAngles( struct SGL_GRAD_NODE_T* gradList,
                                    							double theta, 
								  																																	double psi, 
										  																															double phi,
                                    							double gmax,
                                    							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
double calcDutyCycle( SGL_MEAN_SQUARE_CURRENTS_T *msCurrents, 
                      double time,
                  				SGL_MEAN_SQUARE_CURRENTS_T *rmsCurrents );
void   init_duty_cycle( );
void   duty_cycle_of_list( );
void   duty_cycle_of_list_snippet( double startTime, 
                                   double duration );
double calc_duty_cycle( double time );

/***************************************************************************/

void   t_eulerMatrix( double theta, 
                      double psi, 
									    									double phi, 
											    							SGL_EULER_MATRIX_T *eulerM );
void   t_eulerRotate( double readPt, 
                      double phasePt, 
									  											double slicePt,
                   			SGL_EULER_MATRIX_T *euler, 
													  							double *xPt, 
															  					double *yPt, 
																	  			double *zPt);
double t_meanSquareCurrent( double dataPt, 
                            double gmax, 
											  															double imax, 
													  													double duration );
void   t_initDutyCycle( SGL_MEAN_SQUARE_CURRENTS_T *ms, 
                        SGL_MEAN_SQUARE_CURRENTS_T *rms);
void   t_dutyCycleWithEulerMatrix( SGL_GRADIENT_T *readGrad, 
                               				SGL_GRADIENT_T *phaseGrad,
                               				SGL_GRADIENT_T *sliceGrad,
                               				double scaleR, 
																  																	double scaleP, 
																		   														double scaleS,
                               				SGL_EULER_MATRIX_T *rot,
                               				double gmax,
                           							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   t_dutyCycle( SGL_GRADIENT_T *readGrad, 
                				SGL_GRADIENT_T *phaseGrad,
                 			SGL_GRADIENT_T *sliceGrad,
                				double scaleR, 
															  			double scaleP, 
																	  	double scaleS,
                				double gmax,
             							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   t_dutyCycleWithEulerAngles( SGL_GRADIENT_T *readGrad, 
			                               	SGL_GRADIENT_T *phaseGrad,
                               				SGL_GRADIENT_T *sliceGrad,
                               				double scaleR, 
												  	  																		double scaleP,
														  	  																double scaleS,
                               				double theta, 
																		  		  											double psi,
																				  		  									double phi,
                               				double gmax,
							                            SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   t_dutyCycleOfGradListWithEulerMatrix( struct SGL_GRAD_NODE_T* gradList,
                                      							double startTime, 
									    																																double endTime,
                                      							SGL_EULER_MATRIX_T *rot,
                                     	 						double gmax,
                                      							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   t_dutyCycleOfGradList( struct SGL_GRAD_NODE_T* gradList,
                     					  	 double gmax,
                     							  SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   t_dutyCycleOfGradListSnippet( struct SGL_GRAD_NODE_T* gradList,
                            		  					double startTime, 
											  															  							double endTime,
                            				  			double gmax,
                            						  	SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   t_dutyCycleOfGradListWithEulerAngles( struct SGL_GRAD_NODE_T* gradList,
                                      							double theta, 
								  	  																																double psi, 
										  	  																														double phi,
                                      							double gmax,
                                      							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
double t_calcDutyCycle( SGL_MEAN_SQUARE_CURRENTS_T *msCurrents, 
                        double time,
                    				SGL_MEAN_SQUARE_CURRENTS_T *rmsCurrents );
void   t_init_duty_cycle( );
void   t_duty_cycle_of_list( );
void   t_duty_cycle_of_list_snippet( double startTime, 
                                     double duration );
double t_calc_duty_cycle( double time );

/***************************************************************************/

void   x_eulerMatrix( double theta, 
                      double psi, 
									    									double phi, 
											    							SGL_EULER_MATRIX_T *eulerM );
void   x_eulerRotate( double readPt, 
                      double phasePt, 
											  									double slicePt,
                   			SGL_EULER_MATRIX_T *euler, 
															  					double *xPt, 
																	  			double *yPt, 
																			  	double *zPt);
double x_meanSquareCurrent( double dataPt, 
                            double gmax, 
											  															double imax, 
													  													double duration );
void   x_initDutyCycle( SGL_MEAN_SQUARE_CURRENTS_T *ms, 
                        SGL_MEAN_SQUARE_CURRENTS_T *rms);
void   x_dutyCycleWithEulerMatrix( SGL_GRADIENT_T *readGrad, 
                               				SGL_GRADIENT_T *phaseGrad,
                               				SGL_GRADIENT_T *sliceGrad,
                               				double scaleR, 
															  																		double scaleP, 
																	  																double scaleS,
                               				SGL_EULER_MATRIX_T *rot,
                               				double gmax,
                            							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   x_dutyCycle( SGL_GRADIENT_T *readGrad, 
                				SGL_GRADIENT_T *phaseGrad,
                 			SGL_GRADIENT_T *sliceGrad,
                				double scaleR, 
															  			double scaleP, 
																	  	double scaleS,
              				  double gmax,
           							  SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   x_dutyCycleWithEulerAngles( SGL_GRADIENT_T *readGrad, 
			                               	SGL_GRADIENT_T *phaseGrad,
                               				SGL_GRADIENT_T *sliceGrad,
                               				double scaleR, 
												  	  																		double scaleP,
														  	  																double scaleS,
                               				double theta, 
																		  	  												double psi,
																				  	  										double phi,
                               				double gmax,
							                            SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   x_dutyCycleOfGradListWithEulerMatrix( struct SGL_GRAD_NODE_T* gradList,
                                      							double startTime, 
									    																																double endTime,
                                      							SGL_EULER_MATRIX_T *rot,
                                     	 						double gmax,
                                      							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   x_dutyCycleOfGradList( struct SGL_GRAD_NODE_T* gradList,
                     					  	 double gmax,
                     							  SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   x_dutyCycleOfGradListSnippet( struct SGL_GRAD_NODE_T* gradList,
                            		  					double startTime, 
											  															  							double endTime,
                            				  			double gmax,
                            						  	SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
void   x_dutyCycleOfGradListWithEulerAngles( struct SGL_GRAD_NODE_T* gradList,
                                      							double theta, 
								  	  																																double psi, 
										  	  																														double phi,
                                      							double gmax,
                                      							SGL_MEAN_SQUARE_CURRENTS_T *msCurrents );
double x_calcDutyCycle( SGL_MEAN_SQUARE_CURRENTS_T *msCurrents, 
                        double time,
                    				SGL_MEAN_SQUARE_CURRENTS_T *rmsCurrents );
void   x_init_duty_cycle( );
void   x_duty_cycle_of_list( );
void   x_duty_cycle_of_list_snippet( double startTime, 
                                     double duration );
double x_calc_duty_cycle( double time );


#endif
