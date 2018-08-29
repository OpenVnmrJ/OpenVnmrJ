/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.wizard.navigator;

import vnmr.wizard.JWizard;

public interface Navigator
{
    /**
     * Constant representing the "Back" button
     */
    public static final int BACK = 1;

    /**
     * Constant representing the "Next" button
     */
    public static final int NEXT = 2;

    /**
     * Initializes the Screen navigator
     * 
     * @param parent    The JWizard that owns the navigator
     */
    public void init( JWizard parent );

    /**
     * Returns the name of the next screen to display\
     * 
     * @param currentName   The name of the current screen
     * @param direction     The direction that the user is requesting to
     *                      go: BACK or NEXT
     */
    public String getNextScreen( String currentName, int direction );
}
