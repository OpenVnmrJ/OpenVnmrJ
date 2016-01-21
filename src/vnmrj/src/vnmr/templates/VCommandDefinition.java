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
import org.w3c.dom.*;

/**
 *  build element for CommandBuilder
 *  @author		Dean Sindorf
 */
public class VCommandDefinition extends VElement
{
	public boolean isActive() { return true;}
	public CommandDefinition build(){
		NamedNodeMap attrs=getAttributes();
		Hashtable hash=new Hashtable();
		for(int i=0;i<attrs.getLength();i++){
			Node a=attrs.item(i);
			hash.put(a.getNodeName(),a.getNodeValue());
		}
		return new CommandDefinition(hash,type());
	}		
}
