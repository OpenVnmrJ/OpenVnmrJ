/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.wizard.navigator;

import java.util.*;
import vnmr.wizard.JWizard;

/**
 * 
 */
public class DefaultNavigator implements Navigator
{
    protected JWizard parent;

    /**
     * Initializes the Screen navigator
     *
     * @param parent    The JWizard that owns the navigator
     */
    public void init(JWizard parent)
    {
        this.parent = parent;
    }

    /**
     * Returns the name of the next screen to display\
     *
     * @param currentName   The name of the current screen
     * @param direction     The direction that the user is requesting to
     *                      go: BACK or NEXT
     */
    public String getNextScreen(String currentName, int direction)
    {
        Map<Integer, String> wizardScreens = this.parent.getWizardScreens();
        int currentIndex = this.parent.getCurrentScreenIndex();
        if( direction == BACK )
        {
            this.parent.setCurrentScreenIndex( --currentIndex );
            return ( String )wizardScreens.get( new Integer( currentIndex ) );
        }
        else if( direction == NEXT )
        {
            this.parent.setCurrentScreenIndex( ++currentIndex );
            return ( String )wizardScreens.get( new Integer( currentIndex ) );
        }
        return "Unknown";
    }
}
