/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import static vnmr.apt.AptDefs.*;

public class TorqueData {

    private String m_torqueDataPath = null;
    private String m_dataHeader = "Motor\tPosn\tCurrent";

    public TorqueData(String torqueDataDir) {
        m_torqueDataPath = torqueDataDir + "/current.csv";
    }

    public void addTorqueDatum(String strGmi, String strPos, String strTorque) {
        if (m_torqueDataPath == null) {
            return;
        }
        String line = strGmi + "\t" + strPos + "\t" + strTorque;
        TuneUtilities.appendLog(m_torqueDataPath, line, m_dataHeader );
    }

    public void shiftTorqueData(String strGmi, String strShift) {
        if (m_torqueDataPath == null) {
            return;
        }
        StringBuffer sb = new StringBuffer();
        int shift = Integer.parseInt(strShift);
        String buf = TuneUtilities.readFile(m_torqueDataPath, true);
        String[] lines = buf.split(NL);
        for (String line : lines) {
            String[] toks = line.split("\t");
            if (toks.length == 3 && toks[0].equals(strGmi)) {
                int posn = Integer.parseInt(toks[1]);
                toks[1] = String.valueOf(posn - shift);
                line = toks[0] + "\t" + toks[1] + "\t" + toks[2];
            }
            sb.append(line).append(NL);
        }
        TuneUtilities.writeFile(m_torqueDataPath, sb.toString());
    }

}
