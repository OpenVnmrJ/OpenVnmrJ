/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.util.ArrayList;

import vnmr.util.Messages;
import vnmr.util.StrUtil;


/**
 * Holds a parameter that is associated with an LC Run Method.
 * This parameter is NOT connected to any Vnmr parameter.
 */
public class LcMethodParameter {

    /** String indicating an Integer parameter type. */
    public static final String INT_TYPE = "Integer";

    /** String indicating a Double parameter type. */
    public static final String DOUBLE_TYPE = "Double";

    /** String indicating a String parameter type. */
    public static final String STRING_TYPE = "String";

    /** String indicating a Boolean parameter type. */
    public static final String BOOLEAN_TYPE = "Boolean";

    /** String indicating a MsScan parameter type. */
    public static final String MS_SCAN_TYPE = "MsScan";


    /** The (case sensitive) name of this parameter. */
    private String m_name;

    /** A (localized) label for this parameter. */
    private String m_label;

    /** The type of this parameter (text, int, float, boolean). */
    private String m_type;

    /** Whether this parameter can be arrayed. */
    private boolean m_isArrayable;

    /** Whether this parameter can be put in the event table. */
    private boolean m_isTableable;

    /** Whether this parameter is currently in the event table. */
    private boolean m_isTabled;

    /** The order of this parameter in the Method Table. */
    private int m_columnPosition;

    /** The Class of the values of this parameter. */
    private Class<?> m_myClass = null;

    /** The value(s) of this parameter. */
    private ArrayList<Object> m_values = new ArrayList<Object>();


    /**
     * Create a parameter with specified properties and no values.
     * @param name The name of the parameter.
     * @param type The parameter type (text, int, float, boolean).
     * @param isTableable Whether the parameter is put in the event table
     * when it is arrayed.
     * @param isArrayable Whether the parameter can be arrayed.
     * @param label The (localized) label for this parameter.
     */
    public LcMethodParameter(String name, String type, boolean isTableable,
                             boolean isArrayable, String label) {
        m_name = name;
        m_type = type;
        m_isArrayable = isArrayable;
        m_isTableable = isTableable;
        m_label = label;
        m_columnPosition = -1;
        int capacity = isArrayable ? 10 : 1;
        m_values = new ArrayList<Object>(capacity);
        m_isTabled = false;
    }

    /**
     * Sets relative position of this parameter when put in the
     * Method Table. Positions increase monotonically left-to-right,
     * but not all parameters with priorities will be in the table.
     * Has no effect if this parameter cannot be put in the table.
     * @param position The relative position ( > 0 ).
     */
    public void setColumnPosition(int position) {
        if (m_isTableable) {
            m_columnPosition = position;
        }
    }

    /**
     * Gets relative position of this parameter when put in the
     * Method Table. Positions increase monotonically left-to-right,
     * but not all parameters with priorities will be in the table.
     * @return The relative position ( < 0 if not tableable).
     */
    public int getColumnPosition() {
        return m_columnPosition;
    }

    /**
     * Returns the name of the parameter.
     */
    public String getName() {
        return m_name;
    }

    /**
     * Returns the (localized) label of the parameter.
     */
    public String getLabel() {
        return m_label;
    }

    /**
     * Returns true if this parameter is in the method table; i.e.,
     * its value is a (possibly constant!) function of time.
     */
    public boolean isTabled() {
        return m_isTabled;
        //return m_isTableable && m_values.size() > 1;
    }

    /**
     * Returns true if this parameter is in the method table; i.e.,
     * its value is a (possibly constant!) function of time.
     */
    public void setTabled(boolean b) {
        if (!m_isTableable) {
            b = false;
        }
        m_isTabled = b;
        if (!m_isTabled && m_isTableable) {
            while (m_values.size() > 1) {
                m_values.remove(1);
            }
        }
    }

    /**
     * Get the parameter's first value as an Object.
     * If there is no value, returns null.
     */
    public Object getValue() {
        return getValue(0);
    }

    /**
     * Get the parameter's initial value as an Object.
     * If the first element is null, returns the value of the first
     * non-null element.
     * @return The value of the first non-null element, or null if all elements
     * are null.
     */
    public Object getInitialValue() {
        for (Object value : m_values) {
            if (value != null) {
                return value;
            }
        }
        return null;
    }

    /**
     * Gets the Class of the values of this parameter.
     * This is used by LcMethodTableModel.getColumnClass(int)
     * to assign the renderer and editor for the table column that
     * holds the values of this parameter.
     * @return The class.
     */
    public Class<?> getParameterClass() {
        if (m_myClass == null) {
            if (m_type.equals(STRING_TYPE)) {
                m_myClass = String.class;
            } else if (m_type.equals(INT_TYPE)) {
                m_myClass = Integer.class;
            } else if (m_type.equals(DOUBLE_TYPE)) {
                m_myClass = Double.class;
            } else if (m_type.equals(BOOLEAN_TYPE)){
                m_myClass = Boolean.class;
            } else if (m_type.equals(MS_SCAN_TYPE)){
                m_myClass = MsScanMenu.class;
            } else {
                m_myClass = String.class;
            }
        }
        return m_myClass;
    }

    /**
     * Gets an Object representing the default value for this
     * class of parameter.
     */
    public Object getDefaultValue() {
        if (m_type.equals(STRING_TYPE) || m_type.equals(MS_SCAN_TYPE)) {
            return "";
        } else if (m_type.equals(INT_TYPE)) {
            return new Integer(0);
        } else if (m_type.equals(DOUBLE_TYPE)) {
            return new Double(0.0);
        } else {                // Default is BOOLEAN_TYPE
            return Boolean.FALSE;
        }
    }

    /**
     * Get a given element of parameter's value as an Object.
     * If the element does not exist, returns null.
     * Elements may also have null values.
     * @param element The element index.
     */
    public Object getValue(int element) {
        Object rtn = null;
        if (m_values.size() > element) {
            rtn = m_values.get(element);
        }
        return rtn;
    }

    /**
     * Get the parameter's first value as a String.
     * If there is no value, returns null.
     * Boolean values are returned as "y" or "n".
     */
    public String getStringValue() {
        return toString();
    }

    /**
     * Get the parameter's n'th value as a String.
     * If there is no n'th value, returns null.
     * Boolean values are returned as "y" or "n".
     * @param n The index of the value to return (from 0).
     */
    public String getStringValue(int n) {
        if (m_values.size() <= n) {
            return null;
        } else {
            return toString(m_values.get(n));
        }
    }

    /**
     * Get all the parameter's values as an array of Strings.
     * <pre>
     * If there is no value, returns a 0 length array.
     * Some elements may be null.
     * </pre>
     */
    public String[] getAllValueStrings() {
        int size = m_values.size();
        String[] rtn = new String[size];
        for (int i = 0; i < size; i++) {
            rtn[i] = toString(m_values.get(i));
        }
        return rtn;
    }

    /**
     * Get all the parameter's values as an array of Objects.
     * <pre>
     * If there is no value, returns a 0 length array.
     * Some elements may be null.
     * </pre>
     */
    public Object[] getAllValueObjects() {
        int size = m_values.size();
        Object[] rtn = new Object[size];
        for (int i = 0; i < size; i++) {
            rtn[i] = m_values.get(i);
        }
        return rtn;
    }

    /**
     * Clear all values from the parameter.
     */
    public void clear() {
        m_values.clear();
    }

    /**
     * Delete elements from the table, specified by index number (from 0).
     * @param elems Array of indices to delete, in ascending order.
     */
    public void deleteElements(int[] elems) {
        // Go in reverse order so indices don't change before we remove them
        for (int i = elems.length - 1; i >= 0; --i) {
            if (m_values.size() > elems[i]) {
                // NB: values list may be short; trailing values assumed null
                m_values.remove(elems[i]);
            }
        }
    }

    /**
     * Set the parameter's values from an array of Strings.
     */
    public void setAllValues(String[] sValues) {
        m_values.clear();
        int len = sValues.length;
        for (int i = 0; i < len; i++) {
            m_values.add(toObject(sValues[i]));
        }
        setTabled(len > 1);
    }

    /**
     * Set the parameter's values from an array of Objects.
     * It is up to the caller to know that the Objects have the correct
     * run-time type.
     */
    public void setAllValues(Object[] oValues) {
        m_values.clear();
        int len = oValues.length;
        for (int i = 0; i < len; i++) {
            m_values.add(oValues[i]);
        }
        setTabled(len > 1);
    }

    /**
     * Set the parameter's values from an array of Objects.
     * Does not set trailing null values.
     * It is up to the caller to know that the Objects have the correct
     * run-time type.
     */
    public void setNonNullValues(Object[] oValues) {
        m_values.clear();
        int len = oValues.length;
        for (int i = 0; i < len; i++) {
            if (oValues[i] != null) {
                insertValue(i, oValues[i]);
            }
        }
        setTabled(m_values.size() > 1);
    }

    /**
     * Converts a String value to an Object appropriate for this parameter.
     * <br>If the given string is null or "null", returns a null object.
     * <br>Boolean values should be input as "y" or "n".
     * @param value The string to translate.
     * @return Returns a Boolean, Integer, Double, String, or null.
     */
    public Object toObject(String value) {
        Object rtn = null;
        try {
            rtn = toObject(value, m_type);
        } catch (NumberFormatException nfe) {
            Messages.postDebug("LcMethodParameter.toObject: "
                               + "bad number string \"" + value + "\""
                               + " for parameter " + getName());
            rtn = toObject("0", m_type); // default numerical value
        }
        return rtn;
    }

    /**
     * Insert a given Object after at a given position.
     * Values formerly at the given position or higher are moved up.
     */
    public void insertValue(int idx, Object value) {
        while (m_values.size() < idx) {
            // Pad with null values so size is at least idx
            m_values.add(null);
        }
        m_values.add(idx, value);
        setTabled(m_values.size() > 1);
    }

    /**
     * Set one of the parameter's values to a given Object.
     */
    public boolean setValue(int idx, Object value) {
        Object oldValue = getValue(idx);
        if (oldValue == value || (value != null && value.equals(oldValue))) {
            return false;
        }
        while (m_values.size() <= idx) {
            // Pad with null values so size is at least (idx + 1)
            m_values.add(null);
        }
        m_values.set(idx, value);
        setTabled(m_values.size() > 1);
        Messages.postDebug("LcMethodParameter",
                           "setValue: old=" + oldValue
                           + ", new=" + value
                           + ", " + m_name + "[0]=" + toString());
        return true;
    }

    /**
     * Set one of the parameter's values from a string.
     */
    public void setValue(int idx, String value) {
        Messages.postDebug("LcMethodParameter",
                           "LcMethodParameter.setValue(" + idx
                           + ", " + value + ")"
                           + ", name=" + getName()
                           + ", obj=" + toObject(value));
        while (m_values.size() <= idx) {
            // Pad with null values so size is at least (idx + 1)
            m_values.add(null);
        }
        m_values.set(idx, toObject(value));
    }

    /**
     * Set the parameter's values from a "long string" in a way that can
     * be handled by Vnmr.
     * Problematic characters are replaced with escape sequences.
     * If the string value is too long (for Vnmr), it is broken up,
     * the parameter is arrayed, and the pieces of the string are
     * placed in successive parameter elements.
     * @see vnmr.util.StrUtil#insertEscapes
     * @param value The string to encode and put in the values array.
     */
    public void setLongStringValue(String value) {
        clear();
        value = StrUtil.insertEscapes(value);
        String[] values = StrUtil.split(value, StrUtil.MAXSTR);
        for (int i = 0; i < values.length; i++) {
            setValue(i, values[i]);
        }
    }

    /**
     * Get the string made by concatenating the strings from all the
     * elements of the parameter.
     * Any escape sequences of the form <code>&#ddd.</code> are converted
     * to the appropriate character.
     * @return The resultant, decoded string.
     */
    public String getLongStringValue() {
        int size = m_values.size();
        if (size == 0) {
            return "";
        } else {
            StringBuffer sb = new StringBuffer(getStringValue(0));
            for (int i = 1; i < size; i++) {
                sb.append(getStringValue(i));
            }
            return StrUtil.decodeEscapes(sb);
        }
    }

    /**
     * Translate a given string into an object of the given type.
     * <br>Types are STRING_TYPE, INT_TYPE, DOUBLE_TYPE, and BOOLEAN_TYPE.
     * Any other type string will return a null object.
     * <br>If the given string is null or "null", returns a null object.
     * <br>Boolean values should be input as "y" or "n".
     * @param value The string to translate.
     * @param type Specifies the type of object to return.
     * @return Returns a Boolean, Integer, Double, String, or null.
     */
    public static Object toObject(String value, String type)
        throws NumberFormatException {

        Object rtn = null;
        if (value != null && !value.equals("null")) {
            if (type.equals(BOOLEAN_TYPE)) {
                if (value.equals("y")) {
                    rtn = Boolean.TRUE;
                } else if (value.equals("n")) {
                    rtn = Boolean.FALSE;
                }
            } else if (type.equals(STRING_TYPE)) {
                rtn = value;
            } else if (type.equals(INT_TYPE)) {
                rtn = Integer.valueOf(value);
            } else if (type.equals(DOUBLE_TYPE)) {
                rtn = Double.valueOf(value);
            } else if (type.equals(MS_SCAN_TYPE)) {
                rtn = value;
            }
        }
        return rtn;
    }

    /**
     * Translate the value of the first element of this parameter into a string.
     * @return Returns null if there are no values.
     * @see #toString(Object)
     */
    public String toString() {
        if (m_values.size() == 0) {
            return null;
        } else {
            return toString(m_values.get(0));
        }
    }

    /**
     * Translate a given object into a string representation of its value.
     * The object is expected to be a number, a Boolean, or a String.
     * <pre>
     * If the given object is null, returns "null".
     * Boolean values are returned as "y" or "n".
     * </pre>
     * @param obj The object to convert into a string.
     */
    public static String toString(Object obj) {
        String rtn;
        if (obj == null) {
            rtn = "null";
        } else if (obj instanceof Boolean) {
            rtn = (Boolean)obj ? "y" : "n";
        } else {
            rtn = obj.toString();
        }
        return rtn;
    }

    /**
     * Returns the integer index value in a paramenter specification
     * such as "myname[3]".
     * Indices in the parname start at 1 (as in Vnmr MAGICAL);
     * the returned value starts at 0 (as in Java).
     * If no element is specified (as in "myname"), returns 0 -- the first
     * element.
     * @param parname The full pararmeter name, such as "myname[3]".
     * @return The index of the element referred to.
     */
    static public int getParIndex(String parname) {
        int rtn = 0;
        if (parname != null) {
            int idx0 = parname.indexOf("[");
            int idx1;
            if (idx0 >= 0 && (idx1 = parname.indexOf("]")) > idx0) {
                try {
                    rtn = Integer.parseInt(parname.substring(++idx0, idx1)) - 1;
                } catch (NumberFormatException nfe) {
                    // No valid index, return 0
                }
            }
        }
        return rtn;
    }

    /**
     * Strips any index part off of a parameter name.
     * For example, turns "myname[3]" into "myname".
     * @param parname The parameter name, perhaps with an index in brackets.
     * @return The part of the name before any brackets.
     */
    static public String stripParIndex(String parname) {
        String rtn = parname;
        int idx;
        if (parname != null && (idx = parname.indexOf("[")) >= 0) {
            rtn = parname.substring(0, idx);
        }
        return rtn;
    }
}
