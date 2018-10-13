/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.util.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import javax.swing.text.*;
import javax.swing.plaf.*;
import vnmr.util.*;
import vnmr.ui.*;

/*********************************************************** <pre>
 * Primarily, VParamArray is used to update JTables of ParamArrayPanel 
 * with the values returned by sending jFunc(25), jFunc(28, id, param). 
 *
 * For example:
 *	VParamArray obj = new VParamArray(null, vnmrIf, null);
 *	obj.setPanel(panel);
 *	obj.setAttribute(CMD, "jFunc(25)");
 *	obj.updateValue();
 *
 * where panel is an object of Component (can be casted to, for example,
 * ParamArrayPanel) that provides public method(s) to handle the values
 * returned from vnmrbg in the setValue method of this class.
 *
 * VParamArray can also be used to send any vnmrCmd that does not expect 
 * return values using setAttribute(CMD, ...), or query parameters by 
 * setAttribute(SETVAL, ...). 
 *
 * For example:
 *	obj.setAttribute(CMD, "jFunc(26)");	

 * or	obj.setAttribute(CMD, "pw = 4,5,6,7,8");
 *
 * or   obj.setAttribute(SETVAL, "$VALUE=nt");
 *	obj.setPanel(panel);
 *
 * For querying vnmrbg, the returned value will be handle by public method(s)
 * of the panel object. i.e., if you want to do something with the returned
 * value(s) in certain panel, you have to call setPanel(panel) and call
 * the panel's method. Panel's method is called in setValue method of this class.
 * setValue needs to be extended for each new case. See current setValue
 * method for examples.
 * 
 * type is used to specify the return type, i.e., ARRAY, aval, eval..
 * This is different from type in other V widgets.
 *
 </pre> **********************************************************/

public class VParamArray extends Component implements VObjIF, VObjDef {
    public String type = null;
    public String value = null;
    public String vnmrCmd = null;
    public String setVal = null;
    public String precision = null;
    private String paramName = null;
    private ButtonIF vnmrIf;
    private Component panel;

    public static final int FUNC25 = 25;
    public static final int FUNC26 = 26;
    public static final int FUNC28 = 28;

    public VParamArray(SessionShare sshare, ButtonIF vif, String typ) {
	this.vnmrIf = vif;
        this.type = typ;
        this.value = " ";
    }

    public void setPanel(Component p) {
	this.panel = p;
    }

    public String getAttribute(int attr) {
        switch (attr) {
          case TYPE:
                     return type;
          case VALUE:
                     return value;
          case CMD:
                     return vnmrCmd;
	  case SETVAL:
                     return setVal;
          case NUMDIGIT:
                     return precision;
          default:
                    return null;
        }
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
          case CMD:
                     vnmrCmd = c;
                     break;
          case SETVAL:
                     setVal = c;
                     break;
          case NUMDIGIT:
                     precision = c;
                     break;
        }
    }

    public ButtonIF getVnmrIF() {
	return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
	vnmrIf = vif;
    }

    public void updateValue() {
	if (vnmrIf == null) return;

	if (vnmrCmd != null) {

	    int cmdType = -1;
	    StringTokenizer tok = new StringTokenizer(vnmrCmd, " ,()\n");
	    String cmd = tok.nextToken();

	    // for array parameter jFuncs 
	    if(cmd.equals("jFunc") && tok.hasMoreTokens()) {

		cmdType = (Integer.valueOf(tok.nextToken())).intValue();

	        if(cmdType == FUNC25) {
		    type = "ARRAY";
	    	    vnmrIf.asyncQueryARRAY(this, cmdType, null);
		} else if(cmdType == FUNC28 && tok.hasMoreTokens()) {
	            type = "aval";
		    tok.nextToken();  // skip the id
		    if(tok.hasMoreTokens()) {
			paramName = tok.nextToken();
	    	    	vnmrIf.asyncQueryARRAY(this, cmdType, paramName);
		    }
		} else if(cmdType == FUNC26) {
		    type = null; 
		    vnmrIf.sendVnmrCmd(this, vnmrCmd); 
		}
	    } 
	    // send cmd to vnmrbg, don't expect return value; 
	    else { type = null;
		   vnmrIf.sendVnmrCmd(this, vnmrCmd);
	    }
        } 
	// query vnmrbg for parameter value(s). 
	// parameter name is save in paramName, since the return value
        // does not have any name attached to it.
	if(setVal != null) {
	    paramName = setVal.substring(7);
	    type = "eval";
	    vnmrIf.asyncQueryParam(this, setVal, precision);
	}
    }

    public void setValue(ParamIF pf) {
	if (pf != null) {
    	    value = pf.value;

	    if(type.equals("ARRAY")) {
		ParamArrayPanel paramArrayPanel = (ParamArrayPanel) panel;
	        paramArrayPanel.vnmrbgUpdateArrayedParams(value);
	    } 

	    //else if(type.equals("aval") && value.indexOf("NOVALUE") == -1) {
	    else if(type.equals("aval")) {
		ParamArrayPanel paramArrayPanel = (ParamArrayPanel) panel;
		paramArrayPanel.vnmrbgUpdateArrayValues(paramName, value);
	    } 

	    else if(type.equals("eval") && paramName.equals("$totalTime")) {
		ParamArrayPanel paramArrayPanel = (ParamArrayPanel) panel;
		paramArrayPanel.setTotalTime(value);
	    }
	}
    }

    public void setEditStatus(boolean s) { }
    public void setEditMode(boolean s) { }
    public void addDefChoice(String s) { }
    public void addDefValue(String s) { }
    public void setDefLabel(String s) { }
    public void setDefColor(String s) { }
    public void setDefLoc(int x, int y) { }
    public void changeFont() { }
    public void refresh() { }
    public void setShowValue(ParamIF p) { }
    public void changeFocus(boolean s) { }
    public void destroy() { }
    public void setModalMode(boolean s) { }
    public void sendVnmrCmd() { }
    public void setSizeRatio(double x, double y) {}
    public Point getDefLoc() { return getLocation(); }
}

