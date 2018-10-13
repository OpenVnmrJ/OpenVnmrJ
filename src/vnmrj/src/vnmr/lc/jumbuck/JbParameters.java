/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc.jumbuck;

import java.util.*;

import vnmr.lc.LcMsg;


/**
 * This class holds the parameter cache for the 335 driver.
 */
public class JbParameters
    extends HashMap<String, JbParameters.Parameter> implements JbDefs {

    /** Keeps track of insertion order of Parameters. */
    private int m_insertionIndex = 0;

    /**
     * Test routine that initializes a table with standard parameters
     * and then prints out all their values.
     */
    static public void main(String[] args) {

        // List of parameter names in lower case.
        SortedSet<String> nameSort = new TreeSet<String>();

        // Tabel of names by index.
        SortedMap<Integer, String> indexSort = new TreeMap<Integer, String>();

        JbParameters table = new JbParameters();
        Iterator itr = table.values().iterator();

        System.out.println("\nTable in random order:");
        while (itr.hasNext()) {
            Parameter p = (Parameter)itr.next();
            System.out.println(p.getName() + " = " + p.getValue());
            nameSort.add(p.getName().toLowerCase());
            indexSort.put(p.getIndex(), p.getName());
        }

        System.out.println("\nTable in sorted order:");
        for (String key: nameSort) {
            Parameter p = table.getParameter(key);
            System.out.println(p.getName() + " = " + p.getValue());
        }

        System.out.println("\nTable in insertion order:");
        while (indexSort.size() > 0) {
            String name = indexSort.remove(indexSort.firstKey());
            System.out.println(name + " = " + table.getParameterValue(name));
        }
    }

    /**
     * Initialize the map with all the standard parameters.
     */
    public JbParameters() {
        Object[][] p = STD_PARAMETERS;
        int npars = p.length;
        for (int i = 0; i < npars; i++) {
            int size = 4;       // The usual size (ints or floats)
            Object ovalue = p[i][1];
            if (p[i].length > 2) {
                // Size specified explicitly
                size = ((Integer)p[i][2]).intValue();
            } else if (ovalue instanceof Byte) {
                size = 1;
            } else if (ovalue instanceof String) {
                // If String size not specified explicitly...
                size = ((String)ovalue).length();
            }
            String name = (String)p[i][0];
            put(new Parameter(name, ovalue, size));

            // Put in detector version of same parameter
            //name = "Detector " + name;
            //put(new Parameter(name, ovalue, size));
        }
    }

    /**
     * Put a parameter into the cache. Creates it if it doesn't exist.
     */
    private void put(Parameter p) {
        String key = p.getName().toLowerCase();
        p.setIndex(m_insertionIndex++);
        put(key, p);
    }

    /**
     * Set the value of a parameter. Fails if the parameter name is
     * unknown or if the new value is a different type than the old value.
     * @param name The parameter name -- case insensitive.
     * @param value The value of the parameter as an Object.
     * @return Returns true on success, false on failure.
     */
    public boolean set(String name, Object value) {
        Parameter p = (Parameter)getParameter(name);
        if (p != null) {
            return p.setValue(value);
        }
        return false;
    }

    public boolean set(String name, String value) {
        Object oldVal = getParameterValue(name);
        if (oldVal == null) {
            LcMsg.postError("JbParameters.set: invalid parameter name: \""
                               + name + "\"");
            return false;
        }
        Object newVal = null;
        try {
            if (oldVal instanceof Float) {
                newVal = new Float(value);
            } else if (oldVal instanceof Integer) {
                newVal = new Integer(value);
            } else if (oldVal instanceof String) {
                newVal = value;
            }
        } catch (NumberFormatException nfe) {
            String type = oldVal.getClass().toString();
            int n = 1 + type.lastIndexOf('.');
            if (n > 0) {
                type = type.substring(n);
            }
            LcMsg.postError("JbParameters.set: parameter value \""
                               + value + "\" is not compatible with type "
                               + type);
            return false;
        }
        return set(name, newVal);
    }

    public boolean setByte(String name, int value) {
        return set(name, new Byte((byte)value));
    }

    public boolean set(String name, int value) {
        return set(name, new Integer(value));
    }

    public boolean set(String name, float value) {
        return set(name, new Float(value));
    }

    /**
     * Get the value of a parameter as an object.
     * @param name The case-insensitive name.
     * @return An object representing the value of the parameter, or null.
     */
    public Parameter getParameter(String name) {
        return get(name.toLowerCase());
    }

    /**
     * Get the value of a parameter as an object.
     * @param name The case-insensitive name.
     * @return An object representing the value of the parameter, or null.
     */
    public Object getParameterValue(String name) {
        Parameter p = getParameter(name);
        if (p != null) {
            return p.getValue();
        } else {
            return null;
        }
    }

    public int getByte(String name) throws ClassCastException {
        int val = ((Byte)getParameterValue(name)).intValue() & 0xff;
        return val;
    }

    public int getInt(String name) throws ClassCastException {
        return ((Integer)getParameterValue(name)).intValue();
    }

    public float getFloat(String name) throws ClassCastException {
        return ((Float)getParameterValue(name)).floatValue();
    }

    public float[] getFloatArray(String name) throws ClassCastException {
        return (float[])getParameterValue(name);
    }

    public String getString(String name) throws ClassCastException {
        return (String)getParameterValue(name);
    }

    public String getPaddedString(String name) throws ClassCastException {
        Parameter par = getParameter(name);
        String value = (String)par.getValue();
        int origLen = value.length();
        int newLen = par.getSize();
        if (origLen > newLen) {
            // Truncate
            value = value.substring(0, newLen);
        } else if (origLen < newLen) {
            // Pad with nulls
            char[] cvals = new char[newLen];
            value.getChars(0, origLen, cvals, 0);
            for (int i = origLen; i < newLen; i++) {
                cvals[i] = '\0';
            }
            value = new String(cvals);
        }
        return value;
    }



    /**
     * A container class for instrument parameters that go into the
     * parameter cache.  Also has static utility routines to interface
     * with the parameter table.
     */
    class Parameter {

        /** The name of the parameter, with canonical mixed-case spelling. */
        private String mm_name;

        /** The value of the parameter. */
        private Object mm_value;

         /** The size of the parameter value in bytes. */
        private int mm_size;

       /** The index of the parameter in order of insertion. */
        private int mm_index;


        public Parameter(String name, Object value, int size) {
            mm_name = name;
            mm_value = value;
            mm_size = size;
        }

        public String getName() {
            return mm_name;
        }

        public Object getValue() {
            return mm_value;
        }

        public int getSize() {
            return mm_size;
        }

        public int getIndex() {
            return mm_index;
        }

        //public void setSize(int size) {
        //    mm_size = size;
        //}

        public void setIndex(int index) {
            mm_index = index;
        }

        /**
         * Set the value of this parameter to a given value.
         * The value must be the same class type as the parameter's
         * current value.
         */
        public boolean setValue(Object value) {
            if (mm_value.getClass() == value.getClass()) {
                mm_value = value;
                return true;
            } else {
                return false;
            }
        }
    }


    /** Known parameters and their initial values. */
    private Object[][] STD_PARAMETERS = {
        // Instrument Identity
        // The standard address:
        {"IP Address",                 JbControl.intIpAddress("172.16.0.60")},
        {"Family",                     FAMILY_LC},
        {"Model",                      MODEL_JB},
        {"Name",                       "Jumbuck",                       TWENTY},
        {"Version",                    INTERFACE_VERSION},
        {"Method Action",              METHOD_ACTION_STOP},
        // Method Header
        {"Method Version",             ZERO},
        {"Method Name",                "My new method",                 FORTY},
        {"Min Wavelength",             MIN_WAVELENGTH},
        {"Max Wavelength",             MAX_WAVELENGTH},
        {"Slit Width",                 SLIT_WIDTH_8},
        {"Analog 1 Source",            ANALOG_SOURCE_OFF},
        {"Analog 1 Peak Ticks",        ANALOG_PEAK_TICKS_DISABLED},
        {"Analog 2 Source",            ANALOG_SOURCE_OFF},
        {"Analog 2 Peak Ticks",        ANALOG_PEAK_TICKS_DISABLED},
        {"Peak Sense Relay Active",    PEAK_RELAY_ACTIVE_CLOSED},
        {"Peak Sense Duration",        PEAK_DURATION_DEFAULT},
        {"Peak Sense Delay",           PEAK_DELAY_DEFAULT},
        {"Analog Time Constant",       ANALOG_TIME_CONSTANT_2000},
        {"Bunching Size",              BUNCHING_SIZE_1},
        {"Noise Monitor Length",       NOISE_MONITOR_LENGTH_MIN},
        {"Diagnostic Method Enabled",  DIAGNOSTIC_METHOD_DISABLED},
        {"Processing Stage Bypass",    ZERO}, // Req for user mode
        {"Bandwidth Filter Size",      new Integer(-1)}, // Req for user mode
        // Method Lines
        {"Run Time",                   ZERO},
        {"Param Wavelength 1",         new Float(190)},
        {"Param Wavelength 2",         new Float(395)},
        {"Param Autozero 1",           ZERO},
        {"Param Autozero 2",           ZERO},
        {"Param Analog 1 Attn",        new Float(2)},
        {"Param Analog 2 Attn",        new Float(2)},
        {"Param Output Relay 1",       ZERO},
        {"Param Output Relay 2",       ZERO},
        {"Param Output Relay 3",       ZERO},
        {"Param Output Relay 4",       ZERO},
        {"Param Peaksense Relay Mode", ZERO},
        {"Param Peaksense Peak Width", TWO},
        {"Param Peaksense S N Ratio",  TWO},
        {"Param Time Slice Trigger",   TIME_SLICE_TRIGGER_OFF},
        {"Param Time Slice Period",    new Integer(1000)}, // ms
        {"Param Level Sense Threshold", new Float(0.1)}, // AU
        {"Param Method End",           END_RUN_HOLD},

        // These codes should be considered read only
        {"Code Wavelength 1",          new Byte((byte)0x10)},
        {"Code Wavelength 2",          new Byte((byte)0x11)},
        {"Code Autozero 1",            new Byte((byte)0x12)},
        {"Code Autozero 2",            new Byte((byte)0x13)},
        {"Code Analog 1 Attn",         new Byte((byte)0x14)},
        {"Code Analog 2 Attn",         new Byte((byte)0x15)},
        {"Code Output Relay 1",        new Byte((byte)0x16)},
        {"Code Output Relay 2",        new Byte((byte)0x17)},
        {"Code Output Relay 3",        new Byte((byte)0x18)},
        {"Code Output Relay 4",        new Byte((byte)0x19)},
        {"Code Peaksense Relay Mode",  new Byte((byte)0x1a)},
        {"Code Peaksense Peak Width",  new Byte((byte)0x1b)},
        {"Code Peaksense S N Ratio",   new Byte((byte)0x1c)},
        {"Code Time Slice Trigger",    new Byte((byte)0x1d)},
        {"Code Time Slice Period",     new Byte((byte)0x1e)},
        {"Code Level Sense Threshold", new Byte((byte)0x1f)},
        {"Code Method End",            new Byte((byte)0xf0)},
        {"Code",                       new Byte((byte)0)},

        // Get multiple spectra
        {"MaxNumberOfSpectra",         new Integer(100)},
        // Lamp
        {"Lamp",                       LAMP_BOTH},
        {"Lamp Action",                LAMP_OFF},
        // Flow Cell
        {"CellRatio",                  new Float(0)},
        {"CellType",                   CELL_TYPE_4x0},
        // Set IP
        {"SetFixedIP",                 JbControl.intIpAddress("172.16.0.60")},
        {"SetSubnetMask",              JbControl.intIpAddress("255.255.255.0")},
        {"GatewayAddress",             JbControl.intIpAddress("172.16.0.1")},
        {"BootPsname",                 BOOTP_SERVER_NAME_ON},
        // Get Strings
        {"ReadFlag",                   READ_FIRST},


        // Needed for responses
        {"Length",                     FOUR},
        {"ReturnCode",                 ZERO},
        // Status
        {"InstErrorField",             ZERO},
        {"D2LampState",                ZERO},
        {"VisLampState",               ZERO},
        {"Wavelength1",                F_ZERO},
        {"Wavelength2",                F_ZERO},
        {"Absorbance1",                F_ZERO},
        {"Absorbance2",                F_ZERO},
        {"DetectorState",              ZERO},
        {"RunTime",                    ZERO},
        {"InstOnTime",                 ZERO},
        {"EndTime",                    ZERO},
        {"InstActivityField",          ZERO},
        {"MethodIdNumber",             ZERO},
        {"ParamIdNumber",              ZERO},
        // Data
        {"CommsBufferLevelUsed",       ZERO},
        {"NumberOfSpectraFollowing",   ZERO},
        {"Status",                     ZERO},
        {"SigMod",                     ZERO},
        {"StartWL",                    ZERO},
        {"Count",                      ZERO},
        {"AbsValue",                   new float[0]},
        // Lamp
        {"OrigCommand",                ZERO},
        // Get Strings
        {"MoreFlag",                   LAST_DATA},
        {"ErrorCode",                  ZERO},
        {"StringDescription",          "Some String",          new Integer(52)},
        //Get Globals
        {"InstType",                   ZERO},
        {"InstConfig",                 ZERO},
        {"InstFwVersion",              ZERO},
        {"InstHwVersion",              ZERO},
        {"InstLogicVersion",           ZERO},
        {"SyncSignals",                ZERO},
        {"EnableReadyIn",              ZERO},
        {"LampActionOnError",          ZERO},
        {"Normalize9x0",               ZERO},
        {"FirmwareRevisionLevel",      "-",                    ONE},
        {"FirmwareIssueNumber",        new Byte((byte)0)},

        // Spare
        {"Spare Int Parameter",        ZERO},
        {"Spare 10 Bytes",             "Some String",          new Integer(10)},
    };
}
