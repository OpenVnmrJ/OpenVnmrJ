/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.Vector;

import vnmr.util.*;
import vnmr.bo.*;







/********************************************************** <pre>
  Summary: Catch MAS status variables  
 
 
     To have the probe ID displayed in the hardware bar, we need to 
     set the vnmr variable 'probe' to the status variable 'probeId1'. 
 
     We keep thin wall speed limit and the std speed limit here and test
     the target against the appropriate one.  The controller cannot do this
     because it does not know about thinwall.  To monitor the setting
     of thinwall in the panel, we not only catch status variables, but
     we also catch pnew variables and look for masthinwall.

     We need to know the value of the vnmr variable 'spintype', so we catch
     that if it comes as a pnew, AND we need to know what it is when we
     start up.  Thus this class is a VObjIF.  We call asyncQueryParam() 
     if we do not yet have a value for spintype.  Then the value comes back
     at a later time and is caught in setValue().  We do not want to do
     things like check for the speed limit if we are not set to 'mas'
  
 </pre> **********************************************************/

public class MasStatusListener implements  StatusListenerIF, VObjIF
{
    /* Special speed limits for thin wall rotors, in Hz */
    public static final int THINWALL_32_LIMIT = 15000;
    public static final int THINWALL_40_LIMIT = 10000;
    public static final int THINWALL_50_LIMIT = 7000;
    public static final int THINWALL_60_LIMIT = 5000;
    public static final int THINWALL_14_LIMIT = 3500;
    // There is actually no thinwall 7.5, so just use the normal limit
    public static final int THINWALL_75_LIMIT = 7000;
    public static final int THINWALL_16_LIMIT = 45000;
    public static final int THINWALL_12_LIMIT = 65000;

    private static int spinLimit=0;
    private static int thinLimit=0;
    private static int target=0;
    private static String curProbeId="";
    private static boolean thinWall=true;
    private static boolean firstParamSetComplete=false;
    private static ExpPanel expPanel;
    private String string;
    private String masSpinTypeStr=null;
    private boolean masSpinTypeBool=true;
    private String probeNameHold=null;



    public MasStatusListener(ExpPanel expPanel) {

        this.expPanel = expPanel;

        // Register as a status listener.  updateStatus will be called
        // for every status variable that comes back from the hardware.
        ExpPanel.addStatusListener(this);
    }

    public void updateStatus(String str) {
        String string;
        String sizeStr;
        boolean thinwallExist = true;

        if(masSpinTypeStr == null) {
            // If not defined yet, then try to define it.  The value will
            // not actually arrive until later, and will be caught in
            // setValue() which will set masSpinTypeBool at that time.

            try {
                expPanel.asyncQueryParam(this, "$VALUE=spintype");
            }
            catch(Exception e) {
                // Stay silent
            }
        }
        // Only look for the things below if spintype is mas or not defined yet
        if(!masSpinTypeBool) {
            return;
        }

        if(str.indexOf("masSpeedLimit") != -1) {
            try {
                // Get the value after the param name and save it as a float
                string = str.substring("masSpeedLimit".length() +1);

                // Sometimes, when the limit has not been set yet, it will be
                // '-' which causes a problem with parseFloat(), so catch it.
                // If it is due to a neg number, it does not make any sense 
                // anyway.
                if(string.indexOf('-') == -1) {
                    int tmp = Integer.parseInt(string);
                    if (tmp > 0)
                       spinLimit = tmp;
                }
            }
            catch (Exception e) {
                Messages.postWarning("Problem setting Mas  masSpeedLimit "
                                     + "with cmd: " + str);
            }
        }
        else if(str.indexOf("probeId1") != -1) {
            /*
             * When the CM speed controller is reset, or a new probe is
             * plugged in, a new value of probeId1 will be sent up to
             * us.  When that happens, we need to default to thinwall = thin
             * and set spinmax to the appropriate thinwall max setting.  If
             * the user then changes thinwall to std spinmax should be set
             * to its default which came from the controller.  So, if,
             * the probe id changes, save the value sent to us for use 
             * if thinwall is changed, and then default thinwall to thin 
             * and set spinmax.
            */
            /* Set the global vnmr variable 'probe' to the value
               so that it will go into the Hardware Bar.
               Get the value after the param name and save it as a string
            */

            // Is this a new value or just a dash?
            String newProbeId = str.substring("probeId1".length() +1);

            if(newProbeId.equals(curProbeId) || newProbeId.equals("-")) {
                // Same name or dash, just get out of here
                return;
            }

            if(DebugOutput.isSetFor("masstatuslistener"))
                        Messages.postDebug("masStatusListener received new "
                                           + "probe ID = " + newProbeId);

            curProbeId = newProbeId;  // Save the new name

            // Look at the module id to get the size.  The size will be the
            // first 3 characters.
            if(curProbeId.startsWith("3.2"))
                thinLimit = THINWALL_32_LIMIT;
            else if(curProbeId.startsWith("4.0"))
                thinLimit = THINWALL_40_LIMIT;
             else if(curProbeId.startsWith("5.0"))
                thinLimit = THINWALL_50_LIMIT;
            else if(curProbeId.startsWith("14"))
                thinLimit = THINWALL_14_LIMIT;
            else if(curProbeId.startsWith("1.6")) 
                thinLimit = THINWALL_16_LIMIT;           
            else if(curProbeId.startsWith("1.2")) 
                thinLimit = THINWALL_12_LIMIT;       
            // The following have no thin wall
            else if(curProbeId.startsWith("7.5")) {
                thinLimit = THINWALL_75_LIMIT;
                thinwallExist = false;
            }
            else if(curProbeId.startsWith("9.0")) {
                thinwallExist = false;
            }
            else if(curProbeId.startsWith("6.0")) {
                thinLimit = THINWALL_60_LIMIT;
                thinwallExist = false;
            }

            
            else if(curProbeId.startsWith("-") || curProbeId.length() == 0)
                return;
            else {
                Messages.postError("Problem determining thinwall speed limit \n"
                                   + "    for probeId = :" + curProbeId + ":");
            }
           
            // Force to thin setting when a new probe id has been received.
            // This should only happen when the probe has been changed
            // or disconnected.  Don't do this if there is no thinwall
            // for this probe.
            if(thinwallExist)
                setMasThinWall("thin");
            
            // If masSpinTypeStr is not set yet, we should not set the
            // probe parameter.  Doing this can cause the liquids probe
            // specification to be set to a solids probe name.  We must
            // wait until masSpinTypeStr gets set to do this.
/*
 *      Setting probe parameter removed as part of VJ 40 project,
 */
/*
            if(!curProbeId.equals("-") && curProbeId.length() > 0) {
                // Set 'probe' to this value if the string is not a dash
                // Set it via Thread because at startup, the hardwareBar
                // Probe item is not updated properly if I don't wait.

                // Be sure there are no leading or tailing spaces and
                // convert other spaces to "_"
                String probeStr = curProbeId.trim();
                probeStr = probeStr.replaceAll(" ", "_");

                // Only set if masSpinTypeStr is defined
                if(masSpinTypeStr != null) {
                    updateParam("probe=\'" + probeStr + "\'");
                    if(DebugOutput.isSetFor("masstatuslistener"))
                        Messages.postDebug("masStatusListener setting probe "
                                           + " param to "
                                           + probeNameHold);

                }
                else {
                    // Since masSpinTypeStr is not set yet, we need to save
                    // this probe name and wait to see what spintype is set
                    // to before deciding whether to set the parameter 
                    // probe to this string or not.  Save the string.
                    probeNameHold = probeStr;
                }
            }
*/
        }
        else if(str.indexOf("masProfileSetting") != -1) {

            try {
                string = str.substring("masProfileSetting".length() +1);
                int profile;
                // It must have a value of 0 or 1.  I tried testing for
                // string.length() > 0 here, but sometimes, string has a
                // length of 1 but no printable value and we get an exception
                // from parseInt.  Thus, just test for 0 or 1 here.
                if(string.equals("1") || string.equals("0")) {
                    profile = Integer.parseInt(string);
                    // the string should be either 1 or 0 
                    updateParam("masprofile=" + profile);
                }
            }
            catch (IndexOutOfBoundsException ioobe) {
                // ignore this one, the string did not contain a value
            }
            catch (Exception e) {
                Messages.postWarning("Problem setting Mas Profile with cmd: "
                                     + str);
                Messages.writeStackTrace(e);
            }
        }
        else if(str.indexOf("masActiveSetPoint") != -1) {

            try {
                string = str.substring("masActiveSetPoint".length() +1);
                int asp;
                if(string.length() > 0 && !string.startsWith("-")) {
                    asp = Integer.parseInt(string);

                    updateParam("masactivesetpoint=" + asp);
                }
            }
            catch (Exception e) {
                Messages.postWarning("Problem setting Mas ActiveSetPoint "
                                     + "with cmd: " + str);
            }
        }
        // spinset is the target spin rate
        else if(str.indexOf("spinset") != -1) {
            try {
                string = str.substring("spinset".length() +1);
                if(string.length() > 0 && !string.startsWith("-")) {
                    // Remove ' Hz' from string
                    string = string.substring(0, string.indexOf(" Hz"));
                    target = Integer.parseInt(string);
                }

                /* Check target for being over the limit. */
                ckMasTarget();
            }
            catch (Exception e) {
                Messages.postWarning("Problem setting Mas spinset "
                                     + "with cmd: " + str);
            }

        }
    }

    /* Check target for being over the appropriate limit based on thinwall.
       If over limit, send cmd to set target to the appropriate limit.
    */
    public void ckMasTarget() {
       /* Do not send sethw commands. Since status updates are sent to all
        * VnmrJ sessions, if one person's session tries to set a spinning speed then
        * another person's session may send a sethw, affecting the first person.
        * This was happining. Person A had a nano-probe and tried to set the spinning
        * speed. Person B had a session running as a data station, but their parameters
        * were set for a MAS probe. When person A tried to regulate spinning speed
        * at 2000, person B's session sent a sethw('spin',0,'nowait'). Not good.
        */

        return;
/*
        // Only test limit if we already have a value for masSpinTypeStr,
        // and spintype is mas.
        if(!masSpinTypeBool || masSpinTypeStr == null) 
            return;

        if(DebugOutput.isSetFor("masstatuslistener")) {
            if(thinWall)
                Messages.postDebug("ckMasTarget checking target (" + target
                                   + ") against thinLimit (" + thinLimit + ")");
            else
                Messages.postDebug("ckMasTarget checking target (" + target
                                   + ") against spinLimit (" + spinLimit + ")");
        }

        // Check target for above max
        if(thinWall && target > thinLimit) {
            Messages.postError("MAS target above limit for thinwall rotor."
                                + " Resetting.");

            // Set target to limit
            updateParam("sethw(\'spin\', " + thinLimit + ", \'nowait\')");
        }
        else if (!thinWall && target > spinLimit) {
            Messages.postError("MAS target above limit for this probe."
                                + " Resetting.");
            // Set target to limit
            updateParam("sethw(\'spin\', " + spinLimit + ", \'nowait\')");
        }
 */
    }

    /* Set the vnmr parameter masThinwall to the argument status. */
    public void setMasThinWall(String status) {

        if(status.equals("thin")) {
            // Set thinwall setting locally
            thinWall = true;
            // Set the vnmr variable which the panel is watching
            updateParam("masthinwall=\'thin\'");

            // We also need to set the vnmr variable that the panel
            // speed limit is watching.
            updateParam("masspeedlimit=" + thinLimit);

            if(DebugOutput.isSetFor("masstatuslistener"))
                Messages.postDebug("Setting thinwall to " + status
                                   + "; Setting speed limit to " + thinLimit);
            
            // Check the target speed if thinwall = true
            ckMasTarget();
        }
        else {
            // Set thinwall setting locally
            thinWall = false;
            // Set the vnmr variable which the panel is watching
            updateParam("masthinwall=\'std\'");

            // We also need to set the vnmr variable that the panel
            // speed limit is watching.
            updateParam("masspeedlimit=" + spinLimit);

            if(DebugOutput.isSetFor("masstatuslistener"))
                Messages.postDebug("Setting thinwall to " + status
                                   + "; Setting speed limit to " + spinLimit);
         }
    }


    /* We need to know when the thinwall status is changed via the panel.
       This does not go nor come from vxWorks as a status parameter, because
       the mas controller does not know about thinwall.  We have to insert
       ourselves into the system in this class and deal with thinwall.
       ExpPanel.c was modified to call this method when a vnmr parameter
       has been changed.  In here we need to check for masthinwall, and if
       it is in the list, get its value and update the thinWall flag
       as necessary.  Then check the current target against the limit.

       parValList is a Vector of Strings in the form
           [attr1, attr2, attr3, val1, val2, val3] where the number
        of attr is variable.

       We also catch spintype here when it is modified by anyone.
    */
    public void updateValue(Vector parValVector) {
        int size;

	size = parValVector.size();
        // Loop thru the attribute name in the first half of the Vector
	for (int i = 0; i < size/2; i++) {
            String paramName = (String)parValVector.elementAt(i);
            if(paramName.equals("masthinwall")) {
  
                // Found it, now get the value from the last half of the Vector
                String val = (String)parValVector.elementAt(size/2 +i);

                if(DebugOutput.isSetFor("masstatuslistener"))
                    Messages.postDebug("Received notice of masthinwall value: "
                                       + val);
                
                // Did the value change?
                if(val.equals("thin") && thinWall == true)
                    return;
                if(!val.equals("thin") && thinWall == false)
                    return;

                // It changed, set the local flag and vnmr variable
                // and test current target.
                setMasThinWall(val);
            }
            // catch spintype if it is changed.
            else if(paramName.equals("spintype")) {
                String val = (String)parValVector.elementAt(size/2 +i);

                // Save value of spintype for general use
                if(val.equals("mas")) 
                    masSpinTypeBool = true;
                else
                    masSpinTypeBool = false;
            }
        }
    }


    /* When vnmrj starts up, we seem to have trouble getting vnmr parameters
       set at first.  I found that waiting a bit in a thread, caused it to
       work properly.  This only needs to be done at startup.  There is a
       variable firstParamSetComplete which is false at startup.  After
       the thread completes its first parameter setting, it will clear this
       flag, and we will not use the thread thereafter.  Since we may receive
       several commands to set params before it completes the first one, we
       will keep setting via thread until we know it is okay to stop using
       the thread method.
    */
    private void updateParam(String cmd) {

        if(firstParamSetComplete)
            Util.sendToVnmr(cmd);
        else {
            UpdateParamThread updateParamThread;
            updateParamThread = new UpdateParamThread(cmd);
            updateParamThread.setPriority(Thread.MIN_PRIORITY);
            updateParamThread.setName("Update Parameter");
            updateParamThread.start();
        }

    }


    /* When vnmrj starts up, we seem to have trouble getting vnmr parameters
       set at first.  I found that waiting a bit in a thread, caused it to
       work properly.  This only needs to be done at startup.  There is a
       variable firstParamSetComplete which is false at startup.  After
       the thread completes its first parameter setting, it will clear this
       flag, and we will not use the thread thereafter.  Since we may receive
       several commands to set params before it completes the first one, we
       will keep setting via thread until we know it is okay to stop using
       the thread method.
    */
    class UpdateParamThread extends Thread {
        String cmd;

        public UpdateParamThread(String cmd) {
            this.cmd = new String(cmd);
        }

        public void run() {
            try {
                // Wait a few seconds, or until the first thread actually
                // completes and clears the flag.
                for(int i=0; i < 40; i++) {
                    sleep(100);
                    if(firstParamSetComplete)
                        break;
                }
                Util.sendToVnmr(cmd);
                if(DebugOutput.isSetFor("masstatuslistener")) {
                    Messages.postDebug("UpdateParamThread sent: " + cmd);
                }

                firstParamSetComplete=true;
            }
            catch (Exception e) {}
        }

    }


    /**************** VObjIF Interface methods ***********************
      We really just want enough of this active to catch spintype after
      we request its value with asyncQueryParam().  The value comes back 
      in setValue() below and we set masSpinTypeBool.
    *******************************************************************/
    public void setAttribute(int t, String s){}
    public String getAttribute(int t){return "spintype";}
    public void setEditStatus(boolean s){}
    public void setEditMode(boolean s){}
    public void addDefChoice(String s){}
    public void addDefValue(String s){}
    public void setDefLabel(String s){}
    public void setDefColor(String s){}
    public void setDefLoc(int x, int y){}
    public void refresh(){}
    public void changeFont(){}
    public void updateValue(){}

    // This should be called when vnmr sends a value.  Use the value of
    // spintype received here to set the boolean for more efficient testing
    // in updateStatus()
    public void setValue(ParamIF p){
        masSpinTypeStr = new String(p.value);
        if(masSpinTypeStr.equals("mas"))
            masSpinTypeBool = true;
        else
            masSpinTypeBool = false;

        if(DebugOutput.isSetFor("masstatuslistener"))
            Messages.postDebug("masSpinTypeBool (spintype) set to " + 
                               masSpinTypeBool);

        // If a parameter for probeId1 came in before masSpinTypeStr was
        // set, we saved the value for use after masSpinTypeStr was set.
        // That is now.  If masSpinTypeBool is true AND probeNameHold != null,
        // then set the probe parameter now.
/*
 *      Setting probe parameter removed as part of VJ 40 project,
 */
/*
        if(masSpinTypeBool && probeNameHold != null) {
            updateParam("probe=\'" + probeNameHold + "\'");
            if(DebugOutput.isSetFor("masstatuslistener"))
                Messages.postDebug("masStatusListener setting probe param to "
                                   + probeNameHold
                                   + " after waiting for spintype to be set");
            probeNameHold = null;  // Clear this string

        }
 */
    }
    public void setShowValue(ParamIF p){}
    public void changeFocus(boolean s){}
    public ButtonIF getVnmrIF(){return expPanel;}
    public void setVnmrIF(ButtonIF vif){}
    public void destroy(){}
    public void setModalMode(boolean s){}
    public void sendVnmrCmd(){}
    public void setSizeRatio(double w, double h){}
    public Point getDefLoc(){return new Point(0,0);}


}


