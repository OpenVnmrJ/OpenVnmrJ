/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.cryo;

public enum CryoPort {
    CRYOBAY(400, "CryoBay Cmd port"),
    UPS(401, "UPS Ctl port"),
    TEMPCTRL(403, "TempCtrl port"),
    DATA(404, "CryoBay Data port");

    private int m_port;
    private String m_label;

    CryoPort(int port, String label) {
        m_port = port;
        m_label = label;
    }

    public int getPort() {
        return m_port;
    }

    public String getLabel() {
        return m_label;
    }

    public String toString() {
        return getLabel();
    }
}
