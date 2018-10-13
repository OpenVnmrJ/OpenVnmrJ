/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import javax.swing.*;

import  vnmr.util.*;
import  vnmr.bo.*;

/**
 * probe button
 *
 * @author Mark Cao
 */
public class ProbeButton extends VButton implements Runnable {
    // ==== static variables
    /** state: no probe */
    public static final int NO_PROBE = 0;
    /** state: wrong probe */
    public static final int WRONG_PROBE = 1;
    /** state: untuned probe */
    public static final int UNTUNED = 2;
    /** state: probe ready */
    public static final int READY = 3;
    /** state: probe error */
    public static final int ERROR = 4;
    /** icon for no probe */
    private static ImageIcon noProbeImage;
    /** icon for wrong probe */
    private static ImageIcon wrongProbeImage;
    /** icon for untuned probe */
    private static ImageIcon untunedProbeImage;
    /** icon for ready probe */
    private static ImageIcon readyProbeImage;
    /** icon for error probe */
    private static ImageIcon errorProbeImage;
    /** probe name (usually a number) */
    private String probeName;

    static {
        noProbeImage = Util.getImageIcon("probeNone.gif");
        wrongProbeImage = Util.getImageIcon("probeWrong.gif");
        untunedProbeImage = Util.getImageIcon("probeUntuned.gif");
        readyProbeImage = Util.getImageIcon("probeReady.gif");
        errorProbeImage = Util.getImageIcon("probeError.gif");
    }

    // ==== instance variables
    /** state of probe */
    private int state;
    private Thread  probeThread;

    /**
     * constructor
     * @param writable boolean
     */
    public ProbeButton(SessionShare sshare, ButtonIF vif, String typ)  {
        super(sshare,vif,typ);
        setHorizontalTextPosition(CENTER);
        setVerticalTextPosition(BOTTOM);
        setState(WRONG_PROBE);
        setProbeName("probe 3");
        probeThread = new Thread(this);
        probeThread.setName("ProbeButton");
        probeThread.start();
    } // ProbeButton()

    /**
     * set the button state
     */
    public void setState(int state) {
        this.state = state;
        switch (state) {
        case NO_PROBE:
            setIcon(noProbeImage);
            break;
        case WRONG_PROBE:
            setIcon(wrongProbeImage);
            break;
        case UNTUNED:
            setIcon(untunedProbeImage);
            break;
        case READY:
            setIcon(readyProbeImage);
            break;
        case ERROR:
            setIcon(errorProbeImage);
            break;
        }
    } // setState()

    /**
     * run thread
     */
    public void run() {
        for (;;) {
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {}
            switch (state) {
            case WRONG_PROBE:
                if (getIcon() == wrongProbeImage) {
                    setIcon(readyProbeImage);
                } else {
                    setIcon(wrongProbeImage);
                }
                break;
            case ERROR:
                if (getIcon() == errorProbeImage) {
                    setIcon(readyProbeImage);
                } else {
                    setIcon(errorProbeImage);
                }
                break;
            }
        } // infinite loop
    } // run()

    /**
     * set probe name
     * @param probeName
     */
    public void setProbeName(String probeName) {
                this.probeName = probeName;
            setAttribute(VALUE,probeName);
    } // setProbeName()

    public void quit() {
	try {
	   probeThread.join(5);
	} catch (InterruptedException e) {}
        probeThread = null;
    }
} // class ProbeButton

