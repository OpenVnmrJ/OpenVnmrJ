/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import com.sun.xml.tree.TreeWalker;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

/**
 * A utility class for traversing element trees 
 *  @author		Dean Sindorf
 */
public class ElementTree extends TreeWalker {
	public ElementTree(Node initial) {
		super(initial);
	}
	public VElement rootElement() {
		return (VElement)getCurrent();
	}
	public VElement nextElement(String tag) {
		return (VElement)getNextElement(tag);
	}
	public VElement nextElement() {
		return (VElement)getNextElement(null);
	}
	public VElement nextAttribute(String attr) {
		VElement e=(VElement)getCurrent();
		if(e !=null && e.hasAttribute(attr)){
			nextElement();
			return e;
		}
		while(e !=null && !e.hasAttribute(attr))
			e=nextElement();
		nextElement();
		return e;
	}
}
