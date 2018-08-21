/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.wizard.event;

public class WizardEvent extends java.util.EventObject {
    private String m_trigger;

    public WizardEvent(Object source, String trigger) {
        super(source);
        m_trigger = trigger;
    }

    public String getTrigger() {
        return m_trigger;
    }

}
