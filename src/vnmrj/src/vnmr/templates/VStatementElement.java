/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import java.util.*;
import vnmr.ui.shuf.*;

/**
 *  StatementElement build element for ShufflerStatementBuilder
 *  @author		Dean Sindorf
 */
public class VStatementElement extends VElement
{
	public boolean isActive() { return true;}
	public StatementElement build(){
		boolean display=true;
		String value=getAttribute("value");
		if(getAttribute("display").equals("false"))
			display=false;
		return new StatementElement(type(),value,display);
	}		
}
