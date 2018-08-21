/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.templates;

import java.util.ArrayList;


/**
 * This class is a list of SQNodes plus a few convenience methods.
 */
public class SQNodeList extends ArrayList<SQNode> {

    /**
     * This ID number should be changed if the class's structure changes.
     */
    private static final long serialVersionUID = 5617623595748075728L;

    /**
     * Makes a human-readable string that lists all the nodes
     * in this object.
     * @return The string representing this object.
     */
    public String toString() {
        StringBuffer sb = new StringBuffer("SQNodeList:size=")
                .append(this.size()).append("{");
        for (SQNode node : this) {
            sb.append(node.getCpName()).append(",");
        }
        sb.setCharAt(sb.length() - 1, '}');
        return sb.toString();
    }

    /**
     * Determines if this list contains a node with the given
     * value of cpName.
     * @param cpName The cpName to look for.
     * @return True if this cpName is in a node in the list.
     */
    public boolean containsCpName(String cpName) {
        return indexOfCpName(cpName) >= 0;
    }

    /**
     * Get the location of the first node with the given cpName.
     * @param cpName The cpName to look for.
     * @return The index of the node in this list, or -1 on failure.
     */
    public int indexOfCpName(String cpName) {
        if (cpName != null) {
            for (int i = 0; i < size(); i++) {
                if (get(i).getCpName().equals(cpName)) {
                    return i;
                }
            }
        }
        return -1;
    }

}