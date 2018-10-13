/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import javax.swing.event.*;
import java.util.*;
import java.beans.*;
import vnmr.ui.*;
import vnmr.util.*;

/**
 * A two-line button that shows Infostat data.
 */
public class VLcStatusChart extends VStatusChart {
             
    /** constructor (for LayoutBuilder) */
    public VLcStatusChart(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
    }

    /** Update state from Infostat. */
    public void updateStatus(String msg)
    {
        //Messages.postDebug("VLcStatusChart.updateStatus(" + msg + ")");/*CMP*/
        if (msg == null) {
            return;
        }
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String key = tok.nextToken();
            String val = "";
            if (tok.hasMoreTokens()) {
                val = tok.nextToken("").trim(); // Get remainder of msg
            }
            if (key.equals(statkey)) {
                if (val !=null && !val.equals("-")) {
                    valstr = val;
                    setState(state);
                }
                /*System.out.println("Chart: statkey=" + statkey
                                   + ", val=" + val);/*CMP*/
            } 
            if (key.equals(statpar)) {
                if (val !=null && !val.equals("-")) {
                    /*System.out.println("Chart statpar=" + statpar
                                       + ", value=" + value);/*CMP*/
                    setState(state);
                }
            }
            if (key.equals(statset)) {
                if (val !=null && !val.equals("-")) {
                    setval = val;
                    try {
                        String num = val.substring(0, val.indexOf(' '));
                        setStatusValue(Double.parseDouble(num));
                    } catch (NumberFormatException nfe) {
                        Messages.postDebug("VLcStatusChart.updateStatus(): "
                                           + "Non-numeric value: " + msg);
                    } catch (StringIndexOutOfBoundsException sioobe) {
                        setStatusValue(0); // No value found
                    }
                    /*System.out.println("Chart statset=" + statset
                                       + ", setval=" + setval);/*CMP*/
                    setState(state);
                }
            }
        }
        repaint();
    }

    /** set state option. */
    protected void setState(int s) {
        setValueColor(Color.black);
        setBackground(Global.BGCOLOR);
        setValueString(valstr);
        //setToolTip();
    }

    /** set an attribute. */
    public void setAttribute(int attr, String c){
        switch (attr) {
          case STATKEY:
            statkey = c;
            updateStatus(ExpPanel.getStatusValue(statkey));
            break;
          case STATPAR:
            statpar = c;
            updateStatus(ExpPanel.getStatusValue(statpar));
            break;
          case STATSET:
            statset = c;
            updateStatus(ExpPanel.getStatusValue(statset));
            break;
          default:
            super.setAttribute(attr, c);
            break;
        }
    }

    /** get an attribute. */
    public String getAttribute(int attr) {
        switch (attr) {
          case STATKEY:
            return statkey;
          case STATPAR:
            return statpar;
          case STATSET:
            return statset;
          default:
            return super.getAttribute(attr);
        }
    }

    /** set tooltip text. */
    protected void setToolTip() {
        setToolTipText(null);
    }
}
