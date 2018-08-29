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
 * param identifier
 *
 */
public class ImageFileType extends ID {
    /**
     * constructor
     */
    public ImageFileType(String id) {
	super(id);
    } // Param()

    /**
     * Get ImageFileType name. For now, just use the id for the name.
     * @return name
     */
    public String getName() {
	return id;
    } // getName()

    /**
     * indicates whether the given object is equal to this one
     * @return boolean
     */
    public boolean equals(Object obj) {
	if (this == obj)
	    return true;
	if (id != null && obj instanceof ImageFileType)
	    return id.equals(((ImageFileType)obj).id);
	return false;
    } // equals()

} // class ImageFileType
