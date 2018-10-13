/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 */
package vnmr.cryomon;

import java.awt.Color;


/**
 * This class contains CryoMonitor definitions and opcodes.
 */
public interface CryoMonitorDefs {
	static final int NITROGEN           = 0;   		// N2 select code
	static final int HELIUM             = 1;   		// He select code
	
    // status codes (SA)
    static int SA				        = 0;   		// code offset for status byte a
   
    static final int PROBE1_ON			= 0x0080;   // He probe 1 on
    static final int PROBE2_ON			= 0x0040; 	// He probe 2 on
    static final int HE_FILL_ON			= 0x0020; 	// He fill mode on
    
	// error codes (SA)
    static final int PROBE1_HE_LOW     	= 0x0010; 	// probe 1 Helium low
    static final int PROBE2_HE_LOW     	= 0x0008; 	// probe 2 Helium low
    static final int PROBE1_READ_ERROR 	= 0x0004; 	// probe 1 read error
    static final int PROBE2_READ_ERROR 	= 0x0002; 	// probe 2 read error
    static final int N2_HW_ERROR		= 0x0001; 	// Nitrogen hardware error
    
    static int ERRORS					= 0x000f;
    
    static int SB				        = 256;   	// code offset for status byte b

    // status codes (SB)

    static final int PROGRAM_RESTART    = 0x8000; 	// program restart at log entry
    static final int N2_ON				= 0x4000; 	// Nitrogen probe enabled
    static final int N2_CAL_SET			= 0x2000; 	// Nitrogen calibration set
    static final int HE_CAL_ACTIVE		= 0x1000; 	// Helium calibration active
    static final int HE_CAL_SET			= 0x0800; 	// Helium calibration set
    static final int FACTORY_CAL_SET	= 0x0400; 	// Factory calibration set
    static final int UNDEFINED	        = 0x0200; 	// not assigned
    static final int HE_NEW_READ	    = 0x0100; 	// new Helium read at log entry
    
    // remote control commands
    
    static final String CMD_CLOCK       = "t\n";    // current time 
    static final String CMD_VERSION     = "V\n";    // software version 
    static final String CMD_PARAMS   	= "W\n";    // read parameters concise 
    static final String CMD_ERRORS      = "EH\n";   // return error status 
    static final String CMD_MEAS_HE     = "HN\n";   // measure Helium now 
    static final String CMD_READ_LOG    = "R\n";    // read log file 
    static final String CMD_CLEAR_LOG   = "Z\n";    // clear log file 
    static final String CMD_READ_CAL    = "Q\n";    // read calibration parameters
 
	static final int SEL_FILL           = 0;   		// FILL select code
	static final int SEL_WARN           = 1;   		// WARN select code
	static final int SEL_ALERT          = 2;   		// ALERT select code

    static final double N2_FILL_LEVEL  	= 50;  		// refill target level
    static final double N2_WARN_LEVEL  	= 20;  		// fill level warning level
    static final double N2_ALERT_LEVEL 	= 10;  		// fill level alert level
    static final double HE_FILL_LEVEL  	= 20;  		// refill target level
    static final double HE_WARN_LEVEL  	= 10;  		// fill level warning level
    static final double HE_ALERT_LEVEL 	= 5;  		// fill level alert level
    static final int MAX_HISTORY 	    = 3000;  	// max history file size (1000 days)
   
    static final Color ALERT_COLOR      = Color.RED;    // status color for level <= ALERT_LEVEL
    static final Color WARN_COLOR       = Color.YELLOW; // status color for level <= WARN_COLOR
    static final Color FILL_COLOR       = Color.GREEN;	// normal status or fill plot color

}
