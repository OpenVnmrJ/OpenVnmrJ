/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import  vnmr.bo.*;

/**
 * The class that contains (or "installs") the application components
 * should implement this interface.
 *
 * @author Mark Cao
 */
public interface AppInstaller {
    /**
     * Install application for the given user.
     * @param user user
     */
    public void installApp(User user);
    public void exitAll();

} // interface AppInstaller
