/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.util;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.SortedSet;
import java.util.StringTokenizer;
import java.util.TreeMap;
import java.util.TreeSet;

/**
 * Class for holding the value(s) of a Vnmr parameter. Also has static methods
 * for reading parameter values from a Vnmr parameter file.
 */
public class VnmrParameter {
    private String parName;
    private ArrayList<Object> parValues;
    private boolean parIsDouble;

//    public VnmrParameter(String name) {
//        parName = name;
//    }

    public VnmrParameter(String name, ArrayList<Object> values) {
        parName = name;
        parValues = values;
        parIsDouble = (values != null
                       && values.size() > 0
                       && values.get(0) instanceof Double);
    }

    public String getName() {
        return parName;
    }

    public double getDoubleValue() {
        return getDoubleValue(0);
    }

    public double getDoubleValue(int idx) {
        double rtn = Double.NaN;
        Object obj;
        if ((parValues.size() > idx)
                && ((obj = parValues.get(idx)) instanceof Double))
        {
            rtn = (Double)obj;
        }
        return rtn;
    }

    public String getStringValue() {
        return getStringValue(0);
    }

    public String getStringValue(int idx) {
        String rtn = null;
        Object obj;
        if ((parValues.size() > idx)
                && ((obj = parValues.get(idx)) instanceof String))
        {
            rtn = (String)obj;
        }
        return rtn;
    }

    public static String[] getStringValues(String path, String name) {
        String[] values = new String[0];
        String[] names = {name};
        try {
            Map<String,VnmrParameter> map = readParameters(path, names);
            VnmrParameter par = map.get(name);
            values = par.parValues.toArray(values);
        } catch (NullPointerException npe) {
            // Probably a NullPointer or ClassCast exception
        }
        return values;
    }

//    static public Map<String,VnmrParameter> readParameters(String path,
//                                                          String[] names) {
//        Map<String,VnmrParameter> map = new TreeMap<String,VnmrParameter>();
//        Set<String> nameSet = new HashSet<String>();
//        for (String name : names) {
//            nameSet.add(name);
//        }
//        BufferedReader in = null;
//        try {
//            String line;
//            in = new BufferedReader(new FileReader(path));
//            while ((line = in.readLine()) != null) {
//                if (Character.isLetter(line.charAt(0))) {
//                    int idxNameEnd = line.indexOf(' ');
//                    String name = line.substring(0, idxNameEnd);
//                    if (nameSet.contains(name)) {
//                        /*CMP*/
//                    }
//                }
//            }
//        } catch (IOException ioe) {
//            Messages.postError("VnmrParameter.readParameters: "
//                               + "Error reading file: " + path);
//        }
//        try { in.close(); } catch (Exception e) {}
//        return map;
//    }

    static public Map<String,VnmrParameter> readParameterFile(String path) {
        // Read the parameter file into a buffer.
        // NB: This returns a 0 length buffer on failure:
        String parBuf = readFileIntoBuffer(path);
        return parseParameterBuffer(parBuf);
    }

    static public Map<String,VnmrParameter> readParameters(String path,
                                                           String[] names) {
        // Read the lines for the specified parameters into a buffer.
        // NB: This returns a 0 length buffer on failure:
        String parBuf = readParameterLines(path, names, false);
        if (parBuf.length() == 0) {
            return null;
        }
        return parseParameterBuffer(parBuf);
    }

    static public String readStringParameter(String path, String name) {
        String[] names = {name};
        String parBuf = readParameterLines(path, names, false);
        if (parBuf.length() == 0) {
            return null;
        }
        Map<String,VnmrParameter> parMap = parseParameterBuffer(parBuf);
        VnmrParameter param = parMap.get(name);
        return param == null ? null : param.getStringValue();
    }

    static public Map<String,VnmrParameter> parseParameterBuffer(String buf) {
        Map<String,VnmrParameter> map = new TreeMap<String,VnmrParameter>();

        // Parse the file for parameters, storing the
        // values in the proper VnmrParameters as we go along.
        int bufIdx = -1;         // Running offset into the buffer
        bufIdx = findNextParameter(buf, bufIdx);
        while (bufIdx >= 0) {
            String name = getParameterName(buf, bufIdx);
            boolean isString = getParameterType(buf, bufIdx);

            int parEnd = findNextParameter(buf, bufIdx);
            QuotedStringTokenizer toker;
            toker  = getValueTokenizer(buf, bufIdx, parEnd, isString);
            if (toker == null) {
                break;
            }

            int nVals = Integer.parseInt(toker.nextToken());
            ArrayList<Object> parValues = new ArrayList<Object>(nVals);
            double idx;
            for (idx = 0; idx < nVals && toker.hasMoreTokens(); idx++) {
                Object val = toker.nextToken();
                if (!isString) {
                    val = Double.parseDouble((String)val);
                }
                parValues.add(val);
            }
            map.put(name, new VnmrParameter(name, parValues));
            bufIdx = parEnd;    // bufIdx at next parameter name, or -1
        }

        return map;
    }

    /**
     * Read the given text file into a String.
     * Lines in the buffer are separated by "\n" regardless of the
     * convention used by the input file.
     * @param filepath The full path and name of the file to read.
     * @return The buffer. The buffer is empty on failure, never null.
     */
    static private String readFileIntoBuffer(String filepath) {
        StringBuffer sb = new StringBuffer();
        BufferedReader in = null;
        try {
            String line;
            in = new BufferedReader(new FileReader(filepath));
            while ((line = in.readLine()) != null) {
                sb.append(line).append("\n");
            }
        } catch (IOException ioe) {
            Messages.postError("VnmrParameter.readFileIntoBuffer: "
                               + "Error reading file: " + filepath);
        }
        try { in.close(); } catch (Exception e) {}
        return sb.toString();
    }

    /**
     * Read some of the given parameter file into a String.
     * Reads only lines for the parameters in the given list of names.
     * Lines in the buffer are separated by "\n" regardless of the
     * convention used by the input file.
     * @param filepath The full path and name of the file to read.
     * @param names The names of the parameters to read.
     * @param errMsgFlag If true, a message is printed on error.
     * @return The buffer. The buffer is empty on failure, never null.
     */
    static private String readParameterLines(String filepath, String[] names,
                                             boolean errMsgFlag) {
        Set<String> nameSet = new TreeSet<String>();
        for (String name : names) {
            nameSet.add(name);
        }
        StringBuffer sb = new StringBuffer();
        BufferedReader in = null;
        try {
            String line;
            in = new BufferedReader(new FileReader(filepath));
            String prevName = "";
            while ((line = in.readLine()) != null && nameSet.size() > 0) {
                if (Character.isLetter(line.charAt(0))) {
                    nameSet.remove(prevName);
                    prevName = getParameterName(line, 0);
                }
                if (nameSet.contains(prevName)) {
                    sb.append(line).append("\n");
                }
            }
        } catch (IOException ioe) {
            if (errMsgFlag) {
                Messages.postError("VnmrParameter.readFileIntoBuffer: "
                                   + "Error reading file: " + filepath);
            }
        }
        try { in.close(); } catch (Exception e) {}
        return sb.toString();
    }

    /**
     * Given a buffer containing a parameter file and an index into
     * that buffer, finds the beginning of the next parameter name.
     * Starts looking at the given index, for a line-feed followed
     * by a letter (the first letter of the parameter name).
     * Exception, if the index is <0, returns 0 if it's a letter.
     * @param buf The buffer containing the parameter file.
     * @param idx Where to start looking in the buffer. Set idx=-1 to
     * search from the beginning.
     * @return The index of the beginning of the parameter name. 
     */
    static private int findNextParameter(String buf, int idx) {
        int len = buf.length();
        if (idx >= len) {
            return -1;
        }
        if (idx < 0 && Character.isLetter(buf.charAt(0))) {
            // Special case: first line of file starts with parameter name
            return 0;
        }
        char chr = buf.charAt(idx);
        // Find first newline followed by a leter.
        for ( ; idx < len; idx++) {
            chr = buf.charAt(idx);
            if (buf.charAt(idx) == '\n') {
                // Check next character
                if (++idx >= len) {
                    return -1;  // EOF
                } else {
                    chr = buf.charAt(idx);
                    if (Character.isLetter(chr)) {
                        return idx;
                    }
                }
            }
        }
        return -1;
    }

    /**
     * Get the parameter name that starts at the given index.
     * 
     * @param buf The buffer containing the parameter file.
     * @param idx Where the name starts in the buffer.
     * @return The parameter name. 
     */
    static private String getParameterName(String buf, int idx) {
        int len = buf.length();
        //int end;
        // Find the space at the end of the name
        //for (end = idx + 1 ; end < len && buf.charAt(end) != ' '; end++);
        int end = buf.indexOf(" ", idx + 1);
        return buf.substring(idx, end);
    }

    /**
     * Given a buffer containing a parameter file
     * and an index into that buffer pointing to a parameter name,
     * finds the type of that parameter.
     * 
     * @param buf The buffer containing the parameter file.
     * @param idx Where the name starts in the buffer.
     * @return True if the parameter is a string type.
     */
    static private boolean getParameterType(String buf, int idx) {
        boolean isString = false;
        int len = buf.length();
        int end = Math.min(idx + 80, len - 1);
        String tbuf = buf.substring(idx, end);
        StringTokenizer toker = new StringTokenizer(tbuf, " ");
        if (toker.hasMoreTokens()) {
            toker.nextToken();  // Skip over the name
        }
        if (toker.hasMoreTokens()) {
            toker.nextToken();  // Skip over the subtype
        }
        if (toker.hasMoreTokens()) {
            if (toker.nextToken().equals("2")) { // String is basictype 2
                isString = true;
            }
        }
        return isString;
    }

    /**
     * @param idx Points to somewhere in the first line of the parameter
     * entry.
     */
    static private QuotedStringTokenizer getValueTokenizer(String parBuf,
                                                           int idx,
                                                           int parEnd,
                                                           boolean isString) {
        int len = parBuf.length();
        QuotedStringTokenizer toker = null;
        // Advance to values line
        for ( ; idx < len && parBuf.charAt(idx) != '\n'; idx++);
        if (idx == len) {
            return null;        // Premature EOF
        }
        idx++;

        String parEntry;
        if (parEnd < 0) {
            parEntry = parBuf.substring(idx);
        } else {
            parEntry = parBuf.substring(idx, parEnd);
        }
        //if (isString) {
        toker = new QuotedStringTokenizer(parEntry, "\n ", "\"");
        //} else {
        //    toker = new StringTokenizer(parEntry, "\n ");
        //}
        return toker;
    }



}
