/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.cryo;


import java.awt.event.ActionListener;
import java.util.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.templates.*;


public class CryoPanel extends VContainer implements StatusListenerIF{

    private static final String UPLOAD_COUNT_LABEL = "Records Uploaded: ";
    private VGroup m_advanced = null;
    private VButton m_detach = null;
    private VButton m_stop = null;
    private VButton m_start = null;
    private VButton m_vacpurge = null;
    private VButton m_probepump = null;
    private VButton m_thermcycle = null;
    private VButton m_cryosend = null;
    private VButton m_thermsend = null;
    private String m_filepath;
    private MessageIF m_cryoMsg;
    private VLabel m_uploadCount;

    public CryoPanel(SessionShare sshare, ButtonIF vif, String typ,
                             String file, MessageIF cryoMsg) {
        super(sshare, vif, typ);
        m_cryoMsg = cryoMsg;
        m_filepath = FileUtil.openPath(file);
        if (m_filepath == null) {
            m_cryoMsg.postError("Cryo panel file not found: " + file);
            return;
        }
        try {
            LayoutBuilder.build(this, vif, m_filepath);
        } catch (Exception e) {
            m_cryoMsg.postError(e.toString());
            m_cryoMsg.writeStackTrace(e);
        }
        
        ExpPanel.addStatusListener(this);
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
            String label = vobj.getAttribute(LABEL);
            if (actionCmd != null && vobj instanceof ActionComponent) {
                ((ActionComponent)vobj).addActionListener(listener);
                //m_cryoMsg.postDebug("cryocmd", "Action Command: "+ actionCmd);
                if(vobj instanceof VButton){
                    if(actionCmd.startsWith("detach")) m_detach= (VButton) vobj;
                    else if(actionCmd.startsWith("start")) m_start= (VButton) vobj;
                    else if(actionCmd.startsWith("stop")) m_stop= (VButton) vobj;
                    else if(actionCmd.startsWith("vacpurge")) m_vacpurge= (VButton) vobj;
                    else if(actionCmd.startsWith("probepump")) m_probepump= (VButton) vobj;
                    else if(actionCmd.startsWith("thermcycle")) m_thermcycle= (VButton) vobj;
                    else if(actionCmd.startsWith("sendToCryobay")) m_cryosend= (VButton) vobj;
                    else if(actionCmd.startsWith("sendToTemp")) m_thermsend= (VButton) vobj;
                }
            } else if (label != null && label.startsWith(UPLOAD_COUNT_LABEL)) {
                m_uploadCount= (VLabel)vobj;
            }
        }
    }

    /**
     * Sets the action listener for the VObjIFs in this panel to
     * the given listener.
     * Only components with the "actionCmd" attribute set will be connected,
     * and the action command will be set to the value of the keyword.
     */
    public void setStatusListener() {
        List<VObjIF> vobjList =  getVObjs();

        //m_cryoMsg.postDebug("cryocmd", "setting up status listners:");

        for (VObjIF vobj : vobjList) {

            String label = vobj.getAttribute(LABEL);
            if(vobj instanceof VTextMsg){
                ExpPanel.addStatusListener((StatusListenerIF)vobj);
            } else if (vobj instanceof VGroup && label!=null){
                //m_cryoMsg.postDebug("cryocmd", "CryoPanel: Show VGroup: " + label);
                if(label.startsWith("Advanced")) {
                    //((VGroup)vobj).setvisible(false);
                    m_advanced = (VGroup)vobj;
                    m_advanced.setVisible(false);
                }
            } 
        }
    }

    public void setAdvanced(boolean state) {
        m_advanced.setVisible(state);
    }

    public void setDetach(boolean state) {
        if(m_detach!=null) m_detach.setEnabled(state);
    }

    public void setStart(boolean state) {
        if(m_start!=null) m_start.setEnabled(state);
    }

    public void setStop(boolean state) {
        if(m_stop!=null) m_stop.setEnabled(state);
    }

    public void setVacPurge(boolean state) {
        if(m_vacpurge!=null) m_vacpurge.setEnabled(state);
    }

    public void setProbePump(boolean state) {
        if(m_probepump!=null) m_probepump.setEnabled(state);
    }

    public void setThermCycle(boolean state) {
        if(m_thermcycle!=null) m_thermcycle.setEnabled(state);
    }

    public void setCryoSend(boolean state) {
        if(m_cryosend!=null) m_cryosend.setEnabled(state);
    }

    public void setThermSend(boolean state) {
        if(m_thermsend!=null) m_thermsend.setEnabled(state);
    }

    /**
     * Updates components that must change to track a change in
     * another (given) component.
     * @param str The new status.
     */
    public void updateStatus(String str){
    	/*if(str.startsWith("cryo")){
    		m_cryoMsg.postDebug("cryocmd", "CryoPanel.update Status "+str);  	
    	} */ 	
    }

    public void updateStatusMessage(String msg){
    	ExpPanel.getStatusValue(msg);
    	//m_cryoMsg.postDebug("cryocmd", "CryoPanel.updateStatusMessage " + msg);
    }


    public void setUploadCount(int i) {
        if (m_uploadCount != null) {
            m_uploadCount.setAttribute(LABEL, UPLOAD_COUNT_LABEL + i);
        }
    }

//    /**
//     * Initializes all the VObjIFs in this panel to the values in the
//     * given LC method.
//     * Only components with the actionCmd attribute set to "VObjAction"
//     * will be set.
//     */
//    public void setValues() {
//        List<VObjIF> vobjList =  getVObjs();
//        for (VObjIF vobj : vobjList) {
//            String statpar = vobj.getAttribute(STATPAR);
//            if (statpar != null) {
//                String fullParname = vobj.getAttribute(PANEL_PARAM);
//                int idx = CryoParameter.getParIndex(fullParname);
//                String parname = CryoParameter.stripParIndex(fullParname);
//                if (parname != null) {
//                    CryoParameter lcpar = method.getParameter(parname);
//                    if (lcpar != null) {
//                        String value;
//                        if (vobj instanceof VTextWin
//                            && parname.equals(fullParname))
//                        {
//                            value = lcpar.getLongStringValue();
//                        } else {
//                            value = lcpar.getStringValue(idx);
//                        }
//                        if (value == null) {
//                            value = "";
//                        }
//                        if (vobj instanceof VGroup) {
//                            setShowValue(vobj, fullParname, value);
//                        } else {
//                            vobj.setValue(new ParamIF(parname, "", value));
//                            //((Component)vobj).setEnabled(!lcpar.isTabled());
//                        }
//                    }
//                }
//            }
//            //m_cryoMsg.postDebug("cryocmd", "cryopanel setting values: " + statpar);
//            //vobj.setValue(new ParamIF(statpar, "", value));
//            //vobj.setDefLabel(value);
//        }
//    }

//    private void setShowValue(VObjIF vobj, String param, String paramValue) {
//        String showCondition = vobj.getAttribute(VALUES);
//        if (showCondition != null) {
//            boolean negate = showCondition.startsWith("!");
//            if (negate) {
//                showCondition = showCondition.substring(1);
//            }
//            int idx = paramValue.indexOf(showCondition);
//            String showValue = (negate == idx < 0) ? "1" : "0";
//            vobj.setShowValue(new ParamIF(param, "", showValue));
//
//            // Remember that this component listens to this parameter
//            
//        }
//    }

}

