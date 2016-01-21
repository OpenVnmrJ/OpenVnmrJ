/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.lc;

import javax.swing.DefaultButtonModel;

/**
 * Derived from Glenn's TristateCheckBox.TristateDecorator class.
 */
public class LcTristateButtonModel extends DefaultButtonModel {

    public static final boolean SILENT = true;

    /**
     * Constructs a default <code>JButtonModel</code>.
     *
     */
    public LcTristateButtonModel() {
        stateMask = 0;
        setEnabled(true);
    }

    /**
     * Sets the state of the checkbox. Notifies ChangeListeners if the
     * new state is different from the previous one.
     * @param state The new state: true, false, or null (don't care).
     */
    public void setState(Boolean state) {
        setState(state, !SILENT);
    }

    /**
     * Sets the state of the checkbox without notifying ChangeListeners.
     * @param state The new state: true, false, or null (don't care).
     */
    public void setStateSilently(Boolean state) {
        setState(state, SILENT);
    }

    /**
     * Override the DefaultButtonModel to disable changes to the
     * "armed" state through this method.  Otherwise, checkbox losing
     * focus, which triggers a call to setArmed(false), causes the
     * checkbox state to go from null to true.
     */
    public void setArmed(boolean b) {
    }

    /**
     * Sets the state of the checkbox.
     * @param state The new state: true, false, or null (don't care).
     * @param silent If true, no ChangeEvent is sent to listeners.
     */
    public void setState(Boolean state, boolean silent) {
        if ((getState() == state) || !isEnabled()) {
            return;
        }

        if (state == Boolean.FALSE) {
            stateMask &= ~(ARMED | PRESSED | SELECTED);
        } else if (state==Boolean.TRUE) {
            stateMask &= ~(ARMED | PRESSED);
            stateMask |= SELECTED;
        } else {
            stateMask |= (ARMED | PRESSED | SELECTED);
        }

        if (!silent) {
            fireStateChanged();
        }
    }

    /**
     * The current state is embedded in the selection / armed
     * state of the model.
     *
     * We return the SELECTED state when the checkbox is selected
     * but not armed, DONT_CARE state when the checkbox is
     * selected and armed (grey) and NOT_SELECTED when the
     * checkbox is deselected.
     */
    public Boolean getState(){
        if(isSelected() && !isArmed()){
            // normal black tick
            return Boolean.TRUE;
        } else if(isSelected() && isArmed()){
            // don't care grey tick
            return null;
        } else{
            // normal deselected
            return Boolean.FALSE;
        }
    }

    /** We rotate between NOT_SELECTED, SELECTED and DONT_CARE.*/
    public void nextState(){
        Boolean current = getState();
        if(current == Boolean.FALSE){
            setState(Boolean.TRUE);
        } else if(current == Boolean.TRUE){
            setState(null);
        } else if(current == null){
            setState(Boolean.FALSE);
        }
    }
}
