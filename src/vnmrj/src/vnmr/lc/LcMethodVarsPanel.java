/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;


import java.awt.event.ActionListener;
import java.util.*;

import javax.swing.JComponent;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.templates.*;


public class LcMethodVarsPanel extends VContainer {

    public LcMethodVarsPanel(SessionShare sshare, ButtonIF vif, String typ,
                             String file) {
        super(sshare, vif, typ);
        String filepath = FileUtil.openPath(file);
        if (filepath == null) {
            Messages.postError("LC panel definition file not found: " + file);
            return;
        }
        try {
            LayoutBuilder.build(this, vif, filepath);
        } catch (Exception e) {
            Messages.postError(e.toString());
            Messages.writeStackTrace(e);
        }
    }

    /**
     * Gets a list of all the VObjIFs in this panel.
     * @return The list.  Could be empty, but never null.
     */
    public List<VObjIF> getVObjs() {
        List<VObjIF> list = new LinkedList<VObjIF>();
        VObj.getVObjs(this, list);
        return list;
    }

    /**
     * Sets the action listener for the VObjIFs in this panel to
     * the given listener.
     * Only components with the "actionCmd" attribute set will be connected,
     * and the action command will be set to the value of the keyword.
     * @param listener The ActionListener to hook up to the components.
     */
    public void setActionListener(ActionListener listener) {
        List<VObjIF> vobjList =  getVObjs();
        for (VObjIF vobj : vobjList) {
            String actionCmd = vobj.getAttribute(ACTIONCMD);
            if (actionCmd != null && vobj instanceof ActionComponent) {
                ((ActionComponent)vobj).addActionListener(listener);
            }
        }
    }

    /**
     * Updates components that must change to track a change in
     * another (given) component.
     */
    public void handleChange(VObjIF comp, LcMethodParameter parm) {
        // TODO: Implement handleChange().
    }

    /**
     * Initializes all the VObjIFs in this panel to the values in the
     * given LC method.
     * Only components with the actionCmd attribute set to "VObjAction"
     * will be set.
     * @param method The method containing the values.
     */
    public void setValues(LcMethod method) {
        List<VObjIF> vobjList =  getVObjs();
        for (VObjIF vobj : vobjList) {
            String actionCmd = vobj.getAttribute(ACTIONCMD);
            if (actionCmd != null && actionCmd.startsWith("VObjAction")) {
                String fullParname = vobj.getAttribute(PANEL_PARAM);
                int idx = LcMethodParameter.getParIndex(fullParname);
                String parname = LcMethodParameter.stripParIndex(fullParname);
                if (parname != null) {
                    LcMethodParameter lcpar = method.getParameter(parname);
                    if (lcpar != null) {
                        String value;
                        if (vobj instanceof VTextWin
                            && parname.equals(fullParname))
                        {
                            value = lcpar.getLongStringValue();
                        } else {
                            value = lcpar.getStringValue(idx);
                        }
                        if (value == null) {
                            value = "";
                        }
                        if (vobj instanceof VGroup) {
                            setShowValue(vobj, fullParname, value);
                        } else {
                            vobj.setValue(new ParamIF(parname, "", value));
                            boolean isTabled = lcpar.isTabled();
                            if (vobj instanceof VLcTableItem) {
                                // NB: only VLcTableItem's can be tabled
                                ((VLcTableItem)vobj).setTabled(isTabled);
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * Set the "enabled" status of all the method components in this panel.
     * Only components with the actionCmd attribute set to "VObjAction"
     * will be set.
     * @param enabled If false, all are disabled.
     */
    public void setEnabled(LcMethod method, boolean enabled) {
        List<VObjIF> vobjList =  getVObjs();
        for (VObjIF vobj : vobjList) {
            String actionCmd = vobj.getAttribute(ACTIONCMD);
            if (actionCmd != null && actionCmd.startsWith("VObjAction")) {
                String fullParname = vobj.getAttribute(PANEL_PARAM);
                String parname = LcMethodParameter.stripParIndex(fullParname);
                if (parname != null) {
                    LcMethodParameter lcpar = method.getParameter(parname);
                    if (lcpar != null) {
                        ((JComponent)vobj).setEnabled(enabled);
                    }
                }
            }
        }
    }

    private void setShowValue(VObjIF vobj, String param, String paramValue) {
        String showCondition = vobj.getAttribute(VALUES);
        if (showCondition != null) {
            boolean negate = showCondition.startsWith("!");
            if (negate) {
                showCondition = showCondition.substring(1);
            }
            int idx = paramValue.indexOf(showCondition);
            String showValue = (negate == idx < 0) ? "1" : "0";
            vobj.setShowValue(new ParamIF(param, "", showValue));

            // Remember that this component listens to this parameter
            
        }
    }

}
