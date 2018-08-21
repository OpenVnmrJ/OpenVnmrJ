/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import java.lang.*;

/**
 * This  tokenizer is modified from StringTokenizer
 */

public class VStrTokenizer implements Enumeration {
    private int currentPosition;
    private int newPosition;
    private int maxPosition;
    private String str;
    private String delimiters;
    private boolean retDelims;
    private boolean delimsChanged;

    /**
     * maxDelimChar stores the value of the delimiter character with the
     * highest value. It is used to optimize the detection of delimiter
     * characters.
     */
    private char maxDelimChar;

    /**
     * Set maxDelimChar to the highest char in the delimiter set.
     */
    private void setMaxDelimChar() {
        if (delimiters == null) {
            maxDelimChar = 0;
            return;
        }

	char m = 0;
	for (int i = 0; i < delimiters.length(); i++) {
	    char c = delimiters.charAt(i);
	    if (m < c)
		m = c;
	}
	maxDelimChar = m;
    }

    public VStrTokenizer(String str, String delim, boolean returnDelims) {
	currentPosition = 0;
	newPosition = -1;
	delimsChanged = false;
	this.str = str;
	maxPosition = str.length();
	delimiters = delim;
	retDelims = returnDelims;
        setMaxDelimChar();
    }

    public VStrTokenizer(String str, String delim) {
	this(str, delim, false);
    }

    public VStrTokenizer(String str) {
	this(str, " \t\n\r\f", false);
    }

    private int skipDelimiters(int startPos) {
        if (delimiters == null)
            throw new NullPointerException();

        int position = startPos;
	while (!retDelims && position < maxPosition) {
            char c = str.charAt(position);
            if ((c > maxDelimChar) || (delimiters.indexOf(c) < 0))
                break;
	    position++;
	}
        return position;
    }

    /**
     * Skips ahead from startPos and returns the index of the next delimiter
     * character encountered, or maxPosition if no such delimiter is found.
     */
    private int scanToken(int startPos) {
        int position = startPos;
        while (position < maxPosition) {
            char c = str.charAt(position);
            if ((c <= maxDelimChar) && (delimiters.indexOf(c) >= 0))
                break;
            position++;
	}
	if (retDelims && (startPos == position)) {
            char c = str.charAt(position);
	    if ((c <= maxDelimChar) && (delimiters.indexOf(c) >= 0))
                position++;
        }
        return position;
    }

    public boolean hasMoreTokens() {
	/*
	 * Temporary store this position and use it in the following
	 * nextToken() method only if the delimiters have'nt been changed in
	 * that nextToken() invocation.
	 */
	newPosition = skipDelimiters(currentPosition);
	return (newPosition < maxPosition);
    }

    public String nextToken() {
	/* 
	 * If next position already computed in hasMoreElements() and
	 * delimiters have changed between the computation and this invocation,
	 * then use the computed value.
	 */

	currentPosition = (newPosition >= 0 && !delimsChanged) ?  
	    newPosition : skipDelimiters(currentPosition);

	/* Reset these anyway */
	delimsChanged = false;
	newPosition = -1;

	if (currentPosition >= maxPosition)
		return null;
	int start = currentPosition;
	currentPosition = scanToken(currentPosition);
	return str.substring(start, currentPosition);
    }

    public String nextToken(String delim) {
	delimiters = delim;

	/* delimiter string specified, so set the appropriate flag. */
	delimsChanged = true;

        setMaxDelimChar();
	return nextToken();
    }

    public boolean hasMoreElements() {
	return hasMoreTokens();
    }

    public Object nextElement() {
	return nextToken();
    }

    public int countTokens() {
	int count = 0;
	int currpos = currentPosition;
	while (currpos < maxPosition) {
            currpos = skipDelimiters(currpos);
	    if (currpos >= maxPosition)
		break;
            currpos = scanToken(currpos);
	    count++;
	}
	return count;
    }

    public int getPosition() {
	return currentPosition;
    }

    public void rewind() {
	currentPosition = 0;
	newPosition = -1;
    }
}
