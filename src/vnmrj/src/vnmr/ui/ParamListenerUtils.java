/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import java.util.*;
import javax.swing.*;
import vnmr.bo.*;

/**
 * Static methods for implementing the ParamListener interface.
 */
public class  ParamListenerUtils implements VObjDef{

    /**
     * Update the given component with a given parameter vector.
     * If the given component is a VGroupIF or a VContainer, it's
     * children will also get updated (recursively).
     * @param jc The given component.
     * @param params The list of paramenters and their values.
     */
    static public void updateValue(JComponent jc, Vector params) {
	StringTokenizer tok;
	String          vars, v;
	boolean 	got;
	int		k;
	int nums = jc.getComponentCount();
        // Unfortunately VSplitPane has to be treated differently
        // to access its contents.
        if (jc instanceof VSplitPane) {
            nums = 2;
        }
	int pnum = params.size();
	for (int i = 0; i < nums; i++) {
	    Component comp;
            if (jc instanceof VSplitPane) {
                VSplitPane sc = (VSplitPane)jc;
                comp = i == 0 ? sc.getLeftComponent() : sc.getRightComponent();
            } else {
                comp = jc.getComponent(i);
            }
	    if (comp instanceof VObjIF) {
	    	if (comp instanceof VGroupIF || comp instanceof VContainer) {
		    ParamListenerUtils.updateValue((JComponent)comp, params);
		}
                VObjIF obj = (VObjIF)comp;
		vars = obj.getAttribute(VARIABLE);
		if (vars != null) {
                    tok = new StringTokenizer(vars, " ,\n");
                    got = false;
                    while (tok.hasMoreTokens()) {
                        v = tok.nextToken();
                        for (k = 0; k < pnum; k++) {
                            if (v.equals(params.elementAt(k))) {
                                got = true;
                                obj.updateValue();
                                break;
                            }
                        }
                        if (got)
                            break;
                    }
                }
            }
        }
	tok = null;
    }

    /**
     * Update this component all the VObjIF's that it may contain.
     *@param jc The top level component.
     */
    static public void updateAllValue(JComponent jc) {
	int nums = jc.getComponentCount();
	for (int i = 0; i < nums; i++) {
	    Component comp = jc.getComponent(i);
	    if (comp instanceof VObjIF) {
		// NB: If comp is a VGroup, it will update its children.
                ((VObjIF)comp).updateValue();
	    }
	}
    }
}
