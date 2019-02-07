/*
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */
/* --------------------------
     * Author  : Bruno Orsier / Gilles Orazi
 * Created : 06/2002
 *           Codes of the events handled
 *           by the Galaxie integration algorithm
 * History :
 * -------------------------- */
// $History: integrator_events_code.h $
/*  */
/* *****************  Version 3  ***************** */
/* User: Go           Date: 4/11/02    Time: 11:54 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DT6566 */
/*  */
/* *****************  Version 2  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 1  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:56 */
/* Created in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */

#ifndef _INTEGRATOR_EVENTS_CODE
#define _INTEGRATOR_EVENTS_CODE

// Events modifying the parameters
#define   INTEGRATIONEVENT_SETPEAKWIDTH           0
#define   INTEGRATIONEVENT_DBLEPEAKWIDTH          1
#define   INTEGRATIONEVENT_HLVEPEAKWIDTH          2
#define   INTEGRATIONEVENT_SETTHRESHOLD           5
#define   INTEGRATIONEVENT_ADDTHRESHOLD           6
#define   INTEGRATIONEVENT_SETSKIMRATIO           7
#define   INTEGRATIONEVENT_SETTHRESHOLD_SOLVENT   8

#define   INTEGRATIONEVENT_INTEGRATION            10

#define   INTEGRATIONEVENT_PARAM_EVENT_MAX        100 // value must be greater than the values above

// Events needing one or more samples of the signal
#define   INTEGRATIONEVENT_ESTIMTHRESHOLD         101
#define   INTEGRATIONEVENT_INVERTNEGPEAKS         102
#define   INTEGRATIONEVENT_CLAMPNEGPEAKS          103
#define   INTEGRATIONEVENT_DETECTNEGPEAKS         104

#define   INTEGRATIONEVENT_ALGO_EVENT_MAX         200 // value must be greater than the values above

// Events modyfing the peaks
#define   INTEGRATIONEVENT_STARTPEAK_NOW          201
#define   INTEGRATIONEVENT_ENDPEAK_NOW            202
#define   INTEGRATIONEVENT_SPLITPEAK_NOW          203
#define   INTEGRATIONEVENT_SLICE_INTEGRATION      204

#define   INTEGRATIONEVENT_PEAK_EVENT_MAX         300 // value must be greater than the values above
// Events on the baseline
#define   INTEGRATIONEVENT_HORIZLINE              301
#define   INTEGRATIONEVENT_HORIZLINE_BACK         302
#define   INTEGRATIONEVENT_COMMONLINE             303
#define   INTEGRATIONEVENT_COMMONLINE_BYPEAK      304
//#define   INTEGRATIONEVENT_ADDPEAKS               305  // will force INTEGRATIONEVENT_COMMONLINE
#define   INTEGRATIONEVENT_HORIZLINE_BYPEAK       306
#define   INTEGRATIONEVENT_HORIZLINE_BACK_BYPEAK  307

#define   INTEGRATIONEVENT_TANGENTSKIM            320 // beware: skimming constants are used by SortAssocsByPriority
#define   INTEGRATIONEVENT_TANGENTSKIM_REAR       321
#define   INTEGRATIONEVENT_TANGENTSKIM_FRONT      322
#define   INTEGRATIONEVENT_TANGENTSKIM_EXP        323
#define   INTEGRATIONEVENT_TANGENTSKIM_REAR_EXP   324
#define   INTEGRATIONEVENT_TANGENTSKIM_FRONT_EXP  325

#define   INTEGRATIONEVENT_VALTOVAL               350
#define   INTEGRATIONEVENT_BASELINE_NOW           351
#define   INTEGRATIONEVENT_BASELINE_NEXTVALLEY    352

#define   INTEGRATIONEVENT_BASELINE_EVENT_MAX     400 // value must be greater than the values above
#define   INTEGRATIONEVENT_MIN_AREA               401
#define   INTEGRATIONEVENT_MIN_HEIGHT             402
#define   INTEGRATIONEVENT_MIN_AREA_PERCENT       403
#define   INTEGRATIONEVENT_MIN_HEIGHT_PERCENT     404

#define   INTEGRATIONEVENT_REJECTION_MAX          500

// special cases
#define   INTEGRATIONEVENT_ADDPEAKS               501
#define   INTEGRATIONEVENT_COMPUTENOISE           1000
#define   INTEGRATIONEVENT_NOTIMPLEMENTED         123456789

#endif
