/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.event.ActionEvent;
import javax.swing.DefaultButtonModel;

public class ThreeButtonModel extends DefaultButtonModel {

    protected int mouseButton = 0;
    protected int modifierKeys = 0;
    protected String[] actionCommandList = new String[3 * 0xf];

	
    /**
     * Constructs a ThreeButtonModel
     *
     */
    public ThreeButtonModel() {
    }
	
    /**
     * Sets the actionCommand that gets sent when the button is activated
     * with the n'th mouse button and the modifiers in modMask.
     */
    public void setActionCommand(int n, int modMask, String actionCommand) {
	if (n < 1 || n > 3) {
	    return;
	}
	modMask &= 0xf;
	int index = modMask * (n - 1);
	actionCommandList[index] = actionCommand;
    }

    /**
     * Checks if the button is armed for the n'th mouse button.
     */
    public boolean isArmed(int n) {
	return ((stateMask & ARMED) != 0) && (mouseButton == n);
    }
	
    /**
     * Checks if the button is pressed by the n'th mouse button.
     */
    public boolean isPressed(int n) {
	return ((stateMask & PRESSED) != 0) && (mouseButton == n);
    }
	
    /**
     * Checks if the button is rolled over.
     */
    public boolean isRollover() {
	return (stateMask & ROLLOVER) != 0;
    }
	
    /**
     * Mark the button armed/disarmed by the n'th mouse button.
     */
    public void setArmed(boolean b, int n) {
	if (b && isArmed()) {
	    return;		// Button already armed by somebody
	}
	if((isArmed() == b) || !isEnabled()) {
	    return;
	}
	    
	if (b) {
	    stateMask |= ARMED;
	    mouseButton = n;
	} else {
	    stateMask &= ~ARMED;
	    mouseButton = 0;
	}
	    
	fireStateChanged();
    }

    /**
     * Mark the button pressed/unpressed by the n'th mouse button.
     */
    public void setPressed(boolean b, int n) {
	//System.out.println("setPressed(" + b + ", " + n + ")");/*CMP*/
	if (b && isPressed()) {
	    System.out.println("setPressed: n=" + n
			       + ", mouseButton=" + mouseButton);
	    return;		// Button already pressed by somebody
	}
	if((isPressed() == b) || !isEnabled()) {
	    return;		// No change requested (or button is disabled)
	}

	if (b) {
	    stateMask |= PRESSED;
	    mouseButton = n;
	} else {
	    stateMask &= ~PRESSED;
	    mouseButton = 0;
	}

	if(!isPressed() && isArmed() && (n != 0)) {
	    String cmd = Integer.toString(n);
	    setActionCommand(cmd); // Necessary!
	    fireActionPerformed(new ActionEvent(this,
						ActionEvent.ACTION_PERFORMED,
						cmd) );
	}
	fireStateChanged();
    }
}
