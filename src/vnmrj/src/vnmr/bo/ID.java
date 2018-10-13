/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

/**
 * identifier for domain objects
 *
 * @author Mark Cao
 */
public class ID {
    /** experiment identifier */
    public String id;

    /**
     * constructor
     */
    public ID(String id) {
	this.id = id;
    } // ID()

    /**
     * return hash code value of object
     * @return hash code
     */
    public int hashCode() {
	return (id != null) ? id.hashCode() : super.hashCode();
    } // hashCode()

    /************************************************** <pre>
     * Summary: Get the ID (name) in this object.
     *
     * Details: This method should be supplied when extended.
     *
     </pre> **************************************************/
    public String getName() {
	return "";
    }

    /**
     * to string
     * @return string
     */
    public String toString() {
	return id;
    } // toString()

} // class ID
