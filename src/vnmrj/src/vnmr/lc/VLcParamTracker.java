/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.util.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;

import static vnmr.lc.LcMethodParameter.*;


public class VLcParamTracker extends VObj {

    // Vnmrbg command to get arrayed values
    static final int JEXECARRAYVAL = 28;

    private String subtype = null;
    private String tabled = null;
    private boolean firsttime = true;
    private String[] m_strValueArray = new String[0];

    public VLcParamTracker(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
    }

    private boolean isTrue(String booString) {
        return (booString != null && booString.startsWith("y"));
    }

    /**
     *  Sets the value for various attributes.
     *  @param attr attribute to be set.
     *  @param c    value of the attribute.
     */
    public void setAttribute(int attr, String c) {
        if (c != null)
        {
            c = c.trim();
            if (c.length() == 0) {c = null;}
        }
        switch (attr) {
          case SUBTYPE:
            subtype = c;
            break;
         default:
            super.setAttribute(attr, c);
            break;
        }
    }

    /**
     *  Gets that value of the specified attribute.
     *  @param attr attribute whose value should be returned.
     */
    public String getAttribute(int attr) {
        String strAttr;
        switch (attr) {
          case SUBTYPE:
            strAttr = subtype;
            break;
          default:
            strAttr = super.getAttribute(attr);
            break;
        }
        return strAttr;
    }

    public void updateValue() {
        if (vnmrIf != null && vnmrVar != null) {
            vnmrIf.asyncQueryARRAY(this, JEXECARRAYVAL, vnmrVar);
        }
    }

    public void setValue(ParamIF pf) {
        Messages.postDebug("VLcParamTracker",
                           "VLcParamTracker[" + vnmrVar + "].setValue(): \""
                           + pf.value + "\"");
        String pfval = pf.value;
        if (pfval != null) {
            if (pfval.indexOf("NOVALUE") >= 0) {
                Messages.postError("VLcParamTracker for param=\"" + vnmrVar
                                   + "\", received \"NOVALUE\".");
                return;
            }
            String type = "";
            int count = 0;
            StringTokenizer toker = new StringTokenizer(pfval, " ;\t");
            int n = toker.countTokens();
            if (n >= 2) {
                type = toker.nextToken();
                if (type.equals("s") || type.equals("r")) {
                    try {
                        count = Integer.parseInt(toker.nextToken());
                    } catch (NumberFormatException nfe) {
                        // Not really an "aval" string; count still 0
                    }
                }
                if (count == 0) {
                    n = 1;      // Not really an "aval" string
                }
            }
                
            String allValues = "";
            if (n < 2) {
                // Not an "aval" string
                // NB: if all white space (and semicolons), sets a null value
                pfval = pfval.trim();
                if (!pfval.equals(value)) {
                    Messages.postDebug("VLcParamTracker",
                                       "...SET UNARRAYED VALUE");
                    allValues = pfval;
                }
            }
            n -= 2;
            if (n > 0) {
                allValues = toker.nextToken("").trim();
            }
            if (count == 1) {
                String[] elems = {allValues};
                setValue(elems);
            } else {
                toker = new StringTokenizer(allValues, " ;\t");
                value = "";
                String[] elems = new String[count];
                for (int i = 0; i < count && toker.hasMoreTokens(); i++) {
                    if (i > 0) {
                        value = value + ",";
                    }
                    if (type.equals("s")) {
                        elems[i] = toker.nextToken(";").trim();
                        value = value + "'" + Util.escape(elems[i], "\'") + "'";
                    } else if (type.equals("r")) {
                        elems[i] = toker.nextToken();
                        value = value + elems[i];
                    }
                }
                setValue(elems);
            }
        }
    }

    public void setValue(String[] vals) {
        m_strValueArray = vals;
    }

    public Object getObjectForValue(String strValue) {

        if (subtype == null || strValue == null) {
            return null;
        }
        StringTokenizer toker = new StringTokenizer(strValue, "' ");
        if (toker.hasMoreTokens()) {
            strValue = toker.nextToken("");
        } else {
            return null;
        }
        if (strValue.equals("null")) {
            return null;
        }
        
        if (subtype.equals(STRING_TYPE) || subtype.equals(MS_SCAN_TYPE)) {
            return strValue;
        } else if (subtype.equals(INT_TYPE)) {
            if (strValue.length() == 0) {
                return new Integer(0);
            } else {
                return new Integer(strValue);
            }
        } else if (subtype.equals(DOUBLE_TYPE)) {
            if (strValue.length() == 0) {
                return new Double(0);
            } else {
                return new Double(strValue);
            }
        } else if (subtype.equals(BOOLEAN_TYPE)) {
            return new Boolean(isTrue(strValue));
        }
        return null;
    }

    /**
     * Get the number of values in this, possibly arrayed, item.
     */
    public int getArraySize() {
        return m_strValueArray.length;
    }

    /**
     * Get the object containing the value for the first element.
     */
    public Object getValue() {
        return getValue(0);
    }

    /**
     * Get the object containing the value for the i'th element.
     */
    public Object getValue(int idx) {
        Object rtn = null;
        if (idx < m_strValueArray.length) {
            rtn = getObjectForValue(m_strValueArray[idx]);
        }
        return rtn;
    }
}
