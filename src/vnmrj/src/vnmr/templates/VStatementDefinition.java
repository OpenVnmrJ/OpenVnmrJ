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
import java.io.*;
import vnmr.ui.shuf.*;

/**
 *  StatementDefinition build element for ShufflerStatementBuilder
 *  @author		Dean Sindorf
 */
public class VStatementDefinition extends VElement
{
	public boolean isActive() { return true;}
	public StatementDefinition build()  throws Exception{
		Enumeration     	elems=children();
		VElement			child;
		ArrayList 			items=new ArrayList();
		StatementDefinition sd;

		while(elems.hasMoreElements()){
			child=(VElement)elems.nextElement();
			if(child instanceof VStatementElement){
				StatementElement item=((VStatementElement)child).build();
				items.add(item);
			}
		}

		try {
			sd = new StatementDefinition(items);
		}
		catch (Exception e) {
			throw new Exception();
		}
		return sd;
	}		
}
