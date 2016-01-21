/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.awt.*;
import java.util.*;
import javax.swing.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;

/********************************************************** <pre>
 * Summary: Individual elements from Shuffler Statement config file.
 *
 * @author  Glenn Sullivan
 * Details:
 *   Each Element from the file, is saved in one of these Elements.
 * 
 </pre> **********************************************************/

public class StatementElement {
    /** ObjectType, Attribute-1, etc. */
    String   elementType;
    /** Display one screen or not */
    boolean  displayElement;
    /** Value/s assigned to this element */
    String[] elementValues;
    /** Jbutton, Jlabel etc. */
    JComponent  jcomponent;	
    

    /************************************************** <pre>
     * constructor for single string of values to be parsed.
     </pre> **************************************************/
    public StatementElement(String etype, String values, boolean dis) {

	elementType = etype;
	displayElement = dis;

	StringTokenizer tok = new StringTokenizer(values, "$");

	int numValues = tok.countTokens();
	elementValues = new String[numValues];
	for(int i=0; i < numValues; i++) {
	    elementValues[i] = tok.nextToken();
	}
    }


    /************************************************** <pre>
     * get elementType.
     </pre> **************************************************/
    public String getelementType() {
	return elementType;
    }

    /************************************************** <pre>
     * get displayElement.
     </pre> **************************************************/
    public boolean getdisplayElement() {
	return displayElement;
    }

    /************************************************** <pre>
     * set displayElement.
     </pre> **************************************************/
    public  void setdisplayElement(boolean dis) {
	displayElement = dis;
    }

    /************************************************** <pre>
     * get elementValues.
     </pre> **************************************************/
    public String[] getelementValues() {
	return elementValues;
    }

    /************************************************** <pre>
     * set elementValues.
     </pre> **************************************************/
    public void setelementValues(String val, int index) {
	elementValues[index] = val;
    }

    /************************************************** <pre>
     * set jcomponent.
     </pre> **************************************************/
    public void setjcomponent(JComponent jcomponent) {
	this.jcomponent = jcomponent;
    }

    /************************************************** <pre>
     * get jcomponent.
     </pre> **************************************************/
    public JComponent getjcomponent() {
	return jcomponent;
    }

    /************************************************** <pre>
     * Output String of appropriate members.
     *
     </pre> **************************************************/


    public String toString() {
	String str;
	
	str = new String(elementType + "  ");
	str = str.concat("display=" + displayElement + "  ");
	for(int i=0; i < elementValues.length; i++)
	    str = str.concat(elementValues[i] + "  ");
	
	return str;
    }


}
