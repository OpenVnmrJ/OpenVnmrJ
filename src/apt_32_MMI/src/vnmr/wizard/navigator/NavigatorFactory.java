/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.wizard.navigator;

public class NavigatorFactory
{
    private static Navigator navigator;

    public static Navigator getNavigator()
    {
        // Already created the navigator, return it to the caller
        if( navigator != null )
        {
            return navigator;
        }
        
        // See if the user specified one
        String navClass = System.getProperty( "vnmr.wizard.navigator.class" );
        if( navClass == null )
        {
            navClass = "vnmr.wizard.navigator.DefaultNavigator";
        }

        try
        {
            // Create the navigator and return it to the user
            navigator = ( Navigator )Class.forName( navClass ).newInstance();
            return navigator;
        }
        catch( Exception e )
        {
            e.printStackTrace();
            return null;
        }
    }
}
