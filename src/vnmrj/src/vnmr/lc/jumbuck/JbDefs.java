/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc.jumbuck;

/**
 *
 */
public interface JbDefs {

    /** The port that the 335 uses for communication. */
    final static int DEFAULT_PORT_NUMBER = 2346;


    // Handy, boxed up Integers
    public static final Integer ZERO = Integer.valueOf(0);
    public static final Integer ONE = Integer.valueOf(1);
    public static final Integer TWO = Integer.valueOf(2);
    public static final Integer THREE = Integer.valueOf(3);
    public static final Integer FOUR = Integer.valueOf(4);
    public static final Integer EIGHT = Integer.valueOf(8);
    public static final Integer TWENTY = Integer.valueOf(20);
    public static final Integer FORTY = Integer.valueOf(40);
    public static final Integer MINUS_ONE = Integer.valueOf(-1);

    public static final Float F_ZERO = Float.valueOf(0);


    // Codes for 335 commands

    //Message Interface IDs for SendRequest command
    //-SPECIAL
    public static final Integer ID_CMD_INST_REBOOT                  = Integer.valueOf(0x0001);
    public static final Integer ID_CMD_GET_INST_IDENTITY            = Integer.valueOf(0x0002);

    //-General CMDs for SendRequest command
    public static final Integer ID_CMD_METHOD_ACTION                = Integer.valueOf(0x5001);
    //public static final Integer ID_CMD_METHOD_HEADER                = new Integer(0x5002);
    //public static final Integer ID_CMD_METHOD_LINE_ENTRY            = new Integer(0x5003);
    public static final Integer ID_CMD_METHOD_CHANGE_END_TIME       = Integer.valueOf(0x5004);
    public static final Integer ID_CMD_METHOD_CLEAR                 = Integer.valueOf(0x5005);
    public static final Integer ID_CMD_SET_CELL_PARAMS              = Integer.valueOf(0x5006);
    public static final Integer ID_CMD_GET_CELL_PARAMS              = Integer.valueOf(0x5007);
    public static final Integer ID_CMD_SET_GLOBALS                  = Integer.valueOf(0x5008);
    public static final Integer ID_CMD_AUTO_ZERO                    = Integer.valueOf(0x5009);
    public static final Integer ID_CMD_LAMP                         = Integer.valueOf(0x500a);
    public static final Integer ID_CMD_SET_CONFIG_OPTIONS           = Integer.valueOf(0x500b);
    public static final Integer ID_CMD_SET_PROTO_PARAMS             = Integer.valueOf(0x500c);
    public static final Integer ID_CMD_GET_PROTO_PARAMS             = Integer.valueOf(0x500d);
    public static final Integer ID_CMD_SET_IP_PARAMS                = Integer.valueOf(0x500e);
    public static final Integer ID_CMD_GET_IP_PARAMS                = Integer.valueOf(0x500f);
    public static final Integer ID_CMD_RETURN_STATUS                = Integer.valueOf(0x5010);
    public static final Integer ID_CMD_RETURN_DATA                  = Integer.valueOf(0x5011);
    public static final Integer ID_CMD_GET_AZ_VALUES                = Integer.valueOf(0x5013);
    public static final Integer ID_CMD_RETURN_RUN_LOG               = Integer.valueOf(0x5014);
    public static final Integer ID_CMD_RETURN_ERROR_LOG             = Integer.valueOf(0x5015);
    public static final Integer ID_CMD_CLEAR_LOG                    = Integer.valueOf(0x5016);
    public static final Integer ID_CMD_GET_ERROR_CODE_STRINGS       = Integer.valueOf(0x5017);
    public static final Integer ID_CMD_GET_RUN_LOG_STRINGS          = Integer.valueOf(0x5018);
    public static final Integer ID_CMD_GET_GLOBALS                  = Integer.valueOf(0x501a);
    public static final Integer ID_CMD_RETURN_METHOD_HEADER         = Integer.valueOf(0x501b);
    public static final Integer ID_CMD_RETURN_ALL_METHOD_LINES      = Integer.valueOf(0x501d);
    public static final Integer ID_CMD_NON_METHOD_ACTION            = Integer.valueOf(0x501e);
    public static final Integer ID_CMD_EXTERNAL_MONITOR_CNTRL       = Integer.valueOf(0x5020);
    public static final Integer ID_CMD_GET_EXT_MONITOR_CNTRL        = Integer.valueOf(0x5021);
    public static final Integer ID_CMD_EXTERNAL_MONITOR_END         = Integer.valueOf(0x5022);
    public static final Integer ID_CMD_DIAGNOSTIC                   = Integer.valueOf(0x5025);
    public static final Integer ID_CMD_SET_DATE_TIME                = Integer.valueOf(0x502a);
    public static final Integer ID_CMD_GET_DATE_TIME                = Integer.valueOf(0x502b);
    public static final Integer ID_CMD_SET_LAMP_ALARM_TABLE         = Integer.valueOf(0x502c);
    public static final Integer ID_CMD_GET_LAMP_ALARM_TABLE         = Integer.valueOf(0x502d);
    public static final Integer ID_CMD_WRITE_STORAGE                = Integer.valueOf(0x5030);
    public static final Integer ID_CMD_READ_STORAGE                 = Integer.valueOf(0x5031);

    public static final Integer ID_CMD_XTALK_0T_TAKE_CAL_REF        = Integer.valueOf(0x5034);
    public static final Integer ID_CMD_XTALK_0T_CALIB               = Integer.valueOf(0x5035);
    public static final Integer ID_CMD_FLASH_PARAMETER              = Integer.valueOf(0xF030);


    // 335 detector only
    public static final Integer ID_CMD_METHOD_HEADER_335            = Integer.valueOf(0x5040);
    public static final Integer ID_CMD_METHOD_LINE_ENTRY_335        = Integer.valueOf(0x5041);
    public static final Integer ID_CMD_GET_CAL_DATA_335             = Integer.valueOf(0x5045);
    // maps same as 325 detector
    public static final Integer ID_CMD_RETURN_SPECTRUM_335          = Integer.valueOf(0x5011);
    public static final Integer ID_CMD_RETURN_MULTIPLE_SPECTRA      = Integer.valueOf(0x5042);

    // Special for this driver.  Map to ID_CMD_METHOD_LINE_ENTRY_335
    // before sending:
    public static final Integer ID_CMD_METHOD_END_335               = Integer.valueOf(0x5077);

    // Special Response
    public static final Integer ID_RSP_GET_INST_IDENTITY            = Integer.valueOf(0x0082);

    //-General Responses
    public static final Integer ID_RSP_METHOD_ACTION                = Integer.valueOf(0x5080);
    //public static final Integer ID_RSP_METHOD_HEADER                = new Integer(0x5081);
    //public static final Integer ID_RSP_METHOD_LINE_ENTRY            = new Integer(0x5082);
    public static final Integer ID_RSP_METHOD_CHANGE_END_TIME       = Integer.valueOf(0x5084);
    public static final Integer ID_RSP_METHOD_CLEAR                 = Integer.valueOf(0x5085);
    public static final Integer ID_RSP_SET_CELL_PARAMS              = Integer.valueOf(0x5086);
    public static final Integer ID_RSP_GET_CELL_PARAMS              = Integer.valueOf(0x5087);
    public static final Integer ID_RSP_SET_GLOBALS                  = Integer.valueOf(0x5088);
    public static final Integer ID_RSP_AUTO_ZERO                    = Integer.valueOf(0x5089);
    public static final Integer ID_RSP_LAMP                         = Integer.valueOf(0x508a);
    public static final Integer ID_RSP_SET_CONFIG_OPTIONS           = Integer.valueOf(0x508b);
    public static final Integer ID_RSP_SET_PROTO_PARAMS             = Integer.valueOf(0x508c);
    public static final Integer ID_RSP_GET_PROTO_PARAMS             = Integer.valueOf(0x508d);
    public static final Integer ID_RSP_SET_IP_PARAMS                = Integer.valueOf(0x508e);
    public static final Integer ID_RSP_GET_IP_PARAMS                = Integer.valueOf(0x508f);
    public static final Integer ID_RSP_RETURN_STATUS                = Integer.valueOf(0x5090);
    //public static final Integer ID_RSP_RETURN_DATA                  = new Integer(0x50);
    //public static final Integer ID_RSP_GET_AZ_VALUES                = new Integer(0x50);
    public static final Integer ID_RSP_RETURN_RUN_LOG               = Integer.valueOf(0x5094);
    public static final Integer ID_RSP_RETURN_ERROR_LOG             = Integer.valueOf(0x5095);
    public static final Integer ID_RSP_CLEAR_LOG                    = Integer.valueOf(0x5096);
    public static final Integer ID_RSP_GET_ERROR_CODE_STRINGS       = Integer.valueOf(0x5097);
    public static final Integer ID_RSP_GET_RUN_LOG_STRINGS          = Integer.valueOf(0x5098);
     public static final Integer ID_RSP_GET_CALIBRATION_DATA        = Integer.valueOf(0x5099);
    public static final Integer ID_RSP_GET_GLOBALS                  = Integer.valueOf(0x509a);
    public static final Integer ID_RSP_RETURN_METHOD_HEADER         = Integer.valueOf(0x50c1);
    public static final Integer ID_RSP_RETURN_ALL_METHOD_LINES      = Integer.valueOf(0x50c6);
     public static final Integer ID_RSP_RTN_METHOD_LINES_BY_CHUNK   = Integer.valueOf(0x50c7);
    public static final Integer ID_RSP_EXTERNAL_MONITOR_CNTRL       = Integer.valueOf(0x50a0);
    public static final Integer ID_RSP_GET_EXT_MONITOR_CNTRL        = Integer.valueOf(0x50a1);
    public static final Integer ID_RSP_EXTERNAL_MONITOR_END         = Integer.valueOf(0x50a2);
     public static final Integer ID_RSP_SET_SCALING_FACTOR          = Integer.valueOf(0x50a3);
     public static final Integer ID_RSP_GET_SCALING_FACTOR          = Integer.valueOf(0x50a2);// ???
    //public static final Integer ID_RSP_NON_METHOD_ACTION            = new Integer(0x501e);
    public static final Integer ID_RSP_DIAGNOSTIC                   = Integer.valueOf(0x50a5);
    public static final Integer ID_RSP_SET_DATE_TIME                = Integer.valueOf(0x50aa);
    public static final Integer ID_RSP_GET_DATE_TIME                = Integer.valueOf(0x50ab);
    public static final Integer ID_RSP_SET_LAMP_ALARM_TABLE         = Integer.valueOf(0x50ac);
    public static final Integer ID_RSP_GET_LAMP_ALARM_TABLE         = Integer.valueOf(0x50ad);
    public static final Integer ID_RSP_WRITE_STORAGE                = Integer.valueOf(0x50b0);
    public static final Integer ID_RSP_READ_STORAGE                 = Integer.valueOf(0x50b1);
    //public static final Integer ID_RSP_XTALK_0T_TAKE_CAL_REF        = new Integer(0x5034);
    public static final Integer ID_RSP_XTALK_0T_CALIB               = Integer.valueOf(0x50b5);
    public static final Integer ID_RSP_UNKNOWN_COMMAND              = Integer.valueOf(0x50ff);
    // 0xf000 - 0xf0ff Reserved for flash upload

    // 335 detector only
    public static final Integer ID_RSP_METHOD_HEADER_335            = Integer.valueOf(0x5081);
    public static final Integer ID_RSP_METHOD_LINE_ENTRY_335        = Integer.valueOf(0x5082);
    public static final Integer ID_RSP_GET_CAL_DATA_335             = Integer.valueOf(0x50c5);
    // maps same as 325 detector
    public static final Integer ID_RSP_RETURN_SPECTRUM              = Integer.valueOf(0x50c0);
    public static final Integer ID_RSP_RETURN_MULTIPLE_SPECTRA      = Integer.valueOf(0x50c2);

    //EEPROM VIDS for GetNVData, SetNVData commands
    public static final Integer VID_D2_LAMP_ON_TIME                 = Integer.valueOf(0);  // R/W
    public static final Integer VID_VIS_LAMP_ON_TIME                = Integer.valueOf(1);  // R/W
    public static final Integer VID_CELL_TYPE                       = Integer.valueOf(2);  // R
    public static final Integer VID_CELL_RATIO                      = Integer.valueOf(3);  // R
    public static final Integer VID_INSTRUMENT_NAME                 = Integer.valueOf(4);  // R/W
    public static final Integer VID_SERIAL_NUM                      = Integer.valueOf(41); // R/W
    public static final Integer VID_MONO_CAL_SLOPE                  = Integer.valueOf(42); // R
    public static final Integer VID_MONO_CAL_OFFSET                 = Integer.valueOf(43); // R
    public static final Integer VID_LAMP_DATE_D2                    = Integer.valueOf(44); // R/W
    public static final Integer VID_LAMP_DATE_VIS                   = Integer.valueOf(45); // R/W
    public static final Integer VID_INSTALLATION_DATE               = Integer.valueOf(46); // R/W
    public static final Integer VID_MAINTENANCE_DATE                = Integer.valueOf(47); // R/W
    public static final Integer VID_FIXED_IP                        = Integer.valueOf(48); // R
    public static final Integer VID_MAC_ADDRESS                     = Integer.valueOf(49); // R/W
    public static final Integer VID_SUBNET_ADDRESS                  = Integer.valueOf(50); // R
    public static final Integer VID_GATEWAY_ADDRESS                 = Integer.valueOf(51); // R
    public static final Integer VID_INSTRUMENT_CONFIG               = Integer.valueOf(52); // R

    // Codes for 335 replies
    public static final Integer RSP_GET_IP_PARAMS = Integer.valueOf(0x508f);
    public static final Integer RTN_MULTIPLE_SPECTRA = Integer.valueOf(0x50c2);


    //
    // Defined parameter values
    //
    public static final Integer METHOD_ACTION_STOP = ZERO;
    public static final Integer METHOD_ACTION_START = ONE;
    public static final Integer METHOD_ACTION_RESUME = TWO;
    public static final Integer METHOD_ACTION_RESET = THREE;

    public static final Integer MIN_WAVELENGTH = Integer.valueOf(190);
    public static final Integer MAX_WAVELENGTH = Integer.valueOf(950);

    public static final Integer SLIT_WIDTH_1 = ZERO;
    public static final Integer SLIT_WIDTH_2 = ONE;
    public static final Integer SLIT_WIDTH_4 = TWO;
    public static final Integer SLIT_WIDTH_8 = THREE;
    public static final Integer SLIT_WIDTH_16 = FOUR;
    public static final Integer SLIT_WIDTH_BIG = Integer.valueOf(5);
    //public static final Integer SLIT_WIDTH_DARK = new Integer(6); // debug only
    //public static final Integer SLIT_WIDTH_0T = new Integer(7); // debug only

    public static final Integer ANALOG_SOURCE_OFF = ZERO; // default
    public static final Integer ANALOG_SOURCE_WAVE1 = ONE;
    public static final Integer ANALOG_SOURCE_WAVE2 = TWO;
    public static final Integer ANALOG_SOURCE_RATIO = THREE;

    public static final Integer ANALOG_PEAK_TICKS_DISABLED = ZERO; // default
    public static final Integer ANALOG_PEAK_TICKS_ENABLED = ONE;

    public static final Integer PEAK_RELAY_ACTIVE_OPEN = ZERO;
    public static final Integer PEAK_RELAY_ACTIVE_CLOSED = ONE; // default

    public static final Integer PEAK_DURATION_MIN = Integer.valueOf(100); // (ms)
    public static final Integer PEAK_DURATION_MAX = Integer.valueOf(99900);
    public static final Integer PEAK_DURATION_DEFAULT = Integer.valueOf(500); // default

    public static final Integer PEAK_DELAY_MIN = ZERO; // (s)
    public static final Integer PEAK_DELAY_MAX = Integer.valueOf(99);
    public static final Integer PEAK_DELAY_DEFAULT = ZERO; // default

    public static final Integer ANALOG_TIME_CONSTANT_50 = Integer.valueOf(50); // (ms)
    public static final Integer ANALOG_TIME_CONSTANT_500 = Integer.valueOf(500);
    public static final Integer ANALOG_TIME_CONSTANT_1000 = Integer.valueOf(1000);
    public static final Integer ANALOG_TIME_CONSTANT_2000 = Integer.valueOf(2000); // default

    public static final Integer BUNCHING_SIZE_1 = ONE; // default
    public static final Integer BUNCHING_SIZE_2 = TWO;
    public static final Integer BUNCHING_SIZE_4 = FOUR;
    public static final Integer BUNCHING_SIZE_8 = EIGHT;
    public static final Integer BUNCHING_SIZE_16 = Integer.valueOf(16);
    public static final Integer BUNCHING_SIZE_32 = Integer.valueOf(32);
    public static final Integer BUNCHING_SIZE_64 = Integer.valueOf(64);
    public static final Integer BUNCHING_SIZE_128 = Integer.valueOf(128);
    public static final Integer BUNCHING_SIZE_256 = Integer.valueOf(256);

    public static final Integer NOISE_MONITOR_LENGTH_MIN = Integer.valueOf(16); // default
    public static final Integer NOISE_MONITOR_LENGTH_MAX = Integer.valueOf(128);

    public static final Integer DIAGNOSTIC_METHOD_DISABLED = ZERO; // default
    public static final Integer DIAGNOSTIC_METHOD_ENABLED = ONE; // debug only

    public static final Integer TIME_SLICE_TRIGGER_TIME_ON = ZERO;
    public static final Integer TIME_SLICE_TRIGGER_TIME_OFF = ONE;
    public static final Integer TIME_SLICE_TRIGGER_PEAK_ON = TWO;
    public static final Integer TIME_SLICE_TRIGGER_PEAK_OFF = THREE;
    public static final Integer TIME_SLICE_TRIGGER_LEVEL_ON = FOUR;
    public static final Integer TIME_SLICE_TRIGGER_LEVEL_OFF = Integer.valueOf(5);
    public static final Integer TIME_SLICE_TRIGGER_OFF = Integer.valueOf(6);

    public static final Integer MAX_RUN_TIME = Integer.valueOf(144000); // 24 hours

    public static final Integer END_RUN_RESET = ZERO;
    public static final Integer END_RUN_HOLD = ONE;

    public static final Integer LAMP_VIS = ZERO;
    public static final Integer LAMP_D2 = ONE;
    public static final Integer LAMP_BOTH = TWO;

    public static final Integer LAMP_OFF = ZERO;
    public static final Integer LAMP_ON = ONE;

    public static final Integer BOOTP_SERVER_NAME_OFF = ZERO;
    public static final Integer BOOTP_SERVER_NAME_ON = ONE;

    public static final Integer CELL_TYPE_0x0 = ZERO;
    public static final Integer CELL_TYPE_9x0 = ONE;
    public static final Integer CELL_TYPE_9x1 = TWO;
    public static final Integer CELL_TYPE_4x0 = THREE;
    public static final Integer CELL_TYPE_4x015 = FOUR;

    // Instrument ID
    public static final Integer INTERFACE_VERSION = Integer.valueOf(145);
    public static final Integer FAMILY_LC = FOUR;
    public static final Integer MODEL_JB = Integer.valueOf(335);

    // Read Flags
    public static final Integer READ_NEXT = ZERO;
    public static final Integer READ_FIRST = ONE;
    public static final Integer LAST_DATA = ZERO;
    public static final Integer MORE_DATA = ONE;




    public static final Integer SPARE_INT_PARAMETER = ZERO;


    public static final int STATE_POWER_ON = 0;
    public static final int STATE_LAMP_OFF = 1;
    public static final int STATE_LAMP_ON = 2;
    public static final int STATE_INITIALIZING = 3;
    public static final int STATE_MONITOR = 4;
    public static final int STATE_READY = 5;
    public static final int STATE_RUNNING = 6;
    public static final int STATE_STOPPED = 7;
    public static final int STATE_DIAGNOSTIC = 8;
    public static final int STATE_CALIBRATE = 9;
    public static final int STATE_HG_CALIBRATE = 10;

    public static final int LAMP_STATE_OFF = 0;
    public static final int LAMP_STATE_ON = 1;
    public static final int LAMP_STATE_WARMING = 2;
    public static final int LAMP_STATE_NOT_PRESENT = 3;
}
