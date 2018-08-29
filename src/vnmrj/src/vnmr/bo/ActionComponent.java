/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.event.ActionListener;

/**
 * Interface indicates that this object can send actionPerformed() calls to
 * registered listeners.
 */
public interface ActionComponent {

    /**
     * Adds an ActionListener to this component.
     * @param l The ActionListener to be added.
     */
    public void addActionListener(ActionListener l);

    /**
     * Sets the action command for this component.
     * @param actionCommand The action command for this component.
     */
    public void setActionCommand(String actionCommand);
}
