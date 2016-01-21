/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package	 vnmr.ui;

/**
 * value of a header cell
 *
 * @author Mark Cao
 */
public class HeaderValue {
    /** text label for header (possibly null) */
    String text;
    /** is this the lock header? */
    boolean lock;
    /** are there to be two lines? */
    boolean twoLine;
    /** is the order of results keyed on this header? */
    boolean keyed;
    /** is the order ascending? */
    boolean ascending;

    /**
     * constructor
     * @param text label for header; if null, assume lock
     * @param twoLine indicates use of two lines
     */
    public HeaderValue(String text, boolean twoLine) {
	this.text = text;
	lock = (text == null);
	this.twoLine = twoLine;
	keyed = false;
	ascending = true;
    } // HeaderValue()

    /**
     * get text (label)
     * @return text
     */
    public String getText() {
	return text;
    } // getText()

    /**
     * set text (label)
     * @return void
     */
    public void setText(String newText) {
	text = new String(newText);
    } // setText()


    /**
     * is this the lock header?
     * @return boolean
     */
    public boolean isLock() {
	return lock;
    } // isLock()

    /**
     * Is this header to have two display lines?
     * @return boolean
     */
    public boolean isTwoLine() {
	return twoLine;
    } // isTwoLine()

    /**
     * set keyed status
     * @param keyed is header keyed?
     */
    public void setKeyed(boolean keyed) {
	this.keyed = keyed;
    } // setKeyed()

    /**
     * Is this header keyed? "Keyed" means the header determines
     * the ascending or descending order of results.
     * @return boolean
     */
    public boolean isKeyed() {
	return keyed;
    } // isKeyed()

    /**
     * set order: true for ascending, false for descending
     * @param ascending is ascending?
     */
    public void setAscending(boolean ascending) {
	this.ascending = ascending;
    } // setAscending()

    /**
     * When keyed, is the order ascending (or descending)?
     * @return boolean
     */
    public boolean isAscending() {
	return ascending;
    } // isAscending()

} // class HeaderValue
