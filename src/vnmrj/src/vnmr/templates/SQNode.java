/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.templates;

import static vnmr.templates.ProtocolBuilder.ATTR_DATA;
import static vnmr.templates.ProtocolBuilder.ATTR_LOCK;
import static vnmr.templates.ProtocolBuilder.ATTR_NAME;
import static vnmr.templates.ProtocolBuilder.ATTR_PSLABEL;
import static vnmr.templates.ProtocolBuilder.ATTR_STATUS;
import static vnmr.templates.ProtocolBuilder.ATTR_TIME;
import static vnmr.templates.ProtocolBuilder.ATTR_TITLE;
import static vnmr.templates.ProtocolBuilder.ATTR_TYPE;
import static vnmr.templates.ProtocolBuilder.ATTR_WHEN;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A container class with get/set methods for the standard attributes
 * of a node. Arbitrary attributes may be accessed with the
 * setAttr and getAttr methods. All attribute values are strings,
 * but for some standard attributes, the string represents a double
 * or boolean value. Boolean values are represented by the strings
 * "on" (true) or "off" (false).
 */
public class SQNode {
    private Map<String,String> values = new HashMap<String,String>();
    private static final String[] stdAttributes = {ATTR_NAME,
                                                   ATTR_PSLABEL,
                                                   ATTR_TIME,
                                                   ATTR_TYPE,
                                                   ATTR_STATUS,
                                                   ATTR_LOCK,
                                                   ATTR_WHEN,
                                                   ATTR_DATA,
                                                   ATTR_TITLE };


    /**
     * Make a node with no attributes defined.
     */
    public SQNode() {
    }

    /**
     * Make a node with all standard attributes specified.
     * @param cpName The node name, e.g., "PROTON_01".
     * @param psLabel The pulse sequence label, e.g., "PROTON".
     * @param acqTime The duration of the acquisition in seconds.
     * @param type E.g., "LIB".
     * @param status The status string - from the list in ProtocolBuilder,
     * e.g., COMPLETED (="Completed").
     * @param locked True if this node is locked (cannot be changed
     * from the interface.)
     * @param when "_day", "_night", or "".
     * @param data FID file where the data is stored.
     * @param title Typically similar to cpName.
     */
    public SQNode(String cpName, String psLabel, double acqTime,
                  String type, String status, boolean locked,
                  String when, String data, String title) {

        setCpName(cpName);
        setPsLabel(psLabel);
        setAcqTime(acqTime);
        setType(type);
        setStatus(status);
        setLocked(locked);
        setWhen(when);
        setData(data);
        setTitle(title);
    }

    /**
     * Get all the standard attribute names as an array of strings.
     * @return The attribute names.
     */
    public static String[] getStandardAttributes() {
        return stdAttributes;
    }

    /**
     * Set the given attribute to the given value.
     * @param name The name of the attribute.
     * @param value The value of the attribute.
     */
    public void setAttr(String name, String value) {
        values.put(name, value);
    }

    /**
     * Get the value of the given attribute.
     * @param name The name of the attribute.
     * @return The value of the attribute.
     */
    public String getAttr(String name) {
        return values.get(name);
    }

    /**
     * Produces a string giving the names and values of all
     * the defined attributes.
     * @return The string representing this node.
     */
    public String toString() {
        StringBuffer sb = new StringBuffer("SQNodeInfo:");
        for (String attr : stdAttributes) {
            sb.append(attr).append("=").append(getAttr(attr)).append(",");
        }
        sb.deleteCharAt(sb.length() - 1);
        return sb.toString();
    }

    /**
     * Get the value of the attribute ProtocolBuilder.ATTR_NAME.
     * @return The cpname of this node.
     */
    public String getCpName() {
        return getAttr(ATTR_NAME);
    }

    /**
     * Set the value of the attribute ProtocolBuilder.ATTR_NAME.
     * @param cpName The cpname of this node.
     */
    public void setCpName(String cpName) {
        setAttr(ATTR_NAME, cpName);
    }

    /**
     * Get the value of the attribute ProtocolBuilder.ATTR_PSLABEL.
     * @return The Pulse Sequence label for this node.
     */
    public String getPsLabel() {
        return getAttr(ATTR_PSLABEL);
    }

    /**
     * Set the value of the attribute ProtocolBuilder.ATTR_PSLABEL.
     * @param psLabel The Pulse Sequence label for this node.
     */
    public void setPsLabel(String psLabel) {
        setAttr(ATTR_PSLABEL, psLabel);
    }

    /**
     * Get the value of the attribute ProtocolBuilder.ATTR_TIME.
     * @return The acquisition time (s).
     */
    public Double getAcqTime() {
        return Double.parseDouble(getAttr(ATTR_TIME));
    }

    /**
     * Set the value of the attribute ProtocolBuilder.ATTR_TIME.
     * @param acqTime The acquisition time (s).
     */
    public void setAcqTime(double acqTime) {
        setAttr(ATTR_TIME, Double.toString(acqTime));
    }

    /**
     * Get the value of the attribute ProtocolBuilder.ATTR_TYPE.
     * @return The type of node.
     */
    public String getType() {
        return getAttr(ATTR_TYPE);
    }

    /**
     * Set the value of the attribute ProtocolBuilder.ATTR_TYPE.
     * @param type The type of node.
     */
    public void setType(String type) {
        setAttr(ATTR_TYPE, type);
    }

    /**
     * Get the value of the attribute ProtocolBuilder.ATTR_STATUS.
     * @return The status of the node, e.g., "Queued".
     */
    public String getStatus() {
        return getAttr(ATTR_STATUS);
    }

    /**
     * Set the value of the attribute ProtocolBuilder.ATTR_STATUS.
     * @param status The status of the node, e.g., "Queued".
     */
    public void setStatus(String status) {
        setAttr(ATTR_STATUS, status);
    }

    /**
     * Get the value of the attribute ProtocolBuilder.ATTR_LOCK.
     * @return True if this node is locked.
     */
    public Boolean isLocked() {
        return ("on" == getAttr(ATTR_LOCK));
    }

    /**
     * Set the value of the attribute ProtocolBuilder.ATTR_LOCK
     * to either "on" or "off".
     * @param cpName If true, "on" is set, otherwise "off".
     */
    public void setLocked(boolean locked) {
        setAttr(ATTR_LOCK, locked ? "on" : "off");
    }

    /**
     * Get the value of the attribute ProtocolBuilder.ATTR_WHEN.
     * @return The attribute's value, e.g., "_day".
     */
    public String getWhen() {
        return getAttr(ATTR_WHEN);
    }

    /**
     * Set the value of the attribute ProtocolBuilder.ATTR_WHEN.
     * @param when The attribute's value, e.g., "_day".
     */
    public void setWhen(String when) {
        setAttr(ATTR_WHEN, when);
    }

    /**
     * Get the value of the attribute ProtocolBuilder.ATTR_DATA.
     * @return The path to the FID data directory.
     */
    public String getData() {
        return getAttr(ATTR_DATA);
    }

    /**
     * Set the value of the attribute ProtocolBuilder.ATTR_DATA.
     * @param data The path to the FID data directory.
     */
    public void setData(String data) {
        setAttr(ATTR_DATA, data);
    }

    /**
     * Get the value of the attribute ProtocolBuilder.ATTR_TITLE.
     * @return The displayed title of the node.
     */
    public String getTitle() {
        return getAttr(ATTR_TITLE);
    }

    /**
     * Set the value of the attribute ProtocolBuilder.ATTR_TITLE.
     * @param title The displayed title of the node.
     */
    public void setTitle(String title) {
        setAttr(ATTR_TITLE, title);
    }

    /**
     * Get a string showing both the differences and the agreements between
     * this node and another node in the values of their standard
     * attributes. For each standard attribute, a line is written giving the
     * value or values.
     * @param other The node to compare with this one.
     * @return A multi-line string.
     */
    public String diff(SQNode other) {
        StringBuffer sb = new StringBuffer();
        for (String key : stdAttributes) {
            sb.append("  " + key + ": ");
            String myValue = getAttr(key);
            String otherValue = other.getAttr(key);
            if (myValue == null) {
                if (otherValue == null) {
                    sb.append("SAME: null");
                } else {
                    sb.append("DIFF: null --> " + otherValue);
                }
            } else {
                if (myValue.equals(otherValue)) {
                    sb.append("SAME: " + myValue);
                } else {
                    sb.append("DIFF: " + myValue + " --> " + otherValue);
                }
            }
            sb.append("\n");
        }
        return sb.toString();
    }

    /**
     * Update selected attributes in this node with the values in
     * the specified node.
     * @param node The node with the new values.
     * @return A list of the nodes that were changed.
     */
    public List<String> update(SQNode node) {
        // List of the attributes that we may need to update:
        final String[] updateAttrs = {
                                      ATTR_DATA,
                                      ATTR_STATUS,
                                      ATTR_TIME,
                                      ATTR_TITLE
        };

        List<String> changedNodes = new ArrayList<String>();
        for (String name : updateAttrs) {
            String oldValue = getAttr(name);
            String newValue = node.getAttr(name);
            if (oldValue == null || !oldValue.equals(newValue)) {
                setAttr(name, newValue);
                changedNodes.add(name);
            }
        }
        return changedNodes;
    }
}